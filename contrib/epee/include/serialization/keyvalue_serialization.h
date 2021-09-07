// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 

#pragma once

#include <boost/utility/value_init.hpp>
#include <boost/foreach.hpp>
#include "misc_log_ex.h"
#include "keyvalue_helpers.h"
#include "keyvalue_serialization_overloads.h"
namespace epee
{
  /************************************************************************/
  /* Serialize map declarations                                           */
  /************************************************************************/
#define BEGIN_KV_SERIALIZE_MAP() \
public: \
  template<class t_storage> \
  bool store(t_storage& st, typename t_storage::hsection hparent_section = nullptr) const\
  {\
  return serialize_map<true>(*this, st, hparent_section); \
}\
  template<class t_storage> \
  bool _load(t_storage& stg, typename t_storage::hsection hparent_section = nullptr)\
  {\
  return serialize_map<false>(*this, stg, hparent_section); \
}\
  template<class t_storage> \
  bool load(t_storage& stg, typename t_storage::hsection hparent_section = nullptr)\
  {\
  try{\
  return serialize_map<false>(*this, stg, hparent_section); \
}\
  catch (const std::exception& err) \
  { \
  (void)(err); \
  LOG_ERROR("Exception on unserializing: " << err.what()); \
  return false; \
  }\
}\
  template<bool is_store, class this_type, class t_storage> \
  static bool serialize_map(this_type& this_ref, t_storage& stg, typename t_storage::hsection hparent_section) \
{

#define KV_SERIALIZE_N(varialble, val_name) \
  epee::serialization::selector<is_store>::serialize(this_ref.varialble, stg, hparent_section, val_name);

#define KV_SERIALIZE_CUSTOM_N(varialble, stored_type, from_v_to_stored, from_stored_to_v, val_name) \
  epee::serialization::selector<is_store>::template serialize_custom<stored_type>(this_ref.varialble, stg, hparent_section, val_name, from_v_to_stored, from_stored_to_v);

#define KV_SERIALIZE_EPHEMERAL_N(stored_type, from_v_to_stored, val_name) \
  epee::serialization::selector<is_store>::template serialize_ephemeral<stored_type>(this_ref, stg, hparent_section, val_name, from_v_to_stored);


#define KV_SERIALIZE_POD_AS_HEX_STRING_N(varialble, val_name) \
	KV_SERIALIZE_CUSTOM_N(varialble, std::string, epee::transform_t_pod_to_str<decltype(varialble)>, epee::transform_str_to_t_pod<decltype(varialble)>, val_name)

#define KV_SERIALIZE_BLOB_AS_HEX_STRING_N(varialble, val_name) \
	KV_SERIALIZE_CUSTOM_N(varialble, std::string, epee::transform_binbuf_to_hexstr, epee::transform_hexstr_to_binbuff, val_name)


#define KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE_N(varialble, val_name) \
  epee::serialization::selector<is_store>::serialize_t_val_as_blob(this_ref.varialble, stg, hparent_section, val_name); 

#define KV_SERIALIZE_VAL_POD_AS_BLOB_N(varialble, val_name) \
  static_assert(std::is_pod<decltype(this_ref.varialble)>::value, "t_type must be a POD type."); \
  KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE_N(varialble, val_name)

#define KV_SERIALIZE_CONTAINER_POD_AS_BLOB_N(varialble, val_name) \
  epee::serialization::selector<is_store>::serialize_stl_container_pod_val_as_blob(this_ref.varialble, stg, hparent_section, val_name);

#define END_KV_SERIALIZE_MAP() return true;}

#define KV_SERIALIZE(varialble)                           KV_SERIALIZE_N(varialble, #varialble)
#define KV_SERIALIZE_VAL_POD_AS_BLOB(varialble)           KV_SERIALIZE_VAL_POD_AS_BLOB_N(varialble, #varialble)
#define KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(varialble)     KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE_N(varialble, #varialble) //skip is_pod compile time check
#define KV_SERIALIZE_CONTAINER_POD_AS_BLOB(varialble)     KV_SERIALIZE_CONTAINER_POD_AS_BLOB_N(varialble, #varialble)
#define KV_SERIALIZE_CUSTOM(varialble, stored_type, from_v_to_stored, from_stored_to_v)    KV_SERIALIZE_CUSTOM_N(varialble, stored_type, from_v_to_stored, from_stored_to_v, #varialble)
#define KV_SERIALIZE_POD_AS_HEX_STRING(varialble)         KV_SERIALIZE_POD_AS_HEX_STRING_N(varialble, #varialble)
#define KV_SERIALIZE_BLOB_AS_HEX_STRING(varialble)         KV_SERIALIZE_BLOB_AS_HEX_STRING_N(varialble, #varialble)
  


#define KV_CHAIN_MAP(variable_obj) epee::namespace_accessor<decltype(this_ref.variable_obj)>::template serialize_map<is_store>(this_ref.variable_obj, stg, hparent_section);
#define KV_CHAIN_BASE(base_type) base_type::serialize_map<is_store>(static_cast<base_type&>(const_cast<typename std::remove_const<this_type>::type&>(this_ref)), stg, hparent_section);
	


  template<typename t_uint>
  struct uint_mask_selector
  {
    template<t_uint mask>
    inline static bool get_value_of_flag_by_mask(const t_uint& given_flags)
    { 
      return (given_flags&mask) == 0 ? false : true;
    }
  }; 


#define KV_SERIALIZE_EPHEMERAL_BOOL_FROM_FLAG_N(var, mask, val_name) \
  KV_SERIALIZE_EPHEMERAL_N(bool, uint_mask_selector<decltype(var)>::get_value_of_flag_by_mask<mask>, val_name)




}




