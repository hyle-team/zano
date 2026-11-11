// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <unordered_set>
#include "crypto/crypto.h"
#include "currency_core/currency_core.h"

struct core_state_helper
{
  std::unordered_set<crypto::hash> blocks_hashes;
  std::unordered_set<crypto::hash> txs_hashes;
  std::unordered_set<crypto::hash> pool_txs_hashes;

  void clear()
  {
    blocks_hashes.clear();
    txs_hashes.clear();
    pool_txs_hashes.clear();
  }

  bool fill(currency::core& c)
  {
    clear();
    std::list<currency::block> blocks;
    std::list<currency::transaction> transactions;

    currency::blockchain_storage& bs = c.get_blockchain_storage();
    bool r = bs.get_blocks(0, bs.get_current_blockchain_size(), blocks, transactions);
    CHECK_AND_ASSERT_MES(r && blocks.size() == bs.get_current_blockchain_size() && transactions.size() == bs.get_total_transactions() - bs.get_current_blockchain_size(), false, "get_blocks failed");

    for (auto& b : blocks)
    {
      r = blocks_hashes.insert(currency::get_block_hash(b)).second;
      CHECK_AND_ASSERT_MES(r, false, "Internal error: block with hash " << currency::get_block_hash(b) << " has already been added");
    }

    for (auto& t : transactions)
    {
      r = txs_hashes.insert(currency::get_transaction_hash(t)).second;
      CHECK_AND_ASSERT_MES(r, false, "Internal error: transaction with hash " << currency::get_transaction_hash(t) << " has already been added");
    }

    transactions.clear();
    r = c.get_tx_pool().get_transactions(transactions);
    CHECK_AND_ASSERT_MES(r, false, "get_transactions failed");
    for (auto& t : transactions)
    {
      r = pool_txs_hashes.insert(currency::get_transaction_hash(t)).second;
      CHECK_AND_ASSERT_MES(r, false, "Internal error: pool transaction with hash " << currency::get_transaction_hash(t) << " has already been added");
    }

    return true;
  }

  bool operator==(const core_state_helper& rhs) const
  {
    CHECK_AND_ASSERT_MES(blocks_hashes.size() == rhs.blocks_hashes.size(), false, "core_state_helper NOT EQUAL in blocks_hashes: " << blocks_hashes.size() << " vs " << rhs.blocks_hashes.size());
    CHECK_AND_ASSERT_MES(txs_hashes.size() == rhs.txs_hashes.size(), false, "core_state_helper NOT EQUAL in txs_hashes: " << txs_hashes.size() << " vs " << rhs.txs_hashes.size());
    CHECK_AND_ASSERT_MES(pool_txs_hashes.size() == rhs.pool_txs_hashes.size(), false, "core_state_helper NOT EQUAL in pool_txs_hashes: " << pool_txs_hashes.size() << " vs " << rhs.pool_txs_hashes.size());

    for (auto& bh : blocks_hashes)
    {
      CHECK_AND_ASSERT_MES(rhs.blocks_hashes.count(bh) == 1, false, "blockchain_state NOT EQUAL in blocks_hashes : " << bh);
    }

    for (auto& th : txs_hashes)
    {
      CHECK_AND_ASSERT_MES(rhs.txs_hashes.count(th) == 1, false, "blockchain_state NOT EQUAL in txs_hashes: " << th);
    }

    for (auto& th : pool_txs_hashes)
    {
      CHECK_AND_ASSERT_MES(rhs.pool_txs_hashes.count(th) == 1, false, "core_state_helper NOT EQUAL in pool_txs_hashes: " << th);
    }

    return true;
  }

  bool operator!=(const core_state_helper& rhs) const
  {
    return !operator==(rhs);
  }
};



