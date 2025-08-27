// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project 
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "db_backend_lmdb.h"
#include "misc_language.h"
#include "string_coding.h"
#include "profile_tools.h"
#include "util.h"
#include <boost/stacktrace.hpp>

#define BUF_SIZE 1024

#define CHECK_AND_ASSERT_MESS_LMDB_DB(rc, ret, mess) CHECK_AND_ASSERT_MES(rc == MDB_SUCCESS, ret, "[DB ERROR]:(" << rc << ")" << mdb_strerror(rc) << ", [message]: " << mess);
#define CHECK_AND_ASSERT_THROW_MESS_LMDB_DB(rc, mess) CHECK_AND_ASSERT_THROW_MES(rc == MDB_SUCCESS, "[DB ERROR]:(" << rc << ")" << mdb_strerror(rc) << ", [message]: " << mess);
#define ASSERT_MES_AND_THROW_LMDB(rc, mess) ASSERT_MES_AND_THROW("[DB ERROR]:(" << rc << ")" << mdb_strerror(rc) << ", [message]: " << mess);

#define BEGIN_TX_LOCAL(is_read_only)   db_transaction_wrapper<lmdb_db_backend> local_tx(*this);  local_tx.begin_transaction(is_read_only);
#define COMMIT_TX_LOCAL()   local_tx.commit_transaction();



// #undef LOG_DEFAULT_CHANNEL 
// #define LOG_DEFAULT_CHANNEL "lmdb"
// 'lmdb' channel is disabled by default
static void print_stacktrace()
{
  LOG_PRINT_L0("Stacktrace \n" << boost::stacktrace::stacktrace());
}


//@#@
#define LMDB_JOURNAL_LIMIT 10000
std::list<std::string> lmdb_journal;
epee::critical_section lmdb_cs;

extern "C" void print_log_to_journal(const char* message)
{
  CRITICAL_REGION_LOCAL(lmdb_cs);
  lmdb_journal.push_back( epee::log_space::get_prefix_entry() + std::string(message));
  if (lmdb_journal.size() > LMDB_JOURNAL_LIMIT)
    lmdb_journal.pop_front();
  //LOG_PRINT_L0("[LMDB] " << message);
}

void print_journal(size_t limit)
{
  CRITICAL_REGION_LOCAL(lmdb_cs);
  std::stringstream ss;
  size_t count = 0;
  for (auto it = lmdb_journal.begin(); it != lmdb_journal.end(); it++)
  {
    ss << *it << ENDL;
    ++count;
    //if (count > limit)
    //  break;
  }
  LOG_PRINT_L0("JOUNRAL: " << ENDL << ss.str());
}
//!@#@


namespace tools
{
  namespace db
  {
    static inline std::string tid_str()
    {
      std::ostringstream os; os << std::this_thread::get_id(); return os.str();
    }

    static std::atomic<uint64_t> g_tx_dbg_counter{0};

    static int print_reader(const char* s, void*) 
    { 
      LOG_PRINT_L0(std::string(s));
      return 0;
    }

    void dump_readers(MDB_env* env)
    {
      int dead = 0;
      mdb_reader_check(env, &dead);
      LOG_PRINT_L0("purged dead readers: " << dead);
      mdb_reader_list(env, print_reader, nullptr);
    }

    std::string lmdb_db_backend::dump_tx_stacks()
    {
      std::lock_guard<boost::recursive_mutex> lock(m_cs);
      
      std::stringstream ss;
      ss << "== TX STACKS ==" << ENDL;

      for (const auto& kv : m_txs) 
      {
        const auto& thr_txs = kv.second; // transactions_list = std::list<lmdb_txn>
        ss << "  tid=" << kv.first << " depth=" << thr_txs.size() <<ENDL;

        size_t i = 0;
        for (const auto& t : lst) {
          LOG_PRINT_L0("[" << i++ << "] id=" << t.dbg_id
            << " ro=" << (t.read_only ? 1 : 0)
//            << " lvl=" << t.stack_level
//            << " nested=" << t.nested_count
            << " ptx=" << (void*)t.ptx);
        }
      }
    }

    lmdb_db_backend::lmdb_db_backend() : m_penv(AUTO_VAL_INIT(m_penv))  
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

      res = mdb_env_set_mapsize(m_penv, cache_sz);
      CHECK_AND_ASSERT_MESS_LMDB_DB(res, false, "Unable to mdb_env_set_mapsize");
      
      m_path = path_;
      CHECK_AND_ASSERT_MES(tools::create_directories_if_necessary(m_path), false, "create_directories_if_necessary failed: " << m_path);

      res = mdb_env_open(m_penv, m_path.c_str(), MDB_NORDAHEAD , 0644);
      CHECK_AND_ASSERT_MESS_LMDB_DB(res, false, "Unable to mdb_env_open, m_path=" << m_path);
      
      return true;
    }

    bool lmdb_db_backend::open_container(const std::string& name, container_handle& h)
    {
      MDB_dbi dbi = AUTO_VAL_INIT(dbi);
      BEGIN_TX_LOCAL(false);
      int res = mdb_dbi_open(get_current_tx(), name.c_str(), MDB_CREATE, &dbi);
      CHECK_AND_ASSERT_MESS_LMDB_DB(res, false, "Unable to mdb_dbi_open with container name: " << name);
      COMMIT_TX_LOCAL();
      h = static_cast<container_handle>(dbi);
      return true;
    }

    bool lmdb_db_backend::close_container(container_handle& h)
    {
      static const container_handle null_handle = AUTO_VAL_INIT(null_handle);
      CHECK_AND_ASSERT_MES(h != null_handle, false, "close_container is called for null container handle");
      MDB_dbi dbi = static_cast<MDB_dbi>(h);
      BEGIN_TX_LOCAL(false);
      mdb_dbi_close(m_penv, dbi);
      COMMIT_TX_LOCAL();
      h = null_handle;
      return true;
    }

    void lmdb_db_backend::dump()
    {
      std::lock_guard<boost::recursive_mutex> lock(m_cs);
      dump_readers(m_penv);
    }

    bool lmdb_db_backend::close()
    {
     
      {
        std::lock_guard<boost::recursive_mutex> lock(m_cs);
        for (auto& tx_thread : m_txs)
        {          
          for (auto& txe : tx_thread.second.m_txs)
          {
            if (!txe.m_read_only)
            {
              LOG_ERROR("not cleared W-transactions on db close");
            }
          }

          //for (auto& txe : tx_thread.second)
          //{
          //  int res = MDB_SUCCESS;
            //if(txe.read_only)
            //  txe.abort();
            //else
         //     res = txe.commit();

          //  if (res != MDB_SUCCESS)
          //  {
          //    LOG_ERROR("[DB ERROR]: On close tranactions: " << mdb_strerror(res));
          //  }
          //}
        }
        //m_txs./ clear();
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
      //@#@
      std::string messg = "[BACK] begin_transaction(";
      if (read_only) messg += "ro";
      messg += ")";
      print_log_to_journal(messg.c_str());
      //!@#@

      if (!read_only)
      {
        LOG_PRINT_CYAN("[DB " << m_path << "] WRITE EXCLUSIVE LOCKED", LOG_LEVEL_3);
        CRITICAL_SECTION_LOCK(m_write_exclusive_lock);
      }


      PROFILE_FUNC("lmdb_db_backend::begin_transaction");
      {
        std::lock_guard<boost::recursive_mutex> lock(m_cs);
        CHECK_AND_ASSERT_THROW_MES(m_penv, "m_penv==null, db closed");
        thread_transactions& thread_txs = m_txs[std::this_thread::get_id()];
        
        MDB_txn* pparent_tx = nullptr;
        
        //do we have to create new lmdb_txn object(and actually call for lmdb api)?         
        if (!read_only || !thread_txs.m_txs.size())
        {
          //now let's figure out parent_tx thing
          if (!read_only && thread_txs.m_txs.size())
          {
            //if there are W-transactions in stack, it always the last one, ar RO-transactions never created on top of W-transactions
            if (!thread_txs.m_txs.back()->m_read_only)
            {
              //assign native lmdb handle
              pparent_tx = thread_txs.m_txs.back()->m_ptx;
            }
          }

          //either RO-transaction with no prev transactions or W-transaction, create new lmdb_txn object for m_txs
          thread_txs.m_txs.push_back(std::shared_ptr<lmdb_txn>( new lmdb_txn(*this, read_only)));
        }

        std::shared_ptr<lmdb_txn> current_tx_pointer = current_tx_pointer = thread_txs.m_txs.back();


        int res = current_tx_pointer->begin(pparent_tx);
        if (res != MDB_SUCCESS)
        {
          if (!current_tx_pointer->m_read_only)
          {
            //Important: if mdb_txn_begin is failed need to unlock previously locked mutex
            CRITICAL_SECTION_UNLOCK(m_write_exclusive_lock);
          }
                    
          if (current_tx_pointer->is_empty())
          {
            thread_txs.m_txs.pop_back();
          }
          else
          {
            LOG_ERROR("current_tx_pointer non empty on begin() failure");
          }


          //throw exception to avoid regular code execution 
          LOG_PRINT_L0("[LMDB] OPEN FAIL tid=" << std::this_thread::get_id() << " ro=" << (current_tx_pointer->m_read_only ? 1 : 0)
            << " thread_txs.m_txs=" << thread_txs.m_txs.size());
          dump();
          dump_tx_stacks();
          print_stacktrace();
          if (!m_journal_printed)
          {
            print_journal(10000);
            m_journal_printed = true;
          }

          ASSERT_MES_AND_THROW_LMDB(res, "Unable to mdb_txn_begin");
        }
      LOG_PRINT_L4("[DB] Transaction started");
      return true;
    }

    MDB_txn* lmdb_db_backend::get_current_tx()
    {
      std::lock_guard<boost::recursive_mutex> lock(m_cs);
      auto& thrd_txs = m_txs[std::this_thread::get_id()];
      CHECK_AND_ASSERT_MES(thrd_txs.m_txs.size(), nullptr, "Unable to find active tx for thread " << std::this_thread::get_id());
      return thrd_txs.m_txs.back()->m_ptx;
    }


    bool lmdb_db_backend::commit_transaction()
    {
      print_log_to_journal("[BACK] commit_transaction()");
      PROFILE_FUNC("lmdb_db_backend::commit_transaction");
      {
        lmdb_txn txe(*this);
        bool r = pop_lmdb_txn(txe);
        CHECK_AND_ASSERT_MES(r, false, "Unable to pop_lmdb_txn");
        
        if (!txe.ptx)
        {
          LOG_PRINT_L4("[DB] Nested transaction scope closed");
          return true;
        }

        if (txe.nested_count == 0 || (txe.read_only && txe.nested_count == 1))
        {
          if(txe.read_only)
          {
            txe.abort();
          }
          else
          {
            int res = txe.commit();
            CHECK_AND_ASSERT_MESS_LMDB_DB(res, false, "Unable to commit transaction(error " << res << ")");
            if (!txe.read_only && !txe.nested_count)
            {
              CRITICAL_SECTION_UNLOCK(m_write_exclusive_lock);
              LOG_PRINT_CYAN("[DB " << m_path << "] WRITE UNLOCKED", LOG_LEVEL_3);
            }
          }
        }
      }
      LOG_PRINT_L4("[DB] Transaction committed");
      return true;
    }
    
    void lmdb_db_backend::abort_transaction()
    {
      print_log_to_journal("[BACK] abort_transaction()");
      {
        lmdb_txn txe(*this);
        bool r = pop_lmdb_txn(txe);
        CHECK_AND_ASSERT_MES(r, void(), "Unable to pop_lmdb_txn");
        if (txe.nested_count == 0 || (txe.read_only && txe.nested_count == 1))
        {
          txe.abort();
          if (!txe.read_only && !txe.nested_count)
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
      BEGIN_TX_LOCAL(true);
//      bool need_to_commit = false;
//       if (!have_tx())
//       {
//         need_to_commit = true;
//         begin_transaction(true);
//       }

      res = mdb_get(get_current_tx(), static_cast<MDB_dbi>(h), &key, &data);

//      if (need_to_commit)
//        commit_transaction();
      COMMIT_TX_LOCAL();

      if (res == MDB_NOTFOUND)
        return false;
      if (res != MDB_SUCCESS)
      {
        return false;
      }

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
      BEGIN_TX_LOCAL(true);
//      bool need_to_commit = false;
//      if (!have_tx())
//      {
//        need_to_commit = true;
//        begin_transaction(true);
//      }
      int res = mdb_stat(get_current_tx(), static_cast<MDB_dbi>(h), &container_stat);

//      if (need_to_commit)
//        commit_transaction();
      COMMIT_TX_LOCAL();
      CHECK_AND_ASSERT_MESS_LMDB_DB(res, false, "Unable to mdb_stat");
      return container_stat.ms_entries;
    }

    bool lmdb_db_backend::set(container_handle h, const char* k, size_t ks, const char* v, size_t vs)
    {
      PROFILE_FUNC("lmdb_db_backend::set");
      int res = 0;
      MDB_val key = AUTO_VAL_INIT(key);
      MDB_val data[2] = {}; // mdb_put may access data[1] if some flags are set, this may trigger static code analizers, so here we allocate two elements to avoid it
      key.mv_data = (void*)k;
      key.mv_size = ks;
      data[0].mv_data = (void*)v;
      data[0].mv_size = vs;

      res = mdb_put(get_current_tx(), static_cast<MDB_dbi>(h), &key, data, 0);
      CHECK_AND_ASSERT_MESS_LMDB_DB(res, false, "Unable to mdb_put");
      return true;
    }
    bool lmdb_db_backend::enumerate(container_handle h, i_db_callback* pcb)
    {
      CHECK_AND_ASSERT_MES(pcb, false, "null capback ptr passed to enumerate");
      MDB_val key = AUTO_VAL_INIT(key);
      MDB_val data = AUTO_VAL_INIT(data);

      BEGIN_TX_LOCAL(true);
//      bool need_to_commit = false;
//      if (!have_tx())
//      {
//        need_to_commit = true;
//        begin_transaction(true);
//      }
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
//      if (need_to_commit)
//        commit_transaction();
      COMMIT_TX_LOCAL();
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

    const char* lmdb_db_backend::name()
    {
      return "lmdb";
    }

    bool lmdb_db_backend::convert_db_4kb_page_to_16kb_page(const std::string& source_path, const std::string& destination_path)
    {
      #define MDB_CHECK(x, msg) {int rc = x; CHECK_AND_ASSERT_MES(rc == MDB_SUCCESS, false, "LMDB 4k->16k error: " << msg << ": " << mdb_strerror(rc));}

      MDB_env *env_src = nullptr, *env_dst = nullptr;

      // source
      MDB_CHECK(mdb_env_create(&env_src), "failed to create LMDB environment");
      MDB_CHECK(mdb_env_set_mapsize(env_src, 4 * 1024 * 1024), "failed to set mapsize"); // mapsize ?
      MDB_CHECK(mdb_env_open(env_src, source_path.c_str(), 0, 0664), "failed to open source LMDB");

      // destination (16k page size)
      MDB_CHECK(mdb_env_create(&env_dst), "failed to create LMDB environment");
      MDB_CHECK(mdb_env_set_mapsize(env_dst, 16 * 1024 * 1024), "failed to set mapsize"); // mapsize ?
      
      // TODO uncomment after mdb_env_set_pagesize is supported
      // MDB_CHECK(mdb_env_set_pagesize(env_dst, 16 * 1024), "failed to set page size to 16K");
      
      MDB_CHECK(mdb_env_open(env_dst, destination_path.c_str(), 0, 0664), "failed to open destination LMDB");

      // begin transactions
      MDB_txn *txn_src = nullptr, *txn_dst = nullptr;
      MDB_dbi dbi_src, dbi_dst;
      MDB_CHECK(mdb_txn_begin(env_src, nullptr, MDB_RDONLY, &txn_src), "failed to begin source transaction");
      MDB_CHECK(mdb_dbi_open(txn_src, nullptr, 0, &dbi_src), "failed to open source database");
      MDB_CHECK(mdb_txn_begin(env_dst, nullptr, 0, &txn_dst), "failed to begin destination transaction");
      MDB_CHECK(mdb_dbi_open(txn_dst, nullptr, MDB_CREATE, &dbi_dst), "failed to open destination database");

      MDB_cursor *cursor;
      MDB_val key, data;

      // Iterate over the source database and copy all key-value pairs to the destination database
      MDB_CHECK(mdb_cursor_open(txn_src, dbi_src, &cursor), "failed to open cursor");

      while (mdb_cursor_get(cursor, &key, &data, MDB_NEXT) == MDB_SUCCESS)
      {
        MDB_CHECK(mdb_put(txn_dst, dbi_dst, &key, &data, 0), "failed to put data in destination database");
      }

      mdb_cursor_close(cursor);

      // commit transactions
      MDB_CHECK(mdb_txn_commit(txn_src), "failed to commit source transaction");
      MDB_CHECK(mdb_txn_commit(txn_dst), "failed to commit destination transaction");

      mdb_dbi_close(env_src, dbi_src);
      mdb_dbi_close(env_dst, dbi_dst);
      mdb_env_close(env_src);
      mdb_env_close(env_dst);

      return true;

      #undef MDB_CHECK
    }

    lmdb_db_backend::lmdb_txn::lmdb_txn(lmdb_db_backend& db)
      : m_db(db)
    {}

    lmdb_db_backend::lmdb_txn::lmdb_txn(lmdb_db_backend& db, bool is_read_only)
      : m_db(db)
    {
      m_read_only = is_read_only;
    }


    
    void lmdb_db_backend::lmdb_txn::check_thread_id()
    {
      if (m_owner != std::this_thread::get_id())
      {
        LOG_ERROR("[lmdb] thread_id thread(" << std::this_thread::get_id() << ") different from owner thread(" << m_owner << ")");
//        m_db.get().dump_tx_stacks();
//        print_stacktrace();
        CHECK_AND_ASSERT_THROW_MES(false, "[lmdb] cross-thread commit");
      }
    }
    
    int lmdb_db_backend::lmdb_txn::begin(MDB_txn* parent_tx)
    {
      if (m_ref_count > 0)
      {
        check_thread_id();
        ++m_ref_count;
        return MDB_SUCCESS;
      }

      //actually opening
      m_owner = std::this_thread::get_id();
      unsigned int flags = m_read_only ? MDB_RDONLY : 0;
      int response = mdb_txn_begin(m_db.get().m_penv, parent_tx, flags, &m_ptx);
      if (response == MDB_SUCCESS)
      {
        m_ref_count = 1;
      }
    }
    
    bool is_empty()
    {
      return m_ref_count == 0;
    }

    
    int lmdb_db_backend::lmdb_txn::commit()
    {
      check_thread_id();

      --m_ref_count;

      if (m_ref_count != 0)
      {
        return MDB_SUCCESS;
      }

      //actually closing tx
      if (!m_ptx)
      {
        CHECK_AND_ASSERT_THROW_MES(false, "[DB ERROR]: m_ptx oiss NULL on commit");
      }
      int res = MDB_SUCCESS;
      if (m_read_only)
      {
        res = mdb_txn_abort(m_ptx);
      }
      else
      {
        res = mdb_txn_commit(m_ptx);
      }

      if (res != MDB_SUCCESS)
      {
        CHECK_AND_ASSERT_THROW_MES(false, "[DB ERROR]: Unable to commit transaction, error : " << mdb_strerror(res) << ", m_read_only= " << m_read_only);
        //LOG_ERROR("[DB ERROR]: Unable to commit transaction, error: " << mdb_strerror(res));
      }
      m_ptx = nullptr;
      return res;

    }
    

    
    void lmdb_db_backend::lmdb_txn::abort()
    {
      check_thread_id();

      --m_ref_count;

      if (m_ref_count != 0)
      {
        return;
      }

      //now we actually abort transaction

      if (!m_ptx)
      {
        CHECK_AND_ASSERT_THROW_MES(false, "[DB ERROR]: m_ptx oiss NULL on abort and m_marked_finished=");
      }

      mdb_txn_abort(m_ptx);
      m_ptx = nullptr; // reset pointer to avoid double abort
    }

    bool lmdb_db_backend::lmdb_txn::validate_empty()
    {
      if (m_ptx || m_ref_count)
      {
        LOG_ERROR("[LMDB ERROR]: m_ptx" << (m_ptx == nullptr ? "null" : "not_null") << ", m_ref_count(" << m_ref_count << ") on destructor");
        //mdb_txn_abort(m_ptx);
        //m_ptx = nullptr;
        return false;
      }
      return true;
    }

    lmdb_db_backend::lmdb_txn::~lmdb_txn()
    {
      validate_empty();
    }
  }
}

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL NULL
