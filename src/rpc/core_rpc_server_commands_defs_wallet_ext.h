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
    DOC_COMMAND("Retrieves the transaction history for a gateway address, if a view secret key is provided, payment_id/attachments are decrypted server-side (this exposes the key to the daemon operator). If it is omitted, the daemon does NOT decrypt and returns the raw transactions instead, so the caller can decrypt locally without sending the key to the daemon (preferred - see gateway_rpc_proxy).");

    struct request
    {
      std::string gateway_address;
      std::optional<crypto::secret_key>  gateway_view_secret_key; // optional, if not provided, additional info like payment_id won't be decrypted
      uint64_t offset;
      uint64_t count;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(gateway_address)               DOC_DSCR("The gateway address for which transaction history is being requested.") DOC_EXMP("gateway1qxyz...") DOC_END
        KV_SERIALIZE(offset)                        DOC_DSCR("The offset in the transaction history from which to start retrieval.") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(count)                         DOC_DSCR("The number of transactions to retrieve from the specified offset.") DOC_EXMP(10) DOC_END
        KV_SERIALIZE_POD_AS_HEX_STRING(gateway_view_secret_key) DOC_DSCR("Optional, if provided, the daemon decrypts payment_id/attachments server-side, if omitted, raw_txs are returned for client-side decryption instead.") DOC_EXMP("f74bb56a5b4fa562e679ccaadd697463498a66de4f1760b2cd40f11c3a00a7a8") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::string status_error;
      uint64_t total_transactions = 0;
      std::list<tools::wallet_public::wallet_transfer_info> transactions;
      std::list<gateway_balance_entry> balances;
      std::list<std::string> raw_txs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
        KV_SERIALIZE(status_error)               DOC_DSCR("Error description if the call failed.") DOC_END
        KV_SERIALIZE(total_transactions)         DOC_DSCR("Total number of transactions associated with the specified gateway address.") DOC_EXMP(100) DOC_END
        KV_SERIALIZE(transactions)               DOC_DSCR("List of transactions, decrypted server-side if a view secret key was provided, otherwise the public part only - decrypt locally from raw_txs.") DOC_EXMP_AUTO(1) DOC_END
        KV_SERIALIZE(balances)                   DOC_DSCR("List of balances for different asset_id associated with the gateway address at the time of the request.") DOC_EXMP_AUTO(1) DOC_END
        KV_SERIALIZE(raw_txs)                    DOC_DSCR("Hex-serialized raw transactions, aligned 1:1 with 'transactions', returned only when no view secret key was provided, for client-side decryption.") DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

}
