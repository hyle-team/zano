// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "hf_booster_tests.h"
#include "chaingen_helpers.h"
#include "currency_core/bc_hf_booster_service.h"
#include "currency_core/bc_attachments_helpers.h"
#include "wallet/wallet_internal_events_definitions.h"
#include "crypto/zarcanum.h"

using namespace currency;

namespace
{
  const uint64_t hf_booster_test_min_distance_between_boost_and_hf = 5;
  const uint64_t hf_booster_test_hf8_initial_active_after_height   = 100;  // the first HF8 block is expected to have height 26  
  const uint64_t hf_booster_test_hf8_override_active_after_height   = 25; // the first HF8 block is expected to have height 26
  const uint64_t hf_booster_test_unknown_hf_far_height              = 10000; // for recorded boosters of an unknown id: never reached, it would stop the node
}

//------------------------------------------------------------------------------

hf_booster_hf8_activation_basic::hf_booster_hf8_activation_basic()
  : m_maintainer_keys(keypair::generate())
{
  REGISTER_CALLBACK_METHOD(hf_booster_hf8_activation_basic, configure_core);
  REGISTER_CALLBACK_METHOD(hf_booster_hf8_activation_basic, c1);

  // HF1..HF7, HF8 are active from the very beginning
  m_hardforks.clear();
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_07, 1);
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_08, hf_booster_test_hf8_initial_active_after_height);
  m_hardforks.last_supported_hardfork_id = ZANO_HARDFORK_08; // the booster moves HF8, so this build must be ready for it
}

bool hf_booster_hf8_activation_basic::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  CHECK_AND_ASSERT_MES(test_chain_unit_enchanced::configure_core(c, ev_index, events), false, "default configure_core failed");
  core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.hf_maintainers_public_key = m_maintainer_keys.pub;  
  pc.minimal_distance_between_boost_and_hf = hf_booster_test_min_distance_between_boost_and_hf;
  pc.pre_hardfork_tx_freeze_period = 0; // the boosted activation is closer than the default freeze period, so keep the freeze out of the way
  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

bool hf_booster_hf8_activation_basic::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: HF8 is not scheduled. The maintainer submits an HF booster tx, signed with the HF8 maintainer key,
  // which sets the HF8 activation height. Make sure the core follows the new schedule only once the booster tx
  // is included in a block, that HF8 becomes active exactly after the given height, and that wallets keep working
  // and create HF8 transactions afterwards.

  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  m_accounts[ALICE_ACC_IDX].generate();
  m_accounts[ALICE_ACC_IDX].set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_07));
  DO_CALLBACK_PARAMS(events, "check_hardfork_inactive", static_cast<size_t>(ZANO_HARDFORK_08));

  DO_CALLBACK(events, "c1");

  return true;
}

bool hf_booster_hf8_activation_basic::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  blockchain_storage& bcs = c.get_blockchain_storage();
  const account_public_address& miner_addr = m_accounts[MINER_ACC_IDX].get_public_address();

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  miner_wlt->refresh();

  CHECK_AND_ASSERT_EQ(bcs.get_core_runtime_config().hard_forks.get_height_the_hardfork_active_after(ZANO_HARDFORK_08), hf_booster_test_hf8_initial_active_after_height);

  // the booster tx is going to be included in the next block
  const uint64_t booster_block_height = c.get_top_block_height() + 1;
  CHECK_AND_ASSERT_MES(booster_block_height + hf_booster_test_min_distance_between_boost_and_hf < hf_booster_test_hf8_initial_active_after_height, false,
    "test setup: HF8 height " << hf_booster_test_hf8_initial_active_after_height << " is too close to the booster block height " << booster_block_height);

  //
  // the maintainer sends the booster tx
  //
  finalized_tx ft{};
  miner_wlt->submit_hf_booster(ZANO_HARDFORK_08, hf_booster_test_hf8_override_active_after_height, m_maintainer_keys.sec, ft);
  CHECK_AND_ASSERT_MES(!ft.was_not_prepared, false, "booster tx was not prepared");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  const crypto::hash booster_tx_id = get_transaction_hash(ft.tx);

  // the booster is carried in tx extra, bound to the tx one-time key and signed with the HF8 maintainer key
  bc_services::hf_booster_update upd{};
  CHECK_AND_ASSERT_MES(bc_services::get_service_attachment_gzipped_json_by_id(ft.tx.extra, BC_HF_BOOSTER_SERVICE_ID, BC_HF_BOOSTER_SERVICE_INSTRUCTION_UPDATE, upd), false,
    "HF booster entry was not found in tx extra");
  CHECK_AND_ASSERT_EQ(upd.hf_id, ZANO_HARDFORK_08);
  CHECK_AND_ASSERT_EQ(upd.hf_new_activation_height, hf_booster_test_hf8_override_active_after_height);
  CHECK_AND_ASSERT_MES(upd.tx_onetime_pub_key == get_tx_pub_key_from_extra(ft.tx), false, "HF booster entry is bound to another tx key");
  CHECK_AND_ASSERT_MES(crypto::verify_schnorr_sig(bc_services::get_hf_booster_update_hash(upd), m_maintainer_keys.pub, upd.maintainer_signature), false,
    "HF booster entry has invalid maintainer signature");

  // a booster tx in the pool doesn't change the schedule
  CHECK_AND_ASSERT_EQ(bcs.get_core_runtime_config().hard_forks.get_height_the_hardfork_active_after(ZANO_HARDFORK_08), hf_booster_test_hf8_initial_active_after_height);

  //
  // the booster tx gets into a block
  //
  CHECK_AND_ASSERT_MES(mine_next_pow_block_in_playtime(miner_addr, c), false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_EQ(c.get_top_block_height(), booster_block_height);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  CHECK_AND_ASSERT_MES(bcs.have_tx(booster_tx_id), false, "booster tx was not included in the blockchain");

  CHECK_AND_ASSERT_EQ(bcs.get_core_runtime_config().hard_forks.get_height_the_hardfork_active_after(ZANO_HARDFORK_08), hf_booster_test_hf8_override_active_after_height);

  //
  // mine up to the boosted height: HF7 rules till the very last block
  //
  while(c.get_top_block_height() < hf_booster_test_hf8_override_active_after_height)
  {
    CHECK_AND_ASSERT_MES(!bcs.is_hardfork_active(ZANO_HARDFORK_08), false, "HF8 is active prematurely, top block height: " << c.get_top_block_height());
    block b{};
    CHECK_AND_ASSERT_MES(mine_next_pow_block_in_playtime(miner_addr, c, &b), false, "mine_next_pow_block_in_playtime failed");
    CHECK_AND_ASSERT_EQ(static_cast<size_t>(b.miner_tx.hardfork_id), ZANO_HARDFORK_07);
  }
  CHECK_AND_ASSERT_EQ(c.get_top_block_height(), hf_booster_test_hf8_override_active_after_height);

  //
  // the first HF8 block
  //
  CHECK_AND_ASSERT_MES(bcs.is_hardfork_active(ZANO_HARDFORK_08), false, "HF8 is not active after the boosted height");
  block blk_hf8{};
  CHECK_AND_ASSERT_MES(mine_next_pow_block_in_playtime(miner_addr, c, &blk_hf8), false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_EQ(get_block_height(blk_hf8), hf_booster_test_hf8_override_active_after_height + 1);
  CHECK_AND_ASSERT_EQ(static_cast<size_t>(blk_hf8.miner_tx.hardfork_id), ZANO_HARDFORK_08);

  //
  // wallets follow the new schedule: they sync with the daemon and create HF8 txs
  //
  miner_wlt->refresh();
  transaction tx{};
  miner_wlt->transfer(MK_TEST_COINS(7), m_accounts[ALICE_ACC_IDX].get_public_address(), tx);
  CHECK_AND_ASSERT_EQ(static_cast<size_t>(tx.hardfork_id), ZANO_HARDFORK_08);
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 1);
  CHECK_AND_ASSERT_MES(mine_next_pow_block_in_playtime(miner_addr, c), false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  alice_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", MK_TEST_COINS(7)), false, "");

  return true;
}

//------------------------------------------------------------------------------
// gentime tests
//------------------------------------------------------------------------------

// Common chain prologue for the gentime tests: genesis, core config, a funded Alice, everything unlocked.
// After it the top block is BLK_HEAD (height 21) and the next block (the first one a booster can get into) has height 22.
// It takes 28 events, and a tx and the block that carries it are always in the same group of 7 events: the blockchain
// save/load replay mode restarts the core between such groups, and a restarted core drops the pool txs it considers
// incompatible before the test config is applied.
#define HF_BOOSTER_CHAIN_PROLOGUE(BLK_HEAD)                                                                       \
  uint64_t ts = test_core_time::get_time();                                                                       \
  m_accounts.resize(TOTAL_ACCS_COUNT);                                                                            \
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);       \
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);       \
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);                                                               \
  DO_CALLBACK(events, "configure_core");                                                                          \
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);                          \
  MAKE_TX(events, tx_fund, miner_acc, alice_acc, MK_TEST_COINS(100), blk_0r);                                     \
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_fund);                                                 \
  REWIND_BLOCKS_N(events, BLK_HEAD, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);                        \
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_07));                     \
  DO_CALLBACK_PARAMS(events, "check_hardfork_inactive", static_cast<size_t>(ZANO_HARDFORK_08));                   \
  DO_CALLBACK_PARAMS(events, "check_hf8_height", hf_booster_test_hf8_initial_active_after_height);                \
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0))

#define HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(TX)                                                                    \
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));                   \
  ADD_CUSTOM_EVENT(events, TX);                                                                                   \
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false))

hf_booster_test_base::hf_booster_test_base()
  : m_maintainer_keys(keypair::generate())
{
  REGISTER_CALLBACK_METHOD(hf_booster_test_base, configure_core);
  REGISTER_CALLBACK_METHOD(hf_booster_test_base, check_hf8_height);
  REGISTER_CALLBACK_METHOD(hf_booster_test_base, check_unsupported_hardfork_id);

  m_hardforks.clear();
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_07, 1);
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_08, hf_booster_test_hf8_initial_active_after_height);
  m_hardforks.last_supported_hardfork_id = ZANO_HARDFORK_08; // the booster moves HF8, so this build must be ready for it
}

core_runtime_config hf_booster_test_base::get_runtime_info_for_core() const
{
  core_runtime_config crc = test_chain_unit_base::get_runtime_info_for_core();
  crc.hf_maintainers_public_key = m_maintainer_keys.pub;
  crc.minimal_distance_between_boost_and_hf = hf_booster_test_min_distance_between_boost_and_hf;
  crc.pre_hardfork_tx_freeze_period = 0;
  return crc;
}

bool hf_booster_test_base::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  CHECK_AND_ASSERT_MES(test_chain_unit_enchanced::configure_core(c, ev_index, events), false, "default configure_core failed");
  core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.hf_maintainers_public_key = m_maintainer_keys.pub;
  pc.minimal_distance_between_boost_and_hf = hf_booster_test_min_distance_between_boost_and_hf;
  pc.pre_hardfork_tx_freeze_period = 0;
  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

bool hf_booster_test_base::check_hf8_height(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  uint64_t expected = 0;
  const std::string& params = boost::get<callback_entry>(events[ev_index]).callback_params;
  CHECK_AND_ASSERT_MES(epee::string_tools::hex_to_pod(params, expected), false, "hex_to_pod failed, params = " << params);

  uint64_t actual = c.get_blockchain_storage().get_core_runtime_config().hard_forks.get_height_the_hardfork_active_after(ZANO_HARDFORK_08);
  CHECK_AND_ASSERT_MES(actual == expected, false, "HF8 is scheduled after height " << actual << ", expected: " << expected << " (top block height: " << c.get_top_block_height() << ")");
  return true;
}

bool hf_booster_test_base::check_unsupported_hardfork_id(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  uint64_t expected = 0;
  const std::string& params = boost::get<callback_entry>(events[ev_index]).callback_params;
  CHECK_AND_ASSERT_MES(epee::string_tools::hex_to_pod(params, expected), false, "hex_to_pod failed, params = " << params);

  const auto& pd = c.get_blockchain_storage().get_performnce_data();
  uint64_t actual = pd.unsupported_hardfork_id;
  CHECK_AND_ASSERT_MES(actual == expected, false, "unsupported_hardfork_id is " << actual << ", expected: " << expected);
  CHECK_AND_ASSERT_MES(expected == 0 || pd.epic_failure_happend, false, "an unsupported hardfork is recorded, but the stop flag is not set");
  return true;
}

bool hf_booster_test_base::make_booster_tx(const std::vector<test_event_entry>& events, const block& blk_head, const hard_forks_descriptor& hf,
  const account_base& from, uint64_t hf_id, uint64_t new_height, transaction& tx, const booster_tx_options& opt /* = booster_tx_options() */) const
{
  // a self-directed tx carrying a booster entry in its extra, the same way the wallet builds it
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  bool r = fill_tx_sources_and_destinations(events, blk_head, from, from, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");

  size_t tx_hardfork_id = 0;
  uint64_t tx_version = get_tx_version_and_hardfork_id(get_block_height(blk_head) + 1, hf, tx_hardfork_id);
  finalize_tx_param ftp{};
  ftp.tx_version = tx_version;
  ftp.tx_hardfork_id = tx_hardfork_id;
  ftp.sources = sources;
  ftp.prepared_destinations = destinations;
  ftp.shuffle = true;

  const crypto::secret_key& signer_sec = opt.p_signer_sec ? *opt.p_signer_sec : m_maintainer_keys.sec;

  epee::misc_utils::events_dispatcher dispatcher;
  dispatcher.SUBSCIRBE_EVENT<wie_construct_tx_on_onetime_key_created>([&](wie_construct_tx_on_onetime_key_created& o)
  {
    bc_services::hf_booster_update upd{};
    upd.hf_id = hf_id;
    upd.hf_new_activation_height = new_height;
    upd.tx_onetime_pub_key = opt.p_bind_to_pub_key ? *opt.p_bind_to_pub_key : o.onetime_keypair.pub;
    crypto::hash h = bc_services::get_hf_booster_update_hash(upd);
    bool sig_ok = crypto::generate_schnorr_sig(h, signer_sec, upd.maintainer_signature);
    CHECK_AND_ASSERT_THROW_MES(sig_ok, "generate_schnorr_sig failed");

    tx_service_attachment att{};
    bc_services::pack_attachment_as_gzipped_json(upd, att);
    att.service_id = BC_HF_BOOSTER_SERVICE_ID;
    att.instruction = BC_HF_BOOSTER_SERVICE_INSTRUCTION_UPDATE;
    o.tx.extra.push_back(att);
  });
  ftp.pevents_dispatcher = &dispatcher;

  finalized_tx ft{};
  r = construct_tx(from.get_keys(), ftp, ft);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  tx = ft.tx;
  return true;
}

//------------------------------------------------------------------------------

bool hf_booster_gentime_basic::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: a booster tx moves HF8 closer, the chain crosses the boosted height, HF8 txs are accepted after it.
  // The same scenario runs in the blockchain save/load mode, where the core is restarted between event chunks.
  // Event layout matters for the save/load mode, see the prologue comment: the booster tx (28) and its block (30),
  // and the HF8 tx (47) and its block (48) must not be separated by a multiple of 7.
  HF_BOOSTER_CHAIN_PROLOGUE(blk_1r);                                                                 // 21, events 0..27
  const uint64_t boosted_height = get_block_height(blk_1r) + 1 + hf_booster_test_min_distance_between_boost_and_hf + 7; // 34

  transaction tx_b1{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_1r, generator.get_hardforks(), miner_acc, ZANO_HARDFORK_08, boosted_height, tx_b1), false, "make_booster_tx failed");
  ADD_CUSTOM_EVENT(events, tx_b1);                                                                   // event 28
  DO_CALLBACK_PARAMS(events, "check_hf8_height", hf_booster_test_hf8_initial_active_after_height); // not applied while in the pool
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_b1);                                      // 22, event 30
  DO_CALLBACK_PARAMS(events, "check_hf8_height", boosted_height);

  // the generator has to know the new schedule to produce correct blocks after the activation
  generator.set_hardfork_height(ZANO_HARDFORK_08, boosted_height);
  REWIND_BLOCKS_N(events, blk_2r, blk_2, miner_acc, boosted_height - get_block_height(blk_2) - 1);  // 23..33
  DO_CALLBACK_PARAMS(events, "check_hardfork_inactive", static_cast<size_t>(ZANO_HARDFORK_08));
  MAKE_NEXT_BLOCK(events, blk_3, blk_2r, miner_acc);                                                 // 34, the last HF7 block
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_08));
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_acc);                                                  // 35, the first HF8 block
  DO_CALLBACK_PARAMS(events, "check_hf8_height", boosted_height);

  // an ordinary HF8 tx
  MAKE_TX(events, tx_1, miner_acc, alice_acc, MK_TEST_COINS(5), blk_4);                              // event 47
  MAKE_NEXT_BLOCK_TX1(events, blk_5, blk_4, miner_acc, tx_1);                                        // 36, event 48
  DO_CALLBACK_PARAMS(events, "check_hf8_height", boosted_height);
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, MK_TEST_COINS(105)));

  return true;
}

//------------------------------------------------------------------------------

bool hf_booster_reorg_before_activation::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: boosters are applied on block connect and reverted on block disconnect.
  // 1) main chain: booster b1 (HF8@35) at 22, booster b2 (postpone to HF8@45) at 25
  // 2) a chain switch that disconnects b2 only -> HF8@35
  // 3) a chain switch that disconnects b1 as well -> HF8 back to the default
  // 4) the new chain crosses both former heights under HF7, then a new booster works on it
  HF_BOOSTER_CHAIN_PROLOGUE(blk_1r);                                                                 // 21
  const uint64_t h1 = 35, h2 = 45, h3 = 60;

  transaction tx_b1{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_1r, generator.get_hardforks(), miner_acc, ZANO_HARDFORK_08, h1, tx_b1), false, "make_booster_tx failed");
  ADD_CUSTOM_EVENT(events, tx_b1);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_b1);                                      // 22
  DO_CALLBACK_PARAMS(events, "check_hf8_height", h1);
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);                                                  // 23
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_acc);                                                  // 24

  transaction tx_b2{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_4, generator.get_hardforks(), alice_acc, ZANO_HARDFORK_08, h2, tx_b2), false, "make_booster_tx failed");
  ADD_CUSTOM_EVENT(events, tx_b2);
  MAKE_NEXT_BLOCK_TX1(events, blk_5, blk_4, miner_acc, tx_b2);                                       // 25
  DO_CALLBACK_PARAMS(events, "check_hf8_height", h2);

  // alt chain A forks after b1 and before b2, becomes the main chain
  MAKE_NEXT_BLOCK(events, blk_5a, blk_4, miner_acc);                                                 // 25
  DO_CALLBACK_PARAMS(events, "check_hf8_height", h2);                                                // still main chain with b2
  MAKE_NEXT_BLOCK(events, blk_6a, blk_5a, miner_acc);                                                // 26, switch
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_6a));
  DO_CALLBACK_PARAMS(events, "check_hf8_height", h1);                                                // b2 disconnected, b1 still there

  // alt chain B forks before b1, becomes the main chain
  REWIND_BLOCKS_N(events, blk_7b, blk_1r, miner_acc, 6);                                             // 22..27, switch at 27
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_7b));
  DO_CALLBACK_PARAMS(events, "check_hf8_height", hf_booster_test_hf8_initial_active_after_height);  // no boosters on this chain

  // no activation at the former heights
  REWIND_BLOCKS_N(events, blk_7br, blk_7b, miner_acc, 20);                                           // 28..47
  DO_CALLBACK_PARAMS(events, "check_hardfork_inactive", static_cast<size_t>(ZANO_HARDFORK_08));
  DO_CALLBACK_PARAMS(events, "check_hf8_height", hf_booster_test_hf8_initial_active_after_height);

  // a new booster on the new main chain, then the activation
  transaction tx_b3{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_7br, generator.get_hardforks(), miner_acc, ZANO_HARDFORK_08, h3, tx_b3), false, "make_booster_tx failed");
  ADD_CUSTOM_EVENT(events, tx_b3);
  MAKE_NEXT_BLOCK_TX1(events, blk_8, blk_7br, miner_acc, tx_b3);                                     // 48
  DO_CALLBACK_PARAMS(events, "check_hf8_height", h3);

  generator.set_hardfork_height(ZANO_HARDFORK_08, h3);
  REWIND_BLOCKS_N(events, blk_8r, blk_8, miner_acc, h3 - get_block_height(blk_8));                   // 49..60
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_08));
  MAKE_NEXT_BLOCK(events, blk_9, blk_8r, miner_acc);                                                 // 61, the first HF8 block
  DO_CALLBACK_PARAMS(events, "check_hf8_height", h3);

  return true;
}

//------------------------------------------------------------------------------

bool hf_booster_reorg_after_activation::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: a chain switch across the boosted activation height, both chains contain the booster,
  // the alt chain is built with HF8 blocks after the boosted height. The schedule survives the switch.
  HF_BOOSTER_CHAIN_PROLOGUE(blk_1r);                                                                 // 21
  const uint64_t h1 = 30;

  transaction tx_b1{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_1r, generator.get_hardforks(), miner_acc, ZANO_HARDFORK_08, h1, tx_b1), false, "make_booster_tx failed");
  ADD_CUSTOM_EVENT(events, tx_b1);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_b1);                                      // 22
  DO_CALLBACK_PARAMS(events, "check_hf8_height", h1);

  generator.set_hardfork_height(ZANO_HARDFORK_08, h1);
  REWIND_BLOCKS_N(events, blk_2r, blk_2, miner_acc, 9);                                              // 23..31, HF8 is active after 30
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_08));

  // alt chain forks right after the booster block and outgrows the main chain
  REWIND_BLOCKS_N(events, blk_2ar, blk_2, miner_acc, 10);                                            // 23..32, switch at 32
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_2ar));
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_08));
  DO_CALLBACK_PARAMS(events, "check_hf8_height", h1);

  // an ordinary HF8 tx on the new main chain
  MAKE_TX(events, tx_1, miner_acc, alice_acc, MK_TEST_COINS(5), blk_2ar);
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2ar, miner_acc, tx_1);                                      // 33
  DO_CALLBACK_PARAMS(events, "check_hf8_height", h1);
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, MK_TEST_COINS(105)));

  return true;
}

//------------------------------------------------------------------------------

bool hf_booster_rollback_on_invalid_block::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: a block carries a valid booster and a bad tx. The block is rejected, and the valid booster must not
  // leave any trace, no matter how the bad tx fails:
  // A) a booster with a height in the past (before HF8 only a bad signature or key binding is ignored, see C)
  // B) a double spend
  // C) before HF8 a booster with a bad signature is ignored: that block is valid and its valid booster is applied
  // D) after HF8 a booster with a bad signature makes the block invalid. The valid booster next to it is for an id this
  //    build does not know (the only kind that is not active yet), so it would be recorded in the DB only: the chain
  //    crossing its height shows it was not, as a recorded one stops the node there
  HF_BOOSTER_CHAIN_PROLOGUE(blk_1r);                                                                 // 21
  const uint64_t h1 = 35;

  // Alice's txs are built before any of them is pushed, so that her single output can be reused on purpose
  transaction tx_x{}, tx_ds{};
  CHECK_AND_ASSERT_MES(construct_tx_to_key(generator.get_hardforks(), events, tx_x, blk_1r, alice_acc, miner_acc, MK_TEST_COINS(10), TESTS_DEFAULT_FEE, 0), false, "construct_tx_to_key failed");
  CHECK_AND_ASSERT_MES(construct_tx_to_key(generator.get_hardforks(), events, tx_ds, blk_1r, alice_acc, miner_acc, MK_TEST_COINS(10), TESTS_DEFAULT_FEE, 0,
    CURRENCY_TO_KEY_OUT_RELAXED, std::vector<extra_v>(), std::vector<attachment_v>(), false /* check_for_spends */), false, "construct_tx_to_key failed");
  CHECK_AND_ASSERT_MES(get_transaction_hash(tx_x) != get_transaction_hash(tx_ds), false, "tx_x and tx_ds are the same");

  transaction tx_good{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_1r, generator.get_hardforks(), miner_acc, ZANO_HARDFORK_08, h1, tx_good), false, "make_booster_tx failed");

  ADD_CUSTOM_EVENT(events, tx_x);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_x);                                       // 22, Alice's output is spent now

  // B) valid booster + double spend
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_good);
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_ds);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3x, blk_2, miner_acc, std::list<transaction>({ tx_good, tx_ds }));
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_2));
  DO_CALLBACK_PARAMS(events, "check_hf8_height", hf_booster_test_hf8_initial_active_after_height);

  // A) valid booster + booster with a height in the past (built after tx_good was pushed, so it spends another miner output)
  transaction tx_past{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_2, generator.get_hardforks(), miner_acc, ZANO_HARDFORK_08, 10, tx_past), false, "make_booster_tx failed");
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_good);
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_past);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3y, blk_2, miner_acc, std::list<transaction>({ tx_good, tx_past }));
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_2));
  DO_CALLBACK_PARAMS(events, "check_hf8_height", hf_booster_test_hf8_initial_active_after_height);

  // C) valid booster + booster with a bad signature, before HF8: the bad one is ignored, the block is valid
  keypair bogus_keys = keypair::generate();
  booster_tx_options bad_sig_opt{};
  bad_sig_opt.p_signer_sec = &bogus_keys.sec;
  transaction tx_bad{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_2, generator.get_hardforks(), miner_acc, ZANO_HARDFORK_08, 40, tx_bad, bad_sig_opt), false, "make_booster_tx failed");
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_good);
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_bad);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3, blk_2, miner_acc, std::list<transaction>({ tx_good, tx_bad }));  // 23
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_3));
  DO_CALLBACK_PARAMS(events, "check_hf8_height", h1);

  generator.set_hardfork_height(ZANO_HARDFORK_08, h1);
  REWIND_BLOCKS_N(events, blk_3r, blk_3, miner_acc, h1 - get_block_height(blk_3));                   // 24..35
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_08));
  MAKE_NEXT_BLOCK(events, blk_4, blk_3r, miner_acc);                                                 // 36, the first HF8 block
  DO_CALLBACK_PARAMS(events, "check_hf8_height", h1);

  // D) after HF8: valid booster + booster with a bad signature, both for an id this build does not know
  const uint64_t h9 = get_block_height(blk_4) + 1 + hf_booster_test_min_distance_between_boost_and_hf + 5; // 47
  transaction tx_good9{}, tx_bad9{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_4, generator.get_hardforks(), miner_acc, ZANO_HARDFORKS_TOTAL, h9, tx_good9), false, "make_booster_tx failed");
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_good9);
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_4, generator.get_hardforks(), miner_acc, ZANO_HARDFORKS_TOTAL, h9, tx_bad9, bad_sig_opt), false, "make_booster_tx failed");
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_bad9);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_5x, blk_4, miner_acc, std::list<transaction>({ tx_good9, tx_bad9 }));
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_4));

  // had tx_good9 been recorded, the node would refuse the block at h9 + 1
  REWIND_BLOCKS_N(events, blk_4r, blk_4, miner_acc, h9 - get_block_height(blk_4) + 1);              // 37..48
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_4r));

  return true;
}

//------------------------------------------------------------------------------

bool hf_booster_invalid_boosters_rejected::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: how each kind of bad booster is treated.
  // 1) a booster that fails its height checks makes the block invalid, before HF8 as well
  // 2) a foreign signature, a wrong key binding and an id below HF8 fail the signature check: the tx pool rejects
  //    them, and before HF8 a block that carries one is valid, the booster is ignored
  // 3) a correctly signed booster for an id this build does not know is recorded only, the known schedule stays
  // 4) after HF8 every kind of 2), and a booster for an already active hardfork, makes the block invalid
  HF_BOOSTER_CHAIN_PROLOGUE(blk_1r);                                                                 // 21
  const uint64_t next_height = get_block_height(blk_1r) + 1;                                         // 22, where a booster would get in
  const uint64_t too_close_height = next_height + hf_booster_test_min_distance_between_boost_and_hf; // 27, distance is not strictly greater

  // 1) all of these branch off blk_1r and are rejected, so blk_1r stays the top block
  // the height is too close to the booster block
  transaction tx_too_close{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_1r, generator.get_hardforks(), miner_acc, ZANO_HARDFORK_08, too_close_height, tx_too_close), false, "make_booster_tx failed");
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_too_close);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_2c, blk_1r, miner_acc, tx_too_close);
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_1r));
  DO_CALLBACK_PARAMS(events, "check_hf8_height", hf_booster_test_hf8_initial_active_after_height);

  // the height is in the past
  transaction tx_past{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_1r, generator.get_hardforks(), miner_acc, ZANO_HARDFORK_08, 10, tx_past), false, "make_booster_tx failed");
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_past);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_2d, blk_1r, miner_acc, tx_past);
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_1r));
  DO_CALLBACK_PARAMS(events, "check_hf8_height", hf_booster_test_hf8_initial_active_after_height);

  // 2) each one: rejected by the tx pool, then valid in a block as an ignored booster; the blocks are chained
  keypair bogus_keys = keypair::generate();
  booster_tx_options bad_sig_opt{};
  bad_sig_opt.p_signer_sec = &bogus_keys.sec;
  keypair foreign_tx_key = keypair::generate();
  booster_tx_options wrong_key_opt{};
  wrong_key_opt.p_bind_to_pub_key = &foreign_tx_key.pub;

  // a foreign signature
  transaction tx_bad_sig{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_1r, generator.get_hardforks(), alice_acc, ZANO_HARDFORK_08, 40, tx_bad_sig, bad_sig_opt), false, "make_booster_tx failed");
  DO_CALLBACK(events, "mark_invalid_tx");
  ADD_CUSTOM_EVENT(events, tx_bad_sig);
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0));
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_bad_sig);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_bad_sig);                                 // 22
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_2));
  DO_CALLBACK_PARAMS(events, "check_hf8_height", hf_booster_test_hf8_initial_active_after_height);

  // bound to a key that is not the tx key
  transaction tx_wrong_key{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_2, generator.get_hardforks(), miner_acc, ZANO_HARDFORK_08, 40, tx_wrong_key, wrong_key_opt), false, "make_booster_tx failed");
  DO_CALLBACK(events, "mark_invalid_tx");
  ADD_CUSTOM_EVENT(events, tx_wrong_key);
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0));
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_wrong_key);
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_wrong_key);                                // 23
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_3));
  DO_CALLBACK_PARAMS(events, "check_hf8_height", hf_booster_test_hf8_initial_active_after_height);

  // a hardfork id below HF8 (boosters exist from HF8 on)
  transaction tx_low_hf{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_3, generator.get_hardforks(), miner_acc, ZANO_HARDFORK_07, 40, tx_low_hf), false, "make_booster_tx failed");
  DO_CALLBACK(events, "mark_invalid_tx");
  ADD_CUSTOM_EVENT(events, tx_low_hf);
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0));
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_low_hf);
  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_3, miner_acc, tx_low_hf);                                   // 24
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_4));
  DO_CALLBACK_PARAMS(events, "check_hf8_height", hf_booster_test_hf8_initial_active_after_height);

  // 3) an unknown hardfork id, correctly signed: the pool takes it, the block is valid, the known schedule stays.
  //    Far away, because a recorded unknown hardfork stops the node at its activation
  transaction tx_unknown_hf{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_4, generator.get_hardforks(), miner_acc, ZANO_HARDFORKS_TOTAL, hf_booster_test_unknown_hf_far_height, tx_unknown_hf), false, "make_booster_tx failed");
  ADD_CUSTOM_EVENT(events, tx_unknown_hf);
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(1));
  MAKE_NEXT_BLOCK_TX1(events, blk_5, blk_4, miner_acc, tx_unknown_hf);                               // 25
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_5));
  DO_CALLBACK_PARAMS(events, "check_hf8_height", hf_booster_test_hf8_initial_active_after_height);

  // the closest allowed height is accepted
  const uint64_t closest_height = get_block_height(blk_5) + 1 + hf_booster_test_min_distance_between_boost_and_hf + 1; // 32
  transaction tx_closest{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_5, generator.get_hardforks(), miner_acc, ZANO_HARDFORK_08, closest_height, tx_closest), false, "make_booster_tx failed");
  ADD_CUSTOM_EVENT(events, tx_closest);
  MAKE_NEXT_BLOCK_TX1(events, blk_6, blk_5, miner_acc, tx_closest);                                  // 26
  DO_CALLBACK_PARAMS(events, "check_hf8_height", closest_height);

  generator.set_hardfork_height(ZANO_HARDFORK_08, closest_height);
  REWIND_BLOCKS_N(events, blk_6r, blk_6, miner_acc, closest_height - get_block_height(blk_6) + 1);   // 27..33, 33 is the first HF8 block
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_08));

  // 4) after HF8 all of these make the block invalid; all branch off blk_6r
  transaction tx_bad_sig_8{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_6r, generator.get_hardforks(), miner_acc, ZANO_HARDFORKS_TOTAL, hf_booster_test_unknown_hf_far_height, tx_bad_sig_8, bad_sig_opt), false, "make_booster_tx failed");
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_bad_sig_8);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_7a, blk_6r, miner_acc, tx_bad_sig_8);
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_6r));

  transaction tx_wrong_key_8{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_6r, generator.get_hardforks(), miner_acc, ZANO_HARDFORKS_TOTAL, hf_booster_test_unknown_hf_far_height, tx_wrong_key_8, wrong_key_opt), false, "make_booster_tx failed");
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_wrong_key_8);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_7b, blk_6r, miner_acc, tx_wrong_key_8);
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_6r));

  transaction tx_low_hf_8{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_6r, generator.get_hardforks(), miner_acc, ZANO_HARDFORK_07, hf_booster_test_unknown_hf_far_height, tx_low_hf_8), false, "make_booster_tx failed");
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_low_hf_8);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_7c, blk_6r, miner_acc, tx_low_hf_8);
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_6r));

  // an already active hardfork
  transaction tx_active_hf{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_6r, generator.get_hardforks(), miner_acc, ZANO_HARDFORK_08, hf_booster_test_unknown_hf_far_height, tx_active_hf), false, "make_booster_tx failed");
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_active_hf);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_7d, blk_6r, miner_acc, tx_active_hf);
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_6r));
  DO_CALLBACK_PARAMS(events, "check_hf8_height", closest_height);

  return true;
}

//------------------------------------------------------------------------------

hf_booster_unsupported_hf_halts::hf_booster_unsupported_hf_halts()
{
  m_hardforks.last_supported_hardfork_id = ZANO_HARDFORK_07; // knows HF8 (ZANO_HARDFORKS_TOTAL = 9) but is not ready for it
}

bool hf_booster_unsupported_hf_halts::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: a build that is not ready for HF8 still applies a booster for it, so it follows the same schedule as
  // upgraded nodes until the activation. The first HF8 block is refused, and the stop flag names HF8.
  HF_BOOSTER_CHAIN_PROLOGUE(blk_1r);                                                                 // 21
  const uint64_t hf8_height = 30;
  transaction tx_b8{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_1r, generator.get_hardforks(), miner_acc, ZANO_HARDFORK_08, hf8_height, tx_b8), false, "make_booster_tx failed");
  ADD_CUSTOM_EVENT(events, tx_b8);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_b8);                                      // 22
  DO_CALLBACK_PARAMS(events, "check_hf8_height", hf8_height);                                        // the not-ready build schedules it

  REWIND_BLOCKS_N(events, blk_2r, blk_2, miner_acc, hf8_height - get_block_height(blk_2));          // 23..30
  DO_CALLBACK_PARAMS(events, "check_hf8_height", hf8_height);
  DO_CALLBACK_PARAMS(events, "check_unsupported_hardfork_id", static_cast<uint64_t>(0));            // no stop yet
  generator.set_hardfork_height(ZANO_HARDFORK_08, hf8_height);
  MAKE_NEXT_BLOCK(events, blk_3, blk_2r, miner_acc);                                                 // 31, the first HF8 block
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_2r));                           // refused
  DO_CALLBACK_PARAMS(events, "check_unsupported_hardfork_id", static_cast<uint64_t>(ZANO_HARDFORK_08));
  return true;
}

//------------------------------------------------------------------------------

bool hf_booster_unknown_hf_halts::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: a correctly signed booster for an id this build does not know (ZANO_HARDFORKS_TOTAL) is recorded without
  // touching the known schedule, and the first block for which that hardfork is active is refused.
  // Blockchain save/load layout (the core restarts before events 7, 14, ..., 49): the booster block is event 29, the
  // blocks 23..40 are events 32..49, so the last restart happens with the top block at 39. A restart with the top block
  // at 40 would correctly refuse to start (the next block falls under the unknown hardfork), which the harness reports
  // as a failure, so the activation must stay inside the last chunk.
  HF_BOOSTER_CHAIN_PROLOGUE(blk_1r);                                                                 // 21, events 0..27
  const uint64_t unknown_hf_height = 40;
  transaction tx_unknown{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_1r, generator.get_hardforks(), miner_acc, ZANO_HARDFORKS_TOTAL, unknown_hf_height, tx_unknown), false, "make_booster_tx failed");
  ADD_CUSTOM_EVENT(events, tx_unknown);                                                              // 28
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_unknown);                                 // 22, event 29, recorded only
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_2));                            // 30
  DO_CALLBACK_PARAMS(events, "check_hf8_height", hf_booster_test_hf8_initial_active_after_height);   // 31, the known schedule stays
  REWIND_BLOCKS_N(events, blk_2r, blk_2, miner_acc, unknown_hf_height - get_block_height(blk_2));   // 23..40, events 32..49
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_2r));                           // 50
  MAKE_NEXT_BLOCK(events, blk_3, blk_2r, miner_acc);                                                 // 41, event 51: the unknown hardfork is active for it
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_2r));                           // 52, refused
  DO_CALLBACK_PARAMS(events, "check_unsupported_hardfork_id", static_cast<uint64_t>(ZANO_HARDFORKS_TOTAL)); // 53
  return true;
}
