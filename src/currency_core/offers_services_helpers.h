// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "profile_tools.h"
#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage_template_helper.h"
#include "currency_basic.h"
#include "offers_service_basics.h"
#include "currency_protocol/blobdatatype.h"
#include "zlib_helper.h"
#include "bc_offers_service_basic.h"
#include "bc_attachments_helpers.h"

namespace bc_services
{


  struct offer_id
  {
    crypto::hash tx_id;
    uint64_t index;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_POD_AS_HEX_STRING(tx_id)
      KV_SERIALIZE(index)
    END_KV_SERIALIZE_MAP()
  };

#define ORDER_BY_TIMESTAMP                 0
#define ORDER_BY_AMOUNT_PRIMARY            1
#define ORDER_BY_AMOUNT_TARGET             2
#define ORDER_BY_AMOUNT_RATE               3
#define ORDER_BY_PAYMENT_TYPES             4
#define ORDER_BY_CONTACTS                  5
#define ORDER_BY_LOCATION                  6
#define ORDER_BY_NAME                      7

#define ORDER_CALLBACKS_COUNT               8


  typedef std::wstring(*to_low_case_ptr_type)(const std::wstring& soruce);

  void set_external_to_low_converter(to_low_case_ptr_type cb);

  typedef bool(*sort_offers_func_type)(const offer_details_ex* a, const offer_details_ex* b);
  extern std::vector<sort_offers_func_type> gsort_offers_predicates;
  bool is_offer_matched_by_filter(const offer_details_ex& o, const core_offers_filter& of, uint64_t currnet_time);
  bool filter_offers_list(std::list<offer_details_ex>& offers, const core_offers_filter& filter, uint64_t current_core_time);
  crypto::hash offer_id_from_hash_and_index(const crypto::hash& tx_id, uint64_t index);
  crypto::hash offer_id_from_hash_and_index(const offer_id& oid);
  const crypto::public_key get_offer_secure_key_by_index_from_tx(const currency::transaction& tx, size_t index);

  inline double calculate_offer_rate(const offer_details& od)
  {
    return od.amount_primary > 0 ? static_cast<double>(od.amount_target) / od.amount_primary : 0;
  }

  template<class offer_instruction_t>
  void put_offer_into_attachment(const offer_instruction_t& oi, std::vector<currency::attachment_v>& att_container)
  {
    std::string json_buff;
    currency::tx_service_attachment att = AUTO_VAL_INIT(att);

    bool r = bc_services::pack_attachment_as_gzipped_json(oi, att);
    CHECK_AND_ASSERT_MES(r, void(), "Failed to pack_attachment_as_gzipped_json");

    att.service_id = BC_OFFERS_SERVICE_ID;
    if (typeid(offer_instruction_t) == typeid(offer_details))
    {
      att.instruction = BC_OFFERS_SERVICE_INSTRUCTION_ADD;
    }
    else if (typeid(offer_instruction_t) == typeid(update_offer))
    {
      att.instruction = BC_OFFERS_SERVICE_INSTRUCTION_UPD;
    }
    else if (typeid(offer_instruction_t) == typeid(cancel_offer))
    {
      att.instruction = BC_OFFERS_SERVICE_INSTRUCTION_DEL;
    }
    else
    {
      LOG_ERROR("Filed to pack tx_service_attachment in bc_offers_service, unknown_type: " << typeid(offer_instruction_t).name());
      return;
    }
    
    //if att.security vector contain public_key then transaction constructor put there correct public_key related with one time transaction keys
    if (att.instruction == BC_OFFERS_SERVICE_INSTRUCTION_ADD || att.instruction == BC_OFFERS_SERVICE_INSTRUCTION_UPD)
      att.security.push_back(currency::null_pkey);

    att_container.push_back(att);
  }

  inline currency::blobdata make_offer_sig_blob(const update_offer& uo)
  {
    currency::blobdata bd;
    epee::string_tools::append_pod_to_strbuff(bd, uo.tx_id);
    epee::string_tools::append_pod_to_strbuff(bd, uo.offer_index);
    bd += epee::serialization::store_t_to_binary(uo.of);
    return bd;
  }

  inline currency::blobdata make_offer_sig_blob(const cancel_offer& co)
  {
    currency::blobdata bd;
    epee::string_tools::append_pod_to_strbuff(bd, co.tx_id);
    epee::string_tools::append_pod_to_strbuff(bd, co.offer_index);
    return bd;
  }

  //----------------------------------------------------------------------------------------------------
  template<class t_contained_type, class t_srv_attachments_container>
  bool extract_type_and_add(const std::string& body, t_srv_attachments_container& srv_cnt)
  {
    std::string json_buff;
    if (!epee::zlib_helper::unpack(body, json_buff))
    {
      LOG_ERROR("Filed to unpack tx_service_attachment in bc_offers_service, tx_id");
      return false;
    }
    t_contained_type obj = AUTO_VAL_INIT(obj);
    bool r = epee::serialization::load_t_from_json(obj, json_buff);
    if (!r)
    {
      LOG_ERROR("Filed to load json from tx_service_attachment in bc_offers_service, body: " << body);
      return false;
    }
    srv_cnt.push_back(obj);
    return true;
  }
  //----------------------------------------------------------------------------------------------------
  template<class t_srv_attachments_container, class t_tx_attachments_container>
  void extract_market_instructions(t_srv_attachments_container& cnt, const t_tx_attachments_container& raw_att)
  {
    PROFILE_FUNC("bc_service::extract_market_instructions");
    for (auto& att : raw_att)
    {
      if (att.type() == typeid(currency::tx_service_attachment))
      {
        const currency::tx_service_attachment& sa = boost::get<const currency::tx_service_attachment>(att);
        if (sa.service_id == BC_OFFERS_SERVICE_ID)
        {
          if (sa.instruction == BC_OFFERS_SERVICE_INSTRUCTION_ADD)
          {
             extract_type_and_add<bc_services::offer_details>(sa.body, cnt);
          }
          else if (sa.instruction == BC_OFFERS_SERVICE_INSTRUCTION_UPD)
          {
             extract_type_and_add<bc_services::update_offer>(sa.body, cnt);
          }
          else if (sa.instruction == BC_OFFERS_SERVICE_INSTRUCTION_DEL)
          {
            extract_type_and_add<bc_services::cancel_offer>(sa.body, cnt);
          }
        }
      }
    }
  }




#define OFFER_TYPE_PRIMARY_TO_TARGET        0
#define OFFER_TYPE_TARGET_TO_PRIMARY        1
#define OFFER_TYPE_GOODS_TO_PRIMARY         2
#define OFFER_TYPE_PRIMARY_TO_GOODS         3

#define OFFER_TYPE_MASK_PRIMARY_TO_TARGET   0x00000001
#define OFFER_TYPE_MASK_TARGET_TO_PRIMARY   0x00000002
#define OFFER_TYPE_MASK_GOODS_TO_PRIMARY    0x00000004
#define OFFER_TYPE_MASK_PRIMARY_TO_GOODS    0x00000008

#define OFFER_TYPE_UNDEFINED                255


  struct offer_details_ex_with_hash : public offer_details_ex
  {
    crypto::hash h;
    mutable std::string nxt_offer; //in case of updated offer
  };

  typedef offer_details_ex_with_hash odeh;

  inline crypto::hash extract_id(const odeh& v)             { return v.h; }
  inline uint64_t     extrct_timestamp(const odeh& v)       { return v.timestamp; }
  inline uint64_t     extract_amount_primary(const odeh& v) { return v.amount_primary; }
  inline uint64_t     extract_amount_target(const odeh& v)  { return v.amount_target; }
  inline double       extract_rate(const odeh& v)           { return calculate_offer_rate(v); }
  inline size_t       extract_payment_types(const odeh& v)  { return v.payment_types.size(); }
  inline std::string  extract_contacts(const odeh& v)       { return v.contacts; }
  inline std::string  extract_location(const odeh& v) {

#ifndef MOBILE_WALLET_BUILD
    return currency::utf8_to_lower(v.location_country + v.location_city);
#else 
    return "UNSUPORTED";
#endif
  }
  inline std::string  extract_name(const odeh& v)           { 
#ifndef MOBILE_WALLET_BUILD
    return currency::utf8_to_lower(v.target); 
#else
    return "UNSUPORTED";
#endif
  }

  template<int sort_type>
  struct sort_id_to_type
  {};

#define TAG_NAME(tag_name) struct tag_name{};
  TAG_NAME(by_id);
  TAG_NAME(by_timestamp);
  TAG_NAME(by_amount_primary);
  TAG_NAME(by_amount_target);
  TAG_NAME(by_rate);
  TAG_NAME(by_payment_types);
  TAG_NAME(by_contacts);
  TAG_NAME(by_location);
  TAG_NAME(by_name);
#undef TAG_NAME

  template<>  struct sort_id_to_type<ORDER_BY_TIMESTAMP>      { typedef by_timestamp index_type; };
  template<>  struct sort_id_to_type<ORDER_BY_AMOUNT_PRIMARY> { typedef by_amount_primary index_type; };
  template<>  struct sort_id_to_type<ORDER_BY_AMOUNT_TARGET>  { typedef by_amount_target index_type; };
  template<>  struct sort_id_to_type<ORDER_BY_AMOUNT_RATE>    { typedef by_rate index_type; };
  template<>  struct sort_id_to_type<ORDER_BY_PAYMENT_TYPES>  { typedef by_payment_types index_type; };
  template<>  struct sort_id_to_type<ORDER_BY_CONTACTS>       { typedef by_contacts index_type; };
  template<>  struct sort_id_to_type<ORDER_BY_LOCATION>       { typedef by_location index_type; };
  template<>  struct sort_id_to_type<ORDER_BY_NAME>           { typedef by_name index_type; };
}
