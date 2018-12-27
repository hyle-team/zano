// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"

struct cumulative_difficulty_adjustment_test : public test_chain_unit_base
{
  cumulative_difficulty_adjustment_test();
  ~cumulative_difficulty_adjustment_test();

  bool check_block_verification_context(const currency::block_verification_context& bvc, size_t event_idx, const currency::block& /*blk*/)
  {
      return true;
  }

  bool generate(std::vector<test_event_entry>& events) const;
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool configure_check_height1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool memorize_main_chain(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_main_chain(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_reorganize(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool remember_block_befor_alt(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  

private:
  currency::block_extended_info bei;
};

struct cumulative_difficulty_adjustment_test_alt : public test_chain_unit_base
{
  cumulative_difficulty_adjustment_test_alt();

  bool check_block_verification_context(const currency::block_verification_context& bvc, size_t event_idx, const currency::block& /*blk*/)
  {
    return true;
  }

  bool generate(std::vector<test_event_entry>& events) const;
  //bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool configure_check_height1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  //bool memorize_main_chain(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  //bool check_main_chain(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  currency::block_extended_info bei;
};