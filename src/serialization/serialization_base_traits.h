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
#include <boost/type_traits.hpp>


template <class Archive, class T>
struct variant_serialization_traits {};
template <class T>
struct is_blob_type { typedef boost::false_type type; };
template <class T>
struct has_free_serializer { typedef boost::true_type type; };


#define BLOB_SERIALIZER(T) \
  template<> struct is_blob_type<T> { typedef boost::true_type type; }
#define FREE_SERIALIZER(T) \
  template<> struct has_free_serializer<T> { typedef boost::true_type type; }
#define VARIANT_TAG(A, T, Tg) \
  template <bool W> struct variant_serialization_traits<A<W>, T> { static inline typename A<W>::variant_tag_type get_tag() { return Tg; } }

