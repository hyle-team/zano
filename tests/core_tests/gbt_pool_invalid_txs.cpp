// Copyright (c) 2014-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "gbt_pool_invalid_txs.h"
#include "wallet_test_core_proxy.h"

using namespace epee;
using namespace crypto;
using namespace currency;

gbt_pool_invalid_txs_alias_collision::gbt_pool_invalid_txs_alias_collision()
{
  REGISTER_CALLBACK_METHOD(gbt_pool_invalid_txs_alias_collision, c1);
}

bool gbt_pool_invalid_txs_alias_collision::generate(std::vector<test_event_entry>& events) const
{

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

bool gbt_pool_invalid_txs_alias_collision::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);

  size_t blocks_fetched = 0;
  bool received_money = false;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);

  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool before the test: " << c.get_pool_transactions_count());

  // create two alias txs that conflict with each other
  const std::string alias_name(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, 'a');
  const uint64_t alias_reward = get_alias_coast_from_fee(alias_name, c.get_blockchain_storage().get_tx_fee_median());

  extra_alias_entry ai_miner = AUTO_VAL_INIT(ai_miner);
  ai_miner.m_alias = alias_name;
  ai_miner.m_address = m_accounts[MINER_ACC_IDX].get_public_address();
  transaction alias_tx_1 = AUTO_VAL_INIT(alias_tx_1);
  bool ok = true;
  try
  {
    miner_wlt->request_alias_registration(ai_miner, alias_tx_1, TESTS_DEFAULT_FEE, alias_reward);
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("First alias registration threw unexpectedly: " << e.what());
    ok = false;
  }
  CHECK_AND_ASSERT_MES(ok, false, "First alias registration failed to enter the pool");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "after first alias registration pool size is " << c.get_pool_transactions_count() << ", expected 1");

  //second reg - the same alias, different owner + disable validation
  c.get_tx_pool().unsecure_disable_tx_validation_on_addition(true);

  extra_alias_entry ai_alice = AUTO_VAL_INIT(ai_alice);
  ai_alice.m_alias = alias_name;
  ai_alice.m_address = m_accounts[ALICE_ACC_IDX].get_public_address();
  transaction alias_tx_2 = AUTO_VAL_INIT(alias_tx_2);
  try
  {
    miner_wlt->request_alias_registration(ai_alice, alias_tx_2, TESTS_DEFAULT_FEE, alias_reward);
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("Second (conflicting) alias registration threw even with pool validation disabled: " << e.what());
    c.get_tx_pool().unsecure_disable_tx_validation_on_addition(false);
    return false;
  }

  c.get_tx_pool().unsecure_disable_tx_validation_on_addition(false);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 2, false, "after the second (conflicting) alias registration pool size is " << c.get_pool_transactions_count() << ", expected 2");

  LOG_PRINT_MAGENTA("pool prepared with two conflicting alias registrations:"
    << ENDL << "  tx1 (" << get_transaction_hash(alias_tx_1) << "): alias \"" << alias_name << "\" -> miner"
    << ENDL << "  tx2 (" << get_transaction_hash(alias_tx_2) << "): alias \"" << alias_name << "\" -> alice", LOG_LEVEL_0);

  const uint64_t height_before = c.get_current_blockchain_size();
  // mint -> getblocktemplate
  bool mint_res = alice_wlt->try_mint_pos();

  const uint64_t height_after = c.get_current_blockchain_size();
  const size_t pool_txs_after = c.get_pool_transactions_count();

  LOG_PRINT_MAGENTA("after try_mint_pos: result=" << (mint_res ? "true" : "false")
    << ", height " << height_before << " -> " << height_after
    << ", pool txs=" << pool_txs_after, LOG_LEVEL_0);

  // observe blacklist state after the failed stake attempt
  const crypto::hash h1 = get_transaction_hash(alias_tx_1);
  const crypto::hash h2 = get_transaction_hash(alias_tx_2);
  const bool bl1 = c.get_tx_pool().is_tx_blacklisted(h1);
  const bool bl2 = c.get_tx_pool().is_tx_blacklisted(h2);
  LOG_PRINT_MAGENTA("blacklist state after first mint attempt:"
    << ENDL << "  tx1 (" << h1 << "): " << (bl1 ? "BLACKLISTED" : "normal")
    << ENDL << "  tx2 (" << h2 << "): " << (bl2 ? "BLACKLISTED" : "normal"), LOG_LEVEL_0);

  CHECK_AND_ASSERT_MES(bl1 != bl2, false, "expected exactly one of the two conflicting alias txs to be blacklisted after the failed stake, got bl1=" << bl1 << " bl2=" << bl2);

  const crypto::hash blacklisted_hash = bl1 ? h1 : h2;
  const crypto::hash surviving_hash = bl1 ? h2 : h1;

  // blacklisted tx is not removed from the pool - just marked as ignored by fill_block_template
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 2, false, "blacklisted tx should remain in the pool, pool size now: " << c.get_pool_transactions_count());

  // second getblocktemplate - must skip the blacklisted tx and return a valid template
  create_block_template_params params2 = AUTO_VAL_INIT(params2);
  params2.miner_address = m_accounts[MINER_ACC_IDX].get_public_address();
  params2.stakeholder_address = m_accounts[MINER_ACC_IDX].get_public_address();
  params2.pos = false;
  create_block_template_response resp2 = AUTO_VAL_INIT(resp2);
  bool gbt_ok = c.get_block_template(params2, resp2);
  CHECK_AND_ASSERT_MES(gbt_ok, false, "second get_block_template call failed");

  LOG_PRINT_MAGENTA("second getblocktemplate returned " << resp2.b.tx_hashes.size() << " tx(es) in the template", LOG_LEVEL_0);
  for (const auto& h : resp2.b.tx_hashes)
    LOG_PRINT_MAGENTA("  " << h, LOG_LEVEL_0);

  const bool contains_blacklisted = std::find(resp2.b.tx_hashes.begin(), resp2.b.tx_hashes.end(), blacklisted_hash) != resp2.b.tx_hashes.end();
  const bool contains_surviving = std::find(resp2.b.tx_hashes.begin(), resp2.b.tx_hashes.end(), surviving_hash) != resp2.b.tx_hashes.end();

  CHECK_AND_ASSERT_MES(!contains_blacklisted, false, "second getblocktemplate still includes the blacklisted tx " << blacklisted_hash << " - fill_block_template is supposed to skip it");
  CHECK_AND_ASSERT_MES(contains_surviving, false, "second getblocktemplate dropped the surviving alias tx " << surviving_hash << " - expected it to remain in the template");

  // expected postfix behavior - block is staked successfully
  CHECK_AND_ASSERT_MES(mint_res, false, "try_mint_pos returned false: wallet could not stake a block because getblocktemplate returned a template that the core refused to accept");
  CHECK_AND_ASSERT_MES(height_after == height_before + 1, false, "blockchain height did not advance: " << height_before << " -> " << height_after);
  // the pool must be cleaned of the tx that caused the conflict
  CHECK_AND_ASSERT_MES(pool_txs_after < 2, false, "after a successful stake attempt the pool still contains both conflicting " "alias txs (" << pool_txs_after << ")");

  return true;
}
