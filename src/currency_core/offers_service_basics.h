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
      KV_SERIALIZE_N(preview_url, "url")
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

  inline std::string transform_double_to_string(const double& a)
  {
    return std::to_string(a);
  }

  inline double transform_string_to_double(const std::string& d)
  {
    double n = 0;
    epee::string_tools::get_xtype_from_string(n, d);
    return n;
  }


  struct core_offers_filter
  {
    uint64_t order_by;
    bool reverse;
    uint64_t offset;
    uint64_t limit;
    //filter entry
    uint64_t timestamp_start;
    uint64_t timestamp_stop;
    uint64_t offer_type_mask;
    uint64_t amount_low_limit;
    uint64_t amount_up_limit;
    double rate_low_limit;
    double rate_up_limit;
    std::list<std::string> payment_types;
    std::string location_country;
    std::string location_city;
    std::string target;
    std::string primary;
    bool bonus;
    std::string category;
    std::string keyword;
    bool fake;
    uint64_t current_time;


    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(order_by)
      KV_SERIALIZE(reverse)
      KV_SERIALIZE(offset)
      KV_SERIALIZE(limit)
      KV_SERIALIZE(timestamp_start)
      KV_SERIALIZE(timestamp_stop)
      KV_SERIALIZE(offer_type_mask)
      KV_SERIALIZE(amount_low_limit)
      KV_SERIALIZE(amount_up_limit)
      KV_SERIALIZE_CUSTOM(rate_low_limit, std::string, bc_services::transform_double_to_string, bc_services::transform_string_to_double)
      KV_SERIALIZE_CUSTOM(rate_up_limit, std::string, bc_services::transform_double_to_string, bc_services::transform_string_to_double)
      KV_SERIALIZE(payment_types)
      KV_SERIALIZE(location_country)
      KV_SERIALIZE(location_city)
      KV_SERIALIZE(target)
      KV_SERIALIZE(primary)
      KV_SERIALIZE(bonus)
      KV_SERIALIZE(category)
      KV_SERIALIZE(keyword)
      KV_SERIALIZE(fake)
    END_KV_SERIALIZE_MAP()
  };


}