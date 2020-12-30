// Copyright (c) 2020 The Zano developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "hash.h"
extern "C" {
#include "RIPEMD160.h"
}



namespace crypto {

#pragma pack(push, 1)
  POD_CLASS hash160{
    char data[20];
  };
#pragma pack(pop)

  inline void RIPEMD160_hash(const void *data, std::size_t length, hash &h)
  {
    void MDinit(dword *MDbuf);
    void compress(dword *MDbuf, dword *X);
    void MDfinish(dword *MDbuf, byte *strptr, dword lswlen, dword mswlen);
  }

  inline hash RIPEMD160_hash(const void *data, std::size_t length)
  {
    hash h;
    RIPEMD160_hash(data, length, reinterpret_cast<char *>(&h));
    return h;
  }

}

POD_MAKE_HASHABLE(crypto, hash)
