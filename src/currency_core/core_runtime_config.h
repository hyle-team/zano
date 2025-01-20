// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "boost/serialization/array.hpp"
#include "misc_language.h"
#include "string_tools.h"

namespace currency
{
  struct hard_forks_descriptor
  {
    constexpr static size_t m_total_count = ZANO_HARDFORKS_TOTAL;
    std::array<uint64_t, m_total_count> m_height_the_hardfork_n_active_after;

    hard_forks_descriptor()
    {
      clear();
    }

    void clear()
    {
      m_height_the_hardfork_n_active_after.fill(CURRENCY_MAX_BLOCK_NUMBER);
    }

    void set_hardfork_height(size_t hardfork_id, uint64_t height_the_hardfork_is_active_after)
    {
      CHECK_AND_ASSERT_THROW_MES(hardfork_id < m_total_count, "invalid hardfork id: " << hardfork_id);
      m_height_the_hardfork_n_active_after[hardfork_id] = height_the_hardfork_is_active_after;

      // set all unset previous hardforks to this height
      for(size_t hid = hardfork_id - 1; hid + 1 != 0; --hid)
      {
        if (m_height_the_hardfork_n_active_after[hid] == CURRENCY_MAX_BLOCK_NUMBER)
          m_height_the_hardfork_n_active_after[hid] = height_the_hardfork_is_active_after;
      }
    }

    bool is_hardfork_active_for_height(size_t hardfork_id, uint64_t height) const
    {
      if (hardfork_id == 0)
        return true; // hardfork #0 is a special case that is considered always active 
      CHECK_AND_ASSERT_THROW_MES(hardfork_id < m_total_count, "invalid hardfork id: " << hardfork_id);
      return height > m_height_the_hardfork_n_active_after[hardfork_id];
    }

    std::string get_str_height_the_hardfork_active_after(size_t hardfork_id) const
    {
      if (hardfork_id == 0)
        return "0"; // hardfork #0 is a special case that is considered always active 
      CHECK_AND_ASSERT_THROW_MES(hardfork_id < m_total_count, "invalid hardfork id: " << hardfork_id);
      return epee::string_tools::num_to_string_fast(m_height_the_hardfork_n_active_after[hardfork_id]);
    }

    size_t get_the_most_recent_hardfork_id_for_height(uint64_t height) const
    {
      for(size_t hid = m_total_count - 1; hid != 0; --hid) // 0 is not including
      {
        if(is_hardfork_active_for_height(hid, height))
          return hid;
      }
      return 0;
    }

    uint8_t get_block_major_version_by_height(uint64_t height) const
    {      
      if (!this->is_hardfork_active_for_height(1, height))
        return BLOCK_MAJOR_VERSION_INITIAL;
      else if (!this->is_hardfork_active_for_height(3, height))
        return HF1_BLOCK_MAJOR_VERSION;
      else if (!this->is_hardfork_active_for_height(4, height))
        return HF3_BLOCK_MAJOR_VERSION;
      else
        return CURRENT_BLOCK_MAJOR_VERSION;
    }

    uint8_t get_block_minor_version_by_height(uint64_t height) const
    {
       return HF3_BLOCK_MINOR_VERSION;
    }

    bool operator==(const hard_forks_descriptor& rhs) const
    {
      return m_height_the_hardfork_n_active_after == rhs.m_height_the_hardfork_n_active_after;
    }

    bool operator!=(const hard_forks_descriptor& rhs) const
    {
      return ! operator==(rhs);
    }
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

    hard_forks_descriptor hard_forks;
    std::array<int, ZANO_HARDFORKS_TOTAL> min_build_numbers_for_hard_forks;

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

  inline core_runtime_config get_default_core_runtime_config()
  {
    core_runtime_config pc{};
    pc.min_coinstake_age = POS_MINIMUM_COINSTAKE_AGE;
    pc.pos_minimum_heigh = POS_START_HEIGHT;
    pc.tx_pool_min_fee = TX_MINIMUM_FEE;
    pc.tx_default_fee = TX_DEFAULT_FEE;
    pc.max_alt_blocks = CURRENCY_ALT_BLOCK_MAX_COUNT;
    pc.hf4_minimum_mixins = CURRENCY_HF4_MANDATORY_DECOY_SET_SIZE;
    pc.max_pos_difficulty = wide_difficulty_type(POS_MAX_DIFFICULTY_ALLOWED);
    
    // TODO: refactor the following
    pc.hard_forks.set_hardfork_height(1, ZANO_HARDFORK_01_AFTER_HEIGHT);
    pc.hard_forks.set_hardfork_height(2, ZANO_HARDFORK_02_AFTER_HEIGHT);
    pc.hard_forks.set_hardfork_height(3, ZANO_HARDFORK_03_AFTER_HEIGHT);
    pc.hard_forks.set_hardfork_height(4, ZANO_HARDFORK_04_AFTER_HEIGHT);
    pc.hard_forks.set_hardfork_height(5, ZANO_HARDFORK_05_AFTER_HEIGHT); pc.min_build_numbers_for_hard_forks[5] = ZANO_HARDFORK_05_MIN_BUILD_VER;
    static_assert(5 + 1 == ZANO_HARDFORKS_TOTAL);

    pc.get_core_time = &core_runtime_config::_default_core_time_function;
    bool r = epee::string_tools::hex_to_pod(ALIAS_SHORT_NAMES_VALIDATION_PUB_KEY, pc.alias_validation_pubkey);
    CHECK_AND_ASSERT_THROW_MES(r, "failed to parse alias_validation_pub_key");
    return pc;
  }
}
