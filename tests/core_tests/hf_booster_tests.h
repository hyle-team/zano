// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include "chaingen.h"
#include "wallet_tests_basic.h"

struct hf_booster_hf8_activation_basic : public wallet_test
{
  hf_booster_hf8_activation_basic();
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool generate(std::vector<test_event_entry>& events) const;
  bool c1(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  currency::keypair m_maintainer_keys;
};


// Common base for gentime HF booster tests: HF1..HF7 are active from the start, HF8 is scheduled far away,
// the core gets a test maintainer key for HF8 and a short boost-to-activation distance.
struct hf_booster_test_base : public wallet_test
{
  hf_booster_test_base();
  currency::core_runtime_config get_runtime_info_for_core() const; // the replay engine applies it after every core (re)init
  bool configure_core(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_hf8_height(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events); // params: uint64_t expected "active after" height of HF8

protected:
  struct booster_tx_options
  {
    const crypto::secret_key* p_signer_sec = nullptr;                   // nullptr: sign with the maintainer key
    const crypto::public_key* p_bind_to_pub_key = nullptr;              // nullptr: bind to the tx one-time pub key (correct binding)
  };

  bool make_booster_tx(const std::vector<test_event_entry>& events, const currency::block& blk_head, const currency::hard_forks_descriptor& hf,
    const currency::account_base& from, uint64_t hf_id, uint64_t new_height, currency::transaction& tx, const booster_tx_options& opt = booster_tx_options()) const;

  currency::keypair m_maintainer_keys;
};

// A booster tx is mined, HF8 is activated at the boosted height, HF8 txs are accepted afterwards.
// Also registered in the blockchain save/load mode so that the schedule has to survive a core restart.
struct hf_booster_gentime_basic : public hf_booster_test_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

// Chain switches before HF8 activation: one removes a postponing booster, another removes the first booster.
struct hf_booster_reorg_before_activation : public hf_booster_test_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

// Chain switch across the boosted activation height, both chains contain the booster.
struct hf_booster_reorg_after_activation : public hf_booster_test_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

// A block with a valid booster and another bad tx is rejected: both the exception path (bad booster signature)
// and the regular path (double spend) must leave the schedule untouched.
struct hf_booster_rollback_on_invalid_block : public hf_booster_test_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

// The core runtime config is set again after a booster was applied: the boosted schedule must survive.
struct hf_booster_runtime_config_reset_keeps_boost : public hf_booster_test_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

// Boosters that must not be accepted: wrong signer, wrong key binding, bad heights, unknown or active hardfork.
struct hf_booster_invalid_boosters_rejected : public hf_booster_test_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
