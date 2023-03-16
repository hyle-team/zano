// Copyright (c) 2014-2022 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "ionic_swap_tests.h"
#include "wallet_test_core_proxy.h"

#include "random_helper.h"
#include "tx_builder.h"

#define  AMOUNT_TO_TRANSFER_MULTIASSETS_BASIC (TESTS_DEFAULT_FEE)

#define  AMOUNT_ASSETS_TO_TRANSFER_MULTIASSETS_BASIC  500000000000000000



ionic_swap_basic_test::ionic_swap_basic_test()
{
  REGISTER_CALLBACK_METHOD(ionic_swap_basic_test, configure_core);
  REGISTER_CALLBACK_METHOD(ionic_swap_basic_test, c1);

  m_hardforks.set_hardfork_height(1, 1);
  m_hardforks.set_hardfork_height(2, 1);
  m_hardforks.set_hardfork_height(3, 1);
  m_hardforks.set_hardfork_height(4, 2);
}

bool ionic_swap_basic_test::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc =   m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);
  //account_base& carol_acc = m_accounts[CAROL_ACC_IDX]; carol_acc.generate(); carol_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  DO_CALLBACK(events, "c1");

  return true;
}

bool zarcanum_basic_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  miner_wlt->get_account().set_createtime(0);

  //std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  //std::shared_ptr<tools::wallet2> bob_wlt   = init_playtime_test_wallet(events, c, BOB_ACC_IDX);

  // check passing over the hardfork
  CHECK_AND_ASSERT_MES(!c.get_blockchain_storage().is_hardfork_active(ZANO_HARDFORK_04_ZARCANUM), false, "ZANO_HARDFORK_04_ZARCANUM is active");


  account_base alice_acc;
  alice_acc.generate();
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, alice_acc);
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
  destinations[0].asset_id = currency::ffff_hash;
  destinations[1].addr.push_back(alice_wlt->get_account().get_public_address());
  destinations[1].amount = AMOUNT_ASSETS_TO_TRANSFER_MULTIASSETS_BASIC;
  destinations[1].asset_id = currency::ffff_hash;

  LOG_PRINT_MAGENTA("destinations[0].asset_id:" << destinations[0].asset_id, LOG_LEVEL_0);
  LOG_PRINT_MAGENTA("destinations[1].asset_id:" << destinations[1].asset_id, LOG_LEVEL_0);
  LOG_PRINT_MAGENTA("currency::ffff_hash:" << currency::ffff_hash, LOG_LEVEL_0);

  currency::transaction tx = AUTO_VAL_INIT(tx);
  crypto::hash asset_id = currency::null_hash;
  miner_wlt->publish_new_asset(adb, destinations, tx, asset_id);
  LOG_PRINT_L0("Published new asset: " << asset_id << ", tx_id: " << currency::get_transaction_hash(tx));

  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");


  miner_wlt->refresh();
  alice_wlt->refresh();
  uint64_t mined = 0;
  std::unordered_map<crypto::hash, tools::wallet_public::asset_balance_entry_base> balances;
  miner_wlt->balance(balances, mined);

  auto it_asset = balances.find(asset_id);
  auto it_native = balances.find(currency::null_hash);


  CHECK_AND_ASSERT_MES(it_asset != balances.end() && it_native != balances.end(), false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_asset->second.total == AMOUNT_ASSETS_TO_TRANSFER_MULTIASSETS_BASIC, false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_native->second.total == uint64_t(17517226)*COIN, false, "Failed to find needed asset in result balances");

  uint64_t mined_balance = it_native->second.total;

  balances.clear();
  alice_wlt->balance(balances, mined);

  it_asset = balances.find(asset_id);
  it_native = balances.find(currency::null_hash);

  CHECK_AND_ASSERT_MES(it_asset != balances.end(), false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_native == balances.end(), false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_asset->second.total == AMOUNT_ASSETS_TO_TRANSFER_MULTIASSETS_BASIC, false, "Failed to find needed asset in result balances");

  const uint64_t assets_to_exchange = 10 * COIN;
  const uint64_t native_tokens_to_exchange = 1 * COIN;

  //alice_wlt want to trade with miner_wlt, to exchange 10.0 TCT to 1.0 ZANO 
  view::ionic_swap_proposal_info proposal_details = AUTO_VAL_INIT(proposal_details);
  proposal_details.expiration_time = alice_wlt->get_core_runtime_config().get_core_time() + 10 * 60;
  proposal_details.fee = TESTS_DEFAULT_FEE;
  proposal_details.mixins = 10;
  proposal_details.from.push_back(view::asset_funds{ asset_id , assets_to_exchange });
  proposal_details.to.push_back(view::asset_funds{ currency::null_pkey , native_tokens_to_exchange });

  currency::transaction tx_template = AUTO_VAL_INIT(tx_template);
  alice_wlt->create_ionic_swap_proposal(proposal_details, miner_wlt->get_account().get_public_address(), tx_template);

  view::ionic_swap_proposal_info proposal_decoded_info;
  miner_wlt->get_ionic_swap_proposal_info(tx_template, proposal_decoded_info);

  //Validate proposal
  if (proposal_decoded_info.from != proposal_details.from || proposal_decoded_info.to != proposal_details.to)
  {
    CHECK_AND_ASSERT_MES(false, false, "proposal actual and proposals decoded mismatch");
  }

  currency::transaction res_tx = AUTO_VAL_INIT(res_tx);
  r = miner_wlt->accept_ionic_swap_proposal(tx_template, res_tx);
  CHECK_AND_ASSERT_MES(r, false, "Failed to accept ionic proposal");

  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  miner_wlt->refresh();
  alice_wlt->refresh();


  balances.clear();
  alice_wlt->balance(balances, mined);

  it_asset = balances.find(asset_id);
  it_native = balances.find(currency::null_hash);

  CHECK_AND_ASSERT_MES(it_asset != balances.end(), false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_native->second.total == native_tokens_to_exchange, false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_asset->second.total == AMOUNT_ASSETS_TO_TRANSFER_MULTIASSETS_BASIC - assets_to_exchange, false, "Failed to find needed asset in result balances");


  balances.clear();
  miner_wlt->balance(balances, mined);

  CHECK_AND_ASSERT_MES(it_asset != balances.end(), false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_native->second.total == mined_balance - native_tokens_to_exchange, false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_asset->second.total == AMOUNT_ASSETS_TO_TRANSFER_MULTIASSETS_BASIC + assets_to_exchange, false, "Failed to find needed asset in result balances");


  return true;
}

