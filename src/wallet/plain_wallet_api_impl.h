// Copyright (c) 2014-2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#pragma once

#include <string>
#include "wallet2.h"
#include "wallet_rpc_server.h"
#include "plain_wallet_api_defs.h"

namespace plain_wallet
{
  class plain_wallet_api_impl
  {
  public: 
    plain_wallet_api_impl(const std::string& ip, const std::string& port);
    ~plain_wallet_api_impl();
    std::string open(const std::string& path, const std::string& password);
    std::string restore(const std::string& seed, const std::string& path, const std::string& password);
    std::string generate(const std::string& path, const std::string& password);

    std::string start_sync_thread();
    std::string cancel_sync_thread();
    std::string get_sync_status();

    std::string sync();
    std::string invoke(const std::string& params);
  private: 
    bool get_wallet_info(view::wallet_info& wi);
    std::thread m_sync_thread;
    std::atomic<bool> m_stop;
    std::atomic<bool> m_sync_finished;
    std::shared_ptr<tools::wallet2> m_wallet;
    std::shared_ptr<tools::wallet_rpc_server> m_rpc_wrapper;
  };

}