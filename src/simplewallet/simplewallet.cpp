// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cstdlib>
#if defined(WIN32)
  #include <crtdbg.h>
#endif
#include <thread>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "include_base_utils.h"
#include "common/command_line.h"
#include "common/util.h"
#include "p2p/net_node.h"
#include "currency_protocol/currency_protocol_handler.h"
#include "simplewallet.h"
#include "currency_core/currency_format_utils.h"
#include "storages/http_abstract_invoke.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "wallet/wallet_rpc_server.h"
#include "version.h"
#include "string_coding.h"
#include "wallet/wrap_service.h"
#include "common/general_purpose_commands_defs.h"

#include "wallet/wallet_helpers.h"



using namespace std;
using namespace epee;
using namespace currency;
using boost::lexical_cast;
namespace po = boost::program_options;
namespace ph = boost::placeholders;

#define EXTENDED_LOGS_FILE "wallet_details.log"


#define SIMPLE_WALLET_BEGIN_TRY_ENTRY()     try {
#define SIMPLE_WALLET_CATCH_TRY_ENTRY()     } \
          catch (const tools::error::daemon_busy&) \
          { \
            fail_msg_writer() << "daemon is busy. Please try later"; \
          } \
          catch (const tools::error::no_connection_to_daemon&) \
          { \
            fail_msg_writer() << "no connection to daemon. Please, make sure daemon is running."; \
          } \
          catch (const tools::error::wallet_rpc_error& e) \
          { \
            LOG_ERROR("Unknown RPC error: " << e.to_string()); \
            fail_msg_writer() << "RPC error \"" << e.what() << '"'; \
          } \
          catch (const tools::error::get_random_outs_error&) \
          { \
            fail_msg_writer() << "failed to get random outputs to mix"; \
          } \
          catch (const tools::error::not_enough_money& e) \
          { \
            fail_msg_writer() << "not enough money to transfer, " << e.to_string(); \
          } \
          catch (const tools::error::not_enough_outs_to_mix& e) \
          { \
            auto writer = fail_msg_writer(); \
            writer << "not enough outputs for specified mixin_count = " << e.mixin_count() << ":"; \
            for (const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& outs_for_amount : e.scanty_outs()) \
            { \
              writer << "\noutput amount = " << print_money(outs_for_amount.amount) << ", fount outputs to mix = " << outs_for_amount.outs.size(); \
            } \
          } \
          catch (const tools::error::tx_not_constructed&) \
          { \
            fail_msg_writer() << "transaction was not constructed"; \
          } \
          catch (const tools::error::tx_rejected& e) \
          { \
            fail_msg_writer() << "transaction " << get_transaction_hash(e.tx()) << " was rejected by daemon with status \"" << e.status() << '"'; \
          } \
          catch (const tools::error::tx_sum_overflow& e) \
          { \
            fail_msg_writer() << e.what(); \
          } \
          catch (const tools::error::tx_too_big& e) \
          { \
            currency::transaction tx = e.tx(); \
            fail_msg_writer() << "transaction " << get_transaction_hash(e.tx()) << " is too big. Transaction size: " << \
              get_object_blobsize(e.tx()) << " bytes, transaction size limit: " << e.tx_size_limit() << " bytes. Try to separate this payment into few smaller transfers."; \
          } \
          catch (const tools::error::zero_destination&) \
          { \
            fail_msg_writer() << "one of destinations is zero"; \
          } \
          catch (const tools::error::transfer_error& e) \
          { \
            LOG_ERROR("unknown transfer error: " << e.to_string()); \
            fail_msg_writer() << "unknown transfer error: " << e.what(); \
          } \
          catch (const tools::error::wallet_internal_error& e) \
          { \
            LOG_ERROR("internal error: " << e.to_string()); \
            fail_msg_writer() << "internal error: " << e.what(); \
          } \
          catch (const std::exception& e) \
          { \
            LOG_ERROR("unexpected error: " << e.what()); \
            fail_msg_writer() << "unexpected error: " << e.what(); \
          } \
          catch (...) \
          { \
            LOG_ERROR("Unknown error"); \
            fail_msg_writer() << "unknown error"; \
          } \

#define CONFIRM_WITH_PASSWORD() if(!check_password_for_operation()) return true;

#define DEFAULT_WALLET_MIN_UTXO_COUNT_FOR_DEFRAGMENTATION_TX  3
#define DEFAULT_WALLET_MAX_UTXO_COUNT_FOR_DEFRAGMENTATION_TX  10


namespace
{
  const command_line::arg_descriptor<std::string>   arg_wallet_file  ("wallet-file", "Use wallet <arg>", "");
  const command_line::arg_descriptor<std::string>   arg_generate_new_wallet  ("generate-new-wallet", "Generate new wallet and save it to <arg> or <address>.wallet by default", "");
  const command_line::arg_descriptor<std::string>   arg_generate_new_auditable_wallet  ("generate-new-auditable-wallet", "Generate new auditable wallet and store it to <arg>", "");
  const command_line::arg_descriptor<std::string>   arg_daemon_address  ("daemon-address", "Use daemon instance at <host>:<port>", "");
  const command_line::arg_descriptor<std::string>   arg_daemon_host  ("daemon-host", "Use daemon instance at host <arg> instead of localhost", "");
  const command_line::arg_descriptor<std::string>   arg_password  ("password", "Wallet password");
  const command_line::arg_descriptor<bool>          arg_dont_refresh  ( "no-refresh", "Do not refresh after load");
  const command_line::arg_descriptor<bool>          arg_dont_set_date  ( "no-set-creation-date", "Do not set wallet creation date", false);
  const command_line::arg_descriptor<int>           arg_daemon_port  ("daemon-port", "Use daemon instance at port <arg> instead of default", 0);
  const command_line::arg_descriptor<bool>          arg_do_pos_mining  ( "do-pos-mining", "Do PoS mining", false);
  const command_line::arg_descriptor<std::string>   arg_pos_mining_reward_address  ( "pos-mining-reward-address", "Block reward will be sent to the giving address if specified", "" );
  const command_line::arg_descriptor<std::string>   arg_pos_mining_defrag  ( "pos-mining-defrag", "<min_outs_cnt>,<max_outs_cnt>,<amount_limit>|disable Generate defragmentation tx for small outputs each time a PoS block is found. Disabled by default. If empty string given, the default params used: " STR(DEFAULT_WALLET_MIN_UTXO_COUNT_FOR_DEFRAGMENTATION_TX) "," STR(DEFAULT_WALLET_MAX_UTXO_COUNT_FOR_DEFRAGMENTATION_TX) ",1.0", "disable" );
  const command_line::arg_descriptor<std::string>   arg_restore_wallet  ( "restore-wallet", "Restore wallet from seed phrase or tracking seed and save it to <arg>", "" );
  const command_line::arg_descriptor<bool>          arg_offline_mode  ( "offline-mode", "Don't connect to daemon, work offline (for cold-signing process)");
  const command_line::arg_descriptor<std::string>   arg_scan_for_wallet  ( "scan-for-wallet", "");
  const command_line::arg_descriptor<std::string>   arg_addr_to_compare  ( "addr-to-compare", "");
  const command_line::arg_descriptor<bool>          arg_disable_tor_relay  ( "disable-tor-relay", "Disable TOR relay", false);
  const command_line::arg_descriptor<unsigned int>  arg_set_timeout("set-timeout", "Set timeout for the wallet");
  const command_line::arg_descriptor<std::string>   arg_voting_config_file("voting-config-file", "Set voting config instead of getting if from daemon", "");
  const command_line::arg_descriptor<bool>          arg_no_password_confirmations("no-password-confirmation", "Enable/Disable password confirmation for transactions", false);

  const command_line::arg_descriptor< std::vector<std::string> > arg_command  ("command", "");

  inline std::string interpret_rpc_response(bool ok, const std::string& status)
  {
    std::string err;
    if (ok)
    {
      if (status == API_RETURN_CODE_BUSY)
      {
        err = "daemon is busy. Please try later";
      }
      else if (status != API_RETURN_CODE_OK)
      {
        err = status;
      }
    }
    else
    {
      err = "possible lost connection to daemon";
    }
    return err;
  }

  class message_writer
  {
  public:
    message_writer(epee::log_space::console_colors color = epee::log_space::console_color_default, bool bright = false,
      std::string&& prefix = std::string(), int log_level = LOG_LEVEL_2)
      : m_flush(true)
      , m_color(color)
      , m_bright(bright)
      , m_log_level(log_level)
    {
      m_oss << prefix;
    }

    message_writer(message_writer&& rhs)
      : m_flush(std::move(rhs.m_flush))
#if defined(_MSC_VER)
      , m_oss(std::move(rhs.m_oss))
#else
      // GCC bug: http://gcc.gnu.org/bugzilla/show_bug.cgi?id=54316
      , m_oss(rhs.m_oss.str(), ios_base::out | ios_base::ate) 
#endif
      , m_color(std::move(rhs.m_color))
      , m_log_level(std::move(rhs.m_log_level))
      , m_bright(false)
    {
      rhs.m_flush = false;
    }

    template<typename T>
    std::ostream& operator<<(const T& val)
    {
      m_oss << val;
      return m_oss;
    }

    ~message_writer()
    {
      NESTED_TRY_ENTRY();

      if (m_flush)
      {
        m_flush = false;

        LOG_PRINT(m_oss.str(), m_log_level)
        epee::log_space::set_console_color(m_color, m_bright);
        std::cout << m_oss.str();
        epee::log_space::reset_console_color();
        std::cout << std::endl;
      }

      NESTED_CATCH_ENTRY(__func__);
    }

  private:
    message_writer(message_writer& rhs);
    message_writer& operator=(message_writer& rhs);
    message_writer& operator=(message_writer&& rhs);

  private:
    bool m_flush;
    std::stringstream m_oss;
    epee::log_space::console_colors m_color;
    bool m_bright;
    int m_log_level;
  };

  message_writer success_msg_writer(bool color = false)
  {
    return message_writer(color ? epee::log_space::console_color_green : epee::log_space::console_color_default, false, std::string(), LOG_LEVEL_2);
  }

  message_writer fail_msg_writer()
  {
    return message_writer(epee::log_space::console_color_red, true, "Error: ", LOG_LEVEL_0);
  }
}

void display_vote_info(tools::wallet2& w)
{
  const tools::wallet_public::wallet_vote_config& votes = w.get_current_votes();
  if (votes.entries.size())
  {
    message_writer(epee::log_space::console_color_magenta, true) << "VOTING SET LOADED:";
    for (const auto& e : votes.entries)
    {
      epee::log_space::console_colors color = epee::log_space::console_color_green;
      if (e.h_end < w.get_top_block_height())
      {
        color = epee::log_space::console_color_white;
      }

      message_writer(color, true) << "\t\t" << e.proposal_id << "\t\t" << (e.vote ? "1" : "0") << "\t\t(" << e.h_start << " - " << e.h_end << ")";
    }
  }
}


std::string simple_wallet::get_commands_str()
{
  std::stringstream ss;
  ss << "Commands: " << ENDL;
  ss << m_cmd_binder.get_usage() << ENDL;
  return ss.str();
}

bool simple_wallet::help(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  success_msg_writer() << get_commands_str();
  return true;
}

simple_wallet::simple_wallet()
  : m_daemon_port(0), 
  m_do_refresh_after_load(false),
  m_do_not_set_date(false),
  m_do_pos_mining(false),
  m_refresh_progress_reporter(*this),
  m_offline_mode(false)
{
  m_cmd_binder.set_handler("start_mining", boost::bind(&simple_wallet::start_mining, this, ph::_1), "start_mining <threads_count> - Start mining in daemon");
  m_cmd_binder.set_handler("stop_mining", boost::bind(&simple_wallet::stop_mining, this, ph::_1), "Stop mining in daemon");
  m_cmd_binder.set_handler("refresh", boost::bind(&simple_wallet::refresh, this, ph::_1), "Resynchronize transactions and balance");
  m_cmd_binder.set_handler("balance", boost::bind(&simple_wallet::show_balance, this, ph::_1), "[force_all] Show current wallet balance, with 'force_all' param it displays all assets without filtering against whitelists"); 
  m_cmd_binder.set_handler("show_staking_history", boost::bind(&simple_wallet::show_staking_history, this, ph::_1), "show_staking_history [2] - Show staking transfers, if option provided - number of days for history to display");
  m_cmd_binder.set_handler("incoming_transfers", boost::bind(&simple_wallet::show_incoming_transfers, this, ph::_1), "incoming_transfers [available|unavailable] - Show incoming transfers - all of them or filter them by availability");
  m_cmd_binder.set_handler("incoming_counts", boost::bind(&simple_wallet::show_incoming_transfers_counts, this, ph::_1), "incoming_transfers counts");
  m_cmd_binder.set_handler("list_recent_transfers", boost::bind(&simple_wallet::list_recent_transfers, this, ph::_1), "list_recent_transfers [offset] [count] - Show recent maximum 1000 transfers, offset default = 0, count default = 100 ");
  m_cmd_binder.set_handler("export_recent_transfers", boost::bind(&simple_wallet::export_recent_transfers, this, ph::_1), "list_recent_transfers_tx - Write recent transfer in json to wallet_recent_transfers.txt");
  m_cmd_binder.set_handler("list_outputs", boost::bind(&simple_wallet::list_outputs, this, ph::_1), "list_outputs [spent|unspent] [ticker=ZANO] [unknown] - Lists all the outputs. The result may be filtered by spent status, asset ticker or unknown asset ids.");
  m_cmd_binder.set_handler("lo", boost::bind(&simple_wallet::list_outputs, this, ph::_1), "alias for list_outputs");
  m_cmd_binder.set_handler("dump_transfers", boost::bind(&simple_wallet::dump_trunsfers, this, ph::_1), "dump_transfers - Write  transfers in json to dump_transfers.txt");
  m_cmd_binder.set_handler("dump_keyimages", boost::bind(&simple_wallet::dump_key_images, this, ph::_1), "dump_keyimages - Write  key_images in json to dump_key_images.txt");
  m_cmd_binder.set_handler("payments", boost::bind(&simple_wallet::show_payments, this, ph::_1), "payments <payment_id_1> [<payment_id_2> ... <payment_id_N>] - Show payments <payment_id_1>, ... <payment_id_N>");
  m_cmd_binder.set_handler("bc_height", boost::bind(&simple_wallet::show_blockchain_height, this,ph::_1), "Show blockchain height");
  m_cmd_binder.set_handler("wallet_bc_height", boost::bind(&simple_wallet::show_wallet_bcheight, this,ph::_1), "Show blockchain height");
  m_cmd_binder.set_handler("transfer", boost::bind(&simple_wallet::transfer, this,ph::_1), "transfer <mixin_count> [asset_id_1:]<addr_1> <amount_1> [ [asset_id_2:]<addr_2> <amount_2> ... [asset_id_N:]<addr_N> <amount_N>] [payment_id] - Transfer <amount_1>,... <amount_N> to <address_1>,... <address_N>, respectively. <mixin_count> is the number of transactions yours is indistinguishable from (from 0 to maximum available), <payment_id> is an optional HEX-encoded string");
  m_cmd_binder.set_handler("set_log", boost::bind(&simple_wallet::set_log, this,ph::_1), "set_log <level> - Change current log detalisation level, <level> is a number 0-4");
  m_cmd_binder.set_handler("enable_console_logger", boost::bind(&simple_wallet::enable_console_logger, this,ph::_1), "Enables console logging");
  m_cmd_binder.set_handler("resync", boost::bind(&simple_wallet::resync_wallet, this,ph::_1), "Causes wallet to reset all transfers and re-synchronize wallet");
  m_cmd_binder.set_handler("help", boost::bind(&simple_wallet::help, this,ph::_1), "Show this help");
  m_cmd_binder.set_handler("get_transfer_info", boost::bind(&simple_wallet::get_transfer_info, this,ph::_1), "displays transfer info by key_image or index");
  m_cmd_binder.set_handler("scan_for_collision", boost::bind(&simple_wallet::scan_for_key_image_collisions, this,ph::_1), "Rescan transfers for key image collisions");
  m_cmd_binder.set_handler("fix_collisions", boost::bind(&simple_wallet::fix_collisions, this,ph::_1), "Rescan transfers for key image collisions");
  m_cmd_binder.set_handler("scan_transfers_for_id", boost::bind(&simple_wallet::scan_transfers_for_id, this,ph::_1), "Rescan transfers for tx_id");
  m_cmd_binder.set_handler("scan_transfers_for_ki", boost::bind(&simple_wallet::scan_transfers_for_ki, this,ph::_1), "Rescan transfers for key image");
  m_cmd_binder.set_handler("print_utxo_distribution", boost::bind(&simple_wallet::print_utxo_distribution, this,ph::_1), "Prints utxo distribution");
  m_cmd_binder.set_handler("sweep_below", boost::bind(&simple_wallet::sweep_below, this,ph::_1), "sweep_below <mixin_count> <address> <amount_lower_limit> [payment_id] -  Tries to transfers all coins with amount below the given limit to the given address");
  m_cmd_binder.set_handler("sweep_bare_outs", boost::bind(&simple_wallet::sweep_bare_outs, this,ph::_1), "sweep_bare_outs - Transfers all bare unspent outputs to itself. Uses several txs if necessary.");
  
  m_cmd_binder.set_handler("address", boost::bind(&simple_wallet::print_address, this,ph::_1), "Show current wallet public address");
  m_cmd_binder.set_handler("integrated_address", boost::bind(&simple_wallet::integrated_address, this,ph::_1), "integrated_address [<payment_id>|<integrated_address] - encodes given payment_id along with wallet's address into an integrated address (random payment_id will be used if none is provided). Decodes given integrated_address into standard address");
  m_cmd_binder.set_handler("show_seed", boost::bind(&simple_wallet::show_seed, this,ph::_1), "Display secret 24 word phrase that could be used to recover this wallet");
  m_cmd_binder.set_handler("spendkey", boost::bind(&simple_wallet::spendkey, this,ph::_1), "Display secret spend key");
  m_cmd_binder.set_handler("viewkey",  boost::bind(&simple_wallet::viewkey, this,ph::_1), "Display secret view key");
  
  m_cmd_binder.set_handler("get_tx_key", boost::bind(&simple_wallet::get_tx_key, this,ph::_1), "Get transaction one-time secret key (r) for a given <txid>");

  m_cmd_binder.set_handler("tracking_seed", boost::bind(&simple_wallet::tracking_seed, this,ph::_1), "For auditable wallets: prints tracking seed for wallet's audit by a third party");

  m_cmd_binder.set_handler("save", boost::bind(&simple_wallet::save, this,ph::_1), "Save wallet synchronized data");
  m_cmd_binder.set_handler("save_watch_only", boost::bind(&simple_wallet::save_watch_only, this,ph::_1), "save_watch_only <filename> <password> - save as watch-only wallet file.");

  m_cmd_binder.set_handler("sign_transfer", boost::bind(&simple_wallet::sign_transfer, this,ph::_1), "sign_transfer <unsgined_tx_file> <signed_tx_file> - sign unsigned tx from a watch-only wallet");
  m_cmd_binder.set_handler("submit_transfer", boost::bind(&simple_wallet::submit_transfer, this,ph::_1), "submit_transfer <signed_tx_file> - broadcast signed tx");
  m_cmd_binder.set_handler("export_history", boost::bind(&simple_wallet::submit_transfer, this,ph::_1), "Export transaction history in CSV file");
  m_cmd_binder.set_handler("tor_enable", boost::bind(&simple_wallet::tor_enable, this, ph::_1), "Enable relaying transactions over TOR network(enabled by default)");
  m_cmd_binder.set_handler("tor_disable", boost::bind(&simple_wallet::tor_disable, this, ph::_1), "Enable relaying transactions over TOR network(enabled by default)");
  m_cmd_binder.set_handler("deploy_new_asset", boost::bind(&simple_wallet::deploy_new_asset, this, ph::_1), "deploy_new_asset <json_filename> - Deploys new asset in the network, with current wallet as a maintainer");
  m_cmd_binder.set_handler("emit_asset", boost::bind(&simple_wallet::emit_asset, this, ph::_1), "emit_asset <asset_id> <amount> - Emmit more coins for the asset, possible only if current wallet is a maintainer for the asset");
  m_cmd_binder.set_handler("burn_asset", boost::bind(&simple_wallet::burn_asset, this, ph::_1), "burn_asset <asset_id> <amount> - Burn coins for the asset, possible only if current wallet is a maintainer for the asset AND possess given amount of coins to burn");
  m_cmd_binder.set_handler("update_asset", boost::bind(&simple_wallet::update_asset, this, ph::_1), "update_asset <asset_id> <path_to_metadata_file> - Update asset descriptor's metadata, possible only if current wallet is a maintainer for the asset");

  m_cmd_binder.set_handler("add_custom_asset_id", boost::bind(&simple_wallet::add_custom_asset_id, this, ph::_1), "Approve asset id to be recognized in the wallet and returned in balances");
  m_cmd_binder.set_handler("remove_custom_asset_id", boost::bind(&simple_wallet::remove_custom_asset_id, this, ph::_1), "Cancel previously made approval for asset id");

  m_cmd_binder.set_handler("generate_ionic_swap_proposal", boost::bind(&simple_wallet::generate_ionic_swap_proposal, this, ph::_1), "generate_ionic_swap_proposal <proposal_config.json> <destination_addr>- Generates ionic_swap proposal with given conditions");
  m_cmd_binder.set_handler("get_ionic_swap_proposal_info", boost::bind(&simple_wallet::get_ionic_swap_proposal_info, this, ph::_1), "get_ionic_swap_proposal_info <hex_encoded_raw_proposal.txt> - Extracts and display information from ionic_swap proposal raw data");
  m_cmd_binder.set_handler("accept_ionic_swap_proposal", boost::bind(&simple_wallet::accept_ionic_swap_proposal, this, ph::_1), "accept_ionic_swap_proposal <hex_encoded_raw_proposal.txt> - Accept ionic_swap proposal and generates exchange transaction");

}
//----------------------------------------------------------------------------------------------------
simple_wallet::~simple_wallet()
{
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::enable_console_logger(const std::vector<std::string> &args)
{
  log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_0);
  LOG_PRINT_L0("Console logger enabled");
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_log(const std::vector<std::string> &args)
{
  if(args.size() != 1) 
  {
    fail_msg_writer() << "use: set_log <log_level_number_0-4>";
    return true;
  }
  uint16_t l = 0;
  if(!string_tools::get_xtype_from_string(l, args[0]))
  {
    fail_msg_writer() << "wrong number format, use: set_log <log_level_number_0-4>";
    return true;
  }
  if(LOG_LEVEL_4 < l)
  {
    fail_msg_writer() << "wrong number range, use: set_log <log_level_number_0-4>";
    return true;
  }

  log_space::log_singletone::get_set_log_detalisation_level(true, l);
  return true;
}
//----------------------------------------------------------------------------------------------------
void process_wallet_command_line_params(const po::variables_map& vm, tools::wallet2& wal, bool is_server_mode = true)
{
  if (command_line::has_arg(vm, arg_disable_tor_relay))
  {
    wal.set_disable_tor_relay(command_line::get_arg(vm, arg_disable_tor_relay));
    message_writer(epee::log_space::console_color_default, true, std::string(), LOG_LEVEL_0) << "Notice: Relaying transactions over TOR disabled with command line parameter";
  }
  else
  {
    if (is_server_mode)
    {
      //disable TOR by default for server-mode, to avoid potential sporadic errors due to TOR connectivity fails
      wal.set_disable_tor_relay(true);
    }
  }

  if (command_line::has_arg(vm, arg_set_timeout))
  {
    wal.set_connectivity_options(command_line::get_arg(vm, arg_set_timeout));
  }

}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::init(const boost::program_options::variables_map& vm)
{
  handle_command_line(vm);

  if (!m_daemon_address.empty() && !m_daemon_host.empty() && 0 != m_daemon_port)
  {
    fail_msg_writer() << "you can't specify daemon host or port several times";
    return false;
  }

  if (m_wallet_file.empty() && m_generate_new.empty() && m_restore_wallet.empty() && m_generate_new_aw.empty())
  {
    fail_msg_writer() << "you must specify --wallet-file, --generate-new-wallet, --generate-new-auditable-wallet, --restore-wallet";
    return false;
  }

  if (m_daemon_host.empty())
    m_daemon_host = "localhost";
  if (!m_daemon_port)
    m_daemon_port = RPC_DEFAULT_PORT;
  if (m_daemon_address.empty())
    m_daemon_address = std::string("http://") + m_daemon_host + ":" + std::to_string(m_daemon_port);

  tools::password_container pwd_container;
  if (command_line::has_arg(vm, arg_password))
  {
    pwd_container.password(command_line::get_arg(vm, arg_password));
  }
  else
  {
    bool r = pwd_container.read_password();
    if (!r)
    {
      fail_msg_writer() << "failed to read wallet password";
      return false;
    }
  }

  m_do_refresh_after_load = true;
  if (command_line::has_arg(vm, arg_dont_refresh))
  {
    m_do_refresh_after_load = false;
  }

  m_password_salt = crypto::rand<uint64_t>();
  m_password_hash = get_hash_from_pass_and_salt(pwd_container.password(), m_password_salt);

  bool was_open = false;
  if (!m_generate_new.empty())
  {
    bool r = new_wallet(m_generate_new, pwd_container.password(), false);
    CHECK_AND_ASSERT_MES(r, false, "failed to create new wallet");
  }
  else if (!m_generate_new_aw.empty())
  {
    bool r = new_wallet(m_generate_new_aw, pwd_container.password(), true);
    CHECK_AND_ASSERT_MES(r, false, "failed to create new auditable wallet");
  }
  else if (!m_restore_wallet.empty())
  {
    if (boost::filesystem::exists(m_restore_wallet))
    {
      fail_msg_writer() << "file " << m_restore_wallet << " already exists";
      return false;
    }

    tools::password_container restore_seed_container;
    if (!restore_seed_container.read_password("please, enter wallet seed phrase or an auditable wallet's tracking seed:\n"))
    {
      fail_msg_writer() << "failed to read seed phrase";
      return false;
    }
    bool looks_like_tracking_seed = restore_seed_container.password().find(':') != std::string::npos;
    tools::password_container seed_password_container;

    if (!looks_like_tracking_seed)
    {
      bool is_password_protected = false;
      bool sr = account_base::is_seed_password_protected(restore_seed_container.password(), is_password_protected);
      if (!sr)
      {
        fail_msg_writer() << "failed to parse seed phrase";
        return false;
      }

      if (is_password_protected && !seed_password_container.read_password("This seed is secured, to use it please enter the password:\n"))
      {
        fail_msg_writer() << "failed to read seed phrase";
        return false;
      }
    }
    bool r = restore_wallet(m_restore_wallet, restore_seed_container.password(), pwd_container.password(), looks_like_tracking_seed, seed_password_container.password());
    CHECK_AND_ASSERT_MES(r, false, "wallet restoring failed");
  }
  else
  {
    bool r = open_wallet(m_wallet_file, pwd_container.password());
    CHECK_AND_ASSERT_MES(r, false, "could not open account");
    was_open = true;
  }
  process_wallet_command_line_params(vm, *m_wallet, false);

  m_wallet->init(m_daemon_address);

  if (was_open && (m_do_refresh_after_load && !m_offline_mode))
    refresh(std::vector<std::string>());

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::deinit()
{
  if (!m_wallet.get())
    return true;

  return close_wallet();
}
//----------------------------------------------------------------------------------------------------
crypto::hash simple_wallet::get_hash_from_pass_and_salt(const std::string& pass, uint64_t salt)
{
  std::string pass_and_salt = pass + std::to_string(salt);
  return crypto::cn_fast_hash(pass_and_salt.data(), pass_and_salt.size());
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::handle_command_line(const boost::program_options::variables_map& vm)
{
  m_wallet_file     = command_line::get_arg(vm, arg_wallet_file);
  m_generate_new    = command_line::get_arg(vm, arg_generate_new_wallet);
  m_generate_new_aw = command_line::get_arg(vm, arg_generate_new_auditable_wallet);
  m_daemon_address  = command_line::get_arg(vm, arg_daemon_address);
  m_daemon_host     = command_line::get_arg(vm, arg_daemon_host);
  m_daemon_port     = command_line::get_arg(vm, arg_daemon_port);
  m_do_not_set_date = command_line::get_arg(vm, arg_dont_set_date);
  m_do_pos_mining   = command_line::get_arg(vm, arg_do_pos_mining);
  m_restore_wallet  = command_line::get_arg(vm, arg_restore_wallet);
  m_disable_tor     = command_line::get_arg(vm, arg_disable_tor_relay);
  m_voting_config_file = command_line::get_arg(vm, arg_voting_config_file);
  m_no_password_confirmations = command_line::get_arg(vm, arg_no_password_confirmations);  
} 
//----------------------------------------------------------------------------------------------------

#define PASSWORD_CONFIRMATION_ATTEMPTS 3
bool simple_wallet::check_password_for_operation()
{
  if (m_no_password_confirmations)
    return true;
  for (size_t i = 0; i != PASSWORD_CONFIRMATION_ATTEMPTS; i++)
  {
    tools::password_container pass_container;
    if (!pass_container.read_password("Enter password to confirm operation:\n"))
    {
      fail_msg_writer() << "Failed to read password";
      return false;
    }
    if (get_hash_from_pass_and_salt(pass_container.password(), m_password_salt) != m_password_hash)
    {
      fail_msg_writer() << "Wrong password";
      continue;
    }
    return true;
  }

  fail_msg_writer() << "Confirmation failed with " << PASSWORD_CONFIRMATION_ATTEMPTS << " attempts";
  return false;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::try_connect_to_daemon()
{
  if (!m_wallet->check_connection())
  {
    fail_msg_writer() << "wallet failed to connect to daemon (" << m_daemon_address << "). " <<
      "Daemon either is not started or passed wrong port. " <<
      "Please, make sure that daemon is running or restart the wallet with correct daemon address.";
    return false;
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::new_wallet(const string &wallet_file, const std::string& password, bool create_auditable_wallet)
{
  m_wallet_file = wallet_file;

  m_wallet.reset(new tools::wallet2());
  m_wallet->callback(this->shared_from_this());
  if (!m_voting_config_file.empty())
    m_wallet->set_votes_config_path(m_voting_config_file);

  m_wallet->set_do_rise_transfer(false);
  try
  {
    m_wallet->generate(epee::string_encoding::utf8_to_wstring(m_wallet_file), password, create_auditable_wallet);
    message_writer(epee::log_space::console_color_white, true) << "Generated new " << (create_auditable_wallet ? "AUDITABLE" : "") << " wallet: " << m_wallet->get_account().get_public_address_str();
    display_vote_info(*m_wallet);
    std::cout << "view key: " << string_tools::pod_to_hex(m_wallet->get_account().get_keys().view_secret_key) << std::endl << std::flush;
    if (m_wallet->is_auditable())
      std::cout << "tracking seed: " << std::endl << m_wallet->get_account().get_tracking_seed() << std::endl << std::flush;
    if (m_do_not_set_date)
      m_wallet->reset_creation_time(0);

  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << "failed to generate new wallet: " << e.what();
    return false;
  }

  success_msg_writer() <<
    "**********************************************************************\n" <<
    "Your wallet has been generated.\n" <<
    "**********************************************************************";
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::restore_wallet(const std::string& wallet_file, const std::string& seed_or_tracking_seed, const std::string& password, bool tracking_wallet, const std::string& seed_password)
{
  m_wallet_file = wallet_file;

  m_wallet.reset(new tools::wallet2());
  m_wallet->callback(this->shared_from_this());
  if (!m_voting_config_file.empty())
    m_wallet->set_votes_config_path(m_voting_config_file);

  m_wallet->set_do_rise_transfer(true);
  try
  {
    if (tracking_wallet)
    {
      // auditable watch-only aka tracking wallet
      m_wallet->restore(epee::string_encoding::utf8_to_wstring(wallet_file), password, seed_or_tracking_seed, true, "");
      message_writer(epee::log_space::console_color_white, true) << "Tracking wallet restored: " << m_wallet->get_account().get_public_address_str();
    }
    else
    {
      // normal or auditable wallet
      m_wallet->restore(epee::string_encoding::utf8_to_wstring(wallet_file), password, seed_or_tracking_seed, false, seed_password);
      message_writer(epee::log_space::console_color_white, true) << (m_wallet->is_auditable() ? "Auditable wallet" : "Wallet") << " restored: " << m_wallet->get_account().get_public_address_str();
      std::cout << "view key: " << string_tools::pod_to_hex(m_wallet->get_account().get_keys().view_secret_key) << std::endl << std::flush;
      if (m_wallet->is_auditable())
        std::cout << "tracking seed: " << std::endl << m_wallet->get_account().get_tracking_seed() << std::endl << std::flush;
    }
    display_vote_info(*m_wallet);
    if (m_do_not_set_date)
      m_wallet->reset_creation_time(0);
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << "failed to restore wallet, check your " << (tracking_wallet ? "tracking seed!" : "seed phrase!") << ENDL << e.what();
    return false;
  }
  catch (...)
  {
    fail_msg_writer() << "failed to restore wallet, check your " << (tracking_wallet ? "tracking seed!" : "seed phrase!") << ENDL;
    return false;
  }

  success_msg_writer() <<
    "**********************************************************************\n" <<
    "Your wallet has been restored.\n" <<
    "To start synchronizing with the daemon use \"refresh\" command.\n" <<
    "Use \"help\" command to see the list of available commands.\n" <<
    "Always use \"exit\" command when closing simplewallet to save\n" <<
    "current session's state. Otherwise, you will possibly need to synchronize \n" <<
    "your wallet again. Your wallet keys is NOT under risk anyway.\n" <<
    "**********************************************************************";
  return true;
}
//----------------------------------------------------------------------------------------------------

bool simple_wallet::open_wallet(const string &wallet_file, const std::string& password)
{
  m_wallet_file = wallet_file;
  m_wallet.reset(new tools::wallet2());
  m_wallet->callback(shared_from_this());
  if (!m_voting_config_file.empty())
    m_wallet->set_votes_config_path(m_voting_config_file);


  while (true)
  {
    try
    {
      m_wallet->load(epee::string_encoding::utf8_to_wstring(m_wallet_file), password);
      message_writer(epee::log_space::console_color_white, true) << "Opened" << (m_wallet->is_auditable() ? " auditable" : "") << (m_wallet->is_watch_only() ? " watch-only" : "") << " wallet: " << m_wallet->get_account().get_public_address_str();
      display_vote_info(*m_wallet);

      break;
    }
    catch (const tools::error::wallet_load_notice_wallet_restored& /*e*/)
    {
      message_writer(epee::log_space::console_color_white, true) << "Opened wallet: " << m_wallet->get_account().get_public_address_str();
      message_writer(epee::log_space::console_color_red, true) << "NOTICE: Wallet file was damaged and restored.";
      break;
    }
    catch (const std::exception& e)
    {
      fail_msg_writer() << "failed to load wallet: " << e.what();
      return false;
    }
  }

  success_msg_writer() <<
    "**********************************************************************\n" <<
    "Use \"help\" command to see the list of available commands.\n" <<
    "**********************************************************************";
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::close_wallet()
{
  bool r = m_wallet->deinit();
  if (!r)
  {
    fail_msg_writer() << "failed to deinit wallet";
    return false;
  }

  try
  {
    m_wallet->store();
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << e.what();
    return false;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::save(const std::vector<std::string> &args)
{
  try
  {
    m_wallet->store();
    success_msg_writer() << "Wallet data saved";
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << e.what();
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::start_mining(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  COMMAND_RPC_START_MINING::request req;
  req.miner_address = m_wallet->get_account().get_public_address_str();

  if (0 == args.size())
  {
    req.threads_count = 1;
  }
  else if (1 == args.size())
  {
    uint16_t num;
    bool ok = string_tools::get_xtype_from_string(num, args[0]);
    if(!ok || 0 == num)
    {
      fail_msg_writer() << "wrong number of mining threads: \"" << args[0] << "\"";
      return true;
    }
    req.threads_count = num;
  }
  else
  {
    fail_msg_writer() << "wrong number of arguments, expected the number of mining threads";
    return true;
  }

  COMMAND_RPC_START_MINING::response res;
  bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/start_mining", req, res, m_http_client);
  std::string err = interpret_rpc_response(r, res.status);
  if (err.empty())
    success_msg_writer() << "Mining started in daemon";
  else
    fail_msg_writer() << "mining has NOT been started: " << err;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::stop_mining(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  COMMAND_RPC_STOP_MINING::request req;
  COMMAND_RPC_STOP_MINING::response res;
  bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/stop_mining", req, res, m_http_client);
  std::string err = interpret_rpc_response(r, res.status);
  if (err.empty())
    success_msg_writer() << "Mining stopped in daemon";
  else
    fail_msg_writer() << "mining has NOT been stopped: " << err;
  return true;
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_new_block(uint64_t height, const currency::block& block)
{
  m_refresh_progress_reporter.update(height, false);
}
//----------------------------------------------------------------------------------------------------
std::string print_money_trailing_zeros_replaced_with_spaces(uint64_t amount, size_t decimal_point = CURRENCY_DISPLAY_DECIMAL_POINT)
{
  std::string s = print_money(amount);
  size_t p = s.find_last_not_of('0');
  if (p != std::string::npos)
  {
    if (s[p] == '.')
      ++p;
    size_t l = s.length() - p - 1;
    return s.replace(p + 1, l, l, ' ');
  }
  return s;
}
//----------------------------------------------------------------------------------------------------
std::string simple_wallet::get_tocken_info_string(const crypto::public_key& asset_id, uint64_t& decimal_points)
{
  std::string token_info = "ZANO";
  decimal_points = CURRENCY_DISPLAY_DECIMAL_POINT;
  if (asset_id != currency::native_coin_asset_id)
  {
    currency::asset_descriptor_base adb = AUTO_VAL_INIT(adb);
    bool whitelisted = false;
    if (!m_wallet->get_asset_id_info(asset_id, adb, whitelisted))
    {
      token_info = "!UNKNOWN!";
    }
    else {
      decimal_points = adb.decimal_point;
      token_info = adb.ticker;

      if (whitelisted)
      {
        token_info += "[*]";
      }
      else
      {
        token_info += std::string("[") + epee::string_tools::pod_to_hex(asset_id) + "]";
      }
    }
  }
  return token_info;
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_transfer2(const tools::wallet_public::wallet_transfer_info& wti, const std::list<tools::wallet_public::asset_balance_entry>& balances, uint64_t total_mined)
{

  if (wti.subtransfers.size() == 1)
  {
    epee::log_space::console_colors color = !wti.has_outgoing_entries() ? epee::log_space::console_color_green : epee::log_space::console_color_magenta;
    uint64_t decimal_points = CURRENCY_DISPLAY_DECIMAL_POINT;
    std::string token_info = get_tocken_info_string(wti.subtransfers[0].asset_id, decimal_points);
    message_writer(color, false) <<
      "height " << wti.height <<
      ", tx " << wti.tx_hash <<
      " " << std::right << std::setw(18) << print_money_trailing_zeros_replaced_with_spaces(wti.subtransfers[0].amount, decimal_points) << (wti.subtransfers[0].is_income ? " received," : " spent") << " " << token_info;
  }
  else
  {
    message_writer(epee::log_space::console_color_cyan, false) <<
      "height " << wti.height <<
      ", tx " << wti.tx_hash;
    for (const auto& st : wti.subtransfers)
    {
      epee::log_space::console_colors color = st.is_income ? epee::log_space::console_color_green : epee::log_space::console_color_magenta;
      uint64_t decimal_points = CURRENCY_DISPLAY_DECIMAL_POINT;
      std::string token_info = get_tocken_info_string(st.asset_id, decimal_points);

      message_writer(epee::log_space::console_color_cyan, false) << "       " 
        << std::right << std::setw(18) << print_money_trailing_zeros_replaced_with_spaces(st.amount, decimal_points) << (st.is_income ? " received," : " spent") << " " << token_info;
    }
  }

  m_refresh_progress_reporter.update(wti.height, true);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_message(i_wallet2_callback::message_severity severity, const std::string& m)
{
  epee::log_space::console_colors color = epee::log_space::console_color_white;
  if (severity == i_wallet2_callback::ms_red)
    color = epee::log_space::console_color_red;
  else if (severity == i_wallet2_callback::ms_yellow)
    color = epee::log_space::console_color_yellow;

  message_writer(color, true, std::string()) << m;
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_tor_status_change(const std::string& state)
{
  std::string human_message;
  if (state == TOR_LIB_STATE_INITIALIZING)
    human_message = "Initializing...";
  else if (state == TOR_LIB_STATE_DOWNLOADING_CONSENSUS)
    human_message = "Downloading consensus...";
  else if (state == TOR_LIB_STATE_MAKING_TUNNEL_A)
    human_message = "Building tunnel to A...";
  else if (state == TOR_LIB_STATE_MAKING_TUNNEL_B)
    human_message = "Building tunnel to B...";
  else if (state == TOR_LIB_STATE_CREATING_STREAM)
    human_message = "Creating stream...";
  else if (state == TOR_LIB_STATE_SUCCESS)
    human_message = "Successfully created stream";
  else if (state == TOR_LIB_STATE_FAILED)
    human_message = "Failed created stream";
  else if (state == WALLET_LIB_STATE_SENDING)
    human_message = "Sending transaction...";
  else if (state == WALLET_LIB_SENT_SUCCESS)
    human_message = "Successfully sent!";
  else if (state == WALLET_LIB_SEND_FAILED)
    human_message = "Sending failed";


  message_writer(epee::log_space::console_color_yellow, true, std::string("[TOR]: ")) << human_message;
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_mw_get_wallets(std::vector<tools::wallet_public::wallet_entry_info>& wallets)
{
  wallets.resize(1);
  tools::get_wallet_info(*m_wallet, wallets[0].wi);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::on_mw_select_wallet(uint64_t wallet_id)
{
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::refresh(const std::vector<std::string>& args)
{
  if (m_offline_mode)
  {
    success_msg_writer() << "refresh is meaningless in OFFLINE MODE";
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  message_writer() << "Starting refresh...";
  size_t fetched_blocks = 0;
  bool ok = false;
  std::ostringstream ss;
  try
  {
    m_wallet->refresh(fetched_blocks);
    ok = true;
    // Clear line "Height xxx of xxx"
    std::cout << "\r                                                                \r";
    success_msg_writer(true) << "Refresh done, blocks received: " << fetched_blocks;
    show_balance();
  }
  catch (const tools::error::daemon_busy&)
  {
    ss << "daemon is busy. Please try later";
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    ss << "no connection to daemon. Please, make sure daemon is running";
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("Unknown RPC error: " << e.to_string());
    ss << "RPC error \"" << e.what() << '"';
  }
  catch (const tools::error::refresh_error& e)
  {
    LOG_ERROR("refresh error: " << e.to_string());
    ss << e.what();
  }
  catch (const tools::error::wallet_internal_error& e)
  {
    LOG_ERROR("internal error: " << e.to_string());
    ss << "internal error: " << e.what();
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    ss << "unexpected error: " << e.what();
  }
  catch (...)
  {
    LOG_ERROR("Unknown error");
    ss << "unknown error";
  }

  if (!ok)
  {
    fail_msg_writer() << "refresh failed: " << ss.str() << ". Blocks received: " << fetched_blocks;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_balance(const std::vector<std::string>& args /* = std::vector<std::string>()*/)
{
  if (args.size() == 1 && args[0] == "raw")
  {
    success_msg_writer() << m_wallet->get_balance_str_raw();
  }
  else
  {
    success_msg_writer() << m_wallet->get_balance_str();
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::print_wti(const tools::wallet_public::wallet_transfer_info& wti)
{
  epee::log_space::console_colors cl;
  if (!wti.has_outgoing_entries())
    cl = epee::log_space::console_color_green;
  else
    cl = epee::log_space::console_color_magenta;

  std::string payment_id_placeholder;
  if (wti.payment_id.size())
    payment_id_placeholder = std::string("(payment_id:") + epee::string_tools::buff_to_hex_nodelimer(wti.payment_id) + ")";

  static const std::string separator = ", ";
  std::string remote_side;
  if (!wti.remote_aliases.empty())
  {
    for (auto it : wti.remote_aliases)
      remote_side += remote_side.empty() ? it : (separator + it);
  }
  else
  {
    for (auto it : wti.remote_addresses)
      remote_side += remote_side.empty() ? it : (separator + it);
  }

  if (wti.subtransfers.size() == 1)
  {
    success_msg_writer(cl) << "[" << wti.transfer_internal_index << "]" << epee::misc_utils::get_time_str_v2(wti.timestamp) << " "
      << (wti.subtransfers[0].is_income ? "Received " : "Sent    ")
      << print_money(wti.subtransfers[0].amount) << "(fee:" << print_money(wti.fee) << ")  "
      << remote_side
      << " " << wti.tx_hash << payment_id_placeholder;
  }else
  {
    success_msg_writer(cl) << "[" << wti.transfer_internal_index << "]" << epee::misc_utils::get_time_str_v2(wti.timestamp) << " (fee:" << print_money(wti.fee) << ")  "
      << remote_side
      << " " << wti.tx_hash << payment_id_placeholder;
    for (auto& st: wti.subtransfers)
    {
      epee::log_space::console_colors cl = st.is_income ? epee::log_space::console_color_green: epee::log_space::console_color_magenta;
      uint64_t decimal_points = CURRENCY_DISPLAY_DECIMAL_POINT;
      std::string token_info = get_tocken_info_string(st.asset_id, decimal_points);

      success_msg_writer(cl) 
        << (st.is_income ? "Received " : "Sent    ")
        << print_money(st.amount, decimal_points) << token_info;
    }
  
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::list_recent_transfers(const std::vector<std::string>& args)
{
  std::vector<tools::wallet_public::wallet_transfer_info> unconfirmed;
  std::vector<tools::wallet_public::wallet_transfer_info> recent;
  uint64_t offset = 0;
  if (args.size() > 0)
    offset = std::stoll(args[0]);
  uint64_t count = 1000; 
  if (args.size() > 1)
    count = std::stoll(args[1]);
  uint64_t total = 0;
  uint64_t last_index = 0;
  m_wallet->get_recent_transfers_history(recent, offset, count, total, last_index, false, false);
  m_wallet->get_unconfirmed_transfers(unconfirmed, false);
  //workaround for missed fee
  
  success_msg_writer() << "Unconfirmed transfers: ";
  for (auto & wti : unconfirmed)
  {
    wti.fee = currency::get_tx_fee(wti.tx);
    print_wti(wti);
  }
  success_msg_writer() << "Recent transfers: ";
  for (auto & wti : recent)
  {
    wti.fee = currency::get_tx_fee(wti.tx);
    print_wti(wti);
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
std::string wti_to_text_line(const tools::wallet_public::wallet_transfer_info& wti)
{
  stringstream ss;

  return ss.str();
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::export_recent_transfers(const std::vector<std::string>& args)
{
  bool export_to_json = true;
  bool ignore_pos = false;
  if (args.size() > 1)
  {
    if (args[1] == "ignore-pos")
      ignore_pos = true;
  }

  std::string format = "csv";
  if (args.size() > 0)
  {
    if (args[0] == "json" || args[0] == "csv" || args[0] == "text")
    {
      format = args[0];
    }
    else
    {
      fail_msg_writer() << "Unknown format: \"" << args[0] << "\", only \"csv\"(default), \"json\" and \"text\" supported";
    }
  }

  try {
    boost::filesystem::ofstream fstream;
    fstream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fstream.open(log_space::log_singletone::get_default_log_folder() + "/wallet_recent_transfers.txt", std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
    m_wallet->export_transaction_history(fstream, format, !ignore_pos);
    fstream.close();
  }
  catch (...)
  {
    success_msg_writer() << "Failed";
    return false;
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::dump_trunsfers(const std::vector<std::string>& args)
{

  stringstream ss;
  success_msg_writer() << "Generating text....";
  m_wallet->dump_trunsfers(ss);
  success_msg_writer() << "Storing text to dump_transfers.txt....";
  file_io_utils::save_string_to_file(log_space::log_singletone::get_default_log_folder() + "/dump_transfers.txt", ss.str());
  success_msg_writer() << "Done....";
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::dump_key_images(const std::vector<std::string>& args)
{

  stringstream ss;
  success_msg_writer() << "Generating text....";
  m_wallet->dump_key_images(ss);
  success_msg_writer() << "Storing text to dump_keyimages.txt....";
  file_io_utils::save_string_to_file(log_space::log_singletone::get_default_log_folder() + "/dump_keyimages.txt", ss.str());
  success_msg_writer() << "Done....";
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_staking_history(const std::vector<std::string>& args)
{
  uint64_t n_days = 0;
  if (!args.empty())
  {
    if (!epee::string_tools::get_xtype_from_string(n_days, args[0]))
    {
      fail_msg_writer() << "Unknown amount of days to list";
      return true;
    }
  }

  tools::transfer_container transfers;
  m_wallet->get_transfers(transfers);

  uint64_t timestamp = 0;

  if (n_days)
    timestamp = static_cast<uint64_t>(time(nullptr)) - (n_days * 60 * 60 * 24);
  
  uint64_t amount_total_staked = 0;
  bool transfers_found = false;
  for (auto it = transfers.rbegin(); it != transfers.rend(); it++)
  {
    const auto& td = *it;

    if (timestamp && td.m_ptx_wallet_info->m_block_timestamp < timestamp)
      break;

    if (!(td.m_flags&WALLET_TRANSFER_DETAIL_FLAG_MINED_TRANSFER))
      continue;

    bool pos_coinbase = false;
    is_coinbase(td.m_ptx_wallet_info->m_tx, pos_coinbase);
    if (!pos_coinbase)
      continue;
  
    if (!transfers_found)
    {
      message_writer() << "        amount       \tspent\tglobal index\t                              tx id";
      transfers_found = true;
    }
    amount_total_staked += td.amount();
    message_writer(static_cast<bool>(td.m_flags&WALLET_TRANSFER_DETAIL_FLAG_SPENT) ? epee::log_space::console_color_magenta : epee::log_space::console_color_green, false) <<
      std::setw(21) << print_money(td.amount()) << '\t' <<
      std::setw(3) << (static_cast<bool>(td.m_flags&WALLET_TRANSFER_DETAIL_FLAG_SPENT) ? 'T' : 'F') << "  \t" <<
      std::setw(12) << td.m_global_output_index << '\t' <<
      get_transaction_hash(td.m_ptx_wallet_info->m_tx) << "[" << td.m_ptx_wallet_info->m_block_height << "] unlocked: " << (m_wallet->is_transfer_unlocked(td) ? 'T' : 'F');
  }

  if (!transfers_found)
  {
    success_msg_writer() << "No staking transactions";
  }
  else
  {
    success_msg_writer() << "Total staked: " << print_money(amount_total_staked);
  }

  

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_incoming_transfers(const std::vector<std::string>& args)
{
  bool filter = false;
  bool available = false;
  if (!args.empty())
  {
    if (args[0] == "available")
    {
      filter = true;
      available = true;
    }
    else if (args[0] == "unavailable")
    {
      filter = true;
      available = false;
    }
  }

  tools::transfer_container transfers;
  m_wallet->get_transfers(transfers);

  bool transfers_found = false;
  for (const auto& td : transfers)
  {
    if (!filter || available != static_cast<bool>(td.m_flags&WALLET_TRANSFER_DETAIL_FLAG_SPENT))
    {
      if (!transfers_found)
      {
        message_writer() << "        amount       \tspent\tglobal index\t                              tx id";
        transfers_found = true;
      }
      message_writer(static_cast<bool>(td.m_flags&WALLET_TRANSFER_DETAIL_FLAG_SPENT) ? epee::log_space::console_color_magenta : epee::log_space::console_color_green, false) <<
        std::setw(21) << print_money(td.amount()) << '\t' <<
        std::setw(3) << (static_cast<bool>(td.m_flags&WALLET_TRANSFER_DETAIL_FLAG_SPENT) ? 'T' : 'F') << "  \t" <<
        std::setw(12) << td.m_global_output_index << '\t' <<
        get_transaction_hash(td.m_ptx_wallet_info->m_tx) << "[" << td.m_ptx_wallet_info->m_block_height << "] unlocked: " << (m_wallet->is_transfer_unlocked(td) ? 'T' : 'F');
    }
  }

  if (!transfers_found)
  {
    if (!filter)
    {
      success_msg_writer() << "No incoming transfers";
    }
    else if (available)
    {
      success_msg_writer() << "No incoming available transfers";
    }
    else
    {
      success_msg_writer() << "No incoming unavailable transfers";
    }
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_incoming_transfers_counts(const std::vector<std::string>& args)
{

  tools::transfer_container transfers;
  m_wallet->get_transfers(transfers);

  uint64_t spent_count = 0;
  uint64_t unspent_count = 0;
  for (const auto& td : transfers)
  {
    if (td.m_flags&WALLET_TRANSFER_DETAIL_FLAG_SPENT)
    {
      ++spent_count;
    }
    else
    {
      ++unspent_count;
    }
  }
  success_msg_writer() << "Total outs: Spent: " << spent_count << ", Unspent: " << unspent_count;

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::scan_for_key_image_collisions(const std::vector<std::string> &args)
{
  std::unordered_map<crypto::key_image, std::list<size_t> > key_images;
  if (!m_wallet->scan_for_collisions(key_images))
  {
    message_writer() << "Collisions not found";
    return true;
  }
  else
  {
    size_t total_colisions = 0;
    for (auto& item : key_images)
    {
      if (item.second.size() > 1)
      {
        std::stringstream ss;
        for (auto it = item.second.begin(); it != item.second.end(); it++)
          ss << std::to_string(*it)+"|";

        message_writer() << "Collision: " << item.first << " " << ss.str();
        LOG_PRINT_L0("Collision: " << item.first << " " << ss.str());
        total_colisions++;
      }
    }
    message_writer() << "Total collisions: " << total_colisions;
  }

  return true;
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::fix_collisions(const std::vector<std::string> &args)
{
  size_t collisions_fixed = m_wallet->fix_collisions();
  message_writer() << collisions_fixed << " collisions fixed";
  return true;
}

void print_td_list(const std::list<tools::transfer_details>& td)
{
  message_writer() << "entries found: " << td.size();
  for (auto& e : td)
  {
    std::string json_details = epee::serialization::store_t_to_json(e);
    message_writer() << "------------------------" << ENDL << json_details;
  }
  
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::scan_transfers_for_id(const std::vector<std::string> &args)
{
  crypto::hash id = currency::null_hash;
  if (!epee::string_tools::parse_tpod_from_hex_string(args[0], id))
  {
    fail_msg_writer() << "expected valid tx id";
    return true;
  }
  std::list<tools::transfer_details> td;
  m_wallet->scan_for_transaction_entries(id, currency::null_ki, td);
  print_td_list(td);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::scan_transfers_for_ki(const std::vector<std::string> &args)
{
  crypto::key_image ki = currency::null_ki;
  if (!epee::string_tools::parse_tpod_from_hex_string(args[0], ki))
  {
    fail_msg_writer() << "expected valid key_image";
    return true;
  }
  std::list<tools::transfer_details> td;
  m_wallet->scan_for_transaction_entries(currency::null_hash, ki, td);
  print_td_list(td);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::print_utxo_distribution(const std::vector<std::string> &args)
{
  std::map<uint64_t, uint64_t> distribution;
  m_wallet->get_utxo_distribution(distribution);
  for (auto& e : distribution)
  {    
    message_writer() << std::left << setw(25) << print_money(e.first) << "|" << e.second;
  }
  return true;
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::get_transfer_info(const std::vector<std::string> &args)
{
  if (args.empty())
  {
    fail_msg_writer() << "expected key_image";
    return true;
  }

  size_t i = 0;
  tools::transfer_details td = AUTO_VAL_INIT(td);

  if (epee::string_tools::get_xtype_from_string(i, args[0]))
  {
    if (!m_wallet->get_transfer_info_by_index(i, td))
    {
      fail_msg_writer() << "failed to find transfer info by index";
      return true;
    }
  }
  else
  {
    crypto::key_image ki = currency::null_ki;
    if (!epee::string_tools::parse_tpod_from_hex_string(args[0], ki))
    {
      fail_msg_writer() << "expected valid key_image";
      return true;
    }
    if (!m_wallet->get_transfer_info_by_key_image(ki, td, i))
    {
      fail_msg_writer() << "failed to find transfer info by key_image";
      return true;
    }
  }
  std::string json_details = epee::serialization::store_t_to_json(td);
  message_writer() << "related transfer info:[" << i << "] " << ENDL << json_details;

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_payments(const std::vector<std::string> &args)
{
  if(args.empty())
  {
    fail_msg_writer() << "expected at least one payment_id";
    return true;
  }

  message_writer() << "                            payment                             \t" <<
    "                          transaction                           \t" <<
    "  height\t       amount        \tunlock time";

  bool payments_found = false;
  for(std::string arg : args)
  {
    std::list<tools::payment_details> payments;
    m_wallet->get_payments(arg, payments);
    if (payments.empty())
    {
      success_msg_writer() << "No payments with id " << arg;
      continue;
    }

    for (const tools::payment_details& pd : payments)
    {
      if (!payments_found)
      {
        payments_found = true;
      }
      success_msg_writer(true) <<
        arg << '\t' <<
        pd.m_tx_hash << '\t' <<
        std::setw(8) << pd.m_block_height << '\t' <<
        std::setw(21) << print_money(pd.m_amount) << '\t' <<
        pd.m_unlock_time;
    }
//     else
//     {
//       fail_msg_writer() << "payment id has invalid format: \"" << arg << "\", expected 64-character string";
//     }
  }

  return true;
}
//------------------------------------------------------------------------------------------------------------------
uint64_t simple_wallet::get_daemon_blockchain_height(std::string& err)
{
  COMMAND_RPC_GET_HEIGHT::request req;
  COMMAND_RPC_GET_HEIGHT::response res = boost::value_initialized<COMMAND_RPC_GET_HEIGHT::response>();
  bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/getheight", req, res, m_http_client);
  err = interpret_rpc_response(r, res.status);
  return res.height;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_blockchain_height(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  std::string err;
  uint64_t bc_height = get_daemon_blockchain_height(err);
  if (err.empty())
    success_msg_writer() << bc_height;
  else
    fail_msg_writer() << "failed to get blockchain height: " << err;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_wallet_bcheight(const std::vector<std::string>& args)
{

  uint64_t bc_height = m_wallet->get_blockchain_current_size();
  success_msg_writer() << bc_height;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::validate_wrap_status(uint64_t amount)
{
  //check if amount is fit erc20 fees and amount left in circulation 
  epee::net_utils::http::http_simple_client http_client;

  currency::void_struct req = AUTO_VAL_INIT(req);
  currency::rpc_get_wrap_info_response res = AUTO_VAL_INIT(res);
  bool r = epee::net_utils::invoke_http_json_remote_command2("http://wrapped.zano.org/api2/get_wrap_info", req, res, http_client, 10000);
  if (!r)
  {
    fail_msg_writer() << "Failed to request wrap status from server, check internet connection";
    return false;
  }
  //check if amount is bigger then erc20 fee
  uint64_t zano_needed_for_wrap = std::stoll(res.tx_cost.zano_needed_for_erc20);
  if (amount <= zano_needed_for_wrap)
  {
    fail_msg_writer() << "Too small amount to cover ERC20 fee. ERC20 cost is: " 
      << print_money(zano_needed_for_wrap) << " Zano" <<
      "($" << res.tx_cost.usd_needed_for_erc20 << ")";
    return false;
  }
  uint64_t unwrapped_coins_left = std::stoll(res.unwraped_coins_left);
  if (amount > unwrapped_coins_left)
  {
    fail_msg_writer() << "Amount is bigger than ERC20 tokens left available: "
      << print_money(unwrapped_coins_left) << " wZano";
    return false;
  }
  
  success_msg_writer(false) << "You'll receive estimate " << print_money(amount - zano_needed_for_wrap) << " wZano (" << print_money(zano_needed_for_wrap)<< " Zano will be used to cover ERC20 fee)";
  success_msg_writer(false) << "Proceed? (yes/no)";
  while (true)
  {
    std::string user_response;
    std::getline(std::cin, user_response);
    if (user_response == "yes" || user_response == "y")
      return true;
    else if (user_response == "no" || user_response == "n")
      return false;
    else {
      success_msg_writer(false) << "Wrong response, can be \"yes\" or \"no\"";
    }
  }
}
//----------------------------------------------------------------------------------------------------
bool preprocess_asset_id(std::string& address_arg, crypto::public_key& asset_id)
{
  auto p = address_arg.find(':');
  if (p == std::string::npos)
    return true;
  std::string asset_id_str = address_arg.substr(0, p);
  std::string address_itself = address_arg.substr(p+1, address_arg.size());
  if (!epee::string_tools::parse_tpod_from_hex_string(asset_id_str, asset_id))
    return false;
  address_arg = address_itself;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::transfer(const std::vector<std::string> &args_)
{
  CONFIRM_WITH_PASSWORD();
  SIMPLE_WALLET_BEGIN_TRY_ENTRY();
  if (!try_connect_to_daemon())
    return true;

  std::vector<std::string> local_args = args_;
  if(local_args.size() < 3)
  {
    fail_msg_writer() << "wrong number of arguments, expected at least 3, got " << local_args.size();
    return true;
  }

  size_t fake_outs_count;
  if(!string_tools::get_xtype_from_string(fake_outs_count, local_args[0]))
  {
    fail_msg_writer() << "mixin_count should be non-negative integer, got " << local_args[0];
    return true;
  }
  local_args.erase(local_args.begin());

  
  currency::payment_id_t payment_id;
  if (1 == local_args.size() % 2)
  {
    std::string payment_id_hex = local_args.back();
    local_args.pop_back();
    if (!payment_id_hex.empty() && !currency::parse_payment_id_from_hex_str(payment_id_hex, payment_id))
    {
      fail_msg_writer() << "unable to parse payment id: " << payment_id_hex << ", HEX-encoded string is expected";
      return true;
    }
  }
  
  if (!currency::is_payment_id_size_ok(payment_id))
  {
    fail_msg_writer() << "decoded payment id is too long: " << payment_id.size() << " bytes, max allowed: " << BC_PAYMENT_ID_SERVICE_SIZE_MAX;
    return true;
  }

  std::vector<extra_v> extra;
  vector<currency::tx_destination_entry> dsts;
  bool wrapped_transaction = false;
  for (size_t i = 0; i < local_args.size(); i += 2) 
  {
    std::string integrated_payment_id;
    currency::tx_destination_entry de = AUTO_VAL_INIT(de);
    de.addr.resize(1);    

    bool ok = currency::parse_amount(de.amount, local_args[i + 1]);
    if (!ok || 0 == de.amount)
    {
      fail_msg_writer() << "amount is wrong: " << local_args[i] << ' ' << local_args[i + 1] <<
        ", expected number from 0 to " << print_money(std::numeric_limits<uint64_t>::max());
      return true;
    }

    if (!preprocess_asset_id(local_args[i], de.asset_id))
    {
      fail_msg_writer() << "address is wrong: " << local_args[i];
      return true;
    }
    
    //check if address looks like wrapped address
    if (is_address_like_wrapped(local_args[i]))
    {

      success_msg_writer(false) << "Address " << local_args[i] << " recognized as wrapped address, creating wrapping transaction.";
      success_msg_writer(false) << "This transaction will create wZano (\"Wrapped Zano\") which will be sent to the specified address on the Ethereum network.";

      if (!validate_wrap_status(de.amount))
      {
        return true;
      }
      //put into service attachment specially encrypted entry which will contain wrap address and network
      tx_service_attachment sa = AUTO_VAL_INIT(sa);
      sa.service_id = BC_WRAP_SERVICE_ID;
      sa.instruction = BC_WRAP_SERVICE_INSTRUCTION_ERC20;
      sa.flags = TX_SERVICE_ATTACHMENT_ENCRYPT_BODY | TX_SERVICE_ATTACHMENT_ENCRYPT_BODY_ISOLATE_AUDITABLE;
      sa.body = local_args[i];
      extra.push_back(sa);

      currency::account_public_address acc = AUTO_VAL_INIT(acc);
      currency::get_account_address_from_str(acc, BC_WRAP_SERVICE_CUSTODY_WALLET);
      de.addr.front() = acc;
      wrapped_transaction = true;
      //encrypt body with a special way
    }
    else if(!(de.addr.size() == 1 && m_wallet->get_transfer_address(local_args[i], de.addr.front(), integrated_payment_id)))
    {
      fail_msg_writer() << "wrong address: " << local_args[i];
      return true;
    }

    if (local_args.size() <= i + 1)
    {
      fail_msg_writer() << "amount for the last address " << local_args[i] << " is not specified";
      return true;
    }

    if (integrated_payment_id.size() != 0)
    {
      if (payment_id.size() != 0)
      {
        fail_msg_writer() << "address " + local_args[i] + " has integrated payment id " + epee::string_tools::buff_to_hex_nodelimer(integrated_payment_id) + " which conflicts with previously set payment id: " << epee::string_tools::buff_to_hex_nodelimer(payment_id) << "";
        return true;
      }
      payment_id = integrated_payment_id;
    }

    dsts.push_back(de);
  }


  std::vector<currency::attachment_v> attachments;
  if (!payment_id.empty() && !set_payment_id_to_tx(attachments, payment_id, m_wallet->is_in_hardfork_zone(ZANO_HARDFORK_04_ZARCANUM)))
  {
    fail_msg_writer() << "provided (or embedded) payment id can't be set: \"" << payment_id << "\"";
    return true;
  }

  currency::transaction tx;
  m_wallet->transfer(dsts, fake_outs_count, 0, m_wallet->get_core_runtime_config().tx_default_fee, extra, attachments, tx);

  if (!m_wallet->is_watch_only())
  {
    if(wrapped_transaction)
      success_msg_writer(true) << "Transaction successfully sent to wZano custody wallet, id: " << get_transaction_hash(tx) << ", " << get_object_blobsize(tx) << " bytes";
    else
      success_msg_writer(true) << "Transaction successfully sent, id: " << get_transaction_hash(tx) << ", " << get_object_blobsize(tx) << " bytes";
  }
  else
  {
    success_msg_writer(true) << "Transaction prepared for signing and saved into \"zano_tx_unsigned\" file, use full wallet to sign transfer and then use \"submit_transfer\" on this wallet to broadcast the transaction to the network";
  }  
  SIMPLE_WALLET_CATCH_TRY_ENTRY()

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::run()
{
  std::string addr_start = m_wallet->get_account().get_public_address_str().substr(0, 6);
  std::string prompt;
  if (m_wallet->is_watch_only())
    prompt = "[" CURRENCY_NAME_BASE " WO wallet " + addr_start + "]: ";
  else
    prompt = "[" CURRENCY_NAME_BASE " wallet " + addr_start + "]: ";
  return m_cmd_binder.run_handling(prompt, "");
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::stop()
{
  m_cmd_binder.stop_handling();
  m_wallet->stop();
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::print_address(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  success_msg_writer() << m_wallet->get_account().get_public_address_str();
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_seed(const std::vector<std::string> &args)
{
  CONFIRM_WITH_PASSWORD();
  success_msg_writer() << "Please enter a password to secure this seed. Securing your seed is HIGHLY recommended. Leave password blank to stay unsecured.";
  success_msg_writer(true) << "Remember, restoring a wallet from Secured Seed can only be done if you know its password.";

  tools::password_container seed_password_container1;
  if (!seed_password_container1.read_password("Enter seed password: "))
  {
    return false;
  }
  tools::password_container seed_password_container2;
  if (!seed_password_container2.read_password("Confirm seed password: "))
  {
    return false;
  }
  if(seed_password_container1.password() != seed_password_container2.password())
  {
    std::cout << "Error: password mismatch. Please make sure you entered the correct password and confirmed it" << std::endl << std::flush;
  }
  std::cout << m_wallet->get_account().get_seed_phrase(seed_password_container2.password()) << std::endl << std::flush;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::spendkey(const std::vector<std::string> &args)
{
  CONFIRM_WITH_PASSWORD();
  message_writer(epee::log_space::console_color_red, true, std::string())
   << "WARNING! Anyone who knows the following secret key can access your wallet and spend your coins.";

  const account_keys& keys = m_wallet->get_account().get_keys();
  std::cout << "secret: " << epee::string_tools::pod_to_hex(keys.spend_secret_key) << std::endl;
  std::cout << "public: " << epee::string_tools::pod_to_hex(keys.account_address.spend_public_key) << std::endl << std::flush;

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::viewkey(const std::vector<std::string> &args)
{
  CONFIRM_WITH_PASSWORD();
  message_writer(epee::log_space::console_color_yellow, false, std::string())
    << "WARNING! Anyone who knows the following secret key can view your wallet (but can not spend your coins).";

  const account_keys& keys = m_wallet->get_account().get_keys();
  std::cout << "secret: " << epee::string_tools::pod_to_hex(keys.view_secret_key) << std::endl;
  std::cout << "public: " << epee::string_tools::pod_to_hex(keys.account_address.view_public_key) << std::endl << std::flush;

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::resync_wallet(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  m_wallet->reset_history();
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::process_command(const std::vector<std::string> &args)
{
  return m_cmd_binder.process_command_vec(args);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::integrated_address(const std::vector<std::string> &args)
{
  if (args.size() > 1)
  {
    fail_msg_writer() << "none or one argument expected, " << args.size() << " provided";
    return true;
  }

  if (args.size() == 1)
  {
    std::string addr_or_payment_id_hex = args[0];
    currency::account_public_address addr = AUTO_VAL_INIT(addr);
    std::string embedded_payment_id;
    if (currency::get_account_address_and_payment_id_from_str(addr, embedded_payment_id, addr_or_payment_id_hex))
    {
      // address provided
      if (embedded_payment_id.empty())
      {
        success_msg_writer(false) << addr_or_payment_id_hex << " is a standard address";
        return true;
      }
      success_msg_writer(true ) << "integrated address:       " << addr_or_payment_id_hex;
      success_msg_writer(false) << "standard address:         " << get_account_address_as_str(addr);
      success_msg_writer(false) << "payment id (" << std::setw(3) << embedded_payment_id.size() << " bytes) :  " << epee::string_tools::mask_non_ascii_chars(embedded_payment_id);
      success_msg_writer(false) << "payment id (hex-encoded): " << epee::string_tools::buff_to_hex_nodelimer(embedded_payment_id);
      return true;
    }

    // seems like not a valid address, treat as payment id
    std::string payment_id;
    if (!epee::string_tools::parse_hexstr_to_binbuff(addr_or_payment_id_hex, payment_id))
    {
      fail_msg_writer() << "invalid payment id given: \'" << addr_or_payment_id_hex << "\', hex-encoded string was expected";
      return true;
    }

    if (!currency::is_payment_id_size_ok(payment_id))
    {
      fail_msg_writer() << "payment id is too long: " << payment_id.size() << " bytes, max allowed: " << BC_PAYMENT_ID_SERVICE_SIZE_MAX << " bytes";
      return true;
    }
    success_msg_writer(false) << "this wallet standard address: " << m_wallet->get_account().get_public_address_str();
    success_msg_writer(false) << "payment id (" << std::setw(3) << payment_id.size() << " bytes) :      " << epee::string_tools::mask_non_ascii_chars(payment_id);
    success_msg_writer(false) << "payment id (hex-encoded) :    " << epee::string_tools::buff_to_hex_nodelimer(payment_id);
    success_msg_writer(true ) << "integrated address:           " << get_account_address_and_payment_id_as_str(m_wallet->get_account().get_public_address(), payment_id);
    return true;
  }

  // args.size() == 0
  std::string payment_id(8, ' ');
  crypto::generate_random_bytes(payment_id.size(), &payment_id.front());
  std::string payment_id_hex = epee::string_tools::buff_to_hex_nodelimer(payment_id);
  success_msg_writer(false) << "this wallet standard address:       " << m_wallet->get_account().get_public_address_str();
  success_msg_writer(false) << "generated payment id (hex-encoded): " << payment_id_hex;
  success_msg_writer(true ) << "integrated address:                 " << get_account_address_and_payment_id_as_str(m_wallet->get_account().get_public_address(), payment_id);

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::get_tx_key(const std::vector<std::string> &args_)
{
  std::vector<std::string> local_args = args_;

  if (local_args.size() != 1) {
    fail_msg_writer() << "usage: get_tx_key <txid>";
    return true;
  }

  currency::blobdata txid_data;
  if (!epee::string_tools::parse_hexstr_to_binbuff(local_args.front(), txid_data) || txid_data.size() != sizeof(crypto::hash))
  {
    fail_msg_writer() << "failed to parse txid";
    return true;
  }
  crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

  crypto::secret_key tx_key;
  std::vector<crypto::secret_key> amount_keys;
  if (m_wallet->get_tx_key(txid, tx_key))
  {
    success_msg_writer() << "tx one-time secret key: " << epee::string_tools::pod_to_hex(tx_key);
    return true;
  }
  else
  {
    fail_msg_writer() << "no tx keys found for this txid";
    return true;
  }
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::tracking_seed(const std::vector<std::string> &args_)
{
  if (!m_wallet->is_auditable())
  {
    fail_msg_writer() << "this command is allowed for auditable wallets only";
    return true;
  }

  success_msg_writer() << "Auditable watch-only tracking seed for this wallet is:";
  std::cout << m_wallet->get_account().get_tracking_seed() << ENDL;
  success_msg_writer() << "Anyone having this tracking seed is able to watch your balance and transaction history, but unable to spend coins.";
  return true;
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::set_offline_mode(bool offline_mode)
{
  if (offline_mode && !m_offline_mode)
  {
    message_writer(epee::log_space::console_color_yellow, true, std::string(), LOG_LEVEL_0)
      << "WARNING: the wallet is running in OFFLINE MODE!";
  }
  m_offline_mode = offline_mode;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::save_watch_only(const std::vector<std::string> &args)
{
  if (args.size() < 2)
  {
    fail_msg_writer() << "wrong parameters, expected filename and password";
    return true;
  }
  try
  {
    m_wallet->store_watch_only(epee::string_encoding::convert_to_unicode(args[0]), args[1]);
    success_msg_writer() << "Watch-only wallet has been stored to " << args[0];
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    fail_msg_writer() << "unexpected error: " << e.what();
  }
  catch (...)
  {
    LOG_ERROR("Unknown error");
    fail_msg_writer() << "unknown error";
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::list_outputs(const std::vector<std::string> &args)
{
  bool include_spent = true, include_unspent = true, show_only_unknown = false;
  std::string filter_asset_ticker{};

  bool arg_spent_flags = false, arg_unknown_assets = false, arg_ticker_filer = false;

  auto process_arg = [&](const std::string& arg) -> bool {
    if (!arg_spent_flags && (arg == "u" || arg == "unspent" || arg == "available"))
      arg_spent_flags = true, include_spent = false;
    else if (!arg_spent_flags && (arg == "s" || arg == "spent" || arg == "unavailable"))
      arg_spent_flags = true, include_unspent = false;
    else if (!arg_unknown_assets && (arg == "unknown"))
      arg_unknown_assets = true, show_only_unknown = true;
    else if (!arg_ticker_filer && (arg.find("ticker=") == 0 || arg.find("t=") == 0))
      arg_ticker_filer = true, filter_asset_ticker = boost::erase_all_copy(boost::erase_all_copy(arg, "ticker="), "t=");
    else
      return false;
    return true;
  };

  for(auto& arg : args)
  {
    if (!process_arg(arg))
    {
      fail_msg_writer() << "invalid parameter: " << arg;
      return true;
    }
  }

  success_msg_writer() << m_wallet->get_transfers_str(include_spent, include_unspent, show_only_unknown, filter_asset_ticker);

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sign_transfer(const std::vector<std::string> &args)
{
  CONFIRM_WITH_PASSWORD();
  if (m_wallet->is_watch_only())
  {
    fail_msg_writer() << "You can't sign transaction in watch-only wallet";
    return true;

  }

  if (args.size() < 2)
  {
    fail_msg_writer() << "wrong parameters, expected: <unsigned_tx_file> <signed_tx_file>";
    return true;
  }
  try
  {
    currency::transaction res_tx;
    m_wallet->sign_transfer_files(args[0], args[1], res_tx);
    success_msg_writer(true) << "transaction signed and stored to file: " << args[1] << ", transaction " << get_transaction_hash(res_tx) << ", " << get_object_blobsize(res_tx) << " bytes";
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    fail_msg_writer() << "unexpected error: " << e.what();
  }
  catch (...)
  {
    LOG_ERROR("Unknown error");
    fail_msg_writer() << "unknown error";
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::submit_transfer(const std::vector<std::string> &args)
{
  if (args.size() < 1)
  {
    fail_msg_writer() << "wrong parameters, expected filename";
    return true;
  }
  try
  {
    currency::transaction res_tx;
    m_wallet->submit_transfer_files(args[0], res_tx);
    success_msg_writer(true) << "transaction " << get_transaction_hash(res_tx) << " was successfully sent, size: " << get_object_blobsize(res_tx) << " bytes";
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    fail_msg_writer() << "unexpected error: " << e.what();
  }
  catch (...)
  {
    LOG_ERROR("Unknown error");
    fail_msg_writer() << "unknown error";
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::tor_enable(const std::vector<std::string> &args)
{
  success_msg_writer(true) << "TOR relaying enabled";
  m_wallet->set_disable_tor_relay(false);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::tor_disable(const std::vector<std::string> &args)
{
  m_wallet->set_disable_tor_relay(true);
  success_msg_writer(true) << "TOR relaying disabled";
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::deploy_new_asset(const std::vector<std::string> &args)
{
  CONFIRM_WITH_PASSWORD();
  SIMPLE_WALLET_BEGIN_TRY_ENTRY();
  asset_descriptor_base adb = AUTO_VAL_INIT(adb);
  if (!args.size() || args.size() > 1)
  {
    fail_msg_writer() << "invalid arguments count: " << args.size() << ", expected 1";
    return true;
  }
  bool r = epee::serialization::load_t_from_json_file(adb, args[0]);
  if (!r)
  {
    fail_msg_writer() << "Failed to load json file with asset specification: " << args[0];
    return true;
  }
  tx_destination_entry td = AUTO_VAL_INIT(td);
  td.addr.push_back(m_wallet->get_account().get_public_address());
  td.amount = adb.current_supply;
  td.asset_id = currency::null_pkey;
  std::vector<currency::tx_destination_entry> destinations;
  destinations.push_back(td);
  currency::transaction result_tx = AUTO_VAL_INIT(result_tx);
  crypto::public_key result_asset_id = currency::null_pkey;
  m_wallet->deploy_new_asset(adb, destinations, result_tx, result_asset_id);

  success_msg_writer(true) << "New asset successfully deployed with tx " << get_transaction_hash(result_tx) << " (unconfirmed) : " << ENDL
    << "Asset ID:     " << result_asset_id << ENDL
    << "Title:        " << adb.full_name << ENDL
    << "Ticker:       " << adb.ticker << ENDL
    << "Emitted:      " << print_fixed_decimal_point(adb.current_supply, adb.decimal_point) << ENDL
    << "Max emission: " << print_fixed_decimal_point(adb.total_max_supply, adb.decimal_point) << ENDL
    ;
  
  SIMPLE_WALLET_CATCH_TRY_ENTRY();
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::emit_asset(const std::vector<std::string> &args)
{
  CONFIRM_WITH_PASSWORD();
  SIMPLE_WALLET_BEGIN_TRY_ENTRY();
  if (args.size() != 2)
  {
    fail_msg_writer() << "invalid arguments count: " << args.size() << ", expected 2";
    return true;
  }
  crypto::public_key asset_id = currency::null_pkey;
  bool r = epee::string_tools::parse_tpod_from_hex_string(args[0], asset_id);
  if (!r)
  {
    fail_msg_writer() << "Failed to load asset_id from: " << args[0];
    return true;
  }

  uint64_t amount = 0;
  r = epee::string_tools::get_xtype_from_string(amount, args[1]);
  if (!r)
  {
    fail_msg_writer() << "Failed to load amount from: " << args[1];
    return true;
  }


  currency::asset_descriptor_base adb = AUTO_VAL_INIT(adb);
  r = m_wallet->daemon_get_asset_info(asset_id, adb);
  if (!r)
  {
    fail_msg_writer() << "Wallet seems to don't have control over asset: " << args[0];
    return true;
  }

  tx_destination_entry td = AUTO_VAL_INIT(td);
  td.addr.push_back(m_wallet->get_account().get_public_address());
  td.amount = amount;
  td.asset_id = currency::null_pkey;
  std::vector<currency::tx_destination_entry> destinations;
  destinations.push_back(td);
  currency::transaction result_tx = AUTO_VAL_INIT(result_tx);
  m_wallet->emit_asset(asset_id, destinations, result_tx);

  success_msg_writer(true) << "Emitted " << get_transaction_hash(result_tx) << " (unconfirmed) : " << ENDL
    << "Asset ID:     " << asset_id << ENDL
    << "Title:        " << adb.full_name << ENDL
    << "Ticker:       " << adb.ticker << ENDL
    << "Emitted:      " << print_fixed_decimal_point(amount, adb.decimal_point) << ENDL
    << "Max emission: " << print_fixed_decimal_point(adb.total_max_supply, adb.decimal_point) << ENDL
    ;

  SIMPLE_WALLET_CATCH_TRY_ENTRY();
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::burn_asset(const std::vector<std::string> &args)
{
  CONFIRM_WITH_PASSWORD();
  SIMPLE_WALLET_BEGIN_TRY_ENTRY();
  if (args.size() != 2)
  {
    fail_msg_writer() << "invalid arguments count: " << args.size() << ", expected 2";
    return true;
  }
  crypto::public_key asset_id = currency::null_pkey;
  bool r = epee::string_tools::parse_tpod_from_hex_string(args[0], asset_id);
  if (!r)
  {
    fail_msg_writer() << "Failed to load asset_id from: " << args[0];
    return true;
  }

  uint64_t amount = 0;
  r = epee::string_tools::get_xtype_from_string(amount, args[1]);
  if (!r)
  {
    fail_msg_writer() << "Failed to load amount from: " << args[1];
    return true;
  }


  currency::asset_descriptor_base adb = AUTO_VAL_INIT(adb);
  r = m_wallet->daemon_get_asset_info(asset_id, adb);
  if (!r)
  {
    fail_msg_writer() << "Wallet seems to don't have control over asset: " << args[0];
    return true;
  }

  currency::transaction result_tx = AUTO_VAL_INIT(result_tx);
  m_wallet->burn_asset(asset_id, amount, result_tx);

  success_msg_writer(true) << "Burned " << get_transaction_hash(result_tx) << " (unconfirmed) : " << ENDL
    << "Asset ID:     " << asset_id << ENDL
    << "Title:        " << adb.full_name << ENDL
    << "Ticker:       " << adb.ticker << ENDL
    << "Burned:      " << print_fixed_decimal_point(amount, adb.decimal_point) << ENDL
    << "Max emission: " << print_fixed_decimal_point(adb.total_max_supply, adb.decimal_point) << ENDL
    ;

  SIMPLE_WALLET_CATCH_TRY_ENTRY();
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::update_asset(const std::vector<std::string> &args)
{
  CONFIRM_WITH_PASSWORD();
  SIMPLE_WALLET_BEGIN_TRY_ENTRY();
  if (args.size() != 2)
  {
    fail_msg_writer() << "invalid arguments count: " << args.size() << ", expected 2";
    return true;
  }

  crypto::public_key asset_id = currency::null_pkey;
  bool r = epee::string_tools::parse_tpod_from_hex_string(args[0], asset_id);
  if (!r)
  {
    fail_msg_writer() << "Failed to load asset_id from: " << args[0];
    return true;
  }

  std::string buff_metainfo = "";
  r = epee::file_io_utils::load_file_to_string(args[1], buff_metainfo);
  if (!r)
  {
    fail_msg_writer() << "Failed to load metainfo data file: " << args[1];
    return true;
  }

  currency::asset_descriptor_base adb = AUTO_VAL_INIT(adb);
  r = m_wallet->daemon_get_asset_info(asset_id, adb);
  if (!r)
  {
    fail_msg_writer() << "Wallet seems to don't have control over asset: " << args[0];
    return true;
  }

  adb.meta_info = buff_metainfo;
  currency::transaction result_tx = AUTO_VAL_INIT(result_tx);
  m_wallet->update_asset(asset_id, adb, result_tx);

  success_msg_writer(true) << "Asset metainfo update tx sent: " << get_transaction_hash(result_tx) << " (unconfirmed) : " << ENDL
    << "Asset ID:     " << asset_id << ENDL
    << "Title:        " << adb.full_name << ENDL
    << "Ticker:       " << adb.ticker << ENDL
    << "Emitted:      " << print_fixed_decimal_point(adb.current_supply, adb.decimal_point) << ENDL
    << "Max emission: " << print_fixed_decimal_point(adb.total_max_supply, adb.decimal_point) << ENDL
    ;

  SIMPLE_WALLET_CATCH_TRY_ENTRY();
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::add_custom_asset_id(const std::vector<std::string> &args)
{
  CONFIRM_WITH_PASSWORD();
  SIMPLE_WALLET_BEGIN_TRY_ENTRY();
  if (!args.size() || args.size() > 1)
  {
    fail_msg_writer() << "invalid arguments count: " << args.size() << ", expected 1";
    return true;
  }
  crypto::public_key asset_id = currency::null_pkey;
  if (!epee::string_tools::parse_tpod_from_hex_string(args[0], asset_id))
  {
    fail_msg_writer() << "expected valid asset_id";
    return true;
  }
  asset_descriptor_base asset_descriptor = AUTO_VAL_INIT(asset_descriptor);
  bool r = m_wallet->add_custom_asset_id(asset_id, asset_descriptor);
  if(!r)
  {
    fail_msg_writer() << "Asset id " << asset_id  << " not found as registered asset";
    return true;
  }
  else
  {
    success_msg_writer() << "The following custom asset was successfully added to the wallet:" << ENDL 
      << " id:     " << asset_id << ENDL
      << " title:  " << asset_descriptor.full_name << ENDL
      << " ticker: " << asset_descriptor.ticker << ENDL
      << " supply: " << print_fixed_decimal_point(asset_descriptor.current_supply, asset_descriptor.decimal_point) << ENDL
      ;
  }
  SIMPLE_WALLET_CATCH_TRY_ENTRY();
  return true;

}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::generate_ionic_swap_proposal(const std::vector<std::string> &args)
{
  CONFIRM_WITH_PASSWORD();
  SIMPLE_WALLET_BEGIN_TRY_ENTRY();

  if (args.size() != 2)
  {
    fail_msg_writer() << "invalid arguments count: " << args.size() << ", expected 1";
    return true;
  }

  view::ionic_swap_proposal_info proposal_info = AUTO_VAL_INIT(proposal_info);
  bool r = epee::serialization::load_t_from_json_file(proposal_info, args[0]);
  if (!r)
  {
    fail_msg_writer() << "Failed to load json file with asset specification: " << args[0];
    return true;
  }

  currency::account_public_address destination_addr = AUTO_VAL_INIT(destination_addr);
  currency::payment_id_t integrated_payment_id;
  if (!m_wallet->get_transfer_address(args[1], destination_addr, integrated_payment_id))
  {
    fail_msg_writer() << "wrong address: " << args[1];
    return true;
  }
  if (integrated_payment_id.size())
  {
    fail_msg_writer() << "Integrated addresses not supported yet";
    return true;
  }

  tools::wallet_public::ionic_swap_proposal proposal = AUTO_VAL_INIT(proposal);
  r = m_wallet->create_ionic_swap_proposal(proposal_info, destination_addr, proposal);
  if (!r)
  {
    fail_msg_writer() << "Failed to create ionic_swap proposal";
    return true;
  }
  else
  {    
    success_msg_writer() << "Generated proposal: " << ENDL << epee::string_tools::buff_to_hex_nodelimer(t_serializable_object_to_blob(proposal));
  }
  SIMPLE_WALLET_CATCH_TRY_ENTRY();
  return true;

}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::get_ionic_swap_proposal_info(const std::vector<std::string> &args)
{
  SIMPLE_WALLET_BEGIN_TRY_ENTRY();

  if (args.size() != 1)
  {
    fail_msg_writer() << "invalid arguments count: " << args.size() << ", expected 1";
    return true;
  }

  std::string raw_hex_proposal;
  bool r = epee::file_io_utils::load_file_to_string(args[0], raw_hex_proposal);
  if (!r)
  {
    fail_msg_writer() << "Failed to load proposal hex from file";
    return true;
  }

  std::string raw_proposal;
  r = epee::string_tools::parse_hexstr_to_binbuff(raw_hex_proposal, raw_proposal);
  if (!r)
  {
    fail_msg_writer() << "Failed to parse proposal hex to raw data";
    return true;
  }

  view::ionic_swap_proposal_info proposal_info = AUTO_VAL_INIT(proposal_info);
  if (!m_wallet->get_ionic_swap_proposal_info(raw_proposal, proposal_info))
  {
    fail_msg_writer() << "Failed to decode proposal info";
    return true;
  }
  std::string json_proposal_info = epee::serialization::store_t_to_json(proposal_info);


  success_msg_writer() << "Proposal details: " << ENDL << json_proposal_info;

  SIMPLE_WALLET_CATCH_TRY_ENTRY();
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::accept_ionic_swap_proposal(const std::vector<std::string> &args)
{
  CONFIRM_WITH_PASSWORD();
  SIMPLE_WALLET_BEGIN_TRY_ENTRY();

  if (args.size() != 1)
  {
    fail_msg_writer() << "invalid arguments count: " << args.size() << ", expected 1";
    return true;
  }

  std::string raw_hex_proposal;
  bool r = epee::file_io_utils::load_file_to_string(args[0], raw_hex_proposal);
  if (!r)
  {
    fail_msg_writer() << "Failed to load proposal hex from file";
    return true;
  }


  std::string raw_proposal;
  r = epee::string_tools::parse_hexstr_to_binbuff(raw_hex_proposal, raw_proposal);
  if (!r)
  {
    fail_msg_writer() << "Failed to parse proposal hex to raw data";
    return true;
  }

  currency::transaction result_tx = AUTO_VAL_INIT(result_tx);
  if (!m_wallet->accept_ionic_swap_proposal(raw_proposal, result_tx))
  {
    fail_msg_writer() << "Failed accept ionic_swap proposal";
    return true;
  }

  success_msg_writer() << "Proposal accepted and executed, tx_id : " << currency::get_transaction_hash(result_tx);

  SIMPLE_WALLET_CATCH_TRY_ENTRY();
  return true;
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::remove_custom_asset_id(const std::vector<std::string> &args)
{
  CONFIRM_WITH_PASSWORD();
  SIMPLE_WALLET_BEGIN_TRY_ENTRY();
  if (!args.size() || args.size() > 1)
  {
    fail_msg_writer() << "invalid arguments count: " << args.size() << ", expected 1";
    return true;
  }
  crypto::public_key asset_id = currency::null_pkey;
  if (!epee::string_tools::parse_tpod_from_hex_string(args[0], asset_id))
  {
    fail_msg_writer() << "expected valid asset_id";
    return true;
  }

  bool r = m_wallet->delete_custom_asset_id(asset_id);
  if (!r)
  {
    fail_msg_writer() << "Asset id " << asset_id << " not present in this wallet";
    return true;
  }
  else
  {
    success_msg_writer() << "Asset id " << asset_id << " removed from wallet custom list" << ENDL;
  }

  SIMPLE_WALLET_CATCH_TRY_ENTRY();
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sweep_below(const std::vector<std::string> &args)
{
  CONFIRM_WITH_PASSWORD();
  SIMPLE_WALLET_BEGIN_TRY_ENTRY();
  bool r = false;
  if (args.size() < 3 || args.size() > 4)
  {
    fail_msg_writer() << "invalid agruments count: " << args.size() << ", expected 3 or 4";
    return true;
  }

  size_t fake_outs_count = 0;
  if (!string_tools::get_xtype_from_string(fake_outs_count, args[0]))
  {
    fail_msg_writer() << "mixin_count should be non-negative integer, got " << args[0];
    return true;
  }

  // parse payment_id
  currency::payment_id_t payment_id;
  if (args.size() == 4)
  {
    const std::string &payment_id_str = args.back();
    r = parse_payment_id_from_hex_str(payment_id_str, payment_id);
    if (!r)
    {
      fail_msg_writer() << "payment id has invalid format: \"" << payment_id_str << "\", expected hex string";
      return true;
    }
  }

  currency::account_public_address addr;
  currency::payment_id_t integrated_payment_id;
  if (!m_wallet->get_transfer_address(args[1], addr, integrated_payment_id))
  {
    fail_msg_writer() << "wrong address: " << args[1];
    return true;
  }

  // handle integrated payment id
  if (!integrated_payment_id.empty())
  {
    if (!payment_id.empty())
    {
      fail_msg_writer() << "address " << args[1] << " has integrated payment id " << epee::string_tools::buff_to_hex_nodelimer(integrated_payment_id) <<
        " which is incompatible with payment id " << epee::string_tools::buff_to_hex_nodelimer(payment_id) << " that was already assigned to this transfer";
      return true;
    }

    payment_id = integrated_payment_id; // remember integrated payment id as the main payment id
    success_msg_writer() << "NOTE: using payment id " << epee::string_tools::buff_to_hex_nodelimer(payment_id) << " from integrated address " << args[1];
  }

  uint64_t amount = 0;
  r = currency::parse_amount(amount, args[2]);
  if (!r || amount == 0)
  {
    fail_msg_writer() << "incorrect amount: " << args[2];
    return true;
  }


  uint64_t fee = m_wallet->get_core_runtime_config().tx_default_fee;
  size_t outs_total = 0, outs_swept = 0;
  uint64_t amount_total = 0, amount_swept = 0;
  currency::transaction result_tx = AUTO_VAL_INIT(result_tx);
  std::string filename = "zano_tx_unsigned";
  m_wallet->sweep_below(fake_outs_count, addr, amount, payment_id, fee, outs_total, amount_total, outs_swept, amount_swept, &result_tx, &filename);

  success_msg_writer(false) << outs_swept << " outputs (" << print_money_brief(amount_swept) << " coins) of " << outs_total << " total (" << print_money_brief(amount_total)
    << ") below the specified limit of " << print_money_brief(amount) << " were successfully swept";

  if (m_wallet->is_watch_only())
    success_msg_writer(true) << "Transaction prepared for signing and saved into \"" << filename << "\" file, use full wallet to sign transfer and then use \"submit_transfer\" on this wallet to broadcast the transaction to the network";
  else
    success_msg_writer(true) << "tx: " << get_transaction_hash(result_tx) << " size: " << get_object_blobsize(result_tx) << " bytes";

  SIMPLE_WALLET_CATCH_TRY_ENTRY();
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sweep_bare_outs(const std::vector<std::string> &args)
{
  CONFIRM_WITH_PASSWORD();
  SIMPLE_WALLET_BEGIN_TRY_ENTRY();
  bool r = false;

  if (args.size() > 1)
  {
    fail_msg_writer() << "Invalid number of agruments given";
    return true;
  }

  currency::account_public_address target_address = m_wallet->get_account().get_public_address();
  currency::payment_id_t integrated_payment_id{};
  if (args.size() == 1)
  {
    if (!m_wallet->get_transfer_address(args[0], target_address, integrated_payment_id))
    {
      fail_msg_writer() << "Unable to parse address from " << args[1];
      return true;
    }
    if (!integrated_payment_id.empty())
    {
      fail_msg_writer() << "Payment id is not supported. Please, don't use integrated address with this command.";
      return true;
    }
  }
  
  if (!m_wallet->has_bare_unspent_outputs())
  {
    success_msg_writer(true) << "This wallet doesn't have bare unspent outputs.\nNothing to do. Everything looks good.";
    return true;
  }

  std::vector<tools::wallet2::batch_of_bare_unspent_outs> groups;
  m_wallet->get_bare_unspent_outputs_stats(groups);
  if (groups.empty())
  {
    uint64_t unlocked_balance = 0;
    uint64_t balance = m_wallet->balance(unlocked_balance);
    if (balance < COIN)
      success_msg_writer(false) << "Looks like it's not enough coins to perform this operation. Transferring " << print_money_brief(TX_MINIMUM_FEE) << " ZANO or more to this wallet may help.";
    else if (unlocked_balance < COIN)
      success_msg_writer(false) << "Not enough spendable outputs to perform this operation. Please, try again later.";
    else
    {
      success_msg_writer(false) << "This operation couldn't be performed for some reason. Please, copy simplewallet's log file and ask for support. Nothing was done.";
      LOG_PRINT_L0("strange situation: balance: " << print_money_brief(balance) << ", unlocked_balance: " << print_money_brief(unlocked_balance) << " but get_bare_unspent_outputs_stats returned empty result");
    }
    return true;
  }

  size_t i = 0, total_bare_outs = 0;
  uint64_t total_amount = 0;
  std::stringstream details_ss;
  for(auto &g : groups)
  {
    details_ss << std::setw(2) << i << ": ";
    for (auto& tid: g.tids)
    {
      tools::transfer_details td{};
      CHECK_AND_ASSERT_THROW_MES(m_wallet->get_transfer_info_by_index(tid, td), "get_transfer_info_by_index failed with index " << tid);
      details_ss << tid << " (" << print_money_brief(td.m_amount) << "), ";
      total_amount += td.m_amount;
    }

    if (g.additional_tid)
      details_ss << "additional tid: " << g.tids.back() << "( " << print_money_brief(g.additional_tid_amount) << ")";
    else
      details_ss.seekp(-2, std::ios_base::end);

    details_ss << ENDL;
    ++i;
    total_bare_outs += g.tids.size();
  }

  LOG_PRINT_L1("bare UTXO:" << ENDL << details_ss.str());
  
  success_msg_writer(true) << "This wallet contains " << total_bare_outs << " bare outputs with total amount of " << print_money_brief(total_amount) <<
    ". They can be converted in " << groups.size() << " transaction" << (groups.size() > 1 ? "s" : "") << ", with total fee = " << print_money_brief(TX_DEFAULT_FEE * i) << ".";
  if (target_address != m_wallet->get_account().get_public_address())
    message_writer(epee::log_space::console_color_yellow, false) << print_money_brief(total_amount) << " coins will be sent to address " << get_account_address_as_str(target_address);

  tools::password_container reader;
  if (!reader.read_input("Would you like to continue? (y/yes/n/no):\n") || (reader.get_input() != "y" && reader.get_input() != "yes"))
  {
    success_msg_writer(false) << "Operatation terminated as requested by user.";
    return true;
  }

  size_t total_tx_sent = 0;
  uint64_t total_fee_spent = 0;
  uint64_t total_amount_sent = 0;
  auto on_tx_sent_callback = [&](size_t batch_index, const currency::transaction& tx, uint64_t amount, uint64_t fee, bool sent_ok, const std::string& err) {
      auto mw = success_msg_writer(false);
      mw << std::setw(2) << batch_index << ": transaction ";
      if (!sent_ok)
      {
        mw << "failed  (" << err << ")";
        return;
      }
      mw << get_transaction_hash(tx) << ", fee: " << print_money_brief(fee) << ", amount: " << print_money_brief(amount);
      ++total_tx_sent;
      total_fee_spent += fee;
      total_amount_sent += amount;
    };

  if (!m_wallet->sweep_bare_unspent_outputs(target_address, groups, on_tx_sent_callback))
  {
    auto mw = fail_msg_writer();
    mw << "Operatation failed.";
    if (total_tx_sent > 0)
      mw << " However, " << total_tx_sent << " transaction" << (total_tx_sent == 1 ? " was" : "s were") << " successfully sent, " << print_money_brief(total_amount_sent) << " coins transferred, and " << print_money_brief(total_fee_spent) << " was spent for fees.";
  }
  else
  {
    success_msg_writer(true) << "Operatation succeeded. " << ENDL << total_tx_sent << " transaction" << (total_tx_sent == 1 ? " was" : "s were") << " successfully sent, " << print_money_brief(total_amount_sent) << " coins transferred, and " << print_money_brief(total_fee_spent) << " was spent for fees.";
  }

  SIMPLE_WALLET_CATCH_TRY_ENTRY();
  return true;
}
//----------------------------------------------------------------------------------------------------
uint64_t
get_tick_count__()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

//----------------------------------------------------------------------------------------------------
bool check_if_file_looks_like_a_wallet(const std::wstring& wallet_)
{
  std::string keys_buff;


  boost::system::error_code e;
  bool exists = boost::filesystem::exists(wallet_, e);
  if (e || !exists)
    return false;

  boost::filesystem::ifstream data_file;
  data_file.open(wallet_, std::ios_base::binary | std::ios_base::in);
  if (data_file.fail())
    return false;

  tools::wallet_file_binary_header wbh = AUTO_VAL_INIT(wbh);

  data_file.read((char*)&wbh, sizeof(wbh));
  if (data_file.fail())
  {
    return false;
  }

  if (wbh.m_signature != WALLET_FILE_SIGNATURE_OLD && wbh.m_signature != WALLET_FILE_SIGNATURE_V2)
  {
    return false;
  }

  //std::cout << "\r                                                                           \r";
  LOG_PRINT_L0("Found wallet file: " << epee::string_encoding::convert_to_ansii(wallet_));
  return false;
}

bool search_for_wallet_file(const std::wstring &search_here/*, const std::string &addr_to_compare*/)
{
  if (search_here == L"/proc" || search_here == L"/bin" || search_here == L"/dev" || search_here == L"/etc"
    || search_here == L"/lib" || search_here == L"/lib64" || search_here == L"/proc" || search_here == L"/run"
    || search_here == L"/sbin" || search_here == L"/srv" || search_here == L"/sys" || search_here == L"/usr"
    || search_here == L"/var")
  {
    LOG_PRINT_L0("Skiping    " << epee::string_encoding::convert_to_ansii(search_here));
    return false;
  }

  //LOG_PRINT_L0("FOLDER: " << epee::string_encoding::convert_to_ansii(search_here));
  static uint64_t last_tick = 0;
  using namespace boost::filesystem;
  //recursive_directory_iterator dir(search_here), end;
  try
  {
    for (auto& dir : boost::make_iterator_range(directory_iterator(search_here), {}))
    {
      boost::system::error_code ec = AUTO_VAL_INIT(ec);
      bool r = boost::filesystem::is_directory(dir.path(), ec);
      if (r)
      {
        if (get_tick_count__() - last_tick > 300)
        {
          last_tick = get_tick_count__();
          //std::cout << "\r                                                                                                                                        \r ->" << dir.path();
        }
        bool r = search_for_wallet_file(dir.path().wstring());
        if (r)
          return true;
      }
      else
      {
        boost::system::error_code ec = AUTO_VAL_INIT(ec);
        bool r = boost::filesystem::is_regular_file(dir.path(), ec);
        if (!r)
        {
          //LOG_PRINT_L0("Skiping as not regular: " << epee::string_encoding::convert_to_ansii(dir.path().wstring()));
          return false;

        }
        //LOG_PRINT_L0("FILE: " << dir.path().string());
        std::wstring pa = dir.path().wstring();
        r = check_if_file_looks_like_a_wallet(pa);
        if (r)
          return true;

      }
    }
  }
  catch (std::exception& /* ex*/)
  {
    //std::cout << "\r                                                                           \r";
    LOG_PRINT_CYAN("Skip: " << search_here, LOG_LEVEL_0);
    return false;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------
#ifdef WIN32
int wmain( int argc, wchar_t* argv_w[ ], wchar_t* envp[ ] )
#else
int main(int argc, char* argv[])
#endif
{
#ifdef WIN32
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  //_CrtSetBreakAlloc(9594);
#endif

  TRY_ENTRY();

  tools::signal_handler::install_fatal([](int sig_number, void* address) {
    std::cout << "\n\nFATAL ERROR\nsig: " << sig_number << ", address: " << address; // in case error happens before log init
    std::fflush(nullptr); // all open output streams are flushed
    LOG_ERROR("\n\nFATAL ERROR\nsig: " << sig_number << ", address: " << address);
    std::fflush(nullptr);
  });

#ifdef WIN32
  // windows: convert argv_w into UTF-8-encoded std::string the same way it is in Linux and macOS
  std::vector<std::string> argv_str(argc, "");
  std::vector<const char*> argv_vec(argc, nullptr);
  for (size_t i = 0; i < argc; ++i)
  {
    argv_str[i] = epee::string_encoding::wstring_to_utf8( argv_w[i] );
    argv_vec[i] = argv_str[i].c_str();
  }
  const char* const* argv = argv_vec.data();
#endif

  string_tools::set_module_name_and_folder(argv[0]);

  po::options_description desc_general("General options");
  command_line::add_arg(desc_general, command_line::arg_help);
  command_line::add_arg(desc_general, command_line::arg_version);

  po::options_description desc_params("Wallet options", 160);
  command_line::add_arg(desc_params, arg_wallet_file);
  command_line::add_arg(desc_params, arg_generate_new_wallet);
  command_line::add_arg(desc_params, arg_generate_new_auditable_wallet);
  command_line::add_arg(desc_params, arg_password);
  command_line::add_arg(desc_params, arg_daemon_address);
  command_line::add_arg(desc_params, arg_daemon_host);
  command_line::add_arg(desc_params, arg_daemon_port);
  command_line::add_arg(desc_params, arg_command);
  //command_line::add_arg(desc_params, arg_log_level);
  command_line::add_arg(desc_params, arg_dont_refresh);
  command_line::add_arg(desc_params, arg_dont_set_date);
  command_line::add_arg(desc_params, arg_do_pos_mining);
  command_line::add_arg(desc_params, arg_pos_mining_reward_address);
  command_line::add_arg(desc_params, arg_pos_mining_defrag);
  command_line::add_arg(desc_params, arg_restore_wallet);
  command_line::add_arg(desc_params, arg_offline_mode);
  command_line::add_arg(desc_params, command_line::arg_log_file);
  command_line::add_arg(desc_params, command_line::arg_log_level);
  command_line::add_arg(desc_params, arg_scan_for_wallet);
  command_line::add_arg(desc_params, arg_addr_to_compare);
  command_line::add_arg(desc_params, arg_disable_tor_relay);
  command_line::add_arg(desc_params, arg_set_timeout);
  command_line::add_arg(desc_params, arg_voting_config_file);
  command_line::add_arg(desc_params, arg_no_password_confirmations);
  command_line::add_arg(desc_params, command_line::arg_generate_rpc_autodoc);    

  tools::wallet_rpc_server::init_options(desc_params);

  po::positional_options_description positional_options;
  positional_options.add(arg_command.name, -1);

  shared_ptr<currency::simple_wallet> sw(new currency::simple_wallet);

  po::options_description desc_all;
  desc_all.add(desc_general).add(desc_params);
  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_all, [&]()
  {
    po::store(command_line::parse_command_line(argc, argv, desc_general, true), vm);

    if (command_line::get_arg(vm, command_line::arg_help))
    {
      success_msg_writer() << "Usage: simplewallet [--wallet-file=<file>|--generate-new-wallet=<file>] [--daemon-address=<host>:<port>] [<COMMAND>]";
      success_msg_writer() << desc_all << '\n' << sw->get_commands_str();
      return false;
    }
    else if (command_line::get_arg(vm, command_line::arg_version))
    {
      success_msg_writer() << CURRENCY_NAME << " wallet v" << PROJECT_VERSION_LONG;
      return false;
    }

    auto parser = po::command_line_parser(argc, argv).options(desc_params).positional(positional_options);
    po::store(parser.run(), vm);
    po::notify(vm);
    return true;
  });
  if (!r)
    return EXIT_FAILURE;

  //set up logging options
  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_0);
  boost::filesystem::path log_file_path(command_line::get_arg(vm, command_line::arg_log_file));
  if (log_file_path.empty())
    log_file_path = log_space::log_singletone::get_default_log_file();
  std::string log_dir;
  log_dir = log_file_path.has_parent_path() ? log_file_path.parent_path().string() : log_space::log_singletone::get_default_log_folder();
  log_space::log_singletone::add_logger(LOGGER_FILE, log_file_path.filename().string().c_str(), log_dir.c_str(), LOG_LEVEL_4);
  message_writer(epee::log_space::console_color_white, true, std::string(), LOG_LEVEL_0) << CURRENCY_NAME << " simplewallet v" << PROJECT_VERSION_LONG;

  if (command_line::has_arg(vm, command_line::arg_log_level))
  {
    int old_log_level = log_space::get_set_log_detalisation_level(false);
    int new_log_level = command_line::get_arg(vm, command_line::arg_log_level);
    log_space::get_set_log_detalisation_level(true, new_log_level);
    LOG_PRINT_L0("Log level changed: " << old_log_level << " -> " << new_log_level);
    message_writer(epee::log_space::console_color_white, true) << "Log level changed: " << old_log_level << " -> " << new_log_level;
  }

  if (command_line::has_arg(vm, command_line::arg_generate_rpc_autodoc))
  {
    tools::wallet_rpc_server wallet_rpc_server(std::shared_ptr<tools::wallet2>(new tools::wallet2()));
    std::string path_to_generate = command_line::get_arg(vm, command_line::arg_generate_rpc_autodoc);

    if (!generate_doc_as_md_files(path_to_generate, wallet_rpc_server))
      return 1;
    return 0;
  }


  if (command_line::has_arg(vm, arg_scan_for_wallet))
  {
    log_space::log_singletone::add_logger(LOGGER_CONSOLE, nullptr, nullptr, LOG_LEVEL_4);
    LOG_PRINT_L0("Searching from "
      << epee::string_encoding::convert_to_ansii(command_line::get_arg(vm, arg_scan_for_wallet)));

    search_for_wallet_file(epee::string_encoding::convert_to_unicode(command_line::get_arg(vm, arg_scan_for_wallet)));
    return EXIT_SUCCESS;
  }

  bool offline_mode = command_line::get_arg(vm, arg_offline_mode);

  if (command_line::has_arg(vm, tools::wallet_rpc_server::arg_rpc_bind_port))
  {
    // runs wallet as RPC server
    log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_2);
    sw->set_offline_mode(offline_mode);
    LOG_PRINT_L0("Starting in RPC server mode...");

    if (!command_line::has_arg(vm, arg_wallet_file) || command_line::get_arg(vm, arg_wallet_file).empty())
    {
      LOG_ERROR("Wallet file is not set.");
      return EXIT_FAILURE;
    }

    if (!command_line::has_arg(vm, arg_daemon_address) && !command_line::has_arg(vm, arg_offline_mode))
    {
      LOG_ERROR("Daemon address is not set.");
      return EXIT_FAILURE;
    }

    tools::password_container pwd_container;
    if (command_line::has_arg(vm, arg_password))
    {
      pwd_container.password(command_line::get_arg(vm, arg_password));
    }
    else
    {
      bool r = pwd_container.read_password();
      if (!r)
      {
        LOG_ERROR("failed to read wallet password");
        return EXIT_FAILURE;
      }
    }

    std::string wallet_file = command_line::get_arg(vm, arg_wallet_file);
    std::string daemon_address = command_line::get_arg(vm, arg_daemon_address);
    std::string daemon_host = command_line::get_arg(vm, arg_daemon_host);
    int daemon_port = command_line::get_arg(vm, arg_daemon_port);
    if (daemon_host.empty())
      daemon_host = "localhost";
    if (!daemon_port)
      daemon_port = RPC_DEFAULT_PORT;
    if (daemon_address.empty())
      daemon_address = std::string("http://") + daemon_host + ":" + std::to_string(daemon_port);

    std::shared_ptr<tools::wallet2> wallet_ptr(new tools::wallet2());
    tools::wallet2& wal = *wallet_ptr;
    //try to open it
    while (true)
    {
      try
      {
        LOG_PRINT_L0("Loading wallet...");
        wal.load(epee::string_encoding::utf8_to_wstring(wallet_file), pwd_container.password());
      }
      catch (const tools::error::wallet_load_notice_wallet_restored& e)
      {
        LOG_ERROR("Wallet initialize was with problems, but still worked : " << e.what());
      }
      catch (const std::exception& e)
      {
        LOG_ERROR("Wallet initialize failed: " << e.what());
        return EXIT_FAILURE;
      }
      break;
    }
    //try to sync it
    struct wallet_rpc_local_callback : public tools::i_wallet2_callback
    {
      std::shared_ptr<tools::wallet2> m_wlt_ptr;
      wallet_rpc_local_callback(std::shared_ptr<tools::wallet2> ptr): m_wlt_ptr(ptr)
      {}
      virtual void on_mw_get_wallets(std::vector<tools::wallet_public::wallet_entry_info>& wallets) 
      {
        wallets.push_back(tools::wallet_public::wallet_entry_info());
        wallets.back().wallet_id = 0;
        tools::get_wallet_info(*m_wlt_ptr, wallets.back().wi);
      }

    };

    std::shared_ptr<tools::i_wallet2_callback> callback(new wallet_rpc_local_callback(wallet_ptr));

    while (true)
    {
      try
      {
        LOG_PRINT_L0("Initializing wallet...");
        process_wallet_command_line_params(vm, wal);
        wal.init(daemon_address);
        display_vote_info(wal);
        if (command_line::get_arg(vm, arg_generate_new_wallet).size() || command_line::get_arg(vm, arg_generate_new_auditable_wallet).size())
          return EXIT_FAILURE;

        wal.set_use_assets_whitelisting(true);
        wal.callback(callback);

        if (!offline_mode)
          wal.refresh();
        LOG_PRINT_GREEN("Loaded ok", LOG_LEVEL_0);
      }
      catch (const std::exception& e)
      {
        LOG_ERROR("Wallet initialize failed: " << e.what());
        return EXIT_FAILURE;
      }
      break;
    }

    currency::account_public_address miner_address = wal.get_account().get_public_address();
    if (command_line::has_arg(vm, arg_pos_mining_reward_address))
    {
      std::string arg_pos_mining_reward_address_str = command_line::get_arg(vm, arg_pos_mining_reward_address);
      if (!arg_pos_mining_reward_address_str.empty())
      {
        std::string payment_id;
        r = wal.get_transfer_address(arg_pos_mining_reward_address_str, miner_address, payment_id);
        CHECK_AND_ASSERT_MES(r, EXIT_FAILURE, "Failed to parse miner address from string: " << arg_pos_mining_reward_address_str);
        CHECK_AND_ASSERT_MES(payment_id.size() == 0, EXIT_FAILURE, "Address for rewards should not be integrated address: " << arg_pos_mining_reward_address_str);
        LOG_PRINT_YELLOW("PoS reward will be sent to another address: " << arg_pos_mining_reward_address_str, LOG_LEVEL_0);
      }
    }

    if (command_line::has_arg(vm, arg_pos_mining_defrag))
    {
      std::string arg_pos_mining_defrag_str = command_line::get_arg(vm, arg_pos_mining_defrag);
      if (arg_pos_mining_defrag_str.empty())
      {
        // enable with default params
        wal.set_defragmentation_tx_settings(true, DEFAULT_WALLET_MIN_UTXO_COUNT_FOR_DEFRAGMENTATION_TX, DEFAULT_WALLET_MAX_UTXO_COUNT_FOR_DEFRAGMENTATION_TX, COIN);
      }
      else
      {
        if (arg_pos_mining_defrag_str != "disable")
        {
          std::vector<std::string> params;
          boost::split(params, arg_pos_mining_defrag_str, boost::is_any_of(",;"), boost::token_compress_on);
          CHECK_AND_ASSERT_MES(params.size() == 3, EXIT_FAILURE, "incorrect number of params given: " << arg_pos_mining_defrag_str);
          int64_t outs_min = 0, outs_max = 0;
          uint64_t max_amount = 0;
          CHECK_AND_ASSERT_MES(epee::string_tools::string_to_num_fast(params[0], outs_min) && outs_min > 0 && outs_min < 256, EXIT_FAILURE, "incorrect param: " << params[0]);
          CHECK_AND_ASSERT_MES(epee::string_tools::string_to_num_fast(params[1], outs_max) && outs_max > 0 && outs_max < 256, EXIT_FAILURE, "incorrect param: " << params[1]);
          CHECK_AND_ASSERT_MES(currency::parse_amount(max_amount, params[2]), EXIT_FAILURE, "incorrect param: " << params[2]);
          wal.set_defragmentation_tx_settings(true, outs_min, outs_max, max_amount);
        }
      }
    }
    
    

    tools::wallet_rpc_server wrpc(wallet_ptr);
    bool r = wrpc.init(vm);
    CHECK_AND_ASSERT_MES(r, EXIT_FAILURE, "Failed to initialize wallet rpc server");

    tools::signal_handler::install([&wrpc/*, &wal*/ /* TODO(unassigned): use? */] {
      wrpc.send_stop_signal();
    });
    LOG_PRINT_L0("Starting wallet rpc server");
    wrpc.run(command_line::get_arg(vm, arg_do_pos_mining), offline_mode, miner_address);
    LOG_PRINT_L0("Stopped wallet rpc server");
    try
    {
      LOG_PRINT_L0("Storing wallet...");
      wal.store();
      LOG_PRINT_GREEN("Stored ok", LOG_LEVEL_0);
    }
    catch (const std::exception& e)
    {
      LOG_ERROR("Failed to store wallet: " << e.what());
      return EXIT_FAILURE;
    }
  }
  else // if(command_line::has_arg(vm, tools::wallet_rpc_server::arg_rpc_bind_port))
  {
    if (command_line::get_arg(vm, arg_do_pos_mining))
    { 
      // PoS mining can be turned on only in RPC server mode, please provide --rpc-bind-port to make this
      fail_msg_writer() << "PoS mining can only be started in RPC server mode, please use command-line option --rpc-bind-port=<PORT_NUM> along with --do-pos-mining to enable staking";
      return EXIT_FAILURE;
    }

    //runs wallet with console interface
    sw->set_offline_mode(offline_mode);
    r = sw->init(vm);
    CHECK_AND_ASSERT_MES(r, 1, "Failed to initialize wallet");
    if (command_line::get_arg(vm, arg_generate_new_wallet).size() || command_line::get_arg(vm, arg_generate_new_auditable_wallet).size())
      return EXIT_FAILURE;


    std::vector<std::string> command = command_line::get_arg(vm, arg_command);
    if (!command.empty())
    {
      sw->process_command(command);
      sw->stop();
      sw->deinit();
    }
    else
    {
      tools::signal_handler::install([sw] {
        sw->stop();
      });
      sw->run();

      sw->deinit();
    }
  }

  CATCH_ENTRY_L0(__func__, EXIT_FAILURE);
  return EXIT_SUCCESS;
}
