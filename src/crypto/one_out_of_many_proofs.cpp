// Copyright (c) 2023 Zano Project
// Copyright (c) 2023 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include "one_out_of_many_proofs.h"
#include "../currency_core/crypto_config.h"
#include "epee/include/misc_log_ex.h"

//DISABLE_GCC_AND_CLANG_WARNING(unused-function)

#if 0
#  define DBG_VAL_PRINT(x) std::cout << std::setw(30) << std::left << #x ": " << x << std::endl
#  define DBG_PRINT(x)     std::cout << x << std::endl
#else
#  define DBG_VAL_PRINT(x) (void(0))
#  define DBG_PRINT(x)     (void(0))
#endif

namespace crypto
{
  static const size_t N_max = 256;
  static const size_t mn_max = 16;

  const point_t& get_BGE_generator(size_t index, bool& ok)
  {
    static std::vector<point_t> precalculated_generators;
    if (precalculated_generators.empty())
    {
      precalculated_generators.resize(mn_max * 2);

      scalar_t hash_buf[2] = { hash_helper_t::hs("Zano BGE generator"), 0 };

      for(size_t i = 0; i < precalculated_generators.size(); ++i)
      {
        hash_buf[1].m_u64[0] = i;
        precalculated_generators[i] = hash_helper_t::hp(&hash_buf, sizeof hash_buf);
      }
    }

    if (index >= mn_max * 2)
    {
      ok = false;
      return c_point_0;
    }

    ok = true;
    return precalculated_generators[index];
  }




#define CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(cond, err_code) \
    if (!(cond)) { LOG_PRINT_RED("generate_BGE_proof: \"" << #cond << "\" is false at " << LOCATION_SS << ENDL << "error code = " << (int)err_code, LOG_LEVEL_3); \
    if (p_err) { *p_err = err_code; } return false; }

  bool generate_BGE_proof(const hash& context_hash, const std::vector<point_t>& ring, const scalar_t& secret, const size_t secret_index, BGE_proof& result, uint8_t* p_err /* = nullptr */)
  {
    DBG_PRINT(" - - - generate_BGE_proof - - -");
    size_t N = ring.size();
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(N > 0, 0);
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(secret_index < N, 1);

#ifndef NDEBUG
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(ring[secret_index] == secret * crypto::c_point_X, 2);
#endif

    return true;
  }


  bool verify_BGE_proof(const hash& context_hash, const std::vector<const public_key*>& ring, BGE_proof& result, uint8_t* p_err /* = nullptr */)
  {
    return false;
  }


} // namespace crypto
