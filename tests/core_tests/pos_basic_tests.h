// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"

struct gen_pos_basic_tests : public test_chain_unit_base
{
  gen_pos_basic_tests();

  bool check_block_verification_context(const currency::block_verification_context& bvc, size_t event_idx, const currency::block& /*blk*/)
  {
      return true;
  }

  bool generate(std::vector<test_event_entry>& events) const;
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool configure_check_height1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool configure_check_height2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_exchange_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

};


struct pos_mining_with_decoys : public wallet_test
{
  pos_mining_with_decoys();
  bool generate(std::vector<test_event_entry>& events) const;
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};
