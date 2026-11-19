// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include "chaingen.h"
#include "wallet_tests_basic.h"

// params of the check_gw_nonce callback: the nonce the blockchain expects in the next tx with inputs of gw_addr
struct gw_nonce_check_param
{
  currency::gateway_address_id_type gw_addr;
  uint64_t next_nonce = 0;

  BEGIN_SERIALIZE()
    FIELD(gw_addr)
    FIELD(next_nonce)
  END_SERIALIZE()
};

struct gw_nonce_test_base : public wallet_test
{
  gw_nonce_test_base();
  bool check_gw_nonce(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct gw_nonce_strict_order : public gw_nonce_test_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gw_nonce_per_tx_and_per_address : public gw_nonce_test_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gw_nonce_chain_switch : public gw_nonce_test_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
