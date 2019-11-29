// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"

struct packing_for_pos_minting_wallet_test : public wallet_test
{
  packing_for_pos_minting_wallet_test();
  mined_balance_wallet_test();
  bool generate(std::vector<test_event_entry>& events) const;
  bool set_core_config(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};