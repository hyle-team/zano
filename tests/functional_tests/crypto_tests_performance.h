// Copyright (c) 2021 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include <numeric>

uint64_t get_bits_v1(const scalar_t& s, uint8_t bit_index_first, uint8_t bits_count)
{
  if (bits_count == 0 || bits_count > 64)
    return 0;
  unsigned int bit_index_last = bit_index_first + bits_count - 1;
  if (bit_index_last > 255)
    bit_index_last = 255;
  uint64_t result_mask = ((1ull << (bits_count - 1)) - 1) << 1 | 1;  // (just because 1ull << 64 in undefined behaviour, not a 0 as one would expect)

  uint64_t result = s.m_u64[bit_index_first >> 6] >> (bit_index_first & 63);
  if (bits_count > (bit_index_last & 63) + 1)
    result |= s.m_u64[bit_index_last >> 6] << (bits_count - (bit_index_last & 63) - 1);
  return result & result_mask;
}


TEST(crypto, ge_precomp)
{
  //precomp_data_t G_precomp = {};
  //construct_precomp_data(G_precomp, c_point_G);
  //std::cout << "size of G_precomp: " << sizeof G_precomp << " bytes" << ENDL;
  //for(size_t i = 0; i < 32; ++i)
  //  for(size_t j = 0; j < 8; ++j)
  //    std::cout << "i: " << i << ", j: " << j << ", precomp: " << ENDL << G_precomp[i][j] << ENDL;

  precomp_data_t H_precomp = {};
  construct_precomp_data(H_precomp, c_point_H);

  auto check_for_x = [&](const scalar_t& x) -> bool {
    point_t P;
    ge_scalarmult_precomp_vartime(&P.m_p3, H_precomp, x.m_s);
    return P == x * c_point_H;
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




TEST(perf, primitives)
{
  struct helper
  {
    static void make_rnd_indicies(std::vector<size_t>& v, size_t size)
    {
      v.resize(size);
      for (size_t i = 0; i < size; ++i)
        v[i] = i;
      std::shuffle(v.begin(), v.end(), crypto::uniform_random_bit_generator());
    };
  };

  struct timer_t
  {
    std::chrono::high_resolution_clock::time_point m_tp{};
    uint64_t m_t{};
    uint64_t m_div_coeff{ 1 };
    void start(uint64_t div_coeff = 1) { m_tp = std::chrono::high_resolution_clock::now(); m_div_coeff = div_coeff; }
    void stop() { m_t = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_tp).count(); }
    uint64_t get_time_mcs() { return m_div_coeff == 1 ? m_t : m_t / m_div_coeff; }
  };

  typedef uint64_t(*run_func_t)(timer_t& t, size_t rounds);



  auto run = [](const std::string& title, size_t rounds, run_func_t cb)
  {
    uint64_t result;
    timer_t t_warmup, t, t_total;
    t_total.start();
    result = cb(t_warmup, rounds);
    result += cb(t, rounds);
    t_total.stop();
    double run_time_mcs_x_100 = double(uint64_t(t.get_time_mcs() / (rounds / 100)));
    LOG_PRINT_L0(std::left << std::setw(40) << title << std::setw(7) << rounds << " rnds ->  "
      << std::right << std::setw(7) << std::fixed << std::setprecision(2) << run_time_mcs_x_100 / 100.0 << "  mcs avg. (gross: "
      << std::fixed << std::setprecision(2) << double(t_total.get_time_mcs()) / 1000.0 << " ms), result hash: " << result);
  };

#define HASH_64_VEC(vec_var_name) hash_64(vec_var_name.data(), vec_var_name.size() * sizeof(vec_var_name[0]))

  LOG_PRINT_L0(ENDL << "hash functions:");

  struct run_cn_fash_hash
  {
    static uint64_t run(timer_t& t, size_t rounds, size_t data_size)
    {
      std::vector<size_t> rnd_indecies;
      helper::make_rnd_indicies(rnd_indecies, rounds);

      struct bytes64
      {
        unsigned char b[64];
      };

      std::vector<bytes64> scalars_64(rounds);
      for (size_t i = 0; i < scalars_64.size(); ++i)
        crypto::generate_random_bytes(sizeof(bytes64), scalars_64[i].b);

      std::vector<hash> results(rounds);
      t.start();
      for (size_t i = 0; i < rounds; ++i)
      {
        results[i] = cn_fast_hash(scalars_64[rnd_indecies[i]].b, 64);
      }
      t.stop();

      return HASH_64_VEC(results);
    };
  };

  run("cn_fast_hash(64 bytes)", 1000, [](timer_t& t, size_t rounds) {
    return run_cn_fash_hash::run(t, rounds, 64ull);
  });

  run("cn_fast_hash(2048 bytes)", 1000, [](timer_t& t, size_t rounds) {
    return run_cn_fash_hash::run(t, rounds, 2048ull);
  });

  LOG_PRINT_L0(ENDL << "native crypto primitives:");

  run("sc_reduce", 30000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    struct bytes64
    {
      unsigned char b[64];
    };

    std::vector<bytes64> scalars_64(rounds);
    for (size_t i = 0; i < scalars_64.size(); ++i)
      crypto::generate_random_bytes(sizeof(bytes64), scalars_64[i].b);

    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      sc_reduce(scalars_64[rnd_indecies[i]].b);
    }
    t.stop();

    return HASH_64_VEC(scalars_64);
  });

  run("sc_reduce32", 30000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<crypto::ec_scalar> scalars(rounds);
    for (size_t i = 0; i < scalars.size(); ++i)
      crypto::generate_random_bytes(sizeof(crypto::ec_scalar), scalars[i].data);

    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      sc_reduce32((unsigned char*)&scalars[rnd_indecies[i]].data);
    }
    t.stop();

    return HASH_64_VEC(scalars);
  });

  run("sc_mul", 50000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<scalar_t> a(rounds), b(rounds);
    for (size_t i = 0; i < rounds; ++i)
    {
      a[i].make_random();
      b[i].make_random();
    }

    std::vector<scalar_t> result(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
      sc_mul(result[rnd_indecies[i]].m_s, a[rnd_indecies[i]].m_s, b[rnd_indecies[i]].m_s);
    t.stop();

    return HASH_64_VEC(result);
  });

  run("sc_add", 50000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<scalar_t> a(rounds), b(rounds);
    for (size_t i = 0; i < rounds; ++i)
    {
      a[i].make_random();
      b[i].make_random();
    }

    std::vector<scalar_t> result(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
      sc_add(result[i].m_s, a[rnd_indecies[i]].m_s, b[rnd_indecies[i]].m_s);
    t.stop();

    return HASH_64_VEC(result);
  });

  run("sc_mul + sc_add", 50000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<scalar_t> a(rounds), b(rounds), c(rounds);
    for (size_t i = 0; i < rounds; ++i)
    {
      a[i].make_random();
      b[i].make_random();
      c[i].make_random();
    }

    std::vector<scalar_t> result(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      scalar_t tmp;
      sc_mul(tmp.m_s, a[rnd_indecies[i]].m_s, b[rnd_indecies[i]].m_s); // tmp = a * b
      sc_add(result[i].m_s, tmp.m_s, c[rnd_indecies[i]].m_s); // result = tmp + c
    }
    t.stop();

    return HASH_64_VEC(result);
  });

  run("sc_muladd", 50000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<scalar_t> a(rounds), b(rounds), c(rounds);
    for (size_t i = 0; i < rounds; ++i)
    {
      a[i].make_random();
      b[i].make_random();
      c[i].make_random();
    }

    std::vector<scalar_t> result(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
      sc_muladd(result[i].m_s, a[rnd_indecies[i]].m_s, b[rnd_indecies[i]].m_s, c[rnd_indecies[i]].m_s);
    t.stop();

    return HASH_64_VEC(result);
  });

  run("ge_p3_tobytes", 10000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<ge_p3> points_p3(rounds);
    ge_scalarmult_base(&points_p3[0], c_scalar_1.data());
    for (size_t i = 1; i < points_p3.size(); ++i)
      ge_bytes_hash_to_ec_32(&points_p3[i], (const unsigned char*)&points_p3[i - 1].X); // P_{i+1} = Hp(P_i.X)

    std::vector<crypto::ec_point> points(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_p3_tobytes((unsigned char*)points[i].data, &points_p3[rnd_indecies[i]]);
    }
    t.stop();

    return HASH_64_VEC(points);
  });

  run("ge_frombytes_vartime(p3)", 10000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    point_t P = c_point_G;

    std::vector<crypto::ec_point> points_p3_bytes(rounds);
    for (size_t i = 0; i < points_p3_bytes.size(); ++i)
    {
      P = hash_helper_t::hp(P);
      ge_p3_tobytes((unsigned char*)&points_p3_bytes[i], &P.m_p3);
    }

    std::vector<ge_p3> points(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_frombytes_vartime(&points[i], (unsigned char*)&points_p3_bytes[rnd_indecies[i]]);
    }
    t.stop();

    return HASH_64_VEC(points);
  });

  run("ge_p3_to_cached(p3)", 10000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<ge_p3> points_p3(rounds);
    ge_scalarmult_base(&points_p3[0], c_scalar_1.data());
    for (size_t i = 1; i < points_p3.size(); ++i)
      ge_bytes_hash_to_ec_32(&points_p3[i], (const unsigned char*)&points_p3[i - 1].X); // P_{i+1} = Hp(P_i.X)

    std::vector<ge_cached> points_cached(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_p3_to_cached(&points_cached[i], &points_p3[rnd_indecies[i]]);
    }
    t.stop();

    return HASH_64_VEC(points_cached);
  });

  run("ge_add(p1p1 = p3 + cached)", 50000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);
    std::vector<ge_cached> points_cached(rounds);
    point_t p = scalar_t::random() * c_point_G;
    for (size_t i = 0; i < rnd_indecies.size(); ++i)
    {
      ge_p3_to_cached(&points_cached[i], &p.m_p3);
      p = p + p;
    }
    ge_p3 Q;
    ge_scalarmult_base(&Q, &scalar_t::random().m_s[0]);
    std::vector<ge_p1p1> results(rnd_indecies.size());

    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_add(&results[i], &Q, &points_cached[rnd_indecies[i]]);
    }
    t.stop();

    return HASH_64_VEC(results);
  });

  run("ge_p1p1_to_p3(p1p1)", 50000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    ge_cached G;
    ge_p3_to_cached(&G, &c_point_G.m_p3);

    std::vector<ge_p1p1> points_p1p1(rounds);
    ge_add(&points_p1p1[0], &c_point_G.m_p3, &G);
    for (size_t i = 1; i < points_p1p1.size(); ++i)
    {
      ge_p3 p3;
      ge_p1p1_to_p3(&p3, &points_p1p1[i - 1]);
      ge_add(&points_p1p1[i], &p3, &G);
    }

    std::vector<ge_p3> points_p3(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_p1p1_to_p3(&points_p3[i], &points_p1p1[rnd_indecies[i]]);
    }
    t.stop();

    return HASH_64_VEC(points_p3);
  });

  run("ge_scalarmult()", 5000, [](timer_t& t, size_t rounds) {
    //rounds -= rounds % 8;
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    scalar_t x;
    x.make_random();

    std::vector<crypto::ec_scalar> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
    {
      //scalar_t x = x + x + x;
      scalar_t x;
      x.make_random();
      memcpy(&scalars[i].data, x.data(), 32);
    }

    point_t p = scalar_t::random() * c_point_G;

    //std::vector<ge_p2> points_p2(rounds);
    std::vector<ge_p3> points_p3(rounds);

    // warmup round
    //for (size_t i = 0; i < rounds; ++i)
    //  ge_scalarmult((ge_p2*)&points_p3[i], (const unsigned char*)&scalars[rnd_indecies[i]], &p.m_p3);

    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_scalarmult((ge_p2*)&points_p3[i], (const unsigned char*)&scalars[rnd_indecies[i]], &p.m_p3);
      //ge_scalarmult(&points_p2[i * 4 + 3], (const unsigned char*)&scalars[rnd_indecies[i * 4 + 3]], &p.m_p3);
      //ge_scalarmult(&points_p2[i * 4 + 0], (const unsigned char*)&scalars[rnd_indecies[i * 4 + 0]], &p.m_p3);
      //ge_scalarmult(&points_p2[i * 4 + 1], (const unsigned char*)&scalars[rnd_indecies[i * 4 + 1]], &p.m_p3);
      //ge_scalarmult(&points_p2[i * 4 + 2], (const unsigned char*)&scalars[rnd_indecies[i * 4 + 2]], &p.m_p3);
    }
    t.stop();

    return HASH_64_VEC(points_p3);
  });

  run("ge_scalarmult() (2)", 5000, [](timer_t& t, size_t rounds) {
    //rounds -= rounds % 8;
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    scalar_t x;
    x.make_random();

    std::vector<crypto::ec_scalar> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
    {
      //scalar_t x = x + x + x;
      scalar_t x;
      x.make_random();
      memcpy(&scalars[i].data, x.data(), 32);
    }

    point_t p = scalar_t::random() * c_point_G;

    //std::vector<ge_p2> points_p2(rounds);
    std::vector<ge_p3> points_p3(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_scalarmult((ge_p2*)&points_p3[i], (const unsigned char*)&scalars[rnd_indecies[i]], &p.m_p3);
      //ge_scalarmult(&points_p2[i * 4 + 3], (const unsigned char*)&scalars[rnd_indecies[i * 4 + 3]], &p.m_p3);
      //ge_scalarmult(&points_p2[i * 4 + 0], (const unsigned char*)&scalars[rnd_indecies[i * 4 + 0]], &p.m_p3);
      //ge_scalarmult(&points_p2[i * 4 + 1], (const unsigned char*)&scalars[rnd_indecies[i * 4 + 1]], &p.m_p3);
      //ge_scalarmult(&points_p2[i * 4 + 2], (const unsigned char*)&scalars[rnd_indecies[i * 4 + 2]], &p.m_p3);
    }
    t.stop();

    return HASH_64_VEC(points_p3);
  });

  run("ge_scalarmult_p3()", 5000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    scalar_t x;
    x.make_random();

    std::vector<crypto::ec_scalar> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
    {
      //scalar_t x = x + x + x;
      scalar_t x;
      x.make_random();
      memcpy(&scalars[i].data, x.data(), 32);
    }

    point_t p = scalar_t::random() * c_point_G;

    std::vector<ge_p3> points_p3(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_scalarmult_p3(&points_p3[i], (const unsigned char*)&scalars[rnd_indecies[i]], &p.m_p3);
    }
    t.stop();

    return HASH_64_VEC(points_p3);
  });

  run("ge_scalarmult_vartime_p3()", 5000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    scalar_t x;
    x.make_random();

    std::vector<crypto::ec_scalar> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
    {
      //scalar_t x = x + x + x;
      scalar_t x;
      x.make_random();
      memcpy(&scalars[i].data, x.data(), 32);
    }

    point_t p = scalar_t::random() * c_point_G;

    //memcpy(&scalars[rnd_indecies[0]], scalar_t(1).data(), 32);

    std::vector<ge_p3> points_p3(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_scalarmult_vartime_p3(&points_p3[i], (const unsigned char*)&scalars[rnd_indecies[i]], &p.m_p3);
    }
    t.stop();

    return HASH_64_VEC(points_p3);
  });

  run("ge_scalarmult_vartime_p3_v2()", 5000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    scalar_t x;
    x.make_random();

    std::vector<crypto::ec_scalar> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
    {
      //scalar_t x = x + x + x;
      scalar_t x;
      x.make_random();
      memcpy(&scalars[i].data, x.data(), 32);
    }

    point_t p = scalar_t::random() * c_point_G;

    std::vector<ge_p3> points_p3(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_scalarmult_vartime_p3_v2(&points_p3[i], (const unsigned char*)&scalars[rnd_indecies[i]], &p.m_p3);
    }
    t.stop();

    return HASH_64_VEC(points_p3);
  });

  run("ge_scalarmult_base()", 10000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    scalar_t x;
    x.make_random();

    std::vector<crypto::ec_scalar> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
    {
      scalar_t x = x + x + x;
      memcpy(&scalars[i].data, x.data(), 32);
    }

    std::vector<ge_p3> points_p3(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_scalarmult_base(&points_p3[i], (const unsigned char*)&scalars[rnd_indecies[i]]);
    }
    t.stop();

    return HASH_64_VEC(points_p3);
  });

  run("construct_precomp_data()", 300, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    unsigned char s[32] = {};
    std::vector<point_t> random_points(rounds);
    for (size_t i = 0; i < rounds; ++i)
    {
      s[0] = i;
      ge_p2 p2;
      ge_fromfe_frombytes_vartime(&p2, s);
      ge_p2_to_p3(&random_points[i].m_p3, &p2);
    }

    std::vector<ge_p3> points_p3(rounds);
    precomp_data_t precomp_data;
    uint64_t result = 0;
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      construct_precomp_data(precomp_data, random_points[rnd_indecies[i]]);
      result ^= (precomp_data[1][1].xy2d[1] + precomp_data[31][7].xy2d[9]);
    }
    t.stop();

    return result;
  });

  run("ge_scalarmult_precomp_vartime()", 10000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    scalar_t x;
    x.make_random();

    std::vector<crypto::ec_scalar> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
    {
      scalar_t x = x + x + x;
      memcpy(&scalars[i].data, x.data(), 32);
    }

    precomp_data_t precomp_data;
    construct_precomp_data(precomp_data, x * c_point_X);

    std::vector<ge_p3> points_p3(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_scalarmult_precomp_vartime(&points_p3[i], precomp_data, (const unsigned char*)&scalars[rnd_indecies[i]]);
    }
    t.stop();

    return HASH_64_VEC(points_p3);
  });

  run("ge_scalarmult_base_vartime()", 10000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    scalar_t x;
    x.make_random();

    std::vector<crypto::ec_scalar> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
    {
      scalar_t x = x + x + x;
      memcpy(&scalars[i].data, x.data(), 32);
    }

    std::vector<ge_p3> points_p3(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_scalarmult_base_vartime(&points_p3[i], (const unsigned char*)&scalars[rnd_indecies[i]]);
    }
    t.stop();

    return HASH_64_VEC(points_p3);
  });

  run("ge_mul8_p3()", 5000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<ge_p3> points_p3(rounds);
    ge_scalarmult_base(&points_p3[0], c_scalar_1.data());
    for (size_t i = 1; i < points_p3.size(); ++i)
      ge_bytes_hash_to_ec_32(&points_p3[i], (const unsigned char*)&points_p3[i - 1].X); // P_{i+1} = Hp(P_i.X)

    std::vector<ge_p3> points_result(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_mul8_p3(&points_result[i], &points_p3[rnd_indecies[i]]);
    }
    t.stop();

    return HASH_64_VEC(points_result);
  });

  run("ge_mul8()", 5000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    point_t p = scalar_t::random() * c_point_G;

    std::vector<ge_p2> points_p2(rounds);
    ge_p3_to_p2(&points_p2[0], &p.m_p3);
    ge_p1p1 p1;
    for (size_t i = 0; i < points_p2.size() - 1; ++i)
    {
      ge_p2_dbl(&p1, &points_p2[i]);
      ge_p1p1_to_p2(&points_p2[i + 1], &p1);
    }

    std::vector<ge_p1p1> points_result(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      ge_mul8(&points_result[i], &points_p2[rnd_indecies[i]]);
    }
    t.stop();

    return HASH_64_VEC(points_result);
  });




  LOG_PRINT_L0(ENDL << "new primitives:");

  run("point_t + point_t", 50000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<point_t> points(rounds);
    point_t p = c_point_G;
    for (size_t i = 0; i < rounds; ++i)
    {
      points[i] = p;
      p = p + p;
    }

    std::vector<point_t> result(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      result[i] = points[rnd_indecies[i]] + p;
    }
    t.stop();

    return HASH_64_VEC(result);
  });

  run("sclar_t * point_t", 5000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<scalar_t> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
      scalars[i].make_random();

    point_t p = scalar_t::random() * c_point_G;

    std::vector<point_t> result(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      result[i] = scalars[rnd_indecies[i]] * p;
    }
    t.stop();

    return HASH_64_VEC(result);
  });

  run("sclar_t * point_g_t", 5000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<scalar_t> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
      scalars[i].make_random();

    std::vector<point_t> result(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      result[i] = scalars[rnd_indecies[i]] * c_point_G;
    }
    t.stop();

    return HASH_64_VEC(result);
  });

  run("sclar_t * scalar_t", 50000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<scalar_t> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
      scalars[i].make_random();

    scalar_t s = scalar_t::random();

    std::vector<scalar_t> result(rounds);
    t.start(4);
    for (size_t i = 0; i < rounds; ++i)
    {
      result[i] = scalars[rnd_indecies[i]] * s * s * s * s;
    }
    t.stop();

    return HASH_64_VEC(result);
  });

  run("sclar_t / scalar_t", 10000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<scalar_t> scalars(rounds);
    for (size_t i = 0; i < rounds; ++i)
      scalars[i].make_random();

    scalar_t s = scalar_t::random();

    std::vector<scalar_t> result(rounds);
    t.start(2);
    for (size_t i = 0; i < rounds; ++i)
    {
      result[i] = scalars[rnd_indecies[i]] / s / s;
    }
    t.stop();

    return HASH_64_VEC(result);
  });

  run("mul_plus_G", 2000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    std::vector<point_t> points(rounds);
    point_t p = c_point_G;
    for (size_t i = 0; i < rounds; ++i)
    {
      points[i] = p;
      p = p + p;
    }

    scalar_t a, b;
    a.make_random();
    b.make_random();

    std::vector<point_t> result(rounds);
    t.start(2);
    for (size_t i = 0; i < rounds; ++i)
    {
      result[i] = points[rnd_indecies[i]].mul_plus_G(a, b).mul_plus_G(a, b);
    }
    t.stop();

    return HASH_64_VEC(result);
  });

  run("get_bits x 10", 20000, [](timer_t& t, size_t rounds) {
    std::vector<size_t> rnd_indecies;
    helper::make_rnd_indicies(rnd_indecies, rounds);

    scalar_vec_t data;
    data.resize_and_make_random(rounds);

    std::vector<uint64_t> result(rounds);
    t.start();
    for (size_t i = 0; i < rounds; ++i)
    {
      auto& x = data[rnd_indecies[i]];
      result[i] =
        x.get_bits(x.m_s[11], x.m_s[21] % 65) ^ 
        x.get_bits(x.m_s[12], x.m_s[22] % 65) ^ 
        x.get_bits(x.m_s[13], x.m_s[23] % 65) ^ 
        x.get_bits(x.m_s[14], x.m_s[24] % 65) ^ 
        x.get_bits(x.m_s[15], x.m_s[25] % 65) ^ 
        x.get_bits(x.m_s[16], x.m_s[26] % 65) ^ 
        x.get_bits(x.m_s[17], x.m_s[27] % 65) ^ 
        x.get_bits(x.m_s[18], x.m_s[28] % 65) ^
        x.get_bits(x.m_s[19], x.m_s[29] % 65) ^
        x.get_bits(x.m_s[20], x.m_s[30] % 65);
    }
    t.stop();

    return HASH_64_VEC(result);
  });

  return true;
} // TEST  



////////////////////////////////////////////////////////////////////////////////
///////////////// v3



///////////////// v4


template<typename CT>
bool msm_and_check_zero_pippenger_v4(const scalar_vec_t& g_scalars, const scalar_vec_t& h_scalars, const point_t& summand, size_t c)
{
  // TODO: with c = 8 and with direct access got much worse result than with c = 7 and get_bits(), consider checking again for bigger datasets (N>256) 
  CHECK_AND_ASSERT_MES(g_scalars.size() <= CT::c_bpp_mn_max, false, "g_scalars oversized");
  CHECK_AND_ASSERT_MES(h_scalars.size() <= CT::c_bpp_mn_max, false, "h_scalars oversized");
  CHECK_AND_ASSERT_MES(c < 10, false, "c is too big");

  size_t C = 1ull << c;

  // k_max * c + (c-1) >= max_bit_idx
  // 
  //                 max_bit_idx - (c - 1)            max_bit_idx - (c - 1) + (c - 1)              max_bit_idx
  // k_max = ceil ( --------------------- ) = floor ( ------------------------------ )  =  floor ( ----------- )
  //                           c                                    c                                   c      
  const size_t b = 253; // the maximum number of bits in x  https://eprint.iacr.org/2022/999.pdf  TODO: we may also scan for maximum bit used in all the scalars if all the scalars are small
  const size_t max_bit_idx = b - 1;
  const size_t k_max = max_bit_idx / c;
  const size_t K = k_max + 1;

  std::unique_ptr<point_t[]> buckets( new point_t[C * K] );
  std::vector<bool> buckets_inited(C * K);

  // first loop, calculate partial bucket sums
  for (size_t n = 0; n < g_scalars.size(); ++n)
  {
    for (size_t k = 0; k < K; ++k)
    {
      uint64_t l = g_scalars[n].get_bits(k * c, c); // l in [0; 2^c-1]
      if (l != 0)
      {
        size_t bucket_id = l * K + k;
        if (buckets_inited[bucket_id])
          buckets[bucket_id] += CT::get_generator(false, n);
        else
        {
          buckets[bucket_id] =  CT::get_generator(false, n);
          buckets_inited[bucket_id] = true;
        }
      }
    }
  }

  for (size_t n = 0; n < h_scalars.size(); ++n)
  {
    for (size_t k = 0; k < K; ++k)
    {
      uint64_t l = h_scalars[n].get_bits(k * c, c); // l in [0; 2^c-1]
      if (l != 0)
      {
        size_t bucket_id = l * K + k;
        if (buckets_inited[bucket_id])
          buckets[bucket_id] += CT::get_generator(true, n);
        else
        {
          buckets[bucket_id] =  CT::get_generator(true, n);
          buckets_inited[bucket_id] = true;
        }
      }
    }
  }

  // the second loop
  // S[l, k] = S[l-1, k] + B[l, k]
  // G[k]    = sum{1..C-1} S[l, k] 
  std::unique_ptr<point_t[]> Sk( new point_t[K] );
  std::vector<bool> Sk_inited(K);
  std::unique_ptr<point_t[]> Gk( new point_t[K] );
  std::vector<bool> Gk_inited(K);
  for (size_t l = C - 1; l > 0; --l)
  {
    for (size_t k = 0; k < K; ++k)
    {
      size_t bucket_id = l * K + k;
      if (buckets_inited[bucket_id])
      {
        if (Sk_inited[k])
          Sk[k] += buckets[bucket_id];
        else
        {
          Sk[k] = buckets[bucket_id];
          Sk_inited[k] = true;
        }
      }

      if (Sk_inited[k])
      {
        if (Gk_inited[k])
          Gk[k] += Sk[k];
        else
        {
          Gk[k] = Sk[k];
          Gk_inited[k] = true;
        }
      }
    }
  }

  // the third loop: Horner’s rule
  point_t result = Gk_inited[K - 1] ? Gk[K - 1] : c_point_0;
  for (size_t k = K - 2; k != SIZE_MAX; --k)
  {
    result.modify_mul_pow_2(c);
    if (Gk_inited[k])
      result += Gk[k];
  }

  result += summand;

  if (!result.is_zero())
  {
    LOG_PRINT_L0("msm result is non zero: " << result);
    return false;
  }

  return true;
}



////////////////////////////////////////////////////////////////////////////////

//template<typename CT>
//struct mes_msm_and_check_zero_pippenger_v1
//{
//  static bool msm_and_check_zero(const scalar_vec_t& g_scalars, const scalar_vec_t& h_scalars, const point_t& summand, size_t c)
//  {
//    return msm_and_check_zero_pippenger_v1<CT>(g_scalars, h_scalars, summand, c);
//  }
//};
//
//template<typename CT>
//struct mes_msm_and_check_zero_pippenger_v2
//{
//  static bool msm_and_check_zero(const scalar_vec_t& g_scalars, const scalar_vec_t& h_scalars, const point_t& summand, size_t c)
//  {
//    return msm_and_check_zero_pippenger_v2<CT>(g_scalars, h_scalars, summand, c);
//  }
//};

template<typename CT>
struct mes_msm_and_check_zero_pippenger_v3
{
  static bool msm_and_check_zero(const scalar_vec_t& g_scalars, const scalar_vec_t& h_scalars, const point_t& summand, size_t c)
  {
    return msm_and_check_zero_pippenger_v3<CT>(g_scalars, h_scalars, summand, c);
  }
};

template<typename CT>
struct mes_msm_and_check_zero_pippenger_v4
{
  static bool msm_and_check_zero(const scalar_vec_t& g_scalars, const scalar_vec_t& h_scalars, const point_t& summand, size_t c)
  {
    return msm_and_check_zero_pippenger_v4<CT>(g_scalars, h_scalars, summand, c);
  }
};



struct pme_runner_i
{
  virtual ~pme_runner_i() {}
  virtual bool iteration(bool warmup) = 0;
};

template<size_t N, typename CT, template<typename> typename selector_t>
struct pme_runner_t : public pme_runner_i
{
  pme_runner_t(const char* testname_, size_t pip_partition_bits_c)
    : testname(testname_)
    , pip_partition_bits_c(pip_partition_bits_c)
  {
    testname += std::string(", ") + std::string(typeid(selector_t<CT>).name()).erase(0, 11) + std::string(", c = ") + epee::string_tools::num_to_string_fast(pip_partition_bits_c);
    std::cout << testname << ENDL;
  }
  virtual ~pme_runner_t()
  {
    if (timings.empty())
      return;

    uint64_t median = 0;
    auto median_it = timings.begin() + timings.size() / 2;
    std::nth_element(timings.begin(), median_it, timings.end());
    median = *median_it;
    if (timings.size() % 2 == 0)
    {
      auto max_it = std::max_element(timings.begin(), median_it);
      median = (median + *max_it) /  2;
    }

    uint64_t total_time = std::accumulate(timings.begin(), timings.end(), 0);
    std::cout << std::left << std::setw(100) << testname << "  :  " << std::setw(5) << median << " (median),  " << std::setw(5) << total_time / timings.size() << " (avg), mcs" << ENDL;
  }

  virtual bool iteration(bool warmup)
  {
    scalar_vec_t g_scalars, h_scalars;
    g_scalars.resize_and_make_random(N);
    g_scalars[0] = c_scalar_Lm1;
    //std::cout << "bit 251: " << g_scalars[0].get_bit(251) << ", bit 252: " << g_scalars[0].get_bit(252) << ENDL;
    h_scalars.resize_and_make_random(N);
    point_t sum = c_point_0;
    for(size_t i = 0; i < N; ++i)
    {
      //g_scalars[i].m_u64[3] = 0;
      //h_scalars[i].m_u64[3] = 0;
      //g_scalars[i].m_s[31] = 0;
      //h_scalars[i].m_s[31] = 0;
      sum += g_scalars[i] * CT::get_generator(false, i) + h_scalars[i] * CT::get_generator(true, i);
    }

    TIME_MEASURE_START(t);
    bool r = selector_t<CT>::msm_and_check_zero(g_scalars, h_scalars, -sum, pip_partition_bits_c);
    TIME_MEASURE_FINISH(t);
    ASSERT_TRUE(r);

    if (!warmup)
      timings.push_back(t);
    return true;
  }

  std::vector<uint64_t> timings;
  std::string testname;
  size_t pip_partition_bits_c;
};


TEST(perf, msm)
{
  bool r = false;

  std::deque<std::unique_ptr<pme_runner_i>> runners;
  //runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v1> >("Zarcanum, BPPE, 128", 1));
  //runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v1> >("Zarcanum, BPPE, 128", 2));
  //runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v1> >("Zarcanum, BPPE, 128", 3));
  //runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v1> >("Zarcanum, BPPE, 128", 4));
  //runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v1> >("Zarcanum, BPPE, 128", 5));
  //runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v1> >("Zarcanum, BPPE, 128", 6));
  //runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v1> >("Zarcanum, BPPE, 128", 7));
  //runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v1> >("Zarcanum, BPPE, 128", 8));
  //runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v1> >("Zarcanum, BPPE, 128", 9));
  //runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v1> >("ZC out, BPP, 256", 1));
  //runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v1> >("ZC out, BPP, 256", 2));
  //runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v1> >("ZC out, BPP, 256", 3));
  //runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v1> >("ZC out, BPP, 256", 4));
  //runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v1> >("ZC out, BPP, 256", 5));
  //runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v1> >("ZC out, BPP, 256", 6));
  //runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v1> >("ZC out, BPP, 256", 7));
  //runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v1> >("ZC out, BPP, 256", 8));
  //runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v1> >("ZC out, BPP, 256", 9));

  //runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v2> >("Zarcanum, BPPE, 128", 1));
  //runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v2> >("Zarcanum, BPPE, 128", 2));
  //runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v2> >("Zarcanum, BPPE, 128", 3));
  //runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v2> >("Zarcanum, BPPE, 128", 4));
  //runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v2> >("Zarcanum, BPPE, 128", 5));
  //runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v2> >("Zarcanum, BPPE, 128", 6));
  //runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v2> >("Zarcanum, BPPE, 128", 7));
  //runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v2> >("Zarcanum, BPPE, 128", 8));
  //runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v2> >("Zarcanum, BPPE, 128", 9));
  //runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v2> >("ZC out, BPP, 256", 1));
  //runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v2> >("ZC out, BPP, 256", 2));
  //runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v2> >("ZC out, BPP, 256", 3));
  //runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v2> >("ZC out, BPP, 256", 4));
  //runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v2> >("ZC out, BPP, 256", 5));
  //runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v2> >("ZC out, BPP, 256", 6));
  //runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v2> >("ZC out, BPP, 256", 7));
  //runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v2> >("ZC out, BPP, 256", 8));
  //runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v2> >("ZC out, BPP, 256", 9));


  runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v3> >("Zarcanum, BPPE, 128 +++++++++++", 7));
  runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v3> >("ZC out, BPP, 256    +++++++++++", 7));

  runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v4> >("Zarcanum, BPPE, 128 ###########", 7));
  runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v4> >("ZC out, BPP, 256    ###########", 7));

  //runners.emplace_front(std::make_unique< pme_runner_t<128, bpp_crypto_trait_Zarcanum,  mes_msm_and_check_zero_pippenger_v1> >("Zarcanum, BPPE, 128", 7));
  //runners.emplace_front(std::make_unique< pme_runner_t<256, bpp_crypto_trait_ZC_out,    mes_msm_and_check_zero_pippenger_v1> >("ZC out, BPP, 256", 7));


  std::cout << "warm up..." << ENDL;
  size_t runs_count = 30;
  for(size_t k = 0; k < runs_count; ++k)
  {
    for(auto& runner : runners)
      ASSERT_TRUE(runner->iteration(true));
  }

  runs_count = 200;
  for(size_t k = 0; k < runs_count; ++k)
  {
    for(auto& runner : runners)
      ASSERT_TRUE(runner->iteration(false));

    size_t done_percent = 100 * k / runs_count;
    if (100 * (k + 1) / runs_count > done_percent && done_percent % 5 == 0)
      std::cout << done_percent << " %" << ENDL;
  }

  return true;
}



template<typename T>
bool perf_generators_runner(const T& generator, const char* title)
{
  const size_t warmup_rounds = 20;
  const size_t rounds = 500;
  const size_t inner_rounds = 128;
  uint64_t h = 0;
  std::vector<uint64_t> timings;

  size_t N = 1024;
  scalar_vec_t scalars;
  scalars.resize_and_make_random(N);
  std::vector<point_t> points(N);

  for(size_t i = 0; i < warmup_rounds; ++i)
    for(size_t j = 0; j < inner_rounds; ++j)
      points[(i + j) % N] = scalars[(i + j) % N] * generator;

  h = hash_64(points.data(), points.size() * sizeof(point_t));

  for(size_t i = 0; i < rounds; ++i)
  {
    TIME_MEASURE_START(t);
    for(size_t j = 0; j < inner_rounds; ++j)
      points[(i + j) % N] = scalars[(i + j) % N] * generator;
    TIME_MEASURE_FINISH(t);
    timings.push_back(t);
  }

  h ^= hash_64(points.data(), points.size() * sizeof(point_t));

  std::cout << std::left << std::setw(20) << title << " : " << std::setw(5) << std::fixed << std::setprecision(1) << (double)epee::misc_utils::median(timings) / inner_rounds << " mcs,  hash = " << h << ENDL;

  return true;
}

TEST(perf, generators)
{
#define TEST_GENERATOR(G) ASSERT_TRUE(perf_generators_runner(G, #G))

  TEST_GENERATOR(c_point_0);
  TEST_GENERATOR(c_point_G);
  TEST_GENERATOR(c_point_H);
  TEST_GENERATOR(c_point_H2);
  TEST_GENERATOR(c_point_U);
  TEST_GENERATOR(c_point_X);
  TEST_GENERATOR(c_point_H_plus_G);
  TEST_GENERATOR(c_point_H_minus_G);

  return true;
}
