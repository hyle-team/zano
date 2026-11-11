// Copyright (c) 2012-2013 The Boolberry developers
// Copyright (c) 2012-2013 The Zano developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "currency_core/currency_basic.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "wallet/wallet_public_structs_defs.h"



inline bool print_struct_to_json()
{
  tools::wallet_public::COMMAND_MARKETPLACE_PUSH_OFFER::request of = AUTO_VAL_INIT(of);
  tools::wallet_public::COMMAND_MARKETPLACE_PUSH_UPDATE_OFFER::request uo = AUTO_VAL_INIT(uo);

  bc_services::core_offers_filter ft = AUTO_VAL_INIT(ft);

  std::string s = epee::serialization::store_t_to_json(of);
  s = epee::serialization::store_t_to_json(ft);
  s = epee::serialization::store_t_to_json(uo);
  
  return true;
}