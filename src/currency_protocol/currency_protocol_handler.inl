// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/interprocess/detail/atomic.hpp>
#include "currency_core/currency_format_utils.h"
#include "profile_tools.h"
namespace currency
{

  //-----------------------------------------------------------------------------------------------------------------------  
  template<class t_core>
  t_currency_protocol_handler<t_core>::t_currency_protocol_handler(t_core& rcore, nodetool::i_p2p_endpoint<connection_context>* p_net_layout)
    : m_core(rcore)
    , m_p2p(p_net_layout)
    , m_synchronized(false)
    , m_have_been_synchronized(false)
    , m_max_height_seen(0)
    , m_core_inital_height(0)
    , m_want_stop(false)
    , m_last_median2local_time_difference(0)
    , m_last_ntp2local_time_difference(0)
    , m_debug_ip_address(0)
    , m_disable_ntp(false)
  {
    if(!m_p2p)
      m_p2p = &m_p2p_stub;
  }
  //-----------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  t_currency_protocol_handler<t_core>::~t_currency_protocol_handler()
  {
    deinit();
  }
  //-----------------------------------------------------------------------------------------------------------------------
  template<class t_core> 
  bool t_currency_protocol_handler<t_core>::init(const boost::program_options::variables_map& vm)
  {
    m_relay_que_thread = std::thread([this](){relay_que_worker();});
    if (command_line::has_arg(vm, command_line::arg_disable_ntp))
      m_disable_ntp = command_line::get_arg(vm, command_line::arg_disable_ntp);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------  
  template<class t_core> 
  bool t_currency_protocol_handler<t_core>::deinit()
  {    
    m_want_stop = true;
    m_relay_que_cv.notify_all();
    if (m_relay_que_thread.joinable())
      m_relay_que_thread.join();
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------  
  template<class t_core> 
  void t_currency_protocol_handler<t_core>::set_p2p_endpoint(nodetool::i_p2p_endpoint<connection_context>* p2p)
  {
    if(p2p)
      m_p2p = p2p;
    else
      m_p2p = &m_p2p_stub;
  }
  //------------------------------------------------------------------------------------------------------------------------  
  template<class t_core> 
  bool t_currency_protocol_handler<t_core>::on_callback(currency_connection_context& context)
  {
    LOG_PRINT_L3("callback fired");
    CHECK_AND_ASSERT_MES_CC( context.m_priv.m_callback_request_count > 0, false, "false callback fired, but context.m_priv.m_callback_request_count=" << context.m_priv.m_callback_request_count);
    --context.m_priv.m_callback_request_count;

    if(context.m_state == currency_connection_context::state_synchronizing)
    {
      NOTIFY_REQUEST_CHAIN::request r = boost::value_initialized<NOTIFY_REQUEST_CHAIN::request>();
      m_core.get_short_chain_history(r.block_ids);
      LOG_PRINT_L2("[NOTIFY]NOTIFY_REQUEST_CHAIN(on_callback): m_block_ids.size()=" << r.block_ids.size());
      LOG_PRINT_L3("[NOTIFY]NOTIFY_REQUEST_CHAIN(on_callback): " << ENDL << print_kv_structure(r));
      post_notify<NOTIFY_REQUEST_CHAIN>(r, context);
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core> 
  bool t_currency_protocol_handler<t_core>::get_stat_info(const core_stat_info::params& pr, core_stat_info& stat_inf)
  {
    return m_core.get_stat_info(pr, stat_inf);
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core> 
  void t_currency_protocol_handler<t_core>::log_connections()
  {
    std::stringstream ss;

    ss << std::setw(29) << std::left << "Remote Host" 
      << std::setw(20) << "Peer id"
      << std::setw(25) << "Recv/Sent (idle,sec)"
      << std::setw(25) << "State"
      << std::setw(20) << "Livetime" 
      << std::setw(20) << "Client version" << ENDL;

    size_t incoming_count = 0, outgoing_count = 0;
    std::multimap<time_t, std::string> conn_map;
    m_p2p->for_each_connection([&](const connection_context& cntxt, nodetool::peerid_type peer_id)
    {
      std::stringstream conn_ss;
      time_t livetime = time(NULL) - cntxt.m_started;
      conn_ss << std::setw(29) << std::left << std::string(cntxt.m_is_income ? "[INC]":"[OUT]") + 
        epst::get_ip_string_from_int32(cntxt.m_remote_ip) + ":" + std::to_string(cntxt.m_remote_port) 
        << std::setw(20) << std::hex << peer_id
        << std::setw(25) << std::to_string(cntxt.m_recv_cnt)+ "(" + std::to_string(time(NULL) - cntxt.m_last_recv) + ")" + "/" + std::to_string(cntxt.m_send_cnt) + "(" + std::to_string(time(NULL) - cntxt.m_last_send) + ")"
        << std::setw(25) << get_protocol_state_string(cntxt.m_state)
        << std::setw(20) << epee::misc_utils::get_time_interval_string(livetime) 
        << std::setw(20) << cntxt.m_remote_version
        << ENDL;
      conn_map.insert(std::make_pair(livetime, conn_ss.str()));
      (cntxt.m_is_income ? incoming_count : outgoing_count) += 1;
      return true;
    });

    for(auto it = conn_map.rbegin(); it != conn_map.rend(); ++it)
      ss << it->second;

    LOG_PRINT_L0("Connections (" << incoming_count << " in, " << outgoing_count << " out, " << incoming_count + outgoing_count << " total):" << ENDL << ss.str());
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core> 
  bool t_currency_protocol_handler<t_core>::process_payload_sync_data(const CORE_SYNC_DATA& hshd, currency_connection_context& context, bool is_inital)
  {


    context.m_remote_version = hshd.client_version;

    if(context.m_state == currency_connection_context::state_befor_handshake && !is_inital)
      return true;

    uint64_t local_time = m_core.get_blockchain_storage().get_core_runtime_config().get_core_time();
    context.m_time_delta = local_time - hshd.core_time;

    // for outgoing connections -- check time difference
    if (!context.m_is_income)
    {
      if (!add_time_delta_and_check_time_sync(context.m_time_delta))
      {
        // serious time sync problem detected
        i_critical_error_handler* ceh(m_core.get_critical_error_handler());
        if (ceh != nullptr && ceh->on_critical_time_sync_error())
        {
          // error is handled by a callee, should not be ignored here, stop processing immideately
          return true;
        }
      }
    }

    if(context.m_state == currency_connection_context::state_synchronizing)
      return true;


    bool have_in_que = false;
    CRITICAL_REGION_BEGIN(m_blocks_id_que_lock);
    have_in_que = m_blocks_id_que.find(hshd.top_id) != m_blocks_id_que.end();
    CRITICAL_REGION_END();

    if(have_in_que || m_core.have_block(hshd.top_id))
    {

      context.m_state = currency_connection_context::state_normal;
      if(is_inital)
        on_connection_synchronized();
      return true;
    } 

    if(hshd.top_id == currency::null_hash)
    {
      LOG_PRINT_L0("wtf");
    }

    int64_t diff = static_cast<int64_t>(hshd.current_height) - static_cast<int64_t>(m_core.get_current_blockchain_size());
    LOG_PRINT_COLOR2(LOG_DEFAULT_TARGET, (is_inital ? "Inital ":"Idle ") << "sync data returned unknown top block (" << hshd.top_id << "): " << m_core.get_top_block_height() << " -> " << hshd.current_height - 1
      << " [" << std::abs(diff) << " blocks (" << diff / (24 * 60 * 60 / DIFFICULTY_TOTAL_TARGET ) << " days) "
      << (0 <= diff ? std::string("behind") : std::string("ahead"))
      << "] " << ENDL << "SYNCHRONIZATION started", (is_inital ? LOG_LEVEL_0 : LOG_LEVEL_1), (is_inital ? epee::log_space::console_color_yellow : epee::log_space::console_color_magenta));
    LOG_PRINT_L1("Remote top block height: " << hshd.current_height << ", id: " << hshd.top_id);
    /*check if current height is in remote's checkpoints zone*/
    if(hshd.last_checkpoint_height 
      && m_core.get_blockchain_storage().get_checkpoints().get_top_checkpoint_height() < hshd.last_checkpoint_height 
      && m_core.get_current_blockchain_size() < hshd.last_checkpoint_height )
    {
      LOG_PRINT_RED("Remote node has longer checkpoints zone (" << hshd.last_checkpoint_height <<  ") " << 
        "than local (" << m_core.get_blockchain_storage().get_checkpoints().get_top_checkpoint_height() << "). " <<
        "It means that current software is outdated, please updated it! " << 
        "Current height lays under checkpoints zone on remote host, so it's impossible to validate remote transactions locally, disconnecting.", LOG_LEVEL_0);
      return false;
    }
    else if (m_core.get_blockchain_storage().get_checkpoints().get_top_checkpoint_height() < hshd.last_checkpoint_height)
    {
      LOG_PRINT_MAGENTA("Remote node has longer checkpoints zone (" << hshd.last_checkpoint_height <<  ") " <<
        "than local (" << m_core.get_blockchain_storage().get_checkpoints().get_top_checkpoint_height() << "). " << 
        "It means that current software is outdated, please updated it!", LOG_LEVEL_0);
    }

    context.m_state = currency_connection_context::state_synchronizing;
    context.m_remote_blockchain_height = hshd.current_height;
    //let the socket to send response to handshake, but request callback, to let send request data after response
    LOG_PRINT_L3("requesting callback");
    ++context.m_priv.m_callback_request_count;
    m_p2p->request_callback(context);
    //update progress vars 
    if (m_max_height_seen < hshd.current_height)
      m_max_height_seen = hshd.current_height;
    if (!m_core_inital_height)
      m_core_inital_height = m_core.get_current_blockchain_size();

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------  
  template<class t_core>
  uint64_t t_currency_protocol_handler<t_core>::get_core_inital_height()
  {
    return m_core_inital_height;
  }
  //------------------------------------------------------------------------------------------------------------------------  
  template<class t_core>
  uint64_t t_currency_protocol_handler<t_core>::get_max_seen_height()
  {
    return m_max_height_seen;
  }
  //------------------------------------------------------------------------------------------------------------------------  
  template<class t_core> 
  bool t_currency_protocol_handler<t_core>::get_payload_sync_data(CORE_SYNC_DATA& hshd)
  {
    m_core.get_blockchain_top(hshd.current_height, hshd.top_id);
    hshd.current_height +=1;
    hshd.last_checkpoint_height = m_core.get_blockchain_storage().get_checkpoints().get_top_checkpoint_height();
    hshd.core_time = m_core.get_blockchain_storage().get_core_runtime_config().get_core_time();
    hshd.client_version = PROJECT_VERSION_LONG;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------  
    template<class t_core> 
    bool t_currency_protocol_handler<t_core>::get_payload_sync_data(blobdata& data)
  {
    CORE_SYNC_DATA hsd = AUTO_VAL_INIT(hsd);
    get_payload_sync_data(hsd);
    epee::serialization::store_t_to_binary(hsd, data);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------  
    template<class t_core> 
    int t_currency_protocol_handler<t_core>::handle_notify_new_block(int command, NOTIFY_NEW_BLOCK::request& arg, currency_connection_context& context)
  {
    //do not process requests if it comes from node wich is debugged
    if (m_debug_ip_address != 0 && context.m_remote_ip == m_debug_ip_address)
        return 1;

    if(context.m_state != currency_connection_context::state_normal)
      return 1;

    //check if block already exists
    block b = AUTO_VAL_INIT(b);
    block_verification_context bvc = AUTO_VAL_INIT(bvc);
    if (!m_core.parse_block(arg.b.block, b, bvc))
    {
      LOG_PRINT_RED("Block parsing failed, dropping connection", LOG_LEVEL_0);
      m_p2p->drop_connection(context);
      return 1;
    }

    crypto::hash block_id = get_block_hash(b);
    LOG_PRINT_GREEN("[HANDLE]NOTIFY_NEW_BLOCK " << block_id << " HEIGHT " << get_block_height(b) << " (hop " << arg.hop << ")", LOG_LEVEL_2);

    CRITICAL_REGION_BEGIN(m_blocks_id_que_lock);
    auto it = m_blocks_id_que.find(block_id);
    if (it != m_blocks_id_que.end())
    {
      //already have this block handler in que
      LOG_PRINT("Block " << block_id << " already in processing que", LOG_LEVEL_3);
      return 1;
    }
    else
    {
      m_blocks_id_que.insert(block_id);
    }
    CRITICAL_REGION_END();
    auto slh = epee::misc_utils::create_scope_leave_handler([&]()
    {
      CRITICAL_REGION_LOCAL(m_blocks_id_que_lock);
      auto it = m_blocks_id_que.find(block_id);
      CHECK_AND_ASSERT_MES_NO_RET(it != m_blocks_id_que.end(), "Internal error, block " << block_id << " not found in m_blocks_id_que");
      m_blocks_id_que.erase(it);
    });

    if (m_core.have_block(block_id))
    {
      LOG_PRINT_L3("Block " << block_id << " already in core");
      return 1;
    }
    
    //pre-validate block here, and propagate it to network asap to avoid latency of handling big block (tx flood)
    //########################################################
    /*
    problem with prevalidation: in case of pre_validate_block() is passed but handle_incoming_tx() is failed
    network got spammed with notifications about this broken block and then connections got closed.
    temporary disabled to more investigation
    bool prevalidate_relayed = false; 
    if (m_core.pre_validate_block(b, bvc, block_id) && bvc.m_added_to_main_chain)
    {
      //not alternative block, relay it
      ++arg.hop;
      relay_block(arg, context);
      prevalidate_relayed = true;
    }
    */
    //########################################################

    //now actually process block
    for(auto tx_blob_it = arg.b.txs.begin(); tx_blob_it!=arg.b.txs.end();tx_blob_it++)
    {
      if (tx_blob_it->size() > CURRENCY_MAX_TRANSACTION_BLOB_SIZE)
      {
        LOG_ERROR("WRONG TRANSACTION BLOB, too big size " << tx_blob_it->size() << ", rejected");
        m_p2p->drop_connection(context);
        return 1;
      }

      crypto::hash tx_hash = null_hash;
      transaction tx;
      if (!parse_and_validate_tx_from_blob(*tx_blob_it, tx, tx_hash))
      {
        LOG_ERROR("WRONG TRANSACTION BLOB, Failed to parse, rejected");
        m_p2p->drop_connection(context);
        return 1;
      }
      bvc.m_onboard_transactions[tx_hash] = tx;
    }
    
    m_core.pause_mine();
    m_core.handle_incoming_block(b, bvc);
    m_core.resume_mine();
    if(bvc.m_verification_failed)
    {
      LOG_PRINT_L0("Block verification failed, dropping connection");
      m_p2p->drop_connection(context);
      return 1;
    }
    LOG_PRINT_GREEN("[HANDLE]NOTIFY_NEW_BLOCK EXTRA " << block_id 
      << " bvc.m_added_to_main_chain=" << bvc.m_added_to_main_chain
      //<< ", prevalidate_result=" << prevalidate_relayed
      << ", bvc.added_to_altchain=" << bvc.m_added_to_altchain
      << ", bvc.m_marked_as_orphaned=" << bvc.m_marked_as_orphaned, LOG_LEVEL_2);

    if (bvc.m_added_to_main_chain || (bvc.m_added_to_altchain && bvc.m_height_difference < 2))
    { 
      if (true/*!prevalidate_relayed*/)
      {
        // pre-validation failed prevoiusly, but complete check was success, not an alternative block
        ++arg.hop;
        //TODO: Add here announce protocol usage
        relay_block(arg, context);
      }
    }else if(bvc.m_marked_as_orphaned)
    {
      context.m_state = currency_connection_context::state_synchronizing;
      NOTIFY_REQUEST_CHAIN::request r = boost::value_initialized<NOTIFY_REQUEST_CHAIN::request>();
      m_core.get_short_chain_history(r.block_ids);
      LOG_PRINT_MAGENTA("State changed to state_synchronizing.", LOG_LEVEL_2);
      LOG_PRINT_L2("[NOTIFY]NOTIFY_REQUEST_CHAIN(on_orphaned): m_block_ids.size()=" << r.block_ids.size() );
      LOG_PRINT_L3("[NOTIFY]NOTIFY_REQUEST_CHAIN(on_orphaned): " << ENDL << print_kv_structure(r));
      post_notify<NOTIFY_REQUEST_CHAIN>(r, context);
    }
      
    return 1;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core> 
  int t_currency_protocol_handler<t_core>::handle_notify_new_transactions(int command, NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::request& arg, currency_connection_context& context)
  {
    NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::response rsp_dummy = AUTO_VAL_INIT(rsp_dummy);
    return this->handle_new_transaction_from_net(arg, rsp_dummy, context, true);
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  int t_currency_protocol_handler<t_core>::handle_invoke_new_transaction(int command, NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::request& req, NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::response& rsp, currency_connection_context& context)
  {
    return this->handle_new_transaction_from_net(req, rsp, context, false);
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  int t_currency_protocol_handler<t_core>::handle_new_transaction_from_net(NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::request& arg, NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::response& rsp, currency_connection_context& context, bool is_notify)
  {
    //do not process requests if it comes from node wich is debugged
    if (m_debug_ip_address != 0 && context.m_remote_ip == m_debug_ip_address)
    {
      rsp.code = API_RETURN_CODE_ACCESS_DENIED;
      return 1;
    }

    //if(context.m_state != currency_connection_context::state_normal)
    //  return 1;
    if (!this->is_synchronized())
    {
      rsp.code = API_RETURN_CODE_BUSY;
      return 1;
    }


    uint64_t inital_tx_count = arg.txs.size();

    if (inital_tx_count > CURRENCY_RELAY_TXS_MAX_COUNT)
    {
      LOG_PRINT_L1("NOTIFY_NEW_TRANSACTIONS: To many transactions in NOTIFY_OR_INVOKE_NEW_TRANSACTIONS(" << inital_tx_count << ")");
      rsp.code = API_RETURN_CODE_OVERFLOW;
      return 1;
    }

    TIME_MEASURE_START_MS(new_transactions_handle_time);
    for (auto tx_blob_it = arg.txs.begin(); tx_blob_it != arg.txs.end();)
    {
      currency::tx_verification_context tvc = AUTO_VAL_INIT(tvc);

      m_core.handle_incoming_tx(*tx_blob_it, tvc, false);
      if (tvc.m_verification_failed)
      {
        LOG_PRINT_L0("NOTIFY_NEW_TRANSACTIONS: Tx verification failed, dropping connection");
        if(is_notify)
          m_p2p->drop_connection(context);
        else 
          rsp.code = API_RETURN_CODE_FAIL;
        return 1;
      }
      if (tvc.m_should_be_relayed)
        ++tx_blob_it;
      else
        arg.txs.erase(tx_blob_it++);
    }

    if (arg.txs.size())
    {
      //TODO: add announce usage here
      relay_transactions(arg, context);
    }
    TIME_MEASURE_FINISH_MS(new_transactions_handle_time);

    LOG_PRINT_L2("NOTIFY_OR_INVOKE_NEW_TRANSACTIONS(is_notify=" << is_notify <<"): " << new_transactions_handle_time << "ms (inital_tx_count: " << inital_tx_count << ", relayed_tx_count: " << arg.txs.size() << ")");
    rsp.code = API_RETURN_CODE_OK;
    return 1;
  }

  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core> 
  int t_currency_protocol_handler<t_core>::handle_request_get_objects(int command, NOTIFY_REQUEST_GET_OBJECTS::request& arg, currency_connection_context& context)
  {
    //do not process requests if it comes from node wich is debugged
    if (m_debug_ip_address != 0 && context.m_remote_ip == m_debug_ip_address)
      return 1;

    LOG_PRINT_L2("[HANDLE]NOTIFY_REQUEST_GET_OBJECTS: arg.blocks.size() = " << arg.blocks.size() << ", arg.txs.size()="<< arg.txs.size());
    LOG_PRINT_L3("[HANDLE]NOTIFY_REQUEST_GET_OBJECTS: " << ENDL << currency::print_kv_structure(arg));

    if (arg.blocks.size() > CURRENCY_PROTOCOL_MAX_BLOCKS_REQUEST_COUNT ||
      arg.txs.size() > CURRENCY_PROTOCOL_MAX_TXS_REQUEST_COUNT)
    {
      LOG_ERROR_CCONTEXT("Requested objects count is to big (" << arg.blocks.size() <<")expected not more then " << CURRENCY_PROTOCOL_MAX_BLOCKS_REQUEST_COUNT);
      m_p2p->drop_connection(context);
    }

    NOTIFY_RESPONSE_GET_OBJECTS::request rsp;
    if(!m_core.handle_get_objects(arg, rsp, context))
    {
      LOG_ERROR_CCONTEXT("failed to handle request NOTIFY_REQUEST_GET_OBJECTS, dropping connection");
      m_p2p->drop_connection(context);
    }

    LOG_PRINT_L2("[NOTIFY]NOTIFY_RESPONSE_GET_OBJECTS: blocks.size()=" << rsp.blocks.size() << ", txs.size()=" << rsp.txs.size() 
                            << ", rsp.m_current_blockchain_height=" << rsp.current_blockchain_height << ", missed_ids.size()=" << rsp.missed_ids.size());


    LOG_PRINT_L3("[NOTIFY]NOTIFY_RESPONSE_GET_OBJECTS: " << ENDL << currency::print_kv_structure(rsp));
    post_notify<NOTIFY_RESPONSE_GET_OBJECTS>(rsp, context);
    return 1;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_currency_protocol_handler<t_core>::check_stop_flag_and_drop_cc(currency_connection_context& context)
  {
    if (m_p2p->is_stop_signal_sent() || m_want_stop)
    {
      m_p2p->drop_connection(context);
      return true;
    }
    return false;
  }




#define CHECK_STOP_FLAG__DROP_AND_RETURN_IF_SET(ret_v, msg) if (check_stop_flag_and_drop_cc(context)) { LOG_PRINT_YELLOW("Stop flag detected within NOTIFY_RESPONSE_GET_OBJECTS. " << msg, LOG_LEVEL_0); return ret_v; }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  int t_currency_protocol_handler<t_core>::handle_response_get_objects(int command, NOTIFY_RESPONSE_GET_OBJECTS::request& arg, currency_connection_context& context)
  {
    //do not process requests if it comes from node wich is debugged
    if (m_debug_ip_address != 0 && context.m_remote_ip == m_debug_ip_address)
      return 1;

    LOG_PRINT_L2("[HANDLE]NOTIFY_RESPONSE_GET_OBJECTS: arg.blocks.size()=" << arg.blocks.size() << ", arg.missed_ids.size()=" << arg.missed_ids.size() << ", arg.txs.size()=" << arg.txs.size());
    LOG_PRINT_L3("[HANDLE]NOTIFY_RESPONSE_GET_OBJECTS: " << ENDL << currency::print_kv_structure(arg));
    if(context.m_last_response_height > arg.current_blockchain_height)
    {
      LOG_ERROR_CCONTEXT("sent wrong NOTIFY_HAVE_OBJECTS: arg.m_current_blockchain_height=" << arg.current_blockchain_height 
        << " < m_last_response_height=" << context.m_last_response_height << ", dropping connection");
      m_p2p->drop_connection(context);
      return 1;
    }

    context.m_remote_blockchain_height = arg.current_blockchain_height;

    uint64_t total_blocks_parsing_time = 0;
    size_t count = 0;
    for (const block_complete_entry& block_entry : arg.blocks)
    {
      CHECK_STOP_FLAG__DROP_AND_RETURN_IF_SET(1, "Blocks processing interrupted, connection dropped");

      ++count;
      block b;
      TIME_MEASURE_START(block_parsing_time);
      if(!parse_and_validate_block_from_blob(block_entry.block, b))
      {
        LOG_ERROR_CCONTEXT("sent wrong block: failed to parse and validate block: \r\n" 
          << epst::buff_to_hex_nodelimer(block_entry.block) << "\r\n dropping connection");
        m_p2p->drop_connection(context);
        m_p2p->add_ip_fail(context.m_remote_ip);
        return 1;
      }      
      TIME_MEASURE_FINISH(block_parsing_time);
      total_blocks_parsing_time += block_parsing_time;

      //to avoid concurrency in core between connections, suspend connections which delivered block later then first one
      if(count == 2)
      { 
        if(m_core.have_block(get_block_hash(b)))
        {
          context.m_state = currency_connection_context::state_idle;
          context.m_priv.m_needed_objects.clear();
          context.m_priv.m_requested_objects.clear();
          LOG_PRINT_L1("Connection set to idle state.");
          return 1;
        }
      }
      
      auto req_it = context.m_priv.m_requested_objects.find(get_block_hash(b));
      if(req_it == context.m_priv.m_requested_objects.end())
      {
        LOG_ERROR_CCONTEXT("sent wrong NOTIFY_RESPONSE_GET_OBJECTS: block with id=" << epst::pod_to_hex(get_blob_hash(block_entry.block)) 
          << " wasn't requested, dropping connection");
        m_p2p->drop_connection(context);
        return 1;
      }
      if(b.tx_hashes.size() != block_entry.txs.size()) 
      {
        LOG_ERROR_CCONTEXT("sent wrong NOTIFY_RESPONSE_GET_OBJECTS: block with id=" << epst::pod_to_hex(get_blob_hash(block_entry.block)) 
          << ", tx_hashes.size()=" << b.tx_hashes.size() << " mismatch with block_complete_entry.m_txs.size()=" << block_entry.txs.size() << ", dropping connection");
        m_p2p->drop_connection(context);
        return 1;
      }

      context.m_priv.m_requested_objects.erase(req_it);

      LOG_PRINT_L4("[NOTIFY_RESPONSE_GET_OBJECTS] BLOCK " << get_block_hash(b) << "[" << get_block_height(b) << "/" << count << "], txs: " << b.tx_hashes.size());
    }

    LOG_PRINT_CYAN("Block parsing time avr: " << (count > 0 ? total_blocks_parsing_time / count : 0) << " mcs, total for " << count << " blocks: " << total_blocks_parsing_time / 1000 << " ms", LOG_LEVEL_2);
    
    if(context.m_priv.m_requested_objects.size())
    {
      LOG_PRINT_RED("returned not all requested objects (context.m_priv.m_requested_objects.size()=" 
        << context.m_priv.m_requested_objects.size() << "), dropping connection", LOG_LEVEL_0);
      m_p2p->drop_connection(context);
      return 1;
    }

    {
      m_core.pause_mine();
      epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler(
        boost::bind(&t_core::resume_mine, &m_core));
      size_t count = 0;
      for (const block_complete_entry& block_entry : arg.blocks)
      {
        CHECK_STOP_FLAG__DROP_AND_RETURN_IF_SET(1, "Blocks processing interrupted, connection dropped");
        block_verification_context bvc = boost::value_initialized<block_verification_context>();
        //process transactions
        size_t count_txs = 0;
        TIME_MEASURE_START(transactions_process_time);
        for (const auto& tx_blob : block_entry.txs)
        {
          LOG_PRINT_L4("[NOTIFY_RESPONSE_GET_OBJECTS] BL/TX ["<< count << "/" << count_txs << "]: " << epst::buff_to_hex_nodelimer(tx_blob));
          CHECK_STOP_FLAG__DROP_AND_RETURN_IF_SET(1, "Block txs processing interrupted, connection dropped");
          crypto::hash tx_id = null_hash;
          transaction tx = AUTO_VAL_INIT(tx);
          if (!parse_and_validate_tx_from_blob(tx_blob, tx, tx_id))
          {
            LOG_ERROR_CCONTEXT("failed to parse tx: " 
              << epst::pod_to_hex(get_blob_hash(tx_blob)) << ", dropping connection");
            m_p2p->drop_connection(context);
            return 1;
          }
          bvc.m_onboard_transactions[tx_id] = tx;
          count_txs++;
//           tx_verification_context tvc = AUTO_VAL_INIT(tvc);
//           m_core.handle_incoming_tx(tx_blob, tvc, true);
//           if(tvc.m_verification_failed)
//           {
//             LOG_ERROR_CCONTEXT("transaction verification failed on NOTIFY_RESPONSE_GET_OBJECTS, \r\ntx_id = " 
//               << string_tools::pod_to_hex(get_blob_hash(tx_blob)) << ", dropping connection");
//             m_p2p->drop_connection(context);
//             return 1;
//           }
        }
        TIME_MEASURE_FINISH(transactions_process_time);

        //process block
        TIME_MEASURE_START(block_process_time);

        m_core.handle_incoming_block(block_entry.block, bvc, false);
        if (count > 2 && bvc.m_already_exists)
        {
          context.m_state = currency_connection_context::state_idle;
          context.m_priv.m_needed_objects.clear();
          context.m_priv.m_requested_objects.clear();
          LOG_PRINT_L1("Connection set to idle state.");
          return 1;
        }

        if(bvc.m_verification_failed)
        {
          LOG_PRINT_L0("Block verification failed, dropping connection");
          m_p2p->drop_connection(context);
          m_p2p->add_ip_fail(context.m_remote_ip);
          return 1;
        }
        if(bvc.m_marked_as_orphaned)
        {
          LOG_PRINT_L0("Block received at sync phase was marked as orphaned, dropping connection, details on response: " << ENDL << print_kv_structure(arg));
          
          m_p2p->drop_connection(context);
          m_p2p->add_ip_fail(context.m_remote_ip);
          return 1;
        }

        TIME_MEASURE_FINISH(block_process_time);
        LOG_PRINT_L2("Block process time: " << block_process_time + transactions_process_time << "(" << transactions_process_time << "/" << block_process_time << ")ms");
        ++count;
      }
    }
    uint64_t current_size = m_core.get_blockchain_storage().get_current_blockchain_size();
    LOG_PRINT_YELLOW(">>>>>>>>> sync progress: " << arg.blocks.size() << " blocks added, now have "
      << current_size << " of " << context.m_remote_blockchain_height
      << " ( " << std::fixed << std::setprecision(2) << current_size * 100.0 / context.m_remote_blockchain_height << "% ) and "
      << context.m_remote_blockchain_height - current_size << " blocks left"
      , LOG_LEVEL_0);

    request_missing_objects(context, true);
    return 1;
  }
#undef CHECK_STOP_FLAG__DROP_AND_RETURN_IF_SET
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core> 
  bool t_currency_protocol_handler<t_core>::on_idle()
  {
    size_t synchronized_connections_count = get_synchronized_connections_count();
    size_t total_connections_count = m_p2p->get_connections_count();
    bool have_enough_synchronized_connections = synchronized_connections_count > total_connections_count / 2;

    if (have_enough_synchronized_connections && !m_synchronized)
    {
      on_connection_synchronized();
      m_synchronized = true;
      LOG_PRINT_MAGENTA("Synchronized set to TRUE (" << synchronized_connections_count << " of " << total_connections_count << " conn. synced)", LOG_LEVEL_0);
    }
    else if (!have_enough_synchronized_connections && m_synchronized)
    {
      LOG_PRINT_MAGENTA("Synchronized set to FALSE (" << synchronized_connections_count << " of " << total_connections_count << " conn. synced)", LOG_LEVEL_0);
      m_synchronized = false;
    }

    return m_core.on_idle();
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  int t_currency_protocol_handler<t_core>::handle_request_chain(int command, NOTIFY_REQUEST_CHAIN::request& arg, currency_connection_context& context)
  {
    //do not process requests if it comes from node wich is debugged
    if (m_debug_ip_address != 0 && context.m_remote_ip == m_debug_ip_address)
      return 1;

    LOG_PRINT_L2("[HANDLE]NOTIFY_REQUEST_CHAIN: block_ids.size()=" << arg.block_ids.size());
    LOG_PRINT_L3("[HANDLE]NOTIFY_REQUEST_CHAIN: " << print_kv_structure(arg));
    NOTIFY_RESPONSE_CHAIN_ENTRY::request r;
    if(!m_core.find_blockchain_supplement(arg.block_ids, r))
    {
      LOG_ERROR_CCONTEXT("Failed to handle NOTIFY_REQUEST_CHAIN.");
      return 1;
    }
    LOG_PRINT_L2("[NOTIFY]NOTIFY_RESPONSE_CHAIN_ENTRY: m_start_height=" << r.start_height << ", m_total_height=" << r.total_height << ", m_block_ids.size()=" << r.m_block_ids.size());
    LOG_PRINT_L3("[NOTIFY]NOTIFY_RESPONSE_CHAIN_ENTRY: " << print_kv_structure(r));
    post_notify<NOTIFY_RESPONSE_CHAIN_ENTRY>(r, context);
    return 1;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core> 
  bool t_currency_protocol_handler<t_core>::request_missing_objects(currency_connection_context& context, bool check_having_blocks)
  {
    if(context.m_priv.m_needed_objects.size())
    {
      //we know objects that we need, request this objects
      NOTIFY_REQUEST_GET_OBJECTS::request req;
      size_t count = 0;
      auto it = context.m_priv.m_needed_objects.begin();
      uint64_t requested_cumulative_size = 0;

      while (it != context.m_priv.m_needed_objects.end() && count < BLOCKS_SYNCHRONIZING_DEFAULT_COUNT && requested_cumulative_size < BLOCKS_SYNCHRONIZING_DEFAULT_SIZE)
      {
        if( !(check_having_blocks && m_core.have_block(it->h)))
        {
          req.blocks.push_back(it->h);
          requested_cumulative_size += it->cumul_size;
          ++count;
          context.m_priv.m_requested_objects.insert(it->h);
        }
        context.m_priv.m_needed_objects.erase(it++);
      }

      LOG_PRINT_L2("[NOTIFY]NOTIFY_REQUEST_GET_OBJECTS(req_missing): requested_cumulative_size=" << requested_cumulative_size << ", blocks.size()=" << req.blocks.size() << ", txs.size()=" << req.txs.size());
      LOG_PRINT_L3("[NOTIFY]NOTIFY_REQUEST_GET_OBJECTS(req_missing): " << ENDL << currency::print_kv_structure(req));
      post_notify<NOTIFY_REQUEST_GET_OBJECTS>(req, context);    
    }else if(context.m_last_response_height < context.m_remote_blockchain_height-1)
    {//we have to fetch more objects ids, request blockchain entry
     
      NOTIFY_REQUEST_CHAIN::request r = boost::value_initialized<NOTIFY_REQUEST_CHAIN::request>();
      m_core.get_short_chain_history(r.block_ids);
      LOG_PRINT_L2("[NOTIFY]NOTIFY_REQUEST_CHAIN: m_block_ids.size()=" << r.block_ids.size() );
      LOG_PRINT_L3("[NOTIFY]NOTIFY_REQUEST_CHAIN: " << ENDL << print_kv_structure(r) );
      post_notify<NOTIFY_REQUEST_CHAIN>(r, context);
    }else
    { 
      CHECK_AND_ASSERT_MES(context.m_last_response_height == context.m_remote_blockchain_height-1 
                           && !context.m_priv.m_needed_objects.size() 
                           && !context.m_priv.m_requested_objects.size(), false, "request_missing_blocks final condition failed!" 
                           << "\r\nm_last_response_height=" << context.m_last_response_height
                           << "\r\nm_remote_blockchain_height=" << context.m_remote_blockchain_height
                           << "\r\nm_needed_objects.size()=" << context.m_priv.m_needed_objects.size()
                           << "\r\nm_requested_objects.size()=" << context.m_priv.m_requested_objects.size()
                           << "\r\non connection [" << epee::net_utils::print_connection_context_short(context)<< "]");
      
      context.m_state = currency_connection_context::state_normal;
      LOG_PRINT_GREEN("[REQUEST_MISSING_OBJECTS]: SYNCHRONIZED OK", LOG_LEVEL_0);
      on_connection_synchronized();
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core> 
  bool t_currency_protocol_handler<t_core>::on_connection_synchronized()
  {
    bool val_expected = false;
    if (m_have_been_synchronized.compare_exchange_strong(val_expected, true))
    {
      m_core.on_synchronized();
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  void t_currency_protocol_handler<t_core>::relay_que_worker()
  {
    while (!m_want_stop)
    {
      std::list<relay_que_entry> local_que;
      {
        CRITICAL_REGION_LOCAL(m_relay_que_lock);
        local_que.swap(m_relay_que);
      }
      if (local_que.size())
        process_current_relay_que(local_que);
      
      epee::misc_utils::sleep_no_w(500);
    }
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  void t_currency_protocol_handler<t_core>::process_current_relay_que(const std::list<relay_que_entry>& que)
  {
    if (que.size() > 1){LOG_PRINT_MAGENTA("RELAY_QUE: " << que.size(), LOG_LEVEL_0);}

    TIME_MEASURE_START_MS(ms);
    std::stringstream debug_ss;
    std::list<connection_context> connections;
    m_p2p->get_connections(connections);
    for (auto& cc : connections)
    {
      NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::request req = AUTO_VAL_INIT(req);
      for (auto& qe : que)
      {
        //exclude relaying to original sender
        if (qe.second.m_connection_id == cc.m_connection_id)
          continue;
        req.txs.insert(req.txs.begin(), qe.first.txs.begin(), qe.first.txs.end());
      }
      if (req.txs.size())
      {
        post_notify<NOTIFY_OR_INVOKE_NEW_TRANSACTIONS>(req, cc);

        if (debug_ss.tellp())
          debug_ss << ", ";
        debug_ss << cc << ": " << req.txs.size();
      }
    }
    TIME_MEASURE_FINISH_MS(ms);
    LOG_PRINT_GREEN("[POST RELAY] NOTIFY_NEW_TRANSACTIONS relayed (" << ms << "ms) to: " << debug_ss.str(), LOG_LEVEL_2);
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core> 
  size_t t_currency_protocol_handler<t_core>::get_synchronized_connections_count()
  {
    size_t count = 0;
    m_p2p->for_each_connection([&](currency_connection_context& context, nodetool::peerid_type peer_id)->bool{
      if (context.m_state == currency_connection_context::state_normal)
        ++count;
      return true;
    });
    return count;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  size_t t_currency_protocol_handler<t_core>::get_synchronizing_connections_count()
  {
    size_t count = 0;
    m_p2p->for_each_connection([&](currency_connection_context& context, nodetool::peerid_type peer_id)->bool{
      if (context.m_state == currency_connection_context::state_synchronizing)
        ++count;
      return true;
    });
    return count;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  int64_t t_currency_protocol_handler<t_core>::get_net_time_delta_median()
  {
    std::vector<int64_t> deltas;
    m_p2p->for_each_connection([&](currency_connection_context& context, nodetool::peerid_type peer_id)->bool{
      deltas.push_back(context.m_time_delta);
      return true;
    });

    return epee::misc_utils::median(deltas);
  }
  //------------------------------------------------------------------------------------------------------------------------
  #define TIME_SYNC_DELTA_RING_BUFFER_SIZE         8
  #define TIME_SYNC_DELTA_TO_LOCAL_MAX_DIFFERENCE  (60 * 5)               // max acceptable difference between time delta median among peers and local time (seconds)
  #define TIME_SYNC_NTP_TO_LOCAL_MAX_DIFFERENCE    (60 * 5)               // max acceptable difference between NTP time and local time (seconds)

  template<class t_core>
  bool t_currency_protocol_handler<t_core>::add_time_delta_and_check_time_sync(int64_t time_delta)
  {
    CRITICAL_REGION_LOCAL(m_time_deltas_lock);

    auto get_core_time = [this] { return m_core.get_blockchain_storage().get_core_runtime_config().get_core_time(); };

    m_time_deltas.push_back(time_delta);
    while (m_time_deltas.size() > TIME_SYNC_DELTA_RING_BUFFER_SIZE)
      m_time_deltas.pop_front();

    if (m_time_deltas.size() < TIME_SYNC_DELTA_RING_BUFFER_SIZE)
      return true; // not enough data
   
    std::vector<int64_t> time_deltas_copy(m_time_deltas.begin(), m_time_deltas.end());

    m_last_median2local_time_difference = epee::misc_utils::median(time_deltas_copy);
    LOG_PRINT_MAGENTA("TIME: network time difference is " << m_last_median2local_time_difference << " (max is " << TIME_SYNC_DELTA_TO_LOCAL_MAX_DIFFERENCE << ")", ((m_last_median2local_time_difference >= 3) ? LOG_LEVEL_2 : LOG_LEVEL_3));
    if (std::abs(m_last_median2local_time_difference) > TIME_SYNC_DELTA_TO_LOCAL_MAX_DIFFERENCE)
    {
      // treat as error getting ntp time
      if (m_disable_ntp)
      {
        LOG_PRINT_RED("TIME: network time difference is " << m_last_median2local_time_difference << " (max is " << TIME_SYNC_DELTA_TO_LOCAL_MAX_DIFFERENCE << ") while NTP is disabled", LOG_LEVEL_0);
        return false;
      }
      int64_t ntp_time = tools::get_ntp_time();
      LOG_PRINT_L2("NTP: received time " << ntp_time << " (" << epee::misc_utils::get_time_str_v2(ntp_time) << "), diff: " << std::showpos << get_core_time() - ntp_time);
      if (ntp_time == 0)
      {
        // error geting ntp time
        LOG_PRINT_RED("TIME: network time difference is " << m_last_median2local_time_difference << " (max is " << TIME_SYNC_DELTA_TO_LOCAL_MAX_DIFFERENCE << ") but NTP servers did not respond", LOG_LEVEL_0);
        return false;
      }

      // got ntp time correctly
      // update local time, because getting ntp time could be time consuming
      uint64_t local_time_2 = get_core_time();
      m_last_ntp2local_time_difference = local_time_2 - ntp_time;
      if (std::abs(m_last_ntp2local_time_difference) > TIME_SYNC_NTP_TO_LOCAL_MAX_DIFFERENCE)
      {
        // local time is out of sync
        LOG_PRINT_RED("TIME: network time difference is " << m_last_median2local_time_difference << " (max is " << TIME_SYNC_DELTA_TO_LOCAL_MAX_DIFFERENCE << "), NTP time difference is " <<
          m_last_ntp2local_time_difference << " (max is " << TIME_SYNC_NTP_TO_LOCAL_MAX_DIFFERENCE << ")", LOG_LEVEL_0);
        return false;
      }

      // NTP time is OK
      LOG_PRINT_YELLOW("TIME: network time difference is " << m_last_median2local_time_difference << " (max is " << TIME_SYNC_DELTA_TO_LOCAL_MAX_DIFFERENCE << "), NTP time difference is " <<
        m_last_ntp2local_time_difference << " (max is " << TIME_SYNC_NTP_TO_LOCAL_MAX_DIFFERENCE << ")", LOG_LEVEL_1);
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core>
  bool t_currency_protocol_handler<t_core>::get_last_time_sync_difference(int64_t& last_median2local_time_difference, int64_t& last_ntp2local_time_difference)
  {
    CRITICAL_REGION_LOCAL(m_time_deltas_lock);
    last_median2local_time_difference = m_last_median2local_time_difference;
    last_ntp2local_time_difference = m_last_ntp2local_time_difference;

    return !(std::abs(m_last_median2local_time_difference) > TIME_SYNC_DELTA_TO_LOCAL_MAX_DIFFERENCE && std::abs(m_last_ntp2local_time_difference) > TIME_SYNC_NTP_TO_LOCAL_MAX_DIFFERENCE);
  }
  template<class t_core>
  void t_currency_protocol_handler<t_core>::set_to_debug_mode(uint32_t ip)
  {
    m_debug_ip_address = ip;
    LOG_PRINT_L0("debug mode is set for IP " << epst::get_ip_string_from_int32(m_debug_ip_address));
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core> 
  int t_currency_protocol_handler<t_core>::handle_response_chain_entry(int command, NOTIFY_RESPONSE_CHAIN_ENTRY::request& arg, currency_connection_context& context)
  {
    //do not process requests if it comes from node wich is debugged
    if (m_debug_ip_address != 0 && context.m_remote_ip == m_debug_ip_address)
      return 1;

    LOG_PRINT_L2("[HANDLE]NOTIFY_RESPONSE_CHAIN_ENTRY: m_block_ids.size()=" << arg.m_block_ids.size() 
      << ", m_start_height=" << arg.start_height << ", m_total_height=" << arg.total_height);
    LOG_PRINT_L3("[HANDLE]NOTIFY_RESPONSE_CHAIN_ENTRY: " << ENDL << currency::print_kv_structure(arg));
    
    if(!arg.m_block_ids.size())
    {
      LOG_ERROR_CCONTEXT("sent empty m_block_ids, dropping connection");
      m_p2p->drop_connection(context);
      m_p2p->add_ip_fail(context.m_remote_ip);
      return 1;
    }

    if(!m_core.have_block(arg.m_block_ids.front().h))
    {
      LOG_ERROR_CCONTEXT("sent m_block_ids starting from unknown id: "
                                              << epst::pod_to_hex(arg.m_block_ids.front()) << " , dropping connection");
      m_p2p->drop_connection(context);
      m_p2p->add_ip_fail(context.m_remote_ip);
      return 1;
    }
    
    context.m_remote_blockchain_height = arg.total_height;
    context.m_last_response_height = arg.start_height + arg.m_block_ids.size()-1;
    if(context.m_last_response_height > context.m_remote_blockchain_height)
    {
      LOG_ERROR_CCONTEXT("sent wrong NOTIFY_RESPONSE_CHAIN_ENTRY, with \r\nm_total_height=" << arg.total_height
                                                                         << "\r\nm_start_height=" << arg.start_height
                                                                         << "\r\nm_block_ids.size()=" << arg.m_block_ids.size());
      m_p2p->drop_connection(context);
      m_p2p->add_ip_fail(context.m_remote_ip);
    }

    BOOST_FOREACH(auto& bl_details, arg.m_block_ids)
    {
      if (!m_core.have_block(bl_details.h))
        context.m_priv.m_needed_objects.push_back(bl_details);
    }

    request_missing_objects(context, false);
    return 1;
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core> 
  bool t_currency_protocol_handler<t_core>::relay_block(NOTIFY_NEW_BLOCK::request& arg, currency_connection_context& exclude_context)
  {
    return relay_post_notify<NOTIFY_NEW_BLOCK>(arg, exclude_context);
  }
  //------------------------------------------------------------------------------------------------------------------------
  template<class t_core> 
  bool t_currency_protocol_handler<t_core>::relay_transactions(NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::request& arg, currency_connection_context& exclude_context)
    {
#ifdef ASYNC_RELAY_MODE
    {
      CRITICAL_REGION_LOCAL(m_relay_que_lock);
      m_relay_que.push_back(AUTO_VAL_INIT(relay_que_entry()));
      m_relay_que.back().first = arg;
      m_relay_que.back().second = exclude_context;
    }
    //m_relay_que_cv.notify_all();
    return true;
#else 
    return relay_post_notify<NOTIFY_OR_INVOKE_NEW_TRANSACTIONS>(arg, exclude_context);
#endif
  }
}
