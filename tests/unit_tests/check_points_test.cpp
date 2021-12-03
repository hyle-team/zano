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


TEST(checkpoints_test, get_checkpoint_before_height_1)
{
  currency::checkpoints cp;
  cp.add_checkpoint(15, "0000000000000000000000000000000000000000000000000000000000000000");

  for (uint64_t h = 0; h <= 15; ++h)
    ASSERT_TRUE(cp.get_checkpoint_before_height(h) == 0);

  ASSERT_TRUE(cp.get_checkpoint_before_height(16) == 15);

  for (uint64_t h = 17; h < 100; ++h)
    ASSERT_TRUE(cp.get_checkpoint_before_height(h) == 15);
}


TEST(checkpoints_test, get_checkpoint_before_height_2)
{
  currency::checkpoints cp;
  cp.add_checkpoint(11, "0000000000000000000000000000000000000000000000000000000000000000");
  cp.add_checkpoint(15, "0000000000000000000000000000000000000000000000000000000000000000");

  for (uint64_t h = 0; h < 11; ++h)
    ASSERT_TRUE(cp.get_checkpoint_before_height(h) == 0);

  ASSERT_TRUE(cp.get_checkpoint_before_height(11) == 0);

  ASSERT_TRUE(cp.get_checkpoint_before_height(12) == 11);
  ASSERT_TRUE(cp.get_checkpoint_before_height(13) == 11);
  ASSERT_TRUE(cp.get_checkpoint_before_height(14) == 11);
  ASSERT_TRUE(cp.get_checkpoint_before_height(15) == 11);

  ASSERT_TRUE(cp.get_checkpoint_before_height(16) == 15);

  for (uint64_t h = 17; h < 100; ++h)
    ASSERT_TRUE(cp.get_checkpoint_before_height(h) == 15);
}


TEST(checkpoints_test, get_checkpoint_before_height_3)
{
  currency::checkpoints cp;
  cp.add_checkpoint(11, "0000000000000000000000000000000000000000000000000000000000000000");
  cp.add_checkpoint(15, "0000000000000000000000000000000000000000000000000000000000000000");
  cp.add_checkpoint(21, "0000000000000000000000000000000000000000000000000000000000000000");

  for (uint64_t h = 0; h < 11; ++h)
    ASSERT_TRUE(cp.get_checkpoint_before_height(h) == 0);

  ASSERT_TRUE(cp.get_checkpoint_before_height(11) == 0);

  ASSERT_TRUE(cp.get_checkpoint_before_height(12) == 11);
  ASSERT_TRUE(cp.get_checkpoint_before_height(13) == 11);
  ASSERT_TRUE(cp.get_checkpoint_before_height(14) == 11);
  ASSERT_TRUE(cp.get_checkpoint_before_height(15) == 11);

  ASSERT_TRUE(cp.get_checkpoint_before_height(16) == 15);
  ASSERT_TRUE(cp.get_checkpoint_before_height(17) == 15);
  ASSERT_TRUE(cp.get_checkpoint_before_height(18) == 15);
  ASSERT_TRUE(cp.get_checkpoint_before_height(19) == 15);
  ASSERT_TRUE(cp.get_checkpoint_before_height(20) == 15);
  ASSERT_TRUE(cp.get_checkpoint_before_height(21) == 15);

  ASSERT_TRUE(cp.get_checkpoint_before_height(22) == 21);

  for (uint64_t h = 22; h < 100; ++h)
    ASSERT_TRUE(cp.get_checkpoint_before_height(h) == 21);
}


TEST(checkpoints_test, is_in_checkpoint_zone)
{
  currency::checkpoints cp;
  cp.add_checkpoint(11, "0000000000000000000000000000000000000000000000000000000000000000");
  cp.add_checkpoint(15, "0000000000000000000000000000000000000000000000000000000000000000");
  cp.add_checkpoint(21, "0000000000000000000000000000000000000000000000000000000000000000");

  for (uint64_t h = 0; h < 22; ++h)
    ASSERT_TRUE(cp.is_in_checkpoint_zone(h));

  for (uint64_t h = 22; h < 100; ++h)
    ASSERT_FALSE(cp.is_in_checkpoint_zone(h));
}
