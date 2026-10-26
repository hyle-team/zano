// Copyright (c) 2014-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstdint>
#include <unordered_map>

namespace currency
{
namespace bare_outputs_snapshot
{
  // Full amount-bucket sizes: valid global indices are [0, count).
  extern const std::unordered_map<std::uint64_t, std::uint64_t> bare_output_count_by_amount;
}
}
