// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "wallet_tests.h"
#include "wallet_test_core_proxy.h"
#include "../../src/wallet/wallet_public_structs_defs.h"
#include "offers_helper.h"
#include "string_coding.h"

using namespace epee;
using namespace crypto;
using namespace currency;

wallet_test::wallet_test()
  : m_core_proxy(nullptr)
{
  REGISTER_CALLBACK_METHOD(wallet_test, check_balance);
}

bool wallet_test::check_balance_via_build_wallets(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // This is old version, it checks balance using test_generator::build_wallets(), which is using blockchain data from test_generator::m_blocks_info
  params_check_balance pcb = AUTO_VAL_INIT(pcb);
  bool r = epee::string_tools::hex_to_pod(boost::get<callback_entry>(events[ev_index]).callback_params, pcb);
  CHECK_AND_ASSERT_MES(r, false, "check_balance: Can't obtain event params");
  CHECK_AND_ASSERT_MES(pcb.account_index < m_accounts.size(), false, "check_balance: invalid account index");

  const account_base& acc = m_accounts[pcb.account_index];

  // obtain top block as most recent block
  const block* top_block = 0;
  size_t i = ev_index;
  do
  {
    --i;
    if (events[i].type() == typeid(block))
    {
      top_block = &boost::get<block>(events[i]);
      break;
    }
  } while (i != 0);
  CHECK_AND_ASSERT_MES(top_block != 0, false, "check_balance: failed to obtain top block from events queue");

  std::list<account_base> accounts(1, acc);
  test_generator::wallets_vector w;
  r = generator.build_wallets(get_block_hash(*top_block), accounts, w, c.get_blockchain_storage().get_core_runtime_config());
  CHECK_AND_ASSERT_MES(r && w.size() == 1 && w[0].wallet != 0, false, "check_balance: failed to build wallets");

  if (!check_balance_via_wallet(*w[0].wallet, get_test_account_name_by_id(pcb.account_index).c_str(), pcb.total_balance, pcb.mined_balance, pcb.unlocked_balance, pcb.awaiting_in, pcb.awaiting_out))
    return false;

  return true;
}

bool wallet_test::check_balance(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // Newer version, uses the core with actual data via special proxy to sync wallets and get balance.
  params_check_balance pcb = AUTO_VAL_INIT(pcb);
  bool r = epee::string_tools::hex_to_pod(boost::get<callback_entry>(events[ev_index]).callback_params, pcb);
  CHECK_AND_ASSERT_MES(r, false, "check_balance: Can't obtain event params");

  std::shared_ptr<tools::wallet2> w = init_playtime_test_wallet(events, c, pcb.account_index);
  w->refresh();
  bool has_aliases = false;
  w->scan_tx_pool(has_aliases);

  if (!check_balance_via_wallet(*w.get(), get_test_account_name_by_id(pcb.account_index).c_str(), pcb.total_balance, INVALID_BALANCE_VAL, pcb.unlocked_balance, pcb.awaiting_in, pcb.awaiting_out))
    return false;

  return true;
}

 std::string wallet_test::get_test_account_name_by_id(size_t acc_id)
 {
   switch(acc_id)
   {
   case MINER_ACC_IDX: return "miner";
   case ALICE_ACC_IDX: return "Alice";
   case BOB_ACC_IDX:   return "Bob";
   case CAROL_ACC_IDX: return "Carol";
   case DAN_ACC_IDX:   return "Dan";
   default:            return "unknown";
   }
 }



std::shared_ptr<tools::wallet2> wallet_test::init_playtime_test_wallet(const std::vector<test_event_entry>& events, currency::core& c, const account_base& acc) const
{
  return init_playtime_test_wallet_t<tools::wallet2>(events, c, acc);
}

std::shared_ptr<tools::wallet2> wallet_test::init_playtime_test_wallet(const std::vector<test_event_entry>& events, currency::core& c, size_t account_index) const
{
  CHECK_AND_ASSERT_THROW_MES(account_index < m_accounts.size(), "Invalid account index");
  return init_playtime_test_wallet(events, c, m_accounts[account_index]);
}

