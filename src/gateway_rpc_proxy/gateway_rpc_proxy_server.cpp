// Copyright (c) 2014-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "include_base_utils.h"
using namespace epee;

#include <cstdlib>
#include <algorithm>

#include "gateway_rpc_proxy_server.h"
#include "syncobj.h"
#include "string_tools.h"
#include "file_io_utils.h"
#include "net/http_base.h"
#include "net/net_parse_helpers.h"
#include "crypto/bitcoin/sha256_helper.h"
#include "common/command_line.h"
#include "common/error_codes.h"
#include "currency_core/currency_basic.h"
#include "currency_core/currency_format_utils.h"
#include "currency_core/currency_format_utils_transactions.h"
#include "jwt-cpp/jwt.h"

#undef LOG_DEFAULT_CHANNEL
#define LOG_DEFAULT_CHANNEL "gateway_rpc_proxy"
ENABLE_CHANNEL_BY_DEFAULT("gateway_rpc_proxy");

#define GWP_JWT_TOKEN_EXPIRATION_MAXIMUM          (60 * 60 * 1000)
#define GWP_JWT_TOKEN_CLAIM_NAME_BODY_HASH        "body_hash"
#define GWP_JWT_TOKEN_CLAIM_NAME_SALT             "salt"
#define GWP_JWT_TOKEN_OVERWHELM_LIMIT             100000

#define GWP_DAEMON_CONNECTION_TIMEOUT             30000   // ms, gateway history can be sizeable
#define GWP_DAEMON_ATTEMPTS                       3
#define GWP_DAEMON_DEFAULT_PORT                   RPC_DEFAULT_PORT

namespace gateway_rpc_proxy
{
  namespace
  {
    const command_line::arg_descriptor<std::string> arg_rpc_bind_ip            ("rpc-bind-ip",   "Specify IP to bind the gateway RPC proxy server", "127.0.0.1");
    const command_line::arg_descriptor<std::string> arg_rpc_bind_port          ("rpc-bind-port", "Specify port for the gateway RPC proxy server", std::to_string(GATEWAY_RPC_PROXY_DEFAULT_RPC_PORT));
    const command_line::arg_descriptor<std::string> arg_daemon_address         ("daemon-address","Upstream zanod RPC address used to fetch keyless gateway history", "http://127.0.0.1:" + std::to_string(GWP_DAEMON_DEFAULT_PORT));
    const command_line::arg_descriptor<std::string> arg_gw_view_secret_key_file("gateway-view-secret-key-file", "Path to a file (restricted permissions) containing the gateway view secret key as 64 hex chars. Alternatively set the ZANO_GATEWAY_VIEW_SECRET_KEY env var.", "");
    const command_line::arg_descriptor<std::string> arg_jwt_secret             ("jwt-secret",    "Enable JWT (HS256) auth over the provided secret string", "");
    const command_line::arg_descriptor<bool>        arg_unsecure_no_auth       ("unsecure-no-auth", "Acknowledge running the gateway RPC proxy server without authentication");

    bool is_loopback_ip(const std::string& ip)
    {
      return ip == "127.0.0.1" || ip == "::1" || ip == "localhost" || (ip.size() >= 4 && ip.compare(0, 4, "127.") == 0);
    }
  }

  gateway_rpc_proxy_server::gateway_rpc_proxy_server()
    : m_daemon_timeout(GWP_DAEMON_CONNECTION_TIMEOUT)
    , m_daemon_attempts(GWP_DAEMON_ATTEMPTS)
    , m_gateway_view_secret_key(currency::null_skey)
    , m_unsecure_no_auth(false)
  {}

  //------------------------------------------------------------------------------------------------------------------------------
  void gateway_rpc_proxy_server::init_options(boost::program_options::options_description& desc)
  {
    command_line::add_arg(desc, arg_rpc_bind_ip);
    command_line::add_arg(desc, arg_rpc_bind_port);
    command_line::add_arg(desc, arg_daemon_address);
    command_line::add_arg(desc, arg_gw_view_secret_key_file);
    command_line::add_arg(desc, arg_jwt_secret);
    command_line::add_arg(desc, arg_unsecure_no_auth);
  }

  //------------------------------------------------------------------------------------------------------------------------------
  bool gateway_rpc_proxy_server::handle_command_line(const boost::program_options::variables_map& vm)
  {
    m_bind_ip        = command_line::get_arg(vm, arg_rpc_bind_ip);
    m_port           = command_line::get_arg(vm, arg_rpc_bind_port);
    m_daemon_address = command_line::get_arg(vm, arg_daemon_address);
    m_jwt_secret     = command_line::get_arg(vm, arg_jwt_secret);
    m_unsecure_no_auth = command_line::get_arg(vm, arg_unsecure_no_auth);
    return true;
  }

  //------------------------------------------------------------------------------------------------------------------------------
  bool gateway_rpc_proxy_server::load_view_secret_key(const boost::program_options::variables_map& vm)
  {
    // never accept the key as a plain CLI arg, only from a premission restricted file or env var
    std::string hex;
    std::string key_file = command_line::get_arg(vm, arg_gw_view_secret_key_file);
    if (!key_file.empty())
    {
      if (!epee::file_io_utils::load_file_to_string(key_file, hex))
      {
        LOG_ERROR("gateway_rpc_proxy: cannot read gateway view secret key file: " << key_file);
        return false;
      }
    }
    else
    {
      const char* env = std::getenv("ZANO_GATEWAY_VIEW_SECRET_KEY");
      if (env != nullptr)
        hex = env;
    }

    epee::string_tools::trim(hex);
    if (hex.empty())
    {
      LOG_ERROR("gateway_rpc_proxy: gateway view secret key not provided. Use --gateway-view-secret-key-file <path> or set ZANO_GATEWAY_VIEW_SECRET_KEY.");
      return false;
    }

    m_gateway_view_secret_key = currency::null_skey;
    bool parsed = epee::string_tools::hex_to_pod(hex, m_gateway_view_secret_key);
    // wipe the source buffer regardless of outcome
    std::fill(hex.begin(), hex.end(), '\0');
    hex.clear();

    if (!parsed)
    {
      LOG_ERROR("gateway_rpc_proxy: gateway view secret key must be exactly 64 hex characters."); // do NOT log the value
      return false;
    }
    if (m_gateway_view_secret_key == currency::null_skey)
    {
      LOG_ERROR("gateway_rpc_proxy: gateway view secret key is null/zero, refusing to start.");
      return false;
    }
    crypto::public_key pub = AUTO_VAL_INIT(pub);
    if (!crypto::secret_key_to_public_key(m_gateway_view_secret_key, pub))
    {
      LOG_ERROR("gateway_rpc_proxy: gateway view secret key is not a valid scalar, refusing to start.");
      return false;
    }
    return true;
  }

  //------------------------------------------------------------------------------------------------------------------------------
  bool gateway_rpc_proxy_server::init(const boost::program_options::variables_map& vm)
  {
    m_net_server.set_threads_prefix("GWP-RPC");

    if (!handle_command_line(vm))
    {
      LOG_ERROR("gateway_rpc_proxy: failed to process command line");
      return false;
    }
    if (!load_view_secret_key(vm))
      return false;

    if (!is_loopback_ip(m_bind_ip) && m_jwt_secret.empty() && !m_unsecure_no_auth)
    {
      LOG_PRINT_RED("gateway_rpc_proxy: refusing to bind RPC to non-loopback ip '" << m_bind_ip << "' without authentication." << ENDL
        << "Use --jwt-secret <secret> to enable JWT auth, bind to 127.0.0.1, or pass --unsecure-no-auth to acknowledge the risk.", LOG_LEVEL_0);
      return false;
    }
    if (m_jwt_secret.empty())
    {
      LOG_PRINT_L0("gateway_rpc_proxy: RPC authentication is DISABLED. Use --jwt-secret <secret> to enable JWT auth.");
    }
    else
    {
      LOG_PRINT_L0("gateway_rpc_proxy: JWT authentication is ENABLED.");
    }

    LOG_PRINT_L0("gateway_rpc_proxy: upstream daemon address: " << m_daemon_address);

    bool r = epee::http_server_impl_base<gateway_rpc_proxy_server, connection_context>::init(m_port, m_bind_ip);
    return r;
  }

  //------------------------------------------------------------------------------------------------------------------------------
  bool gateway_rpc_proxy_server::auth_http_request(const epee::net_utils::http::http_request_info& query_info, epee::net_utils::http::http_response_info& response, connection_context& conn_context)
  {
    auto it = std::find_if(query_info.m_header_info.m_etc_fields.begin(), query_info.m_header_info.m_etc_fields.end(), [](const auto& element)
    {
      return !epee::string_tools::compare_no_case(element.first, GATEWAY_RPC_PROXY_ACCESS_TOKEN); 
    });

    if (it == query_info.m_header_info.m_etc_fields.end())
      return false;

    try
    {
      CRITICAL_REGION_LOCAL(m_jwt_lock);

      if (m_jwt_used_salts.get_set().size() > GWP_JWT_TOKEN_OVERWHELM_LIMIT)
        throw std::runtime_error("Salt is overwhelmed");

      auto decoded = jwt::decode(it->second, [](const std::string& str)
      {
        return jwt::base::decode<jwt::alphabet::base64>(jwt::base::pad<jwt::alphabet::base64>(str));
      });

      auto verifier = jwt::verify().allow_algorithm(jwt::algorithm::hs256{ m_jwt_secret });
      verifier.verify(decoded);

      std::string body_hash = decoded.get_payload_claim(GWP_JWT_TOKEN_CLAIM_NAME_BODY_HASH).as_string();
      std::string salt      = decoded.get_payload_claim(GWP_JWT_TOKEN_CLAIM_NAME_SALT).as_string();

      crypto::hash jwt_claim_sha256 = currency::null_hash;
      epee::string_tools::hex_to_pod(body_hash, jwt_claim_sha256);
      crypto::hash sha256 = crypto::sha256_hash(query_info.m_body.data(), query_info.m_body.size());
      if (sha256 != jwt_claim_sha256)
        throw std::runtime_error("Body hash mismatch");

      if (m_jwt_used_salts.get_set().find(salt) != m_jwt_used_salts.get_set().end())
        throw std::runtime_error("Salt reused");

      uint64_t ticks_now = epee::misc_utils::get_tick_count();
      m_jwt_used_salts.add(salt, ticks_now + GWP_JWT_TOKEN_EXPIRATION_MAXIMUM);
      m_jwt_used_salts.remove_if_expiration_less_than(ticks_now);

      LOG_PRINT_L3("JWT token OK");
      return true;
    }
    catch (const std::exception& e)
    {
      LOG_ERROR("Invalid JWT token: " << e.what());
      return false;
    }
  }

  //------------------------------------------------------------------------------------------------------------------------------
  bool gateway_rpc_proxy_server::handle_http_request(const epee::net_utils::http::http_request_info& query_info, epee::net_utils::http::http_response_info& response, epee::net_utils::connection_context_base& conn_context, bool& call_found, documentation& docs)
  {
    if (m_jwt_secret.size())
    {
      if (!auth_http_request(query_info, response, conn_context))
      {
        call_found = true;
        response.m_response_code    = 401;
        response.m_response_comment = "Unauthorized";
        return true;
      }
    }

    response.m_response_code = 200;
    response.m_response_comment = "Ok";
    if (!handle_http_request_map(query_info, response, conn_context, call_found, docs) && response.m_response_code == 200)
    {
      response.m_response_code = 500;
      response.m_response_comment = "Internal Server Error";
      return true;
    }
    if (!call_found)
    {
      response.m_response_code = 404;
      response.m_response_comment = "Not Found";
      return true;
    }
    return true;
  }

  //------------------------------------------------------------------------------------------------------------------------------
  bool gateway_rpc_proxy_server::ensure_daemon_connection()
  {
    CRITICAL_REGION_LOCAL(m_daemon_lock);
    if (m_daemon_client.is_connected())
      return true;

    epee::net_utils::http::url_content u = AUTO_VAL_INIT(u);
    if (!epee::net_utils::parse_url(m_daemon_address, u) || u.host.empty())
    {
      LOG_ERROR("gateway_rpc_proxy: failed to parse daemon address: " << m_daemon_address);
      return false;
    }
    if (!u.port)
      u.port = GWP_DAEMON_DEFAULT_PORT;
    m_daemon_client.set_is_ssl(u.schema == "https");
    if (!m_daemon_client.connect(u.host, std::to_string(u.port), m_daemon_timeout))
    {
      LOG_ERROR("gateway_rpc_proxy: cannot connect to daemon " << u.host << ":" << u.port);
      return false;
    }
    return true;
  }

  //------------------------------------------------------------------------------------------------------------------------------
  bool gateway_rpc_proxy_server::forward_keyless_history(const currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::request& req, currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::response& res)
  {
    CRITICAL_REGION_LOCAL(m_daemon_lock);

    currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::request d_req = AUTO_VAL_INIT(d_req);
    d_req.gateway_address = req.gateway_address;
    d_req.offset          = req.offset;
    d_req.count           = req.count;
    d_req.gateway_view_secret_key.reset();

    bool ok = false;
    for (size_t i = m_daemon_attempts; i && !ok; --i)
    {
      if (!ensure_daemon_connection())
        continue;
      ok = epee::net_utils::invoke_http_json_rpc("/json_rpc", "gateway_get_address_history", d_req, res, m_daemon_client, m_daemon_timeout);
      if (!ok)
        m_daemon_client.disconnect();
    }
    if (!ok)
      LOG_ERROR("gateway_rpc_proxy: gateway_get_address_history forward failed after " << m_daemon_attempts << " attempt(s) to " << m_daemon_address);
    return ok;
  }

  //------------------------------------------------------------------------------------------------------------------------------
  bool gateway_rpc_proxy_server::on_gateway_get_address_history(const currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::request& req, currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::response& res, connection_context& cntx)
  {
    currency::gateway_address_id_type addr_id = {};
    currency::address_v v_addr = {};
    currency::payment_id_t dummy_payment_id = {};
    if (!currency::get_account_address_and_payment_id_from_str(v_addr, dummy_payment_id, req.gateway_address))
    {
      res.status = API_RETURN_CODE_BAD_ARG_INVALID_ADDRESS;
      return true;
    }
    if (v_addr.type() != typeid(currency::gateway_address_id_type))
    {
      res.status = API_RETURN_CODE_BAD_ARG_INVALID_ADDRESS_TYPE;
      return true;
    }
    addr_id = boost::get<currency::gateway_address_id_type>(v_addr);

    if (!forward_keyless_history(req, res))
    {
      res = currency::COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY::response();
      res.status = API_RETURN_CODE_INTERNAL_ERROR;
      res.status_error = "gateway_rpc_proxy: failed to query upstream daemon";
      return true;
    }
    if (res.status != API_RETURN_CODE_OK)
    {
      res.raw_txs.clear();
      return true;
    }

    if (res.transactions.size() != res.raw_txs.size())
    {
      LOG_ERROR("gateway_rpc_proxy: daemon returned " << res.transactions.size() << " transactions but " << res.raw_txs.size()
        << " raw_txs; cannot decrypt (is the daemon up to date - does it return raw_txs for a keyless gateway_get_address_history?)");
      res.transactions.clear();
      res.raw_txs.clear();
      res.status = API_RETURN_CODE_INTERNAL_ERROR;
      res.status_error = "gateway_rpc_proxy: upstream daemon did not return raw txs required for decryption";
      return true;
    }

    // decrypt each entry locally using the held gateway view secret key
    auto raw_it = res.raw_txs.begin();
    for (auto& wti : res.transactions)
    {
      const std::string& raw_hex = *raw_it++;
      std::string blob;
      if (!epee::string_tools::parse_hexstr_to_binbuff(raw_hex, blob))
      {
        res.transactions.clear(); res.raw_txs.clear();
        res.status = API_RETURN_CODE_INTERNAL_ERROR;
        res.status_error = "gateway_rpc_proxy: invalid raw tx hex received from daemon";
        return true;
      }
      currency::transaction tx = AUTO_VAL_INIT(tx);
      if (!currency::parse_and_validate_tx_from_blob(blob, tx))
      {
        res.transactions.clear(); res.raw_txs.clear();
        res.status = API_RETURN_CODE_INTERNAL_ERROR;
        res.status_error = "gateway_rpc_proxy: failed to parse raw tx received from daemon";
        return true;
      }
      wti.tx = tx;
      if (!currency::gateway_decrypt_wti(m_gateway_view_secret_key, addr_id, wti))
      {
        LOG_ERROR("gateway_rpc_proxy: failed to decrypt gateway tx " << wti.tx_hash);
        res.transactions.clear(); res.raw_txs.clear();
        res.status = API_RETURN_CODE_INTERNAL_ERROR;
        res.status_error = "gateway_rpc_proxy: failed to decrypt gateway transaction details";
        return true;
      }
    }

    res.raw_txs.clear();
    res.status = API_RETURN_CODE_OK;
    return true;
  }
}
