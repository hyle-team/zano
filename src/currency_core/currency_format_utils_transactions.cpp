// Copyright (c) 2018-2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.



#include "currency_format_utils_transactions.h"
#include "serialization/serialization.h"
#include "currency_format_utils.h"
#include "currency_format_utils_abstract.h"
#include "common/variant_helper.h"

namespace currency
{
  //---------------------------------------------------------------
  account_public_address get_crypt_address_from_destinations(const account_keys& sender_account_keys, const std::vector<tx_destination_entry>& destinations)
  {
    for (const auto& de : destinations)
    {
      if (de.addr.size() == 1 && sender_account_keys.account_address != de.addr.back())
        return de.addr.back();                    // return the first destination address that is non-multisig and not equal to the sender's address
    }
    return sender_account_keys.account_address; // otherwise, fallback to sender's address
  }
  //------------------------------------------------------------------
  bool is_tx_expired(const transaction& tx, uint64_t expiration_ts_median)
  {
    if (expiration_ts_median == 0)
      return false;
    /// tx expiration condition (tx is ok if the following is true)
    /// tx_expiration_time - TX_EXPIRATION_MEDIAN_SHIFT > get_last_n_blocks_timestamps_median(TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW)
    uint64_t expiration_time = get_tx_expiration_time(tx);
    if (expiration_time == 0)
      return false; // 0 means it never expires
    return expiration_time <= expiration_ts_median + TX_EXPIRATION_MEDIAN_SHIFT;
  }
  //---------------------------------------------------------------
  uint64_t get_burned_amount(const transaction& tx)
  {
    uint64_t res = 0;
    for (auto& o : tx.vout)
    {
      VARIANT_SWITCH_BEGIN(o);
      VARIANT_CASE_CONST(tx_out_bare, o)
        if (o.target.type() == typeid(txout_to_key))
        {
          if (boost::get<txout_to_key>(o.target).key == null_pkey)
            res += o.amount;
        }
      VARIANT_CASE_CONST(tx_out_zarcanum, o)
        //@#@# TODO obtain info about public burn of native coins in ZC outputs
      VARIANT_CASE_THROW_ON_OTHER();        
      VARIANT_SWITCH_END();
    }
    return res;
  }
  //---------------------------------------------------------------
  uint64_t get_tx_max_unlock_time(const transaction& tx)
  {
    // etc_tx_details_unlock_time have priority over etc_tx_details_unlock_time2
    uint64_t v = get_tx_x_detail<etc_tx_details_unlock_time>(tx);
    if (v)
      return v;

    etc_tx_details_unlock_time2 ut2 = AUTO_VAL_INIT(ut2);
    get_type_in_variant_container(tx.extra, ut2);
    if (!ut2.unlock_time_array.size())
      return 0;

    uint64_t max_unlock_time = 0;
    CHECK_AND_ASSERT_THROW_MES(ut2.unlock_time_array.size() == tx.vout.size(), "unlock_time_array.size=" << ut2.unlock_time_array.size()
      << " is not the same as  tx.vout.size =" << tx.vout.size() << " in tx: " << get_transaction_hash(tx));
    for (size_t i = 0; i != tx.vout.size(); i++)
    {
      if (ut2.unlock_time_array[i] > max_unlock_time)
        max_unlock_time = ut2.unlock_time_array[i];
    }

    return max_unlock_time;
  }

  //---------------------------------------------------------------
  uint64_t get_tx_unlock_time(const transaction& tx, uint64_t o_i)
  { 
    // etc_tx_details_expiration_time have priority over etc_tx_details_expiration_time2
    uint64_t v = get_tx_x_detail<etc_tx_details_unlock_time>(tx); 
    if (v)
      return v;

    CHECK_AND_ASSERT_THROW_MES(tx.vout.size() > o_i, "tx.vout.size=" << tx.vout.size()
      << " is not bigger then o_i=" << o_i << " in tx: " << get_transaction_hash(tx));


    etc_tx_details_unlock_time2 ut2 = AUTO_VAL_INIT(ut2);
    get_type_in_variant_container(tx.extra, ut2);
    if (!ut2.unlock_time_array.size())
      return 0;
    
    CHECK_AND_ASSERT_THROW_MES(ut2.unlock_time_array.size() > o_i, "unlock_time_array.size=" << ut2.unlock_time_array.size() 
      << " is less or equal to o_i=" << o_i << " in tx: " << get_transaction_hash(tx));

    return ut2.unlock_time_array[o_i];
  }
  //---------------------------------------------------------------
  bool get_tx_max_min_unlock_time(const transaction& tx, uint64_t& max_unlock_time, uint64_t& min_unlock_time)
  {
    max_unlock_time = min_unlock_time = 0;
    // etc_tx_details_expiration_time have priority over etc_tx_details_expiration_time2
    uint64_t v = get_tx_x_detail<etc_tx_details_unlock_time>(tx);
    if (v)
    {
      max_unlock_time = min_unlock_time = v;
      return true;
    }

    etc_tx_details_unlock_time2 ut2 = AUTO_VAL_INIT(ut2);
    get_type_in_variant_container(tx.extra, ut2);
    if (!ut2.unlock_time_array.size())
    {
      return true;
    }
    CHECK_AND_ASSERT_THROW_MES(ut2.unlock_time_array.size() == tx.vout.size(), "unlock_time_array.size=" << ut2.unlock_time_array.size()
      << " is not equal tx.vout.size()=" << tx.vout.size() << " in tx: " << get_transaction_hash(tx));
    if (ut2.unlock_time_array.size())
    {
      max_unlock_time = min_unlock_time = ut2.unlock_time_array[0];
      for (size_t i = 1; i != ut2.unlock_time_array.size(); i++)
      {
        if (ut2.unlock_time_array[i] > max_unlock_time)
          max_unlock_time = ut2.unlock_time_array[i];
        if (ut2.unlock_time_array[i] < min_unlock_time)
          min_unlock_time = ut2.unlock_time_array[i];
      }
    }


    return true;
  }
  //---------------------------------------------------------------
  void get_transaction_prefix_hash(const transaction_prefix& tx, crypto::hash& h)
  {
    std::ostringstream s;
    binary_archive<true> a(s);
    ::serialization::serialize(a, const_cast<transaction_prefix&>(tx));
    std::string data = s.str();
    crypto::cn_fast_hash(data.data(), data.size(), h);
  }
  //---------------------------------------------------------------
  crypto::hash get_transaction_prefix_hash(const transaction_prefix& tx)
  {
    crypto::hash h = null_hash;
    get_transaction_prefix_hash(tx, h);
    return h;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_tx_from_blob(const blobdata& tx_blob, transaction& tx)
  {
    std::stringstream ss;
    ss << tx_blob;
    binary_archive<false> ba(ss);
    bool r = ::serialization::serialize(ba, tx);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse transaction from blob");
    return true;
  }
  //---------------------------------------------------------------
  bool parse_and_validate_tx_from_blob(const blobdata& tx_blob, transaction& tx, crypto::hash& tx_hash)
  {
    std::stringstream ss;
    ss << tx_blob;
    binary_archive<false> ba(ss);
    bool r = ::serialization::serialize(ba, tx);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse transaction from blob");
    //TODO: validate tx

    //crypto::cn_fast_hash(tx_blob.data(), tx_blob.size(), tx_hash);
    get_transaction_prefix_hash(tx, tx_hash);
    return true;
  }
  //---------------------------------------------------------------
  crypto::hash get_transaction_hash(const transaction& t)
  {
    return get_transaction_prefix_hash(t);
  }
  //---------------------------------------------------------------
  bool get_transaction_hash(const transaction& t, crypto::hash& res)
  {
    uint64_t blob_size = 0;
    return get_object_hash(static_cast<const transaction_prefix&>(t), res, blob_size);
  }
  //---------------------------------------------------------------
  bool get_transaction_hash(const transaction& t, crypto::hash& res, uint64_t& blob_size)
  {
    blob_size = 0;
    bool r = get_object_hash(static_cast<const transaction_prefix&>(t), res, blob_size);
    blob_size = get_object_blobsize(t, blob_size);
    return r;
  }
  //---------------------------------------------------------------
  size_t get_object_blobsize(const transaction& t)
  {
    size_t tx_blob_size = get_object_blobsize(static_cast<const transaction_prefix&>(t));
    return get_object_blobsize(t, tx_blob_size);
  }
  //---------------------------------------------------------------
  size_t get_objects_blobsize(const std::list<transaction>& ls)
  {
    size_t total = 0;
    for (const auto& tx : ls)
    {
      total += get_object_blobsize(tx);
    }
    return total;
  }
  //---------------------------------------------------------------
  inline size_t get_input_expected_signature_size_local(const txin_v& tx_in, bool last_input_in_separately_signed_tx)
  {
    struct txin_signature_size_visitor : public boost::static_visitor<size_t>
    {
      txin_signature_size_visitor(size_t add) : a(add) {}
      size_t a;
      size_t operator()(const txin_gen& /*txin*/) const   { return 0; }
      size_t operator()(const txin_to_key& txin) const    { return tools::get_varint_packed_size(txin.key_offsets.size() + a) + sizeof(crypto::signature) * (txin.key_offsets.size() + a); }
      size_t operator()(const txin_multisig& txin) const  { return tools::get_varint_packed_size(txin.sigs_count + a) + sizeof(crypto::signature) * (txin.sigs_count + a); }
      size_t operator()(const txin_htlc& txin) const      { return tools::get_varint_packed_size(1 + a) + sizeof(crypto::signature) * (1 + a);  }
      size_t operator()(const txin_zc_input& txin) const  { return 96 + tools::get_varint_packed_size(txin.key_offsets.size()) + txin.key_offsets.size() * 32; }
    };

    return boost::apply_visitor(txin_signature_size_visitor(last_input_in_separately_signed_tx ? 1 : 0), tx_in);
  }
  //---------------------------------------------------------------
  size_t get_object_blobsize(const transaction& t, uint64_t prefix_blob_size)
  {
    size_t tx_blob_size = prefix_blob_size;

    if (is_coinbase(t))
    {
      if (is_pos_miner_tx(t) && t.version > TRANSACTION_VERSION_PRE_HF4)
      {
        // Zarcanum
        return tx_blob_size;
      }
      return tx_blob_size;
    }

    // for purged tx, with empty signatures and attachments, this function should return the blob size
    // which the tx would have if the signatures and attachments were correctly filled with actual data

    // 1. signatures
    bool separately_signed_tx = get_tx_flags(t) & TX_FLAG_SIGNATURE_MODE_SEPARATE;

    tx_blob_size += tools::get_varint_packed_size(t.vin.size()); // size of transaction::signatures (equals to total inputs count)
    if (t.version > TRANSACTION_VERSION_PRE_HF4)
      tx_blob_size += t.vin.size(); // for HF4 txs 'signatures' is a verctor of variants, so it's +1 byte per signature (assuming sigs count equals to inputs count) 

    for (size_t i = 0; i != t.vin.size(); i++)
    {
      size_t sig_size = get_input_expected_signature_size_local(t.vin[i], separately_signed_tx && i == t.vin.size() - 1);
      tx_blob_size += sig_size;
    }

    // 2. attachments (try to find extra_attachment_info in tx prefix and count it in if succeed)
    extra_attachment_info eai = AUTO_VAL_INIT(eai);
    bool got_eai = false;
    if (separately_signed_tx)
    {
      // for separately-signed tx, try to obtain extra_attachment_info from the last input's etc_details
      const std::vector<txin_etc_details_v>* p_etc_details = get_input_etc_details(t.vin.back());
      got_eai = p_etc_details != nullptr && get_type_in_variant_container(*p_etc_details, eai);
    }
    if (!got_eai)
      got_eai = get_type_in_variant_container(t.extra, eai); // then from the extra

    if (got_eai)
      tx_blob_size += eai.sz; // sz is a size of whole serialized attachment blob, including attachments vector size
    else
      tx_blob_size += tools::get_varint_packed_size(static_cast<size_t>(0)); // no extra_attachment_info found - just add zero vector's size, 'cause it's serialized anyway

    return tx_blob_size;
  }
  //---------------------------------------------------------------
  blobdata tx_to_blob(const transaction& tx)
  {
    return t_serializable_object_to_blob(tx);
  }
  //---------------------------------------------------------------
  bool tx_to_blob(const transaction& tx, blobdata& b_blob)
  {
    return t_serializable_object_to_blob(tx, b_blob);
  }
  //---------------------------------------------------------------
  bool read_keyimages_from_tx(const transaction& tx, std::list<crypto::key_image>& kil)
  {
    std::unordered_set<crypto::key_image> ki;
    for(const auto& in : tx.vin)
    {
      if (in.type() == typeid(txin_to_key) || in.type() == typeid(txin_htlc) || in.type() == typeid(txin_zc_input))
      {
         
        if (!ki.insert(get_key_image_from_txin_v(in)).second)
          return false;
      }
    }
    return true;
  }
  //---------------------------------------------------------------
  bool validate_inputs_sorting(const transaction& tx)
  {
    if (get_tx_flags(tx) & TX_FLAG_SIGNATURE_MODE_SEPARATE)
      return true;


    size_t i = 0;
    for(; i+1 < tx.vin.size(); i++)
    {
      //same less_txin_v() function should be used for sorting inputs during transacction creation
      if (less_txin_v(tx.vin[i+1], tx.vin[i]))
      {
        return false;
      }
    }
    return true;
  }
  //---------------------------------------------------------------
  bool is_asset_emitting_transaction(const transaction& tx, asset_descriptor_operation* p_ado /* = nullptr */)
  {
    if (tx.version <= TRANSACTION_VERSION_PRE_HF4)
      return false;

    asset_descriptor_operation local_ado{};
    if (p_ado == nullptr)
      p_ado = &local_ado;

    if (!get_type_in_variant_container(tx.extra, *p_ado))
      return false;
    
    if (p_ado->operation_type == ASSET_DESCRIPTOR_OPERATION_REGISTER || p_ado->operation_type == ASSET_DESCRIPTOR_OPERATION_EMIT)
      return true;

    return false;
  }
  //---------------------------------------------------------------
  // Prepapres vector of output_entry to be used in key_offsets in a transaction input:
  // 1) sort all entries by gindex (while moving all ref_by_id to the end, keeping they relative order)
  // 2) convert absolute global indices to relative key_offsets 
  std::vector<tx_source_entry::output_entry> prepare_outputs_entries_for_key_offsets(const std::vector<tx_source_entry::output_entry>& outputs, size_t old_real_index, size_t& new_real_index) noexcept
  {
    TRY_ENTRY()

    std::vector<tx_source_entry::output_entry> result = outputs;
    if (outputs.size() < 2)
    {
      new_real_index = old_real_index;
      return result;
    }

    std::sort(result.begin(), result.end(), [](const tx_source_entry::output_entry& lhs, const tx_source_entry::output_entry& rhs)
    {
      if (lhs.out_reference.type() == typeid(uint64_t))
      {
        if (rhs.out_reference.type() == typeid(uint64_t))
          return boost::get<uint64_t>(lhs.out_reference) < boost::get<uint64_t>(rhs.out_reference);
        if (rhs.out_reference.type() == typeid(ref_by_id))
          return true;
        CHECK_AND_ASSERT_THROW_MES(false, "unexpected type in out_reference 1: " << rhs.out_reference.type().name());
      }
      else if (lhs.out_reference.type() == typeid(ref_by_id))
      {
        if (rhs.out_reference.type() == typeid(uint64_t))
          return false;
        if (rhs.out_reference.type() == typeid(ref_by_id))
          return false; // don't change the order of ref_by_id elements
        CHECK_AND_ASSERT_THROW_MES(false, "unexpected type in out_reference 2: " << rhs.out_reference.type().name());
      }
      return false;
    });

    // restore index of the selected element, if needed
    if (old_real_index != SIZE_MAX)
    {
      CHECK_AND_ASSERT_THROW_MES(old_real_index < outputs.size(), "old_real_index is OOB");
      auto it = std::find(result.begin(), result.end(), outputs[old_real_index]);
      CHECK_AND_ASSERT_THROW_MES(it != result.end(), "internal error: cannot find old_real_index");
      new_real_index = it - result.begin();
    }

    // find the last uint64_t entry - skip ref_by_id entries goint from the end to the beginnning
    size_t i = result.size() - 1;
    while (i != 0 && result[i].out_reference.type() == typeid(ref_by_id))
      --i;

    for (; i != 0; i--)
    {
      boost::get<uint64_t>(result[i].out_reference) -= boost::get<uint64_t>(result[i - 1].out_reference);
    }

    return result;

    CATCH_ENTRY2(std::vector<tx_source_entry::output_entry>{});
  }
  //---------------------------------------------------------------
  bool validate_tx_details_against_tx_generation_context(const transaction& tx, const tx_generation_context& tgc)
  {
    CHECK_AND_ASSERT_MES(tx.version >= TRANSACTION_VERSION_POST_HF4, false, "validate_tx_details_against_tx_generation_context should never be called for pre-HF4 txs");

    //
    // outputs
    // (all outputs are ZC outputs, because we require tx.version to be post-HF4)
    size_t tx_outs_count = tx.vout.size();
    CHECK_AND_ASSERT_EQ(tgc.asset_ids.size(),               tx_outs_count);
    CHECK_AND_ASSERT_EQ(tgc.blinded_asset_ids.size(),       tx_outs_count);
    CHECK_AND_ASSERT_EQ(tgc.amount_commitments.size(),      tx_outs_count);
    CHECK_AND_ASSERT_EQ(tgc.asset_id_blinding_masks.size(), tx_outs_count);
    CHECK_AND_ASSERT_EQ(tgc.amounts.size(),                 tx_outs_count);
    CHECK_AND_ASSERT_EQ(tgc.amount_blinding_masks.size(),   tx_outs_count);

    crypto::point_t amount_commitments_sum = crypto::c_point_0;
    crypto::scalar_t amount_blinding_masks_sum{};
    crypto::scalar_t asset_id_blinding_mask_x_amount_sum{};
    for(size_t i = 0; i < tx_outs_count; ++i)
    {
      crypto::point_t calculated_T = tgc.asset_ids[i] + tgc.asset_id_blinding_masks[i] * crypto::c_point_X;
      CHECK_AND_ASSERT_MES(calculated_T.is_in_main_subgroup(), false, "calculated_T isn't in the main subgroup");
      CHECK_AND_ASSERT_EQ(tgc.blinded_asset_ids[i], calculated_T);

      crypto::point_t calculated_A = tgc.amounts[i] * calculated_T + tgc.amount_blinding_masks[i] * crypto::c_point_G;
      CHECK_AND_ASSERT_MES(calculated_A.is_in_main_subgroup(), false, "calculated_A isn't in the main subgroup");
      CHECK_AND_ASSERT_EQ(tgc.amount_commitments[i], calculated_A);
      
      const tx_out_zarcanum& tx_out_zc = boost::get<tx_out_zarcanum>(tx.vout[i]);
      crypto::point_t tx_T = crypto::point_t(tx_out_zc.blinded_asset_id).modify_mul8();
      CHECK_AND_ASSERT_EQ(tgc.blinded_asset_ids[i], tx_T);
      crypto::point_t tx_A = crypto::point_t(tx_out_zc.amount_commitment).modify_mul8();
      CHECK_AND_ASSERT_EQ(tgc.amount_commitments[i], tx_A);
      amount_commitments_sum += calculated_A;
      amount_blinding_masks_sum += tgc.amount_blinding_masks[i];
      asset_id_blinding_mask_x_amount_sum += tgc.asset_id_blinding_masks[i] * tgc.amounts[i];
    }
    CHECK_AND_ASSERT_EQ(tgc.amount_commitments_sum,               amount_commitments_sum);
    CHECK_AND_ASSERT_EQ(tgc.amount_blinding_masks_sum,            amount_blinding_masks_sum);
    CHECK_AND_ASSERT_EQ(tgc.asset_id_blinding_mask_x_amount_sum,  asset_id_blinding_mask_x_amount_sum);

    //
    // inputs
    //

    size_t tx_inputs_count = tx.vin.size();
    size_t tx_zc_inputs_count = count_type_in_variant_container<txin_zc_input>(tx.vin);
    size_t tx_bare_inputs_count = count_type_in_variant_container<txin_to_key>(tx.vin);
    CHECK_AND_ASSERT_EQ(tx_inputs_count, tx_zc_inputs_count + tx_bare_inputs_count);
    CHECK_AND_ASSERT_EQ(tx.signatures.size(), tx.vin.size()); // can do this because we shouldn't face additional signature so far

    CHECK_AND_ASSERT_EQ(tgc.pseudo_outs_blinded_asset_ids.size(),             tx_zc_inputs_count);
    CHECK_AND_ASSERT_EQ(tgc.pseudo_outs_plus_real_out_blinding_masks.size(),  tx_zc_inputs_count);
    CHECK_AND_ASSERT_EQ(tgc.real_zc_ins_asset_ids.size(),                     tx_zc_inputs_count);
    CHECK_AND_ASSERT_EQ(tgc.zc_input_amounts.size(),                          tx_zc_inputs_count);

    crypto::point_t pseudo_out_amount_commitments_sum = crypto::c_point_0;

    for(size_t input_index = 0, zc_input_index = 0; input_index < tx.vin.size(); ++input_index)
    {
      const txin_v& in_v = tx.vin[input_index];
      if (in_v.type() == typeid(txin_zc_input))
      {
        CHECK_AND_ASSERT_EQ(tx.signatures[input_index].type(), typeid(ZC_sig));
        const ZC_sig& sig = boost::get<ZC_sig>(tx.signatures[input_index]);

        crypto::point_t sig_pseudo_out_amount_commitment = crypto::point_t(sig.pseudo_out_amount_commitment).modify_mul8();
        crypto::point_t sig_pseudo_out_blinded_asset_id = crypto::point_t(sig.pseudo_out_blinded_asset_id).modify_mul8();

        crypto::point_t pseudo_out_blinded_asset_id_calculated = tgc.real_zc_ins_asset_ids[zc_input_index] + tgc.pseudo_outs_plus_real_out_blinding_masks[zc_input_index] * crypto::c_point_X; // T^P = H_i + r_pi*X + r'_i*X
        CHECK_AND_ASSERT_EQ(sig_pseudo_out_blinded_asset_id, pseudo_out_blinded_asset_id_calculated);
        CHECK_AND_ASSERT_EQ(tgc.pseudo_outs_blinded_asset_ids[zc_input_index], pseudo_out_blinded_asset_id_calculated);
        CHECK_AND_ASSERT_MES(pseudo_out_blinded_asset_id_calculated.is_in_main_subgroup(), false, "pseudo_out_blinded_asset_id_calculated isn't in the main subgroup");
        // cannot be verified:
        // tgc.zc_input_amounts[zc_input_index];                        // ZC only input amounts

        pseudo_out_amount_commitments_sum += sig_pseudo_out_amount_commitment;
        ++zc_input_index;
      }
      else if (in_v.type() == typeid(txin_to_key))
      {
        // do nothing
      }
      else
      {
        // should never get here
        CHECK_AND_ASSERT_MES(false, false, "internal error, unexpected type: " << in_v.type().name());
      }
    }

    // tx secret and public keys
    CHECK_AND_ASSERT_MES(tgc.tx_pub_key_p.is_in_main_subgroup(), false, "tgc.tx_pub_key_p isn't in the main subgroup");
    CHECK_AND_ASSERT_EQ(tgc.tx_pub_key_p.to_public_key(), tgc.tx_key.pub);
    CHECK_AND_ASSERT_EQ(crypto::scalar_t(tgc.tx_key.sec) * crypto::c_point_G, tgc.tx_pub_key_p);

    // no ongoing asset operation
    CHECK_AND_ASSERT_EQ(tgc.ao_asset_id, currency::null_pkey);
    CHECK_AND_ASSERT_EQ(tgc.ao_asset_id_pt, crypto::c_point_0);
    CHECK_AND_ASSERT_EQ(tgc.ao_amount_commitment, crypto::c_point_0);
    CHECK_AND_ASSERT_EQ(tgc.ao_amount_blinding_mask, crypto::c_scalar_0);

    // cannot be verified:
    // tgc.pseudo_out_amount_blinding_masks_sum
    // tgc.real_in_asset_id_blinding_mask_x_amount_sum

    return true;
  }

  //----------------------------------------------------------------------------------------------------
  std::string transform_tx_to_str(const currency::transaction& tx)
  {
    return currency::obj_to_json_str(tx);
  }
  //----------------------------------------------------------------------------------------------------
  transaction transform_str_to_tx(const std::string& tx_str)
  {
    CHECK_AND_ASSERT_THROW_MES(false, "transform_str_to_tx shoruld never be called");
    return transaction();
  }


}
