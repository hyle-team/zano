// Copyright (c) 2022 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#pragma once

namespace currency
{

  struct pos_mining_context
  {
                                                      // Zarcanum notation:
    wide_difficulty_type basic_diff;                  //  D
    stake_kernel      sk;
    crypto::scalar_t  last_pow_block_id_hashed;       //  f'
    crypto::scalar_t  secret_q;                       //  q
    boost::multiprecision::uint256_t z_l_div_z_D;     //  z * floor( l / (z * D) )  (max possible value (assuming z=2^64) :  z * 2^252 / (z * 1) ~= 2^252)
    crypto::hash      kernel_hash;                    //  h
    crypto::scalar_t  stake_out_amount_blinding_mask; //  f
    uint64_t          stake_amount;                   //  a

    bool              zarcanum; // false for pre-HF4 classic PoS with explicit amounts 

    void init(const wide_difficulty_type& pos_diff, const stake_modifier_type& sm, bool is_zarcanum);

    void prepare_entry(uint64_t stake_amount, const crypto::key_image& stake_out_ki, const crypto::public_key& stake_source_tx_pub_key, uint64_t stake_out_in_tx_index,
      const crypto::scalar_t& stake_out_blinding_mask, const crypto::secret_key& view_secret);

    bool do_iteration(uint64_t ts);
  };

};
