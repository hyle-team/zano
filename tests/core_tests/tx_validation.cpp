// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "tx_validation.h"
#include "offers_tests_common.h"
#include "tx_builder.h"
#include "chaingen_helpers.h"

using namespace epee;
using namespace crypto;
using namespace currency;

//----------------------------------------------------------------------------------------------------------------------
// Tests

bool gen_tx_big_version::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0r, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

  tx_builder builder;
  builder.step1_init(CURRENT_TRANSACTION_VERSION + 1, 0);
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);
  builder.step4_calc_hash();
  builder.step5_sign(sources);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_unlock_time::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS_N(events, blk_1, blk_0, miner_account, 25);
  REWIND_BLOCKS(events, blk_1r, blk_1, miner_account);

  auto make_tx_with_unlock_time = [&](uint64_t unlock_time) -> transaction
  {
    return tx_builder::make_simple_tx_with_unlock_time(events, blk_1r, miner_account, miner_account, MK_TEST_COINS(1), unlock_time);
  };

  std::list<transaction> txs_0;

  txs_0.push_back(make_tx_with_unlock_time(0));
  events.push_back(txs_0.back());

  txs_0.push_back(make_tx_with_unlock_time(get_block_height(blk_1r) - 1));
  events.push_back(txs_0.back());

  txs_0.push_back(make_tx_with_unlock_time(get_block_height(blk_1r)));
  events.push_back(txs_0.back());

  txs_0.push_back(make_tx_with_unlock_time(get_block_height(blk_1r) + 1));
  events.push_back(txs_0.back());

  txs_0.push_back(make_tx_with_unlock_time(get_block_height(blk_1r) + 2));
  events.push_back(txs_0.back());

  txs_0.push_back(make_tx_with_unlock_time(ts_start - 1));
  events.push_back(txs_0.back());

  txs_0.push_back(make_tx_with_unlock_time(time(0) + 60 * 60));
  events.push_back(txs_0.back());

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1r, miner_account, txs_0);

  return true;
}

bool gen_tx_input_is_not_txin_to_key::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  MAKE_NEXT_BLOCK(events, blk_tmp, blk_0r, miner_account);
  events.pop_back();

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(blk_tmp.miner_tx);

  /*
  auto make_tx_with_input = [&](const txin_v& tx_input) -> transaction
  {
    std::vector<tx_source_entry> sources;
    std::vector<tx_destination_entry> destinations;
    fill_tx_sources_and_destinations(events, blk_0r, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

    tx_builder builder;
    builder.step1_init();
    builder.m_tx.vin.push_back(tx_input);
    builder.step3_fill_outputs(destinations);
    return builder.m_tx;
  };
  */

  DO_CALLBACK(events, "mark_invalid_tx");
  //fix script here
  //events.push_back(make_tx_with_input(txin_to_script()));

  DO_CALLBACK(events, "mark_invalid_tx");
  //TODO: fix script here
  //events.push_back(make_tx_with_input(txin_to_scripthash()));

  return true;
}

bool gen_tx_no_inputs_no_outputs::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);

  tx_builder builder;
  builder.step1_init();

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_no_inputs_has_outputs::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);


  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0r, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

  tx_builder builder;
  builder.step1_init();
  builder.step3_fill_outputs(destinations);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_has_inputs_no_outputs::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0r, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);
  destinations.clear();

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);
  builder.step4_calc_hash();
  builder.step5_sign(sources);

  events.push_back(builder.m_tx);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_account, builder.m_tx);

  return true;
}

bool gen_tx_invalid_input_amount::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  bool r = fill_tx_sources_and_destinations(events, blk_0r, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  sources.front().amount++;

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);
  builder.step4_calc_hash();
  builder.step5_sign(sources);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_input_wo_key_offsets::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0r, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);
  txin_to_key& in_to_key = boost::get<txin_to_key>(builder.m_tx.vin.front());
  uint64_t key_offset = boost::get<uint64_t>(in_to_key.key_offsets.front());
  in_to_key.key_offsets.pop_back();
  CHECK_AND_ASSERT_MES(in_to_key.key_offsets.empty(), false, "txin contained more than one key_offset");
  builder.step4_calc_hash();
  in_to_key.key_offsets.push_back(key_offset);
  builder.step5_sign(sources);
  in_to_key.key_offsets.pop_back();

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_key_offest_points_to_foreign_key::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);
  REWIND_BLOCKS(events, blk_1r, blk_1, miner_account);
  MAKE_ACCOUNT(events, alice_account);
  MAKE_ACCOUNT(events, bob_account);
  MAKE_TX_LIST_START(events, txs_0, miner_account, bob_account, MK_TEST_COINS(60) + 1, blk_1r);
  MAKE_TX_LIST(events, txs_0, miner_account, alice_account, MK_TEST_COINS(60) + 1, blk_1r);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1r, miner_account, txs_0);

  std::vector<tx_source_entry> sources_bob;
  std::vector<tx_destination_entry> destinations_bob;
  fill_tx_sources_and_destinations(events, blk_2, bob_account, miner_account, MK_TEST_COINS(60) + 1 - TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, 0, sources_bob, destinations_bob);

  std::vector<tx_source_entry> sources_alice;
  std::vector<tx_destination_entry> destinations_alice;
  fill_tx_sources_and_destinations(events, blk_2, alice_account, miner_account, MK_TEST_COINS(60) + 1 - TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, 0, sources_alice, destinations_alice);

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(bob_account.get_keys(), sources_bob);
  txin_to_key& in_to_key = boost::get<txin_to_key>(builder.m_tx.vin.front());
  in_to_key.key_offsets.front() = sources_alice.front().outputs.front().out_reference;
  builder.step3_fill_outputs(destinations_bob);
  builder.step4_calc_hash();
  builder.step5_sign(sources_bob);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_sender_key_offest_not_exist::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0r, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  txin_to_key& in_to_key = boost::get<txin_to_key>(builder.m_tx.vin.front());
  in_to_key.key_offsets.front() = std::numeric_limits<uint64_t>::max();
  builder.step3_fill_outputs(destinations);
  builder.step4_calc_hash();
  builder.step5_sign(sources);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_mixed_key_offest_not_exist::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);
  REWIND_BLOCKS(events, blk_1r, blk_1, miner_account);
  MAKE_ACCOUNT(events, alice_account);
  MAKE_ACCOUNT(events, bob_account);
  MAKE_TX_LIST_START(events, txs_0, miner_account, bob_account, MK_TEST_COINS(1) + TESTS_DEFAULT_FEE, blk_1r);
  MAKE_TX_LIST(events, txs_0, miner_account, alice_account, MK_TEST_COINS(1) + TESTS_DEFAULT_FEE, blk_1r);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1r, miner_account, txs_0);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_2, bob_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 1, sources, destinations);

  sources.front().outputs[(sources.front().real_output + 1) % 2].out_reference = std::numeric_limits<uint64_t>::max();

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(bob_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);
  builder.step4_calc_hash();
  builder.step5_sign(sources);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_key_image_not_derive_from_tx_key::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0r, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);

  txin_to_key& in_to_key = boost::get<txin_to_key>(builder.m_tx.vin.front());
  keypair kp = keypair::generate();
  key_image another_ki;
  crypto::generate_key_image(kp.pub, kp.sec, another_ki);
  in_to_key.k_image = another_ki;

  builder.step3_fill_outputs(destinations);
  builder.step4_calc_hash();

  // Tx with invalid key image can't be subscribed, so create empty signature
  builder.m_tx.signatures.resize(1);
  boost::get<currency::NLSAG_sig>(builder.m_tx.signatures[0]).s.resize(1);
  boost::get<currency::NLSAG_sig>(builder.m_tx.signatures[0]).s[0] = boost::value_initialized<crypto::signature>();

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_key_image_is_invalid::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0r, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);

  txin_to_key& in_to_key = boost::get<txin_to_key>(builder.m_tx.vin.front());
  crypto::public_key pub = tx_builder::generate_invalid_pub_key();
  memcpy(&in_to_key.k_image, &pub, sizeof(crypto::ec_point));

  builder.step3_fill_outputs(destinations);
  builder.step4_calc_hash();

  // Tx with invalid key image can't be subscribed, so create empty signature
  builder.m_tx.signatures.resize(1);
  boost::get<currency::NLSAG_sig>(builder.m_tx.signatures[0]).s.resize(1);
  boost::get<currency::NLSAG_sig>(builder.m_tx.signatures[0]).s[0] = boost::value_initialized<crypto::signature>();

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_check_input_unlock_time::generate(std::vector<test_event_entry>& events) const
{
  static const size_t tests_count = 6;

  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS_N(events, blk_1, blk_0, miner_account, tests_count - 1);
  REWIND_BLOCKS(events, blk_1r, blk_1, miner_account);

  std::array<account_base, tests_count> accounts;
  for (size_t i = 0; i < tests_count; ++i)
  {
    MAKE_ACCOUNT(events, acc);
    accounts[i] = acc;
  }

  std::list<transaction> txs_0;
  auto make_tx_to_acc = [&](size_t acc_idx, uint64_t unlock_time)
  {
    txs_0.push_back(tx_builder::make_simple_tx_with_unlock_time(events, blk_1r, miner_account, accounts[acc_idx],
      MK_TEST_COINS(1) + TESTS_DEFAULT_FEE, unlock_time));
    events.push_back(txs_0.back());
  };

  uint64_t blk_3_height = get_block_height(blk_1r) + 2;
  make_tx_to_acc(0, 0);
  make_tx_to_acc(1, blk_3_height - 1);
  make_tx_to_acc(2, blk_3_height);
  make_tx_to_acc(3, blk_3_height + 10 + 1);
  make_tx_to_acc(4, time(0) - 1);
  make_tx_to_acc(5, time(0) + 60 * 60);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1r, miner_account, txs_0);
  REWIND_BLOCKS(events, blk_3, blk_2, miner_account);

  std::list<transaction> txs_1;
  auto make_tx_from_acc = [&](size_t acc_idx, bool invalid)
  {
    transaction tx = tx_builder::make_simple_tx_with_unlock_time(events, blk_3, accounts[acc_idx], miner_account, MK_TEST_COINS(1), 0, false);
    if (invalid)
    {
      DO_CALLBACK(events, "mark_invalid_tx");
    }
    else
    {
      txs_1.push_back(tx);
    }
    events.push_back(tx);
    LOG_PRINT_L0("[gen_tx_check_input_unlock_time]make_tx_from_acc(" << acc_idx << ", " << invalid << ") as event[" << events.size() - 1 << "]");
  };

  make_tx_from_acc(0, false);
  make_tx_from_acc(1, false);
  make_tx_from_acc(2, false);
  make_tx_from_acc(3, true);
  make_tx_from_acc(4, false);
  make_tx_from_acc(5, true);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_4, blk_3, miner_account, txs_1);

  return true;
}

bool gen_tx_txout_to_key_has_invalid_key::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0r, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);

  txout_to_key& out_to_key =  boost::get<txout_to_key>(boost::get<tx_out_bare>(builder.m_tx.vout.front()).target);
  out_to_key.key = tx_builder::generate_invalid_pub_key();

  builder.step4_calc_hash();
  builder.step5_sign(sources);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_output_with_zero_amount::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0r, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);

  boost::get<tx_out_bare>(builder.m_tx.vout.front()).amount = 0;

  builder.step4_calc_hash();
  builder.step5_sign(sources);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_output_is_not_txout_to_key::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);

  return true;
  
  // this test is not needed anymore, to be deleted
  /*REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0r, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);

  builder.m_tx.vout.push_back(tx_out());
  builder.m_tx.vout.back().amount = 1;
  //TODO: fix test
  //builder.m_tx.vout.back().target = txout_to_script();

  builder.step4_calc_hash();
  builder.step5_sign(sources);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);

  builder.m_tx.vout.push_back(tx_out());
  builder.m_tx.vout.back().amount = 1;
  //TODO: fix test
  //builder.m_tx.vout.back().target = txout_to_scripthash();
  

  builder.step4_calc_hash();
  builder.step5_sign(sources);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
  */
}

void check_broken_tx(std::vector<test_event_entry>& events, const transaction& broken_tx, const block& head_block, const account_base& miner_acc, test_generator& generator)
{
  // 1. make sure broken_tx will be rejected by the tx_pool
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(broken_tx);
  
  // 2. make sure it will be rejected by blockchain_storage even if it is accepted by the pool
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block | event_visitor_settings::set_skip_txs_blobsize_check, true, true)); // to avoid many checks in the pool
  events.push_back(broken_tx);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block | event_visitor_settings::set_skip_txs_blobsize_check, false, false));
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, broken_block, head_block, miner_acc, broken_tx);

  // clear the pool of broken_tx
  DO_CALLBACK(events, "clear_tx_pool");
}

bool gen_tx_signatures_are_invalid::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(preminer_account);
  GENERATE_ACCOUNT(miner_account);
  GENERATE_ACCOUNT(alice_account);
  GENERATE_ACCOUNT(bob_account);
  GENERATE_ACCOUNT(carol_account);
  GENERATE_ACCOUNT(dan_account);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_account, test_core_time::get_time());
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);
  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_account, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  uint64_t amount = TESTS_DEFAULT_FEE * 80;

  // prepare needed outputs by transferring them to Carol and Dan
  MAKE_TX(events, tx_a, miner_account, carol_account, amount / 2, blk_1r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1r_a, blk_1r, miner_account, tx_a);
  MAKE_TX(events, tx_b, miner_account, carol_account, amount / 2, blk_1r_a);
  MAKE_NEXT_BLOCK_TX1(events, blk_1r_b, blk_1r_a, miner_account, tx_b);
  MAKE_TX(events, tx_c, miner_account, carol_account, amount / 2, blk_1r_b);
  MAKE_NEXT_BLOCK_TX1(events, blk_1r_c, blk_1r_b, miner_account, tx_c);
  MAKE_TX(events, tx_d, miner_account, dan_account, amount / 2, blk_1r_c);
  MAKE_NEXT_BLOCK_TX1(events, blk_1r_d, blk_1r_c, miner_account, tx_d);
  MAKE_TX(events, tx_e, miner_account, dan_account, amount / 2, blk_1r_d);
  MAKE_NEXT_BLOCK_TX1(events, blk_1r_e, blk_1r_d, miner_account, tx_e);
  MAKE_TX(events, tx_f, miner_account, dan_account, amount / 2, blk_1r_e);
  MAKE_NEXT_BLOCK_TX1(events, blk_1r_f, blk_1r_e, miner_account, tx_f);

  // create reference transaction tx_0, Carol->Alice, nmix=0
  MAKE_TX(events, tx_0, carol_account, alice_account, amount, blk_1r_f);
  events.pop_back();
  CHECK_AND_ASSERT_MES(tx_0.vin.size() > 1 && tx_0.vout.size() > 1 && tx_0.signatures.size() > 1, false, "tx_0 is incorrect"); // need > 1 for this test

  // create reference transaction tx_1, Dan->Alice, nmix=1
  MAKE_TX_MIX(events, tx_1, dan_account, alice_account, amount, 1, blk_1r_f);
  events.pop_back();
  CHECK_AND_ASSERT_MES(tx_1.vin.size() > 1 && tx_1.vout.size() > 1 && tx_1.signatures.size() > 1, false, "tx_1 is incorrect"); // need > 1 for this test

  const block& prev_block = blk_1r_f;
  transaction broken_tx;

  // Tx with nmix = 0 without signatures
  broken_tx = tx_0;
  broken_tx.signatures.clear();
  check_broken_tx(events, broken_tx, prev_block, miner_account, generator);

  // Tx with nmix = 0 have a few inputs, and not enough signatures
  broken_tx = tx_0;
  broken_tx.signatures.resize(broken_tx.signatures.size() - 1);
  check_broken_tx(events, broken_tx, prev_block, miner_account, generator);

  // Tx with nmix = 0 have a few inputs, and too many signatures (1/2)
  broken_tx = tx_0;
  boost::get<currency::NLSAG_sig>(broken_tx.signatures.back()).s.push_back(invalid_signature);
  check_broken_tx(events, broken_tx, prev_block, miner_account, generator);

  // Tx with nmix = 0 have a few inputs, and too many signatures (2/2)
  broken_tx = tx_0;
  broken_tx.signatures.push_back(currency::NLSAG_sig());
  boost::get<currency::NLSAG_sig>(broken_tx.signatures.back()).s.push_back(invalid_signature);
  check_broken_tx(events, broken_tx, prev_block, miner_account, generator);


  // Tx with nmix = 1 without signatures
  broken_tx = tx_1;
  broken_tx.signatures.clear();
  check_broken_tx(events, broken_tx, prev_block, miner_account, generator);

  // Tx with nmix = 1 have not enough signatures (1/2)
  broken_tx = tx_1;
  broken_tx.signatures.resize(broken_tx.signatures.size() - 1);
  check_broken_tx(events, broken_tx, prev_block, miner_account, generator);

  // Tx with nmix = 1 have not enough signatures (2/2)
  broken_tx = tx_1;
  boost::get<currency::NLSAG_sig>(broken_tx.signatures.back()).s.resize(boost::get<currency::NLSAG_sig>(broken_tx.signatures.back()).s.size() - 1);
  check_broken_tx(events, broken_tx, prev_block, miner_account, generator);

  // Tx with nmix = 1 have too many signatures (1/2)
  broken_tx = tx_1;
  boost::get<currency::NLSAG_sig>(broken_tx.signatures.back()).s.push_back(invalid_signature);
  check_broken_tx(events, broken_tx, prev_block, miner_account, generator);

  // Tx with nmix = 1 have too many signatures (2/2)
  broken_tx = tx_1;
  broken_tx.signatures.push_back(currency::NLSAG_sig());
  boost::get<currency::NLSAG_sig>(broken_tx.signatures.back()).s.push_back(invalid_signature);
  check_broken_tx(events, broken_tx, prev_block, miner_account, generator);


  // Finally check that the original transactions are accepted and passed
  events.push_back(tx_0);
  events.push_back(tx_1);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3, prev_block, miner_account, std::list<transaction>({ tx_0, tx_1 }));

  // spend all Alice's money to make sure she has successfully received it with 'tx'
  MAKE_TX(events, tx_2, alice_account, bob_account, amount * 2 - TESTS_DEFAULT_FEE, blk_3);
  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_3, miner_account, tx_2);

  return true;
}

void fill_test_offer_(bc_services::offer_details& od)
{
  od.offer_type = OFFER_TYPE_PRIMARY_TO_TARGET;
  od.amount_primary = 1000000000;
  od.amount_target = 22222222;
  od.target = "USD";
  od.location_country = "US";
  od.location_city = "New York City";
  od.contacts = "skype: zina; icq: 12313212; email: zina@zina.com; mobile: +621234567834";
  od.comment = "The best ever rate in NYC!!!";
  od.payment_types = "BTC;BANK;CASH";
  od.expiration_time = 10;
}
bool gen_broken_attachments::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;
  GENERATE_ACCOUNT(miner_account);
  //
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_account);
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_account);
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_account);
  REWIND_BLOCKS(events, blk_5, blk_4, miner_account);
  REWIND_BLOCKS_N(events, blk_5r, blk_5, miner_account, 20);


  bc_services::offer_details od = AUTO_VAL_INIT(od);
  fill_test_offer_(od);
  std::vector<currency::attachment_v> attachments;
  bc_services::put_offer_into_attachment(od, attachments);

  std::list<currency::transaction> txs_set;
  DO_CALLBACK(events, "mark_invalid_tx");
  construct_broken_tx(txs_set, events, miner_account, miner_account, blk_5r, attachments, [](transaction& tx)
  {
    //don't put attachments info into 
    return true;
  });

  DO_CALLBACK(events, "mark_invalid_tx");
  construct_broken_tx(txs_set, events, miner_account, miner_account, blk_5r, std::vector<currency::attachment_v>(), [&](transaction& tx)
  {
    extra_attachment_info eai = extra_attachment_info();

    //put hash into extra
    std::stringstream ss;
    binary_archive<true> oar(ss);
    bool r = ::do_serialize(oar, const_cast<std::vector<attachment_v>&>(attachments));
    CHECK_AND_ASSERT_MES(r, false, "do_serialize failed");
    std::string buff = ss.str();
    eai.sz = buff.size();
    eai.hsh = get_blob_hash(buff);
    tx.extra.push_back(eai);
    return true;
  });


  return true;
}

//------------------------------------------------------------------------------

gen_crypted_attachments::gen_crypted_attachments()
{
  REGISTER_CALLBACK("check_crypted_tx", gen_crypted_attachments::check_crypted_tx);
  REGISTER_CALLBACK("set_blockchain_height", gen_crypted_attachments::set_blockchain_height);
  REGISTER_CALLBACK("set_crypted_tx_height", gen_crypted_attachments::set_crypted_tx_height);
  REGISTER_CALLBACK("check_offers_count_befor_cancel", gen_crypted_attachments::check_offers_count_befor_cancel);
  REGISTER_CALLBACK("check_offers_count_after_cancel", gen_crypted_attachments::check_offers_count_after_cancel);

  m_hardforks.set_hardfork_height(1, 0);
  m_hardforks.set_hardfork_height(2, 0); // tx_payer is allowed only after HF2
}

bool gen_crypted_attachments::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;
  GENERATE_ACCOUNT(miner_account);
  //
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  set_hard_fork_heights_to_generator(generator);
  DO_CALLBACK(events, "configure_core");
  MAKE_ACCOUNT(events, bob_account);

  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);
  MAKE_NEXT_BLOCK(events, blk_4, blk_1, miner_account);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_5, blk_4, miner_account, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  DO_CALLBACK(events, "set_blockchain_height");

  pr.acc_addr = miner_account.get_keys().account_address;
  cm.comment = "Comandante Che Guevara";
  //ms.msg = "Hasta Siempre, Comandante";

  std::vector<currency::attachment_v> attachments;
  attachments.push_back(pr);
  attachments.push_back(cm);
  //attachments.push_back(ms);

  MAKE_TX_LIST_START(events, txs_list, miner_account, bob_account, MK_TEST_COINS(1), blk_5);
  MAKE_TX_LIST_ATTACH(events, txs_list, miner_account, bob_account, MK_TEST_COINS(1), blk_5, attachments);
  DO_CALLBACK(events, "set_crypted_tx_height");
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_6, blk_5, miner_account, txs_list);
  DO_CALLBACK(events, "check_crypted_tx");

  MAKE_TX_LIST_START(events, txs_list2, miner_account, bob_account, MK_TEST_COINS(1), blk_6);
  
  //create two offers
  currency::transaction tx;
  std::vector<currency::attachment_v> attachments2;
  bc_services::offer_details od = AUTO_VAL_INIT(od);
  fill_test_offer_(od);
  bc_services::put_offer_into_attachment(od, attachments2);
  bc_services::put_offer_into_attachment(od, attachments2);
  construct_tx_to_key(m_hardforks, events, tx, blk_6, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, CURRENCY_TO_KEY_OUT_RELAXED, std::vector<currency::extra_v>(), attachments2);
  txs_list2.push_back(tx);
  events.push_back(tx);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_7, blk_6, miner_account, txs_list2);
  DO_CALLBACK(events, "check_offers_count_befor_cancel");
  
  //create two cancel offers
  MAKE_TX_LIST_START(events, txs_list3, miner_account, bob_account, MK_TEST_COINS(1), blk_7);
  std::vector<currency::attachment_v> attachments3;
  bc_services::cancel_offer co = AUTO_VAL_INIT(co);
  co.tx_id = get_transaction_hash(tx);
  co.offer_index = 0;

  crypto::public_key related_tx_pub_key = get_tx_pub_key_from_extra(tx);
  crypto::public_key related_tx_offer_pub_key = bc_services::get_offer_secure_key_by_index_from_tx(tx, 0);
  currency::keypair ephemeral;
  bool r = currency::derive_ephemeral_key_helper(miner_account.get_keys(), related_tx_pub_key, 0, ephemeral);
  CHECK_AND_ASSERT_THROW_MES(r, "Failed to derive_ephemeral_key_helper");
  CHECK_AND_ASSERT_THROW_MES(ephemeral.pub == related_tx_offer_pub_key, "Failed to derive_ephemeral_key_helper(pub keys missmatch)");

  blobdata bl_for_sig = bc_services::make_offer_sig_blob(co);
  crypto::generate_signature(crypto::cn_fast_hash(bl_for_sig.data(), bl_for_sig.size()), ephemeral.pub, ephemeral.sec, co.sig);

  //  blobdata bl_for_sig = currency::make_offer_sig_blob(co);
//  crypto::generate_signature(crypto::cn_fast_hash(bl_for_sig.data(), bl_for_sig.size()), 
//                             currency::get_tx_pub_key_from_extra(tx), 
//                             off_key_pair.sec, co.sig);



  bc_services::put_offer_into_attachment(co, attachments3);
  co.offer_index = 1;

  crypto::public_key related_tx_pub_key_1 = get_tx_pub_key_from_extra(tx);
  crypto::public_key related_tx_offer_pub_key_1 = bc_services::get_offer_secure_key_by_index_from_tx(tx, 1);
  currency::keypair ephemeral_1;
  r = currency::derive_ephemeral_key_helper(miner_account.get_keys(), related_tx_pub_key_1, 1, ephemeral_1);
  CHECK_AND_ASSERT_THROW_MES(r, "Failed to derive_ephemeral_key_helper");
  CHECK_AND_ASSERT_THROW_MES(ephemeral_1.pub == related_tx_offer_pub_key_1, "Failed to derive_ephemeral_key_helper(pub keys missmatch)");

  bl_for_sig = bc_services::make_offer_sig_blob(co);
  crypto::generate_signature(crypto::cn_fast_hash(bl_for_sig.data(), bl_for_sig.size()), ephemeral_1.pub, ephemeral_1.sec, co.sig);


//   bl_for_sig = currency::make_offer_sig_blob(co);
//   crypto::generate_signature(crypto::cn_fast_hash(bl_for_sig.data(), bl_for_sig.size()), 
//                              currency::get_tx_pub_key_from_extra(tx), 
//                              off_key_pair.sec, co.sig);
  bc_services::put_offer_into_attachment(co, attachments3);
  construct_tx_to_key(m_hardforks, events, tx, blk_7, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, CURRENCY_TO_KEY_OUT_RELAXED, std::vector<currency::extra_v>(), attachments3);
  txs_list3.push_back(tx);
  events.push_back(tx);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_8, blk_7, miner_account, txs_list3);
  DO_CALLBACK(events, "check_offers_count_after_cancel");
  return true;
}

bool gen_crypted_attachments::set_blockchain_height(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
{
  bc_height_before = c.get_current_blockchain_size();
  return true;
}
bool gen_crypted_attachments::set_crypted_tx_height(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  crypted_tx_height = ev_index - 1;
  return true;
}
bool gen_crypted_attachments::check_offers_count_befor_cancel(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
{
  std::list<bc_services::offer_details_ex> od;
  get_all_offers(c, od);
  offers_before_canel = od.size();
  return true;
}
bool gen_crypted_attachments::check_offers_count_after_cancel(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
{ 
  std::list<bc_services::offer_details_ex> od;
  get_all_offers(c, od);
  CHECK_EQ(offers_before_canel - 2, od.size());
  return true;
}
bool gen_crypted_attachments::check_crypted_tx(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{

  //// check cha cha
  std::string test_key = "ssssss";
  std::vector<std::string> test_array;
  test_array.push_back("qweqweqwqw");
  test_array.push_back("4g45");
  test_array.push_back("qweqwe56575qwqw");
  test_array.push_back("q3f34f4");
  std::vector<std::string> test_array2 = test_array;

  for (auto& v : test_array2)
    chacha_crypt(v, test_key);

  for (auto& v : test_array2)
    chacha_crypt(v, test_key);

  if (test_array2 != test_array)
  {
    LOG_ERROR("cha cha broken");
    return false;
  }

  //


  const currency::transaction& tx = boost::get<currency::transaction>(events[crypted_tx_height]);
  const currency::account_base& bob_acc = boost::get<currency::account_base>(events[2]);

  CHECK_EQ(c.get_current_blockchain_size(), bc_height_before+1);

  auto ptx_from_bc = c.get_blockchain_storage().get_tx(currency::get_transaction_hash(tx));
  CHECK_TEST_CONDITION(ptx_from_bc );

  std::vector<payload_items_v> at;
  bool r = currency::decrypt_payload_items(true, *ptx_from_bc, bob_acc.get_keys(), at);
  CHECK_EQ(r, true);
  if (at.size() != 16)
  {
    std::stringstream ss;
    for(auto& el : at)
      ss << "    " << el.type().name() << ENDL;
    LOG_PRINT_RED("at.size() = " << at.size() << " : " << ENDL << ss.str(), LOG_LEVEL_0);

    // expected items:
    //   public_key
    //   etc_tx_flags16_t
    //   tx_derivation_hint     x 10
    //   extra_attachment_info
    //   tx_payer
    //   tx_comment
    //   tx_crypto_checksum
    //
    // total: 16
    CHECK_AND_ASSERT_MES(false, false, "unexpected number of decrypted items");
  }

  currency::tx_payer decrypted_pr = AUTO_VAL_INIT(decrypted_pr);
  r = get_type_in_variant_container(at, decrypted_pr);
  CHECK_AND_ASSERT_MES(r, false, "get_type_in_variant_container failed");
  if (pr.acc_addr != decrypted_pr.acc_addr)
  {
    LOG_ERROR("decrypted wrong data: " 
      << epee::string_tools::pod_to_hex(decrypted_pr.acc_addr)
      << "expected:" << epee::string_tools::pod_to_hex(pr.acc_addr));
    return false;
  }

  currency::tx_comment decrypted_comment = AUTO_VAL_INIT(decrypted_comment);
  r = get_type_in_variant_container(at, decrypted_comment);
  CHECK_AND_ASSERT_MES(r, false, "get_type_in_variant_container failed");
  if (cm.comment != decrypted_comment.comment)
  {
    LOG_ERROR("decrypted wrong data: "
      << decrypted_comment.comment
      << "expected:" << cm.comment);
    return false;
  }

//   currency::tx_message decrypted_message = AUTO_VAL_INIT(decrypted_message);
//   r = get_type_in_variant_container(at, decrypted_message);
//   CHECK_AND_ASSERT_MES(r, false, "get_type_in_variant_container failed");
//   if (ms.msg != decrypted_message.msg)
//   {
//     LOG_ERROR("decrypted wrong data: "
//       << decrypted_message.msg
//       << "expected:" << ms.msg);
//     return false;
//   }

  // now cancel attachment

  return true;
}

gen_tx_extra_double_entry::gen_tx_extra_double_entry()
{
  REGISTER_CALLBACK_METHOD(gen_tx_extra_double_entry, configure_core);
}

bool gen_tx_extra_double_entry::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE;
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH;
  pc.hard_forks.set_hardfork_height(1, 0);
  pc.hard_forks.set_hardfork_height(2, 0);
  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

bool gen_tx_extra_double_entry::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1450000000;
  bool r = false;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  generator.set_hardfork_height(1, 0);
  generator.set_hardfork_height(2, 0); // extra_alias_entry is only allowed after HF2, so switch it on here
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_account, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");

  tx_builder builder;

  // 1. double pubkey
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);
  add_tx_pub_key_to_extra(builder.m_tx, builder.m_tx_key.pub); // add redundant second pubkey to tx extra
  builder.step4_calc_hash();
  builder.step5_sign(sources);
  // EXPECTED: tx and blk_1 are rejected
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_account, builder.m_tx);

  
  // 2. double attachment
  bc_services::offer_details od = AUTO_VAL_INIT(od);
  fill_test_offer_(od);
  //std::vector<currency::attachment_v> attachments;

  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);
  bc_services::put_offer_into_attachment(od, builder.m_tx.attachment);
  add_attachments_info_to_extra(builder.m_tx.extra, builder.m_tx.attachment);
  add_attachments_info_to_extra(builder.m_tx.extra, builder.m_tx.attachment); // two attachment entry
  builder.step4_calc_hash();
  builder.step5_sign(sources);

  // EXPECTED: tx and blk_2 are rejected
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_0r, miner_account, builder.m_tx);


  // 3. double alias
  extra_alias_entry ai;
  ai.m_alias = "some-alias";
  ai.m_address = miner_account.get_public_address();
  ai.m_text_comment = "Mandame unas monedas, por favor!";
  uint64_t alias_reward = currency::get_alias_coast_from_fee(ai.m_alias, TESTS_DEFAULT_FEE);
  sources.clear();
  destinations.clear();
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE + alias_reward, 0, sources, destinations);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  tx_destination_entry de;
  de.addr.push_back(null_pub_addr);
  de.amount = alias_reward;
  destinations.push_back(de);

  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);
  add_tx_extra_alias(builder.m_tx, ai);
  add_tx_extra_alias(builder.m_tx, ai); // second alias entry in the extra
  builder.step4_calc_hash();
  builder.step5_sign(sources);

  // EXPECTED: tx and blk_3 are rejected
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_0r, miner_account, builder.m_tx);

  // 4. double extra_user_data
  sources.clear();
  destinations.clear();
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);
  extra_user_data eud;
  eud.buff = std::string(2049, '$');
  builder.m_tx.extra.push_back(eud);
  builder.m_tx.extra.push_back(eud); // second extra user data entry
  builder.step4_calc_hash();
  builder.step5_sign(sources);

  // EXPECTED: tx and blk_4 are rejected
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_0r, miner_account, builder.m_tx);


  // 5. double extra_padding
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);
  extra_padding padding;
  padding.buff.resize(2049);
  builder.m_tx.extra.push_back(padding);
  builder.m_tx.extra.push_back(padding); // extra_padding entry
  builder.step4_calc_hash();
  builder.step5_sign(sources);

  // EXPECTED: tx and blk_5 are rejected
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_5, blk_0r, miner_account, builder.m_tx);


  // 6. each entry is added only once
  ai = AUTO_VAL_INIT(ai);
  ai.m_alias = "uwndjckforptifmslf";
  ai.m_address = miner_account.get_public_address();
  ai.m_text_comment = "Mandame unas monedas, por favor!";

  sources.clear();
  destinations.clear();
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE + alias_reward, 0, sources, destinations);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  de = AUTO_VAL_INIT(de);
  de.addr.push_back(currency::account_public_address());
  de.amount = alias_reward;
  destinations.push_back(de);

  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);
  bc_services::put_offer_into_attachment(od, builder.m_tx.attachment);
  add_attachments_info_to_extra(builder.m_tx.extra, builder.m_tx.attachment); // attachment
  add_tx_extra_alias(builder.m_tx, ai); // alias
  builder.m_tx.extra.push_back(eud); // user data
  builder.m_tx.extra.push_back(padding); // padding
  builder.step4_calc_hash();
  builder.step5_sign(sources);

  // EXPECTED: tx and blk_6 are accepted
  events.push_back(builder.m_tx);
  MAKE_NEXT_BLOCK_TX1(events, blk_65, blk_0r, miner_account, builder.m_tx);

  return true;
}

bool gen_tx_double_key_image::generate(std::vector<test_event_entry>& events) const
{
  // Make a tx with correct, but used more then once key_image in inputs. Should be invalid tx.
  uint64_t ts_start = test_core_time::get_time();

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_account, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0r, miner_account, miner_account, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

  sources.push_back(sources.front()); // dobule source entry => double imput => double key image

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);
  builder.step4_calc_hash();
  builder.step5_sign(sources);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

//------------------------------------------------------------------

bool tx_expiration_time::generate(std::vector<test_event_entry>& events) const
{
  bool r = false;
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  GENERATE_ACCOUNT(bob_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // transfer to Alice some coins in chunks
  uint64_t alice_amount = MK_TEST_COINS(10);
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), alice_amount, 10, TESTS_DEFAULT_FEE, tx_1);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_1);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);

  // check Alice's balance
  std::shared_ptr<tools::wallet2> alice_wlt;
  r = generator.init_test_wallet(alice_acc, get_block_hash(blk_0), alice_wlt);
  CHECK_AND_ASSERT_MES(r, false, "init_test_wallet failed");
  r = generator.refresh_test_wallet(events, alice_wlt.get(), get_block_hash(blk_2), CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2);
  CHECK_AND_ASSERT_MES(r, false, "refresh_test_wallet failed");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "alice", alice_amount, 0, 0, 0, 0), false, "");

  uint64_t ts_median = generator.get_timestamps_median(get_block_hash(blk_2), TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW);
  LOG_PRINT_YELLOW("%%%%%%% ts_median = " << ts_median, LOG_LEVEL_0);

  /// core tx expiration condition:
  /// expiration_time - TX_EXPIRATION_MEDIAN_SHIFT >  get_last_n_blocks_timestamps_median(TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW)

  // 1/3
  // create normal tx, then add expiration time entry to the extra and then resign
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  r = fill_tx_sources_and_destinations(events, blk_2, alice_acc.get_keys(), bob_acc.get_public_address(), MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  transaction tx_2  = AUTO_VAL_INIT(tx_2);
  r = construct_tx(alice_acc.get_keys(), sources, destinations, empty_attachment, tx_2, get_tx_version_from_events(events),  0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  set_tx_expiration_time(tx_2, ts_median + TX_EXPIRATION_MEDIAN_SHIFT + 1); // one second greather than minimum allowed
  r = resign_tx(alice_acc.get_keys(), sources, tx_2);
  CHECK_AND_ASSERT_MES(r, false, "resign_tx failed");
  
  // should pass normally
  events.push_back(tx_2);
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_2);

  // 2/3
  // normal tx with expiration time entry equal to median minus shift - should not pass at tx_pool
  ts_median = generator.get_timestamps_median(get_block_hash(blk_3), TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW);
  sources.clear();
  destinations.clear();
  r = fill_tx_sources_and_destinations(events, blk_3, alice_acc.get_keys(), bob_acc.get_public_address(), MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  transaction tx_3  = AUTO_VAL_INIT(tx_3);
  r = construct_tx(alice_acc.get_keys(), sources, destinations, empty_attachment, tx_3, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  set_tx_expiration_time(tx_3, ts_median + TX_EXPIRATION_MEDIAN_SHIFT + 0); // exact expiration time, should not pass (see core condition above)
  r = resign_tx(alice_acc.get_keys(), sources, tx_3);
  CHECK_AND_ASSERT_MES(r, false, "resign_tx failed");

  // should be rejected at tx_pool
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_3);


  // 3/3
  // normal tx with expiration time entry equal to median minus shift - should not pass at the core

  // create a new block, but do not add it to the events vector for a while
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_acc);
  events.pop_back();

  // update median
  ts_median = generator.get_timestamps_median(get_block_hash(blk_4), TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW);
  // prepare transaction - use tx_3 change it's expiration time and resign 
  set_tx_expiration_time(tx_3, ts_median + TX_EXPIRATION_MEDIAN_SHIFT + 0); // exact expiration time, should not pass (see core condition above)
  r = resign_tx(alice_acc.get_keys(), sources, tx_3);
  CHECK_AND_ASSERT_MES(r, false, "resign_tx failed");

  // should be accepted by the pool
  events.push_back(tx_3);
  
  // add blk_4, this step was deferred in order to skip expiration condition at tx_memory_pool::add_tx
  events.push_back(blk_4);

  // blk_5 contain already expired tx_3 and thus should be rejected by the core
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_5, blk_4, miner_acc, tx_3);

  return true;
}

//------------------------------------------------------------------

tx_expiration_time_and_block_template::tx_expiration_time_and_block_template()
{
  REGISTER_CALLBACK_METHOD(tx_expiration_time_and_block_template, c1);
}

bool tx_expiration_time_and_block_template::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure expired transaction, waiting in tx pool, won't be included into a block template, making it invalid

  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  uint64_t ts_median = generator.get_timestamps_median(get_block_hash(blk_0r), TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW);
  LOG_PRINT_YELLOW("%%%%%%% ts_median = " << ts_median, LOG_LEVEL_0);

  /// core tx expiration condition:
  /// expiration_time - TX_EXPIRATION_MEDIAN_SHIFT >  get_last_n_blocks_timestamps_median(TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW)

  // create normal tx, then add expiration time entry to the extra and then resign
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  bool r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc.get_keys(), miner_acc.get_public_address(), MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  transaction tx_1  = AUTO_VAL_INIT(tx_1);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_1, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  uint64_t tx_1_expiration_time = ts_median + TX_EXPIRATION_MEDIAN_SHIFT + 1;  // one second greather than minimum allowed
  set_tx_expiration_time(tx_1, tx_1_expiration_time);
  r = resign_tx(miner_acc.get_keys(), sources, tx_1);
  CHECK_AND_ASSERT_MES(r, false, "resign_tx failed");

  events.push_back(tx_1); // should normally pass into tx pool

  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);

  ts_median = generator.get_timestamps_median(get_block_hash(blk_2), TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW);

  // make sure tx_1 is expired with this median
  CHECK_AND_ASSERT_MES(tx_1_expiration_time - TX_EXPIRATION_MEDIAN_SHIFT <= ts_median, false, "tx_1 didn't expire yet as expected");

  // okay, now we have expired transaction in tx pool, try to construct block_template and mine a block
  DO_CALLBACK(events, "c1");

  return true;
}

bool tx_expiration_time_and_block_template::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect tx count in the pool: " << c.get_pool_transactions_count());

  account_public_address addr = AUTO_VAL_INIT(addr);
  bool r = mine_next_pow_block_in_playtime(addr, c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  // tx MAY stay in the pool, check it as forced condition (may change in future)
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect tx count in the pool: " << c.get_pool_transactions_count());

  LOG_PRINT("tx pool:" << ENDL << c.get_tx_pool().print_pool(true), LOG_LEVEL_0);

  LOG_PRINT_YELLOW("tx_pool::on_idle()", LOG_LEVEL_0);
  c.get_tx_pool().on_idle();

  LOG_PRINT("tx pool:" << ENDL << c.get_tx_pool().print_pool(true), LOG_LEVEL_0);

  return true;
}

//------------------------------------------------------------------

bool tx_expiration_time_and_chain_switching::generate(std::vector<test_event_entry>& events) const
{
  // test idea: check border case when tx with expiration time are moved back from blockchain to the pool

  bool r = false;
  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  //
  // chain A
  //
  //  0 ... 20    21    22    23    24          <- height
  // (0 )- (0r)- (1 )- (2 )-                    <- chain A
  //             tx_1
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, static_cast<size_t>(std::max(CURRENCY_MINED_MONEY_UNLOCK_WINDOW, TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW)));

  uint64_t ts_median = generator.get_timestamps_median(get_block_hash(blk_0r), TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW);
  LOG_PRINT_YELLOW("%%%%%%% ts_median = " << ts_median << " for the last " << TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW << " blocks", LOG_LEVEL_0);

  /// core tx expiration GOOD condition:
  /// expiration_time - TX_EXPIRATION_MEDIAN_SHIFT >  get_last_n_blocks_timestamps_median(TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW)

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc.get_keys(), miner_acc.get_public_address(), MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  transaction tx_0  = AUTO_VAL_INIT(tx_0);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_0, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  uint64_t tx_0_expiration_time = ts_median + TX_EXPIRATION_MEDIAN_SHIFT + 0;  // one second less than minimum allowed (see condition above)
  set_tx_expiration_time(tx_0, tx_0_expiration_time);
  r = resign_tx(miner_acc.get_keys(), sources, tx_0);
  CHECK_AND_ASSERT_MES(r, false, "resign_tx failed");
  
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_0); // should not pass at tx pool as it has expiration time exactly at the right boundary of allowed period

  // tx_1 is the same except the expiration time
  transaction tx_1 = tx_0;
  uint64_t tx_1_expiration_time = ts_median + TX_EXPIRATION_MEDIAN_SHIFT + 1;  // minimum allowed ts (see condition above)
  set_tx_expiration_time(tx_1, tx_1_expiration_time);
  r = resign_tx(miner_acc.get_keys(), sources, tx_1);
  CHECK_AND_ASSERT_MES(r, false, "resign_tx failed");
  events.push_back(tx_1); // should pass

  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1); // tx_1 and blk_1 should pass

  //
  // chain B
  //
  //  0 ... 20    21    22    23    24          <- height
  // (0 )- (0r)- (1 )-                          <- chain A
  //         |   tx_1
  //          \ 
  //           \ (1b)- (2b)-                    <- chain B (became main after 2b)
  //                              
  MAKE_NEXT_BLOCK(events, blk_1b, blk_0r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_2b, blk_1b, miner_acc); // <-- should trigger chain switching
  
  // make sure the chain switching was ok
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_2b));
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1))


  //  0 ... 20    21    22    23    24          <- height
  // (0 )- (0r)- (1 )-                          <- chain A
  //         |   tx_1
  //          \ 
  //           \ (1b)- (2b)- !3b!-              <- chain B, block 3b is rejected because tx_1 is already expired
  //                         tx_1 

  // tx_1 is still in the pool, but can't be added to any block anymore as it was already expired
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_3b_bad, blk_2b, miner_acc, tx_1);
  // make sure top block wasn't changed
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_2b));
  // make sure tx_1 is still in the pool
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1))

  //
  // chain C
  //
  //  0 ... 20    21    22    23    24          <- height
  // (0 )- (0r)- (1 )-                          <- chain A
  //         |   tx_1
  //         |\ 
  //         | \ (1b)- (2b)-                    <- chain B
  //         |                    
  //          \- (1c)- (2c)- (3c)-              <- chain C
  //             tx_1

  // should pass as tx_1 can be added to any block on height 21 (it's not yet expired on that height)
  MAKE_NEXT_BLOCK_TX1(events, blk_1c, blk_0r, miner_acc, tx_1);
  MAKE_NEXT_BLOCK(events, blk_2c, blk_1c, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_3c, blk_2c, miner_acc); // <- should trigger chain switching

  // make sure the chain switching was ok and the pool is empty
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_3c));
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0))

  //
  // switch to chain B again
  //
  //  0 ... 20    21    22    23    24          <- height
  // (0 )- (0r)- (1 )-                          <- chain A
  //         |   tx_1
  //         |\ 
  //         | \ (1b)- (2b)- (3b)- (4b)-        <- chain B
  //         |                    
  //          \- (1c)- (2c)- (3c)-              <- chain C
  //             tx_1

  // should pass as tx_1 can be added to any block on height 21 (it's not yet expired on that height)
  MAKE_NEXT_BLOCK(events, blk_3b, blk_2b, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_4b, blk_3b, miner_acc); // <- should trigger chain switching

  // make sure the chain switching was ok
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_4b));
  // tx_1 should be popped out to the pool again
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1))


  return true;
}

//------------------------------------------------------------------

bool tx_key_image_pool_conflict::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: check tx that is stuck in tx pool because one on its key images is already spent in the blockchain
  // 1) if it's linked to an alt block -- tx will not be removed as long as linked alt block exists (in order to be able to switch)
  // 2) if it's not linked to an alt block -- it will be removed after CONFLICT_KEY_IMAGE_SPENT_DEPTH_TO_REMOVE_TX_FROM_POOL confirmations of conflicted tx
  //    or it will be removed once tx is old enough (CURRENCY_MEMPOOL_TX_LIVETIME)

  bool r = false;

  m_miner_acc.generate();
  GENERATE_ACCOUNT(bob_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, m_miner_acc, test_core_time::get_time());

  REWIND_BLOCKS_N(events, blk_0r, blk_0, m_miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  // make tx_0 : miner -> bob
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  r = fill_tx_sources_and_destinations(events, blk_0r, m_miner_acc.get_keys(), bob_acc.get_public_address(), MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  transaction tx_0  = AUTO_VAL_INIT(tx_0);
  r = construct_tx(m_miner_acc.get_keys(), sources, destinations, empty_attachment, tx_0, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  LOG_PRINT_YELLOW("tx_0 = " << get_transaction_hash(tx_0), LOG_LEVEL_0);
  // do not push tx_0 into events yet

  // tx_1 spends the same key image as tx_0
  transaction tx_1 = tx_0;
  keypair kp = keypair::generate();
  // change tx pub key to end up with different tx hash
  update_or_add_field_to_extra(tx_1.extra, kp.pub);
  r = resign_tx(m_miner_acc.get_keys(), sources, tx_1);
  CHECK_AND_ASSERT_MES(r, false, "resign_tx failed");
  LOG_PRINT_YELLOW("tx_1 = " << get_transaction_hash(tx_1), LOG_LEVEL_0);

  // tx_2 spends the same key image as tx_0
  transaction tx_2 = tx_0;
  kp = keypair::generate();
  // change tx pub key to end up with different tx hash
  update_or_add_field_to_extra(tx_2.extra, kp.pub);
  r = resign_tx(m_miner_acc.get_keys(), sources, tx_2);
  CHECK_AND_ASSERT_MES(r, false, "resign_tx failed");
  LOG_PRINT_YELLOW("tx_2 = " << get_transaction_hash(tx_2), LOG_LEVEL_0);

  events.push_back(tx_1);

  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));

  // as long as tx_0 is using the same key image as tx_1, tx_0 and tx_2 can't be added to the pool atm
  // make sure that it's true
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_0);
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_2);

  // however, tx_0 and tx_2 can be added with kept_by_block flag (to simulate it's going with blk_1)
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
  events.push_back(tx_0);
  events.push_back(tx_2);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));
  
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));

  // make a block with tx_0 and put tx_0 to the blockchain
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, m_miner_acc, tx_0);

  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));

  // tx_1 and tx_2 is still in the pool
  // it can never be added to any block as long as blk_1 is in the blockchain due to key image conflict

  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_2_bad, blk_1, m_miner_acc, tx_1);

  // add tx_1 to alt block, it should go well
  MAKE_NEXT_BLOCK_TX1(events, blk_1a, blk_0r, m_miner_acc, tx_1);

  // however, it does not remove tx from the pool
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));

  //
  // make sure stuck tx will be removed from the pool when it's too old
  //
  
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, m_miner_acc);

  // remove_stuck_txs should not remove anything, tx_1 and tx_2 should be in the pool
  DO_CALLBACK(events, "remove_stuck_txs");
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));

  // shift time by CURRENCY_MEMPOOL_TX_LIVETIME
  events.push_back(event_core_time(CURRENCY_MEMPOOL_TX_LIVETIME + 1, true));

  // remove_stuck_txs should have removed tx_2 because it's too old
  DO_CALLBACK(events, "remove_stuck_txs");
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));

  //
  // make sure stuck tx will be removed from the pool as soon as one of its key images is spent deep enough in the blockchain
  // (even if it's not too old to be removed by age)
  //
  
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, m_miner_acc);

  // re-add tx_2 with kept_by_block flag
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
  events.push_back(tx_2);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));

  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));

  // remove_stuck_txs should not remove anything, tx_1 and tx_2 should be in the pool
  DO_CALLBACK(events, "remove_stuck_txs");
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));

  // rewind 50 blocks so tx_0 spending its key image will be deep enough
  REWIND_BLOCKS_N_WITH_TIME(events, blk_3r, blk_3, m_miner_acc, 50);

  // remove_stuck_txs should remove only tx_2 and left tx_1 (linked to alt block)
  DO_CALLBACK(events, "remove_stuck_txs");
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));

  DO_CALLBACK(events, "print_tx_pool");

  return true;
}
