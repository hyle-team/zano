// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#pragma once
#include <cstdint>
#include "native_wallet_birthday_prefix.h"

namespace tools { namespace read_only {
namespace policy = zano::native_wallet_birthday_prefix;
enum class origin_phase : uint8_t { Absent, Scanning, Complete };
struct native_history_origin
{
  origin_phase phase = origin_phase::Absent;
  uint64_t rounded_date = 0;
  uint64_t first_height = 0;
  policy::Hash anchor_hash{};
  policy::ChainView view{};
  uint64_t connected_height = 0;
  policy::Hash connected_hash{};
};
}}
