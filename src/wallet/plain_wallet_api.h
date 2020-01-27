// Copyright (c) 2014-2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#pragma once

#include <string>

namespace plain_wallet
{
  typedef void* hwallet;
  hwallet create_instance(const std::string ip, const std::string port);
  void destroy_instance(hwallet h);

  std::string open(hwallet h, const std::string& path, const std::string password);
  std::string restore(hwallet h, const std::string& seed, const std::string& path, const std::string password);
  std::string generate(hwallet h, const std::string& path, const std::string password);

  void start_sync_thread(hwallet h);
  std::string get_sync_status(hwallet h);
  std::string sync(hwallet h);
  std::string invoke(hwallet h, const std::string& params);
}