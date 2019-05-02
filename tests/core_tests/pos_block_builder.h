// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 

struct pos_block_builder
{
  pos_block_builder();
  void clear();
  
  void step1_init_header(size_t block_height, crypto::hash& prev_block_hash);
  
  void step2_set_txs(const std::vector<currency::transaction>& txs);
  
  void step3_build_stake_kernel(uint64_t stake_output_amount,
    size_t stake_output_gindex,
    const crypto::key_image& output_key_image,
    currency::wide_difficulty_type difficulty,
    const crypto::hash& last_pow_block_hash,
    const crypto::hash& last_pos_block_kernel_hash,
    uint64_t timestamp_lower_bound,
    uint64_t timestamp_window = POS_SCAN_WINDOW,
    uint64_t timestamp_step = POS_SCAN_STEP);
  
  void step4_generate_coinbase_tx(size_t median_size,
    const boost::multiprecision::uint128_t& already_generated_coins,
    const currency::account_public_address &reward_receiver_address,
    const currency::blobdata& extra_nonce = currency::blobdata(),
    size_t max_outs = CURRENCY_MINER_TX_MAX_OUTS,
    const currency::extra_alias_entry& alias = currency::extra_alias_entry(),
    currency::keypair tx_one_time_key = currency::keypair::generate());
  
  void step5_sign(const crypto::public_key& stake_tx_pub_key, size_t stake_tx_out_index, const crypto::public_key& stake_tx_out_pub_key, const currency::account_base& stakeholder_account);

  currency::block         m_block;
  size_t                  m_step;
  size_t                  m_txs_total_size;
  uint64_t                m_total_fee;
  currency::stake_kernel  m_stake_kernel;
  size_t                  m_height;
  size_t                  m_pos_stake_output_gindex;
  uint64_t                m_pos_stake_amount;
};

bool construct_homemade_pos_miner_tx(size_t height, size_t median_size, const boost::multiprecision::uint128_t& already_generated_coins,
  size_t current_block_size,
  uint64_t fee,
  uint64_t pos_stake_amount,
  crypto::key_image pos_stake_keyimage,
  size_t pos_stake_gindex,
  const currency::account_public_address &miner_address,
  currency::transaction& tx,
  const currency::blobdata& extra_nonce = currency::blobdata(),
  size_t max_outs = CURRENCY_MINER_TX_MAX_OUTS,
  const currency::extra_alias_entry& alias = currency::extra_alias_entry(),
  currency::keypair tx_one_time_key = currency::keypair::generate());

bool mine_next_pos_block_in_playtime_sign_cb(currency::core& c, const currency::block& prev_block, const currency::block& coinstake_scr_block, const currency::account_base& acc,
  std::function<bool(currency::block&)> before_sign_cb, currency::block& output);

inline bool mine_next_pos_block_in_playtime(currency::core& c, const currency::block& prev_block, const currency::block& coinstake_scr_block, const currency::account_base& acc,
  std::function<bool(currency::block&)> before_sign_cb, currency::block& output)
{
  return mine_next_pos_block_in_playtime_sign_cb(c, prev_block, coinstake_scr_block, acc, [](currency::block&){ return true; }, output);
}
