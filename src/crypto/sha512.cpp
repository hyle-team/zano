// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include "sha512.h"
#include "epee/include/misc_language.h"
#include <openssl/evp.h>

namespace crypto
{

  bool sha512(const void* data, size_t data_size, hash64& result) noexcept
  {
    static_assert(sizeof result <= EVP_MAX_MD_SIZE, "size missmatch");

    try
    {
      EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
      auto slh = epee::misc_utils::create_scope_leave_handler([mdctx](){ EVP_MD_CTX_free(mdctx); });

      const EVP_MD* md = EVP_get_digestbyname("sha512");
      EVP_DigestInit_ex(mdctx, md, nullptr);
      EVP_DigestUpdate(mdctx, data, data_size);
      unsigned int md_len = 0;
      EVP_DigestFinal_ex(mdctx, result.data, &md_len);
    }
    catch(...)
    {
      return false;
    }
    return true;
  }


  bool operator==(const hash64& lhs, const hash64& rhs)
  {
    return memcmp(lhs.data, rhs.data, sizeof lhs.data) == 0;
  }



} // namespace crypto
