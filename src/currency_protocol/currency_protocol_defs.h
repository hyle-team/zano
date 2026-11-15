// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <list>
#include <memory>
#include "net/levin_base.h"
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
    bool compact = false;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(block)
      KV_SERIALIZE(txs)
      KV_SERIALIZE(coinbase_global_outs)
      KV_SERIALIZE(tx_global_outs)
      KV_SERIALIZE(compact)
    END_KV_SERIALIZE_MAP()
  };

  struct block_direct_data_entry
  {
    std::shared_ptr<const block_extended_info> block_ptr;
    std::shared_ptr<const transaction_chain_entry> coinbase_ptr;
    std::list<std::shared_ptr<const transaction_chain_entry> > txs_ptr;
    // wallet transport metadata; never serialized into the blockchain database
    bool compact = false;
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct NOTIFY_NEW_BLOCK
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 1;

    struct request
    {
      //keep existing p2p limits, 10k entries,array and sections
      inline static constexpr epee::serialization::portable_storage_limits local_portable_storage_limits{10000,10000,10000, LEVIN_DEFAULT_MAX_PACKET_SIZE};

      block_complete_entry b;
      uint64_t current_blockchain_height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(b)
        KV_SERIALIZE(current_blockchain_height)
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
      // tx count: CURRENCY_RELAY_TXS_MAX_COUNT = 5
      // tx size: CURRENCY_MAX_TRANSACTION_BLOB_SIZE = 122800 bytes
      // 10pb_h + 5*(4len + 122800) = 614036 * 2 = 1228072bytes
      inline static constexpr epee::serialization::portable_storage_limits local_portable_storage_limits{16,8,8, 1228072};

      std::list<blobdata>   txs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txs)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      // 10pb_h + 6field + 1str len + 13 ACCESS_DENIED = 30 * 10 = 300 bytes
      inline static constexpr epee::serialization::portable_storage_limits local_portable_storage_limits{4,2,2, 300};
  
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
      //10pb_h + 5tx_field + 2blob_sz + (500tx_hashes*32bytes) + 8blocks + 2blob_sz + (500blk_hashes * 32) = 32027 * 3 = 96081 bytes
      inline static constexpr epee::serialization::portable_storage_limits local_portable_storage_limits{8,4,4, 96081};

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

      //default limits, we cant calculate the exact size of entries, sections and arrays
      inline static constexpr epee::serialization::portable_storage_limits local_portable_storage_limits{10000, 10000, 10000, LEVIN_DEFAULT_MAX_PACKET_SIZE};
      std::list<blobdata>              txs;
      std::list<block_complete_entry>  blocks;
      std::list<crypto::hash>          missed_ids;
      uint64_t                         current_blockchain_height; // height of the top block + 1

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
    bool non_pruning_mode_enabled;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(current_height)
      KV_SERIALIZE_VAL_POD_AS_BLOB(top_id)
      KV_SERIALIZE(last_checkpoint_height)
      KV_SERIALIZE(core_time)
      KV_SERIALIZE(client_version)
      KV_SERIALIZE(non_pruning_mode_enabled)
    END_KV_SERIALIZE_MAP()
  };

  struct NOTIFY_REQUEST_CHAIN
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 6;

    struct request
    {
      // CURRENCY_NOTIFY_REQUEST_CHAIN_MAX_BLOCKS_COUNT = 100 hashes 32 bytes each
      // 10pb_h + 11bi + 2blob_len + 100*32 = 3223 * 3 = 9669 bytes
      inline static constexpr epee::serialization::portable_storage_limits local_portable_storage_limits{ 8, 4, 4, 9669 };

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
      // BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT = 2000
      // 10pb_h + 22sh + 22th + 13bi + 2000*57 = 114403 * 2 = 228806 bytes
      // entries 3+2 * 2000 = 4003, 2000arr elems, 2001sections - i add over by my imho
       inline static constexpr epee::serialization::portable_storage_limits local_portable_storage_limits{5000, 4000, 4000, 228806};
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
