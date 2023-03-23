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

#define CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(cond, err_code) \
    if (!(cond)) { LOG_PRINT_RED("generate_BGE_proof: \"" << #cond << "\" is false at " << LOCATION_SS << ENDL << "error code = " << (int)err_code, LOG_LEVEL_3); \
    if (p_err) { *p_err = err_code; } return false; }

  bool generate_BGE_proof(const hash& m, const std::vector<point_t>& ring, const scalar_t& secret, const size_t secret_index, BGE_proof& result, uint8_t* p_err /* = nullptr */)
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


  bool verify_BGE_proof(const hash& m, const std::vector<const public_key*>& ring, BGE_proof& result, uint8_t* p_err /* = nullptr */)
  {
    return false;
  }


} // namespace crypto
