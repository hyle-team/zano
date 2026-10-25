// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "hf_booster_tests.h"
#include "chaingen_helpers.h"
#include "currency_core/bc_hf_booster_service.h"

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
