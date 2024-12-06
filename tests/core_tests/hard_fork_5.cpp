// Copyright (c) 2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "chaingen.h"
#include "hard_fork_5.h"
#include "random_helper.h"

using namespace currency;

bool hard_fork_5_tx_version::generate(std::vector<test_event_entry>& events) const
{
  //
  // Test idea: 
  //
  uint64_t ts = test_core_time::get_time();
  GENERATE_ACCOUNT(miner_acc);
  GENERATE_ACCOUNT(alice_acc);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  //// rebuild genesis miner tx
  //std::vector<tx_destination_entry> destinations;
  //destinations.emplace_back(MK_TEST_COINS(1), alice_acc.get_public_address());
  //destinations.emplace_back(MK_TEST_COINS(1), alice_acc.get_public_address());
  //CHECK_AND_ASSERT_MES(replace_coinbase_in_genesis_block(destinations, generator, events, blk_0), false, ""); // leftover amount will be also send to miner

  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  MAKE_TX(events, tx_1, miner_acc, alice_acc, MK_TEST_COINS(1), blk_0r);

  return true;
}
