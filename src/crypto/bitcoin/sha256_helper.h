// Copyright (c) 2020 The Zano developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "crypto/hash.h"
#include "sha256.h"


namespace crypto {

  inline void sha256_hash(const void *data, std::size_t length, hash &h) 
  {
    CSHA256 sh;
    sh.Write((const unsigned char*)data, length);
    sh.Finalize((unsigned char* )&h);
  }

  inline hash sha256_hash(const void *data, std::size_t length) 
  {
    hash h;
    sha256_hash(data, length, h);
    return h;
  }

}
