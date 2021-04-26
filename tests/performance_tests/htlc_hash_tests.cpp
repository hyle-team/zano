// Copyright (c) 2014-2015 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cstdint>
#include <vector>


#include "crypto/bitcoin/sha256_helper.h"
#include "crypto/RIPEMD160_helper.h"
#include "include_base_utils.h"

#include "currency_core/currency_core.h"


bool do_htlc_hash_tests()
{
  std::string test_string("some hash");
  crypto::hash res_hash = AUTO_VAL_INIT(res_hash);
  res_hash = crypto::sha256_hash(test_string.data(), test_string.size());
  LOG_PRINT_L0("Sha256: " << res_hash);

  crypto::hash160 res_hash160 = AUTO_VAL_INIT(res_hash160);
  res_hash160 = crypto::RIPEMD160_hash(test_string.data(), test_string.size());
  LOG_PRINT_L0("RIPEMD160: " << res_hash160);


  return true;
}



