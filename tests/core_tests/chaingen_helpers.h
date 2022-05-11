// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "currency_core/currency_core.h"
#include "currency_core/miner.h"
#include "wallet/wallet2.h"
#include "test_core_time.h"
#include "chaingen.h"

// chaingen-independent helpers that may be used outside of core_tests (for ex. in functional_tests)


template<typename t_callbacktype>
inline bool mine_next_pow_block_in_playtime(const currency::account_public_address& miner_addr, currency::core& c, t_callbacktype modify_block_cb, currency::block* output = nullptr)
{
  currency::block b = AUTO_VAL_INIT(b);
  currency::wide_difficulty_type diff;
  uint64_t height;
  currency::blobdata extra = AUTO_VAL_INIT(extra);
  bool r = c.get_block_template(b, miner_addr, miner_addr, diff, height, extra);
  CHECK_AND_ASSERT_MES(r, false, "get_block_template failed");

  // adjust block's timestamp to keep difficulty low
  currency::block last_block = AUTO_VAL_INIT(last_block);
  c.get_blockchain_storage().get_top_block(last_block);
  b.timestamp = last_block.timestamp + DIFFICULTY_POW_TARGET;
  // keep global time up with blocks' timestamps
  test_core_time::adjust(b.timestamp);

  modify_block_cb(b);
  r = currency::miner::find_nonce_for_given_block(b, diff, height);
  CHECK_AND_ASSERT_MES(r, false, "find_nonce_for_given_block failed");

  currency::block_verification_context bvc = AUTO_VAL_INIT(bvc);
  c.handle_incoming_block(t_serializable_object_to_blob(b), bvc);
  CHECK_AND_NO_ASSERT_MES(!bvc.m_verification_failed && !bvc.m_marked_as_orphaned && !bvc.m_already_exists, false, "block verification context check failed");

  if (output)
    *output = b;

  return true;
}

inline bool mine_next_pow_block_in_playtime(const currency::account_public_address& miner_addr, currency::core& c, currency::block* output = nullptr)
{
  auto cb = [&](currency::block& b)
  {};
  return mine_next_pow_block_in_playtime(miner_addr, c, cb, output);
}

inline bool mine_next_pow_block_in_playtime_with_given_txs(const currency::account_public_address& miner_addr, currency::core& c, const std::vector<currency::transaction>& txs, const crypto::hash& prev_id, uint64_t height, currency::block* output = nullptr)
{
  struct loc_helper
  {
    static bool fill_block_template_func(currency::block &bl, bool pos, size_t median_size, const boost::multiprecision::uint128_t& already_generated_coins, size_t &total_size, uint64_t &fee, uint64_t height)
    {
      fee = 0;
      total_size = 0;
      const std::vector<currency::transaction>& txs = *txs_accessor();
      for (auto& tx : txs)
      {
        bl.tx_hashes.push_back(currency::get_transaction_hash(tx));
        fee += currency::get_tx_fee(tx);
        total_size += currency::get_object_blobsize(tx);
      }
      return true;
    }

    static const std::vector<currency::transaction>*& txs_accessor()
    {
      static const std::vector<currency::transaction>* txs = nullptr;
      return txs;
    }
  };
  static epee::critical_section s_locker;

  CHECK_AND_ASSERT_MES((height == SIZE_MAX) == (prev_id == currency::null_hash), false, "invalid agruments: height and prev_id should be specified or not specified together");
  currency::block b = AUTO_VAL_INIT(b);
  currency::wide_difficulty_type diff;
  uint64_t height_from_template = 0;
  currency::blobdata extra = AUTO_VAL_INIT(extra);
  currency::pos_entry pe = AUTO_VAL_INIT(pe);
  bool r = false;
  {
    CRITICAL_REGION_LOCAL(s_locker);
    loc_helper::txs_accessor() = &txs;
    r = c.get_blockchain_storage().create_block_template(b, miner_addr, miner_addr, diff, height_from_template, extra, false, pe, loc_helper::fill_block_template_func);
  }
  CHECK_AND_ASSERT_MES(r, false, "get_block_template failed");

  // adjust block's timestamp to keep difficulty low
  currency::block last_block = AUTO_VAL_INIT(last_block);
  if (prev_id == currency::null_hash)
    r = c.get_blockchain_storage().get_top_block(last_block);
  else
    r = c.get_blockchain_storage().get_block_by_hash(prev_id, last_block);
  CHECK_AND_ASSERT_MES(r, false, "failed to obtain last block, prev_id = " << prev_id);
  b.timestamp = last_block.timestamp + DIFFICULTY_POW_TARGET;
  // keep global time up with blocks' timestamps
  test_core_time::adjust(b.timestamp);

  if (prev_id != currency::null_hash)
  {
    b.prev_id = prev_id;
    CHECK_AND_ASSERT_MES(b.miner_tx.vin.size() > 0, false, "invalid miner_tx.vin");
    CHECKED_GET_SPECIFIC_VARIANT(b.miner_tx.vin[0], currency::txin_gen, in, false);
    in.height = height;
    set_tx_unlock_time(b.miner_tx, height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  }
  else
  {
    height = height_from_template;
  }

  r = currency::miner::find_nonce_for_given_block(b, diff, height);
  CHECK_AND_ASSERT_MES(r, false, "find_nonce_for_given_block failed");

  currency::block_verification_context bvc = AUTO_VAL_INIT(bvc);
  for (auto& tx : txs)
  {
    crypto::hash tx_id = currency::get_transaction_hash(tx);
    bvc.m_onboard_transactions[tx_id] = tx;
  }
  c.handle_incoming_block(t_serializable_object_to_blob(b), bvc);
  CHECK_AND_NO_ASSERT_MES(!bvc.m_verification_failed && !bvc.m_marked_as_orphaned && !bvc.m_already_exists, false, "block verification context check failed");

  if (output)
    *output = b;

  return true;
}

inline bool mine_next_pow_block_in_playtime_with_given_txs(const currency::account_public_address& miner_addr, currency::core& c, const std::vector<currency::transaction>& txs, currency::block* output = nullptr)
{
  return mine_next_pow_block_in_playtime_with_given_txs(miner_addr, c, txs, currency::null_hash, SIZE_MAX, output);
}

inline bool mine_next_pow_blocks_in_playtime(const currency::account_public_address& miner_addr, currency::core& c, size_t blocks_count)
{
  for (size_t i = 0; i != blocks_count; i++)
    if (!mine_next_pow_block_in_playtime(miner_addr, c))
      return false;

  return true;
}

inline bool mine_next_pow_blocks_in_playtime_with_given_txs(const currency::account_public_address& miner_addr, const std::vector<currency::transaction>& txs, currency::core& c, size_t blocks_count, const crypto::hash& prev_id)
{
  std::vector<currency::transaction> txs_local = txs;

  crypto::hash prev_id_internal = prev_id;
  currency::block prv_block = AUTO_VAL_INIT(prv_block);
  bool r = c.get_blockchain_storage().get_block_by_hash(prev_id, prv_block);
  CHECK_AND_ASSERT_MES(r, false, "block with id " << prev_id  << " not found");

  for (size_t i = 0; i != blocks_count; i++)
  {
    if (!mine_next_pow_block_in_playtime_with_given_txs(miner_addr, c, txs_local, prev_id_internal, currency::get_block_height(prv_block)+1, &prv_block))
      return false;
    prev_id_internal = get_block_hash(prv_block);
    txs_local.clear();
  }
  return true;
}



// NOTE: stake coins return back to the wallet, newly generated coins go to miner_address (by default they are the same destinations)
inline bool mine_next_pos_block_in_playtime_with_wallet(tools::wallet2& w, const currency::account_public_address& miner_address, size_t& pos_entries_count)
{
  tools::wallet2::mining_context ctx = AUTO_VAL_INIT(ctx);
  w.fill_mining_context(ctx);
  if (!ctx.rsp.is_pos_allowed)
    return false;
  
  pos_entries_count = ctx.sp.pos_entries.size();

  std::atomic<bool> stop(false);
  w.scan_pos(ctx, stop, [&w](){size_t blocks_fetched; w.refresh(blocks_fetched); return blocks_fetched == 0; }, w.get_core_runtime_config());
  
  if (ctx.rsp.status != API_RETURN_CODE_OK)
    return false;
  
  return w.build_minted_block(ctx.sp, ctx.rsp, miner_address);
}

inline uint64_t random_in_range(uint64_t from, uint64_t to)
{
  if (from == to)
    return from;
  CHECK_AND_ASSERT_MES(from < to, 0, "Invalid arguments: from = " << from << ", to = " << to);
  return crypto::rand<uint64_t>() % (to - from + 1) + from;
}

struct log_level_scope_changer
{
  log_level_scope_changer(int desired_log_level)
  {
    m_original_log_level = log_space::get_set_log_detalisation_level();
    log_space::get_set_log_detalisation_level(true, desired_log_level);
  }

  ~log_level_scope_changer()
  {
    log_space::get_set_log_detalisation_level(true, m_original_log_level);
  }

  int m_original_log_level;
};

inline bool resign_tx(const currency::account_keys& sender_keys, const std::vector<currency::tx_source_entry>& sources,
  currency::transaction& tx /* IN/OUT */,
  const currency::transaction* p_source_tx = nullptr /* IN */)
{
  if (sources.size() != tx.vin.size())
    return false;

  tx.signatures.clear();
  crypto::hash tx_prefix_hash = get_transaction_hash(tx);

  size_t i = 0;
  for (const currency::tx_source_entry& se : sources)
  {
    crypto::hash tx_hash_for_signature = prepare_prefix_hash_for_sign(tx, i, tx_prefix_hash);
    if (tx_hash_for_signature == currency::null_hash)
      return false;

    crypto::secret_key in_ephemeral_sec = AUTO_VAL_INIT(in_ephemeral_sec);
    crypto::key_derivation recv_derivation = AUTO_VAL_INIT(recv_derivation);
    if (!crypto::generate_key_derivation(se.real_out_tx_key, sender_keys.view_secret_key, recv_derivation))
      return false;
    crypto::derive_secret_key(recv_derivation, se.real_output_in_tx_index, sender_keys.spend_secret_key, in_ephemeral_sec);

    tx.signatures.push_back(std::vector<crypto::signature>());
    std::vector<crypto::signature>& sigs = tx.signatures.back();

    if (se.is_multisig())
    {
      // multisig
      if (p_source_tx == nullptr)
        return false;

      bool r = false, tx_fully_signed = false;
      size_t ms_input_index = get_multisig_in_index(tx.vin);
      size_t ms_input_sigs_count = boost::get<currency::txin_multisig>(tx.vin[ms_input_index]).sigs_count;
      sigs.resize(ms_input_sigs_count);

      r = sign_multisig_input_in_tx(tx, ms_input_index, sender_keys, *p_source_tx, &tx_fully_signed);
      CHECK_AND_ASSERT_MES(r && !tx_fully_signed, false, "sign_multisig_input_in_tx failed, returned: " << r << ", tx_fully_signed: " << tx_fully_signed);
    }
    else
    {
      std::vector<const crypto::public_key*> keys_ptrs;
      for (const currency::tx_source_entry::output_entry& o : se.outputs)
        keys_ptrs.push_back(&o.second);

      sigs.resize(se.outputs.size());
      generate_ring_signature(tx_hash_for_signature, boost::get<currency::txin_to_key>(tx.vin[i]).k_image, keys_ptrs, in_ephemeral_sec, se.real_output, sigs.data());
    }
    i++;
  }

  return true;
}

inline std::string gen_random_alias(size_t len)
{
  const char allowed_chars[] = "abcdefghijklmnopqrstuvwxyz0123456789";
  char buffer[2048] = "";
  if (len >= sizeof buffer)
    return "";

  crypto::generate_random_bytes(len, buffer);
  buffer[len] = 0;
  for(size_t i = 0; i < len; ++i)
    buffer[i] = allowed_chars[buffer[i] % (sizeof allowed_chars - 1)];
  return buffer;
}

template<typename alias_entry_t>
inline bool put_alias_via_tx_to_list(const currency::hard_forks_descriptor& hf,
    std::vector<test_event_entry>& events,
    std::list<currency::transaction>& tx_set,
    const currency::block& head_block,
    const currency::account_base& miner_acc,
    const alias_entry_t& ae,
    test_generator& generator)
{
  const currency::hard_forks_descriptor& m_hardforks = hf; //a name to feed macro MAKE_TX_MIX_LIST_EXTRA_MIX_ATTR
  std::vector<currency::extra_v> ex;
  ex.push_back(ae);
  currency::account_base reward_acc;
  currency::account_keys& ak = const_cast<currency::account_keys&>(reward_acc.get_keys());
  currency::get_aliases_reward_account(ak.account_address, ak.view_secret_key);

  uint64_t fee_median = generator.get_last_n_blocks_fee_median(get_block_hash(head_block));
  uint64_t reward = currency::get_alias_coast_from_fee(ae.m_alias, fee_median);

  MAKE_TX_MIX_LIST_EXTRA_MIX_ATTR(events, 
    tx_set,
    miner_acc,
    reward_acc,
    reward,
    0,
    head_block,
    CURRENCY_TO_KEY_OUT_RELAXED,
    ex,
    std::vector<currency::attachment_v>());
  

  uint64_t found_alias_reward = get_amount_for_zero_pubkeys(tx_set.back());
  if (found_alias_reward != reward)
  {
    LOCAL_ASSERT(false);
    CHECK_AND_ASSERT_MES(false, false, "wrong transaction constructed, first input value not match alias amount or account");
    return false;
  }

  return true;
}
