// Copyright (c) 2014-2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "plain_wallet_api.h"
#include "plain_wallet_api_impl.h"

//TODO: global objects, need refactoring. Just temporary solution
std::map<int64_t, plain_wallet::plain_wallet_api_impl*> ginstances;
epee::critical_section ginstances_lock;
std::atomic<int64_t> gcounter = 1;

#define GENERAL_INTERNAL_ERRROR_INSTANCE "GENERAL_INTERNAL_ERROR: WALLET INSTNACE NOT FOUND"

#define GET_INSTANCE(var_name, instance_handle) plain_wallet_api_impl* var_name = nullptr;\
  CRITICAL_REGION_BEGIN(ginstances_lock);\
  auto it = ginstances.find(instance_handle);\
  if (it == ginstances.end())\
  {\
    LOG_ERROR("Internall error: attempt to delete wallet with wrong instance id: " << instance_handle);\
    return GENERAL_INTERNAL_ERRROR_INSTANCE;\
  }\
  var_name = it->second;\
  CRITICAL_REGION_END();


namespace plain_wallet
{
  hwallet create_instance(const std::string& ip, const std::string& port)
  {
    plain_wallet_api_impl* ptr = new plain_wallet_api_impl(ip, port);
    hwallet new_h = gcounter++;
    CRITICAL_REGION_BEGIN(ginstances_lock);     
    ginstances[new_h] = ptr;
    CRITICAL_REGION_END();
    return new_h;      
  }

  void destroy_instance(hwallet h)
  {
    plain_wallet_api_impl* instance_ptr = nullptr;
    CRITICAL_REGION_BEGIN(ginstances_lock);
    auto it = ginstances.find(h);
    if (it == ginstances.end())
    {
      LOG_ERROR("Internall error: attempt to delete wallet with wrong instance id: " << h);
    }
    instance_ptr = it->second;
    ginstances.erase(it);
    CRITICAL_REGION_END();
    delete instance_ptr; 
  }
  std::string open(hwallet h, const std::string& path, const std::string& password)
  {
    GET_INSTANCE(pimpl, h);
    return pimpl->open(path, password);
  }
  std::string restore(hwallet h, const std::string& seed, const std::string& path, const std::string& password)
  {
    GET_INSTANCE(pimpl, h);
    return pimpl->restore(seed, path, password);
  }
  std::string generate(hwallet h, const std::string& path, const std::string& password)
  {
    GET_INSTANCE(pimpl, h);
    return pimpl->generate(path, password);
  }
  std::string start_sync_thread(hwallet h)
  {
    GET_INSTANCE(pimpl, h);
    pimpl->start_sync_thread();
    return "";
  }
  std::string get_sync_status(hwallet h)
  {
    GET_INSTANCE(pimpl, h);
    return pimpl->get_sync_status();
  }
  std::string sync(hwallet h)
  {
    GET_INSTANCE(pimpl, h);
    return pimpl->sync();
  }
  std::string invoke(hwallet h, const std::string& params)
  {
    GET_INSTANCE(pimpl, h);
    return pimpl->invoke(params);
  }
}