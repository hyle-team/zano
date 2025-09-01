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

struct wallet_rpc_alias_tests : public wallet_test
{
  wallet_rpc_alias_tests();
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

struct wallet_true_rpc_pos_mining : public wallet_test
{
  wallet_true_rpc_pos_mining();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};


struct wallet_rpc_thirdparty_custody : public wallet_test
{
  wallet_rpc_thirdparty_custody();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct wallet_rpc_cold_signing : public wallet_test
{
  wallet_rpc_cold_signing();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  void set_wallet_options(std::shared_ptr<tools::wallet2> w);

  mutable std::wstring m_wallet_filename = L"~coretests.wallet_rpc_cold_signing.file.tmp";
  mutable std::string m_wallet_password = "ballerinacappuccina";
};

struct wallet_rpc_multiple_receivers : public wallet_test
{
  wallet_rpc_multiple_receivers();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  void set_wallet_options(std::shared_ptr<tools::wallet2> w);
};

struct wallet_rpc_hardfork_verification : public wallet_test
{
  wallet_rpc_hardfork_verification();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct cb_capture_errors : public tools::i_wallet2_callback
{
  std::vector<std::string> msgs;

  void on_message(message_severity /*sev*/, const std::string& m) override;
  void clear();
  bool has_pull_err() const;
  bool has_hf_mismatch() const;
};