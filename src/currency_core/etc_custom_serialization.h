// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2014-2015 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#define  ETC_AMOUNT_DIVIDER 100000000
#define  ETC_AMOUNT_DIVIDER_DECIMAL_POINT 8

namespace bc_services
{
  std::string transform_amount_to_string(const uint64_t& a);
  uint64_t transform_string_to_amount(const std::string& a);
  
}
