// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "escrow_wallet_altchain_test.h"
#include "random_helper.h"
#include "chaingen_helpers.h"
#include "escrow_wallet_common.h"

using namespace epee;
using namespace crypto;
using namespace currency;

//==============================================================================================================================

const char *tx_behaviour_to_cstr(uint32_t v)
{
  switch (v)
  {
  case eam_tx_unknown:          return "tx_unknown";
  case eam_tx_make:             return "tx_make";
  case eam_tx_confirm:          return "tx_confirm";
  case eam_tx_make_and_confirm: return "tx_make_and_confirm";
  default:                      return "invalid!";
  }
}

const char *party_to_cstr(uint32_t v)
{
  switch (v)
  {
  case wallet_test::MINER_ACC_IDX:  return "Miner";
  case wallet_test::ALICE_ACC_IDX:  return "Alice";
  case wallet_test::BOB_ACC_IDX:    return "Bob";
  case wallet_test::CAROL_ACC_IDX:  return "Carol";
  case wallet_test::DAN_ACC_IDX:    return "Dan";
  default:                          return "invalid!";
  }
}

escrow_altchain_meta_impl::escrow_altchain_meta_impl(const eam_test_data_t &etd)
  : m_etd(etd)
  , m_last_block_hash(null_hash)
  , m_last_block_height(0)
{
}

bool escrow_altchain_meta_impl::generate(std::vector<test_event_entry>& events) const
{
  bool r = false;

  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base &miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base &alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base &bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  m_etd.cpd.a_addr = alice_acc.get_public_address();
  m_etd.cpd.b_addr = bob_acc.get_public_address();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // give Alice and Bob 'm_etd.alice_bob_start_amoun' coins for pocket money
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), m_etd.alice_bob_start_amount, m_etd.start_amount_outs_count, TESTS_DEFAULT_FEE, tx_1);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_1);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  transaction tx_2 = AUTO_VAL_INIT(tx_2);
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_1, miner_acc.get_keys(), bob_acc.get_public_address(), m_etd.alice_bob_start_amount, m_etd.start_amount_outs_count, TESTS_DEFAULT_FEE, tx_2);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_2);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, tx_2);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_2r, blk_2, miner_acc, 7 + WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");
  
  return true;
}

bool escrow_altchain_meta_impl::mine_next_block_with_tx(currency::core& c, const currency::transaction& tx)
{
  block b = AUTO_VAL_INIT(b);
  bool r = mine_next_pow_block_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), c, std::vector<transaction>({ tx }), m_last_block_hash, m_last_block_height + 1, &b);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime_with_given_txs failed");
  m_last_block_hash = get_block_hash(b);
  m_last_block_height = get_block_height(b);
  return true;
}

bool escrow_altchain_meta_impl::mine_next_block_with_no_tx(currency::core& c)
{
  block b = AUTO_VAL_INIT(b);
  bool r = mine_next_pow_block_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), c, std::vector<transaction>(), m_last_block_hash, m_last_block_height + 1, &b);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime_with_given_txs failed");
  m_last_block_hash = get_block_hash(b);
  m_last_block_height = get_block_height(b);
  return true;
}

bool escrow_altchain_meta_impl::handle_event_proposal(currency::core& c, std::shared_ptr<tools::wallet2> alice_wlt, const eam_event_proposal& se)
{
  if (se.tx_behavior == eam_tx_make || se.tx_behavior == eam_tx_make_and_confirm)
  {
    try
    {
      transaction escrow_template_tx = AUTO_VAL_INIT(escrow_template_tx);
      m_etd.proposal_tx = AUTO_VAL_INIT(m_etd.proposal_tx);
      alice_wlt->send_escrow_proposal(m_etd.cpd, 0, se.unlock_time, se.expiration_period, se.a_proposal_fee, se.b_release_fee, "", m_etd.proposal_tx, escrow_template_tx);
      m_etd.ms_id = get_multisig_out_id(escrow_template_tx, get_multisig_out_index(escrow_template_tx.vout));
      CHECK_AND_ASSERT_MES(m_etd.ms_id != null_hash, false, "Can't get valid ms id from escrow template tx");
    }
    catch(std::exception& e)
    {
      LOG_ERROR("A -> send_escrow_proposal failed with exception: " << e.what());
      return false;
    }
  }
      
  if (se.tx_behavior == eam_tx_confirm || se.tx_behavior == eam_tx_make_and_confirm)
  {
    CHECK_AND_NO_ASSERT_MES(mine_next_block_with_tx(c, m_etd.proposal_tx), false, "");
  }

  return true;
}

bool escrow_altchain_meta_impl::handle_event_acceptance(currency::core& c, std::shared_ptr<tools::wallet2> bob_wlt, const eam_event_acceptance& se)
{
  if (se.tx_behavior == eam_tx_make || se.tx_behavior == eam_tx_make_and_confirm)
  {
    try
    {
      m_etd.acceptance_tx = AUTO_VAL_INIT(m_etd.acceptance_tx);
      bob_wlt->accept_proposal(m_etd.ms_id, se.b_acceptance_fee, &m_etd.acceptance_tx);
    }
    catch(std::exception& e)
    {
      LOG_ERROR("B -> accept_proposal failed with exception: " << e.what());
      return false;
    }
  }
      
  if (se.tx_behavior == eam_tx_confirm || se.tx_behavior == eam_tx_make_and_confirm)
  {
    CHECK_AND_NO_ASSERT_MES(mine_next_block_with_tx(c, m_etd.acceptance_tx), false, "");
  }
  return true;
}

bool escrow_altchain_meta_impl::handle_event_release(currency::core& c, std::shared_ptr<tools::wallet2> alice_wlt, const eam_event_release& se)
{
  if (se.tx_behavior == eam_tx_make || se.tx_behavior == eam_tx_make_and_confirm)
  {
    try
    {
      m_etd.release_tx = AUTO_VAL_INIT(m_etd.release_tx);
      alice_wlt->finish_contract(m_etd.ms_id, se.burn ? BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_BURN : BC_ESCROW_SERVICE_INSTRUCTION_RELEASE_NORMAL, &m_etd.release_tx);
    }
    catch(std::exception& e)
    {
      LOG_ERROR("A -> finish_contract failed with exception: " << e.what());
      return false;
    }
  }
      
  if (se.tx_behavior == eam_tx_confirm || se.tx_behavior == eam_tx_make_and_confirm)
  {
    CHECK_AND_NO_ASSERT_MES(mine_next_block_with_tx(c, m_etd.release_tx), false, "");
  }
  return true;
}

bool escrow_altchain_meta_impl::handle_event_cancellation_proposal(currency::core& c, std::shared_ptr<tools::wallet2> alice_wlt, const eam_event_cancellation_proposal& se)
{
  if (se.tx_behavior == eam_tx_make || se.tx_behavior == eam_tx_make_and_confirm)
  {
    try
    {
      m_etd.cancellation_proposal_tx = AUTO_VAL_INIT(m_etd.cancellation_proposal_tx);
      alice_wlt->request_cancel_contract(m_etd.ms_id, se.a_cancellation_fee, se.expiration_period, &m_etd.cancellation_proposal_tx);
    }
    catch(std::exception& e)
    {
      LOG_ERROR("A -> request_cancel_contract failed with exception: " << e.what());
      return false;
    }
  }
      
  if (se.tx_behavior == eam_tx_confirm || se.tx_behavior == eam_tx_make_and_confirm)
  {
    CHECK_AND_NO_ASSERT_MES(mine_next_block_with_tx(c, m_etd.cancellation_proposal_tx), false, "");
  }
  return true;
}

bool escrow_altchain_meta_impl::handle_event_release_cancel(currency::core& c, std::shared_ptr<tools::wallet2> bob_wlt, const eam_event_release_cancel& se)
{
  if (se.tx_behavior == eam_tx_make || se.tx_behavior == eam_tx_make_and_confirm)
  {
    try
    {
      m_etd.release_cancel_tx = AUTO_VAL_INIT(m_etd.release_cancel_tx);
      bob_wlt->accept_cancel_contract(m_etd.ms_id, &m_etd.release_cancel_tx);
    }
    catch(std::exception& e)
    {
      LOG_ERROR("B -> accept_proposal failed with exception: " << e.what());
      return false;
    }
  }
      
  if (se.tx_behavior == eam_tx_confirm || se.tx_behavior == eam_tx_make_and_confirm)
  {
    CHECK_AND_NO_ASSERT_MES(mine_next_block_with_tx(c, m_etd.release_cancel_tx), false, "");
  }
  return true;
}

bool escrow_altchain_meta_impl::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  blockchain_storage& bcs = c.get_blockchain_storage();

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();
  uint64_t alice_start_balance = alice_wlt->balance();
  CHECK_AND_ASSERT_MES(alice_start_balance == m_etd.alice_bob_start_amount, false, "Alice has invalid start balance: " << print_money_brief(alice_start_balance) << ", expected is " << print_money_brief(m_etd.alice_bob_start_amount));

  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  bob_wlt->refresh();
  uint64_t bob_start_balance = bob_wlt->balance();
  CHECK_AND_ASSERT_MES(bob_start_balance == m_etd.alice_bob_start_amount, false, "Bob has invalid start balance: " << print_money_brief(bob_start_balance) << ", expected is " << print_money_brief(m_etd.alice_bob_start_amount));

  std::vector<transaction> empty_txs_vector;
  currency::block b = AUTO_VAL_INIT(b);

  std::unordered_map<std::string, std::pair<crypto::hash, uint64_t>> block_labels;
  
  m_last_block_hash = bcs.get_top_block_id();
  m_last_block_height = bcs.get_top_block_height();
  
  for (size_t eam_event_index = 0; eam_event_index < m_etd.events.size(); ++eam_event_index)
  {
    eam_event_t& e = m_etd.events[eam_event_index];

    CHECK_AND_ASSERT_MES(e.height >= m_last_block_height + 1, false, "sub event #" << eam_event_index << " height " << e.height << " is behind the last block height, which is " << m_last_block_height);
    
    // until next event e doesn't match current height, mine a block with no txs
    while (e.height > m_last_block_height + 1)
    {
      CHECK_AND_NO_ASSERT_MES(mine_next_block_with_no_tx(c), false, "");
    }

    // okay, we have (e.height == last_block_height + 1), handle it
    bool mine_empty_block = false;
    bool expected_result = (e.flag_mask & eam_f_error) == 0;

    if (e.v.type() == typeid(eam_event_proposal))
    {
      eam_event_proposal& se = boost::get<eam_event_proposal>(e.v);
      LOG_PRINT_YELLOW("sub event #" << eam_event_index << " (height " << e.height << "): eam_event_proposal(" << tx_behaviour_to_cstr(se.tx_behavior) << ")" << (expected_result ? "" : " ERROR expected"), LOG_LEVEL_0);
      CHECK_AND_ASSERT_MES((se.tx_behavior == eam_tx_make) || (e.flag_mask & eam_f_no_block) == 0, false, "eam_f_no_block is incompalible with tx_confirm behavior");

      CHECK_AND_ASSERT_MES(handle_event_proposal(c, alice_wlt, se) == expected_result, false, "sub event handling failed, expected result: " << (expected_result ? "true" : "false"));

      mine_empty_block = (se.tx_behavior == eam_tx_make); // mine a block with no txs in that case
    }
    else if (e.v.type() == typeid(eam_event_acceptance))
    {
      eam_event_acceptance& se = boost::get<eam_event_acceptance>(e.v);
      LOG_PRINT_YELLOW("sub event #" << eam_event_index << " (height " << e.height << "): eam_event_acceptance(" << tx_behaviour_to_cstr(se.tx_behavior) << ")" << (expected_result ? "" : " ERROR expected"), LOG_LEVEL_0);

      CHECK_AND_ASSERT_MES(handle_event_acceptance(c, bob_wlt, se) == expected_result, false, "sub event handling failed, expected result: " << (expected_result ? "true" : "false"));

      mine_empty_block = (se.tx_behavior == eam_tx_make); // mine a block with no txs in that case
    }
    else if (e.v.type() == typeid(eam_event_release))
    {
      eam_event_release& se = boost::get<eam_event_release>(e.v);
      LOG_PRINT_YELLOW("sub event #" << eam_event_index << " (height " << e.height << "): eam_event_release(" << tx_behaviour_to_cstr(se.tx_behavior) << ")" << (expected_result ? "" : " ERROR expected"), LOG_LEVEL_0);

      CHECK_AND_ASSERT_MES(handle_event_release(c, alice_wlt, se) == expected_result, false, "sub event handling failed, expected result: " << (expected_result ? "true" : "false"));
      
      mine_empty_block = (se.tx_behavior == eam_tx_make); // mine a block with no txs in that case
    }
    else if (e.v.type() == typeid(eam_event_cancellation_proposal))
    {
      eam_event_cancellation_proposal& se = boost::get<eam_event_cancellation_proposal>(e.v);
      LOG_PRINT_YELLOW("sub event #" << eam_event_index << " (height " << e.height << "): eam_event_cancellation_proposal(" << tx_behaviour_to_cstr(se.tx_behavior) << ")" << (expected_result ? "" : " ERROR expected"), LOG_LEVEL_0);

      CHECK_AND_ASSERT_MES(handle_event_cancellation_proposal(c, alice_wlt, se) == expected_result, false, "sub event handling failed, expected result: " << (expected_result ? "true" : "false"));

      mine_empty_block = (se.tx_behavior == eam_tx_make); // mine a block with no txs in that case
    }
    else if (e.v.type() == typeid(eam_event_release_cancel))
    {
      eam_event_release_cancel& se = boost::get<eam_event_release_cancel>(e.v);
      LOG_PRINT_YELLOW("sub event #" << eam_event_index << " (height " << e.height << "): eam_event_release_cancel(" << tx_behaviour_to_cstr(se.tx_behavior) << ")" << (expected_result ? "" : " ERROR expected"), LOG_LEVEL_0);

      CHECK_AND_ASSERT_MES(handle_event_release_cancel(c, bob_wlt, se) == expected_result, false, "sub event handling failed, expected result: " << (expected_result ? "true" : "false"));

      mine_empty_block = (se.tx_behavior == eam_tx_make); // mine a block with no txs in that case
    }
    else if (e.v.type() == typeid(eam_event_refresh_and_check))
    {
      eam_event_refresh_and_check& se = boost::get<eam_event_refresh_and_check>(e.v);
      LOG_PRINT_YELLOW("sub event #" << eam_event_index << " (height " << e.height << "): eam_event_refresh_and_check", LOG_LEVEL_0);

      LOG_PRINT_GREEN("Alice's wallet is refreshing...", LOG_LEVEL_1);
      tools::escrow_contracts_container contracts;
      bool stub;
      alice_wlt->scan_tx_pool(stub);
      size_t blocks_fetched = 0;
      alice_wlt->refresh(blocks_fetched);
      CHECK_AND_ASSERT_MES(blocks_fetched == se.expected_blocks, false, "Alice got " << blocks_fetched << " after refresh, but " << se.expected_blocks << " is expected");
      LOG_PRINT_GREEN("Alice's transfers:" << ENDL << alice_wlt->dump_trunsfers(), LOG_LEVEL_1);
      if (se.a_balance != UINT64_MAX)
      {
        uint64_t alice_balance = alice_wlt->balance();
        CHECK_AND_ASSERT_MES(alice_balance == se.a_balance, false, "Alice has incorrect balance: " << print_money_brief(alice_balance) << ", expected: " << print_money_brief(se.a_balance));
      }
      if (se.a_state != eam_contract_state_none)
      {
        contracts.clear();
        alice_wlt->get_contracts(contracts);
        CHECK_AND_ASSERT_MES(check_contract_state(contracts, m_etd.cpd, static_cast<tools::wallet_public::escrow_contract_details_basic::contract_state>(se.a_state), "Alice"), false, "");
      }

      LOG_PRINT_GREEN("Bob's wallet is refreshing...", LOG_LEVEL_1);
      bob_wlt->scan_tx_pool(stub);
      blocks_fetched = 0;
      bob_wlt->refresh(blocks_fetched);
      CHECK_AND_ASSERT_MES(blocks_fetched == se.expected_blocks, false, "Bob got " << blocks_fetched << " after refresh, but " << se.expected_blocks << " is expected");
      LOG_PRINT_GREEN("Bob's transfers:" << ENDL << bob_wlt->dump_trunsfers(), LOG_LEVEL_1);
      if (se.b_balance != UINT64_MAX)
      {
        uint64_t bob_balance = bob_wlt->balance();
        CHECK_AND_ASSERT_MES(bob_balance == se.b_balance, false, "Bob has incorrect balance: " << print_money_brief(bob_balance) << ", expected: " << print_money_brief(se.b_balance));
      }
      if (se.b_state != eam_contract_state_none)
      {
        contracts.clear();
        bob_wlt->get_contracts(contracts);
        CHECK_AND_ASSERT_MES(check_contract_state(contracts, m_etd.cpd, static_cast<tools::wallet_public::escrow_contract_details_basic::contract_state>(se.b_state), "Bob"), false, "");
      }

      mine_empty_block = true;
    }
    else if (e.v.type() == typeid(eam_event_go_to))
    {
      eam_event_go_to& se = boost::get<eam_event_go_to>(e.v);
      LOG_PRINT_YELLOW("sub event #" << eam_event_index << " (height " << e.height << "): eam_event_go_to(" << se.label << ")", LOG_LEVEL_0);

      auto it = block_labels.find(se.label);
      CHECK_AND_ASSERT_MES(it != block_labels.end(), false, "can't find block by label " << se.label);
      m_last_block_hash = it->second.first;
      m_last_block_height = it->second.second;
      LOG_PRINT_YELLOW("      eam_event_go_to: set prev block to " << m_last_block_hash << " @ height " << m_last_block_height, LOG_LEVEL_0);
      mine_empty_block = false;
    }
    else if (e.v.type() == typeid(eam_event_noop))
    {
      LOG_PRINT_YELLOW("sub event #" << eam_event_index << " (height " << e.height << "): eam_event_noop", LOG_LEVEL_0);
      // do nothing
      mine_empty_block = true;
    }
    else if (e.v.type() == typeid(eam_event_check_top_block))
    {
      eam_event_check_top_block& se = boost::get<eam_event_check_top_block>(e.v);
      LOG_PRINT_YELLOW("sub event #" << eam_event_index << " (height " << e.height << "): eam_event_check_top_block", LOG_LEVEL_0);
      CHECK_AND_ASSERT_MES(bcs.get_top_block_height() == se.height, false, "incorrect top block height: " << bcs.get_top_block_height() << ", expected: " << se.height);
      if (!se.label.empty())
      {
        auto it = block_labels.find(se.label);
        CHECK_AND_ASSERT_MES(it != block_labels.end(), false, "can't find block by label " << se.label);
        CHECK_AND_ASSERT_MES(it->second.second == se.height, false, "block labeled with " << se.label << " has height " << it->second.second << ", expected height is " << se.height);
        CHECK_AND_ASSERT_MES(it->second.first == bcs.get_top_block_id(), false, "incorrect top block id: " << bcs.get_top_block_id() << ", expected: " << it->second.first);
      }
      mine_empty_block = true;
    }
    else if (e.v.type() == typeid(eam_event_transfer))
    {
      eam_event_transfer& se = boost::get<eam_event_transfer>(e.v);
      LOG_PRINT_YELLOW("sub event #" << eam_event_index << " (height " << e.height << "): eam_event_transfer(" << party_to_cstr(se.party_from) << " -> " << party_to_cstr(se.party_to) << ", " <<
        print_money_brief(se.amount) << ")", LOG_LEVEL_0);

      CHECK_AND_ASSERT_MES(se.party_from == ALICE_ACC_IDX || se.party_from == BOB_ACC_IDX, false, "invalid party_from: " << se.party_from);
      CHECK_AND_ASSERT_MES(se.party_to == ALICE_ACC_IDX || se.party_to == BOB_ACC_IDX, false, "invalid party_to: " << se.party_to);

      std::shared_ptr<tools::wallet2> wlt_from = se.party_from == ALICE_ACC_IDX ? alice_wlt : bob_wlt;
      std::shared_ptr<tools::wallet2> wlt_to = se.party_to == ALICE_ACC_IDX ? alice_wlt : bob_wlt;

      currency::transaction tx = AUTO_VAL_INIT(tx);
      std::vector<tx_destination_entry> destinations{ tx_destination_entry(se.amount, wlt_to->get_account().get_public_address()) };
      wlt_from->transfer(destinations, 0, 0, se.fee, empty_extra, empty_attachment, tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), tx);

      CHECK_AND_NO_ASSERT_MES(mine_next_block_with_tx(c, tx), false, "mine_next_block_with_tx failed");
      mine_empty_block = false;
    }

    if (mine_empty_block && (e.flag_mask & eam_f_no_block) == 0)
    {
      CHECK_AND_NO_ASSERT_MES(mine_next_block_with_no_tx(c), false, "");
      if (!e.label.empty())
      {
        r = block_labels.insert(std::make_pair(e.label, std::make_pair(m_last_block_hash, m_last_block_height))).second;
        CHECK_AND_ASSERT_MES(r, false, "can't insert block label " << e.label << ", duplicate entry?");
      }
    }

    c.get_tx_pool().remove_stuck_transactions();


  }

  return true;
}
