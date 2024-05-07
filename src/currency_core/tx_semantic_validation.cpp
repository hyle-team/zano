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
    std::unordered_set<crypto::key_image> key_images;
    crypto::key_image ki{};
    for(const auto& in_v : tx.vin)
    {
      if (get_key_image_from_txin_v(in_v, ki))
      {
        if (!key_images.insert(ki).second)
          return false;
      }
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool validate_tx_semantic(const transaction& tx, size_t tx_blob_size)
  {
    if (tx_blob_size >= CURRENCY_MAX_TRANSACTION_BLOB_SIZE)
    {
      LOG_PRINT_RED_L0("tx blob size is " << tx_blob_size << ", it is greater than or equal to allowed maximum of " << CURRENCY_MAX_TRANSACTION_BLOB_SIZE);
      return false;
    }

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
      LOG_PRINT_RED_L0("tx has invalid outputs, rejected for tx id= " << get_transaction_hash(tx));
      return false;
    }

    if (!check_bare_money_overflow(tx))
    {
      LOG_PRINT_RED_L0("tx has money overflow, rejected for tx id= " << get_transaction_hash(tx));
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

    // inexpensive check for pre-HF4 txs
    // post-HF4 txs balance are being checked in check_tx_balance()
    if (tx.version <= TRANSACTION_VERSION_PRE_HF4)
    {
      if (!check_tx_bare_balance(tx))
      {
        LOG_PRINT_RED_L0("balance check failed for tx " << get_transaction_hash(tx));
        return false;
      }
    }

    return true;
  }
}