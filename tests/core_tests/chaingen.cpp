// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <vector>
#include <iostream>
#include <sstream>

#include "include_base_utils.h"

#include "console_handler.h"

#include "p2p/net_node.h"
#include "currency_core/currency_basic.h"
#include "currency_core/currency_format_utils.h"
#include "currency_core/miner.h"
#include "currency_core/bc_offers_service.h"
#include "wallet/wallet2.h"
#include "chaingen.h"
#include "wallet_test_core_proxy.h"
#include "pos_block_builder.h"
 
//using namespace std;

using namespace epee;
using namespace currency;

#define POW_DIFF_UP_TIMESTAMP_DELTA (DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN*2/3)
#define POS_DIFF_UP_TIMESTAMP_DELTA (DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN*2/3)

std::atomic<int64_t> test_core_time::m_time_shift;
test_gentime_settings test_generator::m_test_gentime_settings_default = test_gentime_settings(tests_digits_split_strategy, CURRENCY_MINER_TX_MAX_OUTS, WALLET_MAX_ALLOWED_OUTPUT_AMOUNT, DEFAULT_DUST_THRESHOLD);
test_gentime_settings test_generator::m_test_gentime_settings = test_generator::m_test_gentime_settings_default;

crypto::signature create_invalid_signature()
{
  crypto::signature res = AUTO_VAL_INIT(res);
  uint8_t* p = reinterpret_cast<uint8_t*>(&res);
  p[sizeof res / 2 ] = 170;
  p[sizeof res / 4 ] = 255;
  return res;
}

const crypto::signature invalid_signature = create_invalid_signature();

test_generator::test_generator()
  : m_wallet_test_core_proxy(new wallet_test_core_proxy()), 
  m_do_pos_to_low_timestamp(false),
  m_ignore_last_pow_in_wallets(false),
  m_last_found_timestamp(0), 
  m_hardfork_after_heigh(CURRENCY_MAX_BLOCK_NUMBER)
{
}

void test_generator::set_hardfork_height(uint64_t h)
{
  m_hardfork_after_heigh = h;
}

void test_generator::get_block_chain(std::vector<const block_info*>& blockchain, const crypto::hash& head, size_t n) const
{
  crypto::hash curr = head;
  while (null_hash != curr && blockchain.size() < n)
  {
    auto it = m_blocks_info.find(curr);
    CHECK_AND_ASSERT_THROW_MES(it != m_blocks_info.end(), "can't find block in m_blocks_info by hash " << curr);

    blockchain.push_back(&it->second);
    curr = it->second.b.prev_id;
  }

  std::reverse(blockchain.begin(), blockchain.end());
}

void test_generator::get_last_n_block_sizes(std::vector<size_t>& block_sizes, const crypto::hash& head, size_t n) const
{
  std::vector<const block_info*> blockchain;
  get_block_chain(blockchain, head, n);
  BOOST_FOREACH(auto& bi, blockchain)
  {
    block_sizes.push_back(bi->block_size);
  }
}

uint64_t get_last_block_of_type(bool looking_for_pos, const test_generator::blockchain_vector& blck_chain)
{
  uint64_t sz = blck_chain.size();
  if (!sz)
    return 0;
  for (uint64_t i = sz - 1; i != 0; --i)
  {
    bool is_pos_bl = is_pos_block(blck_chain[i]->b);
    if ((looking_for_pos && !is_pos_bl) || (!looking_for_pos && is_pos_bl))
      continue;
    return i;
  }
  return 0;
}


uint64_t test_generator::get_already_generated_coins(const crypto::hash& blk_id) const
{
  auto it = m_blocks_info.find(blk_id);
  if (it == m_blocks_info.end())
    throw std::runtime_error("block hash wasn't found");

  return it->second.already_generated_coins;
}

currency::wide_difficulty_type test_generator::get_block_difficulty(const crypto::hash& blk_id) const
{
  auto it = m_blocks_info.find(blk_id);
  if (it == m_blocks_info.end())
    throw std::runtime_error("[get_block_difficulty] block hash wasn't found");
  
  auto it_prev = m_blocks_info.find(it->second.b.prev_id);
  if (it_prev == m_blocks_info.end())
    throw std::runtime_error("[get_block_difficulty] block hash wasn't found");

  return it->second.cumul_difficulty - it_prev->second.cumul_difficulty;
}

currency::wide_difficulty_type test_generator::get_cumul_difficulty(const crypto::hash& head_id) const
{
  auto it = m_blocks_info.find(head_id);
  if (it == m_blocks_info.end())
    throw std::runtime_error("[get_cumul_difficulty] block hash wasn't found");

  return it->second.cumul_difficulty;
}

uint64_t test_generator::get_already_generated_coins(const currency::block& blk) const
{
  crypto::hash blk_hash;
  get_block_hash(blk, blk_hash);
  return get_already_generated_coins(blk_hash);
}

void test_generator::add_block(const currency::block& blk, 
                               size_t tsx_size, 
                               std::vector<size_t>& block_sizes, 
                               uint64_t already_generated_coins, 
                               wide_difficulty_type cum_diff, 
                               const std::list<currency::transaction>& tx_list,
                               const crypto::hash& ks_hash)
{
  const size_t block_size = tsx_size + get_object_blobsize(blk.miner_tx);
  uint64_t block_reward;
  get_block_reward(is_pos_block(blk), misc_utils::median(block_sizes), block_size, already_generated_coins, block_reward, currency::get_block_height(blk));

  m_blocks_info[get_block_hash(blk)] = block_info(blk, already_generated_coins + block_reward, block_size, cum_diff, tx_list, ks_hash);
  LOG_PRINT_MAGENTA("ADDED_BLOCK[" << get_block_hash(blk) << "][" << (is_pos_block(blk)? "PoS":"PoW") <<"][" << get_block_height(blk) << "][cumul_diff:" << cum_diff << "]", LOG_LEVEL_0);
}

void test_generator::add_block_info(const block_info& bi)
{
  m_blocks_info[get_block_hash(bi.b)] = bi;
}

bool test_generator::add_block_info(const currency::block& b, const std::list<currency::transaction>& tx_list)
{
  size_t txs_total_size = 0;
  for (auto& tx : tx_list)
    txs_total_size += get_object_blobsize(tx);
  uint64_t mined_money = get_reward_from_miner_tx(b.miner_tx);
  crypto::hash sk_hash = null_hash;
  if (is_pos_block(b))
  {
    stake_kernel sk = AUTO_VAL_INIT(sk);
    std::vector<const block_info*> chain;
    get_block_chain(chain, b.prev_id, SIZE_MAX);
    uint64_t pos_idx = get_last_block_of_type(true, chain);
    if (pos_idx != 0)
      sk.stake_modifier.last_pos_kernel_id = chain[pos_idx]->ks_hash;
    else
    {
      CHECK_AND_ASSERT_MES(string_tools::parse_tpod_from_hex_string(POS_STARTER_KERNEL_HASH, sk.stake_modifier.last_pos_kernel_id), false, "Failed to parse POS_STARTER_MODFIFIER");
    }
    uint64_t pow_idx = get_last_block_of_type(false, chain);
    sk.stake_modifier.last_pow_id = get_block_hash(chain[pow_idx]->b);
    sk.kimage = boost::get<txin_to_key>(b.miner_tx.vin[1]).k_image;
    sk.block_timestamp = b.timestamp;
    sk_hash = crypto::cn_fast_hash(&sk, sizeof(sk));
  }
  add_block_info(block_info(b, get_already_generated_coins(b.prev_id) + mined_money,
    txs_total_size + get_object_blobsize(b.miner_tx), get_cumul_difficulty_for_next_block(b.prev_id), tx_list, sk_hash));
  return true;
}



bool test_generator::construct_block(currency::block& blk, 
                                     uint64_t height, 
                                     const crypto::hash& prev_id,
                                     const currency::account_base& miner_acc, 
                                     uint64_t timestamp, 
                                     uint64_t already_generated_coins,
                                     std::vector<size_t>& block_sizes, 
                                     const std::list<currency::transaction>& tx_list, 
                                     const std::list<currency::account_base>& coin_stake_sources)//in case of PoS block
{
  if (height > m_hardfork_after_heigh)
    blk.major_version = CURRENT_BLOCK_MAJOR_VERSION;
  else
    blk.major_version = BLOCK_MAJOR_VERSION_INITAL;
  
  blk.minor_version = CURRENT_BLOCK_MINOR_VERSION;
  blk.timestamp = timestamp;
  blk.prev_id = prev_id;

  crypto::hash kernerl_hash = null_hash;
  blk.tx_hashes.reserve(tx_list.size());
  BOOST_FOREACH(const transaction &tx, tx_list)
  {
    crypto::hash tx_hash;
    get_transaction_hash(tx, tx_hash);
    blk.tx_hashes.push_back(tx_hash);
  }

  uint64_t total_fee = 0;
  size_t txs_size = 0;
  BOOST_FOREACH(auto& tx, tx_list)
  {
    uint64_t fee = 0;
    bool r = get_tx_fee(tx, fee);
    CHECK_AND_ASSERT_MES(r, false, "wrong transaction passed to construct_block");
    total_fee += fee;
    txs_size += get_object_blobsize(tx);
  }

  // build block chain
  blockchain_vector blocks;
  outputs_index oi;
  tx_global_indexes txs_outs;
  get_block_chain(blocks, blk.prev_id, std::numeric_limits<size_t>::max());

  //pos
  wallets_vector wallets;

  size_t won_walled_index = 0;
  pos_entry pe = AUTO_VAL_INIT(pe);
  if (coin_stake_sources.size())
  {
    //build outputs index
    build_outputs_indext_for_chain(blocks, oi, txs_outs);

    //build wallets
    build_wallets(blocks, coin_stake_sources, txs_outs, wallets);
    bool r = find_kernel(coin_stake_sources, 
                         blocks, 
                         oi, 
                         wallets, 
                         pe, 
                         won_walled_index, 
                         blk.timestamp, 
                         kernerl_hash);
    CHECK_AND_ASSERT_THROW_MES(r, "failed to find_kernel ");
    blk.flags = CURRENCY_BLOCK_FLAG_POS_BLOCK;
  }

  blk.miner_tx = AUTO_VAL_INIT(blk.miner_tx);
  size_t target_block_size = txs_size + 0; // zero means no cost for ordinary coinbase
  while (true)
  {
    if (!construct_miner_tx(height, misc_utils::median(block_sizes), 
                                    already_generated_coins,
                                    target_block_size, 
                                    total_fee, 
                                    miner_acc.get_keys().m_account_address, 
                                    miner_acc.get_keys().m_account_address,
                                    blk.miner_tx, 
                                    blobdata(),
                                    test_generator::get_test_gentime_settings().miner_tx_max_outs,
                                    static_cast<bool>(coin_stake_sources.size()), 
                                    pe))
      return false;

    size_t coinbase_size = get_object_blobsize(blk.miner_tx);
    if (coinbase_size <= CURRENCY_COINBASE_BLOB_RESERVED_SIZE) // if less than that constant then coinbase goes for free
      break;

    size_t actual_block_size = txs_size + coinbase_size;
    if (target_block_size < actual_block_size)
    {
      target_block_size = actual_block_size;
    }
    else if (actual_block_size < target_block_size)
    {
      // increase miner tx a little using extra padding
      size_t delta = target_block_size - actual_block_size;
      extra_padding &padding = get_or_add_field_to_extra<extra_padding>(blk.miner_tx.extra);
      padding.buff.resize(padding.buff.size() + delta, '$');
      actual_block_size = txs_size + get_object_blobsize(blk.miner_tx);
      if (actual_block_size == target_block_size) // in some cases (ex: padding.buff.size() == 128) it may not be true, if so--go for one more round
        break;
    }
    else
    {
      break; // actual_block_size == target_block_size just what we want
    }
  }

  wide_difficulty_type a_diffic = get_difficulty_for_next_block(blocks, !static_cast<bool>(coin_stake_sources.size()));
  CHECK_AND_ASSERT_MES(a_diffic, false, "get_difficulty_for_next_block for test blocks returned 0!");
  // Nonce search...
  blk.nonce = 0;
  if (!coin_stake_sources.size())
  {
    //pow block
    while (!find_nounce(blk, blocks, a_diffic, height))
      blk.timestamp++;
  }
  else
  {
    //need to build pos block
    bool r = sign_block(blk, pe, *wallets[won_walled_index], blocks, oi);
    CHECK_AND_ASSERT_MES(r, false, "Failed to find_kernel_and_sign()");
  }

  uint64_t last_x = get_last_block_of_type(is_pos_block(blk), blocks);

  add_block(blk, 
            txs_size, 
            block_sizes,
            already_generated_coins, 
            last_x ? blocks[last_x]->cumul_difficulty + a_diffic: a_diffic,
            tx_list, 
            kernerl_hash);

  return true;
}

bool test_generator::sign_block(currency::block& b, 
                                pos_entry& pe, 
                                tools::wallet2& w, 
                                const std::vector<const block_info*>& blocks, 
                                const outputs_index& oi)
{
  uint64_t h = 0;
  uint64_t out_i = 0;
  const transaction * pts = nullptr;
  crypto::public_key source_tx_pub_key = null_pkey;
  crypto::public_key out_key = null_pkey;

  bool r = get_output_details_by_global_index(blocks,
    oi,
    pe.amount,
    pe.index, 
    h,
    pts,
    out_i,
    source_tx_pub_key,
    out_key);
  CHECK_AND_ASSERT_THROW_MES(r, "Failed to get_output_details_by_global_index()");

  std::vector<const crypto::public_key*> keys_ptrs;
  keys_ptrs.push_back(&out_key);
  r = w.prepare_and_sign_pos_block(b, pe, source_tx_pub_key, out_i, keys_ptrs);
  CHECK_AND_ASSERT_THROW_MES(r,"Failed to prepare_and_sign_pos_block()");

  return true;
}

bool test_generator::build_wallets(const blockchain_vector& blocks, 
                                   const std::list<currency::account_base>& accs, 
                                   const tx_global_indexes& txs_outs,
                                   wallets_vector& wallets,
                                   const core_runtime_config& cc)
{
  struct stub_core_proxy: public tools::i_core_proxy
  {
    const tx_global_indexes& m_txs_outs;
    stub_core_proxy(const tx_global_indexes& txs_outs) : m_txs_outs(txs_outs)
    {}
    virtual bool call_COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES(const currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& rqt, currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& rsp)
    {
      auto it = m_txs_outs.find(rqt.txid);
      CHECK_AND_ASSERT_MES(it != m_txs_outs.end(), false, "tx " << rqt.txid << " was not found in tx global outout indexes");
      rsp.status = CORE_RPC_STATUS_OK;
      rsp.o_indexes = it->second;
      return true; 
    }
  };

  std::shared_ptr<tools::i_core_proxy> tmp_proxy(new stub_core_proxy(txs_outs));

  //build wallets
  wallets.clear();
  for (auto a : accs)
  {
    wallets.push_back(std::shared_ptr<tools::wallet2>(new tools::wallet2()));
    wallets.back()->assign_account(a);
    wallets.back()->get_account().set_createtime(0);
    wallets.back()->set_core_proxy(tmp_proxy);

    currency::core_runtime_config pc = cc;
    pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE;
    pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH;
    pc.hard_fork1_starts_after_height = m_hardfork_after_heigh;
    wallets.back()->set_core_runtime_config(pc);
  }

  for (auto& w : wallets)
  {
    uint64_t height = 0;
    for (auto& b : blocks)
    {
      uint64_t h = get_block_height(b->b);
      if (!h)
        continue;
      CHECK_AND_ASSERT_MES(height + 1 == h, false, "Failed to return");
      height = h;
      //skip genesis
      currency::block_direct_data_entry bdde = AUTO_VAL_INIT(bdde);
      std::shared_ptr<block_extended_info> bptr(new block_extended_info());
      bptr->bl = b->b;
      bdde.block_ptr = bptr;
      for (auto& tx : b->m_transactions)
      {
        std::shared_ptr<transaction_chain_entry> tx_ptr(new transaction_chain_entry());
        tx_ptr->tx = tx;
        bdde.txs_ptr.push_back(tx_ptr);
      }

      w->process_new_blockchain_entry(b->b, bdde, currency::get_block_hash(b->b), height);
    }
  }
  return true;
}

bool test_generator::build_wallets(const crypto::hash& blockchain_head, const std::list<currency::account_base>& accounts, wallets_vector& wallets, const core_runtime_config& cc)
{
  blockchain_vector blocks;
  outputs_index oi;
  tx_global_indexes txs_outs;

  get_block_chain(blocks, blockchain_head, std::numeric_limits<size_t>::max());
  build_outputs_indext_for_chain(blocks, oi, txs_outs);
  return build_wallets(blocks, accounts, txs_outs, wallets, cc);
}

size_t test_generator::get_tx_out_gindex(const crypto::hash& blockchain_head, const crypto::hash& tx_hash, const size_t output_index) const
{
  blockchain_vector blocks;
  outputs_index oi;
  tx_global_indexes txs_outs;

  get_block_chain(blocks, blockchain_head, std::numeric_limits<size_t>::max());
  build_outputs_indext_for_chain(blocks, oi, txs_outs);
  const auto& it = txs_outs.find(tx_hash);
  if (it == txs_outs.end() || output_index >= it->second.size())
    return std::numeric_limits<size_t>::max();

  return it->second[output_index];
}


uint64_t test_generator::get_timestamps_median(const blockchain_vector& blck_chain, size_t window_size /* = BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW */)
{
  std::vector<uint64_t> timestamps;
  for(size_t i = blck_chain.size() - std::min(blck_chain.size(), window_size); i < blck_chain.size(); ++i)
    timestamps.push_back(blck_chain[i]->b.timestamp);

  return epee::misc_utils::median(timestamps);
}

uint64_t test_generator::get_timestamps_median(const crypto::hash& blockchain_head, size_t window_size /* = BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW */)
{
  blockchain_vector blocks;

  get_block_chain(blocks, blockchain_head, std::numeric_limits<size_t>::max());
  return get_timestamps_median(blocks, window_size);
}

bool test_generator::find_kernel(const std::list<currency::account_base>& accs,
  const blockchain_vector& blck_chain,
  const outputs_index& indexes,
  wallets_vector& wallets,
  currency::pos_entry& pe,
  size_t& found_wallet_index, 
  uint64_t& found_timestamp,
  crypto::hash& found_kh)
{
  bool is_after_hardfork = blck_chain.size() > m_hardfork_after_heigh ? true : false;
  uint64_t median_timestamp = get_timestamps_median(blck_chain);
  wide_difficulty_type basic_diff = 0;

  uint64_t i_last_pos_block = get_last_block_of_type(true, blck_chain);
  
  uint64_t last_pos_block_timestamp = 0;
  if(i_last_pos_block)
    last_pos_block_timestamp = blck_chain[i_last_pos_block]->b.timestamp;
  else
    last_pos_block_timestamp = blck_chain.back()->b.timestamp - DIFFICULTY_POS_TARGET/2;
  
  uint64_t starter_timestamp = last_pos_block_timestamp + DIFFICULTY_POS_TARGET;

  if (starter_timestamp < median_timestamp)
    starter_timestamp = median_timestamp;

  m_last_found_timestamp = 0;
  basic_diff = get_difficulty_for_next_block(blck_chain, false);
  if (basic_diff < 10)
  {
    starter_timestamp -= 90;
  }
  if (m_do_pos_to_low_timestamp)
    starter_timestamp += 60;

  //adjust timestamp starting from timestamp%POS_SCAN_STEP = 0
  //starter_timestamp = starter_timestamp - POS_SCAN_WINDOW;
  starter_timestamp = POS_SCAN_STEP - (starter_timestamp%POS_SCAN_STEP) + starter_timestamp;

  for (uint64_t ts = starter_timestamp; ts < starter_timestamp + POS_SCAN_WINDOW/2; ts += POS_SCAN_STEP)
  {
    //lets try to find block
    for (auto& w : wallets)
    {
      //set m_last_pow_block_h to big value, to let wallet to use any available outputs, including the those which is not behind last pow block
      if (m_ignore_last_pow_in_wallets)
        w->m_last_pow_block_h = CURRENCY_MAX_BLOCK_NUMBER;

      currency::COMMAND_RPC_SCAN_POS::request scan_pos_entries;
      bool r = w->get_pos_entries(scan_pos_entries);
      CHECK_AND_ASSERT_THROW_MES(r, "Failed to get_pos_entries");

      for (size_t i = 0; i != scan_pos_entries.pos_entries.size(); i++)
      {

        stake_kernel sk = AUTO_VAL_INIT(sk);
        uint64_t coindays_weight = 0;
        build_kernel(scan_pos_entries.pos_entries[i].amount,
          scan_pos_entries.pos_entries[i].index,
          scan_pos_entries.pos_entries[i].keyimage,
          sk,
          coindays_weight,
          blck_chain,
          indexes, 
          ts);
        crypto::hash kernel_hash = crypto::cn_fast_hash(&sk, sizeof(sk));
        wide_difficulty_type this_coin_diff = basic_diff / coindays_weight;
        if (!check_hash(kernel_hash, this_coin_diff))
          continue;
        else
        {
          if (m_do_pos_to_low_timestamp)
          {
            if (!m_last_found_timestamp)
            {
              m_last_found_timestamp = ts;
              continue;
            }

            if(m_last_found_timestamp >= ts)
              continue;
          }

          //found kernel
          LOG_PRINT_GREEN("Found kernel: amount=" << print_money(scan_pos_entries.pos_entries[i].amount)
            << ", index=" << scan_pos_entries.pos_entries[i].index
            << ", key_image" << scan_pos_entries.pos_entries[i].keyimage
            << ", diff: " << this_coin_diff, LOG_LEVEL_0);
          pe = scan_pos_entries.pos_entries[i];
          found_wallet_index = i;
          found_kh = kernel_hash;
          found_timestamp = ts;
          return true;
        }
      }
    }
    
  }
  return false;
}
//------------------------------------------------------------------
bool test_generator::build_outputs_indext_for_chain(const blockchain_vector& blocks, outputs_index& index, tx_global_indexes& txs_outs) const
{
  for (size_t h = 0; h != blocks.size(); h++)
  {
    std::vector<uint64_t>& coinbase_outs = txs_outs[currency::get_transaction_hash(blocks[h]->b.miner_tx)];
    for (size_t out_i = 0; out_i != blocks[h]->b.miner_tx.vout.size(); out_i++)
    {
      coinbase_outs.push_back(index[blocks[h]->b.miner_tx.vout[out_i].amount].size());
      index[blocks[h]->b.miner_tx.vout[out_i].amount].push_back(std::tuple<size_t, size_t, size_t>(h, 0, out_i));
    }

    for (size_t tx_index = 0; tx_index != blocks[h]->m_transactions.size(); tx_index++)
    {
      std::vector<uint64_t>& tx_outs_indx = txs_outs[currency::get_transaction_hash(blocks[h]->m_transactions[tx_index])];
      for (size_t out_i = 0; out_i != blocks[h]->m_transactions[tx_index].vout.size(); out_i++)
      {
        tx_outs_indx.push_back(index[blocks[h]->m_transactions[tx_index].vout[out_i].amount].size());
        index[blocks[h]->m_transactions[tx_index].vout[out_i].amount].push_back(std::tuple<size_t, size_t, size_t>(h, tx_index + 1, out_i));
      }
    }
  }
  return true;
}
//------------------------------------------------------------------
bool test_generator::get_output_details_by_global_index(const test_generator::blockchain_vector& blck_chain,
                                                        const test_generator::outputs_index& indexes,
                                                        uint64_t amount,
                                                        uint64_t global_index,
                                                        uint64_t& h,
                                                        const transaction* tx,
                                                        uint64_t& tx_out_index,
                                                        crypto::public_key& tx_pub_key,
                                                        crypto::public_key& output_key)
{
  auto it = indexes.find(amount);
  CHECK_AND_ASSERT_MES(it != indexes.end(), false, "Failed to find amount in coin stake " << amount);

  CHECK_AND_ASSERT_MES(it->second.size() > global_index, false, "wrong key offset " << global_index  << " with amount kernel_in.amount");

  h = std::get<0>(it->second[global_index]);
  size_t tx_index = std::get<1>(it->second[global_index]);
  tx_out_index = std::get<2>(it->second[global_index]);

  CHECK_AND_ASSERT_THROW_MES(h < blck_chain.size(), "std::get<0>(it->second[global_index]) < blck_chain.size()");
  CHECK_AND_ASSERT_THROW_MES(tx_index  < blck_chain[h]->m_transactions.size() + 1, "tx_index < blck_chain[h].m_transactions.size()");
  tx = tx_index ? &blck_chain[h]->m_transactions[tx_index - 1] : &blck_chain[h]->b.miner_tx;

  CHECK_AND_ASSERT_THROW_MES(tx_out_index < tx->vout.size(), "tx_index < blck_chain[h].m_transactions.size()");

  tx_pub_key = get_tx_pub_key_from_extra(*tx);
  CHECK_AND_ASSERT_THROW_MES(tx->vout[tx_out_index].target.type() == typeid(currency::txout_to_key),
    "blck_chain[h]->m_transactions[tx_index].vout[tx_out_index].target.type() == typeid(currency::txout_to_key)");

  CHECK_AND_ASSERT_THROW_MES(tx->vout[tx_out_index].amount == amount,
    "blck_chain[h]->m_transactions[tx_index].vout[tx_out_index].amount == amount");

  output_key = boost::get<currency::txout_to_key>(tx->vout[tx_out_index].target).key;
  return true;
}
//------------------------------------------------------------------
bool test_generator::build_kernel(uint64_t amount, 
                                  uint64_t global_index, 
                                  const crypto::key_image& ki, 
                                  stake_kernel& kernel, 
                                  uint64_t& coindays_weight,
                                  const test_generator::blockchain_vector& blck_chain,
                                  const test_generator::outputs_index& indexes, 
                                  uint64_t timestamp)
{
  coindays_weight = 0;
  kernel = stake_kernel();
  kernel.kimage = ki;

  //get block related with coinstake source transaction
  uint64_t h = 0; 
  uint64_t out_i = 0;
  const transaction * pts = nullptr;
  crypto::public_key source_tx_pub_key = null_pkey;
  crypto::public_key out_key = null_pkey;

  bool r = get_output_details_by_global_index(blck_chain, 
                                     indexes, 
                                     amount, 
                                     global_index, 
                                     h, 
                                     pts, 
                                     out_i, 
                                     source_tx_pub_key, 
                                     out_key);
  CHECK_AND_ASSERT_THROW_MES(r,"Failed to get_output_details_by_global_index()");

  kernel.block_timestamp = timestamp;

  coindays_weight = get_coinday_weight(amount);
  build_stake_modifier(kernel.stake_modifier, blck_chain);
  return true;
}


bool test_generator::build_stake_modifier(stake_modifier_type& sm, const test_generator::blockchain_vector& blck_chain)
{
  sm = stake_modifier_type();

  uint64_t last_pos_i = get_last_block_of_type(true, blck_chain);
  uint64_t last_pow_i = get_last_block_of_type(false, blck_chain);

  if (last_pos_i)
    sm.last_pos_kernel_id = blck_chain[last_pos_i]->ks_hash;
  else
  {
    bool r = string_tools::parse_tpod_from_hex_string(POS_STARTER_KERNEL_HASH, sm.last_pos_kernel_id);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse POS_STARTER_MODFIFIER");
  }

  sm.last_pow_id = get_block_hash(blck_chain[last_pow_i]->b);


  return true;
}

bool test_generator::call_COMMAND_RPC_SCAN_POS(const currency::COMMAND_RPC_SCAN_POS::request& req, currency::COMMAND_RPC_SCAN_POS::response& rsp)
{
  return false;
}

currency::wide_difficulty_type test_generator::get_difficulty_for_next_block(const crypto::hash& head_id, bool pow) const
{
  std::vector<const block_info*> blocks;
  get_block_chain(blocks, head_id, std::numeric_limits<size_t>::max());

  return get_difficulty_for_next_block(blocks, pow);
}

currency::wide_difficulty_type test_generator::get_difficulty_for_next_block(const std::vector<const block_info*>& blocks, bool pow) const
{
  std::vector<uint64_t> timestamps;
  std::vector<wide_difficulty_type> commulative_difficulties;
  if (!blocks.size())
    return DIFFICULTY_STARTER;

  for (size_t i = blocks.size() - 1; i != 0; --i)
  {
    if ((pow && is_pos_block(blocks[i]->b)) || (!pow && !is_pos_block(blocks[i]->b)))
      continue;
    timestamps.push_back(blocks[i]->b.timestamp);
    commulative_difficulties.push_back(blocks[i]->cumul_difficulty);
  }
  return next_difficulty_1(timestamps, commulative_difficulties, pow ? DIFFICULTY_POW_TARGET : DIFFICULTY_POS_TARGET);
}

currency::wide_difficulty_type test_generator::get_cumul_difficulty_for_next_block(const crypto::hash& head_id, bool pow) const
{
  return get_cumul_difficulty(head_id) + get_difficulty_for_next_block(head_id, pow);
}

uint64_t test_generator::get_base_reward_for_next_block(const crypto::hash& head_id, bool pow /* = true */) const
{
  auto it = m_blocks_info.find(head_id);
  if (it == m_blocks_info.end())
    return 0;
  return get_base_block_reward(!pow, it->second.already_generated_coins, get_block_height(it->second.b));
}

bool test_generator::find_nounce(currency::block& blk, std::vector<const block_info*>& blocks, wide_difficulty_type dif, uint64_t height) const
{
  if(height != blocks.size())
    throw std::runtime_error("wrong height in find_nounce");
  
  return miner::find_nonce_for_given_block(blk, dif, height);
}

bool test_generator::construct_genesis_block(currency::block& blk, const currency::account_base& miner_acc, uint64_t timestamp)
{
  std::vector<size_t> block_sizes;
  std::list<currency::transaction> tx_list;
  return construct_block(blk, 0, null_hash, miner_acc, timestamp, 0, block_sizes, tx_list);
}

size_t get_prev_block_of_type(const std::vector<currency::block>& blockchain, bool pos)
{
  if (!blockchain.size())
    return 0;

  for (size_t i = blockchain.size() - 1; i != 0; i--)
  {
    if (is_pos_block(blockchain[i]) == pos)
    {
      return i;
    }
  }
  return 0;
}

bool have_n_blocks_of_type(const std::vector<currency::block>& blockchain, bool pos)
{
  size_t count = 0;
  size_t stop_count = 10;
  if (pos)
    stop_count = 20;

  for (auto it = blockchain.rbegin(); it != blockchain.rend(); it++)
  {
    if (is_pos_block(*it) == pos)
    {
      ++count;
      if (count >= stop_count)
        return true;
    }
  }
  return false;
}

bool test_generator::construct_block(const std::vector<test_event_entry>& events,
                                     currency::block& blk,
                                     const currency::block& blk_prev,
                                     const currency::account_base& miner_acc,
                                     const std::list<currency::transaction>& tx_list, 
                                     const std::list<currency::account_base>& coin_stake_sources)
{
  return construct_block(0, events, blk, blk_prev, miner_acc, tx_list, coin_stake_sources);
}
bool test_generator::construct_block(int64_t manual_timestamp_adjustment, 
                                     const std::vector<test_event_entry>& events,
                                     currency::block& blk,
                                     const currency::block& blk_prev,
                                     const currency::account_base& miner_acc,
                                     const std::list<currency::transaction>& tx_list, 
                                     const std::list<currency::account_base>& coin_stake_sources)
{
  uint64_t height = boost::get<txin_gen>(blk_prev.miner_tx.vin[0]).height + 1;
  crypto::hash prev_id = get_block_hash(blk_prev);
  // Keep push difficulty little up to be sure about PoW hash success
  std::vector<currency::block> blockchain;
  map_hash2tx_t mtx;
  bool r = find_block_chain(events, blockchain, mtx, get_block_hash(blk_prev));
  CHECK_AND_ASSERT_MES(r, false, "can't find a blockchain up from given blk_prev");
  bool pos_bl = coin_stake_sources.size();
  bool adjust_timestamp_finished = have_n_blocks_of_type(blockchain, pos_bl);
  uint64_t diff_up_timestamp_delta = POW_DIFF_UP_TIMESTAMP_DELTA;
  if (pos_bl)
  { 
    //in debug purpose
    diff_up_timestamp_delta = POS_DIFF_UP_TIMESTAMP_DELTA;
  }

  uint64_t diff_target = pos_bl ? DIFFICULTY_POS_TARGET : DIFFICULTY_POW_TARGET;

  uint64_t timestamp = 0;
  size_t prev_i = get_prev_block_of_type(blockchain, pos_bl);
  block& prev_same_type = blockchain[prev_i];

  timestamp = adjust_timestamp_finished ? prev_same_type.timestamp + diff_target : prev_same_type.timestamp + diff_target - diff_up_timestamp_delta;
  timestamp = timestamp + manual_timestamp_adjustment;

  uint64_t already_generated_coins = get_already_generated_coins(prev_id);
  std::vector<size_t> block_sizes;
  get_last_n_block_sizes(block_sizes, prev_id, CURRENCY_REWARD_BLOCKS_WINDOW);

  return construct_block(blk, height, prev_id, miner_acc, timestamp, already_generated_coins, block_sizes, tx_list, coin_stake_sources);
}

 bool test_generator::construct_block_manually(currency::block& blk, const block& prev_block, const account_base& miner_acc,
                                               int actual_params/* = bf_none*/, uint8_t major_ver/* = 0*/,
                                               uint8_t minor_ver/* = 0*/, uint64_t timestamp/* = 0*/,
                                              const crypto::hash& prev_id/* = crypto::hash()*/, const wide_difficulty_type& diffic/* = 1*/,
                                              const transaction& miner_tx/* = transaction()*/,
                                              const std::vector<crypto::hash>& tx_hashes/* = std::vector<crypto::hash>()*/,
                                              size_t txs_sizes/* = 0*/)
{
  size_t height = get_block_height(prev_block) + 1;
  blk.major_version = actual_params & bf_major_ver ? major_ver : BLOCK_MAJOR_VERSION_INITAL;
  blk.minor_version = actual_params & bf_minor_ver ? minor_ver : CURRENT_BLOCK_MINOR_VERSION;
  blk.timestamp     = actual_params & bf_timestamp ? timestamp : (height > 10 ? prev_block.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN: prev_block.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN-POW_DIFF_UP_TIMESTAMP_DELTA); // Keep difficulty unchanged
  blk.prev_id       = actual_params & bf_prev_id   ? prev_id   : get_block_hash(prev_block);
  blk.tx_hashes     = actual_params & bf_tx_hashes ? tx_hashes : std::vector<crypto::hash>();

  
  uint64_t already_generated_coins = get_already_generated_coins(prev_block);
  std::vector<size_t> block_sizes;
  get_last_n_block_sizes(block_sizes, get_block_hash(prev_block), CURRENCY_REWARD_BLOCKS_WINDOW);
  if (actual_params & bf_miner_tx)
  {
    blk.miner_tx = miner_tx;
  }
  else
  {
    size_t current_block_size = txs_sizes + get_object_blobsize(blk.miner_tx);
    // TODO: This will work, until size of constructed block is less then CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE
    if (!construct_miner_tx(height, misc_utils::median(block_sizes), already_generated_coins, current_block_size, 0, miner_acc.get_public_address(), miner_acc.get_public_address(), blk.miner_tx, blobdata(), 1))
      return false;
  }

  //blk.tree_root_hash = get_tx_tree_hash(blk);
  std::vector<const block_info*> blocks;
  get_block_chain(blocks, blk.prev_id, std::numeric_limits<size_t>::max());

  wide_difficulty_type a_diffic = actual_params & bf_diffic ? diffic : get_difficulty_for_next_block(blocks);
  find_nounce(blk, blocks, a_diffic, height);

  std::list<transaction> txs; // fake list here
  add_block(blk, txs_sizes, block_sizes, already_generated_coins, blocks.size() ? blocks.back()->cumul_difficulty + a_diffic : a_diffic, txs, null_hash);

  return true;
}

bool test_generator::construct_block_manually_tx(currency::block& blk, const currency::block& prev_block,
                                                 const currency::account_base& miner_acc,
                                                 const std::vector<crypto::hash>& tx_hashes, size_t txs_size)
{
  return construct_block_manually(blk, prev_block, miner_acc, bf_tx_hashes, 0, 0, 0, crypto::hash(), 0, transaction(), tx_hashes, txs_size);
}

bool test_generator::init_test_wallet(const currency::account_base& account, const crypto::hash& genesis_hash, std::shared_ptr<tools::wallet2> &result)
{
  core_runtime_config crc = get_default_core_runtime_config();
  crc.get_core_time = test_core_time::get_time;
  crc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH;
  crc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE;

  std::shared_ptr<tools::wallet2> w(new tools::wallet2);
  w->set_core_runtime_config(crc);
  w->assign_account(account);
  w->set_genesis(genesis_hash);
  w->set_core_proxy(m_wallet_test_core_proxy);

  result = w;
  return true;
}

bool test_generator::refresh_test_wallet(const std::vector<test_event_entry>& events, tools::wallet2* w, const crypto::hash& top_block_hash, size_t expected_blocks_to_be_fetched)
{
  m_wallet_test_core_proxy->update_blockchain(events, *this, top_block_hash);

  size_t blocks_fetched = 0;
  bool received_money = false;
  bool ok = false;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  bool r = w->refresh(blocks_fetched, received_money, ok, atomic_false);
  CHECK_AND_ASSERT_MES(r, false, "test wallet refersh failed");
  CHECK_AND_ASSERT_MES(expected_blocks_to_be_fetched == std::numeric_limits<size_t>::max() || expected_blocks_to_be_fetched == blocks_fetched, false, "test wallet refresh fetched " << blocks_fetched << ", expected: " << expected_blocks_to_be_fetched);

  bool has_aliases;
  w->scan_tx_pool(has_aliases);
  return true;
}

uint64_t test_generator::get_last_n_blocks_fee_median(const crypto::hash& head_block_hash, size_t n)
{
  std::vector<const block_info*> blockchain;
  get_block_chain(blockchain, head_block_hash, n);

  std::vector<uint64_t> tx_fee_list;
  for (auto& bi : blockchain)
  {
    for (auto& tx : bi->m_transactions)
      tx_fee_list.push_back(get_tx_fee(tx));
  }

  if (tx_fee_list.empty())
    return TESTS_DEFAULT_FEE;

  return epee::misc_utils::median(tx_fee_list);
}

bool test_generator::construct_pow_block_with_alias_info_in_coinbase(const account_base& acc, const block& prev_block, const extra_alias_entry& ai, block& b)
{
  return construct_block_gentime_with_coinbase_cb(prev_block, acc, [&acc, &ai](transaction& miner_tx){
    miner_tx.extra.push_back(ai);
    if (ai.m_sign.empty())
    {
      // if no alias update - reduce block reward by alias cost
      uint64_t alias_cost = get_alias_coast_from_fee(ai.m_alias, TESTS_DEFAULT_FEE);
      uint64_t block_reward = get_outs_money_amount(miner_tx);
      CHECK_AND_ASSERT_MES(alias_cost <= block_reward, false, "Alias '" << ai.m_alias << "' can't be registered via block coinbase, because it's price: " << print_money(alias_cost) << " is greater than block reward: " << print_money(block_reward));
      block_reward -= alias_cost;
      miner_tx.vout.clear();
      if (block_reward > 0)
      {
        bool null_out_key = false;
        auto f = [&acc, &miner_tx, &null_out_key](uint64_t amount){
          keypair kp = AUTO_VAL_INIT(kp);
          derive_ephemeral_key_helper(acc.get_keys(), get_tx_pub_key_from_extra(miner_tx), miner_tx.vout.size(), kp);
          txout_to_key otk = AUTO_VAL_INIT(otk);
          otk.key = null_out_key ? null_pkey : kp.pub;
          miner_tx.vout.push_back(tx_out({ amount, otk }));
        };
        decompose_amount_into_digits(block_reward, DEFAULT_DUST_THRESHOLD, f, f);
        null_out_key = true;
        f(alias_cost); // add an output for burning alias cost into ashes
      }
    }
    return true;
  }, b);
}

//------------------------------------------------------------------------------

struct output_index {
    const currency::txout_target_v out;
    uint64_t amount;
    size_t blk_height; // block height
    size_t tx_no; // index of transaction in block
    size_t out_no; // index of out in transaction
    size_t idx;
    bool spent;
    const currency::block *p_blk;
    const currency::transaction *p_tx;

    output_index(const currency::txout_target_v &_out, uint64_t _a, size_t _h, size_t tno, size_t ono, const currency::block *_pb, const currency::transaction *_pt)
        : out(_out), amount(_a), blk_height(_h), tx_no(tno), out_no(ono), idx(0), spent(false), p_blk(_pb), p_tx(_pt) { }

    output_index(const output_index &other)
        : out(other.out), amount(other.amount), blk_height(other.blk_height), tx_no(other.tx_no), out_no(other.out_no), idx(other.idx), spent(other.spent), p_blk(other.p_blk), p_tx(other.p_tx) {  }

    const std::string toString() const {
        std::stringstream ss;

        ss << "output_index{blk_height=" << blk_height
           << " tx_no=" << tx_no
           << " out_no=" << out_no
           << " amount=" << amount
           << " idx=" << idx
           << " spent=" << spent
           << "}";

        return ss.str();
    }

    output_index& operator=(const output_index& other)
    {
      new(this) output_index(other);
      return *this;
    }
};

typedef std::map<uint64_t, std::vector<size_t> > map_output_t;           // amount -> [N -> global out index]
typedef std::map<uint64_t, std::vector<output_index> > map_output_idx_t; // amount -> [global out index -> 'output_index']
typedef std::pair<uint64_t, size_t>  outloc_t;

namespace
{
  uint64_t get_inputs_amount(const std::vector<tx_source_entry> &s)
  {
    uint64_t r = 0;
    BOOST_FOREACH(const tx_source_entry &e, s)
    {
      r += e.amount;
    }

    return r;
  }
}

bool init_output_indices(map_output_idx_t& outs, map_output_t& outs_mine, const std::vector<currency::block>& blockchain, const map_hash2tx_t& mtx, const currency::account_keys& acc_keys)
{
    for(const block& blk : blockchain)
    {
        std::vector<const transaction*> vtx;
        vtx.push_back(&blk.miner_tx);

        for(const crypto::hash &h : blk.tx_hashes)
        {
            const map_hash2tx_t::const_iterator cit = mtx.find(h);
            CHECK_AND_ASSERT_MES(cit != mtx.end(), false, "block at height " << get_block_height(blk) << " contains a reference to unknown tx " << h);
            vtx.push_back(cit->second);
        }

        for (size_t i = 0; i < vtx.size(); i++)
        {
            const transaction &tx = *vtx[i];
            crypto::key_derivation derivation;
            bool r = generate_key_derivation(get_tx_pub_key_from_extra(tx), acc_keys.m_view_secret_key, derivation);
            CHECK_AND_ASSERT_MES(r, false, "generate_key_derivation failed");

            for (size_t j = 0; j < tx.vout.size(); ++j)
            {
                const tx_out &out = tx.vout[j];
                output_index oi(out.target, out.amount, boost::get<txin_gen>(*blk.miner_tx.vin.begin()).height, i, j, &blk, vtx[i]);

                if (out.target.type() == typeid(txout_to_key))
                {
                    outs[out.amount].push_back(oi);
                    size_t tx_global_idx = outs[out.amount].size() - 1;
                    outs[out.amount][tx_global_idx].idx = tx_global_idx;
                    // Is out to me?
                    if (is_out_to_acc(acc_keys, boost::get<txout_to_key>(out.target), derivation, j))
                        outs_mine[out.amount].push_back(tx_global_idx);
                }
            }
        }
    }

    return true;
}

bool init_spent_output_indices(map_output_idx_t& outs, map_output_t& outs_mine, const std::vector<currency::block>& blockchain, const map_hash2tx_t& mtx, const currency::account_keys& from)
{
    for(const map_output_t::value_type &o : outs_mine)
    {
        for (size_t i = 0; i < o.second.size(); ++i)
        {
            output_index &oi = outs[o.first][o.second[i]];

            // construct key image for this output
            crypto::key_image out_ki;
            keypair in_ephemeral;
            generate_key_image_helper(from, get_tx_pub_key_from_extra(*oi.p_tx), oi.out_no, in_ephemeral, out_ki);

            // lookup for this key image in the events std::vector
            for(auto& tx_pair : mtx)
            {
                const transaction& tx = *tx_pair.second;
                for(const txin_v &in : tx.vin)
                {
                    if (typeid(txin_to_key) == in.type())
                    {
                        const txin_to_key &itk = boost::get<txin_to_key>(in);
                        if (itk.k_image == out_ki)
                            oi.spent = true;
                    }
                }
            }

            // check whether this key image has been spent in miner tx of a PoS block
            // TODO change this check to simply adding PoS miner tx to mtx map
            for (auto& b : blockchain)
            {
              if (!is_pos_block(b))
                continue;
              for (const txin_v &in : b.miner_tx.vin)
              {
                if (in.type() == typeid(txin_to_key))
                {
                    const txin_to_key &itk = boost::get<txin_to_key>(in);
                    if (itk.k_image == out_ki)
                        oi.spent = true;
                }
              }
            }
        }
    }

    return true;
}

bool fill_output_entries(std::vector<output_index>& out_indices,
                         size_t sender_out, size_t nmix, uint64_t& real_entry_idx,
                         std::vector<tx_source_entry::output_entry>& output_entries,
                         bool use_ref_by_id)
{
  if (out_indices.size() <= nmix)
    return false;

  bool sender_out_found = false;
  size_t rest = nmix;
  for (size_t i = 0; i < out_indices.size() && (0 < rest || !sender_out_found); ++i)
  {
    const output_index& oi = out_indices[i];
    if (oi.spent)
      continue;

    bool append = false;
    if (i == sender_out)
    {
      append = true;
      sender_out_found = true;
      real_entry_idx = output_entries.size();
    }
    else if (0 < rest)
    {
      if(boost::get<txout_to_key>(oi.out).mix_attr == CURRENCY_TO_KEY_OUT_FORCED_NO_MIX || boost::get<txout_to_key>(oi.out).mix_attr > nmix+1)
        continue;

      --rest;
      append = true;
    }

    if (append)
    {
      tx_source_entry::output_entry oe = AUTO_VAL_INIT(oe);
      const txout_to_key& otk = boost::get<txout_to_key>(oi.out);
      if (use_ref_by_id)                                              // <-- HINT: this could be replaced by 'true' to enforce using ref_by_id across all the tests if needed
      {
        ref_by_id rbi = AUTO_VAL_INIT(rbi);
        rbi.n = oi.out_no;
        rbi.tx_id = get_transaction_hash(*oi.p_tx);
        oe = tx_source_entry::output_entry(rbi, otk.key);
      }
      else
      {
        oe = tx_source_entry::output_entry(oi.idx, otk.key);
      }
      output_entries.push_back(oe);
    }
  }

  return 0 == rest && sender_out_found;
}



bool fill_tx_sources(std::vector<tx_source_entry>& sources, const std::vector<test_event_entry>& events,
                     const block& blk_head, const currency::account_keys& from, uint64_t amount, size_t nmix, bool check_for_spends, bool check_for_unlocktime, bool use_ref_by_id)
{
  return fill_tx_sources(sources, events, blk_head, from, amount, nmix, std::vector<tx_source_entry>(), check_for_spends, check_for_unlocktime, use_ref_by_id);
}

bool fill_tx_sources(std::vector<currency::tx_source_entry>& sources, const std::vector<test_event_entry>& events,
                     const currency::block& blk_head, const currency::account_keys& from, uint64_t amount, size_t nmix, const std::vector<currency::tx_source_entry>& sources_to_avoid,
                     bool check_for_spends, bool check_for_unlocktime, bool use_ref_by_id, uint64_t* p_sources_amount_found /* = nullptr */)
{
    map_output_idx_t outs;
    map_output_t outs_mine;

    std::vector<currency::block> blockchain;
    map_hash2tx_t mtx;
    if (!find_block_chain(events, blockchain, mtx, get_block_hash(blk_head)))
        return false;

    if (!init_output_indices(outs, outs_mine, blockchain, mtx, from))
        return false;

    if(check_for_spends)
    {
      if (!init_spent_output_indices(outs, outs_mine, blockchain, mtx, from))
        return false;
    }

    // mark some outputs as spent to avoid their using
    for (const auto& s : sources_to_avoid)
    {
      for (const auto& s_outputs_el : s.outputs) // avoid all outputs, including fake mix-ins
      {
        txout_v sout = s_outputs_el.first;
        if (sout.type().hash_code() == typeid(uint64_t).hash_code())       // output by global index
        {
          uint64_t gindex = boost::get<uint64_t>(sout);
          auto& outs_by_amount = outs[s.amount];
          if (gindex >= outs_by_amount.size())
            return false;
          outs_by_amount[gindex].spent = true;
        }
        else if (sout.type().hash_code() == typeid(ref_by_id).hash_code()) // output by ref_by_id
        {
          ref_by_id out_ref_by_id = boost::get<ref_by_id>(sout);
          const auto it = mtx.find(out_ref_by_id.tx_id);
          if (it == mtx.end())
            return false;
          const transaction* p_tx = it->second;
          for (auto& e : outs[s.amount]) // linear search by transaction among all outputs with such amount
          {
            if (e.p_tx == p_tx)
            {
              e.spent = true;
              p_tx = nullptr; // means 'found'
              break;
            }
          }
          if (p_tx != nullptr)
            return false; // output, referring by ref_by_id was not found
        }
        else
        {
          return false; // unknown output type
        }
      }
    }

    // Iterate in reverse is more efficiency
    uint64_t sources_amount = 0;
    bool sources_found = false;
    BOOST_REVERSE_FOREACH(const map_output_t::value_type o, outs_mine)
    {
        for (size_t i = 0; i < o.second.size() && !sources_found; ++i)
        {
            size_t sender_out = o.second[i];
            const output_index& oi = outs[o.first][sender_out];
            if (oi.spent)
                continue;
            if (check_for_unlocktime)
            {
              if (currency::get_tx_max_unlock_time(*oi.p_tx) < CURRENCY_MAX_BLOCK_NUMBER)
              {
                //interpret as block index
                if (currency::get_tx_max_unlock_time(*oi.p_tx) > blockchain.size())
                  continue;
              }
              else
              {
                //interpret as time
              }
            }


            currency::tx_source_entry ts = AUTO_VAL_INIT(ts);
            ts.amount = oi.amount;
            ts.real_output_in_tx_index = oi.out_no;
            ts.real_out_tx_key = get_tx_pub_key_from_extra(*oi.p_tx); // incoming tx public key
            if (!fill_output_entries(outs[o.first], sender_out, nmix, ts.real_output, ts.outputs, use_ref_by_id))
              continue;

            sources.push_back(ts);

            sources_amount += ts.amount;
            sources_found = amount <= sources_amount;
        }

        if (sources_found)
            break;
    }

    if (p_sources_amount_found != nullptr)
      *p_sources_amount_found = sources_amount;

    return sources_found;
}

bool fill_tx_sources_and_destinations(const std::vector<test_event_entry>& events, const block& blk_head,
  const currency::account_keys& from, const std::list<currency::account_public_address>& to,
  uint64_t amount, uint64_t fee, size_t nmix, std::vector<tx_source_entry>& sources,
  std::vector<tx_destination_entry>& destinations,
  bool check_for_spends,
  bool check_for_unlocktime,
  size_t minimum_sigs,
  bool use_ref_by_id)
{
  CHECK_AND_ASSERT_MES(!to.empty(), false, "destination addresses vector is empty");
  CHECK_AND_ASSERT_MES(amount + fee > amount, false, "amount + fee overflow!");
  sources.clear();
  destinations.clear();
  bool b_multisig = to.size() > 1;

  uint64_t source_amount_found = 0;
  bool r = fill_tx_sources(sources, events, blk_head, from, amount + fee, nmix, std::vector<currency::tx_source_entry>(), check_for_spends, check_for_unlocktime, use_ref_by_id, &source_amount_found);
  CHECK_AND_ASSERT_MES(r, false, "couldn't fill transaction sources: " << ENDL <<
    "  required:      " << print_money(amount + fee) << " = " << std::fixed << std::setprecision(1) << ceil(1.0 * (amount + fee) / TESTS_DEFAULT_FEE) << " x TESTS_DEFAULT_FEE" << ENDL <<
    "  unspent coins: " << print_money(source_amount_found) << " = " << std::fixed << std::setprecision(1) << ceil(1.0 * source_amount_found / TESTS_DEFAULT_FEE) << " x TESTS_DEFAULT_FEE" << ENDL <<
    "  lack of coins: " << print_money(amount + fee - source_amount_found) << " = " << std::fixed << std::setprecision(1) << ceil(1.0 * (amount + fee - source_amount_found) / TESTS_DEFAULT_FEE) << " x TESTS_DEFAULT_FEE" 
  );

  uint64_t inputs_amount = get_inputs_amount(sources);
  CHECK_AND_ASSERT_MES(inputs_amount >= amount + fee, false, "Pre-condition fail: inputs_amount is less than amount + fee");
  uint64_t cache_back = inputs_amount - (amount + fee);

  if (b_multisig)
  {
    destinations.push_back(tx_destination_entry(amount, to));
    if (minimum_sigs != SIZE_MAX)
      destinations.back().minimum_sigs = minimum_sigs; // set custom minimum_sigs only if != SIZE_MAX, use default in tx_destination_entry::ctor() otherwise
    if (cache_back > 0)
      destinations.push_back(tx_destination_entry(cache_back, from.m_account_address));
  }
  else
  {
    tx_destination_entry change_dst = AUTO_VAL_INIT(change_dst);
    if (cache_back > 0)
      change_dst = tx_destination_entry(cache_back, from.m_account_address);
    std::vector<tx_destination_entry> dsts(1, tx_destination_entry(amount, to.back()));
    uint64_t dust = 0;
    const test_gentime_settings& tgs = test_generator::get_test_gentime_settings();
    switch (tgs.split_strategy)
    {
    case tests_null_split_strategy:
      tools::detail::null_split_strategy(dsts, change_dst, tgs.dust_threshold, destinations, dust, tgs.tx_max_out_amount);
      break;
    case tests_void_split_strategy:
      tools::detail::void_split_strategy(dsts, change_dst, tgs.dust_threshold, destinations, dust, tgs.tx_max_out_amount);
      break;
    case tests_digits_split_strategy:
      tools::detail::digit_split_strategy(dsts, change_dst, tgs.dust_threshold, destinations, dust, tgs.tx_max_out_amount);
      break;
    default:
      CHECK_AND_ASSERT_MES(false, false, "Invalid split strategy set in gentime settings");
    }
    CHECK_AND_ASSERT_MES(destinations.size() > 0, false, "split strategy failed");
    CHECK_AND_ASSERT_MES(dust <= tgs.dust_threshold, false, "split strategy failed and returned incorrect dust");
  }

  return true;
}

bool fill_tx_sources_and_destinations(const std::vector<test_event_entry>& events, const block& blk_head,
                                      const currency::account_keys& from, const currency::account_public_address& to,
                                      uint64_t amount, uint64_t fee, size_t nmix, std::vector<tx_source_entry>& sources,
                                      std::vector<tx_destination_entry>& destinations,
                                      bool check_for_spends,
                                      bool check_for_unlocktime,
                                      bool use_ref_by_id)
{
  return fill_tx_sources_and_destinations(events, blk_head, from, std::list<account_public_address>({ to }), amount, fee, nmix, sources, destinations, check_for_spends, check_for_unlocktime, 0, use_ref_by_id);
}

bool fill_tx_sources_and_destinations(const std::vector<test_event_entry>& events, const currency::block& blk_head,
                                      const currency::account_base& from, const currency::account_base& to,
                                      uint64_t amount, uint64_t fee, size_t nmix,
                                      std::vector<currency::tx_source_entry>& sources,
                                      std::vector<currency::tx_destination_entry>& destinations,
                                      bool check_for_spends,
                                      bool check_for_unlocktime,
                                      bool use_ref_by_id)
{
  return fill_tx_sources_and_destinations(events, blk_head, from.get_keys(), to.get_public_address(), amount, fee, nmix, sources, destinations, check_for_spends, check_for_unlocktime, use_ref_by_id);
}

/*
void fill_nonce(currency::block& blk, const wide_difficulty_type& diffic, uint64_t height)
{
  blk.nonce = 0;
  while (!miner::find_nonce_for_given_block(blk, diffic, height))
    blk.timestamp++;
}*/

bool construct_miner_tx_manually(size_t height, uint64_t already_generated_coins,
                                 const account_public_address& miner_address, transaction& tx, uint64_t fee,
                                 keypair* p_txkey/* = 0*/)
{
  keypair txkey;
  txkey = keypair::generate();
  add_tx_pub_key_to_extra(tx, txkey.pub);

  if (0 != p_txkey)
    *p_txkey = txkey;

  txin_gen in;
  in.height = height;
  tx.vin.push_back(in);

  // This will work, until size of constructed block is less then CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE
  uint64_t block_reward;
  if (!get_block_reward(false, 0, 0, already_generated_coins, block_reward, height))
  {
    LOG_PRINT_L0("Block is too big");
    return false;
  }
  block_reward += fee;

  crypto::key_derivation derivation;
  crypto::public_key out_eph_public_key;
  crypto::generate_key_derivation(miner_address.m_view_public_key, txkey.sec, derivation);
  crypto::derive_public_key(derivation, 0, miner_address.m_spend_public_key, out_eph_public_key);

  tx_out out;
  out.amount = block_reward;
  out.target = txout_to_key(out_eph_public_key);
  tx.vout.push_back(out);

  tx.version = CURRENT_TRANSACTION_VERSION;
  currency::set_tx_unlock_time(tx, height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  return true;
}


bool construct_tx_to_key(const std::vector<test_event_entry>& events, 
                         currency::transaction& tx, 
                         const block& blk_head,
                         const currency::account_base& from, 
                         const currency::account_base& to, 
                         uint64_t amount,
                         uint64_t fee, 
                         size_t nmix, 
                         uint8_t mix_attr, 
                         const std::vector<currency::extra_v>& extr,
                         const std::vector<currency::attachment_v>& att, 
                         bool check_for_spends,
                         bool check_for_unlocktime)
{
  crypto::secret_key sk = AUTO_VAL_INIT(sk);
  return construct_tx_to_key(events,
                      tx, 
                         blk_head,
                         from, 
                         to, 
                         amount,
                         fee, 
                         nmix,
                         sk,
                         mix_attr, 
                         extr,
                         att, 
                         check_for_spends,
                         check_for_unlocktime);
}

bool construct_tx_to_key(const std::vector<test_event_entry>& events, 
                         currency::transaction& tx, 
                         const block& blk_head,
                         const currency::account_base& from, 
                         const currency::account_base& to, 
                         uint64_t amount,
                         uint64_t fee, 
                         size_t nmix, 
                         crypto::secret_key& sk,
                         uint8_t mix_attr, 
                         const std::vector<currency::extra_v>& extr,
                         const std::vector<currency::attachment_v>& att, 
                         bool check_for_spends,
                         bool check_for_unlocktime)
{
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  if (!fill_tx_sources_and_destinations(events, blk_head, from.get_keys(), to.get_public_address(), amount, fee, nmix, sources, destinations, check_for_spends, check_for_unlocktime))
    return false;
  return construct_tx(from.get_keys(), sources, destinations, extr, att, tx, sk, 0, mix_attr);
}

bool construct_tx_to_key(const std::vector<test_event_entry>& events, 
                         currency::transaction& tx, 
                         const currency::block& blk_head,
                         const currency::account_base& from,
                         const std::vector<currency::tx_destination_entry>& destinations,
                         uint64_t fee /* = TX_POOL_MINIMUM_FEE */, 
                         size_t nmix /* = 0 */, 
                         uint8_t mix_attr /* = CURRENCY_TO_KEY_OUT_RELAXED */, 
                         const std::vector<currency::extra_v>& extr /* = empty_extra */,
                         const std::vector<currency::attachment_v>& att /* = empty_attachment */, 
                         bool check_for_spends /* = true */,
                         bool check_for_unlocktime /* = true */,
                         bool use_ref_by_id /* = false */)
{
  crypto::secret_key sk; // stub
  uint64_t spending_amount = fee;
  for(auto& el: destinations)
    spending_amount += el.amount;

  std::vector<tx_source_entry> sources;
  if (!fill_tx_sources(sources, events, blk_head, from.get_keys(), spending_amount, nmix, check_for_spends, check_for_unlocktime, use_ref_by_id))
    return false;

  boost::multiprecision::int128_t change = get_sources_total_amount(sources);
  change -= spending_amount;
  if (change < 0)
    return false; // should never happen if fill_tx_sources succeded
  if (change == 0)
    return construct_tx(from.get_keys(), sources, destinations, extr, att, tx, sk, 0, mix_attr);
  std::vector<tx_destination_entry> local_dst = destinations;
  local_dst.push_back(tx_destination_entry(change.convert_to<uint64_t>(), from.get_public_address()));
  return construct_tx(from.get_keys(), sources, local_dst, extr, att, tx, sk, 0, mix_attr);
}


transaction construct_tx_with_fee(std::vector<test_event_entry>& events, const block& blk_head,
                                  const account_base& acc_from, const account_base& acc_to, uint64_t amount, uint64_t fee)
{
  transaction tx;
  construct_tx_to_key(events, tx, blk_head, acc_from, acc_to, amount, fee, 0);
  events.push_back(tx);
  return tx;
}

bool construct_tx_with_many_outputs(std::vector<test_event_entry>& events, const currency::block& blk_head,
  const currency::account_keys& keys_from, const currency::account_public_address& addr_to,
  uint64_t total_amount, size_t outputs_count, uint64_t fee, currency::transaction& tx)
{
  std::vector<currency::tx_source_entry> sources;
  bool r = fill_tx_sources(sources, events, blk_head, keys_from, total_amount + fee, 0, true, false, false);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");

  std::vector<currency::tx_destination_entry> destinations;
  uint64_t amount = total_amount / outputs_count;
  for(size_t i = 0; i < outputs_count; ++i)
    destinations.push_back(tx_destination_entry(amount, addr_to));

  uint64_t sources_amount = get_sources_total_amount(sources);
  if (sources_amount > total_amount + fee)
    destinations.push_back(tx_destination_entry(sources_amount - (total_amount + fee), keys_from.m_account_address)); // change

  return construct_tx(keys_from, sources, destinations, empty_attachment, tx, 0);
}

uint64_t get_balance(const currency::account_keys& addr, const std::vector<currency::block>& blockchain, const map_hash2tx_t& mtx, bool dbg_log) {
    uint64_t res = 0;
    std::map<uint64_t, std::vector<output_index> > outs;
    std::map<uint64_t, std::vector<size_t> > outs_mine;

    map_hash2tx_t confirmed_txs;
    get_confirmed_txs(blockchain, mtx, confirmed_txs);

    if (!init_output_indices(outs, outs_mine, blockchain, confirmed_txs, addr))
        return false;

    if (!init_spent_output_indices(outs, outs_mine, blockchain, confirmed_txs, addr))
        return false;

    BOOST_FOREACH (const map_output_t::value_type &o, outs_mine) {
        for (size_t i = 0; i < o.second.size(); ++i) {
            if (outs[o.first][o.second[i]].spent)
                continue;

            output_index& oiv = outs[o.first][o.second[i]];
            res += oiv.amount;
            if (dbg_log)
              LOG_PRINT_L0(oiv.toString());
        }
    }

    return res;
}

uint64_t get_balance(const currency::account_base& addr, const std::vector<currency::block>& blockchain, const map_hash2tx_t& mtx, bool dbg_log)
{
  return get_balance(addr.get_keys(), blockchain, mtx, dbg_log);
}

void get_confirmed_txs(const std::vector<currency::block>& blockchain, const map_hash2tx_t& mtx, map_hash2tx_t& confirmed_txs)
{
  std::unordered_set<crypto::hash> confirmed_hashes;
  BOOST_FOREACH(const block& blk, blockchain)
  {
    BOOST_FOREACH(const crypto::hash& tx_hash, blk.tx_hashes)
    {
      confirmed_hashes.insert(tx_hash);
    }
  }

  BOOST_FOREACH(const auto& tx_pair, mtx)
  {
    if (0 != confirmed_hashes.count(tx_pair.first))
    {
      confirmed_txs.insert(tx_pair);
    }
  }
}

bool find_block_chain(const std::vector<test_event_entry>& events, std::vector<currency::block>& blockchain, map_hash2tx_t& mtx, const crypto::hash& head) {
  std::unordered_map<crypto::hash, const block*> block_index;
  BOOST_FOREACH(const test_event_entry& ev, events)
  {
    if (typeid(currency::block) == ev.type())
    {
      const block* blk = &boost::get<block>(ev);
      block_index[get_block_hash(*blk)] = blk;
    }
    else if (typeid(event_special_block) == ev.type())
    {
      const event_special_block& esb = boost::get<event_special_block>(ev);
      crypto::hash block_hash = get_block_hash(esb.block);
      CHECK_AND_ASSERT_MES(esb.special_flags == 0 || (esb.special_flags & event_special_block::flag_skip) != 0, false, "Unsupported special flag for block " << block_hash);
      block_index[block_hash] = &esb.block;
    }
    else if (typeid(transaction) == ev.type())
    {
      const transaction& tx = boost::get<transaction>(ev);
      mtx[get_transaction_hash(tx)] = &tx;
    }
  }

  bool b_success = false;
  crypto::hash id = head;
  for (auto it = block_index.find(id); block_index.end() != it; it = block_index.find(id))
  {
    blockchain.push_back(*it->second);
    id = it->second->prev_id;
    if (null_hash == id)
    {
      b_success = true;
      break;
    }
  }
  reverse(blockchain.begin(), blockchain.end());

  return b_success;
}

void balance_via_wallet(const tools::wallet2& w, uint64_t* p_total, uint64_t* p_unlocked, uint64_t* p_awaiting_in, uint64_t* p_awaiting_out, uint64_t* p_mined)
{
  uint64_t total, unlocked, awaiting_in, awaiting_out, mined;
  total = w.balance(unlocked, awaiting_in, awaiting_out, mined);

  if (p_total)
    *p_total = total;
  if (p_unlocked)
    *p_unlocked = unlocked;
  if (p_awaiting_in)
    *p_awaiting_in = awaiting_in;
  if (p_awaiting_out)
    *p_awaiting_out = awaiting_out;
  if (p_mined)
    *p_mined = mined;
}

bool check_balance_via_wallet(const tools::wallet2& w, const char* account_name,
  uint64_t expected_total, uint64_t expected_mined, uint64_t expected_unlocked, uint64_t expected_awaiting_in, uint64_t expected_awaiting_out)
{
  uint64_t total, unlocked, awaiting_in, awaiting_out, mined;
  balance_via_wallet(w, &total, &unlocked, &awaiting_in, &awaiting_out, &mined);

  LOG_PRINT_CYAN("Balance for wallet " << account_name << ":" << ENDL <<
    "unlocked:     " << print_money(unlocked) << ENDL <<
    "awaiting in:  " << print_money(awaiting_in) << ENDL <<
    "awaiting out: " << print_money(awaiting_out) << ENDL <<
    "mined:        " << print_money(mined) << ENDL <<
    "-----------------------------------------" << ENDL <<
    "total:        " << print_money(total) << ENDL,
    LOG_LEVEL_0);

  bool r = true;

#define _CHECK_BAL(v) if (!(expected_##v == INVALID_BALANCE_VAL || v == expected_##v)) { r = false; LOG_PRINT_RED_L0("invalid " #v " balance, expected: " << print_money(expected_##v)); }
  _CHECK_BAL(unlocked)
  _CHECK_BAL(awaiting_in)
  _CHECK_BAL(awaiting_out)
  _CHECK_BAL(mined)
  _CHECK_BAL(total)
#undef _CHECK_BAL

  if (!r)
  {
    LOG_PRINT(account_name << "'s transfers: " << ENDL << w.dump_trunsfers(), LOG_LEVEL_0);
  }

  return r;
}

// In assumption we have only genesis and few blocks with the same reward (==first_blocks_reward),
// this function helps to calculate such amount that many outputs have it, and amount, no output has it.
// It can fail, so check the returning value.
bool calculate_amounts_many_outs_have_and_no_outs_have(const uint64_t first_blocks_reward, uint64_t& amount_many_outs_have, uint64_t& amount_no_outs_have)
{
  std::set<uint64_t, std::greater<uint64_t> > digits;
  decompose_amount_into_digits(first_blocks_reward, DEFAULT_DUST_THRESHOLD, [&digits](uint64_t chunk){ digits.insert(chunk); }, [&digits](uint64_t dust) { /* nope */ });
  CHECK_AND_ASSERT_MES(!digits.empty(), false, "decompose_amount_into_digits failed");
  amount_many_outs_have = *std::max_element(digits.begin(), digits.end());

  amount_no_outs_have = 0;
  for (uint64_t a : digits)
  {
    uint64_t divider = 1;
    for (; divider < 1000000000000000000; divider *= 10)
      if (a / divider < 10)
        break;
    amount_no_outs_have = (a != divider) ? (a / divider - 1) * divider : 9 * (divider / 10);
    if (amount_no_outs_have != 0 && digits.count(amount_no_outs_have) == 0)
      break;
  }
  CHECK_AND_ASSERT_MES(amount_no_outs_have != 0, false, "failed to find amount_no_outs_have");
  return true;
}

bool find_global_index_for_output(const std::vector<test_event_entry>& events, const crypto::hash& head_block_hash, const currency::transaction& reference_tx, const size_t reference_tx_out_index, uint64_t& global_index)
{
  std::vector<currency::block> blockchain;
  map_hash2tx_t mtx;
  bool r = find_block_chain(events, blockchain, mtx, head_block_hash);
  CHECK_AND_ASSERT_MES(r, false, "Can't find blockchain starting from block " << head_block_hash);

  std::map<uint64_t, uint64_t> global_outputs; // amount -> outs count
  auto process_tx = [&reference_tx, &reference_tx_out_index, &global_outputs](const currency::transaction& tx) -> uint64_t
  {
    for (size_t tx_out_index = 0; tx_out_index < tx.vout.size(); ++tx_out_index) {
      const tx_out &out = tx.vout[tx_out_index];
      if (out.target.type() == typeid(txout_to_key)) {
        uint64_t global_out_index = global_outputs[out.amount]++;

        if (tx_out_index == reference_tx_out_index && tx == reference_tx)
          return global_out_index;
      }
    }
    return UINT64_MAX;
  };


  for(const block& blk : blockchain)
  {
    global_index = process_tx(blk.miner_tx);
    if (global_index != UINT64_MAX)
      return true;
    for(const crypto::hash &h : blk.tx_hashes)
    {
      const map_hash2tx_t::const_iterator cit = mtx.find(h);
      CHECK_AND_ASSERT_MES(cit != mtx.end(), false, "internal error: got unknown tx reference in a block");
      global_index = process_tx(*cit->second);
      if (global_index != UINT64_MAX)
        return true;
    }
  }
  
  return false;
}

size_t get_tx_out_index_by_amount(const currency::transaction& tx, const uint64_t amount)
{
  for (size_t i = 0; i < tx.vout.size(); ++i)
    if (tx.vout[i].amount == amount)
      return i;

  return SIZE_MAX;
}

// test-oriented version of currency_format_utils function -- less checks for more negative tests-cases
bool sign_multisig_input_in_tx_custom(currency::transaction& tx, size_t ms_input_index, const currency::account_keys& keys, const currency::transaction& source_tx, bool *p_is_input_fully_signed /* = nullptr */, bool compact_sigs /* = true */)
{
#define LOC_CHK(cond, msg) CHECK_AND_ASSERT_MES(cond, false, msg << ", ms input index: " << ms_input_index << ", tx: " << get_transaction_hash(tx) << ", source tx: " << get_transaction_hash(source_tx))
  if (p_is_input_fully_signed != nullptr)
    *p_is_input_fully_signed = false;

  LOC_CHK(ms_input_index < tx.vin.size(), "ms input index is out of bounds, vin.size() = " << tx.vin.size());
  LOC_CHK(tx.vin[ms_input_index].type() == typeid(txin_multisig), "ms input has wrong type, txin_multisig expected");
  const txin_multisig& ms_in = boost::get<txin_multisig>(tx.vin[ms_input_index]);

  // search ms output in source tx by ms_in.multisig_out_id
  size_t ms_out_index = SIZE_MAX;
  for (size_t i = 0; i < source_tx.vout.size(); ++i)
  {
    if (source_tx.vout[i].target.type() == typeid(txout_multisig) && ms_in.multisig_out_id == get_multisig_out_id(source_tx, i))
    {
      ms_out_index = i;
      break;
    }
  }
  LOC_CHK(ms_out_index != SIZE_MAX, "failed to find ms output in source tx " << get_transaction_hash(source_tx) << " by ms id " << ms_in.multisig_out_id);
  const txout_multisig& out_ms = boost::get<txout_multisig>(source_tx.vout[ms_out_index].target);

  crypto::public_key source_tx_pub_key = get_tx_pub_key_from_extra(source_tx);

  keypair ms_in_ephemeral_key = AUTO_VAL_INIT(ms_in_ephemeral_key);
  bool r = currency::derive_ephemeral_key_helper(keys, source_tx_pub_key, ms_out_index, ms_in_ephemeral_key);
  LOC_CHK(r, "derive_ephemeral_key_helper failed");

  LOC_CHK(ms_input_index < tx.signatures.size(), "transaction does not have signatures vectory entry for ms input #" << ms_input_index);

  auto& sigs = tx.signatures[ms_input_index];
  LOC_CHK(!sigs.empty(), "empty signatures container");
  bool extra_signature_expected = (get_tx_flags(tx) & TX_FLAG_SIGNATURE_MODE_SEPARATE) && ms_input_index == tx.vin.size() - 1;
  size_t allocated_sigs_for_participants = extra_signature_expected ? sigs.size() - 1 : sigs.size();

  LOC_CHK(sigs.size() >= out_ms.keys.size(), "too little signatures: " << sigs.size() << " for ms out key size: " << out_ms.keys.size());

  // determine participant index, taking into account the fact it could be similar keys
  size_t participant_index = SIZE_MAX;
  for (size_t i = 0; i < out_ms.keys.size(); ++i)
  {
    if (out_ms.keys[i] == ms_in_ephemeral_key.pub && sigs[i] == null_sig)
    {
      participant_index = i;
      break;
    }
  }
  LOC_CHK(participant_index < out_ms.keys.size(), "Can't find given participant's ms key in ms output keys list OR corresponding signature is already present");
  LOC_CHK(participant_index < allocated_sigs_for_participants, "participant index (" << participant_index << ") is out of bound: " << allocated_sigs_for_participants); // NOTE: this may fail if the input has already been fully signed and 'sigs' was compacted
  
  crypto::hash tx_hash_for_signature = prepare_prefix_hash_for_sign(tx, ms_input_index, get_transaction_hash(tx));
  LOC_CHK(tx_hash_for_signature != null_hash, "failed to  prepare_prefix_hash_for_sign");

  crypto::generate_signature(tx_hash_for_signature, ms_in_ephemeral_key.pub, ms_in_ephemeral_key.sec, sigs[participant_index]);

  // check whether the input is fully signed
  size_t non_null_sigs_count = 0;
  for (size_t i = 0; i < allocated_sigs_for_participants; ++i)
  {
    if (sigs[i] != null_sig)
      ++non_null_sigs_count;
  }

  if (compact_sigs && non_null_sigs_count >= out_ms.minimum_sigs) // in normal case it's always '<='
  {
    // this input is fully signed, now we gonna compact sigs container by removing null signatures
    sigs.erase(std::remove(sigs.begin(), sigs.end(), null_sig), sigs.end());

    if (p_is_input_fully_signed != nullptr)
      *p_is_input_fully_signed = true;
  }

  return true;
#undef LOC_CHK
}

bool make_tx_multisig_to_key(const currency::transaction& source_tx,
  size_t source_tx_out_idx,
  const std::list<currency::account_keys>& participants,
  const currency::account_public_address& target_address,
  currency::transaction& tx,
  uint64_t fee /* = TX_POOL_MINIMUM_FEE */,
  const std::vector<currency::attachment_v>& attachments /* = empty_attachment */,
  const std::vector<currency::extra_v>& extra /* = empty_extra */)
{
  tx_source_entry se = AUTO_VAL_INIT(se);
  se.real_out_tx_key = get_tx_pub_key_from_extra(source_tx);
  CHECK_AND_ASSERT_MES(source_tx_out_idx < source_tx.vout.size(), false, "tx " << se.real_output << " has " << source_tx.vout.size() << " outputs, #" << source_tx_out_idx << " specified");
  se.real_output_in_tx_index = source_tx_out_idx;
  se.multisig_id = get_multisig_out_id(source_tx, se.real_output_in_tx_index);
  CHECK_AND_ASSERT_MES(source_tx.vout[se.real_output_in_tx_index].target.type() == typeid(txout_multisig), false, "tx " << se.real_output << " output #" << source_tx_out_idx << " is not a txout_multisig");
  const txout_multisig& ms_out = boost::get<txout_multisig>(source_tx.vout[se.real_output_in_tx_index].target);
  se.ms_keys_count = ms_out.keys.size();
  se.ms_sigs_count = ms_out.minimum_sigs;
  se.amount = source_tx.vout[se.real_output_in_tx_index].amount;

  tx_destination_entry de(se.amount - fee, target_address);

  currency::account_keys keys = AUTO_VAL_INIT(keys);
  bool r = construct_tx(keys, std::vector<tx_source_entry>({ se }), std::vector<tx_destination_entry>({ de }), empty_attachment, tx, 0, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

  bool tx_fully_signed = false;
  for(auto& key : participants)
  {
    r = sign_multisig_input_in_tx_custom(tx, 0, key, source_tx, &tx_fully_signed, true);
    CHECK_AND_ASSERT_MES(r, false, "sign_multisig_input_in_tx_custom failed");
  }
  CHECK_AND_ASSERT_MES(tx_fully_signed, false, "sign_multisig_input_in_tx_custom failed: tx_fully_signed is " << tx_fully_signed);

  return true;
}

bool estimate_wallet_balance_blocked_for_escrow(const tools::wallet2& w, uint64_t& result, bool substruct_change_from_result /* = true */, uint64_t* p_change /* = nullptr */)
{
  std::deque<tools::wallet2::transfer_details> transfers;
  w.get_transfers(transfers);

  result = 0;
  for (const tools::wallet2::transfer_details& td : transfers)
  {
    if (td.m_flags == (WALLET_TRANSFER_DETAIL_FLAG_BLOCKED | WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION))
      result += td.amount();
  }
  if (substruct_change_from_result || p_change != nullptr)
  {
    const std::list<tools::wallet2::expiration_entry_info>& ee = w.get_expiration_entries();
    for (auto &e : ee)
    {
      CHECK_AND_ASSERT_MES(result >= e.change_amount, false, "wrong transfers: result: " << print_money(result) << " is expected to be NOT LESS than change_amount: " << print_money(e.change_amount));
      if (substruct_change_from_result)
        result -= e.change_amount;
      if (p_change != nullptr)
        *p_change += e.change_amount;
    }
  }
  return true;
}

bool check_wallet_balance_blocked_for_escrow(const tools::wallet2& w, const char* wallet_name, uint64_t expected_amount)
{
  uint64_t actual_amount = 0;
  bool r = estimate_wallet_balance_blocked_for_escrow(w, actual_amount);
  CHECK_AND_ASSERT_MES(r, false, "estimate_wallet_balance_blocked_for_escrow failed");
  CHECK_AND_ASSERT_MES(actual_amount == expected_amount, false, std::string(wallet_name) << "'s wallet has incorrect money amount blocked for escrow: " << print_money(actual_amount) << ", expected: " << print_money(expected_amount));
  return true;
}

bool refresh_wallet_and_check_balance(const char* intro_log_message, const char* wallet_name, std::shared_ptr<tools::wallet2> wallet, uint64_t expected_total,
  bool print_transfers /*= false */,
  size_t block_to_be_fetched /* = SIZE_MAX */,
  uint64_t expected_unlocked /* = UINT64_MAX */,
  uint64_t expected_mined /* = UINT64_MAX */,
  uint64_t expected_awaiting_in /* = UINT64_MAX */,
  uint64_t expected_awaiting_out /* = UINT64_MAX */)
{
  bool stub_bool = false;
  size_t blocks_fetched = 0;
  LOG_PRINT_CYAN("***** " << intro_log_message << " refreshing " << wallet_name << "'s wallet...", LOG_LEVEL_0);
  wallet->refresh(blocks_fetched);
  CHECK_AND_ASSERT_MES(block_to_be_fetched == SIZE_MAX || blocks_fetched == block_to_be_fetched, false, wallet_name << ": incorrect amount of fetched blocks: " << blocks_fetched << ", expected: " << block_to_be_fetched);
  LOG_PRINT_CYAN("Scanning tx pool for " << wallet_name << "'s wallet...", LOG_LEVEL_0);
  wallet->scan_tx_pool(stub_bool);

  if (print_transfers)
  {
    LOG_PRINT_CYAN(wallet_name << "'s transfers: " << ENDL << wallet->dump_trunsfers(), LOG_LEVEL_0);
  }

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*wallet.get(), wallet_name, expected_total, expected_mined, expected_unlocked, expected_awaiting_in, expected_awaiting_out), false, "");

  return true;
}

bool generate_pos_block_with_given_coinstake(test_generator& generator, const std::vector<test_event_entry> &events, const currency::account_base& miner, const currency::block& prev_block, const currency::transaction& stake_tx, size_t stake_output_idx, currency::block& result, uint64_t stake_output_gidx /* = UINT64_MAX */)
{
  crypto::hash prev_id = get_block_hash(prev_block);
  size_t height = get_block_height(prev_block) + 1;
  currency::wide_difficulty_type diff = generator.get_difficulty_for_next_block(prev_id, false);

  try
  {
    crypto::public_key stake_tx_pub_key = get_tx_pub_key_from_extra(stake_tx);
    if (stake_output_gidx == UINT64_MAX)
    {
      bool r = find_global_index_for_output(events, prev_id, stake_tx, stake_output_idx, stake_output_gidx);
      CHECK_AND_ASSERT_MES(r, false, "find_global_index_for_output failed");
    }
    uint64_t stake_output_amount = stake_tx.vout[stake_output_idx].amount;
    crypto::key_image stake_output_key_image;
    keypair kp;
    generate_key_image_helper(miner.get_keys(), stake_tx_pub_key, stake_output_idx, kp, stake_output_key_image);
    crypto::public_key stake_output_pubkey = boost::get<txout_to_key>(stake_tx.vout[stake_output_idx].target).key;

    pos_block_builder pb;
    pb.step1_init_header(height, prev_id);
    pb.step2_set_txs(std::vector<transaction>());
    pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, diff, prev_id, null_hash, prev_block.timestamp);
    pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(prev_block), miner.get_public_address());
    pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, miner);
    result = pb.m_block;

    return true;
  }
  catch (std::exception& e)
  {
    LOG_ERROR("PoS generation at height " << height << " failed, got an exception: " << e.what());
  }

  return false;
}

bool check_ring_signature_at_gen_time(const std::vector<test_event_entry>& events, const crypto::hash& last_block_id, const txin_to_key& in_t_k,
  const crypto::hash& hash_for_sig, const std::vector<crypto::signature> &sig)
{
  std::vector<currency::block> blockchain;
  map_hash2tx_t mtx;
  bool r = find_block_chain(events, blockchain, mtx, last_block_id);
  CHECK_AND_ASSERT_MES(r, false, "can't find a blockchain for given last block id == " << last_block_id);
  for (auto& b : blockchain)
    mtx[get_transaction_hash(b.miner_tx)] = &b.miner_tx;

  std::vector<crypto::public_key> pub_keys;
  pub_keys.reserve(in_t_k.key_offsets.size());
  std::vector<const crypto::public_key*> pub_keys_ptrs;
  pub_keys_ptrs.reserve(in_t_k.key_offsets.size());
  for (auto& ko : in_t_k.key_offsets)
  {
    if (ko.type() == typeid(uint64_t))
    {
      CHECK_AND_ASSERT_MES(false, false, "not implemented");
    }
    else if (ko.type() == typeid(ref_by_id))
    {
      auto& rbi = boost::get<ref_by_id>(ko);
      LOG_PRINT_YELLOW("rbi: tx: " << rbi.tx_id << ", out n: " << rbi.n, LOG_LEVEL_0);
      auto it = mtx.find(rbi.tx_id);
      CHECK_AND_ASSERT_MES(it != mtx.end(), false, "it == end");
      CHECK_AND_ASSERT_MES(rbi.n < it->second->vout.size(), false, "FAIL: rbi.n < it->second->vout.size()");
      auto& pub_key = boost::get<txout_to_key>(it->second->vout[rbi.n].target).key;

      pub_keys.push_back(pub_key);
      pub_keys_ptrs.push_back(&pub_keys.back());
    }
  }

  r = check_ring_signature(hash_for_sig, in_t_k.k_image, pub_keys_ptrs, sig.data());
  LOG_PRINT("checking RING SIG: " << dump_ring_sig_data(hash_for_sig, in_t_k.k_image, pub_keys_ptrs, sig), LOG_LEVEL_0);
  CHECK_AND_ASSERT_MES(r, false, "check_ring_signature failed!");

  return true;
}

//------------------------------------------------------------------------------

void test_chain_unit_base::register_callback(const std::string& cb_name, verify_callback cb)
{
  m_callbacks[cb_name] = cb;
}

bool test_chain_unit_base::verify(const std::string& cb_name, currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  auto cb_it = m_callbacks.find(cb_name);
  if(cb_it == m_callbacks.end())
  {
    LOG_ERROR("Failed to find callback " << cb_name);
    return false;
  }
  return cb_it->second(c, ev_index, events);
}

//------------------------------------------------------------------------------

test_chain_unit_enchanced::test_chain_unit_enchanced()
  : m_invalid_block_index(std::numeric_limits<size_t>::max())
  , m_orphan_block_index(std::numeric_limits<size_t>::max())
  , m_invalid_tx_index(std::numeric_limits<size_t>::max())
{
  REGISTER_CALLBACK_METHOD(test_chain_unit_enchanced, configure_core);
  REGISTER_CALLBACK_METHOD(test_chain_unit_enchanced, mark_invalid_tx);
  REGISTER_CALLBACK_METHOD(test_chain_unit_enchanced, mark_invalid_block);
  REGISTER_CALLBACK_METHOD(test_chain_unit_enchanced, mark_orphan_block);
  REGISTER_CALLBACK_METHOD(test_chain_unit_enchanced, check_top_block);
  REGISTER_CALLBACK_METHOD(test_chain_unit_enchanced, clear_tx_pool);
  REGISTER_CALLBACK_METHOD(test_chain_unit_enchanced, check_tx_pool_empty);
  REGISTER_CALLBACK_METHOD(test_chain_unit_enchanced, check_tx_pool_count);
  REGISTER_CALLBACK_METHOD(test_chain_unit_enchanced, print_tx_pool);
  REGISTER_CALLBACK_METHOD(test_chain_unit_enchanced, check_offers_count);
}

bool test_chain_unit_enchanced::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE;
  pc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH;

  c.get_blockchain_storage().set_core_runtime_config(pc);
  return true;
}

bool test_chain_unit_enchanced::check_top_block(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  params_top_block ptb = AUTO_VAL_INIT(ptb);
  bool r = epee::string_tools::hex_to_pod(boost::get<callback_entry>(events[ev_index]).callback_params, ptb);
  CHECK_AND_ASSERT_MES(r, false, "test_chain_unit_enchanced: Can't obtain event params. Forgot to pass them?");

  uint64_t height;
  crypto::hash hash;
  r = c.get_blockchain_top(height, hash);
  CHECK_AND_ASSERT_MES(r, false, "get_blockchain_top failed");
  CHECK_AND_ASSERT_MES(height == ptb.height && hash == ptb.hash, false, "Top block check failed.");
  return true;
}

bool test_chain_unit_enchanced::clear_tx_pool(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  c.get_tx_pool().purge_transactions();
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool after purge_transactions(): " << c.get_pool_transactions_count());
  return true;
}

bool test_chain_unit_enchanced::check_tx_pool_empty(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  if (c.get_pool_transactions_count() != 0)
  {
    LOG_ERROR("Incorrect txs count in the pool: " << c.get_pool_transactions_count());
    LOG_PRINT_L0(ENDL << c.get_tx_pool().print_pool(true));
    return false;
  }
  return true;
}

bool test_chain_unit_enchanced::check_tx_pool_count(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  size_t txs_count = 0;
  const std::string& params = boost::get<callback_entry>(events[ev_index]).callback_params;
  CHECK_AND_ASSERT_MES(epee::string_tools::hex_to_pod(params, txs_count), false, "hex_to_pod failed, params = " << params);
  if (c.get_pool_transactions_count() != txs_count)
  {
    LOG_ERROR("Incorrect txs count in the pool: " << c.get_pool_transactions_count() << ", expected: " << txs_count);
    LOG_PRINT_L0(ENDL << c.get_tx_pool().print_pool(true));
    return false;
  }
  return true;
}

bool test_chain_unit_enchanced::print_tx_pool(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  LOG_PRINT_L0(ENDL << c.get_tx_pool().print_pool(true));
  return true;
}

std::string print_market(bc_services::bc_offers_service* offers_service)
{
  std::stringstream ss;
  size_t index = 0;
  for (auto& o : offers_service->get_offers_container())
  {
    ss  << "#" << index++ << ENDL
        << "id: " << o.h << ENDL
        << "nxt_id: " << o.nxt_offer << ENDL
        << "o.stopped: " << (o.stopped ? "TRUE" : "false") << ENDL
        << epee::serialization::store_t_to_json(o) << ENDL
        << "----------------------------------------------" << ENDL;
  }

  return ss.str();
}

bool test_chain_unit_enchanced::check_offers_count(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bc_services::bc_offers_service* offers_service = dynamic_cast<bc_services::bc_offers_service*>(c.get_blockchain_storage().get_attachment_services_manager().get_service_by_id(BC_OFFERS_SERVICE_ID));
  CHECK_AND_ASSERT_MES(offers_service != nullptr, false, "Offers service was not registered in attachment service manager!");

  offers_count_param param;
  bool r = epee::string_tools::hex_to_pod(boost::get<callback_entry>(events[ev_index]).callback_params, param);
  CHECK_AND_ASSERT_MES(r, false, "hex_to_pod failed");
  LOG_PRINT_YELLOW("check_offers_count(" << param.offers_count << ", " << param.offers_count_raw << ")", LOG_LEVEL_0);

  if (param.offers_count_raw != SIZE_MAX)
  {
    CHECK_AND_ASSERT_MES(offers_service->get_offers_container().size() == param.offers_count_raw, false, "Incorrect offers raw count: " << offers_service->get_offers_container().size() << ", expected: " << param.offers_count_raw
      << ENDL << "Market:" << ENDL << print_market(offers_service));
  }

  if (param.offers_count != SIZE_MAX)
  {
    std::list<bc_services::offer_details_ex> offers;

    bc_services::core_offers_filter cof = AUTO_VAL_INIT(cof);
    cof.limit = UINT64_MAX;
    cof.offset = 0;
    cof.order_by = ORDER_BY_TIMESTAMP;
    uint64_t total_count_stub;
    offers_service->get_offers_ex(cof, offers, total_count_stub, c.get_blockchain_storage().get_core_runtime_config().get_core_time());
  
    CHECK_AND_ASSERT_MES(offers.size() == param.offers_count, false, "Incorrect offers count: " << offers.size() << ", expected: " << param.offers_count
      << ENDL << "Market:" << ENDL << print_market(offers_service));
  }

  return true;
}
