// Copyright (c) 2018-2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.



#include "tx_semantic_validation.h"
#include "currency_format_utils.h"

namespace currency
{
  //-----------------------------------------------------------------------------------------------
  bool check_tx_extra(const transaction& tx)
  {
    tx_extra_info ei = AUTO_VAL_INIT(ei);
    bool r = parse_and_validate_tx_extra(tx, ei);
    if (!r)
      return false;
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool check_tx_inputs_keyimages_diff(const transaction& tx)
  {
    std::unordered_set<crypto::key_image> ki;
    BOOST_FOREACH(const auto& in, tx.vin)
    {
      if (in.type() == typeid(txin_to_key))
      {
        CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, tokey_in, false);
        if (!ki.insert(tokey_in.k_image).second)
          return false;
      }
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool validate_tx_semantic(const transaction& tx, size_t tx_block_size)
  {
    if (!tx.vin.size())
    {
      LOG_PRINT_RED_L0("tx with empty inputs, rejected for tx id= " << get_transaction_hash(tx));
      return false;
    }

    if (!check_inputs_types_supported(tx))
    {
      LOG_PRINT_RED_L0("unsupported input types for tx id= " << get_transaction_hash(tx));
      return false;
    }

    if (!check_outs_valid(tx))
    {
      LOG_PRINT_RED_L0("tx with invalid outputs, rejected for tx id= " << get_transaction_hash(tx));
      return false;
    }

    if (!check_money_overflow(tx))
    {
      LOG_PRINT_RED_L0("tx has money overflow, rejected for tx id= " << get_transaction_hash(tx));
      return false;
    }

    uint64_t amount_in = 0;
    get_inputs_money_amount(tx, amount_in);
    uint64_t amount_out = get_outs_money_amount(tx);

    if (amount_in < amount_out)
    {
      LOG_PRINT_RED_L0("tx with wrong amounts: ins " << amount_in << ", outs " << amount_out << ", rejected for tx id= " << get_transaction_hash(tx));
      return false;
    }

    if (tx_block_size >= CURRENCY_MAX_TRANSACTION_BLOB_SIZE)
    {
      LOG_PRINT_RED_L0("tx has too big size " << tx_block_size << ", expected no bigger than " << CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE);
      return false;
    }

    //check if tx use different key images
    if (!check_tx_inputs_keyimages_diff(tx))
    {
      LOG_PRINT_RED_L0("tx inputs have the same key images");
      return false;
    }

    if (!check_tx_extra(tx))
    {
      LOG_PRINT_RED_L0("tx has wrong extra, rejected");
      return false;
    }

    return true;
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

}