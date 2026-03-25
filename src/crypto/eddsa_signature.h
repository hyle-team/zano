// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include <cstdint>
#include <iosfwd>
#include <string>
#include "hash.h"

namespace crypto
{

  struct eddsa_seed
  {
    uint8_t data[32];
  };

  struct eddsa_sec_prefix 
  {
    uint8_t data[32];
  };

  struct eddsa_secret_key 
  {
    uint8_t data[32];
  };

  struct eddsa_public_key
  {
    uint8_t data[32];
  };

  struct eddsa_signature
  {
    uint8_t data[64];
  };

  /*
    seed (32 bytes)                           <- this is what RFC 8032 calls "secret key"
    │
    |
    sha512(seed) (64 bytes)
    │
    |- first 32 bytes -> clamp -> scalar a    <- real secret key, meaning a * G = public key A
    │
    |- last 32 bytes  -> "prefix"             <- used for nonce derivation r = sc_reduce(sha512(prefix | m)) instead of random
  */

  bool eddsa_generate_random_seed(eddsa_seed& seed) noexcept;
  bool eddsa_seed_to_secret_key_public_key_and_prefix(const eddsa_seed& seed, eddsa_secret_key& sec_key, eddsa_public_key& pub_key, eddsa_sec_prefix& prefix) noexcept;
  bool generate_eddsa_signature(const std::string& m, const eddsa_sec_prefix& prefix, const eddsa_secret_key& sec_key, const eddsa_public_key& pub_key, eddsa_signature& sig) noexcept;
  bool verify_eddsa_signature(const std::string& m, const eddsa_public_key& pub_key, const eddsa_signature& sig) noexcept;
  
  bool generate_eddsa_signature(const hash& m, const eddsa_sec_prefix& prefix, const eddsa_secret_key& sec_key, const eddsa_public_key& pub_key, eddsa_signature& sig) noexcept;
  bool verify_eddsa_signature(const hash& m, const eddsa_public_key& pub_key, const eddsa_signature& sig) noexcept;


  inline bool operator==(const eddsa_public_key& lhs, const eddsa_public_key& rhs)
  {
    return memcmp(lhs.data, rhs.data, sizeof lhs.data) == 0;
  }

  inline bool operator!=(const eddsa_public_key& lhs, const eddsa_public_key & rhs)
  {
    return !(lhs == rhs);
  }

  inline bool operator==(const eddsa_secret_key& lhs, const eddsa_secret_key& rhs)
  {
    return memcmp(lhs.data, rhs.data, sizeof lhs.data) == 0;
  }

  inline bool operator!=(const eddsa_secret_key& lhs, const eddsa_secret_key& rhs)
  {
    return !(lhs == rhs);
  }

  inline bool operator==(const eddsa_signature& lhs, const eddsa_signature& rhs)
  {
    return memcmp(lhs.data, rhs.data, sizeof lhs.data) == 0;
  }

  inline bool operator!=(const eddsa_signature& lhs, const eddsa_signature& rhs)
  {
    return !(lhs == rhs);
  }

  std::ostream& operator<<(std::ostream& o, const eddsa_secret_key & v);
  std::ostream& operator<<(std::ostream& o, const eddsa_public_key& v);
  std::ostream& operator<<(std::ostream& o, const eddsa_signature& v);

} // namespace crypto
