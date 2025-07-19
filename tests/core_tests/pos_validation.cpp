// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "pos_validation.h"
#include "tx_builder.h"
#include "pos_block_builder.h"
#include "random_helper.h"

using namespace epee;
using namespace crypto;
using namespace currency;

pos_validation::pos_validation()
  : m_invalid_block_index(std::numeric_limits<size_t>::max())
  , m_invalid_tx_index(std::numeric_limits<size_t>::max())
{
  REGISTER_CALLBACK_METHOD(pos_validation, configure_core);
  REGISTER_CALLBACK_METHOD(pos_validation, mark_invalid_tx);
  REGISTER_CALLBACK_METHOD(pos_validation, mark_invalid_block);
}

bool pos_validation::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE; //four blocks
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH; //four blocks

  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

bool gen_pos_coinstake_already_spent::generate(std::vector<test_event_entry>& events) const
{
  //test_gentime_settings s = test_generator::get_test_gentime_settings();
  //s.miner_tx_max_outs = 11; // limit genesis outs for speed-up, as we going to spend all premine
  //test_generator::set_test_gentime_settings(s);

  GENERATE_ACCOUNT(preminer);
  GENERATE_ACCOUNT(miner);
  GENERATE_ACCOUNT(alice);
  MAKE_GENESIS_BLOCK(events, blk_0, preminer, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner);
  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner, CURRENCY_MINED_MONEY_UNLOCK_WINDOW - 1);

  // spend all money earned from block 1
  uint64_t amount = get_outs_money_amount(blk_1.miner_tx);
  MAKE_TX(events, tx_0, miner, alice, amount - TESTS_DEFAULT_FEE, blk_1r);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, alice, tx_0);

  CREATE_TEST_WALLET(miner_wlt, miner, blk_0);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, miner_wlt, blk_2, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  LOG_PRINT_L0("miner's transfers:" << ENDL << miner_wlt->dump_transfers(false));

  // Legend: (n) - PoW block, [m] - PoS block
  //  0     1     11    12    13       <-- blockchain height  (assuming CURRENCY_MINED_MONEY_UNLOCK_WINDOW == 10)
  // (0 )--(1 )--(1r)--(2 )--[3 ]      main chain

  // try to make a PoS block referring to genesis' miner tx out, that is already spent
  // create PoS block manually
  block& prev_block = blk_2;
  crypto::hash prev_id = get_block_hash(prev_block);
  size_t height = get_block_height(prev_block) + 1;
  currency::wide_difficulty_type diff = generator.get_difficulty_for_next_block(prev_id, false);
  
  const transaction& stake = blk_1.miner_tx;
  crypto::public_key stake_tx_pub_key = get_tx_pub_key_from_extra(stake);
  size_t stake_output_idx = 0;
  uint64_t stake_output_gidx = 0;
  bool r = find_global_index_for_output(events, prev_id, stake, stake_output_idx, stake_output_gidx);
  CHECK_AND_ASSERT_MES(r, false, "find_global_index_for_output failed");
  uint64_t stake_output_amount =boost::get<currency::tx_out_bare>( stake.vout[stake_output_idx]).amount;
  crypto::key_image stake_output_key_image;
  keypair kp;
  generate_key_image_helper(miner.get_keys(), stake_tx_pub_key, stake_output_idx, kp, stake_output_key_image);
  crypto::public_key stake_output_pubkey = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(stake.vout[stake_output_idx]).target).key;

  pos_block_builder pb;
  pb.step1_init_header(generator.get_hardforks(), height, prev_id);
  pb.step2_set_txs(std::vector<transaction>());
  pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, diff, prev_id, null_hash, prev_block.timestamp);
  pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(prev_block), alice.get_public_address());
  pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, miner);

  block blk_3 = pb.m_block;

  // EXPECTED: blk_2 is rejected as invalid, because coinstake refers to already spent output
  DO_CALLBACK(events, "mark_invalid_block");
  events.push_back(blk_3);

  return true;
}

bool gen_pos_incorrect_timestamp::generate(std::vector<test_event_entry>& events) const
{
  uint64_t genesis_ts = 1450000000;

  GENERATE_ACCOUNT(miner);
  GENERATE_ACCOUNT(alice);
  MAKE_GENESIS_BLOCK(events, blk_0, miner, genesis_ts);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner, BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW);

  // Legend: (n) - PoW block, [m] - PoS block
  //  0     60    61                 <-- blockchain height  (assuming BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW == 60)
  // (0 )--(0r)--[1 ]                main chain

  /* // se
  uint64_t tiny_little_money = 1;
  MAKE_TX(events, tx_0, miner, alice, tiny_little_money, blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner, tx_0);
  */

  // make a PoS block manually with incorrect timestamp
  crypto::hash prev_id = get_block_hash(blk_0r);
  size_t height = BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW + 1;
  currency::wide_difficulty_type diff = generator.get_difficulty_for_next_block(prev_id, false);

  const transaction& stake = blk_0.miner_tx;
  crypto::public_key stake_tx_pub_key = get_tx_pub_key_from_extra(stake);
  size_t stake_output_idx = 0;
  size_t stake_output_gidx = 0;
  uint64_t stake_output_amount =boost::get<currency::tx_out_bare>( stake.vout[stake_output_idx]).amount;
  crypto::key_image stake_output_key_image;
  keypair kp;
  generate_key_image_helper(miner.get_keys(), stake_tx_pub_key, stake_output_idx, kp, stake_output_key_image);
  crypto::public_key stake_output_pubkey = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(stake.vout[stake_output_idx]).target).key;

  pos_block_builder pb;
  pb.step1_init_header(generator.get_hardforks(), height, prev_id);
  pb.step2_set_txs(std::vector<transaction>());
  // use incorrect timestamp_step
  pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, diff, prev_id, null_hash, blk_0r.timestamp - 1, POS_SCAN_WINDOW, POS_SCAN_STEP - 1);
  pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(blk_0r), alice.get_public_address());
  pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, miner);
  block blk_1 = pb.m_block;

  // EXPECTED: blk_1 is rejected as invalid, because it's timestamps % POS_SCAN_STEP != 0
  DO_CALLBACK(events, "mark_invalid_block");
  events.push_back(blk_1);

  // shift core time a little
  uint64_t ts = blk_0r.timestamp + 60;
  events.push_back(event_core_time(ts));

  // Now try PoS timestamp window boundaries.
  pb.clear();
  pb.step1_init_header(generator.get_hardforks(), height, prev_id);
  pb.step2_set_txs(std::vector<transaction>());
  // move timestamp to the future
  pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, diff, prev_id, null_hash, ts + CURRENCY_POS_BLOCK_FUTURE_TIME_LIMIT + 1, POS_SCAN_WINDOW, POS_SCAN_STEP);
  pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(blk_0r), alice.get_public_address());
  pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, miner);
  block blk_2 = pb.m_block;

  // EXPECTED: blk_2 is rejected as invalid, because its timestamp is beyond upper limit
  DO_CALLBACK(events, "mark_invalid_block");
  events.push_back(blk_2);

  // lower limit
  pb.clear();
  pb.step1_init_header(generator.get_hardforks(), height, prev_id);
  pb.step2_set_txs(std::vector<transaction>());
  // move timestamp to the future
  pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, diff, prev_id, null_hash, genesis_ts - POS_SCAN_WINDOW, POS_SCAN_WINDOW, POS_SCAN_STEP);
  pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(blk_0r), alice.get_public_address());
  pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, miner);
  block blk_3 = pb.m_block;

  // EXPECTED: blk_3 is rejected as invalid, because its timestamp is beyond lower limit
  DO_CALLBACK(events, "mark_invalid_block");
  events.push_back(blk_3);

  return true;
}

//------------------------------------------------------------------

gen_pos_too_early_pos_block::gen_pos_too_early_pos_block()
  : m_pos_min_height(CURRENCY_MINED_MONEY_UNLOCK_WINDOW + TESTS_POS_CONFIG_POS_MINIMUM_HEIGH)
{
  REGISTER_CALLBACK_METHOD(gen_pos_too_early_pos_block, configure_core);
}

bool gen_pos_too_early_pos_block::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = time(NULL);

  GENERATE_ACCOUNT(miner);
  MAKE_GENESIS_BLOCK(events, blk_0, miner, ts);
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner, m_pos_min_height - 2);

  // Legend: (n) - PoW block, [m] - PoS block
  //  0     N     N+1                <-- blockchain height (N= CURRENCY_MINED_MONEY_UNLOCK_WINDOW + TESTS_POS_CONFIG_POS_MINIMUM_HEIGH - 2, now is 12)
  // (0 )--(0r)--[1 ]                main chain

  // EXPECTED: blk_1 is rejected as invalid, because PoS blocks are not allowed on that height
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_POS_BLOCK(events, blk_1, blk_0r, miner, std::list<account_base>(1, miner));

  return true;
}

bool gen_pos_too_early_pos_block::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE;
  pc.pos_minimum_heigh = m_pos_min_height;

  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

//------------------------------------------------------------------

bool gen_pos_extra_nonce::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>&)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE;
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH;
  pc.hf4_minimum_mixins = 0;
  pc.hard_forks = m_hardforks;
  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

gen_pos_extra_nonce::gen_pos_extra_nonce()
{
  REGISTER_CALLBACK_METHOD(gen_pos_extra_nonce, configure_core);
  REGISTER_CALLBACK_METHOD(gen_pos_extra_nonce, request_pow_template_with_nonce);
  REGISTER_CALLBACK_METHOD(gen_pos_extra_nonce, check_pow_nonce);
  REGISTER_CALLBACK_METHOD(gen_pos_extra_nonce, check_pos_nonce);
}

// Test: verify custom extra_nonce in blocks and templates
/*
 * Scenarios:
 * 1. PoW block contains m_pow_nonce
 * 2. PoS block contains m_pos_nonce
 * 3. PoW mining template contains m_pow_template_nonce
 */
bool gen_pos_extra_nonce::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(miner);
  GENERATE_ACCOUNT(alice);
  m_accounts.push_back(miner);
  m_accounts.push_back(alice);
  m_pow_nonce = currency::blobdata(254, 'w');
  m_pos_nonce = currency::blobdata(255, 's');
  m_pow_template_nonce = "POW_TEMPLATE123";

  uint64_t ts = test_core_time::get_time();
  MAKE_GENESIS_BLOCK(events, blk_0, miner, ts);
  DO_CALLBACK(events, "configure_core");
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner);

  block blk_2 = AUTO_VAL_INIT(blk_2);
  ts = blk_2.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN / 2; // to increase main chain difficulty
  bool r = generator.construct_block_manually(blk_2, blk_1, miner, test_generator::bf_timestamp, 0, 0, 
    ts, crypto::hash(), 1, transaction(), std::vector<crypto::hash>(), 0, m_pow_nonce);
  CHECK_AND_ASSERT_MES(r, false, "construct_block_manually failed");
  events.push_back(blk_2);

  DO_CALLBACK(events, "check_pow_nonce");
  REWIND_BLOCKS(events, blk_0r, blk_2, miner);

  // setup params for PoS
  const currency::transaction& stake = blk_2.miner_tx;
  
  currency::block new_pos_block;
  bool ok = generate_pos_block_with_extra_nonce(generator, events, miner, alice, blk_0r, stake, m_pos_nonce, new_pos_block); 
  CHECK_AND_ASSERT_MES(ok, false, "generate_pos_block_with_extra_nonce failed");

  events.push_back(new_pos_block);
  DO_CALLBACK(events, "check_pos_nonce");
  DO_CALLBACK(events, "request_pow_template_with_nonce");
  return true;
}

bool gen_pos_extra_nonce::request_pow_template_with_nonce(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  block bl;
  wide_difficulty_type diff;
  uint64_t height;
  bool ok = c.get_block_template(bl, m_accounts[0].get_public_address(), m_accounts[0].get_public_address(), diff, height, m_pow_template_nonce);
  CHECK_AND_ASSERT_MES(ok, false, "get_block_template failed");
  CHECK_AND_ASSERT_MES(has_extra_nonce(bl, m_pow_template_nonce), false, "PoW extra_nonce not found");
  return true;
}

bool gen_pos_extra_nonce::check_pow_nonce(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  block top;
  bool ok = c.get_blockchain_storage().get_top_block(top);
  CHECK_AND_ASSERT_MES(ok, false, "get_top_block failed");
  CHECK_AND_ASSERT_MES(has_extra_nonce(top, m_pow_nonce), false, "PoW extra_nonce not found");
  return true;
}

bool gen_pos_extra_nonce::check_pos_nonce(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  block top;
  bool ok = c.get_blockchain_storage().get_top_block(top);
  CHECK_AND_ASSERT_MES(ok, false, "get_top_block failed");
  CHECK_AND_ASSERT_MES(has_extra_nonce(top, m_pos_nonce), false, "PoS extra_nonce not found");
  return true;
}

bool gen_pos_extra_nonce::has_extra_nonce(currency::block& blk, const std::string& expected_nonce)
{
  if (auto const* ud = get_type_in_variant_container<extra_user_data>(blk.miner_tx.extra)) {
    LOG_PRINT_L0("Found extra nonce: '" << ud->buff << "' expected: '" << expected_nonce << "'");
    return ud->buff == expected_nonce;
  }
  return false;
}

//------------------------------------------------------------------

gen_pos_min_allowed_height::gen_pos_min_allowed_height()
{
  REGISTER_CALLBACK_METHOD(gen_pos_min_allowed_height, configure_core);
}

bool gen_pos_min_allowed_height::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(miner);
  GENERATE_ACCOUNT(alice);
  MAKE_GENESIS_BLOCK(events, blk_0, miner, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, alice, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // Legend: (n) - PoW block, [m] - PoS block
  //  0     N     N+1                <-- blockchain height  (assuming CURRENCY_MINED_MONEY_UNLOCK_WINDOW == N)
  // (0 )--(0r)--[1 ]                main chain

  MAKE_TX(events, tx_1, alice, miner, MK_TEST_COINS(1), blk_0r);

  crypto::hash prev_id = get_block_hash(blk_0r);
  size_t height = CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1;
  currency::wide_difficulty_type diff = generator.get_difficulty_for_next_block(prev_id, false);

  const transaction& stake = blk_0.miner_tx;
  crypto::public_key stake_tx_pub_key = get_tx_pub_key_from_extra(stake);
  size_t stake_output_idx = 0;
  size_t stake_output_gidx = 0;
  uint64_t stake_output_amount =boost::get<currency::tx_out_bare>( stake.vout[stake_output_idx]).amount;
  crypto::key_image stake_output_key_image;
  keypair kp;
  generate_key_image_helper(miner.get_keys(), stake_tx_pub_key, stake_output_idx, kp, stake_output_key_image);
  crypto::public_key stake_output_pubkey = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(stake.vout[stake_output_idx]).target).key;

  pos_block_builder pb;
  pb.step1_init_header(generator.get_hardforks(), height, prev_id);
  pb.step2_set_txs(std::vector<transaction>(1, tx_1));
  pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, diff, prev_id, null_hash, blk_0r.timestamp);
  pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(blk_0r), alice.get_public_address());
  pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, miner);
  block blk_1 = pb.m_block;

  // EXPECTED: blk_1 is accepted
  events.push_back(blk_1);

  return true;
}

bool gen_pos_min_allowed_height::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE; //four blocks
  pc.pos_minimum_heigh = CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1;

  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

//------------------------------------------------------------------

bool gen_pos_invalid_coinbase::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make a PoS block with zero signature in miner_tx (exploits a bug).

  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // create PoS block manually
  const block& prev_block = blk_0r;
  crypto::hash prev_id = get_block_hash(prev_block);
  size_t height = CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1;
  currency::wide_difficulty_type diff = generator.get_difficulty_for_next_block(prev_id, false);

  const transaction& stake = blk_0.miner_tx;
  crypto::public_key stake_tx_pub_key = get_tx_pub_key_from_extra(stake);
  size_t stake_output_idx = 0;
  size_t stake_output_gidx = 0;
  uint64_t stake_output_amount =boost::get<currency::tx_out_bare>( stake.vout[stake_output_idx]).amount;
  crypto::key_image stake_output_key_image;
  keypair kp;
  generate_key_image_helper(miner_acc.get_keys(), stake_tx_pub_key, stake_output_idx, kp, stake_output_key_image);
  crypto::public_key stake_output_pubkey = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(stake.vout[stake_output_idx]).target).key;

  pos_block_builder pb;
  pb.step1_init_header(generator.get_hardforks(), height, prev_id);
  pb.step2_set_txs(std::vector<transaction>());
  pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, diff, prev_id, null_hash, prev_block.timestamp);
  pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(prev_block), alice_acc.get_public_address());
  pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, miner_acc);

  pb.m_block.miner_tx.signatures.clear(); // remove signatures

  block blk_1 = pb.m_block;

  // EXPECTED: blk_1 is rejected as invalid, because coinbase cointains no signatures
  DO_CALLBACK(events, "mark_invalid_block");
  events.push_back(blk_1);

  return true;

}

//------------------------------------------------------------------

void decompose_amount_into_exact_number_of_pos_entries(uint64_t amount, size_t pos_entries_count, std::vector<uint64_t> &pos_amounts)
{
  std::list<uint64_t> pos_amounts_list;

  // decompose algorithm: we need exactly 'pos_entries_count' entries of power-10 which will give 'amount' in sum
  auto f = [&pos_amounts_list](uint64_t a){ pos_amounts_list.push_back(a); };
  decompose_amount_into_digits(amount, DEFAULT_DUST_THRESHOLD, f, f); // get some initial decomposition
  while (pos_amounts_list.size() < pos_entries_count)
  {
    for (auto it = pos_amounts_list.begin(); it != pos_amounts_list.end() && pos_amounts_list.size() < pos_entries_count; /* nothing */)
    {
      if (*it >= 2 * TESTS_DEFAULT_FEE)
      {
        // each iteration pops one element 'a' and pushes two elements: c1 and c2, so that a == c1 + c2 (sum is invariant)
        uint64_t a = *it;

        // remove zeros form the tail of 'a':   a = 1230000  ->  c = 123  *  pow = 10000
        uint64_t c = a, pow = 1;
        for(;;)
        {
          uint64_t tc = c / 10, tpow = pow * 10;
          if (tc * tpow != a)
            break;
          c = tc;
          pow = tpow;
        }

        if (c == 1)
        {
          c = 10;
          pow /= 10;
        }
        // randomly split 'a' into sum of c1 and c2
        // c in [2; 10], pow in [1...10^^n]
        uint64_t c1 = random_in_range(1, c - 1); // c1: [1; c-1]
        uint64_t c2 = c - c1;                    // c2: [c1; c],  c1 + c2 == c
        pos_amounts_list.push_front(c1 * pow);
        pos_amounts_list.push_front(c2 * pow);
        it = pos_amounts_list.erase(it);
        continue;
      }
      ++it;
    }
  }

  // random shuffle pos amounts to get rid of any kind of order
  pos_amounts.assign(pos_amounts_list.begin(), pos_amounts_list.end());
  std::shuffle(pos_amounts.begin(), pos_amounts.end(), crypto::uniform_random_bit_generator());
}

bool populate_wallet_with_stake_coins(std::shared_ptr<tools::wallet2> w, std::shared_ptr<tools::wallet2> money_source_w, size_t pos_entries_count, uint64_t amount, const std::vector<uint64_t>& pos_amounts)
{
#define WALLET_TRY_CATCH(statement)                             \
    try                                                         \
    {                                                           \
      statement;                                                \
    }                                                           \
    catch (std::exception& e)                                   \
    {                                                           \
      LOG_ERROR("Wallet operation failed: " << e.what());       \
      return false;                                             \
    }                                                           

  LOG_PRINT_L0("populate_wallet_with_stake_coins: amount: " << print_money(amount) << ", pos_entries_count: " << pos_entries_count);
  
  // make sure w is empty
  w->refresh();
  uint64_t balance_unlocked = 0;
  uint64_t balance = w->balance(balance_unlocked);
  CHECK_AND_ASSERT_MES(balance == balance_unlocked, false, "WARNING: balance is " << balance << " NOT EQUAL TO unlocked balance, which is " << balance_unlocked);
  if (balance_unlocked > TESTS_DEFAULT_FEE)
    WALLET_TRY_CATCH(w->transfer(balance_unlocked, money_source_w->get_account().get_public_address()));

  uint64_t sum = 0;
  for(auto a : pos_amounts)
    sum += a;
  CHECK_AND_ASSERT_MES(pos_amounts.size() == pos_entries_count && sum == amount, false, "Failed to decompose " << print_money(amount) << " into " << pos_entries_count << " entries: pos_amounts.size is " << pos_amounts.size() << ", sum is " << sum);

  // populate current wallet with pos entries from scratch
  money_source_w->refresh();
  balance = money_source_w->balance(balance_unlocked);
  CHECK_AND_ASSERT_MES(balance_unlocked > amount + TESTS_DEFAULT_FEE * pos_entries_count, false, "source wallet has not enough money: balance_unlocked: " << balance_unlocked << ", balance: " << balance << ", required amount: " << amount + TESTS_DEFAULT_FEE * pos_entries_count);

  const account_public_address& wallet_address = w->get_account().get_public_address();
  std::vector<tx_destination_entry> destinations;
  destinations.reserve(pos_amounts.size());
  for(auto pos_amount : pos_amounts)
    destinations.push_back(tx_destination_entry(pos_amount, wallet_address));

  WALLET_TRY_CATCH(money_source_w->transfer(destinations, 0, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment));

  return true;

#undef WALLET_TRY_CATCH
}

bool pos_wallet_minting_same_amount_diff_outs::prepare_wallets_0(currency::core& c, const std::vector<test_event_entry>& events, std::vector<wlt_info_t> &minting_wallets, std::vector<size_t>& minting_wallets_order)
{
  // Prepare equal wallets: same amount, same transfers (count and amount), same pos entries
  // This is to test evenness of simulation environment and the stuff beyound wallet pos entries.

  bool r = false;
  block b = AUTO_VAL_INIT(b);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX); // Alice is preminer

  size_t pos_entries_count = 990;
  std::vector<uint64_t> pos_amounts; // use the same stake decomposition for all wallets
  decompose_amount_into_exact_number_of_pos_entries(m_wallet_stake_amount, pos_entries_count, pos_amounts);

  std::vector<pos_entry> first_wallet_pos_entries;

  // populate minting wallets with money
  for (size_t i = 0; i < m_minting_wallets_count; ++i)
  {
    minting_wallets_order[i] = i;
    auto& w = minting_wallets[i];
    w.w = init_playtime_test_wallet(events, c, ALICE_ACC_IDX + 1 + i);
    //w.pos_entries_count = random_in_range(m_pos_entries_count_min, m_pos_entries_count_max);
    w.pos_entries_count = pos_entries_count;

    r = populate_wallet_with_stake_coins(w.w, alice_wlt, w.pos_entries_count, m_wallet_stake_amount, pos_amounts);
    CHECK_AND_ASSERT_MES(r, false, "populate_wallet_with_stake_coins failed");

    r = mine_next_pow_blocks_in_playtime(alice_wlt->get_account().get_public_address(), c, WALLET_DEFAULT_TX_SPENDABLE_AGE + 1); // to preventing run out of the money for Alice
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");
    c.get_blockchain_storage().get_top_block(b);
    uint64_t ts = b.timestamp;
    test_core_time::adjust(ts);

    // check pos entries actually available
    w.w->refresh();
    std::vector<pos_entry> pos_entries;
    r = w.w->get_pos_entries(pos_entries);
    CHECK_AND_ASSERT_MES(r, false, "get_pos_entries failed");
    CHECK_AND_ASSERT_MES(pos_entries.size() == w.pos_entries_count, false, "incorrect pos entries count in the wallet: " << pos_entries.size() << ", expected: " << w.pos_entries_count);
    uint64_t balance_unlocked = 0;
    w.w->balance(balance_unlocked);
    CHECK_AND_ASSERT_MES(balance_unlocked == m_wallet_stake_amount, false, "incorrect wallet unlocked balance: " << print_money(balance_unlocked) << ", expected: " << print_money(m_wallet_stake_amount));

    // make sure all wallets has the same distribution of pos_entries
    if (i == 0)
    {
      first_wallet_pos_entries = pos_entries;
    }
    else
    {
      for(size_t i = 0; i < pos_amounts.size(); ++i)
      {
        CHECK_AND_ASSERT_MES(pos_entries[i].amount == first_wallet_pos_entries[i].amount, false, "Wallet pos entry #" << i << " has incorrect amount: " << print_money(pos_entries[i].amount) << ", expected (as in 1st wallet): " << print_money(first_wallet_pos_entries[i].amount));
      }
    }
  }

  return true;
}

bool pos_wallet_minting_same_amount_diff_outs::prepare_wallets_1(currency::core& c, const std::vector<test_event_entry>& events, std::vector<wlt_info_t> &minting_wallets, std::vector<size_t>& minting_wallets_order)
{
  // prepare five wallets with the same total amount but different pos entries distribution to test how it influence minting efficiency
  size_t pos_entries_count_values[] = {100, 1000, 300, 990, 2000};
  CHECK_AND_ASSERT_MES(m_minting_wallets_count == (sizeof pos_entries_count_values / sizeof pos_entries_count_values[0]), false, "sizeof pos_entries_count_values != m_minting_wallets_count");
  
  bool r = false;
  block b = AUTO_VAL_INIT(b);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX); // Alice is preminer

  // populate minting wallets with money
  for (size_t i = 0; i < m_minting_wallets_count; ++i)
  {
    minting_wallets_order[i] = i;
    auto& w = minting_wallets[i];
    w.w = init_playtime_test_wallet(events, c, ALICE_ACC_IDX + 1 + i);
    //w.pos_entries_count = random_in_range(m_pos_entries_count_min, m_pos_entries_count_max);
    w.pos_entries_count = pos_entries_count_values[i % (sizeof pos_entries_count_values / sizeof pos_entries_count_values[0])];

    std::vector<uint64_t> pos_amounts;
    decompose_amount_into_exact_number_of_pos_entries(m_wallet_stake_amount, w.pos_entries_count, pos_amounts);
    r = populate_wallet_with_stake_coins(w.w, alice_wlt, w.pos_entries_count, m_wallet_stake_amount, pos_amounts);
    CHECK_AND_ASSERT_MES(r, false, "populate_wallet_with_stake_coins failed");

    r = mine_next_pow_blocks_in_playtime(alice_wlt->get_account().get_public_address(), c, WALLET_DEFAULT_TX_SPENDABLE_AGE + 1); // to preventing run out of the money for Alice
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");
    c.get_blockchain_storage().get_top_block(b);
    uint64_t ts = b.timestamp;
    test_core_time::adjust(ts);

    // check pos entries actually available
    w.w->refresh();
    size_t pos_entries_count = w.w->get_pos_entries_count();
    CHECK_AND_ASSERT_MES(pos_entries_count == w.pos_entries_count, false, "incorrect pos entries count in the wallet: " << pos_entries_count << ", expected: " << w.pos_entries_count);
    uint64_t balance_unlocked = 0;
    w.w->balance(balance_unlocked);
    CHECK_AND_ASSERT_MES(balance_unlocked == m_wallet_stake_amount, false, "incorrect wallet unlocked balance: " << print_money(balance_unlocked) << ", expected: " << print_money(m_wallet_stake_amount));
  }

  return true;
}

pos_wallet_minting_same_amount_diff_outs::pos_wallet_minting_same_amount_diff_outs()
{
  REGISTER_CALLBACK_METHOD(pos_wallet_minting_same_amount_diff_outs, c1);

  m_minting_wallets_count = 5;
  m_wallet_stake_amount   = 1000 * COIN;
}

bool pos_wallet_minting_same_amount_diff_outs::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.resize(ALICE_ACC_IDX + 1 + m_minting_wallets_count);
  m_accounts[MINER_ACC_IDX] = miner_acc;
  m_accounts[ALICE_ACC_IDX] = alice_acc;
  for (size_t i = 0; i < m_minting_wallets_count; ++i)
    m_accounts[ALICE_ACC_IDX + 1 + i].generate();

  MAKE_GENESIS_BLOCK(events, blk_0, alice_acc, test_core_time::get_time()); // Alice is a preminer!
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK(events, "c1");

  return true;
}

bool pos_wallet_minting_same_amount_diff_outs::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // Test idea: few wallets with the same total amount but different transfers are mining simultaneously.
  // All generated money is calculated and goes to Miner, so wallets' balances are constant.
  // Generated amounts per each wallet should be more or less the same, as far they have the same stakes.

  uint64_t ts = test_core_time::get_time();
  bool r = false;
  block b = AUTO_VAL_INIT(b);

  //
  // 1/2 prepare wallets
  //
  std::vector<size_t> minting_wallets_order(m_minting_wallets_count); // to iterate wallets in random order while minting
  std::vector<wlt_info_t> minting_wallets(m_minting_wallets_count);
  r = prepare_wallets_0(c, events, minting_wallets, minting_wallets_order);
  CHECK_AND_ASSERT_MES(r, false, "prepare wallets failed");


  //
  // 2/2 simulate the mining process
  //
  uint64_t pos_target = DIFFICULTY_POS_TARGET;
  uint64_t pow_target = DIFFICULTY_POW_TARGET;
  uint64_t last_pow_block_ts = ts - pow_target;
  //uint64_t last_pos_block_ts = ts - pos_target; // block_header::ts
  //uint64_t last_pos_block_ts_real = ts - pos_target; // real time at the moment of successfull minting
  wide_difficulty_type last_pos_diff = 0;
  size_t pos_blocks = 0;
  uint64_t total_minted_coins = 0;

#define LOG1_FILENAME "pos_wallet_minting_same_amount_diff_outs.log" // use custom log to store data sheet-friendly
  // print header to the log
  std::stringstream ss;
  ss << "\t" << "height" << "\t" << "pos %" << "\t" << "mnt.coins/pos blks" << "\t" << "last_pos_diff" << "\t" << "stake_to_diff_coeff" << "\t" << "pow_seq_factor" << "\t" << "pos_seq_factor";
  for (size_t w_idx = 0; w_idx < m_minting_wallets_count; ++w_idx)
    ss << "\tw#" << w_idx << "_" << minting_wallets[w_idx].pos_entries_count;
  LOG_PRINT2(LOG1_FILENAME, ss.str(), LOG_LEVEL_0);

  for(size_t n = 0; n < 30000; ++n)
  {
    // log status
    c.get_blockchain_storage().get_top_block(b);
    size_t height = get_block_height(b);
    size_t pow_seq_factor = c.get_blockchain_storage().get_current_sequence_factor(false);
    size_t pos_seq_factor = c.get_blockchain_storage().get_current_sequence_factor(true);
    double pos_total_stake_to_diff_coeff = static_cast<double>(last_pos_diff / COIN) / static_cast<double>(m_minting_wallets_count * m_wallet_stake_amount / COIN);
    float pos_blocks_proportion = static_cast<float>(pos_blocks) / height;
    uint64_t minted_coins_per_pos_block = total_minted_coins / height;
    ss.str(""); // <-- just a wired way to clear stringstream, have a good day!
    ss << "\t" << height << "\t" << pos_blocks_proportion << "\t" << print_money(minted_coins_per_pos_block) << "\t" << last_pos_diff << "\t" << pos_total_stake_to_diff_coeff << "\t" << pow_seq_factor << "\t" << pos_seq_factor;
    for (size_t w_idx = 0; w_idx < m_minting_wallets_count; ++w_idx)
      ss << "\t" << print_money(minting_wallets[w_idx].mined_coins);
    LOG_PRINT2(LOG1_FILENAME, ss.str(), LOG_LEVEL_0);

    if (n % 250 == 0)
      dump_wallets_entries(minting_wallets);

    // pow
    uint64_t ideal_next_pow_block_ts = last_pow_block_ts + pow_target;
    if (ts < ideal_next_pow_block_ts)
      ts = ideal_next_pow_block_ts; // "wait" until ideal_next_pow_block_ts if it was not already happened (fast forward but don't wayback the time)
    test_core_time::adjust(ts);
    size_t tx_count_before = c.get_pool_transactions_count();
    r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
    CHECK_AND_ASSERT_MES(tx_count_before == 0 || c.get_pool_transactions_count() < tx_count_before, false, "invalid number of txs in tx pool: " << c.get_pool_transactions_count() << ", was: " << tx_count_before);
    last_pow_block_ts = ts;

    // pos
    uint64_t pos_ts_upper_bound = ts + pos_target / 2;
    while(ts <= pos_ts_upper_bound)
    {
      // every POS_SCAN_STEP all wallets tries to mint a PoS block simultaneously
      std::shuffle(minting_wallets_order.begin(), minting_wallets_order.end(), crypto::uniform_random_bit_generator()); // shuffle to iterate in random order
      bool a_block_was_minted = true;
      do
      {
        a_block_was_minted = false;
        for(size_t i = 0; i < m_minting_wallets_count; ++i)
        {
          size_t w_idx = minting_wallets_order[i];
          LOG_PRINT_L0("Minting with wallet #" << w_idx);
          auto& current_wallet = minting_wallets[w_idx];
          current_wallet.w->refresh();
          if (n % 10 == 0) // check balances every 10 PoW blocks in a sake of speed
          {
            uint64_t balance = current_wallet.w->balance();
            CHECK_AND_ASSERT_MES(balance == m_wallet_stake_amount, false, "Wallet #" << w_idx << " has incorrect balance: " << print_money(balance) << ", expected: " << print_money(m_wallet_stake_amount));
          }
          last_pos_diff = c.get_blockchain_storage().get_next_diff_conditional(true);
          uint64_t bc_height_before = c.get_blockchain_storage().get_current_blockchain_size();
          size_t pos_entries_count = 0;
          if (mine_next_pos_block_in_playtime_with_wallet(*current_wallet.w.get(), m_accounts[MINER_ACC_IDX].get_public_address(), pos_entries_count))
          {
            ++pos_blocks;
            a_block_was_minted = true;
            height = c.get_blockchain_storage().get_current_blockchain_size();
            CHECK_AND_ASSERT_MES(bc_height_before < height, false, "didn't mint a block!");
            LOG_PRINT_GREEN(">>>>>>>>>>>>>> Minted a PoS block by wallet #" << w_idx << " at height " << height << " and time " << ts << " with difficuly: " << last_pos_diff << " using " << pos_entries_count << " pos entries", LOG_LEVEL_0);
            CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "invalid number of txs in tx pool: " << c.get_pool_transactions_count());
            c.get_blockchain_storage().get_top_block(b);
            //last_pos_block_ts = b.timestamp;
            //last_pos_block_ts_real = ts;
            CHECK_AND_ASSERT_MES(is_pos_block(b), false, "Top block is not a PoS block!");
            uint64_t generated_coins = get_outs_money_amount(b.miner_tx) - boost::get<txin_to_key>(b.miner_tx.vin[1]).amount;
            current_wallet.mined_coins += generated_coins;
            total_minted_coins += generated_coins;
          }
        }
      }
      while(a_block_was_minted);

      ts += POS_SCAN_STEP;
      test_core_time::adjust(ts);
    }
  }

  // Todo: add generated money comparation

  return true;

#undef LOG1_FILENAME
}

void pos_wallet_minting_same_amount_diff_outs::dump_wallets_entries(const std::vector<wlt_info_t>& minting_wallets)
{
#define LOG2_FILENAME "pos_wallet_minting_same_amount_diff_outs.wallets_dumps.log"
  LOG_PRINT2(LOG2_FILENAME, ENDL << ENDL << ENDL << ENDL << ENDL << ENDL, LOG_LEVEL_0);
  for (size_t i = 0; i < minting_wallets.size(); ++i)
  {
    LOG_PRINT2(LOG2_FILENAME, "wallet #" << i << ":" << ENDL << minting_wallets[i].w->dump_transfers() << ENDL, LOG_LEVEL_0);
  }
#undef LOG2_FILENAME
}

//------------------------------------------------------------------

pos_wallet_big_block_test::pos_wallet_big_block_test()
{
  REGISTER_CALLBACK_METHOD(pos_wallet_big_block_test, c1);
  REGISTER_CALLBACK_METHOD(pos_wallet_big_block_test, configure_core);
}

bool pos_wallet_big_block_test::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: check PoS mining via wallet for block size greather than median (of sizes of last N block)

  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  // make a couple of huge txs
  bool r = false;
  size_t padding_size = CURRENCY_MAX_TRANSACTION_BLOB_SIZE * 5 / 8; // to get over one median (of sizes of last N blocks) but not to exceed 2 medians
  extra_padding padding = AUTO_VAL_INIT(padding);
  padding.buff.resize(padding_size);
  std::vector<extra_v> extra({ padding });
  
  std::vector<tx_source_entry> sources_1, sources_2;
  r = fill_tx_sources(sources_1, events, blk_0r, miner_acc.get_keys(), MK_TEST_COINS(3), 0);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  r = fill_tx_sources(sources_2, events, blk_0r, miner_acc.get_keys(), MK_TEST_COINS(3), 0, sources_1);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");

  std::vector<tx_destination_entry> destinations({ tx_destination_entry(MK_TEST_COINS(1), miner_acc.get_public_address())}); // yep, it will be bigger fee than usual, don't mind

  crypto::secret_key stub;
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(miner_acc.get_keys(), sources_1, destinations, extra, empty_attachment, tx_1, get_tx_version_from_events(events), stub, 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  size_t tx_size = get_object_blobsize(tx_1);
  CHECK_AND_ASSERT_MES(tx_size > padding_size, false, "Tx size is: " << tx_size << ", expected to be bigger than: " << padding_size);
  events.push_back(tx_1); // push it to the pool

  transaction tx_2 = AUTO_VAL_INIT(tx_2);
  r = construct_tx(miner_acc.get_keys(), sources_2, destinations, extra, empty_attachment, tx_2, get_tx_version_from_events(events), stub, 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  tx_size = get_object_blobsize(tx_2);
  CHECK_AND_ASSERT_MES(tx_size > padding_size, false, "Tx size is: " << tx_size << ", expected to be bigger than: " << padding_size);
  events.push_back(tx_2); // push it to the pool


  DO_CALLBACK(events, "c1");

  return true;
}

bool pos_wallet_big_block_test::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE; //four blocks
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH; //four blocks

  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

bool pos_wallet_big_block_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false, stub_bool;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  miner_wlt->refresh();
  miner_wlt->scan_tx_pool(stub_bool);

  LOG_PRINT_L0("miner transfers:" << ENDL << miner_wlt->dump_transfers(false));

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 2, false, "Incorrect number of txs in the pool: " << c.get_pool_transactions_count());

  try
  {
    r = miner_wlt->try_mint_pos();
  }
  catch (std::exception& e)
  {
    LOG_ERROR("Got exception in try_mint_pos(): " << e.what());
    return false;
  }
  CHECK_AND_ASSERT_MES(r, false, "try_mint_pos failed");

  // make sure the tx has gone
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect number of txs in the pool: " << c.get_pool_transactions_count());

  // check top block - should be PoS with certain size
  block b = AUTO_VAL_INIT(b);
  r = c.get_blockchain_storage().get_top_block(b);
  CHECK_AND_ASSERT_MES(r, false, "get_top_block failed");
  CHECK_AND_ASSERT_MES(is_pos_block(b), false, "top block is not PoS");

  size_t block_size = get_object_blobsize(b.miner_tx);
  for (const crypto::hash& h : b.tx_hashes)
  {
    transaction t = AUTO_VAL_INIT(t);
    r = c.get_transaction(h, t);
    CHECK_AND_ASSERT_MES(r, false, "get_transaction failed");
    block_size += get_object_blobsize(t);
  }

  uint64_t block_size_median = c.get_blockchain_storage().get_current_comulative_blocksize_limit() / 2;
  CHECK_AND_ASSERT_MES(block_size_median < block_size && block_size < 2 * block_size_median, false, "Incorrect PoS block size: " << block_size << ", expected to be between median and 2 medinas: " << block_size_median);

  return true;
}

//------------------------------------------------------------------

pos_altblocks_validation::pos_altblocks_validation()
{
  //test_chain_unit_base::set_hardforks_for_old_tests();
}

bool pos_altblocks_validation::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = 1;
  pc.pos_minimum_heigh = 1;
  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

bool pos_altblocks_validation::generate(std::vector<test_event_entry>& events) const
{
  bool r = false;
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();

  std::list<account_base> miner_acc_lst(1, miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, alice_acc);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  bool HF4_active = m_hardforks.is_hardfork_active_for_height(ZANO_HARDFORK_04_ZARCANUM, get_block_height(blk_1r) + 1);

  MAKE_NEXT_BLOCK(events, blk_2, blk_1r, miner_acc);
  const transaction& stake_tx = blk_1.miner_tx;
  uint64_t stake_tx_out_id = 0;
  uint64_t alice_money = decode_native_output_amount_or_throw(alice_acc, blk_1.miner_tx, stake_tx_out_id);
  //// select stake_tx_out_id as an output with the biggest amount
  //for (size_t i = 1; i < stake_tx.vout.size(); ++i)
  //{
  //  if (boost::get<currency::tx_out_bare>(stake_tx.vout[i]).amount > boost::get<currency::tx_out_bare>( stake_tx.vout[stake_tx_out_id]).amount)
  //    stake_tx_out_id = i;
  //}

  MAKE_TX_FEE(events, tx_0, alice_acc, alice_acc, alice_money - TESTS_DEFAULT_FEE * 17, TESTS_DEFAULT_FEE * 17, blk_2);
  // tx_0 transfers all Alice's money, so it effectevily spends all outputs in stake_ts, make sure it does
  //CHECK_AND_ASSERT_MES(tx_0.vin.size() == stake_tx.vout.size(), false, "probably, tx_0 doesn't spend all Alice's money as expected, tx_0.vin.size()=" << tx_0.vin.size() << ", stake_tx.vout.size()=" << stake_tx.vout.size());

  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_0);
  
  // remember some tx_0's output info for further reference
  size_t tx_0_some_output_idx = 0;
  uint64_t tx_0_some_output_gindex = UINT64_MAX;
  r = find_global_index_for_output(events, get_block_hash(blk_3), tx_0, tx_0_some_output_idx, tx_0_some_output_gindex);
  CHECK_AND_ASSERT_MES(r, false, "find_global_index_for_output failed");

  REWIND_BLOCKS_N_WITH_TIME(events, blk_3r, blk_3, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  MAKE_NEXT_POS_BLOCK(events, blk_4, blk_3r, miner_acc, miner_acc_lst);
  MAKE_NEXT_BLOCK(events, blk_5, blk_4, miner_acc);
  MAKE_NEXT_POS_BLOCK(events, blk_6, blk_5, miner_acc, miner_acc_lst);

  //  0      11       21      22      23      33      34      35      36         <- height
  //
  //          +------- blk_1 mined by Alice
  //          |
  // (0 )-   (1 )-...(1r)-   (2 )-   (3 )-...(3r)-   (4 )-   (5 )-   (6 )-       <- main chain
  //          |                      tx_0
  //          +---<<---uses-blk_1-out--+


  // Case 1 (should pass)
  // alt PoS block (blk_2a) refers in its coinstake to an output (stake_tx_out_id) already spent in the main chain
  block blk_2a = AUTO_VAL_INIT(blk_2a);
  r = generate_pos_block_with_given_coinstake(generator, events, alice_acc, blk_1r, stake_tx, stake_tx_out_id, blk_2a);
  CHECK_AND_ASSERT_MES(r, false, "generate_pos_block_with_given_coinstake failed");
  ADD_CUSTOM_EVENT(events, blk_2a);
  CHECK_AND_ASSERT_MES(generator.add_block_info(blk_2a, std::list<transaction>()), false, "add_block_info failed");

  //  0      11       21      22      23      33      34      35      36         <- height
  //
  //          +-----------------------+
  //          |                      tx_0                                        tx_0 spends all outputs in blk_1 (main chain)
  // (0 )-   (1 )-...(1r)-   (2 )-   (3 )-...(3r)-   (4 )-   (5 )-   (6 )-       <- main chain
  //           |        \ 
  //           |         \-  (2a)-                                               <- alt chain
  //           +--------------+                                                  PoS block 2a uses stake already spent in main chain


  // Case 2a (should fail)
  // alt PoS block (blk_3a) refers in its coinstake to an output (stake_tx_out_id) already spent in this alt chain (in blk_2a)
  block blk_3a = AUTO_VAL_INIT(blk_3a);
  r = generate_pos_block_with_given_coinstake(generator, events, alice_acc, blk_2a, stake_tx, stake_tx_out_id, blk_3a);
  CHECK_AND_ASSERT_MES(r, false, "generate_pos_block_with_given_coinstake failed");
  DO_CALLBACK(events, "mark_invalid_block");
  ADD_CUSTOM_EVENT(events, blk_3a);

  //  0      11       21      22      23      33      34      35      36         <- height
  //
  //          +-----------------------+
  //          |                      tx_0                                        tx_0 spends all outputs in blk_1 (main chain)
  // (0 )-   (1 )-...(1r)-   (2 )-   (3 )-...(3r)-   (4 )-   (5 )-   (6 )-       <- main chain
  //          ||        \ 
  //          ||         \-  (2a)-   #3a#-                                       <- alt chain
  //          |+--------------+       |                                          PoS block 2a uses stake already spent in main chain (okay)
  //          +-----------------------+                                          PoS block 3a uses stake already spent in current alt chain (fail)


  // Case 2b (should fail)
  // alt PoS block (blk_3aa) has invalid signature
  block blk_3aa{};
  r = generate_pos_block_with_given_coinstake(generator, events, miner_acc, blk_2a, blk_0r.miner_tx, 0, blk_3aa);
  CHECK_AND_ASSERT_MES(r, false, "generate_pos_block_with_given_coinstake failed");
  if (HF4_active)
    boost::get<zarcanum_sig>(blk_3aa.miner_tx.signatures[0]).y0.m_u64[1] = 5; // invalidate signature
  else
    boost::get<NLSAG_sig>(blk_3aa.miner_tx.signatures[0]).s[0].c.data[5] = 7; // invalidate signature
  DO_CALLBACK(events, "mark_invalid_block");
  ADD_CUSTOM_EVENT(events, blk_3aa);

  //  0      11       21      22      23      33      34      35      36         <- height
  //
  //          +-----------------------+
  //          |                      tx_0                                        tx_0 spends all outputs in blk_1 (main chain)
  // (0 )-   (1 )-...(1r)-   (2 )-   (3 )-...(3r)-   (4 )-   (5 )-   (6 )-       <- main chain
  //          ||        \ 
  //          ||         \-  (2a)-   #3aa#-                                      <- alt chain
  //          |+--------------+       |                                          PoS block 2a uses stake already spent in main chain (okay)
  //          +-----------------------+                                          PoS block 3aa has incorrect signature (fail)

  REWIND_BLOCKS_N_WITH_TIME(events, blk_2br, blk_2a, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // Case 3 (should pass)
  // alt PoS block (blk_3b) in its coinstake refers to an output from current altchain (from blk_2a)
  block blk_3b = AUTO_VAL_INIT(blk_3b);
  r = generate_pos_block_with_given_coinstake(generator, events, alice_acc, blk_2br, blk_2a.miner_tx, 0, blk_3b);
  CHECK_AND_ASSERT_MES(r, false, "generate_pos_block_with_given_coinstake failed");
  ADD_CUSTOM_EVENT(events, blk_3b);
  CHECK_AND_ASSERT_MES(generator.add_block_info(blk_3b, std::list<transaction>()), false, "add_block_info failed");

  //  0      11       21      22      23      33      34      35      36         <- height
  //
  //          +-----------------------+
  //          |                      tx_0                                                tx_0 spends all outputs in blk_1 (main chain)
  // (0 )-   (1 )-...(1r)-   (2 )-   (3 )- ........  (3r)-   (4 )-   (5 )-   (6 )-       <- main chain
  //          ||        \ 
  //          ||         \-  (2a)-   #3a#-                                               <- alt chain
  //          |+--------------+ \     |                                                  PoS block 2a uses stake already spent in main chain (okay)
  //          +---------------|-------+                                                  PoS block 3a uses stake already spent in current alt chain (fail)
  //                          |   \ 
  //                          |    \ ...... (2br)-   (3b)-                               <- alt chain
  //                          |                       |
  //                          +-----------------------+                                  PoS block 3b uses as stake an output, created in current alt chain (2a)


  // Case 4 (should fail)
  // alt PoS block (blk_4b) in its coinstake refers to an output (tx_0) that appeared in the main chain (blk_3) above split height
  block blk_4b = AUTO_VAL_INIT(blk_4b);
  r = generate_pos_block_with_given_coinstake(generator, events, alice_acc, blk_3b, tx_0, tx_0_some_output_idx, blk_4b, tx_0_some_output_gindex);
  if (HF4_active)
  {
    CHECK_AND_ASSERT_MES(!r, false, "generate_pos_block_with_given_coinstake not failed as expected");
  }
  else
  {
    CHECK_AND_ASSERT_MES(r, false, "generate_pos_block_with_given_coinstake failed");
    DO_CALLBACK(events, "mark_invalid_block");
    ADD_CUSTOM_EVENT(events, blk_4b);
  }

  //  0      11       21      22      23      33      34      35      36         <- height
  //                                    +------------------+
  //          +-----------------------+ |                  |
  //          |                      tx_0                  |                             tx_0 spends all outputs in blk_1 (main chain)
  // (0 )-   (1 )-...(1r)-   (2 )-   (3 )- ........  (3r)-   (4 )-   (5 )-   (6 )-       <- main chain
  //          ||        \                                  |
  //          ||         \-  (2a)-   #3a#-                 |                             <- alt chain
  //          |+--------------+ \     |                    \                             PoS block 2a uses stake already spent in main chain (okay)
  //          +---------------|-------+                     \                            PoS block 3a uses stake already spent in current alt chain (fail)
  //                          |   \                          \ 
  //                          |    \ ...... (2br)-   (3b)-   #4b#                        <- alt chain
  //                          |                       |
  //                          +-----------------------+                                  PoS block 3b uses as stake an output, created in current alt chain (2a)

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_6), get_block_hash(blk_6)));


  // Final check: switch the chains
  MAKE_NEXT_BLOCK(events, blk_4c, blk_3b, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_5c, blk_4c, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_6c, blk_5c, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_7c, blk_6c, miner_acc);

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_7c), get_block_hash(blk_7c)));
  size_t txs_count = 1;
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", txs_count); // tx_0 should left in the pool

  return true;
}

//------------------------------------------------------------------

pos_minting_tx_packing::pos_minting_tx_packing()
  : m_pos_mint_packing_size(5)
{
  REGISTER_CALLBACK_METHOD(pos_minting_tx_packing, configure_core);
  REGISTER_CALLBACK_METHOD(pos_minting_tx_packing, c1);
}

bool pos_minting_tx_packing::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = 1;
  pc.pos_minimum_heigh = 1;
  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

bool pos_minting_tx_packing::pos_minting_tx_packing::generate(std::vector<test_event_entry>& events) const
{
  bool r = false;
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();

  std::list<account_base> miner_acc_lst(1, miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 5);

  m_alice_start_amount = 10 * CURRENCY_BLOCK_REWARD * m_pos_mint_packing_size;// +TESTS_DEFAULT_FEE;

  // 10 outputs each of (CURRENCY_BLOCK_REWARD * m_pos_mint_packing_size) coins
  
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), m_alice_start_amount, 10, TESTS_DEFAULT_FEE, tx_1);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");

  events.push_back(tx_1);

  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

bool pos_minting_tx_packing::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, m_alice_start_amount, true, UINT64_MAX, m_alice_start_amount), false, "");

  size_t pos_entries_count = 0;

  for (size_t i = 0; i < m_pos_mint_packing_size; ++i)
  {
    r = mine_next_pos_block_in_playtime_with_wallet(*alice_wlt, m_accounts[ALICE_ACC_IDX].get_public_address(), pos_entries_count);
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pos_block_in_playtime_with_wallet failed");
    alice_wlt->refresh();
  }

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, m_alice_start_amount + CURRENCY_BLOCK_REWARD * m_pos_mint_packing_size, true, UINT64_MAX), false, "");

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, WALLET_DEFAULT_TX_SPENDABLE_AGE);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt,
    m_alice_start_amount + CURRENCY_BLOCK_REWARD * m_pos_mint_packing_size, // total
    true,
    UINT64_MAX,
    m_alice_start_amount + CURRENCY_BLOCK_REWARD * m_pos_mint_packing_size // unlocked
  ), false, "");

  alice_wlt->set_defragmentation_tx_settings(true, m_pos_mint_packing_size + 1, m_pos_mint_packing_size + 1); // +1 because previous implementation () had an error with this limit

  // no coinbase tx outputs should be packed
  r = alice_wlt->try_mint_pos();
  CHECK_AND_ASSERT_MES(r, false, "try_mint_pos failed");
  
  // make sure the wallet has only received new locked incoming reward
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt,
    m_alice_start_amount + CURRENCY_BLOCK_REWARD * (m_pos_mint_packing_size + 1), // total
    true,
    UINT64_MAX,
    m_alice_start_amount // unlocked (one output with amount == CURRENCY_BLOCK_REWARD * m_pos_mint_packing_size was spent as stake)
  ), false, "");

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, WALLET_DEFAULT_TX_SPENDABLE_AGE);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");
  
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt,
    m_alice_start_amount + CURRENCY_BLOCK_REWARD * (m_pos_mint_packing_size + 1), // total
    true,
    UINT64_MAX,
    m_alice_start_amount + CURRENCY_BLOCK_REWARD * (m_pos_mint_packing_size + 1) // unlocked
  ), false, "");

  // coinbase tx outputs should be packed now, there's enough coinbase outputs (> m_pos_mint_packing_size)
  r = alice_wlt->try_mint_pos();
  CHECK_AND_ASSERT_MES(r, false, "try_mint_pos failed");
  
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt,
    m_alice_start_amount + CURRENCY_BLOCK_REWARD * (m_pos_mint_packing_size + 2), // total (+1 one block reward)
    true,
    UINT64_MAX,
    // CURRENCY_BLOCK_REWARD * m_pos_mint_packing_size locked for stake
    // CURRENCY_BLOCK_REWARD * (m_pos_mint_packing_size + 1) locked for packing tx
    m_alice_start_amount + CURRENCY_BLOCK_REWARD - CURRENCY_BLOCK_REWARD * (m_pos_mint_packing_size + 1) // unlocked
  ), false, "");

  r = alice_wlt->try_mint_pos();
  CHECK_AND_ASSERT_MES(r, false, "try_mint_pos failed");

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt,
    m_alice_start_amount + CURRENCY_BLOCK_REWARD * (m_pos_mint_packing_size + 3), // total (+1 one block reward)
    true,
    UINT64_MAX,
    // CURRENCY_BLOCK_REWARD * m_pos_mint_packing_size locked for stake
    m_alice_start_amount + CURRENCY_BLOCK_REWARD - CURRENCY_BLOCK_REWARD * (m_pos_mint_packing_size + 1) - CURRENCY_BLOCK_REWARD * m_pos_mint_packing_size // unlocked
  ), false, "");

  return true;
}

