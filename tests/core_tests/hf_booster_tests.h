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
  bool check_unsupported_hardfork_id(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events); // params: uint64_t expected unsupported_hardfork_id, non-zero also means the stop flag is set

protected:
  struct booster_tx_options
  {
    booster_tx_options() : p_signer_sec(nullptr), p_bind_to_pub_key(nullptr) {} // GCC rejects default member initializers here (used in a default argument below)
    const crypto::secret_key* p_signer_sec;                   // nullptr: sign with the maintainer key
    const crypto::public_key* p_bind_to_pub_key;              // nullptr: bind to the tx one-time pub key (correct binding)
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

// A block with a valid booster and another bad tx is rejected, and the valid booster leaves no trace: a booster with
// a height in the past and a double spend before HF8, a booster with a bad signature after HF8. Before HF8 a bad
// signature is ignored instead and the block is valid.
struct hf_booster_rollback_on_invalid_block : public hf_booster_test_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

// Bad boosters: bad heights make a block invalid; a wrong signer, a wrong key binding or an id below HF8 are rejected by
// the tx pool, ignored in a block before HF8 and make it invalid after HF8; an unknown id is recorded only.
struct hf_booster_invalid_boosters_rejected : public hf_booster_test_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};

// A build that knows HF8 but is not ready for it (last_supported_hardfork_id = 7): it follows a booster for HF8 until the
// activation, then refuses the first HF8 block and raises the stop flag.
struct hf_booster_unsupported_hf_halts : public hf_booster_test_base
{
  hf_booster_unsupported_hf_halts();
  bool generate(std::vector<test_event_entry>& events) const;
};

// A correctly signed booster for a hardfork id this build does not know is recorded only, and the node refuses the first
// block for which that hardfork is active. Also registered in the blockchain save/load mode: the record survives restarts.
struct hf_booster_unknown_hf_halts : public hf_booster_test_base
{
  bool generate(std::vector<test_event_entry>& events) const;
};
