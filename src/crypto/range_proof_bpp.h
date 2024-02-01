// Copyright (c) 2021-2022 Zano Project (https://zano.org/)
// Copyright (c) 2021-2022 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

//
// This file contains the implementation of range proof protocol.
// Namely, Bulletproofs+ https://eprint.iacr.org/2020/735
//

namespace crypto
{
  struct bpp_signature
  {
    std::vector<public_key> L;  // size = ceil( log_2(m * n) )
    std::vector<public_key> R;
    public_key A0;
    public_key A;
    public_key B;
    scalar_t r;
    scalar_t s;
    scalar_t delta;
  };

#if 0
#  define DBG_VAL_PRINT(x) std::cout << std::setw(30) << std::left << #x ": " << x << std::endl
#  define DBG_PRINT(x)     std::cout << x << std::endl
#else
#  define DBG_VAL_PRINT(x) (void(0))
#  define DBG_PRINT(x)     (void(0))
#endif


#define CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(cond, err_code) \
  if (!(cond)) { LOG_PRINT_RED("bpp_gen: \"" << #cond << "\" is false at " << LOCATION_SS << ENDL << "error code = " << (int)err_code, LOG_LEVEL_3); \
  if (p_err) { *p_err = err_code; } return false; }


  template<typename CT>
  bool bpp_gen(const scalar_vec_t& values, const scalar_vec_t& masks, const std::vector<const crypto::public_key*>& commitments_1div8, bpp_signature& sig, uint8_t* p_err = nullptr)
  {
    // Note: commitments_1div8 are supposed to be already calculated
    static_assert(CT::c_bpp_n <= 255, "too big N");
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(values.size() > 0 && values.size() <= CT::c_bpp_values_max && values.size() == masks.size() && values.size() == commitments_1div8.size(), 1);
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(masks.is_reduced(), 3);

    const size_t c_bpp_log2_m = constexpr_ceil_log2(values.size());
    const size_t c_bpp_m = 1ull << c_bpp_log2_m;
    const size_t c_bpp_mn = c_bpp_m * CT::c_bpp_n;
    const size_t c_bpp_log2_mn = c_bpp_log2_m + CT::c_bpp_log2_n;

#ifndef NDEBUG
    for(size_t i = 0; i < values.size(); ++i)
    {
      point_t V{};
      CT::calc_pedersen_commitment(values[i], masks[i], V);
      CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(point_t(*commitments_1div8[i]).modify_mul8() == V, 4);
    }
#endif

    // s.a. BP+ paper, page 15, eq. 11
    // decompose v into aL and aR:
    //   v = aL o (1, 2, 2^2, ..., 2^n-1),  o - component-wise product aka Hadamard product
    //   aR = aL - (1, 1, ... 1)
    //   aR o aL = 0

    // aLs = (aL_0, aL_1, ..., aL_m-1) -- `bit` matrix of c_bpp_m x c_bpp_n, each element is a scalar

    scalar_mat_t<CT::c_bpp_n> aLs(c_bpp_mn), aRs(c_bpp_mn);
    aLs.zero();
    aRs.zero();
    // m >= values.size, first set up [0..values.size-1], then -- [values.size..m-1]  (padding area) 
    for (size_t i = 0; i < values.size(); ++i)
    {
      const scalar_t& v = values[i];
      for (uint8_t j = 0; j < CT::c_bpp_n; ++j)
      {
        if (v.get_bit(j))
          aLs(i, j) = c_scalar_1;    // aL = 1, aR = 0
        else
          aRs(i, j) = c_scalar_Lm1;  // aL = 0, aR = -1
      }
    }

    for (size_t i = values.size(); i < c_bpp_m; ++i)
      for (size_t j = 0; j < CT::c_bpp_n; ++j)
        aRs(i, j) = c_scalar_Lm1;  // aL = 0, aR = -1


    // using e as Fiat-Shamir transcript
    scalar_t e = CT::get_initial_transcript();
    DBG_PRINT("initial transcript: " << e);

    hash_helper_t::hs_t hsc;
    CT::update_transcript(hsc, e, commitments_1div8);

    // BP+ paper, page 15: The prover begins with sending A = g^aL h^aR h^alpha (group element)
    // so we calculate A0 = alpha * H + SUM(aL_i * G_i) + SUM(aR_i * H_i)

    scalar_t alpha = scalar_t::random();
    point_t A0 = alpha * CT::bpp_H;

    for (size_t i = 0; i < c_bpp_mn; ++i)
      A0 += aLs[i] * CT::get_generator(false, i) + aRs[i] * CT::get_generator(true, i);

    // pre-multiply all output points by c_scalar_1div8
    // in order to enforce these points to be in the prime-order subgroup (after mul by 8 in bpp_verify())
    A0 *= c_scalar_1div8;
    A0.to_public_key(sig.A0);

    DBG_VAL_PRINT(alpha);
    DBG_VAL_PRINT(A0);

    // calculate scalar challenges y and z
    hsc.add_scalar(e);
    hsc.add_pub_key(sig.A0);
    scalar_t y = hsc.calc_hash();
    scalar_t z = hash_helper_t::hs(y);
    e = z; // transcript for further steps
    DBG_VAL_PRINT(y);
    DBG_VAL_PRINT(z);

    // Computing vector d for aggregated version of the protocol (BP+ paper, page 17)
    // (note: elements are stored column-by-column in memory)
    // d = | 1       * z^(2*1),        1 * z^(2*2),        1 * z^(2*3),      ...,        1 * z^(2*m)  |
    //     | 2       * z^(2*1),        2 * z^(2*2),        2 * z^(2*3),      ...,        2 * z^(2*m)  |
    //     | 4       * z^(2*1),        4 * z^(2*2),        4 * z^(2*3),      ...,        4 * z^(2*m)  |
    //     | .......................................................................................  |
    //     | 2^(n-1) * z^(2*1),  2^(n-1) * z^(2*2),  2^(n-1) * z^(2*3),      ...,  2^(n-1) * z^(2*m)) |
    // Note: sum(d_i) = (2^n - 1) * ((z^2)^1 + (z^2)^2 + ... (z^2)^m)) = (2^n-1) * sum_of_powers(x^2, log(m))

    scalar_t z_sq = z * z;
    scalar_mat_t<CT::c_bpp_n> d(c_bpp_mn);
    d(0, 0) = z_sq;
    // first row
    for (size_t i = 1; i < c_bpp_m; ++i)
      d(i, 0) = d(i - 1, 0) * z_sq;
    // all rows
    for (size_t j = 1; j < CT::c_bpp_n; ++j)
      for (size_t i = 0; i < c_bpp_m; ++i)
        d(i, j) = d(i, j - 1) + d(i, j - 1);

    DBG_PRINT("Hs(d): " << d.calc_hs());

    // calculate extended Vandermonde vector y = (1, y, y^2, ..., y^(mn+1))   (BP+ paper, page 18, Fig. 3)
    // (calculate two more elements (1 and y^(mn+1)) for convenience)
    scalar_vec_t y_powers(c_bpp_mn + 2);
    y_powers[0] = 1;
    for (size_t i = 1; i <= c_bpp_mn + 1; ++i)
      y_powers[i] = y_powers[i - 1] * y;

    const scalar_t& y_mn_p1 = y_powers[c_bpp_mn + 1];

    DBG_PRINT("Hs(y_powers): " << y_powers.calc_hs());

    // aL_hat = aL - 1*z
    scalar_vec_t aLs_hat = aLs - z;
    // aR_hat = aR + d o y^leftarr + 1*z where y^leftarr = (y^n, y^(n-1), ..., y)  (BP+ paper, page 18, Fig. 3)
    scalar_vec_t aRs_hat = aRs + z;
    for (size_t i = 0; i < c_bpp_mn; ++i)
      aRs_hat[i] += d[i] * y_powers[c_bpp_mn - i];

    DBG_PRINT("Hs(aLs_hat): " << aLs_hat.calc_hs());
    DBG_PRINT("Hs(aRs_hat): " << aRs_hat.calc_hs());

    // calculate alpha_hat
    // alpha_hat = alpha + SUM(z^(2j) * gamma_j * y^(mn+1)) for j = 1..m
    // i.e. \hat{\alpha} = \alpha + y^{m n+1} \sum_{j = 1}^{m} z^{2j} \gamma_j
    scalar_t alpha_hat = 0;
    for (size_t i = 0; i < masks.size(); ++i)
      alpha_hat += d(i, 0) * masks[i];
    alpha_hat = alpha + y_mn_p1 * alpha_hat;

    DBG_VAL_PRINT(alpha_hat);

    // calculate 1, y^-1, y^-2, ...
    const scalar_t y_inverse = y.reciprocal();
    scalar_vec_t y_inverse_powers(c_bpp_mn / 2 + 1); // the greatest power we need is c_bpp_mn/2 (at the first reduction round)
    y_inverse_powers[0] = 1;
    for (size_t i = 1, size = y_inverse_powers.size(); i < size; ++i)
      y_inverse_powers[i] = y_inverse_powers[i - 1] * y_inverse;

    // prepare generator's vector
    std::vector<point_t> g(c_bpp_mn), h(c_bpp_mn);
    for (size_t i = 0; i < c_bpp_mn; ++i)
    {
      g[i] = CT::get_generator(false, i);
      h[i] = CT::get_generator(true, i);
    }

    // WIP zk-argument called with zk-WIP(g, h, G, H, A_hat, aL_hat, aR_hat, alpha_hat)

    scalar_vec_t& a = aLs_hat;
    scalar_vec_t& b = aRs_hat;

    sig.L.resize(c_bpp_log2_mn);
    sig.R.resize(c_bpp_log2_mn);

    // zk-WIP reduction rounds (s.a. the preprint page 13 Fig. 1)
    for (size_t n = c_bpp_mn / 2, ni = 0; n >= 1; n /= 2, ++ni)
    {
      DBG_PRINT(ENDL << "#" << ni);

      // zk-WIP(g, h, G, H, P, a, b, alpha)

      scalar_t dL = scalar_t::random();
      DBG_VAL_PRINT(dL);
      scalar_t dR = scalar_t::random();
      DBG_VAL_PRINT(dR);

      // a = (a1, a2),  b = (b1, b2)                      -- vectors of scalars
      // cL = <a1, ((y, y^2, ...) o b2)>                  -- scalar
      scalar_t cL = 0;
      for (size_t i = 0; i < n; ++i)
        cL += a[i] * y_powers[i + 1] * b[n + i];

      DBG_VAL_PRINT(cL);

      // cR = <a2, ((y, y^2, ...) o b1)> * y^n            -- scalar
      scalar_t cR = 0;
      for (size_t i = 0; i < n; ++i)
        cR += a[n + i] * y_powers[i + 1] * b[i];
      cR *= y_powers[n];

      DBG_VAL_PRINT(cR);

      // L = y^-n * a1 * g2 + b2 * h1 + cL * G + dL * H   -- point
      point_t sum = c_point_0;
      for (size_t i = 0; i < n; ++i)
        sum += a[i] * g[n + i];
      point_t L;
      CT::calc_pedersen_commitment(cL, dL, L);
      for (size_t i = 0; i < n; ++i)
        L += b[n + i] * h[i];
      L += y_inverse_powers[n] * sum;
      L *= c_scalar_1div8;
      DBG_VAL_PRINT(L);

      // R = y^n  * a2 * g1 + b1 * h2 + cR * G + dR * H   -- point
      sum.zero();
      for (size_t i = 0; i < n; ++i)
        sum += a[n + i] * g[i];
      point_t R;
      CT::calc_pedersen_commitment(cR, dR, R);
      for (size_t i = 0; i < n; ++i)
        R += b[i] * h[n + i];
      R += y_powers[n] * sum;
      R *= c_scalar_1div8;
      DBG_VAL_PRINT(R);

      // put L, R to the sig
      L.to_public_key(sig.L[ni]);
      R.to_public_key(sig.R[ni]);

      // update the transcript
      hsc.add_scalar(e);
      hsc.add_pub_key(sig.L[ni]);
      hsc.add_pub_key(sig.R[ni]);
      e = hsc.calc_hash();
      DBG_VAL_PRINT(e);

      // recalculate arguments for the next round
      scalar_t e_squared = e * e;
      scalar_t e_inverse = e.reciprocal();
      scalar_t e_inverse_squared = e_inverse * e_inverse;
      scalar_t e_y_inv_n = e * y_inverse_powers[n];
      scalar_t e_inv_y_n = e_inverse * y_powers[n];

      // g_hat = e^-1 * g1 + (e * y^-n) * g2              -- vector of points
      for (size_t i = 0; i < n; ++i)
        g[i] = e_inverse * g[i] + e_y_inv_n * g[n + i];

      // h_hat = e * h1 + e^-1 * h2                       -- vector of points
      for (size_t i = 0; i < n; ++i)
        h[i] = e * h[i] + e_inverse * h[n + i];

      // P_hat = e^2 * L + P + e^-2 * R                   -- point

      // a_hat = e * a1 + e^-1 * y^n * a2                 -- vector of scalars
      for (size_t i = 0; i < n; ++i)
        a[i] = e * a[i] + e_inv_y_n * a[n + i];

      // b_hat = e^-1 * b1 + e * b2                       -- vector of scalars
      for (size_t i = 0; i < n; ++i)
        b[i] = e_inverse * b[i] + e * b[n + i];

      // alpha_hat = e^2 * dL + alpha + e^-2 * dR         -- scalar
      alpha_hat += e_squared * dL + e_inverse_squared * dR;

      // run next iteraton zk-WIP(g_hat, h_hat, G, H, P_hat, a_hat, b_hat, alpha_hat)
    }
    DBG_PRINT("");

    // zk-WIP last round
    scalar_t r = scalar_t::random();
    scalar_t s = scalar_t::random();
    scalar_t delta = scalar_t::random();
    scalar_t eta = scalar_t::random();
    DBG_VAL_PRINT(r);
    DBG_VAL_PRINT(s);
    DBG_VAL_PRINT(delta);
    DBG_VAL_PRINT(eta);

    // A = r * g + s * h + (r y b + s y a) * G + delta * H -- point
    point_t A = c_point_0;
    CT::calc_pedersen_commitment(y * (r * b[0] + s * a[0]), delta, A);
    A += r * g[0] + s * h[0];
    A *= c_scalar_1div8;
    A.to_public_key(sig.A);
    DBG_VAL_PRINT(A);

    // B = (r * y * s) * G + eta * H
    point_t B = c_point_0;
    CT::calc_pedersen_commitment(r * y * s, eta, B);
    B *= c_scalar_1div8;
    B.to_public_key(sig.B);
    DBG_VAL_PRINT(B);

    // update the transcript
    hsc.add_scalar(e);
    hsc.add_pub_key(sig.A);
    hsc.add_pub_key(sig.B);
    e = hsc.calc_hash();
    DBG_VAL_PRINT(e);

    // finalize the signature
    sig.r = r + e * a[0];
    sig.s = s + e * b[0];
    sig.delta = eta + e * delta + e * e * alpha_hat;
    DBG_VAL_PRINT(sig.r);
    DBG_VAL_PRINT(sig.s);
    DBG_VAL_PRINT(sig.delta);

    return true;
  } // bpp_gen()

  
  // convenient overload for tests 
  template<typename CT>
  bool bpp_gen(const scalar_vec_t& values, const scalar_vec_t& masks, bpp_signature& sig, std::vector<point_t>& commitments_1div8_to_be_generated, uint8_t* p_err = nullptr)
  {
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(values.size() == masks.size(), 91);
    commitments_1div8_to_be_generated.resize(values.size());
    std::vector<crypto::public_key> commitments_1div8(values.size());
    std::vector<const crypto::public_key*> commitments_1div8_pointers(values.size());
    for(size_t i = 0; i < values.size(); ++i)
    {
      CT::calc_pedersen_commitment(c_scalar_1div8 * values[i], c_scalar_1div8 * masks[i], commitments_1div8_to_be_generated[i]);
      commitments_1div8[i] = (commitments_1div8_to_be_generated[i]).to_public_key();
      commitments_1div8_pointers[i] = &commitments_1div8[i];
    }
    return bpp_gen<CT>(values, masks, commitments_1div8_pointers, sig, p_err);
  }
#undef CHECK_AND_FAIL_WITH_ERROR_IF_FALSE


  struct bpp_sig_commit_ref_t
  {
    bpp_sig_commit_ref_t(const bpp_signature& sig, const std::vector<point_t>& commitments)
      : sig(sig)
      , commitments(commitments)
    {}
    const bpp_signature& sig;
    const std::vector<point_t>& commitments; // assumed to be premultiplied by 1/8
  };


  template<typename CT>
  bool bpp_verify(const std::vector<bpp_sig_commit_ref_t>& sigs, uint8_t* p_err = nullptr)
  {
#define CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(cond, err_code) \
    if (!(cond)) { LOG_PRINT_RED("bpp_verify: \"" << #cond << "\" is false at " << LOCATION_SS << ENDL << "error code = " << (int)err_code, LOG_LEVEL_3); \
    if (p_err) { *p_err = err_code; } return false; }

    DBG_PRINT(ENDL << " . . . . bpp_verify() . . . . ");

    static_assert(CT::c_bpp_n <= 255, "too big N");
    const size_t kn = sigs.size();
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(kn > 0, 1);

    struct intermediate_element_t
    {
      scalar_t y;
      scalar_t z;
      scalar_t z_sq;
      scalar_vec_t e;
      scalar_vec_t e_sq;
      scalar_t e_final;
      scalar_t e_final_sq;
      size_t inv_e_offset; // offset in batch_for_inverse
      size_t inv_y_offset; // offset in batch_for_inverse
      size_t c_bpp_log2_m;
      size_t c_bpp_m;
      size_t c_bpp_mn;
      point_t A;
      point_t A0;
      point_t B;
      std::vector<point_t> L;
      std::vector<point_t> R;
    };
    std::vector<intermediate_element_t> interms(kn);

    size_t c_bpp_log2_m_max = 0;
    for (size_t k = 0; k < kn; ++k)
    {
      const bpp_sig_commit_ref_t& bsc = sigs[k];
      const bpp_signature& sig = bsc.sig;
      CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(bsc.commitments.size() > 0, 2);
      CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(sig.L.size() > 0 && sig.L.size() == sig.R.size(), 3);
      CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(sig.r.is_reduced() && sig.s.is_reduced() && sig.delta.is_reduced(), 4);

      intermediate_element_t& interm = interms[k];
      interm.c_bpp_log2_m = constexpr_ceil_log2(bsc.commitments.size());
      if (c_bpp_log2_m_max < interm.c_bpp_log2_m)
        c_bpp_log2_m_max = interm.c_bpp_log2_m;

      CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(sig.L.size() == interm.c_bpp_log2_m + CT::c_bpp_log2_n, 5);

      interm.c_bpp_m = 1ull << interm.c_bpp_log2_m;
      interm.c_bpp_mn = interm.c_bpp_m * CT::c_bpp_n;

      CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(interm.A0.from_public_key(sig.A0), 6);
      CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(interm.A.from_public_key(sig.A), 7);
      CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(interm.B.from_public_key(sig.B), 8);
      interm.L.resize(sig.L.size());
      interm.R.resize(sig.R.size());
      for (size_t i = 0; i < interm.L.size(); ++i)
      {
        CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(interm.L[i].from_public_key(sig.L[i]), 9);
        CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(interm.R[i].from_public_key(sig.R[i]), 10);
      }
    }
    const size_t c_bpp_m_max = 1ull << c_bpp_log2_m_max;
    const size_t c_bpp_mn_max = c_bpp_m_max * CT::c_bpp_n;
    const size_t c_bpp_LR_size_max = c_bpp_log2_m_max + CT::c_bpp_log2_n;


    //
    // prepare stuff
    //
    /*
    std::vector<point_t> g(c_bpp_mn_max), h(c_bpp_mn_max);
    for (size_t i = 0; i < c_bpp_mn_max; ++i)
    {
      g[i] = CT::get_generator(false, i);
      h[i] = CT::get_generator(true, i);
    }
    */

    scalar_vec_t batch_for_inverse;
    batch_for_inverse.reserve(kn + kn * c_bpp_LR_size_max);


    for (size_t k = 0; k < kn; ++k)
    {
      DBG_PRINT(ENDL << "SIG #" << k);
      const bpp_sig_commit_ref_t& bsc = sigs[k];
      const bpp_signature& sig = bsc.sig;
      intermediate_element_t& interm = interms[k];

      // restore y and z
      // using e as Fiat-Shamir transcript
      scalar_t e = CT::get_initial_transcript();
      DBG_PRINT("initial transcript: " << e);
      hash_helper_t::hs_t hsc;
      CT::update_transcript(hsc, e, bsc.commitments);
      // calculate scalar challenges y and z
      hsc.add_scalar(e);
      hsc.add_pub_key(sig.A0);
      hsc.assign_calc_hash(interm.y);
      interm.z = hash_helper_t::hs(interm.y);
      interm.z_sq = interm.z * interm.z;
      DBG_VAL_PRINT(interm.y);
      DBG_VAL_PRINT(interm.z);
      e = interm.z; // transcript for further steps

      interm.inv_y_offset = batch_for_inverse.size();
      batch_for_inverse.push_back(interm.y);
      interm.inv_e_offset = batch_for_inverse.size();

      interm.e.resize(sig.L.size());
      interm.e_sq.resize(sig.L.size());

      for (size_t i = 0; i < sig.L.size(); ++i)
      {
        hsc.add_scalar(e);
        hsc.add_pub_key(sig.L[i]);
        hsc.add_pub_key(sig.R[i]);
        hsc.assign_calc_hash(e);
        interm.e[i] = e;
        interm.e_sq[i] = e * e;
        DBG_PRINT("e[" << i << "]: " << e);
        batch_for_inverse.push_back(e);
      }

      hsc.add_scalar(e);
      hsc.add_pub_key(sig.A);
      hsc.add_pub_key(sig.B);
      hsc.assign_calc_hash(interm.e_final);
      interm.e_final_sq = interm.e_final * interm.e_final;
      DBG_VAL_PRINT(interm.e_final);
    }

    batch_for_inverse.invert();

    // Notation:
    // 1_vec ^ n = (1, 1, 1, ..., 1)
    // 2_vec ^ n = (2^0, 2^1, 2^2, ..., 2^(n-1))
    // -1_vec ^ n = ((-1)^0, (-1)^1, (-1)^2, ... (-1)^(n-1)) = (1, -1, 1, -1, ...)
    // y<^n = (y^n, y^(n-1), ..., y^1)
    // y>^n = (y^1, y^2, ..., y^n)

    // from page 13, Fig 1:
    // Verifier outputs Accept IFF the following equality holds (single proof):
    // P^e^2 * A^e * B == g ^ (r' e) * h ^ (s' e) * G ^ (r' y s') * H ^ delta'
    //   (where g and h are calculated in each round)
    // The same equation in additive notation:
    // e^2 * P + e * A + B == (r' * e) * g + (s' * e) * h + (r' y s') * G + delta' * H
    // <=>
    // (r' * e) * g + (s' * e) * h + (r' y s') * G + delta' * H   -   e^2 * P - e * A - B == 0     (*)
    // where A, B, r', s', delta' is taken from the signature
    // and  P_{k+1} = e^2 * L_k + P_k + e^-2 * R_k  for all rounds 
    //
    // from page 18, Fig 3:
    // P and V computes:
    // A_hat = A0   +   (- 1^(mn) * z) * g   +   (d o y<^(mn) + 1^(mn) * z) * h   +  
    //       + y^(mn+1) * (SUM{j=1..m} z^(2j) * V_j)   +
    //       + (z*SUM(y^>mn) - z*y^(mn+1)*SUM(d) - z^2 * SUM(y^>mn)) * G 
    // (calculated once)
    //
    // As suggested in Section 6.1 "Practical Optimizations":
    // 1) g and h exponentianions can be optimized in order not to be calculated at each round as the following (page 20):
    //
    //   (r' * e * s_vec) * g + (s' * e * s'_vec) * h + (r' y s') * G + delta' * H -
    //   - e^2 * A_hat
    //   - SUM{j=1..log(n)}(e_final^2 * e_j^2 * L_j   + e_final^2 * e_j^-2 * R_j)
    //   - e * A - B   =   0                                                                       (**)
    //
    // where:
    //   g, h - vector of fixed generators
    //   s_vec_i = y^(1-i) * PROD{j=1..log(n)}(e_j ^ b(i,j))
    //   s'_vec_i = PROD{j=1..log(n)}(e_j ^ -b(i,j))
    //   b(i, j) = { 2 * ((1<<(j-1)) & (i-1)) - 1)  (counting both from 1) (page 20)
    //   b(i, j) = { 2 * ((1<<j) & i) - 1)  (counting both from 0)
    //
    // 2) we gonna aggregate all (**) for each round by multiplying them to a random weights and then sum up
    // insert A_hat into (**) =>

    //   (r' * e * s_vec) * g + (s' * e * s'_vec) * h + (r' y s') * G + delta' * H -
    //   - e^2 * (A0 + (- 1^(mn) * z) * g + (d o y<^(mn) + 1^(mn) * z) * h +
    //       + y^(mn+1) * (SUM{j=1..m} z^(2j) * V_j)   +
    //       + (z*SUM(y^>mn) - z*y^(mn+1)*SUM(d) - z^2 * SUM(y^>mn)) * G
    //     )
    //   - SUM{j=1..log(n)}(e_final^2 * e_j^2 * L_j   + e_final^2 * e_j^-2 * R_j)
    //   - e * A - B   =   0                                                                       

    // =>

    // (for single signature)
    //
    //   (r' * e * s_vec   - e^2 * (- 1_vec^(mn) * z))                         * g     | these are
    // + (s' * e * s'_vec  - e^2 * (d o y<^(mn) + 1_vec^(mn) * z))             * h     | fixed generators 
    // + (r' y s' - e^2 * ((z - z^2)*SUM(y^>mn) - z*y^(mn+1)*SUM(d))           * G     | across all
    // + delta'                                                                * H     | the signatures
    //
    // - e^2 * A0
    // - e^2 * y^(mn+1) * (SUM{j=1..m} z^(2j) * V_j))
    // - e^2 * SUM{j=1..log(n)}(e_j^2 * L_j  + e_j^-2 * R_j)
    // - e * A - B   =   0                                                                         (***)
    //
    // All (***) will be muptiplied by random weightning factor and then summed up.

    // Calculate cummulative sclalar multiplicand for fixed generators across all the sigs.
    scalar_vec_t g_scalars;
    g_scalars.resize(c_bpp_mn_max, 0);
    scalar_vec_t h_scalars;
    h_scalars.resize(c_bpp_mn_max, 0);
    scalar_t G_scalar = 0;
    scalar_t H_scalar = 0;
    point_t summand = c_point_0;

    for (size_t k = 0; k < kn; ++k)
    {
      DBG_PRINT(ENDL << "SIG #" << k);
      const bpp_sig_commit_ref_t& bsc = sigs[k];
      const bpp_signature& sig = bsc.sig;
      intermediate_element_t& interm = interms[k];

      // random weightning factor for speed-optimized batch verification (preprint page 20)
      const scalar_t rwf = scalar_t::random();
      DBG_PRINT("rwf: " << rwf);

      // prepare d vector (see also d structure description in proof function)
      scalar_mat_t<CT::c_bpp_n> d(interm.c_bpp_mn);
      d(0, 0) = interm.z_sq;
      // first row
      for (size_t i = 1; i < interm.c_bpp_m; ++i)
        d(i, 0) = d(i - 1, 0) * interm.z_sq;
      // all rows
      for (size_t j = 1; j < CT::c_bpp_n; ++j)
        for (size_t i = 0; i < interm.c_bpp_m; ++i)
          d(i, j) = d(i, j - 1) + d(i, j - 1);
      // sum(d) (see also note in proof function for this)
      const scalar_t sum_d = CT::get_2_to_the_power_of_N_minus_1() * sum_of_powers(interm.z_sq, interm.c_bpp_log2_m);

      DBG_PRINT("Hs(d): " << d.calc_hs());
      DBG_PRINT("sum(d): " << sum_d);

      const scalar_t& y_inv = batch_for_inverse[interm.inv_y_offset];
      auto get_e_inv = [&](size_t i) { return batch_for_inverse[interm.inv_e_offset + i]; }; // i belongs to [0; L.size()-1]

      // prepare s_vec (unlike the paper here we moved y-component out of s_vec for convenience, so s_vec'[x] = s_vec[~x & (MN-1)])
      // complexity (sc_mul's): MN+2*log2(MN)-2
      // the idea is the following:
      // s_vec[00000b] = ... * (e_4)^-1 * (e_3)^-1 * (e_2)^-1 * (e_1)^-1 * (e_0)^-1
      // s_vec[00101b] = ... * (e_4)^-1 * (e_3)^-1 * (e_2)^+1 * (e_1)^-1 * (e_0)^+1
      const size_t log2_mn = sig.L.size(); // at the beginning we made sure that sig.L.size() == c_bpp_log2_m + c_bpp_log2_n
      scalar_vec_t s_vec(interm.c_bpp_mn);
      s_vec[0] = get_e_inv(0);
      for (size_t i = 1; i < log2_mn; ++i)
        s_vec[0] *= get_e_inv(i);          // s_vec[0] = (e_0)^-1 * (e_1)^-1 * .. (e_{log2_mn-1})^-1 
      DBG_PRINT("[0] " << s_vec[0]);
      for (size_t i = 1; i < interm.c_bpp_mn; ++i)
      {
        size_t base_el_index = i & (i - 1); // base element index: 0, 0, 2, 0, 4, 4, 6, 0, 8, 8, 10... base element differs in one bit (0) from the current one (1)
        size_t bit_index = log2_mn - calc_lsb_32((uint32_t)i) - 1; // the bit index where current element has the difference with the base
        s_vec[i] = s_vec[base_el_index] * interm.e_sq[bit_index]; // (e_j)^-1 * (e_j)^2 = (e_j)^+1
        DBG_PRINT("[" << i << "] " << " " << base_el_index << ", " << bit_index << " : " << s_vec[i]);
      }

      // prepare y_inv vector
      scalar_vec_t y_inverse_powers(interm.c_bpp_mn);
      y_inverse_powers[0] = 1;
      for (size_t i = 1; i < interm.c_bpp_mn; ++i)
        y_inverse_powers[i] = y_inverse_powers[i - 1] * y_inv;

      // y^(mn+1)
      scalar_t y_power_mnp1 = interm.y;
      for (size_t i = 0; i < log2_mn; ++i)
        y_power_mnp1 *= y_power_mnp1;
      y_power_mnp1 *= interm.y;
      DBG_VAL_PRINT(y_power_mnp1);

      // now calculate all multiplicands for common generators

      // g vector multiplicands:
      // rwf * (r' * e * (1, y^-1, y^-2, ...) o s_vec   + e^2 * z) =
      // rwf * r' * e * ((1, y^-1, ...) o s_vec) + rwf * e^2 * z * (1, 1, ...)
      scalar_t rwf_e_sq_z = rwf * interm.e_final_sq * interm.z;
      scalar_t rwf_r_e = rwf * interm.e_final * sig.r;
      for (size_t i = 0; i < interm.c_bpp_mn; ++i)
        g_scalars[i] += rwf_r_e * y_inverse_powers[i] * s_vec[i] + rwf_e_sq_z;

      DBG_PRINT("Hs(g_scalars): " << g_scalars.calc_hs());

      // h vector multiplicands:
      // rwf * (s' * e * s'_vec  - e^2 * (d o y<^(mn) + 1_vec^(mn) * z))
      // rwf * s' * e * s'_vec - rwf * e^2 * z * (1, 1...) - rwf * e^2 * (d o y<^(mn))
      //scalar_t rwf_e_sq_z = rwf * interm.e_final_sq * interm.z;
      scalar_t rwf_s_e = rwf * sig.s * interm.e_final;
      scalar_t rwf_e_sq_y = rwf * interm.e_final_sq * interm.y;
      for (size_t i = interm.c_bpp_mn - 1; i != SIZE_MAX; --i)
      {
        h_scalars[i] += rwf_s_e * s_vec[interm.c_bpp_mn - 1 - i] - rwf_e_sq_z - rwf_e_sq_y * d[i];
        rwf_e_sq_y *= interm.y;
      }

      DBG_PRINT("Hs(h_scalars): " << h_scalars.calc_hs());

      // G point multiplicands:
      // rwf * (r' y s' - e ^ 2 * ((z - z ^ 2)*SUM(y^>mn) - z * y^(mn+1) * SUM(d)) = 
      // = rwf * r' y s' - rwf * e^2 * (z - z ^ 2)*SUM(y^>mn) + rwf * e^2 * z * y^(mn+1) * SUM(d)
      G_scalar += rwf * sig.r * interm.y * sig.s + rwf_e_sq_y * sum_d * interm.z;
      G_scalar -= rwf * interm.e_final_sq * (interm.z - interm.z_sq) * sum_of_powers(interm.y, log2_mn);
      DBG_PRINT("sum_y: " << sum_of_powers(interm.y, log2_mn));
      DBG_PRINT("G_scalar: " << G_scalar);

      // H point multiplicands:
      // rwf * delta
      H_scalar += rwf * sig.delta;
      DBG_PRINT("H_scalar: " << H_scalar);

      // uncommon generators' multiplicands
      point_t summand_8 = c_point_0; // this summand to be multiplied by 8 before adding to the main summand
      // - rwf * e^2 * A0
      summand_8 -= rwf * interm.e_final_sq * interm.A0;
      DBG_PRINT("A0_scalar: " << c_scalar_Lm1 * interm.e_final_sq * rwf);

      // - rwf * e^2 * y^(mn+1) * (SUM{j=1..m} (z^2)^j * V_j))
      scalar_t e_sq_y_mn1_z_sq_power = rwf * interm.e_final_sq * y_power_mnp1;
      for (size_t j = 0; j < bsc.commitments.size(); ++j)
      {
        e_sq_y_mn1_z_sq_power *= interm.z_sq;
        summand_8 -= e_sq_y_mn1_z_sq_power * bsc.commitments[j];
        DBG_PRINT("V_scalar[" << j << "]: " << c_scalar_Lm1 * e_sq_y_mn1_z_sq_power);
      }

      // - rwf * e^2 * SUM{j=1..log(n)}(e_j^2 * L_j  + e_j^-2 * R_j)
      scalar_t rwf_e_sq = rwf * interm.e_final_sq;
      for (size_t j = 0; j < log2_mn; ++j)
      {
        summand_8 -= rwf_e_sq * (interm.e_sq[j] * interm.L[j] + get_e_inv(j) * get_e_inv(j) * interm.R[j]);
        DBG_PRINT("L_scalar[" << j << "]: " << c_scalar_Lm1 * rwf_e_sq * interm.e_sq[j]);
        DBG_PRINT("R_scalar[" << j << "]: " << c_scalar_Lm1 * rwf_e_sq * get_e_inv(j) * get_e_inv(j));
      }

      // - rwf * e * A - rwf * B   =   0
      summand_8 -= rwf * interm.e_final * interm.A + rwf * interm.B;
      DBG_PRINT("A_scalar: " << c_scalar_Lm1 * rwf * interm.e_final);
      DBG_PRINT("B_scalar: " << c_scalar_Lm1 * rwf);

      summand_8.modify_mul8();
      summand += summand_8;
    }

    point_t GH_exponents = c_point_0;
    CT::calc_pedersen_commitment(G_scalar, H_scalar, GH_exponents);
    bool result = msm_and_check_zero<CT>(g_scalars, h_scalars, summand + GH_exponents);
    if (result)
      DBG_PRINT(ENDL << " . . . . bpp_verify() -- SUCCEEDED!!!" << ENDL);
    return result;
#undef CHECK_AND_FAIL_WITH_ERROR_IF_FALSE
  }

#undef DBG_VAL_PRINT
#undef DBG_PRINT

} // namespace crypto
