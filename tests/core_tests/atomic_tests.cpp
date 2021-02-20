// Copyright (c) 2014-2021 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "escrow_wallet_tests.h"
#include "random_helper.h"
#include "chaingen_helpers.h"
#include "atomic_tests.h"

using namespace epee;
using namespace crypto;
using namespace currency;

#define INIT_RUNTIME_WALLET(instance_name)   \
  currency::account_base instance_name##acc_base; \
  instance_name##acc_base.generate(); \
  LOG_PRINT_MAGENTA(": " << currency::get_account_address_as_str(instance_name##acc_base.get_public_address()), LOG_LEVEL_0); \
  std::shared_ptr<tools::wallet2> instance_name = init_playtime_test_wallet(events, c, instance_name##acc_base);


//==============================================================================================================================

struct wallet_tests_callback_handler : public tools::i_wallet2_callback
{
  virtual void on_transfer2(const tools::wallet_public::wallet_transfer_info& wti, uint64_t balance, uint64_t unlocked_balance, uint64_t total_mined) 
  {
    all_wtis.push_back(wti);
  }

  std::vector<tools::wallet_public::wallet_transfer_info> all_wtis;
};

atomic_simple_test::atomic_simple_test()
{
  REGISTER_CALLBACK_METHOD(atomic_simple_test, c1);
}

bool atomic_simple_test::generate(std::vector<test_event_entry>& events) const
{
  epee::debug::get_set_enable_assert(true, true);

  currency::account_base genesis_acc;
  genesis_acc.generate();
  m_mining_accunt.generate();


  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, genesis_acc, test_core_time::get_time());
  events.push_back(blk_0);

  REWIND_BLOCKS_N(events, blk_0r, blk_0, m_mining_accunt, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 5);

  DO_CALLBACK(events, "c1");
  epee::debug::get_set_enable_assert(true, false);
  return true;
}



bool atomic_simple_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  epee::debug::get_set_enable_assert(true, true);
  misc_utils::auto_scope_leave_caller scope_exit_handler = misc_utils::create_scope_leave_handler([&](){epee::debug::get_set_enable_assert(true, false); });


  /*
  let's pretend that we have two different blockchains: A and B
  Alice and Bob want to have atomic swap via htlc, both has wallets in blockchain A and B
  Steps are following:
  1. Alice initiate swap by sending to Bob HTLC in blockchain A
  2. Bob see HTLC to his address in blockchain A and create HTLC with 
     the same hash addressed to Alice in blockchain B
  3. Alice see HTLC addressed to her in blockchain B and creates there redeem transaction, which reveals HTLC origin 
  4. Bob observe origin revealed by Alice in blockchain B and create redeem transaction in blockchain A
  5. Everybody check balances and celebrate successful atomic swap 
  
  */

  LOG_PRINT_MAGENTA("Mining Address: " << currency::get_account_address_as_str(m_mining_accunt.get_public_address()), LOG_LEVEL_0);


  //create wallet instances and calculate balances
  INIT_RUNTIME_WALLET(alice_a_wlt_instance);
  INIT_RUNTIME_WALLET(alice_b_wlt_instance);
  INIT_RUNTIME_WALLET(bob_a_wlt_instance);
  INIT_RUNTIME_WALLET(bob_b_wlt_instance);
  INIT_RUNTIME_WALLET(refund_test_instance);
  INIT_RUNTIME_WALLET(refund_test_instance2);

#define AMOUNT_TO_TRANSFER_HTLC    (TESTS_DEFAULT_FEE*10)

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, m_mining_accunt);

  size_t blocks_fetched = 0;
  bool received_money = false;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  uint64_t transfer_amount = AMOUNT_TO_TRANSFER_HTLC + TESTS_DEFAULT_FEE;
  miner_wlt->transfer(transfer_amount, alice_a_wlt_instance->get_account().get_public_address());
  LOG_PRINT_MAGENTA("Transaction sent to Alice A: " << transfer_amount, LOG_LEVEL_0);

  miner_wlt->transfer(transfer_amount, bob_b_wlt_instance->get_account().get_public_address());
  LOG_PRINT_MAGENTA("Transaction sent to Bob B: " << transfer_amount, LOG_LEVEL_0);

  miner_wlt->transfer(transfer_amount, refund_test_instance->get_account().get_public_address());
  LOG_PRINT_MAGENTA("Transaction sent to Refund test: " << transfer_amount, LOG_LEVEL_0);

  bool r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");


  alice_a_wlt_instance->refresh();
  alice_b_wlt_instance->refresh();
  bob_a_wlt_instance->refresh();
  bob_b_wlt_instance->refresh();
  refund_test_instance->refresh();
  refund_test_instance2->refresh();


  CHECK_AND_FORCE_ASSERT_MES(alice_a_wlt_instance->balance() == transfer_amount, false, "Incorrect balance");
  CHECK_AND_FORCE_ASSERT_MES(bob_b_wlt_instance->balance() == transfer_amount, false, "Incorrect balance");
  CHECK_AND_FORCE_ASSERT_MES(refund_test_instance->balance() == transfer_amount, false, "Incorrect balance");


  //=============  phase 1  =============
  //-----------  preparation  -----------
  //a) basic full atomic process test 
  std::string alice_origin; //will be deterministically generated by Alice's A wallet
  currency::transaction res_tx = AUTO_VAL_INIT(res_tx);
  alice_a_wlt_instance->create_htlc_proposal(transfer_amount - TESTS_DEFAULT_FEE, bob_a_wlt_instance->get_account().get_public_address(), 100, res_tx, currency::null_hash, alice_origin);

  //b) htlc refund test
  std::string refund_origin; //will be deterministically generated by Alice's A wallet
  currency::transaction refund_res_tx = AUTO_VAL_INIT(refund_res_tx);
  refund_test_instance->create_htlc_proposal(transfer_amount - TESTS_DEFAULT_FEE, miner_wlt->get_account().get_public_address(), 8, refund_res_tx, currency::null_hash, refund_origin);


  //-----------  rewinding  -----------
  r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");
  alice_a_wlt_instance->refresh();
  alice_b_wlt_instance->refresh();
  bob_a_wlt_instance->refresh();
  bob_b_wlt_instance->refresh();
  refund_test_instance->refresh();
  refund_test_instance2->refresh();



  //-----------  checks  -----------
  //a) basic full atomic process test 
  std::list<tools::wallet_public::htlc_entry_info> htlcs_alice_a;
  alice_a_wlt_instance->get_list_of_active_htlc(htlcs_alice_a, false);
  CHECK_AND_ASSERT_MES(htlcs_alice_a.size() == 1, false, "htlcs_alice.size() == 1 failed");

  std::list<tools::wallet_public::htlc_entry_info> htlcs_bob_a;
  bob_a_wlt_instance->get_list_of_active_htlc(htlcs_bob_a, false);
  CHECK_AND_ASSERT_MES(htlcs_bob_a.size() == 1, false, "htlcs_bob.size() == 1  failed");

  const tools::wallet_public::htlc_entry_info& hei_bob = *htlcs_bob_a.begin();
  CHECK_AND_ASSERT_MES(hei_bob.is_redeem == true, false, "hei_bob.is_redeem == true failed");


  const tools::wallet_public::htlc_entry_info& hei_alice = *htlcs_alice_a.begin();
  CHECK_AND_ASSERT_MES(hei_alice.is_redeem == false, false, "hei_alice.is_redeem == false failed");

  CHECK_AND_ASSERT_MES(hei_alice.amount == hei_bob.amount 
    && hei_alice.sha256_hash == hei_bob.sha256_hash
    && hei_alice.tx_id == hei_bob.tx_id, false, "hei_alice !=hei_bob ");

  //b) htlc refund test 
  //money at refund_test_instance should be released as refunded, and balance should get to transfer_amount - TESTS_DEFAULT_FEE
  CHECK_AND_ASSERT_MES(refund_test_instance->balance() == transfer_amount - TESTS_DEFAULT_FEE, false, "refund_test_instance->balance() == transfer_amount - TESTS_DEFAULT_FEE failed");

  //=============  phase 2  =============
  //-----------  preparation  -----------
  //a) basic full atomic process test 
  currency::transaction res_bob_tx = AUTO_VAL_INIT(res_tx);
  std::string dummy_origin;
  bob_b_wlt_instance->create_htlc_proposal(transfer_amount - TESTS_DEFAULT_FEE, 
    alice_b_wlt_instance->get_account().get_public_address(),
    100, 
    res_tx, hei_bob.sha256_hash, dummy_origin);
  
  //b) htlc refund test 
  //try to spend refunded money to make sure it's spendable 
  refund_test_instance->transfer(transfer_amount - TESTS_DEFAULT_FEE * 2, refund_test_instance2->get_account().get_public_address());

  //-----------  rewinding  -----------
  r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");
  alice_a_wlt_instance->refresh();
  alice_b_wlt_instance->refresh();
  bob_a_wlt_instance->refresh();
  bob_b_wlt_instance->refresh();
  refund_test_instance->refresh();
  refund_test_instance2->refresh();


  //-----------  checks  -----------
  //a) basic full atomic process test 
  std::list<tools::wallet_public::htlc_entry_info> htlcs_alice_b;
  alice_b_wlt_instance->get_list_of_active_htlc(htlcs_alice_b, false);
  CHECK_AND_ASSERT_MES(htlcs_alice_b.size() == 1, false, "htlcs_alice_b.size() == 1 failed");

  std::list<tools::wallet_public::htlc_entry_info> htlcs_bob_b;
  bob_b_wlt_instance->get_list_of_active_htlc(htlcs_bob_b, false);
  CHECK_AND_ASSERT_MES(htlcs_bob_b.size() == 1, false, "htlcs_bob_b.size() == 1  failed");

  std::list<tools::wallet_public::htlc_entry_info> htlcs_bob_a_2;
  bob_a_wlt_instance->get_list_of_active_htlc(htlcs_bob_a_2, false);
  CHECK_AND_ASSERT_MES(htlcs_bob_a_2.size() == 1, false, "htlcs_bob_a.size() == 1  failed");


  const tools::wallet_public::htlc_entry_info& hei_bob_b = *htlcs_bob_b.begin();
  CHECK_AND_ASSERT_MES(hei_bob_b.is_redeem == false, false, "hei_bob_b.is_redeem == true failed");


  const tools::wallet_public::htlc_entry_info& hei_alice_b = *htlcs_alice_b.begin();
  CHECK_AND_ASSERT_MES(hei_alice_b.is_redeem == true, false, "hei_alice_b.is_redeem == false failed");

  CHECK_AND_ASSERT_MES(hei_alice_b.amount == hei_bob_b.amount
    && hei_alice_b.sha256_hash == hei_bob_b.sha256_hash
    && hei_alice_b.tx_id == hei_bob_b.tx_id, false, "hei_alice !=hei_bob ");
  
  //b) htlc refund test 
  CHECK_AND_ASSERT_MES(refund_test_instance->balance() == 0, false, "refund_test_instance->balance() == 0 failed");
  CHECK_AND_ASSERT_MES(refund_test_instance2->balance() == transfer_amount - TESTS_DEFAULT_FEE * 2, false, "refund_test_instance->balance() == transfer_amount - TESTS_DEFAULT_FEE * 2 failed");


  //=============  phase 3  =============
  //-----------  preparation  -----------
  //now alice redeem her contract in blockchain B
  alice_b_wlt_instance->redeem_htlc(hei_alice_b.tx_id, alice_origin);

  //-----------  rewinding  -----------
  r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");
  alice_a_wlt_instance->refresh();
  alice_b_wlt_instance->refresh();
  bob_a_wlt_instance->refresh();
  bob_b_wlt_instance->refresh();

  //-----------  checks  -----------
  std::list<tools::wallet_public::htlc_entry_info> htlcs_bob_a_3;
  bob_a_wlt_instance->get_list_of_active_htlc(htlcs_bob_a_3, false);
  CHECK_AND_ASSERT_MES(htlcs_bob_a_3.size() == 1, false, "htlcs_bob_a.size() == 1  failed");


  //=============  phase 4  =============
  //-----------  preparation  -----------
  std::string bob_detected_origin;
  r = bob_b_wlt_instance->check_htlc_redeemed(hei_bob_b.tx_id, bob_detected_origin);
  CHECK_AND_ASSERT_MES(r, false, "bob_a_wlt_instance->check_htlc_redeemed(hei_bob_b.tx_id, bob_detected_origin); returned false");

  CHECK_AND_ASSERT_MES(bob_detected_origin == alice_origin, false, "bob_detected_origin == alice_origin failed");
  bob_a_wlt_instance->redeem_htlc(hei_bob.tx_id, bob_detected_origin);

  //-----------  rewinding  -----------
  r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");
  alice_a_wlt_instance->refresh();
  alice_b_wlt_instance->refresh();
  bob_a_wlt_instance->refresh();
  bob_b_wlt_instance->refresh();

  //-----------  checks  -----------
  //now we have to check if all balances to make sure that atomic swap passed properly
  CHECK_AND_FORCE_ASSERT_MES(alice_a_wlt_instance->balance() == 0, false, "Incorrect balance");
  CHECK_AND_FORCE_ASSERT_MES(bob_b_wlt_instance->balance() == 0, false, "Incorrect balance");

  CHECK_AND_FORCE_ASSERT_MES(alice_b_wlt_instance->balance() == transfer_amount - TESTS_DEFAULT_FEE*2 , false, "Incorrect balance");
  CHECK_AND_FORCE_ASSERT_MES(bob_a_wlt_instance->balance() == transfer_amount - TESTS_DEFAULT_FEE*2, false, "Incorrect balance");

  return r;
}


//------------------------------------------------------------------------------
//==============================================================================
//==============================================================================
//==============================================================================

atomic_test_wrong_redeem_wrong_refund::atomic_test_wrong_redeem_wrong_refund()
{
  REGISTER_CALLBACK_METHOD(atomic_test_wrong_redeem_wrong_refund, c1);
}

bool atomic_test_wrong_redeem_wrong_refund::generate(std::vector<test_event_entry>& events) const
{
  epee::debug::get_set_enable_assert(true, true);

  currency::account_base genesis_acc;
  genesis_acc.generate();
  m_mining_accunt.generate();


  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, genesis_acc, test_core_time::get_time());
  events.push_back(blk_0);

  REWIND_BLOCKS_N(events, blk_0r, blk_0, m_mining_accunt, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 5);

  DO_CALLBACK(events, "c1");
  epee::debug::get_set_enable_assert(true, false);
  return true;
}



bool atomic_test_wrong_redeem_wrong_refund::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  /*
  we create two HTLC and then we make attempt to spend it: 
  1. spend it as refund when it supposed to be redeem 
  2. spend it as redeem when it supposed to be refund 
  
  */

  LOG_PRINT_MAGENTA("Mining Address: " << currency::get_account_address_as_str(m_mining_accunt.get_public_address()), LOG_LEVEL_0);

  //create wallet instances and calculate balances
  INIT_RUNTIME_WALLET(alice_a_wlt_instance);
  INIT_RUNTIME_WALLET(alice_b_wlt_instance);
  INIT_RUNTIME_WALLET(alice_c_wlt_instance);

#define AMOUNT_TO_TRANSFER_HTLC    (TESTS_DEFAULT_FEE*10)

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, m_mining_accunt);

  size_t blocks_fetched = 0;
  bool received_money = false;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");


  uint64_t transfer_amount = AMOUNT_TO_TRANSFER_HTLC + TESTS_DEFAULT_FEE;
  miner_wlt->transfer(transfer_amount, alice_a_wlt_instance->get_account().get_public_address());
  LOG_PRINT_MAGENTA("Transaction sent to Alice A: " << transfer_amount, LOG_LEVEL_0);

  miner_wlt->transfer(transfer_amount, alice_b_wlt_instance->get_account().get_public_address());
  LOG_PRINT_MAGENTA("Transaction sent to Alice A: " << transfer_amount, LOG_LEVEL_0);


  bool r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  alice_a_wlt_instance->refresh();
  alice_b_wlt_instance->refresh();
  alice_c_wlt_instance->refresh();

  std::string alice_origin; //will be deterministically generated by Alice's A wallet
  currency::transaction res_tx = AUTO_VAL_INIT(res_tx);
  alice_a_wlt_instance->create_htlc_proposal(transfer_amount - TESTS_DEFAULT_FEE, alice_b_wlt_instance->get_account().get_public_address(), 9, res_tx, currency::null_hash, alice_origin);

  std::string alice_origin_b; //will be deterministically generated by Alice's A wallet
  currency::transaction res_tx_b = AUTO_VAL_INIT(res_tx_b);
  alice_b_wlt_instance->create_htlc_proposal(transfer_amount - TESTS_DEFAULT_FEE, alice_c_wlt_instance->get_account().get_public_address(), 12, res_tx, currency::null_hash, alice_origin_b);


  //forward blockchain to create redeem transaction
  r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  alice_a_wlt_instance->refresh();
  alice_b_wlt_instance->refresh();
  alice_c_wlt_instance->refresh();

  CHECK_AND_FORCE_ASSERT_MES(alice_a_wlt_instance->balance() == transfer_amount - TESTS_DEFAULT_FEE, false, "Incorrect balance");

  //create refund tx
  currency::transaction refund_tx = AUTO_VAL_INIT(refund_tx);
  alice_a_wlt_instance->transfer(transfer_amount - TESTS_DEFAULT_FEE*2, alice_b_wlt_instance->get_account().get_public_address(), refund_tx);
  
  //create redeem tx
  std::list<tools::wallet_public::htlc_entry_info> htlcs;
  alice_c_wlt_instance->get_list_of_active_htlc(htlcs, true);
  CHECK_AND_FORCE_ASSERT_MES(htlcs.size() == 1, false, "Epected htlc not found");
  currency::transaction result_redeem_tx = AUTO_VAL_INIT(result_redeem_tx);
  alice_c_wlt_instance->redeem_htlc(htlcs.front().tx_id, alice_origin_b, result_redeem_tx);

  c.get_blockchain_storage().truncate_blockchain(c.get_blockchain_storage().get_current_blockchain_size() - 8);
  
  //try to submit wrong transaction that do refund before expiration
  std::vector<currency::transaction> txs;
  txs.push_back(refund_tx);
  r = mine_next_pow_block_in_playtime_with_given_txs(m_mining_accunt.get_public_address(), c, txs);
  CHECK_AND_FORCE_ASSERT_MES(!r, false, "Blo k with wrong refund tx accepted");

  //forward blockchain and try to submit wrong tx that do redeem after expiration 
  r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  std::vector<currency::transaction> txs2;
  txs2.push_back(result_redeem_tx);
  r = mine_next_pow_block_in_playtime_with_given_txs(m_mining_accunt.get_public_address(), c, txs2);
  CHECK_AND_FORCE_ASSERT_MES(!r, false, "Blo k with wrong refund tx accepted");


  return true;
}

//try to do double spend!!!! (redeem and refund), check if wallet see redeemed tx and don't let it be spent