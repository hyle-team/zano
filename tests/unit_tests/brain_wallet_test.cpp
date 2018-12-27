// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include "include_base_utils.h"
#include "currency_core/account.h"

TEST(brain_wallet, store_restore_test) 
{
  for (size_t i = 0; i != 100; i++)
  {
    currency::account_base acc;
    acc.generate();
    auto restore_data = acc.get_restore_data();
    
    currency::account_base acc2;
    bool r = acc2.restore_keys(restore_data);
    ASSERT_TRUE(r);

    if (memcmp(&acc2.get_keys(), &acc.get_keys(), sizeof(currency::account_keys)))
    {
      ASSERT_TRUE(false);
    }
  }

  for (size_t i = 0; i != 100; i++)
  {
    currency::account_base acc;
    acc.generate();
    auto restore_data = acc.get_restore_braindata();

    currency::account_base acc2;
    bool r = acc2.restore_keys_from_braindata(restore_data);
    ASSERT_TRUE(r);

    if (memcmp(&acc2.get_keys(), &acc.get_keys(), sizeof(currency::account_keys)))
    {
      ASSERT_TRUE(false);
    }
  }

}
