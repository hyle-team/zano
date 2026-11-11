// Copyright (c) 2012-2014 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/*



DISCLAIMER:
Due to major changes in tools::wallet2::select_indices_for_transfer this test should be completely rewritten.



#include "gtest/gtest.h"

#include <cstdint>
#include <vector>

#include "wallet/wallet2.h"

void set_testcase_free_amounts(std::map<uint64_t, std::list<size_t> >& free_amounts)
{
  free_amounts.clear();
  free_amounts[10].push_back(101);
  free_amounts[10].push_back(102);
  free_amounts[10].push_back(103);

  free_amounts[5].push_back(51);
  free_amounts[5].push_back(52);

  free_amounts[1].push_back(11);
  free_amounts[1].push_back(12);
  free_amounts[1].push_back(13);
  free_amounts[1].push_back(14);
  free_amounts[1].push_back(15);
}

TEST(wallet_select_indices_validate, wallet_select_indices_validate)
{
  std::vector<uint64_t> selected_indexes;
  std::map<uint64_t, std::list<size_t> > found_free_amounts;
  set_testcase_free_amounts(found_free_amounts);
  //case 1. 
  ASSERT_EQ(25, tools::wallet2::select_indices_for_transfer(selected_indexes, found_free_amounts, 25));

  //case 2.
  set_testcase_free_amounts(found_free_amounts);
  ASSERT_EQ(45, tools::wallet2::select_indices_for_transfer(selected_indexes, found_free_amounts, 46));
  //case 3.
  set_testcase_free_amounts(found_free_amounts);
  ASSERT_EQ(10, tools::wallet2::select_indices_for_transfer(selected_indexes, found_free_amounts, 10));
  //case 4.
  set_testcase_free_amounts(found_free_amounts);
  ASSERT_EQ(10, tools::wallet2::select_indices_for_transfer(selected_indexes, found_free_amounts, 9));

  set_testcase_free_amounts(found_free_amounts);
  ASSERT_EQ(10, tools::wallet2::select_indices_for_transfer(selected_indexes, found_free_amounts, 6));
  //case 5.
  set_testcase_free_amounts(found_free_amounts);
  ASSERT_EQ(5, tools::wallet2::select_indices_for_transfer(selected_indexes, found_free_amounts, 5));
  //case 5.
  set_testcase_free_amounts(found_free_amounts);
  ASSERT_EQ(5, tools::wallet2::select_indices_for_transfer(selected_indexes, found_free_amounts, 4));
  //case 6.
  set_testcase_free_amounts(found_free_amounts);
  ASSERT_EQ(0, tools::wallet2::select_indices_for_transfer(selected_indexes, found_free_amounts, 0));

*/
