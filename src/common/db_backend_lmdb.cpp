// Copyright (c) 2014-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project 
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "db_backend_lmdb.h"
#include "misc_language.h"
#include "string_coding.h"
#include "profile_tools.h"
#include "util.h"

#define BUF_SIZE 1024
#define DB_RESIZE_MIN_FREE_SIZE    (100 * 1024 * 1024) // DB map size will grow if that much space left on DB
#define DB_RESIZE_MIN_MAX_SIZE     (50 * 1024 * 1024)  // Minimum DB map size (starting size)
#define DB_RESIZE_INCREMENT_SIZE   (100 * 1024 * 1024) // Grow step size
#define DB_RESIZE_COMMITS_TO_CHECK 50

#define CHECK_AND_ASSERT_MESS_LMDB_DB(rc, ret, mess) CHECK_AND_ASSERT_MES(res == MDB_SUCCESS, ret, "[DB ERROR]:(" << rc << ")" << mdb_strerror(rc) << ", [message]: " << mess);
#define CHECK_AND_ASSERT_THROW_MESS_LMDB_DB(rc, mess) CHECK_AND_ASSERT_THROW_MES(res == MDB_SUCCESS, "[DB ERROR]:(" << rc << ")" << mdb_strerror(rc) << ", [message]: " << mess);
#define ASSERT_MES_AND_THROW_LMDB(rc, mess) ASSERT_MES_AND_THROW("[DB ERROR]:(" << rc << ")" << mdb_strerror(rc) << ", [message]: " << mess);

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL "lmdb"
// 'lmdb' channel is disabled by default

namespace tools
{
  namespace db
  {
    lmdb_db_backend::lmdb_db_backend()
      : m_penv(AUTO_VAL_INIT(m_penv))
      , m_commits_count(0)
    {

    }

    lmdb_db_backend::~lmdb_db_backend()
    {
      NESTED_TRY_ENTRY();

      close();

      NESTED_CATCH_ENTRY(__func__);
    }

    bool lmdb_db_backend::open(const std::string& path_, uint64_t cache_sz)
    {
      int res = 0;
      res = mdb_env_create(&m_penv);
      CHECK_AND_ASSERT_MESS_LMDB_DB(res, false, "Unable to mdb_env_create");
      
      res = mdb_env_set_maxdbs(m_penv, 15);
      CHECK_AND_ASSERT_MESS_LMDB_DB(res, false, "Unable to mdb_env_set_maxdbs");

      m_path = path_;
      CHECK_AND_ASSERT_MES(tools::create_directories_if_necessary(m_path), false, "create_directories_if_necessary failed: " << m_path);

      CHECK_AND_ASSERT_MES(tools::create_directories_if_necessary(m_path), false, "create_directories_if_necessary failed: " << m_path);

      res = mdb_env_open(m_penv, m_path.c_str(), MDB_NORDAHEAD /*| MDB_NOSYNC | MDB_WRITEMAP | MDB_MAPASYNC*/, 0644);
      CHECK_AND_ASSERT_MESS_LMDB_DB(res, false, "Unable to mdb_env_open, m_path=" << m_path);

      resize_if_needed();
      
      return true;
    }

    bool lmdb_db_backend::open_container(const std::string& name, container_handle& h)
    {

      MDB_dbi dbi = AUTO_VAL_INIT(dbi);
      begin_transaction();
      int res = mdb_dbi_open(get_current_tx(), name.c_str(), MDB_CREATE, &dbi);
      CHECK_AND_ASSERT_MESS_LMDB_DB(res, false, "Unable to mdb_dbi_open with container name: " << name);
      commit_transaction();
      h = static_cast<container_handle>(dbi);
      return true;
    }

    bool lmdb_db_backend::close()
    {
      {
        std::lock_guard<boost::recursive_mutex> lock(m_cs);
        for (auto& tx_thread : m_txs)
        {
          for (auto txe : tx_thread.second)
          {
            int res = mdb_txn_commit(txe.ptx);
            if (res != MDB_SUCCESS)
            {
              LOG_ERROR("[DB ERROR]: On close tranactions: " << mdb_strerror(res));
            }
          }
        }

        m_txs.clear();
      }
      if (m_penv)
      {
        mdb_env_close(m_penv);
        m_penv = nullptr;
      }
      return true;
    }

    bool lmdb_db_backend::begin_transaction(bool read_only)
    {
      if (!read_only)
      {
        LOG_PRINT_CYAN("[DB " << m_path << "] WRITE LOCKED", LOG_LEVEL_3);
        CRITICAL_SECTION_LOCK(m_write_exclusive_lock);

        if (m_commits_count.fetch_add(1, std::memory_order_relaxed) % DB_RESIZE_COMMITS_TO_CHECK == DB_RESIZE_COMMITS_TO_CHECK - 1)
        {
          if (!resize_if_needed())
            m_commits_count.store(DB_RESIZE_COMMITS_TO_CHECK - 1, std::memory_order_relaxed); // if failed, try again on next commit
        }

      }
      PROFILE_FUNC("lmdb_db_backend::begin_transaction");
      {
        std::lock_guard<boost::recursive_mutex> lock(m_cs);
        CHECK_AND_ASSERT_THROW_MES(m_penv, "m_penv==null, db closed");
        transactions_list& rtxlist = m_txs[std::this_thread::get_id()];
        MDB_txn* pparent_tx = nullptr;
        MDB_txn* p_new_tx = nullptr;
        bool parent_read_only = false;
        if (rtxlist.size())
        {
          pparent_tx = rtxlist.back().ptx;
          parent_read_only = rtxlist.back().read_only;
        }


        if (pparent_tx && read_only)
        {
          ++rtxlist.back().count;
        }
        else
        {
          int res = 0;
          unsigned int flags = 0;
          if (read_only)
            flags += MDB_RDONLY;

          //don't use parent tx in write transactions if parent tx was read-only (restriction in lmdb) 
          //see "Nested transactions: Max 1 child, write txns only, no writemap"
          if (pparent_tx && parent_read_only)
            pparent_tx = nullptr;

          CHECK_AND_ASSERT_THROW_MES(m_penv, "m_penv==null, db closed");
          res = mdb_txn_begin(m_penv, pparent_tx, flags, &p_new_tx);
          if(res != MDB_SUCCESS)
          {
            //Important: if mdb_txn_begin is failed need to unlock previously locked mutex
            CRITICAL_SECTION_UNLOCK(m_write_exclusive_lock);
            //throw exception to avoid regular code execution 
            ASSERT_MES_AND_THROW_LMDB(res, "Unable to mdb_txn_begin");
          }

          rtxlist.push_back(tx_entry());
          rtxlist.back().count = read_only ? 1 : 0;
          rtxlist.back().ptx = p_new_tx;
          rtxlist.back().read_only = read_only;
        }
      }


      LOG_PRINT_L4("[DB] Transaction started");
      return true;
    }

    MDB_txn* lmdb_db_backend::get_current_tx()
    {
      std::lock_guard<boost::recursive_mutex> lock(m_cs);
      auto& rtxlist = m_txs[std::this_thread::get_id()];
      CHECK_AND_ASSERT_MES(rtxlist.size(), nullptr, "Unable to find active tx for thread " << std::this_thread::get_id());
      return rtxlist.back().ptx;
    }

    bool lmdb_db_backend::pop_tx_entry(tx_entry& txe)
    {
      std::lock_guard<boost::recursive_mutex> lock(m_cs);
      auto it = m_txs.find(std::this_thread::get_id());
      CHECK_AND_ASSERT_MES(it != m_txs.end(), false, "[DB] Unable to find id cor current thread");
      CHECK_AND_ASSERT_MES(it->second.size(), false, "[DB] No active tx for current thread");

      txe = it->second.back();

      if (it->second.back().read_only &&  it->second.back().count == 0)
      {
        LOG_ERROR("Internal db tx state error: read_only and count readers == 0");
      }

      if ((it->second.back().read_only && it->second.back().count < 2) || (!it->second.back().read_only && it->second.back().count < 1))
      {
        it->second.pop_back();
        if (!it->second.size())
          m_txs.erase(it);       
      }
      else
      {
        --it->second.back().count;
      }
      return true;
    }

    bool lmdb_db_backend::commit_transaction()
    {
      PROFILE_FUNC("lmdb_db_backend::commit_transaction");
      {
        tx_entry txe = AUTO_VAL_INIT(txe);
        bool r = pop_tx_entry(txe);
        CHECK_AND_ASSERT_MES(r, false, "Unable to pop_tx_entry");
          
        if (txe.count == 0 || (txe.read_only && txe.count == 1))
        {
          int res = 0;
          res = mdb_txn_commit(txe.ptx);
          CHECK_AND_ASSERT_MESS_LMDB_DB(res, false, "Unable to mdb_txn_commit (error " << res << ")");
          if (!txe.read_only && !txe.count)
          {
            CRITICAL_SECTION_UNLOCK(m_write_exclusive_lock);
            LOG_PRINT_CYAN("[DB " << m_path << "] WRITE UNLOCKED", LOG_LEVEL_3);
          }
        } 
      }
      LOG_PRINT_L4("[DB] Transaction committed");
      return true;
    }
    
    void lmdb_db_backend::abort_transaction()
    {
      {
        tx_entry txe = AUTO_VAL_INIT(txe);
        bool r = pop_tx_entry(txe);
        CHECK_AND_ASSERT_MES(r, void(), "Unable to pop_tx_entry");
        if (txe.count == 0 || (txe.read_only && txe.count == 1))
        {
          mdb_txn_abort(txe.ptx);
          if (!txe.read_only && !txe.count)
          {
            CRITICAL_SECTION_UNLOCK(m_write_exclusive_lock);
            LOG_PRINT_CYAN("[DB " << m_path << "] WRITE UNLOCKED(ABORTED)", LOG_LEVEL_3);
          }
        }
          

      }
      LOG_PRINT_L4("[DB] Transaction aborted");
    }

    bool lmdb_db_backend::erase(container_handle h, const char* k, size_t ks)
    {
      int res = 0;
      MDB_val key = AUTO_VAL_INIT(key);
      key.mv_data = (void*)k;
      key.mv_size = ks;

      res = mdb_del(get_current_tx(), static_cast<MDB_dbi>(h), &key, nullptr);
      if (res == MDB_NOTFOUND)
        return false;
      CHECK_AND_ASSERT_MESS_LMDB_DB(res, false, "Unable to mdb_del");
      return true;
    }

    bool lmdb_db_backend::have_tx()
    {
      std::lock_guard<boost::recursive_mutex> lock(m_cs);
      auto it = m_txs.find(std::this_thread::get_id());
      if (it == m_txs.end())
        return false;
      return it->second.size() ? true : false;
    }

    bool lmdb_db_backend::get(container_handle h, const char* k, size_t ks, std::string& res_buff)
    {
      PROFILE_FUNC("lmdb_db_backend::get");
      int res = 0;
      MDB_val key = AUTO_VAL_INIT(key);
      MDB_val data = AUTO_VAL_INIT(data);
      key.mv_data = (void*)k;
      key.mv_size = ks;
      bool need_to_commit = false;
      if (!have_tx())
      {
        need_to_commit = true;
        begin_transaction(true);
      }

      res = mdb_get(get_current_tx(), static_cast<MDB_dbi>(h), &key, &data);

      if (need_to_commit)
        commit_transaction();

      if (res == MDB_NOTFOUND)
        return false;

      CHECK_AND_ASSERT_MESS_LMDB_DB(res, false, "Unable to mdb_get, h: " << h << ", ks: " << ks);
      res_buff.assign((const char*)data.mv_data, data.mv_size);
      return true;
    }

    bool lmdb_db_backend::clear(container_handle h)
    {
      int res = mdb_drop(get_current_tx(), static_cast<MDB_dbi>(h), 0);
      CHECK_AND_ASSERT_MESS_LMDB_DB(res, false, "Unable to mdb_drop");
      return true;
    }
    
    uint64_t lmdb_db_backend::size(container_handle h)
    {
      PROFILE_FUNC("lmdb_db_backend::size");
      MDB_stat container_stat = AUTO_VAL_INIT(container_stat);
      bool need_to_commit = false;
      if (!have_tx())
      {
        need_to_commit = true;
        begin_transaction(true);
      }
      int res = mdb_stat(get_current_tx(), static_cast<MDB_dbi>(h), &container_stat);
      if (need_to_commit)
        commit_transaction();
      CHECK_AND_ASSERT_MESS_LMDB_DB(res, false, "Unable to mdb_stat");
      return container_stat.ms_entries;
    }

    bool lmdb_db_backend::set(container_handle h, const char* k, size_t ks, const char* v, size_t vs)
    {
      PROFILE_FUNC("lmdb_db_backend::set");
      int res = 0;
      MDB_val key = AUTO_VAL_INIT(key);
      MDB_val data = AUTO_VAL_INIT(data);
      key.mv_data = (void*)k;
      key.mv_size = ks;
      data.mv_data = (void*)v;
      data.mv_size = vs;

      res = mdb_put(get_current_tx(), static_cast<MDB_dbi>(h), &key, &data, 0);
      CHECK_AND_ASSERT_MESS_LMDB_DB(res, false, "Unable to mdb_put");
      return true;
    }

    bool lmdb_db_backend::enumerate(container_handle h, i_db_callback* pcb)
    {
      CHECK_AND_ASSERT_MES(pcb, false, "null capback ptr passed to enumerate");
      MDB_val key = AUTO_VAL_INIT(key);
      MDB_val data = AUTO_VAL_INIT(data);

      bool need_to_commit = false;
      if (!have_tx())
      {
        need_to_commit = true;
        begin_transaction(true);
      }
      MDB_cursor* cursor_ptr = nullptr;
      int res = mdb_cursor_open(get_current_tx(), static_cast<MDB_dbi>(h), &cursor_ptr);
      CHECK_AND_ASSERT_MESS_LMDB_DB(res, false, "Unable to mdb_cursor_open");
      CHECK_AND_ASSERT_MES(cursor_ptr, false, "cursor_ptr is null after mdb_cursor_open");

      uint64_t count = 0;
      do
      {
        int res = mdb_cursor_get(cursor_ptr, &key, &data, MDB_NEXT);
        if (res == MDB_NOTFOUND)
          break;
        if (!pcb->on_enum_item(count, key.mv_data, key.mv_size, data.mv_data, data.mv_size))
          break;
        count++;
      } while (cursor_ptr);

      mdb_cursor_close(cursor_ptr);
      if (need_to_commit)
        commit_transaction();
      return true;
    }

    bool lmdb_db_backend::get_stat_info(tools::db::stat_info& si)
    {
      si = AUTO_VAL_INIT_T(tools::db::stat_info);

      MDB_envinfo ei = AUTO_VAL_INIT(ei);
      mdb_env_info(m_penv, &ei);
      si.map_size = ei.me_mapsize;
      
      std::lock_guard<boost::recursive_mutex> lock(m_cs);
      for (auto& e : m_txs)
      {
        for (auto& pr : e.second)
        {
          ++si.tx_count;
          if(!pr.read_only)
            ++si.write_tx_count;
        }
      }
      return true;
    }

    bool lmdb_db_backend::resize_if_needed()
    {
      LOG_PRINT_CYAN("[DB " << m_path << "] WRITE LOCKED in resize_if_needed()", LOG_LEVEL_3);
      CRITICAL_REGION_LOCAL(m_write_exclusive_lock);

      if (have_tx())
      {
        LOG_PRINT_RED("[DB " << m_path << "] : resize_if_needed(): Have txs on stack, unable to resize!", LOG_LEVEL_0);
        return false;
      }

      MDB_stat st = AUTO_VAL_INIT(st);
      mdb_env_stat(m_penv, &st);
      MDB_envinfo ei = AUTO_VAL_INIT(ei);
      mdb_env_info(m_penv, &ei);

      uint64_t dirty_size = ei.me_last_pgno * st.ms_psize;
      int64_t size_diff = ei.me_mapsize - dirty_size;
      if (size_diff >= DB_RESIZE_MIN_FREE_SIZE && ei.me_mapsize >= DB_RESIZE_MIN_MAX_SIZE)
        return true; // resize is not needed

      double gigabyte = 1024 * 1024 * 1024;
      const uint64_t increment_size_pg_aligned = DB_RESIZE_INCREMENT_SIZE - (DB_RESIZE_INCREMENT_SIZE % st.ms_psize);

      // need to resize DB
      uint64_t new_size = ei.me_mapsize - (ei.me_mapsize % increment_size_pg_aligned) + increment_size_pg_aligned;

      int res = mdb_env_set_mapsize(m_penv, new_size);
      CHECK_AND_ASSERT_MESS_LMDB_DB(res, false, "Unable to mdb_env_set_mapsize");

      LOG_PRINT_CYAN("[DB " << m_path << "] has grown: " << std::fixed << std::setprecision(2) << ei.me_mapsize / gigabyte << " GiB  ->  " << std::fixed << std::setprecision(2) << new_size / gigabyte << " GiB", LOG_LEVEL_0);

      return true;
    }

  }
}

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL NULL
