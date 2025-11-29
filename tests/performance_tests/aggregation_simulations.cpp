// Copyright (c) 2025 The Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


// Example program
#include <iostream>
#include <string>
#include <string>
#include <cctype>
#include <iostream>

#include <iostream>
#include <utility>

#include <iostream>
#include <shared_mutex>
#include <thread>

#include <iostream>
#include <string>
#include <random>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/random.hpp>
#include <random>

namespace mp = boost::multiprecision;
const mp::uint256_t c_scalar_L("7237005577332262213973186563042994240857116359379907606001950938285454250989"); // L is the order of the main subgroup
const mp::uint256_t c_zarcanum_z_coeff_mp = mp::pow(mp::uint256_t(2), 64); // 2^64

static thread_local std::mt19937_64 rng(std::random_device{}());
static thread_local std::uniform_int_distribution<uint64_t> dist_uint64;

// Uniform distribution over 256-bit integers
boost::random::uniform_int_distribution<mp::uint256_t> dist_256(0, c_scalar_L - 1);

void cn_fast_hash(const void *data, size_t length, char *hash);


bool is_utxo_eligibile(uint64_t amount, mp::uint256_t random, mp::uint128_t pos_difficulty, uint64_t epoch)
{
  if (random >= c_scalar_L)
    throw std::runtime_error("random value must be in [0; L)");
  //////////////
  mp::uint256_t z_l_div_z_D = c_zarcanum_z_coeff_mp * (c_scalar_L / (c_zarcanum_z_coeff_mp * pos_difficulty)); // == z * floor( l / (z * D) )
  mp::uint256_t lhs = random; // use random here, originally it was scalar_t lhs_s = scalar_t(kernel_hash) * (blinding_mask + secret_q + last_pow_block_id_hashed); // == h * (f + q + f') mod l
  mp::uint512_t rhs = static_cast<mp::uint512_t>(z_l_div_z_D) * amount; // == floor( l / (z * D) ) * z * a
  return lhs < rhs;  //  h * (f + q + f') mod l   <   floor( l / (z * D) ) * z * a
  /////////////
  //return false;
}

static const uint64_t COIN = 1'000'000'000'000;

struct party
{
  uint64_t stake_amount = 100'000 * COIN;
  uint64_t blocks_staked = 0;
};

uint64_t do_simulation_for_D(mp::uint128_t D)
{
  uint64_t amount = 0;
  uint64_t i = 0;
  size_t number_of_blocks = 0;
  std::vector<party> parties;
  //100 stakers 100000 coins each total 10 million coins staked
  //std::vector<party> parties(100);

  //staker with 5Milion coins and others with 5Milion split between each, total 10 million coins staked
  parties.push_back(party{ 5'000'000 * COIN, 0 });

  //100 stakers with 50000 coins each
  for (i = 0; i != 10000; i++)
  {
    parties.push_back(party{ 500 * COIN, 0 });
  }


  for (i = 0; i != 432000; i++)
  {
    for (auto& p : parties)
    {
      if (is_utxo_eligibile(p.stake_amount, dist_256(rng), D, i))
      {
        number_of_blocks++;
        p.blocks_staked++;
        //break;
      }
    }
  }

  std::cout << "For D " << D / COIN << " Found blocks total: " << number_of_blocks << ", Adversary blocks: " << parties[0].blocks_staked << " Adversary made percents: " << (static_cast<double>(parties[0].blocks_staked) / number_of_blocks * 100.0) << "%" << std::endl;
  return 0;

}

////// this block may be entirely skipped
#include "crypto/crypto-sugar.h"
bool check_constants()
{
  if (c_scalar_L != crypto::c_scalar_L.as_boost_mp_type<mp::uint256_t>())
    return false;
  if (c_zarcanum_z_coeff_mp != crypto::c_scalar_2p64.as_boost_mp_type<mp::uint256_t>())
    return false;
  return true;
}
////// end of block



int run_simulations()
{
  if (!check_constants())
  {
    std::cout << "ERROR: check_constants() failed" << std::endl;
    return 1;
  }


  mp::uint128_t D = COIN;
  D *= 200'000'000;
  for (size_t i = 0; i != 100; i++)
  {
    do_simulation_for_D(D);
    //D *= 10;
  }

  //do_simulation_for_D(D);
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// code from keccak.c
// 19-Nov-11  Markku-Juhani O. Saarinen <mjos@iki.fi>
// A baseline Keccak (3rd round) implementation.

#define HASH_SIZE  32
#define HASH_DATA_AREA 136
#pragma pack(push, 1)
  struct hash {
    char data[HASH_SIZE];
  };
#pragma pack(pop)

#define KECCAK_ROUNDS 24
#define ROTL64(x, y) (((x) << (y)) | ((x) >> (64 - (y))))

const uint64_t keccakf_rndc[24] = 
{
    0x0000000000000001, 0x0000000000008082, 0x800000000000808a,
    0x8000000080008000, 0x000000000000808b, 0x0000000080000001,
    0x8000000080008081, 0x8000000000008009, 0x000000000000008a,
    0x0000000000000088, 0x0000000080008009, 0x000000008000000a,
    0x000000008000808b, 0x800000000000008b, 0x8000000000008089,
    0x8000000000008003, 0x8000000000008002, 0x8000000000000080, 
    0x000000000000800a, 0x800000008000000a, 0x8000000080008081,
    0x8000000000008080, 0x0000000080000001, 0x8000000080008008
};

const int keccakf_rotc[24] = 
{
    1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14, 
    27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44
};

const int keccakf_piln[24] = 
{
    10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4, 
    15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1 
};

// update the state with given number of rounds

void keccakf(uint64_t st[25], int rounds)
{
    int i, j, round;
    uint64_t t, bc[5];

    for (round = 0; round < rounds; round++) {

        // Theta
        for (i = 0; i < 5; i++)     
            bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];

        for (i = 0; i < 5; i++) {
            t = bc[(i + 4) % 5] ^ ROTL64(bc[(i + 1) % 5], 1);
            for (j = 0; j < 25; j += 5)
                st[j + i] ^= t;
        }

        // Rho Pi
        t = st[1];
        for (i = 0; i < 24; i++) {
            j = keccakf_piln[i];
            bc[0] = st[j];
            st[j] = ROTL64(t, keccakf_rotc[i]);
            t = bc[0];
        }

        //  Chi
        for (j = 0; j < 25; j += 5) {
            for (i = 0; i < 5; i++)
                bc[i] = st[j + i];
            for (i = 0; i < 5; i++)
                st[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }

        //  Iota
        st[0] ^= keccakf_rndc[round];
    }
}

// compute a keccak hash (md) of given byte length from "in"
typedef uint64_t state_t[25];

int keccak(const uint8_t *in, int inlen, uint8_t *md, int mdlen)
{
    state_t st;
    uint8_t temp[144];
    int i, rsiz, rsizw;

    rsiz = sizeof(state_t) == mdlen ? HASH_DATA_AREA : 200 - 2 * mdlen;
    rsizw = rsiz / 8;
    
    memset(st, 0, sizeof(st));

    for ( ; inlen >= rsiz; inlen -= rsiz, in += rsiz) {
        for (i = 0; i < rsizw; i++)
            st[i] ^= ((uint64_t *) in)[i];
        keccakf(st, KECCAK_ROUNDS);
    }
    
    // last block and padding
    memcpy(temp, in, inlen);
    temp[inlen++] = 1;
    memset(temp + inlen, 0, rsiz - inlen);
    temp[rsiz - 1] |= 0x80;

    for (i = 0; i < rsizw; i++)
        st[i] ^= ((uint64_t *) temp)[i];

    keccakf(st, KECCAK_ROUNDS);

    memcpy(md, st, mdlen);

    return 0;
}

void cn_fast_hash(const void *data, size_t length, char *hash)
{
  keccak((const uint8_t*)data, (int)length, (uint8_t*)hash, HASH_SIZE);
}
