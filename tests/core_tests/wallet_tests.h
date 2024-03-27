// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"
#include "wallet_tests_basic.h"


struct gen_wallet_basic_transfer : public wallet_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_wallet_refreshing_on_chain_switch : public wallet_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_wallet_refreshing_on_chain_switch_2 : public wallet_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_wallet_unconfirmed_tx_from_tx_pool : public wallet_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_wallet_save_load_and_balance : public wallet_test
{
  gen_wallet_save_load_and_balance();

  bool generate(std::vector<test_event_entry>& events) const;
  bool c1_check_balance_and_store(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c3_load_refresh_check_balance(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};


struct gen_wallet_mine_pos_block : public wallet_test
{
  gen_wallet_mine_pos_block();

  bool generate(std::vector<test_event_entry>& events) const;

  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c3(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};


struct gen_wallet_unconfirmed_outdated_tx : public wallet_test
{
  gen_wallet_unconfirmed_outdated_tx();

  bool generate(std::vector<test_event_entry>& events) const;

  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c3(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct gen_wallet_unlock_by_block_and_by_time : public wallet_test
{
  gen_wallet_unlock_by_block_and_by_time();

  bool generate(std::vector<test_event_entry>& events) const;

  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c3(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c4(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct gen_wallet_payment_id : public wallet_test
{
  gen_wallet_payment_id();

  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable std::string m_payment_id;
};

struct gen_wallet_oversized_payment_id : public wallet_test
{
  gen_wallet_oversized_payment_id();

  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct gen_wallet_transfers_and_outdated_unconfirmed_txs : public wallet_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_wallet_transfers_and_chain_switch : public wallet_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_wallet_decrypted_attachments : public wallet_test, virtual public tools::i_wallet2_callback
{
  gen_wallet_decrypted_attachments();
  bool generate(std::vector<test_event_entry>& events) const;

  // intrface tools::i_wallet2_callback
  virtual void on_transfer2(const tools::wallet_public::wallet_transfer_info& wti, const std::list<tools::wallet_public::asset_balance_entry>& balances, uint64_t total_mined) override;

private:
  mutable bool          m_on_transfer2_called;
  mutable std::string   m_comment_to_be_checked;
  mutable std::string   m_address_to_be_checked;

};

struct gen_wallet_alias_and_unconfirmed_txs : public wallet_test
{
  gen_wallet_alias_and_unconfirmed_txs();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c2(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c3(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct gen_wallet_alias_via_special_wallet_funcs : public wallet_test
{
  gen_wallet_alias_via_special_wallet_funcs();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct gen_wallet_fake_outputs_randomness : public wallet_test
{
  gen_wallet_fake_outputs_randomness();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  mutable uint64_t m_amount_many_outs_have;
};

struct gen_wallet_fake_outputs_not_enough : public wallet_test
{
  gen_wallet_fake_outputs_not_enough();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  mutable uint64_t m_amount_many_outs_have;
  mutable uint64_t m_amount_no_outs_have;
};

struct gen_wallet_offers_basic : public wallet_test
{
  gen_wallet_offers_basic();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct gen_wallet_offers_size_limit : public wallet_test
{
  gen_wallet_offers_size_limit();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct gen_wallet_dust_to_account : public wallet_test
{
  gen_wallet_dust_to_account();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct gen_wallet_selecting_pos_entries : public wallet_test
{
  gen_wallet_selecting_pos_entries();
  bool generate(std::vector<test_event_entry>& events) const;
  bool set_core_config(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  mutable uint64_t m_amount;
};

struct gen_wallet_spending_coinstake_after_minting : public wallet_test
{
  gen_wallet_spending_coinstake_after_minting();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  mutable uint64_t m_amount;
};

struct gen_wallet_fake_outs_while_having_too_little_own_outs : public wallet_test
{
  gen_wallet_fake_outs_while_having_too_little_own_outs();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
private:
  mutable uint64_t m_amount_many_outs_have;
};

struct premine_wallet_test : public wallet_test
{
  premine_wallet_test();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_wallet(currency::core& c, const wchar_t* wallet_filename, const char* password, uint64_t amount);
};

struct mined_balance_wallet_test : public wallet_test
{
  mined_balance_wallet_test();
  bool generate(std::vector<test_event_entry>& events) const;
  bool set_core_config(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct wallet_outputs_with_same_key_image : public wallet_test
{
  wallet_outputs_with_same_key_image();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct wallet_unconfirmed_tx_expiration : public wallet_test
{
  wallet_unconfirmed_tx_expiration();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct wallet_chain_switch_with_spending_the_same_ki : public wallet_test
{
  wallet_chain_switch_with_spending_the_same_ki();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct wallet_unconfimed_tx_balance : public wallet_test
{
  wallet_unconfimed_tx_balance();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct packing_outputs_on_pos_minting_wallet : public wallet_test
{
  packing_outputs_on_pos_minting_wallet();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable uint64_t m_single_amount = 0;
  mutable uint64_t m_alice_initial_balance = 0;
  mutable uint64_t m_bob_initial_balance = 0;
};

struct wallet_sending_to_integrated_address : public wallet_test
{
  wallet_sending_to_integrated_address();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct wallet_watch_only_and_chain_switch : public wallet_test
{
  wallet_watch_only_and_chain_switch();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable crypto::hash m_split_point_block_id;
  mutable uint64_t m_split_point_block_height;
};

struct wallet_spend_form_auditable_and_track : public wallet_test
{
  wallet_spend_form_auditable_and_track();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable std::string m_comment;
};

struct wallet_and_sweep_below : public wallet_test
{
  wallet_and_sweep_below();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};


struct block_template_blacklist_test : public wallet_test
{
  block_template_blacklist_test();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};