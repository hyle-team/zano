// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "cumulative_diificulty_adjustments_tests.h"

#include "pos_basic_tests.h"

using namespace epee;
using namespace currency;


cumulative_difficulty_adjustment_test::cumulative_difficulty_adjustment_test()
{
  epee::debug::get_set_enable_assert(true, true);
  REGISTER_CALLBACK_METHOD(cumulative_difficulty_adjustment_test, configure_core);
  REGISTER_CALLBACK_METHOD(cumulative_difficulty_adjustment_test, configure_check_height1);
  REGISTER_CALLBACK_METHOD(cumulative_difficulty_adjustment_test, memorize_main_chain);
  REGISTER_CALLBACK_METHOD(cumulative_difficulty_adjustment_test, check_main_chain);
  REGISTER_CALLBACK_METHOD(cumulative_difficulty_adjustment_test, check_reorganize);
  REGISTER_CALLBACK_METHOD(cumulative_difficulty_adjustment_test, remember_block_befor_alt);

}
cumulative_difficulty_adjustment_test::~cumulative_difficulty_adjustment_test()
{
  epee::debug::get_set_enable_assert(true, false);
}
#define FIRST_ALIAS_NAME "first"
#define SECOND_ALIAS_NAME "second"

bool cumulative_difficulty_adjustment_test::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;
  std::list<currency::account_base> coin_stake_sources;

  GENERATE_ACCOUNT(miner_account);
  GENERATE_ACCOUNT(some_account_1);

  coin_stake_sources.push_back(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  MAKE_ACCOUNT(events, first_acc);
  DO_CALLBACK(events, "configure_core");


  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);
  REWIND_BLOCKS_N(events, blk_11, blk_1, miner_account, 15);
  MAKE_NEXT_POS_BLOCK(events, blk_12, blk_11, miner_account, coin_stake_sources);
  //DO_CALLBACK(events, "configure_check_height1");

  MAKE_NEXT_POS_BLOCK(events, blk_27, blk_12, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_28, blk_27, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_29, blk_28, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_30, blk_29, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_31, blk_30, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_32, blk_31, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_33, blk_32, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_34, blk_33, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_35, blk_34, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_36, blk_35, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_37, blk_36, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_38, blk_37, miner_account, coin_stake_sources);
  MAKE_NEXT_POS_BLOCK(events, blk_39, blk_38, miner_account, coin_stake_sources);
  DO_CALLBACK(events, "memorize_main_chain");
  MAKE_NEXT_BLOCK(events, blk_39_pow, blk_38, miner_account);
  DO_CALLBACK(events, "check_main_chain");
  MAKE_NEXT_BLOCK(events, blk_40_pow, blk_39_pow, miner_account);
  MAKE_NEXT_BLOCK(events, blk_41_pow, blk_40_pow, miner_account);
  MAKE_NEXT_BLOCK(events, blk_42_pow, blk_41_pow, miner_account);
  MAKE_NEXT_POS_BLOCK(events, blk_43, blk_42_pow, miner_account, coin_stake_sources);
  DO_CALLBACK(events, "remember_block_befor_alt");
  //start alt chain
  MAKE_NEXT_BLOCK(events, blk_40_new_pow, blk_39_pow, miner_account);
  MAKE_NEXT_POS_BLOCK(events, blk_41_new_pos, blk_40_new_pow, miner_account, coin_stake_sources);
  MAKE_NEXT_BLOCK(events, blk_42_new_pow, blk_41_new_pos, miner_account);
  MAKE_NEXT_POS_BLOCK(events, blk_43_new_pos, blk_42_new_pow, miner_account, coin_stake_sources);
  MAKE_NEXT_BLOCK(events, blk_44_new_pow, blk_43_new_pos, miner_account);
  MAKE_NEXT_POS_BLOCK(events, blk_45_new_pos, blk_44_new_pow, miner_account, coin_stake_sources);
  //check reorganize
  DO_CALLBACK(events, "check_reorganize");

  return true;
}

bool cumulative_difficulty_adjustment_test::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = get_default_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE; //four blocks
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH; //four blocks

  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}
bool cumulative_difficulty_adjustment_test::configure_check_height1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  uint64_t h = c.get_current_blockchain_size();  
  CHECK_EQ(h, 27);
  return true;
}
bool cumulative_difficulty_adjustment_test::memorize_main_chain(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::block b_from_main = boost::get<currency::block>(events[ev_index - 1]);

  bool r = c.get_blockchain_storage().get_block_extended_info_by_hash(get_block_hash(b_from_main), bei);
  CHECK_EQ(r, true);

  return true;
}
bool cumulative_difficulty_adjustment_test::check_main_chain(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{

  currency::block_extended_info bei_2;
  bool r = c.get_blockchain_storage().get_block_extended_info_by_hash(get_block_hash(bei.bl), bei_2);
  CHECK_AND_ASSERT_MES(r, false, "get_block_extended_info_by_hash failed");

  CHECK_EQ(bei.cumulative_diff_adjusted, bei_2.cumulative_diff_adjusted);
  return true;
}
bool cumulative_difficulty_adjustment_test::remember_block_befor_alt(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::block b_from_main = boost::get<currency::block>(events[ev_index - 1]);
  bool r = c.get_blockchain_storage().get_block_extended_info_by_hash(get_block_hash(b_from_main), bei);
  CHECK_EQ(r, true);

  return true;
}
bool cumulative_difficulty_adjustment_test::check_reorganize(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  //check reorganize happend
  crypto::hash top_id = c.get_blockchain_storage().get_top_block_id();

  currency::block top_bl = boost::get<currency::block>(events[ev_index - 1]);
  crypto::hash event_reorganized_id = get_block_hash(top_bl);
  CHECK_EQ(top_id, event_reorganized_id);


  currency::block_extended_info bei_2;
  bool r = c.get_blockchain_storage().get_block_extended_info_by_hash(get_block_hash(bei.bl), bei_2);
  CHECK_AND_ASSERT_MES(r, false, "get_block_extended_info_by_hash failed");
  CHECK_EQ(bei.stake_hash, bei_2.stake_hash);
  //CHECK_EQ(bei.already_generated_coins, bei_2.already_generated_coins);
  //CHECK_EQ(bei.block_cumulative_size, bei_2.block_cumulative_size);
  CHECK_EQ(bei.cumulative_diff_adjusted, bei_2.cumulative_diff_adjusted);
  CHECK_EQ(bei.cumulative_diff_precise, bei_2.cumulative_diff_precise);
  CHECK_EQ(bei.difficulty, bei_2.difficulty);
  CHECK_EQ(bei.height, bei_2.height);

  return true;
}

//==================================================================================================
cumulative_difficulty_adjustment_test_alt::cumulative_difficulty_adjustment_test_alt()
{
  //REGISTER_CALLBACK_METHOD(cumulative_difficulty_adjustment_test, configure_core);
  REGISTER_CALLBACK_METHOD(cumulative_difficulty_adjustment_test_alt, configure_check_height1);
  //REGISTER_CALLBACK_METHOD(cumulative_difficulty_adjustment_test, memorize_main_chain);
  //REGISTER_CALLBACK_METHOD(cumulative_difficulty_adjustment_test, check_main_chain);
}
#define FIRST_ALIAS_NAME "first"
#define SECOND_ALIAS_NAME "second"

bool cumulative_difficulty_adjustment_test_alt::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS_N(events, blk_41, blk_0, miner_account, 40);

  REWIND_BLOCKS_N(events, blk_41_a, blk_0, miner_account, 40);
  MAKE_NEXT_BLOCK(events, blk_42, blk_41_a, miner_account);
  DO_CALLBACK(events, "configure_check_height1");
  return true;
}

bool cumulative_difficulty_adjustment_test_alt::configure_check_height1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  crypto::hash h_top = c.get_blockchain_storage().get_top_block_id();
  crypto::hash h_reorg = currency::get_block_hash(boost::get<currency::block>(events[events.size() - 2]));

  CHECK_EQ(h_reorg, h_top);
  return true;
}
// bool cumulative_difficulty_adjustment_test_alt::memorize_main_chain(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
// {
//   currency::block b_from_main = boost::get<currency::block>(events[ev_index - 1]);
// 
//   bool r = c.get_blockchain_storage().get_block_extended_info_by_hash(get_block_hash(b_from_main), bei);
//   CHECK_EQ(r, true);
// 
//   return true;
// }
// bool cumulative_difficulty_adjustment_test_alt::check_main_chain(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
// {
// 
//   currency::block_extended_info bei_2;
//   bool r = c.get_blockchain_storage().get_block_extended_info_by_hash(get_block_hash(bei.bl), bei_2);
// 
//   //r = is_pos_block(b);
//   CHECK_EQ(bei.cumulative_diff_adjusted, bei_2.cumulative_diff_adjusted);
//   return true;
// }
