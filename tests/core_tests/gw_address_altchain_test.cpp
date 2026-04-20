// Copyright (c) 2025-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "gw_address_altchain_test.h"
#include "chaingen_helpers.h"

using namespace currency;

//
// gateway address alt-chain tests
//
// A- A- A- B- B- B- B-    <-- main chain
//       |
//       \- C- C- C-       <-- alt chain
//          ^
//          |
//     split height
//

static bool sign_gw_inputs(transaction& tx, const crypto::public_key& gw_view_pub, const keypair& gw_spend_kp)
{
  crypto::hash tx_hash_for_signing = get_transaction_hash(tx);
  for (size_t i = 0; i < tx.signatures.size(); ++i)
  {
    auto& sig = tx.signatures[i];
    if (sig.type() != typeid(gateway_sig))
      continue;
    const auto& in_v = tx.vin[i];
    CHECKED_GET_SPECIFIC_VARIANT(in_v, const txin_gateway, in_gw, false);
    if (in_gw.gateway_addr != gw_view_pub)
      continue;
    gateway_sig& gws = boost::get<gateway_sig>(sig);
    CHECKED_GET_SPECIFIC_VARIANT(gws.s, crypto::generic_schnorr_sig_s, gsss, false);
    crypto::generic_schnorr_sig& gss = static_cast<crypto::generic_schnorr_sig&>(gsss);
    CHECK_AND_ASSERT_TRUE(crypto::generate_schnorr_sig<crypto::gt_G>(tx_hash_for_signing, crypto::point_t(gw_spend_kp.pub), crypto::scalar_t(gw_spend_kp.sec), gss));
  }
  return true;
}

// construct a tx that spends gw_spend_amount from a GW address to recipient, ZC source covers fee only
static bool construct_gw_spend_tx(
  const hard_forks_descriptor& hardforks,
  std::vector<test_event_entry>& events,
  const block& head_block,
  const account_base& fee_payer,
  const crypto::public_key& gw_view_pub,
  const keypair& gw_spend_kp,
  uint64_t gw_spend_amount,
  const account_public_address& recipient,
  transaction& result_tx)
{
  bool r = false;

  std::unordered_map<crypto::public_key, uint64_t> amounts;
  amounts[native_coin_asset_id] = TESTS_DEFAULT_FEE;
  std::vector<tx_source_entry> sources;
  r = fill_tx_sources(sources, events, head_block, fee_payer.get_keys(), amounts, 0, std::vector<tx_source_entry>(), fts_default);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");

  std::unordered_map<crypto::public_key, uint64_t> found_amounts;
  get_sources_total_amount(sources, found_amounts);
  uint64_t change = found_amounts[native_coin_asset_id] - TESTS_DEFAULT_FEE;

  {
    tx_source_entry& tse = sources.emplace_back();
    tse.gateway_origin = gw_view_pub;
    tse.asset_id = native_coin_asset_id;
    tse.amount = gw_spend_amount;
  }

  std::vector<tx_destination_entry> destinations;
  destinations.emplace_back(gw_spend_amount, recipient);
  if (change > 0)
    destinations.emplace_back(change, fee_payer.get_public_address());

  if (destinations.size() < CURRENCY_TX_MIN_ALLOWED_OUTS)
  {
    tx_destination_entry de = destinations.back();
    destinations.pop_back();
    size_t items_to_add = CURRENCY_TX_MIN_ALLOWED_OUTS - destinations.size();
    currency::decompose_amount_randomly(de.amount, [&](uint64_t amount)
    {
      de.amount = amount;
      destinations.push_back(de);
    }, items_to_add);
  }

  size_t tx_hardfork_id{};
  uint64_t tx_version = get_tx_version_and_hardfork_id(get_block_height(head_block) + 1, hardforks, tx_hardfork_id);
  crypto::secret_key tx_key{};
  r = construct_tx(fee_payer.get_keys(), sources, destinations, empty_extra, empty_attachment, result_tx, tx_version, tx_hardfork_id, tx_key, 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

  r = sign_gw_inputs(result_tx, gw_view_pub, gw_spend_kp);
  CHECK_AND_ASSERT_MES(r, false, "sign_gw_inputs failed");

  return true;
}


//------------------------------------------------------------------------------
gw_addr_altchain_spend_in_both_chains::gw_addr_altchain_spend_in_both_chains()
{
  REGISTER_CALLBACK_METHOD(gw_addr_altchain_spend_in_both_chains, c1);

  m_hardforks.clear();
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_04_ZARCANUM, 1);
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_06, 1);
}

bool gw_addr_altchain_spend_in_both_chains::generate(std::vector<test_event_entry>& events) const
{
  // GW address created and funded in common chain A
  // Chain B: two spends (3, 5), Chain C: two spends (4, 2)
  // switch between B and C, checking balances at each step

  bool r = false;
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_06));

  // fund alice for fees
  transaction tx_fund{};
  CHECK_AND_ASSERT_SUCCESS(construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), MK_TEST_COINS(200), 10, TESTS_DEFAULT_FEE, tx_fund));
  ADD_CUSTOM_EVENT(events, tx_fund);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_fund);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // register gw address
  m_gw_addr_view  = keypair::generate();
  m_gw_addr_spend = keypair::generate();

  gateway_address_descriptor_operation gwdo{};
  gateway_address_descriptor_operation_register gwdo_reg{};
  gwdo_reg.view_pub_key = m_gw_addr_view.pub;
  gwdo_reg.descriptor.meta_info = "test_gw_addr_altchain_1";
  gwdo_reg.descriptor.owner_key = m_gw_addr_spend.pub;
  gwdo.operation = gwdo_reg;
  MAKE_TX_EXTRA_ATTACH_FEE(events, tx_reg, miner_acc, miner_acc, 0, CURRENCY_GATEWAY_ADDRESS_REGISTRATION_FEE, blk_1r, std::vector<extra_v>({ gwdo }), empty_attachment);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_reg);

  // fund gw address
  std::vector<tx_destination_entry> destinations;
  destinations.emplace_back(MK_TEST_COINS(20), m_gw_addr_view.pub);
  transaction tx_fund_gw{};
  r = construct_tx_to_key(m_hardforks, events, tx_fund_gw, blk_2, alice_acc, destinations);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_to_key failed");
  ADD_CUSTOM_EVENT(events, tx_fund_gw);
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_fund_gw);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_3r, blk_3, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ m_gw_addr_view.pub, MK_TEST_COINS(20) }));

  // blk_3r is the split point

  // chain B: spend 3 -> bob
  transaction tx_b1{};
  r = construct_gw_spend_tx(m_hardforks, events, blk_3r, alice_acc, m_gw_addr_view.pub, m_gw_addr_spend, MK_TEST_COINS(3), bob_acc.get_public_address(), tx_b1);
  CHECK_AND_ASSERT_MES(r, false, "construct_gw_spend_tx B1 failed");
  ADD_CUSTOM_EVENT(events, tx_b1);
  MAKE_NEXT_BLOCK_TX1(events, blk_b1, blk_3r, miner_acc, tx_b1);

  // gw balance: 20 - 3 = 17
  DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ m_gw_addr_view.pub, MK_TEST_COINS(17) }));

  // chain B: spend 5 -> bob
  transaction tx_b2{};
  r = construct_gw_spend_tx(m_hardforks, events, blk_b1, alice_acc, m_gw_addr_view.pub, m_gw_addr_spend, MK_TEST_COINS(5), bob_acc.get_public_address(), tx_b2);
  CHECK_AND_ASSERT_MES(r, false, "construct_gw_spend_tx B2 failed");
  ADD_CUSTOM_EVENT(events, tx_b2);
  MAKE_NEXT_BLOCK_TX1(events, blk_b2, blk_b1, miner_acc, tx_b2);

  // gw balance: 17 - 5 = 12
  DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ m_gw_addr_view.pub, MK_TEST_COINS(12) }));

  // chain C (alt, from blk_3r): spend 4 -> bob
  transaction tx_c1{};
  r = construct_gw_spend_tx(m_hardforks, events, blk_3r, alice_acc, m_gw_addr_view.pub, m_gw_addr_spend, MK_TEST_COINS(4), bob_acc.get_public_address(), tx_c1);
  CHECK_AND_ASSERT_MES(r, false, "construct_gw_spend_tx C1 failed");
  ADD_CUSTOM_EVENT(events, tx_c1);
  MAKE_NEXT_BLOCK_TX1(events, blk_c1, blk_3r, miner_acc, tx_c1);

  // chain C: spend 2 -> bob
  transaction tx_c2{};
  r = construct_gw_spend_tx(m_hardforks, events, blk_c1, alice_acc, m_gw_addr_view.pub, m_gw_addr_spend, MK_TEST_COINS(2), bob_acc.get_public_address(), tx_c2);
  CHECK_AND_ASSERT_MES(r, false, "construct_gw_spend_tx C2 failed");
  ADD_CUSTOM_EVENT(events, tx_c2);
  MAKE_NEXT_BLOCK_TX1(events, blk_c2, blk_c1, miner_acc, tx_c2);

  // add one more C block to make C heavier (3 vs B's 2)
  MAKE_NEXT_BLOCK(events, blk_c3, blk_c2, miner_acc);

  // C is main, gw balance: 20 - 4 - 2 = 14
  DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ m_gw_addr_view.pub, MK_TEST_COINS(14) }));

  // switch back to B (4 vs C's 3)
  MAKE_NEXT_BLOCK(events, blk_b3, blk_b2, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_b4, blk_b3, miner_acc);

  // B is main, gw balance: 12
  DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ m_gw_addr_view.pub, MK_TEST_COINS(12) }));

  // switch to C again (5 vs B's 4)
  MAKE_NEXT_BLOCK(events, blk_c4, blk_c3, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_c5, blk_c4, miner_acc);

  // C is main, gw balance: 14
  DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ m_gw_addr_view.pub, MK_TEST_COINS(14) }));

  DO_CALLBACK(events, "c1");

  return true;
}

bool gw_addr_altchain_spend_in_both_chains::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // chain C is main, bob received 4 + 2 = 6 from GW
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  bob_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", MK_TEST_COINS(6), UINT64_MAX, 0, 0, 0), false, "");
  return true;
}

//------------------------------------------------------------------------------
gw_addr_altchain_created_in_fork::gw_addr_altchain_created_in_fork()
{
  REGISTER_CALLBACK_METHOD(gw_addr_altchain_created_in_fork, c1);

  m_hardforks.clear();
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_04_ZARCANUM, 1);
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_06, 1);
}

bool gw_addr_altchain_created_in_fork::generate(std::vector<test_event_entry>& events) const
{
  // GW address created in chain B (after split), funded and spent in B
  // Switch to C (GW doesn't exist), then back to B - should be OK

  bool r = false;
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_06));

  // fund alice for fees
  transaction tx_fund{};
  CHECK_AND_ASSERT_SUCCESS(construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), MK_TEST_COINS(200), 10, TESTS_DEFAULT_FEE, tx_fund));
  ADD_CUSTOM_EVENT(events, tx_fund);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_fund);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // blk_1r is the split point

  // chain B: register gw address
  m_gw_addr_view  = keypair::generate();
  m_gw_addr_spend = keypair::generate();

  gateway_address_descriptor_operation gwdo{};
  gateway_address_descriptor_operation_register gwdo_reg{};
  gwdo_reg.view_pub_key = m_gw_addr_view.pub;
  gwdo_reg.descriptor.meta_info = "test_gw_addr_fork";
  gwdo_reg.descriptor.owner_key = m_gw_addr_spend.pub;
  gwdo.operation = gwdo_reg;
  MAKE_TX_EXTRA_ATTACH_FEE(events, tx_reg, miner_acc, miner_acc, 0, CURRENCY_GATEWAY_ADDRESS_REGISTRATION_FEE, blk_1r, std::vector<extra_v>({ gwdo }), empty_attachment);
  MAKE_NEXT_BLOCK_TX1(events, blk_b1, blk_1r, miner_acc, tx_reg);

  // fund gw in B
  std::vector<tx_destination_entry> destinations;
  destinations.emplace_back(MK_TEST_COINS(10), m_gw_addr_view.pub);
  transaction tx_fund_gw{};
  r = construct_tx_to_key(m_hardforks, events, tx_fund_gw, blk_b1, alice_acc, destinations);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_to_key failed");
  ADD_CUSTOM_EVENT(events, tx_fund_gw);
  MAKE_NEXT_BLOCK_TX1(events, blk_b2, blk_b1, miner_acc, tx_fund_gw);

  DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ m_gw_addr_view.pub, MK_TEST_COINS(10) }));

  // spend from gw in B: 6 -> bob
  transaction tx_spend{};
  r = construct_gw_spend_tx(m_hardforks, events, blk_b2, alice_acc, m_gw_addr_view.pub, m_gw_addr_spend, MK_TEST_COINS(6), bob_acc.get_public_address(), tx_spend);
  CHECK_AND_ASSERT_MES(r, false, "construct_gw_spend_tx failed");
  ADD_CUSTOM_EVENT(events, tx_spend);
  MAKE_NEXT_BLOCK_TX1(events, blk_b3, blk_b2, miner_acc, tx_spend);

  // gw balance: 10 - 6 = 4
  DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ m_gw_addr_view.pub, MK_TEST_COINS(4) }));

  MAKE_NEXT_BLOCK(events, blk_b4, blk_b3, miner_acc);

  // chain C (from blk_1r): build enough blocks to overtake B (B has 4, C needs 5+)
  MAKE_NEXT_BLOCK(events, blk_c1, blk_1r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_c2, blk_c1, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_c3, blk_c2, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_c4, blk_c3, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_c5, blk_c4, miner_acc);

  // C is now main, gw address doesn't exist in C
  DO_CALLBACK(events, "c1");

  // switch back to B (6 vs C=5)
  MAKE_NEXT_BLOCK(events, blk_b5, blk_b4, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_b6, blk_b5, miner_acc);

  // B is main again, gw balance should be 4
  DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ m_gw_addr_view.pub, MK_TEST_COINS(4) }));

  return true;
}

bool gw_addr_altchain_created_in_fork::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // verify gw address does not exist when chain C is main
  const blockchain_storage& bcs = c.get_blockchain_storage();
  std::shared_ptr<const currency::gateway_address_data> pd = bcs.get_gateway_address_info(m_gw_addr_view.pub);
  CHECK_AND_ASSERT_MES(!pd, false, "GW address should NOT exist in chain C, but it was found");
  return true;
}

//------------------------------------------------------------------------------
gw_addr_altchain_no_cross_chain_usage::gw_addr_altchain_no_cross_chain_usage()
{
  m_hardforks.clear();
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_04_ZARCANUM, 1);
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_06, 1);
}

bool gw_addr_altchain_no_cross_chain_usage::generate(std::vector<test_event_entry>& events) const
{
  // GW address registered and funded in B but not in C
  // switch to C - gw balance should be 0 (gw doesnt exist)
  // switch back to B - gw balance should be restored

  bool r = false;
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_06));

  // fund alice
  transaction tx_fund{};
  CHECK_AND_ASSERT_SUCCESS(construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), MK_TEST_COINS(200), 10, TESTS_DEFAULT_FEE, tx_fund));
  ADD_CUSTOM_EVENT(events, tx_fund);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_fund);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // chain B: register gw address and fund it
  m_gw_addr_view  = keypair::generate();
  m_gw_addr_spend = keypair::generate();

  gateway_address_descriptor_operation gwdo{};
  gateway_address_descriptor_operation_register gwdo_reg{};
  gwdo_reg.view_pub_key = m_gw_addr_view.pub;
  gwdo_reg.descriptor.meta_info = "test_gw_addr_nocross";
  gwdo_reg.descriptor.owner_key = m_gw_addr_spend.pub;
  gwdo.operation = gwdo_reg;
  MAKE_TX_EXTRA_ATTACH_FEE(events, tx_reg, miner_acc, miner_acc, 0, CURRENCY_GATEWAY_ADDRESS_REGISTRATION_FEE, blk_1r, std::vector<extra_v>({ gwdo }), empty_attachment);
  MAKE_NEXT_BLOCK_TX1(events, blk_b1, blk_1r, miner_acc, tx_reg);

  // fund gw with 20
  std::vector<tx_destination_entry> destinations;
  destinations.emplace_back(MK_TEST_COINS(20), m_gw_addr_view.pub);
  transaction tx_fund_gw{};
  r = construct_tx_to_key(m_hardforks, events, tx_fund_gw, blk_b1, alice_acc, destinations);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_to_key failed");
  ADD_CUSTOM_EVENT(events, tx_fund_gw);
  MAKE_NEXT_BLOCK_TX1(events, blk_b2, blk_b1, miner_acc, tx_fund_gw);
  MAKE_NEXT_BLOCK(events, blk_b3, blk_b2, miner_acc);

  // B is main (3 blocks), gw balance = 20
  DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ m_gw_addr_view.pub, MK_TEST_COINS(20) }));

  // chain C: no gw, just empty blocks - make C heavier (4 > B 3)
  MAKE_NEXT_BLOCK(events, blk_c1, blk_1r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_c2, blk_c1, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_c3, blk_c2, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_c4, blk_c3, miner_acc);

  // C is main, gw doesn't exist here
  { 
    gw_address_balance_check_param p {m_gw_addr_view.pub, 0 };
    p.address_should_not_be_found = true;
    DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(p));
  }

  // switch back to B (5 > C=4)
  MAKE_NEXT_BLOCK(events, blk_b4, blk_b3, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_b5, blk_b4, miner_acc);

  // B is main again, gw balance restored to 20
  DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ m_gw_addr_view.pub, MK_TEST_COINS(20) }));

  return true;
}


//------------------------------------------------------------------------------
gw_addr_altchain_owner_change::gw_addr_altchain_owner_change()
{
  REGISTER_CALLBACK_METHOD(gw_addr_altchain_owner_change, c1);

  m_hardforks.clear();
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_04_ZARCANUM, 1);
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_06, 1);
}

bool gw_addr_altchain_owner_change::generate(std::vector<test_event_entry>& events) const
{
  // gw address created and funded in A
  // chain B: spend with original owner
  // chain C: change owner, make C main, then spend with new owner
  // multi switch B<->C - each gateway spend is submitted to pool only when
  // the correct chain is main (pool validates against main chain state)

  bool r = false;
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_06));

  // fund alice
  transaction tx_fund{};
  CHECK_AND_ASSERT_SUCCESS(construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), MK_TEST_COINS(300), 10, TESTS_DEFAULT_FEE, tx_fund));
  ADD_CUSTOM_EVENT(events, tx_fund);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_fund);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // register gw address with original owner
  m_gw_addr_view      = keypair::generate();
  m_gw_addr_spend     = keypair::generate();
  m_gw_addr_new_spend = keypair::generate();

  gateway_address_descriptor_operation gwdo{};
  gateway_address_descriptor_operation_register gwdo_reg{};
  gwdo_reg.view_pub_key = m_gw_addr_view.pub;
  gwdo_reg.descriptor.meta_info = "test_gw_addr_owner_change";
  gwdo_reg.descriptor.owner_key = m_gw_addr_spend.pub;
  gwdo.operation = gwdo_reg;
  MAKE_TX_EXTRA_ATTACH_FEE(events, tx_reg, miner_acc, miner_acc, 0, CURRENCY_GATEWAY_ADDRESS_REGISTRATION_FEE, blk_1r, std::vector<extra_v>({ gwdo }), empty_attachment);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_reg);

  // fund gw with 30
  {
    std::vector<tx_destination_entry> destinations;
    destinations.emplace_back(MK_TEST_COINS(30), m_gw_addr_view.pub);
    transaction tx_fund_gw{};
    r = construct_tx_to_key(m_hardforks, events, tx_fund_gw, blk_2, alice_acc, destinations);
    CHECK_AND_ASSERT_MES(r, false, "construct_tx_to_key failed");
    ADD_CUSTOM_EVENT(events, tx_fund_gw);
    MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_fund_gw);
    REWIND_BLOCKS_N_WITH_TIME(events, blk_3r, blk_3, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

    DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ m_gw_addr_view.pub, MK_TEST_COINS(30) }));

    // blk_3r - split point

    // chain B: spend 7 with original owner
    // B extends from blk_3r, becomes main (1 block from split, only chain)
    transaction tx_b_spend{};
    r = construct_gw_spend_tx(m_hardforks, events, blk_3r, alice_acc, m_gw_addr_view.pub, m_gw_addr_spend, MK_TEST_COINS(7), bob_acc.get_public_address(), tx_b_spend);
    CHECK_AND_ASSERT_MES(r, false, "construct_gw_spend_tx B failed");
    ADD_CUSTOM_EVENT(events, tx_b_spend);
    MAKE_NEXT_BLOCK_TX1(events, blk_b1, blk_3r, miner_acc, tx_b_spend);
    MAKE_NEXT_BLOCK(events, blk_b2, blk_b1, miner_acc);
    // B is main (2 blocks from split), gw balance 30 - 7 = 23
    DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ m_gw_addr_view.pub, MK_TEST_COINS(23) }));

    // chain C: owner change, alt chain from blk_3r
    // tx_c_update is a regular tx from alice (no gw in) so pool validates fine
    gateway_address_descriptor_operation gwdo_upd{};
    gateway_address_descriptor_operation_update gao_upd{};
    gao_upd.address_id = m_gw_addr_view.pub;
    gao_upd.descriptor.meta_info = "test_gw_addr_owner_change";
    gao_upd.descriptor.owner_key = m_gw_addr_new_spend.pub;
    gwdo_upd.operation = gao_upd;

    size_t tx_hardfork_id{};
    uint64_t tx_version = get_tx_version_and_hardfork_id(get_block_height(blk_3r) + 1, m_hardforks, tx_hardfork_id);

    std::unordered_map<crypto::public_key, uint64_t> fee_amounts;
    fee_amounts[native_coin_asset_id] = TESTS_DEFAULT_FEE;
    std::vector<tx_source_entry> sources;
    r = fill_tx_sources(sources, events, blk_3r, alice_acc.get_keys(), fee_amounts, 0, std::vector<tx_source_entry>(), fts_default);
    CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources for owner change failed");

    std::unordered_map<crypto::public_key, uint64_t> found_amounts;
    get_sources_total_amount(sources, found_amounts);
    uint64_t change = found_amounts[native_coin_asset_id] - TESTS_DEFAULT_FEE;

    std::vector<tx_destination_entry> dests;
    if (change > 0)
      dests.emplace_back(change, alice_acc.get_public_address());

    // HF4+ for req CURRENCY_TX_MIN_ALLOWED_OUTS outs
    if (dests.size() < CURRENCY_TX_MIN_ALLOWED_OUTS && !dests.empty())
    {
      tx_destination_entry de = dests.back();
      dests.pop_back();
      size_t items_to_add = CURRENCY_TX_MIN_ALLOWED_OUTS - dests.size();
      currency::decompose_amount_randomly(de.amount, [&](uint64_t amount)
      {
        de.amount = amount;
        dests.push_back(de);
      }, items_to_add);
    }

    transaction tx_c_update{};
    crypto::secret_key tx_key{};
    r = construct_tx(alice_acc.get_keys(), sources, dests, std::vector<extra_v>({ gwdo_upd }), empty_attachment, tx_c_update, tx_version, tx_hardfork_id, tx_key, 0);
    CHECK_AND_ASSERT_MES(r, false, "construct_tx for owner change failed");

    // add ownership proof signed by current owner
    gateway_address_ownership_proof gaoop{};
    crypto::hash tx_hash = get_transaction_hash(tx_c_update);
    crypto::generic_schnorr_sig_s gsss{};
    crypto::generic_schnorr_sig& gss = static_cast<crypto::generic_schnorr_sig&>(gsss);
    CHECK_AND_ASSERT_TRUE(crypto::generate_schnorr_sig<crypto::gt_G>(tx_hash, crypto::point_t(m_gw_addr_spend.pub), crypto::scalar_t(m_gw_addr_spend.sec), gss));
    gaoop.sign = gsss;
    tx_c_update.proofs.push_back(gaoop);

    ADD_CUSTOM_EVENT(events, tx_c_update);
    MAKE_NEXT_BLOCK_TX1(events, blk_c1, blk_3r, miner_acc, tx_c_update);
    // C: 1 block from split (alt chain, B still main with 2)

    // make C main BEFORE submitting tx_c_spend to pool
    // (pool validates gw input signature against main chains owner key)
    MAKE_NEXT_BLOCK(events, blk_c2, blk_c1, miner_acc);
    MAKE_NEXT_BLOCK(events, blk_c3, blk_c2, miner_acc);
    // C: 3 blocks from split > B: 2 blocks => C is now main, owner change is in effect

    // now tx_c_spend (signed by new owner) will pass pool validation
    transaction tx_c_spend{};
    r = construct_gw_spend_tx(m_hardforks, events, blk_c3, alice_acc, m_gw_addr_view.pub, m_gw_addr_new_spend, MK_TEST_COINS(5), bob_acc.get_public_address(), tx_c_spend);
    CHECK_AND_ASSERT_MES(r, false, "construct_gw_spend_tx C failed");
    ADD_CUSTOM_EVENT(events, tx_c_spend);
    MAKE_NEXT_BLOCK_TX1(events, blk_c4, blk_c3, miner_acc, tx_c_spend);
    // C: 4 blocks from split, gw balance: 30 - 5 = 25
    DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ m_gw_addr_view.pub, MK_TEST_COINS(25) }));

    //switch back to B
    MAKE_NEXT_BLOCK(events, blk_b3, blk_b2, miner_acc);
    MAKE_NEXT_BLOCK(events, blk_b4, blk_b3, miner_acc);
    MAKE_NEXT_BLOCK(events, blk_b5, blk_b4, miner_acc);
    // B: 5 blocks from split > C: 4 blocks => B is main, old owner is active
    DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ m_gw_addr_view.pub, MK_TEST_COINS(23) }));

    // B: spend 3 with old owner (B is main, old owner valid for pool)
    transaction tx_b_spend2{};
    r = construct_gw_spend_tx(m_hardforks, events, blk_b5, alice_acc, m_gw_addr_view.pub, m_gw_addr_spend, MK_TEST_COINS(3), bob_acc.get_public_address(), tx_b_spend2);
    CHECK_AND_ASSERT_MES(r, false, "construct_gw_spend_tx B2 failed");
    ADD_CUSTOM_EVENT(events, tx_b_spend2);
    MAKE_NEXT_BLOCK_TX1(events, blk_b6, blk_b5, miner_acc, tx_b_spend2);
    // B: 6 blocks from split, gw balance: 23 - 3 = 20
    DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ m_gw_addr_view.pub, MK_TEST_COINS(20) }));

    //switch back to C
    MAKE_NEXT_BLOCK(events, blk_c5, blk_c4, miner_acc);
    MAKE_NEXT_BLOCK(events, blk_c6, blk_c5, miner_acc);
    MAKE_NEXT_BLOCK(events, blk_c7, blk_c6, miner_acc);
    // C: 7 blocks from split > B: 6 blocks => C is main, new owner is active
    DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ m_gw_addr_view.pub, MK_TEST_COINS(25) }));

    // C: spend 8 with new owner (C is main, new owner valid for pool)
    transaction tx_c_spend2{};
    r = construct_gw_spend_tx(m_hardforks, events, blk_c7, alice_acc, m_gw_addr_view.pub, m_gw_addr_new_spend, MK_TEST_COINS(8), bob_acc.get_public_address(), tx_c_spend2);
    CHECK_AND_ASSERT_MES(r, false, "construct_gw_spend_tx C2 failed");
    ADD_CUSTOM_EVENT(events, tx_c_spend2);
    MAKE_NEXT_BLOCK_TX1(events, blk_c8, blk_c7, miner_acc, tx_c_spend2);
    // C: 8 blocks from split, gw balance: 25 - 8 = 17
    DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ m_gw_addr_view.pub, MK_TEST_COINS(17) }));
  }

  DO_CALLBACK(events, "c1");

  return true;
}

bool gw_addr_altchain_owner_change::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  return true;
}
