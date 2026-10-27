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
}

bool hf_booster_hf8_activation_basic::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  CHECK_AND_ASSERT_MES(test_chain_unit_enchanced::configure_core(c, ev_index, events), false, "default configure_core failed");
  core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.maintainers_public_keys[ZANO_HARDFORK_08] = m_maintainer_keys.pub;  
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

  m_hardforks.clear();
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_07, 1);
  m_hardforks.set_hardfork_height(ZANO_HARDFORK_08, hf_booster_test_hf8_initial_active_after_height);
}

core_runtime_config hf_booster_test_base::get_runtime_info_for_core() const
{
  core_runtime_config crc = test_chain_unit_base::get_runtime_info_for_core();
  crc.maintainers_public_keys[ZANO_HARDFORK_08] = m_maintainer_keys.pub;
  crc.minimal_distance_between_boost_and_hf = hf_booster_test_min_distance_between_boost_and_hf;
  crc.pre_hardfork_tx_freeze_period = 0;
  return crc;
}

bool hf_booster_test_base::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  CHECK_AND_ASSERT_MES(test_chain_unit_enchanced::configure_core(c, ev_index, events), false, "default configure_core failed");
  core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.maintainers_public_keys[ZANO_HARDFORK_08] = m_maintainer_keys.pub;
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
  // Test idea: a block carries a valid booster followed by a bad tx. The block is rejected, and the booster
  // must not leave any trace in the schedule, no matter how the bad tx fails:
  // A) a booster with a bad maintainer signature
  // B) a double spend
  // Afterwards the same valid booster alone is accepted and HF8 activates at its height.
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

  // A) valid booster + booster with a bad signature (built after tx_good was pushed, so it spends another miner output)
  keypair bogus_keys = keypair::generate();
  booster_tx_options bad_sig_opt{};
  bad_sig_opt.p_signer_sec = &bogus_keys.sec;
  transaction tx_bad{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_2, generator.get_hardforks(), miner_acc, ZANO_HARDFORK_08, 40, tx_bad, bad_sig_opt), false, "make_booster_tx failed");
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_good);
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_bad);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3y, blk_2, miner_acc, std::list<transaction>({ tx_good, tx_bad }));
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_2));
  DO_CALLBACK_PARAMS(events, "check_hf8_height", hf_booster_test_hf8_initial_active_after_height);

  // the valid booster alone is fine
  ADD_CUSTOM_EVENT(events, tx_good);
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_good);                                     // 23
  DO_CALLBACK_PARAMS(events, "check_hf8_height", h1);

  generator.set_hardfork_height(ZANO_HARDFORK_08, h1);
  REWIND_BLOCKS_N(events, blk_3r, blk_3, miner_acc, h1 - get_block_height(blk_3));                   // 24..35
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_08));
  MAKE_NEXT_BLOCK(events, blk_4, blk_3r, miner_acc);                                                 // 36, the first HF8 block
  DO_CALLBACK_PARAMS(events, "check_hf8_height", h1);

  return true;
}

//------------------------------------------------------------------------------

bool hf_booster_runtime_config_reset_keeps_boost::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: the core runtime config gets set again after a booster was applied (the daemon and the tests
  // do that), the boosted schedule has to survive it, before and after the activation.
  HF_BOOSTER_CHAIN_PROLOGUE(blk_1r);                                                                 // 21
  const uint64_t h1 = 35;

  transaction tx_b1{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_1r, generator.get_hardforks(), miner_acc, ZANO_HARDFORK_08, h1, tx_b1), false, "make_booster_tx failed");
  ADD_CUSTOM_EVENT(events, tx_b1);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_b1);                                      // 22
  DO_CALLBACK_PARAMS(events, "check_hf8_height", h1);

  DO_CALLBACK(events, "configure_core");
  DO_CALLBACK_PARAMS(events, "check_hf8_height", h1);

  generator.set_hardfork_height(ZANO_HARDFORK_08, h1);
  REWIND_BLOCKS_N(events, blk_2r, blk_2, miner_acc, h1 - get_block_height(blk_2) + 1);               // 23..36, 36 is the first HF8 block
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_08));

  DO_CALLBACK(events, "configure_core");
  DO_CALLBACK_PARAMS(events, "check_hf8_height", h1);
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_08));
  MAKE_NEXT_BLOCK(events, blk_3, blk_2r, miner_acc);                                                 // 37, HF8 block
  DO_CALLBACK_PARAMS(events, "check_hf8_height", h1);

  return true;
}

//------------------------------------------------------------------------------

bool hf_booster_invalid_boosters_rejected::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: boosters that must be rejected. A block carrying one is invalid and leaves the schedule untouched.
  // The one with a foreign signature must be rejected by the tx pool as well.
  HF_BOOSTER_CHAIN_PROLOGUE(blk_1r);                                                                 // 21
  const uint64_t next_height = get_block_height(blk_1r) + 1;                                         // 22, where a booster would get in
  const uint64_t too_close_height = next_height + hf_booster_test_min_distance_between_boost_and_hf; // 27, distance is not strictly greater
  const uint64_t closest_height = too_close_height + 1;                                              // 28

  // a foreign signature
  keypair bogus_keys = keypair::generate();
  booster_tx_options bad_sig_opt{};
  bad_sig_opt.p_signer_sec = &bogus_keys.sec;
  transaction tx_bad_sig{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_1r, generator.get_hardforks(), alice_acc, ZANO_HARDFORK_08, 40, tx_bad_sig, bad_sig_opt), false, "make_booster_tx failed");
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_bad_sig);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_2a, blk_1r, miner_acc, tx_bad_sig);
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_1r));
  DO_CALLBACK_PARAMS(events, "check_hf8_height", hf_booster_test_hf8_initial_active_after_height);

  // bound to a key that is not the tx key
  keypair foreign_tx_key = keypair::generate();
  booster_tx_options wrong_key_opt{};
  wrong_key_opt.p_bind_to_pub_key = &foreign_tx_key.pub;
  transaction tx_wrong_key{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_1r, generator.get_hardforks(), miner_acc, ZANO_HARDFORK_08, 40, tx_wrong_key, wrong_key_opt), false, "make_booster_tx failed");
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_wrong_key);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_2b, blk_1r, miner_acc, tx_wrong_key);
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_1r));
  DO_CALLBACK_PARAMS(events, "check_hf8_height", hf_booster_test_hf8_initial_active_after_height);

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

  // unknown hardfork id
  transaction tx_unknown_hf{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_1r, generator.get_hardforks(), miner_acc, ZANO_HARDFORKS_TOTAL, 40, tx_unknown_hf), false, "make_booster_tx failed");
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_unknown_hf);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_2e, blk_1r, miner_acc, tx_unknown_hf);
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_1r));
  DO_CALLBACK_PARAMS(events, "check_hf8_height", hf_booster_test_hf8_initial_active_after_height);

  // an already active hardfork
  transaction tx_active_hf{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_1r, generator.get_hardforks(), miner_acc, ZANO_HARDFORK_07, 40, tx_active_hf), false, "make_booster_tx failed");
  HF_BOOSTER_ADD_TX_AS_KEPT_BY_BLOCK(tx_active_hf);
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_2f, blk_1r, miner_acc, tx_active_hf);
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(blk_1r));
  DO_CALLBACK_PARAMS(events, "check_hf8_height", hf_booster_test_hf8_initial_active_after_height);

  // the tx pool must not accept boosters that can never be valid, otherwise every block template would carry one
  DO_CALLBACK(events, "mark_invalid_tx");
  ADD_CUSTOM_EVENT(events, tx_bad_sig);
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0));
  DO_CALLBACK(events, "mark_invalid_tx");
  ADD_CUSTOM_EVENT(events, tx_wrong_key);
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(0));

  // the closest allowed height is accepted
  transaction tx_closest{};
  CHECK_AND_ASSERT_MES(make_booster_tx(events, blk_1r, generator.get_hardforks(), miner_acc, ZANO_HARDFORK_08, closest_height, tx_closest), false, "make_booster_tx failed");
  ADD_CUSTOM_EVENT(events, tx_closest);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_closest);                                 // 22
  DO_CALLBACK_PARAMS(events, "check_hf8_height", closest_height);

  generator.set_hardfork_height(ZANO_HARDFORK_08, closest_height);
  REWIND_BLOCKS_N(events, blk_2r, blk_2, miner_acc, closest_height - get_block_height(blk_2) + 1);   // 23..29, 29 is the first HF8 block
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_08));

  return true;
}
