// Copyright (c) 2023-2023 Zano Project (https://zano.org/)
// Copyright (c) 2023-2023 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

// This file contains Multi-Scalar Multiplication routines

#include "epee/include/misc_log_ex.h"
#include "crypto-sugar.h"

namespace crypto
{

  template<typename CT>
  bool msm_and_check_zero_naive(const scalar_vec_t& g_scalars, const scalar_vec_t& h_scalars, const point_t& summand)
  {
    CHECK_AND_ASSERT_MES(g_scalars.size() <= CT::c_bpp_mn_max, false, "g_scalars oversized");
    CHECK_AND_ASSERT_MES(h_scalars.size() <= CT::c_bpp_mn_max, false, "h_scalars oversized");

    point_t result = summand;

    for (size_t i = 0; i < g_scalars.size(); ++i)
      result += g_scalars[i] * CT::get_generator(false, i);

    for (size_t i = 0; i < h_scalars.size(); ++i)
      result += h_scalars[i] * CT::get_generator(true, i);

    if (!result.is_zero())
    {
      LOG_PRINT_L0("msm result is non zero: " << result);
      return false;
    }
    return true;
  }


  // https://eprint.iacr.org/2022/999.pdf
  // "Pippenger algorithm [1], and its variant that is widely used in the ZK space is called the bucket method"
  template<typename CT>
  bool msm_and_check_zero_pippenger_v3(const scalar_vec_t& g_scalars, const scalar_vec_t& h_scalars, const point_t& summand, uint8_t c)
  {
    // TODO: with c = 8 and with direct access got much worse result than with c = 7 and get_bits() for N = 128..256, consider checking again for bigger datasets (N>256) 
    // TODO: consider preparing a cached generators' points

    CHECK_AND_ASSERT_MES(g_scalars.size() <= CT::c_bpp_mn_max, false, "g_scalars oversized");
    CHECK_AND_ASSERT_MES(h_scalars.size() <= CT::c_bpp_mn_max, false, "h_scalars oversized");
    CHECK_AND_ASSERT_MES(c < 10, false, "c is too big");

    size_t C = 1ull << c;

    // k_max * c + (c-1) >= max_bit_idx
    // 
    //                 max_bit_idx - (c - 1)            max_bit_idx - (c - 1) + (c - 1)              max_bit_idx
    // k_max = ceil ( --------------------- ) = floor ( ------------------------------ )  =  floor ( ----------- )
    //                           c                                    c                                   c      
    const size_t b = 253; // the maximum number of bits in x  https://eprint.iacr.org/2022/999.pdf  TODO: we may also scan for maximum bit used in all the scalars if all the scalars are small
    const size_t max_bit_idx = b - 1;
    const size_t k_max = max_bit_idx / c;
    const size_t K = k_max + 1;

    std::vector<point_t> buckets(C * K);
    std::vector<bool> buckets_inited(C * K);
    std::vector<point_t> Sk(K);
    std::vector<bool> Sk_inited(K);
    std::vector<point_t> Gk(K);
    std::vector<bool> Gk_inited(K);

    // first loop, calculate partial bucket sums
    for (size_t n = 0; n < g_scalars.size(); ++n)
    {
      for (size_t k = 0; k < K; ++k)
      {
        uint64_t l = g_scalars[n].get_bits((uint8_t)(k * c), c); // l in [0; 2^c-1]
        if (l != 0)
        {
          size_t bucket_id = l * K + k;
          if (buckets_inited[bucket_id])
            buckets[bucket_id] += CT::get_generator(false, n);
          else
          {
            buckets[bucket_id] =  CT::get_generator(false, n);
            buckets_inited[bucket_id] = true;
          }
        }
      }
    }
    // still the first loop (continued)
    for (size_t n = 0; n < h_scalars.size(); ++n)
    {
      for (size_t k = 0; k < K; ++k)
      {
        uint64_t l = h_scalars[n].get_bits((uint8_t)(k * c), c); // l in [0; 2^c-1]
        if (l != 0)
        {
          size_t bucket_id = l * K + k;
          if (buckets_inited[bucket_id])
            buckets[bucket_id] += CT::get_generator(true, n);
          else
          {
            buckets[bucket_id] =  CT::get_generator(true, n);
            buckets_inited[bucket_id] = true;
          }
        }
      }
    }

    // the second loop
    for (size_t l = C - 1; l > 0; --l)
    {
      for (size_t k = 0; k < K; ++k)
      {
        size_t bucket_id = l * K + k;
        if (buckets_inited[bucket_id])
        {
          if (Sk_inited[k])
            Sk[k] += buckets[bucket_id];
          else
          {
            Sk[k] = buckets[bucket_id];
            Sk_inited[k] = true;
          }
        }

        if (Sk_inited[k])
        {
          if (Gk_inited[k])
            Gk[k] += Sk[k];
          else
          {
            Gk[k] = Sk[k];
            Gk_inited[k] = true;
          }
        }
      }
    }

    // the third loop: Horner’s rule
    point_t result = Gk_inited[K - 1] ? Gk[K - 1] : c_point_0;
    for (size_t k = K - 2; k != SIZE_MAX; --k)
    {
      result.modify_mul_pow_2(c);
      if (Gk_inited[k])
        result += Gk[k];
    }

    result += summand;

    if (!result.is_zero())
    {
      LOG_PRINT_L0("multiexp result is non zero: " << result);
      return false;
    }

    return true;
  }



  

  // Just switcher

  template<typename CT>
  bool msm_and_check_zero(const scalar_vec_t& g_scalars, const scalar_vec_t& h_scalars, const point_t& summand)
  {
    //return msm_and_check_zero_naive<CT>(g_scalars, h_scalars, summand);
    return msm_and_check_zero_pippenger_v3<CT>(g_scalars, h_scalars, summand, 7);
  }



} // namespace crypto
