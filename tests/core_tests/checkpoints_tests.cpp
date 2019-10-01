// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "checkpoints_tests.h"
#include "tx_builder.h"
#include "pos_block_builder.h"
#include "offers_helper.h"

using namespace epee;
using namespace crypto;
using namespace currency;

checkpoints_test::checkpoints_test()
{
  REGISTER_CALLBACK_METHOD(checkpoints_test, set_checkpoint);
  REGISTER_CALLBACK_METHOD(checkpoints_test, set_far_checkpoint);
  REGISTER_CALLBACK_METHOD(checkpoints_test, check_being_in_cp_zone);
  REGISTER_CALLBACK_METHOD(checkpoints_test, check_not_being_in_cp_zone);
}

bool checkpoints_test::set_checkpoint(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  params_checkpoint pcp = AUTO_VAL_INIT(pcp);
  bool r = epee::string_tools::hex_to_pod(boost::get<callback_entry>(events[ev_index]).callback_params, pcp);
  CHECK_AND_ASSERT_MES(r, false, "Can't obtain event params");

  for (auto e : events)
  {
    if (e.type() != typeid(currency::block))
      continue;
    const block& b = boost::get<currency::block>(e);
    if (currency::get_block_height(b) == pcp.height)
    {
      if (pcp.hash != null_hash && pcp.hash != get_block_hash(b))
        continue;
      currency::checkpoints cp;
      cp.add_checkpoint(currency::get_block_height(b), epee::string_tools::pod_to_hex(currency::get_block_hash(b)));
      c.set_checkpoints(std::move(cp));
      return true;
    }
  }

  return false;
}

bool checkpoints_test::check_being_in_cp_zone(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  CHECK_AND_ASSERT_MES(c.get_blockchain_storage().is_in_checkpoint_zone(), false, "Blockstorage is not in checkpoint zone.");
  return true;
}

bool checkpoints_test::check_not_being_in_cp_zone(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  CHECK_AND_ASSERT_MES(!c.get_blockchain_storage().is_in_checkpoint_zone(), false, "Blockstorage IS in checkpoint zone.");
  return true;
}

bool checkpoints_test::set_far_checkpoint(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::checkpoints cp;
  cp.add_checkpoint(100, "c0ffee0fdeadbeefc0ffeeeec0ffeec0ffeec0ffeec0ffeec0ffeec0ffeec0ff");
  c.set_checkpoints(std::move(cp));
  return true;
}

//------------------------------------------------------------------------------

gen_checkpoints_attachments_basic::gen_checkpoints_attachments_basic()
{
  REGISTER_CALLBACK_METHOD(gen_checkpoints_attachments_basic, check_tx);
}

bool gen_checkpoints_attachments_basic::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1450000000;
  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts_start);
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  //  0 ... N     N+1   N+2   <- height (N = CURRENCY_MINED_MONEY_UNLOCK_WINDOW)
  // (0 )- (0r)- (1 )- (2 )

  DO_CALLBACK_PARAMS(events, "set_checkpoint", params_checkpoint(CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2));

  tx_comment cm;
  cm.comment = "This is tx comment.";
  std::string ms;
  ms = "This is tx message.";

  std::vector<currency::attachment_v> attachments;
  attachments.push_back(cm);
  attachments.push_back(ms);

  MAKE_TX_LIST_START_WITH_ATTACHS(events, txs, miner_acc, miner_acc, MK_TEST_COINS(1), blk_0r, attachments);
  m_tx_hash = get_transaction_hash(txs.front());
  
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, txs);

  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);

  DO_CALLBACK(events, "check_tx");
  return true;
}

bool gen_checkpoints_attachments_basic::check_tx(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  transaction tx;
  bool r = c.get_transaction(m_tx_hash, tx);
  CHECK_AND_ASSERT_MES(r, false, "can't get transaction");

  CHECK_AND_ASSERT_MES(tx.signatures.empty(), false, "tx has non-empty sig");
  CHECK_AND_ASSERT_MES(tx.attachment.empty(), false, "tx has non-empty attachments");
  return true;
}

//------------------------------------------------------------------------------

gen_checkpoints_invalid_keyimage::gen_checkpoints_invalid_keyimage()
{
  REGISTER_CALLBACK_METHOD(gen_checkpoints_invalid_keyimage, check);
}

bool gen_checkpoints_invalid_keyimage::generate(std::vector<test_event_entry>& events) const
{
  // 1. Set checkpoint on height 100.
  // 2. Add tx with invalid key image and signature.
  // 3. Put it into a block, add the block.
  // 4. Make sure block is accepted by the blockstorage.

  uint64_t ts_start = 1450000000;
  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts_start);
  DO_CALLBACK(events, "set_far_checkpoint"); // set the checkpoint to the future
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  bool r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc, miner_acc, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");  
  tx_builder tb;
  tb.step1_init();
  tb.step2_fill_inputs(miner_acc.get_keys(), sources);
  crypto::key_image& ki = boost::get<txin_to_key>(tb.m_tx.vin[0]).k_image;
  *reinterpret_cast<char*>(&ki) = '\xFF'; // invalidate key image
  tb.step3_fill_outputs(destinations);
  tb.step4_calc_hash();
  // don't sign at all
  // tb.step5_sign();

  events.push_back(tb.m_tx);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tb.m_tx);

  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);

  DO_CALLBACK(events, "check");
  
  return true;
}

bool gen_checkpoints_invalid_keyimage::check(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  CHECK_AND_ASSERT_MES(c.get_current_blockchain_size() == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3, false, "Invalid blockchain size");
  return true;
}

//------------------------------------------------------------------------------

bool gen_checkpoints_altblock_before_and_after_cp::generate(std::vector<test_event_entry>& events) const
{
  // Make sure an altblock can't be added before CP when the blockstorage has already passed it by

  uint64_t ts_start = 1450000000;
  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts_start);
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  //  0 ... N    N+1   N+2   N+3    <- height (N = CURRENCY_MINED_MONEY_UNLOCK_WINDOW)
  //                   >CP<         (checkpoint)
  // (0 )- (0r)- (1 )- (2 )- (3 )   <- main chain
  //        \      \      \- (3a)   OK
  //         \      \- (2a)         must be rejected
  //          \- (1a)               must be rejected

  DO_CALLBACK_PARAMS(events, "set_checkpoint", params_checkpoint(CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2));

  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);
  
  DO_CALLBACK(events, "check_being_in_cp_zone"); // make sure CP is passed

  // try to split the chain before checkpoint - block should rejected
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK(events, blk_1a, blk_0r, miner_acc);

  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK(events, blk_2a, blk_1, miner_acc);

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_2), get_block_hash(blk_2)));

  // splitting after the checkpoint should be ok
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_3a, blk_2, miner_acc);

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_3), get_block_hash(blk_3)));

  return true;
}

//------------------------------------------------------------------------------

bool gen_checkpoints_block_in_future::generate(std::vector<test_event_entry>& events) const
{
  // Brief idea: (checkpoint) - (blockchain last block) - (new block)
  // make sure adding new block from "future" is not affected by CPs in the past

  uint64_t ts_start = 1450000000;
  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts_start);
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  //  0 ... N    N+1   N+2   N+3  N+4    <- height (N = CURRENCY_MINED_MONEY_UNLOCK_WINDOW)
  //             >CP<                    (checkpoint)
  // (0 )- (0r)- (1 )- ...- (3 )- (4 )   <- main chain
  //               \        /
  //                \-(2 )-/   (delayed block)    

  DO_CALLBACK_PARAMS(events, "set_checkpoint", params_checkpoint(CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1));

  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, miner_acc);
  DO_CALLBACK(events, "check_being_in_cp_zone"); // make sure CP is passed

  // construct blk_2, but don't push it to the blockstorage
  currency::block blk_2 = AUTO_VAL_INIT(blk_2);
  generator.construct_block(events, blk_2, blk_1, miner_acc);
  events.push_back(event_special_block(blk_2, event_special_block::flag_skip));

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_1), get_block_hash(blk_1)));

  // make next two blocks - they should be rejected as orphans
  DO_CALLBACK(events, "mark_orphan_block");
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);

  DO_CALLBACK(events, "mark_orphan_block");
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_acc);

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_1), get_block_hash(blk_1)));

  // re-add block 2, 3 and 4; should be accepted now
  events.push_back(blk_2);
  events.push_back(blk_3);
  events.push_back(blk_4);

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_4), get_block_hash(blk_4)));
  return true;
}

//------------------------------------------------------------------------------

bool gen_checkpoints_altchain_far_before_cp::generate(std::vector<test_event_entry>& events) const
{
  // Make sure altblocks far before the CP are treated like there's no CP

  uint64_t ts_start = 1450000000;
  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts_start);
  DO_CALLBACK(events, "set_far_checkpoint");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  //  0 ... N     N+1   N+2   N+3   N+4   N+5   N+6   <- blockchain height (N = CURRENCY_MINED_MONEY_UNLOCK_WINDOW)
  // (0 )..(0r)- (1 )- (2 )
  //          \- (1a)- (2a)- (3a)  ....  (5 )- (6 )
  //                            \- (4 )-/             <- delayed block

  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);
  
  // split the chain
  MAKE_NEXT_BLOCK(events, blk_1a, blk_0r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_2a, blk_1a, miner_acc);

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_2), get_block_hash(blk_2)));

  // trigger reorganize
  MAKE_NEXT_BLOCK(events, blk_3a, blk_2a, miner_acc); // this will cause reorganize

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_3a), get_block_hash(blk_3a)));

  currency::block blk_4 = AUTO_VAL_INIT(blk_4);
  generator.construct_block(events, blk_4, blk_3a, miner_acc);
  events.push_back(event_special_block(blk_4, event_special_block::flag_skip));

  DO_CALLBACK(events, "mark_orphan_block");
  MAKE_NEXT_BLOCK(events, blk_5, blk_4, miner_acc);
  DO_CALLBACK(events, "mark_orphan_block");
  MAKE_NEXT_BLOCK(events, blk_6, blk_5, miner_acc);

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_3a), get_block_hash(blk_3a)));

  events.push_back(blk_4);
  events.push_back(blk_5);
  events.push_back(blk_6);

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_6), get_block_hash(blk_6)));

  return true;
}

//------------------------------------------------------------------------------

bool gen_checkpoints_block_in_future_after_cp::generate(std::vector<test_event_entry>& events) const
{
  // Brief idea: (blockchain last block) - (checkpoint) - (new block)
  // make sure adding new block from "future" is not affected by CPs in the past

  uint64_t ts_start = 1450000000;
  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts_start);
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  //  0 ... N    N+1   N+2   N+3  N+4    <- height (N = CURRENCY_MINED_MONEY_UNLOCK_WINDOW)
  //                        >CP<         (checkpoint)
  // (0 )- (0r)- (1 )- ........ -(4 )    <- main chain
  //               \              /
  //                \-(2 )- (3 )-/       (delayed blocks)    

  DO_CALLBACK(events, "check_not_being_in_cp_zone");
  DO_CALLBACK_PARAMS(events, "set_checkpoint", params_checkpoint(CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3));

  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, miner_acc);

  // construct blk_2 and blk_3, but don't push it to the blockstorage
  currency::block blk_2 = AUTO_VAL_INIT(blk_2);
  generator.construct_block(events, blk_2, blk_1, miner_acc);
  events.push_back(event_special_block(blk_2, event_special_block::flag_skip));

  currency::block blk_3 = AUTO_VAL_INIT(blk_3);
  generator.construct_block(events, blk_3, blk_2, miner_acc);
  events.push_back(event_special_block(blk_3, event_special_block::flag_skip));

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_1), get_block_hash(blk_1)));

  // make blk_4 - should be rejected as orphans
  DO_CALLBACK(events, "mark_orphan_block");
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_acc);

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_1), get_block_hash(blk_1)));

  // re-add block 2, 3 and 4; should be accepted now
  DO_CALLBACK(events, "check_being_in_cp_zone");
  events.push_back(blk_2);
  events.push_back(blk_3);
  DO_CALLBACK(events, "check_being_in_cp_zone");
  events.push_back(blk_4);
  DO_CALLBACK(events, "check_not_being_in_cp_zone");

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_4), get_block_hash(blk_4)));
  return true;
}

//------------------------------------------------------------------------------

gen_checkpoints_prun_txs_after_blockchain_load::gen_checkpoints_prun_txs_after_blockchain_load()
{
  REGISTER_CALLBACK_METHOD(gen_checkpoints_prun_txs_after_blockchain_load, check_txs);
}

bool gen_checkpoints_prun_txs_after_blockchain_load::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = 1450000000;

  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  //  0 ... N     N+1   N+2   N+3   N+4   N+5   N+6    <- height (N = CURRENCY_MINED_MONEY_UNLOCK_WINDOW)
  //           +------->CP1      +------->CP2          <- checkpoints
  //           |                 |                     <- when CP are set up
  // (0 )- (0r)- (1 )- (2 )- (3 )- (4 )- (5 )- (6 )    <- main chain
  //             tx_0        tx_1                      <- txs included in blocks
  //
  // Expected: tx_0 and tx_1 are both pruned

  DO_CALLBACK(events, "check_not_being_in_cp_zone");
  DO_CALLBACK_PARAMS(events, "set_checkpoint", params_checkpoint(CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2));

  MAKE_TX(events, tx_0, miner_acc, alice, MK_TEST_COINS(1), blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);
  
  DO_CALLBACK(events, "check_being_in_cp_zone");
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);

  MAKE_TX(events, tx_1, miner_acc, alice, MK_TEST_COINS(1), blk_2);
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_1);

  DO_CALLBACK(events, "check_not_being_in_cp_zone");
  DO_CALLBACK_PARAMS(events, "set_checkpoint", params_checkpoint(CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 5));

  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_5, blk_4, miner_acc);
  DO_CALLBACK(events, "check_being_in_cp_zone");
  MAKE_NEXT_BLOCK(events, blk_6, blk_5, miner_acc);
  DO_CALLBACK(events, "check_not_being_in_cp_zone");

  m_tx0_id = get_transaction_hash(tx_0);
  m_tx1_id = get_transaction_hash(tx_1);
  DO_CALLBACK(events, "check_txs");

  return true;
}

bool gen_checkpoints_prun_txs_after_blockchain_load::check_txs(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  transaction tx_0, tx_1;
  bool r = c.get_transaction(m_tx0_id, tx_0);
  CHECK_AND_ASSERT_MES(r, false, "can't get transaction tx_0");
  CHECK_AND_ASSERT_MES(tx_0.signatures.empty(), false, "tx_0 has non-empty sig");
  CHECK_AND_ASSERT_MES(tx_0.attachment.empty(), false, "tx_0 has non-empty attachments");

  r = c.get_transaction(m_tx1_id, tx_1);
  CHECK_AND_ASSERT_MES(r, false, "can't get transaction tx_1");
  CHECK_AND_ASSERT_MES(tx_1.signatures.empty(), false, "tx_1 has non-empty sig");
  CHECK_AND_ASSERT_MES(tx_1.attachment.empty(), false, "tx_1 has non-empty attachments");

  return true;
}

//------------------------------------------------------------------------------

bool gen_checkpoints_reorganize::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = 1450000000;
  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "check_not_being_in_cp_zone");
  DO_CALLBACK_PARAMS(events, "set_checkpoint", params_checkpoint(3));

  //  0     1     2     3     4     <- height
  //     +--------------CP          <- checkpoints
  //     |                          <- when CP is set up
  // (0 )-                          <- main chain

  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);
  DO_CALLBACK(events, "check_being_in_cp_zone");

  //  0     1     2     3     4     <- height
  //     +--------------CP          <- checkpoints
  //     |                          <- when CP is set up
  // (0 )- (1 )- (2 )-              <- main chain

  // split the chain
  MAKE_NEXT_BLOCK(events, blk_2a, blk_1, miner_acc);

  //  0     1     2     3     4     <- height
  //     +--------------CP          <- checkpoints
  //     |                          <- when CP is set up
  // (0 )- (1 )- (2 )-              <- main chain
  //          \- (2a)-              <- alt chain

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_2), get_block_hash(blk_2)));

  // let's switch the chain!
  MAKE_NEXT_BLOCK(events, blk_3a, blk_2a, miner_acc);

  //  0     1     2     3     4     <- height
  //     +--------------CP          <- checkpoints
  //     |                          <- when CP is set up
  // (0 )- (1 )- (2a)- (3a)-        <- main chain
  //          \- (2 )-              <- alt chain

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_3a), get_block_hash(blk_3a)));
  return true;
}

//------------------------------------------------------------------------------

gen_checkpoints_pos_validation_on_altchain::gen_checkpoints_pos_validation_on_altchain()
{
  REGISTER_CALLBACK_METHOD(gen_checkpoints_pos_validation_on_altchain, init_runtime_config);
}

bool gen_checkpoints_pos_validation_on_altchain::generate(std::vector<test_event_entry>& events) const
{
  // This test is an exploit actually.
  // Outline:
  // 1. Have a blockchain with a CP, that is already passed by main chain.
  // 2. Make an altblock (0a), following one of pre-CP blocks, but with specifed height > CP.
  // 3. Put this altblock into the core. The block is rejected, but blockchain storage is switching into checkpoint zone mode.
  // 4. Make a PoS block (2) with invalid signature and push it to the core. As checkpoint zone is activated previously, this block is accepted with no sig checks.
  // 5. ???
  // 6. PROFIT!

  GENERATE_ACCOUNT(miner_acc);
  std::list<currency::account_base> coin_stake_sources;
  coin_stake_sources.push_back(miner_acc);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "check_not_being_in_cp_zone");
  DO_CALLBACK_PARAMS(events, "set_checkpoint", params_checkpoint(CURRENCY_MINED_MONEY_UNLOCK_WINDOW));
  DO_CALLBACK(events, "init_runtime_config");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW - 1);

  //  0     1 ... 9     10    11    12   <- height
  //     +--------------CP               <- checkpoints
  //     |                               <- when CP is set up
  // (0 )- ..... (0r)- (1 )- (2 )        <- main chain
  //    \- {0a}              {0a}        <- alt chain (0a has specified height 11, but follows block 0)

  // pass the CP by main chain
  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, miner_acc);


  // prepare a block for altchain (following blk_1a but with post-CP height)
  // it's not necessary to be a PoS, but here is using PoS just because of convenient pos_block_builder 
  block blk_0a;
  {
    crypto::hash prev_id = get_block_hash(blk_0);
    size_t height = get_block_height(blk_1) + 1; // note the incorrect height used
    currency::wide_difficulty_type diff = generator.get_difficulty_for_next_block(prev_id, false);

    const transaction& stake = blk_0.miner_tx;
    crypto::public_key stake_tx_pub_key = get_tx_pub_key_from_extra(stake);
    size_t stake_output_idx = 0;
    size_t stake_output_gidx = 0;
    uint64_t stake_output_amount = stake.vout[stake_output_idx].amount;
    crypto::key_image stake_output_key_image;
    keypair kp;
    generate_key_image_helper(miner_acc.get_keys(), stake_tx_pub_key, stake_output_idx, kp, stake_output_key_image);
    crypto::public_key stake_output_pubkey = boost::get<txout_to_key>(stake.vout[stake_output_idx].target).key;

    pos_block_builder pb;
    pb.step1_init_header(height, prev_id);
    pb.step2_set_txs(std::vector<transaction>());
    pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, diff, prev_id, null_hash, blk_0r.timestamp);
    pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(blk_0r), miner_acc.get_public_address());
    pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, miner_acc);
    blk_0a = pb.m_block;
  }

  DO_CALLBACK(events, "mark_invalid_block");
  events.push_back(blk_0a);

  // try to put incorrect PoS (with wrong sig)
  block blk_2;
  {
    crypto::hash prev_id = get_block_hash(blk_1);
    size_t height = get_block_height(blk_1) + 1;
    currency::wide_difficulty_type diff = generator.get_difficulty_for_next_block(prev_id, false);

    const transaction& stake = blk_0.miner_tx;
    crypto::public_key stake_tx_pub_key = get_tx_pub_key_from_extra(stake);
    size_t stake_output_idx = 0;
    size_t stake_output_gidx = 0;
    uint64_t stake_output_amount = stake.vout[stake_output_idx].amount;
    crypto::key_image stake_output_key_image;
    keypair kp;
    generate_key_image_helper(miner_acc.get_keys(), stake_tx_pub_key, stake_output_idx, kp, stake_output_key_image);
    //crypto::public_key stake_output_pubkey = boost::get<txout_to_key>(stake.vout[stake_output_idx].target).key;

    pos_block_builder pb;
    pb.step1_init_header(height, prev_id);
    pb.step2_set_txs(std::vector<transaction>());
    pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, diff, prev_id, null_hash, blk_0r.timestamp);
    pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(blk_0r), miner_acc.get_public_address());
    
    // don't sign at all
    //pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, miner_acc);
    pb.m_block.miner_tx.signatures[0].clear(); // just to make sure

    blk_2 = pb.m_block;
  }

  // this block should be rejected normally
  DO_CALLBACK(events, "mark_invalid_block");
  events.push_back(blk_2);

  return true;
}

bool gen_checkpoints_pos_validation_on_altchain::init_runtime_config(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  core_runtime_config crc = c.get_blockchain_storage().get_core_runtime_config();
  crc.pos_minimum_heigh = 1;
  c.get_blockchain_storage().set_core_runtime_config(crc);

  return true;
}

//------------------------------------------------------------------------------

gen_no_attchments_in_coinbase::gen_no_attchments_in_coinbase()
{
  // NOTE: This test is made deterministic to be able to correctly set up checkpoint.
  random_state_test_restorer::reset_random(); // random generator's state was previously stored, will be restore on dtor (see also m_random_state_test_restorer)

  bool r = m_miner_acc.restore_keys_from_braindata("battle harsh arrow gain best doubt nose raw protect salty apart tell distant final yeah stubborn true stop shoulder breathe throne problem everyone stranger only");
  CHECK_AND_ASSERT_THROW_MES(r, "gen_no_attchments_in_coinbase: Can't restore account from braindata");

  REGISTER_CALLBACK_METHOD(gen_no_attchments_in_coinbase, c1);
  REGISTER_CALLBACK_METHOD(gen_no_attchments_in_coinbase, init_config_set_cp);
}

bool gen_no_attchments_in_coinbase::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = 1450000000;
  test_core_time::adjust(ts);
  
  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, m_miner_acc, ts);
  events.push_back(blk_0);

  DO_CALLBACK(events, "check_not_being_in_cp_zone");
  DO_CALLBACK(events, "init_config_set_cp");

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, m_miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK(events, "c1");
  return true;
}

bool gen_no_attchments_in_coinbase::init_config_set_cp(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  core_runtime_config crc = c.get_blockchain_storage().get_core_runtime_config();
  crc.pos_minimum_heigh = 1;
  c.get_blockchain_storage().set_core_runtime_config(crc);

  m_checkpoints.add_checkpoint(12, "2a6e13df811eccce121c0de4dbdcc640de1d37c8459c2c8ea02af39717779836");
  c.set_checkpoints(currency::checkpoints(m_checkpoints));

  return true;
}

bool gen_no_attchments_in_coinbase::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // Test outline:
  // 0. Prepare determenistic environment (fixes acount, fixes timestamps, reset random generator), mine some block and set CP.
  // 1. Create PoS block with attachments (offer) in miner-tx, make sure it is rejected.
  // 2. Do the same, but construct block for altchain.
  // 3. Mine some PoW-blocks to pass the CP.
  // 4. Repeat steps 1-2 to make sure it's ok after a CP.

  block blk_0 = boost::get<block>(events[0]);
  block blk_0r = boost::get<block>(events[ev_index - 1]);
  test_core_time::adjust(blk_0r.timestamp + DIFFICULTY_TOTAL_TARGET / 2);

  bc_services::offer_details od = AUTO_VAL_INIT(od);
  fill_test_offer(od);

  // make a block with attachments in its coinbase
  block blk_1;
  bool r = mine_next_pos_block_in_playtime_sign_cb(c, blk_0r, blk_0, m_miner_acc, [&od](block& b){
    bc_services::put_offer_into_attachment(od, b.miner_tx.attachment);
    add_attachments_info_to_extra(b.miner_tx.extra, b.miner_tx.attachment);
    return true;
  }, blk_1);

  // blk_1 should be rejected as coinbase has non-empty attachment
  currency::block_verification_context bvc = AUTO_VAL_INIT(bvc);
  c.handle_incoming_block(t_serializable_object_to_blob(blk_1), bvc);
  CHECK_AND_NO_ASSERT_MES(bvc.m_verification_failed && !bvc.m_marked_as_orphaned && !bvc.m_already_exists && !bvc.m_added_to_main_chain, false, "block verification context check NOT failed");

  // try to add alt-block with attachments in coinbase
  block blk_2;
  r = mine_next_pos_block_in_playtime_sign_cb(c, blk_0, blk_0, m_miner_acc, [&od](block& b){
    bc_services::put_offer_into_attachment(od, b.miner_tx.attachment);
    add_attachments_info_to_extra(b.miner_tx.extra, b.miner_tx.attachment);
    return true;
  }, blk_2);

  // blk_2 should be rejected as well
  bvc = AUTO_VAL_INIT(bvc);
  c.handle_incoming_block(t_serializable_object_to_blob(blk_2), bvc);
  CHECK_AND_NO_ASSERT_MES(bvc.m_verification_failed && !bvc.m_marked_as_orphaned && !bvc.m_already_exists && !bvc.m_added_to_main_chain, false, "block verification context check NOT failed");

  // mine a couple of PoW blocks to pass the checkpoint

  CHECK_AND_ASSERT_MES(c.get_blockchain_storage().is_in_checkpoint_zone(), false, "Blockchain is not in checkpoint zone");

  test_core_time::adjust(blk_0r.timestamp + DIFFICULTY_TOTAL_TARGET);

  block blk_a;
  r = mine_next_pow_block_in_playtime(m_miner_acc.get_public_address(), c, &blk_a);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  test_core_time::adjust(blk_a.timestamp + DIFFICULTY_TOTAL_TARGET);

  block blk_b;
  r = mine_next_pow_block_in_playtime(m_miner_acc.get_public_address(), c, &blk_b);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  test_core_time::adjust(blk_b.timestamp + DIFFICULTY_TOTAL_TARGET);

  block blk_c;
  r = mine_next_pow_block_in_playtime(m_miner_acc.get_public_address(), c, &blk_c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  // make sure the checkpoint zone is successfully left behind
  CHECK_AND_ASSERT_MES(!c.get_blockchain_storage().is_in_checkpoint_zone(), false, "Blockchain IS in checkpoint zone");

  // re-do the checks above

  // make a block with attachments in its coinbase
  block blk_3;
  r = mine_next_pos_block_in_playtime_sign_cb(c, blk_c, blk_0, m_miner_acc, [&od](block& b){
    bc_services::put_offer_into_attachment(od, b.miner_tx.attachment);
    add_attachments_info_to_extra(b.miner_tx.extra, b.miner_tx.attachment);
    return true;
  }, blk_3);

  // blk_3 should be rejected as coinbase has non-empty attachment
  bvc = AUTO_VAL_INIT(bvc);
  c.handle_incoming_block(t_serializable_object_to_blob(blk_3), bvc);
  CHECK_AND_NO_ASSERT_MES(bvc.m_verification_failed && !bvc.m_marked_as_orphaned && !bvc.m_already_exists && !bvc.m_added_to_main_chain, false, "block verification context check NOT failed");

  // try to add alt-block with attachments in coinbase
  block blk_4;
  r = mine_next_pos_block_in_playtime_sign_cb(c, blk_0, blk_0, m_miner_acc, [&od](block& b){
    bc_services::put_offer_into_attachment(od, b.miner_tx.attachment);
    add_attachments_info_to_extra(b.miner_tx.extra, b.miner_tx.attachment);
    return true;
  }, blk_4);

  // blk_4 should be rejected as well
  bvc = AUTO_VAL_INIT(bvc);
  c.handle_incoming_block(t_serializable_object_to_blob(blk_4), bvc);
  CHECK_AND_NO_ASSERT_MES(bvc.m_verification_failed && !bvc.m_marked_as_orphaned && !bvc.m_already_exists && !bvc.m_added_to_main_chain, false, "block verification context check NOT failed");

  return true;
}

//------------------------------------------------------------------------------

bool gen_no_attchments_in_coinbase_gentime::generate(std::vector<test_event_entry>& events) const
{
  // Test outline.
  // The same as gen_no_attchments_in_coinbase, but uses PoW blocks rather than PoS AND do all the stuff in gentime.

  bool r = false;
  GENERATE_ACCOUNT(miner_acc);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK_PARAMS(events, "set_checkpoint", params_checkpoint(2)); // set CP to height 2

  bc_services::offer_details od = AUTO_VAL_INIT(od);
  fill_test_offer(od);

  // Part I. Checks in CP-zone

  // basic check - add to main chain
  block blk_a = AUTO_VAL_INIT(blk_a);
  r = generator.construct_block_gentime_with_coinbase_cb(blk_0, miner_acc, [&od](transaction& tx){
    bc_services::put_offer_into_attachment(od, tx.attachment);
    bool r = add_attachments_info_to_extra(tx.extra, tx.attachment);
    CHECK_AND_ASSERT_MES(r, false, "add_attachments_info_to_extra failed");
    return true;
  }, blk_a);
  CHECK_AND_ASSERT_MES(r, false, "construct_block_gentime_with_coinbase_cb failed");
  DO_CALLBACK(events, "mark_invalid_block");
  events.push_back(blk_a);


  DO_CALLBACK(events, "check_being_in_cp_zone");
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_acc);

  // make sure we can't add PoW block with tx to altchain
  block blk_b = AUTO_VAL_INIT(blk_b);
  r = generator.construct_block_gentime_with_coinbase_cb(blk_0, miner_acc, [&od](transaction& tx){
    bc_services::put_offer_into_attachment(od, tx.attachment);
    bool r = add_attachments_info_to_extra(tx.extra, tx.attachment);
    CHECK_AND_ASSERT_MES(r, false, "add_attachments_info_to_extra failed");
    return true;
  }, blk_b);
  CHECK_AND_ASSERT_MES(r, false, "construct_block_gentime_with_coinbase_cb failed");
  DO_CALLBACK(events, "mark_invalid_block");
  events.push_back(blk_b);

  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc); // here we should pass the CP
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);
  DO_CALLBACK(events, "check_not_being_in_cp_zone");


  // Part II. The same checks outside of CP-zone

  // try to add to main chain
  block blk_c = AUTO_VAL_INIT(blk_c);
  r = generator.construct_block_gentime_with_coinbase_cb(blk_3, miner_acc, [&od](transaction& tx){
    bc_services::put_offer_into_attachment(od, tx.attachment);
    bool r = add_attachments_info_to_extra(tx.extra, tx.attachment);
    CHECK_AND_ASSERT_MES(r, false, "add_attachments_info_to_extra failed");
    return true;
  }, blk_c);
  CHECK_AND_ASSERT_MES(r, false, "construct_block_gentime_with_coinbase_cb failed");
  DO_CALLBACK(events, "mark_invalid_block");
  events.push_back(blk_c);

  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_5, blk_4, miner_acc);

  // .. and to alt chain
  block blk_d = AUTO_VAL_INIT(blk_d);
  r = generator.construct_block_gentime_with_coinbase_cb(blk_3, miner_acc, [&od](transaction& tx){
    bc_services::put_offer_into_attachment(od, tx.attachment);
    bool r = add_attachments_info_to_extra(tx.extra, tx.attachment);
    CHECK_AND_ASSERT_MES(r, false, "add_attachments_info_to_extra failed");
    return true;
  }, blk_d);
  CHECK_AND_ASSERT_MES(r, false, "construct_block_gentime_with_coinbase_cb failed");
  DO_CALLBACK(events, "mark_invalid_block");
  events.push_back(blk_d);

  return true;
}
