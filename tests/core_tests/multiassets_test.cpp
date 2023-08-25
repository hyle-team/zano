// Copyright (c) 2014-2023 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "multiassets_test.h"
#include "wallet_test_core_proxy.h"

#include "random_helper.h"
#include "wallet/wallet_debug_events_definitions.h"
using namespace currency;






/*


struct debug_context_event_1
{
  int& i;
  std::string& s;
};



//#define RAISE_DEBUG_EVENT   dw.handle_type


void test_test()
{
  epee::misc_utils::events_dispatcher ed;

  //--------------------------------------------------------------------------------
  //--------------------------------------------------------------------------------
  //--------------------------------------------------------------------------------
  //thus code will be called in the tests
  ed.SUBSCIRBE_DEBUG_EVENT<debug_context_event_1>([&](debug_context_event_1& d) 
  {
    //here some operations
    LOG_PRINT_L0("lala: " << d.i << d.s);    
    //
    d.i = 10;
    d.s = "33333";

  });


  //--------------------------------------------------------------------------------
  //--------------------------------------------------------------------------------
  //--------------------------------------------------------------------------------
  //this code will be in the wallet and helper functions

  int i = 22;
  std::string sss = "11111";
 
  ed.RAISE_DEBUG_EVENT(debug_context_event_1{i, sss });


  LOG_PRINT_L0("lala: " << i << sss);
}
*/







//------------------------------------------------------------------------------

#define  AMOUNT_TO_TRANSFER_MULTIASSETS_BASIC (TESTS_DEFAULT_FEE)
#define  AMOUNT_ASSETS_TO_TRANSFER_MULTIASSETS_BASIC  500000000000000000

uint64_t multiassets_basic_test::ts_starter = 0;
multiassets_basic_test::multiassets_basic_test()
{
  // TODO: remove the following line

  LOG_PRINT_MAGENTA("STARTER TS: " << ts_starter, LOG_LEVEL_0);
  random_state_test_restorer::reset_random(ts_starter);

  REGISTER_CALLBACK_METHOD(multiassets_basic_test, c1);
}

bool multiassets_basic_test::generate(std::vector<test_event_entry>& events) const
{
  m_accounts.resize(MINER_ACC_IDX+1);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX];
  miner_acc.generate();

  uint64_t ts = 145000000;
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  //TODO: Need to make sure REWIND_BLOCKS_N and other coretests codebase are capable of following hardfork4 rules
  //in this test hardfork4 moment moved to runtime section
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  DO_CALLBACK(events, "c1");

  return true;
}

bool multiassets_basic_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  //test_test();

  bool r = false;
  std::shared_ptr<debug_wallet2> miner_wlt = init_playtime_test_wallet_t<debug_wallet2>(events, c, MINER_ACC_IDX);
  miner_wlt->get_account().set_createtime(0);
  account_base alice_acc;
  alice_acc.generate();
  std::shared_ptr<debug_wallet2> alice_wlt = init_playtime_test_wallet_t<debug_wallet2>(events, c, alice_acc);
  alice_wlt->get_account().set_createtime(0);
  miner_wlt->refresh();

  asset_descriptor_base adb = AUTO_VAL_INIT(adb);
  adb.total_max_supply = 1000000000000000000; //1M coins
  adb.full_name = "Test coins";
  adb.ticker = "TCT";
  adb.decimal_point = 12;

  std::vector<currency::tx_destination_entry> destinations(2);
  destinations[0].addr.push_back(miner_wlt->get_account().get_public_address());
  destinations[0].amount = AMOUNT_ASSETS_TO_TRANSFER_MULTIASSETS_BASIC;
  destinations[0].asset_id = currency::null_pkey;
  destinations[1].addr.push_back(alice_wlt->get_account().get_public_address());
  destinations[1].amount = AMOUNT_ASSETS_TO_TRANSFER_MULTIASSETS_BASIC;
  destinations[1].asset_id = currency::null_pkey;
  
  LOG_PRINT_MAGENTA("destinations[0].asset_id:" << destinations[0].asset_id, LOG_LEVEL_0);
  LOG_PRINT_MAGENTA("destinations[1].asset_id:" << destinations[1].asset_id, LOG_LEVEL_0);
  LOG_PRINT_MAGENTA("currency::null_pkey:     " << currency::null_pkey, LOG_LEVEL_0);

  currency::transaction tx = AUTO_VAL_INIT(tx);
  crypto::public_key asset_id = currency::null_pkey;
  miner_wlt->deploy_new_asset(adb, destinations, tx, asset_id);
  LOG_PRINT_L0("Deployed new asset: " << asset_id << ", tx_id: " << currency::get_transaction_hash(tx));

  //pass over hardfork
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");


  miner_wlt->refresh();
  alice_wlt->refresh();
  uint64_t mined = 0;
  std::unordered_map<crypto::public_key,  tools::wallet_public::asset_balance_entry_base> balances;
  miner_wlt->balance(balances, mined);

  auto it_asset = balances.find(asset_id);
  auto it_native = balances.find(currency::native_coin_asset_id);


  CHECK_AND_ASSERT_MES(it_asset != balances.end() && it_native != balances.end(), false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_asset->second.total == AMOUNT_ASSETS_TO_TRANSFER_MULTIASSETS_BASIC, false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_native->second.total == uint64_t(17517226)*COIN, false, "Failed to find needed asset in result balances");


  balances.clear();
  alice_wlt->balance(balances, mined);

  it_asset = balances.find(asset_id);
  it_native = balances.find(currency::native_coin_asset_id);

  CHECK_AND_ASSERT_MES(it_asset != balances.end(), false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_native == balances.end(), false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_asset->second.total == AMOUNT_ASSETS_TO_TRANSFER_MULTIASSETS_BASIC, false, "Failed to find needed asset in result balances");
  
  miner_wlt->transfer(AMOUNT_ASSETS_TO_TRANSFER_MULTIASSETS_BASIC/2, alice_wlt->get_account().get_public_address(), asset_id);
  //pass over hardfork
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  alice_wlt->refresh();
  uint64_t last_alice_balances = alice_wlt->balance(asset_id, mined);
  CHECK_AND_ASSERT_MES(last_alice_balances == AMOUNT_ASSETS_TO_TRANSFER_MULTIASSETS_BASIC + AMOUNT_ASSETS_TO_TRANSFER_MULTIASSETS_BASIC/2, false, "Failed to find needed asset in result balances");

  

  {
    try {

      miner_wlt->transfer(AMOUNT_ASSETS_TO_TRANSFER_MULTIASSETS_BASIC / 2, alice_wlt->get_account().get_public_address(), asset_id);
      //pass over hardfork
      CHECK_AND_ASSERT_MES(false, false, "Transfer with 0 Zano worked(fail)");
    }
    catch (...)
    {
      LOG_PRINT_L0("Transfer failed as planned");
      //return true;
    }
  }

  miner_wlt->refresh();
  uint64_t last_miner_balance = miner_wlt->balance(asset_id, mined);


  asset_descriptor_base asset_info = AUTO_VAL_INIT(asset_info);
  /*
    adb.total_max_supply = 1000000000000000000; //1M coins
    adb.full_name = "Test coins";
    adb.ticker = "TCT";
    adb.decimal_point = 12 
  */
  r = c.get_blockchain_storage().get_asset_info(asset_id, asset_info);
  CHECK_AND_ASSERT_MES(r, false, "Failed to get_asset_info");

  CHECK_AND_ASSERT_MES(asset_info.current_supply == AMOUNT_ASSETS_TO_TRANSFER_MULTIASSETS_BASIC*2, false, "Failed to find needed asset in result balances");

  //test update function
  asset_info.meta_info = "{\"some\": \"info\"}";
  miner_wlt->update_asset(asset_id, asset_info, tx);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 2);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  asset_descriptor_base asset_info2 = AUTO_VAL_INIT(asset_info2);
  r = c.get_blockchain_storage().get_asset_info(asset_id, asset_info2);
  CHECK_AND_ASSERT_MES(r, false, "Failed to get_asset_info");

  CHECK_AND_ASSERT_MES(asset_info2.meta_info == asset_info.meta_info, false, "Failed to find needed asset in result balances");

  //test emmit function
  //use same destinations as we used before
  miner_wlt->emmit_asset(asset_id, destinations, tx);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  miner_wlt->refresh();
  alice_wlt->refresh();
  CHECK_AND_ASSERT_MES(miner_wlt->balance(asset_id, mined) == last_miner_balance + destinations[0].amount, false, "Miner balance wrong");
  CHECK_AND_ASSERT_MES(alice_wlt->balance(asset_id, mined) == last_alice_balances + destinations[1].amount, false, "Alice balance wrong");

  asset_descriptor_base asset_info3 = AUTO_VAL_INIT(asset_info3);
  r = c.get_blockchain_storage().get_asset_info(asset_id, asset_info3);
  CHECK_AND_ASSERT_MES(r, false, "Failed to get_asset_info");
  CHECK_AND_ASSERT_MES(asset_info3.current_supply == asset_info2.current_supply + destinations[1].amount + destinations[0].amount, false, "Failed to find needed asset in result balances");



  miner_wlt->burn_asset(asset_id, last_miner_balance, tx);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 1);

  miner_wlt->refresh();
  CHECK_AND_ASSERT_MES(miner_wlt->balance(asset_id, mined) == destinations[0].amount, false, "Miner balance wrong");

  asset_descriptor_base asset_info4 = AUTO_VAL_INIT(asset_info4);
  r = c.get_blockchain_storage().get_asset_info(asset_id, asset_info4);
  CHECK_AND_ASSERT_MES(r, false, "Failed to get_asset_info");
  CHECK_AND_ASSERT_MES(asset_info4.current_supply == asset_info3.current_supply - last_miner_balance, false, "Failed to find needed asset in result balances");


  //------------------- tests that trying to break stuff  -------------------
  //tests that trying to break stuff
  miner_wlt->get_debug_events_dispatcher().SUBSCIRBE_DEBUG_EVENT<wde_construct_tx_handle_asset_descriptor_operation>([&](wde_construct_tx_handle_asset_descriptor_operation& o)
  {
    crypto::signature s = currency::null_sig;
    o.pado->opt_proof = s;
  });


  //test update function with broken ownership
  r = c.get_blockchain_storage().get_asset_info(asset_id, asset_info);
  CHECK_AND_ASSERT_MES(r, false, "Failed to get_asset_info");

  asset_info.meta_info = "{\"some2\": \"info2\"}";
  miner_wlt->update_asset(asset_id, asset_info, tx);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 2);
  CHECK_AND_ASSERT_MES(!r, false, "Test failed, broken ownership passed");
  c.get_tx_pool().purge_transactions();
  miner_wlt->refresh();

  miner_wlt->get_debug_events_dispatcher().UNSUBSCRIBE_DEBUG_EVENT<wde_construct_tx_handle_asset_descriptor_operation>();

  //------------------- tests that trying to break stuff  -------------------
  //test update function with not allowed fields
  r = c.get_blockchain_storage().get_asset_info(asset_id, asset_info);
  CHECK_AND_ASSERT_MES(r, false, "Failed to get_asset_info");

  asset_info.ticker = "XXX";
  miner_wlt->update_asset(asset_id, asset_info, tx);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 2);
  CHECK_AND_ASSERT_MES(!r, false, "Test failed, broken ownership passed");
  c.get_tx_pool().purge_transactions();

  //------------------- tests that trying to break stuff  -------------------
  //test update function with not allowed fields
  r = c.get_blockchain_storage().get_asset_info(asset_id, asset_info);
  CHECK_AND_ASSERT_MES(r, false, "Failed to get_asset_info");

  asset_info.full_name = "XXX";
  miner_wlt->update_asset(asset_id, asset_info, tx);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 2);
  CHECK_AND_ASSERT_MES(!r, false, "Test failed, broken ownership passed");
  c.get_tx_pool().purge_transactions();
  miner_wlt->refresh();

  //------------------- tests that trying to break stuff  -------------------
  r = c.get_blockchain_storage().get_asset_info(asset_id, asset_info);
  CHECK_AND_ASSERT_MES(r, false, "Failed to get_asset_info");

  asset_info.decimal_point = 3;
  miner_wlt->update_asset(asset_id, asset_info, tx);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 2);
  CHECK_AND_ASSERT_MES(!r, false, "Test failed, broken ownership passed");
  c.get_tx_pool().purge_transactions();
  miner_wlt->refresh();

  //------------------- tests that trying to break stuff  -------------------
  r = c.get_blockchain_storage().get_asset_info(asset_id, asset_info);
  CHECK_AND_ASSERT_MES(r, false, "Failed to get_asset_info");

  currency::keypair kp = currency::keypair::generate();

  asset_info.owner = kp.pub;
  miner_wlt->update_asset(asset_id, asset_info, tx);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 2);
  CHECK_AND_ASSERT_MES(!r, false, "Test failed, broken ownership passed");
  c.get_tx_pool().purge_transactions();
  miner_wlt->refresh();

  //------------------- tests that trying to break stuff  -------------------
  miner_wlt->get_debug_events_dispatcher().SUBSCIRBE_DEBUG_EVENT<wde_construct_tx_handle_asset_descriptor_operation_before_seal>([&](wde_construct_tx_handle_asset_descriptor_operation_before_seal& o)
  {
    o.pado->descriptor.current_supply += 1000000;
  });
  //test emmit function but re-adjust current_supply to wrong amount
  miner_wlt->emmit_asset(asset_id, destinations, tx);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(!r, false, "mine_next_pow_blocks_in_playtime failed");
  c.get_tx_pool().purge_transactions();
  miner_wlt->refresh();

  //------------------- tests that trying to break stuff  -------------------
  //test burn that burn more then tx has
  miner_wlt->get_debug_events_dispatcher().UNSUBSCRIBE_DEBUG_EVENT<wde_construct_tx_handle_asset_descriptor_operation_before_seal>();

  miner_wlt->get_debug_events_dispatcher().SUBSCIRBE_DEBUG_EVENT<wde_construct_tx_handle_asset_descriptor_operation_before_seal>([&](wde_construct_tx_handle_asset_descriptor_operation_before_seal& o)
  {
    o.pado->descriptor.current_supply -= 1000000;
  });

  miner_wlt->burn_asset(asset_id, 10000000000000, tx);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(!r, false, "mine_next_pow_blocks_in_playtime failed");
  c.get_tx_pool().purge_transactions();
  miner_wlt->refresh();

  //

  return true;
}

//------------------------------------------------------------------------------

assets_and_explicit_native_coins_in_outs::assets_and_explicit_native_coins_in_outs()
{
  REGISTER_CALLBACK_METHOD(assets_and_explicit_native_coins_in_outs, c1_alice_cannot_deploy_asset);
  REGISTER_CALLBACK_METHOD(assets_and_explicit_native_coins_in_outs, c2_alice_deploys_asset);

  m_hardforks.clear();
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_04_ZARCANUM, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
}

bool assets_and_explicit_native_coins_in_outs::generate(std::vector<test_event_entry>& events) const
{
  /* Test idea:
  *    1) make sure an asset cannot be deployed if there's no ZC outputs available;
  *    2) make sure an asset emission transaction has hidden asset ids in all outputs;
  *    3) (NOT DONE YET) make sure tx with at least one input with at least one reference to non-explicit native asset id has non-explicit asset ids in outs (TODO: move to separate test)
  *    4) Bob get coins with non-explicit asset id and then tries to register an alias with them (some will be burnt with explicit asset id)
  */

  bool r = false;

  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // necessary to set m_hardforks

  // HF4 requires tests_random_split_strategy (for 2 outputs minimum)
  test_gentime_settings tgts = generator.get_test_gentime_settings();
  tgts.split_strategy = tests_random_split_strategy;
  generator.set_test_gentime_settings(tgts);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK_PARAMS(events, "check_hardfork_inactive", static_cast<size_t>(ZANO_HARDFORK_04_ZARCANUM));

  // tx_0: miner -> Alice
  // make tx_0 before HF4, so Alice will have only bare outs
  m_alice_initial_balance = MK_TEST_COINS(1000) + TESTS_DEFAULT_FEE;
  transaction tx_0{};
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc, alice_acc, m_alice_initial_balance, TESTS_DEFAULT_FEE, 0, sources, destinations, true /* spends */, false /* unlock time */);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_0, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  
  ADD_CUSTOM_EVENT(events, tx_0);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  // make sure HF4 has been activated
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_04_ZARCANUM));

  // rewind blocks
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // check Alice's balance and make sure she cannot deploy an asset
  DO_CALLBACK(events, "c1_alice_cannot_deploy_asset");

  // tx_1: Alice -> Alice (all coins) : this will convert two Alice's outputs to ZC outs (since we're at post-HF4 zone)
  MAKE_TX(events, tx_1, alice_acc, alice_acc, m_alice_initial_balance - TESTS_DEFAULT_FEE, blk_1r);
  CHECK_AND_ASSERT_MES(tx_1.vout.size() == 2, false, "unexpected tx_1.vout.size : " << tx_1.vout.size());

  // make sure that all tx_1 outputs have explicit hative coin asset id
  for(auto& out : tx_1.vout)
  {
    CHECK_AND_ASSERT_MES(out.type() == typeid(tx_out_zarcanum), false, "invalid out type");
    const tx_out_zarcanum& out_zc = boost::get<tx_out_zarcanum>(out);
    CHECK_AND_ASSERT_MES(out_zc.blinded_asset_id == native_coin_asset_id_1div8, false, "tx_1 has non-explicit asset id in outputs");
  }

  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_1);

  // rewind blocks
  REWIND_BLOCKS_N_WITH_TIME(events, blk_2r, blk_2, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // check Alice's balance and make sure she CAN deploy an asset now
  DO_CALLBACK(events, "c2_alice_deploys_asset");

  return true;
}

bool assets_and_explicit_native_coins_in_outs::c1_alice_cannot_deploy_asset(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  alice_wlt->refresh();

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", m_alice_initial_balance, 0, m_alice_initial_balance, 0, 0), false, "");

  asset_descriptor_base adb{};
  adb.total_max_supply = 10;
  adb.full_name = "it doesn't matter";
  adb.ticker = "really";
  adb.decimal_point = 12;

  std::vector<tx_destination_entry> destinations;
  destinations.emplace_back(adb.total_max_supply, m_accounts[MINER_ACC_IDX].get_public_address(), null_pkey);

  transaction asset_emission_tx{};
  crypto::public_key asset_id = null_pkey;
  bool r = false;
  try
  {
    alice_wlt->deploy_new_asset(adb, destinations, asset_emission_tx, asset_id);
  }
  catch(...)
  {
    r = true;
  }

  CHECK_AND_ASSERT_MES(r, false, "Alice successfully deployed an asset, which is unexpected (she has no ZC outs available)");

  return true;
}

bool assets_and_explicit_native_coins_in_outs::c2_alice_deploys_asset(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  alice_wlt->refresh();

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", m_alice_initial_balance - TESTS_DEFAULT_FEE, 0, m_alice_initial_balance - TESTS_DEFAULT_FEE, 0, 0), false, "");

  // make sure Alice has two UTXO now
  tools::wallet2::transfer_container transfers{};
  alice_wlt->get_transfers(transfers);
  size_t unspent_transfers = std::count_if(transfers.begin(), transfers.end(), [](const tools::wallet2::transfer_details& tr){ return !tr.is_spent(); });
  CHECK_AND_ASSERT_MES(unspent_transfers == 2, false, "unexpected number of Alice's unspent transfers: " << unspent_transfers);

  asset_descriptor_base adb{};
  adb.total_max_supply = 100 * 1000000000000;
  adb.full_name = "very confidential asset";
  adb.ticker = "VCA";
  adb.decimal_point = 12;

  std::vector<tx_destination_entry> destinations;
  destinations.emplace_back(adb.total_max_supply / 2, m_accounts[MINER_ACC_IDX].get_public_address(), null_pkey);
  destinations.emplace_back(adb.total_max_supply / 2, m_accounts[MINER_ACC_IDX].get_public_address(), null_pkey);

  transaction asset_emission_tx{};
  crypto::public_key asset_id = null_pkey;

  alice_wlt->deploy_new_asset(adb, destinations, asset_emission_tx, asset_id);


  // make sure the emission tx is correct
  CHECK_AND_ASSERT_MES(asset_emission_tx.vout.size() > 2, false, "Unexpected vout size: " << asset_emission_tx.vout.size());
  for(auto& out : asset_emission_tx.vout)
  {
    CHECK_AND_ASSERT_MES(out.type() == typeid(tx_out_zarcanum), false, "invalid out type");
    const tx_out_zarcanum& out_zc = boost::get<tx_out_zarcanum>(out);
    // as soon as this is the asset emmiting transaction, no output has an obvious asset id
    // make sure it is so
    CHECK_AND_ASSERT_MES(out_zc.blinded_asset_id != native_coin_asset_id_1div8, false, "One of outputs has explicit native asset id, which is unexpected");
  }

  // get this tx confirmed
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  // check Alice balance, make sure all native coins are unlocked
  alice_wlt->refresh();
  uint64_t alice_balance = m_alice_initial_balance - TESTS_DEFAULT_FEE * 2;
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", alice_balance, 0, alice_balance, 0, 0), false, "");

  // now Alice only has UTXO with non explicit asset id
  // Transfer all of them to Bob and check asset ids of outputs
  transaction tx_2{};
  alice_wlt->transfer(alice_balance - TESTS_DEFAULT_FEE, m_accounts[BOB_ACC_IDX].get_public_address(), tx_2, native_coin_asset_id);

  CHECK_AND_ASSERT_MES(tx_2.vout.size() == 2, false, "unexpected tx_2.vout.size : " << tx_2.vout.size());
  for(auto& out : tx_2.vout)
  {
    CHECK_AND_ASSERT_MES(out.type() == typeid(tx_out_zarcanum), false, "invalid out type");
    const tx_out_zarcanum& out_zc = boost::get<tx_out_zarcanum>(out);
    CHECK_AND_ASSERT_MES(out_zc.blinded_asset_id != native_coin_asset_id_1div8, false, "One of outputs has explicit native asset id, which is unexpected");
  }

  // finally, get this tx confirmed
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, m_accounts[BOB_ACC_IDX]);
  bob_wlt->refresh();
  uint64_t bob_balance = alice_balance - TESTS_DEFAULT_FEE;
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt, "Bob", bob_balance, 0, bob_balance, 0, 0), false, "");

  extra_alias_entry ae{};
  ae.m_alias = "kris.kelvin";
  ae.m_address = m_accounts[BOB_ACC_IDX].get_public_address();
  transaction tx_3{};
  bob_wlt->request_alias_registration(ae, tx_3, TESTS_DEFAULT_FEE);

  // finally, get this tx confirmed
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  return true;
}

//------------------------------------------------------------------------------

asset_depoyment_and_few_zc_utxos::asset_depoyment_and_few_zc_utxos()
{
  REGISTER_CALLBACK_METHOD(asset_depoyment_and_few_zc_utxos, c1);

  m_hardforks.clear();
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_04_ZARCANUM, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
}

bool asset_depoyment_and_few_zc_utxos::generate(std::vector<test_event_entry>& events) const
{
  bool r = false;

  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // necessary to set m_hardforks

  // HF4 requires tests_random_split_strategy (for 2 outputs minimum)
  test_gentime_settings tgts = generator.get_test_gentime_settings();
  tgts.split_strategy = tests_random_split_strategy;
  generator.set_test_gentime_settings(tgts);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK_PARAMS(events, "check_hardfork_inactive", static_cast<size_t>(ZANO_HARDFORK_04_ZARCANUM));

  // tx_0: miner -> Alice
  // make tx_0 before HF4, so Alice will have only bare outs
  transaction tx_0{};
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  for(size_t i = 0; i < 100; ++i)
    destinations.emplace_back(TESTS_DEFAULT_FEE, m_accounts[ALICE_ACC_IDX].get_public_address());
  m_alice_initial_balance = TESTS_DEFAULT_FEE * 100;
  r = fill_tx_sources(sources, events, blk_0r, miner_acc.get_keys(), m_alice_initial_balance + TESTS_DEFAULT_FEE, 0);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_0, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

  ADD_CUSTOM_EVENT(events, tx_0);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  // make sure HF4 has been activated
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_04_ZARCANUM));

  // tx_1: miner -> Alice
  // send less than min fee. This output will be the only ZC UTXO in Alice's wallet
  //destinations.clear();

  MAKE_TX(events, tx_1, miner_acc, alice_acc, TESTS_DEFAULT_FEE * 0.5, blk_1);
  m_alice_initial_balance += TESTS_DEFAULT_FEE * 0.5;
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, tx_1);

  // rewind blocks
  REWIND_BLOCKS_N_WITH_TIME(events, blk_2r, blk_2, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // check Alice's balance and make sure she can deploy an asset
  DO_CALLBACK(events, "c1");

  return true;
}

bool asset_depoyment_and_few_zc_utxos::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  alice_wlt->refresh();

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", m_alice_initial_balance, 0, m_alice_initial_balance, 0, 0), false, "");

  // make sure Alice has correct UTXO wallet structure
  tools::wallet2::transfer_container transfers{};
  alice_wlt->get_transfers(transfers);
  size_t zc_unspent_outs = 0, unspent_outs = 0;
  for(auto& td : transfers)
  {
    if (!td.is_spent())
    {
      ++unspent_outs;
      if (td.is_zc())
        ++zc_unspent_outs;
    }
  }
  CHECK_AND_ASSERT_MES(unspent_outs == 101 && zc_unspent_outs == 1, false, "incorrect UTXO structure: " << unspent_outs << ", " << zc_unspent_outs);

  asset_descriptor_base adb{};
  adb.total_max_supply = 100 * 1000000000000;
  adb.full_name = "very confidential asset";
  adb.ticker = "VCA";
  adb.decimal_point = 12;

  std::vector<tx_destination_entry> destinations;
  destinations.emplace_back(adb.total_max_supply, m_accounts[MINER_ACC_IDX].get_public_address(), null_pkey);
  destinations.emplace_back(adb.total_max_supply / 2, m_accounts[MINER_ACC_IDX].get_public_address(), null_pkey);

  transaction asset_emission_tx{};
  crypto::public_key asset_id = null_pkey;

  alice_wlt->deploy_new_asset(adb, destinations, asset_emission_tx, asset_id);

  // make sure the emission tx is correct
  CHECK_AND_ASSERT_MES(asset_emission_tx.vout.size() > 2, false, "Unexpected vout size: " << asset_emission_tx.vout.size());
  for(auto& out : asset_emission_tx.vout)
  {
    CHECK_AND_ASSERT_MES(out.type() == typeid(tx_out_zarcanum), false, "invalid out type");
    const tx_out_zarcanum& out_zc = boost::get<tx_out_zarcanum>(out);
    // as soon as this is the asset emmiting transaction, no output has an obvious asset id
    // make sure it is so
    CHECK_AND_ASSERT_MES(out_zc.blinded_asset_id != native_coin_asset_id_1div8, false, "One of outputs has explicit native asset id, which is unexpected");
  }

  // get this tx confirmed
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  return true;
}
