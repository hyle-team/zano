// Copyright (c) 2014-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>

#include "crypto/chacha.h"
#include "currency_core/crypto_config.h"
#include "currency_core/currency_config.h"

namespace
{
  // old single keccak
  inline void old_kdf_single_keccak(const std::string& password, uint8_t out[32])
  {
    std::string buff;
    buff.reserve(32 + password.size() + 8);
    buff.append(CRYPTO_HDS_CHACHA_WALLET_HEADER, 32);
    buff.append(password);
    uint64_t index = 0;
    buff.append(reinterpret_cast<const char*>(&index), sizeof(index));
    crypto::keccak(reinterpret_cast<const uint8_t*>(buff.data()), (int)buff.size(), out, 32);
  }

  double seconds_since(std::chrono::steady_clock::time_point t0)
  {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  }

  std::string fmt_time(double seconds)
  {
    std::ostringstream o;
    o << std::fixed << std::setprecision(3);
    if (seconds < 1e-6)            o << (seconds * 1e9)       << " ns";
    else if (seconds < 1e-3)       o << (seconds * 1e6)       << " us";
    else if (seconds < 1.0)        o << (seconds * 1e3)       << " ms";
    else if (seconds < 3600.0)     o << seconds               << " s";
    else if (seconds < 86400.0)    o << (seconds / 3600.0)    << " h";
    else if (seconds < 31557600.0) o << (seconds / 86400.0)   << " d";
    else                           o << (seconds / 31557600.0) << " years";
    return o.str();
  }

  std::string fmt_count(double n)
  {
    std::ostringstream o;
    o << std::fixed << std::setprecision(2);
    if (n >= 1e12)     o << (n / 1e12) << "T";
    else if (n >= 1e9) o << (n / 1e9)  << "G";
    else if (n >= 1e6) o << (n / 1e6)  << "M";
    else if (n >= 1e3) o << (n / 1e3)  << "K";
    else               o << std::setprecision(1) << n;
    return o.str();
  }

  std::string fmt_dollars(double usd)
  {
    std::ostringstream o;
    if (usd >= 1e12)      o << std::scientific << std::setprecision(2) << usd;
    else if (usd >= 1e9)  o << std::fixed << std::setprecision(2) << "$" << (usd / 1e9) << "B";
    else if (usd >= 1e6)  o << std::fixed << std::setprecision(2) << "$" << (usd / 1e6) << "M";
    else if (usd >= 1e3)  o << std::fixed << std::setprecision(2) << "$" << (usd / 1e3) << "K";
    else if (usd >= 0.01) o << std::fixed << std::setprecision(2) << "$" << usd;
    else                  o << std::scientific << std::setprecision(2) << "$" << usd;
    return o.str();
  }

  struct password_class
  {
    const char* name;
    double entropy_bits;
  };

  // avg bruteforce: half of the keyspace, i.e. 2^(H-1) attempts
  void print_bruteforce_projection(const char* label, double per_attempt_seconds, double cost_per_second_usd)
  {
    static const password_class classes[] =
    {
      { "6 lowercase chars  (28 bits)", 28.0 },
      { "8 alphanum chars   (48 bits)", 48.0 },
      { "10 random chars    (60 bits)", 60.0 },
      { "12 random chars    (72 bits)", 72.0 },
      { "4 diceware words   (52 bits)", 52.0 },
      { "6 diceware words   (78 bits)", 78.0 },
    };

    std::cout << "  " << label << ":" << std::endl;
    std::cout << "    attempts/sec: " << fmt_count(1.0 / per_attempt_seconds) << std::endl;
    for (const auto& c : classes)
    {
      double attempts = std::ldexp(1.0, (int)c.entropy_bits - 1);
      double t = attempts * per_attempt_seconds;
      double d = t * cost_per_second_usd;
      std::cout << "      " << std::left << std::setw(32) << c.name
        << " : " << std::setw(14) << fmt_time(t)
        << "  " << fmt_dollars(d) << std::endl;
    }
    std::cout << std::endl;
  }
}

TEST(wallet_kdf_romix, correctness)
{
  const std::string pwd = "correct horse battery staple";
  const std::string salt_a(16, 'A');
  const std::string salt_b(16, 'B');
  const uint8_t N_log2 = 10; // 1024 * 32B = 32 KiB, fast enough for test

  uint8_t out1[32], out2[32], out3[32], out4[32], out5[32];

  // determinism
  crypto::derive_key_romix_keccak(CRYPTO_HDS_WALLET_KDF_ROMIX, pwd.data(), pwd.size(), salt_a.data(), salt_a.size(), N_log2, 0, out1);
  crypto::derive_key_romix_keccak(CRYPTO_HDS_WALLET_KDF_ROMIX, pwd.data(), pwd.size(), salt_a.data(), salt_a.size(), N_log2, 0, out2);
  ASSERT_EQ(0, std::memcmp(out1, out2, 32));

  // salt sensitivity
  crypto::derive_key_romix_keccak(CRYPTO_HDS_WALLET_KDF_ROMIX, pwd.data(), pwd.size(), salt_b.data(), salt_b.size(), N_log2, 0, out3);
  ASSERT_NE(0, std::memcmp(out1, out3, 32));

  // password sensitivity
  const std::string pwd2 = "correct horse battery stapleX";
  crypto::derive_key_romix_keccak(CRYPTO_HDS_WALLET_KDF_ROMIX, pwd2.data(), pwd2.size(), salt_a.data(), salt_a.size(), N_log2, 0, out4);
  ASSERT_NE(0, std::memcmp(out1, out4, 32));

  // N_log2 sensitivity (cost factor must be absorbed, not ignored)
  crypto::derive_key_romix_keccak(CRYPTO_HDS_WALLET_KDF_ROMIX, pwd.data(), pwd.size(), salt_a.data(), salt_a.size(), (uint8_t)(N_log2 + 1), 0, out5);
  ASSERT_NE(0, std::memcmp(out1, out5, 32));
}

TEST(wallet_kdf_romix, brute_force_cost_report)
{
  const std::string password = "hunter2hunter2hu";
  const std::string salt(WALLET_KDF_SALT_SIZE, 0x42);

  // rented CPU pricing assumption: AWS c7i, ~$2.86/h on 64 vCPU = ~$7.95e-4/core-sec.
  // we divide by 60 to keep the dollar projection on the conservative side.
  const double rented_cpu_core_usd_per_sec = 7.95e-4 / 60.0;
  // For plain keccak a GPU is ~1000x faster than one CPU core. For the memory-
  // hard path the GPU advantage collapses to VRAM/(N*32B), so per-stream speed
  // is ~CPU. Using these two factors gives a conservative "before vs after".
  const double gpu_speedup_old_kdf = 1000.0;
  const double gpu_speedup_new_kdf = 1.0;

  // OLD KDF: lots of reps to average out timer noise.
  uint8_t sink_old = 0;
  const int old_reps = 200000;
  const auto t0_old = std::chrono::steady_clock::now();
  for (int i = 0; i < old_reps; ++i)
  {
    uint8_t out[32];
    old_kdf_single_keccak(password, out);
    sink_old ^= out[0] ^ out[31];
  }
  const double old_per_attempt = seconds_since(t0_old) / old_reps;

  // NEW KDF: each call is already ~1s, 3 reps is enough.
  uint8_t sink_new = 0;
  const int new_reps = 3;
  const uint8_t N_log2 = WALLET_KDF_ROMIX_N_LOG2;
  const auto t0_new = std::chrono::steady_clock::now();
  for (int i = 0; i < new_reps; ++i)
  {
    uint8_t out[32];
    crypto::derive_key_romix_keccak(CRYPTO_HDS_WALLET_KDF_ROMIX, password.data(), password.size(), salt.data(), salt.size(), N_log2, WALLET_KDF_ROMIX_PHASE2_LOG2_REDUCTION, out);
    sink_new ^= out[0] ^ out[31];
  }
  const double new_per_attempt = seconds_since(t0_new) / new_reps;

  // keep the optimizer honest
  volatile uint8_t dont_optimize_away = (uint8_t)(sink_old ^ sink_new);
  (void)dont_optimize_away;

  const double slowdown = new_per_attempt / old_per_attempt;
  const double buffer_bytes = (double)((uint64_t)1 << N_log2) * 32.0;

  std::cout << std::endl;
  std::cout << "wallet KDF brute-force cost report" << std::endl;
  std::cout << "password: \"" << password << "\" (" << password.size() << " bytes)" << std::endl;
  std::cout << "ROMix-Keccak: N=2^" << (int)N_log2 << " ("  << fmt_count((double)((uint64_t)1 << N_log2)) << " blocks, " << (buffer_bytes / (1024.0 * 1024.0)) 
    << " MiB)" << ", salt=" << WALLET_KDF_SALT_SIZE << "B" << ", phase2 reduction=" << (int)WALLET_KDF_ROMIX_PHASE2_LOG2_REDUCTION << std::endl;
  std::cout << std::endl;

  std::cout << "per-attempt cost on this machine:" << std::endl;
  std::cout << "  old (single keccak)  : " << fmt_time(old_per_attempt) << std::endl;
  std::cout << "  new (ROMix-Keccak)   : " << fmt_time(new_per_attempt) << std::endl;
  std::cout << "  slowdown             : x" << std::fixed << std::setprecision(0) << slowdown << std::endl;
  std::cout << std::endl;

  std::cout << "before (old KDF, GPU farm @ ~$3e-5/GPU-sec, no salt -> rainbow tables OK):" << std::endl;
  print_bruteforce_projection("GPU farm", old_per_attempt / gpu_speedup_old_kdf, 3e-5);

  std::cout << "after (new KDF, per-wallet 128-bit salt, ~" << (int)(24.0 * 1024.0 * 1024.0 * 1024.0 / buffer_bytes) << " parallel streams on a 24GB GPU):" << std::endl;
  print_bruteforce_projection("rented CPU (AWS c7i)", new_per_attempt / gpu_speedup_new_kdf, rented_cpu_core_usd_per_sec);

  // sanity floor: a real 32 MiB ROMix on any sane CPU is at least ~50 ms anything faster means the loop got optimized away or something is off
  ASSERT_GT(new_per_attempt, 0.05) << "new KDF is suspiciously fast";
  ASSERT_GT(slowdown, 1000.0) << "new KDF must be at least 1000x slower than a single keccak";
}
