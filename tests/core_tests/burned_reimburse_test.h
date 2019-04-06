// Copyright (c) 2019 Zano Project
// Copyright (c) 2019 Hyle Team

// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"

struct bunred_coins_reimburse_test : public test_chain_unit_base
{
  bunred_coins_reimburse_test();
  bool generate(std::vector<test_event_entry>& events) const;
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_reimbured(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
};
