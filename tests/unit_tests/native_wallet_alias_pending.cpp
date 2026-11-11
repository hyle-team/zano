// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
// Source-only candidate (alias operation v1). Executed by the QA SDK build
// (unit_tests GLOB at configure); never by the app host checks. The disposable
// wallet/provider fixture is the reduced form of native_wallet_history_interval.cpp.
// Every case runs the real file_session::typed_pending_supported over a really
// opened native file session. It is a view of wallet2::native_signed_pending_supported,
// the one typed reader that also gates wallet2::native_history_replay_state_supported.
// Pending rows are synthetic in-memory values put into the real wallet
// container; nothing is signed, persisted or sent. The pure alias rule itself
// (native_wallet_alias_pending.h) is covered by the app host checks. The
// positive case (an actual signed alias registration/update row beside an
// ordinary payment, then replay) needs real funded construction: QA testnet.
#include "gtest/gtest.h"
#include "wallet/native_wallet_session.h"
#include "wallet/wallet2.h"
#include "currency_core/account.h"
#include "currency_core/currency_config.h"
#include "currency_core/currency_format_utils.h"
#include <array>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <boost/filesystem.hpp>

namespace tools { namespace native_session
{
  // Test-only seam. Defined only here; no production caller.
  struct alias_pending_test_access
  {
    static tools::wallet2& wallet(file_session& session) { return *session.m_wallet; }
    static snapshot_status typed(file_session& session, std::vector<std::string>* names = nullptr)
    {
      std::size_t nested = 0;
      return session.typed_pending_supported(nested, names);
    }
  };
}}

namespace native_alias_pending_test
{
namespace fs = boost::filesystem;
using namespace tools::native_session;
using access = alias_pending_test_access;

// wallet2's typed reader and its replay admission, named through explicit
// instantiation (access is not checked there). Test-only; no production entry.
using legacy_type = snapshot_status (tools::wallet2::*)(std::size_t&, const std::function<bool()>&) const;
using replay_type = bool (tools::wallet2::*)(const std::function<bool()>&) const;
template<class Tag, typename Tag::type Member> struct expose
{
  friend typename Tag::type member(Tag) { return Member; }
};
struct legacy_tag { using type = legacy_type; friend type member(legacy_tag); };
struct replay_tag { using type = replay_type; friend type member(replay_tag); };
template struct expose<legacy_tag, &tools::wallet2::native_signed_pending_supported>;
template struct expose<replay_tag, &tools::wallet2::native_history_replay_state_supported>;
snapshot_status legacy(tools::wallet2& w)
{
  std::size_t nested = 0;
  return (w.*member(legacy_tag{}))(nested, [] { return true; });
}
bool replay_admitted(tools::wallet2& w)
{
  return (w.*member(replay_tag{}))([] { return true; });
}

void require(bool condition)
{
  if (!condition) throw std::runtime_error("native disposable fixture failed");
}

struct exclusion { bool held = false; };
struct provider_state
{
  bool authorized = true;
  std::shared_ptr<exclusion> storage;
};

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
protected:
  const open_material& material() const override { return m_value; }
private:
  open_material m_value;
  std::shared_ptr<provider_state> m_state;
};

class fixture final
{
public:
  fixture()
  {
    try
    {
      parent = fs::canonical(fs::temp_directory_path());
      const auto requested = parent / fs::unique_path("zano-native-alias-pending-%%%%-%%%%-%%%%");
      require(fs::create_directory(requested));
      root = fs::canonical(requested);
      require(root == requested && root.parent_path() == parent);
      input.path = (root / "disposable.wallet").wstring();
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

  std::unique_ptr<file_session> open(const std::shared_ptr<provider_state>& state)
  {
    std::unique_ptr<material_lease> lease(new provider_lease(input, state));
    std::unique_ptr<file_session> session;
    require(file_session::acquire(acquisition_kind::OpenExisting, lease, session) == open_result::Opened);
    require(static_cast<bool>(session) && !lease);
    return session;
  }
  std::shared_ptr<provider_state> control()
  {
    auto state = std::make_shared<provider_state>();
    state->storage = storage;
    return state;
  }

  fs::path root, parent;
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
    catch (...)
    {
      try { ADD_FAILURE() << "native fixture cleanup failed"; }
      catch (...) {}
    }
  }
};

crypto::hash row_hash(unsigned char tag)
{
  crypto::hash id{};
  auto* bytes = reinterpret_cast<unsigned char*>(&id);
  bytes[0] = 0xa1; bytes[1] = tag; bytes[31] = 0x5e;
  return id;
}

// Counterparty-free: the shape an alias operation row has. Its transaction is
// absent, so it can never be recognized as an actual alias operation.
tools::wallet_public::wallet_transfer_info alias_shaped(unsigned char tag)
{
  tools::wallet_public::wallet_transfer_info row{};
  row.tx_hash = row_hash(tag);
  row.comment = "public";
  return row;
}

// One counterparty: the ordinary payment shape, without a real transaction.
tools::wallet_public::wallet_transfer_info payment_shaped(unsigned char tag, std::size_t counterparties = 1)
{
  tools::wallet_public::wallet_transfer_info row{};
  row.tx_hash = row_hash(tag);
  for (std::size_t n = 0; n != counterparties; ++n) row.remote_addresses.push_back("synthetic-counterparty");
  return row;
}

void put(tools::wallet2& w, const tools::wallet_public::wallet_transfer_info& row)
{
  require(w.m_unconfirmed_txs.emplace(row.tx_hash, row).second);
}

// Every retained node is present again, by key and by its own retained hash.
void expect_retained(tools::wallet2& w, const std::vector<crypto::hash>& hashes)
{
  EXPECT_EQ(hashes.size(), w.m_unconfirmed_txs.size());
  for (const auto& hash : hashes)
  {
    const auto found = w.m_unconfirmed_txs.find(hash);
    ASSERT_TRUE(found != w.m_unconfirmed_txs.end());
    EXPECT_TRUE(found->second.tx_hash == hash);
  }
}

template<class Body> void fixed_failure_boundary(Body&& body)
{
  try { body(); }
  catch (...) { ADD_FAILURE() << "native alias pending fixture helper failed"; }
}
#define NATIVE_ALIAS_PENDING_TEST(name) \
  static void native_alias_pending_case_##name(); \
  TEST(native_wallet_alias_pending, name) \
  { fixed_failure_boundary(&native_alias_pending_case_##name); } \
  static void native_alias_pending_case_##name()

#if (!defined(TESTNET) && CURRENCY_FORMATION_VERSION == 84) || \
    (defined(TESTNET) && CURRENCY_FORMATION_VERSION == 103)

NATIVE_ALIAS_PENDING_TEST(payment_only_state_is_the_unchanged_sdk_answer)
{
  fixture f;
  auto session = f.open(f.control());
  auto& w = access::wallet(*session);
  std::vector<std::string> names{"stale"};
  EXPECT_EQ(snapshot_status::Available, access::typed(*session, &names));
  EXPECT_TRUE(names.empty());
  put(w, payment_shaped(1));
  const auto sdk = legacy(w);
  EXPECT_NE(snapshot_status::Available, sdk);
  EXPECT_EQ(sdk, access::typed(*session));
  expect_retained(w, {row_hash(1)});
}

NATIVE_ALIAS_PENDING_TEST(unknown_counterparty_free_row_fails_closed_and_is_retained)
{
  fixture f;
  auto session = f.open(f.control());
  auto& w = access::wallet(*session);
  put(w, alias_shaped(2));
  std::vector<std::string> names;
  EXPECT_EQ(snapshot_status::UnsupportedData, access::typed(*session, &names));
  EXPECT_TRUE(names.empty()); // Nothing unrecognized is reported as an alias.
  expect_retained(w, {row_hash(2)});
  // wallet2's own reader gives the same answer (no relabelling), and the
  // history replay admission, which uses that reader, fails closed too.
  EXPECT_EQ(snapshot_status::UnsupportedData, legacy(w));
  EXPECT_FALSE(replay_admitted(w));
  expect_retained(w, {row_hash(2)});
}

NATIVE_ALIAS_PENDING_TEST(mixed_rows_have_one_reader_answer_and_nothing_moves)
{
  fixture f;
  auto session = f.open(f.control());
  auto& w = access::wallet(*session);
  put(w, payment_shaped(3));
  put(w, alias_shaped(4));
  put(w, alias_shaped(5));
  const auto bucket_count = w.m_unconfirmed_txs.bucket_count();
  // The session reports wallet2's typed reader exactly; no node is extracted.
  const auto reader = legacy(w);
  EXPECT_NE(snapshot_status::Available, reader);
  EXPECT_EQ(reader, access::typed(*session));
  expect_retained(w, {row_hash(3), row_hash(4), row_hash(5)});
  EXPECT_EQ(bucket_count, w.m_unconfirmed_txs.bucket_count());
  // A second run is identical: no row was consumed, moved or released.
  EXPECT_EQ(reader, access::typed(*session));
  EXPECT_FALSE(replay_admitted(w));
  expect_retained(w, {row_hash(3), row_hash(4), row_hash(5)});
}

NATIVE_ALIAS_PENDING_TEST(foreign_multi_counterparty_row_is_never_an_alias)
{
  fixture f;
  auto session = f.open(f.control());
  auto& w = access::wallet(*session);
  put(w, payment_shaped(6, 2));
  EXPECT_NE(snapshot_status::Available, access::typed(*session));
  EXPECT_EQ(legacy(w), access::typed(*session));
  put(w, alias_shaped(7));
  EXPECT_NE(snapshot_status::Available, access::typed(*session));
  expect_retained(w, {row_hash(6), row_hash(7)});
}

NATIVE_ALIAS_PENDING_TEST(revoked_authority_is_reported_with_rows_retained)
{
  fixture f;
  auto state = f.control();
  auto session = f.open(state);
  auto& w = access::wallet(*session);
  put(w, alias_shaped(8));
  state->authorized = false;
  EXPECT_EQ(snapshot_status::Revoked, access::typed(*session));
  expect_retained(w, {row_hash(8)});
}

#endif
}
