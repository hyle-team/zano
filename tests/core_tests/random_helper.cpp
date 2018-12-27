// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "random_helper.h"

std::string get_random_text(size_t len)
{
  static const char     text_chars[]    = "abcdefghijklmnopqrstuvwxyz" "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "0123456789" "~!@#$%^&*()-=_+\\/?,.<>|{}[]`';:\" ";
  static const uint8_t  text_chars_size = sizeof text_chars - 1; // to avoid '\0'
  std::string result;

  if (len < 1)
    return result;

  result.resize(len);
  char* result_buf = &result[0];
  uint8_t* rnd_indices = new uint8_t[len];
  crypto::generate_random_bytes(len, rnd_indices);

  size_t n = len / 4, i;
  for (i = 0; i < n; ++i)
  {
    result_buf[4 * i + 0] = text_chars[rnd_indices[4 * i + 0] % text_chars_size];
    result_buf[4 * i + 1] = text_chars[rnd_indices[4 * i + 1] % text_chars_size];
    result_buf[4 * i + 2] = text_chars[rnd_indices[4 * i + 2] % text_chars_size];
    result_buf[4 * i + 3] = text_chars[rnd_indices[4 * i + 3] % text_chars_size];
  }
  for (size_t j = i * 4; j < len; ++j)
  {
    result_buf[j] = text_chars[rnd_indices[j] % text_chars_size];
  }

  delete[] rnd_indices;
  return result;
}

//------------------------------------------------------------------------------

bool random_state_manupulation_test()
{
  // This test checks that random generator can be manipulated by random_helper routines in predictable way.
  // NOTE: If the test fails, it's most likely that random state permutation procedure was changed OR the state can't be correctly stored/loaded.

  static const uint64_t my_own_random_seed = 4669201609102990671;
  static const char*    my_random_str = "18b79ebb56744e9bafa462631c6f7d760af2b788";
  static const size_t   rnd_buf_len = 20;

  uint64_t first_random_after_state_saved = 0;
  {
    random_state_test_restorer rstr; // <- here random state is saved
    first_random_after_state_saved = crypto::rand<uint64_t>();

    random_state_test_restorer::reset_random(my_own_random_seed); // random seeded, entering deterministic mode...

    std::string rnd_buf(rnd_buf_len, '\x00');
    for (size_t i = 0; i < rnd_buf_len; ++i)
      rnd_buf[i] = crypto::rand<char>();

    std::string hex = epee::string_tools::buff_to_hex_nodelimer(rnd_buf);
    CHECK_AND_ASSERT_MES(hex == my_random_str, false, "Incorrect random data after seeding. Expected: " << my_random_str << " Fetched: " << hex);

    // here rstr dtor is called and random state should be restored
  }

  // make sure random state is correctly restored
  uint64_t first_random_after_state_restored = crypto::rand<uint64_t>();
  CHECK_AND_ASSERT_MES(first_random_after_state_saved == first_random_after_state_restored, false, "Core random generator state wasn't restored correctly");

  return true;
}

bool check_random_evenness(size_t modulus = 11, size_t count = 10000, double allowed_deviation = 0.05)
{
  std::vector<size_t> m;
  m.resize(modulus);
  for (size_t i = 0; i < count; ++i)
  {
    size_t r = crypto::rand<size_t>() % modulus;
    ++m[r];
  }

  double mean = 0;
  double variance = 0;
  for (size_t i = 0; i < modulus; ++i)
  {
    mean += static_cast<double>(i)* m[i] / count;
    variance += static_cast<double>(i)* i * m[i] / count;
  }
  variance -= mean * mean;

  double estimated_mean = (static_cast<double>(modulus)-1) / 2;
  double estimated_variance = (static_cast<double>(modulus)* modulus - 1) / 12;

  LOG_PRINT_L0("Random test for modulus " << modulus << ", trials count " << count << std::endl <<
    "Estimated mean:     " << std::fixed << std::setw(10) << std::setprecision(5) << estimated_mean << " Mean:     " << std::fixed << std::setw(10) << std::setprecision(5) << mean << std::endl <<
    "Estimated variance: " << std::fixed << std::setw(10) << std::setprecision(5) << estimated_variance << " Variance: " << std::fixed << std::setw(10) << std::setprecision(5) << variance);

  CHECK_AND_ASSERT_MES(fabs(mean - estimated_mean) < allowed_deviation * estimated_mean, false, "Mean deviation is more than " << allowed_deviation);
  CHECK_AND_ASSERT_MES(fabs(variance - estimated_variance) < allowed_deviation * estimated_variance, false, "Variance deviation is more than " << allowed_deviation);

  return true;
}

bool random_evenness_test()
{
  bool r = true;
  r = r & check_random_evenness(5,   10000);
  r = r & check_random_evenness(11,  11000);
  r = r & check_random_evenness(37,  50000);
  r = r & check_random_evenness(97,  97000);
  r = r & check_random_evenness(113, 113000);
  return r;
}

bool get_random_text_test()
{
  random_state_test_restorer rstr; // to restore random generator state afterwards
  random_state_test_restorer::reset_random(0); // to make the test determinictic

  TIME_MEASURE_START(time_total);

  size_t max_len = 16 * 1024;
  size_t trials_count = 500;
  for (size_t i = 0; i < trials_count; ++i)
  {
    size_t sz = crypto::rand<size_t>() % max_len;
    std::string r = get_random_text(sz);
    CHECK_AND_ASSERT_MES(r.size() == sz, false, "get_random_text(" << sz << ") returned " << r.size() << " bytes:" << ENDL << r);
  }

  TIME_MEASURE_FINISH(time_total);

  LOG_PRINT_L0("Random text test: trials: " << trials_count << ", total time: " << time_total << " ms.");

  return true;
}
