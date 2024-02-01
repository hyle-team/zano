// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


// !!!
// IMPORTANT NOTE: This file is UTF-8 encoded with BOM, because of using non-ascii literals in check_u8_str_funcs()
// !!!


#include "chaingen.h" // pch
#include "misc_tests.h"
#include "random_helper.h"

using namespace currency;

bool check_allowed_types_in_variant_container_test()
{
  struct A {};
  struct B {};
  struct C {};
  struct D {};

  A a;
  B b;
  C c;
  D d;

  typedef boost::variant<A, B, C, D> var_t;
  std::vector<var_t> allowed_types_examples;
  allowed_types_examples.push_back(A());
  allowed_types_examples.push_back(B());
  allowed_types_examples.push_back(C());

  CHECK_EQ(currency::check_allowed_types_in_variant_container(std::vector<var_t>(), std::vector<var_t>(), true), true);
  CHECK_EQ(currency::check_allowed_types_in_variant_container(std::vector<var_t>(), allowed_types_examples, true), true);
  CHECK_EQ(currency::check_allowed_types_in_variant_container(std::vector<var_t>({ d }), std::vector<var_t>(), true), false);
  CHECK_EQ(currency::check_allowed_types_in_variant_container(std::vector<var_t>({ b, a, c }), allowed_types_examples, true), true);
  CHECK_EQ(currency::check_allowed_types_in_variant_container(std::vector<var_t>({ b, a, d, c }), allowed_types_examples, true), false);
  CHECK_EQ(currency::check_allowed_types_in_variant_container(std::vector<var_t>({ b, a, c, a }), allowed_types_examples, true), false);
  CHECK_EQ(currency::check_allowed_types_in_variant_container(std::vector<var_t>({ b, a, c, a }), allowed_types_examples, false), true);
  CHECK_EQ(currency::check_allowed_types_in_variant_container(std::vector<var_t>({ c, c, a, a }), allowed_types_examples, true), false);
  CHECK_EQ(currency::check_allowed_types_in_variant_container(std::vector<var_t>({ c, c, a, a }), allowed_types_examples, false), true);
  CHECK_EQ(currency::check_allowed_types_in_variant_container(std::vector<var_t>({ c, c, a, a, d }), allowed_types_examples, false), false);

 
  std::unordered_set<std::type_index> allowed_types({ std::type_index(typeid(C)), std::type_index(typeid(A)), std::type_index(typeid(B)) });

  CHECK_EQ(currency::check_allowed_types_in_variant_container(std::vector<var_t>(), allowed_types, true), true);
  CHECK_EQ(currency::check_allowed_types_in_variant_container(std::vector<var_t>({ b, a, c }), allowed_types, true), true);
  CHECK_EQ(currency::check_allowed_types_in_variant_container(std::vector<var_t>({ b, a, d, c }), allowed_types, true), false);
  CHECK_EQ(currency::check_allowed_types_in_variant_container(std::vector<var_t>({ b, a, c, a }), allowed_types, true), false);
  CHECK_EQ(currency::check_allowed_types_in_variant_container(std::vector<var_t>({ b, a, c, a }), allowed_types, false), true);
  CHECK_EQ(currency::check_allowed_types_in_variant_container(std::vector<var_t>({ c, c, a, a }), allowed_types, true), false);
  CHECK_EQ(currency::check_allowed_types_in_variant_container(std::vector<var_t>({ c, c, a, a }), allowed_types, false), true);
  CHECK_EQ(currency::check_allowed_types_in_variant_container(std::vector<var_t>({ c, c, a, a, d }), allowed_types, false), false);

  return true;
}

#if ( defined(_MSC_VER) && (_MSC_VER < 1800) )
  #error MS compilers prior to v 18.00 (MSVC 2013) are not supported
#endif

// compilation workaround for MSVC 2013 Update 5 -- the code works fine in MSVC 2015 Update 2-3
#if ( defined(_MSC_VER) && (1800 <= _MSC_VER) && (_MSC_VER < 1900) )
  #pragma execution_character_set("utf-8") // undocumented pragma making all char strings to be saved as UTF-8
  #define u8 // this to workaround C++11 UTF-8 string literals surprisingly not supported in MSVC 2013
#endif

bool check_u8_str_case_funcs()
{
  struct
  {
    const char* lower_case;
    const char* upper_case;
  } data[] = {
    {  "abcdefghijklmnopqrstuvwxyz",                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"},              // ASCII latin chars
    {u8"áéíóúýàèìòùâêîôûãñõäëïöüÿçš",             u8"ÁÉÍÓÚÝÀÈÌÒÙÂÊÎÔÛÃÑÕÄËÏÖÜŸÇŠ"},             // UTF-8 encoded latin chars with diacritics
    //{u8"grüßen",                                u8"GRÜSSEN"},                                 // non-trivial context-specific example in German -- NOT SUPPORTED
    //{u8"Ὀδυσσεύς",                              u8"ὈΔΥΣΣΕΎΣ"},                                // non-trivial context-specific example in Greek -- NOT SUPPORTED
    {u8"абвгдежзиклмнопрстуфхцчшщьюяёйъэыіўїґєѣ", u8"АБВГДЕЖЗИКЛМНОПРСТУФХЦЧШЩЬЮЯЁЙЪЭЫІЎЇҐЄѢ"}, // UTF-8 encoded cyrillic chars
  };

  for (size_t i = 0; i < sizeof data / sizeof data[0]; ++i)
  {
    //LOG_PRINT2("u8_str_funcs.txt", ENDL << data[i].lower_case << ENDL << utf8_to_lower(data[i].upper_case) << ENDL << data[i].upper_case << ENDL << utf8_to_upper(data[i].lower_case) << ENDL << ENDL, LOG_LEVEL_0);
    //continue;

    std::string l2u = currency::utf8_to_upper(data[i].lower_case);
    CHECK_AND_ASSERT_MES(l2u == data[i].upper_case, false, "Invalid l->U conversion for data set #" << i << ": " << ENDL <<
      "got bytes:      " << epee::string_tools::buff_to_hex(l2u) << ENDL <<
      "expected bytes: " << epee::string_tools::buff_to_hex(std::string(data[i].upper_case)));
    std::string u2l = currency::utf8_to_lower(data[i].upper_case);
    CHECK_AND_ASSERT_MES(u2l == data[i].lower_case, false, "Invalid U->l conversion for data set #" << i << ": " << ENDL <<
      "got bytes:      " << epee::string_tools::buff_to_hex(u2l) << ENDL <<
      "expected bytes: " << epee::string_tools::buff_to_hex(std::string(data[i].lower_case)));
  }

  return true;
}

bool chec_u8_str_matching()
{
  struct
  {
    const char* match;
    const char* string;
    bool expected_result;
  } data[] = {
    {  "",      "",             true},
    {  "",      "982734",       true},
    {  "abc",   "",               false},
    {  "abc",   "ababdbDCAC",     false},
    {  "abC",   "abABcdaC",     true},
    {  "aBc",   "abABBdaBc",    true},
    {u8"Ábç", u8"ÁáBÇÁ",        true},
    {u8"ÁBç", u8"ÁáBáÇÁ",         false},
    {u8"Абц", u8"абацбЦ",         false},
    {u8"АбЦ", u8"АаБбЦцаБц",    true},
  };

  for (size_t i = 0; i < sizeof data / sizeof data[0]; ++i)
  {
    bool r = currency::utf8_substring_test_case_insensitive(data[i].match, data[i].string);
    CHECK_AND_ASSERT_MES(r == data[i].expected_result, false, "utf8_substring_test_case_insensitive failed with (" << data[i].match << ", " << data[i].string << "): got " << r << ", expected: " << data[i].expected_result);
  }
  return true;
}

/*
bool check_add_padding_to_tx()
{
  // tiny test for add_padding_to_tx() correctness - temporary disabled
  std::vector<currency::extra_v> extra { currency::extra_padding() };

  currency::block b = AUTO_VAL_INIT(b);
  bool r = currency::generate_genesis_block(b); // just for miner_tx example
  CHECK_AND_ASSERT_MES(r, false, "generate_genesis_block failed");
  currency::transaction& tx_tmp = b.miner_tx;
  size_t tx_blob_size = get_object_blobsize(tx_tmp);
  for (size_t padding = 1; tx_blob_size < CURRENCY_MAX_TRANSACTION_BLOB_SIZE; ++padding)
  {
    tx_blob_size += padding;
    bool r = add_padding_to_tx(tx_tmp, padding);
    CHECK_AND_ASSERT_MES(r, false, "add_padding_to_tx failed, padding = " << padding);
    size_t current_tx_blob_size = get_object_blobsize(tx_tmp);
    CHECK_AND_ASSERT_MES(current_tx_blob_size == tx_blob_size, false, "blobsize missmatch: got: " << current_tx_blob_size << ", expected: " << tx_blob_size);
  }
  return true;
}

*/


//------------------------------------------------------------------------------

bool check_hash_and_difficulty_monte_carlo_test()
{
  // Idea: for the given difficulty D do N trials of check_hash() for pseudo-random data and calculate number of positive hits H.
  // Compare estimated difficulty Dest = N / H with the given one.

  struct hash_source
  {
    uint64_t index;
    char padding[24];
  } hs = {0, {0} };
  size_t hs_size = sizeof(hs);

  size_t trial_count = 33000;
  currency::wide_difficulty_type difficulty_set[] = {1, 3, 5, 10, 30, 55, 71, 95, 100, 160, 161, 300, 350, 500, 513, 677, 710, 850, 970, 1000, 1111, 1200, 1500, 1900, 3000, 5000, 1000000, 10000000};
  
  LOG_PRINT_CYAN("Difficulty test, trial_count = " << trial_count, LOG_LEVEL_0);
  size_t diff_set_size = sizeof difficulty_set / sizeof difficulty_set[0];
  for(size_t i = 0; i < diff_set_size; ++i)
  {
    currency::wide_difficulty_type difficulty = difficulty_set[i];

    std::string t = get_random_text(23);
    memcpy(hs.padding, t.c_str(), sizeof(hs.padding));

    size_t hits_count = 0;
    for(hs.index = 0; hs.index < trial_count; ++hs.index)
    {
      crypto::hash h = crypto::cn_fast_hash(&hs, hs_size);
      if (currency::check_hash(h, difficulty))
        ++hits_count;
    }
    double estimated_difficulty = hits_count > 0 ? (double)trial_count / hits_count : 0;
    double rel_err = 100.0 * (estimated_difficulty - (double)difficulty) / (double)difficulty;
    size_t bar_width_max = 60;
    size_t bar_width = bar_width_max * std::abs(rel_err) / 100.0;
    std::string bar = std::string(rel_err > 0 ? bar_width_max : bar_width_max - bar_width, ' ') + std::string(bar_width, (rel_err > 0 ? '+' : '-'));
    
    LOG_PRINT_CYAN(std::setw(2) << i << "/" << std::setw(2) << diff_set_size <<
      " difficulty: " << std::setw(10) << difficulty << ", " << std::setw(10) << hits_count << " hits out of " << trial_count << " trials, estimated difficulty: "
      << std::setw(12) << std::setprecision(2) << estimated_difficulty << " " << std::setw(6) << std::setprecision(3) << rel_err << "% of real: " << bar.c_str(), LOG_LEVEL_0);
  }

  return true;
}

//------------------------------------------------------------------------------

block_template_vs_invalid_txs_from_pool::block_template_vs_invalid_txs_from_pool()
{
  REGISTER_CALLBACK_METHOD(block_template_vs_invalid_txs_from_pool, check_block_template);
}

bool block_template_vs_invalid_txs_from_pool::generate(std::vector<test_event_entry>& events) const
{
  // Test idea:
  // 1) create a case when tx pool contains invalid tx (which inputs are already spend in the blockchain)
  // 2) make sure such tx won't get into block template

  bool r = false;

  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  GENERATE_ACCOUNT(bob_acc);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  uint64_t test_amount_alice = MK_TEST_COINS(100);
  uint64_t test_amount_bob = MK_TEST_COINS(20);
  
  MAKE_TX(events, tx_1, miner_acc, alice_acc, test_amount_alice + TESTS_DEFAULT_FEE, blk_0r);

  // tx_1m, txin_to_key -> txout_multisig (miner_acc -> bob_acc + miner_acc)
  tx_destination_entry de = AUTO_VAL_INIT(de);
  de.amount = test_amount_bob + TESTS_DEFAULT_FEE;
  de.addr.push_back(bob_acc.get_public_address());
  de.addr.push_back(miner_acc.get_public_address());
  de.minimum_sigs = 1;
  std::vector<tx_source_entry> sources;
  r = fill_tx_sources(sources, events, blk_0r, miner_acc.get_keys(), de.amount + TESTS_DEFAULT_FEE, 0);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  transaction tx_1m = AUTO_VAL_INIT(tx_1m);
  r = construct_tx(miner_acc.get_keys(), sources, std::vector<tx_destination_entry>({ de }), empty_attachment, tx_1m, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_1m);

  // blk_1 contains tx_1 and tx_1m
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({ tx_1, tx_1m }));
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);

  // make sure Alice got the money
  CREATE_TEST_WALLET(alice_wlt, alice_acc, blk_0);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_2, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, test_amount_alice + TESTS_DEFAULT_FEE);

  // make sure Bob got the money (from ms output)
  //CREATE_TEST_WALLET(bob_wlt, bob_acc, blk_0);
  //REFRESH_TEST_WALLET_AT_GEN_TIME(events, bob_wlt, blk_2, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2);
  //CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(bob_wlt, test_amount_bob + TESTS_DEFAULT_FEE);

  // tx_2 and tx_2a spend the same outputs of tx_1
  MAKE_TX(events, tx_2a, alice_acc, miner_acc, test_amount_alice, blk_2);
  events.pop_back(); // pop_back tx_2a
  MAKE_TX(events, tx_2, alice_acc, miner_acc, test_amount_alice, blk_2);

  // tx_2m and tx_2ma spend the same ms output of tx_1m
  tx_source_entry se = AUTO_VAL_INIT(se);
  se.amount = de.amount;
  se.multisig_id = get_multisig_out_id(tx_1m, 0);
  se.real_output_in_tx_index = 0;
  se.real_out_tx_key = get_tx_pub_key_from_extra(tx_1m);
  se.ms_keys_count = boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(tx_1m.vout[se.real_output_in_tx_index]).target).keys.size();
  se.ms_sigs_count = 1;
  transaction tx_2m = AUTO_VAL_INIT(tx_2m);
  r = construct_tx(bob_acc.get_keys(), std::vector<tx_source_entry>({ se }), std::vector<tx_destination_entry>({ tx_destination_entry(se.amount - TESTS_DEFAULT_FEE, miner_acc.get_public_address()) }),
    empty_attachment, tx_2m, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  bool input_fully_signed = false;
  r = sign_multisig_input_in_tx(tx_2m, 0, bob_acc.get_keys(), tx_1m, &input_fully_signed);
  CHECK_AND_ASSERT_MES(r && input_fully_signed, false, "sign_multisig_input_in_tx failed, input_fully_signed=" << input_fully_signed);
  events.push_back(tx_2m);

  transaction tx_2ma = AUTO_VAL_INIT(tx_2ma);
  r = construct_tx(bob_acc.get_keys(), std::vector<tx_source_entry>({ se }), std::vector<tx_destination_entry>({ tx_destination_entry(se.amount - TESTS_DEFAULT_FEE, miner_acc.get_public_address()) }),
    empty_attachment, tx_2ma, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  input_fully_signed = false;
  r = sign_multisig_input_in_tx(tx_2ma, 0, bob_acc.get_keys(), tx_1m, &input_fully_signed);
  CHECK_AND_ASSERT_MES(r && input_fully_signed, false, "sign_multisig_input_in_tx failed, input_fully_signed=" << input_fully_signed);
  // don't add tx_2ma to events

  // create block blk_3 containing tx_2 and tx_2m
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3, blk_2, miner_acc, std::list<transaction>({ tx_2, tx_2m }));

  //  0      10     11     12     13                           <- blockchain height (assuming CURRENCY_MINED_MONEY_UNLOCK_WINDOW == 10)
  // (0 ).. (0r)-  (1 )-  (2 )-  (3 )-                         <- main chain
  //               tx_1          tx_2                          <- txs
  //               tx_1m         tx_2m

  // make sure Alice spend all the money with tx_2
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_3, 1);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, 0);

  DO_CALLBACK(events, "print_tx_pool");
  
  // switch to altchain, tx_2 is popped back to tx_pool
  MAKE_NEXT_BLOCK(events, blk_3a, blk_2, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_4a, blk_3a, miner_acc);

  DO_CALLBACK(events, "print_tx_pool");

  //  0      10     11     12     13     14                    <- blockchain height (assuming CURRENCY_MINED_MONEY_UNLOCK_WINDOW == 10)
  // (0 ).. (0r)-  (1 )-  (2 )-  (3 )-                         <- alt chain
  //               tx_1          tx_2                          <- txs
  //               tx_1m         tx_2m
  //                         \-  (3a)-  (4a)-                  <- main chain
  // tx pool: tx_2, tx_2m

  // simulate behaviour of handle_notify_new_block -- add tx with kept_by_block == true, then add block itself
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
  events.push_back(tx_2a);
  events.push_back(tx_2ma);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_5a, blk_4a, miner_acc, std::list<transaction>({ tx_2a, tx_2ma }));
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));

  //  0      10     11     12     13     14     15             <- blockchain height (assuming CURRENCY_MINED_MONEY_UNLOCK_WINDOW == 10)
  // (0 ).. (0r)-  (1 )-  (2 )-  (3 )-                         <- alt chain
  //               tx_1          tx_2                          <- txs
  //               tx_1m         tx_2m
  //                         \-  (3a)-  (4a)-  (5a)-           <- main chain
  //                                           tx_2a
  //                                           tx_2ma
  // tx pool: tx_2, tx_2m

  DO_CALLBACK(events, "print_tx_pool");

  // now tx_2, tx_2m are in tx_pool and their inputs are already spent in blockchain (block blk_5a)
  // thus, they're invalid transactions and can't be added to the blockchain, unless chain switching happens

  // make sure invalid txs won't get into block template
  DO_CALLBACK(events, "check_block_template");

  return true;
}

bool block_template_vs_invalid_txs_from_pool::check_block_template(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::account_public_address addr = AUTO_VAL_INIT(addr);

  bool r = false;
  currency::block b = AUTO_VAL_INIT(b);
  r = mine_next_pow_block_in_playtime(addr, c, &b);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  return true;
}

//------------------------------------------------------------------------------

test_blockchain_vs_spent_keyimges::test_blockchain_vs_spent_keyimges()
{
  REGISTER_CALLBACK_METHOD(test_blockchain_vs_spent_keyimges, check);
}

bool test_blockchain_vs_spent_keyimges::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure blockchain_storage::have_tx_keyimges_as_spent() and blockchain_storage::have_tx_keyimg_as_spent()
  // work correctly for key images, known to be spent

  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  uint64_t test_amount = MK_TEST_COINS(70);
  
  MAKE_TX(events, tx_1, miner_acc, alice_acc, test_amount, blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);

  // make sure Alice got the money
  CREATE_TEST_WALLET(alice_wlt, alice_acc, blk_0);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_2, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, test_amount);

  m_tx_1 = tx_1;
  DO_CALLBACK(events, "check");

  return true;
}

bool test_blockchain_vs_spent_keyimges::check(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // check have_tx_keyimges_as_spent()
  currency::blockchain_storage &bcs = c.get_blockchain_storage();
  bool spent = bcs.have_tx_keyimges_as_spent(m_tx_1);
  CHECK_AND_ASSERT_MES(spent, false, "have_tx_keyimges_as_spent returned false for spent key images!");

  // check have_tx_keyimg_as_spent()
  for (const currency::txin_v& in : m_tx_1.vin)
  {
    if (in.type() == typeid(currency::txin_to_key))
    {
      spent = bcs.have_tx_keyimg_as_spent(boost::get<currency::txin_to_key>(in).k_image);
      CHECK_AND_ASSERT_MES(spent, false, "have_tx_keyimg_as_spent returned false for spent key image!");
    }
  }

  return true;
}

//------------------------------------------------------------------------------

test_blockchain_vs_spent_multisig_outs::test_blockchain_vs_spent_multisig_outs()
{
  REGISTER_CALLBACK_METHOD(test_blockchain_vs_spent_multisig_outs, check);
}

bool test_blockchain_vs_spent_multisig_outs::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure blockchain_storage::have_tx_keyimges_as_spent() and blockchain_storage::is_multisig_output_spent()
  // work correctly for ms outputs, known to be spent

  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  std::list<account_public_address> ms_addr_list({ miner_acc.get_public_address(), alice_acc.get_public_address() });

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  bool r = false;

  // make a multisig output
  // tx_0: txin_to_key -> txout_multisig
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc.get_keys(), ms_addr_list, MK_TEST_COINS(2), TESTS_DEFAULT_FEE, 0,
    sources, destinations, true, true, 1);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_0, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_0);

  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  // spend a multisig output from tx_0
  // tx_1: txin_multisig -> txout_to_key
  size_t ms_out_idx = 0;
  tx_source_entry se = AUTO_VAL_INIT(se);
  se.amount =boost::get<currency::tx_out_bare>( tx_0.vout[ms_out_idx]).amount;
  se.ms_keys_count = boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(tx_0.vout[ms_out_idx]).target).keys.size();
  se.ms_sigs_count = 1;
  se.multisig_id = get_multisig_out_id(tx_0, ms_out_idx);
  se.real_output_in_tx_index = ms_out_idx;
  se.real_out_tx_key = get_tx_pub_key_from_extra(tx_0);
  sources.clear();
  sources.push_back(se);
  destinations.clear();
  destinations.push_back(tx_destination_entry(se.amount - TESTS_DEFAULT_FEE, miner_acc.get_public_address()));
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_1, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  bool is_fully_signed = false;
  r = sign_multisig_input_in_tx(tx_1, 0, miner_acc.get_keys(), tx_0, &is_fully_signed);
  CHECK_AND_ASSERT_MES(r && is_fully_signed, false, "sign_multisig_input_in_tx failed, is_fully_signed=" << is_fully_signed);
  events.push_back(tx_1);
  
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, tx_1);

  m_tx_1 = tx_1;
  DO_CALLBACK(events, "check");

  return true;
}

bool test_blockchain_vs_spent_multisig_outs::check(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // check have_tx_keyimges_as_spent()
  currency::blockchain_storage &bcs = c.get_blockchain_storage();
  bool spent = bcs.have_tx_keyimges_as_spent(m_tx_1);
  CHECK_AND_ASSERT_MES(spent, false, "have_tx_keyimges_as_spent returned false for spent ms output!");

  // check is_multisig_output_spent()
  for (const currency::txin_v& in : m_tx_1.vin)
  {
    if (in.type() == typeid(currency::txin_multisig))
    {
      spent = bcs.is_multisig_output_spent(boost::get<currency::txin_multisig>(in).multisig_out_id);
      CHECK_AND_ASSERT_MES(spent, false, "is_multisig_output_spent returned false for spent ms output!");
    }
  }

  return true;
}
