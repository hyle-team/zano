// keccak.h
// 19-Nov-11  Markku-Juhani O. Saarinen <mjos@iki.fi>

// Copyright (c) 2014 The Boolberry developers
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
#define OPT_XOR_4_RES(A_, B_, C_, D_, Res) \
  ((uint64_t*)&Res)[0] = ((const uint64_t*)&A_)[0] ^ ((const uint64_t*)&B_)[0] ^ ((const uint64_t*)&C_)[0] ^ ((const uint64_t*)&D_)[0]; \
  ((uint64_t*)&Res)[1] = ((const uint64_t*)&A_)[1] ^ ((const uint64_t*)&B_)[1] ^ ((const uint64_t*)&C_)[1] ^ ((const uint64_t*)&D_)[1]; \
  ((uint64_t*)&Res)[2] = ((const uint64_t*)&A_)[2] ^ ((const uint64_t*)&B_)[2] ^ ((const uint64_t*)&C_)[2] ^ ((const uint64_t*)&D_)[2]; \
  ((uint64_t*)&Res)[3] = ((const uint64_t*)&A_)[3] ^ ((const uint64_t*)&B_)[3] ^ ((const uint64_t*)&C_)[3] ^ ((const uint64_t*)&D_)[3];

  typedef uint64_t state_t_m[25];
  typedef uint64_t mixin_t[KK_MIXIN_SIZE];

  //with multiplication, for tests
  template<class f_traits>
  int keccak_generic(const uint8_t *in, size_t inlen, uint8_t *md, size_t mdlen)
  {
    state_t_m st;
    uint8_t temp[144];
    size_t i, rsiz, rsizw;

    rsiz = sizeof(state_t_m) == mdlen ? HASH_DATA_AREA : 200 - 2 * mdlen;
    rsizw = rsiz / 8;

    memset(st, 0, sizeof(st));

    for ( ; inlen >= rsiz; inlen -= rsiz, in += rsiz) {
      for (i = 0; i < rsizw; i++)
        st[i] ^= ((uint64_t *) in)[i];
      f_traits::keccakf(st, KECCAK_ROUNDS);
    }


    // last block and padding
    memcpy(temp, in, inlen);
    temp[inlen++] = 1;
    memset(temp + inlen, 0, rsiz - inlen);
    temp[rsiz - 1] |= 0x80;

    for (i = 0; i < rsizw; i++)
      st[i] ^= ((uint64_t *) temp)[i];

    f_traits::keccakf(st, KECCAK_ROUNDS);

    memcpy(md, st, mdlen);

    return 0;
  }
  /*inline 
  void print_state(UINT64* state, const char* comment, size_t rount)
    {
    printf("master_funct: %s round: %d\r\n", comment, rount);
    int i;
    for(i = 0; i != 25; i++)
      {
      printf("[%i]: %p\r\n", i, state[i]);
      }
    }*/

  template<class f_traits, class callback_t>
  int wild_keccak(const uint8_t *in, size_t inlen, uint8_t *md, size_t mdlen, callback_t cb)
  {
    state_t_m st;
    uint8_t temp[144];
    uint64_t rsiz, rsizw;

    rsiz = sizeof(state_t_m) == mdlen ? HASH_DATA_AREA : 200 - 2 * mdlen;
    rsizw = rsiz / 8;
    memset(&st[0], 0, 25*sizeof(st[0]));
    

    for ( ; inlen >= rsiz; inlen -= rsiz, in += rsiz) 
    {
      for (size_t i = 0; i < rsizw; i++)
        st[i] ^= ((uint64_t *) in)[i];

      for(size_t ll = 0; ll != KECCAK_ROUNDS; ll++)
      {
        if(ll != 0)
        {//skip first round
          mixin_t mix_in;
          cb(st, mix_in);
          for (size_t k = 0; k < KK_MIXIN_SIZE; k++)
            st[k] ^= mix_in[k];
        }
        //print_state(&st[0], "before_permut", ll);
        f_traits::keccakf(st, 1);
        //print_state(&st[0], "after_permut", ll);
      }
    }

    // last block and padding
    memcpy(temp, in, inlen);
    temp[inlen++] = 1;
    memset(temp + inlen, 0, rsiz - inlen);
    temp[rsiz - 1] |= 0x80;

    for (size_t i = 0; i < rsizw; i++)
      st[i] ^= ((uint64_t *) temp)[i];

    for(size_t ll = 0; ll != KECCAK_ROUNDS; ll++)
    {
      if(ll != 0)
      {//skip first state with
        mixin_t mix_in;
        cb(st, mix_in);
        for (size_t k = 0; k < KK_MIXIN_SIZE; k++)
          st[k] ^= mix_in[k]; 
      }
      f_traits::keccakf(st, 1);
    }

    memcpy(md, st, mdlen);

    return 0;
  }

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
  int wild_keccak_dbl(const uint8_t *in, size_t inlen, uint8_t *md, size_t mdlen, callback_t cb)
  {
    //Satoshi's classic
    wild_keccak<f_traits>(in, inlen, md, mdlen, cb);
    wild_keccak<f_traits>(md, mdlen, md, mdlen, cb);
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

  class mul_f
  {
  public:
    static void keccakf(uint64_t st[25], int rounds);
  };

  //------------------------------------------------------------------
  inline
  bool get_wild_keccak2(const std::string& bd, crypto::hash& res, const std::vector<crypto::hash>& scratchpad, uint64_t sz)
  {
    crypto::wild_keccak2_dbl<crypto::regular_f>(reinterpret_cast<const uint8_t*>(bd.data()), bd.size(), reinterpret_cast<uint8_t*>(&res), sizeof(res), [&](crypto::state_t_m& st)
    {
      if (!sz)
      {
        return;
      }

      const uint64_t* int_array_ptr = (const uint64_t*)&scratchpad[0];
      size_t int64_sz = sz * 4;

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
          st[i] ^= int_array_ptr[ int_array_ptr[ int_array_ptr[st[depend_index] % int64_sz] % int64_sz] % int64_sz];
        }
    });
    return true;
  }
  //------------------------------------------------------------------
  inline
    bool get_wild_keccak2(const std::string& bd, crypto::hash& res, const std::vector<crypto::hash>& scratchpad)
  {
    return get_wild_keccak2(bd, res, scratchpad, scratchpad.size());
  }
  //------------------------------------------------------------------
  inline
    bool get_wild_keccak(const std::string& bd, crypto::hash& res, uint64_t height, const std::vector<crypto::hash>& scratchpad, uint64_t sz)
  {
    crypto::wild_keccak_dbl<crypto::mul_f>(reinterpret_cast<const uint8_t*>(bd.data()), bd.size(), reinterpret_cast<uint8_t*>(&res), sizeof(res), [&](crypto::state_t_m& st, crypto::mixin_t& mix)
    {
      if (!height)
      {
        memset(&mix, 0, sizeof(mix));
        return;
      }

#define OPT_GET_H(index) scratchpad[st[index]%sz]
#define OPT_GET_M(index) scratchpad[mix[index]%sz]

      for (size_t i = 0; i != 6; i++)
      {
        OPT_XOR_4_RES(OPT_GET_H(i * 4), OPT_GET_H(i * 4 + 1), OPT_GET_H(i * 4 + 2), OPT_GET_H(i * 4 + 3), (*(crypto::hash*)&mix[i * 4]));
      }
    });
    return true;
  }
  //------------------------------------------------------------------
  bool generate_scratchpad(const crypto::hash& source_data, std::vector<crypto::hash>& result_data, uint64_t target_size);


}

