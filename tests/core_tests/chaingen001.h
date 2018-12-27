// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

class one_block : public test_chain_unit_base
{
  currency::account_base alice;
public:
  one_block();
  bool generate(std::vector<test_event_entry> &events);
  bool verify_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
};

class gen_simple_chain_001 : public test_chain_unit_base
{
public:
  gen_simple_chain_001();
  bool generate(std::vector<test_event_entry> &events);
  bool verify_callback_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
  bool verify_callback_2(currency::core& c, size_t ev_index, const std::vector<test_event_entry> &events);
};
