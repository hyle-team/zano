// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "wallet_tests.h"
#include "wallet_test_core_proxy.h"
#include "../../src/wallet/wallet_public_structs_defs.h"
#include "offers_helper.h"
#include "string_coding.h"
#include "random_helper.h"
#include "tx_builder.h"

using namespace epee;
using namespace crypto;
using namespace currency;

const uint64_t uint64_max = std::numeric_limits<uint64_t>::max();
const std::wstring g_wallet_filename = L"~coretests.wallet.file.tmp";
const std::string g_wallet_password = "dofatibmzibeziyekigo";
const currency::account_base null_account = AUTO_VAL_INIT(null_account);


POD_MAKE_COMPARABLE(currency, tx_out);

// Determines which output is real and actually spent in tx inputs, when there are fake outputs.
bool determine_tx_real_inputs(currency::core& c, const currency::transaction& tx, const currency::account_keys& keys, std::vector<size_t>& real_inputs)
{
  struct local_visitor
  {
    local_visitor(const currency::account_keys& keys, const crypto::key_image key_image)
      : m_keys(keys)
      , m_txin_key_image(key_image)
      , m_output_in_input_index(0)
      , m_found(false)
    {}

    bool handle_output(const transaction& source_tx, const transaction& validated_tx, const tx_out& out, uint64_t out_i)
    {
      CHECK_AND_ASSERT_MES(!m_found, false, "Internal error: m_found is true but the visitor is still being applied");
      auto it = std::find(validated_tx.vout.begin(), validated_tx.vout.end(), out);
      if (it == validated_tx.vout.end())
        return false;
      size_t output_tx_index = it - validated_tx.vout.begin();

      crypto::public_key tx_pub_key = get_tx_pub_key_from_extra(validated_tx);
      crypto::key_derivation derivation;
      bool r = generate_key_derivation(tx_pub_key, m_keys.m_view_secret_key, derivation);
      CHECK_AND_ASSERT_MES(r, false, "generate_key_derivation failed");
      crypto::secret_key ephemeral_secret_key;
      derive_secret_key(derivation, output_tx_index, m_keys.m_spend_secret_key, ephemeral_secret_key);

      crypto::public_key output_public_key = boost::get<txout_to_key>(out.target).key;

      /*crypto::public_key ephemeral_public_key;
      derive_public_key(derivation, output_tx_index, m_keys.m_account_address.m_spend_public_key, ephemeral_public_key);*/

      crypto::key_image ki;
      generate_key_image(output_public_key, ephemeral_secret_key, ki);

      if (ki == m_txin_key_image)
      {
        m_found = true;
        return false; // to break the loop in scan_outputkeys_for_indexes
      }

      ++m_output_in_input_index;
      return true;
    }

    currency::account_keys  m_keys;
    crypto::key_image       m_txin_key_image;
    size_t                  m_output_in_input_index;
    bool                    m_found;
  };

  for (auto& txin : tx.vin)
  {
    const txin_to_key& in = boost::get<txin_to_key>(txin);
    if (in.key_offsets.size() == 1)
    {
      real_inputs.push_back(0); // trivial case when no mixin is used
      continue;
    }
    local_visitor vis(keys, in.k_image);
    bool r = c.get_blockchain_storage().scan_outputkeys_for_indexes(tx, in, vis);
    CHECK_AND_ASSERT_MES(r || vis.m_found, false, "scan_outputkeys_for_indexes failed");
    if (!vis.m_found)
      return false;
    real_inputs.push_back(vis.m_output_in_input_index);
  }

  return true;
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

mined_balance_wallet_test::mined_balance_wallet_test()
{
  REGISTER_CALLBACK_METHOD(mined_balance_wallet_test, c1);
  REGISTER_CALLBACK_METHOD(mined_balance_wallet_test, set_core_config);
}

bool mined_balance_wallet_test::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);

  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, preminer_acc, test_core_time::get_time());
  events.push_back(blk_0);

  DO_CALLBACK(events, "set_core_config");
  DO_CALLBACK(events, "c1");

  return true;
}

bool mined_balance_wallet_test::set_core_config(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  core_runtime_config crc = c.get_blockchain_storage().get_core_runtime_config();
  crc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH;
  crc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE;
  c.get_blockchain_storage().set_core_runtime_config(crc);
  return true;
}

bool mined_balance_wallet_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  blockchain_storage& bcs = c.get_blockchain_storage();

  core_runtime_config crc = bcs.get_core_runtime_config();
  crc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH;
  bcs.set_core_runtime_config(crc);

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*miner_wlt.get(), "miner", 0), false, "wrong balance");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "alice", 0), false, "wrong balance");
  
  uint64_t miner_mined_money = 0;
  bool r = false;
  std::list<currency::block> blocks;

  size_t n = CURRENCY_MINED_MONEY_UNLOCK_WINDOW;
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, n);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");
  r = bcs.get_blocks(bcs.get_current_blockchain_size() - n, n, blocks);
  CHECK_AND_ASSERT_MES(r, false, "get_blocks failed");

  for (auto& b : blocks)
    miner_mined_money += get_outs_money_amount(b.miner_tx);

  miner_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*miner_wlt.get(), "miner", miner_mined_money, miner_mined_money), false, "wrong balance");

  n = bcs.get_current_blockchain_size();
  r = miner_wlt->try_mint_pos();
  CHECK_AND_ASSERT_MES(r && bcs.get_current_blockchain_size() > n, false, "can't mint a PoS block");

  block b = AUTO_VAL_INIT(b);
  r = bcs.get_top_block(b);
  CHECK_AND_ASSERT_MES(r, false, "get_top_block failed");
  CHECK_AND_ASSERT_MES(b.miner_tx.vin.size() == 2, false, "Invalid PoS coinbase tx");

  uint64_t coinbase_outs_amount = get_outs_money_amount(b.miner_tx);
  uint64_t stake_amount = boost::get<txin_to_key>(b.miner_tx.vin[1]).amount;
  CHECK_AND_ASSERT_MES(coinbase_outs_amount > stake_amount, false, "coinbase_outs_amount = " << coinbase_outs_amount << ", stake_amount = " << stake_amount << " : invalid condition");
  
  miner_mined_money += coinbase_outs_amount - stake_amount;

  miner_wlt->refresh();

  std::stringstream ss;
  miner_wlt->dump_trunsfers(ss, false);
  LOG_PRINT_CYAN("miner transfers: " << ENDL << ss.str(), LOG_LEVEL_0);

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*miner_wlt.get(), "miner", miner_mined_money, miner_mined_money), false, "wrong balance");

  return true;
}

//------------------------------------------------------------------------------

wallet_outputs_with_same_key_image::wallet_outputs_with_same_key_image()
{
  REGISTER_CALLBACK_METHOD(wallet_outputs_with_same_key_image, c1);
}

bool wallet_outputs_with_same_key_image::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure wallet does not take into account valid outputs having the same key image
  // Only one such output is spendable thus only one output should be taken into account.

  bool r = false;
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2);

  uint64_t tx_amount = MK_TEST_COINS(3);

  // tx_1
  std::vector<tx_source_entry> sources_1;
  r = fill_tx_sources(sources_1, events, blk_0r, miner_acc.get_keys(), tx_amount + TESTS_DEFAULT_FEE, 0);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");

  std::vector<tx_destination_entry> destinations{ tx_destination_entry(tx_amount, alice_acc.get_public_address()) };

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_acc.get_keys(), sources_1);
  builder.step3_fill_outputs(destinations);
  builder.step4_calc_hash();
  builder.step5_sign(sources_1);

  transaction tx_1 = builder.m_tx;
  events.push_back(tx_1);

  // tx_2 with the same secret key
  currency::keypair tmp_sec_key = builder.m_tx_key;
  builder.step1_init();
  builder.m_tx_key = tmp_sec_key;
  builder.m_tx.extra.clear();
  add_tx_pub_key_to_extra(builder.m_tx, builder.m_tx_key.pub);

  std::vector<tx_source_entry> sources_2;
  r = fill_tx_sources(sources_2, events, blk_0r, miner_acc.get_keys(), tx_amount + TESTS_DEFAULT_FEE, 0, sources_1);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");

  // keep destinations the same

  builder.step2_fill_inputs(miner_acc.get_keys(), sources_2);
  builder.step3_fill_outputs(destinations);
  builder.step4_calc_hash();
  builder.step5_sign(sources_2);

  transaction tx_2 = builder.m_tx;
  events.push_back(tx_2);

  // make sure tx_1 and tx_2 have been created with the same tx key
  CHECK_AND_ASSERT_MES(get_tx_pub_key_from_extra(tx_1) == get_tx_pub_key_from_extra(tx_2), false, "internal error: tx_1 and tx_2 have different pub keys");

  // now both txs are in the pool, make sure they are
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(2));

  DO_CALLBACK(events, "c1");

  return true;
}

bool wallet_outputs_with_same_key_image::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  
  // check Alice has no unlocked coins
  bool r = refresh_wallet_and_check_balance("before tx_1 and tx_2 added", "Alice", alice_wlt, MK_TEST_COINS(3) * 2, true, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2, 0);
  CHECK_AND_ASSERT_MES(r, false, "");

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "there are txs in the pool!");

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  // only one tx_1 output is counted as the tx_2 output has the very same key image
  r = refresh_wallet_and_check_balance("after tx_1 and tx_2 added", "Alice", alice_wlt, MK_TEST_COINS(3), true, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1, MK_TEST_COINS(3));
  CHECK_AND_ASSERT_MES(r, false, "");

  // make sure Alice is able to transfer her coins to smbd
  std::vector<tx_destination_entry> destinations{ tx_destination_entry(MK_TEST_COINS(3) - TESTS_DEFAULT_FEE, m_accounts[MINER_ACC_IDX].get_public_address()) };
  try
  {
    alice_wlt->transfer(destinations, 0, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment);
  }
  catch (...)
  {
    CHECK_AND_ASSERT_MES(false, false, "Alice failed to transfer all her funds");
  }

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "wrong tx count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "there are txs in the pool!");

  return true;
}

//------------------------------------------------------------------------------

wallet_unconfirmed_tx_expiration::wallet_unconfirmed_tx_expiration()
{
  REGISTER_CALLBACK_METHOD(wallet_unconfirmed_tx_expiration, c1);
}

bool wallet_unconfirmed_tx_expiration::generate(std::vector<test_event_entry>& events) const
{
  // Test outline:
  // 1. Alice sends tx with expiration.
  // 2. Miner ignores Alice's tx, so tx expires in the pool.
  // 3. Tx is being removed from the pool due to expiration.
  // Make sure Alice eventually spent no coins and all her money is unlocked in the wallet.

  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  bool r = false;
  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  r = construct_tx_with_many_outputs(events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), TESTS_DEFAULT_FEE * 20, 10, TESTS_DEFAULT_FEE, tx_0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs");
  events.push_back(tx_0);
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx_with_many_outputs(events, blk_0r, miner_acc.get_keys(), bob_acc.get_public_address(), TESTS_DEFAULT_FEE * 20, 10, TESTS_DEFAULT_FEE, tx_1);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs");
  events.push_back(tx_1);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({ tx_0, tx_1 }));

  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

bool wallet_unconfirmed_tx_expiration::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);

  uint64_t alice_start_balance = TESTS_DEFAULT_FEE * 20;
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, alice_start_balance), false, "");

  // Alice constructs and sends tx with expiration time
  uint64_t expiration_time = c.get_blockchain_storage().get_tx_expiration_median() + TX_EXPIRATION_MEDIAN_SHIFT + 15;
  etc_tx_details_expiration_time extra_entry = AUTO_VAL_INIT(extra_entry);
  extra_entry.v = expiration_time;
  std::vector<extra_v> extra({ extra_entry }); // extra with expiration time
  std::vector<tx_destination_entry> destinations({ tx_destination_entry(TESTS_DEFAULT_FEE * 2, m_accounts[MINER_ACC_IDX].get_public_address()) });
  transaction tx = AUTO_VAL_INIT(tx);
  try
  {
    alice_wlt->transfer(destinations, 0, 0, TESTS_DEFAULT_FEE, extra, empty_attachment, tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), tx);
  }
  catch (std::exception &e)
  {
    CHECK_AND_ASSERT_MES(false, false, "alice_wlt->transfer() caused an exception: " << e.what());
  }

  CHECK_AND_ASSERT_MES(get_tx_expiration_time(tx) == expiration_time, false, "tx expiration time wasn't set");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Invalid txs count in the pool: " << c.get_pool_transactions_count());

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("tx sent,", "Alice", alice_wlt, alice_start_balance - TESTS_DEFAULT_FEE * 2 - TESTS_DEFAULT_FEE, true, 0, UINT64_MAX, 0, 0, TESTS_DEFAULT_FEE * 2), false, "");

  // mine a few block with no tx, so Alice's tx is expired in the pool
  for (size_t i = 0; i < 5; ++i)
  {
    r = mine_next_pow_block_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), c, std::vector<transaction>());
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime_with_given_txs failed");
  }

  // tx is still there
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Invalid txs count in the pool: " << c.get_pool_transactions_count());

  // make sure expiration median was shifted enough
  CHECK_AND_ASSERT_MES(c.get_blockchain_storage().is_tx_expired(tx), false, "wrong expiration time condition");

  LOG_PRINT_CYAN("%%%%% tx_pool::on_idle()", LOG_LEVEL_0);
  c.get_tx_pool().on_idle();

  // make sure tx was removed by the pool
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Invalid txs count in the pool: " << c.get_pool_transactions_count());

  // mine one more block to trigger wallet's on_idle() and outdated tx clearing
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  // make sure all Alice's money are unlocked and no coins were actually spent
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("tx expired and removed from the pool,", "Alice", alice_wlt, alice_start_balance, true, 6, alice_start_balance, 0, 0, 0), false, "");

  return true;
}

//------------------------------------------------------------------------------

wallet_chain_switch_with_spending_the_same_ki::wallet_chain_switch_with_spending_the_same_ki()
{
  REGISTER_CALLBACK_METHOD(wallet_chain_switch_with_spending_the_same_ki, c1);
}

bool wallet_chain_switch_with_spending_the_same_ki::generate(std::vector<test_event_entry>& events) const
{
  // Test outline
  // 1. A wallet has one unspent output
  // 2. wallet2::transfer() creates tx_0 that spends wallet's output
  // 3. tx_0 is successfully put into the blockchain
  // 4. Due to chain switch tx_0 is removed from the blockchain and get into the transaction pool
  // 5. Make sure the wallet can't spend that output
  // 6. After tx is expired make sure the wallet can spend that output


  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  MAKE_TX(events, tx_0, miner_acc, alice_acc, MK_TEST_COINS(30), blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  // rewind blocks to allow wallet be able to spend the coins
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

bool wallet_chain_switch_with_spending_the_same_ki::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(30), true, UINT64_MAX, MK_TEST_COINS(30)), false, "");

  std::vector<tx_destination_entry> destinations { tx_destination_entry(MK_TEST_COINS(30) - TESTS_DEFAULT_FEE, m_accounts[BOB_ACC_IDX].get_public_address()) };
  try
  {
    // create tx_1
    alice_wlt->transfer(destinations, 0, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment);
  }
  catch (std::exception &e)
  {
    CHECK_AND_ASSERT_MES(false, false, "alice_wlt->transfer() caused an exception: " << e.what());
  }

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Tx pool has incorrect number of txs: " << c.get_pool_transactions_count());

  // mine blk_2 on height 22
  CHECK_AND_ASSERT_MES(mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c), false, "");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Tx pool has incorrect number of txs: " << c.get_pool_transactions_count());

  // refresh wallet
  alice_wlt->refresh();
  // DO NOT scan_tx_pool here intentionally
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", MK_TEST_COINS(0)), false, "");

  uint64_t blk_1r_height = c.get_top_block_height() - 1;
  crypto::hash blk_1r_id = c.get_block_id_by_height(blk_1r_height);
  block blk_2a = AUTO_VAL_INIT(blk_2a);
  r = mine_next_pow_block_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), c, std::vector<transaction>(), blk_1r_id, blk_1r_height + 1, &blk_2a);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime_with_given_txs failed");

  // one more to trigger chain switch
  block blk_3a = AUTO_VAL_INIT(blk_3a);
  r = mine_next_pow_block_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), c, std::vector<transaction>(), get_block_hash(blk_2a), get_block_height(blk_2a) + 1, &blk_3a);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime_with_given_txs failed");

  // make sure tx_1 has been moved back to the pool
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  CHECK_AND_ASSERT_MES(c.get_alternative_blocks_count() == 1, false, "Incorrect alt blocks count: " << c.get_alternative_blocks_count());

  //const transaction& tx_1 = boost::get<transaction>(events[4 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3]);

  // refresh wallet
  alice_wlt->refresh();
  // DO NOT scan_tx_pool here intentionally
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", MK_TEST_COINS(0)), false, "");

  return true;
}

//------------------------------------------------------------------------------

wallet_unconfimed_tx_balance::wallet_unconfimed_tx_balance()
{
  REGISTER_CALLBACK_METHOD(wallet_unconfimed_tx_balance, c1);
}

bool wallet_unconfimed_tx_balance::generate(std::vector<test_event_entry>& events) const
{
  // Test outline:
  // 1. Miner sends 100 coins to Alice (50 + 50)
  // 2. Alice sends 30 back to Miner (tx is unconfirmed)
  // 3. Make sure Alice's wallet has correct balance, when it is checked from wallet's callback
  // 4. Few blocks are mined so the tx is get confirmed
  // 5. Make sure Alice's balance has changed correctly

  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  MAKE_TX(events, tx_0, miner_acc, alice_acc, MK_TEST_COINS(50), blk_0r);
  MAKE_TX(events, tx_1, miner_acc, alice_acc, MK_TEST_COINS(50), blk_0r);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({ tx_0, tx_1 }));

  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

bool wallet_unconfimed_tx_balance::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(100), false, UINT64_MAX, MK_TEST_COINS(100)), false, "");

  bool callback_is_ok = false;
  // this callback will ba called from within wallet2::transfer() below
  std::shared_ptr<wlt_lambda_on_transfer2_wrapper> l(new wlt_lambda_on_transfer2_wrapper(
    [&callback_is_ok](const tools::wallet_public::wallet_transfer_info& wti, uint64_t balance, uint64_t unlocked_balance, uint64_t total_mined) -> bool
    {
      CHECK_AND_ASSERT_MES(balance == MK_TEST_COINS(70), false, "invalid balance: " << print_money_brief(balance));
      CHECK_AND_ASSERT_MES(unlocked_balance == MK_TEST_COINS(50), false, "invalid unlocked_balance: " << print_money_brief(unlocked_balance));
      CHECK_AND_ASSERT_MES(total_mined == 0, false, "invalid total_mined: " << print_money_brief(total_mined));
      callback_is_ok = true;
      return true;
    }
  ));
  alice_wlt->callback(l);

  uint64_t fee = TESTS_DEFAULT_FEE * 3;
  std::vector<tx_destination_entry> destinations{ tx_destination_entry(MK_TEST_COINS(30) - fee, m_accounts[MINER_ACC_IDX].get_public_address()) };
  try
  {
    alice_wlt->transfer(destinations, 0, 0, fee, empty_extra, empty_attachment);
  }
  catch (std::exception &e)
  {
    CHECK_AND_ASSERT_MES(false, false, "alice_wlt->transfer() caused an exception: " << e.what());
  }

  CHECK_AND_NO_ASSERT_MES(callback_is_ok, false, "callback failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Tx pool has incorrect number of txs: " << c.get_pool_transactions_count());

  // 50 coins should be locked and 50 - unlocked
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(70), false, UINT64_MAX, MK_TEST_COINS(50), 0, 0, MK_TEST_COINS(30) - fee), false, "");

  // mine WALLET_DEFAULT_TX_SPENDABLE_AGE blocks so the tx get confirmed and coins get unlocked
  CHECK_AND_ASSERT_MES(mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, WALLET_DEFAULT_TX_SPENDABLE_AGE), false, "");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Tx pool has incorrect number of txs: " << c.get_pool_transactions_count());

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(70), false, UINT64_MAX, MK_TEST_COINS(70), 0, 0, 0), false, "");

  return true;
}
