// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "misc_log_ex.h"

#include "common/int-util.h"
#include "crypto/hash.h"
#include "currency_core/currency_config.h"
#include "difficulty.h"
#include "profile_tools.h"

namespace currency {

  using std::size_t;
  using std::uint64_t;
  using std::vector;

//#if defined(_MSC_VER)
//#include <windows.h>
//#include <winnt.h>

  static inline void mul(uint64_t a, uint64_t b, uint64_t &low, uint64_t &high) {
    boost::multiprecision::uint128_t res = boost::multiprecision::uint128_t(a) * b;
    low = (res & 0xffffffffffffffffLL).convert_to<uint64_t>();
    high = (res >> 64).convert_to<uint64_t>();
    //low = _umul128(a, b, &high);
    //low = UnsignedMultiply128(a, b, &high);
  }

/* #else

  static inline void mul(uint64_t a, uint64_t b, uint64_t &low, uint64_t &high) {
    typedef unsigned __int128 uint128_t;
    uint128_t res = (uint128_t)a * (uint128_t)b;
    low = (uint64_t)res;
    high = (uint64_t)(res >> 64);
  }

#endif */

  static inline bool cadd(uint64_t a, uint64_t b) {
    return a + b < a;
  }

  static inline bool cadc(uint64_t a, uint64_t b, bool c) {
    return a + b < a || (c && a + b == (uint64_t)-1);
  }

 
#if defined(_MSC_VER)
#ifdef max
#undef max
#endif
#endif

  const wide_difficulty_type max64bit(std::numeric_limits<std::uint64_t>::max());
  const boost::multiprecision::uint256_t max128bit(std::numeric_limits<boost::multiprecision::uint128_t>::max());
  const boost::multiprecision::uint512_t max256bit(std::numeric_limits<boost::multiprecision::uint256_t>::max());

  bool check_hash(const crypto::hash &hash_, wide_difficulty_type difficulty)
  {
    //revert byte order
    crypto::hash h = {};
    for (size_t i = 0; i != sizeof(h); i++)
    {
      *(((char*)&h) + (sizeof(h) - (i + 1))) = *(((char*)&hash_) + i);
    }

    PROFILE_FUNC("check_hash");
    if (difficulty < max64bit)
    { // if can convert to small difficulty - do it
      std::uint64_t dl = difficulty.convert_to<std::uint64_t>();
      uint64_t low, high, top, cur;
      // First check the highest word, this will most likely fail for a random hash.
      mul(swap64le(((const uint64_t *)&h)[3]), dl, top, high);
      if (high != 0)
        return false;
      mul(swap64le(((const uint64_t *)&h)[0]), dl, low, cur);
      mul(swap64le(((const uint64_t *)&h)[1]), dl, low, high);
      bool carry = cadd(cur, low);
      cur = high;
      mul(swap64le(((const uint64_t *)&h)[2]), dl, low, high);
      carry = cadc(cur, low, carry);
      carry = cadc(high, top, carry);
      return !carry;
    }

    //std::cout << "   hash_      : " << hash_.data << std::endl;
    //std::cout << "   h          : " << h.data << std::endl;
    std::cout << "ch difficulty : " << difficulty << std::endl;
    std::cout << "ch fast check : " << ((const uint64_t *)&h)[3] << std::endl;
    std::cout << "ch max256bit  : " << max256bit << std::endl;
    
    // fast check
    if (((const uint64_t *)&h)[3] > 0)
      return false;
    // usual slow check
    boost::multiprecision::uint512_t hashVal = 0;
    for(int i = 0; i < 4; i++) 
    {
      hashVal <<= 64;
      hashVal |= swap64le(((const uint64_t *) &h)[3-i]);
    }
    return (hashVal * difficulty <= max256bit);
  }

  uint64_t difficulty_to_boundary(wide_difficulty_type difficulty)
  {
    boost::multiprecision::uint256_t nominal_hash = std::numeric_limits<boost::multiprecision::uint256_t>::max();
    nominal_hash = nominal_hash / difficulty;
    uint64_t res = (nominal_hash >> 192).convert_to<std::uint64_t>();
    return res;
  }

  void difficulty_to_boundary_long(wide_difficulty_type difficulty, crypto::hash& result)
  {
    boost::multiprecision::uint256_t nominal_hash = std::numeric_limits<boost::multiprecision::uint256_t>::max();
    nominal_hash = nominal_hash / difficulty;

    static_assert(sizeof(uint64_t) * 4 == sizeof(result), "!");
    for (size_t i = 0; i < 4; ++i)
    {
      (reinterpret_cast<uint64_t*>(&result))[i] = nominal_hash.convert_to<uint64_t>();
      nominal_hash >>= 64;
    }
  }

  void get_cut_location_from_len(size_t length, size_t& cut_begin, size_t& cut_end, size_t REDEF_DIFFICULTY_WINDOW, size_t REDEF_DIFFICULTY_CUT_OLD, size_t REDEF_DIFFICULTY_CUT_LAST)
  {
    if (length <= REDEF_DIFFICULTY_WINDOW)
    {
      cut_begin = 0;
      cut_end = length;
    }
    else
    {
      cut_begin = REDEF_DIFFICULTY_WINDOW - REDEF_DIFFICULTY_CUT_LAST + 1;
      cut_end = cut_begin + (REDEF_DIFFICULTY_WINDOW - (REDEF_DIFFICULTY_CUT_OLD + REDEF_DIFFICULTY_CUT_LAST));
    }
  }

  void get_adjustment_zone(size_t length, size_t& cut_begin, size_t& cut_end, size_t REDEF_DIFFICULTY_WINDOW, size_t REDEF_DIFFICULTY_CUT_OLD, size_t REDEF_DIFFICULTY_CUT_LAST)
  {
    //cutoff DIFFICULTY_LAG
    if (length <= REDEF_DIFFICULTY_WINDOW - (REDEF_DIFFICULTY_CUT_OLD + REDEF_DIFFICULTY_CUT_LAST))
    {
      cut_begin = 0;
      cut_end = length;
    }
    else
    {
      cut_begin = REDEF_DIFFICULTY_CUT_LAST;
      cut_end = cut_begin + (REDEF_DIFFICULTY_WINDOW - (REDEF_DIFFICULTY_CUT_OLD + REDEF_DIFFICULTY_CUT_LAST));
      if (cut_end > length)
        cut_end = length;
      
    }
    CHECK_AND_ASSERT_THROW_MES(/*cut_begin >= 0 &&*/ cut_begin + 2 <= cut_end && cut_end <= length, "validation in next_difficulty is failed");
  }

  wide_difficulty_type get_adjustment_for_zone(vector<uint64_t>& timestamps_sorted, vector<wide_difficulty_type>& cumulative_difficulties, size_t target_seconds, size_t REDEF_DIFFICULTY_WINDOW, size_t REDEF_DIFFICULTY_CUT_OLD, size_t REDEF_DIFFICULTY_CUT_LAST)
  {
    size_t length = timestamps_sorted.size();
    size_t cut_begin = 0;
    size_t cut_end = 0;
    get_adjustment_zone(length, cut_begin, cut_end, REDEF_DIFFICULTY_WINDOW, REDEF_DIFFICULTY_CUT_OLD, REDEF_DIFFICULTY_CUT_LAST);

    uint64_t time_span = timestamps_sorted[cut_begin] - timestamps_sorted[cut_end - 1];
    if (time_span == 0)
    {
      time_span = 1;
    }
    wide_difficulty_type total_work = cumulative_difficulties[cut_begin] - cumulative_difficulties[cut_end - 1];
    boost::multiprecision::uint256_t res = (boost::multiprecision::uint256_t(total_work) * target_seconds + time_span - 1) / time_span;
    if (res > max128bit)
      return 0; // to behave like previous implementation, may be better return max128bit?
    return res.convert_to<wide_difficulty_type>();
  }

  wide_difficulty_type next_difficulty_1(vector<uint64_t>& timestamps, vector<wide_difficulty_type>& cumulative_difficulties, size_t target_seconds)
  {

    // timestamps  - first is latest, back - is oldest timestamps
    if (timestamps.size() > DIFFICULTY_WINDOW)
    {
      timestamps.resize(DIFFICULTY_WINDOW);
      cumulative_difficulties.resize(DIFFICULTY_WINDOW);
    }


    size_t length = timestamps.size();
    CHECK_AND_ASSERT_MES(length == cumulative_difficulties.size(), 0, "Check \"length == cumulative_difficulties.size()\" failed");
    if (length <= 1)
    {
      return DIFFICULTY_STARTER;
    }
    static_assert(DIFFICULTY_WINDOW >= 2, "Window is too small");

    CHECK_AND_ASSERT_MES(length <= DIFFICULTY_WINDOW, 0, "length <= DIFFICULTY_WINDOW check failed, length=" << length);

    sort(timestamps.begin(), timestamps.end(), std::greater<uint64_t>());
    
    static_assert(2 * DIFFICULTY_CUT <= DIFFICULTY_WINDOW - 2, "Cut length is too large");
    wide_difficulty_type dif_slow = get_adjustment_for_zone(timestamps, cumulative_difficulties, target_seconds, DIFFICULTY_WINDOW, DIFFICULTY_CUT/2, DIFFICULTY_CUT/2);
    wide_difficulty_type dif_medium = get_adjustment_for_zone(timestamps, cumulative_difficulties, target_seconds, DIFFICULTY_WINDOW/3, DIFFICULTY_CUT / 8, DIFFICULTY_CUT / 12);
    wide_difficulty_type dif_fast = get_adjustment_for_zone(timestamps, cumulative_difficulties, target_seconds, DIFFICULTY_WINDOW/18, DIFFICULTY_CUT / 10, 2); 
    uint64_t devider = 1;
    wide_difficulty_type summ = dif_slow;
    if (dif_medium != 0)
    {
      summ += dif_medium;
      ++devider;
    }
    if (dif_fast != 0)
    {
      summ += dif_fast;
      ++devider;
    }
    return summ / devider;
  }

  wide_difficulty_type next_difficulty_2(vector<uint64_t>& timestamps, vector<wide_difficulty_type>& cumulative_difficulties, size_t target_seconds)
  {

    // timestamps  - first is latest, back - is oldest timestamps
    if (timestamps.size() > DIFFICULTY_WINDOW)
    {
      timestamps.resize(DIFFICULTY_WINDOW);
      cumulative_difficulties.resize(DIFFICULTY_WINDOW);
    }


    size_t length = timestamps.size();
    CHECK_AND_ASSERT_MES(length == cumulative_difficulties.size(), 0, "Check \"length == cumulative_difficulties.size()\" failed");
    if (length <= 1)
    {
      return DIFFICULTY_STARTER;
    }
    static_assert(DIFFICULTY_WINDOW >= 2, "Window is too small");

    CHECK_AND_ASSERT_MES(length <= DIFFICULTY_WINDOW, 0, "length <= DIFFICULTY_WINDOW check failed, length=" << length);

    sort(timestamps.begin(), timestamps.end(), std::greater<uint64_t>());

    static_assert(2 * DIFFICULTY_CUT <= DIFFICULTY_WINDOW - 2, "Cut length is too large");
    wide_difficulty_type dif_slow = get_adjustment_for_zone(timestamps, cumulative_difficulties, target_seconds, DIFFICULTY_WINDOW, DIFFICULTY_CUT / 2, DIFFICULTY_CUT / 2);
    wide_difficulty_type dif_medium = get_adjustment_for_zone(timestamps, cumulative_difficulties, target_seconds, DIFFICULTY_WINDOW / 3, DIFFICULTY_CUT / 8, DIFFICULTY_CUT / 12);
    uint64_t devider = 1;
    wide_difficulty_type summ = dif_slow;
    if (dif_medium != 0)
    {
      summ += dif_medium;
      ++devider;
    }
    return summ / devider;
  }
}
