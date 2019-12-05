// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "core_default_rpc_proxy.h"
using namespace epee;
#include "storages/http_abstract_invoke.h"
#include "currency_core/currency_format_utils.h"
#include "currency_core/alias_helper.h"

namespace tools
{
  bool default_http_core_proxy::set_connection_addr(const std::string& url)
  {
    m_daemon_address = url;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES(const currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& req, currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& res)
  {
    return invoke_http_bin_remote_command2_update_is_disconnect("/get_o_indexes.bin", req, res);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_BLOCKS_FAST(const currency::COMMAND_RPC_GET_BLOCKS_FAST::request& req, currency::COMMAND_RPC_GET_BLOCKS_FAST::response& res)
  {
    return invoke_http_bin_remote_command2_update_is_disconnect("/getblocks.bin", req, res);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_BLOCKS_DIRECT(const currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request& rqt, currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response& rsp)
  {
    currency::COMMAND_RPC_GET_BLOCKS_FAST::request req;
    req.block_ids = rqt.block_ids;
    currency::COMMAND_RPC_GET_BLOCKS_FAST::response res = AUTO_VAL_INIT(res);
    bool r = call_COMMAND_RPC_GET_BLOCKS_FAST(req, res);
    rsp.status = res.status;
    if (rsp.status == CORE_RPC_STATUS_OK)
    {
      rsp.current_height = res.current_height;
      rsp.start_height = res.start_height;
      r = unserialize_block_complete_entry(res, rsp);
    }
    return r;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_INFO(const currency::COMMAND_RPC_GET_INFO::request& req, currency::COMMAND_RPC_GET_INFO::response& res)
  {
    return invoke_http_json_remote_command2_update_is_disconnect("/getinfo", req, res);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_TX_POOL(const currency::COMMAND_RPC_GET_TX_POOL::request& req, currency::COMMAND_RPC_GET_TX_POOL::response& res)
  {
    return invoke_http_bin_remote_command2_update_is_disconnect("/get_tx_pool.bin", req, res);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_ALIASES_BY_ADDRESS(const currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::request& req, currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::response& res)
  {
    return invoke_http_json_rpc_update_is_disconnect("get_alias_by_address", req, res);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res)
  {
    return invoke_http_bin_remote_command2_update_is_disconnect("/getrandom_outs.bin", req, res);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_SEND_RAW_TX(const currency::COMMAND_RPC_SEND_RAW_TX::request& req, currency::COMMAND_RPC_SEND_RAW_TX::response& res)
  {
    return invoke_http_json_remote_command2_update_is_disconnect("/sendrawtransaction", req, res);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_FORCE_RELAY_RAW_TXS(const currency::COMMAND_RPC_FORCE_RELAY_RAW_TXS::request& req, currency::COMMAND_RPC_FORCE_RELAY_RAW_TXS::response& res)
  {
    return invoke_http_json_remote_command2_update_is_disconnect("/force_relay", req, res);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_TRANSACTIONS(const currency::COMMAND_RPC_GET_TRANSACTIONS::request& req, currency::COMMAND_RPC_GET_TRANSACTIONS::response& rsp)
  {
    return invoke_http_json_remote_command2_update_is_disconnect("/gettransactions", req, rsp);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_COMMAND_RPC_CHECK_KEYIMAGES(const currency::COMMAND_RPC_CHECK_KEYIMAGES::request& req, currency::COMMAND_RPC_CHECK_KEYIMAGES::response& rsp)
  {
    return invoke_http_bin_remote_command2_update_is_disconnect("/check_keyimages.bin", req, rsp);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_SCAN_POS(const currency::COMMAND_RPC_SCAN_POS::request& req, currency::COMMAND_RPC_SCAN_POS::response& rsp)
  {
    return invoke_http_bin_remote_command2_update_is_disconnect("/scan_pos.bin", req, rsp);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_POS_MINING_DETAILS(const currency::COMMAND_RPC_GET_POS_MINING_DETAILS::request& req, currency::COMMAND_RPC_GET_POS_MINING_DETAILS::response& rsp)
  {
    return invoke_http_bin_remote_command2_update_is_disconnect("/get_pos_details.bin", req, rsp);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_BLOCKS_DETAILS(const currency::COMMAND_RPC_GET_BLOCKS_DETAILS::request& req, currency::COMMAND_RPC_GET_BLOCKS_DETAILS::response& res)
  {
    //return epee::net_utils::invoke_http_bin_remote_command2(m_daemon_address + "/get_blocks_details.bin", req, res, m_http_client, WALLET_RCP_CONNECTION_TIMEOUT);
    return invoke_http_json_rpc_update_is_disconnect("get_blocks_details", req, res);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN(const currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::request& req, currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::response& res)
  {
    return invoke_http_json_rpc_update_is_disconnect("get_current_core_tx_expiration_median", req, res);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GETBLOCKTEMPLATE(const currency::COMMAND_RPC_GETBLOCKTEMPLATE::request& req, currency::COMMAND_RPC_GETBLOCKTEMPLATE::response& rsp)
  {
    return invoke_http_json_rpc_update_is_disconnect("getblocktemplate", req, rsp);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_SUBMITBLOCK(const currency::COMMAND_RPC_SUBMITBLOCK::request& req, currency::COMMAND_RPC_SUBMITBLOCK::response& rsp)
  {
    return invoke_http_json_rpc_update_is_disconnect("submitblock", req, rsp);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_SUBMITBLOCK2(const currency::COMMAND_RPC_SUBMITBLOCK2::request& req, currency::COMMAND_RPC_SUBMITBLOCK2::response& rsp)
  {
    return invoke_http_json_rpc_update_is_disconnect("submitblock2", req, rsp);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::check_connection()
  {
    CRITICAL_REGION_LOCAL(m_lock);
    if (m_http_client.is_connected())
      return true;

    epee::net_utils::http::url_content u;
    epee::net_utils::parse_url(m_daemon_address, u);
    if (!u.port)
      u.port = 8081;
    return m_http_client.connect(u.host, std::to_string(u.port), WALLET_RCP_CONNECTION_TIMEOUT);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_ALL_ALIASES(currency::COMMAND_RPC_GET_ALL_ALIASES::response& res)
  {
    currency::COMMAND_RPC_GET_ALL_ALIASES::request req = AUTO_VAL_INIT(req);
    return invoke_http_json_rpc_update_is_disconnect("get_all_alias_details", req, res);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_ALIAS_DETAILS(const currency::COMMAND_RPC_GET_ALIAS_DETAILS::request& req, currency::COMMAND_RPC_GET_ALIAS_DETAILS::response& res)
  {
    return invoke_http_json_rpc_update_is_disconnect("get_alias_details", req, res);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_ALIAS_REWARD(const currency::COMMAND_RPC_GET_ALIAS_REWARD::request& req, currency::COMMAND_RPC_GET_ALIAS_REWARD::response& rsp)
  {
    return invoke_http_json_rpc_update_is_disconnect("get_alias_reward", req, rsp);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::call_COMMAND_RPC_GET_POOL_INFO(const currency::COMMAND_RPC_GET_POOL_INFO::request& req, currency::COMMAND_RPC_GET_POOL_INFO::response& res)
  {
    return invoke_http_json_rpc_update_is_disconnect("get_pool_info", req, res);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool default_http_core_proxy::get_transfer_address(const std::string& adr_str, currency::account_public_address& addr, std::string& payment_id)
  {
    return tools::get_transfer_address(adr_str, addr, payment_id, this);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  void default_http_core_proxy::set_plast_daemon_is_disconnected(std::atomic<bool> *plast_daemon_is_disconnected)
  {
    CRITICAL_REGION_LOCAL(m_lock);
    m_plast_daemon_is_disconnected = plast_daemon_is_disconnected ? plast_daemon_is_disconnected : &m_last_daemon_is_disconnected_stub;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  default_http_core_proxy::default_http_core_proxy():m_plast_daemon_is_disconnected(&m_last_daemon_is_disconnected_stub)
  {

  }
}

