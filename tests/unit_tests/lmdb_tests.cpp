// Copyright (c) 2019 Zano Project
<<<<<<< HEAD
// Copyright (c) 2018 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <memory>
#include <thread>
#include <atomic>

#include "epee/include/include_base_utils.h"

#define USE_INSECURE_RANDOM_RPNG_ROUTINES // turns on random manupulation for tests
#include "crypto/crypto.h"
#include "gtest/gtest.h"
#include "common/db_abstract_accessor.h"
#include "common/db_backend_lmdb.h"
#include "serialization/serialization.h"
=======
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <memory>
#include "crypto/crypto.h"
#include "gtest/gtest.h"
#include "common/db_backend_lmdb.h"
#include "common/db_abstract_accessor.h"
>>>>>>> commit

using namespace tools;

namespace lmdb_test
{

<<<<<<< HEAD
  crypto::hash null_hash = AUTO_VAL_INIT(null_hash);

  template<typename T>
  T random_t_from_range(T from, T to)
  {
    if (from >= to)
      return from;
    T result;
    crypto::generate_random_bytes(sizeof result, &result);
    return from + result % (to - from + 1);
  }

  //////////////////////////////////////////////////////////////////////////////
  // basic_test
  //////////////////////////////////////////////////////////////////////////////
  TEST(lmdb, basic_test)
  {
    std::shared_ptr<db::lmdb_db_backend> lmdb_ptr = std::make_shared<db::lmdb_db_backend>();
    epee::shared_recursive_mutex db_lock;
    db::basic_db_accessor dbb(lmdb_ptr, db_lock);

    bool r = false;
    
    r = dbb.open("test_lmdb");
    ASSERT_TRUE(r);

    db::container_handle tid_decapod;
    r = lmdb_ptr->open_container("decapod", tid_decapod);
    ASSERT_TRUE(r);

    ASSERT_TRUE(lmdb_ptr->begin_transaction());

    ASSERT_TRUE(lmdb_ptr->clear(tid_decapod));

    uint64_t key = 10;
    std::string buf = "nxjdu47flrp20soam19e7nfhxbcy48owks03of92sbf31n1oqkanmdb47";

    r = lmdb_ptr->set(tid_decapod, (char*)&key, sizeof key, buf.c_str(), buf.size());
    ASSERT_TRUE(r);

    ASSERT_TRUE(lmdb_ptr->commit_transaction());

    r = dbb.close();
    ASSERT_TRUE(r);


    r = dbb.open("test_lmdb");
    ASSERT_TRUE(r);
    r = lmdb_ptr->open_container("decapod", tid_decapod);
    ASSERT_TRUE(r);

    ASSERT_TRUE(lmdb_ptr->begin_transaction());

    std::string out_buffer;
    r = lmdb_ptr->get(tid_decapod, (char*)&key, sizeof key, out_buffer);
    ASSERT_TRUE(r);
    ASSERT_EQ(buf, out_buffer);

    ASSERT_TRUE(lmdb_ptr->commit_transaction());

    r = dbb.close();
    ASSERT_TRUE(r);

  }

  //////////////////////////////////////////////////////////////////////////////
  // multithread_test_1
  //////////////////////////////////////////////////////////////////////////////
  struct multithread_test_1_t : public db::i_db_callback
  {
    enum { c_keys_count = 128 };
    crypto::hash m_keys[c_keys_count];
    size_t m_randomly_mixed_indexes_1[c_keys_count];
    size_t m_randomly_mixed_indexes_2[c_keys_count];
    epee::shared_recursive_mutex m_db_lock;
    std::shared_ptr<db::lmdb_db_backend> m_lmdb_adapter;
    db::basic_db_accessor m_dbb;
    db::container_handle m_table_id;
    size_t m_counter;

    multithread_test_1_t()
      : m_lmdb_adapter(std::make_shared<db::lmdb_db_backend>())
      , m_dbb(m_lmdb_adapter, m_db_lock)
      , m_table_id(0)
      , m_counter(0)
    {
      for (size_t i = 0; i < c_keys_count; ++i)
      {
        m_keys[i] = i == 0 ? null_hash : crypto::cn_fast_hash(&m_keys[i - 1], sizeof(crypto::hash));
        m_randomly_mixed_indexes_1[i] = i;
        m_randomly_mixed_indexes_2[i] = i;
      }

      // prepare m_randomly_mixed_indexes_1 and m_randomly_mixed_indexes_2
      for (size_t i = 0; i < c_keys_count * 100; ++i)
      {
        size_t a = random_t_from_range<size_t>(0, c_keys_count - 1);
        size_t b = random_t_from_range<size_t>(0, c_keys_count - 1);
        size_t c = random_t_from_range<size_t>(0, c_keys_count - 1);
        size_t d = random_t_from_range<size_t>(0, c_keys_count - 1);

        size_t tmp = m_randomly_mixed_indexes_1[a];
        m_randomly_mixed_indexes_1[a] = m_randomly_mixed_indexes_1[b];
        m_randomly_mixed_indexes_1[b] = tmp;

        tmp = m_randomly_mixed_indexes_2[c];
        m_randomly_mixed_indexes_2[c] = m_randomly_mixed_indexes_2[d];
        m_randomly_mixed_indexes_2[d] = tmp;
      }
    }

    void adder_thread(std::atomic<bool>& stop_flag)
    {
      epee::log_space::log_singletone::set_thread_log_prefix("[ adder ] ");
      //epee::misc_utils::sleep_no_w(1000);

      size_t i = 0;
      for(size_t n = 0; n < 1000; ++n)
      {
        // get pseudorandom key index
        size_t key_index = m_randomly_mixed_indexes_1[i];
        i = (i + 1) % c_keys_count;
        const crypto::hash& key = m_keys[key_index];

        bool r = m_lmdb_adapter->begin_transaction();
        CHECK_AND_ASSERT_MES_NO_RET(r, "begin_transaction");

        // get a value by the given key
        std::string value;
        if (!m_lmdb_adapter->get(m_table_id, (const char*)&key, sizeof key, value))
        {
          // if such key does not exist -- add it
          char buffer[128];
          crypto::generate_random_bytes(sizeof buffer, buffer);
          r = m_lmdb_adapter->set(m_table_id, (const char*)&key, sizeof key, buffer, sizeof buffer);
          CHECK_AND_ASSERT_MES_NO_RET(r, "set");

          size_t table_size = m_lmdb_adapter->size(m_table_id);

          r = m_lmdb_adapter->commit_transaction();
          CHECK_AND_ASSERT_MES_NO_RET(r, "commit_transaction");

          LOG_PRINT_L1("added key index " << key_index << ", table size: " << table_size);
        }
        else
        {
          // if key exists in the table - do nothing
          m_lmdb_adapter->abort_transaction();
        }
        epee::misc_utils::sleep_no_w(1);
      }
      LOG_PRINT_L0("adder_thread stopped");
    }

    void deleter_thread(std::atomic<bool>& stop_flag)
    {
      epee::log_space::log_singletone::set_thread_log_prefix("[deleter] ");
      epee::misc_utils::sleep_no_w(1000);

      // get pseudorandom key index
      size_t i = 0;

      while(!stop_flag)
      {
        size_t key_index = m_randomly_mixed_indexes_2[i];
        i = (i + 1) % c_keys_count;
        const crypto::hash& key = m_keys[key_index];
        bool r = m_lmdb_adapter->begin_transaction();
        CHECK_AND_ASSERT_MES_NO_RET(r, "begin_transaction");

        // get a value by the given key
        std::string value;
        if (m_lmdb_adapter->get(m_table_id, (const char*)&key, sizeof key, value))
        {
          // if key exists in the table -- remove it
          r = m_lmdb_adapter->erase(m_table_id, (const char*)&key, sizeof key);
          CHECK_AND_ASSERT_MES_NO_RET(r, "erase");

          size_t table_size = m_lmdb_adapter->size(m_table_id);

          r = m_lmdb_adapter->commit_transaction();
          CHECK_AND_ASSERT_MES_NO_RET(r, "commit_transaction");

          LOG_PRINT_L1("erased key index " << key_index << ", table size: " << table_size);
          if (table_size == 2)
            break;
        }
        else
        {
          // if no such key exists in the table - do nothing
          m_lmdb_adapter->abort_transaction();
        }
        epee::misc_utils::sleep_no_w(1);
      }
      LOG_PRINT_L0("deleter_thread stopped");
    }

    void reader_thread()
    {
      epee::log_space::log_singletone::set_thread_log_prefix("[reader ] ");
      epee::misc_utils::sleep_no_w(1000);

      // get pseudorandom key index
      size_t i = 17;
      uint64_t sum = 0; // just for fun

      for(;;)
      {
        size_t key_index = m_randomly_mixed_indexes_2[i];
        i = (i + 1) % c_keys_count;
        const crypto::hash& key = m_keys[key_index];
        bool r = m_lmdb_adapter->begin_transaction(true); // request Read-Only transaction
        CHECK_AND_ASSERT_MES_NO_RET(r, "begin_transaction(RO=true)");

        // get a value by the given key
        std::string value;
        if (m_lmdb_adapter->get(m_table_id, (const char*)&key, sizeof key, value))
        {
          sum += *reinterpret_cast<const uint64_t*>(value.data());

          size_t table_size = m_lmdb_adapter->size(m_table_id);

          r = m_lmdb_adapter->commit_transaction();
          CHECK_AND_ASSERT_MES_NO_RET(r, "commit_transaction");

          if (table_size == 2)
            break;
        }
        else
        {
          // if no such key exists in the table - do nothing
          m_lmdb_adapter->abort_transaction();
        }
        epee::misc_utils::sleep_no_w(1);
      }
      LOG_PRINT_L0("reader_thread stopped, sum = " << sum);
    }

    bool check()
    {
      size_t table_size = m_lmdb_adapter->size(m_table_id);
      CHECK_AND_ASSERT_MES(table_size == 2, false, "2 elements are expected to left");

      m_counter = 0;
      bool r = m_lmdb_adapter->begin_transaction();
      CHECK_AND_ASSERT_MES(r, false, "begin_transaction");

      m_lmdb_adapter->enumerate(m_table_id, this);

      r = m_lmdb_adapter->commit_transaction();
      CHECK_AND_ASSERT_MES(r, false, "commit_transaction");
      
      return m_counter == 2;
    }

    // class i_db_visitor
    virtual bool on_enum_item(size_t i, const void* key_data, size_t key_size, const void* value_data, size_t value_size) override
    {
      CHECK_AND_ASSERT_MES(key_size == sizeof(crypto::hash), false, "invalid key size: " << key_size);
      const crypto::hash *p_key = reinterpret_cast<const crypto::hash*>(key_data);

      size_t key_index = SIZE_MAX;
      for(size_t j = 0; j < c_keys_count && key_index == SIZE_MAX; ++j)
        if (m_keys[j] == *p_key)
          key_index = j;

      CHECK_AND_ASSERT_MES(key_index != SIZE_MAX, false, "visitor gets a non-existing key");

      LOG_PRINT_L1("visitor: #" << i << ", key index: " << key_index);
      if (i != m_counter)
      {
        LOG_ERROR("invalid visitor index i: " << i << ", m_counter: " << m_counter);
        m_counter = SIZE_MAX;
        return false;
      }
      ++m_counter;

      return true;
    };


    bool run()
    {
      bool r = m_dbb.open("multithread_test_1_t");
      CHECK_AND_ASSERT_MES(r, false, "m_dbb.open");
      r = m_lmdb_adapter->open_container("table1_", m_table_id);
      CHECK_AND_ASSERT_MES(r, false, "open_table");
      r = m_lmdb_adapter->begin_transaction();
      CHECK_AND_ASSERT_MES(r, false, "begin_transaction");
      r = m_lmdb_adapter->clear(m_table_id);
      CHECK_AND_ASSERT_MES(r, false, "clear_table");
      r = m_lmdb_adapter->commit_transaction();
      CHECK_AND_ASSERT_MES(r, false, "commit_transaction");

      std::atomic<bool> stop_adder(false), stop_deleter(false);
      std::thread adder_t(&multithread_test_1_t::adder_thread, this, std::ref(stop_adder));
      std::thread deleter_t(&multithread_test_1_t::deleter_thread, this, std::ref(stop_deleter));

      std::vector<std::thread> readers_t;
      const size_t reader_threads_count = 2;
      for (size_t i = 0; i < reader_threads_count; ++i)
        readers_t.emplace_back(std::thread(&multithread_test_1_t::reader_thread, this));

      for (auto& t : readers_t)
        t.join();

      adder_t.join();
      //stop_deleter = true;
      deleter_t.join();
      r = check();
      m_dbb.close();
      return r;
    }
  };

  TEST(lmdb, multithread_test_1)
  {
    char prng_state[200] = {};
    crypto::random_prng_get_state(prng_state, sizeof prng_state); // store current RPNG state
    crypto::random_prng_initialize_with_seed(0); // this mades this test deterministic

    bool result = false;
    try
    {
      multithread_test_1_t t;
      result = t.run();
    }
    catch (std::exception& e)
    {
      LOG_ERROR("Caught exception: " << e.what());
    }

    // restore PRNG state to keep other tests unaffected
    crypto::random_prng_get_state(prng_state, sizeof prng_state);

    ASSERT_TRUE(result);
  }

  //////////////////////////////////////////////////////////////////////////////
  // bridge_basic_test
  //////////////////////////////////////////////////////////////////////////////
  struct simple_serializable_t
  {
    std::string name;
    uint64_t number;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(name)
      FIELD(number)
    END_SERIALIZE()
  };

  inline bool operator==(const simple_serializable_t &lhs, const simple_serializable_t &rhs)
  {
    return lhs.name == rhs.name &&
      lhs.number == rhs.number;
  }

  struct simple_pod_t
  {
    char c;
    uint64_t u;
    float f;
  };

  inline bool operator==(const simple_pod_t &_v1, const simple_pod_t &_v2)
  {
    return std::memcmp(&_v1, &_v2, sizeof _v1) == 0;
  }

  TEST(lmdb, bridge_basic_test)
  {
    std::shared_ptr<db::lmdb_db_backend> lmdb_ptr = std::make_shared<db::lmdb_db_backend>();
    epee::shared_recursive_mutex db_lock;
    db::basic_db_accessor dbb(lmdb_ptr, db_lock);

    bool r = false;
    
    r = dbb.open("bridge_basic_test");
    ASSERT_TRUE(r);

    db::container_handle tid_decapod;
    r = lmdb_ptr->open_container("decapod", tid_decapod);
    ASSERT_TRUE(r);

    ASSERT_TRUE(dbb.begin_transaction());

    ASSERT_TRUE(dbb.clear(tid_decapod));

    const char key[] = "nxjdu47flrp20soam19e7nfhxbcy48owks03of92sbf31n1oqkanmdb47";
    simple_serializable_t s_object;
    s_object.number = 1001100102;
    s_object.name = "bender";

    r = dbb.set_t_object(tid_decapod, key, s_object);
    ASSERT_TRUE(r);

    dbb.commit_transaction();

    ASSERT_TRUE(dbb.begin_transaction());

    simple_serializable_t s_object2;
    r = dbb.get_t_object(tid_decapod, key, s_object2);
    ASSERT_TRUE(r);
    ASSERT_EQ(s_object, s_object2);

    // del object by key and make sure it does not exist anymore
    r = dbb.erase(tid_decapod, key);
    ASSERT_TRUE(r);

    r = dbb.get_t_object(tid_decapod, key, s_object2);
    ASSERT_FALSE(r);

    // second erase shoud also fail
    r = dbb.erase(tid_decapod, key);
    ASSERT_FALSE(r);

    dbb.commit_transaction();


   // POD type
    const char key_pod[] = "alqocyfu7sbxhaoo5kdnrt77tgwderhjs9a9sdjf324nfjksd9f0s90f2";
    ASSERT_TRUE(dbb.begin_transaction());
    simple_pod_t p_object1 = {' ', 0xf7f7f7f7d3d3d3d3ull, 2.002319f};
    r = dbb.set_pod_object(tid_decapod, key_pod, p_object1);
    ASSERT_TRUE(r);
    dbb.commit_transaction();

    ASSERT_TRUE(dbb.begin_transaction());
    simple_pod_t p_object2;
    r = dbb.get_pod_object(tid_decapod, key_pod, p_object2);
    ASSERT_TRUE(r);
    ASSERT_EQ(p_object1, p_object2);

    // del object by key and make sure it does not exist anymore
    r = dbb.erase(tid_decapod, key_pod);
    ASSERT_TRUE(r);

    r = dbb.get_pod_object(tid_decapod, key_pod, p_object2);
    ASSERT_FALSE(r);

    // second erase shoud also fail
    r = dbb.erase(tid_decapod, key_pod);
    ASSERT_FALSE(r);
    dbb.commit_transaction();


    r = dbb.close();
    ASSERT_TRUE(r);
  }

  //////////////////////////////////////////////////////////////////////////////
  // single_value_test
  //////////////////////////////////////////////////////////////////////////////
  struct serializable_string
  {
    serializable_string() {}
    explicit serializable_string(const std::string& v) : v(v) {}

    std::string v;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(v)
    END_SERIALIZE()
  };
  
  TEST(lmdb, single_value_test)
  {
    const std::string options_table_name("options");
    
    std::shared_ptr<db::lmdb_db_backend> lmdb_ptr = std::make_shared<db::lmdb_db_backend>();
    epee::shared_recursive_mutex db_lock;
    db::basic_db_accessor dbb(lmdb_ptr, db_lock);
    db::basic_key_value_accessor<uint64_t, uint64_t /* does not matter */, false /* does not matter */ > options_container(dbb);

    db::solo_db_value<uint64_t, uint64_t, decltype(options_container),            false>   option_uint64(0, options_container);
    db::solo_db_value<uint64_t, serializable_string, decltype(options_container), true>    option_serializable_obj(1, options_container);
    db::solo_db_value<uint64_t, crypto::hash, decltype(options_container),        false>   option_hash(2, options_container);

    ASSERT_TRUE(dbb.open("single_value_test"));

    // clear table
    db::container_handle options_tid;
    ASSERT_TRUE(lmdb_ptr->open_container(options_table_name, options_tid));
    ASSERT_TRUE(dbb.begin_transaction());
    ASSERT_TRUE(dbb.clear(options_tid));
    dbb.commit_transaction();

    ASSERT_TRUE(options_container.init(options_table_name));

    // check defaults
    ASSERT_TRUE(dbb.begin_transaction());
    uint64_t v = option_uint64;
    ASSERT_EQ(v, 0);
    
    serializable_string ss = option_serializable_obj;
    ASSERT_EQ(ss.v, std::string(""));

    crypto::hash h = option_hash;
    ASSERT_EQ(h, null_hash);

    dbb.commit_transaction();


    // set single values
    ASSERT_TRUE(dbb.begin_transaction());
    option_uint64 = 97;
    option_serializable_obj = serializable_string("New York advertising men");
    option_hash = crypto::cn_fast_hash(options_table_name.c_str(), options_table_name.size());
    dbb.commit_transaction();

    ASSERT_TRUE(dbb.close());

    
    // reopen DB
    ASSERT_TRUE(dbb.open("single_value_test"));
    ASSERT_TRUE(options_container.init(options_table_name));


    // get single value
    ASSERT_TRUE(dbb.begin_transaction());

    v = option_uint64;
    ASSERT_EQ(v, 97);

    ss = option_serializable_obj;
    ASSERT_EQ(ss.v, "New York advertising men");

    h = option_hash;
    ASSERT_EQ(h, crypto::cn_fast_hash(options_table_name.c_str(), options_table_name.size()));

    dbb.commit_transaction();


    ASSERT_TRUE(dbb.close());
  }


  //////////////////////////////////////////////////////////////////////////////
  // array_basic_test
  //////////////////////////////////////////////////////////////////////////////
  TEST(lmdb, array_basic_test)
  {
    bool r = false;

    const std::string array_table_name("test_array");
    
    std::shared_ptr<db::lmdb_db_backend> lmdb_ptr = std::make_shared<db::lmdb_db_backend>();
    epee::shared_recursive_mutex db_lock;
    db::basic_db_accessor dbb(lmdb_ptr, db_lock);

    db::basic_key_to_array_accessor<uint64_t, serializable_string, true> db_array(dbb);

    ASSERT_TRUE(dbb.open("array_basic_test"));

    // clear table
    db::container_handle tid;
    ASSERT_TRUE(lmdb_ptr->open_container(array_table_name, tid));
    ASSERT_TRUE(dbb.begin_transaction());
    ASSERT_TRUE(dbb.clear(tid));
    dbb.commit_transaction();

    ASSERT_TRUE(db_array.init(array_table_name));

    // check defaults
    ASSERT_TRUE(dbb.begin_transaction());

    size_t count = db_array.get_item_size(97);
    ASSERT_EQ(count, 0);

    std::shared_ptr<const serializable_string> ptr;
    r = false;
    try
    {
      ptr = db_array.get_subitem(97, 0);
    }
    catch (...)
    {
      r = true;
    }
    ASSERT_TRUE(r);

    dbb.commit_transaction();



    // write
    ASSERT_TRUE(dbb.begin_transaction());
    db_array.push_back_item(97, serializable_string("507507507507507507507507"));
    db_array.push_back_item(97, serializable_string("787878787878787878787878"));
    db_array.push_back_item(97, serializable_string("ringing phone"));

    count = db_array.get_item_size(97);
    ASSERT_EQ(count, 3);

    dbb.commit_transaction();

    ASSERT_TRUE(dbb.begin_transaction());
    db_array.push_back_item(97, serializable_string("ring"));
    db_array.push_back_item(97, serializable_string("ring"));
    db_array.push_back_item(97, serializable_string("ring"));

    count = db_array.get_item_size(97);
    ASSERT_EQ(count, 6);
    dbb.abort_transaction();

    ASSERT_TRUE(dbb.begin_transaction());
    count = db_array.get_item_size(97);
    ASSERT_EQ(count, 3);
    dbb.commit_transaction();

    ASSERT_TRUE(dbb.close());

    
    // reopen DB
    ASSERT_TRUE(dbb.open("array_basic_test"));
    ASSERT_TRUE(db_array.init(array_table_name));

    ASSERT_TRUE(dbb.begin_transaction());
    count = db_array.get_item_size(97);
    ASSERT_EQ(count, 3);

    ptr = db_array.get_subitem(97, 0);
    ASSERT_TRUE((bool)ptr);
    ASSERT_EQ(ptr->v, "507507507507507507507507");

    ptr = db_array.get_subitem(97, 1);
    ASSERT_TRUE((bool)ptr);
    ASSERT_EQ(ptr->v, "787878787878787878787878");

    ptr = db_array.get_subitem(97, 2);
    ASSERT_TRUE((bool)ptr);
    ASSERT_EQ(ptr->v, "ringing phone");

    ASSERT_EQ(db_array.get_item_size(555), 0);
    db_array.pop_back_item(555);
    ASSERT_EQ(db_array.get_item_size(555), 0);

    db_array.pop_back_item(97);
    db_array.pop_back_item(97);

    dbb.commit_transaction();



    ASSERT_TRUE(dbb.begin_transaction());
    count = db_array.get_item_size(97);
    ASSERT_EQ(count, 1);

    ptr = db_array.get_subitem(97, 0);
    ASSERT_TRUE((bool)ptr);
    ASSERT_EQ(ptr->v, "507507507507507507507507");
    dbb.commit_transaction();


    ASSERT_TRUE(dbb.close());
  }

  //////////////////////////////////////////////////////////////////////////////
  // array_accessor_test
  //////////////////////////////////////////////////////////////////////////////
  TEST(lmdb, array_accessor_test)
  {
    bool r = false;

    const std::string array_table_name("array");
    
    std::shared_ptr<db::lmdb_db_backend> lmdb_ptr = std::make_shared<db::lmdb_db_backend>();
    epee::shared_recursive_mutex db_lock;
    db::basic_db_accessor dbb(lmdb_ptr, db_lock);

    db::array_accessor<serializable_string, true> db_array(dbb);

    ASSERT_TRUE(dbb.open("array_accessor_test"));

    // clear table
    db::container_handle tid;
    ASSERT_TRUE(lmdb_ptr->open_container(array_table_name, tid));
    ASSERT_TRUE(dbb.begin_transaction());
    ASSERT_TRUE(dbb.clear(tid));
    dbb.commit_transaction();

    // check defaults
    ASSERT_TRUE(db_array.init(array_table_name));
    ASSERT_TRUE(db_array.begin_transaction(true));

    size_t count = db_array.size();
    ASSERT_EQ(count, 0);
    count = db_array.size_no_cache();
    ASSERT_EQ(count, 0);

    std::shared_ptr<const serializable_string> ptr;
    r = false;
    try
    {
      ptr = db_array.back();
    }
    catch(...)
    {
      r = true;
    }
    ASSERT_TRUE(r);

    ASSERT_FALSE(db_array.clear()); // clear() should fail on read-only transaction

    r = false;
    try
    {
      ptr = db_array[0];
    }
    catch(...)
    {
      r = true;
    }
    ASSERT_TRUE(r);

    db_array.commit_transaction();



    ASSERT_TRUE(db_array.begin_transaction());
    db_array.push_back(serializable_string("A"));
    db_array.push_back(serializable_string("B"));
    db_array.push_back(serializable_string("C"));
    db_array.push_back(serializable_string("D"));
    db_array.push_back(serializable_string("E"));
    db_array.commit_transaction();

    ASSERT_TRUE(db_array.begin_transaction());
    ptr = db_array[4];
    ASSERT_EQ(ptr->v, "E");
    db_array.pop_back();
    ASSERT_EQ(db_array.size(), 4);

    r = false;
    try
    {
      ptr = db_array[4];
    }
    catch(...)
    {
      r = true;
    }
    ASSERT_TRUE(r);

    ptr = db_array[3];
    ASSERT_EQ(ptr->v, "D");

    db_array.push_back(serializable_string("X"));

    ptr = db_array[4];
    ASSERT_EQ(ptr->v, "X");

    ASSERT_TRUE(db_array.clear());

    db_array.commit_transaction();


    ASSERT_TRUE(db_array.begin_transaction(true));
    count = db_array.size();
    ASSERT_EQ(count, 0);
    db_array.commit_transaction();
  }

  //////////////////////////////////////////////////////////////////////////////
  // key_value_test
  //////////////////////////////////////////////////////////////////////////////
  TEST(lmdb, key_value_test)
  {
    bool r = false;

    const std::string array_table_name("key-value");
    
    std::shared_ptr<db::lmdb_db_backend> lmdb_ptr = std::make_shared<db::lmdb_db_backend>();
    epee::shared_recursive_mutex db_lock;
    db::basic_db_accessor dbb(lmdb_ptr, db_lock);

    db::basic_key_value_accessor<uint64_t, serializable_string, true> db_key_value_map(dbb);

    ASSERT_TRUE(dbb.open("key-value_test"));

    // clear table
    db::container_handle tid;
    ASSERT_TRUE(lmdb_ptr->open_container(array_table_name, tid));
    ASSERT_TRUE(dbb.begin_transaction());
    ASSERT_TRUE(dbb.clear(tid));
    dbb.commit_transaction();

    // check defaults
    ASSERT_TRUE(db_key_value_map.init(array_table_name));
    ASSERT_TRUE(db_key_value_map.begin_transaction(true));

    size_t count = db_key_value_map.size();
    ASSERT_EQ(count, 0);
    count = db_key_value_map.size_no_cache();
    ASSERT_EQ(count, 0);

    std::shared_ptr<const serializable_string> ptr;
    r = false;
    try
    {
      ptr = db_key_value_map[99];
    }
    catch (std::out_of_range&)
    {
      r = true;
    }
    ASSERT_TRUE(r);

    ptr = db_key_value_map.get(99);
    ASSERT_TRUE(!ptr);

    ASSERT_FALSE(db_key_value_map.clear()); // clear() should fail on read-only transaction

    db_key_value_map.commit_transaction();



    ASSERT_TRUE(db_key_value_map.begin_transaction());
    db_key_value_map.set(99, serializable_string("A"));
    db_key_value_map.set(100, serializable_string("B"));
    db_key_value_map.set(101, serializable_string("C"));
    db_key_value_map.set(102, serializable_string("D"));
    db_key_value_map.commit_transaction();

    ASSERT_TRUE(db_key_value_map.begin_transaction());
    ASSERT_EQ(db_key_value_map.size(), 4);
    ptr = db_key_value_map[102];
    ASSERT_EQ(ptr->v, "D");
    ASSERT_TRUE(db_key_value_map.erase_validate(102));
    ASSERT_EQ(db_key_value_map.size(), 3);
    db_key_value_map.erase(102);
    ASSERT_EQ(db_key_value_map.size(), 3);
    ASSERT_FALSE(db_key_value_map.erase_validate(102));
    ASSERT_EQ(db_key_value_map.size(), 3);

    r = false;
    try
    {
      ptr = db_key_value_map[0];
    }
    catch (std::out_of_range&)
    {
      r = true;
    }
    ASSERT_TRUE(r);

    ptr = db_key_value_map[99];
    ASSERT_EQ(ptr->v, "A");

    db_key_value_map.set(102, serializable_string("W"));
    ASSERT_EQ(db_key_value_map.size(), 4);

    ptr = db_key_value_map[102];
    ASSERT_EQ(ptr->v, "W");

    ASSERT_TRUE(db_key_value_map.clear());

    db_key_value_map.commit_transaction();


    ASSERT_TRUE(db_key_value_map.begin_transaction(true));
    ASSERT_EQ(db_key_value_map.size(), 0);
    db_key_value_map.commit_transaction();
  }


  //////////////////////////////////////////////////////////////////////////////
  // 2gb_test
  //////////////////////////////////////////////////////////////////////////////
=======
>>>>>>> commit
  TEST(lmdb, 2gb_test)
  {
    bool r = false;
    epee::shared_recursive_mutex rw_lock;

    epee::log_space::log_singletone::enable_channels("lmdb");

    static const uint64_t buffer_size = 64 * 1024;                                         // 64 KB
    static const uint64_t db_total_size = static_cast<uint64_t>(2.1 * 1024 * 1024 * 1024); // 2.1 GB -- a bit more than 2GB to test 2GB boundary
    static const std::string db_file_path = "2gb_lmdb_test";

    std::shared_ptr<db::lmdb_db_backend> lmdb_ptr = std::make_shared<db::lmdb_db_backend>();
    db::basic_db_accessor bdba(lmdb_ptr, rw_lock);

    //
    // write data
    //

    r = bdba.open(db_file_path, CACHE_SIZE);
    ASSERT_TRUE(r);

    db::container_handle h;
    r = lmdb_ptr->open_container("c1", h);
    ASSERT_TRUE(r);

    ASSERT_TRUE(lmdb_ptr->begin_transaction());
    ASSERT_TRUE(lmdb_ptr->clear(h));

    std::vector<uint8_t> buffer;
    buffer.resize(buffer_size);
    crypto::generate_random_bytes(buffer_size, buffer.data());

    uint64_t total_data = 0;
    for (uint64_t key = 0; key < db_total_size / buffer_size; ++key)
    {
      r = lmdb_ptr->set(h, (char*)&key, sizeof key, reinterpret_cast<const char*>(buffer.data()), buffer_size);
      ASSERT_TRUE(r);
      total_data += buffer_size;

      if (key % 1024 == 0)
      {
        ASSERT_TRUE(lmdb_ptr->commit_transaction());
<<<<<<< HEAD
        //ASSERT_TRUE(lmdb_ptr->resize_if_needed());
=======
        ASSERT_TRUE(lmdb_ptr->resize_if_needed());
>>>>>>> commit
        ASSERT_TRUE(lmdb_ptr->begin_transaction());
        std::cout << total_data / 1024 / 1024 << " MB written to DB" << ENDL;
      }
    }

    ASSERT_TRUE(lmdb_ptr->commit_transaction());

    r = bdba.close();
    ASSERT_TRUE(r);


    //
    // read data and check
    //

    r = bdba.open(db_file_path);
    ASSERT_TRUE(r);
    r = lmdb_ptr->open_container("c1", h);
    ASSERT_TRUE(r);

    ASSERT_TRUE(lmdb_ptr->begin_transaction());

    std::string out_buffer;
    total_data = 0;
    for (uint64_t key = 0; key < db_total_size / buffer_size; ++key)
    {
      r = lmdb_ptr->get(h, (char*)&key, sizeof key, out_buffer);
      ASSERT_TRUE(r);
      ASSERT_EQ(buffer_size, out_buffer.size());

      ASSERT_TRUE(0 == memcmp(buffer.data(), out_buffer.c_str(), buffer_size));

      total_data += buffer_size;
      if (key % 1024 == 0)
        std::cout << total_data / 1024 / 1024 << " MB read from DB" << ENDL;
    }

    ASSERT_TRUE(lmdb_ptr->commit_transaction());

    r = bdba.close();
    ASSERT_TRUE(r);

    boost::filesystem::remove_all(db_file_path);

  }

<<<<<<< HEAD
} // namespace lmdb_test
=======
}
>>>>>>> commit
