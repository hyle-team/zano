// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 

#define SCRATCHPAD_DEFAULT_FILENAME "scratchpad.bin"
#define WILD_KECCAK_ADDENDUMS_ARRAY_SIZE  10

#pragma pack (push, 1)

struct  export_scratchpad_hi
{
  crypto::hash prevhash;
  uint64_t height;
};
struct export_addendums_array_entry
{
  export_scratchpad_hi prev_hi;
  uint64_t add_size;
};
struct export_scratchpad_file_header
{
  export_scratchpad_hi current_hi;
  export_addendums_array_entry add_arr[WILD_KECCAK_ADDENDUMS_ARRAY_SIZE];
  uint64_t scratchpad_size;
};
#pragma pack(pop)
