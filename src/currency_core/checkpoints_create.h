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
    ADD_CHECKPOINT(50000,   "cb05a7bdc7f78c5cdb6ef1048f85b27c569f44879233903ce5f5a4e5bd590a3d");
    ADD_CHECKPOINT(100000,  "6b8b54356a9d44f6c1ebdacb8593d8f5ab2e2e2ca4493e7ae7baf4b3755c5e16");
#else
    // MAINNET
    ADD_CHECKPOINT(425000,  "46a6c36d5dec2d484d5e4845a8525ca322aafc06915ed9c8da2a241b51b7d1e8");
#endif

    return true;
  }

} // namespace currency
