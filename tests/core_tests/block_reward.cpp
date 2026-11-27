// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2016 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "block_reward.h"
#include "chaingen_helpers.h"

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
      if (bcs.is_hardfork_active(ZANO_HARDFORK_06))
      {
        if (coinbase_blob_size > CURRENCY_COINBASE_BLOB_RESERVED_SIZE_HF6)
        {
          CHECK_AND_ASSERT_MES(false, false, "CURRENCY_COINBASE_BLOB_RESERVED_SIZE_HF6 validation in test failed");
        }
      }
      else
      {
        if (coinbase_blob_size > CURRENCY_COINBASE_BLOB_RESERVED_SIZE)
          cumulative_block_size += coinbase_blob_size;
      }

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

//------------------------------------------------------------------------------
//
// block_size_penalty_zone: consensus rules for the size of a block and the block reward, HF6+
//
// S is the size of a block. Starting from HF6 it is the total size of the block's txs: the coinbase is not counted (its own size is
// limited by CURRENCY_COINBASE_BLOB_RESERVED_SIZE_HF6), and the coinbase must state S in etc_coinbase_block_cumulative_size.
// M is the median of the sizes of the last CURRENCY_REWARD_BLOCKS_WINDOW blocks, but not less than CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE.
//
// Rules under test:
//   1) S <= M: the block is valid and gets the full reward (CURRENCY_BLOCK_REWARD);
//   2) M < S < 2 * M (the penalty zone): the block is valid, the reward is CURRENCY_BLOCK_REWARD * (2 * M - S) * S / M^2;
//   3) S >= 2 * M: the block is invalid (the formula gives zero for S == 2 * M);
//   4) the coinbase must state the real size of the block and pay exactly the reward of that size: not more, not less;
//   5) the block template stays out of the penalty zone (tx fees are burned, so a miner gets nothing for a bigger block).
//
// generate() only gives coins to the miner, all the scenarios are played inside c1().
//

namespace
{
  const uint64_t c_alice_output_amount          = 10 * TESTS_DEFAULT_FEE; // amount of each output Alice gets from the miner
  const uint64_t c_alice_tx_amount              = 5 * TESTS_DEFAULT_FEE;  // amount Alice sends to Bob in each tx
  const size_t   c_alice_funding_txs_count      = 2;
  const size_t   c_alice_outputs_per_funding_tx = 14;

  // block reward for the given size of a block, calculated separately from currency::get_block_reward()
  uint64_t calc_block_reward(size_t median_size, size_t block_size)
  {
    if (block_size <= median_size)
      return CURRENCY_BLOCK_REWARD;

    if (block_size >= 2 * median_size)
      return 0;

    boost::multiprecision::uint128_t reward = CURRENCY_BLOCK_REWARD;
    reward *= 2 * median_size - block_size;
    reward *= block_size;
    reward /= median_size;
    reward /= median_size;
    return reward.convert_to<uint64_t>();
  }

  size_t get_txs_size(const std::vector<transaction>& txs)
  {
    size_t result = 0;
    for (const transaction& tx : txs)
      result += get_object_blobsize(tx);
    return result;
  }

  uint64_t get_txs_fee(const std::vector<transaction>& txs)
  {
    uint64_t result = 0;
    for (const transaction& tx : txs)
      result += get_tx_fee(tx);
    return result;
  }
} // namespace

block_size_penalty_zone::block_size_penalty_zone()
{
  REGISTER_CALLBACK_METHOD(block_size_penalty_zone, c1);
}

bool block_size_penalty_zone::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 10); // unlocks miner's coins for the txs to Alice made in c1

  DO_CALLBACK(events, "c1");
  return true;
}

bool block_size_penalty_zone::make_tx_of_size(tools::wallet2& alice_wlt, size_t tx_size, currency::transaction& tx) const
{
  // The size is tuned with extra_padding. All Alice's outputs are the same, so all her txs have the same structure (one input, two outputs),
  // and the size of a tx without the bytes of the padding, once measured, fits the next txs. A tx of a wrong size is thrown away
  // (the wallet keeps its input as a spent one, there are spare outputs for that); the very first tx is always such a one.
  const size_t attempts_max = 5;
  for (size_t attempt = 0; attempt < attempts_max; ++attempt)
  {
    CHECK_AND_ASSERT_MES(tx_size > m_tx_size_without_padding, false, "tx size " << tx_size << " is too small");
    extra_padding padding{};
    padding.buff.resize(tx_size - m_tx_size_without_padding);
    // the size of the padding is stored as a varint, so m_tx_size_without_padding holds only for paddings with the same length of that varint (3 bytes)
    CHECK_AND_ASSERT_MES(padding.buff.size() >= (1 << 14) && padding.buff.size() < (1 << 21), false, "unexpected size of the padding: " << padding.buff.size());

    tools::construct_tx_param ctp = alice_wlt.get_default_construct_tx_param();
    ctp.dsts.emplace_back(c_alice_tx_amount, m_accounts[BOB_ACC_IDX].get_public_address());
    ctp.fee = TESTS_DEFAULT_FEE;
    ctp.extra.push_back(padding);
    try
    {
      alice_wlt.transfer(ctp, tx, false /* send_to_network */);
    }
    catch (const std::exception& e)
    {
      LOG_ERROR("wallet transfer failed: " << e.what());
      return false;
    }

    const size_t actual_size = get_object_blobsize(tx);
    if (actual_size == tx_size)
      return true;

    CHECK_AND_ASSERT_MES(actual_size > padding.buff.size(), false, "unexpected size of the tx: " << actual_size);
    m_tx_size_without_padding = actual_size - padding.buff.size();
  }

  LOG_ERROR("failed to make a tx of " << tx_size << " bytes in " << attempts_max << " attempts");
  return false;
}

bool block_size_penalty_zone::make_txs_of_total_size(tools::wallet2& alice_wlt, size_t txs_count, size_t total_size, std::vector<currency::transaction>& txs) const
{
  txs.clear();
  size_t size_left = total_size;
  for (size_t i = 0; i < txs_count; ++i)
  {
    const size_t tx_size = size_left / (txs_count - i); // the last tx takes everything that is left
    CHECK_AND_ASSERT_TRUE(make_tx_of_size(alice_wlt, tx_size, txs.emplace_back()));
    size_left -= tx_size;
  }

  CHECK_AND_ASSERT_EQ(get_txs_size(txs), total_size);
  return true;
}

bool block_size_penalty_zone::push_block(currency::core& c, const std::vector<currency::transaction>& txs, size_t declared_size, size_t size_for_reward, bool expected_to_be_accepted, const std::string& descr) const
{
  blockchain_storage& bcs = c.get_blockchain_storage();
  const account_public_address& miner_addr = m_accounts[MINER_ACC_IDX].get_public_address();

  // the header, the difficulty and the height come from the block template, the txs and the coinbase are replaced
  create_block_template_params cbtp{};
  cbtp.ignore_pow_ts_check = true;
  cbtp.miner_address = miner_addr;
  create_block_template_response cbtr{};
  bool r = c.get_block_template(cbtp, cbtr);
  CHECK_AND_ASSERT_MES(r, false, descr << ": get_block_template failed");
  block& b = cbtr.b;

  block top_block{};
  r = bcs.get_top_block(top_block);
  CHECK_AND_ASSERT_MES(r, false, descr << ": get_top_block failed");
  const crypto::hash top_block_id = get_block_hash(top_block);
  block_extended_info top_bei{};
  r = bcs.get_block_extended_info_by_hash(top_block_id, top_bei);
  CHECK_AND_ASSERT_MES(r, false, descr << ": get_block_extended_info_by_hash failed");

  // adjust block's timestamp to keep difficulty low, and keep global time up with blocks' timestamps
  b.timestamp = top_block.timestamp + DIFFICULTY_POW_TARGET;
  test_core_time::adjust(b.timestamp);

  b.tx_hashes.clear();
  for (const transaction& tx : txs)
    b.tx_hashes.push_back(get_transaction_hash(tx));

  size_t tx_hardfork_id = 0;
  const uint64_t tx_version = get_tx_version_and_hardfork_id(cbtr.height, m_hardforks, tx_hardfork_id);
  CHECK_AND_ASSERT_MES(tx_hardfork_id >= ZANO_HARDFORK_06, false, descr << ": this test is for HF6 and later hardforks only");

  b.miner_tx = transaction{};
  etc_coinbase_block_cumulative_size ecbs{};
  ecbs.v = declared_size;
  b.miner_tx.extra.push_back(ecbs);
  uint64_t block_reward_without_fee = 0, block_reward = 0;
  r = construct_miner_tx(cbtr.height, bcs.get_current_comulative_blocksize_limit() / 2, top_bei.already_generated_coins, size_for_reward, get_txs_fee(txs),
    miner_addr, miner_addr, b.miner_tx, block_reward_without_fee, block_reward, tx_version, tx_hardfork_id);
  CHECK_AND_ASSERT_MES(r, false, descr << ": construct_miner_tx failed");

  r = currency::find_nonce_for_given_block(b, cbtr.diffic, cbtr.height);
  CHECK_AND_ASSERT_MES(r, false, descr << ": find_nonce_for_given_block failed");

  LOG_PRINT_YELLOW(descr << ": block " << get_block_hash(b) << " with " << txs.size() << " txs of " << get_txs_size(txs) << " bytes in total, declared size: " << declared_size
    << ", coinbase pays " << print_money_brief(block_reward_without_fee) << " (the reward of a block of " << size_for_reward << " bytes), must be "
    << (expected_to_be_accepted ? "accepted" : "rejected"), LOG_LEVEL_0);

  block_verification_context bvc{};
  for (const transaction& tx : txs)
    bvc.m_onboard_transactions[get_transaction_hash(tx)] = tx;
  c.handle_incoming_block(t_serializable_object_to_blob(b), bvc);

  if (expected_to_be_accepted)
  {
    CHECK_AND_ASSERT_MES(bvc.m_added_to_main_chain && !bvc.m_verification_failed, false, descr << ": the block must be accepted, but it is not");
    CHECK_AND_ASSERT_MES(c.get_tail_id() == get_block_hash(b), false, descr << ": the block is accepted, but it is not the top one");
  }
  else
  {
    CHECK_AND_ASSERT_MES(bvc.m_verification_failed && !bvc.m_added_to_main_chain, false, descr << ": the block must be rejected, but it is not");
    CHECK_AND_ASSERT_MES(c.get_tail_id() == top_block_id, false, descr << ": the block is rejected, but the top block has changed");
  }

  return true;
}

bool block_size_penalty_zone::check_top_block(currency::core& c, tools::wallet2& miner_wlt, const std::vector<currency::transaction>& txs, uint64_t expected_reward, const std::string& descr) const
{
  blockchain_storage& bcs = c.get_blockchain_storage();

  block top_block{};
  bool r = bcs.get_top_block(top_block);
  CHECK_AND_ASSERT_MES(r, false, descr << ": get_top_block failed");
  block_extended_info bei{}, prev_bei{};
  r = bcs.get_block_extended_info_by_hash(get_block_hash(top_block), bei) && bcs.get_block_extended_info_by_hash(top_block.prev_id, prev_bei);
  CHECK_AND_ASSERT_MES(r, false, descr << ": get_block_extended_info_by_hash failed");

  std::vector<crypto::hash> expected_tx_ids;
  for (const transaction& tx : txs)
    expected_tx_ids.push_back(get_transaction_hash(tx));
  CHECK_AND_ASSERT_MES(top_block.tx_hashes == expected_tx_ids, false, descr << ": unexpected txs in the top block");
  CHECK_AND_ASSERT_MES(bei.block_cumulative_size == get_txs_size(txs), false, descr << ": unexpected size of the top block: " << bei.block_cumulative_size << ", expected: " << get_txs_size(txs));

  // emission: the reward is added, the fees of the txs are burned
  CHECK_AND_ASSERT_MES(bei.already_generated_coins + get_txs_fee(txs) == prev_bei.already_generated_coins + expected_reward, false, descr << ": unexpected emission, generated coins before the block: "
    << prev_bei.already_generated_coins << ", after: " << bei.already_generated_coins << ", fees: " << get_txs_fee(txs) << ", expected reward: " << expected_reward);

  // the coinbase really gives that reward to the miner
  miner_wlt.refresh();
  const uint64_t miner_balance = miner_wlt.balance();
  CHECK_AND_ASSERT_MES(miner_balance == m_miner_balance + expected_reward, false, descr << ": unexpected miner's balance: " << miner_balance << ", balance before the block: "
    << m_miner_balance << ", expected reward: " << expected_reward);
  m_miner_balance = miner_balance;

  return true;
}

bool block_size_penalty_zone::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // M = CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE (the blocks of the test are too few to move the median).
  // Blocks S1 .. S6 are hand-built and go straight to the core. Each rejected block has an accepted twin that differs only in the value under test.
  //
  // step | size of txs | size stated in coinbase | coinbase pays the reward of | expected
  // -----+-------------+-------------------------+-----------------------------+--------------------------------------------------------------
  // S1   | M           | the real one            | the real size               | accepted, the full reward
  // S2   | M + 1       | the real one            | the real size               | accepted, the reward is less than the full one
  // S3a  | M * 3 / 2   | the real one            | an empty block (full one)   | rejected: the coinbase pays more than allowed
  // S3b  |             | M                       | the real size               | rejected: the stated size is not the real one
  // S3c  |             | the real one            | a block of M * 3 / 2 + 1    | rejected: the coinbase pays less than allowed
  // S3d  |             | the real one            | the real size               | accepted, 3/4 of the full reward
  // S4   | 2 * M - 1   | the real one            | the real size               | accepted, the smallest reward
  // S5   | 2 * M       | the real one            | a block of 2 * M - 1        | rejected: the block is too big
  // S6   | 2 * M + 1   | the real one            | a block of 2 * M - 1        | rejected: the block is too big
  // S7   | a block is mined using the block template, the txs of S5 and S6 are in the pool   | one tx is taken, the full reward

  const size_t M = CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE;
  const uint64_t full_reward = CURRENCY_BLOCK_REWARD;
  const account_public_address& miner_addr = m_accounts[MINER_ACC_IDX].get_public_address();
  const account_public_address& alice_addr = m_accounts[ALICE_ACC_IDX].get_public_address();
  blockchain_storage& bcs = c.get_blockchain_storage();
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);

  // setup: the miner gives Alice a number of equal outputs
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  miner_wlt->refresh();
  try
  {
    for (size_t i = 0; i < c_alice_funding_txs_count; ++i)
    {
      std::vector<tx_destination_entry> destinations(c_alice_outputs_per_funding_tx, tx_destination_entry(c_alice_output_amount, alice_addr));
      transaction tx{};
      miner_wlt->transfer(destinations, 0, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment, tx);
    }
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("failed to give coins to Alice: " << e.what());
    return false;
  }
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), c_alice_funding_txs_count);
  CHECK_AND_ASSERT_TRUE(mine_next_pow_block_in_playtime(miner_addr, c));
  CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  CHECK_AND_ASSERT_TRUE(mine_next_pow_blocks_in_playtime(miner_addr, c, WALLET_DEFAULT_TX_SPENDABLE_AGE)); // makes Alice's outputs spendable

  // Alice's wallet is not refreshed after this point: it never gets the change of its txs, so each tx it makes spends one of the equal outputs
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh();
  uint64_t alice_unlocked_balance = 0;
  const uint64_t alice_balance = alice_wlt->balance(alice_unlocked_balance);
  CHECK_AND_ASSERT_EQ(alice_balance, c_alice_funding_txs_count * c_alice_outputs_per_funding_tx * c_alice_output_amount);
  CHECK_AND_ASSERT_EQ(alice_unlocked_balance, alice_balance);

  miner_wlt->refresh();
  m_miner_balance = miner_wlt->balance();
  CHECK_AND_ASSERT_EQ(bcs.get_current_comulative_blocksize_limit(), 2 * M);

  // S1: the biggest block of the full reward zone
  {
    std::vector<transaction> txs;
    CHECK_AND_ASSERT_TRUE(make_txs_of_total_size(*alice_wlt, 2, M, txs));
    CHECK_AND_ASSERT_TRUE(push_block(c, txs, M, M, true, "S1"));
    CHECK_AND_ASSERT_TRUE(check_top_block(c, *miner_wlt, txs, full_reward, "S1"));
    CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  }

  // S2: the smallest block of the penalty zone
  {
    const size_t S = M + 1;
    const uint64_t reward = calc_block_reward(M, S);
    CHECK_AND_ASSERT_MES(reward < full_reward, false, "S2: the reward must be less than the full one: " << reward);

    std::vector<transaction> txs;
    CHECK_AND_ASSERT_TRUE(make_txs_of_total_size(*alice_wlt, 2, S, txs));
    CHECK_AND_ASSERT_TRUE(push_block(c, txs, S, S, true, "S2"));
    CHECK_AND_ASSERT_TRUE(check_top_block(c, *miner_wlt, txs, reward, "S2"));
    CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  }

  // S3: the middle of the penalty zone, the reward is enforced in both directions
  {
    const size_t S = M + M / 2;
    const uint64_t reward = calc_block_reward(M, S);
    CHECK_AND_ASSERT_EQ(reward, full_reward / 4 * 3);

    std::vector<transaction> txs;
    CHECK_AND_ASSERT_TRUE(make_txs_of_total_size(*alice_wlt, 2, S, txs));
    CHECK_AND_ASSERT_TRUE(push_block(c, txs, S, 0,     false, "S3a"));
    CHECK_AND_ASSERT_TRUE(push_block(c, txs, M, S,     false, "S3b"));
    CHECK_AND_ASSERT_TRUE(push_block(c, txs, S, S + 1, false, "S3c"));
    CHECK_AND_ASSERT_TRUE(push_block(c, txs, S, S,     true,  "S3d"));
    CHECK_AND_ASSERT_TRUE(check_top_block(c, *miner_wlt, txs, reward, "S3d"));
    CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0); // the txs came to the pool from the rejected blocks, and S3d has taken them back
  }

  // S4: the biggest valid block
  {
    const size_t S = 2 * M - 1;
    const uint64_t reward = calc_block_reward(M, S);
    CHECK_AND_ASSERT_MES(0 < reward && reward < full_reward / 4 * 3, false, "S4: unexpected reward: " << reward);

    std::vector<transaction> txs;
    CHECK_AND_ASSERT_TRUE(make_txs_of_total_size(*alice_wlt, 3, S, txs));
    CHECK_AND_ASSERT_TRUE(push_block(c, txs, S, S, true, "S4"));
    CHECK_AND_ASSERT_TRUE(check_top_block(c, *miner_wlt, txs, reward, "S4"));
    CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), 0);
  }

  // S5, S6: blocks that are too big; their coinbases are the same as the one of S4
  std::vector<transaction> txs_5, txs_6;
  {
    CHECK_AND_ASSERT_TRUE(make_txs_of_total_size(*alice_wlt, 3, 2 * M, txs_5));
    CHECK_AND_ASSERT_TRUE(push_block(c, txs_5, 2 * M, 2 * M - 1, false, "S5"));

    txs_6 = txs_5;
    CHECK_AND_ASSERT_TRUE(make_tx_of_size(*alice_wlt, get_object_blobsize(txs_5.back()) + 1, txs_6.back())); // the last tx is replaced by a tx that is one byte bigger
    CHECK_AND_ASSERT_EQ(get_txs_size(txs_6), 2 * M + 1);
    CHECK_AND_ASSERT_TRUE(push_block(c, txs_6, 2 * M + 1, 2 * M - 1, false, "S6"));
  }

  // S7: the txs of the rejected blocks are in the pool, each of them fits into the full reward zone, any two of them don't
  {
    CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), txs_5.size() + 1);

    block b{};
    CHECK_AND_ASSERT_TRUE(mine_next_pow_block_in_playtime(miner_addr, c, &b));
    CHECK_AND_ASSERT_EQ(b.tx_hashes.size(), 1);

    std::vector<transaction> candidates = txs_5, txs;
    candidates.push_back(txs_6.back());
    for (const transaction& tx : candidates)
    {
      if (get_transaction_hash(tx) == b.tx_hashes.front())
        txs.push_back(tx);
    }
    CHECK_AND_ASSERT_EQ(txs.size(), 1);
    CHECK_AND_ASSERT_TRUE(check_top_block(c, *miner_wlt, txs, full_reward, "S7"));
    CHECK_AND_ASSERT_EQ(c.get_pool_transactions_count(), txs_5.size());
  }

  CHECK_AND_ASSERT_EQ(bcs.get_current_comulative_blocksize_limit(), 2 * M); // the big blocks haven't moved the median

  return true;
}
