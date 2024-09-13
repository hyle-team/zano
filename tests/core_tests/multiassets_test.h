// Copyright (c) 2014-2023 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"

struct multiassets_basic_test : public wallet_test
{
  static uint64_t ts_starter;
  multiassets_basic_test();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct assets_and_explicit_native_coins_in_outs : public wallet_test
{
  assets_and_explicit_native_coins_in_outs();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1_alice_cannot_deploy_asset(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c2_alice_deploys_asset(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable uint64_t m_alice_initial_balance = 0;
};

struct asset_depoyment_and_few_zc_utxos : public wallet_test
{
  asset_depoyment_and_few_zc_utxos();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable uint64_t m_alice_initial_balance = 0;
};

struct assets_and_pos_mining : public wallet_test
{
  assets_and_pos_mining();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct asset_emission_and_unconfirmed_balance : public wallet_test
{
  asset_emission_and_unconfirmed_balance();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct asset_operation_and_hardfork_checks : public wallet_test
{
  public:
    asset_operation_and_hardfork_checks();

    bool generate(std::vector<test_event_entry>& events) const;
    bool c1(currency::core& c,
            size_t ev_index,
            const std::vector<test_event_entry>& events);

    bool c2(currency::core& c,
            size_t ev_index,
            const std::vector<test_event_entry>& events);

  private:
    mutable currency::asset_descriptor_base m_adb_hello{};
    mutable currency::asset_descriptor_operation m_ado_hello{};
    mutable currency::asset_descriptor_base m_adb_bye{};
    mutable currency::asset_descriptor_operation m_ado_bye{};
};

struct asset_operation_in_consolidated_tx : public wallet_test
{
public:
  asset_operation_in_consolidated_tx();
  bool generate(std::vector<test_event_entry>& events) const;
  bool assert_balances(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool assert_alice_currency_not_registered(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  mutable currency::asset_descriptor_base m_adb_alice_currency{};
  mutable currency::asset_descriptor_operation m_ado_alice_currency{};
};

struct asset_current_supply_greater_than_total_supply : public wallet_test
{
public:
  asset_current_supply_greater_than_total_supply();
  bool generate(std::vector<test_event_entry>& events) const;
  bool assert_asset_alpha_not_registered(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events) const;
  bool assert_asset_beta_registered(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events) const;
  bool emit_asset_beta_with_incorrect_supply(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events) const;
  bool assert_asset_beta_not_emitted(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events) const;
  bool public_burn_asset_beta_with_incorrect_supply(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events) const;

private:
  enum asset_position { alpha = 0, beta = 1 };
  mutable std::array<currency::asset_descriptor_base, 2> m_adbs{};
  mutable std::array<currency::asset_descriptor_operation, 2> m_ados_register{};
  mutable currency::asset_descriptor_operation m_ado_emit{};
};
