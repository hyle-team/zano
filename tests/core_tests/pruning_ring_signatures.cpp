// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "pruning_ring_signatures.h"

using namespace epee;
using namespace currency;

struct ev_visitor : public boost::static_visitor<bool>
{
  bool operator()(currency::block& b) const
  {
    std::cout << currency::get_block_hash(b) << ENDL;
    return true;
  }
  bool operator()(event_special_block& b) const
  {
    return operator()(b.block);
  }
  bool operator()(currency::transaction& t)const
  {
    size_t real_bloc_size = t_serializable_object_to_blob(t).size();

    std::cout << "prunging sigs: " << t.signatures.size() << ENDL;
    t.signatures.clear();

    //check tx pruning correctnes
    if (real_bloc_size != get_object_blobsize(t))
    {
      ASSERT_MES_AND_THROW("real_bloc_size != get_object_blobsize(t)");
    }
    return true;
  }

  template<typename T>
  bool operator()(const T&) const { return false; }
};


bool prun_ring_signatures::generate_blockchain_with_pruned_rs(std::vector<test_event_entry>& events)const
{
  uint64_t ts_start = 1338224400;
  GENERATE_ACCOUNT(miner_account);
  //
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  DO_CALLBACK(events, "set_check_points");
  MAKE_ACCOUNT(events, some_account_1);
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_account);
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_account);
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_account);
  REWIND_BLOCKS_N(events, blk_5, blk_4, miner_account, 30);
  REWIND_BLOCKS(events, blk_5r, blk_5, miner_account);

  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
  MAKE_TX_LIST_START(events, txs_blk_6, miner_account, some_account_1, MK_TEST_COINS(1), blk_5r);
  MAKE_TX_LIST(events, txs_blk_6, miner_account, some_account_1, MK_TEST_COINS(1), blk_5r);
  MAKE_TX_LIST(events, txs_blk_6, miner_account, some_account_1, MK_TEST_COINS(1), blk_5r);
  MAKE_TX_LIST(events, txs_blk_6, miner_account, some_account_1, MK_TEST_COINS(1), blk_5r);
  MAKE_TX_LIST(events, txs_blk_6, miner_account, some_account_1, MK_TEST_COINS(1), blk_5r);
  MAKE_TX_LIST(events, txs_blk_6, miner_account, some_account_1, MK_TEST_COINS(1), blk_5r);
  MAKE_TX_LIST(events, txs_blk_6, miner_account, some_account_1, MK_TEST_COINS(1), blk_5r);
  MAKE_TX_LIST(events, txs_blk_6, miner_account, some_account_1, MK_TEST_COINS(1), blk_5r);
  MAKE_TX_LIST(events, txs_blk_6, miner_account, some_account_1, MK_TEST_COINS(1), blk_5r);
  MAKE_TX_LIST(events, txs_blk_6, miner_account, some_account_1, MK_TEST_COINS(1), blk_5r);
  MAKE_TX_LIST(events, txs_blk_6, miner_account, some_account_1, MK_TEST_COINS(1), blk_5r);
  MAKE_TX_LIST(events, txs_blk_6, miner_account, some_account_1, MK_TEST_COINS(1), blk_5r);
  MAKE_TX_LIST(events, txs_blk_6, miner_account, some_account_1, MK_TEST_COINS(1), blk_5r);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_6, blk_5r, miner_account, txs_blk_6);
  REWIND_BLOCKS_N(events, blk_6r, blk_6, miner_account, 20);

  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
  MAKE_TX_LIST_START(events, txs_blk_7, some_account_1, some_account_1, MK_TEST_COINS(1) + TESTS_DEFAULT_FEE, blk_6r);
  MAKE_TX_MIX_LIST(events, txs_blk_7, some_account_1, some_account_1, MK_TEST_COINS(1) + TESTS_DEFAULT_FEE, 3, blk_6r, std::vector<currency::attachment_v>());
  MAKE_TX_MIX_LIST(events, txs_blk_7, some_account_1, some_account_1, MK_TEST_COINS(1) + TESTS_DEFAULT_FEE, 3, blk_6r, std::vector<currency::attachment_v>());
  MAKE_TX_MIX_LIST(events, txs_blk_7, some_account_1, some_account_1, MK_TEST_COINS(1) + TESTS_DEFAULT_FEE, 3, blk_6r, std::vector<currency::attachment_v>());
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_7, blk_6r, miner_account, txs_blk_7);
  REWIND_BLOCKS(events, blk_7r, blk_7, miner_account);

  DO_CALLBACK(events, "check_blockchain");

  
  for (size_t i = 0; i != events.size(); i++)
  {
    boost::apply_visitor(ev_visitor(), events[i]);
  }
  
  return true;
}

prun_ring_signatures::prun_ring_signatures()
{
  REGISTER_CALLBACK("set_check_points", prun_ring_signatures::set_check_points);
  REGISTER_CALLBACK("check_blockchain", prun_ring_signatures::check_blockchain);
  
}

bool prun_ring_signatures::generate(std::vector<test_event_entry>& events) const
{
  generate_blockchain_with_pruned_rs(events);
  
  return true;
}

bool prun_ring_signatures::set_check_points(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  const currency::block& b = boost::get<currency::block>(events[events.size() - 4]);
  currency::checkpoints cp;
  cp.add_checkpoint(currency::get_block_height(b), epee::string_tools::pod_to_hex(currency::get_block_hash(b)));
  c.set_checkpoints(std::move(cp));
  return true;
}
bool prun_ring_signatures::check_blockchain(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  const currency::block& b = boost::get<currency::block>(events[events.size() - 2]);
  
  CHECK_EQ(c.get_current_blockchain_size(), currency::get_block_height(b) + 1);

  return true;
}