// Copyright (c) 2020 The Zano developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "hash.h"
#include "sha256.h"


namespace crypto {

  inline void sha256_hash(const void *data, std::size_t length, hash &h) 
  {
    CSHA256 sh;
    sh.Write(data, length);
    sh.Finalize(h);
  }

  inline hash sha256_hash(const void *data, std::size_t length) 
  {
    hash h;
    sha256_hash(data, length, reinterpret_cast<char *>(&h));
    return h;
  }

}

POD_MAKE_HASHABLE(crypto, hash)
