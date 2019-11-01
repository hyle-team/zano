// Copyright (c) 2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"
#include "common/util.h"

TEST(p2p_client_version, test_1)
{
  using namespace tools;

  // good
  
  ASSERT_TRUE(check_remote_client_version("10.101.999.28391[deadbeef31337-dirty]"));
  ASSERT_TRUE(check_remote_client_version("1.1.9.237[aabcd]"));
  ASSERT_TRUE(check_remote_client_version("3.0.2.7[aa00bcd]"));
  ASSERT_TRUE(check_remote_client_version("1.4.2.7[aabcd]"));
  ASSERT_TRUE(check_remote_client_version("1.1.2.67[88f868c]"));

  ASSERT_TRUE(check_remote_client_version("1.1.2.67[88f868c]"));
  ASSERT_TRUE(check_remote_client_version("1.1.2.67[26c00a8]"));
  ASSERT_TRUE(check_remote_client_version("1.1.2.67[26c00a8-dirty]"));
  
  ASSERT_TRUE(check_remote_client_version("1.1.0.65[40ba8cd]"));
  ASSERT_TRUE(check_remote_client_version("1.1.0.63[b0f376b]"));
  
  ASSERT_TRUE(check_remote_client_version("1.1.0.58[14bd668]"));
  ASSERT_TRUE(check_remote_client_version("1.1.0.58[9920eb7]"));
  ASSERT_TRUE(check_remote_client_version("1.1.0.58[e0d4ad8]"));
  
  ASSERT_TRUE(check_remote_client_version("1.1.0.57[b77b915]"));
  ASSERT_TRUE(check_remote_client_version("1.1.0.57[7dd61ae]"));
  ASSERT_TRUE(check_remote_client_version("1.1.0.57[7dd61ae-dirty]"));

  ASSERT_TRUE(check_remote_client_version("1.1.0.57"));

  // bad
  
  ASSERT_FALSE(check_remote_client_version(""));
  ASSERT_FALSE(check_remote_client_version(" "));

  ASSERT_FALSE(check_remote_client_version("1.0.999"));

  ASSERT_FALSE(check_remote_client_version("1.0.40[f77f0d7]"));
  ASSERT_FALSE(check_remote_client_version("1.0.40[734b726]"));
  ASSERT_FALSE(check_remote_client_version("1.0.41[488e369]"));

  ASSERT_FALSE(check_remote_client_version("1.0.40[469]"));
  ASSERT_FALSE(check_remote_client_version("1.0.39[f77f0d7]"));
  ASSERT_FALSE(check_remote_client_version("1.0.38[f77f0d7-dirty]"));
  ASSERT_FALSE(check_remote_client_version("1.0.37[7dd61ae-dirty]"));
  ASSERT_FALSE(check_remote_client_version("0.0.500[000]"));
}
