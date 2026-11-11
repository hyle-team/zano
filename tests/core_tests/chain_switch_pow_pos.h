// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"

struct gen_chain_switch_pow_pos : public test_chain_unit_base
{
  gen_chain_switch_pow_pos();

  bool check_block_verification_context(const currency::block_verification_context& bvc, size_t event_idx, const currency::block& /*block*/)
  {
    if (m_invalid_block_index == event_idx)
      return bvc.m_verification_failed;
   return !bvc.m_verification_failed;
  }

  bool mark_invalid_block(currency::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
  {
    m_invalid_block_index = ev_index + 1;
    return true;
  }


  bool generate(std::vector<test_event_entry>& events) const;
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_height1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_chains_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_balance_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable test_generator generator;
  mutable std::list<currency::account_base> m_accounts;
  mutable uint64_t m_premine_amount;
  mutable uint64_t m_enormous_fee;
  size_t m_invalid_block_index;
};

struct pow_pos_reorganize_specific_case : public test_chain_unit_enchanced
{
  pow_pos_reorganize_specific_case();
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool generate(std::vector<test_event_entry>& events) const;
};
