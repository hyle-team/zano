// Copyright (c) 2018-2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.



#include "currency_format_utils_transactions.h"
#include "serialization/serialization.h"
#include "currency_format_utils_abstract.h"
#include "currency_format_utils.h"

namespace currency
{
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
  uint64_t get_burned_coins_amount(const transaction& tx)
  {
    uint64_t res = 0;
    for (auto& o : tx.vout)
    {
      if (o.target.type() == typeid(txout_to_key))
      {
        if (boost::get<txout_to_key>(o.target).key == currency::null_pkey)
        {
          res += o.amount;
        }
      }
    }
    return res;
  }
  //---------------------------------------------------------------
}