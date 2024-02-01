// Copyright (c) 2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "hard_fork_1.h"
#include "pos_block_builder.h"
//#include "tx_builder.h"
//#include "random_helper.h"

using namespace currency;

template<typename extra_t>
void remove_unlock_v1_entries_from_extra(extra_t& extra)
{
  extra.erase(std::remove_if(extra.begin(), extra.end(), [](extra_v& extra_element) { return extra_element.type() == typeid(etc_tx_details_unlock_time); }), extra.end());
}

template<typename extra_t>
void remove_unlock_v2_entries_from_extra(extra_t& extra)
{
  extra.erase(std::remove_if(extra.begin(), extra.end(), [](extra_v& extra_element) { return extra_element.type() == typeid(etc_tx_details_unlock_time2); }), extra.end());
}

//------------------------------------------------------------------------------

hard_fork_1_base_test::hard_fork_1_base_test(size_t hardfork_height)
  : m_hardfork_height(hardfork_height)
{
  m_hardforks.m_height_the_hardfork_n_active_after[1] = 1440;
  m_hardforks.m_height_the_hardfork_n_active_after[2] = 1800;
  m_hardforks.m_height_the_hardfork_n_active_after[3] = 1801;
  m_hardforks.m_height_the_hardfork_n_active_after[4] = 50000000000;
  REGISTER_CALLBACK_METHOD(hard_fork_1_base_test, configure_core);
}

bool hard_fork_1_base_test::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE;
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH;
  pc.hard_forks.set_hardfork_height(1, m_hardfork_height);
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
  generator.set_hardfork_height(1, m_hardfork_height);
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
  r = construct_tx(miner_acc.get_keys(), sources, destinations, extra, empty_attachment, tx_0, get_tx_version_from_events(events), tx_sec_key, 0 /* unlock time 1 is zero and thus will not be set */);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  // tx_0 shouldn't be accepted
  DO_CALLBACK(events, "mark_invalid_tx"); 
  events.push_back(tx_0);

  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0));
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_1_bad, blk_0r, miner_acc, tx_0);
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0));
  DO_CALLBACK(events, "clear_tx_pool");

  // make another tx with the same inputs and extra (tx_0 was rejected so inputs can be reused)
  transaction tx_0a = AUTO_VAL_INIT(tx_0a);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, extra, empty_attachment, tx_0a, get_tx_version_from_events(events), tx_sec_key, 0 /* unlock time 1 is zero and thus will not be set */);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  // tx_0a shouldn't be accepted as well
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_0a);
  // make an alternative block with it and make sure it is rejected

  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, miner_acc);

  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0));
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_1_alt_bad, blk_0r, miner_acc, tx_0a);              // this alt block should be rejected because of tx_0a
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0));
  DO_CALLBACK(events, "clear_tx_pool");


  // okay, go for a hardfork

  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc); // hardfork should happen here
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);
  // make sure hardfork went okay
  CHECK_AND_ASSERT_MES(blk_2.major_version != HF1_BLOCK_MAJOR_VERSION && blk_3.major_version == HF1_BLOCK_MAJOR_VERSION, false, "hardfork did not happen as expected");
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(1));

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
  r = construct_tx(miner_acc.get_keys(), sources, destinations, extra, empty_attachment, tx_1, get_tx_version_from_events(events), tx_sec_key, 0 /* unlock time 1 is zero and not set */);
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
  r = construct_tx(miner_acc.get_keys(), sources, destinations, extra, empty_attachment, tx_1a, get_tx_version_from_events(events), tx_sec_key, 0 /* unlock time 1 is zero and not set */);
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

  //bool r = false;
  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  generator.set_hardfork_height(1, m_hardfork_height);

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
  CHECK_AND_ASSERT_MES(blk_3.major_version != HF1_BLOCK_MAJOR_VERSION && blk_4.major_version == HF1_BLOCK_MAJOR_VERSION, false, "hardfork did not happen as expected");
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(1));

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

  //bool r = false;
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  generator.set_hardfork_height(1, m_hardfork_height);
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
  CHECK_AND_ASSERT_MES(blk_3a.major_version != HF1_BLOCK_MAJOR_VERSION && blk_4a.major_version == HF1_BLOCK_MAJOR_VERSION, false, "hardfork did not happen as expected");
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(1));

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
  generator.set_hardfork_height(1, m_hardfork_height);
  DO_CALLBACK(events, "configure_core");
  DO_CALLBACK_PARAMS(events, "set_checkpoint", params_checkpoint(2 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 7));
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK(events, "check_being_in_cp_zone");                // make sure CP was has passed

  //
  // before hardfork 1
  //

  std::vector<tx_source_entry> sources;
  CHECK_AND_ASSERT_MES(fill_tx_sources(sources, events, blk_0r, miner_acc.get_keys(), MK_TEST_COINS(90) + TESTS_DEFAULT_FEE, 0), false, "");

  uint64_t stake_lock_time = 100; // locked till block 100

  std::vector<tx_destination_entry> destinations;
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(90), alice_acc.get_public_address()));

  // set unlock_time_2, should be rejected before hardfork 1
  std::vector<extra_v> extra;
  etc_tx_details_unlock_time2 ut2 = AUTO_VAL_INIT(ut2);
  ut2.unlock_time_array.resize(destinations.size());
  ut2.unlock_time_array[0] = stake_lock_time;
  extra.push_back(ut2);
  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  crypto::secret_key tx_sec_key;
  r = construct_tx(miner_acc.get_keys(), sources, destinations, extra, empty_attachment, tx_0, get_tx_version_from_events(events), tx_sec_key, 0 /* unlock time 1 is zero and thus will not be set */);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  // tx_0 should be accepted
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true)); // tx_0 goes with blk_1_bad
  events.push_back(tx_0);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));

  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_1_bad, blk_0r, miner_acc, tx_0); // should be rejected because of tx_0
  DO_CALLBACK(events, "check_tx_pool_empty"); // tx_0 won't be returned to the pool as it came with block blk_1_bad
  DO_CALLBACK(events, "clear_tx_pool");

  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);

  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);             // <-- hard fork
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_acc);
  // make sure hardfork went okay
  CHECK_AND_ASSERT_MES(blk_3.major_version != HF1_BLOCK_MAJOR_VERSION && blk_4.major_version == HF1_BLOCK_MAJOR_VERSION, false, "hardfork did not happen as expected");
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(1));

  //
  // after hardfork 1
  //

  // now tx_0 is okay and can be added to the blockchain
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true)); // tx_0 goes with blk_5
  events.push_back(tx_0);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));
  MAKE_NEXT_BLOCK_TX1(events, blk_5, blk_4, miner_acc, tx_0);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_5r, blk_5, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  CREATE_TEST_WALLET(alice_wlt, alice_acc, blk_0);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_5r, 2 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 5);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, MK_TEST_COINS(90));

  // try to mine a PoS block using locked coins
  block blk_6;
  {
    const block& prev_block = blk_5r;
    const transaction& stake = tx_0;
    const account_base& stakeholder = alice_acc;

    crypto::hash prev_id = get_block_hash(prev_block);
    size_t height = get_block_height(prev_block) + 1;
    currency::wide_difficulty_type diff = generator.get_difficulty_for_next_block(prev_id, false);

    crypto::public_key stake_tx_pub_key = get_tx_pub_key_from_extra(stake);
    size_t stake_output_idx = 0;
    size_t stake_output_gidx = 0;
    uint64_t stake_output_amount =boost::get<currency::tx_out_bare>( stake.vout[stake_output_idx]).amount;
    crypto::key_image stake_output_key_image;
    keypair kp;
    generate_key_image_helper(stakeholder.get_keys(), stake_tx_pub_key, stake_output_idx, kp, stake_output_key_image);
    crypto::public_key stake_output_pubkey = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(stake.vout[stake_output_idx]).target).key;

    pos_block_builder pb;
    pb.step1_init_header(generator.get_hardforks(), height, prev_id);
    pb.m_block.major_version = HF1_BLOCK_MAJOR_VERSION;
    pb.step2_set_txs(std::vector<transaction>());
    pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, diff, prev_id, null_hash, prev_block.timestamp);
    pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(prev_block), miner_acc.get_public_address(), stakeholder.get_public_address());

    // set etc_tx_details_unlock_time2 
    remove_unlock_v1_entries_from_extra(pb.m_block.miner_tx.extra); // clear already set unlock
    std::vector<extra_v> extra;
    etc_tx_details_unlock_time2 ut2 = AUTO_VAL_INIT(ut2);
    ut2.unlock_time_array.push_back(height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW); // reward lock
    for(size_t i = 0; i < pb.m_block.miner_tx.vout.size() - 1; ++i)
      ut2.unlock_time_array.push_back(stake_lock_time); // using the same lock time as stake input
    extra.push_back(ut2);
    pb.m_block.miner_tx.extra.push_back(ut2);

    pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, stakeholder);
    blk_6 = pb.m_block;
  }
  events.push_back(blk_6);
  generator.add_block_info(blk_6, std::list<transaction>()); // add modified block info


  MAKE_NEXT_BLOCK(events, blk_7, blk_6, miner_acc);             // <-- checkpoint
  MAKE_NEXT_BLOCK(events, blk_8, blk_7, miner_acc);

  DO_CALLBACK(events, "check_not_being_in_cp_zone");            // make sure CP was has passed

  REWIND_BLOCKS_N_WITH_TIME(events, blk_8r, blk_8, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // make sure locked Alice's coins still can't be spent
  sources.clear();
  r = fill_tx_sources(sources, events, blk_8r, alice_acc.get_keys(), MK_TEST_COINS(90), 0 /* nmix */, true /* check for spends */, false /* check for unlock time */);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(alice_acc.get_keys(), sources, std::vector<tx_destination_entry>{ tx_destination_entry(MK_TEST_COINS(90) - TESTS_DEFAULT_FEE, miner_acc.get_public_address()) },
    empty_attachment, tx_1, get_tx_version_from_events(events), stake_lock_time /* try to use stake unlock time -- should not work as it is not a coinbase */);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_1);

  // mine another PoS block using the same stake after a checkpoint
  std::list<currency::account_base> pos_stakeing_accounts{alice_acc};
  MAKE_NEXT_POS_BLOCK(events, blk_9, blk_8r, miner_acc, pos_stakeing_accounts);

  return true;
}

//------------------------------------------------------------------------------

struct unique_amount_params
{
  unique_amount_params(uint64_t amount, uint64_t count) : amount(amount), count(count) {}
  uint64_t amount;
  uint64_t count;
};

// check that the given amount has only one non-zero gidit
// 3000  => true
// 11000 => false
bool does_amount_have_one_non_zero_digit(uint64_t amount)
{
  size_t count = 0;
  auto f = [&count](uint64_t){ ++count; };
  decompose_amount_into_digits(amount, DEFAULT_DUST_THRESHOLD, f, f);
  return count == 1;
}


hard_fork_1_pos_and_locked_coins::hard_fork_1_pos_and_locked_coins()
  : hard_fork_1_base_test(25) // hardfork height
{
  REGISTER_CALLBACK_METHOD(hard_fork_1_pos_and_locked_coins, check_outputs_with_unique_amount);
}

bool hard_fork_1_pos_and_locked_coins::generate(std::vector<test_event_entry>& events) const
{
  random_state_test_restorer::reset_random();
  bool r = false;
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  GENERATE_ACCOUNT(bob_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, starter_timestamp);
  generator.set_hardfork_height(1, m_hardfork_height);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  const uint64_t unique_amount_alice = TESTS_DEFAULT_FEE * 9;
  const uint64_t unique_amount_bob   = TESTS_DEFAULT_FEE * 3;

  CHECK_AND_ASSERT_MES(does_amount_have_one_non_zero_digit(unique_amount_alice), false, "does_amount_have_one_non_zero_digit failed for Alice");
  CHECK_AND_ASSERT_MES(does_amount_have_one_non_zero_digit(unique_amount_bob), false, "does_amount_have_one_non_zero_digit failed for Bob");

  // make sure no outputs have such unique amounts
  DO_CALLBACK_PARAMS(events, "check_outputs_with_unique_amount", unique_amount_params(unique_amount_alice, 0) );
  DO_CALLBACK_PARAMS(events, "check_outputs_with_unique_amount", unique_amount_params(unique_amount_bob, 0) );

  // create few locked outputs in the blockchain with unique amount 
  // tx_0 : miner -> Alice (locked till block 100 using etc_tx_details_unlock_time)
  std::vector<extra_v> extra;
  etc_tx_details_unlock_time ut = AUTO_VAL_INIT(ut);
  ut.v = 100; // locked until block 100
  extra.push_back(ut);

  std::vector<tx_destination_entry> destinations;
  for (size_t i = 0; i < 5; ++i)
    destinations.push_back(tx_destination_entry(unique_amount_alice, alice_acc.get_public_address()));

  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  r = construct_tx_to_key(m_hardforks, events, tx_0, blk_0r, miner_acc, destinations, TESTS_DEFAULT_FEE, 0, 0, extra);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_0);

  // tx_1 : miner -> Bob (locked till block 100 using etc_tx_details_unlock_time2)
  extra.clear();
  uint64_t ut2_unlock_time = 100; // locked until block 100
  etc_tx_details_unlock_time2 ut2 = AUTO_VAL_INIT(ut2);
  destinations.clear();
  for (size_t i = 0; i < 5; ++i)
  {
    destinations.push_back(tx_destination_entry(unique_amount_bob, bob_acc.get_public_address()));
    ut2.unlock_time_array.push_back(ut2_unlock_time);
  }
  ut2.unlock_time_array.push_back(ut2_unlock_time);
  extra.push_back(ut2);
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx_to_key(m_hardforks, events, tx_1, blk_0r, miner_acc, destinations, TESTS_DEFAULT_FEE, 0, 0, extra);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_1);
  
  // etc_tx_details_unlock_time is allowed prior to HF 1 so tx_0 should pass all the checks nicely
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  // block with tx_1 should be rejected because etc_tx_details_unlock_time2 is not allowed prior to hardfork 1
  //DO_CALLBACK(events, "mark_invalid_block");
  //MAKE_NEXT_BLOCK_TX1(events, blk_1b, blk_1, miner_acc, tx_1);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // Alice should have received 5 * unique_amount_alice coins via tx_0
  DO_CALLBACK_PARAMS(events, "check_outputs_with_unique_amount", unique_amount_params(unique_amount_alice, 5) );

  // make sure outputs with m_unique_amount are still locked
  r = false;
  try
  {
    MAKE_TX(events, tx_1_bad, alice_acc, miner_acc, unique_amount_alice - TESTS_DEFAULT_FEE, blk_1r);
  }
  catch (std::runtime_error&)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "exception was not cought as expected");

  // try to make a PoS block with locked stake before the hardfork

  block blk_2b;
  {
    const block& prev_block = blk_1r;
    const transaction& stake = tx_0;

    crypto::hash prev_id = get_block_hash(prev_block);
    size_t height = get_block_height(prev_block) + 1;
    currency::wide_difficulty_type diff = generator.get_difficulty_for_next_block(prev_id, false);

    crypto::public_key stake_tx_pub_key = get_tx_pub_key_from_extra(stake);
    size_t stake_output_idx = 0;
    size_t stake_output_gidx = 0;
    uint64_t stake_output_amount =boost::get<currency::tx_out_bare>( stake.vout[stake_output_idx]).amount;
    crypto::key_image stake_output_key_image;
    keypair kp;
    generate_key_image_helper(alice_acc.get_keys(), stake_tx_pub_key, stake_output_idx, kp, stake_output_key_image);
    crypto::public_key stake_output_pubkey = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(stake.vout[stake_output_idx]).target).key;

    pos_block_builder pb;
    pb.step1_init_header(generator.get_hardforks(), height, prev_id);
    pb.step2_set_txs(std::vector<transaction>());
    pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, diff, prev_id, null_hash, prev_block.timestamp);
    pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(prev_block), miner_acc.get_public_address());
    pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, alice_acc);
    blk_2b = pb.m_block;
  }

  // it should not be accepted, because stake coins is still locked
  DO_CALLBACK(events, "mark_invalid_block");
  events.push_back(blk_2b);

  MAKE_NEXT_BLOCK(events, blk_2, blk_1r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);
  // make sure hardfork 1 went okay
  CHECK_AND_ASSERT_MES(blk_2.major_version != HF1_BLOCK_MAJOR_VERSION && blk_3.major_version == HF1_BLOCK_MAJOR_VERSION, false, "hardfork did not happen as expected");
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(1));

  // try to make a PoS block with locked stake after the hardfork

  block blk_4b;
  {
    const block& prev_block = blk_3;
    const transaction& stake = tx_0;

    crypto::hash prev_id = get_block_hash(prev_block);
    size_t height = get_block_height(prev_block) + 1;
    currency::wide_difficulty_type diff = generator.get_difficulty_for_next_block(prev_id, false);

    crypto::public_key stake_tx_pub_key = get_tx_pub_key_from_extra(stake);
    size_t stake_output_idx = 0;
    size_t stake_output_gidx = 0;
    uint64_t stake_output_amount =boost::get<currency::tx_out_bare>( stake.vout[stake_output_idx]).amount;
    crypto::key_image stake_output_key_image;
    keypair kp;
    generate_key_image_helper(alice_acc.get_keys(), stake_tx_pub_key, stake_output_idx, kp, stake_output_key_image);
    crypto::public_key stake_output_pubkey = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(stake.vout[stake_output_idx]).target).key;

    pos_block_builder pb;
    pb.step1_init_header(generator.get_hardforks(), height, prev_id);
    pb.m_block.major_version = HF1_BLOCK_MAJOR_VERSION;
    pb.step2_set_txs(std::vector<transaction>());
    pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, diff, prev_id, null_hash, prev_block.timestamp);
    pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(prev_block), miner_acc.get_public_address());
    pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, alice_acc);
    blk_4b = pb.m_block;
  }

  // it should not be accepted, because stake coins is still locked
  DO_CALLBACK(events, "mark_invalid_block");
  events.push_back(blk_4b);

  // blk_4 with tx_1 (etc_tx_details_unlock_time2) should be accepted after hardfork 1
  events.push_back(tx_1);
  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_3, miner_acc, tx_1);

  block prev = blk_4;
  for(size_t i = 0; i < CURRENCY_MINED_MONEY_UNLOCK_WINDOW; ++i)
  {
    MAKE_NEXT_POS_BLOCK(events, b, prev, miner_acc, std::list<currency::account_base>{miner_acc});
    prev = b;
    events.push_back(event_core_time(get_block_datetime(b) + 100));
  }


  //REWIND_BLOCKS_N_WITH_TIME(events, blk_4r, blk_4, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // try to make a PoS block with the stake locked using etc_tx_details_unlock_time2 (it's still locked)
  block blk_5;
  {
    const block& prev_block = prev;
    const transaction& stake = tx_1;

    crypto::hash prev_id = get_block_hash(prev_block);
    size_t height = get_block_height(prev_block) + 1;
    currency::wide_difficulty_type diff = generator.get_difficulty_for_next_block(prev_id, false);

    crypto::public_key stake_tx_pub_key = get_tx_pub_key_from_extra(stake);
    size_t stake_output_idx = 0;
    size_t stake_output_gidx = 0;
    uint64_t stake_output_amount =boost::get<currency::tx_out_bare>( stake.vout[stake_output_idx]).amount;
    crypto::key_image stake_output_key_image;
    keypair kp;
    generate_key_image_helper(bob_acc.get_keys(), stake_tx_pub_key, stake_output_idx, kp, stake_output_key_image);
    crypto::public_key stake_output_pubkey = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(stake.vout[stake_output_idx]).target).key;

    pos_block_builder pb;
    pb.step1_init_header(generator.get_hardforks(), height, prev_id);
    pb.m_block.major_version = HF1_BLOCK_MAJOR_VERSION;
    pb.step2_set_txs(std::vector<transaction>());
    pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, diff, prev_id, null_hash, prev_block.timestamp);
    pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(prev_block), miner_acc.get_public_address());
    pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, bob_acc);
    blk_5 = pb.m_block;
  }

  // it should not be accepted, because stake coins is still locked
  DO_CALLBACK(events, "mark_invalid_block");
  events.push_back(blk_5);

  return true;
}

bool hard_fork_1_pos_and_locked_coins::check_outputs_with_unique_amount(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  unique_amount_params p(0, 0);
  const std::string& params = boost::get<callback_entry>(events[ev_index]).callback_params;
  CHECK_AND_ASSERT_MES(epee::string_tools::hex_to_pod(params, p), false, "hex_to_pod failed, params = " << params);

  std::list<crypto::public_key> pub_keys;
  bool r = c.get_outs(p.amount, pub_keys);

  CHECK_AND_ASSERT_MES(r && pub_keys.size() == p.count, false, "amount " << print_money_brief(p.amount) << ": " << pub_keys.size() << " != " << p.count);

  return true;
}

//------------------------------------------------------------------------------

hard_fork_1_pos_locked_height_vs_time::hard_fork_1_pos_locked_height_vs_time()
  : hard_fork_1_base_test(11)
{
}

bool hard_fork_1_pos_locked_height_vs_time::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure it's impossible to use height-locked coins as PoS stake IF they are ts-locked (not height-locked) in some outputs
  // (because it could possibly be used to unlock coins eralier)

  bool r = false;
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  GENERATE_ACCOUNT(bob_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  generator.set_hardfork_height(1, m_hardfork_height);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  // create few locked outputs in the blockchain with unique amount 
  // tx_0 : miner -> Alice, miner -> Bob
  uint64_t stake_unlock_time = 100; // locked until block 100
  uint64_t stake_amount = MK_TEST_COINS(10);
  std::vector<extra_v> extra;
  etc_tx_details_unlock_time ut = AUTO_VAL_INIT(ut);
  ut.v = stake_unlock_time; 
  extra.push_back(ut);

  std::vector<tx_destination_entry> destinations;
  destinations.push_back(tx_destination_entry(stake_amount, alice_acc.get_public_address()));
  destinations.push_back(tx_destination_entry(stake_amount, bob_acc.get_public_address()));

  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  r = construct_tx_to_key(m_hardforks, events, tx_0, blk_0r, miner_acc, destinations, TESTS_DEFAULT_FEE, 0, 0, extra);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_0);

  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0); // first block after hardfork

  // make sure hardfork went okay
  CHECK_AND_ASSERT_MES(blk_0r.major_version != HF1_BLOCK_MAJOR_VERSION && blk_1.major_version == HF1_BLOCK_MAJOR_VERSION, false, "hardfork did not happen as expected");
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(1));

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  block blk_b1;
  {
    const block& prev_block = blk_1r;
    const transaction& stake = tx_0;
    const account_base& stakeholder = alice_acc;

    crypto::hash prev_id = get_block_hash(prev_block);
    size_t height = get_block_height(prev_block) + 1;
    currency::wide_difficulty_type diff = generator.get_difficulty_for_next_block(prev_id, false);

    crypto::public_key stake_tx_pub_key = get_tx_pub_key_from_extra(stake);
    size_t stake_output_idx = 0;
    size_t stake_output_gidx = 0;
    uint64_t stake_output_amount =boost::get<currency::tx_out_bare>( stake.vout[stake_output_idx]).amount;
    crypto::key_image stake_output_key_image;
    keypair kp;
    generate_key_image_helper(stakeholder.get_keys(), stake_tx_pub_key, stake_output_idx, kp, stake_output_key_image);
    crypto::public_key stake_output_pubkey = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(stake.vout[stake_output_idx]).target).key;

    pos_block_builder pb;
    pb.step1_init_header(generator.get_hardforks(), height, prev_id);
    pb.m_block.major_version = HF1_BLOCK_MAJOR_VERSION;
    pb.step2_set_txs(std::vector<transaction>());
    pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, diff, prev_id, null_hash, prev_block.timestamp);
    pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(prev_block), miner_acc.get_public_address(), stakeholder.get_public_address());

    // set etc_tx_details_unlock_time2 
    remove_unlock_v1_entries_from_extra(pb.m_block.miner_tx.extra); // clear already set unlock
    std::vector<extra_v> extra;
    etc_tx_details_unlock_time2 ut2 = AUTO_VAL_INIT(ut2);
    ut2.unlock_time_array.push_back(height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW); // reward lock
    for(size_t i = 0; i < pb.m_block.miner_tx.vout.size() - 1; ++i)
      ut2.unlock_time_array.push_back(test_core_time::get_time() - 1000); // stake locked by time and it's already passed
    extra.push_back(ut2);
    pb.m_block.miner_tx.extra.push_back(ut2);

    pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, stakeholder);
    blk_b1 = pb.m_block;
  }

  // should not pass as using time-locking in outputs
  DO_CALLBACK(events, "mark_invalid_block");
  events.push_back(blk_b1);


  block blk_b2;
  {
    const block& prev_block = blk_1r;
    const transaction& stake = tx_0;
    const account_base& stakeholder = alice_acc;

    crypto::hash prev_id = get_block_hash(prev_block);
    size_t height = get_block_height(prev_block) + 1;
    currency::wide_difficulty_type diff = generator.get_difficulty_for_next_block(prev_id, false);

    crypto::public_key stake_tx_pub_key = get_tx_pub_key_from_extra(stake);
    size_t stake_output_idx = 0;
    size_t stake_output_gidx = 0;
    uint64_t stake_output_amount =boost::get<currency::tx_out_bare>( stake.vout[stake_output_idx]).amount;
    crypto::key_image stake_output_key_image;
    keypair kp;
    generate_key_image_helper(stakeholder.get_keys(), stake_tx_pub_key, stake_output_idx, kp, stake_output_key_image);
    crypto::public_key stake_output_pubkey = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(stake.vout[stake_output_idx]).target).key;

    pos_block_builder pb;
    pb.step1_init_header(generator.get_hardforks(), height, prev_id);
    pb.m_block.major_version = HF1_BLOCK_MAJOR_VERSION;
    pb.step2_set_txs(std::vector<transaction>());
    pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, diff, prev_id, null_hash, prev_block.timestamp);
    pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(prev_block), miner_acc.get_public_address(), stakeholder.get_public_address());

    // set etc_tx_details_unlock_time2 
    remove_unlock_v1_entries_from_extra(pb.m_block.miner_tx.extra); // clear already set unlock
    std::vector<extra_v> extra;
    etc_tx_details_unlock_time2 ut2 = AUTO_VAL_INIT(ut2);
    ut2.unlock_time_array.push_back(height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW); // reward lock
    for(size_t i = 0; i < pb.m_block.miner_tx.vout.size() - 1; ++i)
      ut2.unlock_time_array.push_back(stake_unlock_time - 1); // stake locked by 1 less height that stake_unlock_time, that is incorrect as lock time of this coin is decreased
    extra.push_back(ut2);
    pb.m_block.miner_tx.extra.push_back(ut2);

    pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, stakeholder);
    blk_b2 = pb.m_block;
  }

  // should no pass because stake output has less lock time than stake input
  DO_CALLBACK(events, "mark_invalid_block");
  events.push_back(blk_b2);

  block blk_good;
  {
    const block& prev_block = blk_1r;
    const transaction& stake = tx_0;
    const account_base& stakeholder = alice_acc;

    crypto::hash prev_id = get_block_hash(prev_block);
    size_t height = get_block_height(prev_block) + 1;
    currency::wide_difficulty_type diff = generator.get_difficulty_for_next_block(prev_id, false);

    crypto::public_key stake_tx_pub_key = get_tx_pub_key_from_extra(stake);
    size_t stake_output_idx = 0;
    size_t stake_output_gidx = 0;
    uint64_t stake_output_amount =boost::get<currency::tx_out_bare>( stake.vout[stake_output_idx]).amount;
    crypto::key_image stake_output_key_image;
    keypair kp;
    generate_key_image_helper(stakeholder.get_keys(), stake_tx_pub_key, stake_output_idx, kp, stake_output_key_image);
    crypto::public_key stake_output_pubkey = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(stake.vout[stake_output_idx]).target).key;

    pos_block_builder pb;
    pb.step1_init_header(generator.get_hardforks(), height, prev_id);
    pb.m_block.major_version = HF1_BLOCK_MAJOR_VERSION;
    pb.step2_set_txs(std::vector<transaction>());
    pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, diff, prev_id, null_hash, prev_block.timestamp);
    pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(prev_block), miner_acc.get_public_address(), stakeholder.get_public_address());

    // set etc_tx_details_unlock_time2 
    remove_unlock_v1_entries_from_extra(pb.m_block.miner_tx.extra); // clear already set unlock
    std::vector<extra_v> extra;
    etc_tx_details_unlock_time2 ut2 = AUTO_VAL_INIT(ut2);
    ut2.unlock_time_array.push_back(height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW); // reward lock
    for(size_t i = 0; i < pb.m_block.miner_tx.vout.size() - 1; ++i)
      ut2.unlock_time_array.push_back(stake_unlock_time); // using the same lock time as stake input
    extra.push_back(ut2);
    pb.m_block.miner_tx.extra.push_back(ut2);

    pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, stakeholder);
    blk_good = pb.m_block;
  }

  // should pass okay
  events.push_back(blk_good);
  generator.add_block_info(blk_good, std::list<transaction>()); // add modified block info

  MAKE_NEXT_BLOCK(events, blk_2, blk_good, miner_acc);

  std::vector<tx_source_entry> sources;
  r = fill_tx_sources(sources, events, blk_2, alice_acc.get_keys(), stake_amount / 2 + TESTS_DEFAULT_FEE, 0 /* nmix */, true /* check for spends */, false /* check for unlock time */);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(alice_acc.get_keys(), sources, std::vector<tx_destination_entry>{ tx_destination_entry(stake_amount / 2, miner_acc.get_public_address()) },
    empty_attachment, tx_1, get_tx_version_from_events(events), stake_unlock_time /* try to use stake unlock time -- should not work as it is not a coinbase */);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_1);

  return true;
}
