
// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// 
// #pragma once
// #include "db_backend_leveldb.h"
// 
// namespace tools
// {
//   namespace db
//   {
//     level_db_backend : public i_db_backend
//     {
//       std::string m_path;
//       db_handle m_pdb;
//     public:
//       basic_db() :m_pdb(nullptr)
//       {}
//       ~basic_db(){ close(); }
//       bool close()
//       {
//         if (m_pdb)
//           delete m_pdb;
//         m_pdb = nullptr;
// 
//         return true;
//       }
// 
//       void begin_transaction()
//       {
//         //leveldb is not supporting transactions yet
//       }
// 
//       void commit_transaction()
//       {
//         //leveldb is not supporting transactions yet
//       }
// 
// 
//       bool open(const std::string& path)
//       {
//         m_path = path;
//         close();
//         leveldb::Options options;
//         options.create_if_missing = true;
//         leveldb::Status status = leveldb::DB::Open(options, path, &m_pdb);
//         if (!status.ok())
//         {
//           LOG_ERROR("Unable to open/create database " << path << ", error: " << status.ToString());
//           return err_handle;
//         }
//         return true;
//       }
// 
//       bool erase(const char* k, size s)
//       {
//         TRY_ENTRY();
//         leveldb::WriteOptions wo;
//         wo.sync = true;
//         leveldb::Status s = m_pdb->Delete(wo, leveldb::Slice(k, s));
//         if (!s.ok())
//           return false;
// 
//         return true;
//         CATCH_ENTRY_L0("get_t_object_from_db", false);
//       }
// 
// 
//       bool get(const char* k, size s, std::string& res_buff)
//       {
//         TRY_ENTRY();
//         leveldb::ReadOptions ro;
//         leveldb::Status s = m_pdb->Get(ro, leveldb::Slice(k, s), &res_buff);
//         if (!s.ok())
//           return false;
// 
//         CATCH_ENTRY_L0("get_t_object_from_db", false);
//       }
// 
//       bool clear()
//       {
//         close();
//         boost::system::error_code ec;
//         bool res = boost::filesystem::remove_all(m_path, ec);
//         if (!res)
//         {
//           LOG_ERROR("Failed to remove db file " << m_path << ", why: " << ec);
//           return false;
//         }
//         return open(m_path);
//       }
// 
//       bool set(const char* k, size s, const char* v, size vs)
//       {
//         TRY_ENTRY();
//         leveldb::WriteOptions wo;
//         wo.sync = true;
//         leveldb::Status s = m_pdb->Put(wo, leveldb::Slice(k, s), leveldb::Slice(v, vs));
//         if (!s.ok())
//           return false;
// 
//         return true;
//         CATCH_ENTRY_L0("set_t_object_to_db", false);
//       }
//     };
//   }
// }
