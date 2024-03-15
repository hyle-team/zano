// Copyright (c) 2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "hard_fork_2.h"
#include "../../src/wallet/wallet_rpc_server.h"

using namespace currency;

//------------------------------------------------------------------------------

hard_fork_2_base_test::hard_fork_2_base_test(size_t hardfork_02_height)
  : hard_fork_2_base_test(1, hardfork_02_height)
{
}

hard_fork_2_base_test::hard_fork_2_base_test(size_t hardfork_01_height, size_t hardfork_02_height)
  : m_hardfork_01_height(hardfork_01_height)
  , m_hardfork_02_height(hardfork_02_height)
  , m_hardfork_03_height(CURRENCY_MAX_BLOCK_NUMBER)
{
  m_hardforks.clear();
  m_hardforks.set_hardfork_height(1, m_hardfork_01_height);
  m_hardforks.set_hardfork_height(2, m_hardfork_02_height);
  m_hardforks.set_hardfork_height(3, m_hardfork_03_height);
  REGISTER_CALLBACK_METHOD(hard_fork_2_base_test, configure_core);
}

bool hard_fork_2_base_test::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE;
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH;
  pc.hard_forks = m_hardforks;
  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

//------------------------------------------------------------------------------

hard_fork_2_tx_payer_in_wallet::hard_fork_2_tx_payer_in_wallet()
  : hard_fork_2_base_test(24)
{
  REGISTER_CALLBACK_METHOD(hard_fork_2_tx_payer_in_wallet, c1);
}

bool hard_fork_2_tx_payer_in_wallet::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure that wallet uses tx_payer_old only before HF2 and tx_payer after

  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate(true); // Bob has auditable address

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  MAKE_TX(events, tx_0, miner_acc, bob_acc, MK_TEST_COINS(12), blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK(events, "c1");

  return true;
}

bool hard_fork_2_tx_payer_in_wallet::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, m_accounts[MINER_ACC_IDX]);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  std::shared_ptr<tools::wallet2> bob_wlt   = init_playtime_test_wallet(events, c, m_accounts[BOB_ACC_IDX]);

  miner_wlt->refresh();
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, 0), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, MK_TEST_COINS(12)), false, "");

  // wallet RPC server
  tools::wallet_rpc_server miner_wlt_rpc(miner_wlt);
  epee::json_rpc::error je;
  tools::wallet_rpc_server::connection_context ctx;

  // Before HF2: Miner -> Alice (normal address) with payer info
  tools::wallet_public::COMMAND_RPC_TRANSFER::request req_a = AUTO_VAL_INIT(req_a);
  req_a.destinations.push_back(tools::wallet_public::transfer_destination{ MK_TEST_COINS(1), m_accounts[ALICE_ACC_IDX].get_public_address_str() });
  req_a.fee = TESTS_DEFAULT_FEE;
  req_a.push_payer = true;

  tools::wallet_public::COMMAND_RPC_TRANSFER::response res = AUTO_VAL_INIT(res);
  
  r = miner_wlt_rpc.on_transfer(req_a, res, je, ctx);
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


  // Before HF2: Miner -> Bob (auditable address) with payer info
  tools::wallet_public::COMMAND_RPC_TRANSFER::request req_b = AUTO_VAL_INIT(req_b);
  req_b.destinations.push_back(tools::wallet_public::transfer_destination{ MK_TEST_COINS(1), m_accounts[BOB_ACC_IDX].get_public_address_str() });
  req_b.fee = TESTS_DEFAULT_FEE;
  req_b.push_payer = true;

  res = AUTO_VAL_INIT(res);
  
  r = miner_wlt_rpc.on_transfer(req_b, res, je, ctx);
  CHECK_AND_ASSERT_MES(r, false, "on_transfer failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 2, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  tx_hash = null_hash;
  CHECK_AND_ASSERT_MES(epee::string_tools::hex_to_pod(res.tx_hash, tx_hash), false, "");
  
  tx = AUTO_VAL_INIT(tx);
  CHECK_AND_ASSERT_MES(c.get_transaction(tx_hash, tx), false, "");

  r = have_type_in_variant_container<tx_payer_old>(tx.extra);
  CHECK_AND_ASSERT_MES(r, false, "tx_payer_old is not found in extra");
  r = !have_type_in_variant_container<tx_payer>(tx.extra);
  CHECK_AND_ASSERT_MES(r, false, "tx_payer is found in extra");


  // mine a block and confirm both transactions
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  size_t callback_counter = 0;
  std::shared_ptr<wlt_lambda_on_transfer2_wrapper> l(new wlt_lambda_on_transfer2_wrapper(
    [&](const tools::wallet_public::wallet_transfer_info& wti, const std::list<tools::wallet_public::asset_balance_entry>& balances, uint64_t total_mined) -> bool {
      CHECK_AND_ASSERT_THROW_MES(wti.show_sender, "show_sender is false");
      CHECK_AND_ASSERT_THROW_MES(wti.remote_addresses.size() == 1, "incorrect wti.remote_addresses.size() = " << wti.remote_addresses.size());
      CHECK_AND_ASSERT_THROW_MES(wti.remote_addresses.front() == m_accounts[MINER_ACC_IDX].get_public_address_str(), "wti.remote_addresses.front is incorrect");
      ++callback_counter;
      return true;
    }
  ));
  alice_wlt->callback(l);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(1)), false, "");
  CHECK_AND_ASSERT_MES(callback_counter == 1, false, "callback_counter = " << callback_counter);

  bob_wlt->callback(l); // same callback -- same changes
  callback_counter = 0;
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, MK_TEST_COINS(13)), false, "");
  CHECK_AND_ASSERT_MES(callback_counter == 1, false, "callback_counter = " << callback_counter);

  alice_wlt->callback(std::make_shared<tools::i_wallet2_callback>()); // clear callback
  bob_wlt->callback(std::make_shared<tools::i_wallet2_callback>()); // clear callback

  // Before HF2: Bob (auditable address) -> Alice with payer info requested (should NOT put tx_payer or tx_payer_old)
  tools::wallet_rpc_server bob_wlt_rpc(bob_wlt);
  tools::wallet_public::COMMAND_RPC_TRANSFER::request req_c = AUTO_VAL_INIT(req_c);
  req_c.destinations.push_back(tools::wallet_public::transfer_destination{ MK_TEST_COINS(1), m_accounts[ALICE_ACC_IDX].get_public_address_str() });
  req_c.fee = TESTS_DEFAULT_FEE;
  req_c.push_payer = true;
  res = AUTO_VAL_INIT(res);
  r = bob_wlt_rpc.on_transfer(req_c, res, je, ctx);
  CHECK_AND_ASSERT_MES(r, false, "on_transfer failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  tx_hash = null_hash;
  CHECK_AND_ASSERT_MES(epee::string_tools::hex_to_pod(res.tx_hash, tx_hash), false, "");
  tx = AUTO_VAL_INIT(tx);
  CHECK_AND_ASSERT_MES(c.get_transaction(tx_hash, tx), false, "");
  r = !have_type_in_variant_container<tx_payer_old>(tx.extra);
  CHECK_AND_ASSERT_MES(r, false, "tx_payer_old is found in extra");
  r = !have_type_in_variant_container<tx_payer>(tx.extra);
  CHECK_AND_ASSERT_MES(r, false, "tx_payer is found in extra");

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, MK_TEST_COINS(11)), false, "");


  //
  // mine blocks 24, 25, 26 to activate HF2
  //
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 3);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  miner_wlt->refresh();
  alice_wlt->refresh();

  // check again (Miner -> Alice), with different amount
  req_a.destinations.front().amount = MK_TEST_COINS(2);
  r = miner_wlt_rpc.on_transfer(req_a, res, je, ctx);
  CHECK_AND_ASSERT_MES(r, false, "on_transfer failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  tx_hash = null_hash;
  CHECK_AND_ASSERT_MES(epee::string_tools::hex_to_pod(res.tx_hash, tx_hash), false, "");
  CHECK_AND_ASSERT_MES(c.get_transaction(tx_hash, tx), false, "");

  r = have_type_in_variant_container<tx_payer>(tx.extra);
  CHECK_AND_ASSERT_MES(r, false, "tx_payer is not found in extra");
  r = !have_type_in_variant_container<tx_payer_old>(tx.extra);
  CHECK_AND_ASSERT_MES(r, false, "tx_payer_old is found in extra");


  // check again (Miner -> Bob), with different amount
  req_b.destinations.front().amount = MK_TEST_COINS(2);
  r = miner_wlt_rpc.on_transfer(req_b, res, je, ctx);
  CHECK_AND_ASSERT_MES(r, false, "on_transfer failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 2, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

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
    [&](const tools::wallet_public::wallet_transfer_info& wti, const std::list<tools::wallet_public::asset_balance_entry>& balances, uint64_t total_mined) -> bool {
      CHECK_AND_ASSERT_THROW_MES(wti.get_native_amount() == MK_TEST_COINS(2), "incorrect wti.amount = " << print_money_brief(wti.get_native_amount()));
      CHECK_AND_ASSERT_THROW_MES(wti.show_sender, "show_sender is false");
      CHECK_AND_ASSERT_THROW_MES(wti.remote_addresses.size() == 1, "incorrect wti.remote_addresses.size() = " << wti.remote_addresses.size());
      CHECK_AND_ASSERT_THROW_MES(wti.remote_addresses.front() == m_accounts[MINER_ACC_IDX].get_public_address_str(), "wti.remote_addresses.front is incorrect");
      ++callback_counter;
      return true;
    }
  ));

  alice_wlt->callback(l2);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(4)), false, "");

  bob_wlt->callback(l2);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, MK_TEST_COINS(13)), false, "");

  alice_wlt->callback(std::make_shared<tools::i_wallet2_callback>()); // clear callback
  bob_wlt->callback(std::make_shared<tools::i_wallet2_callback>()); // clear callback

  // After HF2: Bob (auditable address) -> Alice with payer info requested (should put tx_payer)
  req_c = AUTO_VAL_INIT(req_c);
  req_c.destinations.push_back(tools::wallet_public::transfer_destination{ MK_TEST_COINS(1), m_accounts[ALICE_ACC_IDX].get_public_address_str() });
  req_c.fee = TESTS_DEFAULT_FEE;
  req_c.push_payer = true;
  res = AUTO_VAL_INIT(res);
  r = bob_wlt_rpc.on_transfer(req_c, res, je, ctx);
  CHECK_AND_ASSERT_MES(r, false, "on_transfer failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  tx_hash = null_hash;
  CHECK_AND_ASSERT_MES(epee::string_tools::hex_to_pod(res.tx_hash, tx_hash), false, "");
  tx = AUTO_VAL_INIT(tx);
  CHECK_AND_ASSERT_MES(c.get_transaction(tx_hash, tx), false, "");
  r = !have_type_in_variant_container<tx_payer_old>(tx.extra);
  CHECK_AND_ASSERT_MES(r, false, "tx_payer_old is found in extra");
  r = have_type_in_variant_container<tx_payer>(tx.extra);
  CHECK_AND_ASSERT_MES(r, false, "tx_payer is NOT found in extra");

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(5)), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, MK_TEST_COINS(11)), false, "");

  return true;
}

//------------------------------------------------------------------------------

hard_fork_2_tx_receiver_in_wallet::hard_fork_2_tx_receiver_in_wallet()
  : hard_fork_2_base_test(23)
  , m_alice_start_balance(0)
{
  REGISTER_CALLBACK_METHOD(hard_fork_2_tx_receiver_in_wallet, c1);
}

bool hard_fork_2_tx_receiver_in_wallet::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure that wallet uses tx_receiver_old only before HF2 and tx_receiver after

  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate(true); // Bob has auditable address

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  m_alice_start_balance = MK_TEST_COINS(111);
  MAKE_TX(events, tx_0, miner_acc, alice_acc, m_alice_start_balance, blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

bool hard_fork_2_tx_receiver_in_wallet::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, m_accounts[MINER_ACC_IDX]);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  std::shared_ptr<tools::wallet2> bob_wlt   = init_playtime_test_wallet(events, c, m_accounts[BOB_ACC_IDX]);

  miner_wlt->refresh();
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, m_alice_start_balance), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, 0), false, "");

  // wallet RPC server
  tools::wallet_rpc_server alice_wlt_rpc(alice_wlt);
  epee::json_rpc::error je;
  tools::wallet_rpc_server::connection_context ctx;

  tools::wallet_public::COMMAND_RPC_TRANSFER::request req = AUTO_VAL_INIT(req);
  req.destinations.push_back(tools::wallet_public::transfer_destination { MK_TEST_COINS(1), m_accounts[MINER_ACC_IDX].get_public_address_str() }); 
  req.destinations.push_back(tools::wallet_public::transfer_destination { MK_TEST_COINS(1), m_accounts[BOB_ACC_IDX].get_public_address_str() });  // auditable address
  req.fee = TESTS_DEFAULT_FEE;
  req.hide_receiver = false; // just to emphasize, this is false by default

  LOG_PRINT_L0("Miner's address: " << m_accounts[MINER_ACC_IDX].get_public_address_str());
  LOG_PRINT_L0("Alice's address: " << m_accounts[ALICE_ACC_IDX].get_public_address_str());
  LOG_PRINT_L0("Bob's address: "   << m_accounts[BOB_ACC_IDX].get_public_address_str());

  tools::wallet_public::COMMAND_RPC_TRANSFER::response res = AUTO_VAL_INIT(res);
  
  r = alice_wlt_rpc.on_transfer(req, res, je, ctx);
  CHECK_AND_ASSERT_MES(r, false, "on_transfer failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  crypto::hash tx_hash = null_hash;
  CHECK_AND_ASSERT_MES(epee::string_tools::hex_to_pod(res.tx_hash, tx_hash), false, "");
  
  transaction tx = AUTO_VAL_INIT(tx);
  CHECK_AND_ASSERT_MES(c.get_transaction(tx_hash, tx), false, "");

  // there should be one tx_receiver_old, as Bob's auditable address should not be supported for tx_receiver
  size_t count = count_type_in_variant_container<tx_receiver_old>(tx.extra);
  CHECK_AND_ASSERT_MES(count == 1, false, "tx_receiver_old count: " << count);

  count = count_type_in_variant_container<tx_receiver>(tx.extra);
  CHECK_AND_ASSERT_MES(count == 0, false, "tx_receiver count: " << count);

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  std::shared_ptr<wlt_lambda_on_transfer2_wrapper> l(new wlt_lambda_on_transfer2_wrapper(
    [&](const tools::wallet_public::wallet_transfer_info& wti, const std::list<tools::wallet_public::asset_balance_entry>& balances, uint64_t total_mined) -> bool {
      CHECK_AND_ASSERT_THROW_MES(!wti.get_native_is_income(), "wti.is_income is " << wti.get_native_is_income());
      CHECK_AND_ASSERT_THROW_MES(wti.remote_addresses.size() == 2, "incorrect wti.remote_addresses.size() = " << wti.remote_addresses.size());
      CHECK_AND_ASSERT_THROW_MES(wti.remote_addresses.front() == m_accounts[MINER_ACC_IDX].get_public_address_str(), "wti.remote_addresses.front is incorrect");
      CHECK_AND_ASSERT_THROW_MES(wti.remote_addresses.back() == m_accounts[BOB_ACC_IDX].get_public_address_str(), "wti.remote_addresses.back is incorrect");
      return true;
    }
  ));
  alice_wlt->callback(l);

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, m_alice_start_balance - MK_TEST_COINS(2) - TESTS_DEFAULT_FEE), false, "");

  // mine blocks 23, 24, 25 to activate HF2
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 3);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  miner_wlt->refresh();
  alice_wlt->refresh();

  // check again
  req.destinations.front().amount = MK_TEST_COINS(2);
  req.destinations.back().amount = MK_TEST_COINS(2);
  r = alice_wlt_rpc.on_transfer(req, res, je, ctx);
  CHECK_AND_ASSERT_MES(r, false, "on_transfer failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  tx_hash = null_hash;
  CHECK_AND_ASSERT_MES(epee::string_tools::hex_to_pod(res.tx_hash, tx_hash), false, "");
  CHECK_AND_ASSERT_MES(c.get_transaction(tx_hash, tx), false, "");

  // shouold be 2 tx_receiver as we passed HF2 and auditable addresses CAN be used with tx_receiver
  count = count_type_in_variant_container<tx_receiver>(tx.extra);
  CHECK_AND_ASSERT_MES(count == 2, false, "tx_receiver count = " << count);
  count = count_type_in_variant_container<tx_receiver_old>(tx.extra);
  CHECK_AND_ASSERT_MES(count == 0, false, "tx_receiver_old count = " << count);

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  std::shared_ptr<wlt_lambda_on_transfer2_wrapper> l2(new wlt_lambda_on_transfer2_wrapper(
    [&](const tools::wallet_public::wallet_transfer_info& wti, const std::list<tools::wallet_public::asset_balance_entry>& balances, uint64_t total_mined) -> bool {
      CHECK_AND_ASSERT_THROW_MES(!wti.get_native_is_income(), "wti.is_income is " << wti.get_native_is_income());
      CHECK_AND_ASSERT_THROW_MES(wti.get_native_amount() == MK_TEST_COINS(4), "incorrect wti.amount = " << print_money_brief(wti.get_native_amount()));
      CHECK_AND_ASSERT_THROW_MES(wti.remote_addresses.size() == 2, "incorrect wti.remote_addresses.size() = " << wti.remote_addresses.size());
      CHECK_AND_ASSERT_THROW_MES(wti.remote_addresses.front() == m_accounts[MINER_ACC_IDX].get_public_address_str(), "wti.remote_addresses.front is incorrect");
      CHECK_AND_ASSERT_THROW_MES(wti.remote_addresses.back() == m_accounts[BOB_ACC_IDX].get_public_address_str(), "wti.remote_addresses.back is incorrect");
      return true;
    }
  ));
  alice_wlt->callback(l2);

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, m_alice_start_balance - MK_TEST_COINS(6) - TESTS_DEFAULT_FEE * 2), false, "");

  return true;
}

//------------------------------------------------------------------------------

hard_fork_2_tx_extra_alias_entry_in_wallet::hard_fork_2_tx_extra_alias_entry_in_wallet()
  : hard_fork_2_base_test(23)
{
  REGISTER_CALLBACK_METHOD(hard_fork_2_tx_extra_alias_entry_in_wallet, c1);
}

bool hard_fork_2_tx_extra_alias_entry_in_wallet::generate(std::vector<test_event_entry>& events) const
{
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate(true); // auditable address

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  uint64_t biggest_alias_reward = get_alias_coast_from_fee("a", TESTS_DEFAULT_FEE);
  MAKE_TX(events, tx_0, miner_acc, alice_acc, biggest_alias_reward + TESTS_DEFAULT_FEE, blk_0r);
  MAKE_TX(events, tx_1, miner_acc, alice_acc, biggest_alias_reward + TESTS_DEFAULT_FEE, blk_0r);
  MAKE_TX(events, tx_2, miner_acc, alice_acc, biggest_alias_reward + TESTS_DEFAULT_FEE, blk_0r);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({ tx_0, tx_1, tx_2 }));

  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

bool hard_fork_2_tx_extra_alias_entry_in_wallet::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, m_accounts[MINER_ACC_IDX]);

  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE, false, "Incorrect numbers of blocks fetched");

  extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = "alicealice";
  ai.m_address = m_accounts[ALICE_ACC_IDX].get_public_address();
  uint64_t alias_reward = get_alias_coast_from_fee(ai.m_alias, TESTS_DEFAULT_FEE);
  transaction res_tx = AUTO_VAL_INIT(res_tx);
  alice_wlt->request_alias_registration(ai, res_tx, TESTS_DEFAULT_FEE, alias_reward);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // before the HF2 -- old structure should be present
  r = have_type_in_variant_container<extra_alias_entry_old>(res_tx.extra);
  CHECK_AND_ASSERT_MES(r, false, "extra_alias_entry_old is not found in extra");
  r = !have_type_in_variant_container<extra_alias_entry>(res_tx.extra);
  CHECK_AND_ASSERT_MES(r, false, "extra_alias_entry is found in extra");

  // before the HF2 an alias to an auditable address is not supported
  extra_alias_entry ai_bob = AUTO_VAL_INIT(ai_bob);
  ai_bob.m_alias = "bobbobbob";
  ai_bob.m_address = m_accounts[BOB_ACC_IDX].get_public_address();
  alias_reward = get_alias_coast_from_fee(ai_bob.m_alias, TESTS_DEFAULT_FEE);
  res_tx = AUTO_VAL_INIT(res_tx);
  r = false;
  try
  {
    alice_wlt->request_alias_registration(ai_bob, res_tx, TESTS_DEFAULT_FEE, alias_reward);
  }
  catch (...)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "exception was not cought as expected");

  // should be still one tx in the pool
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // try to update Alice's alias to an auditable address
  extra_alias_entry ai_alice_update = ai;
  ai_alice_update.m_text_comment = "Update to auditable";
  ai_alice_update.m_address = m_accounts[BOB_ACC_IDX].get_public_address(); // auditable
  CHECK_AND_ASSERT_MES(ai_alice_update.m_address.is_auditable(), false, "address is not auditable");
  r = false;
  try
  {
    alice_wlt->request_alias_update(ai_alice_update, res_tx, TESTS_DEFAULT_FEE, 0);
  }
  catch (...)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "exception was not cought as expected");


  // mine few blocks to activate HF2
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 2);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  CHECK_AND_ASSERT_MES(c.get_current_blockchain_size() == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE + 4, false, "Incorrect blockchain size");

  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 3, false, "Incorrect numbers of blocks fetched");

  // update alias, change comment and address
  ai.m_text_comment = "Update to normal";
  ai.m_address = m_accounts[MINER_ACC_IDX].get_public_address();
  alice_wlt->request_alias_update(ai, res_tx, TESTS_DEFAULT_FEE, 0);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // after HF2: extra_alias_entry should be here, not extra_alias_entry_old
  r = have_type_in_variant_container<extra_alias_entry>(res_tx.extra);
  CHECK_AND_ASSERT_MES(r, false, "extra_alias_entry is not found in extra");
  r = !have_type_in_variant_container<extra_alias_entry_old>(res_tx.extra);
  CHECK_AND_ASSERT_MES(r, false, "extra_alias_entry_old is found in extra");

  // make sure alias was updated indeed
  extra_alias_entry ai_check = AUTO_VAL_INIT(ai_check);
  r = c.get_blockchain_storage().get_alias_info(ai.m_alias, ai_check);
  CHECK_AND_ASSERT_MES(r, false, "get_alias_info failed");
  CHECK_AND_ASSERT_MES(ai_check.m_text_comment == ai.m_text_comment && ai_check.m_address == m_accounts[MINER_ACC_IDX].get_public_address(),
    false, "Incorrect alias info retunred by get_alias_info");


  // make sure an alias to auditable address can be registered now
  alice_wlt->request_alias_registration(ai_bob, res_tx, TESTS_DEFAULT_FEE, alias_reward);
  // after HF2: extra_alias_entry should be here, not extra_alias_entry_old
  r = have_type_in_variant_container<extra_alias_entry>(res_tx.extra);
  CHECK_AND_ASSERT_MES(r, false, "extra_alias_entry is not found in extra");
  r = !have_type_in_variant_container<extra_alias_entry_old>(res_tx.extra);
  CHECK_AND_ASSERT_MES(r, false, "extra_alias_entry_old is found in extra");

  // miner a block to confirm it
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // make sure alias was updated to an auditable address indeed
  ai_check = AUTO_VAL_INIT(ai_check);
  r = c.get_blockchain_storage().get_alias_info(ai_bob.m_alias, ai_check);
  CHECK_AND_ASSERT_MES(r, false, "get_alias_info failed");
  CHECK_AND_ASSERT_MES(ai_check.m_text_comment == ai_bob.m_text_comment && ai_check.m_address == m_accounts[BOB_ACC_IDX].get_public_address(),
    false, "Incorrect alias info retunred by get_alias_info");

  miner_wlt->refresh();
  
  // update alias once again, change comment and address to auditable
  // alias updated by miner, as he's the owner now
  miner_wlt->request_alias_update(ai_alice_update, res_tx, TESTS_DEFAULT_FEE, 0);

  // after HF2: extra_alias_entry should be here, not extra_alias_entry_old
  r = have_type_in_variant_container<extra_alias_entry>(res_tx.extra);
  CHECK_AND_ASSERT_MES(r, false, "extra_alias_entry is not found in extra");
  r = !have_type_in_variant_container<extra_alias_entry_old>(res_tx.extra);
  CHECK_AND_ASSERT_MES(r, false, "extra_alias_entry_old is found in extra");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // make sure alias was updated to an auditable address indeed
  ai_check = AUTO_VAL_INIT(ai_check);
  r = c.get_blockchain_storage().get_alias_info(ai_alice_update.m_alias, ai_check);
  CHECK_AND_ASSERT_MES(r, false, "get_alias_info failed");
  CHECK_AND_ASSERT_MES(ai_check.m_text_comment == ai_alice_update.m_text_comment && ai_check.m_address == m_accounts[BOB_ACC_IDX].get_public_address(),
    false, "Incorrect alias info retunred by get_alias_info");

  return true;
}

//------------------------------------------------------------------------------

hard_fork_2_auditable_addresses_basics::hard_fork_2_auditable_addresses_basics()
{
  REGISTER_CALLBACK_METHOD(hard_fork_2_auditable_addresses_basics, c1);
}

bool hard_fork_2_auditable_addresses_basics::generate(std::vector<test_event_entry>& events) const
{
  /*
    Test idea: make sure that:
      (1) before HF2 txs with mix_attr == 1 can be sent and then they are recognized by wallet;
      (2) before HF2 txs to an auditable address CAN be sent via wallet2::transfer()
      (3) after HF2 txs to an auditable address CAN be sent via wallet2::transfer()
  */

  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate(true); // Bob has auditable address

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  MAKE_TX(events, tx_0a, miner_acc, alice_acc, MK_TEST_COINS(11), blk_0r);
  MAKE_TX(events, tx_0b, miner_acc, alice_acc, MK_TEST_COINS(11), blk_0r);

  // tx_1 has outputs to an auditable address, it's allowed before HF2
  MAKE_TX(events, tx_1, miner_acc, bob_acc, MK_TEST_COINS(5), blk_0r);

  // make sure all Bob's outputs has mix_attr = 1
  for (auto& out : tx_1.vout)
  {
    if (out.type() != typeid(tx_out_bare))
      continue; // skip if we're in postzarcanum era
    if (boost::get<tx_out_bare>(out).amount != MK_TEST_COINS(5))
      continue; // skip change
    uint8_t mix_attr = boost::get<txout_to_key>(boost::get<tx_out_bare>(out).target).mix_attr;
    CHECK_AND_ASSERT_MES(mix_attr == CURRENCY_TO_KEY_OUT_FORCED_NO_MIX, false, "Incorrect mix_attr in tx_1: " << mix_attr);
  }
  
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({ tx_0a, tx_0b, tx_1 }));

  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

bool hard_fork_2_auditable_addresses_basics::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  std::shared_ptr<tools::wallet2> bob_wlt   = init_playtime_test_wallet(events, c, m_accounts[BOB_ACC_IDX]);

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(22), false, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, MK_TEST_COINS(5), false, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE), false, "");

  // sending coins to an auditable address should not be allowed until HF2
  std::vector<tx_destination_entry> destination{tx_destination_entry(MK_TEST_COINS(1), bob_wlt->get_account().get_public_address())};
  transaction tx = AUTO_VAL_INIT(tx);
  alice_wlt->transfer(destination, 0, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment, tx);

  // make sure all Bob's outputs has mix_attr = 1
  for (auto& out : tx.vout)
  {
    if (out.type() != typeid(tx_out_bare))
      continue; // skip if we're in postzarcanum era

    if (boost::get<tx_out_bare>(out).amount != MK_TEST_COINS(1))
      continue; // skip change
    uint8_t mix_attr = boost::get<txout_to_key>(boost::get<tx_out_bare>(out).target).mix_attr;
    CHECK_AND_ASSERT_MES(mix_attr == CURRENCY_TO_KEY_OUT_FORCED_NO_MIX, false, "Incorrect mix_attr in tx: " << mix_attr);
  }

  // mine a block to confirm the tx
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // mine few block to activate HF2
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 2);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(20), false, 3), false, "");

  // repeat the transfer after HF2 (using the same destinations)
  alice_wlt->transfer(destination, 0, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment, tx);

  // make sure all Bob's outputs has mix_attr = 1
  for (auto& out : tx.vout)
  {
    if (out.type() != typeid(tx_out_bare))
      continue; // skip if we're in postzarcanum era

    if (boost::get<tx_out_bare>(out).amount != MK_TEST_COINS(1))
      continue; // skip change
    uint8_t mix_attr = boost::get<txout_to_key>(boost::get<tx_out_bare>(out).target).mix_attr;
    CHECK_AND_ASSERT_MES(mix_attr == CURRENCY_TO_KEY_OUT_FORCED_NO_MIX, false, "Incorrect mix_attr in tx: " << mix_attr);
  }

  // mine a block to confirm the tx
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // make sure the funds were received
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, MK_TEST_COINS(5 + 1 + 1), false, 3 + 1), false, "");

  return true;
}


//------------------------------------------------------------------------------

hard_fork_2_no_new_structures_before_hf::hard_fork_2_no_new_structures_before_hf()
  : hard_fork_2_base_test(16)
{
  REGISTER_CALLBACK_METHOD(hard_fork_2_no_new_structures_before_hf, c1);
}

bool hard_fork_2_no_new_structures_before_hf::generate(std::vector<test_event_entry>& events) const
{
  test_gentime_settings tgs = test_generator::get_test_gentime_settings();
  tgs.ignore_invalid_txs = false; // this test pushes originally invalid tx_0, tx_1 and tx_2 which are good after HF2, so we'd like to avoid mess with the sources among txs
  test_generator::set_test_gentime_settings(tgs);

  bool r = false;
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  std::vector<currency::extra_v> extra;


  // extra with tx_payer -- not allowed before HF2
  tx_payer payer = AUTO_VAL_INIT(payer);
  payer.acc_addr = miner_acc.get_public_address();
  extra.push_back(payer);
  DO_CALLBACK(events, "mark_invalid_tx");
  MAKE_TX_MIX_ATTR_EXTRA(events, tx_0, miner_acc, alice_acc, MK_TEST_COINS(1), 0, blk_0r, 0, extra, true);

  // blk_1b_1 is invalid as containing tx_0
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_1b, blk_0r, miner_acc, tx_0);


  // extra with tx_payer_old -- okay
  extra.clear();
  tx_payer_old payer_old = AUTO_VAL_INIT(payer_old);
  payer_old.acc_addr = miner_acc.get_public_address().to_old();
  extra.push_back(payer_old);
  MAKE_TX_MIX_ATTR_EXTRA(events, tx_0_old, miner_acc, alice_acc, MK_TEST_COINS(1), 0, blk_0r, 0, extra, true);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0_old);


  // extra with tx_receiver -- not allowed before HF2
  extra.clear();
  tx_receiver receiver = AUTO_VAL_INIT(receiver);
  receiver.acc_addr = miner_acc.get_public_address();
  extra.push_back(receiver);
  DO_CALLBACK(events, "mark_invalid_tx");
  MAKE_TX_MIX_ATTR_EXTRA(events, tx_1, miner_acc, alice_acc, MK_TEST_COINS(1), 0, blk_1, 0, extra, true);

  // blk_1b_2 is invalid as containing tx_1
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_2b, blk_1, miner_acc, tx_1);


  // extra with tx_receiver_old -- okay
  extra.clear();
  tx_receiver_old receiver_old = AUTO_VAL_INIT(receiver_old);
  receiver_old.acc_addr = miner_acc.get_public_address().to_old();
  extra.push_back(receiver_old);
  MAKE_TX_MIX_ATTR_EXTRA(events, tx_1_old, miner_acc, alice_acc, MK_TEST_COINS(1), 0, blk_1, 0, extra, true);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, tx_1_old);


  // extra with extra_alias_entry -- not allowed before HF2
  extra_alias_entry alias_entry = AUTO_VAL_INIT(alias_entry);
  alias_entry.m_address = miner_acc.get_public_address();
  alias_entry.m_alias = "minerminer";

  std::list<transaction> tx_set;
  DO_CALLBACK(events, "mark_invalid_tx");
  r = put_alias_via_tx_to_list(m_hardforks, events, tx_set, blk_2, miner_acc, alias_entry, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");
  transaction tx_2 = tx_set.front();

  // blk_1b_3 is invalid as containing tx_2
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_3b, blk_2, miner_acc, tx_2);


  // extra with extra_alias_entry_old -- okay
  extra_alias_entry_old alias_entry_old = AUTO_VAL_INIT(alias_entry_old);
  alias_entry_old.m_address = alice_acc.get_public_address().to_old();
  alias_entry_old.m_alias = "alicealice";

  tx_set.clear();
  r = put_alias_via_tx_to_list(m_hardforks, events, tx_set, blk_2, miner_acc, alias_entry_old, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");
  transaction tx_2_old = tx_set.front();
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_2_old);


  // activate HF2
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_5, blk_4, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_6, blk_5, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_7, blk_6, miner_acc);

  ADD_CUSTOM_EVENT(events, tx_0);
  ADD_CUSTOM_EVENT(events, tx_1);
  ADD_CUSTOM_EVENT(events, tx_2);


  // tx_0 with tx_payer should be accepted after HF2
  MAKE_NEXT_BLOCK_TX1(events, blk_8, blk_7, miner_acc, tx_0);

  // tx_1 with tx_receiver should be accepted after HF2
  MAKE_NEXT_BLOCK_TX1(events, blk_9, blk_8, miner_acc, tx_1);

  // tx_2 with extra_alias_entry should be accepted after HF2
  MAKE_NEXT_BLOCK_TX1(events, blk_10, blk_9, miner_acc, tx_2);

  // check aliases
  DO_CALLBACK(events, "c1");

  return true;
}

bool hard_fork_2_no_new_structures_before_hf::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  extra_alias_entry_base ai = AUTO_VAL_INIT(ai);
  bool r = c.get_blockchain_storage().get_alias_info("alicealice", ai);
  CHECK_AND_ASSERT_MES(r, false, "failed to get alias");
  CHECK_AND_ASSERT_MES(ai.m_address == m_accounts[ALICE_ACC_IDX].get_public_address(), false, "invalid address for alicealice alias");

  ai = AUTO_VAL_INIT(ai);
  r = c.get_blockchain_storage().get_alias_info("minerminer", ai);
  CHECK_AND_ASSERT_MES(r, false, "failed to get alias minerminer");
  CHECK_AND_ASSERT_MES(ai.m_address == m_accounts[MINER_ACC_IDX].get_public_address(), false, "invalid address for minerminer alias");

  return true;
}

//------------------------------------------------------------------------------

template<bool before_hf_2>
hard_fork_2_awo_wallets_basic_test<before_hf_2>::hard_fork_2_awo_wallets_basic_test()
  : hard_fork_2_base_test(before_hf_2 ? 100 : 3)
{
  REGISTER_CALLBACK_METHOD(hard_fork_2_awo_wallets_basic_test, c1);
}

template<bool before_hf_2>
bool hard_fork_2_awo_wallets_basic_test<before_hf_2>::generate(std::vector<test_event_entry>& events) const
{
  bool r = false;
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate(true); // Bob has auditable address

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), MK_TEST_COINS(110), 10, TESTS_DEFAULT_FEE, tx_0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_0);

  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

template<bool before_hf_2>
bool hard_fork_2_awo_wallets_basic_test<before_hf_2>::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  static const std::wstring bob_wo_filename(L"bob_wo_wallet");
  static const std::wstring bob_wo_restored_filename(L"bob_wo_restored_wallet");
  static const std::wstring bob_non_auditable_filename(L"bob_non_auditable_wallet");

  bool r = false;
  
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  std::shared_ptr<tools::wallet2> alice_wlt   = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt     = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt_awo = std::make_shared<tools::wallet2>();

  boost::system::error_code ec;
  boost::filesystem::remove(bob_wo_filename, ec);
  bob_wlt->store_watch_only(bob_wo_filename, "");

  bob_wlt_awo->load(bob_wo_filename, "");
  bob_wlt_awo->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());
  bob_wlt_awo->set_core_proxy(m_core_proxy);
  bob_wlt_awo->set_disable_tor_relay(true);

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(110), false, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, 0, false, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob_awo", bob_wlt_awo, 0, false, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE), false, "");

  CHECK_AND_ASSERT_MES(bob_wlt->get_account().get_public_address() == bob_wlt_awo->get_account().get_public_address(), false, "Bob addresses do not match");

  //
  // Alice -> Bob, Bob_awo
  //
  std::vector<tx_destination_entry> destinations;
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(5), bob_wlt->get_account().get_public_address()));
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(5), bob_wlt_awo->get_account().get_public_address()));
  alice_wlt->transfer(destinations, 2, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(99), false, 1), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, MK_TEST_COINS(10), false, 1), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob_awo", bob_wlt_awo, MK_TEST_COINS(10), false, 1), false, "");

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, WALLET_DEFAULT_TX_SPENDABLE_AGE);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  alice_wlt->refresh();
  bob_wlt->refresh();
  bob_wlt_awo->refresh();

  //
  // Bob -> miner
  //
  r = false;
  try
  {
    // first, try with non-zero mixins first -- should fail
    bob_wlt->transfer(std::vector<tx_destination_entry>{tx_destination_entry(MK_TEST_COINS(9), m_accounts[MINER_ACC_IDX].get_public_address())}, 1 /*mixins*/, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment);
  }
  catch (...)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "an exception was not caught as expected");

  r = false;
  try
  {
    // second, try from bob_wlt_awo -- should fail (watch-only wallet)
    bob_wlt_awo->transfer(std::vector<tx_destination_entry>{tx_destination_entry(MK_TEST_COINS(9), m_accounts[MINER_ACC_IDX].get_public_address())}, 0 /*mixins*/, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment);
  }
  catch (...)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "an exception was not caught as expected");


  // third, try from bob_wlt with zero mixins first -- should pass
  bob_wlt->transfer(std::vector<tx_destination_entry>{tx_destination_entry(MK_TEST_COINS(9), m_accounts[MINER_ACC_IDX].get_public_address())}, 0 /*mixins*/, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(99), false, 1), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, MK_TEST_COINS(0), false, 1), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob_awo", bob_wlt_awo, MK_TEST_COINS(0), false, 1), false, "");

  //
  // Alice -> Bob as non-auditable (mix_attr != 1)
  // this transfer should not be taken into account for Bob and bob_wlt_awo
  //
  account_public_address bob_addr_non_aud = bob_wlt->get_account().get_public_address();
  bob_addr_non_aud.flags = 0; // clear auditable flag

  alice_wlt->transfer(MK_TEST_COINS(7), bob_addr_non_aud);
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  bool callback_called = false;
  std::shared_ptr<wlt_lambda_on_transfer2_wrapper> l(new wlt_lambda_on_transfer2_wrapper(
    [&callback_called](const tools::wallet_public::wallet_transfer_info& wti, const std::list<tools::wallet_public::asset_balance_entry>& balances, uint64_t total_mined) -> bool {
      callback_called = true;
      return true;
    }
  ));
  alice_wlt->callback(l);
  bob_wlt->callback(l);
  bob_wlt_awo->callback(l);

  callback_called = false;
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(91), false, 1), false, "");
  CHECK_AND_ASSERT_MES(callback_called, false, "callback was not called");
  callback_called = false;
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, MK_TEST_COINS(0), false, 1), false, "");
  CHECK_AND_ASSERT_MES(!callback_called, false, "callback was called");
  callback_called = false;
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob_awo", bob_wlt_awo, MK_TEST_COINS(0), false, 1), false, "");
  CHECK_AND_ASSERT_MES(!callback_called, false, "callback was called");


  //
  // Alice -> Bob (normal)
  //
  alice_wlt->transfer(MK_TEST_COINS(3), m_accounts[BOB_ACC_IDX].get_public_address());
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(87), false, 1), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, MK_TEST_COINS(3), false, 1), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob_awo", bob_wlt_awo, MK_TEST_COINS(3), false, 1), false, "");

  //
  // Make sure a wallet, restored from awo blob will has the very same balance
  //
  account_base& bob_acc = m_accounts[BOB_ACC_IDX];
  std::string bob_tracking_seed = bob_acc.get_tracking_seed();

  std::shared_ptr<tools::wallet2> bob_wlt_awo_restored = std::make_shared<tools::wallet2>();

  boost::filesystem::remove(bob_wo_restored_filename, ec);

  bob_wlt_awo_restored->restore(bob_wo_restored_filename, "", bob_tracking_seed, true, "");
  bob_wlt_awo_restored->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());
  bob_wlt_awo_restored->set_core_proxy(m_core_proxy);
  bob_wlt_awo_restored->set_disable_tor_relay(true);

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob_awo_restored", bob_wlt_awo_restored, MK_TEST_COINS(3), false), false, "");


  // miner few blocks to unlock coins
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, WALLET_DEFAULT_TX_SPENDABLE_AGE);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  bob_wlt->refresh();

  //
  // Bob -> miner, and check again all 3 wallets
  //
  bob_wlt->transfer(MK_TEST_COINS(1), m_accounts[MINER_ACC_IDX].get_public_address());
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, MK_TEST_COINS(1), false, 1), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob_awo", bob_wlt_awo, MK_TEST_COINS(1), false, WALLET_DEFAULT_TX_SPENDABLE_AGE + 1), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob_awo_restored", bob_wlt_awo_restored, MK_TEST_COINS(1), false, WALLET_DEFAULT_TX_SPENDABLE_AGE + 1), false, "");


  //
  // Restore Bob wallet as non-auditable and spend mix_attr!=1 output => make sure other auditable Bob's wallets remain intact
  //

  std::string bob_seed = bob_wlt->get_account().get_seed_phrase("");
  bob_seed.erase(bob_seed.find_last_of(" ")); // remove the last word (with flags and checksum) to make seed old-format 25-words non-auditable with the same keys

  std::shared_ptr<tools::wallet2> bob_wlt_non_auditable = std::make_shared<tools::wallet2>();

  boost::filesystem::remove(bob_non_auditable_filename, ec);

  bob_wlt_non_auditable->restore(bob_non_auditable_filename, "", bob_seed, false, "");
  bob_wlt_non_auditable->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());
  bob_wlt_non_auditable->set_core_proxy(m_core_proxy);
  bob_wlt_non_auditable->set_disable_tor_relay(true);

  // the balance for non-auditable wallet should be greather by mix_attr!=1 output (7 test coins + 1 left from prev step)
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob_non_auditable", bob_wlt_non_auditable, MK_TEST_COINS(8), false), false, "");

  // spend mix_attr!=1 7-coins output
  bob_wlt_non_auditable->transfer(MK_TEST_COINS(6), m_accounts[ALICE_ACC_IDX].get_public_address());

  // mine a block
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // all auditable wallets should keep the same balance value
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, MK_TEST_COINS(1), false, 1), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob_awo", bob_wlt_awo, MK_TEST_COINS(1), false, 1), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob_awo_restored", bob_wlt_awo_restored, MK_TEST_COINS(1), false, 1), false, "");
  
  // non-auditable should also show the same balance as we've just spent mix_attr!=1 output
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob_non_auditable", bob_wlt_non_auditable, MK_TEST_COINS(1), false, 1), false, "");

  // make sure Alice received coins
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(93), false, 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE + 1), false, "");

  return true;
}

template hard_fork_2_awo_wallets_basic_test<false>::hard_fork_2_awo_wallets_basic_test();
template bool hard_fork_2_awo_wallets_basic_test<false>::generate(std::vector<test_event_entry>& events) const;
template hard_fork_2_awo_wallets_basic_test<true>::hard_fork_2_awo_wallets_basic_test();
template bool hard_fork_2_awo_wallets_basic_test<true>::generate(std::vector<test_event_entry>& events) const;


//------------------------------------------------------------------------------


template<bool before_hf_2>
hard_fork_2_alias_update_using_old_tx<before_hf_2>::hard_fork_2_alias_update_using_old_tx()
  : hard_fork_2_base_test(before_hf_2 ? 100 : 3)
{
  REGISTER_CALLBACK_METHOD(hard_fork_2_alias_update_using_old_tx, c1);
  
  random_state_test_restorer::reset_random(0); // deterministic test
}

template<bool before_hf_2>
bool hard_fork_2_alias_update_using_old_tx<before_hf_2>::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure that if old, pre-auditable and pre-HF2 code updates an alias is it correctly handled by the most most recent code
  // TODO: difficult to support, we need to implement it another way

  bool r = false;
  uint64_t ts = 1591713000;
  test_core_time::adjust(ts);

  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); // miner_acc.restore_from_seed_phrase("use use use use use use use use use use use use use use use use use use use use use use use use out");
  miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); // alice_acc.restore_from_seed_phrase("any any any any any any any any any any any any any any any any any any any any any any any any out");
  alice_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core");
  events.push_back(event_core_time(ts));

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), MK_TEST_COINS(110), 10, TESTS_DEFAULT_FEE, tx_0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_0);

  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

template<bool before_hf_2>
bool hard_fork_2_alias_update_using_old_tx<before_hf_2>::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(110), false, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE), false, "");

  extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = "alicealice";
  ai.m_address = m_accounts[ALICE_ACC_IDX].get_public_address();
  uint64_t alias_reward = get_alias_coast_from_fee(ai.m_alias, TESTS_DEFAULT_FEE);
  transaction res_tx = AUTO_VAL_INIT(res_tx);
  alice_wlt->request_alias_registration(ai, res_tx, TESTS_DEFAULT_FEE, alias_reward);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());


  // make sure the aliase registration is correct
  extra_alias_entry_base eaeb = AUTO_VAL_INIT(eaeb);
  r = c.get_blockchain_storage().get_alias_info("alicealice", eaeb);
  CHECK_AND_ASSERT_MES(r && eaeb.m_address == m_accounts[ALICE_ACC_IDX].get_public_address(), false, "aliase check failed: alicealice");

  extra_alias_entry ai_upd = ai;

  // update alias, change comment and address
  ai_upd.m_text_comment = "Hello miner!";
  ai_upd.m_address = m_accounts[MINER_ACC_IDX].get_public_address();
  
  
  /*
  //
  // Send tx and print it's blob
  // this code was used to generate old-style tx in develop branch, commit 36eabb916b95c7db06bdfb5d34d9820bd94b82df 
  // UPD: 2024-03-15 tx blob was updated as of commit f538fc3a7612e7ce86556a1cd2602400f6b4e9aa 
  //
  {
    transaction tx_upd = AUTO_VAL_INIT(tx_upd);
    alice_wlt->request_alias_update(ai_upd, tx_upd, TESTS_DEFAULT_FEE, 0);
    std::string tx_upd_hex = epee::string_tools::buff_to_hex_nodelimer(t_serializable_object_to_blob(tx_upd));
    LOG_PRINT_L0("tx upd: " << ENDL << tx_upd_hex);
  }
  */
  

  // use prepared tx blob
  std::string tx_upd_hex("01010180988be49903011a01000000000000007bdeaff6ac11597e9fe397349dd2ecfcd6880cda9bbedaa67f759a5b95ba2ed9000180d0dbc3f4020335db05c510daddcb82257be70789650007ad2d567092ec1eb2a38a93ef8fa0070004140a616c696365616c696365efdcf084d7af59e0d68e8bda3ff4514a02d812ccfd6a09f69811c5a6a1b035c9b1b2395fcd84283d02e2fc4f37395ac9d8f01ff3f1ad1b94a26aaf1c76bb39d00c48656c6c6f206d696e6572210001c2537b8203e9c9981b87a3c63cc0c63e4ccc14ddd0309fdcee94664e4b3bc307ee438e90bb5720b7ac7cbec38fcac9c7aa5b9df02abd2fa1a58c580f9b3eaf0516116e8d89a67440aad0922a7e5dd88870e6f0af792a7af201ee21effd90cc6c7f1700000b02f37101016ed0997a3fe0c6bc47ffde14599a77697ba95c5c1489b65e5e54854146079f06569313d921f19ed2848cdccc8bad7f75fb46fc1608912365f569b7690703010b00");
  std::string tx_upd_blob;
  r = epee::string_tools::parse_hexstr_to_binbuff(tx_upd_hex, tx_upd_blob);
  CHECK_AND_ASSERT_MES(r, false, "parse_hexstr_to_binbuff failed");
  crypto::hash tx_upd_hash{};
  transaction tx_upd{};
  CHECK_AND_ASSERT_MES(parse_and_validate_tx_from_blob(tx_upd_blob, tx_upd, tx_upd_hash), false, "parse_tx_form_blob failed");
  //LOG_PRINT_L0("tx_upd: " << tx_upd_hash << ", json:" << ENDL << obj_to_json_str(tx_upd));
  tx_verification_context tvc = AUTO_VAL_INIT(tvc);
  r = c.handle_incoming_tx(tx_upd, tvc, false, tx_upd_hash);
  CHECK_AND_ASSERT_MES(r, false, "handle_incoming_tx failed");


  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // make sure the aliase registration is correct
  eaeb = AUTO_VAL_INIT(eaeb);
  r = c.get_blockchain_storage().get_alias_info("alicealice", eaeb);
  CHECK_AND_ASSERT_MES(r && eaeb.m_address == m_accounts[MINER_ACC_IDX].get_public_address() && eaeb.m_text_comment == ai_upd.m_text_comment, false, "updated alias check failed: alicealice");

  return true;
}

template hard_fork_2_alias_update_using_old_tx<false>::hard_fork_2_alias_update_using_old_tx();
template bool hard_fork_2_alias_update_using_old_tx<false>::generate(std::vector<test_event_entry>& events) const;
template hard_fork_2_alias_update_using_old_tx<true>::hard_fork_2_alias_update_using_old_tx();
template bool hard_fork_2_alias_update_using_old_tx<true>::generate(std::vector<test_event_entry>& events) const;


//------------------------------------------------------------------------------

template<bool before_hf_2>
hard_fork_2_incorrect_alias_update<before_hf_2>::hard_fork_2_incorrect_alias_update()
  : hard_fork_2_base_test(before_hf_2 ? 100 : 3)
{
  REGISTER_CALLBACK_METHOD(hard_fork_2_incorrect_alias_update, c1);
}

template<bool before_hf_2>
bool hard_fork_2_incorrect_alias_update<before_hf_2>::generate(std::vector<test_event_entry>& events) const
{
  bool r = false;
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate(true); // Bob has auditable address

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), MK_TEST_COINS(110), 10, TESTS_DEFAULT_FEE, tx_0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_0);

  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

template<bool before_hf_2>
bool hard_fork_2_incorrect_alias_update<before_hf_2>::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = "alicealice";
  ai.m_address = m_accounts[ALICE_ACC_IDX].get_public_address();
  uint64_t alias_reward = get_alias_coast_from_fee(ai.m_alias, TESTS_DEFAULT_FEE);
  transaction res_tx = AUTO_VAL_INIT(res_tx);
  alice_wlt->request_alias_registration(ai, res_tx, TESTS_DEFAULT_FEE, alias_reward);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());


  // make sure the aliase registration is correct
  extra_alias_entry_base eaeb = AUTO_VAL_INIT(eaeb);
  r = c.get_blockchain_storage().get_alias_info("alicealice", eaeb);
  CHECK_AND_ASSERT_MES(r && eaeb.m_address == m_accounts[ALICE_ACC_IDX].get_public_address(), false, "aliase check failed: alicealice");

  extra_alias_entry ai_upd = ai;

  // update alias incorrectly by changing its name and keeping the address the same
  ai_upd.m_alias = "alicealice2";
  ai_upd.m_text_comment = "updated";
  
  transaction tx_upd = AUTO_VAL_INIT(tx_upd);
  r = false;
  try
  {
    alice_wlt->request_alias_update(ai_upd, tx_upd, TESTS_DEFAULT_FEE, 0);
  }
  catch (tools::error::tx_rejected&)
  {
    // this should cause an exception with certain type
    r = true;
  }
  
  CHECK_AND_ASSERT_MES(r, false, "incorrect alias update request accepted");

  // make sure tx pool is still having no txs
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  return true;
}

template hard_fork_2_incorrect_alias_update<false>::hard_fork_2_incorrect_alias_update();
template bool hard_fork_2_incorrect_alias_update<false>::generate(std::vector<test_event_entry>& events) const;
template hard_fork_2_incorrect_alias_update<true>::hard_fork_2_incorrect_alias_update();
template bool hard_fork_2_incorrect_alias_update<true>::generate(std::vector<test_event_entry>& events) const;
