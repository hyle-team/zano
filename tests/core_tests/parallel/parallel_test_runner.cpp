#if defined(_WIN32)
  #include "chaingen.h"
#else
  #include "../chaingen.h"
#endif

#include <cstring>
#include <cctype>
#include <mutex>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/process.hpp>

#include "parallel_test_runner.h"
#include "../../src/common/command_line.h"

namespace bp = boost::process;
namespace pt = boost::property_tree;

extern command_line::arg_descriptor<std::string> arg_run_root;
extern command_line::arg_descriptor<uint32_t> arg_processes;
extern command_line::arg_descriptor<int32_t> arg_worker_id;
extern command_line::arg_descriptor<bool> arg_generate_test_data;
extern command_line::arg_descriptor<bool> arg_play_test_data;
extern command_line::arg_descriptor<bool> arg_generate_and_play_test_data;
extern command_line::arg_descriptor<bool> arg_test_transactions;

std::string TAKEN_TESTS_LOG_FILENAME = "taken_tests.log";

namespace
{
  const char* JSON_WORKER_ID           = "worker_id";
  const char* JSON_PROCESSES           = "processes";
  const char* JSON_TESTS_COUNT         = "tests_count";
  const char* JSON_UNIQUE_TESTS_COUNT  = "unique_tests_count";
  const char* JSON_TOTAL_TIME_MS       = "total_time_ms";
  const char* JSON_SKIP_ALL_TILL_END   = "skip_all_till_the_end";
  const char* JSON_EXIT_CODE           = "exit_code";

  const char* JSON_FAILED_TESTS        = "failed_tests";
  const char* JSON_TESTS_RUNNING_TIME  = "tests_running_time";

  const char* JSON_NAME                = "name";
  const char* JSON_MS                  = "ms";

  const char* WORKER_REPORT_FILENAME   = "coretests_report.json";
  const char* WORKER_LOG_FILENAME      = "worker.log";

  const char* ARG_WORKER_ID            = "--worker-id";
  const char* ARG_WORKER_ID_EQ         = "--worker-id=";

  const char* ARG_DATA_DIR             = "--data-dir";
  const char* ARG_DATA_DIR_EQ          = "--data-dir=";
  const char* WORKER_DIR_PREFIX        = "w";

  std::mutex cout_mx;
  std::mutex cerr_mx;

  bool arg_has_prefix(const std::string& arg, const std::string& prefix)
  {
    return arg.size() >= prefix.size() && arg.compare(0, prefix.size(), prefix) == 0;
  }
}

parallel_test_runner::parallel_test_runner(
  const boost::program_options::variables_map& vm)
  : m_vm(vm)
{}

int parallel_test_runner::run_parent_if_needed(int argc, char* argv[]) const
{
  const int rc = run_workers_and_wait(argc, argv);
  return rc;
}

bool parallel_test_runner::write_worker_report(const worker_report& rep) const
{
  return write_worker_report_file(rep);
}

std::filesystem::path parallel_test_runner::get_run_root_path() const
{
  std::string run_root = command_line::get_arg(m_vm, arg_run_root);
  if (run_root.empty())
    run_root = "chaingen_runs";

  std::filesystem::path run_root_path(run_root);

  if (run_root_path.is_relative())
    run_root_path = std::filesystem::absolute(run_root_path);

  return run_root_path;
}

std::filesystem::path parallel_test_runner::get_worker_dir_path(uint32_t worker_id) const
{
  return get_run_root_path() / (WORKER_DIR_PREFIX+ std::to_string(worker_id));
}

std::filesystem::path parallel_test_runner::get_worker_report_path(uint32_t worker_id) const
{
  return get_worker_dir_path(worker_id) / WORKER_REPORT_FILENAME;
}

std::string parallel_test_runner::make_worker_data_dir(const std::filesystem::path& run_root_abs, int worker_id) const
{
  std::filesystem::path worker_dir = run_root_abs / (std::string(WORKER_DIR_PREFIX) + std::to_string(worker_id));
  std::filesystem::create_directories(worker_dir);
  return worker_dir.string();
}

std::vector<std::string> parallel_test_runner::build_base_args_without_worker_specific(int argc, char* argv[]) const
{
  std::vector<std::string> args;
  args.reserve(static_cast<size_t>(argc));

  for (int i = 1; i < argc; ++i)
  {
    std::string current_arg = argv[i];

    // Strip worker-specific args to avoid duplicates
    if (current_arg == ARG_WORKER_ID || arg_has_prefix(current_arg, ARG_WORKER_ID_EQ))
    {
      if (current_arg == ARG_WORKER_ID && i + 1 < argc) ++i;
      continue;
    }

    if (current_arg == ARG_DATA_DIR || arg_has_prefix(current_arg, ARG_DATA_DIR_EQ))
    {
      if (current_arg == ARG_DATA_DIR && i + 1 < argc) ++i;
      continue;
    }

    args.emplace_back(std::move(current_arg));
  }

  return args;
}

void fill_postponed_tests_set(std::set<std::string>& postponed_tests);

int parallel_test_runner::print_aggregated_report_and_return_rc(uint32_t processes, uint64_t wall_time_ms) const
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

  for (const auto& r : reps)
    if (r.exit_code != 0)
      ++failed_workers;

  std::set<std::string> postponed_tests;
  fill_postponed_tests_set(postponed_tests);

  size_t total_tests_count = 0;
  size_t total_unique_tests_count = 0;

  std::set<std::string> failed_tests_union;

  for (const auto& r : reps)
  {
    total_tests_count += r.tests_count;
    total_unique_tests_count += r.unique_tests_count;

    failed_tests_union.insert(r.failed_tests.begin(), r.failed_tests.end());
  }

  size_t failed_postponed_tests_count = 0;
  for (const auto& t : failed_tests_union)
    if (postponed_tests.count(t) != 0)
      ++failed_postponed_tests_count;

  const size_t serious_failures_count = failed_tests_union.size() - failed_postponed_tests_count;

  std::cout << (serious_failures_count == 0 && failed_workers == 0 && !any_missing ? concolor::green : concolor::magenta);

  std::cout << "\nREPORT (aggregated):\n";
  std::cout << "  Workers:          " << processes << "\n";
  if (any_missing)
    std::cout << "  Warning:          some worker reports are missing\n";
  std::cout << "  Worker failures:  " << failed_workers << "\n";

  std::cout << "  Unique tests run: " << total_unique_tests_count << "\n";
  std::cout << "  Total tests run:  " << total_tests_count << "\n";
  std::cout << "  Failures:         " << serious_failures_count
            << " (postponed failures: " << failed_postponed_tests_count << ")\n";
  std::cout << "  Postponed:        " << postponed_tests.size() << "\n";
  std::cout << "  Total time:       " << (wall_time_ms / 1000) << " s. ("
            << (total_tests_count > 0 ? (wall_time_ms / total_tests_count) : 0)
            << " ms per test in average)\n";
  if (!failed_tests_union.empty())
  {
    std::cout << "FAILED/POSTPONED TESTS:\n";
    for (const auto& name : failed_tests_union)
    {
      const bool postponed = postponed_tests.count(name) != 0;
      std::cout << "  " << (postponed ? "POSTPONED: " : "FAILED:    ") << name << "\n";
    }
  }

  std::cout << concolor::normal << std::endl;

  // If any report missing or worker failed, treat as failure
  if (any_missing || failed_workers != 0)
    return 1;

  return serious_failures_count == 0 ? 0 : 1;
}

void parallel_test_runner::print_worker_failure_reasons(uint32_t processes, const std::vector<int>& worker_exit_codes) const
{
  bool any = false;

  for (uint32_t i = 0; i < processes; ++i)
  {
    const int ec = (i < worker_exit_codes.size() ? worker_exit_codes[i] : -999);

    worker_report rep;
    const bool report_ok = read_worker_report_file(i, rep);

    // Skip successful workers that produced a report with exit_code 0.
    if (report_ok && ec == 0 && rep.exit_code == 0)
      continue;

    if (!any)
    {
      std::cout << "\nFAILURE REASONS (workers):\n";
      any = true;
    }

    const auto worker_dir = get_worker_dir_path(i);
    const auto report_path = get_worker_report_path(i);
    const auto log_path = worker_dir / WORKER_LOG_FILENAME;

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
    {
      std::cout << "      - " << test_name << "\n";

      const auto logs = find_logs_for_test(worker_dir, test_name);
      if (logs.empty())
      {
        std::cout << "        log: (not found, expected prefix: " << sanitize_filename(test_name) << ")\n";
        continue;
      }

      std::cout << "        log: " << logs.front().string() << "\n";
    }
  }
}

int parallel_test_runner::run_workers_and_wait(int argc, char* argv[]) const
{
  const uint32_t processes = command_line::get_arg(m_vm, arg_processes);
  const int32_t worker_id  = command_line::get_arg(m_vm, arg_worker_id);

  // Only parent (multi-process mode, worker_id not set) spawns workers.
  if (processes <= 1 || worker_id >= 0)
    return -1;

  // Do not spawn workers for these modes.
  if (command_line::get_arg(m_vm, command_line::arg_help)) return -1;
  if (command_line::get_arg(m_vm, arg_generate_test_data)) return -1;
  if (command_line::get_arg(m_vm, arg_play_test_data)) return -1;
  if (command_line::get_arg(m_vm, arg_generate_and_play_test_data)) return -1;
  if (command_line::get_arg(m_vm, arg_test_transactions)) return -1;

  const std::string run_root = command_line::get_arg(m_vm, arg_run_root);
  std::vector<std::string> base_args = build_base_args_without_worker_specific(argc, argv);

  auto has_flag = [&](const char* flag, const std::string& prefix) -> bool
  {
    for (const auto& a : base_args)
      if (a == flag || arg_has_prefix(a, prefix))
        return true;
    return false;
  };

  if (!has_flag("--processes", "--processes="))
  {
    base_args.emplace_back("--processes");
    base_args.emplace_back(std::to_string(processes));
  }

  std::filesystem::path run_root_abs = run_root.empty()
    ? std::filesystem::path("chaingen_runs")
    : std::filesystem::path(run_root);

  if (run_root_abs.is_relative())
    run_root_abs = std::filesystem::absolute(run_root_abs);

  if (!has_flag("--run-root", "--run-root="))
  {
    base_args.emplace_back("--run-root");
    base_args.emplace_back(run_root_abs.string());
  }

  // Resolve executable path and make it absolute.
  std::string exe = (argv && argv[0]) ? std::string(argv[0]) : std::string();
  if (exe.empty())
  {
    std::cout << concolor::magenta
              << "Cannot spawn workers: empty executable path (argv[0])"
              << concolor::normal << std::endl;
    return 1;
  }

  try
  {
    std::filesystem::path exe_path(exe);

    // If no directory component is present, try PATH lookup first.
    if (exe.find('/') == std::string::npos && exe.find('\\') == std::string::npos)
    {
      boost::filesystem::path resolved = bp::search_path(exe);
      if (!resolved.empty())
        exe_path = std::filesystem::path(resolved.string());
    }

    // Make absolute because workers will run with start_dir = worker directory.
    if (exe_path.is_relative())
      exe_path = std::filesystem::absolute(exe_path);

    exe = exe_path.string();
  }
  catch (...) {}

  if (exe.empty() || !std::filesystem::exists(std::filesystem::path(exe)))
  {
    std::cout << concolor::magenta
              << "Cannot spawn workers: failed to resolve executable: " << exe
              << concolor::normal << std::endl;
    return 1;
  }

  std::vector<bp::child> kids;
  kids.reserve(processes);

  std::vector<int> worker_exit_codes(processes, -1);

  // Streams for tee
  std::vector<std::unique_ptr<bp::ipstream>> kids_out(processes);
  std::vector<std::unique_ptr<bp::ipstream>> kids_err(processes);

  // Tee threads
  std::vector<std::thread> tee_threads;
  tee_threads.reserve(static_cast<size_t>(processes) * 2);

  const auto wall_t0 = std::chrono::steady_clock::now();

  for (uint32_t i = 0; i < processes; ++i)
  {
    std::vector<std::string> args = base_args;

    args.emplace_back("--worker-id");
    args.emplace_back(std::to_string(i));

    const std::string worker_data_dir = make_worker_data_dir(run_root_abs, static_cast<int>(i));
    args.emplace_back("--data-dir");
    args.emplace_back(worker_data_dir);

    try
    {
      const auto worker_dir = get_worker_dir_path(i);
      std::filesystem::create_directories(worker_dir);

      kids_out[i] = std::make_unique<bp::ipstream>();
      kids_err[i] = std::make_unique<bp::ipstream>();

      kids.emplace_back(bp::child(
        exe,
        bp::args(args),
        bp::start_dir = worker_dir.string(),
        bp::std_out > *kids_out[i],
        bp::std_err > *kids_err[i]
      ));

      std::shared_ptr<worker_log_sink> sink(new worker_log_sink());
      sink->open(get_worker_dir_path(i) / WORKER_LOG_FILENAME);

      // Tee stdout: worker -> console; capture per-test log ONLY if failed
      tee_threads.emplace_back([worker_dir, s = kids_out[i].get(), sink]()
      {
        std::filesystem::create_directories(worker_dir);

        std::string line;
        while (std::getline(*s, line))
        {
          sink->write_line(line);
          {
            std::lock_guard<std::mutex> lk(cout_mx);
            std::cout << line << std::endl;
          }
        }
      });

      // Tee stderr: worker -> console(stderr) ONLY (no worker_stderr.log)
      tee_threads.emplace_back([s = kids_err[i].get(), sink]()
      {
        std::string line;
        while (std::getline(*s, line))
        {
          sink->write_line(std::string("[stderr] ") + line);
          std::lock_guard<std::mutex> lk(cerr_mx);
          std::cerr << line << std::endl;
        }
      });
    }
    catch (const std::exception& e)
    {
      std::cout << concolor::magenta
                << "Failed to spawn worker " << i << ": " << e.what()
                << concolor::normal << std::endl;

      for (auto& c : kids)
        if (c.running())
          c.terminate();
      for (auto& c : kids)
        c.wait();

      for (auto& t : tee_threads)
        if (t.joinable())
          t.join();

      return 1;
    }
  }

  int failed_workers = 0;
  for (uint32_t i = 0; i < kids.size(); ++i)
  {
    kids[i].wait();
    worker_exit_codes[i] = kids[i].exit_code();
    if (worker_exit_codes[i] != 0)
      ++failed_workers;
  }

  // Wait tee threads after children exit (streams will close).
  for (auto& t : tee_threads)
    if (t.joinable())
      t.join();

  const auto wall_t1 = std::chrono::steady_clock::now();
  const uint64_t wall_ms =
    static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(wall_t1 - wall_t0).count());

  // Aggregated report first.
  const int aggregated_rc = print_aggregated_report_and_return_rc(processes, wall_ms);

  // Then print diagnostics.
  if (failed_workers != 0)
    print_worker_failure_reasons(processes, worker_exit_codes);

  // Any worker non-zero => fail.
  if (failed_workers != 0)
    return 1;

  return aggregated_rc;
}

std::string parallel_test_runner::sanitize_filename(std::string s)
{
  for (char& c : s)
  {
    const bool ok =
      (c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9') ||
      c == '.' || c == '_' || c == '-';
    if (!ok)
      c = '_';
  }

  // Avoid empty name
  if (s.empty())
    s = "unnamed_test";

  // Keep filenames reasonably short
  constexpr size_t kMax = 180;
  if (s.size() > kMax)
    s.resize(kMax);

  return s;
}

bool parallel_test_runner::parse_test_name_from_header(const std::string& line, std::string& out_name)
{
  constexpr const char* kPrefix = "#TEST# >>>>";
  auto pos = line.find(kPrefix);
  if (pos != 0)
    return false;

  std::string rest = line.substr(std::strlen(kPrefix));
  // trim left
  while (!rest.empty() && std::isspace(static_cast<unsigned char>(rest.front())))
    rest.erase(rest.begin());

  auto end = rest.find("<<<<");
  if (end == std::string::npos)
    return false;

  std::string name = rest.substr(0, end);
  // trim right
  while (!name.empty() && std::isspace(static_cast<unsigned char>(name.back())))
    name.pop_back();

  if (name.empty())
    return false;

  out_name = name;
  return true;
}

std::filesystem::path parallel_test_runner::make_unique_log_path(const std::filesystem::path& dir, const std::string& base_name_no_ext)
{
  std::filesystem::path p = dir / (base_name_no_ext + ".log");
  if (!std::filesystem::exists(p))
    return p;

  for (size_t i = 2; i < 10000; ++i)
  {
    std::filesystem::path alt = dir / (base_name_no_ext + "_" + std::to_string(i) + ".log");
    if (!std::filesystem::exists(alt))
      return alt;
  }

  return p; // fallback
}

std::vector<std::filesystem::path> parallel_test_runner::find_logs_for_test(
  const std::filesystem::path& worker_dir,
  const std::string& test_name)
{
  std::vector<std::filesystem::path> result;

  const std::string base = sanitize_filename(test_name);
  const std::string base_prefix = base + "_";

  try
  {
    for (auto& de : std::filesystem::directory_iterator(worker_dir))
    {
      if (!de.is_regular_file())
        continue;

      const auto p = de.path();
      if (p.extension() != ".log")
        continue;

      const std::string stem = p.stem().string();
      if (stem == base || stem.rfind(base_prefix, 0) == 0)
        result.push_back(p);
    }
  }
  catch (...) {}

  std::sort(result.begin(), result.end(),
    [](const std::filesystem::path& a, const std::filesystem::path& b)
    {
      std::error_code ec1, ec2;
      const auto ta = std::filesystem::last_write_time(a, ec1);
      const auto tb = std::filesystem::last_write_time(b, ec2);
      if (!ec1 && !ec2 && ta != tb)
        return ta > tb;
      return a.string() < b.string();
    });

  return result;
}

bool parallel_test_runner::write_worker_report_file(const worker_report& rep) const
{
  try
  {
    std::filesystem::create_directories(get_worker_dir_path(rep.worker_id));

    pt::ptree root;
    root.put("worker_id", rep.worker_id);
    root.put("processes", rep.processes);
    root.put("tests_count", static_cast<uint64_t>(rep.tests_count));
    root.put("unique_tests_count", static_cast<uint64_t>(rep.unique_tests_count));
    root.put("total_time_ms", rep.total_time_ms);
    root.put("skip_all_till_the_end", rep.skip_all_till_the_end);
    root.put("exit_code", rep.exit_code);

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
  catch (...)
  {
    return false;
  }
}

bool parallel_test_runner::read_worker_report_file(uint32_t worker_id, worker_report& rep) const
{
  try
  {
    const auto path = get_worker_report_path(worker_id);
    if (!std::filesystem::exists(path))
      return false;

    pt::ptree root;
    pt::read_json(path.string(), root);

    rep.worker_id = root.get<uint32_t>(JSON_WORKER_ID, worker_id);
    rep.processes = root.get<uint32_t>(JSON_PROCESSES, 1);

    rep.tests_count = static_cast<size_t>(root.get<uint64_t>(JSON_TESTS_COUNT, 0));
    rep.unique_tests_count = static_cast<size_t>(root.get<uint64_t>(JSON_UNIQUE_TESTS_COUNT, 0));
    rep.total_time_ms = root.get<uint64_t>(JSON_TOTAL_TIME_MS, 0);

    rep.skip_all_till_the_end = root.get<bool>(JSON_SKIP_ALL_TILL_END, false);
    rep.exit_code = root.get<int>(JSON_EXIT_CODE, 1);

    for (const auto& v : root.get_child(JSON_FAILED_TESTS, pt::ptree()))
    {
      const std::string name = v.second.get_value<std::string>();
      if (!name.empty())
        rep.failed_tests.insert(name);
    }

    for (const auto& v : root.get_child(JSON_TESTS_RUNNING_TIME, pt::ptree()))
    {
      const auto& node = v.second;
      const std::string name = node.get<std::string>(JSON_NAME, "");
      const uint64_t ms = node.get<uint64_t>(JSON_MS, 0);
      if (!name.empty())
        rep.tests_running_time.emplace_back(name, ms);
    }

    return true;
  }
  catch (...)
  {
    return false;
  }
}

std::filesystem::path parallel_test_runner::get_worker_log_path(uint32_t worker_id) const
{
  return get_worker_dir_path(worker_id) / WORKER_LOG_FILENAME;
}
