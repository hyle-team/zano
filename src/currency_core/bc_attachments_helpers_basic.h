// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "currency_basic.h"

namespace bc_services
{
  template<class t_attachment_type_container_t>
  bool get_first_service_attachment_by_id(const t_attachment_type_container_t& tx_items, const std::string& id, const std::string& instruction, currency::tx_service_attachment& res)
  {
    for (const auto& item : tx_items)
    {
      if (item.type() == typeid(currency::tx_service_attachment))
      {
        const currency::tx_service_attachment& tsa = boost::get<currency::tx_service_attachment>(item);
        if (tsa.service_id == id && tsa.instruction == instruction)
        {
          res = tsa;
          return true;
        }
      }
    }
    return false;
  }
}