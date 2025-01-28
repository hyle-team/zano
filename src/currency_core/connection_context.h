// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <unordered_set>
#include <atomic>
#include "net/net_utils_base.h"
#include "copyable_atomic.h"

namespace currency
{

  struct block_context_info
  {
    crypto::hash h;
    uint64_t cumul_size;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_VAL_POD_AS_BLOB(h)
      KV_SERIALIZE(cumul_size)
    END_KV_SERIALIZE_MAP()
  };


  struct uncopybale_currency_context
  {
    uncopybale_currency_context() = default;
    uncopybale_currency_context(const uncopybale_currency_context& /**/) {}
    uncopybale_currency_context& operator =(const uncopybale_currency_context& /**/) { return *this; }
    //members that supposed to be accessed only from one thread
    std::list<block_context_info> m_needed_objects;
    std::unordered_set<crypto::hash> m_requested_objects;
    std::atomic<uint32_t> m_callback_request_count; //in debug purpose: problem with double callback rise

  };

  struct currency_connection_context: public epee::net_utils::connection_context_base
  {
    enum state
    {
      state_befor_handshake = 0, //default state
      state_synchronizing,
      state_idle,
      state_normal
    };

    state m_state;
    uint64_t m_remote_blockchain_height;
    uint64_t m_last_response_height;
    int64_t m_time_delta;
    std::string m_remote_version;
    int m_build_number = 0;

  private:
    template<class t_core> friend class t_currency_protocol_handler;
    uncopybale_currency_context m_priv;
  };

  inline std::string get_protocol_state_string(currency_connection_context::state s)
  {
    switch (s)
    {
    case currency_connection_context::state_befor_handshake:
      return "state_befor_handshake";
    case currency_connection_context::state_synchronizing:
      return "state_synchronizing";
    case currency_connection_context::state_idle:
      return "state_idle";
    case currency_connection_context::state_normal:
      return "state_normal";
    default:
      return "unknown";
    }    
  }

}
