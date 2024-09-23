// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "multiassets_test.h"
#include "wallet_test_core_proxy.h"

#include "random_helper.h"
#include "wallet/wallet_debug_events_definitions.h"
#include "wallet/wallet_rpc_server.h"
#include "wallet/wallet_public_structs_defs.h"
using namespace currency;

//------------------------------------------------------------------------------

#define  AMOUNT_TO_TRANSFER_MULTIASSETS_BASIC (TESTS_DEFAULT_FEE)
#define  AMOUNT_ASSETS_TO_TRANSFER_MULTIASSETS_BASIC  500000000000000000

uint64_t multiassets_basic_test::ts_starter = 0;
multiassets_basic_test::multiassets_basic_test()
{
  // TODO: remove the following line
  //LOG_PRINT_MAGENTA("STARTER TS: " << ts_starter, LOG_LEVEL_0);
  //random_state_test_restorer::reset_random(ts_starter);

  REGISTER_CALLBACK_METHOD(multiassets_basic_test, c1);
}

bool multiassets_basic_test::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(ALICE_ACC_IDX+1);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  miner_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  //TODO: Need to make sure REWIND_BLOCKS_N and other coretests codebase are capable of following hardfork4 rules
  //in this test hardfork4 moment moved to runtime section
  //REWIND_BLOCKS_N_WITH_TIME(events, blk_2_inital, blk_0, alice_acc, 2);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  DO_CALLBACK(events, "c1");

  return true;
}

bool multiassets_basic_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<debug_wallet2> miner_wlt = init_playtime_test_wallet_t<debug_wallet2>(events, c, MINER_ACC_IDX);
  miner_wlt->get_account().set_createtime(0);
  miner_wlt->refresh();

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->get_account().set_createtime(0);

  asset_descriptor_base adb = AUTO_VAL_INIT(adb);
  adb.total_max_supply = 10000000000000000000ULL; //1M coins
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
  CHECK_AND_ASSERT_MES(miner_wlt->balance(asset_id) == AMOUNT_ASSETS_TO_TRANSFER_MULTIASSETS_BASIC, false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(miner_wlt->balance() == uint64_t(17517225990000000000ULL), false, "Failed to find needed asset in result balances");

  CHECK_AND_ASSERT_MES(alice_wlt->balance() == 0, false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(alice_wlt->balance(asset_id) == AMOUNT_ASSETS_TO_TRANSFER_MULTIASSETS_BASIC, false, "Failed to find needed asset in result balances");
  
  miner_wlt->transfer(AMOUNT_ASSETS_TO_TRANSFER_MULTIASSETS_BASIC/2, alice_wlt->get_account().get_public_address(), asset_id);
  //pass over hardfork
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  alice_wlt->refresh();
  uint64_t last_alice_balances = alice_wlt->balance(asset_id);
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
  uint64_t last_miner_balance = miner_wlt->balance(asset_id);


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
  r = mine_next_pow_blocks_in_playtime(alice_wlt->get_account().get_public_address(), c, 2);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  asset_descriptor_base asset_info2 = AUTO_VAL_INIT(asset_info2);
  r = c.get_blockchain_storage().get_asset_info(asset_id, asset_info2);
  CHECK_AND_ASSERT_MES(r, false, "Failed to get_asset_info");

  CHECK_AND_ASSERT_MES(asset_info2.meta_info == asset_info.meta_info, false, "Failed to find needed asset in result balances");

  //test emit function
  //use same destinations as we used before
  miner_wlt->emit_asset(asset_id, destinations, tx);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  miner_wlt->refresh();
  alice_wlt->refresh();
  CHECK_AND_ASSERT_MES(miner_wlt->balance(asset_id) == last_miner_balance + destinations[0].amount, false, "Miner balance wrong");
  CHECK_AND_ASSERT_MES(alice_wlt->balance(asset_id) == last_alice_balances + destinations[1].amount, false, "Alice balance wrong");

  asset_descriptor_base asset_info3 = AUTO_VAL_INIT(asset_info3);
  r = c.get_blockchain_storage().get_asset_info(asset_id, asset_info3);
  CHECK_AND_ASSERT_MES(r, false, "Failed to get_asset_info");
  CHECK_AND_ASSERT_MES(asset_info3.current_supply == asset_info2.current_supply + destinations[1].amount + destinations[0].amount, false, "Failed to find needed asset in result balances");



  miner_wlt->burn_asset(asset_id, last_miner_balance, tx);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 1);

  miner_wlt->refresh();
  CHECK_AND_ASSERT_MES(miner_wlt->balance(asset_id) == destinations[0].amount, false, "Miner balance wrong");

  asset_descriptor_base asset_info4 = AUTO_VAL_INIT(asset_info4);
  r = c.get_blockchain_storage().get_asset_info(asset_id, asset_info4);
  CHECK_AND_ASSERT_MES(r, false, "Failed to get_asset_info");
  CHECK_AND_ASSERT_MES(asset_info4.current_supply == asset_info3.current_supply - last_miner_balance, false, "Failed to find needed asset in result balances");


  //------------------- tests that trying to break stuff  -------------------
  //tests that trying to break stuff
  miner_wlt->get_debug_events_dispatcher().SUBSCIRBE_DEBUG_EVENT<wde_construct_tx_after_asset_ownership_proof_generated>([&](const wde_construct_tx_after_asset_ownership_proof_generated& o)
  {
    //crypto::signature s = currency::null_sig;
    o.pownership_proof->gss = crypto::generic_schnorr_sig_s{};
  });


  //test update function with broken ownership
  r = c.get_blockchain_storage().get_asset_info(asset_id, asset_info);
  CHECK_AND_ASSERT_MES(r, false, "Failed to get_asset_info");

  asset_info.meta_info = "{\"some2\": \"info2\"}";
  r = false;
  try
  {
    miner_wlt->update_asset(asset_id, asset_info, tx);
  }
  catch(tools::error::tx_rejected&)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "Test failed, broken ownership passed");
  
  c.get_tx_pool().unsecure_disable_tx_validation_on_addition(true);
  miner_wlt->update_asset(asset_id, asset_info, tx);
  c.get_tx_pool().unsecure_disable_tx_validation_on_addition(false);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 2);
  CHECK_AND_ASSERT_MES(!r, false, "Test failed, broken ownership passed");
  c.get_tx_pool().purge_transactions();
  miner_wlt->refresh();

  miner_wlt->get_debug_events_dispatcher().UNSUBSCRIBE_ALL();

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  //------------------- tests that trying to break stuff  -------------------
  // check update_asset() with modified 'ticker'
  r = c.get_blockchain_storage().get_asset_info(asset_id, asset_info);
  CHECK_AND_ASSERT_MES(r, false, "Failed to get_asset_info");

  asset_info.ticker = "XXX";
  r = false;
  try
  {
    miner_wlt->update_asset(asset_id, asset_info, tx);
  }
  catch(tools::error::tx_rejected&)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "update_asset succeeded, but this shouldn't happened");

  c.get_tx_pool().unsecure_disable_tx_validation_on_addition(true);
  miner_wlt->update_asset(asset_id, asset_info, tx);
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(miner_wlt->get_account().get_public_address(), c);
  CHECK_AND_ASSERT_MES(!r, false, "block with a bad tx was unexpectedly mined");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count()); // make sure tx was not confirmed
  c.get_tx_pool().unsecure_disable_tx_validation_on_addition(false);
  c.get_tx_pool().purge_transactions();

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count()); // make sure tx was not added

  
  // check update_asset() with modified 'full_name'
  r = c.get_blockchain_storage().get_asset_info(asset_id, asset_info);
  CHECK_AND_ASSERT_MES(r, false, "Failed to get_asset_info");

  asset_info.full_name = "XXX";
  r = false;
  try
  {
    miner_wlt->update_asset(asset_id, asset_info, tx);
  }
  catch(tools::error::tx_rejected&)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "update_asset succeeded, but this shouldn't happened");

  c.get_tx_pool().unsecure_disable_tx_validation_on_addition(true);
  miner_wlt->update_asset(asset_id, asset_info, tx);
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(miner_wlt->get_account().get_public_address(), c);
  CHECK_AND_ASSERT_MES(!r, false, "block with a bad tx was unexpectedly mined");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count()); // make sure tx was not confirmed
  c.get_tx_pool().unsecure_disable_tx_validation_on_addition(false);
  c.get_tx_pool().purge_transactions();
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count()); // make sure tx was not added
  miner_wlt->refresh();

  
  // check update_asset() with modified 'decimal_point'
  r = c.get_blockchain_storage().get_asset_info(asset_id, asset_info);
  CHECK_AND_ASSERT_MES(r, false, "Failed to get_asset_info");

  asset_info.decimal_point = 3;
  r = false;
  try
  {
    miner_wlt->update_asset(asset_id, asset_info, tx);
  }
  catch(tools::error::tx_rejected&)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "update_asset succeeded, but this shouldn't happened");

  c.get_tx_pool().unsecure_disable_tx_validation_on_addition(true);
  miner_wlt->update_asset(asset_id, asset_info, tx);
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(miner_wlt->get_account().get_public_address(), c);
  CHECK_AND_ASSERT_MES(!r, false, "block with a bad tx was unexpectedly mined");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count()); // make sure tx was not confirmed
  c.get_tx_pool().unsecure_disable_tx_validation_on_addition(false);
  c.get_tx_pool().purge_transactions();
  miner_wlt->refresh();
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count()); // make sure tx was not added


  // check update_asset() with modified 'owner'
  //r = c.get_blockchain_storage().get_asset_info(asset_id, asset_info);
  //CHECK_AND_ASSERT_MES(r, false, "Failed to get_asset_info");

  //asset_info.owner = currency::keypair::generate().pub;
  //miner_wlt->update_asset(asset_id, asset_info, tx);
  //CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());
  //r = mine_next_pow_block_in_playtime(miner_wlt->get_account().get_public_address(), c);
  //CHECK_AND_ASSERT_MES(!r, false, "block with a bad tx was unexpectedly mined");
  //CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count()); // make sure tx was not confirmed
  //c.get_tx_pool().purge_transactions();
  //miner_wlt->refresh();


  // check emit_asset() with modified 'current_supply'
  miner_wlt->get_debug_events_dispatcher().SUBSCIRBE_DEBUG_EVENT<wde_construct_tx_handle_asset_descriptor_operation_before_seal>([&](const wde_construct_tx_handle_asset_descriptor_operation_before_seal& o)
  {
    o.pado->descriptor.current_supply += 1000000;
  });
  //test emit function but re-adjust current_supply to wrong amount
  r = false;
  try
  {
    miner_wlt->emit_asset(asset_id, destinations, tx);
  }
  catch(tools::error::tx_rejected&)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "emit_asset succeeded, but this shouldn't happened");

  c.get_tx_pool().unsecure_disable_tx_validation_on_addition(true);
  miner_wlt->emit_asset(asset_id, destinations, tx);
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(miner_wlt->get_account().get_public_address(), c);
  CHECK_AND_ASSERT_MES(!r, false, "block with a bad tx was unexpectedly mined");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count()); // make sure tx was not confirmed
  c.get_tx_pool().unsecure_disable_tx_validation_on_addition(false);
  c.get_tx_pool().purge_transactions();
  miner_wlt->refresh();

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count()); // make sure tx was not added

  //------------------- tests that trying to break stuff  -------------------
  //test burn that burns more than tx has
  miner_wlt->get_debug_events_dispatcher().UNSUBSCRIBE_ALL();

  miner_wlt->get_debug_events_dispatcher().SUBSCIRBE_DEBUG_EVENT<wde_construct_tx_handle_asset_descriptor_operation_before_burn>([&](const wde_construct_tx_handle_asset_descriptor_operation_before_burn& o)
  {
    o.pado->descriptor.current_supply -= 1000000;
  });


  r = false;
  try
  {
    miner_wlt->burn_asset(asset_id, 10000000000000, tx);
  }
  catch(tools::error::tx_rejected&)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "burn_asset succeeded, but this shouldn't happened");

  c.get_tx_pool().unsecure_disable_tx_validation_on_addition(true);
  miner_wlt->burn_asset(asset_id, 10000000000000, tx);
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(miner_wlt->get_account().get_public_address(), c);
  CHECK_AND_ASSERT_MES(!r, false, "block with a bad tx was unexpectedly mined");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count()); // make sure tx was not confirmed
  c.get_tx_pool().unsecure_disable_tx_validation_on_addition(false);
  c.get_tx_pool().purge_transactions();
  miner_wlt->refresh();
  miner_wlt->get_debug_events_dispatcher().UNSUBSCRIBE_ALL();

  //
  miner_wlt->transfer_asset_ownership(asset_id, alice_wlt->get_account().get_public_address().spend_public_key, tx);
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(miner_wlt->get_account().get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "block with a bad tx was unexpectedly mined");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count()); // make sure tx was not confirmed

  miner_wlt->refresh();
  alice_wlt->refresh();

  auto miner_own_assets = miner_wlt->get_own_assets();
  auto alice_own_assets = alice_wlt->get_own_assets();
  CHECK_AND_ASSERT_MES(miner_own_assets.size() == 0, false, "Miner wlt still think he own asset");
  CHECK_AND_ASSERT_MES(alice_own_assets.size() == 1, false, "Alice still don't know she own asset");

  //uint64_t balance_alice_native = alice_wlt->balance();
  //uint64_t balance_miner_native = miner_wlt->balance();
  uint64_t balance_alice_asset = alice_wlt->balance(asset_id);
  uint64_t balance_miner_asset = miner_wlt->balance(asset_id);

  alice_wlt->emit_asset(asset_id, destinations, tx);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  miner_wlt->refresh();
  alice_wlt->refresh();

  CHECK_AND_ASSERT_MES(miner_wlt->balance(asset_id) == balance_miner_asset + destinations[0].amount, false, "Miner balance wrong");
  CHECK_AND_ASSERT_MES(alice_wlt->balance(asset_id) == balance_alice_asset + destinations[1].amount, false, "Alice balance wrong");

  //TODO: attempt to emmmit from old key, attempt to emit from more then max supply

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

  // make sure that all tx_1 outputs have explicit native coin asset id
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
  tools::transfer_container transfers{};
  alice_wlt->get_transfers(transfers);
  size_t unspent_transfers = std::count_if(transfers.begin(), transfers.end(), [](const tools::transfer_details& tr){ return !tr.is_spent(); });
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
  tools::transfer_container transfers{};
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

//------------------------------------------------------------------------------

assets_and_pos_mining::assets_and_pos_mining()
{
  REGISTER_CALLBACK_METHOD(assets_and_pos_mining, c1);
}

bool assets_and_pos_mining::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: ensure that post-HF4 Zarcanum staking functions correctly with outputs that have a nonzero asset id blinding mask (i.e., outputs with a non-explicit asset id)

  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // necessary to set m_hardforks

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_04_ZARCANUM));

  DO_CALLBACK(events, "c1");
  return true;
}

bool assets_and_pos_mining::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  miner_wlt->refresh();

  asset_descriptor_base adb{};
  adb.total_max_supply = 10*COIN;
  adb.full_name = "test";
  adb.ticker = "TEST";

  std::vector<currency::tx_destination_entry> destinations;
  destinations.emplace_back(adb.total_max_supply, m_accounts[ALICE_ACC_IDX].get_public_address(), null_pkey);
  destinations.emplace_back(MK_TEST_COINS(2), m_accounts[ALICE_ACC_IDX].get_public_address());
  destinations.emplace_back(MK_TEST_COINS(2), m_accounts[ALICE_ACC_IDX].get_public_address());

  currency::transaction tx{};
  crypto::public_key asset_id = currency::null_pkey;
  miner_wlt->deploy_new_asset(adb, destinations, tx, asset_id);
  LOG_PRINT_L0("Deployed new asset: " << asset_id << ", tx_id: " << currency::get_transaction_hash(tx));

  CHECK_AND_ASSERT_MES(tx.vout.size() >= 3, false, "Unexpected vout size: " << tx.vout.size());
  for(auto& out : tx.vout)
  {
    CHECK_AND_ASSERT_MES(out.type() == typeid(tx_out_zarcanum), false, "invalid out type");
    CHECK_AND_ASSERT_MES(boost::get<tx_out_zarcanum>(out).blinded_asset_id != native_coin_asset_id_1div8, false, "One of outputs has explicit native asset id, which is unexpected");
  }

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", MK_TEST_COINS(4), 0, MK_TEST_COINS(4), 0, 0), false, "");

  size_t current_blockchain_size = c.get_current_blockchain_size();

  r = alice_wlt->try_mint_pos();
  CHECK_AND_ASSERT_MES(r, false, "try_mint_pos failed");
  CHECK_AND_ASSERT_MES(c.get_current_blockchain_size() == current_blockchain_size + 1, false, "incorrect blockchain size: " << c.get_current_blockchain_size());

  r = alice_wlt->try_mint_pos();
  CHECK_AND_ASSERT_MES(r, false, "try_mint_pos failed");
  CHECK_AND_ASSERT_MES(c.get_current_blockchain_size() == current_blockchain_size + 2, false, "incorrect blockchain size: " << c.get_current_blockchain_size());

  // the following attempt should fail because Alice has only two outputs eligiable for staking
  r = alice_wlt->try_mint_pos();
  CHECK_AND_ASSERT_MES(r, false, "try_mint_pos failed");
  CHECK_AND_ASSERT_MES(c.get_current_blockchain_size() == current_blockchain_size + 2, false, "incorrect blockchain size: " << c.get_current_blockchain_size()); // the same height

  return true;
}

//------------------------------------------------------------------------------

asset_emission_and_unconfirmed_balance::asset_emission_and_unconfirmed_balance()
{
  REGISTER_CALLBACK_METHOD(asset_emission_and_unconfirmed_balance, c1);
}

bool asset_emission_and_unconfirmed_balance::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  //account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  miner_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  DO_CALLBACK(events, "c1");

  return true;
}

bool asset_emission_and_unconfirmed_balance::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  miner_wlt->refresh();

  asset_descriptor_base adb{};
  adb.total_max_supply = UINT64_MAX;
  adb.full_name = "2**64";
  adb.ticker = "2POWER64";

  std::vector<currency::tx_destination_entry> destinations;
  destinations.emplace_back(adb.total_max_supply, m_accounts[MINER_ACC_IDX].get_public_address(), null_pkey);

  currency::transaction tx{};
  crypto::public_key asset_id = currency::null_pkey;
  miner_wlt->deploy_new_asset(adb, destinations, tx, asset_id);
  LOG_PRINT_L0("Deployed new asset: " << asset_id << ", tx_id: " << currency::get_transaction_hash(tx));

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  bool stub_bool = 0;
  miner_wlt->refresh();
  miner_wlt->scan_tx_pool(stub_bool);
  uint64_t total, unlocked, awaiting_in, awaiting_out, mined;
  balance_via_wallet(*miner_wlt, asset_id, &total, &unlocked, &awaiting_in, &awaiting_out, &mined);
  CHECK_AND_ASSERT_EQ(total,        UINT64_MAX);
  CHECK_AND_ASSERT_EQ(unlocked,     0);
  CHECK_AND_ASSERT_EQ(awaiting_in,  UINT64_MAX);
  CHECK_AND_ASSERT_EQ(awaiting_out, 0);
  //CHECK_AND_ASSERT_EQ(mined,        0);

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  return true;
}

//------------------------------------------------------------------------------

eth_signed_asset_basics::eth_signed_asset_basics()
{
  REGISTER_CALLBACK_METHOD(eth_signed_asset_basics, c1);
}

bool eth_signed_asset_basics::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  //account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  miner_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  DO_CALLBACK(events, "c1");

  return true;
}

template<typename T>
struct eth_sign_cb_helper : public currency::asset_eth_signer_i
{
  eth_sign_cb_helper(T cb)
    : m_cb(cb)
  {}

  virtual bool sign(const crypto::hash& h, const crypto::eth_public_key& asset_owner, crypto::eth_signature& sig) override
  {
    return m_cb(h, asset_owner, sig);
  }

  T m_cb;
};

template<typename T>
std::shared_ptr<eth_sign_cb_helper<T>> construct_eth_sign_cb_helper(T&& cb)
{
  return std::make_shared<eth_sign_cb_helper<T>>(std::forward<T>(cb));
}

bool eth_signed_asset_basics::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  
  crypto::eth_secret_key eth_sk{};
  crypto::eth_public_key eth_pk{};
  r = crypto::generate_eth_key_pair(eth_sk, eth_pk);
  CHECK_AND_ASSERT_MES(r, false, "generate_eth_key_pair failed");

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  miner_wlt->refresh();

  asset_descriptor_base adb{};
  adb.decimal_point = 2;
  adb.total_max_supply = 10000;
  adb.full_name = "Either";
  adb.ticker = "EITH";
  adb.owner_eth_pub_key = eth_pk; // note setting owner eth pub key here

  uint64_t initial_emit_amount = adb.total_max_supply / 2;

  //std::vector<currency::tx_destination_entry> destinations;
  //destinations.emplace_back(initial_emit_amount, m_accounts[MINER_ACC_IDX].get_public_address(), null_pkey);
  //currency::transaction tx{};

  //
  // custom deploy
  //
  tools::construct_tx_param ctp = miner_wlt->get_default_construct_tx_param();
  ctp.dsts.emplace_back(initial_emit_amount, m_accounts[MINER_ACC_IDX].get_public_address(), null_pkey);
  // TODO reconsider this =>>> // ctp.asset_owner = eth_pk; // note using eth pub key here
  ctp.need_at_least_1_zc = true;

  asset_descriptor_operation ado{};
  ado.descriptor = adb;
  ado.operation_type = ASSET_DESCRIPTOR_OPERATION_REGISTER;
  ctp.extra.push_back(ado);

  finalized_tx ft{};
  miner_wlt->transfer(ctp, ft, true, nullptr);

  crypto::public_key asset_id = currency::null_pkey;
  CHECK_AND_ASSERT_MES(get_or_calculate_asset_id(ado, nullptr, &asset_id), false, "get_or_calculate_asset_id failed");

  currency::asset_descriptor_operation ado2{};
  r = get_type_in_variant_container(ft.tx.extra, ado2);
  CHECK_AND_ASSERT_MES(r, false, "Failed find asset info in tx");
  crypto::public_key new_asset_id{};
  CHECK_AND_ASSERT_MES(get_or_calculate_asset_id(ado, nullptr, &new_asset_id), false, "get_or_calculate_asset_id failed");


  miner_wlt->add_custom_asset_id(asset_id, adb);
  //
  // end of custom deploy
  //

  const transaction& tx = ft.tx;


  LOG_PRINT_L0("Deployed new asset: " << asset_id << ", tx_id: " << currency::get_transaction_hash(tx));
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  uint64_t emit_amount = adb.total_max_supply - initial_emit_amount;

  //
  // custom emit
  //
  ctp = miner_wlt->get_default_construct_tx_param();
  ctp.dsts.emplace_back(emit_amount, m_accounts[MINER_ACC_IDX].get_public_address(), null_pkey);
  ctp.need_at_least_1_zc = true;
  // TODO reconsider this =>> ctp.asset_owner = eth_pk; // note using eth pub key here

  ado = asset_descriptor_operation{};
  ado.descriptor = adb;
  ado.descriptor.current_supply = initial_emit_amount;
  ado.operation_type = ASSET_DESCRIPTOR_OPERATION_EMIT;
  ado.opt_asset_id = asset_id;
  ctp.extra.push_back(ado);

  auto signer = construct_eth_sign_cb_helper([&](const crypto::hash& h, const crypto::eth_public_key& asset_owner, crypto::eth_signature& sig)->bool {
    CHECK_AND_ASSERT_EQ(asset_owner, eth_pk);
    return crypto::generate_eth_signature(h, eth_sk, sig);
  });

  // TODO reconsider this ===>> ctp.p_eth_signer = signer.get();

  ft = finalized_tx{};
  miner_wlt->transfer(ctp, ft, true, nullptr);
  //
  // end of custom emit
  //

  return true;
}

//------------------------------------------------------------------------------

eth_signed_asset_via_rpc::eth_signed_asset_via_rpc()
{
  REGISTER_CALLBACK_METHOD(eth_signed_asset_via_rpc, c1);
}

bool eth_signed_asset_via_rpc::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);
  miner_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  DO_CALLBACK(events, "c1");

  return true;
}

bool eth_signed_asset_via_rpc::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;

  crypto::eth_secret_key eth_sk{};
  crypto::eth_public_key eth_pk{};
  r = crypto::generate_eth_key_pair(eth_sk, eth_pk);
  CHECK_AND_ASSERT_MES(r, false, "generate_eth_key_pair failed");

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  miner_wlt->refresh();
  alice_wlt->refresh();

  // wallet RPC server
  tools::wallet_rpc_server miner_wlt_rpc(miner_wlt);
  epee::json_rpc::error jerr{};
  tools::wallet_rpc_server::connection_context ctx{};

  // core RPC server
  currency::t_currency_protocol_handler<currency::core> m_cprotocol(c, nullptr);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > p2p(m_cprotocol);
  bc_services::bc_offers_service of(nullptr);
  currency::core_rpc_server core_rpc_wrapper(c, p2p, of);
  core_rpc_wrapper.set_ignore_connectivity_status(true);

  // asset description
  asset_descriptor_base adb{};
  adb.decimal_point = 2;
  adb.total_max_supply = 45000;
  adb.full_name = "P450";
  adb.ticker = "P450";
  adb.owner_eth_pub_key = eth_pk; // note setting owner eth pub key here

  uint64_t initial_register_amount = 10000;

  // 1. Miner deploys initial amount of the asset (all go to Alice)
  // deploy operation don't require eth proof and therefore the corresponding tx will be generated and added to the tx pool as usual

  tools::wallet_public::COMMAND_ASSETS_DEPLOY::request deploy_req{};
  deploy_req.asset_descriptor = adb;
  deploy_req.destinations.push_back(tools::wallet_public::transfer_destination{initial_register_amount, m_accounts[ALICE_ACC_IDX].get_public_address_str(), null_pkey});
  deploy_req.do_not_split_destinations = false;
  tools::wallet_public::COMMAND_ASSETS_DEPLOY::response deploy_resp{};
  r = miner_wlt_rpc.on_asset_deploy(deploy_req, deploy_resp, jerr, ctx);
  CHECK_AND_ASSERT_MES(r, false, "RPC on_asset_deploy failed");

  const crypto::public_key asset_id = deploy_resp.new_asset_id;
  LOG_PRINT_GREEN_L0("Asset " << asset_id << " was successfully deployed with tx " << deploy_resp.tx_id);

  // make sure tx was added to the pool
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  // confirm the tx with a block
  CHECK_AND_ASSERT_MES(mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c), false, "");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  size_t blocks_fetched = 0;
  miner_wlt->refresh(blocks_fetched);
  CHECK_AND_ASSERT_EQ(blocks_fetched, 1);

  // Alice checks her asset balance
  alice_wlt->refresh(blocks_fetched);
  CHECK_AND_ASSERT_EQ(blocks_fetched, 1);
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", initial_register_amount, asset_id, adb.decimal_point), false, "");


  //
  // 2. Miner emits additional amount of the asset and transfers it to Alice
  //
  uint64_t additional_emit_amount = 20000;
  uint64_t total_asset_amount = initial_register_amount + additional_emit_amount;

  tools::wallet_public::COMMAND_ASSETS_EMIT::request emit_req{};
  emit_req.asset_id = asset_id;
  emit_req.destinations.push_back(tools::wallet_public::transfer_destination{additional_emit_amount, m_accounts[ALICE_ACC_IDX].get_public_address_str(), asset_id});
  emit_req.do_not_split_destinations = false;

  tools::wallet_public::COMMAND_ASSETS_EMIT::response emit_resp{};
  r = miner_wlt_rpc.on_asset_emit(emit_req, emit_resp, jerr, ctx);
  CHECK_AND_ASSERT_MES(r, false, "RPC on_asset_emit failed: " << jerr.message);

  // make sure tx was NOT added to the pool (because it's only partially signed)
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  // unserialize transaction that we got from RPC
  transaction emit_tx{};
  CHECK_AND_ASSERT_MES(emit_resp.data_for_external_signing.has_value(), false, "data_for_external_signing has no value");
  r = t_unserializable_object_from_blob(emit_tx, emit_resp.data_for_external_signing->unsigned_tx);
  CHECK_AND_ASSERT_MES(r, false, "t_unserializable_object_from_blob failed");
  CHECK_AND_ASSERT_EQ(get_transaction_hash(emit_tx), emit_resp.tx_id);

  // make sure emit transaction cannot be added to the transaction pool (it's not fully signed atm)
  tx_verification_context tvc{};
  r = c.get_tx_pool().add_tx(emit_tx, tvc, false);
  CHECK_AND_ASSERT_MES(!r, false, "emit tx was able to be added to the pool");

  //
  // decrypt emission transaction outputs prior to ETH signing to make sure it's valid
  //
  currency::COMMAND_RPC_DECRYPT_TX_DETAILS::request decrypt_req{};
  decrypt_req.tx_secret_key = emit_resp.data_for_external_signing->tx_secret_key;
  decrypt_req.tx_blob = epee::string_encoding::base64_encode(emit_resp.data_for_external_signing->unsigned_tx);
  // note: decrypt_req.outputs_addresses can be populated using emit_resp.data_for_external_signing->outputs_addresses but we fill it manually here
  decrypt_req.outputs_addresses.push_back(m_accounts[MINER_ACC_IDX].get_public_address_str()); // we expect that the first output is the cashback and addressed to miner
  for(size_t i = 0, size = emit_tx.vout.size() - 1; i < size; ++i)
    decrypt_req.outputs_addresses.push_back(m_accounts[ALICE_ACC_IDX].get_public_address_str()); // we expect all other outputs are asset emission and addresses to Alice
  currency::COMMAND_RPC_DECRYPT_TX_DETAILS::response decrypt_resp{};
  r = core_rpc_wrapper.on_decrypt_tx_details(decrypt_req, decrypt_resp, jerr, ctx);
  CHECK_AND_ASSERT_MES(r, false, "RPC on_decrypt_tx_details failed: " << jerr.message);
  // make sure that verified_tx_id is the one we expect
  CHECK_AND_ASSERT_EQ(decrypt_resp.verified_tx_id, emit_resp.tx_id);

  // after a successfull tx outputs decryption, examine them
  CHECK_AND_ASSERT_EQ(decrypt_resp.decoded_outputs.size(), emit_tx.vout.size());
  uint64_t decrypted_emission_sum = 0;
  for(auto& el : decrypt_resp.decoded_outputs)
  {
    if (el.asset_id == asset_id)
      decrypted_emission_sum += el.amount;
    else
      CHECK_AND_ASSERT_EQ(el.asset_id, native_coin_asset_id);
  }
  // make sure the transaction emits the expected amount of asset
  CHECK_AND_ASSERT_EQ(decrypted_emission_sum, additional_emit_amount);

  LOG_PRINT_YELLOW(decrypt_resp.tx_in_json, LOG_LEVEL_0); // tx with still missing ownership proof

  //
  // as everything is allright, sign emit_tx with ETH signature.
  //
  crypto::eth_signature eth_sig{};
  crypto::generate_eth_signature(emit_resp.tx_id, eth_sk, eth_sig);
  // instant verification, just in case
  r = crypto::verify_eth_signature(emit_resp.tx_id, eth_pk, eth_sig);
  CHECK_AND_ASSERT_MES(r, false, "verify_eth_signature failed");

  //
  // send ETH signature alogn with all previous data to a wallet RPC call for final tx assembling and broadcasting
  //
  tools::wallet_public::COMMAND_ASSET_SEND_EXT_SIGNED_TX::request send_signed_req{};
  send_signed_req.unsigned_tx     = emit_resp.data_for_external_signing->unsigned_tx;
  send_signed_req.eth_sig         = eth_sig;
  send_signed_req.expected_tx_id  = decrypt_resp.verified_tx_id;
  send_signed_req.finalized_tx    = emit_resp.data_for_external_signing->finalized_tx;
  send_signed_req.unlock_transfers_on_fail = true;
  tools::wallet_public::COMMAND_ASSET_SEND_EXT_SIGNED_TX::response send_signed_resp{};
  r = miner_wlt_rpc.on_asset_send_ext_signed_tx(send_signed_req, send_signed_resp, jerr, ctx);
  CHECK_AND_ASSERT_MES(r, false, "RPC send_ext_signed_asset_tx failed: " << jerr.message);

  // make sure tx was broadcasted
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  // confirm the tx with a block
  CHECK_AND_ASSERT_MES(mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c), false, "");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  // Alice checks her asset balance
  alice_wlt->refresh(blocks_fetched);
  CHECK_AND_ASSERT_EQ(blocks_fetched, 1);
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", total_asset_amount, asset_id, adb.decimal_point), false, "");


  //
  // 3. Miner transfer ownership of the asset to another ETH key (#2)
  //
  crypto::eth_secret_key eth_sk_2{};
  crypto::eth_public_key eth_pk_2{};
  r = crypto::generate_eth_key_pair(eth_sk_2, eth_pk_2);
  CHECK_AND_ASSERT_MES(r, false, "generate_eth_key_pair failed");

  tools::wallet_public::COMMAND_ASSETS_UPDATE::request to_req{}; // 'to' means transfer of ownership
  to_req.asset_id = asset_id;
  {
    // request the most recent asset info from core to correctly fill to_req
    currency::COMMAND_RPC_GET_ASSET_INFO::request req{};
    req.asset_id = asset_id;
    currency::COMMAND_RPC_GET_ASSET_INFO::response resp{};
    CHECK_AND_ASSERT_MES(core_rpc_wrapper.on_get_asset_info(req, resp, ctx), false, "on_get_asset_info failed");
    to_req.asset_descriptor = resp.asset_descriptor;
  }
  to_req.asset_descriptor.owner_eth_pub_key = eth_pk_2; // new owner, note using another ETH pub key
  to_req.asset_descriptor.meta_info = "owner: eth_pk_2"; // it's also possible to change meta_info with this update operation
  tools::wallet_public::COMMAND_ASSETS_UPDATE::response to_resp{};
  r = miner_wlt_rpc.on_asset_update(to_req, to_resp, jerr, ctx);
  CHECK_AND_ASSERT_MES(r, false, "RPC send_ext_signed_asset_tx failed: " << jerr.message);

  // make sure tx was NOT added to the pool (because it's only partially signed)
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  CHECK_AND_ASSERT_MES(to_resp.data_for_external_signing.has_value(), false, "data_for_external_signing has no value");
  transaction to_tx{};
  r = t_unserializable_object_from_blob(to_tx, to_resp.data_for_external_signing->unsigned_tx);
  CHECK_AND_ASSERT_MES(r, false, "t_unserializable_object_from_blob failed");
  CHECK_AND_ASSERT_EQ(get_transaction_hash(to_tx), to_resp.tx_id);

  // make sure ownership transferring transaction cannot be added to the transaction pool (it's not fully signed atm)
  tvc = tx_verification_context{};
  r = c.get_tx_pool().add_tx(to_tx, tvc, false);
  CHECK_AND_ASSERT_MES(!r, false, "ownership transferring tx was able to be added to the pool");

  //
  // decrypt ownership transfer transaction prior to ETH signing to make sure it's valid
  //
  decrypt_req = currency::COMMAND_RPC_DECRYPT_TX_DETAILS::request{};
  decrypt_req.tx_secret_key = to_resp.data_for_external_signing->tx_secret_key;
  decrypt_req.tx_blob = epee::string_encoding::base64_encode(to_resp.data_for_external_signing->unsigned_tx);
  // note: decrypt_req.outputs_addresses can be populated using to_resp.data_for_external_signing->outputs_addresses but we fill it manually here
  for(size_t i = 0, size = to_tx.vout.size(); i < size; ++i)
    decrypt_req.outputs_addresses.push_back(m_accounts[MINER_ACC_IDX].get_public_address_str()); // we expect all outputs goes to Miner
  decrypt_resp = currency::COMMAND_RPC_DECRYPT_TX_DETAILS::response{};
  r = core_rpc_wrapper.on_decrypt_tx_details(decrypt_req, decrypt_resp, jerr, ctx);
  CHECK_AND_ASSERT_MES(r, false, "RPC on_decrypt_tx_details failed: " << jerr.message);
  // make sure that verified_tx_id is the one we expect
  CHECK_AND_ASSERT_EQ(decrypt_resp.verified_tx_id, to_resp.tx_id);

  // after a successfull tx outputs decryption, examine them
  CHECK_AND_ASSERT_EQ(decrypt_resp.decoded_outputs.size(), to_tx.vout.size());
  uint64_t asset_sum = 0;
  for(auto& el : decrypt_resp.decoded_outputs)
  {
    if (el.asset_id != native_coin_asset_id)
      asset_sum += el.amount;
  }
  // make sure the transaction don't send assets
  CHECK_AND_ASSERT_EQ(asset_sum, 0);

  // make sure this is an ownership transfer transaction and the ownership is correctly transferred:
  // Note: this check could also be done by examination of decrypt_resp.tx_in_json
  asset_descriptor_operation* pado = get_type_in_variant_container<asset_descriptor_operation>(to_tx.extra);
  CHECK_AND_ASSERT_NEQ(pado, 0);
  CHECK_AND_ASSERT_EQ(pado->operation_type, ASSET_DESCRIPTOR_OPERATION_UPDATE);
  CHECK_AND_ASSERT_EQ(pado->opt_asset_id.has_value(), true);
  CHECK_AND_ASSERT_EQ(pado->opt_asset_id.get(), asset_id);
  CHECK_AND_ASSERT_EQ(pado->descriptor.owner_eth_pub_key.has_value(), true);
  CHECK_AND_ASSERT_EQ(pado->descriptor.owner_eth_pub_key.get(), eth_pk_2); // the most important condition for an ownership transfer
  // other fileds of pado->descriptor may also be checked here

  //
  // as everything is allright, sign to_tx with ETH signature.
  //
  crypto::eth_signature to_eth_sig{};
  crypto::generate_eth_signature(to_resp.tx_id, eth_sk, to_eth_sig); // note using old ETH secret key here, because this tx must be signed with the original owner
  // instant verification, just in case
  r = crypto::verify_eth_signature(to_resp.tx_id, eth_pk, to_eth_sig);
  CHECK_AND_ASSERT_MES(r, false, "verify_eth_signature failed");

  //
  // send ETH signature along with all previous data to a wallet RPC call for final tx assembling and broadcasting
  //
  send_signed_req = tools::wallet_public::COMMAND_ASSET_SEND_EXT_SIGNED_TX::request{};
  send_signed_req.unsigned_tx     = to_resp.data_for_external_signing->unsigned_tx;
  send_signed_req.eth_sig         = to_eth_sig;
  send_signed_req.expected_tx_id  = decrypt_resp.verified_tx_id;
  send_signed_req.finalized_tx    = to_resp.data_for_external_signing->finalized_tx;
  send_signed_req.unlock_transfers_on_fail = true;
  send_signed_resp = tools::wallet_public::COMMAND_ASSET_SEND_EXT_SIGNED_TX::response{};
  r = miner_wlt_rpc.on_asset_send_ext_signed_tx(send_signed_req, send_signed_resp, jerr, ctx);
  CHECK_AND_ASSERT_MES(r, false, "RPC send_ext_signed_asset_tx failed: " << jerr.message);

  // make sure tx was broadcasted
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  // confirm the tx with a block
  CHECK_AND_ASSERT_MES(mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c), false, "");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  // Alice checks her asset balance, it shouldn't change
  alice_wlt->refresh(blocks_fetched);
  CHECK_AND_ASSERT_EQ(blocks_fetched, 1);
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", total_asset_amount, asset_id, adb.decimal_point), false, "");

  miner_wlt->refresh();

  //
  // 4. Miner emits additional amount of the asset (signing it with ETH key #2) and transfers it to Bob
  //
  additional_emit_amount = 15000;
  uint64_t alice_amount = total_asset_amount;
  total_asset_amount += additional_emit_amount;

  emit_req = tools::wallet_public::COMMAND_ASSETS_EMIT::request{};
  emit_req.asset_id = asset_id;
  emit_req.destinations.push_back(tools::wallet_public::transfer_destination{additional_emit_amount, m_accounts[BOB_ACC_IDX].get_public_address_str(), asset_id});
  emit_req.do_not_split_destinations = false;

  emit_resp = tools::wallet_public::COMMAND_ASSETS_EMIT::response{};
  r = miner_wlt_rpc.on_asset_emit(emit_req, emit_resp, jerr, ctx);
  CHECK_AND_ASSERT_MES(r, false, "RPC on_asset_emit failed: " << jerr.message);

  // make sure tx was NOT added to the pool (because it's only partially signed)
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  // unserialize transaction that we got from RPC
  emit_tx = transaction{};
  CHECK_AND_ASSERT_MES(emit_resp.data_for_external_signing.has_value(), false, "data_for_external_signing has no value");
  r = t_unserializable_object_from_blob(emit_tx, emit_resp.data_for_external_signing->unsigned_tx);
  CHECK_AND_ASSERT_MES(r, false, "t_unserializable_object_from_blob failed");
  CHECK_AND_ASSERT_EQ(get_transaction_hash(emit_tx), emit_resp.tx_id);

  // make sure emit transaction cannot be added to the transaction pool (it's not fully signed atm)
  tvc = tx_verification_context{};
  r = c.get_tx_pool().add_tx(emit_tx, tvc, false);
  CHECK_AND_ASSERT_MES(!r, false, "emit tx was able to be added to the pool");

  //
  // decrypt emission transaction outputs prior to ETH signing to make sure it's valid
  //
  decrypt_req = currency::COMMAND_RPC_DECRYPT_TX_DETAILS::request{};
  decrypt_req.tx_secret_key = emit_resp.data_for_external_signing->tx_secret_key;
  decrypt_req.tx_blob = epee::string_encoding::base64_encode(emit_resp.data_for_external_signing->unsigned_tx);
  // note: decrypt_req.outputs_addresses can be populated using emit_resp.data_for_external_signing->outputs_addresses but we fill it manually here
  decrypt_req.outputs_addresses.push_back(m_accounts[MINER_ACC_IDX].get_public_address_str()); // we expect that the first output is the cashback and addressed to miner
  for(size_t i = 0, size = emit_tx.vout.size() - 1; i < size; ++i)
    decrypt_req.outputs_addresses.push_back(m_accounts[BOB_ACC_IDX].get_public_address_str()); // we expect all other outputs are asset emission and addresses to Bob
  decrypt_resp = currency::COMMAND_RPC_DECRYPT_TX_DETAILS::response{};
  r = core_rpc_wrapper.on_decrypt_tx_details(decrypt_req, decrypt_resp, jerr, ctx);
  CHECK_AND_ASSERT_MES(r, false, "RPC on_decrypt_tx_details failed: " << jerr.message);
  // make sure that verified_tx_id is the one we expect
  CHECK_AND_ASSERT_EQ(decrypt_resp.verified_tx_id, emit_resp.tx_id);

  // after a successfull tx outputs decryption, examine them
  CHECK_AND_ASSERT_EQ(decrypt_resp.decoded_outputs.size(), emit_tx.vout.size());
  decrypted_emission_sum = 0;
  for(auto& el : decrypt_resp.decoded_outputs)
  {
    if (el.asset_id == asset_id)
      decrypted_emission_sum += el.amount;
    else
      CHECK_AND_ASSERT_EQ(el.asset_id, native_coin_asset_id);
  }
  // make sure the transaction emits the expected amount of asset
  CHECK_AND_ASSERT_EQ(decrypted_emission_sum, additional_emit_amount);

  //
  // as everything is allright, sign emit_tx with ETH signature.
  //
  eth_sig = crypto::eth_signature{};
  crypto::generate_eth_signature(emit_resp.tx_id, eth_sk_2, eth_sig); // note using ETH key #2
  // instant verification, just in case
  r = crypto::verify_eth_signature(emit_resp.tx_id, eth_pk_2, eth_sig);
  CHECK_AND_ASSERT_MES(r, false, "verify_eth_signature failed");

  //
  // send ETH signature alogn with all previous data to a wallet RPC call for final tx assembling and broadcasting
  //
  send_signed_req = tools::wallet_public::COMMAND_ASSET_SEND_EXT_SIGNED_TX::request{};
  send_signed_req.unsigned_tx     = emit_resp.data_for_external_signing->unsigned_tx;
  send_signed_req.eth_sig         = eth_sig;
  send_signed_req.expected_tx_id  = decrypt_resp.verified_tx_id;
  send_signed_req.finalized_tx    = emit_resp.data_for_external_signing->finalized_tx;
  send_signed_req.unlock_transfers_on_fail = true;
  send_signed_resp = tools::wallet_public::COMMAND_ASSET_SEND_EXT_SIGNED_TX::response{};
  r = miner_wlt_rpc.on_asset_send_ext_signed_tx(send_signed_req, send_signed_resp, jerr, ctx);
  CHECK_AND_ASSERT_MES(r, false, "RPC send_ext_signed_asset_tx failed: " << jerr.message);

  // make sure tx was broadcasted
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  // confirm the tx with a block
  CHECK_AND_ASSERT_MES(mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c), false, "");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  // Alice checks her asset balance, it shouldn't change
  alice_wlt->refresh(blocks_fetched);
  CHECK_AND_ASSERT_EQ(blocks_fetched, 1);
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", alice_amount, asset_id, adb.decimal_point), false, "");

  // Bob checks his asset balance
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  bob_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt, "Bob", additional_emit_amount, asset_id, adb.decimal_point), false, "");

  // finally, check asset's current supply
  asset_descriptor_base adb_temp{};
  r = c.get_blockchain_storage().get_asset_info(asset_id, adb_temp);
  CHECK_AND_ASSERT_MES(r, false, "get_asset_info failed");
  CHECK_AND_ASSERT_EQ(adb_temp.current_supply, total_asset_amount);

  return true;
}
