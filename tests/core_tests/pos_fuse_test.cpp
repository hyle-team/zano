// Copyright (c) 2014-2022 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "pos_fuse_test.h"
#include "wallet_test_core_proxy.h"

#include "random_helper.h"
#include "wallet/wallet_debug_events_definitions.h"
using namespace currency;


using namespace currency;
pos_fuse_test::pos_fuse_test()
{
  REGISTER_CALLBACK_METHOD(pos_fuse_test, c1);
  REGISTER_CALLBACK_METHOD(pos_fuse_test, configure_core);
}

 bool pos_fuse_test::configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  wallet_test::configure_core(c, ev_index, events);
  currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
  pc.max_pos_difficulty = wide_difficulty_type(1);
  //currency::core_runtime_config pc2;
  //pc2 = pc;
  c.get_blockchain_storage().set_core_runtime_config(pc);


  currency::core_runtime_config pc2 = c.get_blockchain_storage().get_core_runtime_config();
  LOG_PRINT_L1("Difficulty: " << pc2.max_pos_difficulty);

  return true;
}

bool pos_fuse_test::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // necessary to set m_hardforks

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 10);

  DO_CALLBACK(events, "c1");
  return true;
}

bool pos_fuse_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  miner_wlt->refresh();



  while (true)
  {
    miner_wlt->refresh();
    wide_difficulty_type pos_diff = c.get_blockchain_storage().get_next_diff_conditional(true);

    r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

    bool r = miner_wlt->try_mint_pos();
    CHECK_AND_ASSERT_MES(r, false, "Failed ot mint pos block");

    currency::core_runtime_config pc = c.get_blockchain_storage().get_core_runtime_config();
    LOG_PRINT_MAGENTA("POS Difficulty: " << pos_diff << ", max allowed diff: " << pc.max_pos_difficulty, LOG_LEVEL_0);
    if (pos_diff > pc.max_pos_difficulty)
    {
      break;
    }
  }

  //check that PoW blocks not going 
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(!r, false, "PoW block unexpectedly generated");

  //check that PoS blocks not going 
  r = miner_wlt->try_mint_pos();
  CHECK_AND_ASSERT_MES(!r, false, "PoS block unexpectedly mined");

  try
  {
    miner_wlt->transfer(1000000, m_accounts[ALICE_ACC_IDX].get_public_address());
    CHECK_AND_ASSERT_MES(false, false, "Transaction unexpectedly sent");
  }
  catch (...)
  {
    LOG_PRINT_L0("Expected exception catched");
  }

  return true;
}
