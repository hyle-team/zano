// Copyright (c) 2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <algorithm>

#include "gtest/gtest.h"
#include "wallet/decoy_selection.h"

TEST(decoy_selection_test, decoy_selection_test)
{
  // const uint64_t test_scale_size = 20000;
  decoy_selection_generator dsg;
  dsg.init(100, decoy_selection_generator::dist_kind::regular);
  std::map<uint64_t, uint64_t> hits;



  //std::vector<uint64_t> decoys = dsg.generate_distribution(15);

  std::cout << "";
 
  //std::vector<uint64_t> hits(test_scale_size, 0);

  
   while (true)
   {
     std::vector<uint64_t> decoys = dsg.generate_distribution(15);
     for (auto d : decoys)
     {
       hits[d]++;
     }
 
     if (hits[10] > 500)
       break;
 
   }
   std::stringstream ss;
   for (auto it = hits.begin(); it != hits.end(); it++)
   {
     if (it->second != 0)
     {
       ss << it->first << ", " << it->second << std::endl;
     }
   }
   epee::file_io_utils::save_string_to_file("distribution.csv", ss.str());



}

// some test for youngest bin
static void check_youngest_bin(decoy_selection_generator::dist_kind kind)
{
  decoy_selection_generator dsg;
  dsg.init(1000, kind);

  const size_t samples_count = 2000000;
  std::vector<uint64_t> draws = dsg.generate_distribution(samples_count);

  std::map<uint64_t, uint64_t> hits;
  for (uint64_t d : draws)
    hits[d]++;

  //freq of youngest scaled height
  const uint64_t boundary_hits = hits[0];
  ASSERT_GT(boundary_hits, 0u) << "youngest height was never emitted";

  // mean per height frequency of the first few populated heights just above the boundary
  uint64_t neighbour_sum = 0, neighbour_count = 0;
  for (uint64_t h = 1; h <= 64 && neighbour_count < 8; ++h)
  {
    auto it = hits.find(h);
    if (it != hits.end() && it->second != 0)
    {
      neighbour_sum += it->second;
      ++neighbour_count;
    }
  }
  ASSERT_GT(neighbour_count, 0u) << "no populated neighbour heights found";
  const double neighbour_avg = static_cast<double>(neighbour_sum) / neighbour_count;
  const double ratio = boundary_hits / neighbour_avg;
  EXPECT_GT(ratio, 0.6) << "youngest height looks collapsed/anomalous: boundary_hits = " << boundary_hits
    << " neighbour_avg = " << neighbour_avg << " ratio = " << ratio;
  EXPECT_LT(ratio, 1.6) << "youngest height looks anomalous: boundary_hits = " << boundary_hits
    << " neighbour_avg = " << neighbour_avg << " ratio = " << ratio;
}

TEST(decoy_selection_test, youngest_bin_is_range_sampled_regular)
{
  check_youngest_bin(decoy_selection_generator::dist_kind::regular);
}

TEST(decoy_selection_test, youngest_bin_is_range_sampled_coinbase)
{
  check_youngest_bin(decoy_selection_generator::dist_kind::coinbase);
}

