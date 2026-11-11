// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "crypto/crypto.h"
#include "read_only_wallet_sync.h"
#include "native_payment_transport.h"

// Compile-time capability marker for the additive product snapshot v4 below.
// Consumers built against an SDK without it keep the v3 capture unchanged.
#define ZANO_NATIVE_SESSION_PRODUCT_SNAPSHOT_V4 1
// Compile-time capability marker for the additive alias operation v1 below
// (prepare_alias_review_v1 / sign_reviewed_alias_to_held_v1). Consumers built
// against an SDK without it keep every existing payment/snapshot path unchanged.
#define ZANO_NATIVE_SESSION_ALIAS_OPERATION_V1 1
// Compile-time capability marker for the additive wallet-file password rekey
// v1 below (rekey_password_v1 / password_rekey_port / rekey_admission).
// material_lease and every existing entry keep their exact ABI.
#define ZANO_NATIVE_SESSION_PASSWORD_REKEY_V1 1

namespace currency
{
  class transaction;
  struct finalize_tx_param;
}
namespace tools
{
  namespace wallet_public { struct wallet_transfer_info; }
  class wallet2;
  struct i_core_proxy;
  namespace native_payment { class decoy_exchange; }
  namespace native_session
  {
    enum class acquisition_kind { OpenExisting, CreateNativeZano, RestoreNativeZano };
    enum class open_result
    {
      Opened, InvalidMaterial, OccupiedOutput, NotFound, InvalidFile, WrongPassword,
      UnsupportedCapability, IdentityMismatch, GenesisMismatch, RecoveryRequired,
      AuthorizationUnavailable, InvalidAddress, Unavailable
    };
    //  OpenUnchanged     open; nothing differs from the last acknowledged file.
    //  OpenDirty         open; clean scanned progress awaits a checkpoint.
    //  RecoveryRequired  sticky: the wallet latched a processing fault or failed
    //                    its identity/profile check. Never saved or released as
    //                    unchanged; resources are retained for the owner.
    //  OutcomeUnknown    sticky: a checkpoint replacement result is uncertain.
    //                    Retained until the owner reconciles the target file.
    //  Released          explicitly closed; Faulted: never opened.
    // Appended values; existing comparisons with OpenUnchanged keep meaning.
    enum class session_state { OpenUnchanged, Released, Faulted, OpenDirty, RecoveryRequired, OutcomeUnknown };
    enum class release_result { ReleasedUnchanged, AlreadyReleased, Unavailable };
    enum class receive_status { Available, Unavailable };

    // Provider transaction step results, shared by provisioning commits and
    // session checkpoints. Committed means the provider durably completed the
    // step. Rejected means it definitely did not (for a checkpoint: the target
    // still holds its previous valid bytes). Unknown, an escaped exception or
    // any other value means the provider cannot tell; it is never retried.
    enum class commit_status { Committed, Rejected, Unknown };

    // Hard upper bound of one rendered checkpoint image. Larger wallets are
    // refused before the provider is called and remain dirty and retained.
    constexpr std::size_t max_checkpoint_image_bytes = std::size_t(64) * 1024 * 1024;

    //  Prepared              scanner (re)created; minimum height fixed at zero.
    //  Revoked               authorization is (or was ever) lost; nothing done.
    //  CheckpointUnsupported the provider cannot persist progress; nothing done.
    //  InvalidTransport      no transport; nothing done.
    //  UnsupportedState      not a scannable native wallet (profile/genesis,
    //                        watch-only, legacy minimum height, sticky fault).
    //  Busy                  a step/checkpoint of this session is running.
    //  Unavailable           not open, or allocation failed.
    enum class sync_prepare_result
    {
      Prepared, Revoked, CheckpointUnsupported, InvalidTransport, UnsupportedState, Busy, Unavailable
    };
    //  Stepped    the scanner ran; scan holds its own typed result.
    //  Revoked    authority was lost before or after the scan. If the scan ran,
    //             scan still reports it (state_may_have_changed) so the owner
    //             can drain, but nothing may be published from it.
    //  NotPrepared/Busy/RecoveryRequired/OutcomeUnknown/Unavailable: no scan.
    enum class sync_step_status { Stepped, Revoked, NotPrepared, Busy, RecoveryRequired, OutcomeUnknown, Unavailable };
    struct sync_step_result
    {
      sync_step_status status = sync_step_status::Unavailable;
      read_only::step_result scan{};
    };

    //  Saved            provider Committed the current image; dirty cleared.
    //  Unchanged        nothing to save; the provider was not called.
    //  Rejected         provider definitely left the target unchanged. Dirty
    //                   state, scanner and material are retained; retryable.
    //  ImageLimit       image exceeds the bound; provider not called; retained.
    //  OutcomeUnknown   writer threw or reported Unknown. Sticky; no retry.
    //  RecoveryRequired faulted wallet; never rendered or written.
    //  IdentityMismatch wallet/lease identity or genesis drifted from the
    //                   verified identity; never written; becomes recovery.
    //  Unsupported      provider has no checkpoint capability; retained.
    //  Busy             called from inside a running step/checkpoint.
    //  Unavailable      not open, or rendering failed before the provider call
    //                   (target untouched, dirty retained).
    enum class checkpoint_result
    {
      Saved, Unchanged, Rejected, ImageLimit, OutcomeUnknown, RecoveryRequired,
      IdentityMismatch, Unsupported, Busy, Unavailable
    };
    //  ReleasedUnchanged nothing to save; resources released.
    //  ReleasedSaved     a checkpoint was Committed, then resources released.
    //  RetainedFailure   definite checkpoint failure; everything retained.
    //  OutcomeUnknown    uncertain replacement (now or earlier); retained.
    //  RecoveryRequired  faulted/identity-drifted wallet; retained, not saved.
    //  AlreadyReleased/Busy/Unavailable as for release.
    enum class close_result
    {
      ReleasedUnchanged, ReleasedSaved, AlreadyReleased, RetainedFailure, OutcomeUnknown,
      RecoveryRequired, Busy, Unavailable
    };

    enum class compiled_network { Mainnet, Testnet };
    enum class profile_status { Available, Unsupported, Unavailable };

    // Chain identity of THIS build. Genesis is generated from the compiled
    // genesis transaction and fixed block inputs with the existing core
    // primitives. It is source-derived authority only, never read from the
    // mutable get_genesis_hash() cache, a wallet file or an endpoint, and it is
    // not an independently verified known-answer or node identity.
    struct compiled_profile
    {
      profile_status status = profile_status::Unavailable;
      compiled_network network = compiled_network::Mainnet;
      uint64_t formation = 0;
      crypto::hash genesis{};
    };

    // Derived once per process and immutable afterwards. Only MAINNET/84 and
    // TESTNET/103 builds are Available; any other formation is Unsupported and
    // reported generation/hash failure is Unavailable, both with a null genesis.
    // Call only after normal runtime initialization, never from a static
    // initializer: MAINNET agreement uses a dynamically initialized literal.
    // A first-call derivation failure remains cached until process restart.
    const compiled_profile& build_profile() noexcept;
    // Uncached rederivation from the same compiled inputs, for regression
    // checks that the descriptor cannot follow the legacy getter override.
    compiled_profile derive_build_profile() noexcept;

    struct public_identity
    {
      crypto::public_key spend_public_key{};
      crypto::public_key view_public_key{};
      uint8_t flags = 0;
      crypto::hash stored_genesis{};
    };

    // Native provider input, never a bridge DTO. Expected identity comes from the
    // provider's committed registry/profile, independently of the file being read.
    // This type does not claim secure erasure of std::string storage.
    struct open_material
    {
      std::wstring path;
      std::string password;
      public_identity expected;
    };

    // Native signing-to-held only. The SDK never dispatches or exports a signed
    // payload. Input identities remain meaningful after transfer-index changes.
    struct held_input
    {
      crypto::hash source_transaction{};
      uint64_t output_index = 0;
      crypto::key_image key_image{};
    };
    enum class held_inputs_status { Loaded, Unsupported, RecoveryRequired, Unavailable };
    struct held_inputs_result
    {
      held_inputs_status status = held_inputs_status::Unavailable;
      std::vector<held_input> inputs;
    };
    struct signed_payment
    {
      std::string saved_draft; // Exact immutable native-store encoding.
      std::string transaction_blob;
      crypto::hash transaction_hash{};
      crypto::secret_key transaction_key{};
      std::vector<held_input> inputs;
      std::string checkpoint_image; // Existing encrypted wallet format.
      uint64_t generation = 0, revision = 0, amount = 0, fee = 0;
      public_identity identity{};
      signed_payment() = default;
      signed_payment(const signed_payment&) = delete;
      signed_payment& operator=(const signed_payment&) = delete;
      ~signed_payment() noexcept;
    };
    struct held_payment_limits
    {
      std::size_t transaction_bytes = 0, inputs = 0, checkpoint_bytes = 0;
    };
    held_payment_limits signing_limits() noexcept;
    // Fresh native authentication + same-key protected use, issued for one
    // exact saved draft and current verified session. No UI boolean is accepted.
    class signing_admission
    {
    public:
      virtual ~signing_admission() noexcept = default;
      virtual bool current() const noexcept = 0;
      virtual const public_identity& subject() const noexcept = 0;
      virtual const crypto::hash& saved_draft_hash() const noexcept = 0;
      virtual bool consume() noexcept = 0;
    protected:
      signing_admission() = default;
    };
    class file_session;
    // The provider establishes private-path confinement and exclusive ownership
    // by material identity before lending this lease, and maintains both until
    // destruction. The strict reader's path preflight is NOT a race-safe vault.
    // All calls and destruction use one native executor. Provider state/material
    // must stay stable except for revocation; methods must not reenter this API.
    // Native owner publication and revocation must be serialized externally:
    // authorization_current() is a sampled predicate, not a publication lock.
    //
    // Checkpoint capability (optional; the defaults are unsupported, so an
    // existing Open/Receive/unchanged-close provider compiles and behaves as
    // before and can never start scanning). A provider returning true from
    // checkpoint_supported() accepts, through replace_checkpoint(), ONLY a
    // complete current-format encrypted image of the wallet this lease is
    // already bound to. It must:
    //  - verify `bound` equals its committed identity for this material and
    //    still hold the same exclusion it established before lending;
    //  - write to its own canonical, confined target (the material path it
    //    lent; never a path chosen by the image, an endpoint or the core);
    //  - create an exclusive no-follow temporary in the target's directory,
    //    write all bytes with checked results, fsync it and close it checked
    //    exactly once, replace the target with ONE atomic rename, then fsync
    //    the directory;
    //  - return Committed only after all of that succeeded; Rejected only when
    //    it definitely failed before the replacement, so the previous valid
    //    target is intact (a leftover temporary is its own cleanup concern);
    //    Unknown (or throw) whenever the replacement or directory fsync failed
    //    or cannot be classified.
    // The image pointer is borrowed for the call; it must not be retained. The
    // method is called on the owning executor, possibly after authorization
    // was revoked (drain authority, see file_session), and must not reenter
    // the core. No second rename, backup copy or sidecar is part of this.
    class material_lease
    {
    public:
      virtual ~material_lease() noexcept = default;
      virtual bool authorization_current() const noexcept = 0;
      virtual bool checkpoint_supported() const noexcept { return false; }
      // The concrete platform loads authenticated operation obligations under
      // its exclusive material writer BEFORE lending this lease. Old providers
      // without signing remain compatible with read-only Open/Receive.
      virtual held_inputs_result held_inputs() const { return {held_inputs_status::Unsupported, {}}; }
      virtual bool signed_payment_supported() const noexcept { return false; }

      material_lease(const material_lease&) = delete;
      material_lease& operator=(const material_lease&) = delete;
    protected:
      material_lease() = default;
      virtual const open_material& material() const = 0;
      virtual commit_status replace_checkpoint(const public_identity& /*bound*/,
        const char* /*image*/, std::size_t /*size*/)
      {
        return commit_status::Rejected; // Unsupported: nothing written.
      }
      // One owned mutation protocol, never general persistence/paths. The
      // platform binds predecessor and planned temporary at Prepared publication.
      // Prepared is the FIRST durable publication and consumes the saved draft.
      // It commits the authenticated artifact + encrypted postimage before
      // any wallet replacement. Completed commits the compact Held record only
      // after exact image replacement and durable directory confirmation.
      virtual commit_status prepare_signed_payment(const signed_payment&) { return commit_status::Rejected; }
      virtual commit_status replace_signed_checkpoint(const signed_payment&) { return commit_status::Rejected; }
      virtual commit_status complete_signed_payment(const signed_payment&) { return commit_status::Rejected; }
    private:
      friend class file_session;
    };

    struct receive_result
    {
      receive_status status = receive_status::Unavailable;
      std::string address;
      public_identity identity{};
    };

    // Fresh native authority for ONE backup reveal on an already-open session.
    // It is deliberately distinct from the open material lease: a current open
    // authorization never implies reveal. subject() is the identity the
    // provider independently committed for the account the reveal was admitted
    // for, never a value read back from the wallet file. The provider owns its
    // lifetime, ordering against platform lock/background, and revocation.
    class backup_admission
    {
    public:
      virtual ~backup_admission() noexcept = default;
      virtual bool admission_current() const noexcept = 0;
      virtual const public_identity& subject() const noexcept = 0;
      backup_admission(const backup_admission&) = delete;
      backup_admission& operator=(const backup_admission&) = delete;
    protected:
      backup_admission() = default;
    };

    //  Presented        the visitor was called exactly once.
    //  Revoked          backup admission or open authorization is not current.
    //  IdentityMismatch the admission subject, the verified session identity
    //                   or the rendered phrase's recovered keys disagree.
    //  NoSeed           the stored account carries no native seed.
    //  Unsupported      watch-only/auditable account (never provisioned here).
    //  InvalidVisitor   no visitor; nothing rendered.
    //  Unavailable      the session is not open, or rendering failed.
    enum class backup_result
    {
      Presented, Revoked, IdentityMismatch, NoSeed, Unsupported, InvalidVisitor, Unavailable
    };
    // Borrowed pointer/length valid only for the duration of the call. The
    // visitor must copy into its own native presentation storage (which it
    // owns and wipes) and must not retain the pointer or reenter the core.
    using backup_visitor = void (*)(void* context, const char* phrase, std::size_t size) noexcept;

    // ---- Detached public snapshot -------------------------------------------
    //
    // One coherent, owned observation of the open wallet's LOCAL state: public
    // identity, compiled profile, local chain position, raw per-asset amounts
    // and one page of retained local activity. It is built completely from one
    // serialized state, checked, and only then published by move.
    //
    // What it is NOT: synchronization, freshness, complete history, pool state
    // or spending readiness. The block scanner never observes the pool, local
    // history may be truncated/pruned, and nothing here was checked against a
    // node. A snapshot never calls checkpoint/store/refresh/scan/send, contacts
    // a proxy, fetches whitelist/alias data or changes a wallet cache.
    //
    //  Available        payload present. Every other value has NO payload.
    //  Busy             called from inside a running step/checkpoint/snapshot.
    //  Revoked          open authorization is (or was ever) lost, including at
    //                   the final sample after the observation was built.
    //  RecoveryRequired sticky wallet fault; OutcomeUnknown: sticky uncertain
    //                   checkpoint. Neither is ever described.
    //  Unavailable      not open, no core clock, or allocation failure.
    //  InvalidRequest   page size outside 1..max_activity_page, or a
    //                   continuation with a zero offset.
    //  StaleCursor      continuation from another session generation, another
    //                   state revision, a superseded first page, a different
    //                   capture time, or past the retained rows.
    //  LimitExceeded    a documented bound below would be exceeded (nothing is
    //                   cut to fit), or a session counter is exhausted.
    //  InvalidData      local data is inconsistent: checked arithmetic would
    //                   wrap/underflow, an index is out of range, a confirmed
    //                   height is not below the local block count, a pending
    //                   record carries a height, a pending key differs from its
    //                   record, a duplicate transaction identity, a null hash,
    //                   an underivable fee, known local precision outside
    //                   uint8_t, a null or contradictory local tip identity, or
    //                   identity/genesis drift (which checkpoint/close still
    //                   detect and latch themselves).
    //  UnsupportedData  a retained pending record whose accounting this core
    //                   deliberately refuses to guess: any escrow contract
    //                   record, or one transaction carrying BOTH directions for
    //                   the same asset (across or within payment-id groups).
    enum class snapshot_status
    {
      Available, Busy, Revoked, RecoveryRequired, OutcomeUnknown, Unavailable,
      InvalidRequest, StaleCursor, LimitExceeded, InvalidData, UnsupportedData
    };

    // Finite bounds. Collections are checked before allocation/insertion.
    constexpr std::size_t max_snapshot_assets = 256;           // Including native.
    constexpr std::size_t max_activity_page = 100;
    constexpr std::size_t default_activity_page = 50;
    // Per collection: outputs, retained history, retained pending records.
    constexpr std::size_t max_snapshot_records = 100000;
    constexpr std::size_t max_activity_legs = 128;             // Per transaction.
    constexpr std::size_t max_activity_counterparties = 32;    // Per transaction.
    constexpr std::size_t max_counterparty_bytes = 256;
    // Aggregate per capture over every traversed payment-id group, leg,
    // employed entry, fee-derivation input/output/extra element, unlock-time
    // extra element and referenced per-output unlock array (charged once per
    // confirmed output, even when outputs share a transaction) and recorded
    // counterparty, so the per-collection bound cannot hide an inner loop.
    constexpr std::size_t max_snapshot_nested_elements = 262144;

    // Exact raw provider fields in atomic units, per asset, from checked
    // arithmetic over fresh local state (never a shared cache):
    //  raw_total        confirmed spendable-or-escrow-reserved unspent outputs,
    //                   plus retained pending incoming legs, plus pending
    //                   change (receive entries of a pending transaction whose
    //                   own legs for that asset are outgoing or absent).
    //  raw_unlocked     the confirmed part of raw_total that is unlocked at the
    //                   snapshot's ONE captured core time and local block count.
    //  raw_awaiting_in  retained pending incoming legs (already in raw_total).
    //  raw_awaiting_out retained pending outgoing legs; for native, the sum of
    //                   one transaction's outgoing native legs minus that
    //                   transaction's checked derived fee.
    // These are NOT the app's normalized settled total, reservations, fee
    // sufficiency, fiat value or Max; a reviewed adapter derives those.
    struct asset_amounts
    {
      crypto::public_key asset_id{};
      uint64_t raw_total = 0;
      uint64_t raw_unlocked = 0;
      uint64_t raw_awaiting_in = 0;
      uint64_t raw_awaiting_out = 0;
      // Local metadata only (native descriptor, local whitelist copy, custom
      // or own asset). Unknown precision stays unknown, never zero. Known
      // precision is the descriptor's uint8_t as recorded (0..255; core's
      // descriptor validation sets no lower ceiling). It is display metadata:
      // the raw fields above are exact atomic units, never scaled by it.
      bool decimals_known = false;
      uint8_t decimals = 0;
    };

    // Derived from the SOURCE container, never from a height value.
    enum class activity_status { RetainedPending, RetainedConfirmed };
    //  NotRecorded    the wallet recorded no fee (zero): not proof of a free
    //                 transaction.
    //  WalletRecorded the wallet's own recorded, wallet-attributed native fee.
    //                 It is not necessarily the full network fee (escrow can
    //                 change it) and says nothing about who paid it.
    enum class fee_record { NotRecorded, WalletRecorded };

    // One wallet-relative leg, flattened from the recorded payment-id groups
    // in recorded order. No payment-id bytes are exposed.
    struct activity_leg
    {
      crypto::public_key asset_id{};
      bool incoming = false;
      uint64_t amount = 0;
    };

    struct activity_row
    {
      crypto::hash tx_hash{};
      activity_status status = activity_status::RetainedPending;
      bool timestamp_known = false;    // Recorded zero is unknown.
      uint64_t timestamp = 0;
      // RetainedConfirmed only: recorded height (< local_blocks) and checked
      // local_blocks - height. Zero for pending rows.
      uint64_t confirmed_height = 0;
      uint64_t confirmations = 0;
      fee_record fee_source = fee_record::NotRecorded;
      uint64_t recorded_fee = 0;       // Separate from legs; never folded in.
      std::vector<activity_leg> legs;
      // Recorded counterparties of the TRANSACTION (not mapped to legs). No
      // alias lookup; empty means none recorded, not "no counterparty".
      std::vector<std::string> counterparties;
    };

    // Native continuation data, not authorization. It binds the owning session
    // generation, state revision, first-page capture id, the frozen capture
    // time and the next offset. It is valid only while ALL of these hold:
    //  - the same live session (a reopened session has a new generation);
    //  - no scan step reported a possible state change since the first page;
    //  - no newer first page was requested on this session;
    //  - the offset is still inside the retained rows.
    // The session retains only the latest first page's capture id and capture
    // time between calls; no rows, raw transactions or wallet state are kept.
    struct activity_cursor
    {
      uint64_t generation = 0;
      uint64_t revision = 0;
      uint64_t capture = 0;
      uint64_t captured_at = 0;
      uint64_t offset = 0;
    };

    struct snapshot_request
    {
      std::size_t page_size = default_activity_page;
      bool continuation = false;       // false: a new first page.
      activity_cursor cursor{};        // Used only for a continuation.
    };

    // Deterministic newest-first order: retained pending before retained
    // confirmed; then descending recorded timestamp (unknown zero sorts last),
    // descending confirmed height, then ascending raw tx hash bytes.
    struct activity_coverage
    {
      uint64_t retained_rows = 0;      // Pending + confirmed retained locally.
      uint64_t offset = 0;
      bool more_rows = false;          // next is valid only when true.
      activity_cursor next{};
      // Fixed facts, carried as values: a full retained page is NOT the full
      // account history, and the pool was not observed by this scanner.
      bool retained_local_only = true;
      bool history_completeness_known = false;
      bool pool_observed = false;
    };

    struct public_snapshot
    {
      public_identity identity{};      // Verified at open; rechecked here.
      compiled_network network = compiled_network::Mainnet;
      uint64_t formation = 0;
      crypto::hash genesis{};
      uint64_t session_generation = 0; // Unique per opened session in process.
      uint64_t revision = 0;           // Session-local; see file_session.
      uint64_t captured_at = 0;        // Core time used for every unlock check.
      uint64_t local_blocks = 0;       // >= 1 (genesis).
      uint64_t local_tip_height = 0;   // local_blocks - 1.
      // Recent tip hash retained by the wallet's chain index. A genesis-only
      // chain (and a tip beyond the retained index) has none; it is never
      // substituted with the genesis or another hash.
      bool local_tip_hash_known = false;
      crypto::hash local_tip_hash{};
      // Ascending asset-id bytes; always includes an explicit native entry.
      std::vector<asset_amounts> assets;
      std::vector<activity_row> activity;
      activity_coverage coverage{};
    };

    struct snapshot_result
    {
      snapshot_status status = snapshot_status::Unavailable;
      std::unique_ptr<public_snapshot> observation; // Non-null iff Available.
    };

    // Versioned normal-product snapshot extension. Existing public_snapshot,
    // snapshot_result and capture_snapshot ABI remain unchanged.
    enum class scan_start_source_v1 { Unobserved, SeedDate, GenesisFallback, SavedBoundary };
    struct scan_coverage_v1
    {
      uint32_t version = 1;
      scan_start_source_v1 source = scan_start_source_v1::Unobserved;
      uint64_t minimum_height = 0; // Meaningful only for an observed source.
      bool checkpointed = false;  // Same captured revision was definitely clean.
    };
    struct product_snapshot_result_v1
    {
      uint32_t version = 1;
      snapshot_status status = snapshot_status::Unavailable;
      std::unique_ptr<public_snapshot> observation;
      scan_coverage_v1 scan_coverage{};
    };

    // Additive per-owned-output observations from this SAME product capture.
    // Unknown carries its recorded index only; all other fields stay default.
    // These are local wallet records, never spendability, pool or release facts.
    enum class owned_output_state_v1
    {
      Unknown, RecordedUnspent, RecordedSpent, UnconfirmedSpendOrReservation
    };
    struct owned_output_v1
    {
      uint64_t output_index = 0;
      owned_output_state_v1 state = owned_output_state_v1::Unknown;
      crypto::public_key asset_id{};
      uint64_t amount = 0;
      uint64_t spent_height = 0;
      bool reservation_flags = false;
      bool native_held = false;
    };
    struct activity_owned_outputs_v1
    {
      crypto::hash tx_hash{};
      std::vector<owned_output_v1> outputs;
    };
    struct product_snapshot_result_v2
    {
      uint32_t version = 2;
      snapshot_status status = snapshot_status::Unavailable;
      std::unique_ptr<public_snapshot> observation;
      scan_coverage_v1 scan_coverage{};
      std::vector<activity_owned_outputs_v1> owned_outputs;
    };

    // Stored native account value, observed inside the same product capture.
    // Zero is unavailable. A retained nonzero timestamp is exact: it is not
    // rewritten, decoded from a phrase or evidence of historical seed origin.
    struct retained_account_birthday_v1
    {
      uint32_t version = 1;
      bool available = false;
      uint64_t timestamp = 0;
    };
    struct product_snapshot_result_v3
    {
      uint32_t version = 3;
      snapshot_status status = snapshot_status::Unavailable;
      std::unique_ptr<public_snapshot> observation;
      scan_coverage_v1 scan_coverage{};
      std::vector<activity_owned_outputs_v1> owned_outputs;
      retained_account_birthday_v1 retained_birthday{};
    };

    // Copy of the AUTHORITATIVE wallet's existing native History-origin record,
    // validated inside the same product capture against the verified identity,
    // the account's own native rounded seed date, the captured scan minimum and
    // the captured local tip. Nothing is selected, scanned, rewritten or
    // inferred here; a legacy image's transient replay candidate is never
    // described before its existing durable admission replaced the wallet.
    //  Unavailable  no payload: every value below stays zero/null. reason says
    //               why (Absent: no record; ScratchReplay: a candidate replay
    //               exists and the authoritative wallet has no lineage;
    //               Unverified: a present record failed a same-capture check).
    //  Scanning     the producer's validated interval has not yet reached its
    //               pinned view. Cancellation, truncation and partial steps
    //               stay here: this capture never promotes a phase.
    //  Complete     the producer marked the validated interval connected
    //               through its pinned view target, and it still reaches the
    //               captured local tip. Complete means that captured interval
    //               ONLY: not global freshness, consensus/finality, full
    //               independent history, original phrase provenance or any
    //               financial authority.
    // checkpointed repeats the same revision's clean fact; false means dirty
    // progress still awaits a Committed checkpoint.
    enum class history_interval_state_v1 { Unavailable, Scanning, Complete };
    enum class history_interval_reason_v1 { Observed, Absent, ScratchReplay, Unverified };
    struct native_history_interval_v1
    {
      uint32_t version = 1;
      history_interval_state_v1 state = history_interval_state_v1::Unavailable;
      history_interval_reason_v1 reason = history_interval_reason_v1::Absent;
      uint64_t rounded_date = 0;       // The record's own; never recomputed into it.
      uint64_t first_height = 0;       // >= 1; first_height - 1 is the anchor.
      crypto::hash anchor_hash{};
      uint64_t view_height = 0;        // Pinned producer view target.
      crypto::hash view_hash{};
      uint64_t connected_height = 0;   // Equals the captured local tip height.
      crypto::hash connected_hash{};
      bool checkpointed = false;
    };
    struct product_snapshot_result_v4
    {
      uint32_t version = 4;
      snapshot_status status = snapshot_status::Unavailable;
      std::unique_ptr<public_snapshot> observation;
      scan_coverage_v1 scan_coverage{};
      std::vector<activity_owned_outputs_v1> owned_outputs;
      retained_account_birthday_v1 retained_birthday{};
      native_history_interval_v1 history_interval{};
    };

    // Local, detached advisory only. Never a quote, reservation, constructible
    // Max, live freshness claim or authority to construct/sign/submit. Every
    // refusal has no payload. Retained pending transactions are conservatively
    // unsupported in this first slice, including incoming/change records.
    enum class payment_status
    {
      LocalAdvisory, Busy, Revoked, RecoveryRequired, OutcomeUnknown, Unavailable,
      StaleObservation, UnsupportedProfile, UnsupportedWallet, IdentityMismatch,
      InvalidRecipient, InvalidAmount, AmountOverflow, RetainedPendingUnsupported,
      LimitExceeded, InvalidData, UnsupportedData
    };
    constexpr std::size_t max_payment_recipient_bytes = 256;
    struct payment_request
    {
      // Exact canonical ordinary account address only. Its text has no network
      // binding; TESTNET admission comes from the verified session profile.
      std::string recipient;
      uint64_t amount = 0;   // Positive native atomic units, never display units.
      uint64_t expected_generation = 0;
      uint64_t expected_revision = 0;
    };
    struct local_payment_assessment
    {
      std::string canonical_recipient;
      crypto::public_key asset_id{};
      uint8_t decimals = 0; // Compiled native precision, not wallet metadata.
      uint64_t amount = 0;
      uint64_t compiled_policy_fee = 0; // Core policy only, not a live quote.
      uint64_t amount_plus_fee = 0;
      uint64_t eligible_local_capacity = 0;
      uint64_t eligible_output_count = 0;
      uint64_t bare_decoy_policy = 0; // Compiled nominal bare-output count.
      uint64_t confidential_decoy_policy = 0; // Core confidential minimum.
      uint64_t advisory_amount_upper_bound = 0; // Capacity minus policy fee, clamped at zero.
      // Even true proves only this bounded local sum. Mandatory confidential
      // inputs, transaction limits and available decoys remain unchecked.
      bool local_capacity_covers_amount_and_fee = false;
      bool pool_observed = false;
      bool live_freshness_verified = false;
      uint64_t session_generation = 0;
      uint64_t revision = 0;
      uint64_t captured_at = 0;
      uint64_t local_blocks = 0;
      compiled_network network = compiled_network::Mainnet;
      uint64_t formation = 0;
      crypto::hash genesis{};
    };
    struct payment_result
    {
      payment_status status = payment_status::Unavailable;
      std::unique_ptr<local_payment_assessment> assessment; // Iff LocalAdvisory.
    };

    // ---- Alias operation v1 (additive; TESTNET/103 ordinary accounts) ---------
    //
    // A typed alias registration or basic update of this wallet's OWN alias.
    // It is never an ordinary payment: the extra is the core's extra_alias_entry
    // (post-HF2 form), registration adds exactly one explicit-native,
    // zero-blinding-mask burn destination to the core alias reward account, and
    // update has no destination at all (fee only, change to this wallet).
    // Legacy wallet2::request_alias_registration/request_alias_update broadcast
    // by default and are never called. Preparation calls ONLY
    // prepare_transaction() through the existing one-call bounded decoy
    // transport and returns unsigned immutable review terms, never signed
    // bytes. Signing happens only in sign_reviewed_alias_to_held_v1, after the
    // caller's fresh protected admission, with finalize_transaction(..., false,
    // false) and the existing Prepared/replace/complete lease protocol.
    //
    // Names: canonical core alphabet (validate_alias_name), 6..255 bytes. Names
    // shorter than ALIAS_MINIMUM_PUBLIC_SHORT_NAME_ALLOWED need the special
    // authority key and are refused here. The public alias comment is strict
    // UTF-8, at most 400 bytes; it is published on chain and is unrelated to
    // the app's private on-device payment note. The bound address is always
    // this wallet's own canonical address with no tracking (view) key; an
    // ownership transfer or tracking-key edit is not part of v1.
    //
    // Node facts are caller-supplied OBSERVATIONS of its selected node, never
    // reservations: registration requires the explicit Unregistered fact and
    // the node's positive registration cost (protocol burn); update requires
    // the node's reported owner address to equal this wallet's canonical
    // address and a zero burn. Burn and miner fee stay separate amounts. A
    // different later cost, owner, tip, hardfork or account needs a new review.
    enum class alias_operation_kind_v1 { Register, Update };
    enum class alias_node_observation_v1 { Unregistered, OwnedByThisWallet };
    constexpr std::size_t min_alias_operation_name_bytes = 6;
    constexpr std::size_t max_alias_operation_name_bytes = 255;
    constexpr std::size_t max_alias_public_comment_bytes = 400;
    struct alias_operation_request_v1
    {
      uint32_t version = 1;
      alias_operation_kind_v1 kind = alias_operation_kind_v1::Register;
      std::string alias;
      std::string public_comment;
      alias_node_observation_v1 observation = alias_node_observation_v1::Unregistered;
      std::string observed_owner_address; // Update only: node-reported canonical address.
      uint64_t quoted_burn = 0;           // Register: node cost > 0. Update: 0.
      uint64_t expected_generation = 0;
      uint64_t expected_revision = 0;
    };
    // Typed refusal detail of an alias InvalidRequest/StaleObservation/
    // PreparationFailed answer; None for every other status.
    enum class alias_detail_v1
    {
      None, InvalidVersion, InvalidName, ShortName, InvalidComment, InvalidObservation,
      OwnerMismatch, InvalidBurn, AmountOverflow, UnsupportedWallet, UnsupportedProfile,
      RetainedPending, HardforkUnsupported, LocalCoverage, Construction, PreparedShape
    };
    bool valid_alias_operation_name(const std::string& alias) noexcept;
    bool valid_alias_public_comment(const std::string& comment) noexcept;

    // ---- Wallet-file password rekey v1 (additive) -------------------------------
    //
    // Re-encrypts THIS open session's wallet file under a new wallet-file
    // password: the account keys, stored genesis, creation time, native History
    // origin, retained history/pending rows and every other serialized fact stay
    // exactly those of the loaded wallet. It is not a payment, a signing
    // authority, a PIN change or a seed-password change (the optional mnemonic
    // seed password is unrelated and never read or written here).
    //
    // Password policy (the producer's own): currency::validate_password's
    // alphabet (ASCII letters, digits and ~!?@#$%^&*_+|{}[]()<>:;"'-=/.,), no
    // space or other byte, length min..max below. Bytes are taken exactly: no
    // trimming, normalization, truncation or replacement. The current password
    // is refused (SamePassword).
    constexpr std::size_t min_wallet_password_bytes = 8;
    constexpr std::size_t max_wallet_password_bytes = 40;
    bool valid_wallet_password(const char* bytes, std::size_t size) noexcept;

    // Fresh native authentication + protected-key use for ONE rekey of the
    // account it names. subject() is the provider's committed identity, never
    // the file's. consume() is single use; current() a nonblocking sample.
    class rekey_admission
    {
    public:
      virtual ~rekey_admission() noexcept = default;
      virtual bool current() const noexcept = 0;
      virtual const public_identity& subject() const noexcept = 0;
      virtual bool consume() noexcept = 0;
      rekey_admission(const rekey_admission&) = delete;
      rekey_admission& operator=(const rekey_admission&) = delete;
    protected:
      rekey_admission() = default;
    };

    // The platform's paired publication of ONE rekey of the exact material
    // lease this session holds (lease() must be that object; anything else is
    // refused before any effect). Calls are serial, on the session executor,
    // inside rekey_password_v1 only; none may reenter the core.
    //  stage    durably records a fixed-purpose intent binding the CURRENT
    //           wallet bytes and credential (old) to `image` and the new
    //           sealed credential (new), then writes `image` to the private
    //           staged path. The live wallet and live credential are untouched.
    //           Committed: staged and durable. Rejected: definitely nothing
    //           authoritative changed (any residue is the provider's own and
    //           explained by its intent). Unknown: the intent may exist.
    //  staged_path  the canonical private staged image path, valid only after a
    //           Committed stage; read-only strict validation reads it.
    //  publish  the commit point (one durable atomic intent transition), then
    //           the exact pair replacement and readback. Committed: the new
    //           wallet + new credential are the authoritative pair. Rejected:
    //           definitely before the commit point (old pair authoritative).
    //           Unknown: the commit point may have been passed.
    //  discard  precommit only: removes the staged image and intent. Committed:
    //           old pair, no residue. Anything else: the intent may remain and
    //           the provider reconciles it from its authenticated facts.
    // The image/password pointers are borrowed for the call only.
    class password_rekey_port
    {
    public:
      virtual ~password_rekey_port() noexcept = default;
      virtual const material_lease* lease() const noexcept = 0;
      virtual commit_status stage(const public_identity& bound, const char* image, std::size_t size,
        const char* password, std::size_t password_size) noexcept = 0;
      virtual const std::wstring* staged_path() const noexcept = 0;
      virtual commit_status publish(const public_identity& bound) noexcept = 0;
      virtual commit_status discard() noexcept = 0;
      password_rekey_port(const password_rekey_port&) = delete;
      password_rekey_port& operator=(const password_rekey_port&) = delete;
    protected:
      password_rekey_port() = default;
    };

    //  Committed        the paired publication committed; the session released
    //                   everything (wallet, scanner, lease) so no later
    //                   checkpoint can ever render under the old password.
    //                   Reopen through ordinary Open with the new credential.
    //  Rejected         definitely before the commit point: the old pair is
    //                   authoritative and the session stays open and usable.
    //  InvalidPassword  policy refused before any effect.
    //  SamePassword     equals the current wallet-file password; no effect.
    //  NotSettled       unsaved progress could not be checkpointed first
    //                   (definite failure); nothing staged, session retained.
    //  Revoked          admission or open authorization not current; no
    //                   commit attempted (any staged residue was discarded).
    //  IdentityMismatch admission subject / lease / wallet identity drift.
    //  ValidationFailed the staged image did not strictly reopen as exactly this
    //                   wallet under the new password; staged residue discarded.
    //  Unsupported      no port, foreign port or no checkpoint capability.
    //  RecoveryRequired faulted wallet; nothing rendered.
    //  OutcomeUnknown   sticky: the commit point (or a staged intent) may exist.
    //                   No checkpoint is ever written by this session again.
    //  Busy/Unavailable as elsewhere; nothing durable was attempted.
    enum class rekey_status_v1
    {
      Committed, Rejected, InvalidPassword, SamePassword, NotSettled, Revoked, IdentityMismatch,
      ValidationFailed, Unsupported, RecoveryRequired, OutcomeUnknown, Busy, Unavailable
    };

    struct checkpoint_test_access; // Defined only by core unit tests.
    struct history_interval_test_access; // Defined only by core unit tests.
    struct alias_pending_test_access;    // Defined only by core unit tests.
    struct password_rekey_test_access;   // Defined only by core unit tests.

    // Separate production-native entry. No wallet pointer, secret, mutable
    // handle, generic RPC, transaction object or path is lent out; balances
    // and retained activity leave only as owned snapshot DTOs. Only
    // OpenExisting is implemented, for an Available build_profile() only
    // (checked before the lease material is read); the expected stored genesis
    // must equal that descriptor before any file access.
    // Creation/restoration must acquire their own persistence
    // protocol before these enum values can succeed.
    //
    // Executor and lifetime. Every method and the destructor run on the one
    // native executor that owns the session; none is reentrant. A call made
    // from inside a running step or checkpoint (e.g. from a transport or
    // provider callback) returns Busy/Unavailable and never destroys the
    // wallet, scanner or lease. Only the caller-owned stop flag passed to
    // step_sync() may be set from another thread; it is observed between
    // blocks and cannot interrupt a transport call already in flight.
    //
    // Authority. Revocation observed by any read/scan sample is sticky for
    // this session: later Receive, backup, preparation and steps refuse even
    // if the provider reports current again, and a step completing after
    // revocation reports Revoked. Revocation does NOT withdraw drain authority:
    // checkpoint() and close() use the already-held material and writer to
    // save clean dirty progress without a new authorization sample, secret
    // unsealing or publication. Faulted/unknown state is never drained.
    //
    // Persistence. Dirty/fault observations live on the wallet (sticky across
    // cancellation, partial failure, equal-height mutation and scanner
    // recreation). Only a Committed checkpoint clears dirty state. Faulted
    // state is never saved or described as unchanged. Destruction releases
    // everything but acknowledges nothing: a runtime owner holding a dirty,
    // RecoveryRequired or OutcomeUnknown session must retain it (and its
    // lease) until it is saved or explicitly reconciled; the destructor is
    // only a last-resort teardown, never persistence evidence.
    //
    // Observation identity. Each opened session receives a process-unique,
    // never reused, nonzero generation. Its state revision starts at zero and
    // advances (never wraps) whenever a step reports state_may_have_changed,
    // including cancelled/failed/equal-height/reorg steps and steps completing
    // after revocation. Checkpoint success, dirty clearing and scanner
    // recreation never reset it. Exhaustion is sticky: later snapshots return
    // LimitExceeded.
    class file_session final
    {
    public:
      static open_result acquire(acquisition_kind kind,
        std::unique_ptr<material_lease>& material,
        std::unique_ptr<file_session>& out) noexcept;
      ~file_session() noexcept;
      file_session(const file_session&) = delete;
      file_session& operator=(const file_session&) = delete;
      file_session(file_session&&) = delete;
      file_session& operator=(file_session&&) = delete;
      // Available while open (unchanged or dirty) and authorized.
      receive_result get_receive() const noexcept;
      // Renders the canonical native 26-word (V2) phrase of the stored seed with
      // NO seed password, from this independently identity-verified session.
      // The rendered phrase is restored into a scratch account and must
      // reproduce the verified spend/view keys and flags before it is lent.
      // For a wallet restored from a V1 phrase or with a seed password the text
      // differs from the words originally entered while recovering the same
      // keys; the original text/password is not reconstructed or stored.
      // No phrase is retained by the session; its temporary copy is wiped
      // best-effort (core account/mnemonic internals are not erased).
      backup_result present_backup(const backup_admission& admission,
        backup_visitor visit, void* context) const noexcept;
      // Clean sessions only: never discards dirty, faulted or unknown state
      // (those return Unavailable and keep everything). Use close() for them.
      release_result release_unchanged() noexcept;
      session_state state() const noexcept;

      // Native-only preparation over the existing bounded scanner. Requires
      // current identity/profile/authorization and checkpoint capability first.
      // A protected native origin record resumes its connected lineage. A
      // legacy image has no such proof: its exact same account is replayed in
      // isolated transient scratch from an actual conservative prefix boundary.
      // The admitted wallet, History, pending rows and Held claims stay intact
      // until a reconciled complete candidate image is durably Committed.
      // Existing authoritative dirty/fault/unknown obligations remain sticky.
      // No Restore, file/profile creation, new lease, provider or date override.
      sync_prepare_result prepare_sync(std::shared_ptr<i_core_proxy> transport,
        read_only::step_limits limits) noexcept;
      // One bounded serial scanner step (at most one block request).
      sync_step_result step_sync(std::atomic<bool>& stop) noexcept;
      // Renders the CURRENT wallet with the existing encrypted current-format
      // serializer (no account/genesis reset, no store(), filenames, sidecars
      // or rename of its own) and hands it to the lease's replace_checkpoint.
      checkpoint_result checkpoint() noexcept;
      // Explicit typed close. Clean: release. Dirty: checkpoint, then release
      // only after Committed. Anything else keeps wallet, scanner and lease.
      // Release order: scanner, wallet, then the material lease.
      close_result close() noexcept;
      // Detached public snapshot (see snapshot_status). Open (unchanged or
      // dirty) and authorized sessions only; authorization is sampled before
      // building and again immediately before publication. Reads the wallet
      // only; persistence, dirty/fault state, scanner and close behavior are
      // unchanged. A first page freezes one core time and supersedes every
      // earlier cursor of this session; a continuation reuses that frozen
      // time, so amounts and confirmations stay consistent across pages.
      snapshot_result capture_snapshot(const snapshot_request& request) noexcept;
      // Same capture, authority samples and immutable page as capture_snapshot.
      // Coverage is taken inside that capture, never a second read or getter.
      product_snapshot_result_v1 capture_product_snapshot_v1(const snapshot_request& request) noexcept;
      product_snapshot_result_v2 capture_product_snapshot_v2(const snapshot_request& request) noexcept;
      product_snapshot_result_v3 capture_product_snapshot_v3(const snapshot_request& request) noexcept;
      // The unchanged v3 capture plus native_history_interval_v1, read from the
      // same authoritative wallet under one busy scope on this executor before
      // a final authorization sample; generation and revision must still equal
      // the built page. Any failure has no payload. v1..v3 are unchanged.
      product_snapshot_result_v4 capture_product_snapshot_v4(const snapshot_request& request) noexcept;
      // TESTNET/103 only, ordinary full native accounts/recipients. Captures bounded
      // owned request values before any provider or clock callback. Reads local
      // confirmed output eligibility under compiled nominal bare/core confidential decoy policy and one
      // captured clock; does not select outputs or mutate wallet/session state
      // (except sticky observed revocation). The owner must serialize publication
      // against revocation, as for snapshots. Exact generation/revision required.
      payment_result assess_local_payment(const payment_request& request) noexcept;
      enum class sign_status
      {
        SignedHeld, InvalidRequest, StaleObservation, Revoked, Unsupported,
        PreparationFailed, RecoveryRequired, OutcomeUnknown, Busy, Unavailable, Prepared
      };
      struct sign_result
      {
        sign_status status = sign_status::Unavailable;
        crypto::hash transaction_hash{}; // Public identity only, iff SignedHeld.
      };
      // SDK-only unsigned construction. No signing/material/submission grant.
      class prepared_payment final {
      public:
        ~prepared_payment() noexcept;
        prepared_payment(const prepared_payment&) = delete;
        prepared_payment& operator=(const prepared_payment&) = delete;
        const std::string& recipient() const noexcept;
        uint64_t amount() const noexcept;
        uint64_t fee() const noexcept;
        uint64_t local_blocks() const noexcept;
        uint64_t input_count() const noexcept;
      private:
        friend class file_session;
        struct data;
        explicit prepared_payment(std::unique_ptr<data>);
        const std::unique_ptr<data> m_data;
        bool m_consumed = false; // Existing serial executor; one use.
      };
      struct prepare_result {
        sign_status status = sign_status::Unavailable;
        std::shared_ptr<prepared_payment> reviewed; // Iff Prepared.
      };
      prepare_result prepare_payment_review(const payment_request&, const std::string& saved_draft,
        std::shared_ptr<native_payment::decoy_exchange>, std::atomic<bool>& stop,
        native_payment::construction_detail* construction = nullptr) noexcept;
      sign_result sign_reviewed_payment_to_held(prepared_payment&, signing_admission&,
        std::atomic<bool>& stop) noexcept;

      // Real wallet2 construction/signing with broadcast permanently false.
      // Decoy preparation gets a deny-by-default one-call transport. Durable
      // prepared/replace/complete ordering belongs to this already-held lease.
      sign_result sign_payment_to_held(const payment_request&, const std::string& saved_draft,
        signing_admission&, std::shared_ptr<native_payment::decoy_exchange>,
        std::atomic<bool>& stop) noexcept;

      // Alias operation v1 (see alias_operation_request_v1). SDK-only unsigned
      // construction: frozen account identity, generation/revision, local
      // blocks/tip, tx version + hardfork, kind, canonical alias/comment/bound
      // address, protocol burn, miner fee and selected inputs. Single use.
      class prepared_alias final {
      public:
        ~prepared_alias() noexcept;
        prepared_alias(const prepared_alias&) = delete;
        prepared_alias& operator=(const prepared_alias&) = delete;
        alias_operation_kind_v1 kind() const noexcept;
        const std::string& alias() const noexcept;
        const std::string& public_comment() const noexcept;
        const std::string& bound_address() const noexcept;
        uint64_t protocol_burn() const noexcept;   // Register only; 0 for update.
        uint64_t miner_fee() const noexcept;       // Compiled policy fee, separate.
        uint64_t total_native() const noexcept;    // burn + fee (checked).
        uint64_t input_count() const noexcept;
        uint64_t input_amount() const noexcept;    // Sum of frozen selected inputs.
        uint64_t local_blocks() const noexcept;
        const crypto::hash& local_tip() const noexcept;
        uint64_t tx_version() const noexcept;
        uint64_t hardfork() const noexcept;
        uint64_t generation() const noexcept;
        uint64_t revision() const noexcept;
      private:
        friend class file_session;
        struct data;
        explicit prepared_alias(std::unique_ptr<data>);
        const std::unique_ptr<data> m_data;
        bool m_consumed = false; // Existing serial executor; one use.
      };
      struct alias_prepare_result_v1 {
        sign_status status = sign_status::Unavailable;
        alias_detail_v1 detail = alias_detail_v1::None;
        std::shared_ptr<prepared_alias> reviewed; // Iff Prepared.
      };
      // `saved_operation` is the app's exact immutable saved alias draft
      // encoding (<= 4096 bytes); the admission later binds its hash. The SDK
      // neither parses nor trusts it beyond that binding.
      alias_prepare_result_v1 prepare_alias_review_v1(const alias_operation_request_v1&,
        const std::string& saved_operation, std::shared_ptr<native_payment::decoy_exchange>,
        std::atomic<bool>& stop, native_payment::construction_detail* construction = nullptr) noexcept;
      // Revalidates and consumes exactly this prepared object (no
      // reconstruction or input reselection). Update's alias-ownership
      // signature is made here, after the admission is current, and inserted
      // into the frozen extra (the signed hash excludes it, so no reviewed term
      // changes). finalize_transaction(..., false, false); no broadcast. The
      // signed_payment handed to the lease carries amount = protocol burn and
      // fee = miner fee; its saved_draft is the saved alias operation.
      sign_result sign_reviewed_alias_to_held_v1(prepared_alias&, signing_admission&,
        std::atomic<bool>& stop) noexcept;

      // Wallet-file password rekey v1 (see rekey_status_v1; separate unit).
      // Order: policy; open/clean (dirty progress is first checkpointed under the
      // CURRENT password through the ordinary writer); admission subject ==
      // verified identity and consumed once; render the CURRENT wallet under the
      // new password with the existing checkpoint serializer (no m_password,
      // live credential or file change); port.stage; strict reopen of the staged
      // image under the new password in a separate scratch wallet and exact
      // comparison of keys/flags, genesis, creation time, scan minimum, History
      // origin, chain position, transfer and history counts; fresh admission and
      // authorization samples; port.publish. Committed releases the session.
      rekey_status_v1 rekey_password_v1(rekey_admission& admission, password_rekey_port& port,
        const char* password, std::size_t size) noexcept;

    private:
      friend struct checkpoint_test_access;
      friend struct history_interval_test_access;
      friend struct alias_pending_test_access;
      friend struct password_rekey_test_access;
      file_session() noexcept = default;
      void destroy_owned() noexcept;
      void release_owned() noexcept;
      bool authorized() const noexcept;
      void observe_wallet() noexcept;
      void mark_recovery() noexcept;
      void advance_revision() noexcept;
      snapshot_result capture_snapshot_impl(const snapshot_request& request, scan_coverage_v1* coverage) noexcept;
      snapshot_result capture_snapshot_impl(const snapshot_request& request, scan_coverage_v1* coverage,
        std::vector<activity_owned_outputs_v1>* owned_outputs) noexcept;
      snapshot_result capture_snapshot_impl(const snapshot_request& request, scan_coverage_v1* coverage,
        std::vector<activity_owned_outputs_v1>* owned_outputs, retained_account_birthday_v1* retained_birthday) noexcept;
      snapshot_status build_snapshot(const snapshot_request& request, uint64_t captured_at,
        public_snapshot& out) const;
      snapshot_status build_snapshot(const snapshot_request& request, uint64_t captured_at,
        public_snapshot& out, std::vector<activity_owned_outputs_v1>* owned_outputs) const;
      snapshot_status build_snapshot(const snapshot_request& request, uint64_t captured_at,
        public_snapshot& out, std::vector<activity_owned_outputs_v1>* owned_outputs,
        retained_account_birthday_v1* retained_birthday) const;
      // Read-only extraction for capture_product_snapshot_v4 (separate unit).
      native_history_interval_v1 observe_history_interval(const public_snapshot& built,
        const scan_coverage_v1& coverage) const noexcept;
      checkpoint_result write_checkpoint() noexcept;
      // Alias operation v1 private helper (separate unit): the local pending
      // row of one actual signed alias transaction. Reads only; no mutation.
      bool build_alias_pending(const currency::transaction& tx,
        const currency::finalize_tx_param& parameters, const crypto::hash& expected_hash,
        uint64_t burn, uint64_t fee, const std::atomic<bool>& stop,
        wallet_public::wallet_transfer_info& out) const;
      // Alias operation v1 (separate unit): the session's view of the one typed
      // retained-pending reader, wallet2::native_signed_pending_supported, which
      // ordinary payment assessment, alias preparation and wallet2's history
      // replay admission all use. Payment rows keep their exact checks; actual
      // alias registration/update rows of this wallet are recognized by
      // classify_pending_alias_v1 (never as payments). Unknown/malformed rows
      // fail closed. Optionally reports the names of the pending alias operations.
      snapshot_status typed_pending_supported(std::size_t& nested,
        std::vector<std::string>* pending_aliases = nullptr) noexcept;
      std::unique_ptr<wallet2> m_wallet;
      // A legacy image without actual conservative-origin lineage is rebuilt
      // into same-account transient scratch. It is never lent, published or
      // used for payment. The existing admitted wallet remains authoritative.
      // Interrupted scratch is recomputable canonical-read work, not a dirty
      // replacement of the admitted wallet or a persisted recovery obligation.
      std::unique_ptr<wallet2> m_replay_wallet;
      bool m_replay_ready = false; // only after actual Complete + reconciliation
      std::unique_ptr<read_only::block_sync> m_sync; // Borrows replay or authoritative wallet.
      std::unique_ptr<material_lease> m_material;
      std::string m_address;
      public_identity m_identity{};
      session_state m_state = session_state::Faulted;
      mutable bool m_revoked = false;
      bool m_busy = false;
      std::size_t m_image_limit = max_checkpoint_image_bytes;
      uint64_t m_generation = 0;         // Assigned once at open; never reused.
      uint64_t m_revision = 0;
      bool m_revision_exhausted = false; // Sticky.
      uint64_t m_page_capture = 0;       // Latest first page; 0 = none.
      uint64_t m_page_captured_at = 0;
    };

    // acquire failure leaves the caller's exact lease and output unchanged.
    // Opened transfers both only after all fallible work and final authorization
    // checking. Its destructor releases resources but does not report or
    // acknowledge an explicit close or any persistence to an owner.
  }
}
