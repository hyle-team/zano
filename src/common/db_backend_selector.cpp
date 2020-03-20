// Copyright (c) 2014-2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "db_backend_selector.h"
#include "currency_core/currency_config.h"
#include "command_line.h"
#include "db_backend_lmdb.h"
#include "db_backend_mdbx.h"

#define LMDB_MAIN_FILE_NAME  "data.mdb"
#define MDBX_MAIN_FILE_NAME  "mdbx.dat"

namespace tools
{
namespace db
{

  db_backend_selector::db_backend_selector()
    : m_engine_type(db_none)
  {
  }

  void db_backend_selector::init_options(boost::program_options::options_description& desc)
  {
    command_line::add_arg(desc, command_line::arg_db_engine);
  }

  bool db_backend_selector::init(const boost::program_options::variables_map& vm)
  {
    try
    {
      m_config_folder = command_line::get_arg(vm, command_line::arg_data_dir);

      if (command_line::get_arg(vm, command_line::arg_db_engine) == ARG_DB_ENGINE_LMDB)
      {
        m_engine_type = db_lmdb;
      }
      else if (command_line::get_arg(vm, command_line::arg_db_engine) == ARG_DB_ENGINE_MDBX)
      {
#ifdef ENABLED_ENGINE_MDBX
        m_engine_type = db_mdbx;
#else
        LOG_PRINT_L0(" DB ENGINE: " << ARG_DB_ENGINE_MDBX << " is not suported by this build(see DISABLE_MDBX cmake option), STOPPING");
#endif 
      }
      else
      {
        LOG_PRINT_RED_L0("UNKNOWN DB ENGINE: " << command_line::get_arg(vm, command_line::arg_db_engine) << ", STOPPING");
      }
    }
    catch (std::exception& e)
    {
      LOG_ERROR("internal error: db_backend_selector::init failed on command-line parsing, exception: " << e.what());
      return false;
    }

    if (m_engine_type == db_none)
      return false;

    return true;
  }

  std::string db_backend_selector::get_db_folder_path() const
  {
    //CHECK_AND_ASSERT_THROW_MES(m_engine_type != db_none, "db_backend_selector was no inited");
    return m_config_folder + ("/" CURRENCY_BLOCKCHAINDATA_FOLDERNAME_PREFIX) + get_engine_name() + CURRENCY_BLOCKCHAINDATA_FOLDERNAME_SUFFIX;
  }

  std::string db_backend_selector::get_temp_db_folder_path() const
  {
    //CHECK_AND_ASSERT_THROW_MES(m_engine_type != db_none, "db_backend_selector was no inited");
    return get_temp_config_folder() + ("/" CURRENCY_BLOCKCHAINDATA_FOLDERNAME_PREFIX) + get_engine_name() + CURRENCY_BLOCKCHAINDATA_FOLDERNAME_SUFFIX;
  }

  std::string db_backend_selector::get_pool_db_folder_path() const
  {
    return m_config_folder + ("/" CURRENCY_POOLDATA_FOLDERNAME_PREFIX) + get_engine_name() + CURRENCY_POOLDATA_FOLDERNAME_SUFFIX;
  }

  std::string db_backend_selector::get_db_main_file_name() const
  {
    switch (m_engine_type)
    {
    case db_lmdb:
      return LMDB_MAIN_FILE_NAME;
    case db_mdbx:
      return MDBX_MAIN_FILE_NAME;
    default:
      return "";
    }
  }

  std::string db_backend_selector::get_engine_name() const
  {
    switch (m_engine_type)
    {
    case db_lmdb:
      return "lmdb";
    case db_mdbx:
      return "mdbx";
    default:
      return "unknown";
    }
  }

  std::shared_ptr<tools::db::i_db_backend> db_backend_selector::create_backend()
  {
    switch (m_engine_type)
    {
    case db_lmdb:
      return std::shared_ptr<tools::db::i_db_backend>(new tools::db::lmdb_db_backend);

    case db_mdbx:
      return std::shared_ptr<tools::db::i_db_backend>(new tools::db::mdbx_db_backend);

    default:
      LOG_ERROR("db_backend_selector was no inited");
      return nullptr;
    }
  }

  std::string db_backend_selector::get_temp_config_folder() const
  {
    return m_config_folder + "_TEMP";                                                                  
  }




} // namespace db
} // namespace tools
