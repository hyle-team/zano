// Copyright (c) 2018-2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.



#include "currency_format_utils_transactions.h"
#include "serialization/serialization.h"
#include "currency_format_utils.h"
#include "currency_format_utils_abstract.h"
#include "variant_helper.h"

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
      VARIANT_CASE(tx_out_bare, o)
        if (o.target.type() == typeid(txout_to_key))
        {
          if (boost::get<txout_to_key>(o.target).key == null_pkey)
            res += o.amount;
        }
      VARIANT_CASE_TV(tx_out_zarcanum)
        //@#@
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
  size_t get_object_blobsize(const transaction& t, uint64_t prefix_blob_size)
  {
    size_t tx_blob_size = prefix_blob_size;

    if (is_coinbase(t))
      return tx_blob_size;

    // for purged tx, with empty signatures and attachments, this function should return the blob size
    // which the tx would have if the signatures and attachments were correctly filled with actual data

    // 1. signatures
    bool separately_signed_tx = get_tx_flags(t) & TX_FLAG_SIGNATURE_MODE_SEPARATE;

    tx_blob_size += tools::get_varint_packed_size(t.vin.size()); // size of transaction::signatures (equals to total inputs count)

    for (size_t i = 0; i != t.vin.size(); i++)
    {
      size_t sig_count = get_input_expected_signatures_count(t.vin[i]);
      if (separately_signed_tx && i == t.vin.size() - 1)
        ++sig_count;                                             // count in one more signature for the last input in a complete separately signed tx
      tx_blob_size += tools::get_varint_packed_size(sig_count);  // size of transaction::signatures[i]
      tx_blob_size += sizeof(crypto::signature) * sig_count;     // size of signatures' data itself
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
    BOOST_FOREACH(const auto& in, tx.vin)
    {
      if (in.type() == typeid(txin_to_key) || in.type() == typeid(txin_htlc))
      {
         
        if (!ki.insert(get_to_key_input_from_txin_v(in).k_image).second)
          return false;
      }
    }
    return true;
  }

}