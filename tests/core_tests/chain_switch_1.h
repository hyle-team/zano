// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"

/************************************************************************/
/*                                                                      */
/************************************************************************/
class gen_chain_switch_1 : public test_chain_unit_base
{
public: 
  gen_chain_switch_1();

  bool generate(std::vector<test_event_entry>& events) const;

  bool check_split_not_switched(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_split_switched(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  std::list<currency::block> m_chain_1;

  currency::account_base m_recipient_account_1;
  currency::account_base m_recipient_account_2;
  currency::account_base m_recipient_account_3;
  currency::account_base m_recipient_account_4;

  std::list<currency::transaction> m_tx_pool;
  mutable uint64_t m_miner_initial_amount;
  mutable currency::account_base m_miner_account;
};


struct bad_chain_switching_with_rollback : public test_chain_unit_enchanced
{
  bad_chain_switching_with_rollback();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable crypto::hash m_invalid_block_hash_to_check;
};

struct chain_switching_and_tx_with_attachment_blobsize : public test_chain_unit_enchanced
{
  chain_switching_and_tx_with_attachment_blobsize();
  bool generate(std::vector<test_event_entry>& events) const;
  bool check_tx_pool_txs(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct chain_switching_when_gindex_spent_in_both_chains : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct alt_chain_coins_pow_mined_then_spent : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct alt_blocks_validation_and_same_new_amount_in_two_txs : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct alt_blocks_with_the_same_txs : public test_chain_unit_enchanced
{
  alt_blocks_with_the_same_txs();
  bool generate(std::vector<test_event_entry>& events) const;
  bool check_tx_related_to_altblock(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_tx_not_related_to_altblock(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct chain_switching_when_out_spent_in_alt_chain_mixin : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct chain_switching_when_out_spent_in_alt_chain_ref_id : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};
