// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstddef>
#include <mutex>
#include <vector>
#include <string>
#include <algorithm>

#include "common/pod-class.h"
#include "generic-ops.h"
#include "hash.h"
#include "warnings.h"

#define CRYPTO_STR_(X) #X
#define CRYPTO_STR(X) CRYPTO_STR_(X)
#define CRYPTO_CHECK_AND_THROW_MES(cond, msg) if (!(cond)) { throw std::runtime_error(msg " @ " __FILE__ ":" CRYPTO_STR(__LINE__)); }

PUSH_GCC_WARNINGS
DISABLE_CLANG_WARNING(unused-private-field)


namespace crypto {

  extern "C" {
#include "random.h"
  }

  extern std::mutex random_lock;

#pragma pack(push, 1)
  POD_CLASS ec_point {
    char data[32];
  };

  POD_CLASS ec_scalar {
    char data[32];
  };

  POD_CLASS public_key: ec_point {
    friend class crypto_ops;
  };

  POD_CLASS secret_key: ec_scalar {
    friend class crypto_ops;
  };

  POD_CLASS key_derivation: ec_point {
    friend class crypto_ops;
  };

  POD_CLASS key_image: public ec_point {
    friend class crypto_ops;
  };

  POD_CLASS signature {
    ec_scalar c, r;
    friend class crypto_ops;
  };
#pragma pack(pop)

  static_assert(sizeof(ec_point) == 32 && sizeof(ec_scalar) == 32 &&
    sizeof(public_key) == 32 && sizeof(secret_key) == 32 &&
    sizeof(key_derivation) == 32 && sizeof(key_image) == 32 &&
    sizeof(signature) == 64, "Invalid structure size");

  class crypto_ops {
    crypto_ops();
    crypto_ops(const crypto_ops &);
    void operator=(const crypto_ops &);
    ~crypto_ops();

    static void generate_keys(public_key &, secret_key &);
    friend void generate_keys(public_key &, secret_key &);
    static void generate_seed_keys(public_key &pub, secret_key &sec, std::vector<unsigned char>& keys_seed_binary, size_t keys_seed_binary_size);
    friend void generate_seed_keys(public_key &pub, secret_key &sec, std::vector<unsigned char>& keys_seed_binary, size_t keys_seed_binary_size);
    static void keys_from_default(const unsigned char* a_part, public_key &pub, secret_key &sec, size_t keys_seed_binary_size);
    friend void keys_from_default(const unsigned char* a_part, public_key &pub, secret_key &sec, size_t keys_seed_binary_size);
    static void dependent_key(const secret_key& first, secret_key& second);
    friend void dependent_key(const secret_key& first, secret_key& second);
    static bool check_key(const public_key &);
    friend bool check_key(const public_key &);
    static bool secret_key_to_public_key(const secret_key &, public_key &);
    friend bool secret_key_to_public_key(const secret_key &, public_key &);
    static bool generate_key_derivation(const public_key &, const secret_key &, key_derivation &);
    friend bool generate_key_derivation(const public_key &, const secret_key &, key_derivation &);
    static void derivation_to_scalar(const key_derivation &, size_t, ec_scalar &);
    friend void derivation_to_scalar(const key_derivation &, size_t, ec_scalar &);
    static bool derive_public_key(const key_derivation &, std::size_t, const public_key &, public_key &);
    friend bool derive_public_key(const key_derivation &, std::size_t, const public_key &, public_key &);
    static void derive_secret_key(const key_derivation &, std::size_t, const secret_key &, secret_key &);
    friend void derive_secret_key(const key_derivation &, std::size_t, const secret_key &, secret_key &);
    static void generate_signature(const hash &, const public_key &, const secret_key &, signature &);
    friend void generate_signature(const hash &, const public_key &, const secret_key &, signature &);
    static bool check_signature(const hash &, const public_key &, const signature &);
    friend bool check_signature(const hash &, const public_key &, const signature &);
    static void generate_key_image(const public_key &, const secret_key &, key_image &);
    friend void generate_key_image(const public_key &, const secret_key &, key_image &);
    static void generate_ring_signature(const hash &, const key_image &,
      const public_key *const *, std::size_t, const secret_key &, std::size_t, signature *);
    friend void generate_ring_signature(const hash &, const key_image &,
      const public_key *const *, std::size_t, const secret_key &, std::size_t, signature *);
    static bool check_ring_signature(const hash &, const key_image &,
      const public_key *const *, std::size_t, const signature *);
    friend bool check_ring_signature(const hash &, const key_image &,
      const public_key *const *, std::size_t, const signature *);
    friend bool validate_key_image(const key_image& ki);
    static bool validate_key_image(const key_image& ki);

  };

  /* Generate a value filled with random bytes.
   */
  template<typename T>
  typename std::enable_if<std::is_pod<T>::value, T>::type rand() {
    typename std::remove_cv<T>::type res;
    std::lock_guard<std::mutex> lock(random_lock);
    generate_random_bytes(sizeof(T), &res);
    return res;
  }

  /* An adapter, to be used with std::shuffle, etc.
   */
  struct uniform_random_bit_generator
  {
    typedef uint64_t result_type;
    static CONSTEXPR uint64_t min() { return 0; }
    static CONSTEXPR uint64_t max() { return UINT64_MAX; }
    uint64_t operator()() { return rand<uint64_t>(); }
  };


  /* Generate a new key pair
   */
  inline void generate_keys(public_key &pub, secret_key &sec) {
    crypto_ops::generate_keys(pub, sec);
  }

  inline void generate_seed_keys(public_key &pub, secret_key &sec, std::vector<unsigned char>& keys_seed_binary, size_t keys_seed_binary_size)
  {
    crypto_ops::generate_seed_keys(pub, sec, keys_seed_binary, keys_seed_binary_size);
  }

  inline void keys_from_default(const unsigned char* a_part, public_key &pub, secret_key &sec, size_t keys_seed_binary_size)
  {
    crypto_ops::keys_from_default(a_part, pub, sec, keys_seed_binary_size);
  }

  inline void dependent_key(const secret_key& first, secret_key& second){
    return crypto_ops::dependent_key(first, second);
  }

  /* Check a public key. Returns true if it is valid, false otherwise.
   */
  inline bool check_key(const public_key &key) {
    return crypto_ops::check_key(key);
  }

  inline bool validate_key_image(const key_image& ki){
    return crypto_ops::validate_key_image(ki);
  }
  /* Checks a private key and computes the corresponding public key.
   */
  inline bool secret_key_to_public_key(const secret_key &sec, public_key &pub) {
    return crypto_ops::secret_key_to_public_key(sec, pub);
  }

  /* To generate an ephemeral key used to send money to:
   * * The sender generates a new key pair, which becomes the transaction key. The public transaction key is included in "extra" field.
   * * Both the sender and the receiver generate key derivation from the transaction key, the receivers' "view" key and the output index.
   * * The sender uses key derivation and the receivers' "spend" key to derive an ephemeral public key.
   * * The receiver can either derive the public key (to check that the transaction is addressed to him) or the private key (to spend the money).
   */
  inline bool generate_key_derivation(const public_key &key1, const secret_key &key2, key_derivation &derivation) {
    return crypto_ops::generate_key_derivation(key1, key2, derivation);
  }
  inline void derivation_to_scalar(const key_derivation &derivation, size_t output_index, ec_scalar &result) {
    crypto::crypto_ops::derivation_to_scalar(derivation, output_index, result);
  }

  inline bool derive_public_key(const key_derivation &derivation, std::size_t output_index,
    const public_key &base, public_key &derived_key) {
    return crypto_ops::derive_public_key(derivation, output_index, base, derived_key);
  }
  inline void derive_secret_key(const key_derivation &derivation, std::size_t output_index,
    const secret_key &base, secret_key &derived_key) {
    crypto_ops::derive_secret_key(derivation, output_index, base, derived_key);
  }

  /* Generation and checking of a standard signature.
   */
  inline void generate_signature(const hash &prefix_hash, const public_key &pub, const secret_key &sec, signature &sig) {
    crypto_ops::generate_signature(prefix_hash, pub, sec, sig);
  }
  inline bool check_signature(const hash &prefix_hash, const public_key &pub, const signature &sig) {
    return crypto_ops::check_signature(prefix_hash, pub, sig);
  }

  /* To send money to a key:
   * * The sender generates an ephemeral key and includes it in transaction output.
   * * To spend the money, the receiver generates a key image from it.
   * * Then he selects a bunch of outputs, including the one he spends, and uses them to generate a ring signature.
   * To check the signature, it is necessary to collect all the keys that were used to generate it. To detect double spends, it is necessary to check that each key image is used at most once.
   */
  inline void generate_key_image(const public_key &pub, const secret_key &sec, key_image &image) {
    crypto_ops::generate_key_image(pub, sec, image);
  }
  inline void generate_ring_signature(const hash &prefix_hash, const key_image &image,
    const public_key *const *pubs, std::size_t pubs_count,
    const secret_key &sec, std::size_t sec_index,
    signature *sig) {
    crypto_ops::generate_ring_signature(prefix_hash, image, pubs, pubs_count, sec, sec_index, sig);
  }
  inline bool check_ring_signature(const hash &prefix_hash, const key_image &image,
    const public_key *const *pubs, std::size_t pubs_count,
    const signature *sig) {
    return crypto_ops::check_ring_signature(prefix_hash, image, pubs, pubs_count, sig);
  }

  /* Variants with vector<const public_key *> parameters.
   */
  inline void generate_ring_signature(const hash &prefix_hash, const key_image &image,
    const std::vector<const public_key *> &pubs,
    const secret_key &sec, std::size_t sec_index,
    signature *sig) {
    generate_ring_signature(prefix_hash, image, pubs.data(), pubs.size(), sec, sec_index, sig);
  }
  inline bool check_ring_signature(const hash &prefix_hash, const key_image &image,
    const std::vector<const public_key *> &pubs,
    const signature *sig) {
    return check_ring_signature(prefix_hash, image, pubs.data(), pubs.size(), sig);
  }

  class stream_cn_hash
  {
  public:
    static constexpr size_t DATA_BLOCK_SIZE = 1024 * 1024;

    stream_cn_hash()
      : m_buffer(HASH_SIZE + DATA_BLOCK_SIZE, '\0')
      , m_p_hash(const_cast<hash*>(reinterpret_cast<const hash*>(m_buffer.data())))
      , m_p_data(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(m_buffer.data())) + HASH_SIZE)
      , m_ready(false)
      , m_data_used(0)
    {
      m_ready = true;
    }

    bool update(const void* data, size_t size)
    {
      if (!m_ready)
        return false;

      const uint8_t* p_source_data = reinterpret_cast<const uint8_t*>(data);

      while(size > 0)
      {
        // fill the buffer up
        size_t bytes_to_copy = std::min(size, DATA_BLOCK_SIZE - m_data_used);
        memcpy(m_p_data + m_data_used, p_source_data, bytes_to_copy);
        m_data_used += bytes_to_copy;
        p_source_data += bytes_to_copy;
        size -= bytes_to_copy;

        if (m_data_used == DATA_BLOCK_SIZE)
        {
          // calc imtermediate hash of the whole buffer and put the result into the beginning of the buffer
          *m_p_hash = cn_fast_hash(m_buffer.data(), HASH_SIZE + m_data_used);
          // clear data buffer for new bytes
          memset(m_p_data, 0, DATA_BLOCK_SIZE);
          m_data_used = 0;
        }

        // repeat if there are source bytes left
      }

      return true;
    }

    hash calculate_hash()
    {
      if (m_data_used == 0)
        return *m_p_hash;

      m_ready = false;
      return cn_fast_hash(m_buffer.data(), HASH_SIZE + m_data_used);
    }
  
  private:
    const std::string m_buffer;
    hash* const     m_p_hash;
    uint8_t* const  m_p_data;
    size_t          m_data_used;
    bool            m_ready;
  }; // class stream_cn_hash

} // namespace crypto

POD_MAKE_HASHABLE(crypto, public_key)
POD_MAKE_COMPARABLE(crypto, secret_key)
POD_MAKE_HASHABLE(crypto, key_image)
POD_MAKE_COMPARABLE(crypto, signature)
POD_MAKE_COMPARABLE(crypto, key_derivation)
POD_MAKE_LESS_OPERATOR(crypto, hash)
POD_MAKE_LESS_OPERATOR(crypto, key_image)
POP_GCC_WARNINGS