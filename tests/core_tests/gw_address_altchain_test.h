// Copyright (c) 2025-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include "chaingen.h"
#include "wallet_tests_basic.h"

struct gw_addr_altchain_spend_in_both_chains : public wallet_test
{
  gw_addr_altchain_spend_in_both_chains();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

protected:
  mutable currency::keypair m_gw_addr_view{};
  mutable currency::keypair m_gw_addr_spend{};
};

struct gw_addr_altchain_created_in_fork : public wallet_test
{
  gw_addr_altchain_created_in_fork();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

protected:
  mutable currency::keypair m_gw_addr_view{};
  mutable currency::keypair m_gw_addr_spend{};
};

struct gw_addr_altchain_no_cross_chain_usage : public wallet_test
{
  gw_addr_altchain_no_cross_chain_usage();
  bool generate(std::vector<test_event_entry>& events) const;

protected:
  mutable currency::keypair m_gw_addr_view{};
  mutable currency::keypair m_gw_addr_spend{};
};

struct gw_addr_altchain_owner_change : public wallet_test
{
  gw_addr_altchain_owner_change();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

protected:
  mutable currency::keypair m_gw_addr_view{};
  mutable currency::keypair m_gw_addr_spend{};
  mutable currency::keypair m_gw_addr_new_spend{}; // new owner for chain C
};
