// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "pos_validation.h"
#include "tx_builder.h"
#include "hard_fork_1_consensus_test.h"
#include "random_helper.h"

using namespace epee;
using namespace crypto;
using namespace currency;

hard_fork_1_cumulative_difficulty_base::hard_fork_1_cumulative_difficulty_base()
{
  REGISTER_CALLBACK_METHOD(hard_fork_1_cumulative_difficulty_base, c1);
  REGISTER_CALLBACK_METHOD(hard_fork_1_cumulative_difficulty_base, configure_core);
}

bool hard_fork_1_cumulative_difficulty_base::generate(std::vector<test_event_entry>& events) const
{
  random_state_test_restorer::reset_random();

  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  std::list<account_base> miner_acc_lst(1, miner_acc);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, 1564434616);
  generator.set_hardfork_height(1, get_hardfork_height());

  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);
  block last_block = blk_0r;
  for (size_t i = 0; i != 20; i++)
  {
    MAKE_NEXT_POS_BLOCK(events, next_blk_pos, last_block, miner_acc, miner_acc_lst);
    MAKE_NEXT_BLOCK(events, next_blk_pow, next_blk_pos, miner_acc);
    events.push_back(event_core_time(next_blk_pow.timestamp - 10));

    last_block = next_blk_pow;
  }
  
  // generator.set_pos_to_low_timestamp(true); <-- this is not supported anymore -- sowle
  last_block = blk_0r; 
  for (size_t i = 0; i != 20; i++)
  {
    MAKE_NEXT_POS_BLOCK(events, next_blk_pos, last_block, miner_acc, miner_acc_lst);
    MAKE_NEXT_BLOCK_TIMESTAMP_ADJUSTMENT(-14, events, next_blk_pow, next_blk_pos, miner_acc);
    last_block = next_blk_pow;
  }


  DO_CALLBACK(events, "c1");
  return true;
}

bool hard_fork_1_cumulative_difficulty_base::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE; //four blocks
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH; //four blocks
  pc.hard_forks.set_hardfork_height(1, get_hardfork_height());

  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

bool before_hard_fork_1_cumulative_difficulty::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  
  std::shared_ptr<block_extended_info> last_pow_block = c.get_blockchain_storage().get_last_block_of_type(false);
  std::shared_ptr<block_extended_info> last_pos_block = c.get_blockchain_storage().get_last_block_of_type(true);
  CHECK_AND_ASSERT_MES(last_pow_block->cumulative_diff_precise == 184, false, "Incorrect condition failed: last_pow_block->cumulative_diff_precise == 184");
  CHECK_AND_ASSERT_MES(last_pos_block->cumulative_diff_precise == 20, false, "Incorrect condition failed: last_pos_block->cumulative_diff_precise == 20");
  //
  return true;
}

uint64_t before_hard_fork_1_cumulative_difficulty::get_hardfork_height()const
{
  return 10000; //just big number which is obviously bigger then test blockchain
}



bool inthe_middle_hard_fork_1_cumulative_difficulty::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{

  std::shared_ptr<block_extended_info> last_pow_block = c.get_blockchain_storage().get_last_block_of_type(false);
  std::shared_ptr<block_extended_info> last_pos_block = c.get_blockchain_storage().get_last_block_of_type(true);
  CHECK_AND_ASSERT_MES(last_pow_block->cumulative_diff_precise == 184, false, "Incorrect condition failed: last_pow_block->cumulative_diff_precise == 184");
  CHECK_AND_ASSERT_MES(last_pos_block->cumulative_diff_precise == 20, false, "Incorrect condition failed: last_pos_block->cumulative_diff_precise == 20");
  //
  return true;
}

uint64_t inthe_middle_hard_fork_1_cumulative_difficulty::get_hardfork_height()const
{
  return 15;
} 
bool after_hard_fork_1_cumulative_difficulty::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{

  std::shared_ptr<block_extended_info> last_pow_block = c.get_blockchain_storage().get_last_block_of_type(false);
  std::shared_ptr<block_extended_info> last_pos_block = c.get_blockchain_storage().get_last_block_of_type(true);
  CHECK_AND_ASSERT_MES(last_pow_block->cumulative_diff_precise == 172, false, "Incorrect condition failed: last_pow_block->cumulative_diff_precise == 184");
  CHECK_AND_ASSERT_MES(last_pos_block->cumulative_diff_precise == 199, false, "Incorrect condition failed: last_pos_block->cumulative_diff_precise == 20");
  //
  return true;
}

uint64_t after_hard_fork_1_cumulative_difficulty::get_hardfork_height()const
{
  return 12;
}

