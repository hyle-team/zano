// Copyright (c) 2023-2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "chaingen.h"
#include "hard_fork_4.h"
#include "random_helper.h"

using namespace currency;

namespace currency
{
  bool construct_tx(const account_keys& sender_account_keys, const std::vector<tx_source_entry>& sources,
    const std::vector<tx_destination_entry>& destinations,
    const std::vector<extra_v>& extra,
    const std::vector<attachment_v>& attachments,
    transaction& tx,
    uint64_t tx_version,
    crypto::secret_key& one_time_secret_key,
    uint64_t unlock_time,
    uint64_t expiration_time,
    uint8_t tx_outs_attr,
    bool shuffle,
    uint64_t flags,
    uint64_t explicit_consolidated_tx_fee,
    tx_generation_context& gen_context)
  {
    //extra copy operation, but creating transaction is not sensitive to this
    finalize_tx_param ftp{};
    ftp.tx_version = tx_version;
    ftp.sources = sources;
    ftp.prepared_destinations = destinations;
    ftp.extra = extra;
    ftp.attachments = attachments;
    ftp.unlock_time = unlock_time;
    // ftp.crypt_address = crypt_destination_addr;
    ftp.expiration_time = expiration_time;
    ftp.tx_outs_attr = tx_outs_attr;
    ftp.shuffle = shuffle;
    ftp.flags = flags;
    ftp.mode_separate_fee = explicit_consolidated_tx_fee;

    finalized_tx ft = AUTO_VAL_INIT(ft);
    ft.tx = tx;
    ft.one_time_key = one_time_secret_key;
    ftp.gen_context = gen_context; // ftp, not ft here, this is UGLY -- sowle
    bool r = construct_tx(sender_account_keys, ftp, ft);
    tx = ft.tx;
    one_time_secret_key = ft.one_time_key;
    gen_context = ft.ftp.gen_context;
    return r;
  }
} // namespace currency

void add_flags_to_all_destination_entries(const uint64_t flags, std::vector<currency::tx_destination_entry>& destinations)
{
  for(auto& de : destinations)
    de.flags |= flags;
}

//-------------------------------


hard_fork_4_consolidated_txs::hard_fork_4_consolidated_txs()
{
  REGISTER_CALLBACK_METHOD(hard_fork_4_consolidated_txs, c1);
}

bool hard_fork_4_consolidated_txs::generate(std::vector<test_event_entry>& events) const
{
  bool r = false;

  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  account_base& bob_acc   = m_accounts[BOB_ACC_IDX];   bob_acc.generate();   bob_acc.set_createtime(ts);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // necessary for the test to be run by GENERATE_AND_PLAY_HF

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  m_post_hf4_zarcanum = get_hardforks().get_the_most_recent_hardfork_id_for_height(CURRENCY_MINED_MONEY_UNLOCK_WINDOW) >= ZANO_HARDFORK_04_ZARCANUM;

  uint64_t alice_amount = MK_TEST_COINS(50);
  MAKE_TX(events, tx_0a, miner_acc, alice_acc, alice_amount, blk_0r);
  // tx_0b is only needed for decoy outputs with amount = alice_amount (important only for pre-HF4)
  transaction tx_0b{};
  construct_tx_with_many_outputs(m_hardforks, events, blk_0r, miner_acc.get_keys(), miner_acc.get_public_address(), alice_amount * 10, 10, TESTS_DEFAULT_FEE, tx_0b);
  ADD_CUSTOM_EVENT(events, tx_0b);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_1, blk_0r, miner_acc, std::list<transaction>({tx_0a, tx_0b}));

  size_t dhc = count_type_in_variant_container<tx_derivation_hint>(tx_0b.extra);
  CHECK_AND_ASSERT_MES(dhc == tx_0b.vout.size(), false, "unexpected derivation hints count: " << dhc);


  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // check Alice's balance
  std::shared_ptr<tools::wallet2> alice_wlt;
  r = generator.init_test_wallet(alice_acc, get_block_hash(blk_0), alice_wlt);
  CHECK_AND_ASSERT_MES(r, false, "init_test_wallet failed");
  r = generator.refresh_test_wallet(events, alice_wlt.get(), get_block_hash(blk_1r), 2 * CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
  CHECK_AND_ASSERT_MES(r, false, "refresh_test_wallet failed");
  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*alice_wlt.get(), "alice", alice_amount, 0, alice_amount, 0, 0), false, "");

  uint64_t miner_amount = MK_TEST_COINS(60);
  m_bob_amount = miner_amount + alice_amount - TX_DEFAULT_FEE;

  // Consolidated tx (TX_FLAG_SIGNATURE_MODE_SEPARATE).
  
  // this data will be transferred between stage 1 and 2
  transaction tx_1{};
  crypto::secret_key one_time_secret_key{};
  tx_generation_context gen_context{};

  // Part 1/2, miner's inputs
  {
    std::vector<tx_source_entry> sources;
    r = fill_tx_sources(sources, events, blk_1r, miner_acc.get_keys(), miner_amount, 10);
    CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
    uint64_t miner_change = get_sources_total_amount(sources) - miner_amount;

    std::vector<tx_destination_entry> destinations;
    if (miner_change != 0)
      destinations.push_back(tx_destination_entry(miner_change, miner_acc.get_public_address()));
    destinations.push_back(tx_destination_entry(m_bob_amount, bob_acc.get_public_address()));

    add_flags_to_all_destination_entries(tx_destination_entry_flags::tdef_explicit_native_asset_id, destinations);
    r = construct_tx(miner_acc.get_keys(), sources, destinations, empty_extra, empty_attachment, tx_1, get_tx_version_from_events(events), one_time_secret_key,
      0, 0, 0, true, TX_FLAG_SIGNATURE_MODE_SEPARATE, TX_DEFAULT_FEE, gen_context);
    CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

    dhc = count_type_in_variant_container<tx_derivation_hint>(tx_1.extra);
    CHECK_AND_ASSERT_MES(dhc == destinations.size(), false, "unexpected derivation hints count: " << dhc);

    // partially completed tx_1 shouldn't be accepted
     
    // now we added a balance check to tx_memory_pool::add_tx() for post-HF4 txs, so the behaviour is the same -- partially completed consolidated tx won't be added to the pool -- sowle
    // (subject to change in future)

    //if (m_post_hf4_zarcanum)
    //{
    //  ADD_CUSTOM_EVENT(events, tx_1);
    //  DO_CALLBACK(events, "mark_invalid_block");
    //  MAKE_NEXT_BLOCK_TX1(events, blk_2a, blk_1r, miner_acc, tx_1);
    //  DO_CALLBACK(events, "clear_tx_pool");
    //}
    //else
    //{
      DO_CALLBACK(events, "mark_invalid_tx");
      ADD_CUSTOM_EVENT(events, tx_1);
    //}
  }


  // Part 2/2, Alice's inputs
  {
    std::vector<tx_source_entry> sources;
    r = fill_tx_sources(sources, events, blk_1r, alice_acc.get_keys(), alice_amount, 10);
    CHECK_AND_ASSERT_MES(r, false, "fill_tx_sources failed");
    CHECK_AND_ASSERT_MES(get_sources_total_amount(sources) == alice_amount, false, "no change for Alice is expected");
    sources.back().separately_signed_tx_complete = true;

    std::vector<tx_destination_entry> destinations;

    r = construct_tx(alice_acc.get_keys(), sources, destinations, empty_extra, empty_attachment, tx_1, get_tx_version_from_events(events), one_time_secret_key,
      0, 0, 0, true, TX_FLAG_SIGNATURE_MODE_SEPARATE, 0 /* note zero fee here */, gen_context);
    CHECK_AND_ASSERT_MES(r, false, "construct_tx failed");

    size_t dhc_2 = count_type_in_variant_container<tx_derivation_hint>(tx_1.extra);
    CHECK_AND_ASSERT_MES(dhc_2 == dhc, false, "unexpected derivation hints count: " << dhc_2);

    ADD_CUSTOM_EVENT(events, tx_1);
  }
  MAKE_NEXT_BLOCK_TX1(events, blk_2, blk_1r, miner_acc, tx_1);

  DO_CALLBACK(events, "c1");

  return true;
}

bool hard_fork_4_consolidated_txs::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  std::shared_ptr<tools::wallet2> bob_wlt = init_playtime_test_wallet(events, c, BOB_ACC_IDX);
  bob_wlt->refresh();

  CHECK_AND_ASSERT_MES(check_balance_via_wallet(*bob_wlt.get(), "Bob", m_bob_amount, 0, 0, 0, 0), false, "");

  return true;
}



/*
hardfork_4_explicit_native_ids_in_outs::hardfork_4_explicit_native_ids_in_outs()
{
   REGISTER_CALLBACK_METHOD(hardfork_4_explicit_native_ids_in_outs, c1);

   m_hardforks.clear();
   m_hardforks.set_hardfork_height(ZANO_HARDFORK_04_ZARCANUM, CURRENCY_MINED_MONEY_UNLOCK_WINDOW + 1);
}

bool hardfork_4_explicit_native_ids_in_outs::generate(std::vector<test_event_entry>& events) const
{
  bool r = false;

  uint64_t ts = test_core_time::get_time();
  m_accounts.resize(TOTAL_ACCS_COUNT);
  account_base& miner_acc = m_accounts[MINER_ACC_IDX]; miner_acc.generate(); miner_acc.set_createtime(ts);
  account_base& alice_acc = m_accounts[ALICE_ACC_IDX]; alice_acc.generate(); alice_acc.set_createtime(ts);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_acc, ts);
  DO_CALLBACK(events, "configure_core"); // necessary to set m_hardforks

  REWIND_BLOCKS_N_WITH_TIME(events, blk_0r, blk_0, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  DO_CALLBACK_PARAMS(events, "check_hardfork_inactive", static_cast<size_t>(ZANO_HARDFORK_04_ZARCANUM));

  // tx_0: miner -> Alice
  // make tx_0 before HF4, so Alice will have only bare outs
  m_alice_initial_balance = MK_TEST_COINS(1000);
  MAKE_TX(events, tx_0, miner_acc, alice_acc, m_alice_initial_balance, blk_0r);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_acc, tx_0);

  // make sure HF4 has been activated
  DO_CALLBACK_PARAMS(events, "check_hardfork_active", static_cast<size_t>(ZANO_HARDFORK_04_ZARCANUM));

  // rewind blocks
  REWIND_BLOCKS_N_WITH_TIME(events, blk_1r, blk_1, miner_acc, CURRENCY_MINED_MONEY_UNLOCK_WINDOW);

  // check Alice's balance and make sure she cannot deploy an asset
  DO_CALLBACK(events, "c1_alice_cannot_deploy_asset");

  // tx_1: Alice -> Alice (all coins) : this will convert all Alice outputs to ZC outs
  MAKE_TX(events, tx_1, alice_acc, alice_acc, m_alice_initial_balance - TESTS_DEFAULT_FEE, blk_1r); 



  return true;
}

bool hardfork_4_explicit_native_ids_in_outs::c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  return true;
}
*/