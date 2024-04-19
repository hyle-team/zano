// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma  once 

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include "net/http_server_impl_base.h"
#include "core_rpc_server_commands_defs.h"
#include "currency_core/currency_core.h"
#include "p2p/net_node.h"
#include "currency_protocol/currency_protocol_handler.h"
#include "currency_core/bc_offers_service.h"


  
#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL "rpc"
ENABLE_CHANNEL_BY_DEFAULT("rpc");

namespace currency
{
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class core_rpc_server: public epee::http_server_impl_base<core_rpc_server>
  {
  public:
    typedef epee::net_utils::connection_context_base connection_context;

    core_rpc_server(core& cr, nodetool::node_server<currency::t_currency_protocol_handler<currency::core> >& p2p, bc_services::bc_offers_service& of);

    static void init_options(boost::program_options::options_description& desc);
    bool init(const boost::program_options::variables_map& vm);
    
    void set_rpc_chain_handler(epee::net_utils::http::i_chain_handler* prpc_chain_handler) { m_prpc_chain_handler = prpc_chain_handler; }
    bool on_get_blocks_direct(const COMMAND_RPC_GET_BLOCKS_DIRECT::request& req, COMMAND_RPC_GET_BLOCKS_DIRECT::response& res, connection_context& cntx);
    

    bool on_get_height(const COMMAND_RPC_GET_HEIGHT::request& req, COMMAND_RPC_GET_HEIGHT::response& res, connection_context& cntx);
    bool on_get_blocks(const COMMAND_RPC_GET_BLOCKS_FAST::request& req, COMMAND_RPC_GET_BLOCKS_FAST::response& res, connection_context& cntx);
    bool on_get_transactions(const COMMAND_RPC_GET_TRANSACTIONS::request& req, COMMAND_RPC_GET_TRANSACTIONS::response& res, connection_context& cntx);
    bool on_get_indexes(const COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& req, COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& res, connection_context& cntx);
    bool on_send_raw_tx(const COMMAND_RPC_SEND_RAW_TX::request& req, COMMAND_RPC_SEND_RAW_TX::response& res, connection_context& cntx);		
    bool on_start_mining(const COMMAND_RPC_START_MINING::request& req, COMMAND_RPC_START_MINING::response& res, connection_context& cntx);
    bool on_stop_mining(const COMMAND_RPC_STOP_MINING::request& req, COMMAND_RPC_STOP_MINING::response& res, connection_context& cntx);
    bool on_get_random_outs(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_LEGACY::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_LEGACY::response& res, connection_context& cntx);
    bool on_get_random_outs1(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res, connection_context& cntx);
    bool on_get_random_outs3(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::response& res, connection_context& cntx);
    bool on_get_info(const COMMAND_RPC_GET_INFO::request& req, COMMAND_RPC_GET_INFO::response& res, connection_context& cntx);
    bool on_set_maintainers_info(const COMMAND_RPC_SET_MAINTAINERS_INFO::request& req, COMMAND_RPC_SET_MAINTAINERS_INFO::response& res, connection_context& cntx);
    bool on_get_tx_pool(const COMMAND_RPC_GET_TX_POOL::request& req, COMMAND_RPC_GET_TX_POOL::response& res, connection_context& cntx);
    bool on_check_keyimages(const COMMAND_RPC_CHECK_KEYIMAGES::request& req, COMMAND_RPC_CHECK_KEYIMAGES::response& res, connection_context& cntx);
    bool on_rpc_get_blocks_details(const COMMAND_RPC_GET_BLOCKS_DETAILS::request& req, COMMAND_RPC_GET_BLOCKS_DETAILS::response& res, connection_context& cntx);
		bool on_force_relaey_raw_txs(const COMMAND_RPC_FORCE_RELAY_RAW_TXS::request& req, COMMAND_RPC_FORCE_RELAY_RAW_TXS::response& res, connection_context& cntx);
    bool on_get_offers_ex(const COMMAND_RPC_GET_OFFERS_EX::request& req, COMMAND_RPC_GET_OFFERS_EX::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    

    //json_rpc
    bool on_getblockcount(const COMMAND_RPC_GETBLOCKCOUNT::request& req, COMMAND_RPC_GETBLOCKCOUNT::response& res, connection_context& cntx);
    bool on_getblockhash(const COMMAND_RPC_GETBLOCKHASH::request& req, COMMAND_RPC_GETBLOCKHASH::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_getblocktemplate(const COMMAND_RPC_GETBLOCKTEMPLATE::request& req, COMMAND_RPC_GETBLOCKTEMPLATE::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_submitblock(const COMMAND_RPC_SUBMITBLOCK::request& req, COMMAND_RPC_SUBMITBLOCK::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_submitblock2(const COMMAND_RPC_SUBMITBLOCK2::request& req, COMMAND_RPC_SUBMITBLOCK2::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_get_last_block_header(const COMMAND_RPC_GET_LAST_BLOCK_HEADER::request& req, COMMAND_RPC_GET_LAST_BLOCK_HEADER::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_get_block_header_by_hash(const COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::request& req, COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_get_block_header_by_height(const COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::request& req, COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_get_alias_details(const COMMAND_RPC_GET_ALIAS_DETAILS::request& req, COMMAND_RPC_GET_ALIAS_DETAILS::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_get_all_aliases(const COMMAND_RPC_GET_ALL_ALIASES::request& req, COMMAND_RPC_GET_ALL_ALIASES::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_get_aliases(const COMMAND_RPC_GET_ALIASES::request& req, COMMAND_RPC_GET_ALIASES::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_aliases_by_address(const COMMAND_RPC_GET_ALIASES_BY_ADDRESS::request& req, COMMAND_RPC_GET_ALIASES_BY_ADDRESS::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_get_alias_reward(const COMMAND_RPC_GET_ALIAS_REWARD::request& req, COMMAND_RPC_GET_ALIAS_REWARD::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);  
    bool on_reset_transaction_pool(const COMMAND_RPC_RESET_TX_POOL::request& req, COMMAND_RPC_RESET_TX_POOL::response& res, connection_context& cntx);
    bool on_remove_tx_from_pool(const COMMAND_RPC_REMOVE_TX_FROM_POOL::request& req, COMMAND_RPC_REMOVE_TX_FROM_POOL::response& res, connection_context& cntx);
    bool on_get_pos_mining_details(const COMMAND_RPC_GET_POS_MINING_DETAILS::request& req, COMMAND_RPC_GET_POS_MINING_DETAILS::response& res, connection_context& cntx);
    bool on_get_current_core_tx_expiration_median(const COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::request& req, COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::response& res, connection_context& cntx);
    bool on_get_tx_details(const COMMAND_RPC_GET_TX_DETAILS::request& req, COMMAND_RPC_GET_TX_DETAILS::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_search_by_id(const COMMAND_RPC_SERARCH_BY_ID::request& req, COMMAND_RPC_SERARCH_BY_ID::response& res, connection_context& cntx);
    bool on_get_out_info(const COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES_BY_AMOUNT::request& req, COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES_BY_AMOUNT::response& res, connection_context& cntx);
    bool on_get_multisig_info(const COMMAND_RPC_GET_MULTISIG_INFO::request& req, COMMAND_RPC_GET_MULTISIG_INFO::response& res, connection_context& cntx);
    bool on_get_pool_txs_details(const COMMAND_RPC_GET_POOL_TXS_DETAILS::request& req, COMMAND_RPC_GET_POOL_TXS_DETAILS::response& res, connection_context& cntx);
    bool on_get_pool_txs_brief_details(const COMMAND_RPC_GET_POOL_TXS_BRIEF_DETAILS::request& req, COMMAND_RPC_GET_POOL_TXS_BRIEF_DETAILS::response& res, connection_context& cntx);
    bool on_get_all_pool_tx_list(const COMMAND_RPC_GET_ALL_POOL_TX_LIST::request& req, COMMAND_RPC_GET_ALL_POOL_TX_LIST::response& res, connection_context& cntx);
    bool on_get_pool_info(const COMMAND_RPC_GET_POOL_INFO::request& req, COMMAND_RPC_GET_POOL_INFO::response& res, connection_context& cntx);
    bool on_get_votes(const COMMAND_RPC_GET_VOTES::request& req, COMMAND_RPC_GET_VOTES::response& res, connection_context& cntx);
    bool on_get_asset_info(const COMMAND_RPC_GET_ASSET_INFO::request& req, COMMAND_RPC_GET_ASSET_INFO::response& res, connection_context& cntx);
    bool on_get_assets_list(const COMMAND_RPC_GET_ASSETS_LIST::request& req, COMMAND_RPC_GET_ASSETS_LIST::response& res, connection_context& cntx);

    bool on_get_main_block_details(const COMMAND_RPC_GET_BLOCK_DETAILS::request& req, COMMAND_RPC_GET_BLOCK_DETAILS::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_get_alt_block_details(const COMMAND_RPC_GET_BLOCK_DETAILS::request& req, COMMAND_RPC_GET_BLOCK_DETAILS::response& res, epee::json_rpc::error& error_resp, connection_context& cntx);
    bool on_get_alt_blocks_details(const COMMAND_RPC_GET_ALT_BLOCKS_DETAILS::request& req, COMMAND_RPC_GET_ALT_BLOCKS_DETAILS::response& res, connection_context& cntx);
    bool on_get_est_height_from_date(const COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::request& req, COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::response& res, connection_context& cntx);
    bool on_validate_signature(const COMMAND_VALIDATE_SIGNATURE::request& req, COMMAND_VALIDATE_SIGNATURE::response& res, epee::json_rpc::error& er, connection_context& cntx);
    
    
    
    CHAIN_HTTP_TO_MAP2(connection_context); //forward http requests to uri map

    BEGIN_URI_MAP2()
      // legacy JSON-alike RPCs
      MAP_URI_AUTO_JON2("/getheight",                 on_get_height,                  COMMAND_RPC_GET_HEIGHT)
      MAP_URI_AUTO_JON2("/gettransactions",           on_get_transactions,            COMMAND_RPC_GET_TRANSACTIONS)
      MAP_URI_AUTO_JON2("/sendrawtransaction",        on_send_raw_tx,                 COMMAND_RPC_SEND_RAW_TX)
			MAP_URI_AUTO_JON2("/force_relay",               on_force_relaey_raw_txs,        COMMAND_RPC_FORCE_RELAY_RAW_TXS)
      MAP_URI_AUTO_JON2("/start_mining",              on_start_mining,                COMMAND_RPC_START_MINING)
      MAP_URI_AUTO_JON2("/stop_mining",               on_stop_mining,                 COMMAND_RPC_STOP_MINING)
      MAP_URI_AUTO_JON2("/getinfo",                   on_get_info,                    COMMAND_RPC_GET_INFO)
      // binary RPCs
      MAP_URI_AUTO_BIN2("/getblocks.bin",             on_get_blocks,                  COMMAND_RPC_GET_BLOCKS_FAST)
      MAP_URI_AUTO_BIN2("/get_o_indexes.bin",         on_get_indexes,                 COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES)      
      MAP_URI_AUTO_BIN2("/getrandom_outs.bin",        on_get_random_outs,             COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_LEGACY)
      MAP_URI_AUTO_BIN2("/getrandom_outs1.bin",       on_get_random_outs1,            COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS)
      MAP_URI_AUTO_BIN2("/getrandom_outs3.bin",       on_get_random_outs3,            COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3)
      MAP_URI_AUTO_BIN2("/set_maintainers_info.bin",  on_set_maintainers_info,        COMMAND_RPC_SET_MAINTAINERS_INFO)
      MAP_URI_AUTO_BIN2("/get_tx_pool.bin",           on_get_tx_pool,                 COMMAND_RPC_GET_TX_POOL)
      MAP_URI_AUTO_BIN2("/check_keyimages.bin",       on_check_keyimages,             COMMAND_RPC_CHECK_KEYIMAGES) 
      MAP_URI_AUTO_BIN2("/get_pos_details.bin",       on_get_pos_mining_details,      COMMAND_RPC_GET_POS_MINING_DETAILS)
      // JSON RPCs
      BEGIN_JSON_RPC_MAP("/json_rpc")
        MAP_JON_RPC   ("getblockcount",               on_getblockcount,               COMMAND_RPC_GETBLOCKCOUNT)
        MAP_JON_RPC_WE("on_getblockhash",             on_getblockhash,                COMMAND_RPC_GETBLOCKHASH)
        MAP_JON_RPC_WE("getblocktemplate",            on_getblocktemplate,            COMMAND_RPC_GETBLOCKTEMPLATE)
        MAP_JON_RPC_WE("submitblock",                 on_submitblock,                 COMMAND_RPC_SUBMITBLOCK)
        MAP_JON_RPC_WE("submitblock2",                on_submitblock2,                COMMAND_RPC_SUBMITBLOCK2)
        MAP_JON_RPC_WE("getlastblockheader",          on_get_last_block_header,       COMMAND_RPC_GET_LAST_BLOCK_HEADER)
        MAP_JON_RPC_WE("getblockheaderbyhash",        on_get_block_header_by_hash,    COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH)
        MAP_JON_RPC_WE("getblockheaderbyheight",      on_get_block_header_by_height,  COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT)
        MAP_JON_RPC_WE("get_alias_details",           on_get_alias_details,           COMMAND_RPC_GET_ALIAS_DETAILS)
        MAP_JON_RPC_WE("get_alias_by_address",        on_aliases_by_address,          COMMAND_RPC_GET_ALIASES_BY_ADDRESS)
        MAP_JON_RPC_WE("get_alias_reward",            on_get_alias_reward,            COMMAND_RPC_GET_ALIAS_REWARD)
        MAP_JON_RPC   ("get_est_height_from_date",    on_get_est_height_from_date,    COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE)
        //block explorer api
        MAP_JON_RPC   ("get_blocks_details",          on_rpc_get_blocks_details,      COMMAND_RPC_GET_BLOCKS_DETAILS)
        MAP_JON_RPC_WE("get_tx_details",              on_get_tx_details,              COMMAND_RPC_GET_TX_DETAILS)
        MAP_JON_RPC   ("search_by_id",                on_search_by_id,                COMMAND_RPC_SERARCH_BY_ID)
        MAP_JON_RPC   ("getinfo",                     on_get_info ,                   COMMAND_RPC_GET_INFO)
        MAP_JON_RPC   ("get_out_info",                on_get_out_info,                COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES_BY_AMOUNT)
        MAP_JON_RPC   ("get_multisig_info",           on_get_multisig_info,           COMMAND_RPC_GET_MULTISIG_INFO)
        MAP_JON_RPC_WE("get_all_alias_details",       on_get_all_aliases,             COMMAND_RPC_GET_ALL_ALIASES)
        MAP_JON_RPC_WE("get_aliases",                 on_get_aliases,                 COMMAND_RPC_GET_ALIASES)
        MAP_JON_RPC   ("get_pool_txs_details",        on_get_pool_txs_details,        COMMAND_RPC_GET_POOL_TXS_DETAILS)
        MAP_JON_RPC   ("get_pool_txs_brief_details",  on_get_pool_txs_brief_details,  COMMAND_RPC_GET_POOL_TXS_BRIEF_DETAILS)
        MAP_JON_RPC   ("get_all_pool_tx_list",        on_get_all_pool_tx_list,        COMMAND_RPC_GET_ALL_POOL_TX_LIST)
        MAP_JON_RPC   ("get_pool_info",               on_get_pool_info,               COMMAND_RPC_GET_POOL_INFO)
        MAP_JON_RPC   ("getrandom_outs",              on_get_random_outs,             COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_LEGACY)
        MAP_JON_RPC   ("getrandom_outs1",             on_get_random_outs1,            COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS)
        MAP_JON_RPC   ("getrandom_outs3",             on_get_random_outs3,            COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3)
        MAP_JON_RPC   ("get_votes",                   on_get_votes,                   COMMAND_RPC_GET_VOTES)
        //assets api
        MAP_JON_RPC   ("get_asset_info",              on_get_asset_info,             COMMAND_RPC_GET_ASSET_INFO)
        MAP_JON_RPC   ("get_assets_list",             on_get_assets_list,            COMMAND_RPC_GET_ASSETS_LIST)

        MAP_JON_RPC_WE("get_main_block_details",      on_get_main_block_details,      COMMAND_RPC_GET_BLOCK_DETAILS)
        MAP_JON_RPC_WE("get_alt_block_details",       on_get_alt_block_details,       COMMAND_RPC_GET_BLOCK_DETAILS)
        MAP_JON_RPC   ("get_alt_blocks_details",      on_get_alt_blocks_details,      COMMAND_RPC_GET_ALT_BLOCKS_DETAILS)
        //
        MAP_JON_RPC   ("reset_transaction_pool",      on_reset_transaction_pool,      COMMAND_RPC_RESET_TX_POOL)
        MAP_JON_RPC   ("remove_tx_from_pool",         on_remove_tx_from_pool,         COMMAND_RPC_REMOVE_TX_FROM_POOL)
        MAP_JON_RPC   ("get_current_core_tx_expiration_median", on_get_current_core_tx_expiration_median, COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN)
        //
        MAP_JON_RPC_WE("marketplace_global_get_offers_ex", on_get_offers_ex,          COMMAND_RPC_GET_OFFERS_EX)        
        MAP_JON_RPC_WE("validate_signature",          on_validate_signature,          COMMAND_VALIDATE_SIGNATURE)
        CHAIN_TO_PHANDLER(m_prpc_chain_handler)
      END_JSON_RPC_MAP()
      CHAIN_TO_PHANDLER(m_prpc_chain_handler)
    END_URI_MAP2()
  
  private:

    //-----------------------
    bool handle_command_line(const boost::program_options::variables_map& vm);
    bool check_core_ready_(const std::string& calling_method);

    //utils
    uint64_t get_block_reward(const block& blk);
    bool fill_block_header_response(const block& blk, bool orphan_status, block_header_response& response);
    void set_session_blob(const std::string& session_id, const currency::block& blob);
    bool get_session_blob(const std::string& session_id, currency::block& blob);
    
    core& m_core;
    nodetool::node_server<currency::t_currency_protocol_handler<currency::core> >& m_p2p;
    bc_services::bc_offers_service& m_of;
    std::string m_port;
    std::string m_bind_ip;
    bool m_ignore_status;
    epee::net_utils::http::i_chain_handler* m_prpc_chain_handler = nullptr;
  };
}

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL NULL
