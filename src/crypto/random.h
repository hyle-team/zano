// Copyright (c) 2018-2025 Zano Project
// Copyright (c) 2014-2018 The Boolberry developers
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stddef.h>
#include <stdint.h>

// NOT thread-safe, use with caution
void generate_random_bytes_no_lock(size_t n, void *result);

#define RANDOM_STATE_SIZE 200

// explicitly define USE_INSECURE_RANDOM_RPNG_ROUTINES for using random_initialize_with_seed
#ifdef USE_INSECURE_RANDOM_RPNG_ROUTINES
// reinitializes PRNG with the given seed
// !!!ATTENTION!!!! Improper use of this routine may lead to SECURITY BREACH!
// Use with care and ONLY for tests or debug purposes!
void random_prng_initialize_with_seed_no_lock(uint64_t seed);

// gets internal RPNG state (state_buffer should be 200 bytes long)
void random_prng_get_state_no_lock(void *state_buffer, const size_t buffer_size);

// sets internal RPNG state (state_buffer should be 200 bytes long)
// !!!ATTENTION!!!! Improper use of this routine may lead to SECURITY BREACH!
// Use with care and ONLY for tests or debug purposes!
void random_prng_set_state_no_lock(const void *state_buffer, const size_t buffer_size);

#endif // #ifdef USE_INSECURE_RANDOM_RPNG_ROUTINES
