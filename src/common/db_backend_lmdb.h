// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project 
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include  <thread>

#include "include_base_utils.h"

#include "db_backend_base.h"
#include "db/liblmdb/lmdb.h"

struct lmdb_txn_destructor_abort_test;
struct lmdb_ro_double_begin_bad_rslot_test;
struct lmdb_ro_cross_thread_finish_test;



namespace tools
{
  namespace db
  {
    struct lmdb_txn_destructor_abort_test;
    class lmdb_db_backend : public i_db_backend
    {
      friend class lmdb_txn;
      friend struct ::lmdb_txn_destructor_abort_test;
      friend struct ::lmdb_ro_double_begin_bad_rslot_test;
      friend struct ::lmdb_ro_cross_thread_finish_test;
      
      
      class lmdb_txn 
      {
      public:
        lmdb_txn(lmdb_db_backend& db);
        lmdb_txn(lmdb_db_backend& db, bool is_read_only);
        //lmdb_txn(lmdb_txn&&) noexcept;
        //lmdb_txn& operator=(lmdb_txn&&) noexcept;
        lmdb_txn(const lmdb_txn&) = delete;
        lmdb_txn& operator=(const lmdb_txn&) = delete;

        ~lmdb_txn();

        int begin(MDB_txn* parent_tx);
        int commit();
        void abort();
        bool is_empty();


        //void mark_finished(); // mark when commit or abort is called, to avoid double commit/abort and correct delete in destructor

        MDB_txn* m_ptx{nullptr};
        bool m_read_only{false};
        uint64_t m_ref_count{ 0 };
        // needed for thread-top transaction, for figure out if we need to unlock exclusive access
        //size_t m_nested_count{0};   //count of read-only nested emulated transactions

        //uint64_t m_dbg_id{0};        // human readable transaction id
        //uint32_t m_stack_level{0};   // depth of the stack at the moment of opening (number of elements before push)
        std::thread::id m_owner{};
      private:
        bool validate_empty();
        void check_thread_id();

        std::reference_wrapper<lmdb_db_backend> m_db;
        //bool m_marked_finished{true};

      };



      /*
      It could be only 1 RO transaction in the list, and it might be only at the beginning of the list
      [0]w-transaction
      or
      [0]ro-transaction
      [1]w-transaction
      or
      [0]ro-transaction
      [1]w-transaction
      [2]w-transaction
      etc

      if begin(ro) call happens at any level where last tx is W-transaction in the list, then it's ref-counter just increases, while m_calls 
      items is always added and linked to item that were before incremented or created. 
      At the commit/abort link to lmdb_txn is validated as it should be the latest object on the m_txs stack

      */
      
      struct begin_call_handler
      {
        begin_call_handler(std::shared_ptr<lmdb_txn> related_tx_object, bool is_read_obly):m_related_tx_object(related_tx_object), m_read_only(is_read_obly)
        {}
        std::shared_ptr<lmdb_txn> m_related_tx_object;
        bool m_read_only = false;
      }

      struct thread_transactions
      {
        st::list<std::shared_ptr<begin_call_handler> > m_calls;
        std::list<std::shared_ptr<lmdb_txn>> m_txs;
      };


      typedef std::list<std::shared_ptr<lmdb_txn>> transactions_list;

      std::string m_path;
      MDB_env *m_penv;

      boost::recursive_mutex m_cs;
      boost::recursive_mutex m_write_exclusive_lock;
      std::map<std::thread::id, transactions_list> m_txs; // thread_id -> count of nested read_only transactions
      bool pop_lmdb_txn(lmdb_txn& txe);
      bool m_journal_printed = false;

    public:
      lmdb_db_backend();
      ~lmdb_db_backend();
      void dump();
      void dump_tx_stacks();
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
