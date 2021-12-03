// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "include_base_utils.h"
using namespace epee;

#include "checkpoints.h"

namespace currency
{
  //---------------------------------------------------------------------------
  checkpoints::checkpoints()
  {
  }
  //---------------------------------------------------------------------------
  bool checkpoints::add_checkpoint(uint64_t height, const std::string& hash_str)
  {
    crypto::hash h = null_hash;
    bool r = epee::string_tools::parse_tpod_from_hex_string(hash_str, h);
    CHECK_AND_ASSERT_MES(r, false, "WRONG HASH IN CHECKPOINTS!!!");
    CHECK_AND_ASSERT_MES(0 == m_points.count(height), false, "WRONG HASH IN CHECKPOINTS!!!");
    m_points[height] = h;
    return true;
  }
  //---------------------------------------------------------------------------
  bool checkpoints::is_in_checkpoint_zone(uint64_t height) const
  {
    return !m_points.empty() && (height <= (--m_points.end())->first);
  }
  //---------------------------------------------------------------------------
  bool checkpoints::is_height_passed_zone(uint64_t height, uint64_t blockchain_last_block_height) const
  {
    if(height > blockchain_last_block_height)
      return false;

    auto it = m_points.lower_bound(height);   // if found, it->first >= height
    if(it == m_points.end())
      return false;
    if(it->first <= blockchain_last_block_height)
      return true; // this is the case only if height <= it->first <= blockchain_last_block_height
    else 
      return false;
  }
  //---------------------------------------------------------------------------
  uint64_t checkpoints::get_top_checkpoint_height() const 
  {
    if (!m_points.size())
      return 0;
    return (--m_points.end())->first;
  }
  //---------------------------------------------------------------------------
  bool checkpoints::check_block(uint64_t height, const crypto::hash& h) const
  {
    auto it = m_points.find(height);
    if(it == m_points.end())
      return true;

    if(it->second == h)
    {
      LOG_PRINT_GREEN("CHECKPOINT PASSED FOR HEIGHT " << height << " " << h, LOG_LEVEL_0);
      return true;
    }else
    {
      LOG_ERROR("CHECKPOINT FAILED FOR HEIGHT " << height << ". EXPECTED HASH: " << it->second << ", FETCHED HASH: " << h);
      return false;
    }
  }
  //---------------------------------------------------------------------------
  uint64_t checkpoints::get_checkpoint_before_height(uint64_t height) const
  {
    // returns height of the leftmost CP with height that is LESS than the given height
    // ex:
    // If there are two CP at 11 and 15:
    // get_checkpoint_before_height(10) = 0
    // get_checkpoint_before_height(11) = 0
    // get_checkpoint_before_height(12) = 11
    // get_checkpoint_before_height(13) = 11
    // get_checkpoint_before_height(14) = 11
    // get_checkpoint_before_height(15) = 11
    // get_checkpoint_before_height(16) = 15

    uint64_t top_cp = get_top_checkpoint_height();
    if (height > top_cp)
      return top_cp;

    auto it = m_points.lower_bound(height);                // if found, it->first >= height
    if (it == m_points.end() || it == m_points.begin())
      return 0;
    return (--it)->first;
  }
}
