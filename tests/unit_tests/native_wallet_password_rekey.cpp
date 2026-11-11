// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
// Source-only candidate (wallet-file password rekey v1). Executed by the QA SDK
// build (unit_tests GLOB at configure); never by the app host checks. The
// disposable wallet/provider fixture is the reduced form of
// native_wallet_alias_pending.cpp: a freshly generated disposable wallet in one
// temporary directory, a random disposable password, removed per case. No
// existing wallet, credential, seed, network or device is used.
//
// Every case runs the REAL file_session::rekey_password_v1 over a really opened
// native file session: the real render_native_checkpoint_image under the new
// password, the real strict loader on the staged image, and then an ordinary
// file_session::acquire of the published file with the new password. The test
// port below is a filesystem stand-in for the platform paired publication (it
// is NOT the Android registry publisher, which the app host checks cover).
#include "gtest/gtest.h"
#include "wallet/native_wallet_session.h"
#include "wallet/wallet2.h"
#include "currency_core/account.h"
#include "currency_core/currency_config.h"
#include "currency_core/currency_format_utils.h"
#include <array>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <boost/filesystem.hpp>

namespace tools { namespace native_session
{
  // Test-only seam. Defined only here; no production caller.
  struct password_rekey_test_access
  {
    static const material_lease* lease(file_session& session) { return session.m_material.get(); }
    static tools::wallet2& wallet(file_session& session) { return *session.m_wallet; }
  };
}}

namespace native_password_rekey_test
{
namespace fs = boost::filesystem;
using namespace tools::native_session;
using access = password_rekey_test_access;

void require(bool condition)
{
  if (!condition) throw std::runtime_error("native disposable fixture failed");
}

std::vector<char> bytes_of(const fs::path& path)
{
  std::ifstream in(path.string(), std::ios::binary);
  return std::vector<char>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

struct exclusion { bool held = false; };
struct provider_state
{
  bool authorized = true;
  std::shared_ptr<exclusion> storage;
};

// Checkpoint-capable disposable provider: the ordinary writer contract, one
// temporary + rename in the same directory (only used if a case makes it dirty).
class provider_lease final : public material_lease
{
public:
  provider_lease(open_material value, std::shared_ptr<provider_state> state)
    : m_value(std::move(value)), m_state(std::move(state))
  {
    require(m_state && m_state->storage && !m_state->storage->held);
    m_state->storage->held = true;
  }
  ~provider_lease() noexcept override { m_state->storage->held = false; }
  bool authorization_current() const noexcept override { return m_state->authorized; }
  bool checkpoint_supported() const noexcept override { return true; }
protected:
  const open_material& material() const override { return m_value; }
  commit_status replace_checkpoint(const public_identity&, const char* image, std::size_t size) override
  {
    try
    {
      const fs::path target(m_value.path);
      const fs::path temporary = target.parent_path() / "checkpoint.tmp";
      { std::ofstream out(temporary.string(), std::ios::binary | std::ios::trunc); out.write(image, size); if (!out) return commit_status::Rejected; }
      fs::rename(temporary, target);
      return commit_status::Committed;
    }
    catch (...) { return commit_status::Unknown; }
  }
private:
  open_material m_value;
  std::shared_ptr<provider_state> m_state;
};

class admission final : public rekey_admission
{
public:
  explicit admission(public_identity subject) : m_subject(subject) {}
  bool current() const noexcept override { return live; }
  const public_identity& subject() const noexcept override { return m_subject; }
  bool consume() noexcept override { return ++consumed == 1; }
  bool live = true;
  int consumed = 0;
private:
  public_identity m_subject;
};

// Filesystem stand-in for the paired publication. stage writes the borrowed
// image to its own staged name; publish renames it over the target.
class port final : public password_rekey_port
{
public:
  port(const material_lease* bound, fs::path staged, fs::path target)
    : m_lease(bound), m_staged(std::move(staged)), m_target(std::move(target)), m_path(m_staged.wstring()) {}
  const material_lease* lease() const noexcept override { return m_lease; }
  commit_status stage(const public_identity&, const char* image, std::size_t size,
    const char* password, std::size_t password_size) noexcept override
  {
    ++stages;
    if (stage_result != commit_status::Committed) return stage_result;
    try
    {
      std::string bytes(image, size);
      if (corrupt && bytes.size() > 64) bytes[bytes.size() - 7] ^= 0x5a;
      std::ofstream out(m_staged.string(), std::ios::binary | std::ios::trunc);
      out.write(bytes.data(), bytes.size());
      if (!out) return commit_status::Rejected;
      credential.assign(password, password_size);
      m_ready = true;
      return commit_status::Committed;
    }
    catch (...) { return commit_status::Unknown; }
  }
  const std::wstring* staged_path() const noexcept override { return m_ready ? &m_path : nullptr; }
  commit_status publish(const public_identity&) noexcept override
  {
    ++publishes;
    if (publish_result != commit_status::Committed) return publish_result;
    try { fs::rename(m_staged, m_target); m_ready = false; return commit_status::Committed; }
    catch (...) { return commit_status::Unknown; }
  }
  commit_status discard() noexcept override
  {
    ++discards;
    boost::system::error_code ignored;
    fs::remove(m_staged, ignored);
    m_ready = false;
    return discard_result;
  }
  commit_status stage_result = commit_status::Committed;
  commit_status publish_result = commit_status::Committed;
  commit_status discard_result = commit_status::Committed;
  bool corrupt = false;
  int stages = 0, publishes = 0, discards = 0;
  std::string credential;
private:
  const material_lease* m_lease;
  fs::path m_staged, m_target;
  std::wstring m_path;
  bool m_ready = false;
};

class fixture final
{
public:
  fixture()
  {
    try
    {
      parent = fs::canonical(fs::temp_directory_path());
      const auto requested = parent / fs::unique_path("zano-native-password-rekey-%%%%-%%%%-%%%%");
      require(fs::create_directory(requested));
      root = fs::canonical(requested);
      require(root == requested && root.parent_path() == parent);
      target = root / "disposable.wallet";
      staged = root / "disposable.wallet.rekey";
      input.path = target.wstring();
      input.expected.stored_genesis = build_profile().genesis;
      epee::log_space::scoped_log_silence quiet;
      require(build_profile().status == profile_status::Available);
      input.password = epee::string_tools::pod_to_hex(crypto::rand<std::array<uint8_t, 20>>());
      require(currency::validate_password(input.password));
      tools::wallet2 setup;
      setup.set_votes_config_path((root / "voting-sentinel.json").string());
      setup.generate(input.path, input.password, false);
      const auto& address = setup.get_account().get_public_address();
      input.expected.spend_public_key = address.spend_public_key;
      input.expected.view_public_key = address.view_public_key;
      input.expected.flags = address.flags;
      setup.set_genesis(input.expected.stored_genesis);
      setup.store();
    }
    catch (...)
    {
      cleanup();
      throw std::runtime_error("native disposable fixture preparation failed");
    }
  }
  ~fixture() noexcept { cleanup(); }
  fixture(const fixture&) = delete;
  fixture& operator=(const fixture&) = delete;

  open_result open(const std::string& password, std::unique_ptr<file_session>& session)
  {
    open_material value = input;
    value.password = password;
    auto state = std::make_shared<provider_state>();
    state->storage = storage;
    std::unique_ptr<material_lease> lease(new provider_lease(value, state));
    return file_session::acquire(acquisition_kind::OpenExisting, lease, session);
  }
  std::unique_ptr<file_session> open_current()
  {
    std::unique_ptr<file_session> session;
    require(open(input.password, session) == open_result::Opened && static_cast<bool>(session));
    return session;
  }
  std::unique_ptr<port> bind(file_session& session)
  {
    return std::unique_ptr<port>(new port(access::lease(session), staged, target));
  }

  fs::path root, parent, target, staged;
  open_material input;
  std::shared_ptr<exclusion> storage = std::make_shared<exclusion>();

private:
  void cleanup() noexcept
  {
    if (root.empty()) return;
    try
    {
      if (storage->held) { ADD_FAILURE() << "native fixture still owns storage exclusion"; return; }
      boost::system::error_code error;
      fs::remove_all(root, error);
      if (error) ADD_FAILURE() << "native fixture cleanup failed";
      root.clear();
    }
    catch (...) { ADD_FAILURE() << "native fixture cleanup failed"; }
  }
};

const std::string kNew = "Rekey-Case.Pass#1";  // Public test constant, policy-valid.

TEST(native_password_rekey, committed_pair_reopens_same_identity_origin_and_history)
{
  fixture f;
  auto session = f.open_current();
  // Public observations only: the account's creation time and one genuine product capture.
  const auto created = access::wallet(*session).get_account().get_createtime();
  const auto before = session->capture_product_snapshot_v4(snapshot_request{});
  ASSERT_EQ(before.status, snapshot_status::Available);
  ASSERT_TRUE(static_cast<bool>(before.observation));
  auto p = f.bind(*session);
  admission a(f.input.expected);
  ASSERT_EQ(session->rekey_password_v1(a, *p,kNew.data(), kNew.size()), rekey_status_v1::Committed);
  EXPECT_EQ(session->state(), session_state::Released);   // Old password can never render again.
  EXPECT_FALSE(f.storage->held);
  EXPECT_EQ(p->stages, 1); EXPECT_EQ(p->publishes, 1); EXPECT_EQ(p->discards, 0);
  EXPECT_EQ(a.consumed, 1);
  EXPECT_EQ(p->credential, kNew);
  EXPECT_FALSE(fs::exists(f.staged));
  session.reset();

  std::unique_ptr<file_session> stale;
  EXPECT_EQ(f.open(f.input.password, stale), open_result::WrongPassword);  // No old-password fallback.
  EXPECT_FALSE(stale);
  std::unique_ptr<file_session> reopened;
  ASSERT_EQ(f.open(kNew, reopened), open_result::Opened);
  const auto read = reopened->get_receive();
  ASSERT_EQ(read.status, receive_status::Available);
  EXPECT_EQ(read.identity.spend_public_key, f.input.expected.spend_public_key);
  EXPECT_EQ(read.identity.view_public_key, f.input.expected.view_public_key);
  EXPECT_EQ(read.identity.stored_genesis, f.input.expected.stored_genesis);
  EXPECT_EQ(access::wallet(*reopened).get_account().get_createtime(), created);
  const auto after = reopened->capture_product_snapshot_v4(snapshot_request{});
  ASSERT_EQ(after.status, snapshot_status::Available);
  ASSERT_TRUE(static_cast<bool>(after.observation));
  // Same local chain position, retained history, birthday, scan origin and History interval.
  EXPECT_EQ(after.observation->local_blocks, before.observation->local_blocks);
  EXPECT_EQ(after.observation->local_tip_hash_known, before.observation->local_tip_hash_known);
  EXPECT_EQ(after.observation->coverage.retained_rows, before.observation->coverage.retained_rows);
  EXPECT_EQ(after.observation->assets.size(), before.observation->assets.size());
  EXPECT_EQ(after.retained_birthday.available, before.retained_birthday.available);
  EXPECT_EQ(after.retained_birthday.timestamp, before.retained_birthday.timestamp);
  EXPECT_EQ(after.scan_coverage.source, before.scan_coverage.source);
  EXPECT_EQ(after.scan_coverage.minimum_height, before.scan_coverage.minimum_height);
  EXPECT_EQ(after.history_interval.state, before.history_interval.state);
  EXPECT_EQ(after.history_interval.reason, before.history_interval.reason);
  EXPECT_EQ(after.history_interval.first_height, before.history_interval.first_height);
  EXPECT_EQ(reopened->release_unchanged(), release_result::ReleasedUnchanged);
}

TEST(native_password_rekey, policy_same_password_and_authority_refused_without_effect)
{
  fixture f;
  const auto before = bytes_of(f.target);
  auto session = f.open_current();
  auto p = f.bind(*session);
  admission a(f.input.expected);
  for (const std::string bad : {std::string("short1!"), std::string("has space 123"), std::string(41, 'a'),
                                std::string("tab\there123"), std::string("utf8-\xc3\xa9-pass")})
    EXPECT_EQ(session->rekey_password_v1(a, *p,bad.data(), bad.size()), rekey_status_v1::InvalidPassword);
  EXPECT_EQ(session->rekey_password_v1(a, *p,f.input.password.data(), f.input.password.size()),
            rekey_status_v1::SamePassword);
  public_identity other = f.input.expected;
  other.view_public_key = other.spend_public_key;  // Another account's subject.
  admission foreign(other);
  EXPECT_EQ(session->rekey_password_v1(foreign, *p,kNew.data(), kNew.size()), rekey_status_v1::IdentityMismatch);
  admission late(f.input.expected);
  late.live = false;
  EXPECT_EQ(session->rekey_password_v1(late, *p,kNew.data(), kNew.size()), rekey_status_v1::Revoked);
  port wrong(nullptr, f.staged, f.target);  // Not this session's held lease.
  EXPECT_EQ(session->rekey_password_v1(a, wrong, kNew.data(), kNew.size()), rekey_status_v1::Unsupported);
  EXPECT_EQ(p->stages + wrong.stages, 0);
  EXPECT_EQ(a.consumed + foreign.consumed + late.consumed, 0);
  EXPECT_EQ(session->state(), session_state::OpenUnchanged);
  EXPECT_EQ(bytes_of(f.target), before);
  EXPECT_EQ(session->release_unchanged(), release_result::ReleasedUnchanged);
}

TEST(native_password_rekey, definite_precommit_failures_keep_old_pair_and_session)
{
  fixture f;
  const auto before = bytes_of(f.target);
  auto session = f.open_current();
  {
    auto p = f.bind(*session);
    p->stage_result = commit_status::Rejected;
    admission a(f.input.expected);
    EXPECT_EQ(session->rekey_password_v1(a, *p,kNew.data(), kNew.size()), rekey_status_v1::Rejected);
    EXPECT_EQ(p->publishes, 0);
  }
  {
    auto p = f.bind(*session);
    p->corrupt = true;  // Staged bytes do not strictly reopen: never published.
    admission a(f.input.expected);
    EXPECT_EQ(session->rekey_password_v1(a, *p,kNew.data(), kNew.size()), rekey_status_v1::ValidationFailed);
    EXPECT_EQ(p->publishes, 0); EXPECT_EQ(p->discards, 1);
  }
  {
    auto p = f.bind(*session);
    p->publish_result = commit_status::Rejected;
    admission a(f.input.expected);
    EXPECT_EQ(session->rekey_password_v1(a, *p,kNew.data(), kNew.size()), rekey_status_v1::Rejected);
    EXPECT_EQ(p->publishes, 1); EXPECT_EQ(p->discards, 1);
  }
  EXPECT_FALSE(fs::exists(f.staged));
  EXPECT_EQ(session->state(), session_state::OpenUnchanged);
  EXPECT_EQ(session->checkpoint(), checkpoint_result::Unchanged);
  EXPECT_EQ(bytes_of(f.target), before);
  EXPECT_EQ(session->release_unchanged(), release_result::ReleasedUnchanged);
  std::unique_ptr<file_session> reopened;
  EXPECT_EQ(f.open(f.input.password, reopened), open_result::Opened);  // Old pair still authoritative.
  if (reopened) EXPECT_EQ(reopened->release_unchanged(), release_result::ReleasedUnchanged);
}

TEST(native_password_rekey, uncertain_publication_is_sticky_and_never_checkpoints)
{
  fixture f;
  auto session = f.open_current();
  {
    auto p = f.bind(*session);
    p->publish_result = commit_status::Unknown;
    admission a(f.input.expected);
    EXPECT_EQ(session->rekey_password_v1(a, *p,kNew.data(), kNew.size()), rekey_status_v1::OutcomeUnknown);
    EXPECT_EQ(p->discards, 0);  // No automatic rollback once the commit point may have passed.
  }
  EXPECT_EQ(session->state(), session_state::OutcomeUnknown);
  EXPECT_EQ(session->checkpoint(), checkpoint_result::OutcomeUnknown);
  EXPECT_EQ(session->close(), close_result::OutcomeUnknown);
  auto again = f.bind(*session);
  admission retry(f.input.expected);
  EXPECT_EQ(session->rekey_password_v1(retry, *again, kNew.data(), kNew.size()), rekey_status_v1::OutcomeUnknown);
  EXPECT_EQ(again->stages, 0);  // Never silently retried.
  session.reset();  // Last-resort teardown releases the lease; it never stores.
  EXPECT_FALSE(f.storage->held);
}
}
