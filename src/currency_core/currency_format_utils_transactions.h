// Copyright (c) 2018-2019 Zano Project
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
    typedef serializable_pair<txout_ref_v, crypto::public_key> output_entry; // txout_v is either global output index or ref_by_id; public_key - is output ephemeral pub key

    std::vector<output_entry> outputs;  //index + key
    uint64_t real_output;               //index in outputs vector of real output_entry
    crypto::public_key real_out_tx_key; //real output's transaction's public key
    size_t real_output_in_tx_index;     //index in transaction outputs vector
    uint64_t amount;                    //money
    uint64_t transfer_index;            //money
    crypto::hash multisig_id;           //if txin_multisig: multisig output id
    size_t ms_sigs_count;               //if txin_multisig: must be equal to output's minimum_sigs
    size_t ms_keys_count;               //if txin_multisig: must be equal to size of output's keys container
    bool separately_signed_tx_complete; //for separately signed tx only: denotes the last source entry in complete tx to explicitly mark the final step of tx creation

    bool is_multisig() const { return ms_sigs_count > 0; }

    BEGIN_SERIALIZE_OBJECT()
      FIELD(outputs)
      FIELD(real_output)
      FIELD(real_out_tx_key)
      FIELD(real_output_in_tx_index)
      FIELD(amount)
      FIELD(transfer_index)
      FIELD(multisig_id)
      FIELD(ms_sigs_count)
      FIELD(ms_keys_count)
      FIELD(separately_signed_tx_complete)
      END_SERIALIZE()
  };

  struct tx_destination_entry
  {
    uint64_t amount;                                 //money
    std::list<account_public_address>   addr;        //destination address, in case of 1 address - txout_to_key, in case of more - txout_multisig
    size_t   minimum_sigs;                           // if txout_multisig: minimum signatures that are required to spend this output (minimum_sigs <= addr.size())  IF txout_to_key - not used
    uint64_t amount_to_provide;                      //amount money that provided by initial creator of tx, used with partially created transactions
    uint64_t unlock_time;

    tx_destination_entry() : amount(0), minimum_sigs(0), amount_to_provide(0), unlock_time(0){}
    tx_destination_entry(uint64_t a, const account_public_address& ad) : amount(a), addr(1, ad), minimum_sigs(0), amount_to_provide(0), unlock_time(0){}
    tx_destination_entry(uint64_t a, const account_public_address& ad, uint64_t ut) : amount(a), addr(1, ad), minimum_sigs(0), amount_to_provide(0), unlock_time(ut) {}
    tx_destination_entry(uint64_t a, const std::list<account_public_address>& addr) : amount(a), addr(addr), minimum_sigs(addr.size()), amount_to_provide(0), unlock_time(0){}

    BEGIN_SERIALIZE_OBJECT()
      FIELD(amount)
      FIELD(addr)
      FIELD(minimum_sigs)
      FIELD(amount_to_provide)
      FIELD(unlock_time)
    END_SERIALIZE()
  };

  template<class extra_type_t>
  uint64_t get_tx_x_detail(const transaction& tx)
  {
    extra_type_t e = AUTO_VAL_INIT(e);
    get_type_in_variant_container(tx.extra, e);
    return e.v;
  }
  template<class extra_type_t>
  void set_tx_x_detail(transaction& tx, uint64_t v)
  {
    extra_type_t e = AUTO_VAL_INIT(e);
    e.v = v;
    update_or_add_field_to_extra(tx.extra, e);
  }

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


}
