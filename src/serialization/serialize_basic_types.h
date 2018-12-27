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

DEFINE_POD_SERIALIZATION(bool);


