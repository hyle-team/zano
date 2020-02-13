// Copyright (c) 2014-2018 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

#include "include_base_utils.h"
using namespace epee;
#include "misc_language.h"
#include "wallet/plain_wallet_api.h"
#include "wallet/wallet_helpers.h"
#include "wallet/plain_wallet_api_defs.h"


void run_plain_wallet_api_test()
{
  LOG_PRINT_L0("Creating instance...");
  std::string s = plain_wallet::init("195.201.107.230", "11211");

  LOG_PRINT_L0("Generating wallet...");
  std::string rsp = plain_wallet::open(std::string("E:\\tmp\\zano_testwallet_745ss65030.zan"), "");
  LOG_PRINT_L0("RESPONSE:" << ENDL << rsp);
  epee::json_rpc::response<view::open_wallet_response, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
  epee::serialization::load_t_from_json(ok_response, rsp);


  while (true)
  {
    std::string prog = plain_wallet::get_wallet_status(ok_response.result.wallet_id);
    LOG_PRINT_L0("Progress: " << ENDL << prog);
    //     view::sta ssr = AUTO_VAL_INIT(ssr);
    //     epee::serialization::load_t_from_json(ssr, prog);
    //     LOG_PRINT_L0("Progress: " << ssr.progress << "Finished: " << ssr.finished);
    //     if (ssr.finished)
    //       break;
    epee::misc_utils::sleep_no_w(1000);
  }
}

//   LOG_PRINT_L0("Creating instance..." << std::hex << hw);
// 
//   LOG_PRINT_L0("Generating wallet...");
//   std::string rsp = plain_wallet::open(hw, std::string("E:\\tmp\\zano_testwallet_74565030.zan"), "");
//   LOG_PRINT_L0("RESPONSE:" << ENDL << rsp);
//   epee::json_rpc::response<plain_wallet::open_wallet_response, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
//   epee::serialization::load_t_from_json(ok_response, rsp);
// 
//   plain_wallet::start_sync_thread(hw);
//   LOG_PRINT_L0("Started sync thread.");
// 
//   while (true)
//   {
//     std::string prog = plain_wallet::get_sync_status(hw);
//     plain_wallet::sync_status_response ssr = AUTO_VAL_INIT(ssr);
//     epee::serialization::load_t_from_json(ssr, prog);
//     LOG_PRINT_L0("Progress: " << ssr.progress << "Finished: " << ssr.finished);
//     if (ssr.finished)
//       break;
//     epee::misc_utils::sleep_no_w(1000);
//   }
//   LOG_PRINT_L0("Sync finished OK");
// 
//   {
//     //request get wallet info: 
//     epee::json_rpc::request<tools::wallet_public::COMMAND_RPC_GET_WALLET_INFO::request> gbreq = AUTO_VAL_INIT(gbreq);
//     gbreq.method = "get_wallet_info";
//     epee::json_rpc::response<tools::wallet_public::COMMAND_RPC_GET_WALLET_INFO::response, epee::json_rpc::error> gbres = AUTO_VAL_INIT(gbres);
//     std::string req_str = epee::serialization::store_t_to_json(gbreq);
// 
//     std::string res = plain_wallet::invoke(hw, req_str);
//     epee::serialization::load_t_from_json(gbres, res);
// 
//     LOG_PRINT_L0("Balance request returned: code [" << gbres.error.code << "], str_response: "
//       << ENDL << res);
//   }
// 
//   {
//     //request balance
//     epee::json_rpc::request<tools::wallet_public::COMMAND_RPC_GET_BALANCE::request> gbreq = AUTO_VAL_INIT(gbreq);
//     gbreq.method = "getbalance";
//     epee::json_rpc::response<tools::wallet_public::COMMAND_RPC_GET_BALANCE::response, epee::json_rpc::error> gbres = AUTO_VAL_INIT(gbres);
//     std::string req_str = epee::serialization::store_t_to_json(gbreq);
// 
//     std::string res = plain_wallet::invoke(hw, req_str);
//     epee::serialization::load_t_from_json(gbres, res);
// 
//     LOG_PRINT_L0("Balance request returned: code [" << gbres.error.code << "], balance: "
//       << gbres.result.balance << ", unlocked_balance: " << gbres.result.unlocked_balance);
//   }
// 
//   {
//     //request balance
//     epee::json_rpc::request<tools::wallet_public::COMMAND_RPC_STORE::request> gbreq = AUTO_VAL_INIT(gbreq);
//     gbreq.method = "store";
//     epee::json_rpc::response<tools::wallet_public::COMMAND_RPC_STORE::response, epee::json_rpc::error> gbres = AUTO_VAL_INIT(gbres);
//     std::string req_str = epee::serialization::store_t_to_json(gbreq);
// 
//     std::string res = plain_wallet::invoke(hw, req_str);
//     epee::serialization::load_t_from_json(gbres, res);
// 
//     LOG_PRINT_L0("Balance request returned: code [" << gbres.error.code << "], str_response: "
//       << ENDL << res); 
//   }
// 
//   plain_wallet::destroy_instance(hw);
// 
// 
//   return;
//   //-------
//   {
//     LOG_PRINT_L0("Creating instance...");
//     plain_wallet::hwallet hw = plain_wallet::create_instance("127.0.0.1", "11211");
//     LOG_PRINT_L0("Creating instance..." << std::hex << hw);
// 
//     LOG_PRINT_L0("Generating wallet...");
//     std::string rsp = plain_wallet::generate(hw, std::string("E:\\tmp\\zano_testwallet_") + std::to_string(epee::misc_utils::get_tick_count()) + ".zan", "");
//     LOG_PRINT_L0("RESPONSE:" << ENDL << rsp);
//     epee::json_rpc::response<plain_wallet::open_wallet_response, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
//     epee::serialization::load_t_from_json(ok_response, rsp);
// 
//     plain_wallet::start_sync_thread(hw);
//     LOG_PRINT_L0("Started sync thread.");
// 
//     while (true)
//     {
//       std::string prog = plain_wallet::get_sync_status(hw);
//       plain_wallet::sync_status_response ssr = AUTO_VAL_INIT(ssr);
//       epee::serialization::load_t_from_json(ssr, prog);
//       LOG_PRINT_L0("Progress: " << ssr.progress << "Finished: " << ssr.finished);
//       if (ssr.finished)
//         break;
//       epee::misc_utils::sleep_no_w(1000);
//     }
//     LOG_PRINT_L0("Sync finished OK");
// 
//     {
//       //request get wallet info: 
//       epee::json_rpc::request<tools::wallet_public::COMMAND_RPC_GET_WALLET_INFO::request> gbreq = AUTO_VAL_INIT(gbreq);
//       gbreq.method = "get_wallet_info";
//       epee::json_rpc::response<tools::wallet_public::COMMAND_RPC_GET_WALLET_INFO::response, epee::json_rpc::error> gbres = AUTO_VAL_INIT(gbres);
//       std::string req_str = epee::serialization::store_t_to_json(gbreq);
// 
//       std::string res = plain_wallet::invoke(hw, req_str);
//       epee::serialization::load_t_from_json(gbres, res);
// 
//       LOG_PRINT_L0("Balance request returned: code [" << gbres.error.code << "], str_response: "
//         << ENDL << res);
//     }
// 
//     {
//       //request balance
//       epee::json_rpc::request<tools::wallet_public::COMMAND_RPC_GET_BALANCE::request> gbreq = AUTO_VAL_INIT(gbreq);
//       gbreq.method = "getbalance";
//       epee::json_rpc::response<tools::wallet_public::COMMAND_RPC_GET_BALANCE::response, epee::json_rpc::error> gbres = AUTO_VAL_INIT(gbres);
//       std::string req_str = epee::serialization::store_t_to_json(gbreq);
// 
//       std::string res = plain_wallet::invoke(hw, req_str);
//       epee::serialization::load_t_from_json(gbres, res);
// 
//       LOG_PRINT_L0("Balance request returned: code [" << gbres.error.code << "], balance: "
//         << gbres.result.balance << ", unlocked_balance: " << gbres.result.unlocked_balance);
//     }
// 
//     {
//       //request balance
//       epee::json_rpc::request<tools::wallet_public::COMMAND_RPC_STORE::request> gbreq = AUTO_VAL_INIT(gbreq);
//       gbreq.method = "store";
//       epee::json_rpc::response<tools::wallet_public::COMMAND_RPC_STORE::response, epee::json_rpc::error> gbres = AUTO_VAL_INIT(gbres);
//       std::string req_str = epee::serialization::store_t_to_json(gbreq);
// 
//       std::string res = plain_wallet::invoke(hw, req_str);
//       epee::serialization::load_t_from_json(gbres, res);
// 
//       LOG_PRINT_L0("Balance request returned: code [" << gbres.error.code << "], str_response: "
//         << ENDL << res);
//     }
//  }}