// Copyright (c) 2014-2018 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <atomic>

namespace boost
{
  namespace serialization
  {
    template <class Archive, class value_t>
    inline void save(Archive &a, const std::atomic<value_t> &x, const boost::serialization::version_type ver)
    {
      a << x.load();
    }

    template <class Archive, class value_t>
    inline void load(Archive &a, std::atomic<value_t> &x, const boost::serialization::version_type ver)
    {
      value_t s = AUTO_VAL_INIT(s);
      a >> s;
      x.store(s);
    }
    template <class Archive, class value_t>
    inline void serialize(Archive &a, std::atomic<value_t> &x, const boost::serialization::version_type ver)
    {
      split_free(a, x, ver);
    }
  }
}
