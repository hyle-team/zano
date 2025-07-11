// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "offers_tests_common.h"
#include "pos_basic_tests.h"
#include "pos_block_builder.h"

using namespace epee;
using namespace currency;


gen_pos_basic_tests::gen_pos_basic_tests()
{
  REGISTER_CALLBACK_METHOD(gen_pos_basic_tests, configure_core);
  REGISTER_CALLBACK_METHOD(gen_pos_basic_tests, configure_check_height1);
  REGISTER_CALLBACK_METHOD(gen_pos_basic_tests, configure_check_height2);
  REGISTER_CALLBACK_METHOD(gen_pos_basic_tests, check_exchange_1);
}
#define FIRST_ALIAS_NAME "first"
#define SECOND_ALIAS_NAME "second"

bool gen_pos_basic_tests::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;
  std::list<currency::account_base> coin_stake_sources;

  GENERATE_ACCOUNT(preminer_account);
  GENERATE_ACCOUNT(miner_account);
  GENERATE_ACCOUNT(some_account_1);

  coin_stake_sources.push_back(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, preminer_account, ts_start);
  MAKE_ACCOUNT(events, first_acc);
  MAKE_ACCOUNT(events, second_acc);
  MAKE_ACCOUNT(events, third_acc);
  DO_CALLBACK(events, "configure_core");


  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_11, blk_1, miner_account, 10);
  MAKE_NEXT_POS_BLOCK(events, blk_12, blk_11, miner_account, coin_stake_sources);
  MAKE_TX(events, tx_1, miner_account, some_account_1, MK_TEST_COINS(1), blk_12);
  MAKE_NEXT_BLOCK_TX1(events, blk_13, blk_12, miner_account, tx_1);
  MAKE_NEXT_BLOCK(events, blk_14, blk_13, miner_account);
  events.push_back(event_core_time(blk_14.timestamp));
  MAKE_NEXT_POS_BLOCK(events, blk_15, blk_14, miner_account, coin_stake_sources);
  MAKE_NEXT_BLOCK(events, blk_16, blk_15, miner_account);
  MAKE_NEXT_BLOCK(events, blk_17, blk_16, miner_account);
  events.push_back(event_core_time(blk_17.timestamp));
  MAKE_NEXT_POS_BLOCK(events, blk_18, blk_17, miner_account, coin_stake_sources);
  MAKE_NEXT_BLOCK(events, blk_19, blk_18, miner_account);
  MAKE_NEXT_BLOCK(events, blk_20, blk_19, miner_account);
  events.push_back(event_core_time(blk_20.timestamp));
  MAKE_NEXT_POS_BLOCK(events, blk_21, blk_20, miner_account, coin_stake_sources);
  MAKE_NEXT_BLOCK(events, blk_22, blk_21, miner_account);
  MAKE_NEXT_BLOCK(events, blk_23, blk_22, miner_account);
  events.push_back(event_core_time(blk_23.timestamp));
  MAKE_NEXT_POS_BLOCK(events, blk_24, blk_23, miner_account, coin_stake_sources);
  MAKE_NEXT_BLOCK(events, blk_25, blk_24, miner_account);
  MAKE_NEXT_BLOCK(events, blk_26, blk_25, miner_account);
  events.push_back(event_core_time(blk_26.timestamp));
  MAKE_NEXT_POS_BLOCK(events, blk_27, blk_26, miner_account, coin_stake_sources);
  DO_CALLBACK(events, "configure_check_height1");
  
  // start alternative chain
  MAKE_NEXT_POS_BLOCK(events, blk_21_a, blk_20, miner_account, coin_stake_sources);
  MAKE_NEXT_BLOCK(events, blk_22_a, blk_21_a, miner_account);
  MAKE_NEXT_BLOCK(events, blk_23_a, blk_22_a, miner_account);
  MAKE_NEXT_POS_BLOCK(events, blk_24_a, blk_23_a, miner_account, coin_stake_sources);
  MAKE_NEXT_BLOCK(events, blk_25_a, blk_24_a, miner_account);
  MAKE_NEXT_BLOCK(events, blk_26_a, blk_25_a, miner_account);
  MAKE_NEXT_POS_BLOCK(events, blk_27_a, blk_26_a, miner_account, coin_stake_sources);
  MAKE_NEXT_BLOCK(events, blk_28_a, blk_27_a, miner_account);
  MAKE_NEXT_BLOCK(events, blk_29_a, blk_28_a, miner_account);
  MAKE_NEXT_POS_BLOCK(events, blk_30_a, blk_29_a, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_31_a, blk_30_a, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_32_a, blk_31_a, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_33_a, blk_32_a, miner_account, coin_stake_sources);

  DO_CALLBACK(events, "configure_check_height2");
  // start alternative chain
  MAKE_NEXT_BLOCK(events, blk_34, blk_33_a, miner_account);

  bc_services::offer_details od = AUTO_VAL_INIT(od);

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
  std::vector<currency::attachment_v> attachments;
  bc_services::put_offer_into_attachment(od, attachments);

  MAKE_TX_LIST_START_WITH_ATTACHS(events, txs_blk, miner_account, some_account_1, MK_TEST_COINS(1), blk_34, attachments);
  //MAKE_TX_LIST_OFFER(events, txs_blk, miner_account, some_account_1, MK_COINS(1), blk_33_a, offers);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_35, blk_34, miner_account, txs_blk);
  MAKE_NEXT_BLOCK(events, blk_36, blk_35, miner_account);
  DO_CALLBACK(events, "check_exchange_1");

  return true;
}

bool gen_pos_basic_tests::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE; //four blocks
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH; //four blocks

  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}
bool gen_pos_basic_tests::configure_check_height1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  uint64_t h = c.get_current_blockchain_size();  
  CHECK_EQ(h, 28);
  return true;
}

bool gen_pos_basic_tests::configure_check_height2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  uint64_t h = c.get_current_blockchain_size();
  CHECK_EQ(h, 34);
  return true;
}

bool gen_pos_basic_tests::check_exchange_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::list<bc_services::offer_details_ex> offers;
  get_all_offers(c, offers);

  CHECK_EQ(offers.size(), 1);
  return true;
}


//------------------------------------------------------------------------------

pos_mining_with_decoys::pos_mining_with_decoys()
{
  REGISTER_CALLBACK_METHOD(pos_mining_with_decoys, c1);
}

bool pos_mining_with_decoys::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  currency::account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();   miner_acc.set_createtime(ts);
  currency::account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();   alice_acc.set_createtime(ts);
  currency::account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();     bob_acc.set_createtime(ts);
  currency::account_base& carol_acc = m_accounts[CAROL_ACC_IDX]; carol_acc.generate();   carol_acc.set_createtime(ts);
  currency::account_base& dan_acc   = m_accounts[DAN_ACC_IDX];   dan_acc.generate(true); dan_acc.set_createtime(ts);    // Dan has an auditable wallet

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  bool r = false;
  std::vector<tx_source_entry> sources;
  r = fill_tx_sources(sources, events, blk_0r, miner_acc.get_keys(), 2 * COIN, 0);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  std::vector<tx_destination_entry> destinations;
  destinations.emplace_back(47 * TESTS_DEFAULT_FEE, alice_acc.get_public_address());
  destinations.emplace_back(47 * TESTS_DEFAULT_FEE, miner_acc.get_public_address());  // as a decoy for Alice and Dan
  destinations.emplace_back(47 * TESTS_DEFAULT_FEE, dan_acc.get_public_address()); 
  destinations.emplace_back(5  * TESTS_DEFAULT_FEE, bob_acc.get_public_address());
  destinations.emplace_back(COIN,                   carol_acc.get_public_address());

  transaction tx_0{};
  r = construct_tx(miner_acc.get_keys(), sources, destinations, events, this, tx_0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_0);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK(events, "c1");

  return true;
}

bool pos_mining_with_decoys::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  CHECK_AND_ASSERT_MES(!c.get_blockchain_storage().get_core_runtime_config().is_hardfork_active_for_height(4, c.get_top_block_height()), false, "HF4 should not be active");

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, m_accounts[MINER_ACC_IDX]);
  miner_wlt->refresh();

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  alice_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", 47 * TESTS_DEFAULT_FEE, INVALID_BALANCE_VAL, 47 * TESTS_DEFAULT_FEE), false, "");

  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, m_accounts[BOB_ACC_IDX]);
  bob_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt, "Bob", 5 * TESTS_DEFAULT_FEE, INVALID_BALANCE_VAL, 5 * TESTS_DEFAULT_FEE), false, "");

  std::shared_ptr<tools::wallet2> carol_wlt = init_playtime_test_wallet(events, c, m_accounts[CAROL_ACC_IDX]);
  carol_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*carol_wlt, "Carol", COIN, INVALID_BALANCE_VAL, COIN), false, "");

  std::shared_ptr<tools::wallet2> dan_wlt = init_playtime_test_wallet(events, c, m_accounts[DAN_ACC_IDX]);
  dan_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*dan_wlt, "Dan", 47 * TESTS_DEFAULT_FEE, INVALID_BALANCE_VAL, 47 * TESTS_DEFAULT_FEE), false, "");


  // 1. Alice should be able to mine a PoS block with 1 decoys (ring size == 2)
  size_t top_block_height = c.get_top_block_height();

  r = alice_wlt->try_mint_pos(m_accounts[ALICE_ACC_IDX].get_public_address());
  CHECK_AND_ASSERT_MES(r, false, "try_mint_pos failed");
  
  {
    block b{};
    CHECK_AND_ASSERT_MES(c.get_blockchain_storage().get_top_block(b), false, "");
    CHECK_AND_ASSERT_MES(get_block_height(b) == top_block_height + 1, false, "unexpected top block height");
  
    txin_to_key& intk = boost::get<txin_to_key>(b.miner_tx.vin[1]);
    CHECK_AND_ASSERT_MES(intk.amount == 47 * TESTS_DEFAULT_FEE, false, "incorrect amount: " << intk.amount);
    CHECK_AND_ASSERT_MES(intk.key_offsets.size() == 2, false, "unexpected ring size: " << intk.key_offsets.size());
  }


  // 2. Bob should only be able to mine a PoS block with zero decoys (ring size == 1)
  top_block_height = c.get_top_block_height();

  bob_wlt->refresh();
  r = bob_wlt->try_mint_pos(m_accounts[BOB_ACC_IDX].get_public_address());
  CHECK_AND_ASSERT_MES(r, false, "try_mint_pos failed");

  {
    block b{};
    CHECK_AND_ASSERT_MES(c.get_blockchain_storage().get_top_block(b), false, "");
    CHECK_AND_ASSERT_MES(get_block_height(b) == top_block_height + 1, false, "unexpected top block height");

    txin_to_key& intk = boost::get<txin_to_key>(b.miner_tx.vin[1]);
    CHECK_AND_ASSERT_MES(intk.amount == 5 * TESTS_DEFAULT_FEE, false, "incorrect amount: " << intk.amount);
    CHECK_AND_ASSERT_MES(intk.key_offsets.size() == 1, false, "unexpected ring size: " << intk.key_offsets.size());
  }


  // 3. Carol should only be able to mine a PoS block with CURRENCY_DEFAULT_DECOY_SET_SIZE decoys (ring size == CURRENCY_DEFAULT_DECOY_SET_SIZE + 1)
  top_block_height = c.get_top_block_height();

  carol_wlt->refresh();
  r = carol_wlt->try_mint_pos(m_accounts[CAROL_ACC_IDX].get_public_address());
  CHECK_AND_ASSERT_MES(r, false, "try_mint_pos failed");

  {
    block b{};
    CHECK_AND_ASSERT_MES(c.get_blockchain_storage().get_top_block(b), false, "");
    CHECK_AND_ASSERT_MES(get_block_height(b) == top_block_height + 1, false, "unexpected top block height");

    txin_to_key& intk = boost::get<txin_to_key>(b.miner_tx.vin[1]);
    CHECK_AND_ASSERT_MES(intk.amount == COIN, false, "incorrect amount: " << intk.amount);
    CHECK_AND_ASSERT_MES(intk.key_offsets.size() == CURRENCY_DEFAULT_DECOY_SET_SIZE + 1, false, "unexpected ring size: " << intk.key_offsets.size());
  }


  // 4. Dan has an auditable wallet that couldn't use mixins, but still he should be able to successfully mine a PoS block (ring size = 1, zero decoys)
  top_block_height = c.get_top_block_height();

  CHECK_AND_ASSERT_MES(dan_wlt->is_auditable(), false, "Dan's wallet is not auditable, which is unexpected");

  dan_wlt->refresh();
  r = dan_wlt->try_mint_pos(m_accounts[DAN_ACC_IDX].get_public_address());
  CHECK_AND_ASSERT_MES(r, false, "try_mint_pos failed");

  {
    block b{};
    CHECK_AND_ASSERT_MES(c.get_blockchain_storage().get_top_block(b), false, "");
    CHECK_AND_ASSERT_MES(get_block_height(b) == top_block_height + 1, false, "unexpected top block height");

    txin_to_key& intk = boost::get<txin_to_key>(b.miner_tx.vin[1]);
    CHECK_AND_ASSERT_MES(intk.amount == 47 * TESTS_DEFAULT_FEE, false, "incorrect amount: " << intk.amount);
    CHECK_AND_ASSERT_MES(intk.key_offsets.size() == 1, false, "unexpected ring size: " << intk.key_offsets.size());
  }

  return true;
}

bool pos_and_no_pow_blocks_between_output_and_stake::generate(std::vector<test_event_entry>& events) const
{
  /*
    Test idea:
    Test checks that a PoS block is rejected if any of the inputs (stake or decoy)
    refers to an output created after the last PoW block.
    Rule violation: max_related_block_height > last_pow_block_height.

    Script:
    - The last PoW block is height 19
    - Creates a UTXO in PoS block 20
    - Trying to use it for steaking after unlocking window
    - Error expected: max_related_block_height (20) > last_pow_block_height (19)
  */
  uint64_t ts = test_core_time::get_time();
  std::list<currency::account_base> coin_stake_sources;
  m_accounts.resize(TOTAL_ACCS_COUNT);

  account_base& miner = m_accounts[MINER_ACC_IDX];
  miner.generate();
  miner.set_createtime(ts);

  account_base& alice = m_accounts[ALICE_ACC_IDX];
  alice.generate();
  alice.set_createtime(ts);

  account_base& bob = m_accounts[BOB_ACC_IDX];
  bob.generate();
  bob.set_createtime(ts);

  account_base& carol = m_accounts[CAROL_ACC_IDX];
  carol.generate();
  carol.set_createtime(ts);

  //  0     14    15    16    17    18    19    20     30    31                           <- blockchain height
  //        pow   pow   pow   pow   pow   pow   pos    pos   pos
  // (0 )- (1r)- (5 )- (6 )  (7 )- (8 )- (9 )- (2r).. (3r)- (31)                          <- main chain
  //             tx_0  tx_1  tx_2  tx_3  tx_4 tx_alice     invalid
  //
  // Expected: max_related_block_height (20) > last_pow_block_height (19)

  MAKE_GENESIS_BLOCK(events, blk_0, miner, ts);
  DO_CALLBACK(events, "configure_core");

  REWIND_BLOCKS_N(events, blk_1r, blk_0, miner, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + TESTS_POS_CONFIG_POS_MINIMUM_HEIGH);

  MAKE_TX(events, tx_0, miner, bob, MK_TEST_COINS(9000), blk_1r);
  MAKE_NEXT_BLOCK_TX1(events, blk_5, blk_1r, miner, tx_0);

  MAKE_TX(events, tx_1, miner, miner, MK_TEST_COINS(1000), blk_5);
  MAKE_NEXT_BLOCK_TX1(events, blk_6, blk_5, miner, tx_1);

  MAKE_TX(events, tx_2, miner, miner, MK_TEST_COINS(1000), blk_6);
  MAKE_NEXT_BLOCK_TX1(events, blk_7, blk_6, miner, tx_2);

  MAKE_TX(events, tx_3, miner, miner, MK_TEST_COINS(1000), blk_7);
  MAKE_NEXT_BLOCK_TX1(events, blk_8, blk_7, miner, tx_3);

  MAKE_TX(events, tx_4, miner, alice, MK_TEST_COINS(9000), blk_8);
  MAKE_NEXT_BLOCK_TX1(events, blk_9, blk_8, miner, tx_4);  // last pow block

  MAKE_TX(events, tx_to_alice, miner, alice, MK_TEST_COINS(10000), blk_9);  // will be decoy in first case
  MAKE_NEXT_POS_BLOCK_TX1(events, blk_2r, blk_9, miner, { miner }, tx_to_alice);  // related block

  REWIND_POS_BLOCKS_N_WITH_TIME(events, blk_3r, blk_2r, miner, { miner }, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // case 1: Provide valid tx output and invalid decoy
  // Expected error: stake input refs' max related block height is 20 while last PoW block height is 19
  const currency::block& prev_block = blk_3r;
  const transaction& stake          = tx_4;  // valid output

  size_t height        = get_block_height(prev_block) + 1;
  crypto::hash prev_id = get_block_hash(prev_block);

  std::vector<tx_source_entry> sources;

  /**
   * Fills output entries with the real output and a number of decoys using linear search.
   * Decoy outputs are selected sequentially from the list, without any randomization.
   */
  bool r = fill_tx_sources(
      sources,
      events,
      prev_block,
      alice.get_keys(),
      MK_TEST_COINS(9000),
      80,  // invalid decoy
      true,
      true,
      false);

  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  CHECK_AND_ASSERT_MES(shuffle_source_entries(sources), false, "");

  auto it = std::max_element(
      sources.begin(), sources.end(),
      [&](const tx_source_entry& lhs, const tx_source_entry& rhs)
      {
        return lhs.amount < rhs.amount;
      });
  const tx_source_entry& se               = *it;
  const tx_source_entry::output_entry& oe = se.outputs[se.real_output];

  crypto::key_image stake_output_key_image {};
  currency::keypair ephemeral_keys {};
  r = generate_key_image_helper(
      alice.get_keys(),
      se.real_out_tx_key,
      se.real_output_in_tx_index,
      ephemeral_keys,
      stake_output_key_image);
  CHECK_AND_ASSERT_MES(r, false, "generate_key_image_helper failed");

  uint64_t stake_output_gindex = boost::get<uint64_t>(oe.out_reference);

  currency::wide_difficulty_type pos_diff {};
  crypto::hash last_pow_block_hash {}, last_pos_block_kernel_hash {};
  r = generator.get_params_for_next_pos_block(
      prev_id,
      pos_diff,
      last_pow_block_hash,
      last_pos_block_kernel_hash);
  CHECK_AND_ASSERT_MES(r, false, "get_params_for_next_pos_block failed");

  pos_block_builder pb;
  pb.step1_init_header(generator.get_hardforks(), height, prev_id);
  pb.step2_set_txs(std::vector<transaction>());
  pb.step3a(pos_diff, last_pow_block_hash, last_pos_block_kernel_hash);
  pb.step3b(se.amount, stake_output_key_image, se.real_out_tx_key, se.real_output_in_tx_index, se.real_out_amount_blinding_mask, alice.get_keys().view_secret_key,
            stake_output_gindex, prev_block.timestamp, POS_SCAN_WINDOW, POS_SCAN_STEP);
  pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(prev_block), alice.get_public_address());
  pb.step5_sign(se, alice.get_keys());

  DO_CALLBACK(events, "mark_invalid_block");
  events.push_back(pb.m_block);

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_3r));

  // case 2: Provide invalid tx output
  // Expected error: stake input refs' max related block height is 20 while last PoW block height is 19
  {
    const currency::block& prev_block = blk_3r;
    const transaction& stake          = tx_to_alice;
    bool r                            = false;
    std::vector<tx_source_entry> sources;

    size_t height        = get_block_height(prev_block) + 1;
    crypto::hash prev_id = get_block_hash(prev_block);

    r = fill_tx_sources(
        sources,
        events,
        prev_block,
        alice.get_keys(),
        MK_TEST_COINS(10000),
        0,
        std::vector<tx_source_entry> {});

    CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");

    CHECK_AND_ASSERT_MES(shuffle_source_entries(sources), false, "");

    auto it = std::max_element(
        sources.begin(), sources.end(),
        [&](const tx_source_entry& lhs, const tx_source_entry& rhs)
        {
          return lhs.amount < rhs.amount;
        });
    const tx_source_entry& se               = *it;
    const tx_source_entry::output_entry& oe = se.outputs[se.real_output];

    crypto::key_image stake_output_key_image {};
    currency::keypair ephemeral_keys {};
    r = generate_key_image_helper(
        alice.get_keys(),
        se.real_out_tx_key,
        se.real_output_in_tx_index,
        ephemeral_keys,
        stake_output_key_image);
    CHECK_AND_ASSERT_MES(r, false, "generate_key_image_helper failed");

    uint64_t stake_output_gindex = boost::get<uint64_t>(oe.out_reference);
    currency::wide_difficulty_type pos_diff {};
    crypto::hash last_pow_block_hash {}, last_pos_block_kernel_hash {};
    r = generator.get_params_for_next_pos_block(
        prev_id,
        pos_diff,
        last_pow_block_hash,
        last_pos_block_kernel_hash);
    CHECK_AND_ASSERT_MES(r, false, "get_params_for_next_pos_block failed");

    pos_block_builder pb;
    pb.step1_init_header(generator.get_hardforks(), height, prev_id);
    pb.step2_set_txs(std::vector<transaction>());
    pb.step3a(pos_diff, last_pow_block_hash, last_pos_block_kernel_hash);
    pb.step3b(se.amount, stake_output_key_image, se.real_out_tx_key, se.real_output_in_tx_index, se.real_out_amount_blinding_mask, alice.get_keys().view_secret_key,
              stake_output_gindex, prev_block.timestamp, POS_SCAN_WINDOW, POS_SCAN_STEP);
    pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(prev_block), alice.get_public_address());
    pb.step5_sign(se, alice.get_keys());

    DO_CALLBACK(events, "mark_invalid_block");
    events.push_back(pb.m_block);
  }

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_3r));

  //  0     14    15    16    17    18    19    20     30    31                           <- blockchain height
  //        pow   pow   pow   pow   pow   pow   pos    pos   pos
  // (0 )- (1r)- (5 )- (6 )  (7 )- (8 )- (9 )- (2r).. (3r)- (31)                          <- main chain
  //             tx_0  tx_1  tx_2  tx_3  tx_4 tx_alice     invalid
  //                                             \
  //                                              \    pow    pos    pos
  //                                               \- (1a).. (3ra)- (31a)
  //                                                                valid
  // Expected: max_related_block_height (20) < last_pow_block_height (21)

  MAKE_NEXT_BLOCK(events, blk_1a, blk_2r, miner);  // last pow block
  REWIND_POS_BLOCKS_N_WITH_TIME(events, blk_3ra, blk_1a, miner, { miner }, CURRENCY_MINED_MONEY_UNLOCK_WINDOW - 1);

  // case 3: Provide valid tx output
  // Expected error: stake input refs' max related block height is 20 while last PoW block height is 21
  {
    const currency::block& prev_block = blk_3ra;
    const transaction& stake          = tx_to_alice;
    bool r                            = false;
    std::vector<tx_source_entry> sources;

    size_t height        = get_block_height(prev_block) + 1;
    crypto::hash prev_id = get_block_hash(prev_block);

    r = fill_tx_sources(
        sources,
        events,
        prev_block,
        alice.get_keys(),
        MK_TEST_COINS(10000),
        0,
        std::vector<tx_source_entry> {});

    CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");

    CHECK_AND_ASSERT_MES(shuffle_source_entries(sources), false, "");

    auto it = std::max_element(
        sources.begin(), sources.end(),
        [&](const tx_source_entry& lhs, const tx_source_entry& rhs)
        {
          return lhs.amount < rhs.amount;
        });
    const tx_source_entry& se               = *it;
    const tx_source_entry::output_entry& oe = se.outputs[se.real_output];

    crypto::key_image stake_output_key_image {};
    currency::keypair ephemeral_keys {};
    r = generate_key_image_helper(
        alice.get_keys(),
        se.real_out_tx_key,
        se.real_output_in_tx_index,
        ephemeral_keys,
        stake_output_key_image);
    CHECK_AND_ASSERT_MES(r, false, "generate_key_image_helper failed");

    uint64_t stake_output_gindex = boost::get<uint64_t>(oe.out_reference);
    currency::wide_difficulty_type pos_diff {};
    crypto::hash last_pow_block_hash {}, last_pos_block_kernel_hash {};
    r = generator.get_params_for_next_pos_block(
        prev_id,
        pos_diff,
        last_pow_block_hash,
        last_pos_block_kernel_hash);
    CHECK_AND_ASSERT_MES(r, false, "get_params_for_next_pos_block failed");

    pos_block_builder pb;
    pb.step1_init_header(generator.get_hardforks(), height, prev_id);
    pb.step2_set_txs(std::vector<transaction>());
    pb.step3a(pos_diff, last_pow_block_hash, last_pos_block_kernel_hash);
    pb.step3b(se.amount, stake_output_key_image, se.real_out_tx_key, se.real_output_in_tx_index, se.real_out_amount_blinding_mask, alice.get_keys().view_secret_key,
              stake_output_gindex, prev_block.timestamp, POS_SCAN_WINDOW, POS_SCAN_STEP);
    pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(prev_block), alice.get_public_address());
    pb.step5_sign(se, alice.get_keys());

    events.push_back(pb.m_block);

    DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(pb.m_block));
  }

  return true;
}
