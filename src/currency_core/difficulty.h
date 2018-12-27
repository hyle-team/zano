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
    typedef std::uint64_t difficulty_type;
    typedef boost::multiprecision::uint128_t wide_difficulty_type;

    bool check_hash_old(const crypto::hash &hash, difficulty_type difficulty);
    difficulty_type next_difficulty_old(std::vector<std::uint64_t> timestamps, std::vector<difficulty_type> cumulative_difficulties);
    difficulty_type next_difficulty_old(std::vector<std::uint64_t> timestamps, std::vector<difficulty_type> cumulative_difficulties, size_t target_seconds);

    bool check_hash(const crypto::hash &hash, wide_difficulty_type difficulty);
    wide_difficulty_type next_difficulty(std::vector<std::uint64_t> timestamps, std::vector<wide_difficulty_type> cumulative_difficulties, size_t target_seconds);
    uint64_t difficulty_to_boundary(wide_difficulty_type difficulty);
    void difficulty_to_boundary_long(wide_difficulty_type difficulty, crypto::hash& result);
}
