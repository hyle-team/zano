// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "genesis_acc.h"


namespace currency
{
#ifndef TESTNET
  const std::string ggenesis_tx_pub_key_str = "88bad574f43d16719a44152089f88672b4ecd93333c922b3ebdfbd1655fa6403";
  const crypto::public_key ggenesis_tx_pub_key = epee::string_tools::parse_tpod_from_hex_string<crypto::public_key>(ggenesis_tx_pub_key_str);
  const genesis_tx_dictionary_entry ggenesis_dict[26] = {
    { 898363347618325980ULL,7 },
    { 1234271292339965434ULL,1 },
    { 2785329203593578547ULL,12 },
    { 2912579291078040461ULL,18 },
    { 3515932779881697835ULL,17 },
    { 4955366495399988463ULL,11 },
    { 5233257582118330150ULL,5 },
    { 5931539148443336682ULL,24 },
    { 6436517662239927298ULL,19 },
    { 6604452700210763953ULL,13 },
    { 7200550178847042641ULL,15 },
    { 8712326356392296687ULL,9 },
    { 8863158309745010598ULL,4 },
    { 9048445805125559105ULL,16 },
    { 9527474759752332295ULL,2 },
    { 9647541513390373765ULL,20 },
    { 9921730437908704447ULL,8 },
    { 10751885755236960099ULL,25 },
    { 11032572278436047420ULL,22 },
    { 11109691972771859220ULL,0 },
    { 13554174209305230569ULL,23 },
    { 14297297752337562678ULL,3 },
    { 15636081871140663679ULL,21 },
    { 15951161519112687845ULL,6 },
    { 17146058209502212345ULL,14 },
    { 17472133472787764818ULL,10 }
  };
#else 
  const std::string ggenesis_tx_pub_key_str    = "cc27108a5c2af3ba4893ccbd50fdd919187503bda7299b0dbbdbc8acd6028b36";
  const crypto::public_key ggenesis_tx_pub_key = epee::string_tools::parse_tpod_from_hex_string<crypto::public_key>(ggenesis_tx_pub_key_str);
  const genesis_tx_dictionary_entry ggenesis_dict[5] = {
    { 4413532107669521528ULL, 2 },
    { 4848259848862559835ULL, 4 },
    { 4891306118630423916ULL, 1 },
    { 6536034028979999929ULL, 0 },
    { 15528122346224653564ULL, 3 }
};
#endif 
  


}



