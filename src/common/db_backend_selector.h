// Copyright (c) 2014-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "db_backend_lmdb.h"
#include "db_backend_mdbx.h"
#include "common/command_line.h"
#include "common/db_abstract_accessor.h"

namespace tools
{
  namespace db
  {
    inline
      bool select_db_engine_from_arg(const boost::program_options::variables_map& vm, tools::db::basic_db_accessor& rdb)
    {
      if (command_line::get_arg(vm, command_line::arg_db_engine) == ARG_DB_ENGINE_LMDB)
      {
        rdb.reset_backend(std::shared_ptr<tools::db::i_db_backend>(new tools::db::lmdb_db_backend));
      }
      else if (command_line::get_arg(vm, command_line::arg_db_engine) == ARG_DB_ENGINE_MDBX)
      {
#ifdef ENABLED_ENGINE_MDBX
        rdb.reset_backend(std::shared_ptr<tools::db::i_db_backend>(new tools::db::mdbx_db_backend));
#else
        LOG_PRINT_L0(" DB ENGINE: " << ARG_DB_ENGINE_MDBX << " is not suported by this build(see DISABLE_MDBX cmake option), STOPPING");
        return false;
#endif 
      }
      else
      {
        LOG_PRINT_RED_L0(" UNKNOWN DB ENGINE: " << command_line::get_arg(vm, command_line::arg_db_engine) << ", STOPPING");
        return false;
      }
      return true;
    }
  }
}