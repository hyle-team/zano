// Copyright (c) 2022 Zano Project
// Copyright (c) 2022 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Note: This file originates from tests/functional_tests/crypto_tests.cpp 
#include "epee/include/misc_log_ex.h"
#include "zarcanum.h"
#include "crypto/range_proofs.h"
#include "../currency_core/crypto_config.h" // TODO: move it to the crypto

namespace crypto
{
  const scalar_t      c_zarcanum_z_coeff_s  = { 0,                  1,                  0,                  0                  }; // c_scalar_2p64
  const mp::uint256_t c_zarcanum_z_coeff_mp = c_zarcanum_z_coeff_s.as_boost_mp_type<mp::uint256_t>();

  mp::uint256_t zarcanum_precalculate_l_div_z_D(const mp::uint128_t& pos_difficulty)
  {
    //LOG_PRINT_GREEN_L0(ENDL << "floor( l / (z * D) ) =     " << c_scalar_L.as_boost_mp_type<mp::uint256_t>() / (c_zarcanum_z_coeff_mp * pos_difficulty));
    return c_scalar_L.as_boost_mp_type<mp::uint256_t>() / (c_zarcanum_z_coeff_mp * pos_difficulty); // == floor( l / (z * D) )
  }

  mp::uint256_t zarcanum_precalculate_z_l_div_z_D(const mp::uint128_t& pos_difficulty)
  {
    //LOG_PRINT_GREEN_L0(ENDL << "z * floor( l / (z * D) ) = " << c_zarcanum_z_coeff_mp * (c_scalar_L.as_boost_mp_type<mp::uint256_t>() / (c_zarcanum_z_coeff_mp * pos_difficulty)));
    return c_zarcanum_z_coeff_mp * (c_scalar_L.as_boost_mp_type<mp::uint256_t>() / (c_zarcanum_z_coeff_mp * pos_difficulty)); // == z * floor( l / (z * D) )
  }

  bool zarcanum_check_main_pos_inequality(const hash& kernel_hash, const scalar_t& blinding_mask, const scalar_t& secret_q,
    const scalar_t& last_pow_block_id_hashed, const mp::uint256_t& z_l_div_z_D, uint64_t stake_amount, mp::uint256_t& lhs, mp::uint512_t& rhs)
  {
    scalar_t lhs_s = scalar_t(kernel_hash) * (blinding_mask + secret_q + last_pow_block_id_hashed); // == h * (f + q + f') mod l
    lhs = lhs_s.as_boost_mp_type<mp::uint256_t>();
    rhs = static_cast<mp::uint512_t>(z_l_div_z_D) * stake_amount; // == floor( l / (z * D) ) * z * a

    //LOG_PRINT_GREEN_L0(ENDL << 
    //  "z_l_div_z_D =              " << z_l_div_z_D << ENDL <<
    //  "stake_amount =             " << stake_amount << ENDL <<
    //  "lhs =                      " << lhs << ENDL <<
    //  "rhs =                      " << rhs);

    return lhs < rhs;  //  h * (f + q + f') mod l   <   floor( l / (z * D) ) * z * a
  }

  #define CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(cond, err_code) \
    if (!(cond)) { LOG_PRINT_RED("zarcanum_generate_proof: \"" << #cond << "\" is false at " << LOCATION_SS << ENDL << "error code = " << err_code, LOG_LEVEL_3); \
    if (p_err) { *p_err = err_code; } return false; }

  bool zarcanum_generate_proof(const hash& kernel_hash, const public_key& commitment_1div8, const scalar_t& blinding_mask, const scalar_t& secret_q,
    const scalar_t& last_pow_block_id_hashed, uint64_t stake_amount, zarcanum_proof& result, uint8_t* p_err)
  {
    const scalar_t a = stake_amount;
    const scalar_t h = scalar_t(kernel_hash);
    const scalar_t f_plus_q = blinding_mask + secret_q;
    const scalar_t f_plus_q_plus_fp = f_plus_q + last_pow_block_id_hashed;
    const scalar_t lhs = h * f_plus_q_plus_fp;                                                                // == h * (f + q + f') mod l
    const mp::uint256_t d_mp = lhs.as_boost_mp_type<mp::uint256_t>() / (c_zarcanum_z_coeff_mp * stake_amount) + 1;
    result.d = scalar_t(d_mp);

    const scalar_t dz = result.d * c_zarcanum_z_coeff_s;

    const scalar_t ba = dz * a - lhs;                                                                         // b_a = dza - h(f + q + f')

    const scalar_t bf = dz * f_plus_q - h * a;                                                                // b_f = dz(f + q) - ha

    const scalar_t x0 = scalar_t::random(), x1 = scalar_t::random(), x2 = scalar_t::random();

    const scalar_t bx = x2 - h * x1 + dz * x0;                                                                // b_x = x'' - hx' + dzx

    point_t C       = x0 * c_point_X + a            * c_point_H + f_plus_q     * c_point_G;
    point_t C_prime = x1 * c_point_X + f_plus_q     * c_point_H + a            * c_point_G;
    point_t E       = bx * c_point_X +           ba * c_point_H +           bf * c_point_G;

    result.C        = C.to_public_key();
    result.C_prime  = C_prime.to_public_key();
    result.E        = E.to_public_key();

    // three proofs with a shared Fiat-Shamir challenge c
    // 1) linear composition proof for the fact, that  C + C' = lin(X, H + G) = (x + x') X + (a + f + q) (H + G)
    // 2) linear composition proof for the fact, that  C - C' = lin(X, H - G) = (x - x') X + (a - f - q) (H - G)
    // 3) Schnorr proof for the fact, that             hC' - dzC + E + f'hH = lin(X) = x'' X

    point_t F = h * C_prime - dz * C + E + last_pow_block_id_hashed * h * c_point_H;

    scalar_t r0 = scalar_t::random();
    scalar_t r1 = scalar_t::random();
    scalar_t r2 = scalar_t::random();
    scalar_t r3 = scalar_t::random();
    scalar_t r4 = scalar_t::random();

    point_t R_01 = r0 * c_point_X + r1 * c_point_H_plus_G;
    point_t R_23 = r2 * c_point_X + r3 * c_point_H_minus_G;
    point_t R_4  = r4 * c_point_G;

    hash_helper_t::hs_t hash_calc(3);
    hash_calc.add_32_chars(CRYPTO_HDS_ZARCANUM_PROOF_HASH);
    hash_calc.add_point(R_01);
    hash_calc.add_point(R_23);
    hash_calc.add_point(R_4);
    hash_calc.add_point(C + C_prime);
    hash_calc.add_point(C - C_prime);
    hash_calc.add_point(F);
    result.c = hash_calc.calc_hash();

    result.y0 = r0 + result.c * (x0 + x1);                                                                    // y_0 = r_0 + c (x + x')
    result.y1 = r1 + result.c * (a + f_plus_q);                                                               // y_1 = r_1 + c (a + f + q)
    result.y2 = r2 + result.c * (x0 - x1);                                                                    // y_2 = r_2 + c (x - x')
    result.y3 = r3 + result.c * (a - f_plus_q);                                                               // y_3 = r_3 + c (a - f - q)
    result.y4 = r4 + result.c * x2;                                                                           // y_4 = r_4 + c x''

    // range proof for E
    const scalar_vec_t values = { a };  // H component
    const scalar_vec_t masks  = { bf }; // G component
    const scalar_vec_t masks2 = { bx }; // X component
    const std::vector<const public_key*> commitments_1div8 = { &commitment_1div8 };
    
    if (!bppe_gen<bpp_crypto_trait_zano<>>(values, masks, masks2, commitments_1div8, result.E_range_proof, p_err))
    {
      return false;
    }

    // = four-layers ring signature data outline =
    // (j in [0, ring_size-1])
    // layer 0 ring
    //     se.outputs[j].stealth_address;
    // layer 0 secret (with respect to G)
    //     in_contexts[i].in_ephemeral.sec;
    // layer 0 linkability
    //     in.k_image;
    //
    // layer 1 ring
    //     crypto::point_t(se.outputs[j].amount_commitment) - pseudo_out_amount_commitment;
    // layer 1 secret (with respect to G)
    //     se.real_out_amount_blinding_mask - blinding_mask;
    //
    // additional layers for Zarcanum:
    //
    // layer 2 ring
    //     C - A[j] - Q[j]
    // layer 2 secret (with respect to X)
    //     x0
    //
    // layer 3 ring
    //     Q[j]
    // layer 3 secret (with respect to G)
    //     secret_q

      return true;
  }


  bool zarcanum_verify_proof(const hash& kernel_hash, const public_key& commitment_1div8, const scalar_t& last_pow_block_id_hashed, const zarcanum_proof& proof, uint8_t* p_err /* = nullptr */)
  {
    return false;
  }



} // namespace crypto
