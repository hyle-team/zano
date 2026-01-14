// Copyright (c) 2014-2018 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "i_core_rpc_proxy.h"
#include "currency_core/alias_helper.h"

namespace tools
{
  bool i_core_proxy::get_transfer_address(const std::string& adr_str, currency::account_public_address& addr, std::string& payment_id)
  {
    return tools::get_transfer_address(adr_str, addr, payment_id, this);
  }
  bool i_core_proxy::get_transfer_address(const std::string& adr_str, currency::v_address& addr, std::string& payment_id)
  {
    return tools::get_transfer_address(adr_str, addr, payment_id, this);
  }
}

