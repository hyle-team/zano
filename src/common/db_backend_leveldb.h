// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "include_base_utils.h"

// #include "db_backend_base.h"
// 
// #include "currency_db_base.h"
// #include "leveldb/db.h"
// #include "common/boost_serialization_helper.h"
// #include "common/difficulty_boost_serialization.h"
// #include "currency_format_utils.h"
// 
// 
// namespace tools
// {
//   namespace db
//   {
//     typedef leveldb::DB* db_handle;
// 
//     static const db_handle err_handle = nullptr;
// 
//     class level_db_backend : public i_db_backend
//     {
//       std::string m_path;
//       db_handle m_pdb;
//     public:
//       level_db_backend() :m_pdb(nullptr)
//       {}
//       ~level_db_backend(){ close(); }
//       bool close();
//       void begin_transaction();
//       void commit_transaction();
//       bool open(const std::string& path);
//       bool erase(const char* k, size s);
//       bool get(const char* k, size s, std::string& res_buff);
//       bool clear();
//       bool set(const char* k, size s, const char* v, size vs);
//     };
//   }
// }


