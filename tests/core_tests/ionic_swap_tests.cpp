// Copyright (c) 2014-2023 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "ionic_swap_tests.h"
#include "wallet_test_core_proxy.h"

#include "random_helper.h"
#include "tx_builder.h"

using namespace currency;

// Helpers

bool check_ionic_swap_tx_outs(const std::vector<currency::account_base>& accounts, const currency::transaction& tx, std::shared_ptr<const tools::wallet2> w, const tools::wallet_public::ionic_swap_proposal& proposal)
{
  tools::mode_separate_context msc = AUTO_VAL_INIT(msc);
  msc.tx_for_mode_separate = proposal.tx_template;

  tools::wallet_public::ionic_swap_proposal_context ionic_context = AUTO_VAL_INIT(ionic_context);
  bool r = w->get_ionic_swap_proposal_info(proposal, msc.proposal_info, ionic_context);
  CHECK_AND_ASSERT_MES(r, false, "get_ionic_swap_proposal_info failed");

  // decode all outputs amounts and asset_ids
  std::vector<std::pair<uint64_t, crypto::public_key>> outs_amounts_and_asset_ids;
  outs_amounts_and_asset_ids.resize(tx.vout.size());

  //ionic_context.one_time_skey
  const crypto::public_key& tx_pub_key = get_tx_pub_key_from_extra(tx);

  for(size_t i = 0; i < tx.vout.size(); ++i)
  {
    VARIANT_SWITCH_BEGIN(tx.vout[i])
      VARIANT_CASE_CONST(currency::tx_out_zarcanum, oz)
      for(size_t k = 0; k < accounts.size(); ++k)
      {
        crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
        bool r = crypto::generate_key_derivation(tx_pub_key, accounts[k].get_keys().view_secret_key, derivation);
        CHECK_AND_ASSERT_MES(r, false, "generate_key_derivation failed");
        uint64_t decoded_amount = 0;
        crypto::public_key decoded_asset_id{};
        crypto::scalar_t amount_blinding_mask{};
        crypto::scalar_t asset_id_blinding_mask{};
        if (decode_output_amount_and_asset_id(oz, derivation, i, decoded_amount, decoded_asset_id, amount_blinding_mask, asset_id_blinding_mask))
        {
          outs_amounts_and_asset_ids[i] = std::make_pair(decoded_amount, decoded_asset_id);
          break;
        }
      }
    VARIANT_SWITCH_END()
  }

  static_assert(CURRENCY_TX_MIN_ALLOWED_OUTS >= 2);
  const size_t expected_number_of_zero_amount_outputs = CURRENCY_TX_MIN_ALLOWED_OUTS - 2;
  size_t zero_amount_outputs = 0;
  for(auto& oaai : outs_amounts_and_asset_ids)
  {
    CHECK_AND_ASSERT_MES(oaai.second != null_pkey, false, "Not all outputs were decoded");
    if (oaai.first == 0)
      ++zero_amount_outputs;
  }

  CHECK_AND_ASSERT_MES(zero_amount_outputs <= expected_number_of_zero_amount_outputs, false, "zero_amount_outputs: " << zero_amount_outputs << ", expected_number_of_zero_amount_outputs: " << expected_number_of_zero_amount_outputs);

  return true;
}

//----------------------------------------------------------------------------------------------------

ionic_swap_basic_test::ionic_swap_basic_test()
{
  REGISTER_CALLBACK_METHOD(ionic_swap_basic_test, configure_core);
  REGISTER_CALLBACK_METHOD(ionic_swap_basic_test, c1);

  m_hardforks.set_hardfork_height(1, 1);
  m_hardforks.set_hardfork_height(2, 1);
  m_hardforks.set_hardfork_height(3, 1);
  m_hardforks.set_hardfork_height(4, 2);
}

bool ionic_swap_basic_test::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  currency::account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  currency::account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  currency::account_base& bob_acc =   m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);
  //account_base& carol_acc = m_accounts[CAROL_ACC_IDX]; carol_acc.generate(); carol_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  DO_CALLBACK(events, "c1");

  return true;
}

bool ionic_swap_basic_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt   = init_playtime_test_wallet(events, c, BOB_ACC_IDX);

  // check passing over the hardfork
  CHECK_AND_ASSERT_MES(c.get_blockchain_storage().is_hardfork_active(ZANO_HARDFORK_04_ZARCANUM), false, "ZANO_HARDFORK_04_ZARCANUM is active");

  miner_wlt->refresh();

  currency::asset_descriptor_base adb = AUTO_VAL_INIT(adb);
  adb.total_max_supply = 1000000 * COIN; //1M coins
  adb.full_name = "Test coins";
  adb.ticker = "TCT";
  adb.decimal_point = 12;

  uint64_t alice_amount = adb.total_max_supply / 2;
  uint64_t bob_amount = adb.total_max_supply / 2;

  std::vector<currency::tx_destination_entry> destinations(4);
  destinations[0].addr.push_back(m_accounts[ALICE_ACC_IDX].get_public_address());
  destinations[0].amount = alice_amount;
  destinations[0].asset_id = currency::null_pkey;
  destinations[1].addr.push_back(m_accounts[BOB_ACC_IDX].get_public_address());
  destinations[1].amount = bob_amount;
  destinations[1].asset_id = currency::null_pkey;

  destinations[2] = currency::tx_destination_entry(COIN, m_accounts[ALICE_ACC_IDX].get_public_address());
  destinations[3] = currency::tx_destination_entry(COIN, m_accounts[BOB_ACC_IDX].get_public_address());

  LOG_PRINT_MAGENTA("destinations[0].asset_id:" << destinations[0].asset_id, LOG_LEVEL_0);
  LOG_PRINT_MAGENTA("destinations[1].asset_id:" << destinations[1].asset_id, LOG_LEVEL_0);
  LOG_PRINT_MAGENTA("currency::null_pkey:" << currency::null_pkey, LOG_LEVEL_0);

  currency::transaction tx = AUTO_VAL_INIT(tx);
  crypto::public_key asset_id = currency::null_pkey;
  miner_wlt->deploy_new_asset(adb, destinations, tx, asset_id);
  LOG_PRINT_L0("Deployed new asset: " << asset_id << ", tx_id: " << currency::get_transaction_hash(tx));

  //currency::transaction res_tx = AUTO_VAL_INIT(res_tx);
  //miner_wlt->transfer(COIN, alice_wlt->get_account().get_public_address(), res_tx);
  //miner_wlt->transfer(COIN, bob_wlt->get_account().get_public_address(), res_tx);

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");


  bob_wlt->refresh();
  alice_wlt->refresh();
  uint64_t mined = 0;
  std::unordered_map<crypto::public_key, tools::wallet_public::asset_balance_entry_base> balances;
  bob_wlt->balance(balances, mined);

  auto it_asset = balances.find(asset_id);
  auto it_native = balances.find(currency::native_coin_asset_id);


  CHECK_AND_ASSERT_MES(it_asset != balances.end() && it_native != balances.end(), false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_asset->second.total == bob_amount, false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_native->second.total == COIN, false, "Failed to find needed asset in result balances");

  balances.clear();
  alice_wlt->balance(balances, mined);

  it_asset = balances.find(asset_id);
  it_native = balances.find(currency::native_coin_asset_id);

  CHECK_AND_ASSERT_MES(it_asset != balances.end() && it_native != balances.end(), false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_native->second.total == COIN, false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_asset->second.total == alice_amount, false, "Failed to find needed asset in result balances");

  const uint64_t assets_to_exchange = 10 * COIN;
  const uint64_t native_coins_to_exchange = COIN/2;

  {
    // Alice wants to trade with Bob, to exchange 10.0 TCT to 0.5 ZANO 
    view::ionic_swap_proposal_info proposal_details = AUTO_VAL_INIT(proposal_details);
    proposal_details.fee_paid_by_a = TESTS_DEFAULT_FEE;
    proposal_details.to_finalizer.push_back(view::asset_funds{ asset_id , assets_to_exchange });
    proposal_details.to_initiator.push_back(view::asset_funds{ currency::native_coin_asset_id , native_coins_to_exchange });

    tools::wallet_public::ionic_swap_proposal proposal = AUTO_VAL_INIT(proposal);
    alice_wlt->create_ionic_swap_proposal(proposal_details, m_accounts[BOB_ACC_IDX].get_public_address(), proposal);

    view::ionic_swap_proposal_info proposal_decoded_info = AUTO_VAL_INIT(proposal_decoded_info);
    bob_wlt->get_ionic_swap_proposal_info(proposal, proposal_decoded_info);

    //Validate proposal
    if (proposal_decoded_info.to_finalizer != proposal_details.to_finalizer
      || proposal_decoded_info.to_initiator != proposal_details.to_initiator
      || proposal_decoded_info.fee_paid_by_a != proposal_details.fee_paid_by_a
      )
    {
      CHECK_AND_ASSERT_MES(false, false, "proposal actual and proposals decoded mismatch");
    }

    currency::transaction res_tx2 = AUTO_VAL_INIT(res_tx2);
    r = bob_wlt->accept_ionic_swap_proposal(proposal, res_tx2);
    CHECK_AND_ASSERT_MES(r, false, "Failed to accept ionic proposal");
    CHECK_AND_ASSERT_MES(check_ionic_swap_tx_outs(m_accounts, res_tx2, bob_wlt, proposal), false, "");
  }

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");


  alice_wlt->refresh();
  balances.clear();
  alice_wlt->balance(balances, mined);
  it_asset = balances.find(asset_id);
  it_native = balances.find(currency::native_coin_asset_id);

  CHECK_AND_ASSERT_MES(it_asset != balances.end(), false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_native->second.total == native_coins_to_exchange + COIN - TESTS_DEFAULT_FEE, false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_asset->second.total == alice_amount - assets_to_exchange, false, "Failed to find needed asset in result balances");


  bob_wlt->refresh();
  balances.clear();
  bob_wlt->balance(balances, mined);
  it_asset = balances.find(asset_id);
  it_native = balances.find(currency::native_coin_asset_id);

  CHECK_AND_ASSERT_MES(it_asset != balances.end(), false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_native->second.total == COIN - native_coins_to_exchange, false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_asset->second.total == bob_amount + assets_to_exchange, false, "Failed to find needed asset in result balances");

  {
    //now Alice want to trade with Bob, to send 0.5 ZANO and get 10.0 TCT in exchange
    view::ionic_swap_proposal_info proposal_details = AUTO_VAL_INIT(proposal_details);
    proposal_details.fee_paid_by_a = TESTS_DEFAULT_FEE;
    proposal_details.to_finalizer.push_back(view::asset_funds{ currency::native_coin_asset_id , native_coins_to_exchange });
    proposal_details.to_initiator.push_back(view::asset_funds{ asset_id , assets_to_exchange });

    tools::wallet_public::ionic_swap_proposal proposal = AUTO_VAL_INIT(proposal);
    alice_wlt->create_ionic_swap_proposal(proposal_details, m_accounts[BOB_ACC_IDX].get_public_address(), proposal);

    view::ionic_swap_proposal_info proposal_decoded_info = AUTO_VAL_INIT(proposal_decoded_info);
    bob_wlt->get_ionic_swap_proposal_info(proposal, proposal_decoded_info);

    //Validate proposal
    if (proposal_decoded_info.to_finalizer != proposal_details.to_finalizer
      || proposal_decoded_info.to_initiator != proposal_details.to_initiator
      || proposal_decoded_info.fee_paid_by_a != proposal_details.fee_paid_by_a
      )
    {
      CHECK_AND_ASSERT_MES(false, false, "proposal actual and proposals decoded mismatch");
    }

    currency::transaction res_tx2 = AUTO_VAL_INIT(res_tx2);
    r = bob_wlt->accept_ionic_swap_proposal(proposal, res_tx2);
    CHECK_AND_ASSERT_MES(r, false, "Failed to accept ionic proposal");
  }

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");


  alice_wlt->refresh();
  balances.clear();
  alice_wlt->balance(balances, mined);
  it_asset = balances.find(asset_id);
  it_native = balances.find(currency::native_coin_asset_id);

  CHECK_AND_ASSERT_MES(it_asset != balances.end(), false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_native->second.total == native_coins_to_exchange + COIN - TESTS_DEFAULT_FEE * 2 - native_coins_to_exchange, false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_asset->second.total == alice_amount, false, "Failed to find needed asset in result balances");


  bob_wlt->refresh();
  balances.clear();
  bob_wlt->balance(balances, mined);
  it_asset = balances.find(asset_id);
  it_native = balances.find(currency::native_coin_asset_id);

  CHECK_AND_ASSERT_MES(it_asset != balances.end(), false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_native->second.total == COIN - native_coins_to_exchange + native_coins_to_exchange, false, "Failed to find needed asset in result balances");
  CHECK_AND_ASSERT_MES(it_asset->second.total == bob_amount, false, "Failed to find needed asset in result balances");





  //TODO: 
  // add fee paid by bob scenario
  // add transfer of tokens without native coins
  // different fail combination
  return true;
}

//----------------------------------------------------------------------------------------------------

ionic_swap_exact_amounts_test::ionic_swap_exact_amounts_test()
{
  REGISTER_CALLBACK_METHOD(ionic_swap_exact_amounts_test, c1);
}

bool ionic_swap_exact_amounts_test::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  currency::account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  currency::account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  currency::account_base& bob_acc =   m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);
  currency::account_base& carol_acc = m_accounts[CAROL_ACC_IDX]; carol_acc.generate(); carol_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);

  // rebuild genesis miner tx
  std::vector<tx_destination_entry> destinations;
  destinations.emplace_back(MK_TEST_COINS(21), alice_acc.get_public_address());
  destinations.emplace_back(MK_TEST_COINS(21), miner_acc.get_public_address()); // decoy (later Alice will spend her output using mixins)
  destinations.emplace_back(MK_TEST_COINS(21), miner_acc.get_public_address()); // decoy
  destinations.emplace_back(COIN, miner_acc.get_public_address());
  // leftover amount will be also send to miner
  CHECK_AND_ASSERT_MES(replace_coinbase_in_genesis_block(destinations, generator, events, blk_0), false, "");

  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_04_ZARCANUM));

  DO_CALLBACK(events, "c1");

  return true;
}

bool ionic_swap_exact_amounts_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  miner_wlt->refresh();

  asset_descriptor_base adb{};
  adb.total_max_supply = 10000000000;
  adb.full_name = "test";
  adb.ticker = "TEST";

  std::vector<currency::tx_destination_entry> destinations;
  destinations.emplace_back(7070000000, m_accounts[BOB_ACC_IDX].get_public_address(), null_pkey);
  destinations.emplace_back(2930000000, m_accounts[BOB_ACC_IDX].get_public_address(), null_pkey);
  destinations.emplace_back(MK_TEST_COINS(21), m_accounts[CAROL_ACC_IDX].get_public_address()); // Carol will get coins with non-explicit asset id

  currency::transaction tx{};
  crypto::public_key asset_id = currency::null_pkey;
  miner_wlt->deploy_new_asset(adb, destinations, tx, asset_id);
  LOG_PRINT_L0("Deployed new asset: " << asset_id << ", tx_id: " << currency::get_transaction_hash(tx));

  CHECK_AND_ASSERT_MES(tx.vout.size() >= 2, false, "Unexpected vout size: " << tx.vout.size());
  for(auto& out : tx.vout)
  {
    CHECK_AND_ASSERT_MES(out.type() == typeid(tx_out_zarcanum), false, "invalid out type");
    CHECK_AND_ASSERT_MES(boost::get<tx_out_zarcanum>(out).blinded_asset_id != native_coin_asset_id_1div8, false, "One of outputs has explicit native asset id, which is unexpected");
  }

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  bob_wlt->refresh();
  std::shared_ptr<tools::wallet2> carol_wlt = init_playtime_test_wallet(events, c, CAROL_ACC_IDX);
  carol_wlt->refresh();

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", MK_TEST_COINS(21),     MK_TEST_COINS(21), MK_TEST_COINS(21),    0, 0), false, "");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt,   "Bob",   adb.total_max_supply,  0,                 adb.total_max_supply, 0, 0, asset_id), false, "");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*carol_wlt, "Carol", MK_TEST_COINS(21),     0,                 MK_TEST_COINS(21),    0, 0), false, "");

  //size_t current_blockchain_size = c.get_current_blockchain_size();

  // Normal ionic swap between Alice and Bob:  (Alice has only coins with explicit asset id)
  //   before:
  // Alice (initiator): 0.21 ZANO    < - >     Bob (finalizer): 0.01 TEST
  //   after:
  // Alice (initiator): 0.01 TEST              Bob (finalizer): 0.20 ZANO

  view::ionic_swap_proposal_info proposal_details{};
  proposal_details.to_initiator.push_back(view::asset_funds{ asset_id, adb.total_max_supply });
  proposal_details.to_finalizer.push_back(view::asset_funds{ native_coin_asset_id, MK_TEST_COINS(20) });
  proposal_details.fee_paid_by_a = MK_TEST_COINS(1);

  tools::wallet_public::ionic_swap_proposal proposal{};
  alice_wlt->create_ionic_swap_proposal(proposal_details, m_accounts[BOB_ACC_IDX].get_public_address(), proposal);

  view::ionic_swap_proposal_info proposal_decoded_info{};
  bob_wlt->get_ionic_swap_proposal_info(proposal, proposal_decoded_info);
  CHECK_AND_ASSERT_MES(
    proposal_decoded_info.to_finalizer == proposal_details.to_finalizer &&
    proposal_decoded_info.to_initiator == proposal_details.to_initiator &&
    proposal_decoded_info.fee_paid_by_a == proposal_details.fee_paid_by_a,
    false, "actual and decoded proposal mismatch \nproposal_decoded_info: " 
    << epee::serialization::store_t_to_json(proposal_decoded_info) << 
    "\nproposal_details" << epee::serialization::store_t_to_json(proposal_details));

  currency::transaction tx_is{};
  r = bob_wlt->accept_ionic_swap_proposal(proposal, tx_is);
  CHECK_AND_ASSERT_MES(r, false, "Failed to accept ionic proposal");
  CHECK_AND_ASSERT_MES(check_ionic_swap_tx_outs(m_accounts, tx_is, bob_wlt, proposal), false, "");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  alice_wlt->refresh();
  bob_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", adb.total_max_supply,  0, adb.total_max_supply,  0, 0, asset_id), false, "");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt,   "Bob",   MK_TEST_COINS(20),     0, MK_TEST_COINS(20),     0, 0), false, "");


  // Normal ionic swap between Carol and Alice:  (Carol has only coins with non-explicit asset id)
  //   before:
  // Carol (initiator): 0.21 Zano              Alice (finalizer): 0.01 TEST
  //   after:
  // Carol (initiator): 0.01 TEST              Alice (finalizer): 0.2  ZANO

  proposal_details = view::ionic_swap_proposal_info{};
  proposal_details.to_initiator.push_back(view::asset_funds{ asset_id, adb.total_max_supply });
  proposal_details.to_finalizer.push_back(view::asset_funds{ native_coin_asset_id, MK_TEST_COINS(20) });
  proposal_details.fee_paid_by_a = MK_TEST_COINS(1);

  proposal = tools::wallet_public::ionic_swap_proposal{};
  carol_wlt->create_ionic_swap_proposal(proposal_details, m_accounts[ALICE_ACC_IDX].get_public_address(), proposal);

  proposal_decoded_info = view::ionic_swap_proposal_info{};
  alice_wlt->get_ionic_swap_proposal_info(proposal, proposal_decoded_info);
  CHECK_AND_ASSERT_MES(
    proposal_decoded_info.to_finalizer == proposal_details.to_finalizer &&
    proposal_decoded_info.to_initiator == proposal_details.to_initiator &&
    proposal_decoded_info.fee_paid_by_a == proposal_details.fee_paid_by_a,
    false, "actual and decoded proposal mismatch");

  currency::transaction tx_is2{};
  r = alice_wlt->accept_ionic_swap_proposal(proposal, tx_is2);
  CHECK_AND_ASSERT_MES(r, false, "Failed to accept ionic proposal");
  CHECK_AND_ASSERT_MES(check_ionic_swap_tx_outs(m_accounts, tx_is2, alice_wlt, proposal), false, "");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count());

  carol_wlt->refresh();
  alice_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*carol_wlt, "Carol", adb.total_max_supply,  0, adb.total_max_supply,  0, 0, asset_id), false, "");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", MK_TEST_COINS(20),     0, MK_TEST_COINS(20),     0, 0), false, "");

  return true;
}
