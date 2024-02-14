// Copyright (c) 2014-2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 

namespace pre_hf4_api
{
  struct COMMAND_RPC_MAKE_INTEGRATED_ADDRESS
  {
    struct request
    {
      std::string payment_id; // hex-encoded

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payment_id)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string integrated_address;
      std::string payment_id; // hex-encoded

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(integrated_address)
        KV_SERIALIZE(payment_id)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS
  {
    struct request
    {
      std::string integrated_address;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(integrated_address)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string standard_address;
      std::string payment_id; // hex-encoded

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(standard_address)
        KV_SERIALIZE(payment_id)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct transfer_destination
  {
    uint64_t amount;
    std::string address;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE(address)
    END_KV_SERIALIZE_MAP()
  };

  struct tx_service_attachment
  {
    std::string service_id;    //string identifying service which addressed this attachment
    std::string instruction;   //string identifying specific instructions for service/way to interpret data
    std::string body;          //any data identifying service, options etc
    std::vector<crypto::public_key> security; //some of commands need proof of owner
    uint8_t flags;             //special flags (ex: TX_SERVICE_ATTACHMENT_ENCRYPT_BODY), see below

    BEGIN_SERIALIZE()
      FIELD(service_id)
      FIELD(instruction)
      FIELD(body)
      FIELD(security)
      FIELD(flags)
      END_SERIALIZE()

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(service_id)
      KV_SERIALIZE(instruction)
      KV_SERIALIZE_BLOB_AS_HEX_STRING(body)
      KV_SERIALIZE_CONTAINER_POD_AS_BLOB(security)
      KV_SERIALIZE(flags)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_TRANSFER
  {
    struct request
    {
      std::list<transfer_destination> destinations;
      uint64_t fee;
      uint64_t mixin;
      //uint64_t unlock_time;
      std::string payment_id; // hex-encoded
      std::string comment;
      bool push_payer;
      bool hide_receiver;
      std::vector<currency::tx_service_attachment> service_entries;
      bool service_entries_permanent;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(destinations)
        KV_SERIALIZE(fee)
        KV_SERIALIZE(mixin)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(comment)
        KV_SERIALIZE(push_payer)
        KV_SERIALIZE(hide_receiver)
        KV_SERIALIZE(service_entries)
        KV_SERIALIZE(service_entries_permanent)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string tx_hash;
      std::string tx_unsigned_hex; // for cold-signing process
      uint64_t tx_size;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
        KV_SERIALIZE(tx_unsigned_hex)
        KV_SERIALIZE(tx_size)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct wallet_transfer_info_details
  {
    std::list<uint64_t> rcv;
    std::list<uint64_t> spn;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(rcv)
      KV_SERIALIZE(spn)
    END_KV_SERIALIZE_MAP()

  };

  struct wallet_transfer_info
  {
    uint64_t      amount;
    uint64_t      timestamp;
    crypto::hash  tx_hash;
    uint64_t      height;          //if height == 0 then tx is unconfirmed
    uint64_t      unlock_time;
    uint32_t      tx_blob_size;
    std::string   payment_id;
    std::vector<std::string> remote_addresses;  //optional
    std::vector<std::string> recipients_aliases; //optional
    std::string   comment;
    bool          is_income;
    bool          is_service;
    bool          is_mixing;
    bool          is_mining;
    uint64_t      tx_type;
    wallet_transfer_info_details td;
    std::vector<tx_service_attachment> service_entries;
    //not included in streaming serialization
    uint64_t      fee;
    bool          show_sender;
    //std::vector<escrow_contract_details> contract; //not traxking this part
    uint16_t      extra_flags;
    uint64_t      transfer_internal_index;


    //not included in kv serialization map
    currency::transaction tx;
    std::vector<uint64_t> selected_indicies;
    std::list<bc_services::offers_attachment_t> marketplace_entries;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE_POD_AS_HEX_STRING(tx_hash)
      KV_SERIALIZE(height)
      KV_SERIALIZE(unlock_time)
      KV_SERIALIZE(tx_blob_size)
      KV_SERIALIZE_BLOB_AS_HEX_STRING(payment_id)
      KV_SERIALIZE(remote_addresses)
      KV_SERIALIZE(recipients_aliases)
      KV_SERIALIZE(comment)
      KV_SERIALIZE(is_income)
      KV_SERIALIZE(timestamp)
      KV_SERIALIZE(td)
      KV_SERIALIZE(fee)
      KV_SERIALIZE(is_service)
      KV_SERIALIZE(is_mixing)
      KV_SERIALIZE(is_mining)
      KV_SERIALIZE(tx_type)
      KV_SERIALIZE(show_sender)
      //KV_SERIALIZE(contract)
      KV_SERIALIZE(service_entries)
      KV_SERIALIZE(transfer_internal_index)
    END_KV_SERIALIZE_MAP()
  };

  struct wallet_provision_info
  {
    uint64_t                  transfers_count;
    uint64_t                  transfer_entries_count;
    uint64_t                  balance;
    uint64_t                  unlocked_balance;
    uint64_t                  curent_height;


    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(transfers_count)
      KV_SERIALIZE(transfer_entries_count)
      KV_SERIALIZE(balance)
      KV_SERIALIZE(unlocked_balance)
      KV_SERIALIZE(curent_height)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_GET_RECENT_TXS_AND_INFO
  {
    struct request
    {

      /*
      if offset is 0, then GET_RECENT_TXS_AND_INFO return
      unconfirmed transactions as the first first items of "transfers",
      this unconfirmed transactions is not counted regarding "count" parameter
      */
      uint64_t offset;
      uint64_t count;

      /*
      need_to_get_info - should backend re-calculate balance(could be relatively heavy,
      and not needed when getting long tx history with multiple calls
      of GET_RECENT_TXS_AND_INFO with offsets)
      */
      bool update_provision_info;
      bool exclude_mining_txs;
      bool exclude_unconfirmed;
      std::string order; // "FROM_BEGIN_TO_END" or "FROM_END_TO_BEGIN"

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(offset)
        KV_SERIALIZE(count)
        KV_SERIALIZE(update_provision_info)
        KV_SERIALIZE(exclude_mining_txs)
        KV_SERIALIZE(exclude_unconfirmed)
        KV_SERIALIZE(order)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      wallet_provision_info pi;
      std::vector<wallet_transfer_info> transfers;
      uint64_t total_transfers;
      uint64_t last_item_index;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(pi)
        KV_SERIALIZE(transfers)
        KV_SERIALIZE(total_transfers)
        KV_SERIALIZE(last_item_index)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_RPC_GET_BALANCE
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t 	 balance;
      uint64_t 	 unlocked_balance;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(balance)
        KV_SERIALIZE(unlocked_balance)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_RPC_GET_WALLET_INFO
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string               address;
      std::string               path;
      uint64_t                  transfers_count;
      uint64_t                  transfer_entries_count;
      bool                      is_whatch_only;
      std::vector<std::string>  utxo_distribution;
      uint64_t                  current_height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(path)
        KV_SERIALIZE(transfers_count)
        KV_SERIALIZE(transfer_entries_count)
        KV_SERIALIZE(is_whatch_only)
        KV_SERIALIZE(utxo_distribution)
        KV_SERIALIZE(current_height)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_RPC_SEARCH_FOR_TRANSACTIONS
  {
    struct request
    {
      crypto::hash tx_id;
      bool in;
      bool out;
      //bool pending;
      //bool failed;
      bool pool;
      bool filter_by_height;
      uint64_t min_height;
      uint64_t max_height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(tx_id)
        KV_SERIALIZE(in)
        KV_SERIALIZE(out)
        //KV_SERIALIZE(pending)
        //KV_SERIALIZE(failed)
        KV_SERIALIZE(pool)
        KV_SERIALIZE(filter_by_height)
        KV_SERIALIZE(min_height)
        KV_SERIALIZE(max_height)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<wallet_transfer_info> in;
      std::list<wallet_transfer_info> out;
      //std::list<wallet_transfer_info> pending;
      //std::list<wallet_transfer_info> failed;
      std::list<wallet_transfer_info> pool;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(in)
        KV_SERIALIZE(out)
        //KV_SERIALIZE(pending)
        //KV_SERIALIZE(failed)
        KV_SERIALIZE(pool)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct payment_details
  {
    std::string payment_id;
    std::string tx_hash;
    uint64_t amount;
    uint64_t block_height;
    uint64_t unlock_time;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(payment_id)
      KV_SERIALIZE(tx_hash)
      KV_SERIALIZE(amount)
      KV_SERIALIZE(block_height)
      KV_SERIALIZE(unlock_time)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_GET_PAYMENTS
  {
    struct request
    {
      std::string payment_id; // hex-encoded
      bool allow_locked_transactions;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(allow_locked_transactions)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<payment_details> payments;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payments)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_BULK_PAYMENTS
  {
    struct request
    {
      std::vector<std::string> payment_ids;
      uint64_t min_block_height;
      bool allow_locked_transactions;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payment_ids)
        KV_SERIALIZE(min_block_height)
        KV_SERIALIZE(allow_locked_transactions)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<payment_details> payments;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payments)
      END_KV_SERIALIZE_MAP()
    };
  };

}
