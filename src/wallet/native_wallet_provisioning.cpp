// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#include "native_wallet_provisioning.h"
#include "native_wallet_durable_descriptor.h"
#include "wallet2.h"
#include "currency_core/account.h"
#include "currency_core/currency_config.h"
#include "currency_core/currency_format_utils.h"
#include <cctype>
#include <cstring>
#include <utility>
#include <boost/filesystem.hpp>
#if ZANO_NATIVE_PROVISIONING_DESCRIPTORS
#include <cerrno>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

// Source overlay of the accepted SDK base src/wallet/native_wallet_provisioning.cpp.
// The only change is the persistence platform gate: Linux/Android and Darwin
// (iOS device and simulator, macOS host tests) share the descriptor path
// below through native_wallet_durable_descriptor.h. Linux keeps fsync and the
// same listing; Darwin syncs with F_FULLFSYNC. Every check, flag, phase,
// status, reopen and identity rule is unchanged; any other platform still
// refuses with UnsupportedCapability before effects.

namespace tools { namespace native_session
{
  namespace
  {
    using S = provision_status;
    using D = provision_detail;
    constexpr std::size_t max_phrase_bytes = 1024;

    void wipe(void* data, std::size_t size) noexcept
    {
      volatile unsigned char* bytes = static_cast<volatile unsigned char*>(data);
      for (std::size_t i = 0; i != size; ++i) bytes[i] = 0;
    }

    void wipe(std::string& value) noexcept
    {
      if (!value.empty()) wipe(&value[0], value.size());
      value.clear();
    }

    // A std::string copy required by a core API, wiped best-effort on scope exit.
    class scoped_copy final
    {
    public:
      explicit scoped_copy(const secret_buffer& source)
      {
        if (!source.empty()) m_value.assign(source.data(), source.size());
      }
      ~scoped_copy() noexcept { wipe(m_value); }
      scoped_copy(const scoped_copy&) = delete;
      scoped_copy& operator=(const scoped_copy&) = delete;
      const std::string& get() const noexcept { return m_value; }
    private:
      std::string m_value;
    };

    // Borrows the provisioning lease for the existing strict reader. It owns only
    // copies of the path/password and the identity captured before persistence.
    // Authorization and checkpoint capability are forwarded to that lease; the
    // protected writer is reached through a forwarder created by the friend
    // wallet_provisioning, never exposed publicly.
    class reopen_material final : public material_lease
    {
    public:
      using writer = commit_status (*)(provisioning_lease&, const public_identity&, const char*, std::size_t);
      reopen_material(provisioning_lease& lease, writer write, const std::wstring& path,
        const secret_buffer& password, const public_identity& expected)
        : m_lease(lease), m_write(write)
      {
        m_value.path = path;
        m_value.password.assign(password.data(), password.size());
        m_value.expected = expected;
      }
      ~reopen_material() noexcept override { wipe(m_value.password); }
      bool authorization_current() const noexcept override { return m_lease.authorization_current(); }
      bool checkpoint_supported() const noexcept override
      {
        return m_write && m_lease.checkpoint_supported();
      }
    protected:
      const open_material& material() const override { return m_value; }
      commit_status replace_checkpoint(const public_identity& bound, const char* image,
        std::size_t size) override
      {
        // Only the identity this reopen verified; anything else is refused
        // before the provider is reached, leaving the target untouched.
        if (!m_write || bound.spend_public_key != m_value.expected.spend_public_key ||
            bound.view_public_key != m_value.expected.view_public_key ||
            bound.flags != m_value.expected.flags ||
            bound.stored_genesis != m_value.expected.stored_genesis)
          return commit_status::Rejected;
        return m_write(m_lease, bound, image, size);
      }
    private:
      provisioning_lease& m_lease;
      const writer m_write;
      open_material m_value;
    };

    struct location
    {
      std::string directory;
      std::string leaf;
    };

    // Structural preflight only. The provider owns confinement/exclusivity; the
    // descriptor checks and O_EXCL creation below never overwrite anything.
    provision_detail validate_location(const std::wstring& value, location& result)
    {
      if (value.empty() || value.find(L'\0') != std::wstring::npos) return D::InvalidMaterial;
      const boost::filesystem::path path(value);
      if (!path.is_absolute() || path.relative_path().empty()) return D::InvalidMaterial;
      for (const auto& component : path.relative_path())
        if (component == boost::filesystem::path(".") || component == boost::filesystem::path(".."))
          return D::InvalidMaterial;
      const auto parent = path.parent_path();
      const auto leaf = path.filename();
      if (parent.empty() || parent == parent.root_path() || leaf.empty() ||
          leaf == boost::filesystem::path(".") || leaf == boost::filesystem::path(".."))
        return D::InvalidMaterial;
      // An existing, fully resolved directory: no symlinked or missing component.
      boost::system::error_code error;
      const auto resolved = boost::filesystem::canonical(parent, error);
      if (error || resolved != parent) return D::InvalidMaterial;
      result.directory = parent.string();
      result.leaf = leaf.string();
      if (result.leaf.empty() || result.leaf.find('/') != std::string::npos) return D::InvalidMaterial;
      return D::None;
    }

    std::size_t word_count(const std::string& phrase) noexcept
    {
      std::size_t count = 0;
      bool in_word = false;
      for (const unsigned char c : phrase)
      {
        const bool space = std::isspace(c) != 0;
        if (!space && !in_word) ++count;
        in_word = !space;
      }
      return count;
    }

    // Native Zano 25/26-word phrases only, with the core's own normalization,
    // word validation, password flag and checksum. No BIP39 or other derivation.
    provision_detail restore_account(const secret_buffer& phrase_input, const secret_buffer& seed_input,
      currency::account_base& account)
    {
      if (phrase_input.empty() || phrase_input.size() > max_phrase_bytes) return D::InvalidPhrase;
      const scoped_copy phrase(phrase_input);
      const scoped_copy seed_password(seed_input);
      if (currency::account_base::is_seed_tracking(phrase.get())) return D::UnsupportedInput;
      switch (word_count(phrase.get()))
      {
        case SEED_PHRASE_V1_WORDS_COUNT:
        case SEED_PHRASE_V2_WORDS_COUNT: break;
        case 12: case 15: case 18: case 21: case 24: return D::UnsupportedInput; // BIP39-shaped
        default: return D::InvalidPhrase;
      }
      bool password_protected = false;
      try
      {
        if (!currency::account_base::is_seed_password_protected(phrase.get(), password_protected))
          return D::InvalidPhrase;
      }
      catch (...) { return D::InvalidPhrase; }
      if (password_protected == seed_password.get().empty()) return D::SeedPasswordMismatch;
      if (password_protected && !currency::validate_password(seed_password.get())) return D::InvalidMaterial;
      try
      {
        // A V2 checksum detects a wrong seed password; a V1 phrase has none and
        // derives different keys, which only the provider's user flow can catch.
        if (!account.restore_from_seed_phrase(phrase.get(), seed_password.get())) return D::InvalidPhrase;
      }
      catch (...) { return D::InvalidPhrase; }
      return D::None;
    }

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

    provision_result after_commit_failure(commit_status value) noexcept
    {
      return value == commit_status::Rejected
        ? provision_result{S::RecoveryRequired, D::ProviderRejected}
        : provision_result{S::OutcomeUnknown, D::ProviderUncertain};
    }

#if ZANO_NATIVE_PROVISIONING_DESCRIPTORS
    class unique_fd final
    {
    public:
      explicit unique_fd(int fd) noexcept : m_fd(fd) {}
      ~unique_fd() noexcept { if (m_fd >= 0) ::close(m_fd); }
      unique_fd(const unique_fd&) = delete;
      unique_fd& operator=(const unique_fd&) = delete;
      int get() const noexcept { return m_fd; }
      int release() noexcept { const int fd = m_fd; m_fd = -1; return fd; }
    private:
      int m_fd;
    };

    // Any entry in the held directory - destination, .outkey2ki sidecar, legacy
    // .newtmp_/.oldtmp_ name or an unrelated file - makes the location occupied.
    provision_detail require_empty(int directory) noexcept
    {
      switch (durable_descriptor::entries(directory))
      {
        case durable_descriptor::listing::empty: return D::None;
        case durable_descriptor::listing::occupied: return D::OccupiedOutput;
        case durable_descriptor::listing::unavailable: break;
      }
      return D::Unavailable;
    }

    bool write_all(const provisioning_io& io, int fd, const std::string& bytes) noexcept
    {
      std::size_t offset = 0;
      while (offset < bytes.size())
      {
        const long written = io.write(io.context, fd, bytes.data() + offset, bytes.size() - offset);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) return false;
        offset += static_cast<std::size_t>(written);
      }
      return true;
    }
#endif

    // The production descriptor operations: direct POSIX calls, unchanged flags.
    // File and directory durability go through durable_descriptor::sync
    // (fsync on Linux, F_FULLFSYNC on Darwin).
    provisioning_io posix_io() noexcept
    {
      provisioning_io io;
#if ZANO_NATIVE_PROVISIONING_DESCRIPTORS
      io.create = [](void*, int directory, const char* leaf, int flags, unsigned mode) noexcept
      { return ::openat(directory, leaf, flags, static_cast<mode_t>(mode)); };
      io.write = [](void*, int fd, const char* data, std::size_t size) noexcept
      { return static_cast<long>(::write(fd, data, size)); };
      io.sync_file = [](void*, int fd) noexcept { return durable_descriptor::sync(fd); };
      io.close_file = [](void*, int fd) noexcept { return ::close(fd); };
      io.sync_directory = [](void*, int fd) noexcept { return durable_descriptor::sync(fd); };
#endif
      return io;
    }
  }

  secret_buffer::~secret_buffer() noexcept { clear(); }

  secret_buffer::secret_buffer(secret_buffer&& other) noexcept
    : m_data(std::move(other.m_data)), m_size(other.m_size)
  {
    other.m_size = 0;
  }

  secret_buffer& secret_buffer::operator=(secret_buffer&& other) noexcept
  {
    if (this != &other)
    {
      clear();
      m_data = std::move(other.m_data);
      m_size = other.m_size;
      other.m_size = 0;
    }
    return *this;
  }

  void secret_buffer::assign(const char* data, std::size_t size)
  {
    std::unique_ptr<char[]> next;
    if (size)
    {
      next.reset(new char[size]);
      std::memcpy(next.get(), data, size);
    }
    clear();
    m_data = std::move(next);
    m_size = size;
  }

  void secret_buffer::clear() noexcept
  {
    if (m_data) wipe(m_data.get(), m_size);
    m_data.reset();
    m_size = 0;
  }

  bool generated_backup::lend(const provisioning_lease& authority, visitor visit, void* context) const noexcept
  {
    if (!visit || m_phrase.empty()) return false;
    // The lease lifetime that received this backup must still be alive (the
    // lock succeeds) and be exactly the supplied lease's identity. An empty or
    // expired reference never matches, and a live lock keeps the identity's
    // storage from being reused by any other lease.
    const std::shared_ptr<const void> held = m_authority.lock();
    if (!held || !authority.m_lifetime || held != authority.m_lifetime) return false;
    if (!authority.authorization_current()) return false;
    epee::log_space::scoped_log_silence quiet;
    visit(context, m_phrase.data(), m_phrase.size());
    return true;
  }

  provision_result wallet_provisioning::provision(acquisition_kind kind, provisioning_lease& lease,
    std::unique_ptr<file_session>& out) noexcept
  {
    return provision_with_io(kind, lease, out, posix_io());
  }

  bool wallet_provisioning::wallet_password_supported(const secret_buffer& password) noexcept
  {
    if (password.empty()) return false;
    try
    {
      const scoped_copy copy(password);
      return currency::validate_password(copy.get());
    }
    catch (...) { return false; }
  }

  provision_detail wallet_provisioning::preflight_restore(const secret_buffer& phrase,
    const secret_buffer& seed_password) noexcept
  {
    epee::log_space::scoped_log_silence quiet;
    try
    {
      currency::account_base account;
      const auto restored = restore_account(phrase, seed_password, account);
      if (restored != D::None) return restored;
      // Same normal-custodial rule as provision(): no watch-only/auditable.
      if (account.is_watch_only() || account.get_public_address().flags != 0)
        return D::UnsupportedCapability;
      return D::None;
    }
    catch (...) { return D::Unavailable; }
  }

  provision_result wallet_provisioning::provision_with_io(acquisition_kind kind, provisioning_lease& lease,
    std::unique_ptr<file_session>& out, const provisioning_io& io) noexcept
  {
    // Held through generation, encryption, persistence, provider steps, reopen
    // and destruction of every local wallet/session.
    epee::log_space::scoped_log_silence quiet;
    try
    {
      return run(kind, lease, out, io);
    }
    catch (...) // never inspect exception text
    {
      if (!lease.m_effects_possible) return {S::NotStarted, D::Unavailable};
      return {S::OutcomeUnknown, D::Unavailable};
    }
  }

  provision_result wallet_provisioning::run(acquisition_kind kind, provisioning_lease& lease,
    std::unique_ptr<file_session>& out, const provisioning_io& io)
  {
    if (out) return {S::NotStarted, D::OccupiedOutput};
    if (kind != acquisition_kind::CreateNativeZano && kind != acquisition_kind::RestoreNativeZano)
      return {S::NotStarted, D::UnsupportedCapability};
    // One attempt per lease. An attempt with possible effects is never retried.
    if (lease.m_effects_possible) return {S::RecoveryRequired, D::OccupiedOutput};
#if !ZANO_NATIVE_PROVISIONING_DESCRIPTORS
    // No reviewed descriptor/durable-sync persistence path exists for this platform.
    (void)io;
    return {S::NotStarted, D::UnsupportedCapability};
#else
    if (!io.create || !io.write || !io.sync_file || !io.close_file || !io.sync_directory)
      return {S::NotStarted, D::Unavailable};
    if (!lease.m_lifetime) return {S::NotStarted, D::Unavailable};
    if (!lease.authorization_current()) return {S::NotStarted, D::AuthorizationUnavailable};
    // Only the immutable compiled profile; checked before provider material.
    const auto& profile = build_profile();
    if (profile.status == profile_status::Unsupported) return {S::NotStarted, D::UnsupportedCapability};
    if (profile.status != profile_status::Available || profile.genesis == currency::null_hash)
      return {S::NotStarted, D::Unavailable};
    if (lease.provisioning_kind() != kind) return {S::NotStarted, D::InvalidMaterial};

    // ---- Phase A: no side effects. ----
    const provisioning_material& input = lease.material();
    location where;
    const auto invalid = validate_location(input.path, where);
    if (invalid != D::None) return {S::NotStarted, invalid};
    if (input.wallet_password.empty()) return {S::NotStarted, D::InvalidMaterial};
    const scoped_copy password(input.wallet_password);
    if (!currency::validate_password(password.get())) return {S::NotStarted, D::InvalidMaterial};

    currency::account_base account;
    if (kind == acquisition_kind::CreateNativeZano)
    {
      if (!input.phrase.empty() || !input.seed_password.empty()) return {S::NotStarted, D::InvalidMaterial};
      account.generate(false);
    }
    else
    {
      const auto restored = restore_account(input.phrase, input.seed_password, account);
      if (restored != D::None) return {S::NotStarted, restored};
    }
    const auto& address = account.get_public_address();
    // Normal custodial accounts only: spend key present, no auditable flag.
    if (account.is_watch_only() || address.flags != 0) return {S::NotStarted, D::UnsupportedCapability};

    // Independent expected identity, captured from the new account BEFORE any
    // byte is persisted, with the compiled genesis rather than a file value.
    const public_identity expected{address.spend_public_key, address.view_public_key,
      address.flags, profile.genesis};
    const std::string canonical = currency::get_account_address_as_str(address);
    {
      currency::account_public_address parsed{};
      currency::payment_id_t payment_id;
      if (canonical.empty() ||
          !currency::get_account_address_and_payment_id_from_str(parsed, payment_id, canonical) ||
          !payment_id.empty() || parsed.spend_public_key != address.spend_public_key ||
          parsed.view_public_key != address.view_public_key || parsed.flags != address.flags ||
          currency::get_account_address_as_str(parsed) != canonical)
        return {S::NotStarted, D::Unavailable};
    }

    std::unique_ptr<generated_backup> backup;
    if (kind == acquisition_kind::CreateNativeZano)
    {
      // Generated and bound to this lease's lifetime before persistence, so a
      // phrase is available to the creating session for every Ready wallet.
      // It expires with the lease; the encrypted wallet keeps the native seed.
      // The core returns std::string; that copy is wiped best-effort at once.
      backup.reset(new generated_backup());
      backup->m_authority = lease.m_lifetime;
      std::string phrase = account.get_seed_phrase(std::string());
      try { backup->m_phrase.assign(phrase.data(), phrase.size()); }
      catch (...) { wipe(phrase); throw; }
      wipe(phrase);
      if (backup->m_phrase.empty()) return {S::NotStarted, D::Unavailable};
    }

    std::string image;
    {
      // Native construction: deny proxy, no default directory, votes or worker.
      std::unique_ptr<wallet2> wallet(new wallet2(wallet2::native_file_construction_tag{}));
      wallet->render_native_provisioning_image(account, profile.genesis, password.get(), image);
    }
    if (image.size() <= sizeof(wallet_file_binary_header)) return {S::NotStarted, D::Unavailable};
    if (!lease.authorization_current()) return {S::NotStarted, D::AuthorizationUnavailable};

    unique_fd directory(::open(where.directory.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC));
    if (directory.get() < 0) return {S::NotStarted, D::InvalidMaterial};
    struct stat info{};
    if (::fstat(directory.get(), &info) != 0 || !S_ISDIR(info.st_mode) ||
        (info.st_mode & (S_IWGRP | S_IWOTH)) != 0)
      return {S::NotStarted, D::InvalidMaterial};
    const auto occupied = require_empty(directory.get());
    if (occupied != D::None) return {S::NotStarted, occupied};

    // ---- Phase B: effects. Nothing below deletes, truncates or overwrites. ----
    lease.m_effects_possible = true;
    // A failed create leaves no file; the attempt is still recovery (F6).
    unique_fd file(io.create(io.context, directory.get(), where.leaf.c_str(),
      O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, S_IRUSR | S_IWUSR));
    if (file.get() < 0)
      return {S::RecoveryRequired, errno == EEXIST ? D::OccupiedOutput : D::PersistenceFailed};
    if (!write_all(io, file.get(), image)) return {S::RecoveryRequired, D::PersistenceFailed};
    image.clear();
    // Kernel-reported completion of file data and the entries of the attempt
    // directory (Linux fsync, Darwin F_FULLFSYNC). The link naming the attempt
    // directory itself is the provider's commit_material obligation. Device
    // cache policy below that is the platform's; no stream flush is treated
    // as this.
    if (io.sync_file(io.context, file.get()) != 0) return {S::OutcomeUnknown, D::DurabilityUncertain};
    // Ownership passes to close exactly once; a failed close is never retried
    // and the descriptor is never closed again here.
    if (io.close_file(io.context, file.release()) != 0) return {S::OutcomeUnknown, D::DurabilityUncertain};
    if (io.sync_directory(io.context, directory.get()) != 0)
      return {S::OutcomeUnknown, D::DurabilityUncertain};

    const auto step = [&](commit_status (provisioning_lease::*commit)(const public_identity&),
                          const public_identity& identity) noexcept
    {
      try { return checked((lease.*commit)(identity)); }
      catch (...) { return commit_status::Unknown; }
    };
    if (!lease.authorization_current()) return {S::RecoveryRequired, D::AuthorizationUnavailable};
    // The provider makes the attempt-directory ancestry durably reachable
    // from its durable root before recording material; anything short of a
    // checked Committed stops here. phrase/seed_password are not read again.
    auto committed = step(&provisioning_lease::commit_material, expected);
    if (committed != commit_status::Committed) return after_commit_failure(committed);
    if (!lease.authorization_current()) return {S::RecoveryRequired, D::AuthorizationUnavailable};
    committed = step(&provisioning_lease::commit_binding, expected);
    if (committed != commit_status::Committed) return after_commit_failure(committed);
    if (!lease.authorization_current()) return {S::RecoveryRequired, D::AuthorizationUnavailable};

    // Actual reopen through the existing strict reader, compared against the
    // identity captured above. A readback alone would not be independent.
    // The forwarder is a captureless lambda in this friend member, so it can
    // reach the lease's protected writer without making it public.
    const reopen_material::writer forward = [](provisioning_lease& target, const public_identity& bound,
      const char* bytes, std::size_t size) -> commit_status
    {
      return target.replace_checkpoint(bound, bytes, size);
    };
    std::unique_ptr<material_lease> reopen(new reopen_material(lease, forward, input.path,
      input.wallet_password, expected));
    std::unique_ptr<file_session> session;
    const auto opened = file_session::acquire(acquisition_kind::OpenExisting, reopen, session);
    if (opened != open_result::Opened || !session || reopen)
    {
      switch (opened)
      {
        case open_result::AuthorizationUnavailable: return {S::RecoveryRequired, D::AuthorizationUnavailable};
        case open_result::IdentityMismatch:
        case open_result::GenesisMismatch:
        case open_result::InvalidAddress: return {S::RecoveryRequired, D::IdentityMismatch};
        default: return {S::RecoveryRequired, D::ReopenFailed};
      }
    }
    if (!lease.authorization_current()) return {S::RecoveryRequired, D::AuthorizationUnavailable};
    const auto read = session->get_receive();
    if (read.status != receive_status::Available ||
        read.identity.spend_public_key != expected.spend_public_key ||
        read.identity.view_public_key != expected.view_public_key ||
        read.identity.flags != expected.flags ||
        read.identity.stored_genesis != expected.stored_genesis ||
        read.identity.stored_genesis != profile.genesis || read.address != canonical)
      return {S::RecoveryRequired, lease.authorization_current()
        ? D::IdentityMismatch : D::AuthorizationUnavailable};

    committed = step(&provisioning_lease::commit_ready, read.identity);
    if (committed != commit_status::Committed) return after_commit_failure(committed);
    // Ready is committed. Later authorization loss revokes access only and
    // does not cancel persistence. The backup is handed to this lease, but it
    // is lendable only while this lease lives and is authorized: after
    // ReadyAccessRevoked or close it expires with the lease. Later display
    // needs a separate authenticated native flow against the stored wallet.
    if (backup) lease.retain_backup(std::move(backup));
    if (!lease.authorization_current()) return {S::ReadyAccessRevoked, D::AuthorizationUnavailable};
    out = std::move(session);
    return {S::Ready, D::None};
#endif
  }
}}
