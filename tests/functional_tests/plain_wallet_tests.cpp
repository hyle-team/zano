// Copyright (c) 2014-2018 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/dll.hpp>
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


struct try_pull_result_open_response
{
  bool delivered;
  epee::json_rpc::response<view::open_wallet_response, epee::json_rpc::dummy_error> result;
  BEGIN_KV_SERIALIZE_MAP()
    KV_SERIALIZE(delivered)
    KV_SERIALIZE(result)
  END_KV_SERIALIZE_MAP()
};


void run_plain_wallet_api_test()
{
  LOG_PRINT_L0("Creating instance...");

  //plain_wallet::set_bundle_working_dir("E:\\tmp\\check_export");
  std::string s = plain_wallet::init("195.201.107.230", "33333", boost::dll::program_location().parent_path().string(), 1);
  //s = plain_wallet::get_export_private_info("E:\\tmp\\check_export");


  std::string res = plain_wallet::sync_call("get_seed_phrase_info", 0, "{\"seed_phrase\":\"aZxat4HAWriVQ3enkGcVsrZRdMseAJswG3CSEwTqZS246VsFQ53w26eZstYsu1jWE74Atz9ajLxFnBsVTafncWNH5SMv4zHFaTS:1780c4d5dd7e97cc4a75ea8baa7977d12ef948b9a6dddc2a9a37e5e22ac7180e:1599495055\"}");


//  res = plain_wallet::restore("footstep knowledge fur capture honey minute carefully peaceful lovely crawl lunch government nightmare friendship myself sign possibly plan flower depression bread rainbow wrong hardly dark chest",
//    "test_wall2.zan", "111", "");

//  epee::misc_utils::sleep_no_w(30000);

//  plain_wallet::close_wallet(0);
  res = plain_wallet::open("test_wall2.zan", "111");
  res = plain_wallet::close_wallet(0);


  res = plain_wallet::invoke(0, "{\"method\":\"transfer\",\"params\":{\"destinations\":[{\"amount\":10000000000,\"address\":\"aZxat4HAWriVQ3enkGcVsrZRdMseAJswG3CSEwTqZS246VsFQ53w26eZstYsu1jWE74Atz9ajLxFnBsVTafncWNH5SMv4zHFaTS\"}],\"fee\":10000000000,\"mixin\":1011111,\"payment_id\":\"\",\"push_payer\":true,\"hide_receiver\":false}}");

  //epee::misc_utils::sleep_no_w(10000000);

  //std::string key = plain_wallet::generate_random_key(10);
  //std::string test_data = "1234567890 test test ";
  //std::string res = plain_wallet::set_appconfig(test_data, key);
  //std::string test_data2 =  plain_wallet::get_appconfig(key);
  //if (test_data2 != test_data)
  //{
  //  LOG_ERROR("Error");
  //}
  return;
  //std::string fres = plain_wallet::get_logs_buffer();
  //std::string fres2 = plain_wallet::truncate_log();
  //std::string fres3 = plain_wallet::get_logs_buffer();



//   LOG_PRINT_L0("Generating wallet...");
//   view::open_wallet_request owr = AUTO_VAL_INIT(owr);
//   owr.path = "E:\\tmp\\zano_testwallet_745ss65030.zan";
//   owr.pass = "";
//   std::string job_id_str = plain_wallet::async_call("open", 0, epee::serialization::store_t_to_json(owr));
// 
// 
//   try_pull_result_open_response rsp = AUTO_VAL_INIT(rsp);
// 
//   while (true)
//   {
//     std::string res = plain_wallet::try_pull_result(1);
//     LOG_PRINT_L0("[try_pull_result] RESPONSE:" << ENDL << res);
// 
//     if (!epee::serialization::load_t_from_json(rsp, res))
//     {
//       LOG_ERROR("Failed to parse try_pull_result response: " << res);
//       return;
//     }
//     epee::misc_utils::sleep_no_w(1000);
//     if(!rsp.delivered)
//       continue;
//     break;
//   }
// 
// 

  //std::string rsp = plain_wallet::open(std::string("E:\\tmp\\zano_testwallet_745ss65030.zan"), "");
  //LOG_PRINT_L0("RESPONSE:" << ENDL << rsp);
  //epee::json_rpc::response<view::open_wallet_response, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
  //epee::serialization::load_t_from_json(ok_response, rsp);

//   size_t count = 0;
//   while (count < 10)
//   {
//     std::string prog = plain_wallet::get_wallet_status(rsp.result.result.wallet_id);
//     LOG_PRINT_L0("Progress: " << ENDL << prog);
//     view::wallet_sync_status_info wsi = AUTO_VAL_INIT(wsi);
//     if (!epee::serialization::load_t_from_json(wsi, prog))
//     {
//       LOG_ERROR("Failed to get_wallet_status()");
//       return;
//     }
//     if (!wsi.is_in_long_refresh)
//       break;
//     epee::misc_utils::sleep_no_w(1000);
//   }
// 
//   std::string job_id_str2 = plain_wallet::async_call("close", rsp.result.result.wallet_id, "");
//   try_pull_result_open_response rsp2 = AUTO_VAL_INIT(rsp2);
// 
//   while (true)
//   {
//     std::string res = plain_wallet::try_pull_result(2);
//     LOG_PRINT_L0("[try_pull_result] RESPONSE:" << ENDL << res);
// 
//     if (!epee::serialization::load_t_from_json(rsp2, res))
//     {
//       LOG_ERROR("Failed to parse try_pull_result response: " << res);
//       return;
//     }
//     epee::misc_utils::sleep_no_w(1000);
//     if (!rsp2.delivered)
//       continue;
//     break;
//   }
// 
// 
//   LOG_PRINT_L0("OK");

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