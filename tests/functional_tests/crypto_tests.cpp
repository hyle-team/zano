// Copyright (c) 2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define USE_INSECURE_RANDOM_RPNG_ROUTINES // turns on random manupulation for tests
#include <utility>
#include "crypto/crypto.h"
#include "epee/include/misc_log_ex.h"
#include "epee/include/profile_tools.h"
#include "include_base_utils.h"
#include "common/crypto_stream_operators.h"
#include "currency_core/difficulty.h"


extern "C" {
#include "crypto/crypto-ops.h"
} // extern "C"

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
  const char* data = reinterpret_cast<const char*>(&h);
  size_t len = sizeof h;

  std::string s(len * 2, ' ');
  for (size_t i = 0; i < len; ++i) {
    s[2 * i] = hexmap[(data[len - 1 - i] & 0xF0) >> 4];
    s[2 * i + 1] = hexmap[(data[len - 1 - i] & 0x0F)];
  }

  return s;
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

  // copy at most 32 bytes and reduce
  scalar_t(const boost::multiprecision::cpp_int &bigint)
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
    sc_mul(&result.m_s[0], &m_s[0], &v.m_s[0]);
    return result;
  }

  scalar_t& operator*=(const scalar_t& v)
  {
    sc_mul(&m_s[0], &m_s[0], &v.m_s[0]);
    return *this;
  }

  scalar_t reciprocal() const
  {
    scalar_t result;
    sc_invert(&result.m_s[0], &m_s[0]);
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

  // returns a * this + G
  point_t mul_plus_G(const scalar_t& a) const
  {
    static const unsigned char one[32] = { 1 };
    static_assert(sizeof one == sizeof(crypto::ec_scalar), "size missmatch");

    point_t result;
    ge_p2 p2;
    ge_double_scalarmult_base_vartime(&p2, &a.m_s[0], &m_p3, &one[0]);
    ge_p2_to_p3(&result.m_p3, &p2);

    return result;
  }

  // returns a * this + b * G
  point_t mul_plus_G(const scalar_t& a, const scalar_t& b) const
  {
    point_t result;
    ge_p2 p2;
    ge_double_scalarmult_base_vartime(&p2, &a.m_s[0], &m_p3, &b.m_s[0]);
    ge_p2_to_p3(&result.m_p3, &p2);

    return result;
  }

  friend bool operator==(const point_t& lhs, const point_t& rhs)
  {
    // convert to xy form, then compare components (because (z, y, z, t) representation is not unique)
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

    ge_bytes_hash_to_ec(&result.m_p3, (const unsigned char*)&pk);

    return result;
  }

  static point_t hp(const crypto::public_key& p)
  {
    point_t result;
    ge_bytes_hash_to_ec(&result.m_p3, (const unsigned char*)&p);
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
#include "L2S.h"
////////////////////////////////////////////////////////////////////////////////



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
#define ASSERT_EQ(a, b)    CHECK_AND_ASSERT_MES(a == b, false, #a " != " #b)

typedef bool(*bool_func_ptr_t)();
static std::vector<std::pair<std::string, bool_func_ptr_t>> g_tests;
struct test_keeper_t
{
  test_keeper_t(const char* name, bool_func_ptr_t func_p)
  {
    g_tests.push_back(std::make_pair(name, func_p));
  }
};




//
// Tests
//

TEST(crypto, primitives)
{
  struct helper
  {
    static void make_rnd_indicies(std::vector<size_t>& v, size_t size)
    {
      v.resize(size);
      for (size_t i = 0; i < size; ++i)
        v[i] = i;
      std::shuffle(v.begin(), v.end(), crypto::uniform_random_bit_generator());
    };
  };

  struct timer_t
  {
    std::chrono::high_resolution_clock::time_point m_tp{};
    uint64_t m_t{};
    uint64_t m_div_coeff{ 1 };
    void start(uint64_t div_coeff = 1) { m_tp = std::chrono::high_resolution_clock::now(); m_div_coeff = div_coeff; }
    void stop() { m_t = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_tp).count(); }
    uint64_t get_time_mcs() { return m_div_coeff == 1 ? m_t : m_t / m_div_coeff; }
  };

  typedef uint64_t(*run_func_t)(timer_t& t, size_t rounds);



  auto run = [](const std::string& title, size_t rounds, run_func_t cb)
  {
    uint64_t result;
    timer_t t_warmup, t, t_total;
    t_total.start();
    result = cb(t_warmup, rounds);
    result += cb(t, rounds);
    t_total.stop();
    double run_time_mcs_x_100 = double(uint64_t(t.get_time_mcs() / (rounds / 100)));
    LOG_PRINT_L0(std::left << std::setw(50) << title << std::setw(7) << rounds << " rnds -> "
      << std::right << std::setw(7) << std::fixed << std::setprecision(2) << run_time_mcs_x_100 / 100.0 << " mcs avg. (gross: "
      << std::fixed << std::setprecision(2) << double(t_total.get_time_mcs()) / 1000.0 << " ms), result hash: " << result);
  };

#define HASH_64_VEC(vec_var_name) hash_64(vec_var_name.data(), vec_var_name.size() * sizeof(vec_var_name[0]))

  LOG_PRINT_L0(ENDL << "native crypto primitives:");

  run("ge_p3_to_cached(p3)", 10000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<ge_p3> points_p3(rounds);
    ge_scalarmult_base(&points_p3[0], c_scalar_1.data());
    for (size_t i = 1; i < points_p3.size(); ++i)
      ge_bytes_hash_to_ec(&points_p3[i], (const unsigned char*)&points_p3[i - 1].X); // P_{i+1} = Hp(P_i.X)

    std::vector<ge_cached> points_cached(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_p3_to_cached(&points_cached[i], &points_p3[rnd_indecies[i]]);
    }
    t.stop();

    return HASH_64_VEC(points_cached);
  });

  run("ge_add(p3 + p3)", 50000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);
    std::vector<ge_cached> points_cached(rounds);
    point_t p = scalar_t::random() * c_point_G;
    for (size_t i = 0; i < rnd_indecies.size(); ++i)
    {
      ge_p3_to_cached(&points_cached[i], &p.m_p3);
      p = p + p;
    }
    ge_p3 Q;
    ge_scalarmult_base(&Q, &scalar_t::random().m_s[0]);
    std::vector<ge_p1p1> results(rnd_indecies.size());

    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_add(&results[i], &Q, &points_cached[rnd_indecies[i]]);
    }
    t.stop();

    return HASH_64_VEC(results);
  });

  run("ge_p1p1_to_p3(p1p1)", 50000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    ge_cached G;
    ge_p3_to_cached(&G, &c_point_G.m_p3);

    std::vector<ge_p1p1> points_p1p1(rounds);
    ge_add(&points_p1p1[0], &c_point_G.m_p3, &G);
    for (size_t i = 1; i < points_p1p1.size(); ++i)
    {
      ge_p3 p3;
      ge_p1p1_to_p3(&p3, &points_p1p1[i - 1]);
      ge_add(&points_p1p1[i], &p3, &G);
    }

    std::vector<ge_p3> points_p3(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_p1p1_to_p3(&points_p3[i], &points_p1p1[rnd_indecies[i]]);
    }
    t.stop();

    return HASH_64_VEC(points_p3);
  });

  run("ge_scalarmult()", 5000, [](timer_t& t, size_t rounds) {
    //rounds -= rounds % 8;
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    scalar_t x;
    x.make_random();

    std::vector<crypto::ec_scalar> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
    {
      //scalar_t x = x + x + x;
      scalar_t x;
      x.make_random();
      memcpy(&scalars[i].data, x.data(), 32);
    }

    point_t p = scalar_t::random() * c_point_G;

    //std::vector<ge_p2> points_p2(rounds);
    std::vector<ge_p3> points_p3(rounds);

    // warmup round
    //for (size_t i = 0; i < rounds; ++i)
    //  ge_scalarmult((ge_p2*)&points_p3[i], (const unsigned char*)&scalars[rnd_indecies[i]], &p.m_p3);

    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_scalarmult((ge_p2*)&points_p3[i], (const unsigned char*)&scalars[rnd_indecies[i]], &p.m_p3);
      //ge_scalarmult(&points_p2[i * 4 + 3], (const unsigned char*)&scalars[rnd_indecies[i * 4 + 3]], &p.m_p3);
      //ge_scalarmult(&points_p2[i * 4 + 0], (const unsigned char*)&scalars[rnd_indecies[i * 4 + 0]], &p.m_p3);
      //ge_scalarmult(&points_p2[i * 4 + 1], (const unsigned char*)&scalars[rnd_indecies[i * 4 + 1]], &p.m_p3);
      //ge_scalarmult(&points_p2[i * 4 + 2], (const unsigned char*)&scalars[rnd_indecies[i * 4 + 2]], &p.m_p3);
    }
    t.stop();

    return HASH_64_VEC(points_p3);
  });

  run("ge_scalarmult() (2)", 5000, [](timer_t& t, size_t rounds) {
    //rounds -= rounds % 8;
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    scalar_t x;
    x.make_random();

    std::vector<crypto::ec_scalar> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
    {
      //scalar_t x = x + x + x;
      scalar_t x;
      x.make_random();
      memcpy(&scalars[i].data, x.data(), 32);
    }

    point_t p = scalar_t::random() * c_point_G;

    //std::vector<ge_p2> points_p2(rounds);
    std::vector<ge_p3> points_p3(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_scalarmult((ge_p2*)&points_p3[i], (const unsigned char*)&scalars[rnd_indecies[i]], &p.m_p3);
      //ge_scalarmult(&points_p2[i * 4 + 3], (const unsigned char*)&scalars[rnd_indecies[i * 4 + 3]], &p.m_p3);
      //ge_scalarmult(&points_p2[i * 4 + 0], (const unsigned char*)&scalars[rnd_indecies[i * 4 + 0]], &p.m_p3);
      //ge_scalarmult(&points_p2[i * 4 + 1], (const unsigned char*)&scalars[rnd_indecies[i * 4 + 1]], &p.m_p3);
      //ge_scalarmult(&points_p2[i * 4 + 2], (const unsigned char*)&scalars[rnd_indecies[i * 4 + 2]], &p.m_p3);
    }
    t.stop();

    return HASH_64_VEC(points_p3);
  });

  run("ge_scalarmult_p3()", 5000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    scalar_t x;
    x.make_random();

    std::vector<crypto::ec_scalar> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
    {
      //scalar_t x = x + x + x;
      scalar_t x;
      x.make_random();
      memcpy(&scalars[i].data, x.data(), 32);
    }

    point_t p = scalar_t::random() * c_point_G;

    std::vector<ge_p3> points_p3(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_scalarmult_p3(&points_p3[i], (const unsigned char*)&scalars[rnd_indecies[i]], &p.m_p3);
    }
    t.stop();

    return HASH_64_VEC(points_p3);
  });

  run("ge_scalarmult_vartime_p3()", 5000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    scalar_t x;
    x.make_random();

    std::vector<crypto::ec_scalar> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
    {
      //scalar_t x = x + x + x;
      scalar_t x;
      x.make_random();
      memcpy(&scalars[i].data, x.data(), 32);
    }

    point_t p = scalar_t::random() * c_point_G;

    //memcpy(&scalars[rnd_indecies[0]], scalar_t(1).data(), 32);

    std::vector<ge_p3> points_p3(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_scalarmult_vartime_p3(&points_p3[i], (const unsigned char*)&scalars[rnd_indecies[i]], &p.m_p3);
    }
    t.stop();

    return HASH_64_VEC(points_p3);
  });

  run("ge_scalarmult_vartime_p3_v2()", 5000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    scalar_t x;
    x.make_random();

    std::vector<crypto::ec_scalar> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
    {
      //scalar_t x = x + x + x;
      scalar_t x;
      x.make_random();
      memcpy(&scalars[i].data, x.data(), 32);
    }

    point_t p = scalar_t::random() * c_point_G;

    std::vector<ge_p3> points_p3(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_scalarmult_vartime_p3_v2(&points_p3[i], (const unsigned char*)&scalars[rnd_indecies[i]], &p.m_p3);
    }
    t.stop();

    return HASH_64_VEC(points_p3);
  });

  run("ge_scalarmult_base()", 5000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    scalar_t x;
    x.make_random();

    std::vector<crypto::ec_scalar> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
    {
      scalar_t x = x + x + x;
      memcpy(&scalars[i].data, x.data(), 32);
    }

    std::vector<ge_p3> points_p3(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_scalarmult_base(&points_p3[i], (const unsigned char*)&scalars[rnd_indecies[i]]);
    }
    t.stop();

    return HASH_64_VEC(points_p3);
  });



  LOG_PRINT_L0(ENDL << "new primitives:");

  run("point_t + point_t", 50000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<point_t> points(rounds);
    point_t p = c_point_G;
    for (size_t i = 0; i < rounds; ++i)
    {
      points[i] = p;
      p = p + p;
    }

    std::vector<point_t> result(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      result[i] = points[rnd_indecies[i]] + p;
    }
    t.stop();

    return HASH_64_VEC(result);
  });

  run("sclar_t * point_t", 5000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<scalar_t> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
      scalars[i].make_random();

    point_t p = scalar_t::random() * c_point_G;

    std::vector<point_t> result(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      result[i] = scalars[rnd_indecies[i]] * p;
    }
    t.stop();

    return HASH_64_VEC(result);
  });

  run("sclar_t * point_g_t", 5000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<scalar_t> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
      scalars[i].make_random();

    std::vector<point_t> result(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      result[i] = scalars[rnd_indecies[i]] * c_point_G;
    }
    t.stop();

    return HASH_64_VEC(result);
  });

  run("sclar_t * scalar_t", 50000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<scalar_t> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
      scalars[i].make_random();

    scalar_t s = scalar_t::random();

    std::vector<scalar_t> result(rounds);
    t.start(4);
    for (size_t i = 0; i < rounds; ++i)
    {
      result[i] = scalars[rnd_indecies[i]] * s * s * s * s;
    }
    t.stop();

    return HASH_64_VEC(result);
  });

  run("sclar_t / scalar_t", 10000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<scalar_t> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
      scalars[i].make_random();

    scalar_t s = scalar_t::random();

    std::vector<scalar_t> result(rounds);
    t.start(2);
    for (size_t i = 0; i < rounds; ++i)
    {
      result[i] = scalars[rnd_indecies[i]] / s / s;
    }
    t.stop();

    return HASH_64_VEC(result);
  });

  run("mul_plus_G", 2000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<point_t> points(rounds);
    point_t p = c_point_G;
    for (size_t i = 0; i < rounds; ++i)
    {
      points[i] = p;
      p = p + p;
    }

    scalar_t a, b;
    a.make_random();
    b.make_random();

    std::vector<point_t> result(rounds);
    t.start(2);
    for (size_t i = 0; i < rounds; ++i)
    {
      result[i] = points[rnd_indecies[i]].mul_plus_G(a, b).mul_plus_G(a, b);
    }
    t.stop();

    return HASH_64_VEC(result);
  });

  return true;
}
// test crypto_primitives

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
  scalar_t z = 0;
  for (size_t j = 0; j < 1000; ++j)
  {
    z.make_random();
    ASSERT_FALSE(z.is_zero());
    ASSERT_TRUE(z > z - 1);
    ASSERT_TRUE(z < z + 1);
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
