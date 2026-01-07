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

protected:
  mutable crypto::public_key m_asset_id;
  mutable crypto::hash m_tx_2_id;
  mutable crypto::hash m_tx_3_id;
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
