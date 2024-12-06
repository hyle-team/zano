// Copyright (c) 2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"


struct hard_fork_5_tx_version : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};
