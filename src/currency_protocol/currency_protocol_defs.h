// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <list>
#include <memory>
#include "serialization/keyvalue_serialization.h"
#include "currency_core/currency_basic.h"
#include "currency_core/connection_context.h"
#include "currency_core/blockchain_storage_basic.h"
#include "currency_protocol/blobdatatype.h"
#include "currency_core/basic_kv_structs.h"

namespace currency
{


#define BC_COMMANDS_POOL_BASE 2000

  
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct block_complete_entry
  {
    blobdata block;
    std::list<blobdata> txs;
    std::vector<uint64_t> coinbase_global_outs;
    std::vector<struct_with_one_t_type<std::vector<uint64_t> > > tx_global_outs;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(block)
      KV_SERIALIZE(txs)
      KV_SERIALIZE(coinbase_global_outs)
      KV_SERIALIZE(tx_global_outs)
    END_KV_SERIALIZE_MAP()
  };

  struct block_direct_data_entry
  {
    std::shared_ptr<const block_extended_info> block_ptr;
    std::shared_ptr<const transaction_chain_entry> coinbase_ptr;
    std::list<std::shared_ptr<const transaction_chain_entry> > txs_ptr;
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct NOTIFY_NEW_BLOCK
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 1;

    struct request
    {
      block_complete_entry b;
      uint64_t current_blockchain_height;
      uint32_t hop;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(b)
        KV_SERIALIZE(current_blockchain_height)
        KV_SERIALIZE(hop)
      END_KV_SERIALIZE_MAP()
    };
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct NOTIFY_OR_INVOKE_NEW_TRANSACTIONS
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 2;

    struct request
    {
      std::list<blobdata>   txs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txs)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string code;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(code)
      END_KV_SERIALIZE_MAP()
    };

  };
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct NOTIFY_REQUEST_GET_OBJECTS
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 3;

    struct request
    {
      std::list<crypto::hash>    txs;
      std::list<crypto::hash>    blocks;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(txs)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(blocks)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct NOTIFY_RESPONSE_GET_OBJECTS
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 4;

    struct request
    {
      std::list<blobdata>              txs;
      std::list<block_complete_entry>  blocks;
      std::list<crypto::hash>          missed_ids;
      uint64_t                         current_blockchain_height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txs)
        KV_SERIALIZE(blocks)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(missed_ids)
        KV_SERIALIZE(current_blockchain_height)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct CORE_SYNC_DATA
  {
    uint64_t current_height;          // height of the top block + 1
    crypto::hash  top_id;
    uint64_t last_checkpoint_height;
    uint64_t core_time;
    std::string client_version;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(current_height)
      KV_SERIALIZE_VAL_POD_AS_BLOB(top_id)
      KV_SERIALIZE(last_checkpoint_height)
      KV_SERIALIZE(core_time)
      KV_SERIALIZE(client_version)
    END_KV_SERIALIZE_MAP()
  };

  struct NOTIFY_REQUEST_CHAIN
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 6;

    struct request
    {
      std::list<crypto::hash> block_ids; /*IDs of the first 10 blocks are sequential, next goes with pow(2,n) offset, like 2, 4, 8, 16, 32, 64 and so on, and the last one is always genesis block */

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(block_ids)
      END_KV_SERIALIZE_MAP()
    };
  };

  

  struct NOTIFY_RESPONSE_CHAIN_ENTRY
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 7;

    struct request
    {
      uint64_t start_height;
      uint64_t total_height;
      std::list<block_context_info> m_block_ids;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(start_height)
        KV_SERIALIZE(total_height)
        KV_SERIALIZE(m_block_ids)
      END_KV_SERIALIZE_MAP()
    };
  };

}

#include "currency_protocol_defs_print.h"
