// Copyright (c) 2018-2023 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once


#include "include_base_utils.h"
#include "crypto/crypto.h"
#include "currency_core/currency_basic.h"
#include "currency_protocol/blobdatatype.h"
#include "currency_core/account.h"


namespace currency
{
  struct tx_source_entry
  {
    struct output_entry
    {
      output_entry() = default;
      output_entry(const output_entry &) = default;
      output_entry(const txout_ref_v& out_reference, const crypto::public_key& stealth_address)
        : out_reference(out_reference), stealth_address(stealth_address), concealing_point(null_pkey), amount_commitment(null_pkey) {}
      output_entry(const txout_ref_v& out_reference, const crypto::public_key& stealth_address, const crypto::public_key& concealing_point, const crypto::public_key& amount_commitment, const crypto::public_key& blinded_asset_id)
        : out_reference(out_reference), stealth_address(stealth_address), concealing_point(concealing_point), amount_commitment(amount_commitment), blinded_asset_id(blinded_asset_id) {}

      txout_ref_v         out_reference;      // either global output index or ref_by_id
      crypto::public_key  stealth_address;    // a.k.a output's one-time public key
      crypto::public_key  concealing_point;   // only for ZC outputs
      crypto::public_key  amount_commitment;  // only for ZC outputs
      crypto::public_key  blinded_asset_id;   // only for ZC outputs

      bool operator==(const output_entry& rhs) const { return out_reference == rhs.out_reference; } // used in prepare_outputs_entries_for_key_offsets, it's okay to do partially comparison

      BEGIN_SERIALIZE_OBJECT()
        FIELD(out_reference)
        FIELD(stealth_address)
        FIELD(concealing_point)
        FIELD(amount_commitment)
        FIELD(blinded_asset_id)
      END_SERIALIZE()
    };

    //typedef serializable_pair<txout_ref_v, crypto::public_key> output_entry; // txout_ref_v is either global output index or ref_by_id; public_key - is output's stealth address

    std::vector<output_entry> outputs;
    uint64_t real_output = 0;                                 //index in outputs vector of real output_entry
    crypto::public_key real_out_tx_key = currency::null_pkey; //real output's transaction's public key
    crypto::scalar_t real_out_amount_blinding_mask = 0;       //blinding mask of real out's amount committment (only for ZC inputs, otherwise must be 0)
    crypto::scalar_t real_out_asset_id_blinding_mask = 0;     //blinding mask of real out's asset_od (only for ZC inputs, otherwise must be 0)
    size_t real_output_in_tx_index = 0;                       //index in transaction outputs vector
    uint64_t amount = 0;                                      //money
    uint64_t transfer_index = 0;                              //index in m_transfers
    crypto::hash multisig_id = currency::null_hash;           //if txin_multisig: multisig output id
    size_t ms_sigs_count = 0;                                 //if txin_multisig: must be equal to output's minimum_sigs
    size_t ms_keys_count = 0;                                 //if txin_multisig: must be equal to size of output's keys container
    bool separately_signed_tx_complete = false;               //for separately signed tx only: denotes the last source entry in complete tx to explicitly mark the final step of tx creation
    std::string htlc_origin;                                  //for htlc, specify origin
    crypto::public_key asset_id = currency::native_coin_asset_id; //asset id (not blinded, not premultiplied by 1/8) TODO @#@# consider changing to crypto::point_t

    bool is_multisig() const    { return ms_sigs_count > 0; }
    bool is_zc() const          { return !real_out_amount_blinding_mask.is_zero(); }
    bool is_native_coin() const { return asset_id == currency::native_coin_asset_id; }
    uint64_t amount_for_global_output_index() const { return is_zc() ? 0 : amount; } // amount value for global outputs index, it's zero for outputs with hidden amounts

    BEGIN_SERIALIZE_OBJECT()
      FIELD(outputs)
      FIELD(real_output)
      FIELD(real_out_tx_key)
      FIELD(real_out_amount_blinding_mask)
      FIELD(real_out_asset_id_blinding_mask)
      FIELD(real_output_in_tx_index)
      FIELD(amount)
      FIELD(transfer_index)
      FIELD(multisig_id)
      FIELD(ms_sigs_count)
      FIELD(ms_keys_count)
      FIELD(separately_signed_tx_complete)
      FIELD(htlc_origin)
    END_SERIALIZE()
  };


  //if this struct is present, then creating htlc out, expiration -> number of blocks that htlc proposal is active
  struct destination_option_htlc_out
  {
    uint64_t expiration = 0;
    crypto::hash htlc_hash = currency::null_hash;

    BEGIN_SERIALIZE_OBJECT()
      FIELD(expiration)
      FIELD(htlc_hash)
    END_SERIALIZE()
  };


  enum tx_destination_entry_flags
  {
    tdef_none                         = 0,
    tdef_explicit_native_asset_id     = 0x0001,
    tdef_explicit_amount_to_provide   = 0x0002,
    tdef_zero_amount_blinding_mask    = 0x0004  // currently it's only used for burning native coins
  };

  struct tx_destination_entry
  {
    uint64_t amount = 0;                                // money
    std::list<account_public_address>   addr;           // destination address, in case of 1 address - txout_to_key, in case of more - txout_multisig
    size_t   minimum_sigs = 0;                          // if txout_multisig: minimum signatures that are required to spend this output (minimum_sigs <= addr.size())  IF txout_to_key - not used
    uint64_t amount_to_provide = 0;                     // amount money that provided by initial creator of tx, used with partially created transactions
    uint64_t unlock_time = 0;
    destination_option_htlc_out htlc_options;           // htlc options    
    crypto::public_key asset_id = currency::native_coin_asset_id; // not blinded, not premultiplied
    uint64_t flags = 0;                                 // set of flags (see tx_destination_entry_flags)
    
    tx_destination_entry() = default;
    tx_destination_entry(uint64_t a, const account_public_address& ad) : amount(a), addr(1, ad) {}
    tx_destination_entry(uint64_t a, const account_public_address& ad, const crypto::public_key& aid) : amount(a), addr(1, ad), asset_id(aid) {}
    tx_destination_entry(uint64_t a, const account_public_address& ad, uint64_t ut) : amount(a), addr(1, ad), unlock_time(ut) {}
    tx_destination_entry(uint64_t a, const std::list<account_public_address>& addr) : amount(a), addr(addr), minimum_sigs(addr.size()){}
    tx_destination_entry(uint64_t a, const std::list<account_public_address>& addr, const crypto::public_key& aid) : amount(a), addr(addr), minimum_sigs(addr.size()), asset_id(aid) {}

    bool is_native_coin() const { return asset_id == currency::native_coin_asset_id; }


    BEGIN_SERIALIZE_OBJECT()
      FIELD(amount)
      FIELD(addr)
      FIELD(minimum_sigs)
      FIELD(amount_to_provide)
      FIELD(unlock_time)
      FIELD(htlc_options)
      FIELD(asset_id)
      FIELD(flags)
    END_SERIALIZE()
  };
  //---------------------------------------------------------------
  template<class variant_t, class variant_type_t>
  void update_or_add_field_to_extra(std::vector<variant_t>& variant_container, const variant_type_t& v)
  {
    for (auto& ev : variant_container)
    {
      if (ev.type() == typeid(variant_type_t))
      {
        boost::get<variant_type_t>(ev) = v;
        return;
      }
    }
    variant_container.push_back(v);
  }
  //---------------------------------------------------------------
  template<class variant_type_t, class variant_t>
  void remove_field_of_type_from_extra(std::vector<variant_t>& variant_container)
  {
    for (size_t i = 0; i != variant_container.size();)
    {
      if (variant_container[i].type() == typeid(variant_type_t))
      {
        variant_container.erase(variant_container.begin()+i);
      }
      else
      {
        i++;
      }
    }
  }
  //---------------------------------------------------------------
  template<class extra_type_t>
  uint64_t get_tx_x_detail(const transaction& tx)
  {
    extra_type_t e = AUTO_VAL_INIT(e);
    get_type_in_variant_container(tx.extra, e);
    return e.v;
  }
  //---------------------------------------------------------------
  template<class extra_type_t>
  void set_tx_x_detail(transaction& tx, uint64_t v)
  {
    extra_type_t e = AUTO_VAL_INIT(e);
    e.v = v;
    update_or_add_field_to_extra(tx.extra, e);
  }
  //---------------------------------------------------------------
  uint64_t get_tx_unlock_time(const transaction& tx, uint64_t o_i);
  uint64_t get_tx_max_unlock_time(const transaction& tx);
  bool get_tx_max_min_unlock_time(const transaction& tx, uint64_t& max_unlock_time, uint64_t& min_unlock_time);
  inline bool should_unlock_value_be_treated_as_block_height(uint64_t v) { return v < CURRENCY_MAX_BLOCK_NUMBER; }
  inline uint64_t get_tx_flags(const transaction& tx) { return get_tx_x_detail<etc_tx_details_flags>(tx); }
  inline uint64_t get_tx_expiration_time(const transaction& tx) {return get_tx_x_detail<etc_tx_details_expiration_time>(tx); }
  inline void set_tx_unlock_time(transaction& tx, uint64_t v) { set_tx_x_detail<etc_tx_details_unlock_time>(tx, v); }
  inline void set_tx_flags(transaction& tx, uint64_t v) { set_tx_x_detail<etc_tx_details_flags>(tx, v); }
  inline void set_tx_expiration_time(transaction& tx, uint64_t v) { set_tx_x_detail<etc_tx_details_expiration_time>(tx, v); }
  account_public_address get_crypt_address_from_destinations(const account_keys& sender_account_keys, const std::vector<tx_destination_entry>& destinations);
  //-----------------------------------------------------------------------------------------------

  bool is_tx_expired(const transaction& tx, uint64_t expiration_ts_median);
  uint64_t get_burned_amount(const transaction& tx);
  void get_transaction_prefix_hash(const transaction_prefix& tx, crypto::hash& h);
  crypto::hash get_transaction_prefix_hash(const transaction_prefix& tx);
  bool parse_and_validate_tx_from_blob(const blobdata& tx_blob, transaction& tx, crypto::hash& tx_hash);
  bool parse_and_validate_tx_from_blob(const blobdata& tx_blob, transaction& tx);
  crypto::hash get_transaction_hash(const transaction& t);
  bool get_transaction_hash(const transaction& t, crypto::hash& res);
  bool get_transaction_hash(const transaction& t, crypto::hash& res, uint64_t& blob_size);
  size_t get_object_blobsize(const transaction& t);
  size_t get_objects_blobsize(const std::list<transaction>& ls);
  size_t get_object_blobsize(const transaction& t, uint64_t prefix_blob_size);
  blobdata tx_to_blob(const transaction& b);
  bool tx_to_blob(const transaction& b, blobdata& b_blob);
  bool read_keyimages_from_tx(const transaction& tx, std::list<crypto::key_image>& kil);
  bool validate_inputs_sorting(const transaction& tx);
  bool is_asset_emitting_transaction(const transaction& tx, asset_descriptor_operation* p_ado = nullptr);  

  std::vector<tx_source_entry::output_entry> prepare_outputs_entries_for_key_offsets(const std::vector<tx_source_entry::output_entry>& outputs, size_t old_real_index, size_t& new_real_index) noexcept;


  struct tx_generation_context
  {
    tx_generation_context() = default;

    void resize(size_t zc_ins_count, size_t outs_count)
    {
      asset_ids.resize(outs_count);
      blinded_asset_ids.resize(outs_count);
      amount_commitments.resize(outs_count);
      asset_id_blinding_masks.resize(outs_count);
      amounts.resize(outs_count);
      amount_blinding_masks.resize(outs_count);
      zc_input_amounts.resize(zc_ins_count);
    }

    // TODO @#@# reconsider this check -- sowle
    bool check_sizes(size_t zc_ins_count, size_t outs_count) const
    {
      return
        pseudo_outs_blinded_asset_ids.size()  == zc_ins_count &&
        asset_ids.size()                      == outs_count &&
        blinded_asset_ids.size()              == outs_count &&
        amount_commitments.size()             == outs_count &&
        asset_id_blinding_masks.size()        == outs_count &&
        amounts.size()                        == outs_count &&
        amount_blinding_masks.size()          == outs_count;
    }

    void set_tx_key(const keypair& kp)
    {
      tx_key = kp;
      tx_pub_key_p = crypto::point_t(tx_key.pub);
    }

    // per output data
    std::vector<crypto::point_t> asset_ids;
    std::vector<crypto::point_t> blinded_asset_ids;                                   // generate_zc_outs_range_proof
    std::vector<crypto::point_t> amount_commitments;                                  // generate_zc_outs_range_proof   construct_tx_out
    crypto::scalar_vec_t asset_id_blinding_masks;                                     //                                construct_tx_out
    crypto::scalar_vec_t amounts;                                                     // generate_zc_outs_range_proof
    crypto::scalar_vec_t amount_blinding_masks;                                       // generate_zc_outs_range_proof

    // per zc input data
    std::vector<crypto::point_t> pseudo_outs_blinded_asset_ids;                       // generate_asset_surjection_proof
    crypto::scalar_vec_t pseudo_outs_plus_real_out_blinding_masks; // r_pi + r'_j     // generate_asset_surjection_proof
    std::vector<crypto::point_t> real_zc_ins_asset_ids;            // H_i             // generate_asset_surjection_proof
    std::vector<uint64_t> zc_input_amounts;                        // ZC only input amounts

    // common data: inputs
    crypto::point_t  pseudo_out_amount_commitments_sum      = crypto::c_point_0;      //                                                   generate_tx_balance_proof  generate_ZC_sig
    crypto::scalar_t pseudo_out_amount_blinding_masks_sum   = 0;                      //                                                                              generate_ZC_sig
    crypto::scalar_t real_in_asset_id_blinding_mask_x_amount_sum = 0;                 // = sum( real_out_blinding_mask[i] * amount[i] )    generate_tx_balance_proof  generate_ZC_sig

    // common data: outputs
    crypto::point_t  amount_commitments_sum                 = crypto::c_point_0;      //                                                   generate_tx_balance_proof
    crypto::scalar_t amount_blinding_masks_sum              = 0;                      //                                construct_tx_out   generate_tx_balance_proof  generate_ZC_sig
    crypto::scalar_t asset_id_blinding_mask_x_amount_sum    = 0;                      // = sum( blinding_mask[j] * amount[j] )             generate_tx_balance_proof

    // data for ongoing asset operation in tx (if applicable, tx extra should contain asset_descriptor_operation)
    crypto::public_key  ao_asset_id                         {};
    crypto::point_t     ao_asset_id_pt                      = crypto::c_point_0;
    crypto::point_t     ao_amount_commitment                = crypto::c_point_0;
    crypto::scalar_t    ao_amount_blinding_mask             {};                       //                                                   generate_tx_balance_proof  generate_ZC_sig
    bool                ao_commitment_in_outputs            = false;

    // per tx data
    keypair             tx_key                              {};                       //
    crypto::point_t     tx_pub_key_p                        = crypto::c_point_0;      // == tx_key.pub

    // consider redesign, some data may possibly be excluded from kv serialization -- sowle
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_CONTAINER_POD_AS_HEX(asset_ids)
      KV_SERIALIZE_CONTAINER_POD_AS_HEX(blinded_asset_ids)
      KV_SERIALIZE_CONTAINER_POD_AS_HEX(amount_commitments)
      KV_SERIALIZE_CONTAINER_POD_AS_HEX(asset_id_blinding_masks)
      KV_SERIALIZE_CONTAINER_POD_AS_HEX(amounts)
      KV_SERIALIZE_CONTAINER_POD_AS_HEX(amount_blinding_masks)
      KV_SERIALIZE_CONTAINER_POD_AS_HEX(pseudo_outs_blinded_asset_ids)
      KV_SERIALIZE_CONTAINER_POD_AS_HEX(pseudo_outs_plus_real_out_blinding_masks)
      KV_SERIALIZE_CONTAINER_POD_AS_HEX(real_zc_ins_asset_ids)
      KV_SERIALIZE(zc_input_amounts)
      KV_SERIALIZE_POD_AS_HEX_STRING(pseudo_out_amount_commitments_sum)
      KV_SERIALIZE_POD_AS_HEX_STRING(pseudo_out_amount_blinding_masks_sum)
      KV_SERIALIZE_POD_AS_HEX_STRING(real_in_asset_id_blinding_mask_x_amount_sum)
      KV_SERIALIZE_POD_AS_HEX_STRING(amount_commitments_sum)
      KV_SERIALIZE_POD_AS_HEX_STRING(amount_blinding_masks_sum)
      KV_SERIALIZE_POD_AS_HEX_STRING(asset_id_blinding_mask_x_amount_sum)
      KV_SERIALIZE_POD_AS_HEX_STRING(ao_asset_id)
      KV_SERIALIZE_POD_AS_HEX_STRING(ao_asset_id_pt)
      KV_SERIALIZE_POD_AS_HEX_STRING(ao_amount_commitment)
      KV_SERIALIZE_POD_AS_HEX_STRING(ao_amount_blinding_mask)
      KV_SERIALIZE_POD_AS_HEX_STRING(ao_commitment_in_outputs)
      KV_SERIALIZE_POD_AS_HEX_STRING(tx_key)
      KV_SERIALIZE_POD_AS_HEX_STRING(tx_pub_key_p)
    END_KV_SERIALIZE_MAP()
  
    // solely for consolidated txs, asset opration fields are not serialized
    BEGIN_SERIALIZE_OBJECT()
      VERSION(0)
      FIELD(asset_ids)
      FIELD(blinded_asset_ids)
      FIELD(amount_commitments)
      FIELD((std::vector<crypto::scalar_t>&)(asset_id_blinding_masks))
      FIELD((std::vector<crypto::scalar_t>&)(amounts))
      FIELD((std::vector<crypto::scalar_t>&)(amount_blinding_masks))
      FIELD(pseudo_outs_blinded_asset_ids)
      FIELD((std::vector<crypto::scalar_t>&)(pseudo_outs_plus_real_out_blinding_masks))
      FIELD(real_zc_ins_asset_ids)
      FIELD(zc_input_amounts)
      FIELD(pseudo_out_amount_commitments_sum)
      FIELD(pseudo_out_amount_blinding_masks_sum)
      FIELD(real_in_asset_id_blinding_mask_x_amount_sum)
      FIELD(amount_commitments_sum)
      FIELD(amount_blinding_masks_sum)
      FIELD(asset_id_blinding_mask_x_amount_sum)

      // no asset operation fields here
      //ao_asset_id
      //ao_asset_id_pt
      //ao_amount_commitment
      //ao_amount_blinding_mask
      //ao_commitment_in_outputs
      
      FIELD(tx_key.pub) // TODO: change to sane serialization FIELD(tx_key)
      FIELD(tx_key.sec)
      FIELD(tx_pub_key_p)
    END_SERIALIZE()
  }; // struct tx_generation_context

  bool validate_tx_details_against_tx_generation_context(const transaction& tx, const tx_generation_context& gen_context);

  std::string transform_tx_to_str(const transaction& tx);
  transaction transform_str_to_tx(const std::string& tx_str);



} // namespace currency
