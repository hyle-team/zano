// Copyright (c) 2014-2018 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "warnings.h"

PUSH_VS_WARNINGS
DISABLE_VS_WARNINGS(4100)
DISABLE_VS_WARNINGS(4503)
#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage_template_helper.h"
POP_VS_WARNINGS

namespace currency
{
  template<typename t_type>
  struct struct_with_one_t_type
  {
    t_type v;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(v)
    END_KV_SERIALIZE_MAP()
  };
}
