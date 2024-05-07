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
      KV_SERIALIZE_N(offer_type, "ot")             DOC_DSCR("Type of the offer: OFFER_TYPE_PRIMARY_TO_TARGET(SELL ORDER) - 0, OFFER_TYPE_TARGET_TO_PRIMARY(BUY ORDER) - 1 etc.") DOC_EXMP(0) DOC_END
      KV_SERIALIZE_CUSTOM_N(amount_primary, std::string, bc_services::transform_amount_to_string, bc_services::transform_string_to_amount, "ap") DOC_DSCR("Amount of the currency") DOC_EXMP("100000") DOC_END
      KV_SERIALIZE_CUSTOM_N(amount_target, std::string, bc_services::transform_amount_to_string, bc_services::transform_string_to_amount, "at")  DOC_DSCR("Smount of other currency or goods") DOC_EXMP("10000000") DOC_END
      KV_SERIALIZE_N(bonus, "b")                   DOC_DSCR("Bonus associated with the offer") DOC_EXMP("") DOC_END
      KV_SERIALIZE_N(target, "t")                  DOC_DSCR("Target:  currency / goods") DOC_EXMP("USDT") DOC_END
      KV_SERIALIZE_N(primary, "p")                 DOC_DSCR("Currency for goods") DOC_EXMP("ZANO") DOC_END
      KV_SERIALIZE_N(location_country, "lco")      DOC_DSCR("Country of the offer location") DOC_EXMP("Montenegro") DOC_END
      KV_SERIALIZE_N(location_city, "lci")         DOC_DSCR("City of the offer location") DOC_EXMP("Kolasin") DOC_END
      KV_SERIALIZE_N(contacts, "cnt")              DOC_DSCR("Contacts related to the offer") DOC_EXMP("Ranko +38211111111") DOC_END
      KV_SERIALIZE_N(comment, "com")               DOC_DSCR("Comment associated with the offer") DOC_EXMP("Dobr dan") DOC_END
      KV_SERIALIZE_N(payment_types, "pt")          DOC_DSCR("Types of payment accepted for the offer") DOC_EXMP("zano") DOC_END
      KV_SERIALIZE_N(deal_option, "do")            DOC_DSCR("Deal option for the offer") DOC_EXMP("full amount, by parts") DOC_END
      KV_SERIALIZE_N(category, "cat")              DOC_DSCR("Category of the offer") DOC_EXMP("") DOC_END
      KV_SERIALIZE_N(expiration_time, "et")        DOC_DSCR("Expiration time of the offer") DOC_EXMP(0) DOC_END
      KV_SERIALIZE_N(preview_url, "url")           DOC_DSCR("URL for previewing the offer") DOC_EXMP("") DOC_END
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
      KV_SERIALIZE_POD_AS_HEX_STRING(tx_hash)           DOC_DSCR("Transaction hash represented as a hexadecimal string") DOC_EXMP("cc608f59f8080e2fbfe3c8c80eb6e6a953d47cf2d6aebd345bada3a1cab99852") DOC_END
      KV_SERIALIZE_POD_AS_HEX_STRING(tx_original_hash)  DOC_DSCR("Origin transaction hash represented as a hexadecimal string(if offer updated)") DOC_EXMP("cc608f59f8080e2fbfe3c8c80eb6e6a953d47cf2d6aebd345bada3a1cab99852") DOC_END
      KV_SERIALIZE(index_in_tx)                         DOC_DSCR("Index of the tx_service_attachment entrie in transaction") DOC_EXMP(0) DOC_END
      KV_SERIALIZE(timestamp)                           DOC_DSCR("Timestamp of the transaction") DOC_EXMP(1712683857) DOC_END
      KV_SERIALIZE(fee)                                 DOC_DSCR("Fee associated with the transaction") DOC_EXMP(10000000000) DOC_END
      KV_SERIALIZE_POD_AS_HEX_STRING(security)          DOC_DSCR("Onwer's public key for access control") DOC_EXMP("40fa6db923728b38962718c61b4dc3af1acaa1967479c73703e260dc3609c58d") DOC_END
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
    //bool fake;
    uint64_t current_time;


    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(order_by)           DOC_DSCR("Field to order the results by one on this: ORDER_BY_TIMESTAMP=0,ORDER_BY_AMOUNT_PRIMARY=1,ORDER_BY_AMOUNT_TARGET=2,ORDER_BY_AMOUNT_RATE=3,ORDER_BY_PAYMENT_TYPES=4,ORDER_BY_CONTACTS=5,ORDER_BY_LOCATION=6,ORDER_BY_NAME=7") DOC_EXMP(0) DOC_END
      KV_SERIALIZE(reverse)            DOC_DSCR("Flag to indicate whether the results should be sorted in reverse order") DOC_EXMP(false) DOC_END
      KV_SERIALIZE(offset)             DOC_DSCR("Offset for pagination") DOC_EXMP(0) DOC_END
      KV_SERIALIZE(limit)              DOC_DSCR("Maximum number of results to return") DOC_EXMP(100) DOC_END
      KV_SERIALIZE(timestamp_start)    DOC_DSCR("Start timestamp for filtering results") DOC_EXMP(0) DOC_END
      KV_SERIALIZE(timestamp_stop)     DOC_DSCR("Stop timestamp for filtering results") DOC_EXMP(0) DOC_END
      KV_SERIALIZE(offer_type_mask)    DOC_DSCR("Mask representing the types of offers to include in the results, conbination of this: OFFER_TYPE_MASK_PRIMARY_TO_TARGET 0x00000001, OFFER_TYPE_MASK_TARGET_TO_PRIMARY 0x00000002, OFFER_TYPE_MASK_GOODS_TO_PRIMARY 0x00000004, OFFER_TYPE_MASK_PRIMARY_TO_GOODS 0x00000008") DOC_EXMP(0) DOC_END
      KV_SERIALIZE(amount_low_limit)   DOC_DSCR("Lower limit for the amount of offers") DOC_EXMP(0) DOC_END
      KV_SERIALIZE(amount_up_limit)    DOC_DSCR("Upper limit for the amount of offers") DOC_EXMP(0) DOC_END
      KV_SERIALIZE_CUSTOM(rate_low_limit, std::string, bc_services::transform_double_to_string, bc_services::transform_string_to_double) DOC_DSCR("Lower limit for the rate") DOC_EXMP("0.1") DOC_END
      KV_SERIALIZE_CUSTOM(rate_up_limit, std::string, bc_services::transform_double_to_string, bc_services::transform_string_to_double)  DOC_DSCR("Upper limit for the rate") DOC_EXMP("0.1") DOC_END
      KV_SERIALIZE(payment_types)      DOC_DSCR("Types of payment accepted for the offers(in a free form as it is in contract)") DOC_END
      KV_SERIALIZE(location_country)   DOC_DSCR("Country of the location for the offers") DOC_END
      KV_SERIALIZE(location_city)      DOC_DSCR("City of the location for the offers") DOC_END
      KV_SERIALIZE(target)             DOC_DSCR("Target entity of the offers") DOC_END
      KV_SERIALIZE(primary)            DOC_DSCR("Primary field for the offers") DOC_END
      KV_SERIALIZE(bonus)              DOC_DSCR("Bonus associated with the offers") DOC_EXMP(false) DOC_END
      KV_SERIALIZE(category)           DOC_DSCR("Category of the offers")  DOC_END
      KV_SERIALIZE(keyword)            DOC_DSCR("Keyword for searching offers") DOC_EXMP("tubes") DOC_END
      //KV_SERIALIZE(fake)               DOC_DSCR("Flag indicating whether the offer is fake") DOC_EXMP() DOC_END
    END_KV_SERIALIZE_MAP()

  };


}