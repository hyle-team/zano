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
}