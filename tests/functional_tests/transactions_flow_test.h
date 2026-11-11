// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define TRANSACTIONS_FLOW_TESTACTION_DEFAULT          0
#define TRANSACTIONS_FLOW_TESTACTION_ALIAS            1
#define TRANSACTIONS_FLOW_TESTACTION_OFFERS           2
#define TRANSACTIONS_FLOW_TESTACTION_SPLIT_INITAL     3
#define TRANSACTIONS_FLOW_TESTACTION_MIXED            4

bool transactions_flow_test(
  std::wstring path_source_wallet, std::string source_wallet_pass,
  std::wstring path_terget_wallet, std::string target_wallet_pass,
  std::string& daemon_addr_a,
  std::string& daemon_addr_b,
  size_t mix_in_factor, size_t action = TRANSACTIONS_FLOW_TESTACTION_DEFAULT, uint64_t max_tx_in_pool = 10000);
bool test_serialization();
bool test_serialization2();