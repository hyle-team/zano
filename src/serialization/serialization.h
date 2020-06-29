// Copyright (c) 2014-2018 The The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/* serialization.h
 *
 * Simple templated serialization API */

#pragma once
#include <vector>
#include <string>
#include <boost/type_traits/is_integral.hpp>

#include "misc_log_ex.h"
#include "binary_archive.h"

template <class T>
struct is_blob_type { typedef boost::false_type type; };
template <class T>
struct has_free_serializer { typedef boost::true_type type; };

template <class Archive, class T>
struct serializer
{
  static bool serialize(Archive &ar, T &v) {
    return serialize(ar, v, typename boost::is_integral<T>::type(), typename is_blob_type<T>::type());
  }
  static bool serialize(Archive &ar, T &v, boost::false_type, boost::true_type) {
    ar.serialize_blob(&v, sizeof(v));
    return true;
  }
  static bool serialize(Archive &ar, T &v, boost::true_type, boost::false_type) {
    ar.serialize_int(v);
    return true;
  }
  static bool serialize(Archive &ar, T &v, boost::false_type, boost::false_type) {
    //serialize_custom(ar, v, typename has_free_serializer<T>::type());
    return v.do_serialize(ar);
  }
  static void serialize_custom(Archive &ar, T &v, boost::true_type) {
  }
};

template <class Archive, class T>
inline bool do_serialize(Archive &ar, T &v)
{
  return ::serializer<Archive, T>::serialize(ar, v);
}

#if ( !defined(__GNUC__) && !(defined(_MSC_VER) && (_MSC_VER >= 1900)))
#ifndef constexpr
#define constexpr
#endif
#endif

#define BLOB_SERIALIZER(T) \
  template<> struct is_blob_type<T> { typedef boost::true_type type; }
#define FREE_SERIALIZER(T) \
  template<> struct has_free_serializer<T> { typedef boost::true_type type; }
#define VARIANT_TAG(A, T, Tg) \
  template <bool W> struct variant_serialization_traits<A<W>, T> { static inline typename A<W>::variant_tag_type get_tag() { return Tg; } }
#define BEGIN_SERIALIZE() \
  template <bool W, template <bool> class Archive> bool do_serialize(Archive<W> &ar) {
#define BEGIN_SERIALIZE_OBJECT() \
  template <bool W, template <bool> class Archive> bool do_serialize(Archive<W> &ar) { ar.begin_object(); bool r = do_serialize_object(ar); ar.end_object(); return r; } \
  template <bool W, template <bool> class Archive> bool do_serialize_object(Archive<W> &ar){
#define PREPARE_CUSTOM_VECTOR_SERIALIZATION(size, vec) ::serialization::detail::prepare_custom_vector_serialization(size, vec, typename Archive<W>::is_saving())

#define END_SERIALIZE() return true;}


#define VALUE(f) \
do { \
  ar.tag(#f); \
  bool r = ::do_serialize(ar, f); \
  if (!r || !ar.stream().good()) return false; \
} while (0);
#define FIELD_N(t, f) \
do { \
  ar.tag(t); \
  bool r = ::do_serialize(ar, f); \
  if (!r || !ar.stream().good()) return false; \
} while (0);
#define FIELDS(f) \
  bool r = ::do_serialize(ar, f); \
if (!r || !ar.stream().good()) return false;
#define FIELD(f) \
do { \
  ar.tag(#f); \
  bool r = ::do_serialize(ar, f); \
  if (!r || !ar.stream().good()) return false; \
} while (0);
#define VARINT_FIELD(f) \
do { \
  ar.tag(#f); \
  ar.serialize_varint(f); \
  if (!ar.stream().good()) return false; \
} while (0);

#define DEFINE_SERIALIZATION_VERSION(v) inline static uint32_t get_serialization_version() { return v; }


#define VERSION_ENTRY(f) \
do { \
  ar.tag(#f); \
  if (ar.is_saving_arch())  \
    f = this->get_serialization_version(); \
  bool r = ::do_serialize(ar, f); \
  if (!r || !ar.stream().good()) return false; \
} while (0);

template<typename first_type, typename second_type>
class serializable_pair : public std::pair<first_type, second_type>
{
  typedef std::pair<first_type, second_type> base;
public:
  serializable_pair()
  {}
  serializable_pair(const first_type& a, const second_type& b) :std::pair<first_type, second_type>(a, b)
  {}
  serializable_pair(const serializable_pair& sp) :std::pair<first_type, second_type>(sp.first, sp.second)
  {}

  BEGIN_SERIALIZE_OBJECT()
    FIELD(base::first)
    FIELD(base::second)
  END_SERIALIZE()
};

template<typename first_type, typename second_type>
serializable_pair<first_type, second_type> make_serializable_pair(const first_type& first_value, const second_type& second_value)
{
  return serializable_pair<first_type, second_type>(first_value, second_value);
}


namespace serialization {
  namespace detail
  {
    template <typename T>
    void prepare_custom_vector_serialization(size_t size, std::vector<T>& vec, const boost::mpl::bool_<true>& /*is_saving*/)
    {
    }

    template <typename T>
    void prepare_custom_vector_serialization(size_t size, std::vector<T>& vec, const boost::mpl::bool_<false>& /*is_saving*/)
    {
      vec.resize(size);
    }

    template<class Stream>
    bool do_check_stream_state(Stream& s, boost::mpl::bool_<true>)
    {
      return s.good();
    }

    template<class Stream>
    bool do_check_stream_state(Stream& s, boost::mpl::bool_<false>)
    {
      bool result = false;
      if (s.good())
      {
        std::ios_base::iostate state = s.rdstate();
        result = EOF == s.peek();
        s.clear(state);
      }
      return result;
    }
  }

  template<class Archive>
  bool check_stream_state(Archive& ar)
  {
    return detail::do_check_stream_state(ar.stream(), typename Archive::is_saving());
  }

  template <class Archive, class T>
  inline bool serialize(Archive &ar, T &v)
  {
    bool r = do_serialize(ar, v);
    return r && check_stream_state(ar);
  }
}

//---------------------------------------------------------------
template<class t_object>
bool t_serializable_object_to_blob(const t_object& to, std::string& b_blob)
{
  std::stringstream ss;
  binary_archive<true> ba(ss);
  bool r = ::serialization::serialize(ba, const_cast<t_object&>(to));
  b_blob = ss.str();
  return r;
}
//---------------------------------------------------------------
template<class t_object>
bool t_unserializable_object_from_blob(t_object& to, const std::string& blob)
{
  std::stringstream ss;
  ss << blob;
  binary_archive<false> ba(ss);
  bool r = ::serialization::serialize(ba, to);
  CHECK_AND_ASSERT_MES(r, false, "Failed to parse block from blob");
  return true;
}
//---------------------------------------------------------------
template<class t_object>
std::string t_serializable_object_to_blob(const t_object& to)
{
  std::string b;
  t_serializable_object_to_blob(to, b);
  return b;
}

#include "serialize_basic_types.h"
#include "string.h"
#include "multiprecision.h"
#include "stl_containers.h"

