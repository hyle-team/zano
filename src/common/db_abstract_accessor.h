// Copyright (c) 2014-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <unordered_map>
#include <set>
#include <atomic>
#include "include_base_utils.h"

#include "db_backend_base.h"
#include "misc_language.h"
#include "cache_helper.h"
#include "profile_tools.h"
#include "../serialization/serialization.h"
#include "readwrite_lock.h"
#include "math_helper.h"

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL "db"
// 'db' channel is disabled by default

namespace tools
{
  namespace db
  {
    template<class t_pod_key>
    const char* key_to_ptr(const t_pod_key& k, size_t& s) 
    {
      static_assert(std::is_pod<t_pod_key>::value, "t_pod_key must be a POD type.");
      s = sizeof(k);
      return reinterpret_cast<const char*>(&k);
    }
    inline
    const char* key_to_ptr(const std::string& k, size_t& s) 
    {
      s = k.size();
      return k.data();
    }
    template<class t_pod_key>
    void key_from_ptr(t_pod_key& k, const void* pkey, uint64_t ks)
    {
      static_assert(std::is_pod<t_pod_key>::value, "t_pod_key must be a POD type.");
      CHECK_AND_ASSERT_THROW_MES(sizeof(t_pod_key) == ks, "size missmatch");
      CHECK_AND_ASSERT_THROW_MES(pkey, "wrog ptr(null)");
      k = *(t_pod_key*)pkey;
    }
    inline
    void key_from_ptr(std::string& k, const void* pkey, uint64_t ks)
    {
      CHECK_AND_ASSERT_THROW_MES(pkey, "wrog ptr(null)");
      k.assign((const char*)pkey, static_cast<size_t>(ks));
    }

    struct i_db_parent_to_container_callabck
    {
      virtual bool on_write_transaction_begin(){ return true; }
      virtual bool on_write_transaction_commit(){ return true; }
      virtual bool on_write_transaction_abort(){ return true; }
    };


    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    class basic_db_accessor
    {
      std::shared_ptr<i_db_backend> m_backend;
      epee::critical_section m_binded_containers_lock;
      std::set<i_db_parent_to_container_callabck*> m_binded_containers;

      epee::critical_section m_transactions_stack_lock;
      std::map<std::thread::id, std::vector<bool> > m_transactions_stack;
      std::atomic<bool> m_is_open;
      epee::shared_recursive_mutex& m_rwlock;
    public:      
      struct performance_data
      {
        epee::math_helper::average<uint64_t, 10> backend_set_pod_time;
        epee::math_helper::average<uint64_t, 10> backend_set_t_time;
        epee::math_helper::average<uint64_t, 10> set_serialize_t_time;
        epee::math_helper::average<uint64_t, 10> backend_get_pod_time;
        epee::math_helper::average<uint64_t, 10> backend_get_t_time;
        epee::math_helper::average<uint64_t, 10> get_serialize_t_time;
      };
    private:
      mutable performance_data m_gperformance_data;
      mutable std::unordered_map<container_handle, performance_data> m_performance_data_map;
    public:
      basic_db_accessor(std::shared_ptr<i_db_backend> backend, epee::shared_recursive_mutex& rwlock)
        : m_backend(backend), m_rwlock(rwlock), m_is_open(false)
      {
      }

      ~basic_db_accessor()
      {
        close();
      }
      void reset_backend(std::shared_ptr<i_db_backend> backend) { m_backend = backend; }
      performance_data& get_performance_data_for_handle(container_handle h) const  { return m_performance_data_map[h]; }
      performance_data& get_performance_data_global() const { return m_gperformance_data; }


      bool bind_parent_container(i_db_parent_to_container_callabck* pcontainer)
      {
        CRITICAL_REGION_LOCAL(m_binded_containers_lock);
        m_binded_containers.insert(pcontainer);        
        return true;
      }

      bool unbind_parent_container(i_db_parent_to_container_callabck* pcontainer)
      {
        CRITICAL_REGION_LOCAL(m_binded_containers_lock);
        auto it = m_binded_containers.find(pcontainer);
        CHECK_AND_ASSERT_THROW_MES(it != m_binded_containers.end(), "Unable to unbind pointer");
        m_binded_containers.erase(it);
        return true;
      }
      bool has_writer_tx_in_stack(const std::vector<bool>& ve)
      {
        for (auto v : ve)
        {
          if (!v)
            return true;
        }
        return false;
      }
      bool begin_transaction(bool readonly = false)
      {
        bool r = m_backend->begin_transaction(readonly);
        CRITICAL_REGION_LOCAL(m_transactions_stack_lock);
        std::vector<bool>& this_thread_tx_stack = m_transactions_stack[std::this_thread::get_id()];

        if (r)
        {
          bool need_to_brodcst_writer_tx = !has_writer_tx_in_stack(this_thread_tx_stack);
          this_thread_tx_stack.push_back(readonly);
          if (need_to_brodcst_writer_tx && !readonly)
          {
            LOG_PRINT_CYAN("[WRITE_TX_BEGIN]", LOG_LEVEL_2);
            for (auto cnt_ptr : m_binded_containers)
            {
              cnt_ptr->on_write_transaction_begin();
            }
          }
        }
        return r;
      }

      bool begin_readonly_transaction()const
      {
        return const_cast<basic_db_accessor&>(*this).begin_transaction(true);
      }

      void commit_transaction()const
      {
        return const_cast<basic_db_accessor&>(*this).commit_transaction();
      }
      void commit_transaction()
      {
        bool r = false;
        bool is_writer_tx = false;
        {
          CRITICAL_REGION_LOCAL(m_transactions_stack_lock);
          std::vector<bool>& this_thread_tx_stack = m_transactions_stack[std::this_thread::get_id()];
          CHECK_AND_ASSERT_THROW_MES(this_thread_tx_stack.size(), "Internal error: this_thread_tx_stack.size = 0 at commit tx");
          is_writer_tx = !this_thread_tx_stack.back();
        }

        if (is_writer_tx)
          m_rwlock.lock(); // wait all readers and writers to get exclusive access

        {
          CRITICAL_REGION_LOCAL(m_transactions_stack_lock);
          r = m_backend->commit_transaction(); //commit first and then switch cache isolation off (via on_write_transaction_commit() below)

          std::vector<bool>& this_thread_tx_stack = m_transactions_stack[std::this_thread::get_id()];
          CHECK_AND_ASSERT_THROW_MES(this_thread_tx_stack.size(), "Internal error: this_thread_tx_stack.size = 0 at commit tx");
          CHECK_AND_ASSERT_THROW_MES(!this_thread_tx_stack.back() == is_writer_tx, "Internal error: !this_thread_tx_stack.back() != is_writer_tx at commit tx");
          this_thread_tx_stack.pop_back();
          bool has_other_writers_on_stack = has_writer_tx_in_stack(this_thread_tx_stack);
          if (is_writer_tx && !has_other_writers_on_stack)
          {
            LOG_PRINT_CYAN("[WRITE_TX_COMMIT]", LOG_LEVEL_2);
            for (auto cnt_ptr : m_binded_containers)
            {
              cnt_ptr->on_write_transaction_commit(); // will cause cache isolation to be switched off
            }
          }
        }

        if (is_writer_tx)
          m_rwlock.unlock();//manual unlock

        CHECK_AND_ASSERT_THROW_MES(r, "failed to commit tx");
      }
      void abort_transaction()
      {
        bool is_writer_tx = false;
        {
          CRITICAL_REGION_LOCAL(m_transactions_stack_lock);
          std::vector<bool>& this_thread_tx_stack = m_transactions_stack[std::this_thread::get_id()];
          CHECK_AND_ASSERT_THROW_MES(this_thread_tx_stack.size(), "Internal error: this_thread_tx_stack.size = 0 at abort tx");
          CHECK_AND_ASSERT_THROW_MES(!this_thread_tx_stack.back(), "Internal error: abort on readonly tx");
          is_writer_tx = !this_thread_tx_stack.back();
        }
        if (is_writer_tx)
          m_rwlock.lock(); // wait all readers and writers to get exclusive access

        {
          CRITICAL_REGION_LOCAL(m_transactions_stack_lock);
          m_backend->abort_transaction(); // abort first and then switch cache isolation off (via on_write_transaction_abort() below)

          std::vector<bool>& this_thread_tx_stack = m_transactions_stack[std::this_thread::get_id()];
          CHECK_AND_ASSERT_THROW_MES(this_thread_tx_stack.size(), "Internal error: this_thread_tx_stack.size = 0 at abort tx");
          CHECK_AND_ASSERT_THROW_MES(!this_thread_tx_stack.back(), "Internal error: abort on readonly tx");
          this_thread_tx_stack.pop_back();
          bool has_other_writers_on_stack = has_writer_tx_in_stack(this_thread_tx_stack);
          if (!has_other_writers_on_stack)
          {
            LOG_PRINT_CYAN("[WRITE_TX_ABORT]", LOG_LEVEL_2);
            for (auto cnt_ptr : m_binded_containers)
            {
              cnt_ptr->on_write_transaction_abort(); // will cause cache isolation to be switched off
            }
          }
        }

        if (is_writer_tx)
          m_rwlock.unlock();//manual unlock

      }


      std::shared_ptr<i_db_backend> get_backend() const 
      {
        return m_backend;
      }

      bool close()
      {
        m_is_open = false;
        if (!m_backend)
          return true;
        return m_backend->close();
      }

      bool open(const std::string& path, uint64_t cache_sz = CACHE_SIZE)
      {
        bool r = m_backend->open(path, cache_sz);
        if(r)
          m_is_open = true;
        
        return r;
      }

      bool is_open()
      {
        return m_is_open;
      }

      uint64_t size(container_handle h) const
      {
        return m_backend->size(h);
      }

      template<class t_pod_key>
      bool erase(container_handle h, const t_pod_key& k)
      {
        //TRY_ENTRY();
        size_t sk = 0;
        const char* pk = key_to_ptr(k, sk);

        return m_backend->erase(h, pk, sk);
        //CATCH_ENTRY_L0("get_t_object_from_db", false);
      }

      template<class t_pod_key, class t_object>
      bool get_t_object(container_handle h, const t_pod_key& k, t_object& obj) const
      {
        if (!m_is_open)
          return false;
        performance_data& m_performance_data = m_gperformance_data;
        //TRY_ENTRY();
        std::string res_buff;
        size_t sk = 0;
        const char* pk = key_to_ptr(k, sk);

        TIME_MEASURE_START_PD(backend_get_t_time);
        if (!m_backend->get(h, pk, sk, res_buff))
          return false;
        TIME_MEASURE_FINISH_PD(backend_get_t_time);


        TIME_MEASURE_START_PD(get_serialize_t_time);
        bool res = t_unserializable_object_from_blob(obj, res_buff);
        TIME_MEASURE_FINISH_PD(get_serialize_t_time);

        return res;
        //CATCH_ENTRY_L0("get_t_object_from_db", false);
      }

      bool clear(container_handle h)
      {
        return m_backend->clear(h);
      }

      template<class t_pod_key, class t_object>
      bool set_t_object(container_handle h, const t_pod_key& k, t_object& obj)
      {
        performance_data& m_performance_data = m_performance_data_map[h];
        //TRY_ENTRY();
        std::string obj_buff;
        TIME_MEASURE_START_PD(set_serialize_t_time);
        ::t_serializable_object_to_blob(obj, obj_buff);
        TIME_MEASURE_FINISH_PD(set_serialize_t_time);

        size_t sk = 0;
        const char* pk = key_to_ptr(k, sk);
        TIME_MEASURE_START_PD(backend_set_t_time);
        bool r = m_backend->set(h, pk, sk, obj_buff.data(), obj_buff.size());
        TIME_MEASURE_FINISH_PD(backend_set_t_time);
        return r;
        //CATCH_ENTRY_L0("set_t_object_to_db", false);
      }

      template<class t_pod_key, class t_pod_object>
      bool get_pod_object(container_handle h, const t_pod_key& k, t_pod_object& obj) const
      {
        static_assert(std::is_pod<t_pod_object>::value, "t_pod_object must be a POD type.");
        performance_data& m_performance_data = m_gperformance_data;

        //TRY_ENTRY();
        std::string res_buff;
        size_t sk = 0;
        const char* pk = key_to_ptr(k, sk);

        TIME_MEASURE_START_PD(backend_get_pod_time);
        if (!m_backend->get(h, pk, sk, res_buff))
          return false;
        TIME_MEASURE_FINISH_PD(backend_get_pod_time);


        CHECK_AND_ASSERT_MES(sizeof(t_pod_object) == res_buff.size(), false, "sizes missmath at get_pod_object_from_db(). returned size = "
          << res_buff.size() << "expected: " << sizeof(t_pod_object));

        obj = *(t_pod_object*)res_buff.data();
        return true;
        //CATCH_ENTRY_L0("get_t_object_from_db", false);
      }

      template<class t_pod_key, class t_pod_object>
      bool set_pod_object(container_handle h, const t_pod_key& k, const t_pod_object& obj)
      {
        performance_data& m_performance_data = m_performance_data_map[h];
        static_assert(std::is_pod<t_pod_object>::value, "t_pod_object must be a POD type.");
        size_t sk = 0;
        const char* pk = key_to_ptr(k, sk);

        TIME_MEASURE_START_PD(backend_set_pod_time);
        bool r = m_backend->set(h, pk, sk, (const char*)&obj, sizeof(obj));
        TIME_MEASURE_FINISH_PD(backend_set_pod_time);
        if (!r)
          return false;

        return true;
        //CATCH_ENTRY_L0("get_t_object_from_db", false);
      }
    };

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    class key_value_pod_access_strategy
    {
    public:
      template<class t_value>
      static bool from_buff_to_obj(const void* pv, uint64_t vs, t_value& v)
      {
        CHECK_AND_ASSERT_THROW_MES(sizeof(t_value) == vs, "sizes missmath at get_pod_object_from_db(). returned size = " 
          << vs << "expected: " << sizeof(t_value));

        v = *(t_value*)pv;

        return true;
      }

      template<class t_key, class t_value>
      static void set(container_handle h, basic_db_accessor& bdb, const t_key& k, const t_value& v)
      {
        static_assert(std::is_pod<t_value>::value, "t_value must be a POD type.");
        bdb.set_pod_object(h, k, v);
      }
      template<class t_key, class t_value>
      static std::shared_ptr<const t_value> get(container_handle h, basic_db_accessor& bdb, const t_key& k)
      {
        static_assert(std::is_pod<t_value>::value, "t_value must be a POD type.");
        std::shared_ptr<const t_value> res(nullptr);
        t_value v = AUTO_VAL_INIT(v);
        if (bdb.get_pod_object(h, k, v))
        {
          //TODO: remove one extra copy
          res.reset(new t_value(v));
        }
        return res;
      }
    };

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    class key_value_t_access_strategy
    {
    public:
      template<class t_value>
      static bool from_buff_to_obj(const void* pv, uint64_t vs, t_value& v)
      {
        std::string src_blob((const char*)pv, static_cast<size_t>(vs));
        return t_unserializable_object_from_blob(v, src_blob);//::t_serializable_object_to_blob(v, res_buff);
      }



      template<class t_key, class t_value>
      static void set(container_handle h, basic_db_accessor& bdb, const t_key& k, const t_value& v)
      {
        bdb.set_t_object(h, k, v);
      }
      template<class t_key, class t_value>
      static std::shared_ptr<const t_value> get(container_handle h, basic_db_accessor& bdb, const t_key& k)
      {
        std::shared_ptr<const t_value> res(nullptr);
        t_value v = AUTO_VAL_INIT(v);
        if (bdb.get_t_object(h, k, v))
        {
          //TODO: remove one extra copy
          res.reset(new t_value(v));
        }
        return res;
      }
    };

    /****************************************************************************************/
    /* to make easier selecting of strategy by true/false instead of long type definition   */
    /****************************************************************************************/
    template<bool is_t_strategy>
    class access_strategy_selector;

    template<>
    class access_strategy_selector<true>: public key_value_t_access_strategy
    {};

    template<>
    class access_strategy_selector<false>: public key_value_pod_access_strategy
    {};

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    template<class t_cb, class t_key>
    struct keys_accessor_cb : i_db_callback
    {
      t_cb m_cb;
      keys_accessor_cb(t_cb cb) :m_cb(cb){}
      bool on_enum_item(uint64_t i, const void* pkey, uint64_t ks, const void* pval, uint64_t vs)
      {
        t_key k = AUTO_VAL_INIT(k);
        key_from_ptr(k, pkey, ks);
        return m_cb(i, k);
      }
    };

    template<class t_cb, class t_key, class t_value, class t_value_read_strategy>
    struct items_accessor_cb : i_db_callback
    {
      t_cb m_cb;
      items_accessor_cb(t_cb cb) :m_cb(cb){}
      bool on_enum_item(uint64_t i, const void* pkey, uint64_t ks, const void* pval, uint64_t vs)
      {
        t_key k = AUTO_VAL_INIT(k);
        t_value v = AUTO_VAL_INIT(v);
        key_from_ptr(k, pkey, ks);
        t_value_read_strategy::from_buff_to_obj(pval, vs, v);
        return m_cb(i, k, v);
      }
    };

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/

    template<class t_key, class t_value, bool is_t_access_strategy>
    class basic_key_value_accessor : public i_db_parent_to_container_callabck
    {
#ifdef ENABLE_PROFILING
      mutable epee::profile_tools::local_call_account m_get_profiler;
      mutable epee::profile_tools::local_call_account m_set_profiler;
      mutable epee::profile_tools::local_call_account m_explicit_get_profiler;
      mutable epee::profile_tools::local_call_account m_explicit_set_profiler;
      mutable epee::profile_tools::local_call_account m_commit_profiler;
#endif
      mutable uint64_t size_cache;
      mutable bool size_cache_valid;
    protected:
      container_handle m_h;
      basic_db_accessor& bdb;
      epee::misc_utils::isolation_lock m_isolation;
    public:
      typedef t_value t_value_type;

      basic_key_value_accessor(basic_db_accessor& db) :
        bdb(db),
        m_h(AUTO_VAL_INIT(m_h)),
        size_cache(0), 
        size_cache_valid(false)
      {
        db.bind_parent_container(this);
      }

      ~basic_key_value_accessor()
      {
        TRY_ENTRY();
        bdb.unbind_parent_container(this);
        CATCH_ALL_DO_NOTHING();
      }

      virtual bool on_write_transaction_begin()
      {
        m_isolation.set_isolation_mode();
        return true;
      }
      virtual bool on_write_transaction_commit()
      {
        m_isolation.reset_isolation_mode();
        return true;
      }
      virtual bool on_write_transaction_abort()
      {
        m_isolation.reset_isolation_mode();
        return true;
      }

      bool begin_transaction(bool read_only = false)
      {
        return bdb.begin_transaction(read_only);
      }
      void commit_transaction()
      {
        try
        {
          PROFILE_FUNC_ACC(m_commit_profiler);
          bdb.commit_transaction();
        }
        catch (...)
        {
          size_cache_valid = false;
          throw;
        }
      }
      void abort_transaction()
      {
        size_cache_valid = false;
        bdb.abort_transaction();
      }

      bool init(const std::string& container_name)
      {
#ifdef ENABLE_PROFILING
        m_get_profiler.m_name = container_name +":get";
        m_set_profiler.m_name = container_name + ":set";
        m_explicit_get_profiler.m_name = container_name + ":explicit_get";
        m_explicit_set_profiler.m_name = container_name + ":explicit_set";
        m_commit_profiler.m_name        = container_name + ":commit";
#endif
        return bdb.get_backend()->open_container(container_name, m_h);
      }

      bool deinit()
      {
#ifdef ENABLE_PROFILING
        m_get_profiler.m_name           = "";
        m_set_profiler.m_name           = "";
        m_explicit_get_profiler.m_name  = "";
        m_explicit_set_profiler.m_name  = "";
        m_commit_profiler.m_name        = "";
#endif
        bool r = true;
        if (m_h)
          r = bdb.get_backend()->close_container(m_h);
        m_h = AUTO_VAL_INIT(m_h);
        size_cache = 0;
        size_cache_valid = false;
        return r;
      }

      template<class t_cb>
      void enumerate_keys(t_cb cb) const 
      {
        keys_accessor_cb<t_cb, t_key> local_enum_handler(cb);
        bdb.get_backend()->enumerate(m_h, &local_enum_handler);
      }

      // callback format: bool cb(uint64_t index, const key_t& key, const value_t& value)
      // cb should return true to continue, false -- to stop enumeration 
      template<class t_cb>
      void enumerate_items(t_cb cb)const 
      {
        items_accessor_cb<t_cb, t_key, t_value, access_strategy_selector<is_t_access_strategy> > local_enum_handler(cb);
        bdb.get_backend()->enumerate(m_h, &local_enum_handler);
      }

      template<class t_explicit_key, class t_explicit_value, class t_strategy>
      void explicit_set(const t_explicit_key& k, const t_explicit_value& v)
      {
        PROFILE_FUNC_ACC(m_explicit_set_profiler);
        size_cache_valid = false;
        t_strategy::set(m_h, bdb, k, v);
      }

      template<class t_explicit_key, class t_explicit_value, class t_strategy>
      std::shared_ptr<const t_explicit_value>  explicit_get(const t_explicit_key& k)
      {
        PROFILE_FUNC_ACC(m_explicit_get_profiler);
        return t_strategy::template get<t_explicit_key, t_explicit_value>(m_h, bdb, k);
      }

      void set(const t_key& k, const t_value& v)
      {
        PROFILE_FUNC_ACC(m_set_profiler);
        size_cache_valid = false;
        access_strategy_selector<is_t_access_strategy>::set(m_h, bdb, k, v);
      }

      std::shared_ptr<const t_value> get(const t_key& k) const
      {
        PROFILE_FUNC_ACC(m_get_profiler);
        return access_strategy_selector<is_t_access_strategy>::template get<t_key, t_value>(m_h, bdb, k);
      }

      //find() and end() aliases for make easier porting std stuff
      std::shared_ptr<const t_value> find(const t_key& k) const
      {
        return get(k);
      }

      std::shared_ptr<const t_value> end() const
      {
        return std::shared_ptr<const t_value>(nullptr);
      }

      uint64_t size() const
      {
        return m_isolation.isolated_access<uint64_t>([&](bool allowed_cache)
        {
          if (allowed_cache && size_cache_valid)
          {
            return size_cache;
          }
          else
          {
            uint64_t res = bdb.size(m_h);
            if (allowed_cache)
            {
              size_cache = res;
              size_cache_valid = true;
            }
            return res;
          }
        });
      }
      uint64_t size_no_cache() const
      {
        return bdb.size(m_h);
      }
      
      bool clear()
      {
        bool result = bdb.clear(m_h);
        m_isolation.isolated_write_access<bool>([&](){
          size_cache_valid = false;
          return true;
        });
        return result;
      }

      bool erase_validate(const t_key& k)
      {
        bool result = bdb.erase(m_h, k);
        m_isolation.isolated_write_access<bool>([&](){
          size_cache_valid = false;
          return true;
        });
        return result;
      }

      void erase(const t_key& k)
      {
        bdb.erase(m_h, k);
        m_isolation.isolated_write_access<bool>([&](){
          size_cache_valid = false;
          return true;
        });
      }

      std::shared_ptr<const t_value> operator[] (const t_key& k) const
      {
        auto r = this->get(k);
        CHECK_AND_ASSERT_THROW(r.get(), std::out_of_range("Out of range in basic_key_value_accessor"));
        return r;
      }
    };




    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    template<class t_key, class t_value, bool is_t_access_strategy, bool is_ordered_type>
    class cached_key_value_accessor : public basic_key_value_accessor<t_key, t_value, is_t_access_strategy>
    {
      typedef basic_key_value_accessor<t_key, t_value, is_t_access_strategy> base_class;

      
      typedef epee::misc_utils::cache_with_write_isolation<is_ordered_type, t_key, std::shared_ptr<const t_value>, 10000> cache_container_type;
      //typedef epee::misc_utils::cache_dummy<is_ordered_type, t_key, std::shared_ptr<const t_value>, 100000> cache_container_type;
      mutable cache_container_type m_cache;


      virtual bool on_write_transaction_abort()
      {
        clear_cache();
        return base_class::on_write_transaction_abort();
      }

    public:
      struct performance_data
      {
        epee::math_helper::average<uint64_t, 1000> hit_percent;
        epee::math_helper::average<uint64_t, 10> read_cache_microsec;
        epee::math_helper::average<uint64_t, 10> read_db_microsec;
        epee::math_helper::average<uint64_t, 10> update_cache_microsec;
        epee::math_helper::average<uint64_t, 10> write_to_cache_microsec;
        epee::math_helper::average<uint64_t, 10> write_to_db_microsec;
      };

      cached_key_value_accessor(basic_db_accessor& db) :
        basic_key_value_accessor<t_key, t_value, is_t_access_strategy>(db), 
        m_cache(base_class::m_isolation)
      {

      }
      ~cached_key_value_accessor()
      {
        NESTED_TRY_ENTRY();

        m_cache.clear();  //will clear cache isolated

        NESTED_CATCH_ENTRY(__func__);
      }

      void clear_cache() const 
      {
        m_cache.clear();
      }

      void set_cache_size(uint64_t max_cache_size)
      {
        m_cache.set_max_elements(max_cache_size);
      }

      void set(const t_key& k, const t_value& v)
      {
        TIME_MEASURE_START_PD(write_to_db_microsec);
        base_class::set(k, v);
        TIME_MEASURE_FINISH_PD(write_to_db_microsec);

        TIME_MEASURE_START_PD(write_to_cache_microsec);
        m_cache.set(k, std::shared_ptr<const t_value>(new t_value(v)));
        TIME_MEASURE_FINISH_PD(write_to_cache_microsec);
      }

      std::shared_ptr<const t_value> get(const t_key& k) const
      {
        std::shared_ptr<const t_value> res;
        TIME_MEASURE_START_PD(read_cache_microsec);
        bool r = m_cache.get(k, res);
        TIME_MEASURE_FINISH_PD(read_cache_microsec);
        if (r)
        {
          m_performance_data.hit_percent.push(100);
          return res;
        }
        m_performance_data.hit_percent.push(0);

        TIME_MEASURE_START_PD(read_db_microsec);
        res = base_class::get(k);
        TIME_MEASURE_FINISH_PD(read_db_microsec);
        if (res)
        {
          TIME_MEASURE_START_PD(update_cache_microsec);
          m_cache.set(k, res);
          TIME_MEASURE_FINISH_PD(update_cache_microsec);
        }          
        return res;
      }

      size_t clear()
      {
        m_cache.clear();
        return base_class::clear();
      }

      bool erase_validate(const t_key& k)
      {

        auto res_ptr = base_class::erase_validate(k);
        m_cache.erase(k);
        return static_cast<bool>(res_ptr);
      }

      std::shared_ptr<const t_value> operator[] (const t_key& k) const
      {  
        return get(k);
      }

      void erase(const t_key& k)
      {
        base_class::erase(k);
        m_cache.erase(k);
      }
       
      performance_data& get_performance_data() const 
      {
        return m_performance_data;
      }
      typename basic_db_accessor::performance_data& get_performance_data_native() const
      {
        return base_class::bdb.get_performance_data_for_handle(base_class::m_h);
      }
      typename basic_db_accessor::performance_data& get_performance_data_global() const
      {
        return base_class::bdb.get_performance_data_global();
      }

    private:
      mutable performance_data m_performance_data;
    };


    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    template<class t_key, class t_value, class t_accessor, bool is_t_strategy = false>
    class solo_db_value
    {
      t_key m_key;
      t_accessor& m_accessor;
    public:
      solo_db_value(const t_key& k, t_accessor& accessor) :m_accessor(accessor), m_key(k)
      {}

      solo_db_value(const solo_db_value& v) :m_accessor(v.m_accessor), m_key(v.m_key)
      {}
      solo_db_value& operator=(const solo_db_value&) = delete;


      t_value operator=(t_value v) 
      {	
        m_accessor.template explicit_set<t_key, t_value, access_strategy_selector<is_t_strategy> >(m_key, v);
        return v;
      }

      operator t_value() const 
      {
        std::shared_ptr<const t_value> value_ptr = m_accessor.template explicit_get<t_key, t_value, access_strategy_selector<is_t_strategy> >(m_key);
        if (value_ptr.get())
          return *value_ptr.get();

        return AUTO_VAL_INIT(t_value());
      }
    };


#pragma pack(push, 1)
    struct guid_key
    {
      uint64_t a;
      uint64_t b;
    };
#pragma pack(pop)
    
    //we use this suffix to keep elements count of sub-container, guid_key collision supposed to be impossible (almost)
    // {E005D3C8-C9E9-4C2E-B94C-A40EC25505AD}
    static const guid_key const_counter_suffix =
    { 0xe005d3c8c9e94c2e, 0xb94ca40ec25550ad};
    
    
#pragma pack(push, 1)
    template<class t_key_a, class t_key_b>
    struct composite_key
    {
      t_key_a container_id;
      t_key_b sufix;
    };

//     template<class t_key>
//     struct container_counter_key_holder : public composite_key<t_key, guid_key>
//     {};
#pragma pack(pop)

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/

    template<class t_key, class t_value, bool is_t_access_strategy>
    class basic_key_to_array_accessor : public basic_key_value_accessor<composite_key<t_key, uint64_t>, t_value, is_t_access_strategy>
    {
      typedef basic_key_value_accessor<composite_key<t_key, uint64_t>, t_value, is_t_access_strategy> basic_accessor_type;

      solo_db_value<composite_key<t_key, guid_key>, uint64_t, basic_accessor_type>
      get_counter_accessor(const t_key& container_id)
      {
        static_assert(std::is_pod<t_key>::value, "t_pod_key must be a POD type.");
        composite_key<t_key, guid_key> cc = { container_id, const_counter_suffix}; 

        return solo_db_value<composite_key<t_key, guid_key>, uint64_t, basic_accessor_type >(cc, *this);
      }

      const solo_db_value<composite_key<t_key, guid_key>, uint64_t, basic_accessor_type >
      get_counter_accessor(const t_key& container_id) const
      {
        static_assert(std::is_pod<t_key>::value, "t_pod_key must be a POD type.");
        composite_key<t_key, guid_key> cc = { container_id, const_counter_suffix };

        return solo_db_value<composite_key<t_key, guid_key>, uint64_t, basic_accessor_type >(cc, const_cast<basic_accessor_type&>(static_cast<const basic_accessor_type&>(*this)));
      }

      template<typename callback_t>
      struct subitems_visitor : public i_db_callback
      {
        subitems_visitor(callback_t cb)
          : m_callback(cb)
        {}

        virtual bool on_enum_item(uint64_t i, const void* key_data, uint64_t key_size, const void* value_data, uint64_t value_size) override
        {
          if (key_size != sizeof(composite_key<t_key, uint64_t>))
            return true; // skip solo values containing items size

          composite_key<t_key, uint64_t> key = AUTO_VAL_INIT(key);
          key_from_ptr(key, key_data, key_size);

          t_value value = AUTO_VAL_INIT(value);
          access_strategy_selector<is_t_access_strategy>::from_buff_to_obj(value_data, value_size, value);

          return m_callback(i, key.container_id, key.sufix, value);
        }

        callback_t m_callback;
      };

    public:
      basic_key_to_array_accessor(basic_db_accessor& db) : basic_key_value_accessor<composite_key<t_key, uint64_t>, t_value, is_t_access_strategy>(db)
      {}

      uint64_t get_item_size(const t_key& k) const
      {
        return get_counter_accessor(k);
      }

      std::shared_ptr<const t_value> get_subitem(const t_key& k, uint64_t i) const
      {
        if (i >= get_item_size(k))
        {
          LOG_ERROR("Out of bound access: itmes_count = " << get_item_size(k) << ", i = " << i);
          throw std::out_of_range("In basic_key_to_array_accessor");
        }
        composite_key<t_key, uint64_t> ck{ k, i };
        return this->get(ck);
      }

      void push_back_item(const t_key& k, const t_value& v)
      {
        auto counter = get_counter_accessor(k);
        uint64_t new_index = counter;
        counter = new_index+1;
        composite_key<t_key, uint64_t> ck{ k, new_index };
        this->set(ck, v);
      }

      void pop_back_item(const t_key& k)
      { 
        auto counter = get_counter_accessor(k);
        uint64_t sz = static_cast<uint64_t>(counter);

        CHECK_AND_ASSERT_MES(sz != 0, void(), "pop_back_item called on empty subitem");
        
        composite_key<t_key, uint64_t> ck{ k, sz - 1 };
        this->erase(ck);
        counter = --sz;
      }

      // removes all items by key k
      void erase_items(const t_key& k)
      { 
        auto counter = get_counter_accessor(k);
        uint64_t sz = static_cast<uint64_t>(counter);

        while (sz != 0)
        {
          composite_key<t_key, uint64_t> ck{ k, sz - 1 };
          this->erase(ck);
          --sz;
        }

        counter = 0;
      }

      template<typename callback_t>
      void enumerate_subitems(callback_t callback) const
      {
        subitems_visitor<callback_t> visitor(callback);
        basic_accessor_type::bdb.get_backend()->enumerate(basic_accessor_type::m_h, &visitor);
      }

    };

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    template<class t_value, bool is_t_access_strategy>
    class array_accessor : public cached_key_value_accessor<uint64_t, t_value, is_t_access_strategy, true>
    {
    public: 
      array_accessor(basic_db_accessor& db) : cached_key_value_accessor<uint64_t, t_value, is_t_access_strategy, true>(db)
      {}
      void push_back(const t_value& v)
      {
        this->set(this->size(), v);

      }
      void pop_back()
      {
        this->erase(this->size()-1);
      }
      std::shared_ptr<const t_value> back() const
      {
        auto res = this->get(this->size()-1);
        CHECK_AND_ASSERT_THROW(res.get(), std::out_of_range(std::string("Out of range in array_accessor in call back(): size()=") 
          + std::to_string(this->size())
          + ", size_no_cache()=" + std::to_string(this->size_no_cache()) ));
        return res;
      }

      std::shared_ptr<const t_value> operator[] (const uint64_t& k) const
      {
        auto res = this->get(k);
        CHECK_AND_ASSERT_THROW(res.get(), std::out_of_range(std::string("Out of range in array_accessor in call []: key = ")
          + std::to_string(k)
          + ", size_no_cache()=" + std::to_string(this->size_no_cache())));
        
        return res;
      }

    };

    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    template<class t_db>
    class reader_access
    {
      t_db& m_db;
      bool got_transaction;
    public: 
      reader_access(t_db& db) :m_db(db), got_transaction(false)
      {}

      void lock()
      {
        //start read only transaction
        if (m_db.is_open())
        {
          m_db.begin_transaction(true);
          got_transaction = true;
        }
        
      }
      void unlock()
      {
        if (got_transaction)
          m_db.commit_transaction();

        got_transaction = false;
      }

    };

  }
}

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL NULL
