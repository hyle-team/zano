// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#pragma once
#include "include_base_utils.h"
#include "net/http_client.h"
#include "core_rpc_proxy.h"
#include "storages/http_abstract_invoke.h"

#define WALLET_RCP_CONNECTION_TIMEOUT                          200000
#define WALLET_RCP_COUNT_ATTEMNTS                              3

namespace tools
{
  class default_http_core_proxy final : public i_core_proxy
  {
  public:


    bool set_connection_addr(const std::string& url) override;
    bool call_COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES(const currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& rqt, currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& rsp) override;
    bool call_COMMAND_RPC_GET_BLOCKS_FAST(const currency::COMMAND_RPC_GET_BLOCKS_FAST::request& rqt, currency::COMMAND_RPC_GET_BLOCKS_FAST::response& rsp) override;
    bool call_COMMAND_RPC_GET_BLOCKS_DIRECT(const currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request& rqt, currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response& rsp) override;
    bool call_COMMAND_RPC_GET_INFO(const currency::COMMAND_RPC_GET_INFO::request& rqt, currency::COMMAND_RPC_GET_INFO::response& rsp) override;
    bool call_COMMAND_RPC_GET_TX_POOL(const currency::COMMAND_RPC_GET_TX_POOL::request& rqt, currency::COMMAND_RPC_GET_TX_POOL::response& rsp) override;
    bool call_COMMAND_RPC_GET_ALIASES_BY_ADDRESS(const currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::request& rqt, currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::response& rsp) override;
    bool call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& rqt, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& rsp) override;
    bool call_COMMAND_RPC_SEND_RAW_TX(const currency::COMMAND_RPC_SEND_RAW_TX::request& rqt, currency::COMMAND_RPC_SEND_RAW_TX::response& rsp) override;
    bool call_COMMAND_RPC_FORCE_RELAY_RAW_TXS(const currency::COMMAND_RPC_FORCE_RELAY_RAW_TXS::request& rqt, currency::COMMAND_RPC_FORCE_RELAY_RAW_TXS::response& rsp) override;
    bool call_COMMAND_RPC_GET_ALL_ALIASES(currency::COMMAND_RPC_GET_ALL_ALIASES::response& rsp) override;
    bool call_COMMAND_RPC_GET_ALIAS_DETAILS(const currency::COMMAND_RPC_GET_ALIAS_DETAILS::request& req, currency::COMMAND_RPC_GET_ALIAS_DETAILS::response& rsp) override;
    bool call_COMMAND_RPC_GET_ALIAS_REWARD(const currency::COMMAND_RPC_GET_ALIAS_REWARD::request& req, currency::COMMAND_RPC_GET_ALIAS_REWARD::response& rsp) override;
    bool call_COMMAND_RPC_GET_TRANSACTIONS(const currency::COMMAND_RPC_GET_TRANSACTIONS::request& req, currency::COMMAND_RPC_GET_TRANSACTIONS::response& rsp) override;
    bool call_COMMAND_RPC_COMMAND_RPC_CHECK_KEYIMAGES(const currency::COMMAND_RPC_CHECK_KEYIMAGES::request& req, currency::COMMAND_RPC_CHECK_KEYIMAGES::response& rsp) override;
    bool call_COMMAND_RPC_SCAN_POS(const currency::COMMAND_RPC_SCAN_POS::request& req, currency::COMMAND_RPC_SCAN_POS::response& rsp) override;
    bool call_COMMAND_RPC_GETBLOCKTEMPLATE(const currency::COMMAND_RPC_GETBLOCKTEMPLATE::request& req, currency::COMMAND_RPC_GETBLOCKTEMPLATE::response& rsp) override;
    bool call_COMMAND_RPC_SUBMITBLOCK(const currency::COMMAND_RPC_SUBMITBLOCK::request& req, currency::COMMAND_RPC_SUBMITBLOCK::response& rsp) override;
    bool call_COMMAND_RPC_SUBMITBLOCK2(const currency::COMMAND_RPC_SUBMITBLOCK2::request& req, currency::COMMAND_RPC_SUBMITBLOCK2::response& rsp) override;
    bool call_COMMAND_RPC_GET_POS_MINING_DETAILS(const currency::COMMAND_RPC_GET_POS_MINING_DETAILS::request& req, currency::COMMAND_RPC_GET_POS_MINING_DETAILS::response& rsp) override;
    bool call_COMMAND_RPC_GET_BLOCKS_DETAILS(const currency::COMMAND_RPC_GET_BLOCKS_DETAILS::request& req, currency::COMMAND_RPC_GET_BLOCKS_DETAILS::response& res) override;
    bool call_COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN(const currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::request& req, currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::response& res) override;
    bool call_COMMAND_RPC_GET_POOL_INFO(const currency::COMMAND_RPC_GET_POOL_INFO::request& req, currency::COMMAND_RPC_GET_POOL_INFO::response& res) override;

    bool check_connection() override;
    bool get_transfer_address(const std::string& adr_str, currency::account_public_address& addr, std::string& payment_id) override;

    void set_plast_daemon_is_disconnected(std::atomic<bool> *plast_daemon_is_disconnected);
    default_http_core_proxy();
  private:

    template <class t_method>
    bool call_request(t_method request)
    {
      CRITICAL_REGION_LOCAL(m_lock);

      bool ret = false;
      for(size_t i = WALLET_RCP_COUNT_ATTEMNTS; i && !ret; --i)
      {
        ret = request();
      }

      m_plast_daemon_is_disconnected->store(!ret);
      return ret;
    }

    template<class t_request, class t_response>
    inline bool invoke_http_json_rpc_update_is_disconnect(const std::string& method_name, const t_request& req, t_response& res)
    {
      return call_request([&](){
        return epee::net_utils::invoke_http_json_rpc("/json_rpc", method_name, req, res, m_http_client);
      });
    }

    template<class t_request, class t_response>
    inline bool invoke_http_bin_remote_command2_update_is_disconnect(const std::string& url, const t_request& req, t_response& res)
    {
      return call_request([&](){
        return epee::net_utils::invoke_http_bin_remote_command2(m_daemon_address + url, req, res, m_http_client, WALLET_RCP_CONNECTION_TIMEOUT);
      });
    }

    template<class t_request, class t_response>
    inline bool invoke_http_json_remote_command2_update_is_disconnect(const std::string& url, const t_request& req, t_response& res)
    {
      return call_request([&](){
        return epee::net_utils::invoke_http_json_remote_command2(m_daemon_address + url, req, res, m_http_client, WALLET_RCP_CONNECTION_TIMEOUT);
      });
    }

    epee::critical_section m_lock;
    epee::net_utils::http::http_simple_client m_http_client;
    std::string m_daemon_address;
    std::atomic<bool> *m_plast_daemon_is_disconnected;
    std::atomic<bool> m_last_daemon_is_disconnected_stub;

  };
}

