// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/thread/shared_mutex.hpp>
#include <boost/program_options.hpp>
#include "warnings.h"
PUSH_VS_WARNINGS
DISABLE_VS_WARNINGS(4100)
DISABLE_VS_WARNINGS(4503)
#include "include_base_utils.h"
#include "version.h"

#include "console_handler.h"
#include "p2p/net_node.h"
#include "currency_core/checkpoints_create.h"
#ifndef MOBILE_WALLET_BUILD
  #include "currency_core/currency_core.h"
  #include "currency_core/bc_offers_service.h"
  #include "rpc/core_rpc_server.h"
  #include "currency_protocol/currency_protocol_handler.h"
  #include "core_fast_rpc_proxy.h"
#endif
//#include "daemon/daemon_commands_handler.h"
//#include "common/miniupnp_helper.h"
#include "view_iface.h"
#include "wallet2.h"
#include "wallet_id_adapter.h"
#include "wallet_rpc_server.h"

POP_VS_WARNINGS

namespace po = boost::program_options;

#if defined(WIN32)
#include <crtdbg.h>
#endif


struct wallet_lock_time_watching_policy
{
  static void watch_lock_time(uint64_t lock_time);
};


class wallets_manager : public i_backend_wallet_callback, public tools::i_wallet_provider, epee::net_utils::http::i_chain_handler
{

public:
  struct wallet_vs_options
  {
    currency::core_runtime_config core_conf;
    epee::locked_object<std::shared_ptr<tools::wallet2>, wallet_lock_time_watching_policy> w;
    typedef epee::locked_object<std::shared_ptr<tools::wallet2>, wallet_lock_time_watching_policy>::lock_shared_ptr wallet_lock_object;
    std::shared_ptr<tools::wallet_rpc_server> rpc_wrapper; //500 bytes of extra data, we can afford it, to have rpc-like invoke map
    std::atomic<bool> do_mining;
    std::atomic<bool> major_stop;
    std::atomic<bool> stop_for_refresh; //use separate var for passing to "refresh" member function, 
                                        //because it can be changed there due to internal interruption logis


    std::atomic<bool> break_mining_loop;
    std::atomic<uint64_t> wallet_state;
    std::atomic<uint64_t> last_wallet_synch_height;
    std::atomic<uint64_t>* plast_daemon_height;
    std::atomic<uint64_t>* plast_daemon_network_state;
    //std::atomic<bool>* plast_daemon_is_disconnected;
    std::shared_ptr<const tools::proxy_diagnostic_info> m_pproxy_diagnostig_info;
    std::atomic<bool> has_related_alias_in_unconfirmed;
    std::atomic<bool> need_to_update_wallet_info;
    std::atomic<bool> long_refresh_in_progress;
    epee::critical_section long_refresh_in_progress_lock; //secure wallet state and prevent from long wait while long refresh is in work

    view::i_view* pview;
    uint64_t wallet_id;
    epee::locked_object<std::list<bc_services::offer_details_ex>> offers;

    std::thread miner_thread;
    void worker_func();
    void stop(bool wait = true);
    std::string get_log_prefix() const { return std::string("[") + epee::string_tools::num_to_string_fast(wallet_id) + ":" + w->get()->get_log_prefix() + "]"; }
    ~wallet_vs_options();
  };

  wallets_manager();
  ~wallets_manager();
  bool init_command_line(int argc, char* argv[], std::string& fail_message);
  bool init(view::i_view* pview_handler);
  bool start();
  bool stop();
  bool quick_stop_no_save(); //stop without storing wallets
  bool quick_clear_wallets_no_save();
  bool send_stop_signal();
  bool get_opened_wallets(std::list<view::open_wallet_response>& result);
  const po::variables_map& get_arguments();

  std::string open_wallet(const std::wstring& path, const std::string& password, uint64_t txs_to_return, view::open_wallet_response& owr, bool exclude_mining_txs = false);
  std::string generate_wallet(const std::wstring& path, const std::string& password, view::open_wallet_response& owr);
  std::string restore_wallet(const std::wstring& path, const std::string& password, const std::string& seed_phrase, const std::string& seed_password, view::open_wallet_response& owr);
  std::string invoke(uint64_t wallet_id, std::string params);
  std::string get_wallet_status(uint64_t wallet_id);
  std::string run_wallet(uint64_t wallet_id);
  std::string get_recent_transfers(size_t wallet_id, uint64_t offset, uint64_t count, view::transfers_array& tr_hist, bool exclude_mining_txs = false);
  std::string get_wallet_info(uint64_t wallet_id, view::wallet_info& wi);
  std::string get_wallet_info_extra(uint64_t wallet_id, view::wallet_info_extra& wi);
  std::string get_contracts(size_t wallet_id, std::vector<tools::wallet_public::escrow_contract_details>& contracts);
  std::string create_proposal(const view::create_proposal_param_gui& cpp);
  std::string accept_proposal(size_t wallet_id, const crypto::hash& contract_id);
  std::string release_contract(size_t wallet_id, const crypto::hash& contract_id, const std::string& contract_over_type);
  std::string request_cancel_contract(size_t wallet_id, const crypto::hash& contract_id, uint64_t fee, uint64_t expiration_period);
  std::string accept_cancel_contract(size_t wallet_id, const crypto::hash& contract_id);
  
  std::string get_connectivity_status();
  std::string get_wallet_info(wallet_vs_options& w, view::wallet_info& wi);
  std::string close_wallet(size_t wallet_id);
  std::string push_offer(size_t wallet_id, const bc_services::offer_details_ex& od, currency::transaction& res_tx);
  std::string cancel_offer(const view::cancel_offer_param& co, currency::transaction& res_tx);
  std::string push_update_offer(const bc_services::update_offer_details& uo, currency::transaction& res_tx);
  //std::string get_all_offers(currency::COMMAND_RPC_GET_OFFERS_EX::response& od);
  std::string get_offers_ex(const bc_services::core_offers_filter& cof, std::list<bc_services::offer_details_ex>& offers, uint64_t& total_count);
  std::string get_aliases(view::alias_set& al_set);
  std::string get_alias_info_by_address(const std::string& addr, currency::alias_rpc_details& res_details);
  std::string get_alias_info_by_name(const std::string& name, currency::alias_rpc_details& res_details);
  std::string request_alias_registration(const currency::alias_rpc_details& al, uint64_t wallet_id, uint64_t fee, currency::transaction& res_tx, uint64_t reward);
  std::string request_alias_update(const currency::alias_rpc_details& al, uint64_t wallet_id, uint64_t fee, currency::transaction& res_tx, uint64_t reward);
  std::string get_alias_coast(const std::string& a, uint64_t& coast);
  std::string validate_address(const std::string& addr, std::string& payment_id);
  std::string resync_wallet(uint64_t wallet_id);
  std::string start_pos_mining(uint64_t wallet_id);
  std::string stop_pos_mining(uint64_t wallet_id);
  std::string check_available_sources(uint64_t wallet_id, std::list<uint64_t>& amounts);
  std::string get_mining_history(uint64_t wallet_id, tools::wallet_public::mining_history& wrpc);
  std::string get_wallet_restore_info(uint64_t wallet_id, std::string& seed_phrase, const std::string& seed_password);
  std::string backup_wallet(uint64_t wallet_id, const std::wstring& path);
  std::string reset_wallet_password(uint64_t wallet_id, const std::string& pass);
  std::string use_whitelisting(uint64_t wallet_id, bool use);
  std::string is_wallet_password_valid(uint64_t wallet_id, const std::string& pass);
  std::string create_ionic_swap_proposal(uint64_t wallet_id, const tools::wallet_public::create_ionic_swap_proposal_request& proposal, std::string& result_proposal_hex);
  std::string get_ionic_swap_proposal_info(uint64_t wallet_id, std::string&raw_tx_template_hex, tools::wallet_public::ionic_swap_proposal_info& proposal);
  std::string accept_ionic_swap_proposal(uint64_t wallet_id, std::string&raw_tx_template_hex, std::string& result_raw_tx);
  std::string get_my_offers(const bc_services::core_offers_filter& filter, std::list<bc_services::offer_details_ex>& offers);
  std::string get_fav_offers(const std::list<bc_services::offer_id>& hashes, const bc_services::core_offers_filter& filter, std::list<bc_services::offer_details_ex>& offers);
  std::string get_tx_pool_info(currency::COMMAND_RPC_GET_POOL_INFO::response& res);
  std::string export_wallet_history(const view::export_wallet_info& ewi);
  std::string setup_wallet_rpc(const std::string& jwt_secret);

#ifndef MOBILE_WALLET_BUILD
  currency::core_rpc_server& get_rpc_server() { return m_rpc_server; }
#endif
  uint64_t get_default_fee();
  std::string get_mining_estimate(uint64_t amuont_coins, 
    uint64_t time, 
    uint64_t& estimate_result, 
    uint64_t& all_coins_and_pos_diff_rate, 
    std::vector<uint64_t>& days);
  std::string is_pos_allowed();
  void toggle_pos_mining();
  std::string transfer(uint64_t wallet_id, const view::transfer_params& tp, currency::transaction& res_tx);
  std::string get_config_folder();
  std::string is_valid_brain_restore_data(const std::string& seed_phrase, const std::string& seed_password);
  std::string get_seed_phrase_info(const std::string& seed_phrase, const std::string& seed_password, view::seed_phrase_info& result);
#ifndef MOBILE_WALLET_BUILD
  void subscribe_to_core_events(currency::i_core_event_handler* pevents_handler);
  //void unsubscribe_to_core_events();
#endif
  void get_gui_options(view::gui_options& opt);
  std::string get_wallet_log_prefix(size_t wallet_id) const;
  bool is_qt_logs_enabled() const { return m_qt_logs_enbaled; }
  std::string get_qt_dev_tools_option() const { return m_qt_dev_tools; }
  void set_use_deffered_global_outputs(bool use) { m_use_deffered_global_outputs = use; }
  bool set_use_tor(bool use_tor);
  std::string add_custom_asset_id(uint64_t wallet_id, const crypto::public_key& asset_id, currency::asset_descriptor_base& asset_descriptor);
  std::string delete_custom_asset_id(uint64_t wallet_id, const crypto::public_key& asset_id);
  bool is_core_initialized() { return m_core_initialized;}

private:
  void main_worker(const po::variables_map& vm);
  bool init_local_daemon();
  bool deinit_local_daemon();
  bool update_state_info();
  bool update_wallets();
  void loop();
  //bool get_transfer_address(const std::string& adr_str, currency::account_public_address& addr, std::string& payment_id);
  bool get_last_blocks(view::daemon_status_info& dsi);
  void update_wallets_info();
  void init_wallet_entry(wallet_vs_options& wo, uint64_t id);
  static void prepare_wallet_status_info(wallet_vs_options& wo, view::wallet_status_info& wsi);
  bool get_is_remote_daemon_connected();

  template<typename guarded_code_t, typename error_prefix_maker_t>
  bool do_exception_safe_call(guarded_code_t guarded_code, error_prefix_maker_t error_prefix_maker, std::string& api_return_code_result);

  //----- i_backend_wallet_callback ------
  virtual void on_new_block(size_t wallet_id, uint64_t height, const currency::block& block) override;
  virtual void on_transfer2(size_t wallet_id, const tools::wallet_public::wallet_transfer_info& wti, const std::list<tools::wallet_public::asset_balance_entry>& balances, uint64_t total_mined) override;
  virtual void on_pos_block_found(size_t wallet_id, const currency::block& /*block*/) override;
  virtual void on_sync_progress(size_t wallet_id, const uint64_t& /*percents*/) override;
  virtual void on_transfer_canceled(size_t wallet_id, const tools::wallet_public::wallet_transfer_info& wti) override;
  virtual void on_tor_status_change(size_t wallet_id, const std::string& state) override;

  virtual void on_mw_get_wallets(std::vector<tools::wallet_public::wallet_entry_info>& wallets) override;
  virtual bool on_mw_select_wallet(uint64_t wallet_id) override;
  
  //----- i_wallet_provider ------
  virtual void lock();
  virtual void unlock();
//#ifndef MOBILE_WALLET_BUILD
  virtual std::shared_ptr<tools::wallet2> get_wallet();
//#endif
  //--------

  BEGIN_URI_MAP2_VIRTUAL()
    BEGIN_JSON_RPC_MAP("/json_rpc")
      //MULTIWALLET APIs
      MAP_JON_RPC_WE("mw_get_wallets", on_mw_get_wallets, tools::wallet_public::COMMAND_MW_GET_WALLETS)
      MAP_JON_RPC_WE("mw_select_wallet", on_mw_select_wallet, tools::wallet_public::COMMAND_MW_SELECT_WALLET)
      CHAIN_TO_PHANDLER(m_prpc_chain_handler)
    END_JSON_RPC_MAP()
    CHAIN_TO_PHANDLER(m_prpc_chain_handler)
  END_URI_MAP2()

  bool on_mw_get_wallets(const tools::wallet_public::COMMAND_MW_GET_WALLETS::request& req, tools::wallet_public::COMMAND_MW_GET_WALLETS::response& res, epee::json_rpc::error& er, epee::net_utils::connection_context_base& cntx);
  bool on_mw_select_wallet(const tools::wallet_public::COMMAND_MW_SELECT_WALLET::request& req, tools::wallet_public::COMMAND_MW_SELECT_WALLET::response& res, epee::json_rpc::error& er, epee::net_utils::connection_context_base& cntx);


  epee::net_utils::http::i_chain_handler* m_prpc_chain_handler = nullptr;
  std::thread m_main_worker_thread;
  
  std::atomic<bool> m_stop_singal_sent;
  std::mutex m_stop_singal_sent_mutex;
  std::condition_variable m_stop_singal_sent_mutex_cv;

  std::mutex m_select_wallet_rpc_lock;

  view::i_view m_view_stub;
  view::i_view* m_pview;
  std::shared_ptr<tools::i_core_proxy> m_rpc_proxy;
  po::variables_map m_vm;

  bool m_use_deffered_global_outputs;
  std::atomic<uint64_t> m_last_daemon_height;
  std::atomic<uint64_t> m_last_daemon_network_state;
  std::shared_ptr<const tools::proxy_diagnostic_info> m_pproxy_diganostic_info;
  //std::atomic<bool> m_last_daemon_is_disconnected;
//  std::atomic<uint64_t> m_last_wallet_synch_height;
  std::atomic<uint64_t> m_wallet_id_counter;
  std::atomic<bool> m_dont_save_wallet_at_stop;
  std::atomic<bool> m_core_initialized = false;

  std::string m_data_dir;
  view::gui_options m_ui_opt;
  
#ifndef MOBILE_WALLET_BUILD
  //daemon stuff
	bc_services::bc_offers_service m_offers_service;
  currency::core m_ccore;
  currency::t_currency_protocol_handler<currency::core> m_cprotocol;
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > m_p2psrv;
  currency::core_rpc_server m_rpc_server;

  tools::wallet_rpc_server m_wallet_rpc_server;
  wallet_vs_options::wallet_lock_object m_current_wallet_locked_object;
  uint64_t m_rpc_selected_wallet_id = 0;
#endif

  bool m_remote_node_mode;
  bool m_qt_logs_enbaled;
  std::string m_qt_dev_tools;
  std::atomic<bool> m_is_pos_allowed;
  std::atomic<bool> m_use_tor;


  std::map<size_t, wallet_vs_options> m_wallets;
  //mutable critical_section m_wallets_lock;
  mutable boost::shared_mutex m_wallets_lock;

  std::vector<std::string> m_wallet_log_prefixes;
  mutable critical_section m_wallet_log_prefixes_lock;

}; // class wallets_manager


