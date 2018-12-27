// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"

struct wallet_test : public test_chain_unit_enchanced
{
  enum { MINER_ACC_IDX = 0, ALICE_ACC_IDX = 1, BOB_ACC_IDX = 2, CAROL_ACC_IDX = 3, DAN_ACC_IDX = 4, TOTAL_ACCS_COUNT = 5 }; // to be used as index for m_accounts

  wallet_test();

  bool need_core_proxy() const { return true; }
  void set_core_proxy(std::shared_ptr<tools::i_core_proxy> p) { m_core_proxy = p; }
  bool check_balance_via_build_wallets(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_balance(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

protected:

  struct params_check_balance
  {
    params_check_balance(size_t account_index = 0,
      uint64_t total_balance = 0,
      uint64_t unlocked_balance = std::numeric_limits<uint64_t>::max(),
      uint64_t mined_balance = std::numeric_limits<uint64_t>::max(),
      uint64_t awaiting_in = std::numeric_limits<uint64_t>::max(),
      uint64_t awaiting_out = std::numeric_limits<uint64_t>::max())
      : account_index(account_index), total_balance(total_balance), unlocked_balance(unlocked_balance), mined_balance(mined_balance), awaiting_in(awaiting_in), awaiting_out(awaiting_out) {}
    uint64_t total_balance;
    uint64_t unlocked_balance;
    uint64_t mined_balance;
    uint64_t awaiting_in;
    uint64_t awaiting_out;
    size_t account_index;
  };

  std::shared_ptr<tools::wallet2> init_playtime_test_wallet(const std::vector<test_event_entry>& events, currency::core& c, size_t account_index) const;
  std::shared_ptr<tools::wallet2> init_playtime_test_wallet(const std::vector<test_event_entry>& events, currency::core& c, const currency::account_base& acc) const;

  mutable std::vector<currency::account_base> m_accounts;
  mutable test_generator generator;
  std::shared_ptr<tools::i_core_proxy> m_core_proxy;
};
