// Copyright (c) 2014-2018 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "currency_core/currency_format_utils_abstract.h"
#include "currency_core/currency_format_utils_blocks.h"
#include "storages/portable_storage_to_json.h"

namespace currency
{
  //------------------------------------------------------------------------------------------------------------------------
  inline std::string print_complete_block_entry_list(const std::list<block_complete_entry>& blocks)
  {
    std::stringstream ss;
    size_t i = 0;
    for (const block_complete_entry& block_entry : blocks)
    {
      ss << "[" << i << "]";
      block b = AUTO_VAL_INIT(b);
      if (!parse_and_validate_object_from_blob(block_entry.block, b))
      {
        ss << "UNPARSED" << ENDL;
      }
      ss << get_block_hash(b) << ", parent: " << b.prev_id;

      i++;
    }
    return ss.str();
  }

  inline
    std::string print_kv_structure(const NOTIFY_RESPONSE_GET_OBJECTS::request& v)
  {
    std::stringstream ss; 
    ss << "\"blocks\":{" << ENDL << print_complete_block_entry_list(v.blocks) << ENDL << "}, " << ENDL;
    ss << "\"missed_ids\":" << ENDL;
    ::epee::serialization::dump_as_json(ss, v.missed_ids, 2);
    ss << ENDL << "\"current_blockchain_height\":" << v.current_blockchain_height;
    return ss.str();
  }

}