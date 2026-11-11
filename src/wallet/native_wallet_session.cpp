// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#include "native_wallet_session.h"
#include "native_payment_transport.h"
#include "wallet2.h"
#include "currency_core/account.h"
#include "currency_core/currency_config.h"
#include "currency_core/currency_format_utils.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <functional>
#include <limits>
#include <new>
#include <utility>
#include <vector>

namespace tools { namespace native_session
{
  namespace
  {
    // Process-wide; a generation is handed out at most once and never wraps.
    std::atomic<uint64_t> last_generation{0};
    bool next_generation(uint64_t& out) noexcept
    {
      uint64_t current = last_generation.load();
      do
      {
        if (current == std::numeric_limits<uint64_t>::max()) return false;
      } while (!last_generation.compare_exchange_weak(current, current + 1));
      out = current + 1;
      return true;
    }

    bool hash_less(const crypto::hash& a, const crypto::hash& b) noexcept
    {
      return std::memcmp(&a, &b, sizeof(a)) < 0;
    }

    // Order/identity key of one retained record, valid for one build only.
    struct activity_key
    {
      const wallet_public::wallet_transfer_info* record = nullptr;
      bool pending = false;
      uint64_t timestamp = 0;
      uint64_t height = 0;
      crypto::hash hash{};
    };

    // Pending first; newest recorded timestamp; highest height; hash bytes.
    bool newer_first(const activity_key& a, const activity_key& b) noexcept
    {
      if (a.pending != b.pending) return a.pending;
      if (a.timestamp != b.timestamp) return a.timestamp > b.timestamp;
      if (a.height != b.height) return a.height > b.height;
      return hash_less(a.hash, b.hash);
    }

    bool take_nested(std::size_t& nested, std::size_t count) noexcept
    {
      if (nested > max_snapshot_nested_elements || count > max_snapshot_nested_elements - nested)
        return false;
      nested += count;
      return true;
    }
#if !defined(TESTNET) && CURRENCY_FORMATION_VERSION == 84
    constexpr bool profile_supported = true;
    constexpr compiled_network profile_network = compiled_network::Mainnet;
#elif defined(TESTNET) && CURRENCY_FORMATION_VERSION == 103
    constexpr bool profile_supported = true;
    constexpr compiled_network profile_network = compiled_network::Testnet;
#else
    constexpr bool profile_supported = false;
    constexpr compiled_network profile_network = compiled_network::Mainnet;
#endif
    static_assert(profile_supported == strict_compiled_formation_supported,
      "descriptor and strict loader must admit the same compiled profiles");

    open_result translate(offline::open_result result) noexcept
    {
      switch (result)
      {
        case offline::open_result::InvalidMaterial: return open_result::InvalidMaterial;
        case offline::open_result::OccupiedOutput: return open_result::OccupiedOutput;
        case offline::open_result::NotFound: return open_result::NotFound;
        case offline::open_result::InvalidFile: return open_result::InvalidFile;
        case offline::open_result::WrongPassword: return open_result::WrongPassword;
        case offline::open_result::UnsupportedCapability: return open_result::UnsupportedCapability;
        case offline::open_result::IdentityMismatch: return open_result::IdentityMismatch;
        case offline::open_result::GenesisMismatch: return open_result::GenesisMismatch;
        case offline::open_result::RecoveryRequired: return open_result::RecoveryRequired;
        default: return open_result::Unavailable;
      }
    }

    constexpr std::size_t max_backup_phrase_bytes = 1024;

    void wipe(std::string& value) noexcept
    {
      if (!value.empty())
      {
        volatile char* bytes = &value[0];
        for (std::size_t i = 0; i != value.size(); ++i) bytes[i] = 0;
      }
      value.clear();
    }

    // Wipes a core-returned phrase copy on every exit path.
    struct wiped_phrase final
    {
      std::string value;
      wiped_phrase() = default;
      ~wiped_phrase() noexcept { wipe(value); }
      wiped_phrase(const wiped_phrase&) = delete;
      wiped_phrase& operator=(const wiped_phrase&) = delete;
    };

    std::size_t phrase_words(const std::string& phrase) noexcept
    {
      std::size_t count = 0;
      bool in_word = false;
      for (const unsigned char c : phrase)
      {
        const bool space = c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
        if (!space && !in_word) ++count;
        in_word = !space;
      }
      return count;
    }

    bool same_keys(const public_identity& left, const public_identity& right) noexcept
    {
      return left.spend_public_key == right.spend_public_key &&
        left.view_public_key == right.view_public_key && left.flags == right.flags;
    }

    // Any value other than a definite Committed/Rejected is Unknown.
    commit_status checked(commit_status value) noexcept
    {
      switch (value)
      {
        case commit_status::Committed:
        case commit_status::Rejected: return value;
        case commit_status::Unknown: break;
      }
      return commit_status::Unknown;
    }

    // Marks the session as inside a step/checkpoint for reentrancy refusal.
    struct busy_scope final
    {
      bool& busy;
      explicit busy_scope(bool& flag) noexcept : busy(flag) { busy = true; }
      ~busy_scope() noexcept { busy = false; }
      busy_scope(const busy_scope&) = delete;
      busy_scope& operator=(const busy_scope&) = delete;
    };
  }

  compiled_profile derive_build_profile() noexcept
  {
    // Generation prints the hash; keep every diagnostic out of the log.
    epee::log_space::scoped_log_silence quiet;
    compiled_profile result;
    if (!profile_supported)
    {
      result.status = profile_status::Unsupported;
      return result;
    }
    try
    {
      // Compiled genesis transaction and fixed block inputs only. Never the
      // get_genesis_hash() cache, which a legacy caller may override.
      currency::block genesis{};
      crypto::hash id = currency::null_hash;
      if (!currency::generate_genesis_block(genesis) ||
          genesis.nonce != CURRENCY_GENESIS_NONCE || genesis.timestamp != 0 ||
          !currency::get_block_hash(genesis, id) || id == currency::null_hash)
        return result;
      // MAINNET also has an unconditional source literal; require agreement.
      // No TESTNET literal is invented here.
      if (profile_network == compiled_network::Mainnet && id != currency::gdefault_genesis)
        return result;
      result.network = profile_network;
      result.formation = CURRENCY_FORMATION_VERSION;
      result.genesis = id;
      result.status = profile_status::Available;
      return result;
    }
    catch (...) { return compiled_profile{}; } // never inspect exception text
  }

  const compiled_profile& build_profile() noexcept
  {
    // Initialized once; no setter exists and failure is not retried.
    static const compiled_profile profile = derive_build_profile();
    return profile;
  }

  void file_session::destroy_owned() noexcept
  {
    epee::log_space::scoped_log_silence quiet;
    // The core must die while the provider still excludes competing owners and
    // retains the material. Explicit resets keep the guard around every
    // teardown operation, including provider destruction. The scanner borrows
    // the wallet, so it goes first. It may borrow replay scratch, which in turn
    // goes before the authoritative wallet and the material.
    m_sync.reset();
    m_replay_wallet.reset();
    m_replay_ready = false;
    m_wallet.reset();
    m_material.reset();
  }

  void file_session::release_owned() noexcept
  {
    destroy_owned();
    m_address.clear();
    m_identity = {};
    m_state = session_state::Released;
  }

  bool file_session::authorized() const noexcept
  {
    // Sticky: a provider that reports current again cannot restore authority.
    if (m_revoked) return false;
    if (!m_material || !m_material->authorization_current())
    {
      m_revoked = true;
      return false;
    }
    return true;
  }

  void file_session::observe_wallet() noexcept
  {
    if (!m_wallet) return;
    if (m_state != session_state::OpenUnchanged && m_state != session_state::OpenDirty) return;
    if (m_wallet->m_read_only_sync_faulted) m_state = session_state::RecoveryRequired;
    // A reconciled ready replay image is unsaved progress awaiting a Committed checkpoint.
    else m_state = (m_wallet->m_read_only_sync_dirty || m_replay_ready)
      ? session_state::OpenDirty : session_state::OpenUnchanged;
  }

  void file_session::mark_recovery() noexcept
  {
    // Latched on the wallet too, so no later scanner or legacy store path can
    // treat this state as authoritative.
    if (m_wallet) m_wallet->m_read_only_sync_faulted = true;
    m_state = session_state::RecoveryRequired;
  }

  void file_session::advance_revision() noexcept
  {
    // Never wraps: an exhausted counter can no longer distinguish states.
    if (m_revision == std::numeric_limits<uint64_t>::max()) m_revision_exhausted = true;
    else ++m_revision;
  }

  file_session::~file_session() noexcept { destroy_owned(); }

  open_result file_session::acquire(acquisition_kind kind,
    std::unique_ptr<material_lease>& material,
    std::unique_ptr<file_session>& out) noexcept
  {
    epee::log_space::scoped_log_silence quiet;
    if (out) return open_result::OccupiedOutput;
    if (kind != acquisition_kind::OpenExisting) return open_result::UnsupportedCapability;
    if (!material) return open_result::InvalidMaterial;
    if (!material->authorization_current()) return open_result::AuthorizationUnavailable;
    // The immutable compiled descriptor is checked before lease material is read.
    const auto& profile = build_profile();
    if (profile.status == profile_status::Unsupported) return open_result::UnsupportedCapability;
    if (profile.status != profile_status::Available) return open_result::Unavailable;
    try
    {
      const auto& input = material->material();
      if (input.path.empty() || input.expected.stored_genesis == currency::null_hash)
        return open_result::InvalidMaterial;
      // Only the compiled profile's own chain. Never adopt an endpoint's or the
      // file's genesis or let a mismatched registry select another chain.
      if (input.expected.stored_genesis != profile.genesis)
        return open_result::GenesisMismatch;
      if (!material->authorization_current()) return open_result::AuthorizationUnavailable;
      std::unique_ptr<file_session> candidate(new file_session());
      candidate->m_wallet.reset(new wallet2(wallet2::native_file_construction_tag{}));
      candidate->m_wallet->load_strict_current_file(input.path, input.password);

      const auto& address = candidate->m_wallet->m_account.get_public_address();
      if (address.spend_public_key != input.expected.spend_public_key ||
          address.view_public_key != input.expected.view_public_key ||
          address.flags != input.expected.flags)
        return open_result::IdentityMismatch;
      if (candidate->m_wallet->m_chain.get_genesis() != input.expected.stored_genesis ||
          candidate->m_wallet->m_chain.get_genesis() != profile.genesis)
        return open_result::GenesisMismatch;
      if ((address.flags & ~ACCOUNT_PUBLIC_ADDRESS_FLAG_AUDITABLE) != 0)
        return open_result::UnsupportedCapability;
      candidate->m_address = currency::get_account_address_as_str(address);
      currency::account_public_address parsed{};
      currency::payment_id_t payment_id;
      if (candidate->m_address.empty() ||
          !currency::get_account_address_and_payment_id_from_str(parsed, payment_id, candidate->m_address) ||
          !payment_id.empty() || parsed.spend_public_key != address.spend_public_key ||
          parsed.view_public_key != address.view_public_key || parsed.flags != address.flags ||
          currency::get_account_address_as_str(parsed) != candidate->m_address)
        return open_result::InvalidAddress;
      if (!material->authorization_current()) return open_result::AuthorizationUnavailable;
      if (!next_generation(candidate->m_generation)) return open_result::Unavailable;
      candidate->m_identity = {address.spend_public_key, address.view_public_key, address.flags,
        candidate->m_wallet->m_chain.get_genesis()};
      const auto holds = material->held_inputs();
      if (holds.status != held_inputs_status::Loaded && holds.status != held_inputs_status::Unsupported)
        return open_result::RecoveryRequired;
      if (holds.inputs.size() > CURRENCY_TX_MAX_ALLOWED_INPUTS * 128)
        return open_result::RecoveryRequired;
      candidate->m_wallet->m_native_held_inputs = holds.inputs;
      candidate->m_state = session_state::OpenUnchanged;
      candidate->m_material = std::move(material);
      out = std::move(candidate);
      return open_result::Opened;
    }
    catch (offline::open_result result) { return translate(result); }
    catch (const error::invalid_password&) { return open_result::WrongPassword; }
    catch (...) { return open_result::Unavailable; } // never inspect exception text
  }

  receive_result file_session::get_receive() const noexcept
  {
    epee::log_space::scoped_log_silence quiet;
    const auto current = state();
    if ((current != session_state::OpenUnchanged && current != session_state::OpenDirty) ||
        m_busy || !m_wallet || !m_material || !authorized()) return {};
    try
    {
      receive_result result{receive_status::Available, m_address, m_identity};
      if (!authorized()) return {};
      return result;
    }
    catch (...) { return {}; } // allocation failure retains the definite session
  }

  backup_result file_session::present_backup(const backup_admission& admission,
    backup_visitor visit, void* context) const noexcept
  {
    epee::log_space::scoped_log_silence quiet;
    if (!visit) return backup_result::InvalidVisitor;
    const auto current = state();
    if ((current != session_state::OpenUnchanged && current != session_state::OpenDirty) ||
        m_busy || !m_wallet || !m_material)
      return backup_result::Unavailable;
    // The reveal needs its own fresh admission AND the session's authority.
    if (!admission.admission_current() || !authorized())
      return backup_result::Revoked;
    try
    {
      // The admission names the account it was granted for; it must be this
      // session's independently verified identity (cross-account refusal).
      const public_identity& subject = admission.subject();
      if (!same_keys(subject, m_identity) || subject.stored_genesis != m_identity.stored_genesis)
        return backup_result::IdentityMismatch;
      const auto& account = m_wallet->m_account;
      const auto& address = account.get_public_address();
      if (address.spend_public_key != m_identity.spend_public_key ||
          address.view_public_key != m_identity.view_public_key || address.flags != m_identity.flags)
        return backup_result::IdentityMismatch;
      if (account.is_watch_only() || address.flags != 0) return backup_result::Unsupported;

      wiped_phrase phrase;
      phrase.value = account.get_seed_phrase(std::string());
      if (phrase.value.empty()) return backup_result::NoSeed;
      if (phrase.value.size() > max_backup_phrase_bytes ||
          phrase_words(phrase.value) != SEED_PHRASE_V2_WORDS_COUNT)
        return backup_result::Unavailable;
      {
        // Independent round trip through the unchanged native parser: the
        // rendered unprotected V2 text must recover exactly these keys.
        currency::account_base recovered;
        if (!recovered.restore_from_seed_phrase(phrase.value, std::string()))
          return backup_result::IdentityMismatch;
        const auto& check = recovered.get_public_address();
        if (check.spend_public_key != m_identity.spend_public_key ||
            check.view_public_key != m_identity.view_public_key || check.flags != m_identity.flags)
          return backup_result::IdentityMismatch;
      }
      // Sample both authorities again immediately before exposure. The provider
      // must still order this against platform invalidation (see header).
      if (!admission.admission_current() || !authorized())
        return backup_result::Revoked;
      visit(context, phrase.value.data(), phrase.value.size());
      return backup_result::Presented;
    }
    catch (...) { return backup_result::Unavailable; } // never inspect exception text
  }

  release_result file_session::release_unchanged() noexcept
  {
    epee::log_space::scoped_log_silence quiet;
    if (m_state == session_state::Released) return release_result::AlreadyReleased;
    // Never from inside a running step/checkpoint, and never for a wallet with
    // unsaved progress, a latched fault or an unresolved checkpoint outcome.
    if (m_busy) return release_result::Unavailable;
    observe_wallet();
    if (m_state != session_state::OpenUnchanged || !m_wallet || !m_material ||
        m_wallet->m_read_only_sync_dirty || m_wallet->m_read_only_sync_faulted)
      return release_result::Unavailable;
    // Authorization loss prevents reads, not resource release. Unchanged means
    // unchanged since the last acknowledged file (open or Committed checkpoint).
    release_owned();
    return release_result::ReleasedUnchanged;
  }

  session_state file_session::state() const noexcept
  {
    // Derived from the wallet's sticky observations, so a dirty or faulted
    // wallet is never reported as OpenUnchanged between calls.
    if ((m_state == session_state::OpenUnchanged || m_state == session_state::OpenDirty) && m_wallet)
    {
      if (m_wallet->m_read_only_sync_faulted) return session_state::RecoveryRequired;
      if (m_wallet->m_read_only_sync_dirty || m_replay_ready) return session_state::OpenDirty;
      return session_state::OpenUnchanged;
    }
    return m_state;
  }

  sync_prepare_result file_session::prepare_sync(std::shared_ptr<i_core_proxy> transport,
    read_only::step_limits limits) noexcept
  {
    epee::log_space::scoped_log_silence quiet;
    if (m_busy) return sync_prepare_result::Busy;
    observe_wallet();
    if (m_state == session_state::RecoveryRequired || m_state == session_state::OutcomeUnknown)
      return sync_prepare_result::UnsupportedState;
    if ((m_state != session_state::OpenUnchanged && m_state != session_state::OpenDirty) ||
        !m_wallet || !m_material)
      return sync_prepare_result::Unavailable;
    // Authority and persistence capability come first: an incapable or revoked
    // provider can never start a mutation it could not later drain.
    if (!authorized()) return sync_prepare_result::Revoked;
    if (!m_material->checkpoint_supported()) return sync_prepare_result::CheckpointUnsupported;
    if (!transport) return sync_prepare_result::InvalidTransport;
    // A reconciled ready image is only checkpointed or closed, never replayed again.
    if (m_replay_ready) return sync_prepare_result::UnsupportedState;
    try
    {
      // Candidate construction samples authority; no callback may reenter.
      busy_scope guard(m_busy);
      const auto& profile = build_profile();
      if (profile.status != profile_status::Available || m_identity.stored_genesis != profile.genesis ||
          m_wallet->m_chain.get_genesis() != m_identity.stored_genesis)
        return sync_prepare_result::UnsupportedState;
      if (m_wallet->m_watch_only || m_wallet->m_read_only_sync_active)
        return sync_prepare_result::UnsupportedState;
      // Zero-minimum images keep all recorded coverage. Only a clean, untouched
      // genesis image prepares a new birthday. Strictly admitted positive saved
      // boundaries remain local wallet policy, independent of today's clock/node.
      const uint64_t minimum = m_wallet->m_minimum_height;
      if ((minimum == WALLET_MINIMUM_HEIGHT_UNSET_CONST && !m_wallet->native_birthday_fresh()) ||
          !m_wallet->native_scan_minimum_supported())
        return sync_prepare_result::UnsupportedState;
      const bool has_origin = m_wallet->m_native_history_origin.phase != read_only::origin_phase::Absent;
      if (has_origin && !m_wallet->native_history_origin_valid())
        return sync_prepare_result::UnsupportedState;
      // The scanner never advances a saved-minimum image without origin lineage.
      // Such an admitted legacy image is rebuilt in same-account transient
      // scratch; the admitted wallet stays authoritative and is never mutated.
      // A clean genesis image (unset minimum) stamps its own origin directly.
      const bool legacy = !has_origin && minimum != WALLET_MINIMUM_HEIGHT_UNSET_CONST;
      wallet2* target = m_wallet.get();
      std::unique_ptr<wallet2> next_replay;
      bool drop_replay = false;
      if (legacy)
      {
        // Only explicit preparation may replace unusable recomputable scratch.
        // The admitted wallet's dirty/fault observations are never cleared.
        drop_replay = m_replay_wallet && (m_replay_wallet->m_read_only_sync_faulted ||
          (!m_replay_wallet->native_birthday_fresh() && !m_replay_wallet->native_history_origin_valid()));
        if (m_replay_wallet && !drop_replay) target = m_replay_wallet.get(); // resume its in-memory lineage
        else
        {
          const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
          const std::function<bool()> current = [this, deadline] {
            return std::chrono::steady_clock::now() < deadline && authorized();
          };
          next_replay = m_wallet->make_native_history_candidate(m_identity.stored_genesis, current);
          if (!next_replay)
          {
            // Unusable nonready scratch is still disposed (scanner first).
            // Original state, material and lease are untouched.
            if (drop_replay) { m_sync.reset(); m_replay_wallet.reset(); }
            return sync_prepare_result::UnsupportedState;
          }
          target = next_replay.get();
        }
      }
      else drop_replay = static_cast<bool>(m_replay_wallet); // nonready; no longer this image's path
      // Allocate before touching any wallet. The previous scanner (if any) is
      // not running (m_busy was false) and owns no wallet state.
      std::unique_ptr<read_only::block_sync> next(new read_only::block_sync(*target,
        m_identity.stored_genesis, std::move(transport), limits));
      m_sync.reset(); // the scanner dies before scratch it may borrow
      if (drop_replay) m_replay_wallet.reset();
      if (next_replay)
      {
        m_replay_wallet = std::move(next_replay);
        // Previously prepared payments/cursors cannot survive a replay boundary.
        advance_revision();
      }
      m_sync = std::move(next);
      return sync_prepare_result::Prepared;
    }
    catch (...) { return sync_prepare_result::Unavailable; } // never inspect exception text
  }

  sync_step_result file_session::step_sync(std::atomic<bool>& stop) noexcept
  {
    epee::log_space::scoped_log_silence quiet;
    sync_step_result result;
    if (m_busy)
    {
      result.status = sync_step_status::Busy;
      return result;
    }
    observe_wallet();
    switch (m_state)
    {
      case session_state::OpenUnchanged:
      case session_state::OpenDirty: break;
      case session_state::RecoveryRequired: result.status = sync_step_status::RecoveryRequired; return result;
      case session_state::OutcomeUnknown: result.status = sync_step_status::OutcomeUnknown; return result;
      default: return result;
    }
    if (!m_wallet || !m_material) return result;
    if (!m_sync)
    {
      result.status = sync_step_status::NotPrepared;
      return result;
    }
    if (!authorized())
    {
      result.status = sync_step_status::Revoked;
      return result;
    }
    if (!m_material->checkpoint_supported())
    {
      result.status = sync_step_status::NotPrepared;
      return result;
    }
    if (m_replay_ready)
    {
      // Only an explicit checkpoint/close may replace the admitted image.
      result.scan.status = read_only::step_status::PreparationProgress;
      result.scan.local_blocks_before = result.scan.local_blocks_after =
        m_wallet->m_chain.get_blockchain_current_size();
      result.status = authorized() ? sync_step_status::Stepped : sync_step_status::Revoked;
      return result;
    }
    {
      // Reentrant calls (transport callbacks) are refused for the whole step;
      // nothing can destroy the wallet, scanner or lease underneath it.
      busy_scope guard(m_busy);
      bool discard_replay = false;
      if (m_replay_wallet &&
          m_replay_wallet->m_native_history_origin.phase == read_only::origin_phase::Complete &&
          !m_replay_wallet->m_read_only_sync_faulted)
      {
        // Reconciliation is its own bounded preparation step with zero RPCs.
        // It never adds a second local work budget to a network step.
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        const std::function<bool()> current = [this, &stop, deadline] {
          return !stop.load(std::memory_order_relaxed) &&
            std::chrono::steady_clock::now() < deadline && authorized();
        };
        if (!m_replay_wallet->reconcile_native_history_candidate(*m_wallet, m_identity.stored_genesis, current))
        {
          // Only recomputable scratch may have changed. The admitted original
          // remains exact and may prepare a new read explicitly; no retry here.
          discard_replay = true;
          result.scan.status = stop.load(std::memory_order_relaxed)
            ? read_only::step_status::Cancelled : read_only::step_status::RecoveryRequired;
        }
        else
        {
          m_replay_ready = true;
          m_replay_wallet->m_read_only_sync_dirty = true;
          result.scan.status = read_only::step_status::PreparationProgress;
        }
      }
      else result.scan = m_sync->step(stop);
      if (m_replay_wallet)
      {
        // A fresh (never mutated) candidate whose scanner refused cannot advance
        // under that scanner. It is nothing but recomputable empty scratch.
        const auto raw = result.scan.status;
        const bool fresh_refused = !m_replay_ready && m_replay_wallet->native_birthday_fresh() &&
          raw != read_only::step_status::PreparationProgress && raw != read_only::step_status::Cancelled;
        // Existing result/capture contracts describe the admitted wallet only.
        // Scratch never publishes rows, amounts, tip or provenance through them.
        result.scan.state_may_have_changed = false;
        result.scan.local_blocks_before = result.scan.local_blocks_after =
          m_wallet->m_chain.get_blockchain_current_size();
        result.scan.blocks_added = 0;
        result.scan.batch_truncated = false;
        result.scan.reported_daemon_blocks = 0;
        result.scan.has_daemon_observation = false;
        if (raw == read_only::step_status::Progress || raw == read_only::step_status::NoProgress)
          result.scan.status = read_only::step_status::PreparationProgress;
        if (!m_replay_ready && (discard_replay || fresh_refused || m_replay_wallet->m_read_only_sync_faulted ||
            (!m_replay_wallet->native_birthday_fresh() && !m_replay_wallet->native_history_origin_valid())))
        {
          // No protected replacement was attempted. Dispose only unusable
          // nonready scratch (scanner first), preserving the original wallet,
          // material and lease. A ready image survives Rejected/Unknown outcomes.
          m_sync.reset();
          m_replay_wallet.reset();
        }
      }
    }
    // Before any output can be captured, whatever the step status (cancelled,
    // failed, equal height, reorg, revoked): a possible change is a new state.
    if (result.scan.state_may_have_changed) advance_revision();
    observe_wallet();
    // A step that completes after revocation never regains authority. Its
    // observation is still returned so the owner knows it must drain.
    result.status = authorized() ? sync_step_status::Stepped : sync_step_status::Revoked;
    return result;
  }

  checkpoint_result file_session::checkpoint() noexcept
  {
    epee::log_space::scoped_log_silence quiet;
    if (m_busy) return checkpoint_result::Busy;
    return write_checkpoint();
  }

  checkpoint_result file_session::write_checkpoint() noexcept
  {
    observe_wallet();
    switch (m_state)
    {
      case session_state::OpenUnchanged:
      case session_state::OpenDirty: break;
      case session_state::RecoveryRequired: return checkpoint_result::RecoveryRequired;
      case session_state::OutcomeUnknown: return checkpoint_result::OutcomeUnknown;
      default: return checkpoint_result::Unavailable;
    }
    if (!m_wallet || !m_material) return checkpoint_result::Unavailable;
    // Only a reconciled ready replay image replaces the admitted file; unready
    // scratch is never rendered. Otherwise the authoritative wallet is saved.
    wallet2* const checkpoint_wallet = m_replay_ready ? m_replay_wallet.get() : m_wallet.get();
    if (!checkpoint_wallet) return checkpoint_result::Unavailable;
    if (!checkpoint_wallet->m_read_only_sync_dirty) return checkpoint_result::Unchanged;
    if (!m_material->checkpoint_supported()) return checkpoint_result::Unsupported;
    // Drain authority: deliberately no authorization sample. The already-held
    // material and writer may save clean progress after revocation; nothing
    // is published, unsealed or re-admitted here.
    std::string image;
    wallet2::native_image_status rendered = wallet2::native_image_status::Unavailable;
    try
    {
      busy_scope guard(m_busy);
      const auto& profile = build_profile();
      const auto& input = m_material->material();
      if (profile.status != profile_status::Available || m_identity.stored_genesis != profile.genesis ||
          !same_keys(input.expected, m_identity) || input.expected.stored_genesis != m_identity.stored_genesis)
        rendered = wallet2::native_image_status::IdentityMismatch;
      else
      {
        currency::account_public_address expected{};
        expected.spend_public_key = m_identity.spend_public_key;
        expected.view_public_key = m_identity.view_public_key;
        expected.flags = m_identity.flags;
        rendered = checkpoint_wallet->render_native_checkpoint_image(expected, m_identity.stored_genesis,
          input.password, m_image_limit, image);
      }
    }
    catch (...) { rendered = wallet2::native_image_status::Unavailable; } // target untouched
    switch (rendered)
    {
      case wallet2::native_image_status::Rendered: break;
      case wallet2::native_image_status::LimitExceeded: return checkpoint_result::ImageLimit;
      case wallet2::native_image_status::IdentityMismatch:
        mark_recovery();
        return checkpoint_result::IdentityMismatch;
      default: return checkpoint_result::Unavailable;
    }

    commit_status written = commit_status::Unknown;
    {
      busy_scope guard(m_busy);
      try { written = checked(m_material->replace_checkpoint(m_identity, image.data(), image.size())); }
      catch (...) { written = commit_status::Unknown; } // never reinterpreted as a safe failure
    }
    image.clear();
    switch (written)
    {
      case commit_status::Committed:
        // The image was rendered from exactly this state on this executor and
        // nothing ran in between: the file now holds it.
        if (m_replay_ready)
        {
          // Only now, after the provider's positive commit, does the candidate
          // become authoritative. No allocation/callback here; the scanner
          // still borrows the same object. The replaced original dies after it.
          m_wallet.swap(m_replay_wallet);
          m_replay_wallet.reset();
          m_replay_ready = false;
          advance_revision(); // every older authoritative capture is stale
        }
        m_wallet->m_read_only_sync_dirty = false;
        m_state = session_state::OpenUnchanged;
        return checkpoint_result::Saved;
      case commit_status::Rejected:
        // Dirty/ready image, original, scanner and material retained.
        return checkpoint_result::Rejected;
      default:
        m_state = session_state::OutcomeUnknown;
        return checkpoint_result::OutcomeUnknown;
    }
  }

  close_result file_session::close() noexcept
  {
    epee::log_space::scoped_log_silence quiet;
    if (m_state == session_state::Released) return close_result::AlreadyReleased;
    if (m_busy) return close_result::Busy;
    observe_wallet();
    switch (m_state)
    {
      case session_state::OpenUnchanged:
      case session_state::OpenDirty: break;
      case session_state::RecoveryRequired: return close_result::RecoveryRequired;
      case session_state::OutcomeUnknown: return close_result::OutcomeUnknown;
      default: return close_result::Unavailable;
    }
    if (!m_wallet || !m_material) return close_result::Unavailable;
    bool saved = false;
    if (m_replay_wallet && !m_replay_ready)
    {
      // Only recomputable transient scratch is abandoned (scanner first). The
      // admitted wallet, its dirty progress, pending metadata and independent
      // Held claims were never changed. Not a reset of a protected wallet or retry.
      m_sync.reset();
      m_replay_wallet.reset();
    }
    if (m_wallet->m_read_only_sync_dirty || m_replay_ready)
    {
      // Normal close saves authoritative progress or a reconciled ready image;
      // it never discards progress for a later rescan.
      switch (write_checkpoint())
      {
        case checkpoint_result::Saved: saved = true; break;
        case checkpoint_result::OutcomeUnknown: return close_result::OutcomeUnknown;
        case checkpoint_result::RecoveryRequired:
        case checkpoint_result::IdentityMismatch: return close_result::RecoveryRequired;
        case checkpoint_result::Busy: return close_result::Busy;
        default: return close_result::RetainedFailure;
      }
    }
    release_owned();
    return saved ? close_result::ReleasedSaved : close_result::ReleasedUnchanged;
  }

  signed_payment::~signed_payment() noexcept
  {
    volatile unsigned char* key = reinterpret_cast<volatile unsigned char*>(&transaction_key);
    for (std::size_t i = 0; i < sizeof(transaction_key); ++i) key[i] = 0;
    volatile char* bytes = transaction_blob.empty() ? nullptr : &transaction_blob[0];
    for (std::size_t i = 0; i < transaction_blob.size(); ++i) bytes[i] = 0;
  }

  held_payment_limits signing_limits() noexcept
  {
    return {CURRENCY_MAX_TRANSACTION_BLOB_SIZE, CURRENCY_TX_MAX_ALLOWED_INPUTS,
      max_checkpoint_image_bytes};
  }

  struct file_session::prepared_payment::data {
    currency::finalize_tx_param parameters{};
    public_identity identity{};
    std::string saved_draft, recipient;
    std::vector<held_input> inputs;
    uint64_t generation = 0, revision = 0, amount = 0, fee = 0, local_blocks = 0;
    uint64_t confidential_decoys = 0, bare_decoys = 0;
    crypto::hash tip{};
    ~data() noexcept {
      for (auto& source : parameters.sources) {
        volatile unsigned char* a = reinterpret_cast<volatile unsigned char*>(&source.real_out_amount_blinding_mask);
        volatile unsigned char* b = reinterpret_cast<volatile unsigned char*>(&source.real_out_asset_id_blinding_mask);
        for (std::size_t n = 0; n < sizeof(source.real_out_amount_blinding_mask); ++n) a[n] = 0;
        for (std::size_t n = 0; n < sizeof(source.real_out_asset_id_blinding_mask); ++n) b[n] = 0;
      }
    }
  };
  file_session::prepared_payment::prepared_payment(std::unique_ptr<data> d) : m_data(std::move(d)) {}
  file_session::prepared_payment::~prepared_payment() noexcept = default;
  const std::string& file_session::prepared_payment::recipient() const noexcept { return m_data->recipient; }
  uint64_t file_session::prepared_payment::amount() const noexcept { return m_data->amount; }
  uint64_t file_session::prepared_payment::fee() const noexcept { return m_data->fee; }
  uint64_t file_session::prepared_payment::local_blocks() const noexcept { return m_data->local_blocks; }
  uint64_t file_session::prepared_payment::input_count() const noexcept { return m_data->inputs.size(); }

  file_session::prepare_result file_session::prepare_payment_review(
    const payment_request& supplied, const std::string& supplied_draft,
    std::shared_ptr<native_payment::decoy_exchange> exchange, std::atomic<bool>& stop,
    native_payment::construction_detail* construction) noexcept {
    epee::log_space::scoped_log_silence quiet;
    prepare_result out;
    using detail = native_payment::construction_detail;
    if (construction) *construction = detail::None;
    detail first_failure = detail::None;
    detail exception_stage = detail::PreparationException;
    const auto refuse = [&out](sign_status status) { out.status = status; return out; };
    const auto failed = [&](detail fallback) {
      if (construction) *construction = first_failure == detail::None ? fallback : first_failure;
      return refuse(sign_status::PreparationFailed);
    };
    // Replay scratch (ready or not) carries no payment authority; the original
    // must not gain pending state that reconciliation could not honour.
    if (m_busy || m_replay_wallet) return refuse(sign_status::Busy);
    if (supplied.recipient.size() > 256 || supplied_draft.empty() || supplied_draft.size() > 4096)
      return refuse(sign_status::InvalidRequest);
    try {
      const payment_request request = supplied; const std::string saved_draft = supplied_draft;
      if (!authorized() || stop.load()) return refuse(sign_status::Revoked);
      // Advisory is re-evaluated here, never used as a transferable permission.
      auto advisory = assess_local_payment(request);
      if (advisory.status != payment_status::LocalAdvisory || !advisory.assessment) {
        if (advisory.status == payment_status::StaleObservation) return refuse(sign_status::StaleObservation);
        if (advisory.status == payment_status::Revoked) return refuse(sign_status::Revoked);
        if (advisory.status == payment_status::OutcomeUnknown) return refuse(sign_status::OutcomeUnknown);
        if (advisory.status == payment_status::RecoveryRequired) return refuse(sign_status::RecoveryRequired);
        return refuse(sign_status::InvalidRequest);
      }
      const auto& assessment = *advisory.assessment;
      if (!assessment.local_capacity_covers_amount_and_fee) return failed(detail::LocalCoverage);
      if (stop.load()) return failed(detail::StoppedBeforeConstruction);
      if (!exchange) return failed(detail::MissingExchange);
      if (!m_material || !m_material->signed_payment_supported() ||
          !m_material->checkpoint_supported()) return refuse(sign_status::Unsupported);
      if (!authorized())
        return refuse(sign_status::Revoked);
      if (request.expected_generation != m_generation || request.expected_revision != m_revision)
        return refuse(sign_status::StaleObservation);
      if (m_revision == std::numeric_limits<uint64_t>::max())
        return refuse(sign_status::Unsupported);
      busy_scope guard(m_busy);
      auto proxy = native_payment::make_payment_transport(std::move(exchange), assessment.local_blocks, stop, &first_failure);
      if (!proxy) return failed(detail::ProxyCreation);
      struct proxy_scope {
        std::shared_ptr<i_core_proxy>& slot;
        std::shared_ptr<i_core_proxy> prior;
        proxy_scope(std::shared_ptr<i_core_proxy>& value, std::shared_ptr<i_core_proxy> next)
          : slot(value), prior(value) { slot = std::move(next); }
        ~proxy_scope() { slot = std::move(prior); }
      };
      exception_stage = detail::ConstructionSetupException;
      auto ctp = m_wallet->get_default_construct_tx_param_inital();
      currency::account_public_address recipient{};
      currency::payment_id_t payment_id;
      if (!currency::get_account_address_and_payment_id_from_str(recipient, payment_id, assessment.canonical_recipient) ||
          !payment_id.empty() || recipient.flags != 0) return refuse(sign_status::InvalidRequest);
      ctp.dsts.emplace_back(request.amount, recipient, currency::native_coin_asset_id);
      ctp.fee = assessment.compiled_policy_fee;
      ctp.fake_outputs_count = assessment.bare_decoy_policy;
      ctp.unlock_time = 0;
      ctp.extra.clear(); ctp.attachments.clear();
      ctp.flags = 0; ctp.multisig_id = currency::null_hash;
      ctp.create_utxo_defragmentation_tx = false;
      ctp.ado_sign_thirdparty = false;
      currency::finalize_tx_param ftp{};
      ftp.tx_version = m_wallet->get_current_tx_version_and_hardfork_id(ftp.tx_hardfork_id);
      {
        proxy_scope bounded(m_wallet->m_core_proxy, std::move(proxy));
        exception_stage = detail::UnsignedConstructionException;
        if (!m_wallet->prepare_transaction(ctp, ftp)) return failed(detail::UnsignedConstructionFalse);
      }
      exception_stage = detail::PreparedRecordException;
      if (stop.load() || !authorized())
        return refuse(sign_status::Revoked);
      if (ftp.sources.empty() || ftp.sources.size() > CURRENCY_TX_MAX_ALLOWED_INPUTS ||
          ftp.selected_transfers.size() != ftp.sources.size() ||
          ftp.prepared_destinations.empty() ||
          ftp.prepared_destinations.size() > CURRENCY_TX_MAX_ALLOWED_OUTS)
        return failed(detail::PreparedShape);
      std::unique_ptr<prepared_payment::data> built(new prepared_payment::data());
      built->saved_draft = saved_draft;
      built->identity = m_identity;
      built->generation = m_generation; built->revision = m_revision;
      built->amount = request.amount; built->fee = assessment.compiled_policy_fee;
      built->recipient = assessment.canonical_recipient;
      built->local_blocks = assessment.local_blocks; built->tip = m_wallet->m_chain.get_top_block_id();
      built->confidential_decoys = assessment.confidential_decoy_policy;
      built->bare_decoys = assessment.bare_decoy_policy;
      for (const auto index : ftp.selected_transfers) {
        const auto it = m_wallet->m_transfers.find(index);
        if (it == m_wallet->m_transfers.end() || !it->second.m_ptx_wallet_info ||
            m_wallet->native_input_held(it->second)) return failed(detail::SelectedInput);
        const auto& transfer = it->second;
        held_input input{currency::get_transaction_hash(transfer.m_ptx_wallet_info->m_tx),
          transfer.m_internal_output_index, transfer.m_key_image};
        if (input.source_transaction == currency::null_hash || input.key_image == currency::null_ki)
          return failed(detail::SelectedInputIdentity);
        for (const auto& prior : built->inputs)
          if (prior.key_image == input.key_image) return failed(detail::DuplicateSelectedInput);
        built->inputs.push_back(input);
      }
      built->parameters = std::move(ftp);
      out.reviewed = std::shared_ptr<prepared_payment>(new prepared_payment(std::move(built)));
      out.status = sign_status::Prepared; return out;
    } catch (...) {
      if (m_wallet) m_wallet->m_found_free_amounts.clear();
      return failed(exception_stage);
    }
  }

  file_session::sign_result file_session::sign_reviewed_payment_to_held(
    prepared_payment& reviewed, signing_admission& admission, std::atomic<bool>& stop) noexcept {
    epee::log_space::scoped_log_silence quiet;
    sign_result out;
    const auto refuse = [&out](sign_status status) { out.status = status; return out; };
    if (m_busy || m_replay_wallet) return refuse(sign_status::Busy);
    if (reviewed.m_consumed || !reviewed.m_data) return refuse(sign_status::InvalidRequest);
    reviewed.m_consumed = true;
    bool construction_entered = false, publication_boundary_entered = false;
    try {
      const auto& terms = *reviewed.m_data;
      crypto::hash draft_hash{};
      crypto::cn_fast_hash(terms.saved_draft.data(), terms.saved_draft.size(), draft_hash);
      if (!m_wallet || !m_material || !authorized() || stop.load() || !admission.current() ||
          admission.saved_draft_hash() != draft_hash || !same_keys(terms.identity, m_identity) ||
          terms.identity.stored_genesis != m_identity.stored_genesis ||
          !same_keys(admission.subject(), m_identity) || admission.subject().stored_genesis != m_identity.stored_genesis)
        return refuse(sign_status::Revoked);
      // Fresh protected reopen deliberately changes generation. Actual chain,
      // inputs, decoys and destinations remain exact, with no reconstruction.
      if ((terms.generation == m_generation && terms.revision != m_revision) ||
          terms.local_blocks != m_wallet->m_chain.get_blockchain_current_size() ||
          terms.tip != m_wallet->m_chain.get_top_block_id() || terms.fee != TX_MINIMUM_FEE ||
          terms.confidential_decoys != m_wallet->get_core_runtime_config().hf4_minimum_mixins ||
          terms.bare_decoys != CURRENCY_DEFAULT_DECOY_SET_SIZE)
        return refuse(sign_status::StaleObservation);
      if (!m_material->signed_payment_supported() || !m_material->checkpoint_supported())
        return refuse(sign_status::Unsupported);
      if (terms.inputs.size() != terms.parameters.selected_transfers.size() ||
          terms.inputs.size() != terms.parameters.sources.size()) return refuse(sign_status::InvalidRequest);
      for (std::size_t n = 0; n < terms.inputs.size(); ++n) {
        const auto found = m_wallet->m_transfers.find(terms.parameters.selected_transfers[n]);
        const auto& input = terms.inputs[n];
        if (found == m_wallet->m_transfers.end() || !found->second.m_ptx_wallet_info ||
            currency::get_transaction_hash(found->second.m_ptx_wallet_info->m_tx) != input.source_transaction ||
            found->second.m_internal_output_index != input.output_index || found->second.m_key_image != input.key_image ||
            found->second.m_amount != terms.parameters.sources[n].amount || m_wallet->native_input_held(found->second) ||
            !m_wallet->is_transfer_ready_to_go(found->second,
              found->second.is_zc() ? terms.confidential_decoys : terms.bare_decoys))
          return refuse(sign_status::StaleObservation);
      }
      prepared_payment::data working = terms; // Its destructor wipes copied source masks too.
      currency::finalize_tx_param& ftp = working.parameters;
      size_t hardfork = 0;
      if (ftp.tx_version != m_wallet->get_current_tx_version_and_hardfork_id(hardfork) || ftp.tx_hardfork_id != hardfork)
        return refuse(sign_status::StaleObservation);
      if (!admission.current() || !authorized() || !admission.consume()) return refuse(sign_status::Revoked);
      if (m_revision == std::numeric_limits<uint64_t>::max()) return refuse(sign_status::Unsupported);
      busy_scope guard(m_busy);
      std::unique_ptr<signed_payment> built(new signed_payment());
      built->saved_draft = terms.saved_draft; built->identity = terms.identity;
      built->generation = m_generation; built->revision = m_revision;
      built->amount = terms.amount; built->fee = terms.fee; built->inputs = terms.inputs;
      currency::finalized_tx finalized{};
      struct secret_scope {
        crypto::secret_key& value;
        ~secret_scope() {
          volatile unsigned char* p = reinterpret_cast<volatile unsigned char*>(&value);
          for (std::size_t i = 0; i < sizeof(value); ++i) p[i] = 0;
        }
      } wipe_key{finalized.one_time_key};
      if (stop.load() || !admission.current() || !authorized())
        return refuse(sign_status::Revoked);
      // No authority escapes from the in-memory construction. If it fails,
      // discard this session through its sticky recovery path, never checkpoint
      // an unjournaled transaction key or reservation.
      construction_entered = true;
      // No dispatch, tx-key map insertion or sent-history callback here.
      m_wallet->finalize_transaction(ftp, finalized, false, false);
      built->transaction_blob = currency::tx_to_blob(finalized.tx);
      built->transaction_hash = currency::get_transaction_hash(finalized.tx);
      built->transaction_key = finalized.one_time_key;
      uint64_t actual_fee = 0;
      if (built->transaction_blob.empty() ||
          built->transaction_blob.size() >= CURRENCY_MAX_TRANSACTION_BLOB_SIZE ||
          built->transaction_hash == currency::null_hash ||
          !currency::get_tx_fee(finalized.tx, actual_fee) || actual_fee != built->fee) {
        mark_recovery(); return refuse(sign_status::RecoveryRequired);
      }
      if (stop.load() || !admission.current() || !authorized()) {
        mark_recovery(); return refuse(sign_status::Revoked);
      }
      wallet_public::wallet_transfer_info pending{};
      if (!m_wallet->native_build_signed_pending(finalized.tx, ftp, built->transaction_hash,
          terms.recipient, terms.amount, built->fee, stop, pending)) {
        mark_recovery();
        return refuse(stop.load() ? sign_status::Revoked : sign_status::RecoveryRequired);
      }
      if (stop.load() || !admission.current() || !authorized()) {
        mark_recovery(); return refuse(sign_status::Revoked);
      }
      // Every following mutation, including the real pending row, belongs to
      // this same recoverable SignedHeld image. No capture is admitted while
      // busy or on any ensuing recovery/unknown outcome.
      m_wallet->m_read_only_sync_dirty = true;
      m_state = session_state::OpenDirty;
      ++m_revision; m_page_capture = 0;
      // Preserve durable obligations independently of mutable SPENT/cache flags.
      // They are reloaded from authenticated operation records on every Open.
      m_wallet->m_native_held_inputs.insert(m_wallet->m_native_held_inputs.end(),
        built->inputs.begin(), built->inputs.end());
      m_wallet->m_tx_keys.emplace(built->transaction_hash, built->transaction_key);
      m_wallet->mark_transfers_with_flag(ftp.selected_transfers,
        WALLET_TRANSFER_DETAIL_FLAG_COLD_SIG_RESERVATION, "native SignedHeld", true);
      if (!m_wallet->m_unconfirmed_txs.emplace(built->transaction_hash, std::move(pending)).second) {
        mark_recovery(); return refuse(sign_status::RecoveryRequired);
      }
      currency::account_public_address expected{};
      expected.spend_public_key = m_identity.spend_public_key;
      expected.view_public_key = m_identity.view_public_key;
      expected.flags = m_identity.flags;
      if (m_wallet->render_native_checkpoint_image(expected, m_identity.stored_genesis,
          m_material->material().password, m_image_limit, built->checkpoint_image) !=
          wallet2::native_image_status::Rendered) {
        mark_recovery(); return refuse(sign_status::RecoveryRequired);
      }
      if (stop.load() || !admission.current() || !authorized()) {
        mark_recovery(); return refuse(sign_status::Revoked);
      }
      // One publication entry receives the complete signed authority and
      // encrypted post-reservation checkpoint. Prepared itself consumes the
      // saved draft; no separate marker can precede recoverable evidence.
      // Provider entry can leave durable/uncertain evidence even on refusal.
      publication_boundary_entered = true;
      const auto prepared = checked(m_material->prepare_signed_payment(*built));
      if (prepared != commit_status::Committed) {
        if (prepared == commit_status::Unknown) m_state = session_state::OutcomeUnknown;
        else mark_recovery();
        return refuse(prepared == commit_status::Unknown ? sign_status::OutcomeUnknown : sign_status::RecoveryRequired);
      }
      // Live saves may refuse after revocation. An already-durable exact
      // Prepared record is settled by the next authenticated Open, under its
      // retained checkpoint proof. No new signing admission is granted.
      const auto replaced = checked(m_material->replace_signed_checkpoint(*built));
      if (replaced != commit_status::Committed) {
        if (replaced == commit_status::Unknown) m_state = session_state::OutcomeUnknown;
        else mark_recovery();
        return refuse(replaced == commit_status::Unknown ? sign_status::OutcomeUnknown : sign_status::RecoveryRequired);
      }
      const auto completed = checked(m_material->complete_signed_payment(*built));
      if (completed != commit_status::Committed) {
        if (completed == commit_status::Unknown) m_state = session_state::OutcomeUnknown;
        else mark_recovery();
        return refuse(completed == commit_status::Unknown ? sign_status::OutcomeUnknown : sign_status::RecoveryRequired);
      }
      m_wallet->m_read_only_sync_dirty = false;
      m_state = session_state::OpenUnchanged;
      if (!admission.current() || !authorized()) return refuse(sign_status::Revoked);
      out.status = sign_status::SignedHeld;
      out.transaction_hash = built->transaction_hash;
      return out;
    } catch (...) {
      if (!construction_entered && !publication_boundary_entered) return refuse(sign_status::PreparationFailed);
      if (m_wallet) mark_recovery();
      return refuse(sign_status::RecoveryRequired);
    }
  }

  file_session::sign_result file_session::sign_payment_to_held(
    const payment_request& request, const std::string& saved, signing_admission& admission,
    std::shared_ptr<native_payment::decoy_exchange> exchange, std::atomic<bool>& stop) noexcept {
    crypto::hash expected{};
    if(saved.empty() || saved.size()>4096)return {sign_status::InvalidRequest,{}};
    crypto::cn_fast_hash(saved.data(),saved.size(),expected);
    if (!admission.current() || admission.saved_draft_hash()!=expected || !same_keys(admission.subject(),m_identity) ||
        admission.subject().stored_genesis!=m_identity.stored_genesis)return {sign_status::Revoked,{}};
    auto prepared = prepare_payment_review(request, saved, std::move(exchange), stop);
    if (prepared.status != sign_status::Prepared || !prepared.reviewed) return {prepared.status, {}};
    return sign_reviewed_payment_to_held(*prepared.reviewed, admission, stop);
  }

  payment_result file_session::assess_local_payment(const payment_request& request) noexcept
  {
    epee::log_space::scoped_log_silence quiet; // Parser diagnostics contain recipients.
    payment_result result;
    const auto refuse = [&result](payment_status status) -> payment_result
    {
      result.status = status;
      return std::move(result);
    };
    if (m_busy || m_replay_wallet) return refuse(payment_status::Busy);
    switch (state())
    {
      case session_state::OpenUnchanged:
      case session_state::OpenDirty: break;
      case session_state::RecoveryRequired: return refuse(payment_status::RecoveryRequired);
      case session_state::OutcomeUnknown: return refuse(payment_status::OutcomeUnknown);
      default: return result;
    }
    if (!m_wallet || !m_material) return result;
    // Also covers authorization callbacks, so none can reenter this session.
    busy_scope guard(m_busy);
    try
    {
      // Capture caller-owned values before any authorization, material or clock
      // callback. Bound the recipient copy and contain allocation failure under noexcept.
      const uint64_t amount = request.amount;
      const uint64_t expected_generation = request.expected_generation;
      const uint64_t expected_revision = request.expected_revision;
      const bool recipient_too_long = request.recipient.size() > max_payment_recipient_bytes;
      const std::string recipient_text = recipient_too_long ? std::string{} : request.recipient;
      if (!authorized()) return refuse(payment_status::Revoked);
      if (m_revision_exhausted) return refuse(payment_status::LimitExceeded);
      if (expected_generation == 0 || expected_generation != m_generation ||
          expected_revision != m_revision)
        return refuse(payment_status::StaleObservation);
      const auto& profile = build_profile();
      if (profile.status != profile_status::Available || profile.network != compiled_network::Testnet ||
          profile.formation != 103 || profile.genesis == currency::null_hash)
        return refuse(payment_status::UnsupportedProfile);
      const auto identity_current = [this, &profile]()
      {
        const auto& address = m_wallet->m_account.get_public_address();
        return same_keys(m_identity, m_material->material().expected) &&
          m_material->material().expected.stored_genesis == m_identity.stored_genesis &&
          m_identity.stored_genesis == profile.genesis && m_wallet->m_chain.get_genesis() == profile.genesis &&
          address.spend_public_key == m_identity.spend_public_key &&
          address.view_public_key == m_identity.view_public_key && address.flags == m_identity.flags;
      };
      if (!identity_current()) return refuse(payment_status::IdentityMismatch);
      if (!m_wallet->m_native_file_mode || m_wallet->is_watch_only() || m_wallet->m_account.is_watch_only() ||
          m_identity.flags != 0 ||
          !m_wallet->native_scan_minimum_supported() ||
          m_wallet->m_read_only_sync_active)
        return refuse(payment_status::UnsupportedWallet);
      if (amount == 0) return refuse(payment_status::InvalidAmount);
      if (recipient_too_long || recipient_text.empty())
        return refuse(payment_status::InvalidRecipient);
      // No aliases, whitespace, NULs or Unicode. Bound before the core parser.
      const char* const alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
      for (const unsigned char c : recipient_text)
        if (c == 0 || !std::strchr(alphabet, c)) return refuse(payment_status::InvalidRecipient);
      currency::address_v recipient;
      currency::payment_id_t payment_id;
      if (!currency::get_account_address_and_payment_id_from_str(recipient, payment_id, recipient_text) ||
          recipient.type() != typeid(currency::account_public_address) || !payment_id.empty())
        return refuse(payment_status::InvalidRecipient);
      const auto& ordinary = boost::get<currency::account_public_address>(recipient);
      if (ordinary.flags != 0 || currency::get_account_address_as_str(ordinary) != recipient_text)
        return refuse(payment_status::InvalidRecipient); // Also rejects empty integrated IDs.
      const uint64_t fee = m_wallet->get_default_fee(); // TX_DEFAULT_FEE, not a proxy/live quote.
      if (amount > std::numeric_limits<uint64_t>::max() - fee)
        return refuse(payment_status::AmountOverflow);
      if (m_wallet->m_unconfirmed_txs.size() > max_snapshot_records ||
          m_wallet->m_unconfirmed_in_transfers.size() > max_snapshot_records ||
          m_wallet->m_unconfirmed_multisig_transfers.size() > max_snapshot_records ||
          m_wallet->m_money_expirations.size() > max_snapshot_records)
        return refuse(payment_status::LimitExceeded);
      if (!m_wallet->m_unconfirmed_in_transfers.empty() ||
          !m_wallet->m_unconfirmed_multisig_transfers.empty() || !m_wallet->m_money_expirations.empty())
        return refuse(payment_status::RetainedPendingUnsupported);
      size_t pending_work = 0;
      // Payment-only pending: the unchanged wallet2 validator. Actual alias
      // registration/update rows are recognized with their own typed semantics
      // (native_wallet_alias_operation.cpp); anything else still fails closed.
      const auto pending = typed_pending_supported(pending_work);
      if (pending != snapshot_status::Available) {
        if (pending == snapshot_status::Revoked) return refuse(payment_status::Revoked);
        if (pending == snapshot_status::LimitExceeded) return refuse(payment_status::LimitExceeded);
        return refuse(payment_status::RetainedPendingUnsupported);
      }
      const auto clock = m_wallet->get_core_runtime_config().get_core_time;
      if (!clock) return result;
      std::unique_ptr<local_payment_assessment> built(new local_payment_assessment());
      built->captured_at = clock(); // One time for every unlock check.
      built->local_blocks = m_wallet->m_chain.get_blockchain_current_size();
      const snapshot_status status = m_wallet->native_payment_capacity(built->captured_at, built->local_blocks,
        built->eligible_local_capacity, built->eligible_output_count, pending_work);
      if (status != snapshot_status::Available)
      {
        switch (status)
        {
          case snapshot_status::LimitExceeded: return refuse(payment_status::LimitExceeded);
          case snapshot_status::UnsupportedData: return refuse(payment_status::UnsupportedData);
          default: return refuse(payment_status::InvalidData);
        }
      }
      built->canonical_recipient = recipient_text;
      built->asset_id = currency::native_coin_asset_id;
      static_assert(CURRENCY_DISPLAY_DECIMAL_POINT <= std::numeric_limits<uint8_t>::max(), "native precision fits");
      built->decimals = CURRENCY_DISPLAY_DECIMAL_POINT;
      built->bare_decoy_policy = CURRENCY_DEFAULT_DECOY_SET_SIZE;
      built->confidential_decoy_policy = m_wallet->get_core_runtime_config().hf4_minimum_mixins;
      built->amount = amount;
      built->compiled_policy_fee = fee;
      built->amount_plus_fee = amount + fee;
      built->advisory_amount_upper_bound = built->eligible_local_capacity > fee ? built->eligible_local_capacity - fee : 0;
      built->local_capacity_covers_amount_and_fee = built->amount_plus_fee <= built->eligible_local_capacity;
      built->session_generation = expected_generation;
      built->revision = expected_revision;
      built->network = profile.network;
      built->formation = profile.formation;
      built->genesis = profile.genesis;
      if (!identity_current()) return refuse(payment_status::IdentityMismatch);
      if (!authorized()) return refuse(payment_status::Revoked);
      result.assessment = std::move(built);
      result.status = payment_status::LocalAdvisory;
      return result;
    }
    catch (const std::bad_alloc&) { return payment_result{}; }
    catch (...) { return refuse(payment_status::InvalidData); } // Never inspect exception text.
  }

  snapshot_result file_session::capture_snapshot(const snapshot_request& request) noexcept
  {
    return capture_snapshot_impl(request, nullptr);
  }

  product_snapshot_result_v1 file_session::capture_product_snapshot_v1(const snapshot_request& request) noexcept
  {
    product_snapshot_result_v1 out;
    scan_coverage_v1 coverage;
    auto captured = capture_snapshot_impl(request, &coverage);
    out.status = captured.status;
    if (captured.status == snapshot_status::Available && captured.observation)
    {
      out.observation = std::move(captured.observation);
      out.scan_coverage = coverage;
    }
    return out;
  }

  product_snapshot_result_v2 file_session::capture_product_snapshot_v2(const snapshot_request& request) noexcept
  {
    product_snapshot_result_v2 out;
    scan_coverage_v1 coverage;
    auto captured = capture_snapshot_impl(request, &coverage, &out.owned_outputs);
    out.status = captured.status;
    if (captured.status == snapshot_status::Available && captured.observation)
    {
      out.observation = std::move(captured.observation);
      out.scan_coverage = coverage;
    }
    return out;
  }

  product_snapshot_result_v3 file_session::capture_product_snapshot_v3(const snapshot_request& request) noexcept
  {
    product_snapshot_result_v3 out;
    scan_coverage_v1 coverage;
    auto captured = capture_snapshot_impl(request, &coverage, &out.owned_outputs, &out.retained_birthday);
    out.status = captured.status;
    if (captured.status == snapshot_status::Available && captured.observation)
    {
      out.observation = std::move(captured.observation);
      out.scan_coverage = coverage;
    }
    return out;
  }

  snapshot_result file_session::capture_snapshot_impl(const snapshot_request& request, scan_coverage_v1* coverage) noexcept
  {
    return capture_snapshot_impl(request, coverage, nullptr);
  }

  snapshot_result file_session::capture_snapshot_impl(const snapshot_request& request, scan_coverage_v1* coverage,
    std::vector<activity_owned_outputs_v1>* owned_outputs) noexcept
  {
    return capture_snapshot_impl(request, coverage, owned_outputs, nullptr);
  }

  // Owned outputs and the retained birthday are built into capture-local
  // storage and published only after the final authorization sample, together
  // with the observation. Every refusal leaves both outputs untouched/empty.
  snapshot_result file_session::capture_snapshot_impl(const snapshot_request& request, scan_coverage_v1* coverage,
    std::vector<activity_owned_outputs_v1>* owned_outputs, retained_account_birthday_v1* retained_birthday) noexcept
  {
    epee::log_space::scoped_log_silence quiet;
    snapshot_result result;
    if (m_busy)
    {
      result.status = snapshot_status::Busy;
      return result;
    }
    switch (state())
    {
      case session_state::OpenUnchanged:
      case session_state::OpenDirty: break;
      case session_state::RecoveryRequired: result.status = snapshot_status::RecoveryRequired; return result;
      case session_state::OutcomeUnknown: result.status = snapshot_status::OutcomeUnknown; return result;
      default: return result;
    }
    if (!m_wallet || !m_material) return result;
    if (!authorized())
    {
      result.status = snapshot_status::Revoked;
      return result;
    }
    if (request.page_size == 0 || request.page_size > max_activity_page ||
        (request.continuation && request.cursor.offset == 0))
    {
      result.status = snapshot_status::InvalidRequest;
      return result;
    }
    if (m_revision_exhausted)
    {
      result.status = snapshot_status::LimitExceeded;
      return result;
    }
    try
    {
      // Clock and provider callbacks cannot reenter while this is built.
      busy_scope guard(m_busy);
      uint64_t captured_at = 0;
      if (request.continuation)
      {
        const activity_cursor& cursor = request.cursor;
        if (m_page_capture == 0 || cursor.generation != m_generation || cursor.revision != m_revision ||
            cursor.capture != m_page_capture || cursor.captured_at != m_page_captured_at)
        {
          result.status = snapshot_status::StaleCursor;
          return result;
        }
        captured_at = m_page_captured_at; // The first page's frozen time.
      }
      else
      {
        const auto clock = m_wallet->get_core_runtime_config().get_core_time;
        if (!clock) return result;
        if (m_page_capture == std::numeric_limits<uint64_t>::max())
        {
          result.status = snapshot_status::LimitExceeded;
          return result;
        }
        captured_at = clock();
        // A new first page supersedes every earlier cursor, even if it fails.
        ++m_page_capture;
        m_page_captured_at = captured_at;
      }
      std::unique_ptr<public_snapshot> built(new public_snapshot());
      std::vector<activity_owned_outputs_v1> built_owned_outputs;
      retained_account_birthday_v1 built_retained_birthday;
      const snapshot_status status = build_snapshot(request, captured_at, *built,
        owned_outputs ? &built_owned_outputs : nullptr, retained_birthday ? &built_retained_birthday : nullptr);
      if (status != snapshot_status::Available)
      {
        result.status = status;
        return result;
      }
      if (coverage)
      {
        // The minimum belongs to the same authenticated loaded image/revision as
        // the detached rows. Unresolved fresh state is explicit, never genesis.
        scan_coverage_v1 selected;
        if (m_wallet->m_minimum_height != WALLET_MINIMUM_HEIGHT_UNSET_CONST)
        {
          if (!m_wallet->native_scan_minimum_supported() ||
              m_wallet->m_minimum_height > built->local_tip_height)
          { result.status = snapshot_status::InvalidData; return result; }
          selected.minimum_height = m_wallet->m_minimum_height;
          // Transient replay scratch has no admitted boundary to describe.
          const uint32_t source = m_replay_wallet ? 0 :
            (m_sync ? m_sync->scan_start_source_v1(selected.minimum_height) : 3);
          switch (source)
          {
            case 1: selected.source = scan_start_source_v1::SeedDate; break;
            case 2: selected.source = scan_start_source_v1::GenesisFallback; break;
            case 3: selected.source = scan_start_source_v1::SavedBoundary; break;
            default: result.status = snapshot_status::InvalidData; return result;
          }
          selected.checkpointed = state() == session_state::OpenUnchanged;
        }
        *coverage = selected;
      }
      // Final sample immediately before publication. The build ran no step,
      // so the revision and wallet state it describes are still current.
      if (!authorized())
      {
        result.status = snapshot_status::Revoked;
        return result;
      }
      if (owned_outputs) owned_outputs->swap(built_owned_outputs);
      if (retained_birthday) *retained_birthday = built_retained_birthday;
      result.observation = std::move(built);
      result.status = snapshot_status::Available;
      return result;
    }
    catch (snapshot_status status) // Owned-output budget/authority refusals.
    { snapshot_result failed; failed.status = status; return failed; }
    catch (const std::bad_alloc&) { return snapshot_result{}; } // Unavailable, no payload
    catch (...)
    {
      snapshot_result failed;
      failed.status = snapshot_status::InvalidData; // never inspect exception text
      return failed;
    }
  }

  snapshot_status file_session::build_snapshot(const snapshot_request& request, uint64_t captured_at,
    public_snapshot& out) const
  {
    return build_snapshot(request, captured_at, out, nullptr);
  }

  snapshot_status file_session::build_snapshot(const snapshot_request& request, uint64_t captured_at,
    public_snapshot& out, std::vector<activity_owned_outputs_v1>* owned_outputs) const
  {
    return build_snapshot(request, captured_at, out, owned_outputs, nullptr);
  }

  snapshot_status file_session::build_snapshot(const snapshot_request& request, uint64_t captured_at,
    public_snapshot& out, std::vector<activity_owned_outputs_v1>* owned_outputs,
    retained_account_birthday_v1* retained_birthday) const
  {
    // The verified open identity must still describe the loaded wallet and
    // this build's compiled profile. Drift is reported, not latched here;
    // checkpoint/close latch it themselves.
    const auto& profile = build_profile();
    const auto& address = m_wallet->m_account.get_public_address();
    if (profile.status != profile_status::Available || m_identity.stored_genesis != profile.genesis ||
        m_wallet->m_chain.get_genesis() != m_identity.stored_genesis ||
        address.spend_public_key != m_identity.spend_public_key ||
        address.view_public_key != m_identity.view_public_key || address.flags != m_identity.flags)
      return snapshot_status::InvalidData;
    out.identity = m_identity;
    out.network = profile.network;
    out.formation = profile.formation;
    out.genesis = profile.genesis;
    out.session_generation = m_generation;
    out.revision = m_revision;
    out.captured_at = captured_at;

    // Local chain position only; nothing here was compared with a node.
    const auto& chain = m_wallet->m_chain;
    const uint64_t local_blocks = chain.get_blockchain_current_size();
    if (local_blocks == 0) return snapshot_status::InvalidData;
    out.local_blocks = local_blocks;
    out.local_tip_height = local_blocks - 1;
    if (out.local_tip_height != 0)
    {
      std::pair<uint64_t, crypto::hash> tip{};
      if (chain.lookup_item_around(out.local_tip_height, tip) && tip.first == out.local_tip_height)
      {
        if (tip.second == currency::null_hash)
          return snapshot_status::InvalidData;
        // A deep detach can empty the recent index while an exact sparse
        // entry survives: the tip is then unknown, not contradictory.
        const crypto::hash recent = chain.get_top_block_id();
        if (recent != currency::null_hash)
        {
          if (recent != tip.second)
            return snapshot_status::InvalidData;
          out.local_tip_hash_known = true;
          out.local_tip_hash = tip.second;
        }
      }
    }

    std::size_t nested = 0;
    if (retained_birthday)
    {
      // One fixed scalar read, charged to this same capture's work budget.
      // The output points only to capture-local storage until final authority
      // succeeds. No mnemonic, account mutation, clock/date conversion or
      // historical selection-origin inference is involved.
      if (!take_nested(nested, 1)) return snapshot_status::LimitExceeded;
      retained_account_birthday_v1 birthday;
      const uint64_t timestamp = m_wallet->m_account.get_createtime();
      if (timestamp != 0)
      {
        birthday.available = true;
        birthday.timestamp = timestamp;
      }
      *retained_birthday = birthday;
    }
    snapshot_status status = m_wallet->collect_native_amounts(captured_at, local_blocks, nested, out.assets);
    if (status != snapshot_status::Available) return status;

    // Retained activity: order/identity keys only, never record copies.
    const auto& pending = m_wallet->m_unconfirmed_txs;
    if (pending.size() > max_snapshot_records || m_wallet->m_transfer_history.size() > max_snapshot_records)
      return snapshot_status::LimitExceeded;
    std::vector<activity_key> keys;
    keys.reserve(pending.size() + m_wallet->m_transfer_history.size());
    for (const auto& item : pending)
    {
      const auto& wti = item.second;
      // The pending container is the status source; a height there is not a
      // confirmation (and a zero height is never genesis).
      if (item.first != wti.tx_hash || wti.tx_hash == currency::null_hash || wti.height != 0)
        return snapshot_status::InvalidData;
      activity_key key;
      key.record = &wti;
      key.pending = true;
      key.timestamp = wti.timestamp;
      key.hash = wti.tx_hash;
      keys.push_back(key);
    }
    m_wallet->enumerate_transfers_history([&](const wallet_public::wallet_transfer_info& wti)
    {
      if (wti.tx_hash == currency::null_hash || wti.height >= local_blocks)
      {
        status = snapshot_status::InvalidData;
        return false;
      }
      activity_key key;
      key.record = &wti;
      key.timestamp = wti.timestamp;
      key.height = wti.height;
      key.hash = wti.tx_hash;
      keys.push_back(key);
      return true;
    }, true);
    if (status != snapshot_status::Available) return status;

    {
      // One identity per retained transaction, across both containers.
      std::vector<crypto::hash> ids;
      ids.reserve(keys.size());
      for (const auto& key : keys) ids.push_back(key.hash);
      std::sort(ids.begin(), ids.end(), hash_less);
      if (std::adjacent_find(ids.begin(), ids.end()) != ids.end())
        return snapshot_status::InvalidData;
    }
    std::sort(keys.begin(), keys.end(), newer_first);

    const uint64_t total = keys.size();
    const uint64_t offset = request.continuation ? request.cursor.offset : 0;
    if (request.continuation && offset >= total) return snapshot_status::StaleCursor;
    const uint64_t count = std::min<uint64_t>(request.page_size, total - offset);
    const uint64_t end = offset + count;
    out.activity.reserve(static_cast<std::size_t>(count));
    for (uint64_t i = offset; i != end; ++i)
    {
      const activity_key& key = keys[static_cast<std::size_t>(i)];
      const auto& wti = *key.record;
      activity_row row;
      row.tx_hash = key.hash;
      row.status = key.pending ? activity_status::RetainedPending : activity_status::RetainedConfirmed;
      row.timestamp_known = wti.timestamp != 0;
      row.timestamp = wti.timestamp;
      if (!key.pending)
      {
        row.confirmed_height = wti.height;
        row.confirmations = local_blocks - wti.height; // height < local_blocks checked above
      }
      row.fee_source = wti.fee != 0 ? fee_record::WalletRecorded : fee_record::NotRecorded;
      row.recorded_fee = wti.fee;

      std::size_t legs = 0;
      for (const auto& group : wti.subtransfers_by_pid)
      {
        if (!take_nested(nested, 1)) return snapshot_status::LimitExceeded;
        if (group.subtransfers.size() > max_activity_legs - legs) return snapshot_status::LimitExceeded;
        legs += group.subtransfers.size();
      }
      if (!take_nested(nested, legs)) return snapshot_status::LimitExceeded;
      row.legs.reserve(legs);
      for (const auto& group : wti.subtransfers_by_pid)
        for (const auto& sub : group.subtransfers)
        {
          if (sub.asset_id == currency::null_pkey) return snapshot_status::InvalidData;
          row.legs.push_back(activity_leg{sub.asset_id, sub.is_income, sub.amount});
        }

      const auto& remote = wti.remote_addresses;
      if (remote.size() > max_activity_counterparties || !take_nested(nested, remote.size()))
        return snapshot_status::LimitExceeded;
      for (const auto& counterparty : remote)
        if (counterparty.size() > max_counterparty_bytes) return snapshot_status::LimitExceeded;
      row.counterparties.assign(remote.begin(), remote.end());
      out.activity.push_back(std::move(row));
    }

    if (owned_outputs)
    {
      // Additive v2 requested-storage budget: result vectors, sorted reference
      // indexes, canonical buffers and version-conversion temporaries. Existing
      // legacy allocations retain their own limits. Cumulative requests, not
      // only peak storage, consume this budget; allocator overhead is excluded.
      size_t allocations_left = size_t(64) * 1024 * 1024;
      size_t origin_work_left = size_t(16) * 1024 * 1024;
      const auto charge = [&](size_t count, size_t width) {
        if (width != 0 && count > allocations_left / width) throw snapshot_status::LimitExceeded;
        allocations_left -= count * width;
      };
      const auto tick = [&](size_t count) {
        if (!take_nested(nested, count)) throw snapshot_status::LimitExceeded;
        if (!authorized()) throw snapshot_status::Revoked;
      };
      const auto source_compare = [](const crypto::hash& a, const crypto::hash& b) {
        return std::memcmp(&a, &b, sizeof(a));
      };
      struct output_reference
      {
        const transfer_details* transfer = nullptr;
        crypto::hash source{};
      };
      std::vector<output_reference> transfers;
      std::vector<const held_input*> holds_by_source, holds_by_image;
      std::vector<uint8_t> requested_origins;
      const bool full_native = m_wallet->m_native_file_mode && !m_wallet->m_watch_only &&
        !m_wallet->m_account.is_watch_only() && address.flags == 0;
      if (full_native)
      {
        const auto& retained = m_wallet->m_transfers;
        const auto& held = m_wallet->m_native_held_inputs;
        if (retained.size() > max_snapshot_records || held.size() > max_snapshot_records ||
            held.size() > CURRENCY_TX_MAX_ALLOWED_INPUTS * 128)
          return snapshot_status::LimitExceeded;
        // Only this requested page needs origin association. Validate each
        // supported confirmed recorded prefix once, so its ordinary output
        // keys can also admit a retained record with a wrong cached height.
        // Height/key/index are eligibility filters, never output identity.
        tick(static_cast<size_t>(count));
        charge(static_cast<size_t>(count), sizeof(uint8_t));
        requested_origins.assign(static_cast<size_t>(count), 0);
        std::vector<uint64_t> requested_heights;
        charge(static_cast<size_t>(count), sizeof(uint64_t));
        requested_heights.reserve(static_cast<size_t>(count));
        size_t requested_output_count = 0;
        for (uint64_t i = offset; i != end; ++i)
        {
          const auto& key = keys[static_cast<size_t>(i)];
          const auto& received = key.record->employed_entries.receive;
          if (key.pending || received.empty()) continue;
          if (received.size() > max_snapshot_records) return snapshot_status::LimitExceeded;
          crypto::hash recorded_source{};
          const auto read = wallet2::native_history_origin_hash(key.record->tx, nested, origin_work_left,
            allocations_left, [this] { return authorized(); }, recorded_source);
          if (read == snapshot_status::UnsupportedData) continue;
          if (read != snapshot_status::Available) return read;
          if (recorded_source != key.hash) return snapshot_status::InvalidData;
          if (received.size() > max_snapshot_nested_elements - requested_output_count)
            return snapshot_status::LimitExceeded;
          requested_output_count += received.size();
          requested_origins[static_cast<size_t>(i - offset)] = 1;
          requested_heights.push_back(key.height);
        }
        struct requested_output_reference
        {
          crypto::public_key key{};
          uint64_t index = 0;
        };
        const auto ordinary_output_key = [](const currency::tx_out_v& output) -> const crypto::public_key* {
          if (output.type() == typeid(currency::tx_out_zarcanum))
            return &boost::get<currency::tx_out_zarcanum>(output).stealth_address;
          if (output.type() == typeid(currency::tx_out_bare))
          {
            const auto& bare = boost::get<currency::tx_out_bare>(output);
            if (bare.target.type() == typeid(currency::txout_to_key))
              return &boost::get<currency::txout_to_key>(bare.target).key;
          }
          return nullptr;
        };
        const auto key_compare = [](const crypto::public_key& a, const crypto::public_key& b) {
          return std::memcmp(&a, &b, sizeof(a));
        };
        std::vector<requested_output_reference> requested_outputs;
        charge(requested_output_count, sizeof(requested_output_reference));
        requested_outputs.reserve(requested_output_count);
        tick(static_cast<size_t>(count));
        for (uint64_t i = offset; i != end; ++i)
        {
          if (!requested_origins[static_cast<size_t>(i - offset)]) continue;
          const auto& wti = *keys[static_cast<size_t>(i)].record;
          tick(wti.employed_entries.receive.size());
          for (const auto& entry : wti.employed_entries.receive)
          {
            if (entry.index >= wti.tx.vout.size()) return snapshot_status::InvalidData;
            const auto* const key = ordinary_output_key(wti.tx.vout[entry.index]);
            if (!key) return snapshot_status::InvalidData; // supported prefix promised ordinary outputs
            requested_outputs.push_back(requested_output_reference{*key, entry.index});
          }
        }
        std::sort(requested_heights.begin(), requested_heights.end(), [&](uint64_t a, uint64_t b) {
          tick(1); return a < b;
        });
        std::sort(requested_outputs.begin(), requested_outputs.end(),
          [&](const requested_output_reference& a, const requested_output_reference& b) {
            tick(1);
            const int order = key_compare(a.key, b.key);
            return order != 0 ? order < 0 : a.index < b.index;
          });
        // Fixed 8 KiB stack prefilter avoids a logarithmic key search for
        // every unrelated retained output. Bucket collisions only add charged
        // comparisons, never acceptance or an identity/spent-state inference.
        unsigned char requested_key_buckets[8192] = {};
        const auto key_bucket = [](const crypto::public_key& key) {
          const auto* const bytes = reinterpret_cast<const unsigned char*>(&key);
          return size_t(bytes[0]) | (size_t(bytes[1]) << 8);
        };
        tick(requested_outputs.size());
        for (const auto& output : requested_outputs)
        {
          const size_t bucket = key_bucket(output.key);
          requested_key_buckets[bucket >> 3] |= static_cast<unsigned char>(1u << (bucket & 7));
        }
        if (!requested_heights.empty())
        {
          tick(retained.size());
          charge(retained.size(), sizeof(output_reference));
          transfers.reserve(retained.size());
          // One retained scan, preserving pointer/index/chain sanity checks.
          // Only requested heights or exact requested output key/index matches
          // reach the existing pointer grouping, canonical hash and source sort.
          for (const auto& item : retained)
          {
            if (!authorized()) return snapshot_status::Revoked;
            const auto& td = item.second;
            if (!td.m_ptx_wallet_info || td.m_internal_output_index >= td.m_ptx_wallet_info->m_tx.vout.size() ||
                td.m_ptx_wallet_info->m_block_height >= local_blocks)
              return snapshot_status::InvalidData;
            const uint64_t height = td.m_ptx_wallet_info->m_block_height;
            bool candidate = false;
            if (height >= requested_heights.front() && height <= requested_heights.back())
            {
              const auto at = std::lower_bound(requested_heights.begin(), requested_heights.end(), height,
                [&](uint64_t value, uint64_t wanted) { tick(1); return value < wanted; });
              candidate = at != requested_heights.end() && *at == height;
            }
            if (!candidate)
            {
              const auto* const key = ordinary_output_key(td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index]);
              if (key)
              {
                const size_t bucket = key_bucket(*key);
                if (requested_key_buckets[bucket >> 3] & static_cast<unsigned char>(1u << (bucket & 7)))
                {
                  const requested_output_reference wanted{*key, td.m_internal_output_index};
                  const auto at = std::lower_bound(requested_outputs.begin(), requested_outputs.end(), wanted,
                    [&](const requested_output_reference& a, const requested_output_reference& b) {
                      tick(1);
                      const int order = key_compare(a.key, b.key);
                      return order != 0 ? order < 0 : a.index < b.index;
                    });
                  candidate = at != requested_outputs.end() && at->index == wanted.index &&
                    key_compare(at->key, wanted.key) == 0;
                }
              }
            }
            if (candidate) transfers.push_back(output_reference{&td, {}});
          }
          // Pointer order is only a capture-local memoization key. It is never
          // returned, retained or substituted for canonical transaction identity.
          std::sort(transfers.begin(), transfers.end(), [&](const output_reference& a, const output_reference& b) {
            tick(1);
            return std::less<const currency::transaction*>()(&a.transfer->m_ptx_wallet_info->m_tx,
              &b.transfer->m_ptx_wallet_info->m_tx);
          });
          const currency::transaction* previous = nullptr;
          crypto::hash source{};
          size_t resolved = 0;
          tick(transfers.size());
          for (size_t i = 0; i != transfers.size(); ++i)
          {
            const transfer_details* const td = transfers[i].transfer;
            const auto* const parent = &td->m_ptx_wallet_info->m_tx;
            if (parent != previous)
            {
              previous = parent;
              const auto read = wallet2::native_history_origin_hash(*parent, nested, origin_work_left,
                allocations_left, [this] { return authorized(); }, source);
              if (read == snapshot_status::UnsupportedData) source = currency::null_hash;
              else if (read != snapshot_status::Available) return read;
            }
            if (source != currency::null_hash)
              transfers[resolved++] = output_reference{td, source};
          }
          transfers.resize(resolved);
          std::sort(transfers.begin(), transfers.end(), [&](const output_reference& a, const output_reference& b) {
            tick(1);
            const int order = source_compare(a.source, b.source);
            return order != 0 ? order < 0 : a.transfer->m_internal_output_index < b.transfer->m_internal_output_index;
          });
          tick(transfers.size());
          for (size_t i = 1; i < transfers.size(); ++i)
            if (transfers[i-1].source == transfers[i].source &&
                transfers[i-1].transfer->m_internal_output_index == transfers[i].transfer->m_internal_output_index)
              return snapshot_status::InvalidData;
        }
        tick(held.size());
        charge(held.size(), sizeof(const held_input*));
        holds_by_source.reserve(held.size());
        for (const auto& input : held)
        {
          if (input.source_transaction == currency::null_hash || input.key_image == currency::null_ki)
            return snapshot_status::InvalidData;
          holds_by_source.push_back(&input);
        }
        std::sort(holds_by_source.begin(), holds_by_source.end(), [&](const held_input* a, const held_input* b) {
          tick(1);
          const int order = source_compare(a->source_transaction, b->source_transaction);
          if (order != 0) return order < 0;
          if (a->output_index != b->output_index) return a->output_index < b->output_index;
          return std::memcmp(&a->key_image, &b->key_image, sizeof(a->key_image)) < 0;
        });
        tick(holds_by_source.size());
        for (size_t i = 1; i < holds_by_source.size(); ++i)
          if (holds_by_source[i-1]->source_transaction == holds_by_source[i]->source_transaction &&
              holds_by_source[i-1]->output_index == holds_by_source[i]->output_index &&
              holds_by_source[i-1]->key_image != holds_by_source[i]->key_image)
            return snapshot_status::InvalidData;
        tick(holds_by_source.size());
        charge(holds_by_source.size(), sizeof(const held_input*));
        holds_by_image.assign(holds_by_source.begin(), holds_by_source.end());
        std::sort(holds_by_image.begin(), holds_by_image.end(), [&](const held_input* a, const held_input* b) {
          tick(1);
          return std::memcmp(&a->key_image, &b->key_image, sizeof(a->key_image)) < 0;
        });
        tick(holds_by_image.size());
        for (size_t i = 1; i < holds_by_image.size(); ++i)
          if (holds_by_image[i-1]->key_image == holds_by_image[i]->key_image &&
              (holds_by_image[i-1]->source_transaction != holds_by_image[i]->source_transaction ||
               holds_by_image[i-1]->output_index != holds_by_image[i]->output_index))
            return snapshot_status::InvalidData;
      }
      tick(static_cast<size_t>(count));
      charge(static_cast<size_t>(count), sizeof(activity_owned_outputs_v1));
      owned_outputs->reserve(static_cast<size_t>(count));
      for (uint64_t i = offset; i != end; ++i)
      {
        const auto& key = keys[static_cast<size_t>(i)];
        const auto& wti = *key.record;
        const auto& received = wti.employed_entries.receive;
        if (received.size() > max_snapshot_records) return snapshot_status::LimitExceeded;
        tick(received.size());
        struct received_reference { const wallet_public::employed_tx_entry* entry; };
        charge(received.size(), sizeof(received_reference));
        std::vector<received_reference> recorded;
        recorded.reserve(received.size());
        for (const auto& entry : received)
        {
          if (entry.index >= wti.tx.vout.size()) return snapshot_status::InvalidData;
          recorded.push_back(received_reference{&entry});
        }
        std::sort(recorded.begin(), recorded.end(), [&](const received_reference& a, const received_reference& b) {
          tick(1); return a.entry->index < b.entry->index;
        });
        tick(recorded.size());
        activity_owned_outputs_v1 metadata;
        metadata.tx_hash = key.hash;
        charge(recorded.size(), sizeof(owned_output_v1));
        metadata.outputs.reserve(recorded.size());
        uint64_t last_index = 0;
        bool first = true;
        for (const auto& item : recorded)
        {
          const auto& entry = *item.entry;
          if (!first && entry.index == last_index) return snapshot_status::InvalidData;
          first = false;
          last_index = entry.index;
          owned_output_v1 output;
          output.output_index = entry.index;
          if (full_native && !key.pending && requested_origins[static_cast<size_t>(i - offset)])
          {
            const auto found = std::lower_bound(transfers.begin(), transfers.end(), entry.index,
              [&](const output_reference& candidate, uint64_t index) {
                tick(1);
                const int order = source_compare(candidate.source, key.hash);
                return order != 0 ? order < 0 : candidate.transfer->m_internal_output_index < index;
              });
            if (found != transfers.end() && found->source == key.hash &&
                found->transfer->m_internal_output_index == entry.index)
            {
              const auto& td = *found->transfer;
              const bool spent = (td.m_flags & WALLET_TRANSFER_DETAIL_FLAG_SPENT) != 0;
              if ((!spent && td.m_spent_height != 0) || td.m_ptx_wallet_info->m_block_height != wti.height ||
                  (td.m_spent_height != 0 && (td.m_spent_height < wti.height || td.m_spent_height >= local_blocks)) ||
                  td.get_asset_id() == currency::null_pkey || td.get_asset_id() != entry.asset_id || td.amount() != entry.amount)
                return snapshot_status::InvalidData;
              constexpr uint32_t reservations = WALLET_TRANSFER_DETAIL_FLAG_BLOCKED |
                WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION | WALLET_TRANSFER_DETAIL_FLAG_COLD_SIG_RESERVATION |
                WALLET_TRANSFER_DETAIL_FLAG_ASSET_OP_RESERVATION;
              constexpr uint32_t known = reservations | WALLET_TRANSFER_DETAIL_FLAG_SPENT |
                WALLET_TRANSFER_DETAIL_FLAG_MINED_TRANSFER | WALLET_TRANSFER_DETAIL_CONCISE_MODE_PRESERVE;
              const auto& txout = td.m_ptx_wallet_info->m_tx.vout[td.m_internal_output_index];
              const bool ordinary = td.is_zc() ? txout.type() == typeid(currency::tx_out_zarcanum) :
                (txout.type() == typeid(currency::tx_out_bare) &&
                 boost::get<currency::tx_out_bare>(txout).target.type() == typeid(currency::txout_to_key));
              // The resolved parent was already classified as ordinary.
              // A conflicting cached output representation is a contradiction.
              if (!ordinary) return snapshot_status::InvalidData;
              if (!(td.m_flags & ~known) && td.m_key_image != currency::null_ki)
              {
                if (!td.is_zc() && boost::get<currency::tx_out_bare>(txout).amount != td.amount())
                  return snapshot_status::InvalidData;
                const auto hold = std::lower_bound(holds_by_source.begin(), holds_by_source.end(), entry.index,
                  [&](const held_input* candidate, uint64_t index) {
                    tick(1);
                    const int order = source_compare(candidate->source_transaction, key.hash);
                    return order != 0 ? order < 0 : candidate->output_index < index;
                  });
                const bool held = hold != holds_by_source.end() && (*hold)->source_transaction == key.hash &&
                  (*hold)->output_index == entry.index;
                if (held && (*hold)->key_image != td.m_key_image) return snapshot_status::InvalidData;
                const auto image = std::lower_bound(holds_by_image.begin(), holds_by_image.end(), td.m_key_image,
                  [&](const held_input* candidate, const crypto::key_image& value) {
                    tick(1); return std::memcmp(&candidate->key_image, &value, sizeof(value)) < 0;
                  });
                if (image != holds_by_image.end() && (*image)->key_image == td.m_key_image &&
                    ((*image)->source_transaction != key.hash || (*image)->output_index != entry.index))
                  return snapshot_status::InvalidData;
                output.state = spent ? (td.m_spent_height != 0 ? owned_output_state_v1::RecordedSpent :
                  owned_output_state_v1::UnconfirmedSpendOrReservation) : owned_output_state_v1::RecordedUnspent;
                output.asset_id = entry.asset_id;
                output.amount = entry.amount;
                output.spent_height = td.m_spent_height;
                output.reservation_flags = (td.m_flags & reservations) != 0;
                output.native_held = held;
              }
            }
          }
          metadata.outputs.push_back(output);
        }
        owned_outputs->push_back(std::move(metadata));
      }
    }

    out.coverage.retained_rows = total;
    out.coverage.offset = offset;
    out.coverage.more_rows = end < total;
    if (out.coverage.more_rows)
      out.coverage.next = activity_cursor{m_generation, m_revision, m_page_capture, captured_at, end};
    return snapshot_status::Available;
  }
}}
