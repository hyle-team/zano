// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"

struct block_template_against_txs_size : public test_chain_unit_enchanced
{
  block_template_against_txs_size();

  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

// consensus rules for the size of a block and the block reward, HF6+ (see the comment in block_reward.cpp):
// generate() only gives coins to the miner, all the scenarios are played inside c1()
struct block_size_penalty_zone : public wallet_test
{
  block_size_penalty_zone();

  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  // tx (Alice -> Bob) with the blob size of exactly tx_size bytes; the tx is not sent to the pool
  bool make_tx_of_size(tools::wallet2& alice_wlt, size_t tx_size, currency::transaction& tx) const;
  // txs_count txs made by make_tx_of_size with the total blob size of exactly total_size bytes
  bool make_txs_of_total_size(tools::wallet2& alice_wlt, size_t txs_count, size_t total_size, std::vector<currency::transaction>& txs) const;
  // builds a PoW block with the given txs on top of the main chain and hands it over to the core, bypassing the pool and the block template;
  // etc_coinbase_block_cumulative_size of the coinbase holds declared_size, and the coinbase pays the reward of a block of size_for_reward bytes
  bool push_block(currency::core& c, const std::vector<currency::transaction>& txs, size_t declared_size, size_t size_for_reward, bool expected_to_be_accepted, const std::string& descr) const;
  // checks the txs, the size and the reward of the top block (the reward is checked by the emission and by the miner's balance)
  bool check_top_block(currency::core& c, tools::wallet2& miner_wlt, const std::vector<currency::transaction>& txs, uint64_t expected_reward, const std::string& descr) const;

  mutable size_t m_tx_size_without_padding = 0; // size of Alice's tx without the bytes of extra_padding, see make_tx_of_size()
  mutable uint64_t m_miner_balance = 0;         // miner's balance as of the previous check_top_block() call
};

