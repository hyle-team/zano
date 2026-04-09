// Copyright (c) 2021-2023 Zano Project (https://zano.org/)
// Copyright (c) 2021-2023 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include "epee/include/misc_log_ex.h"
#include "crypto-sugar.h"

namespace crypto
{

  // returns x + x^2 + x^3 + ... + x^(2^f)
  // == x * (x + 1) * (x^2 + 1) * (x^4 + 1) * ...(x^(f+1) + 1)
  inline scalar_t sum_of_powers(scalar_t x, size_t f)
  {
    scalar_t result = x;
    for (size_t i = 0; i < f; ++i)
    {
      result.assign_muladd(result, x, result);
      x *= x;
    }
    return result;
  }

  // returns least significant bit uing de Bruijn sequence
  // http://graphics.stanford.edu/~seander/bithacks.html
  inline uint8_t calc_lsb_32(uint32_t v)
  {
    static const uint8_t multiply_de_bruijn_bit_position[32] =
    {
      0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
      31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
    };
    return multiply_de_bruijn_bit_position[((uint32_t)((v & -(int32_t)v) * 0x077CB531U)) >> 27];
  }

  
  ////////////////////////////////////////
  // crypto trait for Zano
  ////////////////////////////////////////
  struct bpp_ct_generators_HGX
  {
    // NOTE! This notation follows the original BP+ whitepaper, see mapping to Zano's generators in range_proofs.cpp
    static const point_t& bpp_G;
    static const point_t& bpp_H;
    static const point_t& bpp_H2;
  };

  struct bpp_ct_generators_UGX
  {
    // NOTE! This notation follows the original BP+ whitepaper, see mapping to Zano's generators in range_proofs.cpp
    static const point_t& bpp_G;
    static const point_t& bpp_H;
    static const point_t& bpp_H2;
  };


  template<typename gen_trait_t, size_t N = 64, size_t values_max = 32>
  struct bpp_crypto_trait_zano : gen_trait_t
  {
    static constexpr size_t c_bpp_n           = N;                           // the upper bound for the witness's range
    static constexpr size_t c_bpp_values_max  = values_max;                  // maximum number of elements in BP+ proof, i.e. max allowed BP+ outputs
    static constexpr size_t c_bpp_log2_n      = constexpr_ceil_log2(c_bpp_n);
    static constexpr size_t c_bpp_mn_max      = c_bpp_n * c_bpp_values_max;

    static void calc_pedersen_commitment(const scalar_t& value, const scalar_t& mask, point_t& commitment)
    {
      // commitment = value * bpp_G + mask * bpp_H
      commitment = operator*(value, bpp_G) + mask * bpp_H;
    }

    static void calc_pedersen_commitment_2(const scalar_t& value, const scalar_t& mask1, const scalar_t& mask2, point_t& commitment)
    {
      // commitment = value * bpp_G + mask1 * bpp_H * mask2 * bpp_H2
      commitment = operator*(value, bpp_G) + mask1 * bpp_H + mask2 * bpp_H2;
    }

    static const scalar_t& get_initial_transcript()
    {
      static scalar_t value = hash_helper_t::hs("Zano BP+ initial transcript");
      return value;
    }

    // assumes hsc is cleared
    static void update_transcript(hash_helper_t::hs_t& hsc, scalar_t& e, const std::vector<point_t>& points)
    {
      hsc.add_scalar(e);
      hsc.add_points_array(points);
      e = hsc.calc_hash();
    }

    // assumes hsc is cleared
    static void update_transcript(hash_helper_t::hs_t& hsc, scalar_t& e, const std::vector<const public_key*>& pub_keys)
    {
      hsc.add_scalar(e);
      for(auto p : pub_keys)
        hsc.add_pub_key(*p);
      e = hsc.calc_hash();
    }

    // TODO: refactor with proper OOB handling
    static const point_t& get_generator(bool select_H, size_t index)
    {
      if (index >= c_bpp_mn_max)
        return c_point_0; // out of bound

      static std::vector<point_t> generators(2 * c_bpp_mn_max);
      static bool calculated = false;
      if (!calculated)
      {
        scalar_t hash_buf[2] = { hash_helper_t::hs("Zano BP+ generator"), 0 };
        for (size_t i = 0; i < 2 * c_bpp_mn_max; ++i)
        {
          hash_buf[1].m_u64[0] = i;
          ge_bytes_hash_to_ec(&generators[i].m_p3, &hash_buf, sizeof hash_buf);
        }
        calculated = true;
      }

      return generators[2 * index + (select_H ? 1 : 0)];
    }

    static const scalar_t& get_2_to_the_power_of_N_minus_1()
    {
      static scalar_t result = scalar_t::power_of_2(c_bpp_n) - 1;
      return result;
    }

    using gen_trait_t::bpp_G;
    using gen_trait_t::bpp_H;
    using gen_trait_t::bpp_H2;
  }; // struct bpp_crypto_trait_zano


  typedef bpp_crypto_trait_zano<bpp_ct_generators_UGX, 64,  32> bpp_crypto_trait_ZC_out;

  typedef bpp_crypto_trait_zano<bpp_ct_generators_HGX, 128, 16> bpp_crypto_trait_Zarcanum;


} // namespace crypto

#include "epee/include/profile_tools.h" // <- remove this, sowle

#include "msm.h"
#include "range_proof_bpp.h"
#include "range_proof_bppe.h"
