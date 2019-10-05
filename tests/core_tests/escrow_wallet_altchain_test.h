// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"

// EAM = escrow altchain meta (test)
// It takes a sequence of high-level escrow events and transforms it into a sequence of generic coretest events, which in turn are played back by the test engine (chaingen_main).

enum { eam_contract_state_initial = 0, eam_contract_state_none = (uint32_t)(-1) }; // extention for tools::wallet_rpc::escrow_contract_details_basic::contract_state

enum { eam_tx_unknown = 0, eam_tx_make, eam_tx_confirm, eam_tx_make_and_confirm }; // make = construct tx and send it to the pool; confirm = put tx into a block

using contract_states = tools::wallet_public::escrow_contract_details_basic;

struct eam_event_noop
{
};

struct eam_event_proposal
{
  eam_event_proposal(uint32_t tx_behavior = eam_tx_make_and_confirm, uint64_t a_proposal_fee = TESTS_DEFAULT_FEE, uint64_t b_release_fee = TESTS_DEFAULT_FEE, uint64_t unlock_time = 0, uint64_t expiration_period = 60 * 60)
    : tx_behavior(tx_behavior)
    , a_proposal_fee(a_proposal_fee)
    , b_release_fee(b_release_fee)
    , unlock_time(unlock_time)
    , expiration_period(expiration_period)
  {}
  uint32_t tx_behavior;
  uint64_t a_proposal_fee;
  uint64_t b_release_fee;
  uint64_t unlock_time;
  uint64_t expiration_period;
};

struct eam_event_acceptance
{
  eam_event_acceptance(uint32_t tx_behavior = eam_tx_make_and_confirm, uint64_t b_acceptance_fee = TESTS_DEFAULT_FEE)
    : tx_behavior(tx_behavior)
    , b_acceptance_fee(b_acceptance_fee)
  {}
  uint32_t tx_behavior;
  uint64_t b_acceptance_fee;
};

struct eam_event_release
{
  eam_event_release(uint32_t tx_behavior = eam_tx_make_and_confirm, bool burn = false)
    : tx_behavior(tx_behavior)
    , burn(burn)
  {}
  uint32_t tx_behavior;
  bool burn;
};

struct eam_event_cancellation_proposal
{
  eam_event_cancellation_proposal(uint32_t tx_behavior = eam_tx_make_and_confirm, uint64_t a_cancellation_fee = TESTS_DEFAULT_FEE, uint64_t expiration_period = 60 * 60)
    : tx_behavior(tx_behavior)
    , a_cancellation_fee(a_cancellation_fee)
    , expiration_period(expiration_period)
  {}
  uint32_t tx_behavior;
  uint64_t a_cancellation_fee;
  uint64_t expiration_period;
};

struct eam_event_release_cancel
{
  eam_event_release_cancel(uint32_t tx_behavior = eam_tx_make_and_confirm)
    : tx_behavior(tx_behavior)
  {}
  uint32_t tx_behavior;
};

struct eam_event_refresh_and_check
{
  eam_event_refresh_and_check(uint32_t expected_blocks, uint32_t a_state = eam_contract_state_none, uint32_t b_state = eam_contract_state_none, uint64_t a_balance = UINT64_MAX, uint64_t b_balance = UINT64_MAX)
    : expected_blocks(expected_blocks)
    , a_state(a_state)
    , b_state(b_state)
    , a_balance(a_balance)
    , b_balance(b_balance)
  {}
  uint32_t expected_blocks; // how many block refresh() is expected to sync (both A and B)
  uint32_t a_state;         // eam_contract_state_none if shouldn't check
  uint32_t b_state;         // eam_contract_state_none if shouldn't check
  uint64_t a_balance;       // UINT64_MAX if shouldn't check
  uint64_t b_balance;       // UINT64_MAX if shouldn't check
};

struct eam_event_go_to
{
  eam_event_go_to(const std::string& label)
    : label(label)
  {}
  std::string label;
};

struct eam_event_check_top_block
{
  eam_event_check_top_block(uint64_t height)
    : height(height)
  {}
  eam_event_check_top_block(uint64_t height, const std::string& label)
    : height(height)
    , label(label)
  {}
  uint64_t height;
  std::string label;
};

struct eam_event_transfer
{
  eam_event_transfer(uint32_t party_from, uint32_t party_to, uint64_t amount, uint64_t fee = TESTS_DEFAULT_FEE)
    : party_from(party_from)
    , party_to(party_to)
    , amount(amount)
    , fee(fee)
  {}

  uint32_t party_from;
  uint32_t party_to;
  uint64_t amount;
  uint64_t fee;
};

typedef boost::variant<eam_event_noop, eam_event_proposal, eam_event_acceptance, eam_event_release, eam_event_cancellation_proposal, eam_event_release_cancel,
  eam_event_refresh_and_check, eam_event_go_to, eam_event_check_top_block, eam_event_transfer> eam_event_v;

// flag mask for eam_event_t
enum { eam_f_normal = 0, eam_f_error = 1 << 0, eam_f_no_block = 1 << 1 };
enum { start_amount_outs_default = 10 };

struct eam_event_t
{
  eam_event_t(uint64_t height, uint32_t flag_mask, eam_event_v v)
    : height(height)
    , v(v)
    , flag_mask(flag_mask)
  {}
  eam_event_t(uint64_t height, uint32_t flag_mask, eam_event_v v, const char* label)
    : height(height)
    , v(v)
    , flag_mask(flag_mask)
    , label(label)
  {}
  uint64_t height; // height at which smth should happen
  uint32_t flag_mask;
  eam_event_v v; // what should happen
  std::string label; // in case we should be able to reference it to later
};

struct eam_test_data_t
{
  eam_test_data_t(uint64_t alice_bob_start_amount, size_t start_amount_outs_count, bc_services::contract_private_details &cpd, const std::vector<eam_event_t>& events)
    : alice_bob_start_amount(alice_bob_start_amount)
    , start_amount_outs_count(start_amount_outs_count)
    , cpd(cpd)
    , events(events)
    , ms_id(currency::null_hash)
  {}

  uint64_t alice_bob_start_amount;
  size_t start_amount_outs_count;
  bc_services::contract_private_details cpd;
  std::vector<eam_event_t> events;
  crypto::hash ms_id; // multisig out id (contract id)
  currency::transaction proposal_tx; // to hold created proposal tx
  currency::transaction acceptance_tx; // to hold created acceptance tx
  currency::transaction release_tx; // to hold created release tx
  currency::transaction cancellation_proposal_tx; // to hold created cancellation_proposal tx
  currency::transaction release_cancel_tx;
};

//==============================================================================================================================
// individual tests speficiation
//==============================================================================================================================

template<int test_data_idx>
struct escrow_altchain_meta_test_data
{
  enum { empty_marker = true };
};

/*

IDEAS:
flag_normal, flag_error, flag_no_block


struct A
  {
    std::string comment;
    currency::account_public_address a_addr; // usually buyer
    currency::account_public_address b_addr; // usually seller
    uint64_t amount_to_pay;
    uint64_t amount_a_pledge;
    uint64_t amount_b_pledge;
  };

  constexpr static crypto::public_key my_null_pkey = { };
  constexpr static currency::account_public_address my_null_pub_addr = { my_null_pkey, my_null_pkey };
*/

// subtest 0 - straightforward contract flow, basic checks
template<>
struct escrow_altchain_meta_test_data<0>
{
  constexpr static uint64_t alice_bob_start_amount = TESTS_DEFAULT_FEE * 1000;
  bc_services::contract_private_details cpd = { "title", "comment",
    currency::null_pub_addr, currency::null_pub_addr, // a_addr and b_addr, will be set in generate()
    TESTS_DEFAULT_FEE * 10, // amount_to_pay
    TESTS_DEFAULT_FEE * 50, // amount_a_pledge
    TESTS_DEFAULT_FEE * 50, // amount_b_pledge
  };
  eam_test_data_t data = eam_test_data_t(alice_bob_start_amount, start_amount_outs_default, cpd, {
    eam_event_t(30, 0,           eam_event_proposal(eam_tx_make)),
    eam_event_t(31, 0,           eam_event_refresh_and_check(1)),
    eam_event_t(32, 0,           eam_event_acceptance(eam_tx_make)),
    eam_event_t(33, 0,           eam_event_refresh_and_check(2, contract_states::contract_accepted, contract_states::contract_accepted)),
    eam_event_t(34, 0,           eam_event_proposal(eam_tx_confirm)),
    eam_event_t(35, 0,           eam_event_refresh_and_check(2, contract_states::proposal_sent, contract_states::proposal_sent)),
    eam_event_t(36, eam_f_error, eam_event_release(eam_tx_make_and_confirm)), // should trigger an error, because contracts are in incorrect state
    eam_event_t(37, 0,           eam_event_acceptance(eam_tx_confirm)),
    eam_event_t(38, 0,           eam_event_refresh_and_check(3, contract_states::contract_accepted, contract_states::contract_accepted)),
    eam_event_t(39, 0,           eam_event_release(eam_tx_make_and_confirm)),
    eam_event_t(40, 0,           eam_event_refresh_and_check(2, contract_states::contract_released_normal, contract_states::contract_released_normal)),
  });
};

// subtest 1 - simple cancellation
template<>
struct escrow_altchain_meta_test_data<1>
{
  constexpr static uint64_t alice_bob_start_amount = TESTS_DEFAULT_FEE * 1000;
  bc_services::contract_private_details cpd = { "title", "comment",
    currency::null_pub_addr, currency::null_pub_addr, // a_addr and b_addr, will be set in generate()
    TESTS_DEFAULT_FEE * 10, // amount_to_pay
    TESTS_DEFAULT_FEE * 50, // amount_a_pledge
    TESTS_DEFAULT_FEE * 50, // amount_b_pledge
  };
  eam_test_data_t data = eam_test_data_t(alice_bob_start_amount, start_amount_outs_default, cpd, {
    eam_event_t(30, 0, eam_event_proposal(eam_tx_make_and_confirm)),
    eam_event_t(31, 0, eam_event_refresh_and_check(1)),
    eam_event_t(32, 0, eam_event_acceptance(eam_tx_make_and_confirm)),
    eam_event_t(33, 0, eam_event_refresh_and_check(2, contract_states::contract_accepted, contract_states::contract_accepted)),
    eam_event_t(34, 0, eam_event_cancellation_proposal(eam_tx_make_and_confirm)),
    eam_event_t(35, 0, eam_event_refresh_and_check(2, contract_states::contract_cancel_proposal_sent, contract_states::contract_cancel_proposal_sent)),
    eam_event_t(36, 0, eam_event_release_cancel(eam_tx_make_and_confirm)),
    eam_event_t(37, 0, eam_event_refresh_and_check(2, contract_states::contract_released_cancelled, contract_states::contract_released_cancelled)),
  });
};

// subtest 2 - simple chain switching
template<>
struct escrow_altchain_meta_test_data<2>
{
  constexpr static uint64_t alice_bob_start_amount = TESTS_DEFAULT_FEE * 1000;
  bc_services::contract_private_details cpd = { "title", "comment",
    currency::null_pub_addr, currency::null_pub_addr, // a_addr and b_addr, will be set in generate()
    TESTS_DEFAULT_FEE * 10, // amount_to_pay
    TESTS_DEFAULT_FEE * 50, // amount_a_pledge
    TESTS_DEFAULT_FEE * 50, // amount_b_pledge
  };
  // 30- 31- 32- 33-                   chain A
  // (p)  |           !                (p) - unconfirmed proposal, ! - block triggered chain switching
  //      \- 32- 33- 34- 35- 36- 37-   chain B
  //          p       a          rb    p - confirmed proposal, a - acceptance, rb - release burn
  eam_test_data_t data = eam_test_data_t(alice_bob_start_amount, start_amount_outs_default, cpd, {
    // chain A
    eam_event_t(30, 0, eam_event_proposal(eam_tx_make)),
    eam_event_t(31, 0, eam_event_noop(), "bifurcation_point"),
    eam_event_t(34, 0, eam_event_go_to("bifurcation_point")),  // make block 31 being prev for the next one
    // chain B
    eam_event_t(32, 0, eam_event_proposal(eam_tx_confirm)),
    eam_event_t(33, 0, eam_event_refresh_and_check(4, contract_states::proposal_sent, contract_states::proposal_sent)), // 4 blocks: 30..33 because wallets are still at main chain
    eam_event_t(34, 0, eam_event_acceptance(eam_tx_make_and_confirm)), // chain switch should happen here
    eam_event_t(35, 0, eam_event_check_top_block(34)),
    eam_event_t(36, 0, eam_event_refresh_and_check(4, contract_states::contract_accepted, contract_states::contract_accepted)), // 4 block: detach to 32 due to chain switch, then go 32..35
    eam_event_t(37, 0, eam_event_release(eam_tx_make_and_confirm, true)),
    eam_event_t(38, 0, eam_event_refresh_and_check(2, contract_states::contract_released_burned, contract_states::contract_released_burned)),
  });
};

// subtest 3 - chain switching after contract acceptance
template<>
struct escrow_altchain_meta_test_data<3>
{
  constexpr static uint64_t alice_bob_start_amount = TESTS_DEFAULT_FEE * 1000;
  bc_services::contract_private_details cpd = { "title", "comment",
    currency::null_pub_addr, currency::null_pub_addr, // a_addr and b_addr, will be set in generate()
    TESTS_DEFAULT_FEE * 10, // amount_to_pay
    TESTS_DEFAULT_FEE * 50, // amount_a_pledge
    TESTS_DEFAULT_FEE * 50, // amount_b_pledge
  };
  
  // 30- 31- 32- 33- 34- 35-           chain A
  //  p w    (a)  |   a w              p - confirmed proposal, (a) - unconfirmed acceptance, a - confirmed acceptance, w - wallet refresh
  //              |           !        ! - block triggered chain switching
  //              \- 34- 35- 36- 37-   chain B
  //                      a  rn w     a - confirmed acceptance, rn - release normal, w - wallet refresh
  eam_test_data_t data = eam_test_data_t(alice_bob_start_amount, start_amount_outs_default, cpd, {
    // chain A
    eam_event_t(30, 0, eam_event_proposal(eam_tx_make_and_confirm)),
    eam_event_t(31, 0, eam_event_refresh_and_check(1, contract_states::proposal_sent, contract_states::proposal_sent)),
    eam_event_t(32, 0, eam_event_acceptance(eam_tx_make)),
    eam_event_t(33, 0, eam_event_noop(), "33"),
    eam_event_t(34, 0, eam_event_acceptance(eam_tx_confirm)),
    eam_event_t(35, 0, eam_event_refresh_and_check(4, contract_states::contract_accepted, contract_states::contract_accepted)), // 4 blocks: 31..34
    eam_event_t(36, 0, eam_event_go_to("33")),  // make block 33 being prev for the next one
    // chain B
    eam_event_t(35, 0, eam_event_acceptance(eam_tx_confirm)),
    eam_event_t(36, 0, eam_event_release(eam_tx_make_and_confirm)),  // chain switch should happen here
    eam_event_t(37, 0, eam_event_refresh_and_check(3, contract_states::contract_released_normal, contract_states::contract_released_normal)), // detach 34..35, update 34..36
  });
};

// subtest 4 - chain switch after unconfirmed acceptance, confirmed acceptance goes to altchain and expires
template<>
struct escrow_altchain_meta_test_data<4>
{
  constexpr static uint64_t alice_bob_start_amount = TESTS_DEFAULT_FEE * 1000;
  bc_services::contract_private_details cpd = { "title", "comment",
    currency::null_pub_addr, currency::null_pub_addr, // a_addr and b_addr, will be set in generate()
    TESTS_DEFAULT_FEE * 10, // amount_to_pay
    TESTS_DEFAULT_FEE * 50, // amount_a_pledge
    TESTS_DEFAULT_FEE * 50, // amount_b_pledge
  };

  constexpr static uint64_t a_proposal_fee = TESTS_DEFAULT_FEE * 3;
  constexpr static uint64_t b_acceptance_fee  = TESTS_DEFAULT_FEE * 4;
  constexpr static uint64_t b_release_fee  = TESTS_DEFAULT_FEE * 2;
  
  //    proposal valid period 
  //  +-----------------------+
  // 30- 31- 32- 33- 34- 35-                              chain A
  //  p w   (a) w |   a w                                 p - confirmed proposal, (a) - unconfirmed acceptance, a - confirmed acceptance, w - wallet refresh
  //              |           !                           ! - block triggered chain switching
  //              \- 34- 35- 36- 37- 38- 39- 50- 51- 52-  chain B
  //                                        w    t  w     w - wallet refresh, t - money transfer
  eam_test_data_t data = eam_test_data_t(alice_bob_start_amount, 2 /* only two transfer for both */, cpd, {
    // chain A
    eam_event_t(30, 0, eam_event_proposal(eam_tx_make_and_confirm, a_proposal_fee, b_release_fee, 0, 0)), // expiration period is 0, tx will expire as soon as ts median moves enough
    eam_event_t(31, 0, eam_event_refresh_and_check(1, contract_states::proposal_sent, contract_states::proposal_sent)),
    eam_event_t(32, 0, eam_event_acceptance(eam_tx_make, b_acceptance_fee)),
    //eam_event_t(33, 0, eam_event_noop(), "33"),
    eam_event_t(33, 0, eam_event_refresh_and_check(2, contract_states::contract_accepted, contract_states::contract_accepted), "33"), // update 31..32
    eam_event_t(34, 0, eam_event_acceptance(eam_tx_confirm)), // should be okay, tx is not expired yet due to median lag
    eam_event_t(35, 0, eam_event_refresh_and_check(2, contract_states::contract_accepted, contract_states::contract_accepted, alice_bob_start_amount - a_proposal_fee - cpd.amount_a_pledge - cpd.amount_to_pay, alice_bob_start_amount - b_acceptance_fee - b_release_fee - cpd.amount_b_pledge)), // update 33..34
    eam_event_t(36, 0, eam_event_go_to("33")),  // make block 33 being prev for the next one
    // chain B
    eam_event_t(50, 0, eam_event_refresh_and_check(16, eam_contract_state_none, eam_contract_state_none, alice_bob_start_amount - a_proposal_fee, alice_bob_start_amount)), // detach 34..35, update 34..49
    
    // the following two line commened by sowle 2019-04-26
    // reason: tx linked with alt blocks are not removed from the pool on expiration or when outdated
    // so atm this test should wait until related alt block will be removed from the blockchain -> this will let tx pool to remove expired tx -> coins will be unlocked
    // should be tested differently somehow
    // TODO: need more attention here
    //
    //eam_event_t(51, 0, eam_event_transfer(wallet_test::ALICE_ACC_IDX, wallet_test::BOB_ACC_IDX, alice_bob_start_amount - a_proposal_fee - TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE)), // make sure money are completely unlocked
    //eam_event_t(52, 0, eam_event_refresh_and_check(2, eam_contract_state_none, eam_contract_state_none, 0, 2 * alice_bob_start_amount - a_proposal_fee - TESTS_DEFAULT_FEE)), // update 51..52
  });
};

// subtest 5 - chain switch after p,(a) (confirmed proposal and unconfirmed acceptance)
template<>
struct escrow_altchain_meta_test_data<5>
{
  constexpr static uint64_t alice_bob_start_amount = TESTS_DEFAULT_FEE * 1000;
  bc_services::contract_private_details cpd = { "title", "comment",
    currency::null_pub_addr, currency::null_pub_addr, // a_addr and b_addr, will be set in generate()
    TESTS_DEFAULT_FEE * 10, // amount_to_pay
    TESTS_DEFAULT_FEE * 50, // amount_a_pledge
    TESTS_DEFAULT_FEE * 50, // amount_b_pledge
  };
  
  //    proposal valid period 
  //  +-----------------------+
  // 30- 31- 32- 33- 34- 35-                       chain A
  //  p w    (a)  |   a w                          p - confirmed proposal, (a) - unconfirmed acceptance, a - confirmed acceptance, w - wallet refresh
  //              |           !                    ! - block triggered chain switching
  //              \- 34- 35- 36- 37- 38- 39- 40-   chain B
  //                      a          rn w          a - confirmed acceptance, rn - release normal
  eam_test_data_t data = eam_test_data_t(alice_bob_start_amount, start_amount_outs_default, cpd, {
    // chain A
    eam_event_t(30, 0, eam_event_proposal(eam_tx_make_and_confirm, TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, 0, DIFFICULTY_POW_TARGET * 2)), // expiration period is 2 pow targets
    eam_event_t(31, 0, eam_event_refresh_and_check(1, contract_states::proposal_sent, contract_states::proposal_sent)),
    eam_event_t(32, 0, eam_event_acceptance(eam_tx_make)),
    eam_event_t(33, 0, eam_event_noop(), "33"),
    eam_event_t(34, 0, eam_event_acceptance(eam_tx_confirm)), // should be okay, tx is not expired yet due to median lag
    eam_event_t(35, 0, eam_event_refresh_and_check(4, contract_states::contract_accepted, contract_states::contract_accepted)), // 4 blocks: 31..34
    eam_event_t(36, 0, eam_event_go_to("33")),  // make block 33 being prev for the next one
    // chain B
    eam_event_t(35, 0, eam_event_acceptance(eam_tx_confirm)),
    eam_event_t(38, 0, eam_event_release(eam_tx_make_and_confirm)),  // chain switch should happen here
    eam_event_t(39, 0, eam_event_refresh_and_check(5, contract_states::contract_released_normal, contract_states::contract_released_normal)), // detach 34..35, update 34..38
  });
};

// subtest 6 - chain switch before p (confirmed proposal)
template<>
struct escrow_altchain_meta_test_data<6>
{
  constexpr static uint64_t alice_bob_start_amount = TESTS_DEFAULT_FEE * 1000;
  bc_services::contract_private_details cpd = { "title", "comment",
    currency::null_pub_addr, currency::null_pub_addr, // a_addr and b_addr, will be set in generate()
    TESTS_DEFAULT_FEE * 10, // amount_to_pay
    TESTS_DEFAULT_FEE * 50, // amount_a_pledge
    TESTS_DEFAULT_FEE * 50, // amount_b_pledge
  };
  // 30- 31- 32- 33- 34- 35-             chain A
  //  |  p  w    a  w   (rn)             p - confirmed proposal, a - confirmed acceptance, w - wallet refresh, (rn) - unconfirmed release normal
  //  |                       !          ! - triggers chain switching
  //   \ 31- 32- 33- 34- 35- 36- 37-     chain B
  //         p   a   rn         w
  eam_test_data_t data = eam_test_data_t(alice_bob_start_amount, start_amount_outs_default, cpd, {
    // chain A
    eam_event_t(30, 0, eam_event_noop(), "30"),
    eam_event_t(31, 0, eam_event_proposal(eam_tx_make_and_confirm)),
    eam_event_t(32, 0, eam_event_refresh_and_check(2, contract_states::proposal_sent, contract_states::proposal_sent)),
    eam_event_t(33, 0, eam_event_acceptance(eam_tx_make_and_confirm)),
    eam_event_t(34, 0, eam_event_refresh_and_check(2, contract_states::contract_accepted, contract_states::contract_accepted)),
    eam_event_t(35, 0, eam_event_release(eam_tx_make)),
    eam_event_t(36, 0, eam_event_go_to("30")),
    // chain B
    eam_event_t(32, 0, eam_event_proposal(eam_tx_confirm)),
    eam_event_t(33, 0, eam_event_acceptance(eam_tx_confirm)),
    eam_event_t(34, 0, eam_event_release(eam_tx_confirm)),
    eam_event_t(37, 0, eam_event_refresh_and_check(6, contract_states::contract_released_normal, contract_states::contract_released_normal)), // detach 31..35, update 31..36
  });
};

// subtest 7 - cancellation proposal expiration in main chain (no switching)
template<>
struct escrow_altchain_meta_test_data<7>
{
  constexpr static uint64_t alice_bob_start_amount = TESTS_DEFAULT_FEE * 1000;
  bc_services::contract_private_details cpd = { "title", "comment",
    currency::null_pub_addr, currency::null_pub_addr, // a_addr and b_addr, will be set in generate()
    TESTS_DEFAULT_FEE * 10, // amount_to_pay
    TESTS_DEFAULT_FEE * 50, // amount_a_pledge
    TESTS_DEFAULT_FEE * 50, // amount_b_pledge
  };
  //                    cp expiration
  //                      +-------+
  // 30- 31- 32- 33- 34- 35- 36- ... 45-    chain A
  //     p  w    a  w    cp w       w       p - confirmed proposal, a - confirmed acceptance, w - wallet refresh, cp - cancellation proposal
  //
  eam_test_data_t data = eam_test_data_t(alice_bob_start_amount, start_amount_outs_default, cpd, {
    // chain A
    eam_event_t(30, 0, eam_event_noop()),
    eam_event_t(31, 0, eam_event_proposal(eam_tx_make_and_confirm)),
    eam_event_t(32, 0, eam_event_refresh_and_check(2, contract_states::proposal_sent, contract_states::proposal_sent)),
    eam_event_t(33, 0, eam_event_acceptance(eam_tx_make_and_confirm)),
    eam_event_t(34, 0, eam_event_refresh_and_check(2, contract_states::contract_accepted, contract_states::contract_accepted)),
    eam_event_t(35, 0, eam_event_cancellation_proposal(eam_tx_make_and_confirm, TESTS_DEFAULT_FEE, 0)), // zero expiration period
    eam_event_t(36, 0, eam_event_refresh_and_check(2, contract_states::contract_cancel_proposal_sent, contract_states::contract_cancel_proposal_sent)),
    eam_event_t(45, 0, eam_event_refresh_and_check(9, contract_states::contract_accepted, contract_states::contract_accepted)),
  });
};

// subtest 8 - chain switch after unconfirmed cancellation proposal
template<>
struct escrow_altchain_meta_test_data<8>
{
  constexpr static uint64_t alice_bob_start_amount = TESTS_DEFAULT_FEE * 1000;
  bc_services::contract_private_details cpd = { "title", "comment",
    currency::null_pub_addr, currency::null_pub_addr, // a_addr and b_addr, will be set in generate()
    TESTS_DEFAULT_FEE * 10, // amount_to_pay
    TESTS_DEFAULT_FEE * 50, // amount_a_pledge
    TESTS_DEFAULT_FEE * 50, // amount_b_pledge
  };

  constexpr static uint64_t a_proposal_fee = TESTS_DEFAULT_FEE * 3;
  constexpr static uint64_t b_acceptance_fee = TESTS_DEFAULT_FEE * 4;
  constexpr static uint64_t b_release_fee = TESTS_DEFAULT_FEE * 2;
  constexpr static uint64_t a_cancellation_fee = TESTS_DEFAULT_FEE * 5;

  //                          cp expiration
  //                      +--------------------+
  // 30- 31- 32- 33- 34- 35- 36- 37- 38- 39- 40- 41-          chain A
  //     p  w    a  w   (cp) w   |   cp    w rc     w         p - confirmed proposal, a - confirmed acceptance, w - wallet refresh
  //                             \                            (cp) - unconfirmed cancellation proposal, cp - confirmed cancellation proposal, rc - release cancel
  //                              \- 38- 39- 40- 41- ... 45-  chain B
  //                                 cp    w rc     w    !    ! - chain switch
  eam_test_data_t data = eam_test_data_t(alice_bob_start_amount, start_amount_outs_default, cpd, {
    // chain A
    eam_event_t(30, 0, eam_event_noop()),
    eam_event_t(31, 0, eam_event_proposal(eam_tx_make_and_confirm, a_proposal_fee, b_release_fee)),
    eam_event_t(32, 0, eam_event_refresh_and_check(2, contract_states::proposal_sent, contract_states::proposal_sent)),
    eam_event_t(33, 0, eam_event_acceptance(eam_tx_make_and_confirm, b_acceptance_fee)),
    eam_event_t(34, 0, eam_event_refresh_and_check(2, contract_states::contract_accepted, contract_states::contract_accepted)),
    eam_event_t(35, 0, eam_event_cancellation_proposal(eam_tx_make, a_cancellation_fee, 120)), // 61 is the minimum, 60 is not enough
    eam_event_t(36, 0, eam_event_refresh_and_check(2, contract_states::contract_cancel_proposal_sent, contract_states::contract_cancel_proposal_sent)),
    eam_event_t(37, 0, eam_event_noop(), "37"),
    eam_event_t(38, 0, eam_event_cancellation_proposal(eam_tx_confirm)),
    eam_event_t(39, 0, eam_event_refresh_and_check(3, contract_states::contract_cancel_proposal_sent, contract_states::contract_cancel_proposal_sent)),
    eam_event_t(40, 0, eam_event_release_cancel(eam_tx_make_and_confirm)),
    eam_event_t(41, 0, eam_event_refresh_and_check(2, contract_states::contract_released_cancelled, contract_states::contract_released_cancelled,
      alice_bob_start_amount - a_proposal_fee - a_cancellation_fee,
      alice_bob_start_amount - b_acceptance_fee - b_release_fee)),
    eam_event_t(42, 0, eam_event_go_to("37")),

    // chain B
    eam_event_t(38, 0, eam_event_cancellation_proposal(eam_tx_confirm)),
    //eam_event_t(39, 0, eam_event_refresh_and_check(1, contract_states::contract_cancel_proposal_sent, contract_states::contract_cancel_proposal_sent)),
    //eam_event_t(40, 0, eam_event_release_cancel(eam_tx_make_and_confirm)),
    //eam_event_t(45, 0, eam_event_refresh_and_check(2, contract_states::contract_released_cancelled, contract_states::contract_released_cancelled,
    //  alice_bob_start_amount - a_proposal_fee - a_cancellation_fee,
    //  alice_bob_start_amount - b_acceptance_fee - b_release_fee)),
  });
};

//==============================================================================================================================

struct escrow_altchain_meta_impl : public wallet_test
{
  escrow_altchain_meta_impl(const eam_test_data_t &etd);
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool handle_event_proposal(currency::core& c, std::shared_ptr<tools::wallet2> alice_wlt, const eam_event_proposal& se);
  bool handle_event_acceptance(currency::core& c, std::shared_ptr<tools::wallet2> bob_wlt, const eam_event_acceptance& se);
  bool handle_event_release(currency::core& c, std::shared_ptr<tools::wallet2> alice_wlt, const eam_event_release& se);
  bool handle_event_cancellation_proposal(currency::core& c, std::shared_ptr<tools::wallet2> alice_wlt, const eam_event_cancellation_proposal& se);
  bool handle_event_release_cancel(currency::core& c, std::shared_ptr<tools::wallet2> alice_wlt, const eam_event_release_cancel& se);

private:
  bool mine_next_block_with_tx(currency::core& c, const currency::transaction& tx);
  bool mine_next_block_with_no_tx(currency::core& c);

  mutable eam_test_data_t m_etd;
  crypto::hash m_last_block_hash;
  uint64_t m_last_block_height;
};

template<int test_data_idx>
struct escrow_altchain_meta_test : virtual public test_chain_unit_base
{
  typedef escrow_altchain_meta_test<test_data_idx> this_t;

  escrow_altchain_meta_test()
    : m_impl(escrow_altchain_meta_test_data<test_data_idx>().data)
  {
    REGISTER_CALLBACK_METHOD(this_t, c1);
  }

  bool generate(std::vector<test_event_entry>& events) const
  {
    return m_impl.generate(events);
  }

  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
  {
    return m_impl.c1(c, ev_index, events);
  }

  // test_chain_unit_base
  bool need_core_proxy() const { return m_impl.need_core_proxy(); }
  void set_core_proxy(std::shared_ptr<tools::i_core_proxy> p) { m_impl.set_core_proxy(p); }

  escrow_altchain_meta_impl m_impl;
};
