// Copyright (c) 2018-2019 Zano Project

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "scratchpad_helper.h"
#include "currency_format_utils.h"



namespace currency
{
  scratchpad_keeper::scratchpad_keeper():m_seed(null_hash)
  {

  }
  //------------------------------------------------------------------------------------
  bool scratchpad_keeper::generate(const crypto::hash& scr_seed, uint64_t height)
  {
    bool r = false;
    CRITICAL_REGION_BEGIN(m_lock);
    r = crypto::generate_scratchpad(scr_seed, m_scratchpad, get_scratchpad_size_for_height(height));
    if (r)
      m_seed = scr_seed;
    CRITICAL_REGION_END();
    return r;
  }
  //------------------------------------------------------------------------------------
  crypto::hash scratchpad_keeper::get_pow_hash_from_blob(const blobdata& bd, uint64_t height, const crypto::hash& scr_seed)
  {
    CRITICAL_REGION_LOCAL(m_lock);
    crypto::hash res_hash = null_hash;
    if (scr_seed != m_seed || get_scratchpad_size_for_height(height) != this->size())
    {
      bool r = generate(scr_seed, height);
      CHECK_AND_ASSERT_THROW_MES(r, "Unable to generate scratchpad");
    }
    CHECK_AND_ASSERT_THROW_MES(get_scratchpad_size_for_height(height) == this->size(), "Fatal error on hash calculation: scratchpad_size=" << m_scratchpad.size() << " at height=" << height << ", scr_seed=" << scr_seed << ", m_seed=" << m_seed);
    CHECK_AND_ASSERT_THROW_MES(scr_seed == m_seed, "Fatal error on hash calculation: scratchpad_seed missmatch scr_seed=" << scr_seed << ", m_seed=" << m_seed);

    bool res = get_wild_keccak2(bd, res_hash, m_scratchpad);
    CHECK_AND_ASSERT_THROW_MES(res, "Fatal error on hash calculation: scratchpad_size=" << m_scratchpad.size());
    return res_hash;
  }
  //------------------------------------------------------------------------------------

  //------------------------------------------------------------------------------------
  uint64_t scratchpad_keeper::size()
  {
    return m_scratchpad.size();
  }
  //------------------------------------------------------------------------------------
  crypto::hash scratchpad_light_pool::get_pow_hash_from_blob(const blobdata& bd, uint64_t height, const crypto::hash& seed)
  {
    CRITICAL_REGION_LOCAL(m_lock);
    std::shared_ptr<std::vector<crypto::hash>> pscr_light;
    if (!m_scratchpad_pools.get(seed, pscr_light))
    {
      LOG_PRINT_MAGENTA("Generating scratchpad light for " << seed << "["<< height <<"]", LOG_LEVEL_0);
      pscr_light.reset(new std::vector<crypto::hash>());
      bool r = crypto::generate_scratchpad_light(seed, *pscr_light, currency::get_scratchpad_size_for_height(height));
      CHECK_AND_ASSERT_THROW_MES(r, "Failed to generate_scratchpad_light");
      m_scratchpad_pools.set(seed, pscr_light);
      LOG_PRINT_MAGENTA("Generated ok", LOG_LEVEL_0);
    }
    CHECK_AND_ASSERT_THROW_MES(pscr_light->size()*10 == currency::get_scratchpad_size_for_height(height),
      "Wrong size of cached scratchpad = " << pscr_light->size() << ", expected " << currency::get_scratchpad_size_for_height(height) << " for height " << height);
    crypto::hash res = currency::null_hash;
    bool r = crypto::get_wild_keccak_light(bd, res, *pscr_light);
    CHECK_AND_ASSERT_THROW_MES(r, "Failed to get_wild_keccak_light");
    return res;
  }
  //------------------------------------------------------------------------------------
}

