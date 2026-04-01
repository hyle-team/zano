// Copyright (c) 2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <algorithm>

#define USE_INSECURE_RANDOM_RPNG_ROUTINES // turns on random manupulation for tests

#include "gtest/gtest.h"
#include "currency_core/currency_format_utils.h"
#include "common/db_abstract_accessor.h"
#include "common/median_db_cache.h"
#include "common/db_backend_lmdb.h"
#include "common/util.h"
#include "misc_log_ex.h"
#include "crypto/crypto.h"
#include "serialization/serialization.h"
#include "file_io_utils.h"

inline uint64_t random_in_range(uint64_t from, uint64_t to)
{
  if (from == to)
    return from;
  CHECK_AND_ASSERT_MES(from < to, 0, "Invalid arguments: from = " << from << ", to = " << to);
  return crypto::rand<uint64_t>() % (to - from + 1) + from;
}


/*
TEST(db_accessor_tests, cached_key_value_accessor_test)
{
  epee::shared_recursive_mutex m_rw_lock;
  tools::db::basic_db_accessor m_db(std::shared_ptr<tools::db::i_db_backend>(new tools::db::lmdb_db_backend), m_rw_lock);
  tools::db::cached_key_value_accessor<uint64_t, uint64_t, false, true> m_container(m_db);
  const std::string folder_name = "./TEST_cached_key_value_accessor_test";
  tools::create_directories_if_necessary(folder_name);
  uint64_t cache_size = CACHE_SIZE;
  
  ASSERT_TRUE(m_db.open(folder_name, cache_size));
  ASSERT_TRUE(m_container.init("container"));

  ... TODO ...
}
*/


TEST(db_accessor_tests_2, recoursive_tx_test)
{
  epee::shared_recursive_mutex m_rw_lock;
  tools::db::basic_db_accessor m_db(std::shared_ptr<tools::db::i_db_backend>(new tools::db::lmdb_db_backend), m_rw_lock);
  tools::db::cached_key_value_accessor<uint64_t, uint64_t, false, true> m_container(m_db);

  const std::string folder_name = "./TEST_db_recursive_tx";
  tools::create_directories_if_necessary(folder_name);
  uint64_t cache_size = CACHE_SIZE;
  ASSERT_TRUE(m_db.open(folder_name, cache_size));
  ASSERT_TRUE(m_container.init("zzzz") );

  bool tx_result = m_container.begin_transaction();
  ASSERT_TRUE(tx_result);

  m_container.set(10, 10);
  m_container.set(11, 11);

  tx_result = m_container.begin_transaction(true);
  ASSERT_TRUE(tx_result);
  tx_result = m_container.begin_transaction(true);
  ASSERT_TRUE(tx_result);
  uint64_t r = *m_container.get(10);
  ASSERT_TRUE(r == 10);
  
  tx_result = m_container.begin_transaction();
  
  m_container.set(13, 13);
  m_container.set(14, 14);
  r = *m_container.get(14);
  ASSERT_TRUE(r == 14);

  m_container.commit_transaction();
  r = *m_container.get(14);
  ASSERT_TRUE(r == 14);

  m_container.commit_transaction();
  m_container.commit_transaction();
  m_container.commit_transaction();

}


TEST(db_accessor_tests_2, db_cache_test_with_abort)
{
  epee::shared_recursive_mutex m_rw_lock;
  tools::db::basic_db_accessor m_db(std::shared_ptr<tools::db::i_db_backend>(new tools::db::lmdb_db_backend), m_rw_lock);
  tools::db::cached_key_value_accessor<uint64_t, uint64_t, false, true> m_container(m_db);

  const std::string folder_name = "./TEST_db_recursive_tx";
  tools::create_directories_if_necessary(folder_name);
  uint64_t cache_size = CACHE_SIZE;
  ASSERT_TRUE(m_db.open(folder_name, cache_size));
  ASSERT_TRUE(m_container.init("zzzz"));

  bool tx_result = m_db.begin_transaction();
  ASSERT_TRUE(tx_result);

  m_container.set(10, 10);
  m_container.set(11, 11);

  m_db.commit_transaction();

  tx_result = m_db.begin_transaction();
  ASSERT_TRUE(tx_result);
  m_container.set(10, 0);
  m_container.set(11, 0);
  
  m_db.abort_transaction();

  auto val_ptr10 = m_container.get(10);
  auto val_ptr11 = m_container.get(11);

  ASSERT_TRUE(*val_ptr10 == 10);
  ASSERT_TRUE(*val_ptr11 == 11);

}

template<typename key_t, typename data_t>
struct naive_median
{
  enum { WINDOW_SIZE = ALIAS_COAST_PERIOD };

  naive_median()
  {
    clear();
  }

  void push_item(const key_t& key, const data_t& data)
  {
    m_items.push_back(std::make_pair(key, data));
  }

  void pop_item(uint64_t fee_to_check = UINT64_MAX /* default value means "don't check" */)
  {
    if (m_items.empty())
      return;
    if (fee_to_check != UINT64_MAX)
    {
      ASSERT_EQ(m_items.back().first, fee_to_check);
    }

    m_items.pop_back();
  }

  key_t get_median() const
  {
    if (m_items.empty() || m_items_in_window_count == 0)
      return (key_t)(0);

    items_t local_items(m_items.end() - m_items_in_window_count, m_items.end()); // copy last m_items_in_window_count items
    
    // calculate median in 'local_items'
    auto middle = local_items.begin() + local_items.size() / 2;
    std::nth_element(local_items.begin(), middle, local_items.end());
    if (local_items.size() % 2 == 1)
      return middle->first; // 1, 3, 5...

    auto prev_middle = std::max_element(local_items.begin(), middle); // here we assume all the elements before 'middle' are not greater than middle, thus max_element === prev_middle (s.a. std::nth_element for details)
    return (prev_middle->first + middle->first) / 2;
  }

  // should be called after push/pop sequence but before get_median()
  template<typename cb_t>
  void scan_tems(cb_t cb)
  {
    // count items for which cb() returns true -- this mean 'item in the window'
    m_items_in_window_count = std::count_if(m_items.begin(), m_items.end(), [&cb](const item_t& x) { return cb(x.first, x.second); });
  }

  void clear()
  {
    m_items.clear();
    m_items_in_window_count = 0;
  }

  void print_to_file(const std::string& filename)
  {
    std::stringstream ss;
    ss << "m_items.size()() == " << m_items.size() << ", m_items_in_window_count == " << m_items_in_window_count << ENDL;
    for(size_t i = 0; i < m_items.size(); ++i)
      ss << i << "\t" << m_items[i].first << "\t" << m_items[i].second << ENDL;
    ss << "THE END";
    epee::file_io_utils::save_string_to_file(filename, ss.str());
  }

  typedef std::pair<key_t, data_t> item_t;
  typedef std::vector<item_t> items_t;
  items_t m_items;
  size_t m_items_in_window_count;
};



// Emulate blockchain_storage a little bit
struct bcs_stub_t
{
  bcs_stub_t(tools::median_db_cache<uint64_t, uint64_t>& tx_fee_median, naive_median<uint64_t, uint64_t>& naive_median)
    : m_tx_fee_median(tx_fee_median)
    , m_naive_median(naive_median)
  {}

  bool is_pos_allowed() const { return true; }

  void update_naive_fee_median()
  {
    uint64_t period = is_pos_allowed() ? ALIAS_COAST_PERIOD : ALIAS_COAST_PERIOD / 2;
    int64_t starter_block_index = m_db_blocks.size() - period;
    m_naive_median.scan_tems([starter_block_index](uint64_t fee, uint64_t height) { return (int64_t)height >= starter_block_index; } );
  }

  // almost copy-pasted from blockchain_storage
  bool update_tx_fee_median()
  {
#define PERIOD_DISABLED   0xffffffffffffffffLL


    uint64_t period = is_pos_allowed() ? ALIAS_COAST_PERIOD : ALIAS_COAST_PERIOD / 2;
    if (period > m_db_blocks.size())
      return true; // can cause problem when there's recent_items -- check this case!
    uint64_t starter_block_index = m_db_blocks.size() - period;

    uint64_t purge_recent_period = is_pos_allowed() ? ALIAS_COAST_RECENT_PERIOD : ALIAS_COAST_RECENT_PERIOD / 2;
    uint64_t purge_recent_block_index = 0;
    if (purge_recent_period >= m_db_blocks.size())
      purge_recent_block_index = PERIOD_DISABLED;
    else
      purge_recent_block_index = m_db_blocks.size() - purge_recent_period;


    auto cb = [&](uint64_t fee, uint64_t height)
    {
      if (height >= starter_block_index)
        return true;
      return false;
    };

    auto cb_final_eraser = [&](uint64_t fee, uint64_t height)
    {
      if (purge_recent_block_index == PERIOD_DISABLED)
        return true;
      if (height >= purge_recent_block_index)
        return true;

      return false;
    };

    if (!m_tx_fee_median.scan_items(cb, cb_final_eraser))
    {
      //recent items is over, need to rebuild m_tx_fee_median helper from scratch
      init_tx_fee_median();
      // re-scan again to correctly populate recent items
      bool r = m_tx_fee_median.scan_items(cb, cb_final_eraser);
      CHECK_AND_ASSERT_THROW_MES(r, "internal error: second call of scan_items returned false");
    }
    return true;
  }

  // almost directly copy-pasted from blockchain_storage
  void init_tx_fee_median()
  {
    LOG_PRINT_L0("Initializing tx fee median buffer...");
    //uint64_t period = is_pos_allowed() ? ALIAS_COAST_PERIOD : ALIAS_COAST_PERIOD / 2;
    uint64_t purge_recent_period = is_pos_allowed() ? ALIAS_COAST_RECENT_PERIOD : ALIAS_COAST_RECENT_PERIOD / 2;
    CRITICAL_REGION_LOCAL(m_read_lock);
    m_tx_fee_median.clear();

    if (purge_recent_period > m_db_blocks.size())
      purge_recent_period = m_db_blocks.size();

    std::vector<uint64_t> tx_fee_list;
    size_t tx_count = 0, blocks_count = 0;
    for (uint64_t i = m_db_blocks.size() - purge_recent_period; i != m_db_blocks.size(); i++)
    {
      auto bei_ptr = m_db_blocks[i];
      for (auto fee : bei_ptr.fees)
      {
        m_tx_fee_median.push_item(fee, i);
        ++tx_count;
      }
      ++blocks_count;
    }
    LOG_PRINT_L0("Tx fee median buffer initialized OK (using " << tx_count << " txs from " << blocks_count << " blocks), current fee median is " << m_tx_fee_median.get_median());
  }

  void init_naive_median()
  {
    LOG_PRINT_L0("Initializing naive median buffer...");
    //uint64_t period = is_pos_allowed() ? ALIAS_COAST_PERIOD : ALIAS_COAST_PERIOD / 2;
    m_naive_median.clear();

    size_t tx_count = 0, blocks_count = 0;
    for (uint64_t i = 0; i != m_db_blocks.size(); ++i)
    {
      auto bei_ptr = m_db_blocks[i];
      for (auto fee : bei_ptr.fees)
      {
        m_naive_median.push_item(fee, i);
        ++tx_count;
      }
      ++blocks_count;
    }
    LOG_PRINT_L0("Naive tx fee median buffer initialized OK (using " << tx_count << " txs from " << blocks_count << " blocks), current fee median is " << m_tx_fee_median.get_median());
  }

  bool load_from_file(const std::string& filename)
  {
    std::string buffer;
    CHECK_AND_ASSERT_MES(epee::file_io_utils::load_file_to_string(filename, buffer), false, "load_file_to_string failed for " << filename);
    CHECK_AND_ASSERT_MES(t_unserializable_object_from_blob(*this, buffer), false, "t_unserializable_object_from_blob failed");
    return true;
  }

  bool save_to_file(const std::string& filename) const
  {
    std::string buffer;
    CHECK_AND_ASSERT_MES(t_serializable_object_to_blob(*this, buffer), false, "t_serializable_object_to_blob failed");
    CHECK_AND_ASSERT_MES(epee::file_io_utils::save_string_to_file(filename, buffer), false, "save_string_to_file failed for " << filename);
    return true;
  }

// emulate block header with the list of txs
  struct block_t
  {
    uint64_t height;
    std::vector<uint64_t> fees;

    BEGIN_SERIALIZE()
      FIELD(height)
      FIELD(fees)
    END_SERIALIZE()
  };

  tools::median_db_cache<uint64_t, uint64_t>& m_tx_fee_median;
  naive_median<uint64_t, uint64_t>& m_naive_median;

  std::vector<block_t> m_db_blocks;
  epee::dummy_critical_section m_read_lock;

  BEGIN_SERIALIZE_OBJECT()
    FIELD(m_db_blocks)
  END_SERIALIZE()
};





TEST(db_accessor_tests, median_db_cache_test)
{
  crypto::random_prng_initialize_with_seed_no_lock(0); // make this test deterministic (the same crypto::rand() sequence)

  epee::shared_recursive_mutex m_rw_lock;
  tools::db::basic_db_accessor m_db(std::shared_ptr<tools::db::i_db_backend>(new tools::db::lmdb_db_backend), m_rw_lock);
  tools::median_db_cache<uint64_t, uint64_t> m_tx_fee_median(m_db);
  const std::string folder_name = "./TEST_median_db_cache";
  const std::string naive_median_serialization_filename = folder_name + "/naive_median";
  tools::create_directories_if_necessary(folder_name);
  uint64_t cache_size = CACHE_SIZE;
  ASSERT_TRUE(m_db.open(folder_name, cache_size));
  ASSERT_TRUE(m_tx_fee_median.init("median_fee"));
  auto db_tx_ptr_ = m_db.begin_transaction_obj();

  naive_median<uint64_t, uint64_t> m_naive_median;

  bcs_stub_t bcs_stub(m_tx_fee_median, m_naive_median);
  bool bcs_loaded = bcs_stub.load_from_file(naive_median_serialization_filename);

  if (!bcs_loaded || bcs_stub.m_tx_fee_median.get_median() == 0)
  {
    // reject saved caches, init from scratch
    size_t initial_blocks_count = ALIAS_COAST_RECENT_PERIOD + 20;
    for (size_t i = 0; i < initial_blocks_count; ++i)
    {
      bcs_stub.m_db_blocks.push_back(bcs_stub_t::block_t());
      bcs_stub_t::block_t &b = bcs_stub.m_db_blocks.back();
      b.height = i;
      b.fees.resize(random_in_range(0, 3));
      for (size_t j = 0; j < b.fees.size(); ++j)
        b.fees[j] = random_in_range(0, COIN);
    }
    bcs_stub.init_tx_fee_median(); // clear tx_fee_median and initialize by reading the 'blockchain'
  }

  bcs_stub.init_naive_median(); // naive median doesn't use db cache, so it always init from the 'blockchain'
  bcs_stub.update_tx_fee_median();
  bcs_stub.update_naive_fee_median();

  ASSERT_EQ(m_naive_median.get_median(), m_tx_fee_median.get_median());

  m_naive_median.print_to_file(folder_name + "/naive_median_1.txt");

  //epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&] { m_db.commit_transaction(); bcs_stub.save_to_file(naive_median_serialization_filename); });

  db_tx_ptr_->commit_transaction();
  bcs_stub.save_to_file(naive_median_serialization_filename);
  auto db_tx_ptr = m_db.begin_transaction_obj();
  
  size_t forward_steps_max_random_boundary = 20, rewind_steps_max_random_boundary = 7;
  size_t steps = 1000;
  for (size_t step = 0; step < steps; ++step)
  {
    LOG_PRINT_L0("Step " << step);

    if (step % 50 == 0) // once a bunch of steps make a dicision upon blockchain trend - insreasing or descreasing. This should result in a kind of oscillation
    {
      uint64_t normal_window = ALIAS_COAST_PERIOD;
      uint64_t recent_window = ALIAS_COAST_RECENT_PERIOD;
      if (bcs_stub.m_db_blocks.size() < normal_window)
        forward_steps_max_random_boundary = 20, rewind_steps_max_random_boundary = 7;
      if (bcs_stub.m_db_blocks.size() > recent_window)
        forward_steps_max_random_boundary = 7, rewind_steps_max_random_boundary = 20;
      // if blocks count lies between window's size -- keep the same
    }

    size_t rewind_steps = random_in_range(1, rewind_steps_max_random_boundary);
    for (size_t i = 0; i < rewind_steps; ++i)
    {
      auto& b = bcs_stub.m_db_blocks.back();
      if (b.fees.size() > 0)
      {
        for (size_t j = b.fees.size() - 1; j < b.fees.size(); --j)
        {
          m_naive_median.pop_item(b.fees[j]);
          m_tx_fee_median.pop_item();
        }
      }
      bcs_stub.m_db_blocks.pop_back();
      bcs_stub.update_tx_fee_median();
      bcs_stub.update_naive_fee_median();

      ASSERT_EQ(m_naive_median.get_median(), m_tx_fee_median.get_median());
    }

    uint64_t top_height = bcs_stub.m_db_blocks.back().height;
    size_t forward_steps = random_in_range(1, forward_steps_max_random_boundary);
    for (size_t h = top_height + 1; h < top_height + 1 + forward_steps; ++h)
    {
      bcs_stub.m_db_blocks.push_back(bcs_stub_t::block_t());
      bcs_stub_t::block_t &b = bcs_stub.m_db_blocks.back();
      b.height = h;
      b.fees.resize(random_in_range(0, 10));
      for (size_t j = 0; j < b.fees.size(); ++j)
      {
        uint64_t fee = random_in_range(0, COIN);
        b.fees[j] = fee;
        m_tx_fee_median.push_item(fee, h);
        m_naive_median.push_item(fee, h);
      }

      bcs_stub.update_tx_fee_median();
      bcs_stub.update_naive_fee_median();

      ASSERT_EQ(m_naive_median.get_median(), m_tx_fee_median.get_median());
    }

  }

  db_tx_ptr->commit_transaction(); // save initial data for tx fee median cache to db
  bcs_stub.save_to_file(naive_median_serialization_filename);

  m_naive_median.print_to_file(folder_name + "/naive_median_2.txt");
}


TEST(db_accessor_tests, dtor_without_init)
{
  epee::shared_recursive_mutex m_rw_lock;

  {
    tools::db::basic_db_accessor m_db(nullptr, m_rw_lock);
    ASSERT_FALSE(m_db.is_open());
  }

  // make sure dtor called successfully with no exceptions

  {
    tools::db::basic_db_accessor m_db(nullptr, m_rw_lock);
    m_db.close();
    ASSERT_FALSE(m_db.is_open());
  }

}


TEST(db_accessor_tests, db_chunked_container_test)
{
  epee::shared_recursive_mutex m_rw_lock;
  tools::db::basic_db_accessor m_db(std::shared_ptr<tools::db::i_db_backend>(new tools::db::lmdb_db_backend), m_rw_lock);
  tools::db::chunked_key_to_array_accessor<crypto::public_key, crypto::hash, false, 10> m_container(m_db);

  const std::string folder_name = "./TEST_db_chunked_container";
  boost::system::error_code ec;
  boost::filesystem::remove_all(folder_name, ec);
  ASSERT_FALSE(static_cast<bool>(ec));
  tools::create_directories_if_necessary(folder_name);

  uint64_t cache_size = CACHE_SIZE;
  ASSERT_TRUE(m_db.open(folder_name, cache_size));
  ASSERT_TRUE(m_container.init("zzzz"));

  // Begin write transaction to keep all operations atomic and consistent.
  auto db_tx_ptr = m_db.begin_transaction_obj();
  ASSERT_TRUE(db_tx_ptr.get());

  // Helpers that create deterministic POD values for keys and hashes.
  auto make_public_key = [](uint8_t seed)
  {
    crypto::public_key key = AUTO_VAL_INIT(key);
    for (size_t i = 0; i < sizeof(key.data); ++i)
      key.data[i] = static_cast<char>(seed + i);
    return key;
  };

  auto make_hash = [](uint8_t seed)
  {
    crypto::hash h = AUTO_VAL_INIT(h);
    for (size_t i = 0; i < sizeof(h.data); ++i)
      h.data[i] = static_cast<char>(seed + i);
    return h;
  };

  const auto key_a = make_public_key(1);
  const auto key_b = make_public_key(2);

  // Verify empty container state: size is zero and subitem access throws.
  ASSERT_EQ(m_container.get_item_size(key_a), 0u);

  const bool prev = epee::debug::get_set_enable_assert();
  epee::debug::get_set_enable_assert(true, false);
  {
    bool caught = false;
    try
    {
      m_container.get_subitem(key_a, 0);
    }
    catch (const std::out_of_range& /*oor*/)
    {
      caught = true;
    }
    catch (...)
    {
      ASSERT_TRUE(false);
    }
    ASSERT_TRUE(caught);
  }
  epee::debug::get_set_enable_assert(true, prev);



  // Fill key_a with 25 items to span multiple chunks (chunk_size = 10).
  std::vector<crypto::hash> values;
  for (uint8_t i = 0; i < 25; ++i)
  {
    auto value = make_hash(static_cast<uint8_t>(i + 1));
    values.push_back(value);
    m_container.push_back_item(key_a, value);
  }

  // Fill key_b with a small number of items to validate multiple keys.
  std::vector<crypto::hash> key_b_values;
  for (uint8_t i = 0; i < 3; ++i)
  {
    auto value = make_hash(static_cast<uint8_t>(100 + i));
    key_b_values.push_back(value);
    m_container.push_back_item(key_b, value);
  }

  // Validate item counters per key.
  ASSERT_EQ(m_container.get_item_size(key_a), static_cast<uint64_t>(values.size()));
  ASSERT_EQ(m_container.get_item_size(key_b), static_cast<uint64_t>(key_b_values.size()));

  // Validate get_subitem for first item, chunk boundary end, and next chunk start.
  auto first_ptr = m_container.get_subitem(key_a, 0);
  ASSERT_TRUE(first_ptr.get());
  EXPECT_EQ(*first_ptr, values[0]);

  auto last_in_first_chunk_ptr = m_container.get_subitem(key_a, 9);
  ASSERT_TRUE(last_in_first_chunk_ptr.get());
  EXPECT_EQ(*last_in_first_chunk_ptr, values[9]);

  auto first_in_second_chunk_ptr = m_container.get_subitem(key_a, 10);
  ASSERT_TRUE(first_in_second_chunk_ptr.get());
  EXPECT_EQ(*first_in_second_chunk_ptr, values[10]);

  // Validate get_subitem for the last element in the final (partial) chunk.
  auto last_ptr = m_container.get_subitem(key_a, static_cast<uint64_t>(values.size() - 1));
  ASSERT_TRUE(last_ptr.get());
  EXPECT_EQ(*last_ptr, values.back());

  // Validate get_subitems across a chunk boundary (offset 8, count 5 spans chunk 0->1).
  std::vector<crypto::hash> subitems;
  ASSERT_TRUE(m_container.get_subitems(key_a, 8, 5, subitems));
  ASSERT_EQ(subitems.size(), 5u);
  for (size_t i = 0; i < subitems.size(); ++i)
    EXPECT_EQ(subitems[i], values[8 + i]);

  // Validate get_subitems with count = 0 returns empty without throwing.
  subitems.clear();
  ASSERT_TRUE(m_container.get_subitems(key_a, 0, 0, subitems));
  ASSERT_TRUE(subitems.empty());

  // Validate out-of-range checks for get_subitems.
  try
  {
    m_container.get_subitems(key_a, static_cast<uint64_t>(values.size() - 1), 2, subitems);
    ASSERT_TRUE(false);
  }
  catch (const std::out_of_range& /*oor*/)
  {
    // Expected exception, do nothing.
  }
  catch (...)
  {
    ASSERT_TRUE(false);
  }
  
  try 
  {
    m_container.get_subitems(key_a, static_cast<uint64_t>(values.size()), 1, subitems);
    ASSERT_TRUE(false);
  }
  catch (const std::out_of_range& /*oor*/)
  {
    // Expected exception, do nothing.
  }
  catch (...)
  {
    ASSERT_TRUE(false);
  }

  // Validate get_chunk for a full chunk (index 0) and its contents.
  auto chunk0 = m_container.get_chunk(key_a, 0);
  ASSERT_TRUE(static_cast<bool>(chunk0));
  ASSERT_EQ(chunk0->size(), 10u);
  EXPECT_EQ(chunk0->at(0), values[0]);
  EXPECT_EQ(chunk0->at(9), values[9]);

  // Validate set_chunk by modifying a value in the last (partial) chunk.
  auto chunk2 = *m_container.get_chunk(key_a, 2);
  ASSERT_EQ(chunk2.size(), 5u);
  crypto::hash replacement = make_hash(200);
  chunk2[2] = replacement;
  m_container.set_chunk(key_a, 2, chunk2);
  values[22] = replacement;

  // Confirm the replacement is visible via get_subitem.
  auto replaced_ptr = m_container.get_subitem(key_a, 22);
  ASSERT_TRUE(replaced_ptr.get());
  EXPECT_EQ(*replaced_ptr, replacement);

  // Validate enumerate_subitems returns all values for key_a (order not guaranteed).
//   std::vector<crypto::hash> enumerated;
//   m_container.enumerate_subitems([&](uint64_t /*i*/, const crypto::public_key& key, uint64_t /*chunk_index*/, const crypto::hash& value)
//   {
//     if (key == key_a)
//       enumerated.push_back(value);
//     return true;
//   });
// 
//   ASSERT_EQ(enumerated.size(), values.size());
//   auto sorted_expected = values;
//   std::sort(enumerated.begin(), enumerated.end());
//   std::sort(sorted_expected.begin(), sorted_expected.end());
//   EXPECT_EQ(enumerated, sorted_expected);

  // Validate pop_back_item across chunk boundary (removing 5 items drops chunk 2).
  for (size_t i = 0; i < 5; ++i)
  {
    m_container.pop_back_item(key_a);
    values.pop_back();
  }

  ASSERT_EQ(m_container.get_item_size(key_a), static_cast<uint64_t>(values.size()));
  auto new_last_ptr = m_container.get_subitem(key_a, static_cast<uint64_t>(values.size() - 1));
  ASSERT_TRUE(new_last_ptr.get());
  EXPECT_EQ(*new_last_ptr, values.back());
  epee::debug::get_set_enable_assert(true, false);
  {
    bool caught = false;
    try
    {
      m_container.get_chunk(key_a, 2);
    }
    catch (const std::out_of_range& /*oor*/)
    {
      caught = true;
    }
    catch (...)
    {
      ASSERT_TRUE(false);
    }
    ASSERT_TRUE(caught);
  }
  

  // Validate erase_items clears all items for key_a and resets its counter.
  m_container.erase_items(key_a);
  ASSERT_EQ(m_container.get_item_size(key_a), 0u);
  {
    bool caught = false;
    try
    {
      m_container.get_subitem(key_a, 0);
    }
    catch (const std::out_of_range& /*oor*/)
    {
      caught = true;
    }
    catch (...)
    {
      ASSERT_TRUE(false);
    }
    ASSERT_TRUE(caught);
  }
  epee::debug::get_set_enable_assert(true, prev);
  // Validate key_b is independent and its data remains intact.
  auto key_b_ptr = m_container.get_subitem(key_b, 0);
  ASSERT_TRUE(key_b_ptr.get());
  EXPECT_EQ(*key_b_ptr, key_b_values[0]);
  ASSERT_EQ(m_container.get_item_size(key_b), static_cast<uint64_t>(key_b_values.size()));

  // Commit the transaction to persist all changes.
  db_tx_ptr->commit_transaction();
}