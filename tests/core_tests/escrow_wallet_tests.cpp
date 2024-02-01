// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "escrow_wallet_tests.h"
#include "random_helper.h"
#include "chaingen_helpers.h"
#include "escrow_wallet_common.h"

using namespace epee;
using namespace crypto;
using namespace currency;

//==============================================================================================================================

struct wallet_tests_callback_handler : public tools::i_wallet2_callback
{
  virtual void on_transfer2(const tools::wallet_public::wallet_transfer_info& wti, const std::list<tools::wallet_public::asset_balance_entry>& balances, uint64_t total_mined)
  {
    all_wtis.push_back(wti);
  }

  std::vector<tools::wallet_public::wallet_transfer_info> all_wtis;
};

escrow_wallet_test::escrow_wallet_test()
{
  REGISTER_CALLBACK_METHOD(escrow_wallet_test, c1);
}

bool escrow_wallet_test::generate(std::vector<test_event_entry>& events) const
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

bool escrow_wallet_test::prepare_proposal_accepted_test(currency::core& c, const std::vector<test_event_entry>& events, std::shared_ptr<tools::wallet2>& wallet_buyer, std::shared_ptr<tools::wallet2>& wallet_seller, crypto::hash& multisig_id)
{
  currency::account_base accunt_buyer;
  currency::account_base accunt_seller;
  accunt_buyer.generate();
  accunt_seller.generate();


  LOG_PRINT_MAGENTA("Miner Address: " << currency::get_account_address_as_str(m_mining_accunt.get_public_address()), LOG_LEVEL_0);
  LOG_PRINT_MAGENTA("Buyer Address: " << currency::get_account_address_as_str(accunt_buyer.get_public_address()), LOG_LEVEL_0);
  LOG_PRINT_MAGENTA("Seller Address: " << currency::get_account_address_as_str(accunt_seller.get_public_address()), LOG_LEVEL_0);


#define AMOUNT_TO_TRANSFER_ESCROW    (TESTS_DEFAULT_FEE*10)
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, m_mining_accunt);

  size_t blocks_fetched = 0;
  bool received_money = false;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  //  CHECK_AND_FORCE_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW, false, "Incorrect numbers of blocks fetched");
  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  miner_wlt->transfer(AMOUNT_TO_TRANSFER_ESCROW * 2 + TESTS_DEFAULT_FEE, accunt_buyer.get_public_address());
  LOG_PRINT_MAGENTA("Transaction sent to buyer_account: " << AMOUNT_TO_TRANSFER_ESCROW * 2 + TESTS_DEFAULT_FEE, LOG_LEVEL_0);

  miner_wlt->transfer(AMOUNT_TO_TRANSFER_ESCROW + TESTS_DEFAULT_FEE * 2, accunt_seller.get_public_address());
  LOG_PRINT_MAGENTA("Transaction sent to seller_account: " << AMOUNT_TO_TRANSFER_ESCROW + TESTS_DEFAULT_FEE * 2, LOG_LEVEL_0);

  bool r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");


  wallet_buyer = init_playtime_test_wallet(events, c, accunt_buyer);
  wallet_seller = init_playtime_test_wallet(events, c, accunt_seller);
  std::shared_ptr<wallet_tests_callback_handler> buyer_backend_mock(new wallet_tests_callback_handler());
  std::shared_ptr<wallet_tests_callback_handler> seller_backend_mock(new wallet_tests_callback_handler());
  wallet_buyer->callback(buyer_backend_mock);
  wallet_seller->callback(seller_backend_mock);
  wallet_buyer->refresh();
  wallet_seller->refresh();
  CHECK_AND_FORCE_ASSERT_MES(wallet_buyer->balance() == AMOUNT_TO_TRANSFER_ESCROW * 2 + TESTS_DEFAULT_FEE, false, "Incorrect balance");
  CHECK_AND_FORCE_ASSERT_MES(wallet_seller->balance() == AMOUNT_TO_TRANSFER_ESCROW + TESTS_DEFAULT_FEE * 2, false, "Incorrect balance");


  bc_services::contract_private_details cpd = AUTO_VAL_INIT(cpd);
  transaction escrow_proposal_tx = AUTO_VAL_INIT(escrow_proposal_tx);
  transaction escrow_template_tx = AUTO_VAL_INIT(escrow_template_tx);
  cpd.a_addr = wallet_buyer->get_account().get_public_address();
  cpd.b_addr = wallet_seller->get_account().get_public_address();
  cpd.amount_a_pledge = AMOUNT_TO_TRANSFER_ESCROW;
  cpd.amount_b_pledge = AMOUNT_TO_TRANSFER_ESCROW;
  cpd.amount_to_pay = AMOUNT_TO_TRANSFER_ESCROW;
  cpd.comment = "I'll build by own theme park. With black jack, and hookers.";
  cpd.title = "Afterlife? If I thought I had to live another life, I'd kill myself right now!";
  wallet_buyer->send_escrow_proposal(cpd, 0, 0, 3600, TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, "", escrow_proposal_tx, escrow_template_tx);

  auto it = std::find_if(escrow_template_tx.vout.begin(), escrow_template_tx.vout.end(), [](const tx_out_v& o){
    if (boost::get<tx_out_bare>(o).target.type() == typeid(txout_multisig))
      return true;
    return false;
  });

  CHECK_AND_FORCE_ASSERT_MES(it != escrow_template_tx.vout.end(), false, "wrong escrow_template_tx");
  size_t multisig_i = it - escrow_template_tx.vout.begin();
  multisig_id = currency::get_multisig_out_id(escrow_template_tx, multisig_i);

  LOG_PRINT_MAGENTA("Escrow proposal sent bseller_account:  escrow_proposal_tx id: " << currency::get_transaction_hash(escrow_proposal_tx) << ", multisig_id: " << multisig_id, LOG_LEVEL_0);


  r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, 2);

  wallet_buyer->refresh();
  wallet_seller->refresh();

  tools::escrow_contracts_container contracts_buyer, contracts_seller;
  wallet_buyer->get_contracts(contracts_buyer);
  wallet_seller->get_contracts(contracts_seller);

  CHECK_AND_FORCE_ASSERT_MES(contracts_buyer.size() == 1, false, "Incorrect contracts_buyer state");
  CHECK_AND_FORCE_ASSERT_MES(contracts_seller.size() == 1, false, "Incorrect contracts_seller state");

  CHECK_AND_FORCE_ASSERT_MES(contracts_buyer.begin()->second.is_a == true, false, "Incorrect contracts_buyer state");
  CHECK_AND_FORCE_ASSERT_MES(contracts_seller.begin()->second.is_a == false, false, "Incorrect contracts_seller state");

  CHECK_AND_FORCE_ASSERT_MES(contracts_buyer.begin()->second.state == tools::wallet_public::escrow_contract_details_basic::proposal_sent, false, "Incorrect contracts_buyer state");
  CHECK_AND_FORCE_ASSERT_MES(contracts_seller.begin()->second.state == tools::wallet_public::escrow_contract_details_basic::proposal_sent, false, "Incorrect contracts_seller state");

  CHECK_AND_FORCE_ASSERT_MES(contracts_buyer.begin()->first == multisig_id, false, "Incorrect contracts_buyer state");
  CHECK_AND_FORCE_ASSERT_MES(contracts_seller.begin()->first == multisig_id, false, "Incorrect contracts_seller state");

  //----------------------
  // accept proposal
  //----------------------
  wallet_seller->accept_proposal(multisig_id, TESTS_DEFAULT_FEE);

  r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  wallet_buyer->refresh();
  wallet_seller->refresh();
  wallet_buyer->get_contracts(contracts_buyer);
  wallet_seller->get_contracts(contracts_seller);

  CHECK_AND_FORCE_ASSERT_MES(contracts_buyer.size() == 1, false, "Incorrect contracts_buyer state");
  CHECK_AND_FORCE_ASSERT_MES(contracts_seller.size() == 1, false, "Incorrect contracts_seller state");

  CHECK_AND_FORCE_ASSERT_MES(contracts_buyer.begin()->second.is_a == true, false, "Incorrect contracts_buyer state");
  CHECK_AND_FORCE_ASSERT_MES(contracts_seller.begin()->second.is_a == false, false, "Incorrect contracts_seller state");

  CHECK_AND_FORCE_ASSERT_MES(contracts_buyer.begin()->second.state == tools::wallet_public::escrow_contract_details_basic::contract_accepted, false, "Incorrect contracts_buyer state");
  CHECK_AND_FORCE_ASSERT_MES(contracts_seller.begin()->second.state == tools::wallet_public::escrow_contract_details_basic::contract_accepted, false, "Incorrect contracts_seller state");

  tools::multisig_transfer_container  ms_buyer, ms_seller;
  wallet_buyer->get_multisig_transfers(ms_buyer);
  wallet_seller->get_multisig_transfers(ms_seller);
  CHECK_AND_FORCE_ASSERT_MES(ms_buyer.size() == 1, false, "Incorrect contracts_buyer state");
  CHECK_AND_FORCE_ASSERT_MES(ms_seller.size() == 1, false, "Incorrect contracts_seller state");
  return true;
}


bool escrow_wallet_test::exec_test_with_specific_release_type(currency::core& c, const std::vector<test_event_entry>& events, bool release_normal)
{
  std::shared_ptr<tools::wallet2> wallet_buyer;
  std::shared_ptr<tools::wallet2> wallet_seller;
  crypto::hash multisig_id = null_hash;
  bool r = prepare_proposal_accepted_test(c, events, wallet_buyer, wallet_seller, multisig_id);
  CHECK_AND_FORCE_ASSERT_MES(r, false, "Failed to prepare_proposal_accepted_test");
  //----------------------
  // release contract
  //----------------------
  std::string release_instruction;
  tools::wallet_public::escrow_contract_details_basic::contract_state expected_state = AUTO_VAL_INIT(expected_state);
  if (release_normal)
  {
    release_instruction = BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL;
    expected_state = tools::wallet_public::escrow_contract_details_basic::contract_released_normal;
  }
  else
  {
    release_instruction = BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_BURN;
    expected_state = tools::wallet_public::escrow_contract_details_basic::contract_released_burned;
  }

  tools::escrow_contracts_container contracts_buyer, contracts_seller;
  wallet_buyer->finish_contract(multisig_id, release_instruction);
  r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, 2);
  wallet_buyer->refresh();
  wallet_seller->refresh();
  wallet_buyer->get_contracts(contracts_buyer);
  wallet_seller->get_contracts(contracts_seller);

  CHECK_AND_FORCE_ASSERT_MES(contracts_buyer.size() == 1, false, "Incorrect contracts_buyer state");
  CHECK_AND_FORCE_ASSERT_MES(contracts_seller.size() == 1, false, "Incorrect contracts_seller state");

  CHECK_AND_FORCE_ASSERT_MES(contracts_buyer.begin()->second.is_a == true, false, "Incorrect contracts_buyer state");
  CHECK_AND_FORCE_ASSERT_MES(contracts_seller.begin()->second.is_a == false, false, "Incorrect contracts_seller state");

  CHECK_AND_FORCE_ASSERT_MES(contracts_buyer.begin()->second.state == expected_state, false, "Incorrect contracts_buyer state");
  CHECK_AND_FORCE_ASSERT_MES(contracts_seller.begin()->second.state == expected_state, false, "Incorrect contracts_seller state");

  return true;
}

bool escrow_wallet_test::exec_test_with_cancel_release_type(currency::core& c, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> wallet_buyer;
  std::shared_ptr<tools::wallet2> wallet_seller;
  std::shared_ptr<tools::wallet2> wallet_miner;
  crypto::hash multisig_id = null_hash;
  bool r = prepare_proposal_accepted_test(c, events, wallet_buyer, wallet_seller, multisig_id);
  CHECK_AND_FORCE_ASSERT_MES(r, false, "Failed to prepare_proposal_accepted_test");

  //----------------------
  // request cancel contract
  //----------------------
  wallet_miner = init_playtime_test_wallet(events, c, m_mining_accunt);
  wallet_miner->refresh();
  wallet_miner->transfer(TESTS_DEFAULT_FEE, wallet_buyer->get_account().get_public_address());
  r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, 10);
  wallet_buyer->refresh();
  tools::escrow_contracts_container contracts_buyer, contracts_seller;
  wallet_buyer->request_cancel_contract(multisig_id, TESTS_DEFAULT_FEE, 60 * 60);
  r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, 2);
  wallet_buyer->refresh();
  wallet_seller->refresh();
  wallet_buyer->get_contracts(contracts_buyer);
  wallet_seller->get_contracts(contracts_seller);

  CHECK_AND_FORCE_ASSERT_MES(contracts_buyer.size() == 1, false, "Incorrect contracts_buyer state");
  CHECK_AND_FORCE_ASSERT_MES(contracts_seller.size() == 1, false, "Incorrect contracts_seller state");

  CHECK_AND_FORCE_ASSERT_MES(contracts_buyer.begin()->second.is_a == true, false, "Incorrect contracts_buyer state");
  CHECK_AND_FORCE_ASSERT_MES(contracts_seller.begin()->second.is_a == false, false, "Incorrect contracts_seller state");

  CHECK_AND_FORCE_ASSERT_MES(contracts_buyer.begin()->second.state == tools::wallet_public::escrow_contract_details_basic::contract_cancel_proposal_sent, false, "Incorrect contracts_buyer state");
  CHECK_AND_FORCE_ASSERT_MES(contracts_seller.begin()->second.state == tools::wallet_public::escrow_contract_details_basic::contract_cancel_proposal_sent, false, "Incorrect contracts_seller state");

  //cancel contract
  wallet_seller->accept_cancel_contract(multisig_id);

  r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, 2);
  wallet_buyer->refresh();
  wallet_seller->refresh();
  wallet_buyer->get_contracts(contracts_buyer);
  wallet_seller->get_contracts(contracts_seller);

  CHECK_AND_FORCE_ASSERT_MES(contracts_buyer.size() == 1, false, "Incorrect contracts_buyer state");
  CHECK_AND_FORCE_ASSERT_MES(contracts_seller.size() == 1, false, "Incorrect contracts_seller state");

  CHECK_AND_FORCE_ASSERT_MES(contracts_buyer.begin()->second.is_a == true, false, "Incorrect contracts_buyer state");
  CHECK_AND_FORCE_ASSERT_MES(contracts_seller.begin()->second.is_a == false, false, "Incorrect contracts_seller state");

  CHECK_AND_FORCE_ASSERT_MES(contracts_buyer.begin()->second.state == tools::wallet_public::escrow_contract_details_basic::contract_released_cancelled, false, "Incorrect contracts_buyer state");
  CHECK_AND_FORCE_ASSERT_MES(contracts_seller.begin()->second.state == tools::wallet_public::escrow_contract_details_basic::contract_released_cancelled, false, "Incorrect contracts_seller state");
  return true;
}


bool escrow_wallet_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  epee::debug::get_set_enable_assert(true, true);
  misc_utils::auto_scope_leave_caller scope_exit_handler = misc_utils::create_scope_leave_handler([&](){epee::debug::get_set_enable_assert(true, false); });

  bool r = exec_test_with_cancel_release_type(c, events);
  if (!r)
    return false;

  r = exec_test_with_specific_release_type(c, events, true);
  if (!r)
    return false;

  r = exec_test_with_specific_release_type(c, events, false);
  if (!r)
    return false;

  epee::debug::get_set_enable_assert(true, false);
  return r;
}


//------------------------------------------------------------------------------

escrow_w_and_fake_outputs::escrow_w_and_fake_outputs()
  : m_pledge_amount(TESTS_DEFAULT_FEE * 9)
  , m_alice_bob_start_chunk_amount(0)
{
  REGISTER_CALLBACK_METHOD(escrow_w_and_fake_outputs, c1);

  // uncomment the following to switch on deterministic mode if needed for debugging
  // random_state_test_restorer::reset_random();
}

bool escrow_w_and_fake_outputs::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: perform a simple escrow having fake outs in inputs

  uint64_t ts = 145000000;

  GENERATE_ACCOUNT(preminer_acc);
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc =   m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);
  account_base& carol_acc = m_accounts[CAROL_ACC_IDX]; carol_acc.generate(); carol_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, ts);
  ADJUST_TEST_CORE_TIME(ts);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 10);

  // transfer some coins to Alice and Bob
  uint64_t stub = 0;
  bool r = calculate_amounts_many_outs_have_and_no_outs_have(get_outs_money_amount(blk_0r.miner_tx), m_alice_bob_start_chunk_amount, stub);
  CHECK_AND_ASSERT_MES(r, false, "calculate_amounts_many_outs_have_and_no_outs_have failed");
  MAKE_TX(events, tx_1, miner_acc, alice_acc, m_alice_bob_start_chunk_amount, blk_0r);
  MAKE_TX(events, tx_11, miner_acc, alice_acc, m_alice_bob_start_chunk_amount, blk_0r);
  MAKE_TX(events, tx_2, miner_acc, bob_acc, m_alice_bob_start_chunk_amount, blk_0r);
  MAKE_TX(events, tx_22, miner_acc, bob_acc, m_alice_bob_start_chunk_amount, blk_0r);

  // miner -> miner txs just to make more available outputs to mixin
  MAKE_TX(events, tx_3, miner_acc, miner_acc, m_pledge_amount, blk_0r);
  MAKE_TX(events, tx_4, miner_acc, miner_acc, m_pledge_amount, blk_0r);
  MAKE_TX(events, tx_5, miner_acc, miner_acc, m_pledge_amount, blk_0r);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({ tx_1, tx_11, tx_2, tx_22, tx_3, tx_4, tx_5 }));

  // rewind a little more to made all txs' outs are spendable from wallets
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

bool escrow_w_and_fake_outputs::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // ADVICE: if the test fails, try to set fake_outs_count to zero in order to check whether it depends on fake outs
  size_t fake_outs_count = 4;

  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  bob_wlt->refresh();

  uint64_t alice_start_balance = alice_wlt->balance();
  CHECK_AND_ASSERT_MES(alice_start_balance == 2 * m_alice_bob_start_chunk_amount, false, "Invalid Alice start balance: " << alice_start_balance << ", expected: " << 2 * m_alice_bob_start_chunk_amount);
  uint64_t bob_start_balance = bob_wlt->balance();
  CHECK_AND_ASSERT_MES(bob_start_balance == 2 * m_alice_bob_start_chunk_amount, false, "Invalid Bob start balance: " << bob_start_balance << ", expected: " << 2 * m_alice_bob_start_chunk_amount);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  bc_services::contract_private_details cpd = AUTO_VAL_INIT(cpd);
  cpd.amount_a_pledge = m_pledge_amount;
  cpd.amount_b_pledge = m_pledge_amount;
  cpd.amount_to_pay = TESTS_DEFAULT_FEE * 7;
  cpd.a_addr = m_accounts[ALICE_ACC_IDX].get_public_address();
  cpd.b_addr = m_accounts[BOB_ACC_IDX].get_public_address();
  cpd.comment = get_random_text(1024);
  cpd.title = get_random_text(7);

  transaction proposal_tx = AUTO_VAL_INIT(proposal_tx);
  transaction escrow_template_tx = AUTO_VAL_INIT(escrow_template_tx);
  test_core_time::adjust(boost::get<block>(events[ev_index - 1]).timestamp + 5); // determenistic time, 'cause escrow template uses current time => time affects hashes
  alice_wlt->send_escrow_proposal(cpd, fake_outs_count, 0, 0, TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, "", proposal_tx, escrow_template_tx);
  uint64_t alice_post_proposal_balance = alice_wlt->balance();
  uint64_t alice_spent_for_proposal = alice_start_balance - alice_post_proposal_balance;
  uint64_t alice_expected_spends_for_proposal = TESTS_DEFAULT_FEE;
  CHECK_AND_ASSERT_MES(alice_spent_for_proposal == alice_expected_spends_for_proposal, false, "Incorrect Alice post-proposal balance: " << alice_post_proposal_balance << ENDL <<
    "Alice has spent for sending proposal: " << alice_spent_for_proposal << ENDL <<
    "Alice should spend for sending proposal: " << alice_expected_spends_for_proposal << " (pledge: " << cpd.amount_a_pledge << ", amount: " << cpd.amount_to_pay << ", fee: " << TESTS_DEFAULT_FEE << ")");
  CHECK_AND_ASSERT_MES(check_wallet_balance_blocked_for_escrow(*alice_wlt.get(), "Alice", cpd.amount_a_pledge + cpd.amount_to_pay), false, "");  

  // check that inputs contain correct number of fake mix-ins
  size_t nmix = 0;
  r = are_all_inputs_mixed(proposal_tx.vin, &nmix);
  CHECK_AND_ASSERT_MES(r && nmix == fake_outs_count, false, "proposal_tx inputs are not mixed-in correctly: " << nmix);
  r = are_all_inputs_mixed(escrow_template_tx.vin, &nmix);
  CHECK_AND_ASSERT_MES(r && nmix == fake_outs_count, false, "escrow_template_tx inputs are not mixed-in correctly: " << nmix);

  crypto::hash ms_id = get_multisig_out_id(escrow_template_tx, get_multisig_out_index(escrow_template_tx.vout));
  CHECK_AND_ASSERT_MES(ms_id != null_hash, false, "Can't obtain multisig id from escrow template tx");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  bob_wlt->refresh();
  bob_wlt->accept_proposal(ms_id, TESTS_DEFAULT_FEE);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  alice_wlt->refresh();
  alice_wlt->finish_contract(ms_id, BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, WALLET_DEFAULT_TX_SPENDABLE_AGE);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  alice_wlt->reset_history(); // just for watching full wallet's history in the log on refresh
  alice_wlt->refresh();
  // Alice pays fee ones: for sending a proposal
  uint64_t alice_estimated_end_balance = alice_start_balance - cpd.amount_to_pay - TESTS_DEFAULT_FEE * 1;
  if (!check_balance_via_wallet(*alice_wlt.get(), "alice", alice_estimated_end_balance, 0, alice_estimated_end_balance, 0, 0))
    return false;

  bob_wlt->reset_history(); // just for watching full wallet's history in the log on refresh
  bob_wlt->refresh();
  // Bob pays double fee: one for accepting prorpsal and another for finishing contract
  uint64_t bob_estimated_end_balance = bob_start_balance + cpd.amount_to_pay - TESTS_DEFAULT_FEE * 2;
  if (!check_balance_via_wallet(*bob_wlt.get(), "bob", bob_estimated_end_balance, 0, bob_estimated_end_balance, 0, 0))
    return false;
  
  // withdraw all money from Bob and Alice accounts to make sure they're spendadble
  alice_wlt->transfer(alice_estimated_end_balance - TESTS_DEFAULT_FEE, m_accounts[CAROL_ACC_IDX].get_public_address());
  bob_wlt->transfer(bob_estimated_end_balance - TESTS_DEFAULT_FEE, m_accounts[CAROL_ACC_IDX].get_public_address());

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 2, false, "Incorrect txs count in the pool");

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  // make sure money was successfully transferred
  std::shared_ptr<tools::wallet2> carol_wlt = init_playtime_test_wallet(events, c, CAROL_ACC_IDX);
  carol_wlt->refresh();
  if (!check_balance_via_wallet(*carol_wlt.get(), "carol", alice_estimated_end_balance + bob_estimated_end_balance - TESTS_DEFAULT_FEE * 2))
    return false;

  // and make sure Alice's and Bob's wallets are empty
  alice_wlt->refresh();
  if (!check_balance_via_wallet(*alice_wlt.get(), "alice", 0, 0, 0, 0, 0))
    return false;

  bob_wlt->refresh();
  if (!check_balance_via_wallet(*bob_wlt.get(), "bob", 0, 0, 0, 0, 0))
    return false;

  return true;
}


//------------------------------------------------------------------------------

escrow_incorrect_proposal::escrow_incorrect_proposal()
{
  REGISTER_CALLBACK_METHOD(escrow_incorrect_proposal, check_normal_proposal);
  REGISTER_CALLBACK_METHOD(escrow_incorrect_proposal, check_incorrect_proposal);
}

bool escrow_incorrect_proposal::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = 145000000;

  GENERATE_ACCOUNT(preminer_acc);
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, ts);
  ADJUST_TEST_CORE_TIME(ts);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  std::vector<tx_source_entry> used_sources;
  bc_services::contract_private_details cpd = AUTO_VAL_INIT(cpd);
  cpd.amount_a_pledge = TESTS_DEFAULT_FEE * 12;
  cpd.amount_b_pledge = TESTS_DEFAULT_FEE * 13;
  cpd.amount_to_pay = TESTS_DEFAULT_FEE * 3;
  cpd.a_addr = miner_acc.get_public_address();
  cpd.b_addr = alice_acc.get_public_address();

  MAKE_TX(events, tx_1, miner_acc, alice_acc, cpd.amount_b_pledge + TESTS_DEFAULT_FEE * 2, blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  bool r = false;

  // 1. create normal proposal and make sure it goes correctly through out the whole mechanism (test self-check)
  // if it fails, it means build_custom_escrow_proposal() produced incorrect proposal
  uint64_t tx_version = get_tx_version(get_block_height(blk_1r),  m_hardforks);
  uint64_t normal_escrow_mask = eccf_proposal_additional_attach | eccf_template_additional_extra | eccf_template_additional_attach;
  transaction normal_escrow_proposal_tx = AUTO_VAL_INIT(normal_escrow_proposal_tx);
  r = build_custom_escrow_proposal(events, blk_1r, miner_acc.get_keys(), cpd, 0, 0, 0, blk_1r.timestamp + 3600, 0, TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, normal_escrow_mask, tx_version, normal_escrow_proposal_tx, used_sources);
  CHECK_AND_ASSERT_MES(r, false, "build_custom_escrow_proposal failed");

  events.push_back(normal_escrow_proposal_tx);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, normal_escrow_proposal_tx);

  // note: two blocks will be mined in playtime during this event:
  DO_CALLBACK(events, "check_normal_proposal");

  // rewind blocks, so effectivly we make an alt chain here and then switch to it - escrow proposal should be moved to tx pool
  REWIND_BLOCKS_N_WITH_TIME(events, blk_3, blk_1r, miner_acc, 4);

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_3), get_block_hash(blk_3)));
  DO_CALLBACK(events, "clear_tx_pool"); // get rid of previous proposal and txs
  used_sources.clear();

  // 2. Create incorrects proposals and push them to the core.
  // It is expected, that Alice should not receive any.

#define ADD_INCORRECT_PROPOSAL_MASK(mask, invalid_tx_flag)  { mask, #mask, invalid_tx_flag }
  struct { uint64_t mask; const char* name; bool invalid_tx_flag; } custom_config_masks[] = {
    ADD_INCORRECT_PROPOSAL_MASK(eccf_template_inv_crypt_address,              0),
    ADD_INCORRECT_PROPOSAL_MASK(eccf_template_inv_a_inputs,                   0),
    ADD_INCORRECT_PROPOSAL_MASK(eccf_template_inv_sa_instruction,             0),
    ADD_INCORRECT_PROPOSAL_MASK(eccf_template_inv_sa_flags,                   0),
    ADD_INCORRECT_PROPOSAL_MASK(eccf_template_no_multisig,                    0),
    ADD_INCORRECT_PROPOSAL_MASK(eccf_template_inv_multisig,                   0),
    ADD_INCORRECT_PROPOSAL_MASK(eccf_template_inv_multisig_2,                 0),
    ADD_INCORRECT_PROPOSAL_MASK(eccf_template_inv_multisig_low_min_sig,       0),
    //ADD_INCORRECT_PROPOSAL_MASK(eccf_template_no_a_sigs,                      0),  TODO: is this correct and necessary check?
    ADD_INCORRECT_PROPOSAL_MASK(eccf_template_no_tx_flags,                    0),
    ADD_INCORRECT_PROPOSAL_MASK(eccf_template_more_than_1_multisig,           0),
    ADD_INCORRECT_PROPOSAL_MASK(eccf_template_inv_ms_amount,                  0),
    ADD_INCORRECT_PROPOSAL_MASK(eccf_proposal_inv_sa_flags,                   0),
    ADD_INCORRECT_PROPOSAL_MASK(eccf_proposal_sa_empty_body,                  0),
    ADD_INCORRECT_PROPOSAL_MASK(eccf_proposal_inv_flags,                      true),
  };
  
  block top_block = blk_3;
  for (size_t i = 0; i < sizeof custom_config_masks / sizeof custom_config_masks[0]; ++i)
  {
    uint64_t config_mask = custom_config_masks[i].mask;
    cpd.comment = "#" + std::to_string(i) + " " + custom_config_masks[i].name;
    
    tx_version = get_tx_version(get_block_height(top_block), m_hardforks);
    transaction escrow_proposal_tx = AUTO_VAL_INIT(escrow_proposal_tx);
    r = build_custom_escrow_proposal(events, top_block, miner_acc.get_keys(), cpd, 0, 0, 0, top_block.timestamp + 3600, 0, TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, config_mask, tx_version, escrow_proposal_tx, used_sources);
    CHECK_AND_ASSERT_MES(r, false, "build_custom_escrow_proposal failed");

    LOG_PRINT_YELLOW("proposal tx: " << get_transaction_hash(escrow_proposal_tx) << " is built for mask: " << cpd.comment, LOG_LEVEL_0);

    if (custom_config_masks[i].invalid_tx_flag)
    {
      //events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
      DO_CALLBACK(events, "mark_invalid_tx");
      events.push_back(escrow_proposal_tx); // should be rejected by the pool even if kept_by_block
      //events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));

      continue;
    }

    events.push_back(escrow_proposal_tx);

    MAKE_NEXT_BLOCK_TX1(events, next_block, top_block, miner_acc, escrow_proposal_tx);

    top_block = next_block;
  }

  // few more variants of incorrect proposal
  ADJUST_TEST_CORE_TIME(top_block.timestamp + DIFFICULTY_TOTAL_TARGET / 2);

  {
    cpd.comment = "incorrect unlock time (past)";
    transaction tx = AUTO_VAL_INIT(tx);
    tx_version = get_tx_version(get_block_height(top_block), m_hardforks);
    // set unlock time to the past (suppose it's incorrect for escrow proposals)
    r = build_custom_escrow_proposal(events, top_block, miner_acc.get_keys(), cpd, top_block.timestamp, 0, top_block.timestamp, 0, 0, TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, eccf_normal, tx_version, tx, used_sources);
    CHECK_AND_ASSERT_MES(r, false, "build_custom_escrow_proposal failed");
    LOG_PRINT_YELLOW("proposal tx: " << get_transaction_hash(tx) << " is built for " << cpd.comment, LOG_LEVEL_0);
    events.push_back(tx);
    MAKE_NEXT_BLOCK_TX1(events, next_block, top_block, miner_acc, tx);
    top_block = next_block;
  }

  {
    cpd.comment = "incorrect unlock time (future)";
    transaction tx = AUTO_VAL_INIT(tx);
    tx_version = get_tx_version(get_block_height(top_block), m_hardforks);
    // set unlock time to the future (suppose it's incorrect for escrow proposals)
    uint64_t unlock_time = top_block.timestamp + 365 * 24 * 60 * 60;
    r = build_custom_escrow_proposal(events, top_block, miner_acc.get_keys(), cpd, unlock_time, 0, unlock_time, 0, 0, TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, eccf_normal, tx_version,  tx, used_sources);
    CHECK_AND_ASSERT_MES(r, false, "build_custom_escrow_proposal failed");
    LOG_PRINT_YELLOW("proposal tx: " << get_transaction_hash(tx) << " is built for " << cpd.comment, LOG_LEVEL_0);
    events.push_back(tx);
    MAKE_NEXT_BLOCK_TX1(events, next_block, top_block, miner_acc, tx);
    top_block = next_block;
  }

  {
    cpd.comment = "template zero expiration time";
    transaction tx = AUTO_VAL_INIT(tx);
    tx_version = get_tx_version(get_block_height(top_block), m_hardforks);
    r = build_custom_escrow_proposal(events, top_block, miner_acc.get_keys(), cpd, 0, 0, 0, 0, 0, TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, eccf_normal, tx_version, tx, used_sources);
    CHECK_AND_ASSERT_MES(r, false, "build_custom_escrow_proposal failed");
    LOG_PRINT_YELLOW("proposal tx: " << get_transaction_hash(tx) << " is built for " << cpd.comment, LOG_LEVEL_0);
    events.push_back(tx);
    MAKE_NEXT_BLOCK_TX1(events, next_block, top_block, miner_acc, tx);
    top_block = next_block;
  }

  {
    cpd.comment = "proposal non-zero expiration time";
    transaction tx = AUTO_VAL_INIT(tx);
    tx_version = get_tx_version(get_block_height(top_block), m_hardforks);
    uint64_t proposal_expiration_time = top_block.timestamp + 12 * 60 * 60;
    uint64_t template_expiration_time = top_block.timestamp + 12 * 60 * 60;
    r = build_custom_escrow_proposal(events, top_block, miner_acc.get_keys(), cpd, 0, proposal_expiration_time, 0, template_expiration_time, 0, TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, eccf_normal, tx_version, tx, used_sources);
    CHECK_AND_ASSERT_MES(r, false, "build_custom_escrow_proposal failed");
    LOG_PRINT_YELLOW("proposal tx: " << get_transaction_hash(tx) << " is built for " << cpd.comment, LOG_LEVEL_0);
    events.push_back(tx);
    MAKE_NEXT_BLOCK_TX1(events, next_block, top_block, miner_acc, tx);
    top_block = next_block;
  }


  DO_CALLBACK(events, "check_incorrect_proposal");

  return true;
}

bool escrow_incorrect_proposal::check_normal_proposal(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // Check that Alice did received the proposal from miner, and make sure it's correct - so the contracts can be successfully finished
  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();

  tools::escrow_contracts_container contracts;
  alice_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(contracts.size() == 1, false, "Alice didn't receive escrow proposal");
  CHECK_AND_ASSERT_MES(contracts.begin()->second.is_a == false, false, "proposal has wrong is_a");
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::proposal_sent, false, "proposal has invalid state");
  crypto::hash contract_id = contracts.begin()->first;
  bc_services::contract_private_details cpd = contracts.begin()->second.private_detailes;

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  alice_wlt->accept_proposal(contract_id, TESTS_DEFAULT_FEE);
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  miner_wlt->refresh();
  miner_wlt->finish_contract(contract_id, BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  alice_wlt->refresh();
  if (!check_balance_via_wallet(*alice_wlt.get(), "alice", cpd.amount_b_pledge + cpd.amount_to_pay))
    return false;

  return true;
}

bool escrow_incorrect_proposal::check_incorrect_proposal(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();

  tools::escrow_contracts_container contracts;
  alice_wlt->get_contracts(contracts);

  if (contracts.size() != 0)
  {
    size_t i = 1;
    std::string s;
    for (auto &c : contracts)
      s += "    " + std::to_string(i) + ") " + c.second.private_detailes.comment + "\r\n", ++i;
    LOG_PRINT_RED_L0("Alice received " << contracts.size() << " wrong contract(s):" << ENDL << s);
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------

escrow_proposal_expiration::escrow_proposal_expiration()
{
  REGISTER_CALLBACK_METHOD(escrow_proposal_expiration, c1);
  REGISTER_CALLBACK_METHOD(escrow_proposal_expiration, c2);
}

bool escrow_proposal_expiration::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: create an escrow proposal with very short expiration time, let it expire, then make sure money is unlocked and the proposal can't be accepted anymore.

  uint64_t ts = 145000000;

  GENERATE_ACCOUNT(preminer_acc);
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc =   m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);
  account_base& carol_acc = m_accounts[CAROL_ACC_IDX]; carol_acc.generate(); carol_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, ts);
  ADJUST_TEST_CORE_TIME(ts);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 4);

  // transfer some coins to Alice and Bob
  uint64_t amount = 0, stub = 0;
  bool r = calculate_amounts_many_outs_have_and_no_outs_have(get_outs_money_amount(blk_0r.miner_tx), amount, stub); // use it to transfer money by large chunks for easier debugging
  CHECK_AND_ASSERT_MES(r, false, "calculate_amounts_many_outs_have_and_no_outs_have failed");
  std::vector<tx_destination_entry> destinations;
  destinations.push_back(tx_destination_entry(amount, alice_acc.get_public_address()));
  destinations.push_back(tx_destination_entry(amount, alice_acc.get_public_address()));
  destinations.push_back(tx_destination_entry(amount, bob_acc.get_public_address()));
  destinations.push_back(tx_destination_entry(amount, bob_acc.get_public_address()));
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx_to_key(m_hardforks, events, tx_1, blk_0r, miner_acc, destinations);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_to_key failed");
  events.push_back(tx_1);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  // rewind a little more to made all txs' outs are spendable from wallets
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1"); // proposal expires after tx added into the blockchain

  DO_CALLBACK(events, "c2"); // proposal expires while it's tx was in the pool

  return true;
}

bool escrow_proposal_expiration::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // make sure escrow proposal correctly expires after its transport tx was added to the blockchain
  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  bob_wlt->refresh();

  uint64_t alice_start_balance = alice_wlt->balance();
  LOG_PRINT_CYAN("Alice's wallet transfers: " << ENDL << alice_wlt->dump_trunsfers(), LOG_LEVEL_0);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  bc_services::contract_private_details cpd = AUTO_VAL_INIT(cpd);
  cpd.amount_a_pledge = TESTS_DEFAULT_FEE * 9;
  cpd.amount_b_pledge = TESTS_DEFAULT_FEE * 9;
  cpd.amount_to_pay = TESTS_DEFAULT_FEE * 7;
  cpd.a_addr = m_accounts[ALICE_ACC_IDX].get_public_address();
  cpd.b_addr = m_accounts[BOB_ACC_IDX].get_public_address();
  cpd.comment = get_random_text(2048);
  cpd.title = get_random_text(15);

  uint64_t expiration_period = 5; // seconds
  
  transaction proposal_tx = AUTO_VAL_INIT(proposal_tx);
  transaction escrow_template_tx = AUTO_VAL_INIT(escrow_template_tx);
  alice_wlt->send_escrow_proposal(cpd, 0, 0, expiration_period, TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, "", proposal_tx, escrow_template_tx);
  LOG_PRINT_CYAN("alice transfers: " << ENDL << alice_wlt->dump_trunsfers(), LOG_LEVEL_0);
  uint64_t alice_post_proposal_balance = alice_wlt->balance();
  uint64_t alice_post_proposal_balance_expected = alice_start_balance - TESTS_DEFAULT_FEE;
  CHECK_AND_ASSERT_MES(alice_post_proposal_balance == alice_post_proposal_balance_expected, false, "Incorrect alice_post_proposal_balance: " << print_money(alice_post_proposal_balance) << ", expected: " << print_money(alice_post_proposal_balance_expected));
  std::deque<tools::transfer_details> transfers;
  alice_wlt->get_transfers(transfers);
  CHECK_AND_ASSERT_MES(transfers.size() == 2 && (
      (transfers[0].is_spent() && (transfers[1].m_flags & (WALLET_TRANSFER_DETAIL_FLAG_BLOCKED | WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION))) ||
      (transfers[1].is_spent() && (transfers[0].m_flags & (WALLET_TRANSFER_DETAIL_FLAG_BLOCKED | WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION)))
    ), false, "Incorrect transfers state in Alice's wallet");
  CHECK_AND_ASSERT_MES(check_wallet_balance_blocked_for_escrow(*alice_wlt.get(), "Alice", cpd.amount_a_pledge + cpd.amount_to_pay), false, "");
  crypto::hash ms_id = get_multisig_out_id(escrow_template_tx, get_multisig_out_index(escrow_template_tx.vout));
  CHECK_AND_ASSERT_MES(ms_id != null_hash, false, "Can't obtain multisig id from escrow template tx");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");

  // mine a block with proposal transport tx
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  // make sure Bob has received correct escrow proposal
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::proposal_sent, ms_id, 1), false, "");

  // mine few block to shift the timestamp median far enough
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  // check Alice's balance
  alice_wlt->refresh();
  uint64_t alice_balance = alice_wlt->balance();
  uint64_t alice_expected_balance = alice_start_balance - TESTS_DEFAULT_FEE;
  CHECK_AND_ASSERT_MES(alice_balance == alice_expected_balance, false, "Alice has incorrect balance after her proposal is expired: " << alice_balance << ", expected: " << alice_expected_balance);

  // check Bob's contracts
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::proposal_sent, ms_id, TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW), false, "");

  // try to accept expired proposal -- an exception should be thrown
  r = false;
  try
  {  
    bob_wlt->accept_proposal(ms_id, TESTS_DEFAULT_FEE);
  }
  catch (tools::error::tx_rejected&)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "Bob tried to accept an expired proposal, but wallet exception was not caught");

  return true;
}

bool escrow_proposal_expiration::c2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // make sure escrow proposal correctly expires while it's waiting in tx pool
  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  bob_wlt->refresh();

  uint64_t alice_start_balance = alice_wlt->balance();
  LOG_PRINT_CYAN("Alice's wallet transfers: " << ENDL << alice_wlt->dump_trunsfers(), LOG_LEVEL_0);
  uint64_t bob_start_balance = bob_wlt->balance();
  LOG_PRINT_CYAN("Bob's wallet transfers: " << ENDL << bob_wlt->dump_trunsfers(), LOG_LEVEL_0);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  bc_services::contract_private_details cpd = AUTO_VAL_INIT(cpd);
  cpd.amount_a_pledge = TESTS_DEFAULT_FEE * 7;
  cpd.amount_b_pledge = TESTS_DEFAULT_FEE * 7;
  cpd.amount_to_pay = TESTS_DEFAULT_FEE * 13;
  cpd.a_addr = m_accounts[ALICE_ACC_IDX].get_public_address();
  cpd.b_addr = m_accounts[BOB_ACC_IDX].get_public_address();
  cpd.comment = get_random_text(2048);
  cpd.title = get_random_text(15);

  uint64_t expiration_period = 5; // seconds
  
  transaction proposal_tx = AUTO_VAL_INIT(proposal_tx);
  transaction escrow_template_tx = AUTO_VAL_INIT(escrow_template_tx);
  alice_wlt->send_escrow_proposal(cpd, 0, 0, expiration_period, TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, "", proposal_tx, escrow_template_tx);
  LOG_PRINT_CYAN("%%%%% Escrow proposal sent, Alice's transfers: " << ENDL << alice_wlt->dump_trunsfers(), LOG_LEVEL_0);
  CHECK_AND_ASSERT_MES(check_wallet_balance_blocked_for_escrow(*alice_wlt.get(), "Alice", cpd.amount_a_pledge + cpd.amount_to_pay), false, "");
  crypto::hash ms_id = get_multisig_out_id(escrow_template_tx, get_multisig_out_index(escrow_template_tx.vout));
  CHECK_AND_ASSERT_MES(ms_id != null_hash, false, "Can't obtain multisig id from escrow template tx");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_contract_state("Alice", alice_wlt, tools::wallet_public::escrow_contract_details::proposal_sent, ms_id, 0), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::proposal_sent, ms_id, 0), false, "");

  // mine a few blocks with no txs
  for (size_t i = 0; i < TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW + 1; ++i)
  {
    r = mine_next_pow_block_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), c, std::vector<transaction>());
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  }

  // expired proposal is still in the pool
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  LOG_PRINT_CYAN("%%%%% tx_memory_pool::on_idle()", LOG_LEVEL_0);
  c.get_tx_pool().on_idle();

  // proposal transport tx has no expiration time, so expired proposal is still in the pool
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // check contract status, should remain the same
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_contract_state("Alice", alice_wlt, tools::wallet_public::escrow_contract_details::proposal_sent, ms_id, TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW + 1), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::proposal_sent, ms_id, TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW + 1), false, "");

  // try to accept expired proposal -- an exception should be thrown
  r = false;
  try
  {  
    bob_wlt->accept_proposal(ms_id, TESTS_DEFAULT_FEE);
  }
  catch (tools::error::tx_rejected&)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "Bob tried to accept an expired proposal, but wallet exception was not caught");

  // mine a block with proposal transport tx
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  // check it has left tx pool
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // check contract status, should remain the same
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_contract_state("Alice", alice_wlt, tools::wallet_public::escrow_contract_details::proposal_sent, ms_id, 1), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::proposal_sent, ms_id, 1), false, "");

  // try to accept expired proposal -- an exception should be thrown
  r = false;
  try
  {  
    bob_wlt->accept_proposal(ms_id, TESTS_DEFAULT_FEE);
  }
  catch (tools::error::tx_rejected&)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "Bob tried to accept an expired proposal, but wallet exception was not caught");

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", alice_start_balance - TESTS_DEFAULT_FEE), false, "");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", bob_start_balance), false, "");

  return true;
}

//------------------------------------------------------------------------------

escrow_proposal_and_accept_expiration::escrow_proposal_and_accept_expiration()
{
  REGISTER_CALLBACK_METHOD(escrow_proposal_and_accept_expiration, c1);
}

bool escrow_proposal_and_accept_expiration::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: accept proposal while its transport tx is unconfirmed, don't confirm both txs, let them expire, then confirm proposal tx
  bool r = false;
  GENERATE_ACCOUNT(preminer_acc);
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  // send few coins to Alice and Bob
  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), TESTS_DEFAULT_FEE * 50, 10, TESTS_DEFAULT_FEE, tx_0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_0);
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), bob_acc.get_public_address(), TESTS_DEFAULT_FEE * 50, 10, TESTS_DEFAULT_FEE, tx_1);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_1);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({ tx_0, tx_1 }));

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

bool escrow_proposal_and_accept_expiration::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  bob_wlt->refresh();

  uint64_t alice_start_balance = alice_wlt->balance();
  uint64_t bob_start_balance = bob_wlt->balance();

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  bc_services::contract_private_details cpd = AUTO_VAL_INIT(cpd);
  cpd.amount_a_pledge = TESTS_DEFAULT_FEE * 13;
  cpd.amount_b_pledge = TESTS_DEFAULT_FEE * 11;
  cpd.amount_to_pay = TESTS_DEFAULT_FEE * 5;
  cpd.a_addr = m_accounts[ALICE_ACC_IDX].get_public_address();
  cpd.b_addr = m_accounts[BOB_ACC_IDX].get_public_address();
  cpd.comment = get_random_text(2048);
  cpd.title = get_random_text(15);
  uint64_t b_release_fee = TESTS_DEFAULT_FEE * 3;

  uint64_t expiration_period = 5; // seconds
  
  transaction proposal_tx = AUTO_VAL_INIT(proposal_tx);
  transaction escrow_template_tx = AUTO_VAL_INIT(escrow_template_tx);
  alice_wlt->send_escrow_proposal(cpd, 0, 0, expiration_period, TESTS_DEFAULT_FEE, b_release_fee, "", proposal_tx, escrow_template_tx);
  LOG_PRINT_CYAN("%%%%% Escrow proposal sent, Alice's transfers: " << ENDL << alice_wlt->dump_trunsfers(), LOG_LEVEL_0);
  CHECK_AND_ASSERT_MES(check_wallet_balance_blocked_for_escrow(*alice_wlt.get(), "Alice", cpd.amount_a_pledge + cpd.amount_to_pay), false, "");

  crypto::hash ms_id = get_multisig_out_id(escrow_template_tx, get_multisig_out_index(escrow_template_tx.vout));
  CHECK_AND_ASSERT_MES(ms_id != null_hash, false, "Can't obtain multisig id from escrow template tx");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_contract_state("Alice", alice_wlt, tools::wallet_public::escrow_contract_details::proposal_sent, ms_id, 0), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::proposal_sent, ms_id, 0), false, "");

  // accept unconfirmed proposal
  try
  {  
    bob_wlt->accept_proposal(ms_id, TESTS_DEFAULT_FEE);
  }
  catch (...)
  {
    CHECK_AND_ASSERT_MES(false, false, "accept_proposal failed");
  }
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 2, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  LOG_PRINT("Tx pool:" << ENDL << c.get_tx_pool().print_pool(true), LOG_LEVEL_0);

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_contract_state("Alice", alice_wlt, tools::wallet_public::escrow_contract_details::contract_accepted, ms_id, 0), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::contract_accepted, ms_id, 0), false, "");

  LOG_PRINT_CYAN("%%%%% Escrow proposal accepted (unconfirmed), Alice's transfers: " << ENDL << alice_wlt->dump_trunsfers(), LOG_LEVEL_0);
  LOG_PRINT_CYAN("%%%%% Escrow proposal accepted (unconfirmed), Bob's transfers: " << ENDL << bob_wlt->dump_trunsfers(), LOG_LEVEL_0);


  // mine a few blocks with no txs
  for (size_t i = 0; i < TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW + 1; ++i)
  {
    r = mine_next_pow_block_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), c, std::vector<transaction>());
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  }

  // proposal and expired acceptance are still in the pool
  LOG_PRINT("Tx pool:" << ENDL << c.get_tx_pool().print_pool(true), LOG_LEVEL_0);
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 2, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  LOG_PRINT_CYAN("%%%%% tx_memory_pool::on_idle()", LOG_LEVEL_0);
  c.get_tx_pool().on_idle();

  // proposal transport tx has no expiration time, so expired proposal is still in the pool, escrow acceptance removed
  LOG_PRINT("Tx pool:" << ENDL << c.get_tx_pool().print_pool(true), LOG_LEVEL_0);
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // check contract status, should remain the same
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_contract_state("Alice", alice_wlt, tools::wallet_public::escrow_contract_details::contract_accepted, ms_id, TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW + 1), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::contract_accepted, ms_id, TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW + 1), false, "");

  LOG_PRINT_CYAN("%%%%% Escrow acceptance tx expired and removed from tx pool, Alice's transfers: " << ENDL << alice_wlt->dump_trunsfers(), LOG_LEVEL_0);
  LOG_PRINT_CYAN("%%%%% Escrow acceptance tx expired and removed from tx pool, Bob's transfers: " << ENDL << bob_wlt->dump_trunsfers(), LOG_LEVEL_0);

  // try to accept expired proposal once again -- an exception should be thrown
  r = false;
  try
  {  
    bob_wlt->accept_proposal(ms_id, TESTS_DEFAULT_FEE);
  }
  catch (...)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "Bob tried to accept an expired proposal, but wallet exception was not caught");

  LOG_PRINT_CYAN("%%%%% mine a block with proposal transport tx", LOG_LEVEL_0);
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  // check it has left tx pool
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // 
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_contract_state("Alice", alice_wlt, tools::wallet_public::escrow_contract_details::proposal_sent, ms_id, 1), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::proposal_sent, ms_id, 1), false, "");

  // try to accept expired proposal -- an exception should be thrown
  r = false;
  try
  {  
    bob_wlt->accept_proposal(ms_id, TESTS_DEFAULT_FEE);
  }
  catch (...)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "Bob tried to accept an expired proposal, but wallet exception was not caught");

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", alice_start_balance - TESTS_DEFAULT_FEE), false, "");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", bob_start_balance), false, "");

  return true;
}

//------------------------------------------------------------------------------

escrow_incorrect_proposal_acceptance::escrow_incorrect_proposal_acceptance()
  : m_alice_bob_start_amount(0)
  , m_bob_fee_release(0)
{
  REGISTER_CALLBACK_METHOD(escrow_incorrect_proposal_acceptance, check_normal_acceptance);
  REGISTER_CALLBACK_METHOD(escrow_incorrect_proposal_acceptance, check_incorrect_acceptance);
}

bool escrow_incorrect_proposal_acceptance::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = 145000000;

  GENERATE_ACCOUNT(preminer_acc);
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc =   m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);
  account_base& carol_acc = m_accounts[CAROL_ACC_IDX]; carol_acc.generate(); carol_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, ts);
  ADJUST_TEST_CORE_TIME(ts);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 4);

  // transfer some coins to Alice and Bob (using big chunks)
  uint64_t big_chunk_amount = 0, stub = 0;
  bool r = calculate_amounts_many_outs_have_and_no_outs_have(get_outs_money_amount(blk_0r.miner_tx), big_chunk_amount, stub);
  CHECK_AND_ASSERT_MES(r, false, "calculate_amounts_many_outs_have_and_no_outs_have failed");

  std::vector<tx_destination_entry> destinations;
  destinations.push_back(tx_destination_entry(big_chunk_amount, alice_acc.get_public_address()));
  destinations.push_back(tx_destination_entry(big_chunk_amount, alice_acc.get_public_address()));
  destinations.push_back(tx_destination_entry(big_chunk_amount, bob_acc.get_public_address()));
  destinations.push_back(tx_destination_entry(big_chunk_amount, bob_acc.get_public_address()));
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx_to_key(m_hardforks, events, tx_1, blk_0r, miner_acc, destinations);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_to_key failed");
  events.push_back(tx_1);
  m_alice_bob_start_amount = big_chunk_amount * 2;

  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  // prepare contract details
  m_cpd.amount_a_pledge = TESTS_DEFAULT_FEE * 12;
  m_cpd.amount_b_pledge = TESTS_DEFAULT_FEE * 13;
  m_cpd.amount_to_pay = TESTS_DEFAULT_FEE * 3;
  m_cpd.a_addr = alice_acc.get_public_address();
  m_cpd.b_addr = bob_acc.get_public_address();
  m_bob_fee_release = 10 * TESTS_DEFAULT_FEE; // Alice states that Bob should pay this much money for upcoming contract release (which will be sent by Alice)
  uint64_t bob_fee_acceptance = TESTS_DEFAULT_FEE;

  std::vector<tx_source_entry> used_sources;

  // create escrow proposal
  bc_services::proposal_body prop = AUTO_VAL_INIT(prop);
  transaction escrow_proposal_tx = AUTO_VAL_INIT(escrow_proposal_tx);
  uint64_t tx_version = get_tx_version(get_block_height(blk_1r), m_hardforks);
  r = build_custom_escrow_proposal(events, blk_1r, alice_acc.get_keys(), m_cpd, 0, 0, 0, blk_1r.timestamp + 36000, 0, TESTS_DEFAULT_FEE, m_bob_fee_release, eccf_normal, tx_version, escrow_proposal_tx, used_sources, &prop);
  CHECK_AND_ASSERT_MES(r, false, "build_custom_escrow_proposal failed");
  events.push_back(escrow_proposal_tx);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, escrow_proposal_tx);

  // create normal acceptance
  tx_version = get_tx_version(get_block_height(blk_2), m_hardforks);
  transaction escrow_normal_acceptance_tx = prop.tx_template;
  uint64_t normal_acceptance_mask = eccf_acceptance_no_tsa_compression;
  r = build_custom_escrow_accept_proposal(events, blk_2, 0, bob_acc.get_keys(), m_cpd, 0, 0, 0, 0, TESTS_DEFAULT_FEE, m_bob_fee_release, normal_acceptance_mask, prop.tx_onetime_secret_key, tx_version, escrow_normal_acceptance_tx, used_sources);
  CHECK_AND_ASSERT_MES(r, false, "build_custom_escrow_accept_proposal failed");

  events.push_back(escrow_normal_acceptance_tx);
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, escrow_normal_acceptance_tx);

  // make sure the acceptance is okay by finishing contract normally
  DO_CALLBACK_PARAMS(events, "check_normal_acceptance", BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL);

  //  0 ... 14    15    25    26    27    28    29                   <- height
  // (0 )- (0r)- (1 )- (1r)- (2 )- (3 )- (4x)                        <- main chain  (4x - mined from a callback)
  //        escrow proposal --|     |     |- escrow normal release
  //            escrow acceptance --|
  //                                 \-- (4b)- (5b)                  <- chain B
  //                                            |- escrow burn

  // rewing the blockchain using altchaing
  MAKE_NEXT_BLOCK(events, blk_4b, blk_3, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_5b, blk_4b, miner_acc); // this should switch the chains
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_5b), get_block_hash(blk_5b)));
  DO_CALLBACK(events, "clear_tx_pool"); // remove old txs, pushed back to the pool after switching chains

  // make sure the acceptance is okay by finishing contract with burning money
  DO_CALLBACK_PARAMS(events, "check_normal_acceptance", BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_BURN);

  //  0 ... 14    15 .. 25    26    27    28    29    30    31       <- height
  // (0 )- (0r)- (1 )- (1r)- (2 )- (3 )- (4x)                        <- main chain  (4x is mined from a callback)
  //        escrow proposal --|     |     |- escrow normal release
  //     escrow normal acceptance --|
  //                          |      \-- (4b)- (5b)- (6x)            <- chain B  (6x is mined from a callback)
  //                          |                       |- escrow burn
  //                          |
  //                           \-- (3c)- (4c)- (5c)- (6c)- (7c)-     <- chain C

  // rewing the blockchain using altchaing
  MAKE_NEXT_BLOCK(events, blk_3c, blk_2, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_4c, blk_3c, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_5c, blk_4c, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_6c, blk_5c, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_7c, blk_6c, miner_acc); // switch to the chain C
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_7c), get_block_hash(blk_7c)));
  DO_CALLBACK(events, "clear_tx_pool"); // remove old txs, pushed back to the pool after switching chains

  // prepare money for Bob's contract acceptances (see below)
  destinations.clear();
  for(size_t i = 0; i < 15; ++i)
    destinations.push_back(tx_destination_entry(m_cpd.amount_b_pledge + m_bob_fee_release + bob_fee_acceptance, bob_acc.get_public_address()));
  transaction tx_2 = AUTO_VAL_INIT(tx_2);
  r = construct_tx_to_key(m_hardforks, events, tx_2, blk_7c, miner_acc, destinations);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_to_key failed");
  events.push_back(tx_2);
  MAKE_NEXT_BLOCK_TX1(events, blk_8c, blk_7c, miner_acc, tx_2);


  used_sources.clear();

  // 2. Create incorrects proposal acceptance and push them to the core.
  // It is expected, that Alice should not receive any.

  uint64_t time_reference = blk_8c.timestamp;

#define ADD_INCORRECT_ACCEPTANCE_MASK(mask, invalid_tx_flag)  { mask, #mask, invalid_tx_flag, 0, 0, 0, 0 }
  struct {
    uint64_t mask;
    const char* name;
    bool invalid_tx_flag;
    uint64_t unlock_time;
    uint64_t expiration_time;
    uint64_t release_unlock_time;
    uint64_t release_expiration_time;
  }
  custom_config_masks[] = {
    {0, "invalid unlock time",                 0, time_reference, 0,                    0,                    0},
    //{0, "invalid expiration time",           0, 0,              time_reference - 100, 0,                    0}, expiration time can't be changed by B, because it's required to be set in escrow template by A (A-sig will be invalid), see test escrow_proposal_expiration for this case
    {0, "invalid release unlock time (1)",     0, 0,              0,                    time_reference + 100, 0},
    {0, "invalid release unlock time (2)",     0, 0,              0,                    1,                    0},
    {0, "invalid release expiration time (1)", 0, 0,              0,                    0,                    time_reference + 100},
    {0, "invalid release expiration time (2)", 0, 0,              0,                    0,                    1},

    ADD_INCORRECT_ACCEPTANCE_MASK(eccf_rel_template_inv_sigs_count,               0),
    ADD_INCORRECT_ACCEPTANCE_MASK(eccf_rel_template_all_money_to_b,               0),
    ADD_INCORRECT_ACCEPTANCE_MASK(eccf_rel_template_no_extra_att_inf,             0),
    ADD_INCORRECT_ACCEPTANCE_MASK(eccf_rel_template_inv_ms_amount,                0),
    ADD_INCORRECT_ACCEPTANCE_MASK(eccf_rel_template_inv_tsa_flags,                0),
    ADD_INCORRECT_ACCEPTANCE_MASK(eccf_rel_template_inv_instruction,              0),
    ADD_INCORRECT_ACCEPTANCE_MASK(eccf_rel_template_signed_parts_in_etc,          0),
    ADD_INCORRECT_ACCEPTANCE_MASK(eccf_rel_template_inv_tx_flags,                 0),
    ADD_INCORRECT_ACCEPTANCE_MASK(eccf_acceptance_no_tsa_encryption,              0),
  };
#undef ADD_INCORRECT_ACCEPTANCE_MASK

  std::vector<tx_source_entry> used_sources_tmp;

  block prev_block = blk_8c;
  for(size_t i = 0; i < sizeof custom_config_masks / sizeof custom_config_masks[0]; ++i)
  {
    const auto &ccm_el = custom_config_masks[i];
    uint64_t mask = ccm_el.mask;

    tx_version = get_tx_version(get_block_height(prev_block), m_hardforks);
    transaction incorrect_acceptance_tx = prop.tx_template;
    r = build_custom_escrow_accept_proposal(events, prev_block, 0, bob_acc.get_keys(), m_cpd, ccm_el.unlock_time, ccm_el.expiration_time, ccm_el.release_unlock_time, ccm_el.release_expiration_time,
      bob_fee_acceptance, m_bob_fee_release, mask, prop.tx_onetime_secret_key, tx_version, incorrect_acceptance_tx, used_sources);
    CHECK_AND_ASSERT_MES(r, false, "build_custom_escrow_accept_proposal failed");

    // In order to use the same escrow proposal to test different invalid acceptance we need to switch chains after each try
    // to avoid double-spend errors, as acceptance transaction does spend few outputs:
    //
    // ...  32    33                                        <- height
    // ... (8c)- (9c)- (10x?)                               <- chain C  (10x might be mined from a callback)
    //            |- incorrect acceptance tx
    //      |
    //       \-  (9d)- (10d)- (11x?)                        (11x might be mined from a callback)
    //                  |- incorrect acceptance tx
    //            |
    //             \-  (10e)- (11e)- (12x?)                 (12x might be mined from a callback)
    //                         |- incorrect acceptance tx
    //     etc...

    if (custom_config_masks[i].invalid_tx_flag)
    {
      DO_CALLBACK(events, "mark_invalid_tx");
      events.push_back(incorrect_acceptance_tx);
    }
    else
    {
      events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true)); // this is to avoid ms id check by tx pool
      events.push_back(incorrect_acceptance_tx);
      events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));
      MAKE_NEXT_BLOCK_TX1(events, next_block, prev_block, miner_acc, incorrect_acceptance_tx); // this may trigger chain swtiching
      DO_CALLBACK(events, "clear_tx_pool"); // remove old txs, pushed back to the pool after switching chains
    }

    DO_CALLBACK(events, "check_incorrect_acceptance");
    boost::get<callback_entry>(events.back()).callback_params = custom_config_masks[i].name;

    DO_CALLBACK(events, "clear_tx_pool"); // remove old txs, pushed back to the pool after switching chains (in case a block was mined within a callback)
    MAKE_NEXT_BLOCK(events, next_block, prev_block, miner_acc);
    prev_block = next_block;
  }

  return true;
}

bool escrow_incorrect_proposal_acceptance::check_normal_acceptance(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::string release_instruction;
  r = epee::string_tools::parse_hexstr_to_binbuff(boost::get<callback_entry>(events[ev_index]).callback_params, release_instruction);
  release_instruction.erase(release_instruction.size() - 1); // remove null-terminator
  bool release_normal = (release_instruction == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL);
  bool release_burn = (release_instruction == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_BURN);
  CHECK_AND_ASSERT_MES(r && (release_normal ^ release_burn), false, "wrong parameter was given to check_normal_acceptance()");

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();
  std::stringstream ss;
  alice_wlt->dump_trunsfers(ss, false);
  LOG_PRINT_L0("check_normal_acceptance(" << release_instruction << "):" << ENDL << "Alice transfers: " << ENDL << ss.str());
  uint64_t alice_balance = alice_wlt->balance();
  uint64_t alice_balance_expected = m_alice_bob_start_amount - m_cpd.amount_a_pledge - m_cpd.amount_to_pay - TESTS_DEFAULT_FEE;
  CHECK_AND_ASSERT_MES(alice_balance == alice_balance_expected, false, "Alice has incorrect balance: " << alice_balance << ", expected: " << alice_balance_expected);
  
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  bob_wlt->refresh();
  uint64_t bob_balance = bob_wlt->balance();
  uint64_t bob_balance_expected = m_alice_bob_start_amount - m_cpd.amount_b_pledge - m_bob_fee_release - TESTS_DEFAULT_FEE;
  CHECK_AND_ASSERT_MES(bob_balance == bob_balance_expected, false, "Bob has incorrect balance: " << bob_balance << ", expected: " << bob_balance_expected);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  tools::escrow_contracts_container contracts;
  r = alice_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Alice failed");

  CHECK_AND_ASSERT_MES(contracts.size() == 1, false, "Alice has incorrect number of contracts: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.private_detailes == m_cpd, false, "Alice has invalid contract's private details");
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::contract_accepted, false, "Alice has invalid contract state: " << contracts.begin()->second.state);

  crypto::hash ms_id = contracts.begin()->first;

  alice_wlt->finish_contract(ms_id, release_normal ? BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL : BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_BURN);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  bob_wlt->refresh();
  contracts.clear();
  r = bob_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Bob failed");
  CHECK_AND_ASSERT_MES(contracts.size() == 1, false, "Bob has incorrect number of contracts: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.private_detailes == m_cpd, false, "Bob has invalid contract's private details");

  uint8_t contract_expected_state = release_normal ? tools::wallet_public::escrow_contract_details::contract_released_normal : tools::wallet_public::escrow_contract_details::contract_released_burned;
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == contract_expected_state, false, "Bob has invalid contract state: " << contracts.begin()->second.state << " expected: " << contract_expected_state);

  uint64_t bob_balance_end = bob_wlt->balance();
  bob_balance_expected = release_normal ? (m_alice_bob_start_amount + m_cpd.amount_to_pay - m_bob_fee_release - TESTS_DEFAULT_FEE) : 
    (m_alice_bob_start_amount - m_cpd.amount_b_pledge - m_bob_fee_release - TESTS_DEFAULT_FEE);
  CHECK_AND_ASSERT_MES(bob_balance_end == bob_balance_expected, false, "Bob has incorrect balance: " << bob_balance_end << ", expected: " << bob_balance_expected);

  alice_wlt->refresh();
  contracts.clear();
  r = alice_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Alice failed");
  CHECK_AND_ASSERT_MES(contracts.size() == 1, false, "Alice has incorrect number of contracts: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.private_detailes == m_cpd, false, "Alice has invalid contract's private details");

  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == contract_expected_state, false, "Alice has invalid contract state: " << contracts.begin()->second.state << " expected: " << contract_expected_state);

  uint64_t alice_balance_end = alice_wlt->balance();
  alice_balance_expected = release_normal ? (m_alice_bob_start_amount - m_cpd.amount_to_pay - TESTS_DEFAULT_FEE) :
    (m_alice_bob_start_amount - m_cpd.amount_a_pledge - m_cpd.amount_to_pay - TESTS_DEFAULT_FEE);
  CHECK_AND_ASSERT_MES(alice_balance_end == alice_balance_expected, false, "Alice has incorrect balance: " << alice_balance_end << ", expected: " << alice_balance_expected);

  return true;
}

bool escrow_incorrect_proposal_acceptance::check_incorrect_acceptance(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::string param = boost::get<callback_entry>(events[ev_index]).callback_params;

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();
  uint64_t alice_balance = alice_wlt->balance();
  std::stringstream ss;
  alice_wlt->dump_trunsfers(ss, false);
  LOG_PRINT_L0("check_incorrect_acceptance(" << param << "):" << ENDL << "Alice balance: " << print_money(alice_balance) << ", transfers: " << ENDL << ss.str());

  tools::escrow_contracts_container contracts;
  r = alice_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Alice failed");

  CHECK_AND_ASSERT_MES(contracts.size() == 1, false, "Alice has incorrect number of contracts: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.private_detailes == m_cpd, false, "Alice has invalid contract's private details");

  const transaction& tx_template = contracts.begin()->second.proposal.tx_template;
  crypto::hash ms_id = get_multisig_out_id(tx_template, get_multisig_out_index(tx_template.vout));
  CHECK_AND_ASSERT_MES(ms_id == contracts.begin()->first, false, "wrong contract id");

  LOG_PRINT_L0("Alice tries to finish contract " << ms_id);
  bool exception_on_finish_contract = false;
  try
  {
    alice_wlt->finish_contract(ms_id, BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL);
    LOG_PRINT_YELLOW("Alice successfully finished incorrect contract " << ms_id << " - this is bad", LOG_LEVEL_0);
  }
  catch(std::exception& e)
  {
    exception_on_finish_contract = true;
    LOG_PRINT_GREEN("An exeption was caught when Alice was finishing incorrect contract " << ms_id << " - this is good. Exception's what(): " << e.what(), LOG_LEVEL_0);
  }

  // check it here in order to let more bugs show off with finish_contract()
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::proposal_sent, false, "Alice has invalid contract state: " << contracts.begin()->second.state);
  CHECK_AND_ASSERT_MES(exception_on_finish_contract, false, "Alice was able to finish contract: " << param);

  return true;
}

//------------------------------------------------------------------------------

struct escrow_custom_test_callback_details
{
  bc_services::contract_private_details cpd;
  uint64_t alice_bob_start_amount;
  size_t fake_outputs_count;
  uint64_t unlock_time;
  uint64_t proposal_expiration_period;
  uint64_t a_proposal_fee;
  uint64_t b_release_fee;
  uint64_t b_accept_fee;
  std::string payment_id;
  std::string release_type; // using service instruction to identify; normal, burn or cancel
  uint64_t a_cancel_proposal_fee;
  uint64_t cancellation_expiration_period;

  bool is_release_normal() const { return release_type == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL; }
  bool is_release_burn()   const { return release_type == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_BURN; }
  bool is_release_cancel() const { return release_type == BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_CANCEL; }

  BEGIN_KV_SERIALIZE_MAP()
    KV_SERIALIZE(cpd)
    KV_SERIALIZE_POD_AS_HEX_STRING(alice_bob_start_amount)
    KV_SERIALIZE_POD_AS_HEX_STRING(fake_outputs_count)
    KV_SERIALIZE_POD_AS_HEX_STRING(unlock_time)
    KV_SERIALIZE_POD_AS_HEX_STRING(proposal_expiration_period)
    KV_SERIALIZE_POD_AS_HEX_STRING(a_proposal_fee)
    KV_SERIALIZE_POD_AS_HEX_STRING(b_release_fee)
    KV_SERIALIZE_POD_AS_HEX_STRING(b_accept_fee)
    KV_SERIALIZE(payment_id)
    KV_SERIALIZE(release_type)
    KV_SERIALIZE_POD_AS_HEX_STRING(a_cancel_proposal_fee)
    KV_SERIALIZE_POD_AS_HEX_STRING(cancellation_expiration_period);
  END_KV_SERIALIZE_MAP()
};

escrow_custom_test::escrow_custom_test()
{
  REGISTER_CALLBACK_METHOD(escrow_custom_test, check);
}

bool escrow_custom_test::generate(std::vector<test_event_entry>& events) const
{
  bool r = false;
  uint64_t ts = 145000000;

  GENERATE_ACCOUNT(preminer_acc);
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc =   m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, ts);
  ADJUST_TEST_CORE_TIME(ts);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 4);

  const uint64_t chunk_amount = 10 * TESTS_DEFAULT_FEE;
  const size_t chunks_count = 20;
  uint64_t alice_bob_start_amount = chunks_count * chunk_amount;
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  r = fill_tx_sources(sources, events, blk_0r, miner_acc.get_keys(), 2 * alice_bob_start_amount + TESTS_DEFAULT_FEE, 0, true, true, false);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  for(size_t i = 0; i < chunks_count; ++i)
  {
    destinations.push_back(tx_destination_entry(chunk_amount, alice_acc.get_public_address()));
    destinations.push_back(tx_destination_entry(chunk_amount, bob_acc.get_public_address()));
  }
  // no change back to the miner - it will become a fee
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_1, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

  events.push_back(tx_1);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);


  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  block prev_block = blk_1r;

  std::vector<escrow_custom_test_callback_details> test_details;

  // setup custom tests by filling in callback details
  {
    escrow_custom_test_callback_details cd = AUTO_VAL_INIT(cd);
    cd.alice_bob_start_amount = alice_bob_start_amount;
    cd.cpd.comment         = "normal contract";
    cd.cpd.amount_a_pledge = 70 * TESTS_DEFAULT_FEE;
    cd.cpd.amount_b_pledge = 94 * TESTS_DEFAULT_FEE;
    cd.cpd.amount_to_pay   = 18 * TESTS_DEFAULT_FEE;
    cd.cpd.a_addr          = alice_acc.get_public_address();
    cd.cpd.b_addr          = bob_acc.get_public_address();
    cd.fake_outputs_count  = 0;
    cd.unlock_time         = 0;
    cd.proposal_expiration_period = 3600;
    cd.a_proposal_fee      = 10 * TESTS_DEFAULT_FEE;
    cd.b_accept_fee        = 1 * TESTS_DEFAULT_FEE;
    cd.b_release_fee       = 5 * TESTS_DEFAULT_FEE;
    cd.payment_id          = "payment id";
    cd.release_type        = BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL;
    test_details.push_back(cd);
  }

  {
    escrow_custom_test_callback_details cd = AUTO_VAL_INIT(cd);
    cd.alice_bob_start_amount = alice_bob_start_amount;
    cd.cpd.comment         = "no change";
    cd.cpd.amount_a_pledge = alice_bob_start_amount - 2 * chunk_amount;
    cd.cpd.amount_b_pledge = alice_bob_start_amount - 2 * chunk_amount;
    cd.cpd.amount_to_pay   = chunk_amount;
    cd.cpd.a_addr          = alice_acc.get_public_address();
    cd.cpd.b_addr          = bob_acc.get_public_address();
    cd.fake_outputs_count  = 0;
    cd.unlock_time         = 0;
    cd.proposal_expiration_period = 3600;
    cd.a_proposal_fee      = chunk_amount;
    cd.b_accept_fee        = chunk_amount;
    cd.b_release_fee       = chunk_amount;
    cd.payment_id          = "payment id";
    cd.release_type        = BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL;
    test_details.push_back(cd);
  }

  {
    escrow_custom_test_callback_details cd = test_details[0];
    cd.cpd.comment         = "zero A pledge";
    cd.cpd.amount_a_pledge = 0;
    test_details.push_back(cd);
  }

  {
    escrow_custom_test_callback_details cd = test_details[0];
    cd.cpd.comment         = "zero B pledge";
    cd.cpd.amount_b_pledge = 0;
    // note: amount to pay is not zero
    test_details.push_back(cd);
  }

  {
    escrow_custom_test_callback_details cd = test_details[0];
    cd.cpd.comment         = "zero amount to pay";
    cd.cpd.amount_to_pay   = 0;
    test_details.push_back(cd);
  }

  {
    escrow_custom_test_callback_details cd = test_details[0];
    cd.cpd.comment         = "release burn";
    cd.cpd.amount_to_pay   = 2 * chunk_amount;
    cd.release_type        = BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_BURN;
    test_details.push_back(cd);
  }

  {
    escrow_custom_test_callback_details cd = test_details[0];
    cd.cpd.comment         = "simple cancellation";
    cd.release_type        = BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_CANCEL;
    cd.a_cancel_proposal_fee = 3 * TESTS_DEFAULT_FEE;
    cd.cancellation_expiration_period = 1000;
    test_details.push_back(cd);
  }

  {
    escrow_custom_test_callback_details cd = test_details[0];
    cd.cpd.comment         = "release normal after cancellation request";
    cd.release_type        = BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL;
    cd.a_cancel_proposal_fee = 3 * TESTS_DEFAULT_FEE; // this will trigger cancellation request
    cd.cancellation_expiration_period = 1000;
    test_details.push_back(cd);
  }

  {
    escrow_custom_test_callback_details cd = test_details[0];
    cd.cpd.comment = "cancellation: zero A pledge";
    cd.cpd.amount_a_pledge = 0;
    cd.release_type = BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_CANCEL;
    cd.a_cancel_proposal_fee = 3 * TESTS_DEFAULT_FEE;
    cd.cancellation_expiration_period = 1000;
    test_details.push_back(cd);
  }

  {
    escrow_custom_test_callback_details cd = test_details[0];
    cd.cpd.comment = "cancellation: zero B pledge";
    cd.cpd.amount_b_pledge = 0;
    cd.release_type = BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_CANCEL;
    cd.a_cancel_proposal_fee = 3 * TESTS_DEFAULT_FEE;
    cd.cancellation_expiration_period = 1000;
    test_details.push_back(cd);
  }

  {
    escrow_custom_test_callback_details cd = test_details[0];
    cd.cpd.comment = "cancellation: zero amount to pay";
    cd.cpd.amount_to_pay = 0;
    cd.release_type = BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_CANCEL;
    cd.a_cancel_proposal_fee = 3 * TESTS_DEFAULT_FEE;
    cd.cancellation_expiration_period = 1000;
    test_details.push_back(cd);
  }

  
  for(auto cd : test_details)
  {
    DO_CALLBACK_PARAMS_STR(events, "check", epee::serialization::store_t_to_json(cd));

    REWIND_BLOCKS_N(events, blk_n, prev_block, miner_acc, 7); // this should switch the chains
    DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_n), get_block_hash(blk_n)));
    DO_CALLBACK(events, "clear_tx_pool"); // remove old txs, pushed back to the pool after switching chains

    prev_block = blk_n;
  }
  
  return true;
}

bool escrow_custom_test::check(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  escrow_custom_test_callback_details cd = AUTO_VAL_INIT(cd);
  bool r = epee::serialization::load_t_from_json(cd, boost::get<callback_entry>(events[ev_index]).callback_params);
  CHECK_AND_ASSERT_MES(r, false, "Can't obtain event params.");
  
  LOG_PRINT_GREEN("****************** starting custom test \"" << cd.cpd.comment << "\"", LOG_LEVEL_0);
  try
  {
    r = do_custom_test(c, ev_index, events, cd);
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("caught std::exception: " << e.what());
    r = false;
  }
  catch (...)
  {
    LOG_ERROR("caught non std::exception");
    r = false;
  }

  if (r)
  {
    LOG_PRINT_GREEN("****************** custom test \"" << cd.cpd.comment << "\" succeeded", LOG_LEVEL_0);
  }
  else
  {
    LOG_PRINT_MAGENTA("****************** custom test \"" << cd.cpd.comment << "\" FAILED", LOG_LEVEL_0);
  }

  return r;
}

bool escrow_custom_test::do_custom_test(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events, const escrow_custom_test_callback_details& cd)
{
  bool stub = false;
  bool r = false;

  uint64_t ms_amount = cd.cpd.amount_a_pledge + cd.cpd.amount_b_pledge + cd.cpd.amount_to_pay + cd.b_release_fee;

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();
  uint64_t alice_start_balance = alice_wlt->balance();
  CHECK_AND_ASSERT_MES(alice_start_balance == cd.alice_bob_start_amount, false, "Alice has incorrect start balance: " << alice_start_balance);

  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  bob_wlt->refresh();
  uint64_t bob_start_balance = bob_wlt->balance();
  CHECK_AND_ASSERT_MES(bob_start_balance == cd.alice_bob_start_amount, false, "Bob has incorrect start balance: " << bob_start_balance);

  // send escrow proposal

  transaction proposal_tx = AUTO_VAL_INIT(proposal_tx);
  transaction escrow_template_tx = AUTO_VAL_INIT(escrow_template_tx);
  try
  {
    alice_wlt->send_escrow_proposal(cd.cpd, cd.fake_outputs_count, cd.unlock_time, cd.proposal_expiration_period, cd.a_proposal_fee, cd.b_release_fee, cd.payment_id, proposal_tx, escrow_template_tx);
  }
  catch(std::exception& e)
  {
    LOG_ERROR("A -> send_escrow_proposal failed with exception: " << e.what());
    return false;
  }

  alice_wlt->scan_tx_pool(stub);
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", alice_start_balance - cd.a_proposal_fee, 0, INVALID_BALANCE_VAL, 0, 0), false, "");
  CHECK_AND_ASSERT_MES(check_wallet_balance_blocked_for_escrow(*alice_wlt.get(), "Alice", cd.cpd.amount_a_pledge + cd.cpd.amount_to_pay), false, "");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  alice_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", alice_start_balance - cd.a_proposal_fee, 0, INVALID_BALANCE_VAL, 0, 0), false, "");
  CHECK_AND_ASSERT_MES(check_wallet_balance_blocked_for_escrow(*alice_wlt.get(), "Alice", cd.cpd.amount_a_pledge + cd.cpd.amount_to_pay), false, "");

  bob_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", bob_start_balance, 0, INVALID_BALANCE_VAL, 0, 0), false, "");

  tools::escrow_contracts_container contracts;
  r = bob_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Bob failed");
  CHECK_AND_ASSERT_MES(check_contract_state(contracts, cd.cpd, tools::wallet_public::escrow_contract_details::proposal_sent, "Bob"), false, "wrong contract");
  CHECK_AND_ASSERT_MES(contracts.begin()->second.payment_id == cd.payment_id, false, "incorrect payment_id was found in Bob's contract: " << contracts.begin()->second.payment_id << ", expected: " << cd.payment_id);
  crypto::hash ms_id = contracts.begin()->first;

  
  // accept the proposal

  try
  {
    bob_wlt->accept_proposal(ms_id, cd.b_accept_fee);
  }
  catch(std::exception& e)
  {
    LOG_ERROR("B -> accept_proposal failed with exception: " << e.what());
    return false;
  }

  bob_wlt->scan_tx_pool(stub);
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob",
    bob_start_balance - cd.cpd.amount_b_pledge - cd.b_accept_fee - cd.b_release_fee,
    0, INVALID_BALANCE_VAL, 0,
    cd.cpd.amount_b_pledge + cd.b_release_fee),
    false, "");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  alice_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", alice_start_balance - cd.cpd.amount_a_pledge - cd.cpd.amount_to_pay - cd.a_proposal_fee, 0, INVALID_BALANCE_VAL, 0, 0), false, "");

  bob_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", bob_start_balance - cd.cpd.amount_b_pledge - cd.b_accept_fee - cd.b_release_fee, 0, INVALID_BALANCE_VAL, 0, 0), false, "");

  contracts.clear();
  r = alice_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Alice failed");
  CHECK_AND_ASSERT_MES(contracts.size() == 1, false, "incorrect contracts size for Alice: " << contracts.size());
  CHECK_AND_ASSERT_MES(check_contract_state(contracts, cd.cpd, tools::wallet_public::escrow_contract_details::contract_accepted, "Alice"), false, "wrong contract");
  CHECK_AND_ASSERT_MES(contracts.begin()->second.payment_id == cd.payment_id, false, "incorrect payment_id was found in Alice's contract: " << contracts.begin()->second.payment_id << ", expected: " << cd.payment_id);

  // request cancellation if needed (normal/burn release should be still possible after that)
  if (cd.a_cancel_proposal_fee > 0)
  {
    try
    {
      alice_wlt->request_cancel_contract(ms_id, cd.a_cancel_proposal_fee, cd.cancellation_expiration_period);
    }
    catch(std::exception& e)
    {
      LOG_ERROR("A -> request_cancel_contract failed with exception: " << e.what());
      return false;
    }
  
    uint64_t alice_expected_balance = alice_start_balance - cd.cpd.amount_a_pledge - cd.cpd.amount_to_pay - cd.a_proposal_fee - cd.a_cancel_proposal_fee;
    alice_wlt->scan_tx_pool(stub);
    CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", alice_expected_balance, 0, INVALID_BALANCE_VAL, 0, 0), false, "");

    uint64_t bob_expected_balance = bob_start_balance - cd.cpd.amount_b_pledge - cd.b_accept_fee - cd.b_release_fee;
    bob_wlt->scan_tx_pool(stub);
    CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", bob_expected_balance, 0, INVALID_BALANCE_VAL, 0, 0), false, ""); // should not change

    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");
    r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

    alice_wlt->refresh();
    contracts.clear();
    r = alice_wlt->get_contracts(contracts);
    CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Alice failed");
    CHECK_AND_ASSERT_MES(check_contract_state(contracts, cd.cpd, tools::wallet_public::escrow_contract_details::contract_cancel_proposal_sent, "Alice"), false, "wrong contract state");

    CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", alice_expected_balance, 0, INVALID_BALANCE_VAL, 0, 0), false, "");

    bob_wlt->refresh();
    contracts.clear();
    r = bob_wlt->get_contracts(contracts);
    CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Bob failed");
    CHECK_AND_ASSERT_MES(check_contract_state(contracts, cd.cpd, tools::wallet_public::escrow_contract_details::contract_cancel_proposal_sent, "Bob"), false, "wrong contract state");

    CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", bob_expected_balance, 0, INVALID_BALANCE_VAL, 0, 0), false, "");
  }

  if (cd.is_release_normal() || cd.is_release_burn())
  {
    // finish contract
    try
    {
      alice_wlt->finish_contract(ms_id, cd.release_type);
    }
    catch(std::exception& e)
    {
      LOG_ERROR("A -> finish_contract failed with exception: " << e.what());
      return false;
    }
  
    uint64_t alice_expected_balance = cd.is_release_normal() ? alice_start_balance - cd.cpd.amount_to_pay - cd.a_proposal_fee - cd.a_cancel_proposal_fee : alice_start_balance - cd.cpd.amount_a_pledge - cd.cpd.amount_to_pay - cd.a_proposal_fee - cd.a_cancel_proposal_fee;
    uint64_t bob_expected_balance = cd.is_release_normal() ? bob_start_balance + cd.cpd.amount_to_pay - cd.b_accept_fee - cd.b_release_fee : bob_start_balance - cd.cpd.amount_b_pledge - cd.b_accept_fee - cd.b_release_fee;

    alice_wlt->scan_tx_pool(stub);
    CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", alice_expected_balance, 0, INVALID_BALANCE_VAL, cd.is_release_normal() ? cd.cpd.amount_a_pledge : 0, cd.is_release_normal() ? 0 : ms_amount - cd.b_release_fee), false, "");

    bob_wlt->scan_tx_pool(stub);
    CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", bob_expected_balance, 0, INVALID_BALANCE_VAL, cd.is_release_normal() ? cd.cpd.amount_b_pledge + cd.cpd.amount_to_pay : 0, cd.is_release_normal() ? 0 : ms_amount - cd.b_release_fee), false, "");

    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");
    r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

    alice_wlt->refresh();
    contracts.clear();
    r = alice_wlt->get_contracts(contracts);
    CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Alice failed");
    tools::wallet_public::escrow_contract_details::contract_state expected_state = cd.is_release_normal() ? tools::wallet_public::escrow_contract_details::contract_released_normal : tools::wallet_public::escrow_contract_details::contract_released_burned;
    CHECK_AND_ASSERT_MES(check_contract_state(contracts, cd.cpd, expected_state, "Alice"), false, "wrong contract");

    CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", alice_expected_balance, 0, INVALID_BALANCE_VAL, 0, 0), false, "");


    bob_wlt->refresh();
    contracts.clear();
    r = bob_wlt->get_contracts(contracts);
    CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Bob failed");
    expected_state = cd.is_release_normal() ? tools::wallet_public::escrow_contract_details::contract_released_normal : tools::wallet_public::escrow_contract_details::contract_released_burned;
    CHECK_AND_ASSERT_MES(check_contract_state(contracts, cd.cpd, expected_state, "Bob"), false, "wrong contract");

    CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", bob_expected_balance, 0, INVALID_BALANCE_VAL, 0, 0), false, "");
  }
  else if (cd.is_release_cancel())
  {
    // cancel proposal should be already sent -- see above
    try
    {
      bob_wlt->accept_cancel_contract(ms_id);
    }
    catch(std::exception& e)
    {
      LOG_ERROR("B -> accept_cancel_contract failed with exception: " << e.what());
      return false;
    }
  
    uint64_t alice_expected_balance = alice_start_balance - cd.a_proposal_fee - cd.a_cancel_proposal_fee;
    uint64_t alice_expected_aw_in_balance = cd.cpd.amount_a_pledge + cd.cpd.amount_to_pay;
    alice_wlt->scan_tx_pool(stub);
    CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", alice_expected_balance, 0, INVALID_BALANCE_VAL, alice_expected_aw_in_balance, 0), false, "");

    uint64_t bob_expected_balance = bob_start_balance - cd.b_accept_fee - cd.b_release_fee;
    uint64_t bob_expected_aw_in_balance = cd.cpd.amount_b_pledge;
    bob_wlt->scan_tx_pool(stub);
    CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", bob_expected_balance, 0, INVALID_BALANCE_VAL, bob_expected_aw_in_balance, 0), false, "");

    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");
    r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

    alice_wlt->refresh();
    contracts.clear();
    r = alice_wlt->get_contracts(contracts);
    CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Alice failed");
    CHECK_AND_ASSERT_MES(check_contract_state(contracts, cd.cpd, tools::wallet_public::escrow_contract_details::contract_released_cancelled, "Alice"), false, "wrong contract state");

    CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", alice_expected_balance, 0, INVALID_BALANCE_VAL, 0, 0), false, "");

    bob_wlt->refresh();
    contracts.clear();
    r = bob_wlt->get_contracts(contracts);
    CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Bob failed");
    CHECK_AND_ASSERT_MES(check_contract_state(contracts, cd.cpd, tools::wallet_public::escrow_contract_details::contract_released_cancelled, "Bob"), false, "wrong contract state");

    CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", bob_expected_balance, 0, INVALID_BALANCE_VAL, 0, 0), false, "");
  }
  else
  {
    LOG_ERROR("Invalid release type for custom escrow test");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------

escrow_incorrect_cancel_proposal::escrow_incorrect_cancel_proposal()
  : m_alice_bob_start_amount(0)
  , m_bob_fee_release(0)
{
  REGISTER_CALLBACK_METHOD(escrow_incorrect_cancel_proposal, check_normal_cancel_proposal);
  REGISTER_CALLBACK_METHOD(escrow_incorrect_cancel_proposal, check_incorrect_cancel_proposal);
}

bool escrow_incorrect_cancel_proposal::generate(std::vector<test_event_entry>& events) const
{
  bool r = false;
  uint64_t ts = 145000000;

  GENERATE_ACCOUNT(preminer_acc);
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc =   m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, ts);
  ADJUST_TEST_CORE_TIME(ts);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 9);

  const uint64_t chunk_amount = 20 * TESTS_DEFAULT_FEE;
  const size_t chunks_count = 20;
  m_alice_bob_start_amount = chunks_count * chunk_amount;
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  r = fill_tx_sources(sources, events, blk_0r, miner_acc.get_keys(), 2 * m_alice_bob_start_amount+ TESTS_DEFAULT_FEE, 0, true, true, false);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  for(size_t i = 0; i < chunks_count; ++i)
  {
    destinations.push_back(tx_destination_entry(chunk_amount, alice_acc.get_public_address()));
    destinations.push_back(tx_destination_entry(chunk_amount, bob_acc.get_public_address()));
  }
  // no change back to the miner - it will become a fee
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_1, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

  events.push_back(tx_1);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  // prepare contract details
  m_cpd.amount_a_pledge = TESTS_DEFAULT_FEE * 12;
  m_cpd.amount_b_pledge = TESTS_DEFAULT_FEE * 13;
  m_cpd.amount_to_pay = TESTS_DEFAULT_FEE * 3;
  m_cpd.a_addr = alice_acc.get_public_address();
  m_cpd.b_addr = bob_acc.get_public_address();
  m_bob_fee_release = 10 * TESTS_DEFAULT_FEE; // Alice states that Bob should pay this much money for upcoming contract release (which will be sent by Alice)

  std::vector<tx_source_entry> used_sources;

  // create normal escrow proposal
  bc_services::proposal_body prop = AUTO_VAL_INIT(prop);
  uint64_t tx_version = get_tx_version(get_block_height(blk_1r), m_hardforks);
  transaction escrow_proposal_tx = AUTO_VAL_INIT(escrow_proposal_tx);
  r = build_custom_escrow_proposal(events, blk_1r, alice_acc.get_keys(), m_cpd, 0, 0, 0, blk_1r.timestamp + 36000, 0, TESTS_DEFAULT_FEE, m_bob_fee_release, eccf_normal, tx_version, escrow_proposal_tx, used_sources, &prop);
  CHECK_AND_ASSERT_MES(r, false, "build_custom_escrow_proposal failed");
  events.push_back(escrow_proposal_tx);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, escrow_proposal_tx);

  // create normal escrow proposal acceptance
  transaction escrow_normal_acceptance_tx = prop.tx_template;
  tx_version = get_tx_version(get_block_height(blk_2), m_hardforks);
  uint64_t normal_acceptance_mask = eccf_acceptance_no_tsa_compression;
  r = build_custom_escrow_accept_proposal(events, blk_2, 0, bob_acc.get_keys(), m_cpd, 0, 0, 0, 0, TESTS_DEFAULT_FEE, m_bob_fee_release, normal_acceptance_mask, prop.tx_onetime_secret_key, tx_version, escrow_normal_acceptance_tx, used_sources);
  CHECK_AND_ASSERT_MES(r, false, "build_custom_escrow_accept_proposal failed");

  events.push_back(escrow_normal_acceptance_tx);
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, escrow_normal_acceptance_tx);

  // create normal cancel proposal
  tx_version = get_tx_version(get_block_height(blk_3), m_hardforks);
  transaction escrow_normal_cancel_proposal_tx = AUTO_VAL_INIT(escrow_normal_cancel_proposal_tx);
  r = build_custom_escrow_cancel_proposal(events, blk_3, 0, alice_acc.get_keys(), m_cpd, 0, 0, 0, blk_3.timestamp + 3600, TESTS_DEFAULT_FEE, eccf_normal, prop.tx_template, tx_version, escrow_normal_cancel_proposal_tx, used_sources);
  CHECK_AND_ASSERT_MES(r, false, "build_custom_escrow_cancel_proposal failed");

  events.push_back(escrow_normal_cancel_proposal_tx);
  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_3, miner_acc, escrow_normal_cancel_proposal_tx);

  // make sure that normal cancel proposal is okay by accepting it from B-side, cancelling the contract
  DO_CALLBACK(events, "check_normal_cancel_proposal");

  //  0 ... 14    15 .. 25    26    27    28    29    30    31       <- height
  // (0 )- (0r)- (1 )- (1r)- (2 )- (3 )- (4 )- (5x)                  <- main chain  (5x - mined from a callback)
  //     A: escrow proposal --|     |     |     |- B: cancellation accepted
  //         B: escrow acceptance --|     |
  //          A: normal cancel proposal --|
  //                                \--  (4b)- (5b)- (6b)-           <- chain B


  // rewing the blockchain using altchaing
  MAKE_NEXT_BLOCK(events, blk_4b, blk_3, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_5b, blk_4b, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_6b, blk_5b, miner_acc); // switch to the alth chain B
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_6b), get_block_hash(blk_6b)));
  DO_CALLBACK(events, "clear_tx_pool"); // remove old txs, pushed back to the pool after switching chains

  used_sources.clear();

  // 2. Create incorrects cancellation proposals and push them to the core.
  // It is expected, that Bob should not receive any.

  uint64_t time_reference = blk_6b.timestamp;

#define ADD_INCORRECT_CANCELLATION_REQUEST_MASK(mask, invalid_tx_flag)  { static_cast<uint64_t>(mask), #mask, invalid_tx_flag, 0, 0, 0, time_reference + 3600 }
  struct {
    uint64_t mask;
    const char* name;
    bool invalid_tx_flag;
    uint64_t unlock_time;
    uint64_t expiration_time;
    uint64_t release_unlock_time;
    uint64_t release_expiration_time;
  }
  custom_config_masks[] = {                    // unlock time     exp. time             release unlock time   release expiration time
    {0, "invalid unlock time",                 0, time_reference, 0,                    0,                    0},
    {0, "invalid expiration time",             0, 0,              time_reference - 100, 0,                    0},
    {0, "invalid release unlock time (1)",     0, 0,              0,                    time_reference + 100, 0},
    {0, "invalid release unlock time (2)",     0, 0,              0,                    1,                    0},
    {0, "invalid release expiration time (1)", 0, 0,              0,                    0,                    0},
    {0, "invalid release expiration time (2)", 0, 0,              0,                    0,                    time_reference - 100},

    ADD_INCORRECT_CANCELLATION_REQUEST_MASK(eccf_rel_template_inv_ms_amount,              0),
    ADD_INCORRECT_CANCELLATION_REQUEST_MASK(eccf_rel_template_inv_sigs_count,             0),
    ADD_INCORRECT_CANCELLATION_REQUEST_MASK(eccf_rel_template_inv_instruction,            0),
    ADD_INCORRECT_CANCELLATION_REQUEST_MASK(eccf_rel_template_inv_tsa_flags,              0),
    ADD_INCORRECT_CANCELLATION_REQUEST_MASK(eccf_rel_template_all_money_to_a,             0),
    ADD_INCORRECT_CANCELLATION_REQUEST_MASK(eccf_rel_template_inv_tx_flags,               0),
    ADD_INCORRECT_CANCELLATION_REQUEST_MASK(eccf_rel_template_signed_parts_in_etc,        0),
    ADD_INCORRECT_CANCELLATION_REQUEST_MASK(eccf_cancellation_no_tsa_encryption,          0),
    ADD_INCORRECT_CANCELLATION_REQUEST_MASK(eccf_cancellation_inv_crypt_address,          0),
  };
#undef ADD_INCORRECT_ACCEPTANCE_MASK

  block prev_block = blk_6b;
  for(size_t i = 0; i < sizeof custom_config_masks / sizeof custom_config_masks[0]; ++i)
  {
    const auto &ccm_el = custom_config_masks[i];
    uint64_t mask = ccm_el.mask;

    transaction incorrect_cancellation_proposal_tx = prop.tx_template;
    uint64_t tx_version = get_tx_version(get_block_height(blk_3), m_hardforks);
    r = build_custom_escrow_cancel_proposal(events, prev_block, 0, alice_acc.get_keys(), m_cpd, ccm_el.unlock_time, ccm_el.expiration_time, ccm_el.release_unlock_time, ccm_el.release_expiration_time,
      TESTS_DEFAULT_FEE, mask, prop.tx_template, tx_version, incorrect_cancellation_proposal_tx, used_sources);
    CHECK_AND_ASSERT_MES(r, false, "build_custom_escrow_cancel_proposal failed");

    // In order to use the same escrow proposal to test different invalid cancellations we need to switch chains after each try
    // to avoid double-spend errors, as transaction does spend few outputs:
    //
    // ...  30    31                                       <- height
    // ... (6b)- (7b)- (8x?)                               <- chain B  (10x might be mined from a callback)
    //            |- incorrect acceptance tx
    //      |
    //       \-  (7c)- (8c)- (9x?)                        (11x might be mined from a callback)
    //                  |- incorrect acceptance tx
    //            |
    //             \-  (8d)- (9d)- (10x?)                 (12x might be mined from a callback)
    //                         |- incorrect acceptance tx
    //     etc...

    if (custom_config_masks[i].invalid_tx_flag)
    {
      DO_CALLBACK(events, "mark_invalid_tx");
      events.push_back(incorrect_cancellation_proposal_tx);
    }
    else
    {
      events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true)); // this is to avoid ms id check by tx pool
      events.push_back(incorrect_cancellation_proposal_tx);
      events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));
      MAKE_NEXT_BLOCK_TX1(events, next_block, prev_block, miner_acc, incorrect_cancellation_proposal_tx); // this may trigger chain swtiching
      DO_CALLBACK(events, "clear_tx_pool"); // remove old txs, pushed back to the pool after switching chains
    }

    DO_CALLBACK(events, "check_incorrect_cancel_proposal");
    boost::get<callback_entry>(events.back()).callback_params = custom_config_masks[i].name;

    DO_CALLBACK(events, "clear_tx_pool"); // remove old txs, pushed back to the pool after switching chains (in case a block was mined within a callback)
    MAKE_NEXT_BLOCK(events, next_block, prev_block, miner_acc);
    prev_block = next_block;
  }

  return true;
}

bool escrow_incorrect_cancel_proposal::check_normal_cancel_proposal(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();
  std::stringstream ss;
  alice_wlt->dump_trunsfers(ss, false);
  LOG_PRINT_L0("check_normal_cancel_proposal:" << ENDL << "Alice transfers: " << ENDL << ss.str());
  uint64_t alice_balance = alice_wlt->balance();
  uint64_t alice_balance_expected = m_alice_bob_start_amount - m_cpd.amount_a_pledge - m_cpd.amount_to_pay - TESTS_DEFAULT_FEE - TESTS_DEFAULT_FEE; // one fee for escrow request, second - for cancel request
  CHECK_AND_ASSERT_MES(alice_balance == alice_balance_expected, false, "Alice has incorrect balance: " << alice_balance << ", expected: " << alice_balance_expected);
  
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  bob_wlt->refresh();
  uint64_t bob_balance = bob_wlt->balance();
  uint64_t bob_balance_expected = m_alice_bob_start_amount - m_cpd.amount_b_pledge - m_bob_fee_release - TESTS_DEFAULT_FEE;
  CHECK_AND_ASSERT_MES(bob_balance == bob_balance_expected, false, "Bob has incorrect balance: " << bob_balance << ", expected: " << bob_balance_expected);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  tools::escrow_contracts_container contracts;
  r = alice_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Alice failed");
  CHECK_AND_ASSERT_MES(contracts.size() == 1, false, "Alice has incorrect number of contracts: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.private_detailes == m_cpd, false, "Alice has invalid contract's private details");
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::contract_cancel_proposal_sent, false, "Alice has invalid contract state: " << contracts.begin()->second.state);

  contracts.clear();
  r = bob_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Bob failed");
  CHECK_AND_ASSERT_MES(contracts.size() == 1, false, "Bob has incorrect number of contracts: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.private_detailes == m_cpd, false, "Bob has invalid contract's private details");
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::contract_cancel_proposal_sent, false, "Bob has invalid contract state: " << contracts.begin()->second.state);

  crypto::hash ms_id = contracts.begin()->first;
  bob_wlt->accept_cancel_contract(ms_id);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  alice_wlt->refresh();
  contracts.clear();
  r = alice_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Alice failed");
  CHECK_AND_ASSERT_MES(contracts.size() == 1, false, "Alice has incorrect number of contracts: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.private_detailes == m_cpd, false, "Alice has invalid contract's private details");
  uint8_t contract_expected_state = tools::wallet_public::escrow_contract_details::contract_released_cancelled;
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == contract_expected_state, false, "Alice has invalid contract state: " << contracts.begin()->second.state << " expected: " << contract_expected_state);

  uint64_t alice_balance_end = alice_wlt->balance();
  alice_balance_expected = m_alice_bob_start_amount - 2 * TESTS_DEFAULT_FEE;
  CHECK_AND_ASSERT_MES(alice_balance_end == alice_balance_expected, false, "Alice has incorrect balance: " << alice_balance_end << ", expected: " << alice_balance_expected);

  bob_wlt->refresh();
  contracts.clear();
  r = bob_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Bob failed");
  CHECK_AND_ASSERT_MES(contracts.size() == 1, false, "Bob has incorrect number of contracts: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.private_detailes == m_cpd, false, "Bob has invalid contract's private details");
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == contract_expected_state, false, "Bob has invalid contract state: " << contracts.begin()->second.state << " expected: " << contract_expected_state);

  uint64_t bob_balance_end = bob_wlt->balance();
  bob_balance_expected = m_alice_bob_start_amount - m_bob_fee_release - TESTS_DEFAULT_FEE;
  CHECK_AND_ASSERT_MES(bob_balance_end == bob_balance_expected, false, "Bob has incorrect balance: " << bob_balance_end << ", expected: " << bob_balance_expected);

  return true;
}

bool escrow_incorrect_cancel_proposal::check_incorrect_cancel_proposal(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::string param = boost::get<callback_entry>(events[ev_index]).callback_params;
  LOG_PRINT_L0("check_incorrect_cancel_proposal (" << param << ")");

  bool r = check_incorrect_cancel_proposal_internal(c, ev_index, events);
  if (r)
  {
    LOG_PRINT_GREEN("check_incorrect_cancel_proposal (" << param << ") succeeded!", LOG_LEVEL_0);
  }
  else
  {
    LOG_PRINT_RED("check_incorrect_cancel_proposal (" << param << ") failed!", LOG_LEVEL_0);
  }

  return r;
}

bool escrow_incorrect_cancel_proposal::check_incorrect_cancel_proposal_internal(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();
  std::stringstream ss;
  alice_wlt->dump_trunsfers(ss, false);
  LOG_PRINT_L0("Alice transfers: " << ENDL << ss.str());
  uint64_t alice_balance = alice_wlt->balance();
  uint64_t alice_balance_expected = m_alice_bob_start_amount - m_cpd.amount_a_pledge - m_cpd.amount_to_pay - TESTS_DEFAULT_FEE - TESTS_DEFAULT_FEE; // one fee for escrow request, second - for cancel request
  CHECK_AND_ASSERT_MES(alice_balance == alice_balance_expected, false, "Alice has incorrect balance: " << alice_balance << ", expected: " << alice_balance_expected);
  
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  bob_wlt->refresh();
  uint64_t bob_balance = bob_wlt->balance();
  uint64_t bob_balance_expected = m_alice_bob_start_amount - m_cpd.amount_b_pledge - m_bob_fee_release - TESTS_DEFAULT_FEE;
  CHECK_AND_ASSERT_MES(bob_balance == bob_balance_expected, false, "Bob has incorrect balance: " << bob_balance << ", expected: " << bob_balance_expected);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  tools::escrow_contracts_container contracts;
  r = alice_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Alice failed");
  CHECK_AND_ASSERT_MES(contracts.size() == 1, false, "Alice has incorrect number of contracts: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.private_detailes == m_cpd, false, "Alice has invalid contract's private details");
  //CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_rpc::escrow_contract_details::contract_cancel_proposal_sent, false, "Alice has invalid contract state: " << contracts.begin()->second.state);

  contracts.clear();
  r = bob_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Bob failed");
  CHECK_AND_ASSERT_MES(contracts.size() == 1, false, "Bob has incorrect number of contracts: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.private_detailes == m_cpd, false, "Bob has invalid contract's private details");

  crypto::hash ms_id = contracts.begin()->first;

  bool exception_on_cancel_contract = false;
  try
  {
    bob_wlt->accept_cancel_contract(ms_id);
  }
  catch(std::exception& e)
  {
    exception_on_cancel_contract = true;
    LOG_PRINT_GREEN("An exeption was caught when Bob was cancelling incorrect contract " << ms_id << " - this is good. Exception's what(): " << e.what(), LOG_LEVEL_0);
  }

  // check it here in order to let more bugs show off with finish_contract()
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::contract_accepted ||
    contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::contract_cancel_proposal_sent, false, "Bob has invalid contract state: " << contracts.begin()->second.state); // It's ok for Bob to be mistaken about state, for ex: in case of too late cancellation acceptance
  CHECK_AND_ASSERT_MES(exception_on_cancel_contract, false, "Bob was able to cancel escrow contract, requested with invalid cancellation request.");

  return true;
}

//------------------------------------------------------------------------------

escrow_proposal_not_enough_money::escrow_proposal_not_enough_money()
{
  REGISTER_CALLBACK_METHOD(escrow_proposal_not_enough_money, c1);
}

bool escrow_proposal_not_enough_money::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure that send_escrow_proposal() doesn't lock any coins if it fails because of not enough money

  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);
  GENERATE_ACCOUNT(bob_acc);
  m_accounts.push_back(bob_acc);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  MAKE_TX(events, tx_0, miner_acc, alice_acc, MK_TEST_COINS(30), blk_0r); // nb: single output for Alice
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE); // make coins transferred by tx_0 spendable

  DO_CALLBACK(events, "c1");

  return true;
}

bool escrow_proposal_not_enough_money::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  alice_wlt->refresh();

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", MK_TEST_COINS(30), 0, MK_TEST_COINS(30), 0, 0), false, "");

  std::deque<tools::transfer_details> transfers;
  alice_wlt->get_transfers(transfers);
  CHECK_AND_ASSERT_MES(transfers.size() == 1, false, "Incorrect transfers size: " << transfers.size());

  bc_services::contract_private_details cpd = AUTO_VAL_INIT(cpd);
  cpd.amount_a_pledge = MK_TEST_COINS(5);
  cpd.amount_b_pledge = MK_TEST_COINS(7);
  cpd.amount_to_pay   = MK_TEST_COINS(8);
  cpd.a_addr          = m_accounts[ALICE_ACC_IDX].get_public_address();
  cpd.b_addr          = m_accounts[BOB_ACC_IDX].get_public_address();
  
  transaction proposal_tx = AUTO_VAL_INIT(proposal_tx);
  transaction escrow_template_tx = AUTO_VAL_INIT(escrow_template_tx);
  bool r = false;
  try
  {
    uint64_t expiration_time = test_core_time::get_time() + 10; // ten seconds
    alice_wlt->send_escrow_proposal(cpd, 0, 0, expiration_time, TESTS_DEFAULT_FEE, COIN * 10000000, "", proposal_tx, escrow_template_tx); // use too big b_release_fee just to make sure it does not affects anything on that stage
  }
  catch(std::exception& e)
  {
    LOG_PRINT_GREEN("A -> send_escrow_proposal failed with exception: \"" << e.what() << "\" as expected.", LOG_LEVEL_0);
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "A -> send_escrow_proposal succeeded, but expected to fail");

  // no tx should be sent
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // wallet's balance must remain the same
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", MK_TEST_COINS(30), 0, MK_TEST_COINS(30), 0, 0), false, "");

  // mine few blocks to make sure there's no problem
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 5);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  // and check balance again
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", MK_TEST_COINS(30), 0, MK_TEST_COINS(30), 0, 0), false, "");

  return true;
}
//------------------------------------------------------------------------------

escrow_cancellation_and_tx_order::escrow_cancellation_and_tx_order()
{
  REGISTER_CALLBACK_METHOD(escrow_cancellation_and_tx_order, c1);
}

bool escrow_cancellation_and_tx_order::generate(std::vector<test_event_entry>& events) const
{
// Test idea: 

  m_accounts.resize(TOTAL_ACCS_COUNT);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts[MINER_ACC_IDX] = miner_acc;
  GENERATE_ACCOUNT(alice_acc);
  m_accounts[ALICE_ACC_IDX] = alice_acc;
  GENERATE_ACCOUNT(bob_acc);
  m_accounts[BOB_ACC_IDX] = bob_acc;

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  bool r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), MK_TEST_COINS(200), 20, TESTS_DEFAULT_FEE, tx_0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_0);

  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), bob_acc.get_public_address(), MK_TEST_COINS(200), 20, TESTS_DEFAULT_FEE, tx_1);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_1);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({tx_0, tx_1}));

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE); // make coins spendable

  DO_CALLBACK(events, "c1");

  return true;
}

bool escrow_cancellation_and_tx_order::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false, stub_bool = false;
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  alice_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", MK_TEST_COINS(200), 0, MK_TEST_COINS(200), 0, 0), false, "");

  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, m_accounts[BOB_ACC_IDX]);
  bob_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", MK_TEST_COINS(200), 0, MK_TEST_COINS(200), 0, 0), false, "");

  // Alice sends escrow proposal
  bc_services::contract_private_details cpd = AUTO_VAL_INIT(cpd);
  cpd.amount_a_pledge = MK_TEST_COINS(5);
  cpd.amount_b_pledge = MK_TEST_COINS(7);
  cpd.amount_to_pay   = MK_TEST_COINS(8);
  cpd.a_addr          = m_accounts[ALICE_ACC_IDX].get_public_address();
  cpd.b_addr          = m_accounts[BOB_ACC_IDX].get_public_address();
  
  transaction proposal_tx = AUTO_VAL_INIT(proposal_tx);
  transaction escrow_template_tx = AUTO_VAL_INIT(escrow_template_tx);
  uint64_t expiration_time = test_core_time::get_time() + 60;
  LOG_PRINT_GREEN("\n" "alice_wlt->send_escrow_proposal()", LOG_LEVEL_0);
  alice_wlt->send_escrow_proposal(cpd, 0, 0, expiration_time, TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, "", proposal_tx, escrow_template_tx);

  tools::escrow_contracts_container contracts;
  r = alice_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Alice failed");
  CHECK_AND_ASSERT_MES(contracts.size() == 1, false, "Alice has incorrect number of contracts: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.private_detailes == cpd, false, "Alice has invalid contract's private details");
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::proposal_sent, false, "Alice has invalid contract state: " << tools::wallet_public::get_escrow_contract_state_name(contracts.begin()->second.state));
  crypto::hash contract_id = contracts.begin()->first;

  // mine a block, containing escrow proposal tx
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  bob_wlt->refresh();
  contracts.clear();
  r = bob_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Bob failed");
  CHECK_AND_ASSERT_MES(contracts.size() == 1, false, "Bob has incorrect number of contracts: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.private_detailes == cpd, false, "Bob has invalid contract's private details");
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::proposal_sent, false, "Bob has invalid contract state: " << tools::wallet_public::get_escrow_contract_state_name(contracts.begin()->second.state));
  
  // Bob accepts the proposal
  LOG_PRINT_GREEN("\n" "bob_wlt->accept_proposal()", LOG_LEVEL_0);
  bob_wlt->accept_proposal(contract_id, TESTS_DEFAULT_FEE);

  // mine a block containing contract acceptance
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  alice_wlt->refresh();
  contracts.clear();
  r = alice_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Alice failed");
  CHECK_AND_ASSERT_MES(contracts.size() == 1, false, "Alice has incorrect number of contracts: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.private_detailes == cpd, false, "Alice has invalid contract's private details");
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::contract_accepted, false, "Alice has invalid contract state: " << tools::wallet_public::get_escrow_contract_state_name(contracts.begin()->second.state));

  // Alice requests cancellation
  LOG_PRINT_GREEN("\n" "alice_wlt->request_cancel_contract()", LOG_LEVEL_0);
  alice_wlt->request_cancel_contract(contract_id, TESTS_DEFAULT_FEE, 60 * 60);

  // cancel request must be in the pool now
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  std::list<transaction> txs;
  r = c.get_pool_transactions(txs);
  CHECK_AND_ASSERT_MES(r && txs.size() == 1, false, "get_pool_transactions failed");
  transaction cancel_request_tx = txs.front();

  // both parties' contracts should change state to contract_cancel_proposal_sent
  alice_wlt->scan_tx_pool(stub_bool);
  contracts.clear();
  r = alice_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r && contracts.size() == 1, false, "get_contracts() for Alice failed or returned wrong contracts count: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::contract_cancel_proposal_sent, false, "Alice has invalid contract state: " << tools::wallet_public::get_escrow_contract_state_name(contracts.begin()->second.state));

  bob_wlt->refresh();
  bob_wlt->scan_tx_pool(stub_bool);
  contracts.clear();
  r = bob_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r && contracts.size() == 1, false, "get_contracts() for Bob failed or returned wrong contracts count: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::contract_cancel_proposal_sent, false, "Bob has invalid contract state: " << tools::wallet_public::get_escrow_contract_state_name(contracts.begin()->second.state));

  // don't mine block, try to accept cancellation on-the-fly
  LOG_PRINT_GREEN("\n" "bob_wlt->accept_cancel_contract()", LOG_LEVEL_0);
  bob_wlt->accept_cancel_contract(contract_id);
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 2, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  txs.clear();
  r = c.get_pool_transactions(txs);
  CHECK_AND_ASSERT_MES(r && txs.size() == 2, false, "get_pool_transactions failed");
  transaction cancel_request_acceptance_tx = get_transaction_hash(cancel_request_tx) == get_transaction_hash(txs.front()) ? txs.back() : txs.front();

  // both parties' contracts should change state to contract_cancel_proposal_sent
  alice_wlt->scan_tx_pool(stub_bool);
  contracts.clear();
  r = alice_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r && contracts.size() == 1, false, "get_contracts() for Alice failed or returned wrong contracts count: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::contract_released_cancelled, false, "Alice has invalid contract state: " << tools::wallet_public::get_escrow_contract_state_name(contracts.begin()->second.state));

  bob_wlt->scan_tx_pool(stub_bool);
  contracts.clear();
  r = bob_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r && contracts.size() == 1, false, "get_contracts() for Bob failed or returned wrong contracts count: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::contract_released_cancelled, false, "Bob has invalid contract state: " << tools::wallet_public::get_escrow_contract_state_name(contracts.begin()->second.state));

  // mine a block containing only cancel_request_acceptance_tx (this should trigger contracts' states swtiching into contract_released_cancelled) 
  LOG_PRINT_GREEN("\n" "mine_next_pow_block_in_playtime_with_given_txs(cancel_request_acceptance_tx)", LOG_LEVEL_0);
  r = mine_next_pow_block_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), c, std::vector<transaction>({cancel_request_acceptance_tx}));
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime_with_given_txs failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  alice_wlt->refresh();
  alice_wlt->scan_tx_pool(stub_bool);
  contracts.clear();
  r = alice_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r && contracts.size() == 1, false, "get_contracts() for Alice failed or returned wrong contracts count: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::contract_released_cancelled, false, "Alice has invalid contract state: " << tools::wallet_public::get_escrow_contract_state_name(contracts.begin()->second.state));

  bob_wlt->refresh();
  bob_wlt->scan_tx_pool(stub_bool);
  contracts.clear();
  r = bob_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r && contracts.size() == 1, false, "get_contracts() for Bob failed or returned wrong contracts count: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::contract_released_cancelled, false, "Bob has invalid contract state: " << tools::wallet_public::get_escrow_contract_state_name(contracts.begin()->second.state));


  // mine a block containing only cancel_request_tx (this SHOULD NOT trigger contracts' states swtiching into contract_cancel_proposal_sent or anything) 
  LOG_PRINT_GREEN("\n" "mine_next_pow_block_in_playtime_with_given_txs(cancel_request_tx)", LOG_LEVEL_0);
  r = mine_next_pow_block_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), c, std::vector<transaction>({cancel_request_tx}));
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime_with_given_txs failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  alice_wlt->refresh();
  alice_wlt->scan_tx_pool(stub_bool);
  contracts.clear();
  r = alice_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r && contracts.size() == 1, false, "get_contracts() for Alice failed or returned wrong contracts count: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::contract_released_cancelled, false, "Alice has invalid contract state: " << tools::wallet_public::get_escrow_contract_state_name(contracts.begin()->second.state));

  bob_wlt->refresh();
  bob_wlt->scan_tx_pool(stub_bool);
  contracts.clear();
  r = bob_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r && contracts.size() == 1, false, "get_contracts() for Bob failed or returned wrong contracts count: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::contract_released_cancelled, false, "Bob has invalid contract state: " << tools::wallet_public::get_escrow_contract_state_name(contracts.begin()->second.state));

  return true;
}

//------------------------------------------------------------------------------

escrow_cancellation_proposal_expiration::escrow_cancellation_proposal_expiration()
{
  REGISTER_CALLBACK_METHOD(escrow_cancellation_proposal_expiration, c1);
}

bool escrow_cancellation_proposal_expiration::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: check correctness of cancellation request expiration being in tx pool, and in blockchain
  // (cancellation request is never accepted in this test)

  m_accounts.resize(TOTAL_ACCS_COUNT);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts[MINER_ACC_IDX] = miner_acc;
  GENERATE_ACCOUNT(alice_acc);
  m_accounts[ALICE_ACC_IDX] = alice_acc;
  GENERATE_ACCOUNT(bob_acc);
  m_accounts[BOB_ACC_IDX] = bob_acc;

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  bool r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), MK_TEST_COINS(200), 20, TESTS_DEFAULT_FEE, tx_0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_0);

  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), bob_acc.get_public_address(), MK_TEST_COINS(200), 20, TESTS_DEFAULT_FEE, tx_1);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_1);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({tx_0, tx_1}));

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE); // make coins spendable

  DO_CALLBACK(events, "c1");

  return true;
}

bool escrow_cancellation_proposal_expiration::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  uint64_t alice_start_balance = MK_TEST_COINS(200);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, alice_start_balance, true, UINT64_MAX, alice_start_balance), false, "");

  uint64_t bob_start_balance = MK_TEST_COINS(200);
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, m_accounts[BOB_ACC_IDX]);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, bob_start_balance, true, UINT64_MAX, bob_start_balance), false, "");

  // Alice sends escrow proposal
  bc_services::contract_private_details cpd = AUTO_VAL_INIT(cpd);
  cpd.amount_a_pledge = MK_TEST_COINS(5);
  cpd.amount_b_pledge = MK_TEST_COINS(7);
  cpd.amount_to_pay   = MK_TEST_COINS(8);
  cpd.a_addr          = m_accounts[ALICE_ACC_IDX].get_public_address();
  cpd.b_addr          = m_accounts[BOB_ACC_IDX].get_public_address();
  uint64_t b_release_fee = TESTS_DEFAULT_FEE * 2;
  
  transaction proposal_tx = AUTO_VAL_INIT(proposal_tx);
  transaction escrow_template_tx = AUTO_VAL_INIT(escrow_template_tx);
  uint64_t expiration_time = test_core_time::get_time() + 60;
  LOG_PRINT_GREEN("\n" "alice_wlt->send_escrow_proposal()", LOG_LEVEL_0);
  alice_wlt->send_escrow_proposal(cpd, 0, 0, expiration_time, TESTS_DEFAULT_FEE, b_release_fee, "", proposal_tx, escrow_template_tx);

  tools::escrow_contracts_container contracts;
  r = alice_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Alice failed");
  CHECK_AND_ASSERT_MES(contracts.size() == 1, false, "Alice has incorrect number of contracts: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.private_detailes == cpd, false, "Alice has invalid contract's private details");
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::proposal_sent, false, "Alice has invalid contract state: " << tools::wallet_public::get_escrow_contract_state_name(contracts.begin()->second.state));
  crypto::hash contract_id = contracts.begin()->first;

  uint64_t alice_blocked_transfers_sum = 0;
  CHECK_AND_ASSERT_MES(estimate_wallet_balance_blocked_for_escrow(*alice_wlt.get(), alice_blocked_transfers_sum), false, "");

  // mine a block, containing escrow proposal tx
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  bob_wlt->refresh();
  contracts.clear();
  r = bob_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Bob failed");
  CHECK_AND_ASSERT_MES(contracts.size() == 1, false, "Bob has incorrect number of contracts: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.private_detailes == cpd, false, "Bob has invalid contract's private details");
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::proposal_sent, false, "Bob has invalid contract state: " << tools::wallet_public::get_escrow_contract_state_name(contracts.begin()->second.state));

  // Bob accepts the proposal
  LOG_PRINT_GREEN("\n" "bob_wlt->accept_proposal()", LOG_LEVEL_0);
  bob_wlt->accept_proposal(contract_id, TESTS_DEFAULT_FEE);

  uint64_t bob_blocked_transfers_sum = 0;
  CHECK_AND_ASSERT_MES(estimate_wallet_balance_blocked_for_escrow(*bob_wlt.get(), bob_blocked_transfers_sum), false, "");
  
  // mine a block containing contract acceptance
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  alice_wlt->refresh();
  contracts.clear();
  r = alice_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r, false, "get_contracts() for Alice failed");
  CHECK_AND_ASSERT_MES(contracts.size() == 1, false, "Alice has incorrect number of contracts: " << contracts.size());
  CHECK_AND_ASSERT_MES(contracts.begin()->second.private_detailes == cpd, false, "Alice has invalid contract's private details");
  CHECK_AND_ASSERT_MES(contracts.begin()->second.state == tools::wallet_public::escrow_contract_details::contract_accepted, false, "Alice has invalid contract state: " << tools::wallet_public::get_escrow_contract_state_name(contracts.begin()->second.state));

  // ===== (1/2) =====
  // Alice requests cancellation for the first time -- this request will not be included into a block and will expire in tx pool
  uint64_t cancel_request_expiration_period = 3; // seconds
  LOG_PRINT_GREEN("\n" "alice_wlt->request_cancel_contract()", LOG_LEVEL_0);
  alice_wlt->request_cancel_contract(contract_id, TESTS_DEFAULT_FEE, cancel_request_expiration_period);
  uint64_t cancel_request_time = test_core_time::get_time(); // remember current time in order to check expiration
  LOG_PRINT_YELLOW(">>>>>>>>>>>>>>>>>>>>>> (1/2) cancel_request_time = " << cancel_request_time << " + period = " << cancel_request_expiration_period + cancel_request_time, LOG_LEVEL_0);

  // cancel request should be in the pool now
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  std::list<transaction> txs;
  r = c.get_pool_transactions(txs);
  CHECK_AND_ASSERT_MES(r && txs.size() == 1, false, "get_pool_transactions failed");
  transaction cancel_request_tx = txs.front();

  transaction tx_cancel_template = AUTO_VAL_INIT(tx_cancel_template);
  CHECK_AND_ASSERT_MES(extract_cancellation_template_tx_from_request_tx(cancel_request_tx, m_accounts[BOB_ACC_IDX].get_keys(), tx_cancel_template), false, "extract_calcellation_template_tx_from_request_tx failed");

  // contract state in wallets should be cancel_proposal_sent for both parties
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Alice", alice_wlt, tools::wallet_public::escrow_contract_details::contract_cancel_proposal_sent, 0), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::contract_cancel_proposal_sent, 1), false, "");

  // mine a few blocks with no txs to shift expiration median
  for(size_t i = 0; i < 7; ++i)
  {
    r = mine_next_pow_block_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), c, std::vector<transaction>());
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime_with_given_txs failed");
  }
  // cancellation request is still in the pool
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // make sure enough time has passed for cancellation accept template to be expired
  CHECK_AND_ASSERT_MES(is_tx_expired(tx_cancel_template, c.get_blockchain_storage().get_tx_expiration_median()), false, "tx_cancel_template did not expire");

  // try to accept cancellation request - should fail
  r = false;
  try
  {
    LOG_PRINT_GREEN("%%%%% Bob : accept_cancel_contract()", LOG_LEVEL_0);
    bob_wlt->accept_cancel_contract(contract_id);
  }
  catch (std::exception& e)
  {
    r = true;
    LOG_PRINT_GREEN("GOOD: unable to call accept_cancel_contract(), what: " << e.what(), LOG_LEVEL_0);
  }
  CHECK_AND_ASSERT_MES(r, false, "accept_cancel_contract succeeded, but it was excepted to fail");

  // as cancellation request is expired, contract state should be contract_accepted for both
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Alice", alice_wlt, tools::wallet_public::escrow_contract_details::contract_accepted, 7), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::contract_accepted, 7), false, "");

  // mine a block containing cancel_request_tx (already expired) -- should be okay
  LOG_PRINT_GREEN("\n\n\n" "mine_next_pow_block_in_playtime() -- including expires contract cancellation request" "\n\n", LOG_LEVEL_0);
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // expired cancellation request--even included into a block--should be ignored, contract state remains the same
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Alice", alice_wlt, tools::wallet_public::escrow_contract_details::contract_accepted, 1), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::contract_accepted, 1), false, "");

  // check balances
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", alice_start_balance - TESTS_DEFAULT_FEE - cpd.amount_a_pledge - cpd.amount_to_pay - TESTS_DEFAULT_FEE), false, "");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", bob_start_balance - TESTS_DEFAULT_FEE - b_release_fee - cpd.amount_b_pledge), false, "");


  // ===== (2/2) =====
  // Alice requests cancellation for the second time -- this request will be included into a block and then will expire
  cancel_request_expiration_period = 3 * DIFFICULTY_POW_TARGET + 15;
  LOG_PRINT_GREEN("\n" "alice_wlt->request_cancel_contract()", LOG_LEVEL_0);
  alice_wlt->request_cancel_contract(contract_id, TESTS_DEFAULT_FEE, cancel_request_expiration_period);
  cancel_request_time = test_core_time::get_time(); // remember current time in order to check expiration
  LOG_PRINT_YELLOW(">>>>>>>>>>>>>>>>>>>>>> (2/2) cancel_request_time = " << cancel_request_time << " + period = " << cancel_request_expiration_period + cancel_request_time, LOG_LEVEL_0);

  // cancel request must be in the pool now
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  txs.clear();
  r = c.get_pool_transactions(txs);
  CHECK_AND_ASSERT_MES(r && txs.size() == 1, false, "get_pool_transactions failed");
  cancel_request_tx = txs.front();
  CHECK_AND_ASSERT_MES(extract_cancellation_template_tx_from_request_tx(cancel_request_tx, m_accounts[BOB_ACC_IDX].get_keys(), tx_cancel_template), false, "extract_calcellation_template_tx_from_request_tx failed");

  // contract state in wallets should be cancel_proposal_sent for both parties, as new cancellation request was handled
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Alice", alice_wlt, tools::wallet_public::escrow_contract_details::contract_cancel_proposal_sent, 0), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::contract_cancel_proposal_sent, 0), false, "");

  // mine a block containing cancel_request_tx
  LOG_PRINT_GREEN("\n" "mine_next_pow_block_in_playtime()", LOG_LEVEL_0);
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // contracts' states should not change after receiving a block
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Alice", alice_wlt, tools::wallet_public::escrow_contract_details::contract_cancel_proposal_sent, 1), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::contract_cancel_proposal_sent, 1), false, "");

  // mine one more block (it's necessary for triggering expiration checks in wallets and shifting an expiration ts median)
  LOG_PRINT_GREEN("\n" "mine_next_pow_block_in_playtime()", LOG_LEVEL_0);
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 7);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // make sure enough time has passed for cancellation accept template to be expired
  CHECK_AND_ASSERT_MES(is_tx_expired(tx_cancel_template, c.get_blockchain_storage().get_tx_expiration_median()), false, "tx_cancel_template did not expire");

  // try to accept cancellation request - should fail
  r = false;
  try
  {
    LOG_PRINT_GREEN("%%%%% Bob : accept_cancel_contract()", LOG_LEVEL_0);
    bob_wlt->accept_cancel_contract(contract_id);
  }
  catch (std::exception& e)
  {
    r = true;
    LOG_PRINT_GREEN("GOOD: unable to call accept_cancel_contract(), what: " << e.what(), LOG_LEVEL_0);
  }
  CHECK_AND_ASSERT_MES(r, false, "accept_cancel_contract succeeded, but it was excepted to fail");

  // check contracts again - their state should change back to contract_accepted
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Alice", alice_wlt, tools::wallet_public::escrow_contract_details::contract_accepted, 7), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::contract_accepted, 7), false, "");

  // check balances
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", alice_start_balance - TESTS_DEFAULT_FEE - cpd.amount_a_pledge - cpd.amount_to_pay - TESTS_DEFAULT_FEE - TESTS_DEFAULT_FEE), false, "");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", bob_start_balance - TESTS_DEFAULT_FEE - b_release_fee - cpd.amount_b_pledge), false, "");

  return true;
}

//------------------------------------------------------------------------------

escrow_cancellation_acceptance_expiration::escrow_cancellation_acceptance_expiration()
{
  REGISTER_CALLBACK_METHOD(escrow_cancellation_acceptance_expiration, c1);
}

bool escrow_cancellation_acceptance_expiration::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: cancel acceptance tx is stuck in the pool till it's expired.
  // Make sure contract is still in "contract_accepted" state and pledges are not returned.

  m_accounts.resize(TOTAL_ACCS_COUNT);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts[MINER_ACC_IDX] = miner_acc;
  GENERATE_ACCOUNT(alice_acc);
  m_accounts[ALICE_ACC_IDX] = alice_acc;
  GENERATE_ACCOUNT(bob_acc);
  m_accounts[BOB_ACC_IDX] = bob_acc;

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  bool r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), MK_TEST_COINS(100), 20, TESTS_DEFAULT_FEE, tx_0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_0);

  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), bob_acc.get_public_address(), MK_TEST_COINS(100), 20, TESTS_DEFAULT_FEE, tx_1);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_1);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({tx_0, tx_1}));

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE); // make coins spendable

  DO_CALLBACK(events, "c1");

  return true;
}

bool escrow_cancellation_acceptance_expiration::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  uint64_t alice_start_balance = MK_TEST_COINS(100);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, alice_start_balance, true, UINT64_MAX, alice_start_balance), false, "");

  uint64_t bob_start_balance = MK_TEST_COINS(100);
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, m_accounts[BOB_ACC_IDX]);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, bob_start_balance, true, UINT64_MAX, bob_start_balance), false, "");

  // Alice sends escrow proposal
  bc_services::contract_private_details cpd = AUTO_VAL_INIT(cpd);
  cpd.amount_a_pledge = MK_TEST_COINS(5);
  cpd.amount_b_pledge = MK_TEST_COINS(7);
  cpd.amount_to_pay   = MK_TEST_COINS(3);
  cpd.a_addr          = m_accounts[ALICE_ACC_IDX].get_public_address();
  cpd.b_addr          = m_accounts[BOB_ACC_IDX].get_public_address();
  uint64_t b_release_fee = TESTS_DEFAULT_FEE * 2;
  
  transaction proposal_tx = AUTO_VAL_INIT(proposal_tx);
  transaction escrow_template_tx = AUTO_VAL_INIT(escrow_template_tx);
  uint64_t expiration_time = test_core_time::get_time() + 60;
  LOG_PRINT_GREEN("\n" "alice_wlt->send_escrow_proposal()", LOG_LEVEL_0);
  alice_wlt->send_escrow_proposal(cpd, 0, 0, expiration_time, TX_DEFAULT_FEE, b_release_fee, "", proposal_tx, escrow_template_tx);

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Alice", alice_wlt, tools::wallet_public::escrow_contract_details::proposal_sent), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::proposal_sent), false, "");
  
  tools::escrow_contracts_container contracts;
  r = alice_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r && contracts.size() == 1, false, "get_contracts() for Alice failed");
  crypto::hash contract_id = contracts.begin()->first;

  //uint64_t alice_blocked_transfers_sum = 0;
  //CHECK_AND_ASSERT_MES(estimate_wallet_balance_blocked_for_escrow(*alice_wlt.get(), alice_blocked_transfers_sum, false), false, "");

  // mine a block, containing escrow proposal tx
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::proposal_sent), false, "");

  // Bob accepts the proposal
  LOG_PRINT_GREEN("\n" "bob_wlt->accept_proposal()", LOG_LEVEL_0);
  bob_wlt->accept_proposal(contract_id, TESTS_DEFAULT_FEE);

  //uint64_t bob_blocked_transfers_sum = 0;
  //CHECK_AND_ASSERT_MES(estimate_wallet_balance_blocked_for_escrow(*bob_wlt.get(), bob_blocked_transfers_sum, false), false, "");
  
  // mine a block containing contract acceptance
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Alice", alice_wlt, tools::wallet_public::escrow_contract_details::contract_accepted, 2), false, "");

  // Alice requests cancellation
  uint64_t cancel_request_expiration_period = DIFFICULTY_TOTAL_TARGET;
  LOG_PRINT_GREEN("\n" "alice_wlt->request_cancel_contract()", LOG_LEVEL_0);
  alice_wlt->request_cancel_contract(contract_id, TESTS_DEFAULT_FEE, cancel_request_expiration_period);
  
  // confirm cancellation request in blockchain
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  CHECK_AND_ASSERT_MES(mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c), false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // contract state in wallets should be cancel_proposal_sent for both parties
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Alice", alice_wlt, tools::wallet_public::escrow_contract_details::contract_cancel_proposal_sent, 1), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::contract_cancel_proposal_sent, 2), false, "");

  LOG_PRINT_GREEN("%%%%% Bob : accept_cancel_contract()", LOG_LEVEL_0);
  bob_wlt->accept_cancel_contract(contract_id);

  // cancel request acceptance tx should be in the pool now
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  std::list<transaction> txs;
  r = c.get_pool_transactions(txs);
  CHECK_AND_ASSERT_MES(r && txs.size() == 1, false, "get_pool_transactions failed");
  transaction cancel_accept_tx = txs.front();

  // contract state in wallets should be cancel_proposal_sent for both parties
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Alice", alice_wlt, tools::wallet_public::escrow_contract_details::contract_released_cancelled, 0), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::contract_released_cancelled, 0), false, "");

  // mine a few blocks with no txs to shift expiration median, cancellation acceptance is still in the pool
  for(size_t i = 0; i < 7; ++i)
  {
    r = mine_next_pow_block_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), c, std::vector<transaction>());
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime_with_given_txs failed");
  }
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // make sure enough time has passed for cancellation acceptance to be expired
  CHECK_AND_ASSERT_MES(is_tx_expired(cancel_accept_tx, c.get_blockchain_storage().get_tx_expiration_median()), false, "cancel_accept_tx did not expire");

  // as cancellation acceptance is expired, contract state should be contract_accepted for both
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Alice", alice_wlt, tools::wallet_public::escrow_contract_details::contract_accepted, 7), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::contract_accepted, 7), false, "");

  // check final balances
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", alice_start_balance - TESTS_DEFAULT_FEE - cpd.amount_a_pledge - cpd.amount_to_pay - TESTS_DEFAULT_FEE), false, "");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", bob_start_balance - TESTS_DEFAULT_FEE - b_release_fee - cpd.amount_b_pledge), false, "");



  LOG_PRINT_GREEN("%%%%% tx_pool::on_idle()", LOG_LEVEL_0);
  c.get_tx_pool().on_idle();
  // expired tx should be removed from tx pool
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // mine a block with cancel_accept_tx (already expired). It should be rejected
  r = mine_next_pow_block_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), c, std::vector<transaction>({ cancel_accept_tx }));
  CHECK_AND_ASSERT_MES(!r, false, "mine_next_pow_block_in_playtime_with_given_txs should have been failed as block contains expired tx");

  // no changes with contract state expected
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Alice", alice_wlt, tools::wallet_public::escrow_contract_details::contract_accepted, 0), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_1_contract_state("Bob", bob_wlt, tools::wallet_public::escrow_contract_details::contract_accepted, 0), false, "");

  // no changes with balances expected
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", alice_start_balance - TESTS_DEFAULT_FEE - cpd.amount_a_pledge - cpd.amount_to_pay - TESTS_DEFAULT_FEE), false, "");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", bob_start_balance - TESTS_DEFAULT_FEE - b_release_fee - cpd.amount_b_pledge), false, "");

  return true;
}

//------------------------------------------------------------------------------

escrow_proposal_acceptance_in_alt_chain::escrow_proposal_acceptance_in_alt_chain()
{
  REGISTER_CALLBACK_METHOD(escrow_proposal_acceptance_in_alt_chain, c1);
}

bool escrow_proposal_acceptance_in_alt_chain::generate(std::vector<test_event_entry>& events) const
{
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc = m_accounts[BOB_ACC_IDX];     bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  bool r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), MK_TEST_COINS(200), 20, TESTS_DEFAULT_FEE, tx_0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_0);

  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), bob_acc.get_public_address(), MK_TEST_COINS(200), 20, TESTS_DEFAULT_FEE, tx_1);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_1);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({tx_0, tx_1}));

  // prepare contract details
  bc_services::contract_private_details cpd = AUTO_VAL_INIT(cpd);
  cpd.amount_a_pledge = TESTS_DEFAULT_FEE * 12;
  cpd.amount_b_pledge = TESTS_DEFAULT_FEE * 13;
  cpd.amount_to_pay = TESTS_DEFAULT_FEE * 3;
  cpd.a_addr = alice_acc.get_public_address();
  cpd.b_addr = bob_acc.get_public_address();
  uint64_t bob_fee_release = 10 * TESTS_DEFAULT_FEE; // Alice states that Bob should pay this much money for upcoming contract release (which will be sent by Alice)

  std::vector<tx_source_entry> used_sources;

  // escrow proposal
  bc_services::proposal_body prop = AUTO_VAL_INIT(prop);
  uint64_t tx_version = get_tx_version(get_block_height(blk_1), m_hardforks);
  transaction escrow_proposal_tx = AUTO_VAL_INIT(escrow_proposal_tx);
  r = build_custom_escrow_proposal(events, blk_1, alice_acc.get_keys(), cpd, 0, 0, 0, blk_1.timestamp + 36000, 0, TESTS_DEFAULT_FEE, bob_fee_release, eccf_normal, tx_version, escrow_proposal_tx, used_sources, &prop);
  CHECK_AND_ASSERT_MES(r, false, "build_custom_escrow_proposal failed");
  events.push_back(escrow_proposal_tx);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, escrow_proposal_tx);

  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);

  // escrow proposal acceptance
  tx_version = get_tx_version(get_block_height(blk_2), m_hardforks);
  transaction escrow_normal_acceptance_tx = prop.tx_template;
  uint64_t normal_acceptance_mask = eccf_normal; //eccf_acceptance_no_tsa_compression;
  r = build_custom_escrow_accept_proposal(events, blk_2, 0, bob_acc.get_keys(), cpd, 0, 0, 0, 0, TESTS_DEFAULT_FEE, bob_fee_release, normal_acceptance_mask, prop.tx_onetime_secret_key, tx_version, escrow_normal_acceptance_tx, used_sources);
  CHECK_AND_ASSERT_MES(r, false, "build_custom_escrow_accept_proposal failed");

  events.push_back(escrow_normal_acceptance_tx);
  MAKE_NEXT_BLOCK_TX1(events, blk_3a, blk_2, miner_acc, escrow_normal_acceptance_tx);


  //DO_CALLBACK(events, "c1");

  return true;
}

bool escrow_proposal_acceptance_in_alt_chain::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{

  //mine_next_pow_block_in_playtime()
  return true;
}

//------------------------------------------------------------------------------

escrow_zero_amounts::escrow_zero_amounts()
{
  REGISTER_CALLBACK_METHOD(escrow_zero_amounts, c1);
}

bool escrow_zero_amounts::generate(std::vector<test_event_entry>& events) const
{
  // Try to accept contracts having (b pledge + amount to pay) == 0
  // It should not be accepted by wallet (ignored), accepting such contracts is impossible

  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc = m_accounts[BOB_ACC_IDX];     bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  bool r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), MK_TEST_COINS(200), 20, TESTS_DEFAULT_FEE, tx_0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_0);

  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), bob_acc.get_public_address(), MK_TEST_COINS(200), 20, TESTS_DEFAULT_FEE, tx_1);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_1);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({tx_0, tx_1}));

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK(events, "c1");
  return true;
}

bool escrow_zero_amounts::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  bob_wlt->refresh();

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(200), true, UINT64_MAX, MK_TEST_COINS(200)), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, MK_TEST_COINS(200), true, UINT64_MAX, MK_TEST_COINS(200)), false, "");
  
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  // a_pledge > 0, b_pledge + amount_to_pay = 0
  bc_services::contract_private_details cpd = AUTO_VAL_INIT(cpd);
  cpd.amount_a_pledge = MK_TEST_COINS(10);
  cpd.amount_b_pledge = 0;
  cpd.amount_to_pay = 0;
  cpd.a_addr = m_accounts[ALICE_ACC_IDX].get_public_address();
  cpd.b_addr = m_accounts[BOB_ACC_IDX].get_public_address();
  cpd.comment = get_random_text(1024);
  cpd.title = get_random_text(7);

  transaction proposal_tx = AUTO_VAL_INIT(proposal_tx);
  transaction escrow_template_tx = AUTO_VAL_INIT(escrow_template_tx);
  alice_wlt->send_escrow_proposal(cpd, 0, 0, 0, TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, "", proposal_tx, escrow_template_tx);

  crypto::hash ms_id = get_multisig_out_id(escrow_template_tx, get_multisig_out_index(escrow_template_tx.vout));
  CHECK_AND_ASSERT_MES(ms_id != null_hash, false, "Can't obtain multisig id from escrow template tx");
  LOG_PRINT_L0("contract 1: " << ms_id);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  bob_wlt->refresh();

  bool caught = false;
  try
  {
    bob_wlt->accept_proposal(ms_id, TESTS_DEFAULT_FEE);
  }
  catch (tools::error::wallet_internal_error &e)
  {
    LOG_PRINT_L0("caught: " << e.what());
    caught = true;
  }

  CHECK_AND_ASSERT_MES(caught, false, "incorrect proposal was accepted");

  return true;
}

//------------------------------------------------------------------------------

escrow_balance::escrow_balance()
  : m_alice_bob_start_amount(0)
  , m_alice_bob_start_chunk_amount(0)
{
  REGISTER_CALLBACK_METHOD(escrow_balance, c1);
}

bool escrow_balance::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: carefull check balances on each stage of escrow contract (including cancellation req and acc):
  // 1) within wallet callback in the middle of contract function call
  // 2) after tx was sent to network but not yet confirmed
  // 3) after tx was confirmed

  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  m_alice_bob_start_amount = MK_TEST_COINS(200);
  //uint64_t amount_chunks = 10;
  m_alice_bob_start_chunk_amount = m_alice_bob_start_amount / 10;

  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  bool r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), m_alice_bob_start_amount, 10, TESTS_DEFAULT_FEE, tx_0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_0);

  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), bob_acc.get_public_address(), m_alice_bob_start_amount, 10, TESTS_DEFAULT_FEE, tx_1);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_1);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({tx_0, tx_1}));

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

bool escrow_balance::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  auto alice_bc = std::make_shared<wallet_callback_balance_checker>("Alice");
  alice_wlt->callback(alice_bc);

  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, m_accounts[BOB_ACC_IDX]);
  auto bob_bc = std::make_shared<wallet_callback_balance_checker>("Bob");
  bob_wlt->callback(bob_bc);

  alice_bc->expect_balance(m_alice_bob_start_amount, 0);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt,
    m_alice_bob_start_amount, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount, // unlocked
    0, // mined
    0, // awaited in
    0  // awainted out
    ), false, "");
  CHECK_AND_ASSERT_MES(alice_bc->check(), false, "balance callback check failed, see above");

  bob_bc->expect_balance(m_alice_bob_start_amount, 0);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt,
    m_alice_bob_start_amount, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount, // unlocked
    0, // mined
    0, // awaited in
    0  // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(bob_bc->check(), false, "balance callback check failed, see above");

  //
  // escrow proposal
  //
  bc_services::contract_private_details cpd = AUTO_VAL_INIT(cpd);
  cpd.amount_a_pledge = MK_TEST_COINS(7);
  cpd.amount_b_pledge = MK_TEST_COINS(5);
  cpd.amount_to_pay   = MK_TEST_COINS(3);
  cpd.a_addr          = m_accounts[ALICE_ACC_IDX].get_public_address();
  cpd.b_addr          = m_accounts[BOB_ACC_IDX].get_public_address();
  uint64_t alice_proposal_fee = MK_TEST_COINS(4);
  uint64_t bob_acceptace_fee = MK_TEST_COINS(2);
  uint64_t bob_release_fee = MK_TEST_COINS(9); // Alice states that Bob should pay this much money for upcoming contract release (which will be sent by Alice)
  uint64_t alice_cancellation_request_fee = MK_TEST_COINS(1);

  alice_bc->expect_balance(m_alice_bob_start_amount - alice_proposal_fee, m_alice_bob_start_amount - 2 * m_alice_bob_start_chunk_amount); // balace after sending the proposal 
  
  transaction proposal_tx = AUTO_VAL_INIT(proposal_tx);
  transaction escrow_template_tx = AUTO_VAL_INIT(escrow_template_tx);
  uint64_t expiration_time = test_core_time::get_time() + 60;
  LOG_PRINT_GREEN("\n" "alice_wlt->send_escrow_proposal()", LOG_LEVEL_0);
  alice_wlt->send_escrow_proposal(cpd, 0, 0, expiration_time, alice_proposal_fee, bob_release_fee, "", proposal_tx, escrow_template_tx);

  CHECK_AND_ASSERT_MES(alice_bc->check(), false, "balance callback check failed, see above");

  tools::escrow_contracts_container contracts;
  r = alice_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r && contracts.size() == 1, false, "get_contracts() for Alice failed");
  crypto::hash contract_id = contracts.begin()->first;

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // proposal tx is not confirmed yet
  alice_bc->expect_balance();
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt,
    m_alice_bob_start_amount - alice_proposal_fee, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount - 2 * m_alice_bob_start_chunk_amount, // unlocked
    0, // mined
    0, // awaited in
    0  // awainted out
    ), false, "");
  CHECK_AND_ASSERT_MES(!alice_bc->called(), false, "balance callback check failed");

  // Bob's balance should not change
  bob_bc->expect_balance(m_alice_bob_start_amount, m_alice_bob_start_amount);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt,
    m_alice_bob_start_amount, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount, // unlocked
    0, // mined
    0, // awaited in
    0  // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(bob_bc->check(), false, "balance callback check failed, see above");

  // mine a block to confirm escrow proposal tx
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // proposal tx is confirmed (balances should stay the same)
  alice_bc->expect_balance(m_alice_bob_start_amount - alice_proposal_fee, m_alice_bob_start_amount - 2 * m_alice_bob_start_chunk_amount);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt,
    m_alice_bob_start_amount - alice_proposal_fee, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount - 2 * m_alice_bob_start_chunk_amount, // unlocked
    0, // mined
    0, // awaited in
    0  // awainted out
    ), false, "");
  CHECK_AND_ASSERT_MES(alice_bc->check(), false, "balance callback check failed, see above");

  bob_bc->expect_balance(m_alice_bob_start_amount, m_alice_bob_start_amount);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt,
    m_alice_bob_start_amount, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount, // unlocked
    0, // mined
    0, // awaited in
    0  // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(bob_bc->check(), false, "balance callback check failed, see above");

  //
  // proposal acceptance
  //
  bob_bc->expect_balance(m_alice_bob_start_amount - cpd.amount_b_pledge - bob_release_fee - bob_acceptace_fee, m_alice_bob_start_amount - m_alice_bob_start_chunk_amount);

  LOG_PRINT_GREEN("\n" "bob_wlt->accept_proposal()", LOG_LEVEL_0);
  bob_wlt->accept_proposal(contract_id, bob_acceptace_fee);

  CHECK_AND_ASSERT_MES(bob_bc->check(), false, "balance callback check failed, see above");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // acceptance tx is not confirmed yet
  alice_bc->expect_balance(m_alice_bob_start_amount - alice_proposal_fee - cpd.amount_a_pledge - cpd.amount_to_pay, m_alice_bob_start_amount - 2 * m_alice_bob_start_chunk_amount);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt,
    m_alice_bob_start_amount - alice_proposal_fee - cpd.amount_a_pledge - cpd.amount_to_pay, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount - 2 * m_alice_bob_start_chunk_amount, // unlocked
    0, // mined
    0, // awaited in
    cpd.amount_a_pledge + cpd.amount_to_pay // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(alice_bc->check(), false, "balance callback check failed, see above");

  bob_bc->expect_balance();
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt,
    m_alice_bob_start_amount - cpd.amount_b_pledge - bob_release_fee - bob_acceptace_fee, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount - 1 * m_alice_bob_start_chunk_amount, // unlocked
    0, // mined
    0, // awaited in
    cpd.amount_b_pledge + bob_release_fee // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(!bob_bc->called(), false, "balance callback check failed");

  // mine a block containing contract acceptance
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // acceptance tx is confirmed
  alice_bc->expect_balance(m_alice_bob_start_amount - alice_proposal_fee - cpd.amount_a_pledge - cpd.amount_to_pay, m_alice_bob_start_amount - 2 * m_alice_bob_start_chunk_amount);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt,
    m_alice_bob_start_amount - alice_proposal_fee - cpd.amount_a_pledge - cpd.amount_to_pay, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount - 2 * m_alice_bob_start_chunk_amount, // unlocked
    0, // mined
    0, // awaited in
    0  // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(alice_bc->check(), false, "balance callback check failed, see above");

  bob_bc->expect_balance(m_alice_bob_start_amount - cpd.amount_b_pledge - bob_release_fee - bob_acceptace_fee, m_alice_bob_start_amount - 1 * m_alice_bob_start_chunk_amount);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt,
    m_alice_bob_start_amount - cpd.amount_b_pledge - bob_release_fee - bob_acceptace_fee, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount - 1 * m_alice_bob_start_chunk_amount, // unlocked
    0, // mined
    0, // awaited in
    0  // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(bob_bc->check(), false, "balance callback check failed, see above");

  //
  // cancellation request
  //
  alice_bc->expect_balance(m_alice_bob_start_amount - alice_proposal_fee - cpd.amount_a_pledge - cpd.amount_to_pay - alice_cancellation_request_fee, m_alice_bob_start_amount - 3 * m_alice_bob_start_chunk_amount);

  LOG_PRINT_GREEN("\n" "alice_wlt->request_cancel_contract()", LOG_LEVEL_0);
  alice_wlt->request_cancel_contract(contract_id, alice_cancellation_request_fee, 60 * 60);

  CHECK_AND_ASSERT_MES(alice_bc->check(), false, "balance callback check failed, see above");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // cancellation request is not confirmed yet
  alice_bc->expect_balance();
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt,
    m_alice_bob_start_amount - alice_proposal_fee - cpd.amount_a_pledge - cpd.amount_to_pay - alice_cancellation_request_fee, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount - 3 * m_alice_bob_start_chunk_amount, // unlocked
    0, // mined
    0, // awaited in
    0  // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(!alice_bc->called(), false, "balance callback check failed");

  bob_bc->expect_balance(m_alice_bob_start_amount - cpd.amount_b_pledge - bob_release_fee - bob_acceptace_fee, m_alice_bob_start_amount - 1 * m_alice_bob_start_chunk_amount);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt,
    m_alice_bob_start_amount - cpd.amount_b_pledge - bob_release_fee - bob_acceptace_fee, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount - 1 * m_alice_bob_start_chunk_amount, // unlocked
    0, // mined
    0, // awaited in
    0  // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(bob_bc->check(), false, "balance callback check failed, see above");

  // mine a block containing cancellation request
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // cancellation request is confirmed
  alice_bc->expect_balance(m_alice_bob_start_amount - alice_proposal_fee - cpd.amount_a_pledge - cpd.amount_to_pay - alice_cancellation_request_fee, m_alice_bob_start_amount - 3 * m_alice_bob_start_chunk_amount);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt,
    m_alice_bob_start_amount - alice_proposal_fee - cpd.amount_a_pledge - cpd.amount_to_pay - alice_cancellation_request_fee, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount - 3 * m_alice_bob_start_chunk_amount, // unlocked
    0, // mined
    0, // awaited in
    0  // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(alice_bc->check(), false, "balance callback check failed, see above");

  bob_bc->expect_balance(m_alice_bob_start_amount - cpd.amount_b_pledge - bob_release_fee - bob_acceptace_fee, m_alice_bob_start_amount - 1 * m_alice_bob_start_chunk_amount);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt,
    m_alice_bob_start_amount - cpd.amount_b_pledge - bob_release_fee - bob_acceptace_fee, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount - 1 * m_alice_bob_start_chunk_amount, // unlocked
    0, // mined
    0, // awaited in
    0  // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(bob_bc->check(), false, "balance callback check failed, see above");

  //
  // cancellation acceptance
  //
  bob_bc->expect_balance();
  
  LOG_PRINT_GREEN("\n" "bob_wlt->accept_cancel_contract()", LOG_LEVEL_0);
  bob_wlt->accept_cancel_contract(contract_id);

  CHECK_AND_ASSERT_MES(!bob_bc->called(), false, "balance callback check failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // cancellation acceptance is not confirmed yet
  alice_bc->expect_balance(m_alice_bob_start_amount - alice_proposal_fee - alice_cancellation_request_fee, m_alice_bob_start_amount - 3 * m_alice_bob_start_chunk_amount);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt,
    m_alice_bob_start_amount - alice_proposal_fee - alice_cancellation_request_fee, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount - 3 * m_alice_bob_start_chunk_amount, // unlocked
    0, // mined
    cpd.amount_a_pledge + cpd.amount_to_pay, // awaited in
    0  // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(alice_bc->check(), false, "balance callback check failed, see above");

  bob_bc->expect_balance(m_alice_bob_start_amount - bob_release_fee - bob_acceptace_fee, m_alice_bob_start_amount - 1 * m_alice_bob_start_chunk_amount);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt,
    m_alice_bob_start_amount - bob_release_fee - bob_acceptace_fee, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount - 1 * m_alice_bob_start_chunk_amount, // unlocked
    0, // mined
    cpd.amount_b_pledge, // awaited in
    0  // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(bob_bc->check(), false, "balance callback check failed, see above");

  // mine a block containing cancellation acceptance
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // cancellation acceptance is confirmed
  alice_bc->expect_balance(m_alice_bob_start_amount - alice_proposal_fee - alice_cancellation_request_fee, m_alice_bob_start_amount - 3 * m_alice_bob_start_chunk_amount);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt,
    m_alice_bob_start_amount - alice_proposal_fee - alice_cancellation_request_fee, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount - 3 * m_alice_bob_start_chunk_amount, // unlocked
    0, // mined
    0, // awaited in
    0  // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(alice_bc->check(), false, "balance callback check failed, see above");

  bob_bc->expect_balance(m_alice_bob_start_amount - bob_release_fee - bob_acceptace_fee, m_alice_bob_start_amount - 1 * m_alice_bob_start_chunk_amount);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt,
    m_alice_bob_start_amount - bob_release_fee - bob_acceptace_fee, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount - 1 * m_alice_bob_start_chunk_amount, // unlocked
    0, // mined
    0, // awaited in
    0  // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(bob_bc->check(), false, "balance callback check failed, see above");


  //
  // Stage 2 : check normal contract workflow
  // don't check balances on request and accept as it was checked above
  //
  
  uint64_t alice_balance_before_stage_2 = m_alice_bob_start_amount - alice_proposal_fee - alice_cancellation_request_fee;
  uint64_t bob_balance_before_stage_2 = m_alice_bob_start_amount - bob_release_fee - bob_acceptace_fee;

  alice_bc->expect_balance(alice_balance_before_stage_2 - alice_proposal_fee, m_alice_bob_start_amount - 5 * m_alice_bob_start_chunk_amount);

  LOG_PRINT_GREEN("\n" "stage2: alice_wlt->send_escrow_proposal()", LOG_LEVEL_0);
  proposal_tx = AUTO_VAL_INIT(proposal_tx);
  escrow_template_tx = AUTO_VAL_INIT(escrow_template_tx);
  alice_wlt->send_escrow_proposal(cpd, 0, 0, expiration_time, alice_proposal_fee, bob_release_fee, "", proposal_tx, escrow_template_tx);

  CHECK_AND_ASSERT_MES(alice_bc->check(), false, "balance callback check failed, see above");

  contracts.clear();
  r = alice_wlt->get_contracts(contracts);
  CHECK_AND_ASSERT_MES(r && contracts.size() == 2, false, "get_contracts() for Alice failed");
  // get new contract id
  if (contract_id != contracts.begin()->first)
    contract_id = contracts.begin()->first;
  else
    contract_id = (++contracts.begin())->first;

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  bob_wlt->refresh();

  bob_bc->expect_balance(bob_balance_before_stage_2 - cpd.amount_b_pledge - bob_release_fee - bob_acceptace_fee, m_alice_bob_start_amount - 2 * m_alice_bob_start_chunk_amount);

  LOG_PRINT_GREEN("\n" "stage2: bob_wlt->accept_proposal()", LOG_LEVEL_0);
  bob_wlt->accept_proposal(contract_id, bob_acceptace_fee);

  CHECK_AND_ASSERT_MES(bob_bc->check(), false, "balance callback check failed, see above");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  alice_bc->expect_balance(alice_balance_before_stage_2 - alice_proposal_fee - cpd.amount_a_pledge - cpd.amount_to_pay, m_alice_bob_start_amount - 5 * m_alice_bob_start_chunk_amount);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt,
    alice_balance_before_stage_2 - alice_proposal_fee - cpd.amount_a_pledge - cpd.amount_to_pay, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount - 5 * m_alice_bob_start_chunk_amount, // unlocked
    0, // mined
    0, // awaited in
    0  // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(alice_bc->check(), false, "balance callback check failed");

  bob_bc->expect_balance(bob_balance_before_stage_2 - cpd.amount_b_pledge - bob_release_fee - bob_acceptace_fee, m_alice_bob_start_amount - 2 * m_alice_bob_start_chunk_amount);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt,
    bob_balance_before_stage_2 - cpd.amount_b_pledge - bob_release_fee - bob_acceptace_fee, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount - 2 * m_alice_bob_start_chunk_amount, // unlocked
    0, // mined
    0, // awaited in
    0  // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(bob_bc->check(), false, "balance callback check failed, see above");

  //
  // contract release
  //

  alice_bc->expect_balance();

  alice_wlt->finish_contract(contract_id, BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL);

  CHECK_AND_ASSERT_MES(!alice_bc->called(), false, "balance callback check failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // contract release tx is unconfirmed
  alice_bc->expect_balance(alice_balance_before_stage_2 - alice_proposal_fee - cpd.amount_to_pay, m_alice_bob_start_amount - 5 * m_alice_bob_start_chunk_amount);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt,
    alice_balance_before_stage_2 - alice_proposal_fee - cpd.amount_to_pay, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount - 5 * m_alice_bob_start_chunk_amount, // unlocked
    0, // mined
    cpd.amount_a_pledge, // awaited in
    0  // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(alice_bc->check(), false, "balance callback check failed, see above");

  bob_bc->expect_balance(bob_balance_before_stage_2 - bob_release_fee - bob_acceptace_fee + cpd.amount_to_pay, m_alice_bob_start_amount - 2 * m_alice_bob_start_chunk_amount);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt,
    bob_balance_before_stage_2 - bob_release_fee - bob_acceptace_fee + cpd.amount_to_pay, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount - 2 * m_alice_bob_start_chunk_amount, // unlocked
    0, // mined
    cpd.amount_b_pledge + cpd.amount_to_pay, // awaited in
    0  // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(bob_bc->check(), false, "balance callback check failed, see above");

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // contract release tx is confirmed
  alice_bc->expect_balance(alice_balance_before_stage_2 - alice_proposal_fee - cpd.amount_to_pay, m_alice_bob_start_amount - 5 * m_alice_bob_start_chunk_amount);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt,
    alice_balance_before_stage_2 - alice_proposal_fee - cpd.amount_to_pay, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount - 5 * m_alice_bob_start_chunk_amount, // unlocked
    0, // mined
    0, // awaited in
    0  // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(alice_bc->check(), false, "balance callback check failed, see above");

  bob_bc->expect_balance(bob_balance_before_stage_2 - bob_release_fee - bob_acceptace_fee + cpd.amount_to_pay, m_alice_bob_start_amount - 2 * m_alice_bob_start_chunk_amount);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt,
    bob_balance_before_stage_2 - bob_release_fee - bob_acceptace_fee + cpd.amount_to_pay, // total
    true, UINT64_MAX,
    m_alice_bob_start_amount - 2 * m_alice_bob_start_chunk_amount, // unlocked
    0, // mined
    0, // awaited in
    0  // awaited out
    ), false, "");
  CHECK_AND_ASSERT_MES(bob_bc->check(), false, "balance callback check failed, see above");

  return true;
}
