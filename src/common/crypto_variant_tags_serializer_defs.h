// Copyright (c) 2014-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once


#include "crypto/chacha.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "crypto/crypto-sugar.h"
#include "crypto/eth_signature.h"
#include "serialization/serialization_basic_traits.h"
#include "serialization/debug_archive.h"


VARIANT_TAG(debug_archive, crypto::hash, "hash");
VARIANT_TAG(debug_archive, crypto::public_key, "public_key");
VARIANT_TAG(debug_archive, crypto::secret_key, "secret_key");
VARIANT_TAG(debug_archive, crypto::key_derivation, "key_derivation");
VARIANT_TAG(debug_archive, crypto::key_image, "key_image");
VARIANT_TAG(debug_archive, crypto::signature, "signature");
VARIANT_TAG(debug_archive, crypto::eth_public_key, "eth_public_key");
VARIANT_TAG(debug_archive, crypto::eth_signature, "eth_signature");
