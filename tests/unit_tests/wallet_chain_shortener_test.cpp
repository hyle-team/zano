// Copyright (c) 2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <algorithm>

#include "gtest/gtest.h"
#include "wallet/wallet_chain_shortener.h"

TEST(wallet_chain_shortener, wallet_chain_shortener)
{
  uint64_t counter = 0;
  wallet_chain_shortener ws;

  for (counter = 1; counter != 1000000; counter++)
  {
    crypto::hash id_ = AUTO_VAL_INIT(id_);
    *((uint64_t*)&id_) = counter;

    ws.push_new_block_id(id_, counter);
  }

  std::list<crypto::hash> short_chain1;
  ws.get_short_chain_history(short_chain1);
  for(auto& id: short_chain1)
  {
    LOG_PRINT_L0("{" << *((uint64_t*)&id) << "}{" << counter - *((uint64_t*)&id) << "}" << ENDL);
  }


  ws.detach(counter - 10000);
  std::list<crypto::hash> short_chain2;
  ws.get_short_chain_history(short_chain2);
  for (auto& id : short_chain2)
  {
    LOG_PRINT_L0("{" << *((uint64_t*)&id) << "}{" << counter - *((uint64_t*)&id) << "}" << ENDL);
  }

  for (counter = counter - 10000; counter != 1000000; counter++)
  {
    crypto::hash id_ = AUTO_VAL_INIT(id_);
    *((uint64_t*)&id_) = counter;

    ws.push_new_block_id(id_, counter);
  }

  std::list<crypto::hash> short_chain3;
  ws.get_short_chain_history(short_chain3);
  for (auto& id : short_chain3)
  {
    LOG_PRINT_L0("{" << *((uint64_t*)&id) << "}{" << counter - *((uint64_t*)&id) << "}" << ENDL);
  }

  LOG_PRINT_L0("Finished");

  ASSERT_EQ(short_chain3.size(), short_chain1.size());
  ASSERT_EQ(short_chain3, short_chain1);

}

