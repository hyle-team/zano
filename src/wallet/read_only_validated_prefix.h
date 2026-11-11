// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#pragma once

#include <atomic>
#include <cstddef>
#include "rpc/core_rpc_server_commands_defs.h"

namespace tools { namespace read_only
{
  struct prefix_selection_limits
  {
    size_t max_blocks;       // Includes the inclusive anchor; must be at least two.
    size_t max_transactions; // Includes every coinbase.
  };

  struct validated_prefix_response
  {
    currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response selected;
    size_t full_block_count = 0;
    bool prefix_truncated = false;
  };

  // Native scanner side interface. Success requires validation of the entire
  // capped reply before publishing its whole-block prefix. No borrowed wire
  // views escape. Failure empties the result; the legacy DIRECT API stays whole.
  class validated_prefix_source
  {
  public:
    virtual ~validated_prefix_source() = default;
    virtual bool fetch_validated_prefix(
      const currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request& request,
      prefix_selection_limits limits, validated_prefix_response& response,
      const std::atomic<bool>& stop) = 0;
  };
}}
