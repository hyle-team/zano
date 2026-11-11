// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "account.h"
#include "currency_core/currency_boost_serialization.h"

//namespace currency {
namespace boost
{
  namespace serialization
  {
    template <class Archive>
    inline void serialize(Archive &a, currency::account_keys &x, const boost::serialization::version_type ver)
    {
      a & x.account_address;
      a & x.spend_secret_key;
      a & x.view_secret_key;
    }

  }
}
