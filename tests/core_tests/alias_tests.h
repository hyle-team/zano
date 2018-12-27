// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"

struct gen_alias_tests : public wallet_test
{
  gen_alias_tests();

  bool generate(std::vector<test_event_entry>& events) const;
  bool check_first_alias_added(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_second_alias_added(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_aliases_removed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_splitted_back(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_alias_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_alias_not_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_alias_added_in_tx(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_height_not_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_height_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_too_many_aliases_registration(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
private:
  uint64_t m_h;
  size_t m_invalid_tx_index;
  size_t m_invalid_block_index;
};

struct gen_alias_strange_data : gen_alias_tests
{
  gen_alias_strange_data();
  bool generate(std::vector<test_event_entry>& events) const;
  bool check_alias_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  currency::account_base m_alice;
};

struct gen_alias_concurrency_with_switch : gen_alias_tests
{
  gen_alias_concurrency_with_switch();
  bool generate(std::vector<test_event_entry>& events) const;
  bool check_alias(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct gen_alias_same_alias_in_tx_pool : gen_alias_tests
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_alias_switch_and_tx_pool : gen_alias_tests
{
  gen_alias_switch_and_tx_pool();
  bool generate(std::vector<test_event_entry>& events) const;
  bool check_alias(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct gen_alias_update_after_addr_changed : gen_alias_tests
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_alias_blocking_reg_by_invalid_tx : gen_alias_tests
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_alias_blocking_update_by_invalid_tx : gen_alias_tests
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_alias_reg_with_locked_money : gen_alias_tests
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_alias_too_much_reward : gen_alias_tests
{
  gen_alias_too_much_reward();
  bool generate(std::vector<test_event_entry>& events) const;
  bool check_alias(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct gen_alias_too_small_reward : gen_alias_tests
{
  gen_alias_too_small_reward();
  bool generate(std::vector<test_event_entry>& events) const;  
  bool make_tx_reg_alias(std::vector<test_event_entry>& events, test_generator &generator, const currency::block& prev_block, const std::string& alias, const currency::account_public_address& alias_addr, uint64_t reward_diff, const currency::account_base& miner_acc, currency::transaction &tx, std::vector<currency::tx_source_entry>& used_sources) const;
  bool init_runtime_config(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_alias(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct gen_alias_tx_no_outs : gen_alias_tests
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_alias_switch_and_check_block_template : gen_alias_tests
{
  gen_alias_switch_and_check_block_template();
  bool generate(std::vector<test_event_entry>& events) const;
  bool add_block_from_template(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct gen_alias_too_many_regs_in_block_template : wallet_test
{
  gen_alias_too_many_regs_in_block_template();
  bool generate(std::vector<test_event_entry>& events) const;
  bool add_block_from_template(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
private:
  mutable uint64_t m_estimated_alias_cost;
  const size_t m_total_alias_to_gen;
};

struct gen_alias_update_for_free : gen_alias_tests
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_alias_in_coinbase : gen_alias_tests
{
  gen_alias_in_coinbase();
  bool generate(std::vector<test_event_entry>& events) const;
  bool check(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};
