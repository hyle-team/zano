// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include "include_base_utils.h"

int main(int argc, char** argv)
{
  epee::debug::get_set_enable_assert(true, false);

  epee::log_space::get_set_log_detalisation_level(true, LOG_LEVEL_2);
  epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_4);
  epee::log_space::log_singletone::add_logger(LOGGER_FILE, "unittests.log", "unittests_log");


  ::testing::InitGoogleTest(&argc, argv);
  int ret = RUN_ALL_TESTS();
  return ret;
}
