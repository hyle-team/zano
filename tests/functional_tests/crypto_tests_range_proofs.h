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


static_assert(constexpr_floor_log_n(0, 2) == 0, "");
static_assert(constexpr_floor_log_n(1, 2) == 0, "");
static_assert(constexpr_floor_log_n(2, 2) == 1, "");
static_assert(constexpr_floor_log_n(3, 2) == 1, "");
static_assert(constexpr_floor_log_n(4, 2) == 2, "");
static_assert(constexpr_floor_log_n(5, 2) == 2, "");
static_assert(constexpr_floor_log_n(64, 2) == 6, "");
static_assert(constexpr_floor_log_n(100, 2) == 6, "");
static_assert(constexpr_floor_log_n(100000000, 2) == 26, "");
static_assert(constexpr_floor_log_n(0x7fffffffffffffff, 2) == 62, "");
static_assert(constexpr_floor_log_n(SIZE_MAX, 2) == 63, "");

static_assert(constexpr_floor_log_n(0, 0) == 0, "");
static_assert(constexpr_floor_log_n(1, 0) == 0, "");
static_assert(constexpr_floor_log_n(2, 0) == 0, "");
static_assert(constexpr_floor_log_n(100, 0) == 0, "");

static_assert(constexpr_floor_log_n(1, 3) == 0, "");
static_assert(constexpr_floor_log_n(2, 3) == 0, "");
static_assert(constexpr_floor_log_n(3, 3) == 1, "");
static_assert(constexpr_floor_log_n(8, 3) == 1, "");
static_assert(constexpr_floor_log_n(9, 3) == 2, "");
static_assert(constexpr_floor_log_n(26, 3) == 2, "");
static_assert(constexpr_floor_log_n(27, 3) == 3, "");
static_assert(constexpr_floor_log_n(100, 3) == 4, "");
static_assert(constexpr_floor_log_n(531441, 3) == 12, "");
static_assert(constexpr_floor_log_n(0x7fffffffffffffff, 3) == 39, "");
static_assert(constexpr_floor_log_n(SIZE_MAX, 3) == 40, "");

static_assert(constexpr_ceil_log_n(0, 0) == 0, "");
static_assert(constexpr_ceil_log_n(0, 1) == 0, "");
static_assert(constexpr_ceil_log_n(0, 2) == 0, "");
static_assert(constexpr_ceil_log_n(1, 0) == 0, "");
static_assert(constexpr_ceil_log_n(2, 0) == 0, "");
static_assert(constexpr_ceil_log_n(1, 1) == 0, "");
static_assert(constexpr_ceil_log_n(1, 2) == 0, "");
static_assert(constexpr_ceil_log_n(2, 1) == 0, "");

static_assert(constexpr_ceil_log_n(0, 5) == 0, "");
static_assert(constexpr_ceil_log_n(1, 5) == 0, "");
static_assert(constexpr_ceil_log_n(4, 5) == 1, "");
static_assert(constexpr_ceil_log_n(5, 5) == 1, "");
static_assert(constexpr_ceil_log_n(6, 5) == 2, "");
static_assert(constexpr_ceil_log_n(25, 5) == 2, "");
static_assert(constexpr_ceil_log_n(100, 5) == 3, "");
static_assert(constexpr_ceil_log_n(624, 5) == 4, "");
static_assert(constexpr_ceil_log_n(625, 5) == 4, "");
static_assert(constexpr_ceil_log_n(626, 5) == 5, "");
static_assert(constexpr_ceil_log_n(100000000, 5) == 12, "");
static_assert(constexpr_ceil_log_n(0x7fffffffffffffff, 5) == 28, "");
static_assert(constexpr_ceil_log_n(SIZE_MAX, 5) == 28, "");

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

static_assert(constexpr_pow(0, 0) == 1, "");
static_assert(constexpr_pow(0, 1) == 1, "");
static_assert(constexpr_pow(1, 1) == 1, "");

static_assert(constexpr_pow(1, 0) == 0, "");
static_assert(constexpr_pow(0, 2) == 1, "");
static_assert(constexpr_pow(1, 2) == 2, "");
static_assert(constexpr_pow(10, 2) == 1024, "");
static_assert(constexpr_pow(63, 2) == 1ull << 63, "");
static_assert(constexpr_pow(3, 3) == 27, "");
static_assert(constexpr_pow(33, 3) == 5559060566555523ull, "");



TEST(bpp, basics)
{
  /*
  srand(0);
  for (size_t i = 0; i < 10; ++i)
    std::cout << scalar_t::random().to_string_as_secret_key() << ENDL;
  */

  auto foo = [&](scalar_t v){
    scalar_vec_t values = { v };
    scalar_vec_t masks  = { scalar_t::random() };
    bpp_signature bpp_sig;
    std::vector<point_t> commitments_1div8;
    uint8_t err = 0;

    bool r = bpp_gen<bpp_crypto_trait_ZC_out>(values, masks, bpp_sig, commitments_1div8, &err);
    if (!r)
    {
      LOG_PRINT_L0("bpp_gen err = " << (uint16_t)err);
      return false;
    }

    std::vector<bpp_sig_commit_ref_t> sigs;
    sigs.emplace_back(bpp_sig, commitments_1div8);

    r = bpp_verify<bpp_crypto_trait_ZC_out>(sigs, &err);
    if (!r)
    {
      LOG_PRINT_L0("bpp_verify err = " << (uint16_t)err);
      return false;
    }

    return true;
  };

  ASSERT_TRUE(foo(scalar_t(0)));
  ASSERT_TRUE(foo(scalar_t(1)));
  ASSERT_TRUE(foo(scalar_t(5)));
  ASSERT_TRUE(foo(scalar_t(UINT64_MAX)));

  ASSERT_FALSE(foo(scalar_t(UINT64_MAX, 1, 0, 0)));
  ASSERT_FALSE(foo(scalar_t(0, 1, 0, 0)));
  ASSERT_FALSE(foo(scalar_t(0, 0, 1, 0)));
  ASSERT_FALSE(foo(scalar_t(0, 0, 0, 1)));
  ASSERT_FALSE(foo(c_scalar_Lm1));
  ASSERT_FALSE(foo(c_scalar_L));
  ASSERT_FALSE(foo(c_scalar_256m1));

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

    r = bpp_gen<bpp_crypto_trait_Zarcanum>(values, masks, bpp_sig, commitments, &err);
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

    r = bpp_gen<bpp_crypto_trait_Zarcanum>(values, masks, bpp_sig, commitments, &err);
    ASSERT_TRUE(r);

    sigs.emplace_back(bpp_sig, commitments);
  }

  r = bpp_verify<bpp_crypto_trait_Zarcanum>(sigs, &err);
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

  std::vector<bpp_sig_commit_ref_t> sig_commit_refs;
  uint8_t err = 0;
  bool r = false;

  {
    signatures_vector.resize(signatures_vector.size() + 1);
    bpp_signature &bpp_sig = signatures_vector.back();
    commitments_vector.resize(commitments_vector.size() + 1);
    std::vector<point_t>& commitments = commitments_vector.back();

    scalar_vec_t values = { 5 };
    scalar_vec_t masks = { scalar_t(77 + 256 * 77) };

    r = bpp_gen<bpp_crypto_trait_ZC_out>(values, masks, bpp_sig, commitments, &err);
    ASSERT_TRUE(r);

    sig_commit_refs.emplace_back(bpp_sig, commitments);
  }

  {
    signatures_vector.resize(signatures_vector.size() + 1);
    bpp_signature &bpp_sig = signatures_vector.back();
    commitments_vector.resize(commitments_vector.size() + 1);
    std::vector<point_t>& commitments = commitments_vector.back();

    scalar_vec_t values = { 5, 700, 8 };
    scalar_vec_t masks = { scalar_t(77 + 256 * 77), scalar_t(255), scalar_t(17) };

    r = bpp_gen<bpp_crypto_trait_ZC_out>(values, masks, bpp_sig, commitments, &err);
    ASSERT_TRUE(r);

    sig_commit_refs.emplace_back(bpp_sig, commitments);
  }

  r = bpp_verify<bpp_crypto_trait_ZC_out>(sig_commit_refs, &err);
  ASSERT_TRUE(r);


  return true;
}



//
// tests for Bulletproofs+ Extended (with double-blinded commitments)
//


TEST(bppe, basics)
{
  /*
  srand(0);
  for (size_t i = 0; i < 10; ++i)
    std::cout << scalar_t::random().to_string_as_secret_key() << ENDL;
  */

  scalar_vec_t values = { 5 };
  scalar_vec_t masks   = { 0 };
  scalar_vec_t masks_2 = { 0 };
  
  bppe_signature bppe_sig;
  std::vector<point_t> commitments;
  uint8_t err = 0;

  bool r = bppe_gen<bpp_crypto_trait_Zarcanum>(values, masks, masks_2, bppe_sig, commitments, &err);

  ASSERT_TRUE(r);

  return true;
}

TEST(bppe, two)
{
  std::vector<bppe_signature> signatures_vector;
  signatures_vector.reserve(10);
  std::vector<std::vector<point_t>> commitments_vector;
  commitments_vector.reserve(10);

  std::vector<bppe_sig_commit_ref_t> sigs;
  uint8_t err = 0;
  bool r = false;

  {
    signatures_vector.resize(signatures_vector.size() + 1);
    bppe_signature &bppe_sig = signatures_vector.back();
    commitments_vector.resize(commitments_vector.size() + 1);
    std::vector<point_t>& commitments = commitments_vector.back();

    scalar_vec_t values = { 5 };
    scalar_vec_t masks  = { scalar_t(77 + 256 * 77) };
    scalar_vec_t masks2 = { scalar_t(88 + 256 * 88) };

    r = bppe_gen<bpp_crypto_trait_Zarcanum>(values, masks, masks2, bppe_sig, commitments, &err);
    ASSERT_TRUE(r);

    sigs.emplace_back(bppe_sig, commitments);
  }

  {
    signatures_vector.resize(signatures_vector.size() + 1);
    bppe_signature &bppe_sig = signatures_vector.back();
    commitments_vector.resize(commitments_vector.size() + 1);
    std::vector<point_t>& commitments = commitments_vector.back();

    scalar_vec_t values = { 5, 700, 8 };
    scalar_vec_t masks  = { scalar_t(77 + 256 * 77), scalar_t(255), scalar_t(17) };
    scalar_vec_t masks2 = { scalar_t(88 + 256 * 88), scalar_t(1), scalar_t(19) };

    r = bppe_gen<bpp_crypto_trait_Zarcanum>(values, masks, masks2, bppe_sig, commitments, &err);
    ASSERT_TRUE(r);

    sigs.emplace_back(bppe_sig, commitments);
  }

  r = bppe_verify<bpp_crypto_trait_Zarcanum>(sigs, &err);
  ASSERT_TRUE(r);


  return true;
}

TEST(bppe, power_128)
{
  std::vector<bppe_signature> signatures_vector;
  signatures_vector.reserve(200);
  std::vector<std::vector<point_t>> commitments_vector;
  commitments_vector.reserve(200);

  std::vector<bppe_sig_commit_ref_t> sigs;
  uint8_t err = 0;
  bool r = false;

  auto gen_rp_for_vec = [&](const scalar_vec_t& values)
  {
    signatures_vector.resize(signatures_vector.size() + 1);
    bppe_signature &bppe_sig = signatures_vector.back();
    commitments_vector.resize(commitments_vector.size() + 1);
    std::vector<point_t>& commitments = commitments_vector.back();

    scalar_vec_t masks, masks2;
    for(auto& el: values)
    {
      masks.emplace_back(scalar_t::random());
      masks2.emplace_back(scalar_t::random());
    }

    r = bppe_gen<bpp_crypto_trait_Zarcanum>(values, masks, masks2, bppe_sig, commitments, &err);
    ASSERT_TRUE(r);
    sigs.emplace_back(bppe_sig, commitments);
    return true;
  };

  auto gen_rp_for_value = [&](const scalar_t& v) { return gen_rp_for_vec(scalar_vec_t{ v }); };

  const scalar_t s_128_max = scalar_t(UINT64_MAX, UINT64_MAX, 0, 0);

  LOG_PRINT_L0("1");
  ASSERT_TRUE(gen_rp_for_value(s_128_max));
  ASSERT_TRUE(bppe_verify<bpp_crypto_trait_Zarcanum>(sigs, &err));
  signatures_vector.clear(), commitments_vector.clear(), sigs.clear();

  LOG_PRINT_L0("2");
  ASSERT_TRUE(gen_rp_for_value(scalar_t(crypto::rand<uint64_t>(), crypto::rand<uint64_t>(), 0, 0)));
  ASSERT_TRUE(bppe_verify<bpp_crypto_trait_Zarcanum>(sigs, &err));
  signatures_vector.clear(), commitments_vector.clear(), sigs.clear();

  LOG_PRINT_L0("3");
  ASSERT_TRUE(gen_rp_for_value(scalar_t(0, 0, 1, 0)));
  ASSERT_FALSE(bppe_verify<bpp_crypto_trait_Zarcanum>(sigs, &err));
  signatures_vector.clear(), commitments_vector.clear(), sigs.clear();

  LOG_PRINT_L0("4");
  ASSERT_TRUE(gen_rp_for_value(scalar_t(0, 0, crypto::rand<uint64_t>(), 0)));
  ASSERT_FALSE(bppe_verify<bpp_crypto_trait_Zarcanum>(sigs, &err));
  signatures_vector.clear(), commitments_vector.clear(), sigs.clear();

  LOG_PRINT_L0("5");
  ASSERT_TRUE(gen_rp_for_value(scalar_t(0, 0, 0, 1)));
  ASSERT_FALSE(bppe_verify<bpp_crypto_trait_Zarcanum>(sigs, &err));
  signatures_vector.clear(), commitments_vector.clear(), sigs.clear();

  LOG_PRINT_L0("6");
  ASSERT_TRUE(gen_rp_for_value(c_scalar_Lm1));
  ASSERT_FALSE(bppe_verify<bpp_crypto_trait_Zarcanum>(sigs, &err));
  signatures_vector.clear(), commitments_vector.clear(), sigs.clear();

  LOG_PRINT_L0("7");
  ASSERT_TRUE(gen_rp_for_vec(scalar_vec_t{s_128_max, s_128_max, s_128_max, s_128_max}));
  LOG_PRINT_L0("simple generated");
  ASSERT_TRUE(bppe_verify<bpp_crypto_trait_Zarcanum>(sigs, &err));
  LOG_PRINT_L0("simple verified");
  for(size_t i = 0; i < 16; ++i)
  {
    LOG_PRINT_L0("    #" << i << " simple generated");
    scalar_vec_t vec;
    for(size_t j = 0, n = crypto::rand<size_t>() % 4 + 1; j < n; ++j)
      vec.emplace_back(scalar_t(crypto::rand<uint64_t>(), crypto::rand<uint64_t>(), 0, 0));
    ASSERT_TRUE(gen_rp_for_vec(vec));
  }
  LOG_PRINT_L0("verification started");
  ASSERT_TRUE(bppe_verify<bpp_crypto_trait_Zarcanum>(sigs, &err));
  signatures_vector.clear(), commitments_vector.clear(), sigs.clear();
  LOG_PRINT_L0("verification finished" << ENDL);

  LOG_PRINT_L0("8");
  ASSERT_TRUE(gen_rp_for_value(s_128_max));
  ASSERT_TRUE(gen_rp_for_value(c_scalar_Lm1));
  ASSERT_TRUE(gen_rp_for_value(s_128_max));
  ASSERT_FALSE(bppe_verify<bpp_crypto_trait_Zarcanum>(sigs, &err));
  signatures_vector.clear(), commitments_vector.clear(), sigs.clear();

  return true;
}
