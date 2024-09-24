// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include "serialization/keyvalue_hexemizer.h"
#include "currency_protocol/currency_protocol_defs.h"
#include "currency_core/currency_basic.h"
//#include "currency_core/difficulty.h"
#include "crypto/hash.h"
#include "p2p/p2p_protocol_defs.h"
//#include "storages/portable_storage_base.h"
#include "currency_core/offers_service_basics.h"
#include <serialization/keyvalue_serialization.h>
#include <crypto/crypto.h>
#include <currency_core/basic_kv_structs.h>
#include <currency_core/blockchain_storage_basic.h>
#include <currency_core/currency_format_utils_transactions.h>
#include <currency_protocol/blobdatatype.h>
#include "common/error_codes.h"
#include <cstdint>
#include <list>
#include <string>
#include <vector>
#include <currency_core/account.h>
//#include "currency_core/basic_api_response_codes.h"

namespace currency
{
  //-----------------------------------------------

  struct alias_rpc_details_base
  {
    std::string address;
    std::string tracking_key;
    std::string comment;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(address)                      DOC_DSCR("Address of the alias.") DOC_EXMP("ZxCSpsGGeJsS8fwvQ4HktDU3qBeauoJTR6j73jAWWZxFXdF7XTbGm4YfS2kXJmAP4Rf5BVsSQ9iZ45XANXEYsrLN2L2W77dH7") DOC_END
      KV_SERIALIZE(tracking_key)                 DOC_DSCR("View secret key of the corresponding address (optional).") DOC_EXMP("18bb94f69ed61b47b6556f3871b89dff8f9a6f4f798f706fd199b05ccf8ef20c") DOC_END
      KV_SERIALIZE(comment)                      DOC_DSCR("Arbitrary comment (optional).") DOC_EXMP("Society is never gonna make any progress until we all learn to pretend to like each other.") DOC_END
    END_KV_SERIALIZE_MAP()
  };

  struct alias_rpc_details
  {
    std::string alias;
    alias_rpc_details_base details;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(alias)                        DOC_DSCR("Alias itself, a brief shortcut for an address.") DOC_EXMP("zxdya6q6whzwqjkmtcsjpc3ku") DOC_END
      KV_CHAIN_MAP(details)                      //DOC_DSCR("Object of alias_rpc_details_base struct.") DOC_EXMP_AUTO() DOC_END
    END_KV_SERIALIZE_MAP()
  };


  struct update_alias_rpc_details 
  {
    std::string old_address;
    std::string alias;
    alias_rpc_details_base details;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(old_address)                  DOC_DSCR("Previous address of the alias.") DOC_EXMP("ZxCSpsGGeJsS8fwvQ4HktDU3qBeauoJTR6j73jAWWZxFXdF7XTbGm4YfS2kXJmAP4Rf5BVsSQ9iZ45XANXEYsrLN2L2W77dH7") DOC_END
      KV_SERIALIZE(alias)                        DOC_DSCR("Alias itself, a brief shortcut for an address.") DOC_EXMP("zxdya6q6whzwqjkmtcsjpc3ku") DOC_END
      KV_SERIALIZE(details)                      DOC_DSCR("Object of alias_rpc_details_base struct.") DOC_EXMP_AUTO() DOC_END
    END_KV_SERIALIZE_MAP()
  };




  struct COMMAND_RPC_GET_POOL_INFO
  {
    DOC_COMMAND("Obtain basic information about the transaction pool.");

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
        KV_SERIALIZE(status)                     DOC_DSCR("Status code, OK if succeeded.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
        KV_SERIALIZE(error_code)                 DOC_DSCR("Error code, if there's any error (optional).") DOC_EXMP_AUTO() DOC_END
        KV_SERIALIZE(aliases_que)                DOC_DSCR("List of aliases from txs that are currently in the tx pool.") DOC_EXMP_AUTO(1) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_VOTES
  {
    DOC_COMMAND("Get votes' results from the given block range.");

    struct request
    {
      uint64_t h_start;
      uint64_t h_end;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(h_start)                    DOC_DSCR("Start of the block range to search in (including).") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(h_end)                      DOC_DSCR("End of the block range to serach in (excluding).") DOC_EXMP(40000) DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::string error_code;
      vote_results votes;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
        KV_SERIALIZE(error_code)                 DOC_DSCR("Error code, if any.") DOC_END
        KV_SERIALIZE(votes)                      DOC_DSCR("Found votes in the given range.") DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct asset_id_kv
  {
    crypto::public_key asset_id;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_POD_AS_HEX_STRING(asset_id)   DOC_DSCR("ID of an asset.") DOC_EXMP("cc4e69455e63f4a581257382191de6856c2156630b3fba0db4bdd73ffcfb36b6") DOC_END
    END_KV_SERIALIZE_MAP()
  };


  struct COMMAND_RPC_GET_ASSET_INFO
  {
    DOC_COMMAND("Obtain information for the given asset by its ID.");

    typedef asset_id_kv request;

    struct response
    {
      std::string status;
      asset_descriptor_base asset_descriptor;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
        KV_SERIALIZE(asset_descriptor)           DOC_DSCR("Descriptor of the given asset.") DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_ASSETS_LIST
  {
    DOC_COMMAND("Return list of assets registered in Zano blockchain");

    struct request
    {
      uint64_t offset = 0;
      uint64_t count = 100;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(offset)                     DOC_DSCR("Offset for the item to start copying") DOC_EXMP(0)     DOC_END
        KV_SERIALIZE(count)                      DOC_DSCR("Number of items to recieve")           DOC_EXMP(100)   DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::list<currency::asset_descriptor_with_id> assets;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status code of operation, OK if success")     DOC_EXMP(API_RETURN_CODE_OK)     DOC_END
        KV_SERIALIZE(assets)                     DOC_DSCR("List of assets registered in Zano blockchain")     DOC_EXMP_AUTO(1)     DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_RPC_DECRYPT_TX_DETAILS
  {
    DOC_COMMAND("Decrypts transaction private information. Should be used only with your own local daemon for security reasons.");

    struct request
    {
      std::string tx_id;
      currency::blobdata tx_blob;
      crypto::secret_key tx_secret_key;
      std::vector<std::string> outputs_addresses;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_id)                      DOC_DSCR("[either] ID for a transaction if it is already in the blockchain. Can be ommited if tx_blob is provided.") DOC_EXMP("a6e8da986858e6825fce7a192097e6afae4e889cabe853a9c29b964985b23da8") DOC_END
        KV_SERIALIZE(tx_blob)                    DOC_DSCR("[or] base64-encoded or hex-encoded tx blob. Can be ommited if tx_id is provided.") DOC_EXMP("ewogICJ2ZXJzaW9uIjogMSwgC....iAgInZpbiI6IFsgewogICAgIC") DOC_END
        KV_SERIALIZE_POD_AS_HEX_STRING(tx_secret_key) DOC_DSCR("Hex-encoded transaction secret key.") DOC_EXMP("2e0b840e70dba386effd64c5d988622dea8c064040566e6bf035034cbb54a5c08") DOC_END
        KV_SERIALIZE(outputs_addresses)          DOC_DSCR("Address of each of tx's output. Order is important and should correspond to order of tx's outputs. Empty strings are ignored.") DOC_EXMP_AGGR("ZxDNaMeZjwCjnHuU5gUNyrP1pM3U5vckbakzzV6dEHyDYeCpW8XGLBFTshcaY8LkG9RQn7FsQx8w2JeJzJwPwuDm2NfixPAXf", "ZxBvJDuQjMG9R2j4WnYUhBYNrwZPwuyXrC7FHdVmWqaESgowDvgfWtiXeNGu8Px9B24pkmjsA39fzSSiEQG1ekB225ZnrMTBp") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    // TODO consider reusing existing structure transfer_destination -- sowle
    struct decoded_output
    {
      uint64_t amount = 0;
      std::string address;
      crypto::public_key asset_id;
      uint64_t out_index;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amount)                      DOC_DSCR("Amount begin transferred.") DOC_EXMP(10000000000000)     DOC_END
        KV_SERIALIZE(address)                     DOC_DSCR("Destination address.") DOC_EXMP("ZxBvJDuQjMG9R2j4WnYUhBYNrwZPwuyXrC7FHdVmWqaESgowDvgfWtiXeNGu8Px9B24pkmjsA39fzSSiEQG1ekB225ZnrMTBp")     DOC_END
        KV_SERIALIZE_POD_AS_HEX_STRING(asset_id)  DOC_DSCR("Asset id.") DOC_EXMP("cc608f59f8080e2fbfe3c8c80eb6e6a953d47cf2d6aebd345bada3a1cab99852")     DOC_END
        KV_SERIALIZE(out_index)                   DOC_DSCR("Index of the corresponding output in the transaction.") DOC_EXMP(1) DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::vector<decoded_output> decoded_outputs;
      std::string tx_in_json;
      crypto::hash verified_tx_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status code of operation, OK if success") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
        KV_SERIALIZE(decoded_outputs)            DOC_DSCR("Transaction's decoded outputs") DOC_EXMP_AUTO(1) DOC_END
        KV_SERIALIZE_BLOB_AS_BASE64_STRING(tx_in_json) DOC_DSCR("Serialized transaction represented in JSON, encoded in Base64.") DOC_EXMP("ewogICJ2ZXJzaW9uIjogMSwgC....iAgInZpbiI6IFsgewogICAgIC") DOC_END
        KV_SERIALIZE_POD_AS_HEX_STRING(verified_tx_id) DOC_DSCR("(Re)calculated transaction id. Can be used in third-party proof generation.") DOC_EXMP("a6e8da986858e6825fce7a192097e6afae4e889cabe853a9c29b964985b23da8") DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_HEIGHT
  {
    DOC_COMMAND("Return current blockchain height");

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
        KV_SERIALIZE(height)                     DOC_DSCR("Height of the blockchain (equals to top block's height + 1).") DOC_EXMP(11111) DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };
  

  template<class t_block_complete_entry>
  struct COMMAND_RPC_GET_BLOCKS_FAST_T
  {

    struct request
    {
      uint64_t minimum_height;
      std::list<crypto::hash> block_ids;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(minimum_height)                  DOC_DSCR("The minimum height of the returning buch of blocks.") DOC_EXMP(0) DOC_END
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(block_ids) /* TODO !!! DOC_DSCR("Current state of the local blockchain. Hashes of the most recent 10 blocks goes first, then each 2nd, then 4th, 8, 16, 32, 64 and so on, and the last one is always hash of the genesis block.") DOC_END */
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<t_block_complete_entry> blocks;
      uint64_t    start_height;
      uint64_t    current_height;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(blocks)                     DOC_DSCR("Bunch of blocks") DOC_EXMP_AUTO(1) DOC_END
        KV_SERIALIZE(start_height)               DOC_DSCR("Starting height of the resulting bunch of blocks.") DOC_EXMP(2000000) DOC_END
        KV_SERIALIZE(current_height)             DOC_DSCR("Current height of the blockchain.") DOC_EXMP(2555000) DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  typedef COMMAND_RPC_GET_BLOCKS_FAST_T<block_complete_entry> COMMAND_RPC_GET_BLOCKS_FAST;
  typedef COMMAND_RPC_GET_BLOCKS_FAST_T<block_direct_data_entry> COMMAND_RPC_GET_BLOCKS_DIRECT;
  
  //-----------------------------------------------
  struct COMMAND_RPC_GET_TRANSACTIONS
  {
    DOC_COMMAND("Retreive transactions by their IDs.")

    struct request
    {
      std::list<std::string> txs_hashes;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txs_hashes)                 DOC_DSCR("List of transactions' IDs.") DOC_EXMP_AGGR("146791c4f5ca94bcf423557e5eb859a3a69991bd33960d52f709d88bf5d1ac6d","ec4d913a40a9ac1fbd9d33b71ef507b5c85d1f503b89096618a18b08991b5171") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<std::string> txs_as_hex;  //transactions blobs as hex
      std::list<std::string> missed_tx;   //not found transactions
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txs_as_hex)                 DOC_DSCR("Transactions stored as blobs")   DOC_EXMP_AUTO(1, "7d914497d91442f8f3c2268397d914497d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc2f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc") DOC_END
        KV_SERIALIZE(missed_tx)                  DOC_DSCR("Missed transactions hashes")     DOC_EXMP_AUTO(1, "ec4d913a40a9ac1fbd9d33b71ef507b5c85d1f503b89096618a18b08991b5171") DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------
  struct COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE
  {
    DOC_COMMAND("Give an estimation of block height by the given date.")

    struct request
    {
      uint64_t timestamp;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(timestamp)                  DOC_DSCR("Linux timestamp for the required date.") DOC_EXMP(1711021795) DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t h;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(h)                          DOC_DSCR("Estimated height of a block.") DOC_EXMP(2555000) DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_FIND_OUTS_IN_RECENT_BLOCKS
  {
    DOC_COMMAND("Retrieves information about outputs in recent blocks that are targeted for the given address with the corresponding secret view key.")
  
    static constexpr uint64_t blocks_limit_default = 5;

    struct request
    {
      account_public_address  address;
      crypto::secret_key      viewkey;
      uint64_t                blocks_limit = blocks_limit_default;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_ADDRESS_AS_TEXT(address)    DOC_DSCR("Target address for which outputs are being searched") DOC_EXMP("ZxCSpsGGeJsS8fwvQ4HktDU3qBeauoJTR6j73jAWWZxFXdF7XTbGm4YfS2kXJmAP4Rf5BVsSQ9iZ45XANXEYsrLN2L2W77dH7") DOC_END
        KV_SERIALIZE_POD_AS_HEX_STRING(viewkey)  DOC_DSCR("Secret view key corresponding to the given address.") DOC_EXMP("5fa8eaaf231a305053260ff91d69c6ef1ecbd0f5") DOC_END
        KV_SERIALIZE(blocks_limit)               DOC_DSCR("Block count limit. If 0, only the transaction pool will be searched. Maximum and default is " + epee::string_tools::num_to_string_fast(blocks_limit_default) + ".") DOC_EXMP(1711021795) DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct out_entry
    {
      uint64_t            amount;
      crypto::public_key  asset_id;
      crypto::hash        tx_id;
      int64_t             tx_block_height;
      uint64_t            output_tx_index;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amount)                     DOC_DSCR("The amount of the output.") DOC_EXMP(1000000000000) DOC_END
        KV_SERIALIZE_POD_AS_HEX_STRING(asset_id) DOC_DSCR("Asset ID of the output.") DOC_EXMP("cc4e69455e63f4a581257382191de6856c2156630b3fba0db4bdd73ffcfb36b6") DOC_END
        KV_SERIALIZE_POD_AS_HEX_STRING(tx_id)    DOC_DSCR("Transaction ID where the output is present, if found.") DOC_EXMP("a6e8da986858e6825fce7a192097e6afae4e889cabe853a9c29b964985b23da8") DOC_END
        KV_SERIALIZE(tx_block_height)            DOC_DSCR("Block height where the transaction is present.") DOC_EXMP(2555000) DOC_END
        KV_SERIALIZE(output_tx_index)            DOC_DSCR("Index of the output in the transaction.") DOC_EXMP(2) DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<out_entry> outputs;
      uint64_t blockchain_top_block_height;
      uint64_t                blocks_limit;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(outputs)                    DOC_DSCR("List of found outputs.") DOC_EXMP_AUTO(1) DOC_END
        KV_SERIALIZE(blockchain_top_block_height) DOC_DSCR("Height of the most recent block in the blockchain.") DOC_EXMP(2555000) DOC_END
        KV_SERIALIZE(blocks_limit)               DOC_DSCR("Used limit for block count.") DOC_EXMP(5) DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_GET_TX_POOL
  {
    DOC_COMMAND("Retreives transactions from tx pool (and other information).")

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
        KV_SERIALIZE(txs)                        DOC_DSCR("Transactions as blobs.") DOC_EXMP_AUTO(1, "7d914497d91442f8f3c2268397d914497d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc2f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc") DOC_END
        KV_SERIALIZE(tx_expiration_ts_median)    DOC_DSCR("Timestamp median value of last TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW blocks.") DOC_EXMP(1711021795) DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_CHECK_KEYIMAGES
  {
    DOC_COMMAND("Check spent status of given key images.")

    struct request
    {
      std::list<crypto::key_image> images;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(images) //DOC_DSCR("List of key images.") DOC_EXMP_AGGR({{'\xd6', '\x32', '\x9b', '\x5b', '\x1f', '\x7c', '\x08', '\x05', '\xb5', '\xc3', '\x45', '\xf4', '\x95', '\x75', '\x54', '\x00', '\x2a', '\x2f', '\x55', '\x78', '\x45', '\xf6', '\x4d', '\x76', '\x45', '\xda', '\xe0', '\xe0', '\x51', '\xa6', '\x49', '\x8a'}}) DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<uint64_t> images_stat;  //true - unspent, false - spent
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(images_stat)                DOC_DSCR("List of spent states, where 1 means unspent and 0 means spent.") DOC_EXMP_AUTO(1, 0) DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES
  {
    DOC_COMMAND("Obtain global outputs' indexes for the given txs.")
    
    struct request
    {
      std::list<crypto::hash> txids;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(txids)// DOC_DSCR("List of transaction hashes.") DOC_END
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::vector<struct_with_one_t_type<std::vector<uint64_t> > > tx_global_outs;
      //std::vector<uint64_t> o_indexes;
      std::string status;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_global_outs)             DOC_DSCR("List of global indexies for each output for each transaction.") DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES_BY_AMOUNT
  {
    DOC_COMMAND("Returns transaction ID and local output index for a given output amount and its global index.")

    struct request
    {
      uint64_t amount;
      uint64_t i;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amount)                     DOC_DSCR("The specific amount of output to query.") DOC_EXMP(3000000000000) DOC_END
        KV_SERIALIZE(i)                          DOC_DSCR("The global index of the output amount to be queried.") DOC_EXMP(0) DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      crypto::hash tx_id;
      uint64_t out_no;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(tx_id)    DOC_DSCR("Transaction ID where the queried output is present, if found.") DOC_EXMP("a6e8da986858e6825fce7a192097e6afae4e889cabe853a9c29b964985b23da8") DOC_END
        KV_SERIALIZE(out_no)                     DOC_DSCR("Local output index within the transaction.") DOC_EXMP(7) DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_RPC_GET_MULTISIG_INFO
  {
    DOC_COMMAND("Retrieve basic information about a multisig output using its unique identifier (hash).");

    struct request
    {
      crypto::hash ms_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(ms_id)    DOC_DSCR("The multisig output's unique identifier (hash).") DOC_EXMP("a6e8da986858e6825fce7a192097e6afae4e889cabe853a9c29b964985b23da8") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      crypto::hash tx_id;
      uint64_t out_no;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(tx_id)    DOC_DSCR("Transaction ID where the multisig output is present, if found.") DOC_EXMP("a88541e38d64f87c41b9153412d1d7667f6e4337fe429ed1374722355fa7b423") DOC_END
        KV_SERIALIZE(out_no)                     DOC_DSCR("Local output index within the transaction.") DOC_EXMP(11) DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS
  {
    DOC_COMMAND("Retrieve random decoy outputs for specified amounts, to be used for mixing in transactions.");

    struct request
    {
      std::list<uint64_t> amounts;
      uint64_t            decoys_count;       // how many decoy outputs needed (per amount)
      uint64_t            height_upper_limit; // if nonzero, all the decoy outputs must be either older than, or the same age as this height
      bool                use_forced_mix_outs;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amounts)                    DOC_DSCR("List of amounts for which decoy outputs are requested.") DOC_EXMP_AGGR(0, 10000000000, 5000000000000) DOC_END
        KV_SERIALIZE(decoys_count)               DOC_DSCR("Number of decoy outputs required for each amount specified.") DOC_EXMP_AUTO(10) DOC_END
        KV_SERIALIZE(height_upper_limit)         DOC_DSCR("Maximum blockchain height from which decoys can be taken. If nonzero, decoys must be at this height or older.") DOC_EXMP_AUTO(2555000) DOC_END
        KV_SERIALIZE(use_forced_mix_outs)        DOC_DSCR("If true, only outputs with a 'mix_attr' greater than 0 are used as decoys.") DOC_EXMP_AUTO(false) DOC_END
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
        KV_SERIALIZE(amount)                     DOC_DSCR("The amount for which decoys are returned.") DOC_EXMP_AUTO(10000000000) DOC_END
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(outs) //DOC_DSCR("List of 'out_entry' structures, serialized as a blob.") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<outs_for_amount> outs;
      std::string status;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(outs)                       DOC_DSCR("List of 'outs_for_amount' structures, each containing decoys for a specific amount.") DOC_EXMP_AUTO(1) DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3
  {
    DOC_COMMAND("Version 3 of the command to retrieve random decoy outputs for specified amounts, focusing on either pre-zarcanum or post-zarcanum zones based on the amount value.");

    struct offsets_distribution
    {
      uint64_t amount; //if amount is 0 then lookup in post-zarcanum zone only, if not 0 then pre-zarcanum only
      std::vector<uint64_t> global_offsets; //[i] = global_index to pick up

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amount)                     DOC_DSCR("If set to 0, only ZC outputs are considered. If nonzero, only old bare outputs are considered.") DOC_EXMP_AUTO(0) DOC_END
        KV_SERIALIZE(global_offsets)             DOC_DSCR("List of global indices for picking decoys. Each index corresponds to a potential decoy output.") DOC_EXMP_AGGR(1, 3, 5928, 2811) DOC_END
      END_KV_SERIALIZE_MAP()
    };


    struct request
    {
      std::vector<offsets_distribution> amounts;
      uint64_t            height_upper_limit; // if nonzero, all the decoy outputs must be either older than, or the same age as this height
      bool                use_forced_mix_outs;
      uint64_t            coinbase_percents;     //from 0 to 100, estimate percents of coinbase outputs included in decoy sets  
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amounts)                    DOC_DSCR("List of amount distributions specifying where to look for decoys, based on old bare outputs or ZC outputs.") DOC_EXMP_AUTO(1) DOC_END
        KV_SERIALIZE(height_upper_limit)         DOC_DSCR("Maximum blockchain height from which decoys can be taken. If nonzero, decoys must be at this height or older.") DOC_EXMP(2555000) DOC_END
        KV_SERIALIZE(use_forced_mix_outs)        DOC_DSCR("If true, only outputs with a 'mix_attr' greater than 0 are used as decoys.") DOC_EXMP(false) DOC_END
        KV_SERIALIZE(coinbase_percents)          DOC_DSCR("Specifies the estimated percentage of coinbase outputs to be included in the decoy sets, ranging from 0 to 100.") DOC_EXMP(15) DOC_END
      END_KV_SERIALIZE_MAP()
    };

    typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response response;
  };
  //-----------------------------------------------
  struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_LEGACY
  {
    DOC_COMMAND("Retrieve random decoy outputs for specified amounts (legacy format).");

    struct request
    {
      std::list<uint64_t> amounts;
      uint64_t outs_count;
      bool use_forced_mix_outs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amounts)                    DOC_DSCR("List of amounts for which decoy outputs are requested.") DOC_EXMP_AGGR(0, 10000000000) DOC_END
        KV_SERIALIZE(outs_count)                 DOC_DSCR("Number of decoy outputs requested for each amount.") DOC_EXMP(2) DOC_END
        KV_SERIALIZE(use_forced_mix_outs)        DOC_DSCR("If true, only outputs with a 'mix_attr' greater than 0 are used as decoys.") DOC_EXMP(false) DOC_END
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
        KV_SERIALIZE(amount)                     DOC_DSCR("The amount for which decoys are returned.") DOC_EXMP(10000000000) DOC_END
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(outs) //DOC_DSCR("List of 'out_entry' structures serialized as a blob, representing the decoy outputs for the specified amount.") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<outs_for_amount> outs;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(outs)                       DOC_DSCR("List of 'outs_for_amount' structures, each containing decoys for a specific amount.") DOC_EXMP_AUTO(1) DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
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
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------
  struct COMMAND_RPC_SEND_RAW_TX
  {
    DOC_COMMAND("Broadcasts a raw transaction encoded in hexadecimal format to the network.");

    struct request
    {
      std::string tx_as_hex;

      request() {}
      explicit request(const transaction &);

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_as_hex)                  DOC_DSCR("The transaction data as a hexadecimal string, ready for network broadcast.") DOC_EXMP("00018ed1535b8b4862e.....368cdc5a86") DOC_END
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_RPC_FORCE_RELAY_RAW_TXS
  {
    struct request
    {
      std::vector<std::string> txs_as_hex;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txs_as_hex)                 DOC_DSCR("List of transactions as a hexadecimal strings.") DOC_EXMP_AGGR("000535b8b2e.....3685a86", "00087368b2e.....349b77f") DOC_END
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_START_MINING
  {
    DOC_COMMAND("Initiates PoW mining process on a node using the specified miner address and the number of CPU threads.");

    struct request
    {
      std::string miner_address;
      uint64_t    threads_count;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(miner_address)              DOC_DSCR("The address where the mining rewards will be deposited.") DOC_EXMP("ZxCSpsGGeJsS8fwvQ4HktDU3qBeauoJTR6j73jAWWZxFXdF7XTbGm4YfS2kXJmAP4Rf5BVsSQ9iZ45XANXEYsrLN2L2W77dH7") DOC_END
        KV_SERIALIZE(threads_count)              DOC_DSCR("The number of CPU threads to use for mining.") DOC_EXMP(2) DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  struct outs_index_stat
  {
    uint64_t amount_0;
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
      KV_SERIALIZE(amount_0)
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
    DOC_COMMAND("Retrieves various information about the blockchain node. The user must specify their needs via a 'flags' field in the request by combining necessary flags using binary OR. Some values are always calculated and provided, others only if the corresponding flag is specified.");

    struct request
    {
      uint64_t flags;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(flags)                      DOC_DSCR("Combination of flags to request specific data elements that are computationally expensive to calculate.") DOC_EXMP(1048575) DOC_END
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
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
        // Always calculated and provided fields
        KV_SERIALIZE(height)                     DOC_DSCR("The current size of the blockchain, equal to the height of the top block plus one.") DOC_EXMP(2555000) DOC_END
        KV_SERIALIZE(pos_allowed)                DOC_DSCR("Boolean value indicating whether PoS mining is currently allowed based on network rules and state.") DOC_EXMP(true) DOC_END
        KV_SERIALIZE(pos_difficulty)             DOC_DSCR("Current difficulty for Proof of Stake mining.") DOC_EXMP("1848455949616658404658") DOC_END
        KV_SERIALIZE(pow_difficulty)             DOC_DSCR("Current difficulty for Proof of Work mining.") DOC_EXMP(12777323347117) DOC_END
        KV_SERIALIZE(tx_count)                   DOC_DSCR("Total number of transactions in the blockchain.") DOC_EXMP(767742) DOC_END
        KV_SERIALIZE(tx_pool_size)               DOC_DSCR("Number of transactions currently in the pool.") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(alt_blocks_count)           DOC_DSCR("Number of alternative blocks on the blockchain.") DOC_EXMP(99) DOC_END
        KV_SERIALIZE(outgoing_connections_count) DOC_DSCR("Number of outgoing P2P connections to other nodes.") DOC_EXMP(8) DOC_END
        KV_SERIALIZE(incoming_connections_count) DOC_DSCR("Number of incoming P2P connections established by other nodes.") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(synchronized_connections_count) DOC_DSCR("Number of P2P connections to nodes that have a fully synchronized blockchain.") DOC_EXMP(8) DOC_END
        KV_SERIALIZE(white_peerlist_size)        DOC_DSCR("Size of the white peer list, which includes addresses of reliable nodes.") DOC_EXMP(12) DOC_END
        KV_SERIALIZE(grey_peerlist_size)         DOC_DSCR("Size of the grey peer list, which includes addresses of nodes with less consistent availability.") DOC_EXMP(321) DOC_END
        KV_SERIALIZE(current_blocks_median)      // TODO
        KV_SERIALIZE(alias_count)                DOC_DSCR("The total number of unique aliases registered on the blockchain. Aliases are alternate, human-readable names associated with addresses.") DOC_EXMP(1653) DOC_END
        KV_SERIALIZE(current_max_allowed_block_size) DOC_DSCR("Current maximum allowed cummulative block size in bytes.") DOC_EXMP(250000) DOC_END
        KV_SERIALIZE(is_hardfok_active)          DOC_DSCR("A list of boolean values indicating whether each corresponding hardfork is active. For example, a list 'true, true, false' indicates that the first hardfork is activated, while the second is not. Hardfork #0 is always active as it is a stub.") DOC_EXMP_AGGR(true,true,true,true,true,false) DOC_END
        KV_SERIALIZE(daemon_network_state)       DOC_DSCR("Current network state of the daemon, which could be connecting, synchronizing, online, loading core, internal error, unloading core, or downloading database.") DOC_EXMP(2) DOC_END
        KV_SERIALIZE(synchronization_start_height) DOC_DSCR("Blockchain height at which the current synchronization process started. Indicates the starting point for catching up to the network's latest state.") DOC_EXMP(2555000) DOC_END
        KV_SERIALIZE(max_net_seen_height)        DOC_DSCR("Maximum blockchain height observed in the network by this node.") DOC_EXMP(2555743) DOC_END
        KV_SERIALIZE(default_fee)                DOC_DSCR("Default fee for transactions.") DOC_EXMP(10000000000) DOC_END
        KV_SERIALIZE(minimum_fee)                DOC_DSCR("Minimum fee for transactions.") DOC_EXMP(10000000000) DOC_END
        KV_SERIALIZE(mi)                         DOC_DSCR("The most recent mainterner's info.") DOC_EXMP_AUTO() DOC_END

        // Fields dependent on flags for their inclusion
        KV_SERIALIZE(net_time_delta_median)      DOC_DSCR("A value of 0 indicates no time synchronization issues, while a value of 1 indicates the presence of time sync issues. Only available if the COMMAND_RPC_GET_INFO_FLAG_NET_TIME_DELTA_MEDIAN flag is set.") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(current_network_hashrate_50)DOC_DSCR("The PoW hash rate calculated over the last 50 blocks of any type. This information is only provided if the COMMAND_RPC_GET_INFO_FLAG_CURRENT_NETWORK_HASHRATE_50 flag is set.") DOC_EXMP(109575236643) DOC_END
        KV_SERIALIZE(current_network_hashrate_350) DOC_DSCR("The PoW hash rate calculated over the last 350 blocks of any type. This information is only provided if the COMMAND_RPC_GET_INFO_FLAG_CURRENT_NETWORK_HASHRATE_350 flag is set.") DOC_EXMP(107939216153) DOC_END
        KV_SERIALIZE(seconds_for_10_blocks)      DOC_DSCR("The time period in seconds between the most recent block and the 10th block older. This information is only provided if the COMMAND_RPC_GET_INFO_FLAG_SECONDS_FOR_10_BLOCKS flag is set.") DOC_EXMP(476) DOC_END
        KV_SERIALIZE(seconds_for_30_blocks)      DOC_DSCR("The time period in seconds between the most recent block and the 30th block older. This information is only provided if the COMMAND_RPC_GET_INFO_FLAG_SECONDS_FOR_30_BLOCKS flag is set.") DOC_EXMP(1264) DOC_END
        KV_SERIALIZE(transactions_cnt_per_day)   DOC_DSCR("The number of non-mining transactions recorded over the last 24 hours. This information is only provided if the COMMAND_RPC_GET_INFO_FLAG_TRANSACTIONS_DAILY_STAT flag is set.") DOC_EXMP(1325) DOC_END
        KV_SERIALIZE(transactions_volume_per_day)DOC_DSCR("The total sum of input amounts from all non-mining transactions over the last 24 hours. Only old bare inputs with explicit amounts are considered. This information is only provided if the COMMAND_RPC_GET_INFO_FLAG_TRANSACTIONS_DAILY_STAT flag is set.") DOC_EXMP(6615220203700000) DOC_END
        KV_SERIALIZE(last_pos_timestamp)         DOC_DSCR("The timestamp of the most recent PoS block. This information is only provided if the COMMAND_RPC_GET_INFO_FLAG_LAST_POS_TIMESTAMP flag is set.") DOC_EXMP(1719585105) DOC_END
        KV_SERIALIZE(last_pow_timestamp)         DOC_DSCR("The timestamp of the most recent PoW block. This information is only provided if the COMMAND_RPC_GET_INFO_FLAG_LAST_POW_TIMESTAMP flag is set.") DOC_EXMP(1719586493) DOC_END
        KV_SERIALIZE(total_coins)                DOC_DSCR("The total amount of all emitted coins in the system. This information is only provided if the COMMAND_RPC_GET_INFO_FLAG_TOTAL_COINS flag is set.") DOC_EXMP("14308874719144585856") DOC_END
        KV_SERIALIZE(last_block_size)            DOC_DSCR("The size of the last block in bytes. This information is only provided if the COMMAND_RPC_GET_INFO_FLAG_LAST_BLOCK_SIZE flag is set.") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(tx_count_in_last_block)     DOC_DSCR("The number of non-mining transactions in the last block. This information is only provided if the COMMAND_RPC_GET_INFO_FLAG_TX_COUNT_IN_LAST_BLOCK flag is set.") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(pos_sequence_factor)        DOC_DSCR("The current PoS sequence factor, representing the number of consecutive PoS blocks. This information is only provided if the COMMAND_RPC_GET_INFO_FLAG_POS_SEQUENCE_FACTOR flag is set.") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(pow_sequence_factor)        DOC_DSCR("The current PoW sequence factor, representing the number of consecutive PoW blocks. This information is only provided if the COMMAND_RPC_GET_INFO_FLAG_POW_SEQUENCE_FACTOR flag is set.") DOC_EXMP(1) DOC_END
        KV_SERIALIZE(block_reward)               DOC_DSCR("The base block reward that is effective for the next block. Calculated only if either COMMAND_RPC_GET_INFO_FLAG_POS_DIFFICULTY or COMMAND_RPC_GET_INFO_FLAG_TOTAL_COINS flag is set.") DOC_EXMP(1000000000000) DOC_END
        KV_SERIALIZE(last_block_total_reward)    DOC_DSCR("Reward for the last block, including base reward and transaction fees. Calculated only if either COMMAND_RPC_GET_INFO_FLAG_POS_DIFFICULTY or COMMAND_RPC_GET_INFO_FLAG_TOTAL_COINS flag is set.") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(pos_diff_total_coins_rate)  DOC_DSCR("PoS difficulty divided by the total amount of all coins in the system minus a premined amount (17,517,203). Calculated only if either COMMAND_RPC_GET_INFO_FLAG_POS_DIFFICULTY or COMMAND_RPC_GET_INFO_FLAG_TOTAL_COINS flag is set.") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(last_block_timestamp)       DOC_DSCR("Timestamp of the last block. Calculated only if either COMMAND_RPC_GET_INFO_FLAG_POS_DIFFICULTY or COMMAND_RPC_GET_INFO_FLAG_TOTAL_COINS flag is set.") DOC_EXMP(1719586493) DOC_END
        KV_SERIALIZE(last_block_hash)            DOC_DSCR("Hash of the last block. Calculated only if either COMMAND_RPC_GET_INFO_FLAG_POS_DIFFICULTY or COMMAND_RPC_GET_INFO_FLAG_TOTAL_COINS flag is set.") DOC_EXMP("153af86fd0d7d0a427526258e30505a4d21b8f77261f5276c7669e0a6c83efa0") DOC_END
        KV_SERIALIZE(pos_block_ts_shift_vs_actual) DOC_DSCR("The difference between the timestamp used in the last PoS block for mining purposes and its actual timestamp as stored in the miner's transaction extra data. This information is only provided if the COMMAND_RPC_GET_INFO_FLAG_POS_BLOCK_TS_SHIFT_VS_ACTUAL flag is set.") DOC_EXMP(-1387) DOC_END
        KV_SERIALIZE(outs_stat)                  DOC_DSCR("Statistics for the number of outputs that have a specific amount. This information is only provided if the COMMAND_RPC_GET_INFO_FLAG_OUTS_STAT flag is set.") DOC_EXMP_AUTO() DOC_END
        KV_SERIALIZE(performance_data)           DOC_DSCR("Detailed technical performance data intended for developers. This information is only provided if the COMMAND_RPC_GET_INFO_FLAG_PERFORMANCE flag is set.") DOC_EXMP_AUTO() DOC_END
        KV_SERIALIZE(tx_pool_performance_data)   DOC_DSCR("Detailed technical performance data intended for developers. This information is only provided if the COMMAND_RPC_GET_INFO_FLAG_PERFORMANCE flag is set.") DOC_EXMP_AUTO() DOC_END
        KV_SERIALIZE(offers_count)               DOC_DSCR("Current number of offers in the offers service. This information is only provided if the COMMAND_RPC_GET_INFO_FLAG_PERFORMANCE flag is set.") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(expiration_median_timestamp)DOC_DSCR("Median of timestamps of the last N blocks, used to determine the expiration status of transactions. This information is only provided if the COMMAND_RPC_GET_INFO_FLAG_EXPIRATIONS_MEDIAN flag is set.") DOC_EXMP(1719585827) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };    
  //-----------------------------------------------
  struct COMMAND_RPC_STOP_MINING
  {
    DOC_COMMAND("Stop PoW mining process on CPU.")

    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_RPC_GETBLOCKCOUNT
  {
    DOC_COMMAND("Returns the total number of blocks in the blockchain (the height of the top block plus one).");

    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t count;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(count)                      DOC_DSCR("The total number of blocks in the blockchain, equivalent to the top block's height plus one.") DOC_EXMP(2697388) DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_RPC_GETBLOCKHASH
  {
    DOC_COMMAND("Returns block hash by the given height.");

    typedef std::vector<uint64_t> request;

    typedef std::string response;
  };

  //-----------------------------------------------

  struct COMMAND_RPC_GETBLOCKTEMPLATE
  {
    DOC_COMMAND("Generates a block template for mining, intended for both PoW and PoS types of blocks based on the provided parameters.");

    struct request
    {
      blobdata explicit_transaction;
      std::string extra_text;
      std::string wallet_address;
      std::string stakeholder_address;           // address for stake return (PoS blocks)
      pos_entry pe;                              // for PoS blocks
      bool pos_block;                            // is pos block 

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_BLOB_AS_HEX_STRING(explicit_transaction) DOC_DSCR("A transaction blob that must be explicitly included in the block.") DOC_EXMP("5fa8eaaf231a305053260ff91d69c6ef1ecbd0f5") DOC_END
        KV_SERIALIZE(extra_text)                 DOC_DSCR("Arbitrary data added to the extra field of the miner transaction.") DOC_EXMP("OMG, you can't just ask people why they're PoW-maxi") DOC_END
        KV_SERIALIZE(wallet_address)             DOC_DSCR("Address where mining rewards will be deposited.") DOC_EXMP("ZxCSpsGGeJsS8fwvQ4HktDU3qBeauoJTR6j73jAWWZxFXdF7XTbGm4YfS2kXJmAP4Rf5BVsSQ9iZ45XANXEYsrLN2L2W77dH7") DOC_END
        KV_SERIALIZE(stakeholder_address)        DOC_DSCR("Address where the stake is returned for PoS blocks (usually the same as 'wallet_address').") DOC_END
        KV_SERIALIZE(pe)                         DOC_DSCR("PoS entry details, relevant only for PoS block generation.") DOC_END
        KV_SERIALIZE(pos_block)                  DOC_DSCR("Flag indicating whether the block is a PoS block.") DOC_EXMP(false) DOC_END
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
        KV_SERIALIZE(difficulty)                 DOC_DSCR("The mining difficulty targeted by the block template.") DOC_EXMP("12936195379842") DOC_END
        KV_SERIALIZE(height)                     DOC_DSCR("The height of the block template in the blockchain.") DOC_EXMP(2555002) DOC_END
        KV_SERIALIZE_POD_AS_HEX_STRING(seed)     DOC_DSCR("Seed value for the ProgPoWZ mining algorithm's epoch.") DOC_EXMP("0518e1373ff88ccabb28493cac10cb0731313135d880dae0d846be6016ab9acf") DOC_END
        KV_SERIALIZE(blocktemplate_blob)         DOC_DSCR("Serialized block template blob.") DOC_EXMP("030000000000000000ae73338b792......6258a2b5ee340700") DOC_END
        KV_SERIALIZE(prev_hash)                  DOC_DSCR("Hash of the previous block in the chain.") DOC_EXMP("ae73338b7927df71b6ed477937625c230172219306750ba97995fb5109dda703") DOC_END
        KV_SERIALIZE(miner_tx_tgc)               DOC_DSCR("Miner transaction generation context. Intended for PoS blocks and Zarcanum.") DOC_EXMP_AUTO() DOC_END
        KV_SERIALIZE(block_reward_without_fee)   DOC_DSCR("Base block reward excluding any transaction fees.") DOC_EXMP(1000000000000) DOC_END
        KV_SERIALIZE(block_reward)               DOC_DSCR("Total block reward, including transaction fees if they are given to the miner (legacy), or the base reward if fees are burnt (current state).") DOC_EXMP(1000000000000) DOC_END
        KV_SERIALIZE(txs_fee)                    DOC_DSCR("Total fees from transactions included in the block.") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_RPC_SUBMITBLOCK
  {
    DOC_COMMAND("Adds new block to the blockchain. Request should contain one string with hex-encoded block blob.");

    typedef std::vector<std::string> request;
    
    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };
  
  //-----------------------------------------------

  struct COMMAND_RPC_SUBMITBLOCK2
  {
    DOC_COMMAND("Adds new block to the blockchain.");

    struct request
    {
      std::string b;                              //hex encoded block blob
      std::list<epee::hexemizer> explicit_txs;    //hex encoded tx blobs

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_BLOB_AS_HEX_STRING(b)       DOC_DSCR("Hex-encoded serialized block.") DOC_EXMP("030000000000000000ae73338b7926258a2b5ee340700") DOC_END
        KV_SERIALIZE(explicit_txs)               DOC_DSCR("List of hex-encoded transactions to be explicitly included in the block.") DOC_EXMP_AGGR({"6258a2b5ee7a9c20"}, {"8e0f0a2cee340700"}) DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

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
        KV_SERIALIZE(major_version)              DOC_DSCR("Major version of the block.") DOC_EXMP(3) DOC_END
        KV_SERIALIZE(minor_version)              DOC_DSCR("Minor version of the block.") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(timestamp)                  DOC_DSCR("Timestamp of the block creation.") DOC_EXMP(1719588270) DOC_END
        KV_SERIALIZE(prev_hash)                  DOC_DSCR("Hash of the previous block in the chain.") DOC_EXMP("a1b4359c02985720b0cf542678e08f0d4075e518fbd0cd54bd280269545e0e6f") DOC_END
        KV_SERIALIZE(nonce)                      DOC_DSCR("Nonce used for generating the block to meet the network difficulty.") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(orphan_status)              DOC_DSCR("Indicates if the block is an orphan (true) or a normal block (false).") DOC_EXMP(false) DOC_END
        KV_SERIALIZE(height)                     DOC_DSCR("Height of the block in the blockchain.") DOC_EXMP(2697404) DOC_END
        KV_SERIALIZE(depth)                      DOC_DSCR("Depth of the block in the blockchain. Depth 0 indicates the most recent block.") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(hash)                       DOC_DSCR("Hash of the block.") DOC_EXMP("f5df39c4b1590394976aa6e72f04df7836e22dbdfc1e6f61f6cc1b624d83cd94") DOC_END
        KV_SERIALIZE(difficulty)                 DOC_DSCR("Network difficulty target that the block met.") DOC_EXMP("1849593878843995770114") DOC_END
        KV_SERIALIZE(reward)                     DOC_DSCR("Total mining reward of the block including transaction fees (if applicable).") DOC_EXMP(0) DOC_END
      END_KV_SERIALIZE_MAP()
  };
  
  struct COMMAND_RPC_GET_LAST_BLOCK_HEADER
  {
    DOC_COMMAND("Returns the block header information of the most recent block.");
    typedef std::list<std::string> request; // TODO @#@# fix this

    struct response
    {
      std::string status;
      block_header_response block_header;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(block_header)               DOC_DSCR("Detailed header information of the block.") DOC_EXMP_AUTO() DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };

  };
  
  //-----------------------------------------------

  struct COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH
  {
    DOC_COMMAND("Retrieves the block header information for a given block hash.");

    struct request
    {
      std::string hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(hash)                       DOC_DSCR("The hash of the block for which the header information is being requested.") DOC_EXMP("a1b4359c02985720b0cf542678e08f0d4075e518fbd0cd54bd280269545e0e6f") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      block_header_response block_header;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(block_header)               DOC_DSCR("Detailed header information of the block.") DOC_EXMP_AUTO() DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT
  {
    DOC_COMMAND("Retrieves the block header information for a given block height.");

    struct request
    {
      uint64_t height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height)                     DOC_DSCR("The height of the block for which the header information is being requested.") DOC_EXMP(2555000) DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      block_header_response block_header;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(block_header)               DOC_DSCR("Detailed header information of the block.") DOC_EXMP_AUTO() DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_RPC_GET_ALIAS_DETAILS
  {
    DOC_COMMAND("Retrieves information about a specific address alias.");

    struct request
    {
      std::string alias;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(alias)                      DOC_DSCR("The alias name for which details are being requested.") DOC_EXMP("gigabyted") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      alias_rpc_details_base alias_details;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(alias_details)              DOC_DSCR("Contains the detailed information about the specified alias, including the associated wallet address, tracking key, comment etc..") DOC_EXMP_AUTO() DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_RPC_GET_ALIAS_REWARD
  {
    DOC_COMMAND("Retrieves the cost of registering an alias on the blockchain.");

    struct request
    {
      std::string alias;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(alias)                      DOC_DSCR("The alias name for which the registration cost is being queried.") DOC_EXMP("zxdya6q6whzwqjkmtcsjpc3ku") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t reward;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(reward)                     DOC_DSCR("The registration cost for the specified alias.") DOC_EXMP(100000000000) DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_RPC_GET_ALL_ALIASES
  {
    DOC_COMMAND("Retrieves all registered aliases along with associated information.");

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
        KV_SERIALIZE(aliases)                    DOC_DSCR("List of alias_rpc_details objects, each containing information about an individual alias.") DOC_EXMP_AUTO(2) DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_RPC_GET_ALIASES
  {
    DOC_COMMAND("Retrieves a specified portion of all registered aliases, allowing pagination through large sets of aliases.");

    struct request
    {
      uint64_t offset; // The starting point from which aliases are to be retrieved.
      uint64_t count;  // The number of aliases to retrieve.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(offset)                     DOC_DSCR("The offset in the list of all aliases from which to start retrieving.") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(count)                      DOC_DSCR("The number of aliases to retrieve from the specified offset.") DOC_EXMP(2) DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<alias_rpc_details> aliases;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(aliases)                    DOC_DSCR("List of alias_rpc_details objects, each containing information about an individual alias retrieved based on the request parameters.") DOC_EXMP_AUTO(2) DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_RPC_GET_ALIASES_BY_ADDRESS
  {
    DOC_COMMAND("Retrieves all aliases registered for a given address.");

    typedef std::string request;

    struct response
    {
      std::vector<alias_rpc_details> alias_info_list;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(alias_info_list)            DOC_DSCR("List of alias_rpc_details objects, each containing detailed information about each alias registered to the specified address.") DOC_EXMP_AUTO(1) DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_RPC_RESET_TX_POOL
  {
    DOC_COMMAND("Clears transaction pool.");

    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_RPC_REMOVE_TX_FROM_POOL
  {
    DOC_COMMAND("Removes specified transactions from the transaction pool, typically to clear out transactions that are no longer valid or needed.");

    struct request
    {
      std::list<std::string> tx_to_remove;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_to_remove)               DOC_DSCR("List of transaction IDs that are to be removed from the transaction pool.") DOC_EXMP_AGGR("c5efacd06128fc5a73f58392c84534cd1a146de7d47ffbe770486cce5130dc1f","c2f0de2ef4753dc0ec8dd2da5ebf8e77f07d2ac0791357a9e3f2537071b33762") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_RPC_GET_POS_MINING_DETAILS
  {
    DOC_COMMAND("Retrieves basic information regarding PoS mining, including current PoS conditions and constraints.");

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
        KV_SERIALIZE_VAL_POD_AS_BLOB(sm)         //DOC_DSCR("Stake modifier object used in PoS mining calculations.") DOC_END
        KV_SERIALIZE(pos_basic_difficulty)       DOC_DSCR("Current PoS difficulty.") DOC_END
        KV_SERIALIZE(starter_timestamp)          DOC_DSCR("Timestamp from which timestamps are evaluated for meeting PoS win condition.") DOC_END
        KV_SERIALIZE(pos_mining_allowed)         DOC_DSCR("Indicates whether PoS mining is currently allowed, which may be restricted under certain blockchain conditions or in testnets.") DOC_END
        KV_SERIALIZE(pos_sequence_factor_is_good) DOC_DSCR("Indicates whether the PoS sequence factor is at a level that allows for continued PoS mining, requiring a PoW block to reset if too high.") DOC_END
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
        KV_SERIALIZE_VAL_POD_AS_BLOB(last_block_hash) //DOC_DSCR("Hash of the most recent block in the blockchain.") DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct tx_out_rpc_entry
  {
    uint64_t amount;
    std::list<std::string> pub_keys;
    uint64_t minimum_sigs;
    bool is_spent;
    uint64_t global_index;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)                       DOC_DSCR("The output's amount, 0 for ZC outputs.") DOC_EXMP(9000000000) DOC_END
      KV_SERIALIZE(pub_keys)                     DOC_DSCR("List of public keys associated with the output.") DOC_EXMP_AGGR("7d0c755e7e24a241847176c9a3cf4c970bcd6377018068abe6fe4535b23f5323") DOC_END
      KV_SERIALIZE(minimum_sigs)                 DOC_DSCR("Minimum number of signatures required to spend the output, for multisig outputs only.") DOC_EXMP(0) DOC_END
      KV_SERIALIZE(is_spent)                     DOC_DSCR("Indicates whether the output has been spent.") DOC_EXMP(false) DOC_END
      KV_SERIALIZE(global_index)                 DOC_DSCR("Global index of the output for this specific amount.") DOC_EXMP(0) DOC_END
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
      KV_SERIALIZE(amount)                       DOC_DSCR("The amount of coins being transacted.") DOC_EXMP(1000000000000) DOC_END
      KV_SERIALIZE(htlc_origin)                  DOC_DSCR("Origin hash for HTLC (Hash Time Locked Contract).") DOC_END
      KV_SERIALIZE(kimage_or_ms_id)              DOC_DSCR("Contains either the key image for the input or the multisig output ID, depending on the input type.") DOC_EXMP("2540e0544b1fed3b104976f803dbd83681335c427f9d601d9d5aecf86ef276d2") DOC_END
      KV_SERIALIZE(global_indexes)               DOC_DSCR("List of global indexes indicating the outputs referenced by this input, where only one is actually being spent.") DOC_EXMP_AGGR(0,2,12,27) DOC_END
      KV_SERIALIZE(multisig_count)               DOC_DSCR("Number of multisig signatures used, relevant only for multisig outputs.") DOC_EXMP(0) DOC_END
      KV_SERIALIZE(etc_options)                  DOC_DSCR("Auxiliary options associated with the input, containing additional configuration or data.") DOC_END
    END_KV_SERIALIZE_MAP()
  };

  struct tx_extra_rpc_entry
  {
    std::string type;
    std::string short_view;
    std::string details_view;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(type)                         DOC_DSCR("Type of the extra entry in the transaction.") DOC_EXMP("pub_key") DOC_END
      KV_SERIALIZE(short_view)                   DOC_DSCR("A concise representation of the extra entry.") DOC_EXMP("0feef5e2ea0e88b592c0a0e6639ce73e12ea9b3136d89464748fcb60bb6f18f5") DOC_END
      KV_SERIALIZE(details_view)                 DOC_DSCR("A detailed representation of the extra entry.") DOC_EXMP("") DOC_END
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
      KV_SERIALIZE_BLOB_AS_BASE64_STRING(blob)  DOC_DSCR("Serialized form of the transaction, encoded in Base64.") DOC_EXMP("ARMBgKCUpY0dBBoAAAAAAAAAABoCAAAAAAAAABoKAAAAAAAAABoPAAAAAAAAACVA4FRLH") DOC_END
      KV_SERIALIZE(blob_size)                   DOC_DSCR("Size of the serialized transaction in bytes.") DOC_EXMP(6794) DOC_END
      KV_SERIALIZE(timestamp)                   DOC_DSCR("Timestamp when the transaction was created.") DOC_EXMP(1557345925) DOC_END
      KV_SERIALIZE(keeper_block)                DOC_DSCR("Block height where the transaction is confirmed, or -1 if it is unconfirmed.") DOC_EXMP(51) DOC_END
      KV_SERIALIZE(fee)                         DOC_DSCR("Transaction fee in the smallest currency unit.") DOC_EXMP(1000000000) DOC_END
      KV_SERIALIZE(amount)                      DOC_DSCR("Total output amount of the transaction (legacy, for pre-Zarcanum txs).") DOC_EXMP(18999000000000) DOC_END
      KV_SERIALIZE(id)                          DOC_DSCR("Hash of the transaction.") DOC_EXMP("a6e8da986858e6825fce7a192097e6afae4e889cabe853a9c29b964985b23da8") DOC_END
      KV_SERIALIZE(pub_key)                     DOC_DSCR("Public key associated with the transaction.") DOC_EXMP("0feef5e2ea0e88b592c0a0e6639ce73e12ea9b3136d89464748fcb60bb6f18f5") DOC_END
      KV_SERIALIZE(outs)                        DOC_DSCR("Outputs of the transaction.") DOC_EXMP_AUTO(1) DOC_END
      KV_SERIALIZE(ins)                         DOC_DSCR("Inputs of the transaction.") DOC_EXMP_AUTO(1) DOC_END
      KV_SERIALIZE(extra)                       DOC_DSCR("Extra data associated with the transaction.") DOC_EXMP_AUTO(1) DOC_END
      KV_SERIALIZE(attachments)                 DOC_DSCR("Additional attachments to the transaction.") DOC_EXMP_AUTO(1) DOC_END
      KV_SERIALIZE_BLOB_AS_BASE64_STRING(object_in_json) DOC_DSCR("Serialized transaction represented in JSON, encoded in Base64.") DOC_EXMP("ewogICJ2ZXJzaW9uIjogMSwgCiAgInZpbiI6IFsgewogICAgIC") DOC_END
    END_KV_SERIALIZE_MAP()
  };

  struct tx_rpc_brief_info
  {
   std::string id;
   uint64_t fee;
   uint64_t total_amount;
   uint64_t sz;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(id)                           DOC_DSCR("Hash of the transaction.") DOC_EXMP("a6e8da986858e6825fce7a192097e6afae4e889cabe853a9c29b964985b23da8") DOC_END
      KV_SERIALIZE(fee)                          DOC_DSCR("Transaction fee in the smallest currency unit.") DOC_EXMP(1000000000) DOC_END
      KV_SERIALIZE(total_amount)                 DOC_DSCR("Total amount transferred in the transaction (legacy, for pre-Zarcanum txs).") DOC_EXMP(9000000000) DOC_END
      KV_SERIALIZE(sz)                           DOC_DSCR("Size of the transaction in bytes.") DOC_EXMP(6142) DOC_END
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
      KV_SERIALIZE(blob)                         DOC_DSCR("Serialized form of the block.") DOC_EXMP("") DOC_END
      KV_SERIALIZE(height)                       DOC_DSCR("Height of the block in the blockchain.") DOC_EXMP(51) DOC_END
      KV_SERIALIZE(timestamp)                    DOC_DSCR("Timestamp when the block was created, in PoS blocks used for mining.") DOC_EXMP(1557345925) DOC_END
      KV_SERIALIZE(actual_timestamp)             DOC_DSCR("Actual timestamp encoded in the block's extra data for PoS blocks.") DOC_EXMP(1557345925) DOC_END
      KV_SERIALIZE(block_cumulative_size)        DOC_DSCR("Cumulative size of the block including all transactions.") DOC_EXMP(6794) DOC_END
      KV_SERIALIZE(total_txs_size)               DOC_DSCR("Total size of all transactions included in the block.") DOC_EXMP(6794) DOC_END
      KV_SERIALIZE(block_tself_size)             // TODO ?
      KV_SERIALIZE(base_reward)                  DOC_DSCR("Base mining reward for the block.") DOC_EXMP(1000000000000) DOC_END
      KV_SERIALIZE(summary_reward)               DOC_DSCR("Total reward for the block, including base reward and transaction fees (legacy).") DOC_EXMP(1001000000000) DOC_END
      KV_SERIALIZE(total_fee)                    DOC_DSCR("Total transaction fees included in the block.") DOC_EXMP(1000000000) DOC_END
      KV_SERIALIZE(penalty)                      DOC_DSCR("Penalty applied to the reward if the block is larger than median but not large enough to be rejected.") DOC_EXMP(0) DOC_END
      KV_SERIALIZE(id)                           DOC_DSCR("Unique identifier of the block.") DOC_EXMP("af05b814c75e10872afc0345108e830884bc4c32091db783505abe3dac9929cf") DOC_END
      KV_SERIALIZE(prev_id)                      DOC_DSCR("Hash of the previous block in the chain.") DOC_EXMP("37fe382c755bb8869e4f5255f2aed6a8fb503e195bb4180b65b8e1450b84cafe") DOC_END
      KV_SERIALIZE(pow_seed)                     // TODO
      KV_SERIALIZE(cumulative_diff_adjusted)     DOC_DSCR("Adjusted cumulative difficulty of the blockchain up to this block.") DOC_EXMP("42413051198") DOC_END
      KV_SERIALIZE(cumulative_diff_precise)      DOC_DSCR("Precise cumulative difficulty of the blockchain up to this block.") DOC_EXMP("28881828324942") DOC_END
      KV_SERIALIZE(difficulty)                   DOC_DSCR("Mining difficulty of the block.") DOC_EXMP("951296929031") DOC_END
      KV_SERIALIZE(already_generated_coins)      DOC_DSCR("Total amount of coins generated in the blockchain up to this block.") DOC_EXMP("17517253670000000000") DOC_END
      KV_SERIALIZE(this_block_fee_median)        DOC_DSCR("Median transaction fee of the transactions within this block.") DOC_EXMP(1000000000) DOC_END
      KV_SERIALIZE(effective_fee_median)         // TODO
      KV_SERIALIZE(transactions_details)         DOC_DSCR("Detailed information about each transaction included in the block.") DOC_EXMP_AUTO(1) DOC_END
      KV_SERIALIZE(type)                         DOC_DSCR("Type of the block.") DOC_EXMP(1) DOC_END
      KV_SERIALIZE(is_orphan)                    DOC_DSCR("Indicates whether the block is an orphan.") DOC_EXMP(false) DOC_END
      KV_SERIALIZE(miner_text_info)              DOC_DSCR("Additional textual information provided by the miner of the block.") DOC_EXMP("") DOC_END
      KV_SERIALIZE(object_in_json)               DOC_DSCR("Serialized representation of the block in JSON format.") DOC_END
    END_KV_SERIALIZE_MAP()
  };

  //-----------------------------------------------


  struct COMMAND_RPC_GET_BLOCKS_DETAILS
  {
    DOC_COMMAND("Retrieves detailed information about a sequence of blocks starting from a specific height.");

    struct request
    {
      uint64_t height_start;
      uint64_t count;
      bool ignore_transactions;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height_start)               DOC_DSCR("The starting block height from which block details are retrieved.") DOC_EXMP(51) DOC_END
        KV_SERIALIZE(count)                      DOC_DSCR("The number of blocks to retrieve from the starting height.") DOC_EXMP(1) DOC_END
        KV_SERIALIZE(ignore_transactions)        // TODO
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::list<block_rpc_extended_info> blocks;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
        KV_SERIALIZE(blocks)                     DOC_DSCR("List of blocks with detailed information, starting from the specified height.")  DOC_EXMP_AUTO(1) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_RPC_GET_ALT_BLOCKS_DETAILS
  {
    DOC_COMMAND("Retrieves details of alternative blocks in the blockchain, allowing for pagination through large datasets.");

    struct request
    {
      uint64_t offset;
      uint64_t count;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(offset)                     DOC_DSCR("The offset in the list of alternative blocks from which to start retrieval.") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(count)                      DOC_DSCR("The number of alternative blocks to retrieve from the specified offset.") DOC_EXMP(1) DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::vector<block_rpc_extended_info> blocks;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
        KV_SERIALIZE(blocks)                     DOC_DSCR("List of alternative blocks with detailed information, retrieved based on the specified parameters.") DOC_EXMP_AUTO(1) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_RPC_GET_BLOCK_DETAILS
  {
    DOC_COMMAND("Retrieves detailed information about a specific block identified by its hash.");

    struct request
    {
      crypto::hash id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(id)       DOC_DSCR("The hash ID of the block for which detailed information is being requested.") DOC_EXMP("4cf2c7c7e16d1a2a0771cd552c696dd94e7db4e1031982ed88eff99d5006ee4a") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      block_rpc_extended_info block_details;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
        KV_SERIALIZE(block_details)              DOC_DSCR("Detailed information about the block retrieved based on the provided hash ID.") DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_RPC_GET_POOL_TXS_DETAILS
  {
    DOC_COMMAND("Retrieves detailed information about specific transactions in the transaction pool, identified by their IDs.");

    struct request
    {
      std::list<std::string> ids;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(ids)                        DOC_DSCR("List of transaction IDs.") DOC_EXMP_AGGR("bd9a89f95c9115d29540c6778dab9d9798eb251143dcd4b8960fcd9730a1471c","1c938f04c935d976310c4338fc570ea20777951471609f3edecb341ea4932b0a") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::list<tx_rpc_extended_info> txs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
        KV_SERIALIZE(txs)                        DOC_DSCR("List of transactions with detailed information.") DOC_EXMP_AUTO(1) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_RPC_GET_POOL_TXS_BRIEF_DETAILS
  {
    DOC_COMMAND("Retrieves brief details about specific transactions in the transaction pool, identified by their IDs.");

    struct request
    {
      std::list<std::string> ids;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(ids)                        DOC_DSCR("List of transaction IDs.") DOC_EXMP_AGGR("bd9a89f95c9115d29540c6778dab9d9798eb251143dcd4b8960fcd9730a1471c","1c938f04c935d976310c4338fc570ea20777951471609f3edecb341ea4932b0a") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::list<tx_rpc_brief_info> txs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
        KV_SERIALIZE(txs)                        DOC_DSCR("List of transactions with detailed information.") DOC_EXMP_AUTO(1) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_RPC_GET_ALL_POOL_TX_LIST
  {
    DOC_COMMAND("Retrieves a list of all transaction IDs currently in the transaction pool.");

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
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
        KV_SERIALIZE(ids)                        DOC_DSCR("List of all transaction IDs currently in the transaction pool.") DOC_EXMP_AUTO(1) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };
  
  //-----------------------------------------------

  // TODO looks like it is never used, a typo?
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

  //-----------------------------------------------

  struct COMMAND_RPC_GET_TX_DETAILS
  {
    DOC_COMMAND("Retrieves detailed information about a specific transaction.");

    struct request
    {
      std::string tx_hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)                    DOC_DSCR("The hash of the transaction for which detailed information is being requested.") DOC_EXMP("d46c415c3aa3f3e17bd0bf85ffb813cacf4d9595d2d5392f42dacbaffcffdc70")  DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      tx_rpc_extended_info tx_info;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
        KV_SERIALIZE(tx_info)                    DOC_DSCR("Detailed information about the transaction.") DOC_EXMP_AUTO() DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct search_entry
  {
    std::string type;
  };

  struct COMMAND_RPC_SERARCH_BY_ID
  {
    DOC_COMMAND("Searches for a given ID across various entity types such as blocks, transactions, key images, multisig outputs, and alternative blocks, useful when the entity type is unknown or unspecified.");

    struct request
    {
      std::string id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(id)                         DOC_DSCR("The identifier used to search across various types of entities.") DOC_EXMP("729811f9340537e8d5641949e6cc58261f91f109687a706f39bae9514757e819") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::list<std::string> types_found;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
        KV_SERIALIZE(types_found)                DOC_DSCR("List of entity types where the identifier was found.") DOC_EXMP_AGGR("key_image") DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_RPC_GET_OFFERS_EX
  {
    DOC_COMMAND("Fetch from daemon offers listed in the marketplace with given filters");

    struct request
    {
      bc_services::core_offers_filter filter;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(filter)                     DOC_DSCR("Filter options.") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::list<bc_services::offer_details_ex> offers;
      uint64_t total_offers;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the operation.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
        KV_SERIALIZE(offers)                     DOC_DSCR("List of offers related to the operation.") DOC_EXMP_AUTO(1) DOC_END
        KV_SERIALIZE(total_offers)               DOC_DSCR("Total number of offers.") DOC_EXMP(1) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN
  {
    DOC_COMMAND("Retrieves the current core transaction expiration median.");

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
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
        KV_SERIALIZE(expiration_median)          DOC_DSCR("The median timestamp from the last N blocks, used to determine if transactions are expired based on their timestamp.") DOC_EXMP(1719591540) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct COMMAND_VALIDATE_SIGNATURE
  {
    DOC_COMMAND("Validates a Schnorr signature for arbitrary data. The public key for verification is provided directly or retrieved using an associated alias.");

    struct request
    {
      std::string buff; //base64 encoded data
      crypto::signature sig = currency::null_sig;
      crypto::public_key pkey = currency::null_pkey;
      std::string alias;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(buff)                       DOC_DSCR("Base64 encoded data for which the signature is to be validated.") DOC_EXMP("SSBkaWRuJ3QgZXhwZWN0IGFueW9uZSB0byBkZWNyeXB0IHRoaXMgZGF0YSwgc2luY2UgaXQncyBqdXN0IGFuIGV4YW1wbGUuIEJ1dCB5b3UgZGVjcnlwdGVkIGl0ISBJJ20gYW1hemVkLg==") DOC_END
        KV_SERIALIZE_POD_AS_HEX_STRING(sig)      DOC_DSCR("Schnorr signature to validate, encoded as a hexadecimal string.") DOC_EXMP("5c202d4bf82c2dd3c6354e2f02826ca72c797950dbe8db5bc5e3b2e60290a407ac2ef85bfc905ace8fe3b3819217084c00faf7237fee3ad2f6a7f662636cd20f") DOC_END
        KV_SERIALIZE_POD_AS_HEX_STRING(pkey)     DOC_DSCR("Public key used for signature verification, encoded as a hexadecimal string. If null or not set, the public key is retrieved using the provided alias.") DOC_END
        KV_SERIALIZE(alias)                      DOC_DSCR("Alias to retrieve the associated public spend key if no explicit public key is provided for verification.") DOC_EXMP("sowle") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call.") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------

  struct void_struct
  {
    BEGIN_KV_SERIALIZE_MAP()
    END_KV_SERIALIZE_MAP()
  };

}

