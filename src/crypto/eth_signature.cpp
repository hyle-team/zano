// Copyright (c) 2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "eth_signature.h"
#include "crypto.h"
#include "bitcoin-secp256k1/include/secp256k1.h"
#include "random.h"
#include "misc_language.h"
#include <string_tools.h>


#define USE_OPEN_SSL_FOR_ETH

#ifdef USE_OPEN_SSL_FOR_ETH
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#endif





// Function to create EC_KEY from raw 32 - byte private key
EC_KEY * create_ec_key_from_private_key(const unsigned char* private_key) {
  EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
  if (!key) {
    std::cerr << "Failed to create new EC Key" << std::endl;
    return nullptr;
  }

  BIGNUM* priv_key_bn = BN_bin2bn(private_key, 32, nullptr);
  if (!priv_key_bn) {
    std::cerr << "Failed to convert private key to BIGNUM" << std::endl;
    EC_KEY_free(key);
    return nullptr;
  }

  if (!EC_KEY_set_private_key(key, priv_key_bn)) {
    std::cerr << "Failed to set private key" << std::endl;
    EC_KEY_free(key);
    BN_free(priv_key_bn);
    return nullptr;
  }

  BN_free(priv_key_bn);
  return key;
}


void ensure_canonical_s(BIGNUM* s, const EC_GROUP* group) {
  // Get the order of the curve
  BIGNUM* order = BN_new();
  EC_GROUP_get_order(group, order, nullptr);

  // Compute half of the order: `n / 2`
  BIGNUM* half_order = BN_new();
  BN_rshift1(half_order, order);

  // If `s` is greater than `n / 2`, replace `s` with `n - s`
  if (BN_cmp(s, half_order) > 0) {
    BN_sub(s, order, s);
  }

  BN_free(order);
  BN_free(half_order);
}

// Update the function to ensure canonical `s`
bool generate_ethereum_signature(const unsigned char* hash, const unsigned char* private_key, crypto::eth_signature& sig_res) {
  EC_KEY* ec_key = create_ec_key_from_private_key(private_key);
  if (!ec_key) {
    throw std::runtime_error("Failed to create EC key from private key");
  }

  // Sign the hash
  unsigned int sig_len = ECDSA_size(ec_key);
  std::vector<unsigned char> signature(sig_len);
  if (ECDSA_sign(0, hash, 32, signature.data(), &sig_len, ec_key) == 0) {
    EC_KEY_free(ec_key);
    throw std::runtime_error("Failed to create signature");
  }
  signature.resize(sig_len);

 
  // The OpenSSL ECDSA signature output is DER encoded, Ethereum expects (r, s, v)
  const unsigned char* p = signature.data();
  ECDSA_SIG* sig = d2i_ECDSA_SIG(nullptr, &p, sig_len);
  if (!sig) {
    EC_KEY_free(ec_key);
    throw std::runtime_error("Failed to parse ECDSA signature");
  }

  const BIGNUM* r = nullptr;
  const BIGNUM* s = nullptr;
  ECDSA_SIG_get0(sig, &r, &s);

  // Ensure canonical `s`
  BIGNUM* s_canonical = BN_dup(s);
  ensure_canonical_s(s_canonical, EC_KEY_get0_group(ec_key));

  //std::vector<unsigned char> r_bytes(32);
  //std::vector<unsigned char> s_bytes(32);

  //BN_bn2binpad(r, r_bytes.data(), 32);
  //BN_bn2binpad(s_canonical, s_bytes.data(), 32);
  BN_bn2binpad(r, (unsigned char* )&sig_res.data[0], 32);
  BN_bn2binpad(s_canonical, (unsigned char*)&sig_res.data[32], 32);



  // To determine the recovery ID (v), you'd need to use custom logic to determine this.
  //unsigned char v = 27; // Placeholder

  //std::vector<unsigned char> eth_signature(65);
  //std::copy(r_bytes.begin(), r_bytes.end(), eth_signature.begin());
  //std::copy(s_bytes.begin(), s_bytes.end(), eth_signature.begin() + 32);
  //eth_signature[64] = v;

  ECDSA_SIG_free(sig);
  BN_free(s_canonical);
  EC_KEY_free(ec_key);
  return true;
}

// Convert raw 33-byte compressed public key to EC_KEY object
EC_KEY* create_ec_key_from_compressed_public_key(const unsigned char* compressed_pub_key) {
  EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
  if (!key) {
    std::cerr << "Failed to create EC_KEY object" << std::endl;
    return nullptr;
  }

  EC_POINT* pub_point = EC_POINT_new(EC_KEY_get0_group(key));
  if (!EC_POINT_oct2point(EC_KEY_get0_group(key), pub_point, compressed_pub_key, 33, nullptr)) {
    std::cerr << "Failed to convert compressed public key" << std::endl;
    EC_POINT_free(pub_point);
    EC_KEY_free(key);
    return nullptr;
  }

  if (!EC_KEY_set_public_key(key, pub_point)) {
    std::cerr << "Failed to set public key" << std::endl;
    EC_POINT_free(pub_point);
    EC_KEY_free(key);
    return nullptr;
  }

  EC_POINT_free(pub_point);
  return key;
}

// Function to verify Ethereum-compatible signature
bool verify_ethereum_signature(const crypto::hash& m, const crypto::eth_signature& sig_res, const crypto::eth_public_key& compressed_pub_key) {
  EC_KEY* ec_key = create_ec_key_from_compressed_public_key((const unsigned char*)&compressed_pub_key.data[0]);
  if (!ec_key) {
    throw std::runtime_error("Failed to create EC key from compressed public key");
  }
  const unsigned char* r = (unsigned char*)&sig_res.data[0];
  const unsigned char* s = (unsigned char*)&sig_res.data[32];
  const unsigned char* hash = (unsigned char*)&m;

  // Create ECDSA_SIG from r and s
  BIGNUM* bn_r = BN_bin2bn(r, 32, nullptr);
  BIGNUM* bn_s = BN_bin2bn(s, 32, nullptr);
  if (!bn_r || !bn_s) {
    EC_KEY_free(ec_key);
    BN_free(bn_r);
    BN_free(bn_s);
    throw std::runtime_error("Failed to convert r or s to BIGNUM");
  }

  ECDSA_SIG* sig = ECDSA_SIG_new();
  if (!sig) {
    EC_KEY_free(ec_key);
    BN_free(bn_r);
    BN_free(bn_s);
    throw std::runtime_error("Failed to create ECDSA_SIG object");
  }

  if (!ECDSA_SIG_set0(sig, bn_r, bn_s)) {
    EC_KEY_free(ec_key);
    ECDSA_SIG_free(sig);
    BN_free(bn_r);
    BN_free(bn_s);
    throw std::runtime_error("Failed to set r and s in ECDSA_SIG");
  }

  // Verify the signature
  int verification_result = ECDSA_do_verify(hash, 32, sig, ec_key);

  ECDSA_SIG_free(sig);
  EC_KEY_free(ec_key);
  //BN_free(bn_r);
  //BN_free(bn_s);

  return verification_result == 1;
}


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
#ifndef USE_OPEN_SSL_FOR_ETH___
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
#else 
      return generate_ethereum_signature((const unsigned char*)&m.data, (unsigned char*)&sec_key.data, sig);
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
      // TODO (performance) consider using secp256k1_context_static for verification -- sowle
#ifndef USE_OPEN_SSL_FOR_ETH
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
#else
      return verify_ethereum_signature(m, sig, pub_key);
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
