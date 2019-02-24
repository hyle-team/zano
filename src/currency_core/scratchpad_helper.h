// Copyright (c) 2014-2018 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "crypto/wild_keccak.h"
#include "currency_protocol/blobdatatype.h"
#include "currency_core/currency_basic.h"
#include "cache_helper.h"

namespace currency
{
  template<class t_parent>
  class scratchpad_keeper_base
  {
  public:
    crypto::hash get_pow_hash(const block& b, const crypto::hash& scr_seed)
    {
      blobdata bl = get_block_hashing_blob(b);
      return static_cast<t_parent*>(this)->get_pow_hash_from_blob(bl, get_block_height(b), scr_seed);
    }
  };


  class scratchpad_keeper: public scratchpad_keeper_base<scratchpad_keeper>
  {
  public:
    scratchpad_keeper();
    bool generate(const crypto::hash& seed, uint64_t height);
    crypto::hash get_pow_hash_from_blob(const blobdata& bd, uint64_t height, const crypto::hash& seed);
    uint64_t size();
  private:
    scratchpad_keeper(const scratchpad_keeper&) {}
    crypto::hash m_seed;
    std::vector<crypto::hash> m_scratchpad;
    std::recursive_mutex m_lock;
  };


  class scratchpad_light_pool : public scratchpad_keeper_base<scratchpad_light_pool>
  {
  public:
    scratchpad_light_pool() {}
    crypto::hash get_pow_hash_from_blob(const blobdata& bd, uint64_t height, const crypto::hash& seed);
  private:
    //map of seed to 
    epee::misc_utils::cache_base<false, crypto::hash, std::shared_ptr<std::vector<crypto::hash>>, 4> m_scratchpad_pools;
    std::recursive_mutex m_lock;
  };

}