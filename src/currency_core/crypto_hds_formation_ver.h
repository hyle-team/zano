// Copyright (c) 2022-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#pragma once
#include "currency_config.h"

#define CRYPTO_HDS_STRINGIFY_(x) #x
#define CRYPTO_HDS_STRINGIFY(x) CRYPTO_HDS_STRINGIFY_(x)

// formation version as a string, padded with '_' to exactly 4 chars
#if CURRENCY_FORMATION_VERSION < 10
#  define CRYPTO_HDS_FORMATION_VER_PAD "___"
#elif CURRENCY_FORMATION_VERSION < 100
#  define CRYPTO_HDS_FORMATION_VER_PAD "__"
#elif CURRENCY_FORMATION_VERSION < 1000
#  define CRYPTO_HDS_FORMATION_VER_PAD "_"
#elif CURRENCY_FORMATION_VERSION < 10000
#  define CRYPTO_HDS_FORMATION_VER_PAD ""
#else
#  error CURRENCY_FORMATION_VERSION does not fit into 4 chars
#endif

#define CRYPTO_HDS_FORMATION_VER_4 CRYPTO_HDS_STRINGIFY(CURRENCY_FORMATION_VERSION) CRYPTO_HDS_FORMATION_VER_PAD
