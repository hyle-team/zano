// Copyright (c) 2018-2023 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include "include_base_utils.h"
#include "currency_format_utils_transactions.h"


namespace currency
{
  //check correct values, ins and outs types, amounts and all lightweight checks not related to the database
  bool validate_tx_semantic(const transaction& tx, size_t tx_block_size);
}
