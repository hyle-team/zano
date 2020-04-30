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

  std::list<crypto::hash> short_chain;
  ws.get_short_chain_history(short_chain);
  for(auto& id: short_chain)
  {
    LOG_PRINT_L0("{" << *((uint64_t*)&id) << "}{" << counter - *((uint64_t*)&id) << "}" << ENDL);
  }


  ws.detach(counter - 10000);
  short_chain.clear();
  ws.get_short_chain_history(short_chain);
  for (auto& id : short_chain)
  {
    LOG_PRINT_L0("{" << *((uint64_t*)&id) << "}{" << counter - *((uint64_t*)&id) << "}" << ENDL);
  }

  for (counter = counter - 10000 + 1; counter != 1000000; counter++)
  {
    crypto::hash id_ = AUTO_VAL_INIT(id_);
    *((uint64_t*)&id_) = counter;

    ws.push_new_block_id(id_, counter);
  }

  short_chain.clear();
  ws.get_short_chain_history(short_chain);
  for (auto& id : short_chain)
  {
    LOG_PRINT_L0("{" << *((uint64_t*)&id) << "}{" << counter - *((uint64_t*)&id) << "}" << ENDL);
  }

  LOG_PRINT_L0("Finished");

}

