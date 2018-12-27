// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/map.hpp>
#include <boost/foreach.hpp>
#include <boost/serialization/is_bitwise_serializable.hpp>
#include "crypto/crypto.h"

namespace boost
{
  namespace serialization
  {
    //---------------------------------------------------
    template <class Archive>
    inline void serialize(Archive &a, crypto::public_key &x, const boost::serialization::version_type ver)
    {
      a & reinterpret_cast<char (&)[sizeof(crypto::public_key)]>(x);
    }
    template <class Archive>
    inline void serialize(Archive &a, crypto::secret_key &x, const boost::serialization::version_type ver)
    {
      a & reinterpret_cast<char (&)[sizeof(crypto::secret_key)]>(x);
    }
    template <class Archive>
    inline void serialize(Archive &a, crypto::key_derivation &x, const boost::serialization::version_type ver)
    {
      a & reinterpret_cast<char (&)[sizeof(crypto::key_derivation)]>(x);
    }
    template <class Archive>
    inline void serialize(Archive &a, crypto::key_image &x, const boost::serialization::version_type ver)
    {
      a & reinterpret_cast<char (&)[sizeof(crypto::key_image)]>(x);
    }

    template <class Archive>
    inline void serialize(Archive &a, crypto::signature &x, const boost::serialization::version_type ver)
    {
      a & reinterpret_cast<char (&)[sizeof(crypto::signature)]>(x);
    }
    template <class Archive>
    inline void serialize(Archive &a, crypto::hash &x, const boost::serialization::version_type ver)
    {
      a & reinterpret_cast<char (&)[sizeof(crypto::hash)]>(x);
    }    
  }
}

//}
