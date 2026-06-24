// Copyright (c) 2014-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <string>

#include "net/http_server_impl_base.h"
#include "net/http_client.h"
#include "storages/http_abstract_invoke.h"
#include "misc_language.h"
#include "syncobj.h"
#include "crypto/crypto.h"
#include "currency_core/currency_basic.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "rpc/core_rpc_server_commands_defs_wallet_ext.h"

#define GATEWAY_RPC_PROXY_ACCESS_TOKEN              "Zano-Access-Token"
#define GATEWAY_RPC_PROXY_DEFAULT_RPC_PORT          12333
#define GATEWAY_RPC_PROXY_DEFAULT_RPC_THREADS       2

namespace gateway_rpc_proxy
{
  class gateway_rpc_proxy_server : public epee::http_server_impl_base<gateway_rpc_proxy_server>, public epee::net_utils::http::i_chain_handler
  {
  public:
    typedef epee::net_utils::connection_context_base connection_context;

    gateway_rpc_proxy_server();

    static void init_options(boost::program_options::options_description& desc);
    bool init(const boost::program_options::variables_map& vm);

    virtual bool handle_http_request(const epee::net_utils::http::http_request_info& query_info, epee::net_utils::http::http_response_info& response_info, connection_context& conn_context) override
    {
      bool call_found = false;
      return this->handle_http_request(query_info, response_info, conn_context, call_found, epee::net_utils::http::i_chain_handler::m_empty_documentation);
    }

    virtual bool handle_http_request(const epee::net_utils::http::http_request_info& query_info, epee::net_utils::http::http_response_info& response_info, epee::net_utils::connection_context_base& conn_context,
      bool& call_found, documentation& docs = epee::net_utils::http::i_chain_handler::m_empty_documentation) override;

    bool on_gateway_get_address_history(const currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::request& req, currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::response& res, connection_context& cntx);

    BEGIN_URI_MAP2_VIRTUAL()
      BEGIN_JSON_RPC_MAP("/json_rpc")
        MAP_JON_RPC("gateway_get_address_history", on_gateway_get_address_history, currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY)
      END_JSON_RPC_MAP()
    END_URI_MAP2()

  private:
    bool handle_command_line(const boost::program_options::variables_map& vm);
    bool load_view_secret_key(const boost::program_options::variables_map& vm);
    bool ensure_daemon_connection();
    bool forward_keyless_history(const currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::request& req, currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::response& res);
    bool auth_http_request(const epee::net_utils::http::http_request_info& query_info,  epee::net_utils::http::http_response_info& response, connection_context& conn_context);

    std::string m_bind_ip;
    std::string m_port;

    // upstream zanod
    std::string m_daemon_address;
    epee::critical_section m_daemon_lock;
    epee::net_utils::http::http_universal_client m_daemon_client;
    unsigned int m_daemon_timeout;
    size_t m_daemon_attempts;

    // the secret
    crypto::secret_key m_gateway_view_secret_key;

    // auth
    std::string m_jwt_secret;
    bool m_unsecure_no_auth;
    epee::critical_section m_jwt_lock;
    epee::misc_utils::expirating_set<std::string, uint64_t> m_jwt_used_salts;
  };
}
