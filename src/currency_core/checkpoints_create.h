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

#else
    // MAINNET
    ADD_CHECKPOINT(425000,   "46a6c36d5dec2d484d5e4845a8525ca322aafc06915ed9c8da2a241b51b7d1e8");
    ADD_CHECKPOINT(525000,   "8c1ac57e67448130207a224b2d6e33ccdc64d6dd1c59dbcf9ad2361dc0d07d51");
    ADD_CHECKPOINT(600000,   "d9fe316086e1aaea07d94082973ec764eff5fc5a05ed6e1eca273cee59daeeb4");
    ADD_CHECKPOINT(900000,   "2205b73cd79d4937b087b02a8b001171b73c34464bc4a952834eaf7c2bd63e86");
    ADD_CHECKPOINT(1161000,  "96990d851b484e30190678756ba2a4d3a2f92b987e2470728ac1e38b2bf35908");
    ADD_CHECKPOINT(1480000,  "5dd3381eec35e8b4eba4518bfd8eec682a4292761d92218fd59b9f0ffedad3fe");
    ADD_CHECKPOINT(2000000,  "7b6698a8cc279aa78d6263f01fef186bd16f5b1ea263a7f4714abc1d506b9cb3");
#endif

    return true;
  }

} // namespace currency 
