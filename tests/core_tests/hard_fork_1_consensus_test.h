// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"


struct hard_fork_1_cumulative_difficulty_base : public wallet_test
{
  hard_fork_1_cumulative_difficulty_base();
  bool generate(std::vector<test_event_entry>& events) const;
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  virtual bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)= 0;
  virtual uint64_t get_hardfork_height()const =0;
};

// this test check if code still work same way as it supposed to work before hard fork point
struct before_hard_fork_1_cumulative_difficulty : public hard_fork_1_cumulative_difficulty_base
{  
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  virtual uint64_t get_hardfork_height()const;
};

// this test check if code still work same way as it supposed to work is split happened before hard fork point but finished after hard fork point
struct inthe_middle_hard_fork_1_cumulative_difficulty : public hard_fork_1_cumulative_difficulty_base
{
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  virtual uint64_t get_hardfork_height()const;
};

// this test check if code follow the new consensus algorithm and prefer balanced branch of the blockchain tree
struct after_hard_fork_1_cumulative_difficulty : public hard_fork_1_cumulative_difficulty_base
{
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  virtual uint64_t get_hardfork_height()const;
};

