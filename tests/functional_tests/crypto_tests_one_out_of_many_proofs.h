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



TEST(BGE_proof, positive)
{
  BGE_proff_check_t cc;
  
  for(size_t N = 1; N <= 256; ++N)
  {
    cc.prepare_random_data(N);
    ASSERT_TRUE(cc.generate());
    std::cout << "N = " << N << ", size = " << (cc.sig.Pk.size() + cc.sig.f.size() + 4)  * 32 << " bytes" << ENDL;
    ASSERT_TRUE(cc.verify());
  }

  return true;
}

bool invalidate_BGE_proof(size_t index, BGE_proof& s)
{
  static point_t te{};
  static bool te_init = te.from_string(canonical_torsion_elements[0].string);
  if (!te_init)
    throw std::runtime_error("te_init");

  switch(index)
  {
  case 0:  s.A = (point_t(s.A) + te).to_public_key(); return true;
  case 1:  s.B = (point_t(s.B) + te).to_public_key(); return true;
  case 2:  s.Pk[0] = (point_t(s.Pk[0]) + te).to_public_key(); return true;
  case 3:  s.Pk.back() = (point_t(s.Pk.back()) + te).to_public_key(); return true;
  case 4:  s.f[0] = 0; return true;
  case 5:  s.f[0] = c_scalar_256m1; return true;
  case 6:  s.f.back() = 0; return true;
  case 7:  s.f.back() = c_scalar_256m1; return true;
  case 8:  s.y = 0; return true;
  case 9:  s.y = c_scalar_256m1; return true;
  case 10: s.z = 0; return true;
  case 11: s.z = c_scalar_256m1; return true;
  default:
    return false;
  }
}

TEST(BGE_proof, negative)
{
  BGE_proff_check_t cc;

  for(size_t N = 1; N <= 13; ++N)
  {
    cc.prepare_random_data(13);
    ASSERT_TRUE(cc.generate());
    BGE_proff_check_t cc_good = cc;
    ASSERT_TRUE(cc_good.verify());

    for(size_t i = 0; true; ++i)
    {
      cc = cc_good;
      if (!invalidate_BGE_proof(i, cc.sig))
      {
        ASSERT_TRUE(i != 0);
        break;
      }
      ASSERT_FALSE(cc.verify());
    }
  }

  return true;
}
