// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"

struct get_random_outs_test : public test_chain_unit_enchanced
{
  get_random_outs_test();
  bool generate(std::vector<test_event_entry>& events) const;
  bool check_get_rand_outs(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  mutable uint64_t m_amount;
};

struct random_outs_and_burnt_coins : public wallet_test
{
  random_outs_and_burnt_coins();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable uint64_t m_amount;
  static constexpr uint64_t m_fake_amounts_count = 3;
};
