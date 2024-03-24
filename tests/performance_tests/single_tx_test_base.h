// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "currency_core/account.h"
#include "currency_core/currency_basic.h"
#include "currency_core/currency_format_utils.h"

class single_tx_test_base
{
public:
  bool init()
  {
    using namespace currency;

    m_bob.generate();

    uint64_t block_reward_without_fee = 0;

    uint64_t block_reward = 0;
    if(!construct_miner_tx(0, 0, 0, 2, 0, m_bob.get_keys().account_address, m_bob.get_keys().account_address, m_tx, block_reward_without_fee, block_reward, TRANSACTION_VERSION_PRE_HF4, blobdata(), CURRENCY_MINER_TX_MAX_OUTS))
      return false;
    m_tx_pub_key = get_tx_pub_key_from_extra(m_tx);
    return true;
  }

protected:
  currency::account_base m_bob;
  currency::transaction m_tx;
  crypto::public_key m_tx_pub_key;
};
