// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
// Additive product snapshot v4. capture_snapshot and product v1..v3 stay
// byte-for-byte in native_wallet_session.cpp; v4 runs the unchanged v3 capture
// and then copies the AUTHORITATIVE wallet's existing History-origin record on
// the same executor, under one busy scope, before one more authorization
// sample. No scanner, selector, proxy, clock, checkpoint or mutation is used.
#include "native_wallet_session.h"
#include "wallet2.h"
#include <cstring>
#include <utility>

namespace tools { namespace native_session
{
  namespace
  {
    using wide_hash = read_only::policy::Hash;

    wide_hash wide(const crypto::hash& value) noexcept
    {
      wide_hash result{};
      static_assert(sizeof(value) == sizeof(result), "Native hash width must match the origin record");
      std::memcpy(result.data(), &value, result.size());
      return result;
    }

    crypto::hash narrow(const wide_hash& value) noexcept
    {
      crypto::hash result{};
      static_assert(sizeof(value) == sizeof(result), "Native hash width must match the origin record");
      std::memcpy(&result, value.data(), value.size());
      return result;
    }

    // Same rule as the session's own guard: reentrant calls see Busy.
    struct interval_busy final
    {
      bool& busy;
      explicit interval_busy(bool& flag) noexcept : busy(flag) { busy = true; }
      ~interval_busy() noexcept { busy = false; }
      interval_busy(const interval_busy&) = delete;
      interval_busy& operator=(const interval_busy&) = delete;
    };
  }

  native_history_interval_v1 file_session::observe_history_interval(const public_snapshot& built,
    const scan_coverage_v1& coverage) const noexcept
  {
    native_history_interval_v1 unavailable; // Unavailable/Absent, every value zero.
    try
    {
      if (!m_wallet)
      {
        unavailable.reason = history_interval_reason_v1::Unverified;
        return unavailable;
      }
      // A legacy image's replay candidate is recomputable scratch until its
      // existing Committed checkpoint swaps it in. Never describe it.
      if (m_replay_wallet)
      {
        unavailable.reason = history_interval_reason_v1::ScratchReplay;
        return unavailable;
      }
      const auto& origin = m_wallet->m_native_history_origin;
      if (origin.phase == read_only::origin_phase::Absent) return unavailable;
      unavailable.reason = history_interval_reason_v1::Unverified;
      const bool complete = origin.phase == read_only::origin_phase::Complete;
      if (!complete && origin.phase != read_only::origin_phase::Scanning) return unavailable;
      // The producer's own validator: native account, native rounded seed date,
      // minimum = first - 1, anchor/genesis/connected tip of this wallet's chain.
      if (!m_wallet->native_history_origin_valid()) return unavailable;
      // The same captured page: session, revision, verified identity/genesis.
      if (built.session_generation != m_generation || built.revision != m_revision ||
          built.genesis != m_identity.stored_genesis || origin.view.genesis_hash != wide(built.genesis))
        return unavailable;
      // The same capture's selected boundary.
      if (coverage.version != 1 || origin.first_height == 0 ||
          (coverage.source != scan_start_source_v1::SeedDate &&
           coverage.source != scan_start_source_v1::SavedBoundary) ||
          coverage.minimum_height != origin.first_height - 1)
        return unavailable;
      // The same capture's local tip. An unknown tip hash is never assumed.
      if (built.local_blocks == 0 || built.local_tip_height != built.local_blocks - 1 ||
          origin.connected_height != built.local_tip_height)
        return unavailable;
      if (built.local_tip_hash_known)
      {
        if (origin.connected_hash != wide(built.local_tip_hash)) return unavailable;
      }
      else if (built.local_tip_height != 0 || origin.connected_hash != wide(built.genesis))
      {
        return unavailable;
      }
      if (origin.view.target_height < origin.first_height ||
          (complete && origin.connected_height < origin.view.target_height))
        return unavailable;

      native_history_interval_v1 observed;
      // Copied, never promoted: Scanning stays Scanning even at its target.
      observed.state = complete ? history_interval_state_v1::Complete : history_interval_state_v1::Scanning;
      observed.reason = history_interval_reason_v1::Observed;
      observed.rounded_date = origin.rounded_date;
      observed.first_height = origin.first_height;
      observed.anchor_hash = narrow(origin.anchor_hash);
      observed.view_height = origin.view.target_height;
      observed.view_hash = narrow(origin.view.target_hash);
      observed.connected_height = origin.connected_height;
      observed.connected_hash = narrow(origin.connected_hash);
      observed.checkpointed = coverage.checkpointed && state() == session_state::OpenUnchanged;
      return observed;
    }
    catch (...)
    {
      native_history_interval_v1 failed;
      failed.reason = history_interval_reason_v1::Unverified;
      return failed;
    }
  }

  product_snapshot_result_v4 file_session::capture_product_snapshot_v4(const snapshot_request& request) noexcept
  {
    epee::log_space::scoped_log_silence quiet;
    product_snapshot_result_v4 out;
    auto captured = capture_product_snapshot_v3(request);
    if (captured.version != 3 || captured.status != snapshot_status::Available || !captured.observation)
    {
      out.status = captured.status == snapshot_status::Available ? snapshot_status::Unavailable : captured.status;
      return out;
    }
    // v3 returned on this executor; no step, checkpoint or callback ran since.
    if (m_busy)
    {
      out.status = snapshot_status::Busy;
      return out;
    }
    native_history_interval_v1 interval;
    {
      interval_busy guard(m_busy);
      interval = observe_history_interval(*captured.observation, captured.scan_coverage);
      if (captured.observation->session_generation != m_generation ||
          captured.observation->revision != m_revision)
      {
        out.status = snapshot_status::InvalidData;
        return out;
      }
      // Final sample now also covers the interval copy. Sticky on loss.
      if (!authorized())
      {
        out.status = snapshot_status::Revoked;
        return out;
      }
    }
    out.observation = std::move(captured.observation);
    out.scan_coverage = captured.scan_coverage;
    out.owned_outputs.swap(captured.owned_outputs);
    out.retained_birthday = captured.retained_birthday;
    out.history_interval = interval;
    out.status = snapshot_status::Available;
    return out;
  }
}}
