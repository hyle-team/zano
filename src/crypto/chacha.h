// Copyright (c) 2018-2025 Zano Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdint.h>
#include <stddef.h>

#define CHACHA_KEY_SIZE 32
#define CHACHA_IV_SIZE 8

#if defined(__cplusplus)
#include <memory.h>
namespace crypto
{
  // all C function share the same space of names regardless of 'namespace', but here we put it into crypto just for convenience -- sowle
  extern "C"
  {
    #include "keccak.h"
  }
}
#include "hash.h"
#include "string_tools.h"

namespace crypto
{
  extern "C"
  {
#endif
    void chacha(int rounds_count, const void* data, size_t length, const uint8_t* key, const uint8_t* iv, char* cipher);
#if defined(__cplusplus)
  }

#pragma pack(push, 1)
  struct chacha_key
  {
    uint8_t data[CHACHA_KEY_SIZE];

    ~chacha_key()
    {
      memset(data, 0, sizeof(data));
    }
  };

  union chacha_iv
  {
    uint8_t  data[CHACHA_IV_SIZE];
    uint64_t u64;

    inline chacha_iv& operator++()
    {
      ++u64;
      return *this;
    }

    static_assert(sizeof data == sizeof u64);
  };
#pragma pack(pop)

  static_assert(sizeof(chacha_key) == CHACHA_KEY_SIZE && sizeof(chacha_iv) == CHACHA_IV_SIZE, "Invalid structure size");
  static_assert(sizeof(chacha_key) == sizeof(hash), "crypto::hash and crypto::chacha_key size missmatch");

  inline void chacha8(const void* data, std::size_t length, const chacha_key& key, const chacha_iv& iv, void* cipher)
  {
    chacha(8, data, length, reinterpret_cast<const uint8_t*>(&key), reinterpret_cast<const uint8_t*>(&iv), reinterpret_cast<char*>(cipher));
  }

  inline void chacha20(const void* data, std::size_t length, const chacha_key& key, const chacha_iv& iv, void* cipher)
  {
    chacha(20, data, length, reinterpret_cast<const uint8_t*>(&key), reinterpret_cast<const uint8_t*>(&iv), reinterpret_cast<char*>(cipher));
  }

  inline void chacha_generate_key_and_iv(const char(&hdss)[32], const void* p_data, size_t data_size, uint64_t index, chacha_key& key, chacha_iv& iv)
  {
    std::string buff;
    epee::string_tools::append_pod_to_strbuff(hdss, buff);
    buff.append(reinterpret_cast<const char*>(p_data), data_size);
    epee::string_tools::append_pod_to_strbuff(index, buff);

    struct {
      chacha_key key;
      chacha_iv iv;
    } hash_result;

    keccak((uint8_t*)buff.data(), (int)buff.size(), (uint8_t*)&hash_result, (int)(sizeof hash_result));

    key = hash_result.key;
    iv = hash_result.iv;
  }

  inline std::string chacha20(const chacha_key& key, const chacha_iv& iv, const std::string& source_str)
  {
    std::string result(source_str.size(), '\0');
    chacha20(source_str.data(), source_str.size(), key, iv, result.data());
    return result;
  }

  inline void chacha20_inplace(const chacha_key& key, const chacha_iv& iv, std::string& str)
  {
    chacha20(str.data(), str.size(), key, iv, str.data());
  }

  template<typename T>
  inline void chacha20_pod_inplace(const chacha_key& key, const chacha_iv& iv, T& pod_object)
  {
    static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable (POD)");
    chacha20(&pod_object, sizeof pod_object, key, iv, &pod_object);
  }


  //
  // legacy functions
  //

  inline void generate_chacha_key_legacy(const void* password_data, size_t password_data_size, chacha_key& key)
  {
    char pwd_hash[HASH_SIZE];
    //TODO: change wallet encryption algo
    crypto::cn_fast_hash(password_data, password_data_size, pwd_hash);
    static_assert(sizeof key.data == sizeof pwd_hash);
    memcpy(&key.data, pwd_hash, sizeof(key.data));
    memset(pwd_hash, 0, sizeof(pwd_hash));
  }

  inline void generate_chacha_key_legacy(std::string password, chacha_key& key) 
  {
    generate_chacha_key_legacy(password.data(), password.size(), key);
  }

  inline bool chacha_crypt_legacy(const void* src_buff, size_t sz, void* target_buff, const void* key_buff, size_t key_buff_size)
  {
    crypto::chacha_key key;
    crypto::chacha_iv iv;
    memset(&iv, 0, sizeof(iv));
    crypto::generate_chacha_key_legacy(key_buff, key_buff_size, key);
    crypto::chacha8(src_buff, sz, key, iv, (char*)target_buff);
    return true;
  }

  inline bool chacha_crypt_legacy(std::string& buff, const std::string& pass)
  {

    std::string decrypted_buff;
    decrypted_buff.resize(buff.size());
    chacha_crypt_legacy(buff.data(), buff.size(), (void*)decrypted_buff.data(), pass.data(), pass.size());
    buff = decrypted_buff;
    return true;
  }

  template<typename pod_to_encrypt, typename pod_pass>
  inline bool chacha_crypt_legacy(pod_to_encrypt& crypt, const pod_pass& pass)
  {
    std::string buff;
    buff.resize(sizeof(pod_to_encrypt));
    chacha_crypt_legacy(&crypt, sizeof(crypt), (void*)buff.data(), &pass, sizeof(pass));
    memcpy(&crypt, buff.data(), sizeof(crypt));
    return true;
  }

  template<typename pod_pass>
  inline bool chacha_crypt_legacy(std::string& buff, const pod_pass& pass)
  {
    std::string buff_target;
    buff_target.resize(buff.size());
    chacha_crypt_legacy(buff.data(), buff.size(), (void*)buff_target.data(), &pass, sizeof(pass));
    buff = buff_target;
    return true;
  }

  template<typename pod_pass>
  inline std::string chacha_crypt_legacy(const std::string& input, const pod_pass& pass)
  {
    std::string result;
    result.resize(input.size());
    if (chacha_crypt_legacy(input.data(), input.size(), (void*)result.data(), &pass, sizeof pass))
      return result;
    return "";
  }

} // namespace crypto

#endif // #if defined(__cplusplus)
