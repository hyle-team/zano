// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "currency_core/account.h"
#include "currency_core/currency_basic.h"
#include "currency_core/currency_format_utils.h"

#include "single_tx_test_base.h"

class test_is_out_to_acc : public single_tx_test_base
{
public:
  static const size_t loop_count = 1000;

  bool test()
  {
    const currency::txout_to_key& tx_out = boost::get<currency::txout_to_key>(boost::get<currency::tx_out_bare>(m_tx.vout[0]).target);

    crypto::key_derivation derivation;
    generate_key_derivation(m_tx_pub_key, m_bob.get_keys().view_secret_key, derivation);
    return currency::is_out_to_acc(m_bob.get_public_address(), tx_out, derivation, 0);
  }
};
