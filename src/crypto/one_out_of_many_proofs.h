// Copyright (c) 2023 Zano Project
// Copyright (c) 2023 sowle (val@zano.org, crypto.sowle@gmail.com)
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#pragma once
#include "crypto-sugar.h"

namespace crypto
{
  //
  // BGE stands for Bootle, Groth, Esgin
  //
  // This is a proof-of-concept implementation of a log-size one-out-of-many proof based on ideas and approaches by Bootle et al, Groth et al and Esgin et al
  //
  // https://eprint.iacr.org/2014/764
  // https://eprint.iacr.org/2015/643
  // https://eprint.iacr.org/2019/1287
  // 
  // Disclaimer: shouldn't be used in production code until the security proofs and the code are peer-reviewed.
  //

  struct BGE_proof
  {
    point_t A;
    point_t B;
    std::vector<point_t> Pk;
    scalar_vec_t f;
    scalar_t y;
    scalar_t z;
  };

  bool generate_BGE_proof(const hash& m, const std::vector<point_t>& ring, const scalar_t& secret, const size_t secret_index, BGE_proof& result, uint8_t* p_err = nullptr);


  bool verify_BGE_proof(const hash& m, const std::vector<const public_key*>& ring, BGE_proof& result, uint8_t* p_err = nullptr);


} // namespace crypto
