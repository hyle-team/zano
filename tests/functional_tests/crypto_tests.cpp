// Copyright (c) 2020-2021 Zano Project
// Copyright (c) 2020-2021 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define USE_INSECURE_RANDOM_RPNG_ROUTINES // turns on random manupulation for tests
#include <utility>
#include "crypto/crypto.h"
#include "epee/include/misc_log_ex.h"
#include "epee/include/profile_tools.h"
#include "include_base_utils.h"
#include "common/crypto_stream_operators.h"
#include "common/varint.h"
#include "currency_core/difficulty.h"

extern "C" {
#include "crypto/crypto-ops.h"
} // extern "C"

namespace mp = boost::multiprecision;

// out = z ^ s (mod l)
void sc_exp(unsigned char* out, const unsigned char* z, const unsigned char* s)
{
  sc_0(out);
  out[0] = 1;
  if (sc_isnonzero(s) == 0)
  {
    return;
  }

  // calc position of the most significant bit of s
  size_t msb_s = 0;
  for (size_t i = 31; i != SIZE_MAX; --i)
  {
    if (s[i] != 0)
    {
      msb_s = 8 * i;
      uint8_t t = s[i] >> 1;
      while (t)
      {
        t >>= 1;
        ++msb_s;
      }
      break;
    }
  }

  for (size_t i = msb_s; i != SIZE_MAX; --i)
  {
    sc_mul(out, out, out);
    //std::cout << "sc_mul(out, out, out);" << std::endl;
    uint8_t bit = (s[i / 8] >> (i % 8)) & 1;
    if (bit)
    {
      sc_mul(out, out, z);
      //std::cout << "sc_mul(out, out, z);" << std::endl;
    }
  }
}

/*
 Input:
 s[0]+256*s[1]+...+256^31*s[31] = s
 a[0]+256*a[1]+...+256^31*a[31] = a
 n
 *
 Output:
 s[0]+256*s[1]+...+256^31*s[31] = a * s^(2^n) mod l
 where l = 2^252 + 27742317777372353535851937790883648493.
 Overwrites s in place.
 */

static inline void
sc_sqmul(unsigned char s[32], const int n, const unsigned char a[32])
{
  int i;
  for (i = 0; i < n; ++i)
    sc_mul(s, s, s);
  sc_mul(s, s, a);
}

void sc_invert2(unsigned char* recip, const unsigned char* s)
{
  unsigned char _10[32], _100[32], _1000[32], _10000[32], _100000[32],
    _1000000[32], _10010011[32], _10010111[32], _100110[32], _1010[32],
    _1010000[32], _1010011[32], _1011[32], _10110[32], _10111101[32],
    _11[32], _1100011[32], _1100111[32], _11010011[32], _1101011[32],
    _11100111[32], _11101011[32], _11110101[32];

  sc_mul(_10, s, s);
  sc_mul(_11, s, _10);
  sc_mul(_100, s, _11);
  sc_mul(_1000, _100, _100);
  sc_mul(_1010, _10, _1000);
  sc_mul(_1011, s, _1010);
  sc_mul(_10000, _1000, _1000);
  sc_mul(_10110, _1011, _1011);
  sc_mul(_100000, _1010, _10110);
  sc_mul(_100110, _10000, _10110);
  sc_mul(_1000000, _100000, _100000);
  sc_mul(_1010000, _10000, _1000000);
  sc_mul(_1010011, _11, _1010000);
  sc_mul(_1100011, _10000, _1010011);
  sc_mul(_1100111, _100, _1100011);
  sc_mul(_1101011, _100, _1100111);
  sc_mul(_10010011, _1000000, _1010011);
  sc_mul(_10010111, _100, _10010011);
  sc_mul(_10111101, _100110, _10010111);
  sc_mul(_11010011, _10110, _10111101);
  sc_mul(_11100111, _1010000, _10010111);
  sc_mul(_11101011, _100, _11100111);
  sc_mul(_11110101, _1010, _11101011);

  sc_mul(recip, _1011, _11110101);

  sc_sqmul(recip, 126, _1010011);

  sc_sqmul(recip, 9, _10);
  sc_mul(recip, recip, _11110101);
  sc_sqmul(recip, 7, _1100111);
  sc_sqmul(recip, 9, _11110101);
  sc_sqmul(recip, 11, _10111101);
  sc_sqmul(recip, 8, _11100111);
  sc_sqmul(recip, 9, _1101011);
  sc_sqmul(recip, 6, _1011);
  sc_sqmul(recip, 14, _10010011);
  sc_sqmul(recip, 10, _1100011);
  sc_sqmul(recip, 9, _10010111);
  sc_sqmul(recip, 10, _11110101);
  sc_sqmul(recip, 8, _11010011);
  sc_sqmul(recip, 8, _11101011);
}

extern void *sha3(const void *in, size_t inlen, void *md, int mdlen);


//
// Helpers
//

template<class pod_t>
std::string pod_to_hex_big_endian(const pod_t &h)
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


uint64_t rand_in_range(uint64_t from_including, uint64_t to_not_including)
{
  uint64_t result = 0;
  crypto::generate_random_bytes(sizeof result, &result);
  return from_including + result % (to_not_including - from_including);
}



int fe_cmp(const fe a, const fe b)
{
  for (size_t i = 9; i != SIZE_MAX; --i)
  {
    if (reinterpret_cast<const uint32_t&>(a[i]) < reinterpret_cast<const uint32_t&>(b[i])) return -1;
    if (reinterpret_cast<const uint32_t&>(a[i]) > reinterpret_cast<const uint32_t&>(b[i])) return 1;
  }
  return 0;
}

static const fe scalar_L_fe = { 16110573, 10012311, -6632702, 16062397, 5471207, 0, 0, 0, 0, 4194304 };


struct alignas(32) scalar_t
{
  union
  {
    uint64_t      m_u64[4];
    unsigned char m_s[32];
  };
  // DONE! consider 1) change to aligned array of unsigned chars
  // consider 2) add 32 byte after to speed up sc_reduce by decreasing num of copy operations

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
    if (v == 0)
      return;
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
    unsigned char tmp[64];
    crypto::generate_random_bytes(64, tmp);
    sc_reduce(tmp);
    scalar_t result;
    memcpy(&result.m_s, tmp, sizeof result.m_s);
    return result;
  }

  // genrate 0 <= x < L
  void make_random()
  {
    unsigned char tmp[64];
    crypto::generate_random_bytes(64, tmp);
    sc_reduce(tmp);
    memcpy(&m_s, tmp, sizeof m_s);
  }

  bool is_zero() const
  {
    return sc_isnonzero(&m_s[0]) == 0;
  }

  bool is_reduced() const
  {
    return sc_check(&m_s[0]) == 0;
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

  /*
  I think it has bad symantic (operator-like), consider rename/reimplement
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
    return ss << "0x" << pod_to_hex_big_endian(v);
  }

  std::string to_string_as_hex_number() const
  {
    return pod_to_hex_big_endian(*this);
  }

  std::string to_string_as_secret_key() const
  {
    return epee::string_tools::pod_to_hex(*this);
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


//__declspec(align(32))
struct point_t
{
  // A point(x, y) is represented in extended homogeneous coordinates (X, Y, Z, T)
  // with x = X / Z, y = Y / Z, x * y = T / Z.
  ge_p3 m_p3;

  point_t()
  {
  }

  explicit point_t(const crypto::public_key& pk)
  {
    from_public_key(pk); // TODO: what if it fails?
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

  void zero()
  {
    ge_p3_0(&m_p3);
  }

  bool is_zero() const
  {
    // (0, 1) ~ (0, z, z, 0)
    return fe_isnonzero(m_p3.X) * fe_cmp(m_p3.Y, m_p3.Z) == 0;
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
    if (!epee::string_tools::parse_tpod_from_hex_string(str, pk))
      return false;
    return from_public_key(pk);
  }

  crypto::public_key to_public_key() const
  {
    crypto::public_key result;
    ge_p3_tobytes((unsigned char*)&result, &m_p3);
    return result;
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

  friend point_t operator*(const scalar_t& lhs, const point_t& rhs)
  {
    point_t result;
    ge_scalarmult_p3(&result.m_p3, reinterpret_cast<const unsigned char*>(&lhs), &rhs.m_p3);
    return result;
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

  friend std::ostream& operator<<(std::ostream& ss, const point_t &v)
  {
    crypto::public_key pk = v.to_public_key();
    return ss << epee::string_tools::pod_to_hex(pk);
  }

  operator std::string() const
  {
    crypto::public_key pk = to_public_key();
    return epee::string_tools::pod_to_hex(pk);
  }

  std::string to_string() const
  {
    crypto::public_key pk = to_public_key();
    return epee::string_tools::pod_to_hex(pk);
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

static const point_g_t c_point_G;

static const scalar_t c_scalar_1      = { 1 };
static const scalar_t c_scalar_L      = { 0x5812631a5cf5d3ed, 0x14def9dea2f79cd6, 0x0,                0x1000000000000000 };
static const scalar_t c_scalar_Lm1    = { 0x5812631a5cf5d3ec, 0x14def9dea2f79cd6, 0x0,                0x1000000000000000 };
static const scalar_t c_scalar_P      = { 0xffffffffffffffed, 0xffffffffffffffff, 0xffffffffffffffff, 0x7fffffffffffffff };
static const scalar_t c_scalar_Pm1    = { 0xffffffffffffffec, 0xffffffffffffffff, 0xffffffffffffffff, 0x7fffffffffffffff };
static const scalar_t c_scalar_256m1  = { 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff };
static const scalar_t c_scalar_1div8  = { 0x6106e529e2dc2f79, 0x7d39db37d1cdad0,  0x0,                0x600000000000000  };
static const point_t  c_point_H       = { 0x05087c1f5b9b32d6, 0x00547595f445c3b5, 0x764df64578552f2a, 0x8a49a651e0e0da45 };  // == Hp(G), this is being check in bpp_basics


// H_s hash function
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

    union item_t
    {
      item_t(const scalar_t& scalar) : scalar(scalar) {}
      item_t(const crypto::public_key& pk) : pk(pk) {}
      item_t(const crypto::key_image& ki) : ki(ki) {}
      scalar_t scalar;
      crypto::public_key pk;
      crypto::key_image ki;
    };

    std::vector<item_t> m_elements;
  };

  /*static scalar_t hs(const scalar_t& s, const std::vector<scalar_t>& ss, const std::vector<point_t>& ps)
  {
    scalar_t result = 0;
    return result;
  }*/

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
};

//
// test helpers
//

inline std::ostream& operator<<(std::ostream& ss, const fe &f)
{
  constexpr size_t fe_index_max = (sizeof f / sizeof f[0]) - 1;
  ss << "{";
  for (size_t i = 0; i <= fe_index_max; ++i)
    ss << f[i] << ", ";
  return ss << f[fe_index_max] << "}";
}

point_t point_from_str(const std::string& str)
{
  crypto::public_key pk;
  if (!epee::string_tools::parse_tpod_from_hex_string(str, pk))
    throw std::runtime_error("couldn't parse pub key");

  point_t result;
  if (!result.from_public_key(pk))
    throw std::runtime_error("invalid pub key");

  return result;
}

scalar_t scalar_from_str(const std::string& str)
{
  crypto::secret_key sk;
  if (!epee::string_tools::parse_tpod_from_hex_string(str, sk))
    throw std::runtime_error("couldn't parse sec key");

  scalar_t result;
  result.from_secret_key(sk);
  if (result > c_scalar_Lm1)
    throw std::runtime_error("sec key scalar >= L");

  return result;
}

crypto::hash hash_from_str(const std::string& str)
{
  crypto::hash hash;
  if (!epee::string_tools::parse_tpod_from_hex_string(str, hash))
    throw std::runtime_error("couldn't parse hash");

  return hash;
}

std::string point_to_str(const point_t& point)
{
  crypto::public_key pk = point.to_public_key();
  return epee::string_tools::pod_to_hex(pk);
}

std::string scalar_to_str(const scalar_t& scalar)
{
  return epee::string_tools::pod_to_hex(scalar);
}

bool generate_test_ring_and_sec_keys(size_t N, size_t L, std::vector<point_t>& ring, std::vector<scalar_t>& secret_keys,
  std::vector<size_t>& ring_mapping, std::vector<point_t>& key_images)
{
  secret_keys.resize(L);
  for (size_t i = 0; i < L; ++i)
    secret_keys[i].make_random();

  std::vector<point_t> fake_pub_keys(N / 2 - L);
  for (size_t i = 0; i < fake_pub_keys.size(); ++i)
    fake_pub_keys[i] = hash_helper_t::hp(i * c_point_G);

  ring_mapping.resize(N / 2);
  for (size_t i = 0; i < N / 2; ++i)
    ring_mapping[i] = i;

  std::shuffle(ring_mapping.begin(), ring_mapping.end(), crypto::uniform_random_bit_generator());

  ring.resize(N / 2);
  for (size_t i = 0; i < ring.size(); ++i)
  {
    if (i < L)
    {
      // own keys
      ring[ring_mapping[i]] = secret_keys[i] * c_point_G;
    }
    else
    {
      // fake keys
      ring[ring_mapping[i]] = fake_pub_keys[i - L];
    }
  }

  ring_mapping.resize(L);

  key_images.resize(L);
  for (size_t i = 0; i < L; ++i)
    key_images[i] = hash_helper_t::hp(ring[ring_mapping[i]]) / secret_keys[i];

  return true;
}

bool generate_test_ring_and_sec_keys(size_t N, size_t L, std::vector<crypto::public_key>& ring, std::vector<crypto::secret_key>& secret_keys,
  std::vector<size_t>& ring_mapping, std::vector<crypto::key_image> & key_images)
{
  secret_keys.resize(L);
  for (size_t i = 0; i < L; ++i)
    secret_keys[i] = scalar_t::random().as_secret_key();

  std::vector<point_t> fake_pub_keys(N / 2 - L);
  for (size_t i = 0; i < fake_pub_keys.size(); ++i)
    fake_pub_keys[i] = hash_helper_t::hp(i * c_point_G);

  ring_mapping.resize(N / 2);
  for (size_t i = 0; i < N / 2; ++i)
    ring_mapping[i] = i;

  std::shuffle(ring_mapping.begin(), ring_mapping.end(), crypto::uniform_random_bit_generator());

  ring.resize(N / 2);
  for (size_t i = 0; i < ring.size(); ++i)
  {
    if (i < L)
    {
      // own keys
      ring[ring_mapping[i]] = (secret_keys[i] * c_point_G).to_public_key();
    }
    else
    {
      // fake keys
      ring[ring_mapping[i]] = fake_pub_keys[i - L].to_public_key();
    }
  }

  ring_mapping.resize(L);

  key_images.resize(L);
  for (size_t i = 0; i < L; ++i)
    key_images[i] = (hash_helper_t::hp(ring[ring_mapping[i]]) / secret_keys[i]).to_key_image();

  return true;
}

uint64_t hash_64(const void* data, size_t size)
{
  crypto::hash h = crypto::cn_fast_hash(data, size);
  uint64_t* phash_as_array = (uint64_t*)&h;
  return phash_as_array[0] ^ phash_as_array[1] ^ phash_as_array[2] ^ phash_as_array[3];
}




////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////



/*
* tiny facade to gtest-alike interface to make simplier further tests transfer to unit_tests
*/
#define TEST(test_name_a, test_name_b) \
  static bool test_name_a ## _ ## test_name_b(); \
  static test_keeper_t test_name_a ## _ ## test_name_b ## _ ## keeper(STR(COMBINE(test_name_a ## _, test_name_b)), & test_name_a ## _ ## test_name_b); \
  static bool test_name_a ## _ ## test_name_b()
#define ASSERT_TRUE(expr)  CHECK_AND_ASSERT_MES(expr, false, "This is not true: " #expr)
#define ASSERT_FALSE(expr) CHECK_AND_ASSERT_MES((expr) == false, false, "This is not false: " #expr)
#define ASSERT_EQ(a, b)    CHECK_AND_ASSERT_MES(a == b, false, #a " != " #b "\n    " << a << " != " << b)

typedef bool(*bool_func_ptr_t)();
static std::vector<std::pair<std::string, bool_func_ptr_t>> g_tests;
struct test_keeper_t
{
  test_keeper_t(const char* name, bool_func_ptr_t func_p)
  {
    g_tests.push_back(std::make_pair(name, func_p));
  }
};


////////////////////////////////////////////////////////////////////////////////
#include "L2S.h"
////////////////////////////////////////////////////////////////////////////////



//
// Tests
//

#include "crypto_tests_performance.h"


TEST(crypto, ge_scalarmult_vartime_p3)
{
  // make sure that my ge_scalarmult_vartime_p3 gives the same result as ge_scalarmul_p3

  size_t N = 5000;
  std::vector<point_t> points;
  points.push_back(0 * c_point_G);
  points.push_back(1 * c_point_G);
  points.push_back(c_scalar_Lm1 * c_point_G);
  points.push_back(c_scalar_L * c_point_G);
  points.push_back((c_scalar_L + 1) * c_point_G);
  size_t i = points.size();
  points.resize(N); // should have kept previously added points
  ASSERT_EQ(points[0], points[3]);
  ASSERT_EQ(points[1], points[4]);

  for (; i < points.size(); ++i)
  {
    if (i & 1)
      points[i] = scalar_t::random() * c_point_G;
    else
      points[i] = hash_helper_t::hp(points[i - 1]);
  }

  for (size_t j = 0; j < points.size(); ++j)
  {
    scalar_t r;
    r.make_random();

    point_t A;
    ge_scalarmult_p3(&A.m_p3, r.data(), &points[j].m_p3);

    point_t B;
    ge_scalarmult_vartime_p3(&B.m_p3, r.data(), &points[j].m_p3);

    ASSERT_EQ(A, B);
  }

  return true;
}

size_t find_pos_hash(const boost::multiprecision::uint256_t& L_div_D, const currency::wide_difficulty_type& D, const uint64_t amount,
  uint64_t& kernel, scalar_t& d0, uint64_t& d1)
{
  static const boost::multiprecision::uint256_t c_L_w = c_scalar_L.as_boost_mp_type<boost::multiprecision::uint256_t>();

  crypto::generate_random_bytes(sizeof kernel, &kernel);

  const boost::multiprecision::uint512_t L_div_D_mul_v = boost::multiprecision::uint512_t(L_div_D) * amount;
  scalar_t L_div_D_mul_v_sc;
  if (L_div_D_mul_v < c_L_w)
    L_div_D_mul_v_sc = scalar_t(L_div_D_mul_v); // here we assured that L_div_D_mul_v < 2**256
  else
    L_div_D_mul_v_sc = scalar_t(c_L_w); // too small D or too big amount, so any h would go

  size_t i = 0;
  for (; i < 1000000; ++i)
  {
    scalar_t h = hash_helper_t::hs(&kernel, sizeof kernel);
    if (h < L_div_D_mul_v_sc)
    {
      // found!
      boost::multiprecision::uint512_t h_w = h.as_boost_mp_type<boost::multiprecision::uint512_t>();
      boost::multiprecision::uint512_t d0_w = h_w / amount * D;
      ASSERT_TRUE(d0_w < c_L_w);
      d0 = scalar_t(d0_w);

      boost::multiprecision::uint512_t ddv = (d0_w / D) * amount;
      ASSERT_TRUE(h_w < ddv);
      if (h_w == ddv)
      {
        ASSERT_TRUE(false);
      }

      boost::multiprecision::uint512_t d1_w = ddv - h_w;
      ASSERT_TRUE(d1_w <= UINT64_MAX);

      d1 = d1_w.convert_to<uint64_t>();
      break;
    }
    ++kernel;
  }

  return i;
}

struct sig_check_t
{
  crypto::hash prefix_hash;
  crypto::key_image ki;
  std::vector<crypto::public_key> pub_keys;
  std::vector<const crypto::public_key*> pub_keys_p;
  crypto::secret_key xi;
  size_t secret_key_index;
  std::vector<crypto::signature> sigs;

  sig_check_t()
  {}

  void prepare_random_data(size_t decoy_set_size)
  {
    crypto::public_key Pi;
    crypto::generate_keys(Pi, xi);

    crypto::generate_random_bytes(sizeof prefix_hash, &prefix_hash);

    for (size_t i = 0; i + 1 < decoy_set_size; ++i)
    {
      crypto::public_key p;
      crypto::secret_key s;
      crypto::generate_keys(p, s);
      pub_keys.push_back(p);
    }

    secret_key_index = rand_in_range(0, pub_keys.size());
    pub_keys.insert(pub_keys.begin() + secret_key_index, Pi);

    for (auto& pk : pub_keys)
      pub_keys_p.push_back(&pk);

    crypto::generate_key_image(Pi, xi, ki);

    sigs.resize(pub_keys.size());
  }

  void generate()
  {
    crypto::generate_ring_signature(prefix_hash, ki, pub_keys_p, xi, secret_key_index, sigs.data());
  }

  bool check()
  {
    return crypto::check_ring_signature(prefix_hash, ki, pub_keys_p, sigs.data());
  }
};


TEST(crypto, ring_sigs)
{
  return true;
  size_t n = 1000;
  size_t decoy_set_size = 8;

  std::cout << "using decoy set with size = " << decoy_set_size << std::endl;

  std::vector<sig_check_t> sigs;
  sigs.resize(n);

  for (size_t i = 0; i < sigs.size(); ++i)
    sigs[i].prepare_random_data(decoy_set_size);

  std::cout << n << " random sigs prepared" << std::endl;

  bool r = true;

  TIME_MEASURE_START(gen_mcs);
  for (size_t i = 0; i < sigs.size(); ++i)
  {
    sigs[i].generate();
  }
  TIME_MEASURE_FINISH(gen_mcs);

  std::cout << n << " random sigs generated in " << gen_mcs / 1000 << " s" << std::endl;

  TIME_MEASURE_START(check_mcs);
  for (size_t i = 0; i < sigs.size(); ++i)
  {
    if (!sigs[i].check())
    {
      r = false;
      break;
    }
  }
  TIME_MEASURE_FINISH(check_mcs);

  ASSERT_TRUE(r);

  std::cout << n << " random sigs checked:" << std::endl;
  std::cout << "    " << std::right << std::setw(8) << gen_mcs / 1000 << " ms for generation total" << std::endl;
  std::cout << "    " << std::right << std::setw(8) << std::fixed << std::setprecision(1) << double(gen_mcs) / n << " mcs for generating per one signature" << std::endl;
  std::cout << "    " << std::right << std::setw(8) << check_mcs / 1000 << " ms for checking total" << std::endl;
  std::cout << "    " << std::right << std::setw(8) << std::fixed << std::setprecision(1) << double(check_mcs) / n << " mcs for checking per one signature" << std::endl;

  return true;
}

TEST(crypto, keys)
{
  // keypair: sk: 407b3b73df8f11737494bdde6ca47a42e1b537390aec2fa781a2d170335c440f
  //          pk: 1b546af91d31fdb1c476fd62fbb65b6fd5ed47804185fc77d48bc4cc00f47ef0

  bool r = false;
  crypto::public_key pk;
  r = epee::string_tools::parse_tpod_from_hex_string("1b546af91d31fdb1c476fd62fbb65b6fd5ed47804185fc77d48bc4cc00f47ef0", pk);
  ASSERT_TRUE(r);

  crypto::secret_key sk;
  r = epee::string_tools::parse_tpod_from_hex_string("407b3b73df8f11737494bdde6ca47a42e1b537390aec2fa781a2d170335c440f", sk);
  ASSERT_TRUE(r);

  crypto::public_key pk2;
  r = crypto::secret_key_to_public_key(sk, pk2);
  ASSERT_TRUE(r);

  std::cout << pk << std::endl;
  std::cout << pk2 << std::endl;

  ASSERT_EQ(pk, pk2);
  ASSERT_TRUE(crypto::check_key(pk));
  ASSERT_TRUE(crypto::check_key(pk2));

  std::cout << std::endl;

  crypto::generate_keys(pk, sk);
  r = crypto::secret_key_to_public_key(sk, pk2);
  ASSERT_TRUE(r);

  ASSERT_EQ(pk, pk2);

  ASSERT_TRUE(crypto::check_key(pk));
  ASSERT_TRUE(crypto::check_key(pk2));

  return true;
}


TEST(crypto, scalar_basics)
{
  scalar_t zero = 0;
  ASSERT_TRUE(zero.is_zero());
  scalar_t one = 1;
  ASSERT_FALSE(one.is_zero());
  ASSERT_TRUE(one > zero);
  ASSERT_TRUE(one.muladd(zero, zero) == zero);

  scalar_t z = 0;
  for (size_t j = 0; j < 1000; ++j)
  {
    z.make_random();
    ASSERT_FALSE(z.is_zero());
    ASSERT_TRUE(z.is_reduced());
    ASSERT_TRUE(z > z - 1);
    ASSERT_TRUE(z < z + 1);
    ASSERT_TRUE(z.muladd(one, zero) == z);
    ASSERT_TRUE(z.muladd(zero, one) == one);
    ASSERT_TRUE(z.muladd(z, z) == z * z + z);
  }

  ASSERT_TRUE(c_scalar_L > 0 && !(c_scalar_L < 0));
  ASSERT_TRUE(c_scalar_Lm1 > 0 && !(c_scalar_Lm1 < 0));
  ASSERT_TRUE(c_scalar_Lm1 < c_scalar_L);
  ASSERT_FALSE(c_scalar_Lm1 > c_scalar_L);
  ASSERT_TRUE(c_scalar_P > c_scalar_Pm1);
  ASSERT_FALSE(c_scalar_P < c_scalar_Pm1);

  std::cout << "0   = " << zero << std::endl;
  std::cout << "1   = " << one << std::endl;
  std::cout << "L   = " << c_scalar_L << std::endl;
  std::cout << "L-1 = " << c_scalar_Lm1 << std::endl;
  std::cout << "P   = " << c_scalar_P << std::endl;
  std::cout << "P-1 = " << c_scalar_Pm1 << std::endl;
  std::cout << std::endl;

  // check rolling over L for scalars arithmetics
  ASSERT_EQ(c_scalar_Lm1 + 1, 0);
  ASSERT_EQ(scalar_t(0) - 1, c_scalar_Lm1);
  ASSERT_EQ(c_scalar_Lm1 * 2, c_scalar_Lm1 - 1); // (L - 1) * 2 = L + L - 2 = (L - 1) - 1  (mod L)
  ASSERT_EQ(c_scalar_Lm1 * 100, c_scalar_Lm1 - 99);
  ASSERT_EQ(c_scalar_Lm1 * c_scalar_Lm1, 1);     // (L - 1) * (L - 1) = L*L - 2L + 1 = 1 (mod L)
  ASSERT_EQ(c_scalar_Lm1 * (c_scalar_Lm1 - 1) * c_scalar_Lm1, c_scalar_Lm1 - 1);
  ASSERT_EQ(c_scalar_L   * c_scalar_L, 0);

  ASSERT_EQ(scalar_t(3) / c_scalar_Lm1, scalar_t(3) * c_scalar_Lm1);  // because (L - 1) ^ 2 = 1

  // check is_reduced
  ASSERT_TRUE(c_scalar_Lm1.is_reduced());
  ASSERT_FALSE(c_scalar_L.is_reduced());
  scalar_t p = c_scalar_L;
  ASSERT_FALSE(p.is_reduced());
  p = p + 1;
  ASSERT_TRUE(p.is_reduced());
  p = 0;
  p = p + c_scalar_P;
  ASSERT_TRUE(p.is_reduced());
  mp::uint256_t mp_p_mod_l = c_scalar_P.as_boost_mp_type<mp::uint256_t>() % c_scalar_L.as_boost_mp_type<mp::uint256_t>();
  ASSERT_EQ(p, scalar_t(mp_p_mod_l));

  return true;
}

TEST(crypto, sc_mul_performance)
{
  std::vector<scalar_t> scalars(100000);
  for (auto& s : scalars)
    s.make_random();

  scalar_t m = 1;
  
  TIME_MEASURE_START(t);
  for (auto& s : scalars)
    m *= s;
  TIME_MEASURE_FINISH(t);

  std::cout << m << std::endl;

  LOG_PRINT_L0("sc_mul: " << std::fixed << std::setprecision(3) << t / 1000.0 << " ms");

  return true;
}

TEST(crypto, hp)
{
  bool r = false;
  scalar_t sk;
  r = epee::string_tools::parse_tpod_from_hex_string("407b3b73df8f11737494bdde6ca47a42e1b537390aec2fa781a2d170335c440f", sk);
  ASSERT_TRUE(r);
  crypto::public_key pk;
  r = epee::string_tools::parse_tpod_from_hex_string("1b546af91d31fdb1c476fd62fbb65b6fd5ed47804185fc77d48bc4cc00f47ef0", pk);
  ASSERT_TRUE(r);

  point_t P;
  ASSERT_TRUE(P.from_public_key(pk));
  // make sure pk and sk are pair
  ASSERT_EQ(P, sk * c_point_G);

  // make sure generate_key_image does the same as sk * hash_helper::hp(pk)
  crypto::key_image ki;
  crypto::generate_key_image(pk, sk.as_secret_key(), ki);
  point_t KI;
  ASSERT_TRUE(KI.from_public_key((crypto::public_key&)ki)); // key image is a point

  ASSERT_EQ(KI, sk * hash_helper_t::hp(P));

  LOG_PRINT_L0(sk.as_secret_key() << " * G = " << P);

  point_t P1 = hash_helper_t::hp(P);
  LOG_PRINT_L0("Hp(" << P << ") = " << P1);
  ASSERT_EQ(P1, point_from_str("f9506848342ddb23b014e5975462757f5d296a6acfa0e9837ff940f7655becdb"));

  point_t P100 = P;
  for (size_t i = 0; i < 100; ++i)
    P100 = hash_helper_t::hp(P100);

  ASSERT_EQ(P100, point_from_str("925f195fc629fa15f768c775f7eed3a43dcd45c702974d161eb610a9ab9df4f0"));


  crypto::hash hash;
  crypto::cn_fast_hash(sk.data(), sizeof sk, hash);

  LOG_PRINT_L0("cn_fast_hash(" << sk.as_secret_key() << ") = " << hash);
  ASSERT_EQ(hash, hash_from_str("ee05b0f64eebf20da306eec142da99283154316391caf474be41ff010afb4298"));

  crypto::cn_fast_hash(nullptr, 0, hash);
  LOG_PRINT_L0("cn_fast_hash('') = " << hash);
  ASSERT_EQ(hash, hash_from_str("c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470"));

  scalar_t z(hash);
  point_t zG = z * c_point_G;
  LOG_PRINT_L0("cn_fast_hash('') * G = " << zG.to_public_key() << " (pub_key)");
  ASSERT_EQ(zG, point_from_str("7849297236cd7c0d6c69a3c8c179c038d3c1c434735741bb3c8995c3c9d6f2ac"));

  crypto::cn_fast_hash("zano", 4, hash);
  LOG_PRINT_L0("cn_fast_hash('zano') = " << hash);
  ASSERT_EQ(hash, hash_from_str("23cea10abfdf3ace0b7132291d51e4eb5a392afb2147e67f907ff4f8f5dd4f9f"));

  z = hash;
  zG = z * c_point_G;
  LOG_PRINT_L0("cn_fast_hash('zano') * G = " << zG.to_public_key() << " (pub_key)");
  ASSERT_EQ(zG, point_from_str("71407d59e9d671fa02f26a6a7f4726c3087d8f1732453396638a1dc2929fb57a"));

  char buf[2000];
  for (size_t i = 0; i < sizeof buf; i += 4)
    *(uint32_t*)&buf[i] = *(uint32_t*)"zano";
  crypto::cn_fast_hash(buf, sizeof buf, (char*)&hash);
  LOG_PRINT_L0("cn_fast_hash('zano' x 500) = " << hash);
  ASSERT_EQ(hash, hash_from_str("16d87120c601a6ef3e4ffa5e58176a36b814288199f23ec09ef178c554e8879b"));

  z = hash;
  zG = z * c_point_G;
  LOG_PRINT_L0("cn_fast_hash('zano' x 500) * G = " << zG.to_public_key() << " (pub_key)");
  ASSERT_EQ(zG, point_from_str("dd93067a02fb8661aa64504ac1503402a34426f43650d970c35147cec4b61d55"));

  return true;
}

TEST(crypto, sc_invert_performance)
{
  std::vector<scalar_t> scalars(10000);
  LOG_PRINT_L0("Running " << scalars.size() << " sc_invert tests...");

  for (auto& s : scalars)
  {
    s.make_random();
    scalar_t a, b;
    sc_invert(&a.m_s[0], &s.m_s[0]);
    sc_invert2(&b.m_s[0], &s.m_s[0]);
    ASSERT_EQ(a, b);
  }

  std::vector<scalar_t> results_0(scalars.size());
  std::vector<scalar_t> results_1(scalars.size());
  std::vector<scalar_t> results_2(scalars.size());

  for (size_t j = 0; j < 10; ++j)
  {
    LOG_PRINT_L0("Run #" << j);

    // warm-up
    for (size_t i = 0; i < scalars.size(); ++i)
      sc_invert(&results_0[i].m_s[0], &scalars[i].m_s[0]);

    TIME_MEASURE_START(t_1);
    for (size_t i = 0; i < scalars.size(); ++i)
      sc_invert(&results_1[i].m_s[0], &scalars[i].m_s[0]);
    TIME_MEASURE_FINISH(t_1);

    TIME_MEASURE_START(t_2);
    for (size_t i = 0; i < scalars.size(); ++i)
      sc_invert(&results_2[i].m_s[0], &scalars[i].m_s[0]);
    TIME_MEASURE_FINISH(t_2);

    LOG_PRINT_L0("sc_invert:  " << std::fixed << std::setprecision(3) << 1.0 * t_1 / scalars.size() << " mcs " << (t_1 < t_2 ? "WIN" : ""));
    LOG_PRINT_L0("sc_invert2: " << std::fixed << std::setprecision(3) << 1.0 * t_2 / scalars.size() << " mcs " << (t_1 < t_2 ? "" : "    WIN"));
  }

  return true;
}

TEST(crypto, scalar_arithmetic_assignment)
{
  std::vector<scalar_t> scalars(1000);
  scalar_t mm = 1, sum = 0;
  for (auto& s : scalars)
  {
    s.make_random();
    mm /= s;
    sum += s;
  }
  ASSERT_TRUE(!mm.is_zero() && mm != scalar_t(1));
  ASSERT_TRUE(!sum.is_zero());
  std::shuffle(scalars.begin(), scalars.end(), crypto::uniform_random_bit_generator());
  for (auto& s : scalars)
  {
    mm *= s;
    sum -= s;
  }
  ASSERT_EQ(mm, 1);
  ASSERT_EQ(sum, 0);

  return true;
}

TEST(crypto, point_basics)
{
  scalar_t s = 4;
  point_t E = s * c_point_G;
  point_t X = 4 * E;
  point_t K = 193847 * c_point_G;
  point_t C = E + K;

  ASSERT_EQ(X, 16 * c_point_G);
  ASSERT_EQ(C - K, E);
  ASSERT_EQ(C - E, K);
  ASSERT_EQ(C, (193847 + 4) * c_point_G);

  ASSERT_EQ(c_point_G / 1, 1 * c_point_G);
  ASSERT_EQ(C / 3, E / 3 + K / 3);
  //ASSERT_EQ(K, 61 * (K / (61)));
  //ASSERT_EQ(K, 192847 * (K / scalar_t(192847)));
  ASSERT_EQ(K, 61 * (283 * (192847 * (K / (192847ull * 283 * 61)))));

  ASSERT_EQ(E, c_point_G + c_point_G + c_point_G + c_point_G);
  ASSERT_EQ(E - c_point_G, 3 * c_point_G);

  for (size_t i = 0; i < 1000; ++i)
  {
    E = hash_helper_t::hp(E);
    point_t Z = 0 * E;
    ASSERT_TRUE(Z.is_zero());
    scalar_t rnd;
    rnd.make_random();
    Z = rnd * E;
    Z = Z - E;
    Z = Z - (rnd - 1) * E;
    ASSERT_TRUE(Z.is_zero());
  }

  return true;
}

TEST(crypto, scalar_reciprocal)
{
  int64_t test_nums[] = { 1, 2, 10 };

  for (size_t i = 0; i < sizeof test_nums / sizeof test_nums[0]; ++i)
  {
    scalar_t s = test_nums[i];
    scalar_t z = s - s;
    ASSERT_TRUE(z.is_zero());
  }

  scalar_t s = 20;
  scalar_t d = 5;
  scalar_t e = s / d;
  scalar_t m = e * d;

  ASSERT_TRUE(m == s);

  return true;
}


TEST(crypto, scalars)
{
  scalar_t s = 20;
  scalar_t d = 5;
  scalar_t e = s / d;
  scalar_t m = e * d;

  ASSERT_TRUE(m == s);

  return true;
}

//
// ML2S tests
//

TEST(ml2s, rsum)
{
  // Ref: Rsum(3, 8, [1, 2, 3, 4, 5, 6, 7, 8], { 1: 1, 2 : 2, 3 : 3 }, { 1: 4, 2 : 5 }) == 659
  
  point_t A = scalar_t::random() * c_point_G;
  point_t result;

  bool r = ml2s_rsum(3, std::vector<point_t>{ A, 2 * A, 3 * A, 4 * A, 5 * A, 6 * A, 7 * A, 8 * A },
    std::vector<scalar_t>{ 1, 2, 3 }, std::vector<scalar_t>{ 4, 5 }, result);
  ASSERT_TRUE(r);
  ASSERT_EQ(result, 659 * A);

  return true;
}

TEST(ml2s, hs)
{
  scalar_t x = 2, p = 250;
  //sc_exp(r.data(), x.data(), p.data());

  x = 0;

  crypto::hash h;
  scalar_t r;

  sha3(0, 0, &h, sizeof h);
  LOG_PRINT("SHA3 0 -> " << h, LOG_LEVEL_0);
  LOG_PRINT("SHA3 0 -> " << (scalar_t&)h, LOG_LEVEL_0);

  h = crypto::cn_fast_hash(0, 0);
  LOG_PRINT("CN 0 -> " << h, LOG_LEVEL_0);
  LOG_PRINT("CN 0 -> " << (scalar_t&)h, LOG_LEVEL_0);

  std::string abc("abc");
  sha3(abc.c_str(), abc.size(), &h, sizeof h);
  LOG_PRINT(abc << " -> " << h, LOG_LEVEL_0);
  LOG_PRINT(abc << " -> " << (scalar_t&)h, LOG_LEVEL_0);

  h = crypto::cn_fast_hash(abc.c_str(), abc.size());
  LOG_PRINT(abc << " -> " << h, LOG_LEVEL_0);
  LOG_PRINT(abc << " -> " << (scalar_t&)h, LOG_LEVEL_0);


  return true;
}

TEST(ml2s, hsc)
{
  hash_helper_t::hs_t hsc;
  hsc.add_point(c_point_G);
  LOG_PRINT_L0("hsc(G) = " << hsc.calc_hash().as_secret_key());

  hsc.add_point(c_point_G);
  hsc.add_scalar(scalar_from_str("0b2900d8eaf9996d2c5345833b280ef93be5c6881dc26f89ecad7a86441cc10e"));
  hsc.add_point(c_point_G);
  LOG_PRINT_L0("hsc(GsG) = " << hsc.calc_hash().as_secret_key());

  return true;
}


TEST(ml2s, py2cpp)
{
  // verify a signature generated by python's reference code

  std::vector<point_t> B_array = { point_from_str("2f1132ca61ab38dff00f2fea3228f24c6c71d58085b80e47e19515cb27e8d047"), point_from_str("5866666666666666666666666666666666666666666666666666666666666666") };
  ml2s_signature sig;
  sig.z = scalar_from_str("0b2900d8eaf9996d2c5345833b280ef93be5c6881dc26f89ecad7a86441cc10e");
  {
    ml2s_signature_element e;
    e.H_array = { point_from_str("de6169b6f9e4e3dcf450c87f5f9cc5954db82fea3a0afccf285ffff0590cc343"), point_from_str("0d09ad5376090d844afde60f7b0ba78e8010987724e76eeb1f73d490e26ef114") };
    e.T = point_from_str("8df512bbe80238bbb5579e41caab69f209c35560f03a7fdde5d1369b059e38f8");
    e.T0 = point_from_str("fb973d514bb0df3f2bb30038dfe50164a9cd48ef6e7b9ac7a253cbc328f214ab");
    e.Z = point_from_str("27b77c99771a4d8198fd5ebcfae71dc063bd2286006fb9f4a72b72dbcf552c30");
    e.Z0 = point_from_str("a6dd57fcfb5d00c9ad866df931732df491fa0dde14ac76a817399fc2e212666d");
    e.r_array = { scalar_from_str("12921a30105714028187e9ecccf2106d04f6e08dbe7d07001fe3e93a1ffa6e04"), scalar_from_str("fbe2fb0ff0481309db53791b77557eab7359d6cf8274d8dac6ad5092075ec009") };
    e.t = scalar_from_str("002fca7de8af4c57b94b4b49ba2792644dddf09baf729fe31be1c265d2df2e0e");
    e.t0 = scalar_from_str("e0c20ec9adbb047d3b26f8073b49e21e700b9bfe6601b6565ec5ec0e83ad230a");
    sig.elements.emplace_back(e);
  }
  {
    ml2s_signature_element e;
    e.H_array = { point_from_str("661f05591cc6e0a42bc94b5f9d45318248051368306bc199d555306598c6330a"), point_from_str("5281aa630ee6e7ef9af5d758fd98fbf72680e8a97e3329827c9ba24577b06aea") };
    e.T = point_from_str("97463bbf9bf49474853e4e68bb927b8aeb53472b33315930583887ee63d0e341");
    e.T0 = point_from_str("e4877f4e8a9dd8a57955a13a66a2ced96f3e7f2a64ff40beef9641ad4eecbf91");
    e.Z = point_from_str("24a64d04e530772d23a3455047b89569fe796d002dc3d40b57f2ab350eddff47");
    e.Z0 = point_from_str("8db841a90faac3c019f42824fac7177505ca9fb51a21ba0fd8d4112ec9dd9e89");
    e.r_array = { scalar_from_str("52038eb19d0eaffe10cc379ce5a739557adccf4a95ccf77c3159c71d2f166b0f"), scalar_from_str("aebb28319c552708ad2495802a3f585780ae1286146d6e5a845e3074607c0a03") };
    e.t = scalar_from_str("285a411ddcf56747fe11f5053ec3bff7617ad3ed9c6f1929874d90d91fb7680a");
    e.t0 = scalar_from_str("44552f3136dbee1b5b63d4a1a9d8799aed54ddd75ed9ebdf6de0a6e032ac7505");
    sig.elements.emplace_back(e);
  }

  scalar_t m = 31337;

  uint8_t err = 0;
  bool r = ml2s_lnk_sig_verif(m, B_array, sig, &err);

  ASSERT_TRUE(r);

  return true;
}

std::string ml2s_sig_to_python(const std::vector<point_t>& B_array, const ml2s_signature& sig, const std::vector<point_t>& I_array)
{
  std::string str;
  if (B_array.empty() || sig.elements.empty())
    return str;

  auto points_array_to_dict = [](const std::vector<point_t>& v) -> std::string
  {
    std::string s;
    size_t idx = 1;
    for (auto& p : v)
      s += epee::string_tools::num_to_string_fast(idx++) + ": ed25519.Point('" + p.to_string() + "'), ";
    if (s.size() > 2)
      s.erase(s.end() - 2, s.end());
    return s;
  };

  auto points_array_to_array = [](const std::vector<point_t>& v) -> std::string
  {
    std::string s;
    for (auto& p : v)
      s += "ed25519.Point('" + p.to_string() + "'), ";
    if (s.size() > 2)
      s.erase(s.end() - 2, s.end());
    return s;
  };

  auto scalars_array_to_dict = [](const std::vector<scalar_t>& v) -> std::string
  {
    std::string s;
    size_t idx = 1;
    for (auto& p : v)
      s += epee::string_tools::num_to_string_fast(idx++) + ": 0x" + p.to_string_as_hex_number() + ", ";
    if (s.size() > 2)
      s.erase(s.end() - 2, s.end());
    return s;
  };

  std::string indent = "    ";

  str += indent + "B_array = [ ";
  for (auto& B : B_array)
    str += std::string("ed25519.Point('") + B.to_string() + "'), ";
  str.erase(str.end() - 2, str.end());
  str += " ]\n";

  str += indent + "signature = (0x" + sig.z.to_string_as_hex_number() + ", [\n";
  for (size_t i = 0; i < sig.elements.size(); ++i)
  {
    const auto& sel = sig.elements[i];
    str += indent + indent + "{\n";
    str += indent + indent + indent + "'H' : { " + points_array_to_dict(sel.H_array) + " },\n";
    str += indent + indent + indent + "'T' : ed25519.Point('" + sel.T.to_string() + "'),\n";
    str += indent + indent + indent + "'T0' : ed25519.Point('" + sel.T0.to_string() + "'),\n";
    str += indent + indent + indent + "'Z' : ed25519.Point('" + sel.Z.to_string() + "'),\n";
    str += indent + indent + indent + "'Z0' : ed25519.Point('" + sel.Z0.to_string() + "'),\n";
    str += indent + indent + indent + "'r' : { " + scalars_array_to_dict(sel.r_array) + " },\n";
    str += indent + indent + indent + "'t' : 0x" + sel.t.to_string_as_hex_number() + ",\n";
    str += indent + indent + indent + "'t0' : 0x" + sel.t0.to_string_as_hex_number() + "\n";
    str += indent + indent + "},\n";
  }
  str.erase(str.end() - 2, str.end());
  str += "\n";
  str += indent + "])\n";
  str += "\n";

  str += indent + "I_reference = [ " + points_array_to_array(I_array) + " ]\n";

  return str;
}

TEST(ml2s, cpp2py)
{
  // Generate a random sig and python code to check it
  scalar_t m;
  m.make_random();
  size_t n = 8;
  size_t N = 1ull << n;
  size_t L = 8;

  // generate a signature

  std::vector<point_t> ring;
  std::vector<scalar_t> secret_keys;
  std::vector<size_t> ring_mapping;
  std::vector<point_t> key_images;
  generate_test_ring_and_sec_keys(N, L, ring, secret_keys, ring_mapping, key_images);

  ml2s_signature sig;
  ASSERT_TRUE(ml2s_lnk_sig_gen(m, ring, secret_keys, ring_mapping, sig));

  ASSERT_TRUE(ml2s_lnk_sig_verif(m, ring, sig, nullptr, &key_images));

  // generate Python code

  std::string str, indent = "    ";
  str += "import ed25519\n";
  str += "import L2S\n";
  str += "\n";
  str += "def check_sig():\n";
  str += ml2s_sig_to_python(ring, sig, key_images);
  str += indent + "m = 0x" + m.to_string_as_hex_number() + "\n";
  str += indent + "result = L2S.mL2SLnkSig_Verif(len(B_array) * 2, B_array, m, signature)\n";
  str += indent + "print(f\"Verif returned : {result}\")\n";
  str += indent + "\n";
  str += indent + "if result != I_reference:\n";
  str += indent + indent + "print(\"ERROR: Key images don't match\")\n";
  str += indent + indent + "return False\n";
  str += indent + "\n";
  str += indent + "return True\n";
  str += "\n";
  str += "if check_sig():\n";
  str += indent + "print(\"Signature verified, key images matched\")\n";

  epee::file_io_utils::save_string_to_file("ml2s_sig_check_test.py", str);

  return true;
}


//
// test's runner
//

bool wildcard_match(const std::string& needle, const std::string& haystack)
{
  size_t h = 0;
  for (size_t n = 0; n < needle.size(); ++n)
  {
    switch (needle[n])
    {
    case '?':
      if (h == haystack.size())
        return false;
      ++h;
      break;
    case '*':
      if (n + 1 == needle.size())
        return true;
      for (size_t i = 0; i + h < haystack.size(); i++)
        if (wildcard_match(needle.substr(n + 1), haystack.substr(h + i)))
          return true;
      return false;
    default:
      if (haystack[h] != needle[n])
        return false;
      ++h;
    }
  }
  return h == haystack.size();
}

int crypto_tests(const std::string& cmd_line_param)
{
  epee::log_space::get_set_log_detalisation_level(true, LOG_LEVEL_3);
  epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_3);
  epee::log_space::log_singletone::add_logger(LOGGER_FILE,
    epee::log_space::log_singletone::get_default_log_file().c_str(),
    epee::log_space::log_singletone::get_default_log_folder().c_str());



  std::vector<size_t> failed_tests;
  for (size_t i = 0; i < g_tests.size(); ++i)
  {
    auto& test = g_tests[i];

    if (!wildcard_match(cmd_line_param.c_str(), test.first.c_str()))
      continue;

    LOG_PRINT("   " << std::setw(40) << std::left << test.first << " >", LOG_LEVEL_0);
    TIME_MEASURE_START(runtime);
    bool r = false;
    try
    {
      r = test.second();
    }
    catch (std::exception& e)
    {
      LOG_PRINT_RED("EXCEPTION: " << e.what(), LOG_LEVEL_0);
    }
    catch (...)
    {
      LOG_PRINT_RED("EXCEPTION: unknown", LOG_LEVEL_0);
    }
    TIME_MEASURE_FINISH(runtime);
    uint64_t runtime_ms = runtime / 1000;
    uint64_t runtime_mcs = runtime % 1000;
    if (r)
    {
      LOG_PRINT_GREEN("   " << std::setw(40) << std::left << test.first << "OK [" << runtime_ms << "." << std::setw(3) << std::setfill('0') << runtime_mcs << " ms]", LOG_LEVEL_0);
    }
    else
    {
      LOG_PRINT_RED(ENDL << "   " << std::setw(40) << std::left << test.first << "FAILED" << ENDL, LOG_LEVEL_0);
      failed_tests.push_back(i);
    }
  }

  if (failed_tests.empty())
  {
    LOG_PRINT_GREEN(ENDL, LOG_LEVEL_0);
    LOG_PRINT_GREEN("All tests passed okay", LOG_LEVEL_0);
    return 0;
  }

  LOG_PRINT_RED_L0(ENDL);
  LOG_PRINT_RED_L0(ENDL << "Failed tests:");
  for (size_t i : failed_tests)
  {
    LOG_PRINT_RED_L0(g_tests[i].first);
  }

  return 1;
}
