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

//
// default alias tests
//
// The "default alias" feature lets an address that owns several aliases mark one of them as the one
// UIs should display. It is set by an alias update that re-assigns an alias to its current owner
// without touching anything else (same address, same comment, same view key). The daemon keeps the
// choice in a per-address stack keyed by tx id, so that a chain switch can roll it back.
//

struct gen_alias_default_alias : gen_alias_tests
{
  gen_alias_default_alias();
  bool generate(std::vector<test_event_entry>& events) const;

  bool check_after_first_reg(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_after_second_alias(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_after_third_alias(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_after_set_default(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_after_default_transferred(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_after_transfer_rolled_back(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_after_set_default_rolled_back(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  mutable currency::account_public_address m_alice_addr;
  mutable currency::account_public_address m_bob_addr;
};

struct gen_alias_default_alias_last_alias_reorg : gen_alias_tests
{
  gen_alias_default_alias_last_alias_reorg();
  bool generate(std::vector<test_event_entry>& events) const;

  bool check_after_reg(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_after_transfer(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_after_rollback(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_after_regained_second_alias(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  mutable currency::account_public_address m_alice_addr;
  mutable currency::account_public_address m_bob_addr;
};

struct gen_alias_default_alias_info_update : gen_alias_tests
{
  gen_alias_default_alias_info_update();
  bool generate(std::vector<test_event_entry>& events) const;

  bool check_before_info_update(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_after_info_update(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_after_info_update_rolled_back(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  mutable currency::account_public_address m_alice_addr;
};
