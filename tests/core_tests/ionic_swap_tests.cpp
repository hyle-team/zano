// Copyright (c) 2014-2022 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "ionic_swap_tests.h"
#include "wallet_test_core_proxy.h"

#include "random_helper.h"
#include "tx_builder.h"


ionic_swap_basic_test::ionic_swap_basic_test()
{
  REGISTER_CALLBACK_METHOD(ionic_swap_basic_test, configure_core);
  REGISTER_CALLBACK_METHOD(ionic_swap_basic_test, c1);

  m_hardforks.set_hardfork_height(1, 1);
  m_hardforks.set_hardfork_height(2, 1);
  m_hardforks.set_hardfork_height(3, 1);
  m_hardforks.set_hardfork_height(4, 2);
}

bool zarcanum_basic_test::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc =   m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);
  //account_base& carol_acc = m_accounts[CAROL_ACC_IDX]; carol_acc.generate(); carol_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  DO_CALLBACK(events, "c1");

  return true;
}

bool zarcanum_basic_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt   = init_playtime_test_wallet(events, c, BOB_ACC_IDX);

  // check passing over the hardfork
  CHECK_AND_ASSERT_MES(!c.get_blockchain_storage().is_hardfork_active(ZANO_HARDFORK_04_ZARCANUM), false, "ZANO_HARDFORK_04_ZARCANUM is active");

  return true;
}

