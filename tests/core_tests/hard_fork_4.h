// Copyright (c) 2023-2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"


struct hard_fork_4_consolidated_txs : public wallet_test
{
  hard_fork_4_consolidated_txs();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable bool m_post_hf4_zarcanum = false;
  mutable uint64_t m_bob_amount = 0;
};


struct hardfork_4_explicit_native_ids_in_outs : public wallet_test
{
  hardfork_4_explicit_native_ids_in_outs();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable uint64_t m_alice_initial_balance = 0;
};
