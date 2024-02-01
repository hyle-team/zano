// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "pos_validation.h"
#include "tx_builder.h"
#include "hard_fork_1_bad_pos_source.h"
#include "random_helper.h"

using namespace epee;
using namespace crypto;
using namespace currency;

hard_fork_1_bad_pos_source::hard_fork_1_bad_pos_source()
{
  REGISTER_CALLBACK_METHOD(hard_fork_1_bad_pos_source, c1);
  REGISTER_CALLBACK_METHOD(hard_fork_1_bad_pos_source, configure_core);
}

bool hard_fork_1_bad_pos_source::generate(std::vector<test_event_entry>& events) const
{
  random_state_test_restorer::reset_random();

  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(pos_miner_acc_before_pow);
  GENERATE_ACCOUNT(pos_miner_acc_after_pow);
  m_accounts.push_back(miner_acc);
  m_accounts.push_back(pos_miner_acc_before_pow);
  m_accounts.push_back(pos_miner_acc_after_pow);
  std::list<account_base> miner_acc_lst(1, miner_acc);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, 1564434616);
  generator.set_hardfork_height(1, get_hardfork_height());

  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 5);
  MAKE_TX(events, tx_1, miner_acc, pos_miner_acc_before_pow, 1000000000000, blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_pow_tx1, blk_0r, miner_acc, tx_1);
  MAKE_TX(events, tx_2, preminer_acc, pos_miner_acc_after_pow, 1000000000000, blk_pow_tx1);
  MAKE_NEXT_POS_BLOCK_TX1(events, blk_pos_tx2, blk_pow_tx1, miner_acc, miner_acc_lst, tx_2);

  block last_block = blk_pos_tx2;
  for (size_t i = 0; i != CURRENCY_MINED_MONEY_UNLOCK_WINDOW; i++)
  {
    MAKE_NEXT_POS_BLOCK(events, next_blk_pos, last_block, miner_acc, miner_acc_lst);
    events.push_back(event_core_time(next_blk_pos.timestamp - 10));
    last_block = next_blk_pos;
  }
  
  std::list<currency::account_base> accounts_before_pow;
  accounts_before_pow.push_back(pos_miner_acc_before_pow);
  //let's try to mint PoW block from account 
  MAKE_NEXT_POS_BLOCK(events, next_blk_pos2, last_block, miner_acc, accounts_before_pow);

  std::list<currency::account_base> accounts_after_pow;
  accounts_after_pow.push_back(pos_miner_acc_after_pow);
  //let's try to mint PoW block from account 
  generator.set_ignore_last_pow_in_wallets(true);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_POS_BLOCK(events, next_blk_pos3, next_blk_pos2, miner_acc, accounts_after_pow);

  DO_CALLBACK(events, "c1");
  return true;
}

bool hard_fork_1_bad_pos_source::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE; //four blocks
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH; //four blocks
  pc.hard_forks.set_hardfork_height(1, get_hardfork_height());

  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

bool hard_fork_1_bad_pos_source::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  
  uint64_t h = c.get_blockchain_storage().get_current_blockchain_size();
  CHECK_AND_ASSERT_MES(h == 29, false, "locked pos block is not accepted");
  return true;
}

uint64_t hard_fork_1_bad_pos_source::get_hardfork_height()const
{
  return 10; 
}

