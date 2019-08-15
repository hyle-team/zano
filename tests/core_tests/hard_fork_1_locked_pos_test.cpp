// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "pos_validation.h"
#include "tx_builder.h"
#include "hard_fork_1_locked_pos_test.h"
#include "random_helper.h"

using namespace epee;
using namespace crypto;
using namespace currency;

hard_fork_1_locked_mining_test::hard_fork_1_locked_mining_test()
{
  REGISTER_CALLBACK_METHOD(hard_fork_1_locked_mining_test, c1);
  REGISTER_CALLBACK_METHOD(hard_fork_1_locked_mining_test, configure_core);
}

bool hard_fork_1_locked_mining_test::generate(std::vector<test_event_entry>& events) const
{
  random_state_test_restorer::reset_random();

  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(pos_miner_acc);
  m_accounts.push_back(miner_acc);
  m_accounts.push_back(pos_miner_acc);
  std::list<account_base> miner_acc_lst(1, miner_acc);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, 1564434616);
  generator.set_hardfork_height(get_hardfork_height());

  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  //construc tx that locks transaction for some period of time
  // make a couple of huge txs
  bool r = false;
  std::vector<extra_v> extra;

  std::vector<tx_source_entry> sources_1;
  r = fill_tx_sources(sources_1, events, blk_0r, miner_acc.get_keys(), 2000000000000+TESTS_DEFAULT_FEE, 0);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  std::vector<tx_destination_entry> destinations({ tx_destination_entry(2010000000000, pos_miner_acc.get_public_address()) });
  crypto::secret_key stub;
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(miner_acc.get_keys(), sources_1, destinations, extra, empty_attachment, tx_1, stub, get_block_height(blk_0r)+2000);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_1); // push it to the pool  
  
  MAKE_NEXT_BLOCK_TX1(events, blk_0r_tx, blk_0r, miner_acc, tx_1);

  block last_block = blk_0r_tx;
  for (size_t i = 0; i != CURRENCY_MINED_MONEY_UNLOCK_WINDOW; i++)
  {
    MAKE_NEXT_POS_BLOCK(events, next_blk_pos, last_block, miner_acc, miner_acc_lst);
    MAKE_NEXT_BLOCK(events, next_blk_pow, next_blk_pos, miner_acc);
    events.push_back(event_core_time(next_blk_pow.timestamp - 10));
    last_block = next_blk_pow;
  }
  
  std::list<currency::account_base> accounts_2;
  accounts_2.push_back(pos_miner_acc);
  //let's try to mint PoS block from locked account
  MAKE_NEXT_POS_BLOCK(events, next_blk_pos, last_block, pos_miner_acc, accounts_2);

  DO_CALLBACK(events, "c1");
  return true;
}

bool hard_fork_1_locked_mining_test::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE; //four blocks
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH; //four blocks
  pc.hard_fork1_starts_after_height = get_hardfork_height();

  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

bool hard_fork_1_locked_mining_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  
  uint64_t h = c.get_blockchain_storage().get_current_blockchain_size();
  CHECK_AND_ASSERT_MES(h == 36, false, "locked pos block is not accepted");
  return true;
}

uint64_t hard_fork_1_locked_mining_test::get_hardfork_height()const
{
  return 10; 
}

