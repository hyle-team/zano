#include "chaingen.h"

#include <chrono>
#include <cctype>
#include <mutex>
#include <thread>
#include <atomic>
#include <boost/property_tree/json_parser.hpp>
#include <boost/process.hpp>
#include <unordered_set>
#include <algorithm>

#if defined(_WIN32) || defined(_WIN64)
  #include <io.h>
  #include <windows.h>
  #ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
    #define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
  #endif
#else
  #include <unistd.h>
  #include <sys/ioctl.h>
#endif

#include "parallel_test_runner.h"
#include "../chaingen_args.h"
#include "../../src/common/command_line.h"
#include "tests_distribution_entry.h"

using namespace chaingen_args;
namespace bp = boost::process;

namespace
{
  constexpr const int UNKNOWN_ERROR_EXIT_CODE = -999;
  constexpr const int MS_PER_SECOND = 1000;
  constexpr const int RESERVE_UNIQUE_TESTS_HINT = 8192;

  constexpr const int DASHBOARD_TICK_MS = 400;
  constexpr const int DASHBOARD_FALLBACK_WIDTH = 120;
  constexpr const uint32_t DASHBOARD_DOTS_CYCLE = 5;

  bool is_stdout_tty()
  {
#if defined(_WIN32) || defined(_WIN64)
    return _isatty(_fileno(stdout)) != 0;
#else
    return ::isatty(::fileno(stdout)) != 0;
#endif
  }

  bool enable_ansi_terminal()
  {
    if (!is_stdout_tty())
      return false;
#if defined(_WIN32) || defined(_WIN64)
    HANDLE h = ::GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE || h == nullptr)
      return false;
    DWORD mode = 0;
    if (!::GetConsoleMode(h, &mode))
      return false;
    if ((mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0)
      return true;
    return ::SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
#else
    return true;
#endif
  }

  int get_terminal_width()
  {
#if defined(_WIN32) || defined(_WIN64)
    CONSOLE_SCREEN_BUFFER_INFO info{};
    HANDLE h = ::GetStdHandle(STD_OUTPUT_HANDLE);
    if (h && ::GetConsoleScreenBufferInfo(h, &info))
    {
      int w = static_cast<int>(info.srWindow.Right - info.srWindow.Left + 1);
      if (w > 0)
        return w;
    }
    return DASHBOARD_FALLBACK_WIDTH;
#else
    struct winsize ws{};
    if (::ioctl(::fileno(stdout), TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
      return static_cast<int>(ws.ws_col);
    return DASHBOARD_FALLBACK_WIDTH;
#endif
  }

  std::string fit_to_width(const std::string& s, int max_width)
  {
    if (max_width <= 4)
      return s.substr(0, static_cast<size_t>(std::max(max_width, 0)));
    if (static_cast<int>(s.size()) <= max_width)
      return s;
    return s.substr(0, static_cast<size_t>(max_width - 3)) + "...";
  }

  std::string dots_for_frame(uint32_t frame)
  {
    const uint32_t n = (frame % DASHBOARD_DOTS_CYCLE) + 1;
    return std::string(n, '.');
  }
}

parallel_test_runner::parallel_test_runner(
  const boost::program_options::variables_map& vm)
  : m_vm(vm)
{}

int parallel_test_runner::run_parent_if_needed(int argc, char* argv[], const std::vector<test_job>& g_test_jobs) const
{
  const int rc = run_workers_and_wait(argc, argv, g_test_jobs);
  return (rc < 0) ? k_not_parent : rc;
}

bool parallel_test_runner::write_worker_report(const worker_report& rep) const
{
  return write_worker_report_file(rep);
}

std::filesystem::path parallel_test_runner::get_run_root_path() const
{
  std::string run_root = command_line::get_arg(m_vm, arg_run_root);
  if (run_root.empty())
    run_root = paths::default_run_root;

  std::filesystem::path run_root_path(run_root);

  if (run_root_path.is_relative())
    run_root_path = std::filesystem::absolute(run_root_path);

  return run_root_path;
}

std::filesystem::path parallel_test_runner::get_worker_dir_path(uint32_t worker_id) const
{
  return get_run_root_path() / (paths::worker_dir_prefix + std::to_string(worker_id));
}

std::filesystem::path parallel_test_runner::get_worker_report_path(uint32_t worker_id) const
{
  return get_worker_dir_path(worker_id) / files::worker_report;
}

std::string parallel_test_runner::make_worker_data_dir(const std::filesystem::path& run_root_abs, int worker_id) const
{
  std::filesystem::path worker_dir = run_root_abs / (std::string(paths::worker_dir_prefix) + std::to_string(worker_id));
  std::filesystem::create_directories(worker_dir);
  return worker_dir.string();
}

// Removes worker-specific arguments from the command line
std::vector<std::string> parallel_test_runner::build_base_args_without_worker_specific(int argc, char* argv[]) const
{
  static const std::set<std::string> worker_specific_args = {
    cli_args::multiprocess_worker_id, cli_args::multiprocess_run, cli_args::multiprocess_run_root,
    cli_args::multiprocess_shm_name, cli_args::data_dir,
  };

  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i)
  {
    std::string current_arg = argv[i];
    bool skip = false;
    for (const auto& wa : worker_specific_args)
    {
      if (current_arg == wa || current_arg.find(wa + "=") == 0)
      {
        if (current_arg == wa && i + 1 < argc) ++i;
        skip = true;
        break;
      }
    }
    if (!skip)
      args.emplace_back(std::move(current_arg));
  }
  return args;
}

void fill_postponed_tests_set(std::set<std::string>& postponed_tests);

int parallel_test_runner::print_aggregated_report_and_return_rc(uint32_t processes, uint64_t wall_time_ms, const coretests_shm::shared_state* shm_state, const std::vector<test_job>& jobs) const
{
  std::vector<worker_report> reps;
  reps.reserve(processes);

  bool any_missing = false;
  int failed_workers = 0;

  for (uint32_t i = 0; i < processes; ++i)
  {
    worker_report r;
    if (!read_worker_report_file(i, r))
    {
      any_missing = true;
      continue;
    }
    reps.push_back(std::move(r));
  }

  size_t total_tests_count = 0;
  size_t total_unique_tests_count = 0;

  std::set<std::string> failed_tests_union;
  std::unordered_set<std::string> executed_tests_union;
  for (const auto& r : reps)
  {
    if (r.exit_code != 0)
      ++failed_workers;

    total_tests_count += r.tests_count;
    total_unique_tests_count += r.unique_tests_count;
    failed_tests_union.insert(r.failed_tests.begin(), r.failed_tests.end());
    for (const auto& it : r.tests_running_time)
      executed_tests_union.insert(it.first);
  }

  std::set<std::string> postponed_tests;
  fill_postponed_tests_set(postponed_tests);

  if (shm_state)
  {
    uint32_t n_fails = shm_state->write_idx.load(std::memory_order_relaxed);
    if (n_fails > coretests_shm::k_max_fails)
      n_fails = coretests_shm::k_max_fails;

    for (uint32_t j = 0; j < n_fails; ++j)
    {
      const coretests_shm::fail_entry& e = shm_state->entries[j];
      if (e.test_name[0] != '\0')
        failed_tests_union.insert(std::string(e.test_name));
      if (e.test_name[0] != '\0')
        executed_tests_union.insert(std::string(e.test_name));
    }
  }

  if (!jobs.empty())
  {
    for (const auto& j : jobs)
    {
      const bool failed = failed_tests_union.count(j.name) != 0;
      const bool executed = executed_tests_union.count(j.name) != 0;

      if (failed)
        std::cout << concolor::magenta << j.name << concolor::normal << '\n';
      else if (executed)
        std::cout << concolor::green << j.name << concolor::normal << '\n';
      else
        std::cout << concolor::yellow << j.name << concolor::normal << '\n';
    }
    std::cout << '\n';
  }

  // 2) Postponed tests list
  if (!postponed_tests.empty())
  {
    std::cout << concolor::yellow << postponed_tests.size() << " POSTPONED TESTS:\n";
    for (const auto& el : postponed_tests)
      std::cout << "  " << el << '\n';
    std::cout << concolor::normal << '\n';
  }

  size_t failed_postponed_tests_count = 0;
  for (const auto& t : failed_tests_union)
    if (postponed_tests.count(t) != 0)
      ++failed_postponed_tests_count;

  const size_t serious_failures_count = failed_tests_union.size() >= failed_postponed_tests_count ? (failed_tests_union.size() - failed_postponed_tests_count) : 0;
  const bool ok = (!any_missing && failed_workers == 0 && serious_failures_count == 0);

  std::cout << (serious_failures_count == 0 ? concolor::green : concolor::magenta);
  std::cout << "\nREPORT:\n";
  std::cout << "  Unique tests run: " << total_unique_tests_count << '\n';
  std::cout << "  Total tests run:  " << total_tests_count << '\n';
  std::cout << "  Failures:         " << serious_failures_count << " (postponed failures: " << failed_postponed_tests_count << ")" << '\n';
  std::cout << "  Postponed:        " << postponed_tests.size() << '\n';
  std::cout << "  Total time:       " << (wall_time_ms / MS_PER_SECOND) << " s. (" << (total_tests_count > 0 ? (wall_time_ms / total_tests_count) : 0) << " ms per test in average)" << '\n';

  if (!failed_tests_union.empty())
  {
    std::cout << "FAILED/POSTPONED TESTS:\n";
    for (const auto& name : failed_tests_union)
    {
      const bool postponed = postponed_tests.count(name) != 0;
      std::cout << "  " << (postponed ? "POSTPONED: " : "FAILED:    ") << name << "\n";
    }
  }

  std::cout << concolor::normal << '\n';

  if (!reps.empty())
    (void)write_workers_report_file(processes, reps);

  if (any_missing || failed_workers != 0)
    return 1;

  return serious_failures_count == 0 ? 0 : 1;
}

void parallel_test_runner::print_worker_failure_reasons(uint32_t processes, const std::vector<int>& worker_exit_codes) const
{
  bool any = false;

  for (uint32_t i = 0; i < processes; ++i)
  {
    const int ec = (i < worker_exit_codes.size() ? worker_exit_codes[i] : UNKNOWN_ERROR_EXIT_CODE);

    worker_report rep;
    const bool report_ok = read_worker_report_file(i, rep);

    if (report_ok && ec == 0 && rep.exit_code == 0)
      continue;

    if (!any)
    {
      std::cout << "\nFAILURE REASONS (workers):\n";
      any = true;
    }

    const std::filesystem::path worker_dir = get_worker_dir_path(i);
    const std::filesystem::path report_path = get_worker_report_path(i);
    const std::filesystem::path log_path = worker_dir / files::worker_log;

    std::cout << "    report: " << (std::filesystem::exists(report_path) ? report_path.string() : ("missing (" + report_path.string() + ")")) << "\n";
    std::cout << "    logs dir: " << worker_dir.string() << "\n";
    std::cout << "    log file: " << (std::filesystem::exists(log_path) ? log_path.string() : ("missing (" + log_path.string() + ")")) << "\n";

    if (!report_ok)
    {
      std::cout << "    failed tests: (cannot read report)\n";
      continue;
    }

    if (rep.failed_tests.empty())
    {
      std::cout << "    failed tests: (none in report)\n";
      continue;
    }

    std::cout << "    failed tests:\n";
    for (const auto& test_name : rep.failed_tests)
      std::cout << "      - " << test_name << "\n";
  }
}

int parallel_test_runner::run_workers_and_wait(int argc, char* argv[], const std::vector<test_job>& g_test_jobs) const
{
  const uint32_t processes = command_line::get_arg(m_vm, arg_processes);
  const int32_t worker_id  = command_line::get_arg(m_vm, arg_worker_id);

  if (worker_id >= 0)
    return -1;

  if (command_line::get_arg(m_vm, command_line::arg_help)) return -1;
  if (command_line::get_arg(m_vm, arg_generate_test_data)) return -1;
  if (command_line::get_arg(m_vm, arg_play_test_data)) return -1;
  if (command_line::get_arg(m_vm, arg_generate_and_play_test_data)) return -1;
  if (command_line::get_arg(m_vm, arg_test_transactions)) return -1;

  const auto wall_start = std::chrono::steady_clock::now();
  const std::string run_root = command_line::get_arg(m_vm, arg_run_root);
  std::vector<std::string> base_args = build_base_args_without_worker_specific(argc, argv);
  std::filesystem::path run_root_abs = run_root.empty() ? std::filesystem::path(paths::default_run_root) : std::filesystem::path(run_root);

  if (run_root_abs.is_relative())
    run_root_abs = std::filesystem::absolute(run_root_abs);

  base_args.emplace_back(cli_args::multiprocess_run_root);
  base_args.emplace_back(run_root_abs.string());

  std::string exe = (argv && argv[0]) ? std::string(argv[0]) : std::string();
  if (exe.empty())
  {
    std::cout << concolor::magenta << "Cannot spawn workers: empty executable path (argv[0])" << concolor::normal << '\n';
    return 1;
  }

  try
  {
    std::filesystem::path exe_path(exe);

    if (exe.find('/') == std::string::npos && exe.find('\\') == std::string::npos)
    {
      boost::filesystem::path resolved = bp::search_path(exe);
      if (!resolved.empty())
        exe_path = std::filesystem::path(resolved.string());
    }

    if (exe_path.is_relative())
      exe_path = std::filesystem::absolute(exe_path);

    exe = exe_path.string();
  }
  catch (const std::exception& e)
  {
    std::cout << concolor::magenta << "Failed to resolve executable path: " << e.what() << concolor::normal << '\n';
  }
  catch (...)
  {
    std::cout << concolor::magenta << "Failed to resolve executable path: unknown error" << concolor::normal << '\n';
  }

  if (exe.empty() || !std::filesystem::exists(std::filesystem::path(exe)))
  {
    std::cout << concolor::magenta << "Cannot spawn workers: failed to resolve executable: " << exe << concolor::normal << '\n';
    return 1;
  }

  const uint32_t parent_pid =
#if defined(_WIN32) || defined(_WIN64)
    static_cast<uint32_t>(::GetCurrentProcessId());
#else
    static_cast<uint32_t>(::getpid());
#endif

  const std::string shm_name = "zano_coretests_fail_" + std::to_string(parent_pid);

  namespace bip = boost::interprocess;

  bip::shared_memory_object::remove(shm_name.c_str());

  int failed_workers = 0;
  int rc = 1;

  try
  {
    bip::shared_memory_object shm(bip::create_only, shm_name.c_str(), bip::read_write);
    shm.truncate(sizeof(coretests_shm::shared_state));
    bip::mapped_region region(shm, bip::read_write);

    coretests_shm::shared_state* st = new (region.get_address()) coretests_shm::shared_state();
    st->init();

    base_args.emplace_back(cli_args::multiprocess_shm_name);
    base_args.emplace_back(shm_name);

    struct worker_proc
    {
      bp::child child;
      std::unique_ptr<bp::ipstream> out;
      std::unique_ptr<std::ofstream> file;
      std::thread reader;
      uint32_t wid = 0;
    };

    std::vector<worker_proc> kids;
    kids.reserve(processes);
    fill_shm_work_order(st, g_test_jobs);

    for (uint32_t i = 0; i < processes; ++i)
    {
      std::vector<std::string> args = base_args;

      args.emplace_back(cli_args::multiprocess_worker_id);
      args.emplace_back(std::to_string(i));

      const std::string worker_data_dir = make_worker_data_dir(run_root_abs, static_cast<int>(i));
      args.emplace_back(cli_args::data_dir);
      args.emplace_back(worker_data_dir);

      const std::filesystem::path worker_dir = get_worker_dir_path(i);
      std::filesystem::create_directories(worker_dir);

      try
      {
        const std::filesystem::path log_path = (worker_dir / files::worker_log).string();

        worker_proc wp;
        wp.wid = i;
        wp.out = std::make_unique<bp::ipstream>();
        wp.file = std::make_unique<std::ofstream>(log_path, std::ios::out | std::ios::app);

        if (!wp.file->is_open())
        {
          std::cout << concolor::magenta << "Failed to open log file for worker " << i << ": " << log_path << concolor::normal << '\n';
          failed_workers = 1;
          break;
        }

        wp.child = bp::child(
          exe,
          bp::args(args),
          bp::start_dir = worker_dir.string(),
          bp::std_out > *wp.out
        );

        wp.reader = std::thread([out = wp.out.get(), f = wp.file.get()]()
        {
          std::string line;
          while (std::getline(*out, line))
            (*f) << line << '\n';
        });

        kids.emplace_back(std::move(wp));
      }
      catch (const std::exception& e)
      {
        std::cout << concolor::magenta << "Failed to spawn worker " << i << ": " << e.what() << concolor::normal << '\n';

        for (auto& p : kids)
          if (p.child.running())
            p.child.terminate();
        for (auto& p : kids)
          p.child.wait();
        for (auto& p : kids)
          if (p.reader.joinable())
            p.reader.join();

        failed_workers = 1;
        break;
      }
    }

    std::atomic<bool> dashboard_stop{false};
    std::thread dashboard;
    if (!kids.empty())
    {
      dashboard = std::thread([this, &dashboard_stop, st, processes, &g_test_jobs]()
      {
        dashboard_loop(dashboard_stop, st, processes, g_test_jobs);
      });
    }

    for (auto& p : kids)
      p.child.wait();

    dashboard_stop.store(true, std::memory_order_release);
    if (dashboard.joinable())
      dashboard.join();

    for (auto& p : kids)
      if (p.reader.joinable())
        p.reader.join();

    for (const auto& p : kids)
      if (p.child.exit_code() != 0)
        ++failed_workers;

    const auto wall_end = std::chrono::steady_clock::now();
    const uint64_t wall_time_ms = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(wall_end - wall_start).count());

    rc = print_aggregated_report_and_return_rc(processes, wall_time_ms, st, g_test_jobs);

    if (failed_workers != 0 && rc == 0)
      rc = 1;

    std::vector<int> exit_codes;
    exit_codes.reserve(kids.size());
    for (const auto& p : kids)
      exit_codes.push_back(p.child.exit_code());

    const uint32_t spawned = static_cast<uint32_t>(kids.size());

    if (failed_workers != 0 || rc != 0)
      print_worker_failure_reasons(spawned, exit_codes);
  }
  catch (const std::exception& e)
  {
    std::cout << concolor::magenta << "Cannot spawn workers: " << e.what() << concolor::normal << std::endl;
    rc = 1;
  }
  catch (...)
  {
    std::cout << concolor::magenta << "Cannot spawn workers: unknown error" << concolor::normal << std::endl;
    rc = 1;
  }

  bip::shared_memory_object::remove(shm_name.c_str());
  return rc;
}

void parallel_test_runner::fill_worker_report_header(pt::ptree& root, const worker_report& rep) const
{
  root.put("worker_id", rep.worker_id);
  root.put("processes", rep.processes);
  root.put("tests_count", static_cast<uint64_t>(rep.tests_count));
  root.put("unique_tests_count", static_cast<uint64_t>(rep.unique_tests_count));
  root.put("total_time_ms", rep.total_time_ms);
  root.put("skip_all_till_the_end", rep.skip_all_till_the_end);
  root.put("exit_code", rep.exit_code);
}

void parallel_test_runner::read_worker_report_header(const pt::ptree& root, uint32_t worker_id_fallback, worker_report& rep) const
{
  rep.worker_id = root.get<uint32_t>("worker_id", worker_id_fallback);
  rep.processes = root.get<uint32_t>("processes", 1);
  rep.tests_count = static_cast<size_t>(root.get<uint64_t>("tests_count", 0));
  rep.unique_tests_count = static_cast<size_t>(root.get<uint64_t>("unique_tests_count", 0));
  rep.total_time_ms = root.get<uint64_t>("total_time_ms", 0);
  rep.skip_all_till_the_end = root.get<bool>("skip_all_till_the_end", false);
  rep.exit_code = root.get<int>("exit_code", 1);
}

bool parallel_test_runner::write_worker_report_file(const worker_report& rep) const
{
  try
  {
    pt::ptree root;
    std::filesystem::create_directories(get_worker_dir_path(rep.worker_id));
    fill_worker_report_header(root, rep);

    pt::ptree failed_arr;
    for (const auto& s : rep.failed_tests)
    {
      pt::ptree v;
      v.put("", s);
      failed_arr.push_back(std::make_pair("", v));
    }
    root.add_child("failed_tests", failed_arr);

    pt::ptree times_arr;
    for (const auto& it : rep.tests_running_time)
    {
      pt::ptree node;
      node.put("name", it.first);
      node.put("ms", it.second);
      times_arr.push_back(std::make_pair("", node));
    }
    root.add_child("tests_running_time", times_arr);

    std::ofstream out(get_worker_report_path(rep.worker_id));
    if (!out.is_open())
      return false;

    pt::write_json(out, root, /*pretty*/true);
    return true;
  }
  catch (const std::exception& e)
  {
    std::lock_guard<std::mutex> lk(cerr_mx);
    std::cerr << "write_worker_report_file: exception: " << e.what() << ", worker_id=" << rep.worker_id << std::endl;
    return false;
  }
  catch (...)
  {
    std::lock_guard<std::mutex> lk(cerr_mx);
    std::cerr << "write_worker_report_file: unknown exception, worker_id=" << rep.worker_id << std::endl;
    return false;
  }
}

bool parallel_test_runner::read_worker_report_file(uint32_t worker_id, worker_report& rep) const
{
  try
  {
    const std::filesystem::path path = get_worker_report_path(worker_id);
    if (!std::filesystem::exists(path))
      return false;

    pt::ptree root;
    pt::read_json(path.string(), root);
    read_worker_report_header(root, worker_id, rep);

    for (const auto& v : root.get_child("failed_tests", pt::ptree()))
    {
      const std::string name = v.second.get_value<std::string>();
      if (!name.empty())
        rep.failed_tests.insert(name);
    }

    for (const auto& v : root.get_child("tests_running_time", pt::ptree()))
    {
      const auto& node = v.second;
      const std::string name = node.get<std::string>("name", "");
      const uint64_t ms = node.get<uint64_t>("ms", 0);
      if (!name.empty())
        rep.tests_running_time.emplace_back(name, ms);
    }

    return true;
  }
  catch (const std::exception& e)
  {
    std::lock_guard<std::mutex> lk(cerr_mx);
    std::cerr << "read_worker_report_file: exception: " << e.what() << ", worker_id=" << worker_id << ", path=" << get_worker_report_path(worker_id).string() << std::endl;
    return false;
  }
  catch (...)
  {
    std::lock_guard<std::mutex> lk(cerr_mx);
    std::cerr << "read_worker_report_file: unknown exception" << ", worker_id=" << worker_id << ", path=" << get_worker_report_path(worker_id).string() << std::endl;
    return false;
  }
}

std::filesystem::path parallel_test_runner::get_worker_log_path(uint32_t worker_id) const
{
  return get_worker_dir_path(worker_id) / files::worker_log;
}

std::filesystem::path parallel_test_runner::get_taken_tests_log_path_for_this_process() const
{
  const uint32_t processes = command_line::get_arg(m_vm, arg_processes);
  const int32_t worker_id  = command_line::get_arg(m_vm, arg_worker_id);

  if (processes > 1 && worker_id >= 0)
  {
    const uint32_t wid = static_cast<uint32_t>(worker_id);
    std::filesystem::create_directories(get_worker_dir_path(wid));
    return get_worker_dir_path(wid) / files::taken_tests_log;
  }

  const std::string data_dir = command_line::get_arg(m_vm, command_line::arg_data_dir);
  std::filesystem::create_directories(data_dir);
  return std::filesystem::path(data_dir) / files::taken_tests_log;
}

void parallel_test_runner::log_test_taken_by_this_process(const std::string& test_name) const
{
  std::filesystem::path p;

  try
  {
    p = get_taken_tests_log_path_for_this_process();

    std::ofstream f(p, std::ios::out | std::ios::binary | std::ios::app);
    if (!f.is_open())
      return;

    f << test_name << '\n';
  }
  catch (const std::exception& e)
  {
    std::lock_guard<std::mutex> lk(cerr_mx);
    std::cerr << "parallel_test_runner::log_test_taken_by_this_process: exception: " << e.what() << ", test=" << test_name << ", path=" << p.string() << std::endl;
  }
  catch (...)
  {
    std::lock_guard<std::mutex> lk(cerr_mx);
    std::cerr << "parallel_test_runner::log_test_taken_by_this_process: unknown exception" << ", test=" << test_name << ", path=" << p.string() << std::endl;
  }
}

std::filesystem::path parallel_test_runner::get_workers_report_path() const
{
  return get_run_root_path() / WORKERS_REPORT_FILENAME;
}

bool parallel_test_runner::write_workers_report_file(uint32_t processes, const std::vector<worker_report>& reps) const
{
  try
  {
    std::unordered_map<std::string, uint64_t> ms_by_test;
    ms_by_test.reserve(RESERVE_UNIQUE_TESTS_HINT);

    for (const auto& r : reps)
    {
      for (const auto& it : r.tests_running_time)
      {
        const std::string& name = it.first;
        const uint64_t ms = it.second;
        auto ins = ms_by_test.emplace(name, ms);
        if (!ins.second)
          ins.first->second = std::max(ins.first->second, ms);
      }
    }

    std::vector<std::pair<std::string, uint64_t>> tests;
    tests.reserve(ms_by_test.size());
    for (auto& kv : ms_by_test)
      tests.emplace_back(std::move(kv.first), kv.second);

    std::sort(tests.begin(), tests.end(), [](const auto& a, const auto& b)
    {
      if (a.second != b.second) return a.second > b.second;
      return a.first < b.first;
    });

    pt::ptree root;
    root.put("format", 1);
    root.put("processes", processes);

    pt::ptree arr;
    for (const auto& t : tests)
    {
      pt::ptree node;
      node.put("name", t.first);
      node.put("ms", t.second);
      arr.push_back(std::make_pair("", node));
    }
    root.add_child("tests", arr);

    std::filesystem::path path = get_workers_report_path();
    std::ofstream out(path);
    if (!out.is_open())
    {
      std::lock_guard<std::mutex> lk(cerr_mx);
      std::cerr << "write_workers_report_file: cannot open file: " << path.string() << std::endl;
      return false;
    }

    pt::write_json(out, root, /*pretty*/true);
    return true;
  }
  catch (const std::exception& e)
  {
    std::lock_guard<std::mutex> lk(cerr_mx);
    std::cerr << "write_workers_report_file: exception: " << e.what() << ", path=" << get_workers_report_path().string() << std::endl;
    return false;
  }
  catch (...)
  {
    std::lock_guard<std::mutex> lk(cerr_mx);
    std::cerr << "write_workers_report_file: unknown exception, path=" << get_workers_report_path().string() << std::endl;
    return false;
  }
}

bool parallel_test_runner::fill_shm_work_order(coretests_shm::shared_state* st, const std::vector<test_job>& jobs) const
{
  if (!st)
    return false;

  struct item_t { uint64_t ms; uint32_t idx; };

  std::vector<item_t> items;
  items.reserve(jobs.size());

  for (uint32_t i = 0; i < jobs.size(); ++i)
  {
    const uint64_t ms = get_test_estimated_ms(jobs[i].name);
    items.push_back(item_t{ms, i});
  }

  std::sort(items.begin(), items.end(), [](const item_t& a, const item_t& b)
  {
    if (a.ms != b.ms) return a.ms > b.ms;
    return a.idx < b.idx;
  });

  const uint32_t total = static_cast<uint32_t>(items.size());
  if (total > coretests_shm::k_max_tests)
  {
    st->tests_total.store(coretests_shm::k_max_tests, std::memory_order_release);
    st->next_pos.store(0, std::memory_order_release);
    st->stop_all.store(false, std::memory_order_release);
    for (uint32_t i = 0; i < coretests_shm::k_max_tests; ++i)
      st->order[i] = i;
    return false;
  }

  st->tests_total.store(total, std::memory_order_release);
  st->next_pos.store(0, std::memory_order_release);
  st->stop_all.store(false, std::memory_order_release);

  for (uint32_t pos = 0; pos < total; ++pos)
    st->order[pos] = items[pos].idx;

  return true;
}

void parallel_test_runner::dashboard_loop(std::atomic<bool>& stop_flag, const coretests_shm::shared_state* st,
  uint32_t processes, const std::vector<test_job>& jobs) const
{
  if (!st || jobs.empty())
    return;

  const bool ansi_ok = enable_ansi_terminal();
  const uint32_t workers_capped = std::min<uint32_t>(processes, coretests_shm::k_max_workers);
  const size_t jobs_capped = std::min<size_t>(jobs.size(), coretests_shm::k_max_tests);

  std::vector<bool> reported(jobs.size(), false);
  size_t prev_live_lines = 0;
  uint32_t frame = 0;

  auto flush_completions = [&]() -> size_t
  {
    size_t emitted = 0;
    for (size_t i = 0; i < jobs_capped; ++i)
    {
      if (reported[i])
        continue;
      const uint8_t s = st->test_status_arr[i].load(std::memory_order_acquire);
      if (s == coretests_shm::ts_ok)
      {
        std::cout << concolor::green << "[ OK ]  " << concolor::normal << jobs[i].name << '\n';
        reported[i] = true;
        ++emitted;
      }
      else if (s == coretests_shm::ts_failed)
      {
        std::cout << concolor::magenta << "[FAIL]  " << concolor::normal << jobs[i].name << '\n';
        reported[i] = true;
        ++emitted;
      }
    }
    return emitted;
  };

  auto erase_prev_live_zone = [&]()
  {
    if (!ansi_ok || prev_live_lines == 0)
      return;
    std::cout << "\r\033[" << prev_live_lines << "A\033[J";
    prev_live_lines = 0;
  };

  auto draw_live_zone = [&]() -> size_t
  {
    if (!ansi_ok)
      return 0;

    const int width = get_terminal_width();
    const std::string dots = dots_for_frame(frame);

    size_t lines = 0;
    for (uint32_t wid = 0; wid < workers_capped; ++wid)
    {
      const uint16_t v = st->worker_running_test[wid].load(std::memory_order_acquire);
      if (v == coretests_shm::k_no_test)
        continue;
      if (v >= jobs.size())
        continue;

      std::string line = "*  " + jobs[v].name + "  [w" + std::to_string(wid) + "]  " + dots;
      line = fit_to_width(line, width - 1);

      std::cout << concolor::yellow << line << concolor::normal << '\n';
      ++lines;
    }
    return lines;
  };

  while (!stop_flag.load(std::memory_order_acquire))
  {
    erase_prev_live_zone();
    flush_completions();
    prev_live_lines = draw_live_zone();
    std::cout.flush();

    ++frame;
    std::this_thread::sleep_for(std::chrono::milliseconds(DASHBOARD_TICK_MS));
  }

  erase_prev_live_zone();
  flush_completions();
  std::cout.flush();
}
