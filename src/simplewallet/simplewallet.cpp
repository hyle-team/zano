// Copyright (c) 2014-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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

#include <cstdlib>

#if defined(WIN32)
#include <crtdbg.h>
#endif

using namespace std;
using namespace epee;
using namespace currency;
using boost::lexical_cast;
namespace po = boost::program_options;

#define EXTENDED_LOGS_FILE "wallet_details.log"


namespace
{
  const command_line::arg_descriptor<std::string> arg_wallet_file = {"wallet-file", "Use wallet <arg>", ""};
  const command_line::arg_descriptor<std::string> arg_generate_new_wallet = {"generate-new-wallet", "Generate new wallet and save it to <arg> or <address>.wallet by default", ""};
  const command_line::arg_descriptor<std::string> arg_generate_new_auditable_wallet = {"generate-new-auditable-wallet", "Generate new auditable wallet and store it to <arg>", ""};
  const command_line::arg_descriptor<std::string> arg_daemon_address = {"daemon-address", "Use daemon instance at <host>:<port>", ""};
  const command_line::arg_descriptor<std::string> arg_daemon_host = {"daemon-host", "Use daemon instance at host <arg> instead of localhost", ""};
  const command_line::arg_descriptor<std::string> arg_password = {"password", "Wallet password", "", true};
  const command_line::arg_descriptor<bool> arg_dont_refresh = { "no-refresh", "Do not refresh after load", false, true };
  const command_line::arg_descriptor<bool> arg_dont_set_date = { "no-set-creation-date", "Do not set wallet creation date", false, false };
  const command_line::arg_descriptor<bool> arg_print_brain_wallet = { "print-brain-wallet", "Print to conosole brain wallet", false, false };
  const command_line::arg_descriptor<int> arg_daemon_port = {"daemon-port", "Use daemon instance at port <arg> instead of default", 0};
  const command_line::arg_descriptor<uint32_t> arg_log_level = {"set-log", "", 0, true};
  const command_line::arg_descriptor<bool> arg_do_pos_mining = { "do-pos-mining", "Do PoS mining", false, false };
  const command_line::arg_descriptor<std::string> arg_pos_mining_reward_address = { "pos-mining-reward-address", "Block reward will be sent to the giving address if specified", "" };
  const command_line::arg_descriptor<std::string> arg_restore_wallet = { "restore-wallet", "Restore wallet from the seed phrase and save it to <arg>", "" };
  const command_line::arg_descriptor<bool> arg_offline_mode = { "offline-mode", "Don't connect to daemon, work offline (for cold-signing process)", false, true };

  const command_line::arg_descriptor< std::vector<std::string> > arg_command = {"command", ""};

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

        if (epee::log_space::console_color_default == m_color)
        {
          std::cout << m_oss.str();
        }
        else
        {
          epee::log_space::set_console_color(m_color, m_bright);
          std::cout << m_oss.str();
          epee::log_space::reset_console_color();
        }
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
  m_print_brain_wallet(false), 
  m_do_refresh_after_load(false),
  m_do_not_set_date(false),
  m_do_pos_mining(false),
  m_refresh_progress_reporter(*this),
  m_offline_mode(false)
{
  m_cmd_binder.set_handler("start_mining", boost::bind(&simple_wallet::start_mining, this, _1), "start_mining <threads_count> - Start mining in daemon");
  m_cmd_binder.set_handler("stop_mining", boost::bind(&simple_wallet::stop_mining, this, _1), "Stop mining in daemon");
  m_cmd_binder.set_handler("refresh", boost::bind(&simple_wallet::refresh, this, _1), "Resynchronize transactions and balance");
  m_cmd_binder.set_handler("balance", boost::bind(&simple_wallet::show_balance, this, _1), "Show current wallet balance");
  m_cmd_binder.set_handler("incoming_transfers", boost::bind(&simple_wallet::show_incoming_transfers, this, _1), "incoming_transfers [available|unavailable] - Show incoming transfers - all of them or filter them by availability");
  m_cmd_binder.set_handler("incoming_counts", boost::bind(&simple_wallet::show_incoming_transfers_counts, this, _1), "incoming_transfers counts");
  m_cmd_binder.set_handler("list_recent_transfers", boost::bind(&simple_wallet::list_recent_transfers, this, _1), "list_recent_transfers - Show recent maximum 1000 transfers");
  m_cmd_binder.set_handler("export_recent_transfers", boost::bind(&simple_wallet::export_recent_transfers, this, _1), "list_recent_transfers_tx - Write recent transfer in json to wallet_recent_transfers.txt");
  m_cmd_binder.set_handler("list_outputs", boost::bind(&simple_wallet::list_outputs, this, _1), "list_outputs [spent|unspent] - Lists all the outputs that have ever been sent to this wallet if called without arguments, otherwise it lists only the spent or unspent outputs");
  m_cmd_binder.set_handler("dump_transfers", boost::bind(&simple_wallet::dump_trunsfers, this, _1), "dump_transfers - Write  transfers in json to dump_transfers.txt");
  m_cmd_binder.set_handler("dump_keyimages", boost::bind(&simple_wallet::dump_key_images, this, _1), "dump_keyimages - Write  key_images in json to dump_key_images.txt");
  m_cmd_binder.set_handler("payments", boost::bind(&simple_wallet::show_payments, this, _1), "payments <payment_id_1> [<payment_id_2> ... <payment_id_N>] - Show payments <payment_id_1>, ... <payment_id_N>");
  m_cmd_binder.set_handler("bc_height", boost::bind(&simple_wallet::show_blockchain_height, this, _1), "Show blockchain height");
  m_cmd_binder.set_handler("wallet_bc_height", boost::bind(&simple_wallet::show_wallet_bcheight, this, _1), "Show blockchain height");
  m_cmd_binder.set_handler("transfer", boost::bind(&simple_wallet::transfer, this, _1), "transfer <mixin_count> <addr_1> <amount_1> [<addr_2> <amount_2> ... <addr_N> <amount_N>] [payment_id] - Transfer <amount_1>,... <amount_N> to <address_1>,... <address_N>, respectively. <mixin_count> is the number of transactions yours is indistinguishable from (from 0 to maximum available), <payment_id> is an optional HEX-encoded string");
  m_cmd_binder.set_handler("set_log", boost::bind(&simple_wallet::set_log, this, _1), "set_log <level> - Change current log detalisation level, <level> is a number 0-4");
  m_cmd_binder.set_handler("enable_console_logger", boost::bind(&simple_wallet::enable_console_logger, this, _1), "Enables console logging");
  m_cmd_binder.set_handler("resync", boost::bind(&simple_wallet::resync_wallet, this, _1), "Causes wallet to reset all transfers and re-synchronize wallet");
  m_cmd_binder.set_handler("help", boost::bind(&simple_wallet::help, this, _1), "Show this help");
  m_cmd_binder.set_handler("get_transfer_info", boost::bind(&simple_wallet::get_transfer_info, this, _1), "displays transfer info by key_image or index");
  m_cmd_binder.set_handler("scan_for_collision", boost::bind(&simple_wallet::scan_for_key_image_collisions, this, _1), "Rescan transfers for key image collisions");
  m_cmd_binder.set_handler("fix_collisions", boost::bind(&simple_wallet::fix_collisions, this, _1), "Rescan transfers for key image collisions");
  m_cmd_binder.set_handler("scan_transfers_for_id", boost::bind(&simple_wallet::scan_transfers_for_id, this, _1), "Rescan transfers for tx_id");
  m_cmd_binder.set_handler("scan_transfers_for_ki", boost::bind(&simple_wallet::scan_transfers_for_ki, this, _1), "Rescan transfers for key image");
  m_cmd_binder.set_handler("print_utxo_distribution", boost::bind(&simple_wallet::print_utxo_distribution, this, _1), "Prints utxo distribution");
  m_cmd_binder.set_handler("sweep_below", boost::bind(&simple_wallet::sweep_below, this, _1), "sweep_below <mixin_count> <address> <amount_lower_limit> [payment_id] -  Tries to transfers all coins with amount below the given limit to the given address");
  
  m_cmd_binder.set_handler("address", boost::bind(&simple_wallet::print_address, this, _1), "Show current wallet public address");
  m_cmd_binder.set_handler("integrated_address", boost::bind(&simple_wallet::integrated_address, this, _1), "integrated_address [<payment_id>|<integrated_address] - encodes given payment_id along with wallet's address into an integrated address (random payment_id will be used if none is provided). Decodes given integrated_address into standard address");
  m_cmd_binder.set_handler("show_seed", boost::bind(&simple_wallet::show_seed, this, _1), "Display secret 24 word phrase that could be used to recover this wallet");
  m_cmd_binder.set_handler("spendkey", boost::bind(&simple_wallet::spendkey, this, _1), "Display secret spend key");
  m_cmd_binder.set_handler("viewkey",  boost::bind(&simple_wallet::viewkey, this, _1), "Display secret view key");
  
  m_cmd_binder.set_handler("get_tx_key", boost::bind(&simple_wallet::get_tx_key, this, _1), "Get transaction one-time secret key (r) for a given <txid>");

  m_cmd_binder.set_handler("save", boost::bind(&simple_wallet::save, this, _1), "Save wallet synchronized data");
  m_cmd_binder.set_handler("save_watch_only", boost::bind(&simple_wallet::save_watch_only, this, _1), "save_watch_only <filename> <password> - save as watch-only wallet file.");

  m_cmd_binder.set_handler("sign_transfer", boost::bind(&simple_wallet::sign_transfer, this, _1), "sign_transfer <unsgined_tx_file> <signed_tx_file> - sign unsigned tx from a watch-only wallet");
  m_cmd_binder.set_handler("submit_transfer", boost::bind(&simple_wallet::submit_transfer, this, _1), "submit_transfer <signed_tx_file> - broadcast signed tx");
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
    fail_msg_writer() << "you must specify --wallet-file, --generate-new-wallet, --generate-new-auditable-wallet or --restore-wallet";
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

  if (command_line::has_arg(vm, arg_print_brain_wallet))
  {
    m_print_brain_wallet = true;
  }

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
    if (!restore_seed_container.read_password("please, enter wallet seed phrase:\n"))
    {
      fail_msg_writer() << "failed to read seed phrase";
      return false;
    }
    
    bool r = restore_wallet(m_restore_wallet, restore_seed_container.password(), pwd_container.password());
    CHECK_AND_ASSERT_MES(r, false, "wallet restoring failed");
  }
  else
  {
    bool r = open_wallet(m_wallet_file, pwd_container.password());
    CHECK_AND_ASSERT_MES(r, false, "could not open account");
  }

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
  m_wallet->set_do_rise_transfer(false);
  try
  {
    m_wallet->generate(epee::string_encoding::utf8_to_wstring(m_wallet_file), password, create_auditable_wallet);
    message_writer(epee::log_space::console_color_white, true) << "Generated new " << (create_auditable_wallet ? "AUDITABLE" : "") << " wallet: " << m_wallet->get_account().get_public_address_str();
    std::cout << "view key: " << string_tools::pod_to_hex(m_wallet->get_account().get_keys().view_secret_key) << std::endl << std::flush;
    if (m_do_not_set_date)
      m_wallet->reset_creation_time(0);

    if (m_print_brain_wallet)
    {
      std::cout << "Brain wallet: " << m_wallet->get_account().get_restore_braindata() << std::endl << std::flush;
    }

  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << "failed to generate new wallet: " << e.what();
    return false;
  }

  m_wallet->init(m_daemon_address);


  success_msg_writer() <<
    "**********************************************************************\n" <<
    "Your wallet has been generated.\n" <<
    "**********************************************************************";
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::restore_wallet(const std::string &wallet_file, const std::string &restore_seed, const std::string& password)
{
  m_wallet_file = wallet_file;

  m_wallet.reset(new tools::wallet2());
  m_wallet->callback(this->shared_from_this());
  m_wallet->set_do_rise_transfer(false);
  try
  {
    m_wallet->restore(epee::string_encoding::utf8_to_wstring(wallet_file), password, restore_seed);
    message_writer(epee::log_space::console_color_white, true) << "Wallet restored: " << m_wallet->get_account().get_public_address_str();
    std::cout << "view key: " << string_tools::pod_to_hex(m_wallet->get_account().get_keys().view_secret_key) << std::endl << std::flush;
    if (m_do_not_set_date)
      m_wallet->reset_creation_time(0);
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << "failed to restore wallet, check your seed phrase!" << ENDL << e.what();
    return false;
  }

  m_wallet->init(m_daemon_address);

  success_msg_writer() <<
    "**********************************************************************\n" <<
    "Your wallet has been restored.\n" <<
    "To start synchronizing with the daemon use \"refresh\" command.\n" <<
    "Use \"help\" command to see the list of available commands.\n" <<
    "Always use \"exit\" command when closing simplewallet to save\n" <<
    "current session's state. Otherwise, you will possibly need to synchronize \n" <<
    "your wallet again. Your wallet key is NOT under risk anyway.\n" <<
    "**********************************************************************";
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::open_wallet(const string &wallet_file, const std::string& password)
{
  m_wallet_file = wallet_file;
  m_wallet.reset(new tools::wallet2());
  m_wallet->callback(shared_from_this());

  while (true)
  {
    try
    {
      m_wallet->load(epee::string_encoding::utf8_to_wstring(m_wallet_file), password);
      message_writer(epee::log_space::console_color_white, true) << "Opened" << (m_wallet->is_auditable() ? " auditable" : "") << (m_wallet->is_watch_only() ? " watch-only" : "") << " wallet: " << m_wallet->get_account().get_public_address_str();

      if (m_print_brain_wallet)
        std::cout << "Brain wallet: " << m_wallet->get_account().get_restore_braindata() << std::endl << std::flush;

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


  m_wallet->init(m_daemon_address);

  if (m_do_refresh_after_load && !m_offline_mode)
    refresh(std::vector<std::string>());

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
std::string print_money_trailing_zeros_replaced_with_spaces(uint64_t amount)
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
void simple_wallet::on_transfer2(const tools::wallet_public::wallet_transfer_info& wti, uint64_t balance, uint64_t unlocked_balance, uint64_t total_mined)
{
  epee::log_space::console_colors color = wti.is_income ? epee::log_space::console_color_green : epee::log_space::console_color_magenta;
  message_writer(color, false) <<
    "height " << wti.height <<
    ", tx " << wti.tx_hash <<
    " " << std::right << std::setw(18) << print_money_trailing_zeros_replaced_with_spaces(wti.amount) << (wti.is_income ? " received," : " spent,   ") <<
    " balance: " << print_money_brief(balance);
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
bool simple_wallet::show_balance(const std::vector<std::string>& args/* = std::vector<std::string>()*/)
{
  success_msg_writer() << "balance: " << print_money(m_wallet->balance()) << ", unlocked balance: " << print_money(m_wallet->unlocked_balance());
  return true;
}
//----------------------------------------------------------------------------------------------------
bool print_wti(const tools::wallet_public::wallet_transfer_info& wti)
{
  epee::log_space::console_colors cl;
  if (wti.is_income)
    cl = epee::log_space::console_color_green;
  else
    cl = epee::log_space::console_color_magenta;

  std::string payment_id_placeholder;
  if (wti.payment_id.size())
    payment_id_placeholder = std::string("(payment_id:") + epee::string_tools::buff_to_hex_nodelimer(wti.payment_id) + ")";

  static const std::string separator = ", ";
  std::string remote_side;
  if (!wti.recipients_aliases.empty())
  {
    for (auto it : wti.recipients_aliases)
      remote_side += remote_side.empty() ? it : (separator + it);
  }
  else
  {
    for (auto it : wti.remote_addresses)
      remote_side += remote_side.empty() ? it : (separator + it);
  }

  message_writer(cl) << epee::misc_utils::get_time_str_v2(wti.timestamp) << " "
    << (wti.is_income ? "Received " : "Sent    ")
    << print_money(wti.amount) << "(fee:" << print_money(wti.fee) << ")  "
    << remote_side
    << " " << wti.tx_hash << payment_id_placeholder;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::list_recent_transfers(const std::vector<std::string>& args)
{
  std::vector<tools::wallet_public::wallet_transfer_info> unconfirmed;
  std::vector<tools::wallet_public::wallet_transfer_info> recent;
  uint64_t total = 0;
  m_wallet->get_recent_transfers_history(recent, 0, 0, total);
  m_wallet->get_unconfirmed_transfers(unconfirmed);
  //workaround for missed fee
  
  success_msg_writer() << "Unconfirmed transfers: ";
  for (auto & wti : unconfirmed)
  {
    if (!wti.fee)
      wti.fee = currency::get_tx_fee(wti.tx);
    print_wti(wti);
  }
  success_msg_writer() << "Recent transfers: ";
  for (auto & wti : recent)
  {
    if (!wti.fee)
      wti.fee = currency::get_tx_fee(wti.tx);
    print_wti(wti);
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
std::string wti_to_text_line(const tools::wallet_public::wallet_transfer_info& wti)
{
  stringstream ss;
  ss << (wti.is_income ? "[INC]" : "[OUT]") << "\t"
    << epee::misc_utils::get_time_str(wti.timestamp) << "\t"
    << print_money(wti.amount) << "\t"
    << print_money(wti.fee) << "\t"
    << wti.remote_addresses << "\t"
    << wti.comment << "\t";
  return ss.str();
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::export_recent_transfers(const std::vector<std::string>& args)
{
  bool export_to_json = true;
  bool ignore_pos = false;
  if (args.size())
  {
    if (args[0] == "json")
      export_to_json = true;
    else if (args[0] == "txt")
      export_to_json = false;
  }
  if (args.size() > 1)
  {
    if (args[1] == "ignore-pos")
      ignore_pos = true;
  }

  std::vector<tools::wallet_public::wallet_transfer_info> unconfirmed;
  std::vector<tools::wallet_public::wallet_transfer_info> recent;
  uint64_t total = 0;
  m_wallet->get_recent_transfers_history(recent, 0, 0, total);
  m_wallet->get_unconfirmed_transfers(unconfirmed);
  //workaround for missed fee
  stringstream ss;
  LOG_PRINT_GREEN("Generating text....", LOG_LEVEL_0);
  ss << "Unconfirmed transfers: " << ENDL;
  for (auto & wti : unconfirmed)
  {
    if(ignore_pos && wti.is_mining)
      continue;
    if (!wti.fee)
      wti.fee = currency::get_tx_fee(wti.tx);
    if(export_to_json)
      ss << epee::serialization::store_t_to_json(wti) << ENDL;
    else
      ss << wti_to_text_line(wti) << ENDL;

  }
  ss << "Recent transfers: " << ENDL;
  for (auto & wti : recent)
  {
    if (ignore_pos && wti.is_mining)
      continue;
    if (!wti.fee)
      wti.fee = currency::get_tx_fee(wti.tx);
    
    if (export_to_json)
      ss << epee::serialization::store_t_to_json(wti) << ENDL;
    else
      ss << wti_to_text_line(wti) << ENDL;
  }
  LOG_PRINT_GREEN("Storing text to wallet_recent_transfers.txt....", LOG_LEVEL_0);
  file_io_utils::save_string_to_file(log_space::log_singletone::get_default_log_folder() + "/wallet_recent_transfers.txt", ss.str());
  LOG_PRINT_GREEN("Done", LOG_LEVEL_0);

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

  tools::wallet2::transfer_container transfers;
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

  tools::wallet2::transfer_container transfers;
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

void print_td_list(const std::list<tools::wallet2::transfer_details>& td)
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
  std::list<tools::wallet2::transfer_details> td;
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
  std::list<tools::wallet2::transfer_details> td;
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
  tools::wallet2::transfer_details td = AUTO_VAL_INIT(td);

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
    std::list<tools::wallet2::payment_details> payments;
    m_wallet->get_payments(arg, payments);
    if (payments.empty())
    {
      success_msg_writer() << "No payments with id " << arg;
      continue;
    }

    for (const tools::wallet2::payment_details& pd : payments)
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
bool simple_wallet::transfer(const std::vector<std::string> &args_)
{
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

  vector<currency::tx_destination_entry> dsts;
  for (size_t i = 0; i < local_args.size(); i += 2) 
  {
    std::string integrated_payment_id;
    currency::tx_destination_entry de;
    de.addr.resize(1);
    if(!(de.addr.size() == 1 && m_wallet->get_transfer_address(local_args[i], de.addr.front(), integrated_payment_id)))
    {
      fail_msg_writer() << "wrong address: " << local_args[i];
      return true;
    }

    if (local_args.size() <= i + 1)
    {
      fail_msg_writer() << "amount for the last address " << local_args[i] << " is not specified";
      return true;
    }

    bool ok = currency::parse_amount(de.amount, local_args[i + 1]);
    if(!ok || 0 == de.amount)
    {
      fail_msg_writer() << "amount is wrong: " << local_args[i] << ' ' << local_args[i + 1] <<
        ", expected number from 0 to " << print_money(std::numeric_limits<uint64_t>::max());
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
  if (!payment_id.empty() && !set_payment_id_to_tx(attachments, payment_id))
  {
    fail_msg_writer() << "provided (or embedded) payment id can't be set: \"" << payment_id << "\"";
    return true;
  }

  try
  {
    currency::transaction tx;
    std::vector<extra_v> extra;
    m_wallet->transfer(dsts, fake_outs_count, 0, m_wallet->get_core_runtime_config().tx_default_fee, extra, attachments, tx);

    if (!m_wallet->is_watch_only())
      success_msg_writer(true) << "Money successfully sent, transaction " << get_transaction_hash(tx) << ", " << get_object_blobsize(tx) << " bytes";
    else
      success_msg_writer(true) << "Transaction prepared for signing and saved into \"zano_tx_unsigned\" file, use full wallet to sign transfer and then use \"submit_transfer\" on this wallet to broadcast the transaction to the network";
  }
  catch (const tools::error::daemon_busy&)
  {
    fail_msg_writer() << "daemon is busy. Please try later";
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    fail_msg_writer() << "no connection to daemon. Please, make sure daemon is running.";
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("Unknown RPC error: " << e.to_string());
    fail_msg_writer() << "RPC error \"" << e.what() << '"';
  }
  catch (const tools::error::get_random_outs_error&)
  {
    fail_msg_writer() << "failed to get random outputs to mix";
  }
  catch (const tools::error::not_enough_money& e)
  {
    fail_msg_writer() << "not enough money to transfer, available only " << print_money(e.available()) <<
      ", transaction amount " << print_money(e.tx_amount() + e.fee()) << " = " << print_money(e.tx_amount()) <<
      " + " << print_money(e.fee()) << " (fee)";
  }
  catch (const tools::error::not_enough_outs_to_mix& e)
  {
    auto writer = fail_msg_writer();
    writer << "not enough outputs for specified mixin_count = " << e.mixin_count() << ":";
    for (const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& outs_for_amount : e.scanty_outs())
    {
      writer << "\noutput amount = " << print_money(outs_for_amount.amount) << ", fount outputs to mix = " << outs_for_amount.outs.size();
    }
  }
  catch (const tools::error::tx_not_constructed&)
  {
    fail_msg_writer() << "transaction was not constructed";
  }
  catch (const tools::error::tx_rejected& e)
  {
    fail_msg_writer() << "transaction " << get_transaction_hash(e.tx()) << " was rejected by daemon with status \"" << e.status() << '"';
  }
  catch (const tools::error::tx_sum_overflow& e)
  {
    fail_msg_writer() << e.what();
  }
  catch (const tools::error::tx_too_big& e)
  {
    currency::transaction tx = e.tx();
    fail_msg_writer() << "transaction " << get_transaction_hash(e.tx()) << " is too big. Transaction size: " <<
      get_object_blobsize(e.tx()) << " bytes, transaction size limit: " << e.tx_size_limit() << " bytes. Try to separate this payment into few smaller transfers.";
  }
  catch (const tools::error::zero_destination&)
  {
    fail_msg_writer() << "one of destinations is zero";
  }
  catch (const tools::error::transfer_error& e)
  {
    LOG_ERROR("unknown transfer error: " << e.to_string());
    fail_msg_writer() << "unknown transfer error: " << e.what();
  }
  catch (const tools::error::wallet_internal_error& e)
  {
    LOG_ERROR("internal error: " << e.to_string());
    fail_msg_writer() << "internal error: " << e.what();
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
  success_msg_writer() << "Here's your wallet's seed phrase. Write it down and keep in a safe place.";
  success_msg_writer(true) << "Anyone who knows the following 26 words can access your wallet:";
  std::cout << m_wallet->get_account().get_restore_braindata() << std::endl << std::flush;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::spendkey(const std::vector<std::string> &args)
{
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
  if (args.size() > 1)
  {
    fail_msg_writer() << "invalid syntax: one or none parameters are expected, " << args.size() << " was given";
    return true;
  }

  bool include_spent = true, include_unspent = true;
  if (args.size() == 1)
  {
    if (args[0] == "unspent" || args[0] == "available")
      include_spent = false;
    else if (args[0] == "spent" || args[0] == "unavailable")
      include_unspent = false;
    else
    {
      fail_msg_writer() << "invalid parameter: " << args[0];
      return true;
    }
  }

  success_msg_writer() << "list of all the outputs that have ever been sent to this wallet:" << ENDL <<
    m_wallet->get_transfers_str(include_spent, include_unspent);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sign_transfer(const std::vector<std::string> &args)
{
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
bool simple_wallet::sweep_below(const std::vector<std::string> &args)
{
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

  try
  {
    uint64_t fee = m_wallet->get_core_runtime_config().tx_default_fee;
    size_t outs_total = 0, outs_swept = 0;
    uint64_t amount_total = 0, amount_swept = 0;
    currency::transaction result_tx = AUTO_VAL_INIT(result_tx);
    std::string filename = "zano_tx_unsigned";
    m_wallet->sweep_below(fake_outs_count, addr, amount, payment_id, fee, outs_total, amount_total, outs_swept, &result_tx, &filename);
    if (!get_inputs_money_amount(result_tx, amount_swept))
      LOG_ERROR("get_inputs_money_amount failed, tx: " << obj_to_json_str(result_tx));

    success_msg_writer(false) << outs_swept << " outputs (" << print_money_brief(amount_swept) << " coins) of " << outs_total << " total (" << print_money_brief(amount_total)
      << ") below the specified limit of " << print_money_brief(amount) << " were successfully swept";

    if (m_wallet->is_watch_only())
      success_msg_writer(true) << "Transaction prepared for signing and saved into \"" << filename << "\" file, use full wallet to sign transfer and then use \"submit_transfer\" on this wallet to broadcast the transaction to the network";
    else
      success_msg_writer(true) << "tx: " << get_transaction_hash(result_tx) << " size: " << get_object_blobsize(result_tx) << " bytes";
  }
  catch (const std::exception& e)
  {
    LOG_ERROR(e.what());
    fail_msg_writer() << e.what();
    return true;
  }

  return true;
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

  po::options_description desc_params("Wallet options");
  command_line::add_arg(desc_params, arg_wallet_file);
  command_line::add_arg(desc_params, arg_generate_new_wallet);
  command_line::add_arg(desc_params, arg_generate_new_auditable_wallet);
  command_line::add_arg(desc_params, arg_password);
  command_line::add_arg(desc_params, arg_daemon_address);
  command_line::add_arg(desc_params, arg_daemon_host);
  command_line::add_arg(desc_params, arg_daemon_port);
  command_line::add_arg(desc_params, arg_command);
  command_line::add_arg(desc_params, arg_log_level);
  command_line::add_arg(desc_params, arg_dont_refresh);
  command_line::add_arg(desc_params, arg_dont_set_date);
  command_line::add_arg(desc_params, arg_print_brain_wallet);
  command_line::add_arg(desc_params, arg_do_pos_mining);
  command_line::add_arg(desc_params, arg_pos_mining_reward_address);
  command_line::add_arg(desc_params, arg_restore_wallet);
  command_line::add_arg(desc_params, arg_offline_mode);
  command_line::add_arg(desc_params, command_line::arg_log_file);
  command_line::add_arg(desc_params, command_line::arg_log_level);


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
  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_2);
  boost::filesystem::path log_file_path(command_line::get_arg(vm, command_line::arg_log_file));
  if (log_file_path.empty())
    log_file_path = log_space::log_singletone::get_default_log_file();
  std::string log_dir;
  log_dir = log_file_path.has_parent_path() ? log_file_path.parent_path().string() : log_space::log_singletone::get_default_log_folder();
  log_space::log_singletone::add_logger(LOGGER_FILE, log_file_path.filename().string().c_str(), log_dir.c_str(), LOG_LEVEL_4);
  message_writer(epee::log_space::console_color_white, true) << CURRENCY_NAME << " wallet v" << PROJECT_VERSION_LONG;

  if (command_line::has_arg(vm, arg_log_level))
  {
    LOG_PRINT_L0("Setting log level = " << command_line::get_arg(vm, arg_log_level));
    log_space::get_set_log_detalisation_level(true, command_line::get_arg(vm, arg_log_level));
  }
  if (command_line::has_arg(vm, command_line::arg_log_level))
  {
    LOG_PRINT_L0("Setting log level = " << command_line::get_arg(vm, command_line::arg_log_level));
    log_space::get_set_log_detalisation_level(true, command_line::get_arg(vm, command_line::arg_log_level));
  }

  bool offline_mode = command_line::get_arg(vm, arg_offline_mode);

  if (command_line::has_arg(vm, tools::wallet_rpc_server::arg_rpc_bind_port))
  {
    // runs wallet as RPC server
    log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_2);
    sw->set_offline_mode(offline_mode);
    LOG_PRINT_L0("Starting wallet RPC server...");

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

    tools::wallet2 wal;
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
    while (true)
    {
      try
      {
        LOG_PRINT_L0("Initializing wallet...");
        wal.init(daemon_address);
        if (command_line::get_arg(vm, arg_generate_new_wallet).size() || command_line::get_arg(vm, arg_generate_new_auditable_wallet).size())
          return EXIT_FAILURE;

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
        r = get_account_address_from_str(miner_address, arg_pos_mining_reward_address_str);
        CHECK_AND_ASSERT_MES(r, EXIT_FAILURE, "Failed to parse miner address from string: " << arg_pos_mining_reward_address_str);
        LOG_PRINT_YELLOW("PoS reward will be sent to another address: " << arg_pos_mining_reward_address_str, LOG_LEVEL_0);
      }
    }

    tools::wallet_rpc_server wrpc(wal);
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
