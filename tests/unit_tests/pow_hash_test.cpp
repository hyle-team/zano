// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"
#include "crypto/crypto.h"
#include "currency_core/currency_basic.h"
#include "profile_tools.h"
//#include "crypto/wild_keccak.h"


#include "currency_core/currency_format_utils.h"

using namespace currency;
// 
// bool test_scratchpad_against_light_scratchpad()
// {
//   crypto::hash seed = crypto::cn_fast_hash("sdssdsffdss", 10);
//   std::vector<crypto::hash> scratchpad;
//   size_t count = 400;
//   TIME_MEASURE_START_MS(gen_time);
//   LOG_PRINT_L0("Generating full ....");
//   crypto::generate_scratchpad2(seed, scratchpad, count);
//   TIME_MEASURE_FINISH_MS(gen_time);
//   LOG_PRINT_L0("Generated full scratchpad " << (scratchpad.size() * 32) / (1024 * 1024) << "MB in " << gen_time << "ms");
//   std::vector<crypto::hash> scratchpad_light;
//   crypto::generate_scratchpad_light(seed, scratchpad_light, count);
// 
//   std::string hashing_str = "dsfssfadfada";
//   crypto::hash full_h = currency::null_hash;
//   crypto::hash light_h = currency::null_hash;
//   crypto::get_wild_keccak2(hashing_str, full_h, scratchpad);
//   crypto::get_wild_keccak_light(hashing_str, light_h, scratchpad_light);
//   if (full_h != light_h)
//   {
//     LOG_ERROR("Hash missmatch");
//     return false;
//   }
//   return true;
// }
// 
// TEST(pow_tests, test_full_against_light)
// {
//   bool res = test_scratchpad_against_light_scratchpad();
//   ASSERT_TRUE(res);
// }
