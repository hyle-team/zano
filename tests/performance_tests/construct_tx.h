// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "currency_core/account.h"
#include "currency_core/currency_basic.h"
#include "currency_core/currency_format_utils.h"

#include "multi_tx_test_base.h"

template<size_t a_in_count, size_t a_out_count>
class test_construct_tx : private multi_tx_test_base<a_in_count>
{
  static_assert(0 < a_in_count, "in_count must be greater than 0");
  static_assert(0 < a_out_count, "out_count must be greater than 0");

public:
  static const size_t loop_count = (a_in_count + a_out_count < 100) ? 100 : 10;
  static const size_t in_count  = a_in_count;
  static const size_t out_count = a_out_count;

  typedef multi_tx_test_base<a_in_count> base_class;

  bool init()
  {
    using namespace currency;

    if (!base_class::init())
      return false;

    m_alice.generate();

    for (size_t i = 0; i < out_count; ++i)
    {
      m_destinations.push_back(tx_destination_entry(this->m_source_amount / out_count, m_alice.get_keys().account_address));
    }

    return true;
  }

  bool test()
  {
    return currency::construct_tx(this->m_miners[this->real_source_idx].get_keys(), this->m_sources, m_destinations, m_tx, 0);
  }

private:
  currency::account_base m_alice;
  std::vector<currency::tx_destination_entry> m_destinations;
  currency::transaction m_tx;
};




void parse_weird_tx()
{
  const std::string expected_hash = "62ee9fb89293f042cbcebb02d05fa15036ffca2bdeb4c48162f2d93a420e5974";
  std::string tx_hex_buff;
  epee::file_io_utils::load_file_to_string("E:\\tmp\\tx_as_hex.txt", tx_hex_buff);
  std::string tx_blob;
  epee::string_tools::parse_hexstr_to_binbuff(tx_hex_buff, tx_blob);
  currency::transaction tx;
  bool r = currency::parse_and_validate_tx_from_blob(tx_blob, tx);
  std::string res_hash = epee::string_tools::pod_to_hex(currency::get_transaction_hash(tx));
  if (res_hash != expected_hash)
  {
    std::cout << "this is sad";
  }
}