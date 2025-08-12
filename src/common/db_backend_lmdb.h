// Copyright (c) 2014-2024 Zano Project
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
      friend class lmdb_txn;
      class lmdb_txn 
      {
      public:
        lmdb_txn(lmdb_db_backend& db);
        lmdb_txn(lmdb_db_backend& db, bool is_read_only);
        lmdb_txn(lmdb_txn&&) noexcept;
        lmdb_txn& operator=(lmdb_txn&&) noexcept;
        lmdb_txn(const lmdb_txn&) = delete;
        lmdb_txn& operator=(const lmdb_txn&) = delete;

        ~lmdb_txn();

        int begin(MDB_txn* parent_tx);
        int commit();
        void abort();
        void mark_finished(); // mark when commit or abort is called, to avoid double commit/abort and correct delete in destructor

        MDB_txn* ptx{nullptr};
        bool read_only{false}; // needed for thread-top transaction, for figure out if we need to unlock exclusive access
        size_t nested_count{0};   //count of read-only nested emulated transactions

      private:
        std::reference_wrapper<lmdb_db_backend> m_db;
        bool m_marked_finished{false};
      };

      typedef std::list<lmdb_txn> transactions_list;

      std::string m_path;
      MDB_env *m_penv;

      boost::recursive_mutex m_cs;
      boost::recursive_mutex m_write_exclusive_lock;
      std::map<std::thread::id, transactions_list> m_txs; // thread_id -> count of nested read_only transactions
      bool pop_lmdb_txn(lmdb_txn& txe);

    public:
      lmdb_db_backend();
      ~lmdb_db_backend();

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
      MDB_txn* get_current_tx();

      static bool convert_db_4kb_page_to_16kb_page(const std::string& source_path, const std::string& destination_path);

    };
  }
}
