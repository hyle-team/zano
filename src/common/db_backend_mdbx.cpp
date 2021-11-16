// Copyright (c) 2014-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project 
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifdef ENABLED_ENGINE_MDBX
#include "db_backend_mdbx.h"
#include "misc_language.h"
#include "string_coding.h"
#include "profile_tools.h"
#include "util.h"


#define BUF_SIZE 1024

#define CHECK_AND_ASSERT_MESS_MDBX_DB(rc, ret, mess) CHECK_AND_ASSERT_MES(res == MDBX_SUCCESS, ret, "[DB ERROR]:(" << rc << ")" << mdbx_strerror(rc) << ", [message]: " << mess);
#define CHECK_AND_ASSERT_THROW_MESS_MDBX_DB(rc, mess) CHECK_AND_ASSERT_THROW_MES(res == MDBX_SUCCESS, "[DB ERROR]:(" << rc << ")" << mdbx_strerror(rc) << ", [message]: " << mess);
#define ASSERT_MES_AND_THROW_MDBX(rc, mess) ASSERT_MES_AND_THROW("[DB ERROR]:(" << rc << ")" << mdbx_strerror(rc) << ", [message]: " << mess);

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL "mdbx"
// 'mdbx' channel is disabled by default

namespace tools
{
  namespace db
  {
    mdbx_db_backend::mdbx_db_backend() : m_penv(AUTO_VAL_INIT(m_penv))  
    {

    }
    mdbx_db_backend::~mdbx_db_backend()
    {
      NESTED_TRY_ENTRY();

      close();

      NESTED_CATCH_ENTRY(__func__);
    }

    bool mdbx_db_backend::open(const std::string& path_, uint64_t cache_sz)
    {
      int res = 0;
      res = mdbx_env_create(&m_penv);
      CHECK_AND_ASSERT_MESS_MDBX_DB(res, false, "Unable to mdbx_env_create");
      
      res = mdbx_env_set_maxdbs(m_penv, 15);
      CHECK_AND_ASSERT_MESS_MDBX_DB(res, false, "Unable to mdbx_env_set_maxdbs");

      intptr_t size_lower = 0;
      intptr_t size_now = -1;            //don't change current database size
      intptr_t size_upper = 0x10000000000;          //don't set db file size limit
      intptr_t growth_step = 0x40000000; //increment step 1GB
      intptr_t shrink_threshold = -1;
      intptr_t pagesize = 0x00001000;   //4kb
      res = mdbx_env_set_geometry(m_penv, size_lower, size_now, size_upper, growth_step, shrink_threshold, pagesize);
      CHECK_AND_ASSERT_MESS_MDBX_DB(res, false, "Unable to mdbx_env_set_mapsize");
      
      m_path = path_;
      CHECK_AND_ASSERT_MES(tools::create_directories_if_necessary(m_path), false, "create_directories_if_necessary failed: " << m_path);

      res = mdbx_env_open(m_penv, m_path.c_str(), MDBX_NORDAHEAD , 0644);
      CHECK_AND_ASSERT_MESS_MDBX_DB(res, false, "Unable to mdbx_env_open, m_path=" << m_path);
      
      return true;
    }

    bool mdbx_db_backend::open_container(const std::string& name, container_handle& h)
    {
      MDBX_dbi dbi = AUTO_VAL_INIT(dbi);
      begin_transaction();
      int res = mdbx_dbi_open(get_current_tx(), name.c_str(), MDBX_CREATE, &dbi);
      CHECK_AND_ASSERT_MESS_MDBX_DB(res, false, "Unable to mdbx_dbi_open with container name: " << name);
      commit_transaction();
      h = static_cast<container_handle>(dbi);
      return true;
    }

    bool mdbx_db_backend::close_container(container_handle& h)
    {
      static const container_handle null_handle = AUTO_VAL_INIT(null_handle);
      CHECK_AND_ASSERT_MES(h != null_handle, false, "close_container is called for null container handle");

      MDBX_dbi dbi = static_cast<MDBX_dbi>(h);
      begin_transaction();
      mdbx_dbi_close(m_penv, dbi);
      commit_transaction();
      h = null_handle;
      return true;
    }

    bool mdbx_db_backend::close()
    {
      {
        std::lock_guard<boost::recursive_mutex> lock(m_cs);
        for (auto& tx_thread : m_txs)
        {
          for (auto txe : tx_thread.second)
          {
            int res = mdbx_txn_commit(txe.ptx);
            if (res != MDBX_SUCCESS)
            {
              LOG_ERROR("[DB ERROR]: On close tranactions: " << mdbx_strerror(res));
            }
          }
        }

        m_txs.clear();
      }
      if (m_penv)
      {
        mdbx_env_close(m_penv);
        m_penv = nullptr;
      }
      return true;
    }

    bool mdbx_db_backend::begin_transaction(bool read_only)
    {
      if (!read_only)
      {
        LOG_PRINT_CYAN("[DB " << m_path << "] WRITE LOCKED", LOG_LEVEL_3);
        CRITICAL_SECTION_LOCK(m_write_exclusive_lock);
      }
      PROFILE_FUNC("mdbx_db_backend::begin_transaction");
      {
        std::lock_guard<boost::recursive_mutex> lock(m_cs);
        CHECK_AND_ASSERT_THROW_MES(m_penv, "m_penv==null, db closed");
        transactions_list& rtxlist = m_txs[std::this_thread::get_id()];
        MDBX_txn* pparent_tx = nullptr;
        MDBX_txn* p_new_tx = nullptr;
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
          //MDBX_txn_flags_t flags = MDBX_txn_flags_t();
          unsigned int flags = 0;
          if (read_only)
            flags = MDBX_RDONLY;//flags = MDBX_TXN_RDONLY;

          //don't use parent tx in write transactions if parent tx was read-only (restriction in mdbx) 
          //see "Nested transactions: Max 1 child, write txns only, no writemap"
          if (pparent_tx && parent_read_only)
            pparent_tx = nullptr;

          CHECK_AND_ASSERT_THROW_MES(m_penv, "m_penv==null, db closed");
          res = mdbx_txn_begin(m_penv, pparent_tx, flags, &p_new_tx);
          if(res != MDBX_SUCCESS)
          {
            //Important: if mdbx_txn_begin is failed need to unlock previously locked mutex
            CRITICAL_SECTION_UNLOCK(m_write_exclusive_lock);
            //throw exception to avoid regular code execution 
            ASSERT_MES_AND_THROW_MDBX(res, "Unable to mdbx_txn_begin");
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

    MDBX_txn* mdbx_db_backend::get_current_tx()
    {
      std::lock_guard<boost::recursive_mutex> lock(m_cs);
      auto& rtxlist = m_txs[std::this_thread::get_id()];
      CHECK_AND_ASSERT_MES(rtxlist.size(), nullptr, "Unable to find active tx for thread " << std::this_thread::get_id());
      return rtxlist.back().ptx;
    }

    bool mdbx_db_backend::pop_tx_entry(tx_entry& txe)
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

    bool mdbx_db_backend::commit_transaction()
    {
      PROFILE_FUNC("mdbx_db_backend::commit_transaction");
      {
        tx_entry txe = AUTO_VAL_INIT(txe);
        bool r = pop_tx_entry(txe);
        CHECK_AND_ASSERT_MES(r, false, "Unable to pop_tx_entry");
          
        if (txe.count == 0 || (txe.read_only && txe.count == 1))
        {
          int res = 0;
          res = mdbx_txn_commit(txe.ptx);
          CHECK_AND_ASSERT_MESS_MDBX_DB(res, false, "Unable to mdbx_txn_commit (error " << res << ")");
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
    
    void mdbx_db_backend::abort_transaction()
    {
      {
        tx_entry txe = AUTO_VAL_INIT(txe);
        bool r = pop_tx_entry(txe);
        CHECK_AND_ASSERT_MES(r, void(), "Unable to pop_tx_entry");
        if (txe.count == 0 || (txe.read_only && txe.count == 1))
        {
          mdbx_txn_abort(txe.ptx);
          if (!txe.read_only && !txe.count)
          {
            CRITICAL_SECTION_UNLOCK(m_write_exclusive_lock);
            LOG_PRINT_CYAN("[DB " << m_path << "] WRITE UNLOCKED(ABORTED)", LOG_LEVEL_3);
          }
        }
          

      }
      LOG_PRINT_L4("[DB] Transaction aborted");
    }

    bool mdbx_db_backend::erase(container_handle h, const char* k, size_t ks)
    {
      int res = 0;
      MDBX_val key = AUTO_VAL_INIT(key);
      key.iov_base = (void*)k;
      key.iov_len = ks;

      res = mdbx_del(get_current_tx(), static_cast<MDBX_dbi>(h), &key, nullptr);
      if (res == MDBX_NOTFOUND)
        return false;
      CHECK_AND_ASSERT_MESS_MDBX_DB(res, false, "Unable to mdbx_del");
      return true;
    }

    bool mdbx_db_backend::have_tx()
    {
      std::lock_guard<boost::recursive_mutex> lock(m_cs);
      auto it = m_txs.find(std::this_thread::get_id());
      if (it == m_txs.end())
        return false;
      return it->second.size() ? true : false;
    }

    bool mdbx_db_backend::get(container_handle h, const char* k, size_t ks, std::string& res_buff)
    {
      PROFILE_FUNC("mdbx_db_backend::get");
      int res = 0;
      MDBX_val key = AUTO_VAL_INIT(key);
      MDBX_val data = AUTO_VAL_INIT(data);
      key.iov_base = (void*)k;
      key.iov_len = ks;
      bool need_to_commit = false;
      if (!have_tx())
      {
        need_to_commit = true;
        begin_transaction(true);
      }

      res = mdbx_get(get_current_tx(), static_cast<MDBX_dbi>(h), &key, &data);

      if (need_to_commit)
        commit_transaction();

      if (res == MDBX_NOTFOUND)
        return false;

      CHECK_AND_ASSERT_MESS_MDBX_DB(res, false, "Unable to mdbx_get, h: " << h << ", ks: " << ks);
      res_buff.assign((const char*)data.iov_base, data.iov_len);
      return true;
    }

    bool mdbx_db_backend::clear(container_handle h)
    {
      int res = mdbx_drop(get_current_tx(), static_cast<MDBX_dbi>(h), 0);
      CHECK_AND_ASSERT_MESS_MDBX_DB(res, false, "Unable to mdbx_drop");
      return true;
    }
    
    uint64_t mdbx_db_backend::size(container_handle h)
    {
      PROFILE_FUNC("mdbx_db_backend::size");
      MDBX_stat container_stat = AUTO_VAL_INIT(container_stat);
      bool need_to_commit = false;
      if (!have_tx())
      {
        need_to_commit = true;
        begin_transaction(true);
      }
      int res = mdbx_dbi_stat(get_current_tx(), static_cast<MDBX_dbi>(h), &container_stat, sizeof(MDBX_stat));
      if (need_to_commit)
        commit_transaction();
      CHECK_AND_ASSERT_MESS_MDBX_DB(res, false, "Unable to mdbx_stat");
      return container_stat.ms_entries;
    }

    bool mdbx_db_backend::set(container_handle h, const char* k, size_t ks, const char* v, size_t vs)
    {
      PROFILE_FUNC("mdbx_db_backend::set");
      int res = 0;
      MDBX_val key = AUTO_VAL_INIT(key);
      MDBX_val data[2] = {}; // mdbx_put may access data[1] if some flags are set, this may trigger static code analizers, so here we allocate two elements to avoid it
      key.iov_base = (void*)k;
      key.iov_len = ks;
      data[0].iov_base = (void*)v;
      data[0].iov_len = vs;

      //MDBX_put_flags_t flags = MDBX_put_flags_t();
      unsigned  flags = 0;
      res = mdbx_put(get_current_tx(), static_cast<MDBX_dbi>(h), &key, data, flags);
      CHECK_AND_ASSERT_MESS_MDBX_DB(res, false, "Unable to mdbx_put");
      return true;
    }
    bool mdbx_db_backend::enumerate(container_handle h, i_db_callback* pcb)
    {
      CHECK_AND_ASSERT_MES(pcb, false, "null capback ptr passed to enumerate");
      MDBX_val key = AUTO_VAL_INIT(key);
      MDBX_val data = AUTO_VAL_INIT(data);

      bool need_to_commit = false;
      if (!have_tx())
      {
        need_to_commit = true;
        begin_transaction(true);
      }
      MDBX_cursor* cursor_ptr = nullptr;
      int res = mdbx_cursor_open(get_current_tx(), static_cast<MDBX_dbi>(h), &cursor_ptr);
      CHECK_AND_ASSERT_MESS_MDBX_DB(res, false, "Unable to mdbx_cursor_open");
      CHECK_AND_ASSERT_MES(cursor_ptr, false, "cursor_ptr is null after mdbx_cursor_open");

      uint64_t count = 0;
      do
      {
        int res = mdbx_cursor_get(cursor_ptr, &key, &data, MDBX_NEXT);
        if (res == MDBX_NOTFOUND)
          break;
        if (!pcb->on_enum_item(count, key.iov_base, key.iov_len, data.iov_base, data.iov_len))
          break;
        count++;
      } while (cursor_ptr);

      mdbx_cursor_close(cursor_ptr);
      if (need_to_commit)
        commit_transaction();
      return true;
    }

    bool mdbx_db_backend::get_stat_info(tools::db::stat_info& si)
    {
      si = AUTO_VAL_INIT_T(tools::db::stat_info);

      MDBX_envinfo ei = AUTO_VAL_INIT(ei);
      mdbx_env_info(m_penv, &ei, sizeof(MDBX_envinfo));
      si.map_size = ei.mi_mapsize;
      
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
    const char* mdbx_db_backend::name()
    {
      return "mdbx";
    }
  }
}

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL NULL
#endif