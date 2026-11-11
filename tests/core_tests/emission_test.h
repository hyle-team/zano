// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include "chaingen.h"
#include "wallet_tests_basic.h"

struct emission_test : public test_chain_unit_enchanced
{
  emission_test();
  bool generate(std::vector<test_event_entry> &events);
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
  mutable currency::account_base m_miner_acc;
};


struct pos_emission_test : public wallet_test
{
  enum pos_entries_change_scheme { change_pos_entries_count, change_total_money_in_minting } ;
  pos_emission_test();
  bool generate(std::vector<test_event_entry> &events);
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
  bool c2(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
  bool c3(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events);

  bool populate_wallet_with_pos_entries(std::shared_ptr<tools::wallet2> w, std::shared_ptr<tools::wallet2> coin_source, size_t iter_idx, pos_entries_change_scheme change_scheme);

  mutable uint64_t m_total_money_in_minting;
  mutable size_t m_pos_entries_to_generate;
  mutable uint64_t m_pos_entry_amount;

};
