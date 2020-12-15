// Copyright (c) 2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define USE_INSECURE_RANDOM_RPNG_ROUTINES // turns on random manupulation for tests
#include "gtest/gtest.h"
#include "crypto/crypto.h"

extern "C" {
#include "crypto/crypto-ops.h"
void fe_reduce(fe u, const fe h)
{
  /* From fe_frombytes.c */

  // loading changed

  int64_t h0 = h[0];
  int64_t h1 = h[1];
  int64_t h2 = h[2];
  int64_t h3 = h[3];
  int64_t h4 = h[4];
  int64_t h5 = h[5];
  int64_t h6 = h[6];
  int64_t h7 = h[7];
  int64_t h8 = h[8];
  int64_t h9 = h[9];
  int64_t carry0;
  int64_t carry1;
  int64_t carry2;
  int64_t carry3;
  int64_t carry4;
  int64_t carry5;
  int64_t carry6;
  int64_t carry7;
  int64_t carry8;
  int64_t carry9;

  carry9 = (h9 + (int64_t)(1 << 24)) >> 25; h0 += carry9 * 19; h9 -= carry9 << 25;
  carry1 = (h1 + (int64_t)(1 << 24)) >> 25; h2 += carry1; h1 -= carry1 << 25;
  carry3 = (h3 + (int64_t)(1 << 24)) >> 25; h4 += carry3; h3 -= carry3 << 25;
  carry5 = (h5 + (int64_t)(1 << 24)) >> 25; h6 += carry5; h5 -= carry5 << 25;
  carry7 = (h7 + (int64_t)(1 << 24)) >> 25; h8 += carry7; h7 -= carry7 << 25;

  carry0 = (h0 + (int64_t)(1 << 25)) >> 26; h1 += carry0; h0 -= carry0 << 26;
  carry2 = (h2 + (int64_t)(1 << 25)) >> 26; h3 += carry2; h2 -= carry2 << 26;
  carry4 = (h4 + (int64_t)(1 << 25)) >> 26; h5 += carry4; h4 -= carry4 << 26;
  carry6 = (h6 + (int64_t)(1 << 25)) >> 26; h7 += carry6; h6 -= carry6 << 26;
  carry8 = (h8 + (int64_t)(1 << 25)) >> 26; h9 += carry8; h8 -= carry8 << 26;

  u[0] = h0;
  u[1] = h1;
  u[2] = h2;
  u[3] = h3;
  u[4] = h4;
  u[5] = h5;
  u[6] = h6;
  u[7] = h7;
  u[8] = h8;
  u[9] = h9;

  /* End fe_frombytes.c */

}


void sc_mul(unsigned char *s, const unsigned char *a, const unsigned char *b)
{
  unsigned char c[32];
  unsigned char neg_a[32];
  sc_0(c);
  sc_sub(neg_a, c, a);
  // s = c - ab
  sc_mulsub(s, neg_a, b, c);
}

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

  //memcpy(out, z, sizeof(crypto::ec_scalar));

  for (size_t i = msb_s; i != SIZE_MAX; --i)
  {
    sc_mul(out, out, out);
    std::cout << "sc_mul(out, out, out);" << std::endl;
    uint8_t bit = (s[i / 8] >> (i % 8)) & 1;
    if (bit)
    {
      sc_mul(out, out, z);
      std::cout << "sc_mul(out, out, z);" << std::endl;
    }
  }
}

void sc_invert(unsigned char* out, const unsigned char* z)
{
  memcpy(out, z, sizeof(crypto::ec_scalar));
  for(size_t i = 0; i < 128; ++i)
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




template<class pod_t>
std::string pod_to_hex_big_endian(const pod_t &h)
{
  constexpr char hexmap[] = "0123456789abcdef";
  const char* data = reinterpret_cast<const char*>(&h);
  size_t len = sizeof h;

  std::string s(len * 2, ' ');
  for (size_t i = 0; i < len; ++i) {
    s[2 * i]     = hexmap[(data[len - 1 - i] & 0xF0) >> 4];
    s[2 * i + 1] = hexmap[(data[len - 1 - i] & 0x0F)];
  }

  return s;
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
    /*unsigned char bytes[32] = {2};
    fe t;
    fe_frombytes(t, (unsigned char*)&bytes);
    fe r;
    fe_invert(r, t);
    fe m;
    fe_mul(m, r, t);


    fe r2;
    my_fe_invert(r2, t);
    fe m2;
    fe_mul(m2, r2, t);

    scalar_t result;
    fe v_f;
    fe result_f;
    fe_frombytes(v_f, reinterpret_cast<const unsigned char*>(&m_s));
    my_fe_invert(result_f, v_f);
    fe_tobytes(reinterpret_cast<unsigned char*>(&result), result_f);
    //sc_reduce(reinterpret_cast<unsigned char*>(&result));
    fe result_check;
    fe_frombytes(result_check, reinterpret_cast<unsigned char*>(&result));
    fe v_check;
    fe_invert(v_check, result_check);
    //sc_reduce(reinterpret_cast<unsigned char*>(&result));
    return result;*/

    scalar_t result;
    fe v_f;
    fe result_f;
    fe_frombytes(v_f, reinterpret_cast<const unsigned char*>(&m_s));
    fe_invert(result_f, v_f);

    /*fe x2;
    fe_mul(x2, result_f, v_f);

    fe_reduce(result_f, result_f);
    if (fe_cmp(result_f, scalar_L_fe) > 0)
    {
      // result_f > L
      fe_sub(result_f, result_f, scalar_L_fe);
      if (fe_cmp(result_f, scalar_L_fe) >= 0)
        return false; // fail
    }*/
    unsigned char tmp[64] = { 0 };
    fe_tobytes(reinterpret_cast<unsigned char*>(&tmp), result_f);
    sc_reduce(tmp);
    memcpy(&result, &tmp, sizeof result);
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

static const scalar_t scalar_L     = { 0x5812631a5cf5d3ed, 0x14def9dea2f79cd6, 0x0,                0x1000000000000000 };
static const scalar_t scalar_Lm1   = { 0x5812631a5cf5d3ec, 0x14def9dea2f79cd6, 0x0,                0x1000000000000000 };
static const scalar_t scalar_P     = { 0xffffffffffffffed, 0xffffffffffffffff, 0xffffffffffffffff, 0x7fffffffffffffff };
static const scalar_t scalar_Pm1   = { 0xffffffffffffffec, 0xffffffffffffffff, 0xffffffffffffffff, 0x7fffffffffffffff };
static const scalar_t scalar_256m1 = { 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff, 0xffffffffffffffff };


/* temporary disable in order not to break the compilation
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
    ASSERT_GT(z, z - 1);
    ASSERT_LT(z, z + 1);
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



  fe L_fe;
  fe_frombytes(L_fe, &scalar_L.m_s[0]);

  fe Pm1_fe;
  fe_frombytes(Pm1_fe, &scalar_Pm1.m_s[0]);

  fe r;
  fe f_1 = { 1 };
  fe_add(r, Pm1_fe, f_1);



  while(true)
  {
    static int ti = 2;
    static int pi = 3;
    scalar_t t = ti;
    scalar_t p = pi;
    scalar_t r = 0;
    //sc_exp(r.data(), t.data(), Lm2);

    sc_invert(r.data(), t.data());

    std::cout << ti << " ^ L-2" << " = " << r << std::endl;

    r = r * 6;
    std::cout << r << std::endl;
  }



  scalar_t a = 2;
  a = a / 2;
  std::cout << "2 / 2       = " << a << std::endl;
  a = scalar_Lm1 / 2;
  std::cout << "L-1 / 2     = " << a << std::endl;
  a = a * 2;
  std::cout << "L-1 / 2 * 2 = " << a << std::endl;

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
}

TEST(crypto, scalar_reciprocal)
{
  int64_t test_nums[] = {1, 2, 10};

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
}


TEST(crypto, scalars)
{
  scalar_t s = 20;
  scalar_t d = 5;
  scalar_t e = s / d;
  scalar_t m = e * d;

  ASSERT_TRUE(m == s);
}

*/