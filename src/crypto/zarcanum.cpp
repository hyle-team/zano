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

  #define DBG_VAL_PRINT(x) (void(0)) // std::cout << #x ": " << x << std::endl
  #define DBG_PRINT(x)     (void(0)) // std::cout << x << std::endl


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
    if (!(cond)) { LOG_PRINT_RED("zarcanum_generate_proof: \"" << #cond << "\" is false at " << LOCATION_SS << ENDL << "error code = " << (int)err_code, LOG_LEVEL_3); \
    if (p_err) { *p_err = err_code; } return false; }
  
  bool zarcanum_generate_proof(const hash& m, const hash& kernel_hash, const std::vector<CLSAG_GGXG_input_ref_t>& ring,
    const scalar_t& last_pow_block_id_hashed, const key_image& stake_ki,
    const scalar_t& secret_x, const scalar_t& secret_q, uint64_t secret_index, const scalar_t& pseudo_out_blinding_mask, uint64_t stake_amount, const scalar_t& stake_blinding_mask,
    zarcanum_proof& result, uint8_t* p_err /* = nullptr */)
  {
    DBG_PRINT("zarcanum_generate_proof");
    const scalar_t a = stake_amount;
    const scalar_t h = scalar_t(kernel_hash);
    const scalar_t f_plus_q = stake_blinding_mask + secret_q;
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

    result.C        = (c_scalar_1div8 * C).to_public_key();
    result.C_prime  = (c_scalar_1div8 * C_prime).to_public_key();
    result.E        = (c_scalar_1div8 * E).to_public_key();

    // three proofs with a shared Fiat-Shamir challenge c
    // 1) linear composition proof for the fact, that  C + C' = lin(X, H + G) = (x + x') X + (a + f + q) (H + G)
    // 2) linear composition proof for the fact, that  C - C' = lin(X, H - G) = (x - x') X + (a - f - q) (H - G)
    // 3) Schnorr proof for the fact, that             hC' - dzC + E + f'hH = lin(X) = x'' X

    point_t F = h * C_prime - dz * C + E + last_pow_block_id_hashed * h * c_point_H;

    DBG_VAL_PRINT(h); DBG_VAL_PRINT(last_pow_block_id_hashed); DBG_VAL_PRINT(dz);
    DBG_VAL_PRINT(C); DBG_VAL_PRINT(C_prime); DBG_VAL_PRINT(E); DBG_VAL_PRINT(F);

    scalar_t r0 = scalar_t::random();
    scalar_t r1 = scalar_t::random();
    scalar_t r2 = scalar_t::random();
    scalar_t r3 = scalar_t::random();
    scalar_t r4 = scalar_t::random();

    point_t R_01 = r0 * c_point_X + r1 * c_point_H_plus_G;
    point_t R_23 = r2 * c_point_X + r3 * c_point_H_minus_G;
    point_t R_4  = r4 * c_point_X;

    hash_helper_t::hs_t hash_calc(7);
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
    const scalar_vec_t values = { ba }; // H component
    const scalar_vec_t masks  = { bf }; // G component
    const scalar_vec_t masks2 = { bx }; // X component
    const std::vector<const public_key*> E_1div8_vec_ptr = { &result.E };
    
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(bppe_gen<bpp_crypto_trait_zano<128>>(values, masks, masks2, E_1div8_vec_ptr, result.E_range_proof), 10);

    // = four-layers ring signature data outline =
    // (j in [0, ring_size-1])
    // layer 0 ring
    //     A[j] ( = ring[j].stealth_address)
    // layer 0 secret (with respect to G)
    //     secret_x
    // layer 0 linkability
    //     stake_ki
    //
    // layer 1 ring
    //     ring[j].amount_commitment - pseudo_out_amount_commitment
    // layer 1 secret (with respect to G)
    //     stake_blinding_mask - pseudo_out_blinding_mask
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

    point_t pseudo_out_amount_commitment = a * crypto::c_point_H + pseudo_out_blinding_mask * crypto::c_point_G;
    result.pseudo_out_amount_commitment = (crypto::c_scalar_1div8 * pseudo_out_amount_commitment).to_public_key();

    TRY_ENTRY()
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(generate_CLSAG_GGXG(m, ring, pseudo_out_amount_commitment, C, stake_ki,
      secret_x, stake_blinding_mask - pseudo_out_blinding_mask, x0, secret_q, secret_index,
      result.clsag_ggxg), 20);
    CATCH_ENTRY2(false);

    return true;
  }
  #undef CHECK_AND_FAIL_WITH_ERROR_IF_FALSE



  #define CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(cond, err_code) \
    if (!(cond)) { LOG_PRINT_RED("zarcanum_verify_proof: \"" << #cond << "\" is false at " << LOCATION_SS << ENDL << "error code = " << (int)err_code, LOG_LEVEL_3); \
    if (p_err) { *p_err = err_code; } return false; }

  bool zarcanum_verify_proof(const hash& m, const hash& kernel_hash, const std::vector<CLSAG_GGXG_input_ref_t>& ring,
    const scalar_t& last_pow_block_id_hashed, const key_image& stake_ki,
    const mp::uint128_t& pos_difficulty,
    const zarcanum_proof& sig, uint8_t* p_err /* = nullptr */)
  {
    DBG_PRINT("zarcanum_verify_proof");
    bool r = false;

    // make sure 0 < d <= l / floor(z * D)
    const mp::uint256_t l_div_z_D_mp = crypto::zarcanum_precalculate_l_div_z_D(pos_difficulty);
    const scalar_t l_div_z_D(l_div_z_D_mp);
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(!sig.d.is_zero() && sig.d < l_div_z_D, 2);
    const scalar_t dz = sig.d * c_zarcanum_z_coeff_s;

    // calculate h
    const scalar_t h = scalar_t(kernel_hash);

    // calculate F
    point_t C_prime = point_t(sig.C_prime);
    C_prime.modify_mul8();
    point_t C = point_t(sig.C);
    C.modify_mul8();
    point_t E = point_t(sig.E);
    E.modify_mul8();
    point_t F = h * C_prime - dz * C + E + last_pow_block_id_hashed * h * c_point_H;

    DBG_VAL_PRINT(h); DBG_VAL_PRINT(last_pow_block_id_hashed); DBG_VAL_PRINT(dz);
    DBG_VAL_PRINT(C); DBG_VAL_PRINT(C_prime); DBG_VAL_PRINT(E); DBG_VAL_PRINT(F);

    // check three proofs with a shared Fiat-Shamir challenge c
    point_t C_plus_C_prime  = C + C_prime;
    point_t C_minus_C_prime = C - C_prime;
    hash_helper_t::hs_t hash_calc(7);
    hash_calc.add_32_chars(CRYPTO_HDS_ZARCANUM_PROOF_HASH);
    hash_calc.add_point(sig.y0 * c_point_X + sig.y1 * c_point_H_plus_G - sig.c * C_plus_C_prime);             // y_0 * X + y1 (H + G) - c (C + C')
    hash_calc.add_point(sig.y2 * c_point_X + sig.y3 * c_point_H_minus_G - sig.c * C_minus_C_prime);           // y_2 * X + y3 (H - G) - c (C - C')
    hash_calc.add_point(sig.y4 * c_point_X - sig.c * F);                                                      // y_4 * X - c * F
    hash_calc.add_point(C_plus_C_prime);
    hash_calc.add_point(C_minus_C_prime);
    hash_calc.add_point(F);
    scalar_t c_prime = hash_calc.calc_hash();
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(sig.c == c_prime, 3);

    // check extended range proof for E
    std::vector<point_t> E_for_range_proof = { point_t(sig.E) }; // consider changing to 8*sig.E to avoid additional conversion
    std::vector<bppe_sig_commit_ref_t> range_proofs = { bppe_sig_commit_ref_t(sig.E_range_proof, E_for_range_proof) };
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(bppe_verify<bpp_crypto_trait_zano<128>>(range_proofs), 10);

    // check extended CLSAG-GGXG ring signature
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(verify_CLSAG_GGXG(m, ring, sig.pseudo_out_amount_commitment, sig.C, stake_ki, sig.clsag_ggxg), 1);

    return true;
  }

  #undef CHECK_AND_FAIL_WITH_ERROR_IF_FALSE



} // namespace crypto
