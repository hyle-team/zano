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
  // Test idea: make sure PoS mining on locked coins is possible

  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(pos_miner_acc);
  m_accounts.push_back(miner_acc);
  m_accounts.push_back(pos_miner_acc);
  std::list<account_base> miner_acc_lst(1, miner_acc);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, 1564434616);
  generator.set_hardfork_height(1, get_hardfork_height());

  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  //construct tx that locks coins for some period of time
  //make a couple of huge txs
  bool r = false;
  std::vector<extra_v> extra;

  std::vector<tx_source_entry> sources_1;
  r = fill_tx_sources(sources_1, events, blk_0r, miner_acc.get_keys(), CURRENCY_BLOCK_REWARD + TESTS_DEFAULT_FEE, 0);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  std::vector<tx_destination_entry> destinations({ tx_destination_entry(CURRENCY_BLOCK_REWARD, pos_miner_acc.get_public_address()) });
  crypto::secret_key stub;
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  uint64_t unlock_time = get_block_height(blk_0r) + 2000;
  r = construct_tx(miner_acc.get_keys(), sources_1, destinations, extra, empty_attachment, tx_1, get_tx_version_from_events(events), stub, unlock_time);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_1); // push it to the pool  

  uint64_t ut1 = get_tx_x_detail<etc_tx_details_unlock_time>(tx_1);
  etc_tx_details_unlock_time2 ut2 = AUTO_VAL_INIT(ut2);
  get_type_in_variant_container(tx_1.extra, ut2);
  std::stringstream ss;
  for (auto v : ut2.unlock_time_array)
    ss << v << " ";
  LOG_PRINT_YELLOW("tx1: ut1: " << ut1 << ", ut2: " << ss.str(), LOG_LEVEL_0);

  
  MAKE_NEXT_BLOCK_TX1(events, blk_0r_tx, blk_0r, miner_acc, tx_1);

  block last_block = blk_0r_tx;
  for (size_t i = 0; i != CURRENCY_MINED_MONEY_UNLOCK_WINDOW; i++)
  {
    MAKE_NEXT_POS_BLOCK(events, next_blk_pos, last_block, miner_acc, miner_acc_lst);
    MAKE_NEXT_BLOCK(events, next_blk_pow, next_blk_pos, miner_acc);
    events.push_back(event_core_time(next_blk_pow.timestamp - 10));
    last_block = next_blk_pow;
  }

  MAKE_NEXT_BLOCK(events, blk_1, last_block, miner_acc);
  MAKE_NEXT_POS_BLOCK(events, blk_2, blk_1, miner_acc, miner_acc_lst);
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);
  
  std::list<currency::account_base> accounts_2;
  accounts_2.push_back(pos_miner_acc);
  //mint PoS block from locked account into an altchain
  MAKE_NEXT_POS_BLOCK(events, next_blk_pos, last_block, pos_miner_acc, accounts_2);

  // switch chains
  MAKE_NEXT_BLOCK(events, blk_2a, next_blk_pos, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_3a, blk_2a, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_4a, blk_3a, miner_acc);

  // make sure switch happened
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_4a));

  REWIND_BLOCKS_N_WITH_TIME(events, blk_4ar, blk_4a, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // mint another PoS block from locked account
  MAKE_NEXT_POS_BLOCK(events, blk_5a, blk_4ar, pos_miner_acc, accounts_2);


  return true;
}

bool hard_fork_1_locked_mining_test::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE; //four blocks
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH; //four blocks
  pc.hard_forks.set_hardfork_height(1, get_hardfork_height());

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

