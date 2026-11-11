// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <map>

#include "currency_core/currency_format_utils.h"

namespace currency
{
  class checkpoints
  {
  public:
    checkpoints();
    bool add_checkpoint(uint64_t height, const std::string& hash_str);
    bool is_in_checkpoint_zone(uint64_t height) const;
    bool is_height_passed_zone(uint64_t height, uint64_t blockchain_last_block_height) const;
    bool check_block(uint64_t height, const crypto::hash& h) const;
    uint64_t get_top_checkpoint_height() const;

    uint64_t get_checkpoint_before_height(uint64_t height) const;
  private:
    std::map<uint64_t, crypto::hash> m_points;
  };
}
