// Copyright (c) 2025 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "hard_fork_6.h"

using namespace currency;

hard_fork_6_intrinsic_payment_id_basic_test::hard_fork_6_intrinsic_payment_id_basic_test()
{
  REGISTER_CALLBACK_METHOD(hard_fork_6_intrinsic_payment_id_basic_test, c1);

  m_hardforks.clear();
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_04_ZARCANUM, 1);
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_06, 10);
}

bool hard_fork_6_intrinsic_payment_id_basic_test::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: check basics of legacy payment id + intrinsic payment id combination.

  bool r = false;
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);
  account_base& carol_acc = m_accounts[CAROL_ACC_IDX]; carol_acc.generate(); carol_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);

  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_04_ZARCANUM));

  MAKE_TX(events, tx_0a, miner_acc, alice_acc, MK_TEST_COINS(100), blk_0r);
  MAKE_TX(events, tx_0b, miner_acc, alice_acc, MK_TEST_COINS(100), blk_0r);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({tx_0a, tx_0b}));

  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // register and asset
  transaction tx_1{};
  asset_descriptor_base adb_0{};
  {
    size_t current_hardfork_id = m_hardforks.get_the_most_recent_hardfork_id_for_height(get_block_height(blk_1r));
    adb_0.total_max_supply = 173;
    adb_0.full_name = "In fact, forget the park!";
    adb_0.ticker = "THEMEPARK";
    adb_0.decimal_point = 0;
    asset_descriptor_operation ado{};
    fill_ado_version_based_onhardfork(ado, current_hardfork_id);
    ado.opt_descriptor = adb_0;
    fill_adb_version_based_onhardfork(*ado.opt_descriptor, current_hardfork_id);
    ado.operation_type = ASSET_DESCRIPTOR_OPERATION_REGISTER;
    if (current_hardfork_id >= ZANO_HARDFORK_05)
      ado.opt_asset_id_salt = 1;
    std::vector<tx_destination_entry> destinations;
    destinations.emplace_back(10, m_accounts[ALICE_ACC_IDX].get_public_address(), null_pkey);
    destinations.emplace_back(10, m_accounts[ALICE_ACC_IDX].get_public_address(), null_pkey);
    r = construct_tx_to_key(m_hardforks, events, tx_1, blk_1r, alice_acc, destinations, TESTS_DEFAULT_FEE, 0, 0, std::vector<attachment_v>({ado}));
    CHECK_AND_ASSERT_MES(r, false, "construct_tx_to_key failed");
    // get and store asset id
    r = get_type_in_variant_container(tx_1.extra, ado);
    CHECK_AND_ASSERT_MES(r, false, "get_type_in_variant_container failed");
    r = get_or_calculate_asset_id(ado, nullptr, &m_asset_id);
    CHECK_AND_ASSERT_MES(r, false, "get_or_calculate_asset_id failed");
  }
  ADD_CUSTOM_EVENT(events, tx_1);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_1);

  REWIND_BLOCKS_N(events, blk_2r, blk_2, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_06));

  
  //
  // tx_2 : Alice -> Bob; only intrinsic payment ids
  //
  std::vector<tx_destination_entry> destinations;
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(11), bob_acc.get_public_address()));
  destinations.back().payment_id = 0; // effectively payment id is not set
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(11), bob_acc.get_public_address()));
  destinations.back().payment_id = 1;
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(11), bob_acc.get_public_address()));
  destinations.back().payment_id = 2;
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(11), bob_acc.get_public_address()));
  destinations.back().payment_id = 2;
  destinations.push_back(tx_destination_entry(5, bob_acc.get_public_address(), m_asset_id)); // an asset
  destinations.back().payment_id = 2;
  destinations.push_back(tx_destination_entry(1, bob_acc.get_public_address(), m_asset_id)); // an asset
  destinations.back().payment_id = 0; // effectively payment id is not set

  transaction tx_2{};
  r = construct_tx_to_key(m_hardforks, events, tx_2, blk_2r, alice_acc, destinations);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_to_key failed");
  ADD_CUSTOM_EVENT(events, tx_2);
  m_tx_2_id = get_transaction_hash(tx_2);


  //
  // tx_3 : Alice -> Carol; both legacy tx-wide payment id + intrinsig payment ids
  //
  destinations.clear();
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(11), carol_acc.get_public_address()));
  destinations.back().payment_id = 0; // effectively payment id is not set
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(11), carol_acc.get_public_address()));
  destinations.back().payment_id = 1;
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(11), carol_acc.get_public_address()));
  destinations.back().payment_id = 2;
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(11), carol_acc.get_public_address()));
  destinations.back().payment_id = 2;
  destinations.push_back(tx_destination_entry(5, carol_acc.get_public_address(), m_asset_id)); // an asset
  destinations.back().payment_id = 2;
  destinations.push_back(tx_destination_entry(1, carol_acc.get_public_address(), m_asset_id)); // an asset
  destinations.back().payment_id = 0; // effectively payment id is not set

  std::vector<payload_items_v> extra;
  payment_id_t tx_wide_payment_id = "abcdefghi";
  r = set_payment_id_to_tx(extra, tx_wide_payment_id, true);
  CHECK_AND_ASSERT_MES(r, false, "set_payment_id_to_tx failed");

  transaction tx_3{};
  r = construct_tx_to_key(m_hardforks, events, tx_3, blk_2r, alice_acc, destinations, TESTS_DEFAULT_FEE, 0, 0, extra);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_to_key failed");
  ADD_CUSTOM_EVENT(events, tx_3);
  m_tx_3_id = get_transaction_hash(tx_3);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3, blk_2r, miner_acc, std::list({ tx_2, tx_3 }));

  REWIND_BLOCKS_N(events, blk_3r, blk_3, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);


  DO_CALLBACK(events, "c1");

  return true;
}

bool hard_fork_6_intrinsic_payment_id_basic_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  bool r = false;
  
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  std::shared_ptr<tools::wallet2> carol_wlt = init_playtime_test_wallet(events, c, CAROL_ACC_IDX);
  
  bob_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", MK_TEST_COINS(44), UINT64_MAX, MK_TEST_COINS(44), 0, 0), false, "");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", 6, UINT64_MAX, 6, 0, 0, m_asset_id, 0), false, "");

  std::list<tools::payment_details> payments;
  bob_wlt->get_payments(convert_payment_id(1), payments);
  CHECK_AND_ASSERT_EQ(payments.size(), 1);
  CHECK_AND_ASSERT_EQ(payments.back().m_amount,             MK_TEST_COINS(11));
  CHECK_AND_ASSERT_EQ(payments.back().m_block_height,       34);
  CHECK_AND_ASSERT_EQ(payments.back().m_tx_hash,            m_tx_2_id);
  CHECK_AND_ASSERT_EQ(payments.back().m_unlock_time,        0);
  CHECK_AND_ASSERT_EQ(payments.back().subtransfers.size(),  0);

  payments.clear();
  bob_wlt->get_payments(convert_payment_id(2), payments);
  CHECK_AND_ASSERT_EQ(payments.size(), 1);
  CHECK_AND_ASSERT_EQ(payments.back().m_amount,             MK_TEST_COINS(22));
  CHECK_AND_ASSERT_EQ(payments.back().m_block_height,       34);
  CHECK_AND_ASSERT_EQ(payments.back().m_tx_hash,            m_tx_2_id);
  CHECK_AND_ASSERT_EQ(payments.back().m_unlock_time,        0);
  CHECK_AND_ASSERT_EQ(payments.back().subtransfers.size(),  1);
  CHECK_AND_ASSERT_EQ(payments.back().subtransfers.back().amount,  5);
  CHECK_AND_ASSERT_EQ(payments.back().subtransfers.back().asset_id, m_asset_id);

  tools::transfer_container transfers;
  bob_wlt->get_transfers(transfers);
  CHECK_AND_ASSERT_EQ(transfers.size(),                     6);

  std::vector<tools::wallet_public::wallet_transfer_info> wtis;
  uint64_t total = 0, last_item_index = 0;
  bob_wlt->get_recent_transfers_history(wtis, 0, 100, total, last_item_index, false /*exclude_mining_txs*/, false /*start_form_end*/);
  CHECK_AND_ASSERT_EQ(wtis.size(),                          1);
  CHECK_AND_ASSERT_EQ(wtis.back().height,                   34);
  CHECK_AND_ASSERT_EQ(wtis.back().comment,                  "");
  CHECK_AND_ASSERT_EQ(wtis.back().tx_hash,                  m_tx_2_id);
  CHECK_AND_ASSERT_EQ(wtis.back().fee,                      TX_DEFAULT_FEE);
  CHECK_AND_ASSERT_EQ(wtis.back().is_mining,                false);
  CHECK_AND_ASSERT_EQ(wtis.back().is_mixing,                false);
  CHECK_AND_ASSERT_EQ(wtis.back().is_service,               false);
  CHECK_AND_ASSERT_EQ(wtis.back().tx_type,                  GUI_TX_TYPE_NORMAL);
  CHECK_AND_ASSERT_EQ(wtis.back().unlock_time,              0);
  CHECK_AND_ASSERT_EQ(wtis.back().tx_wide_payment_id,       "");
  CHECK_AND_ASSERT_EQ(wtis.back().subtransfers_by_pid.size(), 3);

  std::unordered_map<std::string, std::unordered_map<crypto::public_key, tools::wallet_public::wallet_sub_transfer_info>> substransfers_grouped_by_pid_aid;
  for (auto& stbp : wtis.back().subtransfers_by_pid)
  {
    for (auto& st : stbp.subtransfers)
    {
      CHECK_AND_ASSERT_EQ(substransfers_grouped_by_pid_aid[stbp.payment_id].count(st.asset_id), 0);
      substransfers_grouped_by_pid_aid[stbp.payment_id][st.asset_id] = st;
    }
  }

  CHECK_AND_ASSERT_EQ(substransfers_grouped_by_pid_aid[convert_payment_id(0)][native_coin_asset_id].amount, MK_TEST_COINS(11));
  CHECK_AND_ASSERT_EQ(substransfers_grouped_by_pid_aid[convert_payment_id(0)][m_asset_id].amount,           1);
  CHECK_AND_ASSERT_EQ(substransfers_grouped_by_pid_aid[convert_payment_id(1)][native_coin_asset_id].amount, MK_TEST_COINS(11));
  CHECK_AND_ASSERT_EQ(substransfers_grouped_by_pid_aid[convert_payment_id(2)][native_coin_asset_id].amount, MK_TEST_COINS(22));
  CHECK_AND_ASSERT_EQ(substransfers_grouped_by_pid_aid[convert_payment_id(2)][m_asset_id].amount,           5);


  // Carol should have the same balances as Bob, but they are split differently into payments
  carol_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*carol_wlt.get(), "Carol", MK_TEST_COINS(44), UINT64_MAX, MK_TEST_COINS(44), 0, 0), false, "");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*carol_wlt.get(), "Carol", 6, UINT64_MAX, 6, 0, 0, m_asset_id, 0), false, "");

  payments.clear();
  carol_wlt->get_payments(convert_payment_id(1), payments);
  CHECK_AND_ASSERT_EQ(payments.size(), 0);

  carol_wlt->get_payments(convert_payment_id(2), payments);
  CHECK_AND_ASSERT_EQ(payments.size(), 0);

  carol_wlt->get_payments("abcdefghi", payments);
  CHECK_AND_ASSERT_EQ(payments.size(), 1);
  CHECK_AND_ASSERT_EQ(payments.back().m_amount,             MK_TEST_COINS(44));
  CHECK_AND_ASSERT_EQ(payments.back().m_block_height,       34);
  CHECK_AND_ASSERT_EQ(payments.back().m_tx_hash,            m_tx_3_id);
  CHECK_AND_ASSERT_EQ(payments.back().m_unlock_time,        0);
  CHECK_AND_ASSERT_EQ(payments.back().subtransfers.size(),  1);
  CHECK_AND_ASSERT_EQ(payments.back().subtransfers.back().amount,  6);
  CHECK_AND_ASSERT_EQ(payments.back().subtransfers.back().asset_id, m_asset_id);

  transfers.clear();
  carol_wlt->get_transfers(transfers);
  CHECK_AND_ASSERT_EQ(transfers.size(),                     6);

  wtis.clear();
  total = 0, last_item_index = 0;
  carol_wlt->get_recent_transfers_history(wtis, 0, 100, total, last_item_index, false /*exclude_mining_txs*/, false /*start_form_end*/);
  CHECK_AND_ASSERT_EQ(wtis.size(),                          1);
  CHECK_AND_ASSERT_EQ(wtis.back().height,                   34);
  CHECK_AND_ASSERT_EQ(wtis.back().comment,                  "");
  CHECK_AND_ASSERT_EQ(wtis.back().tx_hash,                  m_tx_3_id);
  CHECK_AND_ASSERT_EQ(wtis.back().fee,                      TX_DEFAULT_FEE);
  CHECK_AND_ASSERT_EQ(wtis.back().is_mining,                false);
  CHECK_AND_ASSERT_EQ(wtis.back().is_mixing,                false);
  CHECK_AND_ASSERT_EQ(wtis.back().is_service,               false);
  CHECK_AND_ASSERT_EQ(wtis.back().tx_type,                  GUI_TX_TYPE_NORMAL);
  CHECK_AND_ASSERT_EQ(wtis.back().unlock_time,              0);
  CHECK_AND_ASSERT_EQ(wtis.back().tx_wide_payment_id,       "abcdefghi");

  CHECK_AND_ASSERT_EQ(wtis.back().subtransfers_by_pid.size(), 1);

  substransfers_grouped_by_pid_aid.clear();
  for (auto& stbp : wtis.back().subtransfers_by_pid)
  {
    for (auto& st : stbp.subtransfers)
    {
      CHECK_AND_ASSERT_EQ(substransfers_grouped_by_pid_aid[stbp.payment_id].count(st.asset_id), 0);
      substransfers_grouped_by_pid_aid[stbp.payment_id][st.asset_id] = st;
    }
  }

  CHECK_AND_ASSERT_EQ(substransfers_grouped_by_pid_aid["abcdefghi"][native_coin_asset_id].amount, MK_TEST_COINS(44));
  CHECK_AND_ASSERT_EQ(substransfers_grouped_by_pid_aid["abcdefghi"][m_asset_id].amount,           6);
  CHECK_AND_ASSERT_EQ(substransfers_grouped_by_pid_aid.count(convert_payment_id(0)),              0);
  CHECK_AND_ASSERT_EQ(substransfers_grouped_by_pid_aid.count(convert_payment_id(1)),              0);
  CHECK_AND_ASSERT_EQ(substransfers_grouped_by_pid_aid.count(convert_payment_id(2)),              0);
 

  return true;
}
