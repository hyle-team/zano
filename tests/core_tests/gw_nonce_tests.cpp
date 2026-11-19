// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "gw_nonce_tests.h"
#include "chaingen_helpers.h"
#include "currency_core/crypto_config.h"

using namespace currency;

//
// gateway address nonce tests (consensus rules only, the tx pool is involved only for txs that are valid for the current main chain)
//
// Rules under test:
//   1) the first tx with inputs of a gw address carries CURRENCY_GATEWAY_ADDRESS_NONCE_STARTER, each next one carries the previous nonce + 1;
//   2) a block is valid only if every gw tx in it carries exactly the nonce expected at that point (no gaps, no repeats, no reordering);
//   3) a nonce is taken once per tx: all gw inputs of a tx carry the same nonce;
//   4) each gw address has its own counter;
//   5) the counter follows the chain: it is rolled back when blocks are detached.
//
// Txs that must be rejected by a block are delivered inside the block (set_txs_kept_by_block), so these tests don't depend on tx pool rules.
//

namespace
{
  struct gw_tx_input
  {
    uint64_t amount;
    uint64_t nonce;
  };

  // gw-only tx: native gw inputs of one gw address and one gw output back to the same address; the difference (TESTS_DEFAULT_FEE) is the fee,
  // so each confirmed tx reduces the native balance of the gw address by TESTS_DEFAULT_FEE
  bool construct_gw_self_transfer_tx(const hard_forks_descriptor& hardforks, uint64_t block_height, const keypair& gw_view, const keypair& gw_spend,
    const std::vector<gw_tx_input>& inputs, transaction& tx)
  {
    std::vector<tx_source_entry> sources;
    uint64_t inputs_sum = 0;
    for (const gw_tx_input& in : inputs)
    {
      tx_source_entry& tse = sources.emplace_back();
      tse.gateway_origin = gw_view.pub;
      tse.asset_id = native_coin_asset_id;
      tse.amount = in.amount;
      tse.nonce = in.nonce;
      inputs_sum += in.amount;
    }
    CHECK_AND_ASSERT_MES(inputs_sum > TESTS_DEFAULT_FEE, false, "gw inputs don't cover the fee");

    std::vector<tx_destination_entry> destinations;
    destinations.emplace_back(inputs_sum - TESTS_DEFAULT_FEE, gw_view.pub);

    size_t tx_hardfork_id{};
    uint64_t tx_version = get_tx_version_and_hardfork_id(block_height, hardforks, tx_hardfork_id);
    crypto::secret_key tx_key{};
    bool r = construct_tx(account_keys{}, sources, destinations, empty_extra, empty_attachment, tx, tx_version, tx_hardfork_id, tx_key, 0); // gw-only tx: no sender keys needed
    CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

    r = sign_gw_inputs_in_tx([&](const crypto::public_key&, crypto::secret_key& gw_spend_secret) -> bool {
      gw_spend_secret = gw_spend.sec;
      return true;
    }, tx);
    CHECK_AND_ASSERT_MES(r, false, "sign_gw_inputs_in_tx failed");
    return true;
  }

  // registers a gw address (owner key: gw_spend) and gives it native_amount, both by the miner, one block each;
  // top_block is the block to build on, on success it is replaced with the new chain head
  bool register_and_fund_gw_address(test_generator& generator, std::vector<test_event_entry>& events, const account_base& miner_acc,
    const keypair& gw_view, const keypair& gw_spend, uint64_t native_amount, block& top_block)
  {
    gateway_address_descriptor_operation gwdo{};
    gateway_address_descriptor_operation_register gwdo_reg{};
    gwdo_reg.view_pub_key = gw_view.pub;
    gwdo_reg.descriptor.owner_key = gw_spend.pub;
    gwdo.operation = gwdo_reg;
    MAKE_TX_EXTRA_ATTACH_FEE(events, tx_reg, miner_acc, miner_acc, 0, CURRENCY_GATEWAY_ADDRESS_REGISTRATION_FEE, top_block, std::vector<extra_v>({ gwdo }), empty_attachment);
    MAKE_NEXT_BLOCK_TX1(events, blk_reg, top_block, miner_acc, tx_reg);

    std::vector<tx_destination_entry> destinations;
    destinations.emplace_back(native_amount, gw_view.pub);
    transaction tx_fund{};
    bool r = construct_tx_to_key(generator.get_hardforks(), events, tx_fund, blk_reg, miner_acc, destinations);
    CHECK_AND_ASSERT_MES(r, false, "construct_tx_to_key failed");
    ADD_CUSTOM_EVENT(events, tx_fund);
    MAKE_NEXT_BLOCK_TX1(events, blk_fund, blk_reg, miner_acc, tx_fund);

    top_block = blk_fund;
    return true;
  }
} // namespace

//------------------------------------------------------------------------------

gw_nonce_test_base::gw_nonce_test_base()
{
  REGISTER_CALLBACK_METHOD(gw_nonce_test_base, check_gw_nonce);
}

bool gw_nonce_test_base::check_gw_nonce(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  gw_nonce_check_param params{};
  bool r = t_unserializable_object_from_blob(params, boost::get<callback_entry>(events[ev_index]).callback_params);
  CHECK_AND_ASSERT_MES(r, false, "check_gw_nonce: couldn't deserialize params");

  uint64_t next_nonce = c.get_blockchain_storage().get_gw_next_nonce(params.gw_addr);
  CHECK_AND_ASSERT_MES(next_nonce == params.next_nonce, false, "check_gw_nonce: gw address " << params.gw_addr << " expects nonce " << next_nonce << ", while the test expects: " << params.next_nonce);
  return true;
}

//------------------------------------------------------------------------------

bool gw_nonce_strict_order::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: a gw address accepts its txs strictly in the order of their nonces.
  // case | block content (N is the expected nonce)        | block
  // -----+------------------------------------------------+--------
  // S1   | nonce 0 (below the starter)                    | reject
  // S2   | nonce N + 1 (N is skipped)                     | reject
  // S3   | nonce N                                        | accept
  // S4   | the nonce of S3 again (another tx)             | reject
  // S5   | two txs: N + 1, then N                         | reject
  // S6   | two txs with the same nonce N                  | reject (the increment made by the first tx is rolled back too)
  // S7   | two txs: N, then N + 1                         | accept
  // S8   | nonce N, signed when the tx carried N + 5      | reject (pool and block): the nonce is covered by the owner's signature

  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  const keypair gw_view  = keypair::generate();
  const keypair gw_spend = keypair::generate();
  const uint64_t gw_balance = MK_TEST_COINS(50);
  block blk_1 = blk_0r;
  CHECK_AND_ASSERT_TRUE(register_and_fund_gw_address(generator, events, miner_acc, gw_view, gw_spend, gw_balance, blk_1));

  const uint64_t tx_height = get_block_height(blk_1) + 1; // the same hardfork is active for all the blocks of this test, so one height fits all the txs
  uint64_t salt = 0; // makes every tx unique
  auto make_gw_tx = [&](uint64_t nonce, transaction& tx) -> bool {
    return construct_gw_self_transfer_tx(m_hardforks, tx_height, gw_view, gw_spend, { gw_tx_input{ MK_TEST_COINS(2) + (++salt), nonce } }, tx);
  };

  auto check_gw_state = [&](uint64_t expected_next_nonce, uint64_t expected_balance) {
    DO_CALLBACK_PARAMS_STR(events, "check_gw_nonce", t_serializable_object_to_blob(gw_nonce_check_param{ gw_view.pub, expected_next_nonce }));
    DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ gw_view.pub, expected_balance }));
  };

  // the txs are delivered inside a block on top of prev_block, the block must be rejected and prev_block must remain the top one
  auto play_bad_block = [&](const std::list<transaction>& txs, const block& prev_block) {
    events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
    for (const transaction& tx : txs)
    {
      ADD_CUSTOM_EVENT(events, tx);
    }
    DO_CALLBACK(events, "mark_invalid_block");
    MAKE_NEXT_BLOCK_TX_LIST(events, blk_bad, prev_block, miner_acc, txs);
    events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));
    DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(prev_block));
    DO_CALLBACK(events, "clear_tx_pool"); // txs of the rejected block that were processed before the failing one are moved to the pool
  };

  const uint64_t n = CURRENCY_GATEWAY_ADDRESS_NONCE_STARTER;
  check_gw_state(n, gw_balance);

  // S1: nonce 0
  transaction tx_s1{};
  CHECK_AND_ASSERT_TRUE(make_gw_tx(0, tx_s1));
  play_bad_block({ tx_s1 }, blk_1);
  check_gw_state(n, gw_balance);

  // S2: the expected nonce is skipped
  transaction tx_s2{};
  CHECK_AND_ASSERT_TRUE(make_gw_tx(n + 1, tx_s2));
  play_bad_block({ tx_s2 }, blk_1);
  check_gw_state(n, gw_balance);

  // S3: the expected nonce
  transaction tx_s3{};
  CHECK_AND_ASSERT_TRUE(make_gw_tx(n, tx_s3));
  ADD_CUSTOM_EVENT(events, tx_s3);
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, tx_s3);
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0));
  check_gw_state(n + 1, gw_balance - TESTS_DEFAULT_FEE);

  // S4: a used nonce
  transaction tx_s4{};
  CHECK_AND_ASSERT_TRUE(make_gw_tx(n, tx_s4));
  play_bad_block({ tx_s4 }, blk_2);
  check_gw_state(n + 1, gw_balance - TESTS_DEFAULT_FEE);

  // S5: two correct nonces in the wrong order
  transaction tx_a{}, tx_b{};
  CHECK_AND_ASSERT_TRUE(make_gw_tx(n + 1, tx_a));
  CHECK_AND_ASSERT_TRUE(make_gw_tx(n + 2, tx_b));
  play_bad_block({ tx_b, tx_a }, blk_2);
  check_gw_state(n + 1, gw_balance - TESTS_DEFAULT_FEE);

  // S6: the same nonce twice in a block
  transaction tx_a_dup{};
  CHECK_AND_ASSERT_TRUE(make_gw_tx(n + 1, tx_a_dup));
  play_bad_block({ tx_a, tx_a_dup }, blk_2);
  check_gw_state(n + 1, gw_balance - TESTS_DEFAULT_FEE);

  // S7: the same two txs as in S5 in the right order
  ADD_CUSTOM_EVENT(events, tx_a);
  ADD_CUSTOM_EVENT(events, tx_b);
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(2));
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3, blk_2, miner_acc, std::list<transaction>({ tx_a, tx_b }));
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0));
  check_gw_state(n + 3, gw_balance - 3 * TESTS_DEFAULT_FEE);

  // S8: the nonce is changed to the expected one after the tx was signed
  transaction tx_s8{};
  CHECK_AND_ASSERT_TRUE(make_gw_tx(n + 3 + 5, tx_s8));
  CHECK_AND_ASSERT_MES(tx_s8.vin.size() == 1 && tx_s8.vin[0].type() == typeid(txin_gateway), false, "unexpected inputs in tx_s8");
  boost::get<txin_gateway>(tx_s8.vin[0]).nonce = n + 3;
  DO_CALLBACK(events, "mark_invalid_tx");
  ADD_CUSTOM_EVENT(events, tx_s8);
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0));
  play_bad_block({ tx_s8 }, blk_3);
  check_gw_state(n + 3, gw_balance - 3 * TESTS_DEFAULT_FEE);

  return true;
}

//------------------------------------------------------------------------------

bool gw_nonce_per_tx_and_per_address::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: a nonce is taken once per tx (not per gw input), and each gw address counts its own txs.
  // case | block content (N1, N2 are the expected nonces of gw addresses 1 and 2)   | pool   | block
  // -----+--------------------------------------------------------------------------+--------+--------
  // P1   | address 1: two inputs with nonces N1 and N1 + 1                          | reject | reject
  // P2   | address 1: two inputs, both with nonce N1                                | accept | accept, N1 grows by 1 (not by 2)
  // P3   | two txs: address 1 with N1, address 2 with N2 (N2 is still the starter)  | accept | accept
  // P4   | address 2 with the nonce expected by address 1                           |        | reject
  // P5   | address 2 with N2                                                        | accept | accept

  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  const keypair gw_1_view  = keypair::generate();
  const keypair gw_1_spend = keypair::generate();
  const keypair gw_2_view  = keypair::generate();
  const keypair gw_2_spend = keypair::generate();
  const uint64_t gw_balance = MK_TEST_COINS(50); // of each address
  block blk_1 = blk_0r;
  CHECK_AND_ASSERT_TRUE(register_and_fund_gw_address(generator, events, miner_acc, gw_1_view, gw_1_spend, gw_balance, blk_1));
  CHECK_AND_ASSERT_TRUE(register_and_fund_gw_address(generator, events, miner_acc, gw_2_view, gw_2_spend, gw_balance, blk_1));

  const uint64_t tx_height = get_block_height(blk_1) + 1; // the same hardfork is active for all the blocks of this test, so one height fits all the txs
  uint64_t salt = 0; // makes every tx unique
  auto make_gw_1_tx = [&](uint64_t nonce, transaction& tx) -> bool {
    return construct_gw_self_transfer_tx(m_hardforks, tx_height, gw_1_view, gw_1_spend, { gw_tx_input{ MK_TEST_COINS(2) + (++salt), nonce } }, tx);
  };
  auto make_gw_1_tx_two_inputs = [&](uint64_t nonce_a, uint64_t nonce_b, transaction& tx) -> bool {
    return construct_gw_self_transfer_tx(m_hardforks, tx_height, gw_1_view, gw_1_spend, { gw_tx_input{ MK_TEST_COINS(2) + (++salt), nonce_a }, gw_tx_input{ MK_TEST_COINS(3) + (++salt), nonce_b } }, tx);
  };
  auto make_gw_2_tx = [&](uint64_t nonce, transaction& tx) -> bool {
    return construct_gw_self_transfer_tx(m_hardforks, tx_height, gw_2_view, gw_2_spend, { gw_tx_input{ MK_TEST_COINS(2) + (++salt), nonce } }, tx);
  };

  auto check_gw_states = [&](uint64_t expected_next_nonce_1, uint64_t expected_balance_1, uint64_t expected_next_nonce_2, uint64_t expected_balance_2) {
    DO_CALLBACK_PARAMS_STR(events, "check_gw_nonce", t_serializable_object_to_blob(gw_nonce_check_param{ gw_1_view.pub, expected_next_nonce_1 }));
    DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ gw_1_view.pub, expected_balance_1 }));
    DO_CALLBACK_PARAMS_STR(events, "check_gw_nonce", t_serializable_object_to_blob(gw_nonce_check_param{ gw_2_view.pub, expected_next_nonce_2 }));
    DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ gw_2_view.pub, expected_balance_2 }));
  };

  // the txs are delivered inside a block on top of prev_block, the block must be rejected and prev_block must remain the top one
  auto play_bad_block = [&](const std::list<transaction>& txs, const block& prev_block) {
    events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
    for (const transaction& tx : txs)
    {
      ADD_CUSTOM_EVENT(events, tx);
    }
    DO_CALLBACK(events, "mark_invalid_block");
    MAKE_NEXT_BLOCK_TX_LIST(events, blk_bad, prev_block, miner_acc, txs);
    events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));
    DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(prev_block));
    DO_CALLBACK(events, "clear_tx_pool"); // txs of the rejected block that were processed before the failing one are moved to the pool
  };

  const uint64_t n = CURRENCY_GATEWAY_ADDRESS_NONCE_STARTER;
  check_gw_states(n, gw_balance, n, gw_balance);

  // P1: gw inputs of one tx carry different nonces
  transaction tx_p1{};
  CHECK_AND_ASSERT_TRUE(make_gw_1_tx_two_inputs(n, n + 1, tx_p1));
  DO_CALLBACK(events, "mark_invalid_tx");
  ADD_CUSTOM_EVENT(events, tx_p1);
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0));
  play_bad_block({ tx_p1 }, blk_1);
  check_gw_states(n, gw_balance, n, gw_balance);

  // P2: two gw inputs with the same nonce take one nonce
  transaction tx_p2{};
  CHECK_AND_ASSERT_TRUE(make_gw_1_tx_two_inputs(n, n, tx_p2));
  ADD_CUSTOM_EVENT(events, tx_p2);
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, tx_p2);
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0));
  check_gw_states(n + 1, gw_balance - TESTS_DEFAULT_FEE, n, gw_balance);

  // P3: each gw address has its own counter
  transaction tx_p3_1{}, tx_p3_2{};
  CHECK_AND_ASSERT_TRUE(make_gw_1_tx(n + 1, tx_p3_1));
  CHECK_AND_ASSERT_TRUE(make_gw_2_tx(n, tx_p3_2));
  ADD_CUSTOM_EVENT(events, tx_p3_1);
  ADD_CUSTOM_EVENT(events, tx_p3_2);
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(2));
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3, blk_2, miner_acc, std::list<transaction>({ tx_p3_1, tx_p3_2 }));
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0));
  check_gw_states(n + 2, gw_balance - 2 * TESTS_DEFAULT_FEE, n + 1, gw_balance - TESTS_DEFAULT_FEE);

  // P4: address 2 can't use the nonce expected by address 1
  transaction tx_p4{};
  CHECK_AND_ASSERT_TRUE(make_gw_2_tx(n + 2, tx_p4));
  play_bad_block({ tx_p4 }, blk_3);
  check_gw_states(n + 2, gw_balance - 2 * TESTS_DEFAULT_FEE, n + 1, gw_balance - TESTS_DEFAULT_FEE);

  // P5: address 2 goes on with its own nonce
  transaction tx_p5{};
  CHECK_AND_ASSERT_TRUE(make_gw_2_tx(n + 1, tx_p5));
  ADD_CUSTOM_EVENT(events, tx_p5);
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));
  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_3, miner_acc, tx_p5);
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0));
  check_gw_states(n + 2, gw_balance - 2 * TESTS_DEFAULT_FEE, n + 2, gw_balance - 2 * TESTS_DEFAULT_FEE);

  return true;
}

//------------------------------------------------------------------------------

bool gw_nonce_chain_switch::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: the nonce counter of a gw address belongs to the chain state, it is rolled back and re-applied by chain switches.
  //
  //  blk_1 (gw address registered and funded, the expected nonce is the starter N)
  //    |- blk_a1 (tx_a1: N) - blk_a2 (tx_a2: N + 1) - blk_a3 - blk_a4 - blk_a5      chain A
  //    |- blk_b1 (tx_b1: N) - blk_b2 - blk_b3 - blk_b4 (tx_a2: N + 1)               chain B
  //    \- blk_c1 - ... - blk_c6 - blk_c7 (tx_c1: N)                                 chain C
  //
  // step | action                                         | main chain | expected nonce
  // -----+------------------------------------------------+------------+----------------
  // C1   | blk_a1, blk_a2                                 | A          | N + 2
  // C2   | blk_b1 .. blk_b3                               | B          | N + 1
  // C3   | tx_a1 (N, used by tx_b1) on top of blk_b3      | B          | N + 1 (block rejected)
  // C4   | tx_a2 (N + 1, made for chain A) in blk_b4      | B          | N + 2 (a nonce doesn't bind a tx to a chain)
  // C5   | blk_a3 .. blk_a5                               | A          | N + 2
  // C6   | blk_c1 .. blk_c6 (no gw txs at all)            | C          | N (the first spend is rolled back too)
  // C7   | tx_c1 (N) in blk_c7                            | C          | N + 1
  //
  // Txs of chains B and C are delivered inside their blocks: the tx pool follows the main chain, and these txs conflict by the nonce with the txs it holds.

  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  const keypair gw_view  = keypair::generate();
  const keypair gw_spend = keypair::generate();
  const uint64_t gw_balance = MK_TEST_COINS(50);
  block blk_1 = blk_0r;
  CHECK_AND_ASSERT_TRUE(register_and_fund_gw_address(generator, events, miner_acc, gw_view, gw_spend, gw_balance, blk_1));

  const uint64_t tx_height = get_block_height(blk_1) + 1; // the same hardfork is active for all the blocks of this test, so one height fits all the txs
  uint64_t salt = 0; // makes every tx unique
  auto make_gw_tx = [&](uint64_t nonce, transaction& tx) -> bool {
    return construct_gw_self_transfer_tx(m_hardforks, tx_height, gw_view, gw_spend, { gw_tx_input{ MK_TEST_COINS(2) + (++salt), nonce } }, tx);
  };

  auto check_gw_state = [&](uint64_t expected_next_nonce, uint64_t expected_balance) {
    DO_CALLBACK_PARAMS_STR(events, "check_gw_nonce", t_serializable_object_to_blob(gw_nonce_check_param{ gw_view.pub, expected_next_nonce }));
    DO_CALLBACK_PARAMS_STR(events, "check_gw_balance", t_serializable_object_to_blob(gw_address_balance_check_param{ gw_view.pub, expected_balance }));
  };

  const uint64_t n = CURRENCY_GATEWAY_ADDRESS_NONCE_STARTER;
  check_gw_state(n, gw_balance);

  // C1: chain A, two gw txs
  transaction tx_a1{}, tx_a2{};
  CHECK_AND_ASSERT_TRUE(make_gw_tx(n, tx_a1));
  CHECK_AND_ASSERT_TRUE(make_gw_tx(n + 1, tx_a2));
  ADD_CUSTOM_EVENT(events, tx_a1);
  MAKE_NEXT_BLOCK_TX1(events, blk_a1, blk_1, miner_acc, tx_a1);
  ADD_CUSTOM_EVENT(events, tx_a2);
  MAKE_NEXT_BLOCK_TX1(events, blk_a2, blk_a1, miner_acc, tx_a2);
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_a2));
  check_gw_state(n + 2, gw_balance - 2 * TESTS_DEFAULT_FEE);

  // C2: chain B with another first spend, it becomes the main chain
  transaction tx_b1{};
  CHECK_AND_ASSERT_TRUE(make_gw_tx(n, tx_b1));
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
  ADD_CUSTOM_EVENT(events, tx_b1);
  MAKE_NEXT_BLOCK_TX1(events, blk_b1, blk_1, miner_acc, tx_b1);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));
  MAKE_NEXT_BLOCK(events, blk_b2, blk_b1, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_b3, blk_b2, miner_acc);
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_b3));
  check_gw_state(n + 1, gw_balance - TESTS_DEFAULT_FEE);

  // C3: tx_a1 carries the nonce that tx_b1 has used in chain B
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
  ADD_CUSTOM_EVENT(events, tx_a1);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_b4_bad, blk_b3, miner_acc, tx_a1);
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_b3));
  check_gw_state(n + 1, gw_balance - TESTS_DEFAULT_FEE);

  // C4: tx_a2 carries the nonce that chain B expects next, so it is valid in chain B as well
  ADD_CUSTOM_EVENT(events, tx_a2);
  MAKE_NEXT_BLOCK_TX1(events, blk_b4, blk_b3, miner_acc, tx_a2);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_b4));
  check_gw_state(n + 2, gw_balance - 2 * TESTS_DEFAULT_FEE);

  // C5: chain A becomes the main chain again (5 blocks against 4)
  MAKE_NEXT_BLOCK(events, blk_a3, blk_a2, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_a4, blk_a3, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_a5, blk_a4, miner_acc);
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_a5));
  check_gw_state(n + 2, gw_balance - 2 * TESTS_DEFAULT_FEE);

  // C6: chain C without gw txs becomes the main chain (6 blocks against 5), every spend of the gw address is rolled back
  MAKE_NEXT_BLOCK(events, blk_c1, blk_1, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_c2, blk_c1, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_c3, blk_c2, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_c4, blk_c3, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_c5, blk_c4, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_c6, blk_c5, miner_acc);
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_c6));
  check_gw_state(n, gw_balance);

  // C7: the gw address starts from the first nonce again
  transaction tx_c1{};
  CHECK_AND_ASSERT_TRUE(make_gw_tx(n, tx_c1));
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
  ADD_CUSTOM_EVENT(events, tx_c1);
  MAKE_NEXT_BLOCK_TX1(events, blk_c7, blk_c6, miner_acc, tx_c1);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_c7));
  check_gw_state(n + 1, gw_balance - TESTS_DEFAULT_FEE);

  return true;
}
