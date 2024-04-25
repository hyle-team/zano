// Copyright (c) 2020-2024 Zano Project
// Copyright (c) 2020-2024 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Note: This file originates from tests/functional_tests/crypto_tests.cpp 
#pragma once
#include <string>
#include <boost/multiprecision/cpp_int.hpp>
#include "crypto.h"

namespace crypto
{
  namespace mp = boost::multiprecision;

  extern "C"
  {
#include "crypto/crypto-ops.h"
  } // extern "C"

  //
  // Helpers
  //

  // returns greatest k, s.t. n**k <= v
  // tests in crypto_tests_range_proofs.h
  constexpr uint64_t constexpr_floor_log_n(uint64_t v, uint64_t n)
  {
    return (v < n || n <= 1) ? 0 : constexpr_floor_log_n(v / n, n) + 1;
  }

  // returns smallest k, s.t. v <= n**k
  // tests in crypto_tests_range_proofs.h
  constexpr uint64_t constexpr_ceil_log_n(uint64_t v, uint64_t n)
  {
    return (v <= 1 || n <= 1) ? 0 : constexpr_floor_log_n(v - 1, n) + 1;
  }

  // returns smallest k, s.t. v <= 2**k
  // tests in crypto_tests_range_proofs.h
  constexpr uint64_t constexpr_ceil_log2(uint64_t v)
  {
    return constexpr_ceil_log_n(v, 2);
  }

  // returns base ** k
  constexpr uint64_t constexpr_pow(uint64_t k, uint64_t base)
  {
    return k == 0 ? 1 : base * constexpr_pow(k - 1, base);
  }


  template<class pod_t>
  std::string pod_to_hex_reversed(const pod_t &h)
  {
    constexpr char hexmap[] = "0123456789abcdef";
    const unsigned char* data = reinterpret_cast<const unsigned char*>(&h);
    size_t len = sizeof h;

    std::string s(len * 2, ' ');
    for (size_t i = 0; i < len; ++i)
    {
      s[2 * i] = hexmap[data[len - 1 - i] >> 4];
      s[2 * i + 1] = hexmap[data[len - 1 - i] & 0x0F];
    }

    return s;
  }

  template<class pod_t>
  std::string pod_to_hex(const pod_t &h)
  {
    constexpr char hexmap[] = "0123456789abcdef";
    const unsigned char* data = reinterpret_cast<const unsigned char*>(&h);
    size_t len = sizeof h;

    std::string s(len * 2, ' ');
    for (size_t i = 0; i < len; ++i)
    {
      s[2 * i] = hexmap[data[i] >> 4];
      s[2 * i + 1] = hexmap[data[i] & 0x0F];
    }

    return s;
  }

  template<class pod_t>
  std::string pod_to_hex_comma_separated_bytes(const pod_t &h)
  {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    size_t len = sizeof h;
    const unsigned char* p = (const unsigned char*)&h;
    for (size_t i = 0; i < len; ++i)
    {
      ss << "0x" << std::setw(2) << static_cast<unsigned int>(p[i]);
      if (i + 1 != len)
        ss << ", ";
    }
    return ss.str();
  }

  template<class pod_t>
  std::string pod_to_comma_separated_chars(const pod_t &h)
  {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    size_t len = sizeof h;
    const unsigned char* p = (const unsigned char*)&h;
    for (size_t i = 0; i < len; ++i)
    {
      ss << "'\\x" << std::setw(2) << static_cast<unsigned int>(p[i]) << "'";
      if (i + 1 != len)
        ss << ", ";
    }
    return ss.str();
  }

  template<class pod_t>
  std::string pod_to_hex_comma_separated_uint64(const pod_t &h)
  {
    static_assert((sizeof h) % 8 == 0, "size of h should be a multiple of 64 bit");
    size_t len = (sizeof h) / 8;
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    const uint64_t* p = (const uint64_t*)&h;
    for (size_t i = 0; i < len; ++i)
    {
      ss << "0x" << std::setw(16) << static_cast<uint64_t>(p[i]);
      if (i + 1 != len)
        ss << ", ";
    }
    return ss.str();
  }

  template<class pod_t>
  std::string pod_to_comma_separated_int32(const pod_t &h)
  {
    static_assert((sizeof h) % 4 == 0, "size of h should be a multiple of 32 bit");
    size_t len = (sizeof h) / 4;
    std::stringstream ss;
    const int32_t* p = (const int32_t*)&h;
    for (size_t i = 0; i < len; ++i)
    {
      ss << static_cast<int32_t>(p[i]);
      if (i + 1 != len)
        ss << ", ";
    }
    return ss.str();
  }

  template<typename t_pod_type>
  bool parse_tpod_from_hex_string(const std::string& hex_str, t_pod_type& t_pod)
  {
    static const int16_t char_map[256] = { // 0-9, a-f, A-F is only allowed
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   // 0x00 - 0x1F
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,   // 0x20 - 0x3F
      -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   // 0x40 - 0x5F
      -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   // 0x60 - 0x7F
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   // 0x80 - 0x9F
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   // 0xA0 - 0xBF
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   // 0xC0 - 0xDF
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }; // 0xE0 - 0xFF

    size_t pod_size = sizeof t_pod;
    uint8_t *p = reinterpret_cast<uint8_t*>(&t_pod);

    if (hex_str.size() != 2 * pod_size)
      return false;

    for (size_t i = 0; i < pod_size; ++i)
    {
      int16_t hi = char_map[static_cast<uint8_t>(hex_str[2 * i])];
      int16_t lo = char_map[static_cast<uint8_t>(hex_str[2 * i + 1])];
      if (hi < 0 || lo < 0)
      {
        // invalid characters in hex_str
        memset(p, 0, pod_size);
        return false;
      }
      p[i] = static_cast<uint8_t>(hi * 16 + lo); // write byte to pod
    }
    return true;
  }

  template<typename t_pod_type>
  t_pod_type parse_tpod_from_hex_string(const std::string& hex_str)
  {
    t_pod_type t_pod = AUTO_VAL_INIT(t_pod);
    crypto::parse_tpod_from_hex_string(hex_str, t_pod); // using fully qualified name to avoid Argument-Dependent Lookup issues
    return t_pod;
  }

  //
  // scalar_t - holds a 256-bit scalar, normally in [0..L-1]
  //
  struct /* TODO alignas(32) */ scalar_t
  {
    union
    {
      uint64_t            m_u64[4];
      unsigned char       m_s[32];
      crypto::secret_key  m_sk;
    };

    scalar_t() = default;

    // won't check scalar range validity (< L)
    constexpr scalar_t(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) noexcept
      : m_u64{a0, a1, a2, a3}
    {
    }

    // won't check scalar range validity (< L)
    scalar_t(const unsigned char(&v)[32])
    {
      memcpy(m_s, v, 32);
    }

    // won't check secret key validity (sk < L)
    scalar_t(const crypto::secret_key& sk)
    {
      from_secret_key(sk);
    }

    // copy data and reduce
    scalar_t(const crypto::hash& hash)
    {
      m_u64[0] = ((uint64_t*)&hash)[0];
      m_u64[1] = ((uint64_t*)&hash)[1];
      m_u64[2] = ((uint64_t*)&hash)[2];
      m_u64[3] = ((uint64_t*)&hash)[3];
      sc_reduce32(&m_s[0]);
    }

    scalar_t(uint64_t v)
    {
      zero();
      m_u64[0] = v;
      // do not need to call reduce as 2^64 < L
    }

    // copy at most 256 bits (32 bytes) and reduce
    template<typename T>
    explicit scalar_t(const boost::multiprecision::number<T>& bigint)
    {
      zero();
      size_t bytes_to_copy = bigint.backend().size() * bigint.backend().limb_bits / 8;
      if (bytes_to_copy > sizeof *this)
        bytes_to_copy = sizeof *this;
      memcpy(&m_s[0], bigint.backend().limbs(), bytes_to_copy);
      sc_reduce32(&m_s[0]);
    }

    unsigned char* data()
    {
      return &m_s[0];
    }

    const unsigned char* data() const
    {
      return &m_s[0];
    }

    crypto::secret_key &as_secret_key()
    {
      return m_sk;
    }

    const crypto::secret_key& as_secret_key() const
    {
      return m_sk;
    }

    operator crypto::secret_key() const
    {
      return m_sk;
    }

    void from_secret_key(const crypto::secret_key& sk)
    {
      m_sk = sk;
      // assuming secret key is correct (< L), so we don't need to call reduce here
    }

    void zero()
    {
      m_u64[0] = 0;
      m_u64[1] = 0;
      m_u64[2] = 0;
      m_u64[3] = 0;
    }

    // genrate 0 <= x < L
    static scalar_t random()
    {
      scalar_t result;
      result.make_random();
      return result;
    }

    // generate 0 <= x < L
    void make_random()
    {
      unsigned char tmp[64];
      crypto::generate_random_bytes(64, tmp);
      sc_reduce(tmp);
      memcpy(&m_s, tmp, sizeof m_s);

      /*  // for tests
      int x[8] = { rand() };
      crypto::cn_fast_hash(&x, sizeof x, *(crypto::hash*)this);
      sc_reduce32(m_s);
      */
    }

    bool is_zero() const
    {
      return sc_isnonzero(&m_s[0]) == 0;
    }

    bool is_reduced() const
    {
      return sc_check(&m_s[0]) == 0;
    }

    void reduce()
    {
      sc_reduce32(&m_s[0]);
    }

    scalar_t operator+(const scalar_t& v) const
    {
      scalar_t result;
      sc_add(&result.m_s[0], &m_s[0], &v.m_s[0]);
      return result;
    }

    scalar_t& operator+=(const scalar_t& v)
    {
      sc_add(&m_s[0], &m_s[0], &v.m_s[0]);
      return *this;
    }

    scalar_t operator-(const scalar_t& v) const
    {
      scalar_t result;
      sc_sub(&result.m_s[0], &m_s[0], &v.m_s[0]);
      return result;
    }

    scalar_t& operator-=(const scalar_t& v)
    {
      sc_sub(&m_s[0], &m_s[0], &v.m_s[0]);
      return *this;
    }

    scalar_t operator*(const scalar_t& v) const
    {
      scalar_t result;
      sc_mul(result.m_s, m_s, v.m_s);
      return result;
    }

    scalar_t& operator*=(const scalar_t& v)
    {
      sc_mul(m_s, m_s, v.m_s);
      return *this;
    }

    scalar_t operator-() const
    {
      static unsigned char zero[32] = { 0 };
      scalar_t result;
      sc_sub(&result.m_s[0], zero, &m_s[0]);
      return result;
    }

    // returns this = a * b
    scalar_t& assign_mul(const scalar_t& a, const scalar_t& b)
    {
      sc_mul(m_s, a.m_s, b.m_s);
      return *this;
    }

    /*
    I think it has bad symantic (operator-like), consider rename/reimplement -- sowle
    */
    // returns c + this * b
    scalar_t muladd(const scalar_t& b, const scalar_t& c) const
    {
      scalar_t result;
      sc_muladd(result.m_s, m_s, b.m_s, c.m_s);
      return result;
    }

    // returns this = c + a * b
    scalar_t& assign_muladd(const scalar_t& a, const scalar_t& b, const scalar_t& c)
    {
      sc_muladd(m_s, a.m_s, b.m_s, c.m_s);
      return *this;
    }

    // returns this = c - a * b
    scalar_t& assign_mulsub(const scalar_t& a, const scalar_t& b, const scalar_t& c)
    {
      sc_mulsub(m_s, a.m_s, b.m_s, c.m_s);
      return *this;
    }

    scalar_t reciprocal() const
    {
      scalar_t result;
      sc_invert(result.m_s, m_s);
      return result;
    }

    scalar_t operator/(const scalar_t& v) const
    {
      return operator*(v.reciprocal());
    }

    scalar_t& operator/=(const scalar_t& v)
    {
      scalar_t reciprocal;
      sc_invert(&reciprocal.m_s[0], &v.m_s[0]);
      sc_mul(&m_s[0], &m_s[0], &reciprocal.m_s[0]);
      return *this;
    }

    bool operator==(const scalar_t& rhs) const
    {
      return
        m_u64[0] == rhs.m_u64[0] &&
        m_u64[1] == rhs.m_u64[1] &&
        m_u64[2] == rhs.m_u64[2] &&
        m_u64[3] == rhs.m_u64[3];
    }

    bool operator!=(const scalar_t& rhs) const
    {
      return
        m_u64[0] != rhs.m_u64[0] ||
        m_u64[1] != rhs.m_u64[1] ||
        m_u64[2] != rhs.m_u64[2] ||
        m_u64[3] != rhs.m_u64[3];
    }

    bool operator<(const scalar_t& rhs) const
    {
      if (m_u64[3] < rhs.m_u64[3]) return true;
      if (m_u64[3] > rhs.m_u64[3]) return false;
      if (m_u64[2] < rhs.m_u64[2]) return true;
      if (m_u64[2] > rhs.m_u64[2]) return false;
      if (m_u64[1] < rhs.m_u64[1]) return true;
      if (m_u64[1] > rhs.m_u64[1]) return false;
      if (m_u64[0] < rhs.m_u64[0]) return true;
      if (m_u64[0] > rhs.m_u64[0]) return false;
      return false;
    }

    bool operator>(const scalar_t& rhs) const
    {
      if (m_u64[3] < rhs.m_u64[3]) return false;
      if (m_u64[3] > rhs.m_u64[3]) return true;
      if (m_u64[2] < rhs.m_u64[2]) return false;
      if (m_u64[2] > rhs.m_u64[2]) return true;
      if (m_u64[1] < rhs.m_u64[1]) return false;
      if (m_u64[1] > rhs.m_u64[1]) return true;
      if (m_u64[0] < rhs.m_u64[0]) return false;
      if (m_u64[0] > rhs.m_u64[0]) return true;
      return false;
    }

    friend std::ostream& operator<<(std::ostream& ss, const scalar_t &v)
    {
      return ss << pod_to_hex(v);
    }

    std::string to_string_as_hex_number() const
    {
      return pod_to_hex_reversed(*this);
    }

    std::string to_string_as_secret_key() const
    {
      return pod_to_hex(*this);
    }

    template<typename MP_type>
    MP_type as_boost_mp_type() const
    {
      MP_type result = 0;
      static_assert(sizeof result >= sizeof *this, "size missmatch"); // to avoid using types less than uint256_t
      unsigned int sz = sizeof *this / sizeof(boost::multiprecision::limb_type);
      result.backend().resize(sz, sz);
      memcpy(result.backend().limbs(), &m_s[0], sizeof *this);
      result.backend().normalize();
      return result;
    }

    // Little-endian assumed; TODO: consider Big-endian support
    bool get_bit(uint8_t bit_index) const
    {
      return (m_u64[bit_index >> 6] & (1ull << (bit_index & 63))) != 0;
    }

    // Little-endian assumed; TODO: consider Big-endian support
    void set_bit(size_t bit_index)
    {
      m_u64[bit_index >> 6] |= (1ull << (bit_index & 63));
    }

    // Little-endian assumed; TODO: consider Big-endian support
    void clear_bit(size_t bit_index)
    {
      m_u64[bit_index >> 6] &= ~(1ull << (bit_index & 63));
    }

    // the result is guaranteed to be within [ 0; 2 ^ bits_count )
    uint64_t get_bits(uint8_t bit_index_first, uint8_t bits_count) const
    {
      if (bits_count == 0 || bits_count > 64)
        return 0;
      uint8_t bits_count_m_1 = bits_count - 1;
      unsigned int bit_index_last = bit_index_first + bits_count_m_1;
      if (bit_index_last > 255)
        bit_index_last = 255;

      uint64_t result = m_u64[bit_index_first >> 6] >> (bit_index_first & 63);
      if (bits_count_m_1 > (bit_index_last & 63))
        result |= m_u64[bit_index_last >> 6] << (bits_count_m_1 - (bit_index_last & 63));

      uint64_t result_mask = ((1ull << bits_count_m_1) - 1) << 1 | 1;  // (just because 1ull << 64 in undefined behaviour, not a 0 as one would expect)
      return result & result_mask;
    }

    // does not reduce
    static scalar_t power_of_2(uint8_t exponent)
    {
      scalar_t result = 0;
      result.set_bit(exponent);
      return result;
    }

  }; // struct scalar_t

  //
  // Global constants (checked in crypto_constants)
  //

  static constexpr scalar_t c_scalar_0       = { 0,                  0,                  0,                  0                  };
  static constexpr scalar_t c_scalar_1       = { 1,                  0,                  0,                  0                  };
  static constexpr scalar_t c_scalar_2p64    = { 0,                  1,                  0,                  0                  };
  static constexpr scalar_t c_scalar_L       = { 0x5812631a5cf5d3ed, 0x14def9dea2f79cd6, 0x0,                0x1000000000000000 };
  static constexpr scalar_t c_scalar_Lm1     = { 0x5812631a5cf5d3ec, 0x14def9dea2f79cd6, 0x0,                0x1000000000000000 };
  static constexpr scalar_t c_scalar_P       = { 0xffffffffffffffed, 0xffffffffffffffff, 0xffffffffffffffff, 0x7fffffffffffffff };
  static constexpr scalar_t c_scalar_Pm1     = { 0xffffffffffffffec, 0xffffffffffffffff, 0xffffffffffffffff, 0x7fffffffffffffff };
  static constexpr scalar_t c_scalar_256m1   = { 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff };
  static constexpr scalar_t c_scalar_1div8   = { 0x6106e529e2dc2f79, 0x07d39db37d1cdad0, 0x0,                0x0600000000000000 };

  static_assert(sizeof(scalar_t::m_sk) == sizeof(scalar_t::m_u64) && sizeof(scalar_t::m_u64) == sizeof(scalar_t::m_s), "size missmatch");

  //
  //
  //
  struct point_t
  {
    struct tag_zero {};

    // A point(x, y) is represented in extended homogeneous coordinates (X, Y, Z, T)
    // with x = X / Z, y = Y / Z, x * y = T / Z.
    ge_p3 m_p3;

    point_t() = default;

    explicit point_t(const crypto::public_key& pk) // can throw std::runtime_error
    {
      CRYPTO_CHECK_AND_THROW_MES(from_public_key(pk), "invalid public key");
    }

    point_t(const unsigned char(&v)[32]) // can throw std::runtime_error
    {
      static_assert(sizeof(crypto::public_key) == sizeof v, "size missmatch");
      CRYPTO_CHECK_AND_THROW_MES(from_public_key(*(const crypto::public_key*)v), "invalid public key (char[32])");
    }

    point_t(const uint64_t(&v)[4]) // can throw std::runtime_error
    {
      static_assert(sizeof(crypto::public_key) == sizeof v, "size missmatch");
      CRYPTO_CHECK_AND_THROW_MES(from_public_key(*(const crypto::public_key*)v), "invalid public key (uint64_t[4])");
    }

    point_t(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) // can throw std::runtime_error
    {
      crypto::public_key pk;
      ((uint64_t*)&pk)[0] = a0;
      ((uint64_t*)&pk)[1] = a1;
      ((uint64_t*)&pk)[2] = a2;
      ((uint64_t*)&pk)[3] = a3;

      CRYPTO_CHECK_AND_THROW_MES(from_public_key(pk), "invalid public key (four uint64_t)");
    }

    explicit point_t(tag_zero&&)
    {
      zero();
    }

    explicit point_t(const key_image& ki) // can throw std::runtime_error
      : point_t(static_cast<const public_key&>(static_cast<const ec_point&>(ki)))
    {
    }

    explicit constexpr point_t(const int32_t(&v)[40]) noexcept
      : m_p3{
        {v[ 0], v[ 1], v[ 2], v[ 3], v[ 4], v[ 5], v[ 6], v[ 7], v[ 8], v[9]},
        {v[10], v[11], v[12], v[13], v[14], v[15], v[16], v[17], v[18], v[19]},
        {v[20], v[21], v[22], v[23], v[24], v[25], v[26], v[27], v[28], v[29]},
        {v[30], v[31], v[32], v[33], v[34], v[35], v[36], v[37], v[38], v[39]}
      }
    {
    }

    // as we're using additive notation, zero means identity group element (EC point (0, 1)) here and after
    void zero()
    {
      ge_p3_0(&m_p3);
    }

    bool is_zero() const
    {
      // (0, 1) ~ (0, z, z, 0) for any non-zero z           https://www.rfc-editor.org/rfc/rfc8032#page-17
      if (fe_isnonzero(m_p3.X) != 0)
        return false; // x != 0
      if (fe_isnonzero(m_p3.Z) == 0)
        return false; // z == 0
      if (fe_isnonzero(m_p3.T) != 0)
        return false; // t != 0
      fe y_minus_z;
      fe_sub(y_minus_z, m_p3.Y, m_p3.Z);
      return fe_isnonzero(y_minus_z) == 0;
    }

    bool is_in_main_subgroup() const
    {
      return (c_scalar_L * *this).is_zero();
    }

    bool from_public_key(const crypto::public_key& pk)
    {
      return ge_frombytes_vartime(&m_p3, reinterpret_cast<const unsigned char*>(&pk)) == 0;
    }

    bool from_key_image(const crypto::key_image& ki)
    {
      return ge_frombytes_vartime(&m_p3, reinterpret_cast<const unsigned char*>(&ki)) == 0;
    }

    bool from_string(const std::string& str)
    {
      crypto::public_key pk;
      if (!parse_tpod_from_hex_string(str, pk))
        return false;
      return from_public_key(pk);
    }

    crypto::public_key to_public_key() const
    {
      crypto::public_key result;
      ge_p3_tobytes((unsigned char*)&result, &m_p3);
      return result;
    }

    void to_public_key(crypto::public_key& result) const
    {
      ge_p3_tobytes((unsigned char*)&result, &m_p3);
    }

    crypto::key_image to_key_image() const
    {
      crypto::key_image result;
      ge_p3_tobytes((unsigned char*)&result, &m_p3);
      return result;
    }

    point_t operator+(const point_t& rhs) const
    {
      point_t result;
      ge_cached rhs_c;
      ge_p1p1 t;
      ge_p3_to_cached(&rhs_c, &rhs.m_p3);
      ge_add(&t, &m_p3, &rhs_c);
      ge_p1p1_to_p3(&result.m_p3, &t);
      return result;
    }

    point_t& operator+=(const point_t& rhs)
    {
      ge_cached rhs_c;
      ge_p1p1 t;
      ge_p3_to_cached(&rhs_c, &rhs.m_p3);
      ge_add(&t, &m_p3, &rhs_c);
      ge_p1p1_to_p3(&m_p3, &t);
      return *this;
    }

    point_t operator-(const point_t& rhs) const
    {
      point_t result;
      ge_cached rhs_c;
      ge_p1p1 t;
      ge_p3_to_cached(&rhs_c, &rhs.m_p3);
      ge_sub(&t, &m_p3, &rhs_c);
      ge_p1p1_to_p3(&result.m_p3, &t);
      return result;
    }

    point_t& operator-=(const point_t& rhs)
    {
      ge_cached rhs_c;
      ge_p1p1 t;
      ge_p3_to_cached(&rhs_c, &rhs.m_p3);
      ge_sub(&t, &m_p3, &rhs_c);
      ge_p1p1_to_p3(&m_p3, &t);
      return *this;
    }

    friend point_t operator*(const scalar_t& lhs, const point_t& rhs)
    {
      point_t result;
      //ge_scalarmult_p3(&result.m_p3, lhs.m_s, &rhs.m_p3);
      ge_scalarmult_vartime_p3(&result.m_p3, lhs.m_s, &rhs.m_p3);
      return result;
    }

    point_t& operator*=(const scalar_t& rhs)
    {
      //ge_scalarmult_p3(&m_p3, rhs.m_s, &m_p3);
      ge_scalarmult_vartime_p3(&m_p3, rhs.m_s, &m_p3);
      return *this;
    }

    friend point_t operator/(const point_t& lhs, const scalar_t& rhs)
    {
      point_t result;
      scalar_t reciprocal;
      sc_invert(&reciprocal.m_s[0], &rhs.m_s[0]);
      //ge_scalarmult_p3(&result.m_p3, &reciprocal.m_s[0], &lhs.m_p3);
      ge_scalarmult_vartime_p3(&result.m_p3, &reciprocal.m_s[0], &lhs.m_p3);
      return result;
    }

    point_t operator-() const
    {
      point_t result = *this;
      fe zero = {0};
      fe_sub(result.m_p3.Y, zero, result.m_p3.Y);
      fe_sub(result.m_p3.Z, zero, result.m_p3.Z);
      return result;
    }

    point_t& modify_mul8()
    {
      ge_mul8_p3(&m_p3, &m_p3);
      return *this;
    }

    point_t modify_mul_pow_2(size_t power)
    {
      if (power > 0)
      {
        ge_p1p1 p1;
        ge_p2 p2;
        ge_p3_to_p2(&p2, &m_p3);
        for (size_t i = 1; i < power; ++i)
        {
          ge_p2_dbl(&p1, &p2);
          ge_p1p1_to_p2(&p2, &p1);
        }
        ge_p2_dbl(&p1, &p2);
        ge_p1p1_to_p3(&m_p3, &p1);
      }
      return *this;
    }

    // returns a * this + G
    point_t mul_plus_G(const scalar_t& a) const
    {
      static const unsigned char one[32] = { 1 };
      static_assert(sizeof one == sizeof(crypto::ec_scalar), "size missmatch");

      point_t result;
      ge_double_scalarmult_base_vartime_p3(&result.m_p3, &a.m_s[0], &m_p3, &one[0]);
      return result;
    }

    // returns a * this + b * G
    point_t mul_plus_G(const scalar_t& a, const scalar_t& b) const
    {
      point_t result;
      ge_double_scalarmult_base_vartime_p3(&result.m_p3, &a.m_s[0], &m_p3, &b.m_s[0]);
      return result;
    }

    // *this = a * A + b * G
    void assign_mul_plus_G(const scalar_t& a, const point_t& A, const scalar_t& b)
    {
      ge_double_scalarmult_base_vartime_p3(&m_p3, &a.m_s[0], &A.m_p3, &b.m_s[0]);
    }

    friend bool operator==(const point_t& lhs, const point_t& rhs)
    {
      // TODO: @#@# (performance) consider checking (lhs - rhs).is_zero() instead

      // convert to xy form, then compare components (because (x, y, z, t) representation is not unique)
      fe lrecip, lx, ly;
      fe rrecip, rx, ry;

      fe_invert(lrecip, lhs.m_p3.Z);
      fe_invert(rrecip, rhs.m_p3.Z);

      fe_mul(lx, lhs.m_p3.X, lrecip);
      fe_mul(rx, rhs.m_p3.X, rrecip);
      if (memcmp(&lx, &rx, sizeof lx) != 0)
        return false;

      fe_mul(ly, lhs.m_p3.Y, lrecip);
      fe_mul(ry, rhs.m_p3.Y, rrecip);
      if (memcmp(&ly, &ry, sizeof ly) != 0)
        return false;

      return true;
    }

    friend bool operator!=(const point_t& lhs, const point_t& rhs)
    {
      return !(lhs == rhs);
    }

    friend bool operator==(const point_t& lhs, const public_key& rhs)
    {
      return lhs.to_public_key() == rhs;
    }

    friend bool operator!=(const point_t& lhs, const public_key& rhs)
    {
      return !(lhs == rhs);
    }

    friend bool operator==(const public_key& lhs, const point_t& rhs)
    {
      return lhs == rhs.to_public_key();
    }

    friend bool operator!=(const public_key& lhs, const point_t& rhs)
    {
      return !(lhs == rhs);
    }

    friend std::ostream& operator<<(std::ostream& ss, const point_t &v)
    {
      crypto::public_key pk = v.to_public_key();
      return ss << pod_to_hex(pk);
    }

    operator std::string() const
    {
      crypto::public_key pk = to_public_key();
      return pod_to_hex(pk);
    }

    std::string to_string() const
    {
      crypto::public_key pk = to_public_key();
      return pod_to_hex(pk);
    }

    std::string to_hex_comma_separated_bytes_str() const
    {
      crypto::public_key pk = to_public_key();
      return pod_to_hex_comma_separated_bytes(pk);
    }

    std::string to_hex_comma_separated_uint64_str() const
    {
      crypto::public_key pk = to_public_key();
      return pod_to_hex_comma_separated_uint64(pk);
    }

    std::string to_comma_separated_int32_str() const
    {
      return pod_to_comma_separated_int32(m_p3);
    }

  }; // struct point_t


  //
  // point_g_t -- special type for curve's base point
  //
  struct point_g_t : public point_t
  {
    explicit constexpr point_g_t(const int32_t(&v)[40]) noexcept
      : point_t(v)
    {
    }

    friend point_t operator*(const scalar_t& lhs, const point_g_t&)
    {
      point_t result;
      //ge_scalarmult_base(&result.m_p3, &lhs.m_s[0]);
      ge_scalarmult_base_vartime(&result.m_p3, &lhs.m_s[0]);
      return result;
    }

    friend point_t operator/(const point_g_t&, const scalar_t& rhs)
    {
      point_t result;
      scalar_t reciprocal;
      sc_invert(&reciprocal.m_s[0], &rhs.m_s[0]);
      //ge_scalarmult_base(&result.m_p3, &reciprocal.m_s[0]);
      ge_scalarmult_base_vartime(&result.m_p3, &reciprocal.m_s[0]);
      return result;
    }

    static_assert(sizeof(crypto::public_key) == 32, "size error");

  }; // struct point_g_t


  void construct_precomp_data(precomp_data_t precomp_data, const point_t& point);

  //
  // point_pc_t -- point with 30kB of precomputed data, which make possible to do very fast single scalar multiplication
  //
  struct point_pc_t : public point_t
  {
    constexpr point_pc_t(const int32_t(&v)[40], const precomp_data_t* precomp_data_p)
      : point_t(v)
      , m_precomp_data_p(precomp_data_p)
    {
      //construct_precomp_data(m_precomp_data, *this);
    }

    friend point_t operator*(const scalar_t& lhs, const point_pc_t& self)
    {
      point_t result;
      ge_scalarmult_precomp_vartime(&result.m_p3, *self.m_precomp_data_p, &lhs.m_s[0]);
      return result;
    }

    friend point_t operator/(const point_pc_t& self, const scalar_t& rhs)
    {
      point_t result;
      scalar_t reciprocal;
      sc_invert(&reciprocal.m_s[0], &rhs.m_s[0]);
      ge_scalarmult_precomp_vartime(&result.m_p3, *self.m_precomp_data_p, &reciprocal.m_s[0]);
      return result;
    }

    static_assert(sizeof(crypto::public_key) == 32, "size error");

    const precomp_data_t* m_precomp_data_p;
  }; // struct point_pc_t


  //
  // vector of scalars
  //
  struct scalar_vec_t : public std::vector<scalar_t>
  {
    typedef std::vector<scalar_t> super_t;

    scalar_vec_t() {}
    scalar_vec_t(size_t n) : super_t(n) {}
    scalar_vec_t(std::initializer_list<scalar_t> init_list) : super_t(init_list) {}

    bool is_reduced() const
    {
      for (auto& el : *this)
        if (!el.is_reduced())
          return false;
      return true;
    }

    // add a scalar rhs to each element
    scalar_vec_t operator+(const scalar_t& rhs) const
    {
      scalar_vec_t result(size());
      for (size_t i = 0, n = size(); i < n; ++i)
        result[i] = at(i) + rhs;
      return result;
    }

    // subtract a scalar rhs to each element
    scalar_vec_t operator-(const scalar_t& rhs) const
    {
      scalar_vec_t result(size());
      for (size_t i = 0, n = size(); i < n; ++i)
        result[i] = at(i) - rhs;
      return result;
    }

    // multiply each element of the vector by a scalar
    scalar_vec_t operator*(const scalar_t& rhs) const
    {
      scalar_vec_t result(size());
      for (size_t i = 0, n = size(); i < n; ++i)
        result[i] = at(i) * rhs;
      return result;
    }

    // component-wise multiplication (a.k.a the Hadamard product) (only if their sizes match)
    scalar_vec_t operator*(const scalar_vec_t& rhs) const
    {
      scalar_vec_t result;
      const size_t n = size();
      if (n != rhs.size())
        return result;

      result.resize(size());
      for (size_t i = 0; i < n; ++i)
        result[i] = at(i) * rhs[i];
      return result;
    }

    // add each element of two vectors, but only if their sizes match
    scalar_vec_t operator+(const scalar_vec_t& rhs) const
    {
      scalar_vec_t result;
      const size_t n = size();
      if (n != rhs.size())
        return result;

      result.resize(size());
      for (size_t i = 0; i < n; ++i)
        result[i] = at(i) + rhs[i];
      return result;
    }

    // zeroes all elements
    void zero()
    {
      PUSH_GCC_WARNINGS
      DISABLE_GCC_WARNING(class-memaccess)
      size_t size_bytes = sizeof(scalar_t) * size();
      memset(data(), 0, size_bytes);
      POP_GCC_WARNINGS
    }

    // invert all elements in-place efficiently: 4*N muptiplications + 1 inversion
    void invert()
    {
      // muls                    muls_rev
      // 0:                      1 2 3 .. n-1  
      // 1:   0                  2 3 .. n-1
      // 2:   0 1                3 .. n-1
      //
      // n-1: 0 1 2 3 .. n-2     

      const size_t size = this->size();

      if (size < 2)
      {
        if (size == 1)
          at(0) = at(0).reciprocal();
        return;
      }

      scalar_vec_t muls(size), muls_rev(size);
      muls[0] = 1;
      for (size_t i = 0; i < size - 1; ++i)
        muls[i + 1] = at(i) * muls[i];

      muls_rev[size - 1] = 1;
      for (size_t i = size - 1; i != 0; --i)
        muls_rev[i - 1] = at(i) * muls_rev[i];

      scalar_t inv = (muls[size - 1] * at(size - 1)).reciprocal();

      for (size_t i = 0; i < size; ++i)
        at(i) = muls[i] * inv * muls_rev[i];
    }

    scalar_t calc_hs() const;

    void make_random()
    {
      for(size_t size = this->size(), i = 0; i < size; ++i)
        at(i).make_random();
    }

    void resize_and_make_random(size_t size)
    {
      this->resize(size);
      make_random();
    }


  }; // scalar_vec_t


  // treats vector of scalars as an M x N matrix just for convenience
  template<size_t N>
  struct scalar_mat_t : public scalar_vec_t
  {
    typedef scalar_vec_t super_t;
    static_assert(N > 0, "invalid N value");

    scalar_mat_t() {}
    scalar_mat_t(size_t n) : super_t(n) {}
    scalar_mat_t(std::initializer_list<scalar_t> init_list) : super_t(init_list) {}

    // matrix accessor M rows x N cols
    scalar_t& operator()(size_t row, size_t col)
    {
      return at(row * N + col);
    }
  }; // scalar_mat_t



  //
  // Global constants (checked in crypto_constants and crypto_generators_precomp tests)
  //

  namespace xdetails
  {
    extern const precomp_data_t c_point_H_precomp_data;
    extern const precomp_data_t c_point_H2_precomp_data;
    extern const precomp_data_t c_point_U_precomp_data;
    extern const precomp_data_t c_point_X_precomp_data;
    extern const precomp_data_t c_point_H_plus_G_precomp_data;
    extern const precomp_data_t c_point_H_minus_G_precomp_data;
  };

  inline constexpr point_t    c_point_0         {{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }};
  inline constexpr point_g_t  c_point_G         {{ 25485296, 5318399, 8791791, -8299916, -14349720, 6939349, -3324311, -7717049, 7287234, -6577708, -758052, -1832720, 13046421, -4857925, 6576754, 14371947, -13139572, 6845540, -2198883, -4003719, -947565, 6097708, -469190, 10704810, -8556274, -15589498, -16424464, -16608899, 14028613, -5004649, 6966464, -2456167, 7033433, 6781840, 28785542, 12262365, -2659449, 13959020, -21013759, -5262166 }};

  inline constexpr point_pc_t c_point_H         {{ 20574939, 16670001, -29137604, 14614582, 24883426, 3503293, 2667523, 420631, 2267646, -4769165, -11764015, -12206428, -14187565, -2328122, -16242653, -788308, -12595746, -8251557, -10110987, 853396, -4982135, 6035602, -21214320, 16156349, 977218, 2807645, 31002271, 5694305, -16054128, 5644146, -15047429, -568775, -22568195, -8089957, -27721961, -10101877, -29459620, -13359100, -31515170, -6994674 },         &xdetails::c_point_H_precomp_data };
  inline constexpr point_pc_t c_point_H2        {{ 1318371, 14804112, 12545972, -13482561, -12089798, -16020744, -21221907, -8410994, -33080606, 11275578, 3807637, 11185450, -23227561, -12892068, 1356866, -1025012, -8022738, -8139671, -20315029, -13916324, -6475650, -7025596, 12403179, -5139984, -12068178, 10445584, -14826705, -4927780, 13964546, 12525942, -2314107, -10566315, 32243863, 15603849, 5154154, 4276633, -20918372, -15718796, -26386151, 8434696 }, &xdetails::c_point_H2_precomp_data };
  inline constexpr point_pc_t c_point_U         {{ 30807552, 984924, 23426137, -5598760, 7545909, 16325843, 993742, 2594106, -31962071, -959867, 16454190, -4091093, 1197656, 13586872, -9269020, -14133290, 1869274, 13360979, -24627258, -10663086, 2212027, 1198856, 20515811, 15870563, -23833732, 9839517, -19416306, 11567295, -4212053, 348531, -2671541, 484270, -19128078, 1236698, -16002690, 9321345, 9776066, 10711838, 11187722, -16371275 },                    &xdetails::c_point_U_precomp_data };
  inline constexpr point_pc_t c_point_X         {{ 25635916, -5459446, 5768861, 5666160, -6357364, -12939311, 29490001, -4543704, -31266450, -2582476, 23705213, 9562626, -716512, 16560168, 7947407, 2039790, -2752711, 4742449, 3356761, 16338966, 17303421, -5790717, -5684800, 12062431, -3307947, 8139265, -26544839, 12058874, 3452748, 3359034, 26514848, -6060876, 31255039, 11154418, -21741975, -3782423, -19871841, 5729859, 21754676, -12454027 },                &xdetails::c_point_X_precomp_data };
  inline constexpr point_pc_t c_point_H_plus_G  {{ 12291435, 3330843, -3390294, 13894858, -1099584, -6848191, 12040668, -15950068, -7494633, 12566672, -5526901, -16645799, -31081168, -1095427, -13082463, 4573480, -11255691, 4344628, 33477173, 11137213, -3837023, -12436594, -8471924, -814016, 10785607, 9492721, 10992667, 7406385, -5687296, -127915, -6229107, -9324867, 558657, 6493750, 4895261, 12642545, 9549220, 696086, 21894285, -10521807 },                 &xdetails::c_point_H_plus_G_precomp_data };
  inline constexpr point_pc_t c_point_H_minus_G {{ -28347682, 3523701, -3380175, -14453727, 4238027, -6032522, 20235758, 4091609, 12557126, -8064113, 4212476, -13419094, -114185, -7650727, -24238, 16663404, 23676363, -6819610, 18286466, 8714527, -3837023, -12436594, -8471924, -814016, 10785607, 9492721, 10992667, 7406385, -5687296, -127915, -20450317, 13815641, -11604061, -447489, 27380225, 9400847, -8551293, -1173627, -28110171, 14241295 },                 &xdetails::c_point_H_minus_G_precomp_data };


  //
  // hash functions' helper
  //
  struct hash_helper_t
  {
    static scalar_t hs(const scalar_t& s)
    {
      return scalar_t(crypto::cn_fast_hash(s.data(), sizeof s)); // will reduce mod L
    }

    static scalar_t hs(const void* data, size_t size)
    {
      return scalar_t(crypto::cn_fast_hash(data, size)); // will reduce mod L
    }

    static scalar_t hs(const std::string& str)
    {
      return scalar_t(crypto::cn_fast_hash(str.c_str(), str.size())); // will reduce mod L
    }

    static hash h(const std::string& str)
    {
      return crypto::cn_fast_hash(str.c_str(), str.size());
    }

    struct hs_t
    {
      hs_t(size_t size_to_reserve = 0)
      {
        static_assert(sizeof(scalar_t) == sizeof(crypto::public_key), "unexpected size of data");
        m_elements.reserve(size_to_reserve);
      }

      void reserve(size_t elements_count)
      {
        m_elements.reserve(elements_count);
      }

      void resize(size_t elements_count)
      {
        m_elements.resize(elements_count);
      }

      void clear()
      {
        m_elements.clear();
      }

      void add_scalar(const scalar_t& scalar)
      {
        m_elements.emplace_back(scalar);
      }

      void add_point(const point_t& point)
      {
        m_elements.emplace_back(point.to_public_key());
      }

      void add_pub_key(const crypto::public_key& pk)
      {
        m_elements.emplace_back(pk);
      }

      void add_key_image(const crypto::key_image& ki)
      {
        m_elements.emplace_back(ki);
      }

      scalar_t& access_scalar(size_t index)
      {
        return m_elements[index].scalar;
      }

      public_key& access_public_key(size_t index)
      {
        return m_elements[index].pk;
      }

      void add_points_array(const std::vector<point_t>& points_array)
      {
        for (size_t i = 0, size = points_array.size(); i < size; ++i)
          add_point(points_array[i]);
      }

      void add_pub_keys_array(const std::vector<crypto::public_key>& pub_keys_array)
      {
        for (size_t i = 0, size = pub_keys_array.size(); i < size; ++i)
          m_elements.emplace_back(pub_keys_array[i]);
      }

      void add_key_images_array(const std::vector<crypto::key_image>& key_image_array)
      {
        for (size_t i = 0, size = key_image_array.size(); i < size; ++i)
          m_elements.emplace_back(key_image_array[i]);
      }

      void add_hash(const hash& h)
      {
        m_elements.emplace_back(h);
      }

      void add_32_chars(const char(&str32)[32])
      {
        m_elements.emplace_back(str32);
      }

      scalar_t calc_hash(bool clear = true)
      {
        size_t data_size_bytes = m_elements.size() * sizeof(item_t);
        crypto::hash hash;
        crypto::cn_fast_hash(m_elements.data(), data_size_bytes, hash);
        if (clear)
          this->clear();
        return scalar_t(hash); // this will reduce to L
      }
      
      hash calc_hash_no_reduce(bool clear = true)
      {
        size_t data_size_bytes = m_elements.size() * sizeof(item_t);
        hash result;
        crypto::cn_fast_hash(m_elements.data(), data_size_bytes, result);
        if (clear)
          this->clear();
        return result;
      }
      
      void assign_calc_hash(scalar_t& result, bool clear = true)
      {
        static_assert(sizeof result == sizeof(crypto::hash), "size missmatch");
        size_t data_size_bytes = m_elements.size() * sizeof(item_t);
        crypto::cn_fast_hash(m_elements.data(), data_size_bytes, (crypto::hash&)result);
        result.reduce();
        if (clear)
          this->clear();
      }

      union item_t
      {
        item_t() {}
        item_t(const scalar_t& scalar) : scalar(scalar) {}
        item_t(const crypto::public_key& pk) : pk(pk) {}
        item_t(const crypto::key_image& ki) : ki(ki) {}
        item_t(const crypto::hash& h) : h(h) {}
        item_t(const char(&str32)[32]) { memcpy(c, str32, sizeof c); }
        scalar_t scalar;
        crypto::public_key pk;
        crypto::key_image ki;
        crypto::hash h;
        char c[32];
      };

      static_assert(sizeof(item_t::c) == sizeof(item_t::pk), "size missmatch");
      static_assert(sizeof(item_t::h) == sizeof(item_t::pk), "size missmatch");

      std::vector<item_t> m_elements;
    };

    static scalar_t hs(const scalar_t& s, const std::vector<point_t>& ps0, const std::vector<point_t>& ps1)
    {
      hs_t hs_calculator(3);
      hs_calculator.add_scalar(s);
      hs_calculator.add_points_array(ps0);
      hs_calculator.add_points_array(ps1);
      return hs_calculator.calc_hash();
    }

    static scalar_t hs(const crypto::hash& s, const std::vector<crypto::public_key>& ps0, const std::vector<crypto::key_image>& ps1)
    {
      static_assert(sizeof(crypto::hash) == sizeof(scalar_t), "size missmatch");
      hs_t hs_calculator(3);
      hs_calculator.add_scalar(*reinterpret_cast<const scalar_t*>(&s));
      hs_calculator.add_pub_keys_array(ps0);
      hs_calculator.add_key_images_array(ps1);
      return hs_calculator.calc_hash();
    }

    static scalar_t hs(const std::vector<point_t>& ps0, const std::vector<point_t>& ps1)
    {
      hs_t hs_calculator(2);
      hs_calculator.add_points_array(ps0);
      hs_calculator.add_points_array(ps1);
      return hs_calculator.calc_hash();
    }

    static scalar_t hs(const char(&str32)[32], const scalar_t& s)
    {
      hs_t hs_calculator(2);
      hs_calculator.add_32_chars(str32);
      hs_calculator.add_scalar(s);
      return hs_calculator.calc_hash();
    }

    static scalar_t hs(const char(&str32)[32], const scalar_t& s, const crypto::public_key& pk)
    {
      hs_t hs_calculator(2);
      hs_calculator.add_32_chars(str32);
      hs_calculator.add_scalar(s);
      hs_calculator.add_pub_key(pk);
      return hs_calculator.calc_hash();
    }

    static scalar_t hs(const crypto::public_key& pk, const uint64_t i)
    {
      hs_t hs_calculator(2);
      hs_calculator.add_pub_key(pk);
      hs_calculator.add_scalar(scalar_t(i));
      return hs_calculator.calc_hash();
    }

    static scalar_t hs(const crypto::secret_key& sk, const uint64_t i)
    {
      hs_t hs_calculator(2);
      hs_calculator.add_scalar(sk);
      hs_calculator.add_scalar(scalar_t(i));
      return hs_calculator.calc_hash();
    }

    static scalar_t hs(const char(&str32)[32], const crypto::public_key& pk, const uint64_t i)
    {
      hs_t hs_calculator(3);
      hs_calculator.add_32_chars(str32);
      hs_calculator.add_pub_key(pk);
      hs_calculator.add_scalar(scalar_t(i));
      return hs_calculator.calc_hash();
    }

    static scalar_t hs(const char(&str32)[32], const crypto::hash& h)
    {
      hs_t hs_calculator(2);
      hs_calculator.add_32_chars(str32);
      hs_calculator.add_hash(h);
      return hs_calculator.calc_hash();
    }

    static scalar_t hs(const char(&str32)[32], const crypto::point_t& p)
    {
      hs_t hs_calculator(2);
      hs_calculator.add_32_chars(str32);
      hs_calculator.add_point(p);
      return hs_calculator.calc_hash();
    }

    static scalar_t hs(const char(&str32)[32], const crypto::key_derivation& derivation, uint64_t index)
    {
      hs_t hs_calculator(3);
      hs_calculator.add_32_chars(str32);
      hs_calculator.add_pub_key(reinterpret_cast<const crypto::public_key&>(derivation));
      hs_calculator.add_scalar(index);
      return hs_calculator.calc_hash();
    }

    static point_t hp(const point_t& p)
    {
      point_t result;
      crypto::public_key pk = p.to_public_key();

      ge_bytes_hash_to_ec_32(&result.m_p3, (const unsigned char*)&pk);

      return result;
    }

    static point_t hp(const crypto::public_key& p)
    {
      point_t result;
      ge_bytes_hash_to_ec_32(&result.m_p3, (const unsigned char*)&p);
      return result;
    }

    static point_t hp(const scalar_t& s)
    {
      point_t result;
      ge_bytes_hash_to_ec_32(&result.m_p3, s.data());
      return result;
    }

    static point_t hp(const void* data, size_t size)
    {
      point_t result;
      ge_bytes_hash_to_ec(&result.m_p3, data, size);
      return result;
    }

    static point_t hp(const std::string& str)
    {
      point_t result;
      ge_bytes_hash_to_ec(&result.m_p3, str.data(), str.size());
      return result;
    }

  }; // hash_helper_t struct


  inline scalar_t scalar_vec_t::calc_hs() const
  {
    // hs won't touch memory if size is 0, so it's safe
    return hash_helper_t::hs(data(), sizeof(scalar_t) * size());
  } 

} // namespace crypto
