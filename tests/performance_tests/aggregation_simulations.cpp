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
const mp::uint256_t c_scalar_L("7237005577332262213973186563042994252870990258390"); // L is the order of the main subgroup

static thread_local std::mt19937_64 rng(std::random_device{}());
static thread_local std::uniform_int_distribution<uint64_t> dist_uint64;

// Uniform distribution over 256-bit integers
boost::random::uniform_int_distribution<mp::uint256_t> dist_256(0, c_scalar_L - 1);




bool is_utxo_eligibile(uint64_t amount, mp::uint256_t random, mp::uint128_t D, uint64_t epoch)
{
  //////////////

  const mp::uint128_t pos_difficulty = D;
  const mp::uint256_t c_zarcanum_z_coeff_mp = mp::pow(mp::uint256_t(2), 64); // 2^64
  
  mp::uint256_t z_l_div_z_D = c_zarcanum_z_coeff_mp * (c_scalar_L / (c_zarcanum_z_coeff_mp * pos_difficulty)); // == z * floor( l / (z * D) )
  mp::uint256_t lhs = random; // use random here, originally it was scalar_t lhs_s = scalar_t(kernel_hash) * (blinding_mask + secret_q + last_pow_block_id_hashed); // == h * (f + q + f') mod l
  mp::uint512_t rhs = static_cast<mp::uint512_t>(z_l_div_z_D) * amount; // == floor( l / (z * D) ) * z * a
  return lhs < rhs;  //  h * (f + q + f') mod l   <   floor( l / (z * D) ) * z * a
  /////////////
  //return false;
}

static const uint64_t COIN = 1000000000000;

struct party
{
  uint64_t stake_amount = 100000 * COIN;
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
  parties.push_back(party{ 5000000 * COIN, 0 });

  //100 stakers with 50000 coins each
  for (i = 0; i != 100; i++)
  {
    parties.push_back(party{ 50000 * COIN, 0 });
  }


  for (i = 0; i != 432000; i++)
  {
    for (auto p : parties)
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

int run_simulations() {
  mp::uint128_t D = COIN;
  D *= 200000000;
  for (size_t i = 0; i != 100; i++)
  {
    do_simulation_for_D(D);
    //D *= 10;
  }

  //do_simulation_for_D(D);
  return true;
}
