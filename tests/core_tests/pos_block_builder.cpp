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
  *this = AUTO_VAL_INIT_T(pos_block_builder);
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
  CHECK_AND_ASSERT_THROW_MES(m_step == 2, "pos_block_builder: incorrect step sequence");
  m_pos_stake_amount = stake_output_amount;
  m_pos_stake_output_gindex = stake_output_gindex;

  m_stake_kernel.kimage = stake_output_key_image;
  m_stake_kernel.block_timestamp = 0;
  m_stake_kernel.stake_modifier.last_pow_id = last_pow_block_hash;
  m_stake_kernel.stake_modifier.last_pos_kernel_id = last_pos_block_kernel_hash;
  if (last_pos_block_kernel_hash == null_hash)
  {
    bool r = string_tools::parse_tpod_from_hex_string(POS_STARTER_KERNEL_HASH, m_stake_kernel.stake_modifier.last_pos_kernel_id);
    CHECK_AND_ASSERT_THROW_MES(r, "Failed to parse POS_STARTER_KERNEL_HASH");
  }

  wide_difficulty_type stake_difficulty = difficulty / stake_output_amount;
  // align timestamp_lower_bound up to timestamp_step boundary if needed
  if (timestamp_lower_bound % timestamp_step != 0)
    timestamp_lower_bound = timestamp_lower_bound - (timestamp_lower_bound % timestamp_step) + timestamp_step;
  bool sk_found = false;
  for (uint64_t ts = timestamp_lower_bound; !sk_found && ts < timestamp_lower_bound + timestamp_window; ts += timestamp_step)
  {
    m_stake_kernel.block_timestamp = ts;
    crypto::hash sk_hash = crypto::cn_fast_hash(&m_stake_kernel, sizeof(m_stake_kernel));
    if (check_hash(sk_hash, stake_difficulty))
    {
      sk_found = true;
    }
  }

  if (!sk_found)
    ASSERT_MES_AND_THROW("Could't build stake kernel");

  // update block header with found timestamp
  m_block.timestamp = m_stake_kernel.block_timestamp;

  m_step = 3;
}

void pos_block_builder::step4_generate_coinbase_tx(size_t median_size,
  const boost::multiprecision::uint128_t& already_generated_coins,
  const account_public_address &reward_and_stake_receiver_address,
  const blobdata& extra_nonce,
  size_t max_outs,
  const extra_alias_entry& alias,
  keypair tx_one_time_key)
{
  step4_generate_coinbase_tx(median_size, already_generated_coins, reward_and_stake_receiver_address, reward_and_stake_receiver_address, extra_nonce, max_outs, alias, tx_one_time_key);
}

void pos_block_builder::step4_generate_coinbase_tx(size_t median_size,
  const boost::multiprecision::uint128_t& already_generated_coins,
  const account_public_address &reward_receiver_address,
  const account_public_address &stakeholder_address,
  const blobdata& extra_nonce,
  size_t max_outs,
  const extra_alias_entry& alias,
  keypair tx_one_time_key)
{
  CHECK_AND_ASSERT_THROW_MES(m_step == 3, "pos_block_builder: incorrect step sequence");

  // generate miner tx using incorrect current_block_size only for size estimation
  size_t estimated_block_size = m_txs_total_size;
  bool r = construct_homemade_pos_miner_tx(m_height, median_size, already_generated_coins, estimated_block_size, m_total_fee, m_pos_stake_amount, m_stake_kernel.kimage,
    m_pos_stake_output_gindex, reward_receiver_address, stakeholder_address, m_block.miner_tx, extra_nonce, max_outs, tx_one_time_key);
  CHECK_AND_ASSERT_THROW_MES(r, "construct_homemade_pos_miner_tx failed");

  estimated_block_size = m_txs_total_size + get_object_blobsize(m_block.miner_tx);
  size_t cumulative_size = 0;
  for (size_t try_count = 0; try_count != 10; ++try_count)
  {
    r = construct_homemade_pos_miner_tx(m_height, median_size, already_generated_coins, estimated_block_size, m_total_fee, m_pos_stake_amount, m_stake_kernel.kimage,
      m_pos_stake_output_gindex, reward_receiver_address, stakeholder_address, m_block.miner_tx, extra_nonce, max_outs, tx_one_time_key);
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

void pos_block_builder::step5_sign(const crypto::public_key& stake_tx_pub_key, size_t stake_tx_out_index, const crypto::public_key& stake_tx_out_pub_key, const currency::account_base& stakeholder_account)
{
  CHECK_AND_ASSERT_THROW_MES(m_step == 4, "pos_block_builder: incorrect step sequence");

  crypto::key_derivation pos_coin_derivation = AUTO_VAL_INIT(pos_coin_derivation);
  bool r = crypto::generate_key_derivation(stake_tx_pub_key, stakeholder_account.get_keys().view_secret_key, pos_coin_derivation); // derivation(tx_pub; view_sec)
  CHECK_AND_ASSERT_THROW_MES(r, "generate_key_derivation failed");

  crypto::secret_key derived_secret_ephemeral_key = AUTO_VAL_INIT(derived_secret_ephemeral_key);
  crypto::derive_secret_key(pos_coin_derivation, stake_tx_out_index, stakeholder_account.get_keys().spend_secret_key, derived_secret_ephemeral_key); // derivation.derive(spend_sec, out_idx) => input ephemeral secret key

  // sign block actually in coinbase transaction
  crypto::hash block_hash = currency::get_block_hash(m_block);
  std::vector<const crypto::public_key*> keys_ptrs(1, &stake_tx_out_pub_key);
  crypto::generate_ring_signature(block_hash, m_stake_kernel.kimage, keys_ptrs, derived_secret_ephemeral_key, 0, &boost::get<currency::NLSAG_sig>(m_block.miner_tx.signatures[0]).s[0]);

  m_step = 5;
}

bool construct_homemade_pos_miner_tx(size_t height, size_t median_size, const boost::multiprecision::uint128_t& already_generated_coins,
  size_t current_block_size,
  uint64_t fee,
  uint64_t pos_stake_amount,
  crypto::key_image pos_stake_keyimage,
  size_t pos_stake_gindex,
  const account_public_address &reward_receiving_address,
  const account_public_address &stakeholder_address,
  transaction& tx,
  const blobdata& extra_nonce /*= blobdata()*/,
  size_t max_outs /*= CURRENCY_MINER_TX_MAX_OUTS*/,
  keypair tx_one_time_key /*= keypair::generate()*/)
{
  boost::value_initialized<transaction> new_tx;
  tx = new_tx;

  tx.version = TRANSACTION_VERSION_PRE_HF4;
  set_tx_unlock_time(tx, height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // calculate block reward
  uint64_t block_reward;
  bool r = get_block_reward(true, median_size, current_block_size, already_generated_coins, block_reward, height);
  CHECK_AND_ASSERT_MES(r, false, "Block is too big");
  block_reward += fee;

  // decompose reward into outputs and populate tx.vout
  std::vector<size_t> out_amounts;
  decompose_amount_into_digits(block_reward, DEFAULT_DUST_THRESHOLD,
    [&out_amounts](uint64_t a_chunk) { out_amounts.push_back(a_chunk); },
    [&out_amounts](uint64_t a_dust) { out_amounts.push_back(a_dust); });

  CHECK_AND_ASSERT_MES(2 <= max_outs, false, "max_out must be greather than 1");
  while (out_amounts.size() + 1 > max_outs)
  {
    out_amounts[out_amounts.size() - 2] += out_amounts.back();
    out_amounts.resize(out_amounts.size() - 1);
  }

  // reward
  bool burn_money = reward_receiving_address.spend_public_key == null_pkey && reward_receiving_address.view_public_key == null_pkey; // if true, burn reward, so no one on Earth can spend them
  for (size_t output_index = 0; output_index < out_amounts.size(); ++output_index)
  {
    txout_to_key tk;
    tk.key = null_pkey; // null means burn money
    tk.mix_attr = 0;

    if (!burn_money)
    {
      r = currency::derive_public_key_from_target_address(reward_receiving_address, tx_one_time_key.sec, output_index, tk.key); // derivation(view_pub; tx_sec).derive(output_index, spend_pub) => output pub key
      CHECK_AND_ASSERT_MES(r, false, "failed to derive_public_key_from_target_address");
    }

    tx_out_bare out;
    out.amount = out_amounts[output_index];
    out.target = tk;
    tx.vout.push_back(out);
  }

  // stake
  burn_money = stakeholder_address.spend_public_key == null_pkey && stakeholder_address.view_public_key == null_pkey; // if true, burn stake
  {
    txout_to_key tk;
    tk.key = null_pkey; // null means burn money
    tk.mix_attr = 0;

    if (!burn_money)
    {
      r = currency::derive_public_key_from_target_address(stakeholder_address, tx_one_time_key.sec, tx.vout.size(), tk.key);
      CHECK_AND_ASSERT_MES(r, false, "failed to derive_public_key_from_target_address");
    }

    tx_out_bare out;
    out.amount = pos_stake_amount;
    out.target = tk;
    tx.vout.push_back(out);
  }

  // take care about extra
  add_tx_pub_key_to_extra(tx, tx_one_time_key.pub);
  if (extra_nonce.size())
    if (!add_tx_extra_userdata(tx, extra_nonce))
      return false;
  
  // populate ins with 1) money-generating and 2) PoS
  txin_gen in;
  in.height = height;
  tx.vin.push_back(in);

  txin_to_key posin;
  posin.amount = pos_stake_amount;
  posin.key_offsets.push_back(pos_stake_gindex);
  posin.k_image = pos_stake_keyimage;
  tx.vin.push_back(posin);
  //reserve place for ring signature
  tx.signatures.resize(1);
  boost::get<currency::NLSAG_sig>(tx.signatures[0]).s.resize(posin.key_offsets.size());

  return true;
}

bool mine_next_pos_block_in_playtime_sign_cb(currency::core& c, const currency::block& prev_block, const currency::block& coinstake_scr_block, const currency::account_base& acc,
  std::function<bool(currency::block&)> before_sign_cb, currency::block& output)
{
  blockchain_storage& bcs = c.get_blockchain_storage();

  // these values (median and diff) are correct only for the next main chain block, it's incorrect for altblocks, especially for old altblocks
  // but for now we assume they will work fine
  uint64_t block_size_median = bcs.get_current_comulative_blocksize_limit() / 2;
  currency::wide_difficulty_type difficulty = bcs.get_next_diff_conditional(true);

  crypto::hash prev_id = get_block_hash(prev_block);
  size_t height = get_block_height(prev_block) + 1;

  block_extended_info bei = AUTO_VAL_INIT(bei);
  bool r = bcs.get_block_extended_info_by_hash(prev_id, bei);
  CHECK_AND_ASSERT_MES(r, false, "get_block_extended_info_by_hash failed for hash = " << prev_id);


  const transaction& stake = coinstake_scr_block.miner_tx;
  crypto::public_key stake_tx_pub_key = get_tx_pub_key_from_extra(stake);
  size_t stake_output_idx = 0;
  size_t stake_output_gidx = 0;
  uint64_t stake_output_amount =boost::get<currency::tx_out_bare>( stake.vout[stake_output_idx]).amount;
  crypto::key_image stake_output_key_image;
  keypair kp;
  generate_key_image_helper(acc.get_keys(), stake_tx_pub_key, stake_output_idx, kp, stake_output_key_image);
  crypto::public_key stake_output_pubkey = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(stake.vout[stake_output_idx]).target).key;

  pos_block_builder pb;
  pb.step1_init_header(bcs.get_core_runtime_config().hard_forks, height, prev_id);
  pb.step2_set_txs(std::vector<transaction>());
  pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, difficulty, prev_id, null_hash, prev_block.timestamp);
  pb.step4_generate_coinbase_tx(block_size_median, bei.already_generated_coins, acc.get_public_address());

  if (!before_sign_cb(pb.m_block))
    return false;

  pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, acc);
  output = pb.m_block;

  return true;
}
