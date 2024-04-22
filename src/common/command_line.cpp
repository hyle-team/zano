// Copyright (c) 2014-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "command_line.h"
#include "currency_core/currency_config.h"

namespace command_line
{
  const arg_descriptor<bool>		arg_help  ("help", "Produce help message");
  const arg_descriptor<bool>		arg_version  ("version", "Output version information");
  const arg_descriptor<std::string> arg_data_dir  ("data-dir", "Specify data directory", "");
  
  const arg_descriptor<int>         arg_stop_after_height  ( "stop-after-height", "If specified, the daemon will stop immediately after a block with the given height is added", 0 );

  const arg_descriptor<std::string> arg_config_file   ( "config-file", "Specify configuration file", std::string(CURRENCY_NAME_SHORT ".conf") );
  const arg_descriptor<bool>        arg_os_version    ( "os-version", "" );

  const arg_descriptor<std::string> arg_log_dir       ( "log-dir", "");
  const arg_descriptor<std::string> arg_log_file      ( "log-file", "", "");
  const arg_descriptor<int>         arg_log_level     ( "log-level", "");

  const arg_descriptor<bool>        arg_console       ( "no-console", "Disable daemon console commands" );
  const arg_descriptor<bool>        arg_show_details  ( "currency-details", "Display currency details" );
  const arg_descriptor<std::string> arg_generate_rpc_autodoc  ( "generate-rpc-autodoc", "Make auto-generated RPC API documentation documents at the given path" );

  const arg_descriptor<bool>        arg_disable_upnp  ( "disable-upnp", "Disable UPnP (enhances local network privacy)");
  const arg_descriptor<bool>        arg_disable_ntp  ( "disable-ntp", "Disable NTP, could enhance to time synchronization issue but increase network privacy, consider using disable-stop-if-time-out-of-sync with it");

  const arg_descriptor<bool>        arg_disable_stop_if_time_out_of_sync  ( "disable-stop-if-time-out-of-sync", "Do not stop the daemon if serious time synchronization problem is detected");
  const arg_descriptor<bool>        arg_disable_stop_on_low_free_space    ( "disable-stop-on-low-free-space", "Do not stop the daemon if free space at data dir is critically low");
  const arg_descriptor<bool>        arg_enable_offers_service  ( "enable-offers-service", "Enables marketplace feature", false);
  const arg_descriptor<std::string> arg_db_engine              ( "db-engine", "Specify database engine for storage. May be \"lmdb\"(default) or \"mdbx\"", ARG_DB_ENGINE_LMDB );

  const arg_descriptor<bool>        arg_no_predownload        ( "no-predownload", "Do not pre-download blockchain database");
  const arg_descriptor<bool>        arg_force_predownload     ( "force-predownload", "Pre-download blockchain database regardless of it's status");
  const arg_descriptor<std::string> arg_process_predownload_from_path("predownload-from-local-path", "Instead of downloading file use downloaded local file");
  const arg_descriptor<bool>        arg_validate_predownload  ( "validate-predownload", "Paranoid mode, re-validate each block from pre-downloaded database and rebuild own database");
  const arg_descriptor<std::string> arg_predownload_link      ( "predownload-link", "Override url for blockchain database pre-downloading");

  const arg_descriptor<std::string> arg_deeplink  ( "deeplink-params", "Deeplink parameter, in that case app just forward params to running app");

}
