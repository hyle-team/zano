// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "daemon_backend.h"
#include "currency_core/alias_helper.h"
#include "core_fast_rpc_proxy.h"
#include "string_coding.h"
#include "currency_core/core_tools.h"
#include "common/callstack_helper.h"
#include "wallet/wallet_helpers.h"

#define GET_WALLET_OPT_BY_ID(wallet_id, name) \
  CRITICAL_REGION_LOCAL(m_wallets_lock);    \
  auto it = m_wallets.find(wallet_id);      \
  if (it == m_wallets.end())                \
    return API_RETURN_CODE_WALLET_WRONG_ID; \
  auto& name = it->second;

#define GET_WALLET_BY_ID(wallet_id, name)       \
CRITICAL_REGION_LOCAL(m_wallets_lock);    \
auto it = m_wallets.find(wallet_id);      \
if (it == m_wallets.end())                \
  return API_RETURN_CODE_WALLET_WRONG_ID; \
auto& name = it->second.w;



daemon_backend::daemon_backend():m_pview(&m_view_stub),
                                 m_stop_singal_sent(false),
                                 m_ccore(&m_cprotocol),
                                 m_cprotocol(m_ccore, &m_p2psrv),
                                 m_p2psrv(m_cprotocol),
                                 m_rpc_server(m_ccore, m_p2psrv, m_offers_service),
                                 m_rpc_proxy(new tools::core_fast_rpc_proxy(m_rpc_server)),
                                 m_last_daemon_height(0),
                                 m_last_daemon_is_disconnected(false),
                                 m_wallet_id_counter(0),
                                 m_offers_service(nullptr), 
                                 m_ui_opt(AUTO_VAL_INIT(m_ui_opt)), 
                                 m_remote_node_mode(false),
                                 m_is_pos_allowed(false),
                                 m_qt_logs_enbaled(false)
{
  m_offers_service.set_disabled(true);
	//m_ccore.get_blockchain_storage().get_attachment_services_manager().add_service(&m_offers_service);
}

const command_line::arg_descriptor<bool> arg_alloc_win_console = {"alloc-win-console", "Allocates debug console with GUI", false};
const command_line::arg_descriptor<std::string> arg_html_folder = {"html-path", "Manually set GUI html folder path", "",  true};
const command_line::arg_descriptor<std::string> arg_xcode_stub = {"-NSDocumentRevisionsDebugMode", "Substitute for xcode bug", "",  true};
const command_line::arg_descriptor<bool> arg_enable_gui_debug_mode = { "gui-debug-mode", "Enable debug options in GUI", false, true };
const command_line::arg_descriptor<uint32_t> arg_qt_remote_debugging_port = { "remote-debugging-port", "Specify port for Qt remote debugging", 30333, true };
const command_line::arg_descriptor<std::string> arg_remote_node = { "remote-node", "Switch GUI to work with remote node instead of local daemon", "",  true };
const command_line::arg_descriptor<bool> arg_enable_qt_logs = { "enable-qt-logs", "Forward Qt log messages into main log", false,  true };

void wallet_lock_time_watching_policy::watch_lock_time(uint64_t lock_time)
{
  if (lock_time > 500)
  {
    LOG_PRINT_RED_L0("[wallet_lock_time_watching_policy::watch_lock_time] LOCK_TIME: " << lock_time);
  }
}

daemon_backend::~daemon_backend()
{
  stop();
}

void terminate_handler_func()
{
  LOG_ERROR("\n\nTERMINATE HANDLER\n"); // should print callstack
  std::fflush(nullptr); // all open output streams are flushed
  std::abort(); // default terminate handler's behavior
}

bool daemon_backend::init(int argc, char* argv[], view::i_view* pview_handler)
{
  m_stop_singal_sent = false;
  if (pview_handler)
    m_pview = pview_handler;

  view::daemon_status_info dsi = AUTO_VAL_INIT(dsi);
  dsi.pos_difficulty = dsi.pow_difficulty = "---";
  pview_handler->update_daemon_status(dsi);

  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_0);
  log_space::get_set_need_thread_id(true, true);
  log_space::log_singletone::enable_channels("core,currency_protocol,tx_pool,p2p,wallet");

  tools::signal_handler::install_fatal([](int sig_number, void* address) {
    LOG_ERROR("\n\nFATAL ERROR\nsig: " << sig_number << ", address: " << address);
    std::fflush(nullptr); // all open output streams are flushed
  });

  // setup custom callstack retrieving function
  epee::misc_utils::get_callstack(tools::get_callstack);

  // setup custom terminate functions
  std::set_terminate(&terminate_handler_func);

  //#if !defined(NDEBUG)
  //  log_space::log_singletone::add_logger(LOGGER_DEBUGGER, nullptr, nullptr);
  //#endif
  LOG_PRINT_L0("Initing...");

  TRY_ENTRY();
  po::options_description desc_cmd_only("Command line options");
  po::options_description desc_cmd_sett("Command line options and settings options");

  command_line::add_arg(desc_cmd_only, command_line::arg_help);
  command_line::add_arg(desc_cmd_only, command_line::arg_version);
  command_line::add_arg(desc_cmd_only, command_line::arg_os_version);
  // tools::get_default_data_dir() can't be called during static initialization
  command_line::add_arg(desc_cmd_only, command_line::arg_data_dir, tools::get_default_data_dir());
  command_line::add_arg(desc_cmd_only, command_line::arg_config_file);

  command_line::add_arg(desc_cmd_sett, command_line::arg_log_dir);
  command_line::add_arg(desc_cmd_sett, command_line::arg_log_level);
  command_line::add_arg(desc_cmd_sett, command_line::arg_console);
  command_line::add_arg(desc_cmd_sett, command_line::arg_show_details);
  command_line::add_arg(desc_cmd_sett, arg_alloc_win_console);
  command_line::add_arg(desc_cmd_sett, arg_html_folder);
  command_line::add_arg(desc_cmd_sett, arg_xcode_stub);
  command_line::add_arg(desc_cmd_sett, arg_enable_gui_debug_mode);
  command_line::add_arg(desc_cmd_sett, arg_qt_remote_debugging_port);
  command_line::add_arg(desc_cmd_sett, arg_remote_node);
  command_line::add_arg(desc_cmd_sett, arg_enable_qt_logs);


  currency::core::init_options(desc_cmd_sett);
  currency::core_rpc_server::init_options(desc_cmd_sett);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core> >::init_options(desc_cmd_sett);
  currency::miner::init_options(desc_cmd_sett);
  bc_services::bc_offers_service::init_options(desc_cmd_sett);

  po::options_description desc_options("Allowed options");
  desc_options.add(desc_cmd_only).add(desc_cmd_sett);


  
  bool coomand_line_parsed = command_line::handle_error_helper(desc_options, [&]()
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

  //set up logging options
  if (command_line::has_arg(m_vm, arg_alloc_win_console))
  {
    log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
  }
  if (command_line::has_arg(m_vm, command_line::arg_log_level))
  {
    log_space::log_singletone::get_set_log_detalisation_level(true, command_line::get_arg(m_vm, command_line::arg_log_level));
  }
  if (command_line::has_arg(m_vm, arg_enable_gui_debug_mode))
  {
    m_ui_opt.use_debug_mode = true;
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
    path_to_html = string_tools::get_current_module_folder() + "/html";
  }
  else
  {
    path_to_html = command_line::get_arg(m_vm, arg_html_folder);
  }

  log_space::log_singletone::add_logger(LOGGER_FILE, log_file_name.c_str(), log_dir.c_str());
  LOG_PRINT_L0(CURRENCY_NAME << " v" << PROJECT_VERSION_LONG);
  LOG_PRINT("Module folder: " << argv[0], LOG_LEVEL_0);

  if (command_line::has_arg(m_vm, arg_remote_node))
  {
    // configure for remote node
  }

  m_qt_logs_enbaled = command_line::get_arg(m_vm, arg_enable_qt_logs);

  m_pview->init(path_to_html);

  if (!coomand_line_parsed)
  {
    std::stringstream ss;
    for (int i = 0; i != argc; i++)
      ss << "[" << i << "] " << argv[i] << std::endl;

    LOG_PRINT_L0("Command line has wrong arguments: " << std::endl << ss.str());
  }
  return true;
  CATCH_ENTRY_L0("init", false);
}

bool daemon_backend::start()
{
  TRY_ENTRY();

  m_main_worker_thread = std::thread([this](){main_worker(this->m_vm);});

  return true;
  CATCH_ENTRY_L0("main", false);
 }

 

bool daemon_backend::send_stop_signal()
{
  m_stop_singal_sent = true;
  return true;
}


bool daemon_backend::stop()
{
  send_stop_signal();
  if (m_main_worker_thread.joinable())
  {
	  LOG_PRINT_L0("Waiting for backend main worker thread: " << m_main_worker_thread.get_id());
	  m_main_worker_thread.join();
  }
    


  return true;
}

std::string daemon_backend::get_config_folder()
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

bool daemon_backend::init_local_daemon()
{
  view::daemon_status_info dsi = AUTO_VAL_INIT(dsi);
  dsi.pos_difficulty = dsi.pos_difficulty = "---";
  dsi.daemon_network_state = currency::COMMAND_RPC_GET_INFO::daemon_network_state_loading_core;
  m_pview->update_daemon_status(dsi);


  //initialize core here
  LOG_PRINT_L0("Initializing core...");
  //dsi.text_state = "Initializing core";
  m_pview->update_daemon_status(dsi);
  bool res = m_ccore.init(m_vm);
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

  LOG_PRINT_L0("Starting core rpc server...");
  //dsi.text_state = "Starting core rpc server";
  m_pview->update_daemon_status(dsi);
  res = m_rpc_server.run(2, false);
  CHECK_AND_ASSERT_AND_SET_GUI(res, "Failed to initialize core rpc server.");
  LOG_PRINT_L0("Core rpc server started ok");

  LOG_PRINT_L0("Starting p2p net loop...");
  //dsi.text_state = "Starting network loop";
  m_pview->update_daemon_status(dsi);
  res = m_p2psrv.run(false);
  CHECK_AND_ASSERT_AND_SET_GUI(res, "Failed to run p2p loop.");
  LOG_PRINT_L0("p2p net loop stopped");

  return true;
}

bool daemon_backend::deinit_local_daemon()
{
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

  return true;
}

void daemon_backend::main_worker(const po::variables_map& m_vm)
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
  

  CRITICAL_REGION_BEGIN(m_wallets_lock);
  for (auto& wo : m_wallets)
  {
    LOG_PRINT_L0("Storing wallet data...");
    //dsi.text_state = "Storing wallets data...";
    //m_pview->update_daemon_status(dsi);
    try
    {
      wo.second.major_stop = true;
      wo.second.stop_for_refresh = true;
      wo.second.w.unlocked_get()->stop();

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
  CATCH_ENTRY_L0("daemon_backend::main_worker", void());
}

bool daemon_backend::update_state_info()
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
    LOG_ERROR("Failed to call get_info");
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

bool daemon_backend::get_last_blocks(view::daemon_status_info& dsi)
{

  return true;
}



void daemon_backend::toggle_pos_mining()
{
  //m_do_mint = !m_do_mint;
  //update_wallets_info();
}

void daemon_backend::loop()
{
  while(!m_stop_singal_sent)
  {
    update_state_info();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

void daemon_backend::init_wallet_entry(wallet_vs_options& wo, uint64_t id)
{
  wo.wallet_id = id;
  wo.do_mining = false;
  wo.major_stop = false;
  wo.stop_for_refresh = false;
  wo.plast_daemon_height = &m_last_daemon_height;
  wo.plast_daemon_network_state = &m_last_daemon_network_state;
  wo.plast_daemon_is_disconnected = &m_last_daemon_is_disconnected;
  wo.pview = m_pview;  
  wo.has_related_alias_in_unconfirmed = false;
  if (m_remote_node_mode)
    wo.core_conf = currency::get_default_core_runtime_config();
  else 
    wo.core_conf = m_ccore.get_blockchain_storage().get_core_runtime_config();

  // update wallet log prefix for further usage
  {
    CRITICAL_REGION_LOCAL(m_wallet_log_prefixes_lock);
    if (m_wallet_log_prefixes.size() <= id)
      m_wallet_log_prefixes.resize(id + 1);
    m_wallet_log_prefixes[id] = std::string("[") + epee::string_tools::num_to_string_fast(id) + ":" + wo.w->get()->get_account().get_public_address_str().substr(0, 6) + "] ";
  }
}


std::string daemon_backend::get_tx_pool_info(currency::COMMAND_RPC_GET_POOL_INFO::response& res)
{
  currency::COMMAND_RPC_GET_POOL_INFO::request req = AUTO_VAL_INIT(req);
  res = AUTO_VAL_INIT_T(currency::COMMAND_RPC_GET_POOL_INFO::response);
  if (!m_rpc_proxy->call_COMMAND_RPC_GET_POOL_INFO(req, res))
  {
    return API_RETURN_CODE_FAIL;
  }
  return API_RETURN_CODE_OK;
}


uint64_t daemon_backend::get_default_fee()
{
  return TX_DEFAULT_FEE;
}
std::string daemon_backend::get_fav_offers(const std::list<bc_services::offer_id>& hashes, const bc_services::core_offers_filter& filter, std::list<bc_services::offer_details_ex>& offers)
{
  if (m_remote_node_mode)
    return API_RETURN_CODE_FAIL;

  currency::blockchain_storage& bcs = m_ccore.get_blockchain_storage();

  m_offers_service.get_offers_by_id(hashes, offers);
  filter_offers_list(offers, filter, bcs.get_core_runtime_config().get_core_time());
  return API_RETURN_CODE_OK;
}

std::string daemon_backend::get_my_offers(const bc_services::core_offers_filter& filter, std::list<bc_services::offer_details_ex>& offers)
{
  if (m_remote_node_mode)
    return API_RETURN_CODE_FAIL;

  CRITICAL_REGION_LOCAL(m_wallets_lock);
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
      return std::string(API_RETURN_CODE_WRONG_PASSWORD) + ":" + e.what();
    }
    
  }

  size_t offers_count_before_filtering = offers.size();
  bc_services::filter_offers_list(offers, filter, m_ccore.get_blockchain_storage().get_core_runtime_config().get_core_time());
  LOG_PRINT("get_my_offers(): " << offers.size() << " offers returned (" << offers_count_before_filtering << " was before filter)", LOG_LEVEL_1);

  return API_RETURN_CODE_OK;
}

std::string daemon_backend::open_wallet(const std::wstring& path, const std::string& password, uint64_t txs_to_return, view::open_wallet_response& owr)
{
  std::shared_ptr<tools::wallet2> w(new tools::wallet2());
  owr.wallet_id = m_wallet_id_counter++;

  w->callback(std::shared_ptr<tools::i_wallet2_callback>(new i_wallet_to_i_backend_adapter(this, owr.wallet_id)));
  if (m_remote_node_mode)
  {
    w->set_core_proxy(m_rpc_proxy);
  }
  else
  {
    w->set_core_proxy(std::shared_ptr<tools::i_core_proxy>(new tools::core_fast_rpc_proxy(m_rpc_server)));
  }
  
  std::string return_code = API_RETURN_CODE_OK;
  CRITICAL_REGION_LOCAL(m_wallets_lock);
  while (true)
  {
    try
    {
      w->load(path, password);
      if (w->is_watch_only())
        return API_RETURN_CODE_WALLET_WATCH_ONLY_NOT_SUPPORTED;
      w->get_recent_transfers_history(owr.recent_history.history, 0, txs_to_return, owr.recent_history.total_history_items);
      //w->get_unconfirmed_transfers(owr.recent_history.unconfirmed);      
      w->get_unconfirmed_transfers(owr.recent_history.history);
      //workaround for missed fee
      break;
    }
    catch (const tools::error::file_not_found& /**/)
    {
      return API_RETURN_CODE_FILE_NOT_FOUND;
    }
    catch (const tools::error::wallet_load_notice_wallet_restored& /**/)
    {
      return_code = API_RETURN_CODE_FILE_RESTORED;
      break;
    }
    catch (const std::exception& e)
    {
      return std::string(API_RETURN_CODE_WRONG_PASSWORD) + ":" + e.what();
    }
  }

  wallet_vs_options& wo = m_wallets[owr.wallet_id];
  **wo.w = w;
  get_wallet_info(wo, owr.wi);
  init_wallet_entry(wo, owr.wallet_id);
  
  return return_code;
}

std::string daemon_backend::get_recent_transfers(size_t wallet_id, uint64_t offset, uint64_t count, view::transfers_array& tr_hist)
{
  GET_WALLET_BY_ID(wallet_id, w);
  auto wallet_locked = w.try_lock();
  if (!wallet_locked.get())
  {
    return API_RETURN_CODE_CORE_BUSY;
  }

  w->get()->get_recent_transfers_history(tr_hist.history, offset, count, tr_hist.total_history_items);
  //workaround for missed fee
  for (auto & he : tr_hist.history)
  {
    he.show_sender = currency::is_showing_sender_addres(he.tx);
    if (!he.fee && !currency::is_coinbase(he.tx))
    {
      he.fee = currency::get_tx_fee(he.tx);
    }
  }

  return API_RETURN_CODE_OK;
}

std::string daemon_backend::generate_wallet(const std::wstring& path, const std::string& password, view::open_wallet_response& owr)
{
  std::shared_ptr<tools::wallet2> w(new tools::wallet2());
  owr.wallet_id = m_wallet_id_counter++;
  w->callback(std::shared_ptr<tools::i_wallet2_callback>(new i_wallet_to_i_backend_adapter(this, owr.wallet_id)));
  if (m_remote_node_mode)
  {
    w->set_core_proxy(m_rpc_proxy);
  }
  else
  {
    w->set_core_proxy(std::shared_ptr<tools::i_core_proxy>(new tools::core_fast_rpc_proxy(m_rpc_server)));
  }


  CRITICAL_REGION_LOCAL(m_wallets_lock);

  try
  {
    w->generate(path, password);
  }
  catch (const tools::error::file_exists/*& e*/)
  {
    return API_RETURN_CODE_ALREADY_EXISTS;
  }

  catch (const std::exception& e)
  {
    return std::string(API_RETURN_CODE_FAIL) + ":" + e.what();
  }
  wallet_vs_options& wo = m_wallets[owr.wallet_id];
  **wo.w = w;
  init_wallet_entry(wo, owr.wallet_id);
  get_wallet_info(wo, owr.wi);
  return API_RETURN_CODE_OK;
}

std::string daemon_backend::get_mining_estimate(uint64_t amuont_coins, 
  uint64_t time, 
  uint64_t& estimate_result, 
  uint64_t& pos_coins_and_pos_diff_rate, 
  std::vector<uint64_t>& days)
{
  if (m_remote_node_mode)
    return API_RETURN_CODE_FAIL;

  m_ccore.get_blockchain_storage().get_pos_mining_estimate(amuont_coins, time, estimate_result, pos_coins_and_pos_diff_rate, days);
  return API_RETURN_CODE_OK;
}

std::string daemon_backend::is_pos_allowed()
{
  if (m_is_pos_allowed)
    return API_RETURN_CODE_TRUE;
  else 
    return API_RETURN_CODE_FALSE;
}
std::string daemon_backend::is_valid_brain_restore_data(const std::string& brain_text)
{
  currency::account_base acc;
  if (acc.restore_keys_from_braindata(brain_text))
    return API_RETURN_CODE_TRUE;
  else
    return API_RETURN_CODE_FALSE;
}
void daemon_backend::subscribe_to_core_events(currency::i_core_event_handler* pevents_handler)
{
  if(!m_remote_node_mode)
    m_ccore.get_blockchain_storage().set_event_handler(pevents_handler);
}
void daemon_backend::get_gui_options(view::gui_options& opt)
{
  opt = m_ui_opt;
}
std::string daemon_backend::restore_wallet(const std::wstring& path, const std::string& password, const std::string& restore_key, view::open_wallet_response& owr)
{
  std::shared_ptr<tools::wallet2> w(new tools::wallet2());
  owr.wallet_id = m_wallet_id_counter++;
  w->callback(std::shared_ptr<tools::i_wallet2_callback>(new i_wallet_to_i_backend_adapter(this, owr.wallet_id)));
  if (m_remote_node_mode)
  {
    w->set_core_proxy(m_rpc_proxy);
  }
  else
  {
    w->set_core_proxy(std::shared_ptr<tools::i_core_proxy>(new tools::core_fast_rpc_proxy(m_rpc_server)));
  }

  currency::account_base acc;

  CRITICAL_REGION_LOCAL(m_wallets_lock);

  try
  {
    w->restore(path, password, restore_key);
  }
  catch (const tools::error::file_exists/*& e*/)
  {
    return API_RETURN_CODE_ALREADY_EXISTS;
  }

  catch (const std::exception& e)
  {
    return std::string(API_RETURN_CODE_FAIL) + ":" + e.what();
  }
  wallet_vs_options& wo = m_wallets[owr.wallet_id];
  **wo.w = w;
  init_wallet_entry(wo, owr.wallet_id);
  get_wallet_info(wo, owr.wi);
  return API_RETURN_CODE_OK;
}
std::string daemon_backend::close_wallet(size_t wallet_id)
{
  CRITICAL_REGION_LOCAL(m_wallets_lock);
  
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

std::string daemon_backend::get_aliases(view::alias_set& al_set)
{
  if (m_remote_node_mode)
    return API_RETURN_CODE_OVERFLOW;

  if (m_ccore.get_blockchain_storage().get_aliases_count() > API_MAX_ALIASES_COUNT)
    return API_RETURN_CODE_OVERFLOW;


  currency::COMMAND_RPC_GET_ALL_ALIASES::response aliases = AUTO_VAL_INIT(aliases);
  if (m_rpc_proxy->call_COMMAND_RPC_GET_ALL_ALIASES(aliases) && aliases.status == CORE_RPC_STATUS_OK)
  {
    al_set.aliases = aliases.aliases;
    return API_RETURN_CODE_OK;
  }

  return API_RETURN_CODE_FAIL;
}
std::string daemon_backend::get_alias_info_by_address(const std::string& addr, currency::alias_rpc_details& res_details)
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

  res_details = res.alias_info;
  return res.status;
}

std::string daemon_backend::get_alias_info_by_name(const std::string& name, currency::alias_rpc_details& res_details)
{
  if(name.empty())
    return API_RETURN_CODE_FILE_NOT_FOUND;

  currency::COMMAND_RPC_GET_ALIAS_DETAILS::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_ALIAS_DETAILS::response res = AUTO_VAL_INIT(res);

  req.alias = name;

  bool r = m_rpc_proxy->call_COMMAND_RPC_GET_ALIAS_DETAILS(req, res);
  if (!r)
    return API_RETURN_CODE_FAIL;


  res_details.alias = name;
  res_details.details = res.alias_details;
  return res.status;
}

std::string daemon_backend::get_alias_coast(const std::string& a, uint64_t& coast)
{
  currency::COMMAND_RPC_GET_ALIAS_REWARD::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_ALIAS_REWARD::response rsp = AUTO_VAL_INIT(rsp);
  req.alias = a;
  if (!m_rpc_proxy->call_COMMAND_RPC_GET_ALIAS_REWARD(req, rsp))
    return API_RETURN_CODE_BAD_ARG;

  coast = rsp.reward + rsp.reward/10; //add 10% of price to be sure
  return rsp.status;

}
std::string daemon_backend::request_alias_registration(const currency::alias_rpc_details& al, uint64_t wallet_id, uint64_t fee, currency::transaction& res_tx, uint64_t reward)
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
  if (m_rpc_proxy->call_COMMAND_RPC_GET_ALIAS_DETAILS(req, rsp) && rsp.status == CORE_RPC_STATUS_NOT_FOUND)
  {
    GET_WALLET_BY_ID(wallet_id, w);
    try
    {
      w->get()->request_alias_registration(ai, res_tx, fee, reward);
      return API_RETURN_CODE_OK;
    }
    catch (const std::exception& e)
    {
      LOG_ERROR(get_wallet_log_prefix(wallet_id) + "request_alias_registration error: " << e.what());
      std::string err_code = API_RETURN_CODE_INTERNAL_ERROR;
      err_code += std::string(":") + e.what();
      return err_code;
    }
    catch (...)
    {
      LOG_ERROR(get_wallet_log_prefix(wallet_id) + "request_alias_registration error: unknown error");
      return API_RETURN_CODE_INTERNAL_ERROR;
    }
  }

  return API_RETURN_CODE_ALREADY_EXISTS;
}

std::string daemon_backend::request_alias_update(const currency::alias_rpc_details& al, uint64_t wallet_id, uint64_t fee, currency::transaction& res_tx, uint64_t reward)
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
  if (m_rpc_proxy->call_COMMAND_RPC_GET_ALIAS_DETAILS(req, rsp) && rsp.status == CORE_RPC_STATUS_OK)
  {
    GET_WALLET_BY_ID(wallet_id, w);
    try
    {
      w->get()->request_alias_update(ai, res_tx, fee, reward);
      return API_RETURN_CODE_OK;
    }
    catch (const std::exception& e)
    {
      LOG_ERROR(get_wallet_log_prefix(wallet_id) + "request_alias_update error: " << e.what());
      std::string err_code = API_RETURN_CODE_INTERNAL_ERROR;
      err_code += std::string(":") + e.what();
      return err_code;
    }
    catch (...)
    {
      LOG_ERROR(get_wallet_log_prefix(wallet_id) + "request_alias_update error: unknown error");
      return API_RETURN_CODE_INTERNAL_ERROR;
    }
  }

  return API_RETURN_CODE_FILE_NOT_FOUND;
}


std::string daemon_backend::transfer(size_t wallet_id, const view::transfer_params& tp, currency::transaction& res_tx)
{

  std::vector<currency::tx_destination_entry> dsts;
  if(!tp.destinations.size())
    return API_RETURN_CODE_BAD_ARG_EMPTY_DESTINATIONS;

  uint64_t fee = tp.fee;
//  if (!currency::parse_amount(fee, tp.fee))
//    return API_RETURN_CODE_BAD_ARG_WRONG_FEE;

  std::string payment_id = tp.payment_id;
  for(auto& d: tp.destinations)
  {
    dsts.push_back(currency::tx_destination_entry());
    dsts.back().addr.resize(1);
    std::string embedded_payment_id;
    if (!tools::get_transfer_address(d.address, dsts.back().addr.back(), embedded_payment_id, m_rpc_proxy.get()))
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
  }
  //payment_id
  std::vector<currency::attachment_v> attachments;
  std::vector<currency::extra_v> extra;
  if (payment_id.size())
  {
    if (!currency::is_payment_id_size_ok(payment_id))
      return API_RETURN_CODE_BAD_ARG_WRONG_PAYMENT_ID; // payment id is too big

    if (!currency::set_payment_id_to_tx(attachments, payment_id))
      return API_RETURN_CODE_INTERNAL_ERROR;
  }

  GET_WALLET_BY_ID(wallet_id, w);



  try
  { 
    //set transaction unlock time if it was specified by user 
    uint64_t unlock_time = 0;
    if (tp.lock_time)
    {
      if (tp.lock_time > CURRENCY_MAX_BLOCK_NUMBER)
        unlock_time = tp.lock_time;
      else
        unlock_time = w->get()->get_blockchain_current_height() + tp.lock_time;
    }
      
    
    //proces attachments
    if (tp.comment.size())
    {
      currency::tx_comment tc = AUTO_VAL_INIT(tc);
      tc.comment = tp.comment;
      extra.push_back(tc);
    }
    if (tp.push_payer)
    {
      currency::tx_payer txp = AUTO_VAL_INIT(txp);
      txp.acc_addr = w->get()->get_account().get_keys().m_account_address;
      extra.push_back(txp);
    }    
    if (!tp.hide_receiver)
    {
      for (auto& d : dsts)
      {
        for (auto& a : d.addr)
        {
          currency::tx_receiver txr = AUTO_VAL_INIT(txr);
          txr.acc_addr = a;
          extra.push_back(txr);
        }
      }
    }
    w->get()->transfer(dsts, tp.mixin_count, unlock_time ? unlock_time + 1 : 0, fee, extra, attachments, res_tx);
    //update_wallets_info();
  }
  catch (const std::exception& e)
  {
    LOG_ERROR(get_wallet_log_prefix(wallet_id) + "Transfer error: " << e.what());
    std::string err_code = API_RETURN_CODE_INTERNAL_ERROR;
    err_code += std::string(":") + e.what();
    return err_code;
  }
  catch (...)
  {
    LOG_ERROR(get_wallet_log_prefix(wallet_id) + "Transfer error: unknown error");
    return API_RETURN_CODE_INTERNAL_ERROR;
  }

  return API_RETURN_CODE_OK;
}

std::string daemon_backend::get_wallet_info(size_t wallet_id, view::wallet_info& wi)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);
  return get_wallet_info(w, wi);
}
std::string daemon_backend::get_contracts(size_t wallet_id, std::vector<tools::wallet_public::escrow_contract_details>& contracts)
{
  tools::wallet2::escrow_contracts_container cc;
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
std::string daemon_backend::create_proposal(const view::create_proposal_param_gui& cpp)
{
  //tools::wallet2::escrow_contracts_container cc;
  GET_WALLET_OPT_BY_ID(cpp.wallet_id, w);
  try
  {
    currency::transaction tx = AUTO_VAL_INIT(tx);
    currency::transaction template_tx = AUTO_VAL_INIT(template_tx);
    w.w->get()->send_escrow_proposal(cpp, tx, template_tx);
    //TODO: add some 
    return API_RETURN_CODE_OK;
  }
  catch (const tools::error::not_enough_money& e)
  {
    LOG_ERROR(get_wallet_log_prefix(cpp.wallet_id) + "send_escrow_proposal error: API_RETURN_CODE_NOT_ENOUGH_MONEY: " << e.what());
    std::string err_code = API_RETURN_CODE_NOT_ENOUGH_MONEY;
    return err_code;
  }
  catch (const std::exception& e)
  {
    LOG_ERROR(get_wallet_log_prefix(cpp.wallet_id) + "send_escrow_proposal error: " << e.what());
    std::string err_code = API_RETURN_CODE_INTERNAL_ERROR;
    err_code += std::string(":") + e.what();
    return err_code;
  }
  catch (...)
  {
    LOG_ERROR(get_wallet_log_prefix(cpp.wallet_id) + "send_escrow_proposal error: unknown error");
    return API_RETURN_CODE_INTERNAL_ERROR;
  }
}

std::string daemon_backend::accept_proposal(size_t wallet_id, const crypto::hash& contract_id)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);
  try
  {
    w.w->get()->accept_proposal(contract_id, TX_DEFAULT_FEE);
    //TODO: add some 
    return API_RETURN_CODE_OK;
  }
  catch (...)
  {
    return API_RETURN_CODE_FAIL;
  }
}
std::string daemon_backend::release_contract(size_t wallet_id, const crypto::hash& contract_id, const std::string& contract_over_type)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);
  try
  {
    w.w->get()->finish_contract(contract_id, contract_over_type);
    //TODO: add some 
    return API_RETURN_CODE_OK;
  }
  catch (...)
  {
    return API_RETURN_CODE_FAIL;
  }
}
std::string daemon_backend::request_cancel_contract(size_t wallet_id, const crypto::hash& contract_id, uint64_t fee, uint64_t expiration_period)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);
  try
  {
    w.w->get()->request_cancel_contract(contract_id, fee, expiration_period);
    //TODO: add some 
    return API_RETURN_CODE_OK;
  }
  catch (const tools::error::not_enough_money& e)
  {
    LOG_ERROR(get_wallet_log_prefix(wallet_id) + "request_cancel_contract error: API_RETURN_CODE_NOT_ENOUGH_MONEY: " << e.what());
    return API_RETURN_CODE_NOT_ENOUGH_MONEY;
  }
  catch (...)
  {
    return API_RETURN_CODE_FAIL;
  }
}
std::string daemon_backend::accept_cancel_contract(size_t wallet_id, const crypto::hash& contract_id)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);
  try
  {
    TIME_MEASURE_START_MS(timing1);
    w.w->get()->accept_cancel_contract(contract_id);
    //TODO: add some 
    TIME_MEASURE_FINISH_MS(timing1);
    if (timing1 > 500)
      LOG_PRINT_RED_L0(get_wallet_log_prefix(wallet_id) + "[daemon_backend::accept_cancel_contract] LOW PERFORMANCE: " << timing1 );

    return API_RETURN_CODE_OK;
  }
  catch (...)
  {
    return API_RETURN_CODE_FAIL;
  }
}
std::string daemon_backend::backup_wallet(uint64_t wallet_id, const std::wstring& path)
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
std::string daemon_backend::reset_wallet_password(uint64_t wallet_id, const std::string& pass)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);
  if (w.w->get()->reset_password(pass))
    return API_RETURN_CODE_OK;
  else
    return API_RETURN_CODE_FAIL;
}
std::string daemon_backend::is_wallet_password_valid(uint64_t wallet_id, const std::string& pass)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);
  if (w.w->get()->is_password_valid(pass))
    return API_RETURN_CODE_OK;
  else
    return API_RETURN_CODE_FAIL;
}
std::string daemon_backend::resync_wallet(uint64_t wallet_id)
{
  GET_WALLET_OPT_BY_ID(wallet_id, w);
  w.w->get()->reset_history();
  w.last_wallet_synch_height = 0;
  return API_RETURN_CODE_OK;
}
std::string daemon_backend::start_pos_mining(uint64_t wallet_id)
{
  GET_WALLET_OPT_BY_ID(wallet_id, wo);
  wo.do_mining = true;
  wo.need_to_update_wallet_info = true;
  return API_RETURN_CODE_OK;
}
std::string daemon_backend::get_mining_history(uint64_t wallet_id, tools::wallet_public::mining_history& mh)
{
  GET_WALLET_OPT_BY_ID(wallet_id, wo);

  if (wo.wallet_state != view::wallet_status_info::wallet_state_ready)
    return API_RETURN_CODE_CORE_BUSY;

  wo.w->get()->get_mining_history(mh);
  return API_RETURN_CODE_OK;
}
std::string daemon_backend::get_wallet_restore_info(uint64_t wallet_id, std::string& restore_key)
{
  GET_WALLET_OPT_BY_ID(wallet_id, wo);
  restore_key = wo.w->get()->get_account().get_restore_braindata();
//  restore_key = tools::base58::encode(rst_data);
  return API_RETURN_CODE_OK;
}
void daemon_backend::prepare_wallet_status_info(wallet_vs_options& wo, view::wallet_status_info& wsi)
{
  wsi.is_mining = wo.do_mining;
  wsi.wallet_id = wo.wallet_id;
  wsi.is_alias_operations_available = !wo.has_related_alias_in_unconfirmed;
  wsi.balance = wo.w->get()->balance(wsi.unlocked_balance, wsi.awaiting_in, wsi.awaiting_out, wsi.minied_total);
}
std::string daemon_backend::check_available_sources(uint64_t wallet_id, std::list<uint64_t>& amounts)
{
  GET_WALLET_OPT_BY_ID(wallet_id, wo);
  if (wo.w->get()->check_available_sources(amounts))
    return API_RETURN_CODE_TRUE;
  else
    return API_RETURN_CODE_FALSE;
}
std::string daemon_backend::stop_pos_mining(uint64_t wallet_id)
{
  GET_WALLET_OPT_BY_ID(wallet_id, wo);
  wo.do_mining = false;
  wo.need_to_update_wallet_info = true;
  return API_RETURN_CODE_OK;
}
std::string daemon_backend::run_wallet(uint64_t wallet_id)
{
  GET_WALLET_OPT_BY_ID(wallet_id, wo);
  wo.miner_thread = std::thread(boost::bind(&daemon_backend::wallet_vs_options::worker_func, &wo));
  return API_RETURN_CODE_OK;
}

std::string daemon_backend::get_wallet_info(wallet_vs_options& wo, view::wallet_info& wi)
{
  auto locker_object = wo.w.lock();
  tools::wallet2& rw = *(*(*locker_object)); //this looks a bit crazy, i know
  tools::get_wallet_info(rw, wi);
  return API_RETURN_CODE_OK;
}

std::string daemon_backend::push_offer(size_t wallet_id, const bc_services::offer_details_ex& od, currency::transaction& res_tx)
{
  GET_WALLET_BY_ID(wallet_id, w);

  try
  {
    w->get()->push_offer(od, res_tx);
    return API_RETURN_CODE_OK;
  }
  catch (const std::exception& e)
  {
    LOG_ERROR(get_wallet_log_prefix(wallet_id) + "push_offer error: " << e.what());
    std::string err_code = API_RETURN_CODE_INTERNAL_ERROR;
    err_code += std::string(":") + e.what();
    return err_code;
  }
  catch (...)
  {
    LOG_ERROR(get_wallet_log_prefix(wallet_id) + "push_offer error: unknown error");
    return API_RETURN_CODE_INTERNAL_ERROR;
  }
}
std::string daemon_backend::cancel_offer(const view::cancel_offer_param& co, currency::transaction& res_tx)
{
  GET_WALLET_BY_ID(co.wallet_id, w);
  try
  {
    w->get()->cancel_offer_by_id(co.tx_id, co.no, TX_DEFAULT_FEE, res_tx);
    return API_RETURN_CODE_OK;
  }
  catch (const std::exception& e)
  {
    LOG_ERROR(get_wallet_log_prefix(co.wallet_id) + "cancel_offer error: " << e.what());
    std::string err_code = API_RETURN_CODE_INTERNAL_ERROR;
    err_code += std::string(":") + e.what();
    return err_code;
  }
  catch (...)
  {
    LOG_ERROR(get_wallet_log_prefix(co.wallet_id) + "cancel_offer error: unknown error");
    return API_RETURN_CODE_INTERNAL_ERROR;
  }
}

std::string daemon_backend::push_update_offer(const bc_services::update_offer_details& uo, currency::transaction& res_tx)
{
  GET_WALLET_BY_ID(uo.wallet_id, w);

  try
  {
    w->get()->update_offer_by_id(uo.tx_id, uo.no, uo.od, res_tx);
    return API_RETURN_CODE_OK;
  }
  catch (const std::exception& e)
  {
    LOG_ERROR(get_wallet_log_prefix(uo.wallet_id) + "push_update_offer error: " << e.what());
    std::string err_code = API_RETURN_CODE_INTERNAL_ERROR;
    err_code += std::string(":") + e.what();
    return err_code;
  }
  catch (...)
  {
    LOG_ERROR(get_wallet_log_prefix(uo.wallet_id) + "push_update_offer error: unknown error");
    return API_RETURN_CODE_INTERNAL_ERROR;
  }
}

// std::string daemon_backend::get_all_offers(currency::COMMAND_RPC_GET_OFFERS_EX::response& od)
// {
//   currency::COMMAND_RPC_GET_OFFERS_EX::request rq = AUTO_VAL_INIT(rq);
//   m_rpc_proxy->call_COMMAND_RPC_GET_OFFERS_EX(rq, od);
//   return API_RETURN_CODE_OK;
// }


std::string daemon_backend::get_offers_ex(const bc_services::core_offers_filter& cof, std::list<bc_services::offer_details_ex>& offers, uint64_t& total_count)
{
  if (m_remote_node_mode)
    return API_RETURN_CODE_FAIL;  
  //TODO: make it proxy-like call
  //m_ccore.get_blockchain_storage().get_offers_ex(cof, offers, total_count);
	m_offers_service.get_offers_ex(cof, offers, total_count, m_ccore.get_blockchain_storage().get_core_runtime_config().get_core_time());
  return API_RETURN_CODE_OK;
}


std::string daemon_backend::validate_address(const std::string& addr_str, std::string& payment_id)
{
  currency::account_public_address acc = AUTO_VAL_INIT(acc);
  if (currency::get_account_address_and_payment_id_from_str(acc, payment_id, addr_str))
    return API_RETURN_CODE_TRUE;
  else
    return API_RETURN_CODE_FALSE;
}

void daemon_backend::on_new_block(size_t wallet_id, uint64_t /*height*/, const currency::block& /*block*/)
{

}

void daemon_backend::on_transfer2(size_t wallet_id, const tools::wallet_public::wallet_transfer_info& wti, uint64_t balance, uint64_t unlocked_balance, uint64_t total_mined)
{
  view::transfer_event_info tei = AUTO_VAL_INIT(tei);
  tei.ti = wti;
  tei.balance = balance;
  tei.unlocked_balance = unlocked_balance;
  tei.wallet_id = wallet_id;
  m_pview->money_transfer(tei);
}
void daemon_backend::on_pos_block_found(size_t wallet_id, const currency::block& b)
{
  m_pview->pos_block_found(b);
}
void daemon_backend::on_sync_progress(size_t wallet_id, const uint64_t& percents)
{
  // do not lock m_wallets_lock down the callstack! It will lead to a deadlock, because wallet locked_object is aready locked
  // and other threads are usually locks m_wallets_lock before locking wallet's locked_object

  view::wallet_sync_progres_param wspp = AUTO_VAL_INIT(wspp);
  wspp.progress = percents;
  wspp.wallet_id = wallet_id;
  m_pview->wallet_sync_progress(wspp);
}
void daemon_backend::on_transfer_canceled(size_t wallet_id, const tools::wallet_public::wallet_transfer_info& wti)
{
  view::transfer_event_info tei = AUTO_VAL_INIT(tei);
  tei.ti = wti;

  CRITICAL_REGION_LOCAL(m_wallets_lock);
  auto& w = m_wallets[wallet_id].w;
  if (w->get() != nullptr)
  {
    tei.balance = w->get()->balance();
    tei.unlocked_balance = w->get()->unlocked_balance();
    tei.wallet_id = wallet_id;
  }
  else
  {
    LOG_ERROR(get_wallet_log_prefix(wallet_id) + "on_transfer() wallet with id = " << wallet_id << " not found");
  }
  m_pview->money_transfer_cancel(tei);
}
void daemon_backend::wallet_vs_options::worker_func()
{
  LOG_PRINT_GREEN("[WALLET_HANDLER] Wallet handler thread started, addr: " << w->get()->get_account().get_public_address_str(), LOG_LEVEL_0);
  epee::math_helper::once_a_time_seconds<1> scan_pool_interval;
  epee::math_helper::once_a_time_seconds<POS_WALLET_MINING_SCAN_INTERVAL> pos_minin_interval;
  view::wallet_status_info wsi = AUTO_VAL_INIT(wsi);
  while (!major_stop)
  {
    stop_for_refresh = false;
    try
    {
      wsi.wallet_state = view::wallet_status_info::wallet_state_ready;
      if (plast_daemon_is_disconnected && plast_daemon_is_disconnected->load())
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

          if (show_progress)
          {
            wallet_state = wsi.wallet_state = view::wallet_status_info::wallet_state_synchronizing;
            prepare_wallet_status_info(*this, wsi);
            pview->update_wallet_status(wsi);
          }
          w->get()->refresh(stop_for_refresh);
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
          if (!w->get()->fill_mining_context(ctx) || ctx.rsp.status != CORE_RPC_STATUS_OK)
          {
            LOG_PRINT_L1(get_log_prefix() + " cannot obtain PoS mining context, skip iteration");
            return true;
          }

          uint64_t pos_entries_amount = 0;
          for (auto& ent : ctx.sp.pos_entries)
            pos_entries_amount += ent.amount;

          tools::wallet2::scan_pos(ctx, break_mining_loop, [this](){
            return *plast_daemon_network_state == currency::COMMAND_RPC_GET_INFO::daemon_network_state_online &&  *plast_daemon_height == last_wallet_synch_height;
          }, core_conf);

          if (ctx.rsp.status == CORE_RPC_STATUS_OK)
          {
            w->get()->build_minted_block(ctx.sp, ctx.rsp);
          }
          LOG_PRINT_L1(get_log_prefix() << " PoS mining iteration finished, status: " << ctx.rsp.status << ", used " << ctx.sp.pos_entries.size() << " entries with total amount: " << currency::print_money_brief(pos_entries_amount) << ", processed: " << ctx.rsp.iterations_processed << " iter.");
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
daemon_backend::wallet_vs_options::~wallet_vs_options()
{
  do_mining = false;
  major_stop = true;
  stop_for_refresh = true;
  break_mining_loop = true;
  if (miner_thread.joinable())
    miner_thread.join();
}

std::string daemon_backend::get_wallet_log_prefix(size_t wallet_id) const
{
  CRITICAL_REGION_LOCAL(m_wallet_log_prefixes_lock);

  CHECK_AND_ASSERT_MES(wallet_id < m_wallet_log_prefixes.size(), std::string("[") + epee::string_tools::num_to_string_fast(wallet_id) + ":???] ", "wallet prefix is not found for id " << wallet_id);
  return m_wallet_log_prefixes[wallet_id];
}

