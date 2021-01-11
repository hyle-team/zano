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


extern "C" {
#include "crypto/crypto-ops.h"
} // extern "C"

unsigned char Lm2[32] = { 0xeb, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58, 0xd6, 0x9c, 0xf7, 0xa2, 0xde, 0xf9, 0xde, 0x14, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x10 };

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


__declspec(align(32))
struct scalar_t
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

  static scalar_t random()
  {
    unsigned char tmp[64];
    crypto::generate_random_bytes(64, tmp);
    sc_reduce(tmp);
    scalar_t result;
    memcpy(&result.m_s, tmp, sizeof result.m_s);
    return result;
  }

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

  void zero()
  {
    ge_p3_0(&m_p3);
  }

  bool from_public_key(const crypto::public_key& pk)
  {
    return ge_frombytes_vartime(&m_p3, reinterpret_cast<const unsigned char*>(&pk)) == 0;
  }

  operator crypto::public_key() const
  {
    crypto::public_key result;
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
    crypto::public_key pk;
    ge_p3_tobytes((unsigned char*)&pk, &v.m_p3);
    return ss << epee::string_tools::pod_to_hex(pk);
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

static const scalar_t c_scalar_L      = { 0x5812631a5cf5d3ed, 0x14def9dea2f79cd6, 0x0,                0x1000000000000000 };
static const scalar_t c_scalar_Lm1    = { 0x5812631a5cf5d3ec, 0x14def9dea2f79cd6, 0x0,                0x1000000000000000 };
static const scalar_t c_scalar_P      = { 0xffffffffffffffed, 0xffffffffffffffff, 0xffffffffffffffff, 0x7fffffffffffffff };
static const scalar_t c_scalar_Pm1    = { 0xffffffffffffffec, 0xffffffffffffffff, 0xffffffffffffffff, 0x7fffffffffffffff };
static const scalar_t c_scalar_256m1  = { 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff };



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

    for (size_t i = 0; i < decoy_set_size; ++i)
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
  size_t n = 1000;
  size_t decoy_set = 2;

  std::vector<sig_check_t> sigs;
  sigs.resize(n);

  for (size_t i = 0; i < sigs.size(); ++i)
    sigs[i].prepare_random_data(decoy_set);

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


int crypto_tests()
{
  epee::log_space::get_set_log_detalisation_level(true, LOG_LEVEL_1);
  epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_2);
  epee::log_space::log_singletone::add_logger(LOGGER_FILE,
    epee::log_space::log_singletone::get_default_log_file().c_str(),
    epee::log_space::log_singletone::get_default_log_folder().c_str());



  std::vector<size_t> failed_tests;
  for (size_t i = 0; i < g_tests.size(); ++i)
  {
    auto& test = g_tests[i];
    TIME_MEASURE_START(runtime);
    bool r = test.second();
    TIME_MEASURE_FINISH(runtime);
    uint64_t runtime_ms = runtime / 1000;
    uint64_t runtime_mcs = runtime % 1000;
    if (r)
    {
      LOG_PRINT_GREEN("  " << std::setw(40) << std::left << test.first << "OK [" << runtime_ms << "." << std::setw(3) << std::setfill('0') << runtime_mcs << " ms]", LOG_LEVEL_0);
    }
    else
    {
      LOG_PRINT_RED(ENDL << "  " << std::setw(40) << std::left << test.first << "FAILED" << ENDL, LOG_LEVEL_0);
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
