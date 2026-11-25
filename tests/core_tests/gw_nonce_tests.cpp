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

//------------------------------------------------------------------------------
//
// playtime tests: tx pool rules, block template and pool cleanup for txs with gw inputs
//
// Rules under test (N is the nonce the blockchain expects next for a gw address):
//   1) the pool accepts a tx if its nonce is within [N, N + gw_nonce_max_gap), gaps are allowed;
//   2) a pooled tx is replaced by a tx with the same nonce that pays at least the old fee + gw_nonce_fee_window;
//   3) the block template takes gw txs first, each gw address strictly in the order of nonces starting from N, and takes nothing after a gap;
//   4) gw txs take no more than 80% of the penalty-free block size, and a block template is never bigger than the penalty-free size;
//   5) a tx that fails in a block because of its nonce is not blacklisted, and a blacklisted gw tx doesn't keep its nonce in the pool;
//   6) a tx with a nonce that is already used in the blockchain is removed from the pool.
//
// generate() only registers and funds the gw addresses, all the scenarios are played inside c1().
//

namespace
{
  const uint64_t c_playtime_gw_balance = MK_TEST_COINS(1000); // native balance of each gw address

  // gw-only tx: one native gw input and one gw output back to the same address, the difference is the fee;
  // padding_size > 0 adds extra_padding of that size to make the tx bigger
  bool construct_gw_self_transfer_tx_with_fee(const hard_forks_descriptor& hardforks, uint64_t block_height, const keypair& gw_view, const keypair& gw_spend,
    uint64_t nonce, uint64_t fee, size_t padding_size, transaction& tx)
  {
    static uint64_t salt = 0; // makes every tx unique

    std::vector<tx_source_entry> sources;
    tx_source_entry& tse = sources.emplace_back();
    tse.gateway_origin = gw_view.pub;
    tse.asset_id = native_coin_asset_id;
    tse.amount = fee + MK_TEST_COINS(1) + (++salt);
    tse.nonce = nonce;

    std::vector<tx_destination_entry> destinations;
    destinations.emplace_back(tse.amount - fee, gw_view.pub);

    std::vector<extra_v> extra;
    if (padding_size != 0)
    {
      extra_padding padding{};
      padding.buff.resize(padding_size);
      extra.push_back(padding);
    }

    size_t tx_hardfork_id{};
    uint64_t tx_version = get_tx_version_and_hardfork_id(block_height, hardforks, tx_hardfork_id);
    crypto::secret_key tx_key{};
    bool r = construct_tx(account_keys{}, sources, destinations, extra, empty_attachment, tx, tx_version, tx_hardfork_id, tx_key, 0); // gw-only tx: no sender keys needed
    CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

    r = sign_gw_inputs_in_tx([&](const crypto::public_key&, crypto::secret_key& gw_spend_secret) -> bool {
      gw_spend_secret = gw_spend.sec;
      return true;
    }, tx);
    CHECK_AND_ASSERT_MES(r, false, "sign_gw_inputs_in_tx failed");
    return true;
  }

  // compares tx ids with the expected txs, the order matters
  bool check_tx_ids(const std::vector<crypto::hash>& actual_ids, const std::vector<const transaction*>& expected_txs, const std::string& descr)
  {
    std::vector<crypto::hash> expected_ids;
    for (const transaction* ptx : expected_txs)
      expected_ids.push_back(get_transaction_hash(*ptx));

    if (actual_ids == expected_ids)
      return true;

    std::stringstream ss;
    ss << descr << ": unexpected txs" << ENDL << "  expected (" << expected_ids.size() << "):" << ENDL;
    for (const crypto::hash& h : expected_ids)
      ss << "    " << h << ENDL;
    ss << "  actual (" << actual_ids.size() << "):" << ENDL;
    for (const crypto::hash& h : actual_ids)
      ss << "    " << h << ENDL;
    LOG_ERROR(ss.str());
    return false;
  }
} // namespace

//------------------------------------------------------------------------------

gw_nonce_playtime_base::gw_nonce_playtime_base(size_t gw_addresses_count)
  : m_gw_addresses_count(gw_addresses_count)
{
  REGISTER_CALLBACK_METHOD(gw_nonce_playtime_base, c1);
}

bool gw_nonce_playtime_base::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  block top_block = blk_0r;
  m_gw_view_keys.clear();
  m_gw_spend_keys.clear();
  for (size_t i = 0; i < m_gw_addresses_count; ++i)
  {
    m_gw_view_keys.push_back(keypair::generate());
    m_gw_spend_keys.push_back(keypair::generate());
    CHECK_AND_ASSERT_TRUE(register_and_fund_gw_address(generator, events, miner_acc, m_gw_view_keys.back(), m_gw_spend_keys.back(), c_playtime_gw_balance, top_block));
  }

  REWIND_BLOCKS_N(events, blk_r, top_block, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW); // unlocks miner's coins for regular txs made in c1

  DO_CALLBACK(events, "c1");
  return true;
}

bool gw_nonce_playtime_base::make_gw_tx(currency::core& c, size_t gw_index, uint64_t nonce, uint64_t fee, currency::transaction& tx, size_t padding_size /* = 0 */) const
{
  CHECK_AND_ASSERT_MES(gw_index < m_gw_view_keys.size(), false, "invalid gw_index: " << gw_index);
  const uint64_t next_block_height = c.get_blockchain_storage().get_current_blockchain_size();
  return construct_gw_self_transfer_tx_with_fee(m_hardforks, next_block_height, m_gw_view_keys[gw_index], m_gw_spend_keys[gw_index], nonce, fee, padding_size, tx);
}

bool gw_nonce_playtime_base::make_regular_tx(currency::core& c, tools::wallet2& miner_wlt, uint64_t fee, currency::transaction& tx, size_t padding_size /* = 0 */) const
{
  std::vector<extra_v> extra;
  if (padding_size != 0)
  {
    extra_padding padding{};
    padding.buff.resize(padding_size);
    extra.push_back(padding);
  }

  std::vector<tx_destination_entry> destinations{ tx_destination_entry(MK_TEST_COINS(1), m_accounts[BOB_ACC_IDX].get_public_address()) };
  try
  {
    miner_wlt.refresh();
    miner_wlt.transfer(destinations, 0, 0, fee, extra, empty_attachment, tx);
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("wallet transfer failed: " << e.what());
    return false;
  }

  CHECK_AND_ASSERT_MES(is_tx_in_pool(c, tx), false, "regular tx " << get_transaction_hash(tx) << " is not in the pool");
  return true;
}

bool gw_nonce_playtime_base::push_tx(currency::core& c, const currency::transaction& tx, tx_push_result expected, const std::string& descr) const
{
  const crypto::hash tx_id = get_transaction_hash(tx);
  tx_verification_context tvc{};
  c.handle_incoming_tx(t_serializable_object_to_blob(tx), tvc, false);

  switch (expected)
  {
  case tx_accepted:
    CHECK_AND_ASSERT_MES(tvc.m_added_to_pool && !tvc.m_verification_failed, false, descr << ": tx " << tx_id << " must be accepted by the pool, but it is not");
    CHECK_AND_ASSERT_MES(is_tx_in_pool(c, tx), false, descr << ": tx " << tx_id << " is accepted, but it is not in the pool");
    break;
  case tx_rejected:
    // a hard failure: the peer that sent such a tx is dropped
    CHECK_AND_ASSERT_MES(tvc.m_verification_failed && !tvc.m_added_to_pool, false, descr << ": tx " << tx_id << " must be rejected by the pool, but it is not");
    CHECK_AND_ASSERT_MES(!is_tx_in_pool(c, tx), false, descr << ": tx " << tx_id << " is rejected, but it is in the pool");
    break;
  case tx_already_exists:
    CHECK_AND_ASSERT_MES(tvc.m_already_existed && !tvc.m_verification_failed && !tvc.m_added_to_pool, false, descr << ": tx " << tx_id << " must be reported as an already existing one");
    break;
  default:
    CHECK_AND_ASSERT_MES(false, false, "unexpected tx_push_result");
  }
  return true;
}

bool gw_nonce_playtime_base::is_tx_in_pool(currency::core& c, const currency::transaction& tx) const
{
  transaction pool_tx{};
  return c.get_tx_pool().get_transaction(get_transaction_hash(tx), pool_tx);
}

bool gw_nonce_playtime_base::check_block_template(currency::core& c, const txs_refs& expected_txs, const std::string& descr) const
{
  currency::create_block_template_params cbtp{};
  cbtp.ignore_pow_ts_check = true;
  cbtp.miner_address = m_accounts[MINER_ACC_IDX].get_public_address();
  currency::create_block_template_response cbtr{};
  bool r = c.get_block_template(cbtp, cbtr);
  CHECK_AND_ASSERT_MES(r, false, descr << ": get_block_template failed");
  return check_tx_ids(cbtr.b.tx_hashes, expected_txs, descr + ", block template");
}

bool gw_nonce_playtime_base::mine_block(currency::core& c, const txs_refs& expected_txs, const std::string& descr) const
{
  block b{};
  bool r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, &b);
  CHECK_AND_ASSERT_MES(r, false, descr << ": mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_tail_id() == get_block_hash(b), false, descr << ": the mined block is not the top one");
  return check_tx_ids(b.tx_hashes, expected_txs, descr + ", mined block");
}

bool gw_nonce_playtime_base::mine_blocks_with_given_txs(currency::core& c, const std::vector<currency::transaction>& txs, const crypto::hash& prev_id, size_t blocks_count) const
{
  return mine_next_pow_blocks_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), txs, c, blocks_count, prev_id);
}

uint64_t gw_nonce_playtime_base::get_next_nonce(currency::core& c, size_t gw_index) const
{
  return c.get_blockchain_storage().get_gw_next_nonce(m_gw_view_keys[gw_index].pub);
}

//------------------------------------------------------------------------------

bool gw_nonce_pool_rules::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // Test idea: tx pool admission rules for txs with gw inputs, and the nonce the daemon picks for a new tx.
  // N is the nonce the blockchain expects, W is gw_nonce_max_gap, F is the fee of the pooled tx, S is gw_nonce_fee_window.
  // step | tx                                                   | pool
  // -----+------------------------------------------------------+--------------------------------
  // R1   | gw address A, nonce N                                | accept
  // R2   | gw address B, nonce N                                | accept (each address has its own nonces)
  // R3   | A, nonce N, fee F                                    | reject
  // R4   | A, nonce N, fee F + S - 1                            | reject
  // R5   | A, nonce N, fee F + S                                | accept, the old tx is replaced
  // R6   | A, nonce N, fee F + 2S - 1, then F + 2S              | reject, then accept (S is counted from the current tx)
  // R7   | the tx of R6 again                                   | already exists (not a failure)
  // R8   | A, nonce N + 2 (N + 1 is missing)                    | accept
  // R9   | A, nonce N + W - 1, then N + W                       | accept, then reject
  // R10  | nonce picked by gateway_create_transfer              | N for an empty queue, the highest pooled nonce + 1 otherwise; an explicit nonce is used as is
  // R11  | a block confirms the nonce N of A and of B           | the window moves: A, nonce N + W is accepted now
  // R12  | A, nonce N; B, nonce N                               | reject (already used in the blockchain)

  const size_t gw_a = 0, gw_b = 1, gw_c = 2;
  const core_runtime_config& crc = c.get_blockchain_storage().get_core_runtime_config();
  const uint64_t f = TESTS_DEFAULT_FEE;
  const uint64_t s = crc.gw_nonce_fee_window;
  const uint64_t w = crc.gw_nonce_max_gap;
  const uint64_t n = CURRENCY_GATEWAY_ADDRESS_NONCE_STARTER;
  CHECK_AND_ASSERT_MES(s != 0 && w > 3, false, "unexpected config: gw_nonce_fee_window = " << s << ", gw_nonce_max_gap = " << w);
  CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_a), n);
  CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_b), n);
  CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_c), n);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  // R1, R2
  transaction tx_a1{}, tx_b1{};
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, n, f, tx_a1));
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx_a1, tx_accepted, "R1"));
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_b, n, f, tx_b1));
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx_b1, tx_accepted, "R2"));
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 2);

  // R3, R4
  transaction tx{};
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, n, f, tx));
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx, tx_rejected, "R3"));
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, n, f + s - 1, tx));
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx, tx_rejected, "R4"));
  CHECK_AND_ASSERT_MES(is_tx_in_pool(c, tx_a1), false, "R4: tx_a1 must stay in the pool");

  // R5
  transaction tx_a1_r1{};
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, n, f + s, tx_a1_r1));
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx_a1_r1, tx_accepted, "R5"));
  CHECK_AND_ASSERT_MES(!is_tx_in_pool(c, tx_a1), false, "R5: tx_a1 must be replaced");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 2);

  // R6
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, n, f + 2 * s - 1, tx));
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx, tx_rejected, "R6, not enough fee"));
  transaction tx_a1_r2{};
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, n, f + 2 * s, tx_a1_r2));
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx_a1_r2, tx_accepted, "R6"));
  CHECK_AND_ASSERT_MES(!is_tx_in_pool(c, tx_a1_r1), false, "R6: tx_a1_r1 must be replaced");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 2);

  // R7
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx_a1_r2, tx_already_exists, "R7"));
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 2);

  // R8
  transaction tx_a3{};
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, n + 2, f, tx_a3));
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx_a3, tx_accepted, "R8"));

  // R9
  transaction tx_a_last{}, tx_a_next_window{};
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, n + w - 1, f, tx_a_last));
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx_a_last, tx_accepted, "R9, the last nonce of the window"));
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, n + w, f, tx_a_next_window));
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx_a_next_window, tx_rejected, "R9, the first nonce beyond the window"));
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 4);

  // R10
  auto get_rpc_nonce = [&](size_t gw_index, uint64_t requested_nonce, uint64_t& tx_nonce) -> bool
  {
    currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::request req = {};
    currency::COMMAND_RPC_GATEWAY_CREATE_TRANSFER::response resp = {};
    req.origin_gateway_id = m_gw_view_keys[gw_index].pub;
    req.destinations.push_back({ MK_TEST_COINS(1), m_accounts[BOB_ACC_IDX].get_public_address_str() });
    req.fee = TESTS_DEFAULT_FEE;
    req.gateway_view_secret_key = m_gw_view_keys[gw_index].sec;
    req.nonce = requested_nonce;
    bool r = invoke_text_json_for_core(c, "gateway_create_transfer", req, resp);
    CHECK_AND_ASSERT_MES(r && resp.status == API_RETURN_CODE_OK, false, "gateway_create_transfer failed, status: " << resp.status << ", " << resp.status_error);

    transaction rpc_tx{};
    CHECK_AND_ASSERT_MES(t_unserializable_object_from_blob(rpc_tx, resp.tx_blob), false, "failed to parse the tx made by gateway_create_transfer");
    for (const auto& in : rpc_tx.vin)
    {
      if (in.type() == typeid(txin_gateway))
      {
        tx_nonce = boost::get<txin_gateway>(in).nonce;
        return true;
      }
    }
    LOG_ERROR("the tx made by gateway_create_transfer has no gw inputs");
    return false;
  };

  uint64_t rpc_nonce = 0;
  CHECK_AND_ASSERT_TRUE(get_rpc_nonce(gw_c, 0, rpc_nonce)); // no pooled txs
  CHECK_AND_ASSERT_EQ(rpc_nonce, n);
  CHECK_AND_ASSERT_TRUE(get_rpc_nonce(gw_b, 0, rpc_nonce)); // pooled: N
  CHECK_AND_ASSERT_EQ(rpc_nonce, n + 1);
  CHECK_AND_ASSERT_TRUE(get_rpc_nonce(gw_a, 0, rpc_nonce)); // pooled: N, N + 2, N + W - 1 (gaps are not filled)
  CHECK_AND_ASSERT_EQ(rpc_nonce, n + w);
  CHECK_AND_ASSERT_TRUE(get_rpc_nonce(gw_c, n + 5, rpc_nonce)); // explicit nonce
  CHECK_AND_ASSERT_EQ(rpc_nonce, n + 5);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 4); // gateway_create_transfer doesn't send anything to the pool

  // R11: tx_a1_r2 pays more than tx_b1, so it goes first; tx_a3 and tx_a_last wait for the missing nonces
  CHECK_AND_ASSERT_TRUE(mine_block(c, { &tx_a1_r2, &tx_b1 }, "R11"));
  CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_a), n + 1);
  CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_b), n + 1);
  CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_c), n);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 2);
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, n + w, f, tx_a_next_window)); // the same nonce as in R9
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx_a_next_window, tx_accepted, "R11"));
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 3);

  // R12
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, n, f, tx));
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx, tx_rejected, "R12, gw address A"));
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_b, n, f + 10 * s, tx));
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx, tx_rejected, "R12, gw address B"));
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 3);

  return true;
}

//------------------------------------------------------------------------------

bool gw_nonce_block_template::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // Test idea: the order of gw txs in the block template, blocks with a wrong nonce, blacklisted and stale gw txs in the pool.
  // All blocks are mined using the block template, except the hand-built ones in T5 and T8.
  // step | pool                                                              | expected
  // -----+-------------------------------------------------------------------+--------------------------------------------------------------
  // T1   | A: N, N + 1, N + 2, each next one pays more                       | mined in the order of nonces
  // T2   | A: N + 1 only; then N comes                                       | nothing is mined; then both are mined in the order of nonces
  // T3   | A: N (fee 1), N + 1 (fee 5); B: N (fee 3); a regular tx (fee 10)  | B:N, A:N, A:N+1, the regular tx (gw txs go first, round by round)
  // T4   | A: N, replaced by a tx with a bigger fee                          | only the replacement is mined
  // T5   | A: N, N + 1; a hand-built block with N + 1 only                   | the block is rejected, the tx is not blacklisted, both are mined later
  // T6   | A: N (blacklisted), N + 1; B: N; C: N (blacklisted); a regular tx | only B:N is selected; C:N is replaced by a tx with the same fee;
  //      | (blacklisted)                                                     | the cleanup removes A:N only; a new A:N makes A:N+1 selectable again
  // T7   | A: N, N + 1; a hand-built block confirms another tx with A:N      | A:N is ignored by the template and removed by the cleanup; A:N+1 is mined

  const size_t gw_a = 0, gw_b = 1, gw_c = 2;
  const uint64_t f = TESTS_DEFAULT_FEE;
  uint64_t na = get_next_nonce(c, gw_a), nb = get_next_nonce(c, gw_b), nc = get_next_nonce(c, gw_c);
  CHECK_AND_ASSERT_EQ(na, CURRENCY_GATEWAY_ADDRESS_NONCE_STARTER);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);

  // T1: the txs come in the reverse order
  {
    transaction tx_1{}, tx_2{}, tx_3{};
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na,     1 * f, tx_1));
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na + 1, 2 * f, tx_2));
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na + 2, 3 * f, tx_3));
    CHECK_AND_ASSERT_TRUE(push_tx(c, tx_3, tx_accepted, "T1"));
    CHECK_AND_ASSERT_TRUE(push_tx(c, tx_2, tx_accepted, "T1"));
    CHECK_AND_ASSERT_TRUE(push_tx(c, tx_1, tx_accepted, "T1"));
    CHECK_AND_ASSERT_TRUE(check_block_template(c, { &tx_1, &tx_2, &tx_3 }, "T1"));
    CHECK_AND_ASSERT_TRUE(mine_block(c, { &tx_1, &tx_2, &tx_3 }, "T1"));
    na += 3;
    CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_a), na);
    CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  }

  // T2
  {
    transaction tx_1{}, tx_2{};
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na,     f, tx_1));
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na + 1, f, tx_2));
    CHECK_AND_ASSERT_TRUE(push_tx(c, tx_2, tx_accepted, "T2"));
    CHECK_AND_ASSERT_TRUE(check_block_template(c, { }, "T2, gap"));
    CHECK_AND_ASSERT_TRUE(mine_block(c, { }, "T2, gap"));
    CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_a), na);
    CHECK_AND_ASSERT_MES(is_tx_in_pool(c, tx_2), false, "T2: tx_2 must stay in the pool");
    CHECK_AND_ASSERT_TRUE(push_tx(c, tx_1, tx_accepted, "T2"));
    CHECK_AND_ASSERT_TRUE(mine_block(c, { &tx_1, &tx_2 }, "T2"));
    na += 2;
    CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_a), na);
    CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  }

  // T3
  {
    transaction tx_a1{}, tx_a2{}, tx_b1{}, tx_regular{};
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na,     1 * f, tx_a1));
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na + 1, 5 * f, tx_a2));
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_b, nb,     3 * f, tx_b1));
    CHECK_AND_ASSERT_TRUE(push_tx(c, tx_a1, tx_accepted, "T3"));
    CHECK_AND_ASSERT_TRUE(push_tx(c, tx_a2, tx_accepted, "T3"));
    CHECK_AND_ASSERT_TRUE(push_tx(c, tx_b1, tx_accepted, "T3"));
    CHECK_AND_ASSERT_TRUE(make_regular_tx(c, *miner_wlt, 10 * f, tx_regular));
    CHECK_AND_ASSERT_TRUE(check_block_template(c, { &tx_b1, &tx_a1, &tx_a2, &tx_regular }, "T3"));
    CHECK_AND_ASSERT_TRUE(mine_block(c, { &tx_b1, &tx_a1, &tx_a2, &tx_regular }, "T3"));
    na += 2;
    nb += 1;
    CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_a), na);
    CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_b), nb);
    CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  }

  // T4
  {
    transaction tx_1{}, tx_1_replacement{};
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na, 1 * f, tx_1));
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na, 2 * f, tx_1_replacement));
    CHECK_AND_ASSERT_TRUE(push_tx(c, tx_1, tx_accepted, "T4"));
    CHECK_AND_ASSERT_TRUE(push_tx(c, tx_1_replacement, tx_accepted, "T4, replacement"));
    CHECK_AND_ASSERT_TRUE(check_block_template(c, { &tx_1_replacement }, "T4"));
    CHECK_AND_ASSERT_TRUE(mine_block(c, { &tx_1_replacement }, "T4"));
    na += 1;
    CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_a), na);
    CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  }

  // T5
  {
    transaction tx_1{}, tx_2{};
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na,     f, tx_1));
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na + 1, f, tx_2));
    CHECK_AND_ASSERT_TRUE(push_tx(c, tx_1, tx_accepted, "T5"));
    CHECK_AND_ASSERT_TRUE(push_tx(c, tx_2, tx_accepted, "T5"));

    const crypto::hash top_block_id = c.get_tail_id();
    CHECK_AND_ASSERT_MES(!mine_blocks_with_given_txs(c, { tx_2 }, top_block_id, 1), false, "T5: a block with the nonce N + 1 only must be rejected");
    CHECK_AND_ASSERT_MES(c.get_tail_id() == top_block_id, false, "T5: the top block has changed");
    CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_a), na);
    CHECK_AND_ASSERT_MES(!c.get_tx_pool().is_tx_blacklisted(get_transaction_hash(tx_2)), false, "T5: tx_2 is blacklisted because of its place in someone's block");
    CHECK_AND_ASSERT_MES(is_tx_in_pool(c, tx_1) && is_tx_in_pool(c, tx_2), false, "T5: both txs must stay in the pool");

    CHECK_AND_ASSERT_TRUE(mine_block(c, { &tx_1, &tx_2 }, "T5"));
    na += 2;
    CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_a), na);
    CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  }

  // T6
  transaction tx_regular_blacklisted{};
  {
    transaction tx_a1{}, tx_a2{}, tx_b1{}, tx_c1{};
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na,     1 * f, tx_a1));
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na + 1, 1 * f, tx_a2));
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_b, nb,     3 * f, tx_b1));
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_c, nc,     1 * f, tx_c1));
    CHECK_AND_ASSERT_TRUE(push_tx(c, tx_a1, tx_accepted, "T6"));
    CHECK_AND_ASSERT_TRUE(push_tx(c, tx_a2, tx_accepted, "T6"));
    CHECK_AND_ASSERT_TRUE(push_tx(c, tx_b1, tx_accepted, "T6"));
    CHECK_AND_ASSERT_TRUE(push_tx(c, tx_c1, tx_accepted, "T6"));
    CHECK_AND_ASSERT_TRUE(make_regular_tx(c, *miner_wlt, f, tx_regular_blacklisted));

    c.get_tx_pool().add_transaction_to_black_list(tx_a1);
    c.get_tx_pool().add_transaction_to_black_list(tx_c1);
    c.get_tx_pool().add_transaction_to_black_list(tx_regular_blacklisted);
    // a blacklisted tx is not selected, and nothing that follows it for the same gw address is selected either
    CHECK_AND_ASSERT_TRUE(check_block_template(c, { &tx_b1 }, "T6, blacklisted"));

    // a blacklisted tx doesn't protect its nonce: a tx with the same fee replaces it
    transaction tx_c1_replacement{};
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_c, nc, 1 * f, tx_c1_replacement));
    CHECK_AND_ASSERT_TRUE(push_tx(c, tx_c1_replacement, tx_accepted, "T6, replacement of a blacklisted tx"));
    CHECK_AND_ASSERT_MES(!is_tx_in_pool(c, tx_c1), false, "T6: tx_c1 must be replaced");
    CHECK_AND_ASSERT_TRUE(check_block_template(c, { &tx_b1, &tx_c1_replacement }, "T6, replaced"));

    // the cleanup removes a blacklisted tx only if it has gw inputs
    CHECK_AND_ASSERT_TRUE(c.get_tx_pool().remove_stuck_transactions());
    CHECK_AND_ASSERT_MES(!is_tx_in_pool(c, tx_a1), false, "T6: blacklisted tx_a1 must be removed by the cleanup");
    CHECK_AND_ASSERT_MES(is_tx_in_pool(c, tx_a2) && is_tx_in_pool(c, tx_b1) && is_tx_in_pool(c, tx_c1_replacement), false, "T6: the cleanup removed a wrong tx");
    CHECK_AND_ASSERT_MES(is_tx_in_pool(c, tx_regular_blacklisted), false, "T6: a blacklisted tx without gw inputs must stay in the pool");
    CHECK_AND_ASSERT_TRUE(check_block_template(c, { &tx_b1, &tx_c1_replacement }, "T6, after the cleanup")); // tx_a2 waits for the nonce N

    transaction tx_a1_new{};
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na, 2 * f, tx_a1_new));
    CHECK_AND_ASSERT_TRUE(push_tx(c, tx_a1_new, tx_accepted, "T6, the nonce is free again"));
    CHECK_AND_ASSERT_TRUE(check_block_template(c, { &tx_b1, &tx_a1_new, &tx_c1_replacement, &tx_a2 }, "T6, final"));
    CHECK_AND_ASSERT_TRUE(mine_block(c, { &tx_b1, &tx_a1_new, &tx_c1_replacement, &tx_a2 }, "T6"));
    na += 2;
    nb += 1;
    nc += 1;
    CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_a), na);
    CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_b), nb);
    CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_c), nc);
    CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1); // tx_regular_blacklisted
  }

  // T7
  {
    transaction tx_1{}, tx_1_other{}, tx_2{};
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na,     f, tx_1));
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na,     f, tx_1_other)); // never comes to the pool
    CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na + 1, f, tx_2));
    CHECK_AND_ASSERT_TRUE(push_tx(c, tx_1, tx_accepted, "T7"));
    CHECK_AND_ASSERT_TRUE(push_tx(c, tx_2, tx_accepted, "T7"));

    CHECK_AND_ASSERT_MES(mine_blocks_with_given_txs(c, { tx_1_other }, c.get_tail_id(), 1), false, "T7: a block with tx_1_other was not accepted");
    CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_a), na + 1);
    CHECK_AND_ASSERT_MES(is_tx_in_pool(c, tx_1) && is_tx_in_pool(c, tx_2), false, "T7: both txs must stay in the pool");
    CHECK_AND_ASSERT_TRUE(check_block_template(c, { &tx_2 }, "T7, stale tx in the pool"));

    CHECK_AND_ASSERT_TRUE(c.get_tx_pool().remove_stuck_transactions());
    CHECK_AND_ASSERT_MES(!is_tx_in_pool(c, tx_1), false, "T7: tx_1 with a used nonce must be removed by the cleanup");
    CHECK_AND_ASSERT_MES(is_tx_in_pool(c, tx_2), false, "T7: tx_2 must stay in the pool");

    CHECK_AND_ASSERT_TRUE(mine_block(c, { &tx_2 }, "T7"));
    na += 2;
    CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_a), na);
    CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1); // tx_regular_blacklisted
  }

  return true;
}

//------------------------------------------------------------------------------

bool gw_nonce_template_size_caps::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // Test idea: size limits of the block template.
  // gw txs may take up to 80% of CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE; all txs together never go beyond the penalty-free size
  // (tx fees are burned, so a miner gets nothing for a bigger block).
  //
  // pool: A: a1, a2, a3 (big ones), a4 (small, the biggest fee); B: b1 (big); regular: r1, r2 (medium ones)
  // block | expected txs      | why
  // ------+-------------------+----------------------------------------------------------------------------------------------
  // 1     | a1, b1, a2, r1    | a3 doesn't fit into the gw part, so a4 is not taken either (it follows a3); r1 uses the room
  //       |                   | that is left for regular txs; r2 doesn't fit into the penalty-free size
  // 2     | a3, a4, r2        | everything that is left

  const size_t gw_a = 0, gw_b = 1;
  const uint64_t f = TESTS_DEFAULT_FEE;
  const size_t gw_padding = 30000, regular_padding = 20000;
  const size_t gw_txs_size_limit = CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE - CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE / 5;
  const size_t all_txs_size_limit = CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE - CURRENCY_COINBASE_BLOB_RESERVED_SIZE_HF6;
  uint64_t na = get_next_nonce(c, gw_a), nb = get_next_nonce(c, gw_b);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);

  transaction tx_a1{}, tx_a2{}, tx_a3{}, tx_a4{}, tx_b1{}, tx_r1{}, tx_r2{};
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na,     4 * f, tx_a1, gw_padding));
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na + 1, 3 * f, tx_a2, gw_padding));
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na + 2, 1 * f, tx_a3, gw_padding));
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, na + 3, 9 * f, tx_a4));
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_b, nb,     2 * f, tx_b1, gw_padding));
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx_a1, tx_accepted, "a1"));
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx_a2, tx_accepted, "a2"));
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx_a3, tx_accepted, "a3"));
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx_a4, tx_accepted, "a4"));
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx_b1, tx_accepted, "b1"));
  CHECK_AND_ASSERT_TRUE(make_regular_tx(c, *miner_wlt, 2 * f, tx_r1, regular_padding));
  CHECK_AND_ASSERT_TRUE(make_regular_tx(c, *miner_wlt, 1 * f, tx_r2, regular_padding));
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 7);

  // make sure the sizes of the txs give the intended picture
  const size_t gw_part_size = get_object_blobsize(tx_a1) + get_object_blobsize(tx_b1) + get_object_blobsize(tx_a2);
  const size_t block_1_size = gw_part_size + get_object_blobsize(tx_r1);
  CHECK_AND_ASSERT_MES(gw_part_size <= gw_txs_size_limit, false, "a1, b1, a2 must fit into the gw part: " << gw_part_size);
  CHECK_AND_ASSERT_MES(gw_part_size + get_object_blobsize(tx_a3) > gw_txs_size_limit, false, "a3 must not fit into the gw part");
  CHECK_AND_ASSERT_MES(gw_part_size + get_object_blobsize(tx_a4) <= gw_txs_size_limit, false, "a4 itself must fit into the gw part");
  CHECK_AND_ASSERT_MES(block_1_size > gw_txs_size_limit && block_1_size <= all_txs_size_limit, false, "r1 must fit into the penalty-free size beyond the gw part: " << block_1_size);
  CHECK_AND_ASSERT_MES(block_1_size + get_object_blobsize(tx_r2) > all_txs_size_limit, false, "r2 must not fit into the penalty-free size");

  CHECK_AND_ASSERT_TRUE(check_block_template(c, { &tx_a1, &tx_b1, &tx_a2, &tx_r1 }, "block 1"));
  CHECK_AND_ASSERT_TRUE(mine_block(c, { &tx_a1, &tx_b1, &tx_a2, &tx_r1 }, "block 1"));
  na += 2;
  nb += 1;
  CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_a), na);
  CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_b), nb);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 3);

  CHECK_AND_ASSERT_TRUE(check_block_template(c, { &tx_a3, &tx_a4, &tx_r2 }, "block 2"));
  CHECK_AND_ASSERT_TRUE(mine_block(c, { &tx_a3, &tx_a4, &tx_r2 }, "block 2"));
  na += 2;
  CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_a), na);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  return true;
}

//------------------------------------------------------------------------------

bool gw_nonce_reorg_and_pool::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // Test idea: gw txs in the pool during chain switches.
  //
  //  F (fork point, the expected nonce is N)
  //    |- B1 (tx_1: N) - B2 (tx_2: N + 1)                             chain B
  //    \- C1 - C2 - C3 - E (tx_1, tx_2) - E1 - E2                     chain C
  //                   \- D1 (tx_1_other: N) - D2                      chain D
  //
  // step | action                                             | main chain | expected
  // -----+----------------------------------------------------+------------+------------------------------------------------------------
  // S1   | tx_1_other (bigger fee) and tx_2 come to the pool  |            |
  //      | B1 is hand-built with tx_1, B2 is mined            | B          | nonce N + 2; tx_1_other is still in the pool
  // S2   | C1 .. C3                                           | C          | nonce N; tx_1 and tx_2 are back in the pool, tx_1 takes
  //      |                                                    |            | the nonce N from tx_1_other regardless of the fee
  // S3   | E is mined                                         | C          | tx_1 and tx_2 are confirmed again, nonce N + 2
  // S4   | D1 (hand-built with tx_1_other), D2                | D          | nonce N + 1; tx_1 has a used nonce, only tx_2 is selected
  // S5   | the pool is cleared, then E1, E2                   | C          | nonce N + 2: E is applied again from its own copies of txs

  const size_t gw_a = 0;
  const uint64_t f = TESTS_DEFAULT_FEE;
  const uint64_t n = get_next_nonce(c, gw_a);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  transaction tx_1{}, tx_1_other{}, tx_2{};
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, n,     1 * f, tx_1));
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, n,     5 * f, tx_1_other));
  CHECK_AND_ASSERT_TRUE(make_gw_tx(c, gw_a, n + 1, 1 * f, tx_2));
  const crypto::hash tx_1_id = get_transaction_hash(tx_1), tx_2_id = get_transaction_hash(tx_2);

  // S1
  const crypto::hash blk_f_id = c.get_tail_id();
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx_1_other, tx_accepted, "S1"));
  CHECK_AND_ASSERT_TRUE(push_tx(c, tx_2, tx_accepted, "S1"));
  CHECK_AND_ASSERT_MES(mine_blocks_with_given_txs(c, { tx_1 }, blk_f_id, 1), false, "S1: block B1 was not accepted");
  CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_a), n + 1);
  CHECK_AND_ASSERT_TRUE(mine_block(c, { &tx_2 }, "S1, block B2")); // tx_1_other has a used nonce now
  CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_a), n + 2);
  CHECK_AND_ASSERT_MES(is_tx_in_pool(c, tx_1_other), false, "S1: tx_1_other must stay in the pool");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);

  // S2
  CHECK_AND_ASSERT_MES(mine_blocks_with_given_txs(c, { }, blk_f_id, 3), false, "S2: chain C was not accepted");
  const crypto::hash blk_c3_id = c.get_tail_id();
  CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_a), n);
  CHECK_AND_ASSERT_MES(is_tx_in_pool(c, tx_1) && is_tx_in_pool(c, tx_2), false, "S2: tx_1 and tx_2 must return to the pool");
  CHECK_AND_ASSERT_MES(!is_tx_in_pool(c, tx_1_other), false, "S2: tx_1_other must give the nonce back to tx_1");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 2);

  // S3
  CHECK_AND_ASSERT_TRUE(check_block_template(c, { &tx_1, &tx_2 }, "S3"));
  CHECK_AND_ASSERT_TRUE(mine_block(c, { &tx_1, &tx_2 }, "S3, block E"));
  const crypto::hash blk_e_id = c.get_tail_id();
  CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_a), n + 2);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  // S4
  CHECK_AND_ASSERT_MES(mine_blocks_with_given_txs(c, { tx_1_other }, blk_c3_id, 2), false, "S4: chain D was not accepted");
  const crypto::hash blk_d2_id = c.get_tail_id();
  CHECK_AND_ASSERT_MES(blk_d2_id != blk_e_id, false, "S4: chain D didn't become the main one");
  CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_a), n + 1);
  CHECK_AND_ASSERT_MES(is_tx_in_pool(c, tx_1) && is_tx_in_pool(c, tx_2), false, "S4: tx_1 and tx_2 must return to the pool");
  CHECK_AND_ASSERT_MES(!is_tx_in_pool(c, tx_1_other), false, "S4: tx_1_other is confirmed and must not be in the pool");
  CHECK_AND_ASSERT_TRUE(check_block_template(c, { &tx_2 }, "S4"));

  // S5
  c.get_tx_pool().purge_transactions();
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  CHECK_AND_ASSERT_MES(mine_blocks_with_given_txs(c, { }, blk_e_id, 2), false, "S5: blocks E1, E2 were not accepted");
  CHECK_AND_ASSERT_MES(c.get_tail_id() != blk_d2_id, false, "S5: chain C didn't become the main one again");
  CHECK_AND_ASSERT_EQ(get_next_nonce(c, gw_a), n + 2);
  CHECK_AND_ASSERT_MES(c.get_blockchain_storage().have_tx(tx_1_id) && c.get_blockchain_storage().have_tx(tx_2_id), false, "S5: tx_1 and tx_2 must be confirmed again");
  CHECK_AND_ASSERT_MES(is_tx_in_pool(c, tx_1_other), false, "S5: tx_1_other must return to the pool");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);

  // tx_1_other has a used nonce again, this time for good
  CHECK_AND_ASSERT_TRUE(check_block_template(c, { }, "S5"));

  return true;
}
