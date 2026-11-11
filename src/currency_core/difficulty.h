// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstdint>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "crypto/hash.h"

namespace currency
{
      
    typedef boost::multiprecision::uint128_t wide_difficulty_type;

    bool check_hash(const crypto::hash &hash, wide_difficulty_type difficulty);
    wide_difficulty_type next_difficulty_1(std::vector<std::uint64_t>& timestamps, std::vector<wide_difficulty_type>& cumulative_difficulties, size_t target_seconds, const wide_difficulty_type& difficulty_starter);
    wide_difficulty_type next_difficulty_2(std::vector<std::uint64_t>& timestamps, std::vector<wide_difficulty_type>& cumulative_difficulties, size_t target_seconds, const wide_difficulty_type& difficulty_starter);
    uint64_t difficulty_to_boundary(wide_difficulty_type difficulty);
    void difficulty_to_boundary_long(wide_difficulty_type difficulty, crypto::hash& result);
}
