// Copyright (c) 2020 Zano Project (https://zano.org/)
// Copyright (c) 2020 Locksmith (acmxddk@gmail.com)
// Copyright (c) 2020 sowle (crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

//
// This file contains the implementation of L2S membership proof protocol
// and a linkable ring signature scheme based on it.
//

point_t ml2s_rsum_impl(size_t n, size_t N, std::vector<point_t>::const_iterator X_array_bg_it, const std::vector<scalar_t>& c1_array,
  const std::vector<scalar_t>& c3_array, const scalar_t& cn)
{
  if (n == 1)
    return *X_array_bg_it + cn * *(X_array_bg_it + 1);
  
  // n >= 2, N >= 4
  return ml2s_rsum_impl(n - 1, N / 2, X_array_bg_it,         c1_array, c3_array, c1_array[n - 2]) +
    cn * ml2s_rsum_impl(n - 1, N / 2, X_array_bg_it + N / 2, c1_array, c3_array, c3_array[n - 2]);
}

bool ml2s_rsum(size_t n, const std::vector<point_t>& X_array, const std::vector<scalar_t>& c1_array,
  const std::vector<scalar_t>& c3_array, point_t& result)
{
  size_t N = (size_t)1 << n;
  CHECK_AND_ASSERT_MES(n != 0, false, "n == 0");
  CHECK_AND_ASSERT_MES(N == X_array.size(), false, "|X_array| != N, " << X_array.size() << ", " << N);
  CHECK_AND_ASSERT_MES(c1_array.size() == n, false, "|c1_array| != n, " << c1_array.size() << ", " << n);
  CHECK_AND_ASSERT_MES(c3_array.size() == n - 1, false, "|c3_array| != n - 1, " << c3_array.size() << ", " << n - 1);

  result = ml2s_rsum_impl(n, N, X_array.begin(), c1_array, c3_array, c1_array[n - 1]);
  return true;
}

struct ml2s_signature_element
{
  point_t   Z0;
  point_t   T0;
  scalar_t  t0;
  point_t   Z;
  std::vector<scalar_t> r_array;
  std::vector<point_t>  H_array;
  point_t   T;
  scalar_t  t;
};

struct ml2s_signature
{
  scalar_t z;
  std::vector<ml2s_signature_element> elements;
};

/* WIP
// reference: mL2SLnkSig_Verif()
bool ml2s_lnk_sig_verif(const scalar_t& m, const std::vector<point_t>& B_array, const ml2s_signature& signature, uint8_t* p_err = nullptr)
{
#define CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(cond, err_code) \
  if (!(cond)) { LOG_PRINT_RED("ml2s_lnk_sig_verif: \"" << #cond << "\" is false at " << LOCATION_SS, LOG_LEVEL_3); if (p_err) *p_err = err_code; return false; }

  auto hash_point_lambda = [&signature](const point_t& point) { return point + signature.z * hash_helper_t::hp(point); };

  size_t L = signature.elements.size();
  CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(L > 0, 0);
  size_t n = signature.elements[0].r_array.size();
  CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(n < 32, 4);
  size_t N = (size_t)1 << n;
  CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(B_array.size() == N / 2, 5);

  std::vector<point_t> I_array(L);
  std::vector<point_t> A_array(L);
  
  for (size_t i = 0; i < L; ++i)
  {
    I_array[i] = (signature.elements[i].Z0 - c_point_G) / signature.z;
    A_array[i] = signature.elements[i].Z0;
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(signature.elements[i].r_array.size() == n, 1);
    CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(signature.elements[i].H_array.size() == n, 2);
  }

  scalar_t z_ = hash_helper_t::hs(m, B_array, I_array);
  CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(z_ == signature.z, 3);

  scalar_t e = hash_helper_t::hs(signature.z);

  // ref: mL2SHPoM_Verif()

  // ref: X = mL2SHPoM_GetDecoySet(N, A, hash_point_cb, preimage_set_gen_cb)
  std::vector<point_t> P_array(B_array.size());
  for (size_t i = 0; i < B_array.size(); ++i)
    P_array[i] = hash_point_lambda(B_array[i]);

  point_t Q_shift = hash_helper_t::hs(A_array, P_array) * c_point_G;

  CHECK_AND_FAIL_WITH_ERROR_IF_FALSE(P_array.size() * 2 == N, 6);
  std::vector<point_t> X_array(N);
  // X_array = { P_array[0], Q_array[0], P_array[1], Q_array[1], etc.
  for (size_t i = 0; i < N; ++i)
  {
    if (i % 2 == 0)
      X_array[i] = P_array[i / 2];
    else
      X_array[i] = hash_point_lambda(Q_shift + B_array[i / 2]);
  }


  return false;
#undef CHECK_AND_FAIL_WITH_ERROR_IF_FALSE
}

*/
