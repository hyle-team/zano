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

  struct transfer_destination
  {
    uint64_t amount = 0;
    std::string address;
    crypto::public_key asset_id = currency::native_coin_asset_id;
    uint64_t payment_id = 0;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)                      DOC_DSCR("Amount to transfer to destination") DOC_EXMP(10000000000000)     DOC_END
      KV_SERIALIZE(address)                     DOC_DSCR("Destination address") DOC_EXMP("ZxBvJDuQjMG9R2j4WnYUhBYNrwZPwuyXrC7FHdVmWqaESgowDvgfWtiXeNGu8Px9B24pkmjsA39fzSSiEQG1ekB225ZnrMTBp")     DOC_END
      KV_SERIALIZE_POD_AS_HEX_STRING(asset_id)  DOC_DSCR("Asset id to transfer") DOC_EXMP("cc608f59f8080e2fbfe3c8c80eb6e6a953d47cf2d6aebd345bada3a1cab99852")     DOC_END
      KV_SERIALIZE(payment_id)                  DOC_DSCR("[optional] Intrinsic 8-byte payment id for this destination. Incompatible with integrated addresses.") DOC_EXMP(1020394) DOC_END
    END_KV_SERIALIZE_MAP()
  };



}
