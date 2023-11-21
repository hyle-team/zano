// Copyright (c) 2012-2014 The Zano developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include <cstdint>
#include <vector>

#include "misc_language.h"
#include "currency_core/currency_format_utils.h"


TEST(governance_testig, governance_tests_1)
{
  std::string vote = "{\"ZGP11\":1,\"ZGP23\":0}";

  std::list<std::pair<std::string, bool>> votes;
  currency::parse_vote(vote, votes);
  ASSERT_TRUE(votes.size() == 2);
  ASSERT_TRUE(votes.begin()->first == "ZGP11");
  ASSERT_TRUE(votes.begin()->second == true);
  ASSERT_TRUE((++votes.begin())->first == "ZGP23");
  ASSERT_TRUE((++votes.begin())->second == false);

}