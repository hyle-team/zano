// Copyright (c) 2022 Zano Project
// Copyright (c) 2022 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Note: This file originates from tests/functional_tests/crypto_tests.cpp 
#include "zarcanum.h"
namespace crypto
{
  const scalar_t      c_zarcanum_z_coeff_s  = c_scalar_2p64;
  const mp::uint256_t c_zarcanum_z_coeff_mp = c_zarcanum_z_coeff_s.as_boost_mp_type<mp::uint256_t>();

  mp::uint256_t zarcanum_precalculate_l_div_z_D(const mp::uint128_t& pos_difficulty)
  {
    return c_scalar_L.as_boost_mp_type<mp::uint256_t>() / (c_zarcanum_z_coeff_mp * pos_difficulty); // == floor( l / (z * D) )
  }

  mp::uint256_t zarcanum_precalculate_z_l_div_z_D(const mp::uint128_t& pos_difficulty)
  {
    return c_zarcanum_z_coeff_mp * (c_scalar_L.as_boost_mp_type<mp::uint256_t>() / (c_zarcanum_z_coeff_mp * pos_difficulty)); // == z * floor( l / (z * D) )
  }

  bool zarcanum_check_main_pos_inequality(const hash& kernel_hash, const scalar_t& blinding_mask, const scalar_t& secret_q,
    const scalar_t& last_pow_block_id_hashed, const mp::uint256_t& z_l_div_z_D, uint64_t stake_amount, mp::uint256_t& lhs, mp::uint512_t& rhs)
  {
    scalar_t lhs_s = scalar_t(kernel_hash) * (blinding_mask + secret_q + last_pow_block_id_hashed); // == h * (f + q + f') mod l
    lhs = lhs_s.as_boost_mp_type<mp::uint256_t>();
    rhs = static_cast<mp::uint512_t>(z_l_div_z_D) * stake_amount; // == floor( l / (z * D) ) * z * a

    return lhs < rhs;  //  h * (f + q + f') mod l   <   floor( l / (z * D) ) * z * a
  }

} // namespace crypto
