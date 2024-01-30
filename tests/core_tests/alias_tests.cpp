// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "alias_tests.h"
#include "tx_builder.h"
#include "random_helper.h"

using namespace epee;
using namespace currency;

#define FIRST_ALIAS_NAME   "first--01234567890"
#define SECOND_ALIAS_NAME  "second--01234567890"
#define THIRD_ALIAS_NAME   "test-alias-via-tx-1--01234567890"
#define FOURTH_NAME        "fourth--01234567890"
#define FIFTH_NAME         "fifth--01234567890"
#define SIX_NAME           "sixsix-double--01234567890"

bool put_alias_via_tx_to_list(const currency::hard_forks_descriptor& hf, 
  std::vector<test_event_entry>& events,
  std::list<currency::transaction>& tx_set,
  const block& head_block,
  const std::string& alias_name,
  const account_base& miner_acc,
  const account_base& alias_acc,
  test_generator& generator)
{
  currency::extra_alias_entry ai2 = AUTO_VAL_INIT(ai2);
  ai2.m_alias = alias_name;
  ai2.m_address = alias_acc.get_keys().account_address;
  ai2.m_text_comment = "ssdss";
  return put_alias_via_tx_to_list(hf, events, tx_set, head_block, miner_acc, ai2, generator);
}

bool put_next_block_with_alias_in_tx(const currency::hard_forks_descriptor& hf, 
                                     std::vector<test_event_entry>& events,
                                     block& b, 
                                     const block& head_block, 
                                     const account_base& miner_acc, 
                                     const currency::extra_alias_entry& ai,
                                     test_generator& generator)
{
  std::list<currency::transaction> txs_0;
  if (!put_alias_via_tx_to_list(hf, events, txs_0, head_block, miner_acc, ai, generator))
    return false;
    
  MAKE_NEXT_BLOCK_TX_LIST(events, blk, head_block, miner_acc, txs_0);
  b = blk;
  return true;
}


bool put_next_block_with_alias_in_tx(const currency::hard_forks_descriptor& hf, std::vector<test_event_entry>& events,
  block& b,
  const block& head_block,
  const std::string& alias_name,
  const account_base& miner_acc,
  const account_base& alias_acc,
  test_generator& generator)
{
  currency::extra_alias_entry ai2 = AUTO_VAL_INIT(ai2);
  ai2.m_alias = alias_name;
  ai2.m_address = alias_acc.get_keys().account_address;
  ai2.m_text_comment = "ssdss";

  return put_next_block_with_alias_in_tx(hf, events, b, head_block, miner_acc, ai2, generator);
}

#define MAKE_BLOCK_WITH_ALIAS_IN_TX(EVENTS, NAME, HEAD, ALIAS_NAME) \
  block NAME; \
  put_next_block_with_alias_in_tx(m_hardforks, EVENTS, NAME, HEAD, ALIAS_NAME, miner_account, second_acc, generator)

#define MAKE_BLOCK_WITH_ALIAS_INFO_IN_TX(EVENTS, NAME, HEAD, MINER_ACCOUNT, ALIAS_INFO) \
  block NAME; \
  put_next_block_with_alias_in_tx(m_hardforks, EVENTS, NAME, HEAD, MINER_ACCOUNT, ALIAS_INFO, generator)

//------------------------------------------------------------------------------

gen_alias_tests::gen_alias_tests()
  : m_invalid_tx_index(std::numeric_limits<decltype(m_invalid_tx_index)>::max())
  , m_invalid_block_index(std::numeric_limits<decltype(m_invalid_block_index)>::max())
  , m_h(0)
{
  REGISTER_CALLBACK_METHOD(gen_alias_tests, mark_invalid_tx);
  REGISTER_CALLBACK_METHOD(gen_alias_tests, mark_invalid_block);
  REGISTER_CALLBACK_METHOD(gen_alias_tests, check_first_alias_added);
  REGISTER_CALLBACK_METHOD(gen_alias_tests, check_second_alias_added);
  REGISTER_CALLBACK_METHOD(gen_alias_tests, check_aliases_removed);
  REGISTER_CALLBACK_METHOD(gen_alias_tests, check_splitted_back);
  REGISTER_CALLBACK_METHOD(gen_alias_tests, check_alias_changed);
  REGISTER_CALLBACK_METHOD(gen_alias_tests, check_alias_not_changed);
  REGISTER_CALLBACK_METHOD(gen_alias_tests, check_alias_added_in_tx);
  REGISTER_CALLBACK_METHOD(gen_alias_tests, check_height_not_changed);
  REGISTER_CALLBACK_METHOD(gen_alias_tests, check_height_changed);
  REGISTER_CALLBACK_METHOD(gen_alias_tests, check_too_many_aliases_registration);
}

bool gen_alias_tests::generate(std::vector<test_event_entry>& events) const
{
  bool r = false;
  GENERATE_ACCOUNT(preminer_account);
  GENERATE_ACCOUNT(miner_account);
  m_accounts.push_back(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, preminer_account, test_core_time::get_time());

  size_t small_outs_to_transfer = MAX_ALIAS_PER_BLOCK + 10;

  // rebuild genesis miner tx
  std::vector<tx_destination_entry> destinations(small_outs_to_transfer, tx_destination_entry(TESTS_DEFAULT_FEE * 11, preminer_account.get_public_address()));
  CHECK_AND_ASSERT_MES(replace_coinbase_in_genesis_block(destinations, generator, events, blk_0), false, "");

  DO_CALLBACK(events, "configure_core");
  MAKE_ACCOUNT(events, first_acc);
  MAKE_ACCOUNT(events, second_acc);
  MAKE_ACCOUNT(events, third_acc);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, preminer_account, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  size_t outs_in_a_tx = 32 /* <-- TODO change to some constant, meaning max outputs in post HF4 txs */ - 1;
  std::vector<transaction> txs;
  while(small_outs_to_transfer > 0)
  {
    size_t outs_count = std::min(outs_in_a_tx, small_outs_to_transfer);
    small_outs_to_transfer -= outs_count;
    txs.push_back(transaction{});
    r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, preminer_account.get_keys(), miner_account.get_public_address(), outs_count * TESTS_DEFAULT_FEE * 11, outs_count, TESTS_DEFAULT_FEE, txs.back());
    CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
    ADD_CUSTOM_EVENT(events, txs.back());
  }
  // split into two blocks because they won't fit into one
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_a, blk_0r, miner_account, std::list<transaction>(txs.begin(),                  txs.begin() + txs.size() / 2));
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_a2, blk_a, miner_account, std::list<transaction>(txs.begin() + txs.size() / 2, txs.end()));

  REWIND_BLOCKS_N_WITH_TIME(events, blk_ar, blk_a2, miner_account, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  MAKE_NEXT_BLOCK(events, blk_1, blk_ar, miner_account);                                                               // 4N+7
  currency::extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = FIRST_ALIAS_NAME;
  ai.m_text_comment = "first@first.com blablabla";
  ai.m_address = first_acc.get_keys().account_address;
  MAKE_BLOCK_WITH_ALIAS_INFO_IN_TX(events, blk_2, blk_1, miner_account, ai);                                           // 4N+8,4N+9
  DO_CALLBACK(events, "check_first_alias_added");                                                                      // 4N+10

  ai.m_alias = SECOND_ALIAS_NAME;
  ai.m_text_comment = "second@second.com blablabla";
  ai.m_address = second_acc.get_keys().account_address;
  MAKE_BLOCK_WITH_ALIAS_INFO_IN_TX(events, blk_3, blk_2, miner_account, ai);                                           // 4N+11,4N+12
  DO_CALLBACK(events, "check_second_alias_added");                                                                     // 4N+13
  
  //check split with remove alias
  MAKE_NEXT_BLOCK(events, blk_2_split, blk_1, miner_account);                                                          // 4N+14
  MAKE_NEXT_BLOCK(events, blk_3_split, blk_2_split, miner_account);                                                    // 4N+15
  MAKE_NEXT_BLOCK(events, blk_4_split, blk_3_split, miner_account);                                                    // 4N+16
  DO_CALLBACK(events, "check_aliases_removed");                                                                        // 4N+17
    
  //make back split to original and try update alias
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_account);                                                                // 4N+18
  MAKE_NEXT_BLOCK(events, blk_5, blk_4, miner_account);                                                                // 4N+19
  DO_CALLBACK(events, "check_splitted_back");                                                                          // 4N+20


  currency::extra_alias_entry ai_upd = AUTO_VAL_INIT(ai_upd);
  ai_upd.m_alias = FIRST_ALIAS_NAME;
  ai_upd.m_address =third_acc.get_keys().account_address;
  ai_upd.m_text_comment = "changed alias haha";
  r = sign_extra_alias_entry(ai_upd, first_acc.get_keys().account_address.spend_public_key, first_acc.get_keys().spend_secret_key);
  CHECK_AND_ASSERT_MES(r, false, "failed to sign update_alias");
  MAKE_BLOCK_WITH_ALIAS_INFO_IN_TX(events, blk_6, blk_5, miner_account, ai_upd);                                       // 4N+21,N+22
  DO_CALLBACK(events, "check_alias_changed");                                                                          // 4N+23

  //try to make fake alias change
  currency::extra_alias_entry ai_upd_fake = AUTO_VAL_INIT(ai_upd_fake);
  ai_upd_fake.m_alias = SECOND_ALIAS_NAME;
  ai_upd_fake.m_address =third_acc.get_keys().account_address;
  ai_upd_fake.m_text_comment = "changed alias haha";
  r = sign_extra_alias_entry(ai_upd_fake, second_acc.get_keys().account_address.spend_public_key, second_acc.get_keys().spend_secret_key);
  CHECK_AND_ASSERT_MES(r, false, "failed to sign update_alias");
  ai_upd_fake.m_text_comment = "changed alias haha - fake"; // changed text, signature became wrong
  std::list<currency::transaction> tx_list;
  r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_6, miner_account, ai_upd_fake, generator);                         // 4N+24
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list");
  DO_CALLBACK(events, "mark_invalid_block");                                                                           // 4N+25
  // EXPECTED: blk_7 is rejected as containing incorrect tx (wrong alias sig)
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_7, blk_6, miner_account, tx_list);                                               // 4N+26
  DO_CALLBACK(events, "check_alias_not_changed");                                                                      // 4N+27

  account_base someone;
  someone.generate();
  block blk_8;
  r = put_next_block_with_alias_in_tx(m_hardforks, events, blk_8, blk_6, THIRD_ALIAS_NAME, miner_account, someone, generator);      // 4N+28,4N+29
  CHECK_AND_ASSERT_MES(r, false, "put_next_block_with_alias_in_tx failed");

  MAKE_NEXT_BLOCK(events, blk_9, blk_8, miner_account);                                                                // 4N+30
  DO_CALLBACK(events, "check_alias_added_in_tx");                                                                      // 4N+31
  
  // lets try to register same name
  ai = AUTO_VAL_INIT(ai);
  ai.m_alias = THIRD_ALIAS_NAME;
  ai.m_address = miner_account.get_public_address();
  tx_list.clear();
  DO_CALLBACK(events, "mark_invalid_tx");                                                                              // 4N+32
  // EXPECTED: tx is rejected, because alias is already registered
  r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_9, miner_account, ai, generator);                                  // 4N+33
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list");
  DO_CALLBACK(events, "check_alias_not_changed");                                                                      // 4N+34

  DO_CALLBACK(events, "check_height_not_changed");                                                                     // 4N+35

  //check notmal tx in tx pool
  MAKE_TX_LIST_START(events, txs_0, miner_account, miner_account, MK_TEST_COINS(1), blk_9);                            // 4N+36

  if (!put_alias_via_tx_to_list(m_hardforks, events, txs_0, blk_9, FOURTH_NAME, miner_account, miner_account, generator))           // 4N+37
    return false;

  someone.generate();
  if (!put_alias_via_tx_to_list(m_hardforks, events, txs_0, blk_9, FIFTH_NAME, miner_account, someone, generator))                  // 4N+38
    return false;

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_13, blk_9, miner_account, txs_0);                                                // 4N+39
  DO_CALLBACK(events, "check_height_changed");                                                                         // 4N+40
  //
  //check duplicate tx in tx pool
  MAKE_TX_LIST_START(events, txs_2, miner_account, miner_account, MK_TEST_COINS(1), blk_13);                           // 4N+41

  someone.generate();
  if (!put_alias_via_tx_to_list(m_hardforks, events, txs_2, blk_13, SIX_NAME, miner_account, someone, generator))                   // 4N+42
    return false;
  DO_CALLBACK(events, "mark_invalid_tx");                                                                              // 4N+43
  // EXPECTED: the next tx is rejected, because alias is already registered
  if (!put_alias_via_tx_to_list(m_hardforks, events, txs_2, blk_13, SIX_NAME, miner_account, miner_account, generator))             // 4N+44
    return false;

  DO_CALLBACK(events, "mark_invalid_block");                                                                           // 4N+45
  // EXPECTED: block is rejected as containing invalid tx
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_14, blk_13, miner_account, txs_2);                                               // 4N+46

  DO_CALLBACK(events, "check_height_not_changed");                                                                     // 4N+47
  
  DO_CALLBACK(events, "clear_tx_pool");                                                                                // 4N+48
  DO_CALLBACK(events, "check_too_many_aliases_registration");                                                          // 4N+49

  return true;
}

bool gen_alias_tests::check_first_alias_added(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  const currency::account_base& first_acc = boost::get<const currency::account_base>(events[2]);

  currency::extra_alias_entry_base ai = AUTO_VAL_INIT(ai);
  bool r = c.get_blockchain_storage().get_alias_info(FIRST_ALIAS_NAME, ai);
  CHECK_AND_ASSERT_MES(r, false, "first alias name check failed");
  CHECK_AND_ASSERT_MES(ai.m_address.spend_public_key == first_acc.get_keys().account_address.spend_public_key
                    && ai.m_address.view_public_key == first_acc.get_keys().account_address.view_public_key, false, "first alias name check failed");  
  return true;
}
bool gen_alias_tests::check_second_alias_added(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  const currency::account_base& second_acc = boost::get<const currency::account_base>(events[3]);

  currency::extra_alias_entry_base ai = AUTO_VAL_INIT(ai);
  bool r = c.get_blockchain_storage().get_alias_info(SECOND_ALIAS_NAME, ai);
  CHECK_AND_ASSERT_MES(r, false, "first alias name check failed");
  CHECK_AND_ASSERT_MES(ai.m_address.spend_public_key == second_acc.get_keys().account_address.spend_public_key
                    && ai.m_address.view_public_key == second_acc.get_keys().account_address.view_public_key, false, "second alias name check failed");  

  return true;
}
bool gen_alias_tests::check_aliases_removed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::extra_alias_entry_base ai = AUTO_VAL_INIT(ai);
  bool r = c.get_blockchain_storage().get_alias_info(FIRST_ALIAS_NAME, ai);
  CHECK_AND_ASSERT_MES(!r, false, "first alias name check failed");
  r = c.get_blockchain_storage().get_alias_info(SECOND_ALIAS_NAME, ai);
  CHECK_AND_ASSERT_MES(!r, false, "second alias name check failed");
  return true;
}
bool gen_alias_tests::check_alias_added_in_tx(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::extra_alias_entry_base ai = AUTO_VAL_INIT(ai);
  bool r = c.get_blockchain_storage().get_alias_info(THIRD_ALIAS_NAME, ai);
  CHECK_AND_ASSERT_MES(r, false, "third alias name check failed");
  m_h = c.get_blockchain_storage().get_current_blockchain_size();
  return true;
}

bool gen_alias_tests::check_height_not_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  uint64_t new_h = c.get_blockchain_storage().get_current_blockchain_size();
  CHECK_AND_ASSERT_MES(new_h == m_h, false, "third alias name check failed");
  return true;
}

bool gen_alias_tests::check_height_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  uint64_t new_h = c.get_blockchain_storage().get_current_blockchain_size();
  CHECK_AND_ASSERT_MES(new_h == m_h+1, false, "fifth alias name check failed");

  currency::extra_alias_entry_base ai = AUTO_VAL_INIT(ai);
  bool r = c.get_blockchain_storage().get_alias_info(FOURTH_NAME, ai);
  CHECK_AND_ASSERT_MES(r, false, "FOURTH_NAME alias name check failed");

  r = c.get_blockchain_storage().get_alias_info(FIFTH_NAME, ai);
  CHECK_AND_ASSERT_MES(r, false, "FIFTH_NAME alias name check failed");
  m_h = c.get_blockchain_storage().get_current_blockchain_size();

  return true;
}

bool gen_alias_tests::check_splitted_back(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  return check_first_alias_added(c, ev_index, events) && check_second_alias_added(c, ev_index, events);
}

bool gen_alias_tests::check_alias_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  const currency::account_base& third_acc = boost::get<const currency::account_base>(events[4]);

  currency::extra_alias_entry_base ai = AUTO_VAL_INIT(ai);
  bool r = c.get_blockchain_storage().get_alias_info(FIRST_ALIAS_NAME, ai);
  CHECK_AND_ASSERT_MES(r, false, "first alias name check failed");
  CHECK_AND_ASSERT_MES(ai.m_address.spend_public_key == third_acc.get_keys().account_address.spend_public_key
                    && ai.m_address.view_public_key == third_acc.get_keys().account_address.view_public_key, false, "first alias update name check failed");  

  return true;
}

bool gen_alias_tests::check_alias_not_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  return check_second_alias_added(c, ev_index, events);
}

bool gen_alias_tests::check_too_many_aliases_registration(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // register too many aliases
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  miner_wlt->refresh();

  uint64_t estimated_alias_cost = get_alias_coast_from_fee(std::string(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, 'a'), c.get_blockchain_storage().get_tx_fee_median());
  
  extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_address = m_accounts[MINER_ACC_IDX].get_public_address();

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count() << ", expected: 0");

  // attempt to create block from block template, then check how many aliases it will contain
  currency::block b;
  wide_difficulty_type diff;
  uint64_t height;
  blobdata extra = AUTO_VAL_INIT(extra);
  bool r = c.get_block_template(b, ai.m_address, ai.m_address, diff, height, extra);
  CHECK_AND_ASSERT_MES(r, false, "get_block_template failed");
  CHECK_AND_ASSERT_MES(b.tx_hashes.empty(), false, "block template has some txs, expected--none");
  
  account_base someone;
  const size_t total_alias_to_gen = MAX_ALIAS_PER_BLOCK + 2;
  try
  {
    log_level_scope_changer llsc(LOG_LEVEL_0); // eliminate all the mess in the log during alias registration, comment this out if case of debugging

    for (size_t i = 0; i < total_alias_to_gen; ++i)
    {
      LOG_PRINT_YELLOW("Generating alias #" << i << " of " << total_alias_to_gen << "...", LOG_LEVEL_0);
      someone.generate();
      ai.m_address = someone.get_public_address();
      ai.m_alias = gen_random_alias(random_in_range(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED + 20));
      transaction alias_reg_tx = AUTO_VAL_INIT(alias_reg_tx);
      miner_wlt->request_alias_registration(ai, alias_reg_tx, TESTS_DEFAULT_FEE, estimated_alias_cost);
      
      b.tx_hashes.push_back(get_transaction_hash(alias_reg_tx)); // add this tx to block template
    }
  }
  catch (std::exception& e)
  {
    LOG_ERROR("Caught an exception while registering an alias: " << e.what());
    return false;
  }

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == total_alias_to_gen, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count() << ", expected: " << total_alias_to_gen);

  // complete block template and try to process it
  r = miner::find_nonce_for_given_block(b, diff, height);
  CHECK_AND_ASSERT_MES(r, false, "find_nonce_for_given_block failed");

  currency::block_verification_context bvc = AUTO_VAL_INIT(bvc);
  c.handle_incoming_block(t_serializable_object_to_blob(b), bvc);
  CHECK_AND_NO_ASSERT_MES(!bvc.m_added_to_main_chain, false, "block verification context check failed: m_added_to_main_chain");
  CHECK_AND_NO_ASSERT_MES(bvc.m_verification_failed, false, "block verification context check failed: ! m_verification_failed ");

  return true;
}

//------------------------------------------------------------------------------
gen_alias_strange_data::gen_alias_strange_data()
{
  m_alice.generate();
  REGISTER_CALLBACK_METHOD(gen_alias_strange_data, check_alias_changed);
}

bool gen_alias_strange_data::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1450000000;

  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts_start);
  set_hard_fork_heights_to_generator(generator);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  std::list<currency::transaction> tx_list;

  // empty comment
  currency::extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = std::string(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, 'a');
  ai.m_text_comment = "";
  ai.m_address = m_alice.get_public_address();
  bool r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_0r, miner_acc, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, tx_list);
  
  // empty comment
  currency::extra_alias_entry ai_upd = AUTO_VAL_INIT(ai_upd);
  ai_upd.m_alias = ai.m_alias;
  ai_upd.m_address.spend_public_key = null_pkey;
  ai_upd.m_address.view_public_key = null_pkey;
  ai_upd.m_text_comment = "";
  r = sign_extra_alias_entry(ai_upd, m_alice.get_public_address().spend_public_key, m_alice.get_keys().spend_secret_key);
  CHECK_AND_ASSERT_MES(r, false, "failed to sign update_alias");

  MAKE_BLOCK_WITH_ALIAS_INFO_IN_TX(events, blk_2, blk_1, miner_acc, ai_upd);
  DO_CALLBACK(events, "check_alias_changed");


  // max length alias + comment + view key
  tx_list.clear();
  ai = AUTO_VAL_INIT(ai);
  ai.m_alias = std::string(255, 'x');
  ai.m_text_comment = std::string(255, 'c');
  ai.m_address = m_alice.get_public_address();
  ai.m_view_key.push_back(m_alice.get_keys().view_secret_key);
  r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_0r, miner_acc, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3, blk_2, miner_acc, tx_list);

  tx_list.clear();
  ai.m_text_comment = std::string(255, '\0');
  r = sign_extra_alias_entry(ai, m_alice.get_public_address().spend_public_key, m_alice.get_keys().spend_secret_key);
  CHECK_AND_ASSERT_MES(r, false, "sign_extra_alias_entry failed");
  r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_0r, miner_acc, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_4, blk_3, miner_acc, tx_list);

  return true;
}

bool gen_alias_strange_data::check_alias_changed(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::extra_alias_entry ai;
  bool r = c.get_blockchain_storage().get_alias_info(std::string(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, 'a'), ai);
  CHECK_AND_ASSERT_MES(r, false, "get_alias_info failed");
  CHECK_AND_ASSERT_MES(ai.m_address.spend_public_key == null_pkey && ai.m_address.view_public_key == null_pkey, false, "alias didn't change");

  return true;
}

//------------------------------------------------------------------------------
gen_alias_concurrency_with_switch::gen_alias_concurrency_with_switch()
{
  REGISTER_CALLBACK_METHOD(gen_alias_concurrency_with_switch, check_alias);
}

bool gen_alias_concurrency_with_switch::generate(std::vector<test_event_entry>& events) const
{
  // make really invalid address from random bytes
  // uint8_t invalid_addr[sizeof currency::account_public_address] = { 57, 244, 75, 72, 186, 222, 255, 245, 157, 183, 255, 152, 163, 1, 13, 139, 165, 252, 186, 21, 170, 30, 79, 91, 87, 107, 69, 223, 130, 76, 104, 205, 109, 50, 39, 215, 73, 216, 246, 24, 122, 180, 205, 42, 162, 217, 58, 215, 42, 38, 27, 207, 92, 12, 211, 217, 203, 202, 65, 237, 49, 190, 244, 240 };

  uint64_t ts = 2000000000; // welcome to the future!
  test_core_time::adjust(ts);

  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  set_hard_fork_heights_to_generator(generator);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // two txs with concurrent alias registration: 1st
  std::list<currency::transaction> tx_list;
  currency::extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = std::string(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, 'a');
  ai.m_text_comment = "";
  ai.m_address = miner_acc.get_public_address();
  //ai.m_address = *reinterpret_cast<currency::account_public_address*>(invalid_addr);
  ai.m_view_key.push_back(miner_acc.get_keys().view_secret_key);
  bool r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_0r, miner_acc, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");

  // 2nd
  std::list<currency::transaction> tx_list2;
  ai.m_address.spend_public_key = null_pkey, ai.m_address.view_public_key = null_pkey;
  DO_CALLBACK(events, "mark_invalid_tx"); // because tx with a duplicate alias request (?)
  r = put_alias_via_tx_to_list(m_hardforks, events, tx_list2, blk_0r, miner_acc, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");

  //  0  .. 10    11    12      <- height
  // (0 )..(0r)- (1 )
  //          \- (2 )- (3 )

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, tx_list);

  // split the chain

  // simulate handling a block with that tx: handle tx like going with the block...
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
  events.push_back(tx_list2.front());
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_0r, miner_acc, tx_list2);

  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc); // reorganize
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_acc); // go ahead

  DO_CALLBACK(events, "check_alias");

  return true;
}

bool gen_alias_concurrency_with_switch::check_alias(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::extra_alias_entry ai = AUTO_VAL_INIT(ai);
  bool r = c.get_blockchain_storage().get_alias_info(std::string(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, 'a'), ai);
  CHECK_AND_ASSERT_MES(r, false, "get_alias_info failed");
  CHECK_AND_ASSERT_MES(ai.m_address.spend_public_key == null_pkey && ai.m_address.view_public_key == null_pkey, false, "wrong alias");
  return true;
}


//------------------------------------------------------------------------------

bool gen_alias_same_alias_in_tx_pool::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  test_core_time::adjust(ts);

  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  set_hard_fork_heights_to_generator(generator);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 4);


  // 1. add to the tx pool two txs: 1) register an alias; 2) update it
  std::list<currency::transaction> tx_list;
  currency::extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = std::string(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, 'a');
  ai.m_address = miner_acc.get_public_address();
  ai.m_view_key.push_back(miner_acc.get_keys().view_secret_key);
  bool r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_0r, miner_acc, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");

  ai.m_address.spend_public_key = null_pkey, ai.m_address.view_public_key = null_pkey;
  r = sign_extra_alias_entry(ai, miner_acc.get_public_address().spend_public_key, miner_acc.get_keys().spend_secret_key);
  CHECK_AND_ASSERT_MES(r, false, "sign_extra_alias_entry failed");
  DO_CALLBACK(events, "mark_invalid_tx"); // because tx with a duplicate alias request
  r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_0r, miner_acc, ai, generator);
  tx_list.pop_back(); // 'cause it's invalid
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");

  // add a block to clear the tx pool
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, tx_list);


  // 2. add to the pool two txs: 1) update previously registered alias 2) update it once again
  tx_list.clear();
  ai.m_address.spend_public_key = miner_acc.get_public_address().spend_public_key;
  r = sign_extra_alias_entry(ai, miner_acc.get_public_address().spend_public_key, miner_acc.get_keys().spend_secret_key);
  CHECK_AND_ASSERT_MES(r, false, "sign_extra_alias_entry failed");
  r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_0r, miner_acc, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");

  ai.m_text_comment = "Oops, I forgot a comment!";
  r = sign_extra_alias_entry(ai, miner_acc.get_public_address().spend_public_key, miner_acc.get_keys().spend_secret_key);
  CHECK_AND_ASSERT_MES(r, false, "sign_extra_alias_entry failed");
  DO_CALLBACK(events, "mark_invalid_tx"); // because tx with a duplicate alias request
  r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_0r, miner_acc, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");
  tx_list.pop_back(); // 'cause it's invalid

  // and put it to the block
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1, miner_acc, tx_list);

  return true;
}

//------------------------------------------------------------------------------
gen_alias_switch_and_tx_pool::gen_alias_switch_and_tx_pool()
{
  REGISTER_CALLBACK_METHOD(gen_alias_switch_and_tx_pool, check_alias);
}

bool gen_alias_switch_and_tx_pool::generate(std::vector<test_event_entry>& events) const
{
  // Brief outline:
  // 1) add a tx, registering an alias, got it in a block;
  // 2) add a tx, updating an alias, got it in another block;
  // 3) make a chain split;
  // 4) trigger switching to an alt chain, this should push both txs to the pool;
  // 5) look how two txs referring the same alias will survive in the pool (should survive);
  // 6) to make things a little worse, add a tx registering the same alias in block, which triggers chain switch;
  // 7) after switching the alias should be registered with the tx in p. 6.

  uint64_t ts = test_core_time::get_time();
  test_core_time::adjust(ts);
  GENERATE_ACCOUNT(alice);
  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  set_hard_fork_heights_to_generator(generator);
  DO_CALLBACK(events, "configure_core");
  events.push_back(alice);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  //  0 ...10     11    12    13        <- height
  //              +--- reg an alias in blk_1
  //              |     +--- update the alias in blk_2
  // (0 )- (0r)- (1 )- (2 )
  //          \- (3 )- (4 )- (5 )

  // 1. register an alias
  std::list<currency::transaction> tx_list;
  currency::extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = std::string(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, 'a');
  ai.m_address = miner_acc.get_public_address();
  ai.m_view_key.push_back(miner_acc.get_keys().view_secret_key);
  bool r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_0r, miner_acc, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");

  // add a block to clear the tx pool
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, tx_list);


  // 2. update the alias 
  tx_list.clear();
  ai.m_address.spend_public_key = null_pkey, ai.m_address.view_public_key = null_pkey;
  r = sign_extra_alias_entry(ai, miner_acc.get_public_address().spend_public_key, miner_acc.get_keys().spend_secret_key);
  CHECK_AND_ASSERT_MES(r, false, "sign_extra_alias_entry failed");
  r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_0r, miner_acc, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");

  // add one more block to bring the tx to life (i.e. blockchain)
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1, miner_acc, tx_list);

  // split the blockchain
  MAKE_NEXT_BLOCK(events, blk_3, blk_0r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_acc);


  // make a tx, registering the same alias, simulate tx handling
  tx_list.clear();
  ai = AUTO_VAL_INIT(ai);
  ai.m_alias = std::string(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, 'a');
  ai.m_address = alice.get_public_address();
  ai.m_view_key.push_back(alice.get_keys().view_secret_key);
  DO_CALLBACK(events, "mark_invalid_tx"); // tx is rejected, because tx pool already has tx with reg/upd this alias
  r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_4, miner_acc, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");

  
  // simulate handling a block with that tx: handle tx like going with the block...
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
  events.push_back(tx_list.front()); // now tx should be accepted
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));

  // ... then handle the block itself, it should be accepted
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_5, blk_4, miner_acc, tx_list);

  DO_CALLBACK_PARAMS_STR(events, "check_alias", ai.m_alias);

  return true;
}

bool gen_alias_switch_and_tx_pool::check_alias(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  const std::string& alias_name = boost::get<callback_entry>(events[ev_index]).callback_params;
  const currency::account_base& alice = boost::get<const currency::account_base>(events[2]);
  currency::extra_alias_entry_base ai;
  bool r = c.get_blockchain_storage().get_alias_info(alias_name, ai);
  CHECK_AND_ASSERT_MES(r, false, "get_alias_info failed");
  CHECK_AND_ASSERT_MES(ai.m_address == alice.get_public_address() && !ai.m_view_key.empty() && ai.m_view_key.front() == alice.get_keys().view_secret_key, false, "wrong alias");

  return true;
}


//------------------------------------------------------------------------------
bool gen_alias_update_after_addr_changed::generate(std::vector<test_event_entry>& events) const
{
  // Brief idea: make sure an alias can only update the one, who owns private key to alias' target address.

  uint64_t ts = test_core_time::get_time();

  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // register an alias on behalf of miner_acc with miner's address
  std::list<transaction> tx_list;
  currency::extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = std::string(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, 'a');
  ai.m_address = miner_acc.get_public_address();
  bool r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_0r, miner_acc, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, tx_list);

  // change the address to alice, while signing still by miner
  tx_list.clear();
  ai.m_text_comment = "upd1";
  ai.m_address = alice.get_public_address();
  r = sign_extra_alias_entry(ai, miner_acc.get_public_address().spend_public_key, miner_acc.get_keys().spend_secret_key);
  CHECK_AND_ASSERT_MES(r, false, "sign_extra_alias_entry failed");
  r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_1, miner_acc, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1, miner_acc, tx_list);

  // try to update the alias on behalf of miner (should fail because it must be alice who can sign it)
  tx_list.clear();
  ai.m_text_comment = "upd2";
  r = sign_extra_alias_entry(ai, miner_acc.get_public_address().spend_public_key, miner_acc.get_keys().spend_secret_key);
  CHECK_AND_ASSERT_MES(r, false, "sign_extra_alias_entry failed");
  r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_2, miner_acc, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");
  // the next block will be rejected as invalid, as cantaining tx with invalid alias sign
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2i, blk_2, miner_acc, tx_list);

  // !!!!!!
  // TODO: last tx is invalid, but still in the pool - this is a problem, because nobody can update the alias until this tx will be removed from the pool
  // !!!!!!

  // try to update the alias on behalf of alice (should be ok)
  tx_list.clear();
  ai.m_address = miner_acc.get_public_address();
  r = sign_extra_alias_entry(ai, alice.get_public_address().spend_public_key, alice.get_keys().spend_secret_key);
  CHECK_AND_ASSERT_MES(r, false, "sign_extra_alias_entry failed");
  r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_2, miner_acc, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3, blk_2, miner_acc, tx_list);

  // try to change the alias on behalf on alice again (should fail)
  tx_list.clear();
  ai.m_text_comment = "upd3";
  r = sign_extra_alias_entry(ai, alice.get_public_address().spend_public_key, alice.get_keys().spend_secret_key);
  CHECK_AND_ASSERT_MES(r, false, "sign_extra_alias_entry failed");
  r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_3, miner_acc, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");
  // the next block will be rejected as invalid, as cantaining tx with invalid alias sign
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3i, blk_3, miner_acc, tx_list);

  return true;
}

//------------------------------------------------------------------------------
bool gen_alias_blocking_reg_by_invalid_tx::generate(std::vector<test_event_entry>& events) const
{
  // 1. Attacker makes an invalid tx to update an alias
  // 2. No one can include it into a block
  // 3. No one can register this alias

  std::list<transaction> tx_list;
  uint64_t ts = test_core_time::get_time();
  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice);
  GENERATE_ACCOUNT(attacker);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, ts);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 4);
  uint64_t miner_money = get_outs_money_amount(blk_0r.miner_tx) * 4;
  // alice and attacker get some money
  MAKE_TX_LIST(events, tx_list, miner_acc, alice, miner_money / 2, blk_0r);
  MAKE_TX_LIST(events, tx_list, miner_acc, attacker, get_outs_money_amount(blk_0r.miner_tx), blk_0r);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, tx_list);
  tx_list.clear();

  // Attacker makes invalid tx
  extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = "sweet.name";
  ai.m_address = attacker.get_public_address();
  ai.m_sign.push_back(invalid_signature); // it's an invalid sign, so effectivly it's an update alias request
  std::vector<currency::extra_v> ex(1, ai);
  MAKE_TX_FEE_MIX_ATTR_EXTRA(events, tx_0, attacker, attacker, TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, 0, blk_1, CURRENCY_TO_KEY_OUT_RELAXED, ex, false);

  // No one can't include this tx into a block
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_1i, blk_1, miner_acc, tx_0);

  // Someone tries to correctly register this alias, but failes
  ai = AUTO_VAL_INIT(ai);
  ai.m_alias = "sweet.name";
  ai.m_address = alice.get_public_address();
  bool r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_1, alice, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1, miner_acc, tx_list);
  tx_list.clear();
  
  return true;
}

//------------------------------------------------------------------------------

bool gen_alias_blocking_update_by_invalid_tx::generate(std::vector<test_event_entry>& events) const
{
  // 1. Register an alias.
  // 2. Attacker makes a tx to update this alias with fake sign.
  // 3. Nobody can add a block, including attacker's tx.
  // 4. Alias owner can't update his alias until attacker's tx is in the pool.

  std::list<transaction> tx_list;
  uint64_t ts = test_core_time::get_time();
  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice);
  GENERATE_ACCOUNT(attacker);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, ts);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 4);
  uint64_t miner_money = get_outs_money_amount(blk_0r.miner_tx) * 4;
  // alice and attacker get some money
  MAKE_TX_LIST(events, tx_list, miner_acc, alice, miner_money / 2, blk_0r);
  MAKE_TX_LIST(events, tx_list, miner_acc, attacker, get_outs_money_amount(blk_0r.miner_tx), blk_0r);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, tx_list);
  tx_list.clear();
  //REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // Alice registers an alias for herself
  extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = "sweet.name";
  ai.m_address = alice.get_public_address();
  bool r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_1, alice, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1, miner_acc, tx_list);
  tx_list.clear();

  // Attacker makes fake update alias tx
  ai = AUTO_VAL_INIT(ai);
  ai.m_alias = "sweet.name";
  ai.m_address = attacker.get_public_address();
  ai.m_sign.push_back(invalid_signature);
  r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_2, attacker, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");
  
  // Obviously, miner can't add it to his next block, because it turns out to be invalid.
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2i, blk_2, miner_acc, tx_list);
  tx_list.clear();
  

  // Alice tries to update her alias
  ai = AUTO_VAL_INIT(ai);
  ai.m_alias = "sweet.name";
  ai.m_address = alice.get_public_address();
  ai.m_text_comment = "alice@mail.com";
  r = sign_extra_alias_entry(ai, alice.get_public_address().spend_public_key, alice.get_keys().spend_secret_key);
  CHECK_AND_ASSERT_MES(r, false, "sign_extra_alias_entry failed");
  r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_2, alice, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3, blk_2, miner_acc, tx_list);
  tx_list.clear();


  return true;
}

//------------------------------------------------------------------------------

bool gen_alias_reg_with_locked_money::generate(std::vector<test_event_entry>& events) const
{
  // Make sure alias can't be paid with locked money
  
  std::list<transaction> tx_list;
  uint64_t ts = test_core_time::get_time();
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core");

  currency::block& prev_block = blk_0;

  // the following line unlocks money
  // REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW); prev_block = blk_0r;

  extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = std::string(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, 'a');
  ai.m_address = miner_acc.get_public_address();

  currency::tx_source_entry se = AUTO_VAL_INIT(se);
  se.amount = boost::get<currency::tx_out_bare>(blk_0.miner_tx.vout[0]).amount;
  currency::tx_source_entry::output_entry oe = AUTO_VAL_INIT(oe);
  oe.out_reference = 0;
  oe.stealth_address = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(blk_0.miner_tx.vout[0]).target).key;
  se.outputs.push_back(oe);
  //se.outputs.push_back(make_serializable_pair<txout_ref_v, crypto::public_key>(0, boost::get<currency::txout_to_key>(boost::get<currency::tx_out_bare>(blk_0.miner_tx.vout[0]).target).key));
  se.real_output = 0;
  se.real_output_in_tx_index = 0;
  se.real_out_tx_key = currency::get_tx_pub_key_from_extra(blk_0.miner_tx);
  std::vector<currency::tx_source_entry> sources(1, se);
  account_public_address null_addr = AUTO_VAL_INIT(null_addr);
  std::vector<currency::tx_destination_entry> destinations;
  destinations.push_back(tx_destination_entry(get_alias_coast_from_fee(ai.m_alias, TESTS_DEFAULT_FEE), null_addr));

  tx_builder tb;
  tb.step1_init();
  tb.step2_fill_inputs(miner_acc.get_keys(), sources);
  tb.step3_fill_outputs(destinations);
  tb.m_tx.extra.push_back(ai);
  tb.step4_calc_hash();
  tb.step5_sign(sources);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tb.m_tx);

  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_2, prev_block, miner_acc, tb.m_tx);

  return true;
}

//------------------------------------------------------------------------------
gen_alias_too_much_reward::gen_alias_too_much_reward()
{
  REGISTER_CALLBACK_METHOD(gen_alias_too_much_reward, check_alias);

  test_gentime_settings s = test_generator::get_test_gentime_settings();
  s.miner_tx_max_outs = 11; // limit genesis outs for speed-up, as we going to spend all premine
  test_generator::set_test_gentime_settings(s);
}

bool gen_alias_too_much_reward::generate(std::vector<test_event_entry>& events) const
{
  // pay for alias far too much and see, if it's ok
  // UPDATE: since HF4 it's not ok, the reward must be precise

  uint64_t ts = test_core_time::get_time();

  GENERATE_ACCOUNT(miner_acc);                                                                             // event index
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);                                                        // 0
  set_hard_fork_heights_to_generator(generator);
  DO_CALLBACK(events, "configure_core");                                                                   // 1
  events.push_back(miner_acc);                                                                             // 2
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);         // N+3
  uint64_t premine = get_outs_money_amount(blk_0.miner_tx);

  extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = std::string(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, 'a');
  ai.m_address = miner_acc.get_public_address();
  ai.m_view_key.push_back(miner_acc.get_keys().view_secret_key);
  std::vector<currency::extra_v> extra;
  extra.push_back(ai);

  account_base reward_acc;
  bool r = get_aliases_reward_account(const_cast<currency::account_public_address&>(reward_acc.get_public_address()));
  CHECK_AND_ASSERT_MES(r, false, "get_aliases_reward_account failed");

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc, reward_acc, premine, TESTS_DEFAULT_FEE, 0, sources, destinations);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  for(auto& d : destinations)
    if (d.addr.back() == null_pub_addr)
      d.flags |= tx_destination_entry_flags::tdef_explicit_native_asset_id | tx_destination_entry_flags::tdef_zero_amount_blinding_mask;
  transaction tx_0{};
  crypto::secret_key sk{};
  r = construct_tx(miner_acc.get_keys(), sources, destinations, std::vector<currency::extra_v>({ ai }), empty_attachment, tx_0, get_tx_version_from_events(events), sk, 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  
  if (tx_0.version <= TRANSACTION_VERSION_PRE_HF4)
  {
    ADD_CUSTOM_EVENT(events, tx_0);
    MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);
    DO_CALLBACK(events, "check_alias");
  }
  else
  {
    // post HF4: alias reward must be precise
    DO_CALLBACK(events, "mark_invalid_tx");
    ADD_CUSTOM_EVENT(events, tx_0);
    DO_CALLBACK(events, "mark_invalid_block");
    MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);
  }

  return true;
}

bool gen_alias_too_much_reward::check_alias(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  const currency::account_base& miner_acc = boost::get<const currency::account_base>(events[2]);
  currency::extra_alias_entry_base ai;
  bool r = c.get_blockchain_storage().get_alias_info(std::string(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, 'a'), ai);
  CHECK_AND_ASSERT_MES(r, false, "get_alias_info failed");
  CHECK_AND_ASSERT_MES(ai.m_address == miner_acc.get_public_address() && ai.m_view_key.size() == 1 && ai.m_view_key.front() == miner_acc.get_keys().view_secret_key, false, "wrong alias");

  return true;
}

//------------------------------------------------------------------------------

#define ALIAS_SHORT_NAMES_VALIDATION_PUB_KEY_TESTS "2aa4f4571d454462f4f1d426475235a9449b97dd1e3a7f44aa58a407a475e75c"
#define ALIAS_SHORT_NAMES_VALIDATION_PRIV_KEY_TESTS "44c7a7727a24c3a69e2030c52d366b82934f4d690c07c5206b45edd119dc010d"

struct alias_entry
{
  char name[16];
  account_public_address addr;
};

account_public_address pub_addr_from_string(const char* str)
{
  account_public_address r = AUTO_VAL_INIT(r);
  CHECK_AND_ASSERT_THROW_MES(get_account_address_from_str(r, str), "get_account_address_from_str failed for " << str);
  return r;
}

gen_alias_too_small_reward::gen_alias_too_small_reward()
{
  REGISTER_CALLBACK_METHOD(gen_alias_too_small_reward, init_runtime_config);
  REGISTER_CALLBACK_METHOD(gen_alias_too_small_reward, check_alias);
}

bool gen_alias_too_small_reward::init_runtime_config(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{

  core_runtime_config crc = c.get_blockchain_storage().get_core_runtime_config();
  bool r = epee::string_tools::hex_to_pod(ALIAS_SHORT_NAMES_VALIDATION_PUB_KEY_TESTS, crc.alias_validation_pubkey);
  CHECK_AND_ASSERT_THROW_MES(r, "failed to parse alias_validation_pub_key");
  c.get_blockchain_storage().set_core_runtime_config(crc);
  return true;
}

bool gen_alias_too_small_reward::generate(std::vector<test_event_entry>& events) const
{
  // pay for alias too small and see, if it's ok
  const alias_entry aliases[] = {
    {"a",          pub_addr_from_string("ZxD95C97dFqVBsuTMA1VKjFxx8Cc7by2YTA9kLhFc8JB3zJ3qKKXRm9Lu2YQsjPP5UKhCSfLLEqJr5ovyQNYYQWV1Pv98fRzt")},
    {"bb",         pub_addr_from_string("ZxDL9euTT4C9FUj28QY1RdWnreMHJiWfrPs39rXhrgai8H4pmaFzJ4vUUYRmHhNxToN64H1U5sMnaHuD3S4kVbyY1mKHnERVZ")},
    {"ccc",        pub_addr_from_string("ZxBrpHp3xrjLrMMSyJUg44YmyJVZjetouVFdtqLfxpHUMSxiEyyQ7iKSj4sr6gn7qwXrj6YSw7UjJZLyc1H37QtF2p96c2gAD")},
    {"dddd",       pub_addr_from_string("ZxDtT1pTwt6R2t3eGw9VD6N1heHCKNLKuCFUvqgHpXkAVnPkfai4KDYEjRSV8E42XKN3MJeaHJMaxa9hUmaXLyHm2nQ12aX93")},
    {"eeeee",      pub_addr_from_string("ZxCABdwUJpqHstWJUHQ21piADBwaSsXcAh5EPtpSr8xXderWqvDef566ReFGrRqBUrE2tCgZ3HE5XRuxoq8mNTrP2X4J35yQq")},
    {"ffffff",     pub_addr_from_string("ZxC34uAJJ2iW15GkvcqaQd4RKZdu16tpmf4ubmsirw7eFtKoLi2xswhNqy3Q4VacCq5mM7zuYyoWEW8AS5HGtoXr1m9RuTUuu")},
    {"ggggggg",    pub_addr_from_string("ZxDHxZizSe5MNQoRkC1unqTrhYUkh1ZG7iEXMzLatyZ5EHRPat4Ls4ZRnN4CYLvJLq5F5gxdDtu17Zrvur7dcqU52sv2pryn7")},
    {"hhhhhhhh",   pub_addr_from_string("ZxDXME4qrbh7mAbrqDmCbzGj14VQP1n9KLLi7fXBMNvDd5UUpcevCSXQ9zSkZcJtbzBS7u16NiykAiv3q9VkZySL2ySB6hTfL")},
    {"iiiiiiiii",  pub_addr_from_string("ZxDtpxbC2bN8yu3J49tsUYUSoPYTnAgjmBogzFviUg3t2fGfWzmZ2gbKNC1XKVdMEE2hoW5sULs2hAF5T3igoAVW2MsHUmaj4")},
    {"jjjjjjjjjj", pub_addr_from_string("ZxCBLxnctYwB37YZi7MsJqBCujXzkBeJEh7wPbYrFUvMiqXiPLkyBRAh6ahQ6wre2tGR8FHesZwKn2zYPkTuibyu2648g2CGV")}
  };
  
  const size_t aliases_count = sizeof aliases / sizeof aliases[0];

  uint64_t ts = test_core_time::get_time();
  bool r = false;

  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  set_hard_fork_heights_to_generator(generator);
  DO_CALLBACK(events, "configure_core");
  DO_CALLBACK(events, "init_runtime_config"); 
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW+20);
  
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), miner_acc.get_public_address(), 3 * aliases_count * TESTS_DEFAULT_FEE * 100, 3 * aliases_count, TESTS_DEFAULT_FEE, tx_1);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_1);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  std::vector<tx_source_entry> used_sources;
  std::list<transaction> txs;
  for (size_t i = 0; i < aliases_count; ++i)
  {
    uint64_t alias_reward = get_alias_coast_from_fee(aliases[i].name, ALIAS_VERY_INITAL_COAST);

    transaction tx = AUTO_VAL_INIT(tx);
    DO_CALLBACK(events, "mark_invalid_tx"); // should be rejected, because it's paid ALIAS_VERY_INITAL_COAST / 10
    r = make_tx_reg_alias(events, generator, blk_1, aliases[i].name, aliases[i].addr, ALIAS_VERY_INITAL_COAST / 10, miner_acc, tx, used_sources);
    CHECK_AND_ASSERT_MES(r, false, "make_tx_reg_alias failed, i: " << i);

//     this block commented due to new fee median rules, TODO: review
//     DO_CALLBACK(events, "mark_invalid_tx"); // should be rejected, because it's paid TX_POOL_MINIMUM_FEE / 10 less then required
//     if (!make_tx_reg_alias(events, generator, blk_1, aliases[i].name, aliases[i].addr, alias_reward - TESTS_DEFAULT_FEE / 10, miner_acc, tx, used_sources))
//       return false;

    // should be accepted
    tx = transaction{};
    r = make_tx_reg_alias(events, generator, blk_1, aliases[i].name, aliases[i].addr, alias_reward, miner_acc, tx, used_sources);
    CHECK_AND_ASSERT_MES(r, false, "make_tx_reg_alias failed, i: " << i);

    uint64_t burnt_amount = 0;
    CHECK_AND_ASSERT_MES(check_native_coins_amount_burnt_in_outs(tx, alias_reward, &burnt_amount), false,
      "registration of alias '" << aliases[i].name << "' failed due to incorrect reward; expected reward: " << print_money_brief(alias_reward)
      << "; burnt amount: " << (tx.version <= TRANSACTION_VERSION_PRE_HF4 ? print_money_brief(burnt_amount) : std::string("hidden"))
      << "; tx: " << get_transaction_hash(tx));

    txs.push_back(tx);
  }

  // one alias per block to avoid the limits
  block prev_block = blk_1;
  for(auto& tx : txs)
  {
    MAKE_NEXT_BLOCK_TX1(events, blk_n, prev_block, miner_acc, tx);
    prev_block = blk_n;
  }

  for (size_t i = 0; i < sizeof aliases / sizeof aliases[0]; ++i)
  {
    DO_CALLBACK_PARAMS(events, "check_alias", aliases[i]);
  }

  return true;
}

bool gen_alias_too_small_reward::make_tx_reg_alias(std::vector<test_event_entry>& events, test_generator &generator, const currency::block& prev_block, const std::string& alias,
  const account_public_address& alias_addr, uint64_t alias_reward, const currency::account_base& miner_acc, currency::transaction &tx, std::vector<tx_source_entry>& used_sources) const
{
  extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = alias;
  ai.m_address = alias_addr;
  ai.m_view_key.push_back(miner_acc.get_keys().view_secret_key);


  if (alias.size() < ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED)
  {
    crypto::secret_key alias_secret_key = AUTO_VAL_INIT(alias_secret_key);
    bool r = epee::string_tools::hex_to_pod(ALIAS_SHORT_NAMES_VALIDATION_PRIV_KEY_TESTS, alias_secret_key);
    CHECK_AND_ASSERT_THROW_MES(r, "failed to parse alias_secret_key");

    crypto::public_key alias_public_key = AUTO_VAL_INIT(alias_public_key);
    r = epee::string_tools::hex_to_pod(ALIAS_SHORT_NAMES_VALIDATION_PUB_KEY_TESTS, alias_public_key);
    CHECK_AND_ASSERT_THROW_MES(r, "failed to parse alias_public_key");

    r = currency::sign_extra_alias_entry(ai, alias_public_key, alias_secret_key);
    CHECK_AND_ASSERT_THROW_MES(r, "failed to sign_extra_alias_entry");
  }

  std::vector<currency::extra_v> extra;
  extra.push_back(ai);

  account_base reward_acc;
  bool r = get_aliases_reward_account(const_cast<currency::account_public_address&>(reward_acc.get_public_address()));
  CHECK_AND_ASSERT_MES(r, false, "get_aliases_reward_account failed");

  std::vector<tx_source_entry> sources;
  uint64_t amount = alias_reward + TESTS_DEFAULT_FEE;
  r = fill_tx_sources(sources, events, prev_block, miner_acc.get_keys(), amount, 0, used_sources, /* check for spends: */ true, /* check for unlock time: */ true);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed, requested money: " << print_money_brief(amount));

  std::vector<tx_destination_entry> destinations;
  tx_destination_entry burn_dst(alias_reward, reward_acc.get_public_address());
  burn_dst.flags |= tx_destination_entry_flags::tdef_explicit_native_asset_id | tx_destination_entry_flags::tdef_zero_amount_blinding_mask; // burning outs need to have this flags to facilitate balance check 
  destinations.push_back(burn_dst);
  uint64_t sources_amount = get_sources_total_amount(sources);
  if (sources_amount > alias_reward + TESTS_DEFAULT_FEE)
    destinations.push_back(tx_destination_entry(sources_amount - (alias_reward + TESTS_DEFAULT_FEE), miner_acc.get_public_address())); // change
  
  crypto::secret_key stub = AUTO_VAL_INIT(stub);
  uint64_t tx_version = get_tx_version(get_block_height(prev_block), m_hardforks);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, extra, empty_attachment, tx, tx_version, stub, uint64_t(0));
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  PRINT_EVENT_N_TEXT(events, "make_tx_reg_alias -> construct_tx()");
  events.push_back(tx);

  append_vector_by_another_vector(used_sources, sources);

  return true;
}

bool gen_alias_too_small_reward::check_alias(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  alias_entry ae = AUTO_VAL_INIT(ae);
  bool r = epee::string_tools::hex_to_pod(boost::get<callback_entry>(events[ev_index]).callback_params, ae);
  CHECK_AND_ASSERT_MES(r, false, "hex_to_pod failed: " << boost::get<callback_entry>(events[ev_index]).callback_params);

  const currency::account_base& miner_acc = m_accounts[MINER_ACC_IDX];

  extra_alias_entry ai = AUTO_VAL_INIT(ai);
  r = c.get_blockchain_storage().get_alias_info(ae.name, ai);
  CHECK_AND_ASSERT_MES(r, false, "get_alias_info failed");

  CHECK_AND_ASSERT_MES(ai.m_address == ae.addr && ai.m_view_key.size() == 1 && ai.m_view_key.front() == miner_acc.get_keys().view_secret_key, false, "wrong alias registration: " << ae.name);

  return true;
}


//------------------------------------------------------------------------------

bool gen_alias_tx_no_outs::generate(std::vector<test_event_entry>& events) const
{
  // pay for alias with tx, having no outs
  uint64_t ts = test_core_time::get_time();
  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core");
  events.push_back(miner_acc);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = "abcdefg";
  ai.m_address = miner_acc.get_public_address();

  currency::tx_source_entry se = AUTO_VAL_INIT(se);
  se.amount = boost::get<currency::tx_out_bare>(blk_0.miner_tx.vout[0]).amount;
  se.outputs.push_back(currency::tx_source_entry::output_entry(0, boost::get<currency::txout_to_key>(boost::get<currency::tx_out_bare>(blk_0.miner_tx.vout[0]).target).key));
  se.real_output = 0;
  se.real_output_in_tx_index = 0;
  se.real_out_tx_key = currency::get_tx_pub_key_from_extra(blk_0.miner_tx);
  std::vector<currency::tx_source_entry> sources(1, se);
  account_public_address null_addr = AUTO_VAL_INIT(null_addr);
  std::vector<currency::tx_destination_entry> destinations;

  tx_builder tb;
  tb.step1_init();
  tb.step2_fill_inputs(miner_acc.get_keys(), sources);
  tb.step3_fill_outputs(destinations);
  tb.m_tx.extra.push_back(ai);
  tb.step4_calc_hash();
  tb.step5_sign(sources);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tb.m_tx);

  return true;
}

//------------------------------------------------------------------------------

gen_alias_switch_and_check_block_template::gen_alias_switch_and_check_block_template()
{
  REGISTER_CALLBACK_METHOD(gen_alias_switch_and_check_block_template, add_block_from_template);
}

bool gen_alias_switch_and_check_block_template::generate(std::vector<test_event_entry>& events) const
{
  // 1) reg an alias, put tx into a block;
  // 2) update an alias with tx with a bigger fee, put tx into a block;
  // 3) split the chain by adding few blocks;
  // 4) switch to altchain, so both registration and updating txs go to the pool;
  // 5) create a block from block template and add it.

  //  0 ...10     11    12    13    14                            <- height
  //                    +--- reg an alias in blk_2
  //                    |     +--- update the alias in blk_3
  // (0 )- (0r)- (1 )- (2 )- (3 )
  //                \- (4 )- (5 )- (6 )

  std::list<transaction> tx_list;
  uint64_t ts = test_core_time::get_time();
  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, ts);                                                       // 0
  set_hard_fork_heights_to_generator(generator);
  DO_CALLBACK(events, "configure_core");                                                                     // 1
  events.push_back(alice);                                                                                   // 2
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 4);       // 2N = CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2
  uint64_t miner_amount = get_outs_money_amount(blk_0r.miner_tx, miner_acc.get_keys()) * 4;
  // alice get some money
  MAKE_TX_LIST(events, tx_list, miner_acc, alice, miner_amount / 2, blk_0r);                                 // 2N+3
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1_, blk_0r, miner_acc, tx_list);                                        // 2N+4
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1, blk_1_, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);       // 2N = CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2
  tx_list.clear();

  // Alice registers an alias
  extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = std::string(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, 'x');
  ai.m_address = alice.get_public_address();
  bool r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_1, alice, ai, generator);              // 2N+5
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");                                         
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1, miner_acc, tx_list);                                         // 2N+6
  tx_list.clear();

  // Alice updates her alias (paid by miner in order not to confuse and mix the inputs)
  ai.m_text_comment = "alice@mail.com";
  r = sign_extra_alias_entry(ai, alice.get_public_address().spend_public_key, alice.get_keys().spend_secret_key);
  CHECK_AND_ASSERT_MES(r, false, "sign_extra_alias_entry failed");
  std::vector<currency::extra_v> extra(1, ai);

  MAKE_TX_FEE_MIX_ATTR_EXTRA(events, tx_0, miner_acc, miner_acc, 1, TESTS_DEFAULT_FEE, 0, blk_2, CURRENCY_TO_KEY_OUT_RELAXED, extra, true); // 2N+7
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_0);                                                // 2N+8

  // split the chain
  MAKE_NEXT_BLOCK(events, blk_4, blk_1, miner_acc);                                                          // 2N+9
  MAKE_NEXT_BLOCK(events, blk_5, blk_4, miner_acc);                                                          // 2N+10

  // switch to altchain
  MAKE_NEXT_BLOCK(events, blk_6, blk_5, miner_acc);                                                          // 2N+11

  // try to create and add block from template
  DO_CALLBACK_PARAMS_STR(events, "add_block_from_template", ai.m_alias);                                     // 2N+12

  return true;
}

bool gen_alias_switch_and_check_block_template::add_block_from_template(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  account_base acc;
  acc.generate();
  currency::block b;
  wide_difficulty_type diff;
  uint64_t height;
  blobdata extra = AUTO_VAL_INIT(extra);
  bool r = c.get_block_template(b, acc.get_public_address(), acc.get_public_address(), diff, height, extra);
  CHECK_AND_ASSERT_MES(r, false, "get_block_template failed");

  r = miner::find_nonce_for_given_block(b, diff, height);
  CHECK_AND_ASSERT_MES(r, false, "find_nonce_for_given_block failed");

  currency::block_verification_context bvc = AUTO_VAL_INIT(bvc);
  c.handle_incoming_block(t_serializable_object_to_blob(b), bvc);
  r = check_block_verification_context(bvc, ev_index, b);
  CHECK_AND_NO_ASSERT_MES(r, false, "block verification context check failed");

  // check that alias was successfully registered
  const std::string& alias_name = boost::get<callback_entry>(events[ev_index]).callback_params;
  const currency::account_base& alice_acc = boost::get<const currency::account_base>(events[2]);

  extra_alias_entry ai = AUTO_VAL_INIT(ai);
  r = c.get_blockchain_storage().get_alias_info(alias_name, ai);
  CHECK_AND_ASSERT_MES(r, false, "get_alias_info failed");
  CHECK_AND_ASSERT_MES(ai.m_address == alice_acc.get_public_address(), false, "wrong alias registration: " << alias_name);

  return true;
}

//------------------------------------------------------------------------------

gen_alias_too_many_regs_in_block_template::gen_alias_too_many_regs_in_block_template()
  : m_estimated_alias_cost(0)
  , m_total_alias_to_gen(MAX_ALIAS_PER_BLOCK + 1)
{
  REGISTER_CALLBACK_METHOD(gen_alias_too_many_regs_in_block_template, add_block_from_template);
}

bool gen_alias_too_many_regs_in_block_template::generate(std::vector<test_event_entry>& events) const
{
  // 1) add lots of alias-registering txs to the pool;
  // 2) create block template and check it contains not too many such txs.

  bool r = false;
  uint64_t ts = test_core_time::get_time();
  GENERATE_ACCOUNT(preminer_acc);
  m_accounts.resize(TOTAL_ACCS_COUNT);
  GENERATE_ACCOUNT(miner_acc); m_accounts[MINER_ACC_IDX] = miner_acc;
  GENERATE_ACCOUNT(alice_acc); m_accounts[ALICE_ACC_IDX] = alice_acc;

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, ts);                                                          // 0
  set_hard_fork_heights_to_generator(generator);
  DO_CALLBACK(events, "configure_core");                                                                        // 1
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  uint64_t fee_median = generator.get_last_n_blocks_fee_median(get_block_hash(blk_0r));
  m_estimated_alias_cost = currency::get_alias_coast_from_fee(std::string(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, 'a'), fee_median);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  uint64_t total_alias_cost = 0;
  for(size_t i = 0; i < m_total_alias_to_gen; ++i)
  {
    destinations.push_back(tx_destination_entry(m_estimated_alias_cost + TESTS_DEFAULT_FEE, alice_acc.get_public_address()));
    total_alias_cost += m_estimated_alias_cost + TESTS_DEFAULT_FEE;
  }
  total_alias_cost += TESTS_DEFAULT_FEE;
  r = fill_tx_sources(sources, events, blk_0r, preminer_acc.get_keys(), total_alias_cost, 0);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  uint64_t sources_amount = get_sources_total_amount(sources);
  if (sources_amount > total_alias_cost)
    destinations.push_back(tx_destination_entry(sources_amount - total_alias_cost, preminer_acc.get_public_address())); // return the change in order to keep median_fee low

  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  uint64_t tx_version = get_tx_version(get_block_height(blk_0r), m_hardforks);
  r = construct_tx(preminer_acc.get_keys(), sources, destinations, empty_attachment, tx_1, tx_version, 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

  events.push_back(tx_1);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE); // rewind few block to made money unlocked in Alice's wallet

  DO_CALLBACK(events, "add_block_from_template");

  return true;
}

bool gen_alias_too_many_regs_in_block_template::add_block_from_template(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // register too many aliases
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();
  
  extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_address = m_accounts[ALICE_ACC_IDX].get_public_address();

  account_base someone;

  try
  {
    log_level_scope_changer llsc(LOG_LEVEL_0); // eliminate all the mess in the log during alias registration, comment this out if case of debugging

    for (size_t i = 0; i < m_total_alias_to_gen; ++i)
    {
      LOG_PRINT_YELLOW("Generating alias #" << i << " of " << m_total_alias_to_gen << "...", LOG_LEVEL_0);
      someone.generate();
      ai.m_address = someone.get_public_address();
      ai.m_alias = gen_random_alias(random_in_range(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED + 20));
      transaction alias_reg_tx = AUTO_VAL_INIT(alias_reg_tx);
      alice_wlt->request_alias_registration(ai, alias_reg_tx, TESTS_DEFAULT_FEE, m_estimated_alias_cost);
    }
  }
  catch (std::exception& e)
  {
    LOG_ERROR("Caught an exception while registering an alias: " << e.what());
    return false;
  }

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == m_total_alias_to_gen, false, "Unexpected number of txs in the pool: " << c.get_pool_transactions_count() << ", expected: " << m_total_alias_to_gen);

  // attempt to create block from block template, then check how many aliases it will contain
  account_base acc;
  acc.generate();
  currency::block b;
  wide_difficulty_type diff;
  uint64_t height;
  blobdata extra = AUTO_VAL_INIT(extra);
  bool r = c.get_block_template(b, acc.get_public_address(), acc.get_public_address(), diff, height, extra);
  CHECK_AND_ASSERT_MES(r, false, "get_block_template failed");

  r = miner::find_nonce_for_given_block(b, diff, height);
  CHECK_AND_ASSERT_MES(r, false, "find_nonce_for_given_block failed");

  currency::block_verification_context bvc = AUTO_VAL_INIT(bvc);
  c.handle_incoming_block(t_serializable_object_to_blob(b), bvc);
  r = check_block_verification_context(bvc, ev_index, b);
  CHECK_AND_NO_ASSERT_MES(r, false, "block verification context check failed");

  return true;
}
//------------------------------------------------------------------------------

bool gen_alias_update_for_free::generate(std::vector<test_event_entry>& events) const
{
  std::list<transaction> tx_list;
  uint64_t ts = test_core_time::get_time();
  GENERATE_ACCOUNT(miner_acc);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);                                                          // 0
  set_hard_fork_heights_to_generator(generator);
  DO_CALLBACK(events, "configure_core");                                                                     // 1
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);           // 2N+1, 2N = CURRENCY_MINED_MONEY_UNLOCK_WINDOW

  // registrate an alias - pay as usual
  extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = std::string(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, 'a');
  ai.m_address = miner_acc.get_public_address();
  bool r = put_alias_via_tx_to_list(m_hardforks, events, tx_list, blk_0r, miner_acc, ai, generator);
  CHECK_AND_ASSERT_MES(r, false, "put_alias_via_tx_to_list failed");
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, tx_list);
  tx_list.clear();

  // update the alias - pay nothing
  ai.m_text_comment = "miner@mail.com";
  r = sign_extra_alias_entry(ai, miner_acc.get_public_address().spend_public_key, miner_acc.get_keys().spend_secret_key);
  CHECK_AND_ASSERT_MES(r, false, "sign_extra_alias_entry failed");
  std::vector<currency::extra_v> extra(1, ai);

  // create a tx with chargeback, paying only the minimum required fee
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  r = fill_tx_sources(sources, events, blk_0r, miner_acc.get_keys(), TESTS_DEFAULT_FEE, 0, true, true);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  uint64_t input_amount = 0;
  for (auto se : sources)
    input_amount += se.amount;
  if (input_amount > TESTS_DEFAULT_FEE)
  {
    uint64_t d = input_amount - TESTS_DEFAULT_FEE;
    destinations.push_back(tx_destination_entry(d / 2, miner_acc.get_public_address()));
    d -= d / 2;
    destinations.push_back(tx_destination_entry(d, miner_acc.get_public_address()));
  }

  transaction tx{};
  uint64_t tx_version = currency::get_tx_version(get_block_height(blk_0r) + 1, generator.get_hardforks());
  crypto::secret_key sk{};
  r = construct_tx(miner_acc.get_keys(), sources, destinations, std::vector<extra_v>({ ai }), empty_attachment, tx, tx_version, sk, 0);

  /*tx_builder tb;
  tb.step1_init();
  tb.step2_fill_inputs(miner_acc.get_keys(), sources);
  tb.step3_fill_outputs(destinations);
  tb.m_tx.extra.push_back(ai);
  tb.step4_calc_hash();
  tb.step5_sign(sources);*/

  ADD_CUSTOM_EVENT(events, tx);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, tx);

  return true;
}

//------------------------------------------------------------------------------

gen_alias_in_coinbase::gen_alias_in_coinbase()
{
  REGISTER_CALLBACK_METHOD(gen_alias_in_coinbase, check);
}

bool gen_alias_in_coinbase::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure that alias can be registered and update via coinbase transaction as well as normal one.
  // This also checks that coinbase transacton can hold an arbitrary entity in it's extra.

  uint64_t ts = 145000000;
  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  set_hard_fork_heights_to_generator(generator);
  DO_CALLBACK(events, "configure_core");

  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_acc);

  // reg an alias using coinbase

  extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = "emmanuel.goldstein"; // long enough alias to minimize it's cost
  ai.m_address = miner_acc.get_public_address();
  block blk_2 = AUTO_VAL_INIT(blk_2);
  bool r = generator.construct_pow_block_with_alias_info_in_coinbase(miner_acc, blk_1, ai, blk_2);
  CHECK_AND_ASSERT_MES(r, false, "construct_block_gentime_with_coinbase_cb failed");

  events.push_back(blk_2);

  DO_CALLBACK_PARAMS_STR(events, "check", t_serializable_object_to_blob(ai));


  // update an alias using coinbase

  ai.m_text_comment = "A Party member is expected to have no private emotions and no respites from enthusiasm.";
  ai.m_view_key.push_back(miner_acc.get_keys().view_secret_key);
  r = sign_extra_alias_entry(ai, miner_acc.get_public_address().spend_public_key, miner_acc.get_keys().spend_secret_key);
  CHECK_AND_ASSERT_MES(r, false, "sign_extra_alias_entry failed");

  block blk_3 = AUTO_VAL_INIT(blk_3);
  r = generator.construct_pow_block_with_alias_info_in_coinbase(miner_acc, blk_2, ai, blk_3);
  CHECK_AND_ASSERT_MES(r, false, "construct_block_gentime_with_coinbase_cb failed");

  events.push_back(blk_3);

  DO_CALLBACK_PARAMS_STR(events, "check", t_serializable_object_to_blob(ai));

  return true;
}

bool gen_alias_in_coinbase::check(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  extra_alias_entry param_ai = AUTO_VAL_INIT(param_ai);
  bool r = t_unserializable_object_from_blob(param_ai, boost::get<callback_entry>(events[ev_index]).callback_params);
  CHECK_AND_ASSERT_MES(r, false, "Can't obtain event params.");

  currency::extra_alias_entry_base ai = AUTO_VAL_INIT(ai);
  r = c.get_blockchain_storage().get_alias_info(param_ai.m_alias, ai);
  CHECK_AND_ASSERT_MES(r, false, "blockchain has no info for alias: " << param_ai.m_alias);

  CHECK_AND_ASSERT_MES(ai.m_address      == param_ai.m_address,      false, "m_address are wrong");
  CHECK_AND_ASSERT_MES(ai.m_sign         == param_ai.m_sign,         false, "m_sign are wrong");
  CHECK_AND_ASSERT_MES(ai.m_text_comment == param_ai.m_text_comment, false, "m_text_comment are wrong");
  CHECK_AND_ASSERT_MES(ai.m_view_key     == param_ai.m_view_key,     false, "m_view_key are wrong");

  return true;
}

  
  