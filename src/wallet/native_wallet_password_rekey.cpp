// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
//
// Wallet-file password rekey v1 (see native_wallet_session.h). A separate unit
// of file_session: it reuses the existing checkpoint serializer
// (wallet2::render_native_checkpoint_image, which takes the password as a
// parameter and never assigns m_password, writes a file or resets account or
// genesis) and the existing strict current-format loader
// (wallet2::load_strict_current_file) on a separate scratch instance. The live
// wallet, its m_password and the lease material are never changed here; the
// paired durable publication belongs to the platform's password_rekey_port.
#include "native_wallet_session.h"
#include "wallet2.h"
#include "currency_core/account.h"
#include "currency_core/currency_format_utils.h"
#include <cstring>
#include <memory>
#include <string>

namespace tools { namespace native_session
{
  namespace
  {
    void wipe_text(std::string& value) noexcept
    {
      if (!value.empty())
      {
        volatile char* bytes = &value[0];
        for (std::size_t i = 0; i != value.size(); ++i) bytes[i] = 0;
      }
      value.clear();
    }

    // Owns one plaintext/ciphertext copy and wipes it on every exit path.
    struct wiped_text final
    {
      std::string value;
      wiped_text() = default;
      ~wiped_text() noexcept { wipe_text(value); }
      wiped_text(const wiped_text&) = delete;
      wiped_text& operator=(const wiped_text&) = delete;
    };

    // Same reentrancy marker as the session unit (provider callbacks run inside).
    struct rekey_busy_scope final
    {
      bool& busy;
      explicit rekey_busy_scope(bool& flag) noexcept : busy(flag) { busy = true; }
      ~rekey_busy_scope() noexcept { busy = false; }
      rekey_busy_scope(const rekey_busy_scope&) = delete;
      rekey_busy_scope& operator=(const rekey_busy_scope&) = delete;
    };

    bool same_identity(const public_identity& a, const public_identity& b) noexcept
    {
      return a.spend_public_key == b.spend_public_key && a.view_public_key == b.view_public_key &&
        a.flags == b.flags && a.stored_genesis == b.stored_genesis;
    }

    // Any value other than a definite Committed/Rejected is Unknown.
    commit_status definite(commit_status value) noexcept
    {
      switch (value)
      {
        case commit_status::Committed:
        case commit_status::Rejected: return value;
        case commit_status::Unknown: break;
      }
      return commit_status::Unknown;
    }

    bool password_symbol(unsigned char c) noexcept
    {
      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) return true;
      static constexpr char symbols[] = "~!?@#$%^&*_+|{}[]()<>:;\"'-=/.,";
      for (std::size_t i = 0; i + 1 < sizeof(symbols); ++i)
        if (static_cast<unsigned char>(symbols[i]) == c) return true;
      return false;
    }
  }

  bool valid_wallet_password(const char* bytes, std::size_t size) noexcept
  {
    if (!bytes || size < min_wallet_password_bytes || size > max_wallet_password_bytes) return false;
    for (std::size_t i = 0; i != size; ++i)
      if (!password_symbol(static_cast<unsigned char>(bytes[i]))) return false;
    return true;
  }

  rekey_status_v1 file_session::rekey_password_v1(rekey_admission& admission, password_rekey_port& port,
    const char* password, std::size_t size) noexcept
  {
    epee::log_space::scoped_log_silence quiet;
    // A rekey replaces the admitted file outside the replay candidate's own
    // checkpoint; no rekey authority exists while any scratch is retained.
    if (m_busy || m_replay_wallet) return rekey_status_v1::Busy;
    // Policy first: no state is read or written for a refused candidate.
    if (!valid_wallet_password(password, size)) return rekey_status_v1::InvalidPassword;
    observe_wallet();
    switch (m_state)
    {
      case session_state::OpenUnchanged:
      case session_state::OpenDirty: break;
      case session_state::RecoveryRequired: return rekey_status_v1::RecoveryRequired;
      case session_state::OutcomeUnknown: return rekey_status_v1::OutcomeUnknown;
      default: return rekey_status_v1::Unavailable;
    }
    if (!m_wallet || !m_material) return rekey_status_v1::Unavailable;
    // Only this session's own held lease may publish; a foreign port or a
    // provider without the ordinary checkpoint writer is refused untouched.
    if (port.lease() != m_material.get() || !m_material->checkpoint_supported())
      return rekey_status_v1::Unsupported;
    if (!admission.current() || !authorized()) return rekey_status_v1::Revoked;

    wiped_text candidate;
    try
    {
      candidate.value.assign(password, size);
      // The producer's own alphabet check must agree with the bounded policy.
      if (!currency::validate_password(candidate.value)) return rekey_status_v1::InvalidPassword;
      const auto& input = m_material->material();
      if (input.password.size() == candidate.value.size() &&
          std::memcmp(input.password.data(), candidate.value.data(), candidate.value.size()) == 0)
        return rekey_status_v1::SamePassword;
      const auto& profile = build_profile();
      if (profile.status != profile_status::Available || m_identity.stored_genesis != profile.genesis ||
          !same_identity(admission.subject(), m_identity) || !same_identity(input.expected, m_identity))
        return rekey_status_v1::IdentityMismatch;
      if (m_wallet->m_watch_only || m_wallet->m_account.is_watch_only())
        return rekey_status_v1::Unsupported;
    }
    catch (...) { return rekey_status_v1::Unavailable; } // never inspect exception text

    // Dirty/pending settlement: unsaved clean progress is saved under the
    // CURRENT password through the ordinary writer, so the image rendered
    // below is exactly what the file then holds. Nothing else is accepted.
    if (m_wallet->m_read_only_sync_dirty)
    {
      switch (write_checkpoint())
      {
        case checkpoint_result::Saved:
        case checkpoint_result::Unchanged: break;
        case checkpoint_result::OutcomeUnknown: return rekey_status_v1::OutcomeUnknown;
        case checkpoint_result::RecoveryRequired:
        case checkpoint_result::IdentityMismatch: return rekey_status_v1::RecoveryRequired;
        case checkpoint_result::Busy: return rekey_status_v1::Busy;
        default: return rekey_status_v1::NotSettled;
      }
    }
    observe_wallet();
    if (m_state != session_state::OpenUnchanged || m_wallet->m_read_only_sync_dirty)
      return rekey_status_v1::NotSettled;
    // One use, consumed only once nothing earlier can refuse.
    if (!admission.current() || !authorized() || !admission.consume()) return rekey_status_v1::Revoked;

    // ---- Render the CURRENT wallet under the new password (memory only). ----
    wiped_text image;
    wallet2::native_image_status rendered = wallet2::native_image_status::Unavailable;
    try
    {
      rekey_busy_scope guard(m_busy);
      currency::account_public_address expected{};
      expected.spend_public_key = m_identity.spend_public_key;
      expected.view_public_key = m_identity.view_public_key;
      expected.flags = m_identity.flags;
      rendered = m_wallet->render_native_checkpoint_image(expected, m_identity.stored_genesis,
        candidate.value, m_image_limit, image.value);
    }
    catch (...) { rendered = wallet2::native_image_status::Unavailable; }
    switch (rendered)
    {
      case wallet2::native_image_status::Rendered: break;
      case wallet2::native_image_status::IdentityMismatch:
        mark_recovery();
        return rekey_status_v1::IdentityMismatch;
      case wallet2::native_image_status::UnsupportedState: return rekey_status_v1::Unsupported;
      default: return rekey_status_v1::Unavailable; // LimitExceeded/Unavailable: nothing staged.
    }

    // Precommit residue is the provider's own; a discard that is not definite
    // leaves an intent, so this session must never checkpoint again.
    const auto discarded = [&](rekey_status_v1 outcome) noexcept -> rekey_status_v1
    {
      commit_status removed = commit_status::Unknown;
      {
        rekey_busy_scope guard(m_busy);
        try { removed = definite(port.discard()); }
        catch (...) { removed = commit_status::Unknown; }
      }
      if (removed != commit_status::Committed)
      {
        m_state = session_state::OutcomeUnknown;
        return rekey_status_v1::OutcomeUnknown;
      }
      return outcome;
    };

    // ---- Stage: durable intent + staged encrypted image (live pair untouched).
    commit_status staged = commit_status::Unknown;
    {
      rekey_busy_scope guard(m_busy);
      try
      {
        staged = definite(port.stage(m_identity, image.value.data(), image.value.size(),
          candidate.value.data(), candidate.value.size()));
      }
      catch (...) { staged = commit_status::Unknown; }
    }
    wipe_text(image.value);
    if (staged == commit_status::Rejected) return rekey_status_v1::Rejected;
    if (staged != commit_status::Committed)
    {
      m_state = session_state::OutcomeUnknown;
      return rekey_status_v1::OutcomeUnknown;
    }

    // ---- Validate: the staged bytes strictly reopen as exactly this wallet.
    bool valid = false;
    try
    {
      rekey_busy_scope guard(m_busy);
      const std::wstring* path = port.staged_path();
      if (path && !path->empty() && *path != m_material->material().path)
      {
        std::unique_ptr<wallet2> check(new wallet2(wallet2::native_file_construction_tag{}));
        check->load_strict_current_file(*path, candidate.value);
        const auto& address = check->m_account.get_public_address();
        const auto& origin = check->m_native_history_origin;
        const auto& live = m_wallet->m_native_history_origin;
        valid = address.spend_public_key == m_identity.spend_public_key &&
          address.view_public_key == m_identity.view_public_key && address.flags == m_identity.flags &&
          !check->m_watch_only && !check->m_account.is_watch_only() &&
          check->m_chain.get_genesis() == m_identity.stored_genesis &&
          check->m_account.get_createtime() == m_wallet->m_account.get_createtime() &&
          check->m_minimum_height == m_wallet->m_minimum_height &&
          check->m_chain.get_blockchain_current_size() == m_wallet->m_chain.get_blockchain_current_size() &&
          check->m_chain.get_top_block_id() == m_wallet->m_chain.get_top_block_id() &&
          check->m_transfers.size() == m_wallet->m_transfers.size() &&
          check->m_transfer_history.size() == m_wallet->m_transfer_history.size() &&
          origin.phase == live.phase && origin.rounded_date == live.rounded_date &&
          origin.first_height == live.first_height && origin.anchor_hash == live.anchor_hash &&
          origin.view.target_height == live.view.target_height &&
          origin.view.target_hash == live.view.target_hash &&
          origin.view.genesis_hash == live.view.genesis_hash && origin.view.token == live.view.token &&
          origin.connected_height == live.connected_height && origin.connected_hash == live.connected_hash;
      }
    }
    catch (...) { valid = false; } // wrong password, strict-format or I/O failure: never accepted
    if (!valid) return discarded(rekey_status_v1::ValidationFailed);
    // Fresh samples immediately before the commit point.
    if (!admission.current() || !authorized()) return discarded(rekey_status_v1::Revoked);
    observe_wallet();
    if (m_state != session_state::OpenUnchanged) return discarded(rekey_status_v1::NotSettled);

    // ---- Commit point and paired replacement (provider-owned). ----
    commit_status published = commit_status::Unknown;
    {
      rekey_busy_scope guard(m_busy);
      try { published = definite(port.publish(m_identity)); }
      catch (...) { published = commit_status::Unknown; }
    }
    switch (published)
    {
      case commit_status::Committed:
        // The new pair is authoritative. The held material still names the old
        // password, so nothing of this session may render again: release the
        // scanner, wallet and lease now (destruction never stores).
        release_owned();
        return rekey_status_v1::Committed;
      case commit_status::Rejected:
        return discarded(rekey_status_v1::Rejected);
      default:
        m_state = session_state::OutcomeUnknown;
        return rekey_status_v1::OutcomeUnknown;
    }
  }
}}
