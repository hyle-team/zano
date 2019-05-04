// Copyright (c) 2019, anonimal <anonimal@zano.org>
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include "epee/include/zlib_helper.h"

struct zlib_helper : ::testing::Test
{
 protected:
  std::string const jed_t{"Zoink!"};  // Jed_T is *not* POSIX-compliant ;)
  std::string const zoinked{"789c8bcacfcccb5604000832022d"};  // generated from separate library (not epee)
  std::string compressed, decompressed;
};

TEST_F(zlib_helper, uno)
{
  std::string zed_t{jed_t};
  EXPECT_EQ(true, epee::zlib_helper::pack(zed_t));
  EXPECT_EQ(zoinked, epee::string_tools::buff_to_hex_nodelimer(zed_t));
  EXPECT_EQ(true, epee::zlib_helper::unpack(zed_t));
  EXPECT_EQ(zed_t, jed_t);
}

TEST_F(zlib_helper, due)
{
  EXPECT_EQ(true, epee::zlib_helper::pack(jed_t, compressed));
  EXPECT_EQ(zoinked, epee::string_tools::buff_to_hex_nodelimer(compressed));
  EXPECT_EQ(true, epee::zlib_helper::unpack(compressed, decompressed));
  EXPECT_EQ(decompressed, jed_t);
}
