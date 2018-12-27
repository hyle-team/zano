// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "genesis_acc.h"


namespace currency
{
  const std::string ggenesis_tx_pub_key_str = "024a9c8d295e53d18daccba4f840bad45926b1c1510ee279700f831bfd51cc63";
  const crypto::public_key ggenesis_tx_pub_key = epee::string_tools::parse_tpod_from_hex_string<crypto::public_key>(ggenesis_tx_pub_key_str);

  const genesis_tx_dictionary_entry ggenesis_dict[5] = {
    { 785828676124507135ULL,1 },
    { 1231353406545874119ULL,4 },
    { 2713905913089194270ULL,3 },
    { 3738391252528226312ULL,0 },
    { 11261277230714331250ULL,2 }
  };
}



