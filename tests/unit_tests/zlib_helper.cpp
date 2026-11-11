// Copyright (c) 2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "gtest/gtest.h"
#include "epee/include/misc_log_ex.h"
#include "epee/include/zlib_helper.h"
#include "crypto/crypto.h"

TEST(zlib_helper, test_0)
{
  for (size_t len = 0; len <= 1024; ++len)
  {
    for(size_t iteration = 0; iteration < 4; ++iteration)
    {
      std::string original(len, 'X');
      if (len > 0)
        crypto::generate_random_bytes(len, &original.front());

      std::string result, decoded;
      ASSERT_TRUE(epee::zlib_helper::pack(original, result));
      ASSERT_TRUE(epee::zlib_helper::unpack(result, decoded));
      ASSERT_EQ(original, decoded);
    }
  }
}
