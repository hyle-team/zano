// Copyright (c) 2014-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include "rpc/core_rpc_server_commands_defs.h"
#include "wallet/wallet_public_structs_defs.h"

namespace currency
{
  struct COMMAND_RPC_GATEWAY_GET_ADDRESS_HISTORY
  {
    DOC_COMMAND("Retrieves the transaction history for a specific gateway address.");
    
    struct request
    {
      std::string gateway_address;
      std::optional<crypto::secret_key>  gateway_view_secret_key; // TODO optional, if not provided, additional info like payment_id won't be decrypted
      uint64_t offset;
      uint64_t count;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(gateway_address)               DOC_DSCR("The gateway address for which transaction history is being requested.") DOC_EXMP("gateway1qxyz...") DOC_END
        KV_SERIALIZE(offset)                        DOC_DSCR("The offset in the transaction history from which to start retrieval.") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(count)                         DOC_DSCR("The number of transactions to retrieve from the specified offset.") DOC_EXMP(10) DOC_END
        KV_SERIALIZE_POD_AS_HEX_STRING(gateway_view_secret_key) DOC_DSCR("View secret key to decrypt attachments and payment id") DOC_EXMP("f74bb56a5b4fa562e679ccaadd697463498a66de4f1760b2cd40f11c3a00a7a8") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      uint64_t total_transactions = 0;
      std::list<tools::wallet_public::wallet_transfer_info> transactions;
      std::list<gateway_balance_entry> balances;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
        KV_SERIALIZE(total_transactions)         DOC_DSCR("Total number of transactions associated with the specified gateway address.") DOC_EXMP(100) DOC_END
        KV_SERIALIZE(transactions)               DOC_DSCR("List of transactions associated with the specified gateway address, retrieved based on the provided parameters.") DOC_EXMP_AUTO(1) DOC_END
        KV_SERIALIZE(balances)                   DOC_DSCR("List of balances for different asset_id associated with the gateway address at the time of the request.") DOC_EXMP_AUTO(1) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

}

