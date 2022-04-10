// Copyright (c) 2021 Zano Project (https://zano.org/)
// Copyright (c) 2021 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

// calc weighted inner pruduct of av and bv w.r.t. Vandermonde vector (y, y^2, y^3, ..., y^n)
// <a, ((y, y^2, y^3, ...) o b)> = <c, (y, y^2, y^3, ...)>   (<> -- standard inner product, o - componen-wise)
// s.a. BP+ paper, pages 3-4
bool wip_vandermonde(const scalar_vec_t& av, const scalar_vec_t& bv, const scalar_t& y, scalar_t& result)
{
  result = 0;
  size_t n = av.size();
  if (n != bv.size())
    return false;

  scalar_t y_powered = 1;
  for (size_t i = 0; i < n; ++i)
  {
    y_powered *= y;
    result.assign_muladd(av[i] * bv[i], y_powered, result); // result.a += av[i] * bv[i] * y_powered;
  }

  return true;
}


static_assert(constexpr_floor_log2(0) == 0, "");
static_assert(constexpr_floor_log2(1) == 0, "");
static_assert(constexpr_floor_log2(2) == 1, "");
static_assert(constexpr_floor_log2(3) == 1, "");
static_assert(constexpr_floor_log2(4) == 2, "");
static_assert(constexpr_floor_log2(5) == 2, "");
static_assert(constexpr_floor_log2(64) == 6, "");
static_assert(constexpr_floor_log2(100) == 6, "");
static_assert(constexpr_floor_log2(100000000) == 26, "");
static_assert(constexpr_floor_log2(0x7fffffffffffffff) == 62, "");
static_assert(constexpr_floor_log2(SIZE_MAX) == 63, "");

static_assert(constexpr_ceil_log2(0) == 0, "");
static_assert(constexpr_ceil_log2(1) == 0, "");
static_assert(constexpr_ceil_log2(2) == 1, "");
static_assert(constexpr_ceil_log2(3) == 2, "");
static_assert(constexpr_ceil_log2(4) == 2, "");
static_assert(constexpr_ceil_log2(5) == 3, "");
static_assert(constexpr_ceil_log2(64) == 6, "");
static_assert(constexpr_ceil_log2(100) == 7, "");
static_assert(constexpr_ceil_log2(100000000) == 27, "");
static_assert(constexpr_ceil_log2(0x7fffffffffffffff) == 63, "");
static_assert(constexpr_ceil_log2(SIZE_MAX) == 64, "");


TEST(bpp, basics)
{
  /*
  srand(0);
  for (size_t i = 0; i < 10; ++i)
    std::cout << scalar_t::random().to_string_as_secret_key() << ENDL;
  */

  point_t H = hash_helper_t::hp(c_point_G);
  ASSERT_EQ(H, c_point_H);
  std::string h2_hash_str("h2_generator");
  point_t H2 = hash_helper_t::hp(h2_hash_str.c_str(), h2_hash_str.size());
  ASSERT_EQ(H2, c_point_H2);
  LOG_PRINT_L0("c_point_0 = " << c_point_0 << " = { " << c_point_0.to_hex_comma_separated_uint64_str() << " }");
  LOG_PRINT_L0("Zano G =  " << c_point_G << " = { " << c_point_G.to_hex_comma_separated_bytes_str() << " }");
  LOG_PRINT_L0("Zano H =  " << H << " = { " << H.to_hex_comma_separated_uint64_str() << " }");
  LOG_PRINT_L0("Zano H2 = " << H2 << " = { " << H2.to_hex_comma_separated_uint64_str() << " }");

  scalar_vec_t values = { 5 };
  scalar_vec_t masks  = { 0 };
  bpp_signature bpp_sig;
  std::vector<point_t> commitments;
  uint8_t err = 0;

  bool r = bpp_gen<bpp_crypto_trait_zano<>>(values, masks, bpp_sig, commitments, &err);

  ASSERT_TRUE(r);

  return true;
}


TEST(bpp, two)
{
  std::vector<bpp_signature> signatures_vector;
  signatures_vector.reserve(10);
  std::vector<std::vector<point_t>> commitments_vector;
  commitments_vector.reserve(10);

  std::vector<bpp_sig_commit_ref_t> sigs;
  uint8_t err = 0;
  bool r = false;

  {
    signatures_vector.resize(signatures_vector.size() + 1);
    bpp_signature &bpp_sig = signatures_vector.back();
    commitments_vector.resize(commitments_vector.size() + 1);
    std::vector<point_t>& commitments = commitments_vector.back();

    scalar_vec_t values = { 5 };
    scalar_vec_t masks = { scalar_t(77 + 256 * 77) };

    r = bpp_gen<bpp_crypto_trait_zano<>>(values, masks, bpp_sig, commitments, &err);
    ASSERT_TRUE(r);

    sigs.emplace_back(bpp_sig, commitments);
  }

  {
    signatures_vector.resize(signatures_vector.size() + 1);
    bpp_signature &bpp_sig = signatures_vector.back();
    commitments_vector.resize(commitments_vector.size() + 1);
    std::vector<point_t>& commitments = commitments_vector.back();

    scalar_vec_t values = { 5, 700, 8 };
    scalar_vec_t masks = { scalar_t(77 + 256 * 77), scalar_t(255), scalar_t(17) };

    r = bpp_gen<bpp_crypto_trait_zano<>>(values, masks, bpp_sig, commitments, &err);
    ASSERT_TRUE(r);

    sigs.emplace_back(bpp_sig, commitments);
  }

  r = bpp_verify<bpp_crypto_trait_zano<>>(sigs, &err);
  ASSERT_TRUE(r);


  return true;
}


TEST(bpp, power_256)
{
  // make sure the BPP implementation supports values up to 2^256  (Zarcanum needs 2^170 since b_a < z * 2^64, where z = 2^106, s.a. Zarcanum preprint, page 21)
  std::vector<bpp_signature> signatures_vector;
  signatures_vector.reserve(10);
  std::vector<std::vector<point_t>> commitments_vector;
  commitments_vector.reserve(10);

  std::vector<bpp_sig_commit_ref_t> sig_ñommit_refs;
  uint8_t err = 0;
  bool r = false;

  {
    signatures_vector.resize(signatures_vector.size() + 1);
    bpp_signature &bpp_sig = signatures_vector.back();
    commitments_vector.resize(commitments_vector.size() + 1);
    std::vector<point_t>& commitments = commitments_vector.back();

    scalar_vec_t values = { 5 };
    scalar_vec_t masks = { scalar_t(77 + 256 * 77) };

    r = bpp_gen<bpp_crypto_trait_zano<>>(values, masks, bpp_sig, commitments, &err);
    ASSERT_TRUE(r);

    sig_ñommit_refs.emplace_back(bpp_sig, commitments);
  }

  {
    signatures_vector.resize(signatures_vector.size() + 1);
    bpp_signature &bpp_sig = signatures_vector.back();
    commitments_vector.resize(commitments_vector.size() + 1);
    std::vector<point_t>& commitments = commitments_vector.back();

    scalar_vec_t values = { 5, 700, 8 };
    scalar_vec_t masks = { scalar_t(77 + 256 * 77), scalar_t(255), scalar_t(17) };

    r = bpp_gen<bpp_crypto_trait_zano<>>(values, masks, bpp_sig, commitments, &err);
    ASSERT_TRUE(r);

    sig_ñommit_refs.emplace_back(bpp_sig, commitments);
  }

  r = bpp_verify<bpp_crypto_trait_zano<>>(sig_ñommit_refs, &err);
  ASSERT_TRUE(r);


  return true;
}

