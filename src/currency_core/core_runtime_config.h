// Copyright (c) 2014-2020 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "misc_language.h"
#include "string_tools.h"

namespace currency
{
  typedef uint64_t (*core_time_func_t)();
  
  struct hard_forks_descriptor
  {
    uint64_t hard_fork_01_starts_after_height = 0;
    uint64_t hard_fork_02_starts_after_height = 0;
    uint64_t hard_fork_03_starts_after_height = 0;
    uint64_t hard_fork_04_starts_after_height = UINT64_MAX;
  };

  struct core_runtime_config
  {
    uint64_t min_coinstake_age;
    uint64_t pos_minimum_heigh; //height
    uint64_t tx_pool_min_fee;
    uint64_t tx_default_fee;
    uint64_t max_alt_blocks;
    crypto::public_key alias_validation_pubkey;
    core_time_func_t get_core_time;

    hard_forks_descriptor hard_forks;

    bool is_hardfork_active_for_height(size_t hardfork_id, uint64_t height) const
    {
      switch (hardfork_id)
      {
      case 0: return true;
      case 1: return height > hard_forks.hard_fork_01_starts_after_height;
      case 2: return height > hard_forks.hard_fork_02_starts_after_height;
      case 3: return height > hard_forks.hard_fork_03_starts_after_height;
      case 4: return height > hard_forks.hard_fork_04_starts_after_height;
      default: return false;
      }
    }

    static uint64_t _default_core_time_function()
    {
      return time(NULL);
    }
  };

  inline core_runtime_config get_default_core_runtime_config()
  {
    core_runtime_config pc = AUTO_VAL_INIT(pc);
    pc.min_coinstake_age = POS_MINIMUM_COINSTAKE_AGE;
    pc.pos_minimum_heigh = POS_START_HEIGHT;
    pc.tx_pool_min_fee = TX_MINIMUM_FEE;
    pc.tx_default_fee = TX_DEFAULT_FEE;
    pc.max_alt_blocks = CURRENCY_ALT_BLOCK_MAX_COUNT;
    
    pc.hard_forks.hard_fork_01_starts_after_height = ZANO_HARDFORK_01_AFTER_HEIGHT;
    pc.hard_forks.hard_fork_02_starts_after_height = ZANO_HARDFORK_02_AFTER_HEIGHT;
    pc.hard_forks.hard_fork_03_starts_after_height = ZANO_HARDFORK_03_AFTER_HEIGHT;
    pc.hard_forks.hard_fork_04_starts_after_height = ZANO_HARDFORK_04_AFTER_HEIGHT;
    
    pc.get_core_time = &core_runtime_config::_default_core_time_function;
    bool r = epee::string_tools::hex_to_pod(ALIAS_SHORT_NAMES_VALIDATION_PUB_KEY, pc.alias_validation_pubkey);
    CHECK_AND_ASSERT_THROW_MES(r, "failed to parse alias_validation_pub_key");
    return pc;
  }
}
