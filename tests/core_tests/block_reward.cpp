// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2016 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "block_reward.h"

using namespace crypto;
using namespace currency;

block_template_against_txs_size::block_template_against_txs_size()
{
  REGISTER_CALLBACK_METHOD(block_template_against_txs_size, c1);
}

bool block_template_against_txs_size::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(miner_acc);
                                                                                                    // event index
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());                         // 0
  events.push_back(miner_acc);                                                                      // 1
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK(events, "c1");

  return true;
}


static size_t g_block_txs_total_size = 0;
static uint64_t g_block_txs_fee = 0;
bool custom_fill_block_template_func(block &bl, bool pos, size_t median_size, const boost::multiprecision::uint128_t& already_generated_coins, size_t &total_size, uint64_t &fee, uint64_t height)
{
  total_size = g_block_txs_total_size;
  fee = g_block_txs_fee;
  return true;
}

bool block_template_against_txs_size::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // Test idea: check BCS::create_block_template() against validate_miner_transaction() for big values of 'txs total size'
  // Quite a long test because it checks each tx_total_size value with 1-increments

  bool r = false;
  blockchain_storage &bcs = c.get_blockchain_storage();

  account_base miner_acc = boost::get<account_base>(events[1]);
  account_public_address miner_addr = miner_acc.get_public_address();

  block blk_0 = AUTO_VAL_INIT(blk_0);
  r = bcs.get_block_by_height(0, blk_0);
  CHECK_AND_ASSERT_MES(r, false, "get_block_by_height failed");
  

  // for PoS get output 0 from miner tx of block 0
  crypto::key_image ki = AUTO_VAL_INIT(ki);
  keypair ephemeral = AUTO_VAL_INIT(ephemeral);
  r = generate_key_image_helper(miner_acc.get_keys(), get_tx_pub_key_from_extra(blk_0.miner_tx), 0, ephemeral, ki);
  CHECK_AND_ASSERT_MES(r, false, "generate_key_image_helper failed");
  CHECK_AND_ASSERT_MES(boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(blk_0.miner_tx.vout[0]).target).key == ephemeral.pub, false, "ephemeral.pub doesn't match with output key");
  pos_entry pe = AUTO_VAL_INIT(pe);
  pe.amount = boost::get<currency::tx_out_bare>(blk_0.miner_tx.vout[0]).amount;
  pe.block_timestamp = UINT64_MAX; // doesn't matter
  pe.g_index = 0; // global index
  pe.keyimage = ki;
  pe.wallet_index = UINT64_MAX; // doesn't matter

  uint64_t top_block_height = bcs.get_top_block_height();
  uint64_t blocksize_limit = bcs.get_current_comulative_blocksize_limit();
  uint64_t base_block_reward_pow = get_base_block_reward(top_block_height + 1);
  uint64_t base_block_reward_pos = base_block_reward_pow;

  g_block_txs_fee = TESTS_DEFAULT_FEE; // passing an argument to custom_fill_block_template_func via global variable (not perfect but works well)

  CHECK_AND_ASSERT_MES(blocksize_limit > CURRENCY_COINBASE_BLOB_RESERVED_SIZE, false, "internal invariant failed");

  log_level_scope_changer llsc(LOG_LEVEL_1); // change log level to 1 for the current scope to get rid of miner tx notification messages

  for(size_t txs_total_size = CURRENCY_MAX_TRANSACTION_BLOB_SIZE * 0.95f; txs_total_size <= blocksize_limit - CURRENCY_COINBASE_BLOB_RESERVED_SIZE; ++txs_total_size)
  {
    for (int is_pos = 0; is_pos < 2; ++is_pos)
    {
      block b = AUTO_VAL_INIT(b);                             
      blobdata ex_nonce;
      wide_difficulty_type diff = 0;
      uint64_t height = 0;
      g_block_txs_total_size = txs_total_size; // passing an argument to custom_fill_block_template_func via global variable (not perfect but works well)
      r = bcs.create_block_template(miner_addr, miner_addr, ex_nonce, is_pos != 0, pe, &custom_fill_block_template_func, b, diff, height);
      CHECK_AND_ASSERT_MES(r, false, "create_block_template failed, txs_total_size = " << txs_total_size);
      CHECK_AND_ASSERT_MES(height == top_block_height + 1, false, "Incorrect height: " << height << ", expected: " << top_block_height + 1 << ", txs_total_size = " << txs_total_size);

      uint64_t block_reward_without_fee = 0;

      size_t cumulative_block_size = txs_total_size;
      size_t coinbase_blob_size = get_object_blobsize(b.miner_tx);
      if (coinbase_blob_size > CURRENCY_COINBASE_BLOB_RESERVED_SIZE)
        cumulative_block_size += coinbase_blob_size;

      r = bcs.calculate_block_reward_for_next_top_block(cumulative_block_size, block_reward_without_fee);
      CHECK_AND_ASSERT_MES(r, false, "calculate_block_reward_for_next_top_block failed");
      r = bcs.validate_miner_transaction(b.miner_tx, g_block_txs_fee, block_reward_without_fee);
      CHECK_AND_ASSERT_MES(r, false, "validate_miner_transaction failed, txs_total_size = " << txs_total_size);

      uint64_t generated_coins = get_outs_money_amount(b.miner_tx) - (is_pos != 0 ? boost::get<tx_out_bare>(b.miner_tx.vout.back()).amount : 0) - g_block_txs_fee / 2;
      uint64_t base_block_reward = is_pos != 0 ? base_block_reward_pos : base_block_reward_pow;

      if (txs_total_size % 1000 == 0)
      {
        LOG_PRINT("checked " << (is_pos != 0 ? "PoS" : "PoW") << ", txs_total_size == " << txs_total_size << ", generated_coins: " << std::setw(20) << generated_coins << " (" << std::setw(5) << std::setprecision(4) << generated_coins * 100.0f / base_block_reward << "% of base reward == " << std::setw(20) << base_block_reward << ")"
          << "    progress: " << txs_total_size * 100 / blocksize_limit << "%", LOG_LEVEL_0);
      }
    }
  }

  return true;
}
