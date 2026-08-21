// Copyright (c) 2023-2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "chaingen.h"
#include "hard_fork_4.h"
#include "random_helper.h"

using namespace currency;

static void add_flags_to_all_destination_entries(const uint64_t flags, std::vector<currency::tx_destination_entry>& destinations)
{
  for(auto& de : destinations)
    de.flags |= flags;
}

//-------------------------------


hard_fork_4_consolidated_txs::hard_fork_4_consolidated_txs()
{
  REGISTER_CALLBACK_METHOD(hard_fork_4_consolidated_txs, c1);
}

bool hard_fork_4_consolidated_txs::generate(std::vector<test_event_entry>& events) const
{
  bool r = false;

  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // necessary for the test to be run by GENERATE_AND_PLAY_HF

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  m_post_hf4_zarcanum = get_hardforks().get_the_most_recent_hardfork_id_for_height(CURRENCY_MINED_MONEY_UNLOCK_WINDOW) >= ZANO_HARDFORK_04_ZARCANUM;

  uint64_t alice_amount = MK_TEST_COINS(50);
  MAKE_TX(events, tx_0a, miner_acc, alice_acc, alice_amount, blk_0r);
  // tx_0b is only needed for decoy outputs with amount = alice_amount (important only for pre-HF4)
  transaction tx_0b{};
  construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), miner_acc.get_public_address(), alice_amount * 10, 10, TESTS_DEFAULT_FEE, tx_0b);
  ADD_CUSTOM_EVENT(events, tx_0b);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({tx_0a, tx_0b}));

  size_t dhc = count_type_in_variant_container<tx_derivation_hint>(tx_0b.extra);
  CHECK_AND_ASSERT_MES(dhc == tx_0b.vout.size(), false, "unexpected derivation hints count: " << dhc);


  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // check Alice's balance
  std::shared_ptr<tools::wallet2> alice_wlt;
  r = generator.init_test_wallet(alice_acc, get_block_hash(blk_0), alice_wlt);
  CHECK_AND_ASSERT_MES(r, false, "init_test_wallet failed");
  r = generator.refresh_test_wallet(events, alice_wlt.get(), get_block_hash(blk_1r), 2 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "refresh_test_wallet failed");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "alice", alice_amount, 0, alice_amount, 0, 0), false, "");

  uint64_t miner_amount = MK_TEST_COINS(60);
  m_bob_amount = miner_amount + alice_amount - TX_DEFAULT_FEE;

  // Consolidated tx (TX_FLAG_SIGNATURE_MODE_SEPARATE).
  
  // this data will be transferred between stage 1 and 2
  transaction tx_1{};
  crypto::secret_key one_time_secret_key{};
  tx_generation_context gen_context{};

  // Part 1/2, miner's inputs
  {
    std::vector<tx_source_entry> sources;
    r = fill_tx_sources(sources, events, blk_1r, miner_acc.get_keys(), miner_amount, 10);
    CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
    uint64_t miner_change = get_sources_total_amount(sources) - miner_amount;

    std::vector<tx_destination_entry> destinations;
    if (miner_change != 0)
      destinations.push_back(tx_destination_entry(miner_change, miner_acc.get_public_address()));
    destinations.push_back(tx_destination_entry(m_bob_amount, bob_acc.get_public_address()));

    add_flags_to_all_destination_entries(tx_destination_entry_flags::tdef_explicit_native_asset_id, destinations);
    size_t tx_hardfork_id{};
    uint64_t tx_version = get_tx_version_and_harfork_id_from_events(events, tx_hardfork_id);
    r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_extra, empty_attachment, tx_1, tx_version, tx_hardfork_id, one_time_secret_key,
      0, 0, 0, true, TX_FLAG_SIGNATURE_MODE_SEPARATE, TX_DEFAULT_FEE, gen_context);
    CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

    dhc = count_type_in_variant_container<tx_derivation_hint>(tx_1.extra);
    CHECK_AND_ASSERT_MES(dhc == destinations.size(), false, "unexpected derivation hints count: " << dhc);

    // partially completed tx_1 shouldn't be accepted
     
    // now we added a balance check to tx_memory_pool::add_tx() for post-HF4 txs, so the behaviour is the same -- partially completed consolidated tx won't be added to the pool -- sowle
    // (subject to change in future)

    //if (m_post_hf4_zarcanum)
    //{
    //  ADD_CUSTOM_EVENT(events, tx_1);
    //  DO_CALLBACK(events, "mark_invalid_block");
    //  MAKE_NEXT_BLOCK_TX1(events, blk_2a, blk_1r, miner_acc, tx_1);
    //  DO_CALLBACK(events, "clear_tx_pool");
    //}
    //else
    //{
      DO_CALLBACK(events, "mark_invalid_tx");
      ADD_CUSTOM_EVENT(events, tx_1);
    //}
  }


  // Part 2/2, Alice's inputs
  {
    std::vector<tx_source_entry> sources;
    r = fill_tx_sources(sources, events, blk_1r, alice_acc.get_keys(), alice_amount, 10);
    CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
    CHECK_AND_ASSERT_MES(get_sources_total_amount(sources) == alice_amount, false, "no change for Alice is expected");
    sources.back().separately_signed_tx_complete = true;

    std::vector<tx_destination_entry> destinations;

    tx_comment comment{};
    comment.comment = "The key to any victory is the element of surprise.";

    size_t tx_hardfork_id{};
    uint64_t tx_version = get_tx_version_and_harfork_id_from_events(events, tx_hardfork_id);
    r = construct_tx(alice_acc.get_keys(), sources, destinations, {comment}, empty_attachment, tx_1, tx_version, tx_hardfork_id, one_time_secret_key,
      0, 0, 0, true, TX_FLAG_SIGNATURE_MODE_SEPARATE, 0 /* note zero fee here */, gen_context);
    CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

    size_t dhc_2 = count_type_in_variant_container<tx_derivation_hint>(tx_1.extra);
    CHECK_AND_ASSERT_MES(dhc_2 == dhc, false, "unexpected derivation hints count: " << dhc_2);

    ADD_CUSTOM_EVENT(events, tx_1);
  }
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_1);

  DO_CALLBACK(events, "c1");

  return true;
}

bool hard_fork_4_consolidated_txs::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  bob_wlt->refresh();

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", m_bob_amount, 0, 0, 0, 0), false, "");

  return true;
}



/*
hardfork_4_explicit_native_ids_in_outs::hardfork_4_explicit_native_ids_in_outs()
{
   REGISTER_CALLBACK_METHOD(hardfork_4_explicit_native_ids_in_outs, c1);

   m_hardforks.clear();
   m_hardforks.set_hardfork_height(ZANO_HARDFORK_04_ZARCANUM, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
}

bool hardfork_4_explicit_native_ids_in_outs::generate(std::vector<test_event_entry>& events) const
{
  bool r = false;

  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // necessary to set m_hardforks

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK_PARAMS(events, "check_hardfork_inactive", static_cast<size_t>(ZANO_HARDFORK_04_ZARCANUM));

  // tx_0: miner -> Alice
  // make tx_0 before HF4, so Alice will have only bare outs
  m_alice_initial_balance = MK_TEST_COINS(1000);
  MAKE_TX(events, tx_0, miner_acc, alice_acc, m_alice_initial_balance, blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  // make sure HF4 has been activated
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_04_ZARCANUM));

  // rewind blocks
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // check Alice's balance and make sure she cannot deploy an asset
  DO_CALLBACK(events, "c1_alice_cannot_deploy_asset");

  // tx_1: Alice -> Alice (all coins) : this will convert all Alice outputs to ZC outs
  MAKE_TX(events, tx_1, alice_acc, alice_acc, m_alice_initial_balance - TESTS_DEFAULT_FEE, blk_1r); 



  return true;
}

bool hardfork_4_explicit_native_ids_in_outs::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  return true;
}
*/

//------------------------------------------------------------------------------

hardfork_4_wallet_transfer_with_mandatory_mixins::hardfork_4_wallet_transfer_with_mandatory_mixins()
{
  REGISTER_CALLBACK_METHOD(hardfork_4_wallet_transfer_with_mandatory_mixins, c1);
  REGISTER_CALLBACK_METHOD(hardfork_4_wallet_transfer_with_mandatory_mixins, configure_core);
}

bool hardfork_4_wallet_transfer_with_mandatory_mixins::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  test_chain_unit_enchanced::configure_core(c, ev_index, events); // call default
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.hf4_minimum_mixins = CURRENCY_HF4_MANDATORY_DECOY_SET_SIZE;
  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

bool hardfork_4_wallet_transfer_with_mandatory_mixins::generate(std::vector<test_event_entry>& events) const
{
  /* Test outline: make sure that after HF4 a normal transfer with CURRENCY_HF4_MANDATORY_DECOY_SET_SIZE decoys goes normal.
  * (It should also work prior to HF4.)
  */

  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // necessary for the test to be run by GENERATE_AND_PLAY_HF

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  MAKE_TX(events, tx_1, miner_acc, alice_acc, MK_TEST_COINS(10), blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  DO_CALLBACK(events, "c1");

  return true;
}

bool hardfork_4_wallet_transfer_with_mandatory_mixins::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", MK_TEST_COINS(10), 0, 0, 0, 0), false, "");

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  alice_wlt->refresh();

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", MK_TEST_COINS(10), 0, MK_TEST_COINS(10), 0, 0), false, "");

  alice_wlt->transfer(MK_TEST_COINS(9), m_accounts[BOB_ACC_IDX].get_public_address());

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  bob_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", MK_TEST_COINS(9), 0, 0, 0, 0), false, "");

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  alice_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", 0, 0, 0, 0, 0), false, "");
  bob_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", MK_TEST_COINS(9), 0, MK_TEST_COINS(9), 0, 0), false, "");

  return true;
}

//------------------------------------------------------------------------------

hardfork_4_wallet_sweep_bare_outs::hardfork_4_wallet_sweep_bare_outs()
{
  REGISTER_CALLBACK_METHOD(hardfork_4_wallet_sweep_bare_outs, c1);

  m_hardforks.set_hardfork_height(ZANO_HARDFORK_04_ZARCANUM, 10);
}

bool hardfork_4_wallet_sweep_bare_outs::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure wallet2::sweep_bare_outs works well even if there's not enough outputs to mix

  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);

  // rebuild genesis miner tx
  std::vector<tx_destination_entry> destinations;
  destinations.emplace_back(MK_TEST_COINS(23), alice_acc.get_public_address());
  destinations.emplace_back(MK_TEST_COINS(23), bob_acc.get_public_address());
  destinations.emplace_back(MK_TEST_COINS(55), bob_acc.get_public_address());   // this output is unique and doesn't have decoys
  for (size_t i = 0; i < 10; ++i)
    destinations.emplace_back(MK_TEST_COINS(23), miner_acc.get_public_address()); // decoys (later Alice will spend her output using mixins)
  destinations.emplace_back(COIN, miner_acc.get_public_address()); // leftover amount will be also send to the last destination
  CHECK_AND_ASSERT_MES(replace_coinbase_in_genesis_block(destinations, generator, events, blk_0), false, "");

  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_04_ZARCANUM));

  DO_CALLBACK(events, "c1");

  return true;
}

bool hardfork_4_wallet_sweep_bare_outs::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", MK_TEST_COINS(23), UINT64_MAX, MK_TEST_COINS(23), 0, 0), false, "");

  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  bob_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", MK_TEST_COINS(23 + 55), UINT64_MAX, MK_TEST_COINS(23 + 55), 0, 0), false, "");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  // 1. Try to sweep bare out for Alice (enough decoys to mix with)
  std::vector<tools::wallet2::batch_of_bare_unspent_outs> tids_grouped_by_txs;
  CHECK_AND_ASSERT_MES(alice_wlt->get_bare_unspent_outputs_stats(tids_grouped_by_txs), false, "");
  size_t total_txs_sent = 0;
  uint64_t total_amount_sent = 0;
  uint64_t total_fee_spent = 0;
  uint64_t total_bare_outs_sent = 0;
  CHECK_AND_ASSERT_MES(alice_wlt->sweep_bare_unspent_outputs(m_accounts[ALICE_ACC_IDX].get_public_address(), tids_grouped_by_txs, total_txs_sent, total_amount_sent, total_fee_spent, total_bare_outs_sent), false, "");

  CHECK_V_EQ_EXPECTED_AND_ASSERT(total_txs_sent, 1);
  CHECK_V_EQ_EXPECTED_AND_ASSERT(total_amount_sent, MK_TEST_COINS(23));
  CHECK_V_EQ_EXPECTED_AND_ASSERT(total_fee_spent, TESTS_DEFAULT_FEE);
  CHECK_V_EQ_EXPECTED_AND_ASSERT(total_bare_outs_sent, 1);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  std::list<transaction> txs;
  c.get_pool_transactions(txs);
  auto& tx = txs.back();
  CHECK_AND_ASSERT_MES(check_mixin_value_for_each_input(CURRENCY_DEFAULT_DECOY_SET_SIZE, get_transaction_hash(tx), c), false, "");

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  alice_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", MK_TEST_COINS(23) - TESTS_DEFAULT_FEE, UINT64_MAX, MK_TEST_COINS(23) - TESTS_DEFAULT_FEE, 0, 0), false, "");

  
  // 2. Try to sweep bare out for Bob (not enough decoys to mix with)
  tids_grouped_by_txs.clear();
  CHECK_AND_ASSERT_MES(bob_wlt->get_bare_unspent_outputs_stats(tids_grouped_by_txs), false, "");
  CHECK_AND_ASSERT_MES(bob_wlt->sweep_bare_unspent_outputs(m_accounts[BOB_ACC_IDX].get_public_address(), tids_grouped_by_txs, total_txs_sent, total_amount_sent, total_fee_spent, total_bare_outs_sent), false, "");

  CHECK_V_EQ_EXPECTED_AND_ASSERT(total_txs_sent, 1);
  CHECK_V_EQ_EXPECTED_AND_ASSERT(total_amount_sent, MK_TEST_COINS(23 + 55));
  CHECK_V_EQ_EXPECTED_AND_ASSERT(total_fee_spent, TESTS_DEFAULT_FEE);
  CHECK_V_EQ_EXPECTED_AND_ASSERT(total_bare_outs_sent, 2);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  txs.clear();
  c.get_pool_transactions(txs);
  auto& tx2 = txs.back();
  CHECK_V_EQ_EXPECTED_AND_ASSERT(tx2.vin.size(), 2);

  const txin_to_key &input_with_enough_decoys     = boost::get<txin_to_key>(tx2.vin[0]).amount == MK_TEST_COINS(23) ? boost::get<txin_to_key>(tx2.vin[0]) : boost::get<txin_to_key>(tx2.vin[1]);
  const txin_to_key &input_with_not_enough_decoys = boost::get<txin_to_key>(tx2.vin[0]).amount == MK_TEST_COINS(23) ? boost::get<txin_to_key>(tx2.vin[1]) : boost::get<txin_to_key>(tx2.vin[0]);

  CHECK_V_EQ_EXPECTED_AND_ASSERT(input_with_enough_decoys.key_offsets.size(), CURRENCY_DEFAULT_DECOY_SET_SIZE + 1); 
  CHECK_V_EQ_EXPECTED_AND_ASSERT(input_with_not_enough_decoys.key_offsets.size(), 1); 

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  bob_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", MK_TEST_COINS(23 + 55) - TESTS_DEFAULT_FEE, UINT64_MAX, MK_TEST_COINS(23 + 55) - TESTS_DEFAULT_FEE, 0, 0), false, "");

  return true;
}

//------------------------------------------------------------------------------

hardfork_4_pop_tx_from_global_index::hardfork_4_pop_tx_from_global_index()
{
  REGISTER_CALLBACK_METHOD(hardfork_4_pop_tx_from_global_index, c1);
}

bool hardfork_4_pop_tx_from_global_index::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure that pop_transaction_from_global_index works for tx_out_zarcanum as well (m_db_outputs is consistent after pop_transaction_from_global_index() call)

  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_04_ZARCANUM));

  MAKE_NEXT_BLOCK(events, blk_1a, blk_0r, miner_acc);                      // blk_1a will be the alt chain
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_1a)); // make sure now it's the main chain

  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, miner_acc);

  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);                        // this should trigger chain switching
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_2));  // make sure it did

  // during switching to the alternative chain pop_block_from_blockchain() -> ... -> pop_transaction_from_global_index() will be called
  // but abort_transaction() will not, meaning m_db_outputs will be in incorrect state, if pop_transaction_from_global_index() hasn't properly pop all outs
  // this will be checked later in c1

  DO_CALLBACK(events, "c1");
  return true;
}

bool hardfork_4_pop_tx_from_global_index::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  auto& bcs = c.get_blockchain_storage();

  //currency::outs_index_stat outs_stat{};
  //bcs.get_outs_index_stat(outs_stat); // 24 - bad, 22 - good
  
  COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES_BY_AMOUNT::response res;
  COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES_BY_AMOUNT::request req;
  req.amount = 0;
  req.i = 22;
  CHECK_AND_ASSERT_MES(!bcs.get_global_index_details(req, res), false, "gindex 22 exists which is unexpected");
  req.i = 21;
  CHECK_AND_ASSERT_MES(bcs.get_global_index_details(req, res), false, "gindex 21 does not exist which is unexpected");

  return true;
}

//------------------------------------------------------------------------------

hardfork_4_pos_decoy_transition::core_proxy::core_proxy(std::shared_ptr<tools::i_core_proxy> delegate)
  : m_delegate(std::move(delegate))
{}

bool hardfork_4_pos_decoy_transition::core_proxy::call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request& req, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response& rsp)
{
  m_called = true;
  m_request = req;
  const bool result = m_delegate->call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4(req, rsp);
  m_response = rsp;
  return result;
}

size_t hardfork_4_pos_decoy_transition::count_response_outputs(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response& response)
{
  size_t result = 0;
  for (const auto& batch : response.blocks_batches)
    for (const auto& block : batch.blocks)
      result += block.outs.size();
  return result;
}


hardfork_4_pos_decoy_transition::hardfork_4_pos_decoy_transition()
{
  REGISTER_CALLBACK_METHOD(hardfork_4_pos_decoy_transition, configure_core);
  REGISTER_CALLBACK_METHOD(hardfork_4_pos_decoy_transition, c1);

  m_hardforks.clear();
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_01, 0);
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_02, 0);
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_03, 0);
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_04_ZARCANUM, HF4_TRANSITION_ACTIVE_AFTER);
}

bool hardfork_4_pos_decoy_transition::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  CHECK_AND_ASSERT_MES(test_chain_unit_enchanced::configure_core(c, ev_index, events), false, "default configure_core failed");

  currency::core_runtime_config config = c.get_blockchain_storage().get_core_runtime_config();
  config.min_coinstake_age = CURRENCY_HF4_MANDATORY_MIN_COINAGE;
  config.pos_minimum_heigh = 0;
  config.hf4_minimum_mixins = CURRENCY_HF4_MANDATORY_DECOY_SET_SIZE;
  config.hard_forks = m_hardforks;
  c.get_blockchain_storage().set_core_runtime_config(config);

  LOG_PRINT_MAGENTA("[HF4 TRANSITION] config: HF4 first block=" << HF4_TRANSITION_FIRST_HEIGHT
    << ", maturity=" << config.min_coinstake_age
    << ", mandatory decoys=" << config.hf4_minimum_mixins
    << ", ring size=" << config.hf4_minimum_mixins + 1, LOG_LEVEL_0);
  return true;
}

bool hardfork_4_pos_decoy_transition::generate(std::vector<test_event_entry>& events) const
{
  // [0, 1, 2, ... 20 - HF3 - 21, 22, ... 37 - HF4(pow) - 38] 38h it is first possible HF4 PoS
  // top 30 -> one mature HF4 block; top 36 -> 14 outputs/13 decoys, ring fails;
  // top 37 -> 16 outputs/15 decoys, PoS and regular transaction rings succeed

  const uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_hf3_tip, blk_0, miner_acc, HF4_TRANSITION_ACTIVE_AFTER);
  CHECK_AND_ASSERT_MES(get_block_height(blk_hf3_tip) == HF4_TRANSITION_ACTIVE_AFTER, false, "unexpected HF3 tip height");
  DO_CALLBACK(events, "c1");
  return true;
}

bool hardfork_4_pos_decoy_transition::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  struct transition_snapshot
  {
    uint64_t top = 0;
    uint64_t mature_max = 0;
    size_t mature_heights = 0;
    size_t eligible_zc_outputs = 0;
    size_t wallet_pos_entries = 0;
  };

  auto& bcs = c.get_blockchain_storage();
  CHECK_AND_ASSERT_EQ(bcs.get_top_block_height(), HF4_TRANSITION_ACTIVE_AFTER);
  CHECK_AND_ASSERT_MES(!bcs.get_core_runtime_config().is_hardfork_active_for_height(ZANO_HARDFORK_04_ZARCANUM, HF4_TRANSITION_ACTIVE_AFTER), false, "HF4 is active one block too early");
  CHECK_AND_ASSERT_MES(bcs.get_core_runtime_config().is_hardfork_active_for_height(ZANO_HARDFORK_04_ZARCANUM, HF4_TRANSITION_FIRST_HEIGHT), false, "HF4 is not active at its first block");

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  miner_wlt->refresh();

  auto read_and_log_snapshot = [&](const char* stage, transition_snapshot& snapshot) -> bool
  {
    snapshot.top = bcs.get_top_block_height();
    const uint64_t chain_size = bcs.get_current_blockchain_size();
    snapshot.mature_max = chain_size >= CURRENCY_HF4_MANDATORY_MIN_COINAGE ? chain_size - CURRENCY_HF4_MANDATORY_MIN_COINAGE : 0;

    if (snapshot.mature_max >= HF4_TRANSITION_FIRST_HEIGHT)
    {
      snapshot.mature_heights = static_cast<size_t>(snapshot.mature_max - HF4_TRANSITION_FIRST_HEIGHT + 1);
      for (uint64_t height = HF4_TRANSITION_FIRST_HEIGHT; height <= snapshot.mature_max; ++height)
      {
        std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry> outputs;
        CHECK_AND_ASSERT_MES(bcs.collect_all_outs_in_block(0, height, outputs), false, "cannot collect ZC outputs at height " << height);
        snapshot.eligible_zc_outputs += outputs.size();
      }
    }

    snapshot.wallet_pos_entries = miner_wlt->get_pos_entries_count();
    const std::string mature_range = snapshot.mature_heights ? std::to_string(HF4_TRANSITION_FIRST_HEIGHT) + ".." + std::to_string(snapshot.mature_max) : "empty";
    LOG_PRINT_MAGENTA("[HF4 TRANSITION] " << stage
      << ": top=" << snapshot.top
      << ", next=" << snapshot.top + 1
      << ", mature range=" << mature_range
      << ", mature heights=" << snapshot.mature_heights
      << ", eligible ZC outputs=" << snapshot.eligible_zc_outputs
      << ", wallet PoS entries=" << snapshot.wallet_pos_entries, LOG_LEVEL_0);
    return true;
  };

  auto mine_pow_until = [&](uint64_t target_top) -> bool
  {
    while (bcs.get_top_block_height() < target_top)
    {
      CHECK_AND_ASSERT_MES(mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c), false, "failed to mine transition PoW block");

      block top_block{};
      CHECK_AND_ASSERT_MES(bcs.get_top_block(top_block), false, "cannot get transition PoW block");
      CHECK_AND_ASSERT_MES(!is_pos_block(top_block), false, "unexpected PoS block in PoW-only transition prefix");
      const size_t zc_coinbase_outputs = std::count_if(top_block.miner_tx.vout.begin(), top_block.miner_tx.vout.end(),
        [](const tx_out_v& output) { return output.type() == typeid(tx_out_zarcanum); });
      CHECK_AND_ASSERT_MES(zc_coinbase_outputs == 2, false, "transition PoW block " << get_block_height(top_block) << " has " << zc_coinbase_outputs << " ZC coinbase outputs instead of 2");

      miner_wlt->refresh();
      transition_snapshot snapshot{};
      CHECK_AND_ASSERT_MES(read_and_log_snapshot("after PoW", snapshot), false, "cannot read transition snapshot");
      const size_t expected_mature_heights = snapshot.top >= HF4_TRANSITION_FIRST_HEIGHT + CURRENCY_HF4_MANDATORY_MIN_COINAGE - 1
        ? static_cast<size_t>(snapshot.top - (HF4_TRANSITION_FIRST_HEIGHT + CURRENCY_HF4_MANDATORY_MIN_COINAGE - 2)) : 0;
      CHECK_AND_ASSERT_EQ(snapshot.mature_heights, expected_mature_heights);
      CHECK_AND_ASSERT_EQ(snapshot.eligible_zc_outputs, expected_mature_heights * 2);
      CHECK_AND_ASSERT_EQ(snapshot.wallet_pos_entries, expected_mature_heights * 2);
    }
    return true;
  };

  transition_snapshot initial_snapshot{};
  CHECK_AND_ASSERT_MES(read_and_log_snapshot("HF3 tip", initial_snapshot), false, "cannot read initial transition snapshot");
  CHECK_AND_ASSERT_EQ(initial_snapshot.eligible_zc_outputs, 0);
  CHECK_AND_ASSERT_EQ(initial_snapshot.wallet_pos_entries, 0);

  // at top 30 - we have 10 confirmations for 21 height
  CHECK_AND_ASSERT_MES(mine_pow_until(30), false, "cannot reach the exact maturity boundary");

  tools::transfer_details stake_td{};
  bool stake_found = false;
  miner_wlt->enumerate_transfers([&](uint64_t, const tools::transfer_details& td) -> bool
  {
    if (td.is_zc() && td.is_native_coin() && td.m_ptx_wallet_info->m_block_height == HF4_TRANSITION_FIRST_HEIGHT)
    {
      stake_td = td;
      stake_found = true;
      return false;
    }
    return true;
  });
  CHECK_AND_ASSERT_MES(stake_found, false, "cannot find the first HF4 ZC output in miner wallet");
  CHECK_AND_ASSERT_MES(stake_td.output().type() == typeid(tx_out_zarcanum), false, "selected stake is not a ZC output");
  const tx_out_zarcanum& stake_out = boost::get<tx_out_zarcanum>(stake_td.output());

  auto check_ring_attempt = [&](size_t expected_height_count, size_t expected_response_outputs, size_t expected_picked_decoys, bool expect_success) -> bool
  {
    const uint64_t current_top = bcs.get_top_block_height();
    auto trace_proxy = std::make_shared<core_proxy>(m_core_proxy);
    miner_wlt->set_core_proxy(trace_proxy);

    txin_zc_input stake_input{};
    std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry> decoys;
    std::vector<crypto::CLSAG_GGXXG_input_ref_t> ring;
    uint64_t secret_index = 0;
    bool success = false;
    std::string error;
    try
    {
      success = miner_wlt->prepare_pos_zc_input_and_ring(stake_td, stake_out, stake_input, decoys, ring, secret_index);
    }
    catch (const std::exception& e)
    {
      error = e.what();
    }
    catch (...)
    {
      error = "non-std exception";
    }
    miner_wlt->set_core_proxy(m_core_proxy);

    CHECK_AND_ASSERT_MES(trace_proxy->m_called, false, "wallet did not call getrandomouts4");
    CHECK_AND_ASSERT_EQ(trace_proxy->m_request.look_up_strategy, std::string(LOOK_UP_STRATEGY_POS_COINBASE));
    CHECK_AND_ASSERT_EQ(trace_proxy->m_request.height_upper_limit, current_top);
    CHECK_AND_ASSERT_EQ(trace_proxy->m_request.batches.size(), 1);
    CHECK_AND_ASSERT_EQ(trace_proxy->m_request.batches[0].input_amount, 0);
    CHECK_AND_ASSERT_EQ(trace_proxy->m_request.batches[0].heights.size(), expected_height_count);
    for (size_t i = 0; i < expected_height_count; ++i)
      CHECK_AND_ASSERT_EQ(trace_proxy->m_request.batches[0].heights[i], HF4_TRANSITION_FIRST_HEIGHT + i);

    CHECK_AND_ASSERT_EQ(trace_proxy->m_response.blocks_batches.size(), 1);
    CHECK_AND_ASSERT_EQ(trace_proxy->m_response.blocks_batches[0].blocks.size(), expected_height_count);
    CHECK_AND_ASSERT_EQ(count_response_outputs(trace_proxy->m_response), expected_response_outputs);
    for (const auto& returned_block : trace_proxy->m_response.blocks_batches[0].blocks)
    {
      CHECK_AND_ASSERT_MES(returned_block.block_height >= HF4_TRANSITION_FIRST_HEIGHT, false, "getrandomouts4 returned a pre-HF4 block");
      CHECK_AND_ASSERT_MES(returned_block.block_height <= HF4_TRANSITION_FIRST_HEIGHT + expected_height_count - 1, false, "getrandomouts4 returned an immature block");
      for (const auto& output : returned_block.outs)
      {
        CHECK_AND_ASSERT_MES(output.flags & RANDOM_OUTPUTS_FOR_AMOUNTS_FLAGS_COINBASE, false, "fallback returned a non-coinbase output");
        CHECK_AND_ASSERT_MES(!(output.flags & RANDOM_OUTPUTS_FOR_AMOUNTS_FLAGS_POS_COINBASE), false, "expected PoW coinbase output during transition");
      }
    }

    LOG_PRINT_MAGENTA("[HF4 TRANSITION] getrandomouts4 for next height " << current_top + 1
      << ": requested unique heights=" << expected_height_count
      << " (" << HF4_TRANSITION_FIRST_HEIGHT << ".." << HF4_TRANSITION_FIRST_HEIGHT + expected_height_count - 1 << ")"
      << ", real height=" << stake_td.m_ptx_wallet_info->m_block_height
      << ", response outputs=" << expected_response_outputs
      << ", usable decoys after real exclusion=" << expected_picked_decoys, LOG_LEVEL_0);
    CHECK_AND_ASSERT_EQ(decoys.size(), expected_picked_decoys + (expect_success ? 1 : 0)); // successful storage also contains the real output
    CHECK_AND_ASSERT_EQ(success, expect_success);
    CHECK_AND_ASSERT_EQ(error.empty(), expect_success);
    if (expect_success)
    {
      CHECK_AND_ASSERT_EQ(ring.size(), CURRENCY_HF4_MANDATORY_DECOY_SET_SIZE + 1);
      CHECK_AND_ASSERT_EQ(stake_input.key_offsets.size(), CURRENCY_HF4_MANDATORY_DECOY_SET_SIZE + 1);
      CHECK_AND_ASSERT_MES(secret_index < ring.size(), false, "real stake index is outside the ring");
    }
    else
    {
      CHECK_AND_ASSERT_MES(!error.empty(), false, "insufficient decoy pool was accepted");
      LOG_PRINT_MAGENTA("[HF4 TRANSITION] expected ring failure at next height " << current_top + 1
        << ": response outputs=" << expected_response_outputs
        << ", real excluded -> decoys=" << expected_picked_decoys
        << ", error=" << error, LOG_LEVEL_0);
    }
    return true;
  };

  CHECK_AND_ASSERT_MES(check_ring_attempt(1, 2, 1, false), false, "one-height maturity check failed");

  // candidate block 37 sees heights 21..27: 7 PoW blocks, 14 outputs, and only 13 decoys after excluding the real stake
  CHECK_AND_ASSERT_MES(mine_pow_until(HF4_TRANSITION_LAST_INSUFFICIENT_TOP), false, "cannot reach the 14-output boundary");
  CHECK_AND_ASSERT_MES(check_ring_attempt(7, 14, 13, false), false, "14-output boundary check failed");

  // one more PoW block makes height 28 mature for candidate block 38, 8 blocks * 2 outs = 16 ring members, including the real
  CHECK_AND_ASSERT_MES(mine_pow_until(HF4_TRANSITION_FIRST_SUFFICIENT_TOP), false, "cannot reach the 16-output boundary");
  CHECK_AND_ASSERT_MES(check_ring_attempt(8, 16, 15, true), false, "16-output boundary check failed");

  const uint64_t top_before_pos = bcs.get_top_block_height();
  CHECK_AND_ASSERT_MES(miner_wlt->try_mint_pos(m_accounts[MINER_ACC_IDX].get_public_address()), false, "first structurally possible HF4 PoS block was not minted");
  CHECK_AND_ASSERT_EQ(bcs.get_top_block_height(), top_before_pos + 1);
  block pos_block{};
  CHECK_AND_ASSERT_MES(bcs.get_top_block(pos_block), false, "cannot get minted PoS block");
  CHECK_AND_ASSERT_MES(is_pos_block(pos_block), false, "expected an HF4 PoS block");
  CHECK_AND_ASSERT_MES(pos_block.miner_tx.vin.size() > 1 && pos_block.miner_tx.vin[1].type() == typeid(txin_zc_input), false, "PoS block has no Zarcanum stake input");
  CHECK_AND_ASSERT_EQ(boost::get<txin_zc_input>(pos_block.miner_tx.vin[1]).key_offsets.size(), CURRENCY_HF4_MANDATORY_DECOY_SET_SIZE + 1);
  LOG_PRINT_MAGENTA("[HF4 TRANSITION] PoS success: height=" << get_block_height(pos_block)
    << ", ring size=" << boost::get<txin_zc_input>(pos_block.miner_tx.vin[1]).key_offsets.size(), LOG_LEVEL_0);

  // The regular transaction path uses LOOK_UP_STRATEGY_REGULAR_TX first and
  // must also be able to fall back to the mature PoW coinbase outputs.
  miner_wlt->refresh();
  transaction regular_tx{};
  miner_wlt->transfer(TESTS_DEFAULT_FEE, m_accounts[ALICE_ACC_IDX].get_public_address(), regular_tx);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  size_t zc_inputs = 0;
  for (const auto& input : regular_tx.vin)
  {
    if (input.type() != typeid(txin_zc_input))
      continue;
    ++zc_inputs;
    CHECK_AND_ASSERT_EQ(boost::get<txin_zc_input>(input).key_offsets.size(), CURRENCY_HF4_MANDATORY_DECOY_SET_SIZE + 1);
  }
  CHECK_AND_ASSERT_MES(zc_inputs > 0, false, "regular HF4 transaction has no ZC inputs");
  LOG_PRINT_MAGENTA("[HF4 TRANSITION] regular tx success: tx=" << get_transaction_hash(regular_tx)
    << ", ZC inputs=" << zc_inputs << ", ring size=" << CURRENCY_HF4_MANDATORY_DECOY_SET_SIZE + 1, LOG_LEVEL_0);

  CHECK_AND_ASSERT_MES(mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c), false, "cannot mine regular transition transaction");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  return true;
}
