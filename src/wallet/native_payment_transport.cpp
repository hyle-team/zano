// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#include "native_payment_transport.h"
#include "i_core_rpc_proxy.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "storages/portable_storage_template_helper.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <type_traits>
#include <limits>
#include <set>
#include <utility>

namespace tools { namespace native_payment {
exchange_context::exchange_context(const std::atomic<bool>& stop,
    std::chrono::steady_clock::time_point deadline) : stop_(stop), deadline_(deadline) {}
bool exchange_context::stopped() const noexcept {
  return stop_.load(std::memory_order_relaxed) || std::chrono::steady_clock::now() >= deadline_;
}
std::chrono::milliseconds exchange_context::remaining() const noexcept {
  if (stopped()) return std::chrono::milliseconds{0};
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      deadline_ - std::chrono::steady_clock::now());
}
response_sink::response_sink(const exchange_context& context) : context_(context) {}
bool response_sink::append(const unsigned char* data, std::size_t size) noexcept {
  if (failed_ || context_.stopped() || (!data && size) ||
      size > max_response_bytes - bytes_.size()) {
    failed_ = true; return false;
  }
  if (!size) return true;
  try {
    bytes_.append(reinterpret_cast<const char*>(data), size);
    return true;
  } catch (...) { failed_ = true; return false; }
}

namespace {
using v4 = currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4;
using out_entry = currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry;
constexpr char v4_id[] = "zano-native-payment-v4";
constexpr char v4_prefix[] =
    "{\"jsonrpc\":\"2.0\",\"id\":\"zano-native-payment-v4\",\"method\":\"getrandom_outs4\",\"params\":";
constexpr std::size_t json_charge_limit = 64 * 1024 * 1024;
constexpr std::size_t json_entry_limit = 65536;
constexpr std::size_t json_array_limit = 2 * max_requested_heights + max_batches;
constexpr std::size_t json_depth_limit = 8;
constexpr std::chrono::milliseconds json_work_budget{2000};

bool encode_v4(const v4::request& request, std::string& wire,
    const exchange_context& context) {
  // This serializer sees only the already-bounded, native-owned typed request.
  // Hostile replies never enter portable_storage's unbounded JSON loader.
  std::string params;
  if (context.stopped() || !epee::serialization::store_t_to_json(request, params) ||
      params.empty() || params.size() > max_request_bytes - sizeof(v4_prefix)) return false;
  wire.assign(v4_prefix);
  wire.append(params);
  wire.push_back('}');
  return wire.size() <= max_request_bytes && !context.stopped();
}

// Consumer-specific streaming reader for the canonical V4 JSON-RPC success
// envelope. No DOM, unknown-field skipping, recursive generic value reader or
// second codec pass. All accepted bytes, including the tail, are consumed.
class v4_json_reply final {
 public:
  v4_json_reply(const std::string& body, std::size_t retained_request_capacity,
      const v4::request& request, const exchange_context& context)
      : body_(body), request_(request), context_(context),
        deadline_(std::chrono::steady_clock::now() + json_work_budget) {
    // Charge retained wire/body capacities and bounded owned-request headroom.
    valid_ = charge(body.capacity()) && charge(retained_request_capacity) &&
        charge(max_request_bytes);
  }
  bool current() const noexcept {
    return valid_ && !context_.stopped() && std::chrono::steady_clock::now() < deadline_;
  }
  bool read(v4::response& value) {
    unsigned fields = 0;
    if (!current() || body_.empty() || body_.size() > max_response_bytes ||
        !object([&](const small_string& key) {
          if (key.is("jsonrpc")) {
            small_string version;
            return once(fields, 1) && small(version) && version.is("2.0");
          }
          if (key.is("id")) {
            small_string id;
            return once(fields, 2) && small(id) && id.is(v4_id);
          }
          if (key.is("result")) return once(fields, 4) && result(value);
          // Includes any JSON-RPC error envelope: never interpreted as success.
          return false;
        }) || fields != 7 || !space() || pos_ != body_.size()) return false;
    return current();
  }
 private:
  struct small_string {
    std::array<char, 64> bytes{};
    std::size_t size{0};
    bool is(const char* expected) const noexcept {
      const auto length = std::strlen(expected);
      return size == length && std::memcmp(bytes.data(), expected, length) == 0;
    }
  };
  static bool once(unsigned& fields, unsigned bit) noexcept {
    if (fields & bit) return false;
    fields |= bit; return true;
  }
  bool charge(std::size_t bytes) noexcept {
    if (bytes > json_charge_limit - charged_) { valid_ = false; return false; }
    charged_ += bytes; return true;
  }
  template<class T> bool grow(std::vector<T>& values, std::size_t limit) {
    if (!current() || values.size() >= limit) return false;
    if (values.size() < values.capacity()) return true;
    const auto capacity = values.capacity();
    const auto next = capacity ? capacity + std::min(capacity, limit - capacity) : 1;
    if (next > std::numeric_limits<std::size_t>::max() / sizeof(T) ||
        !charge(next * sizeof(T)) || !charge(256)) return false;
    // Charge the complete new allocation before reserve, without refunding old
    // capacity, so growth overlap and repeated small allocations are included.
    values.reserve(next);
    if (values.capacity() > next &&
        (values.capacity() > std::numeric_limits<std::size_t>::max() / sizeof(T) ||
         !charge((values.capacity() - next) * sizeof(T)))) return false;
    return current();
  }
  bool get(unsigned char& value) {
    // At most 256 wire bytes between clock/cancellation checks, also checked at
    // every structural entry/allocation and through final typed validation.
    if (pos_ >= body_.size() || ((pos_ & 255) == 0 && !current())) return false;
    value = static_cast<unsigned char>(body_[pos_++]); return true;
  }
  bool space() {
    while (pos_ < body_.size()) {
      const auto c = body_[pos_];
      if (c != ' ' && c != '\t' && c != '\n' && c != '\r') break;
      unsigned char ignored;
      if (!get(ignored)) return false;
    }
    return current();
  }
  bool token(char expected) {
    unsigned char value;
    return space() && get(value) && value == static_cast<unsigned char>(expected);
  }
  bool closing(char expected) {
    if (!space() || pos_ >= body_.size() || body_[pos_] != expected) return false;
    ++pos_; --depth_; return current();
  }
  bool open(char expected) {
    return current() && depth_ < json_depth_limit && sections_ < json_entry_limit &&
        token(expected) && (++depth_, ++sections_, true);
  }
  static int hex(unsigned char value) noexcept {
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
  }
  bool string_byte(unsigned char& value, bool& end) {
    if (!get(value)) return false;
    end = value == '"';
    if (end) return true;
    if (value < 0x20) return false;
    if (value != '\\') return true;
    if (!get(value)) return false;
    switch (value) {
      case '"': case '\\': case '/': return true;
      case 'b': value = '\b'; return true;
      case 'f': value = '\f'; return true;
      case 'n': value = '\n'; return true;
      case 'r': value = '\r'; return true;
      case 't': value = '\t'; return true;
      case 'u': {
        unsigned code = 0;
        for (unsigned i = 0; i < 4; ++i) {
          unsigned char digit;
          if (!get(digit) || hex(digit) < 0) return false;
          code = code * 16 + static_cast<unsigned>(hex(digit));
        }
        // epee's POD JSON strings encode bytes, not UTF-8 code points or hex
        // blobs. Its canonical writer emits raw high bytes and \u00XX controls.
        // Decode precisely that byte domain; reject unsupported Unicode.
        if (code > 255) return false;
        value = static_cast<unsigned char>(code); return true;
      }
      default: return false;
    }
  }
  bool small(small_string& value) {
    if (!token('"')) return false;
    for (;;) {
      unsigned char byte; bool end;
      if (!string_byte(byte, end)) return false;
      if (end) return current();
      if (byte < 0x20 || byte > 0x7e || value.size == value.bytes.size()) return false;
      value.bytes[value.size++] = static_cast<char>(byte);
    }
  }
  bool number(std::uint64_t& value) {
    if (!space() || pos_ >= body_.size()) return false;
    unsigned char digit;
    if (!get(digit) || digit < '0' || digit > '9') return false;
    value = digit - '0';
    const bool zero = digit == '0';
    while (pos_ < body_.size() && body_[pos_] >= '0' && body_[pos_] <= '9') {
      if (zero || !get(digit) || value > (std::numeric_limits<std::uint64_t>::max() - (digit - '0')) / 10)
        return false;
      value = value * 10 + (digit - '0');
    }
    return current();
  }
  template<class Field> bool object(Field field) {
    if (!open('{')) return false;
    if (closing('}')) return true;
    for (;;) {
      small_string key;
      if (!current() || entries_ == json_entry_limit || !small(key) || !token(':')) return false;
      ++entries_;
      if (!field(key)) return false;
      if (closing('}')) return true;
      if (!token(',')) return false;
    }
  }
  template<class T, class Item> bool array(std::vector<T>& values, std::size_t limit, Item item) {
    if (!open('[')) return false;
    if (closing(']')) return true;
    for (;;) {
      if (array_elements_ == json_array_limit || !grow(values, limit)) return false;
      ++array_elements_;
      values.emplace_back();
      if (!item(values.back())) return false;
      if (closing(']')) return true;
      if (!token(',')) return false;
    }
  }
  bool blob(std::vector<out_entry>& values) {
    static_assert(std::is_trivial<out_entry>::value, "Canonical RPC POD type required");
    if (!token('"')) return false;
    std::array<unsigned char, sizeof(out_entry)> entry{};
    std::size_t offset = 0;
    for (;;) {
      unsigned char byte; bool end;
      if (!string_byte(byte, end)) return false;
      if (end) return offset == 0 && current();
      entry[offset++] = byte;
      if (offset == entry.size()) {
        if (outputs_ == max_returned_outputs ||
            !grow(values, values.size() + max_returned_outputs - outputs_)) return false;
        values.emplace_back();
        // No unaligned cast and no intermediate decoded blob allocation.
        std::memcpy(&values.back(), entry.data(), entry.size());
        ++outputs_; offset = 0;
      }
    }
  }
  bool block(v4::outputs_in_block& value) {
    unsigned fields = 0;
    // Reserve a conservative logical charge for the existing final uniqueness
    // set's node, in addition to vector capacity and allocation charges.
    return charge(256) && object([&](const small_string& key) {
      if (key.is("block_height")) return once(fields, 1) && number(value.block_height);
      if (key.is("outs")) return once(fields, 2) && blob(value.outs);
      return false;
    }) && (fields & 1); // Canonical serializer omits an empty POD container.
  }
  bool batch(v4::blocks_batch& value, std::size_t index) {
    unsigned fields = 0;
    return object([&](const small_string& key) {
      return key.is("blocks") && once(fields, 1) &&
          array(value.blocks, 2 * request_.batches[index].heights.size(),
              [&](v4::outputs_in_block& item) { return block(item); });
    }); // Canonical serializer also omits an empty blocks vector.
  }
  bool result(v4::response& value) {
    unsigned fields = 0;
    return object([&](const small_string& key) {
      if (key.is("status")) {
        small_string status;
        if (!once(fields, 1) || !small(status) || !charge(256)) return false;
        value.status.assign(status.bytes.data(), status.size); return current();
      }
      return key.is("blocks_batches") && once(fields, 2) &&
          array(value.blocks_batches, request_.batches.size(), [&](v4::blocks_batch& item) {
            return batch(item, value.blocks_batches.size() - 1);
          });
    }) && (fields & 1);
  }
  const std::string& body_;
  const v4::request& request_;
  const exchange_context& context_;
  const std::chrono::steady_clock::time_point deadline_;
  std::size_t pos_{0}, depth_{0}, sections_{0}, entries_{0}, array_elements_{0}, outputs_{0}, charged_{0};
  bool valid_{true};
};
} // namespace

class payment_transport final : public i_core_proxy {
 public:
  payment_transport(std::shared_ptr<decoy_exchange> exchange, std::uint64_t blocks,
      const std::atomic<bool>& stop, construction_detail* detail) : exchange_(std::move(exchange)), blocks_(blocks),
      context_(stop, std::chrono::steady_clock::now() + exchange_budget), detail_(detail) {}
  bool call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4(
      const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request& request,
      currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response& output) override {
    epee::log_space::scoped_log_silence quiet;
    if (called_) return fail(construction_detail::ProxyAlreadyCalled);
    if (context_.stopped()) return fail(construction_detail::ProxyStopped);
    called_ = true; // One RPC, including a failed exchange; no internal retry.
    try {
      if (request.batches.empty() || request.batches.size() > max_batches ||
          request.look_up_strategy != LOOK_UP_STRATEGY_REGULAR_TX ||
          request.height_upper_limit >= blocks_) return fail(construction_detail::ProxyRequestRefused);
      std::size_t heights = 0;
      for (const auto& batch : request.batches) {
        if (batch.heights.empty() ||
            batch.heights.size() > max_requested_heights - heights) return fail(construction_detail::ProxyRequestRefused);
        heights += batch.heights.size();
        for (const auto height : batch.heights) if (height >= blocks_) return fail(construction_detail::ProxyRequestRefused);
      }
      // Pin a zero "latest" ceiling to this already-verified local session.
      auto owned = request;
      if (!owned.height_upper_limit) owned.height_upper_limit = blocks_ - 1;
      std::string wire;
      if (!encode_v4(owned, wire, context_) ||
          wire.empty() || wire.size() > max_request_bytes || context_.stopped()) return fail(construction_detail::ProxyRequestEncoding);
      response_sink sink(context_);
      const auto exchanged = exchange_->exchange_getrandomouts4_bin(
          reinterpret_cast<const unsigned char*>(wire.data()), wire.size(), context_, sink);
      if (exchanged != exchange_result::Complete) return fail(exchange_detail(exchanged));
      if (sink.failed_) return fail(construction_detail::ProxySinkRefused);
      if (sink.bytes_.empty()) return fail(construction_detail::ProxyEmptyResponse);
      if (context_.stopped()) return fail(construction_detail::ProxyStopped);
      v4_json_reply reader(sink.bytes_, wire.capacity(), owned, context_);
      currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response decoded{};
      if (!reader.read(decoded)) return fail(construction_detail::ProxyResponseDecode);
      if (decoded.status != API_RETURN_CODE_OK) return fail(construction_detail::ProxyResponseStatus);
      if (decoded.blocks_batches.size() != owned.batches.size()) return fail(construction_detail::ProxyResponseShape);
      std::size_t output_count = 0;
      for (std::size_t i = 0; i < decoded.blocks_batches.size(); ++i) {
        if (!reader.current()) return fail(context_.stopped() ? construction_detail::ProxyStopped : construction_detail::ProxyResponseDecode);
        const auto& batch = decoded.blocks_batches[i];
        const auto& requested = owned.batches[i].heights;
        // RPC v4 searches around each seed and may make a coinbase fallback
        // pass. Heights need not equal the seeds, but must be unique, bounded
        // and below the verified local ceiling. Do not downgrade wallet policy.
        if (batch.blocks.size() > 2 * requested.size()) return fail(construction_detail::ProxyResponseBounds);
        std::set<std::uint64_t> seen;
        for (const auto& block : batch.blocks) {
          if (!reader.current()) return fail(context_.stopped() ? construction_detail::ProxyStopped : construction_detail::ProxyResponseDecode);
          if (block.block_height > owned.height_upper_limit ||
              block.block_height >= blocks_ || !seen.insert(block.block_height).second ||
              block.outs.size() > max_returned_outputs - output_count) return fail(construction_detail::ProxyResponseBounds);
          output_count += block.outs.size();
        }
      }
      // wallet2 retains its original asset/commitment/decoy validation.
      if (context_.stopped()) return fail(construction_detail::ProxyStopped);
      if (!reader.current()) return fail(construction_detail::ProxyResponseDecode);
      output = std::move(decoded);
      return true;
    } catch (...) { return fail(construction_detail::ProxyException); } // No endpoint/body/exception export.
  }
 private:
  bool fail(construction_detail value) noexcept {
    if (detail_ && *detail_ == construction_detail::None) *detail_ = value;
    return false;
  }
  static construction_detail exchange_detail(exchange_result value) noexcept {
    switch (value) {
      case exchange_result::InvalidInput: return construction_detail::ExchangeInvalidInput;
      case exchange_result::Stopped: return construction_detail::ExchangeStopped;
      case exchange_result::Expired: return construction_detail::ExchangeExpired;
      case exchange_result::NoEnvironment: return construction_detail::ExchangeNoEnvironment;
      case exchange_result::JniFailure: return construction_detail::ExchangeJniFailure;
      case exchange_result::StartRefused: return construction_detail::ExchangeStartRefused;
      case exchange_result::PollFailure: return construction_detail::ExchangePollFailure;
      case exchange_result::InvalidChunk: return construction_detail::ExchangeInvalidChunk;
      case exchange_result::TooLarge: return construction_detail::ExchangeTooLarge;
      case exchange_result::AllocationFailure: return construction_detail::ExchangeAllocationFailure;
      case exchange_result::AppendRefused: return construction_detail::ExchangeAppendRefused;
      case exchange_result::PollLimit: return construction_detail::ExchangePollLimit;
      case exchange_result::PollCancelled: return construction_detail::ExchangePollCancelled;
      case exchange_result::PollDeadlineExpired: return construction_detail::ExchangePollDeadlineExpired;
      case exchange_result::PollHttpStatusRefused: return construction_detail::ExchangePollHttpStatusRefused;
      case exchange_result::PollEncodingRefused: return construction_detail::ExchangePollEncodingRefused;
      case exchange_result::PollResponseTooLarge: return construction_detail::ExchangePollResponseTooLarge;
      case exchange_result::PollExchangeFailed: return construction_detail::ExchangePollExchangeFailed;
      case exchange_result::PollLocalFailure: return construction_detail::ExchangePollLocalFailure;
      case exchange_result::PollInvalidArgument: return construction_detail::ExchangePollInvalidArgument;
      case exchange_result::PollThrew: return construction_detail::ExchangePollThrew;
      case exchange_result::PollJniException: return construction_detail::ExchangePollJniException;
      default: return construction_detail::ExchangeUnclassified;
    }
  }
  const std::shared_ptr<decoy_exchange> exchange_;
  const std::uint64_t blocks_;
  exchange_context context_;
  construction_detail* const detail_;
  bool called_{false};
};
std::shared_ptr<i_core_proxy> make_payment_transport(std::shared_ptr<decoy_exchange> exchange,
    std::uint64_t blocks, const std::atomic<bool>& stop, construction_detail* detail) noexcept {
  if (!exchange || blocks < 2 || stop.load(std::memory_order_relaxed)) return {};
  try { return std::make_shared<payment_transport>(std::move(exchange), blocks, stop, detail); }
  catch (...) { return {}; }
}
} }
