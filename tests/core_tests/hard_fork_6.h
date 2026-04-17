// Copyright (c) 2025 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"
#include "random_helper.h"


struct hard_fork_6_intrinsic_payment_id_basic_test : public wallet_test
{
  hard_fork_6_intrinsic_payment_id_basic_test();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
  bool c2(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events);

protected:
  mutable crypto::public_key m_asset_id;
  mutable crypto::hash m_tx_2_id;
  mutable crypto::hash m_tx_3_id;
  mutable crypto::hash m_tx_4_id;
};


struct hard_fork_6_intrinsic_payment_id_rpc_test : public wallet_test
{
  hard_fork_6_intrinsic_payment_id_rpc_test();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events);

  //random_state_test_restorer m_random_state_test_restorer;

protected:
  mutable crypto::public_key m_asset_id;
  
  mutable crypto::hash m_tx_0_id;
  mutable crypto::hash m_tx_1_id;
  mutable crypto::hash m_tx_2_id;
  mutable crypto::hash m_tx_3_id;
};

struct hard_fork_6_full_gw_tx_test : public wallet_test
{
  hard_fork_6_full_gw_tx_test();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
  bool c2(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events);

protected:
  mutable crypto::public_key m_asset1_id{};
  mutable crypto::public_key m_asset2_id{};

  mutable currency::keypair m_gw_addr1_view{};
  mutable currency::keypair m_gw_addr1_spend{};
  mutable currency::keypair m_gw_addr2_view{};
  mutable currency::keypair m_gw_addr2_spend{};
};

struct hard_fork_6_and_alt_chain : public wallet_test
{
  hard_fork_6_and_alt_chain();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events);

};

struct hard_fork_6_and_self_directed_tx_with_payment_id : public wallet_test
{
  hard_fork_6_and_self_directed_tx_with_payment_id();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
};

struct hard_fork_6_coinbase_size_rules : public wallet_test
{
  hard_fork_6_coinbase_size_rules();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
};
