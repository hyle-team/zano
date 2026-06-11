// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "get_random_outs.h"

using namespace currency;

get_random_outs_test::get_random_outs_test()
  : m_amount(0)
{
  REGISTER_CALLBACK_METHOD(get_random_outs_test, check_get_rand_outs);
}

bool get_random_outs_test::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;
  GENERATE_ACCOUNT(miner_account);

  //                                                                                                    event index
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);                                           //  0
  events.push_back(event_core_time(ts_start));                                                          //  1
  MAKE_ACCOUNT(events, bob_account);                                                                    //  2
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);                                                 //  3
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_account);                                                 //  4
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_account);                                                 //  5
  REWIND_BLOCKS_N_WITH_TIME(events, blk_3r, blk_3, miner_account, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);  //  2N+5  (N == CURRENCY_MINED_MONEY_UNLOCK_WINDOW)

  uint64_t first_blocks_reward = get_outs_money_amount(blk_1.miner_tx), stub;
  bool r = calculate_amounts_many_outs_have_and_no_outs_have(first_blocks_reward, stub, m_amount);
  CHECK_AND_ASSERT_MES(r, false, "calculate_amounts_many_outs_have_and_no_outs_have failed");

  MAKE_TX_LIST_START(events, txs_blk_4, miner_account, miner_account, m_amount, blk_3r);                //  2N+6

  MAKE_TX_LIST(events, txs_blk_4, miner_account, miner_account, m_amount, blk_3r);                      //  2N+7
  MAKE_TX_LIST(events, txs_blk_4, miner_account, miner_account, m_amount, blk_3r);                      //  2N+8
  MAKE_TX_LIST(events, txs_blk_4, miner_account, bob_account, m_amount, blk_3r);                        //  2N+9

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_4, blk_3r, miner_account, txs_blk_4);                             //  2N+10

  MAKE_TX_LIST_START(events, txs_blk_5, bob_account, miner_account, m_amount - TESTS_DEFAULT_FEE, blk_4); // 2N+11
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_5, blk_4, miner_account, txs_blk_5);                              //  2N+12
  REWIND_BLOCKS_N_WITH_TIME(events, blk_5r, blk_5, miner_account, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);  //  4N+12
  DO_CALLBACK(events, "check_get_rand_outs");                                                           //  4N+13     
  return true;
}

bool get_random_outs_test::check_get_rand_outs(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request req = AUTO_VAL_INIT(req);
  req.amounts.push_back(m_amount);
  req.decoys_count = 4;
  req.use_forced_mix_outs = false;
  currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response res = AUTO_VAL_INIT(res);
  c.get_blockchain_storage().get_random_outs_for_amounts(req, res);
  CHECK_AND_ASSERT_MES(res.outs[0].outs.size() == 3, false, "Incorrect number of random outs returned.");

  return true;
}

//------------------------------------------------------------------------------

random_outs_and_burnt_coins::random_outs_and_burnt_coins()
{
  REGISTER_CALLBACK_METHOD(random_outs_and_burnt_coins, c1);
}

bool random_outs_and_burnt_coins::generate(std::vector<test_event_entry>& events) const
{
  // Test idead: make sure burned coins (that are technically will NEVER EVER been spent)
  // cannot be used for mixing in, as it reduces anonimity.

  bool r = false;

  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 4);

  // find unique amount and store it to m_amount
  uint64_t stub;
  r = calculate_amounts_many_outs_have_and_no_outs_have(get_outs_money_amount(blk_0r.miner_tx), stub, m_amount);
  CHECK_AND_ASSERT_MES(r, false, "calculate_amounts_many_outs_have_and_no_outs_have failed");

  // prepare fake outputs and burn it
  // make m_fake_amounts_count outputs each of amount amount_no_outs_have
  std::vector<tx_destination_entry> destinations;
  for(size_t i = 0; i < m_fake_amounts_count; ++i)
    destinations.push_back(tx_destination_entry(m_amount, null_pub_addr));
  std::vector<tx_source_entry> sources;

  r = fill_tx_sources(sources, events, blk_0r, miner_acc.get_keys(), m_amount * m_fake_amounts_count + TESTS_DEFAULT_FEE, 0);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");

  transaction tx_0{};
  r = construct_tx(miner_acc.get_keys(), sources, destinations, events, this, tx_0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

  uint64_t burned_tx_amount_total = get_burned_amount(tx_0);
  CHECK_AND_ASSERT_MES(burned_tx_amount_total == m_fake_amounts_count * m_amount, false, "incorrect value of burned amount: " << burned_tx_amount_total << ", expected: " << m_fake_amounts_count * m_amount);

  events.push_back(tx_0);

  // send to Alice amount_no_outs_have coins
  MAKE_TX(events, tx_1, miner_acc, alice_acc, m_amount, blk_0r);
  
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({tx_0, tx_1}));

  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);

  DO_CALLBACK(events, "c1");

  return true;
}

bool random_outs_and_burnt_coins::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, m_accounts[MINER_ACC_IDX]);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  std::shared_ptr<tools::wallet2> bob_wlt   = init_playtime_test_wallet(events, c, m_accounts[BOB_ACC_IDX]);

  bool r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, WALLET_DEFAULT_TX_SPENDABLE_AGE);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt,
    m_amount, // expected total
    false,
    CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 6 + WALLET_DEFAULT_TX_SPENDABLE_AGE,
    m_amount // expected unlocked
    ), false, "");

  std::vector<tx_destination_entry> destinations({tx_destination_entry(m_amount - TESTS_DEFAULT_FEE, m_accounts[BOB_ACC_IDX].get_public_address())});
  
  // make sure it's impossible to mixin an output with m_amount amount (because each one is burned)
  for (uint64_t fake_outs = m_fake_amounts_count + 1; fake_outs > 0; --fake_outs)
  {
    LOG_PRINT_L0("trying transfer with fake_outs = " << fake_outs);
    r = false;
    try
    {
      alice_wlt->transfer(destinations, fake_outs, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment);
    }
    catch (tools::error::not_enough_outs_to_mix&)
    {
      r = true;
    }
    CHECK_AND_ASSERT_MES(r, false, "exception was not cought as expected for fake_outs = " << fake_outs);
  }


  // make normal output with m_amount amount and try to use it as mixin
  miner_wlt->refresh();
  miner_wlt->transfer(m_amount, m_accounts[BOB_ACC_IDX].get_public_address());

  // miner few blocks to make it mixable
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, WALLET_DEFAULT_TX_SPENDABLE_AGE);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  alice_wlt->refresh();
  
  // try with 2 fake outputs -- should not work, as we've just added to the blockchain only one
  r = false;
  try
  {
    alice_wlt->transfer(destinations, 2 /* fake outs count */, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment);
  }
  catch (tools::error::not_enough_outs_to_mix&)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "exception was not cought as expected");

  // one mixin should perfectly work
  alice_wlt->transfer(destinations, 1 /* fake outs count */, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment);

  // check Bob's balance
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, m_amount * 2 - TESTS_DEFAULT_FEE, false, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 6 + WALLET_DEFAULT_TX_SPENDABLE_AGE * 2), false, "");


  return true;
}

decoy_set_oob_on_multisig_out::decoy_set_oob_on_multisig_out()
{
  REGISTER_CALLBACK_METHOD(decoy_set_oob_on_multisig_out, c1);
}

bool decoy_set_oob_on_multisig_out::generate(std::vector<test_event_entry>& events) const
{
  account_base genesis_acc;
  genesis_acc.generate();
  m_mining_accunt.generate();
  m_accunt_a.generate();
  m_accunt_b.generate();

  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, genesis_acc, test_core_time::get_time());
  events.push_back(blk_0);

  REWIND_BLOCKS_N(events, blk_0r, blk_0, m_mining_accunt, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK(events, "c1");
  return true;
}

bool decoy_set_oob_on_multisig_out::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // Test case: a block contains a multisig output with a non-decomposable amount and thus a stub global index 0. 
  // when the decoy collector iterates every output of a sampled block without skipping them, it throws an out-of-range exception 
  // upon trying to access the empty m_db_outputs bucket
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, m_mining_accunt);
  size_t blocks_fetched = 0;
  bool received_money = false;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);

  std::vector<currency::extra_v> extra;
  std::vector<currency::attachment_v> attachments;
  std::vector<tx_destination_entry> dst(1);
  dst.back().addr.push_back(m_accunt_a.get_public_address());
  dst.back().addr.push_back(m_accunt_b.get_public_address());
  dst.back().amount = ms_amount;
  dst.back().minimum_sigs = dst.back().addr.size();

  transaction ms_tx = AUTO_VAL_INIT(ms_tx);
  miner_wlt->transfer(dst, 0, 0, TESTS_DEFAULT_FEE, extra, attachments, tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), ms_tx);

  const uint64_t ms_tx_height = c.get_current_blockchain_size();

  bool r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, 3 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  // check locate the multisig output and confirm the stub-gindex situation
  size_t ms_vout_idx = SIZE_MAX;
  uint64_t ms_vout_amount = 0;
  for (size_t i = 0; i < ms_tx.vout.size(); ++i)
  {
    if (ms_tx.vout[i].type() == typeid(tx_out_bare) && boost::get<tx_out_bare>(ms_tx.vout[i]).target.type() == typeid(txout_multisig))
    {
      ms_vout_idx = i;
      ms_vout_amount = boost::get<tx_out_bare>(ms_tx.vout[i]).amount;
      break;
    }
  }
  CHECK_AND_ASSERT_MES(ms_vout_idx != SIZE_MAX, false, "multisig output not found in ms_tx");
  CHECK_AND_ASSERT_MES(ms_vout_amount != 0, false, "multisig output has zero (hidden) amount, can't reproduce");

  const crypto::hash ms_txid = get_transaction_hash(ms_tx);
  std::vector<uint64_t> gindexes;
  r = c.get_blockchain_storage().get_tx_outputs_gindexs(ms_txid, gindexes);
  CHECK_AND_ASSERT_MES(r, false, "get_tx_outputs_gindexs failed for multisig tx (tx must be on the main chain)");
  CHECK_AND_ASSERT_MES(gindexes.size() == ms_tx.vout.size(), false, "gindexes size mismatch");
  // the multisig output carries a STUB global index 0 and is absent from m_db_outputs
  CHECK_AND_ASSERT_MES(gindexes[ms_vout_idx] == 0, false, "expected stub global index 0 for the multisig output");

  bool oob_direct = false;
  try
  {
    std::vector<COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry> outs;
    c.get_blockchain_storage().collect_all_outs_in_block(ms_vout_amount, ms_tx_height, outs);
  }
  catch (const std::out_of_range& e)
  {
    oob_direct = true;
    LOG_PRINT_RED("collect_all_outs_in_block: " << e.what(), LOG_LEVEL_0);
  }

  bool oob_rpc = false;
  try
  {
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request req = AUTO_VAL_INIT(req);
    req.look_up_strategy = LOOK_UP_STRATEGY_REGULAR_TX;
    req.height_upper_limit = 0;
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request_batch batch = AUTO_VAL_INIT(batch);
    batch.input_amount = ms_vout_amount;
    batch.heights.push_back(ms_tx_height);
    req.batches.push_back(batch);
    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response resp = AUTO_VAL_INIT(resp);
    c.get_blockchain_storage().get_random_outs_for_amounts4(req, resp);
  }
  catch (const std::out_of_range& e)
  {
    oob_rpc = true;
    LOG_PRINT_RED("get_random_outs_for_amounts4: " << e.what(), LOG_LEVEL_0);
  }

  CHECK_AND_ASSERT_MES(!oob_direct, false, "collect_all_outs_in_block() threw out_of_range on a block with a multisig output (amount=" 
    << ms_vout_amount << ", height=" << ms_tx_height << ")");
  CHECK_AND_ASSERT_MES(!oob_rpc, false, "get_random_outs_for_amounts4() threw out_of_range on a block with a multisig output (height=" 
    << ms_tx_height << ")");

  return true;
}
