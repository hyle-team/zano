// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include "chaingen.h"
#include "wallet_tests_basic.h"

struct hf_booster_hf8_activation_basic : public wallet_test
{
  hf_booster_hf8_activation_basic();
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  currency::keypair m_maintainer_keys;
};
