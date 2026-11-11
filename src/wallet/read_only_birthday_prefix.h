// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#pragma once
#include "native_wallet_birthday_prefix.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <limits>
#include <string>
#include <string_view>

namespace tools { namespace read_only {
class monotonic_clock;
namespace birthday_prefix {
namespace policy = zano::native_wallet_birthday_prefix;
constexpr size_t max_request_bytes = 1024;
constexpr size_t max_response_bytes = 524288;
struct request {
  uint64_t start_height = 0;
  uint64_t count = policy::kMaxHeadersPerPage;
  uint64_t target_height = 0;
  policy::Hash target_hash{};
  policy::Hash genesis_hash{};
};
struct page {
  policy::ChainView view{};
  uint64_t start_height = 0;
  std::array<policy::Header, policy::kMaxHeadersPerPage> headers{};
  size_t count = 0;
  // Internal operation budget, not wallet/progress evidence. Selection consumes
  // the remaining SAME decode deadline; it cannot start a second decode budget.
  std::shared_ptr<const monotonic_clock> decode_clock;
  std::chrono::steady_clock::time_point decode_deadline{};
};
inline bool valid(const request& value) noexcept {
  if (!value.count || value.count > policy::kMaxHeadersPerPage ||
      !policy::nonzero_hash(value.genesis_hash)) return false;
  return policy::nonzero_hash(value.target_hash)
    ? value.start_height <= value.target_height
    : value.start_height == 0 && value.target_height == 0;
}
inline void append_hash(std::string& out, const policy::Hash& value) {
  constexpr char digits[] = "0123456789abcdef";
  for (const auto byte : value) {
    out.push_back(digits[byte >> 4]);
    out.push_back(digits[byte & 15]);
  }
}
// Fixed method and field order, no URL/method/seed input or generic JSON.
inline std::string encode(const request& value) {
  if (!valid(value)) return {};
  std::string out = "{\"jsonrpc\":\"2.0\",\"id\":\"0\",\"method\":\"get_native_birthday_prefix_v1\",\"params\":{\"start_height\":";
  out += std::to_string(value.start_height);
  out += ",\"count\":" + std::to_string(value.count);
  out += ",\"target_height\":" + std::to_string(value.target_height);
  out += ",\"target_hash\":\"";
  if (policy::nonzero_hash(value.target_hash)) append_hash(out, value.target_hash);
  out += "\",\"genesis_hash\":\"";
  append_hash(out, value.genesis_hash);
  out += "\"}}";
  return out;
}

// Fixed-depth, allocation-free complete JSON decoder. The sole array has
// <=1000 fixed-size rows. Every token is charged to the cumulative element
// limit, every byte observes the caller's stop/decode deadline. No ignored
// fields, duplicate fields, errors, escapes, noncanonical numbers or tail.
class response_reader final {
public:
  using abort_check = bool (*)(void*) noexcept;
  response_reader(std::string_view text, abort_check abort, void* context,
      size_t max_elements, size_t max_entries, size_t max_sections,
      size_t max_array_elements, size_t max_depth) noexcept
    : text_(text), abort_(abort), context_(context), max_elements_(max_elements),
      max_entries_(max_entries), max_sections_(max_sections),
      max_array_elements_(max_array_elements), max_depth_(max_depth) {}
  bool parse(page& out) noexcept {
    out = {};
    if (!abort_ || !max_elements_ || text_.empty() ||
        text_.size() > max_response_bytes) return false;
    if (!max_entries_ || !max_sections_ || !max_array_elements_ || max_depth_ < 4) {
      element_limit_ = true; return false;
    }
    if (!section() || !take('{')) return false;
    unsigned fields = 0;
    for (;;) {
      std::string_view key, value;
      if (!entry() || !string(key) || !take(':')) return false;
      unsigned bit = 0;
      if (key == "jsonrpc") {
        bit = 1; if (!string(value) || value != "2.0") return false;
      } else if (key == "id") {
        bit = 2; if (!string(value) || value != "0") return false;
      } else if (key == "result") {
        bit = 4; if (!result(out)) return false;
      } else return false;
      if (fields & bit) return false;
      fields |= bit;
      if (take('}')) break;
      if (!take(',')) return false;
    }
    if (fields != 7 || !space() || at_ != text_.size() || stopped()) return false;
    out.view.token = out.view.target_hash; // actual canonical target identity
    return true;
  }
  bool element_limit_exceeded() const noexcept { return element_limit_; }
private:
  bool stopped() const noexcept { return abort_(context_); }
  bool charge() noexcept {
    if (elements_ >= max_elements_) { element_limit_ = true; return false; }
    ++elements_; return !stopped();
  }
  bool section() noexcept {
    if (sections_ >= max_sections_) { element_limit_ = true; return false; }
    ++sections_; return charge();
  }
  bool entry() noexcept {
    if (entries_ >= max_entries_) { element_limit_ = true; return false; }
    ++entries_; return charge();
  }
  bool space() noexcept {
    while (at_ < text_.size() && (text_[at_] == ' ' || text_[at_] == '\n' ||
        text_[at_] == '\r' || text_[at_] == '\t')) {
      if (stopped()) return false;
      ++at_;
    }
    return !stopped();
  }
  bool take(char value) noexcept {
    if (!space() || at_ == text_.size() || text_[at_] != value) return false;
    ++at_; return true;
  }
  bool string(std::string_view& value) noexcept {
    if (!charge() || !take('"')) return false;
    const size_t start = at_;
    while (at_ < text_.size() && text_[at_] != '"') {
      const auto ch = static_cast<unsigned char>(text_[at_]);
      if (stopped() || ch < 32 || ch >= 127 || ch == '\\') return false;
      ++at_;
    }
    if (at_ == text_.size() || stopped()) return false;
    value = text_.substr(start, at_ - start); ++at_; return true;
  }
  bool number(uint64_t& value) noexcept {
    if (!charge() || !space() || at_ == text_.size() ||
        text_[at_] < '0' || text_[at_] > '9') return false;
    const size_t start = at_; value = 0;
    while (at_ < text_.size() && text_[at_] >= '0' && text_[at_] <= '9') {
      if (stopped() || (at_ > start && text_[start] == '0')) return false;
      const uint64_t digit = static_cast<unsigned>(text_[at_++] - '0');
      if (value > (std::numeric_limits<uint64_t>::max() - digit) / 10) return false;
      value = value * 10 + digit;
    }
    return true;
  }
  bool hash(policy::Hash& value) noexcept {
    std::string_view hex;
    if (!string(hex) || hex.size() != value.size() * 2) return false;
    for (size_t i = 0; i < value.size(); ++i) {
      unsigned byte = 0;
      for (size_t n = 0; n < 2; ++n) {
        if (stopped()) return false;
        const char ch = hex[i * 2 + n];
        unsigned digit = 0;
        if (ch >= '0' && ch <= '9') digit = static_cast<unsigned>(ch - '0');
        else if (ch >= 'a' && ch <= 'f') digit = static_cast<unsigned>(ch - 'a') + 10;
        else return false;
        byte = byte * 16 + digit;
      }
      value[i] = static_cast<uint8_t>(byte);
    }
    return true;
  }
  bool header(policy::Header& row) noexcept {
    if (!section() || !take('{')) return false;
    unsigned fields = 0;
    for (;;) {
      std::string_view key;
      if (!entry() || !string(key) || !take(':')) return false;
      unsigned bit = 0;
      if (key == "height") { bit = 1; if (!number(row.height)) return false; }
      else if (key == "timestamp") { bit = 2; if (!number(row.timestamp)) return false; }
      else if (key == "hash") { bit = 4; if (!hash(row.hash)) return false; }
      else if (key == "prev_hash") { bit = 8; if (!hash(row.previous_hash)) return false; }
      else return false;
      if (fields & bit) return false;
      fields |= bit;
      if (take('}')) break;
      if (!take(',')) return false;
    }
    return fields == 15;
  }
  bool headers(page& out) noexcept {
    if (!charge() || !take('[')) return false;
    // Empty and oversized pages are refused, never interpreted as progress.
    do {
      if (out.count == out.headers.size() || out.count >= max_array_elements_) {
        element_limit_ = true; return false;
      }
      if (!header(out.headers[out.count])) return false;
      ++out.count;
      if (take(']')) return true;
    } while (take(','));
    return false;
  }
  bool result(page& out) noexcept {
    if (!section() || !take('{')) return false;
    unsigned fields = 0;
    for (;;) {
      std::string_view key, value;
      if (!entry() || !string(key) || !take(':')) return false;
      unsigned bit = 0;
      if (key == "status") { bit = 1; if (!string(value) || value != "OK") return false; }
      else if (key == "version") {
        uint64_t version = 0; bit = 2; if (!number(version) || version != 1) return false;
      } else if (key == "target_height") { bit = 4; if (!number(out.view.target_height)) return false; }
      else if (key == "target_hash") { bit = 8; if (!hash(out.view.target_hash)) return false; }
      else if (key == "genesis_hash") { bit = 16; if (!hash(out.view.genesis_hash)) return false; }
      else if (key == "start_height") { bit = 32; if (!number(out.start_height)) return false; }
      else if (key == "headers") { bit = 64; if (!headers(out)) return false; }
      else return false;
      if (fields & bit) return false;
      fields |= bit;
      if (take('}')) break;
      if (!take(',')) return false;
    }
    return fields == 127;
  }
  const std::string_view text_;
  const abort_check abort_;
  void* const context_;
  const size_t max_elements_;
  const size_t max_entries_;
  const size_t max_sections_;
  const size_t max_array_elements_;
  const size_t max_depth_;
  size_t sections_ = 0;
  size_t entries_ = 0;
  size_t at_ = 0;
  size_t elements_ = 0;
  bool element_limit_ = false;
};
}} // namespace read_only::birthday_prefix

namespace read_only {
class birthday_prefix_source {
public:
  virtual ~birthday_prefix_source() = default;
  virtual bool fetch_birthday_prefix_page(const birthday_prefix::request& request,
    birthday_prefix::page& response, const std::atomic<bool>& stop) = 0;
};
}} // namespace tools::read_only
