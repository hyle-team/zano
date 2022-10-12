// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "zarcanum_test.h"
#include "wallet_test_core_proxy.h"

#include "random_helper.h"

#define  AMOUNT_TO_TRANSFER_ZARCANUM_BASIC (TESTS_DEFAULT_FEE*10)


using namespace currency;

//------------------------------------------------------------------------------
zarcanum_basic_test::zarcanum_basic_test()
{
  REGISTER_CALLBACK_METHOD(zarcanum_basic_test, configure_core);
  REGISTER_CALLBACK_METHOD(zarcanum_basic_test, c1);

  m_hardforks.set_hardfork_height(1, 1);
  m_hardforks.set_hardfork_height(2, 1);
  m_hardforks.set_hardfork_height(3, 1);
  m_hardforks.set_hardfork_height(4, 14);
}

bool zarcanum_basic_test::generate(std::vector<test_event_entry>& events) const
{
  m_accounts.resize(MINER_ACC_IDX+1);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX];
  miner_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  set_hard_fork_heights_to_generator(generator);
  //TODO: Need to make sure REWIND_BLOCKS_N and other coretests codebase are capable of following hardfork4 rules
  //in this test hardfork4 moment moved to runtime section
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  DO_CALLBACK(events, "c1");

  return true;
}

bool zarcanum_basic_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);

  account_base alice_acc;
  alice_acc.generate();
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, alice_acc);

  //pass over hardfork
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 2);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");


  miner_wlt->refresh();
  alice_wlt->refresh();



  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  //create transfer from pre-zarcanum inputs to post-zarcanum inputs
  uint64_t transfer_amount = AMOUNT_TO_TRANSFER_ZARCANUM_BASIC + TESTS_DEFAULT_FEE;
  miner_wlt->transfer(transfer_amount, alice_wlt->get_account().get_public_address());
  LOG_PRINT_MAGENTA("Legacy-2-zarcanum transaction sent to Alice: " << transfer_amount, LOG_LEVEL_0);
  miner_wlt->transfer(transfer_amount, alice_wlt->get_account().get_public_address());
  LOG_PRINT_MAGENTA("Legacy-2-zarcanum transaction sent to Alice: " << transfer_amount, LOG_LEVEL_0);
  miner_wlt->transfer(transfer_amount, alice_wlt->get_account().get_public_address());
  LOG_PRINT_MAGENTA("Legacy-2-zarcanum transaction sent to Alice: " << transfer_amount, LOG_LEVEL_0);
  miner_wlt->transfer(transfer_amount, alice_wlt->get_account().get_public_address());
  LOG_PRINT_MAGENTA("Legacy-2-zarcanum transaction sent to Alice: " << transfer_amount, LOG_LEVEL_0);


  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == 4, false, "Incorrect txs count in the pool");


  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  //miner_wlt->refresh();
  alice_wlt->refresh();
  
  //uint64_t unlocked = 0;
  //uint64_t balance = alice_wlt->balance(unlocked);
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", transfer_amount * 4, UINT64_MAX, transfer_amount * 4), false, "");

  account_base bob_acc;
  bob_acc.generate();
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, bob_acc);


  //create transfer from post-zarcanum inputs to post-zarcanum inputs
  uint64_t transfer_amount2 = AMOUNT_TO_TRANSFER_ZARCANUM_BASIC;
  alice_wlt->transfer(transfer_amount2, bob_acc.get_public_address());
  LOG_PRINT_MAGENTA("Zarcanum-2-zarcanum transaction sent from Alice  to Bob " << transfer_amount2, LOG_LEVEL_0);


  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");
  
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  bob_wlt->refresh();
  //balance = bob_wlt->balance(unlocked);
  //CHECK_AND_ASSERT_MES(unlocked == transfer_amount2, false, "wrong amount");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt, "Bob", transfer_amount2, UINT64_MAX, transfer_amount2), false, "");

  account_base staker_benefeciary_acc;
  staker_benefeciary_acc.generate();
  std::shared_ptr<tools::wallet2> staker_benefeciary_acc_wlt = init_playtime_test_wallet(events, c, staker_benefeciary_acc);

  account_base miner_benefeciary_acc;
  miner_benefeciary_acc.generate();
  std::shared_ptr<tools::wallet2> miner_benefeciary_acc_wlt = init_playtime_test_wallet(events, c, miner_benefeciary_acc);

  size_t pos_entries_count = 0;
  //do staking 
  for(size_t i = 0; i!= CURRENCY_MINED_MONEY_UNLOCK_WINDOW+4; i++)
  {
    if (!mine_next_pos_block_in_playtime_with_wallet(*alice_wlt.get(), staker_benefeciary_acc_wlt->get_account().get_public_address(), pos_entries_count))
    {
      CHECK_AND_ASSERT_MES(false, false, "Couldn't generate staking");
      return false;
    }   
    if (!mine_next_pow_block_in_playtime(miner_benefeciary_acc.get_public_address(), c))
    {
      CHECK_AND_ASSERT_MES(false, false, "Couldn't generate pow");
      return false;
    }
  }

  //attempt to spend staked and mined coinbase outs
  staker_benefeciary_acc_wlt->refresh();
  miner_benefeciary_acc_wlt->refresh();

  staker_benefeciary_acc_wlt->transfer(transfer_amount2, bob_acc.get_public_address());
  LOG_PRINT_MAGENTA("Zarcanum(pos-coinbase)-2-zarcanum transaction sent from Staker to Bob " << transfer_amount2, LOG_LEVEL_0);

  miner_benefeciary_acc_wlt->transfer(transfer_amount2, bob_acc.get_public_address());
  LOG_PRINT_MAGENTA("Zarcanum(pow-coinbase)-2-zarcanum transaction sent from Staker to Bob " << transfer_amount2, LOG_LEVEL_0);

  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == 2, false, "Incorrect txs count in the pool");

  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");


  bob_wlt->refresh();
  //balance = bob_wlt->balance(unlocked);
  //CHECK_AND_ASSERT_MES(unlocked == transfer_amount2*3, false, "wrong amount");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt, "Bob", transfer_amount2*3, UINT64_MAX, transfer_amount2*3), false, "");

  //try to make pre-zarcanum block after hardfork 4
  currency::core_runtime_config rc = alice_wlt->get_core_runtime_config();
  rc.hard_forks.set_hardfork_height(4, ZANO_HARDFORK_04_AFTER_HEIGHT);
  alice_wlt->set_core_runtime_config(rc);
  r = mine_next_pos_block_in_playtime_with_wallet(*alice_wlt.get(), staker_benefeciary_acc_wlt->get_account().get_public_address(), pos_entries_count);
  CHECK_AND_ASSERT_MES(!r, false, "Pre-zarcanum block accepted in post-zarcanum era");


  return true;
}
