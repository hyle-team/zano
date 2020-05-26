// Copyright (c) 2014-2020 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Copyright (c) 2012-2013 The Boolberry developers
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

namespace std
{

  // this allows using std::pair<> as a key in unordered std containers
  template <class T1, class T2>
  struct hash<pair<T1, T2>>
  {
    size_t operator()(const pair<T1, T2>& p) const
    {
      auto hash1 = hash<T1>{}(p.first);
      auto hash2 = hash<T2>{}(p.second);
      return hash1 ^ hash2;
    }
  };

} // namespace std

