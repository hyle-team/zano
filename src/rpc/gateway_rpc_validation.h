// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include "currency_core/currency_basic.h"
#include "currency_protocol/blobdatatype.h"

namespace currency
{
  class blockchain_storage;

  namespace gateway_rpc_validation
  {
    bool verify_owner_signature(const crypto::hash& hash, const gateway_owner_key_v& owner_key, const gateway_owner_signature_v& signature);
    bool validate_input_signatures(const transaction& tx, const crypto::hash& tx_id, const blockchain_storage& blockchain, gateway_address_id_type& gateway_id, gateway_owner_key_v& owner_key, std::string& error);
    bool prevalidate_transaction(const transaction& tx, const blobdata& tx_blob, const crypto::hash& tx_id, const blockchain_storage& blockchain, std::string& error);
    bool prevalidate_signed_transaction(const transaction& tx, const blobdata& tx_blob, const crypto::hash& tx_id, const blockchain_storage& blockchain, std::string& error);
  }
}
