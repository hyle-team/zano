// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "wallet_tests.h"
#include "wallet_test_core_proxy.h"
#include "../../src/wallet/wallet_public_structs_defs.h"
#include "offers_helper.h"
#include "string_coding.h"
#include "random_helper.h"
#include "tx_builder.h"

using namespace epee;
using namespace crypto;
using namespace currency;

const uint64_t uint64_max = std::numeric_limits<uint64_t>::max();
const std::wstring g_wallet_filename = L"~coretests.wallet.file.tmp";
const std::string g_wallet_password = "dofatibmzibeziyekigo";
const currency::account_base null_account = AUTO_VAL_INIT(null_account);

//@#@: TODO: need refactoring, unsafe operations
POD_MAKE_COMPARABLE(currency, tx_out_bare);

bool get_stealth_address_form_tx_out_v(const tx_out_v& out_v, crypto::public_key& result)
{
  VARIANT_SWITCH_BEGIN(out_v);
    VARIANT_CASE_CONST(tx_out_bare, out_b);
      VARIANT_SWITCH_BEGIN(out_b.target);
        VARIANT_CASE_CONST(txout_to_key, otk);
          result = otk.key;
          return true;
      VARIANT_SWITCH_END();
    VARIANT_CASE_CONST(tx_out_zarcanum, out_zc);
      result = out_zc.stealth_address;
      return true;
  VARIANT_SWITCH_END();

  return false;
}

// Determines which output is real and actually spent in tx inputs, when there are fake outputs.
bool determine_tx_real_inputs(currency::core& c, const currency::transaction& tx, const currency::account_keys& keys, std::vector<size_t>& real_inputs)
{
  struct local_visitor
  {
    local_visitor(const currency::account_keys& keys, const crypto::key_image key_image)
      : m_keys(keys)
      , m_txin_key_image(key_image)
      , m_output_in_input_index(0)
      , m_found(false)
    {}

    bool handle_output(const transaction& source_tx, const transaction& validated_tx, const tx_out_v& out_v, uint64_t source_tx_output_index)
    {
      CHECK_AND_ASSERT_MES(!m_found, false, "Internal error: m_found is true but the visitor is still being applied");
      /*
      auto is_even = [&](const tx_out_v& v) { return boost::get<tx_out_bare>(v) == out; };
      auto it = std::find_if(validated_tx.vout.begin(), validated_tx.vout.end(), is_even);
      if (it == validated_tx.vout.end())
        return false;
      size_t output_tx_index = it - validated_tx.vout.begin();

      crypto::public_key tx_pub_key = get_tx_pub_key_from_extra(validated_tx);
      crypto::key_derivation derivation;
      bool r = generate_key_derivation(tx_pub_key, m_keys.view_secret_key, derivation);
      CHECK_AND_ASSERT_MES(r, false, "generate_key_derivation failed");
      crypto::secret_key ephemeral_secret_key;
      derive_secret_key(derivation, output_tx_index, m_keys.spend_secret_key, ephemeral_secret_key);

      crypto::public_key output_public_key = boost::get<txout_to_key>(out.target).key;

      /*crypto::public_key ephemeral_public_key;
      derive_public_key(derivation, output_tx_index, m_keys.account_address.spend_public_key, ephemeral_public_key);*/

      /*
      crypto::key_image ki;
      generate_key_image(output_public_key, ephemeral_secret_key, ki);
      s*/

      crypto::key_image ki = AUTO_VAL_INIT(ki);
      crypto::public_key tx_pub_key = get_tx_pub_key_from_extra(validated_tx);
      currency::keypair ephemeral_key = AUTO_VAL_INIT(ephemeral_key);
      currency::generate_key_image_helper(m_keys, tx_pub_key, source_tx_output_index, ephemeral_key, ki);

      crypto::public_key stealth_address = AUTO_VAL_INIT(stealth_address);
      bool r = get_stealth_address_form_tx_out_v(out_v, stealth_address);
      CHECK_AND_ASSERT_MES(r, false, "can't get stealth address from output #" << source_tx_output_index << " tx " << get_transaction_hash(source_tx));
      CHECK_AND_ASSERT_MES(stealth_address == ephemeral_key.pub, false, "derived pub key doesn't match with the stealth address, tx " << get_transaction_hash(source_tx) << " out #" << source_tx_output_index);

      if (ki == m_txin_key_image)
      {
        m_found = true;
        return false; // to break the loop in scan_outputkeys_for_indexes
      }

      ++m_output_in_input_index;
      return true;
    }

    currency::account_keys  m_keys;
    crypto::key_image       m_txin_key_image;
    size_t                  m_output_in_input_index;
    bool                    m_found;
  };

  for (auto& in : tx.vin)
  {
    try
    {
      if (get_key_offsets_from_txin_v(in).size() == 1)
      {
        real_inputs.push_back(0); // trivial case when no mixin is used
        continue;
      }
    }
    catch(...)
    {
      continue;
    }

    crypto::key_image ki = AUTO_VAL_INIT(ki);
    if (!get_key_image_from_txin_v(in, ki))
      continue;

    local_visitor vis(keys, ki);
    bool r = c.get_blockchain_storage().scan_outputkeys_for_indexes(tx, in, vis);
    CHECK_AND_ASSERT_MES(r || vis.m_found, false, "scan_outputkeys_for_indexes failed");
    if (!vis.m_found)
      return false;
    real_inputs.push_back(vis.m_output_in_input_index);
  }

  return true;
}

//------------------------------------------------------------------------------

bool gen_wallet_basic_transfer::generate(std::vector<test_event_entry>& events) const
{
  // Creates and refreshes wallets for a few accounts, makes a transfer via wallet and checks final balances.

  // generate and prepare accounts
  GENERATE_ACCOUNT(miner_acc);
  //const size_t miner_acc_idx = m_accounts.size();
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice);
  const size_t alice_acc_idx = m_accounts.size();
  m_accounts.push_back(alice);
  GENERATE_ACCOUNT(bob);
  const size_t bob_acc_idx = m_accounts.size();
  m_accounts.push_back(bob);

  // don't use MAKE_GENESIS_BLOCK here because it will mask 'generator'
  currency::block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, miner_acc, test_core_time::get_time());
  events.push_back(blk_0);

  // create test wallets for accounts
  CREATE_TEST_WALLET(miner_wlt, miner_acc, blk_0);
  CREATE_TEST_WALLET(alice_wlt, alice, blk_0);
  CREATE_TEST_WALLET(bob_wlt, bob, blk_0);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // send some money to alice...
  MAKE_TX(events, tx_0, miner_acc, alice, MK_TEST_COINS(1000), blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  // ... and to bob
  MAKE_TX(events, tx_1, miner_acc, bob, MK_TEST_COINS(2000), blk_1);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, tx_1);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_2r, blk_2, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // refresh alice's wallet with new blocks
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_2r, CURRENCY_MINED_MONEY_UNLOCK_WINDOW * 2 + 2);

  // check alice's wallet balance both at generation time and at playing back time
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, MK_TEST_COINS(1000));
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(alice_acc_idx, MK_TEST_COINS(1000)));

  // initiate a transfer via alice's wallet
  MAKE_TEST_WALLET_TX(events, tx, alice_wlt, MK_TEST_COINS(100), bob);
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2r, miner_acc, tx);

  // refresh alice's and bob's wallets
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, bob_wlt, blk_3, CURRENCY_MINED_MONEY_UNLOCK_WINDOW * 2 + 3);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_3, 1);

  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, MK_TEST_COINS(900) - TESTS_DEFAULT_FEE);
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(alice_acc_idx, MK_TEST_COINS(900) - TESTS_DEFAULT_FEE));

  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(bob_wlt, MK_TEST_COINS(2100));
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(bob_acc_idx, MK_TEST_COINS(2100)));

  return true;
}

//------------------------------------------------------------------------------

bool gen_wallet_refreshing_on_chain_switch::generate(std::vector<test_event_entry>& events) const
{
  // generate and prepare accounts
  GENERATE_ACCOUNT(miner_acc);
  //const size_t miner_acc_idx = m_accounts.size();
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  const size_t alice_acc_idx = m_accounts.size();
  m_accounts.push_back(alice_acc);

  // don't use MAKE_GENESIS_BLOCK here because it will mask 'generator'
  currency::block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, miner_acc, test_core_time::get_time());
  events.push_back(blk_0);

  //uint64_t premine_amount = get_outs_money_amount(blk_0.miner_tx);
  //uint64_t early_blocks_reward = get_base_block_reward(0, 1, 0);

  // create test wallets for accounts
  CREATE_TEST_WALLET(miner_wlt, miner_acc, blk_0);
  CREATE_TEST_WALLET(alice_wlt, alice_acc, blk_0);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  REFRESH_TEST_WALLET_AT_GEN_TIME(events, miner_wlt, blk_0r, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  
  /*
  // check mined amount
  uint64_t expected_amount = premine_amount + early_blocks_reward * CURRENCY_MINED_MONEY_UNLOCK_WINDOW;
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(miner_wlt, expected_amount);
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(miner_acc_idx, expected_amount, uint64_max, expected_amount));
  */

  MAKE_TEST_WALLET_TX(events, tx_0, miner_wlt, MK_TEST_COINS(2000), alice_acc);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  //  0        10    11    12     <- blockchain height
  // (0 ) ... (0r)- (1 )-
  //                tx_0

  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_1, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, MK_TEST_COINS(2000));
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(alice_acc_idx, MK_TEST_COINS(2000)));

  // alice tries to send some money
  // MAKE_TEST_WALLET_TX(events, tx_1, alice_wlt, MK_TEST_COINS(1500), miner_acc); - can't use wallet because it checks money unlock
  MAKE_TX_FEE(events, tx_1, alice_acc, miner_acc, MK_TEST_COINS(1500), TESTS_DEFAULT_FEE, blk_1);

  // switch the chain
  MAKE_NEXT_BLOCK(events, blk_1a, blk_0r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_2a, blk_1a, miner_acc);

  //  0   ...  10    11    12     <- blockchain height
  // (0 ) ... (0r)- (1 )-
  //           \    tx_0  tx_1    <- txs (tx_1 in tx pool, not included; than both moved to the pool as chain switches)
  //            \                                        .
  //             \- (1a)- (2a)

  // Well, this is kind of a very unlikely case: the wallet prevents spending very recently received money, but the core allows such txs.
  // So tx_1 is spending the money that Alice receives via tx_0.
  // Upon chain switching both txs are moved to tx pool, where they are handled by wallet::scan_tx_pool
  // Unfortunately, the wallet can't determine, that tx_1 is spending its own output, so this money is not counting in awaiging_out
  // So in theory we should have: total = 500 - fee,        unlocked = 0, mined = 0, awaiting_in = 2000 + (500 - fee), awaiting_out = 2000
  // Actually we have:            total = 2000 + 500 - fee, unlocked = 0, mined = 0, awaiting_in = 2000 + (500 - fee), awaiting_out = 0

  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_2a, 2);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, MK_TEST_COINS(2500) - TESTS_DEFAULT_FEE);
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(alice_acc_idx, uint64_max, 0, 0, MK_TEST_COINS(2500) - TESTS_DEFAULT_FEE, 0));

  // make sure her transaction is not valid now
  DO_CALLBACK(events, "mark_invalid_block");
  MAKE_NEXT_BLOCK_TX1(events, blk_3a, blk_2a, miner_acc, tx_1);

  // switch the chain back and make sure tx_1 now can be added to a block
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_1);

  //  0   ...  10    11    12          <- blockchain height
  // (0 ) ... (0r)- (1 )- (2 )- (3 )
  //             \- (1a)- (2a)

  // refresh the wallet and make sure it's ok with correct balance
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_3, 3);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, MK_TEST_COINS(500) - TESTS_DEFAULT_FEE);
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(alice_acc_idx, MK_TEST_COINS(500) - TESTS_DEFAULT_FEE));

  return true;
}

//------------------------------------------------------------------------------

bool gen_wallet_refreshing_on_chain_switch_2::generate(std::vector<test_event_entry>& events) const
{
  // generate and prepare accounts
  GENERATE_ACCOUNT(miner_acc);
  //const size_t miner_acc_idx = m_accounts.size();
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  const size_t alice_acc_idx = m_accounts.size();
  m_accounts.push_back(alice_acc);

  // don't use MAKE_GENESIS_BLOCK here because it will mask 'generator'
  currency::block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, miner_acc, test_core_time::get_time());
  events.push_back(blk_0);                                                                                          // 0 (event index)

  // create test wallets for accounts
  CREATE_TEST_WALLET(miner_wlt, miner_acc, blk_0);
  CREATE_TEST_WALLET(alice_wlt, alice_acc, blk_0);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);                  // 2N (N = CURRENCY_MINED_MONEY_UNLOCK_WINDOW == 10)

  // send some money to alice                                                                                       
  MAKE_TX_FEE(events, tx_0, miner_acc, alice_acc, MK_TEST_COINS(2000), TESTS_DEFAULT_FEE, blk_0r);                // 2N+1
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);                                                      // 2N+1

  MAKE_TX_FEE(events, tx_1, miner_acc, alice_acc, MK_TEST_COINS(500), TESTS_DEFAULT_FEE, blk_0r);                 // 2N+3
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, tx_1);                                                       // 2N+4

  //  0        10    11    12     <- blockchain height
  // (0 ) ... (0r)- (1 )- (2 )
  //                tx_0  tx_1    <- transactions

  // alice refreshes her wallet
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_2, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2);

  // and sees 2500 test coins
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME_FULL(alice_wlt, MK_TEST_COINS(2500), 0, 0, 0, 0);

  // than chain switch happens
  MAKE_NEXT_BLOCK(events, blk_2a, blk_1, miner_acc);                                                                // 2N+5
  MAKE_NEXT_BLOCK(events, blk_3a, blk_2a, miner_acc);                                                               // 2N+6

  //  0        10    11    12    13   <- blockchain height
  // (0 ) ... (0r)- (1 )- (2 )
  //                tx_0              <- tx_1 should be removed from blockchain and wallet
  //                   \- (2a)- (3a)  <- new chain

  // alice refreshes her wallet again
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_3a, 2);

  // there should only one transfer left in transfers history
  CHECK_AND_ASSERT_MES(alice_wlt->get_recent_transfers_total_count() == 1, false, "Invalid recent transfers count");

  // and now she has only 2000 test coins from tx_0, wich remains unaffected by chain switching, plus 500 as awaiting_in
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME_FULL(alice_wlt, MK_TEST_COINS(2500), 0, 0, MK_TEST_COINS(500), 0);
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(alice_acc_idx, MK_TEST_COINS(2500), 0, 0, MK_TEST_COINS(500), 0));            // 2N+7

  // finally, huge chainswitch has come out of the blue...
  MAKE_NEXT_BLOCK(events, blk_1b, blk_0r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_2b, blk_1b, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_3b, blk_2b, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_4b, blk_3b, miner_acc);

  //  0        10    11    12    13    14    <- blockchain height
  // (0 ) ... (0r)- (1 )- (2 )
  //                tx_0                     
  //                   \- (2a)- (3a)         
  //             \- (1b)- (2b)- (3b)- (4b)   <- new chain

  // alice refreshes her wallet again...
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_4b, 4);

  // ...and sees no money at all (but 2500 in awaiting in)
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME_FULL(alice_wlt, MK_TEST_COINS(2500), 0, 0, MK_TEST_COINS(2500), 0);
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(alice_acc_idx, MK_TEST_COINS(2500), 0, 0, MK_TEST_COINS(2500), 0));

  // and no transfers
  CHECK_AND_ASSERT_MES(alice_wlt->get_recent_transfers_total_count() == 0, false, "Invalid recent transfers count");


  return true;
}

//------------------------------------------------------------------------------

bool gen_wallet_unconfirmed_tx_from_tx_pool::generate(std::vector<test_event_entry>& events) const
{
  // generate and prepare accounts
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);

  // don't use MAKE_GENESIS_BLOCK here because it will mask 'generator'
  currency::block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, miner_acc, test_core_time::get_time());
  events.push_back(blk_0);

  // create test wallets for accounts
  CREATE_TEST_WALLET(miner_wlt, miner_acc, blk_0);
  CREATE_TEST_WALLET(alice_wlt, alice_acc, blk_0);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // prepare a tx with money for alice
  MAKE_TX_FEE(events, tx_0, miner_acc, alice_acc, MK_TEST_COINS(2000), TESTS_DEFAULT_FEE, blk_0r);

  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_0r, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  //bool has_aliases = false;
  //alice_wlt->scan_tx_pool(has_aliases);

  if (!check_balance_via_wallet(*alice_wlt.get(), "alice", MK_TEST_COINS(2000), 0, 0, MK_TEST_COINS(2000), 0))
    return false;

  // put the transaction into a block
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  // recheck the balance
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_1, 1);
  //alice_wlt->scan_tx_pool(has_aliases);
  if (!check_balance_via_wallet(*alice_wlt.get(), "alice", MK_TEST_COINS(2000), 0, 0, 0, 0))
    return false;

  // rewind few blocks to unlock the money
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  // recheck the balance
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_1r, WALLET_DEFAULT_TX_SPENDABLE_AGE);
  //alice_wlt->scan_tx_pool(has_aliases);
  if (!check_balance_via_wallet(*alice_wlt.get(), "alice", MK_TEST_COINS(2000), 0, MK_TEST_COINS(2000), 0, 0))
    return false;

  return true;
}

//------------------------------------------------------------------------------

gen_wallet_save_load_and_balance::gen_wallet_save_load_and_balance()
{
  REGISTER_CALLBACK_METHOD(gen_wallet_save_load_and_balance, c1_check_balance_and_store);
  REGISTER_CALLBACK_METHOD(gen_wallet_save_load_and_balance, c2);
  REGISTER_CALLBACK_METHOD(gen_wallet_save_load_and_balance, c3_load_refresh_check_balance);
}

bool gen_wallet_save_load_and_balance::generate(std::vector<test_event_entry>& events) const
{
  // Does simple money transfers and check balances both at generation and playing.
  // Stores/loads the wallet on play time between checks.

  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);

  // don't use MAKE_GENESIS_BLOCK here because it will mask 'generator'
  currency::block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, miner_acc, test_core_time::get_time());
  events.push_back(blk_0);

  DO_CALLBACK(events, "configure_core");

  // create gen-time test wallets for accounts
  CREATE_TEST_WALLET(miner_wlt, miner_acc, blk_0);
  CREATE_TEST_WALLET(alice_wlt, alice_acc, blk_0);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // prepare a tx with money for alice
  MAKE_TX_FEE(events, tx_0, miner_acc, alice_acc, MK_TEST_COINS(2000), TESTS_DEFAULT_FEE, blk_0r);

  // check balance based on gen-time data
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_0r, 10);
  bool has_aliases;
  alice_wlt->scan_tx_pool(has_aliases);
  if (!check_balance_via_wallet(*alice_wlt.get(), "alice", MK_TEST_COINS(2000), 0, 0, MK_TEST_COINS(2000), 0))
    return false;

  // check balance on play-time, then store the wallet into a file
  DO_CALLBACK(events, "c1_check_balance_and_store");

  // put the transaction into a block
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  //  0       10      11      21     23    <- blockchain height (assuming CURRENCY_MINED_MONEY_UNLOCK_WINDOW == 10)
  // (0 )... (0r)-   (1 )... (1r)-         <- main chain
  //                 tx_0                  <- txs
  //            \-   (2 )...........(2r)   <- alt chain

  // rewind few blocks to unlock the money
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  // recheck balance based on gen-time data
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_1r, 11);
  alice_wlt->scan_tx_pool(has_aliases);
  if (!check_balance_via_wallet(*alice_wlt.get(), "alice", MK_TEST_COINS(2000), 0, MK_TEST_COINS(2000), 0, 0))
    return false;

  // load wallet from file, re-chech the balance, store, load again, re-reck again, make tx, make block, check balance, store wallet
  DO_CALLBACK(events, "c2");

  // switch the chain, reload and re-check the wallet
  REWIND_BLOCKS_N_WITH_TIME(events, blk_2r, blk_0r, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE + 3);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_2r, WALLET_DEFAULT_TX_SPENDABLE_AGE + 3);
  alice_wlt->scan_tx_pool(has_aliases);
  if (!check_balance_via_wallet(*alice_wlt.get(), "alice", MK_TEST_COINS(2000), 0, 0, MK_TEST_COINS(2000), 0)) // tx_0 moved to the pool, so its money in "awaiting_in" bucket
    return false;
  DO_CALLBACK(events, "c3_load_refresh_check_balance");

  return true;
}

bool gen_wallet_save_load_and_balance::c1_check_balance_and_store(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW, false, "Incorrect numbers of blocks fetched");

  bool has_alias;
  alice_wlt->scan_tx_pool(has_alias);

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", MK_TEST_COINS(2000), 0, 0, MK_TEST_COINS(2000), 0), false, "");

  alice_wlt->reset_password(g_wallet_password);
  alice_wlt->store(g_wallet_filename);

  return true;
}

bool gen_wallet_save_load_and_balance::c2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wlt(new tools::wallet2);
  alice_wlt->load(g_wallet_filename, g_wallet_password);
  alice_wlt->set_core_proxy(m_core_proxy);
  alice_wlt->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());

  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == WALLET_DEFAULT_TX_SPENDABLE_AGE + 1, false, "Incorrect numbers of blocks fetched");

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", MK_TEST_COINS(2000), 0, MK_TEST_COINS(2000), 0, 0), false, "");
  alice_wlt->store(g_wallet_filename);

  ////// save - load ////////

  alice_wlt.reset(new tools::wallet2);
  alice_wlt->load(g_wallet_filename, g_wallet_password);
  alice_wlt->set_core_proxy(m_core_proxy);
  alice_wlt->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", MK_TEST_COINS(2000), 0, MK_TEST_COINS(2000), 0, 0), false, "");

  alice_wlt->transfer(MK_TEST_COINS(1000) - TESTS_DEFAULT_FEE, m_accounts[MINER_ACC_IDX].get_public_address());

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", MK_TEST_COINS(1000), 0, 0, 0, MK_TEST_COINS(1000) - TESTS_DEFAULT_FEE), false, "");

  bool r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Tx pool is not empty: " << c.get_pool_transactions_count());

  alice_wlt->store(g_wallet_filename);

  return true;
}

bool gen_wallet_save_load_and_balance::c3_load_refresh_check_balance(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wlt(new tools::wallet2);
  alice_wlt->load(g_wallet_filename, g_wallet_password);
  alice_wlt->set_core_proxy(m_core_proxy);
  alice_wlt->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());

  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == WALLET_DEFAULT_TX_SPENDABLE_AGE + 3, false, "Incorrect numbers of blocks fetched");

  bool has_alias;
  alice_wlt->scan_tx_pool(has_alias);

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", MK_TEST_COINS(3000), 0, 0, MK_TEST_COINS(2000), MK_TEST_COINS(1000) - TESTS_DEFAULT_FEE), false, "");

  return true;
}

//------------------------------------------------------------------------------

gen_wallet_mine_pos_block::gen_wallet_mine_pos_block()
{
  REGISTER_CALLBACK_METHOD(gen_wallet_mine_pos_block, c1);
}

bool gen_wallet_mine_pos_block::generate(std::vector<test_event_entry>& events) const
{

  //  0       10      11      21     22    <- blockchain height (assuming CURRENCY_MINED_MONEY_UNLOCK_WINDOW == 10)
  // (0 )... (0r)-   (1 )... (1r)-         <- main chain
  //                 tx_0                  <- txs

  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);

  // don't use MAKE_GENESIS_BLOCK here because it will mask 'generator'
  currency::block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, miner_acc, test_core_time::get_time());
  events.push_back(blk_0);

  DO_CALLBACK(events, "configure_core");

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  MAKE_TX_FEE(events, tx_0, miner_acc, alice_acc, MK_TEST_COINS(2000), TESTS_DEFAULT_FEE, blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

bool gen_wallet_mine_pos_block::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + WALLET_DEFAULT_TX_SPENDABLE_AGE + 1, false, "Incorrect numbers of blocks fetched");

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", MK_TEST_COINS(2000), 0, MK_TEST_COINS(2000), 0, 0), false, "");

  alice_wlt->try_mint_pos();

  CHECK_AND_ASSERT_MES(c.get_current_blockchain_size() == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + WALLET_DEFAULT_TX_SPENDABLE_AGE + 3, false, "Incorrect blockchain height:" << c.get_current_blockchain_size());
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 1, false, "Incorrect numbers of blocks fetched");

  block top_block = AUTO_VAL_INIT(top_block);
  bool r = c.get_blockchain_storage().get_top_block(top_block);
  CHECK_AND_ASSERT_MES(r && is_pos_block(top_block), false, "get_top_block failed or smth goes wrong");
  uint64_t top_block_reward = get_outs_money_amount(top_block.miner_tx);
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", top_block_reward, top_block_reward - MK_TEST_COINS(2000), 0, 0, 0), false, "");

  alice_wlt->reset_password(g_wallet_password);
  alice_wlt->store(g_wallet_filename);

  return true;
}

//------------------------------------------------------------------------------

gen_wallet_unconfirmed_outdated_tx::gen_wallet_unconfirmed_outdated_tx()
{
  REGISTER_CALLBACK_METHOD(gen_wallet_unconfirmed_outdated_tx, c1);
  REGISTER_CALLBACK_METHOD(gen_wallet_unconfirmed_outdated_tx, c2);
  REGISTER_CALLBACK_METHOD(gen_wallet_unconfirmed_outdated_tx, c3);
}

bool gen_wallet_unconfirmed_outdated_tx::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);

  // don't use MAKE_GENESIS_BLOCK here because it will mask 'generator'
  currency::block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, miner_acc, test_core_time::get_time());
  events.push_back(blk_0);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  // create tx_0 like it's 20 sec after blk_0r
  events.push_back(event_core_time(blk_0r.timestamp + 20));
  MAKE_TX_FEE(events, tx_0, miner_acc, alice_acc, MK_TEST_COINS(200), TESTS_DEFAULT_FEE, blk_0r);

  // check balances and populate unconfirmed tx list in a wallet
  DO_CALLBACK(events, "c1");

  // create tx_1 like it's 25 sec after blk_0r
  events.push_back(event_core_time(blk_0r.timestamp + 25));
  MAKE_TX_FEE(events, tx_1, miner_acc, alice_acc, MK_TEST_COINS(500), TESTS_DEFAULT_FEE, blk_0r);

  // check balances and populate unconfirmed tx list in a wallet
  DO_CALLBACK(events, "c2");

  // add a block with no txs, so the next refresh in the wallet triggers unconfirmed txs cleaning
  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, miner_acc);

  // forward the time a lot, tx_0 should now be beyound the allowed livetime limit
  events.push_back(event_core_time(blk_0r.timestamp + 23 + CURRENCY_MEMPOOL_TX_LIVETIME));

  // check again
  DO_CALLBACK(events, "c3");

  return true;
}

bool gen_wallet_unconfirmed_outdated_tx::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  size_t blocks_fetched = 0;
  bool received_money;
  bool has_alias;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1, false, "Incorrect numbers of blocks fetched");
  alice_wlt->scan_tx_pool(has_alias);

  if (!check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", MK_TEST_COINS(200), 0, 0, MK_TEST_COINS(200), 0))
    return false;

  std::vector<tools::wallet_public::wallet_transfer_info> trs;
  alice_wlt->get_unconfirmed_transfers(trs);
  CHECK_AND_ASSERT_MES(trs.size() == 1, false, "Incorrect num of unconfirmed tx");

  alice_wlt->reset_password(g_wallet_password);
  alice_wlt->store(g_wallet_filename);
  return true;
}

bool gen_wallet_unconfirmed_outdated_tx::c2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wlt(new tools::wallet2);
  alice_wlt->load(g_wallet_filename, g_wallet_password);
  alice_wlt->set_core_proxy(m_core_proxy);
  alice_wlt->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());

  size_t blocks_fetched = 0;
  bool received_money;
  bool has_alias;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  alice_wlt->scan_tx_pool(has_alias);

  CHECK_AND_ASSERT_MES(blocks_fetched == 0, false, "Incorrect numbers of blocks fetched");

  if (!check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", MK_TEST_COINS(700), 0, 0, MK_TEST_COINS(700), 0))
    return false;

  std::vector<tools::wallet_public::wallet_transfer_info> trs;
  alice_wlt->get_unconfirmed_transfers(trs);
  CHECK_AND_ASSERT_MES(trs.size() == 2, false, "Incorrect num of unconfirmed tx");
  CHECK_AND_ASSERT_MES(trs[0].timestamp != trs[1].timestamp, false, "wallet set the same timestamp for unconfirmed txs which came not simultaneously");

  alice_wlt->reset_password(g_wallet_password);
  alice_wlt->store(g_wallet_filename);
  return true;
}

bool gen_wallet_unconfirmed_outdated_tx::c3(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wlt(new tools::wallet2);
  alice_wlt->load(g_wallet_filename, g_wallet_password);
  alice_wlt->set_core_proxy(m_core_proxy);
  alice_wlt->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());

  size_t blocks_fetched = 0;
  bool received_money;
  bool has_alias;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  alice_wlt->scan_tx_pool(has_alias);

  CHECK_AND_ASSERT_MES(blocks_fetched == 1, false, "Incorrect numbers of blocks fetched");

  if (!check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", MK_TEST_COINS(500), 0, 0, MK_TEST_COINS(500), 0))
    return false;

  std::vector<tools::wallet_public::wallet_transfer_info> trs;
  alice_wlt->get_unconfirmed_transfers(trs);
  CHECK_AND_ASSERT_MES(trs.size() == 1, false, "Incorrect num of unconfirmed tx");

  return true;
}

//------------------------------------------------------------------------------

gen_wallet_unlock_by_block_and_by_time::gen_wallet_unlock_by_block_and_by_time()
{
  REGISTER_CALLBACK_METHOD(gen_wallet_unlock_by_block_and_by_time, c1);
  REGISTER_CALLBACK_METHOD(gen_wallet_unlock_by_block_and_by_time, c2);
  REGISTER_CALLBACK_METHOD(gen_wallet_unlock_by_block_and_by_time, c3);
  REGISTER_CALLBACK_METHOD(gen_wallet_unlock_by_block_and_by_time, c4);
}

bool gen_wallet_unlock_by_block_and_by_time::generate(std::vector<test_event_entry>& events) const
{
  // Test outline:
  // 1) creates two txs both having unlock_time set to non-zero (few blocks further for the first one and some time in future for the second one);
  // 2) puts these txs into the blockchain;
  // 3) checks balalce and verifies unability to spend money from both of locked tx;
  // 4) makes few blocks to unlock the first tx;
  // 5) spends money form the first tx, checks balance and so..
  // 6) verifies unability to spens money from the second tx that is still locked;
  // 7) moves the time forward to unlock it;
  // 8) spends the money and checks balances.

  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);

  // don't use MAKE_GENESIS_BLOCK here because it will mask 'generator'
  currency::block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, preminer_acc, test_core_time::get_time());
  events.push_back(blk_0);                                                                                                   // 0 (event index)

  // unlock the money
  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);                       // 2N+2 (N = CURRENCY_MINED_MONEY_UNLOCK_WINDOW)

  CREATE_TEST_WALLET(miner_wlt, miner_acc, blk_0);
  CREATE_TEST_WALLET(alice_wlt, alice_acc, blk_0);

  REFRESH_TEST_WALLET_AT_GEN_TIME(events, miner_wlt, blk_0r, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  
  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  std::vector<tx_destination_entry> destinations(
    {
      { MK_TEST_COINS(10), alice_acc.get_public_address() },
      { MK_TEST_COINS(10), alice_acc.get_public_address() },
      { MK_TEST_COINS(10), alice_acc.get_public_address() }
    }
  );
  
  // create two txs (2x 3000 test coins to Alice) both having non-zero unlock_time
  uint64_t unlock_block_num = CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2 + WALLET_DEFAULT_TX_SPENDABLE_AGE + 2;
  uint64_t unlock_time = blk_0.timestamp + 10000;
  miner_wlt->transfer(destinations, 0, unlock_block_num, TESTS_DEFAULT_FEE, std::vector<extra_v>(), std::vector<attachment_v>(), tx_0);
  miner_wlt->transfer(destinations, 0, unlock_time, TESTS_DEFAULT_FEE, std::vector<extra_v>(), std::vector<attachment_v>(), tx_1);
  events.push_back(tx_0);
  events.push_back(tx_1);

  // put them into a block
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({tx_0, tx_1}));
  
  // rewind WALLET_DEFAULT_TX_SPENDABLE_AGE blocks to be able to spend txs outputs 
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  // check balances and make sure we can't spend the money now
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_1r, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2 + WALLET_DEFAULT_TX_SPENDABLE_AGE);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME_FULL(alice_wlt, MK_TEST_COINS(60), 0, 0, 0, 0);
  bool r = false;
  try
  {
    MAKE_TEST_WALLET_TX(events, tx_a, alice_wlt, MK_TEST_COINS(29), miner_acc);
  }
  catch (const tools::error::not_enough_money&)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "Expected error 'not enought money' wasn't caught");
  DO_CALLBACK(events, "c1");

  // make a block, this should unlock the first tx
  MAKE_NEXT_BLOCK(events, blk_2, blk_1r, miner_acc);

  // check balance, spend to money, make sure it was spent
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_2, 1);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME_FULL(alice_wlt, MK_TEST_COINS(60), 0, MK_TEST_COINS(30), 0, 0); // 3000 became unlocked
  MAKE_TEST_WALLET_TX(events, tx_b, alice_wlt, MK_TEST_COINS(29), miner_acc);
  DO_CALLBACK(events, "c2");

  // move the time to the moment just 3 seconds before the second tx gets unloccked
  test_core_time::adjust(unlock_time - CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS - 3);
  events.push_back(event_core_time(unlock_time - CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS - 3));

  // check the balance, make sure the tx is still locked and money can't be spent
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_2, 0);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME_FULL(alice_wlt, MK_TEST_COINS(60 - 29) - TESTS_DEFAULT_FEE, 0, 0, 0, MK_TEST_COINS(29));

  r = false;
  try
  {
    MAKE_TEST_WALLET_TX(events, tx_c, alice_wlt, MK_TEST_COINS(29), miner_acc);
  }
  catch (const tools::error::not_enough_money&)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "Expected error 'not enought money' wasn't caught");
  DO_CALLBACK(events, "c3");

  // move the time to the moment the second tx should be unlocked
  test_core_time::adjust(unlock_time - CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS);
  events.push_back(event_core_time(unlock_time - CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS));
  
  // check balance
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_2, 0);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME_FULL(alice_wlt, MK_TEST_COINS(60 - 29) - TESTS_DEFAULT_FEE, 0, MK_TEST_COINS(30), 0, MK_TEST_COINS(29));

  // make sure the money can be spent now, put a transaction
  MAKE_TEST_WALLET_TX(events, tx_d, alice_wlt, MK_TEST_COINS(29), miner_acc);
  
  // final checks
  DO_CALLBACK(events, "c4");

  return true;
}

bool gen_wallet_unlock_by_block_and_by_time::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  size_t blocks_fetched = 0;
  bool received_money, has_alias;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2 + WALLET_DEFAULT_TX_SPENDABLE_AGE, false, "Incorrect numbers of blocks fetched");
  alice_wlt->scan_tx_pool(has_alias);

  if (!check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", MK_TEST_COINS(60), 0, 0, 0, 0))
    return false;

  bool r = false;
  try
  {
    alice_wlt->transfer(std::vector<tx_destination_entry>({ { MK_TEST_COINS(29), m_accounts[MINER_ACC_IDX].get_public_address() } }), 0, 0, TESTS_DEFAULT_FEE, std::vector<extra_v>(), std::vector<attachment_v>());
  }
  catch (const tools::error::not_enough_money&)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "Expected error 'not enought money' wasn't caught");

  alice_wlt->reset_password(g_wallet_password);
  alice_wlt->store(g_wallet_filename);
  return true;
}

bool gen_wallet_unlock_by_block_and_by_time::c2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wlt(new tools::wallet2);
  alice_wlt->load(g_wallet_filename, g_wallet_password);
  alice_wlt->set_core_proxy(m_core_proxy);
  alice_wlt->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());

  size_t blocks_fetched = 0;
  bool received_money, has_alias;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 1, false, "Incorrect numbers of blocks fetched");
  alice_wlt->scan_tx_pool(has_alias);

  if (!check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", MK_TEST_COINS(60 - 29) - TESTS_DEFAULT_FEE, 0, 0, 0, MK_TEST_COINS(29)))
    return false;

  alice_wlt->reset_password(g_wallet_password);
  alice_wlt->store(g_wallet_filename);
  return true;
}

bool gen_wallet_unlock_by_block_and_by_time::c3(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wlt(new tools::wallet2);
  alice_wlt->load(g_wallet_filename, g_wallet_password);
  alice_wlt->set_core_proxy(m_core_proxy);
  alice_wlt->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());

  size_t blocks_fetched = 0;
  bool received_money, has_alias;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 0, false, "Incorrect numbers of blocks fetched");
  alice_wlt->scan_tx_pool(has_alias);

  if (!check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", MK_TEST_COINS(60 - 29) - TESTS_DEFAULT_FEE, 0, 0, 0, MK_TEST_COINS(29)))
    return false;

  bool r = false;
  try
  {
    alice_wlt->transfer(std::vector<tx_destination_entry>({ { MK_TEST_COINS(29), m_accounts[MINER_ACC_IDX].get_public_address() } }), 0, 0, TESTS_DEFAULT_FEE, std::vector<extra_v>(), std::vector<attachment_v>());
  }
  catch (const tools::error::not_enough_money&)
  {
    r = true;
  }
  CHECK_AND_ASSERT_MES(r, false, "Expected error 'not enought money' wasn't caught");

  alice_wlt->reset_password(g_wallet_password);
  alice_wlt->store(g_wallet_filename);
  return true;
}

bool gen_wallet_unlock_by_block_and_by_time::c4(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wlt(new tools::wallet2);
  alice_wlt->load(g_wallet_filename, g_wallet_password);
  alice_wlt->set_core_proxy(m_core_proxy);
  alice_wlt->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());

  size_t blocks_fetched = 0;
  bool received_money, has_alias;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 0, false, "Incorrect numbers of blocks fetched");
  alice_wlt->scan_tx_pool(has_alias);

  if (!check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", MK_TEST_COINS(60 - 29 - 29) - TESTS_DEFAULT_FEE * 2, 0, 0, 0, MK_TEST_COINS(29 + 29)))
    return false;

  alice_wlt->reset_password(g_wallet_password);
  alice_wlt->store(g_wallet_filename);
  return true;
}

//------------------------------------------------------------------------------

gen_wallet_payment_id::gen_wallet_payment_id()
{
  REGISTER_CALLBACK_METHOD(gen_wallet_payment_id, c1);
  REGISTER_CALLBACK_METHOD(gen_wallet_payment_id, c2);
}

bool gen_wallet_payment_id::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);

  // don't use MAKE_GENESIS_BLOCK here because it will mask 'generator'
  currency::block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, preminer_acc, test_core_time::get_time());
  events.push_back(blk_0);                                                                                                   // 0 (event index)

  // unlock the money
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);                                     // N (N = CURRENCY_MINED_MONEY_UNLOCK_WINDOW)

  MAKE_TX(events, tx_0, miner_acc, alice_acc, MK_TEST_COINS(40), blk_0r);                                                    // N+1

  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);                                                               // N+2

  m_payment_id = epee::string_tools::pod_to_hex(get_block_hash(blk_0)); // use genesis hash as payment id for instance
  std::vector<currency::attachment_v> attachment;
  bool r = currency::set_payment_id_to_tx(attachment, m_payment_id);
  CHECK_AND_ASSERT_MES(r, false, "set_payment_id_to_tx failed");
  MAKE_TX_ATTACH(events, tx_1, miner_acc, alice_acc, MK_TEST_COINS(5), blk_1, attachment);                                   // N+3
  MAKE_TX_ATTACH(events, tx_2, miner_acc, alice_acc, MK_TEST_COINS(7), blk_1, attachment);                                   // N+4
  MAKE_TX_ATTACH(events, tx_3, alice_acc, miner_acc, MK_TEST_COINS(10), blk_1, attachment);                                  // N+5

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1, miner_acc, std::list<transaction>({ tx_1, tx_2, tx_3 }));

  //  0     10    11    12                           <- blockchain height
  // (0 )- (0r)- (1 )- (2 )-                         <- blocks
  //       tx_0        tx_1                          <- txs
  //                   tx_2
  //                   tx_3

  // make a tx with empty string as payment_id
  MAKE_TX_ATTACH(events, tx_4, miner_acc, alice_acc, MK_TEST_COINS(13), blk_2, attachment);
  std::vector<currency::attachment_v> attachment_empty_payment_id;
  r = currency::set_payment_id_to_tx(attachment_empty_payment_id, "");
  CHECK_AND_ASSERT_MES(r, false, "set_payment_id_to_tx_extra failed");
  MAKE_TX_ATTACH(events, tx_5, miner_acc, alice_acc, MK_TEST_COINS(1), blk_2, attachment_empty_payment_id);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3, blk_2, miner_acc, std::list<transaction>({ tx_4, tx_5 }));

  //  0     10    11    12     13                    <- blockchain height
  // (0 )- (0r)- (1 )- (2 )-  (3 )                   <- blocks
  //       tx_0        tx_1   tx_4                   <- txs
  //                   tx_2   tx_5
  //                   tx_3


  CREATE_TEST_WALLET(alice_wlt, alice_acc, blk_0);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_3, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, MK_TEST_COINS(40 + 5 + 7 - 10 + 13 + 1) - TESTS_DEFAULT_FEE);
  DO_CALLBACK_PARAMS(events, "check_balance", params_check_balance(ALICE_ACC_IDX, MK_TEST_COINS(40 + 5 + 7 - 10 + 13 + 1) - TESTS_DEFAULT_FEE));

  std::list<tools::payment_details> payments;
  alice_wlt->get_payments(m_payment_id, payments);
  CHECK_AND_ASSERT_MES(payments.size() == 3, false, "Invalid payments count");
  payments.sort([](const tools::payment_details& lhs, const tools::payment_details& rhs) -> bool { return lhs.m_amount < rhs.m_amount; } ); // sort payments by amount to order them for further checks
  CHECK_AND_ASSERT_MES(payments.front().m_amount == MK_TEST_COINS(5) && payments.front().m_tx_hash == get_transaction_hash(tx_1), false, "Invalid payments #1");
  CHECK_AND_ASSERT_MES(payments.back().m_amount == MK_TEST_COINS(13) && payments.back().m_tx_hash == get_transaction_hash(tx_4), false, "Invalid payments #3");
  payments.clear();
  alice_wlt->get_payments("", payments);
  CHECK_AND_ASSERT_MES(payments.empty(), false, "Nonzero payments returned by \"\"");

  DO_CALLBACK(events, "c1");


  // switch the blockchain
  MAKE_NEXT_BLOCK(events, blk_3a, blk_2, miner_acc);
  MAKE_TX_ATTACH(events, tx_6, miner_acc, alice_acc, MK_TEST_COINS(9), blk_3a, attachment);
  MAKE_NEXT_BLOCK_TX1(events, blk_4a, blk_3a, miner_acc, tx_6);

  //  0     10    11    12     13    14              <- blockchain height
  // (0 )- (0r)- (1 )- (2 )-  (3 )                   <- blocks in altchain (after switching)
  //       tx_0        tx_1   tx_4                   <- txs
  //                   tx_2   tx_5
  //                   tx_3
  //                      \-  (3a)-  (4a)            <- main chain (after switching)
  //                                 tx_6


  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_4a, 2);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME_FULL(alice_wlt, MK_TEST_COINS(40 + 5 + 7 - 10 + 13 + 1 + 9) - TESTS_DEFAULT_FEE, 0, 0, MK_TEST_COINS(13 + 1), 0);
  payments.clear();
  alice_wlt->get_payments(m_payment_id, payments);
  CHECK_AND_ASSERT_MES(payments.size() == 3, false, "Invalid payments count (2)");
  payments.sort([](const tools::payment_details& lhs, const tools::payment_details& rhs) -> bool { return lhs.m_amount < rhs.m_amount; } ); // sort payments by amount to order them for further checks
  CHECK_AND_ASSERT_MES(payments.front().m_amount == MK_TEST_COINS(5) && payments.front().m_tx_hash == get_transaction_hash(tx_1), false, "Invalid payments #1");
  CHECK_AND_ASSERT_MES(payments.back().m_amount == MK_TEST_COINS(9) && payments.back().m_tx_hash == get_transaction_hash(tx_6), false, "Invalid payments #3");

  DO_CALLBACK(events, "c2");

  return true;
}

bool gen_wallet_payment_id::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  size_t blocks_fetched = 0;
  bool received_money, has_alias;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3, false, "Incorrect numbers of blocks fetched");
  alice_wlt->scan_tx_pool(has_alias);

  if (!check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", MK_TEST_COINS(56) - TESTS_DEFAULT_FEE, 0, 0, 0, 0))
    return false;

  std::list<tools::payment_details> payments;
  alice_wlt->get_payments(m_payment_id, payments);
  CHECK_AND_ASSERT_MES(payments.size() == 3, false, "Invalid payments count (4)");
  payments.sort([](const tools::payment_details& lhs, const tools::payment_details& rhs) -> bool { return lhs.m_amount < rhs.m_amount; } ); // sort payments by amount to order them for further checks
  CHECK_AND_ASSERT_MES(payments.front().m_amount == MK_TEST_COINS(5), false, "Invalid payments #1");
  CHECK_AND_ASSERT_MES(payments.back().m_amount == MK_TEST_COINS(13), false, "Invalid payments #3");

  alice_wlt->reset_password(g_wallet_password);
  alice_wlt->store(g_wallet_filename);
  return true;

}

bool gen_wallet_payment_id::c2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wlt(new tools::wallet2);
  alice_wlt->load(g_wallet_filename, g_wallet_password);
  alice_wlt->set_core_proxy(m_core_proxy);
  alice_wlt->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());

  size_t blocks_fetched = 0;
  bool received_money, has_alias;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 2, false, "Incorrect numbers of blocks fetched");
  alice_wlt->scan_tx_pool(has_alias);

  if (!check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", MK_TEST_COINS(40 + 5 + 7 - 10 + 9 + 13 + 1) - TESTS_DEFAULT_FEE, 0, 0, MK_TEST_COINS(13 + 1), 0))
    return false;


  std::list<tools::payment_details> payments;
  alice_wlt->get_payments(m_payment_id, payments);
  CHECK_AND_ASSERT_MES(payments.size() == 3, false, "Invalid payments count (4)");
  payments.sort([](const tools::payment_details& lhs, const tools::payment_details& rhs) -> bool { return lhs.m_amount < rhs.m_amount; } ); // sort payments by amount to order them for further checks
  CHECK_AND_ASSERT_MES(payments.front().m_amount == MK_TEST_COINS(5), false, "Invalid payments #1");
  CHECK_AND_ASSERT_MES(payments.back().m_amount == MK_TEST_COINS(9), false, "Invalid payments #3");

  return true;
}

//------------------------------------------------------------------------------

gen_wallet_oversized_payment_id::gen_wallet_oversized_payment_id()
{
  REGISTER_CALLBACK_METHOD(gen_wallet_oversized_payment_id, c1);
  //REGISTER_CALLBACK_METHOD(gen_wallet_payment_id, c2);
}

bool gen_wallet_oversized_payment_id::generate(std::vector<test_event_entry>& events) const
{
  // 

  GENERATE_ACCOUNT(preminer_acc);
  m_accounts.resize(TOTAL_ACCS_COUNT);
  auto& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  auto& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();

  currency::block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, preminer_acc, test_core_time::get_time());
  events.push_back(blk_0);

  // unlock some money
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  DO_CALLBACK(events, "c1");

  return true;
}

bool gen_wallet_oversized_payment_id::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3, false, "incorrect number of blocks fetched by miner's wallet: " << blocks_fetched);

  std::vector<tx_destination_entry> destinations({ tx_destination_entry(MK_TEST_COINS(3), m_accounts[ALICE_ACC_IDX].get_public_address()) });
  std::vector<attachment_v> attachments;
  
  //
  // 1. check max allowed payment id size
  //
  std::string payment_id(BC_PAYMENT_ID_SERVICE_SIZE_MAX, 'x');
  crypto::generate_random_bytes(payment_id.size(), &payment_id.front());

  bool r = set_payment_id_to_tx(attachments, payment_id);
  CHECK_AND_ASSERT_MES(r, false, "set_payment_id_to_tx failed");
  
  miner_wlt->transfer(destinations, 0, 0, TESTS_DEFAULT_FEE, empty_extra, attachments);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Tx pool has incorrect number of txs: " << c.get_pool_transactions_count());

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Tx pool is not empty: " << c.get_pool_transactions_count());

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3 + 1, false, "incorrect number of blocks fetched by Alice's wallet: " << blocks_fetched);

  std::list<tools::payment_details> payments;
  alice_wlt->get_payments(payment_id, payments);
  CHECK_AND_ASSERT_MES(payments.size() == 1, false, "Invalid payments count: " << payments.size());
  CHECK_AND_ASSERT_MES(payments.front().m_amount == MK_TEST_COINS(3), false, "Invalid payment");


  //
  // 2. check payment id with exceeding size -- should pass normally through the core and the wallet
  //
  payment_id.resize(BC_PAYMENT_ID_SERVICE_SIZE_MAX + 1, 'x');
  crypto::generate_random_bytes(payment_id.size(), &payment_id.front());

  // don't use set_payment_id_to_tx to avoid internal check for payment id length
  tx_service_attachment tsa = AUTO_VAL_INIT(tsa);
  tsa.service_id = BC_PAYMENT_ID_SERVICE_ID;
  tsa.body = payment_id;

  destinations.clear();
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(5), m_accounts[ALICE_ACC_IDX].get_public_address()));

  attachments.clear();
  attachments.push_back(tsa);

  miner_wlt->transfer(destinations, 0, 0, TESTS_DEFAULT_FEE, empty_extra, attachments);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Tx pool has incorrect number of txs: " << c.get_pool_transactions_count());

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Tx pool is not empty: " << c.get_pool_transactions_count());

  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 1, false, "incorrect number of blocks fetched by Alice's wallet: " << blocks_fetched);

  payments.clear();
  alice_wlt->get_payments(payment_id, payments);
  CHECK_AND_ASSERT_MES(payments.size() == 1, false, "Invalid payments count: " << payments.size());
  CHECK_AND_ASSERT_MES(payments.front().m_amount == MK_TEST_COINS(5), false, "Invalid payment");

  return true;
}

//------------------------------------------------------------------------------

bool gen_wallet_transfers_and_outdated_unconfirmed_txs::generate(std::vector<test_event_entry>& events) const
{
  // Test outline:
  // Alice receives two txs from miner.
  // Few blocks later money becomes unlocked. Both transfers in her waller are unspent.
  // Alice creates tx_3 and doesn't include it into a block. One transfers is marked as spent.
  // Alice waits some time, creates tx_4 via wallet (see below why) and doesn't put it into a block as well. Both transfers are spent now.
  // tx_3 becomes outdated, one transfers is not spent anymore.
  // tx_4 becomes outdated, both transfers are not spent.

  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);

  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, miner_acc, test_core_time::get_time());
  events.push_back(blk_0);

  // make initial transfers to Alice and unlock the money
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  MAKE_TX_FEE(events, tx_0a, miner_acc, alice_acc, MK_TEST_COINS(300), TESTS_DEFAULT_FEE, blk_0r);
  MAKE_TX_FEE(events, tx_0b, miner_acc, alice_acc, MK_TEST_COINS(300), TESTS_DEFAULT_FEE, blk_0r);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({ tx_0a, tx_0b }));

  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  // refresh the wallet and check transfers, both should be not spent
  CREATE_TEST_WALLET(alice_wlt, alice_acc, blk_0);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_1r, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2 + WALLET_DEFAULT_TX_SPENDABLE_AGE);
  tools::transfer_container trs;
  alice_wlt->get_transfers(trs);
  CHECK_AND_ASSERT_MES(trs.size() == 2 && !trs[0].is_spent() && !trs[1].is_spent(), false, "Wrong transfers state");

  // make a tx from outside the wallet, so wallet can't mark used indices
  MAKE_TX_FEE(events, tx_3, alice_acc, miner_acc, MK_TEST_COINS(290), TESTS_DEFAULT_FEE, blk_1r);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_1r, 0);
  alice_wlt->get_transfers(trs);
  CHECK_AND_ASSERT_MES(trs.size() == 2 && (trs[0].is_spent() ^ trs[1].is_spent()), false, "Wrong transfers state");

  // set time reference here
  uint64_t tx_4_ts = blk_1r.timestamp + 10;
  test_core_time::adjust(tx_4_ts);

  // make a tx via the wallet, so used selected_indicies is populated for associated transfer
  MAKE_TEST_WALLET_TX(events, tx_4, alice_wlt, MK_TEST_COINS(290), miner_acc);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_1r, 0);
  alice_wlt->get_transfers(trs);
  CHECK_AND_ASSERT_MES(trs.size() == 2 && trs[0].is_spent() && trs[1].is_spent(), false, "Wrong transfers state");

  // make a block to enforce processing outdated txs
  MAKE_NEXT_BLOCK(events, blk_2, blk_1r, miner_acc);

  // fast forward the time so tx_3 becomes outdated, only one transfer should be markes as spent now
  test_core_time::adjust(tx_4_ts - 5 + CURRENCY_MEMPOOL_TX_LIVETIME);

  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_2, 1);
  alice_wlt->get_transfers(trs);
  CHECK_AND_ASSERT_MES(trs.size() == 2 && (trs[0].is_spent() ^ trs[1].is_spent()), false, "Wrong transfers state");

  // fast forward the time so tx_4 becomes outdated too, both transfers are not spent
  test_core_time::adjust(tx_4_ts + 5 + CURRENCY_MEMPOOL_TX_LIVETIME);

  // enforce processing outdated txs in wallet::refresh()
  MAKE_NEXT_BLOCK(events, blk_3, blk_2, miner_acc);

  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_3, 1);
  alice_wlt->get_transfers(trs);
  CHECK_AND_ASSERT_MES(trs.size() == 2 && !trs[0].is_spent() && !trs[1].is_spent(), false, "Wrong transfers state");

  return true;
}

//------------------------------------------------------------------------------

bool gen_wallet_transfers_and_chain_switch::generate(std::vector<test_event_entry>& events) const
{
  // Test outline:
  // 1) create two accounts, mine some coins, transfer them to Alice (tx_0a and tx_0b);
  // 2) Alice spends money in two txs on height M and M+1. One of them created via the wallet, another - is created outside of the wallet;
  // 3) chain split occurs, tx_1 and tx_2 is moving to the pool, but they still make transfers (tx_0*) being marked as spent;
  // 4) aftewards transactions outdate so transfers are not spent anymore.

  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);

  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, preminer_acc, test_core_time::get_time());
  events.push_back(blk_0);

  // make initial transfers to Alice and unlock the money
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  bool r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), MK_TEST_COINS(30) * 2, 2, TESTS_DEFAULT_FEE, tx_0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs failed");
  events.push_back(tx_0);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  // refresh the wallet and check transfers, both should be not spent
  CREATE_TEST_WALLET(alice_wlt, alice_acc, blk_0);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_1r, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE);
  tools::transfer_container trs;
  alice_wlt->get_transfers(trs);
  CHECK_AND_ASSERT_MES(trs.size() == 2 && !trs[0].is_spent() && !trs[1].is_spent(), false, "Wrong transfers state");

  // make a tx from outside the wallet, so wallet can't mark used indices
  MAKE_TX(events, tx_1, alice_acc, miner_acc, MK_TEST_COINS(30) - TESTS_DEFAULT_FEE, blk_1r);
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_1);

  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_2, 1);
  alice_wlt->get_transfers(trs);
  CHECK_AND_ASSERT_MES(trs.size() == 2 && (trs[0].is_spent() ^ trs[1].is_spent()), false, "Wrong transfers state");

  // make a tx via the wallet, so used selected_indicies is populated for associated transfer
  MAKE_TEST_WALLET_TX(events, tx_2, alice_wlt, MK_TEST_COINS(30) - TESTS_DEFAULT_FEE, miner_acc);
  MAKE_NEXT_BLOCK_TX1(events, blk_3, blk_2, miner_acc, tx_2);

  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_3, 1);
  alice_wlt->get_transfers(trs);
  CHECK_AND_ASSERT_MES(trs.size() == 2 && trs[0].is_spent() && trs[1].is_spent(), false, "Wrong transfers state");

  //  0     10    11    21    22    23    24         <- blockchain height
  // (0 )- (0r)- (1 )- (1r)- (2 )- (3 )-             <- main chain
  //             tx_0a   |   tx_1  tx_2              <- txs
  //             tx_0b   |                           <- txs
  //                     \-- (2a)- (3a)- (4a)-       <- alt chain (becomes main at height 24)

  // split the chain!
  MAKE_NEXT_BLOCK(events, blk_2a, blk_1r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_3a, blk_2a, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_4a, blk_3a, miner_acc);

  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_4a, 3);
  alice_wlt->get_transfers(trs);
  CHECK_AND_ASSERT_MES(trs.size() == 2 && trs[0].is_spent() && trs[1].is_spent(), false, "Wrong transfers state");

  // fast forward time to make tx_1 and tx_2 outdated (blk_3 is the block where tx_2 came with)
  test_core_time::adjust(get_block_datetime(blk_3) + CURRENCY_MEMPOOL_TX_LIVETIME + 1);

  MAKE_NEXT_BLOCK(events, blk_5a, blk_4a, miner_acc);

  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_5a, 1);
  alice_wlt->get_transfers(trs);
  CHECK_AND_ASSERT_MES(trs.size() == 2 && !trs[0].is_spent() && !trs[1].is_spent(), false, "Wrong transfers state");

  return true;
}

//------------------------------------------------------------------------------

gen_wallet_decrypted_attachments::gen_wallet_decrypted_attachments()
  : m_on_transfer2_called(false)
{
    m_hardforks.set_hardfork_height(1, 0);
    m_hardforks.set_hardfork_height(2, 0); // tx_payer requires HF2
}

bool gen_wallet_decrypted_attachments::generate(std::vector<test_event_entry>& events) const
{
  // Test outline
  // NOTE: All transactions are sending with same attachments: tx_payer, tx_comment and tx_message
  // 1. Miner sends 10000 test coins to Alice, tx is in the pool.
  // 2. check that Alice's wallet gets correct on_transfer2 notification when refreshing.
  // 3. That tx is added into a block.
  // 4. Check that Alice's wallet gets correct on_transfer2 notification when refreshing (now it comes from blockchain, rather than from pool).
  // 
  // 5. Alice sends 100 test coins back to Miner (NOT via her wallet), tx is in the pool.
  // 6. Same as #2, but Alice spends money this time.
  // 7. Now tx is added into a block.
  // 8. Same as #4, but again - Alice spends money, not receives.
  //
  // The following part of the test is same as #1-#8, but all txs are sending via wallet.
  // 
  // 9. Alice sends 2000 test coins to Bob via her wallet, tx is in the pool.
  // 10. Refresh Alice's wallet and check for notification
  // 11. Get the tx into a block
  // 12. Check notification again
  // 13. Bob sends 200 test coins back to Alice via his wallet
  // 14. Same checks here as in #10-#12.

  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);
  GENERATE_ACCOUNT(bob_acc);
  m_accounts.push_back(bob_acc);

  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, miner_acc, test_core_time::get_time());
  events.push_back(blk_0);

  set_hard_fork_heights_to_generator(generator);
  DO_CALLBACK(events, "configure_core");

  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  CREATE_TEST_WALLET(alice_wlt, alice_acc, blk_0);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_0r, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // these attachments will be used across all the transactions in this test
  currency::tx_payer a_tx_payer = AUTO_VAL_INIT(a_tx_payer);
  a_tx_payer.acc_addr = miner_acc.get_keys().account_address;
  currency::tx_comment a_tx_comment = AUTO_VAL_INIT(a_tx_comment);
  a_tx_comment.comment = "Comandante Che Guevara";
  std::string a_tx_message = AUTO_VAL_INIT(a_tx_message);
  a_tx_message = "Hasta Siempre, Comandante";
  

  {
    // that feeling when you are a bit paranoid
    std::vector<currency::payload_items_v> decrypted_attachments;
    currency::keypair k = currency::keypair::generate();
    transaction t = AUTO_VAL_INIT(t);
    t.attachment = std::vector<currency::attachment_v>({ a_tx_payer, a_tx_comment, a_tx_message });
    add_tx_pub_key_to_extra(t, k.pub);
    add_attachments_info_to_extra(t.extra, t.attachment);
    currency::encrypt_attachments(t, miner_acc.get_keys(), alice_acc.get_public_address(), k);
    bool r = currency::decrypt_payload_items(true, t, alice_acc.get_keys(), decrypted_attachments);
    CHECK_AND_ASSERT_MES(r, false, "encrypt_attachments + decrypt_attachments failed to work together");
  }

  MAKE_TX_ATTACH(events, tx_0, miner_acc, alice_acc, MK_TEST_COINS(10000), blk_0r, std::vector<currency::attachment_v>({ a_tx_payer, a_tx_comment, a_tx_message }));
  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, miner_acc); // don't put tx_0 into this block, the block is only necessary to trigger tx_pool scan on in wallet2::refresh()

  // wallet callback must be passed as shared_ptr, so to avoid deleting "this" construct shared_ptr with custom null-deleter
  std::shared_ptr<tools::i_wallet2_callback> cb_ptr(const_cast<tools::i_wallet2_callback*>(static_cast<const tools::i_wallet2_callback*>(this)), [](tools::i_wallet2_callback*){});
  alice_wlt->callback(cb_ptr);

  m_comment_to_be_checked = a_tx_comment.comment;
  m_address_to_be_checked = get_account_address_as_str(miner_acc.get_public_address());
  m_on_transfer2_called = false;
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_1, 1);
  CHECK_AND_ASSERT_MES(m_on_transfer2_called, false, "on_transfer2() was not called (1)");

  // put tx_0 into a block and re-check callback
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1, miner_acc, tx_0);

  m_on_transfer2_called = false;
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_2, 1);
  CHECK_AND_ASSERT_MES(m_on_transfer2_called, false, "on_transfer2() was not called (2)");

  // Do the same in opposite direction: alice -> miner. Unlock money, received by Alice first.
  REWIND_BLOCKS_N(events, blk_2r, blk_2, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  MAKE_TX_ATTACH(events, tx_1, alice_acc, miner_acc, MK_TEST_COINS(100), blk_2r, std::vector<currency::attachment_v>({ a_tx_payer, a_tx_comment, a_tx_message }));
  MAKE_NEXT_BLOCK(events, blk_3, blk_2r, miner_acc); // don't put tx_1 into this block, the block is only necessary to trigger tx_pool scan on in wallet2::refresh()

  // Spend tx was not sent via Alice wallet instance, so wallet can't obtain destination address and pass it to callback (although, it decrypts attachments & extra)
  m_address_to_be_checked = ""; // So we just check callback with empty address
  m_on_transfer2_called = false;
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_3, WALLET_DEFAULT_TX_SPENDABLE_AGE + 1);
  CHECK_AND_ASSERT_MES(m_on_transfer2_called, false, "on_transfer2() was not called (3)");

  // Again, put the tx into a block, refresh the wallet and re-check.
  MAKE_NEXT_BLOCK_TX1(events, blk_4, blk_3, miner_acc, tx_1);

  m_on_transfer2_called = false;
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_4, 1);
  CHECK_AND_ASSERT_MES(m_on_transfer2_called, false, "on_transfer2() was not called (4)");

  // TODO: check also tx(alice->miner) with attach coming from a block and not previously being in wallets unconrifmed list

  // wait while Alice's cashback from tx_1 becomes unlocked
  REWIND_BLOCKS_N(events, blk_4r, blk_4, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  // keep time up to blockchain to avoid rejecting blocks by exceeding time diff threshold
  test_core_time::adjust(blk_4r.timestamp);
  events.push_back(event_core_time(blk_4r.timestamp));

  //
  // Second part. Almost the same as previous but all txs are sent via wallet.
  //

  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_4r, WALLET_DEFAULT_TX_SPENDABLE_AGE);
  m_comment_to_be_checked = a_tx_comment.comment;
  m_address_to_be_checked = get_account_address_as_str(bob_acc.get_public_address()); // note, that a_tx_payer is NOT refering to Bob's account, but in the callback we should get correct sender addr
  m_on_transfer2_called = false;
  MAKE_TEST_WALLET_TX_ATTACH(events, tx_2, alice_wlt, MK_TEST_COINS(2000), bob_acc, std::vector<currency::attachment_v>({ a_tx_payer, a_tx_comment, a_tx_message }));
  CHECK_AND_ASSERT_MES(m_on_transfer2_called, false, "on_transfer2() was not called (5)");

  MAKE_NEXT_BLOCK(events, blk_5, blk_4r, miner_acc); // don't put tx_2 into this block, the block is only necessary to trigger tx_pool scan on in wallet2::refresh()

  m_on_transfer2_called = false;
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_5, 1);
  CHECK_AND_ASSERT_MES(!m_on_transfer2_called, false, "on_transfer2() was called (6)");

  MAKE_NEXT_BLOCK_TX1(events, blk_6, blk_5, miner_acc, tx_2);

  m_on_transfer2_called = false;
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_6, 1);
  CHECK_AND_ASSERT_MES(m_on_transfer2_called, false, "on_transfer2() was not called (7)");


  REWIND_BLOCKS_N(events, blk_6r, blk_6, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  CREATE_TEST_WALLET(bob_wlt, bob_acc, blk_0);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, bob_wlt, blk_6r, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2 + WALLET_DEFAULT_TX_SPENDABLE_AGE + 2 + WALLET_DEFAULT_TX_SPENDABLE_AGE + 2 + WALLET_DEFAULT_TX_SPENDABLE_AGE);

  a_tx_payer.acc_addr = bob_acc.get_public_address(); // here we specify correct payer address, as it will not be masked by wallet
  MAKE_TEST_WALLET_TX_ATTACH(events, tx_3, bob_wlt, MK_TEST_COINS(200), alice_acc, std::vector<currency::attachment_v>({ a_tx_payer, a_tx_comment, a_tx_message }));

  MAKE_NEXT_BLOCK(events, blk_7, blk_6r, miner_acc); // don't put tx_3 into this block, the block is only necessary to trigger tx_pool scan on in wallet2::refresh()

  m_on_transfer2_called = false;
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_7, WALLET_DEFAULT_TX_SPENDABLE_AGE + 1);
  CHECK_AND_ASSERT_MES(m_on_transfer2_called, false, "on_transfer2() was not called (8)");

  MAKE_NEXT_BLOCK_TX1(events, blk_8, blk_7, miner_acc, tx_3);

  m_on_transfer2_called = false;
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_8, 1);
  CHECK_AND_ASSERT_MES(m_on_transfer2_called, false, "on_transfer2() was not called (9)");

  return true;
}

void gen_wallet_decrypted_attachments::on_transfer2(const tools::wallet_public::wallet_transfer_info& wti, const std::list<tools::wallet_public::asset_balance_entry>& balances, uint64_t total_mined)
{
  m_on_transfer2_called = true;
  //try {
    std::string remote_addresses;
    for (auto it : wti.remote_addresses)
      remote_addresses += remote_addresses.empty() ? it : (", " + it);
    CHECK_AND_ASSERT_THROW_MES(wti.comment == m_comment_to_be_checked &&
      ((m_address_to_be_checked.empty() && wti.remote_addresses.empty()) || std::count(wti.remote_addresses.begin(), wti.remote_addresses.end(), m_address_to_be_checked) == 1),
      "Wrongs wti received: comment: " << wti.comment << " remote addr: " << remote_addresses << std::endl << "  EXPECTED: comment: " << m_comment_to_be_checked << " remote addr: " << m_address_to_be_checked);
  //}
  //catch (...) { LOG_ERROR("!!!!!!!\n!!!!!!!\n!!!!!!!!\n!!!!!!\n!!!!!!!!\n!!!!!!!\n"); }
}

//------------------------------------------------------------------------------

gen_wallet_alias_and_unconfirmed_txs::gen_wallet_alias_and_unconfirmed_txs()
{
  REGISTER_CALLBACK_METHOD(gen_wallet_alias_and_unconfirmed_txs, c1);
  REGISTER_CALLBACK_METHOD(gen_wallet_alias_and_unconfirmed_txs, c2);
  REGISTER_CALLBACK_METHOD(gen_wallet_alias_and_unconfirmed_txs, c3);
}

bool gen_wallet_alias_and_unconfirmed_txs::generate(std::vector<test_event_entry>& events) const
{
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  DO_CALLBACK(events, "configure_core");
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);


  // miner registers an alias for Alice
  extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = "alicealice";
  ai.m_address = alice_acc.get_public_address();

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  bool r = fill_tx_sources_and_destinations(events, blk_0r, miner_acc, null_account, get_alias_coast_from_fee(ai.m_alias, ALIAS_VERY_INITAL_COAST), TESTS_DEFAULT_FEE, 0, sources, destinations);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources_and_destinations failed");
  for(auto& d : destinations)
    if (d.addr.back() == null_pub_addr)
      d.flags |= tx_destination_entry_flags::tdef_explicit_native_asset_id | tx_destination_entry_flags::tdef_zero_amount_blinding_mask;
  transaction tx_alice_alias{};
  crypto::secret_key sk{};
  r = construct_tx(miner_acc.get_keys(), sources, destinations, std::vector<currency::extra_v>({ ai }), empty_attachment, tx_alice_alias, get_tx_version_from_events(events), sk, 0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");
  ADD_CUSTOM_EVENT(events, tx_alice_alias);

  uint64_t alias_cost = get_alias_coast_from_fee(std::string(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, 'a'), ALIAS_VERY_INITAL_COAST);
  MAKE_TX(events, tx_0, miner_acc, alice_acc, alias_cost, blk_0r);
  MAKE_TX(events, tx_1, miner_acc, alice_acc, TESTS_DEFAULT_FEE, blk_0r);
  MAKE_TX(events, tx_2, miner_acc, bob_acc, alias_cost, blk_0r);
  MAKE_TX(events, tx_3, miner_acc, bob_acc, TESTS_DEFAULT_FEE, blk_0r);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({ tx_0, tx_1, tx_2, tx_3, tx_alice_alias }));

  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  DO_CALLBACK(events, "c2");

  DO_CALLBACK(events, "c3");

  return true;
}

bool gen_wallet_alias_and_unconfirmed_txs::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  bob_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3 + WALLET_DEFAULT_TX_SPENDABLE_AGE + 1, false, "Incorrect numbers of blocks fetched");

  // 1. register an alias for wallet's address
  currency::extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = std::string(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, 'a');
  ai.m_address = m_accounts[BOB_ACC_IDX].get_public_address();
  ai.m_view_key.push_back(m_accounts[BOB_ACC_IDX].get_keys().view_secret_key);

  uint64_t alias_reward = get_alias_coast_from_fee(ai.m_alias, ALIAS_VERY_INITAL_COAST);
  bool r = check_balance_via_wallet(*bob_wlt.get(), "bob_wlt", alias_reward + TESTS_DEFAULT_FEE);
  CHECK_AND_ASSERT_MES(r, false, "Incorrect wallet balance");

  transaction tx{};
  std::vector<tx_destination_entry> destinations({tx_destination_entry(alias_reward, null_pub_addr)});
  destinations.back().flags |= tx_destination_entry_flags::tdef_explicit_native_asset_id | tx_destination_entry_flags::tdef_zero_amount_blinding_mask;
  bob_wlt->transfer(destinations, 0, 0, TESTS_DEFAULT_FEE, std::vector<currency::extra_v>({ ai }), empty_attachment, tx);
  
  CHECK_AND_ASSERT_MES(check_native_coins_amount_burnt_in_outs(tx, alias_reward), false, "Generated tx has invalid alias reward");

  bool has_relates_alias_in_unconfirmed = false;
  bob_wlt->scan_tx_pool(has_relates_alias_in_unconfirmed);
  CHECK_AND_ASSERT_MES(has_relates_alias_in_unconfirmed, false, "No alias in unconfired");

  return true;
}

bool gen_wallet_alias_and_unconfirmed_txs::c2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  c.get_tx_pool().purge_transactions();
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "There are txs in the pool.");

  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  bob_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3 + WALLET_DEFAULT_TX_SPENDABLE_AGE + 1, false, "Incorrect numbers of blocks fetched");

  // 2. register smdb else alias
  account_base someone;
  someone.generate();
  currency::extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = std::string(ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED, 'b');
  ai.m_address = someone.get_public_address();
  ai.m_view_key.push_back(someone.get_keys().view_secret_key);
  
  uint64_t alias_reward = get_alias_coast_from_fee(ai.m_alias, ALIAS_VERY_INITAL_COAST);
  bool r = check_balance_via_wallet(*bob_wlt.get(), "bob_wlt", alias_reward + TESTS_DEFAULT_FEE);
  CHECK_AND_ASSERT_MES(r, false, "Incorrect wallet balance");

  transaction tx{};
  std::vector<tx_destination_entry> destinations({tx_destination_entry(alias_reward, null_pub_addr)});
  destinations.back().flags |= tx_destination_entry_flags::tdef_explicit_native_asset_id | tx_destination_entry_flags::tdef_zero_amount_blinding_mask;
  bob_wlt->transfer(destinations, 0, 0, TESTS_DEFAULT_FEE, std::vector<currency::extra_v>({ ai }), empty_attachment, tx);

  CHECK_AND_ASSERT_MES(check_native_coins_amount_burnt_in_outs(tx, alias_reward), false, "Generated tx has invalid alias reward");

  bool has_relates_alias_in_unconfirmed = false;
  bob_wlt->scan_tx_pool(has_relates_alias_in_unconfirmed);
  CHECK_AND_ASSERT_MES(!has_relates_alias_in_unconfirmed, false, "There is Alice-related alias in unconfired");

  return true;
}

bool gen_wallet_alias_and_unconfirmed_txs::c3(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  c.get_tx_pool().purge_transactions();
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "There are txs in the pool.");

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3 + WALLET_DEFAULT_TX_SPENDABLE_AGE + 1, false, "Incorrect numbers of blocks fetched");

  // 3. update an alias
  currency::extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = "alicealice";
  ai.m_address = m_accounts[MINER_ACC_IDX].get_public_address();
  ai.m_view_key.push_back(m_accounts[MINER_ACC_IDX].get_keys().view_secret_key);

  bool r = sign_extra_alias_entry(ai, m_accounts[ALICE_ACC_IDX].get_keys().account_address.spend_public_key, m_accounts[ALICE_ACC_IDX].get_keys().spend_secret_key);
  CHECK_AND_ASSERT_MES(r, false, "sign_extra_alias_entry failed");

  transaction tx{};
  std::vector<tx_destination_entry> destinations({tx_destination_entry(1, null_pub_addr)});
  destinations.back().flags |= tx_destination_entry_flags::tdef_explicit_native_asset_id | tx_destination_entry_flags::tdef_zero_amount_blinding_mask;
  alice_wlt->transfer(destinations, 0, 0, TESTS_DEFAULT_FEE, std::vector<currency::extra_v>({ ai }), empty_attachment, tx);


  bool has_relates_alias_in_unconfirmed = false;
  alice_wlt->scan_tx_pool(has_relates_alias_in_unconfirmed);
  CHECK_AND_ASSERT_MES(has_relates_alias_in_unconfirmed, false, "No alias in unconfired");

  return true;
}

//------------------------------------------------------------------------------

gen_wallet_alias_via_special_wallet_funcs::gen_wallet_alias_via_special_wallet_funcs()
{
  REGISTER_CALLBACK_METHOD(gen_wallet_alias_via_special_wallet_funcs, c1);
}

bool gen_wallet_alias_via_special_wallet_funcs::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);

  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, miner_acc, test_core_time::get_time());
  events.push_back(blk_0);
  set_hard_fork_heights_to_generator(generator);
  DO_CALLBACK(events, "configure_core");

  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_acc);

  extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = "minerminer";
  ai.m_address = miner_acc.get_public_address();
  block blk_2{};
  bool r = generator.construct_pow_block_with_alias_info_in_coinbase(miner_acc, blk_1, ai, blk_2);
  CHECK_AND_ASSERT_MES(r, false, "construct_pow_block_with_alias_info_in_coinbase failed");
  ADD_CUSTOM_EVENT(events, blk_2);

  REWIND_BLOCKS_N(events, blk_2r, blk_2, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  uint64_t biggest_alias_reward = get_alias_coast_from_fee("a", ALIAS_VERY_INITAL_COAST);
  MAKE_TX(events, tx_0, miner_acc, alice_acc, biggest_alias_reward + TESTS_DEFAULT_FEE, blk_2r);
  MAKE_TX(events, tx_1, miner_acc, alice_acc, biggest_alias_reward + TESTS_DEFAULT_FEE, blk_2r);
  MAKE_TX(events, tx_2, miner_acc, alice_acc, biggest_alias_reward + TESTS_DEFAULT_FEE, blk_2r);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3, blk_2r, miner_acc, std::list<transaction>({ tx_0, tx_1, tx_2 }));

  REWIND_BLOCKS_N(events, blk_3r, blk_3, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

bool gen_wallet_alias_via_special_wallet_funcs::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 2 + CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE, false, "Incorrect numbers of blocks fetched");

  extra_alias_entry ai = AUTO_VAL_INIT(ai);
  ai.m_alias = "alicealice";
  ai.m_address = m_accounts[ALICE_ACC_IDX].get_public_address();
  transaction res_tx = AUTO_VAL_INIT(res_tx);
  alice_wlt->request_alias_registration(ai, res_tx, TESTS_DEFAULT_FEE); // use 0 reward for alias so wallet calculate the correct reward by its own

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");

  bool r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");
  CHECK_AND_ASSERT_MES(c.get_current_blockchain_size() == 3 + CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE + 1, false, "Incorrect blockchain size");

  uint64_t biggest_alias_reward = get_alias_coast_from_fee("a", ALIAS_VERY_INITAL_COAST);
  std::shared_ptr<wlt_lambda_on_transfer2_wrapper> l(new wlt_lambda_on_transfer2_wrapper(
    [biggest_alias_reward](const tools::wallet_public::wallet_transfer_info& wti, const std::list<tools::wallet_public::asset_balance_entry>& balances, uint64_t total_mined) -> bool {
      return std::count(wti.remote_aliases.begin(), wti.remote_aliases.end(), "minerminer") == 1 &&
        wti.get_native_amount() == biggest_alias_reward + get_tx_fee(wti.tx);
    }
  ));
  alice_wlt->callback(l);

  transaction t;
  std::vector<tx_destination_entry> destinations(1, tx_destination_entry(biggest_alias_reward, m_accounts[MINER_ACC_IDX].get_public_address()));
  alice_wlt->transfer(destinations, 0, 0, TESTS_DEFAULT_FEE, std::vector<extra_v>(), std::vector<attachment_v>(), t);
  CHECK_AND_ASSERT_MES(l->m_result, false, "Wrong wti received via callback");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");
  CHECK_AND_ASSERT_MES(c.get_current_blockchain_size() == 3 + CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE + 2, false, "Incorrect blockchain size");

  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 2, false, "Incorrect numbers of blocks fetched");

  ai.m_text_comment = "Update!";
  ai.m_address = m_accounts[MINER_ACC_IDX].get_public_address();
  alice_wlt->request_alias_update(ai, res_tx, TESTS_DEFAULT_FEE, 0);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");
  CHECK_AND_ASSERT_MES(c.get_current_blockchain_size() == 3 + CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE + 3, false, "Incorrect blockchain size");

  extra_alias_entry ai2 = AUTO_VAL_INIT(ai2);
  r = c.get_blockchain_storage().get_alias_info(ai.m_alias, ai2);
  CHECK_AND_ASSERT_MES(r, false, "get_alias_info failed");
  CHECK_AND_ASSERT_MES(ai2.m_text_comment == ai.m_text_comment && ai2.m_address == m_accounts[MINER_ACC_IDX].get_public_address(), false, "Incorrect alias info retunred by get_alias_info");

  return true;
}

//------------------------------------------------------------------------------

gen_wallet_fake_outputs_randomness::gen_wallet_fake_outputs_randomness()
  : m_amount_many_outs_have(0)
{
  REGISTER_CALLBACK_METHOD(gen_wallet_fake_outputs_randomness, c1);
}

bool gen_wallet_fake_outputs_randomness::generate(std::vector<test_event_entry>& events) const
{
  // Test outline.
  // 1. Create simple blockchain:
  //
  //  0     1     N    N+1   2N     <- blockchain height
  // (0 )- (..)- (0r)- ... -(1r)
  //
  //       \--------/  \-------/
  //     mined by miner    |
  //                 mined by Alice
  //
  // 2. Miner refreshes his wallet and transfers to Alice more then a half of one block reward with mixin=2.
  // 3. The wallet and tx pool is cleared.
  // 4. Steps 2-3 are repeated few times.
  // 5. Check that real input index selected in the transfer() is not the same over these trials (there should be some randomness).

  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);

  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, preminer_acc, test_core_time::get_time());
  events.push_back(blk_0);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, alice_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_0r, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_2r, blk_1r, alice_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  REWIND_BLOCKS_N_WITH_TIME(events, blk_3r, blk_2r, alice_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW); // to be able to use mined money in the first N blocks as mix-ins

  uint64_t first_blocks_reward = get_outs_money_amount(blk_0r.miner_tx);
  uint64_t stub;
  bool r = calculate_amounts_many_outs_have_and_no_outs_have(first_blocks_reward, m_amount_many_outs_have, stub);
  CHECK_AND_ASSERT_MES(r, false, "calculate_amounts_many_outs_have_and_no_outs_have failed");

  DO_CALLBACK(events, "c1");

  return true;
}

bool gen_wallet_fake_outputs_randomness::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  {
    std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
    size_t blocks_fetched = 0;
    bool received_money;
    std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
    miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
    CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW * 4, false, "Incorrect numbers of blocks fetched");
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

    miner_wlt->reset_password(g_wallet_password);
    miner_wlt->store(g_wallet_filename);
  }

  const size_t fake_outputs_count = 2;
  const size_t trials_count = 100;
  std::map<size_t, size_t> real_inputs;
  size_t tries = 0;
  for (; tries < trials_count; ++tries)
  {
    std::shared_ptr<tools::wallet2> miner_wlt(new tools::wallet2);
    miner_wlt->load(g_wallet_filename, g_wallet_password);
    miner_wlt->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());
    miner_wlt->set_core_proxy(m_core_proxy);


    std::vector<tx_destination_entry> dest(1, tx_destination_entry(m_amount_many_outs_have - TESTS_DEFAULT_FEE, m_accounts[ALICE_ACC_IDX].get_public_address()));
    transaction tx;
    miner_wlt->transfer(dest, fake_outputs_count, 0, TESTS_DEFAULT_FEE, std::vector<extra_v>(), std::vector<attachment_v>(), tx);

    std::vector<size_t> r_ins;
    bool r = determine_tx_real_inputs(c, tx, m_accounts[MINER_ACC_IDX].get_keys(), r_ins);
    CHECK_AND_ASSERT_MES(r, false, "determine_tx_real_inputs failed");

    CHECK_AND_ASSERT_MES(r_ins.size() == 1, false, "Incorrect outs size");
    size_t real_input = r_ins[0];

    LOG_PRINT_MAGENTA(">>>> SELECTED REAL INPUT: " << real_input, LOG_LEVEL_0);
    real_inputs[real_input]++;

    c.get_tx_pool().purge_transactions();
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "There are txs in the pool.");
  }

  // make sure that the same input was not used as real in the half of trials
  for (auto idx : real_inputs)
  {
    LOG_PRINT_L0("Real index " << idx.first << " has been selected " << idx.second << " times of " << tries);
    CHECK_AND_ASSERT_MES(idx.second < tries / 2, false, "Real inputs were selected quite predictable, they expected to be randomly distributed.");
  }

  return true;
};

//------------------------------------------------------------------------------

gen_wallet_fake_outputs_not_enough::gen_wallet_fake_outputs_not_enough()
  : m_amount_many_outs_have(0)
  , m_amount_no_outs_have(0)
{
  REGISTER_CALLBACK_METHOD(gen_wallet_fake_outputs_not_enough, c1);
}

bool gen_wallet_fake_outputs_not_enough::generate(std::vector<test_event_entry>& events) const
{
  // Test outline.
  // 1. Miner mines CURRENCY_MINED_MONEY_UNLOCK_WINDOW blocks
  // 2. Miner transfers 4 non-existing amounts (like 1000000) to Alice.
  // 3. Miner mines few more block to unlock the txs.
  // 4. Now Alice has 4 outputs with same amount. Alice sends to Bob a tx with fake_outputs_count == 3 (so it must be enough mixins in the blockchain), then clears tx pool and wallet.
  // 5. Step 4 repeated several times. Make sure amounts are mixed in randomly.

  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);
  GENERATE_ACCOUNT(bob_acc);
  m_accounts.push_back(bob_acc);

  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, miner_acc, test_core_time::get_time());
  events.push_back(blk_0);

  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  uint64_t first_blocks_reward = get_outs_money_amount(blk_0r.miner_tx);
  bool r = calculate_amounts_many_outs_have_and_no_outs_have(first_blocks_reward, m_amount_many_outs_have, m_amount_no_outs_have);
  CHECK_AND_ASSERT_MES(r, false, "calculate_amounts_many_outs_have_and_no_outs_have failed");

  MAKE_TX(events, tx_0, miner_acc, alice_acc, m_amount_no_outs_have, blk_0r);
  MAKE_TX(events, tx_1, miner_acc, alice_acc, m_amount_no_outs_have, blk_0r);
  MAKE_TX(events, tx_2, miner_acc, alice_acc, m_amount_no_outs_have, blk_0r);
  MAKE_TX(events, tx_3, miner_acc, alice_acc, m_amount_no_outs_have, blk_0r);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({ tx_0, tx_1, tx_2, tx_3 }));

  REWIND_BLOCKS_N(events, blk_1r, blk_1, alice_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW); // to be able to use mined money in the first N blocks as mix-ins


  DO_CALLBACK(events, "c1");

  return true;
}

bool gen_wallet_fake_outputs_not_enough::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::unordered_multiset<size_t> real_inputs;
  for (size_t tries = 0; tries < 5; ++tries)
  {
    std::list<crypto::public_key> pkeys;
    bool r = c.get_blockchain_storage().get_outs(m_amount_no_outs_have, pkeys);
    CHECK_AND_ASSERT_MES(r && pkeys.size() == 4, false, "incorrect number of outputs with amount = " << m_amount_no_outs_have);

    std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
    size_t blocks_fetched = 0;
    bool received_money;
    std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
    alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
    CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW * 2 + 1, false, "Incorrect numbers of blocks fetched");
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

    size_t fake_outputs_count = 3;

    std::vector<tx_destination_entry> dest(1, tx_destination_entry(m_amount_no_outs_have - TESTS_DEFAULT_FEE, m_accounts[BOB_ACC_IDX].get_public_address()));
    transaction tx;
    alice_wlt->transfer(dest, fake_outputs_count, 0, TESTS_DEFAULT_FEE, std::vector<extra_v>(), std::vector<attachment_v>(), tx);

    std::vector<size_t> r_ins;
    r = determine_tx_real_inputs(c, tx, m_accounts[ALICE_ACC_IDX].get_keys(), r_ins);
    CHECK_AND_ASSERT_MES(r, false, "determine_tx_real_inputs failed");

    CHECK_AND_ASSERT_MES(r_ins.size() == 1, false, "Incorrect outs size");
    size_t real_input = r_ins[0];
    LOG_PRINT_MAGENTA(">>>> SELECTED REAL INPUT: " << real_input, LOG_LEVEL_0);
    real_inputs.insert(real_input);

    c.get_tx_pool().purge_transactions();
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "There are txs in the pool.");
  }

  // make sure that the same input was not used as real in the half of trials
  for (auto idx : real_inputs)
  {
    CHECK_AND_ASSERT_MES(real_inputs.count(idx) < real_inputs.size() / 2, false, "Real inputs were selected quite predictable, they expected to be randomly distributed.");
  }

  return true;
}

//------------------------------------------------------------------------------

gen_wallet_offers_basic::gen_wallet_offers_basic()
{
  REGISTER_CALLBACK_METHOD(gen_wallet_offers_basic, c1);
}

bool gen_wallet_offers_basic::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);

  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, preminer_acc, test_core_time::get_time());
  events.push_back(blk_0);

  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  DO_CALLBACK(events, "c1");

  return true;
}

bool gen_wallet_offers_basic::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  // Test outline:
  // 1. Make two offers and push them via synced wallet.
  // 2. Update order #1.
  // 3. Cancel order #2.
  // 4. Make sure everything goes okey.

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1, false, "Incorrect numbers of blocks fetched");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  bc_services::offer_details_ex od1 = AUTO_VAL_INIT(od1);
  fill_test_offer(od1);
  od1.deal_option = "OFFER 1";
  od1.fee = TESTS_DEFAULT_FEE;
  bc_services::offer_details_ex od2 = AUTO_VAL_INIT(od2);
  fill_test_offer(od2);
  od2.deal_option = "OFFER 2";
  od2.fee = TESTS_DEFAULT_FEE;

  transaction tx_o1, tx_o2;
  miner_wlt->push_offer(od1, tx_o1);
  miner_wlt->push_offer(od2, tx_o2);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 2, false, "Incorrect txs count in the pool");

  bool r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 1, false, "Incorrect number of blocks fetched");

  std::list<bc_services::offer_details_ex> offers;
  r = miner_wlt->get_actual_offers(offers);
  CHECK_AND_ASSERT_MES(r, false, "get_actual_offers failed");
  CHECK_AND_ASSERT_MES(offers.size() == 2, false, "Incorrect size of actual offers list");

  crypto::hash tx_o1_hash = get_transaction_hash(tx_o1);
  bc_services::offer_details_ex actual_offer_1, actual_offer_2;
  if (offers.front().tx_hash == tx_o1_hash)
  {
    actual_offer_1 = offers.front();
    actual_offer_2 = offers.back();
  }
  else
  {
    actual_offer_1 = offers.back();
    actual_offer_2 = offers.front();
  }


  CHECK_AND_ASSERT_MES(actual_offer_1.tx_hash == tx_o1_hash, false, "Incorrect tx_hash in actual offer 1");
  CHECK_AND_ASSERT_MES(actual_offer_1.index_in_tx == 0, false, "Incorrect index_in_tx for offer 1");
  CHECK_AND_ASSERT_MES(actual_offer_1.fee == get_tx_fee(tx_o1), false, "Incorrect fee for offer 1");

  r = compare_offers(od1, actual_offer_1);
  CHECK_AND_ASSERT_MES(r, false, "compare_offers failed");

  CHECK_AND_ASSERT_MES(actual_offer_2.tx_hash == get_transaction_hash(tx_o2), false, "Incorrect tx_hash in actual offer 2");
  CHECK_AND_ASSERT_MES(actual_offer_2.index_in_tx == 0, false, "Incorrect index_in_tx for offer 2");
  CHECK_AND_ASSERT_MES(actual_offer_2.fee == get_tx_fee(tx_o2), false, "Incorrect fee for offer 2");

  r = compare_offers(od2, actual_offer_2);
  CHECK_AND_ASSERT_MES(r, false, "compare_offers failed (2)");

  // update offer 2

  bc_services::offer_details_ex offer_2_update = actual_offer_2;
  offer_2_update.comment = "UPDATED OFFER";

  transaction tx_upd;
  miner_wlt->update_offer_by_id(actual_offer_2.tx_hash, offer_2_update.index_in_tx, offer_2_update, tx_upd);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 1, false, "Incorrect number of blocks fetched");

  offers.clear();
  r = miner_wlt->get_actual_offers(offers);
  CHECK_AND_ASSERT_MES(r, false, "get_actual_offers failed");
  CHECK_AND_ASSERT_MES(offers.size() == 2, false, "Incorrect size of actual offers list");

  crypto::hash tx_upd_hash = get_transaction_hash(tx_upd);
  bc_services::offer_details_ex offer_2_updated = offers.front();
  if (offers.back().tx_hash == tx_upd_hash)
    offer_2_updated = offers.back();

  CHECK_AND_ASSERT_MES(offer_2_updated.tx_hash == tx_upd_hash, false, "Incorrect tx_hash in updated offer 2");
  CHECK_AND_ASSERT_MES(offer_2_updated.tx_original_hash == get_transaction_hash(tx_o2), false, "Incorrect ttx_original_hashx_hash in updated offer 2");

  r = compare_offers(offer_2_updated, offer_2_update);
  CHECK_AND_ASSERT_MES(r, false, "compare_offers failed (3)");

  // cancel offer 1

  transaction tx_offer1_cancel;
  miner_wlt->cancel_offer_by_id(actual_offer_1.tx_hash, actual_offer_1.index_in_tx, TESTS_DEFAULT_FEE, tx_offer1_cancel);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 1, false, "Incorrect number of blocks fetched");

  offers.clear();
  r = miner_wlt->get_actual_offers(offers);
  CHECK_AND_ASSERT_MES(r, false, "get_actual_offers failed");
  CHECK_AND_ASSERT_MES(offers.size() == 1, false, "Incorrect size of actual offers list");

  r = compare_offers(offers.back(), offer_2_update);
  CHECK_AND_ASSERT_MES(r, false, "compare_offers failed (4)");

  return true;
}

//------------------------------------------------------------------------------

gen_wallet_offers_size_limit::gen_wallet_offers_size_limit()
{
  REGISTER_CALLBACK_METHOD(gen_wallet_offers_size_limit, c1);
}

bool gen_wallet_offers_size_limit::generate(std::vector<test_event_entry>& events) const
{
  // Test outline:
  // 1. Make offer_details struct instance not exceeding CURRENCY_MAX_TRANSACTION_BLOB_SIZE and exceeding one
  // 2. Push an offer with the first, make sure it goes ok.
  // 3. Try to push an offer with the second, make sure tx_too_big exception is thrown.
  // 4. Try to update an offer from #2 with huge sized offer_detailed, make sure ex

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

// generates such an offer so that result tx will most like have its size within the giving limits
bool generate_oversized_offer(size_t min_size, size_t max_size, bc_services::offer_details_ex& result, uint64_t tx_version)
{
  bc_services::offer_details_ex r = AUTO_VAL_INIT(r);
  result = r;
  fill_test_offer(result);
  result.comment = get_random_text(max_size); // start with reasonable offer size

  std::vector<currency::attachment_v> att_container;

  static const size_t max_attempts_to_find_correct_packed_size = 16;
  for (size_t i = 0; i < max_attempts_to_find_correct_packed_size; ++i)
  {
    att_container.clear();
    put_offer_into_attachment(static_cast<bc_services::offer_details&>(result), att_container);

    // construct fake tx to estimate it's size
    transaction tx = AUTO_VAL_INIT(tx);
    crypto::secret_key one_time_secret_key;
    if (!construct_tx(account_keys(), std::vector<tx_source_entry>(), std::vector<tx_destination_entry>(), empty_extra, att_container, tx, tx_version, one_time_secret_key, 0, 0, true, 0))
      return false;

    size_t sz = get_object_blobsize(tx);
    if (sz > max_size)
    {
      size_t l = std::max(static_cast<size_t>(1), (sz - max_size) * 5 / 4);
      result.comment.erase(result.comment.size() - l, l);
      continue;
    }

    if (sz > min_size)
      return true;

    result.comment += get_random_text((min_size - sz) * 5 / 4);
  }

  return false;
}

bool gen_wallet_offers_size_limit::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW, false, "Incorrect numbers of blocks fetched");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  bc_services::offer_details_ex od_normal = AUTO_VAL_INIT(od_normal);
  bool r = generate_oversized_offer(CURRENCY_MAX_TRANSACTION_BLOB_SIZE - 2048, CURRENCY_MAX_TRANSACTION_BLOB_SIZE - 1024, od_normal, c.get_current_tx_version()); // generate biggest offer but within tx size limits
  CHECK_AND_ASSERT_MES(r, false, "generate_oversized_offer failed");
  od_normal.fee = TESTS_DEFAULT_FEE;

  bc_services::offer_details_ex od_oversized = AUTO_VAL_INIT(od_oversized);
  r = generate_oversized_offer(CURRENCY_MAX_TRANSACTION_BLOB_SIZE, CURRENCY_MAX_TRANSACTION_BLOB_SIZE + 1024, od_oversized, c.get_current_tx_version());
  CHECK_AND_ASSERT_MES(r, false, "generate_oversized_offer failed");
  od_oversized.fee = TESTS_DEFAULT_FEE;

  // switch off logging during these calls to avoid flooding
  int log_level = log_space::get_set_log_detalisation_level();
  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_SILENT);

  transaction tx_od_normal;
  miner_wlt->push_offer(od_normal, tx_od_normal);

  log_space::get_set_log_detalisation_level(true, log_level); // restore

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 1, false, "Incorrect numbers of blocks fetched");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  // switch off logging during these calls to avoid flooding
  log_level = log_space::get_set_log_detalisation_level();
  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_SILENT);

  r = false;
  transaction tx;
  try
  {
    miner_wlt->push_offer(od_oversized, tx);
  }
  catch (const tools::error::tx_too_big&)
  {
    r = true;
  }
  log_space::get_set_log_detalisation_level(true, log_level); // restore

  CHECK_AND_ASSERT_MES(r, false, "error::tx_too_big exception was not caught");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_SILENT);
  r = false;
  try
  {
    miner_wlt->update_offer_by_id(get_transaction_hash(tx_od_normal), 0, od_oversized, tx);
  }
  catch (const tools::error::tx_too_big&)
  {
    r = true;
  }
  log_space::get_set_log_detalisation_level(true, log_level); // restore

  CHECK_AND_ASSERT_MES(r, false, "error::tx_too_big exception was not caught");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  return true;
}

//------------------------------------------------------------------------------

gen_wallet_dust_to_account::gen_wallet_dust_to_account()
{
  REGISTER_CALLBACK_METHOD(gen_wallet_dust_to_account, c1);
}

bool gen_wallet_dust_to_account::generate(std::vector<test_event_entry>& events) const
{
  // Test outline:
  // 1. Mine some blocks.
  // 2. Make a transfer with dust policy, requiring sending dust to an address.
  // 3. Put the tx into a block.
  // 4. Refresh wallet with dust address and make sure it has correct balance.

  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);
  GENERATE_ACCOUNT(bob_acc);
  m_accounts.push_back(bob_acc);

  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, miner_acc, test_core_time::get_time());
  events.push_back(blk_0);

  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK(events, "c1");

  return true;
}

bool gen_wallet_dust_to_account::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{ 
  // test is disabled until DEFAULT_DUST_THRESHOLD becomes non-zero
  static_assert(DEFAULT_DUST_THRESHOLD == 0, "This test should be enabled, because DEFAULT_DUST_THRESHOLD != 0");

  /*
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW, false, "Incorrect numbers of blocks fetched");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  // make dust policy, with Bob's address as dust receiver
  tools::tx_dust_policy dust_policy(DEFAULT_DUST_THRESHOLD, false, m_accounts[BOB_ACC_IDX].get_public_address());

  uint64_t amount = 123.456789 * (float)DEFAULT_DUST_THRESHOLD;
  miner_wlt->transfer(std::vector<tx_destination_entry>({ tx_destination_entry(amount, m_accounts[ALICE_ACC_IDX].get_public_address()) }),
    0, 0, TESTS_DEFAULT_FEE, std::vector<extra_v>(), std::vector<attachment_v>(), tools::detail::digit_split_strategy, dust_policy);

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");

  bool r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  miner_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 1, false, "Incorrect numbers of blocks fetched");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");



  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  bob_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1, false, "Incorrect numbers of blocks fetched");

  uint64_t dust_amount = (DEFAULT_DUST_THRESHOLD - amount % DEFAULT_DUST_THRESHOLD) % DEFAULT_DUST_THRESHOLD;

  r = check_balance_via_wallet(*bob_wlt, "bob_wlt", dust_amount);
  CHECK_AND_ASSERT_MES(r, false, "Incorrect balance");
  */
  return true;
}

//------------------------------------------------------------------------------

gen_wallet_selecting_pos_entries::gen_wallet_selecting_pos_entries()
  : m_amount(0)
{
  REGISTER_CALLBACK_METHOD(gen_wallet_selecting_pos_entries, c1);
  REGISTER_CALLBACK_METHOD(gen_wallet_selecting_pos_entries, set_core_config);
}

bool gen_wallet_selecting_pos_entries::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);

  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, miner_acc, test_core_time::get_time());
  events.push_back(blk_0);

  DO_CALLBACK(events, "set_core_config");

  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3);

  uint64_t stub;
  bool r = calculate_amounts_many_outs_have_and_no_outs_have(get_outs_money_amount(blk_0r.miner_tx), m_amount, stub);
  CHECK_AND_ASSERT_MES(r, false, "calculate_amounts_many_outs_have_and_no_outs_have failed");

  MAKE_TX(events, tx_0, miner_acc, alice_acc, m_amount, blk_0r);
  MAKE_TX(events, tx_1, miner_acc, alice_acc, m_amount, blk_0r);
  MAKE_TX(events, tx_2, miner_acc, alice_acc, m_amount, blk_0r);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({ tx_0, tx_1, tx_2 }));

  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE - 2);

  DO_CALLBACK(events, "c1");

  return true;
}

bool gen_wallet_selecting_pos_entries::set_core_config(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  core_runtime_config crc = c.get_blockchain_storage().get_core_runtime_config();
  crc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH;
  crc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE;
  c.get_blockchain_storage().set_core_runtime_config(crc);
  return true;
}

bool gen_wallet_selecting_pos_entries::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  core_runtime_config crc = c.get_blockchain_storage().get_core_runtime_config();
  crc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH;
  c.get_blockchain_storage().set_core_runtime_config(crc);

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3 + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE - 2, false, "Incorrect numbers of blocks fetched");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  bool r = false;
  CHECK_AND_ASSERT_MES(alice_wlt->get_pos_entries_count() == 0, false, "Incorrect return value and pos_entries size");

  tools::wallet2::mining_context ctx = AUTO_VAL_INIT(ctx);
  r = alice_wlt->fill_mining_context(ctx); // should internally fail because there's no unlocked money to be used as pos entries
  CHECK_AND_ASSERT_MES(r, false, "Incorrect fill_mining_context result"); // nevertheless fill_mining_context returns true in this case, so check it here

  r = alice_wlt->try_mint_pos(); // should fail because there's no unlocked money to be used as pos entries
  CHECK_AND_ASSERT_MES(r, false, "try_mint_pos returns false"); // nevertheless try_mint_pos returns true in this case, so check it here
  
  // check that no blocks have come
  CHECK_AND_ASSERT_MES(c.get_current_blockchain_size() == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3 + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE - 1, false, "Incorrect current blockchain height");

  // this block should unlock the money and PoS-entries should become available
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);

  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 1, false, "Incorrect number of blocks fetched");

  CHECK_AND_ASSERT_MES(alice_wlt->get_pos_entries_count() == 3, false, "Incorrect return value and pos_entries size");

  r = alice_wlt->try_mint_pos(); // should really mine a block
  CHECK_AND_ASSERT_MES(r, false, "try_mint_pos returns false");

  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 1, false, "Incorrect number of blocks fetched");

  CHECK_AND_ASSERT_MES(c.get_current_blockchain_size() == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3 + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE + 1, false, "Incorrect current blockchain height");

  std::vector<tx_destination_entry> dst({
    tx_destination_entry(m_amount - 0, m_accounts[MINER_ACC_IDX].get_public_address()),
    tx_destination_entry(m_amount - TESTS_DEFAULT_FEE, m_accounts[MINER_ACC_IDX].get_public_address())
  });
  alice_wlt->transfer(dst, 0, 0, TESTS_DEFAULT_FEE, std::vector<extra_v>(), std::vector<attachment_v>());
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);

  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 1, false, "Incorrect number of blocks fetched");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  CHECK_AND_ASSERT_MES(alice_wlt->get_pos_entries_count() == 0, false, "Incorrect return value and pos_entries size");

  return true;
}

//------------------------------------------------------------------------------

gen_wallet_spending_coinstake_after_minting::gen_wallet_spending_coinstake_after_minting()
  : m_amount(0)
{
  REGISTER_CALLBACK_METHOD(gen_wallet_spending_coinstake_after_minting, c1);
}

bool gen_wallet_spending_coinstake_after_minting::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);

  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, miner_acc, test_core_time::get_time());
  events.push_back(blk_0);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  m_amount = get_outs_money_amount(blk_0r.miner_tx);

  MAKE_TX(events, tx_0, miner_acc, alice_acc, m_amount, blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

bool gen_wallet_spending_coinstake_after_minting::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  core_runtime_config crc = c.get_blockchain_storage().get_core_runtime_config();
  crc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH;
  c.get_blockchain_storage().set_core_runtime_config(crc);

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE, false, "Incorrect numbers of blocks fetched");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  bool r = alice_wlt->try_mint_pos();
  CHECK_AND_ASSERT_MES(r, false, "try_mint_pos failed");

  CHECK_AND_ASSERT_MES(c.get_current_blockchain_size() == CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE + 2, false, "Incorrect current blockchain height");

  // Don't refresh the wallet here intentionally.
  //alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  //CHECK_AND_ASSERT_MES(blocks_fetched == 1, false, "Incorrect number of blocks fetched");
  //CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  alice_wlt->transfer(std::vector<tx_destination_entry>({ tx_destination_entry(m_amount - TESTS_DEFAULT_FEE, m_accounts[MINER_ACC_IDX].get_public_address()) }),
    0, 0, TESTS_DEFAULT_FEE, std::vector<extra_v>(), std::vector<attachment_v>());

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool");

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);

  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 1, false, "Incorrect number of blocks fetched");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

  return true;
}

//------------------------------------------------------------------------------

gen_wallet_fake_outs_while_having_too_little_own_outs::gen_wallet_fake_outs_while_having_too_little_own_outs()
  : m_amount_many_outs_have(0)
{
  REGISTER_CALLBACK_METHOD(gen_wallet_fake_outs_while_having_too_little_own_outs, c1);
}

bool gen_wallet_fake_outs_while_having_too_little_own_outs::generate(std::vector<test_event_entry>& events) const
{
  // Test outline
  // 1. Miner mines CURRENCY_MINED_MONEY_UNLOCK_WINDOW * 2 blocks
  // 2. Miner transfer to Alice amount of one mined block
  // 3. Alice waits WALLET_DEFAULT_TX_SPENDABLE_AGE blocks for money become unlocked, and few block more (so her tx isn't in the last block)
  // 4. Alice transfers all her money (several outs of different amount) back to miner with fake_outs_count=4
  // 5. Check what every real output used in generated tx refers to the same block where miner-to-Alice tx is kept - so it can easily be guessed.
  // 6. Repeat 4-5 few times and gather some stats to print.

  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);

  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, miner_acc, test_core_time::get_time());
  events.push_back(blk_0);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW * 2);

  m_amount_many_outs_have = get_outs_money_amount(blk_0r.miner_tx);

  MAKE_TX(events, tx_0, miner_acc, alice_acc, m_amount_many_outs_have + TESTS_DEFAULT_FEE, blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE + 6);
  
  DO_CALLBACK(events, "c1");

  return true;
}

bool gen_wallet_fake_outs_while_having_too_little_own_outs::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  {
    std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
    size_t blocks_fetched = 0;
    bool received_money;
    std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
    alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
    CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW * 2 + 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE + 6, false, "Incorrect numbers of blocks fetched");
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool");

    alice_wlt->reset_password(g_wallet_password);
    alice_wlt->store(g_wallet_filename);
  }

  const blockchain_storage::outputs_container& global_outputs = c.get_blockchain_storage().get_outputs_container();
  const size_t fake_outputs_count = 4;
  const size_t trials_count = 15;
  std::map<size_t, std::vector<size_t>> real_inputs; // map: amount -> vector of real inputs idx -> counter
  std::map<size_t, std::map<size_t, size_t>> real_inputs_block_height; // map: amount -> (map: keeper block height -> counter)
  size_t tries = 0;
  for (; tries < trials_count; ++tries)
  {
    std::shared_ptr<tools::wallet2> alice_wlt(new tools::wallet2);
    alice_wlt->load(g_wallet_filename, g_wallet_password);
    alice_wlt->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());
    alice_wlt->set_core_proxy(m_core_proxy);

    std::vector<tx_destination_entry> dest(1, tx_destination_entry(m_amount_many_outs_have - TESTS_DEFAULT_FEE, m_accounts[MINER_ACC_IDX].get_public_address()));
    transaction tx;
    alice_wlt->transfer(dest, fake_outputs_count, 0, TESTS_DEFAULT_FEE, std::vector<extra_v>(), std::vector<attachment_v>(), tx);

    std::vector<size_t> r_ins;
    bool r = determine_tx_real_inputs(c, tx, m_accounts[ALICE_ACC_IDX].get_keys(), r_ins);
    CHECK_AND_ASSERT_MES(r, false, "determine_tx_real_inputs failed");
    CHECK_AND_ASSERT_MES(r_ins.size() == tx.vin.size(), false, "Incorrect outs size");

    for (size_t i = 0; i < tx.vin.size(); ++i)
    {
      const txin_to_key& txin = boost::get<txin_to_key>(tx.vin[i]);
      uint64_t amount = txin.amount;
      size_t real_input = r_ins[i];
      auto& vec = real_inputs[amount];
      if (vec.empty())
        vec.resize(fake_outputs_count + 1);
      CHECK_AND_ASSERT_MES(real_input < fake_outputs_count + 1, false, "determine_tx_real_inputs returned incorrect input");
      CHECK_AND_ASSERT_MES(txin.key_offsets.size() == fake_outputs_count + 1, false, "");
      vec[real_input]++;

      // obtain block height for tx having this input as its output
      uint64_t amount_outs_size = global_outputs.get_item_size(txin.amount);
      CHECK_AND_ASSERT_MES(amount_outs_size, false, "Can't find amount " << txin.amount << " among global outputs");
      uint64_t gindex = 0;
      for (size_t j = real_input; j != std::numeric_limits<size_t>::max(); --j)
        gindex += boost::get<uint64_t>(txin.key_offsets[j]); // because key_offsets contains diff-packed indexes
      CHECK_AND_ASSERT_MES(gindex < amount_outs_size, false, "Global index " << gindex << " for input #" << real_input << " is out of bounds [0.." << amount_outs_size - 1 << "]");
      crypto::hash tx_hash = global_outputs.get_subitem(txin.amount, gindex)->tx_id;
      auto tce_ptr = c.get_blockchain_storage().get_tx_chain_entry(tx_hash);
      CHECK_AND_ASSERT_MES(tce_ptr, false, "get_tx_chain_entry failed to find tx by hash " << tx_hash);
      real_inputs_block_height[amount][tce_ptr->m_keeper_block_height]++;

      LOG_PRINT_MAGENTA(">>>> amount: " << std::setw(14) << amount << ", selected real input: " << std::setw(2) << real_input << ", refers to out with gindex: " << std::setw(3) << gindex << ", from block height: " << tce_ptr->m_keeper_block_height, LOG_LEVEL_0);
    }

    c.get_tx_pool().purge_transactions();
    CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "There are txs in the pool.");
  }

  // make sure that the same input was not used as real in the half of trials
  bool random_enough = true;
  for (auto item : real_inputs)
  {
    LOG_PRINT_L0("Amount: " << item.first);
    for (size_t i = 0; i < item.second.size(); ++i)
    {
      if (item.second[i] == 0)
        continue;
      LOG_PRINT_L0("    real index " << i << " has been selected " << item.second[i] << " times of " << tries);
      random_enough &= item.second[i] < tries / 2;
    }
    for (const auto& it : real_inputs_block_height[item.first])
    {
      LOG_PRINT_L0("    real output from block at " << it.first << " has been selected " << it.second << " times of " << tries);
      random_enough &= it.second < tries / 2;
    }
  }
  CHECK_AND_ASSERT_MES(random_enough, false, "Real inputs were selected quite predictable, they expected to be randomly distributed.");

  return true;

}

premine_wallet_test::premine_wallet_test()
{
  REGISTER_CALLBACK_METHOD(premine_wallet_test, c1);
}

bool premine_wallet_test::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);

  // create and add true genesis
  test_generator::block_info blk_0_info = AUTO_VAL_INIT(blk_0_info);
  currency::generate_genesis_block(blk_0_info.b);
  blk_0_info.already_generated_coins = get_outs_money_amount(blk_0_info.b.miner_tx);
  blk_0_info.block_size = get_object_blobsize(blk_0_info.b.miner_tx);
  blk_0_info.cumul_difficulty = DIFFICULTY_POW_STARTER;
  blk_0_info.ks_hash = currency::null_hash;
  blk_0_info.m_transactions.clear();
  events.push_back(blk_0_info.b);
  generator.add_block_info(blk_0_info);


  // manually construct and add a block to set correct timestamp (genesis has zero timestamp)
  test_generator::block_info blk_1_info = AUTO_VAL_INIT(blk_1_info);
  bool r = generator.construct_block_manually(blk_1_info.b, blk_0_info.b, miner_acc, test_generator::block_fields::bf_timestamp, 0, 0, test_core_time::get_time());
  CHECK_AND_ASSERT_MES(r, false, "construct_block_manually failed");
  blk_1_info.already_generated_coins = blk_0_info.already_generated_coins + get_outs_money_amount(blk_1_info.b.miner_tx);
  blk_1_info.block_size = get_object_blobsize(blk_1_info.b.miner_tx);
  blk_1_info.cumul_difficulty = generator.get_difficulty_for_next_block(get_block_hash(blk_0_info.b));
  blk_1_info.ks_hash = currency::null_hash;
  blk_1_info.m_transactions.clear();
  events.push_back(blk_1_info.b);
  generator.add_block_info(blk_1_info);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_1_info.b, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 4);

  DO_CALLBACK(events, "c1");

  return true;
}

bool premine_wallet_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  static const struct {
    const wchar_t* const filename;
    const char* const password;
    uint64_t amount;
  } wallet_files[] = {
    { L"10591197960882782053.bin", "12345", COIN * 10 * 1000 * 1000 },
    { L"11337635154464432297.bin", "12345", COIN * 10 * 1000 * 1000 },
    { L"12836673186898288796.bin", "12345", COIN * 10 * 1000 * 1000 },
    { L"1353631611246494722.bin",  "12345", COIN * 10 * 1000 * 1000 },
    { L"16026158939479747308.bin", "12345", COIN * 10 * 1000 * 1000 },
    { L"17548894975339876487.bin", "12345", COIN * 10 * 1000 * 1000 },
    { L"6515796793189395368.bin",  "12345", COIN * 10 * 1000 * 1000 },
    { L"7837182425983839079.bin",  "12345", COIN * 10 * 1000 * 1000 }
  };

  bool result = true;
  for (size_t i = 0; i < sizeof wallet_files / sizeof wallet_files[0]; ++i)
  {
    LOG_PRINT_CYAN(ENDL << " =+=+=+=+=+=+=+ wallet file: " << epee::string_encoding::convert_to_ansii(wallet_files[i].filename) << " =+=+=+=+=+=+=+ " << ENDL, LOG_LEVEL_0);
    try{
      bool r = check_wallet(c, wallet_files[i].filename, wallet_files[i].password, wallet_files[i].amount);
      CHECK_AND_ASSERT_MES(r, false, "check_wallet failed");
      result &= r;
    }
    catch (std::exception& e)
    {
      LOG_ERROR("Got exception: " << e.what() << ENDL << " Wallet file: " << epee::string_encoding::convert_to_ansii(wallet_files[i].filename));
      result = false;
    }
  }

  return result;
}

bool premine_wallet_test::check_wallet(currency::core& c, const wchar_t* wallet_filename, const char* password, uint64_t amount)
{
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "There are txs in the pool.");

  std::shared_ptr<tools::wallet2> alice_wlt(new tools::wallet2);
  alice_wlt->load(wallet_filename, password);
  alice_wlt->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());
  alice_wlt->set_core_proxy(m_core_proxy);

  LOG_PRINT_L0(alice_wlt->get_account());

  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 1 + CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 4, false, "Incorrect number of blocks fetched");

  if (!check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", amount))
    return false;

  alice_wlt->transfer(std::vector<tx_destination_entry>({ tx_destination_entry(COIN * 1, m_accounts[MINER_ACC_IDX].get_public_address()) }),
    0, 0, TESTS_DEFAULT_FEE, std::vector<extra_v>(), std::vector<attachment_v>());

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Transfer was unsuccessfull.");

  c.get_tx_pool().purge_transactions();
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "There are txs in the pool.");

  return true;
}


//------------------------------------------------------------------------------

mined_balance_wallet_test::mined_balance_wallet_test()
{
  REGISTER_CALLBACK_METHOD(mined_balance_wallet_test, c1);
  REGISTER_CALLBACK_METHOD(mined_balance_wallet_test, set_core_config);
}

bool mined_balance_wallet_test::generate(std::vector<test_event_entry>& events) const
{
  GENERATE_ACCOUNT(preminer_acc);
  GENERATE_ACCOUNT(miner_acc);
  m_accounts.push_back(miner_acc);
  GENERATE_ACCOUNT(alice_acc);
  m_accounts.push_back(alice_acc);

  block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, preminer_acc, test_core_time::get_time());
  events.push_back(blk_0);

  DO_CALLBACK(events, "set_core_config");
  DO_CALLBACK(events, "c1");

  return true;
}

bool mined_balance_wallet_test::set_core_config(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  core_runtime_config crc = c.get_blockchain_storage().get_core_runtime_config();
  crc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH;
  crc.min_coinstake_age = TESTS_POS_CONFIG_MIN_COINSTAKE_AGE;
  c.get_blockchain_storage().set_core_runtime_config(crc);
  return true;
}

bool mined_balance_wallet_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  blockchain_storage& bcs = c.get_blockchain_storage();

  core_runtime_config crc = bcs.get_core_runtime_config();
  crc.pos_minimum_heigh = TESTS_POS_CONFIG_POS_MINIMUM_HEIGH;
  bcs.set_core_runtime_config(crc);

  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*miner_wlt.get(), "miner", 0), false, "wrong balance");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "alice", 0), false, "wrong balance");
  
  uint64_t miner_mined_money = 0;
  bool r = false;
  std::list<currency::block> blocks;

  size_t n = CURRENCY_MINED_MONEY_UNLOCK_WINDOW;
  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, n);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");
  r = bcs.get_blocks(bcs.get_current_blockchain_size() - n, n, blocks);
  CHECK_AND_ASSERT_MES(r, false, "get_blocks failed");

  for (auto& b : blocks)
    miner_mined_money += get_outs_money_amount(b.miner_tx);

  miner_wlt->refresh();
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*miner_wlt.get(), "miner", miner_mined_money, miner_mined_money), false, "wrong balance");

  n = bcs.get_current_blockchain_size();
  r = miner_wlt->try_mint_pos();
  CHECK_AND_ASSERT_MES(r && bcs.get_current_blockchain_size() > n, false, "can't mint a PoS block");

  block b = AUTO_VAL_INIT(b);
  r = bcs.get_top_block(b);
  CHECK_AND_ASSERT_MES(r, false, "get_top_block failed");
  CHECK_AND_ASSERT_MES(b.miner_tx.vin.size() == 2, false, "Invalid PoS coinbase tx");

  uint64_t coinbase_outs_amount = get_outs_money_amount(b.miner_tx);
  uint64_t stake_amount = boost::get<txin_to_key>(b.miner_tx.vin[1]).amount;
  CHECK_AND_ASSERT_MES(coinbase_outs_amount > stake_amount, false, "coinbase_outs_amount = " << coinbase_outs_amount << ", stake_amount = " << stake_amount << " : invalid condition");
  
  miner_mined_money += coinbase_outs_amount - stake_amount;

  miner_wlt->refresh();

  std::stringstream ss;
  miner_wlt->dump_trunsfers(ss, false);
  LOG_PRINT_CYAN("miner transfers: " << ENDL << ss.str(), LOG_LEVEL_0);

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*miner_wlt.get(), "miner", miner_mined_money, miner_mined_money), false, "wrong balance");

  return true;
}

//------------------------------------------------------------------------------

wallet_outputs_with_same_key_image::wallet_outputs_with_same_key_image()
{
  REGISTER_CALLBACK_METHOD(wallet_outputs_with_same_key_image, c1);
}

bool wallet_outputs_with_same_key_image::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: make sure wallet does not take into account valid outputs having the same key image
  // Only one such output is spendable thus only one output should be taken into account.

  bool r = false;
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2);

  uint64_t tx_amount = MK_TEST_COINS(3);

  // tx_1
  std::vector<tx_source_entry> sources_1;
  r = fill_tx_sources(sources_1, events, blk_0r, miner_acc.get_keys(), tx_amount + TESTS_DEFAULT_FEE, 0);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");

  std::vector<tx_destination_entry> destinations{ tx_destination_entry(tx_amount, alice_acc.get_public_address()) };

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_acc.get_keys(), sources_1);
  builder.step3_fill_outputs(destinations);
  builder.step4_calc_hash();
  builder.step5_sign(sources_1);

  transaction tx_1 = builder.m_tx;
  events.push_back(tx_1);

  // tx_2 with the same secret key
  currency::keypair tmp_sec_key = builder.m_tx_key;
  builder.step1_init();
  builder.m_tx_key = tmp_sec_key;
  builder.m_tx.extra.clear();
  add_tx_pub_key_to_extra(builder.m_tx, builder.m_tx_key.pub);

  std::vector<tx_source_entry> sources_2;
  r = fill_tx_sources(sources_2, events, blk_0r, miner_acc.get_keys(), tx_amount + TESTS_DEFAULT_FEE, 0, sources_1);
  CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");

  // keep destinations the same

  builder.step2_fill_inputs(miner_acc.get_keys(), sources_2);
  builder.step3_fill_outputs(destinations);
  builder.step4_calc_hash();
  builder.step5_sign(sources_2);

  transaction tx_2 = builder.m_tx;
  events.push_back(tx_2);

  // make sure tx_1 and tx_2 have been created with the same tx key
  CHECK_AND_ASSERT_MES(get_tx_pub_key_from_extra(tx_1) == get_tx_pub_key_from_extra(tx_2), false, "internal error: tx_1 and tx_2 have different pub keys");

  // now both txs are in the pool, make sure they are
  DO_CALLBACK_PARAMS(events, "check_tx_pool_count", static_cast<size_t>(2));

  DO_CALLBACK(events, "c1");

  return true;
}

bool wallet_outputs_with_same_key_image::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, m_accounts[ALICE_ACC_IDX]);
  
  // check Alice has no unlocked coins
  bool r = refresh_wallet_and_check_balance("before tx_1 and tx_2 added", "Alice", alice_wlt, MK_TEST_COINS(3) * 2, true, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 2, 0);
  CHECK_AND_ASSERT_MES(r, false, "");

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "there are txs in the pool!");

  r = mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_blocks_in_playtime failed");

  // only one tx_1 output is counted as the tx_2 output has the very same key image
  r = refresh_wallet_and_check_balance("after tx_1 and tx_2 added", "Alice", alice_wlt, MK_TEST_COINS(3), true, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1, MK_TEST_COINS(3));
  CHECK_AND_ASSERT_MES(r, false, "");

  // make sure Alice is able to transfer her coins to smbd
  std::vector<tx_destination_entry> destinations{ tx_destination_entry(MK_TEST_COINS(3) - TESTS_DEFAULT_FEE, m_accounts[MINER_ACC_IDX].get_public_address()) };
  try
  {
    alice_wlt->transfer(destinations, 0, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment);
  }
  catch (...)
  {
    CHECK_AND_ASSERT_MES(false, false, "Alice failed to transfer all her funds");
  }

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "wrong tx count in the pool: " << c.get_pool_transactions_count());
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "there are txs in the pool!");

  return true;
}

//------------------------------------------------------------------------------

wallet_unconfirmed_tx_expiration::wallet_unconfirmed_tx_expiration()
{
  REGISTER_CALLBACK_METHOD(wallet_unconfirmed_tx_expiration, c1);
}

bool wallet_unconfirmed_tx_expiration::generate(std::vector<test_event_entry>& events) const
{
  // Test outline:
  // 1. Alice sends tx with expiration.
  // 2. Miner ignores Alice's tx, so tx expires in the pool.
  // 3. Tx is being removed from the pool due to expiration.
  // Make sure Alice eventually spent no coins and all her money is unlocked in the wallet.

  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);

  bool r = false;
  transaction tx_0 = AUTO_VAL_INIT(tx_0);
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), alice_acc.get_public_address(), TESTS_DEFAULT_FEE * 20, 10, TESTS_DEFAULT_FEE, tx_0);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs");
  events.push_back(tx_0);
  transaction tx_1 = AUTO_VAL_INIT(tx_1);
  r = construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), bob_acc.get_public_address(), TESTS_DEFAULT_FEE * 20, 10, TESTS_DEFAULT_FEE, tx_1);
  CHECK_AND_ASSERT_MES(r, false, "construct_tx_with_many_outputs");
  events.push_back(tx_1);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({ tx_0, tx_1 }));

  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

bool wallet_unconfirmed_tx_expiration::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);

  uint64_t alice_start_balance = TESTS_DEFAULT_FEE * 20;
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, alice_start_balance), false, "");

  // Alice constructs and sends tx with expiration time
  uint64_t expiration_time = c.get_blockchain_storage().get_tx_expiration_median() + TX_EXPIRATION_MEDIAN_SHIFT + 15;
  etc_tx_details_expiration_time extra_entry = AUTO_VAL_INIT(extra_entry);
  extra_entry.v = expiration_time;
  std::vector<extra_v> extra({ extra_entry }); // extra with expiration time
  std::vector<tx_destination_entry> destinations({ tx_destination_entry(TESTS_DEFAULT_FEE * 2, m_accounts[MINER_ACC_IDX].get_public_address()) });
  transaction tx = AUTO_VAL_INIT(tx);
  try
  {
    alice_wlt->transfer(destinations, 0, 0, TESTS_DEFAULT_FEE, extra, empty_attachment, tools::detail::ssi_digit, tools::tx_dust_policy(DEFAULT_DUST_THRESHOLD), tx);
  }
  catch (std::exception &e)
  {
    CHECK_AND_ASSERT_MES(false, false, "alice_wlt->transfer() caused an exception: " << e.what());
  }

  CHECK_AND_ASSERT_MES(get_tx_expiration_time(tx) == expiration_time, false, "tx expiration time wasn't set");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Invalid txs count in the pool: " << c.get_pool_transactions_count());

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("tx sent,", "Alice", alice_wlt, alice_start_balance - TESTS_DEFAULT_FEE * 2 - TESTS_DEFAULT_FEE, true, 0, UINT64_MAX, 0, 0, TESTS_DEFAULT_FEE * 2), false, "");

  // mine a few block with no tx, so Alice's tx is expired in the pool
  for (size_t i = 0; i < 5; ++i)
  {
    r = mine_next_pow_block_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), c, std::vector<transaction>());
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime_with_given_txs failed");
  }

  // tx is still there
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Invalid txs count in the pool: " << c.get_pool_transactions_count());

  // make sure expiration median was shifted enough
  CHECK_AND_ASSERT_MES(c.get_blockchain_storage().is_tx_expired(tx), false, "wrong expiration time condition");

  LOG_PRINT_CYAN("%%%%% tx_pool::on_idle()", LOG_LEVEL_0);
  c.get_tx_pool().on_idle();

  // make sure tx was removed by the pool
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Invalid txs count in the pool: " << c.get_pool_transactions_count());

  // mine one more block to trigger wallet's on_idle() and outdated tx clearing
  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime failed");

  // make sure all Alice's money are unlocked and no coins were actually spent
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("tx expired and removed from the pool,", "Alice", alice_wlt, alice_start_balance, true, 6, alice_start_balance, 0, 0, 0), false, "");

  return true;
}

//------------------------------------------------------------------------------

wallet_chain_switch_with_spending_the_same_ki::wallet_chain_switch_with_spending_the_same_ki()
{
  REGISTER_CALLBACK_METHOD(wallet_chain_switch_with_spending_the_same_ki, c1);
}

bool wallet_chain_switch_with_spending_the_same_ki::generate(std::vector<test_event_entry>& events) const
{
  // Test outline
  // 1. A wallet has one unspent output
  // 2. wallet2::transfer() creates tx_0 that spends wallet's output
  // 3. tx_0 is successfully put into the blockchain
  // 4. Due to chain switch tx_0 is removed from the blockchain and get into the transaction pool
  // 5. Make sure the wallet can't spend that output
  // 6. After tx is expired make sure the wallet can spend that output


  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  MAKE_TX(events, tx_0, miner_acc, alice_acc, MK_TEST_COINS(30), blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  // rewind blocks to allow wallet be able to spend the coins
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

bool wallet_chain_switch_with_spending_the_same_ki::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(30), true, UINT64_MAX, MK_TEST_COINS(30)), false, "");

  std::vector<tx_destination_entry> destinations { tx_destination_entry(MK_TEST_COINS(30) - TESTS_DEFAULT_FEE, m_accounts[BOB_ACC_IDX].get_public_address()) };
  try
  {
    // create tx_1
    alice_wlt->transfer(destinations, 0, 0, TESTS_DEFAULT_FEE, empty_extra, empty_attachment);
  }
  catch (std::exception &e)
  {
    CHECK_AND_ASSERT_MES(false, false, "alice_wlt->transfer() caused an exception: " << e.what());
  }

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Tx pool has incorrect number of txs: " << c.get_pool_transactions_count());

  // mine blk_2 on height 22
  CHECK_AND_ASSERT_MES(mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c), false, "");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Tx pool has incorrect number of txs: " << c.get_pool_transactions_count());

  // refresh wallet
  alice_wlt->refresh();
  // DO NOT scan_tx_pool here intentionally
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", MK_TEST_COINS(0)), false, "");

  uint64_t blk_1r_height = c.get_top_block_height() - 1;
  crypto::hash blk_1r_id = c.get_block_id_by_height(blk_1r_height);
  block blk_2a = AUTO_VAL_INIT(blk_2a);
  r = mine_next_pow_block_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), c, std::vector<transaction>(), blk_1r_id, blk_1r_height + 1, &blk_2a);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime_with_given_txs failed");

  // one more to trigger chain switch
  block blk_3a = AUTO_VAL_INIT(blk_3a);
  r = mine_next_pow_block_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), c, std::vector<transaction>(), get_block_hash(blk_2a), get_block_height(blk_2a) + 1, &blk_3a);
  CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime_with_given_txs failed");

  // make sure tx_1 has been moved back to the pool
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());
  CHECK_AND_ASSERT_MES(c.get_alternative_blocks_count() == 1, false, "Incorrect alt blocks count: " << c.get_alternative_blocks_count());

  //const transaction& tx_1 = boost::get<transaction>(events[4 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3]);

  // refresh wallet
  alice_wlt->refresh();
  // DO NOT scan_tx_pool here intentionally
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt, "Alice", MK_TEST_COINS(0)), false, "");

  return true;
}

//------------------------------------------------------------------------------

wallet_unconfimed_tx_balance::wallet_unconfimed_tx_balance()
{
  REGISTER_CALLBACK_METHOD(wallet_unconfimed_tx_balance, c1);
}

bool wallet_unconfimed_tx_balance::generate(std::vector<test_event_entry>& events) const
{
  // Test outline:
  // 1. Miner sends 100 coins to Alice (50 + 50)
  // 2. Alice sends 30 back to Miner (tx is unconfirmed)
  // 3. Make sure Alice's wallet has correct balance, when it is checked from wallet's callback
  // 4. Few blocks are mined so the tx is get confirmed
  // 5. Make sure Alice's balance has changed correctly

  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  MAKE_TX(events, tx_0, miner_acc, alice_acc, MK_TEST_COINS(50), blk_0r);
  MAKE_TX(events, tx_1, miner_acc, alice_acc, MK_TEST_COINS(50), blk_0r);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({ tx_0, tx_1 }));

  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  DO_CALLBACK(events, "c1");

  return true;
}

bool wallet_unconfimed_tx_balance::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(100), false, UINT64_MAX, MK_TEST_COINS(100)), false, "");

  bool callback_is_ok = false;
  // this callback will ba called from within wallet2::transfer() below
  std::shared_ptr<wlt_lambda_on_transfer2_wrapper> l(new wlt_lambda_on_transfer2_wrapper(
    [&callback_is_ok](const tools::wallet_public::wallet_transfer_info& wti, const std::list<tools::wallet_public::asset_balance_entry>& balances, uint64_t total_mined) -> bool
    {
    tools::wallet_public::asset_balance_entry abe = get_native_balance_entry(balances);
    uint64_t balance = abe.total;
    uint64_t unlocked_balance = abe.unlocked;

      CHECK_AND_ASSERT_MES(balance == MK_TEST_COINS(70), false, "invalid balance: " << print_money_brief(balance));
      CHECK_AND_ASSERT_MES(unlocked_balance == MK_TEST_COINS(50), false, "invalid unlocked_balance: " << print_money_brief(unlocked_balance));
      CHECK_AND_ASSERT_MES(total_mined == 0, false, "invalid total_mined: " << print_money_brief(total_mined));
      callback_is_ok = true;
      return true;
    }
  ));
  alice_wlt->callback(l);

  uint64_t fee = TESTS_DEFAULT_FEE * 3;
  std::vector<tx_destination_entry> destinations{ tx_destination_entry(MK_TEST_COINS(30) - fee, m_accounts[MINER_ACC_IDX].get_public_address()) };
  try
  {
    alice_wlt->transfer(destinations, 0, 0, fee, empty_extra, empty_attachment);
  }
  catch (std::exception &e)
  {
    CHECK_AND_ASSERT_MES(false, false, "alice_wlt->transfer() caused an exception: " << e.what());
  }

  CHECK_AND_NO_ASSERT_MES(callback_is_ok, false, "callback failed");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Tx pool has incorrect number of txs: " << c.get_pool_transactions_count());

  // 50 coins should be locked and 50 - unlocked
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(70), false, UINT64_MAX, MK_TEST_COINS(50), 0, 0, MK_TEST_COINS(30) - fee), false, "");

  // mine WALLET_DEFAULT_TX_SPENDABLE_AGE blocks so the tx get confirmed and coins get unlocked
  CHECK_AND_ASSERT_MES(mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, WALLET_DEFAULT_TX_SPENDABLE_AGE), false, "");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Tx pool has incorrect number of txs: " << c.get_pool_transactions_count());

  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(70), false, UINT64_MAX, MK_TEST_COINS(70), 0, 0, 0), false, "");

  return true;
}

//------------------------------------------------------------------------------

packing_outputs_on_pos_minting_wallet::packing_outputs_on_pos_minting_wallet()
{
  REGISTER_CALLBACK_METHOD(packing_outputs_on_pos_minting_wallet, c1);
}

bool packing_outputs_on_pos_minting_wallet::generate(std::vector<test_event_entry>& events) const
{
  // Test ideas:
  // 1) UTXO defragmentation tx cannot select stake output by accident;
  // 2) if UTXO defragmentation tx cannot be constructed because of too few decoy outputs, PoS can still be created;
  // 3) UTXO defragmentation tx limits works as expected.

  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();

  // don't use MAKE_GENESIS_BLOCK here because it will mask 'generator'
  currency::block blk_0 = AUTO_VAL_INIT(blk_0);
  generator.construct_genesis_block(blk_0, miner_acc, test_core_time::get_time());
  events.push_back(blk_0);

  DO_CALLBACK(events, "configure_core");

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  m_single_amount = TESTS_DEFAULT_FEE * 5;

  MAKE_TX(events, tx_0, miner_acc, alice_acc, m_single_amount, blk_0r);
  MAKE_TX(events, tx_1, miner_acc, alice_acc, m_single_amount, blk_0r);
  MAKE_TX(events, tx_2, miner_acc, alice_acc, m_single_amount, blk_0r);
  MAKE_TX(events, tx_3, miner_acc, alice_acc, m_single_amount, blk_0r);
  MAKE_TX(events, tx_4, miner_acc, alice_acc, m_single_amount, blk_0r);
  MAKE_TX(events, tx_5, miner_acc, bob_acc,   m_single_amount, blk_0r);
  MAKE_TX(events, tx_6, miner_acc, bob_acc,   m_single_amount, blk_0r);

  m_alice_initial_balance = m_single_amount * 5;
  m_bob_initial_balance   = m_single_amount * 2;

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({tx_0, tx_1, tx_2, tx_3, tx_4, tx_5, tx_6}));

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  CREATE_TEST_WALLET(alice_wlt, alice_acc, blk_0);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, alice_wlt, blk_1r, CURRENCY_MINED_MONEY_UNLOCK_WINDOW * 2 + 1);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(alice_wlt, m_alice_initial_balance);

  CREATE_TEST_WALLET(bob_wlt, bob_acc, blk_0);
  REFRESH_TEST_WALLET_AT_GEN_TIME(events, bob_wlt, blk_1r, CURRENCY_MINED_MONEY_UNLOCK_WINDOW * 2 + 1);
  CHECK_TEST_WALLET_BALANCE_AT_GEN_TIME(bob_wlt, m_bob_initial_balance);

  DO_CALLBACK(events, "c1");

  return true;
}

bool packing_outputs_on_pos_minting_wallet::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  size_t blocks_fetched = 0;
  bool received_money;
  std::atomic<bool> atomic_false = ATOMIC_VAR_INIT(false);

  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW * 2 + 1, false, "Incorrect numbers of blocks fetched: " << blocks_fetched);
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", m_alice_initial_balance, 0, m_alice_initial_balance, 0, 0), false, "");

  bob_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == CURRENCY_MINED_MONEY_UNLOCK_WINDOW * 2 + 1, false, "Incorrect numbers of blocks fetched: " << blocks_fetched);
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "bob_wlt", m_bob_initial_balance, 0, m_bob_initial_balance, 0, 0), false, "");

  // 1. Try to defragment the same UTXO that is used for staking
  // (Bob has two: one UTXO is for staking, other is being defragmented)
  bob_wlt->set_defragmentation_tx_settings(true, 1, 10, CURRENCY_BLOCK_REWARD, 0);
  bob_wlt->try_mint_pos();

  CHECK_AND_ASSERT_MES(c.get_current_blockchain_size() == CURRENCY_MINED_MONEY_UNLOCK_WINDOW * 2 + 3, false, "Incorrect blockchain height:" << c.get_current_blockchain_size());
  bob_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 1, false, "Incorrect numbers of blocks fetched: " << blocks_fetched);

  // (also make sure that unlocked balance is zero)
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "bob_wlt", m_bob_initial_balance + CURRENCY_BLOCK_REWARD, INVALID_BALANCE_VAL, 0), false, "");

  
  // 2. Try to mine a PoS block and defragment some of UTXO
  alice_wlt->set_defragmentation_tx_settings(true, 2, 2, CURRENCY_BLOCK_REWARD, 0);
  alice_wlt->try_mint_pos();

  CHECK_AND_ASSERT_MES(c.get_current_blockchain_size() == CURRENCY_MINED_MONEY_UNLOCK_WINDOW * 2 + 4, false, "Incorrect blockchain height:" << c.get_current_blockchain_size());
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 2, false, "Incorrect numbers of blocks fetched: " << blocks_fetched);

  // (also make sure that only two UTXOs is unlocked)
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", m_alice_initial_balance + CURRENCY_BLOCK_REWARD, INVALID_BALANCE_VAL, m_single_amount * 2), false, "");


  // 3. Try to mine a PoS block and defragment with huge decoy set. Make sure block is mined successfully without a defragmentation tx
  // Alice has one UTXO
  alice_wlt->set_defragmentation_tx_settings(true, 1, 1, CURRENCY_BLOCK_REWARD, 80);
  alice_wlt->try_mint_pos();

  CHECK_AND_ASSERT_MES(c.get_current_blockchain_size() == CURRENCY_MINED_MONEY_UNLOCK_WINDOW * 2 + 5, false, "Incorrect blockchain height:" << c.get_current_blockchain_size());
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 1, false, "Incorrect numbers of blocks fetched: " << blocks_fetched);

  // Alice's unlocked balance should consist only of one UTXO
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", m_alice_initial_balance + CURRENCY_BLOCK_REWARD * 2, INVALID_BALANCE_VAL, m_single_amount), false, "");


  // 4. Finally mine a PoS and defragment the last one unlocked UTXO
  alice_wlt->set_defragmentation_tx_settings(true, 1, 1, CURRENCY_BLOCK_REWARD, 0);
  alice_wlt->try_mint_pos();

  CHECK_AND_ASSERT_MES(c.get_current_blockchain_size() == CURRENCY_MINED_MONEY_UNLOCK_WINDOW * 2 + 6, false, "Incorrect blockchain height:" << c.get_current_blockchain_size());
  alice_wlt->refresh(blocks_fetched, received_money, atomic_false);
  CHECK_AND_ASSERT_MES(blocks_fetched == 1, false, "Incorrect numbers of blocks fetched: " << blocks_fetched);

  // Alice's unlocked balance should be zero
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "alice_wlt", m_alice_initial_balance + CURRENCY_BLOCK_REWARD * 3, INVALID_BALANCE_VAL, 0), false, "");

  return true;
}

//------------------------------------------------------------------------------

wallet_sending_to_integrated_address::wallet_sending_to_integrated_address()
{
  REGISTER_CALLBACK_METHOD(wallet_sending_to_integrated_address, c1);
}

bool wallet_sending_to_integrated_address::generate(std::vector<test_event_entry>& events) const
{
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate();

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK(events, "c1");

  return true;
}

bool wallet_sending_to_integrated_address::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);

  miner_wlt->refresh();

  std::string payment_id = "super-payment-id-1948503537205028248";
  std::string alice_integrated_address = get_account_address_and_payment_id_as_str(m_accounts[ALICE_ACC_IDX].get_public_address(), payment_id);

  bool callback_succeded = false;
  std::shared_ptr<wlt_lambda_on_transfer2_wrapper> l(new wlt_lambda_on_transfer2_wrapper(
    [&](const tools::wallet_public::wallet_transfer_info& wti, const std::list<tools::wallet_public::asset_balance_entry>& balances, uint64_t total_mined) -> bool {
    LOG_PRINT_YELLOW("on_transfer: " << print_money_brief(wti.get_native_amount()) << " pid len: " << wti.payment_id.size() << " remote addr: " << (wti.remote_addresses.size() > 0 ? wti.remote_addresses[0] : ""), LOG_LEVEL_0);
    if (wti.payment_id.empty())
      return true; // skip another outputs
    CHECK_AND_ASSERT_MES(wti.payment_id == payment_id, false, "incorrect payment id");
    CHECK_AND_ASSERT_MES(wti.remote_addresses.size() == 1, false, "remote_addressed.size() = " << wti.remote_addresses.size());
    CHECK_AND_ASSERT_MES(wti.remote_addresses[0] == alice_integrated_address, false, "incorrect remote address");
    callback_succeded = true;
    return true;
  }
  ));
  miner_wlt->callback(l);

  std::vector<tx_destination_entry> destinations;
  destinations.push_back(tx_destination_entry(MK_TEST_COINS(2), m_accounts[ALICE_ACC_IDX].get_public_address()));

  std::vector<payload_items_v> extra, attachments;
  // add tx_receiver to show the recipient in wti
  uint64_t top_block_height = 0;
  crypto::hash stub_hash;
  r = c.get_blockchain_top(top_block_height, stub_hash);
  CHECK_AND_ASSERT_MES(r, false, "get_blockchain_top failed");
  currency::create_and_add_tx_receiver_to_container_from_address(extra, m_accounts[ALICE_ACC_IDX].get_public_address(), top_block_height, miner_wlt->get_core_runtime_config());
  // add payment_id
  set_payment_id_to_tx(attachments, payment_id);

  
  callback_succeded = false;
  miner_wlt->transfer(destinations, /* fake outs */ 0, /* unlock time */ 0, TESTS_DEFAULT_FEE, extra, attachments);
  CHECK_AND_ASSERT_MES(callback_succeded, false, "callback was not succeded");

  refresh_wallet_and_check_balance("", "alice", alice_wlt, MK_TEST_COINS(2));

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 1, false, "Tx pool has incorrect number of txs: " << c.get_pool_transactions_count());
  CHECK_AND_ASSERT_MES(mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c), false, "");
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Tx pool has incorrect number of txs: " << c.get_pool_transactions_count());

  // check one again with normal sync (callback should be called as well)
  std::shared_ptr<tools::wallet2> miner_wlt_2 = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  miner_wlt_2->callback(l);

  callback_succeded = false;
  miner_wlt_2->refresh();
  CHECK_AND_ASSERT_MES(callback_succeded, false, "callback was not succeded (2)");

  return true;
}

//------------------------------------------------------------------------------

wallet_watch_only_and_chain_switch::wallet_watch_only_and_chain_switch()
  : m_split_point_block_height(0)
{
  REGISTER_CALLBACK_METHOD(wallet_watch_only_and_chain_switch, c1);
}

bool wallet_watch_only_and_chain_switch::generate(std::vector<test_event_entry>& events) const
{

  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.make_account_watch_only();
  account_base& bob_acc = m_accounts[BOB_ACC_IDX];   bob_acc.generate(true); bob_acc.make_account_watch_only(); // Bob gonna have a tracking wallet

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);


  //  0     10    11    12    13    23    15         <- blockchain height
  // (0 )- (0r)- (1 )- (2 )- (3 )- (3r)-             <- main chain
  //              |          tx_0
  //              \          tx_1
  //               \
  //                \- (2a)- (3a)- (3ar)- 


  MAKE_NEXT_BLOCK(events, blk_1, blk_0r, miner_acc);
  MAKE_NEXT_BLOCK(events, blk_2, blk_1, miner_acc);

  MAKE_TX_FEE(events, tx_0, miner_acc, alice_acc, MK_TEST_COINS(9), TESTS_DEFAULT_FEE, blk_2);
  MAKE_TX_FEE(events, tx_1, miner_acc, bob_acc, MK_TEST_COINS(7), TESTS_DEFAULT_FEE, blk_2);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3, blk_2, miner_acc, std::list<transaction>({ tx_0, tx_1 }));

  REWIND_BLOCKS_N(events, blk_3r, blk_3, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  m_split_point_block_id = get_block_hash(blk_1);
  m_split_point_block_height = get_block_height(blk_1);

  DO_CALLBACK(events, "c1");

  return true;
}

bool wallet_watch_only_and_chain_switch::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt   = init_playtime_test_wallet(events, c, BOB_ACC_IDX);

  // make sure Alice's wallet is watch-only but not a tracking wallet (not auditable)
  CHECK_AND_ASSERT_MES(alice_wlt->is_watch_only() && !alice_wlt->is_auditable(), false, "incorrect type of Alice's wallet");
  // make sure Bob's wallet is a tracking wallet
  CHECK_AND_ASSERT_MES(bob_wlt->is_watch_only() && bob_wlt->is_auditable(), false, "incorrect type of Bob's wallet");

  miner_wlt->refresh();
  
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // check the balances
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(9), false, UINT64_MAX, MK_TEST_COINS(9), 0, 0, 0), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt,     MK_TEST_COINS(7), false, UINT64_MAX, MK_TEST_COINS(7), 0, 0, 0), false, "");

  // make a split
  block out_block;
  crypto::hash prev_id = m_split_point_block_id;
  uint64_t current_height = m_split_point_block_height + 1;

  for (size_t i = 0; i < CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 3; ++i)
  {
    r = mine_next_pow_block_in_playtime_with_given_txs(m_accounts[MINER_ACC_IDX].get_public_address(), c, std::vector<transaction>(), prev_id, current_height, &out_block);
    CHECK_AND_ASSERT_MES(r, false, "mine_next_pow_block_in_playtime_with_given_txs failed");
    current_height++;
    prev_id = get_block_hash(out_block);
  }

  // make sure the split has happened
  uint64_t top_height = 0;
  crypto::hash top_id;
  c.get_blockchain_top(top_height, top_id);
  CHECK_AND_ASSERT_MES(top_height == current_height - 1, false, "incorrect top_hegiht = " << top_height);
  CHECK_AND_ASSERT_MES(top_id == prev_id, false, "incorrect top id = " << top_id);

  // tx_0 and tx_1 should be moved back to the pool
  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 2, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  // check the balances, should be zero in actual and few coins in awaiting
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(9), false, UINT64_MAX, 0, 0, MK_TEST_COINS(9), 0), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, MK_TEST_COINS(7), false, UINT64_MAX, 0, 0, MK_TEST_COINS(7), 0), false, "");


  return true;
}

//------------------------------------------------------------------------------

wallet_spend_form_auditable_and_track::wallet_spend_form_auditable_and_track()
{
  REGISTER_CALLBACK_METHOD(wallet_spend_form_auditable_and_track, c1);
}

bool wallet_spend_form_auditable_and_track::generate(std::vector<test_event_entry>& events) const
{

  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate();
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(true); // Alice has auditable wallet
  account_base& bob_acc = m_accounts[BOB_ACC_IDX];   bob_acc = alice_acc; bob_acc.make_account_watch_only(); // Bob has Alice's tracking wallet

  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, test_core_time::get_time());
  REWIND_BLOCKS_N(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  MAKE_TX(events, tx_1, miner_acc, alice_acc, MK_TEST_COINS(5), blk_0r);

  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_1);
  REWIND_BLOCKS_N(events, blk_1r, blk_1, miner_acc, WALLET_DEFAULT_TX_SPENDABLE_AGE);

  std::vector<attachment_v> attachments;
  tx_comment comment_attachment = AUTO_VAL_INIT(comment_attachment);
  m_comment = "Jokes are funny!";
  comment_attachment.comment = m_comment;
  attachments.push_back(comment_attachment);
  MAKE_TX_ATTACH(events, tx_2, alice_acc, miner_acc, MK_TEST_COINS(1), blk_1r, attachments);

  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_2);

  DO_CALLBACK(events, "c1");

  return true;
}

bool wallet_spend_form_auditable_and_track::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);

  // make sure Alice's wallet is autibale and not watch-only
  CHECK_AND_ASSERT_MES(!alice_wlt->is_watch_only() && alice_wlt->is_auditable(), false, "incorrect type of Alice's wallet");
  // make sure Bob's wallet is a tracking wallet
  CHECK_AND_ASSERT_MES(bob_wlt->is_watch_only() && bob_wlt->is_auditable(), false, "incorrect type of Bob's wallet");

  const account_public_address& bob_addr   = bob_wlt->get_account().get_public_address();
  const account_public_address& alice_addr = alice_wlt->get_account().get_public_address();

  // make sure their addresses are linked indeed
  CHECK_AND_ASSERT_MES(bob_addr.view_public_key == alice_addr.view_public_key && bob_addr.spend_public_key == alice_addr.spend_public_key, false,
    "Bob's tracking wallet address is not linked with Alice's one");

  CHECK_AND_ASSERT_MES(c.get_pool_transactions_count() == 0, false, "Incorrect txs count in the pool: " << c.get_pool_transactions_count());

  bob_wlt->refresh();

  // check the balances
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, MK_TEST_COINS(3), false, UINT64_MAX, 0, 0, 0, 0), false, "");
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Bob", bob_wlt, MK_TEST_COINS(3), false, UINT64_MAX, 0, 0, 0, 0), false, "");

  r = false;
  bool r_comment = false;
  bob_wlt->enumerate_transfers_history([&](const tools::wallet_public::wallet_transfer_info& wti) {
    if (wti.get_native_amount() == MK_TEST_COINS(5))
    {
      r_comment = (wti.comment == m_comment);
      if (!r_comment)
        return false; // stop
    }
    return true; // continue
  }, true);
  CHECK_AND_ASSERT_MES(r, false, "cannot get comment from tx");
  CHECK_AND_ASSERT_MES(r_comment, false, "wrong comment got from tx");

  return true;
}

//------------------------------------------------------------------------------

wallet_and_sweep_below::wallet_and_sweep_below()
{
  REGISTER_CALLBACK_METHOD(wallet_and_sweep_below, c1);
}

bool wallet_and_sweep_below::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: basic check for wallet2::sweep_below() functionality 

  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base preminer_acc;
  preminer_acc.generate();
  preminer_acc.set_createtime(ts);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, ts);
  DO_CALLBACK(events, "configure_core");

  MAKE_NEXT_BLOCK(events, blk_1, blk_0, preminer_acc);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, 3 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW - 1);

  DO_CALLBACK(events, "c1");
  return true;
}

bool wallet_and_sweep_below::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);

  uint64_t miner_balance = (3 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW - 1) * COIN;
  uint64_t unlocked_miner_balance = miner_balance - (CURRENCY_MINED_MONEY_UNLOCK_WINDOW - 1) * COIN;
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Miner", miner_wlt, miner_balance, false, SIZE_MAX, unlocked_miner_balance), false, "");

  size_t outs_total = 0;
  size_t amount_total = 0;
  size_t outs_swept = 0;
  size_t amount_swept = 0;
  transaction tx{};
  miner_wlt->sweep_below(10 /* <- decoys */, m_accounts[ALICE_ACC_IDX].get_public_address(), COIN + 1, payment_id_t(), TESTS_DEFAULT_FEE, outs_total, amount_total, outs_swept, amount_swept, &tx);

  CHECK_AND_ASSERT_MES(amount_swept == amount_total, false, "amount_swept != amount_total");
  CHECK_AND_ASSERT_MES(amount_swept == COIN * (2 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW), false, "amount_swept = " << amount_swept);

  mine_next_pow_blocks_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c, 1);

  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX);
  CHECK_AND_ASSERT_MES(refresh_wallet_and_check_balance("", "Alice", alice_wlt, amount_swept - TESTS_DEFAULT_FEE, false, SIZE_MAX, 0), false, "");

  return true;
}


//------------------------------------------------------------------------------
block_template_blacklist_test::block_template_blacklist_test()
{
  REGISTER_CALLBACK_METHOD(block_template_blacklist_test, c1);
}

bool block_template_blacklist_test::generate(std::vector<test_event_entry>& events) const
{
  // Test idea: basic check for wallet2::sweep_below() functionality 

  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base preminer_acc;
  preminer_acc.generate();
  preminer_acc.set_createtime(ts);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  MAKE_GENESIS_BLOCK(events, blk_0, preminer_acc, ts);
  DO_CALLBACK(events, "configure_core");

  MAKE_NEXT_BLOCK(events, blk_1, blk_0, preminer_acc);

  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, 3 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW - 1);

  DO_CALLBACK(events, "c1");
  return true;
}

bool block_template_blacklist_test::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  bool r = false;
  std::shared_ptr<tools::wallet2> miner_wlt = init_playtime_test_wallet(events, c, MINER_ACC_IDX);
  std::shared_ptr<tools::wallet2> alice_wlt = init_playtime_test_wallet(events, c, ALICE_ACC_IDX); 

  miner_wlt->refresh();
  miner_wlt->transfer(COIN / 10, alice_wlt->get_account().get_public_address());
  miner_wlt->transfer(COIN / 10, alice_wlt->get_account().get_public_address());
  
  //take first transaction and corrupt it intentionalyy
  std::list<transaction> txs;
  c.get_tx_pool().get_transactions(txs);
  CHECK_AND_ASSERT_MES(txs.size() == 2, false, "wrong tx count");


  txs.resize(1);
  currency::transaction broken_tx;
  uint64_t blob_size = 0;
  uint64_t fee = 0;
  r = c.get_tx_pool().take_tx(currency::get_transaction_hash(*txs.begin()), broken_tx, blob_size, fee);
  CHECK_AND_ASSERT_MES(r, false, "failed to take from pool");
  

  broken_tx.signatures.resize(broken_tx.signatures.size() - 1);
  //manually add completely broken tx to pool
  c.get_tx_pool().do_insert_transaction(broken_tx, get_transaction_hash(broken_tx), currency::get_object_blobsize(broken_tx), false, get_tx_fee(broken_tx), c.get_block_id_by_height(0), 0);
  
  CHECK_AND_ASSERT_MES(c.get_tx_pool().get_transactions_count() == 2, false, "wrong tx count");
  

  currency::create_block_template_params cbtp = AUTO_VAL_INIT(cbtp);
  cbtp.miner_address = miner_wlt->get_account().get_public_address();
  
  {
    currency::create_block_template_response cbtr = AUTO_VAL_INIT(cbtr);
    r = c.get_block_template(cbtp, cbtr);

    CHECK_AND_ASSERT_MES(r, false, "failed to create block template");
    CHECK_AND_ASSERT_MES(cbtr.b.tx_hashes.size() == 2, false, "failed to create block template");
  }

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(!r, false, "Unexpectedly created block");

  //now let's check if broken tx actually added to next blocktemplate

  {
    currency::create_block_template_response cbtr = AUTO_VAL_INIT(cbtr);
    r = c.get_block_template(cbtp, cbtr);

    CHECK_AND_ASSERT_MES(r, false, "failed to create block template");
    CHECK_AND_ASSERT_MES(cbtr.b.tx_hashes.size() == 1, false, "failed to create block template");

  }

  r = mine_next_pow_block_in_playtime(m_accounts[MINER_ACC_IDX].get_public_address(), c);
  CHECK_AND_ASSERT_MES(r, false, "Unexpectedly failed to create block");


  return true;
}
