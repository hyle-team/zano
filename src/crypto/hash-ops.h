// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#if !defined(__cplusplus)

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "common/int-util.h"
#include "warnings.h"

static inline void *padd(void *p, size_t i) {
  return (char *) p + i;
}

static inline const void *cpadd(const void *p, size_t i) {
  return (const char *) p + i;
}

PUSH_VS_WARNINGS
DISABLE_VS_WARNINGS(4267)
static_assert(sizeof(size_t) == 4 || sizeof(size_t) == 8, "size_t must be 4 or 8 bytes long");
static inline void place_length(uint8_t *buffer, size_t bufsize, size_t length) {
  if (sizeof(size_t) == 4) {
    *(uint32_t *) padd(buffer, bufsize - 4) = swap32be(length);
  } else {
    *(uint64_t *) padd(buffer, bufsize - 8) = swap64be(length);
  }
}
POP_VS_WARNINGS

#pragma pack(push, 1)
union hash_state {
  uint8_t b[200];
  uint64_t w[25];
};
#pragma pack(pop)
static_assert(sizeof(union hash_state) == 200, "Invalid structure size");

void hash_permutation(union hash_state *state);
void hash_process(union hash_state *state, const uint8_t *buf, size_t count);

#endif


#define HASH_SIZE  32
#define HASH_DATA_AREA 136


void cn_fast_hash_old(const void *data, size_t length, char *hash);
void cn_fast_hash(const void *data, size_t length, char *hash);

void hash_extra_blake(const void *data, size_t length, char *hash);
void hash_extra_groestl(const void *data, size_t length, char *hash);
void hash_extra_jh(const void *data, size_t length, char *hash);
void hash_extra_skein(const void *data, size_t length, char *hash);

void tree_hash(const char (*hashes)[HASH_SIZE], size_t count, char *root_hash);

/* Checked caller-scratch form of tree_hash(): same root bytes, no allocation.
   Scratch is measured in HASH_SIZE-byte units; tree_hash_scratch_count(count)
   is the exact number required (0 for count <= 2, otherwise the greatest power
   of two <= count - 1, which is < count). Writes scratch[0 .. required - 1]
   only, then root_hash. Returns 1 on success; returns 0 and writes nothing to
   root_hash or scratch when hashes/root_hash is NULL, count is 0, count *
   HASH_SIZE overflows, or (count > 2) scratch is NULL or scratch_count is below
   the required count. scratch may be NULL when count <= 2. hashes, scratch
   and root_hash must not overlap. Only the return value reports success. */
size_t tree_hash_scratch_count(size_t count);
int tree_hash_checked(const char (*hashes)[HASH_SIZE], size_t count,
  char (*scratch)[HASH_SIZE], size_t scratch_count, char *root_hash);
