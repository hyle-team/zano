// Copyright (c) 2022-2024 Zano Project
// Copyright (c) 2022-2024 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// This file contains implementation of the original d-CLSAG (s.a. https://eprint.iacr.org/2019/654.pdf by Goodel at el)
// and the extended d/v-CLSAG version (s.a. https://github.com/hyle-team/docs/blob/master/zano/dv-CLSAG-extension/ by sowle)
//
#pragma once
#include "crypto-sugar.h"

namespace crypto
{
  // 2-CLSAG signature where both dimensions are with respect to the group element G (that's why 'GG')
  struct CLSAG_GG_signature
  {
    scalar_t      c;
    scalar_vec_t  r;  // size = size of the ring
    public_key    K1; // auxiliary key image for layer 1
  };


  inline bool operator==(const CLSAG_GG_signature& lhs, const CLSAG_GG_signature& rhs)
  {
    return
      lhs.c == rhs.c &&
      lhs.r == rhs.r &&
      lhs.K1 == rhs.K1;
  }

  inline bool operator!=(const CLSAG_GG_signature& lhs, const CLSAG_GG_signature& rhs) { return !(lhs == rhs); }

  struct CLSAG_GG_input_ref_t
  {
    CLSAG_GG_input_ref_t(const public_key& stealth_address, const public_key& amount_commitment)
      : stealth_address(stealth_address), amount_commitment(amount_commitment)
    {}

    const public_key& stealth_address;   // P, not premultiplied by 1/8, TODO @#@#: make sure it's okay
    const public_key& amount_commitment; // A, premultiplied by 1/8
  };

  // pseudo_out_amount_commitment -- not premultiplied by 1/8
  bool generate_CLSAG_GG(const hash& m, const std::vector<CLSAG_GG_input_ref_t>& ring, const point_t& pseudo_out_amount_commitment, const key_image& ki,
    const scalar_t& secret_x, const scalar_t& secret_f, uint64_t secret_index, CLSAG_GG_signature& sig);

  // pseudo_out_amount_commitment -- premultiplied by 1/8
  bool verify_CLSAG_GG(const hash& m, const std::vector<CLSAG_GG_input_ref_t>& ring, const public_key& pseudo_out_amount_commitment, const key_image& ki,
    const CLSAG_GG_signature& sig);


  //
  // Disclaimer: extensions to the CLSAG implemented below are non-standard and are in proof-of-concept state.
  // They shouldn't be used in production code until formal security proofs are done and (ideally) the code is peer-reviewed.
  // -- sowle
  //



  //
  // 3/2-CLSAG
  //


  // 3/2-CLSAG signature (with respect to the group element G, G, X -- that's why 'GGX')
  struct CLSAG_GGX_signature
  {
    scalar_t      c;
    scalar_vec_t  r_g;  // for G-components (layers 0, 1),    size = size of the ring
    scalar_vec_t  r_x;  // for X-component  (layer 2),        size = size of the ring
    public_key    K1;   // auxiliary key image for layer 1 (G)
    public_key    K2;   // auxiliary key image for layer 2 (X)
  };

  struct CLSAG_GGX_input_ref_t : public CLSAG_GG_input_ref_t
  {
    CLSAG_GGX_input_ref_t(const public_key& stealth_address, const public_key& amount_commitment, const public_key& blinded_asset_id)
      : CLSAG_GG_input_ref_t(stealth_address, amount_commitment)
      , blinded_asset_id(blinded_asset_id)
    {}

    const public_key& blinded_asset_id; // T, premultiplied by 1/8
  };

  // pseudo_out_amount_commitment -- not premultiplied by 1/8
  // pseudo_out_asset_id         -- not premultiplied by 1/8
  bool generate_CLSAG_GGX(const hash& m, const std::vector<CLSAG_GGX_input_ref_t>& ring, const point_t& pseudo_out_amount_commitment, const point_t& pseudo_out_asset_id, const key_image& ki,
    const scalar_t& secret_0_xp, const scalar_t& secret_1_f, const scalar_t& secret_2_t, uint64_t secret_index, CLSAG_GGX_signature& sig);

  // pseudo_out_amount_commitment -- premultiplied by 1/8
  // pseudo_out_asset_id         -- premultiplied by 1/8
  // may throw an exception TODO @#@# make sure it's okay
  bool verify_CLSAG_GGX(const hash& m, const std::vector<CLSAG_GGX_input_ref_t>& ring, const public_key& pseudo_out_amount_commitment,
    const public_key& pseudo_out_asset_id, const key_image& ki, const CLSAG_GGX_signature& sig);


  /*
  //
  // 4/2-CLSAG  (eventually, it's not used in Zano)
  //


  // 4/2-CLSAG signature (with respect to the group element G, G, X, G -- that's why 'GGXG')
  struct CLSAG_GGXG_signature
  {
    scalar_t      c;
    scalar_vec_t  r_g;  // for G-components (layers 0, 1, 3), size = size of the ring
    scalar_vec_t  r_x;  // for X-component  (layer 2),        size = size of the ring
    public_key    K1;   // auxiliary key image for layer 1 (G)
    public_key    K2;   // auxiliary key image for layer 2 (X)
    public_key    K3;   // auxiliary key image for layer 3 (G)
  };

  struct CLSAG_GGXG_input_ref_t : public CLSAG_GG_input_ref_t
  {
    CLSAG_GGXG_input_ref_t(const public_key& stealth_address, const public_key& amount_commitment, const public_key& concealing_point)
      : CLSAG_GG_input_ref_t(stealth_address, amount_commitment)
      , concealing_point(concealing_point)
    {}

    const public_key& concealing_point; // Q, premultiplied by 1/8
  };

  // pseudo_out_amount_commitment -- not premultiplied by 1/8
  // extended_amount_commitment   -- not premultiplied by 1/8
  bool generate_CLSAG_GGXG(const hash& m, const std::vector<CLSAG_GGXG_input_ref_t>& ring, const point_t& pseudo_out_amount_commitment, const point_t& extended_amount_commitment, const key_image& ki,
    const scalar_t& secret_0_xp, const scalar_t& secret_1_f, const scalar_t& secret_2_x, const scalar_t& secret_3_q, uint64_t secret_index, CLSAG_GGXG_signature& sig);

  // pseudo_out_amount_commitment -- premultiplied by 1/8
  // extended_amount_commitment   -- premultiplied by 1/8
  // may throw an exception TODO @#@# make sure it's okay
  bool verify_CLSAG_GGXG(const hash& m, const std::vector<CLSAG_GGXG_input_ref_t>& ring, const public_key& pseudo_out_amount_commitment,
    const public_key& extended_amount_commitment, const key_image& ki, const CLSAG_GGXG_signature& sig);

  */

  //
  // 5/2-CLSAG
  //


  // 5/2-CLSAG signature (with respect to the group element G, G, X, X, G -- that's why 'GGXXG')
  struct CLSAG_GGXXG_signature
  {
    scalar_t      c;
    scalar_vec_t  r_g;  // for G-components (layers 0, 1, 4), size = size of the ring
    scalar_vec_t  r_x;  // for X-component  (layers 2, 3),    size = size of the ring
    public_key    K1;   // auxiliary key image for layer 1 (G)
    public_key    K2;   // auxiliary key image for layer 2 (X)
    public_key    K3;   // auxiliary key image for layer 2 (X)
    public_key    K4;   // auxiliary key image for layer 3 (G)
  };

  struct CLSAG_GGXXG_input_ref_t : public CLSAG_GGX_input_ref_t
  {
    CLSAG_GGXXG_input_ref_t(const public_key& stealth_address, const public_key& amount_commitment, const public_key& blinded_asset_id, const public_key& concealing_point)
      : CLSAG_GGX_input_ref_t(stealth_address, amount_commitment, blinded_asset_id)
      , concealing_point(concealing_point)
    {}

    const public_key& concealing_point; // Q, premultiplied by 1/8
  };

  // pseudo_out_amount_commitment -- not premultiplied by 1/8
  // pseudo_out_asset_id          -- not premultiplied by 1/8
  // extended_amount_commitment   -- not premultiplied by 1/8
  bool generate_CLSAG_GGXXG(const hash& m, const std::vector<CLSAG_GGXXG_input_ref_t>& ring, const point_t& pseudo_out_amount_commitment, const point_t& pseudo_out_blinded_asset_id, const point_t& extended_amount_commitment, const key_image& ki,
    const scalar_t& secret_0_xp, const scalar_t& secret_1_f, const scalar_t& secret_2_r, const scalar_t& secret_3_x, const scalar_t& secret_4_q, uint64_t secret_index, CLSAG_GGXXG_signature& sig);

  // pseudo_out_amount_commitment -- premultiplied by 1/8
  // pseudo_out_asset_id          -- premultiplied by 1/8
  // extended_amount_commitment   -- premultiplied by 1/8
  // may throw an exception TODO @#@# make sure it's okay
  bool verify_CLSAG_GGXXG(const hash& m, const std::vector<CLSAG_GGXXG_input_ref_t>& ring, const public_key& pseudo_out_amount_commitment, const public_key& pseudo_out_blinded_asset_id, const public_key& extended_amount_commitment, const key_image& ki,
    const CLSAG_GGXXG_signature& sig);


} // namespace crypto
