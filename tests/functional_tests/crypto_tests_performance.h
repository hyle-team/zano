// Copyright (c) 2021 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once


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


inline std::ostream &operator <<(std::ostream &o, const crypto::ge_precomp v)
{
  o << "{{";
  
  for(size_t i = 0; i < 9; ++i)
    o << v.yplusx[i] << ", ";

  o << v.yplusx[9] << "},\n {";
  
  for(size_t i = 0; i < 9; ++i)
    o << v.yminusx[i] << ", ";
  
  o << v.yminusx[9] << "},\n {";
  
  for(size_t i = 0; i < 9; ++i)
    o << v.xy2d[i] << ", ";
  
  o << v.xy2d[9] << "}}\n";
  return o;
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

  run("ge_add(p3 + p3)", 50000, [](timer_t& t, size_t rounds) {
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

  run("ge_scalarmult_base()", 5000, [](timer_t& t, size_t rounds) {
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

  return true;
}
