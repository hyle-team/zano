#pragma once
// AUTO-GENERATED. refs_normal membership for blocks [3833001, 3878977], normal (non-coinbase) txs only.
// Key (amount, gindex) as in m_db_outputs; amount==0 for ZC. Data lives in out_back_refs_hop1.cpp (add it to the build).
#include <cstdint>

namespace currency
{
  static constexpr uint64_t c_hf6_rollback_min_height = 3833001; // including
  static constexpr uint64_t c_hf6_rollback_max_height = 3878977; // including

  bool is_out_in_back_refs_hop1(uint64_t amount, uint64_t gindex);
}
