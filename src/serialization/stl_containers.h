// Copyright (c) 2014-2017 The The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <set>
#include <vector>
#include <list>

//#include "serialization.h"
template <template <bool> class Archive, class T>
bool do_serialize(Archive<false> &ar, std::vector<T> &v);
template <template <bool> class Archive, class T>
bool do_serialize(Archive<true> &ar, std::vector<T> &v);

namespace serialization
{
  namespace detail
  {
    template <typename Archive, class T>
    bool serialize_container_element(Archive& ar, T& e)
    {
      return ::do_serialize(ar, e);
    }

    template <typename Archive>
    bool serialize_container_element(Archive& ar, uint64_t& e)
    {
      ar.serialize_varint(e);
      return true;
    }
  }
}

template <template <bool> class Archive, class T>
bool do_serialize(Archive<false> &ar, std::vector<T> &v)
{
  size_t cnt;
  ar.begin_array(cnt);
  if (!ar.stream().good())
    return false;
  v.clear();

  // very basic sanity check
  if (ar.remaining_bytes() < cnt) {
    ar.stream().setstate(std::ios::failbit);
    return false;
  }

  v.reserve(cnt);
  for (size_t i = 0; i < cnt; i++) {
    if (i > 0)
      ar.delimit_array();
    
    T t = T();
    if (!::serialization::detail::serialize_container_element(ar, t))
      return false;
    if (!ar.stream().good())
      return false;
    v.push_back(t);
  }
  ar.end_array();
  return true;
}

template <template <bool> class Archive, class T>
bool do_serialize(Archive<true> &ar, std::vector<T> &v)
{
  size_t cnt = v.size();
  ar.begin_array(cnt);
  for (size_t i = 0; i < cnt; i++) {
    if (!ar.stream().good())
      return false;
    if (i > 0)
      ar.delimit_array();
    if(!::serialization::detail::serialize_container_element(ar, v[i]))
      return false;
    if (!ar.stream().good())
      return false;
  }
  ar.end_array();
  return true;
}

template <template <bool> class Archive, class T>
bool do_serialize(Archive<false> &ar, std::list<T> &v)
{
  size_t cnt;
  ar.begin_array(cnt);
  if (!ar.stream().good())
    return false;
  v.clear();

  // very basic sanity check
  if (ar.remaining_bytes() < cnt) {
    ar.stream().setstate(std::ios::failbit);
    return false;
  }

  for (size_t i = 0; i < cnt; i++) {
    if (i > 0)
      ar.delimit_array();
    v.push_back(T());
    if (!::serialization::detail::serialize_container_element(ar, v.back()))
      return false;
    if (!ar.stream().good())
      return false;
  }
  ar.end_array();
  return true;
}

template <template <bool> class Archive, class T>
bool do_serialize(Archive<true> &ar, std::list<T> &v)
{
  size_t cnt = v.size();
  ar.begin_array(cnt);
  for (auto it = v.begin(); it != v.end(); it++)
  {
    if (!ar.stream().good())
      return false;
    if (it != v.begin())
      ar.delimit_array();
    if (!::serialization::detail::serialize_container_element(ar, *it))
      return false;
    if (!ar.stream().good())
      return false;
  }
  ar.end_array();
  return true;
}

template <template <bool> class Archive, class T>
bool do_serialize(Archive<false> &ar, std::set<T> &v)
{
  size_t cnt;
  ar.begin_array(cnt);
  if (!ar.stream().good())
    return false;
  v.clear();

  // very basic sanity check
  if (ar.remaining_bytes() < cnt) {
    ar.stream().setstate(std::ios::failbit);
    return false;
  }

  for (size_t i = 0; i < cnt; i++) {
    if (i > 0)
      ar.delimit_array();
    T t = T();
    
    if (!::serialization::detail::serialize_container_element(ar, t))
      return false;
    if (!ar.stream().good())
      return false;
    v.insert(t);
  }
  ar.end_array();
  return true;
}

template <template <bool> class Archive, class T>
bool do_serialize(Archive<true> &ar, std::set<T> &v)
{
  size_t cnt = v.size();
  ar.begin_array(cnt);
  for (auto it = v.begin(); it != v.end(); it++)
  {
    if (!ar.stream().good())
      return false;
    if (it != v.begin())
      ar.delimit_array();
    //TODO: refactoring needed to remove const_cast 
    if (!::serialization::detail::serialize_container_element(ar, const_cast<std::string&>(*it)))
      return false;
    if (!ar.stream().good())
      return false;
  }
  ar.end_array();
  return true;
}

template <template <bool> class Archive, class A, class B>
bool do_serialize(Archive<false> &ar, std::pair<A, B> &v)
{
  if (!::serialization::detail::serialize_container_element(ar, v.first))
    return false;
  if (!ar.stream().good())
    return false;

  if (!::serialization::detail::serialize_container_element(ar, v.second))
    return false;
  if (!ar.stream().good())
    return false;

  return true;
}

template <template <bool> class Archive, class A, class B>
bool do_serialize(Archive<true> &ar, std::pair<A, B> &v)
{
  if (!::serialization::detail::serialize_container_element(ar, v.first))
    return false;
  if (!ar.stream().good())
    return false;

  if (!::serialization::detail::serialize_container_element(ar, v.second))
    return false;
  if (!ar.stream().good())
    return false;

  return true;
}

// we do specialization for std::vector<bool> cz it 

// template <template <bool> class Archive>
// bool do_serialize(Archive<false> &ar, std::vector<bool> &v)
// {
//   size_t cnt;
//   ar.begin_array(cnt);
//   if (!ar.stream().good())
//     return false;
//   v.clear();
// 
//   // very basic sanity check
//   if (ar.remaining_bytes() < cnt) {
//     ar.stream().setstate(std::ios::failbit);
//     return false;
//   }
// 
//   v.reserve(cnt);
//   for (size_t i = 0; i < cnt; i++) {
//     if (i > 0)
//       ar.delimit_array();
//     v.resize(i + 1);
//     if (!::serialization::detail::serialize_container_element(ar, v[i]))
//       return false;
//     if (!ar.stream().good())
//       return false;
//   }
//   ar.end_array();
//   return true;
// }

template <template <bool> class Archive>
bool do_serialize(Archive<true> &ar, std::vector<bool> &v)
{
  size_t cnt = v.size();
  ar.begin_array(cnt);
  for (size_t i = 0; i < cnt; i++) {
    if (!ar.stream().good())
      return false;
    if (i > 0)
      ar.delimit_array();

    bool local_b = static_cast<bool>(v[i]);
    if (!::serialization::detail::serialize_container_element(ar, local_b))
      return false;
    if (!ar.stream().good())
      return false;
  }
  ar.end_array();
  return true;
}