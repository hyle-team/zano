// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 

#include "currency_boost_serialization.h"

namespace boost
{
  namespace serialization
  {


    template<class archive_t>
    void serialize(archive_t & ar, currency::transaction_chain_entry& te, const unsigned int version)
    {
      ar & te.tx;
      ar & te.m_keeper_block_height;
      ar & te.m_global_output_indexes;
      ar & te.m_spent_flags;
    }

    // The following method is used in tests only atm
    // TODO: Consider to remove completely
    template<class archive_t>
    void serialize(archive_t & ar, currency::block_extended_info& ei, const unsigned int version)
    {
      ar & ei.bl;
      ar & ei.height;
      ar & ei.cumulative_diff_adjusted;
      ar & ei.cumulative_diff_precise;
      ar & ei.difficulty;
      ar & ei.block_cumulative_size;
      ar & ei.already_generated_coins;
      ar & ei.stake_hash;

      ar & ei.cumulative_diff_precise_adjusted;
      //ar & ei.version;
      ar & ei.this_block_tx_fee_median;
      ar & ei.effective_tx_fee_median;
    }

  }
}
