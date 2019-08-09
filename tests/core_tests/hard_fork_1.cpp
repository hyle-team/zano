// Copyright (c) 2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "hard_fork_1.h"
#include "pos_block_builder.h"
//#include "tx_builder.h"
//#include "random_helper.h"

using namespace currency;

hard_fork_1_base_test::hard_fork_1_base_test(size_t hardfork_height)
  : m_hardfork_height(hardfork_height)
{
  REGISTER_CALLBACK_METHOD(hard_fork_1_base_test, configure_core);
}

bool hard_fork_1_base_test::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE;
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH;
  pc.hard_fork1_starts_after_height = m_hardfork_height;
  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

//------------------------------------------------------------------------------


hard_fork_1_unlock_time_2_in_normal_tx::hard_fork_1_unlock_time_2_in_normal_tx()
  : hard_fork_1_base_test(12)
{
  REGISTER_CALLBACK_METHOD(hard_fork_1_unlock_time_2_in_normal_tx, configure_core);
}

bool hard_fork_1_unlock_time_2_in_normal_tx::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure etc_tx_details_unlock_time2 can be used in normal (non-coinbase) tx
  // only after hardfork 1

  bool r = false;
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  generator.set_hardfork_height(m_hardfork_height);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  //
  // before hardfork 1
  //

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
  r = construct_tx(miner_acc.get_keys(), sources, destinations, extra, empty_attachment, tx_0, tx_sec_key, 0 /* unlock time 1 is zero and thus will not be set */);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  // tx_0 should be accepted
  events.push_back(tx_0);

  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_1_bad, blk_0r, miner_acc, tx_0);
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));
  DO_CALLBACK(events, "clear_tx_pool");

  // make another tx with the same inputs and extra (tx_0 was rejected so inputs can be reused)
  transaction tx_0a = AUTO_VAL_INIT(tx_0a);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, extra, empty_attachment, tx_0a, tx_sec_key, 0 /* unlock time 1 is zero and thus will not be set */);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  // tx_0a should be accepted as well
  events.push_back(tx_0a);
  // make an alternative block with it and make sure it is rejected

  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, miner_acc);

  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_1_alt_bad, blk_0r, miner_acc, tx_0a);              // this alt block should be rejected because of tx_0a
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));
  DO_CALLBACK(events, "clear_tx_pool");


  // okay, go for a hardfork

  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc); // hardfork should happen here
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);
  // make sure hardfork went okay
  CHECK_AND_ASSERT_MES(blk_2.major_version != CURRENT_BLOCK_MAJOR_VERSION && blk_3.major_version == CURRENT_BLOCK_MAJOR_VERSION, false, "hardfork did not happen as expected");


  //
  // after hardfork 1
  //

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
  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_3, miner_acc, tx_1);                      // block with tx_1 should be accepted
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0));

  // do the same check for alt block
  sources.clear();
  destinations.clear();
  CHECK_AND_ASSERT_MES(fill_tx_sources_and_destinations(events, blk_4, miner_acc, alice_acc, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations), false, "");
  extra.clear();
  ut2 = AUTO_VAL_INIT(ut2);
  ut2.unlock_time_array.resize(destinations.size());
  ut2.unlock_time_array[0] = 1; // not zero, unlocked from block 1
  extra.push_back(ut2);
  
  transaction tx_1a = AUTO_VAL_INIT(tx_1a);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, extra, empty_attachment, tx_1a, tx_sec_key, 0 /* unlock time 1 is zero and not set */);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_1a);
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));

  MAKE_NEXT_BLOCK(events, blk_5, blk_4, miner_acc);

  MAKE_NEXT_BLOCK_TX1(events, blk_5a, blk_4, miner_acc, tx_1a);                    // alt block with tx_1a should be accepted
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));       // tx is still in the pool

  // switch chains
  MAKE_NEXT_BLOCK(events, blk_6a, blk_5a, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_7a, blk_6a, miner_acc);

  // make sure switching really happened
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_7a));

  // and tx_1a has gone
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0));

  return true;
}

//------------------------------------------------------------------------------

hard_fork_1_unlock_time_2_in_coinbase::hard_fork_1_unlock_time_2_in_coinbase()
  : hard_fork_1_base_test(3)
{
}

bool hard_fork_1_unlock_time_2_in_coinbase::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure etc_tx_details_unlock_time2 can be used in-coinbase txs
  // only after hardfork 1

  bool r = false;
  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  generator.set_hardfork_height(m_hardfork_height);

  DO_CALLBACK(events, "configure_core");

  
  // before hardfork 1
  
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_acc);
  events.pop_back(); // remove blk_1

  // remove etc_tx_details_unlock_time entries from miner_tx
  blk_1.miner_tx.extra.erase(std::remove_if(blk_1.miner_tx.extra.begin(), blk_1.miner_tx.extra.end(), [](extra_v& extra_element) { return extra_element.type() == typeid(etc_tx_details_unlock_time); }), blk_1.miner_tx.extra.end());

  // add etc_tx_details_unlock_time2 entry
  etc_tx_details_unlock_time2 ut2 = AUTO_VAL_INIT(ut2);
  ut2.unlock_time_array.resize(blk_1.miner_tx.vout.size());
  ut2.unlock_time_array[0] = get_block_height(blk_1) + CURRENCY_MINED_MONEY_UNLOCK_WINDOW;
  blk_1.miner_tx.extra.push_back(ut2);

  DO_CALLBACK(events, "mark_invalid_block");
  // add blk_1 with modified miner tx
  events.push_back(blk_1);
  generator.add_block_info(blk_1, std::list<transaction>()); // add modified block info

  MAKE_NEXT_BLOCK(events, blk_1a, blk_0, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_2, blk_1a, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);  // hardfork should happen here
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_acc);
  // make sure hardfork went okay
  CHECK_AND_ASSERT_MES(blk_3.major_version != CURRENT_BLOCK_MAJOR_VERSION && blk_4.major_version == CURRENT_BLOCK_MAJOR_VERSION, false, "hardfork did not happen as expected");


  // after hardfork 1

  MAKE_NEXT_BLOCK(events, blk_5, blk_4, miner_acc);
  wide_difficulty_type diff = generator.get_block_difficulty(get_block_hash(blk_5));  // remember block difficulty for nonce searching after modification
  events.pop_back();

  // remove etc_tx_details_unlock_time entries from miner_tx
  blk_5.miner_tx.extra.erase(std::remove_if(blk_5.miner_tx.extra.begin(), blk_5.miner_tx.extra.end(), [](extra_v& extra_element) { return extra_element.type() == typeid(etc_tx_details_unlock_time); }), blk_5.miner_tx.extra.end());

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

//------------------------------------------------------------------------------

hard_fork_1_chain_switch_pow_only::hard_fork_1_chain_switch_pow_only()
  : hard_fork_1_base_test(13)
{
}

bool hard_fork_1_chain_switch_pow_only::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure chain switches without PoS before and after hardfork

  bool r = false;
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  generator.set_hardfork_height(m_hardfork_height);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  //
  // before hardfork 1
  //

  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_1a, blk_0r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_2a, blk_1a, miner_acc);

  // make sure switch happened
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_2a));

  MAKE_NEXT_BLOCK(events, blk_3a, blk_2a, miner_acc); // hardfork should happen here
  MAKE_NEXT_BLOCK(events, blk_4a, blk_3a, miner_acc);
  // make sure hardfork went okay
  CHECK_AND_ASSERT_MES(blk_3a.major_version != CURRENT_BLOCK_MAJOR_VERSION && blk_4a.major_version == CURRENT_BLOCK_MAJOR_VERSION, false, "hardfork did not happen as expected");


  //
  // after hardfork 1
  //

  MAKE_NEXT_BLOCK(events, blk_5a, blk_4a, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_5b, blk_4a, miner_acc); // alternative chain B
  // switch chains
  MAKE_NEXT_BLOCK(events, blk_6b, blk_5b, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_7b, blk_6b, miner_acc);

  // make sure switch happened
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_7b));

  return true;
}

//------------------------------------------------------------------------------

hard_fork_1_checkpoint_basic_test::hard_fork_1_checkpoint_basic_test()
  : hard_fork_1_base_test(13)
  , checkpoints_test()
{
}

bool hard_fork_1_checkpoint_basic_test::generate(std::vector<test_event_entry>& events) const
{
  bool r = false;
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  generator.set_hardfork_height(m_hardfork_height);
  DO_CALLBACK(events, "configure_core");
  DO_CALLBACK_PARAMS(events, "set_checkpoint", params_checkpoint(CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2));
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  //
  // before hardfork 1
  //

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  CHECK_AND_ASSERT_MES(fill_tx_sources_and_destinations(events, blk_0r, miner_acc, alice_acc, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations), false, "");

  // set unlock_time_2, should be rejected before hardfork 1
  std::vector<extra_v> extra;
  etc_tx_details_unlock_time2 ut2 = AUTO_VAL_INIT(ut2);
  ut2.unlock_time_array.resize(destinations.size());
  ut2.unlock_time_array[0] = 1; // not zero, unlocked from block 1
  extra.push_back(ut2);
  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  crypto::secret_key tx_sec_key;
  r = construct_tx(miner_acc.get_keys(), sources, destinations, extra, empty_attachment, tx_0, tx_sec_key, 0 /* unlock time 1 is zero and thus will not be set */);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  // tx_0 should be accepted
  events.push_back(tx_0);

  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_1_bad, blk_0r, miner_acc, tx_0); // should be rejected because of tx_0
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));
  DO_CALLBACK(events, "clear_tx_pool");

  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);             // <-- checkpoint

  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);             // <-- hard fork
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_acc);
  // make sure hardfork went okay
  CHECK_AND_ASSERT_MES(blk_3.major_version != CURRENT_BLOCK_MAJOR_VERSION && blk_4.major_version == CURRENT_BLOCK_MAJOR_VERSION, false, "hardfork did not happen as expected");

  return true;
}

//------------------------------------------------------------------------------

hard_fork_1_pos_and_locked_coins::hard_fork_1_pos_and_locked_coins()
  : hard_fork_1_base_test(13) // hardfork height
  , m_unique_amount(TESTS_DEFAULT_FEE * 9)
{
  REGISTER_CALLBACK_METHOD(hard_fork_1_pos_and_locked_coins, check_outputs_with_unique_amount);
}

bool hard_fork_1_pos_and_locked_coins::generate(std::vector<test_event_entry>& events) const
{
  bool r = false;
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  generator.set_hardfork_height(m_hardfork_height);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  DO_CALLBACK_PARAMS(events, "check_outputs_with_unique_amount", static_cast<size_t>(0));

  // create few locked outputs in the blockchain with unique amount 
  std::vector<extra_v> extra;
  etc_tx_details_unlock_time ut = AUTO_VAL_INIT(ut);
  ut.v = 100; // locked until block 100
  extra.push_back(ut);

  std::vector<tx_destination_entry> destinations;
  for (size_t i = 0; i < 5; ++i)
    destinations.push_back(tx_destination_entry(m_unique_amount, alice_acc.get_public_address()));

  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  r = construct_tx_to_key(events, tx_0, blk_0r, miner_acc, destinations, TESTS_DEFAULT_FEE, 0, 0, extra);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_0);

  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  DO_CALLBACK_PARAMS(events, "check_outputs_with_unique_amount", static_cast<size_t>(5));


  block blk_0a;
  {
    crypto::hash prev_id = get_block_hash(blk_0);
    size_t height = get_block_height(blk_0) + 1;
    currency::wide_difficulty_type diff = generator.get_difficulty_for_next_block(prev_id, false);

    const transaction& stake = blk_0.miner_tx;
    crypto::public_key stake_tx_pub_key = get_tx_pub_key_from_extra(stake);
    size_t stake_output_idx = 0;
    size_t stake_output_gidx = 0;
    uint64_t stake_output_amount = stake.vout[stake_output_idx].amount;
    crypto::key_image stake_output_key_image;
    keypair kp;
    generate_key_image_helper(miner_acc.get_keys(), stake_tx_pub_key, stake_output_idx, kp, stake_output_key_image);
    crypto::public_key stake_output_pubkey = boost::get<txout_to_key>(stake.vout[stake_output_idx].target).key;

    pos_block_builder pb;
    pb.step1_init_header(height, prev_id);
    pb.step2_set_txs(std::vector<transaction>());
    pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, diff, prev_id, null_hash, blk_0r.timestamp);
    pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(blk_0r), miner_acc.get_public_address());
    pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, miner_acc);
    blk_0a = pb.m_block;
  }

  return true;
}

bool hard_fork_1_pos_and_locked_coins::check_outputs_with_unique_amount(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  size_t expected_outputs_count = 0;
  const std::string& params = boost::get<callback_entry>(events[ev_index]).callback_params;
  CHECK_AND_ASSERT_MES(epee::string_tools::hex_to_pod(params, expected_outputs_count), false, "hex_to_pod failed, params = " << params);

  std::list<crypto::public_key> pub_keys;
  bool r = c.get_outs(m_unique_amount, pub_keys);

  CHECK_AND_ASSERT_MES(r && pub_keys.size() == expected_outputs_count, false, "amount " << print_money_brief(m_unique_amount) << ": " << pub_keys.size() << " != " << expected_outputs_count);

  return true;
}
