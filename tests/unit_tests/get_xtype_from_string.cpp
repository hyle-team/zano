// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include <string_tools.h>

using namespace epee::string_tools;

namespace
{
  template<typename T>
  void do_pos_test(T expected, const std::string& str)
  {
    T val;
    ASSERT_TRUE(get_xtype_from_string(val, str));
    ASSERT_EQ(expected, val);
  }

  template<typename T>
  void do_neg_test(const std::string& str)
  {
    T val;
    ASSERT_FALSE(get_xtype_from_string(val, str));
  }
}

#define MAKE_TEST_NAME(prefix, int_type, ln, test_type) \
  test_type ## _ ## prefix ## _ ## int_type ## _ ## ln

#define MAKE_POS_TEST_NAME(prefix, int_type, ln) \
  MAKE_TEST_NAME(prefix, int_type, ln, POS)

#define MAKE_NEG_TEST_NAME(prefix, int_type, ln) \
  MAKE_TEST_NAME(prefix, int_type, ln, NEG)

#define TEST_pos(int_type, expected, str)                                          \
  TEST(get_xtype_from_string, MAKE_POS_TEST_NAME(handles_pos, int_type, __LINE__)) \
  {                                                                                \
    do_pos_test<int_type>(expected, str);                                          \
  }

#define TEST_neg(int_type, str)                                                    \
  TEST(get_xtype_from_string, MAKE_NEG_TEST_NAME(handles_neg, int_type, __LINE__)) \
  {                                                                                \
    do_neg_test<int_type>(str);                                                    \
  }

TEST_pos(uint16_t,     0,     "0");
TEST_pos(uint16_t,     1,     "1");
TEST_pos(uint16_t, 65535, "65535");

TEST_neg(uint16_t, "");
TEST_neg(uint16_t, "+0");
TEST_neg(uint16_t, "+1");
TEST_neg(uint16_t, "+65535");
TEST_neg(uint16_t, "+65536");

TEST_neg(uint16_t, "-0");
TEST_neg(uint16_t, "-1");
TEST_neg(uint16_t, "-65535");
TEST_neg(uint16_t, "-65536");

TEST_neg(uint16_t, ".0");
TEST_neg(uint16_t, ".1");
TEST_neg(uint16_t, "0.0");
TEST_neg(uint16_t, "0.1");
TEST_neg(uint16_t, "1.0");
TEST_neg(uint16_t, "1.1");

TEST_neg(uint16_t, "w");
TEST_neg(uint16_t, "0w");
TEST_neg(uint16_t, "1w");
TEST_neg(uint16_t, "1w1");
TEST_neg(uint16_t, "65535w");

TEST_neg(uint16_t, "65536");
TEST_neg(uint16_t, "4294967296");
TEST_neg(uint16_t, "18446744073709551616");


TEST_pos(uint32_t,          0,          "0");
TEST_pos(uint32_t,          1,          "1");
TEST_pos(uint32_t, 4294967295, "4294967295");

TEST_neg(uint32_t, "");
TEST_neg(uint32_t, "+0");
TEST_neg(uint32_t, "+1");
TEST_neg(uint32_t, "+4294967295");
TEST_neg(uint32_t, "+4294967296");

TEST_neg(uint32_t, "-0");
TEST_neg(uint32_t, "-1");
TEST_neg(uint32_t, "-4294967295");
TEST_neg(uint32_t, "-4294967296");

TEST_neg(uint32_t, ".0");
TEST_neg(uint32_t, ".1");
TEST_neg(uint32_t, "0.0");
TEST_neg(uint32_t, "0.1");
TEST_neg(uint32_t, "1.0");
TEST_neg(uint32_t, "1.1");

TEST_neg(uint32_t, "w");
TEST_neg(uint32_t, "0w");
TEST_neg(uint32_t, "1w");
TEST_neg(uint32_t, "1w1");
TEST_neg(uint32_t, "4294967295w");

TEST_neg(uint32_t, "4294967296");
TEST_neg(uint32_t, "18446744073709551616");

TEST_pos(uint64_t, 0, "0");
TEST_pos(uint64_t, 1, "1");
TEST_pos(uint64_t, 18446744073709551615ULL, "18446744073709551615");

TEST_neg(uint64_t, "");
TEST_neg(uint64_t, "+0");
TEST_neg(uint64_t, "+1");
TEST_neg(uint64_t, "+18446744073709551615");
TEST_neg(uint64_t, "+18446744073709551616");

TEST_neg(uint64_t, "-0");
TEST_neg(uint64_t, "-1");
TEST_neg(uint64_t, "-18446744073709551615");
TEST_neg(uint64_t, "-18446744073709551616");

TEST_neg(uint64_t, ".0");
TEST_neg(uint64_t, ".1");
TEST_neg(uint64_t, "0.0");
TEST_neg(uint64_t, "0.1");
TEST_neg(uint64_t, "1.0");
TEST_neg(uint64_t, "1.1");

TEST_neg(uint64_t, "w");
TEST_neg(uint64_t, "0w");
TEST_neg(uint64_t, "1w");
TEST_neg(uint64_t, "1w1");
TEST_neg(uint64_t, "18446744073709551615w");

TEST_neg(uint64_t, "18446744073709551616");

TEST_pos(int16_t,  32'767,  "32767"); // 2^15 - 1
TEST_pos(int16_t, -32'768, "-32768"); // -2^15
TEST_pos(int16_t,       0,     "-0");
TEST_pos(int16_t,       0,     "+0");

TEST_neg(int16_t, "32768"); // 2^15
TEST_neg(int16_t, "+32768"); // 2^15
TEST_neg(int16_t, "-32769"); // -2^15 - 1
TEST_neg(int16_t, "");

TEST_pos(int32_t,  2'147'483'647,  "2147483647"); // 2^31 - 1
TEST_pos(int32_t, -2'147'483'648, "-2147483648"); // -2^31
TEST_pos(int32_t,              0,          "-0");
TEST_pos(int32_t,              0,          "+0");

TEST_neg(int32_t, "-2147483649");
TEST_neg(int32_t,  "2147483648");
TEST_neg(int32_t,            "");

TEST_pos(int64_t,        9'223'372'036'854'775'807LL,  "9223372036854775807"); // 2^63 - 1
TEST_pos(int64_t, -9'223'372'036'854'775'807LL - 1LL, "-9223372036854775808"); // -2^63
TEST_pos(int64_t,                                0LL,                   "-0");
TEST_pos(int64_t,                                0LL,                   "+0");

TEST_neg(int64_t, "-9223372036854775809"); // -2^63 - 1
TEST_neg(int64_t,  "9223372036854775808"); // 2^63
TEST_neg(int64_t,                     "");
