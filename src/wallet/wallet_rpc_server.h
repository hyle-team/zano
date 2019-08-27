// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma  once 

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include "net/http_server_impl_base.h"
#include "wallet_public_structs_defs.h"
#include "wallet2.h"
#include "common/command_line.h"
namespace tools
{
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class wallet_rpc_server: public epee::http_server_impl_base<wallet_rpc_server>
  {
  public:
    typedef epee::net_utils::connection_context_base connection_context;

    wallet_rpc_server(wallet2& cr);

    const static command_line::arg_descriptor<std::string> arg_rpc_bind_port;
    const static command_line::arg_descriptor<std::string> arg_rpc_bind_ip;
    const static command_line::arg_descriptor<std::string> arg_miner_text_info;
    const static command_line::arg_descriptor<bool>        arg_deaf_mode;


    static void init_options(boost::program_options::options_description& desc);
    bool init(const boost::program_options::variables_map& vm);
    bool run(bool do_mint, bool offline_mode);

    
    bool handle_http_request(const epee::net_utils::http::http_request_info& query_info, epee::net_utils::http::http_response_info& response, connection_context& m_conn_context);

    BEGIN_URI_MAP2()
      BEGIN_JSON_RPC_MAP("/json_rpc")
        MAP_JON_RPC_WE("getbalance",                on_getbalance,                wallet_public::COMMAND_RPC_GET_BALANCE)
        MAP_JON_RPC_WE("getaddress",                on_getaddress,                wallet_public::COMMAND_RPC_GET_ADDRESS)
        MAP_JON_RPC_WE("transfer",                  on_transfer,                  wallet_public::COMMAND_RPC_TRANSFER)
        MAP_JON_RPC_WE("store",                     on_store,                     wallet_public::COMMAND_RPC_STORE)
        MAP_JON_RPC_WE("get_payments",              on_get_payments,              wallet_public::COMMAND_RPC_GET_PAYMENTS)
        MAP_JON_RPC_WE("get_bulk_payments",         on_get_bulk_payments,         wallet_public::COMMAND_RPC_GET_BULK_PAYMENTS)
        MAP_JON_RPC_WE("make_integrated_address",   on_make_integrated_address,   wallet_public::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS)
        MAP_JON_RPC_WE("split_integrated_address",  on_split_integrated_address,  wallet_public::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS)
        MAP_JON_RPC_WE("sign_transfer",             on_sign_transfer,             wallet_public::COMMAND_SIGN_TRANSFER)
        MAP_JON_RPC_WE("submit_transfer",           on_submit_transfer,           wallet_public::COMMAND_SUBMIT_TRANSFER)
        //market API
        MAP_JON_RPC_WE("contracts_send_proposal",             on_submit_contract_proposal,  wallet_public::COMMAND_SUBMIT_CONTRACT_PROPOSAL)
        MAP_JON_RPC_WE("contracts_accept_proposal",           on_submit_contract_accept,    wallet_public::COMMAND_SUBMIT_CONTRACT_ACCEPT)
        MAP_JON_RPC_WE("contracts_get_all",                   on_get_contracts,             wallet_public::COMMAND_GET_CONTRACTS)
        MAP_JON_RPC_WE("contracts_release",                   on_release_contract,          wallet_public::COMMAND_RELEASE_CONTRACT)
        MAP_JON_RPC_WE("contracts_request_cancel",            on_request_cancel_contract,   wallet_public::COMMAND_REQUEST_CANCEL_CONTRACT)
        MAP_JON_RPC_WE("contracts_accept_cancel",             on_accept_cancel_contract,    wallet_public::COMMAND_ACCEPT_CANCEL_CONTRACT)
      END_JSON_RPC_MAP()
    END_URI_MAP2()

      //json_rpc
      bool on_getbalance(const wallet_public::COMMAND_RPC_GET_BALANCE::request& req, wallet_public::COMMAND_RPC_GET_BALANCE::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_getaddress(const wallet_public::COMMAND_RPC_GET_ADDRESS::request& req, wallet_public::COMMAND_RPC_GET_ADDRESS::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_transfer(const wallet_public::COMMAND_RPC_TRANSFER::request& req, wallet_public::COMMAND_RPC_TRANSFER::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_store(const wallet_public::COMMAND_RPC_STORE::request& req, wallet_public::COMMAND_RPC_STORE::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_get_payments(const wallet_public::COMMAND_RPC_GET_PAYMENTS::request& req, wallet_public::COMMAND_RPC_GET_PAYMENTS::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_get_bulk_payments(const wallet_public::COMMAND_RPC_GET_BULK_PAYMENTS::request& req, wallet_public::COMMAND_RPC_GET_BULK_PAYMENTS::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_make_integrated_address(const wallet_public::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::request& req, wallet_public::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_split_integrated_address(const wallet_public::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::request& req, wallet_public::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_sign_transfer(const wallet_public::COMMAND_SIGN_TRANSFER::request& req, wallet_public::COMMAND_SIGN_TRANSFER::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_submit_transfer(const wallet_public::COMMAND_SUBMIT_TRANSFER::request& req, wallet_public::COMMAND_SUBMIT_TRANSFER::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_submit_contract_proposal(const wallet_public::COMMAND_SUBMIT_CONTRACT_PROPOSAL::request& req, wallet_public::COMMAND_SUBMIT_CONTRACT_PROPOSAL::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_submit_contract_accept(const wallet_public::COMMAND_SUBMIT_CONTRACT_ACCEPT::request& req, wallet_public::COMMAND_SUBMIT_CONTRACT_ACCEPT::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_get_contracts(const wallet_public::COMMAND_GET_CONTRACTS::request& req, wallet_public::COMMAND_GET_CONTRACTS::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_release_contract(const wallet_public::COMMAND_RELEASE_CONTRACT::request& req, wallet_public::COMMAND_RELEASE_CONTRACT::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_request_cancel_contract(const wallet_public::COMMAND_REQUEST_CANCEL_CONTRACT::request& req, wallet_public::COMMAND_REQUEST_CANCEL_CONTRACT::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_accept_cancel_contract(const wallet_public::COMMAND_ACCEPT_CANCEL_CONTRACT::request& req, wallet_public::COMMAND_ACCEPT_CANCEL_CONTRACT::response& res, epee::json_rpc::error& er, connection_context& cntx);


      bool handle_command_line(const boost::program_options::variables_map& vm);

  private:
      wallet2& m_wallet;
      std::string m_port;
      std::string m_bind_ip;
      bool m_do_mint;
      bool m_deaf;
  };

} // namespace tools
