// Copyright (c) 2023 Zano Project
// Copyright (c) 2023 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include "one_out_of_many_proofs.h"
#include "../currency_core/crypto_config.h"
#include "epee/include/misc_log_ex.h"

//DISABLE_GCC_AND_CLANG_WARNING(unused-function)

#if 0
#  define DBG_VAL_PRINT(x) std::cout << std::setw(30) << std::left << #x ": " << x << std::endl
#  define DBG_PRINT(x)     std::cout << x << std::endl
#else
#  define DBG_VAL_PRINT(x) (void(0))
#  define DBG_PRINT(x)     (void(0))
#endif

namespace crypto
{
  static const size_t N_max = 256;
  static const size_t mn_max = 16;

  const point_t& get_BGE_generator(size_t index, bool& ok)
  {
    static std::vector<point_t> precalculated_generators;
    if (precalculated_generators.empty())
    {
      precalculated_generators.resize(mn_max * 2);

      scalar_t hash_buf[2] = { hash_helper_t::hs("Zano BGE generator"), 0 };

      for(size_t i = 0; i < precalculated_generators.size(); ++i)
      {
        hash_buf[1].m_u64[0] = i;
        precalculated_generators[i] = hash_helper_t::hp(&hash_buf, sizeof hash_buf);
      }
    }

    if (index >= mn_max * 2)
    {
      ok = false;
      return c_point_0;
    }

    ok = true;
    return precalculated_generators[index];
  }




#define CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(cond, err_code) \
    if (!(cond)) { LOG_PRINT_RED("generate_BGE_proof: \"" << #cond << "\" is false at " << LOCATION_SS << ENDL << "error code = " << (int)err_code, LOG_LEVEL_3); \
    if (p_err) { *p_err = err_code; } return false; }

  bool generate_BGE_proof(const hash& context_hash, const std::vector<point_t>& ring, const scalar_t& secret, const size_t secret_index, BGE_proof& result, uint8_t* p_err /* = nullptr */)
  {
    static constexpr size_t n = 4; // TODO: @#@# move it out

    DBG_PRINT(" - - - generate_BGE_proof - - -");
    size_t ring_size = ring.size();
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(ring_size > 0, 0);
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(secret_index < ring_size, 1);

#ifndef NDEBUG
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(ring[secret_index] == secret * crypto::c_point_X, 2);
#endif


    const size_t m = std::max(static_cast<uint64_t>(1), constexpr_ceil_log_n(ring_size, n));
    const size_t N = constexpr_pow(m, n);
    const size_t mn = m * n;

    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(N <= N_max, 3);
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(mn <= mn_max, 4);

    scalar_mat_t<n> a_mat(mn);       // m x n matrix
    a_mat.zero();
    std::vector<size_t> l_digits(m); // l => n-ary gidits
    size_t l = secret_index;
    for(size_t j = 0; j < m; ++j)
    {
      for(size_t i = n - 1; i != 0; --i) // [n - 1; 1]
      {
        a_mat(j, i).make_random();
        a_mat(j, 0) -= a_mat(j, i); // a[j; 0] = -sum( a[j; i] ), i in [1; n-1]
      }

      size_t digit = l % n;         // j-th digit of secret_index
      l_digits[j] = digit;
      l = l / n;
    }

#ifndef NDEBUG
    for(size_t j = 0; j < m; ++j)
    {
      scalar_t a_sum{};
      for(size_t i = 0; i != n; ++i)
        a_sum += a_mat(j, i);
      CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(a_sum.is_zero(), 230);
    }
#endif

    //
    // coeffs calculation (naive implementation, consider optimization in future)
    //
    scalar_vec_t coeffs(N * m);                     // m x N matrix
    coeffs.zero();
    for(size_t i = 0; i < N; ++i)
    {
      coeffs[i] = c_scalar_1;                       // first row is (1, ..., 1)
      size_t i_tmp = i;
      size_t m_bound = 1;
      for(size_t j = 0; j < m; ++j)
      {
        size_t i_j = i_tmp % n;                     // j-th digit of i
        i_tmp /= n;

        if (i_j == l_digits[j])                     // true if j-th digits of i and l matches
        {
          scalar_t carry{};
          for(size_t k = 0; k < m_bound; ++k)
          {
            scalar_t old = coeffs[k * N + i];
            coeffs[k * N + i] *= a_mat(j, i_j);
            coeffs[k * N + i] += carry;
            carry = old;
          }
          if (m_bound < m)
            coeffs[m_bound * N + i] += carry;
          ++m_bound;
        }
        else
        {
          for(size_t k = 0; k < m_bound; ++k)
            coeffs[k * N + i] *= a_mat(j, i_j);
        }
      }
    }

    scalar_t r_A = scalar_t::random();
    scalar_t r_B = scalar_t::random();
    scalar_vec_t ro(m);
    ro.make_random();

    point_t A = c_point_0;
    point_t B = c_point_0;

    result.Pk.clear();

    bool r = false, r2 = false;
    for(size_t j = 0; j < m; ++j)
    {
      for(size_t i = 0; i < n; ++i)
      {
        const point_t& gen_1 = get_BGE_generator((j * n + i) * 2 + 0, r);
        const point_t& gen_2 = get_BGE_generator((j * n + i) * 2 + 1, r2);
        CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(r && r2, 5);
        const scalar_t& a = a_mat(j, i);
        A += a * gen_1 - a * a * gen_2;
        if (l_digits[j] == i)
          B += gen_1 - a * gen_2;
        else
          B += a * gen_2;
      }

      point_t Pk = c_point_0;
      for(size_t i = 0; i < ring_size; ++i)
        Pk += coeffs[j * N + i] * ring[i];
      for(size_t i = ring_size; i < N; ++i)
        Pk += coeffs[j * N + i] * ring[ring_size - 1];

      Pk += ro[j] * c_point_X;
      result.Pk.emplace_back(std::move((c_scalar_1div8 * Pk).to_public_key()));
    }
    
    A += r_A * c_point_X;
    result.A = (c_scalar_1div8 * A).to_public_key();
    B += r_B * c_point_X;
    result.B = (c_scalar_1div8 * B).to_public_key();

    hash_helper_t::hs_t hsc(1 + ring_size + 2 + m);
    hsc.add_hash(context_hash);
    for(auto& ring_el : ring)
      hsc.add_point(c_scalar_1div8 * ring_el);
    hsc.add_pub_key(result.A);
    hsc.add_pub_key(result.B);
    hsc.add_pub_keys_array(result.Pk);
    scalar_t x = hsc.calc_hash();
    DBG_VAL_PRINT(x);

    result.f.resize(m * (n - 1));
    for(size_t j = 0; j < m; ++j)
    {
      for(size_t i = 1; i < n; ++i)
      {
        result.f[j * (n - 1) + i - 1] = a_mat(j, i);
        if (l_digits[j] == i)
          result.f[j * (n - 1) + i - 1] += x;
      }
    }

    result.y = r_A + x * r_B;
    
    result.z = 0;
    scalar_t x_power = c_scalar_1;
    for(size_t k = 0; k < m; ++k)
    {
      result.z -= x_power * ro[k];
      x_power *= x;
    }
    result.z += secret * x_power;

    return true;
  }
#undef CHECK_AND_FAIL_WITH_ERROR_IF_FALSE

  //---------------------------------------------------------------


#define CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(cond, err_code) \
    if (!(cond)) { LOG_PRINT_RED("generate_BGE_proof: \"" << #cond << "\" is false at " << LOCATION_SS << ENDL << "error code = " << (int)err_code, LOG_LEVEL_3); \
    if (p_err) { *p_err = err_code; } return false; }

  bool verify_BGE_proof(const hash& context_hash, const std::vector<const public_key*>& ring, const BGE_proof& sig, uint8_t* p_err /* = nullptr */)
  {
    static constexpr size_t n = 4; // TODO: @#@# move it out

    DBG_PRINT(" - - - verify_BGE_proof - - -");
    size_t ring_size = ring.size();
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(ring_size > 0, 0);

    const size_t m = std::max(static_cast<uint64_t>(1), constexpr_ceil_log_n(ring_size, n));
    const size_t N = constexpr_pow(m, n);
    //const size_t mn = m * n;

    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(sig.Pk.size() == m, 1);
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(sig.f.size() == m * (n - 1), 2);

    hash_helper_t::hs_t hsc(1 + ring_size + 2 + m);
    hsc.add_hash(context_hash);
    for(const public_key* ppk : ring)
      hsc.add_pub_key(*ppk);
    hsc.add_pub_key(sig.A);
    hsc.add_pub_key(sig.B);
    hsc.add_pub_keys_array(sig.Pk);
    scalar_t x = hsc.calc_hash();
    DBG_VAL_PRINT(x);

    scalar_vec_t f0(m); // the first column  f_{i,0} = x - sum{j=1}{n-1}( f_{i,j} )
    for(size_t j = 0; j < m; ++j)
    {
      f0[j] = x;
      for(size_t i = 1; i < n; ++i)
        f0[j] -= sig.f[j * (n - 1) + i - 1];
    }

    //
    // 1
    //
    point_t A = point_t(sig.A).modify_mul8();
    point_t B = point_t(sig.B).modify_mul8();

    point_t Z = A + x * B;

    bool r = false, r2 = false;
    for(size_t j = 0; j < m; ++j)
    {
      for(size_t i = 0; i < n; ++i)
      {
        const point_t& gen_1 = get_BGE_generator((j * n + i) * 2 + 0, r);
        const point_t& gen_2 = get_BGE_generator((j * n + i) * 2 + 1, r2);
        CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(r && r2, 5);
        const scalar_t& f_ji = (i == 0) ? f0[j] : sig.f[j * (n - 1) + i - 1];
        
        Z -= f_ji * gen_1 + f_ji * (x - f_ji) * gen_2;
      }
    }
    Z -= sig.y * c_point_X;
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(Z.is_zero(), 100);

    //
    // 2
    //
    scalar_vec_t p_vec(N);
    for(size_t i = 0; i < N; ++i)
    {
      p_vec[i] = c_scalar_1;
      size_t i_tmp = i;
      for(size_t j = 0; j < m; ++j)
      {
        size_t i_j = i_tmp % n;                     // j-th digit of i
        i_tmp /= n;
        const scalar_t& f_jij = (i_j == 0) ? f0[j] : sig.f[j * (n - 1) + i_j - 1];
        p_vec[i] *= f_jij;
      }
    }

    for(size_t i = 0; i < ring_size; ++i)
      Z += p_vec[i] * point_t(*ring[i]).modify_mul8();
    for(size_t i = ring_size; i < N; ++i)
      Z += p_vec[i] * point_t(*ring[ring_size - 1]).modify_mul8();

    scalar_t x_power = c_scalar_1;
    for(size_t k = 0; k < m; ++k)
    {
      Z -= x_power * point_t(sig.Pk[k]).modify_mul8();
      x_power *= x;
    }

    Z -= sig.z * c_point_X;
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(Z.is_zero(), 101);

    return true;
  }

#undef CHECK_AND_FAIL_WITH_ERROR_IF_FALSE


} // namespace crypto
