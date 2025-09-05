// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "performance_tests.h"
#include "performance_utils.h"
#include <mutex>

// tests
#include "construct_tx.h"
#include "check_ring_signature.h"
#include "derive_public_key.h"
#include "derive_secret_key.h"
#include "generate_key_derivation.h"
#include "generate_key_image.h"
#include "generate_key_image_helper.h"
#include "is_out_to_acc.h"
#include "core_market_performance_test.h"
#include "serialization_performance_test.h"
#include "chacha_stream_performance_test.h"
#include "keccak_test.h"
#include "blake2_test.h"
#include "print_struct_to_json.h"
#include "free_space_check.h"
#include "htlc_hash_tests.h"
#include "threads_pool_tests.h"
#include "wallet/plain_wallet_api.h"  
#include "wallet/view_iface.h"
#include "wallet/plain_wallet_api_defs.h"
#include "math_helper.h"
  
PUSH_VS_WARNINGS
DISABLE_VS_WARNINGS(4244)
#include "jwt-cpp/jwt.h"
POP_VS_WARNINGS



void test_base64_serialization()
{
  currency::COMMAND_RPC_GET_POOL_TXS_DETAILS::response rsp;
  rsp.txs.resize(1);
  rsp.txs.back().blob.resize(4000);
  crypto::generate_random_bytes(rsp.txs.back().blob.size(), (void*)rsp.txs.back().blob.data());

  std::string str_json;
  epee::serialization::store_t_to_json(rsp, str_json);

  currency::COMMAND_RPC_GET_POOL_TXS_DETAILS::response rsp2;
  epee::serialization::load_t_from_json(rsp2, str_json);
  if (rsp.txs.back().blob != rsp2.txs.back().blob)
  {
    LOG_PRINT_L0("Troubles");
  }
    
}


void test_tx_json_serialization()
{
  currency::transaction tx{};

  tx.version = CURRENT_TRANSACTION_VERSION;
  currency::asset_descriptor_operation ado{};
  ado.version = ASSET_DESCRIPTOR_OPERATION_LAST_VER;
  ado.opt_asset_id = currency::null_pkey;
  currency::asset_descriptor_base adb{};
  ado.opt_descriptor = adb;
  tx.extra.push_back(ado);
  std::string json_tx = currency::obj_to_json_str(tx);
}

void test_plain_wallet()
{
  //std::string res = plain_wallet::init("195.201.107.230", "33340", "C:\\Users\\roky\\home\\", 0);
  //std::string res = plain_wallet::init("", "", "C:\\Users\\roky\\home\\", 0);
  // std::string res = plain_wallet::init("https://195.201.107.230", "443", "C:\\git_repos\\zano\\build_msvc2022_64\\src\\Debug\\", LOG_LEVEL_2);
  std::string res = plain_wallet::init("https://node.zano.org", "443", "C:\\git_repos\\zano\\build_msvc2022_64\\src\\Debug\\", LOG_LEVEL_2);
  //std::string res = plain_wallet::init("127.0.0.1", "12111", "C:\\Users\\roky\\home22\\", 0);
  
  plain_wallet::configure_object conf = AUTO_VAL_INIT(conf);
  //plain_wallet::configure_response conf_resp = AUTO_VAL_INIT(conf_resp);
  //conf.postponed_run_wallet = true;
  std::string r = plain_wallet::sync_call("configure", 0, epee::serialization::store_t_to_json(conf));
  


  //-------------------------------------------------------------------
//test proxy_to_wallet
  std::string daemon_body = "{\"method\":\"getinfo\",\"params\":{}}";
  std::string daemon_body_base64 = epee::string_encoding::base64_encode(daemon_body);
  tools::wallet_public::COMMAND_PROXY_TO_DAEMON::request req_to_daemon;
  req_to_daemon.uri = "/json_rpc";
  req_to_daemon.base64_body = daemon_body_base64;
  std::string rsp_ = plain_wallet::sync_call("proxy_to_daemon", 0, epee::serialization::store_t_to_json(req_to_daemon));
  //-------------------------------------------------------------------




  //std::string seed;
  //if (!epee::file_io_utils::load_file_to_string("C:\\Users\\roky\\home\\temp\\wallets\\seed.txt", seed))
  //  return;

  //std::string res___ = plain_wallet::get_wallet_files();


  uint64_t instance_id = 0;
  res = plain_wallet::open("test_restored_3.zan", "111");
  //res = plain_wallet::restore(seed,
  //  "test_restored_3.zan", "111", "test");

  epee::misc_utils::sleep_no_w(2000);

  //res = plain_wallet::sync_call("reset_connection_url", 0, "195.201.107.230:33336");
  //res = plain_wallet::sync_call("reset_connection_url", 0, "https://node.zano.org:443");
  //res = plain_wallet::sync_call("reset_connection_url", 0, "https://zano.cakewallet.com");
  //res = plain_wallet::sync_call("reset_connection_url", 0, "https://zano.api.wombat.systems:443");
  //res = plain_wallet::sync_call("reset_connection_url", 0, "http://127.0.0.1:11211");

  
  //r = plain_wallet::sync_call("run_wallet", instance_id, "");






  while(true)
  {
    epee::misc_utils::sleep_no_w(2000);
    res = plain_wallet::sync_call("get_wallet_status", instance_id, "");
    view::wallet_sync_status_info wsi = AUTO_VAL_INIT(wsi);
    epee::serialization::load_t_from_json(wsi, res);
    LOG_PRINT_L0("Progress: "  << wsi.progress << " state: " << wsi.wallet_state << "is_in_long_refresh: " << wsi.is_in_long_refresh << ", height: " << wsi.current_wallet_height << " of " << wsi.current_daemon_height);
    if (wsi.wallet_state == 2)
      break;
  }
  


  std::string invoke_body = "{\"method\":\"store\",\"params\":{}}";
  std::string res1 = plain_wallet::sync_call("invoke", instance_id, invoke_body);

  LOG_PRINT_L0("Store: " << res1);
  return;
  {
    invoke_body = "{\"method\":\"getbalance\",\"params\":{}}";
    std::string res3 = plain_wallet::sync_call("invoke", instance_id, invoke_body);
    invoke_body = "";
  }

  {
    invoke_body = "{\"method\":\"assets_whitelist_get\",\"params\":{}}";
    std::string res3 = plain_wallet::sync_call("invoke", instance_id, invoke_body);
    invoke_body = "";
  }



  {
    //invoke_body = "{\"method\":\"assets_whitelist_get\",\"params\":{}}";
    //std::string json_request;
    bool r = epee::file_io_utils::load_file_to_string("C:\\Users\\roky\\home\\wallets\\deploy_asset_request.json", invoke_body);
    CHECK_AND_ASSERT_MES(r, void(), "wrong bla bla bla");
    std::string res3 = plain_wallet::sync_call("invoke", instance_id, invoke_body);
    invoke_body = "";
  }


  //invoke_body = "{\"method\":\"get_recent_txs_and_info\",\"params\":{\"offset\":0,\"count\":30,\"update_provision_info\":true}}";  
  //std::string res2 = plain_wallet::sync_call("invoke", instance_id, invoke_body);

  //invoke_body = "{\"method\":\"get_recent_txs_and_info2\",\"params\":{\"offset\":0,\"count\":30,\"update_provision_info\":true}}";
  //res2 = plain_wallet::sync_call("invoke", instance_id, invoke_body);


  invoke_body = "{\"method\":\"getbalance\",\"params\":{}}";
  std::string res3 = plain_wallet::sync_call("invoke", instance_id, invoke_body);


  //invoke_body = "{\r\n  \"method\": \"transfer\",\r\n  \"params\": {\r\n    \"destinations\": [\r\n      {\r\n        \"amount\": \"1000000000000\",\r\n        \"address\": \"ZxD9oVwGwW6ULix9Pqttnr7JDpaoLvDVA1KJ9eA9KRxPMRZT5X7WwtU94XH1Z6q6XTMxNbHmbV2xfZ429XxV6fST2DxEg4BQV\",\r\n        \"asset_id\": \"cc4e69455e63f4a581257382191de6856c2156630b3fba0db4bdd73ffcfb36b6\"\r\n      }\r\n    ],\r\n    \"fee\": 10000000000,\r\n    \"mixin\": 10,\r\n    \"payment_id\": \"\",\r\n    \"comment\": \"\",\r\n    \"push_payer\": false,\r\n    \"hide_receiver\": true\r\n  }\r\n}";
  //std::string res4 = plain_wallet::sync_call("invoke", instance_id, invoke_body);

  //LOG_PRINT_L0(res);

}

void multithread_test_of_get_coinbase_hash_cached()
{
  epee::math_helper::once_a_time_seconds<1> print_interwal;

  try
  {
    crypto::hash h = currency::null_hash;
    utils::threads_pool pool;
    pool.init();
    for (uint64_t j = 0; j != 100000000; j++)
    {

      print_interwal.do_call([&]() { LOG_PRINT_L0("Job " << j << " started, h=" << h); return true; });
      currency::block_extended_info bei = AUTO_VAL_INIT(bei);
      utils::threads_pool::jobs_container jobs;
      size_t i = 0;

      for (; i != 10; i++)
      {
        utils::threads_pool::add_job_to_container(jobs, [&, i]() {
          h = get_coinbase_hash_cached(bei);
          //LOG_PRINT_L0("Job " << i << " started"); 
          //epee::misc_utils::sleep_no_w(10000); 
          // ++count_jobs_finished; LOG_PRINT_L0("Job " << i << " finished"); 
          });
      }

      pool.add_batch_and_wait(jobs);
    }
  }
  catch (...)
  {
    LOG_ERROR("Exception happened");
  }


}



int main(int argc, char** argv)
{
  epee::string_tools::set_module_name_and_folder(argv[0]);
  epee::log_space::get_set_log_detalisation_level(true, LOG_LEVEL_3);
  epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_3);
  //epee::log_space::log_singletone::add_logger(LOGGER_FILE,
  //  epee::log_space::log_singletone::get_default_log_file().c_str(),
  //  epee::log_space::log_singletone::get_default_log_folder().c_str());
  
  //multithread_test_of_get_coinbase_hash_cached();
  //test_tx_json_serialization();
  //test_base64_serialization();
  test_plain_wallet();
  //parse_weird_tx();
  //thread_pool_tests();

//   std::string buf1 = tools::get_varint_data<uint64_t>(CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX);
//   std::string buf2 = tools::get_varint_data<uint64_t>(CURRENCY_PUBLIC_INTEG_ADDRESS_BASE58_PREFIX);
//   std::string buf3 = tools::get_varint_data<uint64_t>(CURRENCY_PUBLIC_INTEG_ADDRESS_V2_BASE58_PREFIX);
//   std::string buf4 = tools::get_varint_data<uint64_t>(CURRENCY_PUBLIC_AUDITABLE_ADDRESS_BASE58_PREFIX);
//   std::string buf5 = tools::get_varint_data<uint64_t>(CURRENCY_PUBLIC_AUDITABLE_INTEG_ADDRESS_BASE58_PREFIX);
// 
//   std::cout << "Buf1: " << epee::string_tools::buff_to_hex_nodelimer(buf1) << ENDL;
//   std::cout << "Buf2: " << epee::string_tools::buff_to_hex_nodelimer(buf2) << ENDL;
//   std::cout << "Buf3: " << epee::string_tools::buff_to_hex_nodelimer(buf3) << ENDL;
//   std::cout << "Buf4: " << epee::string_tools::buff_to_hex_nodelimer(buf4) << ENDL;
//   std::cout << "Buf5: " << epee::string_tools::buff_to_hex_nodelimer(buf5) << ENDL;
// 
//   
  //do_htlc_hash_tests();
  //run_serialization_performance_test();
  //return 1;
  //run_core_market_performance_tests(100000);

  //set_process_affinity(1);
  //set_thread_high_priority();

  //do_chacha_stream_performance_test();
  //test_blake2();

  //free_space_check();
  
  //print_struct_to_json();

  //performance_timer timer;
  //timer.start();

  //generate_scratchpad();
  //generate_light_scratchpad(); 
  //measure_keccak_over_scratchpad();
  //test_scratchpad_against_light_scratchpad();
  /*
  TEST_PERFORMANCE2(test_construct_tx, 1, 1);
  TEST_PERFORMANCE2(test_construct_tx, 1, 2);
  TEST_PERFORMANCE2(test_construct_tx, 1, 10);
  TEST_PERFORMANCE2(test_construct_tx, 1, 100);
  TEST_PERFORMANCE2(test_construct_tx, 1, 1000);

  TEST_PERFORMANCE2(test_construct_tx, 2, 1);
  TEST_PERFORMANCE2(test_construct_tx, 2, 2);
  TEST_PERFORMANCE2(test_construct_tx, 2, 10);
  TEST_PERFORMANCE2(test_construct_tx, 2, 100);

  TEST_PERFORMANCE2(test_construct_tx, 10, 1);
  TEST_PERFORMANCE2(test_construct_tx, 10, 2);
  TEST_PERFORMANCE2(test_construct_tx, 10, 10);
  TEST_PERFORMANCE2(test_construct_tx, 10, 100);

  TEST_PERFORMANCE2(test_construct_tx, 100, 1);
  TEST_PERFORMANCE2(test_construct_tx, 100, 2);
  TEST_PERFORMANCE2(test_construct_tx, 100, 10);
  TEST_PERFORMANCE2(test_construct_tx, 100, 100);

  TEST_PERFORMANCE1(test_check_ring_signature, 1);
  TEST_PERFORMANCE1(test_check_ring_signature, 2);
  TEST_PERFORMANCE1(test_check_ring_signature, 10);
  TEST_PERFORMANCE1(test_check_ring_signature, 100);
  */
  //TEST_PERFORMANCE0(test_is_out_to_acc);
  //TEST_PERFORMANCE0(test_generate_key_image_helper);
  //TEST_PERFORMANCE0(test_generate_key_derivation);
  //TEST_PERFORMANCE0(test_generate_key_image);
  //TEST_PERFORMANCE0(test_derive_public_key);
  //TEST_PERFORMANCE0(test_derive_secret_key);
  
  //std::cout << "Tests finished. Elapsed time: " << timer.elapsed_ms() / 1000 << " sec" << std::endl;

  return 0;
}
