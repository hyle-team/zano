// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "performance_tests.h"
#include "performance_utils.h"
#include <mutex>

// tests
#include "construct_tx.h"
#include "check_ring_signature.h"
#include "derive_public_key.h"
#include "derive_secret_key.h"
#include "generate_key_derivation.h"
#include "generate_key_image.h"
#include "generate_key_image_helper.h"
#include "is_out_to_acc.h"
#include "core_market_performance_test.h"
#include "serialization_performance_test.h"
#include "keccak_test.h"
#include "blake2_test.h"
#include "print_struct_to_json.h"
#include "free_space_check.h"

int main(int argc, char** argv)
{
  epee::string_tools::set_module_name_and_folder(argv[0]);
  epee::log_space::get_set_log_detalisation_level(true, LOG_LEVEL_2);
  epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_2);
  epee::log_space::log_singletone::add_logger(LOGGER_FILE,
    epee::log_space::log_singletone::get_default_log_file().c_str(),
    epee::log_space::log_singletone::get_default_log_folder().c_str());

  //run_serialization_performance_test();
  //return 1;
  //run_core_market_performance_tests(100000);

  set_process_affinity(1);
  set_thread_high_priority();

  //test_blake2();

  free_space_check();
  
  //print_struct_to_json();

  //performance_timer timer;
  //timer.start();

  //generate_scratchpad();
  //generate_light_scratchpad(); 
  //measure_keccak_over_scratchpad();
  //test_scratchpad_against_light_scratchpad();
  /*
  TEST_PERFORMANCE2(test_construct_tx, 1, 1);
  TEST_PERFORMANCE2(test_construct_tx, 1, 2);
  TEST_PERFORMANCE2(test_construct_tx, 1, 10);
  TEST_PERFORMANCE2(test_construct_tx, 1, 100);
  TEST_PERFORMANCE2(test_construct_tx, 1, 1000);

  TEST_PERFORMANCE2(test_construct_tx, 2, 1);
  TEST_PERFORMANCE2(test_construct_tx, 2, 2);
  TEST_PERFORMANCE2(test_construct_tx, 2, 10);
  TEST_PERFORMANCE2(test_construct_tx, 2, 100);

  TEST_PERFORMANCE2(test_construct_tx, 10, 1);
  TEST_PERFORMANCE2(test_construct_tx, 10, 2);
  TEST_PERFORMANCE2(test_construct_tx, 10, 10);
  TEST_PERFORMANCE2(test_construct_tx, 10, 100);

  TEST_PERFORMANCE2(test_construct_tx, 100, 1);
  TEST_PERFORMANCE2(test_construct_tx, 100, 2);
  TEST_PERFORMANCE2(test_construct_tx, 100, 10);
  TEST_PERFORMANCE2(test_construct_tx, 100, 100);

  TEST_PERFORMANCE1(test_check_ring_signature, 1);
  TEST_PERFORMANCE1(test_check_ring_signature, 2);
  TEST_PERFORMANCE1(test_check_ring_signature, 10);
  TEST_PERFORMANCE1(test_check_ring_signature, 100);
  */
  //TEST_PERFORMANCE0(test_is_out_to_acc);
  //TEST_PERFORMANCE0(test_generate_key_image_helper);
  //TEST_PERFORMANCE0(test_generate_key_derivation);
  //TEST_PERFORMANCE0(test_generate_key_image);
  //TEST_PERFORMANCE0(test_derive_public_key);
  //TEST_PERFORMANCE0(test_derive_secret_key);
  
  //std::cout << "Tests finished. Elapsed time: " << timer.elapsed_ms() / 1000 << " sec" << std::endl;

  return 0;
}
