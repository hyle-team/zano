// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <numeric>
#include <set>

#include "chaingen.h"
#include "wallet_rpc_tests.h"
#include "wallet_test_core_proxy.h"
#include "currency_core/currency_core.h"
#include "currency_core/bc_offers_service.h"
#include "currency_core/crypto_config.h" // CRYPTO_HDS_GW_* hash domain separators
#include "rpc/core_rpc_server.h"
#include "currency_protocol/currency_protocol_handler.h"
#include "wallet/wallet2.h"
#include "wallet/wallet_debug_events_definitions.h"

#include "../../src/wallet/wallet_rpc_server.h"
#include "offers_helper.h"
#include "random_helper.h"

using namespace currency;

namespace
{
  bool gateway_rpc_proxy_decrypt_history(currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::response& resp, const std::string& gw_address, const crypto::secret_key& view_secret_key)
  {
    currency::address_v v_addr = {};
    currency::payment_id_t dummy_pid = {};
    CHECK_AND_ASSERT_MES(currency::get_account_address_and_payment_id_from_str(v_addr, dummy_pid, gw_address), false, "gateway_rpc_proxy_decrypt_history: bad gw address");
    CHECK_AND_ASSERT_MES(v_addr.type() == typeid(currency::gateway_address_id_type), false, "gateway_rpc_proxy_decrypt_history: not a gw address");
    currency::gateway_address_id_type addr_id = boost::get<currency::gateway_address_id_type>(v_addr);

    CHECK_AND_ASSERT_MES(resp.transactions.size() == resp.raw_txs.size(), false, "gateway_rpc_proxy_decrypt_history: transactions/raw_txs size mismatch");
    auto raw_it = resp.raw_txs.begin();
    for (auto& wti : resp.transactions)
    {
      std::string blob;
      CHECK_AND_ASSERT_MES(epee::string_tools::parse_hexstr_to_binbuff(*raw_it, blob), false, "gateway_rpc_proxy_decrypt_history: bad raw tx hex");
      ++raw_it;
      currency::transaction tx = AUTO_VAL_INIT(tx);
      CHECK_AND_ASSERT_MES(currency::parse_and_validate_tx_from_blob(blob, tx), false, "gateway_rpc_proxy_decrypt_history: parse tx failed");
      wti.tx = tx;
      CHECK_AND_ASSERT_MES(currency::gateway_decrypt_wti(view_secret_key, addr_id, wti), false, "gateway_rpc_proxy_decrypt_history: decrypt failed");
    }
    resp.raw_txs.clear();
    return true;
  }
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
  miner_wlt_rpc.set_flag_allow_legacy_payment_id_size(true);
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


  // TODO: add cases for miner_wlt_rpc.set_flag_allow_legacy_payment_id_size(false)

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
  std::string alice_integrated_address = get_account_address_as_str(m_accounts[ALICE_ACC_IDX].get_public_address(), payment_id);

  // wallet RPC server
  boost::program_options::options_description wallet_desc_options;
  tools::wallet_rpc_server::init_options(wallet_desc_options);
  boost::program_options::variables_map vm_allow_legacy_pid_size_wallet;
  const char* const argv_w[] = {"", "--allow-legacy-payment-id-size", "--rpc-bind-port=0"};
  boost::program_options::store(boost::program_options::parse_command_line(sizeof argv_w / sizeof argv_w[0], argv_w, wallet_desc_options), vm_allow_legacy_pid_size_wallet);
  tools::wallet_rpc_server miner_wlt_rpc(miner_wlt);
  CHECK_AND_ASSERT_SUCCESS(miner_wlt_rpc.init(vm_allow_legacy_pid_size_wallet));
  epee::json_rpc::error je;
  tools::wallet_rpc_server::connection_context ctx;

  tools::wallet_public::COMMAND_RPC_TRANSFER::request  req = AUTO_VAL_INIT(req);
  req.fee = TESTS_DEFAULT_FEE;
  req.mixin = 0;
  currency::transfer_destination tds = AUTO_VAL_INIT(tds);
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
  currency::transfer_destination tds2 = AUTO_VAL_INIT(tds2);
  tds2.address = m_accounts[ALICE_ACC_IDX].get_public_address_str();
  tds2.amount = MK_TEST_COINS(7);
  req.destinations.push_back(tds2);

  req.payment_id = "Zuckerman";
  je = AUTO_VAL_INIT(je);
  r = miner_wlt_rpc.on_transfer(req, res, je, ctx);
  CHECK_AND_ASSERT_MES(!r, false, "RPC call not failed as expected");

  // 4. standard address + external payment id => failure (payment id is deprecated)
  req.payment_id = "0A13fFEe";
  std::string payment_id2;
  epee::string_tools::parse_hexstr_to_binbuff(req.payment_id, payment_id2);
  je = AUTO_VAL_INIT(je);
  r = miner_wlt_rpc.on_transfer(req, res, je, ctx);
  CHECK_AND_ASSERT_MES(!r, false, "RPC call not failed as expected");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "enexpected pool txs count: " << c.get_pool_transactions_count());

  alice_wlt->refresh();

  // check the transfer has been received
  payments.clear();
  alice_wlt->get_payments(payment_id2, payments);
  CHECK_AND_ASSERT_EQ(payments.size(), 0);
  
  alice_wlt->get_payments(payment_id, payments);
  CHECK_AND_ASSERT_MES(payments.size() == 1, false, "Invalid payments count: " << payments.size());
  CHECK_AND_ASSERT_MES(payments.front().m_amount == MK_TEST_COINS(3), false, "Invalid payment");
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
  currency::transfer_destination tds = AUTO_VAL_INIT(tds);
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

  // make sure tx_receiver and tx_payer is not present
  std::shared_ptr<const transaction_chain_entry> pche = c.get_blockchain_storage().get_tx_chain_entry(td.tx_hash());
  CHECK_AND_ASSERT_MES(currency::count_type_in_variant_container<tx_receiver>(pche->tx.extra) == 0, false, "tx_receiver: incorrect count of items");
  CHECK_AND_ASSERT_MES(currency::count_type_in_variant_container<tx_payer>(pche->tx.extra) == 0, false, "tx_payer: incorrect count of items");


  /*CHECK_AND_ASSERT_MES(r, false, "RPC call failed, code: " << je.code << ", msg: " << je.message);

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
  //CHECK_AND_ASSERT_MES(currency::count_type_in_variant_container<tx_payer>(pche->tx.extra) == 1, false, "tx_payer: incorrect count of items");*/


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


std::string gen_payment_id_as_hex_str(tools::wallet_rpc_server& custody_wlt_rpc)
{
  pre_hf4_api::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::request req = AUTO_VAL_INIT(req);
  pre_hf4_api::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::response resp = AUTO_VAL_INIT(resp);
  bool r = invoke_text_json_for_rpc(custody_wlt_rpc, "make_integrated_address", req, resp);
  CHECK_AND_ASSERT_MES(r, "", "failed to call");
  return resp.payment_id;
}

std::string get_integr_addr(tools::wallet_rpc_server& custody_wlt_rpc, const std::string payment_id_hex_str)
{
  pre_hf4_api::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::request req = AUTO_VAL_INIT(req);
  req.payment_id = payment_id_hex_str;
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
  tr_req.fee = TESTS_DEFAULT_FEE;
  pre_hf4_api::COMMAND_RPC_TRANSFER::response tr_resp = AUTO_VAL_INIT(tr_resp);
  bool r = invoke_text_json_for_rpc(custody_wlt_rpc, "transfer", tr_req, tr_resp);
  CHECK_AND_ASSERT_MES(r, "", "failed to call");
  return tr_resp.tx_hash;
}


std::string transfer_new(std::shared_ptr<tools::wallet2> wlt, const std::string& address, uint64_t amount)
{
  tools::wallet_rpc_server custody_wlt_rpc(wlt);
  tools::wallet_public::COMMAND_RPC_TRANSFER::request tr_req = AUTO_VAL_INIT(tr_req);
  tr_req.comment = TRANSFER_COMMENT;
  tr_req.destinations.resize(1);
  tr_req.destinations.back().address = address;
  tr_req.destinations.back().amount = amount;
  tr_req.fee = TESTS_DEFAULT_FEE;
  tools::wallet_public::COMMAND_RPC_TRANSFER::response tr_resp = AUTO_VAL_INIT(tr_resp);
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

  bool hf4_active = c.get_blockchain_storage().is_hardfork_active(ZANO_HARDFORK_04_ZARCANUM);
  bool hf6_active = c.get_blockchain_storage().is_hardfork_active(ZANO_HARDFORK_06);

  // wallet RPC server
  tools::wallet_rpc_server custody_wlt_rpc(custody_wlt);

  r = test_payment_ids_generation(custody_wlt_rpc);
  CHECK_AND_ASSERT_MES(r, false, "test_payment_ids_generation() failed ");
  //======================================================================
  std::string alice_payment_id_hex_str  = gen_payment_id_as_hex_str(custody_wlt_rpc);
  std::string alice_payment_id;
  CHECK_AND_ASSERT_TRUE(epee::string_tools::parse_hexstr_to_binbuff(alice_payment_id_hex_str, alice_payment_id));
  std::string bob_payment_id_hex_str    = gen_payment_id_as_hex_str(custody_wlt_rpc);
  std::string bob_payment_id;
  CHECK_AND_ASSERT_TRUE(epee::string_tools::parse_hexstr_to_binbuff(bob_payment_id_hex_str, bob_payment_id));
  std::string carol_payment_id_hex_str  = gen_payment_id_as_hex_str(custody_wlt_rpc);
  std::string carol_payment_id;
  CHECK_AND_ASSERT_TRUE(epee::string_tools::parse_hexstr_to_binbuff(carol_payment_id_hex_str, carol_payment_id));

  // generate payment id's for each wallet and deposit
  custody_wlt->refresh();
  alice_wlt->refresh();
  bob_wlt->refresh();
  carol_wlt->refresh();

#define TRANSFER_AMOUNT   COIN / 10

  std::string alice_tx1 = transfer_(alice_wlt, get_integr_addr(custody_wlt_rpc, alice_payment_id_hex_str), TRANSFER_AMOUNT);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 1);

  std::string bob_tx1 = transfer_(bob_wlt, get_integr_addr(custody_wlt_rpc, bob_payment_id_hex_str), TRANSFER_AMOUNT);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 1);

  std::string bob_tx2 = transfer_(bob_wlt, get_integr_addr(custody_wlt_rpc, bob_payment_id_hex_str), TRANSFER_AMOUNT);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 1);

  std::string carol_tx1 = transfer_(carol_wlt, get_integr_addr(custody_wlt_rpc, carol_payment_id_hex_str), TRANSFER_AMOUNT);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 1);

  std::string carol_tx2 = transfer_(carol_wlt, get_integr_addr(custody_wlt_rpc, carol_payment_id_hex_str), TRANSFER_AMOUNT);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 1);

  std::string carol_tx3 = transfer_(carol_wlt, get_integr_addr(custody_wlt_rpc, carol_payment_id_hex_str), TRANSFER_AMOUNT);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 1);

  LOG_PRINT_GREEN_L0("");
  LOG_PRINT_GREEN_L0("alice_tx1: " << alice_tx1);
  LOG_PRINT_GREEN_L0("bob_tx1: "   << bob_tx1);
  LOG_PRINT_GREEN_L0("bob_tx2: "   << bob_tx2);
  LOG_PRINT_GREEN_L0("carol_tx1: " << carol_tx1);
  LOG_PRINT_GREEN_L0("carol_tx2: " << carol_tx2);
  LOG_PRINT_GREEN_L0("carol_tx3: " << carol_tx3);
  LOG_PRINT_GREEN_L0("");

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
  
  CHECK_AND_ASSERT_EQ(resp.pi.balance, 600000000000);
  CHECK_AND_ASSERT_EQ(resp.pi.unlocked_balance, 600000000000);
  CHECK_AND_ASSERT_EQ(resp.pi.transfers_count, 6);
  CHECK_AND_ASSERT_EQ(resp.total_transfers, 6);
  CHECK_AND_ASSERT_EQ(resp.transfers.size(), 6);
  // below: tx_comment is temporary disabled, @#@#TODO -- sowle
  //CHECK_RESPONSE_EQUAL(resp.transfers[0].comment == TRANSFER_COMMENT);
  CHECK_AND_ASSERT_EQ(resp.transfers[0].amount, TRANSFER_AMOUNT);
  CHECK_AND_ASSERT_EQ(resp.transfers[0].is_income, true);
  CHECK_AND_ASSERT_EQ(resp.transfers[0].payment_id, carol_payment_id);
  CHECK_AND_ASSERT_EQ(boost::lexical_cast<std::string>(resp.transfers[0].tx_hash), carol_tx3);

  //CHECK_RESPONSE_EQUAL(resp.transfers[1].comment == TRANSFER_COMMENT);
  CHECK_AND_ASSERT_EQ(resp.transfers[1].amount, TRANSFER_AMOUNT);
  CHECK_AND_ASSERT_EQ(resp.transfers[1].is_income, true);
  CHECK_AND_ASSERT_EQ(resp.transfers[1].payment_id, carol_payment_id);
  CHECK_AND_ASSERT_EQ(boost::lexical_cast<std::string>(resp.transfers[1].tx_hash), carol_tx2);

  //CHECK_RESPONSE_EQUAL(resp.transfers[2].comment == TRANSFER_COMMENT);
  CHECK_AND_ASSERT_EQ(resp.transfers[2].amount, TRANSFER_AMOUNT);
  CHECK_AND_ASSERT_EQ(resp.transfers[2].is_income, true);
  CHECK_AND_ASSERT_EQ(resp.transfers[2].payment_id, carol_payment_id);
  CHECK_AND_ASSERT_EQ(boost::lexical_cast<std::string>(resp.transfers[2].tx_hash), carol_tx1);

  //CHECK_RESPONSE_EQUAL(resp.transfers[3].comment == TRANSFER_COMMENT);
  CHECK_AND_ASSERT_EQ(resp.transfers[3].amount, TRANSFER_AMOUNT);
  CHECK_AND_ASSERT_EQ(resp.transfers[3].is_income, true);
  CHECK_AND_ASSERT_EQ(resp.transfers[3].payment_id, bob_payment_id);
  CHECK_AND_ASSERT_EQ(boost::lexical_cast<std::string>(resp.transfers[3].tx_hash), bob_tx2);

  //CHECK_RESPONSE_EQUAL(resp.transfers[4].comment == TRANSFER_COMMENT);
  CHECK_AND_ASSERT_EQ(resp.transfers[4].amount, TRANSFER_AMOUNT);
  CHECK_AND_ASSERT_EQ(resp.transfers[4].is_income, true);
  CHECK_AND_ASSERT_EQ(resp.transfers[4].payment_id, bob_payment_id);
  CHECK_AND_ASSERT_EQ(boost::lexical_cast<std::string>(resp.transfers[4].tx_hash), bob_tx1);

  //CHECK_RESPONSE_EQUAL(resp.transfers[5].comment == TRANSFER_COMMENT);
  CHECK_AND_ASSERT_EQ(resp.transfers[5].amount, TRANSFER_AMOUNT);
  CHECK_AND_ASSERT_EQ(resp.transfers[5].is_income, true);
  CHECK_AND_ASSERT_EQ(resp.transfers[5].payment_id, alice_payment_id);
  CHECK_AND_ASSERT_EQ(boost::lexical_cast<std::string>(resp.transfers[5].tx_hash), alice_tx1);


  req.count = 10;
  req.offset = 2;
  r = invoke_text_json_for_rpc(custody_wlt_rpc, "get_recent_txs_and_info", req, resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");

  CHECK_AND_ASSERT_EQ(resp.pi.balance, 600000000000);
  CHECK_AND_ASSERT_EQ(resp.pi.unlocked_balance, 600000000000);
  CHECK_AND_ASSERT_EQ(resp.pi.transfers_count, 6);
  CHECK_AND_ASSERT_EQ(resp.total_transfers, 6);
  CHECK_AND_ASSERT_EQ(resp.transfers.size(), 4);
  //CHECK_RESPONSE_EQUAL(resp.transfers[0].comment == TRANSFER_COMMENT);
  CHECK_AND_ASSERT_EQ(resp.transfers[0].amount, TRANSFER_AMOUNT);
  CHECK_AND_ASSERT_EQ(resp.transfers[0].is_income, true);
  CHECK_AND_ASSERT_EQ(resp.transfers[0].payment_id, carol_payment_id);
  CHECK_AND_ASSERT_EQ(boost::lexical_cast<std::string>(resp.transfers[0].tx_hash), carol_tx1);

  //CHECK_RESPONSE_EQUAL(resp.transfers[1].comment == TRANSFER_COMMENT);
  CHECK_AND_ASSERT_EQ(resp.transfers[1].amount, TRANSFER_AMOUNT);
  CHECK_AND_ASSERT_EQ(resp.transfers[1].is_income, true);
  CHECK_AND_ASSERT_EQ(resp.transfers[1].payment_id, bob_payment_id);
  CHECK_AND_ASSERT_EQ(boost::lexical_cast<std::string>(resp.transfers[1].tx_hash), bob_tx2);

  //CHECK_RESPONSE_EQUAL(resp.transfers[2].comment == TRANSFER_COMMENT);
  CHECK_AND_ASSERT_EQ(resp.transfers[2].amount, TRANSFER_AMOUNT);
  CHECK_AND_ASSERT_EQ(resp.transfers[2].is_income, true);
  CHECK_AND_ASSERT_EQ(resp.transfers[2].payment_id, bob_payment_id);
  CHECK_AND_ASSERT_EQ(boost::lexical_cast<std::string>(resp.transfers[2].tx_hash), bob_tx1);

  //CHECK_RESPONSE_EQUAL(resp.transfers[3].comment == TRANSFER_COMMENT);
  CHECK_AND_ASSERT_EQ(resp.transfers[3].amount, TRANSFER_AMOUNT);
  CHECK_AND_ASSERT_EQ(resp.transfers[3].is_income, true);
  CHECK_AND_ASSERT_EQ(resp.transfers[3].payment_id, alice_payment_id);
  CHECK_AND_ASSERT_EQ(boost::lexical_cast<std::string>(resp.transfers[3].tx_hash), alice_tx1);

  //getbalance
  pre_hf4_api::COMMAND_RPC_GET_BALANCE::request gb_req = AUTO_VAL_INIT(gb_req);
  pre_hf4_api::COMMAND_RPC_GET_BALANCE::response gb_resp = AUTO_VAL_INIT(gb_resp);
  r = invoke_text_json_for_rpc(custody_wlt_rpc, "getbalance", gb_req, gb_resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");
  CHECK_AND_ASSERT_EQ(gb_resp.balance, 600000000000);
  CHECK_AND_ASSERT_EQ(gb_resp.unlocked_balance, 600000000000);

  //get_wallet_info
  pre_hf4_api::COMMAND_RPC_GET_WALLET_INFO::request gwi_req = AUTO_VAL_INIT(gwi_req);
  pre_hf4_api::COMMAND_RPC_GET_WALLET_INFO::response gwi_resp = AUTO_VAL_INIT(gwi_resp);
  r = invoke_text_json_for_rpc(custody_wlt_rpc, "get_wallet_info", gwi_req, gwi_resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");
  CHECK_AND_ASSERT_EQ(gwi_resp.current_height, 35);
  CHECK_AND_ASSERT_EQ(gwi_resp.transfer_entries_count, 6);
  CHECK_AND_ASSERT_EQ(gwi_resp.transfers_count, 6);
  CHECK_AND_ASSERT_EQ(gwi_resp.address, custody_wlt->get_account().get_public_address_str());


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


  CHECK_AND_ASSERT_EQ(st_resp.in.size(), 1);
  //CHECK_RESPONSE_EQUAL(st_resp.in.begin()->comment == TRANSFER_COMMENT);
  CHECK_AND_ASSERT_EQ(st_resp.in.begin()->amount, TRANSFER_AMOUNT);
  CHECK_AND_ASSERT_EQ(st_resp.in.begin()->is_income, true);
  CHECK_AND_ASSERT_EQ(st_resp.in.begin()->fee, TESTS_DEFAULT_FEE);
  CHECK_AND_ASSERT_EQ(st_resp.in.begin()->payment_id, bob_payment_id);
  CHECK_AND_ASSERT_EQ(boost::lexical_cast<std::string>(st_resp.in.begin()->tx_hash), bob_tx2);

  
  // refresh Alice's wallet so it gets all new txs before load cycle to test serialization
  alice_wlt->refresh();

  // perform a load cycle for Alice's wallet to check more potential issues
  alice_wlt->reset_password(m_alice_wallet_password);
  alice_wlt->store(m_alice_wallet_filename);
  alice_wlt.reset(new tools::wallet2);
  alice_wlt->load(m_alice_wallet_filename, m_alice_wallet_password);
  alice_wlt->set_core_proxy(m_core_proxy);
  alice_wlt->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());
  set_playtime_test_wallet_options(alice_wlt, false);
  tools::wallet_rpc_server alice_rpc(alice_wlt);

  // search_for_transactions (outgoing)
  st_req = {};
  st_req.filter_by_height = false;
  st_req.in = false;
  st_req.out = true;
  st_req.pool = true;
  st_resp = {};
  r = invoke_text_json_for_rpc(alice_rpc, "search_for_transactions", st_req, st_resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");

  CHECK_AND_ASSERT_EQ(st_resp.out.size(), 1);
  CHECK_AND_ASSERT_EQ(st_resp.out.front().amount, TRANSFER_AMOUNT + TESTS_DEFAULT_FEE); // outgoing wti includes fee into amount/subtransfers
  CHECK_AND_ASSERT_EQ(st_resp.out.front().is_income, false);
  CHECK_AND_ASSERT_EQ(st_resp.out.front().fee, TESTS_DEFAULT_FEE);
  if (!hf6_active)
  {
    // this check is not valid since HF6, as we decided to stop accounting pid for outgoing wti due to complications with subtransfers -- sowle
    CHECK_AND_ASSERT_EQ(st_resp.out.front().payment_id, alice_payment_id);
  }
  CHECK_AND_ASSERT_EQ(boost::lexical_cast<std::string>(st_resp.out.front().tx_hash), alice_tx1);

  // search_for_transactions2 (outgoing)
  tools::wallet_public::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS::request st2_req{};
  st2_req.filter_by_height = false;
  st2_req.in = false;
  st2_req.out = true;
  st2_req.pool = true;
  tools::wallet_public::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS::response st2_resp{};
  r = invoke_text_json_for_rpc(alice_rpc, "search_for_transactions2", st2_req, st2_resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");

  CHECK_AND_ASSERT_EQ(st2_resp.out.size(), 1);
  CHECK_AND_ASSERT_EQ(st2_resp.out.front().fee, TESTS_DEFAULT_FEE);
  CHECK_AND_ASSERT_EQ(st2_resp.out.front().subtransfers_by_pid.size(), 1);
  if (!hf6_active)
  {
    // this check is not valid since HF6, as we decided to stop accounting pid for outgoing wti due to complications with subtransfers -- sowle
    CHECK_AND_ASSERT_EQ(st2_resp.out.front().subtransfers_by_pid.front().payment_id, alice_payment_id);
  }
  CHECK_AND_ASSERT_EQ(st2_resp.out.front().subtransfers_by_pid.front().subtransfers.size(), 1);
  CHECK_AND_ASSERT_EQ(st2_resp.out.front().subtransfers_by_pid.front().subtransfers.front().amount, TRANSFER_AMOUNT + TESTS_DEFAULT_FEE);
  CHECK_AND_ASSERT_EQ(st2_resp.out.front().subtransfers_by_pid.front().subtransfers.front().is_income, false);
  CHECK_AND_ASSERT_EQ(st2_resp.out.front().subtransfers_by_pid.front().subtransfers.front().asset_id, native_coin_asset_id);


  //get_payments
  pre_hf4_api::COMMAND_RPC_GET_PAYMENTS::request gps_req{};
  gps_req.payment_id = carol_payment_id_hex_str;
  pre_hf4_api::COMMAND_RPC_GET_PAYMENTS::response gps_resp{};
  r = invoke_text_json_for_rpc(custody_wlt_rpc, "get_payments", gps_req, gps_resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");

  {
    //sort by block_height to have deterministic order
    gps_resp.payments.sort([&](const pre_hf4_api::payment_details& a, const pre_hf4_api::payment_details& b) {return a.block_height < b.block_height; });

    CHECK_AND_ASSERT_EQ(gps_resp.payments.size(), 3);
    auto  it = gps_resp.payments.begin();
    CHECK_AND_ASSERT_EQ(it->amount, 100000000000);
    CHECK_AND_ASSERT_EQ(it->payment_id, carol_payment_id_hex_str);
    CHECK_AND_ASSERT_EQ(it->block_height, 23);
    it++;
    CHECK_AND_ASSERT_EQ(it->amount, 100000000000);
    CHECK_AND_ASSERT_EQ(it->payment_id, carol_payment_id_hex_str);
    CHECK_AND_ASSERT_EQ(it->block_height, 24);
    it++;
    CHECK_AND_ASSERT_EQ(it->amount, 100000000000);
    CHECK_AND_ASSERT_EQ(it->payment_id, carol_payment_id_hex_str);
    CHECK_AND_ASSERT_EQ(it->block_height, 25);
  }



  //get_bulk_payments
  pre_hf4_api::COMMAND_RPC_GET_BULK_PAYMENTS::request gbps_req{};
  gbps_req.payment_ids.push_back(bob_payment_id_hex_str);
  gbps_req.payment_ids.push_back(alice_payment_id_hex_str);
  pre_hf4_api::COMMAND_RPC_GET_BULK_PAYMENTS::response gbps_resp{};
  r = invoke_text_json_for_rpc(custody_wlt_rpc, "get_bulk_payments", gbps_req, gbps_resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");
  
  {
    //sort by block_height to have deterministic order
    gbps_resp.payments.sort([&](const pre_hf4_api::payment_details& a, const pre_hf4_api::payment_details& b) {return a.block_height < b.block_height; });

    CHECK_AND_ASSERT_EQ(gbps_resp.payments.size(), 3);
    auto  it = gbps_resp.payments.begin();
    CHECK_AND_ASSERT_EQ(it->amount, 100000000000);
    CHECK_AND_ASSERT_EQ(it->payment_id, alice_payment_id_hex_str);
    CHECK_AND_ASSERT_EQ(it->block_height, 20);
    it++;
    CHECK_AND_ASSERT_EQ(it->amount, 100000000000);
    CHECK_AND_ASSERT_EQ(it->payment_id, bob_payment_id_hex_str);
    CHECK_AND_ASSERT_EQ(it->block_height, 21);
    it++;
    CHECK_AND_ASSERT_EQ(it->amount, 100000000000);
    CHECK_AND_ASSERT_EQ(it->payment_id, bob_payment_id_hex_str);
    CHECK_AND_ASSERT_EQ(it->block_height, 22);
  }


  // since HF4 confidential assets can be received against a payment id. Make sure both get_payments and
  // get_bulk_payments report the received asset via payment_details::payment_subtransfers.
  if (hf4_active)
  {
    tools::wallet_rpc_server miner_wlt_rpc(miner_wlt);
    miner_wlt->refresh();

    // miner deploys a new asset (whole supply goes to miner itself by default)
    const uint64_t asset_supply = 1000;
    const uint64_t asset_amount = 700; // amount to be sent to custody
    tools::wallet_public::COMMAND_ASSETS_DEPLOY::request dep_req{};
    dep_req.asset_descriptor.current_supply   = asset_supply;
    dep_req.asset_descriptor.total_max_supply = asset_supply;
    dep_req.asset_descriptor.decimal_point    = 0;
    dep_req.asset_descriptor.full_name        = "exchange suite asset";
    dep_req.asset_descriptor.ticker           = "ESA";
    tools::wallet_public::COMMAND_ASSETS_DEPLOY::response dep_resp{};
    r = invoke_text_json_for_rpc(miner_wlt_rpc, "deploy_asset", dep_req, dep_resp);
    CHECK_AND_ASSERT_MES(r, false, "deploy_asset failed");
    crypto::public_key asset_id = dep_resp.new_asset_id;

    r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
    miner_wlt->refresh();

    // miner sends the asset (no native coins) to custody's integrated address, i.e. against a payment id
    std::string asset_payment_id_hex_str = gen_payment_id_as_hex_str(custody_wlt_rpc);
    std::string asset_payment_id;
    CHECK_AND_ASSERT_TRUE(epee::string_tools::parse_hexstr_to_binbuff(asset_payment_id_hex_str, asset_payment_id));

    tools::wallet_public::COMMAND_RPC_TRANSFER::request tr_req{};
    tr_req.destinations.emplace_back(currency::transfer_destination{ asset_amount, get_integr_addr(custody_wlt_rpc, asset_payment_id_hex_str), asset_id });
    tr_req.fee = TESTS_DEFAULT_FEE;
    tr_req.mixin = 0;
    tools::wallet_public::COMMAND_RPC_TRANSFER::response tr_resp{};
    r = invoke_text_json_for_rpc(miner_wlt_rpc, "transfer", tr_req, tr_resp);
    CHECK_AND_ASSERT_MES(r, false, "asset transfer failed");

    r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
    custody_wlt->refresh();

    // get_payments must report the received asset as a subtransfer (native amount stays 0)
    {
      tools::wallet_public::COMMAND_RPC_GET_PAYMENTS::request gp_req{};
      gp_req.payment_id = asset_payment_id_hex_str;
      tools::wallet_public::COMMAND_RPC_GET_PAYMENTS::response gp_resp{};
      r = invoke_text_json_for_rpc(custody_wlt_rpc, "get_payments", gp_req, gp_resp);
      CHECK_AND_ASSERT_MES(r, false, "get_payments failed");

      CHECK_AND_ASSERT_EQ(gp_resp.payments.size(), 1);
      const auto& p = gp_resp.payments.front();
      CHECK_AND_ASSERT_EQ(p.payment_id, asset_payment_id_hex_str);
      CHECK_AND_ASSERT_EQ(p.amount, 0); // no native coins were sent
      CHECK_AND_ASSERT_EQ(p.payment_subtransfers.size(), 1);
      CHECK_AND_ASSERT_EQ(p.payment_subtransfers.front().asset_id, asset_id);
      CHECK_AND_ASSERT_EQ(p.payment_subtransfers.front().amount, asset_amount);
    }

    // get_bulk_payments must report the same
    {
      tools::wallet_public::COMMAND_RPC_GET_BULK_PAYMENTS::request gbp_req{};
      gbp_req.payment_ids.push_back(asset_payment_id_hex_str);
      tools::wallet_public::COMMAND_RPC_GET_BULK_PAYMENTS::response gbp_resp{};
      r = invoke_text_json_for_rpc(custody_wlt_rpc, "get_bulk_payments", gbp_req, gbp_resp);
      CHECK_AND_ASSERT_MES(r, false, "get_bulk_payments failed");

      CHECK_AND_ASSERT_EQ(gbp_resp.payments.size(), 1);
      const auto& p = gbp_resp.payments.front();
      CHECK_AND_ASSERT_EQ(p.payment_id, asset_payment_id_hex_str);
      CHECK_AND_ASSERT_EQ(p.amount, 0);
      CHECK_AND_ASSERT_EQ(p.payment_subtransfers.size(), 1);
      CHECK_AND_ASSERT_EQ(p.payment_subtransfers.front().asset_id, asset_id);
      CHECK_AND_ASSERT_EQ(p.payment_subtransfers.front().amount, asset_amount);
    }

    // native-only payment: payment_subtransfers must stay empty, native amount goes to 'amount'
    {
      const uint64_t native_amount = COIN / 5;
      std::string native_payment_id_hex_str = gen_payment_id_as_hex_str(custody_wlt_rpc);

      tools::wallet_public::COMMAND_RPC_TRANSFER::request ntr_req{};
      ntr_req.destinations.emplace_back(currency::transfer_destination{ native_amount, get_integr_addr(custody_wlt_rpc, native_payment_id_hex_str) });
      ntr_req.fee = TESTS_DEFAULT_FEE;
      ntr_req.mixin = 0;
      tools::wallet_public::COMMAND_RPC_TRANSFER::response ntr_resp{};
      r = invoke_text_json_for_rpc(miner_wlt_rpc, "transfer", ntr_req, ntr_resp);
      CHECK_AND_ASSERT_MES(r, false, "native transfer failed");

      r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
      custody_wlt->refresh();

      tools::wallet_public::COMMAND_RPC_GET_PAYMENTS::request gp_req{};
      gp_req.payment_id = native_payment_id_hex_str;
      tools::wallet_public::COMMAND_RPC_GET_PAYMENTS::response gp_resp{};
      r = invoke_text_json_for_rpc(custody_wlt_rpc, "get_payments", gp_req, gp_resp);
      CHECK_AND_ASSERT_MES(r, false, "get_payments failed");
      CHECK_AND_ASSERT_EQ(gp_resp.payments.size(), 1);
      CHECK_AND_ASSERT_EQ(gp_resp.payments.front().amount, native_amount);
      CHECK_AND_ASSERT_EQ(gp_resp.payments.front().payment_subtransfers.size(), 0);

      tools::wallet_public::COMMAND_RPC_GET_BULK_PAYMENTS::request gbp_req{};
      gbp_req.payment_ids.push_back(native_payment_id_hex_str);
      tools::wallet_public::COMMAND_RPC_GET_BULK_PAYMENTS::response gbp_resp{};
      r = invoke_text_json_for_rpc(custody_wlt_rpc, "get_bulk_payments", gbp_req, gbp_resp);
      CHECK_AND_ASSERT_MES(r, false, "get_bulk_payments failed");
      CHECK_AND_ASSERT_EQ(gbp_resp.payments.size(), 1);
      CHECK_AND_ASSERT_EQ(gbp_resp.payments.front().amount, native_amount);
      CHECK_AND_ASSERT_EQ(gbp_resp.payments.front().payment_subtransfers.size(), 0);
    }
  }


  // native+asset in one tx under one payment id works since HF4; HF6-specific here is the pid mechanism: a short
  // integrated-address pid is carried per-output as an intrinsic pid (HF6+) instead of a single tx-wide legacy pid.
  // Check both the native and asset outputs get regrouped under that one pid by get_payments / get_bulk_payments.
  if (hf6_active)
  {
    tools::wallet_rpc_server miner_wlt_rpc(miner_wlt);
    miner_wlt->refresh();

    const uint64_t asset_supply  = 1000;
    const uint64_t asset_amount  = 500;
    const uint64_t native_amount = COIN / 4;

    // miner deploys another asset (whole supply goes to miner itself by default)
    tools::wallet_public::COMMAND_ASSETS_DEPLOY::request dep_req{};
    dep_req.asset_descriptor.current_supply   = asset_supply;
    dep_req.asset_descriptor.total_max_supply = asset_supply;
    dep_req.asset_descriptor.decimal_point    = 0;
    dep_req.asset_descriptor.full_name        = "exchange suite asset 2";
    dep_req.asset_descriptor.ticker           = "ESA2";
    tools::wallet_public::COMMAND_ASSETS_DEPLOY::response dep_resp{};
    r = invoke_text_json_for_rpc(miner_wlt_rpc, "deploy_asset", dep_req, dep_resp);
    CHECK_AND_ASSERT_MES(r, false, "deploy_asset failed");
    crypto::public_key asset_id = dep_resp.new_asset_id;

    r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
    miner_wlt->refresh();

    // single tx to one integrated address (i.e. one payment id) carrying both native coins and the asset
    std::string mixed_payment_id_hex_str = gen_payment_id_as_hex_str(custody_wlt_rpc);
    std::string integr_addr = get_integr_addr(custody_wlt_rpc, mixed_payment_id_hex_str);

    tools::wallet_public::COMMAND_RPC_TRANSFER::request tr_req{};
    tr_req.destinations.emplace_back(currency::transfer_destination{ native_amount, integr_addr });            // native coins
    tr_req.destinations.emplace_back(currency::transfer_destination{ asset_amount, integr_addr, asset_id });   // asset
    tr_req.fee = TESTS_DEFAULT_FEE;
    tr_req.mixin = 0;
    tools::wallet_public::COMMAND_RPC_TRANSFER::response tr_resp{};
    r = invoke_text_json_for_rpc(miner_wlt_rpc, "transfer", tr_req, tr_resp);
    CHECK_AND_ASSERT_MES(r, false, "mixed native+asset transfer failed");

    r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
    custody_wlt->refresh();

    // get_payments must report native coins in 'amount' and the asset in payment_subtransfers
    {
      tools::wallet_public::COMMAND_RPC_GET_PAYMENTS::request gp_req{};
      gp_req.payment_id = mixed_payment_id_hex_str;
      tools::wallet_public::COMMAND_RPC_GET_PAYMENTS::response gp_resp{};
      r = invoke_text_json_for_rpc(custody_wlt_rpc, "get_payments", gp_req, gp_resp);
      CHECK_AND_ASSERT_MES(r, false, "get_payments failed");

      CHECK_AND_ASSERT_EQ(gp_resp.payments.size(), 1);
      const auto& p = gp_resp.payments.front();
      CHECK_AND_ASSERT_EQ(p.payment_id, mixed_payment_id_hex_str);
      CHECK_AND_ASSERT_EQ(p.amount, native_amount);
      CHECK_AND_ASSERT_EQ(p.payment_subtransfers.size(), 1);
      CHECK_AND_ASSERT_EQ(p.payment_subtransfers.front().asset_id, asset_id);
      CHECK_AND_ASSERT_EQ(p.payment_subtransfers.front().amount, asset_amount);
    }

    // get_bulk_payments must report the same
    {
      tools::wallet_public::COMMAND_RPC_GET_BULK_PAYMENTS::request gbp_req{};
      gbp_req.payment_ids.push_back(mixed_payment_id_hex_str);
      tools::wallet_public::COMMAND_RPC_GET_BULK_PAYMENTS::response gbp_resp{};
      r = invoke_text_json_for_rpc(custody_wlt_rpc, "get_bulk_payments", gbp_req, gbp_resp);
      CHECK_AND_ASSERT_MES(r, false, "get_bulk_payments failed");

      CHECK_AND_ASSERT_EQ(gbp_resp.payments.size(), 1);
      const auto& p = gbp_resp.payments.front();
      CHECK_AND_ASSERT_EQ(p.payment_id, mixed_payment_id_hex_str);
      CHECK_AND_ASSERT_EQ(p.amount, native_amount);
      CHECK_AND_ASSERT_EQ(p.payment_subtransfers.size(), 1);
      CHECK_AND_ASSERT_EQ(p.payment_subtransfers.front().asset_id, asset_id);
      CHECK_AND_ASSERT_EQ(p.payment_subtransfers.front().amount, asset_amount);
    }
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
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet_with_true_http_rpc(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet_with_true_http_rpc(events, c, ALICE_ACC_IDX);

  /*currency::t_currency_protocol_handler<currency::core> m_cprotocol(c, nullptr);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > p2p(m_cprotocol);
  bc_services::bc_offers_service bos(nullptr);
  bos.set_disabled(true);
  currency::core_rpc_server core_rpc_wrapper(c, p2p, bos);
  core_rpc_wrapper.set_ignore_connectivity_status(true); 
  core_rpc_wrapper.set_enabled_admin_api(true);
  
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
  emm_req.destinations.push_back(currency::transfer_destination{ COINS_TO_TRANSFER, bob_wlt->get_account().get_public_address_str(), emm_req.asset_id });
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

  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

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

  // (optional step) skip this reservation just for fun and then create a transaction once again
  tools::wallet_public::COMMAND_CLEAR_UTXO_COLD_SIG_RESERVATION::request req_clear_csr{};
  tools::wallet_public::COMMAND_CLEAR_UTXO_COLD_SIG_RESERVATION::response res_clear_csr{};
  invoke_text_json_for_rpc(rpc_wo, "clear_utxo_cold_sig_reservation", req_clear_csr, res_clear_csr);
  CHECK_AND_ASSERT_MES(res_clear_csr.affected_outputs.size() > 0, false, "no outputs were affected after clear_utxo_cold_sig_reservation");
  res_transfer = tools::wallet_public::COMMAND_RPC_TRANSFER::response{};
  r = invoke_text_json_for_rpc(rpc_wo, "transfer", req_transfer, res_transfer);
  CHECK_AND_ASSERT_MES(r, false, "RPC 'transfer' failed");
    
  // secure wallet with full keys set signs the transaction
  tools::wallet_public::COMMAND_SIGN_TRANSFER::request req_sign{};
  req_sign.tx_unsigned_hex = res_transfer.tx_unsigned_hex;
  tools::wallet_public::COMMAND_SIGN_TRANSFER::response res_sign{};
  r = invoke_text_json_for_rpc(rpc, "sign_transfer", req_sign, res_sign);
  CHECK_AND_ASSERT_MES(r, false, "RPC 'sign_transfer' failed");

  // (optional step) make sure clear_utxo_cold_sig_reservation cannot be called in a full keys wallet
  r = !invoke_text_json_for_rpc(rpc, "clear_utxo_cold_sig_reservation", req_clear_csr, res_clear_csr);
  CHECK_AND_ASSERT_MES(r, false, "clear_utxo_cold_sig_reservation called in full key wallet");
    
  // watch only wallet submits signed transaction
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
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt, "Bob", 0, 0, 0, 0, 0), false, "");

  // for post HF4 cases transfer some assets along with native coins
  crypto::public_key deployed_asset_id{};
  size_t deployed_asset_decimal_point = 0;
  const bool use_assets = c.get_blockchain_storage().is_hardfork_active(ZANO_HARDFORK_04_ZARCANUM);
  if (use_assets)
  {
    // miner deploys new asset and sends 50 coins of it to Alice
    tools::wallet_rpc_server miner_rpc(miner_wlt);
    tools::wallet_public::COMMAND_ASSETS_DEPLOY::request req_deploy{};
    req_deploy.asset_descriptor.current_supply = 50;
    req_deploy.asset_descriptor.decimal_point = deployed_asset_decimal_point;
    req_deploy.asset_descriptor.full_name = "50 pounds per person";
    req_deploy.asset_descriptor.ticker = "50PPP";
    req_deploy.asset_descriptor.total_max_supply = 200; // for a family of four
    req_deploy.destinations.emplace_back(currency::transfer_destination{50, m_accounts[ALICE_ACC_IDX].get_public_address_str(), null_pkey});
    req_deploy.do_not_split_destinations = true;
    tools::wallet_public::COMMAND_ASSETS_DEPLOY::response res_deploy{};
    r = invoke_text_json_for_rpc(miner_rpc, "deploy_asset", req_deploy, res_deploy);
    CHECK_AND_ASSERT_MES(r, false, "RPC 'deploy_asset' failed");
    deployed_asset_id = res_deploy.new_asset_id;
  }

  // also, send some native coins
  miner_wlt->transfer(MK_TEST_COINS(100), m_accounts[ALICE_ACC_IDX].get_public_address(), native_coin_asset_id);

  // confirm this tx and mine 10 block to unlock the coins
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == (use_assets ? 2 : 1), false, "enexpected pool txs count: " << c.get_pool_transactions_count());
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Tx pool is not empty: " << c.get_pool_transactions_count());

  // Alice: prepare watch-only wallet
  account_base alice_acc_wo = m_accounts[ALICE_ACC_IDX];
  alice_acc_wo.make_account_watch_only();
  std::shared_ptr<tools::wallet2> alice_wlt_wo = init_playtime_test_wallet(events, c, alice_acc_wo);

  alice_wlt_wo->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt_wo, "Alice (WO)", MK_TEST_COINS(100), 0, MK_TEST_COINS(100), 0, 0), false, "");

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

  // send a cold-signed transaction: Alice -> Bob; 50 test coins + 50 of the asset
  tools::wallet_public::COMMAND_RPC_TRANSFER::request req{};
  req.destinations.emplace_back(currency::transfer_destination{MK_TEST_COINS(50), m_accounts[BOB_ACC_IDX].get_public_address_str(), native_coin_asset_id});
  if (use_assets)
    req.destinations.emplace_back(currency::transfer_destination{50, m_accounts[BOB_ACC_IDX].get_public_address_str(), deployed_asset_id});
  req.fee = TESTS_DEFAULT_FEE;
  req.mixin = 10;
  r = make_cold_signing_transaction(*alice_rpc_wo_ptr, alice_rpc, req);
  CHECK_AND_ASSERT_MES(r, false, "make_cold_signing_transaction failed");

  uint64_t bob_expected_balance = req.destinations.front().amount;

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


  // send a cold-signed transaction: Alice -> Bob; all rest test coins (should be 50 - 1 = 49)
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


  // send a cold-signed transaction: Alice -> Bob; all rest test coins (should be 7 - 1 = 6)
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
  if (use_assets)
    CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt_wo.get(), "Alice", 0, 0, 0, 0, 0, deployed_asset_id, deployed_asset_decimal_point), false, "");



  // finally, check Bob's balance
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, bob_expected_balance, true, 4 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW, bob_expected_balance, 0, 0, 0), false, "");
  if (use_assets)
    CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", 50, 0, 50, 0, 0, deployed_asset_id, deployed_asset_decimal_point), false, "");

  
  // Alice watch-only: reload the wallet, make sure the balance is still okay (zero)
  alice_rpc_wo_ptr.reset();
  alice_wlt_wo->reset_password(m_wallet_password);
  alice_wlt_wo->store(m_wallet_filename);
  CHECK_AND_ASSERT_EQ(alice_wlt_wo.unique(), true);
  alice_wlt_wo.reset(new tools::wallet2);
  alice_wlt_wo->load(m_wallet_filename, m_wallet_password);
  alice_wlt_wo->set_core_proxy(m_core_proxy);
  alice_wlt_wo->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());
  set_wallet_options(alice_wlt_wo);

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("after re-loading with zero balance:", "Alice", alice_wlt_wo, 0, true, 0, 0, 0, 0, 0), false, "");
  if (use_assets)
    CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt_wo.get(), "Alice", 0, 0, 0, 0, 0, deployed_asset_id, deployed_asset_decimal_point), false, "");


  // Alice: remove outkey2ki file with key images copies and restore the wallet from watch-only account
  std::string alice_seed_phrase = m_accounts[ALICE_ACC_IDX].get_seed_phrase("");
  CHECK_AND_ASSERT_EQ(alice_wlt_wo.unique(), true);
  alice_wlt_wo.reset();
  CHECK_AND_ASSERT_MES(boost::filesystem::remove(epee::string_tools::cut_off_extension(m_wallet_filename) + L".outkey2ki"), false, "boost::filesystem::remove failed");
  CHECK_AND_ASSERT_MES(boost::filesystem::remove(m_wallet_filename), false, "boost::filesystem::remove failed");

  alice_wlt_wo = init_playtime_test_wallet(events, c, alice_acc_wo);
  alice_wlt_wo->set_core_proxy(m_core_proxy);
  alice_wlt_wo->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());
  set_wallet_options(alice_wlt_wo);

  // without key images Alice watch-only wallet is only able to detect incoming transfers and thus calculate incorrect balance
  alice_expected_balance = MK_TEST_COINS(100 + 49 + 7);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("after sending the second cold-signed tx:", "Alice", alice_wlt_wo, alice_expected_balance, true,
    5 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1, alice_expected_balance, 0, 0, 0), false, "");
  if (use_assets)
    CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt_wo.get(), "Alice", 50, 0, 50, 0, 0, deployed_asset_id, deployed_asset_decimal_point), false, "");

  // store this broken watch-only wallet into a file ...
  alice_wlt_wo->reset_password(m_wallet_password);
  alice_wlt_wo->store(m_wallet_filename);
  CHECK_AND_ASSERT_EQ(alice_wlt_wo.unique(), true);
  alice_wlt_wo.reset();

  // ... and repair it using full key wallet
  alice_wlt->restore_key_images_in_wo_wallet(m_wallet_filename, m_wallet_password);

  alice_wlt_wo.reset(new tools::wallet2);
  alice_wlt_wo->load(m_wallet_filename, m_wallet_password);
  alice_wlt_wo->set_core_proxy(m_core_proxy);
  alice_wlt_wo->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());
  set_wallet_options(alice_wlt_wo);

  // re-check the balance, it should be zero now
  alice_expected_balance = 0;
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("after sending the second cold-signed tx:", "Alice", alice_wlt_wo, alice_expected_balance, true,
    5 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1, alice_expected_balance, 0, 0, 0), false, "");
  if (use_assets)
    CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt_wo.get(), "Alice", 0, 0, 0, 0, 0, deployed_asset_id, deployed_asset_decimal_point), false, "");

  return true;
}

//------------------------------------------------------------------------------

// TODO: work in progress -- sowle
wallet_rpc_multiple_receivers::wallet_rpc_multiple_receivers()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_multiple_receivers, c1);
}

void wallet_rpc_multiple_receivers::set_wallet_options(std::shared_ptr<tools::wallet2> w)
{
  set_playtime_test_wallet_options(w);
  w->set_concise_mode(false);
}

bool wallet_rpc_multiple_receivers::generate(std::vector<test_event_entry>& events) const
{
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");

  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  DO_CALLBACK(events, "c1");

  return true;
}

bool wallet_rpc_multiple_receivers::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);

  tools::wallet_rpc_server miner_rpc(miner_wlt);
  tools::wallet_rpc_server alice_rpc(alice_wlt);

  miner_wlt->refresh();
  //bob_wlt->refresh();
  //check_balance_via_wallet(*bob_wlt, "Bob", 0, 0, 0, 0, 0);

  // for post HF4 cases transfer some assets along with native coins
  crypto::public_key deployed_asset_id{};
  size_t deployed_asset_decimal_point = 0;

  // miner deploys new asset and sends 50 coins of it to Alice
  tools::wallet_public::COMMAND_ASSETS_DEPLOY::request req_deploy{};
  req_deploy.asset_descriptor.current_supply = 50;
  req_deploy.asset_descriptor.decimal_point = deployed_asset_decimal_point;
  req_deploy.asset_descriptor.full_name = "50 pounds per person";
  req_deploy.asset_descriptor.ticker = "50PPP";
  req_deploy.asset_descriptor.total_max_supply = 200; // for a family of four
  req_deploy.destinations.emplace_back(currency::transfer_destination{50, m_accounts[ALICE_ACC_IDX].get_public_address_str(), null_pkey});
  req_deploy.do_not_split_destinations = true;
  tools::wallet_public::COMMAND_ASSETS_DEPLOY::response res_deploy{};
  r = invoke_text_json_for_rpc(miner_rpc, "deploy_asset", req_deploy, res_deploy);
  CHECK_AND_ASSERT_MES(r, false, "RPC 'deploy_asset' failed");
  deployed_asset_id = res_deploy.new_asset_id;


  // 
  tools::wallet_public::COMMAND_RPC_TRANSFER::request tr_req{};
  tools::wallet_public::COMMAND_RPC_TRANSFER::response tr_res{};
  tr_req.destinations.emplace_back(currency::transfer_destination{MK_TEST_COINS(80), m_accounts[ALICE_ACC_IDX].get_public_address_str()});
  tr_req.destinations.emplace_back(currency::transfer_destination{MK_TEST_COINS(90), m_accounts[BOB_ACC_IDX].get_public_address_str()});
  tr_req.fee = TESTS_DEFAULT_FEE;
  tr_req.mixin = 0;
  r = invoke_text_json_for_rpc(miner_rpc, "transfer", tr_req, tr_res);
  CHECK_AND_ASSERT_MES(r, false, "RPC failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 2, false, "enexpected pool txs count: " << c.get_pool_transactions_count());
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Tx pool is not empty: " << c.get_pool_transactions_count());


  // Alice: prepare watch-only wallet
  alice_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", MK_TEST_COINS(80), 0, MK_TEST_COINS(80), 0, 0), false, "");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", req_deploy.asset_descriptor.current_supply, 0, req_deploy.asset_descriptor.current_supply, 0, 0, deployed_asset_id, deployed_asset_decimal_point), false, "");

  bob_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt, "Bob", MK_TEST_COINS(90), 0, MK_TEST_COINS(90), 0, 0), false, "");

  return true;
}

//------------------------------------------------------------------------------

wallet_rpc_hardfork_verification::wallet_rpc_hardfork_verification()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_hardfork_verification, configure_core);
  REGISTER_CALLBACK_METHOD(wallet_rpc_hardfork_verification, c1);
}

bool wallet_rpc_hardfork_verification::generate(std::vector<test_event_entry>& events) const
{
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; 
  miner_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  set_hard_fork_heights_to_generator(generator);
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2);

  DO_CALLBACK(events, "c1");
  return true;
}

bool wallet_rpc_hardfork_verification::c1(currency::core& c, size_t, const std::vector<test_event_entry>& events)
{
  // The test idea: verify that wallet and core hardfork versions must match, 
  // if wallet is on a different hardfork than the daemon, refresh should fail. 
  // when both sides switch to the same hardfork again, refresh must succeed.

  std::shared_ptr<debug_wallet2> miner_wlt = init_playtime_test_wallet_t<debug_wallet2>(events, c, MINER_ACC_IDX);

  size_t blocks_added = 0;
  bool received_money = false;
  std::atomic<bool> stop{false};

  bool hf_mismatch_err = false;

  miner_wlt->get_debug_events_dispatcher()
    .SUBSCIRBE_DEBUG_EVENT<wde_error_type>([&](const wde_error_type& e)
    {
      if (e == wde_pulling_hardforks_missmatch)
        hf_mismatch_err = true;
    });

  miner_wlt->refresh(blocks_added, received_money, stop);

  // 1) wallet->HF6, core->HF5 => error
  {
    auto cfg = c.get_blockchain_storage().get_core_runtime_config();
    cfg.hard_forks.set_hardfork_height(6, 1);
    miner_wlt->set_core_runtime_config(cfg);

    hf_mismatch_err = false;
    blocks_added = 0;
    received_money = false;

    miner_wlt->refresh(blocks_added, received_money, stop);

    CHECK_AND_ASSERT_MES(hf_mismatch_err, false, "wallet->hf6 vs core->hf5 must fail");
  }

  // wallet->HF5 and core->HF6 -> error
  {
    miner_wlt->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());
    auto cfg = c.get_blockchain_storage().get_core_runtime_config();
    cfg.hard_forks.set_hardfork_height(6, 1);
    c.get_blockchain_storage().set_core_runtime_config(cfg);

    hf_mismatch_err = false;
    blocks_added = 0;
    received_money = false;

    miner_wlt->refresh(blocks_added, received_money, stop);

    CHECK_AND_ASSERT_MES(hf_mismatch_err, false, "wallet->hf5 vs core->hf6 must fail");
  }

  // both HF6 -> success
  {
    auto cfg = c.get_blockchain_storage().get_core_runtime_config();
    cfg.hard_forks.set_hardfork_height(6, 1);
    miner_wlt->set_core_runtime_config(cfg);
    c.get_blockchain_storage().set_core_runtime_config(cfg);

    hf_mismatch_err = false;
    blocks_added = 0;
    received_money = false;

    miner_wlt->refresh(blocks_added, received_money, stop);

    CHECK_AND_ASSERT_MES(!hf_mismatch_err, false, "refresh must succeed when both on the same HF");
  }

  miner_wlt->callback(std::make_shared<tools::i_wallet2_callback>());

  return true;
}

//------------------------------------------------------------------------------

wallet_rpc_gateway_address::wallet_rpc_gateway_address()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_gateway_address, c1);
}

bool wallet_rpc_gateway_address::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core"); // default callback will initialize core runtime config with m_hardforks

  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  DO_CALLBACK(events, "c1");

  return true;
}

bool wallet_rpc_gateway_address::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;

  //make core rpc server wrapper
  currency::t_currency_protocol_handler<currency::core> cprotocol(c, NULL);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > dummy_p2p(cprotocol);
  bc_services::bc_offers_service dummy_bc(nullptr);
  currency::core_rpc_server core_rpc_wrapper(c, dummy_p2p, dummy_bc);
  core_rpc_wrapper.set_ignore_connectivity_status(true); 
  core_rpc_wrapper.set_enabled_admin_api(true);

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt   = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  
  tools::wallet_rpc_server miner_wlt_rpc(miner_wlt);
  tools::wallet_rpc_server alice_wlt_rpc(alice_wlt);
  tools::wallet_rpc_server bob_wlt_rpc(bob_wlt);

  miner_wlt->refresh();
  miner_wlt->transfer(CURRENCY_GATEWAY_ADDRESS_REGISTRATION_FEE, alice_wlt->get_account().get_public_address());



  // miner deploys new asset and sends 50 coins of it to Alice
  uint8_t deployed_asset_decimal_point = 0;
  tools::wallet_public::COMMAND_ASSETS_DEPLOY::request req_deploy{};
  req_deploy.asset_descriptor.current_supply = 50;
  req_deploy.asset_descriptor.decimal_point = deployed_asset_decimal_point;
  req_deploy.asset_descriptor.full_name = "To the moon!";
  req_deploy.asset_descriptor.ticker = "ArtemisII";
  req_deploy.asset_descriptor.total_max_supply = 1000;
  req_deploy.destinations.emplace_back(currency::transfer_destination{50, m_accounts[ALICE_ACC_IDX].get_public_address_str(), null_pkey});
  req_deploy.do_not_split_destinations = true;
  tools::wallet_public::COMMAND_ASSETS_DEPLOY::response res_deploy{};
  r = invoke_text_json_for_rpc(miner_wlt_rpc, "deploy_asset", req_deploy, res_deploy);
  CHECK_AND_ASSERT_MES(r, false, "RPC 'deploy_asset' failed");
  crypto::public_key deployed_asset_id = res_deploy.new_asset_id;

  LOG_PRINT_GREEN_L0("Deployed asset: " << deployed_asset_id);

  // Alice and miner mine some blocks, confirming asset registering tx
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 2);
  CHECK_AND_ASSERT_TRUE(mine_next_pow_blocks_in_playtime(alice_wlt->get_account().get_public_address(), c, 3));
  CHECK_AND_ASSERT_TRUE(mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW));
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  uint64_t alice_expected_balance_native = 3 * COIN + CURRENCY_GATEWAY_ADDRESS_REGISTRATION_FEE; // mined
  uint64_t alice_expected_balance_asset = req_deploy.asset_descriptor.current_supply;

  CHECK_AND_ASSERT_TRUE(refresh_wallet_and_check_balance("got asset and mined 3 blocks", "Alice", alice_wlt, alice_expected_balance_native));
  CHECK_AND_ASSERT_TRUE(check_balance_via_wallet(*alice_wlt.get(), "Alice", alice_expected_balance_asset, 0, alice_expected_balance_asset, 0, 0, deployed_asset_id, deployed_asset_decimal_point));

  // Alice -> miner : 10 test coins
  //std::string rs = transfer_(alice_wlt, miner_wlt->get_account().get_public_address_str(), MK_TEST_COINS(10));
  //CHECK_AND_ASSERT_NEQ(rs, "");
  //alice_expected_balance_native -= MK_TEST_COINS(10) + TESTS_DEFAULT_FEE;
  //r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  //CHECK_AND_ASSERT_TRUE(r);


  //std::string alice_payment_id_hex_str  = gen_payment_id_as_hex_str(alice_wlt_rpc);
  //std::string alice_payment_id;
  //CHECK_AND_ASSERT_TRUE(epee::string_tools::parse_hexstr_to_binbuff(alice_payment_id_hex_str, alice_payment_id));

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, alice_expected_balance_native), false, "");

  crypto::public_key gw_addr_public_key{};
  crypto::secret_key gw_addr_secret_key{};
  crypto::generate_keys(gw_addr_public_key, gw_addr_secret_key);

  std::string gw_address = currency::get_account_address_as_str(gw_addr_public_key);
#define TRANSFER_AMOUNT   COIN / 10

  // register gw address
  tools::wallet_public::COMMAND_GATEWAY_REGISTER_ADDRESS::request gw_reg_req = {};
  gw_reg_req.view_pub_key = gw_addr_public_key;
  gw_reg_req.descriptor_info.opt_owner_custom_schnorr_pub_key = gw_addr_public_key;
  gw_reg_req.descriptor_info.meta_info = "sdcscsdc";
  tools::wallet_public::COMMAND_GATEWAY_REGISTER_ADDRESS::response gw_reg_resp = {};

  // register gw address using invalid view key (with a nonzero torsion component)
  gw_reg_req.view_pub_key = (crypto::point_t(gw_addr_public_key) + crypto::point_t(crypto::parse_tpod_from_hex_string<crypto::public_key>("ecffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7f"))).to_public_key();
  r = invoke_text_json_for_rpc_and_check_status(alice_wlt_rpc, "register_gateway_address", gw_reg_req, gw_reg_resp);
  CHECK_AND_ASSERT_MES(r, false, "register_gateway_address failed");
  // tx is created but cannot be confirmed in a block
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);

  bool assert_enabled = epee::debug::get_set_enable_assert(false, false);
  epee::debug::get_set_enable_assert(true, false);
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_FALSE(r); // <- should fail
  epee::debug::get_set_enable_assert(true, assert_enabled);
  c.get_tx_pool().clear();
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  alice_wlt->reset_history();
  alice_wlt->refresh();
  // register gw address using good view key
  gw_reg_resp = {};
  gw_reg_req.view_pub_key = gw_addr_public_key;
  r = invoke_text_json_for_rpc_and_check_status(alice_wlt_rpc, "register_gateway_address", gw_reg_req, gw_reg_resp);
  CHECK_AND_ASSERT_MES(r, false, "register_gateway_address failed");

  LOG_PRINT_GREEN_L0("Registered gw address: " << gw_reg_resp.address << ", view pub key: " << gw_reg_resp.address_id);

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 3);
  CHECK_AND_ASSERT_TRUE(r);

  // request gw address info for that freshly genereated gw address and validate the response
  currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_INFO::request gw_get_info_req = {};
  currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_INFO::response gw_get_info_resp = {};

  gw_get_info_req.gateway_address = gw_reg_resp.address;
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_info", gw_get_info_req, gw_get_info_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_get_address_info failed");

  CHECK_AND_ASSERT_EQ(gw_get_info_resp.balances.size(), 0);
  CHECK_AND_ASSERT_EQ(gw_get_info_resp.gateway_view_pub_key, gw_addr_public_key);
  CHECK_AND_ASSERT_EQ(gw_get_info_resp.descriptor_info.meta_info, gw_reg_req.descriptor_info.meta_info);
  CHECK_AND_ASSERT_EQ(gw_get_info_resp.descriptor_info.opt_owner_custom_schnorr_pub_key.has_value(), true);
  CHECK_AND_ASSERT_EQ(gw_get_info_resp.descriptor_info.opt_owner_custom_schnorr_pub_key.value(), gw_reg_req.descriptor_info.opt_owner_custom_schnorr_pub_key.value());
  CHECK_AND_ASSERT_EQ(gw_get_info_resp.payment_id, "");


  //
  // ZC -> GW
  //
  tools::wallet_public::COMMAND_RPC_TRANSFER::request tr_to_gw_req{};
  tools::wallet_public::COMMAND_RPC_TRANSFER::response tr_to_gw_res{};
  tr_to_gw_req.destinations.emplace_back(currency::transfer_destination{MK_TEST_COINS(9), m_accounts[BOB_ACC_IDX].get_public_address_str()});                    // ZC, native
  tr_to_gw_req.destinations.emplace_back(currency::transfer_destination{MK_TEST_COINS(2), gw_reg_resp.address});                                                 // GW, native
  tr_to_gw_req.destinations.emplace_back(currency::transfer_destination{10,               m_accounts[BOB_ACC_IDX].get_public_address_str(), deployed_asset_id}); // ZC, asset
  tr_to_gw_req.destinations.emplace_back(currency::transfer_destination{10,               gw_reg_resp.address, deployed_asset_id});                              // GW, asset
  tr_to_gw_req.fee = TESTS_DEFAULT_FEE;
  r = invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_to_gw_req, tr_to_gw_res);
  CHECK_AND_ASSERT_MES(r, false, "RPC 'transfer' failed");

  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  std::unordered_map<crypto::public_key, uint64_t> gw_balances;
  auto update_gw_balances = [&](const std::list<gateway_balance_entry>& balances)
  {
    gw_balances.clear();
    for(auto& el : balances)
      gw_balances[el.asset_id] = el.amount;
  };

  // request gw address info to check updated balances
  gw_get_info_resp = {};
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_info", gw_get_info_req, gw_get_info_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_get_address_info failed");
  CHECK_AND_ASSERT_EQ(gw_get_info_resp.balances.size(), 2);
  update_gw_balances(gw_get_info_resp.balances);
  CHECK_AND_ASSERT_EQ(gw_balances[native_coin_asset_id],  MK_TEST_COINS(2));
  CHECK_AND_ASSERT_EQ(gw_balances[deployed_asset_id],     10);
  CHECK_AND_ASSERT_EQ(gw_get_info_resp.gateway_view_pub_key, gw_addr_public_key);
  CHECK_AND_ASSERT_EQ(gw_get_info_resp.descriptor_info.meta_info, gw_reg_req.descriptor_info.meta_info);
  CHECK_AND_ASSERT_EQ(gw_get_info_resp.descriptor_info.opt_owner_custom_schnorr_pub_key.has_value(), true);
  CHECK_AND_ASSERT_EQ(gw_get_info_resp.descriptor_info.opt_owner_custom_schnorr_pub_key.value(), gw_reg_req.descriptor_info.opt_owner_custom_schnorr_pub_key.value());
  CHECK_AND_ASSERT_EQ(gw_get_info_resp.payment_id, "");

  //
  // GW -> ZC
  //
  currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::request gw_create_transfer_req = {};
  currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::response gw_create_transfer_resp = {};
  gw_create_transfer_req.destinations.push_back({ MK_TEST_COINS(1), miner_wlt->get_account().get_public_address_str() });
  gw_create_transfer_req.destinations.push_back({ 8,                miner_wlt->get_account().get_public_address_str(), deployed_asset_id });
  gw_create_transfer_req.fee = TESTS_DEFAULT_FEE;
  gw_create_transfer_req.comment = "this is a transfer from a gw address to a normal address";
  gw_create_transfer_req.origin_gateway_id = gw_addr_public_key;
  gw_create_transfer_req.gateway_view_secret_key = gw_addr_secret_key;
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_create_transfer", gw_create_transfer_req, gw_create_transfer_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_create_transfer failed");

  currency::COMMAND_RPC_GATEWAY_SIGN_TRANSFER::request gw_sign_transfer_req = {};
  currency::COMMAND_RPC_GATEWAY_SIGN_TRANSFER::response gw_sign_transfer_resp = {}; 

  //TODO: gw_sign_transfer_req.opt_custom_schnorr_signature =
  crypto::generic_schnorr_sig_s sig{};
  r = crypto::generate_schnorr_sig(gw_create_transfer_resp.tx_hash_to_sign, gw_addr_secret_key, sig);
  CHECK_AND_ASSERT_MES(r, false, "failed to call generate_schnorr_sig");
  gw_sign_transfer_req.opt_custom_schnorr_signature = sig;
  gw_sign_transfer_req.tx_blob = gw_create_transfer_resp.tx_blob;
  gw_sign_transfer_req.tx_id = gw_create_transfer_resp.tx_id;
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_sign_transfer", gw_sign_transfer_req, gw_sign_transfer_resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");

  currency::COMMAND_RPC_SEND_RAW_TX::request send_raw_tx_req = {};
  currency::COMMAND_RPC_SEND_RAW_TX::response send_raw_tx_resp = {};
  send_raw_tx_req.tx_as_hex = epee::string_tools::buff_to_hex_nodelimer(gw_sign_transfer_resp.signed_tx_blob);
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "sendrawtransaction", send_raw_tx_req, send_raw_tx_resp);
  CHECK_AND_ASSERT_MES(r, false, "failed to call");



  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  r = mine_next_pow_blocks_in_playtime(alice_wlt->get_account().get_public_address(), c, 3);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

   // request gw address info to check updated balances
  gw_get_info_resp = {};
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_info", gw_get_info_req, gw_get_info_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_get_address_info failed");
  CHECK_AND_ASSERT_EQ(gw_get_info_resp.balances.size(), 2);
  update_gw_balances(gw_get_info_resp.balances);
  CHECK_AND_ASSERT_EQ(gw_balances[native_coin_asset_id],  0);
  CHECK_AND_ASSERT_EQ(gw_balances[deployed_asset_id],     2);
  CHECK_AND_ASSERT_EQ(gw_get_info_resp.gateway_view_pub_key, gw_addr_public_key);
  CHECK_AND_ASSERT_EQ(gw_get_info_resp.descriptor_info.meta_info, gw_reg_req.descriptor_info.meta_info);
  CHECK_AND_ASSERT_EQ(gw_get_info_resp.descriptor_info.opt_owner_custom_schnorr_pub_key.has_value(), true);
  CHECK_AND_ASSERT_EQ(gw_get_info_resp.descriptor_info.opt_owner_custom_schnorr_pub_key.value(), gw_reg_req.descriptor_info.opt_owner_custom_schnorr_pub_key.value());
  CHECK_AND_ASSERT_EQ(gw_get_info_resp.payment_id, "");

  //payment_id tests + history 

    //
  // ZC -> GW
  //
  alice_wlt->refresh();
  std::unordered_map<crypto::public_key, tools::wallet_public::asset_balance_entry_base> balances;
  uint64_t mined = 0;
  alice_wlt->balance(balances, mined);

  std::string payment_id_a = "1dfe5a88ff9effb3";
  std::string payment_id_b = "1dfe5a88ff9effb4";
  currency::COMMAND_RPC_GET_INTEGRATED_ADDRESS::request get_integrated_addr_req{};
  currency::COMMAND_RPC_GET_INTEGRATED_ADDRESS::response get_integrated_addr_resp{};
  get_integrated_addr_req.regular_address = gw_reg_resp.address;
  get_integrated_addr_req.payment_id = payment_id_a;
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "get_integrated_address", get_integrated_addr_req, get_integrated_addr_resp);
  CHECK_AND_ASSERT_MES(r, false, "get_integrated_address failed");

  std::string integrated_address_a = get_integrated_addr_resp.integrated_address;

  get_integrated_addr_req.payment_id = payment_id_b;
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "get_integrated_address", get_integrated_addr_req, get_integrated_addr_resp);
  CHECK_AND_ASSERT_MES(r, false, "get_integrated_address failed");
  std::string integrated_address_b = get_integrated_addr_resp.integrated_address;

  tools::wallet_public::COMMAND_RPC_TRANSFER::request tr_to_gw_req2{};
  tools::wallet_public::COMMAND_RPC_TRANSFER::response tr_to_gw_res2{};
  tr_to_gw_req2.destinations.emplace_back(currency::transfer_destination{ MK_TEST_COINS(1), integrated_address_a });                   // GW, native asset, payment_id a
  tr_to_gw_req2.destinations.emplace_back(currency::transfer_destination{ MK_TEST_COINS(1), integrated_address_b });                   // GW, native asset, payment_id b
  tr_to_gw_req2.destinations.emplace_back(currency::transfer_destination{ 1,               integrated_address_a, deployed_asset_id }); // GW, asset, payment_id a
  tr_to_gw_req2.destinations.emplace_back(currency::transfer_destination{ 2,               integrated_address_b, deployed_asset_id }); // GW, asset, payment_id b
  tr_to_gw_req2.comment = "aaaaabbbbb";
  tr_to_gw_req2.fee = TESTS_DEFAULT_FEE;
  r = invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_to_gw_req2, tr_to_gw_res);
  CHECK_AND_ASSERT_MES(r, false, "RPC 'transfer' failed");

  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);


  // request address history and check that payment ids are correct
  currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::request get_history_req = {};
  currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::response get_history_resp = {};
  get_history_req.gateway_address = gw_reg_resp.address;
  get_history_req.gateway_view_secret_key = gw_addr_secret_key;
  get_history_req.count = 10;
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_history", get_history_req, get_history_resp);

  CHECK_AND_ASSERT_EQ(get_history_resp.total_transactions, 3);
  CHECK_AND_ASSERT_EQ(get_history_resp.transactions.size(), 3);


  std::map<std::string, std::unordered_map<crypto::public_key, uint64_t > >  payment_id_to_asset_id_to_amout;
  CHECK_AND_ASSERT_EQ(get_history_resp.transactions.back().subtransfers_by_pid.size(), 2);

  for (const auto& sub_payment_id : get_history_resp.transactions.back().subtransfers_by_pid)
  {
    for (const auto& subtransfer : sub_payment_id.subtransfers)
    {
      payment_id_to_asset_id_to_amout[epee::string_tools::buff_to_hex_nodelimer(sub_payment_id.payment_id)][subtransfer.asset_id] = subtransfer.amount;
    }
  }

  CHECK_AND_ASSERT_EQ(payment_id_to_asset_id_to_amout[payment_id_a][currency::native_coin_asset_id], MK_TEST_COINS(1));
  CHECK_AND_ASSERT_EQ(payment_id_to_asset_id_to_amout[payment_id_b][currency::native_coin_asset_id], MK_TEST_COINS(1));
  
  CHECK_AND_ASSERT_EQ(payment_id_to_asset_id_to_amout[payment_id_a][deployed_asset_id], 1);
  CHECK_AND_ASSERT_EQ(payment_id_to_asset_id_to_amout[payment_id_b][deployed_asset_id], 2);

  CHECK_AND_ASSERT_EQ(get_history_resp.transactions.back().comment, tr_to_gw_req2.comment);

  // keyless + client-side decryption - exactly what the gateway_rpc_proxy does - must yield the SAME decrypted result as the server-side path above
  {
    currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::request hk_req = {};
    currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::response hk_resp = {};
    hk_req.gateway_address = gw_reg_resp.address;   // gateway_view_secret_key left unset -> daemon returns raw_txs
    hk_req.count = 10;
    r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_history", hk_req, hk_resp);
    CHECK_AND_ASSERT_MES(r, false, "keyless gateway_get_address_history failed");
    CHECK_AND_ASSERT_MES(gateway_rpc_proxy_decrypt_history(hk_resp, gw_reg_resp.address, gw_addr_secret_key), false, "client-side decryption failed");
    CHECK_AND_ASSERT_EQ(hk_resp.transactions.size(), get_history_resp.transactions.size());
    CHECK_AND_ASSERT_EQ(hk_resp.transactions.back().subtransfers_by_pid.size(), get_history_resp.transactions.back().subtransfers_by_pid.size());
  }

  return true;
}

wallet_rpc_gateway_signatures::wallet_rpc_gateway_signatures()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_gateway_signatures, c1);
}

bool wallet_rpc_gateway_signatures::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  DO_CALLBACK(events, "c1");
  return true;
}

bool wallet_rpc_gateway_signatures::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // Tests ETH ECDSA gateway owner key: register gateway with an ETH public key, fund it, create a transfer,
  // sign it with the ETH private key, broadcast, verify balance
  bool r = false;
  currency::t_currency_protocol_handler<currency::core> cprotocol(c, NULL);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > dummy_p2p(cprotocol);
  bc_services::bc_offers_service dummy_bc(nullptr);
  currency::core_rpc_server core_rpc_wrapper(c, dummy_p2p, dummy_bc);
  core_rpc_wrapper.set_ignore_connectivity_status(true); 
  core_rpc_wrapper.set_enabled_admin_api(true);

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  tools::wallet_rpc_server miner_wlt_rpc(miner_wlt);

  std::unordered_map<crypto::public_key, uint64_t> gw_balances;
  auto update_gw_balances = [&](const std::list<gateway_balance_entry>& balances)
  {
    gw_balances.clear();
    for (const auto& el : balances)
      gw_balances[el.asset_id] = el.amount;
  };

  LOG_PRINT_GREEN_L0("--- ETH ECDSA gateway");

  crypto::public_key eth_gw_view_pub_key{};
  crypto::secret_key eth_gw_view_sec_key{};
  crypto::generate_keys(eth_gw_view_pub_key, eth_gw_view_sec_key);

  crypto::eth_secret_key eth_owner_sec_key{};
  crypto::eth_public_key eth_owner_pub_key{};
  r = crypto::generate_eth_key_pair(eth_owner_sec_key, eth_owner_pub_key);
  CHECK_AND_ASSERT_MES(r, false, "generate_eth_key_pair failed");

  miner_wlt->refresh();
  tools::wallet_public::COMMAND_GATEWAY_REGISTER_ADDRESS::request eth_gw_reg_req = {};
  tools::wallet_public::COMMAND_GATEWAY_REGISTER_ADDRESS::response eth_gw_reg_resp = {};
  eth_gw_reg_req.view_pub_key= eth_gw_view_pub_key;
  eth_gw_reg_req.descriptor_info.opt_owner_ecdsa_pub_key = eth_owner_pub_key;
  eth_gw_reg_req.descriptor_info.meta_info = "eth-gw-test";
  r = invoke_text_json_for_rpc_and_check_status(miner_wlt_rpc, "register_gateway_address", eth_gw_reg_req, eth_gw_reg_resp);
  CHECK_AND_ASSERT_MES(r, false, "register_gateway_address (ETH) failed");
  LOG_PRINT_GREEN_L0("Registered ETH gw address: " << eth_gw_reg_resp.address);

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 3);
  CHECK_AND_ASSERT_TRUE(r);

  // verify ETH gateway,descriptor must contain only opt_owner_ecdsa_pub_key
  currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_INFO::request eth_gw_info_req = {};
  currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_INFO::response eth_gw_info_resp = {};
  eth_gw_info_req.gateway_address = eth_gw_reg_resp.address;
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_info", eth_gw_info_req, eth_gw_info_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_get_address_info (ETH) failed");
  CHECK_AND_ASSERT_EQ(eth_gw_info_resp.balances.size(), 0);
  CHECK_AND_ASSERT_EQ(eth_gw_info_resp.gateway_view_pub_key, eth_gw_view_pub_key);
  CHECK_AND_ASSERT_EQ(eth_gw_info_resp.descriptor_info.opt_owner_ecdsa_pub_key.has_value(), true);
  CHECK_AND_ASSERT_EQ(eth_gw_info_resp.descriptor_info.opt_owner_ecdsa_pub_key.value(), eth_owner_pub_key);
  CHECK_AND_ASSERT_EQ(eth_gw_info_resp.descriptor_info.opt_owner_custom_schnorr_pub_key.has_value(), false);
  CHECK_AND_ASSERT_EQ(eth_gw_info_resp.descriptor_info.opt_owner_eddsa_pub_key.has_value(), false);

  miner_wlt->refresh();
  tools::wallet_public::COMMAND_RPC_TRANSFER::request  tr_to_eth_gw_req = {};
  tools::wallet_public::COMMAND_RPC_TRANSFER::response tr_to_eth_gw_resp = {};
  tr_to_eth_gw_req.destinations.emplace_back(currency::transfer_destination{MK_TEST_COINS(4), eth_gw_reg_resp.address});
  tr_to_eth_gw_req.fee = TESTS_DEFAULT_FEE;
  r = invoke_text_json_for_rpc(miner_wlt_rpc, "transfer", tr_to_eth_gw_req, tr_to_eth_gw_resp);
  CHECK_AND_ASSERT_MES(r, false, "RPC transfer to ETH gateway failed");

  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine failed after funding ETH gw");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  eth_gw_info_resp = {};
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_info", eth_gw_info_req, eth_gw_info_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_get_address_info (ETH, after funding) failed");
  update_gw_balances(eth_gw_info_resp.balances);
  CHECK_AND_ASSERT_EQ(gw_balances[native_coin_asset_id], MK_TEST_COINS(4));

  // create transfer ETH gateway -> miner + bob (2 outputs required by hf4)
  // sends 1 to miner + 1 to bob + TESTS_DEFAULT_FEE = 1 left
  currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::request eth_gw_create_req = {};
  currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::response eth_gw_create_resp = {};
  eth_gw_create_req.origin_gateway_id = eth_gw_view_pub_key;
  eth_gw_create_req.destinations.push_back({MK_TEST_COINS(1), miner_wlt->get_account().get_public_address_str()});
  eth_gw_create_req.destinations.push_back({MK_TEST_COINS(1), bob_wlt->get_account().get_public_address_str()});
  eth_gw_create_req.fee = TESTS_DEFAULT_FEE;
  eth_gw_create_req.comment = "eth gateway transfer test";
  eth_gw_create_req.gateway_view_secret_key = eth_gw_view_sec_key;
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_create_transfer", eth_gw_create_req, eth_gw_create_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_create_transfer (ETH) failed");

  crypto::eth_signature eth_sig{};
  r = crypto::generate_eth_signature(eth_gw_create_resp.tx_hash_to_sign, eth_owner_sec_key, eth_sig);
  CHECK_AND_ASSERT_MES(r, false, "generate_eth_signature failed");

  currency::COMMAND_RPC_GATEWAY_SIGN_TRANSFER::request eth_sign_req = {};
  currency::COMMAND_RPC_GATEWAY_SIGN_TRANSFER::response eth_sign_resp = {};
  eth_sign_req.opt_ecdsa_signature = eth_sig;
  eth_sign_req.tx_blob = eth_gw_create_resp.tx_blob;
  eth_sign_req.tx_id = eth_gw_create_resp.tx_id;
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_sign_transfer", eth_sign_req, eth_sign_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_sign_transfer (ETH) failed");

  currency::COMMAND_RPC_SEND_RAW_TX::request send_eth_gw_req = {};
  currency::COMMAND_RPC_SEND_RAW_TX::response send_eth_gw_resp = {};
  send_eth_gw_req.tx_as_hex = epee::string_tools::buff_to_hex_nodelimer(eth_sign_resp.signed_tx_blob);
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "sendrawtransaction", send_eth_gw_req, send_eth_gw_resp);
  CHECK_AND_ASSERT_MES(r, false, "sendrawtransaction (ETH) failed");

  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 3);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  // ETH gateway balance must decrease: 4 - 1 (to miner) - 1 (to bob) - 1 (fee) = 1
  eth_gw_info_resp = {};
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_info", eth_gw_info_req, eth_gw_info_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_get_address_info (ETH, after transfer) failed");
  update_gw_balances(eth_gw_info_resp.balances);
  CHECK_AND_ASSERT_EQ(gw_balances[native_coin_asset_id], MK_TEST_COINS(1));

  LOG_PRINT_GREEN_L0("wallet_rpc_gateway_signatures passed");

  //
  // EDDSA
  //

  crypto::public_key eddsa_gw_view_pub_key{};
  crypto::secret_key eddsa_gw_view_sec_key{};
  crypto::generate_keys(eddsa_gw_view_pub_key, eddsa_gw_view_sec_key);

  crypto::eddsa_seed eddsa_seed{};
  CHECK_AND_ASSERT_TRUE(crypto::eddsa_generate_random_seed(eddsa_seed));
  crypto::eddsa_sec_prefix eddsa_prefix{};
  crypto::eddsa_secret_key eddsa_owner_sec_key{};
  crypto::eddsa_public_key eddsa_owner_pub_key{};
  CHECK_AND_ASSERT_TRUE(crypto::eddsa_seed_to_secret_key_public_key_and_prefix(eddsa_seed, eddsa_owner_sec_key, eddsa_owner_pub_key, eddsa_prefix));

  miner_wlt->refresh();
  tools::wallet_public::COMMAND_GATEWAY_REGISTER_ADDRESS::request eddsa_gw_reg_req = {};
  tools::wallet_public::COMMAND_GATEWAY_REGISTER_ADDRESS::response eddsa_gw_reg_resp = {};
  eddsa_gw_reg_req.view_pub_key = eddsa_gw_view_pub_key;
  eddsa_gw_reg_req.descriptor_info.opt_owner_eddsa_pub_key = eddsa_owner_pub_key;
  eddsa_gw_reg_req.descriptor_info.meta_info = "eddsa-gw-test";
  r = invoke_text_json_for_rpc_and_check_status(miner_wlt_rpc, "register_gateway_address", eddsa_gw_reg_req, eddsa_gw_reg_resp);
  CHECK_AND_ASSERT_MES(r, false, "register_gateway_address (EdDSA) failed");
  LOG_PRINT_GREEN_L0("Registered EdDSA gw address: " << eddsa_gw_reg_resp.address);

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 3);
  CHECK_AND_ASSERT_TRUE(r);

  currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_INFO::request eddsa_gw_info_req = {};
  currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_INFO::response eddsa_gw_info_resp = {};
  eddsa_gw_info_req.gateway_address = eddsa_gw_reg_resp.address;
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_info", eddsa_gw_info_req, eddsa_gw_info_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_get_address_info (EdDSA) failed");
  CHECK_AND_ASSERT_EQ(eddsa_gw_info_resp.balances.size(), 0);
  CHECK_AND_ASSERT_EQ(eddsa_gw_info_resp.gateway_view_pub_key, eddsa_gw_view_pub_key);
  CHECK_AND_ASSERT_EQ(eddsa_gw_info_resp.descriptor_info.opt_owner_eddsa_pub_key.has_value(), true);
  CHECK_AND_ASSERT_EQ(eddsa_gw_info_resp.descriptor_info.opt_owner_eddsa_pub_key.value(), eddsa_owner_pub_key);
  CHECK_AND_ASSERT_EQ(eddsa_gw_info_resp.descriptor_info.opt_owner_custom_schnorr_pub_key.has_value(), false);
  CHECK_AND_ASSERT_EQ(eddsa_gw_info_resp.descriptor_info.opt_owner_ecdsa_pub_key.has_value(), false);

  miner_wlt->refresh();
  tools::wallet_public::COMMAND_RPC_TRANSFER::request  tr_to_eddsa_gw_req  = {};
  tools::wallet_public::COMMAND_RPC_TRANSFER::response tr_to_eddsa_gw_resp = {};
  tr_to_eddsa_gw_req.destinations.emplace_back(currency::transfer_destination{MK_TEST_COINS(4), eddsa_gw_reg_resp.address});
  tr_to_eddsa_gw_req.fee = TESTS_DEFAULT_FEE;
  r = invoke_text_json_for_rpc(miner_wlt_rpc, "transfer", tr_to_eddsa_gw_req, tr_to_eddsa_gw_resp);
  CHECK_AND_ASSERT_MES(r, false, "RPC 'transfer' to EdDSA gateway failed");

  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine failed after funding EdDSA gw");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  eddsa_gw_info_resp = {};
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_info", eddsa_gw_info_req, eddsa_gw_info_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_get_address_info (EdDSA, after funding) failed");
  update_gw_balances(eddsa_gw_info_resp.balances);
  CHECK_AND_ASSERT_EQ(gw_balances[native_coin_asset_id], MK_TEST_COINS(4));

  currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::request  eddsa_gw_create_req = {};
  currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::response eddsa_gw_create_resp = {};
  eddsa_gw_create_req.origin_gateway_id = eddsa_gw_view_pub_key;
  eddsa_gw_create_req.destinations.push_back({MK_TEST_COINS(1), miner_wlt->get_account().get_public_address_str()});
  eddsa_gw_create_req.destinations.push_back({MK_TEST_COINS(1), bob_wlt->get_account().get_public_address_str()});
  eddsa_gw_create_req.fee = TESTS_DEFAULT_FEE;
  eddsa_gw_create_req.comment = "eddsa gateway transfer test";
  eddsa_gw_create_req.gateway_view_secret_key = eddsa_gw_view_sec_key;
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_create_transfer", eddsa_gw_create_req, eddsa_gw_create_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_create_transfer (EdDSA) failed");

  crypto::eddsa_signature eddsa_sig{};
  r = crypto::generate_eddsa_signature(eddsa_gw_create_resp.tx_hash_to_sign, eddsa_prefix, eddsa_owner_sec_key, eddsa_owner_pub_key, eddsa_sig);
  CHECK_AND_ASSERT_MES(r, false, "generate_eddsa_signature failed");

  currency::COMMAND_RPC_GATEWAY_SIGN_TRANSFER::request eddsa_sign_req = {};
  currency::COMMAND_RPC_GATEWAY_SIGN_TRANSFER::response eddsa_sign_resp = {};
  eddsa_sign_req.opt_eddsa_signature = eddsa_sig;
  eddsa_sign_req.tx_blob = eddsa_gw_create_resp.tx_blob;
  eddsa_sign_req.tx_id = eddsa_gw_create_resp.tx_id;
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_sign_transfer", eddsa_sign_req, eddsa_sign_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_sign_transfer (EdDSA) failed");

  currency::COMMAND_RPC_SEND_RAW_TX::request send_eddsa_gw_req = {};
  currency::COMMAND_RPC_SEND_RAW_TX::response send_eddsa_gw_resp = {};
  send_eddsa_gw_req.tx_as_hex = epee::string_tools::buff_to_hex_nodelimer(eddsa_sign_resp.signed_tx_blob);
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "sendrawtransaction", send_eddsa_gw_req, send_eddsa_gw_resp);
  CHECK_AND_ASSERT_MES(r, false, "sendrawtransaction (EdDSA) failed");

  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 3);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  eddsa_gw_info_resp = {};
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_info", eddsa_gw_info_req, eddsa_gw_info_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_get_address_info (EdDSA, after transfer) failed");
  update_gw_balances(eddsa_gw_info_resp.balances);
  CHECK_AND_ASSERT_EQ(gw_balances[native_coin_asset_id], MK_TEST_COINS(1));

  return true;
}


wallet_rpc_gateway_illegal_asset_id::wallet_rpc_gateway_illegal_asset_id()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_gateway_illegal_asset_id, c1);
}

bool wallet_rpc_gateway_illegal_asset_id::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  DO_CALLBACK(events, "c1");
  return true;
}

bool wallet_rpc_gateway_illegal_asset_id::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // test idea that ZC->GW transfer specifying syntactically invalid asset_id (bytes that are not valid Ed25519 curve point)
  // is rejected by the wallet, while transfer of legitimately deployed asset is accepted
  bool r = false;

  currency::t_currency_protocol_handler<currency::core> cprotocol(c, NULL);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > dummy_p2p(cprotocol);
  bc_services::bc_offers_service dummy_bc(nullptr);
  currency::core_rpc_server core_rpc_wrapper(c, dummy_p2p, dummy_bc);
  core_rpc_wrapper.set_ignore_connectivity_status(true);
  core_rpc_wrapper.set_enabled_admin_api(true);

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  tools::wallet_rpc_server miner_wlt_rpc(miner_wlt);

  miner_wlt->refresh();
  crypto::public_key eth_gw_view_pub_key{};
  crypto::secret_key eth_gw_view_sec_key{};
  crypto::generate_keys(eth_gw_view_pub_key, eth_gw_view_sec_key);

  crypto::eth_secret_key eth_owner_sec_key{};
  crypto::eth_public_key eth_owner_pub_key{};
  r = crypto::generate_eth_key_pair(eth_owner_sec_key, eth_owner_pub_key);
  CHECK_AND_ASSERT_MES(r, false, "generate_eth_key_pair failed");

  tools::wallet_public::COMMAND_GATEWAY_REGISTER_ADDRESS::request eth_gw_reg_req = {};
  tools::wallet_public::COMMAND_GATEWAY_REGISTER_ADDRESS::response eth_gw_reg_resp = {};
  eth_gw_reg_req.view_pub_key = eth_gw_view_pub_key;
  eth_gw_reg_req.descriptor_info.opt_owner_ecdsa_pub_key = eth_owner_pub_key;
  eth_gw_reg_req.descriptor_info.meta_info = "illegal-asset-id-test";
  r = invoke_text_json_for_rpc_and_check_status(miner_wlt_rpc, "register_gateway_address", eth_gw_reg_req, eth_gw_reg_resp);
  CHECK_AND_ASSERT_MES(r, false, "register_gateway_address ETH failed");

  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 3);
  CHECK_AND_ASSERT_MES(r, false, "mine failed after ETH gw registration");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_INFO::request eth_gw_info_req = {};
  currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_INFO::response eth_gw_info_resp = {};
  eth_gw_info_req.gateway_address = eth_gw_reg_resp.address;

  std::unordered_map<crypto::public_key, uint64_t> gw_balances;
  auto update_gw_balances = [&](const std::list<gateway_balance_entry>& balances)
  {
    gw_balances.clear();
    for (const auto& el : balances)
      gw_balances[el.asset_id] = el.amount;
  };

  // illegal asset_id (bytes that are not a valid Ed25519 curve point)
  // first, deploy fresh asset and mint 20 coins to the miner so that the miner holds real utxos for a valid but diff asset
  //
  //  A: rejection attempt to transfer to the ETH gateway specifying invalid_asset_id (bytes that dont decode to any Ed25519 point)
  //    wallet fails at UTXO selection (error::not_enough_money - no UTXOs exist for that asset_id) and no tx enters the mempool
  //    second line of defence process_gateway_ouput calls pub_key_mul8() / ge_frombytes_vartime() and would reject the bytes at the blockchain level
  //
  //  B: happy path transfer the same newly deployed asset (valid asset_id, real balance) to the ETH gateway - must succeed and update the gateway balance.
  //
  // test vector for the invalid point: y = 2^255 - 1 > p (field prime 2^255 - 19)

  // deploy asset+ mint 20 coins to miner
  miner_wlt->refresh();
  tools::wallet_public::COMMAND_ASSETS_DEPLOY::request st2_deploy_req = {};
  tools::wallet_public::COMMAND_ASSETS_DEPLOY::response st2_deploy_resp = {};
  st2_deploy_req.asset_descriptor.current_supply = 20;
  st2_deploy_req.asset_descriptor.decimal_point = 0;
  st2_deploy_req.asset_descriptor.full_name = "XDDDGatewayTestAssetXDDD";
  st2_deploy_req.asset_descriptor.ticker = "GTA6";
  st2_deploy_req.asset_descriptor.total_max_supply = 100;
  st2_deploy_req.destinations.emplace_back(currency::transfer_destination{20, miner_wlt->get_account().get_public_address_str(), null_pkey});
  st2_deploy_req.do_not_split_destinations = true;
  r = invoke_text_json_for_rpc(miner_wlt_rpc, "deploy_asset", st2_deploy_req, st2_deploy_resp);
  CHECK_AND_ASSERT_MES(r, false, "deploy_asset failed");
  crypto::public_key st2_asset_id = st2_deploy_resp.new_asset_id;
  LOG_PRINT_GREEN_L0("asset deployed: " << st2_asset_id);

  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine failed after deploy");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  // A: rejection - transfer to ETH gateway specifying invalid_asset_id
  // miner has real UTXOs (st2_asset_id) but none for invalid_asset_id, so select_indices_for_transfer throws error::not_enough_money before building the tx
  LOG_PRINT_GREEN_L0("--- A: illegal asset_id is rejected by the wallet");

  crypto::public_key invalid_asset_id = crypto::parse_tpod_from_hex_string<crypto::public_key>("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7f");
  miner_wlt->refresh();
  tools::wallet_public::COMMAND_RPC_TRANSFER::request tr_invalid_asset_req = {};
  tools::wallet_public::COMMAND_RPC_TRANSFER::response tr_invalid_asset_resp = {};
  tr_invalid_asset_req.destinations.emplace_back(currency::transfer_destination{10, eth_gw_reg_resp.address, invalid_asset_id});
  tr_invalid_asset_req.fee = TESTS_DEFAULT_FEE;
  r = invoke_text_json_for_rpc(miner_wlt_rpc, "transfer", tr_invalid_asset_req, tr_invalid_asset_resp);
  CHECK_AND_ASSERT_FALSE(r);  // must fail: wallet has no utxo for invalid_asset_id
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  // B: happy path send the valid new asset st2_asset_id to the ETH gateway
  // miner has 20 coins of st2_asset_id send 10 to the gateway
  LOG_PRINT_GREEN_L0("--- B: valid different asset accepted by ETH gateway");

  tools::wallet_public::COMMAND_RPC_TRANSFER::request tr_st2_asset_req = {};
  tools::wallet_public::COMMAND_RPC_TRANSFER::response tr_st2_asset_resp = {};
  tr_st2_asset_req.destinations.emplace_back(currency::transfer_destination{10, eth_gw_reg_resp.address, st2_asset_id});
  tr_st2_asset_req.fee = TESTS_DEFAULT_FEE;
  r = invoke_text_json_for_rpc(miner_wlt_rpc, "transfer", tr_st2_asset_req, tr_st2_asset_resp);
  CHECK_AND_ASSERT_MES(r, false, "transfer valid new asset to ETH gateway failed");

  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 3);
  CHECK_AND_ASSERT_MES(r, false, "mine failed after B transfer");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  // verify ETH gateway now holds 10 coins of st2_asset_id
  eth_gw_info_resp = {};
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_info", eth_gw_info_req, eth_gw_info_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_get_address_info B failed");
  update_gw_balances(eth_gw_info_resp.balances);
  CHECK_AND_ASSERT_EQ(gw_balances[st2_asset_id], 10);

  LOG_PRINT_GREEN_L0("wallet_rpc_gateway_illegal_asset_id passed");
  return true;
}

wallet_rpc_gateway_overspend::wallet_rpc_gateway_overspend()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_gateway_overspend, c1);
}

bool wallet_rpc_gateway_overspend::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc = m_accounts[BOB_ACC_IDX]; bob_acc.generate(); bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  DO_CALLBACK(events, "c1");
  return true;
}

bool wallet_rpc_gateway_overspend::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // Test idea: that gateway_create_transfer rejects transfers that exceed the gateways on-chain balance (both large overdrafts and off-by-one boundary cases)
  bool r = false;

  currency::t_currency_protocol_handler<currency::core> cprotocol(c, NULL);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core>> dummy_p2p(cprotocol);
  bc_services::bc_offers_service dummy_bc(nullptr);
  currency::core_rpc_server core_rpc_wrapper(c, dummy_p2p, dummy_bc);
  core_rpc_wrapper.set_ignore_connectivity_status(true); 
  core_rpc_wrapper.set_enabled_admin_api(true);

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  tools::wallet_rpc_server miner_wlt_rpc(miner_wlt);

  std::unordered_map<crypto::public_key, uint64_t> gw_balances;
  auto update_gw_balances = [&](const std::list<gateway_balance_entry>& balances)
  {
    gw_balances.clear();
    for (const auto& el : balances)
      gw_balances[el.asset_id] = el.amount;
  };

  miner_wlt->refresh();
  crypto::public_key eth_gw_view_pub_key{};
  crypto::secret_key eth_gw_view_sec_key{};
  crypto::generate_keys(eth_gw_view_pub_key, eth_gw_view_sec_key);

  crypto::eth_secret_key eth_owner_sec_key{};
  crypto::eth_public_key eth_owner_pub_key{};
  r = crypto::generate_eth_key_pair(eth_owner_sec_key, eth_owner_pub_key);
  CHECK_AND_ASSERT_MES(r, false, "generate_eth_key_pair failed");

  tools::wallet_public::COMMAND_GATEWAY_REGISTER_ADDRESS::request eth_gw_reg_req = {};
  tools::wallet_public::COMMAND_GATEWAY_REGISTER_ADDRESS::response eth_gw_reg_resp = {};
  eth_gw_reg_req.view_pub_key = eth_gw_view_pub_key;
  eth_gw_reg_req.descriptor_info.opt_owner_ecdsa_pub_key = eth_owner_pub_key;
  eth_gw_reg_req.descriptor_info.meta_info = "overspend-test";
  r = invoke_text_json_for_rpc_and_check_status(miner_wlt_rpc, "register_gateway_address", eth_gw_reg_req, eth_gw_reg_resp);
  CHECK_AND_ASSERT_MES(r, false, "register_gateway_address ETH failed");

  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 3);
  CHECK_AND_ASSERT_MES(r, false, "mine failed after ETH gw registration");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_INFO::request eth_gw_info_req = {};
  currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_INFO::response eth_gw_info_resp = {};
  eth_gw_info_req.gateway_address = eth_gw_reg_resp.address;

  // fund ETH gateway with exactly 1 = TESTS_DEFAULT_FEE native coins this means the entire 
  // native balance will be consumed by the fee in any outgoing tx so any native destination > 0 is an overdraft
  miner_wlt->refresh();
  tools::wallet_public::COMMAND_RPC_TRANSFER::request fund_req = {};
  tools::wallet_public::COMMAND_RPC_TRANSFER::response fund_resp = {};
  fund_req.destinations.emplace_back(currency::transfer_destination{MK_TEST_COINS(1), eth_gw_reg_resp.address});
  fund_req.fee = TESTS_DEFAULT_FEE;
  r = invoke_text_json_for_rpc(miner_wlt_rpc, "transfer", fund_req, fund_resp);
  CHECK_AND_ASSERT_MES(r, false, "transfer native to ETH gateway failed");

  // deploy asset and mint 20 coins to miner, then send 10 to the ETH gateway keeping 10 as change to the miner
  tools::wallet_public::COMMAND_ASSETS_DEPLOY::request os_deploy_req = {};
  tools::wallet_public::COMMAND_ASSETS_DEPLOY::response os_deploy_resp = {};
  os_deploy_req.asset_descriptor.current_supply = 20;
  os_deploy_req.asset_descriptor.decimal_point = 0;
  os_deploy_req.asset_descriptor.full_name = "LLLOverspendTestAssetLLL";
  os_deploy_req.asset_descriptor.ticker = "OTA";
  os_deploy_req.asset_descriptor.total_max_supply = 100;
  os_deploy_req.destinations.emplace_back(currency::transfer_destination{20, miner_wlt->get_account().get_public_address_str(), null_pkey});
  os_deploy_req.do_not_split_destinations = true;
  r = invoke_text_json_for_rpc(miner_wlt_rpc, "deploy_asset", os_deploy_req, os_deploy_resp);
  CHECK_AND_ASSERT_MES(r, false, "deploy_asset overspend failed");
  crypto::public_key os_asset_id = os_deploy_resp.new_asset_id;

  // mine enough blocks to unlock the newly deployed asset UTXOs
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 2);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine failed after funding + deploy");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  // refresh and transfer all 10 os_asset_id coins to the ETH gateway 10 remain as change
  miner_wlt->refresh();
  tools::wallet_public::COMMAND_RPC_TRANSFER::request asset_fund_req = {};
  tools::wallet_public::COMMAND_RPC_TRANSFER::response asset_fund_resp = {};
  asset_fund_req.destinations.emplace_back(currency::transfer_destination{10, eth_gw_reg_resp.address, os_asset_id});
  asset_fund_req.fee = TESTS_DEFAULT_FEE;
  r = invoke_text_json_for_rpc(miner_wlt_rpc, "transfer", asset_fund_req, asset_fund_resp);
  CHECK_AND_ASSERT_MES(r, false, "transfer asset to ETH gateway failed");

  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 3);
  CHECK_AND_ASSERT_MES(r, false, "mine failed after asset transfer to gw");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  // verify starting balances native = 1, os_asset_id = 10
  eth_gw_info_resp = {};
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_info", eth_gw_info_req, eth_gw_info_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_get_address_info (initial) failed");
  update_gw_balances(eth_gw_info_resp.balances);
  CHECK_AND_ASSERT_EQ(gw_balances[native_coin_asset_id], MK_TEST_COINS(1));
  CHECK_AND_ASSERT_EQ(gw_balances[os_asset_id], 10);

  LOG_PRINT_GREEN_L0("--- overdraft - gateway_create_transfer must reject");

  // A: native-coin overdraft
  // attempt to send 2 + 1 = 3 plus TESTS_DEFAULT_FEE, while the gateway only holds 1
  LOG_PRINT_GREEN_L0("--- A: native coin overdraft");
  {
    currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::request req = {};
    currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::response resp = {};
    req.origin_gateway_id = eth_gw_view_pub_key;
    req.gateway_view_secret_key = eth_gw_view_sec_key;
    req.destinations.push_back({MK_TEST_COINS(2), miner_wlt->get_account().get_public_address_str()});
    req.destinations.push_back({MK_TEST_COINS(1), bob_wlt->get_account().get_public_address_str()});
    req.fee = TESTS_DEFAULT_FEE;
    r = invoke_text_json_for_rpc(core_rpc_wrapper, "gateway_create_transfer", req, resp);
    CHECK_AND_ASSERT_MES(r, false, "gateway_create_transfer (A) RPC call failed");
    CHECK_AND_ASSERT_EQ(resp.status, API_RETURN_CODE_NOT_ENOUGH_MONEY); // must fail: gateway holds only 1
    CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  }

  // B: asset overdraft
  // attempt to send 15 + 10 = 25 units of os_asset_id, while the gateway only holds 10
  LOG_PRINT_GREEN_L0("--- B: asset overdraft");
  {
    currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::request req = {};
    currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::response resp = {};
    req.origin_gateway_id = eth_gw_view_pub_key;
    req.gateway_view_secret_key = eth_gw_view_sec_key;
    req.destinations.push_back({15, miner_wlt->get_account().get_public_address_str(), os_asset_id});
    req.destinations.push_back({10, bob_wlt->get_account().get_public_address_str(),   os_asset_id});
    req.fee = TESTS_DEFAULT_FEE;
    r = invoke_text_json_for_rpc(core_rpc_wrapper, "gateway_create_transfer", req, resp);
    CHECK_AND_ASSERT_MES(r, false, "gateway_create_transfer (B) RPC call failed");
    CHECK_AND_ASSERT_EQ(resp.status, API_RETURN_CODE_NOT_ENOUGH_MONEY); // must fail: gateway holds only 10 of os_asset_id
    CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  }

  // boundary test
  // gateway balances at this point:
  //   native coin = 1 = TESTS_DEFAULT_FEE all consumed by fee in any tx
  //   os_asset_id = 10

  // С: native coin, over by exactly 1 unit, total_native_needed = 1 (dest) + TESTS_DEFAULT_FEE (fee) = TESTS_DEFAULT_FEE + 1
  // balance = TESTS_DEFAULT_FEE -> exceeds by 1 -> must reject
  LOG_PRINT_GREEN_L0("--- C: native coin over by 1 unit");
  {
    currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::request req = {};
    currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::response resp = {};
    req.origin_gateway_id = eth_gw_view_pub_key;
    req.gateway_view_secret_key = eth_gw_view_sec_key;
    req.destinations.push_back({1, miner_wlt->get_account().get_public_address_str()});  // 1 unit native
    req.fee = TESTS_DEFAULT_FEE;
    r = invoke_text_json_for_rpc(core_rpc_wrapper, "gateway_create_transfer", req, resp);
    CHECK_AND_ASSERT_MES(r, false, "gateway_create_transfer (C) RPC call failed");
    CHECK_AND_ASSERT_EQ(resp.status, API_RETURN_CODE_NOT_ENOUGH_MONEY); // total_native = TESTS_DEFAULT_FEE + 1 > balance
    CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  }

  // D: asset, over by exactly 1
  // total_asset_needed = 11 > balance(10) -> must reject
  LOG_PRINT_GREEN_L0("--- D: asset over by 1");
  {
    currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::request req = {};
    currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::response resp = {};
    req.origin_gateway_id = eth_gw_view_pub_key;
    req.gateway_view_secret_key = eth_gw_view_sec_key;
    req.destinations.push_back({11, miner_wlt->get_account().get_public_address_str(), os_asset_id});
    req.fee = TESTS_DEFAULT_FEE;
    r = invoke_text_json_for_rpc(core_rpc_wrapper, "gateway_create_transfer", req, resp);
    CHECK_AND_ASSERT_MES(r, false, "gateway_create_transfer (D) RPC call failed");
    CHECK_AND_ASSERT_EQ(resp.status, API_RETURN_CODE_NOT_ENOUGH_MONEY); // total_asset = 11 > balance(10)
    CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  }

  // E: asset exactly at balance - the call must success
  LOG_PRINT_GREEN_L0("--- E: asset exactly at balance");
  {
    currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::request req = {};
    currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::response resp = {};
    req.origin_gateway_id = eth_gw_view_pub_key;
    req.gateway_view_secret_key = eth_gw_view_sec_key;
    req.destinations.push_back({10, miner_wlt->get_account().get_public_address_str(), os_asset_id});
    req.fee = TESTS_DEFAULT_FEE;
    r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_create_transfer", req, resp);
    CHECK_AND_ASSERT_MES(r, false, "gateway_create_transfer (3e: exactly at balance) must succeed");
    CHECK_AND_ASSERT_FALSE(resp.tx_blob.empty());
    CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  }

  // verify that all failed and unsigned attempts left the gateway balances untouched.
  eth_gw_info_resp = {};
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_info", eth_gw_info_req, eth_gw_info_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_get_address_info E failed");
  update_gw_balances(eth_gw_info_resp.balances);
  CHECK_AND_ASSERT_EQ(gw_balances[native_coin_asset_id], MK_TEST_COINS(1));
  CHECK_AND_ASSERT_EQ(gw_balances[os_asset_id], 10);

  LOG_PRINT_GREEN_L0("wallet_rpc_gateway_overspend passed: overdraft correctly rejected, balances unchanged");

  return true;
}

wallet_rpc_gateway_service_entries::wallet_rpc_gateway_service_entries()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_gateway_service_entries, c1);
}

bool wallet_rpc_gateway_service_entries::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: verifies that service entries in gateway_create_transfer are included in the constructed tx and are accessible via gateway_get_address_history
  // plaintext entries flags=0 appear verbatim; encrypted entries TX_SERVICE_ATTACHMENT_ENCRYPT_BODY
  // have their flags preserved but body garbled, since the gateway view key differs from the recipients key
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc = m_accounts[BOB_ACC_IDX]; bob_acc.generate(); bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  DO_CALLBACK(events, "c1");
  return true;
}

bool wallet_rpc_gateway_service_entries::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  currency::t_currency_protocol_handler<currency::core> cprotocol(c, NULL);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > dummy_p2p(cprotocol);
  bc_services::bc_offers_service dummy_bc(nullptr);
  currency::core_rpc_server core_rpc_wrapper(c, dummy_p2p, dummy_bc);
  core_rpc_wrapper.set_ignore_connectivity_status(true); 
  core_rpc_wrapper.set_enabled_admin_api(true);

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  tools::wallet_rpc_server miner_wlt_rpc(miner_wlt);

  // reg ETH gateway
  crypto::public_key gw_view_pub_key{};
  crypto::secret_key gw_view_sec_key{};
  crypto::generate_keys(gw_view_pub_key, gw_view_sec_key);

  crypto::eth_secret_key eth_owner_sec_key{};
  crypto::eth_public_key eth_owner_pub_key{};
  r = crypto::generate_eth_key_pair(eth_owner_sec_key, eth_owner_pub_key);
  CHECK_AND_ASSERT_MES(r, false, "generate_eth_key_pair failed");

  miner_wlt->refresh();
  tools::wallet_public::COMMAND_GATEWAY_REGISTER_ADDRESS::request gw_reg_req = {};
  tools::wallet_public::COMMAND_GATEWAY_REGISTER_ADDRESS::response gw_reg_resp = {};
  gw_reg_req.view_pub_key= gw_view_pub_key;
  gw_reg_req.descriptor_info.opt_owner_ecdsa_pub_key = eth_owner_pub_key;
  gw_reg_req.descriptor_info.meta_info = "omg im meta info, show me!";
  r = invoke_text_json_for_rpc_and_check_status(miner_wlt_rpc, "register_gateway_address", gw_reg_req, gw_reg_resp);
  CHECK_AND_ASSERT_MES(r, false, "register_gateway_address failed");

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 3);
  CHECK_AND_ASSERT_TRUE(r);

  // fund the gateway with 4 native
  miner_wlt->refresh();
  tools::wallet_public::COMMAND_RPC_TRANSFER::request  fund_req = {};
  tools::wallet_public::COMMAND_RPC_TRANSFER::response fund_resp = {};
  fund_req.destinations.emplace_back(currency::transfer_destination{MK_TEST_COINS(4), gw_reg_resp.address});
  fund_req.fee = TESTS_DEFAULT_FEE;
  r = invoke_text_json_for_rpc(miner_wlt_rpc, "transfer", fund_req, fund_resp);
  CHECK_AND_ASSERT_MES(r, false, "RPC 'transfer' to gateway failed");

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_TRUE(r);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  // prepare two service entries - one plaintext one body encrypted
  const std::string plain_body = "hello plaintext";
  const std::string enc_body = "hello encrypted";

  currency::tx_service_attachment sa_plain = AUTO_VAL_INIT(sa_plain);
  sa_plain.service_id = "TEST";
  sa_plain.body = plain_body;
  sa_plain.flags = 0;

  currency::tx_service_attachment sa_enc = AUTO_VAL_INIT(sa_enc);
  sa_enc.service_id = "TEST";
  sa_enc.body = enc_body;
  sa_enc.flags = TX_SERVICE_ATTACHMENT_ENCRYPT_BODY;

  // create gateway transfer: 2 outputs required by the hardfork minimum (miner + bob)
  currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::request ct_req = {};
  currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::response ct_resp = {};
  ct_req.origin_gateway_id = gw_view_pub_key;
  ct_req.destinations.push_back({MK_TEST_COINS(1), miner_wlt->get_account().get_public_address_str()});
  ct_req.destinations.push_back({MK_TEST_COINS(1), bob_wlt->get_account().get_public_address_str()});
  ct_req.fee = TESTS_DEFAULT_FEE;
  ct_req.service_entries.push_back(sa_plain);
  ct_req.service_entries.push_back(sa_enc);
  ct_req.service_entries_permanent = false;  // attachments, not extra
  ct_req.gateway_view_secret_key = gw_view_sec_key;
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_create_transfer", ct_req, ct_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_create_transfer with service entries failed");
  CHECK_AND_ASSERT_FALSE(ct_resp.tx_blob.empty());

  crypto::eth_signature eth_sig{};
  r = crypto::generate_eth_signature(ct_resp.tx_hash_to_sign, eth_owner_sec_key, eth_sig);
  CHECK_AND_ASSERT_MES(r, false, "generate_eth_signature failed");

  currency::COMMAND_RPC_GATEWAY_SIGN_TRANSFER::request sign_req = {};
  currency::COMMAND_RPC_GATEWAY_SIGN_TRANSFER::response sign_resp = {};
  sign_req.opt_ecdsa_signature = eth_sig;
  sign_req.tx_blob = ct_resp.tx_blob;
  sign_req.tx_id = ct_resp.tx_id;
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_sign_transfer", sign_req, sign_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_sign_transfer failed");

  currency::COMMAND_RPC_SEND_RAW_TX::request  send_req = {};
  currency::COMMAND_RPC_SEND_RAW_TX::response send_resp = {};
  send_req.tx_as_hex = epee::string_tools::buff_to_hex_nodelimer(sign_resp.signed_tx_blob);
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "sendrawtransaction", send_req, send_resp);
  CHECK_AND_ASSERT_MES(r, false, "sendrawtransaction failed");

  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 3);
  CHECK_AND_ASSERT_TRUE(r);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  // get history + find the outgoing tx with entries
  currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::request hist_req = {};
  currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::response hist_resp = {};
  hist_req.gateway_address = gw_reg_resp.address;
  hist_req.gateway_view_secret_key = gw_view_sec_key;
  hist_req.count = 10;
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_history", hist_req, hist_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_get_address_history failed");
  CHECK_AND_ASSERT_MES(hist_resp.transactions.size() >= 1, false, "history must contain at least one entry");

  bool found_se_tx = false;
  for (const auto& wti : hist_resp.transactions)
  {
    if (wti.service_entries.size() < 2)
      continue;

    found_se_tx = true;

    const auto& se0 = wti.service_entries[0];
    CHECK_AND_ASSERT_MES(se0.flags == 0, false, "plaintext service entry must have flags==0 got " << (int)se0.flags);
    CHECK_AND_ASSERT_MES(se0.body == plain_body, false, "plaintext service entry body mismatch: '" << se0.body << "' != '" << plain_body << "'");

    // gateway uses sender-side derivation symmetry to decrypt its own outgoing entry
    const auto& se1 = wti.service_entries[1];
    CHECK_AND_ASSERT_MES((se1.flags & TX_SERVICE_ATTACHMENT_ENCRYPT_BODY) != 0, false, "encrypted service entry must have TX_SERVICE_ATTACHMENT_ENCRYPT_BODY set");
    CHECK_AND_ASSERT_MES(se1.body == enc_body, false, "encrypted service entry body must decrypt to plaintext using gateway_view_secret_key: '" << se1.body << "' != '" << enc_body << "'");

    break;
  }
  CHECK_AND_ASSERT_MES(found_se_tx, false, "no history entry with 2 service entries found");

  return true;
}

wallet_rpc_gateway_history_after_outgoing::wallet_rpc_gateway_history_after_outgoing()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_gateway_history_after_outgoing, c1);
}

bool wallet_rpc_gateway_history_after_outgoing::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  DO_CALLBACK(events, "c1");
  return true;
}

bool wallet_rpc_gateway_history_after_outgoing::c1(currency::core& c, size_t /*ev_index*/, const std::vector<test_event_entry>& events)
{
//   1) register_gateway_address ECDSA owner
//   2) fund the GW
//   3) gateway_create_transfer with non-empty `comment` -> ECDSA sign -> gateway_sign_transfer -> sendrawtransaction -> mine
//   4) gateway_get_address_history is expected to succeed bit "Derivation hash missmatched in tx id"

  bool r = false;
  currency::t_currency_protocol_handler<currency::core> cprotocol(c, NULL);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core>> dummy_p2p(cprotocol);
  bc_services::bc_offers_service dummy_bc(nullptr);
  currency::core_rpc_server core_rpc_wrapper(c, dummy_p2p, dummy_bc);
  core_rpc_wrapper.set_ignore_connectivity_status(true);
  core_rpc_wrapper.set_enabled_admin_api(true);

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  tools::wallet_rpc_server miner_wlt_rpc(miner_wlt);

  // reg a GW ECDSA owner key
  crypto::public_key gw_view_pub_key{};
  crypto::secret_key gw_view_sec_key{};
  crypto::generate_keys(gw_view_pub_key, gw_view_sec_key);

  crypto::eth_secret_key eth_owner_sec_key{};
  crypto::eth_public_key eth_owner_pub_key{};
  r = crypto::generate_eth_key_pair(eth_owner_sec_key, eth_owner_pub_key);
  CHECK_AND_ASSERT_MES(r, false, "generate_eth_key_pair failed");

  miner_wlt->refresh();
  tools::wallet_public::COMMAND_GATEWAY_REGISTER_ADDRESS::request gw_reg_req = {};
  tools::wallet_public::COMMAND_GATEWAY_REGISTER_ADDRESS::response gw_reg_resp = {};
  gw_reg_req.view_pub_key = gw_view_pub_key;
  gw_reg_req.descriptor_info.opt_owner_ecdsa_pub_key = eth_owner_pub_key;
  r = invoke_text_json_for_rpc_and_check_status(miner_wlt_rpc, "register_gateway_address", gw_reg_req, gw_reg_resp);
  CHECK_AND_ASSERT_MES(r, false, "register_gateway_address failed");

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 3);
  CHECK_AND_ASSERT_TRUE(r);

  // fund GW
  miner_wlt->refresh();
  tools::wallet_public::COMMAND_RPC_TRANSFER::request fund_req = {};
  tools::wallet_public::COMMAND_RPC_TRANSFER::response fund_resp = {};
  fund_req.destinations.emplace_back(currency::transfer_destination{MK_TEST_COINS(4), gw_reg_resp.address});
  fund_req.fee = TESTS_DEFAULT_FEE;
  r = invoke_text_json_for_rpc(miner_wlt_rpc, "transfer", fund_req, fund_resp);
  CHECK_AND_ASSERT_MES(r, false, "RPC 'transfer' to gateway failed");

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_TRUE(r);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  // gateway_get_address_history must work BEFORE any out tx
  {
    currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::request hist_req = {};
    currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::response hist_resp = {};
    hist_req.gateway_address = gw_reg_resp.address;
    hist_req.gateway_view_secret_key = gw_view_sec_key;
    hist_req.count = 10;
    r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_history", hist_req, hist_resp);
    CHECK_AND_ASSERT_MES(r, false, "baseline gateway_get_address_history (incoming-only) failed");
    CHECK_AND_ASSERT_MES(hist_resp.transactions.size() >= 1, false, "baseline history must contain the inflow");
  }

  // ыpend from GW: build -> sign (ECDSA) -> broadcast
  currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::request  ct_req = {};
  currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::response ct_resp = {};
  ct_req.origin_gateway_id = gw_view_pub_key;
  ct_req.destinations.push_back({MK_TEST_COINS(1), bob_wlt->get_account().get_public_address_str()});
  ct_req.fee = TESTS_DEFAULT_FEE;
  ct_req.comment = "history-after-outgoing-repro"; // fire null derivation
  ct_req.gateway_view_secret_key = gw_view_sec_key; // for keeping sender-side derivation symmetric
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_create_transfer", ct_req, ct_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_create_transfer failed");
  CHECK_AND_ASSERT_FALSE(ct_resp.tx_blob.empty());

  crypto::eth_signature eth_sig{};
  r = crypto::generate_eth_signature(ct_resp.tx_hash_to_sign, eth_owner_sec_key, eth_sig);
  CHECK_AND_ASSERT_MES(r, false, "generate_eth_signature failed");

  currency::COMMAND_RPC_GATEWAY_SIGN_TRANSFER::request sign_req = {};
  currency::COMMAND_RPC_GATEWAY_SIGN_TRANSFER::response sign_resp = {};
  sign_req.opt_ecdsa_signature = eth_sig;
  sign_req.tx_blob = ct_resp.tx_blob;
  sign_req.tx_id = ct_resp.tx_id;
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_sign_transfer", sign_req, sign_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_sign_transfer failed");

  currency::COMMAND_RPC_SEND_RAW_TX::request  send_req = {};
  currency::COMMAND_RPC_SEND_RAW_TX::response send_resp = {};
  send_req.tx_as_hex = epee::string_tools::buff_to_hex_nodelimer(sign_resp.signed_tx_blob);
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "sendrawtransaction", send_req, send_resp);
  CHECK_AND_ASSERT_MES(r, false, "sendrawtransaction failed");

  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 3);
  CHECK_AND_ASSERT_TRUE(r);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  // gateway_get_address_history must still succeed but FAILS
  //  fails inside get_encryption_key_derivation with "Derivation hash missmatched in tx id"
  {
    currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::request hist_req = {};
    currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::response hist_resp = {};
    hist_req.gateway_address = gw_reg_resp.address;
    hist_req.gateway_view_secret_key = gw_view_sec_key;
    hist_req.count = 10;
    r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_history", hist_req, hist_resp);
    CHECK_AND_ASSERT_MES(r, false, "bad: gateway_get_address_history fails after a GW out tx (derivation hash missmatched in get_encryption_key_derivation)");
    CHECK_AND_ASSERT_MES(hist_resp.transactions.size() >= 2, false, "history must contain both the inflow and the outflow, got " << hist_resp.transactions.size());
  }

  return true;
}

wallet_rpc_gateway_reorg_spend::wallet_rpc_gateway_reorg_spend()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_gateway_reorg_spend, c1);
}

bool wallet_rpc_gateway_reorg_spend::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc = m_accounts[BOB_ACC_IDX]; bob_acc.generate(); bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  DO_CALLBACK(events, "c1");
  return true;
}

bool wallet_rpc_gateway_reorg_spend::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // Test idea: gateway spend tx that gets rolled back by a blockchain reorg must restore the gateway balance
  // fund gateway -> record fork_point -> mine 1 block containing the spend tx -> mine 2 alt blocks from fork_point (triggers reorg) -> verify balance restored
  bool r = false;

  currency::t_currency_protocol_handler<currency::core> cprotocol(c, NULL);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > dummy_p2p(cprotocol);
  bc_services::bc_offers_service dummy_bc(nullptr);
  currency::core_rpc_server core_rpc_wrapper(c, dummy_p2p, dummy_bc);
  core_rpc_wrapper.set_ignore_connectivity_status(true); 
  core_rpc_wrapper.set_enabled_admin_api(true);

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt= init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  tools::wallet_rpc_server miner_wlt_rpc(miner_wlt);

  crypto::public_key gw_view_pub_key{};
  crypto::secret_key gw_view_sec_key{};
  crypto::generate_keys(gw_view_pub_key, gw_view_sec_key);

  crypto::eth_secret_key eth_owner_sec_key{};
  crypto::eth_public_key eth_owner_pub_key{};
  r = crypto::generate_eth_key_pair(eth_owner_sec_key, eth_owner_pub_key);
  CHECK_AND_ASSERT_MES(r, false, "generate_eth_key_pair failed");

  miner_wlt->refresh();
  tools::wallet_public::COMMAND_GATEWAY_REGISTER_ADDRESS::request gw_reg_req = {};
  tools::wallet_public::COMMAND_GATEWAY_REGISTER_ADDRESS::response gw_reg_resp = {};
  gw_reg_req.view_pub_key = gw_view_pub_key;
  gw_reg_req.descriptor_info.opt_owner_ecdsa_pub_key = eth_owner_pub_key;
  r = invoke_text_json_for_rpc_and_check_status(miner_wlt_rpc, "register_gateway_address", gw_reg_req, gw_reg_resp);
  CHECK_AND_ASSERT_MES(r, false, "register_gateway_address failed");

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 3);
  CHECK_AND_ASSERT_TRUE(r);

  // Fund the gateway with 4
  miner_wlt->refresh();
  tools::wallet_public::COMMAND_RPC_TRANSFER::request fund_req = {};
  tools::wallet_public::COMMAND_RPC_TRANSFER::response fund_resp = {};
  fund_req.destinations.emplace_back(currency::transfer_destination{MK_TEST_COINS(4), gw_reg_resp.address});
  fund_req.fee = TESTS_DEFAULT_FEE;
  r = invoke_text_json_for_rpc(miner_wlt_rpc, "transfer", fund_req, fund_resp);
  CHECK_AND_ASSERT_MES(r, false, "RPC 'transfer' to gateway failed");

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_TRUE(r);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  // helper to read the gateways native balance
  auto check_gw_native_balance = [&](uint64_t expected) -> bool
  {
    currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_INFO::request info_req = {};
    currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_INFO::response info_resp = {};
    info_req.gateway_address = gw_reg_resp.address;
    bool ok = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_info", info_req, info_resp);
    CHECK_AND_ASSERT_MES(ok, false, "gateway_get_address_info failed");
    uint64_t actual = 0;
    for (const auto& b : info_resp.balances)
      if (b.asset_id == currency::native_coin_asset_id)
        actual = b.amount;
    CHECK_AND_ASSERT_MES(actual == expected, false, "gateway native balance: expected " << expected << ", got " << actual);
    return true;
  };

  CHECK_AND_ASSERT_MES(check_gw_native_balance(MK_TEST_COINS(4)), false, "pre-fork balance check");

  // record the fork point - all blocks after this will be rolled back by the reorg
  crypto::hash fork_point = c.get_tail_id();

  // create and broadcast the gateway spend tx:
  //  dest1 = 1 to miner, dest2 = 1 to bob, fee = TESTS_DEFAULT_FEE
  //  total debited = 3*FEE -> post spend balance = 4 - 3*FEE = 1
  currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::request ct_req = {};
  currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::response ct_resp = {};
  ct_req.origin_gateway_id = gw_view_pub_key;
  ct_req.destinations.push_back({MK_TEST_COINS(1), miner_wlt->get_account().get_public_address_str()});
  ct_req.destinations.push_back({MK_TEST_COINS(1), bob_wlt->get_account().get_public_address_str()});
  ct_req.fee = TESTS_DEFAULT_FEE;
  ct_req.gateway_view_secret_key = gw_view_sec_key;
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_create_transfer", ct_req, ct_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_create_transfer failed");

  crypto::eth_signature eth_sig{};
  r = crypto::generate_eth_signature(ct_resp.tx_hash_to_sign, eth_owner_sec_key, eth_sig);
  CHECK_AND_ASSERT_MES(r, false, "generate_eth_signature failed");

  currency::COMMAND_RPC_GATEWAY_SIGN_TRANSFER::request sign_req = {};
  currency::COMMAND_RPC_GATEWAY_SIGN_TRANSFER::response sign_resp = {};
  sign_req.opt_ecdsa_signature = eth_sig;
  sign_req.tx_blob = ct_resp.tx_blob;
  sign_req.tx_id = ct_resp.tx_id;
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_sign_transfer", sign_req, sign_resp);
  CHECK_AND_ASSERT_MES(r, false, "gateway_sign_transfer failed");

  currency::COMMAND_RPC_SEND_RAW_TX::request send_req = {};
  currency::COMMAND_RPC_SEND_RAW_TX::response send_resp = {};
  send_req.tx_as_hex = epee::string_tools::buff_to_hex_nodelimer(sign_resp.signed_tx_blob);
  r = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "sendrawtransaction", send_req, send_resp);
  CHECK_AND_ASSERT_MES(r, false, "sendrawtransaction failed");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);

  // mine 1 block - spend tx is now confirmed on the main chain
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 1);
  CHECK_AND_ASSERT_TRUE(r);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  CHECK_AND_ASSERT_MES(check_gw_native_balance(MK_TEST_COINS(1)), false, "post-spend balance check");

  // alt chain (2 blocks) > main chain (1 block) -> reorg triggers
  r = mine_next_pow_blocks_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), {}, c, 2, fork_point);
  CHECK_AND_ASSERT_MES(r, false, "failed to mine alt chain for reorg");

  // after the reorg the spend tx is undone gateway balance must be restored
  CHECK_AND_ASSERT_MES(check_gw_native_balance(MK_TEST_COINS(4)), false, "post-reorg balance check");

  LOG_PRINT_GREEN_L0("wallet_rpc_gateway_reorg_spend passed: spend tx rolled back, balance restored");
  return true;
}

wallet_rpc_gateway_reorg_receive::wallet_rpc_gateway_reorg_receive()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_gateway_reorg_receive, c1);
}

bool wallet_rpc_gateway_reorg_receive::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc  = m_accounts[BOB_ACC_IDX];  bob_acc.generate(); bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  DO_CALLBACK(events, "c1");
  return true;
}

bool wallet_rpc_gateway_reorg_receive::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // Test idea: tx that funds a gateway and gets rolled back by a reorg must return the gateway balance to zero
  // i register gateway -> record fork_point -> mine 1 block containing the funding tx -> mine 2 alt blocks from fork_point (triggers reorg) -> verify balance is 0

  bool r = false;
  currency::t_currency_protocol_handler<currency::core> cprotocol(c, NULL);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > dummy_p2p(cprotocol);
  bc_services::bc_offers_service dummy_bc(nullptr);
  currency::core_rpc_server core_rpc_wrapper(c, dummy_p2p, dummy_bc);
  core_rpc_wrapper.set_ignore_connectivity_status(true); 
  core_rpc_wrapper.set_enabled_admin_api(true);

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  tools::wallet_rpc_server miner_wlt_rpc(miner_wlt);

  crypto::public_key gw_view_pub_key{};
  crypto::secret_key gw_view_sec_key{};
  crypto::generate_keys(gw_view_pub_key, gw_view_sec_key);

  crypto::eth_secret_key eth_owner_sec_key{};
  crypto::eth_public_key eth_owner_pub_key{};
  r = crypto::generate_eth_key_pair(eth_owner_sec_key, eth_owner_pub_key);
  CHECK_AND_ASSERT_MES(r, false, "generate_eth_key_pair failed");

  miner_wlt->refresh();
  tools::wallet_public::COMMAND_GATEWAY_REGISTER_ADDRESS::request gw_reg_req = {};
  tools::wallet_public::COMMAND_GATEWAY_REGISTER_ADDRESS::response gw_reg_resp = {};
  gw_reg_req.view_pub_key = gw_view_pub_key;
  gw_reg_req.descriptor_info.opt_owner_ecdsa_pub_key = eth_owner_pub_key;
  r = invoke_text_json_for_rpc_and_check_status(miner_wlt_rpc, "register_gateway_address", gw_reg_req, gw_reg_resp);
  CHECK_AND_ASSERT_MES(r, false, "register_gateway_address failed");

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 3);
  CHECK_AND_ASSERT_TRUE(r);

  auto check_gw_native_balance = [&](uint64_t expected) -> bool
  {
    currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_INFO::request info_req = {};
    currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_INFO::response info_resp = {};
    info_req.gateway_address = gw_reg_resp.address;
    bool ok = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_info", info_req, info_resp);
    CHECK_AND_ASSERT_MES(ok, false, "gateway_get_address_info failed");
    uint64_t actual = 0;
    for (const auto& b : info_resp.balances)
      if (b.asset_id == currency::native_coin_asset_id)
        actual = b.amount;
    CHECK_AND_ASSERT_MES(actual == expected, false, "gateway native balance: expected " << expected << ", got " << actual);
    return true;
  };

  CHECK_AND_ASSERT_MES(check_gw_native_balance(0), false, "initial balance must be 0");

  crypto::hash fork_point = c.get_tail_id();

  // miner sends 2 to the gateway
  miner_wlt->refresh();
  tools::wallet_public::COMMAND_RPC_TRANSFER::request fund_req = {};
  tools::wallet_public::COMMAND_RPC_TRANSFER::response fund_resp = {};
  fund_req.destinations.emplace_back(currency::transfer_destination{MK_TEST_COINS(2), gw_reg_resp.address});
  fund_req.fee = TESTS_DEFAULT_FEE;
  r = invoke_text_json_for_rpc(miner_wlt_rpc, "transfer", fund_req, fund_resp);
  CHECK_AND_ASSERT_MES(r, false, "RPC 'transfer' to gateway failed");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);

  // mine 1 block - funding tx now confirmed on the main chain
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 1);
  CHECK_AND_ASSERT_TRUE(r);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  CHECK_AND_ASSERT_MES(check_gw_native_balance(MK_TEST_COINS(2)), false, "post-fund balance check");

  // alt chain (2 blocks) > main chain (1 block) -> reorg triggers
  r = mine_next_pow_blocks_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), {}, c, 2, fork_point);
  CHECK_AND_ASSERT_MES(r, false, "failed to mine alt chain for reorg");

  // after reorg funding tx is undone gateway balance must return to zero
  CHECK_AND_ASSERT_MES(check_gw_native_balance(0), false, "post-reorg balance must be 0");
  LOG_PRINT_GREEN_L0("wallet_rpc_gateway_reorg_receive passed: funding tx rolled back, balance restored to 0");
  return true;
}

// wallet_rpc_gateway_owner_change_altchain
//
// GW address created and funded in chain A (common)
// Chain B: spend with original owner
// Chain C: change owner via gateway_create_owner_change/gateway_submit_owner_change API, then spend with new owner
// Multi-switch B<->C checking balances and owner validity
//   C main (owner changed, spend with new owner)
//   -> B main (old owner, spend with old owner)
//   -> C main again (new owner, spend with new owner)
//
// Main and alt chain outline:
//
// A- A- A- B1- B2                    <-- B main: spend 7 by old owner
//       | \
//       |  C1- C2- C3- C4            <-- C main after C3: owner change, spend 5 by new owner
//       |           \
//       |            B3- B4- B5- B6  <-- B main again after B5: spend 3 by old owner
//       |                      \
//       |                       C5- C6- C7- C8  <-- C main again after C7: spend 8 by new owner
//       ^
//       |
//  split height

wallet_rpc_gateway_owner_change_altchain::wallet_rpc_gateway_owner_change_altchain()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_gateway_owner_change_altchain, c1);
}

bool wallet_rpc_gateway_owner_change_altchain::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  DO_CALLBACK(events, "c1");
  return true;
}

bool wallet_rpc_gateway_owner_change_altchain::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;

  currency::t_currency_protocol_handler<currency::core> cprotocol(c, NULL);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > dummy_p2p(cprotocol);
  bc_services::bc_offers_service dummy_bc(nullptr);
  currency::core_rpc_server core_rpc_wrapper(c, dummy_p2p, dummy_bc);
  core_rpc_wrapper.set_ignore_connectivity_status(true); 
  core_rpc_wrapper.set_enabled_admin_api(true);

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  tools::wallet_rpc_server miner_wlt_rpc(miner_wlt);

  const auto& miner_addr = m_accounts[MINER_ACC_IDX].get_public_address();
  const auto& bob_addr_str = m_accounts[BOB_ACC_IDX].get_public_address_str();

  //key pairs: gw view key, old owner Schnorr, new owner Schnorr
  crypto::public_key gw_view_pub{};
  crypto::secret_key gw_view_sec{};
  crypto::generate_keys(gw_view_pub, gw_view_sec);

  crypto::public_key old_owner_pub{};
  crypto::secret_key old_owner_sec{};
  crypto::generate_keys(old_owner_pub, old_owner_sec);

  crypto::public_key new_owner_pub{};
  crypto::secret_key new_owner_sec{};
  crypto::generate_keys(new_owner_pub, new_owner_sec);

  // reg gw address with old owner
  miner_wlt->refresh();
  tools::wallet_public::COMMAND_GATEWAY_REGISTER_ADDRESS::request gw_reg_req = {};
  tools::wallet_public::COMMAND_GATEWAY_REGISTER_ADDRESS::response gw_reg_resp = {};
  gw_reg_req.view_pub_key = gw_view_pub;
  gw_reg_req.descriptor_info.opt_owner_custom_schnorr_pub_key = old_owner_pub;
  gw_reg_req.descriptor_info.meta_info = "owner_change_altchain_test";
  r = invoke_text_json_for_rpc_and_check_status(miner_wlt_rpc, "register_gateway_address", gw_reg_req, gw_reg_resp);
  CHECK_AND_ASSERT_MES(r, false, "register_gateway_address failed");

  r = mine_next_pow_blocks_in_playtime(miner_addr, c, 3);
  CHECK_AND_ASSERT_TRUE(r);

  // fund gw with 30 coins
  miner_wlt->refresh();
  tools::wallet_public::COMMAND_RPC_TRANSFER::request fund_req = {};
  tools::wallet_public::COMMAND_RPC_TRANSFER::response fund_resp = {};
  fund_req.destinations.emplace_back(currency::transfer_destination{ MK_TEST_COINS(30), gw_reg_resp.address });
  fund_req.fee = TESTS_DEFAULT_FEE;
  r = invoke_text_json_for_rpc(miner_wlt_rpc, "transfer", fund_req, fund_resp);
  CHECK_AND_ASSERT_MES(r, false, "RPC 'transfer' to gateway failed");

  r = mine_next_pow_blocks_in_playtime(miner_addr, c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_TRUE(r);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  auto check_gw_native_balance = [&](uint64_t expected) -> bool
  {
    currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_INFO::request req = {};
    currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_INFO::response resp = {};
    req.gateway_address = gw_reg_resp.address;
    bool ok = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_info", req, resp);
    CHECK_AND_ASSERT_MES(ok, false, "gateway_get_address_info failed");
    uint64_t actual = 0;
    for (const auto& b : resp.balances)
      if (b.asset_id == currency::native_coin_asset_id)
        actual = b.amount;
    CHECK_AND_ASSERT_MES(actual == expected, false, "gw balance: expected " << expected << ", got " << actual);
    return true;
  };

  auto check_gw_owner = [&](const crypto::public_key& expected_owner) -> bool
  {
    currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_INFO::request req = {};
    currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_INFO::response resp = {};
    req.gateway_address = gw_reg_resp.address;
    bool ok = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_get_address_info", req, resp);
    CHECK_AND_ASSERT_MES(ok, false, "gateway_get_address_info failed");
    CHECK_AND_ASSERT_MES(resp.descriptor_info.opt_owner_custom_schnorr_pub_key.has_value(), false, "owner key not set");
    CHECK_AND_ASSERT_MES(resp.descriptor_info.opt_owner_custom_schnorr_pub_key.value() == expected_owner, false,
      "gw owner mismatch: expected " << expected_owner << ", got " << resp.descriptor_info.opt_owner_custom_schnorr_pub_key.value());
    return true;
  };

  auto deserialize_tx = [](const std::string& blob, currency::transaction& tx) -> bool
  {
    bool ok = t_unserializable_object_from_blob(tx, blob);
    CHECK_AND_ASSERT_MES(ok, false, "failed to deserialize transaction blob");
    return true;
  };

  // create + sign a gw transfer via rpc, return deserialized tx
  auto create_gw_transfer = [&](uint64_t amount, const crypto::secret_key& owner_sec, currency::transaction& out_tx) -> bool
  {
    currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::request ct_req = {};
    currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::response ct_resp = {};
    ct_req.origin_gateway_id = gw_view_pub;
    ct_req.destinations.push_back({ amount, bob_addr_str });
    ct_req.fee = TESTS_DEFAULT_FEE;
    ct_req.gateway_view_secret_key = gw_view_sec;
    bool ok = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_create_transfer", ct_req, ct_resp);
    CHECK_AND_ASSERT_MES(ok, false, "gateway_create_transfer failed");

    crypto::generic_schnorr_sig_s sig{};
    ok = crypto::generate_schnorr_sig(ct_resp.tx_hash_to_sign, owner_sec, sig);
    CHECK_AND_ASSERT_MES(ok, false, "generate_schnorr_sig failed");

    currency::COMMAND_RPC_GATEWAY_SIGN_TRANSFER::request st_req = {};
    currency::COMMAND_RPC_GATEWAY_SIGN_TRANSFER::response st_resp = {};
    st_req.opt_custom_schnorr_signature = sig;
    st_req.tx_blob = ct_resp.tx_blob;
    st_req.tx_id = ct_resp.tx_id;
    ok = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_sign_transfer", st_req, st_resp);
    CHECK_AND_ASSERT_MES(ok, false, "gateway_sign_transfer failed");

    return deserialize_tx(st_resp.signed_tx_blob, out_tx);
  };

  // create + sign an owner change via our new rpc API, return deserialized tx
  auto create_gw_owner_change = [&](const crypto::public_key& new_pub, const crypto::secret_key& current_owner_sec, currency::transaction& out_tx) -> bool
  {
    currency::COMMAND_RPC_GATEWAY_CREATE_OWNER_CHANGE::request oc_req = {};
    currency::COMMAND_RPC_GATEWAY_CREATE_OWNER_CHANGE::response oc_resp = {};
    oc_req.address_id = gw_view_pub;
    oc_req.new_descriptor_info.opt_owner_custom_schnorr_pub_key = new_pub;
    oc_req.fee = TESTS_DEFAULT_FEE;
    bool ok = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_create_owner_change", oc_req, oc_resp);
    CHECK_AND_ASSERT_MES(ok, false, "gateway_create_owner_change failed");

    crypto::generic_schnorr_sig_s transfer_sig{};
    ok = crypto::generate_schnorr_sig(oc_resp.hash_to_sign_transfer, current_owner_sec, transfer_sig);
    CHECK_AND_ASSERT_MES(ok, false, "generate_schnorr_sig (transfer) for owner change failed");

    crypto::generic_schnorr_sig_s ownership_sig{};
    ok = crypto::generate_schnorr_sig(oc_resp.hash_to_sign_ownership, current_owner_sec, ownership_sig);
    CHECK_AND_ASSERT_MES(ok, false, "generate_schnorr_sig (ownership) for owner change failed");

    currency::COMMAND_RPC_GATEWAY_SUBMIT_OWNER_CHANGE::request so_req = {};
    currency::COMMAND_RPC_GATEWAY_SUBMIT_OWNER_CHANGE::response so_resp = {};
    so_req.opt_transfer_custom_schnorr_signature = transfer_sig;
    so_req.opt_ownership_custom_schnorr_signature = ownership_sig;
    so_req.tx_blob = oc_resp.tx_blob;
    so_req.tx_id = oc_resp.tx_id;
    ok = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_submit_owner_change", so_req, so_resp);
    CHECK_AND_ASSERT_MES(ok, false, "gateway_submit_owner_change failed");

    return deserialize_tx(so_resp.signed_tx_blob, out_tx);
  };

  // mine one block with given txs on current main chain tip (bypasses pool)
  auto mine_block_with_txs_on_tip = [&](const std::vector<currency::transaction>& txs) -> bool
  {
    return mine_next_pow_blocks_in_playtime_with_given_txs(miner_addr, txs, c, 1, c.get_tail_id());
  };

  CHECK_AND_ASSERT_MES(check_gw_native_balance(MK_TEST_COINS(30)), false, "initial gw balance");
  CHECK_AND_ASSERT_MES(check_gw_owner(old_owner_pub), false, "initial gw owner");

  //  fork point - all blocks after this are on chain B or chain C
  crypto::hash fork_point = c.get_tail_id();
  LOG_PRINT_GREEN_L0("Fork point: " << fork_point);

  // at fork point - prepare txs for both chains

  // owner change tx (for chain C): changes owner from old_owner to new_owner
  currency::transaction tx_c_owner_change{};
  r = create_gw_owner_change(new_owner_pub, old_owner_sec, tx_c_owner_change);
  CHECK_AND_ASSERT_MES(r, false, "create_gw_owner_change failed");

  // spend 7 tx (for chain B): signed by old owner
  currency::transaction tx_b_spend1{};
  r = create_gw_transfer(MK_TEST_COINS(7), old_owner_sec, tx_b_spend1);
  CHECK_AND_ASSERT_MES(r, false, "create_gw_transfer for B spend1 failed");

  // B: 2 blocks (spend 7 with old owner + 1 empty)
  r = mine_next_pow_blocks_in_playtime_with_given_txs(miner_addr, { tx_b_spend1 }, c, 2, fork_point);
  CHECK_AND_ASSERT_MES(r, false, "mine chain B (2 blocks) failed");
  // B is main (2 blocks from fork)

  CHECK_AND_ASSERT_MES(check_gw_native_balance(MK_TEST_COINS(30) - MK_TEST_COINS(7) - TESTS_DEFAULT_FEE), false, "B: balance after spend1");
  CHECK_AND_ASSERT_MES(check_gw_owner(old_owner_pub), false, "B: owner must be old");

  crypto::hash b_tip = c.get_tail_id();
  LOG_PRINT_GREEN_L0("Chain B built: 2 blocks, tip=" << b_tip);

  //   C: 3 blocks (owner change + 2 empty) — overtakes B
  r = mine_next_pow_blocks_in_playtime_with_given_txs(miner_addr, { tx_c_owner_change }, c, 3, fork_point);
  CHECK_AND_ASSERT_MES(r, false, "mine chain C (3 blocks) failed");
  // C is main (3 > 2), reorg happened

  CHECK_AND_ASSERT_MES(check_gw_native_balance(MK_TEST_COINS(30) - TESTS_DEFAULT_FEE), false, "C: balance after owner change");
  CHECK_AND_ASSERT_MES(check_gw_owner(new_owner_pub), false, "C: owner must be new");

  LOG_PRINT_GREEN_L0("Chain C is main: owner changed, balance OK");

  // C: spend 5 with NEW owner
  currency::transaction tx_c_spend1{};
  r = create_gw_transfer(MK_TEST_COINS(5), new_owner_sec, tx_c_spend1);
  CHECK_AND_ASSERT_MES(r, false, "create_gw_transfer for C spend1 failed");

  r = mine_block_with_txs_on_tip({ tx_c_spend1 });
  CHECK_AND_ASSERT_MES(r, false, "mine C spend1 block failed");
  // C now has 4 blocks from fork

  CHECK_AND_ASSERT_MES(check_gw_native_balance(MK_TEST_COINS(30) - MK_TEST_COINS(5) - 2 * TESTS_DEFAULT_FEE), false, "C: balance after spend1");
  CHECK_AND_ASSERT_MES(check_gw_owner(new_owner_pub), false, "C: owner still new after spend");

  crypto::hash c_tip = c.get_tail_id();
  LOG_PRINT_GREEN_L0("Chain C: 4 blocks, spent 5 with new owner, tip=" << c_tip);

  //  SWITCH to B: mine 3 empty blocks from b_tip (B gets 5 > C=4)
  r = mine_next_pow_blocks_in_playtime_with_given_txs(miner_addr, {}, c, 3, b_tip);
  CHECK_AND_ASSERT_MES(r, false, "extend chain B (3 empty blocks) failed");
  // B is main (5 > 4)

  CHECK_AND_ASSERT_MES(check_gw_native_balance(MK_TEST_COINS(30) - MK_TEST_COINS(7) - TESTS_DEFAULT_FEE), false, "B: balance restored after switch");
  CHECK_AND_ASSERT_MES(check_gw_owner(old_owner_pub), false, "B: owner must be old again");

  LOG_PRINT_GREEN_L0("Switched to B: old owner, balance restored");

  // B: spend 3 with OLD owner
  currency::transaction tx_b_spend2{};
  r = create_gw_transfer(MK_TEST_COINS(3), old_owner_sec, tx_b_spend2);
  CHECK_AND_ASSERT_MES(r, false, "create_gw_transfer for B spend2 failed");

  r = mine_block_with_txs_on_tip({ tx_b_spend2 });
  CHECK_AND_ASSERT_MES(r, false, "mine B spend2 block failed");
  // B now has 6 blocks from fork

  CHECK_AND_ASSERT_MES(check_gw_native_balance(MK_TEST_COINS(30) - MK_TEST_COINS(7) - MK_TEST_COINS(3) - 2 * TESTS_DEFAULT_FEE), false, "B: balance after spend2");

  b_tip = c.get_tail_id();
  LOG_PRINT_GREEN_L0("Chain B: 6 blocks, spent 3 more with old owner, tip=" << b_tip);

  //  SWITCH to C: mine 4 empty blocks from c_tip (C gets 8 > B=6)
  r = mine_next_pow_blocks_in_playtime_with_given_txs(miner_addr, {}, c, 4, c_tip);
  CHECK_AND_ASSERT_MES(r, false, "extend chain C (4 empty blocks) failed");
  // C is main (8 > 6)

  CHECK_AND_ASSERT_MES(check_gw_native_balance(MK_TEST_COINS(30) - MK_TEST_COINS(5) - 2 * TESTS_DEFAULT_FEE), false, "C: balance restored after second switch");
  CHECK_AND_ASSERT_MES(check_gw_owner(new_owner_pub), false, "C: owner must be new again");

  LOG_PRINT_GREEN_L0("Switched to C: new owner, balance restored");

  // C: spend 8 with NEW owner
  currency::transaction tx_c_spend2{};
  r = create_gw_transfer(MK_TEST_COINS(8), new_owner_sec, tx_c_spend2);
  CHECK_AND_ASSERT_MES(r, false, "create_gw_transfer for C spend2 failed");

  r = mine_block_with_txs_on_tip({ tx_c_spend2 });
  CHECK_AND_ASSERT_MES(r, false, "mine C spend2 block failed");
  // C now has 9 blocks from fork

  CHECK_AND_ASSERT_MES(check_gw_native_balance(MK_TEST_COINS(30) - MK_TEST_COINS(5) - MK_TEST_COINS(8) - 3 * TESTS_DEFAULT_FEE), false, "C: balance after spend2");
  CHECK_AND_ASSERT_MES(check_gw_owner(new_owner_pub), false, "C: owner still new");

  LOG_PRINT_GREEN_L0("wallet_rpc_gateway_owner_change_altchain PASSED: B<->C switches with owner change verified");
  return true;
}

//------------------------------------------------------------------------------

wallet_rpc_gateway_limits::wallet_rpc_gateway_limits()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_gateway_limits, c1);
}

bool wallet_rpc_gateway_limits::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  DO_CALLBACK(events, "c1");
  return true;
}

bool wallet_rpc_gateway_limits::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // Test idea: check consensus enforcement of gateway descriptor limits (validate_gateway_descriptor_operation_limits),
  // For both register and update operations:
  //   - meta_info size limit (GATEWAY_ADDRESS_META_INFO_MAX_SIZE = 4000): == limit accepted, > limit rejected
  //   - etc must be empty: any non-empty etc rejected
  // The check lives in validate_tx_for_hardfork_specific_terms (runs at pool add and block add), so we assert at
  // the pool via add_tx. Every descriptor is built with valid signatures, so the only variable is its content.

  bool r = false;
  const size_t GW_META_MAX = 4000; // mirrors GATEWAY_ADDRESS_META_INFO_MAX_SIZE (currency_format_utils.cpp)

  currency::t_currency_protocol_handler<currency::core> cprotocol(c, NULL);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > dummy_p2p(cprotocol);
  bc_services::bc_offers_service dummy_bc(nullptr);
  currency::core_rpc_server core_rpc_wrapper(c, dummy_p2p, dummy_bc);
  core_rpc_wrapper.set_ignore_connectivity_status(true);
  core_rpc_wrapper.set_enabled_admin_api(true);

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  tools::wallet_rpc_server miner_wlt_rpc(miner_wlt);
  const auto& miner_addr = m_accounts[MINER_ACC_IDX].get_public_address();

  // the pool is cleared first to avoid input/key-image cross-talk between cases
  auto submit_to_pool = [&](const currency::transaction& tx) -> bool
  {
    c.get_tx_pool().clear();
    currency::tx_verification_context tvc{};
    bool added = c.get_tx_pool().add_tx(tx, tvc, false);
    return added && !tvc.m_verification_failed;
  };

  // keys for the gw we register (in the register pool checks) and then update
  crypto::public_key gw_view_pub{}; crypto::secret_key gw_view_sec{};
  crypto::generate_keys(gw_view_pub, gw_view_sec);
  crypto::public_key owner_pub{}; crypto::secret_key owner_sec{};
  crypto::generate_keys(owner_pub, owner_sec);
  crypto::public_key new_owner_pub{}; crypto::secret_key new_owner_sec{};
  crypto::generate_keys(new_owner_pub, new_owner_sec);
  const std::string gw_address = currency::get_account_address_as_str(gw_view_pub);

  //
  // register operation
  //
  auto build_register_tx = [&](const std::string& meta_info, bool add_etc) -> currency::transaction
  {
    currency::gateway_address_descriptor_operation_register op_reg{};
    op_reg.view_pub_key = gw_view_pub;
    op_reg.descriptor.owner_key = owner_pub;
    op_reg.descriptor.meta_info = meta_info;
    if (add_etc)
      op_reg.descriptor.etc.push_back(currency::dummy{});
    currency::gateway_address_descriptor_operation gw_op{};
    gw_op.operation = op_reg;

    miner_wlt->reset_history();
    miner_wlt->refresh();
    tools::construct_tx_param ctp = miner_wlt->get_default_construct_tx_param();
    ctp.fee = CURRENCY_GATEWAY_ADDRESS_REGISTRATION_FEE;
    currency::tx_destination_entry td{};
    td.addr.push_back(miner_wlt->get_account().get_public_address());
    td.amount = COIN / 100;
    td.asset_id = currency::native_coin_asset_id;
    ctp.dsts.push_back(td);
    ctp.extra.push_back(gw_op);
    ctp.need_at_least_1_zc = true;
    ctp.tx_meaning_for_logs = "gateway registration (limits test)";

    currency::finalized_tx ft{};
    miner_wlt->transfer(ctp, ft, false);
    return ft.tx;
  };

  // meta_info at the limit, empty etc -> accepted (this one we then confirm and update)
  currency::transaction reg_tx = build_register_tx(std::string(GW_META_MAX, 'x'), false);
  CHECK_AND_ASSERT_MES(submit_to_pool(reg_tx), false, "register with meta_info at the limit should be accepted");
  // meta_info one byte over the limit -> rejected
  CHECK_AND_ASSERT_MES(!submit_to_pool(build_register_tx(std::string(GW_META_MAX + 1, 'x'), false)), false, "register with oversized meta_info should be rejected");
  // non-empty etc -> rejected
  CHECK_AND_ASSERT_MES(!submit_to_pool(build_register_tx("ok", true)), false, "register with non-empty etc should be rejected");


  // confirm the accepted registration so we have a real gw to update
  CHECK_AND_ASSERT_MES(submit_to_pool(reg_tx), false, "accepted registration should re-enter the pool");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  r = mine_next_pow_blocks_in_playtime(miner_addr, c, 3);
  CHECK_AND_ASSERT_TRUE(r);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  // fund the gw with native coins to pay the update fee
  miner_wlt->refresh();
  tools::wallet_public::COMMAND_RPC_TRANSFER::request fund_req = {};
  tools::wallet_public::COMMAND_RPC_TRANSFER::response fund_resp = {};
  fund_req.destinations.emplace_back(currency::transfer_destination{ MK_TEST_COINS(30), gw_address });
  fund_req.fee = TESTS_DEFAULT_FEE;
  r = invoke_text_json_for_rpc(miner_wlt_rpc, "transfer", fund_req, fund_resp);
  CHECK_AND_ASSERT_MES(r, false, "funding transfer to gw failed");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  r = mine_next_pow_blocks_in_playtime(miner_addr, c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_TRUE(r);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  //
  // update operation
  //
  auto build_update_tx = [&](const std::string& meta_info, bool add_etc, currency::transaction& out_tx) -> bool
  {
    currency::COMMAND_RPC_GATEWAY_CREATE_OWNER_CHANGE::request oc_req = {};
    currency::COMMAND_RPC_GATEWAY_CREATE_OWNER_CHANGE::response oc_resp = {};
    oc_req.address_id = gw_view_pub;
    oc_req.new_descriptor_info.opt_owner_custom_schnorr_pub_key = new_owner_pub;
    oc_req.new_descriptor_info.meta_info = meta_info;
    oc_req.fee = TESTS_DEFAULT_FEE;
    bool ok = invoke_text_json_for_rpc_and_check_status(core_rpc_wrapper, "gateway_create_owner_change", oc_req, oc_resp);
    CHECK_AND_ASSERT_MES(ok, false, "gateway_create_owner_change failed");

    ok = t_unserializable_object_from_blob(out_tx, oc_resp.tx_blob);
    CHECK_AND_ASSERT_MES(ok, false, "failed to deserialize update tx blob");

    if (add_etc)
    {
      bool patched = false;
      for (auto& e : out_tx.extra)
      {
        if (e.type() == typeid(currency::gateway_address_descriptor_operation))
        {
          currency::gateway_address_descriptor_operation& gw_op = boost::get<currency::gateway_address_descriptor_operation>(e);
          currency::gateway_address_descriptor_operation_update& op_upd = boost::get<currency::gateway_address_descriptor_operation_update>(gw_op.operation);
          op_upd.descriptor.etc.push_back(currency::dummy{});
          patched = true;
        }
      }
      CHECK_AND_ASSERT_MES(patched, false, "gateway_address_descriptor_operation not found in tx extra");
    }

    // re-derive the signing hashes over the (possibly patched) tx and sign with the current owner key, mirroring
    // on_gateway_create_owner_change / on_gateway_submit_owner_change
    crypto::hash tx_id = currency::get_transaction_hash(out_tx);
    crypto::hash prefix_hash_for_input = currency::prepare_prefix_hash_for_sign(out_tx, 0, tx_id);
    crypto::hash hash_to_sign_transfer  = crypto::hash_helper_t::h(CRYPTO_HDS_GW_INPUT_SIGNATURE, prefix_hash_for_input);
    crypto::hash hash_to_sign_ownership = crypto::hash_helper_t::h(CRYPTO_HDS_GW_CHANGE_OWNER_SIGNATURE, tx_id);

    crypto::generic_schnorr_sig_s transfer_sig{};
    ok = crypto::generate_schnorr_sig(hash_to_sign_transfer, owner_sec, transfer_sig);
    CHECK_AND_ASSERT_MES(ok, false, "generate_schnorr_sig (transfer) failed");
    crypto::generic_schnorr_sig_s ownership_sig{};
    ok = crypto::generate_schnorr_sig(hash_to_sign_ownership, owner_sec, ownership_sig);
    CHECK_AND_ASSERT_MES(ok, false, "generate_schnorr_sig (ownership) failed");

    // attach the gateway input signature (fee) and the ownership proof
    for (auto& sig : out_tx.signatures)
    {
      CHECK_AND_ASSERT_MES(sig.type() == typeid(currency::gateway_sig), false, "unexpected signature type in update tx");
      boost::get<currency::gateway_sig>(sig).s = currency::gateway_signature_v(transfer_sig);
    }
    currency::gateway_address_ownership_proof gaoop{};
    gaoop.sign = ownership_sig;
    out_tx.proofs.push_back(gaoop);
    return true;
  };

  // meta_info at the limit, empty etc -> accepted
  currency::transaction upd_tx_ok{};
  CHECK_AND_ASSERT_MES(build_update_tx(std::string(GW_META_MAX, 'x'), false, upd_tx_ok), false, "build update (limit) failed");
  CHECK_AND_ASSERT_MES(submit_to_pool(upd_tx_ok), false, "update with meta_info at the limit should be accepted");
  // meta_info one byte over the limit -> rejected
  currency::transaction upd_tx_big{};
  CHECK_AND_ASSERT_MES(build_update_tx(std::string(GW_META_MAX + 1, 'x'), false, upd_tx_big), false, "build update (oversized) failed");
  CHECK_AND_ASSERT_MES(!submit_to_pool(upd_tx_big), false, "update with oversized meta_info should be rejected");
  // non-empty etc -> rejected
  currency::transaction upd_tx_etc{};
  CHECK_AND_ASSERT_MES(build_update_tx("ok", true, upd_tx_etc), false, "build update (etc) failed");
  CHECK_AND_ASSERT_MES(!submit_to_pool(upd_tx_etc), false, "update with non-empty etc should be rejected");
  

  // confirm the accepted update in a block
  CHECK_AND_ASSERT_MES(submit_to_pool(upd_tx_ok), false, "accepted update should re-enter the pool");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  r = mine_next_pow_blocks_in_playtime(miner_addr, c, 1);
  CHECK_AND_ASSERT_TRUE(r);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  return true;
}

//------------------------------------------------------------------------------

wallet_rpc_and_tx_unlock_time::wallet_rpc_and_tx_unlock_time()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_and_tx_unlock_time, c1);
}

bool wallet_rpc_and_tx_unlock_time::generate(std::vector<test_event_entry>& events) const
{
  bool r = false;
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");

  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_05));

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), MK_TEST_COINS(1), TESTS_DEFAULT_FEE, /*nmix */ 0, sources, destinations, true, true);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  transaction tx_0{};
  size_t tx_hardfork_id{};
  uint64_t tx_version = get_tx_version_and_hardfork_id(get_block_height(blk_0r), m_hardforks, tx_hardfork_id);

  // unlock_time
  uint64_t unlock_time = 0;

  // unlock_time2 and manually put it into extra
  std::vector<extra_v> extra;
  currency::etc_tx_details_unlock_time2 unlock_time2{};
  CHECK_AND_ASSERT_EQ(destinations.size(), 10);
  unlock_time2.unlock_time_array.push_back(11);
  unlock_time2.unlock_time_array.push_back(2);
  unlock_time2.unlock_time_array.push_back(7);
  unlock_time2.unlock_time_array.push_back(7);
  unlock_time2.unlock_time_array.push_back(7);
  unlock_time2.unlock_time_array.push_back(7);
  unlock_time2.unlock_time_array.push_back(6);
  unlock_time2.unlock_time_array.push_back(7);
  unlock_time2.unlock_time_array.push_back(0);
  unlock_time2.unlock_time_array.push_back(3);
  extra.push_back(unlock_time2);

  crypto::secret_key one_time_secret_key{};
  r = construct_tx(miner_acc.get_keys(), sources, destinations, extra, empty_attachment, tx_0, tx_version, tx_hardfork_id, one_time_secret_key, unlock_time, CURRENCY_TO_KEY_OUT_RELAXED, false);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

  if (m_hardforks.is_hardfork_active_for_height(ZANO_HARDFORK_06, get_block_height(blk_0r) + 1))
  {
    // since HF6 unlock_time2 is deprecated, check it
    DO_CALLBACK(events, "mark_invalid_tx");
    ADD_CUSTOM_EVENT(events, tx_0);
    return true;
  }

  // normal flow
  ADD_CUSTOM_EVENT(events, tx_0);

  etc_tx_details_unlock_time2 ut2{};
  get_type_in_variant_container(tx_0.extra, ut2);
  CHECK_AND_ASSERT_EQ(ut2.unlock_time_array.size(), 10);


  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  LOG_PRINT_GREEN_L0("tx_0: " << get_transaction_hash(tx_0) << ENDL << obj_to_json_str(tx_0));

  DO_CALLBACK(events, "c1");

  return true;
}

bool wallet_rpc_and_tx_unlock_time::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Tx pool is not empty: " << c.get_pool_transactions_count());
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, 0), false, "");

  tools::wallet_rpc_server alice_wlt_rpc(alice_wlt);

  pre_hf4_api::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS::request st_req{};
  st_req.filter_by_height = false;
  st_req.in = true;
  st_req.out = true;
  st_req.pool = true;
  //st_req.tx_id = resp.transfers[1].tx_hash; //bob_tx2
  pre_hf4_api::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS::response st_resp{};
  r = invoke_text_json_for_rpc(alice_wlt_rpc, "search_for_transactions", st_req, st_resp);
  CHECK_AND_ASSERT_MES(r, false, "RPC search_for_transactions failed ");

  CHECK_AND_ASSERT_EQ(st_resp.in.size(), 0);

  return true;
}

//------------------------------------------------------------------------------

wallet_rpc_sweep_below::wallet_rpc_sweep_below()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_sweep_below, c1);
}

bool wallet_rpc_sweep_below::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);
  account_base& carol_acc = m_accounts[CAROL_ACC_IDX]; carol_acc.generate(); carol_acc.set_createtime(ts);
  account_base& dan_acc   = m_accounts[DAN_ACC_IDX];   dan_acc.generate();   dan_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  DO_CALLBACK(events, "c1");
  return true;
}

namespace
{
  // simple rpc wrap for best dev expirience:)
  int call_sweep_below(tools::wallet_rpc_server& rpc, const tools::wallet_public::COMMAND_SWEEP_BELOW::request& req, tools::wallet_public::COMMAND_SWEEP_BELOW::response& res)
  {
    epee::json_rpc::error je{};
    tools::wallet_rpc_server::connection_context ctx{};
    bool ok = rpc.on_sweep_below(req, res, je, ctx);
    return ok ? 0 : je.code;
  }
}

bool wallet_rpc_sweep_below::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt   = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  std::shared_ptr<tools::wallet2> carol_wlt = init_playtime_test_wallet(events, c, CAROL_ACC_IDX);

  miner_wlt->refresh();

  const bool use_assets = c.get_blockchain_storage().is_hardfork_active(ZANO_HARDFORK_04_ZARCANUM);
  CHECK_AND_ASSERT_MES(use_assets, false, "test requires HF4 (Zarcanum) to be active for asset sweep path");

  // deploy a custom asset, give miner some asset UTXOs
  crypto::public_key custom_asset_id{};
  const size_t asset_decimal_point = 6;
  // 1000 SBTA total cap, 100 SBTA per emitted UTXO
  const uint64_t asset_total_max_supply = 1000ull * 1000000ull;
  const uint64_t asset_emit_per_destination = 100ull * 1000000ull;

  {
    currency::asset_descriptor_base adb{};
    adb.total_max_supply = asset_total_max_supply;
    adb.full_name = "Sweep Below Test Asset";
    adb.ticker = "SBTA";
    adb.decimal_point = asset_decimal_point;

    // 6 destinations, all to miner: 6 separate asset UTXOs of 100 SBTA each
    std::vector<currency::tx_destination_entry> dsts;
    for (size_t i = 0; i < 6; ++i)
    {
      currency::tx_destination_entry d{};
      d.addr.push_back(miner_wlt->get_account().get_public_address());
      d.amount = asset_emit_per_destination;
      d.asset_id = currency::null_pkey;
      dsts.push_back(d);
    }

    currency::transaction tx{};
    miner_wlt->deploy_new_asset(adb, dsts, tx, custom_asset_id);
    LOG_PRINT_L0("deployed asset: " << custom_asset_id);
  }

  // confirm asset deploy and unlock its outs
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  miner_wlt->refresh();
  const uint64_t miner_asset_balance_initial = miner_wlt->balance(custom_asset_id);
  CHECK_AND_ASSERT_MES(miner_asset_balance_initial == 6 * asset_emit_per_destination, false,
    "miner asset balance: got " << miner_asset_balance_initial << ", expected " << (6 * asset_emit_per_destination));

  // build Alice for sweep_below tests
  // native: 5 UTXOs of MK_TEST_COINS(2) each = 10 small UTXOs of native (each one is less than fee*100 but more than fee)
  const uint64_t small_native = MK_TEST_COINS(2);
  for (size_t i = 0; i < 5; ++i)
    miner_wlt->transfer(small_native, alice_wlt->get_account().get_public_address(), currency::native_coin_asset_id);

  // native: 1 "fat" UTXO of MK_TEST_COINS(100) to ensure asset sweep has a fee-source available
  const uint64_t fat_native = MK_TEST_COINS(100);
  miner_wlt->transfer(fat_native, alice_wlt->get_account().get_public_address(), currency::native_coin_asset_id);

  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  // asset: 5 separate UTXOs of 20 SBTA each - small ones for the threshold logic
  const uint64_t small_asset = 20ull * 1000000ull;
  for (size_t i = 0; i < 5; ++i)
    miner_wlt->transfer(small_asset, alice_wlt->get_account().get_public_address(), custom_asset_id);

  // asset: 1 "fat" UTXO of 80 SBTA - above-threshold one, must NOT be swept
  const uint64_t fat_asset = 80ull * 1000000ull;
  miner_wlt->transfer(fat_asset, alice_wlt->get_account().get_public_address(), custom_asset_id);

  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  alice_wlt->refresh();
  const uint64_t alice_native_initial = alice_wlt->balance(currency::native_coin_asset_id);
  CHECK_AND_ASSERT_MES(alice_native_initial == 5 * small_native + fat_native, false, "Alice native balance unexpected: " << alice_native_initial);
  const uint64_t alice_asset_initial = alice_wlt->balance(custom_asset_id);
  CHECK_AND_ASSERT_MES(alice_asset_initial == 5 * small_asset + fat_asset, false, "Alice asset balance unexpected: " << alice_asset_initial);

  // case D: invalid destination address -> WRONG_ADDRESS
  {
    tools::wallet_rpc_server alice_rpc(alice_wlt);
    tools::wallet_public::COMMAND_SWEEP_BELOW::request req{};
    req.mixin = 15;
    req.address = "this-is-not-a-valid-zano-address";
    req.amount = small_native + 1;
    req.fee = TESTS_DEFAULT_FEE;
    tools::wallet_public::COMMAND_SWEEP_BELOW::response res{};
    int code = call_sweep_below(alice_rpc, req, res);
    CHECK_AND_ASSERT_MES(code == WALLET_RPC_ERROR_CODE_WRONG_ADDRESS, false, "case D: expected WRONG_ADDRESS (" << WALLET_RPC_ERROR_CODE_WRONG_ADDRESS << "), got " << code);
  }

  // case E: fee below tx_pool_min_fee -> WRONG_ARGUMENT
  {
    tools::wallet_rpc_server alice_rpc(alice_wlt);
    tools::wallet_public::COMMAND_SWEEP_BELOW::request req{};
    req.mixin = 15;
    req.address = m_accounts[BOB_ACC_IDX].get_public_address_str();
    req.amount = small_native + 1;
    req.fee = 1; // way below min
    tools::wallet_public::COMMAND_SWEEP_BELOW::response res{};
    int code = call_sweep_below(alice_rpc, req, res);
    CHECK_AND_ASSERT_MES(code == WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT, false, "case E: expected WRONG_ARGUMENT (" << WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT << "), got " << code);
  }

  // case C: payment_id_hex is now deprecated - any non-empty value must fail with WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID
  {
    tools::wallet_rpc_server alice_rpc(alice_wlt);
    tools::wallet_public::COMMAND_SWEEP_BELOW::request req{};
    req.mixin = 15;
    req.address = m_accounts[BOB_ACC_IDX].get_public_address_str();
    req.amount = small_native + 1;
    req.fee = TESTS_DEFAULT_FEE;
    req.payment_id_hex = "abc123def4567890"; // any non-empty hex must be rejected now
    tools::wallet_public::COMMAND_SWEEP_BELOW::response res{};
    int code = call_sweep_below(alice_rpc, req, res);
    CHECK_AND_ASSERT_MES(code == WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID, false, "case C: expected WRONG_PAYMENT_ID (" << WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID << "), got " << code);

    // garbage that wouldn't have parsed in the old code must also be rejected
    req.payment_id_hex = "not_hex_at_all";
    res = tools::wallet_public::COMMAND_SWEEP_BELOW::response{};
    code = call_sweep_below(alice_rpc, req, res);
    CHECK_AND_ASSERT_MES(code == WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID, false, "case C garbage pid: expected WRONG_PAYMENT_ID, got " << code);
  }

  // case B & A: empty payment_id_hex on a native sweep - must succeed
  // sweep all "small" native UTXOs (amount < small_native + 1 = 5 UTXOs of small_native) to Bob the fat UTXO must NOT be touched
  {
    const uint64_t bob_native_before = 0;
    bob_wlt->refresh();
    CHECK_AND_ASSERT_MES(bob_wlt->balance(currency::native_coin_asset_id) == bob_native_before, false, "bob native balance must start at 0");

    tools::wallet_rpc_server alice_rpc(alice_wlt);
    tools::wallet_public::COMMAND_SWEEP_BELOW::request req{};
    req.mixin =  15;
    req.address = m_accounts[BOB_ACC_IDX].get_public_address_str();
    req.amount = small_native + 1; // threshold: only the 5 small UTXOs qualify
    req.fee = TESTS_DEFAULT_FEE;
    req.payment_id_hex = "";
    // req.asset_id is defaulted to native_coin_asset_id by struct init

    tools::wallet_public::COMMAND_SWEEP_BELOW::response res{};
    int code = call_sweep_below(alice_rpc, req, res);
    CHECK_AND_ASSERT_MES(code == 0, false, "case A/B: native sweep failed, code=" << code);

    // response sanity
    CHECK_AND_ASSERT_MES(res.asset_id == currency::native_coin_asset_id, false, "case A: response asset_id must be native");
    CHECK_AND_ASSERT_MES(res.outs_total == 5, false, "case A: outs_total=" << res.outs_total << ", expected 5");
    CHECK_AND_ASSERT_MES(res.outs_swept == 5, false, "case A: outs_swept=" << res.outs_swept << ", expected 5");
    CHECK_AND_ASSERT_MES(res.amount_total == 5 * small_native, false, "case A: amount_total=" << res.amount_total << ", expected " << (5 * small_native));
    CHECK_AND_ASSERT_MES(res.amount_swept == res.amount_total, false, "case A: amount_swept != amount_total");
    CHECK_AND_ASSERT_MES(!res.tx_hash.empty(), false, "case A: tx_hash is empty");

    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "case A: tx must be in the pool, got " << c.get_pool_transactions_count());

    r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "tx pool not empty after mining");

    // bob received (5 * small_native - fee)
    bob_wlt->refresh();
    const uint64_t bob_native_after = bob_wlt->balance(currency::native_coin_asset_id);
    CHECK_AND_ASSERT_MES(bob_native_after == 5 * small_native - TESTS_DEFAULT_FEE, false, "case A: Bob got " << bob_native_after << ", expected " << (5 * small_native - TESTS_DEFAULT_FEE));

    // Alice's fat native UTXO must remain - i.e. her native balance is exactly the fat one
    alice_wlt->refresh();
    const uint64_t alice_native_after = alice_wlt->balance(currency::native_coin_asset_id);
    CHECK_AND_ASSERT_MES(alice_native_after == fat_native, false, "case A: Alice native left=" << alice_native_after << ", expected " << fat_native);
    // asset balance must be unchanged
    CHECK_AND_ASSERT_MES(alice_wlt->balance(custom_asset_id) == alice_asset_initial, false, "case A: Alice asset balance changed unexpectedly");
  }

  // cases for assets sweep_below
  // sweep all asset UTXOs with amount < (small_asset + 1) = 5 asset UTXOs, send them to Carol
  // Verify:
  //  - response.asset_id == custom_asset_id
  //  - outs_swept == 5 (asset UTXOs only, fee UTXO is not counted)
  //  - amount_swept == 5 * small_asset
  //  - Carol receives exactly 5 * small_asset of the asset
  //  - Alice asset balance is reduced by exactly 5 * small_asset (fat asset remains)
  //  - Alice native balance decreased by exactly TESTS_DEFAULT_FEE (i.e. native change of the consumed fee UTXO returned to Alice)
  uint64_t alice_native_before_asset_sweep = 0;
  uint64_t alice_asset_before_asset_sweep = 0;
  {
    alice_wlt->refresh();
    alice_native_before_asset_sweep = alice_wlt->balance(currency::native_coin_asset_id);
    alice_asset_before_asset_sweep  = alice_wlt->balance(custom_asset_id);

    carol_wlt->refresh();
    const uint64_t carol_asset_before = carol_wlt->balance(custom_asset_id);
    const uint64_t carol_native_before = carol_wlt->balance(currency::native_coin_asset_id);
    CHECK_AND_ASSERT_MES(carol_asset_before == 0, false, "case F: Carol asset balance must start at 0");
    CHECK_AND_ASSERT_MES(carol_native_before == 0, false, "case F: Carol native balance must start at 0");

    tools::wallet_rpc_server alice_rpc(alice_wlt);
    tools::wallet_public::COMMAND_SWEEP_BELOW::request req{};
    req.mixin = 15;
    req.address = m_accounts[CAROL_ACC_IDX].get_public_address_str();
    req.amount = small_asset + 1; // threshold: small_asset UTXOs qualify, fat does not
    req.fee = TESTS_DEFAULT_FEE;
    req.asset_id = custom_asset_id;
    tools::wallet_public::COMMAND_SWEEP_BELOW::response res{};
    int code = call_sweep_below(alice_rpc, req, res);
    CHECK_AND_ASSERT_MES(code == 0, false, "case F: asset sweep failed, code=" << code);

    // response asset_id mirrors request asset_id
    CHECK_AND_ASSERT_MES(res.asset_id == custom_asset_id, false, "case H: response asset_id mismatch: " << res.asset_id);

    // outs_swept counts asset UTXOs only (fee UTXO not included)
    CHECK_AND_ASSERT_MES(res.outs_total == 5, false, "case G: outs_total=" << res.outs_total);
    CHECK_AND_ASSERT_MES(res.outs_swept == 5, false, "case G: outs_swept=" << res.outs_swept << ", expected 5");

    // total/swept amounts are in asset units, fat asset NOT included
    CHECK_AND_ASSERT_MES(res.amount_total == 5 * small_asset, false, "case F: amount_total=" << res.amount_total);
    CHECK_AND_ASSERT_MES(res.amount_swept == 5 * small_asset, false, "case F: amount_swept=" << res.amount_swept);

    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "case F: tx must be in the pool");

    r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

    // Carol receives the asset, but no native
    carol_wlt->refresh();
    const uint64_t carol_asset_after = carol_wlt->balance(custom_asset_id);
    const uint64_t carol_native_after = carol_wlt->balance(currency::native_coin_asset_id);
    CHECK_AND_ASSERT_MES(carol_asset_after == 5 * small_asset, false, "case N: Carol asset balance=" << carol_asset_after << ", expected " << (5 * small_asset));
    CHECK_AND_ASSERT_MES(carol_native_after == 0, false, "case N (privacy/funds): Carol must NOT receive any native coin, got " << carol_native_after);

    //Alice's asset balance is reduced by exactly the swept amount, native balance is reduced by exactly the fee
    alice_wlt->refresh();
    const uint64_t alice_asset_after_sweep = alice_wlt->balance(custom_asset_id);
    const uint64_t alice_native_after_sweep = alice_wlt->balance(currency::native_coin_asset_id);
    CHECK_AND_ASSERT_MES(alice_asset_after_sweep == fat_asset, false, "case M: Alice asset after sweep=" << alice_asset_after_sweep << ", expected " << fat_asset);
    CHECK_AND_ASSERT_MES(alice_native_after_sweep == alice_native_before_asset_sweep - TESTS_DEFAULT_FEE, false, "case M: Alice native after asset sweep=" << alice_native_after_sweep
      << ", expected " << (alice_native_before_asset_sweep - TESTS_DEFAULT_FEE));
  }

  // Case: threshold semantics - strict less-than
  // at this point Alice still has the fat asset UTXO (80 SBTA) sweep_below with threshold == fat_asset must NOT include it (strict <)
  // sweep_below with threshold == fat_asset + 1 must include it
  {
    tools::wallet_rpc_server alice_rpc(alice_wlt);

    tools::wallet_public::COMMAND_SWEEP_BELOW::request req{};
    req.mixin = 15;
    req.address = m_accounts[CAROL_ACC_IDX].get_public_address_str();
    req.amount = fat_asset; // strict-less-than: should match nothing
    req.fee = TESTS_DEFAULT_FEE;
    req.asset_id = custom_asset_id;
    tools::wallet_public::COMMAND_SWEEP_BELOW::response res{};
    int code = call_sweep_below(alice_rpc, req, res);
    CHECK_AND_ASSERT_MES(code != 0, false, "case K strict-less: sweep with threshold == fat_asset must fail, code=" << code);
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "case K strict-less: nothing should have been broadcast");
  }

  // case: asset sweep with no native UTXO >= fee available
  // spend all of Alice's native first, leaving only a tiny dust amount < fee then try to asset-sweep - must fail
  // funds must not be lost (transfers remain marked unspent)
  {
    // move all Alice native to Bob, leaving a known small amount via change first
    alice_wlt->refresh();
    const uint64_t alice_native_currently = alice_wlt->balance(currency::native_coin_asset_id);
    CHECK_AND_ASSERT_MES(alice_native_currently > 2 * TESTS_DEFAULT_FEE, false, "case I prep: Alice native should be > 2*fee");

    // send (balance - 2*fee) to Bob after fee
    const uint64_t to_send = alice_native_currently - TESTS_DEFAULT_FEE - (TESTS_DEFAULT_FEE / 2);
    // change after fee = native - to_send - fee = fee/2  (< fee)
    currency::transaction drain_tx{};
    alice_wlt->transfer(to_send, m_accounts[BOB_ACC_IDX].get_public_address(), drain_tx, currency::native_coin_asset_id);

    r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
    CHECK_AND_ASSERT_MES(r, false, "case I prep: mine failed");

    alice_wlt->refresh();
    const uint64_t alice_native_dust = alice_wlt->balance(currency::native_coin_asset_id);
    CHECK_AND_ASSERT_MES(alice_native_dust > 0 && alice_native_dust < TESTS_DEFAULT_FEE, false,
      "case I prep: Alice native dust=" << alice_native_dust << ", expected 0<dust<fee");
    const uint64_t alice_asset_dust_phase = alice_wlt->balance(custom_asset_id);
    CHECK_AND_ASSERT_MES(alice_asset_dust_phase == fat_asset, false,
      "case I prep: Alice asset balance changed unexpectedly");

    // now attempt asset sweep - must fail because no native UTXO >= fee exists
    tools::wallet_rpc_server alice_rpc(alice_wlt);
    tools::wallet_public::COMMAND_SWEEP_BELOW::request req{};
    req.mixin = 15;
    req.address = m_accounts[CAROL_ACC_IDX].get_public_address_str();
    req.amount = fat_asset + 1; // would match the fat UTXO if it had a fee source
    req.fee = TESTS_DEFAULT_FEE;
    req.asset_id = custom_asset_id;
    tools::wallet_public::COMMAND_SWEEP_BELOW::response res{};
    int code = call_sweep_below(alice_rpc, req, res);
    CHECK_AND_ASSERT_MES(code != 0, false, "case I: asset sweep without native fee UTXO must fail, code=" << code);
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "case I: nothing should have been broadcast");

    // funds preserved: balances unchanged
    alice_wlt->refresh();
    CHECK_AND_ASSERT_MES(alice_wlt->balance(custom_asset_id) == fat_asset, false, "case I: Alice asset balance must be preserved after the failed sweep");
    CHECK_AND_ASSERT_MES(alice_wlt->balance(currency::native_coin_asset_id) == alice_native_dust, false, "case I: Alice native balance must be preserved after the failed sweep");

    // now refill Alice with a native UTXO and verify the recovery path works
    miner_wlt->refresh();
    miner_wlt->transfer(MK_TEST_COINS(10), alice_wlt->get_account().get_public_address(), currency::native_coin_asset_id);
    r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
    CHECK_AND_ASSERT_MES(r, false, "case I recovery: mine failed");
    alice_wlt->refresh();
  }

  // case: non-existent asset_id -> "no spendable outputs meet the criterion"
  {
    crypto::public_key bogus_asset_id{};
    // a random non-zero, non-native pubkey; nonexistent in Alice's transfers
    for (size_t i = 0; i < sizeof(bogus_asset_id); ++i)
      reinterpret_cast<uint8_t*>(&bogus_asset_id)[i] = static_cast<uint8_t>(0xA0 + (i & 0x0F));

    tools::wallet_rpc_server alice_rpc(alice_wlt);
    tools::wallet_public::COMMAND_SWEEP_BELOW::request req{};
    req.mixin = 15;
    req.address = m_accounts[CAROL_ACC_IDX].get_public_address_str();
    req.amount = UINT64_MAX;
    req.fee = TESTS_DEFAULT_FEE;
    req.asset_id = bogus_asset_id;
    tools::wallet_public::COMMAND_SWEEP_BELOW::response res{};
    int code = call_sweep_below(alice_rpc, req, res);
    CHECK_AND_ASSERT_MES(code != 0, false,
      "case J: sweep with non-existent asset_id must fail, code=" << code);
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false,
      "case J: nothing should have been broadcast");
  }

  // case: sweep to an integrated address - payment_id is honoured
  //  we use Carol as the receiver via an integrated address generated by her wallet after the sweep, Carol must be able to find the payment by its payment_id
  {
    miner_wlt->refresh();
    miner_wlt->transfer(small_asset, alice_wlt->get_account().get_public_address(), custom_asset_id);
    r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
    CHECK_AND_ASSERT_MES(r, false, "case L prep: mine failed");
    alice_wlt->refresh();
  }

  std::string integrated_payment_id_hex;
  std::string integrated_address;
  {
    tools::wallet_rpc_server carol_rpc(carol_wlt);
    carol_rpc.set_flag_allow_legacy_payment_id_size(true);
    tools::wallet_public::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::request mia_req{};
    // empty payment_id -> server generates one
    tools::wallet_public::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::response mia_res{};
    epee::json_rpc::error je{};
    tools::wallet_rpc_server::connection_context ctx{};
    bool ok = carol_rpc.on_make_integrated_address(mia_req, mia_res, je, ctx);
    CHECK_AND_ASSERT_MES(ok, false, "case L: make_integrated_address failed: " << je.message);
    integrated_payment_id_hex = mia_res.payment_id;
    integrated_address = mia_res.integrated_address;
    CHECK_AND_ASSERT_MES(!integrated_payment_id_hex.empty(), false, "case L: empty payment_id");
    CHECK_AND_ASSERT_MES(!integrated_address.empty(), false, "case L: empty integrated address");
  }

  {
    tools::wallet_rpc_server alice_rpc(alice_wlt);
    tools::wallet_public::COMMAND_SWEEP_BELOW::request req{};
    req.mixin = 15;
    req.address = integrated_address;
    req.amount = small_asset + 1; // matches the freshly added small asset UTXO
    req.fee = TESTS_DEFAULT_FEE;
    req.asset_id = custom_asset_id;
    // payment_id_hex must remain empty here; the payment id is sourced from the integrated address
    tools::wallet_public::COMMAND_SWEEP_BELOW::response res{};
    int code = call_sweep_below(alice_rpc, req, res);
    CHECK_AND_ASSERT_MES(code == 0, false, "case L: sweep into integrated addr failed, code=" << code);
    CHECK_AND_ASSERT_MES(res.outs_swept >= 1, false, "case L: outs_swept=" << res.outs_swept);
    CHECK_AND_ASSERT_MES(res.amount_swept == small_asset, false, "case L: amount_swept=" << res.amount_swept << ", expected " << small_asset);

    r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
    CHECK_AND_ASSERT_MES(r, false, "case L: mine failed");

    // Carol's wallet should see a payment under the integrated payment id
    carol_wlt->refresh();
    std::string payment_id_binary;
    CHECK_AND_ASSERT_MES(epee::string_tools::parse_hexstr_to_binbuff(integrated_payment_id_hex, payment_id_binary),
      false, "case L: parse payment_id_hex failed");

    std::list<tools::payment_details> payments;
    carol_wlt->get_payments(payment_id_binary, payments);
    CHECK_AND_ASSERT_MES(!payments.empty(), false, "case L: Carol must see a payment with integrated payment id " << integrated_payment_id_hex);
  }

  // case: confirm default asset_id (caller doesn't send the field at all)
  // means native sweep we just send a request whose asset_id was left default-initialized = native this is the "old client" backward-compat case
  {
    // top up Alice with a few extra small native UTXOs of a unique-ish amount
    const uint64_t a2_unit = MK_TEST_COINS(1);
    miner_wlt->refresh();
    for (size_t i = 0; i < 3; ++i)
      miner_wlt->transfer(a2_unit, alice_wlt->get_account().get_public_address(), currency::native_coin_asset_id);
    r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
    CHECK_AND_ASSERT_MES(r, false, "case A2 prep: mine failed");

    bob_wlt->refresh();
    const uint64_t bob_native_before_a2 = bob_wlt->balance(currency::native_coin_asset_id);

    alice_wlt->refresh();
    tools::wallet_rpc_server alice_rpc(alice_wlt);
    tools::wallet_public::COMMAND_SWEEP_BELOW::request req{};
    req.mixin = 15;
    req.address = m_accounts[BOB_ACC_IDX].get_public_address_str();
    req.amount = a2_unit + 1;
    req.fee = TESTS_DEFAULT_FEE;
    tools::wallet_public::COMMAND_SWEEP_BELOW::response res{};
    int code = call_sweep_below(alice_rpc, req, res);
    CHECK_AND_ASSERT_MES(code == 0, false, "case A2: default-asset_id native sweep failed, code=" << code);
    CHECK_AND_ASSERT_MES(res.asset_id == currency::native_coin_asset_id, false, "case A2: response asset_id must default to native");
    CHECK_AND_ASSERT_MES(res.outs_swept >= 3, false, "case A2: outs_swept=" << res.outs_swept << ", expected at least 3 (the freshly added small UTXOs)");
    CHECK_AND_ASSERT_MES(res.amount_swept == res.amount_total, false, "case A2: amount_swept != amount_total");
    CHECK_AND_ASSERT_MES(res.amount_swept >= 3 * a2_unit, false, "case A2: amount_swept=" << res.amount_swept << ", expected at least " << (3 * a2_unit));

    r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
    CHECK_AND_ASSERT_MES(r, false, "case A2: mine failed");

    bob_wlt->refresh();
    const uint64_t bob_native_after_a2 = bob_wlt->balance(currency::native_coin_asset_id);
    CHECK_AND_ASSERT_MES(bob_native_after_a2 - bob_native_before_a2 == res.amount_swept - TESTS_DEFAULT_FEE, false,
      "case A2: Bob delta=" << (bob_native_after_a2 - bob_native_before_a2) << ", expected " << (res.amount_swept - TESTS_DEFAULT_FEE));
  }

  // Case watch-only: cold-signing path for asset sweep
  {
    // new account Dan here because a watch-only wallet
    std::shared_ptr<tools::wallet2> dan_wlt = init_playtime_test_wallet(events, c, DAN_ACC_IDX);

    // Top up Dan with 2 small asset UTXOs to sweep + 1 native UTXO for the fee
    miner_wlt->refresh();
    for (size_t i = 0; i < 2; ++i)
      miner_wlt->transfer(small_asset, dan_wlt->get_account().get_public_address(), custom_asset_id);
    miner_wlt->transfer(MK_TEST_COINS(10), dan_wlt->get_account().get_public_address(), currency::native_coin_asset_id);
    r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
    CHECK_AND_ASSERT_MES(r, false, "case WO prep: mine failed");
    dan_wlt->refresh();

    const uint64_t dan_asset_initial  = dan_wlt->balance(custom_asset_id);
    const uint64_t dan_native_initial = dan_wlt->balance(currency::native_coin_asset_id);
    CHECK_AND_ASSERT_MES(dan_asset_initial == 2 * small_asset, false, "case WO: Dan asset balance=" << dan_asset_initial << ", expected " << (2 * small_asset));
    CHECK_AND_ASSERT_MES(dan_native_initial == MK_TEST_COINS(10), false, "case WO: Dan native balance=" << dan_native_initial << ", expected " << MK_TEST_COINS(10));

    // build Dan's watch-only twin from Dan's pub keys Dan has never spent anything yet, so the WO sees the same set of unspent outputs as the full wallet
    account_base dan_acc_wo = m_accounts[DAN_ACC_IDX];
    dan_acc_wo.make_account_watch_only();
    std::shared_ptr<tools::wallet2> dan_wlt_wo = init_playtime_test_wallet(events, c, dan_acc_wo);
    dan_wlt_wo->refresh();
    CHECK_AND_ASSERT_MES(dan_wlt_wo->balance(custom_asset_id) == dan_wlt->balance(custom_asset_id), false, "case WO: balances don't match between WO and full wallet (asset)");
    CHECK_AND_ASSERT_MES(dan_wlt_wo->balance(currency::native_coin_asset_id) == dan_wlt->balance(currency::native_coin_asset_id), false, "case WO: balances don't match between WO and full wallet (native)");

    tools::wallet_rpc_server dan_rpc_wo(dan_wlt_wo);
    tools::wallet_rpc_server dan_rpc(dan_wlt);

    // WO calls sweep_below
    tools::wallet_public::COMMAND_SWEEP_BELOW::request req{};
    req.mixin = 15;
    req.address = m_accounts[CAROL_ACC_IDX].get_public_address_str();
    req.amount = small_asset + 1; // matches the 2 small asset UTXOs in Dan's wallet
    req.fee = TESTS_DEFAULT_FEE;
    req.asset_id = custom_asset_id;
    tools::wallet_public::COMMAND_SWEEP_BELOW::response res{};
    int code = call_sweep_below(dan_rpc_wo, req, res);
    CHECK_AND_ASSERT_MES(code == 0, false, "case WO: sweep_below in WO wallet failed, code=" << code);

    // WO response: tx_hash is empty, tx_unsigned_hex is filled
    CHECK_AND_ASSERT_MES(res.tx_hash.empty(), false, "case WO: WO must not return final tx_hash");
    CHECK_AND_ASSERT_MES(!res.tx_unsigned_hex.empty(), false, "case WO: WO must return non-empty tx_unsigned_hex");
    CHECK_AND_ASSERT_MES(res.outs_swept == 2, false, "case WO: outs_swept=" << res.outs_swept);
    CHECK_AND_ASSERT_MES(res.amount_swept == 2 * small_asset, false, "case WO: amount_swept=" << res.amount_swept);
    CHECK_AND_ASSERT_MES(res.asset_id == custom_asset_id, false, "case WO: response asset_id mismatch");

    // Full wallet signs
    tools::wallet_public::COMMAND_SIGN_TRANSFER::request sign_req{};
    sign_req.tx_unsigned_hex = res.tx_unsigned_hex;
    tools::wallet_public::COMMAND_SIGN_TRANSFER::response sign_res{};
    epee::json_rpc::error je{};
    tools::wallet_rpc_server::connection_context ctx{};
    bool ok = dan_rpc.on_sign_transfer(sign_req, sign_res, je, ctx);
    CHECK_AND_ASSERT_MES(ok, false, "case WO: sign_transfer failed: " << je.message);
    CHECK_AND_ASSERT_MES(!sign_res.tx_signed_hex.empty(), false, "case WO: empty signed hex");

    // WO submits
    tools::wallet_public::COMMAND_SUBMIT_TRANSFER::request submit_req{};
    submit_req.tx_signed_hex = sign_res.tx_signed_hex;
    tools::wallet_public::COMMAND_SUBMIT_TRANSFER::response submit_res{};
    je = epee::json_rpc::error{};
    ok = dan_rpc_wo.on_submit_transfer(submit_req, submit_res, je, ctx);
    CHECK_AND_ASSERT_MES(ok, false, "case WO: submit_transfer failed: " << je.message);
    CHECK_AND_ASSERT_MES(!submit_res.tx_hash.empty(), false, "case WO: empty tx_hash in submit");
    CHECK_AND_ASSERT_MES(submit_res.tx_hash == sign_res.tx_hash, false, "case WO: tx_hash mismatch between sign and submit");

    // mine and verify Carol received
    carol_wlt->refresh();
    const uint64_t carol_asset_before = carol_wlt->balance(custom_asset_id);
    r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
    CHECK_AND_ASSERT_MES(r, false, "case WO: mine failed");
    carol_wlt->refresh();
    const uint64_t carol_asset_after = carol_wlt->balance(custom_asset_id);
    CHECK_AND_ASSERT_MES(carol_asset_after == carol_asset_before + 2 * small_asset, false,
      "case WO: Carol asset delta=" << (carol_asset_after - carol_asset_before) << ", expected " << (2 * small_asset));
  }

  // cases for max_inputs / min_outputs
  const uint64_t mio_unit = MK_TEST_COINS(7); // distinctive amount to avoid collision with leftover UTXOs
  miner_wlt->refresh();
  for (size_t i = 0; i < 4; ++i)
    miner_wlt->transfer(mio_unit, alice_wlt->get_account().get_public_address());
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "mio prep #1: mine failed");
  alice_wlt->refresh();

  // case: min_outputs above the allowed maximum -> WRONG_ARGUMENT
  {
    tools::wallet_rpc_server alice_rpc(alice_wlt);
    tools::wallet_public::COMMAND_SWEEP_BELOW::request req{};
    req.mixin = 15;
    req.address = m_accounts[BOB_ACC_IDX].get_public_address_str();
    req.amount = mio_unit + 1;
    req.fee = TESTS_DEFAULT_FEE;
    req.min_outputs = CURRENCY_TX_MAX_ALLOWED_OUTS + 1;
    tools::wallet_public::COMMAND_SWEEP_BELOW::response res{};
    int code = call_sweep_below(alice_rpc, req, res);
    CHECK_AND_ASSERT_MES(code == WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT, false, "case min_outputs>MAX: expected WRONG_ARGUMENT (" << WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT << "), got " << code);
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "unexpected pool txs count: " << c.get_pool_transactions_count());
  }

  // case: min_outputs = 4 forces the tx to have exactly 4 outputs
  {
    tools::wallet_rpc_server alice_rpc(alice_wlt);
    tools::wallet_public::COMMAND_SWEEP_BELOW::request req{};
    req.mixin = 15;
    req.address = m_accounts[BOB_ACC_IDX].get_public_address_str();
    req.amount = mio_unit + 1;
    req.fee = TESTS_DEFAULT_FEE;
    req.min_outputs = 4;
    tools::wallet_public::COMMAND_SWEEP_BELOW::response res{};
    int code = call_sweep_below(alice_rpc, req, res);
    CHECK_AND_ASSERT_MES(code == 0, false, "case min_outputs=4: sweep failed, code=" << code);
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "unexpected pool txs count: " << c.get_pool_transactions_count());

    std::list<transaction> pool_txs;
    r = c.get_pool_transactions(pool_txs);
    CHECK_AND_ASSERT_MES(r && pool_txs.size() == 1, false, "get_pool_transactions failed");
    CHECK_AND_ASSERT_MES(pool_txs.front().vout.size() == 4, false, "case min_outputs=4: vout.size()=" << pool_txs.front().vout.size() << ", expected 4");

    r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
    CHECK_AND_ASSERT_MES(r, false, "case min_outputs=4: mine failed");
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "tx pool not empty after mining");
  }

  // top up Alice with 4 more native UTXOs for the max_inputs case
  miner_wlt->refresh();
  for (size_t i = 0; i < 4; ++i)
    miner_wlt->transfer(mio_unit, alice_wlt->get_account().get_public_address(), currency::native_coin_asset_id);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "mio prep #2: mine failed");
  alice_wlt->refresh();

  // case: max_inputs caps the number of inputs selected
  {
    tools::wallet_rpc_server alice_rpc(alice_wlt);
    tools::wallet_public::COMMAND_SWEEP_BELOW::request req{};
    req.mixin = 15;
    req.address = m_accounts[BOB_ACC_IDX].get_public_address_str();
    req.amount = mio_unit + 1;
    req.fee = TESTS_DEFAULT_FEE;
    req.max_inputs = 2;
    tools::wallet_public::COMMAND_SWEEP_BELOW::response res{};
    int code = call_sweep_below(alice_rpc, req, res);
    CHECK_AND_ASSERT_MES(code == 0, false, "case max_inputs=2: sweep failed, code=" << code);
    CHECK_AND_ASSERT_MES(res.outs_total == 4, false, "case max_inputs=2: outs_total=" << res.outs_total << ", expected 4");
    CHECK_AND_ASSERT_MES(res.outs_swept == 2, false, "case max_inputs=2: outs_swept=" << res.outs_swept << ", expected 2");
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "unexpected pool txs count: " << c.get_pool_transactions_count());

    r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
    CHECK_AND_ASSERT_MES(r, false, "case max_inputs=2: mine failed");
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "tx pool not empty after mining");
  }

  return true;
}

// WO native sweep + store/reload between each cold-signing step
wallet_rpc_sweep_below_wo_native::wallet_rpc_sweep_below_wo_native()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_sweep_below_wo_native, c1);
}

bool wallet_rpc_sweep_below_wo_native::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);
  DO_CALLBACK(events, "c1");
  return true;
}

namespace
{
  // store WO wallet to disk and load it back fresh
  void wo_reload(std::shared_ptr<tools::wallet2>& w, std::shared_ptr<tools::wallet_rpc_server>& rpc, const std::wstring& filename, const std::string& password, currency::core& c, std::shared_ptr<tools::i_core_proxy> proxy)
  {
    w->reset_password(password);
    w->store(filename);
    rpc.reset();
    w.reset(new tools::wallet2);
    w->load(filename, password);
    w->set_core_proxy(proxy);
    w->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());
    w->set_disable_tor_relay(true);
    w->set_concise_mode(false);
    w->set_use_assets_whitelisting(false);
    rpc.reset(new tools::wallet_rpc_server(w));
  }
}

bool wallet_rpc_sweep_below_wo_native::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  miner_wlt->refresh();

  // 5 small UTXOs to sweep + 1 fat one to stay
  const uint64_t small_native = MK_TEST_COINS(2);
  const uint64_t fat_native   = MK_TEST_COINS(100);
  for (size_t i = 0; i < 5; ++i)
    miner_wlt->transfer(small_native, alice_wlt->get_account().get_public_address(), currency::native_coin_asset_id);
  miner_wlt->transfer(fat_native, alice_wlt->get_account().get_public_address(), currency::native_coin_asset_id);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "prep: mine failed");

  alice_wlt->refresh();
  const uint64_t alice_native_initial = alice_wlt->balance(currency::native_coin_asset_id);
  CHECK_AND_ASSERT_MES(alice_native_initial == 5 * small_native + fat_native, false, "Alice native balance unexpected: " << alice_native_initial);

  // WO twin
  account_base alice_acc_wo = m_accounts[ALICE_ACC_IDX];
  alice_acc_wo.make_account_watch_only();
  std::shared_ptr<tools::wallet2> alice_wlt_wo = init_playtime_test_wallet(events, c, alice_acc_wo);
  alice_wlt_wo->refresh();
  CHECK_AND_ASSERT_MES(alice_wlt_wo->balance(currency::native_coin_asset_id) == alice_native_initial, false, "WO balance != full balance: " << alice_wlt_wo->balance(currency::native_coin_asset_id));

  boost::filesystem::remove(epee::string_tools::cut_off_extension(m_wallet_filename) + L".outkey2ki");
  std::shared_ptr<tools::wallet_rpc_server> alice_rpc_wo;
  wo_reload(alice_wlt_wo, alice_rpc_wo, m_wallet_filename, m_wallet_password, c, m_core_proxy);

  size_t blocks_fetched = 0;
  bool stub0{}, stub1{};
  std::atomic_bool stub2{};
  alice_wlt_wo->refresh(blocks_fetched, stub0, stub1, stub2);
  CHECK_AND_ASSERT_MES(alice_wlt_wo->balance(currency::native_coin_asset_id) == alice_native_initial, false, "WO balance after reload mismatch");

  // WO builds unsigned blob
  tools::wallet_public::COMMAND_SWEEP_BELOW::request req{};
  req.mixin = 15;
  req.address = m_accounts[BOB_ACC_IDX].get_public_address_str();
  req.amount = small_native + 1; // strict < ; only the 5 small UTXOs match
  req.fee = TESTS_DEFAULT_FEE;
  tools::wallet_public::COMMAND_SWEEP_BELOW::response res{};
  int code = call_sweep_below(*alice_rpc_wo, req, res);
  CHECK_AND_ASSERT_MES(code == 0, false, "WO sweep_below failed, code=" << code);
  CHECK_AND_ASSERT_MES(res.tx_hash.empty(), false, "WO must not return tx_hash");
  CHECK_AND_ASSERT_MES(!res.tx_unsigned_hex.empty(), false, "empty tx_unsigned_hex");
  CHECK_AND_ASSERT_MES(res.outs_swept == 5, false, "outs_swept=" << res.outs_swept);
  CHECK_AND_ASSERT_MES(res.amount_swept == 5 * small_native, false, "amount_swept=" << res.amount_swept);
  CHECK_AND_ASSERT_MES(res.asset_id == currency::native_coin_asset_id, false, "response asset_id must be native");

  const std::string unsigned_hex_snapshot = res.tx_unsigned_hex;

  // reload between sweep_below and sign_transfer
  wo_reload(alice_wlt_wo, alice_rpc_wo, m_wallet_filename, m_wallet_password, c, m_core_proxy);
  alice_wlt_wo->refresh(blocks_fetched, stub0, stub1, stub2);

  // full wallet signs the snapshot
  tools::wallet_rpc_server alice_rpc(alice_wlt);
  tools::wallet_public::COMMAND_SIGN_TRANSFER::request sign_req{};
  sign_req.tx_unsigned_hex = unsigned_hex_snapshot;
  tools::wallet_public::COMMAND_SIGN_TRANSFER::response sign_res{};
  epee::json_rpc::error je{};
  tools::wallet_rpc_server::connection_context ctx{};
  bool ok = alice_rpc.on_sign_transfer(sign_req, sign_res, je, ctx);
  CHECK_AND_ASSERT_MES(ok, false, "sign_transfer failed: " << je.message);
  CHECK_AND_ASSERT_MES(!sign_res.tx_signed_hex.empty(), false, "empty signed hex");

  // reload between sign and submit
  wo_reload(alice_wlt_wo, alice_rpc_wo, m_wallet_filename, m_wallet_password, c, m_core_proxy);

  tools::wallet_public::COMMAND_SUBMIT_TRANSFER::request submit_req{};
  submit_req.tx_signed_hex = sign_res.tx_signed_hex;
  tools::wallet_public::COMMAND_SUBMIT_TRANSFER::response submit_res{};
  je = epee::json_rpc::error{};
  ok = alice_rpc_wo->on_submit_transfer(submit_req, submit_res, je, ctx);
  CHECK_AND_ASSERT_MES(ok, false, "submit_transfer failed: " << je.message);
  CHECK_AND_ASSERT_MES(submit_res.tx_hash == sign_res.tx_hash, false, "tx_hash mismatch");

  // Bob = (5*small - fee), Alice keeps fat
  bob_wlt->refresh();
  const uint64_t bob_before = bob_wlt->balance(currency::native_coin_asset_id);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "mine failed");
  bob_wlt->refresh();
  const uint64_t bob_after = bob_wlt->balance(currency::native_coin_asset_id);
  CHECK_AND_ASSERT_MES(bob_after - bob_before == 5 * small_native - TESTS_DEFAULT_FEE, false,
    "Bob delta=" << (bob_after - bob_before) << ", expected " << (5 * small_native - TESTS_DEFAULT_FEE));

  alice_wlt->refresh();
  CHECK_AND_ASSERT_MES(alice_wlt->balance(currency::native_coin_asset_id) == fat_native, false,
    "Alice native left=" << alice_wlt->balance(currency::native_coin_asset_id) << ", expected " << fat_native);

  return true;
}

// WO sweep_below reserves UTXOs; clear_utxo_cold_sig_reservation releases them
wallet_rpc_sweep_below_wo_reservation::wallet_rpc_sweep_below_wo_reservation()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_sweep_below_wo_reservation, c1);
}

bool wallet_rpc_sweep_below_wo_reservation::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);
  DO_CALLBACK(events, "c1");
  return true;
}

bool wallet_rpc_sweep_below_wo_reservation::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt   = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  miner_wlt->refresh();

  const uint64_t small_native = MK_TEST_COINS(2);
  for (size_t i = 0; i < 4; ++i)
    miner_wlt->transfer(small_native, alice_wlt->get_account().get_public_address(), currency::native_coin_asset_id);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "prep: mine failed");

  alice_wlt->refresh();
  CHECK_AND_ASSERT_MES(alice_wlt->balance(currency::native_coin_asset_id) == 4 * small_native, false, "Alice unexpected native balance");

  account_base alice_acc_wo = m_accounts[ALICE_ACC_IDX];
  alice_acc_wo.make_account_watch_only();
  std::shared_ptr<tools::wallet2> alice_wlt_wo = init_playtime_test_wallet(events, c, alice_acc_wo);
  alice_wlt_wo->refresh();
  CHECK_AND_ASSERT_MES(alice_wlt_wo->balance(currency::native_coin_asset_id) == 4 * small_native, false, "WO balance mismatch");

  tools::wallet_rpc_server alice_rpc_wo(alice_wlt_wo);
  tools::wallet_rpc_server alice_rpc(alice_wlt);

  // first sweep: reserves all 4 UTXOs
  tools::wallet_public::COMMAND_SWEEP_BELOW::request req{};
  req.mixin = 15;
  req.address = m_accounts[BOB_ACC_IDX].get_public_address_str();
  req.amount = small_native + 1;
  req.fee = TESTS_DEFAULT_FEE;
  tools::wallet_public::COMMAND_SWEEP_BELOW::response res1{};
  int code = call_sweep_below(alice_rpc_wo, req, res1);
  CHECK_AND_ASSERT_MES(code == 0, false, "first sweep_below failed, code=" << code);
  CHECK_AND_ASSERT_MES(res1.outs_swept == 4, false, "outs_swept=" << res1.outs_swept);
  CHECK_AND_ASSERT_MES(!res1.tx_unsigned_hex.empty(), false, "empty unsigned hex (1)");

  // second sweep without clear must fail — UTXOs are reserved
  tools::wallet_public::COMMAND_SWEEP_BELOW::response res2{};
  code = call_sweep_below(alice_rpc_wo, req, res2);
  CHECK_AND_ASSERT_MES(code != 0, false, "second sweep_below must fail on reserved UTXOs, code=" << code);
  CHECK_AND_ASSERT_MES(res2.tx_unsigned_hex.empty(), false, "failed sweep_below must not return blob");

  // clear releases them
  tools::wallet_public::COMMAND_CLEAR_UTXO_COLD_SIG_RESERVATION::request clear_req{};
  tools::wallet_public::COMMAND_CLEAR_UTXO_COLD_SIG_RESERVATION::response clear_res{};
  epee::json_rpc::error je{};
  tools::wallet_rpc_server::connection_context ctx{};
  bool ok = alice_rpc_wo.on_clear_utxo_cold_sig_reservation(clear_req, clear_res, je, ctx);
  CHECK_AND_ASSERT_MES(ok, false, "clear failed: " << je.message);
  CHECK_AND_ASSERT_MES(clear_res.affected_outputs.size() == 4, false, "clear released " << clear_res.affected_outputs.size() << ", expected 4");

  // sweep works again
  tools::wallet_public::COMMAND_SWEEP_BELOW::response res3{};
  code = call_sweep_below(alice_rpc_wo, req, res3);
  CHECK_AND_ASSERT_MES(code == 0, false, "third sweep_below after clear failed, code=" << code);
  CHECK_AND_ASSERT_MES(res3.outs_swept == 4, false, "outs_swept=" << res3.outs_swept);
  CHECK_AND_ASSERT_MES(!res3.tx_unsigned_hex.empty(), false, "empty unsigned hex (3)");

  // full-keys wallet must refuse clear
  je = epee::json_rpc::error{};
  ok = alice_rpc.on_clear_utxo_cold_sig_reservation(clear_req, clear_res, je, ctx);
  CHECK_AND_ASSERT_MES(!ok, false, "clear must fail on full-keys wallet");

  // sign + submit, Bob must actually receive the money
  tools::wallet_public::COMMAND_SIGN_TRANSFER::request sign_req{};
  sign_req.tx_unsigned_hex = res3.tx_unsigned_hex;
  tools::wallet_public::COMMAND_SIGN_TRANSFER::response sign_res{};
  je = epee::json_rpc::error{};
  ok = alice_rpc.on_sign_transfer(sign_req, sign_res, je, ctx);
  CHECK_AND_ASSERT_MES(ok, false, "sign_transfer failed: " << je.message);

  tools::wallet_public::COMMAND_SUBMIT_TRANSFER::request submit_req{};
  submit_req.tx_signed_hex = sign_res.tx_signed_hex;
  tools::wallet_public::COMMAND_SUBMIT_TRANSFER::response submit_res{};
  je = epee::json_rpc::error{};
  ok = alice_rpc_wo.on_submit_transfer(submit_req, submit_res, je, ctx);
  CHECK_AND_ASSERT_MES(ok, false, "submit_transfer failed: " << je.message);

  bob_wlt->refresh();
  const uint64_t bob_before = bob_wlt->balance(currency::native_coin_asset_id);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "mine failed");
  bob_wlt->refresh();
  CHECK_AND_ASSERT_MES(bob_wlt->balance(currency::native_coin_asset_id) - bob_before == 4 * small_native - TESTS_DEFAULT_FEE, false,
    "Bob delta=" << (bob_wlt->balance(currency::native_coin_asset_id) - bob_before) << ", expected " << (4 * small_native - TESTS_DEFAULT_FEE));

  return true;
}

// repeated sweep_below on the same UTXO set must fail (full-keys path)
wallet_rpc_sweep_below_double_sweep::wallet_rpc_sweep_below_double_sweep()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_sweep_below_double_sweep, c1);
}

bool wallet_rpc_sweep_below_double_sweep::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);
  DO_CALLBACK(events, "c1");
  return true;
}

bool wallet_rpc_sweep_below_double_sweep::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt   = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  miner_wlt->refresh();

  const uint64_t small_native = MK_TEST_COINS(2);
  for (size_t i = 0; i < 3; ++i)
    miner_wlt->transfer(small_native, alice_wlt->get_account().get_public_address(), currency::native_coin_asset_id);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "prep: mine failed");
  alice_wlt->refresh();
  CHECK_AND_ASSERT_MES(alice_wlt->balance(currency::native_coin_asset_id) == 3 * small_native, false,
    "Alice unexpected native balance");

  tools::wallet_rpc_server alice_rpc(alice_wlt);

  // first sweep broadcasts the tx
  tools::wallet_public::COMMAND_SWEEP_BELOW::request req{};
  req.mixin = 15;
  req.address = m_accounts[BOB_ACC_IDX].get_public_address_str();
  req.amount = small_native + 1;
  req.fee = TESTS_DEFAULT_FEE;
  tools::wallet_public::COMMAND_SWEEP_BELOW::response res1{};
  int code = call_sweep_below(alice_rpc, req, res1);
  CHECK_AND_ASSERT_MES(code == 0, false, "first sweep_below failed, code=" << code);
  CHECK_AND_ASSERT_MES(!res1.tx_hash.empty(), false, "first sweep must return tx_hash");
  CHECK_AND_ASSERT_MES(res1.outs_swept == 3, false, "outs_swept=" << res1.outs_swept);
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "tx must be in pool");

  // before mining: UTXOs are already flagged spent locally → must fail
  tools::wallet_public::COMMAND_SWEEP_BELOW::response res2{};
  code = call_sweep_below(alice_rpc, req, res2);
  CHECK_AND_ASSERT_MES(code != 0, false, "sweep_below before mining must fail, code=" << code);

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "mine failed");
  alice_wlt->refresh();
  CHECK_AND_ASSERT_MES(alice_wlt->balance(currency::native_coin_asset_id) == 0, false,
    "Alice native after sweep should be 0, got " << alice_wlt->balance(currency::native_coin_asset_id));

  // after mining: nothing left below threshold -> still fails
  tools::wallet_public::COMMAND_SWEEP_BELOW::response res3{};
  code = call_sweep_below(alice_rpc, req, res3);
  CHECK_AND_ASSERT_MES(code != 0, false, "sweep_below on empty balance must fail, code=" << code);

  // top up -> sweep works again
  miner_wlt->refresh();
  for (size_t i = 0; i < 2; ++i)
    miner_wlt->transfer(small_native, alice_wlt->get_account().get_public_address(), currency::native_coin_asset_id);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "top-up mine failed");
  alice_wlt->refresh();

  tools::wallet_public::COMMAND_SWEEP_BELOW::response res4{};
  code = call_sweep_below(alice_rpc, req, res4);
  CHECK_AND_ASSERT_MES(code == 0, false, "sweep after top-up failed, code=" << code);
  CHECK_AND_ASSERT_MES(res4.outs_swept == 2, false, "outs_swept=" << res4.outs_swept);

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "final mine failed");

  return true;
}

// WO sweep_below(A) when WO holds two distinct custom assets - B must stay intact
wallet_rpc_sweep_below_wo_multi_asset::wallet_rpc_sweep_below_wo_multi_asset()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_sweep_below_wo_multi_asset, c1);
}

bool wallet_rpc_sweep_below_wo_multi_asset::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);
  account_base& carol_acc = m_accounts[CAROL_ACC_IDX]; carol_acc.generate(); carol_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);
  DO_CALLBACK(events, "c1");
  return true;
}

bool wallet_rpc_sweep_below_wo_multi_asset::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  CHECK_AND_ASSERT_MES(c.get_blockchain_storage().is_hardfork_active(ZANO_HARDFORK_04_ZARCANUM), false,
    "multi-asset WO test requires HF4");

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  std::shared_ptr<tools::wallet2> carol_wlt = init_playtime_test_wallet(events, c, CAROL_ACC_IDX);
  miner_wlt->refresh();

  const size_t dec = 6;
  const uint64_t per_unit = 100ull * 1000000ull;
  const uint64_t total_supply = 1000ull * 1000000ull;

  crypto::public_key asset_A{};
  crypto::public_key asset_B{};
  {
    currency::asset_descriptor_base adb{};
    adb.total_max_supply = total_supply;
    adb.full_name = "Multi Asset A";
    adb.ticker = "MAA";
    adb.decimal_point = dec;
    std::vector<currency::tx_destination_entry> dsts;
    for (size_t i = 0; i < 4; ++i)
    {
      currency::tx_destination_entry d{};
      d.addr.push_back(miner_wlt->get_account().get_public_address());
      d.amount = per_unit;
      d.asset_id = currency::null_pkey;
      dsts.push_back(d);
    }
    currency::transaction tx{};
    miner_wlt->deploy_new_asset(adb, dsts, tx, asset_A);
  }
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "deploy A: mine failed");

  {
    currency::asset_descriptor_base adb{};
    adb.total_max_supply = total_supply;
    adb.full_name = "Multi Asset B";
    adb.ticker = "MAB";
    adb.decimal_point = dec;
    std::vector<currency::tx_destination_entry> dsts;
    for (size_t i = 0; i < 4; ++i)
    {
      currency::tx_destination_entry d{};
      d.addr.push_back(miner_wlt->get_account().get_public_address());
      d.amount = per_unit;
      d.asset_id = currency::null_pkey;
      dsts.push_back(d);
    }
    currency::transaction tx{};
    miner_wlt->deploy_new_asset(adb, dsts, tx, asset_B);
  }
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "deploy B: mine failed");

  miner_wlt->refresh();
  CHECK_AND_ASSERT_MES(miner_wlt->balance(asset_A) == 4 * per_unit, false, "miner asset_A balance wrong");
  CHECK_AND_ASSERT_MES(miner_wlt->balance(asset_B) == 4 * per_unit, false, "miner asset_B balance wrong");

  // 3x small A + 2x small B + native for fee
  const uint64_t small_a = 20ull * 1000000ull;
  const uint64_t small_b = 30ull * 1000000ull;
  for (size_t i = 0; i < 3; ++i)
    miner_wlt->transfer(small_a, alice_wlt->get_account().get_public_address(), asset_A);
  for (size_t i = 0; i < 2; ++i)
    miner_wlt->transfer(small_b, alice_wlt->get_account().get_public_address(), asset_B);
  miner_wlt->transfer(MK_TEST_COINS(10), alice_wlt->get_account().get_public_address(), currency::native_coin_asset_id);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "fund Alice: mine failed");

  alice_wlt->refresh();
  CHECK_AND_ASSERT_MES(alice_wlt->balance(asset_A) == 3 * small_a, false, "Alice asset_A balance wrong");
  CHECK_AND_ASSERT_MES(alice_wlt->balance(asset_B) == 2 * small_b, false, "Alice asset_B balance wrong");
  CHECK_AND_ASSERT_MES(alice_wlt->balance(currency::native_coin_asset_id) == MK_TEST_COINS(10), false, "Alice native balance wrong");

  account_base alice_acc_wo = m_accounts[ALICE_ACC_IDX];
  alice_acc_wo.make_account_watch_only();
  std::shared_ptr<tools::wallet2> alice_wlt_wo = init_playtime_test_wallet(events, c, alice_acc_wo);
  alice_wlt_wo->refresh();
  CHECK_AND_ASSERT_MES(alice_wlt_wo->balance(asset_A) == alice_wlt->balance(asset_A), false, "WO asset_A != full");
  CHECK_AND_ASSERT_MES(alice_wlt_wo->balance(asset_B) == alice_wlt->balance(asset_B), false, "WO asset_B != full");

  tools::wallet_rpc_server alice_rpc_wo(alice_wlt_wo);
  tools::wallet_rpc_server alice_rpc(alice_wlt);

  // sweep A only - B must stay untouched
  tools::wallet_public::COMMAND_SWEEP_BELOW::request req{};
  req.mixin = 15;
  req.address = m_accounts[CAROL_ACC_IDX].get_public_address_str();
  req.amount = small_a + 1;
  req.fee = TESTS_DEFAULT_FEE;
  req.asset_id = asset_A;
  tools::wallet_public::COMMAND_SWEEP_BELOW::response res{};
  int code = call_sweep_below(alice_rpc_wo, req, res);
  CHECK_AND_ASSERT_MES(code == 0, false, "sweep_below(A) failed, code=" << code);
  CHECK_AND_ASSERT_MES(res.asset_id == asset_A, false, "response asset_id mismatch");
  CHECK_AND_ASSERT_MES(res.outs_swept == 3, false, "outs_swept=" << res.outs_swept);
  CHECK_AND_ASSERT_MES(res.amount_swept == 3 * small_a, false, "amount_swept=" << res.amount_swept);
  CHECK_AND_ASSERT_MES(res.tx_hash.empty() && !res.tx_unsigned_hex.empty(), false, "WO must return blob, no tx_hash");

  tools::wallet_public::COMMAND_SIGN_TRANSFER::request sign_req{};
  sign_req.tx_unsigned_hex = res.tx_unsigned_hex;
  tools::wallet_public::COMMAND_SIGN_TRANSFER::response sign_res{};
  epee::json_rpc::error je{};
  tools::wallet_rpc_server::connection_context ctx{};
  bool ok = alice_rpc.on_sign_transfer(sign_req, sign_res, je, ctx);
  CHECK_AND_ASSERT_MES(ok, false, "sign_transfer failed: " << je.message);

  tools::wallet_public::COMMAND_SUBMIT_TRANSFER::request submit_req{};
  submit_req.tx_signed_hex = sign_res.tx_signed_hex;
  tools::wallet_public::COMMAND_SUBMIT_TRANSFER::response submit_res{};
  je = epee::json_rpc::error{};
  ok = alice_rpc_wo.on_submit_transfer(submit_req, submit_res, je, ctx);
  CHECK_AND_ASSERT_MES(ok, false, "submit_transfer failed: " << je.message);

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "mine failed");

  carol_wlt->refresh();
  CHECK_AND_ASSERT_MES(carol_wlt->balance(asset_A) == 3 * small_a, false, "Carol asset_A=" << carol_wlt->balance(asset_A));
  CHECK_AND_ASSERT_MES(carol_wlt->balance(asset_B) == 0, false, "Carol must not get asset_B");
  CHECK_AND_ASSERT_MES(carol_wlt->balance(currency::native_coin_asset_id) == 0, false, "Carol must not get native");

  alice_wlt->refresh();
  CHECK_AND_ASSERT_MES(alice_wlt->balance(asset_A) == 0, false, "Alice asset_A leftover=" << alice_wlt->balance(asset_A));
  CHECK_AND_ASSERT_MES(alice_wlt->balance(asset_B) == 2 * small_b, false, "Alice asset_B touched: " << alice_wlt->balance(asset_B));
  CHECK_AND_ASSERT_MES(alice_wlt->balance(currency::native_coin_asset_id) == MK_TEST_COINS(10) - TESTS_DEFAULT_FEE, false,
    "Alice native after fee=" << alice_wlt->balance(currency::native_coin_asset_id));

  return true;
}
//------------------------------------------------------------------------------

wallet_rpc_get_outputs_and_utxo_stats::wallet_rpc_get_outputs_and_utxo_stats()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_get_outputs_and_utxo_stats, c1);
}

bool wallet_rpc_get_outputs_and_utxo_stats::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);
  DO_CALLBACK(events, "c1");
  return true;
}

bool wallet_rpc_get_outputs_and_utxo_stats::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // Covers wallet RPCs 'get_outputs' and 'get_utxo_stats'
  // Tests filter combinations (output_type, asset_id), spent/unspent transitions, per-output fields, and bucket distribution of UTXO statistics
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt   = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  miner_wlt->refresh();

  const bool use_assets = c.get_blockchain_storage().is_hardfork_active(ZANO_HARDFORK_04_ZARCANUM);
  CHECK_AND_ASSERT_MES(use_assets, false, "test requires HF4 (Zarcanum) to be active for asset path");

  // deploy a custom asset with several UTXOs going to the miner
  crypto::public_key custom_asset_id{};
  const uint64_t asset_total_max_supply = 1000ull * 1000000ull;
  const uint64_t asset_emit_per_destination = 100ull * 1000000ull;
  {
    currency::asset_descriptor_base adb{};
    adb.total_max_supply = asset_total_max_supply;
    adb.full_name = "Get Utxo Stats";
    adb.ticker = "GUS";
    adb.decimal_point = 6;

    std::vector<currency::tx_destination_entry> dsts;
    for (size_t i = 0; i < 4; ++i)
    {
      currency::tx_destination_entry d{};
      d.addr.push_back(miner_wlt->get_account().get_public_address());
      d.amount = asset_emit_per_destination;
      d.asset_id = currency::null_pkey;
      dsts.push_back(d);
    }

    currency::transaction tx{};
    miner_wlt->deploy_new_asset(adb, dsts, tx, custom_asset_id);
  }
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "mine after asset deploy failed");
  miner_wlt->refresh();

  // seed Alice with a spread of native UTXOs so utxo_stats buckets are non-trivial
  //   amounts: 3, 7, 25, 90, 300 - one UTXO per amount
  const std::vector<uint64_t> alice_native_amounts =
  {
    MK_TEST_COINS(3), MK_TEST_COINS(7), MK_TEST_COINS(25), MK_TEST_COINS(90), MK_TEST_COINS(300)
  };
  for (uint64_t a : alice_native_amounts)
    miner_wlt->transfer(a, alice_wlt->get_account().get_public_address(), currency::native_coin_asset_id);

  // seed Alice with two custom asset UTXOs of different magnitude so we exercise asset filtering as well
  const uint64_t alice_asset_small = 5ull * 1000000ull;
  const uint64_t alice_asset_big   = 80ull * 1000000ull;
  miner_wlt->transfer(alice_asset_small, alice_wlt->get_account().get_public_address(), custom_asset_id);
  miner_wlt->transfer(alice_asset_big,   alice_wlt->get_account().get_public_address(), custom_asset_id);

  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "mine after seeding Alice failed");
  alice_wlt->refresh();

  // sanity check on balances
  CHECK_AND_ASSERT_MES(alice_wlt->balance(currency::native_coin_asset_id) == std::accumulate(alice_native_amounts.begin(), alice_native_amounts.end(), uint64_t{0}),
    false, "Alice native balance mismatch");
  CHECK_AND_ASSERT_MES(alice_wlt->balance(custom_asset_id) == alice_asset_small + alice_asset_big, false, "Alice asset balance mismatch");

  tools::wallet_rpc_server alice_rpc(alice_wlt);
  epee::json_rpc::error je{};
  tools::wallet_rpc_server::connection_context ctx{};

  // get_outputs no filters
  {
    tools::wallet_public::COMMAND_RPC_GET_OUTPUTS::request req{};
    tools::wallet_public::COMMAND_RPC_GET_OUTPUTS::response res{};
    bool ok = alice_rpc.on_get_outputs(req, res, je, ctx);
    CHECK_AND_ASSERT_MES(ok, false, "on_get_outputs (default) failed: " << je.message);
    // 5 native + 2 asset = 7
    CHECK_AND_ASSERT_MES(res.outputs.size() == alice_native_amounts.size() + 2, false,
      "default get_outputs returned " << res.outputs.size() << ", expected " << (alice_native_amounts.size() + 2));

    // out_id values must be unique within a single response
    std::set<uint64_t> out_ids;
    for (const auto& od : res.outputs)
    {
      auto ins = out_ids.insert(od.out_id);
      CHECK_AND_ASSERT_MES(ins.second, false, "duplicate out_id=" << od.out_id);
    }

    // verify per-field invariants on freshly-received outputs
    size_t native_count = 0, asset_count = 0;
    for (const auto& od : res.outputs)
    {
      CHECK_AND_ASSERT_MES(!od.spent, false, "unexpected spent=true for freshly-received output, out_id=" << od.out_id);
      CHECK_AND_ASSERT_MES(od.spendable, false, "unexpected spendable=false for freshly-received output, out_id=" << od.out_id);
      CHECK_AND_ASSERT_MES(od.spent_height == 0, false, "spent_height must be 0 for unspent, out_id=" << od.out_id);
      CHECK_AND_ASSERT_MES((od.flags & WALLET_TRANSFER_DETAIL_FLAG_SPENT) == 0, false, "WALLET_TRANSFER_DETAIL_FLAG_SPENT must be clear, out_id=" << od.out_id);
      CHECK_AND_ASSERT_MES(od.block_height != 0, false, "block_height must be set, out_id=" << od.out_id);
      CHECK_AND_ASSERT_MES(od.pub_key != currency::null_pkey, false, "pub_key must be set, out_id=" << od.out_id);
      CHECK_AND_ASSERT_MES(od.tx_id != currency::null_hash, false, "tx_id must be set, out_id=" << od.out_id);
      if (od.native_coin)
      {
        CHECK_AND_ASSERT_MES(od.asset_id == currency::native_coin_asset_id, false, "native_coin=true but asset_id is not native");
        ++native_count;
      }
      else
      {
        CHECK_AND_ASSERT_MES(od.asset_id == custom_asset_id, false, "non-native asset_id mismatch for out_id=" << od.out_id);
        ++asset_count;
      }
    }
    CHECK_AND_ASSERT_MES(native_count == alice_native_amounts.size(), false, "native count=" << native_count);
    CHECK_AND_ASSERT_MES(asset_count  == 2, false, "asset count=" << asset_count);
  }

  // get_outputs explicit 'unspent' and 'spent' alias -> 'unavailable' must be empty here
  for (const std::string& type : {std::string("unspent"), std::string("available")})
  {
    tools::wallet_public::COMMAND_RPC_GET_OUTPUTS::request req{};
    req.output_type = type;
    tools::wallet_public::COMMAND_RPC_GET_OUTPUTS::response res{};
    bool ok = alice_rpc.on_get_outputs(req, res, je, ctx);
    CHECK_AND_ASSERT_MES(ok, false, "on_get_outputs(" << type << ") failed: " << je.message);
    CHECK_AND_ASSERT_MES(res.outputs.size() == alice_native_amounts.size() + 2, false,
      "get_outputs(" << type << ") returned " << res.outputs.size());
  }
  for (const std::string& type : {std::string("spent"), std::string("unavailable")})
  {
    tools::wallet_public::COMMAND_RPC_GET_OUTPUTS::request req{};
    req.output_type = type;
    tools::wallet_public::COMMAND_RPC_GET_OUTPUTS::response res{};
    bool ok = alice_rpc.on_get_outputs(req, res, je, ctx);
    CHECK_AND_ASSERT_MES(ok, false, "on_get_outputs(" << type << ") failed: " << je.message);
    CHECK_AND_ASSERT_MES(res.outputs.empty(), false,
      "get_outputs(" << type << ") returned " << res.outputs.size() << ", expected 0 (nothing spent yet)");
  }

  // get_outputs asset_id filter
  {
    tools::wallet_public::COMMAND_RPC_GET_OUTPUTS::request req{};
    req.asset_id = custom_asset_id;
    tools::wallet_public::COMMAND_RPC_GET_OUTPUTS::response res{};
    bool ok = alice_rpc.on_get_outputs(req, res, je, ctx);
    CHECK_AND_ASSERT_MES(ok, false, "on_get_outputs(asset_id=custom) failed: " << je.message);
    CHECK_AND_ASSERT_MES(res.outputs.size() == 2, false, "asset filter returned " << res.outputs.size() << ", expected 2");
    uint64_t sum = 0;
    for (const auto& od : res.outputs)
    {
      CHECK_AND_ASSERT_MES(od.asset_id == custom_asset_id, false, "asset_id filter leaked a foreign output");
      CHECK_AND_ASSERT_MES(!od.native_coin, false, "native_coin must be false for custom asset");
      sum += od.amount;
    }
    CHECK_AND_ASSERT_MES(sum == alice_asset_small + alice_asset_big, false, "asset sum=" << sum);
  }
  {
    tools::wallet_public::COMMAND_RPC_GET_OUTPUTS::request req{};
    req.asset_id = currency::native_coin_asset_id;
    tools::wallet_public::COMMAND_RPC_GET_OUTPUTS::response res{};
    bool ok = alice_rpc.on_get_outputs(req, res, je, ctx);
    CHECK_AND_ASSERT_MES(ok, false, "on_get_outputs(asset_id=native) failed: " << je.message);
    CHECK_AND_ASSERT_MES(res.outputs.size() == alice_native_amounts.size(), false,
      "native filter returned " << res.outputs.size());
    for (const auto& od : res.outputs)
      CHECK_AND_ASSERT_MES(od.native_coin && od.asset_id == currency::native_coin_asset_id, false, "native filter leaked");
  }

  // get_outputs unknown asset id -> empty response, not an error
  {
    tools::wallet_public::COMMAND_RPC_GET_OUTPUTS::request req{};
    crypto::secret_key dummy_sk{};
    crypto::generate_keys(req.asset_id, dummy_sk); // random pkey, definitely not in wallet
    tools::wallet_public::COMMAND_RPC_GET_OUTPUTS::response res{};
    bool ok = alice_rpc.on_get_outputs(req, res, je, ctx);
    CHECK_AND_ASSERT_MES(ok, false, "on_get_outputs(random asset) failed: " << je.message);
    CHECK_AND_ASSERT_MES(res.outputs.empty(), false, "random asset filter must yield 0 outputs, got " << res.outputs.size());
  }

  // get_utxo_stats: native coin
  // expected buckets for alice_native_amounts = {3, 7, 25, 90, 300}:
  //   [10^10..10^11-1]  -> {3, 7},        count=2, sum=1 = 1
  //   [10^11..10^12-1]  -> {25, 90},      count=2, sum=115 = 1.15
  //   [10^12..10^13-1]  -> {300 = 3},     count=1, sum=3
  // all other buckets must be omitted from the response the implementation pops empty buckets
  {
    tools::wallet_public::COMMAND_RPC_GET_UTXO_STATS::request req{};
    // req.asset_id defaults to native_coin_asset_id
    tools::wallet_public::COMMAND_RPC_GET_UTXO_STATS::response res{};
    bool ok = alice_rpc.on_get_utxo_stats(req, res, je, ctx);
    CHECK_AND_ASSERT_MES(ok, false, "on_get_utxo_stats (native) failed: " << je.message);
    CHECK_AND_ASSERT_MES(res.asset_id == currency::native_coin_asset_id, false, "asset_id echoed back must be native");
    CHECK_AND_ASSERT_MES(res.buckets.size() == 3, false, "expected 3 non-empty buckets, got " << res.buckets.size());

    // verify each bucket's contents
    uint64_t total_utxo_sum = 0, total_amount_sum = 0;
    for (const auto& b : res.buckets)
    {
      CHECK_AND_ASSERT_MES(b.total_utxo > 0, false, "empty bucket must have been popped: [" << b.lower_bound << "," << b.upper_bound << "]");
      CHECK_AND_ASSERT_MES(b.lower_bound <= b.upper_bound, false, "bad bucket bounds");
      total_utxo_sum   += b.total_utxo;
      total_amount_sum += b.total_amount;

      if (b.lower_bound == 10000000000ull) // 10^10
      {
        CHECK_AND_ASSERT_MES(b.total_utxo == 2, false, "bucket [1e10..) total_utxo=" << b.total_utxo);
        CHECK_AND_ASSERT_MES(b.total_amount == MK_TEST_COINS(3) + MK_TEST_COINS(7), false, "bucket [1e10..) total_amount=" << b.total_amount);
      }
      else if (b.lower_bound == 100000000000ull) // 10^11
      {
        CHECK_AND_ASSERT_MES(b.total_utxo == 2, false, "bucket [1e11..) total_utxo=" << b.total_utxo);
        CHECK_AND_ASSERT_MES(b.total_amount == MK_TEST_COINS(25) + MK_TEST_COINS(90), false, "bucket [1e11..) total_amount=" << b.total_amount);
      }
      else if (b.lower_bound == 1000000000000ull) // 10^12
      {
        CHECK_AND_ASSERT_MES(b.total_utxo == 1, false, "bucket [1e12..) total_utxo=" << b.total_utxo);
        CHECK_AND_ASSERT_MES(b.total_amount == MK_TEST_COINS(300), false, "bucket [1e12..) total_amount=" << b.total_amount);
      }
      else
      {
        CHECK_AND_ASSERT_MES(false, false, "unexpected bucket lower_bound=" << b.lower_bound);
      }
    }
    CHECK_AND_ASSERT_MES(total_utxo_sum == alice_native_amounts.size(), false, "total UTXO sum=" << total_utxo_sum);
    CHECK_AND_ASSERT_MES(total_amount_sum == alice_wlt->balance(currency::native_coin_asset_id), false, "total amount sum mismatch");
  }

  // get_utxo_stats: custom asset
  {
    tools::wallet_public::COMMAND_RPC_GET_UTXO_STATS::request req{};
    req.asset_id = custom_asset_id;
    tools::wallet_public::COMMAND_RPC_GET_UTXO_STATS::response res{};
    bool ok = alice_rpc.on_get_utxo_stats(req, res, je, ctx);
    CHECK_AND_ASSERT_MES(ok, false, "on_get_utxo_stats (asset) failed: " << je.message);
    CHECK_AND_ASSERT_MES(res.asset_id == custom_asset_id, false, "asset_id echoed back must match request");
    // 5 GUS (5e6) lands in [10^6..10^7-1]; 80 GUS (8e7) lands in [10^7..10^8-1]
    CHECK_AND_ASSERT_MES(res.buckets.size() == 2, false, "expected 2 asset buckets, got " << res.buckets.size());
    for (const auto& b : res.buckets)
    {
      if (b.lower_bound == 1000000ull) // 10^6
      {
        CHECK_AND_ASSERT_MES(b.total_utxo == 1 && b.total_amount == alice_asset_small, false, "asset bucket [1e6..) wrong");
      }
      else if (b.lower_bound == 10000000ull) // 10^7
      {
        CHECK_AND_ASSERT_MES(b.total_utxo == 1 && b.total_amount == alice_asset_big, false, "asset bucket [1e7..) wrong");
      }
      else
      {
        CHECK_AND_ASSERT_MES(false, false, "unexpected asset bucket lower_bound=" << b.lower_bound);
      }
    }
  }

  // get_utxo_stats: unknown asset -> empty bucket list, no error
  {
    tools::wallet_public::COMMAND_RPC_GET_UTXO_STATS::request req{};
    crypto::secret_key dummy_sk{};
    crypto::generate_keys(req.asset_id, dummy_sk);
    tools::wallet_public::COMMAND_RPC_GET_UTXO_STATS::response res{};
    bool ok = alice_rpc.on_get_utxo_stats(req, res, je, ctx);
    CHECK_AND_ASSERT_MES(ok, false, "on_get_utxo_stats(random asset) failed: " << je.message);
    CHECK_AND_ASSERT_MES(res.buckets.empty(), false, "random asset must yield 0 buckets, got " << res.buckets.size());
  }

  // empty wallet (Bob, never received anything): both RPCs must succeed and return empty
  {
    tools::wallet_rpc_server bob_rpc(bob_wlt);
    bob_wlt->refresh();

    tools::wallet_public::COMMAND_RPC_GET_OUTPUTS::request go_req{};
    tools::wallet_public::COMMAND_RPC_GET_OUTPUTS::response go_res{};
    bool ok = bob_rpc.on_get_outputs(go_req, go_res, je, ctx);
    CHECK_AND_ASSERT_MES(ok, false, "Bob on_get_outputs failed: " << je.message);
    CHECK_AND_ASSERT_MES(go_res.outputs.empty(), false, "Bob must have no outputs, got " << go_res.outputs.size());

    tools::wallet_public::COMMAND_RPC_GET_UTXO_STATS::request gs_req{};
    tools::wallet_public::COMMAND_RPC_GET_UTXO_STATS::response gs_res{};
    ok = bob_rpc.on_get_utxo_stats(gs_req, gs_res, je, ctx);
    CHECK_AND_ASSERT_MES(ok, false, "Bob on_get_utxo_stats failed: " << je.message);
    CHECK_AND_ASSERT_MES(gs_res.buckets.empty(), false, "Bob must have no buckets, got " << gs_res.buckets.size());
    CHECK_AND_ASSERT_MES(gs_res.asset_id == currency::native_coin_asset_id, false, "Bob default asset_id must be native");
  }

  // spend one of Alices native UTXOs, then verify spent/unspent partitioning
  alice_wlt->set_concise_mode(false);

  const uint64_t outgoing_native = MK_TEST_COINS(1); // amount + fee == 2 any of Alices UTXOs is bigger
  alice_wlt->transfer(outgoing_native, bob_wlt->get_account().get_public_address(), currency::native_coin_asset_id);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "mine after spend failed");
  alice_wlt->refresh();

  // after the spend: at least one Alice native input is now spent
  uint64_t spent_count = 0, unspent_count = 0;
  uint64_t spent_block_height = 0;
  {
    tools::wallet_public::COMMAND_RPC_GET_OUTPUTS::request req{};
    tools::wallet_public::COMMAND_RPC_GET_OUTPUTS::response res{};
    bool ok = alice_rpc.on_get_outputs(req, res, je, ctx);
    CHECK_AND_ASSERT_MES(ok, false, "post-spend get_outputs failed: " << je.message);
    for (const auto& od : res.outputs)
    {
      if (od.spent)
      {
        ++spent_count;
        CHECK_AND_ASSERT_MES(od.spent_height != 0, false, "spent output must have spent_height != 0, out_id=" << od.out_id);
        CHECK_AND_ASSERT_MES((od.flags & WALLET_TRANSFER_DETAIL_FLAG_SPENT) != 0, false, "spent flag must be set, out_id=" << od.out_id);
        CHECK_AND_ASSERT_MES(!od.spendable, false, "spent output must not be spendable, out_id=" << od.out_id);
        spent_block_height = od.spent_height;
      }
      else
      {
        ++unspent_count;
      }
    }
    CHECK_AND_ASSERT_MES(spent_count >= 1, false, "expected at least 1 spent output after the transfer");
    CHECK_AND_ASSERT_MES(unspent_count >= 1, false, "expected change/leftover unspent outputs");
  }

  // 'unspent' filter must match the previous unspent count
  {
    tools::wallet_public::COMMAND_RPC_GET_OUTPUTS::request req{};
    req.output_type = "unspent";
    tools::wallet_public::COMMAND_RPC_GET_OUTPUTS::response res{};
    bool ok = alice_rpc.on_get_outputs(req, res, je, ctx);
    CHECK_AND_ASSERT_MES(ok, false, "post-spend get_outputs(unspent) failed: " << je.message);
    CHECK_AND_ASSERT_MES(res.outputs.size() == unspent_count, false, "unspent size mismatch: " << res.outputs.size() << " vs " << unspent_count);
    for (const auto& od : res.outputs)
      CHECK_AND_ASSERT_MES(!od.spent, false, "unspent filter leaked a spent output");
  }

  // 'spent' filter must match the spent count and report sane fields
  {
    tools::wallet_public::COMMAND_RPC_GET_OUTPUTS::request req{};
    req.output_type = "spent";
    tools::wallet_public::COMMAND_RPC_GET_OUTPUTS::response res{};
    bool ok = alice_rpc.on_get_outputs(req, res, je, ctx);
    CHECK_AND_ASSERT_MES(ok, false, "post-spend get_outputs(spent) failed: " << je.message);
    CHECK_AND_ASSERT_MES(res.outputs.size() == spent_count, false, "spent size mismatch: " << res.outputs.size() << " vs " << spent_count);
    for (const auto& od : res.outputs)
    {
      CHECK_AND_ASSERT_MES(od.spent, false, "spent filter leaked an unspent output");
      CHECK_AND_ASSERT_MES(od.spent_height == spent_block_height, false, "spent_height mismatch across spent outputs (we did 1 spend tx)");
    }
  }

  // utxo_stats must only count unspent native outputs after the spend
  {
    tools::wallet_public::COMMAND_RPC_GET_UTXO_STATS::request req{};
    tools::wallet_public::COMMAND_RPC_GET_UTXO_STATS::response res{};
    bool ok = alice_rpc.on_get_utxo_stats(req, res, je, ctx);
    CHECK_AND_ASSERT_MES(ok, false, "post-spend get_utxo_stats failed: " << je.message);
    uint64_t utxo_total = 0, amount_total = 0;
    for (const auto& b : res.buckets)
    {
      utxo_total   += b.total_utxo;
      amount_total += b.total_amount;
    }
    CHECK_AND_ASSERT_MES(utxo_total == unspent_count - 2, false, // -2 because asset outputs aren't native
      "post-spend utxo_total=" << utxo_total << ", expected " << (unspent_count - 2));
    CHECK_AND_ASSERT_MES(amount_total == alice_wlt->balance(currency::native_coin_asset_id), false,
      "post-spend amount_total mismatch");
  }

  return true;
}
//------------------------------------------------------------------------------

wallet_rpc_sign_message_with_alias::wallet_rpc_sign_message_with_alias()
{
  REGISTER_CALLBACK_METHOD(wallet_rpc_sign_message_with_alias, configure_core);
  REGISTER_CALLBACK_METHOD(wallet_rpc_sign_message_with_alias, c1);

  m_hardforks.set_hardfork_height(1, 1);
  m_hardforks.set_hardfork_height(2, 1);
  m_hardforks.set_hardfork_height(3, 1);
  m_hardforks.set_hardfork_height(4, 1);
}

bool wallet_rpc_sign_message_with_alias::generate(std::vector<test_event_entry>& events) const
{
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  set_hard_fork_heights_to_generator(generator);
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 6);

  DO_CALLBACK(events, "c1");
  return true;
}

bool wallet_rpc_sign_message_with_alias::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // covers the alias-based path of core RPC 'validate_signature'
  // The unit test in tests/unit_tests/wallet_misc_tests.cpp covers everything but the alias resolution, since aliases require a live blockchain.
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt   = init_playtime_test_wallet(events, c, BOB_ACC_IDX);

  miner_wlt->refresh();

  miner_wlt->transfer(MK_TEST_COINS(10), alice_wlt->get_account().get_public_address(), currency::native_coin_asset_id);
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "mine after funding Alice failed");
  alice_wlt->refresh();

  // miner registers an alias pointing at Alice's address
  const std::string ALIAS_NAME = "alicepub";
  {
    tools::wallet_public::COMMAND_RPC_REGISTER_ALIAS::request req{};
    tools::wallet_public::COMMAND_RPC_REGISTER_ALIAS::response rsp{};
    req.al.alias = ALIAS_NAME;
    req.al.details.address = alice_wlt->get_account().get_public_address_str();
    req.al.details.comment = "Alice";
    r = invoke_text_json_for_wallet(miner_wlt, "register_alias", req, rsp);
    CHECK_AND_ASSERT_MES(r, false, "register_alias failed");
  }
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 3);
  CHECK_AND_ASSERT_MES(r, false, "mine after register_alias failed");

  {
    currency::COMMAND_RPC_GET_ALIAS_DETAILS::request req{};
    currency::COMMAND_RPC_GET_ALIAS_DETAILS::response rsp{};
    req.alias = ALIAS_NAME;
    r = invoke_text_json_for_core(c, "get_alias_details", req, rsp);
    CHECK_AND_ASSERT_MES(r, false, "get_alias_details failed");
    CHECK_AND_ASSERT_MES(rsp.status == API_RETURN_CODE_OK, false, "get_alias_details status=" << rsp.status);
    CHECK_AND_ASSERT_MES(rsp.alias_details.address == alice_wlt->get_account().get_public_address_str(), false,
      "alias points at " << rsp.alias_details.address << ", expected Alice's address");
  }

  // Alice signs a payload mix of binary / non-ASCII / NUL bytes to verify raw-buffer handling
  std::string payload(150, '\0');
  crypto::generate_random_bytes(payload.size(), &payload[0]);

  crypto::signature alice_sig{};
  crypto::public_key alice_pkey{};
  {
    tools::wallet_rpc_server alice_rpc(alice_wlt);
    tools::wallet_public::COMMAND_SIGN_MESSAGE::request  req{};
    tools::wallet_public::COMMAND_SIGN_MESSAGE::response rsp{};
    epee::json_rpc::error je{};
    tools::wallet_rpc_server::connection_context cntx{};
    req.buff = payload;
    bool ok = alice_rpc.on_sign_message(req, rsp, je, cntx);
    CHECK_AND_ASSERT_MES(ok, false, "sign_message failed: " << je.message);
    CHECK_AND_ASSERT_MES(rsp.pkey == alice_wlt->get_account().get_public_address().spend_public_key, false,
      "sign_message returned pkey that doesn't match Alice's spend pkey");
    alice_sig = rsp.sig;
    alice_pkey = rsp.pkey;
  }

  // happy path: alias resolves to Alices spend pkey, v2 signature validates
  {
    COMMAND_VALIDATE_SIGNATURE::request req{};
    COMMAND_VALIDATE_SIGNATURE::response rsp{};
    req.buff = payload;
    req.sig = alice_sig;
    req.alias = ALIAS_NAME;
    r = invoke_text_json_for_core(c, "validate_signature", req, rsp);
    CHECK_AND_ASSERT_MES(r, false, "validate_signature (alias) call failed");
    CHECK_AND_ASSERT_MES(rsp.status == API_RETURN_CODE_OK, false, "validate_signature(alias) status=" << rsp.status);
    CHECK_AND_ASSERT_MES(rsp.sig_format == "v2", false, "sig_format=" << rsp.sig_format << ", expected v2");
  }

  // unknown alias -> failure, no pkey to look up
  {
    COMMAND_VALIDATE_SIGNATURE::request req{};
    COMMAND_VALIDATE_SIGNATURE::response rsp{};
    req.buff = payload;
    req.sig = alice_sig;
    req.alias = "noexistalias";
    r = invoke_text_json_for_core(c, "validate_signature", req, rsp);
    CHECK_AND_ASSERT_MES(r, false, "validate_signature (unknown alias) call failed");
    CHECK_AND_ASSERT_MES(rsp.status != API_RETURN_CODE_OK, false, "validate_signature(unknown alias) accepted: " << rsp.status);
    CHECK_AND_ASSERT_MES(rsp.sig_format.empty(), false, "sig_format=" << rsp.sig_format << ", expected empty on failure");
  }

  // wrong-data via alias path: tamper the payload, signature must no longer validate
  {
    COMMAND_VALIDATE_SIGNATURE::request req{};
    COMMAND_VALIDATE_SIGNATURE::response rsp{};
    req.buff = payload;
    req.buff[0] ^= 0x01;
    req.sig = alice_sig;
    req.alias = ALIAS_NAME;
    r = invoke_text_json_for_core(c, "validate_signature", req, rsp);
    CHECK_AND_ASSERT_MES(r, false, "validate_signature (tampered) call failed");
    CHECK_AND_ASSERT_MES(rsp.status != API_RETURN_CODE_OK, false, "tampered payload accepted via alias path");
    CHECK_AND_ASSERT_MES(rsp.sig_format.empty(), false, "tampered path sig_format=" << rsp.sig_format);
  }

  // alias points at Alice but signature is from Bob -> must reject
  {
    crypto::signature bob_sig{};
    bob_wlt->sign_buffer(payload, bob_sig);

    COMMAND_VALIDATE_SIGNATURE::request req{};
    COMMAND_VALIDATE_SIGNATURE::response rsp{};
    req.buff = payload;
    req.sig = bob_sig;
    req.alias = ALIAS_NAME;
    r = invoke_text_json_for_core(c, "validate_signature", req, rsp);
    CHECK_AND_ASSERT_MES(r, false, "validate_signature (alias-from-other-wallet) call failed");
    CHECK_AND_ASSERT_MES(rsp.status != API_RETURN_CODE_OK, false, "Bob's signature accepted as Alice via alias");
    CHECK_AND_ASSERT_MES(rsp.sig_format.empty(), false, "wrong-signer sig_format=" << rsp.sig_format);
  }

  // legacy signature format (pre-HDS) must validate via alias as well, reporting sig_format="legacy"
  {
    crypto::signature legacy_sig{};
    crypto::hash h = crypto::cn_fast_hash(payload.data(), payload.size());
    crypto::generate_signature(h, alice_pkey, m_accounts[ALICE_ACC_IDX].get_keys().spend_secret_key, legacy_sig);

    COMMAND_VALIDATE_SIGNATURE::request req{};
    COMMAND_VALIDATE_SIGNATURE::response rsp{};
    req.buff = payload;
    req.sig = legacy_sig;
    req.alias = ALIAS_NAME;
    r = invoke_text_json_for_core(c, "validate_signature", req, rsp);
    CHECK_AND_ASSERT_MES(r, false, "validate_signature (legacy via alias) call failed");
    CHECK_AND_ASSERT_MES(rsp.status == API_RETURN_CODE_OK, false, "legacy signature via alias rejected: " << rsp.status);
    CHECK_AND_ASSERT_MES(rsp.sig_format == "legacy", false, "legacy sig_format=" << rsp.sig_format << ", expected 'legacy'");
  }

  // both pkey and alias provided: per implementation, the explicit pkey wins and alias is not consulted
  // verify by setting alias to a value that would otherwise fail (unknown), and pkey to the correct one
  {
    COMMAND_VALIDATE_SIGNATURE::request req{};
    COMMAND_VALIDATE_SIGNATURE::response rsp{};
    req.buff = payload;
    req.sig = alice_sig;
    req.pkey = alice_pkey;
    req.alias = "noexistalias";
    r = invoke_text_json_for_core(c, "validate_signature", req, rsp);
    CHECK_AND_ASSERT_MES(r, false, "validate_signature (pkey+bad alias) call failed");
    CHECK_AND_ASSERT_MES(rsp.status == API_RETURN_CODE_OK, false, "explicit pkey did not override alias lookup: " << rsp.status);
    CHECK_AND_ASSERT_MES(rsp.sig_format == "v2", false, "sig_format=" << rsp.sig_format);
  }

  // after update_alias -> Bob, validating Alice's signature via the same alias must fail
  // (the alias now resolves to Bob's pkey), while Bob's signature via the alias must succeed.
  {
    alice_wlt->refresh();
    tools::wallet_public::COMMAND_RPC_UPDATE_ALIAS::request req{};
    tools::wallet_public::COMMAND_RPC_UPDATE_ALIAS::response rsp{};
    req.al.alias = ALIAS_NAME;
    req.al.details.address = bob_wlt->get_account().get_public_address_str();
    req.al.details.comment = "Bob now";
    r = invoke_text_json_for_wallet(alice_wlt, "update_alias", req, rsp);
    CHECK_AND_ASSERT_MES(r, false, "update_alias failed");
  }
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 3);
  CHECK_AND_ASSERT_MES(r, false, "mine after update_alias failed");

  // Alices signature should no longer validate under ALIAS_NAME (alias now resolves to Bobs pkey)
  {
    COMMAND_VALIDATE_SIGNATURE::request req{};
    COMMAND_VALIDATE_SIGNATURE::response rsp{};
    req.buff = payload;
    req.sig = alice_sig;
    req.alias = ALIAS_NAME;
    r = invoke_text_json_for_core(c, "validate_signature", req, rsp);
    CHECK_AND_ASSERT_MES(r, false, "validate_signature post-update (Alice) call failed");
    CHECK_AND_ASSERT_MES(rsp.status != API_RETURN_CODE_OK, false, "Alice's sig still accepted after alias re-pointed to Bob");
    CHECK_AND_ASSERT_MES(rsp.sig_format.empty(), false, "post-update sig_format=" << rsp.sig_format);
  }

  // Bobs signature under the same alias should validate
  {
    crypto::signature bob_sig{};
    bob_wlt->sign_buffer(payload, bob_sig);

    COMMAND_VALIDATE_SIGNATURE::request req{};
    COMMAND_VALIDATE_SIGNATURE::response rsp{};
    req.buff = payload;
    req.sig = bob_sig;
    req.alias = ALIAS_NAME;
    r = invoke_text_json_for_core(c, "validate_signature", req, rsp);
    CHECK_AND_ASSERT_MES(r, false, "validate_signature post-update (Bob) call failed");
    CHECK_AND_ASSERT_MES(rsp.status == API_RETURN_CODE_OK, false, "Bob's sig rejected via re-pointed alias: " << rsp.status);
    CHECK_AND_ASSERT_MES(rsp.sig_format == "v2", false, "Bob's sig_format=" << rsp.sig_format << ", expected v2");
  }

  return true;
}

