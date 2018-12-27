// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 



#include "serialization/keyvalue_serialization.h"
#include "crypto/crypto.h"
#include "etc_custom_serialization.h"
namespace bc_services
{
  /************************************************************************/
  /* offer structures                                                */
  /************************************************************************/
  struct offer_details
  {

    //fields filled in UI
    uint8_t offer_type;             // OFFER_TYPE_PRIMARY_TO_TARGET - 0, OFFER_TYPE_TARGET_TO_PRIMARY - 1 etc.
    uint64_t amount_primary;        // amount of the currency
    uint64_t amount_target;         // amount of other currency or goods
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
    uint8_t expiration_time;        // n-days
    //-----------------

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_N(offer_type, "ot")
      KV_SERIALIZE_CUSTOM_N(amount_primary, std::string, bc_services::transform_amount_to_string, bc_services::transform_string_to_amount, "ap")
      KV_SERIALIZE_CUSTOM_N(amount_target, std::string, bc_services::transform_amount_to_string, bc_services::transform_string_to_amount, "at")
      KV_SERIALIZE_N(bonus, "b")
      KV_SERIALIZE_N(target, "t")
      KV_SERIALIZE_N(primary, "p")
      KV_SERIALIZE_N(location_country, "lco")
      KV_SERIALIZE_N(location_city, "lci")
      KV_SERIALIZE_N(contacts, "cnt")
      KV_SERIALIZE_N(comment, "com")
      KV_SERIALIZE_N(payment_types, "pt")
      KV_SERIALIZE_N(deal_option, "do")
      KV_SERIALIZE_N(category, "cat")
      KV_SERIALIZE_N(expiration_time, "et")
    END_KV_SERIALIZE_MAP()
  };

  struct offer_details_ex : public offer_details
  {
    //fields contained in dictionary and also returned to UI/ 
    crypto::hash tx_hash;
    crypto::hash tx_original_hash;
    uint64_t index_in_tx;
    uint64_t timestamp;        //this is not kept by transaction, info filled by corresponding transaction
    uint64_t fee;              //value of fee to pay(or paid in case of existing offers) to rank it
    crypto::public_key security; //key used for updating offer
    //-----------------

    mutable bool stopped;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_POD_AS_HEX_STRING(tx_hash)
      KV_SERIALIZE_POD_AS_HEX_STRING(tx_original_hash)
      KV_SERIALIZE(index_in_tx)
      KV_SERIALIZE(timestamp)
      KV_SERIALIZE(fee)
      KV_SERIALIZE_POD_AS_HEX_STRING(security)
      KV_CHAIN_BASE(offer_details)
    END_KV_SERIALIZE_MAP()
  };

  struct cancel_offer
  {

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_POD_AS_HEX_STRING_N(tx_id, "id")
      KV_SERIALIZE_N(offer_index, "oi")
      KV_SERIALIZE_POD_AS_HEX_STRING_N(sig, "sig")
    END_KV_SERIALIZE_MAP()

    crypto::hash tx_id;
    uint64_t offer_index;
    crypto::signature sig; //tx_id signed by transaction secret key
  };


  struct update_offer
  {

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_POD_AS_HEX_STRING_N(tx_id, "id")
      KV_SERIALIZE_N(offer_index, "oi")
      KV_SERIALIZE_POD_AS_HEX_STRING_N(sig, "sig")
      KV_SERIALIZE_N(of, "of")
    END_KV_SERIALIZE_MAP()

    crypto::hash tx_id;
    uint64_t offer_index;
    crypto::signature sig; //tx_id signed by transaction secret key
    offer_details of;
  };

  struct update_offer_details
  {
    uint64_t wallet_id;
    crypto::hash tx_id;
    uint64_t no;
    offer_details_ex od;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(wallet_id)
      KV_SERIALIZE_POD_AS_HEX_STRING_N(tx_id, "id")
      KV_SERIALIZE_N(no, "oi")
      KV_SERIALIZE_N(od, "of")
    END_KV_SERIALIZE_MAP()
  };


  typedef boost::variant<offer_details, update_offer, cancel_offer> offers_attachment_t;
}