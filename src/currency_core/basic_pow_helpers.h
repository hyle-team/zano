// Copyright (c) 2014-2018 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <typeindex>
#include <unordered_set>
#include <unordered_map>

#include "account.h"
#include "include_base_utils.h"

#include "currency_format_utils_abstract.h"
#include "common/crypto_stream_operators.h"
#include "currency_protocol/currency_protocol_defs.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "difficulty.h"
//#include "offers_services_helpers.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "bc_payments_id_service.h"
#include "bc_attachments_helpers_basic.h"
#include "blockchain_storage_basic.h"

#define CURRENCY_MINER_BLOCK_BLOB_NONCE_OFFSET    1

namespace currency
{
  int ethash_height_to_epoch(uint64_t height);
  crypto::hash ethash_epoch_to_seed(int epoch);
  crypto::hash get_block_header_mining_hash(const block& b);
  crypto::hash get_block_longhash(uint64_t h, const crypto::hash& block_header_hash, uint64_t nonce);
  void get_block_longhash(const block& b, crypto::hash& res);
  crypto::hash get_block_longhash(const block& b);

  inline uint64_t& access_nonce_in_block_blob(blobdata& bd)
  {
    return *reinterpret_cast<uint64_t*>(&bd[CURRENCY_MINER_BLOCK_BLOB_NONCE_OFFSET]);
  }

  inline const uint64_t& access_nonce_in_block_blob(const blobdata& bd)
  {
    return *reinterpret_cast<const uint64_t*>(&bd[CURRENCY_MINER_BLOCK_BLOB_NONCE_OFFSET]);
  }
}