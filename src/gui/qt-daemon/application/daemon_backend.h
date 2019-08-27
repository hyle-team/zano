// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/program_options.hpp>
#include "warnings.h"
PUSH_WARNINGS
DISABLE_VS_WARNINGS(4100)
DISABLE_VS_WARNINGS(4503)
#include "include_base_utils.h"
#include "version.h"

using namespace epee;

#include "console_handler.h"
#include "p2p/net_node.h"
#include "currency_core/checkpoints_create.h"
#include "currency_core/currency_core.h"
#include "currency_core/bc_offers_service.h"
#include "rpc/core_rpc_server.h"
#include "currency_protocol/currency_protocol_handler.h"
#include "daemon/daemon_commands_handler.h"
//#include "common/miniupnp_helper.h"
#include "view_iface.h"
#include "core_fast_rpc_proxy.h"
#include "wallet/wallet2.h"
#include "wallet_id_adapter.h"

POP_WARNINGS

namespace po = boost::program_options;

#if defined(WIN32)
#include <crtdbg.h>
#endif

//TODO: need refactoring here. (template classes can't be used in BOOST_CLASS_VERSION)
BOOST_CLASS_VERSION(nodetool::node_server<currency::t_currency_protocol_handler<currency::core> >, CURRENT_P2P_STORAGE_ARCHIVE_VER);

struct wallet_lock_time_watching_policy
{
  static void watch_lock_time(uint64_t lock_time);
};


class daemon_backend : public i_backend_wallet_callback
{

public:
  struct wallet_vs_options
  {
    currency::core_runtime_config core_conf;
    epee::locked_object<std::shared_ptr<tools::wallet2>, wallet_lock_time_watching_policy> w;
    std::atomic<bool> do_mining;
    std::atomic<bool> stop;
    std::atomic<bool> break_mining_loop;
    std::atomic<uint64_t> wallet_state;
    std::atomic<uint64_t> last_wallet_synch_height;
    std::atomic<uint64_t>* plast_daemon_height;
    std::atomic<uint64_t>* plast_daemon_network_state;
    std::atomic<bool>* plast_daemon_is_disconnected;
    std::atomic<bool> has_related_alias_in_unconfirmed;
    std::atomic<bool> need_to_update_wallet_info;
    view::i_view* pview;
    uint64_t wallet_id;
    epee::locked_object<std::list<bc_services::offer_details_ex>> offers;

    std::thread miner_thread;
    void worker_func();
    std::string get_log_prefix() const { return std::string("[") + epee::string_tools::num_to_string_fast(wallet_id) + ":" + w->get()->get_log_prefix() + "]"; }
    ~wallet_vs_options();
  };

  daemon_backend();
  ~daemon_backend();
  bool init(int argc, char* argv[], view::i_view* pview_handler);
  bool start();
  bool stop();
  bool send_stop_signal();
  std::string open_wallet(const std::wstring& path, const std::string& password, view::open_wallet_response& owr);
  std::string generate_wallet(const std::wstring& path, const std::string& password, view::open_wallet_response& owr);
  std::string restore_wallet(const std::wstring& path, const std::string& password, const std::string& restore_key, view::open_wallet_response& owr);
  std::string run_wallet(uint64_t wallet_id);
  std::string get_recent_transfers(size_t wallet_id, uint64_t offset, uint64_t count, view::transfers_array& tr_hist);
  std::string get_wallet_info(size_t wallet_id, view::wallet_info& wi);
  std::string get_contracts(size_t wallet_id, std::vector<tools::wallet_public::escrow_contract_details>& contracts);
  std::string create_proposal(size_t wallet_id, const bc_services::contract_private_details& escrow, const std::string& payment_id,
                              uint64_t expiration_period,
                              uint64_t fee, 
                              uint64_t b_fee);
  std::string create_proposal(const view::create_proposal_param_gui& cpp);
  std::string accept_proposal(size_t wallet_id, const crypto::hash& contract_id);
  std::string release_contract(size_t wallet_id, const crypto::hash& contract_id, const std::string& contract_over_type);
  std::string request_cancel_contract(size_t wallet_id, const crypto::hash& contract_id, uint64_t fee, uint64_t expiration_period);
  std::string accept_cancel_contract(size_t wallet_id, const crypto::hash& contract_id);
  

  std::string get_wallet_info(wallet_vs_options& w, view::wallet_info& wi);
  std::string close_wallet(size_t wallet_id);
  std::string push_offer(size_t wallet_id, const bc_services::offer_details_ex& od, currency::transaction& res_tx);
  std::string cancel_offer(const view::cancel_offer_param& co, currency::transaction& res_tx);
  std::string push_update_offer(const bc_services::update_offer_details& uo, currency::transaction& res_tx);
  //std::string get_all_offers(currency::COMMAND_RPC_GET_ALL_OFFERS::response& od);
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
  std::string get_wallet_restore_info(uint64_t wallet_id, std::string& restore_key);
  std::string backup_wallet(uint64_t wallet_id, const std::wstring& path);
  std::string reset_wallet_password(uint64_t wallet_id, const std::string& pass);
  std::string is_wallet_password_valid(uint64_t wallet_id, const std::string& pass);
  std::string get_my_offers(const bc_services::core_offers_filter& filter, std::list<bc_services::offer_details_ex>& offers);
  std::string get_fav_offers(const std::list<bc_services::offer_id>& hashes, const bc_services::core_offers_filter& filter, std::list<bc_services::offer_details_ex>& offers);
  std::string get_tx_pool_info(currency::COMMAND_RPC_GET_POOL_INFO::response& res);
  uint64_t get_default_fee();
  std::string get_mining_estimate(uint64_t amuont_coins, 
    uint64_t time, 
    uint64_t& estimate_result, 
    uint64_t& all_coins_and_pos_diff_rate, 
    std::vector<uint64_t>& days);
  std::string is_pos_allowed();
  void toggle_pos_mining();
  std::string transfer(size_t wallet_id, const view::transfer_params& tp, currency::transaction& res_tx);
  std::string get_config_folder();
  std::string is_valid_brain_restore_data(const std::string& brain_text);
  void subscribe_to_core_events(currency::i_core_event_handler* pevents_handler);
  void unsubscribe_to_core_events();
  void get_gui_options(view::gui_options& opt);
  std::string get_wallet_log_prefix(size_t wallet_id) const;

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
  //----- i_backend_wallet_callback ------
  virtual void on_new_block(size_t wallet_id, uint64_t height, const currency::block& block);
	virtual void on_transfer2(size_t wallet_id, const tools::wallet_public::wallet_transfer_info& wti, uint64_t balance, uint64_t unlocked_balance, uint64_t total_mined);
  virtual void on_pos_block_found(size_t wallet_id, const currency::block& /*block*/);
  virtual void on_sync_progress(size_t wallet_id, const uint64_t& /*percents*/);
  virtual void on_transfer_canceled(size_t wallet_id, const tools::wallet_public::wallet_transfer_info& wti);

  std::thread m_main_worker_thread;
  std::atomic<bool> m_stop_singal_sent;
  view::i_view m_view_stub;
  view::i_view* m_pview;
  std::shared_ptr<tools::i_core_proxy> m_rpc_proxy;
  mutable critical_section m_wallets_lock;
  po::variables_map m_vm;

  std::atomic<uint64_t> m_last_daemon_height;
  std::atomic<uint64_t> m_last_daemon_network_state;
  std::atomic<bool> m_last_daemon_is_disconnected;
//  std::atomic<uint64_t> m_last_wallet_synch_height;
  std::atomic<uint64_t> m_wallet_id_counter;

  std::string m_data_dir;
  view::gui_options m_ui_opt;
  
  //daemon stuff
	bc_services::bc_offers_service m_offers_service;
  currency::core m_ccore;
  currency::t_currency_protocol_handler<currency::core> m_cprotocol;
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > m_p2psrv;
  currency::core_rpc_server m_rpc_server;

  bool m_remote_node_mode;
  std::atomic<bool> m_is_pos_allowed;


  std::map<size_t, wallet_vs_options> m_wallets;
  std::vector<std::string> m_wallet_log_prefixes;
  mutable critical_section m_wallet_log_prefixes_lock;
};




