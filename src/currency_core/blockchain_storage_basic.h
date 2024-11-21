// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <unordered_map>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include <boost/interprocess/sync/named_mutex.hpp>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/list.hpp>

#include "currency_basic.h"
#include "difficulty.h"
#include "currency_protocol/blobdatatype.h"
#include "currency_format_utils_transactions.h" // only for output_generation_context

namespace currency
{

  struct transaction_chain_entry
  {
    transaction tx;
    uint64_t m_keeper_block_height;
    std::vector<uint64_t> m_global_output_indexes;
    std::vector<bool> m_spent_flags;
    uint32_t version;

    DEFINE_SERIALIZATION_VERSION(1);
    BEGIN_SERIALIZE_OBJECT()
      VERSION_ENTRY(version)
      FIELD(version)
      FIELDS(tx)
      FIELD(m_keeper_block_height)
      FIELD(m_global_output_indexes)
      FIELD(m_spent_flags)
    END_SERIALIZE()
  };

  struct block_extended_info
  {
    block   bl;
    uint64_t height;
    uint64_t block_cumulative_size;
    wide_difficulty_type cumulative_diff_adjusted; // used only before hardfork 1
    wide_difficulty_type cumulative_diff_precise;
    wide_difficulty_type cumulative_diff_precise_adjusted; //used after hardfork 1 (cumulative difficulty adjusted only by sequence factor)
    wide_difficulty_type difficulty;
    boost::multiprecision::uint128_t already_generated_coins;
    crypto::hash stake_hash;                  //TODO: unused field for PoW blocks, subject for refactoring
    uint32_t version;
    uint64_t this_block_tx_fee_median;        //tx fee median for current block transactions 
    uint64_t effective_tx_fee_median;         //current fee median which applied for current block's alias registrations 

    DEFINE_SERIALIZATION_VERSION(1);
    BEGIN_SERIALIZE_OBJECT()
      VERSION_ENTRY(version)
      FIELDS(bl)
      FIELD(height)
      FIELD(block_cumulative_size)
      FIELD(cumulative_diff_adjusted)
      FIELD(cumulative_diff_precise)
      FIELD(cumulative_diff_precise_adjusted)
      FIELD(difficulty)
      FIELD(already_generated_coins)
      FIELD(stake_hash)
      FIELD(this_block_tx_fee_median)
      FIELD(effective_tx_fee_median)
    END_SERIALIZE()
  };

  struct gindex_increment
  {
    uint64_t amount;    // the amount in global outputs table
    uint32_t increment; // how many outputs of such amount a block adds to global outputs container

    static gindex_increment construct(uint64_t amount, uint32_t increment)
    {
      gindex_increment r = { amount, increment };
      return r;
    }

    BEGIN_SERIALIZE()
      VARINT_FIELD(amount)
      VARINT_FIELD(increment)
    END_SERIALIZE()
  };

  struct block_gindex_increments
  {
    std::vector<gindex_increment> increments;

    BEGIN_SERIALIZE()
      FIELD(increments)
    END_SERIALIZE()
  };

  // element of txout_to_key global outputs container
  struct global_output_entry
  {
    crypto::hash tx_id;       // corresponding tx
    uint32_t     out_no;      // local output index in tx

    static global_output_entry construct(crypto::hash tx_id, uint32_t out_no)
    {
      global_output_entry v = { tx_id, out_no };
      return v;
    }
  };

  // element of multisig DB container
  struct ms_output_entry
  {
    crypto::hash tx_id;         // source tx id
    uint32_t     out_no;        // output index in source tx
    uint32_t     spent_height;  // height at which this output was spent (0 - not spent yet)

    static ms_output_entry construct(crypto::hash tx_id, uint32_t out_no, uint32_t spent_height = 0)
    {
      ms_output_entry v = { tx_id, out_no, spent_height };
      return v;
    }
  };

  typedef bool fill_block_template_func_t(block &bl, bool pos, size_t median_size, const boost::multiprecision::uint128_t& already_generated_coins, size_t &total_size, uint64_t &fee, uint64_t height);

  struct create_block_template_params
  {
    account_public_address miner_address;
    account_public_address stakeholder_address;
    blobdata ex_nonce;
    bool pos = false;
    bool ignore_pow_ts_check = false;
    pos_entry pe;
    std::list<transaction> explicit_txs;
    fill_block_template_func_t *pcustom_fill_block_template_func;
  };

  struct create_block_template_response
  {
    block b;
    wide_difficulty_type diffic;
    uint64_t height;
    tx_generation_context miner_tx_tgc; // bad design, a lot of copying, consider redesign -- sowle
    uint64_t block_reward_without_fee;
    uint64_t block_reward;   // == block_reward_without_fee + txs_fee if fees are given to the miner, OR block_reward_without_fee if fees are burnt
    uint64_t txs_fee; // sum of transactions' fee if any
  };

  typedef std::unordered_map<crypto::hash, transaction> transactions_map;

  struct block_ws_txs
  {
    block b;
    transactions_map onboard_transactions;
  };

  struct vote_on_proposal
  {
    std::string proposal_id;
    uint64_t yes;
    uint64_t no;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(proposal_id)                  DOC_DSCR("ID of the proposal.") DOC_EXMP("ZAP999") DOC_END
      KV_SERIALIZE(yes)                          DOC_DSCR("Nubmer of positve votes.") DOC_EXMP(42) DOC_END
      KV_SERIALIZE(no)                           DOC_DSCR("Number of negative votes.") DOC_EXMP(37) DOC_END
    END_KV_SERIALIZE_MAP()
  };

  struct vote_results
  {
    uint64_t total_pos_blocks; //total pos blocks in a given range
    std::list<vote_on_proposal> votes;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(total_pos_blocks)             DOC_DSCR("Number of blocks in a given range.") DOC_EXMP(87482) DOC_END
      KV_SERIALIZE(votes)                        DOC_DSCR("Result of votes in a given range.") DOC_EXMP_AUTO(2) DOC_END
    END_KV_SERIALIZE_MAP()
  };

} // namespace currency
