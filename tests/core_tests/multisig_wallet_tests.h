// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"
#include "random_helper.h"

struct multisig_wallet_test : public wallet_test
{
  multisig_wallet_test();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private: 
  mutable currency::account_base m_mining_accunt;
  mutable currency::account_base m_accunt_a;
  mutable currency::account_base m_accunt_b;
  mutable currency::account_base m_accunt_c;
};

struct multisig_wallet_test_many_dst : public wallet_test
{
  multisig_wallet_test_many_dst();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct multisig_wallet_heterogenous_dst : public wallet_test
{
  multisig_wallet_heterogenous_dst();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct multisig_wallet_same_dst_addr : public wallet_test
{
  multisig_wallet_same_dst_addr();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct multisig_wallet_ms_to_ms : public wallet_test
{
  multisig_wallet_ms_to_ms();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct multisig_minimum_sigs : public wallet_test
{
  multisig_minimum_sigs();
  bool generate(std::vector<test_event_entry>& events) const;
};

struct multisig_and_fake_outputs : public wallet_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct multisig_and_unlock_time : public wallet_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct multisig_and_coinbase : public wallet_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct multisig_with_same_id_in_pool : public wallet_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct multisig_and_checkpoints : public wallet_test
{
  multisig_and_checkpoints();
  bool set_cp(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool generate(std::vector<test_event_entry>& events) const;

private:
  random_state_test_restorer m_random_state_test_restorer;
};

struct multisig_and_checkpoints_bad_txs : public wallet_test
{
  multisig_and_checkpoints_bad_txs();
  bool set_cp(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool generate(std::vector<test_event_entry>& events) const;
};

struct multisig_and_altchains : public wallet_test
{
  multisig_and_altchains();
  bool generate(std::vector<test_event_entry>& events) const;
  bool check_tx_pool(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  struct params_tx_pool
  {
    params_tx_pool() : counter(SIZE_MAX - 1), tx_hash(currency::null_hash)
    {}

    params_tx_pool(size_t number_of_tx_in_the_pool)
      : counter(number_of_tx_in_the_pool)
    {}

    params_tx_pool(const crypto::hash& tx_should_exists_in_the_pool)
      : counter(SIZE_MAX), tx_hash(tx_should_exists_in_the_pool)
    {}

    params_tx_pool(const currency::transaction& tx_should_exists_in_the_pool)
      : counter(SIZE_MAX), tx_hash(currency::get_transaction_hash(tx_should_exists_in_the_pool))
    {}

    size_t counter;
    crypto::hash tx_hash;
  };

};

struct ref_by_id_basics : public wallet_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct ref_by_id_mixed_inputs_types : public wallet_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct multisig_n_participants_seq_signing : public wallet_test
{
  multisig_n_participants_seq_signing();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  size_t m_participants_count;
  size_t m_minimum_signs_to_spend;
  std::vector<currency::account_base> m_participants;
};

struct multisig_out_make_and_spent_in_altchain : public wallet_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct multisig_out_spent_in_altchain_case_b4 : public wallet_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct multisig_unconfirmed_transfer_and_multiple_scan_pool_calls : public wallet_test
{
  multisig_unconfirmed_transfer_and_multiple_scan_pool_calls();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};
