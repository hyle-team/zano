// Copyright (c) 2014-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <memory.h>
#include <string>
#include <vector>

namespace crypto
{
  extern "C"
  {
    #include "keccak.h"
  }

  // memory-hard password stretching for wallet file encryption
  //
  // construction is the ROMix algorithm from scrypt (C. Percival, 2009, "Stronger Key Derivation via Sequential Memory-Hard Functions"),
  // adapted to use Keccak-256 instead of scrypt's PBKDF2/Salsa20 core so we don't pull in any new crypto primitive
  inline void derive_key_romix_keccak(const char (&hdss)[32],
    const void* password_data, size_t password_data_size,
    const void* salt_data, size_t salt_data_size,
    uint8_t N_log2, uint8_t phase2_log2_reduction,
    uint8_t (&out_stretched)[32])
  {
    if (N_log2 < 10)
      N_log2 = 10; // 32kb ram min
    if (N_log2 > 30)
      N_log2 = 30; // 32gb ram max
    const uint64_t N = (uint64_t)1 << N_log2;
    const uint64_t mask = N - 1; // N is power of 2 -> "x & mask" == "x % N"

    if (phase2_log2_reduction >= N_log2)
      phase2_log2_reduction = (uint8_t)(N_log2 - 1);
    const uint64_t phase2_iters = N >> phase2_log2_reduction;

    // phase 1 - sequential fill of V (scrypt-style SMix step 1)
    // V[0]   = keccak(hdss || salt || password)
    // V[i]   = keccak(V[i-1] || i_le64) for i in 1..N
    // serial dependency chain prevents shortcut computation of later blocks
    std::vector<uint8_t> V(N * 32);
    {
      std::string seed; // [hdss 32B][salt 16B][password ?B]
      seed.reserve(32 + salt_data_size + password_data_size);
      seed.append(hdss, 32);
      if (salt_data_size)
        seed.append(reinterpret_cast<const char*>(salt_data), salt_data_size);
      if (password_data_size)
        seed.append(reinterpret_cast<const char*>(password_data), password_data_size);
      keccak(reinterpret_cast<const uint8_t*>(seed.data()), (int)seed.size(), V.data(), 32);
      if (!seed.empty())
        memset(const_cast<char*>(seed.data()), 0, seed.size());
    }

    uint8_t block_in[40]; // 32 (prev block) + 8 (i_le64) = 40
    for (uint64_t i = 1; i < N; ++i)
    {
      memcpy(block_in, V.data() + (i - 1) * 32, 32);
      for (int b = 0; b < 8; ++b)
        block_in[32 + b] = (uint8_t)(i >> (8 * b));
      keccak(block_in, 40, V.data() + i * 32, 32);
    }

    // phase 2 - data-dependent random walk (scrypt-style SMix step 2)
    // X = V[N-1]
    // for j in 0..phase2_iters:
    //   idx = first 8 bytes of X, mod N
    //   X = keccak((X XOR V[idx]) || j_le64)
    // read index depends on the current X, so V cannot be streamed -
    // the attacker must keep V resident or pay the TMTO penalty
    uint8_t X[32];
    memcpy(X, V.data() + (N - 1) * 32, 32);
    uint8_t mix[40];
    for (uint64_t j = 0; j < phase2_iters; ++j)
    {
      uint64_t idx_raw = 0;
      for (int b = 0; b < 8; ++b)
        idx_raw |= (uint64_t)X[b] << (8 * b);
      const uint64_t idx = idx_raw & mask;
      const uint8_t* Vi = V.data() + idx * 32;
      for (int k = 0; k < 32; ++k)
        mix[k] = X[k] ^ Vi[k];
      for (int b = 0; b < 8; ++b)
        mix[32 + b] = (uint8_t)(j >> (8 * b));
      keccak(mix, 40, X, 32);
    }

    memcpy(out_stretched, X, 32);

    // wipe buffs
    memset(V.data(), 0, V.size());
    memset(X, 0, sizeof(X));
    memset(mix, 0, sizeof(mix));
  }
}
