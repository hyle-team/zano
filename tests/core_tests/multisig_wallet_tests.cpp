// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "multisig_wallet_tests.h"
#include "pos_block_builder.h"
#include "tx_builder.h"

using namespace epee;
using namespace crypto;
using namespace currency;

#define TMP_LOG_FLOOD_STOP // <- comment this when debugging a broken test to skip log filtering

#if defined(TMP_LOG_FLOOD_STOP)
#define TMP_LOG_SILENT  log_level = log_space::get_set_log_detalisation_level(); \
  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_SILENT)
#define TMP_LOG_RESTORE log_space::get_set_log_detalisation_level(true, log_level)
#else
#define TMP_LOG_SILENT                        
#define TMP_LOG_RESTORE
#endif

static void exception_handler(){}
//==============================================================================================================================

// helper routine: creates multisig-spending tx using a wallet and keys of other ms-participants, then sends it to the core proxy
void transfer_multisig(tools::wallet2& w,
  const std::list<currency::account_keys>& owner_keys, 
  const crypto::hash& multisig_id,
  const std::vector<currency::tx_destination_entry>& dsts,
  uint64_t unlock_time,
  uint64_t fee,
  const std::vector<currency::extra_v>& extra,
  const std::vector<currency::attachment_v>& attachments,
  tools::detail::split_strategy_id_t split_strategy_id,
  const tools::tx_dust_policy& dust_policy,
  currency::transaction &tx,
  uint64_t tx_version,
  uint8_t tx_outs_attr = CURRENCY_TO_KEY_OUT_RELAXED,
  uint64_t flags = 0,
  bool shuffle = true, 
  bool send_to_network = true)
{
    currency::account_public_address crypt_address = get_crypt_address_from_destinations(w.get_account().get_keys(), dsts);

    // prepare transaction will sign ms input partially with wallet's keys - it needed to be signed fully with the others
    tools::construct_tx_param ctp = AUTO_VAL_INIT(ctp);
    currency::finalize_tx_param ftp = AUTO_VAL_INIT(ftp);
    ctp.attachments = attachments;
    ctp.crypt_address = crypt_address;
    ctp.dsts = dsts;
    ctp.dust_policy = dust_policy;
    ctp.extra = extra;
    ctp.fake_outputs_count = 0;
    ctp.fee = fee;
    ctp.flags = flags;
    ctp.mark_tx_as_complete = false;
    ctp.multisig_id = multisig_id;
    ctp.shuffle = shuffle;
    ctp.split_strategy_id = split_strategy_id;
    ctp.tx_outs_attr = tx_outs_attr;
    ctp.unlock_time = unlock_time;
    
    
    ftp.tx_version = tx_version;
    tools::mode_separate_context emode_separate = AUTO_VAL_INIT(emode_separate);
    emode_separate.tx_for_mode_separate = tx;
    w.prepare_transaction(ctp, ftp, emode_separate);
    crypto::secret_key sk = AUTO_VAL_INIT(sk);
    w.finalize_transaction(ftp, tx, sk, false);

    // sign ms input with all other non-wallet keys
    auto it = w.get_multisig_transfers().find(multisig_id);
    THROW_IF_FALSE_WALLET_INT_ERR_EX(it != w.get_multisig_transfers().end(), "can't find multisig_id: " << multisig_id);
    const currency::transaction& ms_source_tx = it->second.m_ptx_wallet_info->m_tx;
    bool is_tx_input_fully_signed = false;
    bool r = false;
    for (const currency::account_keys& keys : owner_keys)
    {
      //if (keys == w.get_account().get_keys())
      //  continue;
      
      THROW_IF_FALSE_WALLET_INT_ERR_EX(!is_tx_input_fully_signed, "is_tx_input_fully_signed == true, expected: false");
      r = sign_multisig_input_in_tx_custom(tx, 0, keys, ms_source_tx, &is_tx_input_fully_signed, true);
      THROW_IF_FALSE_WALLET_INT_ERR_EX(r, "sign_multisig_input_in_tx failed");
    }
    THROW_IF_FALSE_WALLET_INT_ERR_EX(is_tx_input_fully_signed, "is_tx_input_fully_signed == false, expected: true");

    // send to network
    if (send_to_network)
    {
      COMMAND_RPC_SEND_RAW_TX::request req;
      req.tx_as_hex = epee::string_tools::buff_to_hex_nodelimer(tx_to_blob(tx));
      COMMAND_RPC_SEND_RAW_TX::response daemon_send_resp;
      bool r = w.get_core_proxy()->call_COMMAND_RPC_SEND_RAW_TX(req, daemon_send_resp);
      THROW_IF_TRUE_WALLET_EX(!r, tools::error::no_connection_to_daemon, "sendrawtransaction");
      THROW_IF_TRUE_WALLET_EX(daemon_send_resp.status == API_RETURN_CODE_BUSY, tools::error::daemon_busy, "sendrawtransaction");
      THROW_IF_TRUE_WALLET_EX(daemon_send_resp.status == API_RETURN_CODE_DISCONNECTED, tools::error::wallet_internal_error, "Transfer attempt while daemon offline");
      THROW_IF_TRUE_WALLET_EX(daemon_send_resp.status != API_RETURN_CODE_OK, tools::error::tx_rejected, tx, daemon_send_resp.status);
    }
}



//==============================================================================================================================

multisig_wallet_test::multisig_wallet_test()
{
  REGISTER_CALLBACK_METHOD(multisig_wallet_test, c1);
}

bool multisig_wallet_test::generate(std::vector<test_event_entry>& events) const
{
  currency::account_base genesis_acc;
  genesis_acc.generate();
  m_mining_accunt.generate();
  m_accunt_a.generate();
  m_accunt_b.generate();
  m_accunt_c.generate();


  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, genesis_acc, test_core_time::get_time());
  events.push_back(blk_0);

  REWIND_BLOCKS_N(events, blk_0r, blk_0, m_mining_accunt, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK(events, "c1");

  return true;
}

bool multisig_wallet_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
#define AMOUNT_TO_TRANSFER_MULTISIG    MK_TEST_COINS(11)

  // Test outline:
  // 1. Generate multisig and send it to blockchain
  // 2. Transfer from multisig

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, m_mining_accunt);
  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW, false, "Incorrect numbers of blocks fetched");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  std::vector<currency::extra_v> extra;
  std::vector<currency::attachment_v> attachments;

  std::vector<tx_destination_entry> dst;
  dst.resize(1);
  //multisig
  dst.back().addr.push_back(m_accunt_a.get_public_address());
  dst.back().addr.push_back(m_accunt_b.get_public_address());
  dst.back().amount = AMOUNT_TO_TRANSFER_MULTISIG + TESTS_DEFAULT_FEE;
  dst.back().minimum_sigs = dst.back().addr.size();
  transaction result_tx = AUTO_VAL_INIT(result_tx);
  miner_wlt->transfer(dst, 0, 0, TESTS_DEFAULT_FEE, extra, attachments, tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), result_tx);

  bool r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  //findout multisig out intex
  size_t i = 0;
  for (; i != result_tx.vout.size(); i++)
  {
    if (boost::get<currency::tx_out_bare>(result_tx.vout[i]).target.type() == typeid(txout_multisig))
      break;
  }
  CHECK_AND_ASSERT_MES(i != result_tx.vout.size(), false, "Incorrect txs outs");

  crypto::hash multisig_id = get_multisig_out_id(result_tx, i);
  CHECK_AND_ASSERT_MES(multisig_id != null_hash, false, "Multisig failed: failed to get get_multisig_out_id");

  r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  std::shared_ptr<tools::wallet2> wallet_a = init_playtime_test_wallet(events, c, m_accunt_a);
  std::shared_ptr<tools::wallet2> wallet_b = init_playtime_test_wallet(events, c, m_accunt_b);

  wallet_a->refresh();
  wallet_b->refresh();
  tools::multisig_transfer_container ms_a, ms_b;
  wallet_a->get_multisig_transfers(ms_a);
  wallet_b->get_multisig_transfers(ms_b);
  CHECK_AND_ASSERT_MES(ms_a.size() == 1 && ms_b.size() == 1, false, "Multisig failed: ms_a.size() == 1 && ms_b.size() == 1");

  uint64_t tx_version = currency::get_tx_version(c.get_current_blockchain_size(), c.get_blockchain_storage().get_core_runtime_config().hard_forks) ;
  std::vector<tx_destination_entry> dst2(1);
  dst2.back().addr.resize(1);
  dst2.back().addr.back() = m_accunt_c.get_public_address();
  dst2.back().amount = AMOUNT_TO_TRANSFER_MULTISIG;
  dst2.back().minimum_sigs = dst2.back().addr.size();
  std::list<currency::account_keys> acc_keys;
  //acc_keys.push_back(m_accunt_a.get_keys());
  acc_keys.push_back(m_accunt_b.get_keys());
  transfer_multisig(*wallet_a.get(),
    acc_keys,
    multisig_id,
    dst2,
    0,
    TESTS_DEFAULT_FEE,
    extra,
    attachments,
    tools::detail::ssi_digit,
    tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD),
    result_tx, 
    tx_version);

  r = mine_next_pow_blocks_in_playtime(m_mining_accunt.get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  wallet_a->refresh();
  wallet_b->refresh();
  ms_a.clear(); ms_b.clear();
  wallet_b->get_multisig_transfers(ms_a);
  wallet_b->get_multisig_transfers(ms_b);
  CHECK_AND_ASSERT_MES(ms_a.size() == 1 && ms_b.size() == 1, false, "Multisig failed: ms_a.size() == 1 && ms_b.size() == 1");
  CHECK_AND_ASSERT_MES(ms_a.begin()->second.is_spent() == true && ms_b.begin()->second.is_spent() == true, false, "ms_a.begin()->second.m_spent == false || ms_b.begin()->second.m_spent == false");

  std::shared_ptr<tools::wallet2> wallet_c = init_playtime_test_wallet(events, c, m_accunt_c);
  wallet_c->refresh();
  uint64_t balance = wallet_c->balance();
  CHECK_AND_ASSERT_MES(balance == AMOUNT_TO_TRANSFER_MULTISIG, false, "Multisig failed: balance missmatch");

  return true;
}

//------------------------------------------------------------------------------

multisig_wallet_test_many_dst::multisig_wallet_test_many_dst()
{
  REGISTER_CALLBACK_METHOD(multisig_wallet_test_many_dst, c1);
}

bool multisig_wallet_test_many_dst::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);

  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, preminer_acc, test_core_time::get_time());
  events.push_back(blk_0);

  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK(events, "c1");

  return true;
}

bool multisig_wallet_test_many_dst::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // Test outline:
  // 1. Generate few PoW blocks.
  // 2. Transfer multisig tx to enormous amount of receivers (addresses_count)
  // 3. Receive the transfer, check it's ok.
  // 4. Send the money to Alice and make sure she successfully receives them.
  static const size_t addresses_count = 700;

  // !!!
  // NOTE: When debugging this test undefine TMP_LOG_FLOOD_STOP to skip log filtering!
  // !!!
  int log_level = 0;
  bool r = false;

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, m_accounts[MINER_ACC_IDX]);
  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW, false, "Incorrect numbers of blocks fetched");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  uint64_t amount = 0;
  miner_wlt->balance(amount);
  CHECK_AND_ASSERT_MES(amount > TESTS_DEFAULT_FEE, false, "miner has unexpected balance");
  amount -= TESTS_DEFAULT_FEE;

  std::vector<currency::account_base> addresses(addresses_count);
  tx_destination_entry de = AUTO_VAL_INIT(de);
  for (size_t i = 0; i < addresses_count; ++i)
  {
    addresses[i].generate();
    de.addr.push_back(addresses[i].get_public_address());
  }
  de.amount = amount;
  de.minimum_sigs = de.addr.size();

  transaction result_tx = AUTO_VAL_INIT(result_tx);
  TMP_LOG_SILENT;
  miner_wlt->transfer(std::vector<tx_destination_entry>({ de }), 0, 0, TESTS_DEFAULT_FEE, std::vector<currency::extra_v>(), std::vector<currency::attachment_v>(), tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), result_tx);
  TMP_LOG_RESTORE;

  auto it = std::find_if(result_tx.vout.begin(), result_tx.vout.end(), [](tx_out_v& o) 
  {
    return boost::get<tx_out_bare>(o).target.type() == typeid(txout_multisig);
  });
  CHECK_AND_ASSERT_MES(it != result_tx.vout.end(), false, "Can't find output txout_multisig");
  size_t multisig_index = it - result_tx.vout.begin();

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  std::shared_ptr<tools::wallet2> w = init_playtime_test_wallet(events, c, addresses[0]);
  w->refresh();

  std::list<currency::account_keys> owner_keys;
  for (auto a : addresses)
    owner_keys.push_back(a.get_keys());
  owner_keys.pop_front(); // the first one was used to initialize the wallet 'w'

  tx_destination_entry de2(amount - TESTS_DEFAULT_FEE, m_accounts[ALICE_ACC_IDX].get_public_address());
  transaction tx = AUTO_VAL_INIT(tx);
  TMP_LOG_SILENT;
  transfer_multisig(*w.get(), owner_keys, get_multisig_out_id(result_tx, multisig_index), std::vector<tx_destination_entry>({ de2 }), 0, TESTS_DEFAULT_FEE, std::vector<currency::extra_v>(), std::vector<currency::attachment_v>(), tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), tx, c.get_current_tx_version());
  TMP_LOG_RESTORE;

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW * 2 + 1, false, "Incorrect numbers of blocks fetched");

  r = check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", amount - TESTS_DEFAULT_FEE);
  CHECK_AND_ASSERT_MES(r, false, "invalid balance");

  tools::multisig_transfer_container mstc;
  alice_wlt->get_multisig_transfers(mstc);
  CHECK_AND_ASSERT_MES(mstc.empty(), false, "Got invalid multisig transfer");

  return true;
}

//------------------------------------------------------------------------------

multisig_wallet_heterogenous_dst::multisig_wallet_heterogenous_dst()
{
  REGISTER_CALLBACK_METHOD(multisig_wallet_heterogenous_dst, c1);
}

bool multisig_wallet_heterogenous_dst::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);
  GENERATE_ACCOUNT(bob_acc);
  m_accounts.push_back(bob_acc);
  GENERATE_ACCOUNT(carol_acc);
  m_accounts.push_back(carol_acc);
  GENERATE_ACCOUNT(dan_acc);
  m_accounts.push_back(dan_acc);

  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, preminer_acc, test_core_time::get_time());
  events.push_back(blk_0);

  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK(events, "c1");

  return true;
}

bool multisig_wallet_heterogenous_dst::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, m_accounts[MINER_ACC_IDX]);
  size_t blocks_fetched = 0;
  bool received_money = false;
  bool r = false;
  std::atomic<bool> atomic_false(false);
  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW, false, "Incorrect numbers of blocks fetched");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  uint64_t amount1 = TESTS_DEFAULT_FEE * 13 + 1;
  uint64_t amount2 = TESTS_DEFAULT_FEE * 11 + 2;
  uint64_t amount3 = TESTS_DEFAULT_FEE * 7 + 3;
  uint64_t amount4 = TESTS_DEFAULT_FEE * 5 + 4;
  uint64_t amount5 = TESTS_DEFAULT_FEE * 21;
  uint64_t amount6 = TESTS_DEFAULT_FEE * 23;

  tx_destination_entry de1 = AUTO_VAL_INIT(de1);
  de1.addr.push_back(m_accounts[ALICE_ACC_IDX].get_public_address());
  de1.addr.push_back(m_accounts[BOB_ACC_IDX].get_public_address());
  de1.amount = amount1 + TESTS_DEFAULT_FEE;
  de1.minimum_sigs = de1.addr.size();
  tx_destination_entry de2 = AUTO_VAL_INIT(de2);
  de2.addr.push_back(m_accounts[CAROL_ACC_IDX].get_public_address());
  de2.addr.push_back(m_accounts[DAN_ACC_IDX].get_public_address());
  de2.amount = amount2 + TESTS_DEFAULT_FEE;
  de2.minimum_sigs = de2.addr.size();
  tx_destination_entry de3 = AUTO_VAL_INIT(de3);
  de3.addr.push_back(m_accounts[ALICE_ACC_IDX].get_public_address());
  de3.addr.push_back(m_accounts[DAN_ACC_IDX].get_public_address());
  de3.amount = amount3 + TESTS_DEFAULT_FEE;
  de3.minimum_sigs = de3.addr.size();
  tx_destination_entry de4 = AUTO_VAL_INIT(de4);
  de4.addr.push_back(m_accounts[CAROL_ACC_IDX].get_public_address());
  de4.addr.push_back(m_accounts[BOB_ACC_IDX].get_public_address());
  de4.amount = amount4 + TESTS_DEFAULT_FEE;
  de4.minimum_sigs = de4.addr.size();
  tx_destination_entry de5 = AUTO_VAL_INIT(de5);
  de5.addr.push_back(m_accounts[ALICE_ACC_IDX].get_public_address());
  de5.amount = amount5 + TESTS_DEFAULT_FEE;
  de5.minimum_sigs = de5.addr.size();
  tx_destination_entry de6 = AUTO_VAL_INIT(de6);
  de6.addr.push_back(m_accounts[DAN_ACC_IDX].get_public_address());
  de6.amount = amount6 + TESTS_DEFAULT_FEE;
  de6.minimum_sigs = de6.addr.size();

  // Send multisig tx
  transaction ms_tx = AUTO_VAL_INIT(ms_tx);
  miner_wlt->transfer(std::vector<tx_destination_entry>({ de1, de2, de3, de4, de5, de6 }),
    0, 0, TESTS_DEFAULT_FEE, std::vector<currency::extra_v>(), std::vector<currency::attachment_v>(), tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), ms_tx);

  // calculate multisig hashes for further usage
  crypto::hash ms1_hash = currency::get_multisig_out_id(ms_tx, get_tx_out_index_by_amount(ms_tx, de1.amount));
  crypto::hash ms2_hash = currency::get_multisig_out_id(ms_tx, get_tx_out_index_by_amount(ms_tx, de2.amount));
  crypto::hash ms3_hash = currency::get_multisig_out_id(ms_tx, get_tx_out_index_by_amount(ms_tx, de3.amount));
  crypto::hash ms4_hash = currency::get_multisig_out_id(ms_tx, get_tx_out_index_by_amount(ms_tx, de4.amount));
  CHECK_AND_ASSERT_MES(ms1_hash != null_hash && ms2_hash != null_hash && ms3_hash != null_hash && ms4_hash != null_hash, false, "Can't get multisig out id");

  // Mine a block and make sure tx was put into it
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  // Check Alice's wallet
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1, false, "Incorrect numbers of blocks fetched");
  r = check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", amount5 + TESTS_DEFAULT_FEE);
  CHECK_AND_ASSERT_MES(r, false, "Invalid wallet balance");
  tools::multisig_transfer_container alice_mstc;
  alice_wlt->get_multisig_transfers(alice_mstc);
  CHECK_AND_ASSERT_MES(alice_mstc.size() == 2 && alice_mstc.count(ms1_hash) == 1 && alice_mstc.count(ms3_hash) == 1, false, "Alice has incorrect multisig transfers");

  // Check Bob's wallet
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, m_accounts[BOB_ACC_IDX]);
  bob_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1, false, "Incorrect numbers of blocks fetched");
  r = check_balance_via_wallet(*bob_wlt.get(), "bob_wlt", 0);
  CHECK_AND_ASSERT_MES(r, false, "Invalid wallet balance");
  tools::multisig_transfer_container bob_mstc;
  bob_wlt->get_multisig_transfers(bob_mstc);
  CHECK_AND_ASSERT_MES(bob_mstc.size() == 2 && bob_mstc.count(ms1_hash) == 1 && bob_mstc.count(ms4_hash) == 1, false, "Bob has incorrect multisig transfers");

  // Check Carol's wallet
  std::shared_ptr<tools::wallet2> carol_wlt = init_playtime_test_wallet(events, c, m_accounts[CAROL_ACC_IDX]);
  carol_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1, false, "Incorrect numbers of blocks fetched");
  r = check_balance_via_wallet(*carol_wlt.get(), "carol_wlt", 0);
  CHECK_AND_ASSERT_MES(r, false, "Invalid wallet balance");
  tools::multisig_transfer_container carol_mstc;
  carol_wlt->get_multisig_transfers(carol_mstc);
  CHECK_AND_ASSERT_MES(carol_mstc.size() == 2 && carol_mstc.count(ms2_hash) == 1 && carol_mstc.count(ms4_hash) == 1, false, "Carol has incorrect multisig transfers");

  // Check Dan's wallet
  std::shared_ptr<tools::wallet2> dan_wlt = init_playtime_test_wallet(events, c, m_accounts[DAN_ACC_IDX]);
  dan_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1, false, "Incorrect numbers of blocks fetched");
  r = check_balance_via_wallet(*dan_wlt.get(), "dan_wlt", amount6 + TESTS_DEFAULT_FEE);
  CHECK_AND_ASSERT_MES(r, false, "Invalid wallet balance");
  tools::multisig_transfer_container dan_mstc;
  dan_wlt->get_multisig_transfers(dan_mstc);
  CHECK_AND_ASSERT_MES(dan_mstc.size() == 2 && dan_mstc.count(ms2_hash) == 1 && dan_mstc.count(ms3_hash) == 1, false, "Dan has incorrect multisig transfers");


  // Spending multisig 1: Alice & Bob
  // 1. Alice spends multisig out - send tx to the pool, the Miner add it to a block (so tx become confirmed)
  // 2. Bob refreshes his wallet ans tries to do the same
  // 3. Make sure Bob can't create multisig tx
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  GENERATE_ACCOUNT(receiver_acc);

  currency::transaction tx = AUTO_VAL_INIT(tx);
  transfer_multisig(*alice_wlt.get(), std::list<account_keys>({ m_accounts[BOB_ACC_IDX].get_keys() }), ms1_hash, std::vector<tx_destination_entry>({ tx_destination_entry(amount1, receiver_acc.get_public_address()) }),
    0, TESTS_DEFAULT_FEE, std::vector<currency::extra_v>(), std::vector<currency::attachment_v>(), tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), tx, c.get_current_tx_version());
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  // Once Bob refreshes his wallet he should see that Alice has already spent they shared multisig
  bob_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 1, false, "Incorrect numbers of blocks fetched");

  bool caught = false;
  try
  {
    // as 'false' means don't send to network. This should fail during preparation, not during sending/processing
    transfer_multisig(*bob_wlt.get(), std::list<account_keys>({ m_accounts[ALICE_ACC_IDX].get_keys() }), ms1_hash, std::vector<tx_destination_entry>({ tx_destination_entry(amount1, receiver_acc.get_public_address()) }),
      0, TESTS_DEFAULT_FEE, std::vector<currency::extra_v>(), std::vector<currency::attachment_v>(), tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), tx, 0, 0, true, false);
  }
  catch (tools::error::wallet_internal_error&)
  {
    caught = true;
  }
  CHECK_AND_ASSERT_MES(caught, false, "Bob was able to make multisig tx for alreadly spent output");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");


  // Spending multisig 2: Carol & Dan
  // 1. Carol spends multisig out - send tx to the pool (it's unconfirmed)
  // 2. Dan refreshes wallet and tries to do the same
  // 3. Make sure Dan can't do this
  // 4. Clear the pool
  // 5. Make sure Dan now is able to send tx after it refreshes his wallet

  tx = AUTO_VAL_INIT(tx);
  transfer_multisig(*carol_wlt.get(), std::list<account_keys>({ m_accounts[DAN_ACC_IDX].get_keys() }), ms2_hash, std::vector<tx_destination_entry>({ tx_destination_entry(amount2, receiver_acc.get_public_address()) }),
    0, TESTS_DEFAULT_FEE, std::vector<currency::extra_v>(), std::vector<currency::attachment_v>(), tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), tx, c.get_current_tx_version());

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");

  bool stub;
  dan_wlt->scan_tx_pool(stub);

  caught = false;
  try
  {
    // as 'false' means don't send to network. This should fail during preparation, not during sending/processing
    transfer_multisig(*dan_wlt.get(), std::list<account_keys>({ m_accounts[CAROL_ACC_IDX].get_keys() }), ms2_hash, std::vector<tx_destination_entry>({ tx_destination_entry(amount2, receiver_acc.get_public_address()) }),
      0, TESTS_DEFAULT_FEE, std::vector<currency::extra_v>(), std::vector<currency::attachment_v>(), tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), tx, 0, 0, true, false);
  }
  catch (tools::error::wallet_internal_error&)
  {
    caught = true;
  }
  CHECK_AND_ASSERT_MES(caught, false, "Dan was able to make multisig tx for alreadly spent output");

  // Clear tx pool
  c.get_tx_pool().purge_transactions();
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "There are txs in the pool.");

  dan_wlt->scan_tx_pool(stub);

  // Re-try spending Carol-Dan multisig out on behalf of Dan. It should be OK now
  transfer_multisig(*dan_wlt.get(), std::list<account_keys>({ m_accounts[CAROL_ACC_IDX].get_keys() }), ms2_hash, std::vector<tx_destination_entry>({ tx_destination_entry(amount2, receiver_acc.get_public_address()) }),
    0, TESTS_DEFAULT_FEE, std::vector<currency::extra_v>(), std::vector<currency::attachment_v>(), tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), tx, c.get_current_tx_version());

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "There are txs in the pool.");


  // Spending multisig 3: Alice & Dan
  // 1. Alice spends multisig out - send tx to the pool (it's unconfirmed)
  // 2. Dan refreshes wallet and tries to do the same
  // 3. Make sure Dan can't do this
  // 4. Miner mines a PoW block confirming Alice's tx.
  // 5. Make sure Dan's wallet correctly handle it.
  // 6. Make sure Dan is not able to spend multisig out.

  tx = AUTO_VAL_INIT(tx);
  transfer_multisig(*alice_wlt.get(), std::list<account_keys>({ m_accounts[DAN_ACC_IDX].get_keys() }), ms3_hash, std::vector<tx_destination_entry>({ tx_destination_entry(amount3, receiver_acc.get_public_address()) }),
    0, TESTS_DEFAULT_FEE, std::vector<currency::extra_v>(), std::vector<currency::attachment_v>(), tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), tx, c.get_current_tx_version());

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");

  dan_wlt->scan_tx_pool(stub);

  caught = false;
  try
  {
    // last 'false' means don't send to network. This should fail during preparation, not during sending/processing
    transfer_multisig(*dan_wlt.get(), std::list<account_keys>({ m_accounts[ALICE_ACC_IDX].get_keys() }), ms3_hash, std::vector<tx_destination_entry>({ tx_destination_entry(amount3, receiver_acc.get_public_address()) }),
      0, TESTS_DEFAULT_FEE, std::vector<currency::extra_v>(), std::vector<currency::attachment_v>(), tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), tx, 0, 0, true, false);
  }
  catch (tools::error::wallet_internal_error&)
  {
    caught = true;
  }
  CHECK_AND_ASSERT_MES(caught, false, "Dan was able to make multisig tx for alreadly spent output");

  // Miner mines the next PoW block, confirming Alice's transaction.
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "There are txs in the pool.");

  // Refresh Dan's wallet. Unconfirmed multisig out should be correctly processed in the wallet during refresh.
  dan_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 3, false, "Incorrect numbers of blocks fetched");

  // This should correctly removed the transfer from wallet's unconfirmed list.
  dan_wlt->scan_tx_pool(stub);

  // Both Dan's ms transfers should be marked as 'spent' now.
  dan_mstc.clear();
  dan_wlt->get_multisig_transfers(dan_mstc);
  CHECK_AND_ASSERT_MES(dan_mstc.size() == 2 && dan_mstc.count(ms2_hash) == 1 && dan_mstc.count(ms3_hash) == 1 && dan_mstc[ms2_hash].is_spent() && dan_mstc[ms3_hash].is_spent(), false, "Dan has incorrect multisig transfers");

  // Re-try spending Carol-Dan multisig out on behalf of Dan. It should fail
  caught = false;
  try
  {
    transfer_multisig(*dan_wlt.get(), std::list<account_keys>({ m_accounts[ALICE_ACC_IDX].get_keys() }), ms3_hash, std::vector<tx_destination_entry>({ tx_destination_entry(amount3, receiver_acc.get_public_address()) }),
      0, TESTS_DEFAULT_FEE, std::vector<currency::extra_v>(), std::vector<currency::attachment_v>(), tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), tx, 0, 0, true, false);
  }
  catch (tools::error::wallet_internal_error&)
  {
    caught = true;
  }
  CHECK_AND_ASSERT_MES(caught, false, "Dan was able to make multisig tx for alreadly spent output");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "There are txs in the pool.");


  // Spending multisig 4: Carol & Bob

  // Reset Carol wallet and check it's correctness
  carol_mstc.clear();
  carol_wlt->reset_history();
  carol_wlt->get_multisig_transfers(carol_mstc);
  CHECK_AND_ASSERT_MES(carol_mstc.empty(), false, "Carol has multisig transfers");
  carol_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 4, false, "Incorrect numbers of blocks fetched");
  r = check_balance_via_wallet(*carol_wlt.get(), "carol_wlt", 0);
  CHECK_AND_ASSERT_MES(r, false, "Invalid wallet balance");
  carol_wlt->get_multisig_transfers(carol_mstc);
  CHECK_AND_ASSERT_MES(carol_mstc.size() == 2 && carol_mstc.count(ms2_hash) == 1 && carol_mstc.count(ms4_hash) == 1 && carol_mstc[ms2_hash].is_spent() && !carol_mstc[ms4_hash].is_spent(), false, "Carol has incorrect multisig transfers");


  return true;
}

//------------------------------------------------------------------------------

multisig_wallet_same_dst_addr::multisig_wallet_same_dst_addr()
{
  REGISTER_CALLBACK_METHOD(multisig_wallet_same_dst_addr, c1);
}

bool multisig_wallet_same_dst_addr::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);
  GENERATE_ACCOUNT(bob_acc);
  m_accounts.push_back(bob_acc);

  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, preminer_acc, test_core_time::get_time());
  events.push_back(blk_0);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  DO_CALLBACK(events, "c1");

  return true;
}

bool multisig_wallet_same_dst_addr::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // Test outline:
  // 1. Miner mines few blocks.
  // 2. Miner transfers 'amount' to Alice with multisig out (mentioning Alice's account _twice_).
  // 3. Alice refreshes her wallet and transfers to Bob the money received by multisig tx.
  // 4. Bob checks his wallet and balance.

  size_t blocks_fetched = 0;
  bool received_money = false, r = false;
  std::atomic<bool> atomic_false(false);
  static const uint64_t amount = TESTS_DEFAULT_FEE * 13;

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, m_accounts[MINER_ACC_IDX]);
  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW, false, "Incorrect number of blocks fetched.");

  tx_destination_entry de = AUTO_VAL_INIT(de);
  de.addr.push_back(m_accounts[ALICE_ACC_IDX].get_public_address());
  de.addr.push_back(m_accounts[ALICE_ACC_IDX].get_public_address()); // multisig to the same address
  de.amount = amount + TESTS_DEFAULT_FEE;
  de.minimum_sigs = de.addr.size();

  // transfer multisig to Alice
  transaction ms_tx = AUTO_VAL_INIT(ms_tx);
  miner_wlt->transfer(std::vector<tx_destination_entry>({ de }), 0, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment, ms_tx);

  // mine the next PoW and make sure everythig is allright
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect number of tx in the pool");
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "There are transactions in the pool");

  crypto::hash ms_hash = currency::get_multisig_out_id(ms_tx, get_tx_out_index_by_amount(ms_tx, de.amount));

  // refresh Alice wallet
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1, false, "Incorrect number of blocks fetched.");

  // transfers money, Alice has just received with multitsig tx, to Bob
  transaction tx = AUTO_VAL_INIT(tx);
  transfer_multisig(*alice_wlt.get(), std::list<account_keys>({ m_accounts[ALICE_ACC_IDX].get_keys() }), ms_hash,
    std::vector<tx_destination_entry>({ tx_destination_entry(amount, m_accounts[BOB_ACC_IDX].get_public_address()) }),
    0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment, tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), tx, c.get_current_tx_version());

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect number of tx in the pool");
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "There are transactions in the pool");

  // make sure Bob gets them
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, m_accounts[BOB_ACC_IDX]);
  bob_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2, false, "Incorrect number of blocks fetched.");

  r = check_balance_via_wallet(*bob_wlt.get(), "bob_wlt", amount);
  CHECK_AND_ASSERT_MES(r, false, "check_balance_via_wallet failed");

  return true;
}

//------------------------------------------------------------------------------

multisig_wallet_ms_to_ms::multisig_wallet_ms_to_ms()
{
  REGISTER_CALLBACK_METHOD(multisig_wallet_ms_to_ms, c1);
}

bool multisig_wallet_ms_to_ms::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);
  GENERATE_ACCOUNT(bob_acc);
  m_accounts.push_back(bob_acc);

  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, preminer_acc, test_core_time::get_time());
  events.push_back(blk_0);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  DO_CALLBACK(events, "c1");

  return true;
}

bool multisig_wallet_ms_to_ms::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // Test outline:
  // 1. Miner mines few blocks and sends multisig(Alice, Bob) with 'amount' of coins.
  // 2. Alice receives this multisig tx and spends it by sending the next multisig(Miner, Bob). This makes a tx, having one ms input and one ms output.
  // 3. Miner receives that multisig tx and spends it by sending all the money to Bob.
  // 4. Make sure Bob gets the money.

  size_t blocks_fetched = 0;
  bool received_money = false, r = false;
  std::atomic<bool> atomic_false(false);
  static const uint64_t amount = TESTS_DEFAULT_FEE * 13;

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, m_accounts[MINER_ACC_IDX]);
  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW, false, "Incorrect number of blocks fetched.");

  tx_destination_entry de = AUTO_VAL_INIT(de);
  de.addr.push_back(m_accounts[ALICE_ACC_IDX].get_public_address());
  de.addr.push_back(m_accounts[BOB_ACC_IDX].get_public_address());
  de.amount = amount + TESTS_DEFAULT_FEE;
  de.minimum_sigs = de.addr.size();

  // transfer multisig to Alice + Bob
  transaction ms_tx = AUTO_VAL_INIT(ms_tx);
  miner_wlt->transfer(std::vector<tx_destination_entry>({ de }), 0, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment, ms_tx);

  // mine the next PoW and make sure everythig is allright
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect number of tx in the pool");
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "There are transactions in the pool");

  crypto::hash ms_hash = currency::get_multisig_out_id(ms_tx, get_tx_out_index_by_amount(ms_tx, de.amount));

  // refresh Alice wallet
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1, false, "Incorrect number of blocks fetched.");

  // multisig(Alice, Bob) to multisig(Miner, Bob) transfer
  de = AUTO_VAL_INIT(de);
  de.addr.push_back(m_accounts[MINER_ACC_IDX].get_public_address());
  de.addr.push_back(m_accounts[BOB_ACC_IDX].get_public_address());
  de.amount = amount;
  de.minimum_sigs = de.addr.size();
  transaction ms_tx2 = AUTO_VAL_INIT(ms_tx2);
  transfer_multisig(*alice_wlt.get(), std::list<account_keys>({ m_accounts[BOB_ACC_IDX].get_keys() }), ms_hash,
    std::vector<tx_destination_entry>({ de }),
    0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment, tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), ms_tx2, c.get_current_tx_version());

  // check the pool and mine a block
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect number of tx in the pool");
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "There are transactions in the pool");

  crypto::hash ms_hash2 = currency::get_multisig_out_id(ms_tx2, get_tx_out_index_by_amount(ms_tx2, de.amount));

  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 2, false, "Incorrect number of blocks fetched.");

  // spend the last multisig(Miner, Bob) and transfer to Bob
  transaction tx = AUTO_VAL_INIT(tx);
  transfer_multisig(*miner_wlt.get(), std::list<account_keys>({ m_accounts[BOB_ACC_IDX].get_keys() }), ms_hash2,
    std::vector<tx_destination_entry>({ tx_destination_entry(amount - TESTS_DEFAULT_FEE, m_accounts[BOB_ACC_IDX].get_public_address()) }),
    0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment, tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), tx, c.get_current_tx_version());

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect number of tx in the pool");
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "There are transactions in the pool");

  // make sure Bob gets coins
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, m_accounts[BOB_ACC_IDX]);
  bob_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3, false, "Incorrect number of blocks fetched.");

  r = check_balance_via_wallet(*bob_wlt.get(), "bob_wlt", amount - TESTS_DEFAULT_FEE);
  CHECK_AND_ASSERT_MES(r, false, "check_balance_via_wallet failed");

  return true;
}

//------------------------------------------------------------------------------

multisig_minimum_sigs::multisig_minimum_sigs()
{
}

bool multisig_minimum_sigs::generate(std::vector<test_event_entry>& events) const
{
  // Checks minimum_sigs feature. Doesn't use wallet2. Gentime only.
  // Test outline:
  // Case 1. Create multisig for three participants (Miner, Alice, Bob) and specify minimum_sigs == 4 (should fail on construction)
  // Case 2. Create multisig for three participants (Miner, Alice, Bob) and specify minimum_sigs == 2
  // 2.1. Then try to spend it using only 1 key (should fail)
  // 2.2. Spend using 2 keys (Alice, Bob) by sending money to Bob
  // Case 3. Try to spend already spent multisig output
  // Case 4. Use on spending multisig output more keys than required
  // Case 5. Spending multisig output using more keys than required and, moreover, add some fake keys
  // Case 6. Spending multisig using too many redundant keys (which is computationally expensive)
  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);
  GENERATE_ACCOUNT(bob_acc);
  m_accounts.push_back(bob_acc);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  bool r = false;
  uint64_t amount = TESTS_DEFAULT_FEE * 17;

  std::list<account_public_address> ms_addr_list({ miner_acc.get_public_address(), alice_acc.get_public_address(), bob_acc.get_public_address() });

  // Case 1. Create multisig for three participants (Miner, Alice, Bob) and specify minimum_sigs == 4 (should fail on construction)
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc.get_keys(), ms_addr_list, amount, TESTS_DEFAULT_FEE, 0, sources, destinations, true, true, 4);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  transaction tx = AUTO_VAL_INIT(tx);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r == false, false, "construct_tx was expected to fail, but successed");


  // Case 2. Create multisig for three participants (Miner, Alice, Bob) and specify minimum_sigs == 2
  // 2.1. Then try to spend it using only 1 key (should fail)
  // 2.2. Spend using 2 keys (Alice, Bob) by sending money to Bob
  sources.clear();
  destinations.clear();
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc.get_keys(), ms_addr_list, amount, TESTS_DEFAULT_FEE, 0, sources, destinations, true, true, 2);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");

  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_1, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_1);

  // store multisig output index and id for further reference
  size_t ms_out_idx = get_tx_out_index_by_amount(tx_1, amount);
  crypto::hash ms_id = get_multisig_out_id(tx_1, ms_out_idx);

  // Put tx_1 into a block
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  // Subcase 2.1 Try to spend multisig out using only one key
  tx_source_entry se = AUTO_VAL_INIT(se);
  se.amount = amount;
  se.multisig_id = ms_id;
  se.real_output_in_tx_index = ms_out_idx;
  se.real_out_tx_key = get_tx_pub_key_from_extra(tx_1);
  se.ms_keys_count = boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(tx_1.vout[se.real_output_in_tx_index]).target).keys.size();
  se.ms_sigs_count = 2;

  tx_destination_entry de(se.amount - TESTS_DEFAULT_FEE, bob_acc.get_public_address());

  // Transaction should be successfully created, but rejected by the core
  transaction tx_2 = AUTO_VAL_INIT(tx_2);
  r = construct_tx(miner_acc.get_keys(), std::vector<tx_source_entry>({ se }), std::vector<tx_destination_entry>({ de }), empty_attachment, tx_2, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  bool tx_fully_signed = false;
  r = sign_multisig_input_in_tx(tx_2, 0, bob_acc.get_keys(), tx_1, &tx_fully_signed);
  CHECK_AND_ASSERT_MES(r && !tx_fully_signed, false, "sign_multisig_input_in_tx failed, tx_fully_signed : " << tx_fully_signed);
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_2);

  // Subcase 2.2 Add second participant and send. Should be okay.
  r = sign_multisig_input_in_tx(tx_2, 0, alice_acc.get_keys(), tx_1, &tx_fully_signed);
  CHECK_AND_ASSERT_MES(r && tx_fully_signed, false, "sign_multisig_input_in_tx failed, tx_fully_signed : " << tx_fully_signed); // should be fully signed now
  events.push_back(tx_2);

  // Put the tx into a block
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, tx_2);

  // Make sure Bob's balance is correct
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(BOB_ACC_IDX, de.amount));


  // Case 3. Try to spend already spent multisig output
  se = AUTO_VAL_INIT(se);
  se.amount = amount;
  se.multisig_id = ms_id;
  se.real_output_in_tx_index = ms_out_idx;
  se.real_out_tx_key = get_tx_pub_key_from_extra(tx_1);
  se.ms_keys_count = boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(tx_1.vout[se.real_output_in_tx_index]).target).keys.size();
  se.ms_sigs_count = 2;

  transaction tx_3 = AUTO_VAL_INIT(tx_3);
  r = construct_tx(miner_acc.get_keys(), std::vector<tx_source_entry>({ se }), std::vector<tx_destination_entry>({ tx_destination_entry(se.amount - TESTS_DEFAULT_FEE, bob_acc.get_public_address()) }), empty_attachment, tx_3, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  r = sign_multisig_input_in_tx(tx_3, 0, bob_acc.get_keys(), tx_1, &tx_fully_signed);
  CHECK_AND_ASSERT_MES(r && !tx_fully_signed, false, "sign_multisig_input_in_tx failed, tx_fully_signed : " << tx_fully_signed);
  r = sign_multisig_input_in_tx(tx_3, 0, alice_acc.get_keys(), tx_1, &tx_fully_signed);
  CHECK_AND_ASSERT_MES(r && tx_fully_signed, false, "sign_multisig_input_in_tx failed, tx_fully_signed : " << tx_fully_signed); // should be fully signed now

  DO_CALLBACK(events, "mark_invalid_tx"); // already spent
  events.push_back(tx_3);


  // Case 4. Use on spending multisig output more keys than required
  // create source tx: tx_4
  sources.clear();
  destinations.clear();
  r = fill_tx_sources_and_destinations(events, blk_2, miner_acc.get_keys(), ms_addr_list, amount, TESTS_DEFAULT_FEE, 0, sources, destinations, true, true, 1);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");

  transaction tx_4 = AUTO_VAL_INIT(tx_4);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_4, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_4);

  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_4);

  size_t ms_4_out_idx = get_tx_out_index_by_amount(tx_4, amount);
  crypto::hash ms_4_id = get_multisig_out_id(tx_4, ms_4_out_idx);

  se = AUTO_VAL_INIT(se);
  se.amount = amount;
  se.multisig_id = ms_4_id;
  se.real_output_in_tx_index = ms_4_out_idx;
  se.real_out_tx_key = get_tx_pub_key_from_extra(tx_4);
  se.ms_keys_count = boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(tx_4.vout[se.real_output_in_tx_index]).target).keys.size();
  se.ms_sigs_count = 3;

  transaction tx_5 = AUTO_VAL_INIT(tx_5);
  r = construct_tx(miner_acc.get_keys(), std::vector<tx_source_entry>({ se }), std::vector<tx_destination_entry>({ tx_destination_entry(se.amount - TESTS_DEFAULT_FEE, alice_acc.get_public_address()) }), empty_attachment, tx_5, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

  // use sign_multisig_input_in_tx_custom to create tx with more signatures (3) than minimum_sigs (1)
  r = sign_multisig_input_in_tx_custom(tx_5, ms_4_out_idx, bob_acc.get_keys(), tx_4, &tx_fully_signed, false);
  CHECK_AND_ASSERT_MES(r && !tx_fully_signed, false, "sign_multisig_input_in_tx failed, tx_fully_signed : " << tx_fully_signed);
  r = sign_multisig_input_in_tx_custom(tx_5, ms_4_out_idx, alice_acc.get_keys(), tx_4, &tx_fully_signed, false);
  CHECK_AND_ASSERT_MES(r && !tx_fully_signed, false, "sign_multisig_input_in_tx failed, tx_fully_signed : " << tx_fully_signed);
  r = sign_multisig_input_in_tx_custom(tx_5, ms_4_out_idx, miner_acc.get_keys(), tx_4, &tx_fully_signed, true);
  CHECK_AND_ASSERT_MES(r && tx_fully_signed, false, "sign_multisig_input_in_tx failed, tx_fully_signed : " << tx_fully_signed); // should be fully signed now

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_5);

  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_acc);
  //DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, amount - TX_POOL_MINIMUM_FEE));

  
  // Case 5. Spending multisig output using more keys than required and, moreover, add some fake keys
  sources.clear();
  destinations.clear();
  r = fill_tx_sources_and_destinations(events, blk_4, miner_acc.get_keys(), ms_addr_list, amount, TESTS_DEFAULT_FEE, 0, sources, destinations, true, true, 1);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");

  transaction tx_6 = AUTO_VAL_INIT(tx_6);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_6, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_6);

  MAKE_NEXT_BLOCK_TX1(events, blk_5, blk_4, miner_acc, tx_6);

  // store multisig output index and id for further reference
  size_t ms_6_out_idx = get_tx_out_index_by_amount(tx_6, amount);
  crypto::hash ms_6_id = get_multisig_out_id(tx_6, ms_6_out_idx);

  se = AUTO_VAL_INIT(se);
  se.amount = amount;
  se.multisig_id = ms_6_id;
  se.real_output_in_tx_index = ms_6_out_idx;
  se.real_out_tx_key = get_tx_pub_key_from_extra(tx_6);
  se.ms_keys_count = boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(tx_6.vout[se.real_output_in_tx_index]).target).keys.size();
  se.ms_sigs_count = 4;

  transaction tx_7 = AUTO_VAL_INIT(tx_7);
  r = construct_tx(miner_acc.get_keys(), std::vector<tx_source_entry>({ se }), std::vector<tx_destination_entry>({ tx_destination_entry(se.amount - TESTS_DEFAULT_FEE, alice_acc.get_public_address()) }), empty_attachment, tx_7, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

  // use sign_multisig_input_in_tx_custom to create tx with more signatures (4) than minimum_sigs (1)
  r = sign_multisig_input_in_tx_custom(tx_7, ms_6_out_idx, bob_acc.get_keys(), tx_6, &tx_fully_signed, false);
  CHECK_AND_ASSERT_MES(r && !tx_fully_signed, false, "sign_multisig_input_in_tx failed, tx_fully_signed : " << tx_fully_signed);
  r = sign_multisig_input_in_tx_custom(tx_7, ms_6_out_idx, alice_acc.get_keys(), tx_6, &tx_fully_signed, false);
  CHECK_AND_ASSERT_MES(r && !tx_fully_signed, false, "sign_multisig_input_in_tx failed, tx_fully_signed : " << tx_fully_signed);
  r = sign_multisig_input_in_tx_custom(tx_7, ms_6_out_idx, miner_acc.get_keys(), tx_6, &tx_fully_signed, false);
  CHECK_AND_ASSERT_MES(r && !tx_fully_signed, false, "sign_multisig_input_in_tx failed, tx_fully_signed : " << tx_fully_signed);
  boost::get<currency::NLSAG_sig>(tx_7.signatures[0]).s.push_back(invalid_signature);   // instead of 4th sig just add invalid sig

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_7);


  // Case 6. Spending multisig using too many redundant keys
  sources.clear();
  destinations.clear();
  r = fill_tx_sources_and_destinations(events, blk_5, miner_acc.get_keys(), ms_addr_list, amount, TESTS_DEFAULT_FEE, 0, sources, destinations, true, true, 1);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");

  transaction tx_8 = AUTO_VAL_INIT(tx_8);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_8, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_8);

  MAKE_NEXT_BLOCK_TX1(events, blk_6, blk_5, miner_acc, tx_8);

  // store multisig output index and id for further reference
  size_t ms_8_out_idx = get_tx_out_index_by_amount(tx_8, amount);
  crypto::hash ms_8_id = get_multisig_out_id(tx_8, ms_8_out_idx);

  se = AUTO_VAL_INIT(se);
  se.amount = amount;
  se.multisig_id = ms_8_id;
  se.real_output_in_tx_index = ms_8_out_idx;
  se.real_out_tx_key = get_tx_pub_key_from_extra(tx_8);
  se.ms_keys_count = boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(tx_8.vout[se.real_output_in_tx_index]).target).keys.size();
  static const size_t redundant_keys_count = 7000;
  se.ms_sigs_count = redundant_keys_count;

  transaction tx_9 = AUTO_VAL_INIT(tx_9);
  r = construct_tx(miner_acc.get_keys(), std::vector<tx_source_entry>({ se }), std::vector<tx_destination_entry>({ tx_destination_entry(se.amount - TESTS_DEFAULT_FEE, alice_acc.get_public_address()) }), empty_attachment, tx_9, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

  boost::get<currency::NLSAG_sig>(tx_9.signatures[0]).s.resize(redundant_keys_count, invalid_signature);

  r = sign_multisig_input_in_tx_custom(tx_9, ms_8_out_idx, bob_acc.get_keys(), tx_8, &tx_fully_signed, false);
  CHECK_AND_ASSERT_MES(r && !tx_fully_signed, false, "sign_multisig_input_in_tx failed, tx_fully_signed : " << tx_fully_signed);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_9);

  //MAKE_NEXT_BLOCK_TX1(events, blk_7, blk_6, miner_acc, tx_9);
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, 0));
  return true;
}

//------------------------------------------------------------------------------

bool multisig_and_fake_outputs::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: multisig outs are incompartible with faked mix-ins.
  // Here we try to mix multisig and faked inputs/outputs in one transaction to see if smth goes wrong.

  // Setup accounts
  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc); LOCAL_ASSERT(m_accounts.size() - 1 == MINER_ACC_IDX);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc); LOCAL_ASSERT(m_accounts.size() - 1 == ALICE_ACC_IDX);

  // Mine starter blocks
  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  // Calculate amount that many outs already have to simplify debugging (such amount won't be splitted into digits)
  bool r = false;
  uint64_t amount = 0, stub;
  r = calculate_amounts_many_outs_have_and_no_outs_have(get_outs_money_amount(blk_0r.miner_tx), amount, stub);
  CHECK_AND_ASSERT_MES(r, false, "calculate_amounts_many_outs_have_and_no_outs_have failed");

  // Use two account as multisig target
  std::list<account_public_address> ms_addr_list({ miner_acc.get_public_address(), alice_acc.get_public_address() });

  // tx_1: 1 real + 2 fake inputs => 1 multisig output (Miner + Alice, minimum_sigs = 1)
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc.get_keys(), ms_addr_list, amount, TESTS_DEFAULT_FEE, 2, sources, destinations, true, true, 1);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");

  // Make sure tx_1 is successfully created
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_1, 0, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_1);

  // Add it to blockchain
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  size_t tx_1_ms_out_idx = get_tx_out_index_by_amount(tx_1, amount);
  crypto::hash tx_1_ms_out_id = get_multisig_out_id(tx_1, tx_1_ms_out_idx);

  // Try to create tx_source_entry with a mix of multisig and to_key outputs
  size_t nmix = 2; // mix-in 1 fake output
  sources.clear();
  // First, prepare as normal source entry with some fake inputs
  r = fill_tx_sources(sources, events, blk_1, miner_acc.get_keys(), amount, nmix, true, true);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
  CHECK_AND_ASSERT_MES(sources.size() == 1 && sources[0].amount == amount && sources[0].outputs.size() == nmix + 1, false, "fill_tx_sources returned unexpected results");
  // Second, correctly set up multisig part
  tx_source_entry& se = sources.back();
  se.multisig_id = tx_1_ms_out_id;
  se.ms_keys_count = boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(tx_1.vout[tx_1_ms_out_idx]).target).keys.size();
  se.ms_sigs_count = 1;
  se.real_output_in_tx_index = tx_1_ms_out_idx;
  se.real_out_tx_key = get_tx_pub_key_from_extra(tx_1);

  destinations.clear();
  destinations.push_back(tx_destination_entry(amount - TESTS_DEFAULT_FEE, alice_acc.get_public_address()));

  // Make sure tx is successfully created
  transaction tx_2 = AUTO_VAL_INIT(tx_2);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_2, 0, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  bool tx_fully_signed = false;
  r = sign_multisig_input_in_tx(tx_2, 0, alice_acc.get_keys(), tx_1, &tx_fully_signed);
  CHECK_AND_ASSERT_MES(r && tx_fully_signed, false, "sign_multisig_input_in_tx failed, sign_multisig_input_in_tx: " << r);
  events.push_back(tx_2);


  // ...and put into a block
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, tx_2);

  // Finally, check the balance of Alice's wallet
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, amount - TESTS_DEFAULT_FEE));

  return true;
}

//------------------------------------------------------------------------------

bool multisig_and_unlock_time::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: check how multisig-containing tx with unlock_time and expiration_time are processed

  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc); LOCAL_ASSERT(m_accounts.size() - 1 == MINER_ACC_IDX);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc); LOCAL_ASSERT(m_accounts.size() - 1 == ALICE_ACC_IDX);
  GENERATE_ACCOUNT(bob_acc);
  m_accounts.push_back(bob_acc);   LOCAL_ASSERT(m_accounts.size() - 1 == BOB_ACC_IDX);

  // Mine starter blocks
  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);
  test_core_time::adjust(blk_0r.timestamp); // adjust gentime time to blockchain timestamps

  bool r = false;
  uint64_t amount = TESTS_DEFAULT_FEE * 9;
  std::list<account_public_address> ms_addr_list({ miner_acc.get_public_address(), alice_acc.get_public_address() });

  // noramal input -> multisig output with unlock time
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc.get_keys(), ms_addr_list, amount, TESTS_DEFAULT_FEE, 1 /*nmix*/, sources, destinations, true, true, 1 /* minimum sigs */);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");

  uint64_t unlock_time = blk_0r.timestamp + DIFFICULTY_TOTAL_TARGET * 3 + CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS;
  uint64_t unlock_time_2 = blk_0r.timestamp + DIFFICULTY_TOTAL_TARGET * 6 + CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS;

  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_1, get_tx_version_from_events(events), unlock_time, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  CHECK_AND_ASSERT_MES(get_tx_max_unlock_time(tx_1) == unlock_time, false, "Unlock time was not correctly set");
  events.push_back(tx_1);

  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);
  CHECK_AND_ASSERT_MES(blk_1.timestamp < unlock_time, false, "Block timestamp went far beyond expected limits, consider adjusting 'unlock_time'");
  test_core_time::adjust(blk_1.timestamp); // adjust gentime time to blockchain timestamps
  events.push_back(event_core_time(blk_1.timestamp)); // adjust playtime time to blockchain timestamps

  // Then try to spend multisig out of tx_1 earlier than it's unlock_time
  size_t tx_1_ms_out_idx = get_tx_out_index_by_amount(tx_1, amount);
  crypto::hash tx_1_ms_out_id = get_multisig_out_id(tx_1, tx_1_ms_out_idx);

  tx_source_entry se = AUTO_VAL_INIT(se);
  se.amount = amount;
  se.multisig_id = tx_1_ms_out_id;
  se.real_output_in_tx_index = tx_1_ms_out_idx;
  se.real_out_tx_key = get_tx_pub_key_from_extra(tx_1);
  se.ms_keys_count = boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(tx_1.vout[tx_1_ms_out_idx]).target).keys.size();
  se.ms_sigs_count = 1;
  sources.assign({ se });

  destinations.assign({ tx_destination_entry(amount - TESTS_DEFAULT_FEE, alice_acc.get_public_address()) });

  ADJUST_TEST_CORE_TIME(unlock_time - CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS - 1);

  // tx_2 should be created ok, but rejected by the core, as one of input refers to a locked tx
  // Note: tx_2 has unlock_time_2 specified
  transaction tx_2 = AUTO_VAL_INIT(tx_2);
  r = construct_tx(alice_acc.get_keys(), sources, destinations, empty_attachment, tx_2, get_tx_version_from_events(events), unlock_time_2, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

  bool tx_fully_signed = false;
  r = sign_multisig_input_in_tx(tx_2, tx_1_ms_out_idx, miner_acc.get_keys(), tx_1, &tx_fully_signed);
  CHECK_AND_ASSERT_MES(r && tx_fully_signed, false, "sign_multisig_input_in_tx failed, tx_fully_signed: " << tx_fully_signed);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_2);

  // rewind the time a little, now tx_2 should be accepted
  ADJUST_TEST_CORE_TIME(unlock_time - CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS + 1);
  events.push_back(tx_2);

  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, tx_2);

  // Next, check tx_2 unlock time by spending all coins
  ADJUST_TEST_CORE_TIME(unlock_time_2 - CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS - 1);

  DO_CALLBACK(events, "mark_invalid_tx");
  // instead of MAKE_TX use manual construction to set check_for_unlocktime = false, old: MAKE_TX(events, tx_3, alice_acc, bob_acc, amount - TESTS_DEFAULT_FEE * 2, blk_2);
  r = fill_tx_sources_and_destinations(events, blk_2, alice_acc, bob_acc, amount - TESTS_DEFAULT_FEE * 2, TESTS_DEFAULT_FEE, 0 /*nmix*/, sources, destinations, true, false /* check_for_unlocktime */);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  transaction tx_3{};
  r = construct_tx(alice_acc.get_keys(), sources, destinations, empty_attachment, tx_3, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  ADD_CUSTOM_EVENT(events, tx_3);


  ADJUST_TEST_CORE_TIME(unlock_time_2 - CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS + 1);
  ADD_CUSTOM_EVENT(events, tx_3);
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_3);

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(BOB_ACC_IDX, amount - TESTS_DEFAULT_FEE * 2));
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, 0));

  // Expiration time

  // 1. txin_to_key -> multisig output with expiration time
  uint64_t expiration_time = generator.get_timestamps_median(get_block_hash(blk_3), TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW) - DIFFICULTY_TOTAL_TARGET * 3;
  sources.clear();
  destinations.clear();
  r = fill_tx_sources_and_destinations(events, blk_3, miner_acc.get_keys(), ms_addr_list, amount, TESTS_DEFAULT_FEE, 1, sources, destinations, true, true, 1);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");

  transaction tx_4 = AUTO_VAL_INIT(tx_4);
  crypto::secret_key stub_key = AUTO_VAL_INIT(stub_key);
  etc_tx_details_expiration_time extra_expiration_time = AUTO_VAL_INIT(extra_expiration_time);
  extra_expiration_time.v = expiration_time;
  r = construct_tx(miner_acc.get_keys(), sources, destinations, std::vector<extra_v>({ extra_expiration_time }), empty_attachment, tx_4, get_tx_version_from_events(events), stub_key, 0, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  CHECK_AND_ASSERT_MES(get_tx_expiration_time(tx_4) == expiration_time, false, "Expiration time was not correctly set");
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_4);

  // add similar tx (same sources and destinations) with no expiration_time - should be accepted by the core
  transaction tx_5 = AUTO_VAL_INIT(tx_5);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_extra, empty_attachment, tx_5, get_tx_version_from_events(events), stub_key, 0, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_5);

  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_3, miner_acc, tx_5);

  ADJUST_TEST_CORE_TIME(blk_4.timestamp);

  // 2. Multisig output -> txout_to_key with expiration time
  size_t tx_5_ms_out_idx = get_tx_out_index_by_amount(tx_5, amount);
  crypto::hash tx_5_ms_out_id = get_multisig_out_id(tx_5, tx_5_ms_out_idx);

  se = AUTO_VAL_INIT(se);
  se.amount = amount;
  se.multisig_id = tx_5_ms_out_id;
  se.real_output_in_tx_index = tx_5_ms_out_idx;
  se.real_out_tx_key = get_tx_pub_key_from_extra(tx_5);
  se.ms_keys_count = boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(tx_5.vout[tx_5_ms_out_idx]).target).keys.size();
  se.ms_sigs_count = 1;
  sources.assign({ se });

  destinations.assign({ tx_destination_entry(amount - TESTS_DEFAULT_FEE, alice_acc.get_public_address()) });

  expiration_time = generator.get_timestamps_median(get_block_hash(blk_4), TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW) - DIFFICULTY_TOTAL_TARGET * 3;
  extra_expiration_time = AUTO_VAL_INIT(extra_expiration_time);
  extra_expiration_time.v = expiration_time;
  transaction tx_6 = AUTO_VAL_INIT(tx_6);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, std::vector<extra_v>({ extra_expiration_time }), empty_attachment, tx_6, get_tx_version_from_events(events), stub_key, 0, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  CHECK_AND_ASSERT_MES(get_tx_expiration_time(tx_6) == expiration_time, false, "Expiration time was not correctly set");
  r = sign_multisig_input_in_tx(tx_6, tx_5_ms_out_idx, miner_acc.get_keys(), tx_5, &tx_fully_signed);
  CHECK_AND_ASSERT_MES(r && tx_fully_signed, false, "sign_multisig_input_in_tx failed, tx_fully_signed: " << tx_fully_signed);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_6);

  return true;
}

//------------------------------------------------------------------------------

bool multisig_and_coinbase::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: creat a block with multisig output in its miner tx, then spend it to make sure everything is ok.

  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc); LOCAL_ASSERT(m_accounts.size() - 1 == MINER_ACC_IDX);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc); LOCAL_ASSERT(m_accounts.size() - 1 == ALICE_ACC_IDX);
  GENERATE_ACCOUNT(bob_acc);
  m_accounts.push_back(bob_acc);   LOCAL_ASSERT(m_accounts.size() - 1 == BOB_ACC_IDX);

  bool r = false;

  // Mine starter blocks
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core"); // to lower PoS minimum height
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  //
  // Part 1. PoS block
  //
  const block& prev_block = blk_0r;
  crypto::hash prev_id = get_block_hash(prev_block);
  size_t height = get_block_height(prev_block) + 1;
  currency::wide_difficulty_type diff = generator.get_difficulty_for_next_block(prev_id, false);

  block blk_1 = AUTO_VAL_INIT(blk_1);
  {
    const transaction& stake = blk_0.miner_tx;
    crypto::public_key stake_tx_pub_key = get_tx_pub_key_from_extra(stake);
    size_t stake_output_idx = 0;
    size_t stake_output_gidx = 0;
    uint64_t stake_output_amount =boost::get<currency::tx_out_bare>( stake.vout[stake_output_idx]).amount;
    crypto::key_image stake_output_key_image;
    keypair kp;
    generate_key_image_helper(miner_acc.get_keys(), stake_tx_pub_key, stake_output_idx, kp, stake_output_key_image);
    crypto::public_key stake_output_pubkey = boost::get<txout_to_key>(boost::get<currency::tx_out_bare>(stake.vout[stake_output_idx]).target).key;
    keypair tx_key = keypair::generate();

    pos_block_builder pb;
    pb.step1_init_header(generator.get_hardforks(), height, prev_id);
    pb.step2_set_txs(std::vector<transaction>());
    pb.step3_build_stake_kernel(stake_output_amount, stake_output_gidx, stake_output_key_image, diff, prev_id, null_hash, prev_block.timestamp);
    pb.step4_generate_coinbase_tx(generator.get_timestamps_median(prev_id), generator.get_already_generated_coins(prev_block), miner_acc.get_public_address(),
      blobdata(), CURRENCY_MINER_TX_MAX_OUTS, &tx_key);

    // The builder creates PoS miner tx with normal outputs.
    // Replace all miner_tx outputs with one multisig output and re-sign it.
    {
      std::vector<account_public_address> participants({ alice_acc.get_public_address(), bob_acc.get_public_address() });
      transaction& miner_tx = pb.m_block.miner_tx;
      txout_multisig ms_out_target = AUTO_VAL_INIT(ms_out_target);
      ms_out_target.minimum_sigs = 1;
      size_t multisig_out_idx = 0; // because it's the only output in vout
      for (auto& p : participants)
      {
        crypto::key_derivation der = AUTO_VAL_INIT(der);
        r = crypto::generate_key_derivation(p.view_public_key, tx_key.sec, der);
        CHECK_AND_ASSERT_MES(r, false, "generate_key_derivation failed");
        crypto::public_key key = AUTO_VAL_INIT(key);
        r = crypto::derive_public_key(der, multisig_out_idx, p.spend_public_key, key);
        CHECK_AND_ASSERT_MES(r, false, "derive_public_key failed");
        ms_out_target.keys.push_back(key);
      }
      tx_out_bare ms_out = AUTO_VAL_INIT(ms_out);
      ms_out.amount = get_outs_money_amount(miner_tx); // get amount from vout before clearing
      miner_tx.vout.clear();
      ms_out.target = ms_out_target;
      miner_tx.vout.push_back(ms_out);
    }

    pb.step5_sign(stake_tx_pub_key, stake_output_idx, stake_output_pubkey, miner_acc);
    // add block info to generator only to be able to make the next block in gentime
    generator.add_block_info(test_generator::block_info(pb.m_block, generator.get_already_generated_coins(prev_block) + get_outs_money_amount(pb.m_block.miner_tx) - stake_output_amount,
      pb.m_txs_total_size + get_object_blobsize(pb.m_block), generator.get_cumul_difficulty_for_next_block(prev_id), std::list<transaction>(), null_hash));
    blk_1 = pb.m_block;
  }

  events.push_back(blk_1);

  // In order to spend coinbase tx we must wait for CURRENCY_MINED_MONEY_UNLOCK_WINDOW blocks being added to the blockchain
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // spend multisig output to make sure everything is okay
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  tx_source_entry se = AUTO_VAL_INIT(se);
  se.amount = get_outs_money_amount(blk_1.miner_tx);
  se.real_output_in_tx_index = 0;
  se.multisig_id = get_multisig_out_id(blk_1.miner_tx, se.real_output_in_tx_index);
  //se.participants.push_back(alice_acc.get_keys());
  se.real_out_tx_key = get_tx_pub_key_from_extra(blk_1.miner_tx);
  se.ms_keys_count = boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(blk_1.miner_tx.vout[se.real_output_in_tx_index]).target).keys.size();
  se.ms_sigs_count = 1;
  sources.assign({ se });

  destinations.assign({ tx_destination_entry(se.amount - TESTS_DEFAULT_FEE, bob_acc.get_public_address()) });

  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(alice_acc.get_keys(), sources, destinations, empty_attachment, tx_1, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  bool fully_signed_tx = false;
  r = sign_multisig_input_in_tx(tx_1, 0, alice_acc.get_keys(), blk_1.miner_tx, &fully_signed_tx);
  CHECK_AND_ASSERT_MES(r && fully_signed_tx, false, "sign_multisig_input_in_tx failed, fully_signed_tx: " << fully_signed_tx);
  events.push_back(tx_1);

  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_1);

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, 0));
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(BOB_ACC_IDX, se.amount - TESTS_DEFAULT_FEE));

  //
  // Part 2. The same for PoW
  //

  uint64_t blk_2_reward = generator.get_base_reward_for_next_block(get_block_hash(blk_1), true);
  block blk_3 = AUTO_VAL_INIT(blk_3);
  {
    block& b = blk_3;
    const block& prev_block = blk_2;
    size_t height = get_block_height(prev_block) + 1;

    // manually create a transaction with multisig out
    std::list<account_public_address> ms_addr_list({ miner_acc.get_public_address(), alice_acc.get_public_address() });
    std::vector<tx_source_entry> sources;
    std::vector<tx_destination_entry> destinations;
    r = fill_tx_sources_and_destinations(events, prev_block, miner_acc.get_keys(), ms_addr_list, blk_2_reward, TESTS_DEFAULT_FEE, 0, sources, destinations, false, false, 1);
    CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
    transaction miner_tx = AUTO_VAL_INIT(miner_tx);
    r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, miner_tx, get_tx_version_from_events(events), height + CURRENCY_MINED_MONEY_UNLOCK_WINDOW, CURRENCY_TO_KEY_OUT_RELAXED, true);
    CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

    // replace vin with coinbase input
    txin_gen in_gen = AUTO_VAL_INIT(in_gen);
    in_gen.height = height;
    miner_tx.vin.assign({ in_gen });

    // remove all outputs except the multisig
    auto it = std::find_if(miner_tx.vout.begin(), miner_tx.vout.end(), [](const tx_out_v& o) {return boost::get<tx_out_bare>(o).target.type() == typeid(txout_multisig); });
    CHECK_AND_ASSERT_MES(it != miner_tx.vout.end(), false, "construct_tx didn't create multisig output as expected");
    tx_out_bare ms_out = boost::get<tx_out_bare>(*it);
    miner_tx.vout.assign({ ms_out });
    CHECK_AND_ASSERT_MES(ms_out.amount == blk_2_reward, false, "unexpected amount for found ms output");

    r = generator.construct_block_manually(b, prev_block, miner_acc, test_generator::bf_miner_tx, 0, 0, 0, null_hash, 1, miner_tx);
    CHECK_AND_ASSERT_MES(r, false, "construct_block_manually failed");
  }
  events.push_back(blk_3);

  // rewind blocks to be able to spend mined money
  REWIND_BLOCKS_N(events, blk_3r, blk_3, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // spend coinbase in blk_3 to make sure everything is okay
  se = AUTO_VAL_INIT(se);
  se.amount = blk_2_reward;
  se.multisig_id = get_multisig_out_id(blk_3.miner_tx, 0);
  se.real_output_in_tx_index = 0;
  se.real_out_tx_key = get_tx_pub_key_from_extra(blk_3.miner_tx);
  se.ms_keys_count= boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(blk_3.miner_tx.vout[se.real_output_in_tx_index]).target).keys.size();
  se.ms_sigs_count = 1;

  transaction tx_2 = AUTO_VAL_INIT(tx_2);
  r = construct_tx(alice_acc.get_keys(), std::vector<tx_source_entry>({ se }), std::vector<tx_destination_entry>({ tx_destination_entry(se.amount - TESTS_DEFAULT_FEE, alice_acc.get_public_address()) }),
    empty_attachment, tx_2, get_tx_version_from_events(events), 0);

  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  r = sign_multisig_input_in_tx(tx_2, 0, alice_acc.get_keys(), blk_3.miner_tx, &fully_signed_tx);
  CHECK_AND_ASSERT_MES(r && fully_signed_tx, false, "sign_multisig_input_in_tx failed, fully_signed_tx: " << fully_signed_tx);
  events.push_back(tx_2);

  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_3r, miner_acc, tx_2);

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, se.amount - TESTS_DEFAULT_FEE));

  return true;
}

//------------------------------------------------------------------------------

bool multisig_with_same_id_in_pool::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: create two txs having the same multisig id in their outputs and see how the core handles it.
  // Then put two txs referring to the ms out id in their inputs.

  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc); LOCAL_ASSERT(m_accounts.size() - 1 == MINER_ACC_IDX);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc); LOCAL_ASSERT(m_accounts.size() - 1 == ALICE_ACC_IDX);
  GENERATE_ACCOUNT(bob_acc);
  m_accounts.push_back(bob_acc);   LOCAL_ASSERT(m_accounts.size() - 1 == BOB_ACC_IDX);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2);

  bool r = false;
  uint64_t amount_many_out_have = 0, stub = 0;
  r = calculate_amounts_many_outs_have_and_no_outs_have(generator.get_base_reward_for_next_block(get_block_hash(blk_0r)), amount_many_out_have, stub); // this amount corresponds to many outs, so no changeback will be generated
  CHECK_AND_ASSERT_MES(r, false, "calculate_amounts_many_outs_have_and_no_outs_have failed");
  uint64_t amount = amount_many_out_have - TESTS_DEFAULT_FEE;

  std::list<account_public_address> to_addrs({ alice_acc.get_public_address(), alice_acc.get_public_address() }); // multisig to same account, mentioned twice

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;

  // tx_1: normal input -> multisig output
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc.get_keys(), to_addrs, amount, TESTS_DEFAULT_FEE, 0, sources, destinations, true, true);
  CHECK_AND_ASSERT_MES(r && sources.size() == 1 && destinations.size() == 1, false, "fill_tx_sources_and_destinations failed"); // we selected such good amount so sources.size() and destinations.size() should be eq 1

  // create tx_1 manually in order to have access to its one-time key
  tx_builder txb_1;
  txb_1.step1_init();
  txb_1.step2_fill_inputs(miner_acc.get_keys(), sources);
  txb_1.step3_fill_outputs(destinations, 0, 1);
  txb_1.step4_calc_hash();
  txb_1.step5_sign(sources);
  const transaction& tx_1 = txb_1.m_tx;

  // tx_1 should be accepted, no surprize here
  events.push_back(tx_1);

  // prepare sources and destinations for tx_2, using the same amount
  std::vector<tx_source_entry> sources2;
  std::vector<tx_destination_entry> destinations2;
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc.get_keys(), to_addrs, amount, TESTS_DEFAULT_FEE, 0, sources2, destinations2, true, true);
  CHECK_AND_ASSERT_MES(r && sources2.size() == 1 && destinations2.size() == 1, false, "fill_tx_sources_and_destinations failed"); // we selected such good amount so sources.size() and destinations.size() should be eq 1

  // tx_2 differs in inputs but should have the same outputs
  tx_builder txb_2;
  txb_2.step1_init();
  txb_2.m_tx_key = txb_1.m_tx_key; // use the same one-time tx key as tx_1
  // clear extra and put corresponding pub key to the extra
  txb_2.m_tx.extra.clear();
  add_tx_pub_key_to_extra(txb_2.m_tx, txb_2.m_tx_key.pub);
  txb_2.step2_fill_inputs(miner_acc.get_keys(), sources2); // use other sources
  txb_2.step3_fill_outputs(destinations, 0, 1);  // ...but the same destinations, thus vout should be the same (while one-time tx key is the same)
  txb_2.step4_calc_hash();
  txb_2.step5_sign(sources2);
  const transaction& tx_2 = txb_2.m_tx;

  // Well done! Now multisig output id in both txs should be the same.
  crypto::hash tx_1_ms_id = get_multisig_out_id(tx_1, get_tx_out_index_by_amount(tx_1, amount));
  crypto::hash tx_2_ms_id = get_multisig_out_id(tx_2, get_tx_out_index_by_amount(tx_2, amount));
  CHECK_AND_ASSERT_MES(tx_1_ms_id == tx_2_ms_id, false, "Multisig ids didn't match as expected");

  // tx_2 should be rejected from being added to the pool as it contains the same multisig id as tx_1 which is still in the poll
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_2);

  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  // now tx_2 should still be rejected by the pool as the same multisig id is in then blockchain
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_2);


  // Create and add two txs referring to the same ms out id in their inputs
  tx_source_entry se = AUTO_VAL_INIT(se);
  se.amount = amount;
  se.multisig_id = tx_1_ms_id;
  se.real_output_in_tx_index = get_tx_out_index_by_amount(tx_1, amount);
  se.real_out_tx_key = get_tx_pub_key_from_extra(tx_1);
  se.ms_keys_count = boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(tx_1.vout[se.real_output_in_tx_index]).target).keys.size();
  se.ms_sigs_count = 1;

  tx_destination_entry de(amount - TESTS_DEFAULT_FEE, bob_acc.get_public_address());

  transaction tx_3 = AUTO_VAL_INIT(tx_3);
  r = construct_tx(alice_acc.get_keys(), std::vector<tx_source_entry>({ se }), std::vector<tx_destination_entry>({ de }), empty_attachment, tx_3, 0, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  bool fully_signed_tx = false;
  r = sign_multisig_input_in_tx_custom(tx_3, 0, alice_acc.get_keys(), tx_1, &fully_signed_tx);
  CHECK_AND_ASSERT_MES(r && fully_signed_tx, false, "sign_multisig_input_in_tx failed, fully_signed_tx: " << fully_signed_tx);

  events.push_back(tx_3);

  transaction tx_4 = AUTO_VAL_INIT(tx_4);
  r = construct_tx(alice_acc.get_keys(), std::vector<tx_source_entry>({ se }), std::vector<tx_destination_entry>({ de }), empty_attachment, tx_4, 0, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

  // tx_4 should be rejected as containing multisig input already mentioned in the pool (by tx_3)
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_4);

  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_1, miner_acc, tx_3);

  // tx_4 should still be rejected as spending already spent (by tx_3) multisig output 
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx_4);

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(BOB_ACC_IDX, de.amount));

  return true;
}

//------------------------------------------------------------------------------

multisig_and_checkpoints::multisig_and_checkpoints()
{
  // NOTE: This test is made deterministic to be able to correctly set up checkpoint.
  random_state_test_restorer::reset_random(); // random generator's state was previously stored, will be restore on dtor (see also m_random_state_test_restorer)
  REGISTER_CALLBACK_METHOD(multisig_and_checkpoints, set_cp);
}

bool multisig_and_checkpoints::set_cp(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  currency::checkpoints checkpoints;
  checkpoints.add_checkpoint(15, "fda3e645fbfd0f4852aa68e6ad021c9005c9faf2d7ba6b1b3c8e24efb9c0e8d0");
  c.set_checkpoints(std::move(checkpoints));

  return true;
}

bool multisig_and_checkpoints::generate(std::vector<test_event_entry>& events) const
{
  // Test outline:
  // 1. Set checkpoint (on height 0 for height 15).
  // 2. Make few block to unlock mined money.
  // 3. tx_1: normal input -> multisig output
  // 4. tx_2: multisig input -> normal output (spends tx_1 ms out)
  // 5. tx_3: normal input -> multisig output
  // 6. (here blockchain passes checkpoint)
  // 7. tx_4 multisig input -> normal output (spends tx_3 ms out before CP)
  // 8. tx_5: normal input -> multisig output
  // 9. tx_6 multisig input -> normal output (spends tx_5 ms out)
  // 10. Check balances.

  uint64_t ts = 1450000000;
  test_core_time::adjust(ts);

  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  miner_acc.set_createtime(ts);
  m_accounts.push_back(miner_acc); LOCAL_ASSERT(m_accounts.size() - 1 == MINER_ACC_IDX);
  GENERATE_ACCOUNT(alice_acc);
  alice_acc.set_createtime(ts);
  m_accounts.push_back(alice_acc); LOCAL_ASSERT(m_accounts.size() - 1 == ALICE_ACC_IDX);
  GENERATE_ACCOUNT(bob_acc);
  bob_acc.set_createtime(ts);
  m_accounts.push_back(bob_acc);   LOCAL_ASSERT(m_accounts.size() - 1 == BOB_ACC_IDX);

  bool r = false;
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, ts);
  DO_CALLBACK(events, "configure_core");
  // set checkpoint
  DO_CALLBACK(events, "set_cp");
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2);

  uint64_t amount = TESTS_DEFAULT_FEE * 99;
  std::list<account_public_address> to_addrs({ alice_acc.get_public_address(), alice_acc.get_public_address() }); // multisig to same account, mentioned twice

  // tx_1: normal input -> multisig output
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc.get_keys(), to_addrs, amount, TESTS_DEFAULT_FEE, 0, sources, destinations, true, true, 1);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_1, currency::get_tx_version(get_block_height(blk_0r), m_hardforks), 0, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx");

  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true)); // tx_1 goes with the block blk_1
  events.push_back(tx_1);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));

  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  // tx_2: multisig input -> normal output (spends tx_1 ms out)
  tx_source_entry se = AUTO_VAL_INIT(se);
  se.amount = amount;
  se.real_output_in_tx_index = get_tx_out_index_by_amount(tx_1, amount);
  se.multisig_id = get_multisig_out_id(tx_1, se.real_output_in_tx_index);
  se.real_out_tx_key = get_tx_pub_key_from_extra(tx_1);
  se.ms_keys_count = boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(tx_1.vout[se.real_output_in_tx_index]).target).keys.size();
  se.ms_sigs_count = 1;

  tx_destination_entry de(amount - TESTS_DEFAULT_FEE, bob_acc.get_public_address());

  transaction tx_2 = AUTO_VAL_INIT(tx_2);
  r = construct_tx(alice_acc.get_keys(), std::vector<tx_source_entry>({ se }), std::vector<tx_destination_entry>({ de }), empty_attachment, tx_2, currency::get_tx_version(get_block_height(blk_1), m_hardforks), 0, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  bool tx_fully_signed = false;
  r = sign_multisig_input_in_tx(tx_2, 0, alice_acc.get_keys(), tx_1, &tx_fully_signed);
  CHECK_AND_ASSERT_MES(r & tx_fully_signed, false, "sign_multisig_input_in_tx failed, tx_fully_signed: " << tx_fully_signed);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true)); // tx_2 goes with the block blk_2
  events.push_back(tx_2);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, tx_2);

  ADJUST_TEST_CORE_TIME(blk_2.timestamp);
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(BOB_ACC_IDX, de.amount));


  // tx_3: normal input -> multisig output
  r = fill_tx_sources_and_destinations(events, blk_2, miner_acc.get_keys(), to_addrs, amount, TESTS_DEFAULT_FEE, 0, sources, destinations, true, true, 1);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  transaction tx_3 = AUTO_VAL_INIT(tx_3);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_3, currency::get_tx_version(get_block_height(blk_2), m_hardforks), 0, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx");

  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true)); // tx_3 goes with the block blk_3
  events.push_back(tx_3);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));

  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_3);  // <-- CP

  //
  // test multisig after CP
  //

  // tx_4 multisig input -> normal output (spends tx_3 ms out)
  se = AUTO_VAL_INIT(se);
  se.amount = amount;
  se.real_output_in_tx_index = get_tx_out_index_by_amount(tx_3, amount);
  se.multisig_id = get_multisig_out_id(tx_3, se.real_output_in_tx_index);
  se.real_out_tx_key = get_tx_pub_key_from_extra(tx_3);
  se.ms_keys_count = boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(tx_3.vout[se.real_output_in_tx_index]).target).keys.size();
  se.ms_sigs_count = 1;

  de = tx_destination_entry(amount - TESTS_DEFAULT_FEE, bob_acc.get_public_address());

  transaction tx_4 = AUTO_VAL_INIT(tx_4);
  r = construct_tx(alice_acc.get_keys(), std::vector<tx_source_entry>({ se }), std::vector<tx_destination_entry>({ de }), empty_attachment, tx_4, currency::get_tx_version(get_block_height(blk_3), m_hardforks), 0, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  r = sign_multisig_input_in_tx(tx_4, 0, alice_acc.get_keys(), tx_3, &tx_fully_signed);
  CHECK_AND_ASSERT_MES(r & tx_fully_signed, false, "sign_multisig_input_in_tx failed, tx_fully_signed: " << tx_fully_signed);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true)); // tx_4 goes with the block blk_4
  events.push_back(tx_4);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));
  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_3, miner_acc, tx_4);

  // tx_5: normal input -> multisig output
  r = fill_tx_sources_and_destinations(events, blk_4, miner_acc.get_keys(), to_addrs, amount, TESTS_DEFAULT_FEE, 0, sources, destinations, true, true, 1);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  transaction tx_5 = AUTO_VAL_INIT(tx_5);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_5, currency::get_tx_version(get_block_height(blk_4), m_hardforks), 0, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx");
  events.push_back(tx_5);

  MAKE_NEXT_BLOCK_TX1(events, blk_5, blk_4, miner_acc, tx_5);

  // tx_6 multisig input -> normal output (spends tx_5 ms out)
  se = AUTO_VAL_INIT(se);
  se.amount = amount;
  se.real_output_in_tx_index = get_tx_out_index_by_amount(tx_5, amount);
  se.multisig_id = get_multisig_out_id(tx_5, se.real_output_in_tx_index);
  se.real_out_tx_key = get_tx_pub_key_from_extra(tx_5);
  se.ms_keys_count = boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(tx_5.vout[se.real_output_in_tx_index]).target).keys.size();
  se.ms_sigs_count = 1;

  de = tx_destination_entry(amount - TESTS_DEFAULT_FEE, bob_acc.get_public_address());

  transaction tx_6 = AUTO_VAL_INIT(tx_6);
  r = construct_tx(alice_acc.get_keys(), std::vector<tx_source_entry>({ se }), std::vector<tx_destination_entry>({ de }), empty_attachment, tx_6, currency::get_tx_version(get_block_height(blk_5), m_hardforks), 0, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  r = sign_multisig_input_in_tx(tx_6, 0, alice_acc.get_keys(), tx_5, &tx_fully_signed);
  CHECK_AND_ASSERT_MES(r & tx_fully_signed, false, "sign_multisig_input_in_tx failed, tx_fully_signed: " << tx_fully_signed);
  events.push_back(tx_6);
  MAKE_NEXT_BLOCK_TX1(events, blk_6, blk_5, miner_acc, tx_6);

  ADJUST_TEST_CORE_TIME(blk_6.timestamp);
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(BOB_ACC_IDX, de.amount * 3));

  return true;
}

//------------------------------------------------------------------------------

multisig_and_checkpoints_bad_txs::multisig_and_checkpoints_bad_txs()
{
  REGISTER_CALLBACK_METHOD(multisig_and_checkpoints_bad_txs, set_cp);
}

bool multisig_and_checkpoints_bad_txs::set_cp(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // set checkpoint far enough - it won't be reached in the test
  currency::checkpoints checkpoints;
  checkpoints.add_checkpoint(100, "0000000000000000000000000000000000000000000000000000000000000001");
  c.set_checkpoints(std::move(checkpoints));
  return true;
}

bool multisig_and_checkpoints_bad_txs::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: check few hypotheses about handling incorrect multisig inputs and outputs within checkpoint zone.

  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc); LOCAL_ASSERT(m_accounts.size() - 1 == MINER_ACC_IDX);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc); LOCAL_ASSERT(m_accounts.size() - 1 == ALICE_ACC_IDX);
  GENERATE_ACCOUNT(bob_acc);
  m_accounts.push_back(bob_acc);   LOCAL_ASSERT(m_accounts.size() - 1 == BOB_ACC_IDX);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, test_core_time::get_time());
  DO_CALLBACK(events, "set_cp"); // set checkpoint
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2);

  bool r = false;
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;

  uint64_t amount = TESTS_DEFAULT_FEE * 99;
  std::list<account_public_address> to_addrs({ alice_acc.get_public_address(), alice_acc.get_public_address() }); // multisig to same account, mentioned twice

  // tx_1: normal key to multisig transaction
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc.get_keys(), to_addrs, amount, TESTS_DEFAULT_FEE, 0, sources, destinations, true, true, 1);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_1, TRANSACTION_VERSION_PRE_HF4, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx");

  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true)); // tx_1 goes with the block blk_1
  events.push_back(tx_1);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));

  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, 0, 0, 0, 0, 0));

  // tx_2: invalid ms input sigs_count (should fail even in CP zone)
  transaction tx_2 = AUTO_VAL_INIT(tx_2);
  r = make_tx_multisig_to_key(tx_1, get_tx_out_index_by_amount(tx_1, amount), std::list<account_keys>({ alice_acc.get_keys() }), bob_acc.get_public_address(), tx_2);
  CHECK_AND_ASSERT_MES(r, false, "make_tx_multisig_to_key failed");
  tx_2.signatures.resize(10);
  boost::get<txin_multisig>(tx_2.vin[0]).sigs_count = 10;

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
  events.push_back(tx_2);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));

  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(BOB_ACC_IDX, 0, 0, 0, 0, 0));

  // tx_3: no signatures, zero sigs_count in ms input (should pass under CP zone)
  transaction tx_3 = AUTO_VAL_INIT(tx_3);
  r = make_tx_multisig_to_key(tx_1, get_tx_out_index_by_amount(tx_1, amount), std::list<account_keys>({ alice_acc.get_keys() }), bob_acc.get_public_address(), tx_3);
  CHECK_AND_ASSERT_MES(r, false, "make_tx_multisig_to_key failed");
  tx_3.signatures.clear();
  boost::get<txin_multisig>(tx_3.vin[0]).sigs_count = 0;

  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
  events.push_back(tx_3);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));

  // tx_4: zero ms out keys (should FAIL)
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc.get_keys(), to_addrs, amount, TESTS_DEFAULT_FEE, 0, sources, destinations, true, true, 1);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  tx_builder txb = AUTO_VAL_INIT(txb);
  txb.step1_init();
  txb.step2_fill_inputs(miner_acc.get_keys(), sources);
  txb.step3_fill_outputs(destinations, 0, 1);
  boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(txb.m_tx.vout[0]).target).keys.clear(); // zero keys
  txb.step4_calc_hash();
  txb.step5_sign(sources);
  transaction tx_4 = txb.m_tx;
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
  events.push_back(tx_4);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));

  // tx_6: many ms out keys + no sigs (should pass due to CP zone)
  txb = AUTO_VAL_INIT(txb);
  txb.step1_init();
  txb.step2_fill_inputs(miner_acc.get_keys(), sources);
  txb.step3_fill_outputs(destinations, 0, 1);
  crypto::public_key k = boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(txb.m_tx.vout[0]).target).keys[0];
  boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(txb.m_tx.vout[0]).target).keys.resize(1500, k);
  txb.step4_calc_hash();
  txb.step5_sign(sources);
  txb.m_tx.signatures.clear();
  transaction tx_6 = txb.m_tx;
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
  events.push_back(tx_6);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));

  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_6);

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, 0, 0, 0, 0, 0)); // Alice got the money within ms-input in tx_6, but they don't show up in the balance

  // tx_7: just spending tx_6 ms out, sending to key (Alice -> Bob)
  transaction tx_7 = AUTO_VAL_INIT(tx_7);
  r = make_tx_multisig_to_key(tx_6, get_tx_out_index_by_amount(tx_6, amount), std::list<account_keys>({ alice_acc.get_keys() }), bob_acc.get_public_address(), tx_7);
  CHECK_AND_ASSERT_MES(r, false, "make_tx_multisig_to_key failed");
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true));
  events.push_back(tx_7);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));

  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_3, miner_acc, tx_7);

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(BOB_ACC_IDX, amount - TESTS_DEFAULT_FEE)); // Bob finally got the money

  return true;
}

//------------------------------------------------------------------------------
multisig_and_altchains::multisig_and_altchains()
{
  REGISTER_CALLBACK_METHOD(multisig_and_altchains, check_tx_pool);
}

bool multisig_and_altchains::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: check multisig output handling in case of multiple altchain switching

  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc); LOCAL_ASSERT(m_accounts.size() - 1 == MINER_ACC_IDX);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc); LOCAL_ASSERT(m_accounts.size() - 1 == ALICE_ACC_IDX);
  GENERATE_ACCOUNT(bob_acc);
  m_accounts.push_back(bob_acc);   LOCAL_ASSERT(m_accounts.size() - 1 == BOB_ACC_IDX);
  GENERATE_ACCOUNT(carol_acc);
  m_accounts.push_back(carol_acc); LOCAL_ASSERT(m_accounts.size() - 1 == CAROL_ACC_IDX);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  bool r = false;
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;

  uint64_t amount = TESTS_DEFAULT_FEE * 99;
  std::list<account_public_address> to_addrs({ alice_acc.get_public_address(), alice_acc.get_public_address() }); // multisig to same account, mentioned twice

  // tx_1: normal key to multisig transaction
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc.get_keys(), to_addrs, amount, TESTS_DEFAULT_FEE, 0, sources, destinations, true, true, 1);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_1, 0, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx");
  events.push_back(tx_1);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  MAKE_NEXT_BLOCK(events, blk_1b, blk_0r, miner_acc);

  //  0  ..  10     11     12     13     14     15    <- height
  // (0 ).. (0r)-  (1 )-                              <- main chain
  //           |   tx_1
  //           \-  (1b)-


  // tx_2 and tx_3 both spend the SAME multisig output in tx_1, transferring coins to Bob (tx_2) or Carol (tx_3)
  transaction tx_2 = AUTO_VAL_INIT(tx_2);
  r = make_tx_multisig_to_key(tx_1, get_tx_out_index_by_amount(tx_1, amount), std::list<account_keys>({ alice_acc.get_keys() }), bob_acc.get_public_address(), tx_2);
  CHECK_AND_ASSERT_MES(r, false, "make_tx_multisig_to_key failed");
  transaction tx_3 = AUTO_VAL_INIT(tx_3);
  r = make_tx_multisig_to_key(tx_1, get_tx_out_index_by_amount(tx_1, amount), std::list<account_keys>({ alice_acc.get_keys() }), carol_acc.get_public_address(), tx_3);
  CHECK_AND_ASSERT_MES(r, false, "make_tx_multisig_to_key failed");

  events.push_back(tx_2);

  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_2b, blk_1b, miner_acc, tx_2);

  //  0  ..  10     11     12     13     14     15    <- height
  // (0 ).. (0r)-  (1 )-                              <- main chain
  //           |   tx_1
  //           \-  (1b)-  #2b#                        <- 2b is rejected as tx_2 refers to non-existing multisig out id (in tx_1)
  //           |          tx_2


  // make sure tx_2 is still in the pool
  DO_CALLBACK_PARAMS(events, "check_tx_pool", params_tx_pool(1));
  DO_CALLBACK_PARAMS(events, "check_tx_pool", params_tx_pool(tx_2));

  MAKE_NEXT_BLOCK_TX1(events, blk_1c, blk_0r, miner_acc, tx_1);
  MAKE_NEXT_BLOCK(events, blk_2c, blk_1c, miner_acc);

  DO_CALLBACK(events, "mark_invalid_tx"); // tx_3 should be rejected here by the pool, as it's tx_2 in the pool which is referring to the same ms id in tx_1
  events.push_back(tx_3);

  MAKE_NEXT_BLOCK_TX1(events, blk_3c, blk_2c, miner_acc, tx_2);

  //  0  ..  10     11     12     13     14     15    <- height
  // (0 ).. (0r)-  (1 )-
  //           |   tx_1
  //           \-  (1b)-  #2b#
  //           |          tx_2
  //           |-  (1c)-  (2c)-  (3c)-                <- becomes main chain
  //               tx_1          tx_2

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(BOB_ACC_IDX, amount - TESTS_DEFAULT_FEE));

  DO_CALLBACK(events, "mark_invalid_tx"); // tx_3 should be rejected by the pool, as spending already spent ms output in tx_2
  events.push_back(tx_3);

  DO_CALLBACK_PARAMS(events, "check_tx_pool", params_tx_pool(0));


  // add blocks to the first chain and trigger chain switching
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);
  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_3, miner_acc, tx_2);

  //  0  ..  10     11     12     13     14     15    <- height
  // (0 ).. (0r)-  (1 )-  (2 )-  (3 )-  (4 )-         <- becomes main chain
  //           |   tx_1                 tx_2
  //           \-  (1b)-  #2b#
  //           |          tx_2
  //           |-  (1c)-  (2c)-  (3c)-
  //               tx_1          tx_2 

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(CAROL_ACC_IDX, 0)); // tx_3 was rejected, thus zero
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(BOB_ACC_IDX, amount - TESTS_DEFAULT_FEE));
  DO_CALLBACK_PARAMS(events, "check_tx_pool", params_tx_pool(0));


  // add blocks to new subchain and trigger chain switching
  MAKE_NEXT_BLOCK(events, blk_3d, blk_2, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_4d, blk_3d, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_5d, blk_4d, miner_acc);

  // make sure tx_2 was moved out to the pool
  DO_CALLBACK_PARAMS(events, "check_tx_pool", params_tx_pool(1));
  DO_CALLBACK_PARAMS(events, "check_tx_pool", params_tx_pool(tx_2));

  //  0  ..  10     11     12     13     14     15    <- height
  // (0 ).. (0r)-  (1 )-  (2 )-  (3 )-  (4 )-         <- becomes main chain
  //           |   tx_1                 tx_2
  //                         \-  (3d)-  (4d)-  (5d)-

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(BOB_ACC_IDX, amount - TESTS_DEFAULT_FEE, 0, 0, amount - TESTS_DEFAULT_FEE));

  MAKE_NEXT_BLOCK_TX1(events, blk_6d, blk_5d, miner_acc, tx_2);
  DO_CALLBACK_PARAMS(events, "check_tx_pool", params_tx_pool(0));

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(BOB_ACC_IDX, amount - TESTS_DEFAULT_FEE, 0, 0, 0, 0));

  return true;
}

bool multisig_and_altchains::check_tx_pool(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  params_tx_pool p = AUTO_VAL_INIT(p);
  bool r = epee::string_tools::hex_to_pod(boost::get<callback_entry>(events[ev_index]).callback_params, p);
  CHECK_AND_ASSERT_MES(r, false, "Can't obtain event params. Forgot to pass them?");

  if (p.counter != SIZE_MAX)
  {
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == p.counter, false, "Tx pool has " << c.get_pool_transactions_count() << " transactions, NOT " << p.counter << " as expected");
  }
  else
  {
    std::list<transaction> txs;
    bool r = c.get_pool_transactions(txs);
    CHECK_AND_ASSERT_MES(r, false, "get_pool_transactions failed");
    r = false;
    for (const auto& t : txs)
    {
      if (get_transaction_hash(t) == p.tx_hash)
      {
        r = true;
        break;
      }
    }
    CHECK_AND_ASSERT_MES(r, false, "tx " << p.tx_hash << " is NOT in the pool as expected");
  }

  return true;
}

//------------------------------------------------------------------------------

bool ref_by_id_basics::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: transfer some coins (Miner->Alicee->Bob) using only ref_by_id inputs to make sure they are handled okay.

  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc); LOCAL_ASSERT(m_accounts.size() - 1 == MINER_ACC_IDX);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc); LOCAL_ASSERT(m_accounts.size() - 1 == ALICE_ACC_IDX);
  GENERATE_ACCOUNT(bob_acc);
  m_accounts.push_back(bob_acc);   LOCAL_ASSERT(m_accounts.size() - 1 == BOB_ACC_IDX);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 4);

  bool r = false;
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;

  uint64_t amount = TESTS_DEFAULT_FEE * 90;
                                                                                                        
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc, alice_acc, amount, TESTS_DEFAULT_FEE, 4, sources, destinations, true, true, true);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_1, 0, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx");

  r = check_ring_signature_at_gen_time(events, get_block_hash(blk_0r), boost::get<txin_to_key>(tx_1.vin[0]), get_transaction_hash(tx_1), boost::get<currency::NLSAG_sig>(tx_1.signatures[0]).s);
  CHECK_AND_ASSERT_MES(r, false, "check_ring_signature_at_gen_time failed");

  events.push_back(tx_1);

  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, amount, 0, 0, 0, 0));

  r = fill_tx_sources_and_destinations(events, blk_1, alice_acc, bob_acc, amount - TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, 0, sources, destinations, true, true, true);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  transaction tx_2 = AUTO_VAL_INIT(tx_2);
  r = construct_tx(alice_acc.get_keys(), sources, destinations, empty_attachment, tx_2, 0, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx");
  events.push_back(tx_2);

  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, tx_2);

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, 0, 0, 0, 0, 0));
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(BOB_ACC_IDX, amount - TESTS_DEFAULT_FEE, 0, 0, 0, 0));

  return true;
}

//------------------------------------------------------------------------------

bool ref_by_id_mixed_inputs_types::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc); LOCAL_ASSERT(m_accounts.size() - 1 == MINER_ACC_IDX);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc); LOCAL_ASSERT(m_accounts.size() - 1 == ALICE_ACC_IDX);
  GENERATE_ACCOUNT(bob_acc);
  m_accounts.push_back(bob_acc);   LOCAL_ASSERT(m_accounts.size() - 1 == BOB_ACC_IDX);

  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2);

  bool r = false;
  std::vector<tx_source_entry> sources, sources2;
  std::vector<tx_destination_entry> destinations, destinations2;

  // obtain such amount no output ever has
  uint64_t amount = 0, stub = 0;
  r = calculate_amounts_many_outs_have_and_no_outs_have(get_outs_money_amount(blk_0r.miner_tx), stub, amount);
  CHECK_AND_ASSERT_MES(r, false, "calculate_amounts_many_outs_have_and_no_outs_have failed");

  MAKE_TX_LIST_START(events, tx_list_1, miner_acc, alice_acc, amount, blk_0r);
  MAKE_TX_LIST(events, tx_list_1, miner_acc, alice_acc, amount, blk_0r);
  MAKE_TX_LIST(events, tx_list_1, miner_acc, alice_acc, amount, blk_0r);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, tx_list_1);

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, amount * 3, 0, 0, 0, 0));

  r = fill_tx_sources_and_destinations(events, blk_1, alice_acc, bob_acc, amount * 3 - TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, 0, sources, destinations, true, true, false); // normal inputs
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  r = fill_tx_sources_and_destinations(events, blk_1, alice_acc, bob_acc, amount * 3 - TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, 0, sources2, destinations2, true, true, true); // ref_by_id
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");

  // mix sources to achive mixed normal and ref_by_id inputs
  sources[1] = sources2[1];

  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(alice_acc.get_keys(), sources, destinations, empty_attachment, tx_1, 0, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

  // make sure we created correct transaction
  size_t normal_outs_count = 0, ref_by_id_outs_count = 0;
  for (auto& in : tx_1.vin)
  {
    CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, in2key, false);
    CHECK_AND_ASSERT_MES(in2key.amount == amount, false, "Invalid amount in source entries");
    for (auto& ko : in2key.key_offsets)
    {
      if (ko.type().hash_code() == typeid(ref_by_id).hash_code())
        ++ref_by_id_outs_count;
      else if (ko.type().hash_code() == typeid(uint64_t).hash_code())
        ++normal_outs_count;
    }
  }
  CHECK_AND_ASSERT_MES(normal_outs_count == 2 && ref_by_id_outs_count == 1, false, "Incorrect tx created");

  events.push_back(tx_1);

  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, tx_1);


  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, 0, 0, 0, 0, 0));
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(BOB_ACC_IDX, amount * 3 - TESTS_DEFAULT_FEE, 0, 0, 0, 0));


  return true;

}

//------------------------------------------------------------------------------

multisig_n_participants_seq_signing::multisig_n_participants_seq_signing()
  : m_participants_count(10)
  , m_minimum_signs_to_spend(8)
{
  m_participants.resize(m_participants_count);
  for(auto& p : m_participants)
    p.generate();
}

bool multisig_n_participants_seq_signing::generate(std::vector<test_event_entry>& events) const
{
  // test idea: tx with ms input is being signed by N participants _sequentially_ and _separately_ (real-life approximation: they don't know privates keys of each other)
  bool r = false;
  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);

  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, preminer_acc, test_core_time::get_time());
  events.push_back(blk_0);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // prepare ms data: amount and list of participants' adresses (belived to be publicly available)
  uint64_t ms_amount = TESTS_DEFAULT_FEE * 17;
  std::list<account_public_address> ms_addr_list;
  for(auto &p : m_participants)
    ms_addr_list.push_back(p.get_public_address());

  // Miner makes a tx with ms output to all the participants (with minimum_sigs == m_minimum_signs_to_spend)
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc.get_keys(), ms_addr_list, ms_amount, TESTS_DEFAULT_FEE, 0, sources, destinations, true, true, m_minimum_signs_to_spend);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_1, get_tx_version_from_events(events), 0);
  events.push_back(tx_1);

  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);
  //REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);


  // Alice (a very third person) make a tx, spending ms output
  // THEN all the participants one-by-one sign it
  sources.clear();
  destinations.clear();
  size_t ms_out_index = get_multisig_out_index(tx_1.vout);
  CHECK_AND_ASSERT_MES(ms_out_index != tx_1.vout.size(), false, "Can't find ms out index in tx_1");
  tx_source_entry se = AUTO_VAL_INIT(se);
  se.amount =boost::get<currency::tx_out_bare>( tx_1.vout[ms_out_index]).amount;
  se.multisig_id = get_multisig_out_id(tx_1, ms_out_index);
  se.ms_sigs_count = m_minimum_signs_to_spend;
  // se.outputs -- not used for ms-outs
  // se.real_output -- not used for ms-outs
  se.real_output_in_tx_index = ms_out_index;
  se.real_out_tx_key = get_tx_pub_key_from_extra(tx_1);
  // se.separately_signed_tx_complete -- not a separately-signed tx
  se.ms_keys_count = boost::get<txout_multisig>(boost::get<currency::tx_out_bare>(tx_1.vout[ms_out_index]).target).keys.size();
  sources.push_back(se);

  tx_destination_entry de(ms_amount - TESTS_DEFAULT_FEE, alice_acc.get_public_address());
  destinations.push_back(de);

  // construct a transaction (no participants keys are provided, thus no signs for ms input are created)
  transaction tx = AUTO_VAL_INIT(tx);
  r = construct_tx(alice_acc.get_keys(), sources, destinations, empty_attachment, tx, get_tx_version_from_events(events), 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

  // sign the transaction by (m_minimum_signs_to_spend) participants in random order
  std::vector<account_base> shuffled_participants(m_participants);
  std::shuffle(shuffled_participants.begin(), shuffled_participants.end(), crypto::uniform_random_bit_generator());
  for (size_t i = 0; i < m_minimum_signs_to_spend; ++i)
  {
    r = sign_multisig_input_in_tx(tx, 0, shuffled_participants[i].get_keys(), tx_1);
    CHECK_AND_ASSERT_MES(r, false, "sign_ms_input_by_a_participant failed");
  }

  events.push_back(tx);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, tx);

  std::shared_ptr<tools::wallet2> alice_wlt;
  r = generator.init_test_wallet(alice_acc, get_block_hash(blk_0), alice_wlt);
  CHECK_AND_ASSERT_MES(r, false, "");

  r = generator.refresh_test_wallet(events, alice_wlt.get(), get_block_hash(blk_2), CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2);
  CHECK_AND_ASSERT_MES(r, false, "refresh_test_wallet failed");

  r = check_balance_via_wallet(*alice_wlt.get(), "alice", ms_amount - TESTS_DEFAULT_FEE, 0, 0, 0, 0);
  CHECK_AND_ASSERT_MES(r, false, "invalid balance");

  return true;
}

bool multisig_n_participants_seq_signing::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{


  return true;
}

//------------------------------------------------------------------------------

bool multisig_out_make_and_spent_in_altchain::generate(std::vector<test_event_entry>& events) const
{
  // test idea: one tx has a multisig out in an altchain, and the second tx in the same altchain spents it

  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // make a few blocks, this is main chain
  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_acc);

  bool r = false;
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;

  uint64_t amount = TESTS_DEFAULT_FEE * 33;
  std::list<account_public_address> to_addrs({ alice_acc.get_public_address(), alice_acc.get_public_address() }); // multisig to same account, mentioned twice

  // tx_1: normal key to multisig transaction
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc.get_keys(), to_addrs, amount, TESTS_DEFAULT_FEE * 7, 0, sources, destinations, true, true, 2);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_1, 0, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_1);
  
  MAKE_NEXT_BLOCK_TX1(events, blk_1a, blk_0r, miner_acc, tx_1);

  MAKE_NEXT_BLOCK(events, blk_2a, blk_1a, miner_acc);

  //  0  ..  10     11     12     13     14     15    <- height
  // (0 ).. (0r)-  (1 )-  (2 )-  (3 )-  (4 )-         <- main chain
  //           |
  //           \-  (1a)-  (2a)-  (3a)-  (4a)-
  //               tx_1          tx_2   tx_3          <- tx_2 spents multisig out in tx_1
  //                ^              |      |
  //                +<----<----<---+ <----+

  
  // atm tx_2 can't be added to the pool, as it contains reference to an ms out (in it's inputs) that is already referenced to tx_1 (in it's outs) still waiting in the pool
  // to workaround this we simulate alt block tx pushing, when txs are pushed to the core right before the block and with "kept_by_block" flag
  
  // tx_2
  transaction tx_2 = AUTO_VAL_INIT(tx_2);
  r = make_tx_multisig_to_key(tx_1, get_multisig_out_index(tx_1.vout), std::list<account_keys>({ alice_acc.get_keys(), alice_acc.get_keys() }), bob_acc.get_public_address(), tx_2, TESTS_DEFAULT_FEE * 13);
  CHECK_AND_ASSERT_MES(r, false, "make_tx_multisig_to_key failed");
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true)); // simulate 
  events.push_back(tx_2);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));
  
  MAKE_NEXT_BLOCK_TX1(events, blk_3a, blk_2a, miner_acc, tx_2);

  // tx_3 spends the same ms out
  transaction tx_3 = AUTO_VAL_INIT(tx_3);
  r = make_tx_multisig_to_key(tx_1, get_multisig_out_index(tx_1.vout), std::list<account_keys>({ alice_acc.get_keys(), alice_acc.get_keys() }), bob_acc.get_public_address(), tx_3, TESTS_DEFAULT_FEE * 13);
  CHECK_AND_ASSERT_MES(r, false, "make_tx_multisig_to_key failed");
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, true)); // simulate 
  events.push_back(tx_3);
  events.push_back(event_visitor_settings(event_visitor_settings::set_txs_kept_by_block, false));
  
  DO_CALLBACK(events, "mark_invalid_block"); // alt block 4a should be rejected as it has invalid tx_3 which spends already spent ms output of tx_1
  MAKE_NEXT_BLOCK_TX1(events, blk_4a, blk_3a, miner_acc, tx_3);

  MAKE_NEXT_BLOCK(events, blk_4b, blk_3a, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_5b, blk_4b, miner_acc);

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_5b), get_block_hash(blk_5b)));
  size_t tx_count = 0;
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", tx_count);
  DO_CALLBACK(events, "clear_tx_pool");

  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, 0));
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(BOB_ACC_IDX, amount - get_tx_fee(tx_2)));

  return true;
}

//------------------------------------------------------------------------------

bool multisig_out_spent_in_altchain_case_b4::generate(std::vector<test_event_entry>& events) const
{
  // test idea: multisig output is spent in altchain, while output's source tx is in the main chain atfer split height (case b4)
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  bool r = false;
  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;

  uint64_t amount = TESTS_DEFAULT_FEE * 22;
  std::list<account_public_address> to_addrs({ alice_acc.get_public_address(), alice_acc.get_public_address() }); // multisig to same account, mentioned twice

  // tx_1: normal key to multisig transaction
  r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc.get_keys(), to_addrs, amount, TESTS_DEFAULT_FEE * 5, 0, sources, destinations, true, true, 2);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_attachment, tx_1, 0, CURRENCY_TO_KEY_OUT_RELAXED, true);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  events.push_back(tx_1);
  
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);

  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);

  //  0  ..  10     11     12     13     14     15    <- height
  // (0 ).. (0r)-  (1 )-  (2 )-  (3 )-  (4 )-         <- main chain
  //           |   tx_1          tx_2                 <- tx_2 spents multisig out in tx_1
  //           |    ^              | 
  //           |    +<----<----<---+ 
  //           |
  //           \-  (1a)-  (2a)-  (3a)-  (4a)-         <- alt chain
  //               tx_2                               <- tx_2 refers to unknown ms out (tx_1 won't be in this chain event if switching occurs)

  
  // tx_2
  transaction tx_2 = AUTO_VAL_INIT(tx_2);
  r = make_tx_multisig_to_key(tx_1, get_multisig_out_index(tx_1.vout), std::list<account_keys>({ alice_acc.get_keys(), alice_acc.get_keys() }), bob_acc.get_public_address(), tx_2, TESTS_DEFAULT_FEE * 13);
  CHECK_AND_ASSERT_MES(r, false, "make_tx_multisig_to_key failed");
  events.push_back(tx_2);
  
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_2);
  MAKE_NEXT_BLOCK(events, blk_4, blk_3, miner_acc);


  // alt chain

  // block blk_1a is invalid as it contains tx_2 which refers to unknown ms output
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_1a, blk_0r, miner_acc, tx_2);

  DO_CALLBACK_PARAMS(events, "check_top_block", params_top_block(get_block_height(blk_4), get_block_hash(blk_4)));
  size_t tx_count = 0;
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", tx_count);

  return true;
}

//------------------------------------------------------------------------------

multisig_unconfirmed_transfer_and_multiple_scan_pool_calls::multisig_unconfirmed_transfer_and_multiple_scan_pool_calls()
{
  REGISTER_CALLBACK_METHOD(multisig_unconfirmed_transfer_and_multiple_scan_pool_calls, c1);
}

bool multisig_unconfirmed_transfer_and_multiple_scan_pool_calls::generate(std::vector<test_event_entry>& events) const
{
  m_accounts.resize(TOTAL_ACCS_COUNT);
  auto& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  auto& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  auto& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK(events, "c1");

  return true;
}

bool multisig_unconfirmed_transfer_and_multiple_scan_pool_calls::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  auto& miner_acc = m_accounts[MINER_ACC_IDX];
  auto& alice_acc = m_accounts[ALICE_ACC_IDX];
  auto& bob_acc   = m_accounts[BOB_ACC_IDX];
  
  uint64_t ms_amount = TESTS_DEFAULT_FEE * 13;

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, m_accounts[MINER_ACC_IDX]);
  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW, false, "Incorrect numbers of blocks fetched");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  std::vector<currency::extra_v> extra;
  std::vector<currency::attachment_v> attachments;

  std::vector<tx_destination_entry> dst;
  dst.resize(1);
  // to_key => multisig
  dst.back().addr.push_back(miner_acc.get_public_address());
  dst.back().addr.push_back(alice_acc.get_public_address());
  dst.back().amount = ms_amount + TESTS_DEFAULT_FEE;
  dst.back().minimum_sigs = dst.back().addr.size();
  transaction key_to_ms_tx = AUTO_VAL_INIT(key_to_ms_tx);
  miner_wlt->transfer(dst, 0, 0, TESTS_DEFAULT_FEE, extra, attachments, tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), key_to_ms_tx);

  bool r = mine_next_pow_blocks_in_playtime(miner_acc.get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  size_t ms_out_idx = get_multisig_out_index(key_to_ms_tx.vout);
  crypto::hash multisig_id = get_multisig_out_id(key_to_ms_tx, ms_out_idx);
  CHECK_AND_ASSERT_MES(multisig_id != null_hash, false, "Multisig failed: failed to get get_multisig_out_id");

  //r = mine_next_pow_blocks_in_playtime(miner_acc.get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  //CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, alice_acc);

  miner_wlt->refresh();
  alice_wlt->refresh();
  tools::multisig_transfer_container ms_m, ms_a;
  miner_wlt->get_multisig_transfers(ms_m);
  alice_wlt->get_multisig_transfers(ms_a);
  CHECK_AND_ASSERT_MES(ms_m.size() == 1 && ms_a.size() == 1, false, "Multisig failed: ms_m.size() == 1 && ms_a.size() == 1");

  LOG_PRINT_YELLOW("%%%%% generating multisig spending tx...", LOG_LEVEL_0);

  std::vector<tx_destination_entry> dst2(1);
  dst2.back().addr.resize(1);
  dst2.back().addr.back() = bob_acc.get_public_address();
  dst2.back().amount = ms_amount;
  dst2.back().minimum_sigs = dst2.back().addr.size();
  std::list<currency::account_keys> acc_keys;
  //acc_keys.push_back(miner_acc.get_keys()); will be signed as we pass miner_wlt
  acc_keys.push_back(alice_acc.get_keys());
  transaction tx = AUTO_VAL_INIT(tx);
  transfer_multisig(*miner_wlt.get(),
    acc_keys,
    multisig_id,
    dst2,
    0,
    TESTS_DEFAULT_FEE,
    extra,
    attachments,
    tools::detail::ssi_digit,
    tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD),
    tx, c.get_current_tx_version());

  LOG_PRINT_YELLOW("%%%%% tx " << get_transaction_hash(tx) << " is spending multisig output " << multisig_id, LOG_LEVEL_0);
  
  bool stub;
  std::deque<tools::transfer_details> transfers;
  std::vector<tools::wallet_public::wallet_transfer_info> unconfirmed_transfers;

  alice_wlt->scan_tx_pool(stub);
  alice_wlt->get_transfers(transfers);
  CHECK_AND_ASSERT_MES(transfers.size() == 0, false, "incorrect transfers size for Alice: " << transfers.size() << "\n" << alice_wlt->dump_trunsfers());
  alice_wlt->get_unconfirmed_transfers(unconfirmed_transfers);
  CHECK_AND_ASSERT_MES(unconfirmed_transfers.size() == 1, false, "incorrect unconfirmed transfers size for Alice: " << unconfirmed_transfers.size());
  CHECK_AND_ASSERT_MES(alice_wlt->get_multisig_transfers().size() == 1, false, "incorrect multisig transfers size for Alice: " << alice_wlt->get_multisig_transfers().size());

  uint32_t flags = alice_wlt->get_multisig_transfers().begin()->second.m_flags;
  CHECK_AND_ASSERT_MES(flags == WALLET_TRANSFER_DETAIL_FLAG_SPENT, false, "incorrect ms transfer flags for Alice: " << flags);

  
  // scan the pool second time (nothing happened neither in the pool, nor in blockchain)
  alice_wlt->scan_tx_pool(stub);
  transfers.clear();
  unconfirmed_transfers.clear();
  alice_wlt->get_transfers(transfers);
  CHECK_AND_ASSERT_MES(transfers.size() == 0, false, "incorrect transfers size for Alice: " << transfers.size() << "\n" << alice_wlt->dump_trunsfers());
  alice_wlt->get_unconfirmed_transfers(unconfirmed_transfers);
  CHECK_AND_ASSERT_MES(unconfirmed_transfers.size() == 1, false, "incorrect unconfirmed transfers size for Alice: " << unconfirmed_transfers.size());
  CHECK_AND_ASSERT_MES(alice_wlt->get_multisig_transfers().size() == 1, false, "incorrect multisig transfers size for Alice: " << alice_wlt->get_multisig_transfers().size());

  // flags should not change
  flags = alice_wlt->get_multisig_transfers().begin()->second.m_flags;
  CHECK_AND_ASSERT_MES(flags == WALLET_TRANSFER_DETAIL_FLAG_SPENT, false, "incorrect ms transfer flags for Alice: " << flags);

  return true;
}
