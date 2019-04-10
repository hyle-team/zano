// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "genesis_acc.h"


namespace currency
{
  const std::string ggenesis_tx_pub_key_str = "a68aabfbd365bd8e299e917591c6b8014853fd6237fee41d068d2975e37dbae4";
  const crypto::public_key ggenesis_tx_pub_key = epee::string_tools::parse_tpod_from_hex_string<crypto::public_key>(ggenesis_tx_pub_key_str);
  const genesis_tx_dictionary_entry ggenesis_dict[14] = {
    { 898363347618325980ULL,7 },
    { 1234271292339965434ULL,1 },
    { 2785329203593578547ULL,12 },
    { 4955366495399988463ULL,11 },
    { 5233257582118330150ULL,5 },
    { 6604452700210763953ULL,13 },
    { 8712326356392296687ULL,9 },
    { 8863158309745010598ULL,4 },
    { 9527474759752332295ULL,2 },
    { 9921730437908704447ULL,8 },
    { 11109691972771859220ULL,0 },
    { 14297297752337562678ULL,3 },
    { 15951161519112687845ULL,6 },
    { 17472133472787764818ULL,10 }
  };

}



