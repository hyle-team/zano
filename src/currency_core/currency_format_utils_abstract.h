// Copyright (c) 2018-2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <typeindex>
#include <unordered_set>
#include <unordered_map>
#include <list>
#include <memory>


#include "include_base_utils.h"
#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage_template_helper.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "currency_core/currency_basic.h"
#include "currency_protocol/blobdatatype.h"
#include "common/crypto_stream_operators.h"

namespace currency
{

  template<typename type_t>
  std::string print_kv_structure(const type_t& v)
  {
    return epee::serialization::store_t_to_json(v);
  }

  template<class t_type>
  std::string print_t_array(const std::vector<t_type>& vec)
  {
    std::stringstream ss;
    for (auto& v : vec)
      ss << v << " ";
    return ss.str();
  }

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  template<class t_array>
  struct array_hasher
  {
    std::size_t operator()(const t_array& val) const
    {
      return boost::hash_range(&val.data[0], &val.data[sizeof(val.data)]);
    }
  };

  //------------------------------------------------------------------------------------
  template<class t_pod_type, class result_type>
  result_type get_pod_checksum(const t_pod_type& bl)
  {
    const unsigned char* pbuf = reinterpret_cast<const unsigned char*>(&bl);
    result_type summ = 0;
    for (size_t i = 0; i != sizeof(t_pod_type)-1; i++)
      summ += pbuf[i];

    return summ;
  }

  template<typename object_t>
  bool parse_and_validate_object_from_blob(const blobdata& b_blob, object_t& b)
  {
    return t_unserializable_object_from_blob(b, b_blob);
  }
  //---------------------------------------------------------------
  template<typename specific_type_t, typename variant_t_container>
  bool have_type_in_variant_container(const variant_t_container& av)
  {
    for (auto& ai : av)
    {
      if (ai.type() == typeid(specific_type_t))
      {
        return true;
      }
    }
    return false;
  }
  //---------------------------------------------------------------
  template<typename specific_type_t, typename variant_t_container>
  size_t count_type_in_variant_container(const variant_t_container& av)
  {
    size_t result = 0;
    for (auto& ai : av)
    {
      if (ai.type() == typeid(specific_type_t))
        ++result;
    }
    return result;
  }  
  //---------------------------------------------------------------
  template<typename specific_type_t, typename variant_t_container>
  specific_type_t* get_type_in_variant_container(variant_t_container& av)
  {
    for (auto& ai : av)
    {
      if (ai.type() == typeid(specific_type_t))
      {
        return &boost::get<specific_type_t>(ai);
      }
    }
    return nullptr;
  }
  //---------------------------------------------------------------
  template<typename specific_type_t, typename variant_t_container>
  bool get_type_in_variant_container(variant_t_container& av, specific_type_t& a)
  {
    const specific_type_t* pa = get_type_in_variant_container<const specific_type_t>(av);
    if (pa)
    {
      a = *pa;
      return true;
    }
    return false;
  }
  //---------------------------------------------------------------
  //---------------------------------------------------------------
  template<typename specific_type_t, typename variant_t_container>
  specific_type_t& get_type_in_variant_container_by_ref(variant_t_container& av)
  {
    for (auto& ai : av)
    {
      if (ai.type() == typeid(specific_type_t))
      {
        return boost::get<specific_type_t>(ai);
      }
    }
    ASSERT_MES_AND_THROW("Object with type " << typeid(specific_type_t).name() << " was not found in a container");
  }
  //---------------------------------------------------------------
  // if cb returns true, it means "continue", false -- means "stop"
  template<typename specific_type_t, typename variant_container_t, typename callback_t>
  bool process_type_in_variant_container(const variant_container_t& av, callback_t&& cb, bool return_value_if_none_found = true)
  {
    bool found = false;
    for (auto& ai : av)
    {
      if (ai.type() == typeid(specific_type_t))
      {
        found = true;
        if (!cb(boost::get<specific_type_t>(ai)))
          return false;
      }
    }
    if (found)
      return true;
    return return_value_if_none_found;
  }
  //---------------------------------------------------------------
  // if cb returns false, stop immediately and return false
  template<typename specific_type_t, typename variant_container_t, typename callback_t>
  bool process_type_in_variant_container_and_make_sure_its_unique(const variant_container_t& av, callback_t&& cb, bool return_value_if_none_found = true)
  {
    bool found = false;
    for (auto& ai : av)
    {
      if (ai.type() == typeid(specific_type_t))
      {
        if (found)
          return false; // already have it, type in not unique
        found = true;
        if (!cb(boost::get<specific_type_t>(ai)))
          return false;
      }
    }
    if (found)
      return true;
    return return_value_if_none_found;
  }
  //---------------------------------------------------------------
  // callback should return true to continue iterating through the container
  template <typename A, typename B, typename container_t, typename callback_t>
  bool handle_2_alternative_types_in_variant_container(const container_t& container, callback_t cb)
  {
    bool found = false;
    for (auto& item : container)
    {
      if (item.type() == typeid(A))
      {
        found = true;
        if (!cb(boost::get<A>(item)))
          break;
      }
      else if (item.type() == typeid(B))
      {
        found = true;
        if (!cb(boost::get<B>(item)))
          break;
      }
    }
    return found;
  }
  //---------------------------------------------------------------
  inline
  bool get_key_image_from_txin_v(const txin_v& in_v, crypto::key_image& result) noexcept
  {
    try
    {
      if (in_v.type() == typeid(txin_to_key))
      {
        result = boost::get<txin_to_key>(in_v).k_image;
        return true;
      }
    
      if (in_v.type() == typeid(txin_htlc))
      {
        result = boost::get<txin_htlc>(in_v).k_image;
        return true;
      }

      if (in_v.type() == typeid(txin_zc_input))
      {
        result = boost::get<txin_zc_input>(in_v).k_image;
        return true;
      }
    }
    catch(...)
    {
      // should never go here, just precaution
    }

    return false;
  }
  //---------------------------------------------------------------
  inline
  const crypto::key_image& get_key_image_from_txin_v(const txin_v& in_v)
  {
    if (in_v.type() == typeid(txin_to_key))
      return boost::get<txin_to_key>(in_v).k_image;
    
    if (in_v.type() == typeid(txin_htlc))
      return boost::get<txin_htlc>(in_v).k_image;

    if (in_v.type() == typeid(txin_zc_input))
      return boost::get<txin_zc_input>(in_v).k_image;

    CHECK_AND_ASSERT_THROW_MES(false, "[get_key_image_from_txin_v] Wrong type: " << in_v.type().name());
  }
  //---------------------------------------------------------------
  inline
  const std::vector<currency::txout_ref_v>& get_key_offsets_from_txin_v(const txin_v& in_v)
  {
    if (in_v.type() == typeid(txin_to_key))
      return boost::get<txin_to_key>(in_v).key_offsets;
    
    if (in_v.type() == typeid(txin_htlc))
      return boost::get<txin_htlc>(in_v).key_offsets;

    if (in_v.type() == typeid(txin_zc_input))
      return boost::get<txin_zc_input>(in_v).key_offsets;

    CHECK_AND_ASSERT_THROW_MES(false, "[get_key_offsets_from_txin_v] Wrong type: " << in_v.type().name());
  }
  //---------------------------------------------------------------
  inline
  bool get_mix_attr_from_tx_out_v(const tx_out_v& out_v, uint8_t& result) noexcept
  {
    try
    {
      if (out_v.type() == typeid(tx_out_bare))
      {
        const tx_out_bare& ob = boost::get<tx_out_bare>(out_v);
        if (ob.target.type() == typeid(txout_to_key))
        {
          result = boost::get<txout_to_key>(ob.target).mix_attr;
          return true;
        }
      }
    
      if (out_v.type() == typeid(tx_out_zarcanum))
      {
        result = boost::get<tx_out_zarcanum>(out_v).mix_attr;
        return true;
      }
    }
    catch(...)
    {
      // should never go here, just precaution
    }

    return false;
  }
  //---------------------------------------------------------------
  //, txin_htlc, txin_zc_input
  inline bool compare_variant_by_types(const txin_multisig& left, const txin_multisig& right)
  {
    return (left.multisig_out_id < right.multisig_out_id);
  }
  //---------------------------------------------------------------
  inline bool compare_variant_by_types(const txin_gen& left, const txin_gen& right)
  {
    //actually this should never happen, should we leave it in case it happen in unit tests? @sowle 
    return (left.height < right.height);
  }
  //---------------------------------------------------------------
  template<typename type_with_kimage_t>
  bool compare_variant_by_types(const type_with_kimage_t& left, const type_with_kimage_t& right)
  {
    return (left.k_image < right.k_image);
  }
  //---------------------------------------------------------------
  template<typename t_type_left, typename t_type_right>
  bool compare_variant_by_types(const t_type_left& left, const t_type_right& right)
  {
    if (typeid(t_type_left) == typeid(t_type_right))
    {
      ASSERT_MES_AND_THROW("[compare_varian_by_types] Left and Right types matched type " << typeid(t_type_left).name());
    }
    typedef binary_archive<true> bin_archive;
    typedef variant_serialization_traits<bin_archive, t_type_left> traits_left;
    typedef variant_serialization_traits<bin_archive, t_type_right> traits_right;
    return (traits_left::get_tag() < traits_right::get_tag());
  }
  //---------------------------------------------------------------
  template<typename t_type_left>
  struct right_visitor : public boost::static_visitor<bool>
  {
    const t_type_left& m_rleft;

    right_visitor(const t_type_left& left) : m_rleft(left)
    {}
    
    template<typename t_type_right>
    bool operator()(const t_type_right& right)const
    {
      return compare_variant_by_types(m_rleft, right);
    }
  };

  struct left_visitor : public boost::static_visitor<bool>
  {
    const txin_v& m_rright;

    left_visitor(const txin_v& right) : m_rright(right)
    {}
    template<typename t_type_left>
    bool operator()(const t_type_left& left)const
    {
      return boost::apply_visitor(right_visitor<t_type_left>(left), m_rright);
    }
  };
  //---------------------------------------------------------------
  inline bool less_txin_v(const txin_v& left, const txin_v& right)
  {
    //predefined type hierarchy based on it's tags defined in currency_basic.h, call compare_variant_by_types via 2-level visitor
    return boost::apply_visitor(left_visitor(right), left);
  }
  //---------------------------------------------------------------
  template<typename variant_container_t>
  bool check_allowed_types_in_variant_container(const variant_container_t& container, const std::unordered_set<std::type_index>& allowed_types, bool elements_must_be_unique = true)
  {
    for (auto it = container.begin(); it != container.end(); ++it)
    {
      if (allowed_types.count(std::type_index(it->type())) == 0)
        return false;

      if (elements_must_be_unique)
      {
        for (auto jt = it + 1; jt != container.end(); ++jt)
          if (it->type().hash_code() == jt->type().hash_code())
            return false;
      }
    }
    return true;
  }
  //---------------------------------------------------------------
  template<typename variant_container_t>
  bool check_allowed_types_in_variant_container(const variant_container_t& container, const variant_container_t& allowed_types_examples, bool elements_must_be_unique = true)
  {
    std::unordered_set<std::type_index> allowed_types;
    for (auto& el : allowed_types_examples)
      if (!allowed_types.insert(std::type_index(el.type())).second)
        return false; // invalid allowed_types_examples container

    return check_allowed_types_in_variant_container(container, allowed_types, elements_must_be_unique);
  }
  //---------------------------------------------------------------
  template<typename variant_container_t>
  std::string stringize_types_in_variant_container(const variant_container_t& container)
  {
    std::string result;
    for (auto it = container.begin(); it != container.end(); ++it)
      result = (result + it->type().name()) + (it + 1 != container.end() ? ", " : "");
    return result;
  }
  //---------------------------------------------------------------
  inline 
    void get_blob_hash(const blobdata& blob, crypto::hash& res)
  {
    cn_fast_hash(blob.data(), blob.size(), res);
  }
  //---------------------------------------------------------------
  inline 
    crypto::hash get_blob_hash(const blobdata& blob)
  {
    crypto::hash h = null_hash;
    get_blob_hash(blob, h);
    return h;
  }

  template<class t_object>
  bool get_object_hash(const t_object& o, crypto::hash& res)
  {
    get_blob_hash(t_serializable_object_to_blob(o), res);
    return true;
  }
  //---------------------------------------------------------------
  template<class t_object>
  crypto::hash get_object_hash(const t_object& o)
  {
    crypto::hash h;
    get_object_hash(o, h);
    return h;
  }
  //---------------------------------------------------------------

  template<class t_object>
  size_t get_object_blobsize(const t_object& o)
  {
    blobdata b = t_serializable_object_to_blob(o);
    return b.size();
  }
  //---------------------------------------------------------------
  template<class t_object>
  bool get_object_hash(const t_object& o, crypto::hash& res, uint64_t& blob_size)
  {
    blobdata bl = t_serializable_object_to_blob(o);
    blob_size = bl.size();
    get_blob_hash(bl, res);
    return true;
  }
  //---------------------------------------------------------------
  template <typename T>
  std::string obj_to_json_str(const T& obj)
  {
    std::stringstream ss;
    json_archive<true> ar(ss, true);
    bool r = ::serialization::serialize(ar, const_cast<T&>(obj));
    CHECK_AND_ASSERT_MES(r, "", "obj_to_json_str failed: serialization::serialize returned false");
    return ss.str();
  }
  //---------------------------------------------------------------

  //---------------------------------------------------------------
  size_t get_object_blobsize(const transaction& t);
  size_t get_object_blobsize(const transaction& t, uint64_t prefix_blob_size);


  inline
  void put_t_to_buff(std::string& buff)
  {}

  template <typename T, typename... Types>
  void put_t_to_buff(std::string& buff, const T& var1, Types&... var2)
  {
    static_assert(std::is_pod<T>::value, "T must be a POD type.");
    buff.append((const char*)&var1, sizeof(var1));
    put_t_to_buff(buff, var2...);
  }

  template <typename... Types>
  crypto::hash get_hash_from_POD_objects(Types&... var1)
  {
    std::string buff;
    put_t_to_buff(buff, var1...);
    return crypto::cn_fast_hash(buff.data(), buff.size());
  }


#define CHECKED_GET_SPECIFIC_VARIANT(variant_var, specific_type, variable_name, fail_return_val) \
  CHECK_AND_ASSERT_MES(variant_var.type() == typeid(specific_type), fail_return_val, "wrong variant type: " << variant_var.type().name() << ", expected " << typeid(specific_type).name()); \
  specific_type& variable_name = boost::get<specific_type>(variant_var);

} // namespace currency



