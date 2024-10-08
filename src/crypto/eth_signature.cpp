// Copyright (c) 2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "eth_signature.h"
#include "crypto.h"
#include "bitcoin-secp256k1/include/secp256k1.h"
#include "random.h"
#include "misc_language.h"
#include <string_tools.h>


namespace crypto
{
  bool generate_eth_key_pair(eth_secret_key& sec_key, eth_public_key& pub_key) noexcept
  {
    try
    {
      secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
      auto slh = epee::misc_utils::create_scope_leave_handler([&ctx](){
          secp256k1_context_destroy(ctx);
          ctx = nullptr;
        });

      uint8_t randomness[32];
      crypto::generate_random_bytes(sizeof randomness, randomness);
      if (!secp256k1_context_randomize(ctx, randomness))
        return false;

      for(size_t i = 1024; i != 0; --i)
      {
        crypto::generate_random_bytes(sizeof sec_key, sec_key.data);
        if (secp256k1_ec_seckey_verify(ctx, sec_key.data))
          break;
        if (i == 1)
          return false;
      }

      secp256k1_pubkey uncompressed_pub_key{};
      if (!secp256k1_ec_pubkey_create(ctx, &uncompressed_pub_key, sec_key.data))
        return false;

      size_t output_len = sizeof pub_key;
      if (!secp256k1_ec_pubkey_serialize(ctx, pub_key.data, &output_len, &uncompressed_pub_key, SECP256K1_EC_COMPRESSED))
        return false;

      return true;
    }
    catch(...)
    {
      return false;
    }
  }

  bool eth_secret_key_to_public_key(const eth_secret_key& sec_key, eth_public_key& pub_key) noexcept
  {
    try
    {
      // TODO: do we need this? consider using static context
      secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
      auto slh = epee::misc_utils::create_scope_leave_handler([&ctx](){
        secp256k1_context_destroy(ctx);
        ctx = nullptr;
        });

      secp256k1_pubkey uncompressed_pub_key{};
      if (!secp256k1_ec_pubkey_create(ctx, &uncompressed_pub_key, sec_key.data))
        return false;

      size_t output_len = sizeof pub_key;
      if (!secp256k1_ec_pubkey_serialize(ctx, pub_key.data, &output_len, &uncompressed_pub_key, SECP256K1_EC_COMPRESSED))
        return false;

      return true;
    }
    catch(...)
    {
      return false;
    }
  }

  // generates secp256k1 ECDSA signature
  bool generate_eth_signature(const hash& m, const eth_secret_key& sec_key, eth_signature& sig) noexcept
  {
    try
    {
      secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
      auto slh = epee::misc_utils::create_scope_leave_handler([&ctx](){
          secp256k1_context_destroy(ctx);
          ctx = nullptr;
        });

      uint8_t randomness[32];
      crypto::generate_random_bytes(sizeof randomness, randomness);
      if (!secp256k1_context_randomize(ctx, randomness))
        return false;
      
      secp256k1_ecdsa_signature secp256k1_ecdsa_sig{};
      if (!secp256k1_ecdsa_sign(ctx, &secp256k1_ecdsa_sig, (const unsigned char*)m.data, sec_key.data, NULL, NULL))
        return false;

      if (!secp256k1_ecdsa_signature_serialize_compact(ctx, sig.data, &secp256k1_ecdsa_sig))
        return false;
      
      return true;
    }
    catch(...)
    {
      return false;
    }
  }

  // verifies secp256k1 ECDSA signature
  bool verify_eth_signature(const hash& m, const eth_public_key& pub_key, const eth_signature& sig) noexcept
  {
    try
    {
      // TODO (performance) consider using secp256k1_context_static for verification -- sowle

      secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
      auto slh = epee::misc_utils::create_scope_leave_handler([&ctx](){
          secp256k1_context_destroy(ctx);
          ctx = nullptr;
        });

      uint8_t randomness[32];
      crypto::generate_random_bytes(sizeof randomness, randomness);
      if (!secp256k1_context_randomize(ctx, randomness))
        return false;

      secp256k1_ecdsa_signature secp256k1_ecdsa_sig{};
      secp256k1_pubkey uncompressed_pub_key{};

      if (!secp256k1_ecdsa_signature_parse_compact(ctx, &secp256k1_ecdsa_sig, sig.data))
        return false;

      if (!secp256k1_ec_pubkey_parse(ctx, &uncompressed_pub_key, pub_key.data, sizeof pub_key))
        return false;

      // verify a signature
      if (!secp256k1_ecdsa_verify(ctx, &secp256k1_ecdsa_sig, (const unsigned char*)m.data, &uncompressed_pub_key))
        return false;

      return true;
    }
    catch(...)
    {
      return false;
    }
  }

  std::ostream& operator<<(std::ostream& o, const eth_secret_key& v)
  {
    return o << epee::string_tools::pod_to_hex(v);
  }

  std::ostream& operator<<(std::ostream& o, const eth_public_key& v)
  {
    return o << epee::string_tools::pod_to_hex(v);
  }
  std::ostream& operator<<(std::ostream& o, const eth_signature& v)
  {
    return o << epee::string_tools::pod_to_hex(v);
  }


} // namespace crypto
