// Copyright (c) 2014-2018 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "crypto/wild_keccak.h"
#include "currency_protocol/blobdatatype.h"
#include "currency_core/currency_basic.h"

namespace currency
{
  class scratchpad_keeper
  {
  public:
    bool generate(const crypto::hash& seed, uint64_t height);
    crypto::hash get_pow_hash(const blobdata& bd, uint64_t height, const crypto::hash& seed);
    crypto::hash get_pow_hash(const block& b, const crypto::hash& seed);
    uint64_t size();
  private:
    crypto::hash m_seed;
    std::vector<crypto::hash> m_scratchpad;
  };

}