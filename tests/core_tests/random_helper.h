// Copyright (c) 2014-2019 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

// DISCLAIMER: designed for tests puposes ONLY!
// This class is not intended to be used neither in multi-threaded environment nor in production.

#ifdef USE_INSECURE_RANDOM_RPNG_ROUTINES

// Remebers random state at ctor, restores it at dtor
struct random_state_test_restorer
{
  random_state_test_restorer();
  ~random_state_test_restorer();
  static void reset_random(uint64_t seed = 0);

private:
  uint8_t m_state[RANDOM_STATE_SIZE];
};

#endif // #ifdef USE_INSECURE_RANDOM_RPNG_ROUTINES

std::string get_random_text(size_t len);

bool random_state_manupulation_test();
bool random_evenness_test();
bool get_random_text_test();
