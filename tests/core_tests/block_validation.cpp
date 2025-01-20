// Copyright (c) 2014-2025 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "block_validation.h"

using namespace epee;
using namespace currency;

namespace
{
  bool lift_up_difficulty(std::vector<test_event_entry>& events, std::vector<uint64_t>& timestamps,
                          std::vector<wide_difficulty_type>& cummulative_difficulties, test_generator& generator,
                          size_t new_block_count, const block blk_last, const account_base& miner_account)
  {
    wide_difficulty_type commulative_diffic = cummulative_difficulties.empty() ? 0 : cummulative_difficulties.back();
    block blk_prev = blk_last;
    for (size_t i = 0; i < new_block_count; ++i)
    {
      block blk_next;
      wide_difficulty_type diffic = next_difficulty_1(timestamps, cummulative_difficulties, DIFFICULTY_POW_TARGET, DIFFICULTY_POW_STARTER);
      if (!generator.construct_block_manually(blk_next, blk_prev, miner_account,
        test_generator::bf_timestamp | test_generator::bf_diffic, 0, 0, blk_prev.timestamp, crypto::hash(), diffic))
        return false;

      commulative_diffic += diffic;
      if (timestamps.size() == DIFFICULTY_WINDOW)
      {
        timestamps.erase(timestamps.begin());
        cummulative_difficulties.erase(cummulative_difficulties.begin());
      }
      //TODO: VERY ineffective way, need to rewrite
      timestamps.insert(timestamps.begin(), blk_next.timestamp);
      cummulative_difficulties.insert(cummulative_difficulties.begin(), commulative_diffic);

      events.push_back(blk_next);
      blk_prev = blk_next;
    }

    return true;
  }
}

#define BLOCK_VALIDATION_INIT_GENERATE()                                                \
  GENERATE_ACCOUNT(miner_account);                                                      \
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, 1338224400);                         \
  DO_CALLBACK(events, "configure_core");

//----------------------------------------------------------------------------------------------------------------------
// Tests

bool gen_block_big_major_version::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();

  block blk_1;
  generator.construct_block_manually(blk_1, blk_0, miner_account, test_generator::bf_major_ver, CURRENT_BLOCK_MAJOR_VERSION + 1);
  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_purged");

  return true;
}

bool gen_block_big_minor_version::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();

  block blk_1;
  generator.construct_block_manually(blk_1, blk_0, miner_account, test_generator::bf_minor_ver, 0, CURRENT_BLOCK_MINOR_VERSION + 1);
  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_accepted");

  return true;
}

bool gen_block_ts_not_checked::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_account, BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW - 2);

  block blk_1;
  generator.construct_block_manually(blk_1, blk_0r, miner_account, test_generator::bf_timestamp, 0, 0, blk_0.timestamp - 60 * 60);
  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_accepted");

  return true;
}

bool gen_block_ts_in_past::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_account, BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW - 1);

  uint64_t ts_below_median = boost::get<block>(events[BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW / 2 - 1]).timestamp;
  block blk_1;
  generator.construct_block_manually(blk_1, blk_0r, miner_account, test_generator::bf_timestamp, 0, 0, ts_below_median);
  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_purged");

  return true;
}

bool gen_block_ts_in_future::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();
  block blk_1;
  generator.construct_block_manually(blk_1, blk_0, miner_account, test_generator::bf_timestamp, 0, 0, time(NULL) + 60*60 + CURRENCY_BLOCK_FUTURE_TIME_LIMIT);
  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_purged");

  return true;
}

bool gen_block_invalid_prev_id::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();
  block blk_1;
  crypto::hash prev_id = get_block_hash(blk_0);
  reinterpret_cast<char &>(prev_id) ^= 1;
  generator.construct_block_manually(blk_1, blk_0, miner_account, test_generator::bf_prev_id, 0, 0, 0, prev_id);
  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_purged");

  return true;
}

bool gen_block_invalid_prev_id::check_block_verification_context(const currency::block_verification_context& bvc, size_t event_idx, const currency::block& /*blk*/)
{
  if (1 == event_idx)
    return bvc.m_marked_as_orphaned && !bvc.m_added_to_main_chain && !bvc.m_verification_failed;
  else
    return !bvc.m_marked_as_orphaned && bvc.m_added_to_main_chain && !bvc.m_verification_failed;
}

bool gen_block_invalid_nonce::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();
  std::vector<uint64_t> timestamps;
  std::vector<wide_difficulty_type> commulative_difficulties;
  if (!lift_up_difficulty(events, timestamps, commulative_difficulties, generator, 2, blk_0, miner_account))
    return false;

  // Create invalid nonce
  wide_difficulty_type diffic = next_difficulty_1(timestamps, commulative_difficulties, DIFFICULTY_POW_TARGET, DIFFICULTY_POW_STARTER);
  CHECK_AND_ASSERT_MES(diffic > 1, false, "diffic > 1 validation failed");
  const block& blk_last = boost::get<block>(events.back());
  uint64_t timestamp = blk_last.timestamp;
  block blk_3;
  do
  {
    ++timestamp;
    blk_3.miner_tx = AUTO_VAL_INIT(blk_3.miner_tx);
    if (!generator.construct_block_manually(blk_3, blk_last, miner_account,
      test_generator::bf_diffic | test_generator::bf_timestamp, 0, 0, timestamp, crypto::hash(), diffic))
      return false;
  }
  while (0 == blk_3.nonce);
  --blk_3.nonce;
  events.push_back(blk_3);

  return true;
}

bool gen_block_no_miner_tx::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();
  transaction miner_tx;
  miner_tx = AUTO_VAL_INIT(miner_tx);

  block blk_1;
  generator.construct_block_manually(blk_1, blk_0, miner_account, test_generator::bf_miner_tx, 0, 0, 0, crypto::hash(), 0, miner_tx);
  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_purged");

  return true;
}

bool gen_block_unlock_time_is_low::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();

  MAKE_MINER_TX_MANUALLY(miner_tx, blk_0);
  currency::set_tx_unlock_time(miner_tx, currency::get_tx_max_unlock_time(miner_tx) - 1);

  block blk_1;
  generator.construct_block_manually(blk_1, blk_0, miner_account, test_generator::bf_miner_tx, 0, 0, 0, crypto::hash(), 0, miner_tx);
  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_purged");

  return true;
}

bool gen_block_unlock_time_is_high::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();
  MAKE_MINER_TX_MANUALLY(miner_tx, blk_0);
  set_tx_unlock_time(miner_tx, get_tx_max_unlock_time(miner_tx) + 1);

  block blk_1;
  generator.construct_block_manually(blk_1, blk_0, miner_account, test_generator::bf_miner_tx, 0, 0, 0, crypto::hash(), 0, miner_tx);
  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_purged");

  return true;
}

bool gen_block_unlock_time_is_timestamp_in_past::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();

  MAKE_MINER_TX_MANUALLY(miner_tx, blk_0);
  set_tx_unlock_time(miner_tx, blk_0.timestamp - 10 * 60);

  block blk_1;
  generator.construct_block_manually(blk_1, blk_0, miner_account, test_generator::bf_miner_tx, 0, 0, 0, crypto::hash(), 0, miner_tx);
  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_purged");

  return true;
}

bool gen_block_unlock_time_is_timestamp_in_future::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();
  MAKE_MINER_TX_MANUALLY(miner_tx, blk_0);
  set_tx_unlock_time(miner_tx, blk_0.timestamp + 3 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW * DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN);

  block blk_1;
  generator.construct_block_manually(blk_1, blk_0, miner_account, test_generator::bf_miner_tx, 0, 0, 0, crypto::hash(), 0, miner_tx);
  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_purged");

  return true;
}

bool gen_block_height_is_low::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();
  MAKE_MINER_TX_MANUALLY(miner_tx, blk_0);
  boost::get<txin_gen>(miner_tx.vin[0]).height--;

  block blk_1;
  generator.construct_block_manually(blk_1, blk_0, miner_account, test_generator::bf_miner_tx, 0, 0, 0, crypto::hash(), 0, miner_tx);
  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_purged");

  return true;
}

bool gen_block_height_is_high::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();
  MAKE_MINER_TX_MANUALLY(miner_tx, blk_0);
  boost::get<txin_gen>(miner_tx.vin[0]).height++;

  block blk_1;
  generator.construct_block_manually(blk_1, blk_0, miner_account, test_generator::bf_miner_tx, 0, 0, 0, crypto::hash(), 0, miner_tx);
  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_purged");

  return true;
}

bool gen_block_miner_tx_has_2_tx_gen_in::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();
  MAKE_MINER_TX_MANUALLY(miner_tx, blk_0);

  txin_gen in;
  in.height = get_block_height(blk_0) + 1;
  miner_tx.vin.push_back(in);

  block blk_1;
  generator.construct_block_manually(blk_1, blk_0, miner_account, test_generator::bf_miner_tx, 0, 0, 0, crypto::hash(), 0, miner_tx);
  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_purged");

  return true;
}

bool gen_block_miner_tx_has_2_in::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  GENERATE_ACCOUNT(alice);

  tx_source_entry se = AUTO_VAL_INIT(se);
  se.amount = boost::get<currency::tx_out_bare>(blk_0.miner_tx.vout[0]).amount;
  currency::tx_source_entry::output_entry oe = AUTO_VAL_INIT(oe);
  oe.out_reference = 0;
  oe.stealth_address = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(blk_0.miner_tx.vout[0]).target).key;
  se.outputs.push_back(oe);
  se.real_output = 0;
  se.real_out_tx_key = get_tx_pub_key_from_extra(blk_0.miner_tx);
  se.real_output_in_tx_index = 0;
  std::vector<tx_source_entry> sources;
  sources.push_back(se);

  tx_destination_entry de = AUTO_VAL_INIT(de);
  de.addr.push_back(miner_account.get_keys().account_address);
  de.amount = se.amount;
  std::vector<tx_destination_entry> destinations;
  destinations.push_back(de);

  transaction tmp_tx;
  std::vector<currency::attachment_v> attachments;

  uint64_t tx_version = get_tx_version(get_block_height(blk_0r), m_hardforks);
  if (!construct_tx(miner_account.get_keys(), sources, destinations, attachments, tmp_tx, tx_version, 0))
    return false;

  MAKE_MINER_TX_MANUALLY(miner_tx, blk_0);
  miner_tx.vin.push_back(tmp_tx.vin[0]);

  block blk_1 = AUTO_VAL_INIT(blk_1);
  generator.construct_block_manually(blk_1, blk_0r, miner_account, test_generator::bf_miner_tx, 0, 0, 0, crypto::hash(), 0, miner_tx);
  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_purged");

  return true;
}

bool gen_block_miner_tx_with_txin_to_key::generate(std::vector<test_event_entry>& events) const
{  
  BLOCK_VALIDATION_INIT_GENERATE();

  // This block has only one output
  block blk_1 = AUTO_VAL_INIT(blk_1);
  generator.construct_block_manually(blk_1, blk_0, miner_account, test_generator::bf_none);
  events.push_back(blk_1);

  REWIND_BLOCKS(events, blk_1r, blk_1, miner_account);

  tx_source_entry se = AUTO_VAL_INIT(se);
  se.amount = boost::get<currency::tx_out_bare>(blk_1.miner_tx.vout[0]).amount;
  currency::tx_source_entry::output_entry oe = AUTO_VAL_INIT(oe);
  oe.out_reference = 0;
  oe.stealth_address = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(blk_1.miner_tx.vout[0]).target).key;
  se.outputs.push_back(oe);
  se.real_output = 0;
  se.real_out_tx_key = get_tx_pub_key_from_extra(blk_1.miner_tx);
  se.real_output_in_tx_index = 0;
  std::vector<tx_source_entry> sources;
  sources.push_back(se);

  tx_destination_entry de = AUTO_VAL_INIT(de);
  de.addr.push_back(miner_account.get_keys().account_address);
  de.amount = se.amount;
  std::vector<tx_destination_entry> destinations;
  destinations.push_back(de);

  transaction tmp_tx = AUTO_VAL_INIT(tmp_tx);
  std::vector<currency::attachment_v> attachments;
  uint64_t tx_version = get_tx_version(get_block_height(blk_1r), m_hardforks);
  if (!construct_tx(miner_account.get_keys(), sources, destinations, attachments, tmp_tx, tx_version, 0))
    return false;

  MAKE_MINER_TX_MANUALLY(miner_tx, blk_1);
  miner_tx.vin[0] = tmp_tx.vin[0];

  block blk_2 = AUTO_VAL_INIT(blk_2);
  generator.construct_block_manually(blk_2, blk_1r, miner_account, test_generator::bf_miner_tx, 0, 0, 0, crypto::hash(), 0, miner_tx);
  events.push_back(blk_2);

  DO_CALLBACK(events, "check_block_purged");

  return true;
}

bool gen_block_miner_tx_out_is_small::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();
  MAKE_MINER_TX_MANUALLY(miner_tx, blk_0);
boost::get<currency::tx_out_bare>(  miner_tx.vout[0]).amount /= 2;

  block blk_1;
  generator.construct_block_manually(blk_1, blk_0, miner_account, test_generator::bf_miner_tx, 0, 0, 0, crypto::hash(), 0, miner_tx);
  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_purged");

  return true;
}

bool gen_block_miner_tx_out_is_big::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();
  MAKE_MINER_TX_MANUALLY(miner_tx, blk_0);
boost::get<currency::tx_out_bare>(  miner_tx.vout[0]).amount *= 2;

  block blk_1;
  generator.construct_block_manually(blk_1, blk_0, miner_account, test_generator::bf_miner_tx, 0, 0, 0, crypto::hash(), 0, miner_tx);
  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_purged");

  return true;
}

bool gen_block_miner_tx_has_no_out::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();
  MAKE_MINER_TX_MANUALLY(miner_tx, blk_0);
  miner_tx.vout.clear();

  block blk_1;
  generator.construct_block_manually(blk_1, blk_0, miner_account, test_generator::bf_miner_tx, 0, 0, 0, crypto::hash(), 0, miner_tx);
  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_purged");

  return true;
}

bool gen_block_miner_tx_has_out_to_initiator::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();
  GENERATE_ACCOUNT(alice);

  keypair txkey;
  MAKE_MINER_TX_AND_KEY_MANUALLY(miner_tx, blk_0, &txkey);

  crypto::key_derivation derivation;
  crypto::public_key out_eph_public_key;
  crypto::generate_key_derivation(alice.get_keys().account_address.view_public_key, txkey.sec, derivation);
  crypto::derive_public_key(derivation, 1, alice.get_keys().account_address.spend_public_key, out_eph_public_key);

  tx_out_bare out_to_initiator;
  out_to_initiator.amount =boost::get<currency::tx_out_bare>( miner_tx.vout[0]).amount / 2;
boost::get<currency::tx_out_bare>(  miner_tx.vout[0]).amount -= out_to_initiator.amount;
  out_to_initiator.target = txout_to_key(out_eph_public_key);
  miner_tx.vout.push_back(out_to_initiator);

  block blk_1;
  generator.construct_block_manually(blk_1, blk_0, miner_account, test_generator::bf_miner_tx, 0, 0, 0, crypto::hash(), 0, miner_tx);
  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_accepted");

  return true;
}

bool gen_block_has_invalid_tx::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();
  std::vector<crypto::hash> tx_hashes;
  tx_hashes.push_back(crypto::hash());

  block blk_1;
  generator.construct_block_manually_tx(blk_1, blk_0, miner_account, tx_hashes, 0);
  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_purged");

  return true;
}

bool gen_block_is_too_big::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();
  // Creating a huge miner_tx, it will have a lot of outs
  MAKE_MINER_TX_MANUALLY(miner_tx, blk_0);
  static const size_t tx_out_count = CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE / 2;
  uint64_t amount = get_outs_money_amount(miner_tx);
  uint64_t portion = amount / tx_out_count;
  uint64_t remainder = amount % tx_out_count;
  txout_target_v target =boost::get<currency::tx_out_bare>( miner_tx.vout[0]).target;
  miner_tx.vout.clear();
  for (size_t i = 0; i < tx_out_count; ++i)
  {
    tx_out_bare o;
    o.amount = portion;
    o.target = target;
    miner_tx.vout.push_back(o);
  }
  if (0 < remainder)
  {
    tx_out_bare o;
    o.amount = remainder;
    o.target = target;
    miner_tx.vout.push_back(o);
  }

  // Block reward will be incorrect, as it must be reduced if cumulative block size is very big,
  // but in this test it doesn't matter
  block blk_1;
  if (!generator.construct_block_manually(blk_1, blk_0, miner_account, test_generator::bf_miner_tx, 0, 0, 0, crypto::hash(), 0, miner_tx))
    return false;

  events.push_back(blk_1);

  DO_CALLBACK(events, "check_block_purged");

  return true;
}

gen_block_invalid_binary_format::gen_block_invalid_binary_format()
  : m_corrupt_blocks_begin_idx(0)
{
  REGISTER_CALLBACK("check_all_blocks_purged", gen_block_invalid_binary_format::check_all_blocks_purged);
  REGISTER_CALLBACK("corrupt_blocks_boundary", gen_block_invalid_binary_format::corrupt_blocks_boundary);
}

bool gen_block_invalid_binary_format::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();
  //wide_difficulty_type cummulative_diff = 1;

  // Unlock blk_0 outputs
  block blk_last = blk_0;
  assert(CURRENCY_MINED_MONEY_UNLOCK_WINDOW < DIFFICULTY_WINDOW);
  for (size_t i = 0; i < CURRENCY_MINED_MONEY_UNLOCK_WINDOW; ++i)
  {
    MAKE_NEXT_BLOCK(events, blk_curr, blk_last, miner_account);
    blk_last = blk_curr; 
  }

  // Lifting up takes a while
  wide_difficulty_type diffic;
  do
  {
    MAKE_NEXT_BLOCK(events, blk_curr, blk_last, miner_account);
    diffic = generator.get_block_difficulty(get_block_hash(blk_curr));
    std::cout << "Block #" << events.size() << ", difficulty: " << diffic << std::endl;
    blk_last = blk_curr;
  }
  while (diffic < 200);

  blk_last = boost::get<block>(events.back());
  MAKE_TX(events, tx_0, miner_account, miner_account, MK_TEST_COINS(120), blk_last);
  DO_CALLBACK(events, "corrupt_blocks_boundary");

  block blk_test;
  std::vector<crypto::hash> tx_hashes;
  tx_hashes.push_back(get_transaction_hash(tx_0));
  size_t txs_size = get_object_blobsize(tx_0);
  diffic = generator.get_difficulty_for_next_block(get_block_hash(blk_last)); 
  if (!generator.construct_block_manually(blk_test, blk_last, miner_account,
    test_generator::bf_diffic | test_generator::bf_timestamp | test_generator::bf_tx_hashes, 0, 0, blk_last.timestamp,
    crypto::hash(), diffic, transaction(), tx_hashes, txs_size))
    return false;

  blobdata blob = t_serializable_object_to_blob(blk_test);
  for (size_t i = 0; i < blob.size(); ++i)
  {
    for (size_t bit_idx = 0; bit_idx < sizeof(blobdata::value_type) * 8; ++bit_idx)
    {
      serialized_block sr_block(blob);
      blobdata::value_type& ch = sr_block.data[i];
      ch ^= 1 << bit_idx;

      events.push_back(sr_block);
    }
  }

  DO_CALLBACK(events, "check_all_blocks_purged");

  return true;
}

bool gen_block_invalid_binary_format::check_block_verification_context(const currency::block_verification_context& bvc,
                                                                       size_t event_idx, const currency::block& blk)
{
  if (0 == m_corrupt_blocks_begin_idx || event_idx < m_corrupt_blocks_begin_idx)
  {
    return bvc.m_added_to_main_chain;
  }
  else
  {
    return !bvc.m_added_to_main_chain && (bvc.m_already_exists || bvc.m_marked_as_orphaned || bvc.m_verification_failed);
  }
}

bool gen_block_invalid_binary_format::corrupt_blocks_boundary(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  m_corrupt_blocks_begin_idx = ev_index + 1;
  return true;
}

bool gen_block_invalid_binary_format::check_all_blocks_purged(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{

  CHECK_EQ(1, c.get_pool_transactions_count());
  CHECK_EQ(m_corrupt_blocks_begin_idx - 2, c.get_current_blockchain_size());

  return true;
}


gen_block_wrong_version_agains_hardfork::gen_block_wrong_version_agains_hardfork()
{
  REGISTER_CALLBACK("c1", gen_block_wrong_version_agains_hardfork::c1);
}

bool gen_block_wrong_version_agains_hardfork::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{

  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE; //four blocks
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH; //four blocks
  pc.hard_forks.set_hardfork_height(1, 10);
  pc.hard_forks.set_hardfork_height(2, 10);
  pc.hard_forks.set_hardfork_height(3, 10);
  c.get_blockchain_storage().set_core_runtime_config(pc);

  currency::account_base mining_accunt;
  mining_accunt.generate();
  
  bool r = mine_next_pow_block_in_playtime(mining_accunt.get_public_address(), c); // block with height 1

  uint8_t major_version_to_set = 0;
  uint8_t minor_version_to_set = 0;
  auto cb = [&] (currency::block& b)
  {
    b.major_version = major_version_to_set;
    b.minor_version = minor_version_to_set;
  };

  //between 1 and 2 hardforks
  pc.hard_forks.set_hardfork_height(1, 1);
  pc.hard_forks.set_hardfork_height(2, 10);
  pc.hard_forks.set_hardfork_height(3, 20);
  c.get_blockchain_storage().set_core_runtime_config(pc);

  //major unknown
  major_version_to_set = 2;

  r = mine_next_pow_block_in_playtime(mining_accunt.get_public_address(), c, cb); // block with height 2 (won't pass)
  CHECK_TEST_CONDITION(!r);

  //minor unknown
  major_version_to_set = 1;
  minor_version_to_set = 2;
  r = mine_next_pow_block_in_playtime(mining_accunt.get_public_address(), c, cb); // block with height 2
  CHECK_TEST_CONDITION(r);

  //between 1 and 2 hardforks
  pc.hard_forks.set_hardfork_height(1, 1);
  pc.hard_forks.set_hardfork_height(2, 1);
  pc.hard_forks.set_hardfork_height(3, 1);
  c.get_blockchain_storage().set_core_runtime_config(pc);


  //major correct 
  major_version_to_set = 2;
  minor_version_to_set = 0;
  r = mine_next_pow_block_in_playtime(mining_accunt.get_public_address(), c, cb); // block with height 3
  CHECK_TEST_CONDITION(r);

  //major incorrect 
  major_version_to_set = 3;
  minor_version_to_set = 0;
  r = mine_next_pow_block_in_playtime(mining_accunt.get_public_address(), c, cb); // block with height 4 (won't pass)
  CHECK_TEST_CONDITION(!r);

  //minor  incorrect for hf3
  major_version_to_set = 2;
  minor_version_to_set = 1;
  r = mine_next_pow_block_in_playtime(mining_accunt.get_public_address(), c, cb); // block with height 4  (won't pass)
  CHECK_TEST_CONDITION(!r);

  //major  lower then normal for hf3 (do we need this half-working backward compability? nope, hardfork 3 always put HF3_BLOCK_MAJOR_VERSION in major version )
  major_version_to_set = 0;
  minor_version_to_set = 0;
  r = mine_next_pow_block_in_playtime(mining_accunt.get_public_address(), c, cb); // block with height 4  (won't pass)
  CHECK_TEST_CONDITION(!r);

  return true;
}

bool gen_block_wrong_version_agains_hardfork::generate(std::vector<test_event_entry>& events) const
{
  BLOCK_VALIDATION_INIT_GENERATE();
  DO_CALLBACK(events, "c1");
  return true;
}

block_with_correct_prev_id_on_wrong_height::block_with_correct_prev_id_on_wrong_height()
{
  REGISTER_CALLBACK("assert_blk_2_has_wrong_height", block_with_correct_prev_id_on_wrong_height::assert_blk_2_has_wrong_height);
}

bool block_with_correct_prev_id_on_wrong_height::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure that a block with correct previous block identifier that is at the wrong height won't be accepted by the core.

  GENERATE_ACCOUNT(miner);
  MAKE_GENESIS_BLOCK(events, blk_0, miner, test_core_time::get_time());

  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  MAKE_TX(events, tx_0, miner, miner, MK_TEST_COINS(2), blk_0r);
  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, miner);
  m_blk_2.prev_id = currency::get_block_hash(blk_1);
  m_blk_2.miner_tx = blk_0r.miner_tx;
  m_blk_2.major_version = blk_0r.major_version;
  // blk_1 is the previous for blk_2, but height(blk_2) < height(blk_1).
  CHECK_AND_ASSERT_EQ(currency::get_block_height(blk_1), 11);
  CHECK_AND_ASSERT_EQ(currency::get_block_height(m_blk_2), 10);
  DO_CALLBACK(events, "assert_blk_2_has_wrong_height");

  return true;
}

bool block_with_correct_prev_id_on_wrong_height::assert_blk_2_has_wrong_height(currency::core& c, [[maybe_unused]] size_t ev_index, [[maybe_unused]] const std::vector<test_event_entry>& events) const
{
  currency::block_verification_context context_blk_2{};

  CHECK_AND_ASSERT_EQ(boost::get<txin_gen>(m_blk_2.miner_tx.vin.front()).height, 10);
  CHECK_AND_ASSERT_EQ(c.get_blockchain_storage().add_new_block(m_blk_2, context_blk_2), false);

  return true;
}

struct block_reward_in_main_chain_basic::argument_assert
{
  uint64_t m_height{}, m_balance{};
  std::list<uint64_t> m_rewards{};

  argument_assert() = default;

  template<typename test>
  argument_assert(const test* instance, uint64_t height, const std::list<uint64_t>& blk_tx_fees = {}, const argument_assert previous = {}) : m_height{height}
  {
    CHECK_AND_ASSERT_THROW(instance, std::runtime_error{"Pointer to an instance of the test equals to the nullptr."});
    CHECK_AND_ASSERT_THROW(m_height >= previous.m_height, std::runtime_error{"A height specified in the previous argument is greather than current height."});

    if (height == 0)
    {
      m_rewards.push_back(PREMINE_AMOUNT);
      m_balance = m_rewards.back();
    }

    else
    {
      m_balance = previous.m_balance;

      for (auto fee_iterator{blk_tx_fees.rbegin()}; fee_iterator != blk_tx_fees.rend(); ++fee_iterator)
      {
        if (height != 0)
        {
          m_rewards.push_back(COIN);

          if (const auto& fee{*fee_iterator}; fee <= m_rewards.back())
          {
            if (instance->m_hardforks.is_hardfork_active_for_height(ZANO_HARDFORK_04_ZARCANUM, height))
            {
              m_rewards.back() -= fee;
            }
          }

          m_balance += m_rewards.back();
        }

        else
        {
          m_balance += PREMINE_AMOUNT;
        }

        --height;
      }
    }

    CHECK_AND_ASSERT_THROW(m_rewards.size() > 0, std::runtime_error{"A list of the rewards is empty."});
  }

  BEGIN_SERIALIZE()
    FIELD(m_height)
    FIELD(m_balance)
    FIELD(m_rewards)
  END_SERIALIZE()
};

block_reward_in_main_chain_basic::block_reward_in_main_chain_basic()
{
  REGISTER_CALLBACK_METHOD(block_reward_in_main_chain_basic, assert_balance);
  REGISTER_CALLBACK_METHOD(block_reward_in_main_chain_basic, assert_reward);
}

bool block_reward_in_main_chain_basic::generate(std::vector<test_event_entry>& events) const
{
  // The test idea: make sure that receiving rewards for block insertion is counted correctly.

  const auto assert_balance{[&events](const argument_assert& argument) -> void
    {
      DO_CALLBACK_PARAMS_STR(events, "assert_balance", t_serializable_object_to_blob(argument));
    }
  };

  const auto assert_reward{[&events](const argument_assert& argument) -> void
    {
      DO_CALLBACK_PARAMS_STR(events, "assert_reward", t_serializable_object_to_blob(argument));
    }
  };

  GENERATE_ACCOUNT(miner);
  MAKE_GENESIS_BLOCK(events, blk_0, miner, test_core_time::get_time());
  argument_assert argument{this, get_block_height(blk_0)};

  m_accounts.push_back(miner);
  assert_reward(argument);
  // Make sure the balance equals to the PREMINE_AMOUNT.
  assert_balance(argument);
  DO_CALLBACK(events, "configure_core");

  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner);

  argument = argument_assert{this, get_block_height(blk_1), {0}, argument};
  assert_balance(argument);
  assert_reward(argument);

  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner, CURRENCY_MINED_MONEY_UNLOCK_WINDOW - 1);

  argument = argument_assert{this, get_block_height(blk_1r), std::list<uint64_t>(CURRENCY_MINED_MONEY_UNLOCK_WINDOW - 1), argument};
  assert_balance(argument);
  assert_reward(argument);

  MAKE_TX_FEE(events, tx_0, miner, miner, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, blk_1r);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner, tx_0);

  argument = {this, get_block_height(blk_2), {TESTS_DEFAULT_FEE}, argument};
  assert_reward(argument);
  // Make sure in the balance change in the case of a transaction with the default fee.
  assert_balance(argument);

  MAKE_TX_FEE(events, tx_1, miner, miner, MK_TEST_COINS(3), 2 * TESTS_DEFAULT_FEE, blk_2);
  MAKE_TX_FEE(events, tx_2, miner, miner, MK_TEST_COINS(2), 3 * TESTS_DEFAULT_FEE, blk_2);

  const std::list txs{tx_1, tx_2};
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3, blk_2, miner, txs);

  argument = argument_assert{this, get_block_height(blk_3), {(2 + 3) * TESTS_DEFAULT_FEE}, argument};
  assert_reward(argument);
  // A case of one inserted block with a several transactions with a non default fees.
  assert_balance(argument);

  MAKE_TX_FEE(events, tx_3, miner, miner, MK_TEST_COINS(1), COIN + TESTS_DEFAULT_FEE, blk_3);
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner);

  argument = argument_assert{this, get_block_height(blk_4), {COIN + TESTS_DEFAULT_FEE}, argument};
  assert_reward(argument);
  // A transaction inside blk_4 has a fee greater than a block reward. blk_4 includes only one transaction.
  assert_balance(argument);

  MAKE_TX_FEE(events, tx_4, miner, miner, MK_TEST_COINS(1), 76 * COIN + 14 * TESTS_DEFAULT_FEE, blk_4);
  MAKE_NEXT_BLOCK(events, blk_5, blk_4, miner);

  argument = argument_assert{this, get_block_height(blk_5), {76 * COIN + 14 * TESTS_DEFAULT_FEE}, argument};
  assert_reward(argument);
  // A transaction inside blk_5 has a fee greater than a block reward. blk_5 includes only one transaction.
  assert_balance(argument);

  REWIND_BLOCKS_N(events, blk_5r, blk_5, miner, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  argument = argument_assert{this, get_block_height(blk_5r), std::list<uint64_t>(CURRENCY_MINED_MONEY_UNLOCK_WINDOW), argument};
  assert_reward(argument);
  assert_balance(argument);

  return true;
}

bool block_reward_in_main_chain_basic::assert_balance(currency::core& core, size_t event_index, const std::vector<test_event_entry>& events) const
{
  argument_assert argument{};

  {
    const auto serialized_argument{boost::get<callback_entry>(events.at(event_index)).callback_params};

    CHECK_AND_ASSERT_EQ(t_unserializable_object_from_blob(argument, serialized_argument), true);
  }

  CHECK_AND_ASSERT_EQ(core.get_blockchain_storage().get_top_block_height(), argument.m_height);

  const auto wallet{init_playtime_test_wallet(events, core, m_accounts.front())};

  CHECK_AND_ASSERT(wallet, false);
  wallet->refresh();
  CHECK_AND_ASSERT_EQ(wallet->balance(), argument.m_balance);

  return true;
}

bool block_reward_in_main_chain_basic::assert_reward(currency::core& core, size_t event_index, const std::vector<test_event_entry>& events) const
{
  argument_assert argument{};

  {
    const auto serialized_argument{boost::get<callback_entry>(events.at(event_index)).callback_params};

    CHECK_AND_ASSERT_EQ(t_unserializable_object_from_blob(argument, serialized_argument), true);
  }

  for (const auto expected_reward : argument.m_rewards)
  {
    uint64_t reward{};

    CHECK_AND_ASSERT_EQ(core.get_blockchain_storage().get_block_reward_by_main_chain_height(argument.m_height, reward), true);
    CHECK_AND_ASSERT_EQ(reward, expected_reward);
    --argument.m_height;
  }

  return true;
}

struct block_reward_in_alt_chain_basic::argument_assert
{
  uint64_t m_height{}, m_balance{};
  crypto::hash blk_id{};
  std::list<uint64_t> m_rewards{};

  argument_assert() = default;

  template<typename test>
  argument_assert(const test* instance, const block& block, const std::list<uint64_t>& blk_tx_fees = {}, const argument_assert previous = {}) : blk_id{get_block_hash(block)}
  {
    CHECK_AND_ASSERT_THROW(instance, std::runtime_error{"Pointer to an instance of the test equals to the nullptr."});

    auto height{get_block_height(block)};

    m_height = height;

    if (height == 0)
    {
      m_rewards.push_back(PREMINE_AMOUNT);
      m_balance = m_rewards.back();
      return;
    }

    m_balance = previous.m_balance;

    for (auto fee_iterator{blk_tx_fees.rbegin()}; fee_iterator != blk_tx_fees.rend(); ++fee_iterator)
    {
      if (height != 0)
      {
        m_rewards.push_back(COIN);

        if (const auto& fee{*fee_iterator}; fee <= m_rewards.back())
        {
          if (instance->m_hardforks.is_hardfork_active_for_height(ZANO_HARDFORK_04_ZARCANUM, height))
          {
            m_rewards.back() -= fee;
          }
        }

        m_balance += m_rewards.back();
      }

      else
      {
        m_balance += PREMINE_AMOUNT;
      }

      --height;
    }

    CHECK_AND_ASSERT_THROW(m_rewards.size() > 0, std::runtime_error{"A list of the rewards is empty."});
  }

  BEGIN_SERIALIZE()
    FIELD(m_height)
    FIELD(m_balance)
    FIELD(m_rewards)
    FIELD(blk_id)
  END_SERIALIZE()
};

block_reward_in_alt_chain_basic::block_reward_in_alt_chain_basic()
{
  REGISTER_CALLBACK_METHOD(block_reward_in_alt_chain_basic, assert_balance);
  REGISTER_CALLBACK_METHOD(block_reward_in_alt_chain_basic, assert_reward);
}

bool block_reward_in_alt_chain_basic::generate(std::vector<test_event_entry>& events) const
{
  // The test idea: make sure that receiving rewards for block insertion is counted correctly.

  const auto assert_balance{[&events](const argument_assert& argument) -> void
    {
      DO_CALLBACK_PARAMS_STR(events, "assert_balance", t_serializable_object_to_blob(argument));
    }
  };

  const auto assert_reward{[&events](const argument_assert& argument) -> void
    {
      DO_CALLBACK_PARAMS_STR(events, "assert_reward", t_serializable_object_to_blob(argument));
    }
  };

  GENERATE_ACCOUNT(miner);
  MAKE_GENESIS_BLOCK(events, blk_0, miner, test_core_time::get_time());
  argument_assert argument{this, blk_0}, argument_alt{};

  /* 0
     (blk_0) */

  m_accounts.push_back(miner);
  assert_reward(argument);
  assert_balance(argument);
  DO_CALLBACK(events, "configure_core");

  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  /* 0               10
     (blk_0) - ... - (blk_0r) */

  argument_alt = argument = {this, blk_0r, std::list<uint64_t>(CURRENCY_MINED_MONEY_UNLOCK_WINDOW), argument};
  assert_reward(argument);
  assert_balance(argument);

  MAKE_TX_FEE(events, tx_0, miner, miner, MK_TEST_COINS(1), TESTS_DEFAULT_FEE, blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner, tx_0);

  /* 0               10         11
     (blk_0) - ... - (blk_0r) - (blk_1)
                                 {tx_0} */

  MAKE_TX_FEE(events, tx_1, miner, miner, MK_TEST_COINS(1), 33 * TESTS_DEFAULT_FEE, blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1a, blk_0r, miner, tx_1);

  /* 0               10         11
     (blk_0) - ... - (blk_0r) - (blk_1a)
                            |    {tx_1}
                            |
                            \   11
                              - (blk_1)
                                 {tx_0}

  height(blk_1a) = height(blk_1)
  fee(tx_1) > fee(tx_0) */

  CHECK_AND_ASSERT_EQ(get_block_height(blk_1), get_block_height(blk_1a));

  argument_alt = {this, blk_1a, {33 * TESTS_DEFAULT_FEE}, argument_alt};
  argument = {this, blk_1, {TESTS_DEFAULT_FEE}, argument};

  if (m_hardforks.is_hardfork_active_for_height(ZANO_HARDFORK_04_ZARCANUM, get_block_height(blk_1)))
  {
    assert_reward(argument_alt);
    assert_balance(argument_alt);
  }

  else
  {
    assert_reward(argument);
    assert_balance(argument);
  }

  MAKE_TX_FEE(events, tx_2, miner, miner, MK_TEST_COINS(1), 8 * TESTS_DEFAULT_FEE, blk_1);
  MAKE_TX_FEE(events, tx_3, miner, miner, MK_TEST_COINS(1), 57 * TESTS_DEFAULT_FEE, blk_1);
  const std::list txs_0{tx_2, tx_3};

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1, miner, txs_0);

  /* 0               10         11        12
     (blk_0) - ... - (blk_0r) - (blk_1) - (blk_2)
                            |    {tx_0}    {tx_2, tx_3}
                            |
                            \   11
                              - (blk_1a)
                                 {tx_1}

  height(blk_2) > height(blk_1a). */

  argument = {this, blk_2, {(8 + 57) * TESTS_DEFAULT_FEE}, argument};
  assert_reward(argument);
  assert_balance(argument);

  const auto& head_blk_for_txs_on_height_12{m_hardforks.is_hardfork_active_for_height(ZANO_HARDFORK_04_ZARCANUM, get_block_height(blk_2)) ? blk_1a : blk_0r};
  MAKE_TX_FEE(events, tx_4, miner, miner, MK_TEST_COINS(2), 15 * TESTS_DEFAULT_FEE, head_blk_for_txs_on_height_12);
  MAKE_TX_FEE(events, tx_5, miner, miner, MK_TEST_COINS(5), 29 * TESTS_DEFAULT_FEE, head_blk_for_txs_on_height_12);
  MAKE_TX_FEE(events, tx_6, miner, miner, MK_TEST_COINS(7), 22 * TESTS_DEFAULT_FEE, head_blk_for_txs_on_height_12);
  const std::list txs_1{tx_4, tx_5, tx_6};
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2a, blk_1a, miner, txs_1);

  CHECK_AND_ASSERT_EQ(get_block_height(blk_2a), get_block_height(blk_2));

  /* 0               10         11         12
     (blk_0) - ... - (blk_0r) - (blk_1a) - (blk_2a)
                            |    {tx_1}     {tx_4, tx_5, tx_6}
                            |
                            \   11        12
                              - (blk_1) - (blk_2)
                                 {tx_0}    {tx_2, tx_3}

  height(blk_2a) = height(blk_2) = 12
  fee(tx_2) + fee(tx_3) = (8 + 57) * TESTS_DEFAULT_FEE = 65 * TESTS_DEFAULT_FEE
  fee(tx_4) + fee(tx_5) + fee(tx_6) = (15 + 29 + 22) * TESTS_DEFAULT_FEE = 66 * TESTS_DEFAULT_FEE
  66 > 65 */

  if (m_hardforks.is_hardfork_active_for_height(ZANO_HARDFORK_04_ZARCANUM, get_block_height(blk_2)))
  {
    argument_alt = {this, blk_2a, {(15 + 29 + 22) * TESTS_DEFAULT_FEE}, argument_alt};
    assert_reward(argument_alt);
    assert_balance(argument_alt);
  }

  else
  {
    assert_reward(argument);
    assert_balance(argument);
  }

  return true;
}

bool block_reward_in_alt_chain_basic::assert_balance(currency::core& core, size_t event_index, const std::vector<test_event_entry>& events) const
{
  argument_assert argument{};

  {
    const auto serialized_argument{boost::get<callback_entry>(events.at(event_index)).callback_params};

    CHECK_AND_ASSERT_EQ(t_unserializable_object_from_blob(argument, serialized_argument), true);
  }

  CHECK_AND_ASSERT_EQ(core.get_blockchain_storage().get_top_block_height(), argument.m_height);
  CHECK_AND_ASSERT_EQ(core.get_blockchain_storage().get_top_block_id(), argument.blk_id);

  const auto wallet{init_playtime_test_wallet(events, core, m_accounts.front())};

  CHECK_AND_ASSERT(wallet, false);
  wallet->refresh();
  CHECK_AND_ASSERT_EQ(wallet->balance(), argument.m_balance);

  return true;
}

bool block_reward_in_alt_chain_basic::assert_reward(currency::core& core, size_t event_index, const std::vector<test_event_entry>& events) const
{
  argument_assert argument{};

  {
    const auto serialized_argument{boost::get<callback_entry>(events.at(event_index)).callback_params};

    CHECK_AND_ASSERT_EQ(t_unserializable_object_from_blob(argument, serialized_argument), true);
  }

  {
    auto blk_id{argument.blk_id};

    for (const auto expected_reward : argument.m_rewards)
    {
      uint64_t reward{};
      block blk{};

      CHECK_AND_ASSERT_EQ(core.get_blockchain_storage().get_block_reward_by_hash(blk_id, reward), true);
      CHECK_AND_ASSERT_EQ(reward, expected_reward);
      CHECK_AND_ASSERT_EQ(core.get_blockchain_storage().get_block_by_hash(blk_id, blk), true);
      blk_id = blk.prev_id;
    }
  }

  return true;
}
