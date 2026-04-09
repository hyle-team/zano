// Copyright (c) 2022-2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include "currency_basic.h"
#include "difficulty.h"
#include "pos_mining.h"
#include "wallet/wallet2.h"
#include "crypto/zarcanum.h"
#include "crypto_config.h"

namespace currency
{
  void pos_mining_context::init(const wide_difficulty_type& pos_diff, const stake_modifier_type& sm, bool is_zarcanum)
  {
    this->basic_diff = pos_diff;
    this->sk.stake_modifier = sm;
    this->zarcanum = is_zarcanum;

    if (is_zarcanum)
    {
      this->last_pow_block_id_hashed = crypto::hash_helper_t::hs(CRYPTO_HDS_ZARCANUM_LAST_POW_HASH, this->sk.stake_modifier.last_pow_id);
      this->z_l_div_z_D = crypto::zarcanum_precalculate_z_l_div_z_D(this->basic_diff);
    }
  }

  void pos_mining_context::prepare_entry(uint64_t stake_amount, const crypto::key_image& stake_out_ki, const crypto::public_key& stake_source_tx_pub_key, uint64_t stake_out_in_tx_index,
    const crypto::scalar_t& stake_out_amount_blinding_mask, const crypto::secret_key& view_secret)
  {
    this->stake_amount = stake_amount;
    this->sk.kimage = stake_out_ki;

    if (this->zarcanum)
    {
      crypto::scalar_t v = view_secret;
      crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
      bool r = crypto::generate_key_derivation(stake_source_tx_pub_key, view_secret, derivation); // 8 * v * R
      CHECK_AND_ASSERT_MES_NO_RET(r, "generate_key_derivation failed"); 
      crypto::scalar_t h = AUTO_VAL_INIT(h);
      crypto::derivation_to_scalar(derivation, stake_out_in_tx_index, h.as_secret_key()); // h = Hs(8 * v * R, i)

      // q = Hs(domain_sep, Hs(8 * v * R, i) ) * 8 * v
      this->secret_q = v * 8 * crypto::hash_helper_t::hs(CRYPTO_HDS_OUT_CONCEALING_POINT, h);
      this->stake_out_amount_blinding_mask = stake_out_amount_blinding_mask;
    }
  }

  bool pos_mining_context::do_iteration(uint64_t ts)
  {
    // update stake kernel and calculate it's hash
    this->sk.block_timestamp = ts;
    {
      PROFILE_FUNC("calc_hash");
      this->kernel_hash = crypto::cn_fast_hash(&this->sk, sizeof(this->sk));
    }

    bool found = false;

    if (this->zarcanum)
    {
      crypto::mp::uint256_t lhs;
      crypto::mp::uint512_t rhs;
      {
        PROFILE_FUNC("check_zarcanum");
        found = crypto::zarcanum_check_main_pos_inequality(this->kernel_hash, this->stake_out_amount_blinding_mask, this->secret_q, this->last_pow_block_id_hashed, this->z_l_div_z_D, this->stake_amount, lhs, rhs);
      }
      if (found)
      {
        found = true;
        const boost::multiprecision::uint256_t d_mp = lhs / (crypto::c_zarcanum_z_coeff_mp * this->stake_amount) + 1;
        const boost::multiprecision::uint256_t ba = d_mp * crypto::c_zarcanum_z_coeff_mp * this->stake_amount - lhs;
        const boost::multiprecision::uint256_t l_div_z_D = this->z_l_div_z_D / crypto::c_zarcanum_z_coeff_mp;
        LOG_PRINT_GREEN("Found Zarcanum kernel: amount: " << currency::print_money_brief(this->stake_amount) << ENDL
          << "difficulty:            " << this->basic_diff << ENDL
          << "kernel info:           " << ENDL
          << print_stake_kernel_info(this->sk) 
          << "kernel_hash:           " << this->kernel_hash << ENDL
          << "lhs:                 0x" << crypto::scalar_t(lhs).to_string_as_hex_number() << " = 0x" << std::hex << d_mp << " * 2^64 * " << this->stake_amount << " - 0x" << std::hex << ba << ENDL
          << "rhs:                 0x" << crypto::scalar_t(rhs).to_string_as_hex_number() << ENDL
          << "d:                   0x" << std::hex << d_mp << ENDL
          << "floor(l / z * D):    0x" << std::hex << l_div_z_D
          , LOG_LEVEL_0);
      }
    }
    else
    {
      // old PoS with non-hidden amounts
      currency::wide_difficulty_type final_diff = this->basic_diff / this->stake_amount;
      {
        PROFILE_FUNC("check_hash");
        found = currency::check_hash(this->kernel_hash, final_diff);
      }
      if (found)
      {
        LOG_PRINT_GREEN("Found kernel: amount: " << currency::print_money_brief(this->stake_amount)<< /* ", gindex: " << td.m_global_output_index << */ ENDL
          << "difficulty:            " << this->basic_diff << ", final_diff: " << final_diff << ENDL
          << "kernel info:           " << ENDL
          << print_stake_kernel_info(this->sk) 
          << "kernel_hash(proof):    " << this->kernel_hash,
          LOG_LEVEL_0);
      }
    }

    return found;
  }

}; // namespace currency
