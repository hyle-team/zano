// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma  once
#include "rpc/core_rpc_server.h"
#include "wallet/core_rpc_proxy.h"
#include "currency_core/alias_helper.h"
namespace tools
{
  class core_fast_rpc_proxy: public i_core_proxy
  {
  public:
    core_fast_rpc_proxy(currency::core_rpc_server& rpc_srv) :m_rpc(rpc_srv)
    {}
    //------------------------------------------------------------------------------------------------------------------------------
    virtual bool set_connection_addr(const std::string& url) override
    {
      return true;
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES(const currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& req, currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& res) override
    {
      return m_rpc.on_get_indexes(req, res, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_BLOCKS_FAST(const currency::COMMAND_RPC_GET_BLOCKS_FAST::request& req, currency::COMMAND_RPC_GET_BLOCKS_FAST::response& res) override
    {
      return m_rpc.on_get_blocks(req, res, m_cntxt_stub);
    }
    bool call_COMMAND_RPC_GET_BLOCKS_DIRECT(const currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request& rqt, currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response& rsp) override
    {
      return m_rpc.on_get_blocks_direct(rqt, rsp, m_cntxt_stub);
    }
    bool call_COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE(const currency::COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::request& rqt, currency::COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::response& rsp) override
    { 
      return m_rpc.on_get_est_height_from_date(rqt, rsp, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_INFO(const currency::COMMAND_RPC_GET_INFO::request& req, currency::COMMAND_RPC_GET_INFO::response& res) override
    {
      return m_rpc.on_get_info(req, res, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_TX_POOL(const currency::COMMAND_RPC_GET_TX_POOL::request& req, currency::COMMAND_RPC_GET_TX_POOL::response& res) override
    {
      return m_rpc.on_get_tx_pool(req, res, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_ALIASES_BY_ADDRESS(const currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::request& req, currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::response& res) override
    {
      return m_rpc.on_aliases_by_address(req, res, m_err_stub, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res) override
    {
      return m_rpc.on_get_random_outs1(req, res, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::request& req, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::response& res) override
    {
      return m_rpc.on_get_random_outs3(req, res, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_SEND_RAW_TX(const currency::COMMAND_RPC_SEND_RAW_TX::request& req, currency::COMMAND_RPC_SEND_RAW_TX::response& res) override
    {
      return m_rpc.on_send_raw_tx(req, res, m_cntxt_stub);
    }
		bool call_COMMAND_RPC_FORCE_RELAY_RAW_TXS(const currency::COMMAND_RPC_FORCE_RELAY_RAW_TXS::request& req, currency::COMMAND_RPC_FORCE_RELAY_RAW_TXS::response& res	) override
		{
			return m_rpc.on_force_relaey_raw_txs(req, res, m_cntxt_stub);
		}
    //------------------------------------------------------------------------------------------------------------------------------
    virtual bool check_connection() override
    {
      return true;
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_ALL_ALIASES(currency::COMMAND_RPC_GET_ALL_ALIASES::response& res) override
    {
      currency::COMMAND_RPC_GET_ALL_ALIASES::request req = AUTO_VAL_INIT(req);
      return m_rpc.on_get_all_aliases(req, res, m_err_stub, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_ALIAS_DETAILS(const currency::COMMAND_RPC_GET_ALIAS_DETAILS::request& req, currency::COMMAND_RPC_GET_ALIAS_DETAILS::response& res) override
    {
      return m_rpc.on_get_alias_details(req, res, m_err_stub, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_ALIAS_REWARD(const currency::COMMAND_RPC_GET_ALIAS_REWARD::request& req, currency::COMMAND_RPC_GET_ALIAS_REWARD::response& res) override
    { 
      return m_rpc.on_get_alias_reward(req, res, m_err_stub, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_TRANSACTIONS(const currency::COMMAND_RPC_GET_TRANSACTIONS::request& req, currency::COMMAND_RPC_GET_TRANSACTIONS::response& rsp) override
    {
      return m_rpc.on_get_transactions(req, rsp, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_COMMAND_RPC_CHECK_KEYIMAGES(const currency::COMMAND_RPC_CHECK_KEYIMAGES::request& req, currency::COMMAND_RPC_CHECK_KEYIMAGES::response& rsp) override
    {
      return m_rpc.on_check_keyimages(req, rsp, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GETBLOCKTEMPLATE(const currency::COMMAND_RPC_GETBLOCKTEMPLATE::request& req, currency::COMMAND_RPC_GETBLOCKTEMPLATE::response& rsp) override
    {
      return m_rpc.on_getblocktemplate(req, rsp, m_err_stub, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_SUBMITBLOCK(const currency::COMMAND_RPC_SUBMITBLOCK::request& req, currency::COMMAND_RPC_SUBMITBLOCK::response& rsp) override
    {
      return m_rpc.on_submitblock(req, rsp, m_err_stub, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_SUBMITBLOCK2(const currency::COMMAND_RPC_SUBMITBLOCK2::request& req, currency::COMMAND_RPC_SUBMITBLOCK2::response& rsp) override
    {
      return m_rpc.on_submitblock2(req, rsp, m_err_stub, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_POS_MINING_DETAILS(const currency::COMMAND_RPC_GET_POS_MINING_DETAILS::request& req, currency::COMMAND_RPC_GET_POS_MINING_DETAILS::response& rsp) override
    {
      return m_rpc.on_get_pos_mining_details(req, rsp, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_BLOCKS_DETAILS(const currency::COMMAND_RPC_GET_BLOCKS_DETAILS::request& req, currency::COMMAND_RPC_GET_BLOCKS_DETAILS::response& res) override
    {
      return m_rpc.on_rpc_get_blocks_details(req, res, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN(const currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::request& req, currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::response& res) override
    {
      return m_rpc.on_get_current_core_tx_expiration_median(req, res, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    bool call_COMMAND_RPC_GET_POOL_INFO(const currency::COMMAND_RPC_GET_POOL_INFO::request& req, currency::COMMAND_RPC_GET_POOL_INFO::response& res) override
    {
      return m_rpc.on_get_pool_info(req, res, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    virtual bool call_COMMAND_RPC_GET_ASSET_INFO(const currency::COMMAND_RPC_GET_ASSET_INFO::request& req, currency::COMMAND_RPC_GET_ASSET_INFO::response& res)override
    {
      return m_rpc.on_get_asset_info(req, res, m_cntxt_stub);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    virtual bool call_COMMAND_RPC_INVOKE(const std::string& uri, const std::string& body, int& response_code, std::string& response_body) override
    {
      epee::net_utils::http::http_request_info query_info = AUTO_VAL_INIT(query_info);
      query_info.m_URI = uri;
      query_info.m_http_method = epee::net_utils::http::http_method_get;
      query_info.m_body = body;

      epee::net_utils::http::http_response_info response = AUTO_VAL_INIT(response);
      epee::net_utils::connection_context_base conn_context = AUTO_VAL_INIT(conn_context);

      bool res = m_rpc.handle_http_request(query_info, response, conn_context);
      response_body = response.m_body;
      response_code = response.m_response_code;
      return res;
    }
    //------------------------------------------------------------------------------------------------------------------------------
    virtual bool get_transfer_address(const std::string& adr_str, currency::account_public_address& addr, std::string& payment_id) override
    {
      return tools::get_transfer_address(adr_str, addr, payment_id, this);
    }
    //------------------------------------------------------------------------------------------------------------------------------
    virtual time_t get_last_success_interract_time() override
    { 
      return time(nullptr); 
    }

  private:
    currency::core_rpc_server& m_rpc;
    currency::core_rpc_server::connection_context m_cntxt_stub;
    epee::json_rpc::error m_err_stub;
  };

}



