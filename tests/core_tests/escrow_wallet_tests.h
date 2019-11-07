// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"


struct escrow_wallet_test : public wallet_test
{
  escrow_wallet_test();
  bool generate(std::vector<test_event_entry>& events) const;
  bool prepare_proposal_accepted_test(currency::core& c, const std::vector<test_event_entry>& events, std::shared_ptr<tools::wallet2>& wallet_buyer, std::shared_ptr<tools::wallet2>& wallet_seller, crypto::hash& multisig_id);
  bool exec_test_with_specific_release_type(currency::core& c, const std::vector<test_event_entry>& events, bool release_normal);
  bool exec_test_with_cancel_release_type(currency::core& c, const std::vector<test_event_entry>& events);
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
private: 
  mutable currency::account_base m_mining_accunt;

};


struct escrow_w_and_fake_outputs : public wallet_test
{
  escrow_w_and_fake_outputs();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  uint64_t m_pledge_amount;
  mutable uint64_t m_alice_bob_start_chunk_amount;
};


struct escrow_incorrect_proposal : public wallet_test
{
  escrow_incorrect_proposal();
  bool generate(std::vector<test_event_entry>& events) const;

  bool check_normal_proposal(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_incorrect_proposal(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  typedef std::unordered_map<crypto::hash, std::pair<uint64_t, std::string>> incorrect_proposals_t; // contract id -> (custom mask, mask name)
  mutable incorrect_proposals_t m_incorrect_proposals;
};

struct escrow_proposal_expiration : public wallet_test
{
  escrow_proposal_expiration();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct escrow_proposal_and_accept_expiration : public wallet_test
{
  escrow_proposal_and_accept_expiration();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct escrow_incorrect_proposal_acceptance : public wallet_test
{
  escrow_incorrect_proposal_acceptance();
  bool generate(std::vector<test_event_entry>& events) const;

  bool check_normal_acceptance(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_incorrect_acceptance(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  mutable bc_services::contract_private_details m_cpd;
  mutable uint64_t m_alice_bob_start_amount;
  mutable uint64_t m_bob_fee_release;
};

struct escrow_custom_test_callback_details;
struct escrow_custom_test : public wallet_test
{
  escrow_custom_test();
  bool generate(std::vector<test_event_entry>& events) const;
  bool check(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool do_custom_test(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events, const escrow_custom_test_callback_details& cd);
};

struct escrow_incorrect_cancel_proposal : public wallet_test
{
  escrow_incorrect_cancel_proposal();
  bool generate(std::vector<test_event_entry>& events) const;

  bool check_normal_cancel_proposal(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_incorrect_cancel_proposal(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_incorrect_cancel_proposal_internal(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  mutable bc_services::contract_private_details m_cpd;
  mutable uint64_t m_alice_bob_start_amount;
  mutable uint64_t m_bob_fee_release;
};

struct escrow_proposal_not_enough_money : public wallet_test
{
  escrow_proposal_not_enough_money();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct escrow_cancellation_and_tx_order : public wallet_test
{
  escrow_cancellation_and_tx_order();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct escrow_cancellation_proposal_expiration : public wallet_test
{
  escrow_cancellation_proposal_expiration();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct escrow_cancellation_acceptance_expiration : public wallet_test
{
  escrow_cancellation_acceptance_expiration();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct escrow_proposal_acceptance_in_alt_chain : public wallet_test
{
  escrow_proposal_acceptance_in_alt_chain();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct escrow_zero_amounts : public wallet_test
{
  escrow_zero_amounts();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct escrow_balance : public wallet_test
{
  escrow_balance();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable uint64_t m_alice_bob_start_amount;
  mutable uint64_t m_alice_bob_start_chunk_amount;
};
