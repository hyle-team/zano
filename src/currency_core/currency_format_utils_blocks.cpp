// Copyright (c) 2018-2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.



#include <limits>

#include "currency_format_utils_blocks.h"
#include "serialization/serialization.h"
#include "currency_format_utils.h"
#include "currency_format_utils_abstract.h"
#include "currency_format_utils_transactions.h"
namespace currency
{
  //---------------------------------------------------------------
  blobdata get_block_hashing_blob(const block& b)
  {
    blobdata blob = t_serializable_object_to_blob(static_cast<block_header>(b));
    crypto::hash tree_root_hash = get_tx_tree_hash(b);
    blob.append((const char*)&tree_root_hash, sizeof(tree_root_hash));
    blob.append(tools::get_varint_data(b.tx_hashes.size() + 1));
    return blob;
  }
  //---------------------------------------------------------------
  bool get_block_hash(const block& b, crypto::hash& res)
  {
    return get_object_hash(get_block_hashing_blob(b), res);
  }
  //---------------------------------------------------------------
  crypto::hash get_block_hash(const block& b)
  {
    crypto::hash p = null_hash;
    get_block_hash(b, p);
    return p;
  }
  //---------------------------------------------------------------
  blobdata block_to_blob(const block& b)
  {
    return t_serializable_object_to_blob(b);
  }
  //---------------------------------------------------------------
  bool block_to_blob(const block& b, blobdata& b_blob)
  {
    return t_serializable_object_to_blob(b, b_blob);
  }
  //---------------------------------------------------------------
  void get_tx_tree_hash(const std::vector<crypto::hash>& tx_hashes, crypto::hash& h)
  {
    tree_hash(tx_hashes.data(), tx_hashes.size(), h);
  }
  //---------------------------------------------------------------
  crypto::hash get_tx_tree_hash(const std::vector<crypto::hash>& tx_hashes)
  {
    crypto::hash h = null_hash;
    get_tx_tree_hash(tx_hashes, h);
    return h;
  }
  //---------------------------------------------------------------
  crypto::hash get_tx_tree_hash(const block& b)
  {
    std::vector<crypto::hash> txs_ids;
    crypto::hash h = null_hash;
    get_transaction_hash(b.miner_tx, h);
    txs_ids.push_back(h);
    BOOST_FOREACH(auto& th, b.tx_hashes)
      txs_ids.push_back(th);
    return get_tx_tree_hash(txs_ids);
  }
  //---------------------------------------------------------------
  bool get_block_hash_in_buffer(const block& b, const crypto::hash& miner_tx_hash,
    std::vector<crypto::hash>& tx_ids, std::vector<crypto::hash>& tree_scratch,
    std::vector<char>& buffer, crypto::hash& res)
  {
    // Same layout as get_block_hashing_blob(): header encoding, tree root of
    // (miner tx hash, tx_hashes...), varint(count); then, as get_block_hash()
    // does through get_object_hash(blobdata), the hash of that blob's encoding.
    if (b.tx_hashes.size() >= tx_ids.capacity() || buffer.size() < BLOCK_HASHING_BLOB_MAX_BYTES + 2)
      return false; // Would grow tx_ids, or the scratch cannot hold varint(length) + blob.
    const size_t ids = b.tx_hashes.size() + 1; // < capacity, cannot overflow.
    if (tree_scratch.size() < crypto::tree_hash_scratch_count(ids))
      return false;
    tx_ids.clear();
    tx_ids.push_back(miner_tx_hash);
    tx_ids.insert(tx_ids.end(), b.tx_hashes.begin(), b.tx_hashes.end());
    crypto::hash tree_root_hash = null_hash;
    if (!crypto::tree_hash_checked(tx_ids.data(), tx_ids.size(), tree_scratch.data(), tree_scratch.size(), tree_root_hash))
      return false; // The root is not used after a refusal.

    const block_header header = b;
    size_t header_size = 0;
    if (!serialize_object_to_buffer(header, buffer, header_size))
      return false;
    // Same bytes as tools::get_varint_data(ids), without the stringstream.
    char count[(std::numeric_limits<size_t>::digits + 6) / 7];
    char* count_end = count;
    tools::write_varint(count_end, ids);
    const size_t count_size = static_cast<size_t>(count_end - count);
    if (header_size + sizeof(tree_root_hash) + count_size > BLOCK_HASHING_BLOB_MAX_BYTES)
      return false;
    blobdata blob;
    blob.reserve(BLOCK_HASHING_BLOB_MAX_BYTES);
    blob.assign(buffer.data(), header_size);
    blob.append((const char*)&tree_root_hash, sizeof(tree_root_hash));
    blob.append(count, count_size);
    return get_object_hash_in_buffer(blob, buffer, res);
  }

}
