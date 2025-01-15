// Copyright (c) 2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include <vector>
#include "chaingen.h"

struct fill_tx_rpc_inputs : public test_chain_unit_enchanced
{
  fill_tx_rpc_inputs();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(const currency::core& core, const size_t event_position, const std::vector<test_event_entry>& events) const;
  bool c2(const currency::core& core, const size_t event_position, const std::vector<test_event_entry>& events) const;
  bool c3(const currency::core& core, const size_t event_position, const std::vector<test_event_entry>& events) const;
  bool c4(const currency::core& core, const size_t event_position, const std::vector<test_event_entry>& events) const;
  bool c5(const currency::core& core, const size_t event_position, const std::vector<test_event_entry>& events) const;
  bool c6(const currency::core& core, const size_t event_position, const std::vector<test_event_entry>& events) const;
  bool c7(const currency::core& core, const size_t event_position, const std::vector<test_event_entry>& events) const;
  bool c8(const currency::core& core, const size_t event_position, const std::vector<test_event_entry>& events) const;
};
