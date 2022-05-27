// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "wallet_rpc_tests.h"
#include "wallet_test_core_proxy.h"
#include "../../src/wallet/wallet_rpc_server.h"
#include "offers_helper.h"
#include "random_helper.h"

using namespace currency;

wallet_rpc_integrated_address::wallet_rpc_integrated_address()
{
  //REGISTER_CALLBACK_METHOD(wallet_rpc_integrated_address, c1);
}

bool wallet_rpc_integrated_address::generate(std::vector<test_event_entry>& events) const
{
  bool r = false;
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());

  CREATE_TEST_WALLET(miner_wlt, miner_acc, blk_0);

  // wallet RPC server
  tools::wallet_rpc_server miner_wlt_rpc(*miner_wlt);
  epee::json_rpc::error je;
  tools::wallet_rpc_server::connection_context ctx;

  tools::wallet_public::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::request  mia_req = AUTO_VAL_INIT(mia_req);
  tools::wallet_public::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::response mia_res = AUTO_VAL_INIT(mia_res);
  tools::wallet_public::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::request  sia_req = AUTO_VAL_INIT(sia_req);
  tools::wallet_public::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::response sia_res = AUTO_VAL_INIT(sia_res);

  // 1. make_integrated_address with empty payment id (should use a random payment id instead) + on_split_integrated_address
  mia_req.payment_id = "";
  r = miner_wlt_rpc.on_make_integrated_address(mia_req, mia_res, je, ctx);
  CHECK_AND_ASSERT_MES(r, false, "RPC call failed, code: " << je.code << ", msg: " << je.message);
  CHECK_AND_ASSERT_MES(!mia_res.payment_id.empty(), false, "on_make_integrated_address returned empty payment id");

  sia_req.integrated_address = mia_res.integrated_address;
  r = miner_wlt_rpc.on_split_integrated_address(sia_req, sia_res, je, ctx);
  CHECK_AND_ASSERT_MES(r, false, "RPC call failed, code: " << je.code << ", msg: " << je.message);

  CHECK_AND_ASSERT_MES(sia_res.standard_address == m_accounts[MINER_ACC_IDX].get_public_address_str(), false, "address missmatch");
  CHECK_AND_ASSERT_MES(sia_res.payment_id == mia_res.payment_id, false, "payment id missmatch");

  
  // 2. make_integrated_address with too long payment id
  std::string payment_id = get_random_text(BC_PAYMENT_ID_SERVICE_SIZE_MAX + 1);
  mia_req.payment_id = epee::string_tools::buff_to_hex_nodelimer(payment_id);
  r = miner_wlt_rpc.on_make_integrated_address(mia_req, mia_res, je, ctx);
  CHECK_AND_ASSERT_MES(!r, false, "RPC call not failed as expected");
  
  
  // 3. make_integrated_address with correct payment id + on_split_integrated_address
  payment_id = get_random_text(BC_PAYMENT_ID_SERVICE_SIZE_MAX);
  mia_req.payment_id = epee::string_tools::buff_to_hex_nodelimer(payment_id);
  r = miner_wlt_rpc.on_make_integrated_address(mia_req, mia_res, je, ctx);
  CHECK_AND_ASSERT_MES(r, false, "RPC call failed, code: " << je.code << ", msg: " << je.message);
  CHECK_AND_ASSERT_MES(mia_res.payment_id == mia_req.payment_id, false, "on_make_integrated_address: wrong payment id");

  sia_req.integrated_address = mia_res.integrated_address;
  r = miner_wlt_rpc.on_split_integrated_address(sia_req, sia_res, je, ctx);
  CHECK_AND_ASSERT_MES(r, false, "RPC call failed, code: " << je.code << ", msg: " << je.message);

  CHECK_AND_ASSERT_MES(sia_res.standard_address == m_accounts[MINER_ACC_IDX].get_public_address_str(), false, "address missmatch");
  CHECK_AND_ASSERT_MES(sia_res.payment_id == mia_req.payment_id, false, "payment id missmatch");

  return true;
}

//------------------------------------------------------------------------------

wallet_rpc_integrated_address_transfer::wallet_rpc_integrated_address_transfer()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_integrated_address_transfer, c1);
}

bool wallet_rpc_integrated_address_transfer::generate(std::vector<test_event_entry>& events) const
{
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  DO_CALLBACK(events, "c1");

  return true;
}

bool wallet_rpc_integrated_address_transfer::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);

  miner_wlt->refresh();

  std::string payment_id;
  payment_id.resize(BC_PAYMENT_ID_SERVICE_SIZE_MAX, 'x');
  std::string alice_integrated_address = get_account_address_and_payment_id_as_str(m_accounts[ALICE_ACC_IDX].get_public_address(), payment_id);

  // wallet RPC server
  tools::wallet_rpc_server miner_wlt_rpc(*miner_wlt);
  epee::json_rpc::error je;
  tools::wallet_rpc_server::connection_context ctx;

  tools::wallet_public::COMMAND_RPC_TRANSFER::request  req = AUTO_VAL_INIT(req);
  req.fee = TESTS_DEFAULT_FEE;
  req.mixin = 0;
  tools::wallet_public::transfer_destination tds = AUTO_VAL_INIT(tds);
  tds.address = alice_integrated_address;
  tds.amount = MK_TEST_COINS(3);
  req.destinations.push_back(tds);
  
  tools::wallet_public::COMMAND_RPC_TRANSFER::response res = AUTO_VAL_INIT(res);

  // 1. integrated address + external payment id => the following should fail
  req.payment_id = "90210";
  r = miner_wlt_rpc.on_transfer(req, res, je, ctx);
  CHECK_AND_ASSERT_MES(!r, false, "RPC call not failed as expected");

  // 2. integrated address + no external payment id => normal condition
  req.payment_id.clear();
  r = miner_wlt_rpc.on_transfer(req, res, je, ctx);
  CHECK_AND_ASSERT_MES(r, false, "RPC call failed, code: " << je.code << ", msg: " << je.message);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "enexpected pool txs count: " << c.get_pool_transactions_count());
  
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Tx pool is not empty: " << c.get_pool_transactions_count());

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, tds.amount), false, "");

  // check the transfer has been received
  std::list<tools::wallet2::payment_details> payments;
  alice_wlt->get_payments(payment_id, payments);
  CHECK_AND_ASSERT_MES(payments.size() == 1, false, "Invalid payments count: " << payments.size());
  CHECK_AND_ASSERT_MES(payments.front().m_amount == MK_TEST_COINS(3), false, "Invalid payment");
  CHECK_AND_ASSERT_MES(check_mixin_value_for_each_input(0, payments.front().m_tx_hash, c), false, ""); // make sure number of decoys is correct


  // 3. standard address + invalid external payment id => fail
  req.destinations.clear();
  tools::wallet_public::transfer_destination tds2 = AUTO_VAL_INIT(tds2);
  tds2.address = m_accounts[ALICE_ACC_IDX].get_public_address_str();
  tds2.amount = MK_TEST_COINS(7);
  req.destinations.push_back(tds2);

  req.payment_id = "Zuckerman";
  je = AUTO_VAL_INIT(je);
  r = miner_wlt_rpc.on_transfer(req, res, je, ctx);
  CHECK_AND_ASSERT_MES(!r, false, "RPC call not failed as expected");

  // 4. standard address + external payment id => success
  req.payment_id = "0A13fFEe";
  epee::string_tools::parse_hexstr_to_binbuff(req.payment_id, payment_id);
  je = AUTO_VAL_INIT(je);
  r = miner_wlt_rpc.on_transfer(req, res, je, ctx);
  CHECK_AND_ASSERT_MES(r, false, "RPC call failed, code: " << je.code << ", msg: " << je.message);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "enexpected pool txs count: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Tx pool is not empty: " << c.get_pool_transactions_count());

  alice_wlt->refresh();

  // check the transfer has been received
  payments.clear();
  alice_wlt->get_payments(payment_id, payments);
  CHECK_AND_ASSERT_MES(payments.size() == 1, false, "Invalid payments count: " << payments.size());
  CHECK_AND_ASSERT_MES(payments.front().m_amount == MK_TEST_COINS(7), false, "Invalid payment");
  CHECK_AND_ASSERT_MES(check_mixin_value_for_each_input(0, payments.front().m_tx_hash, c), false, ""); // make sure number of decoys is correct


  return true;
}

//------------------------------------------------------------------------------

wallet_rpc_transfer::wallet_rpc_transfer()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_transfer, configure_core);
  REGISTER_CALLBACK_METHOD(wallet_rpc_transfer, c1);
}

bool wallet_rpc_transfer::generate(std::vector<test_event_entry>& events) const
{
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  set_hard_fork_heights_to_generator(generator);
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 6);

  DO_CALLBACK(events, "c1");

  return true;
}

bool wallet_rpc_transfer::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.hard_forks.set_hardfork_height(1, 1);
  pc.hard_forks.set_hardfork_height(2, 1);
  pc.hard_forks.set_hardfork_height(3, 1);
  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

bool wallet_rpc_transfer::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);

  miner_wlt->refresh();

  // wallet RPC server
  tools::wallet_rpc_server miner_wlt_rpc(*miner_wlt);
  epee::json_rpc::error je;
  tools::wallet_rpc_server::connection_context ctx;

  // 1. Check non-zero mixin and default settings
  tools::wallet_public::COMMAND_RPC_TRANSFER::request  req = AUTO_VAL_INIT(req);
  req.fee = TESTS_DEFAULT_FEE;
  req.mixin = 2;
  tools::wallet_public::transfer_destination tds = AUTO_VAL_INIT(tds);
  tds.address = m_accounts[ALICE_ACC_IDX].get_public_address_str();
  tds.amount = MK_TEST_COINS(3);
  req.destinations.push_back(tds);

  tools::wallet_public::COMMAND_RPC_TRANSFER::response res = AUTO_VAL_INIT(res);

  r = miner_wlt_rpc.on_transfer(req, res, je, ctx);
  CHECK_AND_ASSERT_MES(r, false, "RPC call failed, code: " << je.code << ", msg: " << je.message);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "enexpected pool txs count: " << c.get_pool_transactions_count());

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Tx pool is not empty: " << c.get_pool_transactions_count());

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, tds.amount), false, "");

  // check the transfer has been received
  tools::wallet2::transfer_details td = AUTO_VAL_INIT(td);
  CHECK_AND_ASSERT_MES(alice_wlt->get_transfer_info_by_index(0, td), false, "");
  CHECK_AND_ASSERT_MES(td.amount() == MK_TEST_COINS(3), false, "Invalid payment");
  CHECK_AND_ASSERT_MES(check_mixin_value_for_each_input(2, td.tx_hash(), c), false, "");

  // make sure tx_received is set by default, but tx_payer is not
  std::shared_ptr<const transaction_chain_entry> pche = c.get_blockchain_storage().get_tx_chain_entry(td.tx_hash());
  CHECK_AND_ASSERT_MES(currency::count_type_in_variant_container<tx_receiver>(pche->tx.extra) == 1, false, "tx_receiver: incorrect count of items");
  CHECK_AND_ASSERT_MES(currency::count_type_in_variant_container<tx_payer>(pche->tx.extra) == 0, false, "tx_payer: incorrect count of items");


  // 2. check tx_receiver and tx_payer non-default
  req.mixin = 1;
  req.hide_receiver = true;
  req.push_payer = true;
  tds.amount = MK_TEST_COINS(5);
  req.destinations.clear();
  req.destinations.push_back(tds);

  res = AUTO_VAL_INIT(res);

  r = miner_wlt_rpc.on_transfer(req, res, je, ctx);
  CHECK_AND_ASSERT_MES(r, false, "RPC call failed, code: " << je.code << ", msg: " << je.message);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "enexpected pool txs count: " << c.get_pool_transactions_count());

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Tx pool is not empty: " << c.get_pool_transactions_count());

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(3 + 5)), false, "");
  
  td = AUTO_VAL_INIT(td);
  CHECK_AND_ASSERT_MES(alice_wlt->get_transfer_info_by_index(1, td), false, "");
  CHECK_AND_ASSERT_MES(td.amount() == MK_TEST_COINS(5), false, "Invalid payment");
  CHECK_AND_ASSERT_MES(check_mixin_value_for_each_input(1, td.tx_hash(), c), false, "");

  // make sure tx_received is set by default, but tx_payer is not
  pche = c.get_blockchain_storage().get_tx_chain_entry(td.tx_hash());
  CHECK_AND_ASSERT_MES(currency::count_type_in_variant_container<tx_receiver>(pche->tx.extra) == 0, false, "tx_receiver: incorrect count of items");
  CHECK_AND_ASSERT_MES(currency::count_type_in_variant_container<tx_payer>(pche->tx.extra) == 1, false, "tx_payer: incorrect count of items");


  return true;
}
