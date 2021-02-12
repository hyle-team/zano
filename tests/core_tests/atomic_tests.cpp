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

  currency::account_base accunt_alice_blockchain_a;
  currency::account_base accunt_alice_blockchain_b;
  currency::account_base accunt_bob_blockchain_a;
  currency::account_base accunt_bob_blockchain_b;
  accunt_alice_blockchain_a.generate();
  accunt_alice_blockchain_b.generate();
  accunt_bob_blockchain_a.generate();
  accunt_bob_blockchain_b.generate();

  LOG_PRINT_MAGENTA("Mining Address: " << currency::get_account_address_as_str(m_mining_accunt.get_public_address()), LOG_LEVEL_0);
  LOG_PRINT_MAGENTA("Alice [A] Address: " << currency::get_account_address_as_str(accunt_alice_blockchain_a.get_public_address()), LOG_LEVEL_0);
  LOG_PRINT_MAGENTA("Alice [B] Address: " << currency::get_account_address_as_str(accunt_alice_blockchain_b.get_public_address()), LOG_LEVEL_0);
  LOG_PRINT_MAGENTA("Bob [A] Address: " << currency::get_account_address_as_str(accunt_bob_blockchain_a.get_public_address()), LOG_LEVEL_0);
  LOG_PRINT_MAGENTA("Bob [B] Address: " << currency::get_account_address_as_str(accunt_bob_blockchain_b.get_public_address()), LOG_LEVEL_0);

#define AMOUNT_TO_TRANSFER_HTLC    (TESTS_DEFAULT_FEE*10)

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, m_mining_accunt);

  size_t blocks_fetched = 0;
  bool received_money = false;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  uint64_t transfer_amount = AMOUNT_TO_TRANSFER_HTLC + TESTS_DEFAULT_FEE;
  miner_wlt->transfer(transfer_amount, accunt_alice_blockchain_a.get_public_address());
  LOG_PRINT_MAGENTA("Transaction sent to Alice A: " << transfer_amount, LOG_LEVEL_0);

  miner_wlt->transfer(transfer_amount, accunt_bob_blockchain_b.get_public_address());
  LOG_PRINT_MAGENTA("Transaction sent to Bob B: " << transfer_amount, LOG_LEVEL_0);

  bool r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  //create wallet instances and calculate balances
  std::shared_ptr<tools::wallet2> alice_a_wlt_instance = init_playtime_test_wallet(events, c, accunt_alice_blockchain_a);
  std::shared_ptr<tools::wallet2> alice_b_wlt_instance = init_playtime_test_wallet(events, c, accunt_alice_blockchain_b);
  std::shared_ptr<tools::wallet2> bob_a_wlt_instance = init_playtime_test_wallet(events, c, accunt_bob_blockchain_a);
  std::shared_ptr<tools::wallet2> bob_b_wlt_instance = init_playtime_test_wallet(events, c, accunt_bob_blockchain_b);
//  std::shared_ptr<wallet_tests_callback_handler> backend_mock(new wallet_tests_callback_handler());
//  alice_a_wlt_instance->callback(backend_mock);

  alice_a_wlt_instance->refresh();
  alice_b_wlt_instance->refresh();
  bob_a_wlt_instance->refresh();
  bob_b_wlt_instance->refresh();

  CHECK_AND_FORCE_ASSERT_MES(alice_a_wlt_instance->balance() == transfer_amount, false, "Incorrect balance");
  CHECK_AND_FORCE_ASSERT_MES(bob_b_wlt_instance->balance() == transfer_amount, false, "Incorrect balance");


  std::string alice_origin; //will be deterministically generated by Alice's A wallet
  currency::transaction res_tx = AUTO_VAL_INIT(res_tx);
  alice_a_wlt_instance->create_htlc_proposal(transfer_amount - TESTS_DEFAULT_FEE, bob_a_wlt_instance->get_account().get_public_address(), 20, res_tx, currency::null_hash, alice_origin);
  r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");
  alice_a_wlt_instance->refresh();
  alice_b_wlt_instance->refresh();
  bob_a_wlt_instance->refresh();
  bob_b_wlt_instance->refresh();

  std::list<wallet_public::htlc_entry_info> htlcs_alice_a;
  alice_a_wlt_instance->get_list_of_active_htlc(htlcs_alice_a, false);
  CHECK_AND_ASSERT_MES(htlcs_alice_a.size() == 1, false, "htlcs_alice.size() == 1 failed");

  std::list<wallet_public::htlc_entry_info> htlcs_bob_a;
  bob_a_wlt_instance->get_list_of_active_htlc(htlcs_bob_a, false);
  CHECK_AND_ASSERT_MES(htlcs_bob_a.size() == 1, false, "htlcs_bob.size() == 1  failed");

  const wallet_public::htlc_entry_info& hei_bob = *htlcs_bob_a.begin();
  CHECK_AND_ASSERT_MES(hei_bob.is_redeem == true, false, "hei_bob.is_redeem == true failed");


  const wallet_public::htlc_entry_info& hei_alice = *htlcs_alice_a.begin();
  CHECK_AND_ASSERT_MES(hei_alice.is_redeem == false, false, "hei_alice.is_redeem == false failed");

  CHECK_AND_ASSERT_MES(hei_alice.amount == hei_bob.amount 
    && hei_alice.sha256_hash == hei_bob.sha256_hash
    && hei_alice.tx_id == hei_bob.tx_id, false, "hei_alice !=hei_bob ");


  //std::string bob_origin; 
  currency::transaction res_bob_tx = AUTO_VAL_INIT(res_tx);
  std::string dummy_origin;
  bob_b_wlt_instance->create_htlc_proposal(transfer_amount - TESTS_DEFAULT_FEE, 
    alice_b_wlt_instance->get_account().get_public_address(),
    20, 
    res_tx, hei_bob.sha256_hash, dummy_origin);

  r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");
  alice_a_wlt_instance->refresh();
  alice_b_wlt_instance->refresh();
  bob_a_wlt_instance->refresh();
  bob_b_wlt_instance->refresh();


  std::list<wallet_public::htlc_entry_info> htlcs_alice_b;
  alice_b_wlt_instance->get_list_of_active_htlc(htlcs_alice_b, false);
  CHECK_AND_ASSERT_MES(htlcs_alice_b.size() == 1, false, "htlcs_alice_b.size() == 1 failed");

  std::list<wallet_public::htlc_entry_info> htlcs_bob_b;
  bob_a_wlt_instance->get_list_of_active_htlc(htlcs_bob_b, false);
  CHECK_AND_ASSERT_MES(htlcs_bob_b.size() == 1, false, "htlcs_bob_b.size() == 1  failed");

  const wallet_public::htlc_entry_info& hei_bob_b = *htlcs_bob_b.begin();
  CHECK_AND_ASSERT_MES(hei_bob_b.is_redeem == true, false, "hei_bob.is_redeem == true failed");


  const wallet_public::htlc_entry_info& hei_alice_b = *htlcs_alice_b.begin();
  CHECK_AND_ASSERT_MES(hei_alice_b.is_redeem == false, false, "hei_alice.is_redeem == false failed");

  CHECK_AND_ASSERT_MES(hei_alice_b.amount == hei_bob_b.amount
    && hei_alice_b.sha256_hash == hei_bob_b.sha256_hash
    && hei_alice_b.tx_id == hei_bob_b.tx_id, false, "hei_alice !=hei_bob ");

  return r;
}


//------------------------------------------------------------------------------

