// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include "common/util.h"
#include "currency_core/checkpoints.h"

TEST(checkpoints_test, test_checkpoints_for_alternative)
{
  currency::checkpoints cp;
  bool r = cp.is_height_passed_zone(1, 5);
  ASSERT_FALSE(r);
  r = cp.is_height_passed_zone(3, 5);
  ASSERT_FALSE(r);

  cp.add_checkpoint(1,  "0d6f94ae7e565c6d90ee0087d2feaad4617cd3038c4f8853f26756a6b6d535f3");
  cp.add_checkpoint(3,  "0d6f94ae7e565c6d90ee0087d2feaad4617cd3038c4f8853f26756a6b6d535f3");
  cp.add_checkpoint(5,  "0d6f94ae7e565c6d90ee0087d2feaad4617cd3038c4f8853f26756a6b6d535f3");
  cp.add_checkpoint(10,  "0d6f94ae7e565c6d90ee0087d2feaad4617cd3038c4f8853f26756a6b6d535f3");

  r = cp.is_height_passed_zone(1, 5);
  ASSERT_TRUE(r);
  r = cp.is_height_passed_zone(3, 5);
  ASSERT_TRUE(r);
  r = cp.is_height_passed_zone(4, 5);
  ASSERT_TRUE(r);
  r = cp.is_height_passed_zone(5, 5);
  ASSERT_TRUE(r);
  r = cp.is_height_passed_zone(6, 5);
  ASSERT_FALSE(r);
  r = cp.is_height_passed_zone(10, 5);
  ASSERT_FALSE(r);
  r = cp.is_height_passed_zone(11, 5);
  ASSERT_FALSE(r);
  r = cp.is_height_passed_zone(10, 10);
  ASSERT_TRUE(r);
  r = cp.is_height_passed_zone(11, 12);
  ASSERT_FALSE(r);
}
