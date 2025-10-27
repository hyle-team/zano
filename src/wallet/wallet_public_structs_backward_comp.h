// Copyright (c) 2025 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include "currency_core/currency_basic.h"
#include "currency_core/offers_service_basics.h"
#include <common/variant_helper.h>
//#include "wallet_public_structs_defs.h"

namespace tools::legacy
{
  struct offer_details_hf5
  {
    //fields filled in UI
    uint8_t offer_type = 0;         // OFFER_TYPE_PRIMARY_TO_TARGET(SELL ORDER) - 0, OFFER_TYPE_TARGET_TO_PRIMARY(BUY ORDER) - 1 etc.
    uint64_t amount_primary = 0;    // amount of the currency
    uint64_t amount_target = 0;     // amount of other currency or goods
    std::string bonus;              //
    std::string target;             // [] currency / goods
    std::string primary;            // currency for goods
    std::string location_country;   // ex: US
    std::string location_city;      // ex: ChIJD7fiBh9u5kcRYJSMaMOCCwQ (google geo-autocomplete id)
    std::string contacts;           // [] (Skype, mail, ICQ, etc., website)
    std::string comment;            // []
    std::string payment_types;      // []money accept type(bank transaction, internet money, cash, etc)
    std::string deal_option;        // []full amount, by parts
    std::string category;           // []
    std::string preview_url;        // []
    uint8_t expiration_time = 0;    // n-days
    //-----------------
    // ( kv serialization removed )
  };

  template <class Archive>
  inline void serialize(Archive &a, offer_details_hf5 &x, const boost::serialization::version_type ver)
  {
    a & x.offer_type;
    a & x.amount_primary;
    a & x.amount_target;
    a & x.bonus;
    a & x.target;
    a & x.primary;
    a & x.location_country;
    a & x.location_city;
    a & x.contacts;
    a & x.comment;
    a & x.payment_types;
    a & x.expiration_time;
    a & x.category;
    a & x.deal_option;

  }

  struct offer_details_ex_hf5 : public offer_details_hf5
  {
    //fields contained in dictionary and also returned to UI/ 
    crypto::hash tx_hash;
    crypto::hash tx_original_hash;
    uint64_t index_in_tx;
    uint64_t timestamp;        //this is not kept by transaction, info filled by corresponding transaction
    uint64_t fee;              //value of fee to pay(or paid in case of existing offers) to rank it
    crypto::public_key security; //key used for updating offer
    //-----------------
    // ( kv serialization removed )

    mutable bool stopped;
  };

  template <class Archive>
  inline void serialize(Archive& a, offer_details_ex_hf5& x, const boost::serialization::version_type ver)
  {
    a & static_cast<offer_details_hf5&>(x);
    a & x.timestamp;
    a & x.tx_hash;
    a & x.index_in_tx;
    a & x.tx_original_hash;
    a & x.fee;
    a & x.stopped;
    a & x.security;
  }

  struct cancel_offer_hf5
  {
    // ( kv serialization removed )
    crypto::hash tx_id;
    uint64_t offer_index;
    crypto::signature sig; //tx_id signed by transaction secret key
  };

  template <class Archive>
  inline void serialize(Archive &a, cancel_offer_hf5 &x, const boost::serialization::version_type ver)
  {
    a & x.offer_index;
    a & x.sig;
    a & x.tx_id;
  }

  struct update_offer_hf5
  {
    // ( kv serialization removed )
    crypto::hash tx_id;
    uint64_t offer_index;
    crypto::signature sig; //tx_id signed by transaction secret key
    offer_details_hf5 of;
  };

  template <class Archive>
  inline void serialize(Archive &a, update_offer_hf5 &x, const boost::serialization::version_type ver)
  {
    a & x.of;
    a & x.offer_index;
    a & x.sig;
    a & x.tx_id;
  }

  struct update_offer_details_hf5
  {
    uint64_t wallet_id;
    crypto::hash tx_id;
    uint64_t no;
    offer_details_ex_hf5 od;
    // ( kv serialization removed )
  };

  typedef boost::variant<offer_details_hf5, update_offer_hf5, cancel_offer_hf5> offers_attachment_t_hf5;

  struct proposal_body_hf5
  {
    currency::transaction   tx_template;
    crypto::secret_key      tx_onetime_secret_key;
    // ( binary serialization removed )
  };

  template <class Archive>
  inline void serialize(Archive& a, proposal_body_hf5& x, const unsigned int ver)
  {
    a & x.tx_onetime_secret_key;
    a & x.tx_template;
  }

  struct escrow_relese_templates_body_hf5
  {
    currency::transaction   tx_normal_template;
    currency::transaction   tx_burn_template;
    // ( binary serialization removed )
  };

  template <class Archive>
  inline void serialize(Archive& a, escrow_relese_templates_body_hf5& x, const unsigned int ver)
  {
    a & x.tx_normal_template;
    a & x.tx_burn_template;
  }

  struct escrow_cancel_templates_body_hf5
  {
    currency::transaction   tx_cancel_template;
    // ( binary serialization removed )
  };

  template <class Archive>
  inline void serialize(Archive& a, escrow_cancel_templates_body_hf5& x, const unsigned int ver)
  {
    a & x.tx_cancel_template;
  }

  struct contract_private_details_hf5
  {
    std::string title;
    std::string comment;
    currency::account_public_address a_addr; // usually buyer
    currency::account_public_address b_addr; // usually seller
    uint64_t amount_to_pay;
    uint64_t amount_a_pledge;
    uint64_t amount_b_pledge;
    // ( kv serialization removed )
  };

  template<class archive_t>
  void serialize(archive_t & ar, contract_private_details_hf5& cpd, const unsigned int version)
  {
    ar & cpd.title;
    ar & cpd.comment;
    if (version == 0 && !archive_t::is_saving::value)
    {
      // loading from version 0 (pre-auditable address format)
      currency::account_public_address_old old_addr = AUTO_VAL_INIT(old_addr);
      ar & old_addr;
      cpd.a_addr = currency::account_public_address::from_old(old_addr);
      ar & old_addr;
      cpd.b_addr = currency::account_public_address::from_old(old_addr);
    }
    else
    {
      ar & cpd.a_addr; // usually buyer
      ar & cpd.b_addr; // usually seller
    }
    ar & cpd.amount_to_pay;
    ar & cpd.amount_a_pledge;
    ar & cpd.amount_b_pledge;
  }

  struct escrow_contract_details_basic_hf5
  {
    uint32_t state;
    bool is_a; // "a" - supposed  to be a buyer
    contract_private_details_hf5 private_detailes;
    uint64_t expiration_time;
    uint64_t cancel_expiration_time;
    uint64_t timestamp;
    uint64_t height; // height of the last state change
    std::string payment_id;

   
    //is not kv_serialization map
    proposal_body_hf5 proposal;
    escrow_relese_templates_body_hf5 release_body;
    escrow_cancel_templates_body_hf5 cancel_body;
    // ( kv serialization removed )
  };
  
  template <class Archive>
  inline void serialize(Archive& a, escrow_contract_details_basic_hf5& x, const boost::serialization::version_type ver)
  {
    a & x.state;
    a & x.is_a;
    a & x.private_detailes;
    a & x.expiration_time;
    a & x.cancel_expiration_time;
    a & x.timestamp;
    a & x.height;
    a & x.payment_id;
    //is not kv_serialization map
    a & x.proposal;
    a & x.release_body;
    a & x.cancel_body;
  }

  struct escrow_contract_details_hf5 : public escrow_contract_details_basic_hf5
  {
    crypto::hash contract_id; //multisig id
    // ( kv serialization removed )
  };

  template <class Archive>
  inline void serialize(Archive& a, escrow_contract_details_hf5& x, const boost::serialization::version_type ver)
  {
    a & static_cast<escrow_contract_details_basic_hf5&>(x);
    a & x.contract_id;
  }


  struct tx_service_attachment_hf5
  {
    std::string service_id;    //string identifying service which addressed this attachment
    std::string instruction;   //string identifying specific instructions for service/way to interpret data
    std::string body;          //any data identifying service, options etc
    std::vector<crypto::public_key> security; //some of commands need proof of owner
    uint8_t flags;             //special flags (ex: TX_SERVICE_ATTACHMENT_ENCRYPT_BODY), see below

    // ( binary serialization removed )
    // ( kv serialization removed )
  };

  template <class Archive>
  inline void serialize(Archive &a, tx_service_attachment_hf5 &at, const boost::serialization::version_type ver)
  {
    a & at.service_id;
    a & at.security;
    a & at.instruction;
    a & at.body;
    a & at.flags;
  }



  struct employed_tx_entry_hf5
  {
    uint64_t index = 0;
    uint64_t amount = 0;
    crypto::public_key asset_id = currency::native_coin_asset_id;

    // ( kv serialization removed )

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(index)
      BOOST_SERIALIZE(amount)
      BOOST_SERIALIZE(asset_id)
    END_BOOST_SERIALIZATION()

  };

  struct employed_tx_entries_hf5
  {
    std::vector<employed_tx_entry_hf5> receive;
    std::vector<employed_tx_entry_hf5> spent;

    // ( kv serialization removed )

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(receive)
      BOOST_SERIALIZE(spent)
    END_BOOST_SERIALIZATION()
  };

  struct wallet_sub_transfer_info_hf5
  {
    uint64_t      amount = 0;
    bool          is_income = false;
    crypto::public_key asset_id = currency::native_coin_asset_id; // not blinded, not premultiplied by 1/8
    
    // ( kv serialization removed )

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(amount) 
      BOOST_SERIALIZE(is_income)
      BOOST_SERIALIZE(asset_id)
    END_BOOST_SERIALIZATION()
  };

  struct wallet_transfer_info_hf5
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
    employed_tx_entries_hf5 employed_entries;
    std::vector<tx_service_attachment_hf5> service_entries;
    std::vector<std::string> remote_addresses;  //optional
    std::vector<std::string> remote_aliases; //optional, describe only if there only one remote address
    std::vector<wallet_sub_transfer_info_hf5> subtransfers;

    //not included in streaming serialization
    uint64_t      fee = 0;
    bool          show_sender = false;
    std::vector<escrow_contract_details_hf5> contract;
    uint16_t      extra_flags = 0;
    uint64_t      transfer_internal_index = 0;
   
    //not included in kv serialization map
    currency::transaction tx;
    std::vector<uint64_t> selected_indicies;
    std::list<offers_attachment_t_hf5> marketplace_entries;

    // ( kv serialization removed )

    BOOST_SERIALIZATION_CURRENT_ARCHIVE_VER(12)
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

    // ( additional member functions removed )
  };


} // namespace tools::legacy


inline void transition_convert(tools::legacy::employed_tx_entries_hf5&& from, tools::wallet_public::employed_tx_entries& to)
{
  for(auto& el : from.receive)
    to.receive.push_back(tools::wallet_public::employed_tx_entry{el.index, el.amount, el.asset_id, 0 /* <- payment id */});
  for(auto& el : from.spent)
    to.spent.push_back(tools::wallet_public::employed_tx_entry{el.index, el.amount, el.asset_id, 0 /* <- payment id */});
}

inline void transition_convert(std::vector<tools::legacy::escrow_contract_details_hf5>&& from, std::vector<tools::wallet_public::escrow_contract_details>& to)
{
  for(auto& f : from)
  {
    auto& t = to.emplace_back();
    t.cancel_body.tx_cancel_template    = std::move(f.cancel_body.tx_cancel_template);
    t.cancel_expiration_time            = f.cancel_expiration_time;
    t.contract_id                       = f.contract_id;
    t.expiration_time                   = f.expiration_time;
    t.height                            = f.height;
    t.is_a                              = f.is_a;
    t.payment_id                        = std::move(f.payment_id);
    t.private_detailes.amount_a_pledge  = f.private_detailes.amount_a_pledge;
    t.private_detailes.amount_b_pledge  = f.private_detailes.amount_b_pledge;
    t.private_detailes.amount_to_pay    = f.private_detailes.amount_to_pay;
    t.private_detailes.a_addr           = f.private_detailes.a_addr;
    t.private_detailes.b_addr           = f.private_detailes.b_addr;
    t.private_detailes.comment          = std::move(f.private_detailes.comment);
    t.private_detailes.title            = std::move(f.private_detailes.title);
    t.proposal.tx_onetime_secret_key    = f.proposal.tx_onetime_secret_key;
    t.proposal.tx_template              = std::move(f.proposal.tx_template);
    t.release_body.tx_burn_template     = std::move(f.release_body.tx_burn_template);
    t.release_body.tx_normal_template   = std::move(f.release_body.tx_normal_template);
    t.state                             = f.state;
    t.timestamp                         = f.timestamp;
  }
}

inline void transition_convert(tools::legacy::offer_details_hf5&& od, bc_services::offer_details& t)
{
  t.amount_primary    = od.amount_primary;
  t.amount_target     = od.amount_target;
  t.bonus             = std::move(od.bonus);
  t.category          = std::move(od.category);
  t.comment           = std::move(od.comment);
  t.contacts          = std::move(od.contacts);
  t.deal_option       = std::move(od.deal_option);
  t.expiration_time   = od.expiration_time;
  t.location_city     = std::move(od.location_city);
  t.location_country  = std::move(od.location_country);
  t.offer_type        = od.offer_type;
  t.payment_types     = std::move(od.payment_types);
  t.preview_url       = std::move(od.preview_url);
  t.primary           = std::move(od.primary);
  t.target            = std::move(od.target);
}

inline void transition_convert(std::list<tools::legacy::offers_attachment_t_hf5>&& from, std::list<bc_services::offers_attachment_t>& to)
{
  for(auto& f : from)
  {
    VARIANT_SWITCH_BEGIN(f);
      VARIANT_CASE(tools::legacy::offer_details_hf5, od)
        bc_services::offer_details t{};
        transition_convert(std::move(od), t);
        to.push_back(std::move(t));
      VARIANT_CASE(tools::legacy::update_offer_hf5, uo)
        bc_services::update_offer t{};
        transition_convert(std::move(uo.of), t.of);
        t.offer_index = uo.offer_index;
        t.sig         = uo.sig;
        t.tx_id       = uo.tx_id;
        to.push_back(std::move(t));
      VARIANT_CASE(tools::legacy::cancel_offer_hf5, co)
        bc_services::cancel_offer t{};
        t.offer_index = co.offer_index;
        t.sig         = co.sig;
        t.tx_id       = co.tx_id;
        to.push_back(std::move(t));
      VARIANT_CASE_OTHER()
        throw std::logic_error("transition_convert(offers_attachment_t_hf5) : other not implemented");
    VARIANT_SWITCH_END()
  }
}

inline void transition_convert(std::vector<tools::legacy::tx_service_attachment_hf5>&& from, std::vector<currency::tx_service_attachment>& to)
{
  for(auto& f : from)
  {
    auto& t = to.emplace_back();
    t.body        = std::move(f.body);
    t.flags       = f.flags;
    t.instruction = std::move(f.instruction);
    t.security    = std::move(f.security);
    t.service_id  = std::move(f.service_id);
  }
}

inline void transition_convert(tools::legacy::wallet_transfer_info_hf5&& from, tools::wallet_public::wallet_transfer_info& to)
{
  to.timestamp            = from.timestamp;
  to.tx_hash              = from.tx_hash;
  to.height               = from.height;
  to.tx_blob_size         = from.tx_blob_size;
  to.tx_wide_payment_id   = std::move(from.payment_id);
  to.remote_addresses     = std::move(from.remote_addresses);
  transition_convert(std::move(from.employed_entries), to.employed_entries);
  to.tx                   = std::move(from.tx);
  to.remote_aliases       = std::move(from.remote_aliases);
  to.comment              = std::move(from.comment);
  transition_convert(std::move(from.contract), to.contract);
  to.selected_indicies    = std::move(from.selected_indicies);
  transition_convert(std::move(from.marketplace_entries), to.marketplace_entries);
  to.unlock_time          = from.unlock_time;
  transition_convert(std::move(from.service_entries), to.service_entries);

  // subtransfers
  for(auto& el : from.subtransfers)
    to.get_or_add_subtransfers_by_pid(to.tx_wide_payment_id).subtransfers.push_back(tools::wallet_public::wallet_sub_transfer_info{el.amount, el.is_income, el.asset_id});
}

inline void transition_convert(const tools::wallet_public::wallet_transfer_info& from, tools::legacy::wallet_transfer_info_hf5& to)
{
  // intentionally not implemented; we don't need to convert new data structures to old ones -- sowle
  throw std::logic_error("transition_convert(wallet_transfer_info, wallet_transfer_info_hf5) not implemented");
}

LOOP_BACK_BOOST_SERIALIZATION_VERSION(tools::legacy::wallet_transfer_info_hf5);
