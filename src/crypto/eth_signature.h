// Copyright (c) 2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include <cstdint>
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

  // generates secp256k1 ECDSA signature
  bool generate_eth_signature(const hash& m, const eth_secret_key& sec_key, eth_signature& sig) noexcept;

  // verifies secp256k1 ECDSA signature
  bool verify_eth_signature(const hash& m, const eth_public_key& pub_key, const eth_signature& sig) noexcept;


} // namespace crypto
