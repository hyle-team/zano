// Copyright (c) 2014-2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "plain_wallet_api.h"
#include "plain_wallet_api_impl.h"
#include "currency_core/currency_config.h"
#include "version.h"
#include "currency_core/currency_format_utils.h"
#include "wallets_manager.h"

//TODO: global objects, need refactoring. Just temporary solution
// std::map<int64_t, plain_wallet::plain_wallet_api_impl*> ginstances;
// epee::critical_section ginstances_lock;
// std::atomic<int64_t> gcounter(1);
//  std::atomic<bool> glogs_initialized(false);

#define HOME_FOLDER             "Documents"
#define WALLETS_FOLDER_NAME     "wallets"

#define GENERAL_INTERNAL_ERRROR_INSTANCE "GENERAL_INTERNAL_ERROR: WALLET INSTNACE NOT FOUND"
#define GENERAL_INTERNAL_ERRROR_INIT "Failed to intialize library"
// 
// #define GET_INSTANCE(var_name, instance_handle) plain_wallet_api_impl* var_name = nullptr;\
//   CRITICAL_REGION_BEGIN(ginstances_lock);\
//   auto it = ginstances.find(instance_handle);\
//   if (it == ginstances.end())\
//   {\
//     LOG_ERROR("Internall error: attempt to get instance wallet with wrong id: " << instance_handle);\
//     return GENERAL_INTERNAL_ERRROR_INSTANCE;\
//   }\
//   var_name = it->second;\
//   CRITICAL_REGION_END();


//TODO: global object, subject to refactoring
wallets_manager gwm;

namespace plain_wallet
{
  typedef epee::json_rpc::response<epee::json_rpc::dummy_result, error> error_response;

  std::string get_bundle_root_dir()
  {
    char buffer[1000] = {0};
    strcpy(buffer, getenv("HOME"));
    return buffer;
  }
  
  std::string get_wallets_folder()
  {
    std::string path = get_bundle_root_dir() + "/" + HOME_FOLDER + "/" + WALLETS_FOLDER_NAME;
    return path;
  }

  void initialize_logs()
  {
    std::string log_dir = get_bundle_root_dir();
    log_dir += "/" HOME_FOLDER;
    epee::log_space::get_set_log_detalisation_level(true, LOG_LEVEL_2);
    epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
    epee::log_space::log_singletone::add_logger(LOGGER_FILE, "plain_wallet.log", log_dir.c_str());
    LOG_PRINT_L0("Plain wallet initialized: " << CURRENCY_NAME << " v" << PROJECT_VERSION_LONG << ", log location: " << log_dir + "/plain_wallet.log");

    //glogs_initialized = true;
  }

  std::string init(const std::string& ip, const std::string& port)
  {
    initialize_logs();
    std::string argss_1 = std::string("--remote-nodes=") + ip + ":" + port;    
    char * args[] = {"", 0};
    args[1] = const_cast<char*>(argss_1.c_str());
    if (!gwm.init(2, args, nullptr))
    {
      LOG_ERROR("Failed to init wallets_manager");
      return GENERAL_INTERNAL_ERRROR_INIT;
    }
    
    if(!gwm.start())
    {
      LOG_ERROR("Failed to start wallets_manager");
      return GENERAL_INTERNAL_ERRROR_INIT;
    }

    std::string wallet_folder = get_wallets_folder();
    boost::system::error_code ec;
    boost::filesystem::create_directories(wallet_folder, ec);

    return API_RETURN_CODE_OK;
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

  std::string open(const std::string& path, const std::string& password)
  {
    epee::json_rpc::response<view::open_wallet_response, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
    std::string rsp = gwm.open_wallet(epee::string_encoding::convert_to_unicode(path), password, 20, ok_response.result);
    if (rsp == API_RETURN_CODE_OK || rsp == API_RETURN_CODE_FILE_RESTORED)
    {
      if (rsp == API_RETURN_CODE_FILE_RESTORED)
      {
        ok_response.result.recovered = true;
      }
      return epee::serialization::store_t_to_json(ok_response);
    }
    error_response err_result = AUTO_VAL_INIT(err_result);
    err_result.error.code = rsp;
    return epee::serialization::store_t_to_json(err_result);
  }
  std::string restore(const std::string& seed, const std::string& path, const std::string& password)
  {
    epee::json_rpc::response<view::open_wallet_response, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
    std::string rsp = gwm.restore_wallet(epee::string_encoding::convert_to_unicode(path), password, seed, ok_response.result);
    if (rsp == API_RETURN_CODE_OK || rsp == API_RETURN_CODE_FILE_RESTORED)
    {
      if (rsp == API_RETURN_CODE_FILE_RESTORED)
      {
        ok_response.result.recovered = true;
      }
      return epee::serialization::store_t_to_json(ok_response);
    }
    error_response err_result = AUTO_VAL_INIT(err_result);
    err_result.error.code = rsp;
    return epee::serialization::store_t_to_json(err_result);
  }

  std::string generate(hwallet h, const std::string& path, const std::string& password)
  {
    epee::json_rpc::response<view::open_wallet_response, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
    std::string rsp = gwm.generate_wallet(epee::string_encoding::convert_to_unicode(path), password, ok_response.result);
    if (rsp == API_RETURN_CODE_OK || rsp == API_RETURN_CODE_FILE_RESTORED)
    {
      if (rsp == API_RETURN_CODE_FILE_RESTORED)
      {
        ok_response.result.recovered = true;
      }
      return epee::serialization::store_t_to_json(ok_response);
    }
    error_response err_result = AUTO_VAL_INIT(err_result);
    err_result.error.code = rsp;
    return epee::serialization::store_t_to_json(err_result);
  }
//   std::string start_sync_thread(hwallet h)
//   {
//     GET_INSTANCE(pimpl, h);
//     pimpl->start_sync_thread();
//     return "";
//   }
//   std::string get_sync_status(hwallet h)
//   {
//     GET_INSTANCE(pimpl, h);
//     return pimpl->get_sync_status();
//   }
// 
//   std::string cancel_sync_thread(hwallet h)
//   {
//     GET_INSTANCE(pimpl, h);
//     return pimpl->cancel_sync_thread();
//   }
// 
//   std::string sync(hwallet h)
//   {
//     GET_INSTANCE(pimpl, h);
//     return pimpl->sync();
//   }
  std::string get_wallet_status(hwallet h)
  {
    return gwm.get_wallet_status(h);
  }
  std::string invoke(hwallet h, const std::string& params)
  {
    return gwm.invoke(h, params);
  }
}