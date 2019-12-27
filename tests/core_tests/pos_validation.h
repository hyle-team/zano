// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"

struct pos_validation : public test_chain_unit_base
{
  pos_validation();

  bool check_tx_verification_context(const currency::tx_verification_context& tvc, bool tx_added, size_t event_idx, const currency::transaction& /*tx*/)
  {
    if (m_invalid_tx_index == event_idx)
      return tvc.m_verification_failed;
    
    return !tvc.m_verification_failed && tx_added;
  }

  bool check_block_verification_context(const currency::block_verification_context& bvc, size_t event_idx, const currency::block& /*block*/)
  {
    if (m_invalid_block_index == event_idx)
      return bvc.m_verification_failed;
    
    return !bvc.m_verification_failed;
  }

  bool mark_invalid_block(currency::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
  {
    m_invalid_block_index = ev_index + 1;
    return true;
  }

  bool mark_invalid_tx(currency::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
  {
    m_invalid_tx_index = ev_index + 1;
    return true;
  }

  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable test_generator generator;
  mutable std::list<currency::account_base> m_accounts;

private:
  size_t m_invalid_block_index;
  size_t m_invalid_tx_index;
};


struct gen_pos_coinstake_already_spent : public pos_validation
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_pos_incorrect_timestamp : public pos_validation
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_pos_too_early_pos_block : public pos_validation
{
  gen_pos_too_early_pos_block();
  bool generate(std::vector<test_event_entry>& events) const;
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  size_t m_pos_min_height;
};

struct gen_pos_extra_nonce : public pos_validation
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_pos_min_allowed_height : public pos_validation
{
  gen_pos_min_allowed_height();
  bool generate(std::vector<test_event_entry>& events) const;
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct gen_pos_invalid_coinbase : public pos_validation
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct pos_wallet_minting_same_amount_diff_outs : public wallet_test
{
  pos_wallet_minting_same_amount_diff_outs();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  struct wlt_info_t
  {
    wlt_info_t() : w(nullptr), pos_entries_count(0), mined_coins(0) {}
    std::shared_ptr<tools::wallet2> w;
    size_t pos_entries_count;
    uint64_t mined_coins;
  };

  bool prepare_wallets_0(currency::core& c, const std::vector<test_event_entry>& events, std::vector<wlt_info_t> &minting_wallets, std::vector<size_t>& minting_wallets_order);
  bool prepare_wallets_1(currency::core& c, const std::vector<test_event_entry>& events, std::vector<wlt_info_t> &minting_wallets, std::vector<size_t>& minting_wallets_order);
  void dump_wallets_entries(const std::vector<wlt_info_t>& minting_wallets);

  size_t m_minting_wallets_count;
  uint64_t m_wallet_stake_amount;
};

struct pos_wallet_big_block_test : public wallet_test
{
  pos_wallet_big_block_test();
  bool generate(std::vector<test_event_entry>& events) const;
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct pos_altblocks_validation : public wallet_test
{
  pos_altblocks_validation();
  bool generate(std::vector<test_event_entry>& events) const;
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct pos_minting_tx_packing : public wallet_test
{
  pos_minting_tx_packing();
  bool generate(std::vector<test_event_entry>& events) const;
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable size_t m_pos_mint_packing_size;
  mutable size_t m_alice_start_amount;
};
