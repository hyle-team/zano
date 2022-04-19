// Copyright (c) 2014-2022 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once



namespace currency
{
  struct global_config_data
  {
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(data_storage_path)
      KV_SERIALIZE(log_dir)
      KV_SERIALIZE(arg_log_file)
    END_KV_SERIALIZE_MAP()

    bool init()
    {
      const std::string config_file_path = tools::get_default_data_dir() + "/" CURRENCY_GLOBAL_CONFIG_FILENAME;
      if (command_line::has_arg(m_vm, command_line::arg_data_dir))
      {
        config_file_path = command_line::get_arg(m_vm, command_line::arg_data_dir) + "/" CURRENCY_GLOBAL_CONFIG_FILENAME;
      }
      epee::serialization::load_t_from_json_file(*this, config_file_path);
      validate_config();
      bool r = epee::serialization::store_t_to_json_file(*this, config_file_path);
      if (!r)
      {
        LOG_ERROR("Failed to store config to " << config_file_path);
      }

      return true;
    }


    const std::string get_log_dir()
    {


    }



  private: 
    void validate_config()
    {
      
    }

    boost::program_options::variables_map m_vm;
    
    //params
    std::string data_storage_path;
    std::string log_dir;
    std::string arg_log_file;
    /*
    TODO: add those items as config file options, but solve problem of "undefined" state for every option
    size_t log_level;
    bool enable_in_ui_console;
    bool arg_disable_upnp;
    bool disable_ntp;
    bool disable_stop_if_time_out_of_sync;
    bool disable_stop_on_low_free_space;
    bool enable_offers_service;
    bool db_engine;
    */
       

  };
}

