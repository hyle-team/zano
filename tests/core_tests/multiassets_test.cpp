// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "multiassets_test.h"
#include "wallet_test_core_proxy.h"

#include "random_helper.h"
#include "wallet/wallet_debug_events_definitions.h"
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

asset_operation_and_hardfork_checks::asset_operation_and_hardfork_checks()
{
  m_adb_hello.total_max_supply = 1'000'000'000'000'000'000;
  m_adb_hello.current_supply = 1'000'000'000'000'000'000;
  m_adb_hello.ticker = "HLO";
  m_adb_hello.full_name = "HELLO_WORLD";
  m_adb_hello.meta_info = "Hello, world!";
  m_adb_hello.hidden_supply = false;

  m_ado_hello.operation_type = ASSET_DESCRIPTOR_OPERATION_REGISTER;
  m_ado_hello.opt_asset_id = currency::null_pkey;

  m_adb_bye.total_max_supply = 1'000'000'000'000'000'000;
  m_adb_bye.current_supply = 1'000'000'000'000'000'000;
  m_adb_bye.ticker = "BYE";
  m_adb_bye.full_name = "BYE_WORLD";
  m_adb_bye.meta_info = "Bye, world!";
  m_adb_bye.hidden_supply = false;

  m_ado_bye.operation_type = ASSET_DESCRIPTOR_OPERATION_REGISTER;
  m_ado_hello.opt_asset_id = currency::null_pkey;

  REGISTER_CALLBACK_METHOD(asset_operation_and_hardfork_checks, c1);
  REGISTER_CALLBACK_METHOD(asset_operation_and_hardfork_checks, c2);
}

bool asset_operation_and_hardfork_checks::generate(
  std::vector<test_event_entry>& events) const
{
  /*
    0           10     11           21     22
  ( 0) - ... - (0r) - ( 1) - ... - (1r) - ( 2)
                       \                   \
                        [tx_0]              [tx_1]
  */

  bool success{false};
  std::vector<tx_source_entry> sources{};
  std::vector<tx_destination_entry> destinations{};
  transaction tx_0{}, tx_1{}, tx_2{}, tx_3{}, tx_4{};
  uint64_t tx_version{};
  crypto::secret_key stub{};

  m_accounts.resize(2);
  account_base& miner{m_accounts[MINER_ACC_IDX]};
  miner.generate();
  account_base& alice{m_accounts[ALICE_ACC_IDX]};
  alice.generate();

  m_adb_hello.owner = alice.get_public_address().spend_public_key;
  m_ado_hello.descriptor = m_adb_hello;

  m_adb_bye.owner = alice.get_public_address().spend_public_key;
  m_ado_bye.descriptor = m_adb_bye;

  MAKE_GENESIS_BLOCK(events,
                     blk_0,
                     miner,
                     test_core_time::get_time());

  DO_CALLBACK(events, "configure_core");

  REWIND_BLOCKS_N_WITH_TIME(events,
                            blk_0r,
                            blk_0,
                            miner,
                            CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  success = fill_tx_sources_and_destinations(events,
                                             /* head = */ blk_0r,
                                             /* from = */ miner.get_keys(),
                                             /* to = */ alice.get_public_address(),
                                             /* amount = */ MK_TEST_COINS(12),
                                             /* fee = */ TESTS_DEFAULT_FEE,
                                             /* nmix = */ 0,
                                             sources,
                                             destinations);

  CHECK_AND_ASSERT_MES(success, false, "fail to fill sources, destinations");

  tx_version = get_tx_version(get_block_height(blk_0r),
                              m_hardforks);

  success = construct_tx(miner.get_keys(),
                         sources,
                         destinations,
                         empty_attachment,
                         tx_0,
                         tx_version,
                         0);

  CHECK_AND_ASSERT_MES(success, false, "fail to construct tx_0");

  ADD_CUSTOM_EVENT(events, tx_0);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner, tx_0);

  REWIND_BLOCKS_N_WITH_TIME(events,
                            blk_1r,
                            blk_1,
                            miner,
                            CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  sources.clear();
  destinations.clear();

  success = fill_tx_sources_and_destinations(events,
                                             /* head = */ blk_1r,
                                             /* from = */ alice,
                                             /* to = */ alice,
                                             /* amount = */ MK_TEST_COINS(5),
                                             /* fee = */ TESTS_DEFAULT_FEE,
                                             /* nmix = */ 0,
                                             sources,
                                             destinations);

  CHECK_AND_ASSERT_MES(success, false, "fail to fill sources, destinations");

  destinations.emplace_back(/* amount = */ 1'000'000'000'000'000'000,
                            /* to = */ alice.get_public_address(),
                            /* asset_id = */ currency::null_pkey);

  tx_version = get_tx_version(get_block_height(blk_1r), m_hardforks);

  success = construct_tx(alice.get_keys(),
                         sources,
                         destinations,
                         /* extra = */ {m_ado_hello},
                         empty_attachment,
                         tx_1,
                         tx_version,
                         stub,
                         0);

  CHECK_AND_ASSERT_MES(success, false, "fail to construct tx_1");
  ADD_CUSTOM_EVENT(events, tx_1);

  MAKE_NEXT_BLOCK_TX1(events,
                      blk_2,
                      blk_1r,
                      alice,
                      tx_1);

  REWIND_BLOCKS_N_WITH_TIME(events,
                            blk_2r,
                            blk_2,
                            miner,
                            CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  /* A transaction that has at least 2 registration operation descriptors in its
  extra is rejected by the core. */

  sources.clear();
  destinations.clear();

  success = fill_tx_sources_and_destinations(events,
                                             /* head = */ blk_2r,
                                             /* from = */ alice,
                                             /* to = */ alice,
                                             /* amount = */ MK_TEST_COINS(2),
                                             /* fee = */ TESTS_DEFAULT_FEE,
                                             /* nmix = */ 0,
                                             sources,
                                             destinations);

  CHECK_AND_ASSERT_MES(success, false, "fail to fill sources, destinations");

  tx_version = get_tx_version(get_block_height(blk_2r),
                              m_hardforks);

  success = construct_tx(alice.get_keys(),
                         sources,
                         destinations,
                         /* extra = */ {m_ado_hello, m_ado_hello},
                         empty_attachment,
                         tx_2,
                         tx_version,
                         stub,
                         0);

  CHECK_AND_ASSERT_MES(success, false, "fail to construct tx_2");

  DO_CALLBACK(events, "mark_invalid_tx");
  ADD_CUSTOM_EVENT(events, tx_2);

  sources.clear();
  destinations.clear();

  /* A transaction that contains a registration operation descriptor in its
  attachement, but extra is empty, is valid, but doesn't register the asset. The
  fact that the asset is not registered is checked in the assertions in the
  callback с2. */

  success = fill_tx_sources_and_destinations(events,
                                             /* head = */ blk_2r,
                                             /* from = */ alice,
                                             /* to = */ alice,
                                             /* amount = */ MK_TEST_COINS(2),
                                             /* fee = */ TESTS_DEFAULT_FEE,
                                             /* nmix = */ 0,
                                             sources,
                                             destinations);

  CHECK_AND_ASSERT_MES(success, false, "fail to fill sources, destinations");

  tx_version = get_tx_version(get_block_height(blk_2r),
                              m_hardforks);

  success = construct_tx(alice.get_keys(),
                         sources,
                         destinations,
                         /* attachments = */ {m_ado_bye},
                         tx_3,
                         tx_version,
                         0);

  CHECK_AND_ASSERT_MES(success, false, "fail to construct tx_3");

  ADD_CUSTOM_EVENT(events, tx_3);
  DO_CALLBACK(events, "c2");

  sources.clear();
  destinations.clear();

  /* A transaction that contains a registration operation descriptor in its
  attachement, but extra is empty, is valid, but doesn't register the asset. In
  this case a different definition of the function construct_tx is used. */

  success = fill_tx_sources_and_destinations(events,
                                             /* head = */ blk_2r,
                                             /* from = */ alice,
                                             /* to = */ alice,
                                             /* amount = */ MK_TEST_COINS(2),
                                             /* fee = */ TESTS_DEFAULT_FEE,
                                             /* nmix = */ 0,
                                             sources,
                                             destinations);

  CHECK_AND_ASSERT_MES(success, false, "fail to fill sources, destinations");

  tx_version = get_tx_version(get_block_height(blk_2r),
                              m_hardforks);

  success = construct_tx(alice.get_keys(),
                         sources,
                         destinations,
                         empty_extra,
                         /* attachments = */ {m_ado_bye},
                         tx_4,
                         tx_version,
                         stub,
                         0);

  CHECK_AND_ASSERT_MES(success, false, "fail to construct tx_4");

  ADD_CUSTOM_EVENT(events, tx_4);
  DO_CALLBACK(events, "c2");
  DO_CALLBACK(events, "c1");

  return true;
}

bool asset_operation_and_hardfork_checks::c1(
  currency::core& c,
  size_t ev_index,
  const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> wallet{
    init_playtime_test_wallet_t<tools::wallet2>(events, c, ALICE_ACC_IDX)
  };

  wallet->refresh();

  crypto::point_t asset_id_point{};
  crypto::public_key asset_id_public_key{};
  currency::get_or_calculate_asset_id(m_ado_hello,
                                      &asset_id_point,
                                      &asset_id_public_key);

  CHECK_AND_ASSERT_MES(
    check_balance_via_wallet(*wallet,
                             /* name = */ "Alice",
                             /* expected_total = */ 1'000'000'000'000'000'000,
                             /* expected_mined = */ 2 * COIN,
                             /* expected_unlocked = */ 1'000'000'000'000'000'000,
                             /* expected_awaiting_in = */ INVALID_BALANCE_VAL,
                             /* expected_awaiting_out = */ INVALID_BALANCE_VAL,
                             asset_id_public_key),
    false,
    "balance check failed");

  return true;
}

bool asset_operation_and_hardfork_checks::c2(
  currency::core& c,
  size_t ev_index,
  const std::vector<test_event_entry>& events)
{
  crypto::point_t asset_id_pt{};
  crypto::public_key asset_id{};
  currency::asset_descriptor_base stub{};

  CHECK_AND_ASSERT_MES(
    get_or_calculate_asset_id(m_ado_bye,
                              &asset_id_pt,
                              &asset_id),
    false,
    "fail to calculate asset id");

  CHECK_AND_ASSERT_MES(
    !c.get_blockchain_storage().get_asset_info(asset_id,
                                               stub),
    false,
    "unregistered asset has info");

  return true;
}

asset_operation_in_consolidated_tx::asset_operation_in_consolidated_tx()
{
  m_adb_alice_currency.total_max_supply = 1'000'000'000'000'000'000;
  m_adb_alice_currency.current_supply = 1'000'000'000'000'000'000;
  m_adb_alice_currency.ticker = "ALC";
  m_adb_alice_currency.full_name = "ALICE";
  m_adb_alice_currency.meta_info = "Currency created by Alice";
  m_adb_alice_currency.hidden_supply = false;

  m_ado_alice_currency.operation_type = ASSET_DESCRIPTOR_OPERATION_REGISTER;
  m_ado_alice_currency.opt_asset_id = currency::null_pkey;

  REGISTER_CALLBACK_METHOD(asset_operation_in_consolidated_tx, assert_balances);
  REGISTER_CALLBACK_METHOD(asset_operation_in_consolidated_tx, assert_alice_currency_not_registered);
}

bool asset_operation_in_consolidated_tx::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure that the core rule prohibiting operations with assets in TX_FLAG_SIGNATURE_MODE_SEPARATE transactions works.
  bool success {};
  transaction tx_2 {};
  uint64_t tx_version {};
  crypto::secret_key one_time {};
  tx_generation_context context_tx_2 {};
  GENERATE_ACCOUNT(miner);
  GENERATE_ACCOUNT(alice);
  GENERATE_ACCOUNT(bob);

  m_accounts.push_back(miner);
  m_accounts.push_back(alice);
  m_accounts.push_back(bob);
  m_adb_alice_currency.owner      = m_accounts.at(ALICE_ACC_IDX).get_public_address().spend_public_key;
  m_ado_alice_currency.descriptor = m_adb_alice_currency;

  MAKE_GENESIS_BLOCK(events, blk_0, miner, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  MAKE_TX(events, tx_0, miner, bob, MK_TEST_COINS(10), blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner, tx_0);
  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  MAKE_TX(events, tx_1, miner, alice, MK_TEST_COINS(10), blk_1r);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner, tx_1);
  REWIND_BLOCKS_N(events, blk_2r, blk_2, miner, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  // Miner sent 10 coins to Alice, 10 coins to Bob.
  DO_CALLBACK(events, "assert_balances");

  {
    std::vector<tx_source_entry> sources {};
    std::vector<tx_destination_entry> destinations {};

    success = fill_tx_sources(sources, events, blk_2r, alice.get_keys(), MK_TEST_COINS(5), 1);
    CHECK_AND_ASSERT_MES(success, false, "failed to fill transaction sources on step 1");
    destinations.emplace_back(MK_TEST_COINS(5), bob.get_public_address());
    destinations.emplace_back(MK_TEST_COINS(/* 10 - 5 - 1 = */ 4), alice.get_public_address());
    tx_version = get_tx_version(get_block_height(blk_2r), m_hardforks);
    success    = construct_tx(alice.get_keys(), sources, destinations, empty_extra, empty_attachment, tx_2, tx_version, one_time, 0, 0, 0, true, TX_FLAG_SIGNATURE_MODE_SEPARATE, TESTS_DEFAULT_FEE,
                              context_tx_2);
    CHECK_AND_ASSERT_MES(success, false, "failed to construct transaction tx_2 on step 1");
  }

  // Transaction tx_2 hasn't been constructed completely yet. Core rejects tx_2.
  DO_CALLBACK(events, "mark_invalid_tx");
  ADD_CUSTOM_EVENT(events, tx_2);

  {
    std::vector<tx_source_entry> sources {};
    std::vector<tx_destination_entry> destinations {};

    success = fill_tx_sources(sources, events, blk_2r, bob.get_keys(), MK_TEST_COINS(5), 0);
    CHECK_AND_ASSERT_MES(success, false, "failed to fill transaction sources on step 2");
    for(tx_source_entry& source : sources)
    {
      source.separately_signed_tx_complete = true;
    }
    destinations.emplace_back(MK_TEST_COINS(5), alice.get_public_address());
    destinations.emplace_back(MK_TEST_COINS(/* 10 - 5 - 0 = */ 5), bob.get_public_address());
    destinations.emplace_back(m_adb_alice_currency.current_supply, alice.get_public_address(), null_pkey);
    tx_version = get_tx_version(get_block_height(blk_2r), m_hardforks);
    success    = construct_tx(bob.get_keys(), sources, destinations, { m_ado_alice_currency }, empty_attachment, tx_2, tx_version, one_time, 0, 0, 0, true, TX_FLAG_SIGNATURE_MODE_SEPARATE,
                              /* fee = */ 0, context_tx_2);
    CHECK_AND_ASSERT_MES(success, false, "failed to construct transaction tx_2 on step 2");
  }

  DO_CALLBACK(events, "mark_invalid_tx");
  ADD_CUSTOM_EVENT(events, tx_2);
  // Core rejects transaction tx_2. The balances of Alice, Bob haven't changed: Alice has 10 coins, Bob has 10 coins.
  DO_CALLBACK(events, "assert_balances");
  // Alice's asset hasn't registered, because transaction tx_2 was rejected.
  DO_CALLBACK(events, "assert_alice_currency_not_registered");

  return true;
}

bool asset_operation_in_consolidated_tx::assert_balances(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wallet{init_playtime_test_wallet(events, c, ALICE_ACC_IDX)};
  std::shared_ptr<tools::wallet2> bob_wallet{init_playtime_test_wallet(events, c, BOB_ACC_IDX)};

  alice_wallet->refresh();
  bob_wallet->refresh();

  CHECK_AND_ASSERT_EQ(alice_wallet->balance(currency::native_coin_asset_id), MK_TEST_COINS(10));
  CHECK_AND_ASSERT_EQ(bob_wallet->balance(currency::native_coin_asset_id), MK_TEST_COINS(10));

  return true;
}

bool asset_operation_in_consolidated_tx::assert_alice_currency_not_registered(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  crypto::point_t asset_id_point{};
  crypto::public_key asset_id{};
  currency::asset_descriptor_base stub{};

  CHECK_AND_ASSERT_MES(get_or_calculate_asset_id(m_ado_alice_currency, &asset_id_point, &asset_id), false, "fail to calculate asset id");
  CHECK_AND_ASSERT_MES(!c.get_blockchain_storage().get_asset_info(asset_id, stub), false, "unregistered asset has info");

  return true;
}
