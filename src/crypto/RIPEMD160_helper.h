// Copyright (c) 2020 The Zano developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "hash.h"


namespace crypto {

#pragma pack(push, 1)
  POD_CLASS hash160{
    char data[20];
  };
#pragma pack(pop)

  void RIPEMD160_hash(const void *data, size_t length, hash160 &h);
  hash160 RIPEMD160_hash(const void *data, size_t length);
  hash RIPEMD160_hash_256(const void *data, size_t length);

}

POD_MAKE_HASHABLE(crypto, hash160)



