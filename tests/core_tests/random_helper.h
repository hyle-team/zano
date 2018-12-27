// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#define RANDOM_STATE_SIZE 200
extern "C" volatile union hash_state state; // field defined in random.c
static_assert(RANDOM_STATE_SIZE >= HASH_DATA_AREA, "Invalid RANDOM_STATE_SIZE");

// DISCLAIMER: designed for tests puposes ONLY!
// This class is not intended to be used neither in multi-threaded environment nor in production.

// Remebers random state at ctor, restores it at dtor
struct random_state_test_restorer
{
  random_state_test_restorer()
  {
    memcpy(m_state, const_cast<hash_state*>(&::state), RANDOM_STATE_SIZE);
  }

  ~random_state_test_restorer()
  {
    memcpy(const_cast<hash_state*>(&::state), m_state, RANDOM_STATE_SIZE);
  }

  static void reset_random(uint64_t seed = 0)
  {
    memset(const_cast<hash_state*>(&::state), 0, RANDOM_STATE_SIZE);
    memcpy(const_cast<hash_state*>(&::state), &seed, sizeof seed);
  }

private:
  uint8_t m_state[RANDOM_STATE_SIZE];
};

std::string get_random_text(size_t len);

bool random_state_manupulation_test();
bool random_evenness_test();
bool get_random_text_test();
