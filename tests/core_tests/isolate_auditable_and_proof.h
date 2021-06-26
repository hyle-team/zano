// Copyright (c) 2014-2021 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"


struct isolate_auditable_and_proof : public wallet_test
{
  isolate_auditable_and_proof();
  bool generate(std::vector<test_event_entry>& events) const;
  virtual bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  virtual bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
protected:
  mutable uint64_t m_genesis_timestamp;
  mutable currency::account_base m_mining_accunt;
};
