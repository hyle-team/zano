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
  tools::wallet_rpc_server miner_wlt_rpc(miner_wlt);
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
  tools::wallet_rpc_server miner_wlt_rpc(miner_wlt);
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
  std::list<tools::payment_details> payments;
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
  
  m_hardforks.set_hardfork_height(1, 1);
  m_hardforks.set_hardfork_height(2, 1);
  m_hardforks.set_hardfork_height(3, 1);
}

bool wallet_rpc_transfer::generate(std::vector<test_event_entry>& events) const
{
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core"); // default callback will initialize core runtime config with m_hardforks
  set_hard_fork_heights_to_generator(generator);
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 6);

  DO_CALLBACK(events, "c1");

  return true;
}

bool wallet_rpc_transfer::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);

  miner_wlt->refresh();

  // wallet RPC server
  tools::wallet_rpc_server miner_wlt_rpc(miner_wlt);
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
  tools::transfer_details td = AUTO_VAL_INIT(td);
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

//------------------------------------------------------------------------------

wallet_rpc_exchange_suite::wallet_rpc_exchange_suite()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_exchange_suite, c1);
}

bool wallet_rpc_exchange_suite::generate(std::vector<test_event_entry>& events) const
{
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core"); // default callback will initialize core runtime config with m_hardforks

  DO_CALLBACK(events, "c1");

  return true;
}


struct transport
{
  tools::wallet_rpc_server& m_rpc_srv;
  transport(tools::wallet_rpc_server& rpc_srv):m_rpc_srv(rpc_srv)
  {}
  epee::net_utils::http::http_response_info m_response;

  bool is_connected() { return true; }
  template<typename t_a, typename t_b, typename t_c>
  bool connect(t_a ta, t_b tb, t_c tc) { return true; }

  template<typename dummy_t>
  bool invoke(const std::string uri, const std::string method_, const std::string& body, const epee::net_utils::http::http_response_info** ppresponse_info, const dummy_t& d)
  {
    epee::net_utils::http::http_request_info query_info;
    query_info.m_URI = uri;
    query_info.m_body = body;
    tools::wallet_rpc_server::connection_context ctx;
    bool r = m_rpc_srv.handle_http_request(query_info, m_response, ctx);
    if (ppresponse_info)
      *ppresponse_info = &m_response;
    return r;
  }
};

template<typename request_t, typename response_t>
bool invoke_text_json_for_rpc(tools::wallet_rpc_server& srv, const std::string& method_name, const request_t& req, response_t& resp)
{
  transport tr(srv);

  bool r = epee::net_utils::invoke_http_json_rpc("/json_rpc", method_name, req, resp, tr);
  return r;
}

#include "wallet_rpc_tests_legacy_defs.h"


std::string gen_payment_id(tools::wallet_rpc_server& custody_wlt_rpc)
{
  pre_hf4_api::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::request req = AUTO_VAL_INIT(req);
  pre_hf4_api::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::response resp = AUTO_VAL_INIT(resp);
  bool r = invoke_text_json_for_rpc(custody_wlt_rpc, "make_integrated_address", req, resp);
  CHECK_AND_ASSERT_MES(r, "", "failed to call");
  return resp.payment_id;
}

std::string get_integr_addr(tools::wallet_rpc_server& custody_wlt_rpc, const std::string payment_id)
{
  pre_hf4_api::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::request req = AUTO_VAL_INIT(req);
  req.payment_id = payment_id;
  pre_hf4_api::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::response resp = AUTO_VAL_INIT(resp);
  bool r = invoke_text_json_for_rpc(custody_wlt_rpc, "make_integrated_address", req, resp);
  CHECK_AND_ASSERT_MES(r, "", "failed to call");
  return resp.integrated_address;
}

#define TRANSFER_COMMENT "SSDVSf"
std::string transfer_(std::shared_ptr<tools::wallet2> wlt, const std::string& address, uint64_t amount)
{
  tools::wallet_rpc_server custody_wlt_rpc(wlt);
  pre_hf4_api::COMMAND_RPC_TRANSFER::request tr_req = AUTO_VAL_INIT(tr_req);
  tr_req.comment = TRANSFER_COMMENT;
  tr_req.destinations.resize(1);
  tr_req.destinations.back().address = address;
  tr_req.destinations.back().amount = amount;
  tr_req.fee = TX_DEFAULT_FEE;
  pre_hf4_api::COMMAND_RPC_TRANSFER::response tr_resp = AUTO_VAL_INIT(tr_resp);
  bool r = invoke_text_json_for_rpc(custody_wlt_rpc, "transfer", tr_req, tr_resp);
  CHECK_AND_ASSERT_MES(r, "", "failed to call");
  return tr_resp.tx_hash;
}

bool test_payment_ids_generation(tools::wallet_rpc_server& custody_wlt_rpc)
{
  //check make_integrated_address/split_integrated_address
//check auto generated payment_id
  pre_hf4_api::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::request req = AUTO_VAL_INIT(req);
  pre_hf4_api::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::response resp = AUTO_VAL_INIT(resp);
  bool r = invoke_text_json_for_rpc(custody_wlt_rpc, "make_integrated_address", req, resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");
  //custody_wlt_rpc.handle_http_request()

  pre_hf4_api::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::request req2 = AUTO_VAL_INIT(req2);
  req2.integrated_address = resp.integrated_address;
  pre_hf4_api::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::response resp2 = AUTO_VAL_INIT(resp2);
  r = invoke_text_json_for_rpc(custody_wlt_rpc, "split_integrated_address", req2, resp2);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");

  CHECK_AND_ASSERT_MES(resp2.payment_id == resp.payment_id, false, "generated paymentids missmatched");

  //check manually set payment_id
  req.payment_id = resp.payment_id;
  r = invoke_text_json_for_rpc(custody_wlt_rpc, "make_integrated_address", req, resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");
  //custody_wlt_rpc.handle_http_request()
  req2.integrated_address = resp.integrated_address;
  r = invoke_text_json_for_rpc(custody_wlt_rpc, "split_integrated_address", req2, resp2);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");

  CHECK_AND_ASSERT_MES(resp2.payment_id == req.payment_id, false, "generated paymentids missmatched");
  return true;
}

bool wallet_rpc_exchange_suite::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{

  bool r = false;
  account_base alice_acc, bob_acc, carol_acc, custody_acc;
  alice_acc.generate();
  bob_acc.generate();
  carol_acc.generate();
  custody_acc.generate();

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, alice_acc);
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, bob_acc);
  std::shared_ptr<tools::wallet2> carol_wlt = init_playtime_test_wallet(events, c, carol_acc);
  std::shared_ptr<tools::wallet2> custody_wlt = init_playtime_test_wallet(events, c, custody_acc);

  r = mine_next_pow_blocks_in_playtime(alice_wlt->get_account().get_public_address(), c, 3);
  r = mine_next_pow_blocks_in_playtime(bob_wlt->get_account().get_public_address(), c, 3);
  r = mine_next_pow_blocks_in_playtime(carol_wlt->get_account().get_public_address(), c, 3);
  //r = mine_next_pow_blocks_in_playtime(custody_wlt->get_account().get_public_address(), c, 3);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);


  // wallet RPC server
  tools::wallet_rpc_server custody_wlt_rpc(custody_wlt);

  r = test_payment_ids_generation(custody_wlt_rpc);
  CHECK_AND_ASSERT_MES(r, false, "test_payment_ids_generation() failed ");
  //======================================================================
  std::string alice_payment_id = gen_payment_id(custody_wlt_rpc);
  std::string bob_payment_id = gen_payment_id(custody_wlt_rpc);
  std::string carol_payment_id = gen_payment_id(custody_wlt_rpc);

  // generate payment id's for each wallet and deposit
  custody_wlt->refresh();
  alice_wlt->refresh();
  bob_wlt->refresh();
  carol_wlt->refresh();

#define TRANSFER_AMOUNT   COIN / 10

  std::string alice_tx1 = transfer_(alice_wlt, get_integr_addr(custody_wlt_rpc, alice_payment_id), TRANSFER_AMOUNT);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 1);

  std::string bob_tx1 = transfer_(bob_wlt, get_integr_addr(custody_wlt_rpc, bob_payment_id), TRANSFER_AMOUNT);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 1);

  std::string bob_tx2 = transfer_(bob_wlt, get_integr_addr(custody_wlt_rpc, bob_payment_id), TRANSFER_AMOUNT);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 1);

  std::string carol_tx1 = transfer_(carol_wlt, get_integr_addr(custody_wlt_rpc, carol_payment_id), TRANSFER_AMOUNT);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 1);

  std::string carol_tx2 = transfer_(carol_wlt, get_integr_addr(custody_wlt_rpc, carol_payment_id), TRANSFER_AMOUNT);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 1);

  std::string carol_tx3 = transfer_(carol_wlt, get_integr_addr(custody_wlt_rpc, carol_payment_id), TRANSFER_AMOUNT);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 1);

  CHECK_AND_ASSERT_MES(alice_tx1.size()
    && bob_tx1.size()
    && bob_tx2.size()
    && carol_tx1.size()
    && carol_tx2.size()
    && carol_tx3.size(),
    false, "One of deposit transactions wan't created"
  );

  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  custody_wlt->refresh();
  pre_hf4_api::COMMAND_RPC_GET_RECENT_TXS_AND_INFO::request req = AUTO_VAL_INIT(req);
  req.update_provision_info = true;
  req.count = 10;

  pre_hf4_api::COMMAND_RPC_GET_RECENT_TXS_AND_INFO::response resp = AUTO_VAL_INIT(resp);
  r = invoke_text_json_for_rpc(custody_wlt_rpc, "get_recent_txs_and_info", req, resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");

#define CHECK_RESPONSE_EQUAL(condition) CHECK_AND_ASSERT_MES((condition), false, "Failed check");
  
  CHECK_RESPONSE_EQUAL(resp.pi.balance == 600000000000);
  CHECK_RESPONSE_EQUAL(resp.pi.unlocked_balance == 600000000000);
  CHECK_RESPONSE_EQUAL(resp.pi.transfers_count == 6);
  CHECK_RESPONSE_EQUAL(resp.total_transfers == 6);
  CHECK_RESPONSE_EQUAL(resp.transfers.size() == 6);
  CHECK_RESPONSE_EQUAL(resp.transfers[0].comment == TRANSFER_COMMENT);
  CHECK_RESPONSE_EQUAL(resp.transfers[0].amount == TRANSFER_AMOUNT);
  CHECK_RESPONSE_EQUAL(resp.transfers[0].is_income == true);
  CHECK_RESPONSE_EQUAL(epee::string_tools::buff_to_hex_nodelimer(resp.transfers[0].payment_id) == carol_payment_id);
  CHECK_RESPONSE_EQUAL(boost::lexical_cast<std::string>(resp.transfers[0].tx_hash) == carol_tx3);

  CHECK_RESPONSE_EQUAL(resp.transfers[1].comment == TRANSFER_COMMENT);
  CHECK_RESPONSE_EQUAL(resp.transfers[1].amount == TRANSFER_AMOUNT);
  CHECK_RESPONSE_EQUAL(resp.transfers[1].is_income == true);
  CHECK_RESPONSE_EQUAL(epee::string_tools::buff_to_hex_nodelimer(resp.transfers[1].payment_id) == carol_payment_id);
  CHECK_RESPONSE_EQUAL(boost::lexical_cast<std::string>(resp.transfers[1].tx_hash) == carol_tx2);

  CHECK_RESPONSE_EQUAL(resp.transfers[2].comment == TRANSFER_COMMENT);
  CHECK_RESPONSE_EQUAL(resp.transfers[2].amount == TRANSFER_AMOUNT);
  CHECK_RESPONSE_EQUAL(resp.transfers[2].is_income == true);
  CHECK_RESPONSE_EQUAL(epee::string_tools::buff_to_hex_nodelimer(resp.transfers[2].payment_id) == carol_payment_id);
  CHECK_RESPONSE_EQUAL(boost::lexical_cast<std::string>(resp.transfers[2].tx_hash) == carol_tx1);

  CHECK_RESPONSE_EQUAL(resp.transfers[3].comment == TRANSFER_COMMENT);
  CHECK_RESPONSE_EQUAL(resp.transfers[3].amount == TRANSFER_AMOUNT);
  CHECK_RESPONSE_EQUAL(resp.transfers[3].is_income == true);
  CHECK_RESPONSE_EQUAL(epee::string_tools::buff_to_hex_nodelimer(resp.transfers[3].payment_id) == bob_payment_id);
  CHECK_RESPONSE_EQUAL(boost::lexical_cast<std::string>(resp.transfers[3].tx_hash) == bob_tx2);

  CHECK_RESPONSE_EQUAL(resp.transfers[4].comment == TRANSFER_COMMENT);
  CHECK_RESPONSE_EQUAL(resp.transfers[4].amount == TRANSFER_AMOUNT);
  CHECK_RESPONSE_EQUAL(resp.transfers[4].is_income == true);
  CHECK_RESPONSE_EQUAL(epee::string_tools::buff_to_hex_nodelimer(resp.transfers[4].payment_id) == bob_payment_id);
  CHECK_RESPONSE_EQUAL(boost::lexical_cast<std::string>(resp.transfers[4].tx_hash) == bob_tx1);

  CHECK_RESPONSE_EQUAL(resp.transfers[5].comment == TRANSFER_COMMENT);
  CHECK_RESPONSE_EQUAL(resp.transfers[5].amount == TRANSFER_AMOUNT);
  CHECK_RESPONSE_EQUAL(resp.transfers[5].is_income == true);
  CHECK_RESPONSE_EQUAL(epee::string_tools::buff_to_hex_nodelimer(resp.transfers[5].payment_id) == alice_payment_id);
  CHECK_RESPONSE_EQUAL(boost::lexical_cast<std::string>(resp.transfers[5].tx_hash) == alice_tx1);


  req.count = 10;
  req.offset = 2;
  r = invoke_text_json_for_rpc(custody_wlt_rpc, "get_recent_txs_and_info", req, resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");

  CHECK_RESPONSE_EQUAL(resp.pi.balance == 600000000000);
  CHECK_RESPONSE_EQUAL(resp.pi.unlocked_balance == 600000000000);
  CHECK_RESPONSE_EQUAL(resp.pi.transfers_count == 6);
  CHECK_RESPONSE_EQUAL(resp.total_transfers == 6);
  CHECK_RESPONSE_EQUAL(resp.transfers.size() == 4);
  CHECK_RESPONSE_EQUAL(resp.transfers[0].comment == TRANSFER_COMMENT);
  CHECK_RESPONSE_EQUAL(resp.transfers[0].amount == TRANSFER_AMOUNT);
  CHECK_RESPONSE_EQUAL(resp.transfers[0].is_income == true);
  CHECK_RESPONSE_EQUAL(epee::string_tools::buff_to_hex_nodelimer(resp.transfers[0].payment_id) == carol_payment_id);
  CHECK_RESPONSE_EQUAL(boost::lexical_cast<std::string>(resp.transfers[0].tx_hash) == carol_tx1);

  CHECK_RESPONSE_EQUAL(resp.transfers[1].comment == TRANSFER_COMMENT);
  CHECK_RESPONSE_EQUAL(resp.transfers[1].amount == TRANSFER_AMOUNT);
  CHECK_RESPONSE_EQUAL(resp.transfers[1].is_income == true);
  CHECK_RESPONSE_EQUAL(epee::string_tools::buff_to_hex_nodelimer(resp.transfers[1].payment_id) == bob_payment_id);
  CHECK_RESPONSE_EQUAL(boost::lexical_cast<std::string>(resp.transfers[1].tx_hash) == bob_tx2);

  CHECK_RESPONSE_EQUAL(resp.transfers[2].comment == TRANSFER_COMMENT);
  CHECK_RESPONSE_EQUAL(resp.transfers[2].amount == TRANSFER_AMOUNT);
  CHECK_RESPONSE_EQUAL(resp.transfers[2].is_income == true);
  CHECK_RESPONSE_EQUAL(epee::string_tools::buff_to_hex_nodelimer(resp.transfers[2].payment_id) == bob_payment_id);
  CHECK_RESPONSE_EQUAL(boost::lexical_cast<std::string>(resp.transfers[2].tx_hash) == bob_tx1);

  CHECK_RESPONSE_EQUAL(resp.transfers[3].comment == TRANSFER_COMMENT);
  CHECK_RESPONSE_EQUAL(resp.transfers[3].amount == TRANSFER_AMOUNT);
  CHECK_RESPONSE_EQUAL(resp.transfers[3].is_income == true);
  CHECK_RESPONSE_EQUAL(epee::string_tools::buff_to_hex_nodelimer(resp.transfers[3].payment_id) == alice_payment_id);
  CHECK_RESPONSE_EQUAL(boost::lexical_cast<std::string>(resp.transfers[3].tx_hash) == alice_tx1);

  //getbalance
  pre_hf4_api::COMMAND_RPC_GET_BALANCE::request gb_req = AUTO_VAL_INIT(gb_req);
  pre_hf4_api::COMMAND_RPC_GET_BALANCE::response gb_resp = AUTO_VAL_INIT(gb_resp);
  r = invoke_text_json_for_rpc(custody_wlt_rpc, "getbalance", gb_req, gb_resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");
  CHECK_RESPONSE_EQUAL(gb_resp.balance == 600000000000);
  CHECK_RESPONSE_EQUAL(gb_resp.unlocked_balance == 600000000000);

  //get_wallet_info
  pre_hf4_api::COMMAND_RPC_GET_WALLET_INFO::request gwi_req = AUTO_VAL_INIT(gwi_req);
  pre_hf4_api::COMMAND_RPC_GET_WALLET_INFO::response gwi_resp = AUTO_VAL_INIT(gwi_resp);
  r = invoke_text_json_for_rpc(custody_wlt_rpc, "get_wallet_info", gwi_req, gwi_resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");
  CHECK_RESPONSE_EQUAL(gwi_resp.current_height == 35);
  CHECK_RESPONSE_EQUAL(gwi_resp.transfer_entries_count == 6);
  CHECK_RESPONSE_EQUAL(gwi_resp.transfers_count == 6);
  CHECK_RESPONSE_EQUAL(gwi_resp.address == custody_wlt->get_account().get_public_address_str());


  //search_for_transactions
  pre_hf4_api::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS::request st_req = AUTO_VAL_INIT(st_req);
  st_req.filter_by_height = false;
  st_req.in = true;
  st_req.out = true;
  st_req.pool = true;
  st_req.tx_id = resp.transfers[1].tx_hash; //bob_tx2
  pre_hf4_api::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS::response st_resp = AUTO_VAL_INIT(st_resp);
  r = invoke_text_json_for_rpc(custody_wlt_rpc, "search_for_transactions", st_req, st_resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");
  //TODO: add more cases for search transaction in pool, in and out


  CHECK_RESPONSE_EQUAL(st_resp.in.size() == 1);
  CHECK_RESPONSE_EQUAL(st_resp.in.begin()->comment == TRANSFER_COMMENT);
  CHECK_RESPONSE_EQUAL(st_resp.in.begin()->amount == TRANSFER_AMOUNT);
  CHECK_RESPONSE_EQUAL(st_resp.in.begin()->is_income == true);
  CHECK_RESPONSE_EQUAL(epee::string_tools::buff_to_hex_nodelimer(st_resp.in.begin()->payment_id) == bob_payment_id);
  CHECK_RESPONSE_EQUAL(boost::lexical_cast<std::string>(st_resp.in.begin()->tx_hash) == bob_tx2);

  
  //get_payments
  pre_hf4_api::COMMAND_RPC_GET_PAYMENTS::request gps_req = AUTO_VAL_INIT(gps_req);
  gps_req.payment_id = carol_payment_id;
  pre_hf4_api::COMMAND_RPC_GET_PAYMENTS::response gps_resp = AUTO_VAL_INIT(gps_resp);
  r = invoke_text_json_for_rpc(custody_wlt_rpc, "get_payments", gps_req, gps_resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");

  {
    //sort by block_height to have deterministic order
    gps_resp.payments.sort([&](const pre_hf4_api::payment_details& a, const pre_hf4_api::payment_details& b) {return a.block_height < b.block_height; });

    CHECK_RESPONSE_EQUAL(gps_resp.payments.size() == 3);
    auto  it = gps_resp.payments.begin();
    CHECK_RESPONSE_EQUAL(it->amount == 100000000000);
    CHECK_RESPONSE_EQUAL(it->payment_id == carol_payment_id);
    CHECK_RESPONSE_EQUAL(it->block_height == 23);
    it++;
    CHECK_RESPONSE_EQUAL(it->amount == 100000000000);
    CHECK_RESPONSE_EQUAL(it->payment_id == carol_payment_id);
    CHECK_RESPONSE_EQUAL(it->block_height == 24);
    it++;
    CHECK_RESPONSE_EQUAL(it->amount == 100000000000);
    CHECK_RESPONSE_EQUAL(it->payment_id == carol_payment_id);
    CHECK_RESPONSE_EQUAL(it->block_height == 25);
  }



  //get_bulk_payments
  pre_hf4_api::COMMAND_RPC_GET_BULK_PAYMENTS::request gbps_req = AUTO_VAL_INIT(gbps_req);
  gbps_req.payment_ids.push_back(bob_payment_id);
  gbps_req.payment_ids.push_back(alice_payment_id);
  pre_hf4_api::COMMAND_RPC_GET_BULK_PAYMENTS::response gbps_resp = AUTO_VAL_INIT(gbps_resp);
  r = invoke_text_json_for_rpc(custody_wlt_rpc, "get_bulk_payments", gbps_req, gbps_resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");
  
  {
    //sort by block_height to have deterministic order
    gbps_resp.payments.sort([&](const pre_hf4_api::payment_details& a, const pre_hf4_api::payment_details& b) {return a.block_height < b.block_height; });

    CHECK_RESPONSE_EQUAL(gbps_resp.payments.size() == 3);
    auto  it = gbps_resp.payments.begin();
    CHECK_RESPONSE_EQUAL(it->amount == 100000000000);
    CHECK_RESPONSE_EQUAL(it->payment_id == alice_payment_id);
    CHECK_RESPONSE_EQUAL(it->block_height == 20);
    it++;
    CHECK_RESPONSE_EQUAL(it->amount == 100000000000);
    CHECK_RESPONSE_EQUAL(it->payment_id == bob_payment_id);
    CHECK_RESPONSE_EQUAL(it->block_height == 21);
    it++;
    CHECK_RESPONSE_EQUAL(it->amount == 100000000000);
    CHECK_RESPONSE_EQUAL(it->payment_id == bob_payment_id);
    CHECK_RESPONSE_EQUAL(it->block_height == 22);
  }

  return true;
}
