// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "wallet_rpc_tests.h"
#include "wallet_test_core_proxy.h"
#include "currency_core/currency_core.h"
#include "currency_core/bc_offers_service.h"
#include "rpc/core_rpc_server.h"
#include "currency_protocol/currency_protocol_handler.h"


#include "../../src/wallet/wallet_rpc_server.h"
#include "offers_helper.h"
#include "random_helper.h"

using namespace currency;

template<typename server_t>
struct transport
{
  server_t& m_rpc_srv;
  transport(server_t& rpc_srv) :m_rpc_srv(rpc_srv)
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



template<typename request_t, typename response_t, typename t_rpc_server>
bool invoke_text_json_for_rpc(t_rpc_server& srv, const std::string& method_name, const request_t& req, response_t& resp)
{
  transport<t_rpc_server> tr(srv);

  bool r = epee::net_utils::invoke_http_json_rpc("/json_rpc", method_name, req, resp, tr);
  return r;
}

template<typename request_t, typename response_t>
bool invoke_text_json_for_wallet(std::shared_ptr<tools::wallet2> wlt, const std::string& method_name, const request_t& req, response_t& resp)
{
  tools::wallet_rpc_server wlt_rpc_wrapper(wlt);
  return invoke_text_json_for_rpc(wlt_rpc_wrapper, method_name, req, resp);
}

template<typename request_t, typename response_t>
bool invoke_text_json_for_core(currency::core& c, const std::string& method_name, const request_t& req, response_t& resp)
{
  currency::t_currency_protocol_handler<currency::core> m_cprotocol(c, nullptr);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > p2p(m_cprotocol);
  bc_services::bc_offers_service of(nullptr); 

  currency::core_rpc_server core_rpc_wrapper(c, p2p, of);
  core_rpc_wrapper.set_ignore_connectivity_status(true);
  return invoke_text_json_for_rpc(core_rpc_wrapper, method_name, req, resp);
}



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
  //CHECK_AND_ASSERT_MES(currency::count_type_in_variant_container<tx_receiver>(pche->tx.extra) == 1, false, "tx_receiver: incorrect count of items");
  //CHECK_AND_ASSERT_MES(currency::count_type_in_variant_container<tx_payer>(pche->tx.extra) == 0, false, "tx_payer: incorrect count of items");


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
  //CHECK_AND_ASSERT_MES(currency::count_type_in_variant_container<tx_receiver>(pche->tx.extra) == 0, false, "tx_receiver: incorrect count of items");
  //CHECK_AND_ASSERT_MES(currency::count_type_in_variant_container<tx_payer>(pche->tx.extra) == 1, false, "tx_payer: incorrect count of items");


  return true;
}
//------------------------------------------------------------------------------
wallet_rpc_alias_tests::wallet_rpc_alias_tests()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_alias_tests, configure_core);
  REGISTER_CALLBACK_METHOD(wallet_rpc_alias_tests, c1);

  m_hardforks.set_hardfork_height(1, 1);
  m_hardforks.set_hardfork_height(2, 1);
  m_hardforks.set_hardfork_height(3, 1);
  m_hardforks.set_hardfork_height(4, 1);
}

bool wallet_rpc_alias_tests::generate(std::vector<test_event_entry>& events) const
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

bool wallet_rpc_alias_tests::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);

  miner_wlt->refresh();

  {
    tools::wallet_public::COMMAND_RPC_REGISTER_ALIAS::request req = AUTO_VAL_INIT(req);
    tools::wallet_public::COMMAND_RPC_REGISTER_ALIAS::response rsp = AUTO_VAL_INIT(rsp);
#define ALIAS_FOR_TEST "monero"

    req.al.alias = ALIAS_FOR_TEST;
    req.al.details.address = miner_wlt->get_account().get_public_address_str();
    req.al.details.comment = "XMR";
    r = invoke_text_json_for_wallet(miner_wlt, "register_alias", req, rsp);
    CHECK_AND_ASSERT_MES(r, false, "failed to invoke_text_json_for_wallet");
  }
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 3);
  CHECK_AND_ASSERT_MES(r, false, "failed to mine_next_pow_blocks_in_playtime");

  {
    currency::COMMAND_RPC_GET_ALIAS_DETAILS::request req = AUTO_VAL_INIT(req);
    currency::COMMAND_RPC_GET_ALIAS_DETAILS::response rsp = AUTO_VAL_INIT(rsp);

    req.alias = ALIAS_FOR_TEST;
    r = invoke_text_json_for_core(c, "get_alias_details", req, rsp);
    CHECK_AND_ASSERT_MES(r, false, "failed to invoke_text_json_for_wallet");
    CHECK_AND_ASSERT_MES(rsp.status == API_RETURN_CODE_OK, false, "failed to invoke_text_json_for_wallet");

    CHECK_AND_ASSERT_MES(rsp.alias_details.address == miner_wlt->get_account().get_public_address_str(), false, "failed to invoke_text_json_for_wallet");
  }

  miner_wlt->refresh();

  {
    tools::wallet_public::COMMAND_RPC_UPDATE_ALIAS::request req = AUTO_VAL_INIT(req);
    tools::wallet_public::COMMAND_RPC_UPDATE_ALIAS::response rsp = AUTO_VAL_INIT(rsp);

    req.al.alias = ALIAS_FOR_TEST;
    req.al.details.address = alice_wlt->get_account().get_public_address_str();
    req.al.details.comment = "XMR of Alice";
    r = invoke_text_json_for_wallet(miner_wlt, "update_alias", req, rsp);
    CHECK_AND_ASSERT_MES(r, false, "failed to invoke_text_json_for_wallet");
  }

  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 3);
  CHECK_AND_ASSERT_MES(r, false, "failed to mine_next_pow_blocks_in_playtime");

  {
    currency::COMMAND_RPC_GET_ALIAS_DETAILS::request req = AUTO_VAL_INIT(req);
    currency::COMMAND_RPC_GET_ALIAS_DETAILS::response rsp = AUTO_VAL_INIT(rsp);

    req.alias = ALIAS_FOR_TEST;
    r = invoke_text_json_for_core(c, "get_alias_details", req, rsp);
    CHECK_AND_ASSERT_MES(r, false, "failed to invoke_text_json_for_wallet");
    CHECK_AND_ASSERT_MES(rsp.status == API_RETURN_CODE_OK, false, "failed to invoke_text_json_for_wallet");

    CHECK_AND_ASSERT_MES(rsp.alias_details.address == alice_wlt->get_account().get_public_address_str(), false, "failed to invoke_text_json_for_wallet");
  }


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

//------------------------------------------------------------------------------

wallet_true_rpc_pos_mining::wallet_true_rpc_pos_mining()
{
  REGISTER_CALLBACK_METHOD(wallet_true_rpc_pos_mining, c1);
}
//------------------------------------------------------------------------------
bool wallet_true_rpc_pos_mining::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  DO_CALLBACK(events, "c1");

  return true;
}

#include <boost/program_options/detail/parsers.hpp>

bool wallet_true_rpc_pos_mining::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet_with_true_http_rpc(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet_with_true_http_rpc(events, c, ALICE_ACC_IDX);

  /*currency::t_currency_protocol_handler<currency::core> m_cprotocol(c, nullptr);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > p2p(m_cprotocol);
  bc_services::bc_offers_service bos(nullptr);
  bos.set_disabled(true);
  currency::core_rpc_server core_rpc_wrapper(c, p2p, bos);
  core_rpc_wrapper.set_ignore_connectivity_status(true);
  
  boost::program_options::options_description desc_options;
  currency::core_rpc_server::init_options(desc_options);
  boost::program_options::variables_map vm{};
  char* argv[] = {"--rpc-bind-ip=127.0.0.1", "--rpc-bind-port=33777"};
  boost::program_options::store(boost::program_options::parse_command_line(sizeof argv / sizeof argv[0], argv, desc_options), vm);
  r = core_rpc_wrapper.init(vm);
  CHECK_AND_ASSERT_MES(r, false, "rpc server init failed");
  r = core_rpc_wrapper.run(1, false);
  CHECK_AND_ASSERT_MES(r, 1, "rpc server run failed");
  auto slh = epee::misc_utils::create_scope_leave_handler([&](){
      core_rpc_wrapper.deinit();
    });


  auto http_core_proxy = std::shared_ptr<tools::i_core_proxy>(new tools::default_http_core_proxy());
  http_core_proxy->set_connectivity(5000, 1);
  CHECK_AND_ASSERT_MES(http_core_proxy->set_connection_addr("127.0.0.1:33777"), false, "");
  CHECK_AND_ASSERT_MES(http_core_proxy->check_connection(), false, "no connection");

  miner_wlt->set_core_proxy(http_core_proxy);*/

  uint64_t top_height = c.get_top_block_height();

  size_t blocks_fetched = 0;
  miner_wlt->refresh(blocks_fetched);
  CHECK_AND_ASSERT_EQ(blocks_fetched, top_height);

  alice_wlt->refresh(blocks_fetched);
  CHECK_AND_ASSERT_EQ(blocks_fetched, top_height);

  //uint64_t balance_unlocked = 1, balance_awaiting_in = 1, balance_awaiting_out = 1, balance_mined = 1;
  //uint64_t miner_initial_balance = miner_wlt->balance(balance_unlocked, balance_awaiting_in, balance_awaiting_out, balance_mined);
  //CHECK_AND_ASSERT_EQ(miner_initial_balance, PREMINE_AMOUNT + CURRENCY_BLOCK_REWARD * top_height);
  uint64_t miner_amount = PREMINE_AMOUNT + CURRENCY_BLOCK_REWARD * top_height;
  uint64_t expected_unlocked = miner_amount - (CURRENCY_MINED_MONEY_UNLOCK_WINDOW - 1) * COIN;
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*miner_wlt, "miner", miner_amount, INVALID_BALANCE_VAL, expected_unlocked, 0, 0), false, "");

  CHECK_AND_ASSERT_MES(miner_wlt->try_mint_pos(), false, "try_mint_pos failed");

  CHECK_AND_ASSERT_EQ(c.get_top_block_height(), top_height + 1);

  miner_wlt->refresh(blocks_fetched);
  CHECK_AND_ASSERT_EQ(blocks_fetched, 1);

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*miner_wlt, "miner", miner_amount + COIN), false, "");

  uint64_t amount_to_alice = MK_TEST_COINS(500);
  miner_wlt->transfer(amount_to_alice, m_accounts[ALICE_ACC_IDX].get_public_address());

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "enexpected pool txs count: " << c.get_pool_transactions_count());
  CHECK_AND_ASSERT_MES(miner_wlt->try_mint_pos(), false, "try_mint_pos failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "enexpected pool txs count: " << c.get_pool_transactions_count());

  miner_wlt->refresh(blocks_fetched);
  CHECK_AND_ASSERT_EQ(blocks_fetched, 1);

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*miner_wlt, "miner", miner_amount + 2 * COIN - amount_to_alice - TESTS_DEFAULT_FEE), false, "");

  // Alice

  //alice_wlt->refresh(blocks_fetched);
  //CHECK_AND_ASSERT_EQ(blocks_fetched, 2);

  //CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", amount_to_alice, 0, 0, 0, 0), false, "");

  //r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  //CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  //alice_wlt->refresh(blocks_fetched);
  //CHECK_AND_ASSERT_EQ(blocks_fetched, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  //
  //CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", amount_to_alice, 0, amount_to_alice, 0, 0), false, "");

  //std::this_thread::yield();
  //std::this_thread::sleep_for( std::chrono::milliseconds(1) );
  //CHECK_AND_ASSERT_MES(alice_wlt->try_mint_pos(), false, "try_mint_pos failed");

  //alice_wlt->refresh(blocks_fetched);
  //CHECK_AND_ASSERT_EQ(blocks_fetched, 1);

  //CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", amount_to_alice + CURRENCY_BLOCK_REWARD, 0, 0, 0, 0), false, "");

  return true;
}



wallet_rpc_thirdparty_custody::wallet_rpc_thirdparty_custody()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_thirdparty_custody, c1);
}
//------------------------------------------------------------------------------
bool wallet_rpc_thirdparty_custody::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  DO_CALLBACK(events, "c1");

  return true;
}
//------------------------------------------------------------------------------

#define TEST_TOKEN_NAME  "TEST TOKEN"
#define TEST_TOKEN_METAINFO  "Metainfo"
#define TEST_TOKEN_TICKER  "TT"
#define TEST_TOKEN_MAX_SUPPLY      2000
#define TEST_TOKEN_CURRENT_SUPPLY  1000



typedef boost::variant<crypto::secret_key, crypto::eth_secret_key> secret_key_v;
typedef boost::variant<crypto::public_key, crypto::eth_public_key> public_key_v;


template<typename t_response>
bool ext_sign_and_send_asset_tx(const t_response& rsp_with_data, tools::wallet_rpc_server& attached_wallet_rpc, const secret_key_v& signer_v, const public_key_v& verifier)
{
  bool r = false;
  tools::wallet_public::COMMAND_ASSET_SEND_EXT_SIGNED_TX::request send_signed_req = AUTO_VAL_INIT(send_signed_req);
  if (signer_v.type() == typeid(crypto::secret_key))
  {
    //schnor sig
    const crypto::secret_key& signer = boost::get<crypto::secret_key>(signer_v);
    r = crypto::generate_schnorr_sig(rsp_with_data.tx_id, signer, send_signed_req.regular_sig);
    CHECK_AND_ASSERT_MES(r, false, "generate_schnorr_sig failed");

    // instant verification, just in case
    r = crypto::verify_schnorr_sig(rsp_with_data.tx_id, boost::get<crypto::public_key>(verifier), send_signed_req.regular_sig);
    CHECK_AND_ASSERT_MES(r, false, "verify_schnorr_sig failed");
  }
  else if (signer_v.type() == typeid(crypto::eth_secret_key))
  {
    ////ecdsa sig
    const crypto::eth_secret_key& signer = boost::get<crypto::eth_secret_key>(signer_v);
    r = crypto::generate_eth_signature(rsp_with_data.tx_id, signer, send_signed_req.eth_sig);
    CHECK_AND_ASSERT_MES(r, false, "generate_eth_signature failed");

    r = crypto::verify_eth_signature(rsp_with_data.tx_id, boost::get<crypto::eth_public_key>(verifier), send_signed_req.eth_sig);
    CHECK_AND_ASSERT_MES(r, false, "verify_eth_signature failed");

  }
  else
  {
    return false;
  }

  send_signed_req.unsigned_tx     = rsp_with_data.data_for_external_signing->unsigned_tx;
  send_signed_req.expected_tx_id  = rsp_with_data.tx_id;
  send_signed_req.finalized_tx    = rsp_with_data.data_for_external_signing->finalized_tx;
  send_signed_req.unlock_transfers_on_fail = true;
  tools::wallet_public::COMMAND_ASSET_SEND_EXT_SIGNED_TX::response send_signed_resp{};

  r = invoke_text_json_for_rpc(attached_wallet_rpc, "send_ext_signed_asset_tx", send_signed_req, send_signed_resp);
  CHECK_AND_ASSERT_MES(r, false, "RPC send_ext_signed_asset_tx failed");
  CHECK_AND_ASSERT_MES(send_signed_resp.status == API_RETURN_CODE_OK, false, "RPC send_ext_signed_asset_tx failed: " << send_signed_resp.status);
  return true;
}



bool wallet_rpc_thirdparty_custody::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
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
  //r = mine_next_pow_blocks_in_playtime(custody_wlt->get_account().get_public_address(), c, 3);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);


  // wallet RPC server
  tools::wallet_rpc_server miner_wlt_rpc(miner_wlt);

  miner_wlt->refresh();
  struct tools::wallet_public::COMMAND_ASSETS_DEPLOY::request req = AUTO_VAL_INIT(req);
  struct tools::wallet_public::COMMAND_ASSETS_DEPLOY::response resp = AUTO_VAL_INIT(resp);
  
  req.asset_descriptor.decimal_point = 1;
  req.asset_descriptor.full_name = TEST_TOKEN_NAME;
  req.asset_descriptor.hidden_supply = false;
  req.asset_descriptor.meta_info = TEST_TOKEN_METAINFO;
  req.asset_descriptor.ticker = TEST_TOKEN_TICKER;
  req.asset_descriptor.total_max_supply = TEST_TOKEN_MAX_SUPPLY;
  req.asset_descriptor.current_supply = TEST_TOKEN_CURRENT_SUPPLY;

  r = invoke_text_json_for_rpc(miner_wlt_rpc, "deploy_asset", req, resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");

  if (resp.new_asset_id == currency::null_pkey)
  {
    LOG_ERROR("Failed to deploy asset");
    return false;
  }

  const crypto::public_key asset_id = resp.new_asset_id;

  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 3);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");
   
  // core RPC server
  currency::t_currency_protocol_handler<currency::core> cprotocol(c, NULL);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > dummy_p2p(cprotocol);
  bc_services::bc_offers_service dummy_bc(nullptr);
  currency::core_rpc_server core_rpc(c, dummy_p2p, dummy_bc);
  currency::COMMAND_RPC_GET_ASSET_INFO::request gai_req = AUTO_VAL_INIT(gai_req);
  currency::COMMAND_RPC_GET_ASSET_INFO::response gai_resp = AUTO_VAL_INIT(gai_resp);
  gai_req.asset_id = resp.new_asset_id;
  r = invoke_text_json_for_rpc(core_rpc, "get_asset_info", gai_req, gai_resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");
  CHECK_AND_ASSERT_MES(gai_resp.status == API_RETURN_CODE_OK, false, "failed to call");

  CHECK_AND_ASSERT_MES(gai_resp.asset_descriptor.decimal_point == 1, false, "Wrong asset descriptor");
  CHECK_AND_ASSERT_MES(gai_resp.asset_descriptor.full_name == TEST_TOKEN_NAME, false, "Wrong asset descriptor");
  CHECK_AND_ASSERT_MES(gai_resp.asset_descriptor.hidden_supply == false, false, "Wrong asset descriptor");
  CHECK_AND_ASSERT_MES(gai_resp.asset_descriptor.meta_info == TEST_TOKEN_METAINFO, false, "Wrong asset descriptor");
  CHECK_AND_ASSERT_MES(gai_resp.asset_descriptor.ticker == TEST_TOKEN_TICKER, false, "Wrong asset descriptor");
  CHECK_AND_ASSERT_MES(gai_resp.asset_descriptor.total_max_supply == TEST_TOKEN_MAX_SUPPLY, false, "Wrong asset descriptor");
  CHECK_AND_ASSERT_MES(gai_resp.asset_descriptor.current_supply == TEST_TOKEN_CURRENT_SUPPLY, false, "Wrong asset descriptor");

  alice_wlt->refresh();
  // wallet RPC server
  tools::wallet_rpc_server alice_wlt_rpc(alice_wlt);

  tools::wallet_public::COMMAND_ATTACH_ASSET_DESCRIPTOR::request att_req = AUTO_VAL_INIT(att_req);
  tools::wallet_public::COMMAND_ATTACH_ASSET_DESCRIPTOR::response att_resp = AUTO_VAL_INIT(att_resp);
  att_req.asset_id = resp.new_asset_id;
  att_req.do_attach = true;
  r = invoke_text_json_for_rpc(alice_wlt_rpc, "attach_asset_descriptor", att_req, att_resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");
  CHECK_AND_ASSERT_MES(att_resp.status == API_RETURN_CODE_OK, false, "failed to call");


  //let's emit it, but transfer actually to Bob, by using Alice wallet as attache
  tools::wallet_public::COMMAND_ASSETS_EMIT::request emm_req = AUTO_VAL_INIT(emm_req);
  tools::wallet_public::COMMAND_ASSETS_EMIT::response emm_resp = AUTO_VAL_INIT(emm_resp);
#define COINS_TO_TRANSFER 10

  emm_req.asset_id = resp.new_asset_id;
  emm_req.destinations.push_back(tools::wallet_public::transfer_destination{ COINS_TO_TRANSFER, bob_wlt->get_account().get_public_address_str(), emm_req.asset_id });
  r = invoke_text_json_for_rpc(alice_wlt_rpc, "emit_asset", emm_req, emm_resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");
  if (!emm_resp.data_for_external_signing)
  {
    LOG_ERROR("Missing data_for_external_signing");
    return false;
  }

  r = ext_sign_and_send_asset_tx(emm_resp, alice_wlt_rpc, miner_wlt->get_account().get_keys().spend_secret_key, miner_wlt->get_account().get_keys().account_address.spend_public_key);
  CHECK_AND_ASSERT_MES(r, false, "failed to call ext_sign_and_send_asset_tx");

  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 3);


  //check bob wallet
  tools::wallet_rpc_server bob_wlt_rpc(bob_wlt);
  tools::wallet_public::COMMAND_RPC_GET_BALANCE::request balance_req = AUTO_VAL_INIT(balance_req);
  tools::wallet_public::COMMAND_RPC_GET_BALANCE::response balance_resp = AUTO_VAL_INIT(balance_resp);
  r = invoke_text_json_for_rpc(bob_wlt_rpc, "getbalance", balance_req, balance_resp);
  CHECK_AND_ASSERT_MES(r, false, "RPC getbalance failed");
  r = std::find_if(balance_resp.balances.begin(), balance_resp.balances.end(), [&](tools::wallet_public::asset_balance_entry& e){ return e.asset_info.asset_id == asset_id; }) == balance_resp.balances.end();
  CHECK_AND_ASSERT_MES(r, false, "found asset " << resp.new_asset_id << " which is unexpected");


  tools::wallet_public::COMMAND_ASSETS_WHITELIST_ADD::request wtl_req = AUTO_VAL_INIT(wtl_req);
  tools::wallet_public::COMMAND_ASSETS_WHITELIST_ADD::response wtl_resp = AUTO_VAL_INIT(wtl_resp);
  wtl_req.asset_id = resp.new_asset_id;
  r = invoke_text_json_for_rpc(bob_wlt_rpc, "assets_whitelist_add", wtl_req, wtl_resp);
  CHECK_AND_ASSERT_MES(r, false, "RPC assets_whitelist_add failed");
  CHECK_AND_ASSERT_MES(wtl_resp.status == API_RETURN_CODE_OK, false, "RPC status failed");
  bob_wlt->refresh();

  balance_req = AUTO_VAL_INIT(balance_req);
  balance_resp = AUTO_VAL_INIT(balance_resp);
  r = invoke_text_json_for_rpc(bob_wlt_rpc, "getbalance", balance_req, balance_resp);
  CHECK_AND_ASSERT_MES(r, false, "RPC getbalance failed");
  

  bool found_asset = false;
  for (auto& bal : balance_resp.balances)
  {
    if (bal.asset_info.asset_id == resp.new_asset_id)
    {
      found_asset = true;
      CHECK_EQ(bal.total, COINS_TO_TRANSFER);
    }
  }
  CHECK_AND_ASSERT_MES(found_asset, false, "Asset with id " << resp.new_asset_id << " was not found");



  //transfer ownership of the asset to an ETH key
  //let's change the owner to ecdsa
  crypto::eth_secret_key eth_sk_2{};
  crypto::eth_public_key eth_pk_2{};
  r = crypto::generate_eth_key_pair(eth_sk_2, eth_pk_2);
  CHECK_AND_ASSERT_MES(r, false, "generate_eth_key_pair failed");

  tools::wallet_public::COMMAND_TRANSFER_ASSET_OWNERSHIP::request req_own = AUTO_VAL_INIT(req_own);
  tools::wallet_public::COMMAND_TRANSFER_ASSET_OWNERSHIP::response res_own = AUTO_VAL_INIT(res_own);
  req_own.asset_id = resp.new_asset_id;
  req_own.new_owner_eth_pub_key = eth_pk_2;


  r = invoke_text_json_for_rpc(alice_wlt_rpc, "transfer_asset_ownership", req_own, res_own);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");
  if (!res_own.data_for_external_signing)
  {
    LOG_ERROR("Missing data_for_external_signing");
    return false;
  }

  r = ext_sign_and_send_asset_tx(res_own, alice_wlt_rpc, miner_wlt->get_account().get_keys().spend_secret_key, miner_wlt->get_account().get_keys().account_address.spend_public_key);
  CHECK_AND_ASSERT_MES(r, false, "failed to call ext_sign_and_send_asset_tx");

  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 3);


  //now make another tx and see of ownership got changed
  emm_resp = AUTO_VAL_INIT(emm_resp);
  r = invoke_text_json_for_rpc(alice_wlt_rpc, "emit_asset", emm_req, emm_resp);
  CHECK_AND_ASSERT_MES(r, false, "invoke_text_json_for_rpc failed");
  CHECK_AND_ASSERT_MES(res_own.data_for_external_signing, false, "data_for_external_signing is missing");

  r = ext_sign_and_send_asset_tx(emm_resp, alice_wlt_rpc, eth_sk_2, eth_pk_2);
  CHECK_AND_ASSERT_MES(r, false, "failed to call ext_sign_and_send_asset_tx");

  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 3);
  
  bob_wlt->refresh();
  r = invoke_text_json_for_rpc(bob_wlt_rpc, "getbalance", balance_req, balance_resp);
  CHECK_AND_ASSERT_MES(r, false, "RPC send_ext_signed_asset_tx failed: ");

  found_asset = false;
  for (auto& bal : balance_resp.balances)
  {
    if (bal.asset_info.asset_id == resp.new_asset_id)
    {
      found_asset = true;
      CHECK_AND_ASSERT_MES(bal.total == 2 * COINS_TO_TRANSFER, false, "Amount is unexpected");
    }
  }
  CHECK_AND_ASSERT_MES(found_asset, false, "Asset not found ");


  // Transfer ownership to Carol (standard owner pub key)

  tools::wallet_rpc_server carol_wlt_rpc(carol_wlt);
  req_own = AUTO_VAL_INIT(req_own);
  res_own = AUTO_VAL_INIT(res_own);
  req_own.asset_id = asset_id;
  req_own.new_owner = carol_acc.get_public_address().spend_public_key;
  alice_wlt->refresh();
  r = invoke_text_json_for_rpc(alice_wlt_rpc, "transfer_asset_ownership", req_own, res_own);
  CHECK_AND_ASSERT_MES(r, false, "invoke_text_json_for_rpc failed");
  CHECK_AND_ASSERT_MES(res_own.data_for_external_signing, false, "data_for_external_signing is missing");
  // externally sign and send transfer ownership tx
  r = ext_sign_and_send_asset_tx(res_own, alice_wlt_rpc, eth_sk_2, eth_pk_2);
  CHECK_AND_ASSERT_MES(r, false, "ext_sign_and_send_asset_tx failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "unexpected pool txs count: " << c.get_pool_transactions_count());

  // Miner still has some asset coins, transfer them to Carol
  miner_wlt->refresh();
  tools::wallet_public::COMMAND_RPC_TRANSFER::request miner_tr_req = AUTO_VAL_INIT(miner_tr_req);
  tools::wallet_public::COMMAND_RPC_TRANSFER::request miner_tr_res = AUTO_VAL_INIT(miner_tr_res);
  miner_tr_req.fee = TESTS_DEFAULT_FEE;
  miner_tr_req.destinations.push_back({COINS_TO_TRANSFER * 7ull, carol_acc.get_public_address_str(), asset_id});
  r = invoke_text_json_for_rpc(miner_wlt_rpc, "transfer", miner_tr_req, miner_tr_res);
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 2, false, "unexpected pool txs count: " << c.get_pool_transactions_count());
  // confirm it
  CHECK_AND_ASSERT_MES(mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW), false, "");
  // make sure the pool is now empty
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "unexpected pool txs count: " << c.get_pool_transactions_count());

  // Carol shouldn't have this asset whitelisted and present in balance
  carol_wlt->refresh();
  balance_req = AUTO_VAL_INIT(balance_req);
  balance_resp = AUTO_VAL_INIT(balance_resp);
  r = invoke_text_json_for_rpc(carol_wlt_rpc, "getbalance", balance_req, balance_resp);
  CHECK_AND_ASSERT_MES(r, false, "RPC getbalance failed");
  CHECK_EQ(balance_resp.balance, 0);
  r = std::find_if(balance_resp.balances.begin(), balance_resp.balances.end(), [&](tools::wallet_public::asset_balance_entry& e){ return e.asset_info.asset_id == asset_id; }) == balance_resp.balances.end();
  CHECK_AND_ASSERT_MES(r, false, "asset was found, which in unexpected");
  // make sure she has asset_id among own assets in spite of that
  auto& carol_own_assets = carol_wlt->get_own_assets();
  CHECK_EQ(carol_own_assets.size(), 1);
  CHECK_EQ(carol_own_assets.count(asset_id), 1);

  // whitelist and re-check
  wtl_req = AUTO_VAL_INIT(wtl_req);
  wtl_resp = AUTO_VAL_INIT(wtl_resp);
  wtl_req.asset_id = asset_id;
  CHECK_AND_ASSERT_MES(invoke_text_json_for_rpc(carol_wlt_rpc, "assets_whitelist_add", wtl_req, wtl_resp), false, "");
  CHECK_AND_ASSERT_MES(wtl_resp.status == API_RETURN_CODE_OK, false, "RPC failed");

  // now the asset must show up in the balance
  balance_req = AUTO_VAL_INIT(balance_req);
  balance_resp = AUTO_VAL_INIT(balance_resp);
  r = invoke_text_json_for_rpc(carol_wlt_rpc, "getbalance", balance_req, balance_resp);
  CHECK_AND_ASSERT_MES(r, false, "RPC getbalance failed");
  CHECK_EQ(balance_resp.balance, 0); // the balance for native coin is still zero
  auto it = std::find_if(balance_resp.balances.begin(), balance_resp.balances.end(), [&](tools::wallet_public::asset_balance_entry& e){ return e.asset_info.asset_id == asset_id; });
  CHECK_AND_ASSERT_MES(it != balance_resp.balances.end(), false, "asset was not found, which in unexpected");
  CHECK_EQ(it->total, COINS_TO_TRANSFER * 7);

  return true;
}

//------------------------------------------------------------------------------

wallet_rpc_cold_signing::wallet_rpc_cold_signing()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_cold_signing, c1);
}

bool wallet_rpc_cold_signing::generate(std::vector<test_event_entry>& events) const
{
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");

  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  MAKE_TX(events, tx_0, miner_acc, alice_acc, MK_TEST_COINS(100), blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK(events, "c1");

  return true;
}

bool make_cold_signing_transaction(tools::wallet_rpc_server& rpc_wo, tools::wallet_rpc_server& rpc,
  const tools::wallet_public::COMMAND_RPC_TRANSFER::request& req_transfer)
{
  bool r = false;

  // watch only creates a transaction
  tools::wallet_public::COMMAND_RPC_TRANSFER::response res_transfer{};
  r = invoke_text_json_for_rpc(rpc_wo, "transfer", req_transfer, res_transfer);
  CHECK_AND_ASSERT_MES(r, false, "RPC 'transfer' failed");

    
  // secure wallet with full keys set signs the transaction
  tools::wallet_public::COMMAND_SIGN_TRANSFER::request req_sign{};
  req_sign.tx_unsigned_hex = res_transfer.tx_unsigned_hex;
  tools::wallet_public::COMMAND_SIGN_TRANSFER::response res_sign{};
  r = invoke_text_json_for_rpc(rpc, "sign_transfer", req_sign, res_sign);
  CHECK_AND_ASSERT_MES(r, false, "RPC 'sign_transfer' failed");

    
  // watch only wallet submits it
  tools::wallet_public::COMMAND_SUBMIT_TRANSFER::request req_submit{};
  req_submit.tx_signed_hex = res_sign.tx_signed_hex;
  tools::wallet_public::COMMAND_SUBMIT_TRANSFER::response res_submit{};
  r = invoke_text_json_for_rpc(rpc_wo, "submit_transfer", req_submit, res_submit);
  CHECK_AND_ASSERT_MES(r, false, "RPC 'submit_transfer' failed");

  CHECK_AND_ASSERT_EQ(res_sign.tx_hash, res_submit.tx_hash);

  return true;
}

void wallet_rpc_cold_signing::set_wallet_options(std::shared_ptr<tools::wallet2> w)
{
  set_playtime_test_wallet_options(w);
  w->set_concise_mode(false);
}

bool wallet_rpc_cold_signing::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);

  miner_wlt->refresh();
  bob_wlt->refresh();
  check_balance_via_wallet(*bob_wlt, "Bob", 0, 0, 0, 0, 0);

  account_base alice_acc_wo = m_accounts[ALICE_ACC_IDX];
  alice_acc_wo.make_account_watch_only();
  std::shared_ptr<tools::wallet2> alice_wlt_wo = init_playtime_test_wallet(events, c, alice_acc_wo);

  alice_wlt_wo->refresh();
  check_balance_via_wallet(*alice_wlt_wo, "Alice (WO)", MK_TEST_COINS(100), 0, MK_TEST_COINS(100), 0, 0);

  // Alice: perform initial save-load to properly initialize internal wallet2 structures
  boost::filesystem::remove(epee::string_tools::cut_off_extension(m_wallet_filename) + L".outkey2ki");
  alice_wlt_wo->reset_password(m_wallet_password);
  alice_wlt_wo->store(m_wallet_filename);
  alice_wlt_wo.reset(new tools::wallet2);
  alice_wlt_wo->load(m_wallet_filename, m_wallet_password);
  alice_wlt_wo->set_core_proxy(m_core_proxy);
  alice_wlt_wo->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());
  set_wallet_options(alice_wlt_wo);

  tools::wallet_rpc_server alice_rpc(alice_wlt);
  std::shared_ptr<tools::wallet_rpc_server> alice_rpc_wo_ptr{ new tools::wallet_rpc_server(alice_wlt_wo) };

  // send a cold-signed transaction
  tools::wallet_public::COMMAND_RPC_TRANSFER::request req{};
  req.destinations.emplace_back();
  req.destinations.back().amount = MK_TEST_COINS(50);
  req.destinations.back().address = m_accounts[BOB_ACC_IDX].get_public_address_str();
  req.fee = TESTS_DEFAULT_FEE;
  req.mixin = 10;
  r = make_cold_signing_transaction(*alice_rpc_wo_ptr, alice_rpc, req);
  CHECK_AND_ASSERT_MES(r, false, "make_cold_signing_transaction failed");

  uint64_t bob_expected_balance = req.destinations.back().amount;

  // mine few blocks to unlock Alice's coins
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "unexpected pool txs count: " << c.get_pool_transactions_count());
  
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "tx pool is not empty: " << c.get_pool_transactions_count());

  uint64_t alice_expected_balance = MK_TEST_COINS(50) - TESTS_DEFAULT_FEE;
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("after sending the first cold-signed tx:", "Alice", alice_wlt_wo, alice_expected_balance, true,
    CURRENCY_MINED_MONEY_UNLOCK_WINDOW, alice_expected_balance, 0, 0, 0), false, "");

  // perform save-load before next transaction
  alice_wlt_wo->reset_password(m_wallet_password);
  alice_wlt_wo->store(m_wallet_filename);

  alice_rpc_wo_ptr.reset();
  alice_wlt_wo.reset(new tools::wallet2);
  alice_wlt_wo->load(m_wallet_filename, m_wallet_password);
  alice_wlt_wo->set_core_proxy(m_core_proxy);
  alice_wlt_wo->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());
  set_wallet_options(alice_wlt_wo);
  alice_rpc_wo_ptr.reset(new tools::wallet_rpc_server(alice_wlt_wo));

  size_t blocks_fetched = 0;
  bool stub0{}, stub1{};
  std::atomic_bool stub2{};
  alice_wlt_wo->refresh(blocks_fetched, stub0, stub1, stub2);
  CHECK_AND_ASSERT_EQ(blocks_fetched, 0);


  // send a cold-signed transaction
  req = tools::wallet_public::COMMAND_RPC_TRANSFER::request{};
  req.destinations.emplace_back();
  req.destinations.back().amount = alice_expected_balance - TESTS_DEFAULT_FEE;
  req.destinations.back().address = m_accounts[BOB_ACC_IDX].get_public_address_str();
  req.fee = TESTS_DEFAULT_FEE;
  req.mixin = 0;
  r = make_cold_signing_transaction(*alice_rpc_wo_ptr, alice_rpc, req);
  CHECK_AND_ASSERT_MES(r, false, "make_cold_signing_transaction failed");
  
  bob_expected_balance += req.destinations.back().amount;

  // additionally, send some coins from miner to Alice
  miner_wlt->refresh();
  miner_wlt->transfer(MK_TEST_COINS(7), alice_wlt_wo->get_account().get_public_address());
  alice_expected_balance = MK_TEST_COINS(7);

  // mine few blocks to unlock Alice's coins
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 2, false, "enexpected pool txs count: " << c.get_pool_transactions_count());
  
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Tx pool is not empty: " << c.get_pool_transactions_count());

  // check Alice's balance
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("after sending the second cold-signed tx:", "Alice", alice_wlt_wo, alice_expected_balance, true,
    CURRENCY_MINED_MONEY_UNLOCK_WINDOW, alice_expected_balance, 0, 0, 0), false, "");


  // send a cold-signed transaction
  req = tools::wallet_public::COMMAND_RPC_TRANSFER::request{};
  req.destinations.emplace_back();
  req.destinations.back().amount = alice_expected_balance - TESTS_DEFAULT_FEE;
  req.destinations.back().address = m_accounts[BOB_ACC_IDX].get_public_address_str();
  req.fee = TESTS_DEFAULT_FEE;
  req.mixin = 0;
  r = make_cold_signing_transaction(*alice_rpc_wo_ptr, alice_rpc, req);
  CHECK_AND_ASSERT_MES(r, false, "make_cold_signing_transaction failed");
  
  bob_expected_balance += req.destinations.back().amount;
  alice_expected_balance = 0;

  // mine few blocks to unlock Alice's coins
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "enexpected pool txs count: " << c.get_pool_transactions_count());
  
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Tx pool is not empty: " << c.get_pool_transactions_count());

  // check Alice's balance
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("after sending the second cold-signed tx:", "Alice", alice_wlt_wo, alice_expected_balance, true,
    CURRENCY_MINED_MONEY_UNLOCK_WINDOW, alice_expected_balance, 0, 0, 0), false, "");



  // finally, check Bob's balance
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, bob_expected_balance, true, 3 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW, bob_expected_balance, 0, 0, 0), false, "");

  return true;
}
