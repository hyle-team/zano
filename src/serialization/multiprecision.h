// Copyright (c) 2014-2017 The The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <memory>
#include <boost/multiprecision/cpp_int.hpp>


#include "serialization.h"

template <template <bool> class Archive>
inline bool do_serialize(Archive<false>& ar, boost::multiprecision::uint128_t& diff)
{
  uint64_t hi = 0;
  uint64_t lo = 0;
  ar.serialize_uint(hi);
  ar.serialize_uint(lo);

  diff = hi;
  diff = (diff << 64) | lo;
  return true;
}


template <template <bool> class Archive>
inline bool do_serialize(Archive<true>& ar, boost::multiprecision::uint128_t& diff_)
{
  uint64_t hi = (diff_ >> 64).convert_to<uint64_t>();
  uint64_t lo = (diff_ & (0xFFFFFFFFFFFFFFFFLL)).convert_to<uint64_t>();
  ar.serialize_uint(hi);
  ar.serialize_uint(lo);
  return true;
}
