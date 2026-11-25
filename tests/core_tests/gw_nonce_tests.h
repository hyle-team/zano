// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include "chaingen.h"
#include "wallet_tests_basic.h"

// params of the check_gw_nonce callback: the nonce the blockchain expects in the next tx with inputs of gw_addr
struct gw_nonce_check_param
{
  currency::gateway_address_id_type gw_addr;
  uint64_t next_nonce = 0;

  BEGIN_SERIALIZE()
    FIELD(gw_addr)
    FIELD(next_nonce)
  END_SERIALIZE()
};

struct gw_nonce_test_base : public wallet_test
{
  gw_nonce_test_base();
  bool check_gw_nonce(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct gw_nonce_strict_order : public gw_nonce_test_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gw_nonce_per_tx_and_per_address : public gw_nonce_test_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gw_nonce_chain_switch : public gw_nonce_test_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

// base for the playtime tests of tx pool rules, block template and pool cleanup for txs with gw inputs:
// generate() only registers and funds the gw addresses, all the scenarios are played inside c1()
struct gw_nonce_playtime_base : public gw_nonce_test_base
{
  gw_nonce_playtime_base(size_t gw_addresses_count);
  bool generate(std::vector<test_event_entry>& events) const;
  virtual bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events) = 0;

protected:
  enum tx_push_result { tx_accepted, tx_rejected, tx_already_exists };
  typedef std::vector<const currency::transaction*> txs_refs;

  // gw-only tx of the gw address #gw_index with the given nonce and fee; padding_size > 0 makes the tx bigger
  bool make_gw_tx(currency::core& c, size_t gw_index, uint64_t nonce, uint64_t fee, currency::transaction& tx, size_t padding_size = 0) const;
  // regular tx (miner -> Bob) made by the wallet, the wallet sends it to the pool itself
  bool make_regular_tx(currency::core& c, tools::wallet2& miner_wlt, uint64_t fee, currency::transaction& tx, size_t padding_size = 0) const;
  // pushes the tx to the core in the same way as a tx coming from the network and checks the outcome
  bool push_tx(currency::core& c, const currency::transaction& tx, tx_push_result expected, const std::string& descr) const;
  bool is_tx_in_pool(currency::core& c, const currency::transaction& tx) const;
  // checks txs of the block template (the order matters)
  bool check_block_template(currency::core& c, const txs_refs& expected_txs, const std::string& descr) const;
  // mines a block using the block template and checks its txs (the order matters)
  bool mine_block(currency::core& c, const txs_refs& expected_txs, const std::string& descr) const;
  // mines blocks_count blocks on top of prev_id bypassing the pool and the block template, the txs go to the first block
  bool mine_blocks_with_given_txs(currency::core& c, const std::vector<currency::transaction>& txs, const crypto::hash& prev_id, size_t blocks_count) const;
  uint64_t get_next_nonce(currency::core& c, size_t gw_index) const;

  size_t m_gw_addresses_count;
  mutable std::vector<currency::keypair> m_gw_view_keys;
  mutable std::vector<currency::keypair> m_gw_spend_keys;
};

struct gw_nonce_pool_rules : public gw_nonce_playtime_base
{
  gw_nonce_pool_rules() : gw_nonce_playtime_base(3) {}
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events) override;
};

struct gw_nonce_block_template : public gw_nonce_playtime_base
{
  gw_nonce_block_template() : gw_nonce_playtime_base(3) {}
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events) override;
};

struct gw_nonce_template_size_caps : public gw_nonce_playtime_base
{
  gw_nonce_template_size_caps() : gw_nonce_playtime_base(2) {}
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events) override;
};

struct gw_nonce_reorg_and_pool : public gw_nonce_playtime_base
{
  gw_nonce_reorg_and_pool() : gw_nonce_playtime_base(1) {}
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events) override;
};
