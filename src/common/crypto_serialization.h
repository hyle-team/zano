// Copyright (c) 2014-2022 Zano Project
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

#include "serialization/serialization.h"
#include "serialization/debug_archive.h"
#include "crypto/chacha8.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "crypto/range_proofs.h"
#include "boost_serialization_maps.h"

//
// binary serialization
//

namespace crypto
{
  struct bpp_signature_serialized : public crypto::bpp_signature
  {
    BEGIN_SERIALIZE_OBJECT()
      FIELD(L)
      FIELD(R)
      FIELD(A0)
      FIELD(A)
      FIELD(B)
      FIELD(r)
      FIELD(s)
      FIELD(delta)
    END_SERIALIZE()

    BEGIN_BOOST_SERIALIZATION()
      BOOST_SERIALIZE(L)
      BOOST_SERIALIZE(R)
      BOOST_SERIALIZE(A0)
      BOOST_SERIALIZE(A)
      BOOST_SERIALIZE(B)
      BOOST_SERIALIZE(r)
      BOOST_SERIALIZE(s)
      BOOST_SERIALIZE(delta)
    END_BOOST_SERIALIZATION()
  };
}

BLOB_SERIALIZER(crypto::chacha8_iv);
BLOB_SERIALIZER(crypto::hash);
BLOB_SERIALIZER(crypto::public_key);
BLOB_SERIALIZER(crypto::secret_key);
BLOB_SERIALIZER(crypto::key_derivation);
BLOB_SERIALIZER(crypto::key_image);
BLOB_SERIALIZER(crypto::signature);
VARIANT_TAG(debug_archive, crypto::hash, "hash");
VARIANT_TAG(debug_archive, crypto::public_key, "public_key");
VARIANT_TAG(debug_archive, crypto::secret_key, "secret_key");
VARIANT_TAG(debug_archive, crypto::key_derivation, "key_derivation");
VARIANT_TAG(debug_archive, crypto::key_image, "key_image");
VARIANT_TAG(debug_archive, crypto::signature, "signature");


//
// Boost serialization
//

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
  } // namespace serialization
} // namespace boost
