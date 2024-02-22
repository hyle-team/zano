// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "chain_switch_pow_pos.h"
#include "wallet/wallet2.h"
#include "pos_block_builder.h"

using namespace epee;
using namespace currency;

gen_chain_switch_pow_pos::gen_chain_switch_pow_pos()
  : m_enormous_fee(0)
  , m_invalid_block_index(std::numeric_limits<decltype(m_invalid_block_index)>::max())
{
  m_hardforks.m_height_the_hardfork_n_active_after[1] = 1440;
  m_hardforks.m_height_the_hardfork_n_active_after[2] = 1800;
  m_hardforks.m_height_the_hardfork_n_active_after[3] = 1801;
  m_hardforks.m_height_the_hardfork_n_active_after[4] = 50000000000;

  REGISTER_CALLBACK_METHOD(gen_chain_switch_pow_pos, configure_core);
  REGISTER_CALLBACK_METHOD(gen_chain_switch_pow_pos, check_height1);
  REGISTER_CALLBACK_METHOD(gen_chain_switch_pow_pos, check_chains_1);
  REGISTER_CALLBACK_METHOD(gen_chain_switch_pow_pos, check_balance_1);
  REGISTER_CALLBACK_METHOD(gen_chain_switch_pow_pos, mark_invalid_block);

  test_gentime_settings s = test_generator::get_test_gentime_settings();
  s.tx_max_out_amount = PREMINE_AMOUNT; // use very high max allowed output amount to avoid splitting premine to countless outputs and slowing down the test to the death
  s.miner_tx_max_outs = 11;             // limit genesis coinbase outs count for the same reason
  test_generator::set_test_gentime_settings(s);
}

bool gen_chain_switch_pow_pos::generate(std::vector<test_event_entry>& events) const
{                     
  uint64_t ts_start = 1420000000;
  std::list<currency::account_base> miner_stake_sources, alice_stake_sources, bob_stake_sources;

  //                                                                                          event index
  GENERATE_ACCOUNT(miner_acc);
  miner_stake_sources.push_back(miner_acc);
  m_accounts.push_back(miner_acc);
  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, miner_acc, ts_start);
  events.push_back(blk_0);                                                                    // 0
  events.push_back(miner_acc);                                                                // 1
  MAKE_ACCOUNT(events, alice);                                                                // 2
  alice_stake_sources.push_back(alice);
  m_accounts.push_back(alice);
  MAKE_ACCOUNT(events, bob);                                                                  // 3
  bob_stake_sources.push_back(bob);
  m_accounts.push_back(bob);
  DO_CALLBACK(events, "configure_core");                                                      // 4
  
  block blk_0r = blk_0;
  for(size_t i = 0; i < CURRENCY_MINED_MONEY_UNLOCK_WINDOW; ++i)
  {
    block blk = AUTO_VAL_INIT(blk);
    uint64_t ts = blk_0r.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN / 2; // to increase main chain difficulty
    bool r = generator.construct_block_manually(blk, blk_0r, miner_acc,test_generator::bf_timestamp, 0, 0, ts);
    CHECK_AND_ASSERT_MES(r, false, "construct_block_manually failed");
    events.push_back(blk);
    blk_0r = blk;
  } //                                                                                        // N+4
  
  uint64_t buttloads_of_money = currency::get_outs_money_amount(blk_0.miner_tx);
  m_premine_amount = buttloads_of_money;
  MAKE_TX_FEE(events, tx_1, miner_acc, alice, buttloads_of_money - TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, blk_0r);  // N+5
  buttloads_of_money -= TESTS_DEFAULT_FEE;

  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);                                // N+6

  /* legend: (n) - PoW block, !m! - PoS block
     0....10    11    12   <-- blockchain height
    (0 )-(0r) -(1 )
  */

  DO_CALLBACK(events, "check_height1");                                                       // N+7

  // strange tx with enormous fee
  m_enormous_fee = buttloads_of_money - MK_TEST_COINS(1);
  MAKE_TX_FEE(events, tx_2, alice, alice, MK_TEST_COINS(1), m_enormous_fee, blk_1);           // N+8
  MAKE_TX(events, tx_3, miner_acc, bob, MK_TEST_COINS(1), blk_1);                             // N+9

  block plk_2 = AUTO_VAL_INIT(plk_2);
  generator.construct_block(events, plk_2, blk_1, miner_acc, std::list<transaction>(1, tx_2), miner_stake_sources);
  events.push_back(plk_2);                                                                    // N+10
  PRINT_EVENT_N(events);

  /* legend: (n) - PoW block, !m! - PoS block
     0....10    11    12   <-- blockchain height
    (0 )-(0r) -(1 ) -!2 !
  */

  DO_CALLBACK(events, "check_balance_1");                                                     // N+11
  
  
  MAKE_NEXT_POS_BLOCK(events, plk_3, plk_2, miner_acc, miner_stake_sources);                  // N+12

  /* legend: (n) - PoW block, !m! - PoS block
     0....10    11    12    13                     <-- blockchain height
    (0 )-(0r) -(1 ) -!2 ! -!3 !
  */

  // Alice splits the chain...
  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_1, alice, tx_3);                                     // N+13

  /* legend: (n) - PoW block, !m! - PoS block
     0....10    11    12    13                     <-- blockchain height
    (0 )-(0r) -(1 ) -!2 ! -!3 !                    <-- main
                  \ -(4 )                          <-- alt
                     tx_3
  */

  {
    // test minimum coinage requirements, create fully hommade PoS to avoid sources checking within the generator
    const currency::block& prev_block = blk_4;
    const transaction& stake = tx_1;
    size_t height = CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3;

    crypto::hash prev_id = get_block_hash(prev_block);
    currency::wide_difficulty_type diff = generator.get_difficulty_for_next_block(prev_id, false);
    crypto::public_key stake_tx_pub_key = get_tx_pub_key_from_extra(stake);
    size_t stake_output_idx = 0, i = 0;
    uint64_t stake_output_amount = 0;
    std::for_each(stake.vout.begin(), stake.vout.end(), [&stake_output_amount, &stake_output_idx, &i](const tx_out_v& o_)
      {
      auto& o = boost::get<currency::tx_out_bare>(o_);
        if (o.amount > stake_output_amount) 
        {
          stake_output_amount = o.amount; 
          stake_output_idx = i; 
        } 
        ++i; 
      });
    size_t stake_output_gidx = generator.get_tx_out_gindex(prev_id, currency::get_transaction_hash(stake), stake_output_idx);
    crypto::key_image stake_output_key_image;
    keypair kp;
    generate_key_image_helper(alice.get_keys(), stake_tx_pub_key, stake_output_idx, kp, stake_output_key_image);
    crypto::public_key stake_output_pubkey = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(stake.vout[stake_output_idx]).target).key;

    pos_block_builder pb;
    pb.step1_init_header(generator.get_hardforks(), height, prev_id);
    pb.step2_set_txs(std::vector<transaction>());
    pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, diff, prev_id, null_hash, prev_block.timestamp);
    pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(prev_block), alice.get_public_address());
    pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, alice);

    DO_CALLBACK(events, "mark_invalid_block");                                                // N+14
    // EXPECTED: block rejected as invalid because coinstake age is less then required (coinstake refers to money transferred by a tx in blk_1, age=1)
    events.push_back(pb.m_block);                                                             // N+15

  /* legend: (n) - PoW block, !m! - PoS block
     0....10    11    12    13                     <-- blockchain height
    (0 )-(0r) -(1 ) -!2 ! -!3 !                    <-- main
                  \ -(4 )                          <-- alt
                        \ - ?? <---rejected pos
  */
  }

  // rewing few block so alice can use money transferred to her by tx_1 (block 1, height 11)
  REWIND_BLOCKS_N(events, blk_4r, blk_4, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);         // 2N+15

  /* legend: (n) - PoW block, !m! - PoS block
     0....10    11    12    13    22               <-- blockchain height
    (0 )-(0r) -(1 ) -!2 ! -!3 !                    <-- main
                  \ -(4 )  .... -(4r)              <-- alt (no reorganize due to smaller cumul diff)
  */

  MAKE_NEXT_POS_BLOCK(events, plk_5, blk_4r, alice, alice_stake_sources);                     // 2N+16
  
  /* legend: (n) - PoW block, !m! - PoS block
     0....10    11    12    13    22    23         <-- blockchain height
    (0 )-(0r) -(1 ) -!2 ! -!3 !                    <-- main
                  \ -(4 )  .... -(4r) -!5 !        <-- alt
  */

  DO_CALLBACK(events, "check_chains_1");                                                      // 2N+17

  // Although plk_6's coinstake refers to a tx_3, lying in altchain (blk_4), it's a valid case
  MAKE_NEXT_POS_BLOCK(events, plk_6, plk_5, bob, bob_stake_sources);                          // 2N+18

  /* legend: (n) - PoW block, !m! - PoS block
     0....10    11    12    13    22    23    24      <-- blockchain height
    (0 )-(0r) -(1 ) -!2 ! -!3 !                       <-- main
                  \ -(4 )  .... -(4r) -!5 ! -!6 !     <-- alt
                     tx_3                     |
                      |                       |
                      +-----------------------+       plk_6 uses tx_3 output as coinstake
  */

  return true;
}


bool gen_chain_switch_pow_pos::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE; //four blocks
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH; //four blocks

  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

bool gen_chain_switch_pow_pos::check_height1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  CHECK_AND_ASSERT_MES(c.get_current_blockchain_size() == 12, false, "incorrect blockchain height");
  return true;
}


bool gen_chain_switch_pow_pos::check_chains_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  CHECK_AND_ASSERT_MES(c.get_alternative_blocks_count() == 12, false, "invalid altchain size");
  CHECK_AND_ASSERT_MES(c.get_current_blockchain_size() == 14, false, "incorrect blockchain size");
  CHECK_AND_ASSERT_MES(c.get_blockchain_storage().get_total_transactions() == 14 /*coingen in main*/ + 2 /*tx in main*/, false, "incorect total transactions count");
  return true;
}

bool gen_chain_switch_pow_pos::check_balance_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  const block& plk_2 = boost::get<block>(events[CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 10]);
  CHECK_AND_ASSERT_MES(get_block_height(plk_2) == 12, false, "invalid height");

  test_generator::wallets_vector w;
  bool r = generator.build_wallets(get_block_hash(plk_2), m_accounts, w);
  CHECK_AND_ASSERT_MES(r && w.size() == 3 && w[0].wallet != 0 && w[1].wallet != 0 && w[2].wallet != 0, false, "failed to build wallets");

  /*
  uint64_t mined_amount = m_early_blocks_reward * 12 + TX_POOL_MINIMUM_FEE / 2 + m_enormous_fee / 2;
  if (!check_balance_via_wallet(*w[0], "miner", mined_amount, mined_amount))
    return false;
    */

  if (!check_balance_via_wallet(*w[1].wallet, "alice", MK_TEST_COINS(1), 0, 0, 0, 0))
    return false;

  return true;
}

//------------------------------------------------------------------------------
pow_pos_reorganize_specific_case::pow_pos_reorganize_specific_case()
{
  REGISTER_CALLBACK_METHOD(pow_pos_reorganize_specific_case, configure_core);
}

bool pow_pos_reorganize_specific_case::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  std::list<currency::account_base> miner_stake_sources( {miner_acc} );

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_10, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  MAKE_NEXT_POS_BLOCK(events, blk_11, blk_10, miner_acc, miner_stake_sources);

  MAKE_NEXT_BLOCK(events, blk_12, blk_11, miner_acc);

  MAKE_NEXT_POS_BLOCK(events, blk_13, blk_12, miner_acc, miner_stake_sources);

  MAKE_NEXT_BLOCK(events, blk_14, blk_13, miner_acc);

  MAKE_NEXT_POS_BLOCK(events, blk_15, blk_14, miner_acc, miner_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_16, blk_15, miner_acc, miner_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_17, blk_16, miner_acc, miner_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_18, blk_17, miner_acc, miner_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_19, blk_18, miner_acc, miner_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_20, blk_19, miner_acc, miner_stake_sources);

  MAKE_NEXT_BLOCK(events, blk_20a, blk_19, miner_acc); // this should trigger chain swtich

  // make sure chains were switched indeed
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_20a), get_block_hash(blk_20a)));

  MAKE_NEXT_BLOCK(events, blk_21a, blk_20, miner_acc); // have to go as alternative 
  //should go as alternative 
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_21a), get_block_hash(blk_21a)));


  // mine few more block to make sure everything is okay
  MAKE_NEXT_POS_BLOCK(events, blk_22, blk_21a, miner_acc, miner_stake_sources);
  //MAKE_NEXT_POS_BLOCK(events, blk_22, blk_21, miner_acc, miner_stake_sources);
  MAKE_NEXT_BLOCK(events, blk_23, blk_22, miner_acc);
  MAKE_NEXT_POS_BLOCK(events, blk_24, blk_23, miner_acc, miner_stake_sources);

  return true;
}

bool pow_pos_reorganize_specific_case::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE; //four blocks
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH; //four blocks
  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}
