// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>

#include <boost/program_options/variables_map.hpp>

#include "currency_core/account.h"

#include "wallet/wallet2.h"
#include "console_handler.h"
#include "password_container.h"


namespace currency
{
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class simple_wallet final : public tools::i_wallet2_callback,
                        public std::enable_shared_from_this<simple_wallet>
  {
  public:
    typedef std::vector<std::string> command_type;

    simple_wallet();
    ~simple_wallet();
    bool init(const boost::program_options::variables_map& vm);
    bool deinit();
    bool run();
    void stop();
    void set_offline_mode(bool offline_mode);

    //wallet *create_wallet();
    bool process_command(const std::vector<std::string> &args);
    std::string get_commands_str();
  private:
    void handle_command_line(const boost::program_options::variables_map& vm);

    bool run_console_handler();

    bool new_wallet(const std::string &wallet_file, const std::string& password, bool create_auditable_wallet);
    bool open_wallet(const std::string &wallet_file, const std::string& password);
    bool restore_wallet(const std::string& wallet_file, const std::string& seed_or_tracking_seed, const std::string& password, bool tracking_wallet, const std::string& seed_password);
    bool close_wallet();

    bool help(const std::vector<std::string> &args = std::vector<std::string>());
    bool start_mining(const std::vector<std::string> &args);
    bool stop_mining(const std::vector<std::string> &args);
    bool refresh(const std::vector<std::string> &args);
    bool show_balance(const std::vector<std::string> &args = std::vector<std::string>());
    bool list_recent_transfers(const std::vector<std::string>& args);
    bool export_recent_transfers(const std::vector<std::string>& args);
    bool dump_trunsfers(const std::vector<std::string>& args);
    bool dump_key_images(const std::vector<std::string>& args);
    bool show_incoming_transfers(const std::vector<std::string> &args);
    bool show_staking_history(const std::vector<std::string>& args);
    bool show_incoming_transfers_counts(const std::vector<std::string> &args);
    bool list_outputs(const std::vector<std::string> &args);
    bool show_payments(const std::vector<std::string> &args);
    bool get_transfer_info(const std::vector<std::string> &args);    
    bool scan_for_key_image_collisions(const std::vector<std::string> &args);    
    bool fix_collisions(const std::vector<std::string> &args  );
    bool scan_transfers_for_id(const std::vector<std::string> &args);
    bool scan_transfers_for_ki(const std::vector<std::string> &args);
    bool print_utxo_distribution(const std::vector<std::string> &args);    
    bool show_blockchain_height(const std::vector<std::string> &args);
    bool show_wallet_bcheight(const std::vector<std::string> &args);    
    bool transfer(const std::vector<std::string> &args);
    bool resync_wallet(const std::vector<std::string> &args);    
    bool print_address(const std::vector<std::string> &args = std::vector<std::string>());
    bool show_seed(const std::vector<std::string> &args);
    bool spendkey(const std::vector<std::string> &args);
    bool viewkey(const std::vector<std::string> &args);
    bool save(const std::vector<std::string> &args);
    bool set_log(const std::vector<std::string> &args);
    bool enable_console_logger(const std::vector<std::string> &args);
    bool integrated_address(const std::vector<std::string> &args);
    bool get_tx_key(const std::vector<std::string> &args_);
    bool tracking_seed(const std::vector<std::string> &args_);
    bool save_watch_only(const std::vector<std::string> &args);
    bool sign_transfer(const std::vector<std::string> &args);
    bool submit_transfer(const std::vector<std::string> &args);
    bool sweep_below(const std::vector<std::string> &args);
    bool sweep_bare_outs(const std::vector<std::string> &args);
    bool tor_enable(const std::vector<std::string> &args);
    bool tor_disable(const std::vector<std::string> &args);
    bool deploy_new_asset(const std::vector<std::string> &args);
    bool add_custom_asset_id(const std::vector<std::string> &args);
    bool remove_custom_asset_id(const std::vector<std::string> &args);
    bool emit_asset(const std::vector<std::string> &args);
    bool burn_asset(const std::vector<std::string> &args);
    bool update_asset(const std::vector<std::string> &args);

    //----------------------------------------------------------------------------------------------------
    bool generate_ionic_swap_proposal(const std::vector<std::string> &args);
    bool get_ionic_swap_proposal_info(const std::vector<std::string> &args);
    bool accept_ionic_swap_proposal(const std::vector<std::string> &args);

    bool validate_wrap_status(uint64_t amount);

    bool get_alias_from_daemon(const std::string& alias_name, currency::extra_alias_entry_base& ai);
    bool get_transfer_address(const std::string& adr_str, currency::account_public_address& addr);

    uint64_t get_daemon_blockchain_height(std::string& err);
    bool try_connect_to_daemon();
    std::string get_tocken_info_string(const crypto::public_key& asset_id, uint64_t& decimal_point);
    bool print_wti(const tools::wallet_public::wallet_transfer_info& wti);
    bool check_password_for_operation();
    crypto::hash get_hash_from_pass_and_salt(const std::string& pass, uint64_t salt);

    //----------------- i_wallet2_callback ---------------------
    virtual void on_new_block(uint64_t height, const currency::block& block) override;
    virtual void on_transfer2(const tools::wallet_public::wallet_transfer_info& wti, const std::list<tools::wallet_public::asset_balance_entry>& balances, uint64_t total_mined) override;
    virtual void on_message(i_wallet2_callback::message_severity severity, const std::string& m) override;
    virtual void on_tor_status_change(const std::string& state) override;

    virtual void on_mw_get_wallets(std::vector<tools::wallet_public::wallet_entry_info>& wallets) override;
    virtual bool on_mw_select_wallet(uint64_t wallet_id) override;
    //----------------------------------------------------------

    friend class refresh_progress_reporter_t;

    class refresh_progress_reporter_t
    {
    public:
      refresh_progress_reporter_t(currency::simple_wallet& simple_wallet)
        : m_simple_wallet(simple_wallet)
        , m_blockchain_height(0)
        , m_blockchain_height_update_time()
        , m_print_time()
      {
      }

      void update(uint64_t height, bool force = false)
      {
        auto current_time = std::chrono::system_clock::now();
        if (std::chrono::seconds(DIFFICULTY_TOTAL_TARGET / 2) < current_time - m_blockchain_height_update_time || m_blockchain_height <= height)
        {
          update_blockchain_height();
          m_blockchain_height = (std::max)(m_blockchain_height, height);
        }

        if (std::chrono::milliseconds(100) < current_time - m_print_time || force)
        {
          std::cout << "Height " << height << " of " << m_blockchain_height << '\r';
          m_print_time = current_time;
        }
      }

    private:
      void update_blockchain_height()
      {
        std::string err;
        uint64_t blockchain_height = m_simple_wallet.get_daemon_blockchain_height(err);
        if (err.empty())
        {
          m_blockchain_height = blockchain_height;
          m_blockchain_height_update_time = std::chrono::system_clock::now();
        }
        else
        {
          LOG_ERROR("Failed to get current blockchain height: " << err);
        }
      }

    private:
      currency::simple_wallet& m_simple_wallet;
      uint64_t m_blockchain_height;
      std::chrono::system_clock::time_point m_blockchain_height_update_time;
      std::chrono::system_clock::time_point m_print_time;
    };

  private:
    std::string m_wallet_file;
    std::string m_generate_new;
    std::string m_generate_new_aw;
    std::string m_import_path;

    std::string m_daemon_address;
    std::string m_daemon_host;
    int m_daemon_port;
    bool m_do_refresh_after_load;
    bool m_do_not_set_date;
    bool m_do_pos_mining;
    bool m_offline_mode;
    bool m_disable_tor;
    std::string m_restore_wallet;
    std::string m_voting_config_file;
    bool m_no_password_confirmations = false;
    
    crypto::hash m_password_hash;
    uint64_t m_password_salt;

    epee::console_handlers_binder m_cmd_binder;

    std::shared_ptr<tools::wallet2> m_wallet;
    net_utils::http::http_simple_client m_http_client;
    refresh_progress_reporter_t m_refresh_progress_reporter;
  };
}
