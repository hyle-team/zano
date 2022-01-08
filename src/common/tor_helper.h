// Copyright (c) 2014-2018 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "tor-connect/torlib/tor_wrapper.h"



namespace tools
{
  typedef epee::levin::levin_client_impl2<tools::tor::tor_transport> levin_over_tor_client;
  
}


