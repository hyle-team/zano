// Copyright (c) 2014-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "crypto/crypto.h"

// DISCLAIMER: designed for tests puposes ONLY!
// This class is not intended to be used neither in multi-threaded environment nor in production.

#ifdef USE_INSECURE_RANDOM_RPNG_ROUTINES

// Remebers random state at ctor, restores it at dtor
struct random_state_test_restorer
{
  random_state_test_restorer()
  {
    crypto::random_prng_get_state(&m_state, sizeof m_state);
  }
  ~random_state_test_restorer()
  {
    crypto::random_prng_set_state(&m_state, sizeof m_state);
  }
  static void reset_random(uint64_t seed = 0)
  {
    crypto::random_prng_initialize_with_seed(seed);
  }
private:
  uint8_t m_state[RANDOM_STATE_SIZE];
};

#endif // #ifdef USE_INSECURE_RANDOM_RPNG_ROUTINES

inline std::string get_random_text(size_t len)
{
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
}

// boundaries are included
inline uint64_t random_in_range(uint64_t from, uint64_t to)
{
  if (from == to)
    return from;
  CHECK_AND_ASSERT_MES(from < to, 0, "Invalid arguments: from = " << from << ", to = " << to);
  return crypto::rand<uint64_t>() % (to - from + 1) + from;
}


bool random_state_manupulation_test();
bool random_evenness_test();
bool get_random_text_test();
