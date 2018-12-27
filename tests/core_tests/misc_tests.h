// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "chaingen.h"

bool check_allowed_types_in_variant_container_test();

bool check_u8_str_case_funcs();
bool chec_u8_str_matching();

bool check_hash_and_difficulty_monte_carlo_test();

struct block_template_vs_invalid_txs_from_pool : public test_chain_unit_enchanced
{
  block_template_vs_invalid_txs_from_pool();
  bool generate(std::vector<test_event_entry>& events) const;
  bool check_block_template(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct test_blockchain_vs_spent_keyimges : public test_chain_unit_enchanced
{
  test_blockchain_vs_spent_keyimges();
  bool generate(std::vector<test_event_entry>& events) const;
  bool check(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable currency::transaction m_tx_1;
};

struct test_blockchain_vs_spent_multisig_outs : public test_chain_unit_enchanced
{
  test_blockchain_vs_spent_multisig_outs();
  bool generate(std::vector<test_event_entry>& events) const;
  bool check(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable currency::transaction m_tx_1;
};
