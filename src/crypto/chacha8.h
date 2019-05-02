// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdint.h>
#include <stddef.h>

#define CHACHA8_KEY_SIZE 32
#define CHACHA8_IV_SIZE 8

#if defined(__cplusplus)
#include <memory.h>

#include "hash.h"

namespace crypto {
  extern "C" {
#endif
    void chacha8(const void* data, size_t length, const uint8_t* key, const uint8_t* iv, char* cipher);
#if defined(__cplusplus)
  }

#pragma pack(push, 1)
  struct chacha8_key {
    uint8_t data[CHACHA8_KEY_SIZE];

    ~chacha8_key()
    {
      memset(data, 0, sizeof(data));
    }
  };

  // MS VC 2012 doesn't interpret `class chacha8_iv` as POD in spite of [9.0.10], so it is a struct
  struct chacha8_iv {
    uint8_t data[CHACHA8_IV_SIZE];
  };
#pragma pack(pop)

  static_assert(sizeof(chacha8_key) == CHACHA8_KEY_SIZE && sizeof(chacha8_iv) == CHACHA8_IV_SIZE, "Invalid structure size");

  inline void chacha8(const void* data, std::size_t length, const chacha8_key& key, const chacha8_iv& iv, char* cipher) {
    chacha8(data, length, reinterpret_cast<const uint8_t*>(&key), reinterpret_cast<const uint8_t*>(&iv), cipher);
  }

  inline void generate_chacha8_key(const void* pass, size_t sz, chacha8_key& key) {
    static_assert(sizeof(chacha8_key) <= sizeof(hash), "Size of hash must be at least that of chacha8_key");
    char pwd_hash[HASH_SIZE];
    //TODO: change wallet encryption algo
    crypto::cn_fast_hash(pass, sz, pwd_hash);
    memcpy(&key.data, pwd_hash, sizeof(key.data));
    memset(pwd_hash, 0, sizeof(pwd_hash));
  }

  inline void generate_chacha8_key(std::string password, chacha8_key& key) 
  {
    generate_chacha8_key(password.data(), password.size(), key);
  }


  inline bool chacha_crypt(const void* src_buff, size_t sz, void* target_buff, const void* key_buff, size_t key_buff_size)
  {
    crypto::chacha8_key key;
    crypto::chacha8_iv iv;
    memset(&iv, 0, sizeof(iv));
    crypto::generate_chacha8_key(key_buff, key_buff_size, key);
    crypto::chacha8(src_buff, sz, key, iv, (char*)target_buff);
    return true;
  }

  inline bool chacha_crypt(std::string& buff, const std::string& pass)
  {

    std::string decrypted_buff;
    decrypted_buff.resize(buff.size());
    chacha_crypt(buff.data(), buff.size(), (void*)decrypted_buff.data(), pass.data(), pass.size());
    buff = decrypted_buff;
    return true;
  }

  template<typename pod_to_encrypt, typename pod_pass>
  inline bool chacha_crypt(pod_to_encrypt& crypt, const pod_pass& pass)
  {
    std::string buff;
    buff.resize(sizeof(pod_to_encrypt));
    chacha_crypt(&crypt, sizeof(crypt), (void*)buff.data(), &pass, sizeof(pass));
    memcpy(&crypt, buff.data(), sizeof(crypt));
    return true;
  }

  template<typename pod_pass>
  inline bool chacha_crypt(std::string& buff, const pod_pass& pass)
  {
    std::string buff_target;
    buff_target.resize(buff.size());
    chacha_crypt(buff.data(), buff.size(), (void*)buff_target.data(), &pass, sizeof(pass));
    buff = buff_target;
    return true;
  }

  template<typename pod_pass>
  inline std::string chacha_crypt(const std::string& input, const pod_pass& pass)
  {
    std::string result;
    result.resize(input.size());
    if (chacha_crypt(input.data(), input.size(), (void*)result.data(), &pass, sizeof pass))
      return result;
    return "";
  }

}

#endif
