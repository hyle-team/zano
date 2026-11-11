// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "currency_basic.h"
#include "currency_format_utils.h"
#include "storages/portable_storage_template_helper.h"
#include "zlib_helper.h"

namespace bc_services
{
  template<class t_attachment_type_t>
  bool pack_attachment_as_gzipped_json(const t_attachment_type_t& at, currency::tx_service_attachment& att)
  {
    //TODO: make json without spaces and line endings here to reduce space 
    epee::serialization::store_t_to_json(at, att.body);
    att.flags |= TX_SERVICE_ATTACHMENT_DEFLATE_BODY;
//     if (!epee::zlib_helper::pack(json_buff, att.body))
//     {
//       LOG_ERROR("Filed to pack tx_service_attachment as gzipped json, service id: " << att.service_id << " instruction: " << att.instruction << " flags: " << att.flags);
//       return false;
//     }
    return true;
  }

  template<class t_attachment_type_t>
  bool unpack_attachment_as_gzipped_json(t_attachment_type_t& at, const currency::tx_service_attachment& att)
  {
    std::string json_buff;
    CHECK_AND_ASSERT_MES(att.flags&TX_SERVICE_ATTACHMENT_DEFLATE_BODY, false, "unpack_attachment_as_gzipped_json didn't find TX_SERVICE_ATTACHMENT_DEFLATE_BODY in flags");
    if (!epee::zlib_helper::unpack(att.body, json_buff))
    {
      LOG_ERROR("Filed to unpack tx_service_attachment as gzipped json, service id: " << att.service_id << " instruction: " << att.instruction << " flags: " << att.flags);
      return false;
    }
    return epee::serialization::load_t_from_json(at, json_buff);
  }

  template<class t_attachment_type_t>
  bool unpack_attachment_as_gzipped_bin(t_attachment_type_t& at, const currency::tx_service_attachment& att)
  {
    std::string bin_buff;
    CHECK_AND_ASSERT_MES(att.flags&TX_SERVICE_ATTACHMENT_DEFLATE_BODY, false, "unpack_attachment_as_gzipped_bin didn't find TX_SERVICE_ATTACHMENT_DEFLATE_BODY in flags");
    if (!epee::zlib_helper::unpack(att.body, bin_buff))
    {
      LOG_ERROR("Filed to unpack tx_service_attachment as gzipped bin, service id: " << att.service_id << " instruction: " << att.instruction << " flags: " << att.flags);
      return false;
    }
    return t_unserializable_object_from_blob(at, bin_buff);
  }

  template<class t_attachment_type_t>
  bool pack_attachment_as_gzipped_bin(const t_attachment_type_t& at, currency::tx_service_attachment& att)
  {
    //TODO: make json without spaces and line endings here to reduce space 
    ::t_serializable_object_to_blob(at, att.body);
    att.flags |= TX_SERVICE_ATTACHMENT_DEFLATE_BODY;
//     if (!epee::zlib_helper::pack(bin_buff, att.body))
//     {
//       LOG_ERROR("Filed to pack tx_service_attachment as gzipped bin, service id: " << att.service_id << " instruction: " << att.instruction << " flags: " << att.flags);
//       return false;
//     }
    return true;
  }

  template<class t_attachment_type_container_t>
  bool get_service_attachment_by_id_unpack(const t_attachment_type_container_t& tx_items, const std::string& id, const std::string& instruction, currency::tx_service_attachment& res)
  {
    bool r = get_first_service_attachment_by_id(tx_items, id, instruction, res);
    if (r && res.flags&TX_SERVICE_ATTACHMENT_DEFLATE_BODY)
    {
      r = epee::zlib_helper::unpack(res.body);
      CHECK_AND_ASSERT_MES(r, false, "Failed to unpack in get_service_attachment_by_id_unpack");
    }
    return r;
  }

  inline bool get_first_service_attachment_by_id(const currency::transaction& tx, const std::string& id, const std::string& instruction, currency::tx_service_attachment& res)
  {
    if (get_first_service_attachment_by_id(tx.extra, id, instruction, res))
      return true;
    return get_first_service_attachment_by_id(tx.attachment, id, instruction, res);
  }

  template<class t_attachment_type_container_t, class t_result_type>
  bool get_service_attachment_json_by_id(const t_attachment_type_container_t& tx_items, const std::string& id, const std::string& instruction, t_result_type& res)
  {
    currency::tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
    if (!get_first_service_attachment_by_id(tx_items, id, instruction, tsa))
      return false;

    return epee::serialization::load_t_from_json(res, tsa.body);
  }

  template<class t_attachment_type_container_t, class t_result_type>
  bool get_service_attachment_gzipped_json_by_id(const t_attachment_type_container_t& tx_items, const std::string& id, const std::string& instruction, t_result_type& res)
  {
    currency::tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
    if (!get_first_service_attachment_by_id(tx_items, id, instruction, tsa))
      return false;

    return unpack_attachment_as_gzipped_json(res, tsa);
  }

  template<class t_attachment_type_container_t, class t_result_type>
  bool get_service_attachment_gzipped_bin_by_id(const t_attachment_type_container_t& tx_items, const std::string& id, const std::string& instruction, t_result_type& res)
  {

    currency::tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
    if (!get_first_service_attachment_by_id(tx_items, id, instruction, tsa))
      return false;

    return unpack_attachment_as_gzipped_bin(res, tsa);
  }
  template<class t_attachment_type_container_t, class t_result_type>
  bool get_service_attachment_bin_by_id(const t_attachment_type_container_t& tx_items, const std::string& id, const std::string& instruction, t_result_type& res)
  {

    currency::tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
    if (!get_first_service_attachment_by_id(tx_items, id, instruction, tsa))
      return false;


    return ::t_unserializable_object_from_blob(res, tsa.body);
  }
}