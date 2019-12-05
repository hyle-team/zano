// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"

const size_t invalid_index_value = std::numeric_limits<size_t>::max();


template<class concrete_test>
class gen_double_spend_base : public test_chain_unit_enchanced
{
public:
  static const uint64_t send_amount = MK_TEST_COINS(17);

  gen_double_spend_base();

  bool mark_last_valid_block(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_double_spend(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  currency::block m_last_valid_block;
};


template<bool txs_kept_by_block>
struct gen_double_spend_in_tx : public gen_double_spend_base< gen_double_spend_in_tx<txs_kept_by_block> >
{
  static const uint64_t send_amount = MK_TEST_COINS(70);
  static const bool has_invalid_tx = true;
  static const size_t expected_pool_txs_count = 0;
  static const uint64_t expected_bob_balance = send_amount;
  static const uint64_t expected_alice_balance = 0;

  bool generate(std::vector<test_event_entry>& events) const;
};


template<bool txs_kept_by_block>
struct gen_double_spend_in_the_same_block : public gen_double_spend_base< gen_double_spend_in_the_same_block<txs_kept_by_block> >
{
  static const uint64_t send_amount = MK_TEST_COINS(17);
  static const bool has_invalid_tx = !txs_kept_by_block;
  static const size_t expected_pool_txs_count = 1;
  static const uint64_t expected_bob_balance = send_amount;
  static const uint64_t expected_alice_balance = 0;

  bool generate(std::vector<test_event_entry>& events) const;
};


template<bool txs_kept_by_block>
struct gen_double_spend_in_different_blocks : public gen_double_spend_base< gen_double_spend_in_different_blocks<txs_kept_by_block> >
{
  static const uint64_t send_amount = MK_TEST_COINS(17);
  static const bool has_invalid_tx = !txs_kept_by_block;
  static const size_t expected_pool_txs_count = 0;
  static const uint64_t expected_bob_balance = 0;
  static const uint64_t expected_alice_balance = send_amount - TESTS_DEFAULT_FEE;

  bool generate(std::vector<test_event_entry>& events) const;
};


template<bool txs_kept_by_block>
struct gen_double_spend_in_alt_chain_in_the_same_block : public gen_double_spend_base< gen_double_spend_in_alt_chain_in_the_same_block<txs_kept_by_block> >
{
  static const uint64_t send_amount = MK_TEST_COINS(17);
  static const bool has_invalid_tx = !txs_kept_by_block;
  static const size_t expected_pool_txs_count = txs_kept_by_block ?  0 :1;
  static const uint64_t expected_bob_balance = send_amount;
  static const uint64_t expected_alice_balance = 0;

  bool generate(std::vector<test_event_entry>& events) const;
};


template<bool txs_kept_by_block>
struct gen_double_spend_in_alt_chain_in_different_blocks : public gen_double_spend_base< gen_double_spend_in_alt_chain_in_different_blocks<txs_kept_by_block> >
{
  static const uint64_t send_amount = MK_TEST_COINS(17);
  static const bool has_invalid_tx = !txs_kept_by_block;
  static const size_t expected_pool_txs_count = txs_kept_by_block ? 0:1;
  static const uint64_t expected_bob_balance = send_amount;
  static const uint64_t expected_alice_balance = 0; 
  bool generate(std::vector<test_event_entry>& events) const;
};

class gen_double_spend_in_different_chains : public test_chain_unit_base
{
public:
  static const uint64_t send_amount = MK_TEST_COINS(10);
  static const size_t expected_blockchain_height = 4 + 2 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW;

  gen_double_spend_in_different_chains();

  bool generate(std::vector<test_event_entry>& events) const;

  bool check_double_spend(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};


#define INIT_DOUBLE_SPEND_TEST()                                           \
  uint64_t ts_start = 1338224400;                                          \
  GENERATE_ACCOUNT(miner_account);                                         \
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);              \
  MAKE_ACCOUNT(events, bob_account);                                       \
  MAKE_ACCOUNT(events, alice_account);                                     \
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);                     \
  MAKE_TX(events, tx_0, miner_account, bob_account, send_amount, blk_0r);  \
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_account, tx_0);         \
  REWIND_BLOCKS(events, blk_1r, blk_1, miner_account);


#include "double_spend.inl"
