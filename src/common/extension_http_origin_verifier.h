// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstdint>
#include <string>
#include "net/http_server_handlers_map2.h"

#define ZANO_EXTENSION_HTTP_ORIGIN_ID "chrome-extension://akcgnllhhhkcpmlenfpicmcpgfpindlb"

namespace tools
{
 
  class extension_http_origin_verifier
  {
    bool m_enabled = true;

public:
    bool is_enabled() const { return m_enabled; }
    void set_enabled(bool enabled) {m_enabled = enabled;}
    bool handle_http_request(const epee::net_utils::http::http_request_info& query_info, epee::net_utils::http::http_response_info& response_info,
                             epee::net_utils::connection_context_base& conn_context, bool& call_found, documentation& docs = epee::net_utils::http::i_chain_handler::m_empty_documentation)
    {

      if(!m_enabled)
        return false;

      auto it = std::find_if(query_info.m_header_info.m_etc_fields.begin(), query_info.m_header_info.m_etc_fields.end(), [](const auto& element)
                             { return element.first == "Origin"; });
      if(it == query_info.m_header_info.m_etc_fields.end())
        return false;
      
      if(it->second != ZANO_EXTENSION_HTTP_ORIGIN_ID)
      {
        call_found = true;
        response_info.m_response_code = 403;
        response_info.m_response_comment = "Forbidden";
        return true;
      }
      
      return false;
    }
  };
}
