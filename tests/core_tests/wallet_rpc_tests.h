// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"


struct wallet_rpc_integrated_address : public wallet_test
{
  wallet_rpc_integrated_address();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct wallet_rpc_integrated_address_transfer : public wallet_test
{
  wallet_rpc_integrated_address_transfer();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct wallet_rpc_transfer : public wallet_test
{
  wallet_rpc_transfer();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

/*
   Tests to make sure api for exchanges didn't change after HF4(Zarcanum)
   testing api: get_recent_txs_and_info, make_integrated_address, 
   getbalance, get_wallet_info, get_transfer_by_txid,
*/
struct wallet_rpc_exchange_suite : public wallet_test
{
  wallet_rpc_exchange_suite();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

