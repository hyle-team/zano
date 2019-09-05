// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"

struct offers_tests : public test_chain_unit_base
{
  offers_tests();

  bool check_block_verification_context(const currency::block_verification_context& bvc, size_t event_idx, const currency::block& /*blk*/)
  {
    return true;
  }

  bool generate(std::vector<test_event_entry>& events) const;
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_offers_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_offers_updated_1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_offers_updated_2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:

  mutable crypto::hash tx_id_1;
  mutable crypto::hash tx_id_2;

};


struct offers_expiration_test : public test_chain_unit_enchanced
{
  offers_expiration_test();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
private:
  mutable uint64_t m_offers_ts;
  mutable bc_services::offer_details_ex m_offer;
};


struct offers_filtering_1 : public test_chain_unit_enchanced
{
  offers_filtering_1();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  enum { offers_count = 10 };
};


struct offers_handling_on_chain_switching : public wallet_test
{
  offers_handling_on_chain_switching();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct offer_removing_and_selected_output : public wallet_test
{
  offer_removing_and_selected_output();
  bool generate(std::vector<test_event_entry>& events) const;
  bool check_offers(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct offers_and_stuck_txs : public wallet_test
{
  offers_and_stuck_txs();
  bool generate(std::vector<test_event_entry>& events) const;
  bool check_offers(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct offers_updating_hack : public wallet_test
{
  offers_updating_hack();
  bool generate(std::vector<test_event_entry>& events) const;
  bool update_foreign_offer(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool delete_foreign_offer(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct offers_multiple_update : public wallet_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct offer_sig_validity_in_update_and_cancel : public wallet_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct offer_lifecycle_via_tx_pool : public wallet_test
{
  offer_lifecycle_via_tx_pool();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct offer_cancellation_with_zero_fee : public wallet_test
{
  offer_cancellation_with_zero_fee();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};
