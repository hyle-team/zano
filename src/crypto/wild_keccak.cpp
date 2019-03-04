// keccak.c
// 19-Nov-11  Markku-Juhani O. Saarinen <mjos@iki.fi>
// A baseline Keccak (3rd round) implementation.

// Memory-hard extension of keccak for PoW 
// Copyright (c) 2014 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "wild_keccak.h"
#include "include_base_utils.h"
namespace crypto
{

  const uint64_t keccakf_rndc[24] = 
  {
    0x0000000000000001, 0x0000000000008082, 0x800000000000808a,
    0x8000000080008000, 0x000000000000808b, 0x0000000080000001,
    0x8000000080008081, 0x8000000000008009, 0x000000000000008a,
    0x0000000000000088, 0x0000000080008009, 0x000000008000000a,
    0x000000008000808b, 0x800000000000008b, 0x8000000000008089,
    0x8000000000008003, 0x8000000000008002, 0x8000000000000080, 
    0x000000000000800a, 0x800000008000000a, 0x8000000080008081,
    0x8000000000008080, 0x0000000080000001, 0x8000000080008008
  };

  const int keccakf_rotc[24] = 
  {
    1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14, 
    27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44
  };

  const int keccakf_piln[24] = 
  {
    10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4, 
    15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1 
  };

  // update the state with given number of rounds
  void regular_f::keccakf(uint64_t st[25], int rounds)
  {
    int i, j, round;
    uint64_t t, bc[5];

    for (round = 0; round < rounds; round++) {

      // Theta
      for (i = 0; i < 5; i++)     
        bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];

      for (i = 0; i < 5; i++) {
        t = bc[(i + 4) % 5] ^ ROTL64(bc[(i + 1) % 5], 1);
        for (j = 0; j < 25; j += 5)
          st[j + i] ^= t;
      }

      // Rho Pi
      t = st[1];
      for (i = 0; i < 24; i++) {
        j = keccakf_piln[i];
        bc[0] = st[j];
        st[j] = ROTL64(t, keccakf_rotc[i]);
        t = bc[0];
      }

      //  Chi
      for (j = 0; j < 25; j += 5) {
        for (i = 0; i < 5; i++)
          bc[i] = st[j + i];
        for (i = 0; i < 5; i++)
          st[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
      }

      //  Iota
      st[0] ^= keccakf_rndc[round];
    }
  }

  void mul_f::keccakf(uint64_t st[25], int rounds)
  {
    int i, j, round;
    uint64_t t, bc[5];

    for (round = 0; round < rounds; round++) {

      // Theta
      for (i = 0; i < 5; i++)     
      {
        bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] * st[i + 15] * st[i + 20];//surprise
      }

      for (i = 0; i < 5; i++) {
        t = bc[(i + 4) % 5] ^ ROTL64(bc[(i + 1) % 5], 1);
        for (j = 0; j < 25; j += 5)
          st[j + i] ^= t;
      }

      // Rho Pi
      t = st[1];
      for (i = 0; i < 24; i++) {
        j = keccakf_piln[i];
        bc[0] = st[j];
        st[j] = ROTL64(t, keccakf_rotc[i]);
        t = bc[0];
      }

      //  Chi
      for (j = 0; j < 25; j += 5) {
        for (i = 0; i < 5; i++)
          bc[i] = st[j + i];
        for (i = 0; i < 5; i++)
          st[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
      }

      //  Iota
      st[0] ^= keccakf_rndc[round];
    }
  }
  bool generate_scratchpad(const crypto::hash& seed_data, std::vector<crypto::hash>& result_data, uint64_t target_size)
  {
    result_data.resize(target_size);
    result_data[0] = crypto::cn_fast_hash(&seed_data, sizeof(seed_data));
    for (size_t i = 1; i < target_size; i++)
    {
      result_data[i] = crypto::cn_fast_hash(&result_data[i - 1], sizeof(result_data[i - 1]));
    }
    return true;
  }

#define WK2_COUNT 0

  bool generate_scratchpad2(const crypto::hash& seed_data, std::vector<crypto::hash>& result_data, uint64_t target_size)
  {
    CHECK_AND_ASSERT_THROW_MES(target_size % 10 == 0, "wrong target_size = " << target_size);
    result_data.resize(target_size);
    result_data[0] = crypto::cn_fast_hash(&seed_data, sizeof(seed_data));
    for (size_t i = 1; i < target_size; i++)
    {
      result_data[i] = crypto::cn_fast_hash(&result_data[i - 1], sizeof(result_data[i - 1]));     
    }
    return true;
  }

  bool generate_scratchpad_light(const crypto::hash& seed_data, std::vector<crypto::hash>& result_data, uint64_t target_size)
  {
    CHECK_AND_ASSERT_THROW_MES(target_size % 10 == 0, "wrong target_size = " << target_size);
    result_data.reserve(target_size/10);
    result_data.push_back(crypto::cn_fast_hash(&seed_data, sizeof(seed_data)));
    crypto::hash prev_hash = result_data[0];
    for (size_t i = 1; i < target_size; i++)
    {
      prev_hash = crypto::cn_fast_hash(&prev_hash, sizeof(prev_hash));
      if (!(i % 10))
      {
        result_data.push_back(prev_hash);
      }
    }
    return true;
  }


  bool get_wild_keccak_light(const std::string& bd, crypto::hash& res, const std::vector<crypto::hash>& scratchpad_light)
  {
    if (!scratchpad_light.size())
      return false;
    auto light_scr_accessor = [&](uint64_t i)
    {
      //get index of int64 item in scratchpad from i, where is is random number in whole uint64_t range
      uint64_t int64_mod_index = i%(scratchpad_light.size() * 10 * 4);
      //get related hash index 
      uint64_t hash_index = int64_mod_index / 4;
      //get_in hash index (from 0 to 3)
      uint64_t in_hash_index = int64_mod_index % 4;

      //get index of primary hash in scratchpad_light
      uint64_t primary_item_index = (hash_index - (hash_index % 10)) / 10;
      uint64_t sha_count = hash_index % 10;
      crypto::hash res = scratchpad_light[primary_item_index];
      for (uint64_t i = 0; i != sha_count; i++)
      {
        res = cn_fast_hash(&res, sizeof(res));
      }
      return ((uint64_t*)&res)[in_hash_index];
    };
    return get_wild_keccak_light(bd, res, light_scr_accessor);
  }
}

