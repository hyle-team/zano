// Copyright (c) 2022 Zano Project
// Copyright (c) 2022 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// This file contains implementation of CLSAG (s.a. https://eprint.iacr.org/2019/654.pdf by Goodel at el)
//
#include "clsag.h"

namespace crypto
{

  bool generate_CLSAG_GG(const hash& m, const std::vector<CLSAG_GG_input_ref_t>& ring, const point_t& pseudo_out_amount_commitment, const key_image& ki,
    const scalar_t& secret_x, const scalar_t& secret_f, CLSAG_GG_signature& sig)
  {
    return false;
  }

} // namespace crypto
