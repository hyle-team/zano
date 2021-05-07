// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "checkpoints.h"
#include "misc_log_ex.h"

#define ADD_CHECKPOINT(h, hash)  CHECK_AND_ASSERT(checkpoints.add_checkpoint(h,  hash), false)

namespace currency
{

  inline bool create_checkpoints(currency::checkpoints& checkpoints)
  {
#ifdef TESTNET
    //ADD_CHECKPOINT(50000,   "492ef71f5d722a8a182d65eb0ff731b740e023a2d64881f43db9af7b39ba7988");
#else
    // MAINNET
    ADD_CHECKPOINT(425000,  "46a6c36d5dec2d484d5e4845a8525ca322aafc06915ed9c8da2a241b51b7d1e8");
    ADD_CHECKPOINT(525000,  "8c1ac57e67448130207a224b2d6e33ccdc64d6dd1c59dbcf9ad2361dc0d07d51");
    ADD_CHECKPOINT(600000,  "d9fe316086e1aaea07d94082973ec764eff5fc5a05ed6e1eca273cee59daeeb4");
    ADD_CHECKPOINT(900000,  "2205b73cd79d4937b087b02a8b001171b73c34464bc4a952834eaf7c2bd63e86");
#endif

    return true;
  }

} // namespace currency 
