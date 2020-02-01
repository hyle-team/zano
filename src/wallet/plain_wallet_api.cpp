// Copyright (c) 2014-2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "plain_wallet_api.h"
#include "plain_wallet_api_impl.h"
#include "currency_core/currency_config.h"
#include "version.h"

//TODO: global objects, need refactoring. Just temporary solution
std::map<int64_t, plain_wallet::plain_wallet_api_impl*> ginstances;
epee::critical_section ginstances_lock;
std::atomic<int64_t> gcounter(1);
std::atomic<bool> glogs_initialized(false);

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
  std::string get_bundle_root_dir()
  {
    char buffer[1000] = {0};
    strcpy(buffer, getenv("HOME"));
    return buffer;
  }
  
  std::string get_wallets_folder()
  {
    return get_bundle_root_dir() + "/Documents";
  }

  void initialize_logs()
  {
    std::string log_dir = get_bundle_root_dir();
    log_dir += "/Documents";
    epee::log_space::get_set_log_detalisation_level(true, LOG_LEVEL_2);
    epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
    epee::log_space::log_singletone::add_logger(LOGGER_FILE, "plain_wallet.log", log_dir.c_str());
    LOG_PRINT_L0("Plain wallet initialized: " <<  CURRENCY_NAME << " v" << PROJECT_VERSION_LONG << ", log location: " << log_dir + "/plain_wallet.log");

    glogs_initialized = true;
  }

  std::string get_version()
  {
    return PROJECT_VERSION_LONG;
  }

  struct strings_list
  {
    std::list<std::string> items;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(items)
    END_KV_SERIALIZE_MAP()

  };

  std::string get_wallet_files()
  {
    std::string wallet_files_path = get_wallets_folder();
    strings_list sl = AUTO_VAL_INIT(sl);
    epee::file_io_utils::get_folder_content(wallet_files_path, sl.items, true);
    return epee::serialization::store_t_to_json(sl);
  }

  hwallet create_instance(const std::string& ip, const std::string& port)
  {
    if (!glogs_initialized)
      initialize_logs();

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
    std::string full_path = get_wallets_folder() + "/" + path;
    return pimpl->open(full_path, password);
  }
  std::string restore(hwallet h, const std::string& seed, const std::string& path, const std::string& password)
  {
    GET_INSTANCE(pimpl, h);
    std::string full_path = get_wallets_folder() + "/" + path;
    return pimpl->restore(seed, full_path, password);
  }
  std::string generate(hwallet h, const std::string& path, const std::string& password)
  {
    GET_INSTANCE(pimpl, h);
    std::string full_path = get_wallets_folder() + "/" + path;
    return pimpl->generate(full_path, password);
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

  std::string cancel_sync_thread(hwallet h)
  {
    GET_INSTANCE(pimpl, h);
    return pimpl->cancel_sync_thread();
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