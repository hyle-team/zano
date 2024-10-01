// Copyright (c) 2018-2024 Zano Project
// Copyright (c) 2014-2017 The The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/serialization/optional.hpp>

// boost::optional
template <template <bool> class Archive, class T>
bool do_serialize(Archive<false> &ar, boost::optional<T> &o)
{
  //reading flag
  bool is_none = false;
  if (!::do_serialize(ar, is_none))
  {
    ar.stream().setstate(std::ios::failbit);
    return false;
  }
  if (is_none)
  {
    o.reset();
    return true;
  }
  o = T();
  T& rval = o.value();
  //reading value
  if (!::do_serialize(ar, rval))
  {
    ar.stream().setstate(std::ios::failbit);
    return false;
  }

  return true;
}

template <template <bool> class Archive, class T>
bool do_serialize(Archive<true> &ar, boost::optional<T> &v)
{
  //writing flag
  bool is_none = !v.has_value();
  if (!::do_serialize(ar, is_none))
  {
    ar.stream().setstate(std::ios::failbit);
    return false;
  }
  if (is_none)
  {
    return true;
  }

  if (!::do_serialize(ar, v.value()))
  {
    ar.stream().setstate(std::ios::failbit);
    return false;
  }

  return true;
}

// std::optional
template <template <bool> class Archive, class T>
bool do_serialize(Archive<false> &ar, std::optional<T> &o)
{
  //reading flag
  bool is_none = false;
  if (!::do_serialize(ar, is_none))
  {
    ar.stream().setstate(std::ios::failbit);
    return false;
  }
  if (is_none)
  {
    o.reset();
    return true;
  }
  o = T();
  T& rval = o.value();
  //reading value
  if (!::do_serialize(ar, rval))
  {
    ar.stream().setstate(std::ios::failbit);
    return false;
  }

  return true;
}

template <template <bool> class Archive, class T>
bool do_serialize(Archive<true> &ar, std::optional<T> &v)
{
  //writing flag
  bool is_none = !v.has_value();
  if (!::do_serialize(ar, is_none))
  {
    ar.stream().setstate(std::ios::failbit);
    return false;
  }
  if (is_none)
  {
    return true;
  }

  if (!::do_serialize(ar, v.value()))
  {
    ar.stream().setstate(std::ios::failbit);
    return false;
  }

  return true;
}
