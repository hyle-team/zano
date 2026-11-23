// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "boost/serialization/array.hpp"
#include "misc_language.h"
#include "string_tools.h"
#include "currency_core/currency_basic.h"

namespace currency
{
  struct hard_forks_descriptor
  {
    constexpr static size_t m_total_count = ZANO_HARDFORKS_TOTAL;
    std::array<uint64_t, m_total_count> m_height_the_hardfork_n_active_after;
    uint64_t last_supported_hardfork_id = 0;

    hard_forks_descriptor();
    void clear();
    void set_hardfork_height_if_exist(size_t hardfork_id, uint64_t height_the_hardfork_is_active_after);
    void set_hardfork_height(size_t hardfork_id, uint64_t height_the_hardfork_is_active_after);

    bool is_hardfork_active_for_height(size_t hardfork_id, uint64_t height) const;
    std::string get_str_height_the_hardfork_active_after(size_t hardfork_id) const;
    uint64_t get_height_the_hardfork_active_after(size_t hardfork_id) const;
    size_t get_the_most_recent_hardfork_id_for_height(uint64_t height) const;
    uint8_t get_block_major_version_by_height(uint64_t height) const;
    uint8_t get_block_minor_version_by_height(uint64_t height) const;
    bool operator==(const hard_forks_descriptor& rhs) const;
    bool operator!=(const hard_forks_descriptor& rhs) const;
  };


  typedef uint64_t (*core_time_func_t)();

  struct core_runtime_config
  {
    uint64_t min_coinstake_age;
    uint64_t pos_minimum_heigh; //height
    uint64_t tx_pool_min_fee;
    uint64_t tx_default_fee;
    uint64_t max_alt_blocks;
    crypto::public_key alias_validation_pubkey;
    core_time_func_t get_core_time;
    uint64_t hf4_minimum_mixins;
    wide_difficulty_type max_pos_difficulty;
    uint64_t pre_hardfork_tx_freeze_period; // number of blocks before the hardfork activatio when no new txs are accepted (effective from HF5); 0 means feature is switched off
    uint64_t minimal_distance_between_boost_and_hf = 0;
    uint64_t gw_nonce_max_gap = 0;  // maximum gap between the last used nonce and the next nonce for a given gateway address, if the gap is bigger than this value, the nonce is considered invalid and tx with such nonce will be rejected
    uint64_t gw_nonce_fee_window = 0;  // the fee window for the gateway nonce replacement, if a new tx with the same nonce is submitted, it will be accepted only if its fee is bigger than the previous one by this value

    hard_forks_descriptor hard_forks;
    std::array<int, ZANO_HARDFORKS_TOTAL> min_build_numbers_for_hard_forks;
    crypto::public_key hf_maintainers_public_key = {};

    bool is_hardfork_active_for_height(size_t hardfork_id, uint64_t upcoming_block_height) const
    {
      return hard_forks.is_hardfork_active_for_height(hardfork_id, upcoming_block_height);
    }

    int get_min_allowed_build_version_for_height(uint64_t upcoming_block_height) const
    {
      return min_build_numbers_for_hard_forks[hard_forks.get_the_most_recent_hardfork_id_for_height(upcoming_block_height)];
    }

    static uint64_t _default_core_time_function()
    {
      return time(NULL);
    }
  };

  core_runtime_config get_default_core_runtime_config();

}
