// Copyright (c) 2014-2026 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <algorithm>
#include <sstream>
#include <utility>
#include <vector>

#include "include_base_utils.h"

#include "blockchain_storage.h"
#include "basic_pow_helpers.h"
#include "currency_core/currency_config.h"
#include "currency_format_utils.h"
#include "crypto/crypto.h"
#include "profile_tools.h"

#undef LOG_DEFAULT_CHANNEL
#define LOG_DEFAULT_CHANNEL "core"
ENABLE_CHANNEL_BY_DEFAULT("core");

namespace currency
{
namespace
{
namespace db_perf
{
  constexpr uint64_t random_reads_per_container = 512;
  constexpr uint64_t output_random_reads = 1024;
  constexpr uint64_t keys_sample_limit = 2048;

  constexpr uint64_t max_measurement_time_ms = 1000;
  constexpr uint64_t warning_threshold_reads_per_second = 3000; // 2560 target DB reads / 3000 reads/s it is ~0.85 sec
  constexpr uint64_t pow_warning_threshold_us = 5000; // one pow longhash should stay under 5 ms

  struct result_entry
  {
    std::string name;
    uint64_t reads;
    uint64_t elapsed_us;

    uint64_t reads_per_second() const
    {
      return reads * 1000000 / std::max<uint64_t>(elapsed_us, 1);
    }
  };

  struct db_result
  {
    std::vector<result_entry> items;
    uint64_t total_reads = 0;
    uint64_t total_elapsed_us = 0;
    uint64_t measurement_start_ms = 0;
    uint64_t measurement_finish_ms = 0;
    bool measurement_time_exceeded = false;
    volatile uint64_t read_result_accumulator = 0;

    void start_measurement()
    {
      measurement_start_ms = epee::misc_utils::get_tick_count();
      measurement_finish_ms = measurement_start_ms;
      measurement_time_exceeded = false;
    }

    void finish_measurement()
    {
      if (measurement_start_ms)
        measurement_finish_ms = epee::misc_utils::get_tick_count();
    }

    uint64_t measurement_wall_time_ms() const
    {
      return measurement_finish_ms >= measurement_start_ms ? measurement_finish_ms - measurement_start_ms : 0;
    }

    bool is_time_limit_reached()
    {
      if (!measurement_start_ms)
        start_measurement();

      if (epee::misc_utils::get_tick_count() - measurement_start_ms >= max_measurement_time_ms)
        measurement_time_exceeded = true;

      return measurement_time_exceeded;
    }

    void consume(uint64_t value)
    {
      read_result_accumulator += value;
    }

    void add_result(const char* name, uint64_t reads, uint64_t elapsed_us)
    {
      items.push_back(result_entry{ name, reads, elapsed_us });
      total_reads += reads;
      total_elapsed_us += elapsed_us;
    }

    uint64_t reads_per_second() const
    {
      return total_reads ? total_reads * 1000000 / std::max<uint64_t>(total_elapsed_us, 1) : 0;
    }
  };

  struct pow_result
  {
    bool measured = false;
    crypto::hash hash = currency::null_hash;
    uint64_t context_build_time_ms = 0;
    uint64_t calculation_time_us = 0;
  };

  uint64_t random_index(uint64_t size)
  {
    return size ? crypto::rand<uint64_t>() % size : 0;
  }

  uint64_t reads_count(uint64_t available_items, uint64_t max_reads)
  {
    return std::min(available_items, max_reads);
  }

  std::string details_to_string(const std::vector<result_entry>& items)
  {
    std::stringstream ss;
    for (size_t i = 0; i != items.size(); ++i)
    {
      if (i)
        ss << ", ";

      ss << items[i].name << ": " << items[i].reads_per_second() << " reads/s"
         << " (" << items[i].reads << " reads, " << items[i].elapsed_us / 1000 << " ms)";
    }
    return ss.str();
  }

  template<typename blocks_container_t>
  void measure_blocks(const blocks_container_t& db_blocks, db_result& result)
  {
    if (result.is_time_limit_reached())
      return;

    const uint64_t blocks_count = db_blocks.size();
    if (!blocks_count)
      return;

    const uint64_t count = reads_count(blocks_count, random_reads_per_container);
    db_blocks.clear_cache();

    TIME_MEASURE_START(blocks_read_time);
    uint64_t reads = 0;
    for (uint64_t i = 0; i != count && !result.is_time_limit_reached(); ++i)
    {
      auto block_ptr = db_blocks[random_index(blocks_count)];
      if (block_ptr)
        result.consume(block_ptr->height);
      ++reads;
    }
    TIME_MEASURE_FINISH(blocks_read_time);

    if (reads)
      result.add_result("blocks", reads, blocks_read_time);
  }

  template<typename blocks_container_t, typename blocks_index_t>
  void measure_blocks_index(const blocks_container_t& db_blocks, const blocks_index_t& db_blocks_index, db_result& result)
  {
    if (result.is_time_limit_reached())
      return;

    const uint64_t blocks_count = db_blocks.size();
    if (!blocks_count)
      return;

    const uint64_t count = reads_count(blocks_count, random_reads_per_container);
    std::vector<crypto::hash> block_ids;
    block_ids.reserve(static_cast<size_t>(count));
    for (uint64_t i = 0; i != count && !result.is_time_limit_reached(); ++i)
    {
      auto block_ptr = db_blocks[random_index(blocks_count)];
      if (block_ptr)
        block_ids.push_back(get_block_hash(block_ptr->bl));
    }

    if (block_ids.empty() || result.is_time_limit_reached())
      return;

    db_blocks_index.clear_cache();

    TIME_MEASURE_START(blocks_index_read_time);
    uint64_t reads = 0;
    for (const auto& block_id : block_ids)
    {
      if (result.is_time_limit_reached())
        break;

      auto block_height_ptr = db_blocks_index.get(block_id);
      if (block_height_ptr)
        result.consume(*block_height_ptr);
      ++reads;
    }
    TIME_MEASURE_FINISH(blocks_index_read_time);

    if (reads)
      result.add_result("blocks_index", reads, blocks_index_read_time);
  }

  template<typename outputs_container_t>
  void measure_outputs(const outputs_container_t& db_outputs, db_result& result)
  {
    if (result.is_time_limit_reached())
      return;

    static const uint64_t output_amounts[] =
    {
      0,
      COIN / 1000,
      COIN / 100,
      COIN / 10,
      COIN,
      COIN * 10,
      COIN * 100,
      COIN * 1000,
      COIN * 10000,
      COIN * 100000,
      COIN * 1000000
    };

    std::vector<std::pair<uint64_t, uint64_t>> output_buckets;
    for (size_t i = 0; i != sizeof(output_amounts) / sizeof(output_amounts[0]); ++i)
    {
      const uint64_t outputs_count = db_outputs.get_item_size(output_amounts[i]);
      if (outputs_count)
        output_buckets.push_back(std::make_pair(output_amounts[i], outputs_count));
    }

    if (output_buckets.empty() || result.is_time_limit_reached())
      return;

    TIME_MEASURE_START(outputs_read_time);
    uint64_t reads = 0;
    for (uint64_t i = 0; i != output_random_reads && !result.is_time_limit_reached(); ++i)
    {
      const auto& bucket = output_buckets[random_index(static_cast<uint64_t>(output_buckets.size()))];
      auto output_ptr = db_outputs.get_subitem(bucket.first, random_index(bucket.second));
      if (output_ptr)
        result.consume(output_ptr->out_no);
      ++reads;
    }
    TIME_MEASURE_FINISH(outputs_read_time);

    if (reads)
      result.add_result("outputs", reads, outputs_read_time);
  }

  template<typename spent_keys_container_t>
  void measure_spent_keys(const spent_keys_container_t& db_spent_keys, db_result& result)
  {
    if (result.is_time_limit_reached())
      return;

    std::vector<crypto::key_image> key_images;
    key_images.reserve(static_cast<size_t>(keys_sample_limit));
    db_spent_keys.enumerate_keys([&key_images, &result](uint64_t /*i*/, crypto::key_image& key_image) -> bool
    {
      key_images.push_back(key_image);
      return key_images.size() < keys_sample_limit && !result.is_time_limit_reached();
    });

    if (key_images.empty() || result.is_time_limit_reached())
      return;

    const uint64_t count = reads_count(static_cast<uint64_t>(key_images.size()), random_reads_per_container);
    db_spent_keys.clear_cache();

    TIME_MEASURE_START(spent_keys_read_time);
    uint64_t reads = 0;
    for (uint64_t i = 0; i != count && !result.is_time_limit_reached(); ++i)
    {
      auto height_ptr = db_spent_keys.get(key_images[random_index(static_cast<uint64_t>(key_images.size()))]);
      if (height_ptr)
        result.consume(*height_ptr);
      ++reads;
    }
    TIME_MEASURE_FINISH(spent_keys_read_time);

    if (reads)
      result.add_result("spent_keys", reads, spent_keys_read_time);
  }

  pow_result measure_pow_longhash(const blockchain_storage& storage)
  {
    pow_result result;
    auto pow_block_ptr = storage.get_last_block_of_type(false);
    if (!pow_block_ptr)
      return result;

    result.measured = true;
    result.calculation_time_us = measure_longhash_calculation_time_us(pow_block_ptr->bl, result.hash, &result.context_build_time_ms);
    return result;
  }

  std::string make_log_message(const db_result& db, const pow_result& pow)
  {
    std::stringstream ss;
    ss << "Blockchain storage performance: DB indicator " << db.reads_per_second() << " random reads/s"
      << " (" << db.total_reads << " reads, " << db.total_elapsed_us / 1000 << " ms measured, "
      << db.measurement_wall_time_ms() << " ms total";

    if (db.measurement_time_exceeded)
      ss << ", stopped by " << max_measurement_time_ms << " ms limit";
    ss << ")";

    if (!db.items.empty())
      ss << ENDL << "  DB details: " << details_to_string(db.items);
    if (pow.measured)
      ss << ENDL << "  CPU PoW longhash: " << pow.calculation_time_us << " us"
        << " (epoch context preparation: " << pow.context_build_time_ms << " ms, hash: " << pow.hash << ")";

    return ss.str();
  }

  bool is_slow(const db_result& db, const pow_result& pow)
  {
    return db.measurement_time_exceeded
      || (db.total_reads != 0 && db.reads_per_second() < warning_threshold_reads_per_second)
      || (pow.measured && pow.calculation_time_us > pow_warning_threshold_us);
  }

  void log_result(const db_result& db, const pow_result& pow)
  {
    const std::string message = make_log_message(db, pow);
    if (is_slow(db, pow))
    {
      LOG_PRINT_RED_L0("WARNING: " << message << ENDL
        << "Overall performance might be degraded. Expected at least " << warning_threshold_reads_per_second
        << " DB random reads/s and PoW longhash under " << pow_warning_threshold_us << " us.");
    }
    else
    {
      LOG_PRINT_GREEN(message, LOG_LEVEL_0);
    }
  }
}
}

//------------------------------------------------------------------
void blockchain_storage::measure_db_performance() const
{
  TRY_ENTRY();

  db_perf::db_result db;
  {
    CRITICAL_REGION_LOCAL(m_read_lock);
    db.start_measurement();
    db_perf::measure_blocks(m_db_blocks, db);
    db_perf::measure_blocks_index(m_db_blocks, m_db_blocks_index, db);
    db_perf::measure_outputs(m_db_outputs, db);
    db_perf::measure_spent_keys(m_db_spent_keys, db);
    db.finish_measurement();
  }

  db_perf::log_result(db, db_perf::measure_pow_longhash(*this));

  CATCH_ENTRY_L0("blockchain_storage::measure_db_performance", void());
}

}

#undef LOG_DEFAULT_CHANNEL
#define LOG_DEFAULT_CHANNEL NULL