// Copyright (c) 2023-2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"


struct hard_fork_4_consolidated_txs : public wallet_test
{
  hard_fork_4_consolidated_txs();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable bool m_post_hf4_zarcanum = false;
  mutable uint64_t m_bob_amount = 0;
};


struct hardfork_4_explicit_native_ids_in_outs : public wallet_test
{
  hardfork_4_explicit_native_ids_in_outs();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable uint64_t m_alice_initial_balance = 0;
};


struct hardfork_4_wallet_transfer_with_mandatory_mixins : public wallet_test
{
  hardfork_4_wallet_transfer_with_mandatory_mixins();
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};


struct hardfork_4_wallet_sweep_bare_outs : public wallet_test
{
  hardfork_4_wallet_sweep_bare_outs();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct hardfork_4_pop_tx_from_global_index : public wallet_test
{
  hardfork_4_pop_tx_from_global_index();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};


struct hardfork_4_pos_decoy_transition : public wallet_test
{
  enum
  {
    HF4_TRANSITION_ACTIVE_AFTER = 20,
    HF4_TRANSITION_FIRST_HEIGHT = HF4_TRANSITION_ACTIVE_AFTER + 1,
    HF4_TRANSITION_LAST_INSUFFICIENT_TOP = 36,
    HF4_TRANSITION_FIRST_SUFFICIENT_TOP = 37
  };

  struct core_proxy : public tools::i_core_proxy
  {
    explicit core_proxy(std::shared_ptr<tools::i_core_proxy> delegate);
    bool call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request& req, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response& rsp) override;

    std::shared_ptr<tools::i_core_proxy> m_delegate;
    bool m_called = false;
    currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request m_request{};
    currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response m_response{};
  };

  static size_t count_response_outputs(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response& response);

  hardfork_4_pos_decoy_transition();
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};
