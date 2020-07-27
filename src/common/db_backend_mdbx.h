// Copyright (c) 2014-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project 
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#ifdef ENABLED_ENGINE_MDBX
#include  <thread>

#include "include_base_utils.h"

#include "db_backend_base.h"
#include "db/libmdbx/mdbx.h"


namespace tools
{
  namespace db
  {
    class mdbx_db_backend : public i_db_backend
    {

      struct tx_entry
      {
        MDBX_txn* ptx;
        bool read_only; // needed for thread-top transaction, for figure out if we need to unlock exclusive access
        size_t count;   //count of read-only nested emulated transactions
      };
      typedef std::list<tx_entry> transactions_list;


      std::string m_path;
      MDBX_env *m_penv;

      boost::recursive_mutex m_cs;
      boost::recursive_mutex m_write_exclusive_lock;
      std::map<std::thread::id, transactions_list> m_txs; // size_t -> count of nested read_only transactions
      bool pop_tx_entry(tx_entry& txe);
    public:
      mdbx_db_backend();
      ~mdbx_db_backend();

      //----------------- i_db_backend -----------------------------------------------------
      bool close() override;
      bool begin_transaction(bool read_only = false) override;
      bool commit_transaction() override;
      void abort_transaction() override;
      bool open(const std::string& path, uint64_t cache_sz = CACHE_SIZE) override;
      bool open_container(const std::string& name, container_handle& h) override;
      bool close_container(container_handle& h) override;
      bool erase(container_handle h, const char* k, size_t s) override;
      bool get(container_handle h, const char* k, size_t s, std::string& res_buff) override;
      bool clear(container_handle h) override;
      uint64_t size(container_handle h) override;
      bool set(container_handle h, const char* k, size_t s, const char* v, size_t vs) override;
      bool enumerate(container_handle h, i_db_callback* pcb) override;
      bool get_stat_info(tools::db::stat_info& si) override;
      const char* name() override;
      //-------------------------------------------------------------------------------------
      bool have_tx();
      MDBX_txn* get_current_tx();

    };
    
  }
}
#endif