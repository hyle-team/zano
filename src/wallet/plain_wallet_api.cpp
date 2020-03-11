// Copyright (c) 2014-2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "plain_wallet_api.h"
#include "plain_wallet_api_impl.h"
#include "currency_core/currency_config.h"
#include "version.h"
#include "string_tools.h"
#include "currency_core/currency_format_utils.h"
#include "wallets_manager.h"

#define ANDROID_PACKAGE_NAME    "com.zano_mobile"
#ifdef IOS_BUILD
  #define HOME_FOLDER             "Documents"
#elif ANDROID_BUILD
  #define HOME_FOLDER             "files"
#else 
  #define HOME_FOLDER             ""
#endif
#define WALLETS_FOLDER_NAME     "wallets"

#define GENERAL_INTERNAL_ERRROR_INSTANCE "GENERAL_INTERNAL_ERROR: WALLET INSTNACE NOT FOUND"
#define GENERAL_INTERNAL_ERRROR_INIT "Failed to intialize library"

//TODO: global object, subject to refactoring
wallets_manager gwm;
std::atomic<bool> initialized(false);

std::atomic<uint64_t> gjobs_counter(1);
std::map<uint64_t, std::string> gjobs;
epee::critical_section gjobs_lock;

namespace plain_wallet
{
  typedef epee::json_rpc::response<epee::json_rpc::dummy_result, error> error_response;

  std::string get_bundle_root_dir()
  {
#ifdef WIN32
    return "";
#elif IOS_BUILD
    char* env = getenv("HOME");
    return env ? env : "";
#elif ANDROID_BUILD
    ///      data/data/com.zano_mobile/files
    return "/data/data/" ANDROID_PACKAGE_NAME;
#else
    return "";
#endif
  }
  
  std::string get_wallets_folder()
  {
#ifdef WIN32
    return "";
#else
    std::string path = get_bundle_root_dir() + "/" + HOME_FOLDER + "/" + WALLETS_FOLDER_NAME + "/";
    return path;
#endif // WIN32
  }

  void initialize_logs(int log_level)
  {
    std::string log_dir = get_bundle_root_dir();
    log_dir += "/" HOME_FOLDER;
    epee::log_space::get_set_log_detalisation_level(true, log_level);
    epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
    epee::log_space::log_singletone::add_logger(LOGGER_FILE, "plain_wallet.log", log_dir.c_str());
    LOG_PRINT_L0("Plain wallet initialized: " << CURRENCY_NAME << " v" << PROJECT_VERSION_LONG << ", log location: " << log_dir + "/plain_wallet.log");

    //glogs_initialized = true;
  }

  std::string set_log_level(int log_level)
  {
    epee::log_space::get_set_log_detalisation_level(true, log_level);
    return "{}";
  }

  std::string init(const std::string& ip, const std::string& port, int log_level)
  {
    if (initialized)
    {
      LOG_ERROR("Double-initialization in plain_wallet detected.");
      //throw std::runtime_error("Double-initialization in plain_wallet detected.");
      return "Already initialized!";
    }
      

    initialize_logs(log_level);
    std::string argss_1 = std::string("--remote-node=") + ip + ":" + port;    
    char * args[3];
    args[0] = "stub";
    args[1] = const_cast<char*>(argss_1.c_str());
    args[2] = nullptr;
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
    initialized = true;
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
    std::string full_path = get_wallets_folder() + path;
    epee::json_rpc::response<view::open_wallet_response, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
    std::string rsp = gwm.open_wallet(epee::string_encoding::convert_to_unicode(full_path), password, 20, ok_response.result);
    if (rsp == API_RETURN_CODE_OK || rsp == API_RETURN_CODE_FILE_RESTORED)
    {
      if (rsp == API_RETURN_CODE_FILE_RESTORED)
      {
        ok_response.result.recovered = true;
      }
      gwm.run_wallet(ok_response.result.wallet_id);

      return epee::serialization::store_t_to_json(ok_response);
    }
    error_response err_result = AUTO_VAL_INIT(err_result);
    err_result.error.code = rsp;
    return epee::serialization::store_t_to_json(err_result);
  }
  std::string restore(const std::string& seed, const std::string& path, const std::string& password)
  {
    std::string full_path = get_wallets_folder() + path;
    epee::json_rpc::response<view::open_wallet_response, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
    std::string rsp = gwm.restore_wallet(epee::string_encoding::convert_to_unicode(full_path), password, seed, ok_response.result);
    if (rsp == API_RETURN_CODE_OK || rsp == API_RETURN_CODE_FILE_RESTORED)
    {
      if (rsp == API_RETURN_CODE_FILE_RESTORED)
      {
        ok_response.result.recovered = true;
      }
      gwm.run_wallet(ok_response.result.wallet_id);
      return epee::serialization::store_t_to_json(ok_response);
    }
    error_response err_result = AUTO_VAL_INIT(err_result);
    err_result.error.code = rsp;
    return epee::serialization::store_t_to_json(err_result);
  }

  std::string generate(const std::string& path, const std::string& password)
  {
    std::string full_path = get_wallets_folder() + path;
    epee::json_rpc::response<view::open_wallet_response, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
    std::string rsp = gwm.generate_wallet(epee::string_encoding::convert_to_unicode(full_path), password, ok_response.result);
    if (rsp == API_RETURN_CODE_OK || rsp == API_RETURN_CODE_FILE_RESTORED)
    {
      if (rsp == API_RETURN_CODE_FILE_RESTORED)
      {
        ok_response.result.recovered = true;
      }
      gwm.run_wallet(ok_response.result.wallet_id);
      return epee::serialization::store_t_to_json(ok_response);
    }
    error_response err_result = AUTO_VAL_INIT(err_result);
    err_result.error.code = rsp;
    return epee::serialization::store_t_to_json(err_result);
  }

  std::string close_wallet(hwallet h)
  {
    std::string r = "{\"response\": \"";
    r += gwm.close_wallet(h);
    r += "\"}";
    return r;
  }

  std::string get_wallet_status(hwallet h)
  {
    return gwm.get_wallet_status(h);
  }
  std::string invoke(hwallet h, const std::string& params)
  {
    return gwm.invoke(h, params);
  }

  void put_result(uint64_t job_id, const std::string& res)
  {
    CRITICAL_REGION_LOCAL(gjobs_lock);
    gjobs[job_id] = res;
    LOG_PRINT_L0("[ASYNC_CALL]: Finished(result put), job id: " << job_id);
  }


  std::string async_call(const std::string& method_name, uint64_t instance_id, const std::string& params)
  {
    std::function<void()> async_callback;

    uint64_t job_id = gjobs_counter++;
    if (method_name == "close")
    {
      async_callback = [job_id, instance_id]()
      {
        close_wallet(instance_id);
        view::api_responce_return_code rc = AUTO_VAL_INIT(rc);
        rc.return_code = API_RETURN_CODE_OK;
        put_result(job_id, epee::serialization::store_t_to_json(rc));        
      };
    }
    else if (method_name == "open")
    {
      view::open_wallet_request owr = AUTO_VAL_INIT(owr);
      if (!epee::serialization::load_t_from_json(owr, params))
      {
        view::api_response ar = AUTO_VAL_INIT(ar);
        ar.error_code = "Wrong parameter";
        put_result(job_id, epee::serialization::store_t_to_json(ar));
      }
      async_callback = [job_id, owr]()
      {
        std::string res = open(owr.path, owr.pass);
        put_result(job_id, res);
      };
    }
    else if (method_name == "restore")
    {
      view::restore_wallet_request rwr = AUTO_VAL_INIT(rwr);
      if (!epee::serialization::load_t_from_json(rwr, params))
      {
        view::api_response ar = AUTO_VAL_INIT(ar);
        ar.error_code = "Wrong parameter";
        put_result(job_id, epee::serialization::store_t_to_json(ar));
      }
      async_callback = [job_id, rwr]()
      {
        std::string res = restore(rwr.restore_key, rwr.path, rwr.pass);
        put_result(job_id, res);
      };
    }
    else if (method_name == "invoke")
    {
      std::string local_params = params;
      async_callback = [job_id, local_params, instance_id]()
      {
        std::string res = invoke(instance_id, local_params);
        put_result(job_id, res);
      };
    }
    else
    {
      view::api_response ar = AUTO_VAL_INIT(ar);
      ar.error_code = "UNKNOWN METHOD";
      put_result(job_id, epee::serialization::store_t_to_json(ar));
      return std::string("{ \"job_id\": ") + std::to_string(job_id) + "}";;
    }


    std::thread t([async_callback]() {async_callback(); });
    t.detach();
    LOG_PRINT_L0("[ASYNC_CALL]: started " << method_name << ", job id: " << job_id);
    return std::string("{ \"job_id\": ") + std::to_string(job_id) + "}";
  }

  std::string try_pull_result(uint64_t job_id)
  {
    //TODO: need refactoring
    CRITICAL_REGION_LOCAL(gjobs_lock);
    auto it = gjobs.find(job_id);
    if (it == gjobs.end())
    {
      return "{\"delivered\": false}";
    }
    std::string res = "{\"delivered\": true, \"result\": ";
    res += it->second;
    res += "  }";
    gjobs.erase(it);
    return res;
  }

}