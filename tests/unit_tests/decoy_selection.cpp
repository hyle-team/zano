// Copyright (c) 2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <algorithm>

#include "gtest/gtest.h"
#include "wallet/decoy_selection.h"

TEST(decoy_selection_test, decoy_selection_test)
{
  const uint64_t test_scale_size = 20000;
  decoy_selection_generator dsg;
  dsg.init(100);
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

