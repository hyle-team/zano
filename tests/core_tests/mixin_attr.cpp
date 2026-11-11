// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"

#include "mixin_attr.h"

using namespace epee;
using namespace currency;

mix_attr_tests::mix_attr_tests()
{
  REGISTER_CALLBACK_METHOD(mix_attr_tests, check_balances_1);
  REGISTER_CALLBACK_METHOD(mix_attr_tests, remember_last_block);
  REGISTER_CALLBACK_METHOD(mix_attr_tests, check_last_not_changed);
  REGISTER_CALLBACK_METHOD(mix_attr_tests, check_last2_and_balance);
  REGISTER_CALLBACK_METHOD(mix_attr_tests, check_last_changed);  
}

bool mix_attr_tests::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;
  GENERATE_ACCOUNT(miner_account);

  //TODO                                                                              events
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);                                           //  0
  MAKE_ACCOUNT(events, bob_account);                                                                    //  1
  MAKE_ACCOUNT(events, alice_account);                                                                  //  2
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);                                                 //  3
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_account);                                                 //  4
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_account);                                                 //  5
  REWIND_BLOCKS(events, blk_3r, blk_3, miner_account);                                                  // <N blocks>
  MAKE_TX_LIST_START_MIX_ATTR(events, txs_blk_4, miner_account, 
                                                 miner_account, 
                                                 MK_TEST_COINS(5), 
                                                 blk_3r,
                                                 CURRENCY_TO_KEY_OUT_RELAXED, std::vector<currency::attachment_v>());                    //  6 + N

  MAKE_TX_LIST_MIX_ATTR(events, txs_blk_4, miner_account, miner_account, MK_TEST_COINS(5), blk_3r,
                                                 CURRENCY_TO_KEY_OUT_RELAXED);                          //  7 + N
  MAKE_TX_LIST_MIX_ATTR(events, txs_blk_4, miner_account, miner_account, MK_TEST_COINS(5), blk_3r,
                                                 CURRENCY_TO_KEY_OUT_RELAXED);                          //  8 + N
  MAKE_TX_LIST_MIX_ATTR(events, txs_blk_4, miner_account, bob_account, MK_TEST_COINS(5), blk_3r,
                                                 CURRENCY_TO_KEY_OUT_FORCED_NO_MIX);                    //  9 + N

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_4, blk_3r, miner_account, txs_blk_4);                             // 10 + N
  DO_CALLBACK(events, "check_balances_1");                                                              // 11 + N
  REWIND_BLOCKS(events, blk_4r, blk_4, miner_account);                                                  // 11 + 2N

  // let's try to spend Bob's no-mix output mixing in 1 fake outputs -- should fail
  DO_CALLBACK(events, "remember_last_block");                                                          // 12 + 2N 
  DO_CALLBACK(events, "mark_invalid_tx");                                                              // 13 + 2N
  MAKE_TX_MIX(events, tx_0, bob_account, alice_account, MK_TEST_COINS(5) - TESTS_DEFAULT_FEE, 1, blk_4); // 14 + 2N
  DO_CALLBACK(events, "mark_invalid_block");                                                           // 15 + 2N
  MAKE_NEXT_BLOCK_TX1(events, blk_5_f, blk_4r, miner_account, tx_0);                                   // 16 + 2N
  DO_CALLBACK(events, "check_last_not_changed");                                                       // 17 + 2N             

  // then try to spend Bob's no-mix output with direct spend -- should go well
  MAKE_TX_MIX_ATTR(events, tx_1, bob_account, alice_account, MK_TEST_COINS(5) - TESTS_DEFAULT_FEE, 0, blk_4, CURRENCY_TO_KEY_OUT_RELAXED, false);
  MAKE_NEXT_BLOCK_TX1(events, blk_5, blk_4r, miner_account, tx_1);                                      
  DO_CALLBACK(events, "check_last2_and_balance");                                                                


  //check mixin
  MAKE_ACCOUNT(events, bob_account2);
  MAKE_ACCOUNT(events, alice_account2);


  MAKE_TX_LIST_START_MIX_ATTR(events, txs_blk_6, miner_account, 
                                                 miner_account, 
                                                 MK_TEST_COINS(5),
                                                 blk_5, 
                                                 CURRENCY_TO_KEY_OUT_RELAXED,
                                                 std::vector<currency::attachment_v>());
  //MAKE_TX_LIST_MIX_ATTR(events, txs_blk_6, miner_account, miner_account, MK_COINS(5), blk_5, 4);
  MAKE_TX_LIST_MIX_ATTR(events, txs_blk_6, miner_account, bob_account2, MK_TEST_COINS(5), blk_5, 4); // mix_attr == 4
  MAKE_TX_LIST_MIX_ATTR(events, txs_blk_6, miner_account, bob_account2, MK_TEST_COINS(5), blk_5, 4); // mix_attr == 4
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_6, blk_5, miner_account, txs_blk_6);

  // 
  // 7 outputs with amount == MK_TEST_COINS(5) having these mix_attrs:  0, 0, 0, 1 (miner -> miner, txs_blk_4),  0 (miner -> miner, txs_blk_6), 4, 4 (miner -> bob2, txs_blk_6)
  //
  
  DO_CALLBACK(events, "remember_last_block");                                                           

  bool r = false;
  try
  {
    // should fail to construct as there are not enough outputs to mix (regardless of mix_attr)
    MAKE_TX_MIX_ATTR(events, tx_3, bob_account2, alice_account2, MK_TEST_COINS(5) - TESTS_DEFAULT_FEE, 7, blk_6, CURRENCY_TO_KEY_OUT_RELAXED, false);
  }
  catch (std::runtime_error&)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "tx_3 passed ok, but expected to fail");

  DO_CALLBACK(events, "mark_invalid_tx"); // should fail because Bob2 outputs may only be used in set of 4 and more outputs (there's only 2 + 1 = 3)
  MAKE_TX_MIX_ATTR(events, tx_4, bob_account2, alice_account2, MK_TEST_COINS(5) - TESTS_DEFAULT_FEE, 2, blk_6, CURRENCY_TO_KEY_OUT_RELAXED, false);

  DO_CALLBACK(events, "mark_invalid_tx"); // the same
  MAKE_TX_MIX_ATTR(events, tx_5, bob_account2, alice_account2, MK_TEST_COINS(5) - TESTS_DEFAULT_FEE, 1, blk_6, CURRENCY_TO_KEY_OUT_RELAXED, false);
                                                                                                               
  DO_CALLBACK(events, "mark_invalid_tx"); // the same
  MAKE_TX_MIX_ATTR(events, tx_6, bob_account2, alice_account2, MK_TEST_COINS(5) - TESTS_DEFAULT_FEE, 0, blk_6, CURRENCY_TO_KEY_OUT_RELAXED, false);

  MAKE_TX_MIX_ATTR(events, tx_7, bob_account2, alice_account2, MK_TEST_COINS(5) - TESTS_DEFAULT_FEE, 3, blk_6, CURRENCY_TO_KEY_OUT_RELAXED, false); // should pass
  MAKE_NEXT_BLOCK_TX1(events, blk_7, blk_6, miner_account, tx_7);
  DO_CALLBACK(events, "check_last_changed");

  return true;
}

bool mix_attr_tests::check_balances_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::account_base m_bob_account = boost::get<account_base>(events[1]);
  currency::account_base m_alice_account = boost::get<account_base>(events[2]);

  std::list<block> blocks;
  bool r = c.get_blocks(0, 100 + 2 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW, blocks);
  CHECK_TEST_CONDITION(r);

  std::vector<currency::block> chain;
  map_hash2tx_t mtx;
  r = find_block_chain(events, chain, mtx, get_block_hash(blocks.back()));
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(MK_TEST_COINS(5), get_balance(m_bob_account, chain, mtx));
  CHECK_EQ(0, get_balance(m_alice_account, chain, mtx));

  return true;
}

bool mix_attr_tests::remember_last_block(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{  
  top_id_befor_split = c.get_tail_id();
  return true;
}

bool mix_attr_tests::check_last_not_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{  
  CHECK_EQ(top_id_befor_split, c.get_tail_id());
  return true;
}
bool mix_attr_tests::check_last2_and_balance(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{  
  CHECK_NOT_EQ(top_id_befor_split, c.get_tail_id());

  currency::account_base m_bob_account = boost::get<account_base>(events[1]);
  currency::account_base m_alice_account = boost::get<account_base>(events[2]);

  std::list<block> blocks;
  bool r = c.get_blocks(0, 100 + 2 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW, blocks);
  CHECK_TEST_CONDITION(r);

  std::vector<currency::block> chain;
  map_hash2tx_t mtx;
  r = find_block_chain(events, chain, mtx, get_block_hash(blocks.back()));
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(0, get_balance(m_bob_account, chain, mtx));
  CHECK_EQ(MK_TEST_COINS(5)-TESTS_DEFAULT_FEE, get_balance(m_alice_account, chain, mtx));

  return true;
}
bool mix_attr_tests::check_last_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{  
  CHECK_NOT_EQ(top_id_befor_split, c.get_tail_id());
  return true;
}

//--------------------------------------------------------------------------

struct check_outs_count_params
{
  check_outs_count_params() : amount(0), count(0) {}
  check_outs_count_params(uint64_t amount, size_t count) : amount(amount), count(count) {}

  uint64_t amount;
  size_t count;
  
  BEGIN_KV_SERIALIZE_MAP()
    KV_SERIALIZE_POD_AS_HEX_STRING(amount)
    KV_SERIALIZE_POD_AS_HEX_STRING(count)
  END_KV_SERIALIZE_MAP()
};


mix_in_spent_outs::mix_in_spent_outs()
{
  REGISTER_CALLBACK_METHOD(mix_in_spent_outs, c1);
  REGISTER_CALLBACK_METHOD(mix_in_spent_outs, check_outs_count);
}

bool mix_in_spent_outs::generate(std::vector<test_event_entry>& events) const
{
  // test idea: use obviously spent outputs as fake ones for mixing in. Such outputs MUST NOT be used for mixing in.

  m_accounts.resize(TOTAL_ACCS_COUNT);
  auto& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  auto& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  auto& bob_acc =   m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  GENERATE_ACCOUNT(preminer_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 6);

  m_test_amount = MK_TEST_COINS(6);
  for(auto& o : blk_0.miner_tx.vout)
  {
    CHECK_AND_ASSERT_MES(boost::get<tx_out_bare>(o).amount != m_test_amount, false, "Premine surprisingly has test amount output, change m_test_amount");
  }
  
  MAKE_TX_LIST_START(events, txs, miner_acc, alice_acc, m_test_amount, blk_0r);
  MAKE_TX_LIST(events, txs, miner_acc, alice_acc, m_test_amount, blk_0r);
  MAKE_TX_LIST(events, txs, miner_acc, alice_acc, m_test_amount, blk_0r);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, txs);

  DO_CALLBACK_PARAMS_STR(events, "check_outs_count", epee::serialization::store_t_to_json(check_outs_count_params(m_test_amount, 3)));

  MAKE_TX_MIX(events, tx_0, miner_acc, bob_acc, m_test_amount, 0, blk_1); // mix_attr == 0
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, tx_0);

  DO_CALLBACK_PARAMS_STR(events, "check_outs_count", epee::serialization::store_t_to_json(check_outs_count_params(m_test_amount, 4)));

  // spend 2 of 3 Alice's outputs with direct transfer (not mixins)
  MAKE_TX(events, tx_1, alice_acc, miner_acc, m_test_amount * 2 - TESTS_DEFAULT_FEE, blk_2); // mix_attr == 0
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_1);

  // despite the fact all Alice's outputs are spent now, total number of outputs with such amount should not changed
  DO_CALLBACK_PARAMS_STR(events, "check_outs_count", epee::serialization::store_t_to_json(check_outs_count_params(m_test_amount, 4)));

  REWIND_BLOCKS_N(events, blk_3r, blk_3, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

bool mix_in_spent_outs::check_outs_count(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // makes sure there are params.count outputs with params.amount in the blockchain

  check_outs_count_params params;
  bool r = epee::serialization::load_t_from_json(params, boost::get<callback_entry>(events[ev_index]).callback_params);
  CHECK_AND_ASSERT_MES(r, false, "Can't load params struct");

  auto& bcs = c.get_blockchain_storage();
  uint64_t such_amount_outs_count = bcs.get_outputs_container().get_item_size(params.amount);
  CHECK_AND_ASSERT_MES(such_amount_outs_count == params.count, false, "invalid global outputs with amount == " << print_money(params.amount) << " : " << such_amount_outs_count << ", expected: " << params.count);

  return true;
}

bool mix_in_spent_outs::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, m_accounts[BOB_ACC_IDX]);
  bob_wlt->refresh();

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "There are txs in the pool");

  std::vector<tx_destination_entry> destinations({ tx_destination_entry(m_test_amount - TESTS_DEFAULT_FEE, m_accounts[MINER_ACC_IDX].get_public_address()) });
  
  // check that mixing in 3 or 2 fake outputs fails because of using already spend outputs
  for(size_t fake_outputs_to_mixin = 2; fake_outputs_to_mixin >= 2; fake_outputs_to_mixin--)
  {
    r = false;
    try
    {
      bob_wlt->transfer(destinations, fake_outputs_to_mixin, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment);
    }
    catch (tools::error::not_enough_outs_to_mix& e)
    {
      r = true;
      LOG_PRINT_GREEN("Caught not_enough_outs_to_mix as expected, what() = " << e.what(), LOG_LEVEL_0);
    }
    CHECK_AND_ASSERT_MES(r, false, "Expected exception was not caught for fake_outputs_to_mixin == " << fake_outputs_to_mixin);
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "There are txs in the pool");
  }

  transaction tx = AUTO_VAL_INIT(tx);
  bob_wlt->transfer(destinations, 1, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment, tx); // mixing in 1 fake output should be okay as Alice did not spend it
  
  CHECK_AND_ASSERT_MES(tx.vin.size() == 1, false, "tx vin.size() is not 1");
  CHECK_AND_ASSERT_MES(tx.vin[0].type() == typeid(txin_to_key) && boost::get<txin_to_key>(tx.vin[0]).key_offsets.size() == 2, false, "Incorrect vin[0] type of key_offsets"); // make sure 1 fake output was used
  
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect number of txs in the pool");

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "There are txs in the pool");

  return true;
}
