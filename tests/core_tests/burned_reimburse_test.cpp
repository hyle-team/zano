// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "offers_tests_common.h"
#include "burned_reimburse_test.h"

using namespace epee;
using namespace currency;


bunred_coins_reimburse_test::bunred_coins_reimburse_test()
{
  REGISTER_CALLBACK_METHOD(bunred_coins_reimburse_test, configure_core);
  REGISTER_CALLBACK_METHOD(bunred_coins_reimburse_test, check_reimbured);
}

#define COINS_TO_BURN MK_TEST_COINS(10)

bool bunred_coins_reimburse_test::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;
  std::list<currency::account_base> coin_stake_sources;

  GENERATE_ACCOUNT(preminer_account);
  GENERATE_ACCOUNT(miner_account);
  currency::account_base null_account = AUTO_VAL_INIT(null_account);


  coin_stake_sources.push_back(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, preminer_account, ts_start);
  DO_CALLBACK(events, "configure_core");

  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_11, blk_1, miner_account, 10);
  MAKE_NEXT_POS_BLOCK(events, blk_12, blk_11, miner_account, coin_stake_sources);
  MAKE_TX(events, tx_1, miner_account, null_account, COINS_TO_BURN, blk_12);
  MAKE_NEXT_BLOCK_TX1(events, blk_13, blk_12, miner_account, tx_1);
  DO_CALLBACK(events, "check_reimbured");

  return true;
}

bool bunred_coins_reimburse_test::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE; //four blocks
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH; //four blocks
  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

bool bunred_coins_reimburse_test::check_reimbured(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  uint64_t h = c.get_current_blockchain_size();
  currency::block_extended_info bei_before = AUTO_VAL_INIT(bei_before);
  c.get_blockchain_storage().get_block_extended_info_by_height(h - 2, bei_before);

  currency::block_extended_info bei = AUTO_VAL_INIT(bei);
  c.get_blockchain_storage().get_block_extended_info_by_height(h - 1, bei);


  const currency::transaction& tx_1 = boost::get<currency::transaction>(events[events.size() - 3]);
  uint64_t last_block_reward = (get_outs_money_amount(bei.bl.miner_tx) - get_tx_fee(tx_1));
  uint64_t reimbursed_coins = (bei_before.already_generated_coins + last_block_reward) - bei.already_generated_coins;
  CHECK_EQ(COINS_TO_BURN, reimbursed_coins);
  return true;
}

