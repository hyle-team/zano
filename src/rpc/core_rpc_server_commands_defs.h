// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "serialization/keyvalue_hexemizer.h"
#include "currency_protocol/currency_protocol_defs.h"
#include "currency_core/currency_basic.h"
#include "currency_core/difficulty.h"
#include "crypto/hash.h"
#include "p2p/p2p_protocol_defs.h"
#include "storages/portable_storage_base.h"
#include "currency_core/offers_service_basics.h"
#include "currency_core/basic_api_response_codes.h"
#include "common/error_codes.h"
namespace currency
{
  //-----------------------------------------------

  struct alias_rpc_details_base
  {
    std::string address;
    std::string tracking_key;
    std::string comment;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(address)
      KV_SERIALIZE(tracking_key)
      KV_SERIALIZE(comment)
    END_KV_SERIALIZE_MAP()
  };

  struct alias_rpc_details
  {
    std::string alias;
    alias_rpc_details_base details;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(alias)
      KV_CHAIN_MAP(details)
    END_KV_SERIALIZE_MAP()
  };


  struct update_alias_rpc_details 
  {
    std::string old_address;
    std::string alias;
    alias_rpc_details_base details;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(old_address)
      KV_SERIALIZE(alias)
      KV_SERIALIZE(details)
    END_KV_SERIALIZE_MAP()
  };




  struct COMMAND_RPC_GET_POOL_INFO
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::string error_code;
      std::list<currency::alias_rpc_details> aliases_que;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(error_code)
        KV_SERIALIZE(aliases_que)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_VOTES
  {
    struct request
    {
      uint64_t h_start;
      uint64_t h_end;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(h_start)
        KV_SERIALIZE(h_end)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::string error_code;
      vote_results votes;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(error_code)
        KV_SERIALIZE(votes)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct asset_id_kv
  {
    crypto::public_key asset_id;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_POD_AS_HEX_STRING(asset_id)
    END_KV_SERIALIZE_MAP()
  };


  struct COMMAND_RPC_GET_ASSET_INFO
  {
    typedef asset_id_kv request;

    struct response
    {
      std::string status;
      asset_descriptor_base asset_descriptor;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(asset_descriptor)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_HEIGHT
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t 	 height;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  template<class t_block_complete_entry>
  struct COMMAND_RPC_GET_BLOCKS_FAST_T
  {

    struct request
    {
      uint64_t minimum_height;
      std::list<crypto::hash> block_ids; //*first 10 blocks id goes sequential, next goes in pow(2,n) offset, like 2, 4, 8, 16, 32, 64 and so on, and the last one is always genesis block */

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(minimum_height)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(block_ids)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<t_block_complete_entry> blocks;
      uint64_t    start_height;
      uint64_t    current_height;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(blocks)
        KV_SERIALIZE(start_height)
        KV_SERIALIZE(current_height)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  typedef COMMAND_RPC_GET_BLOCKS_FAST_T<block_complete_entry> COMMAND_RPC_GET_BLOCKS_FAST;
  typedef COMMAND_RPC_GET_BLOCKS_FAST_T<block_direct_data_entry> COMMAND_RPC_GET_BLOCKS_DIRECT;
  
  //-----------------------------------------------
  struct COMMAND_RPC_GET_TRANSACTIONS
  {
    struct request
    {
      std::list<std::string> txs_hashes;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txs_hashes)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::list<std::string> txs_as_hex;  //transactions blobs as hex
      std::list<std::string> missed_tx;   //not found transactions
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txs_as_hex)
        KV_SERIALIZE(missed_tx)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------
  struct COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE
  {
    struct request
    {
      uint64_t timestamp;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(timestamp)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t h;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(h)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_GET_TX_POOL
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<blobdata> txs;  //transactions blobs
      uint64_t tx_expiration_ts_median;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txs)
        KV_SERIALIZE(tx_expiration_ts_median)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_CHECK_KEYIMAGES
  {
    struct request
    {
      std::list<crypto::key_image> images;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(images)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<uint64_t> images_stat;  //true - unspent, false - spent
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(images_stat)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES
  {
    struct request
    {
      std::list<crypto::hash> txids;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(txids)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::vector<struct_with_one_t_type<std::vector<uint64_t> > > tx_global_outs;
      //std::vector<uint64_t> o_indexes;
      std::string status;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_global_outs)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES_BY_AMOUNT
  {
    struct request
    {
      uint64_t amount;
      uint64_t i;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amount)
        KV_SERIALIZE(i)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string status;
      crypto::hash tx_id;
      uint64_t out_no;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(tx_id)
        KV_SERIALIZE(out_no)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_RPC_GET_MULTISIG_INFO
  {
    struct request
    {
      crypto::hash ms_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(ms_id)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string status;
      crypto::hash tx_id;
      uint64_t out_no;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(tx_id)
        KV_SERIALIZE(out_no)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS
  {
    struct request
    {
      std::list<uint64_t> amounts;
      uint64_t            decoys_count;       // how many decoy outputs needed (per amount)
      uint64_t            height_upper_limit; // if nonzero, all the decoy outputs must be either older than, or the same age as this height
      bool                use_forced_mix_outs;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amounts)
        KV_SERIALIZE(decoys_count)
        KV_SERIALIZE(height_upper_limit)
        KV_SERIALIZE(use_forced_mix_outs)
      END_KV_SERIALIZE_MAP()
    };


#define RANDOM_OUTPUTS_FOR_AMOUNTS_FLAGS_COINBASE                       0x0000000000000001LL 
#define RANDOM_OUTPUTS_FOR_AMOUNTS_FLAGS_NOT_ALLOWED                    0x0000000000000002LL 
#define RANDOM_OUTPUTS_FOR_AMOUNTS_FLAGS_POS_COINBASE                   0x0000000000000004LL 

#pragma pack (push, 1)
    struct out_entry
    {
      out_entry() = default;
      out_entry(uint64_t global_amount_index, const crypto::public_key& stealth_address)
        : global_amount_index(global_amount_index), stealth_address(stealth_address), concealing_point{}, amount_commitment{}, blinded_asset_id{}
      {}
      out_entry(uint64_t global_amount_index, const crypto::public_key& stealth_address, const crypto::public_key& amount_commitment, const crypto::public_key& concealing_point, const crypto::public_key& blinded_asset_id)
        : global_amount_index(global_amount_index), stealth_address(stealth_address), concealing_point(concealing_point), amount_commitment(amount_commitment), blinded_asset_id(blinded_asset_id)
      {}
      uint64_t global_amount_index;
      crypto::public_key stealth_address;
      crypto::public_key concealing_point;  // premultiplied by 1/8
      crypto::public_key amount_commitment; // premultiplied by 1/8
      crypto::public_key blinded_asset_id;  // premultiplied by 1/8
      uint64_t flags;
    };
#pragma pack(pop)

    struct outs_for_amount
    {
      uint64_t amount = 0;
      std::list<out_entry> outs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amount)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(outs)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<outs_for_amount> outs;
      std::string status;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(outs)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3
  {
    struct offsets_distribution
    {
      uint64_t amount; //if amount is 0 then lookup in post-zarcanum zone only, if not 0 then pre-zarcanum only
      std::vector<uint64_t> global_offsets; //[i] = global_index to pick up

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amount)
        KV_SERIALIZE(global_offsets)
      END_KV_SERIALIZE_MAP()
    };


    struct request
    {
      std::vector<offsets_distribution> amounts;
      uint64_t            height_upper_limit; // if nonzero, all the decoy outputs must be either older than, or the same age as this height
      bool                use_forced_mix_outs;
      uint64_t            coinbase_percents;     //from 0 to 100, estimate percents of coinbase outputs included in decoy sets  
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amounts)
        KV_SERIALIZE(height_upper_limit)
        KV_SERIALIZE(use_forced_mix_outs)
        KV_SERIALIZE(coinbase_percents)
      END_KV_SERIALIZE_MAP()
    };

    typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response response;
  };

    struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_LEGACY
  {
    struct request
    {
      std::list<uint64_t> amounts;
      uint64_t outs_count;
      bool use_forced_mix_outs;
      BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amounts)
      KV_SERIALIZE(outs_count)
      KV_SERIALIZE(use_forced_mix_outs)
      END_KV_SERIALIZE_MAP()
    };

#pragma pack(push, 1)
    struct out_entry
    {
      uint64_t global_amount_index;
      crypto::public_key out_key;
    };
#pragma pack(pop)

    struct outs_for_amount
    {
      uint64_t amount;
      std::list<out_entry> outs;

      BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE_CONTAINER_POD_AS_BLOB(outs)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<outs_for_amount> outs;
      std::string status;
      BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(outs)
      KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };



  //-----------------------------------------------
  struct COMMAND_RPC_SET_MAINTAINERS_INFO
  {
    typedef nodetool::maintainers_entry request;

    struct response
    {
      std::string status;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------
	struct COMMAND_RPC_SEND_RAW_TX
	{
		struct request
		{
			std::string tx_as_hex;

			request() {}
			explicit request(const transaction &);

			BEGIN_KV_SERIALIZE_MAP()
				KV_SERIALIZE(tx_as_hex)
			END_KV_SERIALIZE_MAP()
		};


		struct response
		{
			std::string status;

			BEGIN_KV_SERIALIZE_MAP()
				KV_SERIALIZE(status)
			END_KV_SERIALIZE_MAP()
		};
	};


  struct COMMAND_RPC_FORCE_RELAY_RAW_TXS
  {
    struct request
    {
      std::vector<std::string> txs_as_hex;

      BEGIN_KV_SERIALIZE_MAP()
				KV_SERIALIZE(txs_as_hex)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_START_MINING
  {
    struct request
    {
      std::string miner_address;
      uint64_t    threads_count;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(miner_address)
        KV_SERIALIZE(threads_count)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  struct outs_index_stat
  {
    uint64_t amount_0_001;
    uint64_t amount_0_01;
    uint64_t amount_0_1;
    uint64_t amount_1;
    uint64_t amount_10;
    uint64_t amount_100;
    uint64_t amount_1000;
    uint64_t amount_10000;
    uint64_t amount_100000;
    uint64_t amount_1000000;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount_0_001)
      KV_SERIALIZE(amount_0_01)
      KV_SERIALIZE(amount_0_1)
      KV_SERIALIZE(amount_1)
      KV_SERIALIZE(amount_10)
      KV_SERIALIZE(amount_100)
      KV_SERIALIZE(amount_1000)
      KV_SERIALIZE(amount_10000)
      KV_SERIALIZE(amount_100000)
      KV_SERIALIZE(amount_1000000)
    END_KV_SERIALIZE_MAP()
  };
  //-----------------------------------------------

  struct bc_performance_data
  {
    //block processing zone
    uint64_t block_processing_time_0;
    uint64_t block_processing_time_1;
    uint64_t target_calculating_time_2;
    uint64_t longhash_calculating_time_3;
    uint64_t all_txs_insert_time_5;
    uint64_t etc_stuff_6;
    uint64_t insert_time_4;
    uint64_t raise_block_core_event;
    uint64_t target_calculating_enum_blocks;
    uint64_t target_calculating_calc;

    //tx processing zone
    uint64_t tx_check_inputs_time;
    uint64_t tx_add_one_tx_time;
    uint64_t tx_process_extra;
    uint64_t tx_process_attachment;
    uint64_t tx_process_inputs;
    uint64_t tx_push_global_index;
    uint64_t tx_check_exist;  
    uint64_t tx_append_time;
    uint64_t tx_append_rl_wait;
    uint64_t tx_append_is_expired;
    uint64_t tx_print_log;
    uint64_t tx_prapare_append;

    uint64_t tx_mixin_count;


    uint64_t tx_store_db;
    
    uint64_t tx_check_inputs_prefix_hash;
    uint64_t tx_check_inputs_attachment_check;
    uint64_t tx_check_inputs_loop;
    uint64_t tx_check_inputs_loop_kimage_check;
    uint64_t tx_check_inputs_loop_ch_in_val_sig;
    uint64_t tx_check_inputs_loop_scan_outputkeys_get_item_size;
    uint64_t tx_check_inputs_loop_scan_outputkeys_relative_to_absolute;
    uint64_t tx_check_inputs_loop_scan_outputkeys_loop;
    uint64_t tx_check_inputs_loop_scan_outputkeys_loop_get_subitem;
    uint64_t tx_check_inputs_loop_scan_outputkeys_loop_find_tx;
    uint64_t tx_check_inputs_loop_scan_outputkeys_loop_handle_output;


    //db performance data
    uint64_t map_size;
    uint64_t tx_count;
    uint64_t writer_tx_count;

   
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(block_processing_time_0)
      KV_SERIALIZE(block_processing_time_1)
      KV_SERIALIZE(target_calculating_time_2)
      KV_SERIALIZE(longhash_calculating_time_3)
      KV_SERIALIZE(all_txs_insert_time_5)
      KV_SERIALIZE(etc_stuff_6)
      KV_SERIALIZE(insert_time_4)
      KV_SERIALIZE(raise_block_core_event)
      KV_SERIALIZE(target_calculating_enum_blocks)
      KV_SERIALIZE(target_calculating_calc)
      //tx processing zone
      KV_SERIALIZE(tx_check_inputs_time)
      KV_SERIALIZE(tx_add_one_tx_time)
      KV_SERIALIZE(tx_process_extra)
      KV_SERIALIZE(tx_process_attachment)
      KV_SERIALIZE(tx_process_inputs)
      KV_SERIALIZE(tx_push_global_index)
      KV_SERIALIZE(tx_check_exist)
      KV_SERIALIZE(tx_append_time)
      KV_SERIALIZE(tx_append_rl_wait)
      KV_SERIALIZE(tx_append_is_expired)
      KV_SERIALIZE(tx_store_db)
      KV_SERIALIZE(tx_print_log)
      KV_SERIALIZE(tx_prapare_append)
      KV_SERIALIZE(tx_mixin_count)      

      KV_SERIALIZE(tx_check_inputs_prefix_hash)
      KV_SERIALIZE(tx_check_inputs_attachment_check)
      KV_SERIALIZE(tx_check_inputs_loop)
      KV_SERIALIZE(tx_check_inputs_loop_kimage_check)
      KV_SERIALIZE(tx_check_inputs_loop_ch_in_val_sig)
      KV_SERIALIZE(tx_check_inputs_loop_scan_outputkeys_get_item_size)
      KV_SERIALIZE(tx_check_inputs_loop_scan_outputkeys_relative_to_absolute)
      KV_SERIALIZE(tx_check_inputs_loop_scan_outputkeys_loop)
      KV_SERIALIZE(tx_check_inputs_loop_scan_outputkeys_loop_get_subitem)
      KV_SERIALIZE(tx_check_inputs_loop_scan_outputkeys_loop_find_tx)
      KV_SERIALIZE(tx_check_inputs_loop_scan_outputkeys_loop_handle_output)

      //db performace data
      KV_SERIALIZE(map_size)
      KV_SERIALIZE(tx_count)
      KV_SERIALIZE(writer_tx_count)
    END_KV_SERIALIZE_MAP()
  };

  struct pool_performance_data
  {
    uint64_t tx_processing_time;
    uint64_t check_inputs_types_supported_time;
    uint64_t expiration_validate_time;
    uint64_t validate_amount_time;
    uint64_t validate_alias_time;
    uint64_t check_keyimages_ws_ms_time;
    uint64_t check_inputs_time;
    uint64_t begin_tx_time;
    uint64_t update_db_time;
    uint64_t db_commit_time;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(tx_processing_time)
      KV_SERIALIZE(check_inputs_types_supported_time)
      KV_SERIALIZE(expiration_validate_time)
      KV_SERIALIZE(validate_amount_time)
      KV_SERIALIZE(validate_alias_time)
      KV_SERIALIZE(check_keyimages_ws_ms_time)
      KV_SERIALIZE(check_inputs_time)
      KV_SERIALIZE(begin_tx_time)
      KV_SERIALIZE(update_db_time)
      KV_SERIALIZE(db_commit_time)
    END_KV_SERIALIZE_MAP()
  };


  //-----------------------------------------------

#define COMMAND_RPC_GET_INFO_FLAG_POS_DIFFICULTY                 0x0000000000000001LL
#define COMMAND_RPC_GET_INFO_FLAG_POW_DIFFICULTY                 0x0000000000000002LL
#define COMMAND_RPC_GET_INFO_FLAG_NET_TIME_DELTA_MEDIAN          0x0000000000000004LL
#define COMMAND_RPC_GET_INFO_FLAG_CURRENT_NETWORK_HASHRATE_50    0x0000000000000008LL
#define COMMAND_RPC_GET_INFO_FLAG_CURRENT_NETWORK_HASHRATE_350   0x0000000000000010LL
#define COMMAND_RPC_GET_INFO_FLAG_SECONDS_FOR_10_BLOCKS          0x0000000000000020LL
#define COMMAND_RPC_GET_INFO_FLAG_SECONDS_FOR_30_BLOCKS          0x0000000000000040LL
#define COMMAND_RPC_GET_INFO_FLAG_TRANSACTIONS_DAILY_STAT        0x0000000000000080LL
#define COMMAND_RPC_GET_INFO_FLAG_LAST_POS_TIMESTAMP             0x0000000000000100LL
#define COMMAND_RPC_GET_INFO_FLAG_LAST_POW_TIMESTAMP             0x0000000000000200LL
#define COMMAND_RPC_GET_INFO_FLAG_TOTAL_COINS                    0x0000000000000400LL
#define COMMAND_RPC_GET_INFO_FLAG_LAST_BLOCK_SIZE                0x0000000000000800LL
#define COMMAND_RPC_GET_INFO_FLAG_TX_COUNT_IN_LAST_BLOCK         0x0000000000001000LL
#define COMMAND_RPC_GET_INFO_FLAG_POS_SEQUENCE_FACTOR            0x0000000000002000LL
#define COMMAND_RPC_GET_INFO_FLAG_POW_SEQUENCE_FACTOR            0x0000000000004000LL
#define COMMAND_RPC_GET_INFO_FLAG_OUTS_STAT                      0x0000000000008000LL
#define COMMAND_RPC_GET_INFO_FLAG_PERFORMANCE                    0x0000000000010000LL
#define COMMAND_RPC_GET_INFO_FLAG_POS_BLOCK_TS_SHIFT_VS_ACTUAL   0x0000000000020000LL
#define COMMAND_RPC_GET_INFO_FLAG_MARKET                         0x0000000000040000LL
#define COMMAND_RPC_GET_INFO_FLAG_EXPIRATIONS_MEDIAN             0x0000000000080000LL

#define COMMAND_RPC_GET_INFO_FLAG_ALL_FLAGS                      0xffffffffffffffffLL


  struct COMMAND_RPC_GET_INFO
  {
    struct request
    {
      uint64_t flags;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(flags)
      END_KV_SERIALIZE_MAP()
    };

    enum
    {
      daemon_network_state_connecting = 0,
      daemon_network_state_synchronizing = 1,
      daemon_network_state_online = 2,
      daemon_network_state_loading_core = 3,
      daemon_network_state_internal_error = 4,
      daemon_network_state_unloading_core = 5,
      daemon_network_state_downloading_database = 6
    };

    struct response
    {
      std::string status;
      uint64_t height;
      std::string pos_difficulty;
      uint64_t pow_difficulty;
      uint64_t tx_count;
      uint64_t tx_pool_size;
      uint64_t alt_blocks_count;
      uint64_t outgoing_connections_count;
      uint64_t incoming_connections_count;
      uint64_t synchronized_connections_count;
      int64_t  net_time_delta_median;
      uint64_t white_peerlist_size;
      uint64_t grey_peerlist_size;
      uint64_t current_blocks_median;
      uint64_t current_network_hashrate_50;
      uint64_t current_network_hashrate_350;
      uint64_t seconds_for_10_blocks;
      uint64_t seconds_for_30_blocks;
      uint64_t alias_count;
      uint64_t daemon_network_state;
      uint64_t synchronization_start_height;
      uint64_t max_net_seen_height;
      uint64_t transactions_cnt_per_day;
      uint64_t transactions_volume_per_day;
      nodetool::maintainers_info_external mi;
      uint64_t pos_sequence_factor; 
      uint64_t pow_sequence_factor;
      uint64_t last_pow_timestamp;
      uint64_t last_pos_timestamp;
      std::string total_coins;
      uint64_t block_reward;
      uint64_t last_block_total_reward;
      uint64_t pos_diff_total_coins_rate;
      int64_t pos_block_ts_shift_vs_actual;
      uint64_t expiration_median_timestamp;
      outs_index_stat outs_stat;
      bc_performance_data performance_data;
      pool_performance_data tx_pool_performance_data;
      bool pos_allowed;
      uint64_t last_block_size;
      uint64_t current_max_allowed_block_size;
      uint64_t tx_count_in_last_block;
      uint64_t default_fee;
      uint64_t minimum_fee;
      uint64_t last_block_timestamp;
      std::string last_block_hash;
      std::list<bool> is_hardfok_active;
      //market
      uint64_t offers_count;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(height)
        KV_SERIALIZE(pos_difficulty)
        KV_SERIALIZE(pow_difficulty)
        KV_SERIALIZE(tx_count)
        KV_SERIALIZE(tx_pool_size)
        KV_SERIALIZE(alt_blocks_count)
        KV_SERIALIZE(outgoing_connections_count)
        KV_SERIALIZE(incoming_connections_count)
        KV_SERIALIZE(synchronized_connections_count)
        KV_SERIALIZE(net_time_delta_median)
        KV_SERIALIZE(white_peerlist_size)
        KV_SERIALIZE(grey_peerlist_size)
        KV_SERIALIZE(current_blocks_median)
        KV_SERIALIZE(current_network_hashrate_50)
        KV_SERIALIZE(current_network_hashrate_350)
        KV_SERIALIZE(alias_count)
        KV_SERIALIZE(daemon_network_state)
        KV_SERIALIZE(synchronization_start_height)
        KV_SERIALIZE(max_net_seen_height)
        KV_SERIALIZE(transactions_cnt_per_day)
        KV_SERIALIZE(transactions_volume_per_day)
        KV_SERIALIZE(mi)
        KV_SERIALIZE(pos_sequence_factor)
        KV_SERIALIZE(pow_sequence_factor)
        KV_SERIALIZE(last_pow_timestamp)
        KV_SERIALIZE(last_pos_timestamp)
        KV_SERIALIZE(seconds_for_10_blocks)
        KV_SERIALIZE(seconds_for_30_blocks)
        KV_SERIALIZE(total_coins)
        KV_SERIALIZE(block_reward)
        KV_SERIALIZE(last_block_total_reward)
        KV_SERIALIZE(pos_diff_total_coins_rate)
        KV_SERIALIZE(pos_block_ts_shift_vs_actual)
        KV_SERIALIZE(expiration_median_timestamp)
        KV_SERIALIZE(pos_allowed)
        KV_SERIALIZE(outs_stat)
        KV_SERIALIZE(performance_data)
        KV_SERIALIZE(tx_pool_performance_data)        
        KV_SERIALIZE(last_block_size)
        KV_SERIALIZE(current_max_allowed_block_size)
        KV_SERIALIZE(tx_count_in_last_block)
        KV_SERIALIZE(default_fee)
        KV_SERIALIZE(minimum_fee)
        KV_SERIALIZE(last_block_timestamp)
        KV_SERIALIZE(last_block_hash)
        KV_SERIALIZE(is_hardfok_active)
        KV_SERIALIZE(offers_count)
      END_KV_SERIALIZE_MAP()
    };
  };    
  //-----------------------------------------------
  struct COMMAND_RPC_STOP_MINING
  {
    struct request
    {

      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };


  //
  struct COMMAND_RPC_GETBLOCKCOUNT
  {
    typedef std::list<std::string> request;

    struct response
    {
      uint64_t count;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(count)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };

  };

  struct COMMAND_RPC_GETBLOCKHASH
  {
    typedef std::vector<uint64_t> request;

    typedef std::string response;
  };


  struct COMMAND_RPC_GETBLOCKTEMPLATE
  {
    struct request
    {
      blobdata explicit_transaction;
      std::string extra_text;
      std::string wallet_address;
      std::string stakeholder_address;  // address for stake return (PoS blocks)
      pos_entry pe;                     // for PoS blocks
      bool pos_block;                   // is pos block 

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_BLOB_AS_HEX_STRING(explicit_transaction)
        KV_SERIALIZE(extra_text)
        KV_SERIALIZE(wallet_address)   
        KV_SERIALIZE(stakeholder_address);
        KV_SERIALIZE(pe)
        KV_SERIALIZE(pos_block)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string difficulty;
      uint64_t height;
      crypto::hash seed;
      blobdata blocktemplate_blob;
      std::string prev_hash;
      tx_generation_context miner_tx_tgc;
      uint64_t block_reward_without_fee;
      uint64_t block_reward; // == block_reward_without_fee + txs_fee if fees are given to the miner, OR block_reward_without_fee if fees are burnt
      uint64_t txs_fee;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(difficulty)
        KV_SERIALIZE(height)
        KV_SERIALIZE_POD_AS_HEX_STRING(seed)
        KV_SERIALIZE(blocktemplate_blob)
        KV_SERIALIZE(prev_hash)
        KV_SERIALIZE(miner_tx_tgc)
        KV_SERIALIZE(block_reward_without_fee)
        KV_SERIALIZE(block_reward)
        KV_SERIALIZE(txs_fee)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SUBMITBLOCK
  {
    typedef std::vector<std::string> request;
    
    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };
  
  struct COMMAND_RPC_SUBMITBLOCK2
  {
    struct request
    {
      std::string b;                              //hex encoded block blob
      std::list<epee::hexemizer> explicit_txs;    //hex encoded tx blobs

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_BLOB_AS_HEX_STRING(b)
        KV_SERIALIZE(explicit_txs)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct block_header_response
  {
      uint8_t major_version;
      uint8_t minor_version;
      uint64_t timestamp;
      std::string prev_hash;
      uint64_t nonce;
      bool orphan_status;
      uint64_t height;
      uint64_t depth;
      std::string hash;
      std::string difficulty;
      uint64_t reward;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(major_version)
        KV_SERIALIZE(minor_version)
        KV_SERIALIZE(timestamp)
        KV_SERIALIZE(prev_hash)
        KV_SERIALIZE(nonce)
        KV_SERIALIZE(orphan_status)
        KV_SERIALIZE(height)
        KV_SERIALIZE(depth)
        KV_SERIALIZE(hash)
        KV_SERIALIZE(difficulty)
        KV_SERIALIZE(reward)
      END_KV_SERIALIZE_MAP()
  };
  
  struct COMMAND_RPC_GET_LAST_BLOCK_HEADER
  {
    typedef std::list<std::string> request;

    struct response
    {
      std::string status;
      block_header_response block_header;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(block_header)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };

  };
  
  struct COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH
  {
    struct request
    {
      std::string hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(hash)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      block_header_response block_header;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(block_header)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };

  };

  struct COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT
  {
    struct request
    {
      uint64_t height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      block_header_response block_header;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(block_header)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };

  };

  struct COMMAND_RPC_GET_ALIAS_DETAILS
  {
    struct request
    {
      std::string alias;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(alias)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      alias_rpc_details_base alias_details;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(alias_details)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_ALIAS_REWARD
  {
    struct request
    {
      std::string alias;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(alias)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t reward;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(reward)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_RPC_GET_ALL_ALIASES
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<alias_rpc_details> aliases;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(aliases)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_ALIASES
  {
    struct request
    {
      uint64_t offset;
      uint64_t count;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(offset)
        KV_SERIALIZE(count)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<alias_rpc_details> aliases;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(aliases)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_ALIASES_BY_ADDRESS
  {

    typedef std::string request;

    struct response
    {
      //std::string alias;
      std::vector<alias_rpc_details> alias_info_list;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(alias_info_list)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_RPC_RESET_TX_POOL
  {

    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_REMOVE_TX_FROM_POOL
  {

    struct request
    {
      std::list<std::string> tx_to_remove;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_to_remove)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_POS_MINING_DETAILS
  {    
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      stake_modifier_type sm;
      uint64_t starter_timestamp;
      std::string pos_basic_difficulty;
      std::string status;
      crypto::hash last_block_hash;
      bool pos_mining_allowed;
      bool pos_sequence_factor_is_good;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_VAL_POD_AS_BLOB(sm)
        KV_SERIALIZE(pos_basic_difficulty)
        KV_SERIALIZE(starter_timestamp)
        KV_SERIALIZE(pos_mining_allowed)
        KV_SERIALIZE(pos_sequence_factor_is_good)
        KV_SERIALIZE(status)
        KV_SERIALIZE_VAL_POD_AS_BLOB(last_block_hash)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct tx_out_rpc_entry
  {
    uint64_t amount;
    std::list<std::string> pub_keys;
    uint64_t minimum_sigs;
    bool is_spent;
    uint64_t global_index;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE(pub_keys)
      KV_SERIALIZE(minimum_sigs)
      KV_SERIALIZE(is_spent)
      KV_SERIALIZE(global_index)
    END_KV_SERIALIZE_MAP()
  };

  struct tx_in_rpc_entry
  {
    uint64_t amount;
    uint64_t multisig_count;
    std::string htlc_origin;
    std::string kimage_or_ms_id;
    std::vector<uint64_t> global_indexes;
    std::vector<std::string> etc_options;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE(htlc_origin)
      KV_SERIALIZE(kimage_or_ms_id)
      KV_SERIALIZE(global_indexes)
      KV_SERIALIZE(multisig_count)
      KV_SERIALIZE(etc_options)
    END_KV_SERIALIZE_MAP()
  };

  struct tx_extra_rpc_entry
  {
    std::string type;
    std::string short_view;
    std::string details_view;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(type)
      KV_SERIALIZE(short_view)
      KV_SERIALIZE(details_view)
    END_KV_SERIALIZE_MAP()
  };


  struct tx_rpc_extended_info
  {
    blobdata blob;
    uint64_t blob_size;
    uint64_t fee;
    uint64_t amount;
    uint64_t timestamp;
    int64_t keeper_block; // signed int, -1 means unconfirmed transaction
    std::string id;
    std::string pub_key;
    std::vector<tx_out_rpc_entry> outs;
    std::vector<tx_in_rpc_entry> ins;
    std::vector<tx_extra_rpc_entry> extra;
    std::vector<tx_extra_rpc_entry> attachments;
    std::string object_in_json;
   
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_BLOB_AS_BASE64_STRING(blob)
      KV_SERIALIZE(blob_size)
      KV_SERIALIZE(timestamp)
      KV_SERIALIZE(keeper_block)
      KV_SERIALIZE(fee)
      KV_SERIALIZE(amount)
      KV_SERIALIZE(id)
      KV_SERIALIZE(pub_key)
      KV_SERIALIZE(outs)
      KV_SERIALIZE(ins)
      KV_SERIALIZE(extra)
      KV_SERIALIZE(attachments)
      KV_SERIALIZE_BLOB_AS_BASE64_STRING(object_in_json)
    END_KV_SERIALIZE_MAP()
  };

  struct tx_rpc_brief_info
  {
   std::string id;
   uint64_t fee;
   uint64_t total_amount;
   uint64_t sz;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(id)
      KV_SERIALIZE(fee)
      KV_SERIALIZE(total_amount)
      KV_SERIALIZE(sz)
    END_KV_SERIALIZE_MAP()
  };

  struct block_rpc_extended_info
  {
    std::string blob;
    uint64_t height;
    uint64_t timestamp;
    uint64_t actual_timestamp;
    uint64_t block_cumulative_size;
    uint64_t total_txs_size;
    uint64_t block_tself_size;
    uint64_t base_reward;
    uint64_t summary_reward;
    uint64_t penalty;
    uint64_t total_fee;
    std::string id;
    std::string cumulative_diff_adjusted;
    std::string cumulative_diff_precise;
    std::string difficulty;
    std::string prev_id;
    std::string pow_seed;
    uint64_t type;
    bool is_orphan;
    std::string already_generated_coins;
    uint64_t this_block_fee_median;
    uint64_t effective_fee_median;
    std::list<tx_rpc_extended_info> transactions_details;
    std::string miner_text_info;
    std::string object_in_json;
    

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(blob)
      KV_SERIALIZE(height)
      KV_SERIALIZE(timestamp)
      KV_SERIALIZE(actual_timestamp)
      KV_SERIALIZE(block_cumulative_size)
      KV_SERIALIZE(total_txs_size)
      KV_SERIALIZE(block_tself_size)
      KV_SERIALIZE(base_reward)  
      KV_SERIALIZE(summary_reward)
      KV_SERIALIZE(total_fee)
      KV_SERIALIZE(penalty)
      KV_SERIALIZE(id)
      KV_SERIALIZE(prev_id)
      KV_SERIALIZE(pow_seed)
      KV_SERIALIZE(cumulative_diff_adjusted)
      KV_SERIALIZE(cumulative_diff_precise)
      KV_SERIALIZE(difficulty)
      KV_SERIALIZE(already_generated_coins)
      KV_SERIALIZE(this_block_fee_median)
      KV_SERIALIZE(effective_fee_median)
      KV_SERIALIZE(transactions_details)
      KV_SERIALIZE(type)
      KV_SERIALIZE(is_orphan)
      KV_SERIALIZE(miner_text_info)      
      KV_SERIALIZE(object_in_json)
    END_KV_SERIALIZE_MAP()
  };

  

  struct COMMAND_RPC_GET_BLOCKS_DETAILS
  {
    struct request
    {
      uint64_t height_start;
      uint64_t count;
      bool ignore_transactions;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height_start)
        KV_SERIALIZE(count)
        KV_SERIALIZE(ignore_transactions)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::list<block_rpc_extended_info> blocks;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(blocks)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_ALT_BLOCKS_DETAILS
  {
    struct request
    {
      uint64_t offset;
      uint64_t count;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(offset)
        KV_SERIALIZE(count)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::vector<block_rpc_extended_info> blocks;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(blocks)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_RPC_GET_BLOCK_DETAILS
  {
    struct request
    {
      crypto::hash id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(id)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      block_rpc_extended_info block_details;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(block_details)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_RPC_GET_POOL_TXS_DETAILS
  {
    struct request
    {
      std::list<std::string> ids;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(ids)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::list<tx_rpc_extended_info> txs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(txs)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_RPC_GET_POOL_TXS_BRIEF_DETAILS
  {
    struct request
    {
      std::list<std::string> ids;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(ids)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::list<tx_rpc_brief_info> txs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(txs)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_ALL_POOL_TX_LIST
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::list<std::string> ids;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(ids)
      END_KV_SERIALIZE_MAP()
    };
  };
  

  struct COMMAND_RPC_GET_GLOBAL_INDEX_INFO
  {
    struct request
    {
      uint64_t height_start;
      uint64_t count;
      bool ignore_transactions;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height_start)
        KV_SERIALIZE(count)
        KV_SERIALIZE(ignore_transactions)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::list<block_rpc_extended_info> blocks;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(blocks)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_TX_DETAILS
  {
    struct request
    {
      std::string tx_hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      tx_rpc_extended_info tx_info;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(tx_info)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct search_entry
  {
    std::string type;
  };

  struct COMMAND_RPC_SERARCH_BY_ID
  {
    struct request
    {
      std::string id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(id)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::list<std::string> types_found;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(types_found)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_RPC_GET_OFFERS_EX
  {
    struct request
    {
      bc_services::core_offers_filter filter;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(filter)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::list<bc_services::offer_details_ex> offers;
      uint64_t total_offers;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(offers)
        KV_SERIALIZE(total_offers)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      uint64_t expiration_median;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(expiration_median)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_VALIDATE_SIGNATURE
  {
    struct request
    {
      std::string buff; //base64 encoded data
      crypto::signature sig = currency::null_sig;
      crypto::public_key pkey = currency::null_pkey;
      std::string alias;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(buff)
        KV_SERIALIZE_POD_AS_HEX_STRING(sig)
        KV_SERIALIZE_POD_AS_HEX_STRING(pkey)
        KV_SERIALIZE(alias)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string status;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct void_struct
  {
    BEGIN_KV_SERIALIZE_MAP()
    END_KV_SERIALIZE_MAP()
  };

}

