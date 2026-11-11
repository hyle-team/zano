// Copyright (c) 2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include "chaingen.h"
#include "wallet_tests_basic.h"
#include "random_helper.h"

struct hard_fork_2_base_test : public wallet_test
{
  hard_fork_2_base_test(size_t hardfork_02_height);
  hard_fork_2_base_test(size_t hardfork_01_height, size_t hardfork_02_height);
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  size_t m_hardfork_01_height;
  size_t m_hardfork_02_height;
  size_t m_hardfork_03_height;
};

struct hard_fork_2_tx_payer_in_wallet : public hard_fork_2_base_test
{
  hard_fork_2_tx_payer_in_wallet();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct hard_fork_2_tx_receiver_in_wallet : public hard_fork_2_base_test
{
  hard_fork_2_tx_receiver_in_wallet();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable uint64_t m_alice_start_balance;
};

struct hard_fork_2_tx_extra_alias_entry_in_wallet : public hard_fork_2_base_test
{
  hard_fork_2_tx_extra_alias_entry_in_wallet();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct hard_fork_2_auditable_addresses_basics : public wallet_test //hard_fork_2_base_test
{
  hard_fork_2_auditable_addresses_basics();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct hard_fork_2_no_new_structures_before_hf : public hard_fork_2_base_test
{
  using hard_fork_2_base_test::check_block_verification_context; // this is necessary for correct work of do_check_block_verification_context, consider rafactoring

  hard_fork_2_no_new_structures_before_hf();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

template<bool before_hf_2>
struct hard_fork_2_awo_wallets_basic_test : public hard_fork_2_base_test
{
  hard_fork_2_awo_wallets_basic_test();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

template<bool before_hf_2>
struct hard_fork_2_alias_update_using_old_tx : public hard_fork_2_base_test
{
  hard_fork_2_alias_update_using_old_tx();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  random_state_test_restorer m_random_state_restorer;
};

template<bool before_hf_2>
struct hard_fork_2_incorrect_alias_update : public hard_fork_2_base_test
{
  hard_fork_2_incorrect_alias_update();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};
