// Copyright (c) 2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include "chaingen.h"
#include "wallet_tests_basic.h"

struct hard_fork_1_unlock_time_2_in_normal_tx : public test_chain_unit_enchanced
{
  hard_fork_1_unlock_time_2_in_normal_tx();
  bool generate(std::vector<test_event_entry>& events) const;
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  const size_t m_hardfork_height;
};

struct hard_fork_1_unlock_time_2_in_coinbase : public test_chain_unit_enchanced
{
  hard_fork_1_unlock_time_2_in_coinbase();
  bool generate(std::vector<test_event_entry>& events) const;
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  const size_t m_hardfork_height;
};

