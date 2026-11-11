// Copyright (c) 2025 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "gtest/gtest.h"
#include "string_tools.h"

TEST(epee_string, format_bytes_human_readable)
{
  ASSERT_EQ(epst::format_bytes_human_readable(0, 1, false, 1024),                   "0 B");

  ASSERT_EQ(epst::format_bytes_human_readable(1, 1, false, 1024),                   "1.0 B");
  ASSERT_EQ(epst::format_bytes_human_readable(1023, 0, false, 1024),                "1023 B");
  ASSERT_EQ(epst::format_bytes_human_readable(1023, 1, false, 1024),                "1023.0 B");
  ASSERT_EQ(epst::format_bytes_human_readable(1023, 2, false, 1024),                "1023.00 B");
  ASSERT_EQ(epst::format_bytes_human_readable(1024, 1, false, 1024),                "1.0 KB");
  ASSERT_EQ(epst::format_bytes_human_readable(1025, 1, false, 1024),                "1.0 KB");
  ASSERT_EQ(epst::format_bytes_human_readable(1024 * 1024, 1, false, 1024),         "1.0 MB");
  ASSERT_EQ(epst::format_bytes_human_readable(1024 * 1024 * 1024, 1, false, 1024),  "1.0 GB");
  
  // different decimal places
  ASSERT_EQ(epst::format_bytes_human_readable(1024, 0, false, 1024),                "1 KB");
  ASSERT_EQ(epst::format_bytes_human_readable(1024, 2, false, 1024),                "1.00 KB");
  ASSERT_EQ(epst::format_bytes_human_readable(1536, 1, false, 1024),                "1.5 KB");
  ASSERT_EQ(epst::format_bytes_human_readable(1536, 2, false, 1024),                "1.50 KB");
  ASSERT_EQ(epst::format_bytes_human_readable(1035, 2, false, 1024),                "1.01 KB");

  ASSERT_EQ(epst::format_bytes_human_readable(2042, 2, false, 1024),                "1.99 KB");
  ASSERT_EQ(epst::format_bytes_human_readable(2043, 2, false, 1024),                "2.00 KB");
  ASSERT_EQ(epst::format_bytes_human_readable(2047, 2, false, 1024),                "2.00 KB");
  ASSERT_EQ(epst::format_bytes_human_readable(2048, 2, false, 1024),                "2.00 KB");
  ASSERT_EQ(epst::format_bytes_human_readable(2049, 2, false, 1024),                "2.00 KB");
  
  // mebibytes
  ASSERT_EQ(epst::format_bytes_human_readable(1024, 1, true, 1024),                 "1.0 KiB");
  ASSERT_EQ(epst::format_bytes_human_readable(1024 * 1024, 1, true, 1024),          "1.0 MiB");
  ASSERT_EQ(epst::format_bytes_human_readable(1024 * 1024 * 1024, 1, true, 1024),   "1.0 GiB");
  ASSERT_EQ(epst::format_bytes_human_readable(1260, 2, true, 1024),                 "1.23 KiB");
  ASSERT_EQ(epst::format_bytes_human_readable(4783910, 2, true, 1024),              "4.56 MiB");
  ASSERT_EQ(epst::format_bytes_human_readable(8471822992ULL, 2, true, 1024),        "7.89 GiB");
  
  // different base unit
  ASSERT_EQ(epst::format_bytes_human_readable(1000, 1, false, 1000),                "1.0 KB");
  ASSERT_EQ(epst::format_bytes_human_readable(1000 * 1000, 1, false, 1000),         "1.0 MB");
  ASSERT_EQ(epst::format_bytes_human_readable(1000 * 1000 * 1000, 1, false, 1000),  "1.0 GB");
  ASSERT_EQ(epst::format_bytes_human_readable(1230, 2, false, 1000),                "1.23 KB");
  ASSERT_EQ(epst::format_bytes_human_readable(4560000, 2, false, 1000),             "4.56 MB");
  ASSERT_EQ(epst::format_bytes_human_readable(7890000000ULL, 2, false, 1000),       "7.89 GB");
  
  // large values
  ASSERT_EQ(epst::format_bytes_human_readable(1024ULL * 1024 * 1024 * 1024, 1, false, 1024), "1.0 TB");
  ASSERT_EQ(epst::format_bytes_human_readable(1024ULL * 1024 * 1024 * 1024 * 1024, 1, false, 1024), "1.0 PB");
  ASSERT_EQ(epst::format_bytes_human_readable(1024ULL * 1024 * 1024 * 1024 * 1024 * 1024, 1, false, 1024), "1.0 EB");
  
  // different decimal places for large values
  ASSERT_EQ(epst::format_bytes_human_readable(1024ULL * 1024 * 1024 * 1024 + 512, 1, false, 1024), "1.0 TB");
  ASSERT_EQ(epst::format_bytes_human_readable(1024ULL * 1024 * 1024 * 1024 + 512, 2, false, 1024), "1.00 TB");
  ASSERT_EQ(epst::format_bytes_human_readable(1024ULL * 1024 * 1024 * 1024 * 110 / 100, 1, false, 1024), "1.1 TB");
  ASSERT_EQ(epst::format_bytes_human_readable(1024ULL * 1024 * 1024 * 1024 * 101 / 100 , 2, false, 1024), "1.01 TB");
}
