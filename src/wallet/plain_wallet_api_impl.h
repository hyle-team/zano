// Copyright (c) 2014-2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#pragma once

#include <string>
#include "wallet2.h"


namespace wallet
{
  class plain_wallet_api_impl
  {
  public: 
    bool init(const std::string ip, const std::string port);
  private: 
    std::shared_ptr<tools::wallet2> m_wallet;
  };

}