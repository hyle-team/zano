// Copyright (c) 2019 The Zano developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
 
#include <string>
#include "include_base_utils.h"
#include "crypto/blake2.h"


inline
void test_blake2()
{
  std::string message = "The quick brown fox jumps over the lazy dog";
  static uint8_t buff[64] = {};
  static uint8_t buff2[2] = {};
  
  performance_timer timer;
  timer.start();
  for (size_t i = 0; i != 100000; i++)
  {
    *((size_t*)message.data()) = i;
    blake2(buff, sizeof(buff), message.data(), message.size(), nullptr, 0);
  }
  
  std::cout << "x100000 blake2 operations, out 64 bytes. Elapsed time: " << timer.elapsed_ms()  << " msec" << std::endl;

  timer.start();
  for (size_t i = 0; i != 1000000; i++)
  {
    *((size_t*)message.data()) = i;
    blake2(buff, sizeof(buff), message.data(), message.size(), nullptr, 0);
  }

  std::cout << "x1000000 blake2 operations, out 64 bytes. Elapsed time: " << timer.elapsed_ms() << " msec" << std::endl;


  timer.start();
  for (size_t i = 0; i != 1000000; i++)
  {
    *((size_t*)message.data()) = i;
    blake2(buff2, sizeof(buff2), message.data(), message.size(), nullptr, 0);
  }

  std::cout << "x1000000 blake2 operations, out 2 bytes. Elapsed time: " << timer.elapsed_ms() << " msec" << std::endl;

  static crypto::hash h = AUTO_VAL_INIT(h);
  timer.start();
  for (size_t i = 0; i != 1000000; i++)
  {
    *((size_t*)message.data()) = i;
    crypto::cn_fast_hash(message.data(), message.size(), h);    
  }

  std::cout << "x1000000 cn_fast_hash operations, out 32 bytes. Elapsed time: " << timer.elapsed_ms() << " msec" << std::endl;


  //std::cout << epee::string_tools::pod_to_hex(buff);
}