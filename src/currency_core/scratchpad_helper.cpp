// Copyright (c) 2018-2019 Zano Project

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "scratchpad_helper.h"
#include "currency_format_utils.h"



namespace currency
{

  bool scratchpad_keeper::generate(const crypto::hash& scr_seed, uint64_t height)
  {
    bool r = crypto::generate_scratchpad(scr_seed, m_scratchpad, get_scratchpad_size_for_height(height));
    if (r)
      m_seed = scr_seed;
    return r;
  }
  crypto::hash scratchpad_keeper::get_pow_hash(const blobdata& bd, uint64_t height, const crypto::hash& scr_seed)
  {
    if (scr_seed != m_seed)
    {
      bool r = generate(scr_seed, height);
      CHECK_AND_ASSERT_THROW_MES(r, "Unable to generate scratchpad");
    }
    CHECK_AND_ASSERT_THROW_MES(get_scratchpad_size_for_height(height) == this->size(), "Fatal error on hash calculation: scratchpad_size=" << m_scratchpad.size() << " at height=" << height);
    CHECK_AND_ASSERT_THROW_MES(scr_seed == m_seed, "Fatal error on hash calculation: scratchpad_seed missmatch");
    crypto::hash res_hash = null_hash;
    bool res = get_wild_keccak2(bd, res_hash, m_scratchpad);
    CHECK_AND_ASSERT_THROW_MES(res, "Fatal error on hash calculation: scratchpad_size=" << m_scratchpad.size());
    return res_hash;
  }
  crypto::hash scratchpad_keeper::get_pow_hash(const block& b, const crypto::hash& scr_seed)
  {
    blobdata bl = get_block_hashing_blob(b);
    return get_pow_hash(bl, get_block_height(b), scr_seed);
  }
  uint64_t scratchpad_keeper::size()
  {
    return m_scratchpad.size();
  }

}