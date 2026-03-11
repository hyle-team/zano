// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include <cstdint>

namespace crypto
{

#pragma pack(push, 1)
  struct hash64
  {
    uint8_t data[64];
  };
#pragma pack(pop)

  bool operator==(const hash64& lhs, const hash64& rhs);

  inline bool operator!=(const hash64& lhs, const hash64& rhs)
  {
    return !(lhs == rhs);
  }


  bool sha512(const void* data, size_t data_size, hash64& result) noexcept;


} // namespace crypto
