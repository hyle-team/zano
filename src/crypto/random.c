// Copyright (c) 2018-2025 Zano Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "hash-ops.h"
#include "random.h"

static_assert(RANDOM_STATE_SIZE >= HASH_DATA_AREA, "Invalid RANDOM_STATE_SIZE");

#if defined(_WIN32)

#include <windows.h>
#include <bcrypt.h>

// thread-safe version
bool generate_system_random_bytes(size_t n, void *result)
{
  if (n == 0)
    return true;

  if (result == NULL)
    return false;
    
  NTSTATUS status = BCryptGenRandom(NULL, (PUCHAR)result, (ULONG)n, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
  return BCRYPT_SUCCESS(status);
}

void generate_system_random_bytes_or_die(size_t n, void *result)
{
  if (!generate_system_random_bytes(n, result))
  {
    fprintf(stderr, "Error: generate_system_random_bytes failed and this is fatal\n\n");
    fflush(stderr);
    _exit(EXIT_FAILURE);
  }
}

#else

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

bool generate_system_random_bytes(size_t n, void *result)
{
  int fd;
    
  fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    return false;

  size_t bytes_read = 0;
  while (bytes_read < n)
  {
    ssize_t res = read(fd, (char*)result + bytes_read, n - bytes_read);
    if (res < 0)
    {
      if (errno != EINTR)
      {
        close(fd);
        return false;
      }
      // EINTR - interrupted by signal, continue reading
    }
    else if (res == 0)
    {
      // EOF - should not happen with /dev/urandom
      close(fd);
      return false;
    }
    else
    {
      bytes_read += res;
    }
  }
    
  close(fd); // don't check, 'cuz failing to close /dev/urandom is not truly fatal, the OS will clean up
  return true;
}

void generate_system_random_bytes_or_die(size_t n, void *result)
{
  if (!generate_system_random_bytes(n, result))
  {
    fprintf(stderr, "FATAL: Failed to generate %zu random bytes: %s\n\n", n, strerror(errno));
    fflush(stderr);
    exit(EXIT_FAILURE);
  }
}

#endif

static union hash_state state;

static_assert(sizeof(union hash_state) == RANDOM_STATE_SIZE, "RANDOM_STATE_SIZE and hash_state size missmatch");

#if !defined(NDEBUG)
static volatile int curstate; /* To catch thread safety problems. */
#endif
/*
FINALIZER(deinit_random) {
#if !defined(NDEBUG)
  assert(curstate == 1);
  curstate = 0;
#endif
  memset(&state, 0, sizeof(union hash_state));
}
*/

void init_random(void)
{
  generate_system_random_bytes_or_die(HASH_DATA_AREA, &state);

#if !defined(NDEBUG)
  assert(curstate == 0);
  curstate = 1;
#endif
}


void grant_random_initialize_no_lock(void)
{
  static bool initalized = false;
  if(!initalized)
  {
    init_random();
    initalized = true;
  }
}

void random_prng_initialize_with_seed_no_lock(uint64_t seed)
{
  grant_random_initialize_no_lock();
#if !defined(NDEBUG)
  assert(curstate == 1);
  curstate = 3;
#endif
  memset(&state, 0, sizeof state);
  memcpy(&state, &seed, sizeof seed);
  for(size_t i = 0, count = seed & 31; i < count; ++i)
    hash_permutation(&state);
#if !defined(NDEBUG)
  assert(curstate == 3);
  curstate = 1;
#endif
}

void random_prng_get_state_no_lock(void *state_buffer, const size_t buffer_size)
{
  grant_random_initialize_no_lock();
#if !defined(NDEBUG)
  assert(curstate == 1);
  curstate = 4;
#endif

  assert(sizeof state == buffer_size);
  memcpy(state_buffer, &state, buffer_size);

#if !defined(NDEBUG)
  assert(curstate == 4);
  curstate = 1;
#endif
}

void random_prng_set_state_no_lock(const void *state_buffer, const size_t buffer_size)
{
  grant_random_initialize_no_lock();
#if !defined(NDEBUG)
  assert(curstate == 1);
  curstate = 5;
#endif

  assert(sizeof state == buffer_size);
  memcpy(&state, state_buffer, buffer_size);

#if !defined(NDEBUG)
  assert(curstate == 5);
  curstate = 1;
#endif
}

void generate_random_bytes_no_lock(size_t n, void *result)
{
  grant_random_initialize_no_lock();
#if !defined(NDEBUG)
  assert(curstate == 1);
  curstate = 2;
#endif
  if (n == 0) {
#if !defined(NDEBUG)
    assert(curstate == 2);
    curstate = 1;
#endif
    return;
  }
  for (;;) {
    hash_permutation(&state);
    if (n <= HASH_DATA_AREA) {
      memcpy(result, &state, n);
#if !defined(NDEBUG)
      assert(curstate == 2);
      curstate = 1;
#endif
      return;
    } else {
      memcpy(result, &state, HASH_DATA_AREA);
      result = padd(result, HASH_DATA_AREA);
      n -= HASH_DATA_AREA;
    }
  }
}
