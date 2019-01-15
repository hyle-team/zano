// Copyright (c) 2012-2013 The Boolberry developers
// Copyright (c) 2012-2013 The Zano developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "crypto/crypto.h"
#include "currency_core/currency_basic.h"


extern "C" {
#include "crypto/keccak.h"
//#include "crypto/alt/KeccakNISTInterface.h"
}


#include "crypto/wild_keccak.h"
//#include "crypto/wild_keccak2.h"
#include "../core_tests/random_helper.h"





#define TEST_BUFF_LEN 200

class test_keccak_base
{
public:
  static const size_t loop_count = 100000;

  bool init()
  {
    currency::block b;
    m_buff = currency::get_block_hashing_blob(b);
    m_buff.append(32 * 4, 0);
    return true;
  }

  bool pretest()
  {
    ++m_buff[0];
    if (!m_buff[0])
      ++m_buff[0];
    return true;
  }
protected:
  std::string m_buff;
};

// class test_keccak : public test_keccak_base
// {
// public:
//   bool test()
//   {
//     pretest();
//     crypto::hash h;
//     keccak(reinterpret_cast<const uint8_t*>(&m_buff[0]), m_buff.size(), reinterpret_cast<uint8_t*>(&h), sizeof(h));
//     LOG_PRINT_L4(h);
//     return true;
//   }
// };


class test_keccak_generic : public test_keccak_base
{
public:
  bool test()
  {
    pretest();
    crypto::hash h;
    crypto::keccak_generic<crypto::regular_f>(reinterpret_cast<const uint8_t*>(&m_buff[0]), m_buff.size(), reinterpret_cast<uint8_t*>(&h), sizeof(h));
    LOG_PRINT_L4(h);
    return true;
  }
};

class test_keccak_generic_with_mul : public test_keccak_base
{
public:
  bool test()
  {
    pretest();
    crypto::hash h;
    crypto::keccak_generic<crypto::mul_f>(reinterpret_cast<const uint8_t*>(&m_buff[0]), m_buff.size(), reinterpret_cast<uint8_t*>(&h), sizeof(h));
    return true;
  }
};

template<int scratchpad_size>
class test_wild_keccak : public test_keccak_base
{
public:
  bool init()
  {
    m_scratchpad_vec.resize(scratchpad_size / sizeof(crypto::hash));
    for (auto& h : m_scratchpad_vec)
      h = crypto::rand<crypto::hash>();

    return test_keccak_base::init();
  }

  bool test()
  {
    pretest();

    crypto::hash h;
    crypto::wild_keccak_dbl<crypto::mul_f>(reinterpret_cast<const uint8_t*>(&m_buff[0]), m_buff.size(), reinterpret_cast<uint8_t*>(&h), sizeof(h), [&](crypto::state_t_m& st, crypto::mixin_t& mix)
    {
#define SCR_I(i) m_scratchpad_vec[st[i]%m_scratchpad_vec.size()]
      for (size_t i = 0; i != 6; i++)
      {
        *(crypto::hash*)&mix[i * 4] = XOR_4(SCR_I(i * 4), SCR_I(i * 4 + 1), SCR_I(i * 4 + 2), SCR_I(i * 4 + 3));
      }
    });

    return true;
  }
protected:
  std::vector<crypto::hash> m_scratchpad_vec;
};

// 
// template<int scratchpad_size>
// class test_wild_keccak2 : public test_keccak_base
// {
// public:
//   bool init()
//   {
//     m_scratchpad_vec.resize(scratchpad_size / sizeof(crypto::hash));
//     for (auto& h : m_scratchpad_vec)
//       h = crypto::rand<crypto::hash>();
// 
//     return test_keccak_base::init();
//   }
// 
//   bool test()
//   {
//     pretest();
// 
//     crypto::hash h2;
//     crypto::wild_keccak_dbl_opt(reinterpret_cast<const uint8_t*>(&m_buff[0]), m_buff.size(), reinterpret_cast<uint8_t*>(&h2), sizeof(h2), (const UINT64*)&m_scratchpad_vec[0], m_scratchpad_vec.size() * 4);
//     LOG_PRINT_L4("HASH:" << h2);
//     return true;
//   }
// protected:
//   std::vector<crypto::hash> m_scratchpad_vec;
// };

#ifdef _DEBUG
  #define max_measere_scratchpad 100000
#else
  #define max_measere_scratchpad 10000000
#endif
#define measere_rounds 10000
void measure_keccak_over_scratchpad()
{
  std::cout << std::setw(20) << std::left << "sz\t" <<
    //std::setw(10) << "original\t" <<
    std::setw(10) << "original opt\t" <<
//    std::setw(10) << "w2\t" << 
    std::setw(10) << "w2_opt" << ENDL;

  std::vector<crypto::hash> scratchpad_vec;
  scratchpad_vec.resize(max_measere_scratchpad);
  std::string has_str = "Keccak is a family of sponge functions. The sponge function is a generalization of the concept of cryptographic hash function with infinite output and can perform quasi all symmetric cryptographic functions, from hashing to pseudo-random number generation to authenticated encryption";

  //static const uint64_t my_own_random_seed = 4669201609102990671;
  //random_state_test_restorer::reset_random(my_own_random_seed); // random seeded, entering deterministic mode...
  //uint64_t size_original = scratchpad_vec.size();
  //scratchpad_vec.resize(i / sizeof(crypto::hash));
  for (size_t j = 0; j != scratchpad_vec.size(); j++)
    scratchpad_vec[j] = crypto::rand<crypto::hash>();


  crypto::hash res_to_test = { 0 };
  crypto::hash res_etalon = { 0 };
  OPT_XOR_4_RES(scratchpad_vec[0], scratchpad_vec[1], scratchpad_vec[2], scratchpad_vec[3], res_to_test);
  res_etalon = XOR_4(scratchpad_vec[0], scratchpad_vec[1], scratchpad_vec[2], scratchpad_vec[3]);


// 
//   crypto::hash res_h1 = currency::null_hash;
//   res_h1 = crypto::get_wild_keccak2_over_scratchpad(has_str, 1, scratchpad_vec, 1000);
// 
//   crypto::hash res_h2 = currency::null_hash;
//   crypto::get_wild_keccak2(has_str, res_h2, 1, scratchpad_vec, 1000);
//   if (res_h2 != res_h1)
//   {
//     return;
//   }

  for (uint64_t i = 1000; i < max_measere_scratchpad; i += 100000)
  {

    crypto::hash res_h = currency::null_hash;
    uint64_t ticks_a = epee::misc_utils::get_tick_count();
    *(uint64_t*)(&has_str[8]) = i;
    //original keccak
//     for (size_t r = 0; r != measere_rounds; r++)
//     {
//       *(size_t*)(&has_str[0]) = r;
//       res_h = crypto::get_blob_longhash(has_str, 1, scratchpad_vec, i);
//     }
    //original keccak opt
    uint64_t ticks_b = epee::misc_utils::get_tick_count();
    for (size_t r = 0; r != measere_rounds; r++)
    {
      *(size_t*)(&has_str[1]) = r;
      crypto::get_wild_keccak(has_str, res_h, 1, scratchpad_vec, i);
    }

    //wild keccak 2
    uint64_t ticks_c = epee::misc_utils::get_tick_count();
//     for (size_t r = 0; r != measere_rounds; r++)
//     {
//       *(size_t*)(&has_str[1]) = r;
//       res_h = crypto::get_wild_keccak2_over_scratchpad(has_str, 1, scratchpad_vec, i);
//     }
    //wild keccak 2 opt
    uint64_t ticks_d = epee::misc_utils::get_tick_count();
    for (size_t r = 0; r != measere_rounds; r++)
    {
      crypto::get_wild_keccak2(has_str, res_h, 1, scratchpad_vec, i);
    }

    uint64_t ticks_e = epee::misc_utils::get_tick_count();
    std::cout << std::setw(20) << std::left << i * sizeof(crypto::hash) << "\t" <<
      //std::setw(10) << ticks_b - ticks_a << "\t" <<
      std::setw(10) << ticks_c - ticks_b << "\t" <<
      //std::setw(10) << ticks_d - ticks_c << "\t" <<
      std::setw(10) << ticks_e - ticks_d << ENDL;
  }
}
