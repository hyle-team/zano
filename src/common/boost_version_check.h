// Copyright (c) 2025 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once


#include <boost/version.hpp>
#include <boost/config.hpp> // for BOOST_LIB_VERSION

#if BOOST_VERSION < 107500
  #  error "Boost version 1.75.0 or newer is required, detected: " BOOST_LIB_VERSION
#endif