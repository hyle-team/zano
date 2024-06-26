// Copyright (c) 2019-2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"
#include "common/util.h"

TEST(p2p_client_version, test_1)
{
  using namespace tools;

  // good (>= 2.x since HF4)
  
  ASSERT_TRUE(check_remote_client_version("10.101.999.28391[deadbeef31337-dirty]"));
  ASSERT_TRUE(check_remote_client_version("3.0.2.7[aa00bcd]"));

  ASSERT_TRUE(check_remote_client_version("2.0.0.000[a]"));
  ASSERT_TRUE(check_remote_client_version("2.4.2.7[aabcd]"));
  ASSERT_TRUE(check_remote_client_version("2.99.0.67[26c00a8-dirty]"));
  ASSERT_TRUE(check_remote_client_version("4.0.0.0[7dd61ae-dirty]"));
  ASSERT_TRUE(check_remote_client_version("5.0.0.0[7dd61ae-dirty]"));

  // bad
  
  ASSERT_FALSE(check_remote_client_version(""));
  ASSERT_FALSE(check_remote_client_version(" "));
  ASSERT_FALSE(check_remote_client_version("  "));
  ASSERT_FALSE(check_remote_client_version("   "));

  ASSERT_FALSE(check_remote_client_version("."));
  ASSERT_FALSE(check_remote_client_version(".."));
  ASSERT_FALSE(check_remote_client_version("..."));

  ASSERT_FALSE(check_remote_client_version("1.0.999"));

  ASSERT_FALSE(check_remote_client_version("1.0.40[f77f0d7]"));
  ASSERT_FALSE(check_remote_client_version("1.0.40[734b726]"));
  ASSERT_FALSE(check_remote_client_version("1.0.41[488e369]"));

  ASSERT_FALSE(check_remote_client_version("1.0.40[469]"));
  ASSERT_FALSE(check_remote_client_version("1.0.39[f77f0d7]"));
  ASSERT_FALSE(check_remote_client_version("1.0.38[f77f0d7-dirty]"));
  ASSERT_FALSE(check_remote_client_version("1.99.37[7dd61ae-dirty]"));
  ASSERT_FALSE(check_remote_client_version("0.0.500[000]"));

  ASSERT_FALSE(check_remote_client_version("a.1.9.237[aabcd]"));
  ASSERT_FALSE(check_remote_client_version("x.0.57"));
}
