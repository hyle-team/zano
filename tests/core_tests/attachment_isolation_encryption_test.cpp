// Copyright (c) 2014-2022 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "attachment_isolation_encryption_test.h"
#include "wallet_test_core_proxy.h"



using namespace currency;


attachment_isolation_test::attachment_isolation_test()
{
  REGISTER_CALLBACK_METHOD(attachment_isolation_test, configure_core);
  REGISTER_CALLBACK_METHOD(attachment_isolation_test, c1);

//  m_hardforks.set_hardfork_height(1, 1);
//  m_hardforks.set_hardfork_height(2, 1);
//  m_hardforks.set_hardfork_height(3, 1);
//  m_hardforks.set_hardfork_height(4, 14);
}

bool attachment_isolation_test::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // default configure_core callback will initialize core runtime config with m_hardforks
  //TODO: Need to make sure REWIND_BLOCKS_N and other coretests codebase are capable of following hardfork4 rules
  //in this test hardfork4 moment moved to runtime section
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  DO_CALLBACK(events, "c1");

  return true;
}

bool attachment_isolation_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  account_base alic_acc;
  alic_acc.generate();
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, alic_acc);

  // check passing over the hardfork
  //CHECK_AND_ASSERT_MES(!c.get_blockchain_storage().is_hardfork_active(ZANO_HARDFORK_04_ZARCANUM), false, "ZANO_HARDFORK_04_ZARCANUM is active");
  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 2);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_blockchain_storage().is_hardfork_active(ZANO_HARDFORK_04_ZARCANUM), false, "ZANO_HARDFORK_04_ZARCANUM is not active");

  miner_wlt->refresh();
  alice_wlt->refresh();


  CHECK_AND_FORCE_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  //create transfer from pre-zarcanum inputs to post-zarcanum inputs
  uint64_t transfer_amount = TESTS_DEFAULT_FEE + TESTS_DEFAULT_FEE;
  
  tools::construct_tx_param ctp = alice_wlt->get_default_construct_tx_param();
  tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
  tsa.body = "aaaaaaa";
  tsa.service_id = "X";
  tsa.flags = TX_SERVICE_ATTACHMENT_ENCRYPT_BODY | TX_SERVICE_ATTACHMENT_ENCRYPT_BODY_ISOLATE_AUDITABLE;
  ctp.extra.push_back(tsa);
  
  currency::tx_destination_entry dst = AUTO_VAL_INIT(dst);
  dst.amount = transfer_amount;
  dst.addr.push_back(alice_wlt->get_account().get_public_address());
  ctp.dsts.push_back(dst);
  currency::finalized_tx result = AUTO_VAL_INIT(result);
  miner_wlt->transfer(ctp, result, true);

  r = mine_next_pow_blocks_in_playtime(miner_wlt->get_account().get_public_address(), c, 2);


  alice_wlt->refresh();
  miner_wlt->refresh();

  uint64_t total = 0;
  uint64_t last_item_index = 0;
  std::vector<tools::wallet_public::wallet_transfer_info> trs;
  alice_wlt->get_recent_transfers_history(trs, 0, 100, total, last_item_index, true);
  CHECK_AND_ASSERT_MES(trs.size() == 1, false, "get_recent_transfers_history returned size != 1");
  CHECK_AND_ASSERT_MES(trs[0].service_entries.size() == 1, false, "get_recent_transfers_history service_entries size != 1");
  CHECK_AND_ASSERT_MES(trs[0].service_entries[0].service_id == tsa.service_id && trs[0].service_entries[0].body == tsa.body, false, "get_recent_transfers_history service_entries -> service entry do not match");

  trs.clear();
  miner_wlt->get_recent_transfers_history(trs, 0, 100, total, last_item_index, true);
  CHECK_AND_ASSERT_MES(trs.size() == 1, false, "get_recent_transfers_history returned size != 1");
  CHECK_AND_ASSERT_MES(trs[0].service_entries.size() == 1, false, "get_recent_transfers_history service_entries size != 1");
  CHECK_AND_ASSERT_MES(trs[0].service_entries[0].service_id == tsa.service_id && trs[0].service_entries[0].body == tsa.body, false, "get_recent_transfers_history service_entries -> service entry do not match");

  return true;
}
