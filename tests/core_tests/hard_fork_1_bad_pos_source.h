// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"


struct hard_fork_1_bad_pos_source : public wallet_test
{
  hard_fork_1_bad_pos_source();
  bool generate(std::vector<test_event_entry>& events) const;
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  virtual bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  virtual uint64_t get_hardfork_height()const;
};
