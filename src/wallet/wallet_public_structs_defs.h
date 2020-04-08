// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <vector>
#include "currency_protocol/currency_protocol_defs.h"
#include "currency_core/currency_basic.h"
#include "crypto/hash.h"
#include "wallet_rpc_server_error_codes.h"
#include "currency_core/offers_service_basics.h"
#include "currency_core/bc_escrow_service.h"
#include "rpc/core_rpc_server_commands_defs.h"

namespace tools
{
namespace wallet_public
{
#define WALLET_RPC_STATUS_OK      "OK"
#define WALLET_RPC_STATUS_BUSY    "BUSY"


  struct wallet_transfer_info_details
  {
    std::list<uint64_t> rcv;
    std::list<uint64_t> spn;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(rcv)
      KV_SERIALIZE(spn)
    END_KV_SERIALIZE_MAP()

  };

  struct escrow_contract_details_basic
  {
    enum contract_state
    {
      proposal_sent = 1,
      contract_accepted = 2,
      contract_released_normal = 3,
      contract_released_burned = 4,
      contract_cancel_proposal_sent = 5, 
      contract_released_cancelled = 6
    };

    uint32_t state;
    bool is_a; // "a" - supposed  to be a buyer
    bc_services::contract_private_details private_detailes;
    uint64_t expiration_time;
    uint64_t cancel_expiration_time;
    uint64_t timestamp;
    uint64_t height; // height of the last state change
    std::string payment_id;

   
    //is not kv_serialization map
    bc_services::proposal_body proposal;
    bc_services::escrow_relese_templates_body release_body;
    bc_services::escrow_cancel_templates_body cancel_body;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(state)
      KV_SERIALIZE(is_a)
      KV_SERIALIZE(expiration_time)
      KV_SERIALIZE(cancel_expiration_time)
      KV_SERIALIZE(timestamp)
      KV_SERIALIZE(height)
      KV_SERIALIZE(payment_id)
      KV_SERIALIZE(private_detailes) 
    END_KV_SERIALIZE_MAP()
  };
  
  struct escrow_contract_details : public escrow_contract_details_basic
  {
    crypto::hash contract_id; //multisig id

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_POD_AS_HEX_STRING(contract_id)
      KV_CHAIN_BASE(escrow_contract_details_basic)
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
    //not included in streaming serialization
    uint64_t      fee;
    bool          show_sender;
    std::vector<escrow_contract_details> contract;
    
    //not included in kv serialization map
    currency::transaction tx;
    std::vector<uint64_t> selected_indicies;
    std::list<bc_services::offers_attachment_t> srv_attachments;

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
      KV_SERIALIZE(contract)
    END_KV_SERIALIZE_MAP()
  };

  struct contracts_array
  {
    std::vector<escrow_contract_details> contracts;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(contracts)
    END_KV_SERIALIZE_MAP()
  };



  struct mining_history_entry
  {
    uint64_t a;
    uint64_t t;
    uint64_t h;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(a)
      KV_SERIALIZE(t)
      KV_SERIALIZE(h)
    END_KV_SERIALIZE_MAP()

  };

  struct mining_history
  {
    std::vector<mining_history_entry> mined_entries;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(mined_entries)
    END_KV_SERIALIZE_MAP()
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

  struct COMMAND_RPC_GET_ADDRESS
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string   address;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
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
      std::string               seed;
      uint64_t                  transfers_count;
      uint64_t                  transfer_entries_count;
      bool                      is_whatch_only;
      std::vector<std::string>  utxo_distribution;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(path)
        KV_SERIALIZE(seed)
        KV_SERIALIZE(transfers_count)
        KV_SERIALIZE(transfer_entries_count)
        KV_SERIALIZE(is_whatch_only)
        KV_SERIALIZE(utxo_distribution)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct wallet_provision_info
  {
    uint64_t                  transfers_count;
    uint64_t                  transfer_entries_count;
    uint64_t                  balance;
    uint64_t                  unlocked_balance;


    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(transfers_count)
      KV_SERIALIZE(transfer_entries_count)
      KV_SERIALIZE(balance)
      KV_SERIALIZE(unlocked_balance)
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

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(offset)
        KV_SERIALIZE(count)
        KV_SERIALIZE(update_provision_info)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      wallet_provision_info pi;
      std::vector<wallet_transfer_info> transfers;
      uint64_t total_transfers;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(pi)
        KV_SERIALIZE(transfers)
        KV_SERIALIZE(total_transfers)
      END_KV_SERIALIZE_MAP()
    };
  };



  
  struct trnsfer_destination
  {
    uint64_t amount;
    std::string address;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE(address)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_TRANSFER
  {
    struct request
    {
      std::list<trnsfer_destination> destinations;
      uint64_t fee;
      uint64_t mixin;
      //uint64_t unlock_time;
      std::string payment_id; // hex-encoded
      std::string comment; 

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(destinations)
        KV_SERIALIZE(fee)
        KV_SERIALIZE(mixin)
        //KV_SERIALIZE(unlock_time)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(comment)
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

  struct COMMAND_RPC_STORE
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
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

  struct COMMAND_SWEEP_BELOW
  {
    struct request
    {
      uint64_t    mixin;
      std::string address;
      uint64_t    amount;
      std::string payment_id_hex;
      uint64_t    fee;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(mixin)
        KV_SERIALIZE(address)
        KV_SERIALIZE(amount)
        KV_SERIALIZE(payment_id_hex)
        KV_SERIALIZE(fee)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string   tx_hash;
      std::string   tx_unsigned_hex;
      uint64_t      outs_total;
      uint64_t      amount_total;
      uint64_t      outs_swept;
      uint64_t      amount_swept;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
        KV_SERIALIZE(tx_unsigned_hex)
        KV_SERIALIZE(outs_total)
        KV_SERIALIZE(amount_total)
        KV_SERIALIZE(outs_swept)
        KV_SERIALIZE(amount_swept)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_SIGN_TRANSFER
  {
    struct request
    {
      std::string     tx_unsigned_hex;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_unsigned_hex)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string     tx_signed_hex;
      std::string     tx_hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_signed_hex)
        KV_SERIALIZE(tx_hash)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_SUBMIT_TRANSFER
  {
    struct request
    {
      //std::string     tx_unsigned_hex;
      std::string     tx_signed_hex;

      BEGIN_KV_SERIALIZE_MAP()
        //KV_SERIALIZE(tx_unsigned_hex)
        KV_SERIALIZE(tx_signed_hex)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string     tx_hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
      END_KV_SERIALIZE_MAP()
    };
  };

  /*stay-alone instance*/
  struct telepod
  {
    std::string account_keys_hex;
    std::string basement_tx_id_hex;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(account_keys_hex)
      KV_SERIALIZE(basement_tx_id_hex)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_MAKETELEPOD
  {
    struct request
    {
      uint64_t amount;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amount)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status; //"OK", "INSUFFICIENT_COINS", "INTERNAL_ERROR"
      telepod tpd;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(tpd)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_RPC_TELEPODSTATUS
  {
    struct request
    {
      telepod tpd;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tpd)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;  //"OK", "UNCONFIRMED", "BAD", "SPENT", "INTERNAL_ERROR"

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
     END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_CLONETELEPOD
  {
    struct request
    {
      telepod tpd;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tpd)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;//"OK", "UNCONFIRMED", "BAD", "SPENT", "INTERNAL_ERROR:"
      telepod tpd;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(tpd)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_WITHDRAWTELEPOD
  {
    struct request
    {
      telepod tpd;
      std::string addr;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tpd)
        KV_SERIALIZE(addr)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;  //"OK", "UNCONFIRMED", "BAD", "SPENT", "INTERNAL_ERROR", "BAD_ADDRESS"
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct create_proposal_param
  {
//    uint64_t wallet_id;
    bc_services::contract_private_details details;
    std::string payment_id;
    uint64_t expiration_period;
    uint64_t fee;
    uint64_t b_fee;
    uint64_t fake_outputs_count;
    uint64_t unlock_time;

    BEGIN_KV_SERIALIZE_MAP()
//      KV_SERIALIZE(wallet_id)
      KV_SERIALIZE(details)
      KV_SERIALIZE(payment_id)
      KV_SERIALIZE(expiration_period)
      KV_SERIALIZE(fee)
      KV_SERIALIZE(b_fee)
      KV_SERIALIZE(fake_outputs_count)
      KV_SERIALIZE(unlock_time)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_CONTRACTS_SEND_PROPOSAL
  {
    typedef create_proposal_param request;

    struct response
    {
      std::string status;  //"OK", "UNCONFIRMED", "BAD", "SPENT", "INTERNAL_ERROR", "BAD_ADDRESS"
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };
  


  struct COMMAND_CONTRACTS_ACCEPT_PROPOSAL
  {
    struct request
    {
      crypto::hash contract_id;
      uint64_t acceptance_fee;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(acceptance_fee)
        KV_SERIALIZE_POD_AS_HEX_STRING(contract_id)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };
  
  struct COMMAND_CONTRACTS_GET_ALL
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    typedef contracts_array response;
  };

  struct COMMAND_CONTRACTS_RELEASE
  {
    struct request
    {
      crypto::hash contract_id;
      std::string release_type;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(release_type)
        KV_SERIALIZE_POD_AS_HEX_STRING(contract_id)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_CONTRACTS_REQUEST_CANCEL
  {
    struct request
    {
      crypto::hash contract_id;
      uint64_t expiration_period;
      uint64_t fee;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(expiration_period)
        KV_SERIALIZE(fee)
        KV_SERIALIZE_POD_AS_HEX_STRING(contract_id)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_CONTRACTS_ACCEPT_CANCEL
  {
    struct request
    {
      crypto::hash contract_id;


      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(contract_id)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  //--------------------
  typedef currency::COMMAND_RPC_GET_OFFERS_EX COMMAND_MARKETPLACE_GET_MY_OFFERS;

  struct COMMAND_MARKETPLACE_PUSH_OFFER
  {
    struct request
    {
      bc_services::offer_details_ex od;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(od)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string tx_hash;
      uint64_t    tx_blob_size;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
        KV_SERIALIZE(tx_blob_size)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_MARKETPLACE_PUSH_UPDATE_OFFER
  {
    struct request
    {
      crypto::hash tx_id;
      uint64_t no;
      bc_services::offer_details_ex od;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(tx_id)
        KV_SERIALIZE(no)
        KV_SERIALIZE(od)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string tx_hash;
      uint64_t    tx_blob_size;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
        KV_SERIALIZE(tx_blob_size)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_MARKETPLACE_CANCEL_OFFER
  {
    struct request
    {
      crypto::hash tx_id;
      uint64_t no;
      uint64_t fee;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(tx_id)
        KV_SERIALIZE(no)
        KV_SERIALIZE(fee)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string tx_hash;
      uint64_t    tx_blob_size;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
        KV_SERIALIZE(tx_blob_size)
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



  inline std::string get_escrow_contract_state_name(uint32_t state)
  {
    switch (state)
    {
    case escrow_contract_details_basic::proposal_sent:
      return "proposal_sent("                 + epee::string_tools::num_to_string_fast(state) + ")";
    case escrow_contract_details_basic::contract_accepted:
      return "contract_accepted("             + epee::string_tools::num_to_string_fast(state) + ")";
    case escrow_contract_details_basic::contract_released_normal:
      return "contract_released_normal("      + epee::string_tools::num_to_string_fast(state) + ")";
    case escrow_contract_details_basic::contract_released_burned:
      return "contract_released_burned("      + epee::string_tools::num_to_string_fast(state) + ")";
    case escrow_contract_details_basic::contract_cancel_proposal_sent:
      return "contract_cancel_proposal_sent(" + epee::string_tools::num_to_string_fast(state) + ")";
    case escrow_contract_details_basic::contract_released_cancelled:
      return "contract_released_cancelled("   + epee::string_tools::num_to_string_fast(state) + ")";
    default:
      return "unknown_state("                 + epee::string_tools::num_to_string_fast(state) + ")";
    }
  }


} // namespace wallet_rpc
} // namespace tools
