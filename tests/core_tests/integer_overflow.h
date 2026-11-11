// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"

struct gen_uint_overflow_base : public test_chain_unit_base
{
  gen_uint_overflow_base();

  bool check_tx_verification_context(const currency::tx_verification_context& tvc, bool tx_added, size_t event_idx, const currency::transaction& tx);
  bool check_block_verification_context(const currency::block_verification_context& bvc, size_t event_idx, const currency::block& block);

  bool mark_last_valid_block(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  size_t m_last_valid_block_event_idx;
};

struct gen_uint_overflow_1 : public gen_uint_overflow_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_uint_overflow_2 : public gen_uint_overflow_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
