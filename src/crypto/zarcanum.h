// Copyright (c) 2022-2024 Zano Project
// Copyright (c) 2022-2024 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Note: This file originates from tests/functional_tests/crypto_tests.cpp 
#pragma once
#include "crypto-sugar.h"
#include "range_proofs.h"
#include "clsag.h"

namespace crypto
{
  extern const mp::uint256_t c_zarcanum_z_coeff_mp;
  extern const scalar_t      c_zarcanum_z_coeff_s;

  mp::uint256_t zarcanum_precalculate_l_div_z_D(const mp::uint128_t& pos_difficulty);
  mp::uint256_t zarcanum_precalculate_z_l_div_z_D(const mp::uint128_t& pos_difficulty);

  bool zarcanum_check_main_pos_inequality(const hash& kernel_hash, const scalar_t& blinding_mask, const scalar_t& secret_q,
  const scalar_t& last_pow_block_id_hashed, const mp::uint256_t& z_l_div_z_D_, uint64_t stake_amount, mp::uint256_t& lhs, mp::uint512_t& rhs);


  struct zarcanum_proof
  {
    scalar_t    d         = 0;
    public_key  C;                // premultiplied by 1/8
    public_key  C_prime;          // premultiplied by 1/8
    public_key  E;                // premultiplied by 1/8

    scalar_t    c;                // shared Fiat-Shamir challenge for the following three proofs
    scalar_t    y0;               // 1st linear composition proof
    scalar_t    y1;               //     ( C + C' = lin(X, H + G) )
    scalar_t    y2;               // 2nd linear composition proof
    scalar_t    y3;               //     ( C - C' = lin(X, H - G) )
    scalar_t    y4;               // Schnorr proof (F = lin(X))
    
    bppe_signature E_range_proof;

    public_key pseudo_out_amount_commitment; // premultiplied by 1/8
    CLSAG_GGXXG_signature clsag_ggxxg;
  };

  bool zarcanum_generate_proof(const hash& m, const hash& kernel_hash, const std::vector<CLSAG_GGXXG_input_ref_t>& ring,
    const scalar_t& last_pow_block_id_hashed, const key_image& stake_ki,
    const scalar_t& secret_x, const scalar_t& secret_q, uint64_t secret_index, uint64_t stake_amount,
    const scalar_t& stake_out_asset_id_blinding_mask, const scalar_t& stake_out_amount_blinding_mask, const scalar_t& pseudo_out_amount_blinding_mask,
    zarcanum_proof& result, uint8_t* p_err = nullptr);
  

  bool zarcanum_verify_proof(const hash& m, const hash& kernel_hash, const std::vector<CLSAG_GGXXG_input_ref_t>& ring,
    const scalar_t& last_pow_block_id_hashed, const key_image& stake_ki,
    const mp::uint128_t& pos_difficulty,
    const zarcanum_proof& sig, uint8_t* p_err = nullptr) noexcept;


  // TODO @#@#: make sure it is used, implement, then move it to an appropriate place
  struct linear_composition_proof
  {
    scalar_t  c;
    scalar_t  y0;
    scalar_t  y1;
  };

  enum generator_tag { gt_void = 0, gt_G = 1, gt_H = 2, gt_H2 = 3, gt_X = 4, gt_U = 5 };

  template<generator_tag gen0 = gt_H, generator_tag gen1 = gt_G>
  bool generate_linear_composition_proof(const hash& m, const public_key& A, const scalar_t& secret_a, const scalar_t& secret_b, linear_composition_proof& result, uint8_t* p_err = nullptr)
  {
    // consider embedding generators' tags into random entropy to distinguish proofs made with different generators during verification
    return false;
  }

  template<generator_tag gen0 = gt_H, generator_tag gen1 = gt_G>
  bool verify_linear_composition_proof(const hash& m, const public_key& A, const linear_composition_proof& sig, uint8_t* p_err = nullptr)
  {
    return false;
  }

  
  struct generic_schnorr_sig
  {
    scalar_t  c;
    scalar_t  y;
  };


  template<typename generator_t>
  inline bool generate_schnorr_sig_custom_generator(const hash& m, const point_t& A, const scalar_t& secret_a, generic_schnorr_sig& result, const generator_t& g_point_g)
  {
#ifndef NDEBUG
    if (A != secret_a * g_point_g)
      return false;
#endif
    scalar_t r = scalar_t::random();
    point_t R = r * g_point_g;
    hash_helper_t::hs_t hsc(3);
    hsc.add_hash(m);
    hsc.add_point(A);
    hsc.add_point(R);
    result.c = hsc.calc_hash();
    result.y.assign_mulsub(result.c, secret_a, r); // y = r - c * secret_a
    return true;
  }

  template<generator_tag gen = gt_G>
  inline bool generate_schnorr_sig(const hash& m, const point_t& A, const scalar_t& secret_a, generic_schnorr_sig& result);

  template<>
  inline bool generate_schnorr_sig<gt_G>(const hash& m, const point_t& A, const scalar_t& secret_a, generic_schnorr_sig& result)
  {
    return generate_schnorr_sig_custom_generator(m, A, secret_a, result, c_point_G);
  }

  template<>
  inline bool generate_schnorr_sig<gt_X>(const hash& m, const point_t& A, const scalar_t& secret_a, generic_schnorr_sig& result)
  {
    return generate_schnorr_sig_custom_generator(m, A, secret_a, result, c_point_X);
  }

  inline bool generate_schnorr_sig(const hash& m, const public_key& A, const secret_key& secret_a, generic_schnorr_sig& result)
  {
    return generate_schnorr_sig(m, point_t(A), scalar_t(secret_a), result);
  }

  inline bool generate_schnorr_sig(const hash& m, const secret_key& secret_a, generic_schnorr_sig& result)
  {
    scalar_t secret_a_s(secret_a);
    if (!secret_a_s.is_reduced())
      return false;
    point_t A = secret_a_s * c_point_G;
    return generate_schnorr_sig_custom_generator(m, A, secret_a_s, result, c_point_G);
  }


  template<generator_tag gen = gt_G>
  inline bool verify_schnorr_sig(const hash& m, const public_key& A, const generic_schnorr_sig& sig) noexcept;

  // TODO @#@# make optimized version   inline bool verify_schnorr_sig(const hash& m, const point_t& A, const generic_schnorr_sig& sig) noexcept;
  // and change check_tx_balance() accordingly

  template<>
  inline bool verify_schnorr_sig<gt_G>(const hash& m, const public_key& A, const generic_schnorr_sig& sig) noexcept
  {
    try
    {
      if (!sig.c.is_reduced() || !sig.y.is_reduced())
        return false;
      hash_helper_t::hs_t hsc(3);
      hsc.add_hash(m);
      hsc.add_pub_key(A);
      hsc.add_point(point_t(A).mul_plus_G(sig.c, sig.y)); // sig.y * G + sig.c * A
      return sig.c == hsc.calc_hash();
    }
    catch(...)
    {
      return false;
    }
  }

  template<>
  inline bool verify_schnorr_sig<gt_X>(const hash& m, const public_key& A, const generic_schnorr_sig& sig) noexcept
  {
    try
    {
      if (!sig.c.is_reduced() || !sig.y.is_reduced())
        return false;
      hash_helper_t::hs_t hsc(3);
      hsc.add_hash(m);
      hsc.add_pub_key(A);
      hsc.add_point(sig.y * c_point_X + sig.c * point_t(A));
      return sig.c == hsc.calc_hash();
    }
    catch(...)
    {
      return false;
    }
  }

  // --------------------------------------------

  // multi-base Schnorr-like proof (two generators, two secrets, one Fiat-Shamir challenge) 
  struct generic_double_schnorr_sig
  {
    scalar_t  c;
    scalar_t  y0;
    scalar_t  y1;
  };

  template<generator_tag gen0, generator_tag gen1>
  inline bool generate_double_schnorr_sig(const hash& m, const point_t& A, const scalar_t& secret_a, const point_t& B, const scalar_t& secret_b, generic_double_schnorr_sig& result);

  template<>
  inline bool generate_double_schnorr_sig<gt_G, gt_G>(const hash& m, const point_t& A, const scalar_t& secret_a, const point_t& B, const scalar_t& secret_b, generic_double_schnorr_sig& result)
  {
#ifndef NDEBUG
    if (A != secret_a * c_point_G || B != secret_b * c_point_G)
      return false;
#endif
    scalar_t r0 = scalar_t::random();
    scalar_t r1 = scalar_t::random();
    point_t R0 = r0 * c_point_G;
    point_t R1 = r1 * c_point_G;
    hash_helper_t::hs_t hsc(5);
    hsc.add_hash(m);
    hsc.add_point(A);
    hsc.add_point(B);
    hsc.add_point(R0);
    hsc.add_point(R1);
    result.c = hsc.calc_hash();
    result.y0.assign_mulsub(result.c, secret_a, r0); // y0 = r0 - c * secret_a
    result.y1.assign_mulsub(result.c, secret_b, r1); // y1 = r1 - c * secret_b
    return true;
  }

  template<>
  inline bool generate_double_schnorr_sig<gt_X, gt_G>(const hash& m, const point_t& A, const scalar_t& secret_a, const point_t& B, const scalar_t& secret_b, generic_double_schnorr_sig& result)
  {
#ifndef NDEBUG
    if (A != secret_a * c_point_X || B != secret_b * c_point_G)
      return false;
#endif
    scalar_t r0 = scalar_t::random();
    scalar_t r1 = scalar_t::random();
    point_t R0 = r0 * c_point_X;
    point_t R1 = r1 * c_point_G;
    hash_helper_t::hs_t hsc(5);
    hsc.add_hash(m);
    hsc.add_point(A);
    hsc.add_point(B);
    hsc.add_point(R0);
    hsc.add_point(R1);
    result.c = hsc.calc_hash();
    result.y0.assign_mulsub(result.c, secret_a, r0); // y0 = r0 - c * secret_a
    result.y1.assign_mulsub(result.c, secret_b, r1); // y1 = r1 - c * secret_b
    return true;
  }

  template<generator_tag gen0, generator_tag gen1>
  inline bool verify_double_schnorr_sig(const hash& m, const point_t& A, const public_key& B, const generic_double_schnorr_sig& sig) noexcept;

  template<>
  inline bool verify_double_schnorr_sig<gt_G, gt_G>(const hash& m, const point_t& A, const public_key& B, const generic_double_schnorr_sig& sig) noexcept
  {
    try
    {
      if (!sig.c.is_reduced() || !sig.y0.is_reduced() || !sig.y1.is_reduced())
        return false;
      hash_helper_t::hs_t hsc(5);
      hsc.add_hash(m);
      hsc.add_point(A);
      hsc.add_pub_key(B);
      hsc.add_point(A.mul_plus_G(sig.c, sig.y0));          // sig.y0 * G + sig.c * A
      hsc.add_point(point_t(B).mul_plus_G(sig.c, sig.y1)); // sig.y1 * G + sig.c * B
      return sig.c == hsc.calc_hash();
    }
    catch(...)
    {
      return false;
    }
  }

  template<>
  inline bool verify_double_schnorr_sig<gt_X, gt_G>(const hash& m, const point_t& A, const public_key& B, const generic_double_schnorr_sig& sig) noexcept
  {
    try
    {
      if (!sig.c.is_reduced() || !sig.y0.is_reduced() || !sig.y1.is_reduced())
        return false;
      hash_helper_t::hs_t hsc(5);
      hsc.add_hash(m);
      hsc.add_point(A);
      hsc.add_pub_key(B);
      hsc.add_point(sig.y0 * c_point_X + sig.c * A);
      hsc.add_point(point_t(B).mul_plus_G(sig.c, sig.y1)); // sig.y1 * G + sig.c * B
      return sig.c == hsc.calc_hash();
    }
    catch(...)
    {
      return false;
    }
  }


  // TODO: improve this proof using random weightning factor
  struct vector_UG_aggregation_proof
  {
    std::vector<public_key> amount_commitments_for_rp_aggregation; // E' = e * U + y' * G, premultiplied by 1/8
    scalar_vec_t y0s;
    scalar_vec_t y1s;
    scalar_t c; // common challenge
  };

  bool generate_vector_UG_aggregation_proof(const hash& m, const scalar_vec_t& u_secrets, const scalar_vec_t& g_secrets0, const scalar_vec_t& g_secrets1,
    const std::vector<point_t>& amount_commitments,
    const std::vector<point_t>& amount_commitments_for_rp_aggregation, 
    const std::vector<point_t>& blinded_asset_ids, 
    vector_UG_aggregation_proof& result, uint8_t* p_err = nullptr);

  bool verify_vector_UG_aggregation_proof(const hash& m, const std::vector<const public_key*> amount_commitments_1div8, const std::vector<const public_key*> blinded_asset_ids_1div8,
    const vector_UG_aggregation_proof& sig, uint8_t* p_err = nullptr) noexcept;


} // namespace crypto