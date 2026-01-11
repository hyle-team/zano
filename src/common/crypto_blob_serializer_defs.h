// Copyright (c) 2014-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once


#include "crypto/chacha.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "crypto/crypto-sugar.h"
#include "crypto/eth_signature.h"
#include "serialization/serialization_base_traits.h"



BLOB_SERIALIZER(crypto::chacha_iv);
BLOB_SERIALIZER(crypto::hash);
//BLOB_SERIALIZER(crypto::public_key);
BLOB_SERIALIZER(crypto::secret_key);
BLOB_SERIALIZER(crypto::key_derivation);
BLOB_SERIALIZER(crypto::key_image);
BLOB_SERIALIZER(crypto::signature);
BLOB_SERIALIZER(crypto::scalar_t);
BLOB_SERIALIZER(crypto::point_t);
BLOB_SERIALIZER(crypto::eth_public_key);
BLOB_SERIALIZER(crypto::eth_signature);
