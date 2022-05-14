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
    bool run(bool do_mint, bool offline_mode, const currency::account_public_address& miner_address);
    bool handle_http_request(const epee::net_utils::http::http_request_info& query_info, epee::net_utils::http::http_response_info& response, connection_context& m_conn_context);

    BEGIN_URI_MAP2()
      BEGIN_JSON_RPC_MAP("/json_rpc")
        MAP_JON_RPC_WE("getbalance",                on_getbalance,                wallet_public::COMMAND_RPC_GET_BALANCE)
        MAP_JON_RPC_WE("getaddress",                on_getaddress,                wallet_public::COMMAND_RPC_GET_ADDRESS)
        MAP_JON_RPC_WE("get_wallet_info",           on_getwallet_info,            wallet_public::COMMAND_RPC_GET_WALLET_INFO)
        MAP_JON_RPC_WE("get_recent_txs_and_info",   on_get_recent_txs_and_info,   wallet_public::COMMAND_RPC_GET_RECENT_TXS_AND_INFO)
        MAP_JON_RPC_WE("transfer",                  on_transfer,                  wallet_public::COMMAND_RPC_TRANSFER)
        MAP_JON_RPC_WE("store",                     on_store,                     wallet_public::COMMAND_RPC_STORE)
        MAP_JON_RPC_WE("get_payments",              on_get_payments,              wallet_public::COMMAND_RPC_GET_PAYMENTS)
        MAP_JON_RPC_WE("get_bulk_payments",         on_get_bulk_payments,         wallet_public::COMMAND_RPC_GET_BULK_PAYMENTS)
        MAP_JON_RPC_WE("make_integrated_address",   on_make_integrated_address,   wallet_public::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS)
        MAP_JON_RPC_WE("split_integrated_address",  on_split_integrated_address,  wallet_public::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS)
        MAP_JON_RPC_WE("sweep_below",               on_sweep_below,               wallet_public::COMMAND_SWEEP_BELOW)
        MAP_JON_RPC_WE("sign_transfer",             on_sign_transfer,             wallet_public::COMMAND_SIGN_TRANSFER)
        MAP_JON_RPC_WE("submit_transfer",           on_submit_transfer,           wallet_public::COMMAND_SUBMIT_TRANSFER)
        MAP_JON_RPC_WE("search_for_transactions",   on_search_for_transactions,   wallet_public::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS)
        MAP_JON_RPC_WE("get_restore_info",          on_getwallet_restore_info,    wallet_public::COMMAND_RPC_GET_WALLET_RESTORE_INFO)
        MAP_JON_RPC_WE("get_seed_phrase_info",      on_get_seed_phrase_info,      wallet_public::COMMAND_RPC_GET_SEED_PHRASE_INFO)
        MAP_JON_RPC_WE("get_mining_history",        on_get_mining_history,        wallet_public::COMMAND_RPC_GET_MINING_HISTORY)
        //contracts API
        MAP_JON_RPC_WE("contracts_send_proposal",             on_contracts_send_proposal,      wallet_public::COMMAND_CONTRACTS_SEND_PROPOSAL)
        MAP_JON_RPC_WE("contracts_accept_proposal",           on_contracts_accept_proposal,    wallet_public::COMMAND_CONTRACTS_ACCEPT_PROPOSAL)
        MAP_JON_RPC_WE("contracts_get_all",                   on_contracts_get_all,            wallet_public::COMMAND_CONTRACTS_GET_ALL)
        MAP_JON_RPC_WE("contracts_release",                   on_contracts_release,            wallet_public::COMMAND_CONTRACTS_RELEASE)
        MAP_JON_RPC_WE("contracts_request_cancel",            on_contracts_request_cancel,     wallet_public::COMMAND_CONTRACTS_REQUEST_CANCEL)
        MAP_JON_RPC_WE("contracts_accept_cancel",             on_contracts_accept_cancel,      wallet_public::COMMAND_CONTRACTS_ACCEPT_CANCEL)
        //marketplace API
        MAP_JON_RPC_WE("marketplace_get_offers_ex",           on_marketplace_get_my_offers,     wallet_public::COMMAND_MARKETPLACE_GET_MY_OFFERS)
        MAP_JON_RPC_WE("marketplace_push_offer",              on_marketplace_push_offer,        wallet_public::COMMAND_MARKETPLACE_PUSH_OFFER)
        MAP_JON_RPC_WE("marketplace_push_update_offer",       on_marketplace_push_update_offer, wallet_public::COMMAND_MARKETPLACE_PUSH_UPDATE_OFFER)
        MAP_JON_RPC_WE("marketplace_cancel_offer",            on_marketplace_cancel_offer,      wallet_public::COMMAND_MARKETPLACE_CANCEL_OFFER)
        //HTLC API
        MAP_JON_RPC_WE("atomics_create_htlc_proposal", on_create_htlc_proposal,         wallet_public::COMMAND_CREATE_HTLC_PROPOSAL)
        MAP_JON_RPC_WE("atomics_get_list_of_active_htlc", on_get_list_of_active_htlc,   wallet_public::COMMAND_GET_LIST_OF_ACTIVE_HTLC)
        MAP_JON_RPC_WE("atomics_redeem_htlc", on_redeem_htlc,                           wallet_public::COMMAND_REDEEM_HTLC)
        MAP_JON_RPC_WE("atomics_check_htlc_redeemed", on_check_htlc_redeemed,           wallet_public::COMMAND_CHECK_HTLC_REDEEMED)

      END_JSON_RPC_MAP()
    END_URI_MAP2()

      //json_rpc
      bool on_getbalance(const wallet_public::COMMAND_RPC_GET_BALANCE::request& req, wallet_public::COMMAND_RPC_GET_BALANCE::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_getaddress(const wallet_public::COMMAND_RPC_GET_ADDRESS::request& req, wallet_public::COMMAND_RPC_GET_ADDRESS::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_getwallet_info(const wallet_public::COMMAND_RPC_GET_WALLET_INFO::request& req, wallet_public::COMMAND_RPC_GET_WALLET_INFO::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_getwallet_restore_info(const wallet_public::COMMAND_RPC_GET_WALLET_RESTORE_INFO::request& req, wallet_public::COMMAND_RPC_GET_WALLET_RESTORE_INFO::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_get_seed_phrase_info(const wallet_public::COMMAND_RPC_GET_SEED_PHRASE_INFO::request& req, wallet_public::COMMAND_RPC_GET_SEED_PHRASE_INFO::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_get_recent_txs_and_info(const wallet_public::COMMAND_RPC_GET_RECENT_TXS_AND_INFO::request& req, wallet_public::COMMAND_RPC_GET_RECENT_TXS_AND_INFO::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_transfer(const wallet_public::COMMAND_RPC_TRANSFER::request& req, wallet_public::COMMAND_RPC_TRANSFER::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_store(const wallet_public::COMMAND_RPC_STORE::request& req, wallet_public::COMMAND_RPC_STORE::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_get_payments(const wallet_public::COMMAND_RPC_GET_PAYMENTS::request& req, wallet_public::COMMAND_RPC_GET_PAYMENTS::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_get_bulk_payments(const wallet_public::COMMAND_RPC_GET_BULK_PAYMENTS::request& req, wallet_public::COMMAND_RPC_GET_BULK_PAYMENTS::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_make_integrated_address(const wallet_public::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::request& req, wallet_public::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_split_integrated_address(const wallet_public::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::request& req, wallet_public::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_sweep_below(const wallet_public::COMMAND_SWEEP_BELOW::request& req, wallet_public::COMMAND_SWEEP_BELOW::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_sign_transfer(const wallet_public::COMMAND_SIGN_TRANSFER::request& req, wallet_public::COMMAND_SIGN_TRANSFER::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_submit_transfer(const wallet_public::COMMAND_SUBMIT_TRANSFER::request& req, wallet_public::COMMAND_SUBMIT_TRANSFER::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_search_for_transactions(const wallet_public::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS::request& req, wallet_public::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_get_mining_history(const wallet_public::COMMAND_RPC_GET_MINING_HISTORY::request& req, wallet_public::COMMAND_RPC_GET_MINING_HISTORY::response& res, epee::json_rpc::error& er, connection_context& cntx);
      
      bool on_contracts_send_proposal(const wallet_public::COMMAND_CONTRACTS_SEND_PROPOSAL::request& req, wallet_public::COMMAND_CONTRACTS_SEND_PROPOSAL::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_contracts_accept_proposal(const wallet_public::COMMAND_CONTRACTS_ACCEPT_PROPOSAL::request& req, wallet_public::COMMAND_CONTRACTS_ACCEPT_PROPOSAL::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_contracts_get_all(const wallet_public::COMMAND_CONTRACTS_GET_ALL::request& req, wallet_public::COMMAND_CONTRACTS_GET_ALL::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_contracts_release(const wallet_public::COMMAND_CONTRACTS_RELEASE::request& req, wallet_public::COMMAND_CONTRACTS_RELEASE::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_contracts_request_cancel(const wallet_public::COMMAND_CONTRACTS_REQUEST_CANCEL::request& req, wallet_public::COMMAND_CONTRACTS_REQUEST_CANCEL::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_contracts_accept_cancel(const wallet_public::COMMAND_CONTRACTS_ACCEPT_CANCEL::request& req, wallet_public::COMMAND_CONTRACTS_ACCEPT_CANCEL::response& res, epee::json_rpc::error& er, connection_context& cntx);

      bool on_marketplace_get_my_offers(const wallet_public::COMMAND_MARKETPLACE_GET_MY_OFFERS::request& req, wallet_public::COMMAND_MARKETPLACE_GET_MY_OFFERS::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_marketplace_push_offer(const wallet_public::COMMAND_MARKETPLACE_PUSH_OFFER::request& req, wallet_public::COMMAND_MARKETPLACE_PUSH_OFFER::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_marketplace_push_update_offer(const wallet_public::COMMAND_MARKETPLACE_PUSH_UPDATE_OFFER::request& req, wallet_public::COMMAND_MARKETPLACE_PUSH_UPDATE_OFFER::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_marketplace_cancel_offer(const wallet_public::COMMAND_MARKETPLACE_CANCEL_OFFER::request& req, wallet_public::COMMAND_MARKETPLACE_CANCEL_OFFER::response& res, epee::json_rpc::error& er, connection_context& cntx);

      bool on_create_htlc_proposal(const wallet_public::COMMAND_CREATE_HTLC_PROPOSAL::request& req, wallet_public::COMMAND_CREATE_HTLC_PROPOSAL::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_get_list_of_active_htlc(const wallet_public::COMMAND_GET_LIST_OF_ACTIVE_HTLC::request& req, wallet_public::COMMAND_GET_LIST_OF_ACTIVE_HTLC::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_redeem_htlc(const wallet_public::COMMAND_REDEEM_HTLC::request& req, wallet_public::COMMAND_REDEEM_HTLC::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_check_htlc_redeemed(const wallet_public::COMMAND_CHECK_HTLC_REDEEMED::request& req, wallet_public::COMMAND_CHECK_HTLC_REDEEMED::response& res, epee::json_rpc::error& er, connection_context& cntx);


      bool handle_command_line(const boost::program_options::variables_map& vm);

  private:
      wallet2& m_wallet;
      std::string m_port;
      std::string m_bind_ip;
      bool m_do_mint;
      bool m_deaf;
      uint64_t m_last_wallet_store_height;
  };

} // namespace tools
