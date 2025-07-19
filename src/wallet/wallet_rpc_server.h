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

#define ZANO_ACCESS_TOKEN "Zano-Access-Token"

namespace tools
{
  struct i_wallet_provider
  {
    virtual void lock() {};
    virtual void unlock() {};
//#ifndef MOBILE_WALLET_BUILD
    virtual std::shared_ptr<wallet2> get_wallet() = 0;
//#endif
  };

  struct wallet_rpc_locker
  {
    wallet_rpc_locker(i_wallet_provider* wallet_provider) :m_pwallet_provider(wallet_provider)
    {
      m_pwallet_provider->lock();
//#ifndef MOBILE_WALLET_BUILD
      m_wallet_ptr = m_pwallet_provider->get_wallet();
//#endif   
      if (!m_wallet_ptr.get())
      {
        throw std::runtime_error("Wallet object closed");
      }
    }

    std::shared_ptr<wallet2> get_wallet() { return m_wallet_ptr; }

    ~wallet_rpc_locker()
    {
      m_pwallet_provider->unlock();
    }

  private:
    std::shared_ptr<wallet2> m_wallet_ptr;
    i_wallet_provider* m_pwallet_provider;
  };


  struct wallet_provider_simple : public i_wallet_provider
  {
    wallet_provider_simple(std::shared_ptr<wallet2> wallet_ptr) : m_wallet_ptr(wallet_ptr)
    {}

    // interface i_wallet_provider
    virtual std::shared_ptr<wallet2> get_wallet() override
    {
      return m_wallet_ptr;
    }

  private:
    std::shared_ptr<wallet2> m_wallet_ptr;
  };


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class wallet_rpc_server : public epee::http_server_impl_base<wallet_rpc_server>, public epee::net_utils::http::i_chain_handler
  {
  public:
    typedef epee::net_utils::connection_context_base connection_context;

    wallet_rpc_server(std::shared_ptr<wallet2> wptr);
    wallet_rpc_server(i_wallet_provider* provider_ptr);

    const static command_line::arg_descriptor<std::string> arg_rpc_bind_port;
    const static command_line::arg_descriptor<std::string> arg_rpc_bind_ip;
    const static command_line::arg_descriptor<std::string> arg_miner_text_info;
    const static command_line::arg_descriptor<bool>        arg_deaf_mode;
    const static command_line::arg_descriptor<std::string> arg_jwt_secret;


    static void init_options(boost::program_options::options_description& desc);
    bool init(const boost::program_options::variables_map& vm);
    bool run(bool do_mint, bool offline_mode, const currency::account_public_address& miner_address);

    virtual bool handle_http_request(const epee::net_utils::http::http_request_info& query_info, epee::net_utils::http::http_response_info& response_info,
      connection_context& conn_context)
    {
      bool  call_found = false;
      return this->handle_http_request(query_info, response_info, conn_context, call_found, epee::net_utils::http::i_chain_handler::m_empty_documentation);
    }

    // interface i_chain_handler
    virtual bool handle_http_request(const epee::net_utils::http::http_request_info& query_info, epee::net_utils::http::http_response_info& response_info,
      epee::net_utils::connection_context_base& conn_context, bool& call_found, documentation& docs = epee::net_utils::http::i_chain_handler::m_empty_documentation) override;

    void set_jwt_secret(const std::string& jwt);
    const std::string& get_jwt_secret();

    BEGIN_URI_MAP2_VIRTUAL()
      BEGIN_JSON_RPC_MAP("/json_rpc")
        MAP_JON_RPC_WE("getbalance",                on_getbalance,                wallet_public::COMMAND_RPC_GET_BALANCE)
        MAP_JON_RPC_WE("getaddress",                on_getaddress,                wallet_public::COMMAND_RPC_GET_ADDRESS)
        MAP_JON_RPC_WE("get_wallet_info",           on_getwallet_info,            wallet_public::COMMAND_RPC_GET_WALLET_INFO)
        MAP_JON_RPC_WE("get_recent_txs_and_info",   on_get_recent_txs_and_info,   wallet_public::COMMAND_RPC_GET_RECENT_TXS_AND_INFO) //LEGACY
        MAP_JON_RPC_WE("get_recent_txs_and_info2",  on_get_recent_txs_and_info2,  wallet_public::COMMAND_RPC_GET_RECENT_TXS_AND_INFO2)
        MAP_JON_RPC_WE("transfer",                  on_transfer,                  wallet_public::COMMAND_RPC_TRANSFER)
        MAP_JON_RPC_WE("store",                     on_store,                     wallet_public::COMMAND_RPC_STORE)
        MAP_JON_RPC_WE("force_rescan_tx_pool",      force_rescan_tx_pool,         wallet_public::COMMAND_RPC_FORCE_RESCAN_TX_POOL)
        MAP_JON_RPC_WE("get_payments",              on_get_payments,              wallet_public::COMMAND_RPC_GET_PAYMENTS)
        MAP_JON_RPC_WE("get_bulk_payments",         on_get_bulk_payments,         wallet_public::COMMAND_RPC_GET_BULK_PAYMENTS)
        MAP_JON_RPC_WE("make_integrated_address",   on_make_integrated_address,   wallet_public::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS)
        MAP_JON_RPC_WE("split_integrated_address",  on_split_integrated_address,  wallet_public::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS)
        MAP_JON_RPC_WE("sweep_below",               on_sweep_below,               wallet_public::COMMAND_SWEEP_BELOW)
        MAP_JON_RPC_WE("get_bare_outs_stats",       on_get_bare_outs_stats,       wallet_public::COMMAND_RPC_GET_BARE_OUTS_STATS)
        MAP_JON_RPC_WE("sweep_bare_outs",           on_sweep_bare_outs,           wallet_public::COMMAND_RPC_SWEEP_BARE_OUTS)
        MAP_JON_RPC_WE("sign_transfer",             on_sign_transfer,             wallet_public::COMMAND_SIGN_TRANSFER)
        MAP_JON_RPC_WE("submit_transfer",           on_submit_transfer,           wallet_public::COMMAND_SUBMIT_TRANSFER)
        MAP_JON_RPC_WE("search_for_transactions",   on_search_for_transactions,   wallet_public::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS_LEGACY)
        MAP_JON_RPC_WE("search_for_transactions2",  on_search_for_transactions2,  wallet_public::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS)
        MAP_JON_RPC_WE("get_restore_info",          on_getwallet_restore_info,    wallet_public::COMMAND_RPC_GET_WALLET_RESTORE_INFO)
        MAP_JON_RPC_WE("get_seed_phrase_info",      on_get_seed_phrase_info,      wallet_public::COMMAND_RPC_GET_SEED_PHRASE_INFO)
        MAP_JON_RPC_WE("get_mining_history",        on_get_mining_history,        wallet_public::COMMAND_RPC_GET_MINING_HISTORY)
        MAP_JON_RPC_WE("register_alias",            on_register_alias,            wallet_public::COMMAND_RPC_REGISTER_ALIAS)
        MAP_JON_RPC_WE("update_alias",              on_update_alias,              wallet_public::COMMAND_RPC_UPDATE_ALIAS)

        //contracts API
        //MAP_JON_RPC_WE("contracts_send_proposal",             on_contracts_send_proposal,      wallet_public::COMMAND_CONTRACTS_SEND_PROPOSAL)
        //MAP_JON_RPC_WE("contracts_accept_proposal",           on_contracts_accept_proposal,    wallet_public::COMMAND_CONTRACTS_ACCEPT_PROPOSAL)
        //MAP_JON_RPC_WE("contracts_get_all",                   on_contracts_get_all,            wallet_public::COMMAND_CONTRACTS_GET_ALL)
        //MAP_JON_RPC_WE("contracts_release",                   on_contracts_release,            wallet_public::COMMAND_CONTRACTS_RELEASE)
        //MAP_JON_RPC_WE("contracts_request_cancel",            on_contracts_request_cancel,     wallet_public::COMMAND_CONTRACTS_REQUEST_CANCEL)
        //MAP_JON_RPC_WE("contracts_accept_cancel",             on_contracts_accept_cancel,      wallet_public::COMMAND_CONTRACTS_ACCEPT_CANCEL)
        //marketplace API
        MAP_JON_RPC_WE("marketplace_get_offers_ex",           on_marketplace_get_my_offers,     wallet_public::COMMAND_MARKETPLACE_GET_MY_OFFERS)
        MAP_JON_RPC_WE("marketplace_push_offer",              on_marketplace_push_offer,        wallet_public::COMMAND_MARKETPLACE_PUSH_OFFER)
        MAP_JON_RPC_WE("marketplace_push_update_offer",       on_marketplace_push_update_offer, wallet_public::COMMAND_MARKETPLACE_PUSH_UPDATE_OFFER)
        MAP_JON_RPC_WE("marketplace_cancel_offer",            on_marketplace_cancel_offer,      wallet_public::COMMAND_MARKETPLACE_CANCEL_OFFER)
        //HTLC API
        //MAP_JON_RPC_WE("atomics_create_htlc_proposal",        on_create_htlc_proposal,          wallet_public::COMMAND_CREATE_HTLC_PROPOSAL)
        //MAP_JON_RPC_WE("atomics_get_list_of_active_htlc",     on_get_list_of_active_htlc,       wallet_public::COMMAND_GET_LIST_OF_ACTIVE_HTLC)
        //MAP_JON_RPC_WE("atomics_redeem_htlc",                 on_redeem_htlc,                   wallet_public::COMMAND_REDEEM_HTLC)
        //MAP_JON_RPC_WE("atomics_check_htlc_redeemed",         on_check_htlc_redeemed,           wallet_public::COMMAND_CHECK_HTLC_REDEEMED)

        //IONIC_SWAPS API
        MAP_JON_RPC_WE("ionic_swap_generate_proposal",        on_ionic_swap_generate_proposal,  wallet_public::COMMAND_IONIC_SWAP_GENERATE_PROPOSAL)
        MAP_JON_RPC_WE("ionic_swap_get_proposal_info",        on_ionic_swap_get_proposal_info,  wallet_public::COMMAND_IONIC_SWAP_GET_PROPOSAL_INFO)
        MAP_JON_RPC_WE("ionic_swap_accept_proposal",          on_ionic_swap_accept_proposal,    wallet_public::COMMAND_IONIC_SWAP_ACCEPT_PROPOSAL)

        // Assets API
        MAP_JON_RPC_WE("assets_whitelist_get",                on_assets_whitelist_get,          wallet_public::COMMAND_ASSETS_WHITELIST_GET)
        MAP_JON_RPC_WE("assets_whitelist_add",                on_assets_whitelist_add,          wallet_public::COMMAND_ASSETS_WHITELIST_ADD)
        MAP_JON_RPC_WE("assets_whitelist_remove",             on_assets_whitelist_remove,       wallet_public::COMMAND_ASSETS_WHITELIST_REMOVE)
        
        MAP_JON_RPC_WE("deploy_asset",                        on_asset_deploy,                  wallet_public::COMMAND_ASSETS_DEPLOY)
        MAP_JON_RPC_WE("emit_asset",                          on_asset_emit,                    wallet_public::COMMAND_ASSETS_EMIT)
        MAP_JON_RPC_WE("update_asset",                        on_asset_update,                  wallet_public::COMMAND_ASSETS_UPDATE)
        MAP_JON_RPC_WE("burn_asset",                          on_asset_burn,                    wallet_public::COMMAND_ASSETS_BURN)
        MAP_JON_RPC_WE("send_ext_signed_asset_tx",            on_asset_send_ext_signed_tx,      wallet_public::COMMAND_ASSET_SEND_EXT_SIGNED_TX)
        MAP_JON_RPC_WE("attach_asset_descriptor",             on_attach_asset_descriptor,       wallet_public::COMMAND_ATTACH_ASSET_DESCRIPTOR)
        MAP_JON_RPC_WE("transfer_asset_ownership",            on_transfer_asset_ownership,      wallet_public::COMMAND_TRANSFER_ASSET_OWNERSHIP)

        //MULTIWALLET APIs
        MAP_JON_RPC_WE("mw_get_wallets",                      on_mw_get_wallets,                wallet_public::COMMAND_MW_GET_WALLETS)
        MAP_JON_RPC_WE("mw_select_wallet",                    on_mw_select_wallet,              wallet_public::COMMAND_MW_SELECT_WALLET)

        //basic crypto operations
        MAP_JON_RPC_WE("sign_message",                        on_sign_message,                  wallet_public::COMMAND_SIGN_MESSAGE)
        MAP_JON_RPC_WE("encrypt_data",                        on_encrypt_data,                  wallet_public::COMMAND_ENCRYPT_DATA)
        MAP_JON_RPC_WE("decrypt_data",                        on_decrypt_data,                  wallet_public::COMMAND_DECRYPT_DATA)

        //utility call
        MAP_JON_RPC_WE("proxy_to_daemon",                     on_proxy_to_daemon,               wallet_public::COMMAND_PROXY_TO_DAEMON)
        MAP_JON_RPC_WE("clear_utxo_cold_sig_reservation",     on_clear_utxo_cold_sig_reservation, wallet_public::COMMAND_CLEAR_UTXO_COLD_SIG_RESERVATION)
      END_JSON_RPC_MAP()
    END_URI_MAP2()

    bool auth_http_request(const epee::net_utils::http::http_request_info& query_info, epee::net_utils::http::http_response_info& response, connection_context& m_conn_context);
    //json_rpc
    bool on_getbalance(const wallet_public::COMMAND_RPC_GET_BALANCE::request& req, wallet_public::COMMAND_RPC_GET_BALANCE::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_getaddress(const wallet_public::COMMAND_RPC_GET_ADDRESS::request& req, wallet_public::COMMAND_RPC_GET_ADDRESS::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_getwallet_info(const wallet_public::COMMAND_RPC_GET_WALLET_INFO::request& req, wallet_public::COMMAND_RPC_GET_WALLET_INFO::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_getwallet_restore_info(const wallet_public::COMMAND_RPC_GET_WALLET_RESTORE_INFO::request& req, wallet_public::COMMAND_RPC_GET_WALLET_RESTORE_INFO::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_get_seed_phrase_info(const wallet_public::COMMAND_RPC_GET_SEED_PHRASE_INFO::request& req, wallet_public::COMMAND_RPC_GET_SEED_PHRASE_INFO::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_get_recent_txs_and_info(const wallet_public::COMMAND_RPC_GET_RECENT_TXS_AND_INFO::request& req, wallet_public::COMMAND_RPC_GET_RECENT_TXS_AND_INFO::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_get_recent_txs_and_info2(const wallet_public::COMMAND_RPC_GET_RECENT_TXS_AND_INFO2::request& req, wallet_public::COMMAND_RPC_GET_RECENT_TXS_AND_INFO2::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_transfer(const wallet_public::COMMAND_RPC_TRANSFER::request& req, wallet_public::COMMAND_RPC_TRANSFER::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_store(const wallet_public::COMMAND_RPC_STORE::request& req, wallet_public::COMMAND_RPC_STORE::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool force_rescan_tx_pool(const wallet_public::COMMAND_RPC_FORCE_RESCAN_TX_POOL::request& req, wallet_public::COMMAND_RPC_FORCE_RESCAN_TX_POOL::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_get_payments(const wallet_public::COMMAND_RPC_GET_PAYMENTS::request& req, wallet_public::COMMAND_RPC_GET_PAYMENTS::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_get_bulk_payments(const wallet_public::COMMAND_RPC_GET_BULK_PAYMENTS::request& req, wallet_public::COMMAND_RPC_GET_BULK_PAYMENTS::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_make_integrated_address(const wallet_public::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::request& req, wallet_public::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_split_integrated_address(const wallet_public::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::request& req, wallet_public::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_sweep_below(const wallet_public::COMMAND_SWEEP_BELOW::request& req, wallet_public::COMMAND_SWEEP_BELOW::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_get_bare_outs_stats(const wallet_public::COMMAND_RPC_GET_BARE_OUTS_STATS ::request& req, wallet_public::COMMAND_RPC_GET_BARE_OUTS_STATS::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_sweep_bare_outs(const wallet_public::COMMAND_RPC_SWEEP_BARE_OUTS::request& req, wallet_public::COMMAND_RPC_SWEEP_BARE_OUTS::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_sign_transfer(const wallet_public::COMMAND_SIGN_TRANSFER::request& req, wallet_public::COMMAND_SIGN_TRANSFER::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_submit_transfer(const wallet_public::COMMAND_SUBMIT_TRANSFER::request& req, wallet_public::COMMAND_SUBMIT_TRANSFER::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_search_for_transactions(const wallet_public::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS_LEGACY::request& req, wallet_public::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS_LEGACY::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_search_for_transactions2(const wallet_public::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS::request& req, wallet_public::COMMAND_RPC_SEARCH_FOR_TRANSACTIONS::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_get_mining_history(const wallet_public::COMMAND_RPC_GET_MINING_HISTORY::request& req, wallet_public::COMMAND_RPC_GET_MINING_HISTORY::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_register_alias(const wallet_public::COMMAND_RPC_REGISTER_ALIAS::request& req, wallet_public::COMMAND_RPC_REGISTER_ALIAS::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_update_alias(const wallet_public::COMMAND_RPC_UPDATE_ALIAS::request& req, wallet_public::COMMAND_RPC_UPDATE_ALIAS::response& res, epee::json_rpc::error& er, connection_context& cntx);
    
      
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

    bool on_ionic_swap_generate_proposal(const wallet_public::COMMAND_IONIC_SWAP_GENERATE_PROPOSAL::request& req, wallet_public::COMMAND_IONIC_SWAP_GENERATE_PROPOSAL::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_ionic_swap_get_proposal_info(const wallet_public::COMMAND_IONIC_SWAP_GET_PROPOSAL_INFO::request& req, wallet_public::COMMAND_IONIC_SWAP_GET_PROPOSAL_INFO::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_ionic_swap_accept_proposal(const wallet_public::COMMAND_IONIC_SWAP_ACCEPT_PROPOSAL::request& req, wallet_public::COMMAND_IONIC_SWAP_ACCEPT_PROPOSAL::response& res, epee::json_rpc::error& er, connection_context& cntx);

    bool on_assets_whitelist_get(const wallet_public::COMMAND_ASSETS_WHITELIST_GET::request& req, wallet_public::COMMAND_ASSETS_WHITELIST_GET::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_assets_whitelist_add(const wallet_public::COMMAND_ASSETS_WHITELIST_ADD::request& req, wallet_public::COMMAND_ASSETS_WHITELIST_ADD::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_assets_whitelist_remove(const wallet_public::COMMAND_ASSETS_WHITELIST_REMOVE::request& req, wallet_public::COMMAND_ASSETS_WHITELIST_REMOVE::response& res, epee::json_rpc::error& er, connection_context& cntx);
    
    bool on_asset_deploy(const wallet_public::COMMAND_ASSETS_DEPLOY::request& req, wallet_public::COMMAND_ASSETS_DEPLOY::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_asset_emit(const wallet_public::COMMAND_ASSETS_EMIT::request& req, wallet_public::COMMAND_ASSETS_EMIT::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_asset_update(const wallet_public::COMMAND_ASSETS_UPDATE::request& req, wallet_public::COMMAND_ASSETS_UPDATE::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_asset_burn(const wallet_public::COMMAND_ASSETS_BURN::request& req, wallet_public::COMMAND_ASSETS_BURN::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_asset_send_ext_signed_tx(const wallet_public::COMMAND_ASSET_SEND_EXT_SIGNED_TX::request& req, wallet_public::COMMAND_ASSET_SEND_EXT_SIGNED_TX::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_attach_asset_descriptor(const wallet_public::COMMAND_ATTACH_ASSET_DESCRIPTOR::request& req, wallet_public::COMMAND_ATTACH_ASSET_DESCRIPTOR::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_transfer_asset_ownership(const wallet_public::COMMAND_TRANSFER_ASSET_OWNERSHIP::request& req, wallet_public::COMMAND_TRANSFER_ASSET_OWNERSHIP::response& res, epee::json_rpc::error& er, connection_context& cntx);
    

    bool on_mw_get_wallets(const wallet_public::COMMAND_MW_GET_WALLETS::request& req, wallet_public::COMMAND_MW_GET_WALLETS::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_mw_select_wallet(const wallet_public::COMMAND_MW_SELECT_WALLET::request& req, wallet_public::COMMAND_MW_SELECT_WALLET::response& res, epee::json_rpc::error& er, connection_context& cntx);

    bool on_sign_message(const wallet_public::COMMAND_SIGN_MESSAGE::request& req, wallet_public::COMMAND_SIGN_MESSAGE::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_encrypt_data(const wallet_public::COMMAND_ENCRYPT_DATA::request& req, wallet_public::COMMAND_ENCRYPT_DATA::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_decrypt_data(const wallet_public::COMMAND_DECRYPT_DATA::request& req, wallet_public::COMMAND_DECRYPT_DATA::response& res, epee::json_rpc::error& er, connection_context& cntx);

    bool on_proxy_to_daemon(const wallet_public::COMMAND_PROXY_TO_DAEMON::request& req, wallet_public::COMMAND_PROXY_TO_DAEMON::response& res, epee::json_rpc::error& er, connection_context& cntx);
    bool on_clear_utxo_cold_sig_reservation(const wallet_public::COMMAND_CLEAR_UTXO_COLD_SIG_RESERVATION::request& req, wallet_public::COMMAND_CLEAR_UTXO_COLD_SIG_RESERVATION::response& res, epee::json_rpc::error& er, connection_context& cntx);


    //std::shared_ptr<wallet2> get_wallet();

    //bool reset_active_wallet(std::shared_ptr<wallet2> w);

    bool handle_command_line(const boost::program_options::variables_map& vm);
    void rpc_destinations_to_currency_destinations(const std::list<wallet_public::transfer_destination>& rpc_destinations, bool nullify_asset_id, bool try_to_split, std::vector<currency::tx_destination_entry>& currency_destinations);


  private:
    std::shared_ptr<i_wallet_provider> m_pwallet_provider_sh_ptr;
    i_wallet_provider* m_pwallet_provider;
    std::string m_port;
    std::string m_bind_ip;
    bool m_do_mint;
    bool m_deaf;
    uint64_t m_last_wallet_store_height;
    std::string m_jwt_secret;
    epee::misc_utils::expirating_set<std::string, uint64_t> m_jwt_used_salts;
  };

} // namespace tools
