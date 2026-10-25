// Copyright (c) 2014-2024 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 


#define BOOST_HF_MINIMAL_DISTANCE_BETWEEN_BOOST_AND_HF 1000  // minimal distance between boost finction and actual hardfork activation height

/////// Hard forks setup //////////////////////////////
#ifndef TESTNET
// Mainnet
#  define ZANO_HARDFORK_08_AFTER_HEIGHT 9999999
#  define ZANO_HARDFORK_08_MIN_BUILD_VER 999
#  define ZANO_HARDFORK_08_MAINTAINERS_PUBLIC_KEY "2343242342342342342342342342342342342342342342342342342342342343"

#else
// Testnet
#  define ZANO_HARDFORK_08_AFTER_HEIGHT 9999999
#  define ZANO_HARDFORK_08_MIN_BUILD_VER 999
#  define ZANO_HARDFORK_08_MAINTAINERS_PUBLIC_KEY "2343242342342342342342342342342342342342342342342342342342342343"
#endif
