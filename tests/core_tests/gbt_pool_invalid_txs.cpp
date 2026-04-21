// Copyright (c) 2014-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "gbt_pool_invalid_txs.h"
#include "wallet_test_core_proxy.h"
#include "wallet/wallet_debug_events_definitions.h"

using namespace epee;
using namespace crypto;
using namespace currency;

gbt_pool_invalid_txs_asset_overemit::gbt_pool_invalid_txs_asset_overemit()
{
  REGISTER_CALLBACK_METHOD(gbt_pool_invalid_txs_asset_overemit, c1);
}

bool gbt_pool_invalid_txs_asset_overemit::generate(std::vector<test_event_entry>& events) const
{
  // some stuff for staking and other, just funding
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);

  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, miner_acc, test_core_time::get_time());
  events.push_back(blk_0);

  DO_CALLBACK(events, "configure_core");

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  MAKE_TX_FEE(events, tx_0, miner_acc, alice_acc, MK_TEST_COINS(2000), TESTS_DEFAULT_FEE, blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

bool gbt_pool_invalid_txs_asset_overemit::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);

  size_t blocks_fetched = 0;
  bool received_money = false;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);

  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "unexpected number of txs in the pool before the test: " << c.get_pool_transactions_count());

  // supply=90 cap=100 emit=6
  const uint64_t unit = 1000000000000ULL;
  const size_t N_EMITS = 5;
  const uint64_t initial_supply = 90 * unit;
  const uint64_t total_max_supply = 100 * unit;
  const uint64_t emit_amount = 6 * unit;

  asset_descriptor_base adb = AUTO_VAL_INIT(adb);
  adb.total_max_supply = total_max_supply;
  adb.full_name = "GetBlockTemplate test coin";
  adb.ticker = "GBT";
  adb.decimal_point = 12;

  // emits produce new gbt
  std::vector<currency::tx_destination_entry> deploy_dst(1);
  deploy_dst[0].addr.push_back(miner_wlt->get_account().get_public_address());
  deploy_dst[0].amount = initial_supply;
  deploy_dst[0].asset_id = currency::null_pkey;

  currency::transaction deploy_tx = AUTO_VAL_INIT(deploy_tx);
  crypto::public_key asset_id = currency::null_pkey;
  try
  {
    miner_wlt->deploy_new_asset(adb, deploy_dst, deploy_tx, asset_id);
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("deploy_new_asset failed: " << e.what());
    return false;
  }
  LOG_PRINT_MAGENTA("deployed asset " << asset_id << " via tx " << get_transaction_hash(deploy_tx)
    << " initial_supply=" << initial_supply << ", total_max_supply=" << total_max_supply
    << ", headroom=" << (total_max_supply - initial_supply), LOG_LEVEL_0);

  // confirm the deploy and let the new gbt source outputs mature
  bool r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed for deploy");

  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "pool should be empty after the deploy tx is confirmed, got " << c.get_pool_transactions_count());

  std::vector<currency::tx_destination_entry> emit_dst(1);
  emit_dst[0].addr.push_back(miner_wlt->get_account().get_public_address());
  emit_dst[0].amount = emit_amount;
  emit_dst[0].asset_id = currency::null_pkey;

  std::vector<crypto::hash> bad_tx_hashes;
  bad_tx_hashes.reserve(N_EMITS);

  c.get_tx_pool().unsecure_disable_tx_validation_on_addition(true);
  for (size_t i = 0; i < N_EMITS; ++i)
  {
    currency::transaction emit_tx = AUTO_VAL_INIT(emit_tx);
    try
    {
      miner_wlt->emit_asset(asset_id, emit_dst, emit_tx);
    }
    catch (const std::exception& e)
    {
      LOG_ERROR("emit #" << i << " failed: " << e.what());
      c.get_tx_pool().unsecure_disable_tx_validation_on_addition(false);
      return false;
    }
    bad_tx_hashes.push_back(get_transaction_hash(emit_tx));
  }
  c.get_tx_pool().unsecure_disable_tx_validation_on_addition(false);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == N_EMITS, false, "after injecting emit txs pool size is " << c.get_pool_transactions_count() << ", expected " << N_EMITS);

  LOG_PRINT_MAGENTA("pool prepared with " << N_EMITS << " legal but conflicting emit txs"
    << " (each emits " << emit_amount << ", only one fits the headroom):", LOG_LEVEL_0);
  for (const auto& h : bad_tx_hashes)
    LOG_PRINT_MAGENTA("  " << h, LOG_LEVEL_0);

  // try mint loop
  const uint64_t height_start = c.get_current_blockchain_size();
  const size_t pool_start = c.get_pool_transactions_count();
  const size_t MAX_ROUNDS = 1;//N_EMITS + 2;
  bool staked = false;

  size_t max_bad_in_template = 0;
  bool saw_dirty_template_fail = false;
  const std::unordered_set<crypto::hash> bad_tx_set(bad_tx_hashes.begin(), bad_tx_hashes.end());


  // call gbt directly, me need pick hash for checks
  currency::block tmpl_block = AUTO_VAL_INIT(tmpl_block);
  wide_difficulty_type tmpl_diff = 0;
  uint64_t tmpl_height = 0;
  bool tmpl_ok = c.get_block_template(tmpl_block,
    alice_wlt->get_account().get_public_address(),
    alice_wlt->get_account().get_public_address(),
    tmpl_diff, tmpl_height, blobdata());
  CHECK_AND_ASSERT_MES(tmpl_ok, false, "get_block_template failed");

  size_t bad_in_template = 0;
  for (const auto& h : tmpl_block.tx_hashes)
    if (bad_tx_set.count(h))
      ++bad_in_template;
  if (bad_in_template > max_bad_in_template)
    max_bad_in_template = bad_in_template;

  LOG_PRINT_MAGENTA("GBT template has " << tmpl_block.tx_hashes.size()
    << " txs, " << bad_in_template << " of them are our legal but conflicting emits", LOG_LEVEL_0);
  for (const auto& h : tmpl_block.tx_hashes)
    LOG_PRINT_MAGENTA("  " << (bad_tx_set.count(h) ? "[BAD] " : " ") << h, LOG_LEVEL_0);

  // try to stake, if the template is dirty mint must fail
  const uint64_t h_before = c.get_current_blockchain_size();
  bool mint_res = alice_wlt->try_mint_pos();
  const uint64_t h_after = c.get_current_blockchain_size();

  size_t blacklisted_now = 0;
  for (const auto& h : bad_tx_hashes)
    if (c.get_tx_pool().is_tx_blacklisted(h)) ++blacklisted_now;

  LOG_PRINT_MAGENTA("try_mint_pos=" << (mint_res ? "OK" : "FAIL")
    << ", height " << h_before << " -> " << h_after
    << ", blacklisted " << blacklisted_now << "/" << N_EMITS
    << ", pool=" << c.get_pool_transactions_count(), LOG_LEVEL_0);


  CHECK_AND_ASSERT_MES(blacklisted_now == 4, false, "missmatch number of transactions");

  bool mint_res2 = alice_wlt->try_mint_pos();
  CHECK_AND_ASSERT_MES(mint_res2, false, "unexpected fail of try_mint_pos");
  
  const uint64_t h_after2 = c.get_current_blockchain_size();
  CHECK_AND_ASSERT_MES(h_after2 == h_before + 1, false, "unexpected h");

  LOG_PRINT_MAGENTA("try_mint_pos2=" << (mint_res2 ? "OK" : "FAIL")
    << ", height " << h_before << " -> " << h_after
    << ", blacklisted " << blacklisted_now << "/" << N_EMITS
    << ", pool=" << c.get_pool_transactions_count(), LOG_LEVEL_0);

  //make sure next transactions are working fine, and the bad ones are not coming back
  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  
  transaction res_tx_1 = AUTO_VAL_INIT(res_tx_1);
  transaction res_tx_2 = AUTO_VAL_INIT(res_tx_2);

  try {
    
    miner_wlt->transfer(COIN / 100, alice_wlt->get_account().get_public_address(), res_tx_1);
    miner_wlt->transfer(COIN / 100, alice_wlt->get_account().get_public_address(), res_tx_2);
  }
  catch (...)
  {
    CHECK_AND_ASSERT_MES(false, false, "unexpected fail of transfer after minting with bad template");
  }
  CHECK_AND_ASSERT_MES(c.get_tx_pool().have_tx(get_transaction_hash(res_tx_1)), false, "unable to find tx in pool");
  CHECK_AND_ASSERT_MES(c.get_tx_pool().have_tx(get_transaction_hash(res_tx_2)), false, "unable to find tx in pool");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 6, false, "unexpected number of txs in the pool before the test: " << c.get_pool_transactions_count());

  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 1);
  CHECK_AND_ASSERT_MES(!c.get_tx_pool().have_tx(get_transaction_hash(res_tx_1)), false, "unable to find tx in pool");
  CHECK_AND_ASSERT_MES(!c.get_tx_pool().have_tx(get_transaction_hash(res_tx_2)), false, "unable to find tx in pool");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 4, false, "unexpected number of txs in the pool before the test: " << c.get_pool_transactions_count());


  return true;
}
