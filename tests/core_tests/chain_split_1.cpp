// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "chain_split_1.h"

//using namespace std;

using namespace epee;
using namespace currency;


gen_simple_chain_split_1::gen_simple_chain_split_1()
{
  REGISTER_CALLBACK("check_split_not_switched", gen_simple_chain_split_1::check_split_not_switched);
  REGISTER_CALLBACK("check_split_not_switched2", gen_simple_chain_split_1::check_split_not_switched2);
  REGISTER_CALLBACK("check_split_switched", gen_simple_chain_split_1::check_split_switched);
  REGISTER_CALLBACK("check_split_not_switched_back", gen_simple_chain_split_1::check_split_not_switched_back);
  REGISTER_CALLBACK("check_split_switched_back_1", gen_simple_chain_split_1::check_split_switched_back_1);
  REGISTER_CALLBACK("check_split_switched_back_2", gen_simple_chain_split_1::check_split_switched_back_2);
  REGISTER_CALLBACK("check_mempool_1", gen_simple_chain_split_1::check_mempool_1);
  REGISTER_CALLBACK("check_mempool_2", gen_simple_chain_split_1::check_mempool_2);
  REGISTER_CALLBACK("check_orphaned_chain_1", gen_simple_chain_split_1::check_orphaned_chain_1);
  REGISTER_CALLBACK("check_orphan_readded", gen_simple_chain_split_1::check_orphan_readded);
  REGISTER_CALLBACK("check_orphaned_switched_to_main", gen_simple_chain_split_1::check_orphaned_switched_to_main);
}
//-----------------------------------------------------------------------------------------------------
bool gen_simple_chain_split_1::generate(std::vector<test_event_entry> &events) const
{
  uint64_t ts_start = 1450000000;

  GENERATE_ACCOUNT(first_miner_account);
  //                                                                                          events index
  MAKE_GENESIS_BLOCK(events, blk_0, first_miner_account, ts_start);                           //  0
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, first_miner_account);                                 //  1
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, first_miner_account);                                 //  2
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, first_miner_account);                                 //  3
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, first_miner_account);                                 //  4
  MAKE_NEXT_BLOCK(events, blk_5, blk_4, first_miner_account);                                 //  5
  MAKE_NEXT_BLOCK(events, blk_6, blk_5, first_miner_account);                                 //  6
  MAKE_NEXT_BLOCK(events, blk_7, blk_6, first_miner_account);                                 //  7
  MAKE_NEXT_BLOCK(events, blk_8, blk_7, first_miner_account);                                 //  8
  //split
  MAKE_NEXT_BLOCK(events, blk_9, blk_5, first_miner_account);                                 //  9
  MAKE_NEXT_BLOCK(events, blk_10, blk_9, first_miner_account);                                //  10
  DO_CALLBACK(events, "check_split_not_switched");                                            //  11
  /* 0    1    2    3    4    5     6     7     8     9     10    11    12    13    14    15                                             <-- main blockchain height
    (0 )-(1 )-(2 )-(3 )-(4 )-(5 ) -(6 ) -(7 ) -(8 )
                                \ -(9 ) -(10)                                                                                                                   */

  MAKE_NEXT_BLOCK(events, blk_11, blk_10, first_miner_account);                               //  12
  DO_CALLBACK(events, "check_split_not_switched2");                                           //  13
  /* 0    1    2    3    4    5     6     7     8     9     10    11    12    13    14    15                                             <-- main blockchain height
    (0 )-(1 )-(2 )-(3 )-(4 )-(5 ) -(6 ) -(7 ) -(8 )
                                \ -(9 ) -(10) -(11)                                                                                                             */

  MAKE_NEXT_BLOCK(events, blk_12, blk_11, first_miner_account);                               //  14
  DO_CALLBACK(events, "check_split_switched");                                                //  15
  /* 0    1    2    3    4    5     6     7     8     9     10    11    12    13    14    15                                             <-- main blockchain height
    (0 )-(1 )-(2 )-(3 )-(4 )-(5 ) -(9 ) -(10) -(11) -(12)
                                \ -(6 ) -(7 ) -(8 )                                                                                                             */

  MAKE_NEXT_BLOCK(events, blk_13, blk_12, first_miner_account);                               //  16
  MAKE_NEXT_BLOCK(events, blk_14, blk_13, first_miner_account);                               //  17
  MAKE_NEXT_BLOCK(events, blk_15, blk_14, first_miner_account);                               //  18
  MAKE_NEXT_BLOCK(events, blk_16, blk_15, first_miner_account);                               //  19
  /* 0    1    2    3    4    5     6     7     8     9     10    11    12    13    14    15                                             <-- main blockchain height
    (0 )-(1 )-(2 )-(3 )-(4 )-(5 ) -(9 ) -(10) -(11) -(12) -(13) -(14) -(15) -(16)
                                \ -(6 ) -(7 ) -(8 )                                                                                                             */

  //split again and check back switching
  MAKE_NEXT_BLOCK(events, blk_17, blk_8, first_miner_account);                                //  20
  MAKE_NEXT_BLOCK(events, blk_18, blk_17,  first_miner_account);                              //  21
  MAKE_NEXT_BLOCK(events, blk_19, blk_18,  first_miner_account);                              //  22
  MAKE_NEXT_BLOCK(events, blk_20, blk_19,  first_miner_account);                              //  23
  MAKE_NEXT_BLOCK(events, blk_21, blk_20,  first_miner_account);                              //  24
  DO_CALLBACK(events, "check_split_not_switched_back");                                       //  25
  /* 0    1    2    3    4    5     6     7     8     9     10    11    12    13    14    15                                             <-- main blockchain height
    (0 )-(1 )-(2 )-(3 )-(4 )-(5 ) -(9 ) -(10) -(11) -(12) -(13) -(14) -(15) -(16)
                                \ -(6 ) -(7 ) -(8 ) -(17) -(18) -(19) -(20) -(21)                                                                               */

  MAKE_NEXT_BLOCK(events, blk_22, blk_21, first_miner_account);                               //  26
  DO_CALLBACK(events, "check_split_switched_back_1");                                         //  27
  /* 0    1    2    3    4    5     6     7     8     9     10    11    12    13    14    15                                             <-- main blockchain height
    (0 )-(1 )-(2 )-(3 )-(4 )-(5 ) -(6 ) -(7 ) -(8 ) -(17) -(18) -(19) -(20) -(21) -(22)
                                \ -(9 ) -(10) -(11) -(12) -(13) -(14) -(15) -(16)                                                                               */

  MAKE_NEXT_BLOCK(events, blk_23, blk_22, first_miner_account);                               //  28
  DO_CALLBACK(events, "check_split_switched_back_2");                                         //  29
  /* 0    1    2    3    4    5     6     7     8     9     10    11    12    13    14    15                                             <-- main blockchain height
    (0 )-(1 )-(2 )-(3 )-(4 )-(5 ) -(6 ) -(7 ) -(8 ) -(17) -(18) -(19) -(20) -(21) -(22) -(23)
                                \ -(9 ) -(10) -(11) -(12) -(13) -(14) -(15) -(16)                                                                               */

  REWIND_BLOCKS(events, blk_23r, blk_23, first_miner_account);                                //  30...30+N-1 (N = unlock window)
  GENERATE_ACCOUNT(alice);
  MAKE_TX_FEE(events, tx_0, first_miner_account, alice, MK_TEST_COINS(10), MK_TEST_COINS(10), blk_23); //  N+30
  MAKE_TX_FEE(events, tx_1, first_miner_account, alice, MK_TEST_COINS(20), MK_TEST_COINS(20), blk_23); //  N+31
  MAKE_TX_FEE(events, tx_2, first_miner_account, alice, MK_TEST_COINS(30), MK_TEST_COINS(30), blk_23); //  N+32
  DO_CALLBACK(events, "check_mempool_1");                                                     //  N+33
  /* 0    1    2    3    4    5     6     7     8     9     10    11    12    13    14    15....25                                       <-- main blockchain height
    (0 )-(1 )-(2 )-(3 )-(4 )-(5 ) -(6 ) -(7 ) -(8 ) -(17) -(18) -(19) -(20) -(21) -(22) -(23) -(23r)
                                \ -(9 ) -(10) -(11) -(12) -(13) -(14) -(15) -(16)                                                                               */

  MAKE_NEXT_BLOCK_TX1(events, blk_24, blk_23r, first_miner_account, tx_0);                    //  N+34
  DO_CALLBACK(events, "check_mempool_2");                                                     //  N+35
  /* 0    1    2    3    4    5     6     7     8     9     10    11    12    13    14    15....25    26                                 <-- main blockchain height
    (0 )-(1 )-(2 )-(3 )-(4 )-(5 ) -(6 ) -(7 ) -(8 ) -(17) -(18) -(19) -(20) -(21) -(22) -(23) -(23r)-(24)
                                \ -(9 ) -(10) -(11) -(12) -(13) -(14) -(15) -(16)                                                                               */

  //check orphaned blocks
  MAKE_NEXT_BLOCK_NO_ADD(events, blk_orph_27, blk_16, first_miner_account);                   //  N+36
  MAKE_TX_FEE(events, tx_3, first_miner_account, alice, MK_TEST_COINS(25), MK_TEST_COINS(25), blk_orph_27); //  N+37
  MAKE_NEXT_BLOCK_TX1(events, blk_25, blk_orph_27, first_miner_account, tx_3);                //  N+38
  MAKE_NEXT_BLOCK(events, blk_26, blk_25, first_miner_account);                               //  N+39
  DO_CALLBACK(events, "check_orphaned_chain_1");                                              //  N+40
  /* 0    1    2    3    4    5     6     7     8     9     10    11    12    13    14    15    16....25    26                           <-- main blockchain height
    (0 )-(1 )-(2 )-(3 )-(4 )-(5 ) -(6 ) -(7 ) -(8 ) -(17) -(18) -(19) -(20) -(21) -(22) -(23) ...... (23r)-(24)
                                \ -(9 ) -(10) -(11) -(12) -(13) -(14) -(15) -(16) 
                                                                                \ ?o27?-{25} - {26}        <-- o27 is missing, so 25 and 26 rejected as orphans */ 

  ADD_BLOCK(events, blk_orph_27);                                                             //  N+41
  DO_CALLBACK(events, "check_orphan_readded");                                                //  N+42
  /* 0    1    2    3    4    5     6     7     8     9     10    11    12    13    14    15    16....25    26                           <-- main blockchain height
    (0 )-(1 )-(2 )-(3 )-(4 )-(5 ) -(6 ) -(7 ) -(8 ) -(17) -(18) -(19) -(20) -(21) -(22) -(23) ...... (23r)-(24)
                                \ -(9 ) -(10) -(11) -(12) -(13) -(14) -(15) -(16)-(o27)                    <-- 25 and 26 have gone as previously rejected       */
  
  ADD_BLOCK(events, blk_25);                                                                  //  N+43
  ADD_BLOCK(events, blk_26);                                                                  //  N+44
  /* 0    1    2    3    4    5     6     7     8     9     10    11    12    13    14    15    16....25    26                           <-- main blockchain height
    (0 )-(1 )-(2 )-(3 )-(4 )-(5 ) -(6 ) -(7 ) -(8 ) -(17) -(18) -(19) -(20) -(21) -(22) -(23) ...... (23r)-(24)
                                \ -(9 ) -(10) -(11) -(12) -(13) -(14) -(15) -(16)-(o27) -(25) -(26)        <-- 25 and 26 are re-added                           */

  REWIND_BLOCKS(events, blk_26r, blk_26, first_miner_account);                                //  N+45 .. 2N+44
  /* 0    1    2    3    4    5     6     7     8     9     10    11    12    13    14    15    16....25    26                           <-- main blockchain height
    (0 )-(1 )-(2 )-(3 )-(4 )-(5 ) -(6 ) -(7 ) -(8 ) -(17) -(18) -(19) -(20) -(21) -(22) -(23) ...... (23r)-(24)
                                \ -(9 ) -(10) -(11) -(12) -(13) -(14) -(15) -(16)-(o27) -(25) -(26)........(26r)                                                */

  MAKE_NEXT_BLOCK(events, blk_28, blk_26r, first_miner_account);                              //  2N+45
  /* 0    1    2    3    4    5     6     7     8     9     10    11    12    13    14    15    16....25    26     27                    <-- main blockchain height
    (0 )-(1 )-(2 )-(3 )-(4 )-(5 ) -(9 ) -(10) -(11) -(12) -(13) -(14) -(15) -(16)-(o27) -(25) -(26)........(26r) -(28)        <-- reorganize
                                \ -(6 ) -(7 ) -(8 ) -(17) -(18) -(19) -(20) -(21) -(22) -(23) ...... (23r)-(24)                                                 */

  DO_CALLBACK(events, "check_orphaned_switched_to_main");                                     //  2N+46

  return true;
}

//-----------------------------------------------------------------------------------------------------
bool gen_simple_chain_split_1::check_mempool_2(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  CHECK_TEST_CONDITION(c.get_pool_transactions_count() == 2);
  return true;
}
//-----------------------------------------------------------------------------------------------------
bool gen_simple_chain_split_1::check_mempool_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  CHECK_TEST_CONDITION(c.get_pool_transactions_count() == 3);
  return true;
}
//-----------------------------------------------------------------------------------------------------
bool gen_simple_chain_split_1::check_split_not_switched(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  //check height
  CHECK_TEST_CONDITION(c.get_current_blockchain_size() == 9);
  CHECK_TEST_CONDITION(c.get_blockchain_total_transactions() == 9);
  CHECK_TEST_CONDITION(c.get_tail_id() == get_block_hash(boost::get<currency::block>(events[8])));
  CHECK_TEST_CONDITION(c.get_alternative_blocks_count() == 2);
  return true;
}
//-----------------------------------------------------------------------------------------------------
bool gen_simple_chain_split_1::check_split_not_switched2(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  //check height
  CHECK_TEST_CONDITION(c.get_current_blockchain_size() == 9);
  CHECK_TEST_CONDITION(c.get_blockchain_total_transactions() == 9);
  CHECK_TEST_CONDITION(c.get_tail_id() == get_block_hash(boost::get<currency::block>(events[8])));
  CHECK_TEST_CONDITION(c.get_alternative_blocks_count() == 3);
  return true;
}
//-----------------------------------------------------------------------------------------------------
bool gen_simple_chain_split_1::check_split_switched(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{

  //check height
  CHECK_TEST_CONDITION(c.get_current_blockchain_size() == 10);
  CHECK_TEST_CONDITION(c.get_blockchain_total_transactions() == 10);
  CHECK_TEST_CONDITION(c.get_tail_id() == get_block_hash(boost::get<currency::block>(events[14])));
  CHECK_TEST_CONDITION(c.get_alternative_blocks_count() == 3);
  return true;
}
//-----------------------------------------------------------------------------------------------------
bool gen_simple_chain_split_1::check_split_not_switched_back(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  //check height
  CHECK_TEST_CONDITION(c.get_current_blockchain_size() == 14);
  CHECK_TEST_CONDITION(c.get_blockchain_total_transactions() == 14);
  CHECK_TEST_CONDITION(c.get_tail_id() == get_block_hash(boost::get<currency::block>(events[19])));
  CHECK_TEST_CONDITION(c.get_alternative_blocks_count() == 8);

  return true;
}
//-----------------------------------------------------------------------------------------------------
bool gen_simple_chain_split_1::check_split_switched_back_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{

  //check height
  CHECK_TEST_CONDITION(c.get_current_blockchain_size()== 15);
  CHECK_TEST_CONDITION(c.get_blockchain_total_transactions() == 15);
  CHECK_TEST_CONDITION(c.get_tail_id() == get_block_hash(boost::get<currency::block>(events[26])));
  CHECK_TEST_CONDITION(c.get_alternative_blocks_count() == 8);

  return true;
}//-----------------------------------------------------------------------------------------------------
bool gen_simple_chain_split_1::check_split_switched_back_2(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{

  //check height
  CHECK_TEST_CONDITION(c.get_current_blockchain_size() == 16);
  CHECK_TEST_CONDITION(c.get_blockchain_total_transactions() == 16);
  CHECK_TEST_CONDITION(c.get_tail_id() == get_block_hash(boost::get<currency::block>(events[28])));
  CHECK_TEST_CONDITION(c.get_alternative_blocks_count() == 8);
  return true;
}
//-----------------------------------------------------------------------------------------------------
bool gen_simple_chain_split_1::check_orphaned_chain_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  CHECK_TEST_CONDITION(c.get_current_blockchain_size()== 27);                                  
  CHECK_TEST_CONDITION(c.get_alternative_blocks_count() == 8);
  CHECK_TEST_CONDITION(c.get_tx_pool().get_transactions_count() == 3);
  return true;
}
//-----------------------------------------------------------------------------------------------------
bool gen_simple_chain_split_1::check_orphan_readded(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  CHECK_TEST_CONDITION(c.get_current_blockchain_size() == 27);
  CHECK_TEST_CONDITION(c.get_alternative_blocks_count() == 9);
  LOG_PRINT_L0("blockchain height: " << c.get_current_blockchain_size() << " alt block count: " << c.get_alternative_blocks_count());
  return true;
}
//-----------------------------------------------------------------------------------------------------
bool gen_simple_chain_split_1::check_orphaned_switched_to_main(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  CHECK_TEST_CONDITION(c.get_current_blockchain_size() == 28);
  CHECK_TEST_CONDITION(c.get_alternative_blocks_count() == 21);
  CHECK_TEST_CONDITION(c.get_tx_pool().get_transactions_count() == 3);
  return true;
}
