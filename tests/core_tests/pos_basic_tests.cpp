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
