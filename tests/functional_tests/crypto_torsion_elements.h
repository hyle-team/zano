// Copyright (c) 2021-2022 Zano Project
// Copyright (c) 2021-2022 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#pragma once

//
// Tosion elements of ed25519 group in canonical and non-canonical form
//
// (partly inspired by "Taming the many EdDSAs" by Chalkias et al https://eprint.iacr.org/2020/1244.pdf)
//

namespace crypto
{

  // let ty = -sqrt((-sqrt(D+1)-1) / D), is_neg(ty) == false
  // canonical serialization                                           sig  order  EC point
  // 26e8958fc2b227b045c3f489f2ef98f0d5dfac05d3c63339b13802886d53fc05  0    8      (sqrt(-1)*ty, ty)
  // 0000000000000000000000000000000000000000000000000000000000000000  0    4      (sqrt(-1), 0)
  // c7176a703d4dd84fba3c0b760d10670f2a2053fa2c39ccc64ec7fd7792ac037a  0    8      (sqrt(-1)*ty, -ty)
  // ecffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7f  0    2      (0, -1)
  // c7176a703d4dd84fba3c0b760d10670f2a2053fa2c39ccc64ec7fd7792ac03fa  1    8      (-sqrt(-1)*ty, -ty)
  // 0000000000000000000000000000000000000000000000000000000000000080  1    4      (-sqrt(-1), 0)
  // 26e8958fc2b227b045c3f489f2ef98f0d5dfac05d3c63339b13802886d53fc85  1    8      (-sqrt(-1)*ty, ty)

  struct canonical_torsion_elements_t
  {
    const char* string;
    bool sign;
    uint8_t order;
    uint8_t incorrect_order_0;
    uint8_t incorrect_order_1;
  };

  canonical_torsion_elements_t canonical_torsion_elements[] = {
    {"26e8958fc2b227b045c3f489f2ef98f0d5dfac05d3c63339b13802886d53fc05", false, 8, 4, 2},
    {"0000000000000000000000000000000000000000000000000000000000000000", false, 4, 2, 6},
    {"c7176a703d4dd84fba3c0b760d10670f2a2053fa2c39ccc64ec7fd7792ac037a", false, 8, 4, 2},
    {"ecffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7f", false, 2, 5, 3},
    {"c7176a703d4dd84fba3c0b760d10670f2a2053fa2c39ccc64ec7fd7792ac03fa", true,  8, 4, 2},
    {"0000000000000000000000000000000000000000000000000000000000000080", true,  4, 2, 6},
    {"26e8958fc2b227b045c3f489f2ef98f0d5dfac05d3c63339b13802886d53fc85", true,  8, 4, 2}
  };

  // non-canonical elements (should not load at all thanks to the checks in ge_frombytes_vartime)
  const char* noncanonical_torsion_elements[] = {
    "0100000000000000000000000000000000000000000000000000000000000080", // (-0, 1)
    "ECFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", // (-0, -1)
    "EEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF7F", // (0, 2*255-18)
    "EEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", // (-0, 2*255-18)
    "EDFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF7F", // (sqrt(-1), 2*255-19)
    "EDFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"  // (-sqrt(-1), 2*255-19)
  };

} // namespace crypto
