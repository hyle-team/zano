// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chrono>
#include "wallets_manager.h"
#include "currency_core/alias_helper.h"
#ifndef MOBILE_WALLET_BUILD
  #include "core_fast_rpc_proxy.h"
  #include "currency_core/core_tools.h"
#endif
#include "common/callstack_helper.h"
#include "string_coding.h"
#include "wallet_helpers.h"
#include "core_default_rpc_proxy.h"
#include "common/db_backend_selector.h"
#include "common/pre_download.h"
#include "wallet/wrap_service.h"

#define GET_WALLET_OPT_BY_ID(wallet_id, name) \
  SHARED_CRITICAL_REGION_LOCAL(m_wallets_lock);    \
  auto it = m_wallets.find(wallet_id);      \
  if (it == m_wallets.end())                \
    return API_RETURN_CODE_WALLET_WRONG_ID; \
  auto& name = it->second;

#define GET_WALLET_BY_ID(wallet_id, name)       \
  SHARED_CRITICAL_REGION_LOCAL(m_wallets_lock);    \
  auto it = m_wallets.find(wallet_id);      \
  if (it == m_wallets.end())                \
    return API_RETURN_CODE_WALLET_WRONG_ID; \
  auto& name = it->second.w;

#define GET_WALLET_OPTIONS_BY_ID_VOID_RET(wallet_id, name)       \
  SHARED_CRITICAL_REGION_LOCAL(m_wallets_lock);    \
  auto it = m_wallets.find(wallet_id);      \
  if (it == m_wallets.end())                \
    return; \
  auto& name = it->second;

#ifdef MOBILE_WALLET_BUILD
  #define DAEMON_IDLE_UPDATE_TIME_MS        10000
  #define TX_POOL_SCAN_INTERVAL             5
#else
  #define DAEMON_IDLE_UPDATE_TIME_MS        2000
  #define TX_POOL_SCAN_INTERVAL             1
#endif

#define HTTP_PROXY_TIMEOUT                4000
#define HTTP_PROXY_ATTEMPTS_COUNT         1

const command_line::arg_descriptor<bool> arg_alloc_win_console  ( "alloc-win-console", "Allocates debug console with GUI", false );
const command_line::arg_descriptor<std::string> arg_html_folder  ( "html-path", "Manually set GUI html folder path");
const command_line::arg_descriptor<bool> arg_enable_gui_debug_mode  ( "gui-debug-mode", "Enable debug options in GUI");
const command_line::arg_descriptor<uint32_t> arg_qt_remote_debugging_port  ( "remote-debugging-port", "Specify port for Qt remote debugging");
const command_line::arg_descriptor<std::string> arg_remote_node  ( "remote-node", "Switch GUI to work with remote node instead of local daemon");
const command_line::arg_descriptor<bool> arg_enable_qt_logs  ( "enable-qt-logs", "Forward Qt log messages into main log");
const command_line::arg_descriptor<bool> arg_disable_logs_init("disable-logs-init", "Disable log initialization in GUI");
const command_line::arg_descriptor<std::string> arg_qt_dev_tools  ( "qt-dev-tools", "Enable main web page inspection with Chromium DevTools, <vertical|horizontal>[,scale], e.g. \"horizontal,1.3\"", "");
const command_line::arg_descriptor<bool> arg_disable_price_fetch("gui-disable-price-fetch", "Disable price fetching in UI(for privacy matter)");

const command_line::arg_descriptor<std::string> arg_xcode_stub("-NSDocumentRevisionsDebugMode", "Substitute for xcode bug");
const command_line::arg_descriptor<std::string> arg_sandbox_disable("no-sandbox", "Substitute for ubuntu/linux rendering problem");

wallets_manager::wallets_manager():m_pview(&m_view_stub),
                                 m_stop_singal_sent(false),
#ifndef MOBILE_WALLET_BUILD
                                 m_ccore(&m_cprotocol),
                                 m_cprotocol(m_ccore, &m_p2psrv),
                                 m_p2psrv(m_cprotocol),
                                 m_rpc_server(m_ccore, m_p2psrv, m_offers_service),
                                 m_rpc_proxy(new tools::core_fast_rpc_proxy(m_rpc_server)),
                                 m_offers_service(nullptr),
                                 m_wallet_rpc_server(this),
#else 
                                 m_rpc_proxy(new tools::default_http_core_proxy()),
#endif                            
                       
                                 m_last_daemon_height(0),                                 
                                 m_wallet_id_counter(0),
                                 m_ui_opt(AUTO_VAL_INIT(m_ui_opt)), 
                                 m_remote_node_mode(false),
                                 m_is_pos_allowed(false),
                                 m_qt_logs_enbaled(false), 
                                 m_dont_save_wallet_at_stop(false), 
                                 m_use_deffered_global_outputs(false), 
                                 m_use_tor(true)
{
#ifndef MOBILE_WALLET_BUILD
  m_offers_service.set_disabled(true);
  m_pproxy_diganostic_info = m_rpc_proxy->get_proxy_diagnostic_info();
#endif
	//m_ccore.get_blockchain_storage().get_attachment_services_manager().add_service(&m_offers_service);
}

wallets_manager::~wallets_manager()
{
  TRY_ENTRY();
  stop();
  LOG_PRINT_L0("[~WALLETS_MANAGER] destroyed");
  CATCH_ENTRY_NO_RETURN();
}

template<typename guarded_code_t, typename error_prefix_maker_t>
bool wallets_manager::do_exception_safe_call(guarded_code_t guarded_code, error_prefix_maker_t error_prefix_maker, std::string& api_return_code_result)
{
  try
  {
    api_return_code_result = API_RETURN_CODE_FAIL; // default, should be reset in guarded_code() callback
    guarded_code();
    return true; // everything is okay, or at least no exceptions happened
  }
  catch (const tools::error::not_enough_money& e)
  {
    LOG_ERROR(error_prefix_maker() << "not enough money: " << e.what());
    api_return_code_result = API_RETURN_CODE_NOT_ENOUGH_MONEY;
  }
  catch (const tools::error::not_enough_outs_to_mix& e)
  {
    LOG_ERROR(error_prefix_maker() << "not enough outs to mix: " << e.what());
    api_return_code_result = API_RETURN_CODE_NOT_ENOUGH_OUTPUTS_FOR_MIXING;
  }
  catch (const tools::error::tx_too_big& e)
  {
    LOG_ERROR(error_prefix_maker() << "transaction is too big: " << e.what());
    api_return_code_result = API_RETURN_CODE_TX_IS_TOO_BIG;
  }
  catch (const tools::error::tx_rejected& e)
  {
    LOG_ERROR(error_prefix_maker() << "transaction " << get_transaction_hash(e.tx()) << " was rejected by daemon with status " << e.status());
    api_return_code_result = API_RETURN_CODE_TX_REJECTED;
  }
  catch (const tools::error::daemon_busy& e)
  {
    LOG_ERROR(error_prefix_maker() << "daemon is busy: " << e.what());
    api_return_code_result = API_RETURN_CODE_BUSY;
  }
  catch (const tools::error::no_connection_to_daemon& e)
  {
    LOG_ERROR(error_prefix_maker() << "no connection to daemon: " << e.what());
    api_return_code_result = API_RETURN_CODE_DISCONNECTED;
  }
  catch (const std::exception& e)
  {
    LOG_ERROR(error_prefix_maker() << "internal error: " << e.what());
    api_return_code_result = API_RETURN_CODE_INTERNAL_ERROR + std::string(":") + e.what();
  }
  catch (...)
  {
    LOG_ERROR(error_prefix_maker() << "unknown error");
    api_return_code_result = API_RETURN_CODE_INTERNAL_ERROR;
  }

  return false; // smth bad happened
}


bool wallets_manager::init_command_line(int argc, char* argv[], std::string& fail_message)
{
  TRY_ENTRY();
  po::options_description desc_cmd_only("Command line options");
  po::options_description desc_cmd_sett("Command line options and settings options");

  command_line::add_arg(desc_cmd_only, command_line::arg_help);
  command_line::add_arg(desc_cmd_only, command_line::arg_version);
  command_line::add_arg(desc_cmd_only, command_line::arg_os_version);
  // tools::get_default_data_dir() can't be called during static initialization
  command_line::add_arg(desc_cmd_sett, command_line::arg_data_dir, tools::get_default_data_dir());
  command_line::add_arg(desc_cmd_only, command_line::arg_stop_after_height);
  command_line::add_arg(desc_cmd_only, command_line::arg_config_file);

  command_line::add_arg(desc_cmd_sett, command_line::arg_log_dir);
  command_line::add_arg(desc_cmd_sett, command_line::arg_log_level);
  command_line::add_arg(desc_cmd_sett, command_line::arg_console);
  command_line::add_arg(desc_cmd_only, command_line::arg_show_details);
  
  command_line::add_arg(desc_cmd_sett, arg_alloc_win_console);
  command_line::add_arg(desc_cmd_sett, arg_sandbox_disable);
  command_line::add_arg(desc_cmd_sett, arg_html_folder);
  command_line::add_arg(desc_cmd_only, arg_xcode_stub);
  command_line::add_arg(desc_cmd_sett, arg_enable_gui_debug_mode);
  command_line::add_arg(desc_cmd_sett, arg_qt_remote_debugging_port);
  command_line::add_arg(desc_cmd_sett, arg_remote_node);
  command_line::add_arg(desc_cmd_sett, arg_enable_qt_logs);
  command_line::add_arg(desc_cmd_sett, arg_disable_logs_init);
  command_line::add_arg(desc_cmd_sett, arg_qt_dev_tools);
  command_line::add_arg(desc_cmd_sett, command_line::arg_no_predownload);
  command_line::add_arg(desc_cmd_sett, command_line::arg_force_predownload);
  command_line::add_arg(desc_cmd_sett, command_line::arg_process_predownload_from_path);
  command_line::add_arg(desc_cmd_sett, command_line::arg_validate_predownload);
  command_line::add_arg(desc_cmd_sett, command_line::arg_predownload_link);
  command_line::add_arg(desc_cmd_only, command_line::arg_deeplink);
  command_line::add_arg(desc_cmd_sett, command_line::arg_disable_ntp);
  command_line::add_arg(desc_cmd_sett, arg_disable_price_fetch);
  


#ifndef MOBILE_WALLET_BUILD
  currency::core::init_options(desc_cmd_sett);
  currency::core_rpc_server::init_options(desc_cmd_sett);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> >::init_options(desc_cmd_sett);
  currency::miner::init_options(desc_cmd_sett);
  bc_services::bc_offers_service::init_options(desc_cmd_sett);
  tools::db::db_backend_selector::init_options(desc_cmd_sett);
#endif

  po::options_description desc_options("Allowed options");
  desc_options.add(desc_cmd_only).add(desc_cmd_sett);

  std::string err_str;
  bool command_line_parsed = command_line::handle_error_helper(desc_options, err_str, [&]()
  {
    po::store(po::parse_command_line(argc, argv, desc_options), m_vm);

    if (command_line::get_arg(m_vm, command_line::arg_help))
    {
      std::cout << CURRENCY_NAME << " v" << PROJECT_VERSION_LONG << ENDL << ENDL;
      std::cout << desc_options << std::endl;
      return false;
    }

    m_data_dir = command_line::get_arg(m_vm, command_line::arg_data_dir);
    std::string config = command_line::get_arg(m_vm, command_line::arg_config_file);

    boost::filesystem::path data_dir_path(epee::string_encoding::utf8_to_wstring(m_data_dir));
    boost::filesystem::path config_path(epee::string_encoding::utf8_to_wstring(config));
    if (!config_path.has_parent_path())
    {
      config_path = data_dir_path / config_path;
    }

    boost::system::error_code ec;
    if (boost::filesystem::exists(config_path, ec))
    {
      po::store(po::parse_config_file<char>(config_path.string<std::string>().c_str(), desc_cmd_sett), m_vm);
    }
    po::notify(m_vm);

    return true;
  });

  if (!command_line_parsed)
  {
    std::stringstream ss;
    ss << "Command line has wrong arguments: " << std::endl;
    for (int i = 0; i != argc; i++)
      ss << "[" << i << "] " << argv[i] << std::endl;
    std::cerr << ss.str() << std::endl << std::flush;

    fail_message = "Error parsing arguments.\n";
    fail_message += err_str + "\n";
    std::stringstream s;
    desc_options.print(s);
    fail_message += s.str();
    return false;
  }

  m_qt_logs_enbaled = command_line::get_arg(m_vm, arg_enable_qt_logs);
  m_qt_dev_tools = command_line::get_arg(m_vm, arg_qt_dev_tools);

  return true;
  CATCH_ENTRY2(false);
}


void terminate_handler_func()
{
  LOG_ERROR("\n\nTERMINATE HANDLER\n"); // should print callstack
  std::fflush(nullptr); // all open output streams are flushed
  std::abort(); // default terminate handler's behavior
}

bool wallets_manager::init(view::i_view* pview_handler)
{
  m_stop_singal_sent = false;
  if (pview_handler)
    m_pview = pview_handler;

  view::daemon_status_info dsi = AUTO_VAL_INIT(dsi);
  dsi.pos_difficulty = dsi.pow_difficulty = "---";
  m_pview->update_daemon_status(dsi);

  tools::signal_handler::install_fatal([](int sig_number, void* address) {
    LOG_ERROR("\n\nFATAL ERROR\nsig: " << sig_number << ", address: " << address);
    std::fflush(nullptr); // all open output streams are flushed
  });

  // setup custom callstack retrieving function
  epee::misc_utils::get_callstack(tools::get_callstack);
//#ifndef MOBILE_WALLET_BUILD
  // setup custom terminate functions
  std::set_terminate(&terminate_handler_func);
//#endif
  //#if !defined(NDEBUG)
  //  log_space::log_singletone::add_logger(LOGGER_DEBUGGER, nullptr, nullptr);
  //#endif
  LOG_PRINT_L0("Initing...");

  TRY_ENTRY();
  //set up logging options
  if (command_line::has_arg(m_vm, arg_alloc_win_console))
  {
    log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
  }
  if (command_line::has_arg(m_vm, command_line::arg_log_level))
  {
    log_space::log_singletone::get_set_log_detalisation_level(true, command_line::get_arg(m_vm, command_line::arg_log_level));
  }
  if (command_line::has_arg(m_vm, arg_enable_gui_debug_mode) && command_line::get_arg(m_vm, arg_enable_gui_debug_mode))
  {
    m_ui_opt.use_debug_mode = true;
  }
  if (command_line::has_arg(m_vm, arg_disable_price_fetch) && command_line::get_arg(m_vm, arg_disable_price_fetch))
  {
    m_ui_opt.disable_price_fetch = true;
  }
  

  //set up logging options
  std::string log_dir;
  std::string log_file_name = log_space::log_singletone::get_default_log_file();
  //check if there was specific option
  if (command_line::has_arg(m_vm, command_line::arg_log_dir))
  {
    log_dir = command_line::get_arg(m_vm, command_line::arg_log_dir);
  }
  else
  {
    log_dir = command_line::get_arg(m_vm, command_line::arg_data_dir);
  }

  log_space::log_singletone::set_default_log_folder(log_dir);

  //setting html path
  std::string path_to_html;
  if (!command_line::has_arg(m_vm, arg_html_folder))
  {
    LOG_PRINT_L0("Detecting APPDIR... ");
#if defined(__unix__) || defined(__linux__)
    const char* env_p = std::getenv("APPDIR");
    LOG_PRINT_L0("APPDIR = " << (void*)env_p);
    if (env_p)
    {
      LOG_PRINT_L0("APPDIR: " << env_p);
    }
    if (env_p && std::strlen(env_p))
    {
      //app running inside AppImage
      LOG_PRINT_L0("APPDIR SET: " << env_p);
      path_to_html = std::string(env_p) + "/usr/bin/html";
    }
    else
#endif
    {
      path_to_html = string_tools::get_current_module_folder() + "/html";
    }
  }
  else
  {
    path_to_html = command_line::get_arg(m_vm, arg_html_folder);
  }

  if (command_line::has_arg(m_vm, arg_remote_node))
  {
    m_remote_node_mode = true;
    auto proxy_ptr = new tools::default_http_core_proxy();
    proxy_ptr->set_connectivity(HTTP_PROXY_TIMEOUT,  HTTP_PROXY_ATTEMPTS_COUNT);
    m_rpc_proxy.reset(proxy_ptr);    
    m_rpc_proxy->set_connection_addr(command_line::get_arg(m_vm, arg_remote_node));
    m_pproxy_diganostic_info = m_rpc_proxy->get_proxy_diagnostic_info();
  }

  if(!command_line::has_arg(m_vm, arg_disable_logs_init))
  {
    log_space::log_singletone::add_logger(LOGGER_FILE, log_file_name.c_str(), log_dir.c_str());
    LOG_PRINT_L0(CURRENCY_NAME << " v" << PROJECT_VERSION_LONG);
    //LOG_PRINT("Module folder: " << argv[0], LOG_LEVEL_0);
  }

  m_pview->init(path_to_html);

  return true;
  CATCH_ENTRY_L0("init", false);
}

bool wallets_manager::start()
{
  TRY_ENTRY();

  m_main_worker_thread = std::thread([this](){main_worker(this->m_vm);});

  return true;
  CATCH_ENTRY_L0("main", false);
 }



bool wallets_manager::stop()
{
  send_stop_signal();
  if (m_main_worker_thread.joinable())
  {
	  LOG_PRINT_L0("Waiting for backend main worker thread: " << m_main_worker_thread.get_id());
	  m_main_worker_thread.join();
  }
   
  return true;
}


bool wallets_manager::quick_stop_no_save() //stop without storing wallets
{
  m_dont_save_wallet_at_stop = true;
  bool r = stop();
  EXCLUSIVE_CRITICAL_REGION_BEGIN(m_wallets_lock);
  m_wallets.clear();
  m_wallet_log_prefixes.clear();
  m_stop_singal_sent = false;
  EXCLUSIVE_CRITICAL_REGION_END();
  return r;
}

bool wallets_manager::quick_clear_wallets_no_save() //stop without storing wallets
{
  SHARED_CRITICAL_REGION_BEGIN(m_wallets_lock);
  LOG_PRINT_L0("Wallets[" << m_wallets.size() << "] stopping...");
  for (auto& w : m_wallets)
  {
    w.second.stop(false);
  }
  LOG_PRINT_L0("Wallets[" << m_wallets.size() << "] waiting...");
  for (auto& w : m_wallets)
  {
    w.second.stop(true);
  }
  SHARED_CRITICAL_REGION_END();

  EXCLUSIVE_CRITICAL_REGION_BEGIN(m_wallets_lock);
  LOG_PRINT_L0("Wallets[" << m_wallets.size() << "] closing...");
  m_wallets.clear();
  m_wallet_log_prefixes.clear();
  m_stop_singal_sent = false;
  EXCLUSIVE_CRITICAL_REGION_END();
  return true;
}

std::string wallets_manager::get_config_folder()
{
  return m_data_dir;
}
#define CHECK_AND_ASSERT_AND_SET_GUI(cond, mess) \
  if (!cond) \
  { \
    LOG_ERROR(mess); \
    dsi.daemon_network_state = currency::COMMAND_RPC_GET_INFO::daemon_network_state_internal_error; \
    m_pview->update_daemon_status(dsi); \
    m_pview->on_backend_stopped(); \
    return false; \
  }

bool wallets_manager::init_local_daemon()
{
#ifndef MOBILE_WALLET_BUILD
  view::daemon_status_info dsi = AUTO_VAL_INIT(dsi);
  dsi.pos_difficulty = dsi.pos_difficulty = "---";
  dsi.daemon_network_state = currency::COMMAND_RPC_GET_INFO::daemon_network_state_loading_core;
  m_pview->update_daemon_status(dsi);

  // pre-downloading handling
  tools::db::db_backend_selector dbbs;
  bool res = dbbs.init(m_vm);
  CHECK_AND_ASSERT_AND_SET_GUI(res,  "Failed to initialize db_backend_selector");
  if (!command_line::has_arg(m_vm, command_line::arg_no_predownload) || command_line::has_arg(m_vm, command_line::arg_force_predownload))
  {
    auto last_update = std::chrono::system_clock::now();
    res = tools::process_predownload(m_vm, [&](uint64_t total_bytes, uint64_t received_bytes){
      auto dif = std::chrono::system_clock::now() - last_update;
      if (dif > std::chrono::milliseconds(300))
      {
        dsi.download_total_data_size = total_bytes;
        dsi.downloaded_bytes = received_bytes;
        dsi.daemon_network_state = currency::COMMAND_RPC_GET_INFO::daemon_network_state_downloading_database;
        m_pview->update_daemon_status(dsi);
        last_update = std::chrono::system_clock::now();
      }

      return static_cast<bool>(m_stop_singal_sent);
    });
    if (!res)
    {
      LOG_PRINT_RED("pre-downloading failed, continue with normal network synchronization", LOG_LEVEL_0);
    }
  }




  //initialize core here
  LOG_PRINT_L0("Initializing core...");
  dsi.daemon_network_state = currency::COMMAND_RPC_GET_INFO::daemon_network_state_loading_core;
  //dsi.text_state = "Initializing core";
  m_pview->update_daemon_status(dsi);
  res = m_ccore.init(m_vm);
  CHECK_AND_ASSERT_AND_SET_GUI(res,  "Failed to initialize core");
  LOG_PRINT_L0("Core initialized OK");

  //check if offers module synchronized with blockchaine storage
  auto& bcs = m_ccore.get_blockchain_storage();
  if (!m_offers_service.is_disabled() && bcs.get_current_blockchain_size() > 1 && bcs.get_top_block_id() != m_offers_service.get_last_seen_block_id())
  {
    res = resync_market(bcs, m_offers_service);
    CHECK_AND_ASSERT_AND_SET_GUI(res, "Failed to initialize core: resync_market");
    //clear events that came after resync market
    currency::i_core_event_handler* ph = m_ccore.get_blockchain_storage().get_event_handler();
    if (ph) ph->on_clear_events();
  }

  currency::checkpoints checkpoints;
  res = currency::create_checkpoints(checkpoints);
  CHECK_AND_ASSERT_MES(res, false, "Failed to initialize checkpoints");
  res = m_ccore.set_checkpoints(std::move(checkpoints));
  CHECK_AND_ASSERT_AND_SET_GUI(res, "Failed to initialize core: set_checkpoints failed");

  //initialize objects
  LOG_PRINT_L0("Initializing p2p server...");
  m_pview->update_daemon_status(dsi);
  res = m_p2psrv.init(m_vm);
  CHECK_AND_ASSERT_AND_SET_GUI(res, "Failed to initialize p2p server.");
  LOG_PRINT_L0("P2p server initialized OK on port: " << m_p2psrv.get_this_peer_port());

  //LOG_PRINT_L0("Starting UPnP");
  //upnp_helper.run_port_mapping_loop(p2psrv.get_this_peer_port(), p2psrv.get_this_peer_port(), 20*60*1000);

  LOG_PRINT_L0("Initializing currency protocol...");
  m_pview->update_daemon_status(dsi);
  res = m_cprotocol.init(m_vm);
  CHECK_AND_ASSERT_AND_SET_GUI(res, "Failed to initialize currency protocol.");
  LOG_PRINT_L0("Currency protocol initialized OK");

  LOG_PRINT_L0("Initializing core rpc server...");
  //dsi.text_state = "Initializing core rpc server";
  m_pview->update_daemon_status(dsi);
  res = m_rpc_server.init(m_vm);
  CHECK_AND_ASSERT_AND_SET_GUI(res, "Failed to initialize core rpc server.");
  LOG_PRINT_GREEN("Core rpc server initialized OK on port: " << m_rpc_server.get_binded_port(), LOG_LEVEL_0);

  m_ui_opt.rpc_port = m_rpc_server.get_binded_port();


  //chain calls to rpc server
  m_prpc_chain_handler = &m_wallet_rpc_server;
  //disable this for main net until we get full support of authentication with network

  LOG_PRINT_L0("Starting core rpc server...");
  //dsi.text_state = "Starting core rpc server";
  m_pview->update_daemon_status(dsi);
  res = m_rpc_server.run(2, false);
  CHECK_AND_ASSERT_AND_SET_GUI(res, "Failed to initialize core rpc server.");
  LOG_PRINT_L0("Core rpc server started ok");

  m_core_initialized = true;
  LOG_PRINT_L0("Starting p2p net loop...");
  //dsi.text_state = "Starting network loop";
  m_pview->update_daemon_status(dsi);
  res = m_p2psrv.run(false);
  CHECK_AND_ASSERT_AND_SET_GUI(res, "Failed to run p2p loop.");
  LOG_PRINT_L0("p2p net loop stopped");
#endif
  return true;
}

std::string wallets_manager::setup_wallet_rpc(const std::string& jwt_secret)
{
#ifndef MOBILE_WALLET_BUILD
  if (!jwt_secret.size())
  {
    //disabling wallet RPC
    m_rpc_server.set_rpc_chain_handler(nullptr);
    return WALLET_RPC_STATUS_OK;
  }

  //we don't override command line JWT secret
  //if(!m_wallet_rpc_server.get_jwt_secret().size() ) 
  m_wallet_rpc_server.set_jwt_secret(jwt_secret);

  m_rpc_server.set_rpc_chain_handler(this);
#endif
  return WALLET_RPC_STATUS_OK;
}

bool wallets_manager::deinit_local_daemon()
{
#ifndef MOBILE_WALLET_BUILD
  view::daemon_status_info dsi = AUTO_VAL_INIT(dsi);
  dsi.daemon_network_state = currency::COMMAND_RPC_GET_INFO::daemon_network_state_unloading_core;
  m_pview->update_daemon_status(dsi);

  LOG_PRINT_L0("Stopping core p2p server...");
  //dsi.text_state = "Stopping p2p network server";
  m_pview->update_daemon_status(dsi);
  m_p2psrv.send_stop_signal();
  m_p2psrv.timed_wait_server_stop(1000 * 60);

  //stop components
  LOG_PRINT_L0("Stopping core rpc server...");
  //dsi.text_state = "Stopping rpc network server";
  m_pview->update_daemon_status(dsi);

  m_rpc_server.send_stop_signal();
  m_rpc_server.timed_wait_server_stop(5000);

  //deinitialize components


  LOG_PRINT_L0("Deinitializing rpc server ...");
  //dsi.text_state = "Deinitializing rpc server";
  m_pview->update_daemon_status(dsi);
  m_rpc_server.deinit();


  LOG_PRINT_L0("Deinitializing currency_protocol...");
  //dsi.text_state = "Deinitializing currency_protocol";
  m_pview->update_daemon_status(dsi);
  m_cprotocol.deinit();


  LOG_PRINT_L0("Deinitializing p2p...");
  //dsi.text_state = "Deinitializing p2p";
  m_pview->update_daemon_status(dsi);
  m_p2psrv.deinit();

  m_ccore.set_currency_protocol(NULL);
  m_cprotocol.set_p2p_endpoint(NULL);

  LOG_PRINT("Node stopped.", LOG_LEVEL_0);
  //dsi.text_state = "Node stopped";
  m_pview->update_daemon_status(dsi);


  m_pview->update_daemon_status(dsi);
  LOG_PRINT_L0("Deinitializing market...");
  m_offers_service.set_last_seen_block_id(m_ccore.get_blockchain_storage().get_top_block_id());
  (static_cast<currency::i_bc_service&>(m_offers_service)).deinit();

  LOG_PRINT_L0("Deinitializing core...");
  //dsi.text_state = "Deinitializing core";
  m_pview->update_daemon_status(dsi);
  m_ccore.deinit();
#endif
  return true;
}

void wallets_manager::main_worker(const po::variables_map& m_vm)
{
	log_space::log_singletone::set_thread_log_prefix("[BACKEND_M]");
  TRY_ENTRY();

  if (!m_remote_node_mode)
  {
    bool r = init_local_daemon();
    if (!r)
      return;
  }

  //go to monitoring view loop
  loop();
  

  SHARED_CRITICAL_REGION_BEGIN(m_wallets_lock);
  //first send stop signal to all wallets
  for (auto& wo : m_wallets)
  {
    try
    {
      wo.second.stop(false);
    }
    catch (const std::exception& e)
    {
      LOG_ERROR("Exception rised at storing wallet: " << e.what());
      continue;
    }
    catch (...)
    {
      LOG_ERROR("Exception rised at storing wallet");
      continue;
    }
  }

  for (auto& wo : m_wallets)
  {
    LOG_PRINT_L0("Storing wallet data...");
    //dsi.text_state = "Storing wallets data...";
    //m_pview->update_daemon_status(dsi);
    try
    {
      wo.second.stop();
      if(!m_dont_save_wallet_at_stop)
        wo.second.w->get()->store();
    }
    catch (const std::exception& e)
    {
      LOG_ERROR("Exception rised at storing wallet: " << e.what());
      continue;
    }
    catch (...)
    {
      LOG_ERROR("Exception rised at storing wallet");
      continue;
    }

  }
  CRITICAL_REGION_END();

  if (!m_remote_node_mode)
  {
    bool r = deinit_local_daemon();
    if (!r)
      return;
  }

  m_pview->on_backend_stopped();
  CATCH_ENTRY_L0("wallets_manager::main_worker", void());
}

bool wallets_manager::update_state_info()
{
  view::daemon_status_info dsi = AUTO_VAL_INIT(dsi);
  currency::COMMAND_RPC_GET_INFO::request req = AUTO_VAL_INIT(req);
  req.flags = COMMAND_RPC_GET_INFO_FLAG_EXPIRATIONS_MEDIAN | COMMAND_RPC_GET_INFO_FLAG_NET_TIME_DELTA_MEDIAN;
  currency::COMMAND_RPC_GET_INFO::response inf = AUTO_VAL_INIT(inf);
  if (!m_rpc_proxy->call_COMMAND_RPC_GET_INFO(req, inf))
  {
    //dsi.text_state = "get_info failed";
    dsi.is_disconnected = true;
    m_last_daemon_network_state = dsi.daemon_network_state;
    m_pview->update_daemon_status(dsi);
    LOG_PRINT_RED_L0("Failed to call get_info");
    return false;
  }
  m_is_pos_allowed = inf.pos_allowed;
  dsi.alias_count = inf.alias_count;
  dsi.pow_difficulty = std::to_string(inf.pow_difficulty);
  dsi.pos_difficulty = inf.pos_difficulty;
//  dsi.hashrate = inf.current_network_hashrate_350;
  dsi.inc_connections_count = inf.incoming_connections_count;
  dsi.out_connections_count = inf.outgoing_connections_count;
  dsi.daemon_network_state = inf.daemon_network_state;
  dsi.synchronization_start_height = inf.synchronization_start_height;
  dsi.max_net_seen_height = inf.max_net_seen_height;
  dsi.net_time_delta_median = inf.net_time_delta_median;
  dsi.synchronized_connections_count = inf.synchronized_connections_count;
  dsi.is_pos_allowed = inf.pos_allowed;
  dsi.expiration_median_timestamp = inf.expiration_median_timestamp; 

  dsi.last_build_available = std::to_string(inf.mi.ver_major)
    + "." + std::to_string(inf.mi.ver_minor)
    + "." + std::to_string(inf.mi.ver_revision)
    + "." + std::to_string(inf.mi.build_no);

  if (inf.mi.mode)
  {
    dsi.last_build_displaymode = inf.mi.mode + 1;
  }
  else
  {
    if (inf.mi.build_no > PROJECT_VERSION_BUILD_NO)
      dsi.last_build_displaymode = view::ui_last_build_displaymode::ui_lb_dm_new;
    else
    {
      dsi.last_build_displaymode = view::ui_last_build_displaymode::ui_lb_dm_actual;
    }
  }
  
  get_last_blocks(dsi);
  m_last_daemon_network_state = dsi.daemon_network_state;
  m_last_daemon_height = dsi.height = inf.height;
  m_pview->update_daemon_status(dsi);
  return true;
}

bool wallets_manager::get_last_blocks(view::daemon_status_info& dsi)
{

  return true;
}



void wallets_manager::toggle_pos_mining()
{
  //m_do_mint = !m_do_mint;
  //update_wallets_info();
}

bool wallets_manager::send_stop_signal()
{
  m_stop_singal_sent = true;
  m_stop_singal_sent_mutex_cv.notify_one();
  return true;
}


void wallets_manager::loop()
{

  while (!m_stop_singal_sent)
  {
    if (!m_stop_singal_sent)
    {
      update_state_info();
    }
    {
      std::unique_lock<std::mutex> lk(m_stop_singal_sent_mutex);
      m_stop_singal_sent_mutex_cv.wait_for(lk, std::chrono::milliseconds(DAEMON_IDLE_UPDATE_TIME_MS), [&] {return m_stop_singal_sent.load(); });
    }
  }
}

void wallets_manager::init_wallet_entry(wallet_vs_options& wo, uint64_t id)
{
  wo.wallet_id = id;
  wo.do_mining = false;
  wo.major_stop = false;
  wo.stop_for_refresh = false;
  wo.plast_daemon_height = &m_last_daemon_height;
  wo.plast_daemon_network_state = &m_last_daemon_network_state;
  //wo.plast_daemon_is_disconnected = &(m_rpc_proxy->get_proxy_diagnostic_info().last_daemon_is_disconnected;
  wo.m_pproxy_diagnostig_info = m_rpc_proxy->get_proxy_diagnostic_info();
  wo.pview = m_pview;  
  wo.has_related_alias_in_unconfirmed = false;
  wo.rpc_wrapper.reset(new tools::wallet_rpc_server(wo.w.unlocked_get()));
  if (m_remote_node_mode)
    wo.core_conf = currency::get_default_core_runtime_config();
  else
  {
#ifndef MOBILE_WALLET_BUILD
    wo.core_conf = m_ccore.get_blockchain_storage().get_core_runtime_config();
#else 
    LOG_ERROR("Unexpected location reached");
#endif
  }
    

  // update wallet log prefix for further usage
  {
    CRITICAL_REGION_LOCAL(m_wallet_log_prefixes_lock);
    if (m_wallet_log_prefixes.size() <= id)
      m_wallet_log_prefixes.resize(id + 1);
    m_wallet_log_prefixes[id] = std::string("[") + epee::string_tools::num_to_string_fast(id) + ":" + wo.w->get()->get_account().get_public_address_str().substr(0, 6) + "] ";
  }
  
  wo.w->get()->set_disable_tor_relay(!m_use_tor);
}


std::string wallets_manager::get_tx_pool_info(currency::COMMAND_RPC_GET_POOL_INFO::response& res)
{
  currency::COMMAND_RPC_GET_POOL_INFO::request req = AUTO_VAL_INIT(req);
  res = AUTO_VAL_INIT_T(currency::COMMAND_RPC_GET_POOL_INFO::response);
  if (!m_rpc_proxy->call_COMMAND_RPC_GET_POOL_INFO(req, res))
  {
    return API_RETURN_CODE_FAIL;
  }
  return API_RETURN_CODE_OK;
}


std::string wallets_manager::export_wallet_history(const view::export_wallet_info& ewi)
{
  GET_WALLET_OPT_BY_ID(ewi.wallet_id, wo);
  try {

    boost::filesystem::ofstream fstream;
    fstream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fstream.open(ewi.path, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
    wo.w->get()->export_transaction_history(fstream, ewi.format, ewi.include_pos_transactions);
    fstream.close();
  }
  catch (...)
  {
    return API_RETURN_CODE_FAIL;
  }
  return API_RETURN_CODE_OK;
}

uint64_t wallets_manager::get_default_fee()
{
  return TX_DEFAULT_FEE;
}
std::string wallets_manager::get_fav_offers(const std::list<bc_services::offer_id>& hashes, const bc_services::core_offers_filter& filter, std::list<bc_services::offer_details_ex>& offers)
{
  if (m_remote_node_mode)
    return API_RETURN_CODE_FAIL;
#ifndef MOBILE_WALLET_BUILD
  currency::blockchain_storage& bcs = m_ccore.get_blockchain_storage();

  m_offers_service.get_offers_by_id(hashes, offers);
  filter_offers_list(offers, filter, bcs.get_core_runtime_config().get_core_time());
  return API_RETURN_CODE_OK;
#else
  return API_RETURN_CODE_FAIL;
#endif
}

std::string wallets_manager::create_ionic_swap_proposal(uint64_t wallet_id, const tools::wallet_public::create_ionic_swap_proposal_request& proposal_req, std::string& result_proposal_hex)
{
  GET_WALLET_OPT_BY_ID(wallet_id, wo);
  try {
    currency::account_public_address dest_account = AUTO_VAL_INIT(dest_account);
    if (!currency::get_account_address_from_str(dest_account, proposal_req.destination_add))
    {
      return API_RETURN_CODE_BAD_ARG;
    }
    tools::wallet_public::ionic_swap_proposal proposal = AUTO_VAL_INIT(proposal);
    bool r = wo.w->get()->create_ionic_swap_proposal(proposal_req.proposal_info, dest_account, proposal);
    if (!r)
    {
      return API_RETURN_CODE_FAIL;
    }
    else
    {
      result_proposal_hex = epee::string_tools::buff_to_hex_nodelimer(t_serializable_object_to_blob(proposal));
      return API_RETURN_CODE_OK;
    }
  }
  catch (...)
  {
    return API_RETURN_CODE_FAIL;
  }
  return API_RETURN_CODE_OK;
}

std::string wallets_manager::get_ionic_swap_proposal_info(uint64_t wallet_id, std::string&raw_tx_template_hex, tools::wallet_public::ionic_swap_proposal_info& proposal)
{
  GET_WALLET_OPT_BY_ID(wallet_id, wo);
  try {
    std::string raw_tx_template;
    bool r = epee::string_tools::parse_hexstr_to_binbuff(raw_tx_template_hex, raw_tx_template);
    if (!r)
    {
      return API_RETURN_CODE_BAD_ARG;
    }

    if (!wo.w->get()->get_ionic_swap_proposal_info(raw_tx_template, proposal))
    {
      return API_RETURN_CODE_FAIL;
    }
  }
  catch (...)
  {
    return API_RETURN_CODE_FAIL;
  }
  return API_RETURN_CODE_OK;
}

std::string wallets_manager::accept_ionic_swap_proposal(uint64_t wallet_id, std::string&raw_tx_template_hex, std::string& result_raw_tx_hex)
{
  GET_WALLET_OPT_BY_ID(wallet_id, wo);
  try {
    std::string raw_tx_template;
    bool r = epee::string_tools::parse_hexstr_to_binbuff(raw_tx_template_hex, raw_tx_template);
    if (!r)
    {
      return API_RETURN_CODE_BAD_ARG;
    }
    currency::transaction result_tx = AUTO_VAL_INIT(result_tx);
    if (!wo.w->get()->accept_ionic_swap_proposal(raw_tx_template, result_tx))
    {
      return API_RETURN_CODE_FAIL;
    }
    result_raw_tx_hex = epee::string_tools::buff_to_hex_nodelimer(t_serializable_object_to_blob(result_tx));
  }
  catch (...)
  {
    return API_RETURN_CODE_FAIL;
  }
  return API_RETURN_CODE_OK;
}
std::string wallets_manager::get_my_offers(const bc_services::core_offers_filter& filter, std::list<bc_services::offer_details_ex>& offers)
{
  if (m_remote_node_mode)
    return API_RETURN_CODE_FAIL;
#ifndef MOBILE_WALLET_BUILD
  SHARED_CRITICAL_REGION_LOCAL(m_wallets_lock);
  while (true)
  {
    try
    {
      size_t wallet_index = 0;
      for (auto& w : m_wallets)
      {
        auto offers_list_proxy = *w.second.offers; // obtain exclusive access to w.second.offers
        size_t offers_count_before = offers.size();
        offers.insert(offers.end(), offers_list_proxy->begin(), offers_list_proxy->end());
        size_t offers_count_after = offers.size();

        std::string wallet_title = "#" + epee::string_tools::num_to_string_fast(wallet_index); //(*(*w_ptr))->get_account().get_public_address_str() + " @ " + std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes((*(*w_ptr))->get_wallet_path());
        CHECK_AND_ASSERT_MES(offers_count_after >= offers_count_before, API_RETURN_CODE_INTERNAL_ERROR, "Invalid offers count after calling get_actual_offers: before: " << offers_count_before << ", after: " << offers_count_after << ", wallet: " << wallet_title);
        size_t offers_added = offers_count_after - offers_count_before;
        if (offers_added > 0)
        {
          LOG_PRINT(get_wallet_log_prefix(w.second.wallet_id) + "get_my_offers(): " << offers_added << " offers added from wallet " << wallet_title, LOG_LEVEL_2);
        }
        
        ++wallet_index;
      }
      break;
    }
    catch (const tools::error::file_not_found& /**/)
    {
      return API_RETURN_CODE_FILE_NOT_FOUND;
    }
    catch (const std::exception& e)
    {
      return std::string(API_RETURN_CODE_INTERNAL_ERROR) + ":" + e.what();
    }
    
  }

  size_t offers_count_before_filtering = offers.size();
  bc_services::filter_offers_list(offers, filter, m_ccore.get_blockchain_storage().get_core_runtime_config().get_core_time());
  LOG_PRINT("get_my_offers(): " << offers.size() << " offers returned (" << offers_count_before_filtering << " was before filter)", LOG_LEVEL_1);

  return API_RETURN_CODE_OK;
#else 
  return API_RETURN_CODE_FAIL;
#endif
}

std::string wallets_manager::open_wallet(const std::wstring& path, const std::string& password, uint64_t txs_to_return, view::open_wallet_response& owr, bool exclude_mining_txs)
{
  // check if that file already opened
  SHARED_CRITICAL_REGION_BEGIN(m_wallets_lock);
  for (auto& wallet_entry : m_wallets)
  {
    if (wallet_entry.second.w.unlocked_get()->get_wallet_path() == path)
      return API_RETURN_CODE_ALREADY_EXISTS;
  }
  SHARED_CRITICAL_REGION_END();


  std::shared_ptr<tools::wallet2> w(new tools::wallet2());
  w->set_use_deffered_global_outputs(m_use_deffered_global_outputs);
  owr.wallet_id = m_wallet_id_counter++;

  w->callback(std::shared_ptr<tools::i_wallet2_callback>(new i_wallet_to_i_backend_adapter(this, owr.wallet_id)));
  if (m_remote_node_mode)
  {
    w->set_core_proxy(m_rpc_proxy);
  }
  else
  {
#ifndef MOBILE_WALLET_BUILD
    w->set_core_proxy(std::shared_ptr<tools::i_core_proxy>(new tools::core_fast_rpc_proxy(m_rpc_server)));
#else 
    LOG_ERROR("Unexpected location reached");
#endif
  }

  w->set_votes_config_path(m_data_dir + "/" + CURRENCY_VOTING_CONFIG_DEFAULT_FILENAME);

  
  std::string return_code = API_RETURN_CODE_OK;
  while (true)
  {
    try
    {
      w->load(path, password);
      if (w->is_watch_only() && !w->is_auditable())
        return API_RETURN_CODE_WALLET_WATCH_ONLY_NOT_SUPPORTED;

      w->get_recent_transfers_history(owr.recent_history.history, 0, txs_to_return, owr.recent_history.total_history_items, owr.recent_history.last_item_index, exclude_mining_txs);
      //w->get_unconfirmed_transfers(owr.recent_history.unconfirmed);      
      w->get_unconfirmed_transfers(owr.recent_history.history, exclude_mining_txs);
      w->set_use_assets_whitelisting(true);
      owr.wallet_local_bc_size = w->get_blockchain_current_size();

      //workaround for missed fee
      owr.seed = w->get_account().get_seed_phrase("");
      break;
    }
    catch (const tools::error::file_not_found& /**/)
    {
      return API_RETURN_CODE_FILE_NOT_FOUND;
    }
    catch (const tools::error::file_read_error&)
    {
      return API_RETURN_CODE_INVALID_FILE;
    }
    catch (const tools::error::wallet_load_notice_wallet_restored& /**/)
    {
      return_code = API_RETURN_CODE_FILE_RESTORED;
      break;
    }
    catch (const tools::error::invalid_password& )
    {
      return std::string(API_RETURN_CODE_WRONG_PASSWORD);
    }
    catch (const std::exception& e)
    {
      return std::string(API_RETURN_CODE_INTERNAL_ERROR) + ", DESCRIPTION: " + e.what();
    }
  }
  EXCLUSIVE_CRITICAL_REGION_LOCAL(m_wallets_lock);
  wallet_vs_options& wo = m_wallets[owr.wallet_id];
  **wo.w = w;
  owr.wallet_file_size = w->get_wallet_file_size();
  get_wallet_info(wo, owr.wi);
  init_wallet_entry(wo, owr.wallet_id);
  
  return return_code;
}

bool wallets_manager::get_opened_wallets(std::list<view::open_wallet_response>& result)
{
  SHARED_CRITICAL_REGION_LOCAL(m_wallets_lock); 
  for (auto& w : m_wallets)
  {
    result.push_back(view::open_wallet_response());
    view::open_wallet_response& owr = result.back();
    owr.wallet_id = w.first;
    owr.wallet_file_size = w.second.w.unlocked_get()->get_wallet_file_size();
    owr.wallet_local_bc_size = w.second.w->get()->get_blockchain_current_size();
    std::string path = epee::string_encoding::convert_to_ansii(w.second.w.unlocked_get()->get_wallet_path());    
    owr.name = boost::filesystem::path(path).filename().string();
    owr.pass = w.second.w.unlocked_get()->get_wallet_password();
    get_wallet_info(w.second, owr.wi);
  }
  return true;
}

const po::variables_map& wallets_manager::get_arguments()
{
  return m_vm;
}

std::string wallets_manager::get_recent_transfers(size_t wallet_id, uint64_t offset, uint64_t count, view::transfers_array& tr_hist, bool exclude_mining_txs)
{
  GET_WALLET_BY_ID(wallet_id, w);
  auto wallet_locked = w.try_lock();
  if (!wallet_locked.get())
  {
    return API_RETURN_CODE_CORE_BUSY;
  }

  w->get()->get_unconfirmed_transfers(tr_hist.unconfirmed, exclude_mining_txs);
  w->get()->get_recent_transfers_history(tr_hist.history, offset, count, tr_hist.total_history_items, tr_hist.last_item_index, exclude_mining_txs);

  auto fix_tx = [](tools::wallet_public::wallet_transfer_info& wti) -> void {
    wti.show_sender = currency::is_showing_sender_addres(wti.tx);
    if (!wti.fee && !currency::is_coinbase(wti.tx))
      wti.fee = currency::get_tx_fee(wti.tx);
  };

  //workaround for missed fee
  for (auto & he : tr_hist.unconfirmed)
    fix_tx(he);

  for (auto & he : tr_hist.history)
    fix_tx(he);

  return API_RETURN_CODE_OK;
}

std::string wallets_manager::generate_wallet(const std::wstring& path, const std::string& password, view::open_wallet_response& owr)
{
  std::shared_ptr<tools::wallet2> w(new tools::wallet2());
  w->set_use_deffered_global_outputs(m_use_deffered_global_outputs);
  w->set_votes_config_path(m_data_dir + "/" + CURRENCY_VOTING_CONFIG_DEFAULT_FILENAME);
  owr.wallet_id = m_wallet_id_counter++;
  w->callback(std::shared_ptr<tools::i_wallet2_callback>(new i_wallet_to_i_backend_adapter(this, owr.wallet_id)));
  if (m_remote_node_mode)
  {
    w->set_core_proxy(m_rpc_proxy);
  }
  else
  {
#ifndef MOBILE_WALLET_BUILD
    w->set_core_proxy(std::shared_ptr<tools::i_core_proxy>(new tools::core_fast_rpc_proxy(m_rpc_server)));
#else 
    LOG_ERROR("Unexpected location reached");
#endif
  }

  try
  {
    w->generate(path, password, false);
    w->set_minimum_height(m_last_daemon_height-1);
    owr.seed = w->get_account().get_seed_phrase("");
  }
  catch (const tools::error::file_exists&)
  {
    return API_RETURN_CODE_ALREADY_EXISTS;
  }

  catch (const std::exception& e)
  {
    return std::string(API_RETURN_CODE_FAIL) + ":" + e.what();
  }
  EXCLUSIVE_CRITICAL_REGION_LOCAL(m_wallets_lock);
  wallet_vs_options& wo = m_wallets[owr.wallet_id];
  **wo.w = w;
  wo.wallet_state = view::wallet_status_info::wallet_state_ready;
  init_wallet_entry(wo, owr.wallet_id);
  get_wallet_info(wo, owr.wi);
  return API_RETURN_CODE_OK;
}

std::string wallets_manager::get_mining_estimate(uint64_t amuont_coins, 
  uint64_t time, 
  uint64_t& estimate_result, 
  uint64_t& pos_coins_and_pos_diff_rate, 
  std::vector<uint64_t>& days)
{
  if (m_remote_node_mode)
    return API_RETURN_CODE_FAIL;
#ifndef MOBILE_WALLET_BUILD
  m_ccore.get_blockchain_storage().get_pos_mining_estimate(amuont_coins, time, estimate_result, pos_coins_and_pos_diff_rate, days);
  return API_RETURN_CODE_OK;
#else 
  return API_RETURN_CODE_FAIL;
#endif 
}

std::string wallets_manager::is_pos_allowed()
{
  if (m_is_pos_allowed)
    return API_RETURN_CODE_TRUE;
  else 
    return API_RETURN_CODE_FALSE;
}
std::string wallets_manager::is_valid_brain_restore_data(const std::string& seed_phrase, const std::string& seed_password)
{
  
  currency::account_base acc;
  if (!currency::account_base::is_seed_tracking(seed_phrase))
  {
    if (acc.restore_from_seed_phrase(seed_phrase, seed_password))
      return API_RETURN_CODE_TRUE;
  }
  else
  {
    currency::account_public_address addr = AUTO_VAL_INIT(addr);
    crypto::secret_key view_sec_key = AUTO_VAL_INIT(view_sec_key);
    uint64_t ts = 0;
    if (currency::parse_tracking_seed(seed_phrase, addr, view_sec_key, ts))
      return API_RETURN_CODE_TRUE;

  }
  return API_RETURN_CODE_FALSE;
}
#ifndef MOBILE_WALLET_BUILD
void wallets_manager::subscribe_to_core_events(currency::i_core_event_handler* pevents_handler)
{

  if(!m_remote_node_mode)
    m_ccore.get_blockchain_storage().set_event_handler(pevents_handler);

}
#endif

std::string wallets_manager::get_seed_phrase_info(const std::string& seed_phrase, const std::string& seed_password, view::seed_phrase_info& result)
{
  return tools::get_seed_phrase_info(seed_phrase, seed_password, result);
}


void wallets_manager::get_gui_options(view::gui_options& opt)
{
  opt = m_ui_opt;
}
std::string wallets_manager::restore_wallet(const std::wstring& path, const std::string& password, const std::string& seed_phrase, const std::string& seed_password, view::open_wallet_response& owr)
{
  std::shared_ptr<tools::wallet2> w(new tools::wallet2());
  w->set_use_deffered_global_outputs(m_use_deffered_global_outputs);
  w->set_votes_config_path(m_data_dir + "/" + CURRENCY_VOTING_CONFIG_DEFAULT_FILENAME);
  owr.wallet_id = m_wallet_id_counter++;
  w->callback(std::shared_ptr<tools::i_wallet2_callback>(new i_wallet_to_i_backend_adapter(this, owr.wallet_id)));
  if (m_remote_node_mode)
  {
    w->set_core_proxy(m_rpc_proxy);
  }
  else
  {
#ifndef MOBILE_WALLET_BUILD
    w->set_core_proxy(std::shared_ptr<tools::i_core_proxy>(new tools::core_fast_rpc_proxy(m_rpc_server)));
#else 
    LOG_ERROR("Unexpected location reached");
#endif

  }

  currency::account_base acc;
  try
  {
    bool is_tracking = currency::account_base::is_seed_tracking(seed_phrase);
    w->restore(path, password, seed_phrase, is_tracking, seed_password);
    owr.seed = w->get_account().get_seed_phrase("");
  }
  catch (const tools::error::file_exists&)
  {
    return API_RETURN_CODE_ALREADY_EXISTS;
  }
  catch (const tools::error::wallet_wrong_seed_error&)
  {
    return API_RETURN_CODE_WRONG_SEED;
  }
  
  catch (const std::exception& e)
  {
    return std::string(API_RETURN_CODE_FAIL) + ":" + e.what();
  }
  EXCLUSIVE_CRITICAL_REGION_LOCAL(m_wallets_lock);
  wallet_vs_options& wo = m_wallets[owr.wallet_id];
  **wo.w = w;
  init_wallet_entry(wo, owr.wallet_id);
  get_wallet_info(wo, owr.wi);
  return API_RETURN_CODE_OK;
}
std::string wallets_manager::close_wallet(size_t wallet_id)
{
  EXCLUSIVE_CRITICAL_REGION_LOCAL(m_wallets_lock);
  
  auto it = m_wallets.find(wallet_id);
  if (it == m_wallets.end())
    return API_RETURN_CODE_WALLET_WRONG_ID;


  try
  {
    it->second.major_stop = true;
    it->second.stop_for_refresh = true;
    it->second.w.unlocked_get()->stop();

    it->second.w->get()->store();
    m_wallets.erase(it);
    {
      CRITICAL_REGION_LOCAL(m_wallet_log_prefixes_lock);
      m_wallet_log_prefixes[wallet_id] = std::string("[") + epee::string_tools::num_to_string_fast(wallet_id) + ":CLOSED] ";
    }
  }

  catch (const std::exception& e)
  {
    return std::string(API_RETURN_CODE_FAIL) + ":" + e.what();
  }
  //m_pview->hide_wallet();
  return API_RETURN_CODE_OK;
}

std::string wallets_manager::get_aliases(view::alias_set& al_set)
{
  if (m_remote_node_mode)
    return API_RETURN_CODE_OVERFLOW;

#ifndef MOBILE_WALLET_BUILD
  if (m_ccore.get_blockchain_storage().get_aliases_count() > API_MAX_ALIASES_COUNT)
    return API_RETURN_CODE_OVERFLOW;


  currency::COMMAND_RPC_GET_ALL_ALIASES::response aliases = AUTO_VAL_INIT(aliases);
  if (m_rpc_proxy->call_COMMAND_RPC_GET_ALL_ALIASES(aliases) && aliases.status == API_RETURN_CODE_OK)
  {
    al_set.aliases = aliases.aliases;
    return API_RETURN_CODE_OK;
  }
#endif
  return API_RETURN_CODE_FAIL;
  
}
std::string wallets_manager::get_alias_info_by_address(const std::string& addr, currency::alias_rpc_details& res_details)
{
  if (addr.empty())
    return API_RETURN_CODE_NOT_FOUND;

  currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::response res = AUTO_VAL_INIT(res);
 
  req = addr;
  bool r = m_rpc_proxy->call_COMMAND_RPC_GET_ALIASES_BY_ADDRESS(req, res);
  if (!r)
    return API_RETURN_CODE_FAIL;
  if (res.status != API_RETURN_CODE_OK)
    return res.status;

  if (res.alias_info_list.empty())
    return API_RETURN_CODE_NOT_FOUND;

  res_details = res.alias_info_list.front();
  return res.status;
}

std::string wallets_manager::get_alias_info_by_name(const std::string& name, currency::alias_rpc_details& res_details)
{
  if(name.empty())
    return API_RETURN_CODE_FILE_NOT_FOUND;

  currency::COMMAND_RPC_GET_ALIAS_DETAILS::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_ALIAS_DETAILS::response res = AUTO_VAL_INIT(res);

  req.alias = name;

  bool r = m_rpc_proxy->call_COMMAND_RPC_GET_ALIAS_DETAILS(req, res);
  if (!r)
    return API_RETURN_CODE_FAIL;

  if (res.status == API_RETURN_CODE_NOT_FOUND)
    return API_RETURN_CODE_NOT_FOUND;

  res_details.alias = name;
  res_details.details = res.alias_details;
  return res.status;
}

std::string wallets_manager::get_alias_coast(const std::string& a, uint64_t& coast)
{
  currency::COMMAND_RPC_GET_ALIAS_REWARD::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_ALIAS_REWARD::response rsp = AUTO_VAL_INIT(rsp);
  req.alias = a;
  if (!m_rpc_proxy->call_COMMAND_RPC_GET_ALIAS_REWARD(req, rsp))
    return API_RETURN_CODE_BAD_ARG;

  coast = rsp.reward;
  return rsp.status;
}

std::string wallets_manager::request_alias_registration(const currency::alias_rpc_details& al, uint64_t wallet_id, uint64_t fee, currency::transaction& res_tx, uint64_t reward)
{
  currency::extra_alias_entry ai = AUTO_VAL_INIT(ai);
  if (!currency::alias_rpc_details_to_alias_info(al, ai))
    return API_RETURN_CODE_BAD_ARG;

  if (!currency::validate_alias_name(ai.m_alias))
  {
    return API_RETURN_CODE_BAD_ARG;
  }

  currency::COMMAND_RPC_GET_ALIAS_DETAILS::request req = AUTO_VAL_INIT(req);
  req.alias = ai.m_alias;
  currency::COMMAND_RPC_GET_ALIAS_DETAILS::response rsp = AUTO_VAL_INIT(rsp);
  if (m_rpc_proxy->call_COMMAND_RPC_GET_ALIAS_DETAILS(req, rsp) && rsp.status == API_RETURN_CODE_NOT_FOUND)
  {
    GET_WALLET_BY_ID(wallet_id, w);

    std::string api_return_code_result = API_RETURN_CODE_FAIL;
    do_exception_safe_call(
      [&]() {
        w->get()->request_alias_registration(ai, res_tx, fee, reward);
        api_return_code_result = API_RETURN_CODE_OK;
      },
      [&]() { return get_wallet_log_prefix(wallet_id) + "request_alias_registration error: "; },
      api_return_code_result
    );
    return api_return_code_result;
  }

  return API_RETURN_CODE_ALREADY_EXISTS;
}

std::string wallets_manager::request_alias_update(const currency::alias_rpc_details& al, uint64_t wallet_id, uint64_t fee, currency::transaction& res_tx, uint64_t reward)
{
  currency::extra_alias_entry ai = AUTO_VAL_INIT(ai);
  if (!currency::alias_rpc_details_to_alias_info(al, ai))
    return API_RETURN_CODE_BAD_ARG;

  if (!currency::validate_alias_name(ai.m_alias))
  {
    return API_RETURN_CODE_BAD_ARG;
  }

  currency::COMMAND_RPC_GET_ALIAS_DETAILS::request req;
  req.alias = ai.m_alias;
  currency::COMMAND_RPC_GET_ALIAS_DETAILS::response rsp = AUTO_VAL_INIT(rsp);
  if (m_rpc_proxy->call_COMMAND_RPC_GET_ALIAS_DETAILS(req, rsp) && rsp.status == API_RETURN_CODE_OK)
  {
    GET_WALLET_BY_ID(wallet_id, w);

    std::string api_return_code_result = API_RETURN_CODE_FAIL;
    do_exception_safe_call(
      [&]() {
        w->get()->request_alias_update(ai, res_tx, fee, reward);
        api_return_code_result = API_RETURN_CODE_OK;
      },
      [&]() { return get_wallet_log_prefix(wallet_id) + "request_alias_update error: "; },
      api_return_code_result
      );
    return api_return_code_result;
  }

  return API_RETURN_CODE_FILE_NOT_FOUND;
}


std::string wallets_manager::transfer(uint64_t wallet_id, const view::transfer_params& tp, currency::transaction& res_tx)
{

  std::vector<currency::tx_destination_entry> dsts;
  if(!tp.destinations.size())
    return API_RETURN_CODE_BAD_ARG_EMPTY_DESTINATIONS;

  uint64_t fee = tp.fee;
  //payment_id
  std::vector<currency::attachment_v> attachments;
  std::vector<currency::extra_v> extra;
  bool wrap = false;
//  if (!currency::parse_amount(fee, tp.fee))
//    return API_RETURN_CODE_BAD_ARG_WRONG_FEE;

  std::string payment_id = tp.payment_id;
  for(auto& d: tp.destinations)
  {
    dsts.push_back(currency::tx_destination_entry());
    dsts.back().addr.resize(1);
    currency::tx_destination_entry& de = dsts.back();
    std::string embedded_payment_id;
    if (currency::is_address_like_wrapped(d.address))
    {
      CHECK_AND_ASSERT_MES(!wrap, API_RETURN_CODE_BAD_ARG, "Second wrap entry in one tx not allowed");
      LOG_PRINT_L0("Address " << d.address << " recognized as wrapped address, creating wrapping transaction...");
      //put into service attachment specially encrypted entry which will contain wrap address and network
      currency::tx_service_attachment sa = AUTO_VAL_INIT(sa);
      sa.service_id = BC_WRAP_SERVICE_ID;
      sa.instruction = BC_WRAP_SERVICE_INSTRUCTION_ERC20;
      sa.flags = TX_SERVICE_ATTACHMENT_ENCRYPT_BODY | TX_SERVICE_ATTACHMENT_ENCRYPT_BODY_ISOLATE_AUDITABLE;
      sa.body = d.address;
      extra.push_back(sa);


      currency::account_public_address acc = AUTO_VAL_INIT(acc);
      currency::get_account_address_from_str(acc, BC_WRAP_SERVICE_CUSTODY_WALLET);
      de.addr.front() = acc;
      wrap = true;
    }
    else if (!tools::get_transfer_address(d.address, dsts.back().addr.back(), embedded_payment_id, m_rpc_proxy.get()))
    {
      return API_RETURN_CODE_BAD_ARG_INVALID_ADDRESS;
    }
    
    
    if(!currency::parse_amount(dsts.back().amount, d.amount))
    {
      return API_RETURN_CODE_BAD_ARG_WRONG_AMOUNT;
    }
    if (embedded_payment_id.size() != 0)
    {
      if (payment_id.size() != 0)
        return API_RETURN_CODE_BAD_ARG_WRONG_PAYMENT_ID; // payment id is specified more than once
      payment_id = embedded_payment_id;
    }
    dsts.back().asset_id = d.asset_id;
  }

  GET_WALLET_BY_ID(wallet_id, w);

  if (payment_id.size())
  {
    if (!currency::is_payment_id_size_ok(payment_id))
      return API_RETURN_CODE_BAD_ARG_WRONG_PAYMENT_ID; // payment id is too big

    if (!currency::set_payment_id_to_tx(attachments, payment_id, w->get()->is_in_hardfork_zone(ZANO_HARDFORK_04_ZARCANUM)))
      return API_RETURN_CODE_INTERNAL_ERROR;
  }


  //set transaction unlock time if it was specified by user 
  uint64_t unlock_time = 0;
  if (tp.lock_time)
  {
    if (tp.lock_time > CURRENCY_MAX_BLOCK_NUMBER)
      unlock_time = tp.lock_time;
    else
      unlock_time = w->get()->get_blockchain_current_size() + tp.lock_time;
  }
      
    
  //process attachments
  if (tp.comment.size())
  {
    currency::tx_comment tc = AUTO_VAL_INIT(tc);
    tc.comment = tp.comment;
    extra.push_back(tc);
  }
  if (tp.push_payer && !wrap)
  {
    currency::create_and_add_tx_payer_to_container_from_address(extra, w->get()->get_account().get_keys().account_address,  w->get()->get_top_block_height(),  w->get()->get_core_runtime_config());
  }    
  if (!tp.hide_receiver)
  {
    for (auto& d : dsts)
    {
      for (auto& a : d.addr)
        currency::create_and_add_tx_receiver_to_container_from_address(extra, a, w->get()->get_top_block_height(),  w->get()->get_core_runtime_config());
    }
  }


  std::string api_return_code_result = API_RETURN_CODE_FAIL;
  do_exception_safe_call(
    [&]() {
      w->get()->transfer(dsts, tp.mixin_count, unlock_time ? unlock_time + 1 : 0, fee, extra, attachments, res_tx);
      api_return_code_result = API_RETURN_CODE_OK;
    },
    [&]() { return get_wallet_log_prefix(wallet_id) + "Transfer error: "; },
    api_return_code_result
    );
  return api_return_code_result;
}

bool wallets_manager::get_is_remote_daemon_connected()
{
  if (!m_remote_node_mode)
    return true;
  if (m_pproxy_diganostic_info->last_daemon_is_disconnected)
    return false;
  if (m_pproxy_diganostic_info->is_busy)
    return false;
  if (time(nullptr) - m_rpc_proxy->get_last_success_interract_time() > DAEMON_IDLE_UPDATE_TIME_MS * 2)
    return false;
  return true;
}

std::string wallets_manager::get_connectivity_status()
{
  view::general_connectivity_info gci = AUTO_VAL_INIT(gci);
  gci.is_online = get_is_remote_daemon_connected();
  gci.last_daemon_is_disconnected = m_pproxy_diganostic_info->last_daemon_is_disconnected;
  gci.is_server_busy = m_pproxy_diganostic_info->is_busy;
  gci.last_proxy_communicate_timestamp = m_rpc_proxy->get_last_success_interract_time();
  return epee::serialization::store_t_to_json(gci);
}

std::string wallets_manager::get_wallet_status(uint64_t wallet_id)
{
  GET_WALLET_OPT_BY_ID(wallet_id, wo);
  view::wallet_sync_status_info wsi = AUTO_VAL_INIT(wsi);
  wsi.is_in_long_refresh = wo.long_refresh_in_progress;
  wsi.is_daemon_connected = get_is_remote_daemon_connected();
  wsi.progress = wo.w.unlocked_get().get()->get_sync_progress();
  wsi.wallet_state = wo.wallet_state;
  wsi.current_daemon_height = m_last_daemon_height;
  wsi.current_wallet_height = wo.w.unlocked_get().get()->get_top_block_height();
  return epee::serialization::store_t_to_json(wsi);
}

std::string wallets_manager::invoke(uint64_t wallet_id, std::string params)
{
  GET_WALLET_OPT_BY_ID(wallet_id, wo);

  CRITICAL_REGION_LOCAL1(wo.long_refresh_in_progress_lock);
  if (wo.long_refresh_in_progress)
  {
    epee::json_rpc::response<epee::json_rpc::dummy_result, epee::json_rpc::error> error_response = AUTO_VAL_INIT(error_response);
    error_response.error.code = -1;
    error_response.error.message = API_RETURN_CODE_BUSY;
    return epee::serialization::store_t_to_json(error_response);
  }


  auto locker_object = wo.w.lock();

  epee::net_utils::http::http_request_info query_info = AUTO_VAL_INIT(query_info);
  epee::net_utils::http::http_response_info response_info = AUTO_VAL_INIT(response_info);
  epee::net_utils::connection_context_base stub_conn_context = AUTO_VAL_INIT(stub_conn_context);
  bool call_found = false;
  query_info.m_URI = "/json_rpc";
  query_info.m_body = params;
  wo.rpc_wrapper->handle_http_request_map(query_info, response_info, stub_conn_context, call_found);
  return response_info.m_body;
}

std::string wallets_manager::get_wallet_info(uint64_t wallet_id, view::wallet_info& wi)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);
  return get_wallet_info(w, wi);
}

std::string wallets_manager::get_wallet_info_extra(uint64_t wallet_id, view::wallet_info_extra& wi)
{
  GET_WALLET_OPT_BY_ID(wallet_id, wo);
  
  auto locker_object = wo.w.lock();
  tools::wallet2& rw = *(*(*locker_object)); //this looks a bit crazy, i know
  
  auto& keys = rw.get_account().get_keys();

  wi.view_private_key = epee::string_tools::pod_to_hex(keys.view_secret_key);
  wi.view_public_key  = epee::string_tools::pod_to_hex(keys.account_address.view_public_key);
  wi.spend_private_key = epee::string_tools::pod_to_hex(keys.spend_secret_key);
  wi.spend_public_key  = epee::string_tools::pod_to_hex(keys.account_address.spend_public_key);
  wi.seed = rw.get_account().get_seed_phrase("");
  return API_RETURN_CODE_OK;
}

std::string wallets_manager::get_contracts(size_t wallet_id, std::vector<tools::wallet_public::escrow_contract_details>& contracts)
{
  tools::escrow_contracts_container cc;
  GET_WALLET_OPT_BY_ID(wallet_id, w);
  try
  {
    w.w->get()->get_contracts(cc);
  }
  catch (...)
  {
    return API_RETURN_CODE_FAIL;
  }

  contracts.resize(cc.size());
  size_t i = 0;
  for (auto& c: cc)
  {
    static_cast<tools::wallet_public::escrow_contract_details_basic&>(contracts[i]) = c.second;
    contracts[i].contract_id = c.first; 
    i++;
  }

  return API_RETURN_CODE_OK;
}
std::string wallets_manager::create_proposal(const view::create_proposal_param_gui& cpp)
{
  //tools::escrow_contracts_container cc;
  GET_WALLET_OPT_BY_ID(cpp.wallet_id, w);
  currency::transaction tx = AUTO_VAL_INIT(tx);
  currency::transaction template_tx = AUTO_VAL_INIT(template_tx);

  std::string api_return_code_result = API_RETURN_CODE_FAIL;
  do_exception_safe_call(
    [&]() {
      w.w->get()->send_escrow_proposal(cpp, tx, template_tx);
      api_return_code_result = API_RETURN_CODE_OK;
    },
    [&]() { return get_wallet_log_prefix(cpp.wallet_id) + "send_escrow_proposal error: "; },
    api_return_code_result
    );
  return api_return_code_result;
}

std::string wallets_manager::accept_proposal(size_t wallet_id, const crypto::hash& contract_id)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);

  std::string api_return_code_result = API_RETURN_CODE_FAIL;
  do_exception_safe_call(
    [&]() {
      w.w->get()->accept_proposal(contract_id, TX_DEFAULT_FEE);
      api_return_code_result = API_RETURN_CODE_OK;
    },
    [&]() { return get_wallet_log_prefix(wallet_id) + "accept_proposal error: "; },
    api_return_code_result
    );
  return api_return_code_result;
}

std::string wallets_manager::release_contract(size_t wallet_id, const crypto::hash& contract_id, const std::string& contract_over_type)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);

  std::string api_return_code_result = API_RETURN_CODE_FAIL;
  do_exception_safe_call(
    [&]() {
      w.w->get()->finish_contract(contract_id, contract_over_type);
      api_return_code_result = API_RETURN_CODE_OK;
    },
    [&]() { return get_wallet_log_prefix(wallet_id) + "release_contract error: "; },
    api_return_code_result
    );
  return api_return_code_result;
}

std::string wallets_manager::request_cancel_contract(size_t wallet_id, const crypto::hash& contract_id, uint64_t fee, uint64_t expiration_period)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);

  std::string api_return_code_result = API_RETURN_CODE_FAIL;
  do_exception_safe_call(
    [&]() {
      w.w->get()->request_cancel_contract(contract_id, fee, expiration_period);
      api_return_code_result = API_RETURN_CODE_OK;
    },
    [&]() { return get_wallet_log_prefix(wallet_id) + "request_cancel_contract error: "; },
    api_return_code_result
    );
  return api_return_code_result;
}

std::string wallets_manager::accept_cancel_contract(size_t wallet_id, const crypto::hash& contract_id)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);

  std::string api_return_code_result = API_RETURN_CODE_FAIL;
  do_exception_safe_call(
    [&]() {
      TIME_MEASURE_START_MS(timing1);
      w.w->get()->accept_cancel_contract(contract_id);
      TIME_MEASURE_FINISH_MS(timing1);
      if (timing1 > 500)
        LOG_PRINT_RED_L0(get_wallet_log_prefix(wallet_id) + "[daemon_backend::accept_cancel_contract] LOW PERFORMANCE: " << timing1);
      api_return_code_result = API_RETURN_CODE_OK;
    },
    [&]() { return get_wallet_log_prefix(wallet_id) + "accept_cancel_contract error: "; },
    api_return_code_result
  );
  return api_return_code_result;
}

std::string wallets_manager::backup_wallet(uint64_t wallet_id, const std::wstring& path)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);
  try
  {
    w.w->get()->store(path);
  }
  catch (...)
  {
    return API_RETURN_CODE_FAIL;
  }
  return API_RETURN_CODE_OK;
}
std::string wallets_manager::reset_wallet_password(uint64_t wallet_id, const std::string& pass)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);
  if (w.w->get()->reset_password(pass))
    return API_RETURN_CODE_OK;
  else
    return API_RETURN_CODE_FAIL;
}
std::string wallets_manager::use_whitelisting(uint64_t wallet_id, bool use)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);  
  w.w->get()->set_use_assets_whitelisting(use);
  return API_RETURN_CODE_OK;
}
std::string wallets_manager::add_custom_asset_id(uint64_t wallet_id, const crypto::public_key& asset_id, currency::asset_descriptor_base& asset_descriptor)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);
  if(w.w->get()->add_custom_asset_id(asset_id, asset_descriptor))
    return API_RETURN_CODE_OK;
  else
    return API_RETURN_CODE_FAIL;
}
std::string wallets_manager::delete_custom_asset_id(uint64_t wallet_id, const crypto::public_key& asset_id)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);
  if (w.w->get()->delete_custom_asset_id(asset_id))
    return API_RETURN_CODE_OK;
  else
    return API_RETURN_CODE_FAIL;

}
std::string wallets_manager::is_wallet_password_valid(uint64_t wallet_id, const std::string& pass)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);
  if (w.w->get()->is_password_valid(pass))
    return API_RETURN_CODE_OK;
  else
    return API_RETURN_CODE_FAIL;
}
std::string wallets_manager::resync_wallet(uint64_t wallet_id)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);
  w.w->get()->reset_history();
  w.last_wallet_synch_height = 0;
  return API_RETURN_CODE_OK;
}
std::string wallets_manager::start_pos_mining(uint64_t wallet_id)
{
  GET_WALLET_OPT_BY_ID(wallet_id, wo);
  wo.do_mining = true;
  wo.need_to_update_wallet_info = true;
  return API_RETURN_CODE_OK;
}
std::string wallets_manager::get_mining_history(uint64_t wallet_id, tools::wallet_public::mining_history& mh)
{
  GET_WALLET_OPT_BY_ID(wallet_id, wo);

  if (wo.wallet_state != view::wallet_status_info::wallet_state_ready)
    return API_RETURN_CODE_CORE_BUSY;

  wo.w->get()->get_mining_history(mh);
  return API_RETURN_CODE_OK;
}
std::string wallets_manager::get_wallet_restore_info(uint64_t wallet_id, std::string& seed_phrase, const std::string& seed_password)
{
  GET_WALLET_OPT_BY_ID(wallet_id, wo);

  if (wo.wallet_state != view::wallet_status_info::wallet_state_ready || wo.long_refresh_in_progress)
    return API_RETURN_CODE_CORE_BUSY;

  seed_phrase = wo.w->get()->get_account().get_seed_phrase(seed_password);

  return API_RETURN_CODE_OK;
}
void wallets_manager::prepare_wallet_status_info(wallet_vs_options& wo, view::wallet_status_info& wsi)
{
  wsi.is_mining = wo.do_mining;
  wsi.wallet_id = wo.wallet_id;
  wsi.is_alias_operations_available = !wo.has_related_alias_in_unconfirmed;
  wo.w->get()->balance(wsi.balances, wsi.minied_total);
  wsi.has_bare_unspent_outputs = wo.w->get()->has_bare_unspent_outputs();
}
std::string wallets_manager::check_available_sources(uint64_t wallet_id, std::list<uint64_t>& amounts)
{
  GET_WALLET_OPT_BY_ID(wallet_id, wo);
  if (wo.w->get()->check_available_sources(amounts))
    return API_RETURN_CODE_TRUE;
  else
    return API_RETURN_CODE_FALSE;
}
std::string wallets_manager::stop_pos_mining(uint64_t wallet_id)
{
  GET_WALLET_OPT_BY_ID(wallet_id, wo);
  wo.do_mining = false;
  wo.need_to_update_wallet_info = true;
  return API_RETURN_CODE_OK;
}
std::string wallets_manager::run_wallet(uint64_t wallet_id)
{
  GET_WALLET_OPT_BY_ID(wallet_id, wo);
  wo.miner_thread = std::thread(boost::bind(&wallets_manager::wallet_vs_options::worker_func, &wo));
  return API_RETURN_CODE_OK;
}

std::string wallets_manager::get_wallet_info(wallet_vs_options& wo, view::wallet_info& wi)
{
  auto locker_object = wo.w.lock();
  tools::wallet2& rw = *(*(*locker_object)); //this looks a bit crazy, i know
  tools::get_wallet_info(rw, wi);
  return API_RETURN_CODE_OK;
}

std::string wallets_manager::push_offer(size_t wallet_id, const bc_services::offer_details_ex& od, currency::transaction& res_tx)
{
  GET_WALLET_BY_ID(wallet_id, w);

  std::string api_return_code_result = API_RETURN_CODE_FAIL;
  do_exception_safe_call(
    [&]() {
      w->get()->push_offer(od, res_tx);
      api_return_code_result = API_RETURN_CODE_OK;
    },
    [&]() { return get_wallet_log_prefix(wallet_id) + "push_offer error: "; },
    api_return_code_result
  );
  return api_return_code_result;
}

std::string wallets_manager::cancel_offer(const view::cancel_offer_param& co, currency::transaction& res_tx)
{
  GET_WALLET_BY_ID(co.wallet_id, w);

  std::string api_return_code_result = API_RETURN_CODE_FAIL;
  do_exception_safe_call(
    [&]() {
      w->get()->cancel_offer_by_id(co.tx_id, co.no, TX_DEFAULT_FEE, res_tx);
      api_return_code_result = API_RETURN_CODE_OK;
    },
    [&]() { return get_wallet_log_prefix(co.wallet_id) + "cancel_offer error: "; },
    api_return_code_result
  );
  return api_return_code_result;
}

std::string wallets_manager::push_update_offer(const bc_services::update_offer_details& uo, currency::transaction& res_tx)
{
  GET_WALLET_BY_ID(uo.wallet_id, w);

  std::string api_return_code_result = API_RETURN_CODE_FAIL;
  do_exception_safe_call(
    [&]() {
      w->get()->update_offer_by_id(uo.tx_id, uo.no, uo.od, res_tx);
      api_return_code_result = API_RETURN_CODE_OK;
    },
    [&]() { return get_wallet_log_prefix(uo.wallet_id) + "push_update_offer error: "; },
    api_return_code_result
  );
  return api_return_code_result;
}


std::string wallets_manager::get_offers_ex(const bc_services::core_offers_filter& cof, std::list<bc_services::offer_details_ex>& offers, uint64_t& total_count)
{
  if (m_remote_node_mode)
    return API_RETURN_CODE_FAIL;  
#ifndef MOBILE_WALLET_BUILD
  //TODO: make it proxy-like call
  //m_ccore.get_blockchain_storage().get_offers_ex(cof, offers, total_count);
	m_offers_service.get_offers_ex(cof, offers, total_count, m_ccore.get_blockchain_storage().get_core_runtime_config().get_core_time());
  return API_RETURN_CODE_OK;
#else 
  return API_RETURN_CODE_FAIL;
#endif
}


std::string wallets_manager::validate_address(const std::string& addr_str, std::string& payment_id)
{
  currency::account_public_address acc = AUTO_VAL_INIT(acc);
  if (currency::is_address_like_wrapped(addr_str))
  {
    return API_RETURN_CODE_WRAP;
  }
  else if (currency::get_account_address_and_payment_id_from_str(acc, payment_id, addr_str))
    return API_RETURN_CODE_TRUE;
  else
    return API_RETURN_CODE_FALSE;
}

void wallets_manager::on_new_block(size_t wallet_id, uint64_t /*height*/, const currency::block& /*block*/)
{

}

void wallets_manager::on_transfer2(size_t wallet_id, const tools::wallet_public::wallet_transfer_info& wti, const std::list<tools::wallet_public::asset_balance_entry>& balances, uint64_t total_mined)
{  
  view::transfer_event_info tei = AUTO_VAL_INIT(tei);
  tei.ti = wti;
  tei.balances = balances;
  tei.total_mined = total_mined;
  tei.wallet_id = wallet_id;

  GET_WALLET_OPTIONS_BY_ID_VOID_RET(wallet_id, w);
  tei.is_wallet_in_sync_process = w.long_refresh_in_progress;
  if (!(w.w->get()->is_watch_only()))
  {
    m_pview->money_transfer(tei);
  }
}
void wallets_manager::on_pos_block_found(size_t wallet_id, const currency::block& b)
{
  m_pview->pos_block_found(b);
}
bool wallets_manager::set_use_tor(bool use_tor)
{
  m_use_tor = use_tor;
  SHARED_CRITICAL_REGION_LOCAL(m_wallets_lock);
  for (auto& w : m_wallets)
  {
    w.second.w->get()->set_disable_tor_relay(!m_use_tor);
  }
  return true;
}

void wallets_manager::on_sync_progress(size_t wallet_id, const uint64_t& percents)
{
  // do not lock m_wallets_lock down the callstack! It will lead to a deadlock, because wallet locked_object is aready locked
  // and other threads are usually locks m_wallets_lock before locking wallet's locked_object

  view::wallet_sync_progres_param wspp = AUTO_VAL_INIT(wspp);
  wspp.progress = percents;
  wspp.wallet_id = wallet_id;
  m_pview->wallet_sync_progress(wspp);
}
void wallets_manager::on_transfer_canceled(size_t wallet_id, const tools::wallet_public::wallet_transfer_info& wti)
{
  view::transfer_event_info tei = AUTO_VAL_INIT(tei);
  tei.ti = wti;

  SHARED_CRITICAL_REGION_LOCAL(m_wallets_lock);
  auto it = m_wallets.find(wallet_id);
  if (it == m_wallets.end())
  {
    LOG_ERROR(get_wallet_log_prefix(wallet_id) + "on_transfer_canceled() wallet with id = " << wallet_id << " not found");
    return;
  }
  auto& w = it->second.w;
  if (w->get() != nullptr)
  {
    w->get()->balance(tei.balances, tei.total_mined);
    tei.wallet_id = wallet_id;
  }
  else
  {
    LOG_ERROR(get_wallet_log_prefix(wallet_id) + "on_transfer_canceled() wallet with id = " << wallet_id << "  has nullptr");
  }
  m_pview->money_transfer_cancel(tei);
}

void wallets_manager::on_tor_status_change(size_t wallet_id, const std::string& state)
{
  view::current_action_status tsu = { wallet_id , state };
  m_pview->update_tor_status(tsu);
}

void wallets_manager::on_mw_get_wallets(std::vector<tools::wallet_public::wallet_entry_info>& wallets)
{
  std::list<view::open_wallet_response> opened_wallets;
  this->get_opened_wallets(opened_wallets);
  wallets.resize(opened_wallets.size());
  size_t i = 0;
  for (const auto& item : opened_wallets)
  {
    wallets[i].wi = item.wi;
    wallets[i].wallet_id = item.wallet_id;
    i++;
  }
}
bool wallets_manager::on_mw_select_wallet(uint64_t wallet_id)
{
  SHARED_CRITICAL_REGION_LOCAL(m_wallets_lock);    
  auto it = m_wallets.find(wallet_id);      
  if (it == m_wallets.end())                
    return false; 
 
#ifndef MOBILE_WALLET_BUILD
  m_rpc_selected_wallet_id = wallet_id;
#endif
  return true;
}


bool wallets_manager::on_mw_get_wallets(const tools::wallet_public::COMMAND_MW_GET_WALLETS::request& req, tools::wallet_public::COMMAND_MW_GET_WALLETS::response& res, epee::json_rpc::error& er, epee::net_utils::connection_context_base& cntx)
{
  this->on_mw_get_wallets(res.wallets);
  return true;
}
bool wallets_manager::on_mw_select_wallet(const tools::wallet_public::COMMAND_MW_SELECT_WALLET::request& req, tools::wallet_public::COMMAND_MW_SELECT_WALLET::response& res, epee::json_rpc::error& er, epee::net_utils::connection_context_base& cntx)
{
  this->on_mw_select_wallet(req.wallet_id);
  res.status = API_RETURN_CODE_OK;
  return true;
}

void wallets_manager::lock() 
{
#ifndef MOBILE_WALLET_BUILD
  m_select_wallet_rpc_lock.lock();
  {
    SHARED_CRITICAL_REGION_LOCAL(m_wallets_lock);
    auto it = m_wallets.find(m_rpc_selected_wallet_id);
    if (it == m_wallets.end())
    {
      throw std::runtime_error("Wallet not selected");
    }
    m_current_wallet_locked_object = it->second.w.lock();
  }
#endif
}

void wallets_manager::unlock() 
{
#ifndef MOBILE_WALLET_BUILD
  m_current_wallet_locked_object.reset();
  m_select_wallet_rpc_lock.unlock();
#endif
}
std::shared_ptr<tools::wallet2> wallets_manager::get_wallet()
{
#ifndef MOBILE_WALLET_BUILD
  if (!m_current_wallet_locked_object.get())
  {
    throw std::runtime_error("Wallet is not locked for get_wallet() call");
  }
  return **m_current_wallet_locked_object;
#else
  std::runtime_error("Unexpected call: std::shared_ptr<tools::wallet2> wallets_manager::get_wallet()");
  return std::shared_ptr<tools::wallet2>();
#endif
}

void wallets_manager::wallet_vs_options::worker_func()
{
  LOG_PRINT_GREEN("[WALLET_HANDLER] Wallet handler thread started, addr: " << w->get()->get_account().get_public_address_str(), LOG_LEVEL_0);
  epee::math_helper::once_a_time_seconds<TX_POOL_SCAN_INTERVAL> scan_pool_interval;
  epee::math_helper::once_a_time_seconds<2> pos_minin_interval;
  view::wallet_status_info wsi = AUTO_VAL_INIT(wsi);
  while (!major_stop)
  {
    stop_for_refresh = false;
    try
    {
      wsi.wallet_state = view::wallet_status_info::wallet_state_ready;
      if (m_pproxy_diagnostig_info->last_daemon_is_disconnected.load())
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        continue;
      }

      //******************************************************************************************
      //sync zone
      if (*plast_daemon_network_state == currency::COMMAND_RPC_GET_INFO::daemon_network_state_online)
      {
        if (*plast_daemon_height != last_wallet_synch_height)
        {
          wsi.is_mining = do_mining;
          bool show_progress = true;
          if (last_wallet_synch_height && *plast_daemon_height - last_wallet_synch_height < 3)
            show_progress = false;

          if(*plast_daemon_height - last_wallet_synch_height > 10)
          {
            CRITICAL_REGION_LOCAL(long_refresh_in_progress_lock);
            long_refresh_in_progress = true;
          }


          if (show_progress)
          {
            wallet_state = wsi.wallet_state = view::wallet_status_info::wallet_state_synchronizing;
            prepare_wallet_status_info(*this, wsi);
            pview->update_wallet_status(wsi);
          }
          w->get()->refresh(stop_for_refresh);
          long_refresh_in_progress = false;
          w->get()->resend_unconfirmed();
          {
            auto w_ptr = *w; // get locked exclusive access to the wallet first (it's more likely that wallet is locked for a long time than 'offers')
            auto offers_list_proxy = *offers; // than get locked exclusive access to offers
            offers_list_proxy->clear();
            (*w_ptr)->get_actual_offers(*offers_list_proxy);
          }

          wallet_state = wsi.wallet_state = view::wallet_status_info::wallet_state_ready;
          prepare_wallet_status_info(*this, wsi);
          pview->update_wallet_status(wsi);
          //do refresh
          last_wallet_synch_height = static_cast<uint64_t>(*plast_daemon_height);
        }

        scan_pool_interval.do_call([&](){
          bool has_related_alias = false;
          w->get()->scan_tx_pool(has_related_alias);
          if (has_related_alias_in_unconfirmed != has_related_alias)
          {
            // notify gui about alias operations locked for this wallet
            has_related_alias_in_unconfirmed = has_related_alias;
            wsi.wallet_state = view::wallet_status_info::wallet_state_ready;
            prepare_wallet_status_info(*this, wsi);
            pview->update_wallet_status(wsi);
          }

          return true;
        });
      }

      if (major_stop || stop_for_refresh)
        break;
      //******************************************************************************************
      //mining zone
      if (do_mining && *plast_daemon_network_state == currency::COMMAND_RPC_GET_INFO::daemon_network_state_online)
      {
        pos_minin_interval.do_call([this](){
          tools::wallet2::mining_context ctx = AUTO_VAL_INIT(ctx);
          LOG_PRINT_L1(get_log_prefix() + " Starting PoS mint iteration");
          if (!w->get()->fill_mining_context(ctx))
          {
            LOG_PRINT_L1(get_log_prefix() + " cannot obtain PoS mining context, skip iteration");
            return true;
          }

          //uint64_t pos_entries_amount = 0;
          //for (auto& ent : ctx.sp.pos_entries)
          //  pos_entries_amount += ent.amount;

          w->get()->scan_pos(ctx, break_mining_loop, [this](){
            return *plast_daemon_network_state == currency::COMMAND_RPC_GET_INFO::daemon_network_state_online &&  *plast_daemon_height == last_wallet_synch_height;
          }, core_conf);

          if (ctx.status == API_RETURN_CODE_OK)
          {
            w->get()->build_minted_block(ctx);
          }
          LOG_PRINT_L1(get_log_prefix() << " PoS mining iteration finished, status: " << ctx.status << ", used " << ctx.total_items_checked << " entries with total amount: " << currency::print_money_brief(ctx.total_amount_checked) << ", processed: " << ctx.iterations_processed << " iter.");
          return true;
        });
      }
    }
    catch (const tools::error::daemon_busy& /*e*/)
    {
      boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
      continue;
    }

    catch (const tools::error::no_connection_to_daemon& /*e*/)
    {
      boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
      continue;
    }

    catch (const std::exception& e)
    {
      LOG_ERROR(w->get()->get_log_prefix() + " Exception in wallet handler: " << e.what());
      wallet_state = wsi.wallet_state = view::wallet_status_info::wallet_state_error;
      pview->update_wallet_status(wsi);
      continue;
    }

    catch (...)
    {
      LOG_ERROR(w->get()->get_log_prefix() + " Exception in wallet handler: unknownk exception");
      wallet_state = wsi.wallet_state = view::wallet_status_info::wallet_state_error;
      pview->update_wallet_status(wsi);
      continue;
    }
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
  }
  LOG_PRINT_GREEN("[WALLET_HANDLER] Wallet thread thread stopped", LOG_LEVEL_0);
}
void wallets_manager::wallet_vs_options::stop(bool wait)
{
  w.unlocked_get()->stop();
  do_mining = false;
  major_stop = true;
  stop_for_refresh = true;
  break_mining_loop = true;
  if (wait)
  {
    if (miner_thread.joinable())
      miner_thread.join();
  }
}
wallets_manager::wallet_vs_options::~wallet_vs_options()
{
  stop();
}

std::string wallets_manager::get_wallet_log_prefix(size_t wallet_id) const
{
  CRITICAL_REGION_LOCAL(m_wallet_log_prefixes_lock);

  CHECK_AND_ASSERT_MES(wallet_id < m_wallet_log_prefixes.size(), std::string("[") + epee::string_tools::num_to_string_fast(wallet_id) + ":???] ", "wallet prefix is not found for id " << wallet_id);
  return m_wallet_log_prefixes[wallet_id];
}


//
// wallet_lock_time_watching_policy
//

void wallet_lock_time_watching_policy::watch_lock_time(uint64_t lock_time)
{
  if (lock_time > 500)
  {
    LOG_PRINT_RED_L0("[wallet_lock_time_watching_policy::watch_lock_time] LOCK_TIME: " << lock_time);
  }
}

