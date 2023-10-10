// Copyright (c) 2014-2023 Zano Project
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