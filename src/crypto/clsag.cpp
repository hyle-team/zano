// Copyright (c) 2022 Zano Project
// Copyright (c) 2022 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// This file contains implementation of CLSAG (s.a. https://eprint.iacr.org/2019/654.pdf by Goodel at el)
//
#include "clsag.h"
//#include "misc_log_ex.h"
#include "../currency_core/crypto_config.h"

namespace crypto
{
  #define DBG_VAL_PRINT(x) (void(0)) // std::cout << #x ": " << x << std::endl
  #define DBG_PRINT(x)     (void(0)) // std::cout << x << std::endl

  bool generate_CLSAG_GG(const hash& m, const std::vector<CLSAG_GG_input_ref_t>& ring, const point_t& pseudo_out_amount_commitment, const key_image& ki,
    const scalar_t& secret_x, const scalar_t& secret_f, uint64_t secret_index, CLSAG_GG_signature& sig)
  {
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
    hsc.add_point(pseudo_out_amount_commitment);
    hsc.add_key_image(ki);
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

    sig.r.clear();
    sig.r.reserve(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
      sig.r.emplace_back(scalar_t::random());

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
    hsc.add_point(pseudo_out_amount_commitment_pt);
    hsc.add_key_image(ki);
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
    point_t W_key_image = agg_coeff_0 * point_t(ki) + agg_coeff_1 * point_t(sig.K1).modify_mul8();
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

} // namespace crypto
