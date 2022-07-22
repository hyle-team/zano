// Copyright (c) 2022 Zano Project
// Copyright (c) 2022 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// This file contains implementation of CLSAG (s.a. https://eprint.iacr.org/2019/654.pdf by Goodel at el)
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
      : stealth_address(stealth_address), amount_commitment(amount_commitment) {}

    const public_key& stealth_address;
    const public_key& amount_commitment;
  };

  bool generate_CLSAG_GG(const hash& m, const std::vector<CLSAG_GG_input_ref_t>& ring, const point_t& pseudo_out_amount_commitment, const key_image& ki,
    const scalar_t& secret_x, const scalar_t& secret_f, uint64_t secret_index, CLSAG_GG_signature& sig);

  bool verify_CLSAG_GG(const hash& m, const std::vector<CLSAG_GG_input_ref_t>& ring, const public_key& pseudo_out_amount_commitment, const key_image& ki,
    const CLSAG_GG_signature& sig);


} // namespace crypto
