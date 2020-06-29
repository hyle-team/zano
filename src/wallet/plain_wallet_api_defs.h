// Copyright (c) 2014-2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "net/http_server_handlers_map2.h"
#include "view_iface.h"

namespace plain_wallet
{
  struct error
  {
    std::string code;
    std::string message;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(code)
      KV_SERIALIZE(message)
    END_KV_SERIALIZE_MAP()
  };
  

//   struct open_wallet_response
//   {
//     view::transfers_array recent_history;
//     view::wallet_info wi;
//     BEGIN_KV_SERIALIZE_MAP()
//       KV_SERIALIZE(recent_history)
//       KV_SERIALIZE(wi)
//     END_KV_SERIALIZE_MAP()
//   };

  struct sync_status_response
  {
    bool finished;
    uint64_t progress;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(finished)
      KV_SERIALIZE(progress)
    END_KV_SERIALIZE_MAP()
  };

  struct basic_status_response
  {
    std::string status;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(status)
    END_KV_SERIALIZE_MAP()
  };

} // namespace tools
