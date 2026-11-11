// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include "crypto/crypto.h"

namespace tools
{
  class wallet2;
  struct wallet2_base_state;
  namespace offline
  {
    class fixture_access;
    class offline_wallet_session;
    class strict_file_source;
    class deny_core_proxy;

    enum class open_result
    {
      Opened, InvalidMaterial, OccupiedOutput, NotFound, InvalidFile, WrongPassword,
      UnsupportedCapability, IdentityMismatch, GenesisMismatch, RecoveryRequired, Unavailable
    };
    enum class session_state { OpenUnchanged, Released, Faulted };
    enum class release_result { ReleasedUnchanged, AlreadyReleased, Unavailable };
    enum class info_status { Available, Unavailable };
    enum class connectivity { Offline };
    enum class synchronization { NoSynchronizationEvidence };

    struct public_info
    {
      crypto::public_key spend_public_key{};
      crypto::public_key view_public_key{};
      uint8_t flags = 0;
      crypto::hash stored_genesis{};
      connectivity connection = connectivity::Offline;
      synchronization sync = synchronization::NoSynchronizationEvidence;
    };
    struct info_result
    {
      info_status status = info_status::Unavailable;
      public_info info{};
    };

    // All controls are private, per acquisition, and contain no callbacks or
    // material. Only the unit-test fixture can configure them.
    class fixture_control final
    {
      friend class fixture_access;
      friend class offline_wallet_session;
      friend class prepared_wallet_material;
      friend class storage_lease;
      friend class strict_file_source;
      friend class deny_core_proxy;
      friend class tools::wallet2;
      friend struct tools::wallet2_base_state;
      friend open_result open_prepared(class prepared_wallet_material&,
        std::unique_ptr<offline_wallet_session>&) noexcept;
      enum class checkpoint { None, BeforeAllocation, BeforeKdf, BeforeDecrypt, AfterKeys,
        DuringBody, AfterValidation, BeforeCommit };
      enum class read_stage { Header, Keys, Body, Final };
      enum class read_fault { None, ReturnedError, ShortRead, Throw, ErrorAtEof };
      enum class decoder_fault { None, Failbit, Badbit, Throw };
      enum class decoder_stage { BeforeArchive, AfterObject };
      checkpoint fail_at = checkpoint::None;
      read_stage fail_read_stage = read_stage::Header;
      read_fault read_failure = read_fault::None;
      bool read_fault_fired = false;
      decoder_fault decoder_failure = decoder_fault::None;
      decoder_stage fail_decoder_stage = decoder_stage::BeforeArchive;
      bool close_failure = false;
      size_t file_opens = 0, file_acquisitions = 0, file_closes = 0;
      size_t header_reads = 0, key_reads = 0, body_reads = 0, final_reads = 0, clean_eof_reads = 0;
      size_t key_parses = 0, kdf_calls = 0, decrypt_attempts = 0, body_decodes = 0, body_completions = 0;
      size_t directory_creates = 0, sidecar_opens = 0, voting_reads = 0;
      size_t whitelist_fetches = 0, proxy_calls = 0, store_calls = 0;
      size_t guarded_init = 0, guarded_connection = 0, guarded_proxy = 0, guarded_whitelist = 0;
      size_t wallet_destructions = 0, lease_releases = 0, successful_releases = 0, abandonments = 0;
      size_t sequence = 0, wallet_destroy_sequence = 0, lease_release_sequence = 0;
      bool destruction_was_silenced = false;
      void reach(checkpoint point) const
      {
        if (fail_at == point) throw open_result::Unavailable;
      }
    };

    // A sealed lease token is only a fixture ownership witness. It makes no
    // production path-confinement or cross-process exclusivity claim.
    class storage_lease final
    {
    public:
      ~storage_lease() noexcept;
      storage_lease(const storage_lease&) = delete;
      storage_lease& operator=(const storage_lease&) = delete;
    private:
      friend class fixture_access;
      explicit storage_lease(std::shared_ptr<fixture_control> control) noexcept;
      std::shared_ptr<fixture_control> m_control;
    };

    class prepared_wallet_material final
    {
    public:
      prepared_wallet_material() noexcept;
      ~prepared_wallet_material() noexcept;
      prepared_wallet_material(prepared_wallet_material&&) noexcept;
      prepared_wallet_material& operator=(prepared_wallet_material&&) noexcept;
      prepared_wallet_material(const prepared_wallet_material&) = delete;
      prepared_wallet_material& operator=(const prepared_wallet_material&) = delete;
    private:
      friend class fixture_access;
      friend class offline_wallet_session;
      friend open_result open_prepared(prepared_wallet_material&,
        std::unique_ptr<offline_wallet_session>&) noexcept;
      struct payload
      {
        std::wstring path;
        std::string password;
        public_info expected;
        std::unique_ptr<storage_lease> lease;
        std::shared_ptr<fixture_control> control;
      };
      std::unique_ptr<payload> m_payload;
    };

    // Serializer observation is never serialized, reset policy, or public API.
    class load_observation final
    {
      friend class tools::wallet2;
      friend struct tools::wallet2_base_state;
      friend class fixture_access;
      bool complete = false;
      fixture_control* control = nullptr;
    };

    // Synchronous single-executor lifetime, including destruction. No pointer to
    // the owned wallet is lent out and no persistence obligation is acknowledged.
    class offline_wallet_session final
    {
    public:
      ~offline_wallet_session() noexcept;
      offline_wallet_session(const offline_wallet_session&) = delete;
      offline_wallet_session& operator=(const offline_wallet_session&) = delete;
      offline_wallet_session(offline_wallet_session&&) = delete;
      offline_wallet_session& operator=(offline_wallet_session&&) = delete;
      info_result get_info() const noexcept;
      release_result release_unchanged() noexcept;
      session_state state() const noexcept;
    private:
      friend class fixture_access;
      friend open_result open_prepared(prepared_wallet_material&,
        std::unique_ptr<offline_wallet_session>&) noexcept;
      explicit offline_wallet_session(std::shared_ptr<fixture_control> control) noexcept;
      void destroy_owned() noexcept;
      std::unique_ptr<wallet2> m_wallet;
      std::unique_ptr<prepared_wallet_material::payload> m_material;
      std::shared_ptr<fixture_control> m_control;
      public_info m_info{};
      session_state m_state = session_state::Faulted;
    };

    open_result open_prepared(prepared_wallet_material& material,
      std::unique_ptr<offline_wallet_session>& out) noexcept;
  }
}
