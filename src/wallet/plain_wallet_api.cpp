// Copyright (c) 2014-2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifdef ANDROID_BUILD
  #include <android/log.h>
#endif
#include "plain_wallet_api.h"
#include "plain_wallet_api_defs.h"
#include "currency_core/currency_config.h"
#include "version.h"
#include "string_tools.h"
#include "currency_core/currency_format_utils.h"
#include "wallets_manager.h"
#include "common/base58.h"
#include "common/config_encrypt_helper.h"
#include "static_helpers.h"
#include "wallet_helpers.h"

#define ANDROID_PACKAGE_NAME    "com.zano_mobile"

#define LOGS_FOLDER             "logs"

#define WALLETS_FOLDER_NAME     "wallets"
#define APP_CONFIG_FOLDER       "app_config"
#define APP_CONFIG_FILENAME     "app_cfg.bin"

#define  MOBILE_APP_DATA_FILE_BINARY_SIGNATURE   0x1000111201101011LL //Bender's nightmare

#define GENERAL_INTERNAL_ERRROR_INSTANCE "GENERAL_INTERNAL_ERROR: WALLET INSTNACE NOT FOUND"
#define GENERAL_INTERNAL_ERRROR_INIT "Failed to intialize library"

//TODO: global objects, subject to refactoring



#define GET_INSTANCE_PTR(ptr_name) \
  auto ptr_name = std::atomic_load(&ginstance_ptr); \
  if (!ptr_name) \
  { \
    LOG_ERROR("Core already deinitialised or not initialized yet."); \
    epee::json_rpc::response<view::api_responce_return_code, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response); \
    ok_response.result.return_code = API_RETURN_CODE_UNINITIALIZED; \
    return epee::serialization::store_t_to_json(ok_response); \
  }
namespace plain_wallet
{
  void deinit();
}

void static_destroy_handler()
{
  LOG_PRINT_L0("[DESTROY CALLBACK HANDLER STARTED]: ");
  plain_wallet::deinit();
  LOG_PRINT_L0("[DESTROY CALLBACK HANDLER FINISHED]: ");
}

namespace plain_wallet
{
  struct plain_wallet_instance
  {
    plain_wallet_instance() :initialized(false), gjobs_counter(1)
    {}
    wallets_manager gwm;
    std::atomic<bool> initialized;

    std::atomic<uint64_t> gjobs_counter;
    std::map<uint64_t, std::string> gjobs;
    epee::critical_section gjobs_lock;
  };

  std::shared_ptr<plain_wallet::plain_wallet_instance> ginstance_ptr;



  typedef epee::json_rpc::response<epee::json_rpc::dummy_result, error> error_response;


  std::string get_set_working_dir(bool need_to_set = false, const std::string val = "")
  {
    DEFINE_SECURE_STATIC_VAR(std::string, working_dir);
    if (need_to_set)
      working_dir = val;
    return working_dir;
  }

  std::string get_bundle_working_dir()
  {
    return get_set_working_dir();
  }
  void set_bundle_working_dir(const std::string& dir)
  {
    get_set_working_dir(true, dir);
  }
  
  std::string get_wallets_folder()
  {
#ifdef CAKEWALLET    
    std::string path = "";    
#elif WIN32
    std::string path = get_bundle_working_dir() + "/" + WALLETS_FOLDER_NAME + "/";
#else
    std::string path = get_bundle_working_dir() + "/" + WALLETS_FOLDER_NAME + "/";
#endif // WIN32
    return path;
  }

  std::string get_app_config_folder()
  {
#ifdef CAKEWALLET    
    std::string path = "";
#elif WIN32
    std::string path = get_bundle_working_dir() + "/" + APP_CONFIG_FOLDER + "/";
#else
    std::string path = get_bundle_working_dir() + "/" + APP_CONFIG_FOLDER + "/";
#endif // WIN32
    return path;
  }
#ifdef ANDROID_BUILD
  class android_logger : public log_space::ibase_log_stream
  {
  public:
    int get_type() { return LOGGER_CONSOLE; }
    virtual bool out_buffer(const char* buffer, int buffer_len, int log_level, int color, const char* plog_name = NULL)
    {
      __android_log_write(ANDROID_LOG_INFO, "[tag]", buffer);
      return  true;
    }
  };
#endif

  void initialize_logs(int log_level)
  {
    std::string log_dir = get_bundle_working_dir();
    log_dir += "/" LOGS_FOLDER;

    log_space::get_set_need_thread_id(true, true);
    log_space::log_singletone::enable_channels("core,currency_protocol,tx_pool,p2p,wallet", false);
    epee::log_space::get_set_log_detalisation_level(true, log_level);
#ifdef ANDROID_BUILD
    epee::log_space::log_singletone::add_logger(new android_logger());
#else
    epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
#endif

    epee::log_space::log_singletone::add_logger(LOGGER_FILE, "plain_wallet.log", log_dir.c_str());
    LOG_PRINT_L0("Plain wallet initialized: " << CURRENCY_NAME << " v" << PROJECT_VERSION_LONG << ", log location: " << log_dir + "/plain_wallet.log");

    //glogs_initialized = true;
  }

  std::string set_log_level(int log_level)
  {
    epee::log_space::get_set_log_detalisation_level(true, log_level);
    return "{}";
  }

  void deinit()
  {
    auto local_ptr = std::atomic_load(&ginstance_ptr);
    if (local_ptr)
    {
      std::atomic_store(&ginstance_ptr, std::shared_ptr<plain_wallet_instance>());
      //wait other callers finish
      local_ptr->gjobs_lock.lock();
      local_ptr->gjobs_lock.unlock();
      bool r = local_ptr->gwm.quick_stop_no_save();        
      LOG_PRINT_L0("[QUICK_STOP_NO_SAVE] return " << r);
      //let's prepare wallet manager for quick shutdown
      local_ptr.reset();
    }
  }

  std::string reset()
  {
    GET_INSTANCE_PTR(inst_ptr);
    inst_ptr->gwm.quick_clear_wallets_no_save();
    epee::json_rpc::response<view::api_responce_return_code, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
    ok_response.result.return_code = API_RETURN_CODE_OK;
    return epee::serialization::store_t_to_json(ok_response);
  }


  std::string init(const std::string& ip, const std::string& port, const std::string& working_dir, int log_level)
  {
    auto local_ptr = std::atomic_load(&ginstance_ptr);
    if (local_ptr)
    {
      LOG_ERROR("Double-initialization in plain_wallet detected.");
      epee::json_rpc::response<view::api_responce_return_code, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
      ok_response.result.return_code = API_RETURN_CODE_ALREADY_EXISTS;
      return epee::serialization::store_t_to_json(ok_response);
    }

    epee::static_helpers::set_or_call_on_destruct(true, static_destroy_handler);

    std::shared_ptr<plain_wallet_instance> ptr(new plain_wallet_instance());

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
    std::string command_line_fail_details;
    if (!(ptr->gwm.init_command_line(3, args, command_line_fail_details) && ptr->gwm.init(nullptr)))
    {
      LOG_ERROR("Failed to init wallets_manager");
      return GENERAL_INTERNAL_ERRROR_INIT;
    }
    
    ptr->gwm.set_use_deffered_global_outputs(true);

    if(!ptr->gwm.start())
    {
      LOG_ERROR("Failed to start wallets_manager");
      return GENERAL_INTERNAL_ERRROR_INIT;
    }

    LOG_PRINT_L0("[INIT PLAIN_WALLET_INSTANCE] Ver:" << PROJECT_VERSION_LONG << "(" << BUILD_TYPE << ")");


#ifndef CAKEWALLET
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
#endif
    std::atomic_store(&ginstance_ptr, ptr);
#ifndef CAKEWALLET
    epee::json_rpc::response<view::api_responce_return_code, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
    ok_response.result.return_code = API_RETURN_CODE_OK;
    return epee::serialization::store_t_to_json(ok_response);
#else
    return API_RETURN_CODE_OK;
#endif
  }

  std::string init(const std::string& address, const std::string& working_dir, int log_level)
  {
    epee::net_utils::http::url_content url_data = AUTO_VAL_INIT(url_data);
    if(!epee::net_utils::parse_url(address, url_data))
    {
        LOG_ERROR("Failed to parse address");
        return API_RETURN_CODE_BAD_ARG;
    }
    return init(url_data.host, std::to_string(url_data.port), working_dir, log_level);
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
  bool is_wallet_exist(const std::string& path)
  {
    boost::system::error_code ec;
    return boost::filesystem::exists(path, ec);
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
    GET_INSTANCE_PTR(inst_ptr);

    return inst_ptr->gwm.get_connectivity_status();
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

  std::string get_export_private_info(const std::string& target_dir)
  {
    const std::string src_folder_path = get_bundle_working_dir();
    boost::system::error_code ec;
    const std::string full_target_path = target_dir + "/Zano_export" + std::to_string(epee::misc_utils::get_tick_count());
    boost::filesystem::create_directory(full_target_path, ec);
    if (ec)
    {
      LOG_ERROR("Failed to create target directory(" << full_target_path << "):" << ec.message());
      epee::json_rpc::response<view::api_responce_return_code, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
      ok_response.result.return_code = API_RETURN_CODE_FAIL;
      return epee::serialization::store_t_to_json(ok_response);
    }
    if(!tools::copy_dir(src_folder_path, full_target_path))
    {
      LOG_ERROR("Failed to copy target directory");
      epee::json_rpc::response<view::api_responce_return_code, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
      ok_response.result.return_code = API_RETURN_CODE_FAIL;
      return epee::serialization::store_t_to_json(ok_response);
    }
    epee::json_rpc::response<view::api_responce_return_code, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
    ok_response.result.return_code = API_RETURN_CODE_OK;
    return epee::serialization::store_t_to_json(ok_response);
  }

  std::string delete_wallet(const std::string& file_name)
  {
    std::string wallet_files_path = get_wallets_folder();
    strings_list sl = AUTO_VAL_INIT(sl);
    boost::system::error_code er;
    boost::filesystem::remove(wallet_files_path + file_name, er);
    epee::json_rpc::response<view::api_responce_return_code, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
    ok_response.result.return_code = API_RETURN_CODE_OK;
    return epee::serialization::store_t_to_json(ok_response);
  }

  std::string get_address_info(const std::string& addr)
  {
    currency::account_public_address apa = AUTO_VAL_INIT(apa);
    currency::payment_id_t pid = AUTO_VAL_INIT(pid);
    bool valid = false;
    bool wrap = false;
    while (true)
    {
      if (currency::is_address_like_wrapped(addr))
      {
        wrap = true;
        valid = true;
        break;
      }

      if (currency::get_account_address_and_payment_id_from_str(apa, pid, addr))
      {
        valid = true;
      }
      break;
    }

    //lazy to make struct for it
    std::stringstream res;
    res << "{ \"valid\": " << (valid?"true":"false") << ", \"auditable\": "
      << (apa.is_auditable() ? "true" : "false")
      << ",\"payment_id\": " << (pid.size() ? "true" : "false") 
      << ",\"wrap\": " << (wrap ? "true" : "false")
      << "}";
    return res.str();
  }

  std::string open(const std::string& path, const std::string& password)
  {
    GET_INSTANCE_PTR(inst_ptr);

    std::string full_path = get_wallets_folder() + path;
    epee::json_rpc::response<view::open_wallet_response, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
    std::string rsp = inst_ptr->gwm.open_wallet(epee::string_encoding::convert_to_unicode(full_path), password, 20, ok_response.result);
    if (rsp == API_RETURN_CODE_OK || rsp == API_RETURN_CODE_FILE_RESTORED)
    {
      if (rsp == API_RETURN_CODE_FILE_RESTORED)
      {
        ok_response.result.recovered = true;
      }
      inst_ptr->gwm.run_wallet(ok_response.result.wallet_id);

      return epee::serialization::store_t_to_json(ok_response);
    }
    error_response err_result = AUTO_VAL_INIT(err_result);
    err_result.error.code = rsp;
    return epee::serialization::store_t_to_json(err_result);
  }

  std::string restore(const std::string& seed, const std::string& path, const std::string& password, const std::string& seed_password)
  {
    GET_INSTANCE_PTR(inst_ptr);

    std::string full_path = get_wallets_folder() + path;
    epee::json_rpc::response<view::open_wallet_response, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
    std::string rsp = inst_ptr->gwm.restore_wallet(epee::string_encoding::convert_to_unicode(full_path), password, seed, seed_password, ok_response.result);
    if (rsp == API_RETURN_CODE_OK || rsp == API_RETURN_CODE_FILE_RESTORED)
    {
      if (rsp == API_RETURN_CODE_FILE_RESTORED)
      {
        ok_response.result.recovered = true;
      }
      inst_ptr->gwm.run_wallet(ok_response.result.wallet_id);
      return epee::serialization::store_t_to_json(ok_response);
    }
    error_response err_result = AUTO_VAL_INIT(err_result);
    err_result.error.code = rsp;
    return epee::serialization::store_t_to_json(err_result);
  }

  std::string generate(const std::string& path, const std::string& password)
  {
    GET_INSTANCE_PTR(inst_ptr);

    std::string full_path = get_wallets_folder() + path;
    epee::json_rpc::response<view::open_wallet_response, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
    std::string rsp = inst_ptr->gwm.generate_wallet(epee::string_encoding::convert_to_unicode(full_path), password, ok_response.result);
    if (rsp == API_RETURN_CODE_OK || rsp == API_RETURN_CODE_FILE_RESTORED)
    {
      if (rsp == API_RETURN_CODE_FILE_RESTORED)
      {
        ok_response.result.recovered = true;
      }
      inst_ptr->gwm.run_wallet(ok_response.result.wallet_id);
      return epee::serialization::store_t_to_json(ok_response);
    }
    error_response err_result = AUTO_VAL_INIT(err_result);
    err_result.error.code = rsp;
    return epee::serialization::store_t_to_json(err_result);
  }

  std::string get_opened_wallets()
  {
    GET_INSTANCE_PTR(inst_ptr);
    epee::json_rpc::response<std::list<view::open_wallet_response>, epee::json_rpc::dummy_error> ok_response = AUTO_VAL_INIT(ok_response);
    inst_ptr->gwm.get_opened_wallets(ok_response.result);
    return epee::serialization::store_t_to_json(ok_response);
  }

  std::string close_wallet(hwallet h)
  {
    GET_INSTANCE_PTR(inst_ptr);

    std::string r = "{\"response\": \"";
    r += inst_ptr->gwm.close_wallet(h);
    r += "\"}";
    return r;
  }

  std::string get_wallet_status(hwallet h)
  {
    GET_INSTANCE_PTR(inst_ptr);
    return inst_ptr->gwm.get_wallet_status(h);
  }

  std::string invoke(hwallet h, const std::string& params)
  {
    GET_INSTANCE_PTR(inst_ptr);
    return inst_ptr->gwm.invoke(h, params);
  }

  void put_result(uint64_t job_id, const std::string& res)
  {
    auto inst_ptr = std::atomic_load(&ginstance_ptr);
    if (!inst_ptr)
    {
      return;
    }
    CRITICAL_REGION_LOCAL(inst_ptr->gjobs_lock);
    inst_ptr->gjobs[job_id] = res;
    LOG_PRINT_L2("[ASYNC_CALL]: Finished(result put), job id: " << job_id);
  }


  std::string async_call(const std::string& method_name, uint64_t instance_id, const std::string& params)
  {
    GET_INSTANCE_PTR(inst_ptr);
    std::function<void()> async_callback;

    uint64_t job_id = inst_ptr->gjobs_counter++;
    async_callback = [job_id, instance_id, method_name, params]()
    {
      std::string res_str = sync_call(method_name,  instance_id, params);
      put_result(job_id, res_str);
    };

    std::thread t([async_callback]() {async_callback(); });
    t.detach();
    LOG_PRINT_L2("[ASYNC_CALL]: started " << method_name << ", job id: " << job_id);
    return std::string("{ \"job_id\": ") + std::to_string(job_id) + "}";
  }


  std::string sync_call(const std::string& method_name, uint64_t instance_id, const std::string& params)
  {
    std::string res;
    if (method_name == "close")
    {
        close_wallet(instance_id);
        view::api_responce_return_code rc = AUTO_VAL_INIT(rc);
        rc.return_code = API_RETURN_CODE_OK;
        res = epee::serialization::store_t_to_json(rc);
    }
    else if (method_name == "open")
    {
      view::open_wallet_request owr = AUTO_VAL_INIT(owr);
      if (!epee::serialization::load_t_from_json(owr, params))
      {
        view::api_response ar = AUTO_VAL_INIT(ar);
        ar.error_code = "Wrong parameter";
        res = epee::serialization::store_t_to_json(ar);
      }else
      {
        res = open(owr.path, owr.pass);
      }
    }
    else if (method_name == "restore")
    {
      view::restore_wallet_request rwr = AUTO_VAL_INIT(rwr);
      if (!epee::serialization::load_t_from_json(rwr, params))
      {
        view::api_response ar = AUTO_VAL_INIT(ar);
        ar.error_code = "Wrong parameter";
        res = epee::serialization::store_t_to_json(ar);
      }
      else
      {
        res = restore(rwr.seed_phrase, rwr.path, rwr.pass, rwr.seed_pass);
      }
    }
    else if (method_name == "get_seed_phrase_info")
    {
      view::seed_info_param sip = AUTO_VAL_INIT(sip);
      if (!epee::serialization::load_t_from_json(sip, params))
      {
        view::api_response ar = AUTO_VAL_INIT(ar);
        ar.error_code = "Wrong parameter";
        res = epee::serialization::store_t_to_json(ar);
      }
      else
      {
        view::api_response_t<view::seed_phrase_info> rsp = AUTO_VAL_INIT(rsp);
        rsp.error_code = tools::get_seed_phrase_info(sip.seed_phrase, sip.seed_password, rsp.response_data);
        res = epee::serialization::store_t_to_json(rsp);
      }
    }    
    else if (method_name == "invoke")
    {
      res = invoke(instance_id, params);
    }
    else if (method_name == "get_wallet_status")
    {
      res = get_wallet_status(instance_id);
    }
    else
    {
      view::api_response ar = AUTO_VAL_INIT(ar);
      ar.error_code = "UNKNOWN METHOD";
      res = epee::serialization::store_t_to_json(ar);
    }
    return res;
  }




  std::string try_pull_result(uint64_t job_id)
  {
    auto inst_ptr = std::atomic_load(&ginstance_ptr);
    if (!inst_ptr)
    { 
      return "{\"status\": \"canceled\"}";
    }
    //TODO: need refactoring
    CRITICAL_REGION_LOCAL(inst_ptr->gjobs_lock);
    auto it = inst_ptr->gjobs.find(job_id);
    if (it == inst_ptr->gjobs.end())
    {
      return "{\"status\": \"idle\"}";
    }
    std::string res = "{\"status\": \"delivered\", \"result\": ";
    res += it->second;
    res += "  }";
    inst_ptr->gjobs.erase(it);
    return res;
  }

  struct wallet_extended_info
  {
    view::wallet_info wi;
    view::wallet_info_extra wi_extended;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wi)
      KV_SERIALIZE(wi_extended)
    END_KV_SERIALIZE_MAP()
  };

  std::string get_wallet_info(hwallet h)
  {
    GET_INSTANCE_PTR(inst_ptr);
    wallet_extended_info wei = AUTO_VAL_INIT(wei);
    inst_ptr->gwm.get_wallet_info(h, wei.wi);
    inst_ptr->gwm.get_wallet_info_extra(h, wei.wi_extended);
    return epee::serialization::store_t_to_json(wei);
  }
  std::string reset_wallet_password(hwallet h, const std::string& password)
  {
    GET_INSTANCE_PTR(inst_ptr);
    return inst_ptr->gwm.reset_wallet_password(h, password);
  }


  // 0 (default), 1 (unimportant), 2 (normal), 3 (elevated), 4 (priority)
  uint64_t get_current_tx_fee(uint64_t priority)
  {
    return TX_DEFAULT_FEE;
  }
}