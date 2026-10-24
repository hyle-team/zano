// Copyright (c) 2014-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include "crypto/crypto.h"
//
// Wallet Internal Events
//
struct wie_construct_tx_on_onetime_key_created
{
  currency::keypair& onetime_keypair;
};

