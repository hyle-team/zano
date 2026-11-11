// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#pragma once

#include <atomic>
#include <chrono>
#include "native_wallet_history_origin.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include "crypto/crypto.h"
#include "currency_core/currency_config.h"

namespace tools
{
  class wallet2;
  struct i_core_proxy;
  namespace read_only
  {
    class core_proxy;
    class monotonic_clock;
    enum class step_status
    {
      Progress, NoProgress, Cancelled, Busy, ConnectionFailed, GenesisMismatch,
      InvalidResponse, PreparationRequired, UnsupportedCapability, RecoveryRequired,
      StepLimitExceeded, NodeBehind, Unavailable, PreparationProgress
    };

    struct step_limits
    {
      size_t max_blocks;
      size_t max_transactions; // Includes one coinbase per selected block.
      // Conservative rollback ceiling from the inclusive response anchor.
      // The default matches the existing seven-day wallet retention limit.
      uint64_t max_reorg_blocks = 7 * CURRENCY_BLOCKS_PER_DAY;
    };

    struct step_result
    {
      step_status status = step_status::Unavailable;
      uint64_t local_blocks_before = 0;
      uint64_t local_blocks_after = 0; // May shrink after an accepted reorg.
      // Daemon-reported height only, not a genesis handshake or sync evidence.
      uint64_t reported_daemon_blocks = 0;
      bool has_daemon_observation = false;
      size_t blocks_added = 0;
      // The native prefix transport validates the full reply; only its prefix is processed.
      bool batch_truncated = false;
      // Conservative: processing may change history even without adding a block.
      // Cancellation or failure after this boundary does not undo that work.
      bool state_may_have_changed = false;
    };

    // Native-internal block scanner, not a vault/session factory or bridge API.
    // The caller retains wallet and serializes ALL access for this object's life.
    // It prepares the minimum scan height explicitly, disables callbacks, and
    // supplies a trusted transport with its own timeouts and response-size limits.
    // Limits include the inclusive known anchor, so max_blocks must be >= 2.
    // A full daemon batch may be truncated to a whole-block prefix. Decoded body
    // and tail-byte limits belong to the transport, before this entry point.
    // step() issues at most one prefix-page OR block request and checks stop between blocks; it
    // cannot interrupt an in-flight transport call or a transaction's processing.
    // Busy protects serialized reentrancy only, not concurrent wallet access.
    // A known harmless pre-bootstrap stop/refusal can retain a validated
    // pinned cursor for the next explicit step. Ambiguous transport failure,
    // invalid data and cancellation inside the one-attempt selector are terminal.
    // Below-anchor recovery/reselection is unsupported and mutates no wallet.
    // NoProgress means wait/stop, not spin or claim completion. StepLimitExceeded
    // needs a changed budget/configuration; an identical retry cannot progress.
    // No pool scan, spendability/freshness claim, store, relay, or successful close.
    // Fault/dirty observations persist on the wallet across scanner recreation.
    // A fault requires owner-level recovery into a new wallet instance; this
    // type cannot reset it, save the wallet, or acknowledge persistence.
    // A dirty/faulted wallet must not be persisted as authoritative state merely
    // because persistence_required() is true; its owner must resolve recovery.
    class block_sync final
    {
    public:
      block_sync(wallet2& wallet, const crypto::hash& expected_genesis,
        std::shared_ptr<i_core_proxy> transport, step_limits limits);
      ~block_sync();
      block_sync(const block_sync&) = delete;
      block_sync& operator=(const block_sync&) = delete;
      block_sync(block_sync&&) = delete;
      block_sync& operator=(block_sync&&) = delete;
      step_result step(std::atomic<bool>& stop) noexcept;
      // Sticky processing fault. A clean preflight RecoveryRequired result
      // can request new preparation without setting this latch.
      bool recovery_required() const noexcept;
      bool persistence_required() const noexcept;
      // Existing internal provenance interface; no scan or mutation.
      // 0 = unavailable, 1 = this run's actually bootstrapped prefix boundary,
      // 3 = a validated protected saved origin. No genesis-fallback source.
      uint32_t scan_start_source_v1(uint64_t captured_minimum) const noexcept;
    private:
      wallet2& m_wallet;
      const crypto::hash m_expected_genesis;
      const step_limits m_limits;
      std::shared_ptr<core_proxy> m_proxy;
      bool m_in_step = false;
      static bool prefix_stopped(void* context) noexcept;
      bool history_origin_valid() const;
      bool m_birthday_pending = false;
      bool m_birthday_resolved = false;
      bool m_birthday_committed = false;
      bool m_birthday_terminal = false;
      bool m_genuine_date_valid = false;
      step_status m_birthday_refusal = step_status::PreparationRequired;
      uint64_t m_birthday_date = 0;
      uint64_t m_birthday_trusted_now = 0;
      uint64_t m_birthday_minimum = 0;
      uint64_t m_birthday_next_height = 0;
      policy::ChainView m_birthday_view{};
      std::unique_ptr<policy::Selector> m_birthday_selector;
      std::optional<policy::Boundary> m_birthday_boundary;
      const std::atomic<bool>* m_prefix_stop = nullptr;
      const monotonic_clock* m_prefix_clock = nullptr;
      std::chrono::steady_clock::time_point m_prefix_deadline{};
    };
  }
}
