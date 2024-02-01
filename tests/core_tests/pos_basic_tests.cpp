// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "offers_tests_common.h"
#include "pos_basic_tests.h"

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
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_0, get_tx_version_from_events(events), 0);
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
