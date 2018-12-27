// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstddef>
#include <cstring>
#include <functional>

#define POD_MAKE_COMPARABLE(space, type) \
namespace space { \
  inline bool operator==(const type &_v1, const type &_v2) { \
    return std::memcmp(&_v1, &_v2, sizeof(type)) == 0; \
  } \
  inline bool operator!=(const type &_v1, const type &_v2) { \
    return std::memcmp(&_v1, &_v2, sizeof(type)) != 0; \
  } \
}

#define POD_MAKE_LESS_OPERATOR(space, type) \
namespace space { \
  inline bool operator<(const type &_v1, const type &_v2) { \
    return std::memcmp(&_v1, &_v2, sizeof(type)) < 0; \
  } \
}

#define POD_MAKE_HASHABLE(space, type) \
 POD_MAKE_COMPARABLE(space, type) \
namespace space { \
  static_assert(sizeof(std::size_t) <= sizeof(type), "Size of " #type " must be at least that of size_t"); \
  inline std::size_t hash_value(const type &_v) { \
    return reinterpret_cast<const std::size_t &>(_v); \
  } \
} \
namespace std { \
  template<> \
  struct hash<space::type> { \
    std::size_t operator()(const space::type &_v) const { \
      return reinterpret_cast<const std::size_t &>(_v); \
    } \
  }; \
}

//
// CONSTEXPR
//
#if ( defined(_MSC_VER) && (_MSC_VER < 1800) )
  #error MS compilers prior to v 18.00 (MSVC 2013) are not supported
#endif

// compilation workaround for MSVC 2013 Update 5 wich does not support constexpr
#if ( defined(_MSC_VER) && (1800 <= _MSC_VER) && (_MSC_VER < 1900) )
  #define CONSTEXPR
#else // all other platforms or MSVC 2015 and later
  #define CONSTEXPR constexpr
#endif

