// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <boost/algorithm/string/case_conv.hpp>

#include "string_coding.h"
#include "offers_services_helpers.h"
#include "etc_custom_serialization.h"

#include "misc_language.h" 
#include "currency_basic.h"


namespace bc_services
{


  bool order_offers_by_timestamp(const offer_details_ex* a, const offer_details_ex* b)
  {
    return a->timestamp < b->timestamp;
  }
  //------------------------------------------------------------------
  bool order_offers_by_amount_primary(const offer_details_ex* a, const offer_details_ex* b)
  {
    return a->amount_primary < b->amount_primary;
  }
  //------------------------------------------------------------------
  bool order_offers_by_amount_target(const offer_details_ex* a, const offer_details_ex* b)
  {
    return a->amount_target < b->amount_target;
  }
  //------------------------------------------------------------------
  bool order_offers_by_rate(const offer_details_ex* a, const offer_details_ex* b)
  {
    return calculate_offer_rate(*a) < calculate_offer_rate(*b);
  }
  //------------------------------------------------------------------
  bool order_offers_by_payment_types(const offer_details_ex* a, const offer_details_ex* b)
  {
    return a->payment_types < b->payment_types;
  }
  //------------------------------------------------------------------
  bool order_offers_by_contacts(const offer_details_ex* a, const offer_details_ex* b)
  {
    return a->contacts < b->contacts;
  }
  //------------------------------------------------------------------
  bool order_offers_by_location(const offer_details_ex* a, const offer_details_ex* b)
  {
    return a->location_city < b->location_city;
  }
  //------------------------------------------------------------------
  bool order_offers_by_name(const offer_details_ex* a, const offer_details_ex* b)
  {
#ifndef MOBILE_WALLET_BUILD
    return currency::utf8_to_lower(a->target) < currency::utf8_to_lower(b->target);
#else
    return false;
#endif
  }
  //------------------------------------------------------------------
  std::vector<sort_offers_func_type> gsort_offers_predicates;
  bool init_vector = epee::misc_utils::static_initializer([](){
    gsort_offers_predicates.resize(ORDER_CALLBACKS_COUNT);
    gsort_offers_predicates[ORDER_BY_TIMESTAMP] = &order_offers_by_timestamp;
    gsort_offers_predicates[ORDER_BY_AMOUNT_PRIMARY] = &order_offers_by_amount_primary;
    gsort_offers_predicates[ORDER_BY_AMOUNT_TARGET] = &order_offers_by_amount_target;
    gsort_offers_predicates[ORDER_BY_AMOUNT_RATE] = &order_offers_by_rate;
    gsort_offers_predicates[ORDER_BY_PAYMENT_TYPES] = &order_offers_by_payment_types;
    gsort_offers_predicates[ORDER_BY_CONTACTS] = &order_offers_by_contacts;
    gsort_offers_predicates[ORDER_BY_LOCATION] = &order_offers_by_location;
    gsort_offers_predicates[ORDER_BY_NAME] = &order_offers_by_name;
    return true;
  });
  //  std::locale locale_utf8;
  bool init_locale = epee::misc_utils::static_initializer([](){
    setlocale(LC_ALL, "");
    return true;
  });




  to_low_case_ptr_type to_low_case_external = nullptr;

  void set_external_to_low_converter(to_low_case_ptr_type cb)
  {
    to_low_case_external = cb;
  }


  std::wstring to_lower_local_w(const std::string& s)
  {
    //std::u16string a;
    std::wstring ws = epee::string_encoding::utf8_to_wstring(s);
    if (to_low_case_external)
      return to_low_case_external(ws);
    //std::transform(ws.begin(), ws.end(), ws.begin(), ::towlower);
    boost::algorithm::to_lower(ws);
    return ws;
  }

#define SECONDS_IN_ONE_DAY (60*60*24)


  bool is_offer_matched_by_filter(const offer_details_ex& o, const core_offers_filter& of, uint64_t current_time)
  {
    if (o.stopped)
      return false;
    //check offer type condition
    if (of.offer_type_mask)
    {
      switch (o.offer_type)
      {
      case OFFER_TYPE_PRIMARY_TO_TARGET:
        if (!(of.offer_type_mask & OFFER_TYPE_MASK_PRIMARY_TO_TARGET))
          return false;
        break;
      case OFFER_TYPE_TARGET_TO_PRIMARY:
        if (!(of.offer_type_mask & OFFER_TYPE_MASK_TARGET_TO_PRIMARY))
          return false;
        break;
      case OFFER_TYPE_GOODS_TO_PRIMARY:
        if (!(of.offer_type_mask & OFFER_TYPE_MASK_GOODS_TO_PRIMARY))
          return false;
        break;
      case OFFER_TYPE_PRIMARY_TO_GOODS:
        if (!(of.offer_type_mask & OFFER_TYPE_MASK_PRIMARY_TO_GOODS))
          return false;
        break;
      default:
        return false;
      }
    }

    //check timestamp condition
    if (of.timestamp_start && (o.timestamp < of.timestamp_start))
      return false;
    if (of.timestamp_stop && (o.timestamp > of.timestamp_stop))
      return false;

    //check bonus field
    if (of.bonus && o.bonus.empty())
      return false;

    //check category
    if (!of.category.empty() && o.category.find(of.category) == std::string::npos)
      return false;
#ifndef MOBILE_WALLET_BUILD
    //check target condition
    if (of.target.size() && !currency::utf8_substring_test_case_insensitive(of.target, o.target))
      return false;

    if (of.primary.size() && !currency::utf8_substring_test_case_insensitive(of.primary, o.primary))
      return false;
#endif

    //check payment_types condition (TODO: add more complicated algo here)
    if (of.payment_types.size())
    {
      for (const auto& ot : of.payment_types)
      {
        if (std::string::npos == o.payment_types.find(ot))
          return false;
      }
    }


    //check target condition
    if (of.location_country.size() && (of.location_country != o.location_country))
      return false;
#ifndef MOBILE_WALLET_BUILD
    //check target condition
    if (of.location_city.size() && !currency::utf8_substring_test_case_insensitive(of.location_city, o.location_city))
      return false;
#endif
    //check amount
    if (of.amount_low_limit && (of.amount_low_limit > o.amount_primary))
      return false;
    if (of.amount_up_limit && (of.amount_up_limit < o.amount_primary))
      return false;

    double rate = ((static_cast<double>(o.amount_target) / ETC_AMOUNT_DIVIDER)) / (static_cast<double>(o.amount_primary) / COIN);
    //check rate
    if (of.rate_low_limit && (of.rate_low_limit > rate))
      return false;
    if (of.rate_up_limit && (of.rate_up_limit < rate))
      return false;



    //check keywords
    if (!of.keyword.empty())
    {
      std::wstring all_in_apper;
      all_in_apper += to_lower_local_w(o.bonus);
      all_in_apper += to_lower_local_w(o.target);
      all_in_apper += to_lower_local_w(o.location_country);
      all_in_apper += to_lower_local_w(o.location_city);
      all_in_apper += to_lower_local_w(o.contacts);
      all_in_apper += to_lower_local_w(o.comment);
      all_in_apper += to_lower_local_w(o.payment_types);
      all_in_apper += to_lower_local_w(o.deal_option);
      all_in_apper += to_lower_local_w(o.category);
      std::wstring keyword_lowcase = to_lower_local_w(of.keyword);
      if (all_in_apper.find(keyword_lowcase) == std::wstring::npos)
      {
        return false;
      }
    }
    if (o.expiration_time && current_time > o.timestamp && current_time - o.timestamp > o.expiration_time*SECONDS_IN_ONE_DAY)
      return false;


    return true;
  }
  //--------------------------------------------------------------------------------
  bool filter_offers_list(std::list<bc_services::offer_details_ex>& offers, const bc_services::core_offers_filter& filter, uint64_t current_core_time)
  {
#ifndef MOBILE_WALLET_BUILD
    //filter    
    offers.remove_if([&](bc_services::offer_details_ex& o) -> bool {
      return !is_offer_matched_by_filter(o, filter, current_core_time);
    });
#endif
    //sort 
    CHECK_AND_ASSERT_MES(filter.order_by < gsort_offers_predicates.size(), false, "Wrong cof.order_by value");
    auto cb = *gsort_offers_predicates[static_cast<size_t>(filter.order_by)];

    //sort iterators in needed way
    if (!filter.reverse)
    {
      offers.sort([&](const bc_services::offer_details_ex& a, const bc_services::offer_details_ex& b)
      {
        return cb(&a, &b);
      });
    }
    else
    {
      offers.sort([&](const bc_services::offer_details_ex& a, const bc_services::offer_details_ex& b)
      {
        return cb(&b, &a);
      });
    }

    //now cut what not needed
    auto it = std::next(offers.begin(), std::min(filter.offset, static_cast<uint64_t>(offers.size())));
    offers.erase(offers.begin(), it);
    // cut limit
    it = std::next(offers.begin(), std::min(filter.limit, static_cast<uint64_t>(offers.size())));
    offers.erase(it, offers.end());

    return true;
  }
  //--------------------------------------------------------------------------------
  crypto::hash offer_id_from_hash_and_index(const crypto::hash& tx_id, uint64_t index)
  {
    crypto::hash r = tx_id;
    //xor
    *(uint64_t*)&r ^= index;
    return r;
  }
  //---------------------------------------------------------------
  crypto::hash offer_id_from_hash_and_index(const offer_id& oid)
  {
    return offer_id_from_hash_and_index(oid.tx_id, oid.index);
  }
  //---------------------------------------------------------------
  const crypto::public_key get_offer_secure_key_by_index_from_tx(const currency::transaction& tx, size_t index)
  {
    size_t count = 0;
    for (const auto& a : tx.attachment)
    {
      if (a.type() == typeid(currency::tx_service_attachment))
      {
        const currency::tx_service_attachment& txsa = boost::get<currency::tx_service_attachment>(a);
        if (txsa.service_id == BC_OFFERS_SERVICE_ID)
        {
          if (count != index)
          {
            count++;
            continue;
          }

          if (txsa.security.empty())
            return currency::null_pkey;
          return txsa.security.back();
        }
      }
    }
    return currency::null_pkey;
  }


} // namespace bc_services
