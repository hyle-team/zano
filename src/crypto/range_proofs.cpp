// Copyright (c) 2021 Zano Project (https://zano.org/)
// Copyright (c) 2021 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "range_proofs.h"

namespace crypto
{
  // TODO @#@# redesign needed, consider changing to inline constexpr
  const point_t& bpp_ct_generators_HGX::bpp_G   = c_point_H;
  const point_t& bpp_ct_generators_HGX::bpp_H   = c_point_G;
  const point_t& bpp_ct_generators_HGX::bpp_H2  = c_point_X;

  const point_t& bpp_ct_generators_UGX::bpp_G   = c_point_U;
  const point_t& bpp_ct_generators_UGX::bpp_H   = c_point_G;
  const point_t& bpp_ct_generators_UGX::bpp_H2  = c_point_X;
}
