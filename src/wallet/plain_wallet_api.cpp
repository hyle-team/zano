// Copyright (c) 2014-2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "plain_wallet_api.h"
#include "plain_wallet_api_impl.h"

namespace plain_wallet
{
  hwallet create_instance(const std::string port, const std::string ip)
  {
    return new plain_wallet_api_impl(port, ip);
  }
  void destroy_instance(hwallet h)
  {
    delete ((plain_wallet_api_impl*)h);
  }
  std::string open(hwallet h, const std::string& path, const std::string password)
  {
    plain_wallet_api_impl* pimpl = (plain_wallet_api_impl*)h;
    return pimpl->open(path, password);
  }
  std::string restore(hwallet h, const std::string& seed, const std::string& path, const std::string password)
  {
    plain_wallet_api_impl* pimpl = (plain_wallet_api_impl*)h;
    return pimpl->restore(seed, path, password);
  }
  std::string generate(hwallet h, const std::string& path, const std::string password)
  {
    plain_wallet_api_impl* pimpl = (plain_wallet_api_impl*)h;
    return pimpl->generate(path, password);
  }
  void start_sync_thread(hwallet h)
  {
    plain_wallet_api_impl* pimpl = (plain_wallet_api_impl*)h;
    pimpl->start_sync_thread();
  }
  std::string get_sync_status(hwallet h)
  {
    plain_wallet_api_impl* pimpl = (plain_wallet_api_impl*)h;
    pimpl->get_sync_status();
  }
  std::string sync(hwallet h)
  {
    plain_wallet_api_impl* pimpl = (plain_wallet_api_impl*)h;
    pimpl->sync();
  }
  std::string invoke(hwallet h, const std::string& params)
  {
    plain_wallet_api_impl* pimpl = (plain_wallet_api_impl*)h;
    pimpl->invoke(params);
  }
}