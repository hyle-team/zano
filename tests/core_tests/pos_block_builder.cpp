// Copyright (c) 2014-2022 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "pos_block_builder.h"

using namespace epee;
using namespace currency;

void pos_block_builder::clear()
{
  *this = pos_block_builder{};
}


void pos_block_builder::step1_init_header(const hard_forks_descriptor& hardforks, size_t block_height, crypto::hash& prev_block_hash)
{
  CHECK_AND_ASSERT_THROW_MES(m_step == 0, "pos_block_builder: incorrect step sequence");
  m_block.minor_version = CURRENT_BLOCK_MINOR_VERSION;
  m_block.major_version = hardforks.get_block_major_version_by_height(block_height);
  m_block.timestamp = 0; // to be set at step 3
  m_block.prev_id = prev_block_hash;
  m_block.flags = CURRENCY_BLOCK_FLAG_POS_BLOCK;
  m_block.nonce = 0;

  m_height = block_height;

  m_context.zarcanum = hardforks.is_hardfork_active_for_height(ZANO_HARDFORK_04_ZARCANUM, m_height);

  m_step = 1;
}


void pos_block_builder::step2_set_txs(const std::vector<currency::transaction>& txs)
{
  CHECK_AND_ASSERT_THROW_MES(m_step == 1, "pos_block_builder: incorrect step sequence");

  m_total_fee = 0;
  m_txs_total_size = 0;
  m_block.tx_hashes.reserve(txs.size());
  for (auto& tx : txs)
  {
    uint64_t fee = 0;
    bool r = get_tx_fee(tx, fee);
    CHECK_AND_ASSERT_THROW_MES(r, "wrong transaction passed to step2_set_txs");
    m_total_fee += fee;
    m_txs_total_size += get_object_blobsize(tx);

    m_block.tx_hashes.push_back(get_transaction_hash(tx));
  }

  m_step = 2;
}


void pos_block_builder::step3_build_stake_kernel(
  uint64_t stake_output_amount,
  size_t stake_output_gindex,
  const crypto::key_image& stake_output_key_image,
  currency::wide_difficulty_type difficulty,
  const crypto::hash& last_pow_block_hash,
  const crypto::hash& last_pos_block_kernel_hash,
  uint64_t timestamp_lower_bound,
  uint64_t timestamp_window,
  uint64_t timestamp_step)
{
  step3a(difficulty, last_pow_block_hash, last_pos_block_kernel_hash);

  crypto::public_key stake_source_tx_pub_key  {};
  uint64_t           stake_out_in_tx_index    = UINT64_MAX;
  crypto::scalar_t   stake_out_blinding_mask  {};
  crypto::secret_key view_secret              {};

  step3b(stake_output_amount, stake_output_key_image, stake_source_tx_pub_key, stake_out_in_tx_index, stake_out_blinding_mask, view_secret, stake_output_gindex,
    timestamp_lower_bound, timestamp_window, timestamp_step);
}


void pos_block_builder::step3a(
  currency::wide_difficulty_type difficulty,
  const crypto::hash& last_pow_block_hash,
  const crypto::hash& last_pos_block_kernel_hash
  )
{
  CHECK_AND_ASSERT_THROW_MES(m_step == 2, "pos_block_builder: incorrect step sequence");

  stake_modifier_type sm{};
  sm.last_pow_id = last_pow_block_hash;
  sm.last_pos_kernel_id = last_pos_block_kernel_hash;
  if (last_pos_block_kernel_hash == null_hash)
  {
    bool r = string_tools::parse_tpod_from_hex_string(POS_STARTER_KERNEL_HASH, sm.last_pos_kernel_id);
    CHECK_AND_ASSERT_THROW_MES(r, "Failed to parse POS_STARTER_KERNEL_HASH");
  }

  m_context.init(difficulty, sm, m_context.zarcanum);
  m_step = 31;
}


void pos_block_builder::step3b(
  uint64_t stake_output_amount,
  const crypto::key_image& stake_output_key_image,
  const crypto::public_key& stake_source_tx_pub_key, // zarcanum only
  uint64_t stake_out_in_tx_index,                    // zarcanum only
  const crypto::scalar_t& stake_out_blinding_mask,   // zarcanum only
  const crypto::secret_key& view_secret,             // zarcanum only
  size_t stake_output_gindex,
  uint64_t timestamp_lower_bound,
  uint64_t timestamp_window,
  uint64_t timestamp_step)
{
  CHECK_AND_ASSERT_THROW_MES(m_step == 31, "pos_block_builder: incorrect step sequence");

  m_pos_stake_output_gindex = stake_output_gindex;

  m_context.prepare_entry(stake_output_amount, stake_output_key_image, stake_source_tx_pub_key, stake_out_in_tx_index, stake_out_blinding_mask, view_secret);

  // align timestamp_lower_bound up to timestamp_step boundary if needed
  if (timestamp_lower_bound % timestamp_step != 0)
    timestamp_lower_bound = timestamp_lower_bound - (timestamp_lower_bound % timestamp_step) + timestamp_step;
  bool sk_found = false;
  for (uint64_t ts = timestamp_lower_bound; !sk_found && ts < timestamp_lower_bound + timestamp_window; ts += timestamp_step)
  {
    if (m_context.do_iteration(ts))
      sk_found = true;
  }

  if (!sk_found)
    ASSERT_MES_AND_THROW("Could't build stake kernel");

  // update block header with found timestamp
  m_block.timestamp = m_context.sk.block_timestamp;

  m_step = 3;
}



void pos_block_builder::step4_generate_coinbase_tx(size_t median_size,
  const boost::multiprecision::uint128_t& already_generated_coins,
  const account_public_address &reward_and_stake_receiver_address,
  const blobdata& extra_nonce,
  size_t max_outs,
  const keypair* tx_one_time_key_to_use)
{
  step4_generate_coinbase_tx(median_size, already_generated_coins, reward_and_stake_receiver_address, reward_and_stake_receiver_address, extra_nonce, max_outs, tx_one_time_key_to_use);
}


void pos_block_builder::step4_generate_coinbase_tx(size_t median_size,
  const boost::multiprecision::uint128_t& already_generated_coins,
  const account_public_address &reward_receiver_address,
  const account_public_address &stakeholder_address,
  const blobdata& extra_nonce,
  size_t max_outs,
  const keypair* tx_one_time_key_to_use)
{
  CHECK_AND_ASSERT_THROW_MES(m_step == 3, "pos_block_builder: incorrect step sequence");

  uint64_t tx_version = m_context.zarcanum ? TRANSACTION_VERSION_POST_HF4 : TRANSACTION_VERSION_PRE_HF4;
  pos_entry pe{};
  pe.stake_unlock_time = 0; // TODO
  pe.amount = m_context.stake_amount;

  // generate miner tx using incorrect current_block_size only for size estimation
  uint64_t block_reward_without_fee = 0;
  uint64_t block_reward = 0;
  size_t estimated_block_size = m_txs_total_size;
  bool r = construct_miner_tx(m_height, median_size, already_generated_coins, estimated_block_size, m_total_fee,
    reward_receiver_address, stakeholder_address, m_block.miner_tx, block_reward_without_fee, block_reward, tx_version, extra_nonce, max_outs, true, pe, &m_miner_tx_tgc, tx_one_time_key_to_use);
  CHECK_AND_ASSERT_THROW_MES(r, "construct_miner_tx failed");

  estimated_block_size = m_txs_total_size + get_object_blobsize(m_block.miner_tx);
  size_t cumulative_size = 0;
  for (size_t try_count = 0; try_count != 10; ++try_count)
  {
    r = construct_miner_tx(m_height, median_size, already_generated_coins, estimated_block_size, m_total_fee,
    reward_receiver_address, stakeholder_address, m_block.miner_tx, block_reward_without_fee, block_reward, tx_version, extra_nonce, max_outs, true, pe, &m_miner_tx_tgc, tx_one_time_key_to_use);
    CHECK_AND_ASSERT_THROW_MES(r, "construct_homemade_pos_miner_tx failed");

    cumulative_size = m_txs_total_size + get_object_blobsize(m_block.miner_tx);
    if (cumulative_size == estimated_block_size)
      break; // nice, got what we want
    if (cumulative_size > estimated_block_size)
    {
      estimated_block_size = cumulative_size;
      continue; // one more attempt
    }

    // TODO: implement this rare case
    ASSERT_MES_AND_THROW("step4_generate_coinbase_tx implement todo");
  }
  CHECK_AND_ASSERT_THROW_MES(cumulative_size == estimated_block_size, "step4_generate_coinbase_tx failed to match tx and block size");

  m_step = 4;
}

// supports Zarcanum and mixins
// (se.outputs can be unsorted)
void pos_block_builder::step5_sign(const currency::tx_source_entry& se, const currency::account_keys& stakeholder_keys)
{
  bool r = false;
  CHECK_AND_ASSERT_THROW_MES(m_step == 4, "pos_block_builder: incorrect step sequence");

  // calculate stake_out_derivation and secret_x (derived ephemeral secret key)
  crypto::key_derivation stake_out_derivation = AUTO_VAL_INIT(stake_out_derivation);
  r = crypto::generate_key_derivation(se.real_out_tx_key, stakeholder_keys.view_secret_key, stake_out_derivation);            // d = 8 * v * R
  CHECK_AND_ASSERT_THROW_MES(r, "generate_key_derivation failed");
  crypto::secret_key secret_x = AUTO_VAL_INIT(secret_x);
  crypto::derive_secret_key(stake_out_derivation, se.real_output_in_tx_index, stakeholder_keys.spend_secret_key, secret_x);   // x = Hs(8 * v * R, i) + s

  if (m_context.zarcanum)
  {
    // Zarcanum
    zarcanum_sig& sig = boost::get<zarcanum_sig>(m_block.miner_tx.signatures[0]);
    txin_zc_input& stake_input = boost::get<txin_zc_input>(m_block.miner_tx.vin[1]);

    stake_input.k_image = m_context.sk.kimage;

    size_t prepared_real_out_index = 0;
    std::vector<tx_source_entry::output_entry> prepared_outputs = prepare_outputs_entries_for_key_offsets(se.outputs, se.real_output, prepared_real_out_index);

    std::vector<crypto::CLSAG_GGXXG_input_ref_t> ring;
    for(const auto& el : prepared_outputs)
    {
      stake_input.key_offsets.push_back(el.out_reference);
      ring.emplace_back(el.stealth_address, el.amount_commitment, el.blinded_asset_id, el.concealing_point);
    }

    crypto::hash tx_hash_for_sig = get_block_hash(m_block);

    uint8_t err = 0;
    r = crypto::zarcanum_generate_proof(tx_hash_for_sig, m_context.kernel_hash, ring, m_context.last_pow_block_id_hashed, m_context.sk.kimage,
      secret_x, m_context.secret_q, prepared_real_out_index,m_context.stake_amount, se.real_out_asset_id_blinding_mask,  m_context.stake_out_amount_blinding_mask, -m_miner_tx_tgc.amount_blinding_masks_sum,
      static_cast<crypto::zarcanum_proof&>(sig), &err);
    CHECK_AND_ASSERT_THROW_MES(r, "zarcanum_generate_proof failed, err: " << (int)err);
  }
  else
  {
    CHECK_AND_ASSERT_THROW_MES(se.outputs.size() == 1, "PoS blocks with NLSAG and mixing are not supported atm");

    // old PoS with non-hidden amounts
    NLSAG_sig& sig = boost::get<NLSAG_sig>(m_block.miner_tx.signatures[0]);
    txin_to_key& stake_input = boost::get<txin_to_key>(m_block.miner_tx.vin[1]);

    stake_input.k_image = m_context.sk.kimage;
    stake_input.amount = m_context.stake_amount;
    stake_input.key_offsets.push_back(m_pos_stake_output_gindex);

    crypto::hash block_hash = currency::get_block_hash(m_block);
    std::vector<const crypto::public_key*> keys_ptrs(1, &se.outputs.front().stealth_address);
    sig.s.resize(1);
    crypto::generate_ring_signature(block_hash, m_context.sk.kimage, keys_ptrs, secret_x, 0, sig.s.data());
  }

  m_step = 5;
}

// pre-Zarcanum sign function
void pos_block_builder::step5_sign(const crypto::public_key& stake_tx_pub_key, size_t stake_tx_out_index, const crypto::public_key& stake_tx_out_pub_key,
  const currency::account_base& stakeholder_account)
{
  CHECK_AND_ASSERT_THROW_MES(!m_context.zarcanum, "for zarcanum use another overloading");

  tx_source_entry se{};

  se.real_out_tx_key = stake_tx_pub_key;
  se.real_output_in_tx_index = stake_tx_out_index;
  se.outputs.emplace_back(m_pos_stake_output_gindex, stake_tx_out_pub_key);

  step5_sign(se, stakeholder_account.get_keys());
}
