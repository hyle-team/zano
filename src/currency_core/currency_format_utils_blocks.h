// Copyright (c) 2018-2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once


#include "include_base_utils.h"
#include "crypto/crypto.h"
#include "currency_core/currency_basic.h"
#include "currency_protocol/blobdatatype.h"

namespace currency
{
  blobdata get_block_hashing_blob(const block& b);
  bool get_block_hash(const block& b, crypto::hash& res);
  crypto::hash get_block_hash(const block& b);
  
  blobdata block_to_blob(const block& b);
  bool block_to_blob(const block& b, blobdata& b_blob);
  void get_tx_tree_hash(const std::vector<crypto::hash>& tx_hashes, crypto::hash& h);
  crypto::hash get_tx_tree_hash(const std::vector<crypto::hash>& tx_hashes);
  crypto::hash get_tx_tree_hash(const block& b);

  // Strict opt-in bounded form of get_block_hash() (read-only block transport
  // only; the functions above are unchanged). The caller supplies the miner
  // transaction hash (get_object_hash_in_buffer of its prefix), tx_ids with
  // capacity >= tx_hashes.size() + 1, tree scratch with size() >=
  // crypto::tree_hash_scratch_count(tx_hashes.size() + 1) (checked tree hash;
  // no internal allocation), and fixed hash scratch of at least
  // BLOCK_HASHING_BLOB_MAX_BYTES + 2 (the blob plus its encoded varint
  // length, which is hashed too); nothing here grows. Blobs longer than
  // BLOCK_HASHING_BLOB_MAX_BYTES are refused. Same hash as get_block_hash().
  // On false, res is unchanged and must not be used.
  constexpr size_t BLOCK_HASHING_BLOB_MAX_BYTES = 128;
  static_assert(BLOCK_HASHING_BLOB_MAX_BYTES < (size_t(1) << 14), "varint(blob length) must fit in two bytes");
  bool get_block_hash_in_buffer(const block& b, const crypto::hash& miner_tx_hash,
    std::vector<crypto::hash>& tx_ids, std::vector<crypto::hash>& tree_scratch,
    std::vector<char>& buffer, crypto::hash& res);
}
