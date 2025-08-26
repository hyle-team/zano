// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/multiprecision/cpp_int.hpp>
#include <sstream>
#include "common/db_abstract_accessor.h"
#include "common/db_backend_lmdb.h"
#include "common/util.h"
#include "cache_helper.h"
#include "serialization/binary_archive.h"
#include "serialization/serialization.h"
#include "serialization/multiprecision.h"

#define DB_TEST_SUB_DIR   "db_test"
#define DB_TEST_SIMPLE_KV_ACCESSOR_NAME   "accessor1"


struct simple_get_set_test
{
  typedef tools::db::cached_key_value_accessor<uint64_t, std::string, true, true> accessor_type;
  static bool run_test(accessor_type& acc)
  {

    acc.begin_transaction();


    uint64_t key = 2342342342;
    std::string data1;
    data1 = "lkel;s;lwedck";

    acc.set(key, data1);

    auto ptr = acc.get(key);
    CHECK_AND_ASSERT_MES(ptr.get(), false, "unable to get");

    CHECK_AND_ASSERT_MES(*ptr == data1, false, "missmatch");
    acc.commit_transaction();

    LOG_PRINT_GREEN("Test successful", LOG_LEVEL_0);
    return true;
  }
};


struct simple_check_count
{
  typedef tools::db::cached_key_value_accessor<uint64_t, std::string, true, true> accessor_type;
  static bool run_test(accessor_type& acc)
  {
    acc.begin_transaction();


    uint64_t key1 = 2342342342;
    std::string data1 = "lkel;s;lwedck";

    uint64_t key2 = 2341342232;
    std::string data2 = "lkel;s;lffwedck";

    acc.set(key1, data1);
    acc.set(key2, data2);

    uint64_t cz = acc.size();
    CHECK_AND_ASSERT_MES(cz == 2, false, "");

    acc.erase(key1);
    cz = acc.size();
    CHECK_AND_ASSERT_MES(cz == 1, false, "");

    auto r = acc.find(key1);
    CHECK_AND_ASSERT_MES(r == acc.end(), false, "");
    auto r2 = acc.find(key2);
    CHECK_AND_ASSERT_MES(r2 != acc.end(), false, "");


    acc.clear();
    cz = acc.size();
    CHECK_AND_ASSERT_MES(cz == 0, false, "");

    acc.commit_transaction();

    LOG_PRINT_GREEN("Test successful", LOG_LEVEL_0);
    return true;
  }
};

struct check_array_container
{
  typedef tools::db::array_accessor<std::string, true> accessor_type;

  static bool run_test(accessor_type& acc)
  {
    acc.begin_transaction();

    std::string data1 = "lkel;s;lwedck";
    std::string data2 = "lkel;s;lswedck";

    acc.push_back(data1);
    acc.push_back(data2);

    uint64_t cz = acc.size();
    CHECK_AND_ASSERT_MES(cz == 2, false, "");
    
    auto vptr = acc.get(0);
    CHECK_AND_ASSERT_MES(vptr.get(), false, "");
    CHECK_AND_ASSERT_MES(*vptr == data1, false, "");

    vptr = acc.get(1);
    CHECK_AND_ASSERT_MES(vptr.get(), false, "");
    CHECK_AND_ASSERT_MES(*vptr == data2, false, "");

    std::vector<std::pair<uint64_t, std::string >> enum_res;
    acc.enumerate_items([&](uint64_t i, uint64_t i_, const std::string& val){
      enum_res.push_back(std::make_pair(i_, val));
      return true;
    });

    CHECK_AND_ASSERT_MES(enum_res.size() == 2, false, "");
    CHECK_AND_ASSERT_MES(enum_res[0].first == 0 && enum_res[0].second == data1, false, "");
    CHECK_AND_ASSERT_MES(enum_res[1].first == 1 && enum_res[1].second == data2, false, "");


    acc.pop_back();
    cz = acc.size();
    CHECK_AND_ASSERT_MES(cz == 1, false, "");

    vptr = acc.get(1);
    CHECK_AND_ASSERT_MES(!vptr.get(), false, "");


    vptr = acc.back();
    CHECK_AND_ASSERT_MES(vptr.get(), false, "");
    CHECK_AND_ASSERT_MES(*vptr == data1, false, "");


    acc.commit_transaction();

    LOG_PRINT_GREEN("Test successful", LOG_LEVEL_0);
    return true;
  }
};


struct check_subcontainer_container
{
  typedef tools::db::basic_key_to_array_accessor<uint64_t, std::string, true> accessor_type;

  static bool run_test(accessor_type& acc)
  {
    acc.begin_transaction();

    uint64_t subcontainer_1 = 22;
    uint64_t subcontainer_2 = 0;

    std::string data1 = "lkel;s;lwedck";
    std::string data2 = "lkel;s;lwedck";

    acc.push_back_item(subcontainer_1, data1);
    acc.push_back_item(subcontainer_1, data2);

    uint64_t cz = acc.get_item_size(subcontainer_1);
    CHECK_AND_ASSERT_MES(cz == 2, false, "");

    cz = acc.get_item_size(subcontainer_2);
    CHECK_AND_ASSERT_MES(cz == 0, false, "");

    //----------
    acc.push_back_item(subcontainer_2, data1);
    acc.push_back_item(subcontainer_2, data2);

    cz = acc.get_item_size(subcontainer_2);
    CHECK_AND_ASSERT_MES(cz == 2, false, "");

    auto vptr = acc.get_subitem(subcontainer_1, 0);
    CHECK_AND_ASSERT_MES(vptr.get(), false, "");
    CHECK_AND_ASSERT_MES(*vptr == data1, false, "");

    vptr = acc.get_subitem(subcontainer_1, 1);
    CHECK_AND_ASSERT_MES(vptr.get(), false, "");
    CHECK_AND_ASSERT_MES(*vptr == data2, false, "");

    vptr = acc.get_subitem(subcontainer_2, 0);
    CHECK_AND_ASSERT_MES(vptr.get(), false, "");
    CHECK_AND_ASSERT_MES(*vptr == data1, false, "");

    vptr = acc.get_subitem(subcontainer_2, 1);
    CHECK_AND_ASSERT_MES(vptr.get(), false, "");
    CHECK_AND_ASSERT_MES(*vptr == data2, false, "");

    //===========

    acc.pop_back_item(subcontainer_1);
    cz = acc.get_item_size(subcontainer_1);
    CHECK_AND_ASSERT_MES(cz == 1, false, "");

    acc.pop_back_item(subcontainer_2);
    cz = acc.get_item_size(subcontainer_2);
    CHECK_AND_ASSERT_MES(cz == 1, false, "");

    acc.commit_transaction();

    LOG_PRINT_GREEN("Test successful", LOG_LEVEL_0);
    return true;
  }
};


template<class t_test>
bool prepare_db_and_run_test()
{
  epee::log_space::log_singletone::set_thread_log_prefix(std::string("[") + typeid(t_test).name() + "]");
  LOG_PRINT_MAGENTA("[" << typeid(t_test).name() << "]", LOG_LEVEL_0);


  epee::shared_recursive_mutex rw_lock;
  tools::db::basic_db_accessor m_db(std::shared_ptr<tools::db::i_db_backend>(new tools::db::lmdb_db_backend()), rw_lock);
  typename t_test::accessor_type simple_k_v_accessor(m_db);

  std::string test_folder = epee::string_tools::get_current_module_folder() + "/" + DB_TEST_SUB_DIR;

  boost::system::error_code ec;
  boost::filesystem::remove_all(test_folder, ec);
  if (ec)
  {
    LOG_ERROR("Failed to empty db subdir: " << ec.message());
    return false;
  }
  tools::create_directories_if_necessary(test_folder);

  bool r = m_db.open(test_folder);
  CHECK_AND_ASSERT_MES(r, 0, "Failed to open DB from " << test_folder);
  r = simple_k_v_accessor.init(DB_TEST_SIMPLE_KV_ACCESSOR_NAME);
  CHECK_AND_ASSERT_MES(r, 0, "Failed to open DB_TEST_SIMPLE_KV_ACCESSOR_NAME");

  return t_test::run_test(simple_k_v_accessor);
}

bool db_cache_test()
{
#define CACHE_TEST_MAX_ALOWED_RECORDS   1000
#define CACHE_TEST_TOTAL_RECORDS        10000
  epee::misc_utils::cache_base<true, uint64_t, std::string, CACHE_TEST_MAX_ALOWED_RECORDS> cache;
  //std::map<uint64_t, std::string> source;

  for (uint64_t i = 0; i != CACHE_TEST_TOTAL_RECORDS; i++)
  {
    cache.set(i, std::to_string(i));
  }

  CHECK_AND_ASSERT_MES(cache.size() == CACHE_TEST_MAX_ALOWED_RECORDS, 0, "Wrong cache size");


  for (uint64_t i = CACHE_TEST_TOTAL_RECORDS-1; i != 0; i--)
  {
    std::string str_res;
    bool r = cache.get(i, str_res);
    if (i < CACHE_TEST_TOTAL_RECORDS - CACHE_TEST_MAX_ALOWED_RECORDS)
    {
      CHECK_AND_ASSERT_MES(!r, false, "wrong result");
    }
    else
    {
      CHECK_AND_ASSERT_MES(r, false, "wrong result");
      CHECK_AND_ASSERT_MES(str_res == std::to_string(i), false, "wrong result");
    }
  }
  
  for (uint64_t i = 0; i != 100; i++)
  {
    cache.set(i, std::to_string(i));
  }

#define VALIDATE_GET(i, expected_val) \
  {std::string str_res; bool r = cache.get(i, str_res); if (expected_val){ CHECK_AND_ASSERT_MES(r && str_res == std::to_string(i), false, "wrong condition"); } else { CHECK_AND_ASSERT_MES(!r, false, "wrong condition"); } }

  VALIDATE_GET(0, true);
  VALIDATE_GET(10, true);
  VALIDATE_GET(50, true);
  VALIDATE_GET(99, true);
  VALIDATE_GET(100, false);

  VALIDATE_GET(CACHE_TEST_TOTAL_RECORDS - 1, false );
  VALIDATE_GET(CACHE_TEST_TOTAL_RECORDS - 2, false);
  VALIDATE_GET(CACHE_TEST_TOTAL_RECORDS - 10, false);
  VALIDATE_GET(CACHE_TEST_TOTAL_RECORDS - 50, false);
  VALIDATE_GET(CACHE_TEST_TOTAL_RECORDS - 99, false);
  VALIDATE_GET(CACHE_TEST_TOTAL_RECORDS - 100, false);
  VALIDATE_GET(CACHE_TEST_TOTAL_RECORDS - 101, true);
  VALIDATE_GET(CACHE_TEST_TOTAL_RECORDS - 110, true);
  return true;
}

struct check_boost_multiprecision_container
{
  struct test_mp_struct
  {
    DEFINE_SERIALIZATION_VERSION(1)
    boost::multiprecision::uint128_t some_mp_1;
    boost::multiprecision::uint128_t some_mp_2;
    std::string some_str;
    uint32_t version;
    BEGIN_SERIALIZE_OBJECT()
      VERSION_ENTRY(version)
      FIELD(some_mp_1)
      FIELD(some_mp_2)
      FIELD(some_str)
    END_SERIALIZE()
  };

  typedef tools::db::basic_key_value_accessor<uint64_t, test_mp_struct, true> accessor_type;

  static bool run_test(accessor_type& acc)
  {
    acc.begin_transaction();
    test_mp_struct ts = AUTO_VAL_INIT(ts);
    uint64_t a = 0xfddfdfeffddfdfefll;
    uint64_t key = 0;
    ts.some_mp_1 = a;
    ts.some_mp_1 = ts.some_mp_1 * a;
    ts.some_mp_2 = ts.some_mp_1 * a;
    ts.some_str = ts.some_mp_2.convert_to<std::string>();
    acc.set(key, ts);


    
    auto ts2_ptr = acc.get(key);
    CHECK_AND_ASSERT_MES(ts2_ptr, false, "");

    CHECK_AND_ASSERT_MES(ts2_ptr->some_mp_1 == ts.some_mp_1, false, "");
    CHECK_AND_ASSERT_MES(ts2_ptr->some_mp_2 == ts.some_mp_2, false, "");
    CHECK_AND_ASSERT_MES(ts2_ptr->some_str == ts.some_str, false, "");


    acc.commit_transaction();
    LOG_PRINT_GREEN("Test successful", LOG_LEVEL_0);
    return true;
  }
};


bool solo_container_save_load_test_iteration(bool clear_data, uint64_t& read_value, uint64_t value_to_set)
{
#define BLOCKCHAIN_STORAGE_OPTIONS_ID_STORAGE_MAJOR_COMPABILITY_VERSION     3
#define BLOCKCHAIN_STORAGE_CONTAINER_SOLO_OPTIONS     "solo"

  typedef tools::db::cached_key_value_accessor<uint64_t, uint64_t, false, true> solo_options_container;
  epee::shared_recursive_mutex m_rwlock;
  tools::db::basic_db_accessor m_db(std::shared_ptr<tools::db::i_db_backend>(new tools::db::lmdb_db_backend), m_rwlock);
  solo_options_container m_db_solo_options(m_db);
  tools::db::solo_db_value<uint64_t, uint64_t, solo_options_container> m_db_storage_major_compability_version(BLOCKCHAIN_STORAGE_OPTIONS_ID_STORAGE_MAJOR_COMPABILITY_VERSION, m_db_solo_options);

  std::string test_folder = epee::string_tools::get_current_module_folder() + "/" + DB_TEST_SUB_DIR;
  bool r = false;

  if (clear_data)
  {
    boost::system::error_code ec;
    boost::filesystem::remove_all(test_folder, ec);
    CHECK_AND_ASSERT_MES(!ec, false, "Failed to empty db subdir: " << ec.message());
    r = tools::create_directories_if_necessary(test_folder);
    CHECK_AND_ASSERT_MES(r, false, "create_directories_if_necessary failed");
  }

  r = m_db.open(test_folder);
  CHECK_AND_ASSERT_MES(r, false, "Failed to initialize database in folder: " << test_folder);
  r = m_db_solo_options.init(BLOCKCHAIN_STORAGE_CONTAINER_SOLO_OPTIONS);
  CHECK_AND_ASSERT_MES(r, false, "Unable to init db container");

  read_value = m_db_storage_major_compability_version;
  auto db_tx_ptr = m_db.begin_transaction_obj();
  m_db_storage_major_compability_version = value_to_set;
  db_tx_ptr->commit_transaction();

  m_db.close();
  return true;
}

bool solo_container_save_load_test()
{
  const char * const test_name = "solo container save/load test";
  epee::log_space::log_singletone::set_thread_log_prefix(std::string("[") + test_name + "]");
  LOG_PRINT_MAGENTA("[" << test_name << "]", LOG_LEVEL_0);

  uint64_t v = 100, expected_v = 0;
  bool r = solo_container_save_load_test_iteration(true, v, 1);
  CHECK_AND_ASSERT_MES(r, false, "solo_container_test_load_save_iteration failed");
  expected_v = 0;
  CHECK_AND_ASSERT_MES(v == expected_v, false, "solo_container_save_load_test_iteration returned invalid value from solo container: " << v << ", expected: " << expected_v);

  r = solo_container_save_load_test_iteration(false, v, 2);
  CHECK_AND_ASSERT_MES(r, false, "solo_container_test_load_save_iteration failed");
  expected_v = 1;
  CHECK_AND_ASSERT_MES(v == expected_v, false, "solo_container_save_load_test_iteration returned invalid value from solo container: " << v << ", expected: " << expected_v);

  r = solo_container_save_load_test_iteration(false, v, 3);
  CHECK_AND_ASSERT_MES(r, false, "solo_container_test_load_save_iteration failed");
  expected_v = 2;
  CHECK_AND_ASSERT_MES(v == expected_v, false, "solo_container_save_load_test_iteration returned invalid value from solo container: " << v << ", expected: " << expected_v);

  LOG_PRINT_GREEN("Test successful", LOG_LEVEL_0);
  return true;
}

struct lmdb_txn_basic_write_test
{
  typedef tools::db::cached_key_value_accessor<uint64_t, std::string, true, true> accessor_type;
  static bool run_test(accessor_type& acc)
  {
    acc.begin_transaction();

    uint64_t key = 12345;
    std::string data = "test_data";

    acc.set(key, data);

    auto ptr = acc.get(key);
    CHECK_AND_ASSERT_MES(ptr.get() && *ptr == data, false, "Data mismatch after set in transaction");

    acc.commit_transaction();

    acc.begin_transaction(true);
    ptr = acc.get(key);
    CHECK_AND_ASSERT_MES(ptr.get() && *ptr == data, false, "Data not persisted after commit");
    acc.commit_transaction();

    LOG_PRINT_GREEN("Test successful", LOG_LEVEL_0);
    return true;
  }
};

struct lmdb_txn_basic_read_only_test
{
  typedef tools::db::cached_key_value_accessor<uint64_t, std::string, true, true> accessor_type;
  static bool run_test(accessor_type& acc)
  {
    acc.begin_transaction(false);
    uint64_t key = 67890;
    std::string data = "read_only_data";
    acc.set(key, data);
    acc.commit_transaction();

    acc.begin_transaction(true);
    auto ptr = acc.get(key);
    CHECK_AND_ASSERT_MES(ptr.get() && *ptr == data, false, "Data not readable in read-only transaction");

    try
    {
      acc.set(key, "modified");
      CHECK_AND_ASSERT_MES(false, false, "Write allowed in read-only transaction");
    }
    catch (...) {}

    acc.commit_transaction();

    acc.begin_transaction(true);
    ptr = acc.get(key);
    CHECK_AND_ASSERT_MES(ptr.get() && *ptr == data, false, "Data modified unexpectedly in read-only");
    acc.commit_transaction();

    LOG_PRINT_GREEN("lmdb_txn_basic_read_only_test successful", LOG_LEVEL_0);
    return true;
  }
};

struct lmdb_txn_nested_read_only_test
{
  typedef tools::db::cached_key_value_accessor<uint64_t, std::string, true, true> accessor_type;
  static bool run_test(accessor_type& acc)
  {
    // Insert initial data
    acc.begin_transaction(false);
    uint64_t key = 11111;
    std::string data = "nested_read_data";
    acc.set(key, data);
    //acc.commit_transaction();

    // Start outer read-only
    acc.begin_transaction(true);

    // Nested read-only
    acc.begin_transaction(true);
    auto ptr = acc.get(key);
    CHECK_AND_ASSERT_MES(ptr.get() && *ptr == data, false, "Data not readable in nested read-only");

    // Another nested
    /*
    acc.begin_transaction(true);
    ptr = acc.get(key);
    CHECK_AND_ASSERT_MES(ptr.get() && *ptr == data, false, "Data not readable in double nested read-only");
    acc.commit_transaction(); // Commit inner
    */
    acc.commit_transaction(); // Commit middle

    acc.commit_transaction(); // Commit outer
    acc.commit_transaction(); // writer


    //======
    acc.begin_transaction(false);
    //uint64_t key = 11111;
    //std::string data = "nested_read_data";
    //acc.set(key, data);
    //acc.commit_transaction();

    // Start outer read-only
    acc.begin_transaction(true);

    // Nested read-only
    acc.begin_transaction(true);
    //auto ptr = acc.get(key);
    //CHECK_AND_ASSERT_MES(ptr.get() && *ptr == data, false, "Data not readable in nested read-only");

    // Another nested
    /*
    acc.begin_transaction(true);
    ptr = acc.get(key);
    CHECK_AND_ASSERT_MES(ptr.get() && *ptr == data, false, "Data not readable in double nested read-only");
    acc.commit_transaction(); // Commit inner
    */
    acc.commit_transaction(); // Commit middle

    acc.commit_transaction(); // Commit outer
    acc.commit_transaction(); // writer



    LOG_PRINT_GREEN("lmdb_txn_nested_read_only_test successful", LOG_LEVEL_0);
    return true;
  }
};

struct lmdb_txn_abort_test
{
  typedef tools::db::cached_key_value_accessor<uint64_t, std::string, true, true> accessor_type;
  static bool run_test(accessor_type& acc)
  {
    uint64_t key = 22222;
    std::string data = "abort_data";

    acc.begin_transaction(false);
    acc.set(key, data);
    acc.abort_transaction();

    // Verify data not persisted
    acc.begin_transaction(true);
    auto ptr = acc.get(key);
    CHECK_AND_ASSERT_MES(!ptr.get(), false, "Data persisted after abort");
    acc.commit_transaction();

    LOG_PRINT_GREEN("lmdb_txn_abort_test successful", LOG_LEVEL_0);
    return true;
  }
};

struct lmdb_txn_destructor_abort_test
{
  typedef tools::db::cached_key_value_accessor<uint64_t, std::string, true, true> accessor_type;

  static bool run_test(accessor_type& /*acc*/)
  {
    const std::string base = epee::string_tools::get_current_module_folder() + std::string("/") + DB_TEST_SUB_DIR + "/lmdb_txn_dtor";
    boost::system::error_code ec;
    boost::filesystem::remove_all(base, ec);
    CHECK_AND_ASSERT_MES(!ec, false, "Failed to empty db subdir: " << ec.message());
    CHECK_AND_ASSERT_MES(tools::create_directories_if_necessary(base), false, "mkdir failed");

    epee::shared_recursive_mutex rwlock;
    auto backend_sp = std::make_shared<tools::db::lmdb_db_backend>();
    tools::db::lmdb_db_backend* backend = backend_sp.get();
    tools::db::basic_db_accessor bdb(backend_sp, rwlock);

    CHECK_AND_ASSERT_MES(bdb.open(base), false, "backend open failed");

    tools::db::container_handle h = 0;
    CHECK_AND_ASSERT_MES(bdb.get_backend()->open_container("txn", h), false, "open_container failed");
    MDB_dbi dbi = static_cast<MDB_dbi>(h);

    const std::string key = "dtor_key";
    const std::string val = "dtor_val";

    {
      tools::db::lmdb_db_backend::lmdb_txn txn(*backend, false);
      int rc = txn.begin(nullptr);
      CHECK_AND_ASSERT_MES(rc == MDB_SUCCESS, false, "txn.begin failed");

      MDB_val k = AUTO_VAL_INIT(k);
      MDB_val v = AUTO_VAL_INIT(v);
      k.mv_data = (void*)key.data(); k.mv_size = key.size();
      v.mv_data = (void*)val.data(); v.mv_size = val.size();

      rc = mdb_put(txn.ptx, dbi, &k, &v, 0);
      CHECK_AND_ASSERT_MES(rc == MDB_SUCCESS, false, "mdb_put failed");
    }

    CHECK_AND_ASSERT_MES(bdb.begin_transaction(true), false, "ro begin failed");
    std::string out;
    bool found = backend->get(h, key.data(), key.size(), out);
    bdb.commit_transaction();
    CHECK_AND_ASSERT_MES(!found, false, "Data persisted after lmdb_txn destructor — expected abort");

    bdb.close();
    LOG_PRINT_GREEN("Test successful", LOG_LEVEL_0);
    return true;
  }
};

struct lmdb_txn_thread_specific_test
{
  typedef tools::db::cached_key_value_accessor<uint64_t, std::string, true, true> accessor_type;
  static bool run_test(accessor_type& acc)
  {
    acc.begin_transaction(false);
    uint64_t key = 44444;
    std::string data_main = "main_thread_data";
    acc.set(key, data_main);
    acc.commit_transaction();

    // Read in separate thread, but without macros assert inside
    std::atomic<bool> thread_ok{true};
    std::atomic<bool> saw_expected{false};

    std::thread th([&](){
      try
      {
        acc.begin_transaction(true);
        auto ptr = acc.get(key);
        if (ptr.get() && *ptr == data_main)
          saw_expected.store(true, std::memory_order_relaxed);
        acc.commit_transaction();
      }
      catch (...)
      {
        thread_ok.store(false, std::memory_order_relaxed);
      }
    });

    th.join();
    CHECK_AND_ASSERT_MES(thread_ok.load(), false, "reader thread threw an exception");
    CHECK_AND_ASSERT_MES(saw_expected.load(), false, "Data not visible in thread read-only txn");

    std::thread th_write([&](){
    try
    {
      acc.begin_transaction(false);
      uint64_t key_thread = 55555;
      std::string data_thread = "thread_data";
      acc.set(key_thread, data_thread);
      acc.commit_transaction();
    }
    catch (...)
    {
      thread_ok.store(false, std::memory_order_relaxed);
    }
    });

    th_write.join();
    CHECK_AND_ASSERT_MES(thread_ok.load(), false, "writer thread threw an exception");

    acc.begin_transaction(true);
    auto ptr = acc.get(55555);
    CHECK_AND_ASSERT_MES(ptr.get() && *ptr == "thread_data", false, "Thread write not persisted");
    acc.commit_transaction();
    auto backend_sp = acc.get_db_accessor().get_backend();
    auto* backend = dynamic_cast<tools::db::lmdb_db_backend*>(backend_sp.get());
    if (backend)
      backend->dump();
    LOG_PRINT_GREEN("lmdb_txn_thread_specific_test successful", LOG_LEVEL_0);
    return true;
  }
};

struct lmdb_txn_stress_multithread_test
{
  typedef tools::db::cached_key_value_accessor<uint64_t, std::string, true, true> accessor_type;
  // Two threads read with 10 nested ROs, the third writes, in parallel; check isolation and locks.
  static bool run_test(accessor_type& acc)
  {
    const uint64_t key = 77777;
    acc.begin_transaction(false);
    acc.set(key, std::string("v0"));
    acc.commit_transaction();
    acc.clear_cache();

    std::atomic<bool> stop_readers{false};
    std::atomic<uint64_t> read_loops{0}, read_failures{0};
    std::atomic<uint64_t> write_loops{0}, write_failures{0};

    auto reader = [&]()
    {
      auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
      const int depth = 10;
      while (std::chrono::steady_clock::now() < deadline && !stop_readers.load(std::memory_order_relaxed))
      {
        int opened = 0;
        try
        {
          // open depth nested read-only transactions
          for (int i = 0; i < depth; ++i) { acc.begin_transaction(true); ++opened; }

          //just read the value — it should exist
          auto p = acc.get(key);
          if (!p.get())
            ++read_failures;
          // close all levels (for read-only commit == correct finish)
          for (int i = 0; i < depth; ++i) acc.commit_transaction();

          ++read_loops;
        }
        catch (...)
        {
          ++read_failures;
          // try to correctly close already opened levels (commit for read-only)
          for (int i = 0; i < opened; ++i)
          {
            try { acc.commit_transaction(); } catch (...) {}
          }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    };

    auto writer = [&]()
    {
      auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
      uint64_t n = 1;
      while (std::chrono::steady_clock::now() < deadline)
      {
        bool opened = false;
        try
        {
          acc.begin_transaction(false);
          opened = true;
          acc.set(key, std::string("v") + std::to_string(n++));
          acc.commit_transaction();
          opened = false;
          ++write_loops;
        }
        catch (...)
        {
          ++write_failures;
          if (opened)
          {
            try { acc.abort_transaction(); } catch (...) {}
          }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    };

    std::thread r1(reader), r2(reader);
    std::thread w(writer);

    r1.join();
    r2.join();
    w.join();
    stop_readers.store(true, std::memory_order_relaxed);

    CHECK_AND_ASSERT_MES(read_loops.load() > 0, false, "read loops == 0");
    CHECK_AND_ASSERT_MES(write_loops.load() > 0, false, "write loops == 0");
    CHECK_AND_ASSERT_MES(read_failures.load() == 0, false, "read failures: " << read_failures.load());
    CHECK_AND_ASSERT_MES(write_failures.load() == 0, false, "write failures: " << write_failures.load());

    acc.begin_transaction(true);
    auto final_ptr = acc.get(key);
    CHECK_AND_ASSERT_MES(final_ptr.get(), false, "final value missing after stress");
    acc.commit_transaction();
    
    LOG_PRINT_GREEN("[stress] OK, reads=" << read_loops.load() << " writes=" << write_loops.load(), LOG_LEVEL_0);
    return true;
  }
};

struct lmdb_ro_double_begin_bad_rslot_test
{
  typedef tools::db::cached_key_value_accessor<uint64_t, std::string, true, true> accessor_type;

  static bool run_test(accessor_type& /*acc*/)
  {
    const std::string base =
      epee::string_tools::get_current_module_folder() + std::string("/") + DB_TEST_SUB_DIR + "/ro_double_begin";

    boost::system::error_code ec;
    boost::filesystem::remove_all(base, ec);
    CHECK_AND_ASSERT_MES(!ec, false, "Failed to empty db subdir: " << ec.message());
    CHECK_AND_ASSERT_MES(tools::create_directories_if_necessary(base), false, "mkdir failed");

    epee::shared_recursive_mutex rwlock;
    auto backend_sp = std::make_shared<tools::db::lmdb_db_backend>();
    tools::db::lmdb_db_backend* backend = backend_sp.get();
    tools::db::basic_db_accessor bdb(backend_sp, rwlock);

    CHECK_AND_ASSERT_MES(bdb.open(base), false, "backend open failed");

    tools::db::container_handle h = 0;
    CHECK_AND_ASSERT_MES(bdb.get_backend()->open_container("c", h), false, "open_container failed");

    tools::db::lmdb_db_backend::lmdb_txn t1(*backend, /*is_read_only=*/true);
    int rc1 = t1.begin(nullptr);
    CHECK_AND_ASSERT_MES(rc1 == MDB_SUCCESS, false, "first ro begin failed: " << mdb_strerror(rc1));

    tools::db::lmdb_db_backend::lmdb_txn t2(*backend, /*is_read_only=*/true);
    int rc2 = t2.begin(nullptr);

    LOG_PRINT_MAGENTA("second ro begin rc=" << rc2 << " (" << mdb_strerror(rc2) << ")", LOG_LEVEL_0);

    const int MDB_BAD_RSLOT_LOCAL = -30783;
    CHECK_AND_ASSERT_MES(rc2 == MDB_BAD_RSLOT_LOCAL, false, "Expected MDB_BAD_RSLOT on second ro begin in same thread");

    t1.abort();
    backend_sp->dump();
    bdb.close();

    LOG_PRINT_GREEN("lmdb_ro_double_begin_bad_rslot_test reproduced BAD_RSLOT", LOG_LEVEL_0);
    return true;
  }
};

struct lmdb_ro_cross_thread_finish_test
{
  typedef tools::db::cached_key_value_accessor<uint64_t, std::string, true, true> accessor_type;

  static bool run_test(accessor_type& /*acc*/)
  {
    const std::string base =
      epee::string_tools::get_current_module_folder() + std::string("/") + DB_TEST_SUB_DIR + "/ro_cross_thread";

    boost::system::error_code ec;
    boost::filesystem::remove_all(base, ec);
    CHECK_AND_ASSERT_MES(!ec, false, "Failed to empty db subdir: " << ec.message());
    CHECK_AND_ASSERT_MES(tools::create_directories_if_necessary(base), false, "mkdir failed");

    epee::shared_recursive_mutex rwlock;
    auto backend_sp = std::make_shared<tools::db::lmdb_db_backend>();
    tools::db::lmdb_db_backend* backend = backend_sp.get();
    tools::db::basic_db_accessor bdb(backend_sp, rwlock);
    CHECK_AND_ASSERT_MES(bdb.open(base), false, "backend open failed");

    tools::db::container_handle h = 0;
    CHECK_AND_ASSERT_MES(bdb.get_backend()->open_container("c", h), false, "open_container failed");

    tools::db::lmdb_db_backend::lmdb_txn t(*backend, /*is_read_only=*/true);
    int rc = t.begin(nullptr);
    CHECK_AND_ASSERT_MES(rc == MDB_SUCCESS, false, "ro begin failed: " << mdb_strerror(rc));

    std::atomic<bool> threw{false};

    std::thread th([&]()
    {
      try {
        t.commit();
      } catch (...) {
        threw.store(true, std::memory_order_relaxed);
      }
    });
    th.join();
    CHECK_AND_ASSERT_MES(threw.load(), false, "owner guard didn't fire for cross-thread commit");

    t.abort();

    backend_sp->dump();
    bdb.close();
    LOG_PRINT_GREEN("lmdb_ro_cross_thread_finish_test reproduced cross-thread misuse error", LOG_LEVEL_0);
    return true;
  }
};

int main(int argc, char* argv[])
{
  epee::string_tools::set_module_name_and_folder(argv[0]);
  epee::log_space::get_set_log_detalisation_level(true, LOG_LEVEL_2);
  epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_2);

  //if (!prepare_db_and_run_test<lmdb_ro_double_begin_bad_rslot_test>())
  //  return 0;

  //if (!prepare_db_and_run_test<lmdb_ro_cross_thread_finish_test>())
  //  return 0;

  if (!prepare_db_and_run_test<lmdb_txn_basic_write_test>())
    return 0;

  // if (!prepare_db_and_run_test<lmdb_txn_basic_read_only_test>())
  //   return 0;

  if (!prepare_db_and_run_test<lmdb_txn_nested_read_only_test>())
    return 0;

  if (!prepare_db_and_run_test<lmdb_txn_abort_test>())
    return 0;

  if (!prepare_db_and_run_test<lmdb_txn_destructor_abort_test>())
    return 0;

  if (!prepare_db_and_run_test<lmdb_txn_thread_specific_test>())
    return 0;

  if (!prepare_db_and_run_test<check_boost_multiprecision_container>())
    return 0;

  if (!db_cache_test())
    return 0;

  if (!prepare_db_and_run_test<simple_get_set_test>())
    return 0;

  if (!prepare_db_and_run_test<simple_check_count>())
    return 0;

  if (!prepare_db_and_run_test<check_array_container>())
    return 0;

  if (!prepare_db_and_run_test<check_subcontainer_container>())
    return 0;

  if (!solo_container_save_load_test())
    return 0;

  if (!prepare_db_and_run_test<lmdb_txn_stress_multithread_test>())
    return 0;

  return 1;
}
