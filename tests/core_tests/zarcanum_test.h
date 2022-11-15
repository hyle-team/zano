// Copyright (c) 2014-2022 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"


struct zarcanum_basic_test : public wallet_test
{
  zarcanum_basic_test();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct zarcanum_test_n_inputs_validation : public wallet_test
{
  zarcanum_test_n_inputs_validation();
  bool generate(std::vector<test_event_entry>& events) const;
};

struct zarcanum_gen_time_balance : public wallet_test
{
  zarcanum_gen_time_balance();
  bool generate(std::vector<test_event_entry>& events) const;
};

struct zarcanum_pos_block_math : public test_chain_unit_enchanced
{
  zarcanum_pos_block_math();
  bool generate(std::vector<test_event_entry>& events) const;
};

struct zarcanum_txs_with_big_shuffled_decoy_set_shuffled : public wallet_test
{
  zarcanum_txs_with_big_shuffled_decoy_set_shuffled();
  bool generate(std::vector<test_event_entry>& events) const;
};


