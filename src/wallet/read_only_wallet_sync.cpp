// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#include "read_only_wallet_sync.h"
#include "read_only_core_proxy.h"
#include "read_only_block_transport.h"
#include "wallet2.h"
#include <utility>
#include <ctime>
#include <cstring>
#include <limits>
#include "read_only_wallet_birthday.h"

namespace tools { namespace read_only
{
  namespace
  {
    policy::Hash policy_hash(const crypto::hash& value) noexcept
    {
      policy::Hash result{};
      static_assert(sizeof(value) == sizeof(result), "Native hash width must match prefix policy");
      std::memcpy(result.data(), &value, result.size());
      return result;
    }
    crypto::hash native_hash(const policy::Hash& value) noexcept
    {
      crypto::hash result{};
      static_assert(sizeof(value) == sizeof(result), "Native hash width must match prefix policy");
      std::memcpy(&result, value.data(), value.size());
      return result;
    }
  }

  block_sync::block_sync(wallet2& wallet, const crypto::hash& expected_genesis,
    std::shared_ptr<i_core_proxy> transport, step_limits limits)
    : m_wallet(wallet), m_expected_genesis(expected_genesis), m_limits(limits),
      m_proxy(std::make_shared<core_proxy>(std::move(transport)))
  {
    const auto minimum = m_wallet.m_minimum_height;
    m_birthday_pending = birthday::needs_date(minimum, WALLET_MINIMUM_HEIGHT_UNSET_CONST);
    if (m_birthday_pending)
    {
      // A fresh attempt samples trusted time once before prefix selection.
      const auto current_time = std::time(nullptr);
      if (current_time > 0) m_birthday_trusted_now = static_cast<uint64_t>(current_time);
      m_genuine_date_valid = m_birthday_trusted_now && birthday::rounded_date(
        m_wallet.m_account.get_createtime(), m_birthday_trusted_now, m_birthday_date);
    }
    else
    {
      // Protected saved lineage is checked against the seed format, independently
      // of today's wall clock, which may have rolled back since creation.
      m_genuine_date_valid = birthday::rounded_date(m_wallet.m_account.get_createtime(),
        std::numeric_limits<uint64_t>::max(), m_birthday_date);
    }
  }
  block_sync::~block_sync() = default;
  bool block_sync::recovery_required() const noexcept { return m_wallet.read_only_sync_recovery_required(); }
  bool block_sync::persistence_required() const noexcept { return m_wallet.read_only_sync_persistence_required(); }
  bool block_sync::prefix_stopped(void* context) noexcept
  {
    const auto& scanner = *static_cast<const block_sync*>(context);
    return !scanner.m_prefix_stop || !scanner.m_prefix_clock ||
      scanner.m_prefix_stop->load(std::memory_order_relaxed) ||
      scanner.m_prefix_clock->now() >= scanner.m_prefix_deadline;
  }

  bool block_sync::history_origin_valid() const
  {
    const auto& origin = m_wallet.m_native_history_origin;
    const auto blocks = m_wallet.get_blockchain_current_size();
    if ((origin.phase != origin_phase::Scanning && origin.phase != origin_phase::Complete) ||
        !m_genuine_date_valid || origin.rounded_date != m_birthday_date ||
        origin.first_height == 0 || origin.first_height >= CURRENCY_MAX_BLOCK_NUMBER ||
        m_wallet.m_minimum_height != origin.first_height - 1 ||
        !policy::nonzero_hash(origin.anchor_hash) || !policy::nonzero_hash(origin.connected_hash) ||
        !policy::nonzero_hash(origin.view.target_hash) || origin.view.token != origin.view.target_hash ||
        origin.view.genesis_hash != policy_hash(m_expected_genesis) ||
        origin.view.target_height >= CURRENCY_MAX_BLOCK_NUMBER ||
        origin.view.target_height < origin.first_height || !blocks ||
        origin.connected_height != blocks - 1 || origin.connected_height < origin.first_height - 1 ||
        origin.connected_hash != policy_hash(blocks == 1 ? m_expected_genesis : m_wallet.m_chain.get_top_block_id()) ||
        (origin.phase == origin_phase::Complete && origin.connected_height < origin.view.target_height))
      return false;
    const uint64_t anchor_height = origin.first_height - 1;
    if (anchor_height == 0) return origin.anchor_hash == policy_hash(m_expected_genesis);
    std::pair<uint64_t, crypto::hash> retained{};
    if (m_wallet.m_chain.lookup_item_around(anchor_height, retained) && retained.first == anchor_height &&
        policy_hash(retained.second) != origin.anchor_hash) return false;
    // An aged-out anchor relies on the protected loader's exact origin/tip receipt
    // and the invariant that this scanner never detaches at/below the anchor.
    return true;
  }

  uint32_t block_sync::scan_start_source_v1(uint64_t captured_minimum) const noexcept
  {
    try
    {
      if (!history_origin_valid() || captured_minimum != m_wallet.m_native_history_origin.first_height - 1)
        return 0;
      return m_birthday_pending ? (m_birthday_committed ? 1 : 0) : 3;
    }
    catch (...) { return 0; }
  }

  step_result block_sync::step(std::atomic<bool>& stop) noexcept
  {
    epee::log_space::scoped_log_silence quiet;
    step_result result;
    bool attempt_entered = false;
    auto finish = [&](step_status status, bool prefix_retry = false) {
      if (attempt_entered && m_birthday_pending && !m_birthday_committed &&
          status != step_status::PreparationProgress &&
          !(prefix_retry && !result.state_may_have_changed && m_wallet.native_birthday_fresh()))
      {
        // Invalid data, ambiguous transport failure and a terminal selector
        // discard the attempt. Known harmless exits keep only validated state
        // for a later explicit step; this call never retries or resets a budget.
        m_birthday_terminal = true;
        m_birthday_refusal = status;
        m_birthday_selector.reset();
        m_birthday_boundary.reset();
        m_birthday_view = {};
        m_birthday_next_height = 0;
        m_birthday_minimum = 0;
        m_birthday_resolved = false;
      }
      result.status = status;
      result.local_blocks_after = m_wallet.get_blockchain_current_size();
      return result;
    };
    result.local_blocks_before = m_wallet.get_blockchain_current_size();
    if (m_in_step || m_wallet.m_read_only_sync_active) return finish(step_status::Busy);
    if (m_wallet.m_read_only_sync_faulted) return finish(step_status::RecoveryRequired);
    attempt_entered = true;
    if (m_birthday_terminal) return finish(m_birthday_refusal);
    if (stop.load(std::memory_order_relaxed)) return finish(step_status::Cancelled, true);
    if (m_wallet.m_offline_mode || m_wallet.m_watch_only ||
        m_wallet.m_do_rise_transfer || !m_wallet.m_wcallback.expired())
      return finish(step_status::UnsupportedCapability);
    if (!m_proxy->available() || m_expected_genesis == currency::null_hash ||
        !result.local_blocks_before || m_limits.max_blocks < 2 || !m_limits.max_transactions ||
        m_limits.max_blocks > COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT ||
        m_limits.max_reorg_blocks > WALLET_CONCISE_MODE_MAX_REORG_BLOCKS ||
        (m_wallet.m_minimum_height == WALLET_MINIMUM_HEIGHT_UNSET_CONST && !m_wallet.native_birthday_fresh()))
      return finish(step_status::PreparationRequired);
    if (m_wallet.m_chain.get_genesis() != m_expected_genesis)
      return finish(step_status::GenesisMismatch);

    // Restore the original proxy and mode on every exit. No legacy API is
    // upgraded permanently and the immutable offline fixture is never admitted.
    struct step_scope
    {
      bool& entered;
      bool& active;
      std::shared_ptr<i_core_proxy>& slot;
      std::shared_ptr<i_core_proxy> previous;
      step_scope(bool& entered_, bool& active_, std::shared_ptr<i_core_proxy>& slot_,
        std::shared_ptr<i_core_proxy> replacement) noexcept
        : entered(entered_), active(active_), slot(slot_), previous(std::move(slot_))
      { entered = true; active = true; slot = std::move(replacement); }
      ~step_scope() noexcept
      { slot = std::move(previous); active = false; entered = false; }
    } scope(m_in_step, m_wallet.m_read_only_sync_active, m_wallet.m_core_proxy, m_proxy);

    m_proxy->begin_step();
    try
    {
      static_assert(birthday::date_offset == WALLET_BRAIN_DATE_OFFSET &&
        birthday::date_quantum == WALLET_BRAIN_DATE_QUANTUM && birthday::max_weeks == WALLET_BRAIN_DATE_MAX_WEEKS_COUNT,
        "Birthday policy must match the native seed format");
      static_assert(policy::kEncodedWeekOffset == birthday::date_offset &&
        policy::kEncodedWeekSeconds == birthday::date_quantum,
        "Prefix selection must use the genuine native rounded seed date");
      const bool fresh = m_wallet.m_minimum_height == WALLET_MINIMUM_HEIGHT_UNSET_CONST;
      if (!fresh && !history_origin_valid()) return finish(step_status::PreparationRequired);
      if (m_birthday_pending && !m_birthday_resolved)
      {
        if (!fresh || !m_wallet.native_birthday_fresh() || !m_genuine_date_valid)
          return finish(step_status::PreparationRequired);
        if (!m_proxy->has_birthday_prefix_source()) return finish(step_status::UnsupportedCapability, true);
        birthday_prefix::request prefix_request;
        prefix_request.start_height = m_birthday_next_height;
        prefix_request.genesis_hash = policy_hash(m_expected_genesis);
        if (m_birthday_selector)
        {
          prefix_request.target_height = m_birthday_view.target_height;
          prefix_request.target_hash = m_birthday_view.target_hash;
        }
        birthday_prefix::page page;
        if (!m_proxy->fetch_birthday_prefix(prefix_request, page, stop))
        {
          // The boolean interface cannot distinguish malformed replies from
          // ordinary transport failure. Neither establishes a safe retry cursor.
          if (m_proxy->denied()) return finish(step_status::UnsupportedCapability, true);
          return finish(stop.load(std::memory_order_relaxed) ? step_status::Cancelled :
            step_status::ConnectionFailed);
        }
        if (page.start_height != prefix_request.start_height ||
            (m_birthday_selector && !policy::same_view(page.view, m_birthday_view)) ||
            page.view.target_height >= CURRENCY_MAX_BLOCK_NUMBER ||
            page.view.target_height < page.start_height ||
            page.view.genesis_hash != policy_hash(m_expected_genesis))
          return finish(step_status::InvalidResponse);
        const uint64_t remaining = page.view.target_height - page.start_height;
        const uint64_t expected_count = remaining < prefix_request.count ? remaining + 1 : prefix_request.count;
        if (page.count != expected_count || page.count > page.headers.size())
          return finish(step_status::InvalidResponse);
        if (!page.decode_clock || page.decode_deadline == std::chrono::steady_clock::time_point{})
          return finish(step_status::InvalidResponse);
        if (stop.load(std::memory_order_relaxed) || page.decode_clock->now() >= page.decode_deadline)
          return finish(step_status::Cancelled, true);
        if (!m_birthday_selector)
        {
          m_birthday_view = page.view;
          policy::Request selection;
          selection.rounded_date = m_birthday_date;
          selection.trusted_now = m_birthday_trusted_now;
          selection.expected_genesis = policy_hash(m_expected_genesis);
          selection.view = page.view;
          m_birthday_selector = std::make_unique<policy::Selector>(selection, &block_sync::prefix_stopped, this);
        }
        // Decode and selector share one clock/deadline. The local page owns this
        // context; the scanner borrows it only for the synchronous consume call.
        m_prefix_stop = &stop;
        m_prefix_clock = page.decode_clock.get();
        m_prefix_deadline = page.decode_deadline;
        const auto selected = m_birthday_selector->consume({page.view, page.start_height, page.headers.data(), page.count});
        const bool decode_expired = m_prefix_clock->now() >= m_prefix_deadline;
        m_prefix_stop = nullptr;
        m_prefix_clock = nullptr;
        m_prefix_deadline = {};
        const bool selection_stopped = stop.load(std::memory_order_relaxed) || decode_expired;
        if (selected.status == policy::Status::NeedMore)
        {
          // Selector accepted the entire tail and committed its cursor. Keep
          // the scanner cursor identical even when cancellation arrives late.
          m_birthday_next_height = page.headers[page.count - 1].height + 1;
          return finish(selection_stopped ? step_status::Cancelled : step_status::PreparationProgress,
            selection_stopped);
        }
        if (selected.status != policy::Status::BoundaryFound || !selected.boundary)
          return finish(selected.status == policy::Status::Cancelled ? step_status::Cancelled :
            step_status::InvalidResponse);
        const auto& boundary = *selected.boundary;
        if (boundary.candidate.height == 0 || boundary.candidate.height >= CURRENCY_MAX_BLOCK_NUMBER ||
            boundary.predecessor.height != boundary.candidate.height - 1 ||
            boundary.candidate.previous_hash != boundary.predecessor.hash ||
            boundary.rounded_date != m_birthday_date)
          return finish(step_status::InvalidResponse);
        m_birthday_boundary = boundary;
        m_birthday_minimum = boundary.predecessor.height;
        m_birthday_resolved = true;
        m_birthday_selector.reset();
        m_birthday_next_height = 0;
        // Header proof preparation alone changes no wallet or origin bytes.
        // A completed boundary remains valid if stop arrives after consume.
        return finish(selection_stopped ? step_status::Cancelled : step_status::PreparationProgress,
          selection_stopped);
      }
      const bool bootstrap = fresh;
      if (fresh && (!m_birthday_resolved || !m_birthday_boundary || !m_wallet.native_birthday_fresh()))
        return finish(step_status::PreparationRequired);
      currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request request{};
      validated_prefix_response prefix_response{};
      auto& response = prefix_response.selected;
      request.minimum_height = fresh ? m_birthday_minimum : m_wallet.m_minimum_height;
      m_wallet.m_chain.get_short_chain_history(request.block_ids);
      if (request.block_ids.empty() || request.block_ids.back() != m_expected_genesis)
        return finish(step_status::PreparationRequired);
      if (stop.load(std::memory_order_relaxed)) return finish(step_status::Cancelled, true);
      // The borrowed owner token reaches a stop-aware transport; it is never cleared.
      const bool native_prefix = m_proxy->has_validated_prefix_source();
      const bool received = native_prefix
        ? m_proxy->fetch_validated_prefix(request,
            {m_limits.max_blocks, m_limits.max_transactions}, prefix_response, stop)
        : m_proxy->call_COMMAND_RPC_GET_BLOCKS_DIRECT(request, response, stop);
      // A false result is ambiguous even if owner stop raced the transport.
      // Do not retain proof state after an undistinguished malformed reply.
      if (!received) return finish(stop.load(std::memory_order_relaxed) ?
        step_status::Cancelled : step_status::ConnectionFailed);
      if (stop.load(std::memory_order_relaxed)) return finish(step_status::Cancelled, true);
      if (response.status == API_RETURN_CODE_GENESIS_MISMATCH)
        return finish(step_status::GenesisMismatch);
      if (response.status == API_RETURN_CODE_BUSY) return finish(step_status::Busy, true);
      if (response.status != API_RETURN_CODE_OK)
        return finish(step_status::InvalidResponse);
      if (m_proxy->denied()) return finish(step_status::UnsupportedCapability);
      if (m_wallet.m_chain.get_genesis() != m_expected_genesis)
        return finish(step_status::GenesisMismatch);

      // Native transport has already validated the entire reply and retained
      // the requested whole-block prefix. Legacy transports still return the
      // complete DIRECT response and use the existing selection below.
      const size_t full_block_count = native_prefix
        ? prefix_response.full_block_count : response.blocks.size();
      if (native_prefix &&
          (response.blocks.size() > full_block_count ||
           prefix_response.prefix_truncated != (response.blocks.size() != full_block_count)))
        return finish(step_status::InvalidResponse);
      if (!response.current_height ||
          full_block_count > COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT ||
          response.start_height >= response.current_height ||
          full_block_count > response.current_height - response.start_height)
        return finish(step_status::InvalidResponse);
      if (response.current_hardfork != 0 &&
          response.current_hardfork != m_wallet.get_core_runtime_config().hard_forks
            .get_the_most_recent_hardfork_id_for_height(response.current_height))
        return finish(step_status::InvalidResponse);
      result.has_daemon_observation = true;
      result.reported_daemon_blocks = response.current_height;
      // An old node commonly returns an older inclusive anchor, not an empty
      // response. Never roll back a wallet merely to match that observation.
      if (response.current_height < result.local_blocks_before)
        return finish(step_status::NodeBehind, true);
      if (bootstrap && (!native_prefix || response.start_height != m_birthday_minimum || response.blocks.empty()))
        return finish(step_status::InvalidResponse);
      if (!bootstrap && response.start_height < m_wallet.m_native_history_origin.first_height - 1)
        // Existing origin remains exact. Below-anchor recovery/reselection is
        // explicitly unsupported here; never detach or reset authoritative state.
        return finish(step_status::PreparationRequired);
      if (!bootstrap && response.start_height >= result.local_blocks_before)
      {
        if (request.minimum_height > 0 && response.start_height == request.minimum_height)
          return finish(step_status::PreparationRequired);
        return finish(step_status::InvalidResponse);
      }
      if (!full_block_count)
      {
        if (response.current_height != result.local_blocks_before)
          return finish(step_status::InvalidResponse);
        return finish(step_status::NoProgress);
      }

      size_t transactions_left = m_limits.max_transactions;
      size_t selected_count = 0;
      auto selected_end = response.blocks.begin();
      // Count before hashing. Include the anchor and stop before any block
      // that cannot fit; never partially process a block.
      for (; selected_end != response.blocks.end() &&
             selected_count < m_limits.max_blocks; ++selected_end, ++selected_count)
      {
        if (stop.load(std::memory_order_relaxed)) return finish(step_status::Cancelled, true);
        if (!transactions_left || selected_end->txs_ptr.size() > transactions_left - 1)
          break;
        transactions_left -= 1 + selected_end->txs_ptr.size();
      }
      result.batch_truncated = native_prefix
        ? prefix_response.prefix_truncated : selected_end != response.blocks.end();
      // A native result must fit the limits supplied to its side interface;
      // never silently trim it or accept an empty prefix as an empty full reply.
      if (native_prefix && selected_end != response.blocks.end())
        return finish(step_status::InvalidResponse);
      if (!selected_count || (full_block_count > 1 && selected_count < 2) || (bootstrap && selected_count < 2))
        return finish(step_status::StepLimitExceeded);
      uint64_t height = response.start_height;
      crypto::hash previous{};
      bool first = true;
      for (auto selected = response.blocks.begin(); selected != selected_end; ++selected)
      {
        const auto& entry = *selected;
        if (stop.load(std::memory_order_relaxed)) return finish(step_status::Cancelled, true);
        if (!entry.block_ptr || !entry.coinbase_ptr)
          return finish(step_status::InvalidResponse);
        const auto& block = entry.block_ptr->bl;
        // PoS coinbases have another input; require the generation input first.
        if (block.miner_tx.vin.empty() ||
            block.miner_tx.vin.front().type() != typeid(currency::txin_gen) ||
            currency::get_block_height(block) != height ||
            entry.coinbase_ptr->m_global_output_indexes.size() != block.miner_tx.vout.size() ||
            entry.txs_ptr.size() != block.tx_hashes.size())
          return finish(step_status::InvalidResponse);
        crypto::hash id{};
        if (!currency::get_block_hash(block, id)) return finish(step_status::InvalidResponse);
        if (bootstrap)
        {
          const auto& boundary = *m_birthday_boundary;
          if ((height == boundary.predecessor.height && policy_hash(id) != boundary.predecessor.hash) ||
              (height == boundary.candidate.height && policy_hash(id) != boundary.candidate.hash))
            return finish(step_status::InvalidResponse);
        }
        else if (height == m_wallet.m_native_history_origin.first_height - 1 &&
                 policy_hash(id) != m_wallet.m_native_history_origin.anchor_hash)
          return finish(step_status::PreparationRequired);
        if (height == 0 && id != m_expected_genesis)
          return finish(step_status::GenesisMismatch);
        if (first && !bootstrap)
        {
          bool found = false, matched = false, reset = false;
          m_wallet.m_chain.check_if_block_matched(height, id, found, matched, reset);
          // Require a known anchor. A birthday-height jump or missing history
          // requires a separate preparation/recovery decision.
          if (!found || !matched || reset)
            return finish(height <= m_wallet.m_native_history_origin.first_height - 1 ?
              step_status::PreparationRequired : step_status::RecoveryRequired);
          // Bound every possible later detach independently of mutable concise
          // settings. Conservative: an old anchor can fail even if a later
          // match would reduce the actual rollback. No wallet mutation yet.
          if (result.local_blocks_before - (height + 1) > m_limits.max_reorg_blocks)
            return finish(step_status::RecoveryRequired);
        }
        else if (!first && block.prev_id != previous)
          return finish(step_status::InvalidResponse);
        first = false;
        previous = id;
        size_t tx_index = 0;
        for (const auto& transaction : entry.txs_ptr)
        {
          if (stop.load(std::memory_order_relaxed)) return finish(step_status::Cancelled, true);
          if (!transaction ||
              transaction->m_global_output_indexes.size() != transaction->tx.vout.size() ||
              currency::get_transaction_hash(transaction->tx) != block.tx_hashes[tx_index++])
            return finish(step_status::InvalidResponse);
        }
        ++height; // bounded by current_height above, so cannot overflow.
      }
      if (stop.load(std::memory_order_relaxed)) return finish(step_status::Cancelled, true);

      if (!native_prefix)
        response.blocks.erase(selected_end, response.blocks.end());
      if (stop.load(std::memory_order_relaxed)) return finish(step_status::Cancelled, true);
      // A complete tip requires exact validated content, not reported heights.
      const bool complete_response = !result.batch_truncated && selected_count == full_block_count &&
        height == response.current_height;
      const crypto::hash validated_last_hash = previous;
      // A complete already-known current tip needs no handler work only for a
      // clean, initialized native image whose daemon bookkeeping is already exact.
      // The full reply, selected anchor and every selected transaction were checked
      // above. Excluding genesis-only/fresh state preserves genesis/birthday work;
      // older anchors, reorgs, truncation and metadata changes keep the dirty path.
      if (m_wallet.m_native_file_mode && !fresh &&
          m_wallet.m_native_history_origin.phase == origin_phase::Complete && complete_response &&
          policy_hash(validated_last_hash) == m_wallet.m_native_history_origin.connected_hash &&
          !m_wallet.m_read_only_sync_dirty &&
          !m_wallet.m_read_only_sync_faulted && result.local_blocks_before > 1 &&
          !result.batch_truncated && full_block_count == 1 && selected_count == 1 &&
          response.start_height == result.local_blocks_before - 1 &&
          response.current_height == result.local_blocks_before &&
          m_wallet.m_last_known_daemon_height == response.current_height)
      {
        if (stop.load(std::memory_order_relaxed)) return finish(step_status::Cancelled, true);
        return finish(step_status::NoProgress);
      }
      // Legacy processing mutates incrementally, even on a later exception.
      // Never claim rollback, unchanged release, or completed persistence.
      result.state_may_have_changed = m_wallet.m_read_only_sync_dirty = true;
      const auto stamp_bootstrap_origin = [&]() {
        const auto& boundary = *m_birthday_boundary;
        native_history_origin origin;
        origin.phase = origin_phase::Scanning;
        origin.rounded_date = m_birthday_date;
        origin.first_height = boundary.candidate.height;
        origin.anchor_hash = boundary.predecessor.hash;
        origin.view = boundary.view;
        origin.connected_height = m_wallet.get_blockchain_current_size() - 1;
        origin.connected_hash = policy_hash(origin.connected_height == 0 ?
          m_expected_genesis : m_wallet.m_chain.get_top_block_id());
        m_wallet.m_native_history_origin = origin;
        m_birthday_committed = true;
      };
      if (fresh)
      {
        if (m_birthday_minimum > 0)
        {
          m_wallet.m_minimum_height = m_birthday_minimum;
          // Exact H-1 and H identities, the full R3 reply and selected transactions
          // were checked before inserting the proven virgin sparse anchor.
          const auto& anchor = response.blocks.front();
          m_wallet.process_new_blockchain_entry(anchor.block_ptr->bl, anchor,
            native_hash(m_birthday_boundary->predecessor.hash), m_birthday_minimum);
          ++result.blocks_added;
        }
        else
        {
          // H=1 processing reads the minimum. Set zero only immediately before
          // the existing real handler, after a fresh stop poll. A raced stop
          // can still leave no H=1 progress; the post-handler guard faults that
          // possibly changed state instead of claiming rollback or an origin.
          if (stop.load(std::memory_order_relaxed))
          {
            m_wallet.m_read_only_sync_faulted = true;
            return finish(step_status::Cancelled);
          }
          m_wallet.m_minimum_height = 0;
        }
        if (m_birthday_minimum > 0)
          stamp_bootstrap_origin(); // Only actual successful anchor mutation creates origin.
      }
      else m_wallet.m_native_history_origin.phase = origin_phase::Scanning;
      bool reset_needed = false;
      m_wallet.handle_pulled_blocks(result.blocks_added, stop, response, reset_needed, true);
      if (m_wallet.m_read_only_sync_faulted || reset_needed || m_proxy->denied() ||
          m_wallet.m_chain.get_genesis() != m_expected_genesis)
      {
        m_wallet.m_read_only_sync_faulted = true;
        return finish(step_status::RecoveryRequired);
      }
      // A successful handler may stop after a genuine contiguous partial batch.
      // Update its actual connected tip before returning cancellation so clean
      // R3 dirty progress remains checkpointable as Scanning.
      const uint64_t connected_blocks = m_wallet.get_blockchain_current_size();
      if (fresh && !m_birthday_committed)
      {
        // H=1 uses real genesis handling and must actually process H first.
        if (!result.blocks_added || connected_blocks <= m_birthday_boundary->candidate.height)
        {
          // The handler may have processed genesis/bookkeeping without adding
          // H=1, even when stop raced its entry. No mutation-free rollback is
          // proved: preserve dirty/fault honesty and never fabricate an origin.
          m_wallet.m_read_only_sync_faulted = true;
          return finish(stop.load(std::memory_order_relaxed) ? step_status::Cancelled :
            step_status::RecoveryRequired);
        }
        stamp_bootstrap_origin();
      }
      auto& origin = m_wallet.m_native_history_origin;
      if (!connected_blocks || connected_blocks - 1 < origin.first_height - 1)
      {
        m_wallet.m_read_only_sync_faulted = true;
        return finish(step_status::RecoveryRequired);
      }
      origin.connected_height = connected_blocks - 1;
      origin.connected_hash = policy_hash(connected_blocks == 1 ?
        m_expected_genesis : m_wallet.m_chain.get_top_block_id());
      origin.phase = origin_phase::Scanning;
      if (!history_origin_valid())
      {
        m_wallet.m_read_only_sync_faulted = true;
        return finish(step_status::RecoveryRequired);
      }
      // The protected origin now describes the actual partial tip. Discard
      // transient proof state even when this successful batch was cancelled.
      m_birthday_boundary.reset();
      m_birthday_view = {};
      if (stop.load(std::memory_order_relaxed)) return finish(step_status::Cancelled);
      if (complete_response && connected_blocks == response.current_height &&
          origin.connected_hash == policy_hash(validated_last_hash) &&
          origin.connected_height >= origin.view.target_height)
        origin.phase = origin_phase::Complete;
      if (stop.load(std::memory_order_relaxed))
      {
        origin.phase = origin_phase::Scanning;
        return finish(step_status::Cancelled);
      }
      return finish(result.blocks_added ? step_status::Progress : step_status::NoProgress);
    }
    catch (...)
    {
      if (result.state_may_have_changed)
      {
        m_wallet.m_read_only_sync_faulted = true;
        return finish(step_status::RecoveryRequired);
      }
      return finish(step_status::Unavailable);
    }
  }
}}
