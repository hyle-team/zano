// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"

/************************************************************************/
/*                                                                      */
/************************************************************************/
class gen_simple_chain_split_1 : public test_chain_unit_base 
{
public: 
  gen_simple_chain_split_1();
  bool generate(std::vector<test_event_entry> &events) const; 
  bool check_split_not_switched(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events); 
  bool check_split_not_switched2(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events); 
  bool check_split_switched(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events); 
  bool check_split_not_switched_back(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events); 
  bool check_split_switched_back_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events); 
  bool check_split_switched_back_2(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events); 
  bool check_mempool_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events); 
  bool check_mempool_2(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events); 
  bool check_orphaned_chain_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events); 
  bool check_orphan_readded(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
  bool check_orphaned_switched_to_main(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
private:
};
