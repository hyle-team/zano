// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <stdint.h>
#include <algorithm>
#include "currency_format_utils.h"

namespace currency {

#pragma pack(push, 1)
struct genesis_tx_dictionary_entry {
  uint64_t addr_hash;
  uint64_t offset;

  bool operator<(const genesis_tx_dictionary_entry& s) const
  {
    return addr_hash < s.addr_hash;
  }
};
#pragma pack(pop)

#ifndef TESTNET
extern const genesis_tx_dictionary_entry ggenesis_dict[26];
#else
extern const genesis_tx_dictionary_entry ggenesis_dict[5];
#endif

extern const crypto::public_key ggenesis_tx_pub_key;

inline bool get_account_genesis_offset_by_address(const std::string& addr, uint64_t& offset)
{
  genesis_tx_dictionary_entry key_entry = AUTO_VAL_INIT(key_entry);
  key_entry.addr_hash                   = get_string_uint64_hash(addr);

  const genesis_tx_dictionary_entry* pfirst = &ggenesis_dict[0];
  const genesis_tx_dictionary_entry* plast  = &ggenesis_dict[sizeof(ggenesis_dict) / sizeof(ggenesis_dict[0])];
  const genesis_tx_dictionary_entry* plower = std::lower_bound(pfirst, plast, key_entry);
  if(plower == plast)
    return false;
  if(plower->addr_hash != key_entry.addr_hash)
    return false;
  offset = plower->offset;
  return true;
}
}
