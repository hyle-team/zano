// Copyright (c) 2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include <cstdint>
#include <iosfwd>
#include "hash.h"

namespace crypto
{

  // secp256k1 public key in serialized (compressed) form that is used in Etherium
  struct eth_public_key
  {
    uint8_t data[33];
  };

  // secp256k1 secret key
  struct eth_secret_key
  {
    uint8_t data[32];
  };

  // secp256k1 ECDSA signature is serialized (compressed) form that is used in Etherium
  struct eth_signature
  {
    uint8_t data[64];
  };

  // generates secp256k1 keypair
  bool generate_eth_key_pair(eth_secret_key& sec_key, eth_public_key& pub_key) noexcept;

  // converts eth_secret_key to eth_public_key
  bool eth_secret_key_to_public_key(const eth_secret_key& sec_key, eth_public_key& pub_key) noexcept;

  // generates secp256k1 ECDSA signature
  bool generate_eth_signature(const hash& m, const eth_secret_key& sec_key, eth_signature& sig) noexcept;

  // verifies secp256k1 ECDSA signature
  bool verify_eth_signature(const hash& m, const eth_public_key& pub_key, const eth_signature& sig) noexcept;


  inline bool operator==(const eth_public_key& lhs, const eth_public_key& rhs)
  {
    return memcmp(lhs.data, rhs.data, sizeof lhs.data) == 0;
  }

  inline bool operator!=(const eth_public_key& lhs, const eth_public_key& rhs)
  {
    return !(lhs == rhs);
  }

  inline bool operator==(const eth_secret_key& lhs, const eth_secret_key& rhs)
  {
    return memcmp(lhs.data, rhs.data, sizeof lhs.data) == 0;
  }

  inline bool operator!=(const eth_secret_key& lhs, const eth_secret_key& rhs)
  {
    return !(lhs == rhs);
  }

  std::ostream& operator<<(std::ostream& o, const eth_secret_key& v);
  std::ostream& operator<<(std::ostream& o, const eth_public_key& v);
  std::ostream& operator<<(std::ostream& o, const eth_signature& v);

} // namespace crypto
