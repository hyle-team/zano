// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

namespace tools { namespace read_only {
// Private, fixed-purpose capability. No URL, method, address or seed input.
class birthday_height_source {
public:
  virtual ~birthday_height_source() = default;
  virtual bool resolve_birthday_height(uint64_t timestamp, uint64_t& height,
    const std::atomic<bool>& stop) = 0;
};
namespace birthday {
constexpr uint64_t date_offset = 1543622400;
constexpr uint64_t date_quantum = 604800;
constexpr uint64_t max_weeks = 800;
constexpr size_t max_request_bytes = 1024;
constexpr size_t max_response_bytes = 4096;
// Existing strict-current native images trust their persisted local boundary.
// No clock, estimator or retained original anchor participates in this predicate.
inline constexpr bool needs_date(uint64_t minimum, uint64_t unset) noexcept {
  return minimum == unset;
}
inline constexpr bool saved_minimum_valid(uint64_t minimum, uint64_t unset,
  uint64_t local_blocks, uint64_t maximum) noexcept {
  return local_blocks > 0 && (minimum == 0 || minimum == unset ||
    (minimum < maximum && minimum < local_blocks));
}
// A fresh clock/seed date is an assumed lower bound, not cryptographic history proof.
inline constexpr bool rounded_date(uint64_t raw, uint64_t now, uint64_t& rounded) noexcept {
  rounded = 0;
  if (raw <= date_offset || raw > now) return false;
  const uint64_t weeks = (raw - date_offset) / date_quantum;
  if (weeks >= max_weeks || weeks > (std::numeric_limits<uint64_t>::max() - date_offset) / date_quantum)
    return false;
  rounded = date_offset + weeks * date_quantum;
  return rounded > date_offset;
}
inline constexpr bool anchor_height(uint64_t estimate, uint64_t maximum, uint64_t& anchor) noexcept {
  anchor = 0;
  if (estimate >= maximum) return false;
  // Include the estimated block in output scanning; the preceding block is anchor only.
  anchor = estimate > 1 ? estimate - 1 : 0;
  return true;
}
inline constexpr bool anchor_date_valid(uint64_t timestamp, uint64_t date) noexcept {
  return date >= 3600 && timestamp <= date - 3600;
}
// Strict, bounded JSON-RPC subset. Reject duplicate/unknown fields, errors,
// escapes, non-integer/overflow heights and every non-whitespace trailing byte.
// No allocation or recursion. The caller supplies the same stop/decode deadline.
class response_reader {
public:
  using abort_check = bool (*)(void*);
  constexpr response_reader(std::string_view text, abort_check abort, void* context)
    : text(text), abort(abort), context(context) {}
  constexpr bool parse(uint64_t& height) {
    height = 0;
    if (text.empty() || text.size() > max_response_bytes || !take('{')) return false;
    unsigned fields = 0;
    uint64_t candidate = 0;
    for (;;) {
      std::string_view key, value;
      if (!string(key) || !take(':')) return false;
      unsigned bit = 0;
      if (key == "jsonrpc") { bit = 1; if (!string(value) || value != "2.0") return false; }
      else if (key == "id") { bit = 2; if (!string(value) || value != "0") return false; }
      else if (key == "result") { bit = 4; if (!result(candidate)) return false; }
      else return false;
      if (fields & bit) return false;
      fields |= bit;
      if (take('}')) break;
      if (!take(',')) return false;
    }
    if (fields != 7 || !space() || at != text.size() || cancelled()) return false;
    height = candidate;
    return true;
  }
private:
  constexpr bool cancelled() const { return abort && abort(context); }
  constexpr bool space() {
    while (at < text.size() && (text[at] == ' ' || text[at] == '\n' || text[at] == '\r' || text[at] == '\t')) {
      if (cancelled()) return false;
      ++at;
    }
    return !cancelled();
  }
  constexpr bool take(char c) {
    if (!space() || at == text.size() || text[at] != c) return false;
    ++at; return true;
  }
  constexpr bool string(std::string_view& value) {
    if (!take('"')) return false;
    const size_t start = at;
    while (at < text.size() && text[at] != '"') {
      const unsigned char c = static_cast<unsigned char>(text[at]);
      if (cancelled() || c < 32 || c >= 127 || c == 92) return false;
      ++at;
    }
    if (at == text.size() || cancelled()) return false;
    value = text.substr(start, at - start); ++at; return true;
  }
  constexpr bool number(uint64_t& value) {
    if (!space() || at == text.size() || text[at] < '0' || text[at] > '9') return false;
    value = 0;
    const size_t start = at;
    while (at < text.size() && text[at] >= '0' && text[at] <= '9') {
      if (cancelled() || (at > start && text[start] == '0')) return false;
      const uint64_t digit = static_cast<unsigned>(text[at++] - '0');
      if (value > (std::numeric_limits<uint64_t>::max() - digit) / 10) return false;
      value = value * 10 + digit;
    }
    return true;
  }
  constexpr bool result(uint64_t& height) {
    if (!take('{')) return false;
    unsigned fields = 0;
    for (;;) {
      std::string_view key, status;
      if (!string(key) || !take(':')) return false;
      unsigned bit = 0;
      if (key == "h") { bit = 1; if (!number(height)) return false; }
      else if (key == "status") { bit = 2; if (!string(status) || status != "OK") return false; }
      else return false;
      if (fields & bit) return false;
      fields |= bit;
      if (take('}')) break;
      if (!take(',')) return false;
    }
    return fields == 3;
  }
  const std::string_view text;
  const abort_check abort;
  void* const context;
  size_t at = 0;
};
}}}
