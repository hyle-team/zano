// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#pragma once
namespace currency
{
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct tx_verification_context
  {
    bool m_should_be_relayed;
    bool m_verification_failed; //bad tx, should drop connection
    bool m_verification_impossible; //the transaction is related to alternative blockchain
    bool m_added_to_pool; 
  };

  struct block_verification_context
  {
    bool m_added_to_main_chain;
    bool m_verification_failed; //bad block, should drop connection
    bool m_marked_as_orphaned;
    bool m_already_exists;
    bool m_added_to_altchain;
    uint64_t m_height_difference;
    //this is work like a first-level cache for transactions while block is getting handled. It lets transactions 
    //associated with the block to get handled directly to core without being handled by tx_pool(which makes full
    //inputs validation, including signatures check)
    transactions_map m_onboard_transactions;
  };
}
