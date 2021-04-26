// Copyright (c) 2020-2021 Zano Project
// Copyright (c) 2020-2021 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Note: This file originates from tests/functional_tests/crypto_tests.cpp 

#include "crypto-sugar.h"

namespace crypto
{

  const point_g_t c_point_G;

  const scalar_t c_scalar_1       = { 1 };
  const scalar_t c_scalar_L       = { 0x5812631a5cf5d3ed, 0x14def9dea2f79cd6, 0x0,                0x1000000000000000 };
  const scalar_t c_scalar_Lm1     = { 0x5812631a5cf5d3ec, 0x14def9dea2f79cd6, 0x0,                0x1000000000000000 };
  const scalar_t c_scalar_P       = { 0xffffffffffffffed, 0xffffffffffffffff, 0xffffffffffffffff, 0x7fffffffffffffff };
  const scalar_t c_scalar_Pm1     = { 0xffffffffffffffec, 0xffffffffffffffff, 0xffffffffffffffff, 0x7fffffffffffffff };
  const scalar_t c_scalar_256m1   = { 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff };
  const scalar_t c_scalar_1div8   = { 0x6106e529e2dc2f79, 0x7d39db37d1cdad0,  0x0,                0x600000000000000 };

  const point_t  c_point_H        = { 0x05087c1f5b9b32d6, 0x00547595f445c3b5, 0x764df64578552f2a, 0x8a49a651e0e0da45 };  // == Hp(G), this is being checked in bpp_basics
  const point_t  c_point_0        = point_t(point_t::tag_zero());

} // namespace crypto
