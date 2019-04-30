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
  req.outs_count = 4;
  req.use_forced_mix_outs = false;
  currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response res = AUTO_VAL_INIT(res);
  c.get_blockchain_storage().get_random_outs_for_amounts(req, res);
  CHECK_AND_ASSERT_MES(res.outs[0].outs.size() == 3, false, "Incorrect number of random outs returned.");

  return true;
}
