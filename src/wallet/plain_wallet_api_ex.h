// Copyright (c) 2014-2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#pragma once

#include <string>

#include "wallet2.h"

namespace plain_wallet
{
  struct plain_wallet_instance
{
  plain_wallet_instance() :initialized(false), gjobs_counter(1)
  {}
  wallets_manager gwm;
  std::atomic<bool> initialized;

  std::atomic<uint64_t> gjobs_counter;
  std::map<uint64_t, std::string> gjobs;
  epee::critical_section gjobs_lock;
};


  extern std::shared_ptr<plain_wallet_instance> ginstance_ptr;



}