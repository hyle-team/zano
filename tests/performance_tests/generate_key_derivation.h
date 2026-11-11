// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "crypto/crypto.h"
#include "currency_core/currency_basic.h"

#include "single_tx_test_base.h"

uint64_t g_antioptimisation = 0;

class test_generate_key_derivation : public single_tx_test_base
{
public:
  static const size_t loop_count = 1;
  std::list<currency::account_base> accounts;

  bool init()
  {

    for (size_t i = 0; i != 10000; i++)
    {
      accounts.push_back(currency::account_base());
      accounts.back().generate();
    }

    return single_tx_test_base::init();
  }

  bool test()
  {
    for (auto &a : accounts)
    {
      crypto::key_derivation recv_derivation = AUTO_VAL_INIT(recv_derivation);
      crypto::generate_key_derivation(m_tx_pub_key, a.get_keys().view_secret_key, recv_derivation);
      g_antioptimisation ^= *(uint64_t*)(&recv_derivation);
    }

    return true;
  }
};
