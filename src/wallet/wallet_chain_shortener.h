// Copyright (c) 2014-2018 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#pragma once
#include <memory>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/singleton.hpp>
#include <boost/serialization/extended_type_info.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <atomic>
#include <map>

#include "crypto/crypto.h"

#include "include_base_utils.h"
#include "crypto/crypto.h"
#include "currency_core/currency_basic.h"

class wallet_chain_shortener
{
public:
  void push_new_block_id(const crypto::hash& id, uint64_t height);
  uint64_t get_top_block_height() const;
  uint64_t get_blockchain_current_size() const;
  void get_short_chain_history(std::list<crypto::hash>& ids)const;
  bool lookup_item_around(uint64_t i, std::pair<uint64_t, crypto::hash>& result)const;
  void check_if_block_matched(uint64_t i, const crypto::hash& id, bool& block_found, bool& block_matched, bool& full_reset_needed) const;
  void detach(uint64_t including_height);
  void clear();
  void set_genesis(const crypto::hash& id);
  const crypto::hash& get_genesis();
  template <class t_archive>
  inline void serialize(t_archive &a, const unsigned int ver)
  {
    a & m_local_bc_size;
    a & m_genesis;
    a & m_last_20_blocks;
    a & m_last_144_blocks_every_10;
    a & m_last_144_blocks_every_100;
    a & m_last_144_blocks_every_1000;
  }

  //debug functions
  std::string get_internal_state_text() const;
private:
  std::atomic<uint64_t> m_local_bc_size = 1; //temporary workaround 
  crypto::hash m_genesis = currency::gdefault_genesis;
  std::map<uint64_t, crypto::hash> m_last_20_blocks;
  std::map<uint64_t, crypto::hash> m_last_144_blocks_every_10;   //1 day
  std::map<uint64_t, crypto::hash> m_last_144_blocks_every_100;  //10 days
  std::map<uint64_t, crypto::hash> m_last_144_blocks_every_1000; //100 days
};