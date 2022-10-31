// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "zarcanum_test.h"
#include "wallet_test_core_proxy.h"

#include "random_helper.h"
#include "tx_builder.h"


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
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc =   m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);
  //account_base& carol_acc = m_accounts[CAROL_ACC_IDX]; carol_acc.generate(); carol_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
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
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt   = init_playtime_test_wallet(events, c, BOB_ACC_IDX);

  // check passing over the hardfork
  CHECK_AND_ASSERT_MES(!c.get_blockchain_storage().is_hardfork_active(ZANO_HARDFORK_04_ZARCANUM), false, "ZANO_HARDFORK_04_ZARCANUM is active");
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 2);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_blockchain_storage().is_hardfork_active(ZANO_HARDFORK_04_ZARCANUM), false, "ZANO_HARDFORK_04_ZARCANUM is not active");

  miner_wlt->refresh();
  alice_wlt->refresh();


  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  //create transfer from pre-zarcanum inputs to post-zarcanum inputs
  uint64_t transfer_amount = AMOUNT_TO_TRANSFER_ZARCANUM_BASIC + TESTS_DEFAULT_FEE;
  const size_t batches_to_Alice_count = 4;
  for(size_t i = 0; i < batches_to_Alice_count; ++i)
  {
    miner_wlt->transfer(transfer_amount, alice_wlt->get_account().get_public_address());
    LOG_PRINT_MAGENTA("Legacy-2-zarcanum transaction sent to Alice: " << print_money_brief(transfer_amount), LOG_LEVEL_0);
  }

  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == batches_to_Alice_count, false, "Incorrect txs count in the pool");


  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  alice_wlt->refresh();
  
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", transfer_amount * batches_to_Alice_count, UINT64_MAX, transfer_amount * batches_to_Alice_count), false, "");

  //create transfer from post-zarcanum inputs to post-zarcanum inputs
  uint64_t transfer_amount2 = AMOUNT_TO_TRANSFER_ZARCANUM_BASIC;
  alice_wlt->transfer(transfer_amount2, m_accounts[BOB_ACC_IDX].get_public_address());
  LOG_PRINT_MAGENTA("Zarcanum-2-zarcanum transaction sent from Alice to Bob " << print_money_brief(transfer_amount2), LOG_LEVEL_0);


  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");
  
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  bob_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt, "Bob", transfer_amount2, UINT64_MAX, transfer_amount2), false, "");

  account_base staker_benefeciary_acc;
  staker_benefeciary_acc.generate();
  std::shared_ptr<tools::wallet2> staker_benefeciary_acc_wlt = init_playtime_test_wallet(events, c, staker_benefeciary_acc);

  account_base miner_benefeciary_acc;
  miner_benefeciary_acc.generate();
  std::shared_ptr<tools::wallet2> miner_benefeciary_acc_wlt = init_playtime_test_wallet(events, c, miner_benefeciary_acc);

  size_t pos_entries_count = 0;
  //do staking 
  for(size_t i = 0; i < batches_to_Alice_count - 1; i++)
  {
    alice_wlt->refresh();
    r = mine_next_pos_block_in_playtime_with_wallet(*alice_wlt.get(), staker_benefeciary_acc_wlt->get_account().get_public_address(), pos_entries_count);
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pos_block_in_playtime_with_wallet failed, pos_entries_count = " << pos_entries_count);

    r = mine_next_pow_block_in_playtime(miner_benefeciary_acc.get_public_address(), c);
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  }

  // make sure all mined coins in staker_benefeciary_acc_wlt and miner_benefeciary_acc_wlt are now spendable
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  //attempt to spend staked and mined coinbase outs
  staker_benefeciary_acc_wlt->refresh();
  miner_benefeciary_acc_wlt->refresh();

  uint64_t mined_amount = (batches_to_Alice_count - 1) * COIN;

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*staker_benefeciary_acc_wlt, "staker_benefeciary", mined_amount, mined_amount, mined_amount), false, "");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*miner_benefeciary_acc_wlt, "miner_benefeciary", mined_amount, mined_amount, mined_amount), false, "");


  staker_benefeciary_acc_wlt->transfer(transfer_amount2, bob_wlt->get_account().get_public_address());
  LOG_PRINT_MAGENTA("Zarcanum(pos-coinbase)-2-zarcanum transaction sent from Staker to Bob " << print_money_brief(transfer_amount2), LOG_LEVEL_0);

  miner_benefeciary_acc_wlt->transfer(transfer_amount2, bob_wlt->get_account().get_public_address());
  LOG_PRINT_MAGENTA("Zarcanum(pow-coinbase)-2-zarcanum transaction sent from Staker to Bob " << print_money_brief(transfer_amount2), LOG_LEVEL_0);

  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == 2, false, "Incorrect txs count in the pool");

  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");


  bob_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt, "Bob", transfer_amount2*3, UINT64_MAX, transfer_amount2*3), false, "");

  //try to make pre-zarcanum block after hardfork 4
  currency::core_runtime_config rc = alice_wlt->get_core_runtime_config();
  rc.hard_forks.set_hardfork_height(4, ZANO_HARDFORK_04_AFTER_HEIGHT);  // <-- TODO: this won't help to build pre-hardfork block,
                                                                        // because blocktemplate is created by the core.
                                                                        // (wallet2::build_minted_block will fail instead)
                                                                        // We need to use pos block builder or smth -- sowle
  alice_wlt->set_core_runtime_config(rc);
  r = mine_next_pos_block_in_playtime_with_wallet(*alice_wlt.get(), staker_benefeciary_acc_wlt->get_account().get_public_address(), pos_entries_count);
  CHECK_AND_ASSERT_MES(!r, false, "Pre-zarcanum block accepted in post-zarcanum era");


  return true;
}

//------------------------------------------------------------------------------

zarcanum_test_n_inputs_validation::zarcanum_test_n_inputs_validation()
{
  REGISTER_CALLBACK_METHOD(zarcanum_basic_test, configure_core);

  m_hardforks.set_hardfork_height(1, 1);
  m_hardforks.set_hardfork_height(2, 1);
  m_hardforks.set_hardfork_height(3, 1);
  m_hardforks.set_hardfork_height(4, 12);
}  


bool zarcanum_test_n_inputs_validation::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0r, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);
  destinations.resize(1);

  tx_builder builder;
  builder.step1_init(TRANSACTION_VERSION_PRE_HF4);
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);
  builder.step4_calc_hash();
  builder.step5_sign(sources);

//  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  MAKE_NEXT_BLOCK_TX1(events, blk_11, blk_0r, miner_account, builder.m_tx);
  REWIND_BLOCKS_N(events, blk_14, blk_11, miner_account, 4);


  sources.clear();
  destinations.clear();
  fill_tx_sources_and_destinations(events, blk_0r, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);
  destinations.resize(1);
  tx_builder builder2;
  //TODO: implement configuring for zarcanum type outputs
  builder2.step1_init(TRANSACTION_VERSION_POST_HF4);
  builder2.step2_fill_inputs(miner_account.get_keys(), sources);
  builder2.step3_fill_outputs(destinations);
  builder2.step4_calc_hash();
  builder2.step5_sign(sources);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder2.m_tx);
  REWIND_BLOCKS_N(events, blk_16, blk_14, miner_account, 2);

  return true;
}

//------------------------------------------------------------------------------

zarcanum_gen_time_balance::zarcanum_gen_time_balance()
{
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_04_ZARCANUM, 1);
}

bool zarcanum_gen_time_balance::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure post HF4 transactions with ZC inputs and outputs are handled properly by chaingen gen-time routines
  // (including balance check)

  bool r = false;

  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc =   m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);


  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core"); // necessary to set m_hardforks
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;

  //
  // tx_0: alice_amount from miner to Alice
  uint64_t alice_amount = MK_TEST_COINS(99);
  CHECK_AND_ASSERT_MES(fill_tx_sources_and_destinations(events, blk_0r, miner_acc, alice_acc, alice_amount, TESTS_DEFAULT_FEE, 0, sources, destinations), false, "");

  std::vector<extra_v> extra;
  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  crypto::secret_key tx_sec_key;
  r = construct_tx(miner_acc.get_keys(), sources, destinations, extra, empty_attachment, tx_0, get_tx_version_from_events(events), tx_sec_key, 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  ADD_CUSTOM_EVENT(events, tx_0);

  // add tx_0 to the block
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  // rewind and check unlocked balance
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, alice_amount, alice_amount, 0, 0, 0));

  // do a gen-time balance check
  CREATE_TEST_WALLET(alice_wlt, alice_acc, blk_0);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_1r, 2 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 4);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, alice_amount);
  
  //
  // tx_1: bob_amount, Alice -> Bob
  uint64_t bob_amount = MK_TEST_COINS(15);

  MAKE_TX(events, tx_1, alice_acc, bob_acc, bob_amount, blk_1r);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_1);

  // check Bob's balance in play time...
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(BOB_ACC_IDX, bob_amount, 0, 0, 0, 0));

  // ... and in gen time
  CREATE_TEST_WALLET(bob_wlt, bob_acc, blk_0);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, bob_wlt, blk_2, 2 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 5);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(bob_wlt, bob_amount);

  // try to construct tx with only one output (that is wrong for HF4)
  test_gentime_settings_restorer tgsr;
  test_gentime_settings tgs = test_generator::get_test_gentime_settings();
  tgs.split_strategy = tests_void_split_strategy; // amount won't be splitted by test_genertor::construct_tx()
  test_generator::set_test_gentime_settings(tgs);

  DO_CALLBACK(events, "mark_invalid_tx");
  // transfer all Bob's coins -- they should be put in one out
  MAKE_TX(events, tx_2_bad, bob_acc, alice_acc, bob_amount - TESTS_DEFAULT_FEE, blk_2);
  CHECK_AND_ASSERT_MES(tx_2_bad.vout.size() == 1, false, "tx_2_bad.vout.size() = " << tx_2_bad.vout.size());

  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_3_bad, blk_2, miner_acc, tx_2_bad);

  // now change split strategy and make a tx with more outputs
  tgs.split_strategy = tests_random_split_strategy;
  test_generator::set_test_gentime_settings(tgs);

  MAKE_TX(events, tx_2, bob_acc, alice_acc, bob_amount - TESTS_DEFAULT_FEE, blk_2);
  CHECK_AND_ASSERT_MES(tx_2.vout.size() != 1, false, "tx_2.vout.size() = " << tx_2.vout.size());
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_2);

  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_3, 2);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, alice_amount - 2 * TESTS_DEFAULT_FEE);

  REFRESH_TEST_WALLET_AT_GEN_TIME(events, bob_wlt, blk_3, 1);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(bob_wlt, 0);

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, alice_amount - 2 * TESTS_DEFAULT_FEE, 0, 0, 0, 0));

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(BOB_ACC_IDX, 0, 0, 0, 0, 0));

  return true;
}
