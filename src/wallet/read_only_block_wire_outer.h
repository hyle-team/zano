// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#pragma once

#include "read_only_block_transport.h"
#include <vector>
#include <optional>

struct binary_decode_budget;

namespace tools { namespace read_only { namespace wire
{
  // Borrowed from one immutable response body. No view may outlive that body.
  struct bytes_view { const char* data = nullptr; size_t size = 0; };
  struct block_view
  {
    bytes_view block;
    // Array spans omit the type byte, but INCLUDE the packed element count
    // and complete payload. txs contains strings, tx_indices contains sections
    // with an optional v uint64 array, and coinbase_indices is a uint64 array.
    // An absent array is {nullptr, 0}, count 0. Present empty arrays retain
    // their packed count. Every span is validated by read_outer before use.
    bytes_view txs;
    size_t tx_count = 0;
    size_t max_tx_blob_bytes = 0; // largest actual validated transaction blob
    bytes_view coinbase_indices;
    size_t coinbase_index_count = 0;
    bytes_view tx_indices;
    size_t tx_index_count = 0;
  };
  struct outer_response
  {
    std::string status;
    uint64_t start_height = 0;
    uint64_t current_height = 0;
    uint64_t current_hardfork = 0;
    std::optional<uint64_t> max_count; // Optional on wire; native consumer requires its exact echo.
    std::vector<block_view> blocks;
  };

  // Full finite-schema validation, then atomic publication of borrowed views.
  // Allocations consume the caller's cumulative budget before requesting storage.
  // On failure out is empty. Cancellation is reported as DecodeDeadline; the
  // caller's shared stop/deadline watch supplies the precise cancellation reason.
  transport_failure read_outer(const std::string& body,
    const block_transport_limits&, binary_decode_budget&, outer_response& out,
    transport_limit_context&);

  using tx_visitor = transport_failure (*)(void*, bytes_view);
  using tx_indices_visitor = transport_failure (*)(void*, bytes_view, size_t);
  using tx_pair_visitor = transport_failure (*)(void*, bytes_view transaction, bytes_view indices, size_t index_count);
  // Allocation-free second traversal of previously validated array spans.
  // Visits serialized order and forwards the first visitor failure. No logical
  // element charge is repeated. Polls the shared budget's cancellation callback.
  transport_failure for_each_tx(const block_view&, binary_decode_budget&, tx_visitor, void*);
  transport_failure for_each_tx_indices(const block_view&, binary_decode_budget&, tx_indices_visitor, void*);
  // Walks the two validated arrays together without per-transaction scratch.
  // Different transaction/index-section counts return InvalidResponse; absent
  // arrays retain their zero defaults. An absent v visits {nullptr, 0}, count 0.
  transport_failure for_each_tx_pair(const block_view&, binary_decode_budget&, tx_pair_visitor, void*);
  // Reads one LE uint64 from an indices span that INCLUDES its packed count.
  // An absent/empty/out-of-range span returns false; no unaligned typed loads.
  bool read_index(bytes_view indices, size_t index, uint64_t& value) noexcept;
}}}
