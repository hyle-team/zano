// Copyright (c) 2014-2018 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "serialization/keyvalue_hexemizer.h"
#include "currency_core/currency_basic.h"
#include "storages/portable_storage_base.h"
#include "currency_core/basic_api_response_codes.h"
#include "common/error_codes.h"
namespace currency
{
  //-----------------------------------------------


  struct tx_cost_struct
  {
    std::string usd_needed_for_erc20;
    std::string zano_needed_for_erc20;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(usd_needed_for_erc20)
      KV_SERIALIZE(zano_needed_for_erc20)
    END_KV_SERIALIZE_MAP()
  };

  struct rpc_get_wrap_info_response
  {
    std::string unwraped_coins_left;
    tx_cost_struct tx_cost;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(unwraped_coins_left)
      KV_SERIALIZE(tx_cost)
    END_KV_SERIALIZE_MAP()
  };


}

