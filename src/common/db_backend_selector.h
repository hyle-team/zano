// Copyright (c) 2014-2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/program_options.hpp>
#include "misc_language.h"
#include "db_backend_base.h"

namespace tools
{
  namespace db
  {
    enum db_engine_type { db_none = 0, db_lmdb, db_mdbx };
    
    class db_backend_selector
    {
    public:
      db_backend_selector();

      static void init_options(boost::program_options::options_description& desc);
      bool init(const boost::program_options::variables_map& vm);

      std::string get_db_folder_path() const;
      std::string get_db_main_file_name() const;
      db_engine_type get_engine_type() const { return m_engine_type; }
      std::string get_engine_name() const;
      std::string get_config_folder() const { return m_config_folder; }
      std::string get_temp_config_folder() const;
      std::string get_temp_db_folder_path() const;
 
      std::string get_pool_db_folder_path() const;

      std::shared_ptr<tools::db::i_db_backend> create_backend();

    private:
      db_engine_type m_engine_type;
      std::string m_config_folder;
    };

  } // namespace db
} // namespace tools
