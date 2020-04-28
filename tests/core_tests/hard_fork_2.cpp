// Copyright (c) 2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "hard_fork_2.h"
#include "../../src/wallet/wallet_rpc_server.h"
//#include "pos_block_builder.h"
//#include "tx_builder.h"
//#include "random_helper.h"

using namespace currency;

//------------------------------------------------------------------------------

hard_fork_2_base_test::hard_fork_2_base_test(size_t hardfork_height)
  : m_hardfork_height(hardfork_height)
{
  REGISTER_CALLBACK_METHOD(hard_fork_2_base_test, configure_core);
}

bool hard_fork_2_base_test::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE;
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH;
  pc.hard_fork_01_starts_after_height = m_hardfork_height;
  pc.hard_fork_02_starts_after_height = m_hardfork_height;
  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

//------------------------------------------------------------------------------

hard_fork_2_tx_payer_in_wallet::hard_fork_2_tx_payer_in_wallet()
  : hard_fork_2_base_test(16)
{
  REGISTER_CALLBACK_METHOD(hard_fork_2_tx_payer_in_wallet, c1);
}

bool hard_fork_2_tx_payer_in_wallet::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure that wallet uses tx_payer_old only before HF2 and tx_payer after

  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  generator.set_hardfork_height(1, m_hardfork_height);
  generator.set_hardfork_height(2, m_hardfork_height);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  DO_CALLBACK(events, "c1");

  // REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);


  return true;
}

bool hard_fork_2_tx_payer_in_wallet::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false, stub_bool = false;
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, m_accounts[MINER_ACC_IDX]);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);

  miner_wlt->refresh();

  // wallet RPC server
  tools::wallet_rpc_server miner_wlt_rpc(*miner_wlt);
  epee::json_rpc::error je;
  tools::wallet_rpc_server::connection_context ctx;

  tools::wallet_public::COMMAND_RPC_TRANSFER::request req = AUTO_VAL_INIT(req);
  tools::wallet_public::transfer_destination td{ MK_TEST_COINS(1), m_accounts[ALICE_ACC_IDX].get_public_address_str() };
  req.destinations.push_back(td);
  req.fee = TESTS_DEFAULT_FEE;
  req.push_payer = true;

  tools::wallet_public::COMMAND_RPC_TRANSFER::response res = AUTO_VAL_INIT(res);
  
  r = miner_wlt_rpc.on_transfer(req, res, je, ctx);
  CHECK_AND_ASSERT_MES(r, false, "on_transfer failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  crypto::hash tx_hash = null_hash;
  CHECK_AND_ASSERT_MES(epee::string_tools::hex_to_pod(res.tx_hash, tx_hash), false, "");
  
  transaction tx = AUTO_VAL_INIT(tx);
  CHECK_AND_ASSERT_MES(c.get_transaction(tx_hash, tx), false, "");

  r = have_type_in_variant_container<tx_payer_old>(tx.extra);
  CHECK_AND_ASSERT_MES(r, false, "tx_payer_old is not found in extra");

  r = !have_type_in_variant_container<tx_payer>(tx.extra);
  CHECK_AND_ASSERT_MES(r, false, "tx_payer is found in extra");

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  std::shared_ptr<wlt_lambda_on_transfer2_wrapper> l(new wlt_lambda_on_transfer2_wrapper(
    [&](const tools::wallet_public::wallet_transfer_info& wti, uint64_t balance, uint64_t unlocked_balance, uint64_t total_mined) -> bool {
      CHECK_AND_ASSERT_MES(wti.show_sender, false, "show_sender is false");
      CHECK_AND_ASSERT_MES(wti.remote_addresses.size() == 1, false, "incorrect wti.remote_addresses.size() = " << wti.remote_addresses.size());
      CHECK_AND_ASSERT_MES(wti.remote_addresses.front() == m_accounts[MINER_ACC_IDX].get_public_address_str(), false, "wti.remote_addresses.front is incorrect");
      return true;
    }
  ));
  alice_wlt->callback(l);

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(1)), false, "");

  // mine blocks 15, 16, 17 to activate HF2
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 3);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  miner_wlt->refresh();
  alice_wlt->refresh();

  // check again
  req.destinations.front().amount = MK_TEST_COINS(2);
  r = miner_wlt_rpc.on_transfer(req, res, je, ctx);
  CHECK_AND_ASSERT_MES(r, false, "on_transfer failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  tx_hash = null_hash;
  CHECK_AND_ASSERT_MES(epee::string_tools::hex_to_pod(res.tx_hash, tx_hash), false, "");
  CHECK_AND_ASSERT_MES(c.get_transaction(tx_hash, tx), false, "");

  r = have_type_in_variant_container<tx_payer>(tx.extra);
  CHECK_AND_ASSERT_MES(r, false, "tx_payer is not found in extra");
  r = !have_type_in_variant_container<tx_payer_old>(tx.extra);
  CHECK_AND_ASSERT_MES(r, false, "tx_payer_old is found in extra");

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  std::shared_ptr<wlt_lambda_on_transfer2_wrapper> l2(new wlt_lambda_on_transfer2_wrapper(
    [&](const tools::wallet_public::wallet_transfer_info& wti, uint64_t balance, uint64_t unlocked_balance, uint64_t total_mined) -> bool {
      CHECK_AND_ASSERT_MES(wti.amount == MK_TEST_COINS(2), false, "incorrect wti.amount = " << print_money_brief(wti.amount));
      CHECK_AND_ASSERT_MES(wti.show_sender, false, "show_sender is false");
      CHECK_AND_ASSERT_MES(wti.remote_addresses.size() == 1, false, "incorrect wti.remote_addresses.size() = " << wti.remote_addresses.size());
      CHECK_AND_ASSERT_MES(wti.remote_addresses.front() == m_accounts[MINER_ACC_IDX].get_public_address_str(), false, "wti.remote_addresses.front is incorrect");
      return true;
    }
  ));
  alice_wlt->callback(l2);

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(3)), false, "");



  return true;
}
