#pragma once

#include <boost/program_options/variables_map.hpp>
#include <filesystem>
#include <string>
#include <vector>

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

  int run_parent_if_needed(int argc, char* argv[]) const;
  std::filesystem::path get_worker_dir_path(uint32_t worker_id) const;
  bool write_worker_report(const worker_report& rep) const;

  std::filesystem::path get_taken_tests_log_path_for_this_process() const;
  void log_test_taken_by_this_process(const std::string& test_name) const;

  std::filesystem::path get_workers_report_path() const;
  bool write_workers_report_file(uint32_t processes, const std::vector<worker_report>& reps) const;

private:
  const boost::program_options::variables_map& m_vm;

  std::filesystem::path get_run_root_path() const;
  std::filesystem::path get_worker_report_path(uint32_t worker_id) const;
  std::string make_worker_data_dir(const std::filesystem::path& run_root_abs, int worker_id) const;

  std::vector<std::string> build_base_args_without_worker_specific(int argc, char* argv[]) const;

  int run_workers_and_wait(int argc, char* argv[]) const;

  int print_aggregated_report_and_return_rc(uint32_t processes, uint64_t wall_time_ms) const;
  void print_worker_failure_reasons(uint32_t processes, const std::vector<int>& worker_exit_codes) const;

  static std::string sanitize_filename(std::string s);
  static bool parse_test_name_from_header(const std::string& line, std::string& out_name);
  static std::filesystem::path make_unique_log_path(const std::filesystem::path& dir, const std::string& base_name_no_ext);
  static std::vector<std::filesystem::path> find_logs_for_test(const std::filesystem::path& worker_dir, const std::string& test_name);

  bool write_worker_report_file(const worker_report& rep) const;
  bool read_worker_report_file(uint32_t worker_id, worker_report& rep) const;

  std::filesystem::path get_worker_log_path(uint32_t worker_id) const;
};

struct worker_log_sink
{
  std::mutex mx;
  std::ofstream f;

  void open(const std::filesystem::path& path)
  {
    std::filesystem::create_directories(path.parent_path());
    f.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
  }

  void write_line(const std::string& line)
  {
    std::lock_guard<std::mutex> lk(mx);
    if (!f.is_open())
      return;
    f << line << "\n";
    f.flush();
  }
};

struct test_job
{
  std::string name;
  std::function<bool()> run;
};
