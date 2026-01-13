#pragma once
#include <cstdint>
#include <string>
#include "common/command_line.h"

namespace chaingen_args
{
  extern const char* WORKERS_REPORT_FILENAME;

  extern command_line::arg_descriptor<std::string> arg_test_data_path;
  extern command_line::arg_descriptor<bool>        arg_generate_test_data;
  extern command_line::arg_descriptor<bool>        arg_play_test_data;
  extern command_line::arg_descriptor<bool>        arg_generate_and_play_test_data;
  extern command_line::arg_descriptor<bool>        arg_test_transactions;
  extern command_line::arg_descriptor<std::string> arg_run_single_test;
  extern command_line::arg_descriptor<std::string> arg_run_multiple_tests;
  extern command_line::arg_descriptor<bool>        arg_enable_debug_asserts;
  extern command_line::arg_descriptor<bool>        arg_stop_on_fail;
  extern command_line::arg_descriptor<uint32_t>    arg_processes;
  extern command_line::arg_descriptor<int32_t>     arg_worker_id;
  extern command_line::arg_descriptor<std::string> arg_run_root;
}
