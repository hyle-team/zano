#include "chaingen_args.h"

namespace chaingen_args
{
  const char* WORKERS_REPORT_FILENAME = "workers_report.json";

  command_line::arg_descriptor<std::string> arg_test_data_path               ("test-data-path", "", "");
  command_line::arg_descriptor<bool>        arg_generate_test_data           ("generate-test-data", "");
  command_line::arg_descriptor<bool>        arg_play_test_data               ("play-test-data", "");
  command_line::arg_descriptor<bool>        arg_generate_and_play_test_data  ("generate-and-play-test-data", "");
  command_line::arg_descriptor<bool>        arg_test_transactions            ("test-transactions", "");
  command_line::arg_descriptor<std::string> arg_run_single_test              ("run-single-test", "<TEST_NAME[@HF]> TEST_NAME -- name of a single test to run, HF -- specific hardfork id to run the test for" );
  command_line::arg_descriptor<std::string> arg_run_multiple_tests           ("run-multiple-tests", "comma-separated list of tests to run, OR text file <@filename> containing list of tests");
  command_line::arg_descriptor<bool>        arg_enable_debug_asserts         ("enable-debug-asserts", "" );
  command_line::arg_descriptor<bool>        arg_stop_on_fail                 ("stop-on-fail", "");
  command_line::arg_descriptor<uint32_t>    arg_processes                    ("multiprocess-run", "Run tests in parallel using the specified number of worker processes", 1);
  command_line::arg_descriptor<int32_t>     arg_worker_id                    ("multiprocess-worker-id", "Internal: index of the worker process (assigned automatically by parent)", -1);
  command_line::arg_descriptor<std::string> arg_run_root                     ("multiprocess-run-root", "Internal: directory used by parent and workers to store run artifacts and reports", "");
  command_line::arg_descriptor<std::string> arg_shm_name                     ("multiprocess-shm-name", "Internal: shared memory name used to report failed tests", "");
}
