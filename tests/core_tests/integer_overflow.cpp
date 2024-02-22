// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"

#include "integer_overflow.h"

// TOTAL_MONEY_SUPPLY - total number coins to be generated
#define TX_MAX_TRANSFER_AMOUNT                              ((uint64_t)(-1))


using namespace epee;
using namespace currency;

namespace
{
  void split_miner_tx_outs(transaction& miner_tx, uint64_t amount_1)
  {
    uint64_t total_amount = get_outs_money_amount(miner_tx);
    uint64_t amount_2 = total_amount - amount_1;
    txout_target_v target =boost::get<currency::tx_out_bare>( miner_tx.vout[0]).target;

    miner_tx.vout.clear();

    tx_out_bare out1;
    out1.amount = amount_1;
    out1.target = target;
    miner_tx.vout.push_back(out1);

    tx_out_bare out2;
    out2.amount = amount_2;
    out2.target = target;
    miner_tx.vout.push_back(out2);
  }

  void append_tx_source_entry(std::vector<currency::tx_source_entry>& sources, const transaction& tx, size_t out_idx)
  {
    currency::tx_source_entry se = AUTO_VAL_INIT(se);
    se.amount =boost::get<currency::tx_out_bare>( tx.vout[out_idx]).amount;

    currency::tx_source_entry::output_entry oe = AUTO_VAL_INIT(oe);
    oe.out_reference = 0;
    oe.stealth_address = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(tx.vout[out_idx]).target).key;
    se.outputs.push_back(oe);
    //se.outputs.push_back(make_serializable_pair<txout_ref_v, crypto::public_key>(0, boost::get<currency::txout_to_key>(boost::get<currency::tx_out_bare>(tx.vout[out_idx]).target).key));
    se.real_output = 0;
    se.real_out_tx_key = get_tx_pub_key_from_extra(tx);
    se.real_output_in_tx_index = out_idx;

    sources.push_back(se);
  }
}

//======================================================================================================================

gen_uint_overflow_base::gen_uint_overflow_base()
  : m_last_valid_block_event_idx(static_cast<size_t>(-1))
{
  REGISTER_CALLBACK_METHOD(gen_uint_overflow_1, mark_last_valid_block);
}

bool gen_uint_overflow_base::check_tx_verification_context(const currency::tx_verification_context& tvc, bool tx_added, size_t event_idx, const currency::transaction& /*tx*/)
{
  return m_last_valid_block_event_idx < event_idx ? !tx_added && tvc.m_verification_failed : tx_added && !tvc.m_verification_failed;
}

bool gen_uint_overflow_base::check_block_verification_context(const currency::block_verification_context& bvc, size_t event_idx, const currency::block& /*block*/)
{
  return m_last_valid_block_event_idx < event_idx ? bvc.m_verification_failed | bvc.m_marked_as_orphaned : !bvc.m_verification_failed;
}

bool gen_uint_overflow_base::mark_last_valid_block(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  m_last_valid_block_event_idx = ev_index - 1;
  return true;
}

//======================================================================================================================

bool gen_uint_overflow_1::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;  

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  DO_CALLBACK(events, "mark_last_valid_block");
  MAKE_ACCOUNT(events, bob_account);
  MAKE_ACCOUNT(events, alice_account);

  // Problem 1. Miner tx output overflow
  MAKE_MINER_TX_MANUALLY(miner_tx_0, blk_0);
  split_miner_tx_outs(miner_tx_0, TX_MAX_TRANSFER_AMOUNT);
  block blk_1;
  if (!generator.construct_block_manually(blk_1, blk_0, miner_account, test_generator::bf_miner_tx, 0, 0, 0, crypto::hash(), 0, miner_tx_0))
    return false;
  events.push_back(blk_1);

  // Problem 1. Miner tx outputs overflow
  MAKE_MINER_TX_MANUALLY(miner_tx_1, blk_1);
  split_miner_tx_outs(miner_tx_1, TX_MAX_TRANSFER_AMOUNT);
  block blk_2;
  if (!generator.construct_block_manually(blk_2, blk_1, miner_account, test_generator::bf_miner_tx, 0, 0, 0, crypto::hash(), 0, miner_tx_1))
    return false;
  events.push_back(blk_2);

  REWIND_BLOCKS(events, blk_2r, blk_2, miner_account);
  MAKE_TX_LIST_START(events, txs_0, miner_account, bob_account, TX_MAX_TRANSFER_AMOUNT, blk_2r);
  MAKE_TX_LIST(events, txs_0, miner_account, bob_account, TX_MAX_TRANSFER_AMOUNT, blk_2r);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3, blk_2r, miner_account, txs_0);
  REWIND_BLOCKS(events, blk_3r, blk_3, miner_account);

  // Problem 2. total_fee overflow, block_reward overflow
  std::list<currency::transaction> txs_1;
  // Create txs with huge fee
  txs_1.push_back(construct_tx_with_fee(m_hardforks, events, blk_3, bob_account, alice_account, MK_TEST_COINS(1), TX_MAX_TRANSFER_AMOUNT - MK_TEST_COINS(1)));
  txs_1.push_back(construct_tx_with_fee(m_hardforks, events, blk_3, bob_account, alice_account, MK_TEST_COINS(1), TX_MAX_TRANSFER_AMOUNT - MK_TEST_COINS(1)));
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_4, blk_3r, miner_account, txs_1);

  return true;
}

//======================================================================================================================

bool gen_uint_overflow_2::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;  

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  MAKE_ACCOUNT(events, bob_account);
  MAKE_ACCOUNT(events, alice_account);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);
  DO_CALLBACK(events, "mark_last_valid_block");

  // Problem 1. Regular tx outputs overflow
  std::vector<currency::tx_source_entry> sources;
  for (size_t i = 0; i < blk_0.miner_tx.vout.size(); ++i)
  {
    if (TESTS_DEFAULT_FEE < boost::get<currency::tx_out_bare>(blk_0.miner_tx.vout[i]).amount)
    {
      append_tx_source_entry(sources, blk_0.miner_tx, i);
      break;
    }
  }
  if (sources.empty())
  {
    return false;
  }

  std::vector<currency::tx_destination_entry> destinations;
  const account_public_address& bob_addr = bob_account.get_keys().account_address;
  destinations.push_back(tx_destination_entry(TX_MAX_TRANSFER_AMOUNT, bob_addr));
  destinations.push_back(tx_destination_entry(TX_MAX_TRANSFER_AMOUNT - 1, bob_addr));
  // sources.front().amount = destinations[0].amount + destinations[2].amount + destinations[3].amount + TESTS_DEFAULT_FEE
  destinations.push_back(tx_destination_entry(sources.front().amount - TX_MAX_TRANSFER_AMOUNT - TX_MAX_TRANSFER_AMOUNT + 1 - TESTS_DEFAULT_FEE, bob_addr));

  currency::transaction tx_1;
  std::vector<currency::attachment_v> attachments;
  if (!construct_tx(miner_account.get_keys(), sources, destinations, attachments, tx_1, get_tx_version_from_events(events), 0))
    return false;
  events.push_back(tx_1);

  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_account, tx_1);
  REWIND_BLOCKS(events, blk_1r, blk_1, miner_account);

  // Problem 2. Regular tx inputs overflow
  sources.clear();
  for (size_t i = 0; i < tx_1.vout.size(); ++i)
  {
    auto& tx_1_out = boost::get<tx_out_bare>(tx_1.vout[i]);
    if (tx_1_out.amount < TX_MAX_TRANSFER_AMOUNT - 1)
      continue;

    append_tx_source_entry(sources, tx_1, i);
  }

  destinations.clear();
  currency::tx_destination_entry de;
  de.addr.push_back(alice_account.get_keys().account_address);
  de.amount = TX_MAX_TRANSFER_AMOUNT - TESTS_DEFAULT_FEE;
  destinations.push_back(de);
  destinations.push_back(de);

  currency::transaction tx_2;
  std::vector<currency::attachment_v> attachments2;
  if (!construct_tx(bob_account.get_keys(), sources, destinations, attachments2, tx_2, get_tx_version_from_events(events), 0))
    return false;
  events.push_back(tx_2);

  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_account, tx_2);

  return true;
}
