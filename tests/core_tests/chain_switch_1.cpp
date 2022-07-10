// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "chain_switch_1.h"
#include "random_helper.h"

using namespace epee;
using namespace currency;

#include "currency_core/currency_format_utils.h"

gen_chain_switch_1::gen_chain_switch_1()
  : m_miner_initial_amount(0)
{
  REGISTER_CALLBACK("check_split_not_switched", gen_chain_switch_1::check_split_not_switched);
  REGISTER_CALLBACK("check_split_switched", gen_chain_switch_1::check_split_switched);
}


//-----------------------------------------------------------------------------------------------------
bool gen_chain_switch_1::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1450000000;
  /*
   0    10...11   12...22    23   24   25              <- main blockchain height (assuming unlock window is 10 blocks)
  (0 )-(0r)-(1 )-(2 )-(2r) -(3 )-(4 )                  <- main chain, until 7 isn't connected
                        \ |-(5 )-(6 )|                 <- alt chain, until 7 isn't connected

  (0 )-(0r)-(1 )-(2 )-(2r) -(5 )-(6 )-(7 )             <- main chain, after reorganize
                        \ |-(3 )-(4 )|                 <- alt chain, after reorganize

  MAIN CHAIN                (3 )-(4 )
  miner      -5             -14  -16       = -35
  acc_1      +5              +3            = +8
  acc_2                      +8  +2        = +10
  acc_3                      +1  +13       = +14
  acc_4                      +2  +1        = +3

  ALT CHAIN                (5 )-(6 )-(7 )
  miner      -5             -20  -16       = -41
  acc_1      +5             +1   +2        = +8
  acc_2                     +3             = +3
  acc_3                     +2   +12       = +14
  acc_4                     +14  +2        = +16

  transactions ([n] - tx amount, (m) - block):
  (1)     : miner -[ 5]-> account_1 ( +5 in main chain,  +5 in alt chain)
  (3)     : miner -[ 7]-> account_2 ( +7 in main chain,  +0 in alt chain), tx will be in tx pool after switch
  (4), (6): miner -[11]-> account_3 (+11 in main chain, +11 in alt chain)
  (5)     : miner -[13]-> account_4 ( +0 in main chain, +13 in alt chain), tx will be in tx pool before switch

  transactions orders ([n] - tx amount, (m) - block):
  miner -[1], [2]-> account_1: in main chain (3), (3), in alt chain (5), (6)
  miner -[1], [2]-> account_2: in main chain (3), (4), in alt chain (5), (5)
  miner -[1], [2]-> account_3: in main chain (3), (4), in alt chain (6), (5)
  miner -[1], [2]-> account_4: in main chain (4), (3), in alt chain (5), (6)
  */

  GENERATE_ACCOUNT(miner_account);
  m_miner_account = miner_account;

  //                                                                                              events
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);                                     //  0
  m_miner_initial_amount = get_outs_money_amount(blk_0.miner_tx);
  MAKE_ACCOUNT(events, recipient_account_1);                                                      //  1
  MAKE_ACCOUNT(events, recipient_account_2);                                                      //  2
  MAKE_ACCOUNT(events, recipient_account_3);                                                      //  3
  MAKE_ACCOUNT(events, recipient_account_4);                                                      //  4
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account)                                             // <N blocks>
  MAKE_TX(events, tx_00, miner_account, recipient_account_1, MK_TEST_COINS(5), blk_0r);           //  5 + N
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_account, tx_00);                               //  6 + N
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_account);                                           //  7 + N
  REWIND_BLOCKS(events, blk_2r, blk_2, miner_account)                                             // <N blocks>

  // Transactions to test account balances after switch
  MAKE_TX_LIST_START(events, txs_blk_3, miner_account, recipient_account_2, MK_TEST_COINS(7), blk_2r);  //  8 + 2N
  MAKE_TX_LIST_START(events, txs_blk_4, miner_account, recipient_account_3, MK_TEST_COINS(11), blk_2r); //  9 + 2N
  MAKE_TX_LIST_START(events, txs_blk_5, miner_account, recipient_account_4, MK_TEST_COINS(13), blk_2r); // 10 + 2N
  std::list<transaction> txs_blk_6;
  txs_blk_6.push_back(txs_blk_4.front());

  // Transactions, that has different order in alt block chains
  MAKE_TX_LIST(events, txs_blk_3, miner_account, recipient_account_1, MK_TEST_COINS(1), blk_2r);        // 11 + 2N
  txs_blk_5.push_back(txs_blk_3.back());
  MAKE_TX_LIST(events, txs_blk_3, miner_account, recipient_account_1, MK_TEST_COINS(2), blk_2r);        // 12 + 2N
  txs_blk_6.push_back(txs_blk_3.back());

  MAKE_TX_LIST(events, txs_blk_3, miner_account, recipient_account_2, MK_TEST_COINS(1), blk_2r);        // 13 + 2N
  txs_blk_5.push_back(txs_blk_3.back());
  MAKE_TX_LIST(events, txs_blk_4, miner_account, recipient_account_2, MK_TEST_COINS(2), blk_2r);        // 14 + 2N
  txs_blk_5.push_back(txs_blk_4.back());

  MAKE_TX_LIST(events, txs_blk_3, miner_account, recipient_account_3, MK_TEST_COINS(1), blk_2r);        // 15 + 2N
  txs_blk_6.push_back(txs_blk_3.back());
  MAKE_TX_LIST(events, txs_blk_4, miner_account, recipient_account_3, MK_TEST_COINS(2), blk_2r);        // 16 + 2N
  txs_blk_5.push_back(txs_blk_4.back());

  MAKE_TX_LIST(events, txs_blk_4, miner_account, recipient_account_4, MK_TEST_COINS(1), blk_2r);        // 17 + 2N
  txs_blk_5.push_back(txs_blk_4.back());
  MAKE_TX_LIST(events, txs_blk_3, miner_account, recipient_account_4, MK_TEST_COINS(2), blk_2r);        // 18 + 2N
  txs_blk_6.push_back(txs_blk_3.back());

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3, blk_2r, miner_account, txs_blk_3);                       // 19 + 2N
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_4, blk_3, miner_account, txs_blk_4);                        // 20 + 2N
  //split
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_5, blk_2r, miner_account, txs_blk_5);                       // 22 + 2N
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_6, blk_5, miner_account, txs_blk_6);                        // 23 + 2N
  DO_CALLBACK(events, "check_split_not_switched");                                                // 21 + 2N
  MAKE_NEXT_BLOCK(events, blk_7, blk_6, miner_account);                                           // 24 + 2N
  DO_CALLBACK(events, "check_split_switched");                                                    // 25 + 2N

  return true;
}


//-----------------------------------------------------------------------------------------------------
bool gen_chain_switch_1::check_split_not_switched(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{

  m_recipient_account_1 = boost::get<account_base>(events[1]);
  m_recipient_account_2 = boost::get<account_base>(events[2]);
  m_recipient_account_3 = boost::get<account_base>(events[3]);
  m_recipient_account_4 = boost::get<account_base>(events[4]);

  std::list<block> blocks;
  bool r = c.get_blocks(0, 10000, blocks);
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(5 + 2 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW, blocks.size());
  CHECK_TEST_CONDITION(blocks.back() == boost::get<block>(events[20 + 2 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW]));  // blk_4

  CHECK_EQ(2, c.get_alternative_blocks_count());

  std::vector<currency::block> chain;
  map_hash2tx_t mtx;
  r = find_block_chain(events, chain, mtx, get_block_hash(blocks.back()));
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(MK_TEST_COINS(8),  get_balance(m_recipient_account_1, chain, mtx));
  CHECK_EQ(MK_TEST_COINS(10), get_balance(m_recipient_account_2, chain, mtx));
  CHECK_EQ(MK_TEST_COINS(14), get_balance(m_recipient_account_3, chain, mtx));
  CHECK_EQ(MK_TEST_COINS(3),  get_balance(m_recipient_account_4, chain, mtx));

  crypto::hash genesis_block_hash = currency::get_block_hash(boost::get<currency::block>(events[0]));
  uint64_t mined_after_genesis = 0;
  BOOST_FOREACH(currency::block b, blocks)
  {
    if (currency::get_block_hash(b) != genesis_block_hash)
      mined_after_genesis += get_outs_money_amount(b.miner_tx);
  }
  const size_t tx_main_chain_count = 11;
  uint64_t amount = m_miner_initial_amount + mined_after_genesis - MK_TEST_COINS(35) - TESTS_DEFAULT_FEE * tx_main_chain_count;
  uint64_t balance = get_balance(m_miner_account, chain, mtx);
  CHECK_TEST_CONDITION(amount > balance ? amount - balance : balance - amount <= DEFAULT_DUST_THRESHOLD);

  std::list<transaction> tx_pool;
  r = c.get_pool_transactions(tx_pool);
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(1, tx_pool.size());

  std::vector<wallet_out_info> tx_outs;
  uint64_t transfered;
  crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
  lookup_acc_outs(m_recipient_account_4.get_keys(), tx_pool.front(), get_tx_pub_key_from_extra(tx_pool.front()), tx_outs, transfered, derivation);
  CHECK_EQ(MK_TEST_COINS(13), transfered);

  m_chain_1.swap(blocks);
  m_tx_pool.swap(tx_pool);

  return true;
}

//-----------------------------------------------------------------------------------------------------
bool gen_chain_switch_1::check_split_switched(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{

  std::list<block> blocks;
  bool r = c.get_blocks(0, 10000, blocks);
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(6 + 2 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW, blocks.size());
  auto it = blocks.end();
  --it; --it; --it;
  CHECK_TEST_CONDITION(std::equal(blocks.begin(), it, m_chain_1.begin()));
  CHECK_TEST_CONDITION(blocks.back() == boost::get<block>(events[24 + 2 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW]));  // blk_7

  std::list<block> alt_blocks;
  r = c.get_alternative_blocks(alt_blocks);
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(2, c.get_alternative_blocks_count());

  // Some blocks that were in main chain are in alt chain now
  BOOST_FOREACH(block b, alt_blocks)
  {
    CHECK_TEST_CONDITION(m_chain_1.end() != std::find(m_chain_1.begin(), m_chain_1.end(), b));
  }

  std::vector<currency::block> chain;
  map_hash2tx_t mtx;
  r = find_block_chain(events, chain, mtx, get_block_hash(blocks.back()));
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(MK_TEST_COINS(8),  get_balance(m_recipient_account_1, chain, mtx));
  CHECK_EQ(MK_TEST_COINS(3),  get_balance(m_recipient_account_2, chain, mtx));
  CHECK_EQ(MK_TEST_COINS(14), get_balance(m_recipient_account_3, chain, mtx));
  CHECK_EQ(MK_TEST_COINS(16), get_balance(m_recipient_account_4, chain, mtx));

  crypto::hash genesis_block_hash = currency::get_block_hash(boost::get<currency::block>(events[0]));
  uint64_t mined_after_genesis = 0;
  BOOST_FOREACH(currency::block b, blocks)
  {
    if (currency::get_block_hash(b) != genesis_block_hash)
      mined_after_genesis += get_outs_money_amount(b.miner_tx);
  }
  const size_t tx_alt_chain_count = 11;
  uint64_t amount = m_miner_initial_amount + mined_after_genesis - MK_TEST_COINS(41) - TESTS_DEFAULT_FEE * tx_alt_chain_count;
  uint64_t balance = get_balance(m_miner_account, chain, mtx);
  CHECK_TEST_CONDITION(amount > balance ? amount - balance : balance - amount <= DEFAULT_DUST_THRESHOLD);

  std::list<transaction> tx_pool;
  r = c.get_pool_transactions(tx_pool);
  CHECK_TEST_CONDITION(r);
  CHECK_EQ(1, tx_pool.size());
  CHECK_TEST_CONDITION(!(tx_pool.front() == m_tx_pool.front()));

  std::vector<wallet_out_info> tx_outs;
  uint64_t transfered;
  crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);
  lookup_acc_outs(m_recipient_account_2.get_keys(), tx_pool.front(), tx_outs, transfered, derivation);
  CHECK_EQ(MK_TEST_COINS(7), transfered);

  return true;
}

//-----------------------------------------------------------------------------------------------------

bad_chain_switching_with_rollback::bad_chain_switching_with_rollback()
{
  REGISTER_CALLBACK_METHOD(bad_chain_switching_with_rollback, c1);
}

bool bad_chain_switching_with_rollback::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  GENERATE_ACCOUNT(bob_acc);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 4);

  MAKE_TX(events, tx_1, miner_acc, alice_acc, TESTS_DEFAULT_FEE * 2, blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);

  // make tx_2 referring to tx_1 output
  MAKE_TX(events, tx_2, alice_acc, bob_acc, TESTS_DEFAULT_FEE, blk_2);
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_2);

  // check balance at gentime

  // Alice should have nothing
  std::shared_ptr<tools::wallet2> alice_wlt;
  generator.init_test_wallet(alice_acc, get_block_hash(blk_0), alice_wlt);
  bool r = generator.refresh_test_wallet(events, alice_wlt.get(), get_block_hash(blk_3), CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 4 + 3);
  CHECK_AND_ASSERT_MES(r, false, "refresh_test_wallet failed");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "Alice", 0, 0, 0, 0, 0), false, "");

  // Bob should have TESTS_DEFAULT_FEE
  std::shared_ptr<tools::wallet2> bob_wlt;
  generator.init_test_wallet(bob_acc, get_block_hash(blk_0), bob_wlt);
  r = generator.refresh_test_wallet(events, bob_wlt.get(), get_block_hash(blk_3), CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 4 + 3);
  CHECK_AND_ASSERT_MES(r, false, "refresh_test_wallet failed");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", TESTS_DEFAULT_FEE, 0, 0, 0, 0), false, "");

  // start altchain from blk_0r, include tx_2 but NOT include tx_1
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_1a, blk_0r, miner_acc, tx_2);
  DO_CALLBACK(events, "mark_orphan_block");
  MAKE_NEXT_BLOCK(events, blk_2a, blk_1a, miner_acc);
  
  //  0 ... 14    15    16    17    18    19     <- height
  // (0 )- (0r)- (1 )- (2 )- (3 )-               <- main chain
  //         |   tx_1        tx_2               <- txs (tx_2 uses output of tx_1)
  //         |     |           |
  //         |     +--<--<--<--+
  //         |
  //          \- !1a!- !2a!-                     <- alt chain
  //             tx_2                            <- txs (tx_2 uses output of tx_1)

  // top block should be the 8last block in the main chain -- blk_3
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_3), get_block_hash(blk_3)));

  m_invalid_block_hash_to_check = get_block_hash(blk_1a);
  DO_CALLBACK(events, "c1");

  return true;
}

bool bad_chain_switching_with_rollback::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // altblocks now are marked as invalid only if rollback was unsuccessfull, so don't check it here
  // invalid block can be checked by have_block() but can't be retreived by get_block_by_hash
  //bool r = c.get_blockchain_storage().have_block(m_invalid_block_hash_to_check);
  //CHECK_AND_ASSERT_MES(r, false, "have_block failed when called with id = " << m_invalid_block_hash_to_check);

  block b = AUTO_VAL_INIT(b);
  bool r = c.get_blockchain_storage().get_block_by_hash(m_invalid_block_hash_to_check, b);
  CHECK_AND_ASSERT_MES(!r, false, "get_block_extended_info_by_hash suceeded, but expected to fail");

  return true;
}

//-----------------------------------------------------------------------------------------------------
struct tx_in_pool_info
{
  crypto::hash hash;
  size_t blobsize;
};

struct params_tx_pool
{
  params_tx_pool() {}
  params_tx_pool(crypto::hash hash, size_t blobsize) : txs{ tx_in_pool_info{ hash, blobsize } }
  {}
  params_tx_pool(crypto::hash hash1, size_t blobsize1, crypto::hash hash2, size_t blobsize2)
  {
    txs.push_back(tx_in_pool_info{ hash1, blobsize1 });
    txs.push_back(tx_in_pool_info{ hash2, blobsize2 });
  }

  std::vector<tx_in_pool_info> txs;
  BEGIN_KV_SERIALIZE_MAP()
    KV_SERIALIZE_CONTAINER_POD_AS_BLOB(txs)
  END_KV_SERIALIZE_MAP()
};


chain_switching_and_tx_with_attachment_blobsize::chain_switching_and_tx_with_attachment_blobsize()
{
  REGISTER_CALLBACK_METHOD(chain_switching_and_tx_with_attachment_blobsize, check_tx_pool_txs);
}

bool chain_switching_and_tx_with_attachment_blobsize::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  
  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW+4);

  tx_comment comment_att = AUTO_VAL_INIT(comment_att);
  comment_att.comment = get_random_text(CURRENCY_MAX_TRANSACTION_BLOB_SIZE * 9 / 10); // this is necessary to push cummulative block size out of full reward zone, so block reward will be non-default
  std::vector<attachment_v> attachment({comment_att});

  // create txs with attachment (and put it in the pool)
  MAKE_TX_ATTACH(events, tx_1, miner_acc, miner_acc, TESTS_DEFAULT_FEE, blk_0r, attachment);
  MAKE_TX_ATTACH(events, tx_2, miner_acc, miner_acc, TESTS_DEFAULT_FEE, blk_0r, attachment);

  // make sure the pool has correct txs
  DO_CALLBACK_PARAMS_STR(events, "check_tx_pool_txs", epee::serialization::store_t_to_json(params_tx_pool(get_transaction_hash(tx_1), get_object_blobsize(tx_1), get_transaction_hash(tx_2), get_object_blobsize(tx_2))));

  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, miner_acc);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1, miner_acc, std::list<transaction>({ tx_1, tx_2 }));

  // make sure tx pool is empty now
  DO_CALLBACK(events, "check_tx_pool_empty");

  // make altchain and grow it till reorganization happens
  MAKE_NEXT_BLOCK(events, blk_1a, blk_0r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_2a, blk_1a, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_3a, blk_2a, miner_acc);

  // make sure chain switching happened indeed
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_3a), get_block_hash(blk_3a)));

  // txs must have been added back to tx_pool, make sure they are
  DO_CALLBACK_PARAMS_STR(events, "check_tx_pool_txs", epee::serialization::store_t_to_json(params_tx_pool(get_transaction_hash(tx_1), get_object_blobsize(tx_1), get_transaction_hash(tx_2), get_object_blobsize(tx_2))));

  // put a block with txs
  // (this step triggered invalid block reward calculation a.k.a. "coinbase transaction doesn't use full amount of block reward"
  // in past due to invalid tx blob size calculations)
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_4a, blk_3a, miner_acc, std::list<transaction>({ tx_1, tx_2 }));

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_4a), get_block_hash(blk_4a)));

  // make sure tx pool is empty now
  DO_CALLBACK(events, "check_tx_pool_empty");

  return true;
}

bool chain_switching_and_tx_with_attachment_blobsize::check_tx_pool_txs(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  params_tx_pool p = AUTO_VAL_INIT(p);
  bool r = epee::serialization::load_t_from_json(p, boost::get<callback_entry>(events[ev_index]).callback_params);
  CHECK_AND_ASSERT_MES(r, false, "Can't obtain event params. Forgot to pass them?");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == p.txs.size(), false, "Tx pool has " << c.get_pool_transactions_count() << " transactions, expected: " << p.txs.size());

  for (auto &te : p.txs)
  {
    transaction tx = AUTO_VAL_INIT(tx);
    bool r = c.get_transaction(te.hash, tx);
    CHECK_AND_ASSERT_MES(r, false, "get_transaction failed for " << te.hash);

    crypto::hash hash = get_transaction_hash(tx);
    CHECK_AND_ASSERT_MES(te.hash == hash, false, "Tx in the pool has incorrect hash: " << hash << ", while expected is " << te.hash);

    size_t blobsize = get_object_blobsize(tx);
    CHECK_AND_ASSERT_MES(te.blobsize == blobsize, false, "Tx in the pool has incorrect blobsize == " << blobsize << ", while excepted value is " << te.blobsize);
  }

  return true;
}

//-----------------------------------------------------------------------------------------------------

bool chain_switching_when_gindex_spent_in_both_chains::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, alice_acc);
  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  uint64_t amount = get_outs_money_amount(blk_1.miner_tx) - TESTS_DEFAULT_FEE;

  MAKE_NEXT_BLOCK(events, blk_2, blk_1r, miner_acc);

  MAKE_TX_FEE(events, tx_1, alice_acc, alice_acc, amount, TESTS_DEFAULT_FEE, blk_2);
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_1);

  //  0      1      11     12     13     14             < height
  // (0 )-  (1 )...(1r)-  (2 )-  (3 )-                  < main chain
  //         ^        \          tx_1                   < txs
  //         |         \ 
  //         |          \-(2a)-  (3a)-  (4a)-           < alt chain
  //         +                   tx_1                   < txs

  MAKE_NEXT_BLOCK(events, blk_2a, blk_1r, miner_acc);
  MAKE_NEXT_BLOCK_TX1(events, blk_3a, blk_2a, miner_acc, tx_1);
  
  MAKE_NEXT_BLOCK(events, blk_4a, blk_3a, miner_acc);

  DO_CALLBACK(events, "check_tx_pool_empty");
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_4a), get_block_hash(blk_4a)));

  CREATE_TEST_WALLET(alice_wlt, alice_acc, blk_0);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_4a, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 4);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, amount);

  return true;
}

//-----------------------------------------------------------------------------------------------------

bool alt_chain_coins_pow_mined_then_spent::generate(std::vector<test_event_entry>& events) const
{
  // test idea: in alt chain mine and then spent some coins, then trigger chain switching

  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_acc);
  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  MAKE_NEXT_BLOCK(events, blk_2a, blk_1, alice_acc);
  REWIND_BLOCKS_N(events, blk_2ra, blk_2a, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  //  0      1      2      12      13
  // (0 )-  (1 )-  . . .  (1r)-  
  //          \ 
  //           \-  (2a)...(2ra)-  (3a)-
  //                |
  //                +-------------tx_1      tx_1 spents 2a.miner_tx output
  
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true)); // simulate alt-block tx behaviour
  MAKE_TX(events, tx_1, alice_acc, alice_acc, TESTS_DEFAULT_FEE, blk_2ra);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));
  
  MAKE_NEXT_BLOCK_TX1(events, blk_3a, blk_2ra, miner_acc, tx_1);

  DO_CALLBACK(events, "check_tx_pool_empty");
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_3a), get_block_hash(blk_3a)));

  return true;
};


//-----------------------------------------------------------------------------------------------------

bool alt_blocks_validation_and_same_new_amount_in_two_txs::generate(std::vector<test_event_entry>& events) const
{
  // test idea: make an alt block with two txs, containing the same, but never seen before output amount
  // Should be processed ok with no problems

  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_acc);
  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  // find out an amount that is never seen 
  uint64_t stub, new_amount = 0;
  bool r = calculate_amounts_many_outs_have_and_no_outs_have(get_outs_money_amount(blk_1.miner_tx), stub, new_amount);
  CHECK_AND_ASSERT_MES(r, false, "calculate_amounts_many_outs_have_and_no_outs_have failed");

  // main chain
  MAKE_NEXT_BLOCK(events, blk_2, blk_1r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);
  
  // alt chain
  MAKE_NEXT_BLOCK(events, blk_2a, blk_1r, miner_acc);

  // make two txs with one output (huge fee, probably - doesn't matter) with amount that is never seen before
  std::vector<tx_source_entry> sources;
  r = fill_tx_sources(sources, events, blk_1r, miner_acc.get_keys(), new_amount + TESTS_DEFAULT_FEE, 0);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  std::vector<tx_destination_entry> destinations;
  destinations.push_back(tx_destination_entry(new_amount, miner_acc.get_public_address())); // no cashback, just payment
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  uint64_t tx_version = get_tx_version(get_block_height(blk_3), m_hardforks);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_1, tx_version, 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_1);

  // second tx
  sources.clear();
  r = fill_tx_sources(sources, events, blk_1r, miner_acc.get_keys(), new_amount + TESTS_DEFAULT_FEE, 0, sources);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  transaction tx_2 = AUTO_VAL_INIT(tx_2);
  // use the same destinations
  tx_version = get_tx_version(get_block_height(blk_3), m_hardforks);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_2, tx_version, 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_2);
  
  // make an alt block with these txs
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3a, blk_2a, miner_acc, std::list<transaction>({ tx_1, tx_2 }));

  //  0      1       11      12      13
  // (0 )-  (1 )-...(1r)-   (2 )-   (3 )-
  //                  \ 
  //                   \-   (2a)-   (3a)-
  //                                tx_1
  //                                tx_2

  MAKE_NEXT_BLOCK(events, blk_4a, blk_3a, miner_acc); // this should switch the chains

  DO_CALLBACK(events, "check_tx_pool_empty");
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_4a), get_block_hash(blk_4a)));

  return true;
}

//-----------------------------------------------------------------------------------------------------

alt_blocks_with_the_same_txs::alt_blocks_with_the_same_txs()
{
  REGISTER_CALLBACK_METHOD(alt_blocks_with_the_same_txs, check_tx_related_to_altblock);
  REGISTER_CALLBACK_METHOD(alt_blocks_with_the_same_txs, check_tx_not_related_to_altblock);
}

bool alt_blocks_with_the_same_txs::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: check that many two alt blocks having the same tx are correctly handled with respect to is_tx_related_to_altblock()

  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_acc);
  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  //  0      1       11      12      13      14
  // (0 )-  (1 )-...(1r)-   (2 )-   (3 )-
  //                  \     tx_0
  //                   \ 
  //                    \-  (2a)-   (3a)-   (4 )-
  //                        tx_0

  MAKE_TX(events, tx_0, miner_acc, miner_acc, TESTS_DEFAULT_FEE, blk_1r);

  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_0);
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);

  crypto::hash tx_0_hash = get_transaction_hash(tx_0);
  DO_CALLBACK_PARAMS(events, "check_tx_not_related_to_altblock", tx_0_hash);

  MAKE_NEXT_BLOCK_TX1(events, blk_2a, blk_1r, miner_acc, tx_0);
  MAKE_NEXT_BLOCK(events, blk_3a, blk_2a, miner_acc);
  
  DO_CALLBACK_PARAMS(events, "check_tx_related_to_altblock", tx_0_hash);

  // this should trigger reorganize
  MAKE_NEXT_BLOCK(events, blk_4a, blk_3a, miner_acc);

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_4a), get_block_hash(blk_4a)));

  DO_CALLBACK_PARAMS(events, "check_tx_related_to_altblock", tx_0_hash);

  return true;
}

bool alt_blocks_with_the_same_txs::check_tx_related_to_altblock(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  crypto::hash tx_0_hash = null_hash;
  epee::string_tools::hex_to_pod(boost::get<callback_entry>(events[ev_index]).callback_params, tx_0_hash);

  bool r = c.get_blockchain_storage().is_tx_related_to_altblock(tx_0_hash);
  CHECK_AND_ASSERT_MES(r, false, "tx " << tx_0_hash << " is expected to be related to an altblock, but BCS returned false");

  return true;
}

bool alt_blocks_with_the_same_txs::check_tx_not_related_to_altblock(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  crypto::hash tx_0_hash = null_hash;
  epee::string_tools::hex_to_pod(boost::get<callback_entry>(events[ev_index]).callback_params, tx_0_hash);

  bool r = !c.get_blockchain_storage().is_tx_related_to_altblock(tx_0_hash);
  CHECK_AND_ASSERT_MES(r, false, "tx " << tx_0_hash << " is expected to NOT be related to an altblock, but BCS returned true");

  return true;
}

//-----------------------------------------------------------------------------------------------------

bool chain_switching_when_out_spent_in_alt_chain_mixin::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure a tx can spend an output with mixins from another tx when both txs are in an altchain.
  bool r = false;
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  GENERATE_ACCOUNT(bob_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_acc);
  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  //  0      1       11      12      13      14   
  // (0 )-  (1 )-...(1r)-   (2 )-   (3 )-            <-- main chain
  //                  \     
  //                   \ 
  //                    \-  (2a)-   (3a)-   (4a)-
  //                        tx_0 <- tx_1             // tx_1 spends output from tx_0

  // send batch of 10 x 5 test coins to Alice for easier tx_1 construction (and to generate free decoys)
  transaction tx_0;
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_1r, miner_acc.get_keys(), alice_acc.get_public_address(), MK_TEST_COINS(50), 10,
    TESTS_DEFAULT_FEE, tx_0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_0);

  MAKE_NEXT_BLOCK(events, blk_2, blk_1r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);

  MAKE_NEXT_BLOCK_TX1(events, blk_2a, blk_1r, miner_acc, tx_0);

  // make sure Alice received exactly 50 test coins
  CREATE_TEST_WALLET(alice_wlt, alice_acc, blk_0);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_2a, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, MK_TEST_COINS(50));

  // Alice spends her 5 test coins received by tx_0
  MAKE_TX_FEE_MIX(events, tx_1, alice_acc, bob_acc, MK_TEST_COINS(4), TESTS_DEFAULT_FEE, 3 /* nmix */, blk_2a);
  events.pop_back(); // pop back tx_1 as it won't go into the tx pool normally because of alt chain

  // simulate handling a block with that tx: handle tx like going with the block...
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
  events.push_back(tx_1);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));

  MAKE_NEXT_BLOCK_TX1(events, blk_3a, blk_2a, miner_acc, tx_1);
  MAKE_NEXT_BLOCK(events, blk_4a, blk_3a, miner_acc);

  // make sure Alice has correct balance
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_4a, 2);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, MK_TEST_COINS(45));

  // make sure chain successfully switched
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_4a), get_block_hash(blk_4a)));

  return true;
}

//-----------------------------------------------------------------------------------------------------

bool chain_switching_when_out_spent_in_alt_chain_ref_id::generate(std::vector<test_event_entry>& events) const
{
  random_state_test_restorer::reset_random(0); // to make the test deterministic
  uint64_t ts = 1450000000;
  test_core_time::adjust(ts);

  // Test idea: make sure tx can spend (using ref_by_id) an output from another tx when both txs are in an altchain.
  bool r = false;
  GENERATE_ACCOUNT(miner_acc);  
  GENERATE_ACCOUNT(alice_acc);
  GENERATE_ACCOUNT(bob_acc);
  miner_acc.set_createtime(ts);
  alice_acc.set_createtime(ts);
  bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_acc);
  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  //  0      1       11      12      13      14   
  // (0 )-  (1 )-...(1r)-   (2 )-   (3 )-            <- main chain
  //                  \     
  //                   \ 
  //                    \-  (2a)-   (3a)-   (4a)-
  //                        tx_0 <- tx_1             // tx_1 spends an output from tx_0 using ref_by_id and mixins

  // send batch of 10 x 5 test coins to Alice for easier tx_0 construction
  transaction tx_0;
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_1r, miner_acc.get_keys(), alice_acc.get_public_address(), MK_TEST_COINS(50), 10,
    TESTS_DEFAULT_FEE, tx_0, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");

  // make sure tx_0 really use ref_by_id
  size_t refs_count = 0, gindex_count = 0;
  count_ref_by_id_and_gindex_refs_for_tx_inputs(tx_0, refs_count, gindex_count);
  CHECK_AND_ASSERT_MES(refs_count == 1 && gindex_count == 0, false, "incorrect input references: " << refs_count << ", " << gindex_count);

  events.push_back(tx_0);

  MAKE_NEXT_BLOCK(events, blk_2, blk_1r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);

  MAKE_NEXT_BLOCK_TX1(events, blk_2a, blk_1r, miner_acc, tx_0);

  // make sure Alice received exactly 50 test coins
  CREATE_TEST_WALLET(alice_wlt, alice_acc, blk_0);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_2a, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, MK_TEST_COINS(50));

  // Alice spends her 5 test coins received by tx_0
  transaction tx_1;
  std::vector<tx_destination_entry> destinations;
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(4), bob_acc.get_public_address()));
  size_t nmix = 3;
  r = construct_tx_to_key(m_hardforks, events, tx_1, blk_2a, alice_acc, destinations, TESTS_DEFAULT_FEE, nmix, 0, empty_extra, empty_attachment, true, true, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_to_key failed");

  // make sure tx_1 really use ref_by_id
  refs_count = 0, gindex_count = 0;
  count_ref_by_id_and_gindex_refs_for_tx_inputs(tx_1, refs_count, gindex_count);
  CHECK_AND_ASSERT_MES(refs_count == nmix + 1 && gindex_count == 0, false, "incorrect input references: " << refs_count << ", " << gindex_count);

  // simulate handling a block with that tx: handle tx like going with the block...
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
  events.push_back(tx_1);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));

  MAKE_NEXT_BLOCK_TX1(events, blk_3a, blk_2a, miner_acc, tx_1);
  MAKE_NEXT_BLOCK(events, blk_4a, blk_3a, miner_acc);

  // make sure Alice has correct balance
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_4a, 2);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, MK_TEST_COINS(45));

  // make sure chain successfully switched
  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_4a), get_block_hash(blk_4a)));

  return true;
}
