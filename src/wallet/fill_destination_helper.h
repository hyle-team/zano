// Copyright (c) 2014-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <list>
#include "wallet/wrap_service.h"
  
namespace currency
{
  template<typename callback_t>
  bool rpc_fill_destinations_helper(const std::list<currency::transfer_destination>& destinations_in_api,                                     
                                    std::vector<currency::tx_destination_entry>& dsts, std::vector<currency::extra_v>& extra, bool hf6_active, epee::json_rpc::error& er,
                                    std::string& legacy_tx_wide_payment_id, const bool allow_legacy_payment_id_size, const address_v& self_address,
                                    callback_t cb_get_address)
  {

    bool wrap = false;

    for (auto it = destinations_in_api.begin(); it != destinations_in_api.end(); it++)
    {
      currency::tx_destination_entry de{};
      de.addr.resize(1);
      std::string embedded_payment_id;
      //check if address looks like wrapped address
      if (currency::is_address_like_wrapped(it->address))
      {
        if (wrap)
        {
          LOG_ERROR("More then one entries in transactions");
          er.code = WALLET_RPC_ERROR_CODE_WRONG_ARGUMENT;
          er.message = "Second wrap entry not supported in transactions";
          return false;

        }
        LOG_PRINT_L0("Address " << it->address << " recognized as wrapped address, creating wrapping transaction...");
        //put into service attachment specially encrypted entry which will contain wrap address and network
        currency::tx_service_attachment sa = AUTO_VAL_INIT(sa);
        sa.service_id = BC_WRAP_SERVICE_ID;
        sa.instruction = BC_WRAP_SERVICE_INSTRUCTION_ERC20;
        sa.flags = TX_SERVICE_ATTACHMENT_ENCRYPT_BODY | TX_SERVICE_ATTACHMENT_ENCRYPT_BODY_ISOLATE_AUDITABLE;
        sa.body = it->address;
        extra.push_back(sa);

        currency::account_public_address acc = AUTO_VAL_INIT(acc);
        currency::get_account_address_from_str(acc, BC_WRAP_SERVICE_CUSTODY_WALLET);
        de.addr.front() = acc;
        wrap = true;
        //encrypt body with a special way
      }
      else if (!cb_get_address(it->address, de.addr.back(), embedded_payment_id))
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
        er.message = std::string("WALLET_RPC_ERROR_CODE_WRONG_ADDRESS: ") + it->address;
        return false;
      }

      bool self_directed_destination = (de.addr.front().type() == typeid(account_public_address) ? (de.addr.front() == self_address) : false);
      if (self_directed_destination && embedded_payment_id.size() != 0)
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
        er.message = std::string("transfers with an integrated address to self is not allowed: ") + it->address;
        return false;
      }

      //
      // payment id processing logic (if changed, make simplewallet::transfer_impl consistent; consider code reuse -- sowle)
      //
      if (hf6_active)
      {
        if (embedded_payment_id.size() != 0)
        {
          if (!legacy_tx_wide_payment_id.empty())
          {
            er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
            er.message = std::string("payment_id embeded into integrated address ") + it->address + " conflicts with previously set tx-wide payment id (perhaps, from another integrated address)";
            return false;
          }
          uint64_t intrinsic_embeded_payment_id = 0;
          if (currency::convert_payment_id(embedded_payment_id, intrinsic_embeded_payment_id))
          {
            if (it->payment_id != 0)
            {
              er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
              er.message = std::string("embedded payment id: ") + epee::string_tools::buff_to_hex_nodelimer(embedded_payment_id) + " conflicts with destination intrinsic payment id: " + epee::string_tools::pod_to_hex(it->payment_id);
              return false;
            }
            de.payment_id = intrinsic_embeded_payment_id;
          }
          else
          {
            // it means embedded_payment_id.size() > CURRENCY_HF6_INTRINSIC_PAYMENT_ID_SIZE
            if (!allow_legacy_payment_id_size)
            {
              er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
              er.message = std::string("payment_id embeded into integrated address ") + it->address + " is too long, maximum is " + epee::string_tools::num_to_string_fast(CURRENCY_HF6_INTRINSIC_PAYMENT_ID_SIZE) + " bytes";
              return false;
            }
            if (it != destinations_in_api.begin())
            {
              er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
              er.message = std::string("long embedded payment id: ") + epee::string_tools::buff_to_hex_nodelimer(embedded_payment_id) + " can only be set for the first destination (and so you can use integrated address with long payment id only for the fist destination)";
              return false;
            }
            legacy_tx_wide_payment_id = embedded_payment_id;
          }
        }

        if (it->payment_id != 0)
        {
          if (!legacy_tx_wide_payment_id.empty())
          {
            er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
            er.message = std::string("destination intrinsic payment id: ") + epee::string_tools::pod_to_hex(it->payment_id) + " conflicts with previously set tx-wide payment id";
            return false;
          }
          de.payment_id = it->payment_id;
        }
      }
      else
      {
        // before HF6
        if (it->payment_id != 0)
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
          er.message = std::string("intrinsic payment id cannot be used before HF6");
          return false;
        }

        if (embedded_payment_id.size() != 0)
        {
          if (!allow_legacy_payment_id_size && embedded_payment_id.size() > CURRENCY_HF6_INTRINSIC_PAYMENT_ID_SIZE)
          {
            er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
            er.message = std::string("payment_id embedded into integrated address ") + it->address + " is too long, maximum is " + epee::string_tools::num_to_string_fast(CURRENCY_HF6_INTRINSIC_PAYMENT_ID_SIZE) + " bytes";
            return false;
          }

          if (!legacy_tx_wide_payment_id.empty())
          {
            er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
            er.message = std::string("embedded payment id: ") + epee::string_tools::buff_to_hex_nodelimer(embedded_payment_id) + " conflicts with previously set payment id: " + epee::string_tools::buff_to_hex_nodelimer(legacy_tx_wide_payment_id);
            return false;
          }
          if (it != destinations_in_api.begin())
          {
            er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
            er.message = std::string("embedded payment id: ") + epee::string_tools::buff_to_hex_nodelimer(embedded_payment_id) + " currently can only be set for the first destination (and so you can use integrated address only for the fist destination)";
            return false;
          }
          legacy_tx_wide_payment_id = embedded_payment_id;
        }
      }

      de.amount = it->amount;
      de.asset_id = (it->asset_id == currency::null_pkey ? currency::native_coin_asset_id : it->asset_id);
      dsts.push_back(de);
    }


    return true;
  }


} // namespace tools::wallet_public

