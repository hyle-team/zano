// Copyright (c) 2014-2023 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "include_base_utils.h"
#include <boost/foreach.hpp>
#ifndef MOBILE_WALLET_BUILD
  #include <boost/locale.hpp>
#endif
using namespace epee;

#include "print_fixed_point_helper.h"
#include "currency_format_utils.h"
#include "currency_format_utils_transactions.h"
#include "serialization/binary_utils.h"
#include "serialization/stl_containers.h"
#include "currency_core/currency_config.h"
#include "miner.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "common/int-util.h"
#include "common/base58.h"
#include "bc_offers_service_basic.h"
#include "bc_payments_id_service.h"
#include "bc_escrow_service.h"
#include "bc_attachments_helpers.h"
#include "bc_block_datetime_service.h"
#include "genesis.h"
#include "genesis_acc.h"
#include "common/mnemonic-encoding.h"
#include "crypto/bitcoin/sha256_helper.h"
#include "crypto_config.h"
#include "wallet/wallet_debug_events_definitions.h"


#define DBG_VAL_PRINT(x) ((void)0) // LOG_PRINT_CYAN(std::setw(42) << std::left << #x ":" << x, LOG_LEVEL_0)

namespace currency
{

  //---------------------------------------------------------------
  bool add_tx_extra_alias(transaction& tx, const extra_alias_entry& alinfo)
  {
    tx.extra.push_back(alinfo);
    return true;
  }
  //--------------------------------------------------------------------------------
  bool generate_asset_surjection_proof(const crypto::hash& context_hash, bool has_non_zc_inputs, tx_generation_context& ogc, zc_asset_surjection_proof& result)
  {
    bool r = false;
    size_t outs_count = ogc.blinded_asset_ids.size();
    CHECK_AND_ASSERT_MES(outs_count > 0, false, "blinded_asset_ids shouldn't be empty");
    CHECK_AND_ASSERT_MES(outs_count == ogc.asset_id_blinding_masks.size(), false, "asset_id_blinding_masks != outs_count");

    size_t zc_ins_count = ogc.pseudo_outs_blinded_asset_ids.size();
    if (zc_ins_count == 0)
    {
      // if there's no ZC inputs all the outputs must be native coins with explicit asset ids
      for(size_t j = 0; j < ogc.blinded_asset_ids.size(); ++j)
      {
        CHECK_AND_ASSERT_MES(ogc.blinded_asset_ids[j] == currency::native_coin_asset_id_pt, false, "no ZC ins: out #" << j << " has a non-explicit asset id");
        CHECK_AND_ASSERT_MES(ogc.asset_id_blinding_masks[j] == 0, false, "no ZC ins: out #" << j << " has non-zero asset id blinding mask");
      }
      return true;
    }
    
    // okay, have some ZC inputs

    CHECK_AND_ASSERT_MES(zc_ins_count == ogc.pseudo_outs_plus_real_out_blinding_masks.size(), false, "zc_ins_count != pseudo_outs_plus_real_out_blinding_masks");
    CHECK_AND_ASSERT_MES(zc_ins_count == ogc.real_zc_ins_asset_ids.size(), false, "zc_ins_count != real_zc_ins_asset_ids");

    // ins
    //ogc.pseudo_outs_blinded_asset_ids;             // T^p_i = T_real + r'_i * X
    //ogc.pseudo_outs_plus_real_out_blinding_masks;  // r_pi + r'_j

    // outs
    //ogc.blinded_asset_ids;                         // T'_j = H_j + s_j * X
    //ogc.asset_id_blinding_masks;                   // s_j

    for(size_t j = 0; j < outs_count; ++j)
    {
      const crypto::public_key H = ogc.asset_ids[j].to_public_key();
      const crypto::point_t& T = ogc.blinded_asset_ids[j];
      
      std::vector<crypto::point_t> ring;
      ring.reserve(zc_ins_count);
      size_t secret_index = SIZE_MAX;
      crypto::scalar_t secret = -ogc.asset_id_blinding_masks[j];

      for(size_t i = 0; i < zc_ins_count; ++i)
      {
        ring.emplace_back(ogc.pseudo_outs_blinded_asset_ids[i] - T);
        if (secret_index == SIZE_MAX && ogc.real_zc_ins_asset_ids[i] == H)
        {
          secret_index = i;
          secret += ogc.pseudo_outs_plus_real_out_blinding_masks[secret_index];
        }
      }

      // additional ring member for native coins in txs with non-zc inputs
      if (has_non_zc_inputs)
      {
        ring.emplace_back(currency::native_coin_asset_id_pt - T);
        if (secret_index == SIZE_MAX && H == native_coin_asset_id)
          secret_index = ring.size() - 1;
      }

      // additional ring member for asset emitting operation (which has asset operation commitment in the inputs part)
      if (!ogc.ao_amount_blinding_mask.is_zero() && !ogc.ao_commitment_in_outputs)
      {
        ring.emplace_back(ogc.ao_asset_id_pt - T);
        if (secret_index == SIZE_MAX && H == ogc.ao_asset_id)
          secret_index = ring.size() - 1;
      }

      CHECK_AND_ASSERT_MES(secret_index != SIZE_MAX, false, "out #" << j << ": can't find a corresponding asset id in inputs, asset id: " << H);

      result.bge_proofs.emplace_back(crypto::BGE_proof_s{});
      uint8_t err = 0;
      r = crypto::generate_BGE_proof(context_hash, ring, secret, secret_index, result.bge_proofs.back(), &err);
      CHECK_AND_ASSERT_MES(r, false, "out #" << j << ": generate_BGE_proof failed with err=" << (int)err);
    }

    return true;
  }
  //--------------------------------------------------------------------------------
  bool verify_asset_surjection_proof(const transaction& tx, const crypto::hash& tx_id)
  {
    if (tx.version <= TRANSACTION_VERSION_PRE_HF4)
      return true;

    size_t outs_count = tx.vout.size();

    bool has_ZC_inputs = false;
    bool has_non_ZC_inputs = false;
    for(const auto& in : tx.vin)
    {
      if (in.type() == typeid(txin_zc_input))
        has_ZC_inputs = true;
      else
        has_non_ZC_inputs = true;
    }

    if (!has_ZC_inputs)
    {
      // no ZC ins -- just make sure there's only native outputs with explicit asset ids
      for(size_t j = 0; j < outs_count; ++j)
      {
        CHECK_AND_ASSERT_MES(boost::get<tx_out_zarcanum>(tx.vout[j]).blinded_asset_id == native_coin_asset_id_1div8, false, "output #" << j << " has a non explicitly native asset id");
      }
      return true;
    }

    // tx has ZC inputs
    const zc_asset_surjection_proof& sig = get_type_in_variant_container_by_ref<const zc_asset_surjection_proof>(tx.proofs); // order of proofs and uniqueness of zc_asset_surjection_proof should be check before on prevalidation
    CHECK_AND_ASSERT_MES(sig.bge_proofs.size() == outs_count, false, "ASP count: " << sig.bge_proofs.size() << ", outputs: " << outs_count << " => missmatch");

    // make a ring
    std::vector<crypto::point_t> pseudo_outs_blinded_asset_ids;
    for(const auto& sig : tx.signatures)
    {
      if (sig.type() == typeid(ZC_sig))
        pseudo_outs_blinded_asset_ids.emplace_back(crypto::point_t(boost::get<ZC_sig>(sig).pseudo_out_blinded_asset_id).modify_mul8());
    }
    if (has_non_ZC_inputs)
      pseudo_outs_blinded_asset_ids.emplace_back(currency::native_coin_asset_id_pt); // additional ring member for txs with non-zc inputs

    asset_descriptor_operation ado{};
    if (is_asset_emitting_transaction(tx, &ado))
    {
      crypto::point_t asset_id_pt{};
      CHECK_AND_ASSERT_MES(get_or_calculate_asset_id(ado, &asset_id_pt, nullptr), false, "get_or_calculate_asset_id failed"); // TODO @#@# expensive operation, consider caching 
      pseudo_outs_blinded_asset_ids.emplace_back(asset_id_pt); // additional ring member for asset emitting tx
    }

    for(size_t j = 0; j < outs_count; ++j)
    {
      crypto::point_t blinded_asset_id(boost::get<tx_out_zarcanum>(tx.vout[j]).blinded_asset_id);
      blinded_asset_id.modify_mul8();

      // TODO @#@# remove this redundant conversion to pubkey and back
      std::vector<crypto::public_key> ring(pseudo_outs_blinded_asset_ids.size());
      std::vector<const crypto::public_key*> ring_pointers(pseudo_outs_blinded_asset_ids.size());
      for(size_t i = 0, n = pseudo_outs_blinded_asset_ids.size(); i < n; ++i)
      {
        ring[i] = ((crypto::c_scalar_1div8 * (pseudo_outs_blinded_asset_ids[i] - blinded_asset_id)).to_public_key());
        ring_pointers[i] = &ring[i];
      }

      uint8_t err = 0;
      CHECK_AND_ASSERT_MES(crypto::verify_BGE_proof(tx_id, ring_pointers, sig.bge_proofs[j], &err), false, "verify_BGE_proof failed, err = " << (int)err);
    }

    return true;
  }
  //--------------------------------------------------------------------------------
  bool generate_zc_outs_range_proof(const crypto::hash& context_hash, size_t out_index_start, const tx_generation_context& outs_gen_context,
    const std::vector<tx_out_v>& vouts, zc_outs_range_proof& result)
  {
    size_t outs_count = outs_gen_context.amounts.size();
    // TODO @#@# reconsider this check CHECK_AND_ASSERT_MES(gen_context.check_sizes(outs_count), false, "");
    CHECK_AND_ASSERT_MES(out_index_start + outs_count == vouts.size(), false, "");

    // prepare data for aggregation proof
    std::vector<crypto::point_t> amount_commitments_for_rp_aggregation; // E' = amount * U + y' * G
    crypto::scalar_vec_t y_primes; // y'
    for (size_t out_index = out_index_start, i = 0; i < outs_count; ++out_index, ++i)
    {
      crypto::scalar_t y_prime = crypto::scalar_t::random();
      amount_commitments_for_rp_aggregation.emplace_back(outs_gen_context.amounts[i] * crypto::c_point_U + y_prime * crypto::c_point_G); // E'_j = e_j * U + y'_j * G
      y_primes.emplace_back(std::move(y_prime));
    }

    // aggregation proof
    uint8_t err = 0;
    bool r = crypto::generate_vector_UG_aggregation_proof(context_hash, outs_gen_context.amounts, outs_gen_context.amount_blinding_masks, y_primes,
      outs_gen_context.amount_commitments,
      amount_commitments_for_rp_aggregation,
      outs_gen_context.blinded_asset_ids, result.aggregation_proof, &err);
    CHECK_AND_ASSERT_MES(r, false, "generate_vector_UG_aggregation_proof failed with error " << (int)err);

    // aggregated range proof
    std::vector<const crypto::public_key*> commitments_1div8(result.aggregation_proof.amount_commitments_for_rp_aggregation.size());
    for(size_t i = 0, sz = result.aggregation_proof.amount_commitments_for_rp_aggregation.size(); i < sz; ++i)
      commitments_1div8[i] = &result.aggregation_proof.amount_commitments_for_rp_aggregation[i];

    err = 0;
    r = crypto::bpp_gen<crypto::bpp_crypto_trait_ZC_out>(outs_gen_context.amounts, y_primes, commitments_1div8, result.bpp, &err);
    CHECK_AND_ASSERT_MES(r, false, "bpp_gen failed with error " << (int)err);

    return true;
  }
  //---------------------------------------------------------------
  bool verify_multiple_zc_outs_range_proofs(const std::vector<zc_outs_range_proofs_with_commitments>& range_proofs)
  {
    if (range_proofs.empty())
      return true;

    std::vector<crypto::bpp_sig_commit_ref_t> sigs;
    sigs.reserve(range_proofs.size());
    for(auto& el : range_proofs)
      sigs.emplace_back(el.range_proof.bpp, el.amount_commitments);

    uint8_t err = 0;
    bool r = crypto::bpp_verify<crypto::bpp_crypto_trait_ZC_out>(sigs, &err);
    CHECK_AND_ASSERT_MES(r, false, "bpp_verify failed with error " << (int)err);

    return true;
  }
  //--------------------------------------------------------------------------------
  wide_difficulty_type correct_difficulty_with_sequence_factor(size_t sequence_factor, wide_difficulty_type diff)
  {
    //delta=delta*(0.75^n)
    for (size_t i = 0; i != sequence_factor; i++)
    {
      diff = diff - diff / 4;
    }
    return diff;
  }
  //------------------------------------------------------------------
  bool add_random_derivation_hints_and_put_them_to_tx(uint8_t tx_flags, const std::set<uint16_t>& existing_derivation_hints, std::set<uint16_t>& new_derivation_hints, transaction& tx)
  {
    if (existing_derivation_hints.size() > tx.vout.size())
    {
      if (tx.version < TRANSACTION_VERSION_POST_HF4 && (tx_flags & TX_FLAG_SIGNATURE_MODE_SEPARATE)) // for pre-HF4 consolidated txs just skip if all hints are already added
        return true;
      return false;
    }

    size_t hints_to_be_added = tx.vout.size() - existing_derivation_hints.size();

    size_t attempts_cnt = 0, attempts_max = 4096;
    while(new_derivation_hints.size() < hints_to_be_added && ++attempts_cnt < attempts_max)
    {
      uint16_t hint = crypto::rand<uint16_t>();
      if (existing_derivation_hints.count(hint) == 0)
        new_derivation_hints.insert(hint);
    }

    if (attempts_cnt == attempts_max)
      return false;

    for(auto& el : new_derivation_hints) // iterating in sorted sequence
      tx.extra.push_back(make_tx_derivation_hint_from_uint16(el));

    return true;
  }
  //---------------------------------------------------------------
  bool generate_tx_balance_proof(const transaction &tx, const crypto::hash& tx_id, const tx_generation_context& ogc, uint64_t block_reward_for_miner_tx, currency::zc_balance_proof& proof)
  {
    CHECK_AND_ASSERT_MES(tx.version > TRANSACTION_VERSION_PRE_HF4, false, "unsupported tx.version: " << tx.version);
    CHECK_AND_ASSERT_MES(count_type_in_variant_container<zc_balance_proof>(tx.proofs) == 0, false, "zc_balance_proof is already present");
    bool r = false;

    uint64_t bare_inputs_sum = block_reward_for_miner_tx;
    size_t zc_inputs_count = 0;
    for(auto& vin : tx.vin)
    {
      VARIANT_SWITCH_BEGIN(vin);
      VARIANT_CASE_CONST(txin_to_key, tk)
        bare_inputs_sum += tk.amount;
      VARIANT_CASE_CONST(txin_htlc, foo);
        CHECK_AND_ASSERT_MES(false, false, "unexpected txin_htlc input");
      VARIANT_CASE_CONST(txin_multisig, ms);
        //bare_inputs_sum += ms.amount;
        CHECK_AND_ASSERT_MES(false, false, "unexpected txin_multisig input"); // TODO @#@# check support for multisig inputs
      VARIANT_CASE_CONST(txin_zc_input, foo);
        ++zc_inputs_count;
      VARIANT_SWITCH_END();
    }

    uint64_t fee = 0;
    CHECK_AND_ASSERT_MES(get_tx_fee(tx, fee), false, "unable to get tx fee");

    if (zc_inputs_count == 0)
    {
      // no ZC inputs => all inputs are bare inputs; all outputs have explicit asset_id = native_coin_asset_id; in main balance equation we only need to cancel out G-component
      CHECK_AND_ASSERT_MES(count_type_in_variant_container<ZC_sig>(tx.signatures) == 0, false, "ZC_sig is unexpected");
      CHECK_AND_ASSERT_MES(ogc.asset_id_blinding_mask_x_amount_sum.is_zero(), false, "it's expected that all asset ids for this tx are obvious and thus explicit"); // because this tx has no ZC inputs => all outs clearly have native asset id
      CHECK_AND_ASSERT_MES(ogc.ao_amount_blinding_mask.is_zero(), false, "asset emmission is not allowed for txs without ZC inputs");

      // (sum(bare inputs' amounts) - fee) * H + sum(pseudo out amount commitments) - sum(outputs' commitments) = lin(G)

      crypto::point_t commitment_to_zero = (crypto::scalar_t(bare_inputs_sum) - crypto::scalar_t(fee)) * currency::native_coin_asset_id_pt - ogc.amount_commitments_sum;
      crypto::scalar_t secret_x = -ogc.amount_blinding_masks_sum;
#ifndef NDEBUG
      CHECK_AND_ASSERT_MES(commitment_to_zero == secret_x * crypto::c_point_G, false, "internal error: commitment_to_zero is malformed (G)");
#endif
      r = crypto::generate_double_schnorr_sig<crypto::gt_G, crypto::gt_G>(tx_id, commitment_to_zero, secret_x, ogc.tx_pub_key_p, ogc.tx_key.sec, proof.dss);
      CHECK_AND_ASSERT_MES(r, false, "generate_double_schnorr_sig (G, G) failed");
    }
    else // i.e. zc_inputs_count != 0
    {
      // there're ZC inputs => in main balance equation we only need to cancel out X-component, because G-component cancelled out by choosing blinding mask for the last pseudo out amount commitment

      crypto::point_t commitment_to_zero = (crypto::scalar_t(bare_inputs_sum) - crypto::scalar_t(fee)) * currency::native_coin_asset_id_pt + ogc.pseudo_out_amount_commitments_sum + (ogc.ao_commitment_in_outputs ? -ogc.ao_amount_commitment : ogc.ao_amount_commitment) - ogc.amount_commitments_sum;
      crypto::scalar_t secret_x = ogc.real_in_asset_id_blinding_mask_x_amount_sum - ogc.asset_id_blinding_mask_x_amount_sum;

      DBG_VAL_PRINT(bare_inputs_sum);
      DBG_VAL_PRINT(fee);
      DBG_VAL_PRINT(ogc.pseudo_out_amount_commitments_sum);
      DBG_VAL_PRINT((int)ogc.ao_commitment_in_outputs);
      DBG_VAL_PRINT(ogc.ao_amount_commitment);
      DBG_VAL_PRINT(ogc.amount_commitments_sum);
      DBG_VAL_PRINT(ogc.real_in_asset_id_blinding_mask_x_amount_sum);
      DBG_VAL_PRINT(ogc.asset_id_blinding_mask_x_amount_sum);
      DBG_VAL_PRINT(commitment_to_zero);
      DBG_VAL_PRINT(secret_x);

#ifndef NDEBUG
      bool commitment_to_zero_is_sane = commitment_to_zero == secret_x * crypto::c_point_X;
      CHECK_AND_ASSERT_MES(commitment_to_zero_is_sane, false, "internal error: commitment_to_zero is malformed (X)");
#endif
      r = crypto::generate_double_schnorr_sig<crypto::gt_X, crypto::gt_G>(tx_id, commitment_to_zero, secret_x, ogc.tx_pub_key_p, ogc.tx_key.sec, proof.dss);
      CHECK_AND_ASSERT_MES(r, false, "genergenerate_double_schnorr_sigate_schnorr_sig (X, G) failed");
    }

    return true;
  }
  //------------------------------------------------------------------
  bool apply_unlock_time(const std::vector<tx_destination_entry>& destinations, transaction& tx)
  {
    currency::etc_tx_details_unlock_time2 unlock_time2 = AUTO_VAL_INIT(unlock_time2);
    unlock_time2.unlock_time_array.resize(destinations.size());
    bool found_unlock_time = false;
    for (size_t i = 0; i != unlock_time2.unlock_time_array.size(); i++)
    {
      if (destinations[i].unlock_time)
      {
        found_unlock_time = true;
        unlock_time2.unlock_time_array[i] = destinations[i].unlock_time;
      }
    }
    if (found_unlock_time)
    {
      tx.extra.push_back(unlock_time2);
    }

    return true;
  }
  //---------------------------------------------------------------
  bool construct_miner_tx(size_t height, size_t median_size, const boost::multiprecision::uint128_t& already_generated_coins,
    size_t current_block_size,
    uint64_t fee,
    const account_public_address &miner_address,
    const account_public_address &stakeholder_address,
    transaction& tx,
    uint64_t& block_reward_without_fee,
    uint64_t& block_reward,
    uint64_t tx_version,
    const blobdata& extra_nonce               /* = blobdata() */,
    size_t max_outs                           /* = CURRENCY_MINER_TX_MAX_OUTS */,
    bool pos                                  /* = false */,
    const pos_entry& pe                       /* = pos_entry() */,  // only pe.stake_unlock_time and pe.stake_amount are used now, TODO: consider refactoring -- sowle
    tx_generation_context* ogc_ptr            /* = nullptr */,
    const keypair* tx_one_time_key_to_use     /* = nullptr */, 
    const std::vector<tx_destination_entry>& destinations_ /* = std::vector<tx_destination_entry>() */
  )
  {
    std::vector<tx_destination_entry> destinations = destinations_;

    bool r = false;

    if (!get_block_reward(pos, median_size, current_block_size, already_generated_coins, block_reward_without_fee, height))
    {
      LOG_ERROR("Block is too big");
      return false;
    }

    block_reward = block_reward_without_fee;
    // before HF4: add tx fee to the block reward; after HF4: burn it
    if (tx_version < TRANSACTION_VERSION_POST_HF4)
    {
      block_reward += fee;
    }
      
    if (!destinations.size())
    {
      //
      // prepare destinations
      //
      // 1. split block_reward into out_amounts
      std::vector<uint64_t> out_amounts;
      if (tx_version > TRANSACTION_VERSION_PRE_HF4)
      {
        // randomly split into CURRENCY_TX_MIN_ALLOWED_OUTS outputs for PoW block, or for PoS block only if the stakeholder address differs
        // (otherwise for PoS miner tx there will be ONE output with amount = stake_amount + reward)
        if (!pos || miner_address != stakeholder_address)
          decompose_amount_randomly(block_reward, [&](uint64_t a) { out_amounts.push_back(a); }, CURRENCY_TX_MIN_ALLOWED_OUTS);
      }
      else
      {
        // non-hidden outs: split into digits
        decompose_amount_into_digits(block_reward, DEFAULT_DUST_THRESHOLD,
          [&out_amounts](uint64_t a_chunk) { out_amounts.push_back(a_chunk); },
          [&out_amounts](uint64_t a_dust) { out_amounts.push_back(a_dust); });
        CHECK_AND_ASSERT_MES(1 <= max_outs, false, "max_out must be non-zero");
        while (max_outs < out_amounts.size())
        {
          out_amounts[out_amounts.size() - 2] += out_amounts.back();
          out_amounts.resize(out_amounts.size() - 1);
        }
      }
      // 2. construct destinations using out_amounts

      for (auto a : out_amounts)
      {
        tx_destination_entry de = AUTO_VAL_INIT(de);
        de.addr.push_back(miner_address);
        de.amount = a;
        de.flags |= tx_destination_entry_flags::tdef_explicit_native_asset_id; // don't use asset id blinding as it's obvious which asset it is
        if (pe.stake_unlock_time && pe.stake_unlock_time > height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW)
        {
          //this means that block is creating after hardfork_1 and unlock_time is needed to set for every destination separately
          de.unlock_time = height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW;
        }
        destinations.push_back(de);
      }
    }

    if (pos)
    {
      uint64_t stake_lock_time = 0;
      if (pe.stake_unlock_time && pe.stake_unlock_time > height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW)
        stake_lock_time = pe.stake_unlock_time;
      uint64_t amount = destinations.empty() ? pe.amount + block_reward : pe.amount;  // combine stake and reward into one output if no destinations were generated above
      destinations.push_back(tx_destination_entry(amount, stakeholder_address, stake_lock_time));
      destinations.back().flags |= tx_destination_entry_flags::tdef_explicit_native_asset_id; // don't use asset id blinding as it's obvious which asset it is
    }

    CHECK_AND_ASSERT_MES(destinations.size() <= CURRENCY_TX_MAX_ALLOWED_OUTS || height == 0, false, "Too many outs (" << destinations.size() << ")! Miner tx can't be constructed.");
    tx = AUTO_VAL_INIT_T(transaction);
    tx.version = tx_version;

    tx_generation_context tx_gen_context{};
    tx_gen_context.set_tx_key(tx_one_time_key_to_use ? *tx_one_time_key_to_use : keypair::generate());
    add_tx_pub_key_to_extra(tx, tx_gen_context.tx_key.pub);
    if (extra_nonce.size())
      if (!add_tx_extra_userdata(tx, extra_nonce))
        return false;

    //at this moment we do apply_unlock_time only for coin_base transactions 
    apply_unlock_time(destinations, tx);
    //we always add extra_padding with 2 bytes length to make possible for get_block_template to adjust cumulative size
    tx.extra.push_back(extra_padding());

    size_t zc_ins_count = 0;

    // input #0: txin_gen
    txin_gen in;
    in.height = height;
    tx.vin.push_back(in);

    // input #1: stake (for PoS blocks only)
    if (pos)
    {
      if (tx.version > TRANSACTION_VERSION_PRE_HF4 /* && stake is zarcanum */)
      {
        // just placeholders, they will be filled in wallet2::prepare_and_sign_pos_block()
        tx.vin.emplace_back(std::move(txin_zc_input()));
        tx.signatures.emplace_back(std::move(zarcanum_sig()));
        ++zc_ins_count;
      }
      else
      {
        // old fashioned non-hidden amount direct spend PoS scheme
        // just placeholders, they will be filled in wallet2::prepare_and_sign_pos_block()
        tx.vin.emplace_back(std::move(txin_to_key()));
        tx.signatures.emplace_back(std::move(NLSAG_sig()));
      }
    }

    // fill outputs
    tx_gen_context.resize(zc_ins_count, destinations.size()); // auxiliary data for each output
    uint64_t output_index = 0;
    std::set<uint16_t> derivation_hints;
    for (auto& d : destinations)
    {
      finalized_tx result = AUTO_VAL_INIT(result);
      uint8_t tx_outs_attr = 0;
      r = construct_tx_out(d, tx_gen_context.tx_key.sec, output_index, tx, derivation_hints, account_keys(),
        tx_gen_context.asset_id_blinding_masks[output_index], tx_gen_context.amount_blinding_masks[output_index],
        tx_gen_context.blinded_asset_ids[output_index], tx_gen_context.amount_commitments[output_index], result, tx_outs_attr);
      CHECK_AND_ASSERT_MES(r, false, "construct_tx_out failed, output #" << output_index << ", amount: " << print_money_brief(d.amount));
      tx_gen_context.amounts[output_index] = d.amount;
      tx_gen_context.asset_ids[output_index] = crypto::point_t(d.asset_id);
      tx_gen_context.asset_id_blinding_mask_x_amount_sum += tx_gen_context.asset_id_blinding_masks[output_index] * d.amount;
      tx_gen_context.amount_blinding_masks_sum += tx_gen_context.amount_blinding_masks[output_index];
      tx_gen_context.amount_commitments_sum += tx_gen_context.amount_commitments[output_index];
      ++output_index;
    }
    CHECK_AND_ASSERT_MES(add_random_derivation_hints_and_put_them_to_tx(0, std::set<uint16_t>(), derivation_hints, tx), false, "add_random_derivation_hints_and_put_them_to_tx failed");
    
    if (tx.attachment.size())
      add_attachments_info_to_extra(tx.extra, tx.attachment);

    if (!have_type_in_variant_container<etc_tx_details_unlock_time2>(tx.extra))
    {
      //if stake unlock time was not set, then we can use simple "whole transaction" lock scheme 
      set_tx_unlock_time(tx, height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
    }

    if (ogc_ptr)
      *ogc_ptr = tx_gen_context; // TODO @#@# consider refactoring (a lot of copying) -- sowle


    if (tx.version > TRANSACTION_VERSION_PRE_HF4 && !pos)
    {
      // This is for PoW blocks only, because PoS blocks proofs are handled in wallet2::prepare_and_sign_pos_block() due to the necessity of making Zarcanum proofs first
      // 
      // tx hash should be sealed by now
      crypto::hash tx_id = get_transaction_hash(tx);

      // asset surjection proof
      currency::zc_asset_surjection_proof asp{};
      bool r = generate_asset_surjection_proof(tx_id, true, tx_gen_context, asp);
      CHECK_AND_ASSERT_MES(r, false, "generete_asset_surjection_proof failed");
      tx.proofs.emplace_back(std::move(asp));

      // range proofs
      currency::zc_outs_range_proof range_proofs{};
      r = generate_zc_outs_range_proof(tx_id, 0, tx_gen_context, tx.vout, range_proofs);
      CHECK_AND_ASSERT_MES(r, false, "Failed to generate zc_outs_range_proof()");
      tx.proofs.emplace_back(std::move(range_proofs));

      // balance proof
      currency::zc_balance_proof balance_proof{};
      r = generate_tx_balance_proof(tx, tx_id, tx_gen_context, block_reward, balance_proof);
      CHECK_AND_ASSERT_MES(r, false, "generate_tx_balance_proof failed");
      tx.proofs.emplace_back(std::move(balance_proof));
    }



    return true;
  }
  //------------------------------------------------------------------
  bool check_tx_bare_balance(const transaction& tx, uint64_t additional_inputs_amount_and_fees_for_mining_tx /* = 0 */)
  {
    // legacy checks for old fashioned tx with non-hidden amounts
    CHECK_AND_ASSERT_MES(tx.version <= TRANSACTION_VERSION_PRE_HF4, false, "check_tx_bare_balance can't check post-HF4 txs");
    uint64_t bare_outputs_sum = get_outs_money_amount(tx);
    uint64_t bare_inputs_sum = get_inputs_money_amount(tx);

    if (additional_inputs_amount_and_fees_for_mining_tx == 0)
    {
      // normal tx
      CHECK_AND_ASSERT_MES(bare_inputs_sum >= bare_outputs_sum, false, "tx balance error: the sum of inputs (" << print_money_brief(bare_inputs_sum)
        << ") is less than or equal to the sum of outputs (" << print_money_brief(bare_outputs_sum) << ")");
    }
    else
    {
      // miner tx
      CHECK_AND_ASSERT_MES(bare_inputs_sum + additional_inputs_amount_and_fees_for_mining_tx == bare_outputs_sum, false,
        "tx balance error: the sum of inputs (" << print_money_brief(bare_inputs_sum) <<
        ") + additional inputs and fees (" << print_money_brief(additional_inputs_amount_and_fees_for_mining_tx) <<
        ") is less than or equal to the sum of outputs (" << print_money_brief(bare_outputs_sum) << ")");
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool validate_asset_operation_amount_commitment(asset_op_verification_context& context)
  {
    CHECK_AND_ASSERT_MES(count_type_in_variant_container<asset_operation_proof>(context.tx.proofs) == 1, false, "asset_operation_proof not present or present more than once");
    const asset_operation_proof& aop = get_type_in_variant_container_by_ref<const asset_operation_proof>(context.tx.proofs);

    if (context.ado.descriptor.hidden_supply)
    {
      CHECK_AND_ASSERT_MES(aop.opt_amount_commitment_composition_proof.has_value(), false, "opt_amount_commitment_composition_proof is absent");
      // TODO @#@# if asset is hidden -- check composition proof
      return false;
    }
    else
    {
      // make sure that amount commitment corresponds to opt_amount_commitment_g_proof
      CHECK_AND_ASSERT_MES(aop.opt_amount_commitment_g_proof.has_value(), false, "opt_amount_commitment_g_proof is absent");
      crypto::point_t A = crypto::point_t(context.ado.amount_commitment).modify_mul8() - context.amount_to_validate * context.asset_id_pt;

      bool r = crypto::check_signature(context.tx_id, A.to_public_key(), aop.opt_amount_commitment_g_proof.get());
      CHECK_AND_ASSERT_MES(r, false, "opt_amount_commitment_g_proof check failed");
    }

    return true;
  }
  //------------------------------------------------------------------
  bool check_tx_balance(const transaction& tx, const crypto::hash& tx_id, uint64_t additional_inputs_amount_and_fees_for_mining_tx /* = 0 */)
  {
    if (tx.version > TRANSACTION_VERSION_PRE_HF4)
    {
      zc_balance_proof balance_proof = AUTO_VAL_INIT(balance_proof);
      bool r = get_type_in_variant_container<zc_balance_proof>(tx.proofs, balance_proof);
      CHECK_AND_ASSERT_MES(r, false, "zc_balance_proof is missing in tx proofs");

      crypto::public_key tx_pub_key = get_tx_pub_key_from_extra(tx);

      size_t zc_inputs_count = 0;
      uint64_t bare_inputs_sum = additional_inputs_amount_and_fees_for_mining_tx;
      for(auto& vin : tx.vin)
      {
        VARIANT_SWITCH_BEGIN(vin);
        VARIANT_CASE_CONST(txin_to_key, tk)
          bare_inputs_sum += tk.amount;
        VARIANT_CASE_CONST(txin_htlc, htlc);
          bare_inputs_sum += htlc.amount;
        VARIANT_CASE_CONST(txin_multisig, ms);
          bare_inputs_sum += ms.amount;
        VARIANT_CASE_CONST(txin_zc_input, foo);
          ++zc_inputs_count;
        VARIANT_SWITCH_END();
      }

      crypto::point_t outs_commitments_sum = crypto::c_point_0; // TODO: consider adding additional commitments / spends / burns here
      for(auto& vout : tx.vout)
      {
        CHECK_AND_ASSERT_MES(vout.type() == typeid(tx_out_zarcanum), false, "unexpected type in outs: " << vout.type().name());
        const tx_out_zarcanum& ozc = boost::get<tx_out_zarcanum>(vout);
        outs_commitments_sum += crypto::point_t(ozc.amount_commitment); // amount_commitment premultiplied by 1/8
      }

      uint64_t fee = 0;
      CHECK_AND_ASSERT_MES(get_tx_fee(tx, fee) || additional_inputs_amount_and_fees_for_mining_tx > 0, false, "unable to get fee for a non-mining tx");

      CHECK_AND_ASSERT_MES(additional_inputs_amount_and_fees_for_mining_tx == 0 || fee == 0, false, "invalid tx: fee = " << print_money_brief(fee) <<
        ", additional inputs + fees = " << print_money_brief(additional_inputs_amount_and_fees_for_mining_tx));

      crypto::point_t sum_of_pseudo_out_amount_commitments = crypto::c_point_0;
      // take into account generated/burnt assets
      asset_descriptor_operation ado = AUTO_VAL_INIT(ado);
      if (get_type_in_variant_container(tx.extra, ado))
      {
        if (ado.operation_type == ASSET_DESCRIPTOR_OPERATION_REGISTER || ado.operation_type == ASSET_DESCRIPTOR_OPERATION_EMIT)
        {
          // amount_commitment supposed to be validated earlier in validate_asset_operation_amount_commitment()
          sum_of_pseudo_out_amount_commitments += crypto::point_t(ado.amount_commitment); // *1/8
        }
        else if (ado.operation_type == ASSET_DESCRIPTOR_OPERATION_PUBLIC_BURN)
        {
          outs_commitments_sum += crypto::point_t(ado.amount_commitment); // *1/8
        }
      }
      size_t zc_sigs_count = 0;
      for(auto& sig_v : tx.signatures)
      {
        VARIANT_SWITCH_BEGIN(sig_v);
        VARIANT_CASE_CONST(ZC_sig, zc_sig);
          sum_of_pseudo_out_amount_commitments += crypto::point_t(zc_sig.pseudo_out_amount_commitment); // *1/8
          ++zc_sigs_count;
        VARIANT_CASE_CONST(zarcanum_sig, sig);
          sum_of_pseudo_out_amount_commitments += crypto::point_t(sig.pseudo_out_amount_commitment); // *1/8
          ++zc_sigs_count;
        VARIANT_SWITCH_END();
      }

      outs_commitments_sum.modify_mul8();
      sum_of_pseudo_out_amount_commitments.modify_mul8();

      // (sum(bare inputs' amounts) - fee) * H + sum(pseudo outs commitments for ZC inputs) - sum(outputs' commitments) = lin(X)  OR  = lin(G)
      crypto::point_t commitment_to_zero = (crypto::scalar_t(bare_inputs_sum) - crypto::scalar_t(fee)) * currency::native_coin_asset_id_pt + sum_of_pseudo_out_amount_commitments - outs_commitments_sum;

      CHECK_AND_ASSERT_MES(zc_inputs_count == zc_sigs_count, false, "zc inputs count (" << zc_inputs_count << ") and zc sigs count (" << zc_sigs_count << ") missmatch");
      if (zc_inputs_count > 0)
      {
        r = crypto::verify_double_schnorr_sig<crypto::gt_X, crypto::gt_G>(tx_id, commitment_to_zero, tx_pub_key, balance_proof.dss);
        CHECK_AND_ASSERT_MES(r, false, "verify_double_schnorr_sig (X, G) is invalid");
      }
      else
      {
        r = crypto::verify_double_schnorr_sig<crypto::gt_G, crypto::gt_G>(tx_id, commitment_to_zero, tx_pub_key, balance_proof.dss);
        CHECK_AND_ASSERT_MES(r, false, "verify_double_schnorr_sig (G, G) is invalid");
      }
      return true;
    }
    
    // pre-HF4 txs
    return check_tx_bare_balance(tx, additional_inputs_amount_and_fees_for_mining_tx);
  }
  //------------------------------------------------------------------
  bool derive_ephemeral_key_helper(const account_keys& ack, const crypto::public_key& tx_public_key, size_t real_output_index, keypair& in_ephemeral)
  {
    crypto::key_derivation recv_derivation = AUTO_VAL_INIT(recv_derivation);
    bool r = crypto::generate_key_derivation(tx_public_key, ack.view_secret_key, recv_derivation);
    CHECK_AND_ASSERT_MES(r, false, "key image helper: failed to generate_key_derivation(" << tx_public_key << ", " << ack.view_secret_key << ")");

    r = crypto::derive_public_key(recv_derivation, real_output_index, ack.account_address.spend_public_key, in_ephemeral.pub);
    CHECK_AND_ASSERT_MES(r, false, "key image helper: failed to derive_public_key(" << recv_derivation << ", " << real_output_index << ", " << ack.account_address.spend_public_key << ")");

    crypto::derive_secret_key(recv_derivation, real_output_index, ack.spend_secret_key, in_ephemeral.sec);
    return true;
  }
  //---------------------------------------------------------------
  alias_rpc_details alias_info_to_rpc_alias_info(const currency::extra_alias_entry& ai)
  {
    alias_rpc_details ari;
    alias_info_to_rpc_alias_info(ai, ari);
    return ari;
  }
  //---------------------------------------------------------------
  update_alias_rpc_details alias_info_to_rpc_update_alias_info(const currency::extra_alias_entry& ai, const std::string& old_address)
  {
    update_alias_rpc_details uard;
    alias_info_to_rpc_alias_info(ai, uard);
    uard.old_address = old_address;
    return uard;
  }
  //---------------------------------------------------------------
  bool generate_key_image_helper(const account_keys& ack, const crypto::public_key& tx_public_key, size_t real_output_index, keypair& in_ephemeral, crypto::key_image& ki)
  {
    bool r = derive_ephemeral_key_helper(ack, tx_public_key, real_output_index, in_ephemeral);
    CHECK_AND_ASSERT_MES(r, false, "Failed to call derive_ephemeral_key_helper(...)");

    crypto::generate_key_image(in_ephemeral.pub, in_ephemeral.sec, ki);
    return true;
  }
  //---------------------------------------------------------------
  uint64_t power_integral(uint64_t a, uint64_t b)
  {
    if (b == 0)
      return 1;
    uint64_t total = a;
    for (uint64_t i = 1; i != b; i++)
      total *= a;
    return total;
  }
  //---------------------------------------------------------------
  bool is_mixattr_applicable_for_fake_outs_counter(uint64_t out_tx_version, uint8_t mix_attr, uint64_t fake_outputs_count, const core_runtime_config& rtc)
  {
    if (out_tx_version >= TRANSACTION_VERSION_POST_HF4)
    {
      if (mix_attr != CURRENCY_TO_KEY_OUT_FORCED_NO_MIX)
        return fake_outputs_count >= rtc.hf4_minimum_mixins;
      else
        return fake_outputs_count == 0; // CURRENCY_TO_KEY_OUT_FORCED_NO_MIX
    }
    else
    {
      if (mix_attr >= CURRENCY_TO_KEY_OUT_FORCED_MIX_LOWER_BOUND)
        return fake_outputs_count + 1 >= mix_attr;
      else if (mix_attr == CURRENCY_TO_KEY_OUT_FORCED_NO_MIX)
        return fake_outputs_count == 0;
      else
        return true;
    }
  }
  //---------------------------------------------------------------
  // TODO: reverse order of arguments
  bool parse_amount(uint64_t& amount, const std::string& str_amount_)
  {
    std::string str_amount = str_amount_;
    boost::algorithm::trim(str_amount);

    size_t point_index = str_amount.find_first_of('.');
    size_t fraction_size;
    if (std::string::npos != point_index)
    {
      fraction_size = str_amount.size() - point_index - 1;
      while (CURRENCY_DISPLAY_DECIMAL_POINT < fraction_size && '0' == str_amount.back())
      {
        str_amount.erase(str_amount.size() - 1, 1);
        --fraction_size;
      }
      if (CURRENCY_DISPLAY_DECIMAL_POINT < fraction_size)
        return false;
      str_amount.erase(point_index, 1);
    }
    else
    {
      fraction_size = 0;
    }

    if (str_amount.empty())
      return false;

    if (fraction_size < CURRENCY_DISPLAY_DECIMAL_POINT)
    {
      str_amount.append(CURRENCY_DISPLAY_DECIMAL_POINT - fraction_size, '0');
    }

    return string_tools::get_xtype_from_string(amount, str_amount);
  }
  //--------------------------------------------------------------------------------
  bool parse_tracking_seed(const std::string& tracking_seed, account_public_address& address, crypto::secret_key& view_sec_key, uint64_t& creation_timestamp)
  {
    std::vector<std::string> parts;
    boost::split(parts, tracking_seed, [](char x){ return x == ':'; } );
    if (parts.size() != 2 && parts.size() != 3)
      return false;

    if (!get_account_address_from_str(address, parts[0]))
      return false;

    if (!address.is_auditable())
      return false;

    if (!epee::string_tools::parse_tpod_from_hex_string(parts[1], view_sec_key))
      return false;

    crypto::public_key view_pub_key = AUTO_VAL_INIT(view_pub_key);
    if (!crypto::secret_key_to_public_key(view_sec_key, view_pub_key))
      return false;

    if (view_pub_key != address.view_public_key)
      return false;

    creation_timestamp = 0;
    if (parts.size() == 3)
    {
      // parse timestamp
      int64_t ts = 0;
      if (!epee::string_tools::string_to_num_fast(parts[2], ts))
        return false;

      if (ts < WALLET_BRAIN_DATE_OFFSET)
        return false;
      
      creation_timestamp = ts;
    }

    return true;
  }
  //--------------------------------------------------------------------------------
  std::string print_stake_kernel_info(const stake_kernel& sk)
  {
    std::stringstream ss;
    ss << "block timestamp:       " << sk.block_timestamp << ENDL
       << "key image:             " << sk.kimage << ENDL
       << "sm.last_pos_kernel_id: " << sk.stake_modifier.last_pos_kernel_id << ENDL
       << "sm.last_pow_id:        " << sk.stake_modifier.last_pow_id << ENDL;
    return ss.str();
  }

  //---------------------------------------------------------------
  bool get_tx_fee(const transaction& tx, uint64_t& fee)
  {
    fee = 0;
    if (is_coinbase(tx))
      return true;

    if (tx.version <= TRANSACTION_VERSION_PRE_HF4)
    {
      // all amounts are open:  fee = sum(outputs) - sum(inputs)

      uint64_t amount_in = 0;
      uint64_t amount_out = get_outs_money_amount(tx);
   
      for(auto& in : tx.vin)
        amount_in += get_amount_from_variant(in);

      CHECK_AND_ASSERT_MES(amount_in >= amount_out, false, "transaction spends (" << print_money_brief(amount_in) << ") more than it has (" << print_money_brief(amount_out) << ")");
      fee = amount_in - amount_out;
      return true;
    }

    // tx.version > TRANSACTION_VERSION_PRE_HF4
    // all amounts are hidden with Pedersen commitments
    // therefore fee should be explicitly stated in the extra
    auto cb = [&](const zarcanum_tx_data_v1& ztd) -> bool {
      fee += ztd.fee;
      return true; // continue
      };
      
    bool r = process_type_in_variant_container<zarcanum_tx_data_v1>(tx.extra, cb, false);
      
    if (!r)
    {
      fee = 0;
      return false;
    }

    return true;
  }
  //---------------------------------------------------------------
  uint64_t get_tx_fee(const transaction& tx)
  {
    uint64_t r = 0;
    if (!get_tx_fee(tx, r))
      return 0;
    return r;
  }
  //---------------------------------------------------------------
  crypto::public_key get_tx_pub_key_from_extra(const transaction& tx)
  {
    crypto::public_key pk = null_pkey;
    get_type_in_variant_container(tx.extra, pk);
    return pk;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_tx_extra(const transaction& tx, crypto::public_key& tx_pub_key)
  {
    tx_extra_info e = AUTO_VAL_INIT(e);
    bool r = parse_and_validate_tx_extra(tx, e);
    tx_pub_key = e.m_tx_pub_key;
    return r;
  }
  //---------------------------------------------------------------
  bool sign_extra_alias_entry(extra_alias_entry& ai, const crypto::public_key& pkey, const crypto::secret_key& skey)
  {
    ai.m_sign.clear();
    get_sign_buff_hash_for_alias_update(ai);
    crypto::signature sig = AUTO_VAL_INIT(sig);
    crypto::generate_signature(get_sign_buff_hash_for_alias_update(ai), pkey, skey, sig);
    ai.m_sign.push_back(sig);
    return true;
  }
  //---------------------------------------------------------------
  crypto::hash get_sign_buff_hash_for_alias_update(const extra_alias_entry& ai)
  {
    extra_alias_entry_old ai_old = ai.to_old();
    if (ai_old.m_sign.size())
      ai_old.m_sign.clear();
    return get_object_hash(ai_old);
  }
  //---------------------------------------------------------------
  struct tx_extra_handler : public boost::static_visitor<bool>
  {
    mutable bool was_padding = false; //let the padding goes only at the end
    mutable bool was_pubkey = false;
    mutable bool was_attachment = false;
    mutable bool was_userdata = false;
    mutable bool was_alias = false;
    mutable bool was_asset = false;

    tx_extra_info& rei;
    const transaction& rtx;

    tx_extra_handler(tx_extra_info& ei, const transaction& tx) :rei(ei), rtx(tx)
    {}

#define ENSURE_ONETIME(varname, entry_name) CHECK_AND_ASSERT_MES(varname == false, false, "double entry in tx_extra: " entry_name); varname = true;


    bool operator()(const crypto::public_key& k) const
    {
      ENSURE_ONETIME(was_pubkey, "public_key");
      rei.m_tx_pub_key = k;
      return true;
    }
    bool operator()(const extra_attachment_info& ai) const
    {
      ENSURE_ONETIME(was_attachment, "attachment");
      rei.m_attachment_info = ai;
      return true;
    }
    bool operator()(const extra_alias_entry& ae) const
    {
      ENSURE_ONETIME(was_alias, "alias");
      rei.m_alias = ae;
      return true;
    }
    bool operator()(const asset_descriptor_operation & ado) const
    {
      ENSURE_ONETIME(was_asset, "asset");
      rei.m_asset_operation = ado;
      return true;
    }
    bool operator()(const extra_alias_entry_old& ae) const
    {
      return operator()(static_cast<const extra_alias_entry&>(ae));
    }
    bool operator()(const extra_user_data& ud) const
    {
      ENSURE_ONETIME(was_userdata, "userdata");
      if (!ud.buff.empty())
        rei.m_user_data_blob.assign((const char*)&ud.buff[0], ud.buff.size());
      return true;
    }
    bool operator()(const extra_padding& k) const
    {
      ENSURE_ONETIME(was_padding, "padding");
      return true;
    }
    template<class t_extra_typename>
    bool operator()(const t_extra_typename& k) const
    {
      //do nothing for rest
      return true;
    }
  };
  //------------------------------------------------------------------------------------
  bool parse_and_validate_tx_extra(const transaction& tx, tx_extra_info& extra)
  {
    extra.m_tx_pub_key = null_pkey;

    tx_extra_handler vtr(extra, tx);

    for (const auto& ex_v : tx.extra)
    {
      if (!boost::apply_visitor(vtr, ex_v))
        return false;
    }
    return true;
  }
  //---------------------------------------------------------------
  bool add_tx_pub_key_to_extra(transaction& tx, const crypto::public_key& tx_pub_key)
  {
    tx.extra.push_back(tx_pub_key);
    return true;
  }
  //---------------------------------------------------------------
  // puts explicit fee amount to tx extra, should be used only for tx.verson > TRANSACTION_VERSION_PRE_HF4
  bool add_tx_fee_amount_to_extra(transaction& tx, uint64_t fee, bool make_sure_its_unique /* = true */)
  {
    if (make_sure_its_unique)
    {
      if (count_type_in_variant_container<zarcanum_tx_data_v1>(tx.extra) != 0)
        return false;
    }

    zarcanum_tx_data_v1 ztd = AUTO_VAL_INIT(ztd);
    ztd.fee = fee;

    tx.extra.push_back(ztd);
    return true;
  }
  //---------------------------------------------------------------
  //---------------------------------------------------------------

  struct multisig_id_generator
  {
    //std::vector<txin_v> vin;
    crypto::public_key onetime_key;
    std::vector<tx_out_bare> vout;

    BEGIN_SERIALIZE()
      //FIELD(vin)
      FIELD(onetime_key)
      FIELD(vout)
    END_SERIALIZE()
  };
  crypto::hash get_multisig_out_id(const transaction& tx, size_t n)
  {
    multisig_id_generator msg = AUTO_VAL_INIT(msg);
    //msg.vin = tx.vin;
    msg.onetime_key = get_tx_pub_key_from_extra(tx);
    CHECK_AND_ASSERT_MES(tx.vout.size() > n, null_hash, "tx.vout.size() > n condition failed ");
    CHECK_AND_ASSERT_MES(tx.vout[n].type() == typeid(tx_out_bare), null_hash, "Unexpected type of out:" << tx.vout[n].type().name());
    CHECK_AND_ASSERT_MES(boost::get<tx_out_bare>(tx.vout[n]).target.type() == typeid(txout_multisig), null_hash, "tx.vout[n].target.type() == typeid(txout_multisig) condition failed");
    msg.vout.push_back(boost::get<tx_out_bare>(tx.vout[n]));
    return get_object_hash(msg);
  }
  //---------------------------------------------------------------
  bool add_tx_extra_userdata(transaction& tx, const blobdata& extra_nonce)
  {
    CHECK_AND_ASSERT_MES(extra_nonce.size() <= 255, false, "extra nonce size exceeded (255 bytes max)");
    extra_user_data eud = AUTO_VAL_INIT(eud);
    eud.buff = extra_nonce;
    tx.extra.push_back(eud);
    return true;
  }
  //---------------------------------------------------------------
  bool derive_public_key_from_target_address(const account_public_address& destination_addr, const crypto::secret_key& tx_sec_key, size_t index, crypto::public_key& out_eph_public_key)
  {
    crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
    return derive_public_key_from_target_address(destination_addr, tx_sec_key, index, out_eph_public_key, derivation);
  }
  //---------------------------------------------------------------
  // derived_sec_key = Hs(domain, 8 * src_sec_key * src_pub_key, index)
  // derived_pub_key = derived_sec_key * G
  bool derive_key_pair_from_key_pair(const crypto::public_key& src_pub_key, const crypto::secret_key& src_sec_key, crypto::secret_key& derived_sec_key, crypto::public_key& derived_pub_key, const char(&hs_domain)[32], uint64_t index)
  {
    crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
    bool r = crypto::generate_key_derivation(src_pub_key, src_sec_key, derivation);
    CHECK_AND_ASSERT_MES(r, false, "at creation outs: failed to generate_key_derivation(" << src_pub_key << ", " << src_sec_key << ")");
    crypto::scalar_t sec_key = crypto::hash_helper_t::hs(hs_domain, derivation, index);
    derived_sec_key = sec_key.as_secret_key();
    derived_pub_key = (sec_key * crypto::c_point_G).to_public_key();
    return true;
  }
  //---------------------------------------------------------------
  // derivation = 8 * tx_sec_key * destination_addr.view_public_key
  // out_eph_public_key = destination_addr.spend_public_key + Hs(derivation, index) * G
  bool derive_public_key_from_target_address(const account_public_address& destination_addr, const crypto::secret_key& tx_sec_key, size_t index, crypto::public_key& out_eph_public_key, crypto::key_derivation& derivation)
  {    
    bool r = crypto::generate_key_derivation(destination_addr.view_public_key, tx_sec_key, derivation);
    CHECK_AND_ASSERT_MES(r, false, "at creation outs: failed to generate_key_derivation(" << destination_addr.view_public_key << ", " << tx_sec_key << ")");

    r = crypto::derive_public_key(derivation, index, destination_addr.spend_public_key, out_eph_public_key);
    CHECK_AND_ASSERT_MES(r, false, "at creation outs: failed to derive_public_key(" << derivation << ", " << index << ", " << destination_addr.view_public_key << ")");
    return r;
  }
  //---------------------------------------------------------------
  uint16_t get_derivation_hint(const crypto::key_derivation& derivation)
  {
    crypto::hash h = blake2_hash(&derivation, sizeof(derivation));

    uint16_t* pderiv_hash_as_array = (uint16_t*)&h;
    uint16_t res = pderiv_hash_as_array[0];
    for (size_t i = 1; i != sizeof(h) / sizeof(uint16_t); i++)
      res ^= pderiv_hash_as_array[i];
    return res;
  }
  //---------------------------------------------------------------
  uint64_t get_string_uint64_hash(const std::string& str)
  {
    crypto::hash h = crypto::cn_fast_hash(str.data(), str.size());
    uint64_t* phash_as_array = (uint64_t*)&h;
    return phash_as_array[0] ^ phash_as_array[1] ^ phash_as_array[2] ^ phash_as_array[3];
  }
  //---------------------------------------------------------------
  tx_derivation_hint make_tx_derivation_hint_from_uint16(uint16_t hint)
  {
    tx_derivation_hint dh = AUTO_VAL_INIT(dh);
    dh.msg.assign((const char*)&hint, sizeof(hint));
    return dh;
  }
  //---------------------------------------------------------------
  bool get_uint16_from_tx_derivation_hint(const tx_derivation_hint& dh, uint16_t& hint)
  {
    if (dh.msg.size() != sizeof(hint))
      return false;
    hint = *((uint16_t*)dh.msg.data());
    return true;
  }  
  //---------------------------------------------------------------
  std::string generate_origin_for_htlc(const txout_htlc& htlc, const account_keys& acc_keys)
  {
    std::string blob;
    string_tools::append_pod_to_strbuff(blob, htlc.pkey_redeem);
    string_tools::append_pod_to_strbuff(blob, htlc.pkey_refund);
    string_tools::append_pod_to_strbuff(blob, acc_keys.spend_secret_key);
    crypto::hash origin_hs = crypto::cn_fast_hash(blob.data(), blob.size());
    std::string origin_blob;
    string_tools::append_pod_to_strbuff(origin_blob, origin_hs);
    return origin_blob;
  }
  //---------------------------------------------------------------
  bool validate_ado_update_allowed(const asset_descriptor_base& new_ado, const asset_descriptor_base& prev_ado)
  {
    if (new_ado.total_max_supply != prev_ado.total_max_supply) return false;
    if (new_ado.current_supply > prev_ado.total_max_supply) return false;
    if (new_ado.decimal_point != prev_ado.decimal_point) return false;
    if (new_ado.ticker != prev_ado.ticker) return false;
    if (new_ado.full_name != prev_ado.full_name) return false;
    //a.meta_info;
    //if (a.owner != b.owner) return false;
    if (new_ado.hidden_supply != prev_ado.hidden_supply) return false;
    
    return true;
  }
  //---------------------------------------------------------------
  bool validate_ado_initial(const asset_descriptor_base& new_ado)
  {
    if (new_ado.current_supply > new_ado.total_max_supply) return false;
    return true;
  }
  //---------------------------------------------------------------
  /*
  crypto::hash get_signature_hash_for_asset_operation(const asset_descriptor_operation& ado)
  {
    asset_descriptor_operation ado_local = ado;
    normalize_asset_operation_for_hashing(ado_local);
    std::string buff = t_serializable_object_to_blob(ado_local);
    return crypto::cn_fast_hash(buff.data(), buff.size());
  }

  //---------------------------------------------------------------
  void normalize_asset_operation_for_hashing(asset_descriptor_operation& op)
  {
    op.opt_proof = boost::none;
  }
  */

  //---------------------------------------------------------------
  bool construct_tx_out(const tx_destination_entry& de, const crypto::secret_key& tx_sec_key, size_t output_index, transaction& tx, std::set<uint16_t>& deriv_cache, const account_keys& self, uint8_t tx_outs_attr /*  = CURRENCY_TO_KEY_OUT_RELAXED */)
  {
    finalized_tx result = AUTO_VAL_INIT(result);
    crypto::scalar_t asset_blinding_mask{}, amount_blinding_mask{};
    crypto::point_t blinded_asset_id{}, amount_commitment{};
    return construct_tx_out(de, tx_sec_key, output_index, tx, deriv_cache, self, asset_blinding_mask, amount_blinding_mask, blinded_asset_id, amount_commitment, result, tx_outs_attr);
  }
  //---------------------------------------------------------------
  bool construct_tx_out(const tx_destination_entry& de, const crypto::secret_key& tx_sec_key, size_t output_index, transaction& tx, std::set<uint16_t>& deriv_cache,
    const account_keys& self, crypto::scalar_t& asset_blinding_mask, crypto::scalar_t& amount_blinding_mask, crypto::point_t& blinded_asset_id, crypto::point_t& amount_commitment,
    finalized_tx& result, uint8_t tx_outs_attr)
  {
    if (tx.version > TRANSACTION_VERSION_PRE_HF4)
    {
      // create tx_out_zarcanum
      CHECK_AND_ASSERT_MES(de.addr.size() == 1, false, "zarcanum multisig not implemented for tx_out_zarcanum yet");
      // TODO @#@# implement multisig support

      tx_out_zarcanum out = AUTO_VAL_INIT(out);

      const account_public_address& apa = de.addr.front();
      if (apa.spend_public_key == null_pkey && apa.view_public_key == null_pkey)
      {
        // burn money
        // calculate encrypted_amount and amount_commitment anyway, but using modified derivation
        crypto::scalar_t h = crypto::hash_helper_t::hs(crypto::scalar_t(tx_sec_key), output_index); // h = Hs(r, i)

        out.stealth_address = null_pkey;
        out.concealing_point = null_pkey;

        crypto::scalar_t amount_mask   = crypto::hash_helper_t::hs(CRYPTO_HDS_OUT_AMOUNT_MASK, h);
        out.encrypted_amount = de.amount ^ amount_mask.m_u64[0];
      
        CHECK_AND_ASSERT_MES(~de.flags & tx_destination_entry_flags::tdef_explicit_native_asset_id || de.asset_id == currency::native_coin_asset_id, false, "explicit_native_asset_id may be used only with native asset id");
        asset_blinding_mask = de.flags & tx_destination_entry_flags::tdef_explicit_native_asset_id ? 0 : crypto::hash_helper_t::hs(CRYPTO_HDS_OUT_ASSET_BLINDING_MASK, h); // s = Hs(domain_sep, Hs(8 * r * V, i))
        blinded_asset_id = crypto::point_t(de.asset_id) + asset_blinding_mask * crypto::c_point_X;
        out.blinded_asset_id = (crypto::c_scalar_1div8 * blinded_asset_id).to_public_key(); // T = 1/8 * (H_asset + s * X)
        
        amount_blinding_mask = de.flags & tx_destination_entry_flags::tdef_zero_amount_blinding_mask ? 0 : crypto::hash_helper_t::hs(CRYPTO_HDS_OUT_AMOUNT_BLINDING_MASK, h); // y = Hs(domain_sep, Hs(8 * r * V, i))
        amount_commitment = de.amount * blinded_asset_id + amount_blinding_mask * crypto::c_point_G;
        out.amount_commitment = (crypto::c_scalar_1div8 * amount_commitment).to_public_key(); // E = 1/8 * e * T + 1/8 * y * G

        out.mix_attr = tx_outs_attr; // TODO @#@# @CZ check this
      }
      else
      {
        // normal output
        crypto::public_key derivation = (crypto::scalar_t(tx_sec_key) * crypto::point_t(apa.view_public_key)).modify_mul8().to_public_key(); // d = 8 * r * V
        crypto::scalar_t h = 0;
        crypto::derivation_to_scalar((const crypto::key_derivation&)derivation, output_index, h.as_secret_key()); // h = Hs(8 * r * V, i)

        out.stealth_address = (h * crypto::c_point_G + crypto::point_t(apa.spend_public_key)).to_public_key();
        out.concealing_point = (crypto::hash_helper_t::hs(CRYPTO_HDS_OUT_CONCEALING_POINT, h) * crypto::point_t(apa.view_public_key)).to_public_key(); // Q = 1/8 * Hs(domain_sep, Hs(8 * r * V, i) ) * 8 * V
      
        crypto::scalar_t amount_mask = crypto::hash_helper_t::hs(CRYPTO_HDS_OUT_AMOUNT_MASK, h);
        out.encrypted_amount = de.amount ^ amount_mask.m_u64[0];
      
        CHECK_AND_ASSERT_MES(~de.flags & tx_destination_entry_flags::tdef_explicit_native_asset_id || de.asset_id == currency::native_coin_asset_id, false, "explicit_native_asset_id may be used only with native asset id");
        asset_blinding_mask = de.flags & tx_destination_entry_flags::tdef_explicit_native_asset_id ? 0 : crypto::hash_helper_t::hs(CRYPTO_HDS_OUT_ASSET_BLINDING_MASK, h); // s = Hs(domain_sep, Hs(8 * r * V, i))
        blinded_asset_id = crypto::point_t(de.asset_id) + asset_blinding_mask * crypto::c_point_X;
        out.blinded_asset_id = (crypto::c_scalar_1div8 * blinded_asset_id).to_public_key(); // T = 1/8 * (H_asset + s * X)

        amount_blinding_mask = crypto::hash_helper_t::hs(CRYPTO_HDS_OUT_AMOUNT_BLINDING_MASK, h); // y = Hs(domain_sep, Hs(8 * r * V, i))
        amount_commitment = de.amount * blinded_asset_id + amount_blinding_mask * crypto::c_point_G;
        out.amount_commitment = (crypto::c_scalar_1div8 * amount_commitment).to_public_key(); // E = 1/8 * e * T + 1/8 * y * G

        DBG_VAL_PRINT(output_index);
        DBG_VAL_PRINT(de.amount);
        DBG_VAL_PRINT(de.asset_id);
        DBG_VAL_PRINT(amount_mask);
        DBG_VAL_PRINT(asset_blinding_mask);
        DBG_VAL_PRINT(blinded_asset_id);
        DBG_VAL_PRINT(amount_blinding_mask);
        DBG_VAL_PRINT(amount_mask);
        DBG_VAL_PRINT(amount_commitment);
        
        if (de.addr.front().is_auditable())
          out.mix_attr = CURRENCY_TO_KEY_OUT_FORCED_NO_MIX; // override mix_attr to 1 for auditable target addresses
        else
          out.mix_attr = tx_outs_attr;

        uint16_t hint = get_derivation_hint(reinterpret_cast<crypto::key_derivation&>(derivation));
        deriv_cache.insert(hint); // won't be inserted if such hint already exists
      }

      tx.vout.push_back(out);
    }
    else
    {
      // create tx_out_bare, this section can be removed after HF4
      CHECK_AND_ASSERT_MES(de.addr.size() == 1 || (de.addr.size() > 1 && de.minimum_sigs <= de.addr.size()), false, "Invalid destination entry: amount: " << de.amount << " minimum_sigs: " << de.minimum_sigs << " addr.size(): " << de.addr.size());
      CHECK_AND_ASSERT_MES(de.asset_id == currency::native_coin_asset_id, false, "assets are not allowed prior to HF4");

      std::vector<crypto::public_key> target_keys;
      target_keys.reserve(de.addr.size());
      for (auto& apa : de.addr)
      {
        crypto::public_key out_eph_public_key = AUTO_VAL_INIT(out_eph_public_key);
        if (apa.spend_public_key == null_pkey && apa.view_public_key == null_pkey)
        {
          //burning money(for example alias reward)
          out_eph_public_key = null_pkey;
        }
        else
        {
          crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
          bool r = derive_public_key_from_target_address(apa, tx_sec_key, output_index, out_eph_public_key, derivation);
          CHECK_AND_ASSERT_MES(r, false, "failed to derive_public_key_from_target_address");

          uint16_t hint = get_derivation_hint(derivation);
          deriv_cache.insert(hint); // won't be inserted if such hint already exists
        }
        target_keys.push_back(out_eph_public_key);
      }

      tx_out_bare out;
      out.amount = de.amount;
      if (de.htlc_options.expiration != 0)
      {
        const destination_option_htlc_out& htlc_dest = de.htlc_options;
        //out htlc
        CHECK_AND_ASSERT_MES(target_keys.size() == 1, false, "Unexpected htl keys count = " << target_keys.size() << ", expected ==1");
        txout_htlc htlc = AUTO_VAL_INIT(htlc);
        htlc.expiration = htlc_dest.expiration;
        htlc.flags = 0; //0 - SHA256, 1 - RIPEMD160, by default leave SHA256
        //receiver key
        htlc.pkey_redeem = *target_keys.begin();
        //generate refund key
        crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
        crypto::public_key out_eph_public_key = AUTO_VAL_INIT(out_eph_public_key);
        bool r = derive_public_key_from_target_address(self.account_address, tx_sec_key, output_index, out_eph_public_key, derivation);
        CHECK_AND_ASSERT_MES(r, false, "failed to derive_public_key_from_target_address");
        htlc.pkey_refund = out_eph_public_key;
        //add derivation hint for refund address
        uint16_t hint = get_derivation_hint(derivation);
        deriv_cache.insert(hint); // won't be inserted if such hint already exists


        if (htlc_dest.htlc_hash == null_hash)
        {
          //we use deterministic origin, to make possible access origin on different wallets copies
        
          result.htlc_origin = generate_origin_for_htlc(htlc, self);

          //calculate hash
          if (!htlc.flags&CURRENCY_TXOUT_HTLC_FLAGS_HASH_TYPE_MASK)
          {
            htlc.htlc_hash = crypto::sha256_hash(result.htlc_origin.data(), result.htlc_origin.size());
          }
          else
          {
            crypto::hash160 h160 = crypto::RIPEMD160_hash(result.htlc_origin.data(), result.htlc_origin.size());
            std::memcpy(&htlc.htlc_hash, &h160, sizeof(h160));
          }
        }
        else
        {
          htlc.htlc_hash = htlc_dest.htlc_hash;
        }
        out.target = htlc;
      }
      else if (target_keys.size() == 1)
      {
        //out to key
        txout_to_key tk = AUTO_VAL_INIT(tk);
        tk.key = target_keys.back();

        if (de.addr.front().is_auditable()) // check only the first address because there's only one in this branch
          tk.mix_attr = CURRENCY_TO_KEY_OUT_FORCED_NO_MIX; // override mix_attr to 1 for auditable target addresses
        else
          tk.mix_attr = tx_outs_attr;
      
        out.target = tk;
      }
      else
      {
        //multisig out
        txout_multisig ms = AUTO_VAL_INIT(ms);
        ms.keys = std::move(target_keys);
        ms.minimum_sigs = de.minimum_sigs;
        out.target = ms;
      }
      tx.vout.push_back(out);
    }
    return true;
  }
  //---------------------------------------------------------------
  bool have_attachment_service_in_container(const std::vector<attachment_v>& av, const std::string& service_id, const std::string& instruction)
  {
    for (auto& ai : av)
    {
      if (ai.type() == typeid(tx_service_attachment))
      {
        const tx_service_attachment& tsa = boost::get<tx_service_attachment>(ai);
        if (tsa.service_id == service_id && tsa.instruction == instruction)
          return true;
      }
    }
    return false;
  }
  //---------------------------------------------------------------
  void get_attachment_extra_info_details(const std::vector<attachment_v>& attachment, extra_attachment_info& eai)
  {
    eai = extra_attachment_info();
    if (!attachment.size())
      return;

    //put hash into extra
    std::stringstream ss;
    binary_archive<true> oar(ss);
    if (!::do_serialize(oar, const_cast<std::vector<attachment_v>&>(attachment)))
      return;
    std::string buff = ss.str();
    eai.sz = buff.size();
    eai.hsh = get_blob_hash(buff);
    eai.cnt = attachment.size();
  }
  //---------------------------------------------------------------
  bool construct_tx(const account_keys& sender_account_keys, 
    const std::vector<tx_source_entry>& sources,
    const std::vector<tx_destination_entry>& destinations,
    const std::vector<attachment_v>& attachments,    
    transaction& tx,
    uint64_t tx_version,
    uint64_t unlock_time,
    uint8_t tx_outs_attr, 
    bool shuffle)
  {
    crypto::secret_key one_time_secret_key = AUTO_VAL_INIT(one_time_secret_key);
    return construct_tx(sender_account_keys, sources, destinations, std::vector<extra_v>(), attachments, tx, tx_version, one_time_secret_key, unlock_time, tx_outs_attr, shuffle);
  }
  //---------------------------------------------------------------

  void deterministic_generate_tx_onetime_key(std::list<crypto::key_image>& images, const account_keys& acc, keypair& res_keypair, uint64_t height = 0)
  {
    images.sort([](const crypto::key_image& a, const crypto::key_image& b) {return memcmp(&a, &b, sizeof(a)); });
    std::string hashing_buff;
    for (const auto& im : images)
      epee::string_tools::append_pod_to_strbuff(hashing_buff, im);
    epee::string_tools::append_pod_to_strbuff(hashing_buff, acc.spend_secret_key);
    hashing_buff.append(CRYPTO_HDS_DETERMINISTIC_TX_KEY);
    crypto::scalar_t sec_key = crypto::hash_helper_t::hs(hashing_buff.data(), hashing_buff.length());
    sec_key *= 8;
    res_keypair.sec = sec_key.as_secret_key();
    crypto::secret_key_to_public_key(res_keypair.sec, res_keypair.pub);
  }
  //---------------------------------------------------------------
  void deterministic_generate_tx_onetime_key(const transaction& tx, const account_keys& acc, keypair& res_keypair)
  {
    uint64_t h = 0;
    std::list<crypto::key_image> images;
    for (const auto& in : tx.vin)
    {
      crypto::key_image ki = AUTO_VAL_INIT(ki);
      if (get_key_image_from_txin_v(in, ki))
      {
        images.push_back(ki);
      }
      if (in.type() == typeid(txin_gen))
      {
        h = boost::get<txin_gen>(in).height;
        continue;
      }
    }
    deterministic_generate_tx_onetime_key(images, acc, res_keypair, h);
  }

  //---------------------------------------------------------------

  struct encrypt_attach_visitor : public boost::static_visitor<void>
  {
    bool& m_was_crypted_entries;
    const keypair& m_onetime_keypair;
    const account_public_address& m_destination_addr;
    const crypto::key_derivation& m_key;
    const account_keys& m_sender_account_keys;

    encrypt_attach_visitor(bool& was_crypted_entries, const crypto::key_derivation& key, const  keypair& onetime_keypair, const account_public_address& destination_addr, const account_keys& sender_account_keys) :
      m_was_crypted_entries(was_crypted_entries), m_key(key), m_onetime_keypair(onetime_keypair), m_destination_addr(destination_addr), m_sender_account_keys(sender_account_keys)
    {}
    void operator()(tx_comment& comment)
    {
      crypto::chacha_crypt(comment.comment, m_key);
      m_was_crypted_entries = true;
    }
    void operator()(tx_payer& pr)
    {
      crypto::chacha_crypt(pr.acc_addr, m_key);
      m_was_crypted_entries = true;
    }
    void operator()(tx_receiver& m)
    {
      crypto::chacha_crypt(m.acc_addr, m_key);
      m_was_crypted_entries = true;
    }
    void operator()(tx_payer_old& pr)
    {
      crypto::chacha_crypt(pr.acc_addr, m_key);
      m_was_crypted_entries = true;
    }
    void operator()(tx_receiver_old& m)
    {
      crypto::chacha_crypt(m.acc_addr, m_key);
      m_was_crypted_entries = true;
    }
    void operator()(tx_service_attachment& sa)
    {
      const std::string original_body = sa.body;
      if (sa.flags&TX_SERVICE_ATTACHMENT_DEFLATE_BODY)
      {
        zlib_helper::pack(sa.body);
      }

      if (sa.flags&TX_SERVICE_ATTACHMENT_ENCRYPT_BODY)
      {
        crypto::key_derivation derivation_local = m_key;
        if (sa.flags&TX_SERVICE_ATTACHMENT_ENCRYPT_BODY_ISOLATE_AUDITABLE)
        {
          CHECK_AND_ASSERT_THROW_MES(m_destination_addr.spend_public_key != currency::null_pkey && m_onetime_keypair.sec != currency::null_skey, "tx_service_attachment with TX_SERVICE_ATTACHMENT_ENCRYPT_BODY_ISOLATE_AUDITABLE: keys uninitialized");
          //encrypt with "spend keys" only, to prevent auditable watchers decrypt it
          bool r = crypto::generate_key_derivation(m_destination_addr.spend_public_key, m_onetime_keypair.sec, derivation_local);
          CHECK_AND_ASSERT_THROW_MES(r, "tx_service_attachment with TX_SERVICE_ATTACHMENT_ENCRYPT_BODY_ISOLATE_AUDITABLE: Failed to make derivation");
          crypto::chacha_crypt(sa.body, derivation_local);
          //we add derivation encrypted with sender spend key, so sender can derive information later if the wallet got restored from seed
          sa.security.push_back(*(crypto::public_key*)&derivation_local);
          crypto::chacha_crypt(sa.security.back(), m_sender_account_keys.spend_secret_key);
        }
        else
        {
          crypto::chacha_crypt(sa.body, derivation_local);
        }
        if (sa.flags&TX_SERVICE_ATTACHMENT_ENCRYPT_ADD_PROOF)
        {
          //take hash from derivation and use it as a salt
          crypto::hash derivation_hash = crypto::cn_fast_hash(&derivation_local, sizeof(derivation_local));
          std::string salted_body = original_body;
          string_tools::append_pod_to_strbuff(salted_body, derivation_hash);
          crypto::hash proof_hash = crypto::cn_fast_hash(salted_body.data(), salted_body.size());
          sa.security.push_back(*(crypto::public_key*)&proof_hash);
        }
        m_was_crypted_entries = true;
      }
    }

    template<typename attachment_t>
    void operator()(attachment_t& comment)
    {}
  };

  struct decrypt_attach_visitor : public boost::static_visitor<void>
  {
    const bool m_is_income;
    const transaction& m_tx;
    const account_keys& m_acc_keys;
    const crypto::public_key& m_tx_onetime_pubkey;
    const crypto::key_derivation& rkey;
    std::vector<payload_items_v>& rdecrypted_att;
    decrypt_attach_visitor(const crypto::key_derivation& key,
      std::vector<payload_items_v>& decrypted_att, 
      const account_keys& acc_keys,
      bool is_income,
      const transaction& tx,
      const crypto::public_key& tx_onetime_pubkey = null_pkey) :
      rkey(key),
      m_tx(tx),
      m_is_income(is_income),
      rdecrypted_att(decrypted_att), 
      m_acc_keys(acc_keys), 
      m_tx_onetime_pubkey(tx_onetime_pubkey)
    {}
    void operator()(const tx_comment& comment)
    {
      tx_comment local_comment = comment;
      crypto::chacha_crypt(local_comment.comment, rkey);
      rdecrypted_att.push_back(local_comment);
    }

    void operator()(const tx_service_attachment& sa)
    {
      tx_service_attachment local_sa = sa;
      crypto::key_derivation derivation_local = rkey;
      if (sa.flags&TX_SERVICE_ATTACHMENT_ENCRYPT_BODY)
      {
        if (sa.flags&TX_SERVICE_ATTACHMENT_ENCRYPT_BODY_ISOLATE_AUDITABLE)
        {
          if (!m_is_income)
          {
            //check if we have key for decrypting body for sender
            if (sa.security.size() < 1)
            {
              return; // this field invisible for sender
            }
            //decrypting derivation 
            derivation_local = *(crypto::key_derivation*)&sa.security[0];
            crypto::chacha_crypt(derivation_local, m_acc_keys.spend_secret_key);
          }
          else {
            if (m_acc_keys.spend_secret_key == currency::null_skey)
            {
              //tracking wallet, invisible
              return;
            }
            CHECK_AND_ASSERT_THROW_MES(m_acc_keys.spend_secret_key != currency::null_skey && m_tx_onetime_pubkey != currency::null_pkey, "tx_service_attachment with TX_SERVICE_ATTACHMENT_ENCRYPT_BODY_ISOLATE_AUDITABLE: keys uninitialized");
            bool r = crypto::generate_key_derivation(m_tx_onetime_pubkey, m_acc_keys.spend_secret_key, derivation_local);
            CHECK_AND_ASSERT_THROW_MES(r, "Failed to generate_key_derivation at TX_SERVICE_ATTACHMENT_ENCRYPT_BODY_ISOLATE_AUDITABLE");
          }
          crypto::chacha_crypt(local_sa.body, derivation_local);
        }
        else
        {
          crypto::chacha_crypt(local_sa.body, derivation_local);
        }  
      }

      if (sa.flags&TX_SERVICE_ATTACHMENT_DEFLATE_BODY)
      {
        zlib_helper::unpack(local_sa.body);
      }

      if (sa.flags&TX_SERVICE_ATTACHMENT_ENCRYPT_BODY && sa.flags&TX_SERVICE_ATTACHMENT_ENCRYPT_ADD_PROOF)
      {
        CHECK_AND_ASSERT_MES(sa.security.size() >= 1, void(), "Unexpected key in tx_service_attachment with TX_SERVICE_ATTACHMENT_ENCRYPT_BODY_ISOLATE_AUDITABLE");
        //take hash from derivation and use it as a salt
        crypto::hash derivation_hash = crypto::cn_fast_hash(&derivation_local, sizeof(derivation_local));
        std::string salted_body = local_sa.body;
        string_tools::append_pod_to_strbuff(salted_body, derivation_hash);
        crypto::hash proof_hash = crypto::cn_fast_hash(salted_body.data(), salted_body.size()); // proof_hash = Hs(local_sa.body || Hs(s * R)), s - spend secret, R - tx pub
        CHECK_AND_ASSERT_MES(*(crypto::public_key*)&proof_hash == sa.security.back(), void(), "Proof hash missmatch on decrypting with TX_SERVICE_ATTACHMENT_ENCRYPT_ADD_PROOF");
      }

      rdecrypted_att.push_back(local_sa);
    }

    void operator()(const tx_payer& pr)
    {
      tx_payer payer_local = pr;
      crypto::chacha_crypt(payer_local.acc_addr, rkey);
      rdecrypted_att.push_back(payer_local);
    }
    void operator()(const tx_receiver& pr)
    {
      tx_receiver receiver_local = pr;
      crypto::chacha_crypt(receiver_local.acc_addr, rkey);
      rdecrypted_att.push_back(receiver_local);
    }
    void operator()(const tx_payer_old& pr)
    {
      tx_payer_old payer_local = pr;
      crypto::chacha_crypt(payer_local.acc_addr, rkey);
      rdecrypted_att.push_back(payer_local);
    }
    void operator()(const tx_receiver_old& pr)
    {
      tx_receiver_old receiver_local = pr;
      crypto::chacha_crypt(receiver_local.acc_addr, rkey);
      rdecrypted_att.push_back(receiver_local);
    }
    template<typename attachment_t>
    void operator()(const attachment_t& att)
    {
      rdecrypted_att.push_back(att);
    }
  };

  //---------------------------------------------------------------
  template<class items_container_t>
  bool decrypt_payload_items(const crypto::key_derivation& derivation, const items_container_t& items_to_decrypt, std::vector<payload_items_v>& decrypted_att, bool is_income, const transaction& tx, const account_keys& acc_keys = null_acc_keys,
    const crypto::public_key& tx_onetime_pubkey = null_pkey)
  {
    decrypt_attach_visitor v(derivation, decrypted_att, acc_keys, is_income, tx, tx_onetime_pubkey);
    for (auto& a : items_to_decrypt)
      boost::apply_visitor(v, a);

    return true;
  }
  //---------------------------------------------------------------
  bool is_derivation_used_to_encrypt(const transaction& tx, const crypto::key_derivation& derivation)
  {
    tx_crypto_checksum crypto_info = AUTO_VAL_INIT(crypto_info);
    if (!get_type_in_variant_container(tx.extra, crypto_info) && !get_type_in_variant_container(tx.attachment, crypto_info))
    {
      //no crypt info in tx
      return false;
    }
    //validate derivation we here.
    crypto::hash hash_for_check_sum = crypto::cn_fast_hash(&derivation, sizeof(derivation));
    if (*(uint32_t*)&hash_for_check_sum == crypto_info.derivation_hash)
      return true;

    return false;
  }
  //---------------------------------------------------------------
  crypto::key_derivation get_encryption_key_derivation(bool is_income, const transaction& tx, const account_keys& acc_keys)
  {
    crypto::key_derivation derivation = null_derivation;
    if (is_income)
    {
      crypto::public_key tx_pub_key = currency::get_tx_pub_key_from_extra(tx);

      bool r = crypto::generate_key_derivation(tx_pub_key, acc_keys.view_secret_key, derivation);
      CHECK_AND_ASSERT_MES(r, null_derivation, "failed to generate_key_derivation");
      LOG_PRINT_GREEN("DECRYPTING ON KEY: " << epee::string_tools::pod_to_hex(derivation) << ", key derived from destination addr: " << currency::get_account_address_as_str(acc_keys.account_address), LOG_LEVEL_0);
      return derivation;
    }
    else
    {
      tx_crypto_checksum crypto_info = AUTO_VAL_INIT(crypto_info);
      if (!get_type_in_variant_container(tx.extra, crypto_info) && !get_type_in_variant_container(tx.attachment, crypto_info))
      {
        //no crypt info in tx
        return null_derivation;
      }
      if (acc_keys.spend_secret_key == currency::null_skey)
      {
        //traceable wallet and outgoing transfer - skip
        return null_derivation;
      }

      derivation = crypto_info.encrypted_key_derivation;
      crypto::chacha_crypt(derivation, acc_keys.spend_secret_key);
      LOG_PRINT_GREEN("DECRYPTING ON KEY: " << epee::string_tools::pod_to_hex(derivation) << ", key decrypted from sender address: " << currency::get_account_address_as_str(acc_keys.account_address), LOG_LEVEL_0);
      //validate derivation we here. Yoda style
      crypto::hash hash_for_check_sum = crypto::cn_fast_hash(&derivation, sizeof(derivation));
      CHECK_AND_ASSERT_MES(*(uint32_t*)&hash_for_check_sum == crypto_info.derivation_hash, null_derivation, "Derivation hash missmatched in tx id " << currency::get_transaction_hash(tx));
      return derivation;
    }
  }
  //---------------------------------------------------------------
  template<class total_t_container>
  struct back_inserter : public boost::static_visitor<void>
  {
    total_t_container& ttc;
    back_inserter(total_t_container& tt) :ttc(tt)
    {}
    template<typename attachment_t>
    void operator()(const attachment_t& att)
    {
      ttc.push_back(att);
    }
  };

  bool decrypt_payload_items(bool is_income, const transaction& tx, const account_keys& acc_keys, std::vector<payload_items_v>& decrypted_items)
  {
    PROFILE_FUNC("currency::decrypt_payload_items");
    crypto::key_derivation derivation = get_encryption_key_derivation(is_income, tx, acc_keys);
    if (derivation == null_derivation)
    {
      back_inserter<std::vector<payload_items_v> > v(decrypted_items);
      for (auto& a : tx.extra)
        boost::apply_visitor(v, a);

      for (auto& a : tx.attachment)
        boost::apply_visitor(v, a);
      
      return true;
    }
    const crypto::public_key onetime_pub = get_tx_pub_key_from_extra(tx);
    
    decrypt_payload_items(derivation, tx.extra, decrypted_items, is_income, tx, acc_keys, onetime_pub);
    decrypt_payload_items(derivation, tx.attachment, decrypted_items, is_income, tx, acc_keys, onetime_pub);
    return true;
  }

  //---------------------------------------------------------------
  void encrypt_attachments(transaction& tx, const account_keys& sender_keys, const account_public_address& destination_addr, const keypair& tx_random_key, crypto::key_derivation& derivation)
  {
    bool r = crypto::generate_key_derivation(destination_addr.view_public_key, tx_random_key.sec, derivation);
    CHECK_AND_ASSERT_MES(r, void(), "failed to generate_key_derivation");
    bool was_attachment_crypted_entries = false;
    bool was_extra_crypted_entries = false;

    encrypt_attach_visitor v(was_attachment_crypted_entries, derivation, tx_random_key, destination_addr, sender_keys);
    for (auto& a : tx.attachment)
      boost::apply_visitor(v, a);

    encrypt_attach_visitor v2(was_extra_crypted_entries, derivation, tx_random_key, destination_addr, sender_keys);
    for (auto& a : tx.extra)
      boost::apply_visitor(v2, a);


    if (was_attachment_crypted_entries || was_extra_crypted_entries)
    {
      crypto::hash hash_for_check_sum = crypto::cn_fast_hash(&derivation, sizeof(derivation));
      tx_crypto_checksum chs;
      //put derivation hash to be sure that we decrypting right way
      chs.derivation_hash = *(uint32_t*)&hash_for_check_sum;
      //put encrypted derivation to let sender decrypt all this data from attachment/extra
      chs.encrypted_key_derivation = derivation;
      crypto::chacha_crypt(chs.encrypted_key_derivation, sender_keys.spend_secret_key);
      if (was_extra_crypted_entries)
        tx.extra.push_back(chs);
      else
        tx.attachment.push_back(chs);
    }
  }
  //---------------------------------------------------------------
  void encrypt_attachments(transaction& tx, const account_keys& sender_keys, const account_public_address& destination_addr, const keypair& tx_random_key)
  {
    crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
    return encrypt_attachments(tx, sender_keys, destination_addr, tx_random_key, derivation);
  }
  //---------------------------------------------------------------
  void load_wallet_transfer_info_flags(tools::wallet_public::wallet_transfer_info& x)
  {
    x.is_service = currency::is_service_tx(x.tx);
    x.is_mixing = currency::does_tx_have_only_mixin_inputs(x.tx);
    x.is_mining = currency::is_coinbase(x.tx);
    if (!x.is_mining)
      x.fee = currency::get_tx_fee(x.tx);
    else
      x.fee = 0;
    x.show_sender = currency::is_showing_sender_addres(x.tx);
    tx_out_bare htlc_out = AUTO_VAL_INIT(htlc_out);
    txin_htlc htlc_in = AUTO_VAL_INIT(htlc_in);

    x.tx_type = get_tx_type_ex(x.tx, htlc_out, htlc_in);
    if(x.tx_type == GUI_TX_TYPE_HTLC_DEPOSIT && !x.has_outgoing_entries())
    {
      //need to override amount
      x.get_native_income_amount() = htlc_out.amount;
    }
  }

  //---------------------------------------------------------------
  uint64_t get_tx_type_ex(const transaction& tx, tx_out_bare& htlc_out, txin_htlc& htlc_in)
  {
    if (is_coinbase(tx))
      return GUI_TX_TYPE_COIN_BASE;

    // aliases
    tx_extra_info ei = AUTO_VAL_INIT(ei);
    parse_and_validate_tx_extra(tx, ei);
    if (ei.m_alias.m_alias.size())
    {
      if (ei.m_alias.m_sign.size())
        return GUI_TX_TYPE_UPDATE_ALIAS;
      else
        return GUI_TX_TYPE_NEW_ALIAS;
    }

    // offers
    tx_service_attachment a = AUTO_VAL_INIT(a);
    if (get_type_in_variant_container(tx.attachment, a))
    {
      if (a.service_id == BC_OFFERS_SERVICE_ID)
      {
        if (a.instruction == BC_OFFERS_SERVICE_INSTRUCTION_ADD)
          return GUI_TX_TYPE_PUSH_OFFER;
        else if (a.instruction == BC_OFFERS_SERVICE_INSTRUCTION_UPD)
          return GUI_TX_TYPE_UPDATE_OFFER;
        else if (a.instruction == BC_OFFERS_SERVICE_INSTRUCTION_DEL)
          return GUI_TX_TYPE_CANCEL_OFFER;
      }
    }

    // escrow
    tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
    if (bc_services::get_first_service_attachment_by_id(tx, BC_ESCROW_SERVICE_ID, BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_TEMPLATES, tsa))
      return GUI_TX_TYPE_ESCROW_TRANSFER;
    if (bc_services::get_first_service_attachment_by_id(tx, BC_ESCROW_SERVICE_ID, BC_ESCROW_SERVICE_INSTRUCTION_PROPOSAL, tsa))
      return GUI_TX_TYPE_ESCROW_PROPOSAL;
    if (bc_services::get_first_service_attachment_by_id(tx, BC_ESCROW_SERVICE_ID, BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL, tsa))
      return GUI_TX_TYPE_ESCROW_RELEASE_NORMAL;
    if (bc_services::get_first_service_attachment_by_id(tx, BC_ESCROW_SERVICE_ID, BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_BURN, tsa))
      return GUI_TX_TYPE_ESCROW_RELEASE_BURN;
    if (bc_services::get_first_service_attachment_by_id(tx, BC_ESCROW_SERVICE_ID, BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_CANCEL, tsa))
      return GUI_TX_TYPE_ESCROW_RELEASE_CANCEL;
    if (bc_services::get_first_service_attachment_by_id(tx, BC_ESCROW_SERVICE_ID, BC_ESCROW_SERVICE_INSTRUCTION_CANCEL_PROPOSAL, tsa))
      return GUI_TX_TYPE_ESCROW_CANCEL_PROPOSAL;

    for (auto ov : tx.vout)
    {
      VARIANT_SWITCH_BEGIN(ov);
      VARIANT_CASE_CONST(tx_out_bare, o)
        if (o.target.type() == typeid(txout_htlc))
        {
          htlc_out = o;
          return GUI_TX_TYPE_HTLC_DEPOSIT;
        }
      VARIANT_SWITCH_END();

    }

    if (get_type_in_variant_container(tx.vin, htlc_in))
    {
      return GUI_TX_TYPE_HTLC_REDEEM;
    }


    return GUI_TX_TYPE_NORMAL;
  }
  //---------------------------------------------------------------
  uint64_t get_tx_type(const transaction& tx)
  {
    tx_out_bare htlc_out = AUTO_VAL_INIT(htlc_out);
    txin_htlc htlc_in = AUTO_VAL_INIT(htlc_in);
    return get_tx_type_ex(tx, htlc_out, htlc_in);
  }
  //---------------------------------------------------------------
  size_t get_multisig_out_index(const std::vector<tx_out_v>& outs)
  {
    size_t n = 0;
    for (; n != outs.size(); n++)
    {
      VARIANT_SWITCH_BEGIN(outs[n]);
      VARIANT_CASE_CONST(tx_out_bare, o)
        if (o.target.type() == typeid(txout_multisig))
          break;
      VARIANT_CASE_CONST(tx_out_zarcanum, o)
        //@#@
      VARIANT_SWITCH_END();
    }
    return n;
  }
  //---------------------------------------------------------------
  size_t get_multisig_in_index(const std::vector<txin_v>& inputs)
  {
    size_t n = 0;
    for (; n != inputs.size(); n++)
    {
      if (inputs[n].type() == typeid(txin_multisig))
        break;
    }
    return n;
  }
  //---------------------------------------------------------------
  bool copy_all_derivation_hints_from_tx_to_container(transaction& tx, std::set<uint16_t>& derivation_hints)
  {
    for(auto it = tx.extra.begin(); it != tx.extra.end(); ++it)
    {
      if (it->type() == typeid(tx_derivation_hint))
      {
        uint16_t hint = 0;
        if (!get_uint16_from_tx_derivation_hint(boost::get<tx_derivation_hint>(*it), hint))
          return false;
        if (!derivation_hints.insert(hint).second)
          return false; // maybe we need to skip this?
      }
    }
    return true;
  }
  //---------------------------------------------------------------
  bool construct_tx(const account_keys& sender_account_keys,
    const std::vector<tx_source_entry>& sources,
    const std::vector<tx_destination_entry>& destinations,
    const std::vector<extra_v>& extra,
    const std::vector<attachment_v>& attachments,
    transaction& tx,
    uint64_t tx_version,
    crypto::secret_key& one_time_secret_key,
    uint64_t unlock_time,
    uint8_t tx_outs_attr,
    bool shuffle,
    uint64_t flags)
  {
    //
    //find target account to encrypt attachment.
    //for now we use just firs target account that is not sender, 
    //in case if there is no real targets we use sender credentials to encrypt attachments
    account_public_address crypt_destination_addr = get_crypt_address_from_destinations(sender_account_keys, destinations);

    return construct_tx(sender_account_keys, sources, destinations, extra, attachments, tx, tx_version, one_time_secret_key, unlock_time,
      crypt_destination_addr,
      0,
      tx_outs_attr,
      shuffle,
      flags);
  }
  //---------------------------------------------------------------
  bool construct_tx(const account_keys& sender_account_keys, const std::vector<tx_source_entry>& sources,
    const std::vector<tx_destination_entry>& destinations,
    const std::vector<extra_v>& extra,
    const std::vector<attachment_v>& attachments,
    transaction& tx,
    uint64_t tx_version,
    crypto::secret_key& one_time_secret_key,
    uint64_t unlock_time,
    const account_public_address& crypt_destination_addr,
    uint64_t expiration_time,
    uint8_t tx_outs_attr,
    bool shuffle,
    uint64_t flags)
  {
    //extra copy operation, but creating transaction is not sensitive to this
    finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
    ftp.tx_version = tx_version;
    ftp.sources = sources;
    ftp.prepared_destinations = destinations;
    ftp.extra = extra;
    ftp.attachments = attachments;
    ftp.unlock_time = unlock_time;
    ftp.crypt_address = crypt_destination_addr;
    ftp.expiration_time = expiration_time;
    ftp.tx_outs_attr = tx_outs_attr;
    ftp.shuffle = shuffle;
    ftp.flags = flags;

    finalized_tx ft = AUTO_VAL_INIT(ft);
    ft.tx = tx;
    ft.one_time_key = one_time_secret_key;
    bool r = construct_tx(sender_account_keys, ftp, ft);
    tx = ft.tx;
    one_time_secret_key = ft.one_time_key;
    return r;
  }
  //---------------------------------------------------------------
  // prepare inputs
  struct input_generation_context_data
  {
    keypair in_ephemeral    {};                           // ephemeral output key (stealth_address and secret_x)
    size_t real_out_index   = SIZE_MAX;                   // index of real output in local outputs vector
    std::vector<tx_source_entry::output_entry> outputs{}; // sorted by gindex
  };
  //--------------------------------------------------------------------------------
  bool generate_ZC_sig(const crypto::hash& tx_hash_for_signature, size_t input_index, const tx_source_entry& se, const input_generation_context_data& in_context,
    const account_keys& sender_account_keys, const uint64_t tx_flags, tx_generation_context& ogc, transaction& tx, bool last_output, bool separately_signed_tx_complete)
  {
    bool watch_only_mode = sender_account_keys.spend_secret_key == null_skey;
    CHECK_AND_ASSERT_MES(se.is_zc(), false, "sources contains a non-zc input");
    CHECK_AND_ASSERT_MES(input_index < tx.vin.size(), false, "input_index (" << input_index << ") is out-of-bounds, vin.size = " << tx.vin.size());
    CHECK_AND_ASSERT_MES(tx.vin[input_index].type() == typeid(txin_zc_input), false, "Unexpected type of input #" << input_index);

    txin_zc_input& in = boost::get<txin_zc_input>(tx.vin[input_index]);
    tx.signatures.emplace_back(ZC_sig());
    ZC_sig& sig = boost::get<ZC_sig>(tx.signatures.back());

    if (watch_only_mode)
      return true; // in this mode just append empty signatures

    crypto::point_t asset_id_pt(se.asset_id);
    crypto::point_t source_blinded_asset_id = asset_id_pt + se.real_out_asset_id_blinding_mask * crypto::c_point_X; // T_i = H_i + r_i * X
    CHECK_AND_ASSERT_MES(crypto::point_t(in_context.outputs[in_context.real_out_index].blinded_asset_id).modify_mul8() == source_blinded_asset_id, false, "real output blinded asset id check failed");
    ogc.real_zc_ins_asset_ids.emplace_back(asset_id_pt);

#ifndef NDEBUG
    {
      crypto::point_t source_amount_commitment = se.amount * source_blinded_asset_id + se.real_out_amount_blinding_mask * crypto::c_point_G;
      CHECK_AND_ASSERT_MES(crypto::point_t(in_context.outputs[in_context.real_out_index].amount_commitment).modify_mul8() == source_amount_commitment, false, "real output amount commitment check failed");
    }
#endif

    crypto::scalar_t pseudo_out_amount_blinding_mask = 0;
    crypto::scalar_t pseudo_out_asset_id_blinding_mask = crypto::scalar_t::random();
    if (last_output && ((tx_flags & TX_FLAG_SIGNATURE_MODE_SEPARATE) == 0 || separately_signed_tx_complete))
    {
      // either normal tx or the last signature of consolidated tx -- in both cases we need to calculate non-random blinding mask for pseudo output commitment
      pseudo_out_amount_blinding_mask = ogc.amount_blinding_masks_sum - ogc.pseudo_out_amount_blinding_masks_sum + (ogc.ao_commitment_in_outputs ? ogc.ao_amount_blinding_mask : -ogc.ao_amount_blinding_mask);      // A_1 - A^p_0 = (f_1 - f'_1) * G   =>  f'_{i-1} = sum{y_j} - sum{f'_i}
    }
    else
    {
      pseudo_out_amount_blinding_mask.make_random();
      ogc.pseudo_out_amount_blinding_masks_sum += pseudo_out_amount_blinding_mask;
    }

    DBG_VAL_PRINT("ZC sig generation");
    DBG_VAL_PRINT(input_index);
    DBG_VAL_PRINT(source_blinded_asset_id);
    DBG_VAL_PRINT(pseudo_out_asset_id_blinding_mask);
    DBG_VAL_PRINT(se.real_out_amount_blinding_mask);
    DBG_VAL_PRINT(se.real_out_asset_id_blinding_mask);
    DBG_VAL_PRINT(se.asset_id);
    DBG_VAL_PRINT(se.amount);
    DBG_VAL_PRINT(pseudo_out_amount_blinding_mask);

    crypto::point_t pseudo_out_blinded_asset_id = source_blinded_asset_id + pseudo_out_asset_id_blinding_mask * crypto::c_point_X;            // T^p_i = T_i + r'_i * X
    sig.pseudo_out_blinded_asset_id = (crypto::c_scalar_1div8 * pseudo_out_blinded_asset_id).to_public_key();
    ogc.real_in_asset_id_blinding_mask_x_amount_sum += se.real_out_asset_id_blinding_mask * se.amount;                                        // += r_i * a_i
    ogc.pseudo_outs_blinded_asset_ids.emplace_back(pseudo_out_blinded_asset_id);
    ogc.pseudo_outs_plus_real_out_blinding_masks.emplace_back(pseudo_out_asset_id_blinding_mask + se.real_out_asset_id_blinding_mask);

    crypto::point_t pseudo_out_amount_commitment = se.amount * source_blinded_asset_id + pseudo_out_amount_blinding_mask * crypto::c_point_G; // A^p_i = a_i * T_i + f'_i * G
    sig.pseudo_out_amount_commitment = (crypto::c_scalar_1div8 * pseudo_out_amount_commitment).to_public_key();
    ogc.pseudo_out_amount_commitments_sum += pseudo_out_amount_commitment;

    // = three-layers ring signature data outline =
    // (j in [0, ring_size-1])
    // layer 0 ring
    //     se.outputs[j].stealth_address;
    // layer 0 secret (with respect to G)
    //     in_contexts[i].in_ephemeral.sec;
    // layer 0 linkability
    //     in.k_image;
    //
    // layer 1 ring
    //     crypto::point_t(se.outputs[j].amount_commitment) - pseudo_out_amount_commitment;
    // layer 1 secret (with respect to G)
    //     se.real_out_amount_blinding_mask - pseudo_out_amount_blinding_mask;
    //
    // layer 2 ring
    //     crypto::point_t(se.outputs[j].blinded_asset_id) - pseudo_out_asset_id;
    // layer 2 secret (with respect to X)
    //     -pseudo_out_asset_id_blinding_mask;

    std::vector<crypto::CLSAG_GGX_input_ref_t> ring;
    for(size_t j = 0; j < in_context.outputs.size(); ++j)
      ring.emplace_back(in_context.outputs[j].stealth_address, in_context.outputs[j].amount_commitment, in_context.outputs[j].blinded_asset_id);

    return crypto::generate_CLSAG_GGX(tx_hash_for_signature, ring, pseudo_out_amount_commitment, pseudo_out_blinded_asset_id, in.k_image, in_context.in_ephemeral.sec,
      se.real_out_amount_blinding_mask - pseudo_out_amount_blinding_mask,
      -pseudo_out_asset_id_blinding_mask, in_context.real_out_index, sig.clsags_ggx);
  }
  //--------------------------------------------------------------------------------
  bool generate_NLSAG_sig(const crypto::hash& tx_hash_for_signature, const crypto::hash& tx_prefix_hash, size_t input_index, const tx_source_entry& src_entr,
    const account_keys& sender_account_keys, const input_generation_context_data& in_context, const keypair& txkey, const uint64_t tx_flags, transaction& tx, std::stringstream* pss_ring_s)
  {
    bool watch_only_mode = sender_account_keys.spend_secret_key == null_skey;
    CHECK_AND_ASSERT_MES(input_index < tx.vin.size(), false, "input_index (" << input_index << ") is out-of-bounds, vin.size = " << tx.vin.size());

    tx.signatures.push_back(NLSAG_sig());
    NLSAG_sig& nlsag = boost::get<NLSAG_sig>(tx.signatures.back());
    std::vector<crypto::signature>& sigs = nlsag.s;

    if (src_entr.is_multisig())
    {
      // NLSAG-based multisig -- don't sign anything here (see also sign_multisig_input_in_tx())
      sigs.resize(src_entr.ms_keys_count, null_sig); // just reserve keys.size() null signatures (NOTE: not minimum_sigs!)
    }
    else
    {
      // regular txin_to_key or htlc
      if (pss_ring_s)
        *pss_ring_s << "input #" << input_index << ", pub_keys:" << ENDL;

      std::vector<const crypto::public_key*> keys_ptrs;
      for(const tx_source_entry::output_entry& o : in_context.outputs)
      {
        keys_ptrs.push_back(&o.stealth_address);
        if (pss_ring_s)
          *pss_ring_s << o.stealth_address << ENDL;
      }
      sigs.resize(in_context.outputs.size());

      if (!watch_only_mode)
        crypto::generate_ring_signature(tx_hash_for_signature, get_key_image_from_txin_v(tx.vin[input_index]), keys_ptrs, in_context.in_ephemeral.sec, in_context.real_out_index, sigs.data());

      if (pss_ring_s)
      {
        *pss_ring_s << "signatures:" << ENDL;
        std::for_each(sigs.begin(), sigs.end(), [&pss_ring_s](const crypto::signature& s) { *pss_ring_s << s << ENDL; });
        *pss_ring_s << "prefix_hash: " << tx_prefix_hash << ENDL << "in_ephemeral_key: " << in_context.in_ephemeral.sec << ENDL << "real_output: " << in_context.real_out_index << ENDL;
      }
    }

    if (src_entr.separately_signed_tx_complete)
    {
      // if separately signed tx is complete, put one more signature to the last bunch using tx secret key, which confirms that transaction has been generated by authorized subject
      CHECK_AND_ASSERT_MES(input_index == tx.vin.size() - 1, false, "separately_signed_tx_complete flag is set for source entry #" << input_index << ", allowed only for the last one");
      CHECK_AND_ASSERT_MES(get_tx_flags(tx) & TX_FLAG_SIGNATURE_MODE_SEPARATE, false, "sorce entry separately_signed_tx_complete flag is set for tx with no TX_FLAG_SIGNATURE_MODE_SEPARATE flag");
      CHECK_AND_ASSERT_MES(tx_hash_for_signature == tx_prefix_hash, false, "internal error: hash_for_sign for the last input of separately signed complete tx expected to be the same as tx prefix hash");
      sigs.resize(sigs.size() + 1);
      crypto::generate_signature(tx_prefix_hash, txkey.pub, txkey.sec, sigs.back());
    }

    return true;
  }

#define CRYPTO_HASH_ASSET_ID_ITERATIONS 1024
  bool get_or_calculate_asset_id(const asset_descriptor_operation& ado, crypto::point_t* p_result_point, crypto::public_key* p_result_pub_key)
  {
    if (ado.operation_type == ASSET_DESCRIPTOR_OPERATION_EMIT        ||
        ado.operation_type == ASSET_DESCRIPTOR_OPERATION_UPDATE      ||
        ado.operation_type == ASSET_DESCRIPTOR_OPERATION_PUBLIC_BURN )
    {
      CHECK_AND_ASSERT_MES(ado.opt_asset_id.has_value(), false, "ado.opt_asset_id has no value, op: " << (int)ado.operation_type << ", " << get_asset_operation_type_string(ado.operation_type));
      //LOG_PRINT_YELLOW("ado.opt_asset_id = " << ado.opt_asset_id.get(), LOG_LEVEL_0);
      if (p_result_pub_key)
        *p_result_pub_key = ado.opt_asset_id.get();
      if (p_result_point)
        *p_result_point = crypto::point_t(ado.opt_asset_id.get());
      return true;
    }

    // otherwise, calculate asset id

    crypto::hash_helper_t::hs_t hsc;
    hsc.add_32_chars(CRYPTO_HDS_ASSET_ID);
    hsc.add_hash(crypto::hash_helper_t::h(ado.descriptor.ticker));
    hsc.add_hash(crypto::hash_helper_t::h(ado.descriptor.full_name));
    hsc.add_hash(crypto::hash_helper_t::h(ado.descriptor.meta_info));
    hsc.add_scalar(crypto::scalar_t(ado.descriptor.total_max_supply));
    hsc.add_scalar(crypto::scalar_t(ado.descriptor.decimal_point));
    hsc.add_pub_key(ado.descriptor.owner);
    crypto::hash h = hsc.calc_hash_no_reduce();

    // this hash function needs to be computationally expensive (s.a. the whitepaper)
    for(uint64_t i = 0; i < CRYPTO_HASH_ASSET_ID_ITERATIONS; ++i)
      h = get_hash_from_POD_objects(CRYPTO_HDS_ASSET_ID, h, i);

    crypto::point_t result = crypto::hash_helper_t::hp(&h, sizeof h);
    if (p_result_point)
      *p_result_point = result;
    if (p_result_pub_key)
      result.to_public_key(*p_result_pub_key);

    //LOG_PRINT_YELLOW("calculated asset_id = " << result, LOG_LEVEL_0);
    return true;
  }

  const asset_descriptor_base& get_native_coin_asset_descriptor()
  {
    static asset_descriptor_base native_coin_asset_descriptor = [](){
      asset_descriptor_base adb{};
      adb.total_max_supply  = UINT64_MAX;
      adb.current_supply    = 0;
      adb.decimal_point     = CURRENCY_DISPLAY_DECIMAL_POINT;
      adb.ticker            = CURRENCY_NAME_ABR;
      adb.full_name         = CURRENCY_NAME_BASE;
      adb.meta_info         = "";
      adb.owner             = currency::null_pkey;
      adb.hidden_supply     = false;
      adb.version           = 0;
      return adb;
    }();

    return native_coin_asset_descriptor;
  }

  bool construct_tx_handle_ado(const account_keys& sender_account_keys, 
    const finalize_tx_param& ftp, 
    asset_descriptor_operation& ado, 
    tx_generation_context& gen_context,
    const keypair& tx_key,
    std::vector<tx_destination_entry>& shuffled_dsts)
  {
    if (ado.operation_type == ASSET_DESCRIPTOR_OPERATION_REGISTER)
    {
      //crypto::secret_key asset_control_key{};
      //bool r = derive_key_pair_from_key_pair(sender_account_keys.account_address.spend_public_key, tx_key.sec, asset_control_key, ado.descriptor.owner, CRYPTO_HDS_ASSET_CONTROL_KEY);
      //CHECK_AND_ASSERT_MES(r, false, "derive_key_pair_from_key_pair failed");
      //
      // old:
      // asset_control_key = Hs(CRYPTO_HDS_ASSET_CONTROL_KEY, 8 * tx_key.sec * sender_account_keys.account_address.spend_public_key, 0)
      // ado.descriptor.owner = asset_control_key * G
      
      ado.descriptor.owner = sender_account_keys.account_address.spend_public_key;

      CHECK_AND_ASSERT_MES(get_or_calculate_asset_id(ado, &gen_context.ao_asset_id_pt, &gen_context.ao_asset_id), false, "get_or_calculate_asset_id failed");

      // calculate amount blinding mask
      gen_context.ao_amount_blinding_mask = crypto::hash_helper_t::hs(CRYPTO_HDS_ASSET_CONTROL_ABM, tx_key.sec);

      // set correct asset_id to the corresponding destination entries
      uint64_t amount_of_emitted_asset = 0;
      for (auto& item : shuffled_dsts)
      {
        if (item.asset_id == currency::null_pkey)
        {
          item.asset_id = gen_context.ao_asset_id; // set calculated asset_id to the asset's outputs, if this asset is being emitted within this tx
          amount_of_emitted_asset += item.amount;
        }
      }
      ado.descriptor.current_supply = amount_of_emitted_asset; // TODO: consider setting current_supply beforehand, not setting it hear in ad-hoc manner -- sowle

      gen_context.ao_amount_commitment = amount_of_emitted_asset * gen_context.ao_asset_id_pt + gen_context.ao_amount_blinding_mask * crypto::c_point_G;
      ado.amount_commitment = (crypto::c_scalar_1div8 * gen_context.ao_amount_commitment).to_public_key();
    }
    else if (ado.operation_type == ASSET_DESCRIPTOR_OPERATION_PUBLIC_BURN)
    {
      CHECK_AND_ASSERT_MES(get_or_calculate_asset_id(ado, &gen_context.ao_asset_id_pt, &gen_context.ao_asset_id), false, "get_or_calculate_asset_id failed");

      // calculate amount blinding mask
      gen_context.ao_amount_blinding_mask = crypto::hash_helper_t::hs(CRYPTO_HDS_ASSET_CONTROL_ABM, tx_key.sec);
      gen_context.ao_commitment_in_outputs = true;

      // set correct asset_id to the corresponding destination entries
      uint64_t amount_of_burned_assets = 0;
      for (auto& item : ftp.sources)
      {
        if (item.asset_id == gen_context.ao_asset_id)
        {
          amount_of_burned_assets += item.amount;
        }
      }
      for (auto& item : ftp.prepared_destinations)
      {
        if (item.asset_id == gen_context.ao_asset_id)
        {
          CHECK_AND_ASSERT_THROW_MES(amount_of_burned_assets >= item.amount, "Failed to find burn amount, failed condition: amount_of_burned_assets(" << amount_of_burned_assets << ") >= item.amount(" << item.amount << ")");
          amount_of_burned_assets -= item.amount;
        }
      }
      ado.descriptor.current_supply -= amount_of_burned_assets;

      gen_context.ao_amount_commitment = amount_of_burned_assets * gen_context.ao_asset_id_pt + gen_context.ao_amount_blinding_mask * crypto::c_point_G;
      ado.amount_commitment = (crypto::c_scalar_1div8 * gen_context.ao_amount_commitment).to_public_key();

      if (ftp.pevents_dispatcher) ftp.pevents_dispatcher->RAISE_DEBUG_EVENT(wde_construct_tx_handle_asset_descriptor_operation_before_burn{ &ado });
    
    }
    else
    {
      if (ado.operation_type == ASSET_DESCRIPTOR_OPERATION_EMIT)
      {
        CHECK_AND_ASSERT_MES(get_or_calculate_asset_id(ado, &gen_context.ao_asset_id_pt, &gen_context.ao_asset_id), false, "get_or_calculate_asset_id failed");

        // calculate amount blinding mask
        gen_context.ao_amount_blinding_mask = crypto::hash_helper_t::hs(CRYPTO_HDS_ASSET_CONTROL_ABM, tx_key.sec);

        // set correct asset_id to the corresponding destination entries
        uint64_t amount_of_emitted_asset = 0;
        for (auto& item : shuffled_dsts)
        {
          if (item.asset_id == currency::null_pkey)
          {
            amount_of_emitted_asset += item.amount;
            item.asset_id = gen_context.ao_asset_id;
          }
        }
        ado.descriptor.current_supply += amount_of_emitted_asset;

        gen_context.ao_amount_commitment = amount_of_emitted_asset * gen_context.ao_asset_id_pt + gen_context.ao_amount_blinding_mask * crypto::c_point_G;
        ado.amount_commitment = (crypto::c_scalar_1div8 * gen_context.ao_amount_commitment).to_public_key();

      }
      else if (ado.operation_type == ASSET_DESCRIPTOR_OPERATION_UPDATE)
      {
        CHECK_AND_ASSERT_MES(ado.opt_asset_id, false, "ado.opt_asset_id is not found at ado.operation_type == ASSET_DESCRIPTOR_OPERATION_UPDATE");
        //CHECK_AND_ASSERT_MES(ado.opt_proof, false, "ado.opt_asset_id is not found at ado.operation_type == ASSET_DESCRIPTOR_OPERATION_UPDATE");

        //fields that not supposed to be changed?
      }
      if (ftp.pevents_dispatcher) ftp.pevents_dispatcher->RAISE_DEBUG_EVENT(wde_construct_tx_handle_asset_descriptor_operation_before_seal{ &ado });

      ftp.need_to_generate_ado_proof = true;
      /*
      //seal it with owners signature
      crypto::signature sig = currency::null_sig;
      crypto::hash h = get_signature_hash_for_asset_operation(ado);
      if (ftp.pthirdparty_sign_handler)
      {
        bool r = ftp.pthirdparty_sign_handler->sign(h, ftp.ado_current_asset_owner, sig);
        CHECK_AND_ASSERT_MES(r, false, "asset thirparty sign failed");
      }
      else
      {
        crypto::public_key pub_k = currency::null_pkey;
        crypto::secret_key_to_public_key(sender_account_keys.spend_secret_key, pub_k);
        CHECK_AND_ASSERT_MES(ftp.ado_current_asset_owner == pub_k, false, "asset owner key not matched with provided private key for asset operation signing");
        crypto::generate_signature(h, pub_k, account_keys.spend_secret_key, sig);
      }
      ado.opt_proof = sig;
      */
    }
    return true;
  }

  bool construct_tx(const account_keys& sender_account_keys, const finalize_tx_param& ftp, finalized_tx& result)
  {
    const std::vector<tx_source_entry>& sources = ftp.sources;
    const std::vector<tx_destination_entry>& destinations = ftp.prepared_destinations;
    const std::vector<extra_v>& extra = ftp.extra;
    const std::vector<attachment_v>& attachments = ftp.attachments;
    const uint64_t& unlock_time = ftp.unlock_time;
    const account_public_address& crypt_destination_addr = ftp.crypt_address;
    const uint64_t& expiration_time = ftp.expiration_time;
    const uint8_t& tx_outs_attr = ftp.tx_outs_attr;
    const bool& shuffle = ftp.shuffle;
    const uint64_t& flags = ftp.flags;
    
    bool r = false;
    transaction& tx = result.tx;
    crypto::secret_key& one_time_tx_secret_key = result.one_time_key;

    result.ftp = ftp;
    CHECK_AND_ASSERT_MES(destinations.size() <= CURRENCY_TX_MAX_ALLOWED_OUTS, false, "Too many outs (" << destinations.size() << ")! Tx can't be constructed.");

    bool append_mode = false;
    if (flags&TX_FLAG_SIGNATURE_MODE_SEPARATE && tx.vin.size())
      append_mode = true;

    if (!append_mode)
    {
      tx.vin.clear();
      tx.vout.clear();
      tx.extra = extra;
      tx.signatures.clear();

      tx.version = ftp.tx_version;
      if (unlock_time != 0)
        set_tx_unlock_time(tx, unlock_time);

      if (flags != 0)
        set_tx_flags(tx, flags);
      //generate key pair  
      if (expiration_time != 0)
        set_tx_expiration_time(tx, expiration_time);
    }

    size_t zc_inputs_count = 0;
    bool has_non_zc_inputs = false;

    std::list<crypto::key_image> key_images_total;
    //
    // INs
    //
    uint64_t native_coins_input_sum = 0;
    std::vector<input_generation_context_data> in_contexts;
    std::vector<size_t> inputs_mapping;
    size_t current_index = 0;
    inputs_mapping.resize(sources.size());
    size_t input_starter_index = tx.vin.size();
    bool all_inputs_are_obviously_native_coins = true;
    for (const tx_source_entry& src_entr : sources)
    {
      inputs_mapping[current_index] = current_index;
      current_index++;
      in_contexts.push_back(input_generation_context_data{});
      input_generation_context_data& in_context = in_contexts.back();

      // sort src_entr.outputs entries by global out index, put it to in_context.outputs, convert to relative gindices, and put new real out index into in_context.real_out_index
      in_context.outputs = prepare_outputs_entries_for_key_offsets(src_entr.outputs, src_entr.real_output, in_context.real_out_index);

      if (src_entr.is_multisig())
      {//multisig input
        txin_multisig input_multisig = AUTO_VAL_INIT(input_multisig);
        input_multisig.amount = src_entr.amount;
        input_multisig.multisig_out_id = src_entr.multisig_id;
        input_multisig.sigs_count = src_entr.ms_sigs_count;
        tx.vin.push_back(input_multisig);
        has_non_zc_inputs = true;
      }
      else if (src_entr.htlc_origin.size())
      {
        //htlc redeem
        keypair& in_ephemeral = in_context.in_ephemeral;
        //txin_to_key
        if (src_entr.outputs.size() != 1)
        {
          LOG_ERROR("htlc in: wrong output src_entr.outputs.size() = " << src_entr.outputs.size());
          return false;
        }

        //key_derivation recv_derivation;
        crypto::key_image img;
        if (!generate_key_image_helper(sender_account_keys, src_entr.real_out_tx_key, src_entr.real_output_in_tx_index, in_ephemeral, img))
          return false;

        //check that derivated key is equal with real output key
        if (!(in_ephemeral.pub == src_entr.outputs.front().stealth_address))
        {
          LOG_ERROR("derived public key missmatch with output public key! " << ENDL << "derived_key:"
            << string_tools::pod_to_hex(in_ephemeral.pub) << ENDL << "real output_public_key:"
            << string_tools::pod_to_hex(src_entr.outputs.front().stealth_address));
          return false;
        }
        key_images_total.push_back(img);

        //put key image into tx input
        txin_htlc input_to_key;
        input_to_key.amount = src_entr.amount;
        input_to_key.k_image = img;
        input_to_key.hltc_origin = src_entr.htlc_origin;
        input_to_key.key_offsets.push_back(src_entr.outputs.front().out_reference);

        tx.vin.push_back(input_to_key);
        has_non_zc_inputs = true;
      }
      else
      {
        // txin_to_key or txin_zc_input
        CHECK_AND_ASSERT_MES(in_context.real_out_index < in_context.outputs.size(), false,
          "real_output index (" << in_context.real_out_index << ") greater than or equal to in_context.outputs.size()=" << in_context.outputs.size());

        crypto::key_image img;
        if (!generate_key_image_helper(sender_account_keys, src_entr.real_out_tx_key, src_entr.real_output_in_tx_index, in_context.in_ephemeral, img))
          return false;

        //check that derivated key is equal with real output key
        if (!(in_context.in_ephemeral.pub == in_context.outputs[in_context.real_out_index].stealth_address))
        {
          LOG_ERROR("derived public key missmatch with output public key! " << ENDL << "derived_key:"
            << string_tools::pod_to_hex(in_context.in_ephemeral.pub) << ENDL << "real output_public_key:"
            << string_tools::pod_to_hex(in_context.outputs[in_context.real_out_index].stealth_address));
          return false;
        }


        key_images_total.push_back(img);
        //fill key_offsets array with relative offsets
        std::vector<txout_ref_v> key_offsets;
        for (const tx_source_entry::output_entry& out_entry : in_context.outputs)
          key_offsets.push_back(out_entry.out_reference);

        //TODO: Might need some refactoring since this scheme is not the clearest one(did it this way for now to keep less changes to not broke anything)
        //potentially this approach might help to support htlc and multisig without making to complicated code
        if (src_entr.is_zc())
        {
          ++zc_inputs_count;
          txin_zc_input zc_in = AUTO_VAL_INIT(zc_in);
          zc_in.k_image = img;
          zc_in.key_offsets = std::move(key_offsets);
          tx.vin.push_back(zc_in);
        }
        else
        {
          txin_to_key input_to_key = AUTO_VAL_INIT(input_to_key);
          input_to_key.amount = src_entr.amount;
          input_to_key.k_image = img;
          input_to_key.key_offsets = std::move(key_offsets);
          tx.vin.push_back(input_to_key);
          has_non_zc_inputs = true;
        }
      }

      if (src_entr.is_native_coin())
        native_coins_input_sum += src_entr.amount;

      if (src_entr.is_zc())
      {
        // if at least one decoy output of a ZC input has a non-explicit asset id, then we can't say that all inputs are obviously native coins 
        for (const tx_source_entry::output_entry& oe : in_context.outputs)
        {
          if (crypto::point_t(oe.blinded_asset_id).modify_mul8() != currency::native_coin_asset_id_pt)
          {
            all_inputs_are_obviously_native_coins = false;
            break;
          }
        }
      }
    }

    if (zc_inputs_count != 0 && tx.version <= TRANSACTION_VERSION_PRE_HF4)
    {
      LOG_PRINT_YELLOW("WARNING: tx v1 should not use ZC inputs", LOG_LEVEL_0);
    }

    tx_generation_context& gen_context = result.ftp.gen_context;

    if (!append_mode)
    {
      gen_context.set_tx_key(keypair::generate());
      //deterministic_generate_tx_onetime_key(key_images_total, sender_account_keys, txkey);
      add_tx_pub_key_to_extra(tx, gen_context.tx_key.pub);
      one_time_tx_secret_key = gen_context.tx_key.sec;

      //add flags
      etc_tx_flags16_t e = AUTO_VAL_INIT(e);
      //todo: add some flags here
      update_or_add_field_to_extra(tx.extra, e);

      //include offers if need
      tx.attachment = attachments;
      encrypt_attachments(tx, sender_account_keys, crypt_destination_addr, gen_context.tx_key, result.derivation);
    }
    else
    {
      gen_context.set_tx_key(keypair{get_tx_pub_key_from_extra(tx), one_time_tx_secret_key});
      CHECK_AND_ASSERT_MES(gen_context.tx_key.pub != null_pkey && gen_context.tx_key.sec != null_skey, false, "In append mode both public and secret keys must be provided");

      //separately encrypt attachments without putting extra
      result.derivation = get_encryption_key_derivation(true, tx, sender_account_keys); 
      crypto::key_derivation& derivation = result.derivation;
      CHECK_AND_ASSERT_MES(derivation != null_derivation, false, "failed to generate_key_derivation");
      bool was_attachment_crypted_entries = false;
      std::vector<extra_v> extra_local = extra;
      std::vector<attachment_v> attachments_local = attachments;

      encrypt_attach_visitor v(was_attachment_crypted_entries, derivation, gen_context.tx_key, account_public_address(), sender_account_keys);
      for (auto& a : attachments_local)
        boost::apply_visitor(v, a);
      for (auto& a : extra_local)
        boost::apply_visitor(v, a);


      tx.attachment.insert(tx.attachment.end(), attachments_local.begin(), attachments_local.end());
      tx.extra.insert(tx.extra.end(), extra_local.begin(), extra_local.end());

      for (const auto& in : tx.vin)
      {
        if (in.type() == typeid(txin_zc_input))
        {
          zc_inputs_count++;
        }
        else {
          has_non_zc_inputs = true;
        }
      }
    }


    
    //
    // OUTs
    //
    std::vector<tx_destination_entry> shuffled_dsts(destinations);
    gen_context.resize(zc_inputs_count, tx.vout.size() + shuffled_dsts.size());

    // ASSET oprations handling
    if (tx.version > TRANSACTION_VERSION_PRE_HF4)
    {
      asset_descriptor_operation* pado = nullptr;
      pado = get_type_in_variant_container<asset_descriptor_operation>(tx.extra);
      if (pado)
      {
        bool r = construct_tx_handle_ado(sender_account_keys, ftp, *pado, gen_context, gen_context.tx_key, shuffled_dsts);
        CHECK_AND_ASSERT_MES(r, false, "Failed to construct_tx_handle_ado()");
        if (ftp.pevents_dispatcher) ftp.pevents_dispatcher->RAISE_DEBUG_EVENT(wde_construct_tx_handle_asset_descriptor_operation{ pado });
      }
    }

    // "Shuffle" outs
    if (shuffle)
      std::sort(shuffled_dsts.begin(), shuffled_dsts.end(), [](const tx_destination_entry& de1, const tx_destination_entry& de2) { return de1.amount < de2.amount; });

    // TODO: consider "Shuffle" inputs 

    // construct outputs
    uint64_t native_coins_output_sum = 0;
    size_t output_index = tx.vout.size(); // in case of append mode we need to start output indexing from the last one + 1
    uint64_t range_proof_start_index = 0;
    std::set<uint16_t> existing_derivation_hints, new_derivation_hints;
    CHECK_AND_ASSERT_MES(copy_all_derivation_hints_from_tx_to_container(tx, existing_derivation_hints), false, "move_all_derivation_hints_from_tx_to_container failed");
    for(size_t destination_index = 0; destination_index < shuffled_dsts.size(); ++destination_index, ++output_index)
    {
      tx_destination_entry& dst_entr = shuffled_dsts[destination_index];
      if (!(flags & TX_FLAG_SIGNATURE_MODE_SEPARATE) && all_inputs_are_obviously_native_coins && gen_context.ao_asset_id == currency::null_pkey)
        dst_entr.flags |= tx_destination_entry_flags::tdef_explicit_native_asset_id; // all inputs are obviously native coins -- all outputs must have explicit asset ids (unless there's an asset emission)

      r = construct_tx_out(dst_entr, gen_context.tx_key.sec, output_index, tx, new_derivation_hints, sender_account_keys,
        gen_context.asset_id_blinding_masks[output_index], gen_context.amount_blinding_masks[output_index],
        gen_context.blinded_asset_ids[output_index], gen_context.amount_commitments[output_index], result, tx_outs_attr);
      CHECK_AND_ASSERT_MES(r, false, "Failed to construct tx out");
      gen_context.amounts[output_index] = dst_entr.amount;
      gen_context.asset_ids[output_index] = crypto::point_t(dst_entr.asset_id);
      gen_context.asset_id_blinding_mask_x_amount_sum += gen_context.asset_id_blinding_masks[output_index] * dst_entr.amount;
      gen_context.amount_blinding_masks_sum += gen_context.amount_blinding_masks[output_index];
      gen_context.amount_commitments_sum += gen_context.amount_commitments[output_index];
      if (dst_entr.is_native_coin())
        native_coins_output_sum += dst_entr.amount;
    }

    CHECK_AND_ASSERT_MES(add_random_derivation_hints_and_put_them_to_tx(flags, existing_derivation_hints, new_derivation_hints, tx), false, "add_random_derivation_hints_and_put_them_to_tx failed");

    //process offers and put there offers derived keys
    uint64_t att_count = 0;
    for (auto& o : tx.attachment)
    {
      if (o.type() == typeid(tx_service_attachment))
      {
        tx_service_attachment& tsa = boost::get<tx_service_attachment>(o);
        if (tsa.security.size())
        {
          CHECK_AND_ASSERT_MES(tsa.security.size() == 1, false, "Wrong tsa.security.size() = " << tsa.security.size());

          r = derive_public_key_from_target_address(sender_account_keys.account_address, one_time_tx_secret_key, att_count, tsa.security.back());
          CHECK_AND_ASSERT_MES(r, false, "Failed to derive_public_key_from_target_address");
        }
        att_count++;
      }
    }

    //sort inputs and mapping if it's zarcanum hardfork
    if (tx.version > TRANSACTION_VERSION_PRE_HF4)
    {
      //1 sort mapping
      std::sort(inputs_mapping.begin(), inputs_mapping.end(), [&](size_t a, size_t b) {
        return less_txin_v(tx.vin[input_starter_index + a], tx.vin[input_starter_index + b]);
      });

      //2 sort the inputs in given range
      std::sort(tx.vin.begin() + input_starter_index, tx.vin.end(), less_txin_v);

      // add explicit fee info
      uint64_t fee_to_declare = native_coins_input_sum - native_coins_output_sum;
      if (flags & TX_FLAG_SIGNATURE_MODE_SEPARATE)
      {
        fee_to_declare = ftp.mode_separate_fee;
      }
      if (fee_to_declare)
      {
        r = add_tx_fee_amount_to_extra(tx, fee_to_declare);
        CHECK_AND_ASSERT_MES(r, false, "add_tx_fee_amount_to_extra failed");
      }
    }

    // attachments container should be sealed by now

    if (flags & TX_FLAG_SIGNATURE_MODE_SEPARATE)
    {
      // for separately signed tx each input has to contain information about corresponding outputs, extra entries and attachments
      for (size_t in_index = input_starter_index; in_index != tx.vin.size(); in_index++)
      {
        // add signed_parts to each input's details (number of outputs and extra entries)
        signed_parts so = AUTO_VAL_INIT(so);
        so.n_outs = tx.vout.size();
        so.n_extras = tx.extra.size();
        get_txin_etc_options(tx.vin[in_index]).push_back(so);

        // put attachment extra info to each input's details (in case there are attachments)
        add_attachments_info_to_extra(get_txin_etc_options(tx.vin[in_index]), tx.attachment);
      }
    }
    else
    {
      //take hash from attachment and put into extra
      if (tx.attachment.size())
        add_attachments_info_to_extra(tx.extra, tx.attachment);
    }
    

    //
    // generate proofs and signatures
    // (any changes made below should only affect the signatures/proofs and should not impact the prefix hash calculation)
    //
    crypto::hash tx_prefix_hash = get_transaction_prefix_hash(tx);

    // ring signatures (per-input proofs)
    r = false;
    bool separately_signed_tx_complete = !sources.empty() ? sources.back().separately_signed_tx_complete : false;
    size_t zc_input_index = 0;
    for (size_t i_ = 0; i_ != sources.size(); i_++)
    {
      size_t i_mapped = inputs_mapping[i_];
      const tx_source_entry& source_entry = sources[i_mapped];
      crypto::hash tx_hash_for_signature = prepare_prefix_hash_for_sign(tx, i_ + input_starter_index, tx_prefix_hash);
      CHECK_AND_ASSERT_MES(tx_hash_for_signature != null_hash, false, "prepare_prefix_hash_for_sign failed");      
      std::stringstream ss_ring_s;

      if (source_entry.is_zc())
      {
        // ZC
        r = generate_ZC_sig(tx_hash_for_signature, i_ + input_starter_index, source_entry, in_contexts[i_mapped], sender_account_keys, flags, gen_context, tx, i_ + 1 == sources.size(), separately_signed_tx_complete);
        CHECK_AND_ASSERT_MES(r, false, "generate_ZC_sigs failed");
        gen_context.zc_input_amounts[zc_input_index] = source_entry.amount;
        zc_input_index++;
      }
      else
      {
        // NLSAG
        r = generate_NLSAG_sig(tx_hash_for_signature, tx_prefix_hash, i_ + input_starter_index, source_entry, sender_account_keys, in_contexts[i_mapped], gen_context.tx_key, flags, tx, &ss_ring_s);
        CHECK_AND_ASSERT_MES(r, false, "generate_NLSAG_sig failed");
      }

      LOG_PRINT2("construct_tx.log", "transaction_created: " << get_transaction_hash(tx) << ENDL << obj_to_json_str(tx) << ENDL << ss_ring_s.str(), LOG_LEVEL_3);
    }

    //
    // proofs (transaction-wise, not pre-input)
    //
    if (tx.version > TRANSACTION_VERSION_PRE_HF4 &&
      (append_mode || (flags & TX_FLAG_SIGNATURE_MODE_SEPARATE) == 0))
    {
      // asset surjection proof
      currency::zc_asset_surjection_proof asp{};
      bool r = generate_asset_surjection_proof(tx_prefix_hash, has_non_zc_inputs, gen_context, asp);
      CHECK_AND_ASSERT_MES(r, false, "generete_asset_surjection_proof failed");
      tx.proofs.emplace_back(std::move(asp));

      // range proofs
      currency::zc_outs_range_proof range_proofs{};
      r = generate_zc_outs_range_proof(tx_prefix_hash, range_proof_start_index, gen_context, tx.vout, range_proofs);
      CHECK_AND_ASSERT_MES(r, false, "Failed to generate zc_outs_range_proof()");
      tx.proofs.emplace_back(std::move(range_proofs));

      // balance proof
      currency::zc_balance_proof balance_proof{};
      r = generate_tx_balance_proof(tx, tx_prefix_hash, gen_context, 0, balance_proof);
      CHECK_AND_ASSERT_MES(r, false, "generate_tx_balance_proof failed");
      tx.proofs.emplace_back(std::move(balance_proof));

      // asset operation proof (if necessary)
      if (gen_context.ao_asset_id != currency::null_pkey)
      {
        // construct the asset operation proof
        // TODO @#@# add support for hidden supply
        crypto::signature aop_g_sig{};
        crypto::generate_signature(tx_prefix_hash, crypto::point_t(gen_context.ao_amount_blinding_mask * crypto::c_point_G).to_public_key(), gen_context.ao_amount_blinding_mask, aop_g_sig);
        asset_operation_proof aop{};
        aop.opt_amount_commitment_g_proof = aop_g_sig;
        tx.proofs.emplace_back(std::move(aop));
      }
      if(ftp.need_to_generate_ado_proof)        
      { 
        asset_operation_ownership_proof aoop = AUTO_VAL_INIT(aoop);

        if (ftp.pthirdparty_sign_handler)
        {
          //ask third party to generate proof
          r = ftp.pthirdparty_sign_handler->sign(tx_prefix_hash, ftp.ado_current_asset_owner, aoop.gss);
          CHECK_AND_ASSERT_MES(r, false, "Failed to sign ado by thirdparty");
        }
        else
        {
          //generate signature by wallet account 
          r = crypto::generate_schnorr_sig(tx_prefix_hash, ftp.ado_current_asset_owner, sender_account_keys.spend_secret_key, aoop.gss);
          CHECK_AND_ASSERT_MES(r, false, "Failed to sign ado proof");
        }
        if (ftp.pevents_dispatcher) ftp.pevents_dispatcher->RAISE_DEBUG_EVENT(wde_construct_tx_after_asset_ownership_proof_generated{ &aoop });
        tx.proofs.emplace_back(aoop);
      }
    }

    //size_t prefix_size = get_object_blobsize(static_cast<const transaction_prefix&>(tx));
    //size_t full_blob_size = t_serializable_object_to_blob(tx).size();
    //size_t estimated_blob_size = get_object_blobsize(tx);
    //CHECK_AND_ASSERT_MES(full_blob_size == estimated_blob_size, false, "!");

    return true;
  }


  //---------------------------------------------------------------
  uint64_t get_tx_version(uint64_t tx_expected_block_height, const hard_forks_descriptor& hfd)
  {
    if (!hfd.is_hardfork_active_for_height(ZANO_HARDFORK_04_ZARCANUM, tx_expected_block_height))
    {
      return TRANSACTION_VERSION_PRE_HF4;
    }
    return CURRENT_TRANSACTION_VERSION;
  }
  //---------------------------------------------------------------
  // TODO @#@# this function is obsolete and needs to be re-written
  uint64_t get_reward_from_miner_tx(const transaction& tx)
  {
    uint64_t income = 0;
    for (auto& in : tx.vin)
    {
      if (in.type() == typeid(txin_gen))
      {
        continue;
      }
      else if (in.type() == typeid(txin_to_key))
      {
        income += boost::get<txin_to_key>(in).amount;
      }
    }
    uint64_t reward = 0;
    for (auto& out : tx.vout)
    {
      VARIANT_SWITCH_BEGIN(out);
      VARIANT_CASE_CONST(tx_out_bare, o)
        reward += o.amount;
      VARIANT_CASE_CONST(tx_out_zarcanum, o)
        //@#@      
      VARIANT_SWITCH_END();
    }
    reward -= income;
    return reward;
  }
  //---------------------------------------------------------------
  std::string get_word_from_timstamp(uint64_t timestamp, bool use_password)
  {
    uint64_t date_offset = timestamp > WALLET_BRAIN_DATE_OFFSET ? timestamp - WALLET_BRAIN_DATE_OFFSET : 0;
    uint64_t weeks_count = date_offset / WALLET_BRAIN_DATE_QUANTUM;
    CHECK_AND_ASSERT_THROW_MES(weeks_count < WALLET_BRAIN_DATE_MAX_WEEKS_COUNT, "SEED PHRASE need to be extended or refactored");

    if (use_password)
      weeks_count += WALLET_BRAIN_DATE_MAX_WEEKS_COUNT;

    CHECK_AND_ASSERT_THROW_MES(weeks_count < std::numeric_limits<uint32_t>::max(), "internal error: unable to convert to uint32, val = " << weeks_count);
    uint32_t weeks_count_32 = static_cast<uint32_t>(weeks_count);

    return tools::mnemonic_encoding::word_by_num(weeks_count_32);
  }
  //---------------------------------------------------------------
  uint64_t get_timstamp_from_word(std::string word, bool& password_used)
  {
    uint64_t count_of_weeks = tools::mnemonic_encoding::num_by_word(word);
    if (count_of_weeks >= WALLET_BRAIN_DATE_MAX_WEEKS_COUNT)
    {
      count_of_weeks -= WALLET_BRAIN_DATE_MAX_WEEKS_COUNT;
      password_used = true;
    }
    else {
      password_used = false;
    }
    uint64_t timestamp = count_of_weeks * WALLET_BRAIN_DATE_QUANTUM + WALLET_BRAIN_DATE_OFFSET;
    
    return timestamp;
  }
  //---------------------------------------------------------------
  bool parse_vote(const std::string& json_, std::list<std::pair<std::string, bool>>& votes)
  {
    //do preliminary check of text if it looks like json
    std::string::size_type pos = json_.find('{');
    if (pos == std::string::npos)
      return false;
    std::string json = json_.substr(pos);

    epee::serialization::portable_storage ps;
    bool rs = ps.load_from_json(json);
    if (!rs)
      return false;



    auto cb = [&](const std::string& name, const epee::serialization::storage_entry& entry) {
      if (entry.type() == typeid(uint64_t))
      {
        bool vote = boost::get<uint64_t>(entry) ? true : false;
        votes.push_back(std::make_pair(epee::string_encoding::toupper(name), vote));
      }
      return true;
    };

    ps.enum_entries(nullptr, cb);
    return true;

  }
  //---------------------------------------------------------------
  bool sign_multisig_input_in_tx(currency::transaction& tx, size_t ms_input_index, const currency::account_keys& keys, const currency::transaction& source_tx, bool *p_is_input_fully_signed /* = nullptr */)
  {
#define LOC_CHK(cond, msg) CHECK_AND_ASSERT_MES(cond, false, msg << ", ms input index: " << ms_input_index << ", tx: " << get_transaction_hash(tx) << ", source tx: " << get_transaction_hash(source_tx))
    if (p_is_input_fully_signed != nullptr)
      *p_is_input_fully_signed = false;

    LOC_CHK(ms_input_index < tx.vin.size(), "ms input index is out of bounds, vin.size() = " << tx.vin.size());
    LOC_CHK(tx.vin[ms_input_index].type() == typeid(txin_multisig), "ms input has wrong type, txin_multisig expected");
    const txin_multisig& ms_in = boost::get<txin_multisig>(tx.vin[ms_input_index]);

    // search ms output in source tx by ms_in.multisig_out_id
    size_t ms_out_index = SIZE_MAX;
    for (size_t i = 0; i < source_tx.vout.size(); ++i)
    {
      VARIANT_SWITCH_BEGIN(source_tx.vout[i]);
      VARIANT_CASE_CONST(tx_out_bare, o)
        if (o.target.type() == typeid(txout_multisig) && ms_in.multisig_out_id == get_multisig_out_id(source_tx, i))
        {
          ms_out_index = i;
          break;
        }
      VARIANT_SWITCH_END();
    }
    LOC_CHK(ms_out_index != SIZE_MAX, "failed to find ms output in source tx " << get_transaction_hash(source_tx) << " by ms id " << ms_in.multisig_out_id);
    const txout_multisig& out_ms = boost::get<txout_multisig>( boost::get<tx_out_bare>(source_tx.vout[ms_out_index]).target);

    crypto::public_key source_tx_pub_key = get_tx_pub_key_from_extra(source_tx);

    keypair ms_in_ephemeral_key = AUTO_VAL_INIT(ms_in_ephemeral_key);
    bool r = currency::derive_ephemeral_key_helper(keys, source_tx_pub_key, ms_out_index, ms_in_ephemeral_key);
    LOC_CHK(r, "derive_ephemeral_key_helper failed");

    size_t participant_index = std::find(out_ms.keys.begin(), out_ms.keys.end(), ms_in_ephemeral_key.pub) - out_ms.keys.begin();
    LOC_CHK(participant_index < out_ms.keys.size(), "Can't find given participant's ms key in ms output keys list");
    LOC_CHK(ms_input_index <  tx.signatures.size(), "transaction does not have signatures vector entry for ms input #" << ms_input_index);
    
    LOC_CHK(tx.signatures[ms_input_index].type() == typeid(NLSAG_sig), "Wrong type of signature");
    auto& sigs = boost::get<NLSAG_sig>(tx.signatures[ms_input_index]).s;
    LOC_CHK(!sigs.empty(), "empty signatures container");

    bool extra_signature_expected = (get_tx_flags(tx) & TX_FLAG_SIGNATURE_MODE_SEPARATE) && ms_input_index == tx.vin.size() - 1;
    size_t allocated_sigs_for_participants = extra_signature_expected ? sigs.size() - 1 : sigs.size();
    LOC_CHK(participant_index < allocated_sigs_for_participants, "participant index (" << participant_index << ") is out of bound: " << allocated_sigs_for_participants); // NOTE: this may fail if the input has already been fully signed and 'sigs' was compacted
  
    crypto::hash tx_hash_for_signature = prepare_prefix_hash_for_sign(tx, ms_input_index, get_transaction_hash(tx));
    LOC_CHK(tx_hash_for_signature != null_hash, "failed to  prepare_prefix_hash_for_sign");

    crypto::generate_signature(tx_hash_for_signature, ms_in_ephemeral_key.pub, ms_in_ephemeral_key.sec, sigs[participant_index]);

    // check whether the input is fully signed
    size_t non_null_sigs_count = 0;
    for (size_t i = 0; i < allocated_sigs_for_participants; ++i)
    {
      if (sigs[i] != null_sig)
        ++non_null_sigs_count;
    }
    LOC_CHK(non_null_sigs_count <= out_ms.minimum_sigs, "somehow there are too many non-null signatures for this input: " << non_null_sigs_count << ", minimum_sigs: " << out_ms.minimum_sigs);
    if (non_null_sigs_count == out_ms.minimum_sigs)
    {
      // this input is fully signed, now we gonna compact sigs container by removing null signatures
      sigs.erase(std::remove(sigs.begin(), sigs.end(), null_sig), sigs.end());

      if (p_is_input_fully_signed != nullptr)
        *p_is_input_fully_signed = true;
    }

    return true;
#undef LOC_CHK
  }
  //---------------------------------------------------------------
  uint64_t get_inputs_money_amount(const transaction& tx)
  {
    uint64_t r = 0;
    get_inputs_money_amount(tx, r);
    return r;
  }
  //---------------------------------------------------------------
  bool get_inputs_money_amount(const transaction& tx, uint64_t& money)
  {
    money = 0;
    for(const auto& in : tx.vin)
    {
      uint64_t this_amount = get_amount_from_variant(in);
      money += this_amount;
    }
    return true;
  }
  //---------------------------------------------------------------
  uint64_t get_block_height(const transaction& coinbase)
  {
    CHECK_AND_ASSERT_MES(coinbase.vin.size() == 1 || coinbase.vin.size() == 2, 0, "wrong miner tx in block: -----, b.miner_tx.vin.size() != 1");
    CHECKED_GET_SPECIFIC_VARIANT(coinbase.vin[0], const txin_gen, coinbase_in, 0);
    return coinbase_in.height;
  }
  //---------------------------------------------------------------
  uint64_t get_block_height(const block& b)
  {
    return get_block_height(b.miner_tx);
  }
  //---------------------------------------------------------------
  bool check_inputs_types_supported(const transaction& tx)
  {
    for(const auto& in : tx.vin)
    {
      CHECK_AND_ASSERT_MES(
        in.type() == typeid(txin_to_key) ||
        in.type() == typeid(txin_multisig) ||
        in.type() == typeid(txin_htlc) ||
        in.type() == typeid(txin_zc_input), 
        false, "wrong input type: " << in.type().name() << ", in transaction " << get_transaction_hash(tx));
    }
    return true;
  }
  //------------------------------------------------------------------
  /*
  bool add_padding_to_tx(transaction& tx, size_t count)
  {
    
    WARNING: potantially unsafe implementation!
    1) requires extra_padding being previously added to tx's extra;
    2) transaction size may increase by more than 'count' bytes due to varint encoding (thus, if 'count' is 128 it will add 129 bytes)
    See also check_add_padding_to_tx test (uncomment and update it, if necessary).

    if (!count)
      return true;

    for (auto& ex : tx.extra)
    {
      if (ex.type() == typeid(extra_padding))
      {
        boost::get<extra_padding>(ex).buff.insert(boost::get<extra_padding>(ex).buff.end(), count, 0);
        return true;
      }
    }
    CHECK_AND_ASSERT_THROW_MES(false, "extra_padding entry not found in template mining transaction");
    return false;
  }
  */
  //------------------------------------------------------------------
  bool remove_padding_from_tx(transaction& tx, size_t count)
  {
    if (!count)
      return true;

    for (auto ex : tx.extra)
    {
      if (ex.type() == typeid(extra_padding))
      {
        std::vector<uint8_t>& buff = boost::get<extra_padding>(ex).buff;
        CHECK_AND_ASSERT_MES(buff.size() >= count, false, "Attempt to remove_padding_from_tx for count = " << count << ", while buff.size()=" << buff.size());
        buff.resize(buff.size() - count);
        return true;
      }
    }
    CHECK_AND_ASSERT_THROW_MES(false, "extra_padding entry not found in template mining transaction");
    return false;
  }
  //------------------------------------------------------------------
  bool is_tx_spendtime_unlocked(uint64_t unlock_time, uint64_t current_blockchain_size, uint64_t current_time)
  {
    if (unlock_time < CURRENCY_MAX_BLOCK_NUMBER)
    {
      //interpret as block index
      return unlock_time <= current_blockchain_size - 1 + CURRENCY_LOCKED_TX_ALLOWED_DELTA_BLOCKS;
    }

    //interpret as time
    return unlock_time <= current_time + CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS;
  }
  //-----------------------------------------------------------------------------------------------
  bool check_outs_valid(const transaction& tx)
  {
    for(const auto& vo : tx.vout)
    {
      VARIANT_SWITCH_BEGIN(vo);
      VARIANT_CASE_CONST(tx_out_bare, out)
      {
        CHECK_AND_NO_ASSERT_MES(0 < out.amount, false, "zero amount output in transaction id=" << get_transaction_hash(tx));
        
        VARIANT_SWITCH_BEGIN(out.target);
        VARIANT_CASE_CONST(txout_to_key, tk)
          if (!check_key(tk.key))
            return false;
        VARIANT_CASE_CONST(txout_htlc, htlc)
          if (!check_key(htlc.pkey_redeem))
            return false;
          if (!check_key(htlc.pkey_refund))
            return false;
        VARIANT_CASE_CONST(txout_multisig, ms)
          if (!(ms.keys.size() > 0 && ms.minimum_sigs > 0 && ms.minimum_sigs <= ms.keys.size()))
          {
            LOG_ERROR("wrong multisig output in transaction " << get_transaction_hash(tx));
            return false;
          }
        VARIANT_CASE_OTHER()
          LOG_ERROR("wrong output type: " << out.target.type().name() << " in transaction " << get_transaction_hash(tx));
          return false;
        VARIANT_SWITCH_END();
      }
      VARIANT_CASE_CONST(tx_out_zarcanum, o)
      {
        if (!check_key(o.stealth_address))
          return false;
        if (!check_key(o.concealing_point))
          return false;
        if (!check_key(o.amount_commitment))
          return false;
        if (!check_key(o.blinded_asset_id))
          return false;
      }
      VARIANT_SWITCH_END();
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool check_bare_money_overflow(const transaction& tx)
  {
    return check_bare_inputs_overflow(tx) && check_bare_outs_overflow(tx);
  }
  //---------------------------------------------------------------
  bool check_bare_inputs_overflow(const transaction& tx)
  {
    uint64_t money = 0;
    for(const auto& in : tx.vin)
    {
      uint64_t this_amount = 0;
      if (in.type() == typeid(txin_to_key))
      {
        CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, tokey_in, false);
        this_amount = tokey_in.amount;
      }
      else if (in.type() == typeid(txin_multisig))
      {
        CHECKED_GET_SPECIFIC_VARIANT(in, const txin_multisig, ms_in, false);
        this_amount = ms_in.amount;
      }
      else if (in.type() == typeid(txin_htlc))
      {
        CHECKED_GET_SPECIFIC_VARIANT(in, const txin_htlc, htlc_in, false);
        this_amount = htlc_in.amount;
      }
      else if (in.type() == typeid(txin_zc_input))
      {
        // ignore inputs with hidden amounts
      }
      else
      {
        LOG_ERROR("wrong input type: " << in.type().name());
        return false;
      }
      if (money > this_amount + money)
        return false;
      money += this_amount;
    }
    return true;
  }
  //---------------------------------------------------------------
  bool check_bare_outs_overflow(const transaction& tx)
  {
    uint64_t money = 0;
    for(const auto& o : tx.vout)
    {
      VARIANT_SWITCH_BEGIN(o);
      VARIANT_CASE_CONST(tx_out_bare, o)
        if (money > o.amount + money)
          return false;
        money += o.amount;
      // VARIANT_CASE_CONST(tx_out_zarcanum, o)
        // ignore inputs with hidden amounts
      VARIANT_SWITCH_END();
    }
    return true;
  }
  //---------------------------------------------------------------
  uint64_t get_outs_money_amount(const transaction& tx, const currency::account_keys& keys /* = currency::null_acc_keys */)
  {
    uint64_t outputs_amount = 0;

    bool process_hidden_amounts = false;
    crypto::key_derivation derivation = null_derivation;
    if (keys.spend_secret_key != null_skey && keys.view_secret_key != null_skey)
    {
      process_hidden_amounts = true;
      bool r = crypto::generate_key_derivation(get_tx_pub_key_from_extra(tx), keys.view_secret_key, derivation);
      if (!r)
        LOG_PRINT_YELLOW("generate_key_derivation failed in get_outs_money_amount", LOG_LEVEL_0);
    }

    for (size_t output_index = 0; output_index < tx.vout.size(); ++output_index)
    {
      const auto& o = tx.vout[output_index];
      VARIANT_SWITCH_BEGIN(o);
      VARIANT_CASE_CONST(tx_out_bare, bo)
        outputs_amount += bo.amount;
      VARIANT_CASE_CONST(tx_out_zarcanum, zo)
        if (process_hidden_amounts)
        {
          uint64_t decoded_amount = 0;
          crypto::public_key decoded_asset_id{};
          crypto::scalar_t amount_blinding_mask{}, asset_id_blinding_mask{};
          if (is_out_to_acc(keys.account_address, zo, derivation, output_index, decoded_amount, decoded_asset_id, amount_blinding_mask, asset_id_blinding_mask))
          {
            if (decoded_asset_id == currency::native_coin_asset_id)
              outputs_amount += decoded_amount;
          }
        }
      VARIANT_SWITCH_END();
    }

    return outputs_amount;
  }
  //---------------------------------------------------------------
  std::string short_hash_str(const crypto::hash& h)
  {
    std::string res = string_tools::pod_to_hex(h);
    CHECK_AND_ASSERT_MES(res.size() == 64, res, "wrong hash256 with string_tools::pod_to_hex conversion");
    auto erased_pos = res.erase(8, 48);
    res.insert(8, "....");
    return res;
  }
  //---------------------------------------------------------------
  // out_key.key =?= Hs(derivation || output_index) * G + addr.spend_public_key
  bool is_out_to_acc(const account_public_address& addr, const txout_to_key& out_key, const crypto::key_derivation& derivation, size_t output_index)
  {
    crypto::public_key pk;
    if (!derive_public_key(derivation, output_index, addr.spend_public_key, pk))
      return false;
    return pk == out_key.key;
  }
  //---------------------------------------------------------------
  bool is_out_to_acc(const account_public_address& addr, const txout_multisig& out_multisig, const crypto::key_derivation& derivation, size_t output_index)
  {
    crypto::public_key pk;
    if (!derive_public_key(derivation, output_index, addr.spend_public_key, pk))
      return false;
    auto it = std::find(out_multisig.keys.begin(), out_multisig.keys.end(), pk);
    if (out_multisig.keys.end() == it)
      return false;
    return true;
  }
  //---------------------------------------------------------------
  bool decode_output_amount_and_asset_id(const tx_out_zarcanum& zo, const crypto::key_derivation& derivation, const size_t output_index, uint64_t& decoded_amount, crypto::public_key& decoded_asset_id,
    crypto::scalar_t& amount_blinding_mask, crypto::scalar_t& asset_id_blinding_mask, crypto::scalar_t* derived_h_ptr /* = nullptr */)
  {
    crypto::scalar_t local_h{};
    if (derived_h_ptr == nullptr)
      derived_h_ptr = &local_h;
    crypto::scalar_t& h = *derived_h_ptr;

    crypto::derivation_to_scalar(derivation, output_index, h.as_secret_key()); // h = Hs(8 * r * V, i)

    crypto::scalar_t amount_mask   = crypto::hash_helper_t::hs(CRYPTO_HDS_OUT_AMOUNT_MASK, h);
    decoded_amount = zo.encrypted_amount ^ amount_mask.m_u64[0];

    amount_blinding_mask = crypto::hash_helper_t::hs(CRYPTO_HDS_OUT_AMOUNT_BLINDING_MASK, h); // f = Hs(domain_sep, h)

    crypto::point_t blinded_asset_id = crypto::point_t(zo.blinded_asset_id).modify_mul8();
    crypto::point_t A_prime = decoded_amount * blinded_asset_id + amount_blinding_mask * crypto::c_point_G; // A' * 8 =? a * T + f * G
    if (A_prime != crypto::point_t(zo.amount_commitment).modify_mul8())
      return false;

    if (blinded_asset_id == currency::native_coin_asset_id_pt)
    {
      asset_id_blinding_mask = 0;
      decoded_asset_id = currency::native_coin_asset_id;
    }
    else
    {
      asset_id_blinding_mask = crypto::hash_helper_t::hs(CRYPTO_HDS_OUT_ASSET_BLINDING_MASK, h); // f = Hs(domain_sep, d, i)
      crypto::point_t asset_id = blinded_asset_id - asset_id_blinding_mask * crypto::c_point_X; // H = T - s * X
      decoded_asset_id = asset_id.to_public_key();
    }

    return true;
  }
  //---------------------------------------------------------------
  bool is_out_to_acc(const account_public_address& addr, const tx_out_zarcanum& zo, const crypto::key_derivation& derivation, size_t output_index, uint64_t& decoded_amount, crypto::public_key& decoded_asset_id,
    crypto::scalar_t& amount_blinding_mask, crypto::scalar_t& asset_id_blinding_mask)
  {
    crypto::scalar_t h{};
    if (!decode_output_amount_and_asset_id(zo, derivation, output_index, decoded_amount, decoded_asset_id, amount_blinding_mask, asset_id_blinding_mask, &h))
      return false;

    crypto::point_t P_prime = h * crypto::c_point_G + crypto::point_t(addr.spend_public_key); // P =? Hs(8rV, i) * G + S
    if (P_prime.to_public_key() != zo.stealth_address)
      return false;

    crypto::point_t Q_prime = crypto::hash_helper_t::hs(CRYPTO_HDS_OUT_CONCEALING_POINT, h) * crypto::point_t(addr.view_public_key).modify_mul8(); // Q' * 8 =? Hs(domain_sep, Hs(8 * r * V, i) ) * 8 * V
    if (Q_prime != crypto::point_t(zo.concealing_point).modify_mul8())
      return false;

    return true;
  } 
  //---------------------------------------------------------------
  bool lookup_acc_outs(const account_keys& acc, const transaction& tx, std::vector<wallet_out_info>& outs, crypto::key_derivation& derivation)
  {
    crypto::public_key tx_pub_key = get_tx_pub_key_from_extra(tx);
    if (null_pkey == tx_pub_key)
      return false;
    return lookup_acc_outs(acc, tx, get_tx_pub_key_from_extra(tx), outs, derivation);
  }
  //---------------------------------------------------------------
  bool check_tx_derivation_hint(const transaction& tx, const crypto::key_derivation& derivation)
  {
    bool found_der_xor = false;
    uint16_t hint = get_derivation_hint(derivation);
    tx_derivation_hint dh = make_tx_derivation_hint_from_uint16(hint);
    for (auto& e : tx.extra)
    {
      if (e.type() == typeid(tx_derivation_hint))
      {
        const tx_derivation_hint& tdh = boost::get<tx_derivation_hint>(e);
        if (tdh.msg.size() == sizeof(uint16_t))
        {
          found_der_xor = true;
          if (dh.msg == tdh.msg)
            return true;
        }
      }
    }
    //if tx doesn't have any hints - feature is not supported, use full scan
    if (!found_der_xor)
      return true;

    return false;
  }
  //---------------------------------------------------------------
  bool lookup_acc_outs_genesis(const account_keys& acc, const transaction& tx, const crypto::public_key& tx_pub_key, std::vector<wallet_out_info>& outs, crypto::key_derivation& derivation)
  {
    uint64_t offset = 0;
    bool r = get_account_genesis_offset_by_address(get_account_address_as_str(acc.account_address), offset);
    if (!r)
      return true;

    CHECK_AND_ASSERT_MES(offset < tx.vout.size(), false, "condition failed: offset(" << offset << ") < tx.vout.size() (" << tx.vout.size() << ")");
    auto& ov = tx.vout[offset];
    CHECK_AND_ASSERT_MES(ov.type() == typeid(tx_out_bare), false, "unexpected type id in lookup_acc_outs_genesis:" << ov.type().name());
    const tx_out_bare& o = boost::get<tx_out_bare>(ov);

    CHECK_AND_ASSERT_MES(o.target.type() == typeid(txout_to_key), false, "condition failed: o.target.type() == typeid(txout_to_key)");
    if (is_out_to_acc(acc.account_address, boost::get<txout_to_key>(o.target), derivation, offset))
    {
      outs.emplace_back(offset, o.amount);
    }
    return true;
  }
  //---------------------------------------------------------------
  bool lookup_acc_outs(const account_keys& acc, const transaction& tx, const crypto::public_key& tx_pub_key, std::vector<wallet_out_info>& outs, crypto::key_derivation& derivation)
  {
    std::list<htlc_info> htlc_info_list;
    return lookup_acc_outs(acc, tx, tx_pub_key, outs, derivation, htlc_info_list);
  }
  //---------------------------------------------------------------
  bool lookup_acc_outs(const account_keys& acc, const transaction& tx, const crypto::public_key& tx_pub_key, std::vector<wallet_out_info>& outs, crypto::key_derivation& derivation, std::list<htlc_info>& htlc_info_list)
  {
    bool r = generate_key_derivation(tx_pub_key, acc.view_secret_key, derivation);
    CHECK_AND_ASSERT_MES(r, false, "unable to generate derivation from tx_pub = " << tx_pub_key << " * view_sec, invalid tx_pub?");

    if (is_coinbase(tx) && get_block_height(tx) == 0 &&  tx_pub_key == ggenesis_tx_pub_key)
    {
      //genesis coinbase
      return lookup_acc_outs_genesis(acc, tx, tx_pub_key, outs, derivation);
    }

    if (!check_tx_derivation_hint(tx, derivation))
      return true;

    size_t output_index = 0;
    for(const auto& ov : tx.vout)
    {
      VARIANT_SWITCH_BEGIN(ov);
      VARIANT_CASE_CONST(tx_out_bare, o)
      {
        VARIANT_SWITCH_BEGIN(o.target);
        VARIANT_CASE_CONST(txout_to_key, t)
          if (is_out_to_acc(acc.account_address, t, derivation, output_index))
          {
            outs.emplace_back(output_index, o.amount);
          }
        VARIANT_CASE_CONST(txout_multisig, t)
          if (is_out_to_acc(acc.account_address, t, derivation, output_index))
          {
            outs.emplace_back(output_index, o.amount); // TODO: @#@# consider this
            //don't cout this money in sum_of_native_outs
          }
        VARIANT_CASE_CONST(txout_htlc, htlc)
          htlc_info hi = AUTO_VAL_INIT(hi);
          if (is_out_to_acc(acc.account_address, htlc.pkey_redeem, derivation, output_index))
          {
            hi.hltc_our_out_is_before_expiration = true;
            htlc_info_list.push_back(hi);
          }
          else if (is_out_to_acc(acc.account_address, htlc.pkey_refund, derivation, output_index))
          {
            hi.hltc_our_out_is_before_expiration = false;
            htlc_info_list.push_back(hi);
          }
          else
          {
            LOG_ERROR("lookup_acc_outs: handling txout_htlc went wrong, output_index: " << output_index);
            return false;
          }
          outs.emplace_back(output_index, o.amount);
        VARIANT_CASE_OTHER()
          LOG_ERROR("Wrong type at lookup_acc_outs, unexpected type is: " << o.target.type().name());
          return false;
        VARIANT_SWITCH_END();
      }
      VARIANT_CASE_CONST(tx_out_zarcanum, zo)
      {
        uint64_t amount = 0;
        crypto::public_key asset_id{};
        crypto::scalar_t amount_blinding_mask = 0, asset_id_blinding_mask = 0;
        if (is_out_to_acc(acc.account_address, zo, derivation, output_index, amount, asset_id, amount_blinding_mask, asset_id_blinding_mask))
        {
          crypto::point_t asset_id_pt = crypto::point_t(zo.blinded_asset_id).modify_mul8() - asset_id_blinding_mask * crypto::c_point_X;
          crypto::public_key asset_id = asset_id_pt.to_public_key();
          outs.emplace_back(output_index, amount, amount_blinding_mask, asset_id_blinding_mask, asset_id);
        }
      }
      VARIANT_SWITCH_END();
      output_index++;
    }
    return true;
  }
  //---------------------------------------------------------------
  bool set_payment_id_to_tx(std::vector<attachment_v>& att, const std::string& payment_id, bool is_in_hardfork4)
  {
    if (!is_payment_id_size_ok(payment_id))
      return false;

    tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
    tsa.service_id = BC_PAYMENT_ID_SERVICE_ID;
    tsa.body = payment_id;
    if (is_in_hardfork4)
    {
      tsa.flags = TX_SERVICE_ATTACHMENT_ENCRYPT_BODY;
    }
    att.push_back(tsa);
    return true;
  }
  //---------------------------------------------------------------
  bool validate_output_key_legit(const crypto::public_key& k)
  {
    if (currency::null_pkey == k)
    {
      return false;
    }
    return true;
  }
  //---------------------------------------------------------------
  std::string print_money_brief(uint64_t amount, size_t decimal_point /* = CURRENCY_DISPLAY_DECIMAL_POINT */)
  {
    uint64_t coin = decimal_point == CURRENCY_DISPLAY_DECIMAL_POINT ? COIN : crypto::constexpr_pow(decimal_point, 10);
    uint64_t remainder = amount % coin;
    amount /= coin;
    if (remainder == 0)
      return std::to_string(amount) + ".0";
    std::string r = std::to_string(remainder);
    if (r.size() < decimal_point)
      r.insert(0, decimal_point - r.size(), '0');
    return std::to_string(amount) + '.' + r.substr(0, r.find_last_not_of('0') + 1);
  }
  //---------------------------------------------------------------
  /*bool get_transaction_hash(const transaction& t, crypto::hash& res, size_t& blob_size)
  {


  return get_object_hash(t, res, blob_size);
  }*/
  //------------------------------------------------------------------
  template<typename pod_operand_a, typename pod_operand_b>
  crypto::hash hash_together(const pod_operand_a& a, const pod_operand_b& b)
  {
    std::string blob;
    string_tools::append_pod_to_strbuff(blob, a);
    string_tools::append_pod_to_strbuff(blob, b);
    return crypto::cn_fast_hash(blob.data(), blob.size());
  }
  //------------------------------------------------------------------
  //--------------------------------------------------------------
  bool unserialize_block_complete_entry(const currency::COMMAND_RPC_GET_BLOCKS_FAST::response& serialized,
    currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response& unserialized)
  {
    for (const auto& bl_entry : serialized.blocks)
    {
      unserialized.blocks.push_back(block_direct_data_entry());
      block_direct_data_entry& bdde = unserialized.blocks.back();
      auto blextin_ptr = std::make_shared<currency::block_extended_info>();
      bool r = currency::parse_and_validate_block_from_blob(bl_entry.block, blextin_ptr->bl);
      bdde.block_ptr = blextin_ptr;
      CHECK_AND_ASSERT_MES(r, false, "failed to parse block from blob: " << string_tools::buff_to_hex_nodelimer(bl_entry.block));
      size_t i = 0;
      if (bl_entry.tx_global_outs.size())
      {
        CHECK_AND_ASSERT_MES(bl_entry.tx_global_outs.size() == bl_entry.txs.size(), false, "tx_global_outs count " << bl_entry.tx_global_outs.size() << " count missmatch with bl_entry.txs count " << bl_entry.txs.size());
      }
      if (bl_entry.coinbase_global_outs.size())
      {
        std::shared_ptr<currency::transaction_chain_entry> tche_ptr(new currency::transaction_chain_entry());
        tche_ptr->m_global_output_indexes = bl_entry.coinbase_global_outs;
        bdde.coinbase_ptr = tche_ptr;
      }
      for (const auto& tx_blob : bl_entry.txs)
      {
        std::shared_ptr<currency::transaction_chain_entry> tche_ptr(new currency::transaction_chain_entry());
        r = parse_and_validate_tx_from_blob(tx_blob, tche_ptr->tx);
        CHECK_AND_ASSERT_MES(r, false, "failed to parse tx from blob: " << string_tools::buff_to_hex_nodelimer(tx_blob));
        bdde.txs_ptr.push_back(tche_ptr);
        if (bl_entry.tx_global_outs.size())
        {
          CHECK_AND_ASSERT_MES(bl_entry.tx_global_outs[i].v.size() == tche_ptr->tx.vout.size(), false, "tx_global_outs for tx" << bl_entry.tx_global_outs[i].v.size() << " count missmatch with tche_ptr->tx.vout.size() count " << tche_ptr->tx.vout.size());
          tche_ptr->m_global_output_indexes = bl_entry.tx_global_outs[i].v;
        }
        i++;
      }
    }
    return true;
  }

  //---------------------------------------------------------------
  uint64_t get_alias_coast_from_fee(const std::string& alias, uint64_t median_fee)
  {
    return median_fee * 10;
  }
  //---------------------------------------------------------------
  // TODO: remove this function after HF4 -- sowle
  uint64_t get_block_timestamp_from_miner_tx_extra(const block& b)
  {
    uint64_t tes_ts = b.timestamp;
    if (is_pos_block(b))
    {
      etc_tx_time t = AUTO_VAL_INIT(t);
      if(get_type_in_variant_container(b.miner_tx.extra, t))
        tes_ts = t.v;
    }
    return tes_ts;
  }
  //---------------------------------------------------------------
  // returns timestamp from BC_BLOCK_DATETIME_SERVICE_ID via tx_service_attachment in extra
  // fallbacks to old-style actual timestamp via etc_tx_time, then to block timestamp
  uint64_t get_block_datetime(const block& b)
  {
    // first try BC_BLOCK_DATETIME_SERVICE_ID
    tx_service_attachment sa = AUTO_VAL_INIT(sa);
    if (get_type_in_variant_container(b.miner_tx.extra, sa))
    {
      if (sa.service_id == BC_BLOCK_DATETIME_SERVICE_ID && sa.instruction == BC_BLOCK_DATETIME_INSTRUCTION_DEFAULT)
      {
        uint64_t ts;
        if (epee::string_tools::get_pod_from_strbuff(sa.body, ts))
          return ts;
      }
    }
    
    // next try etc_tx_time
    etc_tx_time t = AUTO_VAL_INIT(t);
    if (get_type_in_variant_container(b.miner_tx.extra, t))
      return t.v;

    // otherwise return default: block.ts
    return b.timestamp;
  }
  //---------------------------------------------------------------
  void set_block_datetime(uint64_t datetime, block& b)
  {
    tx_service_attachment sa = AUTO_VAL_INIT(sa);
    sa.service_id = BC_BLOCK_DATETIME_SERVICE_ID;
    sa.instruction = BC_BLOCK_DATETIME_INSTRUCTION_DEFAULT;
    sa.flags = 0;
    epee::string_tools::append_pod_to_strbuff(sa.body, datetime);
    b.miner_tx.extra.push_back(sa);
  }
  //------------------------------------------------------------------
  bool validate_alias_name(const std::string& al)
  {
    CHECK_AND_ASSERT_MES(al.size() <= ALIAS_NAME_MAX_LEN, false, "Too long alias name, please use name no longer than " << ALIAS_NAME_MAX_LEN);
    /*allowed symbols "0-9", "a-z", "-", "." */
    static bool alphabet[256] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    for (const auto ch : al)
    {
      CHECK_AND_ASSERT_MES(alphabet[static_cast<unsigned char>(ch)], false, "Wrong character in alias: '" << ch << "'");
    }
    return true;
  }
  //------------------------------------------------------------------
  bool validate_password(const std::string& password)
  {
    static const std::string allowed_password_symbols = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz~!?@#$%^&*_+|{}[]()<>:;\"'-=\\/.,";
    size_t n = password.find_first_not_of(allowed_password_symbols, 0);
    return n == std::string::npos;
  }

  //------------------------------------------------------------------
#define ANTI_OVERFLOW_AMOUNT       1000000
#define GET_PERECENTS_BIG_NUMBERS(per, total) (per/ANTI_OVERFLOW_AMOUNT)*100 / (total/ANTI_OVERFLOW_AMOUNT) 

  void print_reward_change()
  {
    std::cout << std::endl << "Reward change for 10 years:" << std::endl;
    std::cout << std::setw(10) << std::left << "day" << std::setw(19) << "block reward" << std::setw(19) << "generated coins" << std::endl;

    boost::multiprecision::uint128_t already_generated_coins = PREMINE_AMOUNT;
    boost::multiprecision::uint128_t money_was_at_begining_of_year = already_generated_coins;
    boost::multiprecision::uint128_t total_generated_in_year_by_pos = 0;
    boost::multiprecision::uint128_t total_generated_in_year_by_pow = 0;
    //uint64_t total_money_supply = TOTAL_MONEY_SUPPLY;
    uint64_t h = 0;
    for (uint64_t day = 0; day != 365 * 10; ++day)
    {
      if (!(day % 366))
      {
        uint64_t emission_reward = 0;
        get_block_reward(h % 2, 0, 0, already_generated_coins, emission_reward, h);

        std::cout << std::left
          << std::setw(10) << day
          << std::setw(19) << print_money(emission_reward)
          << std::setw(4) << print_money(already_generated_coins) 
          << "(POS: " << boost::lexical_cast<std::string>(GET_PERECENTS_BIG_NUMBERS(total_generated_in_year_by_pos, money_was_at_begining_of_year)) << "%"
          << ", POW: " << boost::lexical_cast<std::string>(GET_PERECENTS_BIG_NUMBERS(total_generated_in_year_by_pow, money_was_at_begining_of_year)) << "%)"

          << std::setw(19) << ",PoS coins/year: " << print_money(total_generated_in_year_by_pos)
          << std::setw(19) << ",PoW coins/year:" << print_money(total_generated_in_year_by_pow)


          << std::endl;


          total_generated_in_year_by_pos = total_generated_in_year_by_pow = 0;
          money_was_at_begining_of_year = already_generated_coins;

      }


      for (size_t i = 0; i != CURRENCY_BLOCKS_PER_DAY; i++)
      {
        uint64_t emission_reward = 0;
        h++;
        get_block_reward(h % 2, 0, 0, already_generated_coins, emission_reward, h);
        already_generated_coins += emission_reward;
        if (h % 2)
          total_generated_in_year_by_pos += emission_reward;
        else
          total_generated_in_year_by_pow += emission_reward;

      }
    }
  }
  void print_reward_change_short()
  {
//     std::cout << std::endl << "Reward change for 20 days:" << std::endl;
//     std::cout << std::setw(10) << std::left << "day" << std::setw(19) << "block reward" << std::setw(19) << "generated coins" << std::endl;
// 
//     const boost::multiprecision::uint128_t& already_generated_coins = PREMINE_AMOUNT;
//     //uint64_t total_money_supply = TOTAL_MONEY_SUPPLY;
//     uint64_t h = 0;
//     for (uint64_t day = 0; day != 20; ++day)
//     {
//       uint64_t emission_reward = 0;
//       get_block_reward(h % 2, 0, 0, already_generated_coins, emission_reward, h, (already_generated_coins - PREMINE_AMOUNT) * 140);
// 
//       std::cout << std::left
//         << std::setw(10) << day
//         << std::setw(19) << print_money(emission_reward)
//         << std::setw(4) << print_money(already_generated_coins)//std::string(std::to_string(GET_PERECENTS_BIG_NUMBERS((already_generated_coins), total_money_supply)) + "%")
//         << std::endl;
// 
// 
// 
//       for (size_t i = 0; i != 720; i++)
//       {
//         h++;
//         get_block_reward(h % 2, 0, 0, already_generated_coins, emission_reward, h, (already_generated_coins - PREMINE_AMOUNT) * 140);
//         already_generated_coins += emission_reward;
//         if (h < POS_START_HEIGHT && i > 360)
//           break;
//       }
//     }
  }
  
  std::string print_reward_change_first_blocks(size_t n_of_first_blocks)
  {
    std::stringstream ss;
    ss << std::endl << "Reward change for the first " << n_of_first_blocks << " blocks:" << std::endl;
    ss << std::setw(10) << std::left << "block #" << std::setw(20) << "block reward" << std::setw(20) << "generated coins" << std::setw(8) << "type" << std::endl;

    boost::multiprecision::uint128_t already_generated_coins = 0;
    uint64_t total_generated_pos = 0;
    uint64_t total_generated_pow = 0;

    for (size_t h = 0; h != n_of_first_blocks; ++h)
    {
      bool is_pos = h < 10 ? false : h % 2 == 0;
      uint64_t emission_reward = 0;

      if (h == 0)
      {
        emission_reward = PREMINE_AMOUNT;
      }
      else
      {
        currency::get_block_reward(is_pos, 0, 0, already_generated_coins, emission_reward, h);
        (is_pos ? total_generated_pos : total_generated_pow) += emission_reward;
      }

      already_generated_coins += emission_reward;

      ss << std::left
        << std::setw(10) << h
        << std::setw(20) << currency::print_money(emission_reward)
        << std::setw(20) << currency::print_money(already_generated_coins)
        << std::setw(8) << (h == 0 ? "genesis" : (is_pos ? "PoS" : "PoW"))
        << std::endl;
    }
    ss << "total generated PoW: " << currency::print_money(total_generated_pow) << std::endl;
    ss << "total generated PoS: " << currency::print_money(total_generated_pos) << std::endl;
    return ss.str();
  }
  //------------------------------------------------------------------
  void print_currency_details()
  {
    //for future forks 

    std::cout << "Currency name: \t\t" << CURRENCY_NAME << "(" << CURRENCY_NAME_SHORT << ")" << std::endl;
    std::cout << "Money supply: \t\t " << CURRENCY_BLOCK_REWARD * CURRENCY_BLOCKS_PER_DAY * 365 << " coins per year" << std::endl;
    std::cout << "PoS block interval: \t" << DIFFICULTY_POS_TARGET << " seconds" << std::endl;
    std::cout << "PoW block interval: \t" << DIFFICULTY_POW_TARGET << " seconds" << std::endl;
    std::cout << "Total blocks per day: \t" << CURRENCY_BLOCKS_PER_DAY << " seconds" << std::endl;
    std::cout << "Default p2p port: \t" << P2P_DEFAULT_PORT << std::endl;
    std::cout << "Default rpc port: \t" << RPC_DEFAULT_PORT << std::endl;
#ifndef TEST_FAST_EMISSION_CURVE
    print_reward_change();
#else
    print_reward_change_short();
#endif
  }
  //------------------------------------------------------------------
  std::string dump_patch(const std::map<uint64_t, crypto::hash>& patch)
  {
    std::stringstream ss;
    for (auto& p : patch)
    {
      ss << "[" << p.first << "]" << p.second << ENDL;
    }
    return ss.str();
  }
  //---------------------------------------------------------------
  bool generate_genesis_block(block& bl)
  {
    //genesis block
    bl = boost::value_initialized<block>();

#ifndef TESTNET
//    std::string genesis_coinbase_tx_hex((const char*)&ggenesis_tx_raw, sizeof(ggenesis_tx_raw));

#else 
    std::string genesis_coinbase_tx_hex = "";
#endif

    //genesis proof phrase: "Liverpool beat Barcelona: Greatest Champions League comebacks of all time"
    //taken from: https://www.bbc.com/sport/football/48163330
    //sha3-256 from proof phrase:  a074236b1354901d5dbc029c0ac4c05c948182c34f3030f32b0c93aee7ba275c (included in genesis block)


    blobdata tx_bl((const char*)&ggenesis_tx_raw, sizeof(ggenesis_tx_raw));
    //string_tools::parse_hexstr_to_binbuff(genesis_coinbase_tx_hex, tx_bl);
    bool r = parse_and_validate_tx_from_blob(tx_bl, bl.miner_tx);
    CHECK_AND_ASSERT_MES(r, false, "failed to parse coinbase tx from hard coded blob");
    bl.major_version = BLOCK_MAJOR_VERSION_GENESIS;
    bl.minor_version = BLOCK_MINOR_VERSION_GENESIS;
    bl.timestamp = 0;
    bl.nonce = CURRENCY_GENESIS_NONCE;
    LOG_PRINT_GREEN("Generated genesis: " << get_block_hash(bl), LOG_LEVEL_0);
    return true;
  }
  //----------------------------------------------------------------------------------------------------
  const crypto::hash& get_genesis_hash(bool need_to_set, const crypto::hash& h)
  {
    static crypto::hash genesis_id = null_hash;
    if (genesis_id == null_hash && !need_to_set)
    {
      LOG_PRINT_GREEN("Calculating genesis....", LOG_LEVEL_0);
      block b = AUTO_VAL_INIT(b);
      bool r = generate_genesis_block(b);
      CHECK_AND_ASSERT_MES(r, null_hash, "failed to generate genesis block");
      genesis_id = get_block_hash(b);
    }

    if (need_to_set)
    {
      genesis_id = h;
    }
    return genesis_id;
  }
  //---------------------------------------------------------------
  std::vector<txout_ref_v> relative_output_offsets_to_absolute(const std::vector<txout_ref_v>& off)
  {
    //if array has both types of outs, then global index (uint64_t) should be first, and then the rest could be out_by_id

    std::vector<txout_ref_v> res = off;
    for (size_t i = 1; i < res.size(); i++)
    {
      if (res[i].type() == typeid(ref_by_id))
        break;
      boost::get<uint64_t>(res[i]) += boost::get<uint64_t>(res[i - 1]);
    }

    return res;
  }
  //---------------------------------------------------------------
  bool absolute_sorted_output_offsets_to_relative_in_place(std::vector<txout_ref_v>& offsets) noexcept
  {
    if (offsets.size() < 2)
      return true;

    size_t i = offsets.size() - 1;
    while (i != 0 && offsets[i].type() == typeid(ref_by_id))
      --i;

    try
    {
      for (; i != 0; i--)
      {
        uint64_t& offset_i   = boost::get<uint64_t>(offsets[i]);
        uint64_t& offset_im1 = boost::get<uint64_t>(offsets[i - 1]);
        if (offset_i <= offset_im1)
          return false; // input was not properly sorted
        offset_i -= offset_im1;
      }
    }
    catch(...)
    {
      return false; // unexpected type in boost::get (all ref_by_id's must be at the end of 'offsets')
    }

    return true;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_block_from_blob(const blobdata& b_blob, block& b)
  {
    return parse_and_validate_object_from_blob(b_blob, b);
  }

  //---------------------------------------------------------------
  bool is_service_tx(const transaction& tx)
  {
    auto tx_type = get_tx_type(tx);
    if (GUI_TX_TYPE_NORMAL == tx_type ||
      GUI_TX_TYPE_COIN_BASE == tx_type)
      return false;

    return true;
  }
  //---------------------------------------------------------------
  bool is_showing_sender_addres(const transaction& tx)
  {
    return have_type_in_variant_container<tx_payer>(tx.attachment) || have_type_in_variant_container<tx_payer_old>(tx.attachment);
  }
  //---------------------------------------------------------------
  bool does_tx_have_only_mixin_inputs(const transaction& tx)
  {
    for (const auto& e : tx.vin)
    {
      if (e.type() != typeid(txin_to_key) || e.type() != typeid(txin_multisig) || e.type() != typeid(txin_htlc))
        return false;
      if (boost::get<txin_to_key>(e).key_offsets.size() < 2)
        return false;
    }

    return true;
  }
  //---------------------------------------------------------------
  bool check_native_coins_amount_burnt_in_outs(const transaction& tx, const uint64_t amount, uint64_t* p_amount_burnt /* = nullptr */)
  {
    if (tx.version <= TRANSACTION_VERSION_PRE_HF4)
    {
      uint64_t sum_of_bare_outs_burnt = 0;
      for (const auto& out : tx.vout)
      {
        VARIANT_SWITCH_BEGIN(out);
        VARIANT_CASE_CONST(tx_out_bare, out)
          if (out.target.type() != typeid(txout_to_key))
            continue;
        const txout_to_key& o = boost::get<txout_to_key>(out.target);
        if (o.key == null_pkey)
          sum_of_bare_outs_burnt += out.amount;
        VARIANT_SWITCH_END();
      }
      if (p_amount_burnt)
        *p_amount_burnt = sum_of_bare_outs_burnt;
      return sum_of_bare_outs_burnt >= amount;
    }

    // post HF-4 txs
    // assuming: zero out pubkey, explicit asset_id, zero amount blinding mask
    crypto::point_t sum_of_amount_commitments = crypto::c_point_0;
    for (const auto& out : tx.vout)
    {
      VARIANT_SWITCH_BEGIN(out);
      VARIANT_CASE_CONST(tx_out_zarcanum, out_zc)
        if (out_zc.stealth_address == null_pkey && out_zc.blinded_asset_id == native_coin_asset_id_1div8)
          sum_of_amount_commitments += crypto::point_t(out_zc.amount_commitment).modify_mul8();
      VARIANT_SWITCH_END();
    }
    return sum_of_amount_commitments == amount * native_coin_asset_id_pt;
  }
  //---------------------------------------------------------------
  bool get_aliases_reward_account(account_public_address& acc)
  {
    bool r = string_tools::parse_tpod_from_hex_string(ALIAS_REWARDS_ACCOUNT_SPEND_PUB_KEY, acc.spend_public_key);
    r &= string_tools::parse_tpod_from_hex_string(ALIAS_REWARDS_ACCOUNT_VIEW_PUB_KEY, acc.view_public_key);
    return r;
  }
  //------------------------------------------------------------------
  size_t get_service_attachments_count_in_tx(const transaction& tx)
  {
    size_t cnt = 0;
    for (const auto& at : tx.attachment)
    {
      if (at.type() == typeid(tx_service_attachment))
        ++cnt;
    }
    return cnt;
  }


  //------------------------------------------------------------------
  bool fill_tx_rpc_outputs(tx_rpc_extended_info& tei, const transaction& tx, const transaction_chain_entry* ptce)
  {
    size_t i = 0;
    for (auto& out : tx.vout)
    {
      tei.outs.push_back(tx_out_rpc_entry());
      tei.outs.back().is_spent = ptce ? ptce->m_spent_flags[i] : false;
      tei.outs.back().global_index = ptce ? ptce->m_global_output_indexes[i] : 0;
      VARIANT_SWITCH_BEGIN(out);
      VARIANT_CASE_CONST(tx_out_bare, out)
      {
        tei.outs.back().amount = out.amount;

        VARIANT_SWITCH_BEGIN(out.target);
        VARIANT_CASE_CONST(txout_to_key, otk)
          tei.outs.back().pub_keys.push_back(epee::string_tools::pod_to_hex(otk.key));
          if (otk.mix_attr == CURRENCY_TO_KEY_OUT_FORCED_NO_MIX)
            tei.outs.back().pub_keys.back() += "(FORCED_NO_MIX)";
          if (otk.mix_attr >= CURRENCY_TO_KEY_OUT_FORCED_MIX_LOWER_BOUND)
            tei.outs.back().pub_keys.back() += std::string("(FORCED_MIX_LOWER_BOUND: ") + std::to_string(otk.mix_attr) + ")";
        VARIANT_CASE_CONST(txout_multisig, otm)
          for (auto& k : otm.keys)
          {
            tei.outs.back().pub_keys.push_back(epee::string_tools::pod_to_hex(k));
          }
          tei.outs.back().minimum_sigs = otm.minimum_sigs;
        VARIANT_CASE_CONST(txout_htlc, otk)
          tei.outs.back().pub_keys.push_back(epee::string_tools::pod_to_hex(otk.pkey_redeem) + "(htlc_pkey_redeem)");
          tei.outs.back().pub_keys.push_back(epee::string_tools::pod_to_hex(otk.pkey_refund) + "(htlc_pkey_refund)");
        VARIANT_SWITCH_END();
      }
      VARIANT_CASE_CONST(tx_out_zarcanum, o)
        tei.outs.back().pub_keys.push_back(epee::string_tools::pod_to_hex(o.stealth_address));
        tei.outs.back().pub_keys.push_back(epee::string_tools::pod_to_hex(o.concealing_point));
        tei.outs.back().pub_keys.push_back(epee::string_tools::pod_to_hex(o.amount_commitment));
        tei.outs.back().pub_keys.push_back(epee::string_tools::pod_to_hex(o.blinded_asset_id));
        tei.outs.back().pub_keys.push_back(epee::string_tools::pod_to_hex(o.encrypted_amount));
      VARIANT_SWITCH_END();
      ++i;
    }
    return true;
  }
  //------------------------------------------------------------------
  bool fill_tx_rpc_inputs(tx_rpc_extended_info& tei, const transaction& tx)
  {
    //handle inputs
    for (auto in : tx.vin)
    {
      tei.ins.push_back(tx_in_rpc_entry());
      tx_in_rpc_entry& entry_to_fill = tei.ins.back();
      if (in.type() == typeid(txin_gen))
      {
        entry_to_fill.amount = 0;
      }
      else if (in.type() == typeid(txin_to_key) || in.type() == typeid(txin_htlc) || in.type() == typeid(txin_zc_input))
      {
        //TODO: add htlc info
        entry_to_fill.amount = get_amount_from_variant(in);
        entry_to_fill.kimage_or_ms_id = epee::string_tools::pod_to_hex(get_key_image_from_txin_v(in));
        const std::vector<txout_ref_v>& key_offsets = get_key_offsets_from_txin_v(in);
        std::vector<txout_ref_v> absolute_offsets = relative_output_offsets_to_absolute(key_offsets);
        for (auto& ao : absolute_offsets)
        {
          entry_to_fill.global_indexes.push_back(0);
          if (ao.type() == typeid(uint64_t))
          {
            entry_to_fill.global_indexes.back() = boost::get<uint64_t>(ao);
          }
          else// if (ao.type() == typeid(ref_by_id))
          {
            //disable for the reset at the moment 
            entry_to_fill.global_indexes.back() = std::numeric_limits<uint64_t>::max();
          }
        }
        if (in.type() == typeid(txin_htlc))
        {
          entry_to_fill.htlc_origin = epee::string_tools::buff_to_hex_nodelimer(boost::get<txin_htlc>(in).hltc_origin);
        }
        //tk.etc_details -> visualize it may be later
      }
      else if (in.type() == typeid(txin_multisig))
      {
        txin_multisig& tms = boost::get<txin_multisig>(in);
        entry_to_fill.amount = tms.amount;
        entry_to_fill.kimage_or_ms_id = epee::string_tools::pod_to_hex(tms.multisig_out_id);
        if (tx.signatures.size() >= tei.ins.size() &&
          tx.signatures[tei.ins.size() - 1].type() == typeid(NLSAG_sig))
        {
          entry_to_fill.multisig_count = boost::get<NLSAG_sig>(tx.signatures[tei.ins.size() - 1]).s.size();
        }
          
      }
    }
    return true;
  }
  bool fill_block_rpc_details(block_rpc_extended_info& pei_rpc, const block_extended_info& bei_chain, const crypto::hash& h)
  {
    pei_rpc.difficulty = bei_chain.difficulty.convert_to<std::string>();
    pei_rpc.cumulative_diff_adjusted = bei_chain.cumulative_diff_adjusted.convert_to<std::string>();
    pei_rpc.cumulative_diff_precise = bei_chain.cumulative_diff_precise.convert_to<std::string>();
    pei_rpc.block_cumulative_size = bei_chain.block_cumulative_size;
    pei_rpc.timestamp = bei_chain.bl.timestamp;
    pei_rpc.id = epee::string_tools::pod_to_hex(h);
    pei_rpc.prev_id = epee::string_tools::pod_to_hex(bei_chain.bl.prev_id);
    pei_rpc.actual_timestamp = get_block_datetime(bei_chain.bl);
    pei_rpc.type = is_pos_block(bei_chain.bl) ? 0 : 1;
    pei_rpc.already_generated_coins = boost::lexical_cast<std::string>(bei_chain.already_generated_coins);
    pei_rpc.this_block_fee_median = bei_chain.this_block_tx_fee_median;
    pei_rpc.effective_fee_median = bei_chain.effective_tx_fee_median;
    pei_rpc.height = bei_chain.height;
    pei_rpc.object_in_json = currency::obj_to_json_str(bei_chain.bl);

    extra_user_data eud = AUTO_VAL_INIT(eud);
    if (get_type_in_variant_container(bei_chain.bl.miner_tx.extra, eud))
    {
      pei_rpc.miner_text_info = eud.buff;
    }

    pei_rpc.base_reward = get_base_block_reward(bei_chain.height);
    pei_rpc.summary_reward = get_reward_from_miner_tx(bei_chain.bl.miner_tx);
    pei_rpc.penalty = (pei_rpc.base_reward + pei_rpc.total_fee) - pei_rpc.summary_reward;
    return true;
  }

  //------------------------------------------------------------------
  void append_per_block_increments_for_tx(const transaction& tx, std::unordered_map<uint64_t, uint32_t>& gindices)
  {
    for (size_t n = 0; n < tx.vout.size(); ++n)
    {
      VARIANT_SWITCH_BEGIN(tx.vout[n]);
      VARIANT_CASE_CONST(tx_out_bare, o)
        if (o.target.type() == typeid(txout_to_key) || o.target.type() == typeid(txout_htlc))
        {
          uint64_t amount = o.amount;
          gindices[amount] += 1;
        }
      VARIANT_CASE_CONST(tx_out_zarcanum, o)
        gindices[0] += 1;    
      VARIANT_SWITCH_END();
    }
  }
  //---------------------------------------------------------------
  bool get_aliases_reward_account(account_public_address& acc, crypto::secret_key& acc_view_key)
  {
    bool r = get_aliases_reward_account(acc);
    r &= string_tools::parse_tpod_from_hex_string(ALIAS_REWARDS_ACCOUNT_VIEW_SEC_KEY, acc_view_key);
    return r;
  }
  //---------------------------------------------------------------
  bool is_pos_block(const block& b)
  {
    if (!(b.flags & CURRENCY_BLOCK_FLAG_POS_BLOCK))
      return false;
    return is_pos_miner_tx(b.miner_tx);
  }
  //---------------------------------------------------------------
  bool is_pos_miner_tx(const transaction& tx)
  {
    if (tx.vin.size() == 2 &&
      tx.vin[0].type() == typeid(txin_gen) &&
      (tx.vin[1].type() == typeid(txin_to_key) ||
       tx.vin[1].type() == typeid(txin_zc_input)))
      return true;
    return false;
  }
  //---------------------------------------------------------------
  size_t get_max_block_size()
  {
    return CURRENCY_MAX_BLOCK_SIZE;
  }
  //-----------------------------------------------------------------------------------------------
  uint64_t get_base_block_reward(uint64_t height)
  {
    if (!height)
      return PREMINE_AMOUNT;
  
    return CURRENCY_BLOCK_REWARD;
  }
  //-----------------------------------------------------------------------------------------------
  // Modern version, requires only necessary arguments. Returns 0 if block is too big (current_block_size > 2 * median_block_size)
  uint64_t get_block_reward(uint64_t height, size_t median_block_size, size_t current_block_size)
  {
    uint64_t reward = 0;
    get_block_reward(/* is_pos - doesn't matter */ false, median_block_size, current_block_size, /* boost::multiprecision::uint128_t -- doesn't matter*/ boost::multiprecision::uint128_t(0), reward, height);
    return reward;
  }
  //-----------------------------------------------------------------------------------------------
  // legacy version, some arguments are unnecessary now
  bool get_block_reward(bool is_pos, size_t median_size, size_t current_block_size, const boost::multiprecision::uint128_t& already_generated_coins, uint64_t &reward, uint64_t height)
  {
    uint64_t base_reward = get_base_block_reward(height);

    //make it soft
    if (median_size < CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE)
    {
      median_size = CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE;
    }


    if (current_block_size <= median_size || height == 0)
    {
      reward = base_reward;
      return true;
    }

    if (current_block_size > 2 * median_size)
    {
      LOG_PRINT_L4("Block cumulative size is too big: " << current_block_size << ", expected less than " << 2 * median_size);
      return false;
    }

    uint64_t product_hi;
    // BUGFIX(taken from Monero): 32-bit saturation bug (e.g. ARM7), the result was being
    // treated as 32-bit by default.
    uint64_t multiplicand = 2 * median_size - current_block_size;
    multiplicand *= current_block_size;

    uint64_t product_lo = mul128(base_reward, multiplicand, &product_hi);

    uint64_t reward_hi;
    uint64_t reward_lo;
    div128_32(product_hi, product_lo, static_cast<uint32_t>(median_size), &reward_hi, &reward_lo);
    div128_32(reward_hi, reward_lo, static_cast<uint32_t>(median_size), &reward_hi, &reward_lo);
    CHECK_AND_ASSERT_MES(0 == reward_hi, false, "0 == reward_hi");
    CHECK_AND_ASSERT_MES(reward_lo < base_reward, false, "reward_lo < base_reward, reward_lo: " << reward_lo << ", base_reward: " << base_reward << ", current_block_size: " << current_block_size << ", median_size: " << median_size);

    reward = reward_lo;
    return true;
  }

  //-----------------------------------------------------------------------
  bool is_coinbase(const transaction& tx)
  {
    if (!tx.vin.size() || tx.vin.size() > 2)
      return false;

    if (tx.vin[0].type() != typeid(txin_gen))
      return false;

    return true;
  }
  //-----------------------------------------------------------------------
  bool is_pos_coinbase(const transaction& tx)
  {
    return is_pos_miner_tx(tx);
  }
  //-----------------------------------------------------------------------
  bool is_coinbase(const transaction& tx, bool& pos_coinbase)
  {
    if (!is_coinbase(tx))
      return false;

    pos_coinbase = is_pos_coinbase(tx);
    return true;
  }
  //-----------------------------------------------------------------------
  bool is_payment_id_size_ok(const payment_id_t& payment_id)
  {
    return payment_id.size() <= BC_PAYMENT_ID_SERVICE_SIZE_MAX;
  }
  //-----------------------------------------------------------------------
  std::string get_account_address_as_str(const account_public_address& addr)
  {
    if (addr.flags == 0)
      return tools::base58::encode_addr(CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX, t_serializable_object_to_blob(addr.to_old())); // classic Zano address

    if (addr.flags & ACCOUNT_PUBLIC_ADDRESS_FLAG_AUDITABLE)
      return tools::base58::encode_addr(CURRENCY_PUBLIC_AUDITABLE_ADDRESS_BASE58_PREFIX, t_serializable_object_to_blob(addr)); // new format Zano address (auditable)
    
    return tools::base58::encode_addr(CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX, t_serializable_object_to_blob(addr)); // new format Zano address (normal)
  }
  //-----------------------------------------------------------------------
  bool is_address_like_wrapped(const std::string& addr)
  {
    if (addr.length() == 42 && addr.substr(0, 2) == "0x")
      return true;
    else return false;
  }
  //-----------------------------------------------------------------------
  std::string get_account_address_and_payment_id_as_str(const account_public_address& addr, const payment_id_t& payment_id)
  {
    if (addr.flags == 0)
      return tools::base58::encode_addr(CURRENCY_PUBLIC_INTEG_ADDRESS_BASE58_PREFIX, t_serializable_object_to_blob(addr.to_old()) + payment_id); // classic integrated Zano address

    if (addr.flags & ACCOUNT_PUBLIC_ADDRESS_FLAG_AUDITABLE)
      return tools::base58::encode_addr(CURRENCY_PUBLIC_AUDITABLE_INTEG_ADDRESS_BASE58_PREFIX, t_serializable_object_to_blob(addr) + payment_id); // new format integrated Zano address (auditable)
    
    return tools::base58::encode_addr(CURRENCY_PUBLIC_INTEG_ADDRESS_V2_BASE58_PREFIX, t_serializable_object_to_blob(addr) + payment_id); // new format integrated Zano address (normal)
  }
  //-----------------------------------------------------------------------
  bool get_account_address_from_str(account_public_address& addr, const std::string& str)
  {
    std::string integrated_payment_id; // won't be used
    return get_account_address_and_payment_id_from_str(addr, integrated_payment_id, str);
  }
  //-----------------------------------------------------------------------
  bool get_account_address_and_payment_id_from_str(account_public_address& addr, payment_id_t& payment_id, const std::string& str)
  {
    payment_id.clear();
    blobdata blob;
    uint64_t prefix;
    if (!tools::base58::decode_addr(str, prefix, blob))
    {
      LOG_PRINT_L1("Invalid address format: base58 decoding failed for \"" << str << "\"");
      return false;
    }

    if (blob.size() < sizeof(account_public_address_old))
    {
      LOG_PRINT_L1("Address " << str << " has invalid format: blob size is " << blob.size() << " which is less, than expected " << sizeof(account_public_address_old));
      return false;
    }

    if (blob.size() > sizeof(account_public_address) + BC_PAYMENT_ID_SERVICE_SIZE_MAX)
    {
      LOG_PRINT_L1("Address " << str << " has invalid format: blob size is " << blob.size() << " which is more, than allowed " << sizeof(account_public_address) + BC_PAYMENT_ID_SERVICE_SIZE_MAX);
      return false;
    }

    bool parse_as_old_format = false;

    if (prefix == CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX)
    {
      // normal address
      if (blob.size() == sizeof(account_public_address_old))
      {
        parse_as_old_format = true;
      }
      else if (blob.size() == sizeof(account_public_address))
      {
        parse_as_old_format = false;
      }
      else
      {
        LOG_PRINT_L1("Account public address cannot be parsed from \"" << str << "\", incorrect size");
        return false;
      }
    }
    else if (prefix == CURRENCY_PUBLIC_AUDITABLE_ADDRESS_BASE58_PREFIX)
    {
      // auditable, parse as new format
        parse_as_old_format = false;
    }
    else if (prefix == CURRENCY_PUBLIC_INTEG_ADDRESS_BASE58_PREFIX)
    {
      payment_id = blob.substr(sizeof(account_public_address_old));
      blob = blob.substr(0, sizeof(account_public_address_old));
      parse_as_old_format = true;
    }
    else if (prefix == CURRENCY_PUBLIC_AUDITABLE_INTEG_ADDRESS_BASE58_PREFIX || prefix == CURRENCY_PUBLIC_INTEG_ADDRESS_V2_BASE58_PREFIX)
    {
      payment_id = blob.substr(sizeof(account_public_address));
      blob = blob.substr(0, sizeof(account_public_address));
      parse_as_old_format = false;
    }
    else
    {
      LOG_PRINT_L1("Address " << str << " has wrong prefix " << prefix);
      return false;
    }

    if (parse_as_old_format)
    {
      account_public_address_old addr_old = AUTO_VAL_INIT(addr_old);
      if (!::serialization::parse_binary(blob, addr_old))
      {
        LOG_PRINT_L1("Account public address (old) cannot be parsed from \"" << str << "\"");
        return false;
      }
      addr = account_public_address::from_old(addr_old);
    }
    else
    {
      if (!::serialization::parse_binary(blob, addr))
      {
        LOG_PRINT_L1("Account public address cannot be parsed from \"" << str << "\"");
        return false;
      }
    }

    if (payment_id.size() > BC_PAYMENT_ID_SERVICE_SIZE_MAX)
    {
      LOG_PRINT_L1("Failed to parse address from \"" << str << "\": payment id size exceeded: " << payment_id.size());
      return false;
    }

    if (!crypto::check_key(addr.spend_public_key) || !crypto::check_key(addr.view_public_key))
    {
      LOG_PRINT_L1("Failed to validate address keys for public address \"" << str << "\"");
      return false;
    }

    return true;
  }
  //---------------------------------------------------------------
  bool parse_payment_id_from_hex_str(const std::string& payment_id_str, payment_id_t& payment_id)
  {
    return epee::string_tools::parse_hexstr_to_binbuff(payment_id_str, payment_id);
  }
  //--------------------------------------------------------------------------------
  crypto::hash prepare_prefix_hash_for_sign(const transaction& tx, uint64_t in_index, const crypto::hash& tx_id)
  {
    CHECK_AND_ASSERT_MES(tx.vin.size() > in_index, null_hash, "condition failed: tx.vin.size(" << tx.vin.size() << ") > in_index( " << in_index << " )");

    crypto::hash tx_hash_for_signature = tx_id;
    if (get_tx_flags(tx) & TX_FLAG_SIGNATURE_MODE_SEPARATE)
    {
      // separately signed transaction case - figure out all the outputs, extra entries and attachments the given input refers to
      size_t signed_outputs_count = 0;
      size_t signed_extras_count = 0;
      size_t signed_att_count = 0;

      // extra_attachment_info can be stored either in the tx extra (default) and/or in the input's etc_details (it overrides the default one)
      extra_attachment_info eai = AUTO_VAL_INIT(eai);
      if (get_type_in_variant_container(tx.extra, eai))
        signed_att_count = static_cast<size_t>(eai.cnt); // get signed attachments count from default extra_attachment_info (if exists)

      for (size_t i = 0; i != in_index + 1; i++)
      {
        // get signed outputs and extra entries count from input's details (each input has to have it)
        signed_parts so = AUTO_VAL_INIT(so);
        bool r = get_type_in_variant_container(get_txin_etc_options(tx.vin[i]), so);
        CHECK_AND_ASSERT_MES(r, null_hash, "Invalid input #" << i << " (of " << tx.vin.size() << ") contains no signed_parts");
        CHECK_AND_ASSERT_MES(signed_outputs_count <= so.n_outs, null_hash, "Invalid input #" << i << " (of " << tx.vin.size() << "): Next signed_outputs_count is less then prev");
        CHECK_AND_ASSERT_MES(signed_extras_count <= so.n_extras, null_hash, "Invalid input #" << i << " (of " << tx.vin.size() << "): Next signed_extras_count is less then prev");
        signed_outputs_count = so.n_outs;
        signed_extras_count = so.n_extras;

        // get signed attachments count from input's extra_attachment_info (it's optional, if exists - override default)
        if (get_type_in_variant_container(get_txin_etc_options(tx.vin[i]), eai))
        {
          CHECK_AND_ASSERT_MES(signed_att_count <= eai.cnt, null_hash, "Invalid input #" << i << " (of " << tx.vin.size() << "): Next signed_att_count is less then prev");
          signed_att_count = static_cast<size_t>(eai.cnt);
        }
      }

      if (in_index == tx.vin.size() - 1)
      {
        // for the last input make sure all outs, extra entries and attachments was counted correctly
        CHECK_AND_ASSERT_MES(signed_outputs_count == tx.vout.size(),   null_hash, "Separately signed complete tx has to mention all the outputs in its inputs: signed_outputs_count=" << signed_outputs_count << " tx.vout.size()=" << tx.vout.size());
        CHECK_AND_ASSERT_MES(signed_extras_count == tx.extra.size(),   null_hash, "Separately signed complete tx has to mention all the extra entries in its inputs: signed_extras_count=" << signed_extras_count << " tx.extra.size()=" << tx.extra.size());
        CHECK_AND_ASSERT_MES(signed_att_count == tx.attachment.size(), null_hash, "Separately signed complete tx has to mention all the attachments in its inputs: signed_att_count=" << signed_att_count << " tx.attachment.size()=" << tx.attachment.size());
        // okay, there's nothing to crop - tx_hash_for_signature is tx prefix hash
      }
      else
      {
        // the given input isn't the last one - we have to crop some data in order to calculate correct hash for signing
        transaction tx_local = tx;

        // crop all inputs past the given one
        tx_local.vin.resize(static_cast<size_t>(in_index) + 1);

        // crop outs
        CHECK_AND_ASSERT_MES(signed_outputs_count <= tx_local.vout.size(), null_hash, "signed_outputs_count(" << signed_outputs_count << " ) more than tx_local.vout.size()" << tx_local.vout.size());
        tx_local.vout.resize(signed_outputs_count);

        // crop extra
        CHECK_AND_ASSERT_MES(signed_extras_count <= tx_local.extra.size(), null_hash, "signed_extras_count(" << signed_extras_count << " ) more than tx_local.extra.size()" << tx_local.vout.size());
        tx_local.extra.resize(signed_extras_count);

        // crop attachments
        CHECK_AND_ASSERT_MES(signed_att_count <= tx_local.attachment.size(), null_hash, "signed_att_count(" << signed_att_count << " ) more than tx_local.attachment.size()" << tx_local.attachment.size());
        tx_local.attachment.resize(signed_att_count);

        // calculate hash of cropped tx as the result
        tx_hash_for_signature = get_transaction_hash(tx_local);
      }
    }

    return tx_hash_for_signature;
  }
  //------------------------------------------------------------------
  const char* get_asset_operation_type_string(size_t asset_operation_type, bool short_name /* = false */)
  {
    switch(asset_operation_type)
    {
    case ASSET_DESCRIPTOR_OPERATION_UNDEFINED:
      return short_name ? "undefined" : "ASSET_DESCRIPTOR_OPERATION_UNDEFINED";
    case ASSET_DESCRIPTOR_OPERATION_REGISTER:
      return short_name ? "register"  : "ASSET_DESCRIPTOR_OPERATION_REGISTER";
    case ASSET_DESCRIPTOR_OPERATION_UPDATE:
      return short_name ? "update"    : "ASSET_DESCRIPTOR_OPERATION_UPDATE";
    case ASSET_DESCRIPTOR_OPERATION_EMIT:
      return short_name ? "emit"      : "ASSET_DESCRIPTOR_OPERATION_EMIT";
    case ASSET_DESCRIPTOR_OPERATION_PUBLIC_BURN:
      return short_name ? "burn"      : "ASSET_DESCRIPTOR_OPERATION_PUBLIC_BURN";
    default:
      return "unknown";
    }
  }
  //------------------------------------------------------------------
  std::string dump_ring_sig_data(const crypto::hash& hash_for_sig, const crypto::key_image& k_image, const std::vector<const crypto::public_key*>& output_keys_ptrs, const std::vector<crypto::signature>& sig)
  {
    std::stringstream s;
    s << "  hash for sig: " << hash_for_sig << ENDL
      << "  k_image:      " << k_image << ENDL
      << "  out keys (" << output_keys_ptrs.size() << ")" << ENDL;
    size_t i = 0;
    for (auto& p_out_k : output_keys_ptrs)
      s << "    " << std::setw(2) << i++ << " " << *p_out_k << ENDL;
  
    s << "  signatures (" << sig.size() << ")" << ENDL;
    i = 0;
    for (auto& sig_el : sig)
      s << "    " << std::setw(2) << i++ << " " << sig_el << ENDL;
  
    return s.str();
  }


  //--------------------------------------------------------------------------------
  std::ostream& operator <<(std::ostream& o, const ref_by_id& r)
  {
    return o << "<" << r.n << ":" << r.tx_id << ">";
  }
  //--------------------------------------------------------------------------------
#ifndef MOBILE_WALLET_BUILD
  const std::locale& utf8_get_conversion_locale()
  {
    static std::locale loc = boost::locale::generator().generate("en_US.UTF-8");
    return loc;
  }
  std::string utf8_to_upper(const std::string& s)
  {
    return boost::locale::to_upper(s, utf8_get_conversion_locale());
  }
  std::string utf8_to_lower(const std::string& s)
  {
    return boost::locale::to_lower(s, utf8_get_conversion_locale());
  }
  bool utf8_substring_test_case_insensitive(const std::string& match, const std::string& s)
  {
    if (match.empty())
      return true;
    return utf8_to_lower(s).find(utf8_to_lower(match), 0) != std::string::npos;
  }
#endif
  //--------------------------------------------------------------------------------
  bool operator ==(const currency::transaction& a, const currency::transaction& b) {
    return currency::get_transaction_hash(a) == currency::get_transaction_hash(b);
  }
  //--------------------------------------------------------------------------------
  bool operator ==(const currency::block& a, const currency::block& b) {
    return currency::get_block_hash(a) == currency::get_block_hash(b);
  }
  //--------------------------------------------------------------------------------
  bool operator ==(const currency::extra_attachment_info& a, const currency::extra_attachment_info& b)
  {
    if (a.cnt == b.cnt && a.hsh == b.hsh && a.sz == b.sz)
      return true;
    else 
      return false;
  }
  //--------------------------------------------------------------------------------
  bool operator ==(const currency::NLSAG_sig& a, const currency::NLSAG_sig& b)
  {
    return a.s == b.s;
  }
  //--------------------------------------------------------------------------------
  bool operator ==(const currency::void_sig& a, const currency::void_sig& b)
  {
    //@#@
    ASSERT_MES_AND_THROW("not implemented yet");
    return false;
  }
  //--------------------------------------------------------------------------------
  bool operator ==(const currency::ZC_sig& a, const currency::ZC_sig& b)
  {
    //@#@ TODO
    ASSERT_MES_AND_THROW("not implemented yet");
    return false;
  }
  //--------------------------------------------------------------------------------
  bool operator ==(const currency::zarcanum_sig& a, const currency::zarcanum_sig& b)
  {
    //@#@ TODO
    ASSERT_MES_AND_THROW("not implemented yet");
    return false;
  }
  //--------------------------------------------------------------------------------
  bool operator ==(const currency::ref_by_id& a, const currency::ref_by_id& b)
  {
    return a.n == b.n && a.tx_id == b.tx_id;
  }
  //--------------------------------------------------------------------------------
  bool operator ==(const currency::signed_parts& a, const currency::signed_parts& b)
  {
    return
      a.n_extras  == b.n_extras &&
      a.n_outs    == b.n_outs;
  }
  bool operator ==(const currency::txin_gen& a, const currency::txin_gen& b)
  {
    return a.height == b.height;
  }
  bool operator ==(const currency::txin_to_key& a, const currency::txin_to_key& b)
  {
    return
      a.amount      == b.amount &&
      a.etc_details == b.etc_details &&
      a.key_offsets == b.key_offsets &&
      a.k_image     == b.k_image;
  }
  bool operator ==(const currency::txin_multisig& a, const currency::txin_multisig& b)
  {
    return
      a.amount          == b.amount &&
      a.etc_details     == b.etc_details &&
      a.multisig_out_id == b.multisig_out_id &&
      a.sigs_count      == b.sigs_count;
  }
  bool operator ==(const currency::txin_htlc& a, const currency::txin_htlc& b)
  {
    return
      a.amount      == b.amount &&
      a.etc_details == b.etc_details &&
      a.hltc_origin == b.hltc_origin &&
      a.key_offsets == b.key_offsets &&
      a.k_image     == b.k_image;
  }
  bool operator ==(const currency::txin_zc_input& a, const currency::txin_zc_input& b)
  {
    return
      a.etc_details == b.etc_details &&
      a.key_offsets == b.key_offsets &&
      a.k_image     == b.k_image;
  }
  //--------------------------------------------------------------------------------
  boost::multiprecision::uint1024_t get_a_to_b_relative_cumulative_difficulty(const wide_difficulty_type& difficulty_pos_at_split_point,
    const wide_difficulty_type& difficulty_pow_at_split_point,
    const difficulties& a_diff,
    const difficulties& b_diff )
  {
    static const wide_difficulty_type difficulty_pos_starter = DIFFICULTY_POS_STARTER;
    static const wide_difficulty_type difficulty_pow_starter = DIFFICULTY_POW_STARTER;
    const wide_difficulty_type& a_pos_cumulative_difficulty = a_diff.pos_diff > 0 ? a_diff.pos_diff : difficulty_pos_starter;
    const wide_difficulty_type& b_pos_cumulative_difficulty = b_diff.pos_diff > 0 ? b_diff.pos_diff : difficulty_pos_starter;
    const wide_difficulty_type& a_pow_cumulative_difficulty = a_diff.pow_diff > 0 ? a_diff.pow_diff : difficulty_pow_starter;
    const wide_difficulty_type& b_pow_cumulative_difficulty = b_diff.pow_diff > 0 ? b_diff.pow_diff : difficulty_pow_starter;

    boost::multiprecision::uint1024_t basic_sum = boost::multiprecision::uint1024_t(a_pow_cumulative_difficulty) + (boost::multiprecision::uint1024_t(a_pos_cumulative_difficulty)*difficulty_pow_at_split_point) / difficulty_pos_at_split_point;
    boost::multiprecision::uint1024_t res =
      (basic_sum * a_pow_cumulative_difficulty * a_pos_cumulative_difficulty) / (boost::multiprecision::uint1024_t(b_pow_cumulative_difficulty)*b_pos_cumulative_difficulty);

//     if (res > boost::math::tools::max_value<wide_difficulty_type>())
//     {
//       ASSERT_MES_AND_THROW("[INTERNAL ERROR]: Failed to get_a_to_b_relative_cumulative_difficulty, res = " << res << ENDL
//         << ", difficulty_pos_at_split_point: " << difficulty_pos_at_split_point << ENDL
//         << ", difficulty_pow_at_split_point:" << difficulty_pow_at_split_point << ENDL
//         << ", a_pos_cumulative_difficulty:" << a_pos_cumulative_difficulty << ENDL
//         << ", b_pos_cumulative_difficulty:" << b_pos_cumulative_difficulty << ENDL
//         << ", a_pow_cumulative_difficulty:" << a_pow_cumulative_difficulty << ENDL
//         << ", b_pow_cumulative_difficulty:" << b_pow_cumulative_difficulty << ENDL       
//       );
//     }
    TRY_ENTRY();
//    wide_difficulty_type short_res = res.convert_to<wide_difficulty_type>();
    return res;
    CATCH_ENTRY_WITH_FORWARDING_EXCEPTION();
  }
  //--------------------------------------------------------------------------------
  // Note:  we adjust formula and introduce multiplier, 
  //        that let us never dive into floating point calculations (which we can't use in consensus)
  //        this multiplier should be greater than max multiprecision::uint128_t power 2

  boost::multiprecision::uint1024_t get_adjuster_for_fork_choice_rule_hf4()
  {
    return boost::multiprecision::uint1024_t(std::numeric_limits<boost::multiprecision::uint128_t>::max()) * 10 * std::numeric_limits<boost::multiprecision::uint128_t>::max();
  }

  const boost::multiprecision::uint1024_t adjusting_multiplier = get_adjuster_for_fork_choice_rule_hf4();

  boost::multiprecision::uint1024_t get_a_to_b_relative_cumulative_difficulty_hf4(const wide_difficulty_type& difficulty_pos_at_split_point,
    const wide_difficulty_type& difficulty_pow_at_split_point,
    const difficulties& a_diff,
    const difficulties& b_diff)
  {
    static const wide_difficulty_type difficulty_pos_starter = DIFFICULTY_POS_STARTER;
    static const wide_difficulty_type difficulty_pow_starter = DIFFICULTY_POW_STARTER;
    const wide_difficulty_type& a_pos_cumulative_difficulty = a_diff.pos_diff > 0 ? a_diff.pos_diff : difficulty_pos_starter;
    const wide_difficulty_type& b_pos_cumulative_difficulty = b_diff.pos_diff > 0 ? b_diff.pos_diff : difficulty_pos_starter;
    const wide_difficulty_type& a_pow_cumulative_difficulty = a_diff.pow_diff > 0 ? a_diff.pow_diff : difficulty_pow_starter;
    const wide_difficulty_type& b_pow_cumulative_difficulty = b_diff.pow_diff > 0 ? b_diff.pow_diff : difficulty_pow_starter;

    boost::multiprecision::uint1024_t basic_sum_ = boost::multiprecision::uint1024_t(a_pow_cumulative_difficulty) + (boost::multiprecision::uint1024_t(a_pos_cumulative_difficulty)*difficulty_pow_at_split_point) / difficulty_pos_at_split_point;
    boost::multiprecision::uint1024_t basic_sum_pow_minus2 = adjusting_multiplier /(basic_sum_ * basic_sum_);
    boost::multiprecision::uint1024_t res =
      (basic_sum_pow_minus2 * a_pow_cumulative_difficulty * a_pos_cumulative_difficulty) / (boost::multiprecision::uint1024_t(b_pow_cumulative_difficulty)*b_pos_cumulative_difficulty);

    //     if (res > boost::math::tools::max_value<wide_difficulty_type>())
    //     {
    //       ASSERT_MES_AND_THROW("[INTERNAL ERROR]: Failed to get_a_to_b_relative_cumulative_difficulty, res = " << res << ENDL
    //         << ", difficulty_pos_at_split_point: " << difficulty_pos_at_split_point << ENDL
    //         << ", difficulty_pow_at_split_point:" << difficulty_pow_at_split_point << ENDL
    //         << ", a_pos_cumulative_difficulty:" << a_pos_cumulative_difficulty << ENDL
    //         << ", b_pos_cumulative_difficulty:" << b_pos_cumulative_difficulty << ENDL
    //         << ", a_pow_cumulative_difficulty:" << a_pow_cumulative_difficulty << ENDL
    //         << ", b_pow_cumulative_difficulty:" << b_pow_cumulative_difficulty << ENDL       
    //       );
    //     }
    TRY_ENTRY();
    //    wide_difficulty_type short_res = res.convert_to<wide_difficulty_type>();
    return res;
    CATCH_ENTRY_WITH_FORWARDING_EXCEPTION();
  }



} // namespace currency


