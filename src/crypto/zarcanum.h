// Copyright (c) 2022 Zano Project
// Copyright (c) 2022 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Note: This file originates from tests/functional_tests/crypto_tests.cpp 
#pragma once
#include "crypto-sugar.h"
#include "crypto/range_proofs.h"
#include "crypto/clsag.h"
#include <boost/multiprecision/cpp_int.hpp>

namespace crypto
{
  namespace mp = boost::multiprecision;

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

    crypto::public_key pseudo_out_amount_commitment; // premultiplied by 1/8
    CLSAG_GGXG_signature clsag_ggxg;
  };

  bool zarcanum_generate_proof(const hash& m, const hash& kernel_hash, const std::vector<crypto::CLSAG_GGXG_input_ref_t>& ring,
    const scalar_t& last_pow_block_id_hashed, const key_image& stake_ki,
    const scalar_t& secret_x, const scalar_t& secret_q, uint64_t secret_index, const scalar_t& pseudo_out_blinding_mask, uint64_t stake_amount, const scalar_t& stake_blinding_mask,
    zarcanum_proof& result, uint8_t* p_err = nullptr);
  

  bool zarcanum_verify_proof(const hash& m, const hash& kernel_hash, const std::vector<crypto::CLSAG_GGXG_input_ref_t>& ring,
    const scalar_t& last_pow_block_id_hashed, const key_image& stake_ki,
    const mp::uint128_t& pos_difficulty,
    const zarcanum_proof& sig, uint8_t* p_err = nullptr);



  // TODO: improve this proof using random weightning factor
  struct vector_UG_aggregation_proof
  {
    std::vector<public_key> amount_commitments_for_rp_aggregation; // E' = e * U + y' * G, premultiplied by 1/8
    scalar_vec_t y0s;
    scalar_vec_t y1s;
    scalar_t c; // common challenge
  };

  bool generate_vector_UG_aggregation_proof(const hash& m, const scalar_vec_t& u_secrets, const scalar_vec_t& g_secrets,
    const std::vector<crypto::point_t>& amount_commitments,
    const std::vector<crypto::point_t>& amount_commitments_for_rp_aggregation, 
    const std::vector<crypto::point_t>& blinded_asset_ids, 
    vector_UG_aggregation_proof& result, uint8_t* p_err = nullptr)
  {
    // proof of knowing e_j and y'' in zero knowledge in the following eq:
    //   E_j + E'_j = e_j * (T'_j + U) + y'' * G
    // where:
    //   e_j   -- output's amount
    //   T'_j  -- output's blinded asset tag
    //   E_j   == e_j * T'_j + y_j  * G -- output's amount commitments
    //   E'_j  == e_j * U    + y'_j * G -- additional commitment to the same amount for range proof aggregation

    // amount_commitments[j] + amount_commitments_for_rp_aggregation[j]
    //   ==
    // u_secrets[j] * (blinded_asset_ids[j] + U) + g_secrets[j] * G


    return false;
  }


  bool verify_vector_UG_aggregation_proof(const hash& m, const std::vector<const public_key*> amount_commitments, const std::vector<const public_key*> blinded_asset_ids,
    const vector_UG_aggregation_proof& sig, uint8_t* p_err = nullptr)
  {
    return false;
  }


} // namespace crypto
