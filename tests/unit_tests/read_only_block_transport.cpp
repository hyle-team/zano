// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
//
// Source-only candidate. UNEXECUTED: compilation is not execution evidence.
// No network, wallet material or real daemon is used; the exchange is a fake
// and time is logical, so no test sleeps.
#include "gtest/gtest.h"
#include "wallet/read_only_block_transport.h"
#include "wallet/read_only_core_proxy.h"
#include "currency_core/currency_format_utils.h"
#include "currency_core/currency_format_utils_blocks.h"
#include "serialization/binary_archive.h"
#include "serialization/serialization.h"
#include "serialization/variant.h"
#include "epee/include/storages/portable_storage_template_helper.h"
#include <cstring>
#include <functional>
#include <limits>
#include <list>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace
{
  using namespace tools::read_only;
  using direct_request = currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request;
  using direct_response = currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response;
  using fast_response = currency::COMMAND_RPC_GET_BLOCKS_FAST::response;
  using std::chrono::milliseconds;

  // Logical monotonic time: advances only as a test directs.
  class logical_clock final : public monotonic_clock
  {
  public:
    mutable std::chrono::steady_clock::time_point current{std::chrono::hours(1)};
    std::chrono::steady_clock::duration per_call{0};
    std::function<void()> on_now;
    std::chrono::steady_clock::time_point now() const noexcept override
    {
      if (on_now) on_now();
      const auto value = current;
      current += per_call;
      return value;
    }
  };

  class fake_exchange final : public block_exchange
  {
  public:
    std::string body;
    exchange_status status = exchange_status::Ok;
    bool throws = false;
    size_t calls = 0;
    std::string last_request;
    std::function<void(const exchange_context&)> during;
    exchange_status exchange_getblocks_bin(const unsigned char* request, size_t size,
      const exchange_context& context, response_sink& sink) override
    {
      ++calls;
      last_request.assign(reinterpret_cast<const char*>(request), size);
      if (during) during(context);
      if (throws) throw std::runtime_error("private provider diagnostic");
      if (!sink.append(body.data(), body.size())) return exchange_status::Failed;
      return status;
    }
  };

  currency::transaction make_coinbase(uint64_t height, uint64_t amount)
  {
    currency::transaction tx{};
    tx.version = TRANSACTION_VERSION_INITAL; // Legacy layout: exercises the prefix transition.
    tx.vin.emplace_back(currency::txin_gen{static_cast<size_t>(height)});
    currency::tx_out_bare out{};
    out.amount = amount;
    out.target = currency::txout_to_key{};
    tx.vout.emplace_back(out);
    return tx;
  }

  currency::transaction make_transfer(uint64_t seed)
  {
    currency::transaction tx{};
    tx.version = TRANSACTION_VERSION_PRE_HF4; // Prefix and signature transitions.
    currency::txin_to_key in{};
    in.amount = seed;
    in.key_offsets.push_back(seed);
    tx.vin.push_back(in);
    for (uint64_t i = 0; i != 2; ++i)
    {
      currency::tx_out_bare out{};
      out.amount = seed + i;
      out.target = currency::txout_to_key{};
      tx.vout.emplace_back(out);
    }
    tx.extra.push_back(currency::tx_comment{"public fixture"});
    currency::NLSAG_sig sig{};
    sig.s.emplace_back();
    tx.signatures.push_back(sig);
    return tx;
  }

  // Post-HF4 layouts: Zarcanum outputs/inputs, CLSAG and range-proof vectors.
  // Below TRANSACTION_VERSION_POST_HF6 the outputs use the tx_out_v_v1 form.
  currency::transaction make_modern(uint64_t version, size_t key_offsets, size_t proof_points)
  {
    currency::transaction tx{};
    tx.version = version;
    if (version >= TRANSACTION_VERSION_POST_HF5)
      tx.hardfork_id = version >= TRANSACTION_VERSION_POST_HF6 ? 6 : 5;
    currency::txin_zc_input in{};
    for (size_t i = 0; i != key_offsets; ++i)
      in.key_offsets.push_back(static_cast<uint64_t>(i + 1));
    tx.vin.push_back(in);
    for (uint64_t i = 0; i != 2; ++i)
    {
      currency::tx_out_zarcanum out{};
      out.encrypted_amount = 100 + i;
      out.mix_attr = 1;
      tx.vout.push_back(out);
    }
    tx.extra.push_back(currency::zarcanum_tx_data_v1{10});
    currency::ZC_sig sig{};
    sig.clsags_ggx.r_g.resize(3);
    sig.clsags_ggx.r_x.resize(3);
    tx.signatures.push_back(sig);
    currency::zc_outs_range_proof range{};
    range.bpp.L.resize(proof_points);
    range.bpp.R.resize(proof_points);
    tx.proofs.push_back(range);
    tx.proofs.push_back(currency::zc_balance_proof{});
    return tx;
  }

  // Counts deep copies so default-copy vs strict-move selection is observable.
  struct counted
  {
    static size_t copies;
    uint64_t v = 0;
    counted() = default;
    counted(const counted& o) : v(o.v) { ++copies; }
    counted(counted&& o) noexcept : v(o.v) {}
    counted& operator=(const counted& o) { v = o.v; ++copies; return *this; }
    counted& operator=(counted&& o) noexcept { v = o.v; return *this; }
    BEGIN_SERIALIZE_OBJECT()
      VARINT_FIELD(v)
    END_SERIALIZE()
  };
  size_t counted::copies = 0;

  binary_decode_budget roomy_budget()
  {
    binary_decode_budget b;
    b.max_bytes = 67108864;
    b.max_elements = 262144;
    return b;
  }
}

VARIANT_TAG(binary_archive, counted, 0x71);

namespace
{
  // A contiguous public synthetic batch encoded with the existing serializers.
  struct chain_fixture
  {
    std::vector<currency::block> blocks;
    std::vector<currency::block_complete_entry> entries;
    uint64_t start = 0, current = 0, hardfork = 4;
    std::string status = API_RETURN_CODE_OK;

    chain_fixture(uint64_t start_height, size_t count, size_t txs_per_block)
      : start(start_height), current(start_height + count + 5)
    {
      crypto::hash previous = currency::null_hash;
      for (size_t i = 0; i != count; ++i)
      {
        currency::block b{};
        b.major_version = 1;
        b.timestamp = 1000 + i;
        b.prev_id = previous;
        b.miner_tx = make_coinbase(start + i, 100 + i);
        currency::block_complete_entry e{};
        e.coinbase_global_outs = {10 * i};
        for (size_t k = 0; k != txs_per_block; ++k)
        {
          const auto tx = make_transfer(i * 10 + k + 1);
          b.tx_hashes.push_back(currency::get_transaction_hash(tx));
          e.txs.push_back(t_serializable_object_to_blob(tx));
          decltype(e.tx_global_outs)::value_type outs{};
          outs.v = {1000 + i, 2000 + k};
          e.tx_global_outs.push_back(outs);
        }
        e.block = t_serializable_object_to_blob(b);
        previous = currency::get_block_hash(b);
        blocks.push_back(b);
        entries.push_back(e);
      }
    }
    void reencode(size_t i) { entries[i].block = t_serializable_object_to_blob(blocks[i]); }
    std::string body() const
    {
      fast_response f{};
      f.status = status;
      f.start_height = start;
      f.current_height = current;
      f.current_hardfork = hardfork;
      f.blocks.assign(entries.begin(), entries.end());
      return epee::serialization::store_t_to_binary(f);
    }
  };

  struct harness
  {
    std::shared_ptr<fake_exchange> exchange = std::make_shared<fake_exchange>();
    std::shared_ptr<logical_clock> clock = std::make_shared<logical_clock>();
    std::atomic<bool> stop{false};
    direct_request request{};
    direct_response response{};
    harness()
    {
      request.minimum_height = 7;
      request.block_ids = {crypto::hash{}, currency::gdefault_genesis};
      response.status = "stale"; // Must be cleared on every failure.
      response.current_hardfork = 99;
    }
    std::shared_ptr<tools::i_core_proxy> make(const block_transport_limits& limits = block_transport_limits{})
    {
      return make_block_transport(exchange, limits, clock);
    }
    transport_failure fetch(const block_transport_limits& limits = block_transport_limits{})
    {
      auto transport = make(limits);
      if (!transport) return transport_failure::NotBlockTransport;
      const bool ok = transport->call_COMMAND_RPC_GET_BLOCKS_DIRECT(request, response, stop);
      const auto failure = block_transport_last_failure(*transport);
      EXPECT_EQ(ok, failure == transport_failure::None);
      if (!ok) expect_empty();
      return failure;
    }
    void expect_empty() const
    {
      EXPECT_TRUE(response.status.empty());
      EXPECT_TRUE(response.blocks.empty());
      EXPECT_EQ(0u, response.start_height);
      EXPECT_EQ(0u, response.current_height);
      EXPECT_EQ(0u, response.current_hardfork);
    }
  };

  // Raw portable-storage writer for bodies the typed serializer cannot emit.
  namespace raw
  {
    std::string header()
    {
      std::string s;
      for (uint32_t v : {uint32_t(PORTABLE_STORAGE_SIGNATUREA), uint32_t(PORTABLE_STORAGE_SIGNATUREB)})
        for (int i = 0; i != 4; ++i) s.push_back(static_cast<char>((v >> (8 * i)) & 0xff));
      s.push_back(static_cast<char>(PORTABLE_STORAGE_FORMAT_VER));
      return s;
    }
    std::string count(size_t v) { return std::string(1, static_cast<char>(v << 2)); } // v < 64
    std::string name(const std::string& n) { return std::string(1, static_cast<char>(n.size())) + n; }
    std::string u64(uint64_t v)
    {
      std::string s(1, static_cast<char>(SERIALIZE_TYPE_UINT64));
      for (int i = 0; i != 8; ++i) s.push_back(static_cast<char>((v >> (8 * i)) & 0xff));
      return s;
    }
    std::string str(const std::string& v)
    { return std::string(1, static_cast<char>(SERIALIZE_TYPE_STRING)) + count(v.size()) + v; }
    std::string envelope_fields()
    {
      return name("current_hardfork") + u64(4) + name("current_height") + u64(10) +
        name("start_height") + u64(5) + name("status") + str(API_RETURN_CODE_OK);
    }
  }

  template<class T>
  bool strict_parse(const std::string& blob, T& value, binary_decode_budget& budget)
  {
    std::istringstream stream(blob);
    binary_archive<false> archive(stream);
    archive.set_decode_budget(&budget);
    return ::serialization::serialize(archive, value);
  }
}

TEST(read_only_block_transport, round_trip_retains_every_field_including_hardfork)
{
  harness h;
  chain_fixture c(40, 3, 2);
  h.exchange->body = c.body();
  ASSERT_EQ(transport_failure::None, h.fetch());
  EXPECT_EQ(1u, h.exchange->calls);
  EXPECT_EQ(API_RETURN_CODE_OK, h.response.status);
  EXPECT_EQ(40u, h.response.start_height);
  EXPECT_EQ(48u, h.response.current_height);
  // Regression oracle: the legacy HTTP adapter leaves this at zero.
  EXPECT_EQ(4u, h.response.current_hardfork);
  ASSERT_EQ(3u, h.response.blocks.size());
  size_t i = 0;
  for (const auto& entry : h.response.blocks)
  {
    ASSERT_TRUE(entry.block_ptr && entry.coinbase_ptr);
    EXPECT_EQ(currency::get_block_hash(c.blocks[i]), currency::get_block_hash(entry.block_ptr->bl));
    EXPECT_EQ(c.entries[i].coinbase_global_outs, entry.coinbase_ptr->m_global_output_indexes);
    ASSERT_EQ(2u, entry.txs_ptr.size());
    size_t k = 0;
    for (const auto& tx : entry.txs_ptr)
    {
      EXPECT_EQ(c.blocks[i].tx_hashes[k], currency::get_transaction_hash(tx->tx));
      EXPECT_EQ(c.entries[i].tx_global_outs[k].v, tx->m_global_output_indexes);
      ++k;
    }
    ++i;
  }
  // The request is the existing FAST encoding of the caller's history.
  currency::COMMAND_RPC_GET_BLOCKS_FAST::request sent{};
  ASSERT_TRUE(epee::serialization::load_t_from_binary(sent, h.exchange->last_request));
  EXPECT_EQ(7u, sent.minimum_height);
  EXPECT_EQ(h.request.block_ids, sent.block_ids);
}

TEST(read_only_block_transport, legacy_entry_uses_the_same_bounded_path)
{
  harness h;
  h.exchange->body = chain_fixture(40, 2, 1).body();
  auto transport = h.make();
  ASSERT_TRUE(transport != nullptr);
  EXPECT_TRUE(transport->call_COMMAND_RPC_GET_BLOCKS_DIRECT(h.request, h.response));
  EXPECT_EQ(4u, h.response.current_hardfork);
  block_transport_limits tight;
  // Production requires max_blob_bytes <= max_response_bytes; tighten both.
  tight.max_response_bytes = tight.max_blob_bytes = h.exchange->body.size() - 1;
  auto bounded = h.make(tight);
  ASSERT_TRUE(bounded != nullptr);
  EXPECT_FALSE(bounded->call_COMMAND_RPC_GET_BLOCKS_DIRECT(h.request, h.response));
  EXPECT_EQ(transport_failure::ResponseTooLarge, block_transport_last_failure(*bounded));
  h.expect_empty();
}

TEST(read_only_block_transport, answers_only_the_block_fetch)
{
  harness h;
  auto transport = h.make();
  ASSERT_TRUE(transport != nullptr);
  int code = 0; std::string body;
  currency::COMMAND_RPC_GET_INFO::request info_request{};
  currency::COMMAND_RPC_GET_INFO::response info{};
  EXPECT_FALSE(transport->call_COMMAND_RPC_INVOKE("/getblocks.bin", "x", code, body));
  EXPECT_FALSE(transport->call_COMMAND_RPC_GET_INFO(info_request, info));
  EXPECT_FALSE(transport->set_connection_addr("http://127.0.0.1:1"));
  EXPECT_EQ(0u, h.exchange->calls);
  EXPECT_EQ(transport_failure::NotBlockTransport,
    block_transport_last_failure(tools::read_only::core_proxy(transport)));
}

TEST(read_only_block_transport, stop_before_call_never_exchanges_or_clears_token)
{
  harness h;
  h.exchange->body = chain_fixture(40, 2, 1).body();
  h.stop = true;
  EXPECT_EQ(transport_failure::Stopped, h.fetch());
  EXPECT_EQ(0u, h.exchange->calls);
  EXPECT_TRUE(h.stop.load());
}

TEST(read_only_block_transport, stop_during_callback_discards_late_data)
{
  harness h;
  h.exchange->body = chain_fixture(40, 2, 1).body();
  h.exchange->during = [&](const exchange_context& context) {
    EXPECT_FALSE(context.should_abort());
    h.stop = true;
    EXPECT_TRUE(context.stop_requested());
  };
  EXPECT_EQ(transport_failure::Stopped, h.fetch());
  EXPECT_EQ(1u, h.exchange->calls);
  EXPECT_TRUE(h.stop.load());
}

TEST(read_only_block_transport, exchange_deadline_is_exclusive_and_late_data_is_rejected)
{
  block_transport_limits limits;
  limits.exchange_timeout = milliseconds(1000);
  harness late;
  late.exchange->body = chain_fixture(40, 2, 1).body();
  late.exchange->during = [&](const exchange_context& context) {
    late.clock->current = context.deadline(); // Exactly at the deadline.
    EXPECT_TRUE(context.expired());
  };
  EXPECT_EQ(transport_failure::ExchangeDeadline, late.fetch(limits));
  EXPECT_EQ(1u, late.exchange->calls);
  harness in_time;
  in_time.exchange->body = late.exchange->body;
  in_time.exchange->during = [&](const exchange_context& context) {
    in_time.clock->current = context.deadline() - std::chrono::nanoseconds(1);
  };
  EXPECT_EQ(transport_failure::None, in_time.fetch(limits));
}

TEST(read_only_block_transport, decode_deadline_is_cooperative_and_separate)
{
  block_transport_limits limits;
  limits.decode_timeout = milliseconds(1000);
  harness h;
  h.exchange->body = chain_fixture(40, 3, 2).body();
  // Logical time starts moving only after the exchange returns.
  h.exchange->during = [&](const exchange_context&) { h.clock->per_call = milliseconds(400); };
  EXPECT_EQ(transport_failure::DecodeDeadline, h.fetch(limits));
  EXPECT_EQ(1u, h.exchange->calls);
  EXPECT_FALSE(h.stop.load());
}

TEST(read_only_block_transport, stop_during_decode_is_observed_without_clearing_token)
{
  harness h;
  h.exchange->body = chain_fixture(40, 3, 2).body();
  size_t polls = 0;
  h.exchange->during = [&](const exchange_context&) {
    h.clock->on_now = [&] { if (++polls == 4) h.stop = true; };
  };
  EXPECT_EQ(transport_failure::Stopped, h.fetch());
  EXPECT_GE(polls, 4u);
  EXPECT_TRUE(h.stop.load());
}

TEST(read_only_block_transport, callback_failure_and_exception_are_fixed_and_not_retried)
{
  harness failed;
  failed.exchange->body = chain_fixture(40, 2, 1).body();
  failed.exchange->status = exchange_status::Failed;
  EXPECT_EQ(transport_failure::ExchangeFailed, failed.fetch());
  EXPECT_EQ(1u, failed.exchange->calls);
  harness thrown;
  thrown.exchange->throws = true;
  EXPECT_EQ(transport_failure::ExchangeFailed, thrown.fetch());
  EXPECT_EQ(1u, thrown.exchange->calls);
}

TEST(read_only_block_transport, reentrant_fetch_is_refused)
{
  harness h;
  h.exchange->body = chain_fixture(40, 2, 1).body();
  auto transport = h.make();
  ASSERT_TRUE(transport != nullptr);
  h.exchange->during = [&](const exchange_context&) {
    direct_response nested{};
    EXPECT_FALSE(transport->call_COMMAND_RPC_GET_BLOCKS_DIRECT(h.request, nested, h.stop));
    EXPECT_EQ(transport_failure::Busy, block_transport_last_failure(*transport));
  };
  EXPECT_TRUE(transport->call_COMMAND_RPC_GET_BLOCKS_DIRECT(h.request, h.response, h.stop));
  EXPECT_EQ(1u, h.exchange->calls);
}

TEST(read_only_block_transport, request_and_response_caps_have_exact_boundaries)
{
  harness h;
  h.exchange->body = chain_fixture(40, 2, 1).body();
  block_transport_limits limits;
  // Blob cap tightened with the body cap (both valid, blobs are far smaller),
  // so the factory accepts both settings and the sink boundary is reached.
  limits.max_response_bytes = limits.max_blob_bytes = h.exchange->body.size();
  ASSERT_TRUE(block_transport_limits_valid(limits));
  EXPECT_EQ(transport_failure::None, h.fetch(limits));
  limits.max_response_bytes = limits.max_blob_bytes = h.exchange->body.size() - 1;
  ASSERT_TRUE(block_transport_limits_valid(limits));
  EXPECT_EQ(transport_failure::ResponseTooLarge, h.fetch(limits));

  harness history;
  history.exchange->body = h.exchange->body;
  history.request.block_ids.clear();
  EXPECT_EQ(transport_failure::InvalidRequest, history.fetch());
  history.request.block_ids.assign(65536 / sizeof(crypto::hash) + 1, crypto::hash{});
  EXPECT_EQ(transport_failure::RequestTooLarge, history.fetch());
  EXPECT_EQ(0u, history.exchange->calls);
}

TEST(read_only_block_transport, rejects_malformed_truncated_trailing_and_duplicate_bodies)
{
  const std::string good = chain_fixture(40, 2, 1).body();
  harness truncated; truncated.exchange->body = good.substr(0, good.size() - 1);
  EXPECT_EQ(transport_failure::MalformedResponse, truncated.fetch());
  harness trailing; trailing.exchange->body = good + std::string(1, '\0');
  EXPECT_EQ(transport_failure::MalformedResponse, trailing.fetch());
  harness empty; empty.exchange->body.clear();
  EXPECT_EQ(transport_failure::MalformedResponse, empty.fetch());

  const std::string duplicate = raw::header() + raw::count(5) + raw::envelope_fields() +
    raw::name("status") + raw::str(API_RETURN_CODE_OK);
  harness dup; dup.exchange->body = duplicate;
  EXPECT_EQ(transport_failure::MalformedResponse, dup.fetch());
  // Compatibility: legacy default loading still accepts both unchanged.
  epee::serialization::portable_storage legacy;
  EXPECT_TRUE(legacy.load_from_binary(good + std::string(1, '\0')));
  EXPECT_TRUE(legacy.load_from_binary(duplicate));
}

TEST(read_only_block_transport, rejects_missing_wrong_and_unknown_fields)
{
  harness valid; valid.exchange->body = raw::header() + raw::count(4) + raw::envelope_fields();
  ASSERT_EQ(transport_failure::None, valid.fetch());
  EXPECT_EQ(4u, valid.response.current_hardfork);
  EXPECT_TRUE(valid.response.blocks.empty());

  harness missing;
  missing.exchange->body = raw::header() + raw::count(3) + raw::name("current_height") + raw::u64(10) +
    raw::name("start_height") + raw::u64(5) + raw::name("status") + raw::str(API_RETURN_CODE_OK);
  EXPECT_EQ(transport_failure::MalformedResponse, missing.fetch());
  harness wrong;
  wrong.exchange->body = raw::header() + raw::count(4) + raw::name("current_hardfork") + raw::str("4") +
    raw::name("current_height") + raw::u64(10) + raw::name("start_height") + raw::u64(5) +
    raw::name("status") + raw::str(API_RETURN_CODE_OK);
  EXPECT_EQ(transport_failure::MalformedResponse, wrong.fetch());
  harness unknown;
  unknown.exchange->body = raw::header() + raw::count(5) + raw::name("aaa") + raw::u64(1) + raw::envelope_fields();
  EXPECT_EQ(transport_failure::MalformedResponse, unknown.fetch());
  harness blocks_type;
  blocks_type.exchange->body = raw::header() + raw::count(5) + raw::name("blocks") + raw::str("x") + raw::envelope_fields();
  EXPECT_EQ(transport_failure::MalformedResponse, blocks_type.fetch());
}

TEST(read_only_block_transport, count_limits_have_exact_boundaries)
{
  // Two blocks, one transaction each: 4 transaction objects, 6 output indices.
  chain_fixture c(40, 2, 1);
  size_t largest_blob = 0;
  for (const auto& e : c.entries)
  {
    largest_blob = std::max(largest_blob, e.block.size());
    for (const auto& tx : e.txs) largest_blob = std::max(largest_blob, tx.size());
  }
  auto check = [&](block_transport_limits limits, transport_failure expected) {
    harness h; h.exchange->body = c.body();
    EXPECT_EQ(expected, h.fetch(limits));
  };
  block_transport_limits l;
  l.max_blocks = 2; l.max_transactions = 4; l.max_output_indices = 6; l.max_blob_bytes = largest_blob;
  check(l, transport_failure::None);
  auto blocks = l; blocks.max_blocks = 1;
  check(blocks, transport_failure::LimitExceeded);
  auto txs = l; txs.max_transactions = 3;
  check(txs, transport_failure::LimitExceeded);
  auto outs = l; outs.max_output_indices = 5;
  check(outs, transport_failure::LimitExceeded);
  auto blob = l; blob.max_blob_bytes = largest_blob - 1;
  check(blob, transport_failure::MalformedResponse); // Refused by the outer string cap.
  auto depth = l; depth.max_nesting_depth = 6; // Exactly the schema depth.
  check(depth, transport_failure::None);
}

TEST(read_only_block_transport, inner_budgets_are_enforced_at_transport_level)
{
  chain_fixture c(40, 2, 1);
  harness bytes; bytes.exchange->body = c.body();
  block_transport_limits small;
  small.max_decode_allocation_bytes = 512; // Below one block's intermediate objects.
  EXPECT_EQ(transport_failure::LimitExceeded, bytes.fetch(small));
  harness elements; elements.exchange->body = c.body();
  block_transport_limits few;
  few.max_collection_elements = 1;
  EXPECT_EQ(transport_failure::LimitExceeded, elements.fetch(few));
}

TEST(read_only_block_transport, validates_the_unselected_tail)
{
  // The scanner may select only a prefix; the transport validates everything.
  {
    chain_fixture c(40, 3, 1);
    c.blocks[2].prev_id = currency::null_hash; c.reencode(2);
    harness h; h.exchange->body = c.body();
    EXPECT_EQ(transport_failure::InvalidResponse, h.fetch());
  }
  {
    chain_fixture c(40, 3, 1);
    c.blocks[2].tx_hashes[0] = currency::null_hash; c.reencode(2);
    harness h; h.exchange->body = c.body();
    EXPECT_EQ(transport_failure::InvalidResponse, h.fetch());
  }
  {
    chain_fixture c(40, 3, 1);
    c.entries[2].tx_global_outs[0].v.push_back(1);
    harness h; h.exchange->body = c.body();
    EXPECT_EQ(transport_failure::InvalidResponse, h.fetch());
  }
  {
    chain_fixture c(40, 3, 1);
    c.entries[2].coinbase_global_outs.clear();
    harness h; h.exchange->body = c.body();
    EXPECT_EQ(transport_failure::InvalidResponse, h.fetch());
  }
  {
    chain_fixture c(40, 3, 1);
    boost::get<currency::txin_gen>(c.blocks[2].miner_tx.vin.front()).height = 99; c.reencode(2);
    harness h; h.exchange->body = c.body();
    EXPECT_EQ(transport_failure::InvalidResponse, h.fetch());
  }
  {
    chain_fixture c(40, 3, 1);
    c.current = 42; // Three blocks from 40 cannot fit below height 42.
    harness h; h.exchange->body = c.body();
    EXPECT_EQ(transport_failure::InvalidResponse, h.fetch());
  }
  {
    chain_fixture c(40, 3, 1);
    c.entries[2].block.push_back('\0'); // Blob must be consumed completely.
    harness h; h.exchange->body = c.body();
    EXPECT_EQ(transport_failure::MalformedResponse, h.fetch());
  }
}

TEST(read_only_block_transport, non_ok_status_is_published_without_blocks)
{
  chain_fixture busy(40, 0, 0);
  busy.status = API_RETURN_CODE_BUSY;
  harness h; h.exchange->body = busy.body();
  ASSERT_EQ(transport_failure::None, h.fetch());
  EXPECT_EQ(API_RETURN_CODE_BUSY, h.response.status);
  chain_fixture inconsistent(40, 1, 0);
  inconsistent.status = API_RETURN_CODE_BUSY;
  harness bad; bad.exchange->body = inconsistent.body();
  EXPECT_EQ(transport_failure::InvalidResponse, bad.fetch());
}

TEST(read_only_block_transport, factory_rejects_invalid_or_excessive_settings)
{
  auto exchange = std::make_shared<fake_exchange>();
  EXPECT_TRUE(block_transport_limits_valid(block_transport_limits{}));
  EXPECT_TRUE(make_block_transport(exchange) != nullptr);
  EXPECT_TRUE(make_block_transport(nullptr) == nullptr);
  const block_transport_limits max{};
  std::vector<std::function<void(block_transport_limits&)>> over = {
    [&](block_transport_limits& l) { l.max_request_bytes = max.max_request_bytes + 1; },
    [&](block_transport_limits& l) { l.max_response_bytes = max.max_response_bytes + 1; },
    [&](block_transport_limits& l) { l.max_blob_bytes = max.max_blob_bytes + 1; },
    [&](block_transport_limits& l) { l.max_blocks = max.max_blocks + 1; },
    [&](block_transport_limits& l) { l.max_transactions = max.max_transactions + 1; },
    [&](block_transport_limits& l) { l.max_output_indices = max.max_output_indices + 1; },
    [&](block_transport_limits& l) { l.max_storage_entries = max.max_storage_entries + 1; },
    [&](block_transport_limits& l) { l.max_storage_sections = max.max_storage_sections + 1; },
    [&](block_transport_limits& l) { l.max_storage_array_elements = max.max_storage_array_elements + 1; },
    [&](block_transport_limits& l) { l.max_nesting_depth = max.max_nesting_depth + 1; },
    [&](block_transport_limits& l) { l.max_collection_elements = max.max_collection_elements + 1; },
    [&](block_transport_limits& l) { l.max_decode_allocation_bytes = max.max_decode_allocation_bytes + 1; },
    [&](block_transport_limits& l) { l.exchange_timeout = max.exchange_timeout + milliseconds(1); },
    [&](block_transport_limits& l) { l.decode_timeout = max.decode_timeout + milliseconds(1); },
    [&](block_transport_limits& l) { l.max_response_bytes = 0; },
    [&](block_transport_limits& l) { l.exchange_timeout = milliseconds(0); },
    [&](block_transport_limits& l) { l.decode_timeout = milliseconds(-1); },
    // Contradictions.
    [&](block_transport_limits& l) { l.max_blob_bytes = 4096; l.max_response_bytes = 4095; },
    [&](block_transport_limits& l) { l.max_blocks = 10; l.max_transactions = 9; },
    [&](block_transport_limits& l) { l.max_nesting_depth = 5; },
    [&](block_transport_limits& l) { l.max_request_bytes = 127; },
    [&](block_transport_limits& l) { l.max_output_indices = 10; l.max_storage_array_elements = 9; },
  };
  for (const auto& change : over)
  {
    block_transport_limits l;
    change(l);
    EXPECT_FALSE(block_transport_limits_valid(l));
    EXPECT_TRUE(make_block_transport(exchange, l) == nullptr);
  }
  EXPECT_EQ(0u, exchange->calls);
}

TEST(read_only_block_transport, read_only_proxy_allows_one_fetch_across_both_entries)
{
  harness h;
  h.exchange->body = chain_fixture(40, 2, 1).body();
  tools::read_only::core_proxy proxy(h.make());
  proxy.begin_step();
  EXPECT_TRUE(proxy.call_COMMAND_RPC_GET_BLOCKS_DIRECT(h.request, h.response, h.stop));
  EXPECT_FALSE(proxy.call_COMMAND_RPC_GET_BLOCKS_DIRECT(h.request, h.response));
  EXPECT_FALSE(proxy.call_COMMAND_RPC_GET_BLOCKS_DIRECT(h.request, h.response, h.stop));
  EXPECT_TRUE(proxy.denied());
  EXPECT_EQ(1u, h.exchange->calls);
}

TEST(read_only_block_transport, sink_refuses_overflow_without_trimming)
{
  // The sink is only constructible by core; drive it through a transport.
  harness h;
  h.exchange->body = chain_fixture(40, 2, 1).body();
  block_transport_limits limits;
  limits.max_response_bytes = limits.max_blob_bytes = h.exchange->body.size();
  auto transport = h.make(limits);
  ASSERT_TRUE(transport != nullptr);
  EXPECT_TRUE(transport->call_COMMAND_RPC_GET_BLOCKS_DIRECT(h.request, h.response, h.stop));
  h.exchange->body.push_back('\0'); // One byte over the cap: refused whole.
  EXPECT_FALSE(transport->call_COMMAND_RPC_GET_BLOCKS_DIRECT(h.request, h.response, h.stop));
  EXPECT_EQ(transport_failure::ResponseTooLarge, block_transport_last_failure(*transport));
  h.expect_empty();
  EXPECT_EQ(2u, h.exchange->calls);
}

// ---- Shared serializer hooks: strict opt-in vs unchanged default ----------

TEST(read_only_block_transport_serialization, budget_is_inclusive_and_checked)
{
  binary_decode_budget b;
  b.max_bytes = 100;
  b.max_elements = 10;
  EXPECT_TRUE(b.charge(10, 10, 10)); // Exactly at both maxima.
  EXPECT_FALSE(b.charge(0, 0, 1));
  EXPECT_TRUE(b.exhausted);
  binary_decode_budget overflow;
  overflow.max_bytes = UINT64_MAX;
  overflow.max_elements = 1;
  EXPECT_FALSE(overflow.charge(UINT64_MAX, 2, 0)); // Checked multiplication.
  binary_decode_budget stopped;
  stopped.max_bytes = 100;
  stopped.max_elements = 100;
  stopped.should_stop = [](void*) { return true; };
  EXPECT_FALSE(stopped.charge(0, 0, 0));
  EXPECT_TRUE(stopped.cancelled);
  EXPECT_FALSE(stopped.exhausted);
}

TEST(read_only_block_transport_serialization, amplified_collection_is_refused_before_reserving)
{
  currency::transaction tx = make_coinbase(1, 1);
  const size_t n = 20000;
  for (size_t i = 0; i != n; ++i)
    tx.extra.push_back(currency::etc_tx_time{1});
  const std::string blob = t_serializable_object_to_blob(tx);
  ASSERT_LT(blob.size(), 3 * n); // About two input bytes per element...
  ASSERT_GT(n * sizeof(currency::extra_v), 8 * blob.size()); // ...amplified in memory.

  // Compatibility: the unbudgeted legacy path still decodes it.
  currency::transaction legacy{};
  ASSERT_TRUE(t_unserializable_object_from_blob(legacy, blob));
  EXPECT_EQ(n, legacy.extra.size());

  // Oracle: without the strict hook this decode would succeed like legacy.
  binary_decode_budget budget;
  budget.max_bytes = 8 * blob.size();
  budget.max_elements = 262144;
  currency::transaction strict{};
  EXPECT_FALSE(strict_parse(blob, strict, budget));
  EXPECT_TRUE(budget.exhausted);
  EXPECT_LE(budget.bytes, budget.max_bytes);
  EXPECT_TRUE(strict.extra.empty());

  // Within budget: identical data and identical re-encoding.
  binary_decode_budget enough;
  enough.max_bytes = 67108864;
  enough.max_elements = 262144;
  currency::transaction decoded{};
  ASSERT_TRUE(strict_parse(blob, decoded, enough));
  EXPECT_EQ(blob, t_serializable_object_to_blob(decoded));
  EXPECT_GE(enough.bytes, n * sizeof(currency::extra_v));
  EXPECT_GE(enough.elements, n);

  // Exact boundaries of the measured charge (deterministic for one blob).
  binary_decode_budget exact_bytes = enough;
  exact_bytes.max_bytes = enough.bytes; exact_bytes.bytes = 0; exact_bytes.elements = 0;
  currency::transaction a{};
  EXPECT_TRUE(strict_parse(blob, a, exact_bytes));
  exact_bytes.max_bytes = enough.bytes - 1; exact_bytes.bytes = 0; exact_bytes.elements = 0;
  currency::transaction b{};
  EXPECT_FALSE(strict_parse(blob, b, exact_bytes));
  binary_decode_budget exact_elements = enough;
  exact_elements.max_elements = enough.elements; exact_elements.bytes = 0; exact_elements.elements = 0;
  currency::transaction c{};
  EXPECT_TRUE(strict_parse(blob, c, exact_elements));
  exact_elements.max_elements = enough.elements - 1; exact_elements.bytes = 0; exact_elements.elements = 0;
  currency::transaction d{};
  EXPECT_FALSE(strict_parse(blob, d, exact_elements));
}

TEST(read_only_block_transport_serialization, legacy_conversions_move_identical_values)
{
  // PRE_HF4 layout goes through transaction_prefix_v1/transaction_v1 (the
  // separate post-HF4/pre-HF6 tx_out_v_v1 form is covered by
  // supported_formats_decode_and_hash_identically); strict and default
  // decoding must agree exactly.
  const auto tx = make_transfer(5);
  const std::string blob = t_serializable_object_to_blob(tx);
  currency::transaction legacy{};
  ASSERT_TRUE(t_unserializable_object_from_blob(legacy, blob));
  binary_decode_budget budget;
  budget.max_bytes = 1 << 20;
  budget.max_elements = 1024;
  currency::transaction strict{};
  ASSERT_TRUE(strict_parse(blob, strict, budget));
  EXPECT_EQ(blob, t_serializable_object_to_blob(legacy));
  EXPECT_EQ(blob, t_serializable_object_to_blob(strict));
  EXPECT_EQ(currency::get_transaction_hash(tx), currency::get_transaction_hash(strict));
  // The conversion's destination vectors were charged, not only decoded ones.
  EXPECT_GE(budget.bytes, tx.vout.size() * sizeof(currency::tx_out_v) + tx.signatures.size() * sizeof(currency::signature_v));

  binary_decode_budget tight;
  tight.max_bytes = budget.bytes - 1;
  tight.max_elements = 1024;
  currency::transaction refused{};
  EXPECT_FALSE(strict_parse(blob, refused, tight));
  EXPECT_TRUE(tight.exhausted);
}

TEST(read_only_block_transport_serialization, strict_varint_and_string_paths)
{
  // A truncated varint is refused in strict mode.
  currency::txin_gen gen{};
  binary_decode_budget budget;
  budget.max_bytes = 1024;
  budget.max_elements = 16;
  EXPECT_FALSE(strict_parse(std::string(1, '\x80'), gen, budget));
  // A non-canonical varint (trailing zero continuation) is refused too.
  binary_decode_budget second = budget;
  EXPECT_FALSE(strict_parse(std::string("\x81\x00", 2), gen, second));
  // Strings: identical value to the default path; length charged exactly.
  currency::tx_comment comment{std::string(300, 'z')};
  const std::string blob = t_serializable_object_to_blob(comment);
  currency::tx_comment legacy{}, strict{};
  ASSERT_TRUE(t_unserializable_object_from_blob(legacy, blob));
  binary_decode_budget strings;
  strings.max_bytes = 300;
  strings.max_elements = 1;
  ASSERT_TRUE(strict_parse(blob, strict, strings));
  EXPECT_EQ(legacy.comment, strict.comment);
  EXPECT_EQ(300u, strings.bytes);
  binary_decode_budget short_strings;
  short_strings.max_bytes = 299;
  short_strings.max_elements = 1;
  currency::tx_comment refused{};
  EXPECT_FALSE(strict_parse(blob, refused, short_strings));
  EXPECT_TRUE(refused.comment.empty());
}

// Malformed bool bytes are only ever given to the strict path: loading them
// through the default path would itself form an invalid bool.
TEST(read_only_block_transport_serialization, strict_scalar_bool_accepts_only_zero_and_one)
{
  for (bool value : {false, true})
  {
    SCOPED_TRACE(value);
    const std::string blob = t_serializable_object_to_blob(value);
    ASSERT_EQ(std::string(1, value ? '\1' : '\0'), blob);
    bool legacy = !value, strict = !value;
    ASSERT_TRUE(t_unserializable_object_from_blob(legacy, blob));
    binary_decode_budget budget = roomy_budget();
    ASSERT_TRUE(strict_parse(blob, strict, budget));
    EXPECT_EQ(value, legacy);
    EXPECT_EQ(value, strict);
    EXPECT_EQ(blob, t_serializable_object_to_blob(strict));
  }
  for (unsigned char byte : {static_cast<unsigned char>(2), static_cast<unsigned char>(255)})
  {
    SCOPED_TRACE(static_cast<int>(byte));
    bool target = false; // Never assigned on refusal.
    binary_decode_budget budget = roomy_budget();
    EXPECT_FALSE(strict_parse(std::string(1, static_cast<char>(byte)), target, budget));
    EXPECT_FALSE(target);
  }
  bool truncated = true;
  binary_decode_budget budget = roomy_budget();
  EXPECT_FALSE(strict_parse(std::string(), truncated, budget));
  EXPECT_TRUE(truncated);
}

TEST(read_only_block_transport_serialization, strict_optional_presence_flags_accept_only_zero_and_one)
{
  // Flag byte is is_none: 1 = absent, 0 = value follows.
  const boost::optional<uint64_t> boost_some = uint64_t(5), boost_none;
  const std::optional<uint64_t> std_some = uint64_t(5), std_none;
  for (const std::string& blob : {t_serializable_object_to_blob(boost_some), t_serializable_object_to_blob(boost_none)})
  {
    boost::optional<uint64_t> legacy = uint64_t(7), strict = uint64_t(7);
    ASSERT_TRUE(t_unserializable_object_from_blob(legacy, blob));
    binary_decode_budget budget = roomy_budget();
    ASSERT_TRUE(strict_parse(blob, strict, budget));
    EXPECT_TRUE(legacy == strict); // Not EXPECT_EQ: optionals are not printable here.
    EXPECT_EQ(blob, t_serializable_object_to_blob(strict));
  }
  for (const std::string& blob : {t_serializable_object_to_blob(std_some), t_serializable_object_to_blob(std_none)})
  {
    std::optional<uint64_t> legacy = uint64_t(7), strict = uint64_t(7);
    ASSERT_TRUE(t_unserializable_object_from_blob(legacy, blob));
    binary_decode_budget budget = roomy_budget();
    ASSERT_TRUE(strict_parse(blob, strict, budget));
    EXPECT_TRUE(legacy == strict);
    EXPECT_EQ(blob, t_serializable_object_to_blob(strict));
  }
  ASSERT_EQ(std::string(1, '\1'), t_serializable_object_to_blob(boost_none));
  ASSERT_EQ(std::string(1, '\1'), t_serializable_object_to_blob(std_none));

  // The flag is refused before it selects reset() or a value: the sentinel
  // content survives, for a lone flag and for a flag followed by a value.
  for (unsigned char byte : {static_cast<unsigned char>(2), static_cast<unsigned char>(255)})
  {
    SCOPED_TRACE(static_cast<int>(byte));
    for (const std::string& blob : {std::string(1, static_cast<char>(byte)),
           std::string(1, static_cast<char>(byte)) + t_serializable_object_to_blob(boost_some).substr(1)})
    {
      boost::optional<uint64_t> b = uint64_t(7);
      binary_decode_budget b_budget = roomy_budget();
      EXPECT_FALSE(strict_parse(blob, b, b_budget));
      ASSERT_TRUE(b.has_value());
      EXPECT_EQ(7u, *b);
      std::optional<uint64_t> s = uint64_t(7);
      binary_decode_budget s_budget = roomy_budget();
      EXPECT_FALSE(strict_parse(blob, s, s_budget));
      ASSERT_TRUE(s.has_value());
      EXPECT_EQ(7u, *s);
    }
  }
  // Truncated: no flag byte at all.
  boost::optional<uint64_t> b = uint64_t(7);
  std::optional<uint64_t> s = uint64_t(7);
  binary_decode_budget b_budget = roomy_budget(), s_budget = roomy_budget();
  EXPECT_FALSE(strict_parse(std::string(), b, b_budget));
  EXPECT_FALSE(strict_parse(std::string(), s, s_budget));
  EXPECT_EQ(uint64_t(7), b.value_or(0));
  EXPECT_EQ(uint64_t(7), s.value_or(0));
}

TEST(read_only_block_transport_serialization, strict_asset_descriptor_bools_accept_only_zero_and_one)
{
  // Register operation (current layout, no legacy transition) whose optional
  // descriptor is version 0, so hidden_supply is the descriptor's last byte.
  currency::asset_descriptor_base descriptor{};
  descriptor.version = ASSET_DESCRIPTOR_BASE_HF4_VER;
  descriptor.ticker = "STRICTF1";
  descriptor.full_name = "strict bool fixture";
  descriptor.total_max_supply = 1000;
  descriptor.hidden_supply = false;
  const std::string descriptor_blob = t_serializable_object_to_blob(descriptor);
  ASSERT_EQ('\0', descriptor_blob.back());
  currency::asset_descriptor_operation op{};
  op.version = ASSET_DESCRIPTOR_OPERATION_HF5_VER;
  op.operation_type = ASSET_DESCRIPTOR_OPERATION_REGISTER;
  op.opt_descriptor = descriptor;

  // Reachable through the transport: the operation travels in a transaction's extra.
  currency::transaction tx = make_modern(TRANSACTION_VERSION_POST_HF6, 2, 4);
  tx.extra.push_back(op);
  const std::string op_blob = t_serializable_object_to_blob(op);
  const std::string tx_blob = t_serializable_object_to_blob(tx);

  // Positions in the operation: [varint version][operation_type][opt_amount_commitment flag]...
  ASSERT_EQ('\1', op_blob[2]); // opt_amount_commitment absent.
  const size_t op_descriptor = op_blob.find(descriptor_blob);
  ASSERT_NE(std::string::npos, op_descriptor);
  ASSERT_GT(op_descriptor, 0u);
  ASSERT_EQ('\0', op_blob[op_descriptor - 1]); // opt_descriptor present.
  const size_t op_hidden = op_descriptor + descriptor_blob.size() - 1;
  const size_t tx_descriptor = tx_blob.find(descriptor_blob);
  ASSERT_NE(std::string::npos, tx_descriptor);
  ASSERT_EQ(std::string::npos, tx_blob.find(descriptor_blob, tx_descriptor + 1));
  const size_t tx_hidden = tx_descriptor + descriptor_blob.size() - 1;

  // Valid 0/1 controls: strict and default agree, re-encode and hash identically.
  for (char hidden : {'\0', '\1'})
  {
    SCOPED_TRACE(static_cast<int>(hidden));
    std::string valid_op = op_blob;
    valid_op[op_hidden] = hidden;
    currency::asset_descriptor_operation legacy_op{}, strict_op{};
    ASSERT_TRUE(t_unserializable_object_from_blob(legacy_op, valid_op));
    binary_decode_budget op_budget = roomy_budget();
    ASSERT_TRUE(strict_parse(valid_op, strict_op, op_budget));
    ASSERT_TRUE(legacy_op.opt_descriptor.has_value() && strict_op.opt_descriptor.has_value());
    EXPECT_EQ(hidden == '\1', legacy_op.opt_descriptor->hidden_supply);
    EXPECT_EQ(hidden == '\1', strict_op.opt_descriptor->hidden_supply);
    EXPECT_EQ(valid_op, t_serializable_object_to_blob(strict_op));

    std::string valid_tx = tx_blob;
    valid_tx[tx_hidden] = hidden;
    currency::transaction legacy_tx{}, strict_tx{};
    ASSERT_TRUE(t_unserializable_object_from_blob(legacy_tx, valid_tx));
    binary_decode_budget tx_budget = roomy_budget();
    ASSERT_TRUE(strict_parse(valid_tx, strict_tx, tx_budget));
    EXPECT_EQ(valid_tx, t_serializable_object_to_blob(legacy_tx));
    EXPECT_EQ(valid_tx, t_serializable_object_to_blob(strict_tx));
    EXPECT_EQ(currency::get_transaction_hash(legacy_tx), currency::get_transaction_hash(strict_tx));
  }

  // Malformed hidden_supply and optional flags are refused in strict mode.
  for (unsigned char byte : {static_cast<unsigned char>(2), static_cast<unsigned char>(255)})
  {
    SCOPED_TRACE(static_cast<int>(byte));
    for (size_t position : {size_t(2), op_descriptor - 1, op_hidden})
    {
      SCOPED_TRACE(position);
      std::string bad = op_blob;
      bad[position] = static_cast<char>(byte);
      currency::asset_descriptor_operation refused{};
      binary_decode_budget budget = roomy_budget();
      EXPECT_FALSE(strict_parse(bad, refused, budget));
    }
    std::string bad_tx = tx_blob;
    bad_tx[tx_hidden] = static_cast<char>(byte);
    currency::transaction refused_tx{};
    binary_decode_budget tx_budget = roomy_budget();
    EXPECT_FALSE(strict_parse(bad_tx, refused_tx, tx_budget));
  }
  // Truncated immediately before hidden_supply and before the descriptor flag.
  for (size_t cut : {op_hidden, op_descriptor - 1})
  {
    SCOPED_TRACE(cut);
    currency::asset_descriptor_operation refused{};
    binary_decode_budget budget = roomy_budget();
    EXPECT_FALSE(strict_parse(op_blob.substr(0, cut), refused, budget));
  }
  currency::transaction truncated_tx{};
  binary_decode_budget truncated_budget = roomy_budget();
  EXPECT_FALSE(strict_parse(tx_blob.substr(0, tx_hidden), truncated_tx, truncated_budget));
}

TEST(read_only_block_transport_serialization, default_copies_and_strict_moves_are_selected_by_budget)
{
  // Regression oracle for exact default behavior: the unbudgeted path copies
  // each vector element and variant value as before; only strict moves.
  std::vector<counted> values(5);
  for (size_t i = 0; i != values.size(); ++i) values[i].v = i + 1;
  const std::string vector_blob = t_serializable_object_to_blob(values);
  counted::copies = 0;
  std::vector<counted> legacy;
  ASSERT_TRUE(t_unserializable_object_from_blob(legacy, vector_blob));
  EXPECT_EQ(values.size(), counted::copies);
  counted::copies = 0;
  std::vector<counted> strict;
  binary_decode_budget budget = roomy_budget();
  ASSERT_TRUE(strict_parse(vector_blob, strict, budget));
  EXPECT_EQ(0u, counted::copies);
  ASSERT_EQ(legacy.size(), strict.size());
  for (size_t i = 0; i != strict.size(); ++i) EXPECT_EQ(legacy[i].v, strict[i].v);

  boost::variant<counted> variant = counted{};
  boost::get<counted>(variant).v = 42;
  const std::string variant_blob = t_serializable_object_to_blob(variant);
  counted::copies = 0;
  boost::variant<counted> variant_legacy;
  ASSERT_TRUE(t_unserializable_object_from_blob(variant_legacy, variant_blob));
  EXPECT_GE(counted::copies, 1u);
  counted::copies = 0;
  boost::variant<counted> variant_strict;
  binary_decode_budget variant_budget = roomy_budget();
  ASSERT_TRUE(strict_parse(variant_blob, variant_strict, variant_budget));
  EXPECT_EQ(0u, counted::copies);
  EXPECT_EQ(42u, boost::get<counted>(variant_strict).v);
  EXPECT_EQ(42u, boost::get<counted>(variant_legacy).v);
}

TEST(read_only_block_transport_serialization, node_containers_fail_closed_only_in_strict_mode)
{
  // Not in the transaction/block graph: strict mode refuses them before any
  // node is created instead of claiming an approximate budget.
  const std::list<uint64_t> list{1, 2, 3};
  const std::set<std::string> set{"a", "b"};
  const std::unordered_map<uint64_t, uint64_t> map{{1, 2}, {3, 4}};
  const std::string list_blob = t_serializable_object_to_blob(list);
  const std::string set_blob = t_serializable_object_to_blob(set);
  const std::string map_blob = t_serializable_object_to_blob(map);

  std::list<uint64_t> list_default;
  std::set<std::string> set_default;
  std::unordered_map<uint64_t, uint64_t> map_default;
  ASSERT_TRUE(t_unserializable_object_from_blob(list_default, list_blob));
  ASSERT_TRUE(t_unserializable_object_from_blob(set_default, set_blob));
  ASSERT_TRUE(t_unserializable_object_from_blob(map_default, map_blob));
  EXPECT_EQ(list, list_default);
  EXPECT_EQ(set, set_default);
  EXPECT_EQ(map, map_default);

  binary_decode_budget b1 = roomy_budget(), b2 = roomy_budget(), b3 = roomy_budget();
  std::list<uint64_t> list_strict;
  std::set<std::string> set_strict;
  std::unordered_map<uint64_t, uint64_t> map_strict;
  EXPECT_FALSE(strict_parse(list_blob, list_strict, b1));
  EXPECT_FALSE(strict_parse(set_blob, set_strict, b2));
  EXPECT_FALSE(strict_parse(map_blob, map_strict, b3));
  EXPECT_TRUE(list_strict.empty() && set_strict.empty() && map_strict.empty());
  EXPECT_EQ(0u, b1.bytes + b2.bytes + b3.bytes);
}

TEST(read_only_block_transport_serialization, supported_formats_decode_and_hash_identically)
{
  struct format { const char* name; currency::transaction tx; };
  const std::vector<format> formats = {
    {"initial", make_coinbase(3, 7)},
    {"pre_hf4", make_transfer(9)},
    {"post_hf4_tx_out_v_v1", make_modern(TRANSACTION_VERSION_POST_HF4, 2, 4)},
    {"post_hf5_tx_out_v_v1", make_modern(TRANSACTION_VERSION_POST_HF5, 2, 4)},
    {"post_hf6", make_modern(TRANSACTION_VERSION_POST_HF6, 2, 4)},
  };
  for (const auto& f : formats)
  {
    SCOPED_TRACE(f.name);
    const std::string blob = t_serializable_object_to_blob(f.tx);
    currency::transaction legacy{}, strict{};
    ASSERT_TRUE(t_unserializable_object_from_blob(legacy, blob));
    binary_decode_budget budget = roomy_budget();
    ASSERT_TRUE(strict_parse(blob, strict, budget));
    EXPECT_EQ(blob, t_serializable_object_to_blob(legacy));
    EXPECT_EQ(blob, t_serializable_object_to_blob(strict));

    // Differential hash oracle: the strict fixed-buffer helper against the
    // existing default helper, on the original and the strictly decoded object.
    const crypto::hash expected = currency::get_transaction_hash(f.tx);
    EXPECT_EQ(expected, currency::get_transaction_hash(strict));
    const auto& prefix = static_cast<const currency::transaction_prefix&>(strict);
    const std::string prefix_blob = t_serializable_object_to_blob(prefix);
    ASSERT_LE(prefix_blob.size(), blob.size()); // A scratch sized by the source blob suffices.
    EXPECT_EQ(0, blob.compare(0, prefix_blob.size(), prefix_blob));
    std::vector<char> exact(prefix_blob.size());
    crypto::hash h = currency::null_hash;
    ASSERT_TRUE(currency::get_object_hash_in_buffer(prefix, exact, h));
    EXPECT_EQ(expected, h);
    // The buffer never grows: one byte short fails instead of reallocating.
    std::vector<char> short_by_one(prefix_blob.size() - 1);
    EXPECT_FALSE(currency::get_object_hash_in_buffer(prefix, short_by_one, h));
    EXPECT_EQ(prefix_blob.size() - 1, short_by_one.size());
  }
  // Distinct layouts, not two encodings of one version: HF6 outputs carry the
  // Zarcanum output version and payment id that the tx_out_v_v1 form omits.
  EXPECT_LT(t_serializable_object_to_blob(formats[3].tx).size(), t_serializable_object_to_blob(formats[4].tx).size());
}

TEST(read_only_block_transport_serialization, modern_proof_and_crypto_vectors_are_charged_before_allocation)
{
  const size_t points = 1000;
  const currency::transaction tx = make_modern(TRANSACTION_VERSION_POST_HF6, 2, points);
  const std::string blob = t_serializable_object_to_blob(tx);
  binary_decode_budget enough = roomy_budget();
  currency::transaction decoded{};
  ASSERT_TRUE(strict_parse(blob, decoded, enough));
  EXPECT_EQ(blob, t_serializable_object_to_blob(decoded));
  using point_type = decltype(currency::zc_outs_range_proof{}.bpp.L)::value_type;
  EXPECT_GE(enough.bytes, 2 * points * sizeof(point_type));
  // Precise pre-allocation rejection one byte below the measured charge.
  binary_decode_budget short_budget = roomy_budget();
  short_budget.max_bytes = enough.bytes - 1;
  currency::transaction refused{};
  EXPECT_FALSE(strict_parse(blob, refused, short_budget));
  EXPECT_TRUE(short_budget.exhausted);
  EXPECT_LE(short_budget.bytes, short_budget.max_bytes);

  // A uint64_t inside txout_ref_v uses fixed-width encoding, so the wire
  // size does not imply an eightfold reservation. Test the allocation budget
  // directly, independently of the encoded element width.
  const size_t offsets = 20000;
  currency::transaction many_offsets = make_modern(TRANSACTION_VERSION_POST_HF6, offsets, 0);
  const std::string offset_blob = t_serializable_object_to_blob(many_offsets);
  binary_decode_budget offset_enough = roomy_budget();
  currency::transaction offset_decoded{};
  ASSERT_TRUE(strict_parse(offset_blob, offset_decoded, offset_enough));
  EXPECT_EQ(offset_blob, t_serializable_object_to_blob(offset_decoded));

  const size_t reservation_bytes = offsets * sizeof(currency::txout_ref_v);
  binary_decode_budget below_reservation = roomy_budget();
  below_reservation.max_bytes = reservation_bytes - 1;
  currency::transaction offset_refused{};
  EXPECT_FALSE(strict_parse(offset_blob, offset_refused, below_reservation));
  EXPECT_TRUE(below_reservation.exhausted);
  EXPECT_LE(below_reservation.bytes, below_reservation.max_bytes);
  EXPECT_TRUE(offset_refused.vin.empty());

  // Observe the vector itself: rejecting before reserve must leave capacity
  // zero, whereas merely discarding a partially decoded transaction cannot
  // establish that no temporary reservation occurred.
  auto offset_values = boost::get<currency::txin_zc_input>(many_offsets.vin.front()).key_offsets;
  std::ostringstream offset_stream;
  binary_archive<true> offset_writer(offset_stream);
  ASSERT_TRUE(::serialization::serialize(offset_writer, offset_values));
  const std::string vector_blob = offset_stream.str();

  binary_decode_budget exact_reservation = roomy_budget();
  exact_reservation.max_bytes = reservation_bytes;
  decltype(offset_values) accepted_offsets;
  ASSERT_TRUE(strict_parse(vector_blob, accepted_offsets, exact_reservation));
  EXPECT_EQ(offset_values.size(), accepted_offsets.size());
  std::ostringstream accepted_stream;
  binary_archive<true> accepted_writer(accepted_stream);
  ASSERT_TRUE(::serialization::serialize(accepted_writer, accepted_offsets));
  EXPECT_EQ(vector_blob, accepted_stream.str());
  EXPECT_EQ(reservation_bytes, exact_reservation.bytes);

  binary_decode_budget short_reservation = roomy_budget();
  short_reservation.max_bytes = reservation_bytes - 1;
  decltype(offset_values) refused_offsets;
  EXPECT_FALSE(strict_parse(vector_blob, refused_offsets, short_reservation));
  EXPECT_TRUE(short_reservation.exhausted);
  EXPECT_LE(short_reservation.bytes, short_reservation.max_bytes);
  EXPECT_TRUE(refused_offsets.empty());
  EXPECT_EQ(0u, refused_offsets.capacity());
}

TEST(read_only_block_transport_serialization, strict_block_hash_matches_default_without_growth)
{
  chain_fixture legacy(40, 2, 2);
  currency::block modern = legacy.blocks[1];
  modern.miner_tx = make_modern(TRANSACTION_VERSION_POST_HF4, 1, 2);
  currency::block hf6 = legacy.blocks[1];
  hf6.miner_tx = make_modern(TRANSACTION_VERSION_POST_HF6, 1, 2);
  std::vector<char> scratch(4096);
  for (const currency::block* b : {&legacy.blocks[0], &legacy.blocks[1], &modern, &hf6})
  {
    crypto::hash miner = currency::null_hash, strict = currency::null_hash;
    ASSERT_TRUE(currency::get_object_hash_in_buffer(static_cast<const currency::transaction_prefix&>(b->miner_tx), scratch, miner));
    EXPECT_EQ(currency::get_transaction_hash(b->miner_tx), miner);
    std::vector<crypto::hash> ids;
    ids.reserve(b->tx_hashes.size() + 1);
    std::vector<crypto::hash> tree(crypto::tree_hash_scratch_count(b->tx_hashes.size() + 1));
    ASSERT_TRUE(currency::get_block_hash_in_buffer(*b, miner, ids, tree, scratch, strict));
    EXPECT_EQ(currency::get_block_hash(*b), strict);
    // Insufficient id capacity is refused rather than grown.
    std::vector<crypto::hash> small;
    small.reserve(b->tx_hashes.size());
    const size_t capacity = small.capacity();
    if (capacity <= b->tx_hashes.size())
    {
      EXPECT_FALSE(currency::get_block_hash_in_buffer(*b, miner, small, tree, scratch, strict));
      EXPECT_EQ(capacity, small.capacity());
    }
  }
}

TEST(read_only_block_transport_serialization, strict_block_hash_scratch_minimum_includes_length_varint)
{
  // Fixed scratch contract: BLOCK_HASHING_BLOB_MAX_BYTES + 2 (the blob plus
  // its two-byte maximum varint length), the transport's allocation.
  const size_t minimum = currency::BLOCK_HASHING_BLOB_MAX_BYTES + 2;
  ASSERT_EQ(130u, minimum);
  chain_fixture c(40, 1, 2);
  const currency::block& b = c.blocks[0];
  std::vector<char> roomy(4096);
  crypto::hash miner = currency::null_hash;
  ASSERT_TRUE(currency::get_object_hash_in_buffer(static_cast<const currency::transaction_prefix&>(b.miner_tx), roomy, miner));
  std::vector<crypto::hash> ids;
  ids.reserve(b.tx_hashes.size() + 1);
  std::vector<crypto::hash> tree(crypto::tree_hash_scratch_count(b.tx_hashes.size() + 1));
  crypto::hash unchanged; // Output sentinel (sentinel_hash() is declared below).
  memset(&unchanged, 0xa5, sizeof(unchanged));

  // Exact minimum: accepted, same hash as the legacy helper, never grown.
  std::vector<char> exact(minimum);
  crypto::hash res = unchanged;
  ASSERT_TRUE(currency::get_block_hash_in_buffer(b, miner, ids, tree, exact, res));
  EXPECT_EQ(currency::get_block_hash(b), res);
  EXPECT_EQ(minimum, exact.size());

  // One byte short (and the former 128/127-byte minima): refused before any
  // work, output sentinel unchanged, scratch not grown.
  for (size_t bytes : {minimum - 1, currency::BLOCK_HASHING_BLOB_MAX_BYTES, currency::BLOCK_HASHING_BLOB_MAX_BYTES - 1})
  {
    SCOPED_TRACE(bytes);
    std::vector<char> short_scratch(bytes);
    crypto::hash out = unchanged;
    EXPECT_FALSE(currency::get_block_hash_in_buffer(b, miner, ids, tree, short_scratch, out));
    EXPECT_EQ(unchanged, out);
    EXPECT_EQ(bytes, short_scratch.size());
  }
}

namespace
{
  crypto::hash hash_from_hex(const std::string& hex)
  {
    crypto::hash h{};
    auto nibble = [](char c) { return c <= '9' ? c - '0' : c - 'a' + 10; };
    for (size_t i = 0; i != sizeof(h); ++i)
      reinterpret_cast<unsigned char*>(&h)[i] = static_cast<unsigned char>(nibble(hex[2 * i]) * 16 + nibble(hex[2 * i + 1]));
    return h;
  }

  // Interpreted as tests/hash/main.cpp does: the data is consecutive 32-byte
  // leaves, count = bytes / 32, compared with the expected root.
  std::vector<crypto::hash> leaves_from_hex(const std::string& hex)
  {
    std::vector<crypto::hash> leaves;
    for (size_t i = 0; i + 64 <= hex.size(); i += 64)
      leaves.push_back(hash_from_hex(hex.substr(i, 64)));
    return leaves;
  }

  // Bounded synthetic leaves.
  std::vector<crypto::hash> synthetic_leaves(size_t count)
  {
    std::vector<crypto::hash> leaves(count);
    for (size_t i = 0; i != count; ++i)
    {
      const uint64_t seed = 0x5eed0000 + i;
      leaves[i] = crypto::cn_fast_hash(&seed, sizeof(seed));
    }
    return leaves;
  }

  crypto::hash sentinel_hash(unsigned char fill)
  {
    crypto::hash h;
    memset(&h, fill, sizeof(h));
    return h;
  }
}

TEST(read_only_block_transport_serialization, checked_tree_hash_scratch_count_is_exact)
{
  EXPECT_EQ(0u, crypto::tree_hash_scratch_count(0));
  EXPECT_EQ(0u, crypto::tree_hash_scratch_count(1));
  EXPECT_EQ(0u, crypto::tree_hash_scratch_count(2));
  EXPECT_EQ(2u, crypto::tree_hash_scratch_count(3));
  EXPECT_EQ(2u, crypto::tree_hash_scratch_count(4));
  EXPECT_EQ(4u, crypto::tree_hash_scratch_count(5));
  EXPECT_EQ(4u, crypto::tree_hash_scratch_count(8));
  EXPECT_EQ(8u, crypto::tree_hash_scratch_count(9));
  EXPECT_EQ(8u, crypto::tree_hash_scratch_count(16));
  EXPECT_EQ(16u, crypto::tree_hash_scratch_count(17));
  EXPECT_EQ(512u, crypto::tree_hash_scratch_count(1000));
  for (size_t count = 3; count != 300; ++count)
  {
    const size_t cnt = crypto::tree_hash_scratch_count(count);
    // Greatest power of two <= count - 1: strictly below count, so the
    // transport's per-id precharge covers it.
    EXPECT_EQ(0u, cnt & (cnt - 1));
    EXPECT_LE(cnt, count - 1);
    EXPECT_GT(2 * cnt, count - 1);
    EXPECT_LT(cnt, count);
  }
}

TEST(read_only_block_transport_serialization, checked_tree_hash_matches_known_roots)
{
  // Fixed vectors copied from tests/hash/tests-tree.txt (lines 1-5, 8, 9):
  // an independent oracle, so a shared-reduction regression cannot hide.
  struct vector_case { const char* root; const char* data; size_t count; };
  const vector_case cases[] = {
    {"676567f8b1b470207c20d8efbaacfa64b2753301b46139562111636f36304bb8",
     "676567f8b1b470207c20d8efbaacfa64b2753301b46139562111636f36304bb8", 1},
    {"5077570fed2363a14fa978218185b914059e23517faf366f08a87cf3c47fd58e",
     "3124758667bc8e76e25403eee75a1044175d58fcd3b984e0745d0ab18f473984975ce54240407d80eedba2b395bcad5be99b5c920abc2423865e3066edd4847a", 2},
    {"f8e26aaa7c36523cea4c5202f2df159c62bf70d10670c96aed516dbfd5cb5227",
     "decc1e0aa505d7d5fbe8ed823d7f5da55307c4cc7008e306da82dbce492a0576dbcf0c26646d36b36a92408941f5f2539f7715bcb1e2b1309cedb86ae4211554f56f5e6b2fce16536e44c851d473d1f994793873996ba448dd59b3b4b922b183", 3},
    {"45f6e06fc0263e667caddd8fba84c9fb723a961a01a5b115f7cab7fe8f2c7e44",
     "53edbbf98d3fa50a85fd2d46c42502aafad3fea30bc25ba4f16ec8bf4a475c4d87da8ad3e5c90aae0b10a559a77a0985608eaa3cc3dd338239be52572c3bdf4ba403d27466991997b3cf4e8d238d002a1451ccc9c4790269d0f0085d9382d60fef37717f59726e4cc8787d5d2d75238ba9adb9627a8f4aeeec8d80465ed3f5fb", 4},
    {"e678fb87749ec082a9f92537716de8e19d8bd5bc4c4d832bd3fcfd42498dac83",
     "051a082e670c688e6a0fc2c8fd5b66b7a23cd380c7c49bd0cfffb0e80fb8c2334bb717c5e90db0ac353dfc0750c8b43a07edae0be99d6e820acc6da9f113123ae084c38ccdbf9c6730e228b5d98e7beb9843cfb523747cc32f09f2b16def67f76765cee044883827b9af31c179d3135b16c30f04453943d9676a59b907a6439658f6c98159b8fa1b152f1bcf748740754ca31c918501dbd577faf602c641df59", 5},
    {"090a95612ed9df6eeb854ae320355889a302498b4f5164a79d8e384a3a0d9748",
     "42e7f4058ca80d513c140837dd661acde3fb914779079baccfe188cbce275aed4b515094bb49ab9a825bcc2ac13f84b14a9defeb1b62fc68124b088272a3562696d62ccdfb5d896b2d2b410a2a79f9b1e7849feebc17617ba12a08d4e80affe970ff2fb79917ac13708f79be215bb6484d298b2fe22b4818536e74894db5e0350e1505ca2681da7b7d7171e3d10c89348cab160ff5b2e739d3591443d2af60db5eb36c50a2dfdb79b8ab83b0792161ac4756d9b831f1863188e10c81af5077d0fdb123f66e51670f03a203ff2287dea6827dcd5afd4904736ec4fe9f3b52f7e2bed7beaa1543bd8bfbfff6a8ae8bf1791dc34efa92c6342532fa33a3b72b6c9f", 8},
    {"997ac1178ab7414bab823fbca45b5630df8d1d8263063e6c57da463b85d68a74",
     "947fbbc55ad237fc5dbd7d52dddd44bf3f2a09005c78873422f7ef282d8e6fcc554e35c9566febf91cbbcb1d57a7ebd119abb0ad33a006d01623b7b379e966e00be000ae2fe8a45940e99c953d22014bae4932d8493ad4a551a97d437db2939dd53abedc11a63417f76257a5587f382a57d46d63c372182600c7920bcaf74e9e65289e8c45123ac8a54a45a6104dce5b8c065065ff3a3b6f8bf4d86bf96cb56116df4e01eb3153223d5f3a8c0d7de9eb348158e5ca0c363568674215f68b6ff8e54aeb4a2661f1144cb4f1bde7f9e6371d8a5568d4b3ff3382c65e143ae5d3a5834c890559be95b8b80b82c83d70df85c934bf9dd4b0f2b5f60b8553bd1c1e537b7a1f78a89a17a335a06f5d7143dfecff0c10a2e0a524c91ce913ae04501b65", 9},
  };
  for (const vector_case& c : cases)
  {
    SCOPED_TRACE(c.count);
    const std::vector<crypto::hash> leaves = leaves_from_hex(c.data);
    ASSERT_EQ(c.count, leaves.size());
    ASSERT_EQ(2 * c.count * sizeof(crypto::hash), strlen(c.data)); // Whole leaves only.
    const crypto::hash expected = hash_from_hex(c.root);
    crypto::hash legacy = sentinel_hash(0xa5);
    crypto::tree_hash(leaves.data(), leaves.size(), legacy);
    EXPECT_EQ(expected, legacy);
    std::vector<crypto::hash> scratch(crypto::tree_hash_scratch_count(leaves.size()));
    crypto::hash checked = sentinel_hash(0xa5);
    ASSERT_TRUE(crypto::tree_hash_checked(leaves.data(), leaves.size(), scratch.data(), scratch.size(), checked));
    EXPECT_EQ(expected, checked);
  }
}

TEST(read_only_block_transport_serialization, checked_tree_hash_equals_legacy_on_synthetic_counts)
{
  for (size_t count : {size_t(1), size_t(2), size_t(3), size_t(4), size_t(5), size_t(6), size_t(7), size_t(8),
         size_t(9), size_t(15), size_t(16), size_t(17), size_t(31), size_t(32), size_t(33), size_t(63),
         size_t(64), size_t(65), size_t(100), size_t(127), size_t(128), size_t(129), size_t(1000), size_t(1025)})
  {
    SCOPED_TRACE(count);
    const std::vector<crypto::hash> leaves = synthetic_leaves(count);
    crypto::hash legacy = currency::null_hash;
    crypto::tree_hash(leaves.data(), count, legacy);
    // Exact scratch between guard units that must stay untouched.
    const size_t need = crypto::tree_hash_scratch_count(count);
    const crypto::hash guard = sentinel_hash(0x5a);
    std::vector<crypto::hash> guarded(need + 2, guard);
    crypto::hash checked = sentinel_hash(0xa5);
    ASSERT_TRUE(crypto::tree_hash_checked(leaves.data(), count, guarded.data() + 1, need, checked));
    EXPECT_EQ(legacy, checked);
    EXPECT_EQ(guard, guarded.front());
    EXPECT_EQ(guard, guarded.back());
    // Extra scratch beyond the requirement is accepted and changes nothing.
    std::vector<crypto::hash> roomy(need + 3);
    crypto::hash again = sentinel_hash(0xa5);
    ASSERT_TRUE(crypto::tree_hash_checked(leaves.data(), count, roomy.data(), roomy.size(), again));
    EXPECT_EQ(legacy, again);
  }
}

TEST(read_only_block_transport_serialization, checked_tree_hash_refuses_without_writing)
{
  const crypto::hash unchanged = sentinel_hash(0xa5);
  const crypto::hash fill = sentinel_hash(0x5a);
  // Counts one and two need no scratch: absent scratch is accepted.
  for (size_t count : {size_t(1), size_t(2)})
  {
    const std::vector<crypto::hash> leaves = synthetic_leaves(count);
    crypto::hash legacy = currency::null_hash, checked = unchanged;
    crypto::tree_hash(leaves.data(), count, legacy);
    EXPECT_TRUE(crypto::tree_hash_checked(leaves.data(), count, nullptr, 0, checked));
    EXPECT_EQ(legacy, checked);
  }
  for (size_t count : {size_t(3), size_t(4), size_t(5), size_t(9), size_t(33)})
  {
    SCOPED_TRACE(count);
    const std::vector<crypto::hash> leaves = synthetic_leaves(count);
    const size_t need = crypto::tree_hash_scratch_count(count);
    ASSERT_GT(need, 0u);
    // One unit short: refused, nothing written to the root or the scratch.
    std::vector<crypto::hash> short_scratch(need, fill); // Only need - 1 units are offered.
    crypto::hash root = unchanged;
    EXPECT_FALSE(crypto::tree_hash_checked(leaves.data(), count, short_scratch.data(), need - 1, root));
    EXPECT_EQ(unchanged, root);
    for (const crypto::hash& unit : short_scratch) EXPECT_EQ(fill, unit);
    // Absent scratch.
    EXPECT_FALSE(crypto::tree_hash_checked(leaves.data(), count, nullptr, 0, root));
    EXPECT_FALSE(crypto::tree_hash_checked(leaves.data(), count, nullptr, need, root));
    EXPECT_EQ(unchanged, root);
  }
  // Invalid arguments and overflowing counts are refused before any access:
  // the one-leaf array is never read past its end.
  const std::vector<crypto::hash> one = synthetic_leaves(1);
  std::vector<crypto::hash> scratch(2, fill);
  crypto::hash root = unchanged;
  EXPECT_FALSE(crypto::tree_hash_checked(one.data(), 0, scratch.data(), scratch.size(), root));
  EXPECT_FALSE(crypto::tree_hash_checked(nullptr, 1, scratch.data(), scratch.size(), root));
  const size_t overflow = std::numeric_limits<size_t>::max() / sizeof(crypto::hash) + 1;
  EXPECT_FALSE(crypto::tree_hash_checked(one.data(), overflow, scratch.data(), std::numeric_limits<size_t>::max(), root));
  EXPECT_FALSE(crypto::tree_hash_checked(one.data(), std::numeric_limits<size_t>::max(), scratch.data(), std::numeric_limits<size_t>::max(), root));
  EXPECT_EQ(unchanged, root);
  for (const crypto::hash& unit : scratch) EXPECT_EQ(fill, unit);
}

TEST(read_only_block_transport_serialization, strict_block_hash_propagates_tree_scratch_refusal)
{
  chain_fixture c(40, 1, 4); // Five ids: four units of tree scratch.
  const currency::block& b = c.blocks[0];
  const size_t ids_count = b.tx_hashes.size() + 1;
  const size_t need = crypto::tree_hash_scratch_count(ids_count);
  ASSERT_EQ(4u, need);
  std::vector<char> scratch(4096);
  crypto::hash miner = currency::null_hash;
  ASSERT_TRUE(currency::get_object_hash_in_buffer(static_cast<const currency::transaction_prefix&>(b.miner_tx), scratch, miner));

  std::vector<crypto::hash> ids;
  ids.reserve(ids_count);
  std::vector<crypto::hash> exact(need);
  crypto::hash strict = currency::null_hash;
  ASSERT_TRUE(currency::get_block_hash_in_buffer(b, miner, ids, exact, scratch, strict));
  EXPECT_EQ(currency::get_block_hash(b), strict);

  // Undersized and empty tree scratch: refused, no hash published, no growth.
  for (size_t units : {need - 1, size_t(0)})
  {
    SCOPED_TRACE(units);
    std::vector<crypto::hash> undersized(units);
    const crypto::hash unchanged = sentinel_hash(0xa5);
    crypto::hash res = unchanged;
    EXPECT_FALSE(currency::get_block_hash_in_buffer(b, miner, ids, undersized, scratch, res));
    EXPECT_EQ(unchanged, res);
    EXPECT_EQ(units, undersized.size());
  }
  // A capacity-only reservation is not writable scratch and is refused too.
  std::vector<crypto::hash> reserved_only;
  reserved_only.reserve(need);
  crypto::hash res = sentinel_hash(0xa5);
  EXPECT_FALSE(currency::get_block_hash_in_buffer(b, miner, ids, reserved_only, scratch, res));
  EXPECT_EQ(sentinel_hash(0xa5), res);

  // One/two ids need no tree scratch: an empty vector suffices.
  chain_fixture small(40, 2, 1); // Two ids per block.
  for (const currency::block& sb : small.blocks)
  {
    crypto::hash sm = currency::null_hash, sres = currency::null_hash;
    ASSERT_TRUE(currency::get_object_hash_in_buffer(static_cast<const currency::transaction_prefix&>(sb.miner_tx), scratch, sm));
    std::vector<crypto::hash> sids;
    sids.reserve(sb.tx_hashes.size() + 1);
    std::vector<crypto::hash> none;
    ASSERT_TRUE(currency::get_block_hash_in_buffer(sb, sm, sids, none, scratch, sres));
    EXPECT_EQ(currency::get_block_hash(sb), sres);
  }
}

TEST(read_only_block_transport_serialization, fixed_buffer_latches_failed_varint_writes)
{
  // [varint(1), varint(128)] = 01 80 01. Before the latch, a two-byte buffer
  // could hold 01 80, silently drop the final 01 through ostreambuf_iterator
  // and still report success.
  const std::vector<uint64_t> value{128};
  const std::string expected = t_serializable_object_to_blob(value);
  ASSERT_EQ(std::string("\x01\x80\x01", 3), expected);
  const crypto::hash expected_hash = crypto::cn_fast_hash(expected.data(), expected.size());

  std::vector<char> exact(expected.size());
  size_t size = 99;
  ASSERT_TRUE(currency::serialize_object_to_buffer(value, exact, size));
  EXPECT_EQ(expected.size(), size);
  EXPECT_EQ(expected, std::string(exact.data(), size));
  crypto::hash h = currency::null_hash;
  ASSERT_TRUE(currency::get_object_hash_in_buffer(value, exact, h));
  EXPECT_EQ(expected_hash, h);
  EXPECT_EQ(currency::get_object_hash(value), h); // Ordinary helper equivalence.

  for (size_t bytes : {size_t(0), size_t(1), expected.size() - 1})
  {
    SCOPED_TRACE(bytes);
    std::vector<char> small(bytes);
    size = 99;
    EXPECT_FALSE(currency::serialize_object_to_buffer(value, small, size));
    EXPECT_EQ(0u, size);
    EXPECT_EQ(bytes, small.size()); // Never grown.
    const crypto::hash unchanged = sentinel_hash(0xa5);
    crypto::hash res = unchanged;
    EXPECT_FALSE(currency::get_object_hash_in_buffer(value, small, res));
    EXPECT_EQ(unchanged, res);
  }
}

TEST(read_only_block_transport_serialization, fixed_buffer_latches_short_bulk_writes)
{
  // varint(length) then one bulk write of the characters.
  const std::string value = "abcdef";
  const std::string expected = t_serializable_object_to_blob(value);
  ASSERT_EQ(std::string("\x06") + value, expected);
  std::vector<char> exact(expected.size());
  size_t size = 99;
  ASSERT_TRUE(currency::serialize_object_to_buffer(value, exact, size));
  EXPECT_EQ(expected, std::string(exact.data(), size));
  crypto::hash h = currency::null_hash;
  ASSERT_TRUE(currency::get_object_hash_in_buffer(value, exact, h));
  EXPECT_EQ(currency::get_object_hash(value), h);

  // 0: nothing fits; 1: only the length fits (bulk has no room);
  // size - 1: the bulk write is one byte short.
  for (size_t bytes : {size_t(0), size_t(1), expected.size() - 1})
  {
    SCOPED_TRACE(bytes);
    std::vector<char> small(bytes);
    size = 99;
    EXPECT_FALSE(currency::serialize_object_to_buffer(value, small, size));
    EXPECT_EQ(0u, size);
    const crypto::hash unchanged = sentinel_hash(0xa5);
    crypto::hash res = unchanged;
    EXPECT_FALSE(currency::get_object_hash_in_buffer(value, small, res));
    EXPECT_EQ(unchanged, res);
  }
  // A larger buffer yields the same bytes; only written() bytes are hashed.
  std::vector<char> roomy(expected.size() + 16, '\x7f');
  ASSERT_TRUE(currency::serialize_object_to_buffer(value, roomy, size));
  EXPECT_EQ(expected, std::string(roomy.data(), size));
}

TEST(read_only_block_transport, hash_side_allocations_are_precharged)
{
  // PRE_HF4-layout coinbase with many extras: hashing deep-copies the prefix
  // through transaction_prefix_v1. Before this correction only the compact
  // blob length was charged for hashing.
  chain_fixture c(40, 1, 0);
  for (size_t i = 0; i != 20000; ++i)
    c.blocks[0].miner_tx.extra.push_back(currency::etc_tx_time{1});
  c.reencode(0);
  const std::string& blob = c.entries[0].block;
  binary_decode_budget measure = roomy_budget();
  currency::block probe{};
  ASSERT_TRUE(strict_parse(blob, probe, measure));
  const uint64_t decode_only = measure.bytes;
  ASSERT_GT(decode_only, 8 * blob.size()); // Amplified in memory.

  harness ok; ok.exchange->body = c.body();
  ASSERT_EQ(transport_failure::None, ok.fetch());
  ASSERT_EQ(1u, ok.response.blocks.size());
  EXPECT_EQ(currency::get_block_hash(c.blocks[0]), currency::get_block_hash(ok.response.blocks.front().block_ptr->bl));

  // Oracle: decode plus the former per-blob hashing charge stays far below
  // 2 * decode_only; decode plus the legacy-copy hashing precharge exceeds it.
  block_transport_limits limits;
  limits.max_decode_allocation_bytes = 2 * decode_only;
  ASSERT_TRUE(block_transport_limits_valid(limits));
  harness refused; refused.exchange->body = c.body();
  EXPECT_EQ(transport_failure::LimitExceeded, refused.fetch(limits));
}
