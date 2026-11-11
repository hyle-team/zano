// Copyright (c) 2014-2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "boost_serialization_helper.h"
#include "serialization/serialization.h"

#define BOOST_CHAIN_TO_CRYPTO_SERIALIZATION()     { \
          std::string blob; \
          if constexpr (t_archive::is_saving::value) \
          { \
            std::string blob = ::t_serializable_object_to_blob(*this); \
            _arch& blob; \
          } \
          else if constexpr (t_archive::is_loading::value) \
          { \
            std::string blob; \
            _arch& blob; \
            bool r = ::t_unserializable_object_from_blob(*this, blob); \
            if (!r) \
              throw std::runtime_error("BOOST_SERIALIZATION: transaction::serialization failed"); \
          } \
        }
