// Copyright (c) 2018-2019 Zano Project

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "scratchpad_helper.h"
#include "currency_format_utils.h"



namespace currency
{

  bool scratchpad_keeper::update(const std::vector<crypto::hash>& seed, uint64_t height)
  {
    return crypto::generate_scratchpad(seed, m_scratchpad, get_scratchpad_size_by_height(height));
  }
  crypto::hash scratchpad_keeper::get_pow_hash(const blobdata& bd, uint64_t height)
  {
    CHECK_AND_ASSERT_THROW_MES(get_scratchpad_size_by_height(height) == this->size(), "Fatal error on hash calculation: scratchpad_size=" << m_scratchpad.size() << " at height=" << height);
    crypto::hash res_hash = null_hash;
    bool res = get_wild_keccak2(bd, res_hash, m_scratchpad);
    CHECK_AND_ASSERT_THROW_MES(res, "Fatal error on hash calculation: scratchpad_size=" << m_scratchpad.size());
    return res_hash;
  }
  uint64_t scratchpad_keeper::size()
  {
    return m_scratchpad.size();
  }

}