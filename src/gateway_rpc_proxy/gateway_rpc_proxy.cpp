// Copyright (c) 2014-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "include_base_utils.h"
using namespace epee;

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <cstdlib>

#include "version.h"
#include "common/command_line.h"
#include "common/util.h"
#include "common/callstack_helper.h"
#include "currency_core/currency_config.h"
#include "gateway_rpc_proxy_server.h"

#if defined(WIN32)
#include <crtdbg.h>
#endif

namespace po = boost::program_options;

namespace
{
  void terminate_handler_func()
  {
    LOG_ERROR("\n\nTERMINATE HANDLER (gateway_rpc_proxy)\n");
    std::fflush(nullptr);
    std::abort();
  }
}

int main(int argc, char* argv[])
{
  TRY_ENTRY();

  string_tools::set_module_name_and_folder(argv[0]);
#ifdef WIN32
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_0);
  log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
  log_space::log_singletone::enable_channels("gateway_rpc_proxy");

  tools::signal_handler::install_fatal([](int sig_number, void* address) {
    LOG_ERROR("\n\nFATAL ERROR\nsig: " << sig_number << ", address: " << address);
    std::fflush(nullptr);
  });
  epee::misc_utils::get_callstack(tools::get_callstack);
  std::set_terminate(&terminate_handler_func);

  po::options_description desc_cmd_only("Command line options");
  po::options_description desc_cmd_sett("Command line options and settings options", 130, 83);

  command_line::add_arg(desc_cmd_only, command_line::arg_help);
  command_line::add_arg(desc_cmd_only, command_line::arg_version);
  command_line::add_arg(desc_cmd_only, command_line::arg_config_file);

  command_line::add_arg(desc_cmd_sett, command_line::arg_data_dir, tools::get_default_data_dir());
  command_line::add_arg(desc_cmd_sett, command_line::arg_log_dir);
  command_line::add_arg(desc_cmd_sett, command_line::arg_log_level);

  gateway_rpc_proxy::gateway_rpc_proxy_server::init_options(desc_cmd_sett);

  po::options_description desc_options("Allowed options");
  desc_options.add(desc_cmd_only).add(desc_cmd_sett);

  std::string data_dir;
  po::variables_map vm;
  bool exit_requested = false;
  bool r = command_line::handle_error_helper(desc_options, [&]()
  {
    po::store(po::parse_command_line(argc, argv, desc_options), vm);

    if (command_line::get_arg(vm, command_line::arg_help))
    {
      std::cout << CURRENCY_NAME << " gateway_rpc_proxy v" << PROJECT_VERSION_LONG << ENDL << ENDL;
      std::cout << desc_options << std::endl;
      exit_requested = true;
      return true;
    }
    if (command_line::get_arg(vm, command_line::arg_version))
    {
      std::cout << CURRENCY_NAME << " gateway_rpc_proxy v" << PROJECT_VERSION_LONG << ENDL;
      exit_requested = true;
      return true;
    }

    data_dir = command_line::get_arg(vm, command_line::arg_data_dir);

    std::string config = command_line::get_arg(vm, command_line::arg_config_file);
    boost::filesystem::path data_dir_path(data_dir);
    boost::filesystem::path config_path(config);
    if (!config_path.has_parent_path())
      config_path = data_dir_path / config_path;
    boost::system::error_code ec;
    if (boost::filesystem::exists(config_path, ec))
    {
      po::store(po::parse_config_file<char>(config_path.string().c_str(), desc_cmd_sett), vm);
    }
    po::notify(vm);
    return true;
  });
  if (!r)
    return EXIT_FAILURE;
  if (exit_requested)
    return EXIT_SUCCESS;

  std::string log_dir = data_dir;
  if (command_line::has_arg(vm, command_line::arg_log_dir))
  {
    log_dir = command_line::get_arg(vm, command_line::arg_log_dir);
  }
  log_space::log_singletone::add_logger(LOGGER_FILE, log_space::log_singletone::get_default_log_file().c_str(), log_dir.c_str());
  LOG_PRINT_L0(CURRENCY_NAME << " gateway_rpc_proxy v" << PROJECT_VERSION_LONG);

  if (command_line::has_arg(vm, command_line::arg_log_level))
  {
    int new_log_level = command_line::get_arg(vm, command_line::arg_log_level);
    if (new_log_level >= LOG_LEVEL_MIN && new_log_level <= LOG_LEVEL_MAX)
    {
      log_space::get_set_log_detalisation_level(true, new_log_level);
      LOG_PRINT_L0("LOG_LEVEL set to " << new_log_level);
    }
    else
    {
      LOG_PRINT_L0("Wrong log level value: " << new_log_level);
    }
  }

  gateway_rpc_proxy::gateway_rpc_proxy_server srv;
  if (!srv.init(vm))
  {
    LOG_ERROR("Failed to initialize gateway_rpc_proxy rpc server");
    return EXIT_FAILURE;
  }

  tools::signal_handler::install([&srv] {
    srv.send_stop_signal();
  });

  LOG_PRINT_GREEN("gateway_rpc_proxy rpc server started, listening on port " << srv.get_binded_port(), LOG_LEVEL_0);
  srv.run(GATEWAY_RPC_PROXY_DEFAULT_RPC_THREADS, true);
  LOG_PRINT_L0("gateway_rpc_proxy rpc server stopped");

  srv.deinit();
  return EXIT_SUCCESS;

  CATCH_ENTRY_L0(LOCATION_SS, EXIT_FAILURE);
}
