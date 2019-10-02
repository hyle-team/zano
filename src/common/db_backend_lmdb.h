// Copyright (c) 2014-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project 
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include  <thread>

#include "include_base_utils.h"

#include "db_backend_base.h"
#include "db/liblmdb/lmdb.h"


namespace tools
{
  namespace db
  {
    class lmdb_db_backend : public i_db_backend
    {

      struct tx_entry
      {
        MDB_txn* ptx;
        bool read_only; // needed for thread-top transaction, for figure out if we need to unlock exclusive access
        size_t count;   //count of read-only nested emulated transactions
      };
      typedef std::list<tx_entry> transactions_list;


      std::string m_path;
      MDB_env *m_penv;

      boost::recursive_mutex m_cs;
      boost::recursive_mutex m_write_exclusive_lock;
      std::map<std::thread::id, transactions_list> m_txs; // size_t -> count of nested read_only transactions
      bool pop_tx_entry(tx_entry& txe);
    public:
      lmdb_db_backend();
      ~lmdb_db_backend();

      //----------------- i_db_backend -----------------------------------------------------
      bool close();
      bool begin_transaction(bool read_only = false);
      bool commit_transaction();
      void abort_transaction();
      bool open(const std::string& path, uint64_t cache_sz = CACHE_SIZE);
      bool open_container(const std::string& name, container_handle& h);
      bool erase(container_handle h, const char* k, size_t s);
      bool get(container_handle h, const char* k, size_t s, std::string& res_buff);
      bool clear(container_handle h);
      uint64_t size(container_handle h);
      bool set(container_handle h, const char* k, size_t s, const char* v, size_t vs);
      bool enumerate(container_handle h, i_db_callback* pcb);
      bool get_stat_info(tools::db::stat_info& si);
      //-------------------------------------------------------------------------------------
      bool have_tx();
      MDB_txn* get_current_tx();

    };
  }
}
