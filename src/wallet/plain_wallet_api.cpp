// Copyright (c) 2014-2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "plain_wallet_api.h"
#include "plain_wallet_api_defs.h"
#include "currency_core/currency_config.h"
#include "version.h"
#include "string_tools.h"
#include "currency_core/currency_format_utils.h"
#include "wallets_manager.h"
#include "common/base58.h"
#include "common/config_encrypt_helper.h"

#define ANDROID_PACKAGE_NAME    "com.zano_mobile"

#define LOGS_FOLDER             "logs"

#define WALLETS_FOLDER_NAME     "wallets"
#define APP_CONFIG_FOLDER       "app_config"
#define APP_CONFIG_FILENAME     "app_cfg.bin"

#define  MOBILE_APP_DATA_FILE_BINARY_SIGNATURE   0x1000111201101011LL //Bender's nightmare

#define GENERAL_INTERNAL_ERRROR_INSTANCE "GENERAL_INTERNAL_ERROR: WALLET INSTNACE NOT FOUND"
#define GENERAL_INTERNAL_ERRROR_INIT "Failed to intialize library"

//TODO: global objects, subject to refactoring
wallets_manager gwm;
std::atomic<bool> initialized(false);

std::atomic<uint64_t> gjobs_counter(1);
std::map<uint64_t, std::string> gjobs;
epee::critical_section gjobs_lock;
std::string gconfig_folder;

namespace plain_wallet
{
  typedef epee::json_rpc::response<epee::json_rpc::dummy_result, error> error_response;

  std::string get_bundle_working_dir()
  {
    return gconfig_folder;
// #ifdef WIN32
//     return boost::dll::program_location().parent_path().string();
// #elif IOS_BUILD
//     char* env = getenv("HOME");
//     return env ? env : "";
// #elif ANDROID_BUILD
//     ///      data/data/com.zano_mobile/files
//     return "/data/data/" ANDROID_PACKAGE_NAME;
// #else
//     return "";
// #endif
  }
  void set_bundle_working_dir(const std::string& dir)
  {
    gconfig_folder = dir;
  }
  
  std::string get_wallets_folder()
  {
#ifdef WIN32
    return "";
#else
    std::string path = get_bundle_working_dir() + "/" + WALLETS_FOLDER_NAME + "/";
    return path;
#endif // WIN32
  }

  std::string get_app_config_folder()
  {
#ifdef WIN32
    return "";
#else
    std::string path = get_bundle_working_dir() + "/" + APP_CONFIG_FOLDER + "/";
    return path;
#endif // WIN32
  }

  

  void initialize_logs(int log_level)
  {
    std::string log_dir = get_bundle_working_dir();
    log_dir += "/" LOGS_FOLDER;

    log_space::get_set_need_thread_id(true, true);
    log_space::log_singletone::enable_channels("core,currency_protocol,tx_pool,p2p,wallet");
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

  std::string init(const std::string& ip, const std::string& port, const std::string& working_dir, int log_level)
  {
    if (initialized)
    {
      LOG_ERROR("Double-initialization in plain_wallet detected.");
      epee::json_rpc::response<view::api_responce_return_code, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
      ok_response.result.return_code = API_RETURN_CODE_ALREADY_EXISTS;
      return epee::serialization::store_t_to_json(ok_response);
    }
    set_bundle_working_dir(working_dir);

    initialize_logs(log_level);
    std::string argss_1 = std::string("--remote-node=") + ip + ":" + port;    
    std::string argss_2 = std::string("--disable-logs-init");
    char * args[4];
    static const char* arg0_stub = "stub";
    args[0] = const_cast<char*>(arg0_stub);
    args[1] = const_cast<char*>(argss_1.c_str());
    args[2] = const_cast<char*>(argss_2.c_str());
    args[3] = nullptr;
    if (!gwm.init(3, args, nullptr))
    {
      LOG_ERROR("Failed to init wallets_manager");
      return GENERAL_INTERNAL_ERRROR_INIT;
    }
    
    if(!gwm.start())
    {
      LOG_ERROR("Failed to start wallets_manager");
      return GENERAL_INTERNAL_ERRROR_INIT;
    }

    std::string wallets_folder = get_wallets_folder();
    boost::system::error_code ec;
    boost::filesystem::create_directories(wallets_folder, ec);
    if (ec)
    {
      error_response err_result = AUTO_VAL_INIT(err_result);
      err_result.error.code = API_RETURN_CODE_INTERNAL_ERROR;
      err_result.error.message = LOCATION_STR + " \nmessage:" + ec.message();
      return epee::serialization::store_t_to_json(err_result);
    }

    std::string app_config_folder = get_app_config_folder();
    boost::filesystem::create_directories(app_config_folder, ec);
    if (ec)
    {
      error_response err_result = AUTO_VAL_INIT(err_result);
      err_result.error.code = API_RETURN_CODE_INTERNAL_ERROR;
      err_result.error.message = LOCATION_STR + " \nmessage:" + ec.message();
      return epee::serialization::store_t_to_json(err_result);
    }

    initialized = true;
    epee::json_rpc::response<view::api_responce_return_code, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
    ok_response.result.return_code = API_RETURN_CODE_OK;
    return epee::serialization::store_t_to_json(ok_response);
  }

  std::string get_appconfig(const std::string& encryption_key)
  {
    std::string res_str;
    std::string app_config_config_path = get_app_config_folder() + APP_CONFIG_FILENAME;
    std::string ret_code = tools::load_encrypted_file(app_config_config_path, encryption_key, res_str, MOBILE_APP_DATA_FILE_BINARY_SIGNATURE);
    if (ret_code != API_RETURN_CODE_OK)
    {
      error_response err_result = AUTO_VAL_INIT(err_result);
      err_result.error.code = ret_code;
      return epee::serialization::store_t_to_json(err_result);
    }
    return res_str;
  }
  std::string set_appconfig(const std::string& conf_str, const std::string& encryption_key)
  {
    std::string app_config_config_path = get_app_config_folder() + APP_CONFIG_FILENAME;
    std::string ret_code = tools::store_encrypted_file(app_config_config_path, encryption_key, conf_str, MOBILE_APP_DATA_FILE_BINARY_SIGNATURE);
    if (ret_code != API_RETURN_CODE_OK)
    {
      error_response err_result = AUTO_VAL_INIT(err_result);
      err_result.error.code = ret_code;
      return epee::serialization::store_t_to_json(err_result);
    }
    else
    {
      epee::json_rpc::response<view::api_responce_return_code, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
      ok_response.result.return_code = API_RETURN_CODE_OK;
      return epee::serialization::store_t_to_json(ok_response);
    }
  }

  std::string generate_random_key(uint64_t lenght)
  {
    std::string buff; 
    buff.resize(lenght);
    crypto::generate_random_bytes(lenght, const_cast<char*>(buff.data()));
    return tools::base58::encode(buff);
  }

  std::string get_logs_buffer()
  {
    return epee::log_space::log_singletone::copy_logs_to_buffer();
  }

  std::string truncate_log()
  {
    epee::log_space::log_singletone::truncate_log_files();
    epee::json_rpc::response<view::api_responce_return_code, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
    ok_response.result.return_code = API_RETURN_CODE_OK;
    return epee::serialization::store_t_to_json(ok_response);
  }

  std::string get_connectivity_status()
  {
    return gwm.get_connectivity_status();
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
    else if (method_name == "get_wallet_status")
    {
      std::string local_params = params;
      async_callback = [job_id, local_params, instance_id]()
      {
        std::string res = get_wallet_status(instance_id);
        put_result(job_id, res);
      };
    }else
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