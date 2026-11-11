// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hash-ops.h"

/* Greatest power of two <= count - 1 (count > 2): the reduction width. */
static size_t tree_hash_width(size_t count) {
  size_t i;
  size_t cnt = count - 1;
  for (i = 1; i < 8 * sizeof(size_t); i <<= 1) {
    cnt |= cnt >> i;
  }
  cnt &= ~(cnt >> 1);
  return cnt;
}

/* Shared reduction for count > 2; ints holds cnt == tree_hash_width(count) units. */
static void tree_hash_reduce(const char (*hashes)[HASH_SIZE], size_t count, size_t cnt,
    char (*ints)[HASH_SIZE], char *root_hash) {
  size_t i, j;
  memcpy(ints, hashes, (2 * cnt - count) * HASH_SIZE);
  for (i = 2 * cnt - count, j = 2 * cnt - count; j < cnt; i += 2, ++j) {
    cn_fast_hash(hashes[i], 64, ints[j]);
  }
  assert(i == count);
  while (cnt > 2) {
    cnt >>= 1;
    for (i = 0, j = 0; j < cnt; i += 2, ++j) {
      cn_fast_hash(ints[i], 64, ints[j]);
    }
  }
  cn_fast_hash(ints[0], 64, root_hash);
}

void tree_hash(const char (*hashes)[HASH_SIZE], size_t count, char *root_hash) {
  assert(count > 0);
  if (count == 1) {
    memcpy(root_hash, hashes, HASH_SIZE);
  } else if (count == 2) {
    cn_fast_hash(hashes, 2 * HASH_SIZE, root_hash);
  } else {
    size_t cnt = tree_hash_width(count);
    char (*ints)[HASH_SIZE];
    ints = (char (*)[HASH_SIZE])malloc(cnt * HASH_SIZE);
    if (!ints) {
      memset(root_hash, 0, HASH_SIZE);
      return;
    }
    tree_hash_reduce(hashes, count, cnt, ints, root_hash);
    free(ints);
  }
}

size_t tree_hash_scratch_count(size_t count) {
  return count > 2 ? tree_hash_width(count) : 0;
}

int tree_hash_checked(const char (*hashes)[HASH_SIZE], size_t count,
    char (*scratch)[HASH_SIZE], size_t scratch_count, char *root_hash) {
  size_t cnt;
  if (!hashes || !root_hash || count == 0 || count > SIZE_MAX / HASH_SIZE) {
    return 0;
  }
  if (count == 1) {
    memcpy(root_hash, hashes, HASH_SIZE);
    return 1;
  }
  if (count == 2) {
    cn_fast_hash(hashes, 2 * HASH_SIZE, root_hash);
    return 1;
  }
  cnt = tree_hash_width(count); /* 2 <= cnt < count, so cnt * HASH_SIZE cannot overflow. */
  if (!scratch || scratch_count < cnt) {
    return 0;
  }
  tree_hash_reduce(hashes, count, cnt, scratch, root_hash);
  return 1;
}
