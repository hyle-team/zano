// Copyright (c) 2014-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "command_line.h"
#include "currency_core/currency_config.h"

namespace command_line
{
  const arg_descriptor<bool>		arg_help = {"help", "Produce help message"};
  const arg_descriptor<bool>		arg_version = {"version", "Output version information"};
  const arg_descriptor<std::string> arg_data_dir = {"data-dir", "Specify data directory"};

  const arg_descriptor<std::string> arg_config_file =  { "config-file", "Specify configuration file", std::string(CURRENCY_NAME_SHORT ".conf") };
  const arg_descriptor<bool>        arg_os_version =   { "os-version", "" };

  const arg_descriptor<std::string> arg_log_dir =      { "log-dir", "", "", true};
  const arg_descriptor<std::string> arg_log_file =     { "log-file", "", "" };
  const arg_descriptor<int>         arg_log_level =    { "log-level", "", LOG_LEVEL_0, true };

  const arg_descriptor<bool>        arg_console =      { "no-console", "Disable daemon console commands" };
  const arg_descriptor<bool>        arg_show_details = { "currency-details", "Display currency details" };
  const arg_descriptor<bool>        arg_show_rpc_autodoc = { "show_rpc_autodoc", "Display rpc auto-generated documentation template" };

  const arg_descriptor<bool>        arg_disable_upnp = { "disable-upnp", "Disable UPnP (enhances local network privacy)", false, true };

  const arg_descriptor<bool>        arg_disable_stop_if_time_out_of_sync = { "disable-stop-if-time-out-of-sync", "Do not stop the daemon if serious time synchronization problem is detected", false, true };
}
