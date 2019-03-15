// keccak.h
// 19-Nov-11  Markku-Juhani O. Saarinen <mjos@iki.fi>

// Copyright (c) 2014 The Boolberry developers
// Copyright (c) 2019 The Hyle Team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#pragma once

#include <stdint.h>
#include <string.h>
#include "crypto.h"

extern "C" {
//#include "crypto/alt/KeccakNISTInterface.h"
  }

#ifndef KECCAK_ROUNDS
#define KECCAK_ROUNDS 24
#endif

#ifndef ROTL64
#define ROTL64(x, y) (((x) << (y)) | ((x) >> (64 - (y))))
#endif


#define KK_MIXIN_SIZE 24

namespace crypto
{
  typedef uint64_t state_t_m[25];
  typedef uint64_t mixin_t[KK_MIXIN_SIZE];

  template<class f_traits, class callback_t>
  int wild_keccak2(const uint8_t *in, size_t inlen, uint8_t *md, size_t mdlen, callback_t cb)
  {
    state_t_m st;
    uint8_t temp[144];
    uint64_t rsiz, rsizw;

    rsiz = sizeof(state_t_m) == mdlen ? HASH_DATA_AREA : 200 - 2 * mdlen;
    rsizw = rsiz / 8;
    memset(&st[0], 0, 25 * sizeof(st[0]));


    for (; inlen >= rsiz; inlen -= rsiz, in += rsiz)
    {
      for (size_t i = 0; i < rsizw; i++)
        st[i] ^= ((uint64_t *)in)[i];

      for (int keccak_round = 0; keccak_round != KECCAK_ROUNDS; keccak_round++)
      {
        f_traits::keccakf(st, keccak_round);
        cb(st);
      }
    }

    // last block and padding
    memcpy(temp, in, inlen);
    temp[inlen++] = 1;
    memset(temp + inlen, 0, rsiz - inlen);
    temp[rsiz - 1] |= 0x80;

    for (size_t i = 0; i < rsizw; i++)
      st[i] ^= ((uint64_t *)temp)[i];

    for (int keccak_round = 0; keccak_round != KECCAK_ROUNDS; keccak_round++)
    {
      f_traits::keccakf(st, keccak_round);
      cb(st);
    }

    memcpy(md, st, mdlen);

    return 0;
  }

  template<class f_traits, class callback_t>
  int wild_keccak2_dbl(const uint8_t *in, size_t inlen, uint8_t *md, size_t mdlen, callback_t cb)
  {
    //Satoshi's classic
    wild_keccak2<f_traits>(in, inlen, md, mdlen, cb);
    wild_keccak2<f_traits>(md, mdlen, md, mdlen, cb);
    return 0;
  }

  class regular_f
  {
  public:
    static void keccakf(uint64_t st[25], int rounds);
  };


  //------------------------------------------------------------------
  inline
    bool get_wild_keccak2(const std::string& bd, crypto::hash& res, const std::vector<crypto::hash>& scratchpad, uint64_t sz)
  {
  
    const uint64_t* int_array_ptr = (const uint64_t*)&scratchpad[0];
    size_t int64_sz = sz * 4;
  }
  //------------------------------------------------------------------
  inline
  bool get_wild_keccak2(const std::string& bd, crypto::hash& res, const uint64_t*& int_array_ptr_scratch, uint64_t int64_sz)
  {
    uint64_t count_access = 0;
    crypto::wild_keccak2_dbl<crypto::regular_f>(reinterpret_cast<const uint8_t*>(bd.data()), bd.size(), reinterpret_cast<uint8_t*>(&res), sizeof(res), [&](crypto::state_t_m& st)
    {
      ++count_access;
      if (!int64_sz)
      {
        return;
      }

      for (size_t i = 0; i != sizeof(st) / sizeof(st[0]); i++)
      {
        size_t depend_index = 0;
        if (i == 0)
        {
          depend_index = sizeof(st) / sizeof(st[0]) - 1;
        }
        else
        {
          depend_index = i - 1;
        }
        st[i] ^= int_array_ptr_scratch[int_array_ptr_scratch[int_array_ptr_scratch[st[depend_index] % int64_sz] % int64_sz] % int64_sz];
      }
    });   
    return true;
  }

  template<class t_items_accessor>
    bool get_wild_keccak_light(const std::string& bd, crypto::hash& res, t_items_accessor cb_get_item)
  {
    crypto::wild_keccak2_dbl<crypto::regular_f>(reinterpret_cast<const uint8_t*>(bd.data()), bd.size(), reinterpret_cast<uint8_t*>(&res), sizeof(res), [&](crypto::state_t_m& st)
    {
      for (size_t i = 0; i != sizeof(st) / sizeof(st[0]); i++)
      {
        size_t depend_index = 0;
        if (i == 0)
        {
          depend_index = sizeof(st) / sizeof(st[0]) - 1;
        }
        else
        {
          depend_index = i - 1;
        }
        st[i] ^= cb_get_item(cb_get_item(cb_get_item(st[depend_index])));
      }
    });
    return true;
  }
  //------------------------------------------------------------------
    bool get_wild_keccak_light(const std::string& bd, crypto::hash& res, const std::vector<crypto::hash>& scratchpad_light);    
  //------------------------------------------------------------------
  inline
    bool get_wild_keccak2(const std::string& bd, crypto::hash& res, const std::vector<crypto::hash>& scratchpad)
  {
    return get_wild_keccak2(bd, res, scratchpad, scratchpad.size());
  }
  //------------------------------------------------------------------
  bool generate_scratchpad(const crypto::hash& source_data, std::vector<crypto::hash>& result_data, uint64_t target_size);
  bool generate_scratchpad_light(const crypto::hash& seed_data, std::vector<crypto::hash>& result_data, uint64_t target_size);
}

