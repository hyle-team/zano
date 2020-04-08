// Copyright (c) 2014-2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#pragma once

#include <string>

namespace plain_wallet
{
  typedef int64_t hwallet;
  std::string init(const std::string& ip, const std::string& port, const std::string& working_dir, int log_level);
  std::string set_log_level(int log_level);
  std::string get_version();
  std::string get_wallet_files();

  std::string get_appconfig(const std::string& encryption_key);
  std::string set_appconfig(const std::string& conf_str, const std::string& encryption_key);
  std::string generate_random_key(uint64_t lenght);
  std::string get_logs_buffer();
  std::string truncate_log();
  std::string get_connectivity_status();

  std::string open(const std::string& path, const std::string& password);
  std::string restore(const std::string& seed, const std::string& path, const std::string& password);
  std::string generate(const std::string& path, const std::string& password);

  std::string get_wallet_status(hwallet h);
  std::string close_wallet(hwallet h);
  std::string invoke(hwallet h, const std::string& params);
  
  //async api
  std::string async_call(const std::string& method_name, uint64_t instance_id, const std::string& params);
  std::string try_pull_result(uint64_t);
}