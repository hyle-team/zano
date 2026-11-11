// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#pragma once
#include "read_only_block_wire_outer.h"
#include "currency_core/currency_basic.h"
namespace tools { namespace read_only { namespace wire {
struct transaction_summary {
  uint64_t version = 0;
  size_t outputs = 0;
  size_t canonical_prefix_bytes = 0;
  bool first_input_generation = false;
  uint64_t generation_height = 0;
  crypto::hash prefix_hash{};
};
struct block_summary {
  currency::block_header header{};
  transaction_summary miner;
  bytes_view transaction_hashes{};
  size_t transaction_count = 0;
};
// Native-prefix-only cumulative work. Maximum is the SAME owned reply's
// body.size(), not a tuned constant. Borrowed by each cursor; never reset.
// No-work-pointer callers preserve the original full-graph/work budget.
struct inner_collection_work {
  explicit inner_collection_work(uint64_t maximum) : maximum(maximum) {}
  const uint64_t maximum;
  uint64_t elements = 0;
  bool refused = false;
  bool invalid_progress = false;

  bool admit(uint64_t count, uint64_t per_collection) noexcept {
    if (refused || invalid_progress) return false;
    // Subtraction before addition prevents wrap, including UINT64_MAX.
    if (count > per_collection || elements > maximum || count > maximum-elements) {
      refused = true;
      return false;
    }
    elements += count;
    return true;
  }
  bool begin_element(size_t position, size_t& previous, bool& seen) noexcept {
    if (invalid_progress || (seen && position <= previous)) {
      invalid_progress = true;
      return false;
    }
    previous = position;
    seen = true;
    return true;
  }
  bool end_element(size_t before, size_t after) noexcept {
    if (invalid_progress || after <= before) {
      invalid_progress = true;
      return false;
    }
    return true;
  }
};
// One logical traversal. null destinations never allocate a decoded graph.
// Scratch is caller-owned, precharged once, fixed-size and reused.
// With work: each collection <= budget.max_elements; selected source elements
// cumulatively use that budget. All elements additionally use the shared work.
// Without work: every source element cumulatively uses budget, as before.
transport_failure decode_transaction(bytes_view, binary_decode_budget&, std::vector<char>&,
  currency::transaction*, transaction_summary&, inner_collection_work* = nullptr);
transport_failure decode_block(bytes_view, binary_decode_budget&, std::vector<char>&,
  currency::block*, block_summary&, inner_collection_work* = nullptr);
}}}
