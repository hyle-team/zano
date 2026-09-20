// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "include_base_utils.h"

#include <cstdint>
#include <unordered_map>

#include "gateway_rpc_validation.h"

#include "crypto/hash.h"
#include "currency_core/blockchain_storage.h"
#include "currency_core/crypto_config.h"
#include "currency_core/currency_format_utils.h"
#include "currency_core/currency_format_utils_transactions.h"
#include "currency_core/tx_semantic_validation.h"

namespace currency
{
  namespace gateway_rpc_validation
  {
    namespace
    {
      struct gateway_balance_change
      {
        uint64_t debit = 0;
        uint64_t credit = 0;
      };

      bool validate_balance_changes(const transaction& tx, const blockchain_storage& blockchain, std::string& error)
      {
        std::unordered_map<gateway_address_id_type, std::unordered_map<crypto::public_key, gateway_balance_change>> balance_changes;

        for (const auto& input : tx.vin)
        {
          if (input.type() != typeid(txin_gateway))
            continue;

          const txin_gateway& gateway_input = boost::get<txin_gateway>(input);
          crypto::public_key asset_id = null_pkey;
          if (!crypto::pub_key_mul8(gateway_input.asset_id, asset_id))
          {
            error = "Gateway input contains an invalid asset id";
            return false;
          }
          if (!assign_add_with_overflow_check(gateway_input.amount, balance_changes[gateway_input.gateway_addr][asset_id].debit))
          {
            error = "Gateway input amount overflow";
            return false;
          }
        }

        for (const auto& output : tx.vout)
        {
          if (output.type() != typeid(tx_out_gateway))
            continue;

          const tx_out_gateway& gateway_output = boost::get<tx_out_gateway>(output);
          crypto::public_key asset_id = null_pkey;
          if (!crypto::pub_key_mul8(gateway_output.asset_id, asset_id))
          {
            error = "Gateway output contains an invalid asset id";
            return false;
          }
          if (!assign_add_with_overflow_check(gateway_output.amount, balance_changes[gateway_output.gateway_addr][asset_id].credit))
          {
            error = "Gateway output amount overflow";
            return false;
          }
        }

        for (const auto& [gateway_id, asset_changes] : balance_changes)
        {
          auto gateway_data = blockchain.get_gateway_address_info(gateway_id);
          if (!gateway_data || gateway_data->info_history.empty())
          {
            error = "Gateway input or output refers to an unknown gateway address";
            return false;
          }

          for (const auto& [asset_id, change] : asset_changes)
          {
            auto balance_it = gateway_data->balances.find(asset_id);
            const uint64_t current_balance = balance_it == gateway_data->balances.end() ? 0 : balance_it->second.amount;
            // block application processes all gateway inputs before gateway outputs
            if (change.debit > current_balance)
            {
              error = "Gateway input amount exceeds the current on-chain balance";
              return false;
            }

            uint64_t resulting_balance = current_balance - change.debit;
            if (!assign_add_with_overflow_check(change.credit, resulting_balance))
            {
              error = "Gateway output would overflow the destination balance";
              return false;
            }
          }
        }
        return true;
      }
    }

    bool verify_owner_signature(const crypto::hash& hash, const gateway_owner_key_v& owner_key, const gateway_owner_signature_v& signature)
    {
      if (owner_key.type() == typeid(crypto::public_key))
      {
        if (signature.type() != typeid(crypto::generic_schnorr_sig_s))
          return false;
        return crypto::verify_schnorr_sig(hash, boost::get<crypto::public_key>(owner_key), boost::get<crypto::generic_schnorr_sig_s>(signature));
      }
      if (owner_key.type() == typeid(crypto::eth_public_key))
      {
        if (signature.type() != typeid(crypto::eth_signature))
          return false;
        return crypto::verify_eth_signature(hash, boost::get<crypto::eth_public_key>(owner_key), boost::get<crypto::eth_signature>(signature));
      }
      if (owner_key.type() == typeid(crypto::eddsa_public_key))
      {
        if (signature.type() != typeid(crypto::eddsa_signature))
          return false;
        return crypto::verify_eddsa_signature(hash, boost::get<crypto::eddsa_public_key>(owner_key), boost::get<crypto::eddsa_signature>(signature));
      }
      return false;
    }

    bool validate_input_signatures(const transaction& tx, const crypto::hash& tx_id, const blockchain_storage& blockchain, gateway_address_id_type& gateway_id, gateway_owner_key_v& owner_key, std::string& error)
    {
      if (tx.vin.empty())
      {
        error = "Transaction has no gateway inputs";
        return false;
      }
      if (tx.signatures.size() != tx.vin.size())
      {
        error = "Transaction signatures count differs from inputs count";
        return false;
      }

      bool gateway_id_initialized = false;
      for (size_t i = 0; i != tx.vin.size(); ++i)
      {
        if (tx.vin[i].type() != typeid(txin_gateway) || tx.signatures[i].type() != typeid(gateway_sig))
        {
          error = "Transaction must contain gateway inputs and gateway signatures only";
          return false;
        }

        const txin_gateway& gateway_input = boost::get<txin_gateway>(tx.vin[i]);
        if (!gateway_id_initialized)
        {
          gateway_id = gateway_input.gateway_addr;
          gateway_id_initialized = true;
        }
        else if (gateway_input.gateway_addr != gateway_id)
        {
          error = "All transaction inputs must belong to the same gateway address";
          return false;
        }
      }

      auto gateway_data = blockchain.get_gateway_address_info(gateway_id);
      if (!gateway_data || gateway_data->info_history.empty())
      {
        error = "Gateway address or its current owner was not found";
        return false;
      }
      owner_key = gateway_data->info_history.back().owner_key;

      for (size_t i = 0; i != tx.vin.size(); ++i)
      {
        const crypto::hash prefix_hash = prepare_prefix_hash_for_sign(tx, i, tx_id);
        if (prefix_hash == null_hash)
        {
          error = "Failed to prepare gateway input hash for signature verification";
          return false;
        }
        const crypto::hash hash_to_verify = crypto::hash_helper_t::h(CRYPTO_HDS_GW_INPUT_SIGNATURE, prefix_hash);
        const gateway_sig& signature = boost::get<gateway_sig>(tx.signatures[i]);
        if (!verify_owner_signature(hash_to_verify, owner_key, signature.s))
        {
          error = "Gateway input signature is not valid for the current owner key";
          return false;
        }
      }
      return true;
    }

    bool prevalidate_transaction(const transaction& tx, const blobdata& tx_blob, const crypto::hash& tx_id, const blockchain_storage& blockchain, std::string& error)
    {
      if (!blockchain.validate_tx_for_hardfork_specific_terms(tx, tx_id))
      {
        error = "Transaction does not satisfy current hardfork rules";
        return false;
      }
      if (!validate_tx_semantic(tx, tx_blob.size(), tx_id))
      {
        error = "Transaction semantic validation failed";
        return false;
      }
      if (blockchain.is_tx_expired(tx))
      {
        error = "Transaction is expired";
        return false;
      }
      if (!validate_attachment_info(tx.extra, tx.attachment, false))
      {
        error = "Transaction attachment metadata validation failed";
        return false;
      }

      uint64_t fee = 0;
      if (!get_tx_fee(tx, fee))
      {
        error = "Failed to determine transaction fee";
        return false;
      }
      if (fee < blockchain.get_core_runtime_config().tx_pool_min_fee)
      {
        error = "Transaction fee is below the current minimum";
        return false;
      }
      if (!validate_balance_changes(tx, blockchain, error))
        return false;
      if (!check_single_tx_range_proofs(tx, tx_id))
      {
        error = "Transaction range proof validation failed";
        return false;
      }
      if (!verify_asset_surjection_proof(tx, tx_id))
      {
        error = "Transaction asset surjection proof validation failed";
        return false;
      }
      if (!check_tx_balance(tx, tx_id))
      {
        error = "Transaction balance proof validation failed";
        return false;
      }
      return true;
    }

    bool prevalidate_signed_transaction(const transaction& tx, const blobdata& tx_blob, const crypto::hash& tx_id, const blockchain_storage& blockchain, std::string& error)
    {
      if (!prevalidate_transaction(tx, tx_blob, tx_id, blockchain, error))
        return false;
      if (!blockchain.check_tx_inputs(tx, tx_id))
      {
        error = "Transaction input validation failed";
        return false;
      }
      return true;
    }
  }
}
