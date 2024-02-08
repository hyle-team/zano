// Copyright (c) 2014-2022 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "zarcanum_test.h"
#include "wallet_test_core_proxy.h"

#include "random_helper.h"
#include "tx_builder.h"
#include "pos_block_builder.h"


using namespace currency;

//------------------------------------------------------------------------------
// helpers

void invalidate_CLSAG_GGXXG_sig(crypto::CLSAG_GGXXG_signature& sig)
{
  sig.c = 7;
}

void invalidate_bppe_sig(crypto::bppe_signature& sig)
{
  sig.delta_1.make_random();
}

void invalidate_pub_key(crypto::public_key& pk)
{
  pk.data[5] = 0x33;
}

bool invalidate_zarcanum_sig(size_t n, zarcanum_sig& sig)
{
  switch(n)
  {
  case 0:                                                         break;
  case 1:  invalidate_pub_key(sig.C);                             break;
  case 2:  sig.c.make_random();                                   break;
  case 3:  invalidate_CLSAG_GGXXG_sig(sig.clsag_ggxxg);           break;
  case 4:  invalidate_pub_key(sig.C_prime);                       break;
  case 5:  sig.d.make_random();                                   break;
  case 6:  invalidate_pub_key(sig.E);                             break;
  case 7:  invalidate_bppe_sig(sig.E_range_proof);                break;
  case 8:  invalidate_pub_key(sig.pseudo_out_amount_commitment);  break;
  case 9:  sig.y0.make_random();                                  break;
  case 10: sig.y1.make_random();                                  break;
  case 11: sig.y2.make_random();                                  break;
  case 12: sig.y3.make_random();                                  break;
  case 13: sig.y4.make_random();                                  break;
  default:                                                        return false;
  }
  return true;
}

bool make_next_pos_block(test_generator& generator, std::vector<test_event_entry>& events, const block& prev_block, const account_base& stake_acc,
  uint64_t amount_to_find, size_t nmix, const std::vector<transaction>& transactions, block& result)
{
  bool r = false;
  std::vector<tx_source_entry> sources;

  size_t height = get_block_height(prev_block) + 1;
  crypto::hash prev_id = get_block_hash(prev_block);
  r = fill_tx_sources(sources, events, prev_block, stake_acc.get_keys(), amount_to_find, nmix, true, true, false);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  CHECK_AND_ASSERT_MES(shuffle_source_entries(sources), false, "");
  auto it = std::max_element(sources.begin(), sources.end(), [&](const tx_source_entry& lhs, const tx_source_entry& rhs){ return lhs.amount < rhs.amount; });
  const tx_source_entry& se = *it;
  const tx_source_entry::output_entry& oe = se.outputs[se.real_output];

  crypto::key_image stake_output_key_image  {};
  currency::keypair ephemeral_keys          {};
  r = generate_key_image_helper(stake_acc.get_keys(), se.real_out_tx_key, se.real_output_in_tx_index, ephemeral_keys, stake_output_key_image);
  CHECK_AND_ASSERT_MES(r, false, "generate_key_image_helper failed");
  uint64_t stake_output_gindex = boost::get<uint64_t>(oe.out_reference);

  currency::wide_difficulty_type pos_diff{};
  crypto::hash last_pow_block_hash{}, last_pos_block_kernel_hash{};
  r = generator.get_params_for_next_pos_block(prev_id, pos_diff, last_pow_block_hash, last_pos_block_kernel_hash);
  CHECK_AND_ASSERT_MES(r, false, "get_params_for_next_pos_block failed");

  pos_block_builder pb;
  pb.step1_init_header(generator.get_hardforks(), height, prev_id);
  pb.step2_set_txs(transactions);

  pb.step3a(pos_diff, last_pow_block_hash, last_pos_block_kernel_hash);

  pb.step3b(se.amount, stake_output_key_image, se.real_out_tx_key, se.real_output_in_tx_index, se.real_out_amount_blinding_mask, stake_acc.get_keys().view_secret_key,
    stake_output_gindex, prev_block.timestamp, POS_SCAN_WINDOW, POS_SCAN_STEP);

  pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(prev_block), stake_acc.get_public_address());

  pb.step5_sign(se, stake_acc.get_keys());
  result = pb.m_block;
  return true;
}

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
  uint64_t transfer_amount = TESTS_DEFAULT_FEE*10 + TESTS_DEFAULT_FEE;
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

  //create transfer from post-zarcanum inputs to post-zarcanum inputs with mixins
  uint64_t transfer_amount2 = TESTS_DEFAULT_FEE*10;
  size_t nmix = 10;
  alice_wlt->transfer(transfer_amount2, nmix, m_accounts[BOB_ACC_IDX].get_public_address());
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

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*staker_benefeciary_acc_wlt, "staker_benefeciary", mined_amount, INVALID_BALANCE_VAL, mined_amount), false, "");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*miner_benefeciary_acc_wlt, "miner_benefeciary", mined_amount, INVALID_BALANCE_VAL, mined_amount), false, "");


  staker_benefeciary_acc_wlt->transfer(transfer_amount2, bob_wlt->get_account().get_public_address());
  LOG_PRINT_MAGENTA("Zarcanum(pos-coinbase)-2-zarcanum transaction sent from Staker to Bob " << print_money_brief(transfer_amount2), LOG_LEVEL_0);

  miner_benefeciary_acc_wlt->transfer(transfer_amount2, bob_wlt->get_account().get_public_address());
  LOG_PRINT_MAGENTA("Zarcanum(pow-coinbase)-2-zarcanum transaction sent from Staker to Bob " << print_money_brief(transfer_amount2), LOG_LEVEL_0);

  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == 2, false, "Incorrect txs count in the pool");

  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");


  bob_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt, "Bob", transfer_amount2*3, INVALID_BALANCE_VAL, transfer_amount2*3), false, "");

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
  DO_CALLBACK(events, "configure_core"); // necessary to set m_hardforks
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


  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
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
  MAKE_NEXT_BLOCK_TX1(events, blk_2__, blk_1r, miner_acc, tx_1);

  // check Bob's balance in play time...
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(BOB_ACC_IDX, bob_amount, 0, 0, 0, 0));

  REWIND_BLOCKS_N_WITH_TIME(events, blk_2, blk_2__, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // ... and in gen time
  CREATE_TEST_WALLET(bob_wlt, bob_acc, blk_0);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, bob_wlt, blk_2, 2 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 15);
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

  size_t nmix = 7;
  MAKE_TX_FEE_MIX(events, tx_2, bob_acc, alice_acc, bob_amount - TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, nmix, blk_2);
  CHECK_AND_ASSERT_MES(tx_2.vout.size() != 1, false, "tx_2.vout.size() = " << tx_2.vout.size());
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_2);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_4, blk_3, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);


  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_4, 22);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, alice_amount - 2 * TESTS_DEFAULT_FEE);

  REFRESH_TEST_WALLET_AT_GEN_TIME(events, bob_wlt, blk_4, 11);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(bob_wlt, 0);

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, alice_amount - 2 * TESTS_DEFAULT_FEE, alice_amount - 2 * TESTS_DEFAULT_FEE, 0, 0, 0));

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(BOB_ACC_IDX, 0, 0, 0, 0, 0));

  return true;
}

//------------------------------------------------------------------------------

zarcanum_pos_block_math::zarcanum_pos_block_math()
{
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_04_ZARCANUM, 0);
}

bool zarcanum_pos_block_math::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core"); // necessary to set m_hardforks
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_acc);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 10);

  //std::list<currency::account_base> miner_stake_sources( {miner_acc} );
  //MAKE_NEXT_POS_BLOCK(events, blk_2, blk_1r, miner_acc, miner_stake_sources);

  // blocks with an invalid zarcanum sig
  for(size_t i = 1; ; ++i)
  {
    block blk_1_pos_bad;
    CHECK_AND_ASSERT_MES(make_next_pos_block(generator, events, blk_1r, miner_acc, COIN, 10, std::vector<transaction>(), blk_1_pos_bad), false, "");
    LOG_PRINT_CYAN("i = " << i, LOG_LEVEL_0);
    if (!invalidate_zarcanum_sig(i, boost::get<zarcanum_sig>(blk_1_pos_bad.miner_tx.signatures[0])))
      break;
    generator.add_block_info(blk_1_pos_bad, std::list<transaction>{});
    DO_CALLBACK(events, "mark_invalid_block");
    ADD_CUSTOM_EVENT(events, blk_1_pos_bad);
  }

  // make a normal PoS block
  std::list<currency::account_base> miner_stake_sources( {miner_acc} );
  MAKE_NEXT_POS_BLOCK(events, blk_2, blk_1r, miner_acc, miner_stake_sources);
  // ... and a PoW block
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);

  // make a PoS block and than change its nonce, so its hash also changes
  // this block should fail
  MAKE_NEXT_POS_BLOCK(events, blk_4_bad, blk_3, miner_acc, miner_stake_sources);
  generator.remove_block_info(blk_4_bad);
  events.pop_back();
  blk_4_bad.nonce = 0xc0ffee; // this will change block's hash
  generator.add_block_info(blk_4_bad, std::list<transaction>{});
  DO_CALLBACK(events, "mark_invalid_block");
  ADD_CUSTOM_EVENT(events, blk_4_bad);

  // finally, make a normal block
  MAKE_NEXT_POS_BLOCK(events, blk_4, blk_3, miner_acc, miner_stake_sources);

  return true;
}

//------------------------------------------------------------------------------

zarcanum_txs_with_big_shuffled_decoy_set_shuffled::zarcanum_txs_with_big_shuffled_decoy_set_shuffled()
{
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_04_ZARCANUM, 24);
}

bool zarcanum_txs_with_big_shuffled_decoy_set_shuffled::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make few txs with a big decoy set, each time shuffling the sources (decoy set) before constructing a tx => to make sure real_output can be any.
  // Then do the same after HF4.

  bool r = false;

  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc =   m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // necessary to set m_hardforks

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  uint64_t alice_amount = CURRENCY_BLOCK_REWARD;
  MAKE_TX(events, tx_0_a, miner_acc, alice_acc, alice_amount, blk_0r);
  MAKE_TX(events, tx_0_b, miner_acc, alice_acc, alice_amount, blk_0r);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({ tx_0_a, tx_0_b }));

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // do a gen-time balance check
  CREATE_TEST_WALLET(alice_wlt, alice_acc, blk_0);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_1r, 2 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, 2 * alice_amount);

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, 2 * alice_amount, 2 * alice_amount, 0, 0, 0));

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;

  // tx_1: Alice -> miner with big decoy set
  size_t nmix = 10;
  CHECK_AND_ASSERT_MES(fill_tx_sources_and_destinations(events, blk_1r, alice_acc, miner_acc, alice_amount - TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, nmix, sources, destinations), false, "");

  // randomly source entries (real_output will be changed accordingly)
  CHECK_AND_ASSERT_MES(shuffle_source_entries(sources), false, "shuffle_source_entries failed");

  transaction tx_1_a{};
  r = construct_tx(alice_acc.get_keys(), sources, destinations, empty_attachment, tx_1_a, get_tx_version_from_events(events), 0 /* unlock time */);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  ADD_CUSTOM_EVENT(events, tx_1_a);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_1_a);

  // tx_2 (the same as tx_1): Alice -> miner, big decoy set, all coins are back now
  sources.clear();
  destinations.clear();
  nmix = 10;
  CHECK_AND_ASSERT_MES(fill_tx_sources_and_destinations(events, blk_2, alice_acc, miner_acc, alice_amount - TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, nmix, sources, destinations), false, "");
  CHECK_AND_ASSERT_MES(shuffle_source_entries(sources), false, "shuffle_source_entries failed");
  transaction tx_1_b{};
  r = construct_tx(alice_acc.get_keys(), sources, destinations, empty_attachment, tx_1_b, get_tx_version_from_events(events), 0 /* unlock time */);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  ADD_CUSTOM_EVENT(events, tx_1_b);
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_1_b);

  // make sure Alice has no coins left
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, 0, 0, 0, 0, 0));


  //
  // now do the same after HF4
  //


  // make sure the hardfork goes well
  DO_CALLBACK_PARAMS(events, "check_hardfork_inactive", static_cast<size_t>(ZANO_HARDFORK_04_ZARCANUM));
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_acc);
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_04_ZARCANUM));

  // tx_3_a, tx_3_b: miner -> Alice, alice_amount x 2
  MAKE_TX(events, tx_2_a, miner_acc, alice_acc, alice_amount, blk_4);
  MAKE_TX(events, tx_2_b, miner_acc, alice_acc, alice_amount, blk_4);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_5, blk_4, miner_acc, std::list<transaction>({ tx_2_a, tx_2_b }));

  REWIND_BLOCKS_N_WITH_TIME(events, blk_5r, blk_5, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // do a gen-time balance check
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_5r, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 4);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, 2 * alice_amount);

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, 2 * alice_amount, 2 * alice_amount, 0, 0, 0));

  // tx_3_a: Alice -> miner, alice_amount - TESTS_DEFAULT_FEE, big decoy set
  sources.clear();
  destinations.clear();
  nmix = 10;
  CHECK_AND_ASSERT_MES(fill_tx_sources_and_destinations(events, blk_5r, alice_acc, miner_acc, alice_amount - TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, nmix, sources, destinations), false, "");
  CHECK_AND_ASSERT_MES(shuffle_source_entries(sources), false, "shuffle_source_entries failed");
  transaction tx_3_a{};
  r = construct_tx(alice_acc.get_keys(), sources, destinations, empty_attachment, tx_3_a, get_tx_version_from_events(events), 0 /* unlock time */);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  ADD_CUSTOM_EVENT(events, tx_3_a);
  MAKE_NEXT_BLOCK_TX1(events, blk_6, blk_5r, miner_acc, tx_3_a);

  // tx_3_b: Alice -> miner, alice_amount - TESTS_DEFAULT_FEE, big decoy set
  sources.clear();
  destinations.clear();
  nmix = 10;
  CHECK_AND_ASSERT_MES(fill_tx_sources_and_destinations(events, blk_6, alice_acc, miner_acc, alice_amount - TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, nmix, sources, destinations), false, "");
  CHECK_AND_ASSERT_MES(shuffle_source_entries(sources), false, "shuffle_source_entries failed");
  transaction tx_3_b{};
  r = construct_tx(alice_acc.get_keys(), sources, destinations, empty_attachment, tx_3_b, get_tx_version_from_events(events), 0 /* unlock time */);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  ADD_CUSTOM_EVENT(events, tx_3_b);
  MAKE_NEXT_BLOCK_TX1(events, blk_7, blk_6, miner_acc, tx_3_b);

  // make sure Alice has no coins left
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, 0, 0, 0, 0, 0));

  return true;
}

//------------------------------------------------------------------------------

zarcanum_in_alt_chain::zarcanum_in_alt_chain()
{
  REGISTER_CALLBACK_METHOD(zarcanum_in_alt_chain, c1);
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_04_ZARCANUM, 23);
}

bool zarcanum_in_alt_chain::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc =   m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // necessary to set m_hardforks
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  uint64_t alice_amount = COIN * 100;
  MAKE_TX(events, tx_0, miner_acc, alice_acc, alice_amount, blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  std::list<currency::account_base> alice_stake_sources({ alice_acc });
  MAKE_NEXT_POS_BLOCK(events, blk_2, blk_1r, alice_acc, alice_stake_sources);

  DO_CALLBACK_PARAMS(events, "check_hardfork_inactive", static_cast<size_t>(ZANO_HARDFORK_04_ZARCANUM));
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_04_ZARCANUM));
  
  uint64_t bob_amount = COIN * 100;
  MAKE_TX(events, tx_1, miner_acc, bob_acc, bob_amount, blk_3);
  //make another tx just to create more decoys to fit hf4 rules of 16 decoys
  account_base carol_acc; carol_acc.generate();
  MAKE_TX(events, tx_1_1, miner_acc, carol_acc, bob_amount, blk_3);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_4, blk_3, miner_acc, std::list<transaction>({ tx_1, tx_1_1 }));

  //                                   HF4
  //                                    |
  //  0     10    11    21    22    23  | 24    34    35    36   <- blockchain height
  // (0 )..(0r)- (1 )..(1r)- !2 !- (3 )- (4 )..(4r)- (5 )        <- main chain
  //             tx_0                    tx_1    \   tx_2a
  //                                              \  tx_2b
  //                                                -!5a!- (6a)  <- alt chain

  REWIND_BLOCKS_N_WITH_TIME(events, blk_4r, blk_4, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // Bob: move all to miner
  MAKE_TX(events, tx_2a, bob_acc, miner_acc, bob_amount - TESTS_DEFAULT_FEE, blk_4r);
  // Miner: a little to Alice
  MAKE_TX(events, tx_2b, miner_acc, alice_acc, COIN, blk_4r);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_5, blk_4r, miner_acc, std::list<transaction>({ tx_2a, tx_2b }));

  // now in the main chain Bob has zero coins
  // check it in gen time ...
  CREATE_TEST_WALLET(bob_wlt, bob_acc, blk_0);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, bob_wlt, blk_5, 3 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 5);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(bob_wlt, 0);
  // ... and in play time
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(BOB_ACC_IDX, 0));

  // TODO: check PoS mining against already spent key image 

  //                                   HF4
  //                                    |
  //  0  .. 10    11    21    22    23  | 24 .. 34    35    36 .. 45    46    47   <- blockchain height
  // (0 )..(0r)- (1 )..(1r)- !2 !- (3 )- (4 )..(4r)- (5 )-                         <- alt chain
  //             tx_0                    tx_1    \   tx_2a
  //                                              \  tx_2b
  //                                               \                       c1
  //                                                -!5a!- (6a)- . . . (6ar)|(c1)  <- main chain

  std::list<currency::account_base> bob_stake_sources({ bob_acc });
  MAKE_NEXT_POS_BLOCK(events, blk_5a, blk_4r, bob_acc, bob_stake_sources); // NOTE: tx_2a and blk_5a spend the same Bob's output 
  MAKE_NEXT_BLOCK(events, blk_6a, blk_5a, miner_acc);

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_6a));

  REWIND_BLOCKS_N_WITH_TIME(events, blk_6ar, blk_6a, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK(events, "c1");

  //                                   HF4
  //                                    |
  //  0  .. 10    11    21    22    23  | 24 .. 34    35    36 .. 45    46    47   <- blockchain height
  // (0 )..(0r)- (1 )..(1r)- !2 !- (3 )- (4 )..(4r)- (5 )- . . . (5r)- !6 !- (7 )  <- main chain
  //             tx_0                    tx_1    \   tx_2a
  //                                              \  tx_2b
  //                                               \                        c1
  //                                                -!5a!- (6a)- . . . (6ar)|(c1)  <- alt chain

  REWIND_BLOCKS_N_WITH_TIME(events, blk_5r, blk_5, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  MAKE_NEXT_POS_BLOCK(events, blk_6, blk_5r, alice_acc, alice_stake_sources);
  MAKE_NEXT_BLOCK(events, blk_7, blk_6, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_8, blk_7, miner_acc);

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_8));

  return true;
}

bool zarcanum_in_alt_chain::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  bob_wlt->refresh();

  // tx_2a, tx_2b
  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == 2, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  uint64_t stub = 0;
  uint64_t alice_balance_before = alice_wlt->balance(stub);
  uint64_t bob_balance_before   = bob_wlt->balance(stub);

  uint64_t transfer_amount  = COIN;
  uint64_t transfer_fee     = TESTS_DEFAULT_FEE * 3;
  size_t nmix = 36;
  try {
    bob_wlt->transfer(transfer_amount, nmix, m_accounts[ALICE_ACC_IDX].get_public_address(), transfer_fee);
  }  
  catch (...)
  {
  }

  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == 3, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  // tx_2a can't be added as it's ki is spent
  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  alice_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", alice_balance_before + transfer_amount + COIN), false, ""); // COIN is from tx_2b

  bob_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt, "Bob", bob_balance_before - transfer_amount- transfer_fee), false, "");

  return true;
}

//------------------------------------------------------------------------------

zarcanum_block_with_txs::zarcanum_block_with_txs()
{
  REGISTER_CALLBACK_METHOD(zarcanum_block_with_txs, c1);
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_03,           0);
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_04_ZARCANUM, 23);
}

bool zarcanum_block_with_txs::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure Zarcanum PoS block can have txs and the sum of fees is correctly added to the block reward

  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc =   m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // necessary to set m_hardforks
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  //
  // before HF4
  //
  DO_CALLBACK_PARAMS(events, "check_hardfork_active",   static_cast<size_t>(ZANO_HARDFORK_03));
  DO_CALLBACK_PARAMS(events, "check_hardfork_inactive", static_cast<size_t>(ZANO_HARDFORK_04_ZARCANUM));

  // transfer few coins to Alice (one UTXO)
  m_alice_balance = MK_TEST_COINS(100);
  MAKE_TX(events, tx_0, miner_acc, alice_acc, m_alice_balance, blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  // rewind blocks and make sure Alice has received the coins
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, m_alice_balance, m_alice_balance, 0, 0, 0));

  // then miner sends few coins to Bob via a tx with a big fee amount
  std::list<currency::account_base> alice_stake_sources({ alice_acc });
  uint64_t fee = MK_TEST_COINS(123);
  MAKE_TX_FEE(events, tx_1, miner_acc, bob_acc, MK_TEST_COINS(1000), fee, blk_1r);

  // and Alice mines a PoS block with this tx -- so Alice is expected to receive the fee
  MAKE_NEXT_POS_BLOCK_TX1(events, blk_2, blk_1r, alice_acc, alice_stake_sources, tx_1);

  // make sure Alice received both block reward and the fee
  uint64_t mined_amount = COIN + fee;
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, m_alice_balance + mined_amount, 0, mined_amount, 0, 0));
  m_alice_balance += mined_amount;

  //
  // after HF4
  //
  MAKE_NEXT_BLOCK(events, blk_3_, blk_2, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_3, blk_3_, miner_acc);
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_04_ZARCANUM));

  MAKE_TX(events, tx_2, miner_acc, alice_acc, MK_TEST_COINS(200), blk_3);
  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_3, miner_acc, tx_2);
  m_alice_balance += MK_TEST_COINS(200);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_4r, blk_4, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW+5);
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, m_alice_balance, m_alice_balance, mined_amount, 0, 0));

  // then miner sends few coins to Bob via a tx with a big fee amount
  fee = MK_TEST_COINS(456);
  MAKE_TX_FEE(events, tx_3, miner_acc, bob_acc, MK_TEST_COINS(1000), fee, blk_4r);

  // and Alice mines a PoS block with this tx -- so Alice is expected to receive the fee
  MAKE_NEXT_POS_BLOCK_TX1(events, blk_5, blk_4r, alice_acc, alice_stake_sources, tx_3);

  // make sure Alice received block reward but not received the fee
  uint64_t mined_amount_2 = COIN /* + fee */;
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, m_alice_balance + mined_amount_2, UINT64_MAX, mined_amount + mined_amount_2, 0, 0));
  m_alice_balance += mined_amount_2;



  return true;
}

bool zarcanum_block_with_txs::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  return true;
}
