// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2016 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#pragma once 

//======================================================================================================================

template<class concrete_test>
gen_double_spend_base<concrete_test>::gen_double_spend_base()
{
  REGISTER_CALLBACK_METHOD(gen_double_spend_base<concrete_test>, mark_last_valid_block);
  REGISTER_CALLBACK_METHOD(gen_double_spend_base<concrete_test>, check_double_spend);
}

template<class concrete_test>
bool gen_double_spend_base<concrete_test>::mark_last_valid_block(currency::core& c, size_t /*ev_index*/, const std::vector<test_event_entry>& /*events*/)
{
  std::list<currency::block> block_list;
  bool r = c.get_blocks(c.get_top_block_height(), 1, block_list);
  CHECK_AND_ASSERT_MES(r, false, "core::get_blocks failed");
  m_last_valid_block = block_list.back();
  return true;
}

template<class concrete_test>
bool gen_double_spend_base<concrete_test>::check_double_spend(currency::core& c, size_t /*ev_index*/, const std::vector<test_event_entry>& events)
{

  if (concrete_test::has_invalid_tx)
  {
    CHECK_NOT_EQ(invalid_index_value, m_invalid_tx_index);
  }
  CHECK_NOT_EQ(invalid_index_value, m_invalid_block_index);

  std::list<currency::block> block_list;
  bool r = c.get_blocks(0, 100 + 2 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW, block_list);
  CHECK_TEST_CONDITION(r);
  CHECK_TEST_CONDITION(m_last_valid_block == block_list.back());

  CHECK_EQ(concrete_test::expected_pool_txs_count, c.get_pool_transactions_count());

  currency::account_base bob_account = boost::get<currency::account_base>(events[1]);
  currency::account_base alice_account = boost::get<currency::account_base>(events[2]);

  std::vector<currency::block> chain;
  map_hash2tx_t mtx;
  std::vector<currency::block> blocks(block_list.begin(), block_list.end());
  r = find_block_chain(events, chain, mtx, get_block_hash(blocks.back()));
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(concrete_test::expected_bob_balance, get_balance(bob_account, blocks, mtx));
  CHECK_EQ(concrete_test::expected_alice_balance, get_balance(alice_account, blocks, mtx));

  return true;
}

//======================================================================================================================

template<bool txs_kept_by_block>
bool gen_double_spend_in_tx<txs_kept_by_block>::generate(std::vector<test_event_entry>& events) const
{
  INIT_DOUBLE_SPEND_TEST();
  DO_CALLBACK(events, "mark_last_valid_block");

  std::vector<currency::tx_source_entry> sources;
  currency::tx_source_entry se = AUTO_VAL_INIT(se);
  se.real_output_in_tx_index = 0;
  // find correct output by amount (selecting random or fixed one can be possibly mistaken with changeback)
  for (auto out : tx_0.vout)
  {
    se.amount = boost::get<currency::tx_out_bare>(out).amount;
    if (se.amount == send_amount)
      break;
    ++se.real_output_in_tx_index;
  }
  CHECK_AND_ASSERT_MES(se.amount == send_amount, false, "Invalid state error: can't find correct amount among tx outputs.");
  uint64_t global_out_index = 0;
  bool r = find_global_index_for_output(events, get_block_hash(blk_1r), tx_0, se.real_output_in_tx_index, global_out_index);
  CHECK_AND_ASSERT_MES(r, false, "find_global_index_for_output failed");
  se.outputs.push_back(currency::tx_source_entry::output_entry(global_out_index, boost::get<currency::txout_to_key>(boost::get<currency::tx_out_bare>(tx_0.vout[se.real_output_in_tx_index]).target).key));
  se.real_output = 0;
  se.real_out_tx_key = get_tx_pub_key_from_extra(tx_0);
  sources.push_back(se);
  // Double spend!
  sources.push_back(se);

  currency::tx_destination_entry de = AUTO_VAL_INIT(de);
  de.addr.push_back(alice_account.get_keys().account_address);
  de.amount = 2 * se.amount - TESTS_DEFAULT_FEE;
  std::vector<currency::tx_destination_entry> destinations;
  destinations.push_back(de);

  currency::transaction tx_1 = AUTO_VAL_INIT(tx_1);
  std::vector<currency::attachment_v> attachments;
  if (!construct_tx(bob_account.get_keys(), sources, destinations, attachments, tx_1, this->get_tx_version_from_events(events), uint64_t(0)))
    return false;

  SET_EVENT_VISITOR_SETT(events, event_visitor_settings::set_txs_kept_by_block, txs_kept_by_block);
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_1);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_account, tx_1);
  DO_CALLBACK(events, "check_double_spend");

  return true;
}

template<bool txs_kept_by_block>
bool gen_double_spend_in_the_same_block<txs_kept_by_block>::generate(std::vector<test_event_entry>& events) const
{
  INIT_DOUBLE_SPEND_TEST();

  //  0       10      11       21      22      23
  // (0 )-...(0r)-   (1r)-....(1r)-   !2 !-             // blk_2 marked as invalid
  //                 tx_0                               // tx_0: miner -> bob
  //                                  txs_1[0]          // txs_1[0]: bob -> alice
  //                                  txs_1[1]          // txs_1[1]: bob (the same input) -> alice, marked invalid if txs_kept_by_block == false

  DO_CALLBACK(events, "mark_last_valid_block");
  SET_EVENT_VISITOR_SETT(events, event_visitor_settings::set_txs_kept_by_block, txs_kept_by_block);

  MAKE_TX_LIST_START(events, txs_1, bob_account, alice_account, send_amount - TESTS_DEFAULT_FEE, blk_1);
  currency::transaction tx_1 = txs_1.front();
  auto tx_1_idx = events.size() - 1;
  // Remove tx_1, it is being inserted back a little later
  events.pop_back();

  if (has_invalid_tx)
  {
    DO_CALLBACK(events, "mark_invalid_tx");
  }
  MAKE_TX_LIST(events, txs_1, bob_account, alice_account, send_amount - TESTS_DEFAULT_FEE, blk_1);
  events.insert(events.begin() + tx_1_idx, tx_1);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1r, miner_account, txs_1);
  DO_CALLBACK(events, "check_double_spend");

  return true;
}

template<bool txs_kept_by_block>
bool gen_double_spend_in_different_blocks<txs_kept_by_block>::generate(std::vector<test_event_entry>& events) const
{
  INIT_DOUBLE_SPEND_TEST();

  DO_CALLBACK(events, "mark_last_valid_block");
  SET_EVENT_VISITOR_SETT(events, event_visitor_settings::set_txs_kept_by_block, txs_kept_by_block);

  // Create two identical transactions, but don't push it to events list
  MAKE_TX(events, tx_blk_2, bob_account, alice_account, send_amount - TESTS_DEFAULT_FEE, blk_1);
  events.pop_back();
  MAKE_TX(events, tx_blk_3, bob_account, alice_account, send_amount - TESTS_DEFAULT_FEE, blk_1);
  events.pop_back();

  events.push_back(tx_blk_2);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_account, tx_blk_2);
  DO_CALLBACK(events, "mark_last_valid_block");

  if (has_invalid_tx)
  {
    DO_CALLBACK(events, "mark_invalid_tx");
  }
  events.push_back(tx_blk_3);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_account, tx_blk_3);

  DO_CALLBACK(events, "check_double_spend");

  return true;
}

template<bool txs_kept_by_block>
bool gen_double_spend_in_alt_chain_in_the_same_block<txs_kept_by_block>::generate(std::vector<test_event_entry>& events) const
{
  INIT_DOUBLE_SPEND_TEST();                                                                                          // 2N+4, N = CURRENCY_MINED_MONEY_UNLOCK_WINDOW (== 10 atm)

  //  0       10      11       21      22      23
  // (0 )-...(0r)-   (1r)-....(1r)-   (2 )-
  //                 tx_0        \                      // tx_0: miner -> bob
  //                              \-  (3 )-   (4 )-     // alt chain
  //                                  txs_1             // txs_1[0]: bob -> alice, txs_1[1]: bob (the same input) -> alice
  //                                                    // so blk_3 will be invalid anyway just because of two txs spending the same input

  SET_EVENT_VISITOR_SETT(events, event_visitor_settings::set_txs_kept_by_block, txs_kept_by_block);                  // 2N+5

  // Main chain
  MAKE_NEXT_BLOCK(events, blk_2, blk_1r, miner_account);                                                             // 2N+6
  DO_CALLBACK(events, "mark_last_valid_block");                                                                      // 2N+7

  // Alt chain
  MAKE_TX_LIST_START(events, txs_1, bob_account, alice_account, send_amount - TESTS_DEFAULT_FEE, blk_1);
  currency::transaction tx_1 = txs_1.front();
  auto tx_1_idx = events.size() - 1;                                                                                 // 2N+8 (will be inserted later, oh shi...)
  // Remove tx_1, it is being inserted back a little later
  events.pop_back();

  if (has_invalid_tx)
    DO_CALLBACK(events, "mark_invalid_tx");                                                                          // ? 2N+9

  MAKE_TX_LIST(events, txs_1, bob_account, alice_account, send_amount - TESTS_DEFAULT_FEE, blk_1);                   // this tx spends the same outs as tx_1
  events.insert(events.begin() + tx_1_idx, tx_1);

  DO_CALLBACK(events, "mark_invalid_block");                                                                         // invalid tx will render blk_3 invalid as well, even if it's in alt chain
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3, blk_1r, miner_account, txs_1);                                              // blk_3 contains TWO txs spending the same outs: txs_1[0] (== tx_1 ) and txs_1[1]

  // Try to switch to alternative chain
  DO_CALLBACK(events, "mark_orphan_block"); // blk_4 will always be orphan, regardless of kept_by_block flag, because blk_3 is always invalid
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_account);

  DO_CALLBACK(events, "check_double_spend");

  return true;
}

template<bool txs_kept_by_block>
bool gen_double_spend_in_alt_chain_in_different_blocks<txs_kept_by_block>::generate(std::vector<test_event_entry>& events) const
{
  INIT_DOUBLE_SPEND_TEST();

  SET_EVENT_VISITOR_SETT(events, event_visitor_settings::set_txs_kept_by_block, txs_kept_by_block);

  // Main chain
  MAKE_NEXT_BLOCK(events, blk_2, blk_1r, miner_account);
  DO_CALLBACK(events, "mark_last_valid_block");

  // Alternative chain
  MAKE_TX(events, tx_1, bob_account, alice_account, send_amount - TESTS_DEFAULT_FEE, blk_1);
  events.pop_back();
  MAKE_TX(events, tx_2, bob_account, alice_account, send_amount - TESTS_DEFAULT_FEE, blk_1);
  events.pop_back();

  events.push_back(tx_1);
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_1r, miner_account, tx_1);

  // Try to switch to alternative chain
  if (has_invalid_tx)
  {
    DO_CALLBACK(events, "mark_invalid_tx");
  }
  events.push_back(tx_2);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_3, miner_account, tx_2);

  DO_CALLBACK(events, "check_double_spend");

  return true;
}
