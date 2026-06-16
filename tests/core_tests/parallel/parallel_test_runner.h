#pragma once

#include <boost/program_options/variables_map.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/property_tree/ptree.hpp>
#include <filesystem>
#include <string>
#include <vector>
#include <functional>

namespace pt = boost::property_tree;

struct test_job
{
  std::string name;
  std::function<bool()> run;
};

namespace coretests_shm
{
  constexpr uint32_t k_max_fails = 4096;
  constexpr uint32_t k_max_tests = 20000;
  constexpr uint32_t k_max_workers = 256;
  constexpr uint32_t k_name_max = 256;

  constexpr uint16_t k_no_test = 0xFFFF;

  enum test_status : uint8_t
  {
    ts_pending = 0,
    ts_running = 1,
    ts_ok = 2,
    ts_failed = 3,
  };

  struct fail_entry
  {
    uint32_t worker_id;
    char     test_name[k_name_max];
  };

  struct shared_state
  {
    // fail reporting
    std::atomic<uint32_t> write_idx{0};
    fail_entry entries[k_max_fails];

    // work distribution
    std::atomic<uint32_t> next_pos{0};
    std::atomic<uint32_t> tests_total{0};
    std::atomic<bool> stop_all{false};

    uint32_t order[k_max_tests];

    std::atomic<uint8_t> test_status_arr[k_max_tests];
    std::atomic<uint16_t> worker_running_test[k_max_workers];

    void record_fail(uint32_t wid, const char* name)
    {
      const uint32_t pos = write_idx.fetch_add(1, std::memory_order_acq_rel);
      if (pos >= k_max_fails)
        return;

      entries[pos].worker_id = wid;
      std::strncpy(entries[pos].test_name, name ? name : "", k_name_max - 1);
      entries[pos].test_name[k_name_max - 1] = '\0';
    }

    void init()
    {
      write_idx.store(0, std::memory_order_relaxed);
      next_pos.store(0, std::memory_order_relaxed);
      tests_total.store(0, std::memory_order_relaxed);
      stop_all.store(false, std::memory_order_relaxed);

      for (uint32_t i = 0; i < k_max_tests; ++i)
        test_status_arr[i].store(ts_pending, std::memory_order_relaxed);
      for (uint32_t i = 0; i < k_max_workers; ++i)
        worker_running_test[i].store(k_no_test, std::memory_order_relaxed);
    }

    bool try_take_next(uint32_t& out_test_idx)
    {
      const uint32_t total = tests_total.load(std::memory_order_acquire);
      const uint32_t pos = next_pos.fetch_add(1, std::memory_order_acq_rel);
      if (pos >= total)
        return false;

      out_test_idx = order[pos];
      return true;
    }

    void mark_running(uint32_t test_idx, uint32_t worker_id)
    {
      if (test_idx < k_max_tests)
        test_status_arr[test_idx].store(ts_running, std::memory_order_release);
      if (worker_id < k_max_workers && test_idx < k_max_tests)
        worker_running_test[worker_id].store(static_cast<uint16_t>(test_idx), std::memory_order_release);
    }

    void mark_finished(uint32_t test_idx, uint32_t worker_id, bool ok)
    {
      if (worker_id < k_max_workers)
        worker_running_test[worker_id].store(k_no_test, std::memory_order_release);
      if (test_idx < k_max_tests)
        test_status_arr[test_idx].store(ok ? ts_ok : ts_failed, std::memory_order_release);
    }
  };
}

class parallel_test_runner
{
public:
  static constexpr int k_not_parent = -1;

  explicit parallel_test_runner(const boost::program_options::variables_map& vm);

  struct worker_report
  {
    uint32_t worker_id = 0;
    uint32_t processes = 1;

    size_t tests_count = 0;
    size_t unique_tests_count = 0;

    uint64_t total_time_ms = 0;

    std::vector<std::pair<std::string, uint64_t>> tests_running_time;
    std::set<std::string> failed_tests;
    bool skip_all_till_the_end = false;

    int exit_code = 0;
  };

  int run_parent_if_needed(int argc, char* argv[], const std::vector<test_job>& g_test_jobs) const;
  bool write_worker_report(const worker_report& rep) const;
  void log_test_taken_by_this_process(const std::string& test_name) const;

  void dashboard_loop(std::atomic<bool>& stop_flag, const coretests_shm::shared_state* st,
    uint32_t processes, const std::vector<test_job>& jobs) const;

private:
  struct paths
  {
    static constexpr const char* default_run_root       = "chaingen_runs";
    static constexpr const char* worker_dir_prefix      = "w";
  };

  struct files
  {
    static constexpr const char* taken_tests_log        = "taken_tests.log";
    static constexpr const char* worker_report          = "coretests_report.json";
    static constexpr const char* worker_log             = "worker.log";
  };

  struct cli_args
  {
    static constexpr const char* multiprocess_worker_id = "--multiprocess-worker-id";
    static constexpr const char* multiprocess_run       = "--multiprocess-run";
    static constexpr const char* multiprocess_run_root  = "--multiprocess-run-root";
    static constexpr const char* multiprocess_shm_name  = "--multiprocess-shm-name";
    static constexpr const char* data_dir               = "--data-dir";
  };

  const boost::program_options::variables_map& m_vm;
  mutable std::mutex cerr_mx;

  std::filesystem::path get_run_root_path() const;
  std::filesystem::path get_worker_report_path(uint32_t worker_id) const;
  std::filesystem::path get_worker_log_path(uint32_t worker_id) const;
  std::string make_worker_data_dir(const std::filesystem::path& run_root_abs, int worker_id) const;
  std::filesystem::path get_worker_dir_path(uint32_t worker_id) const;
  std::filesystem::path get_taken_tests_log_path_for_this_process() const;
  std::filesystem::path get_workers_report_path() const;

  std::vector<std::string> build_base_args_without_worker_specific(int argc, char* argv[]) const;

  int run_workers_and_wait(int argc, char* argv[], const std::vector<test_job>& g_test_jobs) const;
  int print_aggregated_report_and_return_rc(uint32_t processes, uint64_t wall_time_ms, const coretests_shm::shared_state* shm_state, const std::vector<test_job>& jobs) const;
  void print_worker_failure_reasons(uint32_t processes, const std::vector<int>& worker_exit_codes) const;

  bool fill_shm_work_order(coretests_shm::shared_state* st, const std::vector<test_job>& jobs) const;

  bool write_workers_report_file(uint32_t processes, const std::vector<worker_report>& reps) const;
  bool write_worker_report_file(const worker_report& rep) const;
  bool read_worker_report_file(uint32_t worker_id, worker_report& rep) const;
  void fill_worker_report_header(pt::ptree& root, const worker_report& rep) const;
  void read_worker_report_header(const pt::ptree& root, uint32_t worker_id_fallback, worker_report& rep) const;
};
