// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/program_options/variables_map.hpp>

#include "storages/levin_abstract_invoke2.h"
#include "warnings.h"
#include "currency_protocol_defs.h"
#include "currency_protocol_handler_common.h"
#include "currency_core/connection_context.h"
#include "currency_core/currency_stat_info.h"
#include "currency_core/verification_context.h"

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL "currency_protocol" 

PUSH_VS_WARNINGS
DISABLE_VS_WARNINGS(4355)

#define ASYNC_RELAY_MODE // relay transactions asyncronously via m_relay_que

namespace currency
{

  template<class t_core>
  class t_currency_protocol_handler:  public i_currency_protocol
  { 
  public:
    typedef currency_connection_context connection_context;
    typedef core_stat_info stat_info;
    typedef t_currency_protocol_handler<t_core> currency_protocol_handler;
    typedef CORE_SYNC_DATA payload_type;
    typedef std::pair<NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::request, currency_connection_context> relay_que_entry;

    t_currency_protocol_handler(t_core& rcore, nodetool::i_p2p_endpoint<connection_context>* p_net_layout);
    ~t_currency_protocol_handler();

    BEGIN_INVOKE_MAP2(currency_protocol_handler)
      HANDLE_NOTIFY_T2(NOTIFY_NEW_BLOCK, &currency_protocol_handler::handle_notify_new_block)
      HANDLE_NOTIFY_T2(NOTIFY_OR_INVOKE_NEW_TRANSACTIONS, &currency_protocol_handler::handle_notify_new_transactions)
      HANDLE_INVOKE_T2(NOTIFY_OR_INVOKE_NEW_TRANSACTIONS, &currency_protocol_handler::handle_invoke_new_transaction)
      HANDLE_NOTIFY_T2(NOTIFY_REQUEST_GET_OBJECTS, &currency_protocol_handler::handle_request_get_objects)
      HANDLE_NOTIFY_T2(NOTIFY_RESPONSE_GET_OBJECTS, &currency_protocol_handler::handle_response_get_objects)
      HANDLE_NOTIFY_T2(NOTIFY_REQUEST_CHAIN, &currency_protocol_handler::handle_request_chain)
      HANDLE_NOTIFY_T2(NOTIFY_RESPONSE_CHAIN_ENTRY, &currency_protocol_handler::handle_response_chain_entry)
    END_INVOKE_MAP2()

    bool on_idle();
    bool init(const boost::program_options::variables_map& vm);
    bool deinit();
    void set_p2p_endpoint(nodetool::i_p2p_endpoint<connection_context>* p2p);
    //bool process_handshake_data(const blobdata& data, currency_connection_context& context);
    bool process_payload_sync_data(const CORE_SYNC_DATA& hshd, currency_connection_context& context, bool is_inital);
    bool get_payload_sync_data(blobdata& data);
    bool get_payload_sync_data(CORE_SYNC_DATA& hshd);
    bool get_stat_info(const core_stat_info::params& pr, core_stat_info& stat_inf);
    bool on_callback(currency_connection_context& context);
    t_core& get_core(){return m_core;}
    virtual bool is_synchronized(){ return m_synchronized; }
    void log_connections();
    uint64_t get_core_inital_height();
    uint64_t get_max_seen_height();
    virtual size_t get_synchronized_connections_count();
    virtual size_t get_synchronizing_connections_count();

    int64_t get_net_time_delta_median();
    bool add_time_delta_and_check_time_sync(int64_t delta);
    bool get_last_time_sync_difference(int64_t& last_median2local_time_difference, int64_t& last_ntp2local_time_difference); // returns true if differences in allowed bounds
                                                                                                                             //-----------------------------------------------------------------------------------
    void set_to_debug_mode(uint32_t ip);

  private:
    //----------------- commands handlers ----------------------------------------------
    int handle_notify_new_block(int command, NOTIFY_NEW_BLOCK::request& arg, currency_connection_context& context);
    int handle_notify_new_transactions(int command, NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::request& arg, currency_connection_context& context);
    int handle_invoke_new_transaction(int command, NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::request& req, NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::response& rsp, currency_connection_context& context);
    int handle_request_get_objects(int command, NOTIFY_REQUEST_GET_OBJECTS::request& arg, currency_connection_context& context);
    int handle_response_get_objects(int command, NOTIFY_RESPONSE_GET_OBJECTS::request& arg, currency_connection_context& context);
    int handle_request_chain(int command, NOTIFY_REQUEST_CHAIN::request& arg, currency_connection_context& context);
    int handle_response_chain_entry(int command, NOTIFY_RESPONSE_CHAIN_ENTRY::request& arg, currency_connection_context& context);
   
    

    //----------------- i_bc_protocol_layout ---------------------------------------
    virtual bool relay_block(NOTIFY_NEW_BLOCK::request& arg, currency_connection_context& exclude_context);
    virtual bool relay_transactions(NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::request& arg, currency_connection_context& exclude_context);
    //----------------------------------------------------------------------------------
    //bool get_payload_sync_data(HANDSHAKE_DATA::request& hshd, currency_connection_context& context);
    bool request_missing_objects(currency_connection_context& context, bool check_having_blocks);
    bool on_connection_synchronized(); 
    void relay_que_worker();
    void process_current_relay_que(const std::list<relay_que_entry>& que);
    bool check_stop_flag_and_drop_cc(currency_connection_context& context);
    int handle_new_transaction_from_net(NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::request& req, NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::response& rsp, currency_connection_context& context, bool is_notify);
    t_core& m_core;

    nodetool::p2p_endpoint_stub<connection_context> m_p2p_stub;
    nodetool::i_p2p_endpoint<connection_context>* m_p2p;
    std::atomic<bool> m_synchronized;
    std::atomic<bool> m_have_been_synchronized;
    std::atomic<uint64_t> m_max_height_seen;
    std::atomic<uint64_t> m_core_inital_height;

    std::unordered_set<crypto::hash> m_blocks_id_que;
    std::recursive_mutex m_blocks_id_que_lock;

    std::list<relay_que_entry> m_relay_que;
    std::mutex m_relay_que_lock;
    std::condition_variable m_relay_que_cv;
    std::thread m_relay_que_thread;
    std::atomic<bool> m_want_stop;

    std::deque<int64_t> m_time_deltas;
    std::mutex m_time_deltas_lock;
    int64_t m_last_median2local_time_difference;
    int64_t m_last_ntp2local_time_difference;
    uint32_t m_debug_ip_address;
    bool m_disable_ntp;

    template<class t_parametr>
    bool post_notify(typename t_parametr::request& arg, currency_connection_context& context)
    {
      LOG_PRINT_L3("[POST]" << typeid(t_parametr).name() << " to " << context);
      std::string blob;
      epee::serialization::store_t_to_binary(arg, blob);
      return m_p2p->invoke_notify_to_peer(t_parametr::ID, blob, context);
    }

    template<class t_parametr>
    bool relay_post_notify(typename t_parametr::request& arg, currency_connection_context& exlude_context)
    {        
      std::string arg_buff;
      epee::serialization::store_t_to_binary(arg, arg_buff);
      std::list<epee::net_utils::connection_context_base> relayed_peers;
      bool r = m_p2p->relay_notify_to_all(t_parametr::ID, arg_buff, exlude_context, relayed_peers);

      LOG_PRINT_GREEN("[POST RELAY] " << typeid(t_parametr).name() << " to (" << relayed_peers.size() << "): " << print_connection_context_list(relayed_peers, ", "), LOG_LEVEL_2);
      return r;
    }
  };
}


#include "currency_protocol_handler.inl"

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL NULL

POP_VS_WARNINGS
