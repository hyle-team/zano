// Copyright (c) 2023-2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "chaingen.h"
#include "hard_fork_4.h"
#include "random_helper.h"
#include "storages/portable_storage_template_helper.h"
#include "wallet/bare_outputs_snapshot.h"

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

hardfork_4_bare_decoy_indices::hardfork_4_bare_decoy_indices()
{
  REGISTER_CALLBACK_METHOD(hardfork_4_bare_decoy_indices, configure_core);
  REGISTER_CALLBACK_METHOD(hardfork_4_bare_decoy_indices, c1);

  m_hardforks.clear();
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_01, 0);
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_02, 0);
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_03, 0);
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_04_ZARCANUM, HF4_ACTIVE_AFTER);
}

bool hardfork_4_bare_decoy_indices::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  CHECK_AND_ASSERT_MES(test_chain_unit_enchanced::configure_core(c, ev_index, events), false, "default configure_core failed");
  core_runtime_config config = c.get_blockchain_storage().get_core_runtime_config();
  config.hf4_minimum_mixins = CURRENCY_HF4_MANDATORY_DECOY_SET_SIZE;
  c.get_blockchain_storage().set_core_runtime_config(config);
  return true;
}

bool hardfork_4_bare_decoy_indices::generate(std::vector<test_event_entry>& events) const
{
  // A short chain with seventeen bare outputs of the same amount.
  const uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc = m_accounts[BOB_ACC_IDX]; bob_acc.generate(); bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  const uint64_t bare_amount = 6 * COIN / 10;
  std::vector<tx_destination_entry> destinations{
    tx_destination_entry(bare_amount, alice_acc.get_public_address()),
    tx_destination_entry(COIN, miner_acc.get_public_address())
  };
  CHECK_AND_ASSERT_MES(replace_coinbase_in_genesis_block(destinations, generator, events, blk_0), false, "cannot fund the bare input");
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  transaction decoy_tx{};
  CHECK_AND_ASSERT_MES(construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), bob_acc.get_public_address(),
    DECOYS_COUNT * bare_amount, DECOYS_COUNT, TESTS_DEFAULT_FEE, decoy_tx), false, "cannot create the bare decoys");
  ADD_CUSTOM_EVENT(events, decoy_tx);
  MAKE_NEXT_BLOCK_TX1(events, decoy_block, blk_0r, miner_acc, decoy_tx);
  CHECK_AND_ASSERT_EQ(get_block_height(decoy_block), DECOY_BLOCK_HEIGHT);
  REWIND_BLOCKS_N_WITH_TIME(events, final_block, decoy_block, miner_acc, FINAL_TOP_HEIGHT - DECOY_BLOCK_HEIGHT);
  DO_CALLBACK(events, "c1");
  return true;
}

bool hardfork_4_bare_decoy_indices::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  struct core_proxy : public tools::i_core_proxy
  {
    enum mutation { NONE, MISSING, DUPLICATE, OWN_KEY, OLD_NODE, NOT_ALLOWED, HEIGHT_LIMIT, EMPTY_POOL, OWN_ABSENT };

    core_proxy(std::shared_ptr<tools::i_core_proxy> delegate, mutation change)
      : m_delegate(std::move(delegate)), m_change(change)
    {}

    bool call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request& req,
      COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response& rsp) override
    {
      ++m_calls;
      m_request = req;
      CHECK_AND_ASSERT_MES(req.batches.size() == 1 && req.batches[0].heights.empty()
        && req.batches[0].ring_size == CURRENCY_HF4_MANDATORY_DECOY_SET_SIZE + 1,
        false, "wallet did not request bare candidates");
      if (m_change == OLD_NODE)
      {
        auto legacy_req = req;
        legacy_req.batches[0].global_offsets.clear();
        legacy_req.batches[0].ring_size = 0;
        CHECK_AND_ASSERT_MES(m_delegate->call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4(legacy_req, rsp), false, "legacy output RPC failed");
        m_response = rsp;
        return true;
      }
      auto forwarded = req;
      if (m_change == HEIGHT_LIMIT)
        forwarded.height_upper_limit = 1; // bare candidates must ignore the height limit
      else if (m_change == OWN_ABSENT)
        for (size_t i = 0; i < forwarded.batches[0].global_offsets.size(); ++i)
          forwarded.batches[0].global_offsets[i] = i + 1; // all sixteen Bob outputs, Alice adds her own index locally
      CHECK_AND_ASSERT_MES(m_delegate->call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4(forwarded, rsp), false, "candidate output RPC failed");
      CHECK_AND_ASSERT_MES(rsp.status == API_RETURN_CODE_OK && rsp.blocks_batches.size() == 1
        && !rsp.blocks_batches[0].blocks.empty() && !rsp.blocks_batches[0].blocks[0].outs.empty(), false, "invalid genuine candidate response");
      auto& blocks = rsp.blocks_batches[0].blocks;
      auto& outs = blocks[0].outs;
      if (m_change == MISSING)
      {
        // a truncated pool must not reduce the requested number of decoys
        size_t remaining = CURRENCY_HF4_MANDATORY_DECOY_SET_SIZE - 1;
        for (auto& block : blocks)
        {
          const size_t keep = std::min(remaining, block.outs.size());
          block.outs.resize(keep);
          remaining -= keep;
        }
      }
      else if (m_change == DUPLICATE)
      {
        blocks.back().outs.back().global_amount_index = outs[0].global_amount_index;
      }
      else if (m_change == OWN_KEY || m_change == NOT_ALLOWED)
      {
        for (auto& block : blocks)
          for (auto& output : block.outs)
          {
            // index 0 belongs to Alice; all other matching outputs belong to Bob
            if (output.global_amount_index == 0)
              continue;
            if (m_change == OWN_KEY)
              output.stealth_address = null_pkey;
            else
              output.flags |= RANDOM_OUTPUTS_FOR_AMOUNTS_FLAGS_NOT_ALLOWED;
          }
      }
      else if (m_change == EMPTY_POOL)
        blocks.clear();
      m_response = rsp;
      return true;
    }

    bool call_COMMAND_RPC_SEND_RAW_TX(const COMMAND_RPC_SEND_RAW_TX::request& req, COMMAND_RPC_SEND_RAW_TX::response& rsp) override
    {
      ++m_sends;
      return m_delegate->call_COMMAND_RPC_SEND_RAW_TX(req, rsp);
    }

    std::shared_ptr<tools::i_core_proxy> m_delegate;
    mutation m_change;
    size_t m_calls = 0;
    size_t m_sends = 0;
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request m_request{};
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response m_response{};
  };

  const uint64_t bare_amount = 6 * COIN / 10;
  const size_t decoys_count = CURRENCY_HF4_MANDATORY_DECOY_SET_SIZE;
  auto& bcs = c.get_blockchain_storage();
  CHECK_AND_ASSERT_EQ(bcs.get_top_block_height(), FINAL_TOP_HEIGHT);
  CHECK_AND_ASSERT_EQ(bcs.get_outputs_container().get_item_size(bare_amount), DECOYS_COUNT + 1);
  CHECK_AND_ASSERT_EQ(bcs.get_core_runtime_config().hf4_minimum_mixins, decoys_count);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  const auto count_it = currency::bare_outputs_snapshot::bare_output_count_by_amount.find(bare_amount);
  CHECK_AND_ASSERT_MES(count_it != currency::bare_outputs_snapshot::bare_output_count_by_amount.end()
    && count_it->second > DECOYS_COUNT + 1, false, "embedded count does not exceed the test chain's amount range");

  // released binary RPC messages omit the additive fields, decode through the actual KV path
  epee::serialization::portable_storage legacy_request_storage;
  const std::string legacy_request_json = "{\"batches\":[{\"input_amount\":" + std::to_string(bare_amount)
    + ",\"heights\":[0]}],\"height_upper_limit\":0,\"look_up_strategy\":\"" + std::string(LOOK_UP_STRATEGY_REGULAR_TX) + "\"}";
  CHECK_AND_ASSERT_MES(legacy_request_storage.load_from_json(legacy_request_json), false, "cannot parse legacy request fixture");
  std::string legacy_request_binary;
  CHECK_AND_ASSERT_MES(legacy_request_storage.store_to_binary(legacy_request_binary), false, "cannot encode legacy request fixture");
  COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request decoded_request{};
  decoded_request.batches.resize(1);
  decoded_request.batches[0].global_offsets = {99};
  decoded_request.batches[0].ring_size = 99;
  CHECK_AND_ASSERT_MES(epee::serialization::load_t_from_binary(decoded_request, legacy_request_binary), false, "cannot decode legacy request fixture");
  CHECK_AND_ASSERT_EQ(decoded_request.batches.size(), 1);
  CHECK_AND_ASSERT_EQ(decoded_request.batches[0].input_amount, bare_amount);
  CHECK_AND_ASSERT_EQ(decoded_request.batches[0].heights.size(), 1);
  CHECK_AND_ASSERT_EQ(decoded_request.batches[0].heights[0], 0);
  CHECK_AND_ASSERT_MES(decoded_request.batches[0].global_offsets.empty(), false, "missing candidates did not default to empty");
  CHECK_AND_ASSERT_EQ(decoded_request.batches[0].ring_size, 0);

  epee::serialization::portable_storage legacy_response_storage;
  CHECK_AND_ASSERT_MES(legacy_response_storage.load_from_json("{\"blocks_batches\":[{\"blocks\":[]}],\"status\":\"OK\"}"),
    false, "cannot parse legacy response fixture");
  std::string legacy_response_binary;
  CHECK_AND_ASSERT_MES(legacy_response_storage.store_to_binary(legacy_response_binary), false, "cannot encode legacy response fixture");
  COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response decoded_response{};
  CHECK_AND_ASSERT_MES(epee::serialization::load_t_from_binary(decoded_response, legacy_response_binary), false, "cannot decode legacy response fixture");
  CHECK_AND_ASSERT_EQ(decoded_response.status, std::string(API_RETURN_CODE_OK));
  CHECK_AND_ASSERT_EQ(decoded_response.blocks_batches.size(), 1);
  CHECK_AND_ASSERT_MES(decoded_response.blocks_batches[0].blocks.empty(), false, "legacy response unexpectedly contains blocks");

  COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request candidate_req{};
  candidate_req.look_up_strategy = LOOK_UP_STRATEGY_REGULAR_TX;
  candidate_req.batches.resize(1);
  candidate_req.batches[0].input_amount = bare_amount;
  candidate_req.batches[0].global_offsets = {0, 1};
  candidate_req.batches[0].ring_size = 2;
  auto check_candidate_response = [&](const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request& req, size_t expected_count, std::unordered_set<uint64_t>& indices) -> bool
  {
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response rsp{};
    CHECK_AND_ASSERT_MES(m_core_proxy->call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4(req, rsp), false, "candidate output RPC failed");
    CHECK_AND_ASSERT_EQ(rsp.status, std::string(API_RETURN_CODE_OK));
    CHECK_AND_ASSERT_EQ(rsp.blocks_batches.size(), 1);
    const auto& batch = rsp.blocks_batches[0];
    const uint64_t indexed_output_count = bcs.get_outputs_container().get_item_size(req.batches[0].input_amount);
    indices.clear();
    for (const auto& block : batch.blocks)
      for (const auto& output : block.outs)
      {
        CHECK_AND_ASSERT_MES(indices.insert(output.global_amount_index).second, false, "duplicate candidate response identity");
        CHECK_AND_ASSERT_MES(output.global_amount_index < indexed_output_count, false, "out-of-range candidate response identity");
        CHECK_AND_ASSERT_EQ(block.block_height, (output.global_amount_index == 0 ? 0 : DECOY_BLOCK_HEIGHT));
        CHECK_AND_ASSERT_EQ(output.flags, 0);
      }
    CHECK_AND_ASSERT_EQ(indices.size(), expected_count);
    return true;
  };
  std::unordered_set<uint64_t> response_indices;
  decoded_request.height_upper_limit = 1;
  CHECK_AND_ASSERT_MES(check_candidate_response(decoded_request, 1, response_indices), false, "legacy height lookup ignored its height limit");
  CHECK_AND_ASSERT_MES(response_indices.count(0), false, "legacy height lookup omitted the genesis output");
  CHECK_AND_ASSERT_MES(check_candidate_response(candidate_req, 2, response_indices), false, "valid candidates failed");
  CHECK_AND_ASSERT_MES(response_indices.count(0) && response_indices.count(1), false, "valid candidate indices were remapped");
  candidate_req.batches[0].global_offsets = {DECOYS_COUNT + 1, 1}; // index == N must wrap to 0.
  CHECK_AND_ASSERT_MES(check_candidate_response(candidate_req, 2, response_indices), false, "boundary modulo failed");
  CHECK_AND_ASSERT_MES(response_indices.count(0) && response_indices.count(1), false, "boundary index did not map to zero");
  candidate_req.batches[0].global_offsets = {DECOYS_COUNT, DECOYS_COUNT};
  CHECK_AND_ASSERT_MES(check_candidate_response(candidate_req, 2, response_indices), false, "collision wrap failed");
  CHECK_AND_ASSERT_MES(response_indices.count(DECOYS_COUNT) && response_indices.count(0), false, "collision did not wrap from the last index to zero");
  candidate_req.batches[0].global_offsets = {2 * DECOYS_COUNT + 1, 2 * DECOYS_COUNT + 1}; // Both seeds map to the last index.
  CHECK_AND_ASSERT_MES(check_candidate_response(candidate_req, 2, response_indices), false, "modulo collision wrap failed");
  CHECK_AND_ASSERT_MES(response_indices.count(DECOYS_COUNT) && response_indices.count(0), false, "modulo collision did not wrap to zero");
  candidate_req.batches[0].ring_size = 4;
  candidate_req.batches[0].global_offsets = {0, 0, 8, 8};
  CHECK_AND_ASSERT_MES(check_candidate_response(candidate_req, 4, response_indices), false, "per-candidate collision fill failed");
  CHECK_AND_ASSERT_MES(response_indices.count(0) && response_indices.count(1) && response_indices.count(8) && response_indices.count(9),
    false, "collisions were not resolved from their own candidate indices");
  candidate_req.batches[0].ring_size = 2;
  candidate_req.batches[0].global_offsets = {DECOYS_COUNT, DECOYS_COUNT};
  candidate_req.height_upper_limit = 1;
  CHECK_AND_ASSERT_MES(check_candidate_response(candidate_req, 2, response_indices), false, "bare candidates used the height limit");
  CHECK_AND_ASSERT_MES(response_indices.count(DECOYS_COUNT) && response_indices.count(0), false, "height limit changed bare collision resolution");
  candidate_req.height_upper_limit = 0;
  candidate_req.batches[0].ring_size = decoys_count + 1;
  candidate_req.batches[0].global_offsets.assign(decoys_count + 1, 2 * (DECOYS_COUNT + 1));
  CHECK_AND_ASSERT_MES(check_candidate_response(candidate_req, decoys_count + 1, response_indices), false, "collision fill failed");
  for (uint64_t index = 0; index < decoys_count + 1; ++index)
    CHECK_AND_ASSERT_MES(response_indices.count(index), false, "linear collision fill omitted index " << index);
  candidate_req.batches[0].global_offsets.assign(2 * (decoys_count + 1), 0);
  CHECK_AND_ASSERT_MES(check_candidate_response(candidate_req, DECOYS_COUNT + 1, response_indices), false, "full bucket lookup omitted eligible outputs");
  candidate_req.height_upper_limit = 1;
  CHECK_AND_ASSERT_MES(check_candidate_response(candidate_req, DECOYS_COUNT + 1, response_indices), false, "height limit reduced the bare pool");
  candidate_req.height_upper_limit = 0;
  candidate_req.batches[0].input_amount = bare_amount + 1;
  CHECK_AND_ASSERT_MES(check_candidate_response(candidate_req, 0, response_indices), false, "empty bucket was not handled");
  candidate_req.batches[0].input_amount = bare_amount;

  auto check_rejected_request = [&](const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request& req, const std::string& status) -> bool
  {
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response rsp{};
    CHECK_AND_ASSERT_MES(m_core_proxy->call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4(req, rsp), false, "request failed before reaching the handler");
    CHECK_AND_ASSERT_EQ(rsp.status, status);
    return true;
  };
  auto bad_req = candidate_req;
  bad_req.batches[0].input_amount = 0;
  CHECK_AND_ASSERT_MES(check_rejected_request(bad_req, API_RETURN_CODE_FAIL), false, "ZC amount accepted in bare mode");
  bad_req = candidate_req;
  bad_req.batches[0].heights = {0};
  CHECK_AND_ASSERT_MES(check_rejected_request(bad_req, API_RETURN_CODE_FAIL), false, "mixed index and height modes accepted");
  bad_req = candidate_req;
  bad_req.batches[0].ring_size = 1;
  CHECK_AND_ASSERT_MES(check_rejected_request(bad_req, API_RETURN_CODE_FAIL), false, "invalid ring size accepted");
  bad_req = candidate_req;
  bad_req.batches[0].global_offsets = {0};
  CHECK_AND_ASSERT_MES(check_rejected_request(bad_req, API_RETURN_CODE_FAIL), false, "candidate target smaller than ring accepted");
  bad_req = candidate_req;
  bad_req.batches[0].global_offsets.assign(CURRENCY_TX_MAX_ALLOWED_INPUTS * (CURRENCY_HF4_MANDATORY_DECOY_SET_SIZE + 1) * 2 + 1, 0);
  CHECK_AND_ASSERT_MES(check_rejected_request(bad_req, API_RETURN_CODE_ARG_OUT_OF_LIMITS), false, "candidate request exceeded the RPC limit");

  auto attempt_transfer = [&](core_proxy::mutation change, size_t account, uint64_t amount,
    bool expect_success, bool keep_in_pool = false) -> bool
  {
    // Fresh wallets and a purged pool let every positive case reach actual core validation.
    const size_t pool_before = c.get_pool_transactions_count();
    auto wallet = init_playtime_test_wallet(events, c, account);
    wallet->refresh();
    std::vector<tools::transfer_details> own_outputs;
    wallet->enumerate_transfers([&](uint64_t, const tools::transfer_details& td) -> bool
    {
      CHECK_AND_ASSERT_MES(!td.is_zc() && !td.is_spent() && td.amount() == bare_amount, false, "unexpected owned output");
      own_outputs.push_back(td);
      return true;
    });
    CHECK_AND_ASSERT_EQ(own_outputs.size(), (account == ALICE_ACC_IDX ? 1 : DECOYS_COUNT));
    auto proxy = std::make_shared<core_proxy>(m_core_proxy, change);
    wallet->set_core_proxy(proxy);
    transaction tx{};
    bool rejected = false;
    bool not_enough_decoys = false;
    try
    {
      std::vector<tx_destination_entry> destinations{tx_destination_entry(amount, m_accounts[MINER_ACC_IDX].get_public_address())};
      wallet->transfer(destinations, decoys_count, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment, tx);
    }
    catch (const tools::error::not_enough_outs_to_mix& e)
    {
      rejected = true;
      not_enough_decoys = true;
      LOG_PRINT_MAGENTA("[BARE DECOY INDICES] wallet rejected transfer: " << e.what(), LOG_LEVEL_0);
    }
    catch (const tools::error::wallet_common_error& e)
    {
      rejected = true;
      LOG_PRINT_MAGENTA("[BARE DECOY INDICES] wallet rejected transfer: " << e.what(), LOG_LEVEL_0);
    }
    wallet->set_core_proxy(m_core_proxy);
    CHECK_AND_ASSERT_EQ(rejected, !expect_success);
    CHECK_AND_ASSERT_EQ(proxy->m_calls, 1);
    CHECK_AND_ASSERT_EQ(proxy->m_sends, (expect_success ? 1 : 0));
    CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), pool_before + (expect_success ? 1 : 0));
    CHECK_AND_ASSERT_EQ(proxy->m_request.batches.size(), 1);
    const auto& batch = proxy->m_request.batches[0];
    CHECK_AND_ASSERT_EQ(batch.input_amount, bare_amount);
    CHECK_AND_ASSERT_EQ(batch.ring_size, decoys_count + 1);
    CHECK_AND_ASSERT_MES(batch.heights.empty(), false, "candidate lookup used block heights");
    const size_t inputs_count = account == ALICE_ACC_IDX ? 1 : 2;
    CHECK_AND_ASSERT_EQ(batch.global_offsets.size(), inputs_count * (decoys_count + 1));
    for (uint64_t index : batch.global_offsets)
      CHECK_AND_ASSERT_MES(index < count_it->second, false, "wallet candidate exceeded the embedded amount range");
    if (!expect_success)
    {
      if (change == core_proxy::MISSING || change == core_proxy::EMPTY_POOL || change == core_proxy::OLD_NODE)
        CHECK_AND_ASSERT_MES(not_enough_decoys, false, "short or empty pool did not report insufficient decoys");
      return true;
    }

    CHECK_AND_ASSERT_MES(!not_enough_decoys, false, "successful transfer reported insufficient decoys");
    CHECK_AND_ASSERT_EQ(tx.vin.size(), inputs_count);
    const auto& response_batch = proxy->m_response.blocks_batches[0];
    std::unordered_set<uint64_t> available;
    for (const auto& block : response_batch.blocks)
      for (const auto& output : block.outs)
        available.insert(output.global_amount_index);
    if (account == BOB_ACC_IDX)
      CHECK_AND_ASSERT_EQ(available.size(), DECOYS_COUNT + 1);
    if (change == core_proxy::OWN_ABSENT)
      CHECK_AND_ASSERT_MES(!available.count(0), false, "own-absent control returned Alice's output");
    if (change == core_proxy::HEIGHT_LIMIT)
      CHECK_AND_ASSERT_MES(std::any_of(response_batch.blocks.begin(), response_batch.blocks.end(), [](const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::outputs_in_block& block)
        { return block.block_height > 1 && !block.outs.empty(); }), false, "height-limit control returned no later bare outputs");
    std::unordered_set<uint64_t> real_indices;
    for (const auto& input_variant : tx.vin)
    {
      CHECK_AND_ASSERT_MES(input_variant.type() == typeid(txin_to_key), false, "expected a bare input");
      const auto& input = boost::get<txin_to_key>(input_variant);
      auto real = std::find_if(own_outputs.begin(), own_outputs.end(), [&](const tools::transfer_details& td) { return td.m_key_image == input.k_image; });
      CHECK_AND_ASSERT_MES(real != own_outputs.end(), false, "wallet selected an unknown input");
      CHECK_AND_ASSERT_MES(real_indices.insert(real->m_global_output_index).second, false, "wallet reused a real input");
      CHECK_AND_ASSERT_EQ(input.key_offsets.size(), decoys_count + 1);
      const auto absolute = relative_output_offsets_to_absolute(input.key_offsets);
      std::unordered_set<uint64_t> ring_indices;
      for (const auto& reference : absolute)
      {
        CHECK_AND_ASSERT_MES(reference.type() == typeid(uint64_t), false, "unexpected output reference");
        const uint64_t index = boost::get<uint64_t>(reference);
        CHECK_AND_ASSERT_MES((index == real->m_global_output_index || available.count(index)) && ring_indices.insert(index).second, false, "invalid or duplicate ring output");
      }
      CHECK_AND_ASSERT_MES(ring_indices.count(real->m_global_output_index), false, "ring omitted its real output");
    }
    LOG_PRINT_MAGENTA("[BARE DECOY COUNTS] " << tx.vin.size() << " bare inputs, " << batch.global_offsets.size()
      << " candidates, " << available.size() << " available outputs", LOG_LEVEL_0);
    if (!keep_in_pool)
      c.get_tx_pool().purge_transactions();
    return true;
  };

  for (const auto change : {core_proxy::MISSING, core_proxy::DUPLICATE, core_proxy::OWN_KEY, core_proxy::OLD_NODE, core_proxy::NOT_ALLOWED, core_proxy::EMPTY_POOL})
    CHECK_AND_ASSERT_MES(attempt_transfer(change, BOB_ACC_IDX, COIN, false), false, "malformed or unavailable response was accepted");
  CHECK_AND_ASSERT_MES(attempt_transfer(core_proxy::OWN_ABSENT, ALICE_ACC_IDX, COIN / 2, true), false, "full pool without the own output could not be spent");
  CHECK_AND_ASSERT_MES(attempt_transfer(core_proxy::NONE, ALICE_ACC_IDX, COIN / 2, true), false, "old genesis input could not be spent");
  CHECK_AND_ASSERT_MES(attempt_transfer(core_proxy::HEIGHT_LIMIT, ALICE_ACC_IDX, COIN / 2, true), false, "height limit prevented a bare transfer");
  CHECK_AND_ASSERT_MES(attempt_transfer(core_proxy::NONE, BOB_ACC_IDX, COIN, true, true), false, "aggregated bare candidate lookup failed");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  return true;
}

//------------------------------------------------------------------------------

hardfork_4_bare_decoy_forced_mix::hardfork_4_bare_decoy_forced_mix()
{
  REGISTER_CALLBACK_METHOD(hardfork_4_bare_decoy_forced_mix, c1);
}

bool hardfork_4_bare_decoy_forced_mix::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc = m_accounts[BOB_ACC_IDX]; bob_acc.generate(); bob_acc.set_createtime(ts);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  const uint64_t bare_amount = 6 * COIN / 10;
  std::vector<tx_destination_entry> destinations{
    tx_destination_entry(bare_amount, alice_acc.get_public_address()),
    tx_destination_entry(COIN, miner_acc.get_public_address())
  };
  CHECK_AND_ASSERT_MES(replace_coinbase_in_genesis_block(destinations, generator, events, blk_0), false, "cannot fund the relaxed bare input");
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // all sixteen Bob outputs require a ring of eighteen, but this amount has only seventeen outputs in total.
  std::vector<tx_destination_entry> forced_destinations(DECOYS_COUNT, tx_destination_entry(bare_amount, bob_acc.get_public_address()));
  transaction decoy_tx{};
  CHECK_AND_ASSERT_MES(construct_tx_to_key(m_hardforks, events, decoy_tx, blk_0r, miner_acc, forced_destinations,
    TESTS_DEFAULT_FEE, 0, DECOYS_COUNT + 2), false, "cannot create forced-mix bare outputs");
  ADD_CUSTOM_EVENT(events, decoy_tx);
  MAKE_NEXT_BLOCK_TX1(events, decoy_block, blk_0r, miner_acc, decoy_tx);
  CHECK_AND_ASSERT_EQ(get_block_height(decoy_block), DECOY_BLOCK_HEIGHT);
  REWIND_BLOCKS_N_WITH_TIME(events, final_block, decoy_block, miner_acc, FINAL_TOP_HEIGHT - DECOY_BLOCK_HEIGHT);
  DO_CALLBACK(events, "c1");
  return true;
}

bool hardfork_4_bare_decoy_forced_mix::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  struct core_proxy : public tools::i_core_proxy
  {
    explicit core_proxy(std::shared_ptr<tools::i_core_proxy> delegate) : m_delegate(std::move(delegate)) {}

    bool call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response& rsp) override
    {
      return m_delegate->call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4(req, rsp);
    }

    bool call_COMMAND_RPC_SEND_RAW_TX(const COMMAND_RPC_SEND_RAW_TX::request& req, COMMAND_RPC_SEND_RAW_TX::response& rsp) override
    {
      ++m_sends;
      return m_delegate->call_COMMAND_RPC_SEND_RAW_TX(req, rsp);
    }

    std::shared_ptr<tools::i_core_proxy> m_delegate;
    size_t m_sends = 0;
  };

  const uint64_t bare_amount = 6 * COIN / 10;
  const size_t ring_size = DECOYS_COUNT + 2;
  CHECK_AND_ASSERT_EQ(c.get_blockchain_storage().get_outputs_container().get_item_size(bare_amount), DECOYS_COUNT + 1);
  COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request req{};
  req.look_up_strategy = LOOK_UP_STRATEGY_REGULAR_TX;
  req.batches.resize(1);
  req.batches[0].input_amount = bare_amount;
  auto check_forced_pool = [&](size_t requested_ring_size, size_t expected_count) -> bool
  {
    req.batches[0].ring_size = requested_ring_size;
    req.batches[0].global_offsets.assign(requested_ring_size, 0);
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response rsp{};
    CHECK_AND_ASSERT_MES(m_core_proxy->call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4(req, rsp), false, "forced-mix output RPC failed");
    CHECK_AND_ASSERT_EQ(rsp.status, std::string(API_RETURN_CODE_OK));
    CHECK_AND_ASSERT_EQ(rsp.blocks_batches.size(), 1);
    std::unordered_set<uint64_t> returned_indices;
    for (const auto& block : rsp.blocks_batches[0].blocks)
      for (const auto& output : block.outs)
      {
        CHECK_AND_ASSERT_MES(output.global_amount_index < DECOYS_COUNT + 1 && returned_indices.insert(output.global_amount_index).second, false, "invalid or duplicate forced-mix output");
        if (requested_ring_size < ring_size)
          CHECK_AND_ASSERT_EQ(output.global_amount_index, 0);
        CHECK_AND_ASSERT_EQ(block.block_height, (output.global_amount_index == 0 ? 0 : DECOY_BLOCK_HEIGHT));
        CHECK_AND_ASSERT_EQ(output.flags, 0);
      }
    CHECK_AND_ASSERT_EQ(returned_indices.size(), expected_count);
    CHECK_AND_ASSERT_MES(returned_indices.count(0), false, "relaxed genesis output was omitted");
    return true;
  };
  CHECK_AND_ASSERT_MES(check_forced_pool(ring_size, DECOYS_COUNT + 1), false, "outputs eligible for the requested ring were pruned");
  CHECK_AND_ASSERT_MES(check_forced_pool(ring_size - 1, 1), false, "forced-mix outputs accepted a smaller requested ring");

  auto proxy = std::make_shared<core_proxy>(m_core_proxy);
  for (size_t account : {BOB_ACC_IDX, ALICE_ACC_IDX})
  {
    auto wallet = init_playtime_test_wallet(events, c, account);
    wallet->refresh();
    wallet->set_core_proxy(proxy);
    bool rejected = false;
    try
    {
      transaction tx{};
      std::vector<tx_destination_entry> destinations{tx_destination_entry(COIN / 2, m_accounts[MINER_ACC_IDX].get_public_address())};
      wallet->transfer(destinations, ring_size - 1, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment, tx);
    }
    catch (const tools::error::not_enough_outs_to_mix&)
    {
      rejected = true;
    }
    CHECK_AND_ASSERT_MES(rejected, false, "insufficient decoys did not reject the requested ring");
    CHECK_AND_ASSERT_EQ(proxy->m_sends, 0);
    CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  }
  LOG_PRINT_MAGENTA("[BARE DECOY COUNTS] forced-mix eligibility uses the requested ring; both wallets reject insufficient decoys without broadcasting", LOG_LEVEL_0);
  return true;
}

//------------------------------------------------------------------------------

hardfork_4_bare_decoy_before_hf4::hardfork_4_bare_decoy_before_hf4()
{
  REGISTER_CALLBACK_METHOD(hardfork_4_bare_decoy_before_hf4, c1);
}

bool hardfork_4_bare_decoy_before_hf4::generate(std::vector<test_event_entry>& events) const
{
  const uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  std::vector<tx_destination_entry> destinations{
    tx_destination_entry(6 * COIN / 10, alice_acc.get_public_address()),
    tx_destination_entry(COIN, miner_acc.get_public_address())
  };
  CHECK_AND_ASSERT_MES(replace_coinbase_in_genesis_block(destinations, generator, events, blk_0), false, "cannot fund the pre-HF4 bare input");
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, final_block, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  DO_CALLBACK(events, "c1");
  return true;
}

bool hardfork_4_bare_decoy_before_hf4::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  struct core_proxy : public tools::i_core_proxy
  {
    explicit core_proxy(std::shared_ptr<tools::i_core_proxy> delegate) : m_delegate(std::move(delegate)) {}

    bool call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response& rsp) override
    {
      ++m_calls;
      m_request = req;
      const bool result = m_delegate->call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4(req, rsp);
      m_response = rsp;
      return result;
    }

    bool call_COMMAND_RPC_SEND_RAW_TX(const COMMAND_RPC_SEND_RAW_TX::request& req, COMMAND_RPC_SEND_RAW_TX::response& rsp) override
    {
      ++m_sends;
      return m_delegate->call_COMMAND_RPC_SEND_RAW_TX(req, rsp);
    }

    std::shared_ptr<tools::i_core_proxy> m_delegate;
    size_t m_calls = 0;
    size_t m_sends = 0;
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request m_request{};
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response m_response{};
  };

  const uint64_t bare_amount = 6 * COIN / 10;
  auto& bcs = c.get_blockchain_storage();
  CHECK_AND_ASSERT_EQ(bcs.get_top_block_height(), CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(!bcs.get_core_runtime_config().is_hardfork_active_for_height(ZANO_HARDFORK_04_ZARCANUM, bcs.get_top_block_height()),
    false, "HF4 is already active in the pre-HF4 fixture");
  CHECK_AND_ASSERT_EQ(bcs.get_outputs_container().get_item_size(bare_amount), 1);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  auto wallet = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  wallet->refresh();
  // Use the shared embedded table even before HF4.
  auto proxy = std::make_shared<core_proxy>(m_core_proxy);
  wallet->set_core_proxy(proxy);
  transaction tx{};
  std::vector<tx_destination_entry> destinations{tx_destination_entry(COIN / 2, m_accounts[MINER_ACC_IDX].get_public_address())};
  bool rejected = false;
  try
  {
    wallet->transfer(destinations, CURRENCY_HF4_MANDATORY_DECOY_SET_SIZE, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment, tx);
  }
  catch (const tools::error::not_enough_outs_to_mix&)
  {
    rejected = true;
  }
  CHECK_AND_ASSERT_MES(rejected, false, "pre-HF4 shortage reduced the requested ring");
  CHECK_AND_ASSERT_EQ(proxy->m_calls, 1);
  CHECK_AND_ASSERT_EQ(proxy->m_sends, 0);
  CHECK_AND_ASSERT_EQ(proxy->m_request.batches.size(), 1);
  const auto& request_batch = proxy->m_request.batches[0];
  CHECK_AND_ASSERT_EQ(request_batch.input_amount, bare_amount);
  CHECK_AND_ASSERT_MES(request_batch.heights.empty(), false, "pre-HF4 bare input used height selection");
  CHECK_AND_ASSERT_EQ(request_batch.ring_size, CURRENCY_HF4_MANDATORY_DECOY_SET_SIZE + 1);
  CHECK_AND_ASSERT_EQ(request_batch.global_offsets.size(), CURRENCY_HF4_MANDATORY_DECOY_SET_SIZE + 1);
  const auto count_it = currency::bare_outputs_snapshot::bare_output_count_by_amount.find(bare_amount);
  CHECK_AND_ASSERT_MES(count_it != currency::bare_outputs_snapshot::bare_output_count_by_amount.end() && count_it->second != 0, false, "missing embedded count for the pre-HF4 bare amount");
  for (uint64_t global_offset : request_batch.global_offsets)
    CHECK_AND_ASSERT_MES(global_offset < count_it->second, false, "pre-HF4 candidate is outside the embedded amount range");
  CHECK_AND_ASSERT_EQ(proxy->m_response.status, std::string(API_RETURN_CODE_OK));
  CHECK_AND_ASSERT_EQ(proxy->m_response.blocks_batches.size(), 1);
  const auto& response_batch = proxy->m_response.blocks_batches[0];
  CHECK_AND_ASSERT_EQ(response_batch.blocks.size(), 1);
  CHECK_AND_ASSERT_EQ(response_batch.blocks[0].block_height, 0);
  CHECK_AND_ASSERT_EQ(response_batch.blocks[0].outs.size(), 1);
  CHECK_AND_ASSERT_EQ(response_batch.blocks[0].outs[0].global_amount_index, 0);
  CHECK_AND_ASSERT_EQ(response_batch.blocks[0].outs[0].flags, 0);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  LOG_PRINT_MAGENTA("[BARE DECOY COUNTS] pre-HF4 with the shared embedded table: sixteen candidates, N=1, shortage rejected without broadcasting", LOG_LEVEL_0);
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
