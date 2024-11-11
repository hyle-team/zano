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
#include "currency_protocol/blobdatatype.h"


const uint64_t WALLET_GLOBAL_OUTPUT_INDEX_UNDEFINED = std::numeric_limits<uint64_t>::max();

const boost::uuids::uuid RPC_INTERNAL_UI_CONTEXT = {0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 2, 1, 0, 0}; //Bender's nightmare 

namespace tools
{
namespace wallet_public
{
#define WALLET_RPC_STATUS_OK      "OK"
#define WALLET_RPC_STATUS_BUSY    "BUSY"


  struct employed_tx_entry {
    uint64_t index = 0;
    uint64_t amount = 0;
    crypto::public_key asset_id = currency::native_coin_asset_id;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(index)
      KV_SERIALIZE(amount)
      KV_SERIALIZE_POD_AS_HEX_STRING(asset_id)
    END_KV_SERIALIZE_MAP()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(index)
      BOOST_SERIALIZE(amount)
      BOOST_SERIALIZE(asset_id)
    END_BOOST_SERIALIZATION()

  };

  struct employed_tx_entries
  {
    std::vector<employed_tx_entry> receive;
    std::vector<employed_tx_entry> spent;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(receive)
      KV_SERIALIZE(spent)
    END_KV_SERIALIZE_MAP()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(receive)
      BOOST_SERIALIZE(spent)
    END_BOOST_SERIALIZATION()
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


#define WALLET_TRANSFER_INFO_FLAGS_HTLC_DEPOSIT   static_cast<uint16_t>(1 << 0)


  struct wallet_sub_transfer_info
  {
    uint64_t      amount = 0;
    bool          is_income = false;
    crypto::public_key asset_id = currency::native_coin_asset_id; // not blinded, not premultiplied by 1/8

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)      DOC_DSCR("Amount of asset the had been transfered")        DOC_EXMP(1000000000000)   DOC_END
      KV_SERIALIZE(is_income)   DOC_DSCR("Indicates if transfer was income our outgoing")  DOC_EXMP(false)           DOC_END
      KV_SERIALIZE_POD_AS_HEX_STRING(asset_id) DOC_DSCR("Asset id")  DOC_EXMP("cc608f59f8080e2fbfe3c8c80eb6e6a953d47cf2d6aebd345bada3a1cab99852")   DOC_END
    END_KV_SERIALIZE_MAP()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(amount) 
      BOOST_SERIALIZE(is_income)
      BOOST_SERIALIZE(asset_id)
    END_BOOST_SERIALIZATION()

  };

  struct wallet_transfer_info
  {
    uint64_t      timestamp = 0;
    crypto::hash  tx_hash = currency::null_hash;
    uint64_t      height = 0;          //if height == 0 then tx is unconfirmed
    uint64_t      unlock_time = 0;
    uint32_t      tx_blob_size = 0;
    std::string   payment_id;
    std::string   comment;
    bool          is_service = false;
    bool          is_mixing = false;
    bool          is_mining = false;
    uint64_t      tx_type = 0;
    employed_tx_entries employed_entries;
    std::vector<currency::tx_service_attachment> service_entries;
    std::vector<std::string> remote_addresses;  //optional
    std::vector<std::string> remote_aliases; //optional, describe only if there only one remote address
    std::vector<wallet_sub_transfer_info> subtransfers;

    //not included in streaming serialization
    uint64_t      fee = 0;
    bool          show_sender = false;
    std::vector<escrow_contract_details> contract;
    uint16_t      extra_flags = 0;
    uint64_t      transfer_internal_index = 0;
   
    //not included in kv serialization map
    currency::transaction tx;
    std::vector<uint64_t> selected_indicies;
    std::list<bc_services::offers_attachment_t> marketplace_entries;



    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_POD_AS_HEX_STRING(tx_hash)     DOC_DSCR("Transaction ID(hash)")  DOC_EXMP("5509650e12c8f901e6731a2bfaf3abfd64409e3e1366d3d94cd11db8beddb0c3")   DOC_END
      KV_SERIALIZE(height)                        DOC_DSCR("Height of the block that included transaction(0 i  transaction is unconfirmed)")  DOC_EXMP(0)   DOC_END
      KV_SERIALIZE(unlock_time)                   DOC_DSCR("Unlock time of this transfer (if present)")  DOC_EXMP(0)   DOC_END
      KV_SERIALIZE(tx_blob_size)                  DOC_DSCR("Size of transaction in bytes")  DOC_EXMP(0)   DOC_END
      KV_SERIALIZE_BLOB_AS_HEX_STRING(payment_id) DOC_DSCR("HEX-encoded payment id blob, if it was present")  DOC_EXMP("00000000ff00ff00")   DOC_END
      KV_SERIALIZE(comment)                       DOC_DSCR("Some human-readable comment")  DOC_EXMP("Comment here")   DOC_END
      KV_SERIALIZE(timestamp)                     DOC_DSCR("Timestamp of the block that included transaction in blockchain, 0 for unconfirmed")  DOC_EXMP(1712590951)   DOC_END
      KV_SERIALIZE(employed_entries)              DOC_DSCR("Mark entries from transaction that was connected to this wallet")  DOC_END
      KV_SERIALIZE(fee)                           DOC_DSCR("Transaction fee")  DOC_EXMP(10000000000)   DOC_END
      KV_SERIALIZE(is_service)                    DOC_DSCR("Tells if this transaction is used as utility by one of Zano services(contracts, ionic swaps, etc)")  DOC_EXMP(false)   DOC_END
      KV_SERIALIZE(is_mixing)                     DOC_DSCR("Tells if this transaction using mixins or not(auditble wallets normally don't use mixins)")  DOC_EXMP(false)   DOC_END
      KV_SERIALIZE(is_mining)                     DOC_DSCR("Tells if this transaction is coinbase transaction(ie generated by PoW mining or by PoS staking)")  DOC_EXMP(false)   DOC_END
      KV_SERIALIZE(tx_type)                       DOC_DSCR("Could be one of this:  GUI_TX_TYPE_NORMAL=0, GUI_TX_TYPE_PUSH_OFFER=1, GUI_TX_TYPE_UPDATE_OFFER=2, GUI_TX_TYPE_CANCEL_OFFER=3, GUI_TX_TYPE_NEW_ALIAS=4,GUI_TX_TYPE_UPDATE_ALIAS=5,GUI_TX_TYPE_COIN_BASE=6,GUI_TX_TYPE_ESCROW_PROPOSAL=7,GUI_TX_TYPE_ESCROW_TRANSFER=8,GUI_TX_TYPE_ESCROW_RELEASE_NORMAL=9,GUI_TX_TYPE_ESCROW_RELEASE_BURN=10,GUI_TX_TYPE_ESCROW_CANCEL_PROPOSAL=11,GUI_TX_TYPE_ESCROW_RELEASE_CANCEL=12,GUI_TX_TYPE_HTLC_DEPOSIT=13,GUI_TX_TYPE_HTLC_REDEEM=14")  DOC_EXMP(0)   DOC_END
      KV_SERIALIZE(show_sender)                   DOC_DSCR("If sender is included in tx")  DOC_EXMP(false)   DOC_END
      KV_SERIALIZE(contract)                      DOC_DSCR("Escrow contract if it's part of transaction")  DOC_EXMP_AUTO(1)   DOC_END
      KV_SERIALIZE(service_entries)               DOC_DSCR("Additional entries that might be stored in transaction but not part of it's consensus")  DOC_EXMP_AUTO(1)   DOC_END
      KV_SERIALIZE(transfer_internal_index)       DOC_DSCR("Index of this entry in the wallet's array of transaction's history")  DOC_EXMP(12)   DOC_END
      KV_SERIALIZE(remote_addresses)              DOC_DSCR("Remote addresses of this transfer(destination if it's outgoing transfer or sender if it's incoming transaction)")  DOC_EXMP_AUTO(1, "ZxBvJDuQjMG9R2j4WnYUhBYNrwZPwuyXrC7FHdVmWqaESgowDvgfWtiXeNGu8Px9B24pkmjsA39fzSSiEQG1ekB225ZnrMTBp")   DOC_END
      KV_SERIALIZE(remote_aliases)                DOC_DSCR("Aliases for remot addresses, of discovered")  DOC_EXMP_AUTO(1, "roger")    DOC_END
      KV_SERIALIZE(subtransfers)                  DOC_DSCR("Essential part of transfer entry: amounts that been transfered in this transaction grouped by asset id")  DOC_EXMP_AUTO(1)   DOC_END
      
      KV_SERIALIZE_EPHEMERAL_N(currency::asset_descriptor_operation, wallet_transfer_info_get_ado, "ado")   DOC_DSCR("\"Asset Descriptor Operation\" if it was present in transaction")   DOC_END
    END_KV_SERIALIZE_MAP()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(timestamp)
      BOOST_SERIALIZE(tx_hash)
      BOOST_SERIALIZE(height)
      BOOST_SERIALIZE(tx_blob_size)
      BOOST_SERIALIZE(payment_id)
      BOOST_SERIALIZE(remote_addresses)
      BOOST_SERIALIZE(employed_entries)
      BOOST_SERIALIZE(tx)
      BOOST_SERIALIZE(remote_aliases)
      BOOST_SERIALIZE(comment)
      BOOST_SERIALIZE(contract)
      BOOST_SERIALIZE(selected_indicies)
      BOOST_SERIALIZE(marketplace_entries)
      BOOST_SERIALIZE(unlock_time)
      BOOST_SERIALIZE(service_entries)
      BOOST_SERIALIZE(subtransfers)
    END_BOOST_SERIALIZATION()

    bool is_income_mode_encryption() const 
    {
      for (const auto& st : subtransfers)
      {
        if (st.asset_id == currency::native_coin_asset_id)
          return st.is_income;
      }
      return true;
    }
    bool has_outgoing_entries() const
    {
      for (const auto& st : subtransfers)
      {
        if (!st.is_income)
          return true;
      }
      return false;
    }
    uint64_t get_native_income_amount() const
    {
      for (const auto& st : subtransfers)
      {
        if (st.asset_id == currency::native_coin_asset_id && st.is_income)
          return st.amount;
      }
      return 0;
    }
    uint64_t get_native_amount() const
    {
      for (const auto& st : subtransfers)
      {
        if (st.asset_id == currency::native_coin_asset_id )
          return st.amount;
      }
      return 0;
    }
    bool get_native_is_income() const
    {
      for (const auto& st : subtransfers)
      {
        if (st.asset_id == currency::native_coin_asset_id)
          return st.is_income;
      }
      return false;
    }
    uint64_t& get_native_income_amount()
    {
      for (auto& st : subtransfers)
      {
        if (st.asset_id == currency::native_coin_asset_id)
        {
          if (st.is_income)
          {
            return st.amount;
          }
          else
          {
            throw std::runtime_error("Unexpected wallet_transfer_info: native is not income type");
          }
        }
      }
      subtransfers.push_back(wallet_sub_transfer_info());
      subtransfers.back().is_income = true;
      return subtransfers.back().amount;
    }
    static inline bool wallet_transfer_info_get_ado(const wallet_transfer_info& tdb, currency::asset_descriptor_operation& val)
    {
      if (currency::get_type_in_variant_container(tdb.tx.extra, val))
        return true;
       
      return false;
    }
  };

  struct wallet_transfer_info_old : public wallet_transfer_info
  {
    //uint64_t amount = 0;
    //bool is_income = false;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_EPHEMERAL_N(uint64_t, wallet_transfer_info_to_amount, "amount")   DOC_DSCR("Native coins amount")                           DOC_EXMP(1000000000000)   DOC_END
      KV_SERIALIZE_EPHEMERAL_N(bool, wallet_transfer_info_to_is_income, "is_income") DOC_DSCR("If trnasfer entrie is income (taken from native subtransfer)") DOC_EXMP(false)           DOC_END
      //KV_SERIALIZE(amount)
      KV_CHAIN_BASE(wallet_transfer_info)
    END_KV_SERIALIZE_MAP()

    static bool wallet_transfer_info_to_amount(const wallet_transfer_info_old& wtio, uint64_t &val)
    {
      val = wtio.get_native_amount();
      return true;
    }

    static bool wallet_transfer_info_to_is_income(const wallet_transfer_info_old& wtio, bool& val)
    {
      val = wtio.get_native_is_income();
      return true;
    }

  };



  struct asset_balance_entry_base
  {
    uint64_t total = 0;
    uint64_t unlocked = 0;
    uint64_t awaiting_in = 0;
    uint64_t awaiting_out = 0;

    //v2
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(total)         DOC_DSCR("Total coins available(including locked)") DOC_EXMP(100000000000000)    DOC_END
      KV_SERIALIZE(unlocked)      DOC_DSCR("Unlocked coins available(the ones that could be used right now)") DOC_EXMP(50000000000000)    DOC_END
      KV_SERIALIZE(awaiting_in)   DOC_DSCR("Unconfirmed amount for receive") DOC_EXMP(1000000000000)    DOC_END
      KV_SERIALIZE(awaiting_out)  DOC_DSCR("Unconfirmed amount for send")    DOC_EXMP(2000000000000)    DOC_END
    END_KV_SERIALIZE_MAP()
  };

  struct asset_balance_entry : public asset_balance_entry_base
  {
    currency::asset_descriptor_with_id asset_info;
    //v2
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(asset_info)            DOC_DSCR("Asset info details")  DOC_END
      KV_CHAIN_BASE(asset_balance_entry_base)
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
      KV_SERIALIZE(a)   DOC_DSCR("Mined amount(block reward)") DOC_EXMP(1000000000000) DOC_END
      KV_SERIALIZE(t)   DOC_DSCR("Timestamp")                  DOC_EXMP(1712683857) DOC_END
      KV_SERIALIZE(h)   DOC_DSCR("height")                     DOC_EXMP(102000) DOC_END
    END_KV_SERIALIZE_MAP()

  };

  struct mining_history
  {
    std::vector<mining_history_entry> mined_entries;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(mined_entries) DOC_DSCR("Mined blocks entries.") DOC_EXMP_AUTO(1) DOC_END
    END_KV_SERIALIZE_MAP()
  };


  struct seed_info_param
  {
    std::string seed_phrase;
    std::string seed_password;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(seed_phrase)     DOC_DSCR("Mnemonic seed phrase used for wallet recovery or generation.") DOC_EXMP("girlfriend unlike mutter tightly social silent expect constant bid nowhere reach flower bite salt lightning conversation dog rush quietly become usually midnight each secret offer class") DOC_END
      KV_SERIALIZE(seed_password)   DOC_DSCR("Password used to encrypt or decrypt the mnemonic seed phrase, if applicable.") DOC_EXMP("0101010103") DOC_END
    END_KV_SERIALIZE_MAP()
  };

  struct seed_phrase_info
  {
    bool syntax_correct;
    bool require_password;
    bool hash_sum_matched;
    bool tracking;
    std::string address;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(syntax_correct)      DOC_DSCR("Indicates whether the syntax is correct.")  DOC_EXMP(true)  DOC_END
      KV_SERIALIZE(require_password)    DOC_DSCR("Indicates whether a password is required.") DOC_EXMP(true)  DOC_END
      KV_SERIALIZE(hash_sum_matched)    DOC_DSCR("Indicates whether the hash sum matches.")   DOC_EXMP(true)  DOC_END
      KV_SERIALIZE(tracking)            DOC_DSCR("Indicates whether tracking is enabled.")    DOC_EXMP(false) DOC_END
      KV_SERIALIZE(address)             DOC_DSCR("Return address of the seed phrase.")        DOC_EXMP("ZxDNaMeZjwCjnHuU5gUNyrP1pM3U5vckbakzzV6dEHyDYeCpW8XGLBFTshcaY8LkG9RQn7FsQx8w2JeJzJwPwuDm2NfixPAXf") DOC_END
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_GET_BALANCE
  {
    DOC_COMMAND("Return the balances across all whitelisted assets of the wallet");

    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t 	 balance;
      uint64_t 	 unlocked_balance;
      std::list<asset_balance_entry> balances;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(balance)           DOC_DSCR("Native coins total amount")           DOC_EXMP(10000000000)               DOC_END
        KV_SERIALIZE(unlocked_balance)  DOC_DSCR("Native coins total unlocked amount")  DOC_EXMP(11000000000)               DOC_END
        KV_SERIALIZE(balances)          DOC_DSCR("Balances groupped by it's asset_id")  DOC_EXMP_AUTO(1)                    DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_ADDRESS
  {
    DOC_COMMAND("Obtains wallet's public address");

    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string   address;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)  DOC_DSCR("string; standard public address of the wallet.")  DOC_EXMP("ZxDNaMeZjwCjnHuU5gUNyrP1pM3U5vckbakzzV6dEHyDYeCpW8XGLBFTshcaY8LkG9RQn7FsQx8w2JeJzJwPwuDm2NfixPAXf") DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  

  struct COMMAND_RPC_GET_WALLET_INFO
  {
    DOC_COMMAND("Returns wallet helpful wallet information");

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
      bool                      has_bare_unspent_outputs;
      std::vector<std::string>  utxo_distribution;
      uint64_t                  current_height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)                  DOC_DSCR("string; standard public address of the wallet.")  DOC_EXMP("ZxDNaMeZjwCjnHuU5gUNyrP1pM3U5vckbakzzV6dEHyDYeCpW8XGLBFTshcaY8LkG9RQn7FsQx8w2JeJzJwPwuDm2NfixPAXf") DOC_END
        KV_SERIALIZE(path)                     DOC_DSCR("Path to wallet file location")   DOC_EXMP("/some/path/to/wallet/file.zan") DOC_END
        KV_SERIALIZE(transfers_count)          DOC_DSCR("Represent number of transactions that happened to this wallet(basically tx history)")  DOC_EXMP(11) DOC_END
        KV_SERIALIZE(transfer_entries_count)   DOC_DSCR("Represent number of internal entries count(each entry represent tx output that have been addressed to this wallet)")  DOC_EXMP(24) DOC_END
        KV_SERIALIZE(is_whatch_only)           DOC_DSCR("Shows if the wallet is watch-only")  DOC_EXMP(false) DOC_END
        KV_SERIALIZE(has_bare_unspent_outputs) DOC_DSCR("Shows if the wallet still has UTXO from pre-zarcanum era")  DOC_EXMP(false) DOC_END
        KV_SERIALIZE(utxo_distribution)        DOC_DSCR("UTXO distribution for this particular wallet: disabled right now")  DOC_EXMP_AUTO(1, "1") DOC_END
        KV_SERIALIZE(current_height)           DOC_DSCR("Current wallet/daemon height")  DOC_EXMP(112132) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_WALLET_RESTORE_INFO
  {
    DOC_COMMAND("Return wallet seed, which could be password-protected(seed secured with passphrase) or open(unsecured seed). If no password provided it returns open (unsecured) seed. ");

    struct request
    {
      std::string               seed_password;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(seed_password) DOC_DSCR("Password to secure wallet's seed")  DOC_EXMP("010101012") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string               seed_phrase;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(seed_phrase)  DOC_DSCR("Wallet's seed(secured with password if it was provided in argument)")  DOC_EXMP("girlfriend unlike offer mutter tightly social silent expect constant bid nowhere reach flower bite salt becomeconversation dog rush quietly become usually lightning midnight each secret class") DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_SEED_PHRASE_INFO
  {
    DOC_COMMAND("This call is used to validate seed phrase and to fetch additional information about it");

    typedef seed_info_param request;
    typedef seed_phrase_info response;
  };

  struct wallet_provision_info
  {
    uint64_t                  transfers_count;
    uint64_t                  transfer_entries_count;
    uint64_t                  balance;
    uint64_t                  unlocked_balance;
    uint64_t                  curent_height;


    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(transfers_count)          DOC_DSCR("Number of transfers in wallet") DOC_EXMP(1) DOC_END
      KV_SERIALIZE(transfer_entries_count)   DOC_DSCR("Number of UTXO entries in wallet") DOC_EXMP(3) DOC_END
      KV_SERIALIZE(balance)                  DOC_DSCR("Current balance of native coins") DOC_EXMP(100000000000) DOC_END
      KV_SERIALIZE(unlocked_balance)         DOC_DSCR("Unlocked balance oof native coins") DOC_EXMP(90000000000) DOC_END
      KV_SERIALIZE(curent_height)            DOC_DSCR("Current sync height of the wallet") DOC_EXMP(121212) DOC_END
    END_KV_SERIALIZE_MAP()

  };

  struct COMMAND_RPC_GET_MINING_HISTORY
  {
    DOC_COMMAND("Returns wallet statistic on mining. As an argument 'v' it receive timestamp from which history is reviewed");

    typedef currency::struct_with_one_t_type<uint64_t> request;
    typedef wallet_public::mining_history response;
  };


#define ORDER_FROM_BEGIN_TO_END        "FROM_BEGIN_TO_END"
#define ORDER_FROM_FROM_END_TO_BEGIN   "FROM_END_TO_BEGIN"

  struct COMMAND_RPC_GET_RECENT_TXS_AND_INFO2
  {
    DOC_COMMAND("Returns wallet history of transactions V2(post-zarcanum version)");

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
        KV_SERIALIZE(offset)                 DOC_DSCR("Offset from what index to start fetching transfers entries(if filters are used then last_item_index could be used from previous call)")         DOC_EXMP(0)       DOC_END
        KV_SERIALIZE(count)                  DOC_DSCR("How many items to fetch, if items fetched is less then count, then it enumeration is over")                       DOC_EXMP(100)                 DOC_END
        KV_SERIALIZE(update_provision_info)  DOC_DSCR("If update pi is required, could be false only if need to optimize performance(appliable for a veru big wallets)") DOC_EXMP(true)                DOC_END
        KV_SERIALIZE(exclude_mining_txs)     DOC_DSCR("Exclude mining/staking transactions from results(last_item_index should be used for subsequential calls)")        DOC_EXMP(false)               DOC_END
        KV_SERIALIZE(exclude_unconfirmed)    DOC_DSCR("Do not include uncomfirmed transactions in results (it also not included is offset is non zero)")                 DOC_EXMP(false)               DOC_END
        KV_SERIALIZE(order)                  DOC_DSCR("Order: \"FROM_BEGIN_TO_END\" or \"FROM_END_TO_BEGIN\"")                                                           DOC_EXMP("FROM_END_TO_BEGIN") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      wallet_provision_info pi;
      std::vector<wallet_transfer_info> transfers;
      uint64_t total_transfers;
      uint64_t last_item_index;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(pi)               DOC_DSCR("Details on wallet balance etc") DOC_END
        KV_SERIALIZE(transfers)        DOC_DSCR("Transfers")                     DOC_EXMP_AUTO(1) DOC_END
        KV_SERIALIZE(total_transfers)  DOC_DSCR("Total transfers")               DOC_EXMP(1) DOC_END
        KV_SERIALIZE(last_item_index)  DOC_DSCR("Last item index")               DOC_EXMP(1) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };


  struct COMMAND_RPC_GET_RECENT_TXS_AND_INFO
  {
    DOC_COMMAND("Returns wallet history of transactions");

    typedef COMMAND_RPC_GET_RECENT_TXS_AND_INFO2::request request;

    struct response
    {
      wallet_provision_info pi;
      std::vector<wallet_transfer_info_old> transfers;
      uint64_t total_transfers;
      uint64_t last_item_index;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(pi)              DOC_DSCR("Additiona details about balance state")                                               DOC_END
        KV_SERIALIZE(transfers)       DOC_DSCR("Transfers history array")                                           DOC_EXMP_AUTO(1)  DOC_END
        KV_SERIALIZE(total_transfers) DOC_DSCR("Total number of transfers in the tx history")                       DOC_EXMP(1)       DOC_END
        KV_SERIALIZE(last_item_index) DOC_DSCR("Index of last returned item(might be needed if filters are used)")  DOC_EXMP(1)       DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_REGISTER_ALIAS
  {
    DOC_COMMAND("Register an alias for the address");

    struct request
    {
      currency::alias_rpc_details al;
      crypto::secret_key authority_key;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(al)                              DOC_DSCR("Alias details")  DOC_END
        KV_SERIALIZE_POD_AS_HEX_STRING(authority_key) DOC_DSCR("Key for registering aliases shorter than 6 letters (team)")  DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      crypto::hash tx_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(tx_id)     DOC_DSCR("If success - transactions that performs registration(alias becomes available after few confirmations)") DOC_EXMP("97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc") DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_UPDATE_ALIAS
  {
    DOC_COMMAND("Update an alias details/transwer alias ownership");

    struct request
    {
      currency::alias_rpc_details al;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(al)                              DOC_DSCR("Alias details")  DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      crypto::hash tx_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(tx_id)     DOC_DSCR("If success - transactions that performs registration(alias becomes available after few confirmations)") DOC_EXMP("97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc") DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  
  struct transfer_destination
  {
    uint64_t amount = 0;
    std::string address;
    crypto::public_key asset_id = currency::native_coin_asset_id;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)                      DOC_DSCR("Amount to transfer to destination") DOC_EXMP(10000000000000)     DOC_END
      KV_SERIALIZE(address)                     DOC_DSCR("Destination address") DOC_EXMP("ZxBvJDuQjMG9R2j4WnYUhBYNrwZPwuyXrC7FHdVmWqaESgowDvgfWtiXeNGu8Px9B24pkmjsA39fzSSiEQG1ekB225ZnrMTBp")     DOC_END
      KV_SERIALIZE_POD_AS_HEX_STRING(asset_id)  DOC_DSCR("Asset id to transfer") DOC_EXMP("cc608f59f8080e2fbfe3c8c80eb6e6a953d47cf2d6aebd345bada3a1cab99852")     DOC_END
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_TRANSFER
  {
    DOC_COMMAND("Make new payment transaction from the wallet");

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
        KV_SERIALIZE(destinations)     DOC_DSCR("List of destinations") DOC_EXMP_AUTO(1)     DOC_END
        KV_SERIALIZE(fee)              DOC_DSCR("Fee to be paid on behalf of sender's wallet(paid in native coins)") DOC_EXMP_AUTO(10000000000)     DOC_END
        KV_SERIALIZE(mixin)            DOC_DSCR("Specifies number of mixins(decoys) that would be used to create input, actual for pre-zarcanum outputs, for post-zarcanum outputs instead of this option, number that is defined by network hard rules(15+)") DOC_EXMP(15)     DOC_END
        KV_SERIALIZE(payment_id)       DOC_DSCR("Hex-encoded payment_id, that normally used for user database by exchanges") DOC_EXMP_AUTO("")     DOC_END
        KV_SERIALIZE(comment)          DOC_DSCR("Text comment that is displayed in UI") DOC_EXMP_AUTO("Thanks for the coffe")     DOC_END
        KV_SERIALIZE(push_payer)       DOC_DSCR("Reveal information about sender of this transaction, basically add sender address to transaction in encrypted way, so only receiver can see who sent transaction") DOC_EXMP(false)     DOC_END
        KV_SERIALIZE(hide_receiver)    DOC_DSCR("This add to transaction information about remote address(destination), might be needed when the wallet restored from seed phrase and fully resynched, if this option were true, then sender won't be able to see remote address for sent transactions anymore.") DOC_EXMP(true)     DOC_END
        KV_SERIALIZE(service_entries)  DOC_DSCR("Service entries that might be used by different apps that works on top of Zano network, not part of consensus") DOC_EXMP_AUTO(1)     DOC_END
        KV_SERIALIZE(service_entries_permanent) DOC_DSCR("Point to wallet that service_entries should be placed to 'extra' section of transaction(which won't be pruned after checkpoints)") DOC_EXMP_AUTO(1)     DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string tx_hash;
      std::string tx_unsigned_hex; // for cold-signing process
      uint64_t tx_size;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)           DOC_DSCR("Has of the generated transaction(if succeded)") DOC_EXMP("01220e8304d46b940a86e383d55ca5887b34f158a7365bbcdd17c5a305814a93")     DOC_END
        KV_SERIALIZE(tx_unsigned_hex)
        KV_SERIALIZE(tx_size)           DOC_DSCR("Transaction size in bytes") DOC_EXMP(1234)     DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_STORE
  {
    DOC_COMMAND("Store wallet's data to file");

    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t wallet_file_size;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(wallet_file_size) DOC_DSCR("Resulting file size in bytes") DOC_EXMP(232243)     DOC_END
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
      KV_SERIALIZE(payment_id)     DOC_DSCR("Payment id that related to this payment") DOC_EXMP("1dfe5a88ff9effb3")     DOC_END
      KV_SERIALIZE(tx_hash)        DOC_DSCR("Transaction ID that is holding this payment") DOC_EXMP("01220e8304d46b940a86e383d55ca5887b34f158a7365bbcdd17c5a305814a93")     DOC_END
      KV_SERIALIZE(amount)         DOC_DSCR("Amount of native coins transfered") DOC_EXMP(100000000000)     DOC_END
      KV_SERIALIZE(block_height)   DOC_DSCR("Block height that holds transaction") DOC_EXMP(12321)     DOC_END
      KV_SERIALIZE(unlock_time)    DOC_DSCR("Timestamp/blocknumber after which this money would become availabe, recommended don't count transfers that has this field not 0") DOC_EXMP(0)     DOC_END
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_GET_PAYMENTS
  {
    DOC_COMMAND("Gets list of incoming transfers by a given payment ID");


    struct request
    {
      std::string payment_id; // hex-encoded
      bool allow_locked_transactions;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payment_id)                  DOC_DSCR("Payment id that is used to identify transfers") DOC_EXMP("1dfe5a88ff9effb3")     DOC_END
        KV_SERIALIZE(allow_locked_transactions)   DOC_DSCR("Says to wallet if locked transfers should be included or not (false is strongly recomennded)") DOC_EXMP(false)     DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<payment_details> payments;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payments)        DOC_DSCR("Array of payments that connected to given payment_id") DOC_EXMP_AUTO(1)     DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_BULK_PAYMENTS
  {
    DOC_COMMAND("Gets list of incoming transfers by a given multiple payment_ids");


    struct request
    {
      std::vector<std::string> payment_ids;
      uint64_t min_block_height;
      bool allow_locked_transactions;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payment_ids)                DOC_DSCR("Payment ids that is used to identify transfers")  DOC_EXMP_AUTO(2, "1dfe5a88ff9effb3")     DOC_END
        KV_SERIALIZE(min_block_height)           DOC_DSCR("Minimal block height to consider")  DOC_EXMP(0)     DOC_END
        KV_SERIALIZE(allow_locked_transactions)  DOC_DSCR("Says to wallet if locked transfers should be included or not (false is strongly recomennded)") DOC_EXMP(false)     DOC_END
      END_KV_SERIALIZE_MAP()
    };

    typedef COMMAND_RPC_GET_PAYMENTS::response response;
  };

  struct COMMAND_RPC_MAKE_INTEGRATED_ADDRESS
  {
    DOC_COMMAND("Generate integrated address");

    struct request
    {
      std::string payment_id; // hex-encoded

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payment_id) DOC_DSCR("Hex-encoded Payment ID to be associated with the this address. If empty then wallet would generate new payment id using system random library") DOC_EXMP("1dfe5a88ff9effb3")  DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string integrated_address;
      std::string payment_id; // hex-encoded

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(integrated_address) DOC_DSCR("Integrated address combining a standard address and payment ID, if applicable.")  DOC_EXMP("iZ2EMyPD7g28hgBfboZeCENaYrHBYZ1bLFi5cgWvn4WJLaxfgs4kqG6cJi9ai2zrXWSCpsvRXit14gKjeijx6YPCLJEv6Fx4rVm1hdAGQFis") DOC_END
        KV_SERIALIZE(payment_id)         DOC_DSCR("Payment ID associated with the this address.") DOC_EXMP("1dfe5a88ff9effb3")  DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS
  {
    DOC_COMMAND("Decode integrated address");

    struct request
    {
      std::string integrated_address;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(integrated_address) DOC_DSCR("Integrated address combining a standard address and payment ID, if applicable.")  DOC_EXMP("iZ2EMyPD7g28hgBfboZeCENaYrHBYZ1bLFi5cgWvn4WJLaxfgs4kqG6cJi9ai2zrXWSCpsvRXit14gKjeijx6YPCLJEv6Fx4rVm1hdAGQFis") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string standard_address;
      std::string payment_id; // hex-encoded

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(standard_address) DOC_DSCR("Standart address.")  DOC_EXMP("ZxBvJDuQjMG9R2j4WnYUhBYNrwZPwuyXrC7FHdVmWqaESgowDvgfWtiXeNGu8Px9B24pkmjsA39fzSSiEQG1ekB225ZnrMTBp") DOC_END
        KV_SERIALIZE(payment_id)       DOC_DSCR("Hex-encoded payment id")  DOC_EXMP("1dfe5a88ff9effb3") DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_SWEEP_BELOW
  {
    DOC_COMMAND("Tries to transfer all coins with amount below the given limit to the given address");

    struct request
    {
      uint64_t    mixin;
      std::string address;
      uint64_t    amount;
      std::string payment_id_hex;
      uint64_t    fee;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(mixin)              DOC_DSCR("Number of outputs from the blockchain to mix with when sending a transaction to improve privacy.") DOC_EXMP(15) DOC_END
        KV_SERIALIZE(address)            DOC_DSCR("Public address for sending or receiving native coins.") DOC_EXMP("ZxBvJDuQjMG9R2j4WnYUhBYNrwZPwuyXrC7FHdVmWqaESgowDvgfWtiXeNGu8Px9B24pkmjsA39fzSSiEQG1ekB225ZnrMTBp") DOC_END
        KV_SERIALIZE(amount)             DOC_DSCR("Threshold amount of native coins to sweep.") DOC_EXMP(1000000000000) DOC_END
        KV_SERIALIZE(payment_id_hex)     DOC_DSCR("Payment ID associated with the transaction in hexadecimal format.") DOC_EXMP("1dfe5a88ff9effb3") DOC_END
        KV_SERIALIZE(fee)                DOC_DSCR("Transaction fee required for processing the transaction.") DOC_EXMP(10000000000) DOC_END
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
        KV_SERIALIZE(tx_hash)            DOC_DSCR("Transaction ID (hash) format.") DOC_EXMP("01220e8304d46b940a86e383d55ca5887b34f158a7365bbcdd17c5a305814a93")  DOC_END
        KV_SERIALIZE(tx_unsigned_hex)    DOC_DSCR("Unsigned transaction data in hexadecimal format.") DOC_EXMP("8304d46b940a86e383d55ca5887b34f158a7365bbcdd17c5a305814a9334f158a7368304d46b940a86e383d55ca5887b34f158a7365bbcdd17c5a305814a9334f158a736")  DOC_END
        KV_SERIALIZE(outs_total)         DOC_DSCR("Total number of outputs in the transaction.") DOC_EXMP(10) DOC_END
        KV_SERIALIZE(amount_total)       DOC_DSCR("Total amount of native coins involved in the transaction.") DOC_EXMP(100000000000) DOC_END
        KV_SERIALIZE(outs_swept)         DOC_DSCR("Number of outputs swept in the transaction.") DOC_EXMP(112) DOC_END
        KV_SERIALIZE(amount_swept)       DOC_DSCR("Amount of native coins swept in the transaction.") DOC_EXMP(101000000000) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_BARE_OUTS_STATS
  {
    DOC_COMMAND("Return information about wallet's pre-zarcanum era outputs. Those outputs should be converted to post-zarcanum varian with trnasfering it sooner or later. (Only outputs that have been created in Zarcanum era can participaet in staking)");


    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t      total_bare_outs;
      uint64_t      total_amount;
      uint64_t      expected_total_fee;
      uint64_t      txs_count;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(total_bare_outs)     DOC_DSCR("Total number of inspent bare outputs in the wallet.") DOC_EXMP(112) DOC_END
        KV_SERIALIZE(total_amount)        DOC_DSCR("Total amount of native coins involved in bare outputs.") DOC_EXMP(12000000000000) DOC_END
        KV_SERIALIZE(expected_total_fee)  DOC_DSCR("Expected total transaction fee required for processing the transaction.") DOC_EXMP(10000000000) DOC_END
        KV_SERIALIZE(txs_count)           DOC_DSCR("Total number of transactions needed to convert all bare outputs .") DOC_EXMP(2) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SWEEP_BARE_OUTS
  {
    DOC_COMMAND("Execute transactions needed be convert all bare(pre-zarcanum) outputs to post-zarcanum outputs. (Only outputs that have been created in Zarcanum era can participaet in staking)");


    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t      bare_outs_swept;
      uint64_t      amount_swept;
      uint64_t      fee_spent;
      uint64_t      txs_sent;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(bare_outs_swept)   DOC_DSCR("Number of bare outputs swept in the transactions.") DOC_EXMP(112) DOC_END
        KV_SERIALIZE(amount_swept)      DOC_DSCR("Amount of native coins swept in the transactions.") DOC_EXMP(12000000000000) DOC_END
        KV_SERIALIZE(fee_spent)         DOC_DSCR("Total fee spent on the transactions.") DOC_EXMP(10000000000) DOC_END
        KV_SERIALIZE(txs_sent)          DOC_DSCR("Total number of transactions sent.") DOC_EXMP(2) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_SIGN_TRANSFER
  {
    DOC_COMMAND("Sign transaction with the wallet's keys");

    struct request
    {
      std::string     tx_unsigned_hex;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_unsigned_hex) DOC_DSCR("Unsigned transaction hex-encoded blob.") DOC_EXMP("8304d46b940a86e383d55ca5887b34f158a7365bbcdd17c5a305814a9334f158a7368304d46b940a86e383d55ca5887b34f158a7365bbcdd17c5a305814a9334f158a736") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string     tx_signed_hex;
      std::string     tx_hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_signed_hex) DOC_DSCR("Signed transaction hex-encoded blob.") DOC_EXMP("8304d46b940a86e383d55ca5887b34f158a7365bbcdd17c5a305814a9334f158a7368304d46b940a86e383d55ca5887b34f158a7365bbcdd17c5a305814a9334f158a7368304d46b940a86e383d55ca5887b34f158a7365bbcdd17c5a305814a9334f158a7368304d46b940a86e383d55ca5887b34f158a7365bbcdd17c5a305814a9334f158a736") DOC_END
        KV_SERIALIZE(tx_hash)       DOC_DSCR("Signed transaction hash.") DOC_EXMP("01220e8304d46b940a86e383d55ca5887b34f158a7365bbcdd17c5a305814a93") DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_SUBMIT_TRANSFER
  {
    DOC_COMMAND("Relay signed transaction over the network");

    struct request
    {
      std::string     tx_signed_hex;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_signed_hex) DOC_DSCR("Signed transaction hex-encoded blob.") DOC_EXMP("8304d46b940a86e383d55ca5887b34f158a7365bbcdd17c5a305814a9334f158a7368304d46b940a86e383d55ca5887b34f158a7365bbcdd17c5a305814a9334f158a7368304d46b940a86e383d55ca5887b34f158a7365bbcdd17c5a305814a9334f158a7368304d46b940a86e383d55ca5887b34f158a7365bbcdd17c5a305814a9334f158a736") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string     tx_hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)   DOC_DSCR("Signed transaction hash.") DOC_EXMP("01220e8304d46b940a86e383d55ca5887b34f158a7365bbcdd17c5a305814a93") DOC_END
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
  struct COMMAND_MARKETPLACE_GET_MY_OFFERS
  {
    DOC_COMMAND("Fetch wallet's offers listed in the marketplace with given filters");

    typedef currency::COMMAND_RPC_GET_OFFERS_EX::request request;
    typedef currency::COMMAND_RPC_GET_OFFERS_EX::response response;
  };

  struct COMMAND_MARKETPLACE_PUSH_OFFER
  {
    DOC_COMMAND("Creates new offer and publish it on the blockchain");
    
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
        KV_SERIALIZE(tx_hash)         DOC_DSCR("Transaction hash") DOC_EXMP("40fa6db923728b38962718c61b4dc3af1acaa1967479c73703e260dc3609c58d") DOC_END
        KV_SERIALIZE(tx_blob_size)    DOC_DSCR("Size of the transaction blob") DOC_EXMP(1234) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_MARKETPLACE_PUSH_UPDATE_OFFER
  {
    DOC_COMMAND("Updates existing offer that this wallet created, and publish updated version on the blockchain");

    struct request
    {
      crypto::hash tx_id;
      uint64_t no;
      bc_services::offer_details_ex od;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(tx_id)   DOC_DSCR("Transaction ID represented as a hexadecimal string") DOC_EXMP("40fa6db923728b38962718c61b4dc3af1acaa1967479c73703e260dc3609c58d") DOC_END
        KV_SERIALIZE(no)                        DOC_DSCR("Number of offer entrie inside transacton(likely 0)") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(od)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string tx_hash;
      uint64_t    tx_blob_size;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)         DOC_DSCR("Transaction hash") DOC_EXMP("40fa6db923728b38962718c61b4dc3af1acaa1967479c73703e260dc3609c58d") DOC_END
        KV_SERIALIZE(tx_blob_size)    DOC_DSCR("Size of the transaction blob") DOC_EXMP(1232) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_MARKETPLACE_CANCEL_OFFER
  {
    DOC_COMMAND("Cancel existing offer that this wallet created(it actually create transaction that says that existing order got canceled)");

    struct request
    {
      crypto::hash tx_id;
      uint64_t no;
      uint64_t fee;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(tx_id)  DOC_DSCR("Transaction ID represented as a hexadecimal string") DOC_EXMP("40fa6db923728b38962718c61b4dc3af1acaa1967479c73703e260dc3609c58d") DOC_END
        KV_SERIALIZE(no)                       DOC_DSCR("Number of offer entrie inside transacton(likely 0)") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(fee)                      DOC_DSCR("Fee for operation") DOC_EXMP(10000000000) DOC_END
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string tx_hash;
      uint64_t    tx_blob_size;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)         DOC_DSCR("Transaction hash") DOC_EXMP("40fa6db923728b38962718c61b4dc3af1acaa1967479c73703e260dc3609c58d") DOC_END
        KV_SERIALIZE(tx_blob_size)    DOC_DSCR("Size of the transaction blob") DOC_EXMP(1232) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SEARCH_FOR_TRANSACTIONS
  {
    DOC_COMMAND("Search for transacrions in the wallet by few parameters");
    struct request
    {
      crypto::hash tx_id;
      bool in;
      bool out;
      bool pool;
      bool filter_by_height;
      uint64_t min_height;
      uint64_t max_height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(tx_id)       DOC_DSCR("Transaction ID represented as a hexadecimal string.") DOC_EXMP("97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc")  DOC_END
        KV_SERIALIZE(in)                            DOC_DSCR("Search over incoming transactions.")                  DOC_EXMP(true)  DOC_END
        KV_SERIALIZE(out)                           DOC_DSCR("Search over outgoing transactions.")                  DOC_EXMP(true)  DOC_END
        KV_SERIALIZE(pool)                          DOC_DSCR("Search over pool transactions.")                      DOC_EXMP(false) DOC_END
        KV_SERIALIZE(filter_by_height)              DOC_DSCR("Do filter transactions by height or not.")            DOC_EXMP(true)  DOC_END
        KV_SERIALIZE(min_height)                    DOC_DSCR("Minimum height for filtering transactions.")          DOC_EXMP(11000) DOC_END
        KV_SERIALIZE(max_height)                    DOC_DSCR("Maximum height for filtering transactions.")          DOC_EXMP(20000) DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<wallet_transfer_info> in;
      std::list<wallet_transfer_info> out;
      std::list<wallet_transfer_info> pool;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(in)      DOC_DSCR("List of incoming transactions.")                  DOC_EXMP_AUTO(1)  DOC_END
        KV_SERIALIZE(out)     DOC_DSCR("List of outgoing transactions.")                  DOC_EXMP_AUTO(1)  DOC_END
        KV_SERIALIZE(pool)    DOC_DSCR("List of pool transactions.")                      DOC_EXMP_AUTO(1)  DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SEARCH_FOR_TRANSACTIONS_LEGACY
  {    
    DOC_COMMAND("Search for transacrions in the wallet by few parameters(legacy version)");

    typedef COMMAND_RPC_SEARCH_FOR_TRANSACTIONS::request request;

    struct response
    {
      std::list<wallet_transfer_info_old> in;
      std::list<wallet_transfer_info_old> out;
      std::list<wallet_transfer_info_old> pool;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(in)      DOC_DSCR("List of incoming transactions.")                  DOC_EXMP_AUTO(1)  DOC_END
        KV_SERIALIZE(out)     DOC_DSCR("List of outgoing transactions.")                  DOC_EXMP_AUTO(1)  DOC_END
        KV_SERIALIZE(pool)    DOC_DSCR("List of pool transactions.")                      DOC_EXMP_AUTO(1)  DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };


  struct htlc_entry_info
  {
    currency::account_public_address counterparty_address;
    crypto::hash sha256_hash;
    crypto::hash tx_id;
    uint64_t amount;
    bool is_redeem;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE_ADDRESS_AS_TEXT(counterparty_address)
      KV_SERIALIZE_POD_AS_HEX_STRING(sha256_hash)
      KV_SERIALIZE_POD_AS_HEX_STRING(tx_id)
      KV_SERIALIZE(is_redeem)
    END_KV_SERIALIZE_MAP()
  };


  struct COMMAND_CREATE_HTLC_PROPOSAL
  {


    struct request
    {
      uint64_t amount;
      currency::account_public_address counterparty_address;
      uint64_t lock_blocks_count;
      crypto::hash htlc_hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amount)
        KV_SERIALIZE_ADDRESS_AS_TEXT(counterparty_address)
        KV_SERIALIZE(lock_blocks_count)
        KV_SERIALIZE_POD_AS_HEX_STRING(htlc_hash)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string result_tx_blob;
      crypto::hash result_tx_id;
      std::string derived_origin_secret; // this field derived in a deterministic way if no htlc_hash was provided

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_BLOB_AS_HEX_STRING(result_tx_blob)
        KV_SERIALIZE_POD_AS_HEX_STRING(result_tx_id)
        KV_SERIALIZE_BLOB_AS_HEX_STRING_N(derived_origin_secret, "derived_origin_secret_as_hex")
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_GET_LIST_OF_ACTIVE_HTLC
  {
    struct request
    {
      bool income_redeem_only;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(income_redeem_only)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<wallet_public::htlc_entry_info> htlcs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(htlcs)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_REDEEM_HTLC
  {
    struct request
    {
      crypto::hash tx_id;
      std::string origin_secret;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(tx_id)
        KV_SERIALIZE_BLOB_AS_HEX_STRING_N(origin_secret, "origin_secret_as_hex")
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string result_tx_blob;
      crypto::hash result_tx_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_BLOB_AS_HEX_STRING(result_tx_blob)
        KV_SERIALIZE_POD_AS_HEX_STRING(result_tx_id)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_CHECK_HTLC_REDEEMED
  {
    struct request
    {
      crypto::hash htlc_tx_id;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(htlc_tx_id)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string origin_secrete;
      crypto::hash redeem_tx_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_BLOB_AS_HEX_STRING_N(origin_secrete, "origin_secrete_as_hex")
        KV_SERIALIZE_POD_AS_HEX_STRING(redeem_tx_id)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct wallet_vote_config_entry
  {
    std::string proposal_id;
    uint64_t h_start;
    uint64_t h_end;
    bool vote;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(proposal_id)
      KV_SERIALIZE(h_start)
      KV_SERIALIZE(h_end)
      KV_SERIALIZE(vote)
    END_KV_SERIALIZE_MAP()

  };

  struct wallet_vote_config
  {
    std::vector<wallet_vote_config_entry> entries;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(entries)
    END_KV_SERIALIZE_MAP()
  };

  struct asset_funds
  {
    crypto::public_key asset_id;
    uint64_t amount;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_POD_AS_HEX_STRING(asset_id)
      KV_SERIALIZE(amount)
    END_KV_SERIALIZE_MAP()
  };


  struct ionic_swap_proposal_info
  {
    std::vector<asset_funds> to_finalizer;   //assets that addressed to receiver of proposal (aka Bob, aka "finalizer") and funded by side that creating proposal (aka Alice, aka "initiator") 
    std::vector<asset_funds> to_initiator;   //assets addressed to initiator of proposal (aka Alice, aka "initiator") and expected to be funded by the side that receiving proposal (aka Bob,  aka "finalizer") 
    //uint64_t mixins;
    uint64_t fee_paid_by_a;
    //uint64_t expiration_time;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(to_finalizer)        DOC_DSCR("Assets sent to the finalizer") DOC_EXMP_AUTO(1, asset_funds{ epee::transform_str_to_t_pod<crypto::public_key>("97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc"), 1000000000000}) DOC_END
      KV_SERIALIZE(to_initiator)        DOC_DSCR("Assets sent to the initiator") DOC_EXMP_AUTO(1, asset_funds{ currency::native_coin_asset_id, 10000000000}) DOC_END
      //KV_SERIALIZE(mixins)              DOC_DSCR("Number of mixins used") DOC_EXMP() DOC_END
      KV_SERIALIZE(fee_paid_by_a)       DOC_DSCR("Fee paid by party A(initiator)") DOC_EXMP(10000000000) DOC_END
    END_KV_SERIALIZE_MAP()

  };


  struct ionic_swap_proposal_context
  {
    currency::tx_generation_context gen_context;
    
    BEGIN_SERIALIZE_OBJECT()
      VERSION(0)   //use VERSION_TO_MEMBER if it's more then 0
      FIELD(gen_context)
    END_SERIALIZE()
  };

  struct ionic_swap_proposal
  {
    currency::transaction tx_template;
    std::string encrypted_context;          //ionic_swap_proposal_context encrypted with derivation

    BEGIN_SERIALIZE_OBJECT()
      VERSION(0)   //use VERSION_TO_MEMBER if it's more then 0
      FIELD(tx_template)
      FIELD(encrypted_context)
    END_SERIALIZE()
  };

  struct create_ionic_swap_proposal_request
  {
    uint64_t wallet_id;
    ionic_swap_proposal_info proposal_info;
    std::string destination_add;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wallet_id)
      KV_SERIALIZE(proposal_info)
      KV_SERIALIZE(destination_add)
    END_KV_SERIALIZE_MAP()
  };


  struct COMMAND_IONIC_SWAP_GENERATE_PROPOSAL
  {
    DOC_COMMAND("Generates ionic swap proposal according to details provided in request, result present as hex-encoded blob, that should be passed to recepient to validate this proposal and executing on it")

    struct request
    {
      ionic_swap_proposal_info proposal;
      std::string destination_address;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(proposal)            DOC_DSCR("Proposal details") DOC_END
        KV_SERIALIZE(destination_address) DOC_DSCR("Destination address") DOC_EXMP("ZxBvJDuQjMG9R2j4WnYUhBYNrwZPwuyXrC7FHdVmWqaESgowDvgfWtiXeNGu8Px9B24pkmjsA39fzSSiEQG1ekB225ZnrMTBp") DOC_END
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string hex_raw_proposal;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(hex_raw_proposal) DOC_DSCR("Hex-encoded proposal raw data(encrypted with common shared key). Includes half-created transaction template and some extra information that would be needed counterparty to finialize and sign transaction") DOC_EXMP("97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc") DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_IONIC_SWAP_GET_PROPOSAL_INFO
  {
    DOC_COMMAND("Reads hex-encoded ionic swap proposal info, generated by other user and addressed to this wallet")


    struct request
    {
      std::string hex_raw_proposal;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(hex_raw_proposal) DOC_DSCR("Hex-encoded proposal raw data(encrypted with common shared key). Includes half-created transaction template and some extra information that would be needed counterparty to finialize and sign transaction") DOC_EXMP("97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc") DOC_END
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      ionic_swap_proposal_info proposal;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(proposal) DOC_DSCR("Proposal details") DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_IONIC_SWAP_ACCEPT_PROPOSAL
  {
    DOC_COMMAND("This essential command actually execute proposal that was sent by counter party, by completing and signing transaction template that was in proposal, and sending it to the network.")


    struct request
    {
      std::string hex_raw_proposal;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(hex_raw_proposal) DOC_DSCR("Hex-encoded proposal raw data(encrypted with common shared key). Includes half-created transaction template and some extra information that would be needed counterparty to finialize and sign transaction") DOC_EXMP("97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc") DOC_END
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      crypto::hash result_tx_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(result_tx_id) DOC_DSCR("Result transaction id")     DOC_EXMP("97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc") DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };


  struct wallet_info
  {
    std::list<tools::wallet_public::asset_balance_entry> balances;
    uint64_t mined_total;
    std::string address;
    std::string view_sec_key;
    std::string path;
    bool is_auditable;
    bool is_watch_only;
    bool has_bare_unspent_outputs;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(balances)                    DOC_DSCR("Balances hold by this wallet") DOC_EXMP_AUTO(1) DOC_END
      KV_SERIALIZE(mined_total)                 DOC_DSCR("Total amount mined")           DOC_EXMP(1000000000000) DOC_END
      KV_SERIALIZE(address)                     DOC_DSCR("Address")                      DOC_EXMP("ZxBvJDuQjMG9R2j4WnYUhBYNrwZPwuyXrC7FHdVmWqaESgowDvgfWtiXeNGu8Px9B24pkmjsA39fzSSiEQG1ekB225ZnrMTBp") DOC_END
      KV_SERIALIZE(view_sec_key)                DOC_DSCR("View secret key")              DOC_EXMP("97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc") DOC_END
      KV_SERIALIZE(path)                        DOC_DSCR("Path to wallet file")          DOC_EXMP("/some/path/to/wallet/file.zan") DOC_END
      KV_SERIALIZE(is_auditable)                DOC_DSCR("Flag indicating whether the wallet is auditable")             DOC_EXMP(false) DOC_END
      KV_SERIALIZE(is_watch_only)               DOC_DSCR("Flag indicating whether the wallet is watch-only")            DOC_EXMP(false) DOC_END
      KV_SERIALIZE(has_bare_unspent_outputs)    DOC_DSCR("Flag indicating whether the wallet has bare unspent outputs(pre-zarcanum outputs)") DOC_EXMP(false) DOC_END
    END_KV_SERIALIZE_MAP()
  };

  struct wallet_info_extra
  {
    std::string view_private_key;
    std::string view_public_key;
    std::string spend_private_key;
    std::string spend_public_key;
    std::string seed;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(view_private_key)
      KV_SERIALIZE(view_public_key)
      KV_SERIALIZE(spend_private_key)
      KV_SERIALIZE(spend_public_key)
      KV_SERIALIZE(seed)
    END_KV_SERIALIZE_MAP()
  };


  struct wallet_entry_info
  {
    wallet_info wi;
    uint64_t    wallet_id;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wi)
      KV_SERIALIZE(wallet_id)  DOC_DSCR("Wallet ID") DOC_EXMP(2) DOC_END
    END_KV_SERIALIZE_MAP()

  };

  struct COMMAND_MW_GET_WALLETS
  {
    DOC_COMMAND("Get loaded wallets list, userful for multi-wallet API")

    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<wallet_entry_info> wallets;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(wallets) DOC_DSCR("Array of wallets")                       DOC_EXMP_AUTO(1)                        DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_MW_SELECT_WALLET
  {
    DOC_COMMAND("Select curent active wallet, after that all wallet RPC call would be addressed to this wallet")

    struct request
    {
      uint64_t wallet_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(wallet_id) DOC_DSCR("Wallet id")    DOC_EXMP_AUTO(2)                        DOC_END
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string status;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status) DOC_DSCR("Result (OK if success)")    DOC_EXMP(API_RETURN_CODE_OK)                        DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_SIGN_MESSAGE
  {
    DOC_COMMAND("Trivially sign base64 encoded data message using wallet spend key")

    struct request
    {
      std::string buff; //base64 encoded data
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(buff)        DOC_DSCR("base64 encoded data message to be signed")    DOC_EXMP("ZGNjc2Ztc2xrZm12O2xrZm12OydlbGtmdm0nbGtmbXY=")  DOC_END
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      crypto::signature sig = currency::null_sig;
      crypto::public_key pkey = currency::null_pkey;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(sig)   DOC_DSCR("Signature represented as a hexadecimal string") DOC_EXMP("97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc") DOC_END
        KV_SERIALIZE_POD_AS_HEX_STRING(pkey)  DOC_DSCR("Wallet's public key represented as a hexadecimal string") DOC_EXMP("97d91442f8f3c22683585eaa60b53757d49bf046a96269cef45c1bc9ff7300cc") DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };
   
  struct COMMAND_ENCRYPT_DATA
  {
    DOC_COMMAND("Trivially encrypt base64 encoded data message with chacha using wallet spend key")

    struct request
    {
      std::string buff; //base64 encoded data

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(buff) DOC_DSCR("base64 encoded data message to be encrypted")    DOC_EXMP("ZGNjc2Ztc2xrZm12O2xrZm12OydlbGtmdm0nbGtmbXY=")  DOC_END
      END_KV_SERIALIZE_MAP()
    };     

    struct response
    { 
      std::string res_buff; //base64 encoded encrypted data
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(res_buff) DOC_DSCR("base64 encoded resulted data message")    DOC_EXMP("ZGNjc2Ztc2xrZm12O2xrZm12OydlbGtmdm0nbGtmbXY=")  DOC_END
      END_KV_SERIALIZE_MAP()
    };
  }; 
  
  struct COMMAND_DECRYPT_DATA
  {
    DOC_COMMAND("Trivially decrypt base64 encoded data message with chacha using wallet spend key")
    struct request
    {
      std::string buff; //base64 encoded encrypted data

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(buff) DOC_DSCR("base64 encoded data message to be decrypted")    DOC_EXMP("ZGNjc2Ztc2xrZm12O2xrZm12OydlbGtmdm0nbGtmbXY=")  DOC_END
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string res_buff; //base64 encoded data

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(res_buff) DOC_DSCR("base64 encoded resulted data message")    DOC_EXMP("ZGNjc2Ztc2xrZm12O2xrZm12OydlbGtmdm0nbGtmbXY=")  DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_PROXY_TO_DAEMON
  {
    DOC_COMMAND("Proxy call to daemon(node), might be not effective in some cases, so need to be carefull with use of it")

    struct request
    {
      std::string uri; 
      std::string base64_body; //base64 encoded body

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(uri)              DOC_DSCR("URI for daemon API") DOC_EXMP("/json_rpc") DOC_END
        KV_SERIALIZE(base64_body)      DOC_DSCR("Base64 encoded request body") DOC_EXMP("ewogICAgImpzb25ycGMiOiAiMi4wIiwKICAgICJpZCI6IDAsCiAgICAibWV0aG9kIjogImdldF9taW5pbmdfaGlzdG9yeSIKfQ==") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string base64_body; //base64 encoded response body
      int32_t response_code; 

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(base64_body)    DOC_DSCR("Base64 encoded daemon response body") DOC_EXMP("ewogICJpZCI6IDAsCiAgImpzb25ycGMiOiAiMi4wIiwKICAicmVzdWx0IjogewogICAgInBheW1lbnRzIjogWwogICAgICB7CiAgICAgICAgInBheW1lbnRfaWQiOiAiMDAwMDAwMDBmZjAwZmYwMCIsCiAgICAgICAgImFtb3VudCI6IDEwMDAwMDAwMCwKICAgICAgICAiYmxvY2tfaGVpZ2h0IjogMjAyNTU2LAogICAgICAgICJ0eF9oYXNoIjogIjAxMjIwZTgzMDRkNDZiOTQwYTg2ZTM4M2Q1NWNhNTg4N2IzNGYxNThhNzM2NWJiY2RkMTdjNWEzMDU4MTRhOTMiLAogICAgICAgICJ1bmxvY2tfdGltZSI6IDAKICAgICAgfSwKICAgICAgewogICAgICAgICJwYXltZW50X2lkIjogIjAwMDAwMDAwZmYwMGZmMDEiLAogICAgICAgICJhbW91bnQiOiAxMDAwMDAwMDAsCiAgICAgICAgImJsb2NrX2hlaWdodCI6IDIwMjU1NiwKICAgICAgICAidHhfaGFzaCI6ICIwYjVlYjk2ODVjMGMxMWRiNzdlMmNkZDk4NzljOGQzYjgxNTUyM2M2ZTRiZjAzZGNlZTYyYzU4M2I3ZTFmNzcyIiwKICAgICAgICAidW5sb2NrX3RpbWUiOiAwCiAgICAgIH0KICAgIF0KICB9Cn0=") DOC_END
        KV_SERIALIZE(response_code)  DOC_DSCR("Response code") DOC_EXMP(200) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct assets_whitelist
  {
    std::vector<currency::asset_descriptor_with_id> assets;
    crypto::signature signature = currency::null_sig;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(assets)
      KV_SERIALIZE_POD_AS_HEX_STRING(signature)
    END_KV_SERIALIZE_MAP()
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

  inline bool operator==(const asset_funds& lhs, const asset_funds& rhs)
  {
    return lhs.amount == rhs.amount && lhs.asset_id == rhs.asset_id;
  }



  struct COMMAND_ASSETS_WHITELIST_GET
  {
    DOC_COMMAND("Get whitelisted assets for this wallet, assets descriptors present in any of the lists in results would be present in balance() call results(if those assets are part of the wallet transfers). Assets that are not included in those lists won't be included in balance even if the wallet own inputs with such assets.")


    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::list<currency::asset_descriptor_with_id> local_whitelist;
      std::list<currency::asset_descriptor_with_id> global_whitelist;
      std::list<currency::asset_descriptor_with_id> own_assets;


      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(local_whitelist)   DOC_DSCR("Local whitelist, assets that hase been added to this wallet file manually(!)") DOC_EXMP_AUTO(1) DOC_END
        KV_SERIALIZE(global_whitelist)  DOC_DSCR("Global whitelist, well-known assets with adoption, mantained by the team and community") DOC_EXMP_AUTO(1) DOC_END
        KV_SERIALIZE(own_assets)        DOC_DSCR("Own assets, the ones that is under control of this wallet") DOC_EXMP_AUTO(1) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_ASSETS_WHITELIST_ADD
  {
    DOC_COMMAND("Add given asset id to local whitelist. This whitelist is stored with the wallet file and reset in case of wallet resync or restoring wallet from seed phrase.")

    struct request
    {
      crypto::public_key asset_id = currency::null_pkey;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(asset_id) DOC_DSCR("Asset id that needed to be added to local whitelist, asset_id must exist in the network") DOC_EXMP("f74bb56a5b4fa562e679ccaadd697463498a66de4f1760b2cd40f11c3a00a7a8") DOC_END
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string status;
      currency::asset_descriptor_base asset_descriptor;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)             DOC_DSCR("Status of the asset") DOC_EXMP("OK") DOC_END
        KV_SERIALIZE(asset_descriptor)   DOC_DSCR("Details of the asset, recieved from node") DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_ASSETS_WHITELIST_REMOVE
  {
    DOC_COMMAND("Remove given asset id from local whitelist. This whitelist is stored with the wallet file and reset in case of wallet resync or restoring wallet from seed phrase.")

    struct request
    {
      crypto::public_key asset_id = currency::null_pkey;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(asset_id) DOC_DSCR("Asset id to be removed from local whitelist") DOC_EXMP("f74bb56a5b4fa562e679ccaadd697463498a66de4f1760b2cd40f11c3a00a7a8") DOC_END
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)    DOC_DSCR("Command result (OK if success)") DOC_EXMP(API_RETURN_CODE_OK) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };




  struct COMMAND_ASSETS_DEPLOY
  {
    DOC_COMMAND("Deploy new asset in the system.");

    struct request
    {
      std::list<transfer_destination> destinations;
      currency::asset_descriptor_base asset_descriptor;
      bool do_not_split_destinations = false;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(destinations)                DOC_DSCR("Addresses where to receive emitted coins. Asset id in the destinations is irreleant and can be omitted.") DOC_EXMP_AUTO(1) DOC_END
        KV_SERIALIZE(asset_descriptor)            DOC_DSCR("Descriptor that holds all information about asset - ticker, emission, description etc") DOC_END
        KV_SERIALIZE(do_not_split_destinations)   DOC_DSCR("If true, the provided destinations will be used as-is and won't be splitted (or altered) to avoid common issues. Default is false.") DOC_EXMP(false) DOC_END
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      crypto::hash tx_id;
      crypto::public_key new_asset_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(tx_id)    DOC_DSCR("Id of transaction that carries asset registration command, asset would be registered as soon as transaction got confirmed") DOC_EXMP("f74bb56a5b4fa562e679ccaadd697463498a66de4f1760b2cd40f11c3a00a7a8") DOC_END
        KV_SERIALIZE_POD_AS_HEX_STRING(new_asset_id)  DOC_DSCR("Issued asset id") DOC_EXMP("40fa6db923728b38962718c61b4dc3af1acaa1967479c73703e260dc3609c58d") DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  // contains data for external checking & signing asset-emitting/-updating transaction by a third-party
  struct data_for_external_asset_signing_tx
  {
    currency::blobdata unsigned_tx;
    crypto::secret_key tx_secret_key;
    std::vector<std::string> outputs_addresses;
    currency::blobdata finalized_tx;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_BLOB_AS_BASE64_STRING(unsigned_tx) DOC_DSCR("Base64-encoded unsigned transaction blob.") DOC_EXMP("ewogICJ2ZXJzaW9uIjogMSwgC....iAgInZpbiI6IFsgewogICAgIC") DOC_END
      KV_SERIALIZE_POD_AS_HEX_STRING(tx_secret_key)   DOC_DSCR("Hex-encoded transaction secret key.") DOC_EXMP("2e0b840e70dba386effd64c5d988622dea8c064040566e6bf035034cbb54a5c08") DOC_END
      KV_SERIALIZE(outputs_addresses)                 DOC_DSCR("Target address for each of the transaction zoutput.") DOC_EXMP_AGGR("ZxDNaMeZjwCjnHuU5gUNyrP1pM3U5vckbakzzV6dEHyDYeCpW8XGLBFTshcaY8LkG9RQn7FsQx8w2JeJzJwPwuDm2NfixPAXf", "ZxBvJDuQjMG9R2j4WnYUhBYNrwZPwuyXrC7FHdVmWqaESgowDvgfWtiXeNGu8Px9B24pkmjsA39fzSSiEQG1ekB225ZnrMTBp") DOC_END
      KV_SERIALIZE_BLOB_AS_BASE64_STRING(finalized_tx)DOC_DSCR("Base64-encoded finalized_tx data structure, which should be passed along with submitting the transaction.") DOC_EXMP("ewogICJ2ZXJzaW9uIjogMSwgC....iAgInZpbiI6IFsgewogICAgIC") DOC_END
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_ASSETS_EMIT
  {
    DOC_COMMAND("Emmit new coins of the the asset, that is controlled by this wallet.");

    struct request
    {
      crypto::public_key asset_id;
      std::list<transfer_destination> destinations;
      bool do_not_split_destinations = false;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(asset_id) DOC_DSCR("Id of the asset to emit more coins") DOC_EXMP("40fa6db923728b38962718c61b4dc3af1acaa1967479c73703e260dc3609c58d") DOC_END
        KV_SERIALIZE(destinations)               DOC_DSCR("Addresses where to receive emitted coins. Asset id in the destinations is irreleant and can be omitted.") DOC_EXMP_AUTO(1) DOC_END
        KV_SERIALIZE(do_not_split_destinations)  DOC_DSCR("If true, the provided destinations will be used as-is and won't be splitted (or altered) to avoid common issues. Default is false.") DOC_EXMP(false) DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      crypto::hash tx_id;
      std::optional<data_for_external_asset_signing_tx> data_for_external_signing;
    
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(tx_id)    DOC_DSCR("Id of transaction that emits the required asset.") DOC_EXMP("f74bb56a5b4fa562e679ccaadd697463498a66de4f1760b2cd40f11c3a00a7a8") DOC_END
        KV_SERIALIZE(data_for_external_signing)  DOC_DSCR("[optional] Additional data for external asset tx signing.") DOC_EXMP_AGGR() DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_ASSETS_UPDATE
  {
    DOC_COMMAND("Update asset descriptor(you can change only owner so far)");

    struct request
    {
      crypto::public_key asset_id;
      currency::asset_descriptor_base asset_descriptor;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(asset_id)  DOC_DSCR("Id of the asset to update") DOC_EXMP("40fa6db923728b38962718c61b4dc3af1acaa1967479c73703e260dc3609c58d") DOC_END
        KV_SERIALIZE(asset_descriptor) DOC_DSCR("Descriptor that holds all information about asset that need to be updated (only owner could be updated)") DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      crypto::hash tx_id;
      std::optional<data_for_external_asset_signing_tx> data_for_external_signing;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(tx_id)    DOC_DSCR("Id of transaction that carries asset registration command, asset would be registered as soon as transaction got confirmed") DOC_EXMP("f74bb56a5b4fa562e679ccaadd697463498a66de4f1760b2cd40f11c3a00a7a8") DOC_END
        KV_SERIALIZE(data_for_external_signing)  DOC_DSCR("[optional] Hex-encoded transaction for external signing. ") DOC_EXMP_AGGR() DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_ASSETS_BURN
  {
    DOC_COMMAND("Burn some owned amount of the coins for the given asset.");

    struct request
    {
      crypto::public_key asset_id = currency::null_pkey;
      uint64_t burn_amount = 0;
      //optional params
      std::string point_tx_to_address;
      uint64_t native_amount = 0;
      std::vector<currency::tx_service_attachment> service_entries;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(asset_id)  DOC_DSCR("Id of the asset to burn") DOC_EXMP("40fa6db923728b38962718c61b4dc3af1acaa1967479c73703e260dc3609c58d") DOC_END
        KV_SERIALIZE(burn_amount) DOC_DSCR("Amount to burn") DOC_EXMP(10000000) DOC_END
        KV_SERIALIZE(point_tx_to_address) DOC_DSCR("Optional, if we need this transaction to be seen by particular wallet") DOC_EXMP("ZxBvJDuQjMG9R2j4WnYUhBYNrwZPwuyXrC7FHdVmWqaESgowDvgfWtiXeNGu8Px9B24pkmjsA39fzSSiEQG1ekB225ZnrMTBp") DOC_END
        KV_SERIALIZE(native_amount) DOC_DSCR("Optional, if we need this transaction to be seen by particular wallet") DOC_EXMP(0) DOC_END
        KV_SERIALIZE(service_entries) DOC_DSCR("Optional, if we need to include service entries for burn transaction") DOC_EXMP_AUTO(1) DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      crypto::hash tx_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_POD_AS_HEX_STRING(tx_id)    DOC_DSCR("Id of transaction that carries asset burn operation") DOC_EXMP("f74bb56a5b4fa562e679ccaadd697463498a66de4f1760b2cd40f11c3a00a7a8") DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_ASSET_SEND_EXT_SIGNED_TX
  {
    DOC_COMMAND("Inserts externally made asset ownership signature into the given transaction and broadcasts it.");

    struct request
    {
      currency::blobdata    finalized_tx;
      currency::blobdata    unsigned_tx;
      crypto::eth_signature eth_sig; //TODO: add value initialization here 
      crypto::hash          expected_tx_id = currency::null_hash;
      bool                  unlock_transfers_on_fail = false;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_BLOB_AS_BASE64_STRING(finalized_tx)DOC_DSCR("Base64-encoded finalized_tx data structure, which was received from emit_asset call.") DOC_EXMP("ewogICJ2ZXJzaW9uIjogMSwgC....iAgInZpbiI6IFsgewogICAgIC") DOC_END
        KV_SERIALIZE_BLOB_AS_BASE64_STRING(unsigned_tx) DOC_DSCR("Base64-encoded unsigned transaction blob, which was received from emit_asset call.") DOC_EXMP("083737bcfd826a973f74bb56a52b4fa562e6579ccaadd2697463498a66de4f1760b2cd40f11c3a00a7a80000") DOC_END
        KV_SERIALIZE_POD_AS_HEX_STRING(eth_sig)         DOC_DSCR("HEX-encoded ETH signature (64 bytes)") DOC_EXMP("674bb56a5b4fa562e679ccacc4e69455e63f4a581257382191de6856c2156630b3fba0db4bdd73ffcfb36b6add697463498a66de4f1760b2cd40f11c3a00a7a8") DOC_END
        KV_SERIALIZE_POD_AS_HEX_STRING(expected_tx_id)  DOC_DSCR("The expected transaction id. Tx won't be sent if the calculated one doesn't match this one. Consider using 'verified_tx_id' returned by 'decrypt_tx_details' call.") DOC_EXMP("40fa6db923728b38962718c61b4dc3af1acaa1967479c73703e260dc3609c58d") DOC_END
        KV_SERIALIZE(unlock_transfers_on_fail)          DOC_DSCR("If true, all locked wallet transfers, corresponding to the transaction, will be unlocked on sending failure. False by default.") DOC_EXMP(false) DOC_END
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string           status;
      bool                  transfers_were_unlocked = false;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)                     DOC_DSCR("Status of the call") DOC_EXMP("OK") DOC_END
        KV_SERIALIZE(transfers_were_unlocked)    DOC_DSCR("If true, all input transfers that were locked when preparing this transaction, are now unlocked and may be spent. Can be true only upon sending failure and if requested.") DOC_EXMP(false) DOC_END
      END_KV_SERIALIZE_MAP()
    };
  };


} // namespace wallet_rpc
} // namespace tools
