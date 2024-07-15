// Copyright (c) 2022-2024 Zano Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"
#include "currency_core/currency_format_utils.h"

using namespace currency;

namespace
{
  void do_pos_test(uint64_t expected, const std::string& str, size_t decimal_point = CURRENCY_DISPLAY_DECIMAL_POINT)
  {
    uint64_t val;
    std::string number_str = str;
    std::replace(number_str.begin(), number_str.end(), '_', '.');
    number_str.erase(std::remove(number_str.begin(), number_str.end(), '~'), number_str.end());
    ASSERT_TRUE(parse_amount(number_str, val, decimal_point));
    ASSERT_EQ(expected, val);
  }

  void do_neg_test(const std::string& str, size_t decimal_point = CURRENCY_DISPLAY_DECIMAL_POINT)
  {
    uint64_t val;
    std::string number_str = str;
    std::replace(number_str.begin(), number_str.end(), '_', '.');
    number_str.erase(std::remove(number_str.begin(), number_str.end(), '~'), number_str.end());
    ASSERT_FALSE(parse_amount(number_str, val, decimal_point));
  }
}

#define TEST_pos(expected, str)            \
  TEST(parse_amount, handles_pos_ ## str)  \
  {                                        \
    do_pos_test(UINT64_C(expected), #str); \
  }

#define TEST_neg(str)                      \
  TEST(parse_amount, handles_neg_ ## str)  \
  {                                        \
    do_neg_test(#str);                     \
  }

#define TEST_neg_n(str, name)              \
  TEST(parse_amount, handles_neg_ ## name) \
  {                                        \
    do_neg_test(#str);                     \
  }

#define TEST_pos_dp(expected, str, decimal_point)                   \
  TEST(parse_amount, handles_pos_ ## str ## _dp ## decimal_point)   \
  {                                                                 \
    do_pos_test(UINT64_C(expected), #str, decimal_point);                          \
  }

#define TEST_neg_dp(str, decimal_point)                             \
  TEST(parse_amount, handles_neg_ ## str ## _dp ## decimal_point)   \
  {                                                                 \
    do_neg_test(#str, decimal_point);                                              \
  }

#define TEST_neg_n_dp(str, name, decimal_point)                     \
  TEST(parse_amount, handles_neg_ ## name ## _dp ## decimal_point)  \
  {                                                                 \
    do_neg_test(#str, decimal_point);                                              \
  }

TEST_pos(0, 0);
TEST_pos(0, 00);
TEST_pos(0, 00000000);
TEST_pos(0, 000000000);
TEST_pos(0, 00000000000000000000000000000000);

TEST_pos(0, _0);
TEST_pos(0, _00);
TEST_pos(0, _00000000);
TEST_pos(0, _000000000);
TEST_pos(0, _00000000000000000000000000000000);

TEST_pos(0, 00000000_);
TEST_pos(0, 000000000_);
TEST_pos(0, 00000000000000000000000000000000_);

TEST_pos(0, 0_);
TEST_pos(0, 0_0);
TEST_pos(0, 0_00);
TEST_pos(0, 0_00000000);
TEST_pos(0, 0_000000000);
TEST_pos(0, 0_00000000000000000000000000000000);

TEST_pos(0, 00_);
TEST_pos(0, 00_0);
TEST_pos(0, 00_00);
TEST_pos(0, 00_00000000);
TEST_pos(0, 00_000000000);
TEST_pos(0, 00_00000000000000000000000000000000);

TEST_pos(10000, 0_00000001);
TEST_pos(10000, 0_000000010);
TEST_pos(10000, 0_000000010000000000000000000000000);
TEST_pos(90000, 0_00000009);
TEST_pos(90000, 0_000000090);
TEST_pos(90000, 0_000000090000000000000000000000000);

TEST_pos(           1000000000000,            1);
TEST_pos(       65535000000000000,        65535);
TEST_pos(  429496729500000000,   429496_7295);
TEST_pos(18446744073700000000, 18446744_0737);
TEST_pos(18446744073700000000, 18446744_07370);
TEST_pos(18446744073700000000, 18446744_073700000000);
TEST_pos(18446744073700000000, 18446744_0737000000000);
TEST_pos(18446744073700000000, 18446744_07370000000000000000000);
TEST_pos(18446744073709551615, 18446744_073709551615);

// non-standard decimal point
TEST_pos_dp(0, 0_0, 3);
TEST_pos_dp(0, 00_0, 3);
TEST_pos_dp(0, 00_00, 3);
TEST_pos_dp(0, 00000000_00, 3);
TEST_pos_dp(0, 00_000000000, 3);
TEST_pos_dp(0, 00_00000000000000000000000000000000, 3);

TEST_pos_dp( 65535,                65535,  0);
TEST_pos_dp( 6553500,              65535,  2);
TEST_pos_dp( 65535000000,          65535,  6);
TEST_pos_dp( 18000000000000000000, 18,     18);
TEST_pos_dp( 1,                    0_1,    1);
TEST_pos_dp( 10,                   0_1,    2);
TEST_pos_dp( 100,                  0_1,    3);
TEST_pos_dp( 10000000000000000000, 0_1,    20);
TEST_pos_dp( 1,                    0_001,  3);
TEST_pos_dp( 123,                  0_123,  3);
TEST_pos_dp( 1230,                 0_123,  4);
TEST_pos_dp( 12300,                0_123,  5);
TEST_pos_dp( 123000,               0_123,  6);

TEST_pos_dp(18446744073709551615, 18446744073709551615, 0);
TEST_pos_dp(18446744073709551615, 18446744073709551615_0, 0);

TEST_pos_dp(18446744073709551615, 1844674407370955161_5, 1);
TEST_pos_dp(18446744073709551615, 1844674407370955161_50, 1);

TEST_pos_dp(18446744073709551615, 18446744073709551_615, 3);
TEST_pos_dp(18446744073709551615, 18446744073709551_615000, 3);

TEST_pos_dp(18446744073709551615, 1_8446744073709551615, 19);
TEST_pos_dp(18446744073709551615, 1_844674407370955161500, 19);

TEST_pos_dp(18446744073709551615, 0_18446744073709551615, 20);
TEST_pos_dp(18446744073709551615, 0_1844674407370955161500, 20);


// Invalid numbers
TEST_neg_n(~, empty_string);
TEST_neg_n(-0, minus_0);
TEST_neg_n(+0, plus_0);
TEST_neg_n(-1, minus_1);
TEST_neg_n(+1, plus_1);
TEST_neg_n(_, only_point);

// A lot of fraction digits
TEST_neg(0_0000000000001);
TEST_neg(0_0000000000009);
TEST_neg(18446744_0737000000001);

TEST_neg_dp(00_184467440737095516150001, 20);
TEST_neg_dp(00_184467440737095516151, 20);
TEST_neg_dp(1_2, 0);

// Overflow
TEST_neg(184467440737_09551616);
TEST_neg(184467440738);
TEST_neg(18446744073709551616);
TEST_neg_dp(18446744073709551616, 0);
TEST_neg_dp(1844674407370955161_60, 1);
TEST_neg_dp(0_18446744073709551616, 20);

// Two or more points
TEST_neg(__);
TEST_neg(0__);
TEST_neg(__0);
TEST_neg(0__0);
TEST_neg(0_0_);
TEST_neg(_0_0);
TEST_neg(0_0_0);

// moved from test_format_utils.cpp
TEST(validate_parse_amount_case, validate_parse_amount)
{
  uint64_t res = 0;
  bool r = currency::parse_amount("0.0001", res);
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 100000000);

  r = currency::parse_amount("100.0001", res);
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 100000100000000);

  r = currency::parse_amount("000.0000", res);
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 0);

  r = currency::parse_amount("0", res);
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 0);


  r = currency::parse_amount("   100.0001    ", res);
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 100000100000000);

  r = currency::parse_amount("   100.0000    ", res);
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 100000000000000);

  r = currency::parse_amount("   100. 0000    ", res);
  ASSERT_FALSE(r);

  r = currency::parse_amount("100. 0000", res);
  ASSERT_FALSE(r);

  r = currency::parse_amount("100 . 0000", res);
  ASSERT_FALSE(r);

  r = currency::parse_amount("100.00 00", res);
  ASSERT_FALSE(r);

  r = currency::parse_amount("1 00.00 00", res);
  ASSERT_FALSE(r);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

size_t get_nonzero_digits_count(uint64_t x)
{
  size_t result = 0;
  while(x != 0)
  {
    if (x % 10 != 0)
      ++result;
    x /= 10;
  }
  return result;
}

void foo(uint64_t amount, size_t outputs_count, size_t num_digits_to_keep)
{
  std::vector<uint64_t> vec;
  decompose_amount_randomly(amount, [&](uint64_t a){ vec.push_back(a); }, outputs_count, num_digits_to_keep);
  //std::cout << amount << " -> (" << vec.size() << ")  ";
  ASSERT_EQ(vec.size(), outputs_count);
  for(size_t i = 0; i + 1 < outputs_count; ++i)
  {
    //std::cout << vec[i] << ",";
    ASSERT_LE(get_nonzero_digits_count(vec[i]), num_digits_to_keep);
  }
  //std::cout << vec.back() << ENDL;
  ASSERT_LE(get_nonzero_digits_count(vec.back()), num_digits_to_keep);
}

TEST(decompose_amount_randomly, 1)
{
  std::vector<uint64_t> vec;
  for(size_t i = 0; i < 1000; ++i)
  {
    vec.clear();
    decompose_amount_randomly(0, [&](uint64_t a){ vec.push_back(a); }, 2, 3);
    ASSERT_EQ(vec.size(), 0);

    vec.clear();
    decompose_amount_randomly(1, [&](uint64_t a){ vec.push_back(a); }, 2, 3);
    ASSERT_EQ(vec.size(), 0);

    vec.clear();
    decompose_amount_randomly(2, [&](uint64_t a){ vec.push_back(a); }, 2, 3);
    ASSERT_EQ(vec.size(), 2);
    ASSERT_EQ(vec[0], 1);
    ASSERT_EQ(vec[1], 1);

    vec.clear();
    decompose_amount_randomly(4, [&](uint64_t a){ vec.push_back(a); }, 2, 1);
    ASSERT_EQ(vec.size(), 2);
    ASSERT_LE(vec[0], 3);
    ASSERT_GE(vec[0], 1);
    ASSERT_LE(vec[1], 3);
    ASSERT_GE(vec[1], 1);

    vec.clear();
    decompose_amount_randomly(3, [&](uint64_t a){ vec.push_back(a); }, 3, 1);
    ASSERT_EQ(vec.size(), 3);
    ASSERT_EQ(vec[0], 1);
    ASSERT_EQ(vec[1], 1);
    ASSERT_EQ(vec[2], 1);

    foo(1000, 2, 3);
    foo(1000, 2, 2);
    foo(1000, 2, 1);

    foo(10000, 4, 2);
    foo(10010, 4, 3);
    foo(17283, 4, 4);
  
    foo(100000, 4, 5);
    foo(100000, 4, 5);

    foo(1000000000000, 2, 3);
  }
}

TEST(print_money_brief, 1)
{
  // decimal point 12 (default)
  ASSERT_EQ(print_money_brief(                   0),         "0.0");
  ASSERT_EQ(print_money_brief(                   1),         "0.000000000001");
  ASSERT_EQ(print_money_brief(       1000000000000),         "1.0");
  ASSERT_EQ(print_money_brief(       1900000000000),         "1.9");
  ASSERT_EQ(print_money_brief(       1000000100000),         "1.0000001");
  ASSERT_EQ(print_money_brief(       1000000000001),         "1.000000000001");
  ASSERT_EQ(print_money_brief(       9999999999999),         "9.999999999999");
  ASSERT_EQ(print_money_brief(      90009990009900),        "90.0099900099");
  ASSERT_EQ(print_money_brief(10109010000000000000),  "10109010.0");
  ASSERT_EQ(print_money_brief(10109010010000000000),  "10109010.01");
  ASSERT_EQ(print_money_brief(18446744073709551610),  "18446744.07370955161");
  ASSERT_EQ(print_money_brief(18446744073709551614),  "18446744.073709551614");
  ASSERT_EQ(print_money_brief(18446744073709551615),  "18446744.073709551615");

  // decimal point 0
  ASSERT_EQ(print_money_brief(                   0,  0),                     "0");
  ASSERT_EQ(print_money_brief(                   1,  0),                     "1");
  ASSERT_EQ(print_money_brief(       1000000000000,  0),         "1000000000000");
  ASSERT_EQ(print_money_brief(       1900000000000,  0),         "1900000000000");
  ASSERT_EQ(print_money_brief(       1000000100000,  0),         "1000000100000");
  ASSERT_EQ(print_money_brief(       1000000000001,  0),         "1000000000001");
  ASSERT_EQ(print_money_brief(       9999999999999,  0),         "9999999999999");
  ASSERT_EQ(print_money_brief(      90009990009900,  0),        "90009990009900");
  ASSERT_EQ(print_money_brief(10109010000000000000,  0),  "10109010000000000000");
  ASSERT_EQ(print_money_brief(10109010010000000000,  0),  "10109010010000000000");
  ASSERT_EQ(print_money_brief(18446744073709551610,  0),  "18446744073709551610");
  ASSERT_EQ(print_money_brief(18446744073709551614,  0),  "18446744073709551614");
  ASSERT_EQ(print_money_brief(18446744073709551615,  0),  "18446744073709551615");

  // decimal point 1
  ASSERT_EQ(print_money_brief(                   0,  1),                    "0.0");
  ASSERT_EQ(print_money_brief(                   1,  1),                    "0.1");
  ASSERT_EQ(print_money_brief(       1000000000000,  1),         "100000000000.0");
  ASSERT_EQ(print_money_brief(       1900000000000,  1),         "190000000000.0");
  ASSERT_EQ(print_money_brief(       1000000100000,  1),         "100000010000.0");
  ASSERT_EQ(print_money_brief(       1000000000001,  1),         "100000000000.1");
  ASSERT_EQ(print_money_brief(       9999999999999,  1),         "999999999999.9");
  ASSERT_EQ(print_money_brief(      90009990009900,  1),        "9000999000990.0");
  ASSERT_EQ(print_money_brief(10109010000000000000,  1),  "1010901000000000000.0");
  ASSERT_EQ(print_money_brief(10109010010000000000,  1),  "1010901001000000000.0");
  ASSERT_EQ(print_money_brief(18446744073709551610,  1),  "1844674407370955161.0");
  ASSERT_EQ(print_money_brief(18446744073709551614,  1),  "1844674407370955161.4");
  ASSERT_EQ(print_money_brief(18446744073709551615,  1),  "1844674407370955161.5");

  // decimal point 2
  ASSERT_EQ(print_money_brief(                   0,  2),                   "0.0");
  ASSERT_EQ(print_money_brief(                   1,  2),                   "0.01");
  ASSERT_EQ(print_money_brief(       1000000000000,  2),         "10000000000.0");
  ASSERT_EQ(print_money_brief(       1900000000000,  2),         "19000000000.0");
  ASSERT_EQ(print_money_brief(       1000000100000,  2),         "10000001000.0");
  ASSERT_EQ(print_money_brief(       1000000000001,  2),         "10000000000.01");
  ASSERT_EQ(print_money_brief(       9999999999999,  2),         "99999999999.99");
  ASSERT_EQ(print_money_brief(      90009990009900,  2),        "900099900099.0");
  ASSERT_EQ(print_money_brief(10109010000000000000,  2),  "101090100000000000.0");
  ASSERT_EQ(print_money_brief(10109010010000000000,  2),  "101090100100000000.0");
  ASSERT_EQ(print_money_brief(18446744073709551610,  2),  "184467440737095516.1");
  ASSERT_EQ(print_money_brief(18446744073709551614,  2),  "184467440737095516.14");
  ASSERT_EQ(print_money_brief(18446744073709551615,  2),  "184467440737095516.15");

  // decimal point 3
  ASSERT_EQ(print_money_brief(                   0,  3),                  "0.0");
  ASSERT_EQ(print_money_brief(                   1,  3),                  "0.001");
  ASSERT_EQ(print_money_brief(       1000000000000,  3),         "1000000000.0");
  ASSERT_EQ(print_money_brief(       1900000000000,  3),         "1900000000.0");
  ASSERT_EQ(print_money_brief(       1000000100000,  3),         "1000000100.0");
  ASSERT_EQ(print_money_brief(       1000000000001,  3),         "1000000000.001");
  ASSERT_EQ(print_money_brief(       9999999999999,  3),         "9999999999.999");
  ASSERT_EQ(print_money_brief(      90009990009900,  3),        "90009990009.9");
  ASSERT_EQ(print_money_brief(10109010000000000000,  3),  "10109010000000000.0");
  ASSERT_EQ(print_money_brief(10109010010000000000,  3),  "10109010010000000.0");
  ASSERT_EQ(print_money_brief(18446744073709551610,  3),  "18446744073709551.61");
  ASSERT_EQ(print_money_brief(18446744073709551614,  3),  "18446744073709551.614");
  ASSERT_EQ(print_money_brief(18446744073709551615,  3),  "18446744073709551.615");

  // decimal point 18
  ASSERT_EQ(print_money_brief(                   0, 18),   "0.0");
  ASSERT_EQ(print_money_brief(                   1, 18),   "0.000000000000000001");
  ASSERT_EQ(print_money_brief(       1000000000000, 18),   "0.000001");
  ASSERT_EQ(print_money_brief(       1900000000000, 18),   "0.0000019");
  ASSERT_EQ(print_money_brief(       1000000100000, 18),   "0.0000010000001");
  ASSERT_EQ(print_money_brief(       1000000000001, 18),   "0.000001000000000001");
  ASSERT_EQ(print_money_brief(       9999999999999, 18),   "0.000009999999999999");
  ASSERT_EQ(print_money_brief(      90009990009900, 18),   "0.0000900099900099");
  ASSERT_EQ(print_money_brief(10109010000000000000, 18),  "10.10901");
  ASSERT_EQ(print_money_brief(10109010010000000000, 18),  "10.10901001");
  ASSERT_EQ(print_money_brief(18446744073709551610, 18),  "18.44674407370955161");
  ASSERT_EQ(print_money_brief(18446744073709551614, 18),  "18.446744073709551614");
  ASSERT_EQ(print_money_brief(18446744073709551615, 18),  "18.446744073709551615");

  // decimal point 19
  ASSERT_EQ(print_money_brief(                   0, 19),  "0.0");
  ASSERT_EQ(print_money_brief(                   1, 19),  "0.0000000000000000001");
  ASSERT_EQ(print_money_brief(       1000000000000, 19),  "0.0000001");
  ASSERT_EQ(print_money_brief(       1900000000000, 19),  "0.00000019");
  ASSERT_EQ(print_money_brief(       1000000100000, 19),  "0.00000010000001");
  ASSERT_EQ(print_money_brief(       1000000000001, 19),  "0.0000001000000000001");
  ASSERT_EQ(print_money_brief(       9999999999999, 19),  "0.0000009999999999999");
  ASSERT_EQ(print_money_brief(      90009990009900, 19),  "0.00000900099900099");
  ASSERT_EQ(print_money_brief(10109010000000000000, 19),  "1.010901");
  ASSERT_EQ(print_money_brief(10109010010000000000, 19),  "1.010901001");
  ASSERT_EQ(print_money_brief(18446744073709551610, 19),  "1.844674407370955161");
  ASSERT_EQ(print_money_brief(18446744073709551614, 19),  "1.8446744073709551614");
  ASSERT_EQ(print_money_brief(18446744073709551615, 19),  "1.8446744073709551615");

  // TODO: remove it after setting reasonable limit of 18
  // decimal point 20
  ASSERT_EQ(print_money_brief(                   0, 20),  "0.0");
  ASSERT_EQ(print_money_brief(                   1, 20),  "0.00000000000000000001");
  ASSERT_EQ(print_money_brief(       1000000000000, 20),  "0.00000001");
  ASSERT_EQ(print_money_brief(       1900000000000, 20),  "0.000000019");
  ASSERT_EQ(print_money_brief(       1000000100000, 20),  "0.000000010000001");
  ASSERT_EQ(print_money_brief(       1000000000001, 20),  "0.00000001000000000001");
  ASSERT_EQ(print_money_brief(       9999999999999, 20),  "0.00000009999999999999");
  ASSERT_EQ(print_money_brief(      90009990009900, 20),  "0.000000900099900099");
  ASSERT_EQ(print_money_brief(10109010000000000000, 20),  "0.1010901");
  ASSERT_EQ(print_money_brief(10109010010000000000, 20),  "0.1010901001");
  ASSERT_EQ(print_money_brief(18446744073709551610, 20),  "0.1844674407370955161");
  ASSERT_EQ(print_money_brief(18446744073709551614, 20),  "0.18446744073709551614");
  ASSERT_EQ(print_money_brief(18446744073709551615, 20),  "0.18446744073709551615");

  // TODO: remove it after setting reasonable limit of 18
  // decimal point 21
  ASSERT_EQ(print_money_brief(                   0, 21),  "0.0");
  ASSERT_EQ(print_money_brief(                   1, 21),  "0.000000000000000000001");
  ASSERT_EQ(print_money_brief(       1000000000000, 21),  "0.000000001");
  ASSERT_EQ(print_money_brief(       1900000000000, 21),  "0.0000000019");
  ASSERT_EQ(print_money_brief(       1000000100000, 21),  "0.0000000010000001");
  ASSERT_EQ(print_money_brief(       1000000000001, 21),  "0.000000001000000000001");
  ASSERT_EQ(print_money_brief(       9999999999999, 21),  "0.000000009999999999999");
  ASSERT_EQ(print_money_brief(      90009990009900, 21),  "0.0000000900099900099");
  ASSERT_EQ(print_money_brief(10109010000000000000, 21),  "0.01010901");
  ASSERT_EQ(print_money_brief(10109010010000000000, 21),  "0.01010901001");
  ASSERT_EQ(print_money_brief(18446744073709551610, 21),  "0.01844674407370955161");
  ASSERT_EQ(print_money_brief(18446744073709551614, 21),  "0.018446744073709551614");
  ASSERT_EQ(print_money_brief(18446744073709551615, 21),  "0.018446744073709551615");
}
