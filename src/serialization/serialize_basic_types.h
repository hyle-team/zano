// Copyright (c) 2014-2017 The The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <memory>
#include "serialization.h"

#define DEFINE_POD_SERIALIZATION(type_to_define)    \
  template <template <bool> class Archive> \
  inline bool do_serialize(Archive<false>& ar, type_to_define& b)  \
{\
  ar.serialize_blob(&b, sizeof(b)); \
  return true; \
} \
template <template <bool> class Archive> \
  inline bool do_serialize(Archive<true>& ar, type_to_define& b) \
{ \
  ar.serialize_blob(&b, sizeof(b));  return true; \
} 

// Strict bool loading (budgeted binary_archive<false> only): the byte is read
// as uint8_t and anything but 0/1, or a truncated read, fails the stream
// before a bool is formed or assigned. Other archives never take this path.
template <class Archive>
inline bool serialization_strict_load_bool(Archive&, bool&) { return false; }
inline bool serialization_strict_load_bool(binary_archive<false>& ar, bool& b)
{
  static_assert(sizeof(bool) == 1, "the default bool encoding is one byte");
  const int c = ar.stream().get();
  if (c == std::char_traits<char>::eof() || static_cast<unsigned char>(c) > 1)
  {
    ar.stream().setstate(std::ios::failbit);
    return false;
  }
  b = static_cast<unsigned char>(c) == 1;
  return true;
}

// Same as DEFINE_POD_SERIALIZATION(bool) except for the strict load above;
// default loading and all saving are unchanged.
template <template <bool> class Archive>
inline bool do_serialize(Archive<false>& ar, bool& b)
{
  if (serialization_budget_strict(ar))
    return serialization_strict_load_bool(ar, b);
  ar.serialize_blob(&b, sizeof(b));
  return true;
}
template <template <bool> class Archive>
inline bool do_serialize(Archive<true>& ar, bool& b)
{
  ar.serialize_blob(&b, sizeof(b));  return true;
}


