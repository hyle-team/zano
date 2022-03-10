// Copyright (c) 2020-2021 Zano Project
// Copyright (c) 2020-2021 sowle (val@zano.org, crypto.sowle@gmail.com)
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
  extern "C"
  {
#include "crypto/crypto-ops.h"
  } // extern "C"



  //
  // Helpers
  //

  template<class pod_t>
  std::string pod_to_hex_reversed(const pod_t &h)
  {
    constexpr char hexmap[] = "0123456789abcdef";
    const unsigned char* data = reinterpret_cast<const unsigned char*>(&h);
    size_t len = sizeof h;

    std::string s(len * 2, ' ');
    for (size_t i = 0; i < len; ++i) {
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
    for (size_t i = 0; i < len; ++i) {
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
    parse_tpod_from_hex_string(hex_str, t_pod);
    return t_pod;
  }

  //
  // scalar_t - holds a 256-bit scalar, normally in [0..L-1]
  //
  struct alignas(32) scalar_t
  {
    union
    {
      uint64_t      m_u64[4];
      unsigned char m_s[32];
    };

    scalar_t()
    {}

    // won't check scalar range validity (< L)
    scalar_t(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3)
    {
      m_u64[0] = a0;
      m_u64[1] = a1;
      m_u64[2] = a2;
      m_u64[3] = a3;
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
      unsigned int bytes_to_copy = bigint.backend().size() * bigint.backend().limb_bits / 8;
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
      return *(crypto::secret_key*)&m_s[0];
    }

    const crypto::secret_key& as_secret_key() const
    {
      return *(const crypto::secret_key*)&m_s[0];
    }

    operator crypto::secret_key() const
    {
      crypto::secret_key result;
      memcpy(result.data, &m_s, sizeof result.data);
      return result;
    }

    void from_secret_key(const crypto::secret_key& sk)
    {
      uint64_t *p_sk64 = (uint64_t*)&sk;
      m_u64[0] = p_sk64[0];
      m_u64[1] = p_sk64[1];
      m_u64[2] = p_sk64[2];
      m_u64[3] = p_sk64[3];
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

    // returns this = a * b
    scalar_t& assign_mul(const scalar_t& a, const scalar_t& b)
    {
      sc_mul(m_s, a.m_s, b.m_s);
      return *this;
    }

    /*
    I think it has bad symantic (operator-like), consider rename/reimplement -- sowle
    */
    // returns this * b + c
    scalar_t muladd(const scalar_t& b, const scalar_t& c) const
    {
      scalar_t result;
      sc_muladd(result.m_s, m_s, b.m_s, c.m_s);
      return result;
    }

    // returns this = a * b + c
    scalar_t& assign_muladd(const scalar_t& a, const scalar_t& b, const scalar_t& c)
    {
      sc_muladd(m_s, a.m_s, b.m_s, c.m_s);
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

  }; // struct scalar_t

  //
  // Global constants
  //

  extern const scalar_t c_scalar_1;
  extern const scalar_t c_scalar_L;
  extern const scalar_t c_scalar_Lm1;
  extern const scalar_t c_scalar_P;
  extern const scalar_t c_scalar_Pm1;
  extern const scalar_t c_scalar_256m1;
  extern const scalar_t c_scalar_1div8;

  //
  //
  //
  struct point_t
  {
    struct tag_zero {};

    // A point(x, y) is represented in extended homogeneous coordinates (X, Y, Z, T)
    // with x = X / Z, y = Y / Z, x * y = T / Z.
    ge_p3 m_p3;

    point_t()
    {
    }

    explicit point_t(const crypto::public_key& pk)
    {
      if (!from_public_key(pk))
        zero();
    }

    point_t(const unsigned char(&v)[32])
    {
      static_assert(sizeof(crypto::public_key) == sizeof v, "size missmatch");
      if (!from_public_key(*(const crypto::public_key*)v))
        zero();
    }

    point_t(const uint64_t(&v)[4])
    {
      static_assert(sizeof(crypto::public_key) == sizeof v, "size missmatch");
      if (!from_public_key(*(const crypto::public_key*)v))
        zero();
    }

    point_t(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3)
    {
      crypto::public_key pk;
      ((uint64_t*)&pk)[0] = a0;
      ((uint64_t*)&pk)[1] = a1;
      ((uint64_t*)&pk)[2] = a2;
      ((uint64_t*)&pk)[3] = a3;

      if (!from_public_key(pk))
        zero();
    }

    explicit point_t(tag_zero&&)
    {
      zero();
    }

    // as we're using additive notation, zero means identity group element (EC point (0, 1)) here and after
    void zero()
    {
      ge_p3_0(&m_p3);
    }

    bool is_zero() const
    {
      // (0, 1) ~ (0, z, z, 0)
      if (fe_isnonzero(m_p3.X) != 0)
        return false;
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
      ge_scalarmult_p3(&result.m_p3, lhs.m_s, &rhs.m_p3);
      return result;
    }

    point_t& operator*=(const scalar_t& rhs)
    {
      // TODO: ge_scalarmult_vartime_p3
      ge_scalarmult_p3(&m_p3, rhs.m_s, &m_p3);
      return *this;
    }

    friend point_t operator/(const point_t& lhs, const scalar_t& rhs)
    {
      point_t result;
      scalar_t reciprocal;
      sc_invert(&reciprocal.m_s[0], &rhs.m_s[0]);
      ge_scalarmult_p3(&result.m_p3, &reciprocal.m_s[0], &lhs.m_p3);
      return result;
    }

    point_t& modify_mul8()
    {
      ge_mul8_p3(&m_p3, &m_p3);
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
    };

    friend bool operator!=(const point_t& lhs, const point_t& rhs)
    {
      return !(lhs == rhs);
    };

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

  }; // struct point_t


  //
  // point_g_t -- special type for curve's base point
  //
  struct point_g_t : public point_t
  {
    point_g_t()
    {
      scalar_t one(1);
      ge_scalarmult_base(&m_p3, &one.m_s[0]);
    }

    friend point_t operator*(const scalar_t& lhs, const point_g_t&)
    {
      point_t result;
      ge_scalarmult_base(&result.m_p3, &lhs.m_s[0]);
      return result;
    }

    friend point_t operator/(const point_g_t&, const scalar_t& rhs)
    {
      point_t result;
      scalar_t reciprocal;
      sc_invert(&reciprocal.m_s[0], &rhs.m_s[0]);
      ge_scalarmult_base(&result.m_p3, &reciprocal.m_s[0]);
      return result;
    }

    static_assert(sizeof(crypto::public_key) == 32, "size error");

  }; // struct point_g_t


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
      size_t size_bytes = sizeof(scalar_t) * size();
      memset(data(), 0, size_bytes);
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
  // Global constants
  //

  extern const point_g_t c_point_G;

  extern const point_t  c_point_H;
  extern const point_t  c_point_0;

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

    struct hs_t
    {
      hs_t()
      {
        static_assert(sizeof(scalar_t) == sizeof(crypto::public_key), "unexpected size of data");
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

        // faster?
        /* static_assert(sizeof point.m_p3 == 5 * sizeof(item_t), "size missmatch");
        const item_t *p = (item_t*)&point.m_p3;
        m_elements.emplace_back(p[0]);
        m_elements.emplace_back(p[1]);
        m_elements.emplace_back(p[2]);
        m_elements.emplace_back(p[3]);
        m_elements.emplace_back(p[4]); */
      }

      void add_pub_key(const crypto::public_key& pk)
      {
        m_elements.emplace_back(pk);
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

      scalar_t calc_hash(bool clear = true)
      {
        size_t data_size_bytes = m_elements.size() * sizeof(item_t);
        crypto::hash hash;
        crypto::cn_fast_hash(m_elements.data(), data_size_bytes, hash);
        if (clear)
          this->clear();
        return scalar_t(hash); // this will reduce to L
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
        scalar_t scalar;
        crypto::public_key pk;
        crypto::key_image ki;
      };

      std::vector<item_t> m_elements;
    };

    static scalar_t hs(const scalar_t& s, const std::vector<point_t>& ps0, const std::vector<point_t>& ps1)
    {
      hs_t hs_calculator;
      hs_calculator.add_scalar(s);
      hs_calculator.add_points_array(ps0);
      hs_calculator.add_points_array(ps1);
      return hs_calculator.calc_hash();
    }

    static scalar_t hs(const crypto::hash& s, const std::vector<crypto::public_key>& ps0, const std::vector<crypto::key_image>& ps1)
    {
      static_assert(sizeof(crypto::hash) == sizeof(scalar_t), "size missmatch");
      hs_t hs_calculator;
      hs_calculator.add_scalar(*reinterpret_cast<const scalar_t*>(&s));
      hs_calculator.add_pub_keys_array(ps0);
      hs_calculator.add_key_images_array(ps1);
      return hs_calculator.calc_hash();
    }

    static scalar_t hs(const std::vector<point_t>& ps0, const std::vector<point_t>& ps1)
    {
      hs_t hs_calculator;
      hs_calculator.add_points_array(ps0);
      hs_calculator.add_points_array(ps1);
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
  }; // hash_helper_t struct


  inline scalar_t scalar_vec_t::calc_hs() const
  {
    // hs won't touch memory if size is 0, so it's safe
    return hash_helper_t::hs(data(), sizeof(scalar_t) * size());
  }


} // namespace crypto
