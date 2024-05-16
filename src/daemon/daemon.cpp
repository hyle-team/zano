// Copyright (c) 2014-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// node.cpp : Defines the entry point for the console application.
//

#include "include_base_utils.h"
#include "version.h"

using namespace epee;

#include <boost/program_options.hpp>

#include "crypto/hash.h"
#include "console_handler.h"
#include "p2p/net_node.h"
#include "currency_core/checkpoints_create.h"
#include "currency_core/currency_core.h"
#include "rpc/core_rpc_server.h"
#include "stratum/stratum_server.h"
#include "currency_protocol/currency_protocol_handler.h"
#include "daemon_commands_handler.h"
#include "common/miniupnp_helper.h"
#include "version.h"
#include "currency_core/core_tools.h"
#include "common/callstack_helper.h"
#include "common/pre_download.h"

#include <cstdlib>

#if defined(WIN32)
#include <crtdbg.h>
#pragma comment(lib, "ntdll") // <-- why do we need this?
#endif


//TODO: need refactoring here. (template classes can't be used in BOOST_CLASS_VERSION) 
BOOST_CLASS_VERSION(nodetool::node_server<currency::t_currency_protocol_handler<currency::core> >, CURRENT_P2P_STORAGE_ARCHIVE_VER);


namespace po = boost::program_options;

bool command_line_preprocessor(const boost::program_options::variables_map& vm);

template<typename p2psrv_t>
struct core_critical_error_handler_t : public currency::i_critical_error_handler
{
  core_critical_error_handler_t(daemon_commands_handler& dch, p2psrv_t& p2psrv, bool dont_stop_on_time_error, bool dont_stop_on_low_space)
    : dch(dch)
    , p2psrv(p2psrv)
    , dont_stop_on_time_error(dont_stop_on_time_error)
    , dont_stop_on_low_space(dont_stop_on_low_space)
  {}

  // interface currency::i_critical_error_handler
  virtual bool on_critical_time_sync_error() override
  {
    if (dont_stop_on_time_error)
      return false; // ignore such errors
    
    LOG_ERROR(ENDL << ENDL << "Serious time sync problem detected, daemon will stop immediately" << ENDL << ENDL);

    // stop handling
    dch.stop_handling();
    return true; // the caller must stop processing
  }

  // interface currency::i_critical_error_handler
  virtual bool on_critical_low_free_space(uint64_t available, uint64_t required) override
  {
    if (dont_stop_on_low_space)
      return false; // ignore such errors
    
    LOG_ERROR(ENDL << ENDL << "Free space at data directory is critically low (" << available / (1024 * 1024) << " MB, while " << required / (1024 * 1024) << " MB is required), daemon will stop immediately" << ENDL << ENDL);

    /*
    temporary disable daemon stop due to issue #133
    */
    return false;
    /*
    // stop handling
    dch.stop_handling();
    p2psrv.send_stop_signal();
    return true; // the caller must stop processing
    */
  }

  virtual bool on_immediate_stop_requested() override
  {
    LOG_PRINT_L0(ENDL << ENDL << "immediate daemon stop requested, stopping..." << ENDL << ENDL);

    dch.stop_handling();
    p2psrv.send_stop_signal();
    return true; // the caller must stop processing
  }


  daemon_commands_handler& dch;
  p2psrv_t& p2psrv;
  bool dont_stop_on_time_error;
  bool dont_stop_on_low_space;
};

void terminate_handler_func()
{
  LOG_ERROR("\n\nTERMINATE HANDLER\n"); // should print callstack
  std::fflush(nullptr); // all open output streams are flushed
  std::abort(); // default terminate handler's behavior
}

int main(int argc, char* argv[])
{
  try
    {
      TRY_ENTRY();

  string_tools::set_module_name_and_folder(argv[0]);
#ifdef WIN32
  _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
  //_CrtSetAllocHook(alloc_hook);

#endif
  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_0);
  log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
  log_space::log_singletone::enable_channels("core,currency_protocol,tx_pool,wallet", false);
  LOG_PRINT_L0("Starting...");

  tools::signal_handler::install_fatal([](int sig_number, void* address) {
    LOG_ERROR("\n\nFATAL ERROR\nsig: " << sig_number << ", address: " << address);
    std::fflush(nullptr); // all open output streams are flushed
  });

  // setup custom callstack retrieving function
  epee::misc_utils::get_callstack(tools::get_callstack);

  // setup custom terminate functions
  std::set_terminate(&terminate_handler_func);

  po::options_description desc_cmd_only("Command line options");
  po::options_description desc_cmd_sett("Command line options and settings options", 130, 83);

  command_line::add_arg(desc_cmd_only, command_line::arg_help);
  command_line::add_arg(desc_cmd_only, command_line::arg_version);
  command_line::add_arg(desc_cmd_only, command_line::arg_os_version);
  // tools::get_default_data_dir() can't be called during static initialization
  command_line::add_arg(desc_cmd_sett, command_line::arg_data_dir, tools::get_default_data_dir());
  command_line::add_arg(desc_cmd_only, command_line::arg_stop_after_height);
  command_line::add_arg(desc_cmd_only, command_line::arg_config_file);
  command_line::add_arg(desc_cmd_only, command_line::arg_disable_upnp);

  command_line::add_arg(desc_cmd_sett, command_line::arg_log_dir);
  command_line::add_arg(desc_cmd_sett, command_line::arg_log_level);
  command_line::add_arg(desc_cmd_sett, command_line::arg_console);
  command_line::add_arg(desc_cmd_only, command_line::arg_show_details);
  command_line::add_arg(desc_cmd_only, command_line::arg_generate_rpc_autodoc);
  command_line::add_arg(desc_cmd_sett, command_line::arg_disable_stop_if_time_out_of_sync);
  command_line::add_arg(desc_cmd_sett, command_line::arg_disable_stop_on_low_free_space);
  command_line::add_arg(desc_cmd_sett, command_line::arg_enable_offers_service);

  command_line::add_arg(desc_cmd_sett, command_line::arg_no_predownload);
  command_line::add_arg(desc_cmd_sett, command_line::arg_force_predownload);
  command_line::add_arg(desc_cmd_sett, command_line::arg_process_predownload_from_path);
  command_line::add_arg(desc_cmd_sett, command_line::arg_validate_predownload);
  command_line::add_arg(desc_cmd_sett, command_line::arg_predownload_link);
  command_line::add_arg(desc_cmd_sett, command_line::arg_disable_ntp);


  arg_market_disable.default_value = true;
  arg_market_disable.use_default = true;

  currency::core::init_options(desc_cmd_sett);
  currency::core_rpc_server::init_options(desc_cmd_sett);
  typedef nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > p2psrv_t;
  p2psrv_t::init_options(desc_cmd_sett);
  currency::miner::init_options(desc_cmd_sett);
  bc_services::bc_offers_service::init_options(desc_cmd_sett);
  currency::stratum_server::init_options(desc_cmd_sett);
  tools::db::db_backend_selector::init_options(desc_cmd_sett);

  po::options_description desc_options("Allowed options");
  desc_options.add(desc_cmd_only).add(desc_cmd_sett);

  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_options, [&]()
  {
    po::store(po::parse_command_line(argc, argv, desc_options), vm);

    if (command_line::get_arg(vm, command_line::arg_help))
    {
      std::cout << CURRENCY_NAME << " v" << PROJECT_VERSION_LONG << ENDL << ENDL;
      std::cout << desc_options << std::endl;
      return false;
    }

    std::string data_dir = command_line::get_arg(vm, command_line::arg_data_dir);
    std::string config = command_line::get_arg(vm, command_line::arg_config_file);

    boost::filesystem::path data_dir_path(epee::string_encoding::utf8_to_wstring(data_dir));
    boost::filesystem::path config_path(epee::string_encoding::utf8_to_wstring(config));
    if (!config_path.has_parent_path())
    {
      config_path = data_dir_path / config_path;
    }

    boost::system::error_code ec;
    if (boost::filesystem::exists(config_path, ec))
    {
      po::store(po::parse_config_file<char>(config_path.string<std::string>().c_str(), desc_cmd_sett), vm);
    }
    po::notify(vm);

    return true;
  });
  if (!r)
    return EXIT_FAILURE;

  //set up logging options
  std::string log_dir;
  std::string log_file_name = log_space::log_singletone::get_default_log_file();
  //check if there was specific option
  if (command_line::has_arg(vm, command_line::arg_log_dir))
  {
    log_dir = command_line::get_arg(vm, command_line::arg_log_dir);
  }
  else
  {
    log_dir = command_line::get_arg(vm, command_line::arg_data_dir);
  }

  log_space::log_singletone::add_logger(LOGGER_FILE, log_file_name.c_str(), log_dir.c_str());
  LOG_PRINT_L0(CURRENCY_NAME << " v" << PROJECT_VERSION_LONG);

  if (command_line_preprocessor(vm))
  {
    return EXIT_SUCCESS;
  }


  // stratum server is enabled if any of its options present
  bool stratum_enabled = currency::stratum_server::should_start(vm);
  LOG_PRINT("Module folder: " << argv[0], LOG_LEVEL_0);

  //create objects and link them
  bc_services::bc_offers_service offers_service(nullptr);
  offers_service.set_disabled(true); //disable by default
  currency::core ccore(NULL);
  currency::t_currency_protocol_handler<currency::core> cprotocol(ccore, NULL );
  p2psrv_t p2psrv(cprotocol);
  currency::core_rpc_server rpc_server(ccore, p2psrv, offers_service);
  cprotocol.set_p2p_endpoint(&p2psrv);
  ccore.set_currency_protocol(&cprotocol);
  daemon_commands_handler dch(p2psrv, rpc_server);

  core_critical_error_handler_t<p2psrv_t> cceh(dch, p2psrv,
    command_line::get_arg(vm, command_line::arg_disable_stop_if_time_out_of_sync),
    command_line::get_arg(vm, command_line::arg_disable_stop_on_low_free_space));
  ccore.set_critical_error_handler(&cceh);


  if (command_line::get_arg(vm, command_line::arg_enable_offers_service))
  {
    offers_service.set_disabled(false);
    ccore.get_blockchain_storage().get_attachment_services_manager().add_service(&offers_service);
  }

  
  
  std::shared_ptr<currency::stratum_server> stratum_server_ptr; 
  if (stratum_enabled)
    stratum_server_ptr = std::make_shared<currency::stratum_server>(&ccore);



  if (command_line::has_arg(vm, command_line::arg_generate_rpc_autodoc))
  {
    std::string path_to_generate = command_line::get_arg(vm, command_line::arg_generate_rpc_autodoc);

    if (!generate_doc_as_md_files(path_to_generate, rpc_server))
      return 1;
    return 0;
  }

  bool res = false;

  //do pre_download if needed
  if (!command_line::has_arg(vm, command_line::arg_no_predownload) || command_line::has_arg(vm, command_line::arg_force_predownload))
  {
    auto is_stop_signal_sent = [&p2psrv]() -> bool {
      return static_cast<nodetool::i_p2p_endpoint<currency::t_currency_protocol_handler<currency::core>::connection_context>*>(&p2psrv)->is_stop_signal_sent();
    };

    if (!tools::process_predownload(vm, [&](uint64_t total_bytes, uint64_t received_bytes) { return is_stop_signal_sent(); }))
    {
      return EXIT_FAILURE;
    }
    
    if (is_stop_signal_sent())
      return 1;
  }


  //initialize objects
  LOG_PRINT_L0("Initializing p2p server...");
  res = p2psrv.init(vm);
  CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize p2p server.");
  LOG_PRINT_L0("P2p server initialized OK on port: " << p2psrv.get_this_peer_port());

  tools::miniupnp_helper upnp_helper;

  if (!command_line::get_arg(vm, command_line::arg_disable_upnp))
  {
    LOG_PRINT_L0("Starting UPnP");
    upnp_helper.start_regular_mapping(p2psrv.get_this_peer_port(), p2psrv.get_this_peer_port(), 20*60*1000);
  }

  LOG_PRINT_L0("Initializing currency protocol...");
  res = cprotocol.init(vm);
  CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize currency protocol.");
  LOG_PRINT_L0("Currency protocol initialized OK");

  LOG_PRINT_L0("Initializing core rpc server...");
  res = rpc_server.init(vm);
  CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize core rpc server.");
  LOG_PRINT_GREEN("Core rpc server initialized OK on port: " << rpc_server.get_binded_port(), LOG_LEVEL_0);

  //initialize core here
  LOG_PRINT_L0("Initializing core...");
  res = ccore.init(vm);
  CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize core");
  LOG_PRINT_L0("Core initialized OK");

  if (stratum_enabled)
  {
    LOG_PRINT_L0("Initializing stratum server...");
    res = stratum_server_ptr->init(vm);
    CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize stratum server.");
  }



  auto& bcs = ccore.get_blockchain_storage();
  if (!offers_service.is_disabled() && bcs.get_current_blockchain_size() > 1 && bcs.get_top_block_id() != offers_service.get_last_seen_block_id())
  {
    res = resync_market(bcs, offers_service);
    CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize core: resync_market");
  }

  currency::checkpoints checkpoints;
  res = currency::create_checkpoints(checkpoints);
  CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize checkpoints");
  res = ccore.set_checkpoints(std::move(checkpoints));
  CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize core");

  // start components
  if (!command_line::has_arg(vm, command_line::arg_console))
  {
    dch.start_handling();
  }

  LOG_PRINT_L0("Starting core rpc server...");
  res = rpc_server.run(2, false);
  CHECK_AND_ASSERT_MES(res, 1, "Failed to initialize core rpc server.");
  LOG_PRINT_L0("Core rpc server started ok");

  //start stratum only after core got initialized
  if (stratum_enabled)
  {
    LOG_PRINT_L0("Starting stratum server...");
    res = stratum_server_ptr->run(false);
    CHECK_AND_ASSERT_MES(res, 1, "Failed to start stratum server.");
    LOG_PRINT_L0("Stratum server started ok");
  }

  tools::signal_handler::install([&dch, &p2psrv, &stratum_server_ptr] {
    dch.stop_handling();
    p2psrv.send_stop_signal();
    if (stratum_server_ptr)
      stratum_server_ptr->send_stop_signal();
  });

  LOG_PRINT_L0("Starting p2p net loop...");
  p2psrv.run();
  LOG_PRINT_L0("p2p net loop stopped");

  //stop components
  if (stratum_enabled)
  {
    LOG_PRINT_L0("Stopping stratum server...");
    stratum_server_ptr->send_stop_signal();
    stratum_server_ptr->timed_wait_server_stop(1000);
    LOG_PRINT_L0("Stratum server stopped");
  }

  LOG_PRINT_L0("Stopping core rpc server...");
  rpc_server.send_stop_signal();
  rpc_server.timed_wait_server_stop(5000);

  //deinitialize components
  LOG_PRINT_L0("Deinitializing core...");
  offers_service.set_last_seen_block_id(ccore.get_blockchain_storage().get_top_block_id());
  ccore.deinit();

  LOG_PRINT_L0("Deinitializing market...");
  (static_cast<currency::i_bc_service&>(offers_service)).deinit();

  LOG_PRINT_L0("Deinitializing stratum server ...");
  stratum_server_ptr.reset();

  LOG_PRINT_L0("Deinitializing rpc server ...");
  rpc_server.deinit();
  LOG_PRINT_L0("Deinitializing currency_protocol...");
  cprotocol.deinit();
  LOG_PRINT_L0("Deinitializing p2p...");
  p2psrv.deinit();

  ccore.set_critical_error_handler(nullptr);
  ccore.set_currency_protocol(NULL);
  cprotocol.set_p2p_endpoint(NULL);

  LOG_PRINT("Node stopped.", LOG_LEVEL_0);
  return EXIT_SUCCESS;

      CATCH_ENTRY_L0(__func__, EXIT_FAILURE);
    }
  catch (...)
    {
      return EXIT_FAILURE;
    }
}

bool command_line_preprocessor(const boost::program_options::variables_map& vm)
{
  bool exit = false;
  if (command_line::get_arg(vm, command_line::arg_version))
  {
    std::cout << CURRENCY_NAME  << " v" << PROJECT_VERSION_LONG << ENDL;
    exit = true;
  }

  if (command_line::get_arg(vm, command_line::arg_show_details))
  {
    currency::print_currency_details();
    exit = true;
  }
  if (command_line::get_arg(vm, command_line::arg_os_version))
  {
    std::cout << "OS: " << tools::get_os_version_string() << ENDL;
    exit = true;
  }

  if (exit)
  {
    return true;
  }

  if (command_line::has_arg(vm, command_line::arg_log_level))
  {
    int new_log_level = command_line::get_arg(vm, command_line::arg_log_level);
    if (new_log_level < LOG_LEVEL_MIN || new_log_level > LOG_LEVEL_MAX)
    {
      LOG_PRINT_L0("Wrong log level value: ");
    }
    else if (log_space::get_set_log_detalisation_level(false) != new_log_level)
    {
      log_space::get_set_log_detalisation_level(true, new_log_level);
      LOG_PRINT_L0("LOG_LEVEL set to " << new_log_level);
    }
  }

  return false;
}
