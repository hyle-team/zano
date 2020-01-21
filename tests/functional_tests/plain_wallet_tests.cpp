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
  plain_wallet::hwallet hw = plain_wallet::create_instance("11211", "127.0.0.1.");
  LOG_PRINT_L0("Creating instance..." << std::hex << hw);

  LOG_PRINT_L0("Generating wallet...");
  std::string rsp = plain_wallet::generate(hw, "E:\\tmp\\sdsd", "");
  LOG_PRINT_L0("RESPONSE:" << ENDL << rsp);
  epee::json_rpc::response<plain_wallet::open_wallet_response, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
  epee::serialization::load_t_from_json(ok_response, rsp);

  plain_wallet::start_sync_thread(hw);
  LOG_PRINT_L0("Started sync thread.");

  while (true)
  {
    std::string prog = plain_wallet::get_sync_status(hw);
    plain_wallet::sync_status_response ssr = AUTO_VAL_INIT(ssr);
    epee::serialization::load_t_from_json(ssr, prog);
    LOG_PRINT_L0("Progress: " << ssr.progress << "Finished: " << ssr.finished);
    if (ssr.finished)
      break;
    epee::misc_utils::sleep_no_w(100);
  }



}