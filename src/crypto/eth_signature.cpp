// Copyright (c) 2024-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "eth_signature.h"
#include "crypto.h"
#include "random.h"
#include "misc_language.h"
#include <string_tools.h>

#ifdef USE_OPEN_SSL_FOR_ECDSA
  #include <openssl/ec.h>
  #include <openssl/ecdsa.h>
  #include <openssl/obj_mac.h>
  #include <openssl/bn.h>
#else
  #include "bitcoin-secp256k1/include/secp256k1.h"
#endif

//
// OpenSSL EC_KEY vs OpenSSL EVP considerations:
//   OpenSSL EC_KEY functions (used below) are deprecated since 3.0, but the deadline is unknown and they are still present in 4.0. The replacement is OpenSSL EVP,
//   which would require OSSL_PARAM key import and DER round-trip for sig scalars => less clean, convenient and efficient, so I decided to keep EC_KEY currently.
// OpenSSL EC_KEY vs libsecp256k1 considerations:
//   It looks like libsecp256k1 verifies about 9.5x faster than EC_KEY and signs about 15x faster (assuming shared context). It also rejects high-S while OpenSSL
//   version requires manual check (already done below). The price is additional submodule/linking/CMake, while OpenSSL is already used in Zano.
//   As for now, we decided to use OpenSSL EC_KEY version while libsecp256k1 is kept under an #ifdef here just in case we need faster ECDSA in future.
// -- sowle
//

namespace
{
#ifdef USE_OPEN_SSL_FOR_ECDSA
  // creates EC_KEY from raw 32 - byte private key
  EC_KEY* create_ec_key_from_private_key(const unsigned char* private_key)
  {
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!key)
      return nullptr;

    const BIGNUM* curve_order = EC_GROUP_get0_order(EC_KEY_get0_group(key));
    if (!curve_order)
    {
      EC_KEY_free(key);
      return nullptr;
    }

    BIGNUM* priv_key_bn = BN_bin2bn(private_key, 32, nullptr);
    if (!priv_key_bn || BN_is_zero(priv_key_bn) || BN_cmp(priv_key_bn, curve_order) >= 0)
    {
      BN_clear_free(priv_key_bn);
      EC_KEY_free(key);
      return nullptr;
    }

    if (!EC_KEY_set_private_key(key, priv_key_bn))
    {
      BN_clear_free(priv_key_bn);
      EC_KEY_free(key);
      return nullptr;
    }

    BN_clear_free(priv_key_bn);
    return key;
  }

  // creates EC_KEY from raw 33-byte compressed public key
  EC_KEY* create_ec_key_from_compressed_public_key(const unsigned char* compressed_pub_key)
  {
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!key)
      return nullptr;

    EC_POINT* pub_point = EC_POINT_new(EC_KEY_get0_group(key));
    if (!pub_point || !EC_POINT_oct2point(EC_KEY_get0_group(key), pub_point, compressed_pub_key, 33, nullptr))
    {
      EC_POINT_free(pub_point);
      EC_KEY_free(key);
      return nullptr;
    }

    if (!EC_KEY_set_public_key(key, pub_point))
    {
      EC_POINT_free(pub_point);
      EC_KEY_free(key);
      return nullptr;
    }

    EC_POINT_free(pub_point);
    return key;
  }
#endif
} // namespace



namespace crypto
{
  bool generate_eth_key_pair(eth_secret_key& sec_key, eth_public_key& pub_key) noexcept
  {
    try
    {
#ifdef USE_OPEN_SSL_FOR_ECDSA
      EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
      if (!key)
        return false;

      auto slh = epee::misc_utils::create_scope_leave_handler([&](){ EC_KEY_free(key); });

      if (EC_KEY_generate_key(key) != 1)
        return false;

      // extract the private key in BN format
      const BIGNUM* sec_bn = EC_KEY_get0_private_key(key);
      if (!sec_bn)
        return false;

      static_assert(sizeof sec_key.data == 32, "size mismatch");
      if (BN_bn2binpad(sec_bn, sec_key.data, 32) != 32)
        return false;

      // extract the public key in compressed format
      const EC_POINT* pub_pt = EC_KEY_get0_public_key(key);
      if (!pub_pt)
        return false;

      static_assert(sizeof pub_key.data == 33, "size mismatch");
      if (EC_POINT_point2oct(EC_KEY_get0_group(key), pub_pt, POINT_CONVERSION_COMPRESSED, pub_key.data, 33, nullptr) != 33)
        return false;

      return true;
#else
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
#endif
    }
    catch(...)
    {
      return false;
    }
  }

#ifndef USE_OPEN_SSL_FOR_ECDSA
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
#endif
  // generates secp256k1 ECDSA signature
  bool generate_eth_signature(const hash& m, const eth_secret_key& sec_key, eth_signature& sig) noexcept
  {
    try
    {
#ifdef USE_OPEN_SSL_FOR_ECDSA
      EC_KEY* ec_key = create_ec_key_from_private_key(sec_key.data);
      if (!ec_key)
        return false;

      auto slh_ec_key = epee::misc_utils::create_scope_leave_handler([&](){ EC_KEY_free(ec_key); });

      static_assert(sizeof m == 32, "size mismatch");
      ECDSA_SIG* ecdsa_sig = ECDSA_do_sign(reinterpret_cast<const uint8_t*>(&m), 32, ec_key);
      if (!ecdsa_sig)
        return false;

      auto slh_ecdsa_sig = epee::misc_utils::create_scope_leave_handler([&](){ ECDSA_SIG_free(ecdsa_sig); });

      const BIGNUM* r = nullptr, *s = nullptr;
      ECDSA_SIG_get0(ecdsa_sig, &r, &s);

      // ensure canonical s
      const BIGNUM* curve_order = EC_GROUP_get0_order(EC_KEY_get0_group(ec_key));
      if (!curve_order)
        return false;

      BIGNUM* s_canonical = BN_dup(s); // s_canonical = s
      BIGNUM* half_order = BN_new();
      auto slh_s_c_h = epee::misc_utils::create_scope_leave_handler([&](){ BN_free(s_canonical); BN_free(half_order); });

      if (!s_canonical || !half_order)
        return false;

      if (BN_rshift1(half_order, curve_order) != 1)
        return false;

      if (BN_cmp(s_canonical, half_order) > 0 && BN_sub(s_canonical, curve_order, s_canonical) != 1) // if (s > order / 2) s = order - s
        return false;

      static_assert(sizeof sig.data == 64, "size mismatch");
      if (BN_bn2binpad(r, &sig.data[0], 32) != 32)
        return false;

      if (BN_bn2binpad(s_canonical, &sig.data[32], 32) != 32)
        return false;

      return true;
#else 
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
#endif
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
#ifdef USE_OPEN_SSL_FOR_ECDSA
      EC_KEY* ec_key = create_ec_key_from_compressed_public_key(pub_key.data);
      if (!ec_key)
        return false;

      static_assert(sizeof sig.data == 64, "size mismatch");
      BIGNUM* half_order = BN_new();
      BIGNUM* r_bn = BN_bin2bn(sig.data,      32, nullptr);
      BIGNUM* s_bn = BN_bin2bn(sig.data + 32, 32, nullptr);
      ECDSA_SIG* ecdsa_sig = ECDSA_SIG_new();

      auto slh_rss = epee::misc_utils::create_scope_leave_handler([&](){ ECDSA_SIG_free(ecdsa_sig); BN_free(s_bn); BN_free(r_bn); BN_free(half_order); EC_KEY_free(ec_key); });

      const BIGNUM* curve_order = EC_GROUP_get0_order(EC_KEY_get0_group(ec_key));

      if (!half_order || !r_bn || !s_bn || !ecdsa_sig || !curve_order)
        return false;

      // ensure s <= order/2
      if (BN_rshift1(half_order, curve_order) != 1 || BN_cmp(s_bn, half_order) > 0)
        return false;

      if (!ECDSA_SIG_set0(ecdsa_sig, r_bn, s_bn))
        return false;

      r_bn = nullptr; // ecdsa_sig now owns both of them
      s_bn = nullptr;

      static_assert(sizeof m == 32, "size mismatch");
      if (ECDSA_do_verify(reinterpret_cast<const uint8_t*>(&m), 32, ecdsa_sig, ec_key) != 1)
        return false;

      return true;
#else
      secp256k1_ecdsa_signature secp256k1_ecdsa_sig{};
      secp256k1_pubkey uncompressed_pub_key{};

      if (!secp256k1_ecdsa_signature_parse_compact(secp256k1_context_static, &secp256k1_ecdsa_sig, sig.data))
        return false;

      if (!secp256k1_ec_pubkey_parse(secp256k1_context_static, &uncompressed_pub_key, pub_key.data, sizeof pub_key))
        return false;

      // verify a signature
      if (!secp256k1_ecdsa_verify(secp256k1_context_static, &secp256k1_ecdsa_sig, (const unsigned char*)m.data, &uncompressed_pub_key))
        return false;

      return true;
#endif
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
