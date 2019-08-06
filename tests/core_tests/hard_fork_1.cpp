// Copyright (c) 2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "hard_fork_1.h"
//#include "pos_validation.h"
//#include "tx_builder.h"
//#include "random_helper.h"

using namespace currency;

hard_fork_1_unlock_time_2_in_normal_tx::hard_fork_1_unlock_time_2_in_normal_tx()
  : m_hardfork_height(12)
{
  REGISTER_CALLBACK_METHOD(hard_fork_1_unlock_time_2_in_normal_tx, configure_core);
}

bool hard_fork_1_unlock_time_2_in_normal_tx::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure etc_tx_details_unlock_time2 can be used in normal (non-coinbase) tx
  // before and after hardfork 1

  bool r = false;
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  generator.set_hardfork_height(m_hardfork_height);

  DO_CALLBACK(events, "configure_core");

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // before hardfork 1

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  CHECK_AND_ASSERT_MES(fill_tx_sources_and_destinations(events, blk_0r, miner_acc, alice_acc, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations), false, "");

  // set unlock_time_2
  std::vector<extra_v> extra;
  etc_tx_details_unlock_time2 ut2 = AUTO_VAL_INIT(ut2);
  ut2.unlock_time_array.resize(destinations.size());
  ut2.unlock_time_array[0] = 1; // not zero, unlocked from block 1
  extra.push_back(ut2);

  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  crypto::secret_key tx_sec_key;
  r = construct_tx(miner_acc.get_keys(), sources, destinations, extra, empty_attachment, tx_0, tx_sec_key, 0 /* unlock time 1 is zero and not set */);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_0);

  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));

  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0));

  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc); // hardfork should happen here
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);
  // make sure hardfork went okay
  CHECK_AND_ASSERT_MES(blk_2.major_version != CURRENT_BLOCK_MAJOR_VERSION && blk_3.major_version == CURRENT_BLOCK_MAJOR_VERSION, false, "hardfork did not happen as expected");


  // after hardfork 1

  sources.clear();
  destinations.clear();
  CHECK_AND_ASSERT_MES(fill_tx_sources_and_destinations(events, blk_3, miner_acc, alice_acc, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations), false, "");

  // set unlock_time_2
  extra.clear();
  ut2 = AUTO_VAL_INIT(ut2);
  ut2.unlock_time_array.resize(destinations.size());
  ut2.unlock_time_array[0] = 1; // not zero, unlocked from block 1
  extra.push_back(ut2);

  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, extra, empty_attachment, tx_1, tx_sec_key, 0 /* unlock time 1 is zero and not set */);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_1);

  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));

  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_3, miner_acc, tx_1);

  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0));

  return true;
}

bool hard_fork_1_unlock_time_2_in_normal_tx::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE;
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH;
  pc.hard_fork1_starts_after_height = m_hardfork_height;
  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

//------------------------------------------------------------------------------

hard_fork_1_unlock_time_2_in_coinbase::hard_fork_1_unlock_time_2_in_coinbase()
  :m_hardfork_height(3)
{
  REGISTER_CALLBACK_METHOD(hard_fork_1_unlock_time_2_in_coinbase, configure_core);
}

bool hard_fork_1_unlock_time_2_in_coinbase::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure etc_tx_details_unlock_time2 can be used in normal (non-coinbase) tx
  // before and after hardfork 1

  bool r = false;
  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  generator.set_hardfork_height(m_hardfork_height);

  DO_CALLBACK(events, "configure_core");

  
  // before hardfork 1
  
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_acc);
  events.pop_back(); // remove blk_1

  // remove etc_tx_details_unlock_time entries from miner_tx
  blk_1.miner_tx.extra.erase(std::remove_if(blk_1.miner_tx.extra.begin(), blk_1.miner_tx.extra.end(), [](auto& extra_element) { return extra_element.type() == typeid(etc_tx_details_unlock_time); }), blk_1.miner_tx.extra.end());

  // add etc_tx_details_unlock_time2 entry
  etc_tx_details_unlock_time2 ut2 = AUTO_VAL_INIT(ut2);
  ut2.unlock_time_array.resize(blk_1.miner_tx.vout.size());
  ut2.unlock_time_array[0] = get_block_height(blk_1) + CURRENCY_MINED_MONEY_UNLOCK_WINDOW;
  blk_1.miner_tx.extra.push_back(ut2);

  // add blk_1 with modified miner tx
  events.push_back(blk_1);
  generator.add_block_info(blk_1, std::list<transaction>()); // add modified block info

  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);  // hardfork should happen here
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_acc);
  // make sure hardfork went okay
  CHECK_AND_ASSERT_MES(blk_3.major_version != CURRENT_BLOCK_MAJOR_VERSION && blk_4.major_version == CURRENT_BLOCK_MAJOR_VERSION, false, "hardfork did not happen as expected");


  // after hardfork 1

  MAKE_NEXT_BLOCK(events, blk_5, blk_4, miner_acc);
  wide_difficulty_type diff = generator.get_block_difficulty(get_block_hash(blk_5));  // remember block difficulty for nonce searching after modification
  events.pop_back();

  // remove etc_tx_details_unlock_time entries from miner_tx
  blk_5.miner_tx.extra.erase(std::remove_if(blk_5.miner_tx.extra.begin(), blk_5.miner_tx.extra.end(), [](auto& extra_element) { return extra_element.type() == typeid(etc_tx_details_unlock_time); }), blk_5.miner_tx.extra.end());

  // add etc_tx_details_unlock_time2 entry
  ut2 = AUTO_VAL_INIT(ut2);
  ut2.unlock_time_array.resize(blk_5.miner_tx.vout.size());
  ut2.unlock_time_array[0] = get_block_height(blk_5) + CURRENCY_MINED_MONEY_UNLOCK_WINDOW;
  blk_5.miner_tx.extra.push_back(ut2);
  miner::find_nonce_for_given_block(blk_5, diff, get_block_height(blk_5));

  // add blk_5 with modified miner tx
  events.push_back(blk_5);
  generator.add_block_info(blk_5, std::list<transaction>()); // add modified block info

  MAKE_NEXT_BLOCK(events, blk_6, blk_5, miner_acc);

  return true;
}

bool hard_fork_1_unlock_time_2_in_coinbase::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE;
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH;
  pc.hard_fork1_starts_after_height = m_hardfork_height;
  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

