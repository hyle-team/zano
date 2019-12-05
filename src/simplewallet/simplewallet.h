// Copyright (c) 2014-2018 Zano Project
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

    bool new_wallet(const std::string &wallet_file, const std::string& password);
    bool open_wallet(const std::string &wallet_file, const std::string& password);
    bool restore_wallet(const std::string &wallet_file, const std::string &restore_seed, const std::string& password);
    bool close_wallet();

    bool help(const std::vector<std::string> &args = std::vector<std::string>());
    bool start_mining(const std::vector<std::string> &args);
    bool stop_mining(const std::vector<std::string> &args);
    bool refresh(const std::vector<std::string> &args);
    bool show_balance(const std::vector<std::string> &args = std::vector<std::string>());
    bool list_recent_transfers(const std::vector<std::string>& args);
    bool list_recent_transfers_ex(const std::vector<std::string>& args);
    bool dump_trunsfers(const std::vector<std::string>& args);
    bool dump_key_images(const std::vector<std::string>& args);
    bool show_incoming_transfers(const std::vector<std::string> &args);
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
    bool enable_concole_logger(const std::vector<std::string> &args);
    bool integrated_address(const std::vector<std::string> &args);
    bool get_tx_key(const std::vector<std::string> &args_);
    bool save_watch_only(const std::vector<std::string> &args);
    bool sign_transfer(const std::vector<std::string> &args);
    bool submit_transfer(const std::vector<std::string> &args);

    bool get_alias_from_daemon(const std::string& alias_name, currency::extra_alias_entry_base& ai);
    bool get_transfer_address(const std::string& adr_str, currency::account_public_address& addr);

    uint64_t get_daemon_blockchain_height(std::string& err);
    bool try_connect_to_daemon();

    //----------------- i_wallet2_callback ---------------------
    virtual void on_new_block(uint64_t height, const currency::block& block);
    virtual void on_money_received(uint64_t height, const currency::transaction& tx, size_t out_index);
    virtual void on_money_spent(uint64_t height, const currency::transaction& in_tx, size_t out_index, const currency::transaction& spend_tx);
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

        if (std::chrono::milliseconds(1) < current_time - m_print_time || force)
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
    std::string m_import_path;

    std::string m_daemon_address;
    std::string m_daemon_host;
    int m_daemon_port;
    bool m_do_refresh_after_load;
    bool m_do_not_set_date;
    bool m_print_brain_wallet;
    bool m_do_pos_mining;
    bool m_offline_mode;
    std::string m_restore_wallet;

    epee::console_handlers_binder m_cmd_binder;

    std::unique_ptr<tools::wallet2> m_wallet;
    net_utils::http::http_simple_client m_http_client;
    refresh_progress_reporter_t m_refresh_progress_reporter;
  };
}
