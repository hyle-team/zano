// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"

struct mix_attr_tests : public wallet_test
{
  mix_attr_tests();
  bool generate(std::vector<test_event_entry>& events) const;

  bool check_balances_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool remember_last_block(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_last_not_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_last2_and_balance(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_last_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
private:
  crypto::hash top_id_befor_split;
};

struct mix_in_spent_outs : public wallet_test
{
  mix_in_spent_outs();
  bool generate(std::vector<test_event_entry>& events) const;
  
  bool check_outs_count(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  mutable uint64_t m_test_amount;

};
