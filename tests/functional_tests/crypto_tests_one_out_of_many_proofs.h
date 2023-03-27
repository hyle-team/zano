// Copyright (c) 2023 Zano Project (https://zano.org/)
// Copyright (c) 2023 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include <crypto/one_out_of_many_proofs.h>


struct BGE_proff_check_t
{
  hash context_hash;
  std::vector<public_key> ring; // 1/8
  std::vector<point_t> ring_pt;
  size_t secret_index;
  scalar_t secret;

  BGE_proof sig;

  BGE_proff_check_t& operator=(const BGE_proff_check_t&) = default;
 
  void prepare_random_data(size_t ring_size)
  {
    context_hash = *(hash*)scalar_t::random().data();

    secret.make_random();
    secret_index = random_in_range(0, ring_size - 1);

    ring_pt.resize(ring_size);
    ring.resize(ring_size);
    for(size_t i = 0; i < ring_size; ++i)
    {
      ring_pt[i] = (i == secret_index) ? (secret * c_point_X) : hash_helper_t::hp(hash_helper_t::hs("0123456789012345678901234567890", scalar_t(i)));
      ring[i] = (c_scalar_1div8 * ring_pt[i]).to_public_key();
    }
  }

  bool generate() noexcept
  {
    try
    {
      uint8_t err = 0;
      return generate_BGE_proof(context_hash, ring_pt, secret, secret_index, sig, &err) && err == 0;
    }
    catch(...)
    {
      return false;
    }
  }

  bool verify() noexcept
  {
    try
    {
      uint8_t err = 0;
      std::vector<const public_key*> ring_ptr;
      for(auto& el : ring)
        ring_ptr.emplace_back(&el);
      return verify_BGE_proof(context_hash, ring_ptr, sig, &err) && err == 0;
    }
    catch(...)
    {
      return false;
    }
  }
};



TEST(BGE_proof, basics)
{
  BGE_proff_check_t cc;
  cc.prepare_random_data(1);
  ASSERT_TRUE(cc.generate());
  ASSERT_TRUE(cc.verify());


  return true;
}
