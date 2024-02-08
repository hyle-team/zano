// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"
#include "random_helper.h"

struct checkpoints_test : virtual public test_chain_unit_enchanced
{
  checkpoints_test();

  bool set_checkpoint(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool set_far_checkpoint(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_being_in_cp_zone(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_not_being_in_cp_zone(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

protected:
  struct params_checkpoint
  {
    params_checkpoint(uint64_t height = 0, const crypto::hash& hash = currency::null_hash) : height(height), hash(hash) {}
    uint64_t height;
    crypto::hash hash;
  };

private:
  currency::checkpoints m_local_checkpoints;
};

struct gen_checkpoints_attachments_basic : public checkpoints_test
{
  gen_checkpoints_attachments_basic();
  bool generate(std::vector<test_event_entry>& events) const;
  bool check_tx(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable crypto::hash m_tx_hash;
};

struct gen_checkpoints_invalid_keyimage : public checkpoints_test
{
  gen_checkpoints_invalid_keyimage();
  bool generate(std::vector<test_event_entry>& events) const;
  bool check(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct gen_checkpoints_altblock_before_and_after_cp : public checkpoints_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_checkpoints_block_in_future : public checkpoints_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_checkpoints_altchain_far_before_cp : public checkpoints_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_checkpoints_block_in_future_after_cp : public checkpoints_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_checkpoints_prun_txs_after_blockchain_load : public checkpoints_test
{
  gen_checkpoints_prun_txs_after_blockchain_load();
  bool generate(std::vector<test_event_entry>& events) const;
  bool check_txs(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  mutable crypto::hash m_tx0_id;
  mutable crypto::hash m_tx1_id;
};

struct gen_checkpoints_reorganize : public checkpoints_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_checkpoints_pos_validation_on_altchain : public checkpoints_test
{
  gen_checkpoints_pos_validation_on_altchain();
  bool generate(std::vector<test_event_entry>& events) const;
  bool init_runtime_config(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct gen_no_attchments_in_coinbase : public checkpoints_test
{
  gen_no_attchments_in_coinbase();
  bool generate(std::vector<test_event_entry>& events) const;

  bool init_config_set_cp(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  random_state_test_restorer m_random_state_test_restorer;
  mutable test_generator generator;
  mutable currency::account_base m_miner_acc;
  currency::checkpoints m_checkpoints;
};

struct gen_no_attchments_in_coinbase_gentime : public checkpoints_test
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_checkpoints_and_invalid_tx_to_pool : public checkpoints_test
{
  gen_checkpoints_and_invalid_tx_to_pool();
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
};

struct gen_checkpoints_set_after_switching_to_altchain : public checkpoints_test
{
  gen_checkpoints_set_after_switching_to_altchain();
  bool generate(std::vector<test_event_entry>& events) const;
};
