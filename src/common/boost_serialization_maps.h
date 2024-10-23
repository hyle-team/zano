// Copyright (c) 2014-2022 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <type_traits>
#ifndef DISABLE_PFR_SERIALIZATION_SELFCHECK
  #include <boost/pfr.hpp>
#endif
#define BEGIN_BOOST_SERIALIZATION()     template <class t_archive> void serialize(t_archive &_arch, const unsigned int ver) {

template<size_t A, size_t B> struct TAssertEquality {
  static_assert(A == B, "Serialization map is not updated, sizeof() missmatch");
  static constexpr bool _cResult = (A == B);
};

//experemental feature: self-validated serialization map, needed to not forget add new members to serialization maps
#define BEGIN_BOOST_SERIALIZATION_SV(sz)  BEGIN_BOOST_SERIALIZATION()  \
              static constexpr bool _cIsEqual = TAssertEquality<sz, sizeof(*this)>::_cResult;



#define BOOST_SERIALIZE(x)    _arch & x;
#define BOOST_SERIALIZE_BASE_CLASS(class_type)    _arch & static_cast<class_type&>(*this);

#define BOOST_END_VERSION_UNDER(x)                            \
  if(ver < x ) {return;}

#define END_BOOST_SERIALIZATION()       }



/**********************************************************************************************************************************
  This serialization closing macro adds self-validation by checking the total number of fields in the structure using boost::pfr.

  Note: "num_fields" does NOT represent the number of fields included in the serialization. Instead, it indicates the total number
  of fields in the structure, some of which might not be included in the serialization for valid reasons. If someone adds new 
  fields to the structure but forgets to update the serialization map, the compilation will fail. Any update to "num_fields" must
  be accompanied by a thorough review of the serialization map to ensure no fields are omitted.
**********************************************************************************************************************************/
#ifndef DISABLE_PFR_SERIALIZATION_SELFCHECK
  #define END_BOOST_SERIALIZATION_TOTAL_FIELDS(num_fields)  static_assert(num_fields == boost::pfr::tuple_size<std::remove_reference<decltype(*this)>::type>::value, "Unexpected number of fields!"); }
#else
  #define END_BOOST_SERIALIZATION_TOTAL_FIELDS(num_fields) END_BOOST_SERIALIZATION()
#endif

#define BOOST_SERIALIZATION_CURRENT_ARCHIVE_VER(current_version) static const unsigned int current_boost_version_serialization_version = current_version;

#define LOOP_BACK_BOOST_SERIALIZATION_VERSION(type) BOOST_CLASS_VERSION(type, type::current_boost_version_serialization_version);



template<bool IsSaving, typename destination_t>
struct boost_transition_t {};

template<typename destination_t>
struct boost_transition_t<true, destination_t>
{
  template <typename archive, typename origin_type>
  static void chain_serialize(archive& ar, const origin_type& origin_tx)
  {
    destination_t dst_tx = AUTO_VAL_INIT(dst_tx);
    transition_convert(origin_tx, dst_tx);
    ar & dst_tx;
  }
};

template<typename destination_t>
struct boost_transition_t<false, destination_t>
{
  template <typename archive, typename origin_type>
  static void chain_serialize(archive& ar, origin_type& origin_tx)
  {
    // TODO: consider using move semantic for temporary 'dst_tx'
    destination_t dst_tx = AUTO_VAL_INIT(dst_tx);
    ar & dst_tx;    
    transition_convert(dst_tx, origin_tx);
  }
};


#define BOOST_CHAIN_TRANSITION_VER(obj_version, old_type)   if (obj_version == ver)  {boost_transition_t<t_archive::is_saving::value, old_type>::chain_serialize(_arch, *this);return;}

#define BOOST_CHAIN_TRANSITION_IF_COND_TRUE(condition, old_type)   if (condition)  {boost_transition_t<t_archive::is_saving::value, old_type>::chain_serialize(_arch, *this);return;}

/*
  example of use: 

  struct tx_extra_info 
  {
    crypto::public_key m_tx_pub_key;
    extra_alias_entry m_alias;
    std::string m_user_data_blob;
    extra_attachment_info m_attachment_info;

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(m_tx_pub_key)
      BOOST_SERIALIZE(m_alias)
      if(ver < xxx) return;
      BOOST_SERIALIZE(m_user_data_blob)
      BOOST_SERIALIZE(m_attachment_info)
    END_BOOST_SERIALIZATION()
  };
*/
