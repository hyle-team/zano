// Copyright (c) 2022 Zano Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"
#include "currency_core/currency_format_utils.h"

using namespace currency;

namespace
{
  void do_pos_test(uint64_t expected, const std::string& str)
  {
    uint64_t val;
    std::string number_str = str;
    std::replace(number_str.begin(), number_str.end(), '_', '.');
    number_str.erase(std::remove(number_str.begin(), number_str.end(), '~'), number_str.end());
    ASSERT_TRUE(parse_amount(val, number_str));
    ASSERT_EQ(expected, val);
  }

  void do_neg_test(const std::string& str)
  {
    uint64_t val;
    std::string number_str = str;
    std::replace(number_str.begin(), number_str.end(), '_', '.');
    number_str.erase(std::remove(number_str.begin(), number_str.end(), '~'), number_str.end());
    ASSERT_FALSE(parse_amount(val, number_str));
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

// Overflow
TEST_neg(184467440737_09551616);
TEST_neg(184467440738);
TEST_neg(18446744073709551616);

// Two or more points
TEST_neg(__);
TEST_neg(0__);
TEST_neg(__0);
TEST_neg(0__0);
TEST_neg(0_0_);
TEST_neg(_0_0);
TEST_neg(0_0_0);

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
