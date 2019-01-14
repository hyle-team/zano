// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "genesis_acc.h"


namespace currency
{
  const std::string ggenesis_tx_pub_key_str = "698b7bd6fc1bea739657b9fa8441a5a7a4cae69cb7efa2defc60e4c6609009f4";
  const crypto::public_key ggenesis_tx_pub_key = epee::string_tools::parse_tpod_from_hex_string<crypto::public_key>(ggenesis_tx_pub_key_str);
  
  const genesis_tx_dictionary_entry ggenesis_dict[10] = {
    { 690562910953636312ULL,5 },
    { 940036618224342135ULL,3 },
    { 1343336992873709805ULL,9 },
    { 2417587344126263675ULL,7 },
    { 2976852013260878279ULL,8 },
    { 9662281438621735689ULL,4 },
    { 14708441932961059072ULL,6 },
    { 15678994962012632719ULL,1 },
    { 17191212896741366274ULL,2 },
    { 18291644074790683797ULL,0 }
  };

}



