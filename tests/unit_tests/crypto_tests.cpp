// Copyright (c) 2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define USE_INSECURE_RANDOM_RPNG_ROUTINES // turns on random manupulation for tests
#include "gtest/gtest.h"
#include "crypto/crypto.h"

extern "C" {
#include "crypto/crypto-ops.h"

/*
Input:
a[0]+256*a[1]+...+256^31*a[31] = a
b[0]+256*b[1]+...+256^31*b[31] = b
Output:
s[0]+256*s[1]+...+256^31*s[31] = (ab) mod l
where l = 2^252 + 27742317777372353535851937790883648493
*/

// TODO: make more efficient multiplication
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


struct scalar_t
{
  //fe m_fe; // 40 bytes, array 10 * 4, optimized form
  crypto::ec_scalar m_s;

  scalar_t()
  {}

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

  operator crypto::secret_key() const
  {
    crypto::secret_key result;
    memcpy(result.data, m_s.data, sizeof result.data);
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
    memset(m_s.data, 0, sizeof m_s.data);
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
    fe_tobytes(reinterpret_cast<unsigned char*>(&result), result_f);
    return result;
  }

  scalar_t operator/(const scalar_t& v) const
  {
    return operator*(v.reciprocal());
  }

  friend bool operator==(const scalar_t& lhs, const scalar_t& rhs)
  {
    return memcmp(&lhs, &rhs, sizeof lhs) == 0;
  }

};

//__declspec(align(32))
struct point_t
{
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
    static_assert(sizeof result == 32, "size error");
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
};

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



};

static const point_g_t point_G;

TEST(crypto, basics)
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

TEST(crypto, scalars)
{
  scalar_t s = 20;
  scalar_t d = 5;
  scalar_t q = s * d;
  scalar_t e = s / d;
  scalar_t m = e * d;

  // ASSERT_TRUE(m == s);
}
