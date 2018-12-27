// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include <string>
namespace currency
{
#pragma pack(push, 1)
  struct genesis_tx_raw_data
  {
    uint64_t const v[33];
    uint8_t const r[1];
  };
#pragma pack(pop)
  extern const genesis_tx_raw_data ggenesis_tx_raw;
}




