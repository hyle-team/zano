// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"

/************************************************************************/
/*                                                                      */
/************************************************************************/
class prun_ring_signatures: public test_chain_unit_base
{
public:
  prun_ring_signatures();

  bool generate(std::vector<test_event_entry>& events) const;

  bool set_check_points(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_blockchain(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool generate_blockchain_with_pruned_rs(std::vector<test_event_entry>& events) const;
private:
};

