// Copyright (c) 2012-2014 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include <cstdint>
#include <vector>

#include "currency_core/currency_format_utils.h"

TEST(alias_name_validation, alias_name_validation)
{
  std::string str("x");
  std::string all_chars = ALIAS_VALID_CHARS;
  char ch = 0;
  do{    
    bool valid_char = std::string::npos != all_chars.find(ch);
    str[0] = ch;
    ASSERT_EQ(valid_char, currency::validate_alias_name(str));

    ++(*reinterpret_cast<unsigned char*>(&ch));
  }while(*reinterpret_cast<unsigned char*>(&ch) != 0);
}