// Copyright (c) 2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "chaingen.h"
#include "hard_fork_5.h"
#include "random_helper.h"

using namespace currency;

hard_fork_5_tx_version::hard_fork_5_tx_version()
{
  REGISTER_CALLBACK_METHOD(hard_fork_5_tx_version, c1);
}

bool hard_fork_5_tx_version::generate(std::vector<test_event_entry>& events) const
{
  //
  // Test idea: ensure that the correct tx.hardfork_id is required after HF5.
  //
  bool r = false;
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  // make a simple tx just to check HF5 tx.hardfork_id rule basic validity
  MAKE_TX(events, tx_1, miner_acc, alice_acc, MK_TEST_COINS(1), blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  //
  // construct a tx with an incorrect hardfork_id and make sure it won't be accepted by either the tx pool or the core
  //
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;

  r = fill_tx_sources_and_destinations(events, blk_1, miner_acc, alice_acc, MK_TEST_COINS(7), TX_DEFAULT_FEE, 4, sources, destinations);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");

  transaction tx_2{};
  size_t tx_hardfork_id{};
  uint64_t tx_version = get_tx_version_and_hardfork_id(get_block_height(blk_1) + 1, m_hardforks, tx_hardfork_id);
  size_t incorrect_tx_hardfork_id = 0;
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_2, tx_version, incorrect_tx_hardfork_id, 0);  // note using incorrect hardfork id here
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  // mark as invalid, shouldn't be accepted by the tx pool
  DO_CALLBACK(events, "mark_invalid_tx");
  ADD_CUSTOM_EVENT(events, tx_2);

  // now add tx_2 as 'kept_by_block', with tx pool checks turned off
  ADD_CUSTOM_EVENT(events, event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
  events.push_back(tx_2); // now tx should be accepted by the pool
  ADD_CUSTOM_EVENT(events, event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));
  // the block shouldn't be accepted, because of invalid tx_2
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_2_bad, blk_1, miner_acc, tx_2);


  //
  // reconstruct the same tx, now using the correct tx_hardfork_id, and make sure it will be accepted
  //
  tx_2 = transaction{};
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_2, tx_version, tx_hardfork_id, 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  ADD_CUSTOM_EVENT(events, tx_2);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, tx_2);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_2r, blk_2, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // epilogue

  DO_CALLBACK(events, "c1");

  return true;
}

bool hard_fork_5_tx_version::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  size_t blocks_fetched = 0;
  alice_wlt->refresh(blocks_fetched);
  CHECK_AND_ASSERT_EQ(blocks_fetched, 2 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 5);

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", MK_TEST_COINS(8)), false, "");

  return true;
}
