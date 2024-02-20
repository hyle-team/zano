// Copyright (c) 2020-2023 Zano Project
// Copyright (c) 2020-2023 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define USE_INSECURE_RANDOM_RPNG_ROUTINES // turns on random manupulation for tests
#include <utility>
#include <boost/multiprecision/cpp_int.hpp>
#include "crypto/crypto.h"
#include "epee/include/misc_log_ex.h"
#include "epee/include/profile_tools.h"
#include "include_base_utils.h"
#include "common/crypto_stream_operators.h"
#include "common/varint.h"
#include "common/boost_serialization_helper.h"
#include "currency_core/difficulty.h"
#include "currency_core/currency_basic.h"

#include "crypto/crypto-sugar.h"
#include "crypto/range_proofs.h"
#include "../core_tests/random_helper.h"
#include "crypto_torsion_elements.h"

using namespace crypto;

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

// out = z ^ -1 (= z ^ (L - 2) according to Fermat little theorem)
void sc_invert2(unsigned char* out, const unsigned char* z)
{
  memcpy(out, z, 32);
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

extern void *sha3(const void *in, size_t inlen, void *md, int mdlen);


uint64_t rand_in_range(uint64_t from_including, uint64_t to_not_including)
{
  uint64_t result = 0;
  crypto::generate_random_bytes(sizeof result, &result);
  return from_including + result % (to_not_including - from_including);
}




static const fe scalar_L_fe = { 16110573, 10012311, -6632702, 16062397, 5471207, 0, 0, 0, 0, 4194304 };






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
#define ASSERT_TRUE(expr)  CHECK_AND_ASSERT_MES(expr,             false, "This is not true: " #expr << " @ " << LOCATION_SS)
#define ASSERT_FALSE(expr) CHECK_AND_ASSERT_MES((expr) == false,  false, "This is not false: " #expr << " @ " << LOCATION_SS)
#define ASSERT_EQ(a, b)    CHECK_AND_ASSERT_MES(a == b,           false, #a " != " #b "\n    " << a << " != " << b << " @ " << LOCATION_SS)
#define ASSERT_NEQ(a, b)   CHECK_AND_ASSERT_MES(a != b,           false, #a " == " #b "\n    " << a << " @ " << LOCATION_SS)

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
// #include "crypto_tests_ml2s.h"
#include "crypto_tests_range_proofs.h"
#include "crypto_tests_clsag.h"
#include "crypto_tests_one_out_of_many_proofs.h"
////////////////////////////////////////////////////////////////////////////////



//
// Tests
//

TEST(crypto, constants)
{
  //
  // field elements
  //
  scalar_t zero = {0, 0, 0, 0};
  scalar_t one  = {1, 0, 0, 0};
  ASSERT_TRUE(zero.is_zero());
  ASSERT_FALSE(one.is_zero());
  ASSERT_TRUE(one > zero);
  scalar_t r = scalar_t::random();
  ASSERT_EQ(r * zero, zero);
  ASSERT_EQ(r * one, r);
  ASSERT_EQ(c_scalar_0, zero);
  ASSERT_EQ(c_scalar_1, one);

  ASSERT_EQ(c_scalar_2p64 - c_scalar_1, scalar_t(UINT64_MAX));
  ASSERT_EQ(c_scalar_2p64, scalar_t(UINT64_MAX) + c_scalar_1);
  ASSERT_EQ(c_scalar_2p64, scalar_t::power_of_2(64));

  ASSERT_EQ(zero + c_scalar_L, zero);
  ASSERT_EQ(c_scalar_L + zero, zero);
  ASSERT_EQ(r + c_scalar_L, r);
  ASSERT_EQ(c_scalar_L + r, r);

  ASSERT_EQ(c_scalar_Lm1 + c_scalar_1, c_scalar_0);
  ASSERT_EQ(c_scalar_0 - c_scalar_1, c_scalar_Lm1);

  ASSERT_EQ(c_scalar_P.as_boost_mp_type<mp::uint256_t>(), scalar_t::power_of_2(255).as_boost_mp_type<mp::uint256_t>() - 19);

  ASSERT_EQ(c_scalar_P.as_boost_mp_type<mp::uint256_t>() - c_scalar_Pm1.as_boost_mp_type<mp::uint256_t>(), mp::uint256_t(1));
  ASSERT_EQ(c_scalar_Pm1 + r, c_scalar_P - c_scalar_1 + r);

  ASSERT_EQ(c_scalar_256m1.as_boost_mp_type<mp::uint256_t>(), scalar_t::power_of_2(255).as_boost_mp_type<mp::uint512_t>() * 2 - 1);

  ASSERT_EQ(c_scalar_1div8 * scalar_t(8), c_scalar_1);
  ASSERT_NEQ(c_scalar_1div8, c_scalar_1);
  ASSERT_NEQ(c_scalar_1div8 * scalar_t(2), c_scalar_1);
  ASSERT_NEQ(c_scalar_1div8 * scalar_t(4), c_scalar_1);

  LOG_PRINT_L0("0         = " << c_scalar_0       << " = { " << pod_to_hex_comma_separated_uint64(c_scalar_0) << " }");
  LOG_PRINT_L0("1         = " << c_scalar_1       << " = { " << pod_to_hex_comma_separated_uint64(c_scalar_1) << " }");
  LOG_PRINT_L0("2^64      = " << c_scalar_2p64    << " = { " << pod_to_hex_comma_separated_uint64(c_scalar_2p64) << " }");
  LOG_PRINT_L0("L         = " << c_scalar_L       << " = { " << pod_to_hex_comma_separated_uint64(c_scalar_L) << " }");
  LOG_PRINT_L0("L - 1     = " << c_scalar_Lm1     << " = { " << pod_to_hex_comma_separated_uint64(c_scalar_Lm1) << " }");
  LOG_PRINT_L0("P         = " << c_scalar_P       << " = { " << pod_to_hex_comma_separated_uint64(c_scalar_P) << " }");
  LOG_PRINT_L0("P - 1     = " << c_scalar_Pm1     << " = { " << pod_to_hex_comma_separated_uint64(c_scalar_Pm1) << " }");
  LOG_PRINT_L0("2^256 - 1 = " << c_scalar_256m1   << " = { " << pod_to_hex_comma_separated_uint64(c_scalar_256m1) << " }");
  LOG_PRINT_L0("1/8       = " << c_scalar_1div8   << " = { " << pod_to_hex_comma_separated_uint64(c_scalar_1div8) << " }");
  LOG_PRINT_L0(ENDL);


  //
  // group elements
  //

  // calculate all generators manually using on external constants
  point_t p_0{};
  ge_p3_0(&p_0.m_p3);

  point_t G{};
  ge_scalarmult_base(&G.m_p3, c_scalar_1.data());

  point_t H   = hash_helper_t::hp(G);
  point_t H2  = hash_helper_t::hp("H2_generator");
  point_t U   = hash_helper_t::hp("U_generator");
  point_t X   = hash_helper_t::hp("X_generator");
  point_t HpG = H + G;
  point_t HmG = H - G;
  point_t Hd8 = c_scalar_1div8 * H;
  
  ASSERT_EQ(scalar_t(8) * Hd8, H);

  // print them
  LOG_PRINT_L0("0     = " << p_0  << " = { " << p_0.to_comma_separated_int32_str() << " }");
  LOG_PRINT_L0("G     = " << G    << " = { " << G.to_comma_separated_int32_str() << " }");
  LOG_PRINT_L0("H     = " << H    << " = { " << H.to_comma_separated_int32_str() << " }");
  LOG_PRINT_L0("      = { " << pod_to_comma_separated_chars(H.to_public_key()) << " }");
  LOG_PRINT_L0("H2    = " << H2   << " = { " << H2.to_comma_separated_int32_str() << " }");
  LOG_PRINT_L0("U     = " << U    << " = { " << U.to_comma_separated_int32_str() << " }");
  LOG_PRINT_L0("X     = " << X    << " = { " << X.to_comma_separated_int32_str() << " }");
  LOG_PRINT_L0("H+G   = " << HpG  << " = { " << HpG.to_comma_separated_int32_str() << " }");
  LOG_PRINT_L0("H-G   = " << HmG  << " = { " << HmG.to_comma_separated_int32_str() << " }");
  LOG_PRINT_L0("H / 8 = " << Hd8  << " = { " << pod_to_hex_comma_separated_bytes(Hd8.to_public_key()) << " }");
  LOG_PRINT_L0("      = { " << pod_to_comma_separated_chars(Hd8.to_public_key()) << " }");

  // check constants
  ASSERT_EQ(p_0,    c_point_0);
  ASSERT_EQ(G,      c_point_G);
  ASSERT_EQ(H,      c_point_H);
  ASSERT_EQ(H2,     c_point_H2);
  ASSERT_EQ(U,      c_point_U);
  ASSERT_EQ(X,      c_point_X);
  ASSERT_EQ(HpG,    c_point_H_plus_G);
  ASSERT_EQ(HmG,    c_point_H_minus_G);

  ASSERT_EQ(currency::native_coin_asset_id,    c_point_H.to_public_key());
  ASSERT_EQ(currency::native_coin_asset_id_pt, c_point_H);
  ASSERT_EQ(currency::native_coin_asset_id_pt.to_public_key(), currency::native_coin_asset_id);
  ASSERT_EQ(currency::native_coin_asset_id_1div8, Hd8.to_public_key());

  return true;
}



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

TEST(crypto, pos)
{
  //scalar_t D = 10000000000000001u;
  scalar_t D = 13042196742415129u; // prime number
  currency::wide_difficulty_type D_w = D.as_boost_mp_type<boost::multiprecision::uint256_t>().convert_to<currency::wide_difficulty_type>();
  uint64_t amount = 1000000000000;
  size_t count_old = 0;
  size_t count_new = 0;
  size_t count_3 = 0;
  scalar_t x;
  x.make_random();

  const boost::multiprecision::uint512_t c_2_pow_256_m1(std::numeric_limits<boost::multiprecision::uint256_t>::max());


  const boost::multiprecision::uint256_t c_L_w = c_scalar_L.as_boost_mp_type<boost::multiprecision::uint256_t>();
  const boost::multiprecision::uint256_t c_L_div_D_w = c_L_w / D_w;
  boost::multiprecision::uint512_t h_tres = c_L_div_D_w * amount;

  currency::wide_difficulty_type final_diff = D_w / amount;

  boost::multiprecision::uint512_t Lv = boost::multiprecision::uint512_t(c_L_w) * amount;

  constexpr uint64_t c_coin = 1000000000000;
  const uint64_t amounts[] = {
    c_coin / 100,
    c_coin / 50,
    c_coin / 20,
    c_coin / 10,
    c_coin / 5,
    c_coin / 2,
    c_coin * 1,
    c_coin * 2,
    c_coin * 5,
    c_coin * 10,
    c_coin * 20,
    c_coin * 50,
    c_coin * 100,
    c_coin * 200,
    c_coin * 500,
    c_coin * 1000,
    c_coin * 2000,
    c_coin * 5000,
    c_coin * 10000,
    c_coin * 20000,
    c_coin * 50000,
    c_coin * 100000,
    c_coin * 200000,
    c_coin * 500000
  };

  uint64_t kernel = 0;
  scalar_t d0 = 0;
  uint64_t d1 = 0;


  /*
  for (size_t i = 0, size = sizeof pos_diffs / sizeof pos_diffs[0]; i < size; ++i)
  {
    auto& D = pos_diffs[i].difficulty;
    boost::multiprecision::uint256_t L_dvi_D = c_L_w / D;

    for (size_t j = 0, size_j = sizeof amounts / sizeof amounts[0]; j < size_j; ++j)
    {
      uint64_t amount = rand_in_range(amounts[j], amounts[j] + amounts[j] / 10);
      size_t iter = find_pos_hash(L_dvi_D, D, amount, kernel, d0, d1);
      LOG_PRINT_L0(i << ", " << amount << ", " << iter);
    }
  }
  */

  return true;
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
  ASSERT_EQ(c_scalar_1.muladd(c_scalar_0, c_scalar_0), c_scalar_0);

  ASSERT_EQ(-c_scalar_1, c_scalar_Lm1);
  ASSERT_EQ(-c_scalar_1, scalar_t(0) - c_scalar_1);
  ASSERT_EQ(-c_scalar_0, c_scalar_0);
  ASSERT_EQ(-c_scalar_Lm1, c_scalar_1);

  scalar_t z = 0;
  for (size_t j = 0; j < 1000; ++j)
  {
    z.make_random();
    ASSERT_FALSE(z.is_zero());
    ASSERT_TRUE(z.is_reduced());
    ASSERT_TRUE(z > z - 1);
    ASSERT_TRUE(z < z + 1);
    ASSERT_TRUE(z.muladd(c_scalar_1, c_scalar_0) == z);
    ASSERT_TRUE(z.muladd(c_scalar_0, c_scalar_1) == c_scalar_1);
    ASSERT_TRUE(z.muladd(z, z) == z * z + z);
  }

  ASSERT_TRUE(c_scalar_L > 0 && !(c_scalar_L < 0));
  ASSERT_TRUE(c_scalar_Lm1 > 0 && !(c_scalar_Lm1 < 0));
  ASSERT_TRUE(c_scalar_Lm1 < c_scalar_L);
  ASSERT_FALSE(c_scalar_Lm1 > c_scalar_L);
  ASSERT_TRUE(c_scalar_P > c_scalar_Pm1);
  ASSERT_FALSE(c_scalar_P < c_scalar_Pm1);

  // check rolling over L for scalars arithmetics
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
  ASSERT_FALSE(c_scalar_P.is_reduced());
  ASSERT_FALSE(c_scalar_Pm1.is_reduced());
  ASSERT_FALSE(c_scalar_256m1.is_reduced());


  p.make_random();
  z.make_random();
  ASSERT_EQ(scalar_t().assign_muladd(z, z, p), p + z * z);
  ASSERT_EQ(scalar_t().assign_muladd(z, p, z), z + z * p);
  ASSERT_EQ(scalar_t().assign_mulsub(z, z, p), p - z * z);
  ASSERT_EQ(scalar_t().assign_mulsub(z, p, z), z - z * p);

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

TEST(crypto, cn_fast_hash_perf)
{
  //return true;
  const crypto::hash h_initial = *(crypto::hash*)(&scalar_t::random());

  std::vector<std::vector<uint8_t>> test_data;
  test_data.push_back(std::vector<uint8_t>(32, 0));
  test_data.push_back(std::vector<uint8_t>(63, 0));
  test_data.push_back(std::vector<uint8_t>(127, 0));
  test_data.push_back(std::vector<uint8_t>(135, 0));
  test_data.push_back(std::vector<uint8_t>(255, 0));
  test_data.push_back(std::vector<uint8_t>(271, 0)); // 271 = 136 * 2 - 1
  test_data.push_back(std::vector<uint8_t>(2030, 0));

  for (size_t j = 0, sz = test_data.size(); j < sz; ++j)
    crypto::generate_random_bytes(test_data[j].size(), test_data[j].data());

  struct times_t
  {
    uint64_t t_old{ 0 }, t_new{ 0 };
    crypto::hash h_old{};
    double diff{ 0 };
  };
  std::vector<times_t> results(test_data.size());

  size_t n = 50000;
  double diff_sum = 0;

  for (size_t k = 0; k < 50; ++k)
  {
    for (size_t j = 0, sz = test_data.size(); j < sz; ++j)
    {
      crypto::hash h = h_initial;
      TIME_MEASURE_START(t_old);
      for (size_t i = 0; i < n; ++i)
      {
        *(crypto::hash*)(test_data[j].data()) = h;
        cn_fast_hash_old(test_data[j].data(), test_data[j].size(), (char*)&h);
      }
      TIME_MEASURE_FINISH(t_old);
      results[j].t_old = t_old;
      results[j].h_old = h;
    }

    for (size_t j = 0, sz = test_data.size(); j < sz; ++j)
    {
      crypto::hash h = h_initial;
      TIME_MEASURE_START(t_new);
      for (size_t i = 0; i < n; ++i)
      {
        *(crypto::hash*)(test_data[j].data()) = h;
        cn_fast_hash(test_data[j].data(), test_data[j].size(), (char*)&h);
      }
      TIME_MEASURE_FINISH(t_new);
      results[j].t_new = t_new;
      ASSERT_EQ(h, results[j].h_old);
    }

    std::stringstream ss;
    double diff_round = 0;
    for (size_t j = 0, sz = test_data.size(); j < sz; ++j)
    {
      double diff = ((int64_t)results[j].t_old - (int64_t)results[j].t_new) / (double)n;

      ss << std::fixed << std::setprecision(3) << results[j].t_old / (double)n << "/" <<
        std::fixed << std::setprecision(3) << results[j].t_new / (double)n << "  ";

      results[j].diff += diff;
      diff_round += diff;
    }

    diff_sum += diff_round;

    LOG_PRINT_L0("cn_fast_hash (old/new) [" << std::setw(2) << k << "]: " << ss.str() << " mcs, diff_round = " << std::fixed << std::setprecision(4) << diff_round <<
     " diff_sum = " << std::fixed << std::setprecision(4) << diff_sum);
  }

  std::stringstream ss;
  for (size_t j = 0, sz = results.size(); j < sz; ++j)
    ss << std::fixed << std::setprecision(4) << results[j].diff << "       ";
  LOG_PRINT_L0("                             " << ss.str());

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

TEST(crypto, neg_identity)
{
  point_t z = c_point_0;                  // 0 group element (identity)
  public_key z_pk = z.to_public_key();    // pub key, corresponding to 0 ge (pub key is not zero bitwise)
  public_key z_neg_pk = z_pk;
  ((unsigned char*)&z_neg_pk)[31] = 0x80; // set sign bit manually
  std::cout << "-Identity = " << z_pk << ENDL;
  point_t z_neg;
  ASSERT_FALSE(z_neg.from_public_key(z_neg_pk)); // negative identity should not be loaded

  key_image z_ki;
  memset(&z_ki, 0, sizeof(z_ki));
  ((unsigned char*)&z_ki)[00] = 0x01; // y = 1

  ASSERT_TRUE(validate_key_image(z_ki));

  key_image z_neg_ki = z_ki;
  ((unsigned char*)&z_neg_ki)[31] = 0x80; // set sign bit manually

  ASSERT_FALSE(validate_key_image(z_neg_ki)); // negative identity should not be loaded


  // also do zero-byte pub key / key image checks

  public_key zzz_pk;
  memset(&zzz_pk, 0, sizeof(public_key));

  ASSERT_TRUE(check_key(zzz_pk));

  point_t zzz;
  ASSERT_TRUE(zzz.from_public_key(zzz_pk));
  ASSERT_FALSE(zzz.is_in_main_subgroup());

  key_image zzz_ki;
  memset(&zzz_ki, 0, sizeof(key_image));

  ASSERT_FALSE(validate_key_image(zzz_ki));

  point_t zzz2;
  ASSERT_TRUE(zzz2.from_key_image(zzz_ki));
  ASSERT_FALSE(zzz2.is_in_main_subgroup());
  ASSERT_EQ(zzz, zzz2);

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

TEST(crypto, hex_tools)
{
  ASSERT_EQ(parse_tpod_from_hex_string<uint8_t>("00"), 0x00);
  ASSERT_EQ(parse_tpod_from_hex_string<uint8_t>("01"), 0x01);
  ASSERT_EQ(parse_tpod_from_hex_string<uint8_t>("f1"), 0xf1);
  ASSERT_EQ(parse_tpod_from_hex_string<uint8_t>("fe"), 0xfe);
  ASSERT_EQ(parse_tpod_from_hex_string<uint64_t>("efcdab8967452301"), 0x0123456789abcdef);
  ASSERT_EQ(parse_tpod_from_hex_string<uint64_t>("0123456789abcdef"), 0xefcdab8967452301);
  ASSERT_EQ(parse_tpod_from_hex_string<scalar_t>("ecffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7f"), c_scalar_Pm1);
  ASSERT_EQ(parse_tpod_from_hex_string<scalar_t>("792fdce229e50661d0da1c7db39dd30700000000000000000000000000000006"), c_scalar_1div8);

  return true;
}


TEST(crypto, calc_lsb_32)
{
  auto local_calc_lsb = [](uint32_t v) {
    uint8_t r = 0;
    while (v != 0 && (v & 1) == 0)
    {
      v >>= 1;
      ++r;
    }
    return r;
  };

  for (uint32_t x = 0; x < UINT32_MAX; ++x)
  {
    if (x % 10000000 == 0)
      std::cout << x << ENDL;
    ASSERT_EQ((int)local_calc_lsb(x), (int)calc_lsb_32(x));
  }
  ASSERT_EQ((int)local_calc_lsb(UINT32_MAX), (int)calc_lsb_32(UINT32_MAX));

  return true;
}

TEST(crypto, torsion_elements)
{
  point_t tor;

  for (size_t i = 0, n = sizeof canonical_torsion_elements / sizeof canonical_torsion_elements[0]; i < n; ++i)
  {
    const canonical_torsion_elements_t& el = canonical_torsion_elements[i];
    ASSERT_TRUE(tor.from_string(el.string));
    ASSERT_FALSE(tor.is_zero());
    ASSERT_FALSE(tor.is_in_main_subgroup());

    ASSERT_EQ((fe_isnegative(tor.m_p3.X) != 0), el.sign);

    ASSERT_FALSE(el.incorrect_order_0 * tor == c_point_0);
    ASSERT_FALSE(el.incorrect_order_1 * tor == c_point_0);
    ASSERT_TRUE(el.order * tor == c_point_0);
  }

  // non-canonical elements should not load at all (thanks to the checks in ge_frombytes_vartime)
  for (size_t i = 0, n = sizeof noncanonical_torsion_elements / sizeof noncanonical_torsion_elements[0]; i < n; ++i)
  {
    ASSERT_FALSE(tor.from_string(noncanonical_torsion_elements[i]));
  }

  return true;
}

TEST(crypto, point_is_zero)
{
  static const fe fancy_p         = { -19, 33554432, -1, 33554432, -1, 33554432, -1, 33554432, -1, 33554432 }; // 2**255 - 19
  static const fe fancy_p_plus_1  = { -18, 33554432, -1, 33554432, -1, 33554432, -1, 33554432, -1, 33554432 }; // 2**255 - 18
  static const fe f_one = { 1 };

  ASSERT_TRUE(fe_isnonzero(fancy_p) == 0);
  ASSERT_TRUE(fe_isnonzero(fancy_p_plus_1) != 0);

  fe f_r, f_x;
  fe_frombytes(f_x, scalar_t::random().data());
  fe_mul(f_r, f_x, fancy_p);
  ASSERT_TRUE(fe_isnonzero(f_r) == 0);
  
  fe_sub(f_r, fancy_p_plus_1, f_one);
  ASSERT_TRUE(fe_isnonzero(f_r) == 0);

  // is_zero

  point_t p;
  memset(&p.m_p3, 0, sizeof p.m_p3);
  memcpy(&p.m_p3.X, fancy_p, sizeof p.m_p3.X); // X = 2**255-19
  memcpy(&p.m_p3.Y, fancy_p_plus_1, sizeof p.m_p3.Y); // Y = 2**255-19+1
  p.m_p3.Z[0] = 1;
  // {P, P+1, 1, 0} == {0, 1} (the identity point)

  ASSERT_TRUE(p.is_zero());


  memset(&p.m_p3, 0, sizeof p.m_p3);
  memcpy(&p.m_p3.X, fancy_p, sizeof p.m_p3.X); // X = 2**255-19
  memcpy(&p.m_p3.Y, fancy_p_plus_1, sizeof p.m_p3.Y); // Y = 2**255-19+1
  p.m_p3.Z[0] = -1;
  // {P, P+1, -1, 0} == {0, -1} (not an identity point, torsion element order 2)

  ASSERT_FALSE(p.is_zero());

  memset(&p.m_p3, 0, sizeof p.m_p3);
  p.m_p3.Y[0] = 2;
  p.m_p3.Z[0] = 2;
  // {0, 2, 2, 0} == {0, 1} (the identity point)

  ASSERT_TRUE(p.is_zero());

  // all fe 10 components must be in [-33554432, 33554432]  (curve25519-20060209.pdf page 9)
  //        2**0      2**26     2**51   2**77    2**102   2**128     2**153    2**179   2**204    2*230 
  fe a0 = { 7172245,  16777211, 922265, 8160646, 9625798, -12989394, 10843498, 6987154, 15156548, -5214544 };
  fe a1 = { 7172245, -16777221, 922266, 8160646, 9625798, -12989394, 10843498, 6987154, 15156548, -5214544 };
  // note, a0 == a1:
  //  16777211 * 2**26  + 922265 * 2**51 = 2076757281067996545024
  // -16777221 * 2**26  + 922266 * 2**51 = 2076757281067996545024

  memset(&p.m_p3, 0, sizeof p.m_p3);
  memcpy(&p.m_p3.Y, &a0, sizeof a0);
  memcpy(&p.m_p3.Z, &a1, sizeof a1);
  // {0, x, x, 0} == {0, 1, 1, 0} == {0, 1} (the identity point)

  ASSERT_TRUE(p.is_zero());

  
  memset(&p.m_p3, 0, sizeof p.m_p3);
  memcpy(&p.m_p3.Y, f_x, sizeof p.m_p3.Y);
  memcpy(&p.m_p3.Z, f_x, sizeof p.m_p3.Z);
  memcpy(&p.m_p3.T, fancy_p, sizeof p.m_p3.T);
  // {0, x, x, P} == {0, 1} (still the identity point)

  ASSERT_TRUE(p.is_zero());

  //
  // negative tests
  //

  memset(&p.m_p3, 0, sizeof p.m_p3);
  // {0, 0, 0, 0} is not a point at all

  ASSERT_FALSE(p.is_zero());


  memset(&p.m_p3, 0, sizeof p.m_p3);
  memcpy(&p.m_p3.Y, f_x, sizeof p.m_p3.Y);
  memcpy(&p.m_p3.Z, f_x, sizeof p.m_p3.Z);
  memcpy(&p.m_p3.T, fancy_p_plus_1, sizeof p.m_p3.T);
  // {0, x, x, !0} is not a valid point (incorrect non-zero T) 

  ASSERT_FALSE(p.is_zero());

  memcpy(&p.m_p3.T, f_x, sizeof p.m_p3.T);
  // {0, x, x, x}, while x != 0 is still incorrect point representation
  ASSERT_FALSE(p.is_zero());

  return true;
}


TEST(crypto, sc_get_bit)
{
  static_assert(sizeof(scalar_t) * 8 == 256, "size missmatch");

  scalar_t v = 0; // all bits are 0
  for (size_t n = 0; n < 256; ++n)
  {
    ASSERT_EQ(v.get_bit(static_cast<uint8_t>(n)), false);
  }

  v = c_scalar_256m1; // all bits are 1
  for (size_t n = 0; n < 256; ++n)
  {
    ASSERT_EQ(v.get_bit(static_cast<uint8_t>(n)), true);
  }

  // check random value
  const scalar_t x = scalar_t::random();
  for (size_t n = 0; n < 64; ++n)
    ASSERT_EQ(x.get_bit(static_cast<uint8_t>(n)), ((x.m_u64[0] & (1ull << (n - 0))) != 0));
  for (size_t n = 64; n < 128; ++n)
    ASSERT_EQ(x.get_bit(static_cast<uint8_t>(n)), ((x.m_u64[1] & (1ull << (n - 64))) != 0));
  for (size_t n = 128; n < 192; ++n)
    ASSERT_EQ(x.get_bit(static_cast<uint8_t>(n)), ((x.m_u64[2] & (1ull << (n - 128))) != 0));
  for (size_t n = 192; n < 256; ++n)
    ASSERT_EQ(x.get_bit(static_cast<uint8_t>(n)), ((x.m_u64[3] & (1ull << (n - 192))) != 0));

  return true;
}


TEST(crypto, sc_set_bit_clear_bit)
{
  static_assert(sizeof(scalar_t) * 8 == 256, "size missmatch");

  // check random value
  const scalar_t x = scalar_t::random();
  scalar_t y = scalar_t::random();
  ASSERT_NEQ(x, y);

  uint8_t i = 0;
  do
  {
    if (x.get_bit(i))
      y.set_bit(i);
    else
      y.clear_bit(i);
  } while(++i != 0);

  ASSERT_EQ(x, y);

  return true;
}


TEST(crypto, schnorr_sig)
{
  public_key invalid_pk = parse_tpod_from_hex_string<public_key>("0000000000000000000000000000000000000000000000000000000000000001");
  ASSERT_FALSE(check_key(invalid_pk));

  hash m = *(crypto::hash*)(&scalar_t::random());
  for(size_t i = 0; i < 100; ++i)
  {
    generic_schnorr_sig_s ss{};
    scalar_t a = scalar_t::random();
    point_t A_pt = a * c_point_G;
    public_key A = A_pt.to_public_key();
    // (different behaviour in debug and release) ASSERT_FALSE(generate_schnorr_sig<gt_X>(m, A_pt, a, ss));
    ASSERT_TRUE(generate_schnorr_sig<gt_G>(m, A_pt, a, ss));
    ASSERT_FALSE(verify_schnorr_sig<gt_X>(m, A, ss));
    ASSERT_TRUE(verify_schnorr_sig<gt_G>(m, A, ss));

    A_pt = a * c_point_X;
    A = A_pt.to_public_key();
    // (different behaviour in debug and release) ASSERT_FALSE(generate_schnorr_sig<gt_G>(m, A_pt, a, ss));
    ASSERT_TRUE(generate_schnorr_sig<gt_X>(m, A_pt, a, ss));
    ASSERT_FALSE(verify_schnorr_sig<gt_G>(m, A, ss));
    ASSERT_TRUE(verify_schnorr_sig<gt_X>(m, A, ss));

    ASSERT_FALSE(verify_schnorr_sig<gt_X>(currency::null_hash, A, ss));
    ASSERT_FALSE(verify_schnorr_sig<gt_X>(m, invalid_pk, ss));

    generic_schnorr_sig_s bad_ss = ss;
    bad_ss.c = c_scalar_Pm1;
    ASSERT_FALSE(bad_ss.c.is_reduced());
    ASSERT_FALSE(verify_schnorr_sig<gt_X>(m, A, bad_ss));
    bad_ss = ss;
    bad_ss.y = c_scalar_Pm1;
    ASSERT_FALSE(bad_ss.y.is_reduced());
    ASSERT_FALSE(verify_schnorr_sig<gt_X>(m, A, bad_ss));

    bad_ss = ss;

    mp::uint256_t c_mp = bad_ss.c.as_boost_mp_type<mp::uint256_t>();
    c_mp += c_scalar_L.as_boost_mp_type<mp::uint256_t>();
    memcpy(bad_ss.c.data(), c_mp.backend().limbs(), sizeof(scalar_t));
    ASSERT_FALSE(bad_ss.c.is_reduced());
    scalar_t tmp(bad_ss.c);
    tmp.reduce();
    ASSERT_EQ(tmp, ss.c);
    ASSERT_FALSE(verify_schnorr_sig<gt_X>(m, A, bad_ss));

    // binary serialization 
    std::string blob = t_serializable_object_to_blob(ss);
    generate_random_bytes(sizeof ss, &ss);
    ASSERT_TRUE(t_unserializable_object_from_blob(ss, blob));
    ASSERT_TRUE(verify_schnorr_sig<gt_X>(m, A, ss));

    // boost serialization
    ASSERT_TRUE(tools::serialize_obj_to_buff(ss, blob));
    generate_random_bytes(sizeof ss, &ss);
    ASSERT_TRUE(tools::unserialize_obj_from_buff(ss, blob));
    ASSERT_TRUE(verify_schnorr_sig<gt_X>(m, A, ss));
  }

  return true;
}


TEST(crypto, point_negation)
{
  ASSERT_EQ(c_point_0, -c_point_0);
  ASSERT_NEQ(c_point_G, -c_point_G);
  ASSERT_EQ(c_point_G, -(-c_point_G));
  ASSERT_EQ(-c_point_G, c_scalar_Lm1 * c_point_G);
  ASSERT_EQ(-c_point_G, c_point_0 - c_point_G);
  ASSERT_EQ(0 * (-c_point_G), c_point_0);
  scalar_t a = scalar_t::random(), b = scalar_t::random();
  ASSERT_EQ(a * (-c_point_G) + b * c_point_G + a * c_point_H + b * (-c_point_H), (b - a) * c_point_G + (a - b) * c_point_H);
  ASSERT_EQ(a * (-c_point_G), (a * c_scalar_Lm1) * c_point_G);

  for(size_t i = 0, sz = sizeof(canonical_torsion_elements) / sizeof(canonical_torsion_elements[0]); i < sz; ++i)
  {
    point_t el{};
    ASSERT_TRUE(el.from_string(canonical_torsion_elements[i].string));
    ASSERT_EQ(el, -(-el));
    ASSERT_EQ((-scalar_t(1)) * el, (c_scalar_0 - c_scalar_1) * el);
    ASSERT_NEQ(-el, (-scalar_t(1)) * el);  // because torsion elements have order != L
    ASSERT_NEQ(-el, c_scalar_Lm1 * el);    // because torsion elements have order != L
    ASSERT_EQ(-el, (scalar_t(canonical_torsion_elements[i].order) - 1) * el); // they rather have order == canonical_torsion_elements[i].order
    ASSERT_EQ(-el, c_point_0 - el);
    ASSERT_EQ((-el) + (el), c_point_0);
    ASSERT_EQ((-el) - (-el), c_point_0);
    scalar_t x = scalar_t::random();
    ASSERT_EQ(x * (-el) + x * el, c_point_0);
  }

  return true;
}


TEST(crypto, scalar_get_bits)
{
  scalar_t x = scalar_t::random();
  for(size_t i = 0; i < 256; ++i)
    ASSERT_EQ(x.get_bits(i, 0), 0);
  for(size_t i = 0; i < 256; ++i)
    ASSERT_EQ(x.get_bits(i, std::min((size_t)255, i + 65)), 0);

  ASSERT_EQ(x.get_bits(0,   64), x.m_u64[0]);
  ASSERT_EQ(x.get_bits(64,  64), x.m_u64[1]);
  ASSERT_EQ(x.get_bits(128, 64), x.m_u64[2]);
  ASSERT_EQ(x.get_bits(192, 64), x.m_u64[3]);

  uint64_t high_32_bits = x.m_u64[3] >> 32;
  ASSERT_EQ(x.get_bits(192+32, 32), high_32_bits);

  for(size_t i = 33; i <= 64; ++i)
    ASSERT_EQ(x.get_bits(192+32, i), high_32_bits);

  for(size_t i = 0; i < 10000; ++i)
  {
    scalar_t b = scalar_t::random();
    scalar_t x = scalar_t::random();
    size_t bit_index_from = b.m_s[5];
    size_t bits_count     = b.m_s[6] % 65; // [0; 64] are allowed

    uint64_t extracted_bits = 0;
    for(size_t j = 0; j < bits_count; ++j)
    {
      if (bit_index_from + j <= 255 && x.get_bit(bit_index_from + j))
        extracted_bits |= 1ull << j;
    }

    if (extracted_bits != x.get_bits(bit_index_from, bits_count))
    {
      std::cout << "i: " << i << ", bit_index_from: " << bit_index_from << ", bits_count: " << bits_count << ENDL
        << "extracted_bits: " << extracted_bits << ", get_bits(): " << x.get_bits(bit_index_from, bits_count);
      ASSERT_TRUE(false);
    }
  }
  return true;
}


TEST(crypto, scalarmult_base_vartime)
{
  auto check_for_x = [&](const scalar_t& x) -> bool {
    point_t P, P2;
    ge_scalarmult_base_vartime(&P.m_p3, x.m_s);
    ge_scalarmult_base(&P2.m_p3, x.m_s);
    return (P - P2).is_zero();
    };

  ASSERT_TRUE(check_for_x(c_scalar_0));
  ASSERT_TRUE(check_for_x(c_scalar_1));
  ASSERT_TRUE(check_for_x(c_scalar_1div8));
  ASSERT_TRUE(check_for_x(c_scalar_Lm1));
  ASSERT_TRUE(check_for_x(c_scalar_L));

  for(size_t i = 0; i < 1000; ++i)
  {
    scalar_t x = scalar_t::random();
    ASSERT_TRUE(check_for_x(x));
  }

  return true;
}


template<typename CT>
bool crypto_msm_runner(size_t N, size_t low_bits_to_clear, size_t high_bits_to_clear)
{
  scalar_vec_t g_scalars, h_scalars;
  g_scalars.resize_and_make_random(N);
  h_scalars.resize_and_make_random(N);
  if (N > 4)
  {
    g_scalars[0] = c_scalar_Lm1; // always include the max and the min
    h_scalars[0] = c_scalar_Lm1;
    g_scalars[1] = 0;
    h_scalars[1] = 0;
  }

  point_t sum = c_point_0;
  for(size_t i = 0; i < N; ++i)
  {
    for(size_t bit_index = 0; bit_index < low_bits_to_clear; ++bit_index)
    {
      g_scalars[i].clear_bit(bit_index);
      h_scalars[i].clear_bit(bit_index);
    }
    for(size_t bit_index = 256 - high_bits_to_clear; bit_index < 256; ++bit_index)
    {
      g_scalars[i].clear_bit(bit_index);
      h_scalars[i].clear_bit(bit_index);
    }
    sum += g_scalars[i] * CT::get_generator(false, i) + h_scalars[i] * CT::get_generator(true, i);
  }

  //TIME_MEASURE_START(t);
  bool r = msm_and_check_zero<CT>(g_scalars, h_scalars, -sum);
  //TIME_MEASURE_FINISH(t);
  return r;
}

TEST(crypto, msm)
{
  // test the default msm_and_check_zero correctness
  bool r = false;

  for(size_t N = 1; N <= 128; ++N)
  {
    std::cout << "N = " << N << ENDL;
    r = crypto_msm_runner<bpp_crypto_trait_Zarcanum>(N, 0, 0);
    ASSERT_TRUE(r);
    r = crypto_msm_runner<bpp_crypto_trait_ZC_out>(N, 0, 0);
    ASSERT_TRUE(r);
  }

  for(size_t i = 0; i <= 128; ++i)
  {
    std::cout << "i = " << i << ENDL;
    r = crypto_msm_runner<bpp_crypto_trait_Zarcanum>(128, i, 0);
    ASSERT_TRUE(r);
    r = crypto_msm_runner<bpp_crypto_trait_Zarcanum>(128, 0, i);
    ASSERT_TRUE(r);
    r = crypto_msm_runner<bpp_crypto_trait_ZC_out>(256, i, 0);
    ASSERT_TRUE(r);
    r = crypto_msm_runner<bpp_crypto_trait_ZC_out>(256, 0, i);
    ASSERT_TRUE(r);
  }

  return true;
}



inline std::ostream &operator <<(std::ostream &o, const crypto::ge_precomp v)
{
  o << "{{";

  for(size_t i = 0; i < 9; ++i)
    o << v.yplusx[i] << ", ";

  o << v.yplusx[9] << "}, {";

  for(size_t i = 0; i < 9; ++i)
    o << v.yminusx[i] << ", ";

  o << v.yminusx[9] << "}, {";

  for(size_t i = 0; i < 9; ++i)
    o << v.xy2d[i] << ", ";

  o << v.xy2d[9] << "}}";
  return o;
}

bool calc_and_print_generator_precomp(const point_pc_t& generator, const char* generator_var_name)
{
  precomp_data_t precomp_data = {};
  construct_precomp_data(precomp_data, generator);

  std::cout << "    const precomp_data_t " << generator_var_name << "_precomp_data = {" << ENDL;

  for(size_t i = 0; i < 32; ++i)
  {
    std::cout << "      {" << ENDL;
    for(size_t j = 0; j < 8; ++j)
      std::cout << "        " << precomp_data[i][j] << (j != 7 ? "," : "" ) << ENDL;
    std::cout << "      }" << (i != 31 ? "," : "" ) << ENDL;
  }

  std::cout << "    };" << ENDL;

  return true;
}

TEST(print, generators_precomp)
{
#define CALC_PRECOMP(G) calc_and_print_generator_precomp(G, #G)

  CALC_PRECOMP(c_point_H);
  CALC_PRECOMP(c_point_H2);
  CALC_PRECOMP(c_point_U);
  CALC_PRECOMP(c_point_X);
  CALC_PRECOMP(c_point_H_plus_G);
  CALC_PRECOMP(c_point_H_minus_G);
  return true;

#undef CALC_PRECOMP
}

bool check_generator_precomp(const point_pc_t& generator, const char* generator_var_name)
{
  point_t generator_pt = generator; // to avoid using precomputed data in scalar multiplications
  point_t random_point = hash_helper_t::hp(scalar_t::random());

  point_t A = generator_pt;
  for(size_t i = 0; i < 32; ++i)
  {
    point_t B = c_point_0;
    for(size_t j = 0; j < 8; ++j)
    {
      B += A;

      // restore ge_p3 from ge_precomp using native NaCl functions... 
      point_t restored_pt{};
      ge_p1p1 p1p1{};
      ge_madd(&p1p1, &random_point.m_p3, &((*generator.m_precomp_data_p)[i][j]));
      ge_p1p1_to_p3(&restored_pt.m_p3, &p1p1);
      restored_pt -= random_point;

      // ...and compare it with the calculated one
      if (B != restored_pt)
      {
        std::cout << "ERROR: " << generator_var_name << ", i: " << i << ", j: " << j << ENDL;
        return false;
      }
    }
    if (i != 31)
      A.modify_mul_pow_2(8);
  }

  std::cout << "   " << std::left << std::setw(32) << generator_var_name << "   OK" << ENDL;
  return true;
}

TEST(crypto, generators_precomp)
{
#define CHECK_PRECOMP(G) ASSERT_TRUE(check_generator_precomp(G, #G))

  CHECK_PRECOMP(c_point_H);
  CHECK_PRECOMP(c_point_H2);
  CHECK_PRECOMP(c_point_U);
  CHECK_PRECOMP(c_point_X);
  CHECK_PRECOMP(c_point_H_plus_G);
  CHECK_PRECOMP(c_point_H_minus_G);

  return true;

#undef CHECK_PRECOMP
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


  size_t filtered_tests_count = 0;
  std::vector<size_t> failed_tests;
  for (size_t i = 0; i < g_tests.size(); ++i)
  {
    auto& test = g_tests[i];

    if (!wildcard_match(cmd_line_param.c_str(), test.first.c_str()))
      continue;

    ++filtered_tests_count;

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

  if (filtered_tests_count == 0)
  {
    LOG_PRINT_YELLOW(ENDL << ENDL << "No tests were selected, check the filter mask", LOG_LEVEL_0);
    return 1;
  }

  if (failed_tests.empty())
  {
    LOG_PRINT_GREEN(ENDL, LOG_LEVEL_0);
    LOG_PRINT_GREEN(filtered_tests_count << " tests passed okay", LOG_LEVEL_0);
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
