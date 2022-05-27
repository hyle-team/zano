// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"

struct gen_tx_big_version : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_unlock_time : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_input_is_not_txin_to_key : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_no_inputs_no_outputs : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_no_inputs_has_outputs : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_has_inputs_no_outputs : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_invalid_input_amount : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_input_wo_key_offsets : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_key_offest_points_to_foreign_key : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_sender_key_offest_not_exist : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_mixed_key_offest_not_exist : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_key_image_not_derive_from_tx_key : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_key_image_is_invalid : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_check_input_unlock_time : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_txout_to_key_has_invalid_key : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_output_with_zero_amount : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_output_is_not_txout_to_key : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_tx_signatures_are_invalid : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_broken_attachments : test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_crypted_attachments : test_chain_unit_enchanced
{
  gen_crypted_attachments();
  
  bool generate(std::vector<test_event_entry>& events) const;
  bool check_crypted_tx(currency::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/);
  bool set_blockchain_height(currency::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/);
  bool set_crypted_tx_height(currency::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/);
  bool check_offers_count_befor_cancel(currency::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/);
  bool check_offers_count_after_cancel(currency::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/);

  mutable size_t crypted_tx_height;
  mutable uint64_t bc_height_before;
  mutable uint64_t offers_before_canel;
  mutable currency::tx_payer pr;
  mutable currency::tx_comment cm;
  //mutable currency::tx_message ms;
};

struct gen_tx_extra_double_entry : test_chain_unit_enchanced
{
  gen_tx_extra_double_entry();
  bool generate(std::vector<test_event_entry>& events) const;
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct gen_tx_double_key_image : test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct tx_expiration_time : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct tx_expiration_time_and_block_template : public test_chain_unit_enchanced
{
  tx_expiration_time_and_block_template();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct tx_expiration_time_and_chain_switching : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct tx_key_image_pool_conflict : public test_chain_unit_enchanced
{
  bool generate(std::vector<test_event_entry>& events) const;
  mutable currency::account_base m_miner_acc;
};
