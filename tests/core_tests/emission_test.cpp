// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "emission_test.h"
#include "pos_block_builder.h"
#include "random_helper.h"

using namespace currency;

emission_test::emission_test()
{
  REGISTER_CALLBACK_METHOD(emission_test, c1);
}

bool emission_test::generate(std::vector<test_event_entry> &events)
{
  m_miner_acc.generate();
  MAKE_GENESIS_BLOCK(events, blk_0, m_miner_acc, test_core_time::get_time());

  DO_CALLBACK(events, "c1");
  return true;
}

bool emission_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  bool r = false;

  size_t days_to_simulate = 365;

  blockchain_storage& bcs = c.get_blockchain_storage();

  const block& genesis = boost::get<block>(events[0]);
  uint64_t already_generated_coins = get_outs_money_amount(genesis.miner_tx);
  uint64_t pos_coins = 0;
  uint64_t pow_coins = 0;

  uint64_t timestamp = genesis.timestamp;

  uint64_t blocks_interval = DIFFICULTY_TOTAL_TARGET;
  wide_difficulty_type difficulty = 0;
  size_t height = 0;
  pos_block_builder pb;

  std::list<std::pair<crypto::hash, size_t>> stake_tx_outs; // pairs of (tx, output_index) using for PoS (due to tx unlock time we can't use the same tx-out for all PoS blocks)
  crypto::hash genesis_miner_tx_hash = get_transaction_hash(genesis.miner_tx);
  for (size_t i = 0; i < genesis.miner_tx.vout.size(); ++i)
    stake_tx_outs.push_back(std::make_pair(genesis_miner_tx_hash, i));

  std::stringstream s;
  s << ENDL << "time" << "\t" << "height" << "\ttype\t" << "gen_coins" << "\t" << "pow_coins" << "\t" << "pos_coins" << "\t" << "already_generated_coins" << ENDL;

  epee::log_space::get_set_log_detalisation_level(true, LOG_LEVEL_0);

  currency::block_verification_context bvc = AUTO_VAL_INIT(bvc);
  for(size_t j = 0; j < days_to_simulate; ++j)
  {
    size_t start_height = height;
    while(height < start_height + CURRENCY_BLOCKS_PER_DAY)
    {
      // pow
      timestamp += blocks_interval;
      test_core_time::adjust(timestamp);

      currency::block b = AUTO_VAL_INIT(b);
      blobdata extra = AUTO_VAL_INIT(extra);
      uint64_t height_from_template = 0;
      r = c.get_block_template(b, m_miner_acc.get_public_address(), m_miner_acc.get_public_address(), difficulty, height_from_template, extra);
      CHECK_AND_ASSERT_MES(r || height_from_template != height, false, "get_block_template failed");
      r = miner::find_nonce_for_given_block(b, difficulty, height);
      CHECK_AND_ASSERT_MES(r, false, "find_nonce_for_given_block failed");
      c.handle_incoming_block(t_serializable_object_to_blob(b), bvc);
      CHECK_AND_NO_ASSERT_MES(!bvc.m_verification_failed && !bvc.m_marked_as_orphaned && !bvc.m_already_exists, false, "block verification context check failed");
      //CHECK_AND_ASSERT_MES(b.timestamp == timestamp, false, "Invalid block ts: " << b.timestamp << ", expected: " << timestamp);
      uint64_t gen_coins = get_outs_money_amount(b.miner_tx);
      already_generated_coins += gen_coins;
      CHECK_AND_ASSERT_MES(already_generated_coins == bcs.total_coins(), false, "total coins missmatch: BCS has: " << bcs.total_coins() << ", expected: " << already_generated_coins);
      pow_coins += gen_coins;

      s << epee::misc_utils::get_time_str_v3(boost::posix_time::from_time_t(timestamp)) << "\t" << height << "\tPoW\t" << gen_coins << "\t" << pow_coins << "\t" << pos_coins << "\t" << already_generated_coins << ENDL;
      ++height;

      // pos
      if (height < CURRENCY_MINED_MONEY_UNLOCK_WINDOW)
        continue; // mint PoS blocks only when premine money is unlocked

      timestamp += blocks_interval;
      test_core_time::adjust(timestamp);

      crypto::hash prev_id = get_block_hash(b);
      crypto::hash stake_tx_hash = stake_tx_outs.front().first;
      size_t stake_output_idx    = stake_tx_outs.front().second;
      auto tce_ptr = bcs.get_tx_chain_entry(stake_tx_hash);
      CHECK_AND_ASSERT_MES(tce_ptr, false, "");
      CHECK_AND_ASSERT_MES(stake_output_idx < tce_ptr->m_global_output_indexes.size(), false, "");

      const transaction& stake = tce_ptr->tx;
      crypto::public_key stake_tx_pub_key = get_tx_pub_key_from_extra(stake);
      size_t stake_output_gidx = tce_ptr->m_global_output_indexes[stake_output_idx];
      uint64_t stake_output_amount =boost::get<currency::tx_out_bare>( stake.vout[stake_output_idx]).amount;
      crypto::key_image stake_output_key_image;
      keypair kp;
      generate_key_image_helper(m_miner_acc.get_keys(), stake_tx_pub_key, stake_output_idx, kp, stake_output_key_image);
      crypto::public_key stake_output_pubkey = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(stake.vout[stake_output_idx]).target).key;

      difficulty = bcs.get_next_diff_conditional(true);
      //size_t median_size = 0; // little hack: we're using small blocks (only coinbase tx), so we're in CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE - don't need to calc median size

      pb.clear();
      pb.step1_init_header(bcs.get_core_runtime_config().hard_forks, get_block_height(b) + 1, prev_id);
      pb.step2_set_txs(std::vector<transaction>());
      pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, difficulty, prev_id, null_hash, timestamp);
      pb.step4_generate_coinbase_tx(0, already_generated_coins, m_miner_acc.get_public_address());
      set_block_datetime(timestamp, pb.m_block);
      pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, m_miner_acc);

      c.handle_incoming_block(t_serializable_object_to_blob(pb.m_block), bvc);
      CHECK_AND_NO_ASSERT_MES(!bvc.m_verification_failed && !bvc.m_marked_as_orphaned && !bvc.m_already_exists, false, "PoS block verification context check failed");
      //CHECK_AND_ASSERT_MES(get_block_datetime(pb.m_block) == timestamp, false, "Invalid PoS block ts: " << get_block_datetime(pb.m_block) << ", expected: " << timestamp);
      gen_coins = get_outs_money_amount(pb.m_block.miner_tx) - boost::get<txin_to_key>(pb.m_block.miner_tx.vin[1]).amount;
      already_generated_coins += gen_coins;
      CHECK_AND_ASSERT_MES(already_generated_coins == bcs.total_coins(), false, "total coins missmatch: BCS has: " << bcs.total_coins() << ", expected: " << already_generated_coins);
      pos_coins += gen_coins;

      // update stakes queue: pop used one from the front and push output of this PoS block to the back
      size_t biggest_output_idx = std::max_element(pb.m_block.miner_tx.vout.begin(), pb.m_block.miner_tx.vout.end(),
        [](const currency::tx_out_v& l, const currency::tx_out_v& r)
      {
        return boost::get<tx_out_bare>(l).amount < boost::get<tx_out_bare>(r).amount;
      }) - pb.m_block.miner_tx.vout.begin();

      stake_tx_outs.pop_front();
      stake_tx_outs.push_back(std::make_pair(get_transaction_hash(pb.m_block.miner_tx), biggest_output_idx));

      s << epee::misc_utils::get_time_str_v3(boost::posix_time::from_time_t(timestamp)) << "\t" << height << "\tPoS\t" << gen_coins << "\t" << pow_coins << "\t" << pos_coins << "\t" << already_generated_coins << ENDL;
      ++height;
    }
  }

  LOG_PRINT2("emission_test.log", s.str(), LOG_LEVEL_0);

  return true;
}

//------------------------------------------------------------------------------

pos_emission_test::pos_emission_test()
{
  REGISTER_CALLBACK_METHOD(pos_emission_test, c1);
  REGISTER_CALLBACK_METHOD(pos_emission_test, c2);
  REGISTER_CALLBACK_METHOD(pos_emission_test, c3);

  m_pos_entries_to_generate = WALLET_DEFAULT_TX_SPENDABLE_AGE * 2;
  m_total_money_in_minting = (50 * COIN) * m_pos_entries_to_generate;
}

bool pos_emission_test::generate(std::vector<test_event_entry> &events)
{
  m_accounts.resize(TOTAL_ACCS_COUNT);
  GENERATE_ACCOUNT(preminer_acc);
  m_accounts[DAN_ACC_IDX] = preminer_acc; // make Dan a preminer
  GENERATE_ACCOUNT(miner_acc);
  m_accounts[MINER_ACC_IDX] = miner_acc;
  GENERATE_ACCOUNT(alice_acc);
  m_accounts[ALICE_ACC_IDX] = alice_acc;
  GENERATE_ACCOUNT(bob_acc);
  m_accounts[BOB_ACC_IDX] = bob_acc;

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  uint64_t pos_entry_amount = m_total_money_in_minting / m_pos_entries_to_generate;
  m_total_money_in_minting = m_pos_entries_to_generate * pos_entry_amount;

  std::vector<tx_source_entry> sources;
  bool r = fill_tx_sources(sources, events, blk_0r, preminer_acc.get_keys(), m_total_money_in_minting + TESTS_DEFAULT_FEE, 0);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  std::vector<tx_destination_entry> destinations;
  for (size_t i = 0; i < m_pos_entries_to_generate; ++i)
    destinations.push_back(tx_destination_entry(pos_entry_amount, alice_acc.get_public_address()));

  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(preminer_acc.get_keys(), sources, destinations, empty_attachment, tx_1, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_1);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  ADJUST_TEST_CORE_TIME(blk_1r.timestamp);

  //DO_CALLBACK(events, "c1");
  DO_CALLBACK(events, "c3");

  return true;
}

bool pos_emission_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  // Mining scenario:
  // 0. Alice has m_total_money_in_minting coins in the wallet and this amount is being kept constant during the test (transferring the excess to Miner)
  // 1. PoW blocks go with equal intervals of DIFFICULTY_POW_TARGET
  // 2. After each PoW:
  // 2.1 Alice tries to mint as many PoS blocks as possible
  // 2.2 If no PoS blocks were minted at this timestamp - she waits for POS_SCAN_STEP secs and repeat 2.1 (within DIFFICULTY_POS_TARGET secs window) 
  // 3. Alice tries to consolidate wallet's pos entries by sending herself pos_entry_amount coins each time they become unlocked 
  //
  // The tests checks merely nothing. The only usefull output is logs

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE, false, "Incorrect numbers of blocks fetched");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  if (!check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", m_total_money_in_minting))
    return false;

  uint64_t ts = test_core_time::get_time();
  uint64_t pos_entry_amount = m_total_money_in_minting / m_pos_entries_to_generate;
  uint64_t pos_target = DIFFICULTY_POS_TARGET;
  uint64_t pow_target = DIFFICULTY_POW_TARGET;
  uint64_t last_pow_block_ts = ts - pow_target;
  //uint64_t last_pos_block_ts = ts - pos_target;
  size_t n_pos_trials = POS_SCAN_WINDOW * 2 / POS_SCAN_STEP; // number of PoS mining trials
  bool r = false;
  block b;
  wide_difficulty_type diff = 0;
  bool switched_to_higher_stake_amount = false;
  size_t height_to_twice_stake_amount = 2000;


  //std::stringstream s;
  //s << ENDL << "time" << "\t" << "height" << "\ttype\t" << "gen_coins" << "\t" << "pow_coins" << "\t" << "pos_coins" << "\t" << "already_generated_coins" << ENDL;
  LOG_PRINT2("pos_emission_test_brief.log", "\theight\tdifficulty\tstake amount\twallet balance\tpos entries count", LOG_LEVEL_0);
  for(size_t n = 0; n < 30000; ++n)
  {
    // pow
    uint64_t ideal_next_pow_block_ts = last_pow_block_ts + pow_target;
    if (ts < ideal_next_pow_block_ts)
      ts = ideal_next_pow_block_ts; // "wait" until ideal_next_pow_block_ts if it was not already happened (fast forward but don't wayback the time)
    test_core_time::adjust(ts);
    r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "invalid number of txs in tx pool: " << c.get_pool_transactions_count());

    //int64_t pow_block_timediff = ts - last_pow_block_ts;
    last_pow_block_ts = ts;

    c.get_blockchain_storage().get_top_block(b);
    //s << epee::misc_utils::get_time_str_v3(boost::posix_time::from_time_t(ts)) << "(" << ts << ")\t" << get_block_height(b) << "\tPoW\t" << b.timestamp << " (+" << pow_block_timediff << ")\t" << ENDL;

    if (get_block_height(b) > height_to_twice_stake_amount && !switched_to_higher_stake_amount)
    {
      miner_wlt->refresh();
      miner_wlt->transfer(m_total_money_in_minting, m_accounts[ALICE_ACC_IDX].get_public_address()); // transfer to Alice some coins to twice the minting wallet amount
      pos_entry_amount *= 2;
      m_total_money_in_minting *= 2;
      switched_to_higher_stake_amount = true;
    }
    
    // pos
    uint64_t pos_ts_upper_bound = ts + pos_target;
    for(uint64_t i = 0; i < n_pos_trials && ts < pos_ts_upper_bound; ++i)
    {
      alice_wlt->refresh();
      uint64_t wallet_balance_mined = 0, wallet_balance_unlocked = 0, stub = 0;
      uint64_t wallet_balance = alice_wlt->balance(wallet_balance_unlocked, stub, stub, wallet_balance_mined);
      LOG_PRINT_BLUE(">>>>>>>>>>>>>> Alice wallet balance: " << print_money(wallet_balance) << ", mined: " << print_money(wallet_balance_mined), LOG_LEVEL_0);
      if (wallet_balance > m_total_money_in_minting && wallet_balance_unlocked > 0)
      {
        uint64_t amount = std::min(wallet_balance_unlocked, wallet_balance - m_total_money_in_minting);
        size_t tx_count_before = c.get_pool_transactions_count();
        alice_wlt->transfer(amount - TESTS_DEFAULT_FEE, m_accounts[MINER_ACC_IDX].get_public_address()); // transfer out excess of money
        CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() > tx_count_before, false, "invalid number of txs in tx pool: " << c.get_pool_transactions_count());
      }

      diff = c.get_blockchain_storage().get_next_diff_conditional(true);
      uint64_t bc_height_before = c.get_blockchain_storage().get_current_blockchain_size();
      r = alice_wlt->try_mint_pos();
      if (r && bc_height_before < c.get_blockchain_storage().get_current_blockchain_size())
      {
        LOG_PRINT_GREEN(">>>>>>>>>>>>>> Minted a PoS block at " << ts << " with difficuly: " << diff, LOG_LEVEL_0);
        CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "invalid number of txs in tx pool: " << c.get_pool_transactions_count());
        c.get_blockchain_storage().get_top_block(b);
        //int64_t pos_block_timediff = b.timestamp - last_pos_block_ts;
        uint64_t stake_amount = boost::get<txin_to_key>(b.miner_tx.vin[1]).amount;
        size_t pos_entries_count = alice_wlt->get_pos_entries_count();
        //s << epee::misc_utils::get_time_str_v3(boost::posix_time::from_time_t(ts)) << "(" << ts << ")\t" << get_block_height(b) << "\tPoS\t" << b.timestamp << " (" << (pos_block_timediff > 0 ? "+" : "") << pos_block_timediff << ")\t" << diff << "\tstake: " << stake_amount << " balance: " << print_money(wallet_balance) << ", mined: " << print_money(wallet_balance_mined) << ENDL;
        LOG_PRINT2("pos_emission_test_brief.log", "\t" << get_block_height(b) << "\t" << diff << "\t" << print_money(stake_amount) << "\t" << print_money(wallet_balance) << "\t" << pos_entries_count, LOG_LEVEL_0);
        //last_pos_block_ts = b.timestamp;
        continue;
      }
      LOG_PRINT_CYAN(">>>>>>>>>>>>>> Can't mint a PoS block at " << ts << ", wait a bit...", LOG_LEVEL_0);
      ts += POS_SCAN_STEP;
      test_core_time::adjust(ts);
    }

    alice_wlt->refresh();
    uint64_t wallet_balance_unlocked = 0;
    alice_wlt->balance(wallet_balance_unlocked);
    if (wallet_balance_unlocked > pos_entry_amount)
    {
      size_t tx_count_before = c.get_pool_transactions_count();
      alice_wlt->transfer(pos_entry_amount, alice_wlt->get_account().get_public_address()); // transfer money from Alice to Alice to consolidate PoS entries for more fast minting process
      CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() > tx_count_before, false, "invalid number of txs in tx pool: " << c.get_pool_transactions_count());
    }
  }

  //LOG_PRINT2("pos_emission_test.log", s.str(), LOG_LEVEL_0);

  return true;
}

bool pos_emission_test::c2(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  // Mining scenario:

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE, false, "Incorrect numbers of blocks fetched");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  if (!check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", m_total_money_in_minting))
    return false;

  uint64_t ts = test_core_time::get_time();
  uint64_t pos_entry_amount = m_total_money_in_minting / m_pos_entries_to_generate;
  uint64_t pos_target = DIFFICULTY_POS_TARGET;
  uint64_t pow_target = DIFFICULTY_POW_TARGET;
  uint64_t last_pow_block_ts = ts - pow_target;
  //uint64_t last_pos_block_ts = ts - pos_target;
  size_t n_pos_trials = POS_SCAN_WINDOW * 2 / POS_SCAN_STEP; // number of PoS mining trials
  bool r = false;
  block b;
  wide_difficulty_type diff = 0;
  bool switched_to_higher_stake_amount = false;
  size_t height_to_twice_stake_amount = 200000;


  //std::stringstream s;
  //s << ENDL << "time" << "\t" << "height" << "\ttype\t" << "gen_coins" << "\t" << "pow_coins" << "\t" << "pos_coins" << "\t" << "already_generated_coins" << ENDL;
  LOG_PRINT2("pos_emission_test_brief_c2.log", "\theight\tdifficulty\tstake amount\twallet balance\tpos entries count", LOG_LEVEL_0);
  for(size_t n = 0; n < 30000; ++n)
  {
    // pow
    uint64_t ideal_next_pow_block_ts = last_pow_block_ts + pow_target;
    if (ts < ideal_next_pow_block_ts)
      ts = ideal_next_pow_block_ts; // "wait" until ideal_next_pow_block_ts if it was not already happened (fast forward but don't wayback the time)
    test_core_time::adjust(ts);
    r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "invalid number of txs in tx pool: " << c.get_pool_transactions_count());

    //int64_t pow_block_timediff = ts - last_pow_block_ts;
    last_pow_block_ts = ts;

    c.get_blockchain_storage().get_top_block(b);
    //s << epee::misc_utils::get_time_str_v3(boost::posix_time::from_time_t(ts)) << "(" << ts << ")\t" << get_block_height(b) << "\tPoW\t" << b.timestamp << " (+" << pow_block_timediff << ")\t" << ENDL;

    if (get_block_height(b) > height_to_twice_stake_amount && !switched_to_higher_stake_amount)
    {
      miner_wlt->refresh();
      for(size_t i = 0; i < m_pos_entries_to_generate; ++i)
        miner_wlt->transfer(pos_entry_amount, m_accounts[ALICE_ACC_IDX].get_public_address()); // transfer to Alice some coins to twice the minting wallet amount
      m_pos_entries_to_generate *= 2;
      m_total_money_in_minting *= 2;
      switched_to_higher_stake_amount = true;
    }

    // pos
    uint64_t pos_ts_upper_bound = ts + pos_target;
    for(uint64_t i = 0; i < n_pos_trials && ts < pos_ts_upper_bound; ++i)
    {
      alice_wlt->refresh();

      diff = c.get_blockchain_storage().get_next_diff_conditional(true);
      uint64_t bc_height_before = c.get_blockchain_storage().get_current_blockchain_size();
      size_t pos_entries_count = 0;
      if (mine_next_pos_block_in_playtime_with_wallet(*alice_wlt.get(), m_accounts[MINER_ACC_IDX].get_public_address(), pos_entries_count))
      {
        CHECK_AND_ASSERT_MES(bc_height_before < c.get_blockchain_storage().get_current_blockchain_size(), false, "didn't mint a block!");
        LOG_PRINT_GREEN(">>>>>>>>>>>>>> Minted a PoS block at " << ts << " with difficuly: " << diff, LOG_LEVEL_0);
        CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "invalid number of txs in tx pool: " << c.get_pool_transactions_count());
        c.get_blockchain_storage().get_top_block(b);
        //int64_t pos_block_timediff = b.timestamp - last_pos_block_ts;
        uint64_t stake_amount = boost::get<txin_to_key>(b.miner_tx.vin[1]).amount;
        //s << epee::misc_utils::get_time_str_v3(boost::posix_time::from_time_t(ts)) << "(" << ts << ")\t" << get_block_height(b) << "\tPoS\t" << b.timestamp << " (" << (pos_block_timediff > 0 ? "+" : "") << pos_block_timediff << ")\t" << diff << "\tstake: " << stake_amount << " balance: " << print_money(wallet_balance) << ", mined: " << print_money(wallet_balance_mined) << ENDL;
        LOG_PRINT2("pos_emission_test_brief_c2.log", "\t" << get_block_height(b) << "\t" << diff << "\t" << print_money(stake_amount) << "\t" << print_money(alice_wlt->balance()) << "\t" << pos_entries_count, LOG_LEVEL_0);
        //last_pos_block_ts = b.timestamp;
        continue;
      }
      LOG_PRINT_CYAN(">>>>>>>>>>>>>> Can't mint a PoS block at " << ts << ", wait a bit...", LOG_LEVEL_0);
      ts += POS_SCAN_STEP;
      test_core_time::adjust(ts);
    }
  }

  //LOG_PRINT2("pos_emission_test.log", s.str(), LOG_LEVEL_0);

  return true;
}

bool pos_emission_test::populate_wallet_with_pos_entries(std::shared_ptr<tools::wallet2> w, std::shared_ptr<tools::wallet2> coin_source, size_t iter_idx, pos_entries_change_scheme change_scheme)
{
  switch(change_scheme)
  {
  case change_pos_entries_count:
    {
      /*
      size_t coin_exponent = log10(COIN);
      size_t total_money_in_mining_exp = log10(m_total_money_in_minting);
      pos_entry_amount = random_in_range(1, 9);
      pos_entry_amount *= pow(10, random_in_range(coin_exponent, total_money_in_mining_exp - 1)); // 10 ^^ [1..100]*/

      static const double pos_entry_amounts[] = {50, 0.5, 80, 10, 100, 120, 15, 0.1};
      m_pos_entry_amount = pos_entry_amounts[iter_idx % (sizeof pos_entry_amounts / sizeof pos_entry_amounts[0])];
      m_pos_entries_to_generate = m_total_money_in_minting / m_pos_entry_amount;
    }
    break;
  case change_total_money_in_minting:
    {
      m_pos_entry_amount = random_in_range(1, 9) * 100 * COIN;
      m_total_money_in_minting = m_pos_entry_amount * m_pos_entries_to_generate;
    }
    break;
  default:
    LOG_ERROR("invalid pos_entries_change_scheme value");
    LOCAL_ASSERT(false);
  }

  LOG_PRINT_L0("changing pos entries: m_total_money_in_minting: " << m_total_money_in_minting << ", m_pos_entries_to_generate: " << m_pos_entries_to_generate << ", pos_entry_amount: " << print_money(m_pos_entry_amount));
      
  coin_source->refresh();
  // populate current wallet with pos entries from scratch
  for(size_t i = 0; i < m_pos_entries_to_generate; ++i)
    coin_source->transfer(m_pos_entry_amount, w->get_account().get_public_address()); // transfer to current wallet stake coins

  uint64_t total_stake_transferred = m_pos_entries_to_generate * m_pos_entry_amount;
  CHECK_AND_ASSERT_MES(total_stake_transferred <= m_total_money_in_minting, false, "total_stake_transferred: " << total_stake_transferred << ", m_total_money_in_minting: " << m_total_money_in_minting);
  uint64_t remainder = m_total_money_in_minting - total_stake_transferred;
  if (remainder > TESTS_DEFAULT_FEE)
    coin_source->transfer(remainder, w->get_account().get_public_address());
  
  return true;
}

bool pos_emission_test::c3(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  // Mining scenario:

#define LOG1_FILENAME "pos_emission_test_brief_c3.log"
#define LOG2_FILENAME "pos_emission_test_brief_c3_aux.log"


  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  std::shared_ptr<tools::wallet2> dan_wlt = init_playtime_test_wallet(events, c, DAN_ACC_IDX); // Dan is a preminer with buttloads of money
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE, false, "Incorrect numbers of blocks fetched");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  if (!check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", m_total_money_in_minting))
    return false;

  uint64_t ts = test_core_time::get_time();
  //uint64_t pos_entry_amount = m_total_money_in_minting / m_pos_entries_to_generate;
  uint64_t pos_target = DIFFICULTY_POS_TARGET;
  uint64_t pow_target = DIFFICULTY_POW_TARGET;
  uint64_t last_pow_block_ts = ts - pow_target;
  //uint64_t last_pos_block_ts = ts - pos_target; // block_header::ts
  //uint64_t last_pos_block_ts_real = ts - pos_target; // real time at the moment of successfull minting
  size_t n_pos_trials = POS_SCAN_WINDOW * 2 / POS_SCAN_STEP; // number of PoS mining trials
  bool r = false;
  block b;
  wide_difficulty_type diff = 0;
  size_t block_n_to_change_pos_entries = 5000;
  size_t next_block_height_to_change_pos_entries = 0; // start with pos entries population as soon as possible
  size_t change_pos_entries_iter_index = 0;
  std::shared_ptr<tools::wallet2> current_wallet = alice_wlt;


  LOG_PRINT2(LOG1_FILENAME, "\theight\tdifficulty\tstake amount\twallet balance\tpos entries count\tstake to diff coeff\tpow seq factor\tpos seq factor", LOG_LEVEL_0);
  LOG_PRINT2_JORNAL(LOG2_FILENAME, "\theight", LOG_LEVEL_0);
  for(size_t n = 0; n < 30000; ++n)
  {
    // pow
    uint64_t ideal_next_pow_block_ts = last_pow_block_ts + pow_target;
    if (ts < ideal_next_pow_block_ts)
      ts = ideal_next_pow_block_ts; // "wait" until ideal_next_pow_block_ts if it was not already happened (fast forward but don't wayback the time)
    test_core_time::adjust(ts);
    size_t tx_count_before = c.get_pool_transactions_count();
    r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
    CHECK_AND_ASSERT_MES(tx_count_before == 0 || c.get_pool_transactions_count() < tx_count_before, false, "invalid number of txs in tx pool: " << c.get_pool_transactions_count() << ", was: " << tx_count_before);
    //uint64_t pow_blocks_interval = ts - last_pow_block_ts;
    last_pow_block_ts = ts;

    c.get_blockchain_storage().get_top_block(b);
    size_t height = get_block_height(b);

    // pos entries changing mechanism
    if (height >= next_block_height_to_change_pos_entries)
    {
      next_block_height_to_change_pos_entries += block_n_to_change_pos_entries;
      
      // switch wallets (so new one is always has all outs unlocked -- to be able to transfers out all the money)
      if (current_wallet == alice_wlt)
        current_wallet = bob_wlt;
      else
        current_wallet = alice_wlt;

      // clear wallet by transferring out all the coins
      current_wallet->refresh();
      uint64_t unlocked_balance = 0;
      uint64_t total_balance = current_wallet->balance(unlocked_balance);
      CHECK_AND_ASSERT_MES(unlocked_balance == total_balance, false, "Total and unlocked balances don't equal: " << unlocked_balance << " != " << total_balance);
      if (unlocked_balance > TESTS_DEFAULT_FEE)
        current_wallet->transfer(unlocked_balance - TESTS_DEFAULT_FEE, m_accounts[MINER_ACC_IDX].get_public_address()); // transfer out all the money to Miner -- clear pos entries

      r = populate_wallet_with_pos_entries(current_wallet, dan_wlt, change_pos_entries_iter_index++, pos_entries_change_scheme::change_pos_entries_count);
      CHECK_AND_ASSERT_MES(r, false, "populate_wallet_with_pos_entries failed");
    }

    // pos
    uint64_t pos_ts_upper_bound = ts + pos_target / 2;
    for(uint64_t i = 0; i < n_pos_trials && ts < pos_ts_upper_bound; ++i)
    {
      size_t pow_seq_factor = c.get_blockchain_storage().get_current_sequence_factor(false);
      size_t pos_seq_factor = c.get_blockchain_storage().get_current_sequence_factor(true);
      current_wallet->refresh();
      diff = c.get_blockchain_storage().get_next_diff_conditional(true);
      uint64_t bc_height_before = c.get_blockchain_storage().get_current_blockchain_size();
      size_t pos_entries_count = 0;
      if (mine_next_pos_block_in_playtime_with_wallet(*current_wallet.get(), m_accounts[MINER_ACC_IDX].get_public_address(), pos_entries_count))
      {
        CHECK_AND_ASSERT_MES(bc_height_before < c.get_blockchain_storage().get_current_blockchain_size(), false, "didn't mint a block!");
        LOG_PRINT_GREEN(">>>>>>>>>>>>>> Minted a PoS block at " << ts << " with difficuly: " << diff, LOG_LEVEL_0);
        CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "invalid number of txs in tx pool: " << c.get_pool_transactions_count());
        c.get_blockchain_storage().get_top_block(b);
        //int64_t pos_block_timediff = b.timestamp - last_pos_block_ts;
        uint64_t stake_amount = boost::get<txin_to_key>(b.miner_tx.vin[1]).amount;
        double pos_total_stake_to_diff_coeff = static_cast<double>(diff / COIN) / static_cast<double>(m_total_money_in_minting / COIN);
        LOG_PRINT2("pos_emission_test_brief_c3.log", "\t" << get_block_height(b) << "\t" << diff << "\t" << print_money(stake_amount) << "\t" << print_money(current_wallet->balance()) << "\t" << pos_entries_count <<
          "\t" << pos_total_stake_to_diff_coeff << "\t" << pow_seq_factor << "\t" << pos_seq_factor, LOG_LEVEL_0);
        //last_pos_block_ts = b.timestamp;
        //last_pos_block_ts_real = ts;
        continue;
      }
      LOG_PRINT_CYAN(">>>>>>>>>>>>>> Can't mint a PoS block at " << ts << ", wait a bit...", LOG_LEVEL_0);
      ts += POS_SCAN_STEP;
      test_core_time::adjust(ts);
    }
  }

  return true;

#undef LOG1_FILENAME
#undef LOG2_FILENAME
}
