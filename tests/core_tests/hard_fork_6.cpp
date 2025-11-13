// Copyright (c) 2025 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "hard_fork_6.h"
#include "wallet/wallet_rpc_server.h"
#include "chaingen_helpers.h"
#include "random_helper.h"

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

//------------------------------------------------------------------------------

hard_fork_6_intrinsic_payment_id_rpc_test::hard_fork_6_intrinsic_payment_id_rpc_test()
{
  //random_state_test_restorer::reset_random(0);
  REGISTER_CALLBACK_METHOD(hard_fork_6_intrinsic_payment_id_rpc_test, c1);

  m_hardforks.clear();
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_04_ZARCANUM, 1);
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_06, 44);
}

bool hard_fork_6_intrinsic_payment_id_rpc_test::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: do various check for wallet and daemon RPC:
  // 1) lagecy tx-wide payment id is deprecated now, should return an error if set (regardless of CLI options);
  // 2) integrated address cannot be created anymore if payment id is longer than 8 bytes, unless --allow-legacy-payment-id-size is set (and => legacy payment id is used);
  // 3) if there is at least one element in destinations with integrated address, having payment id longer than 8 bytes, return an error, unless --allow-legacy-payment-id-size is set;
  // 4) different per-destination intrinsic payment ids can only be used since HF6;

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

  transaction tx_0{};
  CHECK_AND_ASSERT_SUCCESS(construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), MK_TEST_COINS(201), 10, TESTS_DEFAULT_FEE, tx_0));
  ADD_CUSTOM_EVENT(events, tx_0);
  m_tx_0_id = get_transaction_hash(tx_0);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({tx_0}));

  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // register and asset (tx_1: Alice to herself)
  transaction tx_1{};
  asset_descriptor_base adb_0{};
  {
    size_t current_hardfork_id = m_hardforks.get_the_most_recent_hardfork_id_for_height(get_block_height(blk_1r));
    adb_0.total_max_supply = 1836;
    adb_0.ticker = "DIPROTON";
    adb_0.full_name = "The strong force could be stronger!";
    adb_0.decimal_point = 0;
    asset_descriptor_operation ado{};
    fill_ado_version_based_onhardfork(ado, current_hardfork_id);
    ado.opt_descriptor = adb_0;
    fill_adb_version_based_onhardfork(*ado.opt_descriptor, current_hardfork_id);
    ado.operation_type = ASSET_DESCRIPTOR_OPERATION_REGISTER;
    if (current_hardfork_id >= ZANO_HARDFORK_05)
      ado.opt_asset_id_salt = 1;
    std::vector<tx_destination_entry> destinations;
    for(size_t i = 0; i < 12; ++i)
      destinations.emplace_back(10, m_accounts[ALICE_ACC_IDX].get_public_address(), null_pkey);
    destinations.emplace_back(MK_TEST_COINS(100), m_accounts[ALICE_ACC_IDX].get_public_address());
    r = construct_tx_to_key(m_hardforks, events, tx_1, blk_1r, alice_acc, destinations, TESTS_DEFAULT_FEE, 0, 0, std::vector<attachment_v>({ado}));
    CHECK_AND_ASSERT_MES(r, false, "construct_tx_to_key failed");
    // get and store asset id
    r = get_type_in_variant_container(tx_1.extra, ado);
    CHECK_AND_ASSERT_MES(r, false, "get_type_in_variant_container failed");
    r = get_or_calculate_asset_id(ado, nullptr, &m_asset_id);
    CHECK_AND_ASSERT_MES(r, false, "get_or_calculate_asset_id failed");
  }
  ADD_CUSTOM_EVENT(events, tx_1);
  m_tx_1_id = get_transaction_hash(tx_1);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_1);

  REWIND_BLOCKS_N(events, blk_2r, blk_2, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK(events, "c1");

  return true;
}

bool hard_fork_6_intrinsic_payment_id_rpc_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  bool r = false;

  payment_id_t long_legacy_payment_id(CURRENCY_HF6_INTRINSIC_PAYMENT_ID_SIZE + 1, '\0');
  crypto::generate_random_bytes(long_legacy_payment_id.size(), long_legacy_payment_id.data());
  std::string long_legacy_payment_id_hex = epee::string_tools::buff_to_hex_nodelimer(long_legacy_payment_id);
  payment_id_t short_payment_id  = convert_payment_id(1836);
  payment_id_t short_payment_id2 = convert_payment_id(1838);

  //
  // #2 check get_integrated_address
  //
  currency::t_currency_protocol_handler<currency::core> cph(c, nullptr);
  nodetool::node_server<currency::t_currency_protocol_handler<currency::core>> p2p(cph);
  bc_services::bc_offers_service os(nullptr); 
  currency::core_rpc_server core_rpc(c, p2p, os);
  core_rpc.set_ignore_connectivity_status(true);
  boost::program_options::options_description core_desc_options, wallet_desc_options;
  currency::core_rpc_server::init_options(core_desc_options);
  tools::wallet_rpc_server::init_options(wallet_desc_options);
  boost::program_options::variables_map vm_empty, vm_allow_legacy_pid_size_core, vm_allow_legacy_pid_size_wallet;
  const char* const argv_c[] = {"", "--allow-legacy-payment-id-size"};
  boost::program_options::store(boost::program_options::parse_command_line(sizeof argv_c / sizeof argv_c[0], argv_c, core_desc_options), vm_allow_legacy_pid_size_core);
  const char* const argv_w[] = {"", "--allow-legacy-payment-id-size", "--rpc-bind-port=0"};
  boost::program_options::store(boost::program_options::parse_command_line(sizeof argv_w / sizeof argv_w[0], argv_w, wallet_desc_options), vm_allow_legacy_pid_size_wallet);
  const char* const argv_e[] = {"", "--rpc-bind-port=0"};
  boost::program_options::store(boost::program_options::parse_command_line(sizeof argv_e / sizeof argv_e[0], argv_e, wallet_desc_options), vm_empty);
  
  COMMAND_RPC_GET_INTEGRATED_ADDRESS::request  gia_req{};
  COMMAND_RPC_GET_INTEGRATED_ADDRESS::response gia_res{};
  gia_req.regular_address = m_accounts[MINER_ACC_IDX].get_public_address_str();
  // this payment id should be considered as too long unless CLI option is set
  gia_req.payment_id = long_legacy_payment_id_hex;
  r = invoke_text_json_for_rpc(core_rpc, "get_integrated_address", gia_req, gia_res);
  CHECK_AND_ASSERT_EQ(r, false);
  // re-init RPC server with the CLI option
  r = core_rpc.deinit();
  CHECK_AND_ASSERT_EQ(r, true);
  r = core_rpc.init(vm_allow_legacy_pid_size_core);
  CHECK_AND_ASSERT_EQ(r, true);
  // now it should work fine
  r = invoke_text_json_for_rpc(core_rpc, "get_integrated_address", gia_req, gia_res);
  CHECK_AND_ASSERT_EQ(r, true);


  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  miner_wlt->refresh();
  tools::wallet_rpc_server miner_wlt_rpc(miner_wlt);

  // verify obtained integrated address
  tools::wallet_public::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::request sia_req{};
  sia_req.integrated_address = gia_res.integrated_address;
  tools::wallet_public::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::response sia_res{};
  r = invoke_text_json_for_rpc(miner_wlt_rpc, "split_integrated_address", sia_req, sia_res);
  CHECK_AND_ASSERT_EQ(r, true);
  CHECK_AND_ASSERT_EQ(sia_res.standard_address, gia_req.regular_address);
  CHECK_AND_ASSERT_EQ(sia_res.payment_id, gia_req.payment_id);


  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->set_concise_mode(false);
  alice_wlt->refresh();
  tools::wallet_rpc_server alice_wlt_rpc(alice_wlt);
  
  CHECK_AND_ASSERT_SUCCESS(check_balance_via_wallet(*alice_wlt.get(), "Alice", MK_TEST_COINS(200), INVALID_BALANCE_VAL, MK_TEST_COINS(200), 0, 0));
  CHECK_AND_ASSERT_SUCCESS(check_balance_via_wallet(*alice_wlt.get(), "Alice", 120, INVALID_BALANCE_VAL, 120, 0, 0, m_asset_id, 0));

  std::vector<crypto::hash> successfull_txs;

  tools::wallet_public::COMMAND_RPC_TRANSFER::request tr_req{};
  tr_req.comment = "stars continue to operate normally";
  tr_req.payment_id = "1836";
  tr_req.fee = TX_DEFAULT_FEE;
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{MK_TEST_COINS(1), m_accounts[BOB_ACC_IDX].get_public_address_str()});
  tools::wallet_public::COMMAND_RPC_TRANSFER::response tr_resp{};
  CHECK_AND_ASSERT_FAILURE(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));
  // without payment_id it should work
  tr_req.payment_id.clear();
  CHECK_AND_ASSERT_SUCCESS(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));
  successfull_txs.push_back(crypto::parse_tpod_from_hex_string<crypto::hash>(tr_resp.tx_hash)); // 0

  // check any destination cannot has an intrinsic payment id
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{MK_TEST_COINS(2), m_accounts[BOB_ACC_IDX].get_public_address_str()});
  tr_req.destinations.front().payment_id = 137;
  CHECK_AND_ASSERT_FAILURE(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{MK_TEST_COINS(2), m_accounts[BOB_ACC_IDX].get_public_address_str()});
  tr_req.destinations.front().payment_id = 0;
  tr_req.destinations.back().payment_id = 1836;
  CHECK_AND_ASSERT_FAILURE(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));
  tr_req.destinations.back().payment_id = 0;
  // eventually it should go well with no intrinsic pid
  CHECK_AND_ASSERT_SUCCESS(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));
  successfull_txs.push_back(crypto::parse_tpod_from_hex_string<crypto::hash>(tr_resp.tx_hash)); // 1
  
  //
  // checks with no any CLI options for wallet RPC server
  //
  std::string bob_addr = m_accounts[BOB_ACC_IDX].get_public_address_str();
  std::string bob_addr_with_too_long_pid = get_account_address_and_payment_id_as_str(m_accounts[BOB_ACC_IDX].get_public_address(), long_legacy_payment_id);
  std::string bob_addr_with_short_pid = get_account_address_and_payment_id_as_str(m_accounts[BOB_ACC_IDX].get_public_address(), short_payment_id);
  std::string bob_addr_with_short_pid2 = get_account_address_and_payment_id_as_str(m_accounts[BOB_ACC_IDX].get_public_address(), short_payment_id2);
  std::string carol_addr = m_accounts[CAROL_ACC_IDX].get_public_address_str();

  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{3, bob_addr_with_too_long_pid, m_asset_id});
  CHECK_AND_ASSERT_FAILURE(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));

  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{3, bob_addr_with_short_pid, m_asset_id});
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{3, bob_addr_with_too_long_pid, m_asset_id});
  CHECK_AND_ASSERT_FAILURE(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));
  
  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{3, bob_addr_with_short_pid, m_asset_id});
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{3, bob_addr_with_short_pid, m_asset_id});
  CHECK_AND_ASSERT_FAILURE(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));
  
  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{3, bob_addr, m_asset_id});
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{3, bob_addr_with_short_pid, m_asset_id});
  CHECK_AND_ASSERT_FAILURE(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));

  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{3, bob_addr_with_short_pid, m_asset_id});
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{3, bob_addr, m_asset_id});
  CHECK_AND_ASSERT_SUCCESS(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));
  successfull_txs.push_back(crypto::parse_tpod_from_hex_string<crypto::hash>(tr_resp.tx_hash)); // 2

  //
  // checks with --allow-legacy-payment-id-size specified
  //
  // re-init Alice's wallet RPC server with the corresponding CLI option
  CHECK_AND_ASSERT_SUCCESS(alice_wlt_rpc.deinit());
  CHECK_AND_ASSERT_SUCCESS(alice_wlt_rpc.init(vm_allow_legacy_pid_size_wallet));

  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{4, bob_addr_with_too_long_pid, m_asset_id});
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{4, bob_addr_with_short_pid, m_asset_id});
  CHECK_AND_ASSERT_FAILURE(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));

  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{4, bob_addr_with_short_pid, m_asset_id});
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{4, bob_addr_with_too_long_pid, m_asset_id});
  CHECK_AND_ASSERT_FAILURE(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));
  
  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{4, bob_addr_with_short_pid, m_asset_id});
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{4, bob_addr_with_short_pid, m_asset_id});
  CHECK_AND_ASSERT_FAILURE(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));
  
  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{4, bob_addr, m_asset_id});
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{4, bob_addr_with_short_pid, m_asset_id});
  CHECK_AND_ASSERT_FAILURE(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));

  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{4, bob_addr_with_too_long_pid, m_asset_id});
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{4, bob_addr, m_asset_id});
  CHECK_AND_ASSERT_SUCCESS(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));
  successfull_txs.push_back(crypto::parse_tpod_from_hex_string<crypto::hash>(tr_resp.tx_hash)); // 3


  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 4);
  mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  //
  // activate HF6
  //
  CHECK_AND_ASSERT_FALSE(c.get_blockchain_storage().is_hardfork_active(ZANO_HARDFORK_06));
  mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_TRUE(c.get_blockchain_storage().is_hardfork_active(ZANO_HARDFORK_06));

  alice_wlt->refresh();

  // re-init Alice's wallet RPC server
  CHECK_AND_ASSERT_SUCCESS(alice_wlt_rpc.deinit());
  CHECK_AND_ASSERT_SUCCESS(alice_wlt_rpc.init(vm_empty));

  // re-check that payment_id is deprecated after HF6 as well
  tr_req = tools::wallet_public::COMMAND_RPC_TRANSFER::request{};
  tr_req.comment = "stars continue to operate normally";
  tr_req.payment_id = "1836";
  tr_req.fee = TX_DEFAULT_FEE;
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{MK_TEST_COINS(5), m_accounts[BOB_ACC_IDX].get_public_address_str()});
  CHECK_AND_ASSERT_FAILURE(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));
  // without payment_id it should work, as before
  tr_req.payment_id.clear();
  CHECK_AND_ASSERT_SUCCESS(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));
  successfull_txs.push_back(crypto::parse_tpod_from_hex_string<crypto::hash>(tr_resp.tx_hash)); // 4


  // checks without any CLI options specified for wallet RPC
  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{6, bob_addr_with_too_long_pid, m_asset_id}); // long pid needs CLI option
  CHECK_AND_ASSERT_FAILURE(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));

  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{6, bob_addr_with_short_pid, m_asset_id});
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{6, bob_addr_with_too_long_pid, m_asset_id});
  CHECK_AND_ASSERT_FAILURE(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));
  
  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{6, bob_addr, m_asset_id});
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{6, bob_addr_with_short_pid, m_asset_id});  // integrated address in the second destination
  CHECK_AND_ASSERT_SUCCESS(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));
  successfull_txs.push_back(crypto::parse_tpod_from_hex_string<crypto::hash>(tr_resp.tx_hash)); // 5

  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{7, bob_addr_with_short_pid, m_asset_id});
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{7, bob_addr, m_asset_id, 18361836});       // with intrinsic pid specified separately
  CHECK_AND_ASSERT_SUCCESS(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));
  successfull_txs.push_back(crypto::parse_tpod_from_hex_string<crypto::hash>(tr_resp.tx_hash)); // 6

  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{8, bob_addr_with_short_pid, m_asset_id});
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{8, bob_addr_with_short_pid2, m_asset_id});
  CHECK_AND_ASSERT_SUCCESS(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));
  successfull_txs.push_back(crypto::parse_tpod_from_hex_string<crypto::hash>(tr_resp.tx_hash)); // 7
  

  //
  // checks with --allow-legacy-payment-id-size specified
  //
  // re-init Alice's wallet RPC server with the corresponding CLI option
  CHECK_AND_ASSERT_SUCCESS(alice_wlt_rpc.deinit());
  CHECK_AND_ASSERT_SUCCESS(alice_wlt_rpc.init(vm_allow_legacy_pid_size_wallet));

  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{8, bob_addr_with_too_long_pid, m_asset_id, 111111}); // long pid is incompatible with intrinsic pid
  CHECK_AND_ASSERT_FAILURE(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));

  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{8, bob_addr_with_too_long_pid, m_asset_id});
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{8, bob_addr, m_asset_id, 18361836});       // long pid is incompatible with intrinsic pid specified separately
  CHECK_AND_ASSERT_FAILURE(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));

  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{8, bob_addr_with_too_long_pid, m_asset_id});
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{8, bob_addr_with_short_pid, m_asset_id});  // long pid is incompatible with intrinsic pid from integrated address
  CHECK_AND_ASSERT_FAILURE(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));

  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{8, bob_addr_with_short_pid, m_asset_id});  // long pid is incompatible with intrinsic pid from integrated address
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{8, bob_addr_with_too_long_pid, m_asset_id});
  CHECK_AND_ASSERT_FAILURE(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));

  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{8, bob_addr_with_too_long_pid, m_asset_id});
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{8, bob_addr_with_too_long_pid, m_asset_id});  // long pid is incompatible with intrinsic pid from the first destination
  CHECK_AND_ASSERT_FAILURE(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));

  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{8, bob_addr_with_too_long_pid, m_asset_id});  // this
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{8, bob_addr, m_asset_id});                    // and this will be received by Bob with the long legacy payment id, because it's one per tx
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{8, carol_addr, m_asset_id});                  // carol should got receive the same payment id
  CHECK_AND_ASSERT_SUCCESS(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));
  successfull_txs.push_back(crypto::parse_tpod_from_hex_string<crypto::hash>(tr_resp.tx_hash)); // 8

  tr_req.destinations.clear();
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{9, bob_addr_with_short_pid, m_asset_id});
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{9, bob_addr_with_short_pid2, m_asset_id});
  tr_req.destinations.emplace_back(tools::wallet_public::transfer_destination{9, bob_addr, m_asset_id, 18361836});
  CHECK_AND_ASSERT_SUCCESS(invoke_text_json_for_rpc(alice_wlt_rpc, "transfer", tr_req, tr_resp));
  successfull_txs.push_back(crypto::parse_tpod_from_hex_string<crypto::hash>(tr_resp.tx_hash)); // 9


  // mine blocks to confirm transactions
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 6);
  mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  //
  // check what Bob has received
  //
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  bob_wlt->refresh();
  LOG_PRINT_L0("Bob's transfers:" << ENDL << bob_wlt->dump_transfers());
  
  // balance
  CHECK_AND_ASSERT_SUCCESS(check_balance_via_wallet(*bob_wlt.get(), "Bob", MK_TEST_COINS(11), INVALID_BALANCE_VAL, MK_TEST_COINS(11), 0, 0));
  CHECK_AND_ASSERT_SUCCESS(check_balance_via_wallet(*bob_wlt.get(), "Bob", 99, INVALID_BALANCE_VAL, 99, 0, 0, m_asset_id, 0));

  // then transfers history (WTIs)
  std::vector<tools::wallet_public::wallet_transfer_info> wtis;
  uint64_t total = 0, last_item_index = 0;
  bob_wlt->get_recent_transfers_history(wtis, 0, 100, total, last_item_index, false /*exclude_mining_txs*/, false /*start_form_end*/);
  CHECK_AND_ASSERT_EQ(wtis.size(), 10);
  CHECK_AND_ASSERT_EQ(successfull_txs.size(), 10);

  std::vector<std::unordered_map<std::string, std::unordered_map<crypto::public_key, tools::wallet_public::wallet_sub_transfer_info>>> wtis_substransfers_grouped_by_pid_aid; // you'd better watch your micro-aggression's, bro!
  std::vector<size_t> bm; // back mapping

  auto populate_wtis_map_and_bm = [&]() -> bool {
    wtis_substransfers_grouped_by_pid_aid.clear();
    wtis_substransfers_grouped_by_pid_aid.resize(wtis.size());
    bm.clear();
    bm.resize(successfull_txs.size());

    for(size_t i = 0; i < successfull_txs.size(); ++i)
    {
      size_t idx = std::find_if(wtis.begin(), wtis.end(), [&](auto& wti){ return wti.tx_hash == successfull_txs[i]; }) - wtis.begin();
      CHECK_AND_ASSERT_EQ(bm[i], 0);
      bm[i] = idx;
      auto& wti = wtis[idx];
      LOG_PRINT_L1("");
      LOG_PRINT_L1("Checking successfull_txs["<< i << "], wtis[" << idx << "]...");
      CHECK_AND_ASSERT_EQ(wti.fee,                      TX_DEFAULT_FEE);
      CHECK_AND_ASSERT_EQ(wti.is_mining,                false);
      CHECK_AND_ASSERT_EQ(wti.is_mixing,                false);
      CHECK_AND_ASSERT_EQ(wti.is_service,               false);
      CHECK_AND_ASSERT_EQ(wti.tx_type,                  GUI_TX_TYPE_NORMAL);
      CHECK_AND_ASSERT_EQ(wti.unlock_time,              0);

      LOG_PRINT_L0(
        ENDL << "  height             " << wti.height <<
        ENDL << "  comment            " << wti.comment <<
        ENDL << "  tx_hash            " << wti.tx_hash <<
        ENDL << "  tx_wide_payment_id " << epee::string_tools::buff_to_hex_nodelimer(wti.tx_wide_payment_id) <<
        ENDL << "  subtransfers sz    " << wti.subtransfers_by_pid.size()
      );

      for (auto& stbp : wti.subtransfers_by_pid)
      {
        for (auto& st : stbp.subtransfers)
        {
          CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][stbp.payment_id].count(st.asset_id), 0);
          wtis_substransfers_grouped_by_pid_aid[idx][stbp.payment_id][st.asset_id] = st;
          LOG_PRINT_L0("    pid=" << epee::string_tools::buff_to_hex_nodelimer(stbp.payment_id) << ", aid=" << st.asset_id << ", " << (st.is_income ? " IN" : "OUT") << ", amount=" << st.amount);
        }
      }
    }
    return true;
  };

  CHECK_AND_ASSERT_SUCCESS(populate_wtis_map_and_bm());

  size_t idx = bm[0];
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     34);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    tr_req.comment);
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         "");
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 1);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][native_coin_asset_id].amount,     MK_TEST_COINS(1));
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][native_coin_asset_id].is_income,  true);

  idx = bm[1];
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     34);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    tr_req.comment);
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         "");
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 1);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][native_coin_asset_id].amount,     MK_TEST_COINS(5));
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][native_coin_asset_id].is_income,  true);

  idx = bm[2];
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     34);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    "");
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         short_payment_id);
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 1);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][short_payment_id][m_asset_id].amount,                    6);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][short_payment_id][m_asset_id].is_income,                 true);

  idx = bm[3];
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     34);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    "");
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         long_legacy_payment_id);
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 1);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][long_legacy_payment_id][m_asset_id].amount,              8);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][long_legacy_payment_id][m_asset_id].is_income,           true);

  idx = bm[4];
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     45);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    tr_req.comment);
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         "");
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 1);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][native_coin_asset_id].amount,     MK_TEST_COINS(5));
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][native_coin_asset_id].is_income,  true);

  idx = bm[5];
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     45);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    tr_req.comment);
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         "");
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 2);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][m_asset_id].amount,               6);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][m_asset_id].is_income,            true);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][short_payment_id][m_asset_id].amount,                    6);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][short_payment_id][m_asset_id].is_income,                 true);

  idx = bm[6];
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     45);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    tr_req.comment);
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         "");
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 2);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][short_payment_id][m_asset_id].amount,                    7);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][short_payment_id][m_asset_id].is_income,                 true);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(18361836)][m_asset_id].amount,        7);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(18361836)][m_asset_id].is_income,     true);

  idx = bm[7];
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     45);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    tr_req.comment);
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         "");
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 2);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][short_payment_id][m_asset_id].amount,                    8);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][short_payment_id][m_asset_id].is_income,                 true);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][short_payment_id2][m_asset_id].amount,                   8);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][short_payment_id2][m_asset_id].is_income,                true);

  idx = bm[8];
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     45);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    tr_req.comment);
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         long_legacy_payment_id);
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 1);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][long_legacy_payment_id][m_asset_id].amount,              16);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][long_legacy_payment_id][m_asset_id].is_income,           true);

  idx = bm[9];
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     45);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    tr_req.comment);
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         "");
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 3);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][short_payment_id][m_asset_id].amount,                    9);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][short_payment_id][m_asset_id].is_income,                 true);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][short_payment_id2][m_asset_id].amount,                   9);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][short_payment_id2][m_asset_id].is_income,                true);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(18361836)][m_asset_id].amount,        9);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(18361836)][m_asset_id].is_income,     true);


  // Carol should receive one output
  std::shared_ptr<tools::wallet2> carol_wlt = init_playtime_test_wallet(events, c, CAROL_ACC_IDX);
  carol_wlt->refresh();
  LOG_PRINT_L0("Carol's transfers:" << ENDL << carol_wlt->dump_transfers());
  
  // balance
  CHECK_AND_ASSERT_SUCCESS(check_balance_via_wallet(*carol_wlt.get(), "Carol", 0, INVALID_BALANCE_VAL, 0, 0, 0));
  CHECK_AND_ASSERT_SUCCESS(check_balance_via_wallet(*carol_wlt.get(), "Carol", 8, INVALID_BALANCE_VAL, 8, 0, 0, m_asset_id, 0));

  wtis.clear();
  total = 0, last_item_index = 0;
  carol_wlt->get_recent_transfers_history(wtis, 0, 100, total, last_item_index, false /*exclude_mining_txs*/, false /*start_form_end*/);
  CHECK_AND_ASSERT_EQ(wtis.size(), 1);
  auto& wti = wtis.back();
  CHECK_AND_ASSERT_EQ(wti.fee,                      TX_DEFAULT_FEE);
  CHECK_AND_ASSERT_EQ(wti.is_mining,                false);
  CHECK_AND_ASSERT_EQ(wti.is_mixing,                false);
  CHECK_AND_ASSERT_EQ(wti.is_service,               false);
  CHECK_AND_ASSERT_EQ(wti.tx_type,                  GUI_TX_TYPE_NORMAL);
  CHECK_AND_ASSERT_EQ(wti.unlock_time,              0);
  
  CHECK_AND_ASSERT_EQ(wti.tx_wide_payment_id.size(), long_legacy_payment_id.size());
  CHECK_AND_ASSERT_NEQ(wti.tx_wide_payment_id,      long_legacy_payment_id); // Carol receives garbish payment id of the same size
  CHECK_AND_ASSERT_EQ(wti.comment.size(),           tr_req.comment.size());
  CHECK_AND_ASSERT_NEQ(wti.comment,                 tr_req.comment); // Carol receives garbish comment of the same size

  CHECK_AND_ASSERT_EQ(wti.employed_entries.receive.size(),                          1);
  CHECK_AND_ASSERT_EQ(wti.employed_entries.spent.size(),                            0);
  CHECK_AND_ASSERT_EQ(wti.employed_entries.receive.back().amount,                   8);
  CHECK_AND_ASSERT_EQ(wti.employed_entries.receive.back().asset_id,                 m_asset_id);
  CHECK_AND_ASSERT_EQ(wti.subtransfers_by_pid.size(),                               1);
  CHECK_AND_ASSERT_EQ(wti.subtransfers_by_pid.back().payment_id.size(),             long_legacy_payment_id.size());
  CHECK_AND_ASSERT_NEQ(wti.subtransfers_by_pid.back().payment_id,                   long_legacy_payment_id); // Carol receives garbish payment id of the same size
  CHECK_AND_ASSERT_EQ(wti.subtransfers_by_pid.back().subtransfers.size(),           1);
  CHECK_AND_ASSERT_EQ(wti.subtransfers_by_pid.back().subtransfers.back().amount,    8);
  CHECK_AND_ASSERT_EQ(wti.subtransfers_by_pid.back().subtransfers.back().asset_id,  m_asset_id);
  CHECK_AND_ASSERT_EQ(wti.subtransfers_by_pid.back().subtransfers.back().is_income, true);

  //
  // Final: check Alice's WTIs on fresh new wallet (she is always a sender)
  //

  std::shared_ptr<tools::wallet2> alice_wlt2 = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt2->set_concise_mode(false);
  alice_wlt2->refresh();
  wtis.clear();
  total = 0, last_item_index = 0;
  alice_wlt2->get_recent_transfers_history(wtis, 0, 100, total, last_item_index, false /*exclude_mining_txs*/, false /*start_form_end*/);
  CHECK_AND_ASSERT_EQ(wtis.size(),                          12);

  CHECK_AND_ASSERT_SUCCESS(populate_wtis_map_and_bm());

  idx = bm[0];
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     34);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    tr_req.comment);
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         "");
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 1);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][native_coin_asset_id].amount,     MK_TEST_COINS(2));
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][native_coin_asset_id].is_income,  false);

  idx = bm[1];
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     34);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    tr_req.comment);
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         "");
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 1);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][native_coin_asset_id].amount,     MK_TEST_COINS(6));
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][native_coin_asset_id].is_income,  false);

  idx = bm[2];
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     34);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    "");
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         short_payment_id);
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 1);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][short_payment_id][native_coin_asset_id].amount,          MK_TEST_COINS(1));
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][short_payment_id][native_coin_asset_id].is_income,       false);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][short_payment_id][m_asset_id].amount,                    6);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][short_payment_id][m_asset_id].is_income,                 false);

  idx = bm[3];
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     34);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    "");
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         long_legacy_payment_id);
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 1);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][long_legacy_payment_id][native_coin_asset_id].amount,    MK_TEST_COINS(1));
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][long_legacy_payment_id][native_coin_asset_id].is_income, false);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][long_legacy_payment_id][m_asset_id].amount,              8);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][long_legacy_payment_id][m_asset_id].is_income,           false);

  idx = bm[4];
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     45);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    tr_req.comment);
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         "");
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 1);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][native_coin_asset_id].amount,     MK_TEST_COINS(6));
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][native_coin_asset_id].is_income,  false);

  idx = bm[5];
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     45);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    tr_req.comment);
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         "");
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 1);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][native_coin_asset_id].amount,     MK_TEST_COINS(1));
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][native_coin_asset_id].is_income,  false);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][m_asset_id].amount,               12);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][m_asset_id].is_income,            false);

  idx = bm[6];
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     45);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    tr_req.comment);
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         "");
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 1);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][native_coin_asset_id].amount,     MK_TEST_COINS(1));
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][native_coin_asset_id].is_income,  false);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][m_asset_id].amount,               14);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][m_asset_id].is_income,            false);

  idx = bm[7];
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     45);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    tr_req.comment);
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         "");
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 1);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][native_coin_asset_id].amount,     MK_TEST_COINS(1));
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][native_coin_asset_id].is_income,  false);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][m_asset_id].amount,               16);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][m_asset_id].is_income,            false);

  idx = bm[8];
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     45);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    tr_req.comment);
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         long_legacy_payment_id);
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 1);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][long_legacy_payment_id][native_coin_asset_id].amount,    MK_TEST_COINS(1));
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][long_legacy_payment_id][native_coin_asset_id].is_income, false);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][long_legacy_payment_id][m_asset_id].amount,              24);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][long_legacy_payment_id][m_asset_id].is_income,           false);

  idx = bm[9];
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     45);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    tr_req.comment);
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         "");
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 1);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][native_coin_asset_id].amount,     MK_TEST_COINS(1));
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][native_coin_asset_id].is_income,  false);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][m_asset_id].amount,               27);
  CHECK_AND_ASSERT_EQ(wtis_substransfers_grouped_by_pid_aid[idx][convert_payment_id(0)][m_asset_id].is_income,            false);

  // tx_0
  idx = std::find_if(wtis.begin(), wtis.end(), [&](auto& wti){ return wti.tx_hash == m_tx_0_id; }) - wtis.begin();
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     12);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    "");
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         "");
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 1);
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.back().payment_id,                                                    "");
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.back().subtransfers.size(),                                           1);
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.back().subtransfers.back().amount,                                    MK_TEST_COINS(201));
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.back().subtransfers.back().asset_id,                                  native_coin_asset_id);
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.back().subtransfers.back().is_income,                                 true);

  // tx_1
  idx = std::find_if(wtis.begin(), wtis.end(), [&](auto& wti){ return wti.tx_hash == m_tx_1_id; }) - wtis.begin();
  CHECK_AND_ASSERT_EQ(wtis[idx].height,                     23);
  CHECK_AND_ASSERT_EQ(wtis[idx].comment,                    "");
  CHECK_AND_ASSERT_EQ(wtis[idx].tx_wide_payment_id,         "");
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.size(), 1);
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.back().payment_id,                                                    "");
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.back().subtransfers.size(),                                           2);
  const auto& st_container = wtis[idx].subtransfers_by_pid.back().subtransfers;
  size_t jdx = std::find_if(st_container.begin(), st_container.end(), [&](auto& st){ return st.asset_id == native_coin_asset_id; }) - st_container.begin();
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.back().subtransfers[jdx].amount,                                      MK_TEST_COINS(1));
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.back().subtransfers[jdx].asset_id,                                    native_coin_asset_id);
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.back().subtransfers[jdx].is_income,                                   false);
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.back().subtransfers[1-jdx].amount,                                    120);
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.back().subtransfers[1-jdx].asset_id,                                  m_asset_id);
  CHECK_AND_ASSERT_EQ(wtis[idx].subtransfers_by_pid.back().subtransfers[1-jdx].is_income,                                 true);

  return true;
}
