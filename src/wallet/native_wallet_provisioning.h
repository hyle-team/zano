// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include "native_wallet_session.h"

namespace tools
{
  namespace native_session
  {
    // Owned, non-copyable secret bytes. Cleared on reassignment, clear() and
    // destruction. Core account parsing/encryption still needs std::string
    // copies; those are wiped best-effort after use, but no complete erasure
    // (allocator, stdio, KDF or kernel copies) is claimed.
    class secret_buffer final
    {
    public:
      secret_buffer() noexcept = default;
      ~secret_buffer() noexcept;
      secret_buffer(secret_buffer&& other) noexcept;
      secret_buffer& operator=(secret_buffer&& other) noexcept;
      secret_buffer(const secret_buffer&) = delete;
      secret_buffer& operator=(const secret_buffer&) = delete;
      void assign(const char* data, std::size_t size); // May throw bad_alloc.
      void clear() noexcept;
      const char* data() const noexcept { return m_data.get(); }
      std::size_t size() const noexcept { return m_size; }
      bool empty() const noexcept { return m_size == 0; }
    private:
      std::unique_ptr<char[]> m_data;
      std::size_t m_size = 0;
    };

    // Native provider input, never a bridge DTO. The provider owns the location
    // and credentials. path names the final wallet file inside a NEW, EMPTY,
    // provider-confined directory that the provider holds exclusively; the core
    // refuses any existing entry there, including destinations, sidecars and
    // legacy temp names. The directory must be given in resolved canonical form
    // (as returned by the platform's private files-directory API); a symlinked
    // or non-canonical component is InvalidMaterial with no effect.
    // phrase/seed_password are used only for Restore and are never read once
    // commit_material has been invoked, so the provider may erase them there.
    // path and wallet_password must stay stable until the lease is destroyed.
    struct provisioning_material
    {
      std::wstring path;
      secret_buffer wallet_password;
      secret_buffer phrase;
      secret_buffer seed_password;
    };

    // Provider transaction step results use commit_status (declared with the
    // session checkpoint contract). For provisioning, Committed means the
    // provider has durably recorded the step in its own registry/protected
    // storage. Rejected means it definitely did not. Unknown (or an escaped
    // exception) means the provider cannot tell; the attempt must then be
    // recovered, never retried.

    // Terminal classification of one provisioning attempt.
    //  Ready             material persisted (file+directory fsync), provider
    //                    committed material, binding and Ready in that order,
    //                    after a real strict reopen matched the identity captured
    //                    before persistence. The reopened session is published.
    //  NotStarted        no wallet byte, file or provider commit was made by the
    //                    core. effects_possible() is false on the lease.
    //  RecoveryRequired  definite stop after core effects began. The attempt
    //                    directory may hold no file, a partial file or a
    //                    complete file; it and every provider record are kept.
    //  OutcomeUnknown    a write/sync/close or provider step result is uncertain.
    //                    Retain everything; never retry this attempt implicitly.
    //  ReadyAccessRevoked the provider committed Ready, but authorization was
    //                    lost before publication. Persistence stays committed;
    //                    no session is published. The provider must consult its
    //                    registry: the wallet exists and needs authenticated
    //                    OpenExisting, not a new Create/Restore attempt.
    enum class provision_status { Ready, NotStarted, RecoveryRequired, OutcomeUnknown, ReadyAccessRevoked };
    enum class provision_detail
    {
      None, InvalidMaterial, OccupiedOutput, UnsupportedCapability, UnsupportedInput,
      InvalidPhrase, SeedPasswordMismatch, AuthorizationUnavailable, PersistenceFailed,
      DurabilityUncertain, ProviderRejected, ProviderUncertain, ReopenFailed,
      IdentityMismatch, Unavailable
    };
    struct provision_result
    {
      provision_status status = provision_status::NotStarted;
      provision_detail detail = provision_detail::Unavailable;
    };

    class provisioning_lease;
    class wallet_provisioning;

    // Freshly generated Create backup phrase, owned natively. It is creating-
    // session material only: it is lent for the duration of one visitor call,
    // only through the same lease LIFETIME that received it (a private,
    // non-copyable identity allocated with that lease, not its address) and
    // only while that lease's authorization is current. Once that lease is
    // destroyed the backup can never be lent again, even if a new lease is
    // constructed in the same storage and wherever the backup was retained.
    // Expiry is not seed loss: the encrypted wallet keeps the native seed. A
    // later display needs a separately reviewed, freshly authenticated native
    // flow against the stored wallet; this object must not be persisted or
    // re-bound to another lease to work around that. Lending never records
    // backup acknowledgement; that is a separate provider step. A sampled
    // authorization check does not by itself order exposure against platform
    // lock/background invalidation; the display provider must serialize that.
    class generated_backup final
    {
    public:
      using visitor = void (*)(void* context, const char* phrase, std::size_t size) noexcept;
      ~generated_backup() noexcept = default;
      generated_backup(const generated_backup&) = delete;
      generated_backup& operator=(const generated_backup&) = delete;
      bool lend(const provisioning_lease& authority, visitor visit, void* context) const noexcept;
    private:
      friend class wallet_provisioning;
      generated_backup() noexcept = default;
      std::weak_ptr<const void> m_authority; // Expiring; never matches when empty.
      secret_buffer m_phrase;
    };

    // Platform-owned provisioning lease. BEFORE lending it the provider must:
    //  - durably create an IncompleteCreation registry record for this attempt;
    //  - create a new, empty, confined directory it holds exclusively, below a
    //    known durable private-storage root, and name the final wallet path in
    //    it (no endpoint/request/old file chooses it);
    //  - fix the provisioning kind and the linked compiled profile.
    // While lent it must keep path and wallet_password stable, keep exclusion
    // held until the lease is destroyed (after any session using it), never
    // reenter the core or the app port from a callback, and make every commit_*
    // step durable in its own storage before returning Committed. The core
    // never deletes or overwrites anything; recovery of RecoveryRequired/
    // OutcomeUnknown attempts (with an empty, partial or complete attempt
    // directory) is the provider's explicit responsibility. All calls and
    // destruction use one native serial executor, as for material_lease.
    class provisioning_lease
    {
    public:
      virtual ~provisioning_lease() noexcept = default;
      virtual bool authorization_current() const noexcept = 0;
      // Optional checkpoint capability for the Ready session's strict-reopen
      // material, forwarded unchanged for Create and Restore. Defaults are
      // unsupported: existing providers still provision, open, read and close
      // unchanged, but their session can never start scanning. A supporting
      // provider takes on exactly the material_lease::replace_checkpoint
      // obligations for its final wallet path (the path it lent), under the
      // exclusion it holds until this lease is destroyed, and only after
      // commit_ready returned Committed.
      virtual bool checkpoint_supported() const noexcept { return false; }
      // Set by the core immediately before it first tries to create the file,
      // and never cleared. When false after an attempt, the core created no
      // bytes in the location. This is not durability or Ready evidence.
      bool effects_possible() const noexcept { return m_effects_possible; }
      // Neither copyable nor movable: the lifetime identity cannot be moved,
      // transplanted into another lease or outlive this object.
      provisioning_lease(const provisioning_lease&) = delete;
      provisioning_lease& operator=(const provisioning_lease&) = delete;
    protected:
      // Allocates the unique lifetime identity before any core effect. May
      // throw bad_alloc, which is a provider acquisition failure.
      provisioning_lease() : m_lifetime(std::make_shared<lifetime>()) {}
      virtual acquisition_kind provisioning_kind() const noexcept = 0;
      virtual const provisioning_material& material() const = 0;
      // 1. The core has written, fsynced and checked-closed the complete file
      //    and fsynced the attempt directory (its entries). That does NOT make
      //    the attempt directory itself reachable. Before returning Committed
      //    the provider MUST make every directory link it created for this
      //    attempt (the attempt directory and each newly created ancestor)
      //    durably reachable from its known durable confined root, using
      //    checked persistence operations (fsync of each containing directory
      //    up to that root, with checked results), and only then durably record
      //    material_committed plus the expected public identity. It must not
      //    sync or traverse above that root. Return Unknown if any of this is
      //    failed or uncertain; binding and Ready are then never attempted.
      //    From this call on the core never reads phrase/seed_password.
      virtual commit_status commit_material(const public_identity& expected) = 0;
      // 2. Bind the registry account to this material. A provider that already
      //    binds the same identity elsewhere must return Rejected here.
      virtual commit_status commit_binding(const public_identity& expected) = 0;
      // 3. A strict reopen matched the pre-persistence identity. Commit Ready.
      virtual commit_status commit_ready(const public_identity& verified) = 0;
      // Create only, after commit_ready returned Committed. Must not throw.
      // The backup is lendable only while THIS lease lives and is authorized.
      virtual void retain_backup(std::unique_ptr<generated_backup> backup) noexcept = 0;
      // Checkpoint replacement for the Ready session (see material_lease).
      // `bound` is the identity the strict reopen verified against the
      // pre-persistence expectation. Default: nothing written.
      virtual commit_status replace_checkpoint(const public_identity& /*bound*/,
        const char* /*image*/, std::size_t /*size*/)
      {
        return commit_status::Rejected;
      }
    private:
      friend class wallet_provisioning;
      friend class generated_backup;
      struct lifetime final { char unused = 0; };
      const std::shared_ptr<const lifetime> m_lifetime; // Never exposed or copied out.
      bool m_effects_possible = false;
    };

    // Narrow native test seam over the five Phase B descriptor operations. The
    // public provision() always uses the real POSIX calls with production
    // flags; no bridge, JS or global switch reaches this. Only a test
    // translation unit that defines provisioning_test_access can substitute
    // them. Each hook must behave as its POSIX counterpart (result and errno);
    // close_file receives ownership and must release the descriptor exactly
    // once, whatever it reports.
    struct provisioning_io final
    {
      void* context = nullptr;
      int (*create)(void* context, int directory, const char* leaf, int flags, unsigned mode) noexcept = nullptr;
      long (*write)(void* context, int fd, const char* data, std::size_t size) noexcept = nullptr;
      int (*sync_file)(void* context, int fd) noexcept = nullptr;
      int (*close_file)(void* context, int fd) noexcept = nullptr;
      int (*sync_directory)(void* context, int fd) noexcept = nullptr;
    };
    struct provisioning_test_access; // Defined only by core unit tests.

    // Native Create/Restore of a normal custodial Zano wallet for the linked
    // MAINNET/84 or TESTNET/103 build only. Supported persistence platform:
    // Linux/Android POSIX fds with fsync of file and directory; any other
    // platform is UnsupportedCapability before effects. No legacy directory
    // creation, sidecar, voting file, wallet-manager, HTTP client, worker,
    // sync, signing or relay is used. Logging is silenced for the whole call.
    //
    // The caller keeps ownership of `lease`; it is never consumed. On Ready,
    // `out` receives a strict-reopened session that borrows the lease (its
    // authorization and checkpoint capability): close or destroy the session,
    // which destroys its scanner and wallet first, BEFORE destroying the
    // lease. A retained dirty/unknown session keeps the lease alive with it.
    // `out` must be empty.
    class wallet_provisioning final
    {
    public:
      static provision_result provision(acquisition_kind kind, provisioning_lease& lease,
        std::unique_ptr<file_session>& out) noexcept;
      // Effect-free input checks through the same core parser and password
      // policy that provision() applies. They let a provider refuse ordinary
      // invalid input before it records a durable attempt; provision() still
      // rechecks everything authoritatively. No file, provider step, lease or
      // log output is involved; the std::string copies the core APIs need are
      // wiped best-effort, as in provision().
      // wallet_password_supported: nonempty and accepted by
      // currency::validate_password (its listed ASCII alphabet, at most 40).
      static bool wallet_password_supported(const secret_buffer& password) noexcept;
      // None when RestoreNativeZano would accept this phrase and seed password
      // (word count, BIP39 shape, password flag, seed-password policy, words,
      // checksum, normal non-auditable account); otherwise the detail provision()
      // would report with NotStarted. Unavailable when the check itself failed.
      static provision_detail preflight_restore(const secret_buffer& phrase,
        const secret_buffer& seed_password) noexcept;
    private:
      friend struct provisioning_test_access;
      static provision_result provision_with_io(acquisition_kind kind, provisioning_lease& lease,
        std::unique_ptr<file_session>& out, const provisioning_io& io) noexcept;
      static provision_result run(acquisition_kind kind, provisioning_lease& lease,
        std::unique_ptr<file_session>& out, const provisioning_io& io);
    };
  }
}
