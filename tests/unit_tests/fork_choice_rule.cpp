// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include "include_base_utils.h"
#include "currency_core/currency_format_utils.h"

bool if_alt_chain_stronger(const currency::wide_difficulty_type& pos, const currency::wide_difficulty_type& pow)
{
  currency::difficulties main_cumul_diff;
  main_cumul_diff.pos_diff = 400000;
  main_cumul_diff.pow_diff = 4000;
  currency::difficulties alt_cumul_diff;
  alt_cumul_diff.pow_diff = pow;
  alt_cumul_diff.pos_diff = pos;
  static currency::wide_difficulty_type difficulty_pos_at_split_point = 400000;
  static currency::wide_difficulty_type difficulty_pow_at_split_point = 4000;
  boost::multiprecision::uint1024_t main = currency::get_a_to_b_relative_cumulative_difficulty(difficulty_pos_at_split_point, difficulty_pow_at_split_point, main_cumul_diff, alt_cumul_diff);
  boost::multiprecision::uint1024_t alt = currency::get_a_to_b_relative_cumulative_difficulty(difficulty_pos_at_split_point, difficulty_pow_at_split_point, alt_cumul_diff, main_cumul_diff);
  if (alt > main)
    return true;
  return false;
}

TEST(fork_choice_rule_test, fork_choice_rule_test_1)
{
//   std::stringstream ss;
//   for (uint64_t pos = 100000; pos < 1000001; pos += 10000)
//   {
//     for (uint64_t pow = 100; pow < 18000; pow += 100)
//     {
//       bool r = if_alt_chain_stronger(pos, pow);
//       if(r)
//         ss << pos << "\t" << pow << std::endl;
//         //ss << pos << "\t" << pow << "\t" << (r ? "1" : "0") << std::endl;
//       
// 
//     }
//   }
//   bool r = epee::file_io_utils::save_string_to_file("stat.txt", ss.str());
  bool res = false;
  res = if_alt_chain_stronger(1000000, 1000);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger(1000000, 1500);
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger(800000, 1700);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger(800000, 2000);
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger(600000, 2200);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger(600000, 2800);
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger(400000, 3999);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger(400000, 4001);
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger(200000, 7000);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger(200000, 7700);
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger(200000, 7000);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger(200000, 7700);
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger(100000, 10000);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger(200000, 14000);
  ASSERT_TRUE(res);
}


bool if_alt_chain_stronger_hf4(const currency::wide_difficulty_type& pos, const currency::wide_difficulty_type& pow)
{
  currency::difficulties main_cumul_diff;
  main_cumul_diff.pos_diff.assign("1605973467987652534120344647");
  main_cumul_diff.pow_diff.assign("3011264554002844981");
  currency::difficulties alt_cumul_diff;
  alt_cumul_diff.pow_diff = pow;
  alt_cumul_diff.pos_diff = pos;
  currency::wide_difficulty_type difficulty_pos_at_split_point = main_cumul_diff.pos_diff;
  currency::wide_difficulty_type difficulty_pow_at_split_point = main_cumul_diff.pow_diff;
  boost::multiprecision::uint1024_t main = currency::get_a_to_b_relative_cumulative_difficulty_hf4(difficulty_pos_at_split_point, difficulty_pow_at_split_point, main_cumul_diff, alt_cumul_diff);
  boost::multiprecision::uint1024_t alt = currency::get_a_to_b_relative_cumulative_difficulty_hf4(difficulty_pos_at_split_point, difficulty_pow_at_split_point, alt_cumul_diff, main_cumul_diff);
  if (alt > main)
    return true;
  return false;
}
TEST(fork_choice_rule_test, fork_choice_rule_test_hf4)
{
  std::stringstream ss;
  currency::wide_difficulty_type pos_start, pos_end, pos_step, pos_diveder;
  pos_start.assign("16059734679876525341203446");
  pos_end.assign  ("16059734679876525341203446400");
  pos_step.assign ("50000000000000000000000000");
  pos_diveder.assign("100000000000000000000000");

  currency::wide_difficulty_type pow_start, pow_end, pow_step, pow_diveder;
  pow_start.assign("301126455400284498");
  pow_end.assign  ("30112645540028449810");
  pow_step.assign ("500000000000000000");
  pow_diveder.assign("1000000000000000");


  for (currency::wide_difficulty_type pos = pos_start; pos < pos_end; pos += pos_step)
  {
    for (currency::wide_difficulty_type pow = pow_start; pow < pow_end; pow += pow_step)
    {
      bool r = if_alt_chain_stronger_hf4(pos, pow);
      if(r)
        ss << pos/ pos_diveder << "\t" << pow / pow_diveder << std::endl;
        //ss << pos << "\t" << pow << "\t" << (r ? "1" : "0") << std::endl;     
    }
  }
  bool r = epee::file_io_utils::save_string_to_file("stat_hf4.txt", ss.str());


  bool res = false;
  res = if_alt_chain_stronger_hf4(1000000, 1000);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(1000000, 1500);
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(800000, 1700);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(800000, 2000);
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(600000, 2200);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(600000, 2800);
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(400000, 3999);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(400000, 4001);
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(200000, 7000);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(200000, 7700);
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(200000, 7000);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(200000, 7700);
  ASSERT_TRUE(res);
  res = if_alt_chain_stronger_hf4(100000, 10000);
  ASSERT_FALSE(res);
  res = if_alt_chain_stronger_hf4(200000, 14000);
  ASSERT_TRUE(res);
}
