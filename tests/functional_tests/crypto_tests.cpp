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

// out = z ^ -1 (= z ^ (L - 2) according to Fermat little theorem)
void sc_invert(unsigned char* out, const unsigned char* z)
{
  memcpy(out, z, sizeof(crypto::ec_scalar));
  for (size_t i = 0; i < 128; ++i)
    sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
  sc_mul(out, out, out);
  sc_mul(out, out, z);
}


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
  //fe m_fe; // 40 bytes, array 10 * 4, optimized form
  union
  {
    uint64_t      m_u64[4];
    unsigned char m_s[32];
  };
  // DONE! consider 1) change to aligned array of unsigned chars
  // consider 2) add 32 byte after to speed up sc_reduce by decreasing num of copy operations

  scalar_t()
  {}

  scalar_t(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3)
  {
    m_u64[0] = a0;
    m_u64[1] = a1;
    m_u64[2] = a2;
    m_u64[3] = a3;
  }

  scalar_t(int64_t v)
  {
    zero();
    if (v == 0)
    {
      return;
    }
    //unsigned char bytes[32] = {0};
    reinterpret_cast<int64_t&>(m_s) = v;
    //fe_frombytes(m_fe, bytes);
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

  operator crypto::secret_key() const
  {
    crypto::secret_key result;
    memcpy(result.data, &m_s, sizeof result.data);
    //fe_tobytes(reinterpret_cast<unsigned char*>(&result), m_fe);
    return result;
  }

  bool from_secret_key(const crypto::secret_key& sk)
  {
    //fe_frombytes(m_fe, reinterpret_cast<const unsigned char*>(&sk));
    return false;
  }

  void zero()
  {
    //fe_0(m_fe);
    m_u64[0] = 0;
    m_u64[1] = 0;
    m_u64[2] = 0;
    m_u64[3] = 0;
    //memset(&m_s, 0, sizeof m_s);
  }

  void make_random()
  {
    unsigned char tmp[64];
    crypto::generate_random_bytes(64, tmp);
    sc_reduce(tmp);
    memcpy(&m_s, tmp, 32);
  }

  bool is_zero() const
  {
    return sc_isnonzero(&m_s[0]) == 0;
  }

  scalar_t operator+(const scalar_t& v) const
  {
    scalar_t result;
    sc_add(reinterpret_cast<unsigned char*>(&result), reinterpret_cast<const unsigned char*>(&m_s), reinterpret_cast<const unsigned char*>(&v));
    return result;
  }

  scalar_t operator-(const scalar_t& v) const
  {
    scalar_t result;
    sc_sub(reinterpret_cast<unsigned char*>(&result), reinterpret_cast<const unsigned char*>(&m_s), reinterpret_cast<const unsigned char*>(&v));
    return result;
  }

  scalar_t operator*(const scalar_t& v) const
  {
    scalar_t result;
    sc_mul(reinterpret_cast<unsigned char*>(&result), reinterpret_cast<const unsigned char*>(&m_s), reinterpret_cast<const unsigned char*>(&v));
    return result;
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

  bool operator==(const scalar_t& rhs) const
  {
    return
      m_u64[0] == rhs.m_u64[0] &&
      m_u64[1] == rhs.m_u64[1] &&
      m_u64[2] == rhs.m_u64[2] &&
      m_u64[3] == rhs.m_u64[3];
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

  operator crypto::public_key()
  {
    crypto::public_key result;
    ge_p3_tobytes((unsigned char*)&result, &m_p3);
    return result;
  }

  point_t operator+(const point_t& rhs)
  {
    point_t result;
    ge_cached rhs_c;
    ge_p1p1 t;
    ge_p3_to_cached(&rhs_c, &rhs.m_p3);
    ge_add(&t, &m_p3, &rhs_c);
    ge_p1p1_to_p3(&result.m_p3, &t);
    return result;
  }

  point_t operator-(const point_t& rhs)
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
}; // struct point_t

struct point_g_t : public point_t
{
  point_g_t()
  {

  }

  friend point_t operator*(const scalar_t& lhs, const point_g_t&)
  {
    point_t result;
    ge_scalarmult_base(&result.m_p3, reinterpret_cast<const unsigned char*>(&lhs));
    return result;
  }

  /*friend point_t operator*(const int64_t lhs, const point_g_t& rhs)
  {
  return operator*(scalar_t)
  }*/

  static_assert(sizeof(crypto::public_key) == 32, "size error");

}; // struct point_g_t

static const point_g_t point_G;

static const scalar_t scalar_L      = { 0x5812631a5cf5d3ed, 0x14def9dea2f79cd6, 0x0,                0x1000000000000000 };
static const scalar_t scalar_Lm1    = { 0x5812631a5cf5d3ec, 0x14def9dea2f79cd6, 0x0,                0x1000000000000000 };
static const scalar_t scalar_P      = { 0xffffffffffffffed, 0xffffffffffffffff, 0xffffffffffffffff, 0x7fffffffffffffff };
static const scalar_t scalar_Pm1    = { 0xffffffffffffffec, 0xffffffffffffffff, 0xffffffffffffffff, 0x7fffffffffffffff };
static const scalar_t scalar_256m1  = { 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff };


/*
* tiny facade to gtest-alike interface to make simplier further tests transfer to unit_tests
*/
#define TEST(test_name_a, test_name_b) \
  static bool test_name_a ## _ ## test_name_b(); \
  static test_keeper_t test_name_a ## _ ## test_name_b ## keeper(STR(COMBINE(test_name_a ## _, test_name_b)), & test_name_a ## _ ## test_name_b); \
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

  ASSERT_TRUE(scalar_L > 0 && !(scalar_L < 0));
  ASSERT_TRUE(scalar_Lm1 > 0 && !(scalar_Lm1 < 0));
  ASSERT_TRUE(scalar_Lm1 < scalar_L);
  ASSERT_FALSE(scalar_Lm1 > scalar_L);
  ASSERT_TRUE(scalar_P > scalar_Pm1);
  ASSERT_FALSE(scalar_P < scalar_Pm1);

  std::cout << "0   = " << zero << std::endl;
  std::cout << "1   = " << one << std::endl;
  std::cout << "L   = " << scalar_L << std::endl;
  std::cout << "Lm1 = " << scalar_Lm1 << std::endl;
  std::cout << "P   = " << scalar_P << std::endl;
  std::cout << "Pm1 = " << scalar_Pm1 << std::endl;

  // check rolling over L for scalars arithmetics
  ASSERT_EQ(scalar_Lm1 + 1, 0);
  ASSERT_EQ(scalar_t(0) - 1, scalar_Lm1);
  ASSERT_EQ(scalar_Lm1 * 2, scalar_Lm1 - 1); // (L - 1) * 2 = L + L - 2 = (L - 1) - 1  (mod L)
  ASSERT_EQ(scalar_Lm1 * 100, scalar_Lm1 - 99);
  ASSERT_EQ(scalar_Lm1 * scalar_Lm1, 1);     // (L - 1) * (L - 1) = L*L - 2L + 1 = 1 (mod L)

  std::cout << std::endl;



  scalar_t a = 2;
  a = a / 2;
  std::cout << "2 / 2       = " << a << std::endl;
  a = scalar_Lm1 / 2;
  std::cout << "L-1 / 2     = " << a << std::endl;
  a = a * 2;
  std::cout << "L-1 / 2 * 2 = " << a << std::endl;

  return true;
}

TEST(crypto, point_basics)
{
  scalar_t s = 4;
  point_t E = s * point_G;
  point_t X = 4 * E;
  point_t K = 193847 * point_G;
  point_t C = E + K;

  ASSERT_TRUE(X == 16 * point_G);
  ASSERT_TRUE(C - K == E);
  ASSERT_TRUE(C - E == K);
  ASSERT_TRUE(C == 193851 * point_G);

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
    bool r = test.second();
    if (r)
    {
      LOG_PRINT_GREEN("  " << std::setw(40) << std::left << test.first << "OK", LOG_LEVEL_0);
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

  LOG_PRINT_RED_L0(ENDL, LOG_LEVEL_0);
  LOG_PRINT_RED_L0(ENDL << "Failed tests:");
  for (size_t i : failed_tests)
  {
    LOG_PRINT_RED_L0(g_tests[i].first);
  }

  return 1;
}
