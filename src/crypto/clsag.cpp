// Copyright (c) 2022-2024 Zano Project
// Copyright (c) 2022-2024 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// This file contains implementation of the original d-CLSAG (s.a. https://eprint.iacr.org/2019/654.pdf by Goodel at el)
// and the extended d/v-CLSAG version (s.a. https://github.com/hyle-team/docs/blob/master/zano/dv-CLSAG-extension/ by sowle)
//
#include "clsag.h"
//#include "misc_log_ex.h"
#include "../currency_core/crypto_config.h"

DISABLE_GCC_AND_CLANG_WARNING(unused-function)

namespace crypto
{
#if 0
#  define DBG_VAL_PRINT(x) std::cout << std::setw(30) << std::left << #x ": " << x << std::endl
#  define DBG_PRINT(x)     std::cout << x << std::endl
#else
#  define DBG_VAL_PRINT(x) (void(0))
#  define DBG_PRINT(x)     (void(0))
#endif

  static std::ostream &operator <<(std::ostream &o, const crypto::hash &v)       { return o << pod_to_hex(v); }
  static std::ostream &operator <<(std::ostream &o, const crypto::public_key &v) { return o << pod_to_hex(v); }

  bool generate_CLSAG_GG(const hash& m, const std::vector<CLSAG_GG_input_ref_t>& ring, const point_t& pseudo_out_amount_commitment, const key_image& ki,
    const scalar_t& secret_x, const scalar_t& secret_f, uint64_t secret_index, CLSAG_GG_signature& sig)
  {
    DBG_PRINT("generate_CLSAG_GG");
    size_t ring_size = ring.size();
    CRYPTO_CHECK_AND_THROW_MES(ring_size > 0, "ring size is zero");
    CRYPTO_CHECK_AND_THROW_MES(secret_index < ring_size, "secret_index is out of range");

    // calculate key images
    point_t ki_base = hash_helper_t::hp(ring[secret_index].stealth_address);
    point_t key_image = secret_x * ki_base;
    CRYPTO_CHECK_AND_THROW_MES(key_image == point_t(ki), "key image 0 mismatch");
    point_t K1_div8 = (c_scalar_1div8 * secret_f) * ki_base;
    K1_div8.to_public_key(sig.K1);
    point_t K1 = K1_div8;
    K1.modify_mul8();

    // calculate aggregation coefficients
    hash_helper_t::hs_t hsc(3 + 2  * ring_size);
    hsc.add_scalar(m);
    for(size_t i = 0; i < ring_size; ++i)
    {
      hsc.add_pub_key(ring[i].stealth_address);
      hsc.add_pub_key(ring[i].amount_commitment);
    }
    hsc.add_point(c_scalar_1div8 * pseudo_out_amount_commitment);
    hsc.add_key_image(ki);
    hsc.add_pub_key(sig.K1);
    hash input_hash = hsc.calc_hash_no_reduce();

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GG_LAYER_0);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_0 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_0);

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GG_LAYER_1);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_1 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_1);

    // calculate aggregate pub keys
    std::vector<point_t> W_pub_keys;
    W_pub_keys.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      W_pub_keys.emplace_back(agg_coeff_0 * point_t(ring[i].stealth_address) + agg_coeff_1 * (point_t(ring[i].amount_commitment).modify_mul8() - pseudo_out_amount_commitment));
      DBG_VAL_PRINT(W_pub_keys[i]);
    }

    // aggregate secret key
    scalar_t w_sec_key = agg_coeff_0 * secret_x + agg_coeff_1 * secret_f;

    // calculate aggregate key image
    point_t W_key_image = agg_coeff_0 * key_image + agg_coeff_1 * K1;
    DBG_VAL_PRINT(W_key_image);

    // initial commitment
    scalar_t alpha = scalar_t::random();
    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GG_CHALLENGE);
    hsc.add_hash(input_hash);
    hsc.add_point(alpha * c_point_G);
    hsc.add_point(alpha * ki_base);
    scalar_t c_prev = hsc.calc_hash();  // c_{secret_index + 1}

    sig.r.resize_and_make_random(ring_size);

    for(size_t j = 0, i = (secret_index + 1) % ring_size; j < ring_size - 1; ++j, i = (i + 1) % ring_size)
    {
      if (i == 0)
        sig.c = c_prev; // c_0
      hsc.add_32_chars(CRYPTO_HDS_CLSAG_GG_CHALLENGE);
      hsc.add_hash(input_hash);
      hsc.add_point(sig.r[i] * c_point_G + c_prev * W_pub_keys[i]);
      hsc.add_point(sig.r[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image);
      c_prev = hsc.calc_hash(); // c_{i + 1}
    }

    if (secret_index == 0)
      sig.c = c_prev;

    sig.r[secret_index] = alpha - c_prev * w_sec_key;

    return true;
  }



  bool verify_CLSAG_GG(const hash& m, const std::vector<CLSAG_GG_input_ref_t>& ring, const crypto::public_key& pseudo_out_amount_commitment, const key_image& ki,
    const CLSAG_GG_signature& sig)
  {
    DBG_PRINT("verify_CLSAG_GG");
    size_t ring_size = ring.size();
    CRYPTO_CHECK_AND_THROW_MES(ring_size > 0, "ring size is zero");
    CRYPTO_CHECK_AND_THROW_MES(ring_size == sig.r.size(), "ring size != r size");

    point_t key_image(ki);
    CRYPTO_CHECK_AND_THROW_MES(key_image.is_in_main_subgroup(), "key image 0 does not belong to the main subgroup");

    point_t pseudo_out_amount_commitment_pt(pseudo_out_amount_commitment);
    pseudo_out_amount_commitment_pt.modify_mul8();

    // calculate aggregation coefficients
    hash_helper_t::hs_t hsc(3 + 2  * ring_size);
    hsc.add_scalar(m);
    for(size_t i = 0; i < ring_size; ++i)
    {
      hsc.add_pub_key(ring[i].stealth_address);
      hsc.add_pub_key(ring[i].amount_commitment);
    }
    hsc.add_pub_key(pseudo_out_amount_commitment);
    hsc.add_key_image(ki);
    hsc.add_pub_key(sig.K1);
    hash input_hash = hsc.calc_hash_no_reduce();

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GG_LAYER_0);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_0 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_0);

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GG_LAYER_1);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_1 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_1);


    // calculate aggregate pub keys
    std::vector<point_t> W_pub_keys;
    W_pub_keys.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      W_pub_keys.emplace_back(agg_coeff_0 * point_t(ring[i].stealth_address) + agg_coeff_1 * (point_t(ring[i].amount_commitment).modify_mul8() - pseudo_out_amount_commitment_pt));
      DBG_VAL_PRINT(W_pub_keys[i]);
    }

    // calculate aggregate key image
    point_t W_key_image = agg_coeff_0 * key_image + agg_coeff_1 * point_t(sig.K1).modify_mul8();
    DBG_VAL_PRINT(W_key_image);
    
    scalar_t c_prev = sig.c;
    for(size_t i = 0; i < ring_size; ++i)
    {
      hsc.add_32_chars(CRYPTO_HDS_CLSAG_GG_CHALLENGE);
      hsc.add_hash(input_hash);
      hsc.add_point(sig.r[i] * c_point_G + c_prev * W_pub_keys[i]);
      hsc.add_point(sig.r[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image);
      c_prev = hsc.calc_hash(); // c_{i + 1}
    }

    return c_prev == sig.c;
  }


  //---------------------------------------------------------------

  //
  // Disclaimer: extensions to the CLSAG implemented below are non-standard and are in proof-of-concept state.
  // They shouldn't be used in production code until formal security proofs are done and (ideally) the code is peer-reviewed.
  // -- sowle
  //


  bool generate_CLSAG_GGX(const hash& m, const std::vector<CLSAG_GGX_input_ref_t>& ring, const point_t& pseudo_out_amount_commitment, const point_t& pseudo_out_blinded_asset_id, const key_image& ki,
    const scalar_t& secret_0_xp, const scalar_t& secret_1_f, const scalar_t& secret_2_t, uint64_t secret_index, CLSAG_GGX_signature& sig)
  {
    DBG_PRINT("== generate_CLSAG_GGX ==");
    size_t ring_size = ring.size();
    CRYPTO_CHECK_AND_THROW_MES(ring_size > 0, "ring size is zero");
    CRYPTO_CHECK_AND_THROW_MES(secret_index < ring_size, "secret_index is out of range");

    // calculate key images
    point_t ki_base = hash_helper_t::hp(ring[secret_index].stealth_address);
    point_t key_image = secret_0_xp * ki_base;

#ifndef NDEBUG
    CRYPTO_CHECK_AND_THROW_MES(key_image == point_t(ki), "key image 0 mismatch");
    CRYPTO_CHECK_AND_THROW_MES((secret_0_xp * c_point_G).to_public_key() == ring[secret_index].stealth_address, "secret_0_xp mismatch");
    CRYPTO_CHECK_AND_THROW_MES( secret_1_f  * c_point_G == 8 * point_t(ring[secret_index].amount_commitment) - pseudo_out_amount_commitment, "secret_1_f mismatch");
    CRYPTO_CHECK_AND_THROW_MES( secret_2_t  * c_point_X == 8 * point_t(ring[secret_index].blinded_asset_id)  - pseudo_out_blinded_asset_id, "secret_2_t mismatch");
#endif

    point_t K1_div8 = (c_scalar_1div8 * secret_1_f) * ki_base;
    K1_div8.to_public_key(sig.K1);
    point_t K1 = K1_div8;
    K1.modify_mul8();

    point_t K2_div8 = (c_scalar_1div8 * secret_2_t) * ki_base;
    K2_div8.to_public_key(sig.K2);
    point_t K2 = K2_div8;
    K2.modify_mul8();

    // calculate aggregation coefficients
    hash_helper_t::hs_t hsc(4 + 3  * ring_size);
    hsc.add_scalar(m);
    for(size_t i = 0; i < ring_size; ++i)
    {
      hsc.add_pub_key(ring[i].stealth_address);
      hsc.add_pub_key(ring[i].amount_commitment);
      hsc.add_pub_key(ring[i].blinded_asset_id);
      DBG_PRINT("ring[" << i << "]: sa:" << ring[i].stealth_address << ", ac:" << ring[i].amount_commitment << ", baid:" << ring[i].blinded_asset_id);
    }
    hsc.add_point(c_scalar_1div8 * pseudo_out_amount_commitment);
    hsc.add_point(c_scalar_1div8 * pseudo_out_blinded_asset_id);
    hsc.add_key_image(ki);
    hsc.add_pub_key(sig.K1);
    hsc.add_pub_key(sig.K2);
    hash input_hash = hsc.calc_hash_no_reduce();
    DBG_VAL_PRINT(input_hash);

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGX_LAYER_0);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_0 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_0);

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGX_LAYER_1);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_1 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_1);

    // may we get rid of it?
    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGX_LAYER_2);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_2 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_2);

    // prepare A_i, Q_i
    std::vector<point_t> A_i, Q_i;
    A_i.reserve(ring_size), Q_i.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      A_i.emplace_back(ring[i].amount_commitment);
      A_i.back().modify_mul8();
      Q_i.emplace_back(ring[i].blinded_asset_id);
      Q_i.back().modify_mul8();
      DBG_PRINT("A_i[" << i << "] = " << A_i[i] << "  Q_i[" << i << "] = " << Q_i[i]);
    }

    // calculate aggregate pub keys (layers 0, 1; G components)
    std::vector<point_t> W_pub_keys_g;
    W_pub_keys_g.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      W_pub_keys_g.emplace_back(
        agg_coeff_0 * point_t(ring[i].stealth_address) +
        agg_coeff_1 * (A_i[i] - pseudo_out_amount_commitment)
      );
      DBG_VAL_PRINT(W_pub_keys_g[i]);
    }

    // calculate aggregate pub keys (layer 2; X component)
    std::vector<point_t> W_pub_keys_x;
    W_pub_keys_x.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      W_pub_keys_x.emplace_back(
        agg_coeff_2 * (Q_i[i] - pseudo_out_blinded_asset_id)
      );
      DBG_VAL_PRINT(W_pub_keys_x[i]);
    }

    // aggregate secret key (layers 0, 1; G component)
    scalar_t w_sec_key_g = agg_coeff_0 * secret_0_xp + agg_coeff_1 * secret_1_f;
    DBG_VAL_PRINT(w_sec_key_g * c_point_G);

    // aggregate secret key (layer 2; X component)
    scalar_t w_sec_key_x   = agg_coeff_2 * secret_2_t;
    DBG_VAL_PRINT(w_sec_key_x * c_point_X);

    // calculate aggregate key image (layers 0, 1; G component)
    point_t W_key_image_g = agg_coeff_0 * key_image + agg_coeff_1 * K1;
    DBG_VAL_PRINT(key_image);
    DBG_VAL_PRINT(K1);
    DBG_VAL_PRINT(W_key_image_g);

    // calculate aggregate key image (layer 2; X component)
    point_t W_key_image_x = agg_coeff_2 * K2;
    DBG_VAL_PRINT(K2);
    DBG_VAL_PRINT(W_key_image_x);

#ifndef NDEBUG
    CRYPTO_CHECK_AND_THROW_MES(w_sec_key_g * c_point_G == W_pub_keys_g[secret_index], "aggregated secret G and pub key mismatch");
    CRYPTO_CHECK_AND_THROW_MES(w_sec_key_g * hash_helper_t::hp(ring[secret_index].stealth_address) == W_key_image_g, "aggregated secret G and key image mismatch");
    CRYPTO_CHECK_AND_THROW_MES(w_sec_key_x * c_point_X == W_pub_keys_x[secret_index], "aggregated secret X and pub key mismatch");
    CRYPTO_CHECK_AND_THROW_MES(w_sec_key_x * hash_helper_t::hp(ring[secret_index].stealth_address) == W_key_image_x, "aggregated secret X and key image mismatch");
#endif

    // initial commitment
    scalar_t alpha_g = scalar_t::random(); // randomness for layers 0,1
    scalar_t alpha_x = scalar_t::random(); // randomness for layer 2
    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGX_CHALLENGE);
    hsc.add_hash(input_hash);
    hsc.add_point(alpha_g * c_point_G);
    hsc.add_point(alpha_g * ki_base);
    hsc.add_point(alpha_x * c_point_X);
    hsc.add_point(alpha_x * ki_base);
    //DBG_PRINT("c[" << secret_index << "] = Hs(ih, " << alpha_g * c_point_G << ", " << alpha_g * ki_base << ", " << alpha_x * c_point_X << ", " << alpha_x * ki_base << ")");
    scalar_t c_prev = hsc.calc_hash();  // c_{secret_index + 1}

    sig.r_g.resize_and_make_random(ring_size);
    sig.r_x.resize_and_make_random(ring_size);

    for(size_t j = 0, i = (secret_index + 1) % ring_size; j < ring_size - 1; ++j, i = (i + 1) % ring_size)
    {
      DBG_PRINT("c[" << i << "] = " << c_prev);
      if (i == 0)
        sig.c = c_prev; // c_0
      hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGX_CHALLENGE);
      hsc.add_hash(input_hash);
      hsc.add_point(sig.r_g[i] * c_point_G + c_prev * W_pub_keys_g[i]);
      hsc.add_point(sig.r_g[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image_g);
      hsc.add_point(sig.r_x[i] * c_point_X + c_prev * W_pub_keys_x[i]);
      hsc.add_point(sig.r_x[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image_x);
      c_prev = hsc.calc_hash(); // c_{i + 1}
    }
    DBG_PRINT("c[" << secret_index << "] = " << c_prev);

    if (secret_index == 0)
      sig.c = c_prev;

    sig.r_g[secret_index] = alpha_g - c_prev * w_sec_key_g;
    sig.r_x[secret_index] = alpha_x - c_prev * w_sec_key_x;

    return true;
  }

  bool verify_CLSAG_GGX(const hash& m, const std::vector<CLSAG_GGX_input_ref_t>& ring, const public_key& pseudo_out_amount_commitment,
    const public_key& pseudo_out_blinded_asset_id, const key_image& ki, const CLSAG_GGX_signature& sig)
  {
    DBG_PRINT("== verify_CLSAG_GGX ==");
    size_t ring_size = ring.size();
    CRYPTO_CHECK_AND_THROW_MES(ring_size > 0, "ring size is zero");
    CRYPTO_CHECK_AND_THROW_MES(ring_size == sig.r_g.size(), "ring size != r_g size");
    CRYPTO_CHECK_AND_THROW_MES(ring_size == sig.r_x.size(), "ring size != r_x size");

    point_t key_image(ki);
    CRYPTO_CHECK_AND_THROW_MES(key_image.is_in_main_subgroup(), "key image 0 does not belong to the main subgroup");

    point_t pseudo_out_amount_commitment_pt(pseudo_out_amount_commitment);
    pseudo_out_amount_commitment_pt.modify_mul8();

    point_t pseudo_out_blinded_asset_id_pt(pseudo_out_blinded_asset_id);
    pseudo_out_blinded_asset_id_pt.modify_mul8();

    // calculate aggregation coefficients
    hash_helper_t::hs_t hsc(4 + 3  * ring_size);
    hsc.add_scalar(m);
    for(size_t i = 0; i < ring_size; ++i)
    {
      hsc.add_pub_key(ring[i].stealth_address);
      hsc.add_pub_key(ring[i].amount_commitment);
      hsc.add_pub_key(ring[i].blinded_asset_id);
      DBG_PRINT("ring[" << i << "]: sa:" << ring[i].stealth_address << ", ac:" << ring[i].amount_commitment << ", baid:" << ring[i].blinded_asset_id);
    }
    hsc.add_pub_key(pseudo_out_amount_commitment);
    hsc.add_pub_key(pseudo_out_blinded_asset_id);
    hsc.add_key_image(ki);
    hsc.add_pub_key(sig.K1);
    hsc.add_pub_key(sig.K2);
    hash input_hash = hsc.calc_hash_no_reduce();
    DBG_VAL_PRINT(input_hash);

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGX_LAYER_0);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_0 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_0);

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGX_LAYER_1);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_1 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_1);

    // may we get rid of it?
    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGX_LAYER_2);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_2 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_2);

    // prepare A_i, Q_i
    std::vector<point_t> A_i, Q_i;
    A_i.reserve(ring_size), Q_i.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      A_i.emplace_back(ring[i].amount_commitment);
      A_i.back().modify_mul8();
      Q_i.emplace_back(ring[i].blinded_asset_id);
      Q_i.back().modify_mul8();
      DBG_PRINT("A_i[" << i << "] = " << A_i[i] << "  Q_i[" << i << "] = " << Q_i[i]);
    }

    // calculate aggregate pub keys (layers 0, 1; G components)
    std::vector<point_t> W_pub_keys_g;
    W_pub_keys_g.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      W_pub_keys_g.emplace_back(
        agg_coeff_0 * point_t(ring[i].stealth_address) +
        agg_coeff_1 * (A_i[i] - pseudo_out_amount_commitment_pt)
      );
      DBG_VAL_PRINT(W_pub_keys_g[i]);
    }

    // calculate aggregate pub keys (layer 2; X component)
    std::vector<point_t> W_pub_keys_x;
    W_pub_keys_x.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      W_pub_keys_x.emplace_back(
        agg_coeff_2 * (Q_i[i] - pseudo_out_blinded_asset_id_pt)
      );
      DBG_VAL_PRINT(W_pub_keys_x[i]);
    }

    // calculate aggregate key image (layers 0, 1; G components)
    point_t W_key_image_g =
      agg_coeff_0 * key_image +
      agg_coeff_1 * point_t(sig.K1).modify_mul8();
    DBG_VAL_PRINT(point_t(sig.K1).modify_mul8());
    DBG_VAL_PRINT(W_key_image_g);

    // calculate aggregate key image (layer 2; X component)
    point_t W_key_image_x =
      agg_coeff_2 * point_t(sig.K2).modify_mul8();
    DBG_VAL_PRINT(point_t(sig.K2).modify_mul8());
    DBG_VAL_PRINT(W_key_image_x);


    scalar_t c_prev = sig.c;
    DBG_PRINT("c[0] = " << c_prev);
    for(size_t i = 0; i < ring_size; ++i)
    {
      hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGX_CHALLENGE);
      hsc.add_hash(input_hash);
      hsc.add_point(sig.r_g[i] * c_point_G + c_prev * W_pub_keys_g[i]);
      hsc.add_point(sig.r_g[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image_g);
      hsc.add_point(sig.r_x[i] * c_point_X + c_prev * W_pub_keys_x[i]);
      hsc.add_point(sig.r_x[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image_x);
      c_prev = hsc.calc_hash(); // c_{i + 1}
      DBG_PRINT("c[" << i + 1 << "] = " << c_prev);
      //DBG_PRINT("c[" << i + 1 << "] = Hs(ih, " << sig.r_g[i] * c_point_G + c_prev * W_pub_keys_g[i] << ", " << sig.r_g[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image_g << ", " << sig.r_x[i] * c_point_X + c_prev * W_pub_keys_x[i] << ", " << sig.r_x[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image_x << ")");
    }

    return c_prev == sig.c;
  }


  //---------------------------------------------------------------
  /*

  bool generate_CLSAG_GGXG(const hash& m, const std::vector<CLSAG_GGXG_input_ref_t>& ring, const point_t& pseudo_out_amount_commitment, const point_t& extended_amount_commitment, const key_image& ki,
    const scalar_t& secret_0_xp, const scalar_t& secret_1_f, const scalar_t& secret_2_x, const scalar_t& secret_3_q, uint64_t secret_index, CLSAG_GGXG_signature& sig)
  {
    DBG_PRINT("== generate_CLSAG_GGXG ==");
    size_t ring_size = ring.size();
    CRYPTO_CHECK_AND_THROW_MES(ring_size > 0, "ring size is zero");
    CRYPTO_CHECK_AND_THROW_MES(secret_index < ring_size, "secret_index is out of range");

    // calculate key images
    point_t ki_base = hash_helper_t::hp(ring[secret_index].stealth_address);
    point_t key_image = secret_0_xp * ki_base;

#ifndef NDEBUG
    CRYPTO_CHECK_AND_THROW_MES(key_image == point_t(ki), "key image 0 mismatch");
    CRYPTO_CHECK_AND_THROW_MES((secret_0_xp * c_point_G).to_public_key() == ring[secret_index].stealth_address, "secret_0_xp mismatch");
    CRYPTO_CHECK_AND_THROW_MES(secret_1_f * c_point_G == 8 * point_t(ring[secret_index].amount_commitment) - pseudo_out_amount_commitment, "secret_1_f mismatch");
    CRYPTO_CHECK_AND_THROW_MES(secret_3_q * c_point_G == 8 * point_t(ring[secret_index].concealing_point), "secret_3_q mismatch");
    CRYPTO_CHECK_AND_THROW_MES(secret_2_x * c_point_X == extended_amount_commitment - 8 * point_t(ring[secret_index].amount_commitment) - 8 * point_t(ring[secret_index].concealing_point), "secret_2_x mismatch");
#endif

    point_t K1_div8 = (c_scalar_1div8 * secret_1_f) * ki_base;
    K1_div8.to_public_key(sig.K1);
    point_t K1 = K1_div8;
    K1.modify_mul8();

    point_t K2_div8 = (c_scalar_1div8 * secret_2_x) * ki_base;
    K2_div8.to_public_key(sig.K2);
    point_t K2 = K2_div8;
    K2.modify_mul8();

    point_t K3_div8 = (c_scalar_1div8 * secret_3_q) * ki_base;
    K3_div8.to_public_key(sig.K3);
    point_t K3 = K3_div8;
    K3.modify_mul8();

    // calculate aggregation coefficients
    hash_helper_t::hs_t hsc(4 + 3  * ring_size);
    hsc.add_scalar(m);
    for(size_t i = 0; i < ring_size; ++i)
    {
      hsc.add_pub_key(ring[i].stealth_address);
      hsc.add_pub_key(ring[i].amount_commitment);
      hsc.add_pub_key(ring[i].concealing_point);
      DBG_PRINT("ring[" << i << "]: sa:" << ring[i].stealth_address << ", ac:" << ring[i].amount_commitment << ", cp:" << ring[i].concealing_point);
    }
    hsc.add_point(c_scalar_1div8 * pseudo_out_amount_commitment);
    hsc.add_point(c_scalar_1div8 * extended_amount_commitment);
    hsc.add_key_image(ki);
    hsc.add_pub_key(sig.K1);
    hsc.add_pub_key(sig.K2);
    hsc.add_pub_key(sig.K3);
    hash input_hash = hsc.calc_hash_no_reduce();
    DBG_VAL_PRINT(input_hash);

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXG_LAYER_0);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_0 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_0);

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXG_LAYER_1);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_1 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_1);

    // may we get rid of it?
    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXG_LAYER_2);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_2 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_2);

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXG_LAYER_3);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_3 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_3);

    // prepare A_i, Q_i
    std::vector<point_t> A_i, Q_i;
    A_i.reserve(ring_size), Q_i.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      A_i.emplace_back(ring[i].amount_commitment);
      A_i.back().modify_mul8();
      Q_i.emplace_back(ring[i].concealing_point);
      Q_i.back().modify_mul8();
      DBG_PRINT("A_i[" << i << "] = " << A_i[i] << "  Q_i[" << i << "] = " << Q_i[i]);
    }

    // calculate aggregate pub keys (layers 0, 1, 3; G components)
    std::vector<point_t> W_pub_keys_g;
    W_pub_keys_g.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      W_pub_keys_g.emplace_back(
        agg_coeff_0 * point_t(ring[i].stealth_address) +
        agg_coeff_1 * (A_i[i] - pseudo_out_amount_commitment) +
        agg_coeff_3 * Q_i[i]
      );
      DBG_VAL_PRINT(W_pub_keys_g[i]);
    }

    // calculate aggregate pub keys (layer 2; X component)
    std::vector<point_t> W_pub_keys_x;
    W_pub_keys_x.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      W_pub_keys_x.emplace_back(
        agg_coeff_2 * (extended_amount_commitment - A_i[i] - Q_i[i])
      );
      DBG_VAL_PRINT(W_pub_keys_x[i]);
    }

    // aggregate secret key (layers 0, 1, 3; G component)
    scalar_t w_sec_key_g = agg_coeff_0 * secret_0_xp + agg_coeff_1 * secret_1_f + agg_coeff_3 * secret_3_q;
    DBG_VAL_PRINT(w_sec_key_g * c_point_G);

    // aggregate secret key (layer 2; X component)
    scalar_t w_sec_key_x   = agg_coeff_2 * secret_2_x;
    DBG_VAL_PRINT(w_sec_key_x * c_point_X);

    // calculate aggregate key image (layers 0, 1, 3; G component)
    point_t W_key_image_g = agg_coeff_0 * key_image + agg_coeff_1 * K1 + agg_coeff_3 * K3;
    DBG_VAL_PRINT(key_image);
    DBG_VAL_PRINT(K1);
    DBG_VAL_PRINT(K3);
    DBG_VAL_PRINT(W_key_image_g);

    // calculate aggregate key image (layer 2; X component)
    point_t W_key_image_x = agg_coeff_2 * K2;
    DBG_VAL_PRINT(K2);
    DBG_VAL_PRINT(W_key_image_x);

#ifndef NDEBUG
    CRYPTO_CHECK_AND_THROW_MES(w_sec_key_g * c_point_G == W_pub_keys_g[secret_index], "aggregated secret G and pub key mismatch");
    CRYPTO_CHECK_AND_THROW_MES(w_sec_key_g * hash_helper_t::hp(ring[secret_index].stealth_address) == W_key_image_g, "aggregated secret G and key image mismatch");
    CRYPTO_CHECK_AND_THROW_MES(w_sec_key_x * c_point_X == W_pub_keys_x[secret_index], "aggregated secret X and pub key mismatch");
    CRYPTO_CHECK_AND_THROW_MES(w_sec_key_x * hash_helper_t::hp(ring[secret_index].stealth_address) == W_key_image_x, "aggregated secret X and key image mismatch");
#endif

    // initial commitment
    scalar_t alpha_g = scalar_t::random(); // randomness for layers 0,1,3
    scalar_t alpha_x = scalar_t::random();   // randomness for layer 2
    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXG_CHALLENGE);
    hsc.add_hash(input_hash);
    hsc.add_point(alpha_g * c_point_G);
    hsc.add_point(alpha_g * ki_base);
    hsc.add_point(alpha_x * c_point_X);
    hsc.add_point(alpha_x * ki_base);
    //DBG_PRINT("c[" << secret_index << "] = Hs(ih, " << alpha_g * c_point_G << ", " << alpha_g * ki_base << ", " << alpha_x * c_point_X << ", " << alpha_x * ki_base << ")");
    scalar_t c_prev = hsc.calc_hash();  // c_{secret_index + 1}

    sig.r_g.resize_and_make_random(ring_size);
    sig.r_x.resize_and_make_random(ring_size);

    for(size_t j = 0, i = (secret_index + 1) % ring_size; j < ring_size - 1; ++j, i = (i + 1) % ring_size)
    {
      DBG_PRINT("c[" << i << "] = " << c_prev);
      if (i == 0)
        sig.c = c_prev; // c_0
      hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXG_CHALLENGE);
      hsc.add_hash(input_hash);
      hsc.add_point(sig.r_g[i] * c_point_G + c_prev * W_pub_keys_g[i]);
      hsc.add_point(sig.r_g[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image_g);
      hsc.add_point(sig.r_x[i] * c_point_X + c_prev * W_pub_keys_x[i]);
      hsc.add_point(sig.r_x[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image_x);
      c_prev = hsc.calc_hash(); // c_{i + 1}
    }
    DBG_PRINT("c[" << secret_index << "] = " << c_prev);

    if (secret_index == 0)
      sig.c = c_prev;

    sig.r_g[secret_index] = alpha_g - c_prev * w_sec_key_g;
    sig.r_x[secret_index] = alpha_x - c_prev * w_sec_key_x;

    return true;
  }


  bool verify_CLSAG_GGXG(const hash& m, const std::vector<CLSAG_GGXG_input_ref_t>& ring, const public_key& pseudo_out_amount_commitment, const public_key& extended_amount_commitment,
    const key_image& ki, const CLSAG_GGXG_signature& sig)
  {
    DBG_PRINT("== verify_CLSAG_GGXG ==");
    size_t ring_size = ring.size();
    CRYPTO_CHECK_AND_THROW_MES(ring_size > 0, "ring size is zero");
    CRYPTO_CHECK_AND_THROW_MES(ring_size == sig.r_g.size(), "ring size != r_g size");
    CRYPTO_CHECK_AND_THROW_MES(ring_size == sig.r_x.size(), "ring size != r_x size");

    point_t key_image(ki);
    CRYPTO_CHECK_AND_THROW_MES(key_image.is_in_main_subgroup(), "key image 0 does not belong to the main subgroup");

    point_t pseudo_out_amount_commitment_pt(pseudo_out_amount_commitment);
    pseudo_out_amount_commitment_pt.modify_mul8();

    point_t extended_amount_commitment_pt(extended_amount_commitment);
    extended_amount_commitment_pt.modify_mul8();

    // calculate aggregation coefficients
    hash_helper_t::hs_t hsc(4 + 3  * ring_size);
    hsc.add_scalar(m);
    for(size_t i = 0; i < ring_size; ++i)
    {
      hsc.add_pub_key(ring[i].stealth_address);
      hsc.add_pub_key(ring[i].amount_commitment);
      hsc.add_pub_key(ring[i].concealing_point);
      DBG_PRINT("ring[" << i << "]: sa:" << ring[i].stealth_address << ", ac:" << ring[i].amount_commitment << ", cp:" << ring[i].concealing_point);
    }
    hsc.add_pub_key(pseudo_out_amount_commitment);
    hsc.add_pub_key(extended_amount_commitment);
    hsc.add_key_image(ki);
    hsc.add_pub_key(sig.K1);
    hsc.add_pub_key(sig.K2);
    hsc.add_pub_key(sig.K3);
    hash input_hash = hsc.calc_hash_no_reduce();
    DBG_VAL_PRINT(input_hash);

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXG_LAYER_0);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_0 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_0);

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXG_LAYER_1);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_1 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_1);

    // may we get rid of it?
    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXG_LAYER_2);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_2 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_2);

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXG_LAYER_3);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_3 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_3);

    // prepare A_i, Q_i
    std::vector<point_t> A_i, Q_i;
    A_i.reserve(ring_size), Q_i.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      A_i.emplace_back(ring[i].amount_commitment);
      A_i.back().modify_mul8();
      Q_i.emplace_back(ring[i].concealing_point);
      Q_i.back().modify_mul8();
      DBG_PRINT("A_i[" << i << "] = " << A_i[i] << "  Q_i[" << i << "] = " << Q_i[i]);
    }

    // calculate aggregate pub keys (layers 0, 1, 3; G components)
    std::vector<point_t> W_pub_keys_g;
    W_pub_keys_g.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      W_pub_keys_g.emplace_back(
        agg_coeff_0 * point_t(ring[i].stealth_address) +
        agg_coeff_1 * (A_i[i] - pseudo_out_amount_commitment_pt) +
        agg_coeff_3 * Q_i[i]
      );
      DBG_VAL_PRINT(W_pub_keys_g[i]);
    }

    // calculate aggregate pub keys (layer 2; X component)
    std::vector<point_t> W_pub_keys_x;
    W_pub_keys_x.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      W_pub_keys_x.emplace_back(
        agg_coeff_2 * (extended_amount_commitment_pt - A_i[i] - Q_i[i])
      );
      DBG_VAL_PRINT(W_pub_keys_x[i]);
    }

    // calculate aggregate key image (layers 0, 1, 3; G components)
    point_t W_key_image_g =
      agg_coeff_0 * key_image +
      agg_coeff_1 * point_t(sig.K1).modify_mul8() +
      agg_coeff_3 * point_t(sig.K3).modify_mul8();
    DBG_VAL_PRINT(point_t(sig.K1).modify_mul8());
    DBG_VAL_PRINT(point_t(sig.K3).modify_mul8());
    DBG_VAL_PRINT(W_key_image_g);

    // calculate aggregate key image (layer 2; X component)
    point_t W_key_image_x =
      agg_coeff_2 * point_t(sig.K2).modify_mul8();
    DBG_VAL_PRINT(point_t(sig.K2).modify_mul8());
    DBG_VAL_PRINT(W_key_image_x);


    scalar_t c_prev = sig.c;
    DBG_PRINT("c[0] = " << c_prev);
    for(size_t i = 0; i < ring_size; ++i)
    {
      hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXG_CHALLENGE);
      hsc.add_hash(input_hash);
      hsc.add_point(sig.r_g[i] * c_point_G + c_prev * W_pub_keys_g[i]);
      hsc.add_point(sig.r_g[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image_g);
      hsc.add_point(sig.r_x[i] * c_point_X + c_prev * W_pub_keys_x[i]);
      hsc.add_point(sig.r_x[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image_x);
      c_prev = hsc.calc_hash(); // c_{i + 1}
      DBG_PRINT("c[" << i + 1 << "] = " << c_prev);
      //DBG_PRINT("c[" << i + 1 << "] = Hs(ih, " << sig.r_g[i] * c_point_G + c_prev * W_pub_keys_g[i] << ", " << sig.r_g[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image_g << ", " << sig.r_x[i] * c_point_X + c_prev * W_pub_keys_x[i] << ", " << sig.r_x[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image_x << ")");
    }

    return c_prev == sig.c;
  }

  */
  //---------------------------------------------------------------


  bool generate_CLSAG_GGXXG(const hash& m, const std::vector<CLSAG_GGXXG_input_ref_t>& ring, const point_t& pseudo_out_amount_commitment, const point_t& pseudo_out_blinded_asset_id, const point_t& extended_amount_commitment, const key_image& ki,
    const scalar_t& secret_0_xp, const scalar_t& secret_1_f, const scalar_t& secret_2_t, const scalar_t& secret_3_x, const scalar_t& secret_4_q, uint64_t secret_index,
    CLSAG_GGXXG_signature& sig)
  {
    DBG_PRINT("== generate_CLSAG_GGXXG ==");
    size_t ring_size = ring.size();
    CRYPTO_CHECK_AND_THROW_MES(ring_size > 0, "ring size is zero");
    CRYPTO_CHECK_AND_THROW_MES(secret_index < ring_size, "secret_index is out of range");

    // calculate key images
    point_t ki_base = hash_helper_t::hp(ring[secret_index].stealth_address);
    point_t key_image = secret_0_xp * ki_base;

#ifndef NDEBUG
    CRYPTO_CHECK_AND_THROW_MES(key_image == point_t(ki), "key image 0 mismatch");
    CRYPTO_CHECK_AND_THROW_MES((secret_0_xp * c_point_G).to_public_key() == ring[secret_index].stealth_address, "secret_0_xp mismatch");
    CRYPTO_CHECK_AND_THROW_MES( secret_1_f  * c_point_G == 8 * point_t(ring[secret_index].amount_commitment) - pseudo_out_amount_commitment, "secret_1_f mismatch");
    CRYPTO_CHECK_AND_THROW_MES( secret_2_t  * c_point_X == 8 * point_t(ring[secret_index].blinded_asset_id)  - pseudo_out_blinded_asset_id, "secret_2_t mismatch");
    CRYPTO_CHECK_AND_THROW_MES( secret_3_x  * c_point_X == extended_amount_commitment - 8 * point_t(ring[secret_index].amount_commitment) - 8 * point_t(ring[secret_index].concealing_point), "");
    CRYPTO_CHECK_AND_THROW_MES( secret_4_q  * c_point_G == 8 * point_t(ring[secret_index].concealing_point), "secret_3_q mismatch");
#endif

    point_t K1_div8 = (c_scalar_1div8 * secret_1_f) * ki_base;
    K1_div8.to_public_key(sig.K1);
    point_t K1 = K1_div8;
    K1.modify_mul8();

    point_t K2_div8 = (c_scalar_1div8 * secret_2_t) * ki_base;
    K2_div8.to_public_key(sig.K2);
    point_t K2 = K2_div8;
    K2.modify_mul8();

    point_t K3_div8 = (c_scalar_1div8 * secret_3_x) * ki_base;
    K3_div8.to_public_key(sig.K3);
    point_t K3 = K3_div8;
    K3.modify_mul8();

    point_t K4_div8 = (c_scalar_1div8 * secret_4_q) * ki_base;
    K4_div8.to_public_key(sig.K4);
    point_t K4 = K4_div8;
    K4.modify_mul8();

    // calculate aggregation coefficients
    hash_helper_t::hs_t hsc(5 + 4 * ring_size);
    hsc.add_scalar(m);
    for(size_t i = 0; i < ring_size; ++i)
    {
      hsc.add_pub_key(ring[i].stealth_address);
      hsc.add_pub_key(ring[i].amount_commitment);
      hsc.add_pub_key(ring[i].blinded_asset_id);
      hsc.add_pub_key(ring[i].concealing_point);
      DBG_PRINT("ring[" << i << "]: sa:" << ring[i].stealth_address << ", ac:" << ring[i].amount_commitment << ", baid:" << ring[i].blinded_asset_id << ", cp:" << ring[i].concealing_point);
    }
    hsc.add_point(c_scalar_1div8 * pseudo_out_amount_commitment);
    hsc.add_point(c_scalar_1div8 * pseudo_out_blinded_asset_id);
    hsc.add_point(c_scalar_1div8 * extended_amount_commitment);
    hsc.add_key_image(ki);
    hsc.add_pub_key(sig.K1);
    hsc.add_pub_key(sig.K2);
    hsc.add_pub_key(sig.K3);
    hsc.add_pub_key(sig.K4);
    hash input_hash = hsc.calc_hash_no_reduce();
    DBG_VAL_PRINT(input_hash);

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXXG_LAYER_0);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_0 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_0);

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXXG_LAYER_1);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_1 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_1);

    // may we get rid of it?
    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXXG_LAYER_2);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_2 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_2);

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXXG_LAYER_3);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_3 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_3);

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXXG_LAYER_4);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_4 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_4);

    // prepare A_i, Q_i
    std::vector<point_t> A_i, P_i, Q_i;
    A_i.reserve(ring_size), P_i.reserve(ring_size), Q_i.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      A_i.emplace_back(ring[i].amount_commitment);
      A_i.back().modify_mul8();
      P_i.emplace_back(ring[i].blinded_asset_id);
      P_i.back().modify_mul8();
      Q_i.emplace_back(ring[i].concealing_point);
      Q_i.back().modify_mul8();
      DBG_PRINT("A_i[" << i << "] = " << A_i[i] << "  P_i[" << i << "] = " << P_i[i] << "  Q_i[" << i << "] = " << Q_i[i]);
    }

    // calculate aggregate pub keys (layers 0, 1, 4; G components)
    std::vector<point_t> W_pub_keys_g;
    W_pub_keys_g.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      W_pub_keys_g.emplace_back(
        agg_coeff_0 * point_t(ring[i].stealth_address) +
        agg_coeff_1 * (A_i[i] - pseudo_out_amount_commitment) +
        agg_coeff_4 * Q_i[i]
      );
      DBG_VAL_PRINT(W_pub_keys_g[i]);
    }

    // calculate aggregate pub keys (layerx 2, 3; X component)
    std::vector<point_t> W_pub_keys_x;
    W_pub_keys_x.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      W_pub_keys_x.emplace_back(
        agg_coeff_2 * (P_i[i] - pseudo_out_blinded_asset_id) + 
        agg_coeff_3 * (extended_amount_commitment - A_i[i] - Q_i[i])
      );
      DBG_VAL_PRINT(W_pub_keys_x[i]);
    }

    // aggregate secret key (layers 0, 1, 4; G component)
    scalar_t w_sec_key_g = agg_coeff_0 * secret_0_xp + agg_coeff_1 * secret_1_f + agg_coeff_4 * secret_4_q;
    DBG_VAL_PRINT(w_sec_key_g * c_point_G);

    // aggregate secret key (layer 2, 3; X component)
    scalar_t w_sec_key_x   = agg_coeff_2 * secret_2_t + agg_coeff_3 * secret_3_x;
    DBG_VAL_PRINT(w_sec_key_x * c_point_X);

    // calculate aggregate key image (layers 0, 1, 4; G component)
    point_t W_key_image_g = agg_coeff_0 * key_image + agg_coeff_1 * K1 + agg_coeff_4 * K4;
    DBG_VAL_PRINT(key_image);
    DBG_VAL_PRINT(K1);
    DBG_VAL_PRINT(K4);
    DBG_VAL_PRINT(W_key_image_g);

    // calculate aggregate key image (layer 2, 3; X component)
    point_t W_key_image_x = agg_coeff_2 * K2 + agg_coeff_3 * K3;
    DBG_VAL_PRINT(K2);
    DBG_VAL_PRINT(K3);
    DBG_VAL_PRINT(W_key_image_x);

#ifndef NDEBUG
    CRYPTO_CHECK_AND_THROW_MES(w_sec_key_g * c_point_G == W_pub_keys_g[secret_index], "aggregated secret G and pub key mismatch");
    CRYPTO_CHECK_AND_THROW_MES(w_sec_key_g * hash_helper_t::hp(ring[secret_index].stealth_address) == W_key_image_g, "aggregated secret G and key image mismatch");
    CRYPTO_CHECK_AND_THROW_MES(w_sec_key_x * c_point_X == W_pub_keys_x[secret_index], "aggregated secret X and pub key mismatch");
    CRYPTO_CHECK_AND_THROW_MES(w_sec_key_x * hash_helper_t::hp(ring[secret_index].stealth_address) == W_key_image_x, "aggregated secret X and key image mismatch");
#endif

    // initial commitment
    scalar_t alpha_g = scalar_t::random(); // randomness for layers 0,1,4
    scalar_t alpha_x = scalar_t::random(); // randomness for layer 2,3
    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXXG_CHALLENGE);
    hsc.add_hash(input_hash);
    hsc.add_point(alpha_g * c_point_G);
    hsc.add_point(alpha_g * ki_base);
    hsc.add_point(alpha_x * c_point_X);
    hsc.add_point(alpha_x * ki_base);
    //DBG_PRINT("c[" << secret_index << "] = Hs(ih, " << alpha_g * c_point_G << ", " << alpha_g * ki_base << ", " << alpha_x * c_point_X << ", " << alpha_x * ki_base << ")");
    scalar_t c_prev = hsc.calc_hash();  // c_{secret_index + 1}

    sig.r_g.resize_and_make_random(ring_size);
    sig.r_x.resize_and_make_random(ring_size);

    for(size_t j = 0, i = (secret_index + 1) % ring_size; j < ring_size - 1; ++j, i = (i + 1) % ring_size)
    {
      DBG_PRINT("c[" << i << "] = " << c_prev);
      if (i == 0)
        sig.c = c_prev; // c_0
      hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXXG_CHALLENGE);
      hsc.add_hash(input_hash);
      hsc.add_point(sig.r_g[i] * c_point_G + c_prev * W_pub_keys_g[i]);
      hsc.add_point(sig.r_g[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image_g);
      hsc.add_point(sig.r_x[i] * c_point_X + c_prev * W_pub_keys_x[i]);
      hsc.add_point(sig.r_x[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image_x);
      c_prev = hsc.calc_hash(); // c_{i + 1}
      //DBG_PRINT("c[" << i + 1 << "] = Hs(ih, " << sig.r_g[i] * c_point_G + c_prev * W_pub_keys_g[i] << ", " << sig.r_g[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image_g << ", " << sig.r_x[i] * c_point_X + c_prev * W_pub_keys_x[i] << ", " << sig.r_x[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image_x << ")");
    }
    DBG_PRINT("c[" << secret_index << "] = " << c_prev);


    if (secret_index == 0)
      sig.c = c_prev;

    sig.r_g[secret_index] = alpha_g - c_prev * w_sec_key_g;
    sig.r_x[secret_index] = alpha_x - c_prev * w_sec_key_x;

    return true;
  }

  bool verify_CLSAG_GGXXG(const hash& m, const std::vector<CLSAG_GGXXG_input_ref_t>& ring, const public_key& pseudo_out_amount_commitment, const public_key& pseudo_out_blinded_asset_id, const public_key& extended_amount_commitment, const key_image& ki,
    const CLSAG_GGXXG_signature& sig)
  {
    DBG_PRINT("== verify_CLSAG_GGXXG ==");
    size_t ring_size = ring.size();
    CRYPTO_CHECK_AND_THROW_MES(ring_size > 0, "ring size is zero");
    CRYPTO_CHECK_AND_THROW_MES(ring_size == sig.r_g.size(), "ring size != r_g size");
    CRYPTO_CHECK_AND_THROW_MES(ring_size == sig.r_x.size(), "ring size != r_x size");

    point_t key_image(ki);
    CRYPTO_CHECK_AND_THROW_MES(key_image.is_in_main_subgroup(), "key image 0 does not belong to the main subgroup");

    point_t pseudo_out_amount_commitment_pt(pseudo_out_amount_commitment);
    pseudo_out_amount_commitment_pt.modify_mul8();

    point_t pseudo_out_blinded_asset_id_pt(pseudo_out_blinded_asset_id);
    pseudo_out_blinded_asset_id_pt.modify_mul8();

    point_t extended_amount_commitment_pt(extended_amount_commitment);
    extended_amount_commitment_pt.modify_mul8();

    // calculate aggregation coefficients
    hash_helper_t::hs_t hsc(5 + 4 * ring_size);
    hsc.add_scalar(m);
    for(size_t i = 0; i < ring_size; ++i)
    {
      hsc.add_pub_key(ring[i].stealth_address);
      hsc.add_pub_key(ring[i].amount_commitment);
      hsc.add_pub_key(ring[i].blinded_asset_id);
      hsc.add_pub_key(ring[i].concealing_point);
      DBG_PRINT("ring[" << i << "]: sa:" << ring[i].stealth_address << ", ac:" << ring[i].amount_commitment << ", baid:" << ring[i].blinded_asset_id << ", cp:" << ring[i].concealing_point);
    }
    hsc.add_pub_key(pseudo_out_amount_commitment);
    hsc.add_pub_key(pseudo_out_blinded_asset_id);
    hsc.add_pub_key(extended_amount_commitment);
    hsc.add_key_image(ki);
    hsc.add_pub_key(sig.K1);
    hsc.add_pub_key(sig.K2);
    hsc.add_pub_key(sig.K3);
    hsc.add_pub_key(sig.K4);
    hash input_hash = hsc.calc_hash_no_reduce();
    DBG_VAL_PRINT(input_hash);

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXXG_LAYER_0);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_0 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_0);

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXXG_LAYER_1);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_1 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_1);

    // may we get rid of it?
    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXXG_LAYER_2);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_2 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_2);

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXXG_LAYER_3);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_3 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_3);

    hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXXG_LAYER_4);
    hsc.add_hash(input_hash);
    scalar_t agg_coeff_4 = hsc.calc_hash();
    DBG_VAL_PRINT(agg_coeff_4);

    // prepare A_i, Q_i
    std::vector<point_t> A_i, P_i, Q_i;
    A_i.reserve(ring_size), P_i.reserve(ring_size), Q_i.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      A_i.emplace_back(ring[i].amount_commitment);
      A_i.back().modify_mul8();
      P_i.emplace_back(ring[i].blinded_asset_id);
      P_i.back().modify_mul8();
      Q_i.emplace_back(ring[i].concealing_point);
      Q_i.back().modify_mul8();
      DBG_PRINT("A_i[" << i << "] = " << A_i[i] << "  P_i[" << i << "] = " << P_i[i] << "  Q_i[" << i << "] = " << Q_i[i]);
    }

    // calculate aggregate pub keys (layers 0, 1, 4; G components)
    std::vector<point_t> W_pub_keys_g;
    W_pub_keys_g.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      W_pub_keys_g.emplace_back(
        agg_coeff_0 * point_t(ring[i].stealth_address) +
        agg_coeff_1 * (A_i[i] - pseudo_out_amount_commitment_pt) +
        agg_coeff_4 * Q_i[i]
      );
      DBG_VAL_PRINT(W_pub_keys_g[i]);
    }

    // calculate aggregate pub keys (layer 2, 3; X component)
    std::vector<point_t> W_pub_keys_x;
    W_pub_keys_x.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      W_pub_keys_x.emplace_back(
        agg_coeff_2 * (P_i[i] - pseudo_out_blinded_asset_id_pt) +
        agg_coeff_3 * (extended_amount_commitment_pt - A_i[i] - Q_i[i])
      );
      DBG_VAL_PRINT(W_pub_keys_x[i]);
    }

    DBG_VAL_PRINT(point_t(ki));

    // calculate aggregate key image (layers 0, 1, 4; G components)
    DBG_VAL_PRINT(point_t(sig.K1).modify_mul8());
    point_t W_key_image_g =
      agg_coeff_0 * key_image +
      agg_coeff_1 * point_t(sig.K1).modify_mul8() +
      agg_coeff_4 * point_t(sig.K4).modify_mul8();
    DBG_VAL_PRINT(point_t(sig.K1).modify_mul8());
    DBG_VAL_PRINT(point_t(sig.K4).modify_mul8());
    DBG_VAL_PRINT(W_key_image_g);

    // calculate aggregate key image (layer 2, 3; X component)
    point_t W_key_image_x =
      agg_coeff_2 * point_t(sig.K2).modify_mul8() +
      agg_coeff_3 * point_t(sig.K3).modify_mul8();
    DBG_VAL_PRINT(point_t(sig.K2).modify_mul8());
    DBG_VAL_PRINT(point_t(sig.K3).modify_mul8());
    DBG_VAL_PRINT(W_key_image_x);


    scalar_t c_prev = sig.c;
    DBG_PRINT("c[0] = " << c_prev);
    for(size_t i = 0; i < ring_size; ++i)
    {
      hsc.add_32_chars(CRYPTO_HDS_CLSAG_GGXXG_CHALLENGE);
      hsc.add_hash(input_hash);
      hsc.add_point(sig.r_g[i] * c_point_G + c_prev * W_pub_keys_g[i]);
      hsc.add_point(sig.r_g[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image_g);
      hsc.add_point(sig.r_x[i] * c_point_X + c_prev * W_pub_keys_x[i]);
      hsc.add_point(sig.r_x[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image_x);
      c_prev = hsc.calc_hash(); // c_{i + 1}
      DBG_PRINT("c[" << i + 1 << "] = " << c_prev);
      //DBG_PRINT("c[" << i + 1 << "] = Hs(ih, " << sig.r_g[i] * c_point_G + c_prev * W_pub_keys_g[i] << ", " << sig.r_g[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image_g << ", " << sig.r_x[i] * c_point_X + c_prev * W_pub_keys_x[i] << ", " << sig.r_x[i] * hash_helper_t::hp(ring[i].stealth_address) + c_prev * W_key_image_x << ")");
    }

    return c_prev == sig.c;
  }



} // namespace crypto
