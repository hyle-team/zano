#include <random>
#include <array>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <chrono>

#include "epee/include/include_base_utils.h"
#include "include_base_utils.h"
#include "common/db_backend_lmdb.h"
#include "common/db_abstract_accessor.h"
#include "readwrite_lock.h"

// #undef  LOG_DEFAULT_CHANNEL
// #define LOG_DEFAULT_CHANNEL "db-bulk-writer"

using namespace tools::db;

enum class Mode { kv, array };

struct first_item_cb : public i_db_callback
{
  std::string k, v;
  bool got = false;
  bool on_enum_item(uint64_t /*i*/, const void* pkey, uint64_t ks, const void* pval, uint64_t vs) override
  {
    k.assign(static_cast<const char*>(pkey), static_cast<size_t>(ks));
    v.assign(static_cast<const char*>(pval), static_cast<size_t>(vs));
    got = true;
    return false; // stop
  }
};

static void gen_key_bytes(std::mt19937_64& gen, uint64_t counter, std::string& out, size_t size)
{
  out.resize(size);
  for (size_t i = 0; i < size; i += 8)
  {
    uint64_t r = gen() ^ (0x9E3779B97F4A7C15ull * (counter + i));
    std::memcpy(&out[i], &r, std::min<size_t>(8, size - i));
  }
}

class bulk_writer
{
public:
  bulk_writer(basic_db_accessor& db, i_db_backend& _be, const std::string& table, container_handle h, Mode mode)
  : _db(db), _be_(_be), _table(table), _h(h), _mode(mode) {}

  bool init_sample()
  {
    if (_mode == Mode::kv)
    {
      if (_be_.size(_h) == 0)
      {
        LOG_ERROR("[" << _table << "] empty");
        return false;
      }
      first_item_cb cb;
      if (!_be_.enumerate(_h, &cb) || !cb.got)
      {
        LOG_ERROR("[" << _table << "] enumerate failed");
        return false;
      }
      _sample_key = cb.k;
      _sample_val = cb.v;
      _key_size   = _sample_key.size();
      _bytes_per_put = _key_size + _sample_val.size();
      _entries = _be_.size(_h);
      return true;
    }
    else
    { // array
      _entries = _be_.size(_h);
      if (_entries == 0)
      {
        LOG_ERROR("[" << _table << "] empty");
        return false;
      }
      uint64_t last_key = _entries - 1;
      if (!_be_.get(_h, reinterpret_cast<const char*>(&last_key), sizeof(last_key), _sample_val))
      {
        LOG_ERROR("[" << _table << "] get(last) failed");
        return false;
      }
      _key_size = sizeof(uint64_t);
      _bytes_per_put = _key_size + _sample_val.size();
      return true;
    }
  }

  void print_initial_estimate() const
  {
    const long double MB_DEC = 1'000'000.0L;
    long double approx_initial_bytes = static_cast<long double>(_entries) * _bytes_per_put;
    LOG_PRINT_L0("[" << _table << "] entries=" << _entries
                 << ", sample_value=" << _sample_val.size() << " B"
                 << ", approx bytes/put=" << _bytes_per_put << " B");
    LOG_PRINT_L0("[" << _table << "] Approx initial size: "
                 << (approx_initial_bytes / MB_DEC) << " MB");
  }

  bool write(uint64_t target_mb, uint64_t step_mb)
  {
    using clock = std::chrono::steady_clock;

    const uint64_t MB_DEC = 1'000'000;
    const uint64_t target_bytes = target_mb * MB_DEC;
    const uint64_t step_bytes   = step_mb   * MB_DEC;

    if (!_db.begin_transaction(false))
    {
      LOG_ERROR("begin_transaction failed");
      return false;
    }
    bool tx_open = true;
    bool ok = true;

    std::random_device rd;
    std::mt19937_64 gen(rd());

    uint64_t written_total = 0;
    uint64_t next_report   = step_bytes;
    uint64_t chunks_done   = 0;
    uint64_t counter       = 0;
    uint64_t array_key     = _entries;

    const auto all_start   = clock::now();
    auto       chunk_start = all_start;

    while (written_total < target_bytes)
    {
      if (_mode == Mode::kv)
      {
        std::string new_key;
        gen_key_bytes(gen, counter++, new_key, _key_size);
        if (!_be_.set(_h, new_key.data(), new_key.size(), _sample_val.data(), _sample_val.size()))
        {
          LOG_ERROR("[" << _table << "] set(new kv) failed");
          ok = false; break;
        }
      }
      else
      { // array
        if (!_be_.set(_h, reinterpret_cast<const char*>(&array_key), sizeof(array_key),
                     _sample_val.data(), _sample_val.size()))
        {
          LOG_ERROR("[" << _table << "] set(new array) failed");
          ok = false; break;
        }
        ++array_key;
      }

      written_total += _bytes_per_put;

      if (written_total >= next_report)
      {
        const auto t_fill_done = clock::now();

        if (tx_open)
        {
          _db.commit_transaction();
          tx_open = false;
        }
        const auto t_after_commit = clock::now();

        const double fill_sec   = std::chrono::duration<double>(t_fill_done     - chunk_start).count();
        const double commit_sec = std::chrono::duration<double>(t_after_commit  - t_fill_done).count();
        const double chunk_sec  = fill_sec + commit_sec;
        const double total_sec  = std::chrono::duration<double>(t_after_commit  - all_start).count();

        ++chunks_done;

        const long double approx_now_mb =
            (static_cast<long double>(_entries) * _bytes_per_put
          + static_cast<long double>(chunks_done) * step_mb * MB_DEC) / 1'000'000.0L;

        const double chunk_mb      = static_cast<double>(step_mb);
        const double fill_mb_s     = chunk_mb / (fill_sec   > 0 ? fill_sec   : 1e-9);
        const double commit_mb_s   = chunk_mb / (commit_sec > 0 ? commit_sec : 1e-9);
        const double chunk_mb_s    = chunk_mb / (chunk_sec  > 0 ? chunk_sec  : 1e-9);
        const double total_written = static_cast<double>(chunks_done * step_mb);
        const double total_mb_s    = total_written / (total_sec > 0 ? total_sec : 1e-9);

        LOG_PRINT_L0(
          "Writed " << step_mb << "MB in " << _table
          << " | fill: "   << fill_sec   << " s (" << fill_mb_s   << " MB/s)"
          << ", commit: "  << commit_sec << " s (" << commit_mb_s << " MB/s)"
          << ", total: "   << chunk_sec  << " s (" << chunk_mb_s  << " MB/s). "
          << "Total written: " << total_written << " MB, total time: "
          << total_sec << " s (" << total_mb_s << " MB/s avg). "
          << "Est. table size now: " << approx_now_mb << " MB."
        );

        if (written_total < target_bytes)
        {
          if (!_db.begin_transaction(false))
          {
            LOG_ERROR("begin_transaction failed (after commit)");
            ok = false; break;
          }
          tx_open = true;
          chunk_start = clock::now();
        }

        next_report += step_bytes;
      }
    }

    if (tx_open)
    {
      if (ok) _db.commit_transaction();
      else    _db.abort_transaction();
    }
    return ok;
  }

private:
  basic_db_accessor& _db;
  i_db_backend&      _be_;
  std::string        _table;
  container_handle   _h;
  Mode               _mode;
  uint64_t           _entries = 0;
  size_t             _key_size = 0;
  uint64_t           _bytes_per_put = 0;
  std::string        _sample_key;
  std::string        _sample_val;
};

static Mode parse_mode(const char* s)
{
  if (!s)
    return Mode::kv;
  std::string v(s);
  std::transform(v.begin(), v.end(), v.begin(), ::tolower);
  if (v == "array")
    return Mode::array;
  return Mode::kv;
}

int main(int argc, char** argv)
{
  epee::string_tools::set_module_name_and_folder(argv[0]);
  epee::log_space::get_set_log_detalisation_level(true, LOG_LEVEL_0);
  epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, nullptr, nullptr, LOG_LEVEL_0);
  epee::log_space::log_singletone::set_thread_log_prefix("[db-bulk-writer] ");

  if (argc < 3)
  {
    LOG_PRINT_L0(
      std::string("Usage:\n  ") + argv[0] +
      " <env_path> <table_name> [--mode=kv|array] [--target-mb=N] [--step-mb=M]\n\n"
      "Examples:\n  " + argv[0] +
      " ~/.Zano/blockchain_lmdb_v3 transactions --mode=kv --target-mb=1000 --step-mb=100\n  " +
      argv[0] +
      " ~/.Zano/blockchain_lmdb_v3 blocks       --mode=array --target-mb=200  --step-mb=50");
    return 0;
  }


  std::string env_path   = argv[1];
  std::string table_name = argv[2];
  Mode mode = Mode::kv;
  uint64_t target_mb = 1000;
  uint64_t step_mb   = 100;

  for (int i = 3; i < argc; ++i)
  {
    std::string a = argv[i];
    auto eat = [&](const char* pfx)->const char*
    {
      size_t L = std::strlen(pfx);
      if (a.size() > L && a.compare(0,L,pfx) == 0)
        return a.c_str()+L;
      return nullptr;
    };
    if (const char* v = eat("--mode="))
      mode = parse_mode(v);
    else if (const char* v = eat("--target-mb="))
      target_mb = std::strtoull(v, nullptr, 10);
    else if (const char* v = eat("--step-mb="))
      step_mb = std::strtoull(v, nullptr, 10);
  }

  epee::shared_recursive_mutex rwlock;
  std::shared_ptr<i_db_backend> be(new lmdb_db_backend());
  basic_db_accessor db(be, rwlock);

  uint64_t max_db_size = uint64_t(1UL * 512UL) * 1024UL * 1024UL * 1024UL;
  if (!db.open(env_path, max_db_size))
  {
    LOG_ERROR("Failed to open LMDB env at " << env_path);
    return EXIT_FAILURE;
  }

  container_handle h = 0;
  if (!be->open_container(table_name, h))
  {
    LOG_ERROR("open_container(" << table_name << ") failed");
    return EXIT_FAILURE;
  }

  bulk_writer writer(db, *be, table_name, h, mode);
  if (!writer.init_sample())
    return EXIT_FAILURE;
  writer.print_initial_estimate();

  bool ok = writer.write(target_mb, step_mb);
  db.close();

  if (ok)
  {
    LOG_PRINT_L0("DONE");
    return EXIT_SUCCESS;
  }
  else
  {
    LOG_ERROR("FAILED");
    return EXIT_FAILURE;
  }
}
