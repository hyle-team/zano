// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "currency_core/currency_core.h"
#include "currency_core/currency_format_utils.h"
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
    r = c.get_blockchain_storage().create_block_template(miner_addr, miner_addr, extra, false, pe, loc_helper::fill_block_template_func, b, diff, height_from_template);
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
  if (!ctx.is_pos_allowed)
    return false;
 

  std::atomic<bool> stop(false);
  w.scan_pos(ctx, stop, [&w](){size_t blocks_fetched; w.refresh(blocks_fetched); return blocks_fetched == 0; }, w.get_core_runtime_config());
  
  pos_entries_count = ctx.total_items_checked;
  if (ctx.status != API_RETURN_CODE_OK)
    return false;
  
  return w.build_minted_block(ctx, miner_address);
}


struct log_level_scope_changer
{
  log_level_scope_changer(int desired_log_level)
  {
    m_original_log_level = epee::log_space::get_set_log_detalisation_level();
    epee::log_space::get_set_log_detalisation_level(true, desired_log_level);
  }

  ~log_level_scope_changer()
  {
    epee::log_space::get_set_log_detalisation_level(true, m_original_log_level);
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

    tx.signatures.push_back(currency::NLSAG_sig());
    std::vector<crypto::signature>& sigs = boost::get<currency::NLSAG_sig>(tx.signatures.back()).s;

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
        keys_ptrs.push_back(&o.stealth_address);

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
inline bool put_alias_via_tx_to_list(const currency::hard_forks_descriptor& hf, // <-- TODO: remove this
    std::vector<test_event_entry>& events,
    std::list<currency::transaction>& tx_set,
    const currency::block& head_block,
    const currency::account_base& miner_acc,
    const alias_entry_t& ae,
    test_generator& generator)
{
  std::vector<currency::extra_v> extra;
  extra.push_back(ae);
  currency::account_base reward_acc;
  currency::account_keys& ak = const_cast<currency::account_keys&>(reward_acc.get_keys());
  currency::get_aliases_reward_account(ak.account_address, ak.view_secret_key);

  uint64_t alias_reward = 0;
  if (get_block_height(head_block) < ALIAS_MEDIAN_RECALC_INTERWAL)
  {
    alias_reward = currency::get_alias_coast_from_fee(ae.m_alias, ALIAS_VERY_INITAL_COAST); // don't ask why
  }
  else
  {
    LOCAL_ASSERT(false); // not implemented yet, see also all the mess around blockchain_storage::get_tx_fee_median(), get_tx_fee_median_effective_index() etc.
  }

  std::vector<currency::tx_source_entry> sources;
  std::vector<currency::tx_destination_entry> destinations;
  bool r = fill_tx_sources_and_destinations(events, head_block, miner_acc, reward_acc, alias_reward, TESTS_DEFAULT_FEE, 0, sources, destinations);
  CHECK_AND_ASSERT_MES(r, false, "alias: fill_tx_sources_and_destinations failed");

  for(auto& el : destinations)
  {
    if (el.addr.front() == reward_acc.get_public_address())
      el.flags |= currency::tx_destination_entry_flags::tdef_explicit_native_asset_id | currency::tx_destination_entry_flags::tdef_zero_amount_blinding_mask; // all alias-burn outputs must have explicit native asset id and zero amount mask
  }

  uint64_t tx_version = currency::get_tx_version(get_block_height(head_block) + 1, generator.get_hardforks()); // assuming the tx will be in the next block (head_block + 1)
  tx_set.emplace_back();
  r = construct_tx(miner_acc.get_keys(), sources, destinations, extra, empty_attachment, tx_set.back(), tx_version, generator.last_tx_generated_secret_key, 0);
  PRINT_EVENT_N_TEXT(events, "put_alias_via_tx_to_list()");
  events.push_back(tx_set.back());

  // make sure the tx's amount commitments balance each other correctly 
  uint64_t burnt_amount = 0;
  if (!check_native_coins_amount_burnt_in_outs(tx_set.back(), alias_reward, &burnt_amount))
  {
    CHECK_AND_ASSERT_MES(false, false, "alias reward was not found, expected: " << currency::print_money_brief(alias_reward)
      << "; burnt: " << (tx_set.back().version <= TRANSACTION_VERSION_PRE_HF4 ? currency::print_money_brief(burnt_amount) : "hidden") << "; tx: " << get_transaction_hash(tx_set.back()));
  }

  return true;
}

namespace details
{
  template<typename C, typename R>
  void print_tx_size_breakdown_processor(C& container, R& result)
  {
    for(auto& el : container)
      result.emplace_back(el.type().name(), t_serializable_object_to_blob(el).size());
  }
}

inline std::string print_tx_size_breakdown(const currency::transaction& tx)
{
  std::vector<std::pair<std::string, size_t>> sizes;

  details::print_tx_size_breakdown_processor(tx.vin, sizes);
  details::print_tx_size_breakdown_processor(tx.signatures, sizes);
  details::print_tx_size_breakdown_processor(tx.extra, sizes);
  details::print_tx_size_breakdown_processor(tx.vout, sizes);
  details::print_tx_size_breakdown_processor(tx.attachment, sizes);
  details::print_tx_size_breakdown_processor(tx.proofs, sizes);

  size_t len_max = 0;
  for(auto& el : sizes)
  {
    boost::ireplace_all(el.first, "struct ", "");
    boost::ireplace_all(el.first, "currency::", "");
    boost::ireplace_all(el.first, "crypto::", "");
    len_max = std::max(len_max, el.first.size());
  }

  std::stringstream ss;
  ss << "total tx size: " << t_serializable_object_to_blob(tx).size() << ENDL;
  for(auto& el : sizes)
    ss << el.first << std::string(len_max - el.first.size() + 4, ' ') << el.second << ENDL;

  return ss.str();
}

//---------------------------------------------------------------
namespace currency
{
  //this lookup_acc_outs overload is mostly for backward compatibility for tests, ineffective from performance perspective, should not be used in wallet
  inline bool lookup_acc_outs(const currency::account_keys& acc, const currency::transaction& tx, std::vector<currency::wallet_out_info>& outs, uint64_t& sum_of_native_outs, crypto::key_derivation& derivation)
  {
    outs.clear();
    sum_of_native_outs = 0;
    bool res = currency::lookup_acc_outs(acc, tx, outs, derivation);
    for (const auto& o : outs)
    {
      if (o.asset_id == currency::native_coin_asset_id)
      {
        sum_of_native_outs += o.amount;
      }
    }
    return res;
  }

  //this lookup_acc_outs overload is mostly for backward compatibility for tests, ineffective from performance perspective, should not be used in wallet
  inline bool lookup_acc_outs(const currency::account_keys& acc, const currency::transaction& tx, const crypto::public_key& /*tx_onetime_pubkey*/, std::vector<currency::wallet_out_info>& outs, uint64_t& sum_of_native_outs, crypto::key_derivation& derivation)
  {
    sum_of_native_outs = 0;
    bool res = currency::lookup_acc_outs(acc, tx, outs, derivation);
    for (const auto& o : outs)
    {
      if (o.asset_id == currency::native_coin_asset_id)
      {
        sum_of_native_outs += o.amount;
      }
    }
    return res;
  }
 

}