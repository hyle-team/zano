// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2014-2015 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "currency_format_utils.h"
#include "etc_custom_serialization.h"

namespace bc_services
{
  std::string transform_amount_to_string(const uint64_t& a)
  {
//     double d = static_cast<double>(a);
//     d /= ETC_AMOUNT_DIVIDER;
    return std::to_string(a);//print_money(a, ETC_AMOUNT_DIVIDER_DECIMAL_POINT);
  }

  uint64_t transform_string_to_amount(const std::string& d)
  {
    uint64_t n = 0;
    epee::string_tools::get_xtype_from_string(n, d);
    return n;
  }
}