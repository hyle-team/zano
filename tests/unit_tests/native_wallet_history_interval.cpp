// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
// Source-only candidate. The disposable wallet/provider fixture is the reduced
// form of tests/unit_tests/native_wallet_session.cpp (no checkpoint writer);
// execution generates disposable wallets and needs the same runtime gate.
// Every case runs the real capture_product_snapshot_v4 (and the v2/v3 producers
// it is built on) over a really opened native file session. Origin records and
// retained history rows are synthetic in-memory values written into the real
// wallet members; nothing here is persisted or read back.
#include "gtest/gtest.h"
#include "wallet/native_wallet_session.h"
#include "wallet/i_core_rpc_proxy.h"
#include "wallet/read_only_block_transport.h"
#include "wallet/read_only_birthday_prefix.h"
#include "wallet/read_only_validated_prefix.h"
#include "wallet/wallet2.h"
#include "currency_core/account.h"
#include "currency_core/currency_config.h"
#include "currency_core/currency_format_utils.h"
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <list>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <boost/filesystem.hpp>

namespace tools { namespace native_session
{
  // Test-only observation seam. Defined only here; no production caller.
  struct history_interval_test_access
  {
    static tools::wallet2& wallet(file_session& session) { return *session.m_wallet; }
    static uint64_t generation(const file_session& session) { return session.m_generation; }
    static void adopt_scratch(file_session& session) { session.m_replay_wallet.reset(new tools::wallet2()); }
    static void drop_scratch(file_session& session) { session.m_replay_wallet.reset(); }
    static void advance_revision(file_session& session) { session.advance_revision(); }
    // Read-only observation of the production replay lifecycle (no setter).
    static tools::wallet2* replay(file_session& session) { return session.m_replay_wallet.get(); }
    static bool replay_ready(const file_session& session) { return session.m_replay_ready; }
    static uint64_t revision(const file_session& session) { return session.m_revision; }
    static native_history_interval_v1 observe(const file_session& session, const public_snapshot& built,
      const scan_coverage_v1& coverage) { return session.observe_history_interval(built, coverage); }
  };
}}

namespace native_history_interval_test
{
namespace fs = boost::filesystem;
using namespace tools::native_session;
using access = history_interval_test_access;
using origin_record = tools::read_only::native_history_origin;
using tools::read_only::origin_phase;

// Private wallet2 members, named through explicit instantiation (access is not
// checked there). Test-only; production gains no setter.
template<class Tag, typename Tag::type Member> struct expose
{
  friend typename Tag::type member(Tag) { return Member; }
};
struct origin_tag { using type = origin_record tools::wallet2::*; friend type member(origin_tag); };
struct dirty_tag { using type = bool tools::wallet2::*; friend type member(dirty_tag); };
template struct expose<origin_tag, &tools::wallet2::m_native_history_origin>;
template struct expose<dirty_tag, &tools::wallet2::m_read_only_sync_dirty>;
origin_record& origin_of(tools::wallet2& w) { return w.*member(origin_tag{}); }
bool& dirty_of(tools::wallet2& w) { return w.*member(dirty_tag{}); }

void require(bool condition)
{
  if (!condition) throw std::runtime_error("native disposable fixture failed");
}

struct exclusion { bool held = false; };
// Checkpoint writer outcomes of the disposable provider (default: no writer).
enum class writer_mode { Commit, Reject, Unknown };
struct provider_state
{
  bool authorized = true;
  size_t authorization_reads = 0;
  size_t revoke_at_authorization_read = 0;
  std::shared_ptr<exclusion> storage;
  bool checkpoint_capable = false;
  writer_mode writer = writer_mode::Commit;
  size_t checkpoints = 0;
};

std::string read_bytes(const fs::path& path)
{
  std::ifstream input(path.string(), std::ios::binary);
  require(input.is_open());
  std::string bytes((std::istreambuf_iterator<char>(input)), {});
  require(!input.bad());
  return bytes;
}

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
  bool authorization_current() const noexcept override
  {
    ++m_state->authorization_reads;
    if (m_state->revoke_at_authorization_read &&
        m_state->authorization_reads >= m_state->revoke_at_authorization_read)
      m_state->authorized = false;
    return m_state->authorized;
  }
  bool checkpoint_supported() const noexcept override { return m_state->checkpoint_capable; }
protected:
  const open_material& material() const override { return m_value; }
  // Disposable-file writer only (truncate + write the one target). Rejected is
  // refused before any byte; Unknown models an unconfirmed outcome, unwritten.
  commit_status replace_checkpoint(const public_identity& bound, const char* image, std::size_t size) override
  {
    ++m_state->checkpoints;
    if (!m_state->checkpoint_capable || !m_state->storage->held ||
        bound.spend_public_key != m_value.expected.spend_public_key ||
        bound.view_public_key != m_value.expected.view_public_key ||
        bound.flags != m_value.expected.flags || bound.stored_genesis != m_value.expected.stored_genesis)
      return commit_status::Rejected;
    if (m_state->writer == writer_mode::Reject) return commit_status::Rejected;
    if (m_state->writer == writer_mode::Unknown) return commit_status::Unknown;
    std::ofstream output(fs::path(m_value.path).string(), std::ios::binary | std::ios::trunc);
    if (!output.is_open()) return commit_status::Unknown;
    output.write(image, static_cast<std::streamsize>(size));
    output.flush();
    output.close();
    return output.fail() ? commit_status::Unknown : commit_status::Committed;
  }
private:
  open_material m_value;
  std::shared_ptr<provider_state> m_state;
};

class fixture final
{
public:
  // legacy_minimum: the explicit genuine saved minimum 0 of an admitted legacy
  // image without origin lineage (same convention as native_wallet_session.cpp).
  explicit fixture(bool legacy_minimum = false)
  {
    try
    {
      parent = fs::canonical(fs::temp_directory_path());
      const auto requested = parent / fs::unique_path("zano-native-interval-%%%%-%%%%-%%%%");
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
      if (legacy_minimum) setup.set_minimum_height(0);
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
  std::shared_ptr<provider_state> capable(writer_mode writer = writer_mode::Commit)
  {
    auto state = control();
    state->checkpoint_capable = true;
    state->writer = writer;
    return state;
  }
  fs::path file() const { return fs::path(input.path); }

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

crypto::hash block_id(uint64_t height)
{
  crypto::hash id{};
  auto* bytes = reinterpret_cast<unsigned char*>(&id);
  bytes[0] = 0x5a;
  for (unsigned i = 0; i != 8; ++i) bytes[1 + i] = static_cast<unsigned char>(height >> (8 * i));
  bytes[31] = 0xc3;
  return id;
}

tools::read_only::policy::Hash wide(const crypto::hash& value)
{
  tools::read_only::policy::Hash result{};
  std::memcpy(result.data(), &value, result.size());
  return result;
}

void extend_local_chain(tools::wallet2& w, uint64_t blocks)
{
  for (uint64_t h = w.get_blockchain_current_size(); h < blocks; ++h)
    w.m_chain.push_new_block_id(block_id(h), h);
}

uint64_t clock_value = 0;
uint64_t test_clock() { return clock_value; }
void use_test_clock(tools::wallet2& w, uint64_t now)
{
  clock_value = now;
  w.get_core_runtime_config().get_core_time = &test_clock;
}

// Native seed-format rounded date (offset + whole weeks). Synthetic account time.
constexpr uint64_t week_offset = 1543622400, week_seconds = 604800;
constexpr uint64_t synthetic_createtime = week_offset + 400 * week_seconds + 1000;
constexpr uint64_t synthetic_rounded = week_offset + 400 * week_seconds;

// A producer-shaped record for the wallet's current local chain.
origin_record record_for(tools::wallet2& w, uint64_t first_height, uint64_t view_height, origin_phase phase)
{
  const auto genesis = build_profile().genesis;
  const uint64_t tip = w.get_blockchain_current_size() - 1;
  origin_record origin;
  origin.phase = phase;
  origin.rounded_date = synthetic_rounded;
  origin.first_height = first_height;
  origin.anchor_hash = wide(first_height == 1 ? genesis : block_id(first_height - 1));
  origin.view.target_height = view_height;
  origin.view.target_hash = wide(block_id(view_height));
  origin.view.token = origin.view.target_hash;
  origin.view.genesis_hash = wide(genesis);
  origin.connected_height = tip;
  origin.connected_hash = wide(tip == 0 ? genesis : block_id(tip));
  return origin;
}

void install(tools::wallet2& w, const origin_record& origin)
{
  w.get_account().set_createtime(synthetic_createtime);
  w.m_minimum_height = origin.first_height - 1;
  origin_of(w) = origin;
}

product_snapshot_result_v4 capture(file_session& session)
{
  snapshot_request request;
  return session.capture_product_snapshot_v4(request);
}
product_snapshot_result_v3 capture_v3(file_session& session)
{
  snapshot_request request;
  return session.capture_product_snapshot_v3(request);
}

void expect_unavailable(const native_history_interval_v1& interval, history_interval_reason_v1 reason)
{
  EXPECT_EQ(1u, interval.version);
  EXPECT_EQ(history_interval_state_v1::Unavailable, interval.state);
  EXPECT_EQ(reason, interval.reason);
  // Nothing is fabricated: no zero-filled date/height/hash stands for a fact.
  EXPECT_EQ(0u, interval.rounded_date);
  EXPECT_EQ(0u, interval.first_height);
  EXPECT_EQ(0u, interval.view_height);
  EXPECT_EQ(0u, interval.connected_height);
  EXPECT_TRUE(interval.anchor_hash == currency::null_hash);
  EXPECT_TRUE(interval.view_hash == currency::null_hash);
  EXPECT_TRUE(interval.connected_hash == currency::null_hash);
  EXPECT_FALSE(interval.checkpointed);
}

// Helpers never return into a caller that would dereference a missing page:
// a missing observation is recorded and the case stops at its boundary.
[[noreturn]] void fail_closed(const char* what)
{
  ADD_FAILURE() << what;
  throw std::runtime_error(what);
}

void available(const product_snapshot_result_v4& result)
{
  if (result.version != 4 || result.status != snapshot_status::Available || result.observation == nullptr)
    fail_closed("v4 capture has no available observation");
}

// The real capture, owned by the caller's local. Never bind a reference to a
// temporary result: it is move-only and its observation dies with it.
product_snapshot_result_v4 capture_available(file_session& session)
{
  auto result = capture(session);
  available(result);
  return result;
}

// The v4 page is the v3 page: same capture fields and extensions.
void expect_v3_parity(const product_snapshot_result_v3& v3, const product_snapshot_result_v4& v4)
{
  if (v3.status != snapshot_status::Available || !v3.observation || !v4.observation)
    fail_closed("v3/v4 parity page has no available observation");
  EXPECT_EQ(3u, v3.version);
  EXPECT_EQ(v3.observation->session_generation, v4.observation->session_generation);
  EXPECT_EQ(v3.observation->revision, v4.observation->revision);
  EXPECT_EQ(v3.observation->local_blocks, v4.observation->local_blocks);
  EXPECT_EQ(v3.observation->local_tip_hash_known, v4.observation->local_tip_hash_known);
  EXPECT_TRUE(v3.observation->local_tip_hash == v4.observation->local_tip_hash);
  EXPECT_EQ(v3.observation->assets.size(), v4.observation->assets.size());
  EXPECT_EQ(v3.observation->activity.size(), v4.observation->activity.size());
  EXPECT_TRUE(v4.observation->coverage.retained_local_only);
  EXPECT_FALSE(v4.observation->coverage.history_completeness_known);
  EXPECT_FALSE(v4.observation->coverage.pool_observed);
  EXPECT_EQ(v3.scan_coverage.version, v4.scan_coverage.version);
  EXPECT_EQ(v3.scan_coverage.source, v4.scan_coverage.source);
  EXPECT_EQ(v3.scan_coverage.minimum_height, v4.scan_coverage.minimum_height);
  EXPECT_EQ(v3.scan_coverage.checkpointed, v4.scan_coverage.checkpointed);
  EXPECT_EQ(v3.owned_outputs.size(), v4.owned_outputs.size());
  EXPECT_EQ(v3.retained_birthday.available, v4.retained_birthday.available);
  EXPECT_EQ(v3.retained_birthday.timestamp, v4.retained_birthday.timestamp);
}

template<class Body> void fixed_failure_boundary(Body&& body)
{
  try { body(); }
  catch (...) { ADD_FAILURE() << "native interval fixture helper failed"; }
}
#define NATIVE_INTERVAL_TEST(name) \
  static void native_interval_case_##name(); \
  TEST(native_wallet_history_interval, name) \
  { fixed_failure_boundary(&native_interval_case_##name); } \
  static void native_interval_case_##name()

static_assert(noexcept(std::declval<file_session&>().capture_product_snapshot_v4(
  std::declval<const snapshot_request&>())), "v4 has a typed result");
static_assert(!std::is_copy_constructible<product_snapshot_result_v4>::value, "published by move");
static_assert(std::is_same<decltype(product_snapshot_result_v3{}.retained_birthday),
  retained_account_birthday_v1>::value, "v3 layout is unchanged");
static_assert(noexcept(std::declval<file_session&>().capture_product_snapshot_v2(
  std::declval<const snapshot_request&>())), "v2 has a typed result");
static_assert(noexcept(std::declval<file_session&>().capture_product_snapshot_v3(
  std::declval<const snapshot_request&>())), "v3 has a typed result");
static_assert(!std::is_copy_constructible<product_snapshot_result_v2>::value, "published by move");
static_assert(!std::is_copy_constructible<product_snapshot_result_v3>::value, "published by move");
static_assert(std::is_same<decltype(product_snapshot_result_v2{}.owned_outputs),
  std::vector<activity_owned_outputs_v1>>::value, "v2 layout is unchanged");

product_snapshot_result_v2 capture_v2(file_session& session, const snapshot_request& request = snapshot_request{})
{
  return session.capture_product_snapshot_v2(request);
}

// A retained confirmed history row with recorded received outputs. Its pre-HF4
// transaction shape is one whose origin the producer deliberately does not
// resolve, so every owned output is reported Unknown with its index only.
crypto::hash row_id(unsigned char tag)
{
  crypto::hash id{};
  auto* bytes = reinterpret_cast<unsigned char*>(&id);
  bytes[0] = 0xa7;
  bytes[1] = tag;
  bytes[31] = 0x3e;
  return id;
}
tools::wallet_public::wallet_transfer_info retained_row(unsigned char tag, uint64_t height, uint64_t timestamp,
  std::size_t outputs, std::initializer_list<uint64_t> received)
{
  tools::wallet_public::wallet_transfer_info wti;
  wti.tx_hash = row_id(tag);
  wti.height = height;
  wti.timestamp = timestamp;
  wti.tx.version = TRANSACTION_VERSION_POST_HF4 - 1;
  for (std::size_t i = 0; i != outputs; ++i)
  {
    currency::tx_out_bare out;
    out.amount = 1000 + i;
    out.target = currency::txout_to_key();
    wti.tx.vout.push_back(out);
  }
  uint64_t amount = 7;
  for (const uint64_t index : received)
  {
    tools::wallet_public::employed_tx_entry entry;
    entry.index = index;
    entry.amount = amount++;
    wti.employed_entries.receive.push_back(entry);
  }
  return wti;
}

void expect_unknown_index_only(const owned_output_v1& output, uint64_t index)
{
  EXPECT_EQ(index, output.output_index);
  EXPECT_EQ(owned_output_state_v1::Unknown, output.state);
  EXPECT_TRUE(output.asset_id == currency::null_pkey);
  EXPECT_EQ(0u, output.amount);
  EXPECT_EQ(0u, output.spent_height);
  EXPECT_FALSE(output.reservation_flags);
  EXPECT_FALSE(output.native_held);
}

#if (!defined(TESTNET) && CURRENCY_FORMATION_VERSION == 84) || \
    (defined(TESTNET) && CURRENCY_FORMATION_VERSION == 103)

NATIVE_INTERVAL_TEST(absent_origin_is_unavailable_and_the_v3_page_is_unchanged)
{
  fixture f;
  auto session = f.open(f.control());
  auto& w = access::wallet(*session);
  use_test_clock(w, 1);
  const auto v3 = capture_v3(*session);
  const auto v4 = capture(*session);
  available(v4);
  expect_v3_parity(v3, v4);
  expect_unavailable(v4.history_interval, history_interval_reason_v1::Absent);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_INTERVAL_TEST(scanning_and_complete_are_copied_without_promotion_at_the_captured_tip)
{
  fixture f;
  auto session = f.open(f.control());
  auto& w = access::wallet(*session);
  use_test_clock(w, 1);
  extend_local_chain(w, 12); // Tip 11.
  // Partial interval: connected at the tip, view target still ahead.
  install(w, record_for(w, 5, 20, origin_phase::Scanning));
  {
    const auto v4 = capture_available(*session);
    const auto v3 = capture_v3(*session);
    expect_v3_parity(v3, v4);
    const auto& i = v4.history_interval;
    EXPECT_EQ(history_interval_state_v1::Scanning, i.state);
    EXPECT_EQ(history_interval_reason_v1::Observed, i.reason);
    EXPECT_EQ(synthetic_rounded, i.rounded_date);
    EXPECT_EQ(5u, i.first_height);
    EXPECT_TRUE(i.anchor_hash == block_id(4));
    EXPECT_EQ(20u, i.view_height);
    EXPECT_TRUE(i.view_hash == block_id(20));
    EXPECT_EQ(11u, i.connected_height);
    EXPECT_TRUE(i.connected_hash == block_id(11));
    EXPECT_EQ(v4.observation->local_tip_height, i.connected_height);
    EXPECT_EQ(scan_start_source_v1::SavedBoundary, v4.scan_coverage.source);
    EXPECT_EQ(4u, v4.scan_coverage.minimum_height);
    EXPECT_TRUE(i.checkpointed);
  }
  // A cancelled/truncated step that reached the view leaves Scanning: never promoted.
  install(w, record_for(w, 5, 11, origin_phase::Scanning));
  {
    const auto reached = capture_available(*session);
    EXPECT_EQ(history_interval_state_v1::Scanning, reached.history_interval.state);
  }
  // Only the producer's own Complete reaches Complete, and only to its view.
  install(w, record_for(w, 5, 11, origin_phase::Complete));
  {
    const auto v4 = capture_available(*session);
    const auto& i = v4.history_interval;
    EXPECT_EQ(history_interval_state_v1::Complete, i.state);
    EXPECT_EQ(11u, i.view_height);
    EXPECT_EQ(11u, i.connected_height);
  }
  // A Complete record whose connected tip is below its view is refused.
  install(w, record_for(w, 5, 20, origin_phase::Complete));
  {
    const auto below = capture_available(*session);
    expect_unavailable(below.history_interval, history_interval_reason_v1::Unverified);
  }
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_INTERVAL_TEST(zero_boundary_anchor_is_genesis_and_a_zero_first_height_is_never_described)
{
  fixture f;
  auto session = f.open(f.control());
  auto& w = access::wallet(*session);
  use_test_clock(w, 1);
  extend_local_chain(w, 4);
  install(w, record_for(w, 1, 3, origin_phase::Complete));
  {
    const auto v4 = capture_available(*session);
    EXPECT_EQ(history_interval_state_v1::Complete, v4.history_interval.state);
    EXPECT_EQ(1u, v4.history_interval.first_height);
    EXPECT_TRUE(v4.history_interval.anchor_hash == build_profile().genesis);
    EXPECT_EQ(0u, v4.scan_coverage.minimum_height);
  }
  // A genesis-anchored record whose anchor is not genesis.
  auto wrong_anchor = record_for(w, 1, 3, origin_phase::Complete);
  wrong_anchor.anchor_hash = wide(block_id(0));
  install(w, wrong_anchor);
  {
    const auto refused = capture_available(*session);
    expect_unavailable(refused.history_interval, history_interval_reason_v1::Unverified);
  }
  // first_height zero has no anchor height; the minimum it implies is refused.
  auto zero = record_for(w, 1, 3, origin_phase::Complete);
  zero.first_height = 0;
  origin_of(w) = zero;
  w.m_minimum_height = 0;
  {
    const auto refused = capture_available(*session);
    expect_unavailable(refused.history_interval, history_interval_reason_v1::Unverified);
  }
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_INTERVAL_TEST(identity_date_boundary_and_tip_mismatches_are_unverified_while_the_page_stays_v3)
{
  fixture f;
  auto session = f.open(f.control());
  auto& w = access::wallet(*session);
  use_test_clock(w, 1);
  extend_local_chain(w, 12);
  const auto good = record_for(w, 5, 11, origin_phase::Complete);
  const auto refused = [&](const char*) {
    const auto v4 = capture_available(*session);
    const auto v3 = capture_v3(*session);
    expect_v3_parity(v3, v4);
    expect_unavailable(v4.history_interval, history_interval_reason_v1::Unverified);
  };
  // Date: another week is not this account's rounded seed date.
  auto date = good; date.rounded_date += week_seconds; install(w, date); refused("date");
  // Account: the record no longer matches the account's own creation week.
  install(w, good);
  w.get_account().set_createtime(synthetic_createtime + week_seconds);
  refused("account");
  // Genesis of the pinned view is not the verified profile genesis.
  auto genesis = good; genesis.view.genesis_hash = wide(block_id(0)); install(w, genesis); refused("genesis");
  // Selected boundary: the captured minimum is not first_height - 1.
  install(w, good); w.m_minimum_height = 3; refused("minimum");
  // Tip: another connected hash, or a newer local tip than the record reached.
  auto hash = good; hash.connected_hash = wide(block_id(99)); install(w, hash); refused("tip-hash");
  install(w, good);
  extend_local_chain(w, 13);
  refused("tip-height");
  // Retained anchor identity differs.
  auto anchor = record_for(w, 5, 12, origin_phase::Complete);
  anchor.anchor_hash = wide(block_id(77));
  install(w, anchor);
  refused("anchor");
  // The same record over the matching chain is observed again.
  install(w, record_for(w, 5, 12, origin_phase::Complete));
  {
    const auto matched = capture_available(*session);
    EXPECT_EQ(history_interval_state_v1::Complete, matched.history_interval.state);
  }
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_INTERVAL_TEST(dirty_progress_is_not_checkpointed_and_clean_state_is)
{
  fixture f;
  auto session = f.open(f.control());
  auto& w = access::wallet(*session);
  use_test_clock(w, 1);
  extend_local_chain(w, 8);
  install(w, record_for(w, 3, 7, origin_phase::Complete));
  {
    const auto clean = capture_available(*session);
    EXPECT_TRUE(clean.history_interval.checkpointed);
  }
  dirty_of(w) = true;
  EXPECT_EQ(session_state::OpenDirty, session->state());
  {
    const auto v4 = capture_available(*session);
    EXPECT_EQ(history_interval_state_v1::Complete, v4.history_interval.state);
    EXPECT_FALSE(v4.history_interval.checkpointed);
    EXPECT_FALSE(v4.scan_coverage.checkpointed);
  }
  dirty_of(w) = false; // Synthetic flag only; nothing was written.
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_INTERVAL_TEST(revocation_at_the_final_sample_has_no_payload_and_is_sticky)
{
  fixture f;
  const auto c = f.control();
  auto session = f.open(c);
  auto& w = access::wallet(*session);
  use_test_clock(w, 1);
  extend_local_chain(w, 8);
  install(w, record_for(w, 3, 7, origin_phase::Complete));
  const auto before = c->authorization_reads;
  {
    const auto sampled = capture_available(*session);
    (void)sampled;
  }
  const auto per_capture = c->authorization_reads - before;
  ASSERT_GE(per_capture, 3u); // v3's two samples plus the interval's final sample.
  // Revoke exactly at the last sample: v3 succeeded, the interval copy did not publish.
  c->revoke_at_authorization_read = c->authorization_reads + per_capture;
  const auto revoked = capture(*session);
  EXPECT_EQ(snapshot_status::Revoked, revoked.status);
  EXPECT_TRUE(revoked.observation == nullptr);
  EXPECT_TRUE(revoked.owned_outputs.empty());
  expect_unavailable(revoked.history_interval, history_interval_reason_v1::Absent);
  c->authorized = true;
  c->revoke_at_authorization_read = 0;
  EXPECT_EQ(snapshot_status::Revoked, capture(*session).status);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_INTERVAL_TEST(scratch_replay_is_never_described_and_reopen_has_a_new_generation_without_origin)
{
  fixture f;
  uint64_t first_generation = 0;
  {
    auto session = f.open(f.control());
    auto& w = access::wallet(*session);
    use_test_clock(w, 1);
    extend_local_chain(w, 8);
    install(w, record_for(w, 3, 7, origin_phase::Complete));
    const auto v4 = capture(*session);
    available(v4);
    first_generation = access::generation(*session);
    EXPECT_EQ(first_generation, v4.observation->session_generation);
    access::adopt_scratch(*session);
    // Existing v3 coverage refuses while scratch exists; v4 cannot differ.
    EXPECT_EQ(capture_v3(*session).status, capture(*session).status);
    EXPECT_TRUE(capture(*session).observation == nullptr);
    // The extraction itself never describes the authoritative record then.
    expect_unavailable(access::observe(*session, *v4.observation, v4.scan_coverage),
      history_interval_reason_v1::ScratchReplay);
    access::drop_scratch(*session);
    // An earlier page from this session is still checked against its revision.
    EXPECT_EQ(history_interval_state_v1::Complete,
      access::observe(*session, *v4.observation, v4.scan_coverage).state);
    // After a possible state change the old page is stale and never described.
    access::advance_revision(*session);
    expect_unavailable(access::observe(*session, *v4.observation, v4.scan_coverage),
      history_interval_reason_v1::Unverified);
    const auto current = capture(*session);
    available(current);
    EXPECT_EQ(history_interval_state_v1::Complete, current.history_interval.state);
    EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  }
  // Nothing was checkpointed: the reopened file has no origin and a new generation.
  auto again = f.open(f.control());
  use_test_clock(access::wallet(*again), 1);
  const auto reopened = capture(*again);
  available(reopened);
  EXPECT_NE(first_generation, reopened.observation->session_generation);
  expect_unavailable(reopened.history_interval, history_interval_reason_v1::Absent);
  EXPECT_EQ(release_result::ReleasedUnchanged, again->release_unchanged());
}

NATIVE_INTERVAL_TEST(v2_and_v3_are_versioned_pages_of_one_capture_with_the_exact_retained_birthday)
{
  fixture f;
  auto session = f.open(f.control());
  auto& w = access::wallet(*session);
  use_test_clock(w, 1);
  extend_local_chain(w, 6); // Tip 5.
  w.get_account().set_createtime(synthetic_createtime);
  {
    const auto v1 = session->capture_product_snapshot_v1(snapshot_request{});
    const auto v2 = capture_v2(*session);
    const auto v3 = capture_v3(*session);
    if (v1.status != snapshot_status::Available || !v1.observation ||
        v2.status != snapshot_status::Available || !v2.observation)
      fail_closed("v1/v2 page has no available observation");
    EXPECT_EQ(1u, v1.version);
    EXPECT_EQ(2u, v2.version);
    EXPECT_EQ(6u, v2.observation->local_blocks);
    EXPECT_EQ(5u, v2.observation->local_tip_height);
    EXPECT_EQ(access::generation(*session), v2.observation->session_generation);
    EXPECT_EQ(v1.observation->revision, v2.observation->revision);
    EXPECT_EQ(v1.observation->assets.size(), v2.observation->assets.size());
    // One owned-output entry per returned activity row, none otherwise.
    EXPECT_EQ(v2.observation->activity.size(), v2.owned_outputs.size());
    EXPECT_TRUE(v2.owned_outputs.empty());
    EXPECT_EQ(v1.scan_coverage.version, v2.scan_coverage.version);
    EXPECT_EQ(v1.scan_coverage.source, v2.scan_coverage.source);
    EXPECT_EQ(v1.scan_coverage.minimum_height, v2.scan_coverage.minimum_height);
    EXPECT_EQ(v1.scan_coverage.checkpointed, v2.scan_coverage.checkpointed);
    // v3 is the v2 page plus the stored account value, copied without rounding.
    if (v3.status != snapshot_status::Available || !v3.observation)
      fail_closed("v3 page has no available observation");
    EXPECT_EQ(v2.observation->local_blocks, v3.observation->local_blocks);
    EXPECT_EQ(v2.owned_outputs.size(), v3.owned_outputs.size());
    EXPECT_EQ(v2.scan_coverage.source, v3.scan_coverage.source);
    EXPECT_EQ(1u, v3.retained_birthday.version);
    EXPECT_TRUE(v3.retained_birthday.available);
    EXPECT_EQ(synthetic_createtime, v3.retained_birthday.timestamp);
    EXPECT_NE(synthetic_rounded, v3.retained_birthday.timestamp);
  }
  // A zero stored value is unavailable, never a fabricated date; the page stays.
  w.get_account().set_createtime(0);
  {
    const auto v3 = capture_v3(*session);
    EXPECT_EQ(snapshot_status::Available, v3.status);
    EXPECT_TRUE(v3.observation != nullptr);
    EXPECT_EQ(1u, v3.retained_birthday.version);
    EXPECT_FALSE(v3.retained_birthday.available);
    EXPECT_EQ(0u, v3.retained_birthday.timestamp);
  }
  // Transient replay scratch has no admitted boundary: v2 and v3 refuse alike,
  // with no payload in any extension. The boundary exists (minimum set).
  install(w, record_for(w, 3, 5, origin_phase::Complete));
  EXPECT_EQ(snapshot_status::Available, capture_v2(*session).status);
  access::adopt_scratch(*session);
  {
    const auto v2 = capture_v2(*session);
    const auto v3 = capture_v3(*session);
    EXPECT_EQ(snapshot_status::InvalidData, v2.status);
    EXPECT_EQ(v2.status, v3.status);
    EXPECT_TRUE(v2.observation == nullptr && v3.observation == nullptr);
    EXPECT_TRUE(v2.owned_outputs.empty() && v3.owned_outputs.empty());
    EXPECT_FALSE(v3.retained_birthday.available);
    EXPECT_EQ(0u, v3.retained_birthday.timestamp);
  }
  access::drop_scratch(*session);
  EXPECT_EQ(snapshot_status::Available, capture_v3(*session).status);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_INTERVAL_TEST(v2_owned_outputs_follow_the_page_rows_with_sorted_recorded_indexes)
{
  fixture f;
  auto session = f.open(f.control());
  auto& w = access::wallet(*session);
  use_test_clock(w, 1);
  extend_local_chain(w, 6);
  // Newest first by timestamp: A (outputs 2 and 0 recorded out of order), then B (none).
  w.m_transfer_history.push_back(retained_row(0xb0, 3, 100, 1, {}));
  w.m_transfer_history.push_back(retained_row(0xa0, 2, 200, 3, {2, 0}));
  {
    const auto v2 = capture_v2(*session);
    if (v2.status != snapshot_status::Available || !v2.observation)
      fail_closed("v2 page with retained rows has no available observation");
    ASSERT_EQ(2u, v2.observation->activity.size());
    ASSERT_EQ(2u, v2.owned_outputs.size());
    EXPECT_TRUE(v2.observation->activity[0].tx_hash == row_id(0xa0));
    EXPECT_TRUE(v2.owned_outputs[0].tx_hash == row_id(0xa0));
    ASSERT_EQ(2u, v2.owned_outputs[0].outputs.size());
    expect_unknown_index_only(v2.owned_outputs[0].outputs[0], 0);
    expect_unknown_index_only(v2.owned_outputs[0].outputs[1], 2);
    EXPECT_TRUE(v2.owned_outputs[1].tx_hash == row_id(0xb0));
    EXPECT_TRUE(v2.owned_outputs[1].outputs.empty());
    // v3 carries the identical owned-output extension of the same rows.
    const auto v3 = capture_v3(*session);
    ASSERT_EQ(v2.owned_outputs.size(), v3.owned_outputs.size());
    for (std::size_t i = 0; i != v2.owned_outputs.size(); ++i)
    {
      EXPECT_TRUE(v2.owned_outputs[i].tx_hash == v3.owned_outputs[i].tx_hash);
      EXPECT_EQ(v2.owned_outputs[i].outputs.size(), v3.owned_outputs[i].outputs.size());
    }
  }
  // Page bounds: one row per page, the continuation carries the next row's outputs.
  {
    snapshot_request first;
    first.page_size = 1;
    const auto page = capture_v2(*session, first);
    if (page.status != snapshot_status::Available || !page.observation)
      fail_closed("v2 first page has no available observation");
    ASSERT_EQ(1u, page.owned_outputs.size());
    EXPECT_TRUE(page.owned_outputs[0].tx_hash == row_id(0xa0));
    ASSERT_TRUE(page.observation->coverage.more_rows);
    snapshot_request next;
    next.page_size = 1;
    next.continuation = true;
    next.cursor = page.observation->coverage.next;
    const auto rest = capture_v2(*session, next);
    if (rest.status != snapshot_status::Available || !rest.observation)
      fail_closed("v2 continuation has no available observation");
    ASSERT_EQ(1u, rest.owned_outputs.size());
    EXPECT_TRUE(rest.owned_outputs[0].tx_hash == row_id(0xb0));
    EXPECT_FALSE(rest.observation->coverage.more_rows);
  }
  // A duplicate or out-of-range recorded index is inconsistent local data for
  // the owned-output extension only: v2/v3 refuse without payload, v1 is unchanged.
  w.m_transfer_history.back() = retained_row(0xa0, 2, 200, 3, {1, 1});
  {
    const auto v2 = capture_v2(*session);
    EXPECT_EQ(snapshot_status::InvalidData, v2.status);
    EXPECT_TRUE(v2.observation == nullptr);
    EXPECT_TRUE(v2.owned_outputs.empty());
    EXPECT_EQ(snapshot_status::InvalidData, capture_v3(*session).status);
    EXPECT_EQ(snapshot_status::Available, session->capture_product_snapshot_v1(snapshot_request{}).status);
  }
  w.m_transfer_history.back() = retained_row(0xa0, 2, 200, 3, {3});
  {
    const auto v2 = capture_v2(*session);
    EXPECT_EQ(snapshot_status::InvalidData, v2.status);
    EXPECT_TRUE(v2.observation == nullptr && v2.owned_outputs.empty());
  }
  w.m_transfer_history.clear(); // Synthetic rows only; nothing was written.
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_INTERVAL_TEST(v2_samples_authority_while_building_owned_outputs_and_revocation_has_no_payload)
{
  fixture f;
  const auto c = f.control();
  auto session = f.open(c);
  auto& w = access::wallet(*session);
  use_test_clock(w, 1);
  extend_local_chain(w, 6);
  w.m_transfer_history.push_back(retained_row(0xa0, 2, 200, 3, {0}));
  auto before = c->authorization_reads;
  EXPECT_EQ(snapshot_status::Available, session->capture_product_snapshot_v1(snapshot_request{}).status);
  const auto per_v1 = c->authorization_reads - before;
  before = c->authorization_reads;
  {
    const auto sampled = capture_v2(*session);
    EXPECT_EQ(snapshot_status::Available, sampled.status);
  }
  const auto per_v2 = c->authorization_reads - before;
  // The owned-output build samples authority beyond v1's entry and final samples.
  ASSERT_GT(per_v2, per_v1);
  // Revoke at the first sample inside the owned-output build.
  c->revoke_at_authorization_read = c->authorization_reads + 2;
  {
    const auto revoked = capture_v2(*session);
    EXPECT_EQ(snapshot_status::Revoked, revoked.status);
    EXPECT_TRUE(revoked.observation == nullptr);
    EXPECT_TRUE(revoked.owned_outputs.empty());
  }
  c->authorized = true;
  c->revoke_at_authorization_read = 0;
  EXPECT_EQ(snapshot_status::Revoked, capture_v3(*session).status); // Sticky.
  w.m_transfer_history.clear();
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

// ---------------------------------------------------------------------------
// Legacy replay lifecycle. Every case runs the real production prepare_sync,
// step_sync, checkpoint, close and destructor over a really opened legacy
// image (genuine saved minimum 0, no origin lineage). The scanner is the real
// block_sync behind the real core_proxy; the only stand-in is this in-process
// loopback for the scanner's own birthday-prefix and validated-prefix side
// interfaces, serving a synthetic public chain. No endpoint, seed, Restore,
// Create, rekey or funds exist here. No friend-injected or fake scratch.
// ---------------------------------------------------------------------------
using blocks_request = currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request;
using blocks_response = currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response;
using block_entry = currency::block_direct_data_entry;
using prefix_reply = tools::read_only::validated_prefix_response;
namespace header_prefix = tools::read_only::birthday_prefix;
using tools::read_only::step_status;

class steady_source final : public tools::read_only::monotonic_clock
{
public:
  std::chrono::steady_clock::time_point now() const noexcept override { return std::chrono::steady_clock::now(); }
};

// Each armed reply is served once. Any legacy/other request is counted, refused.
class replay_loopback final : public tools::i_core_proxy,
  public tools::read_only::validated_prefix_source, public tools::read_only::birthday_prefix_source
{
public:
  std::shared_ptr<header_prefix::page> headers;
  std::shared_ptr<prefix_reply> blocks;
  size_t header_requests = 0, block_requests = 0, forbidden = 0;
  header_prefix::request last_headers{};
  blocks_request last_blocks{};
  bool fetch_birthday_prefix_page(const header_prefix::request& request, header_prefix::page& out,
    const std::atomic<bool>&) override
  {
    ++header_requests;
    last_headers = request;
    if (!headers) return false;
    out = *headers;
    headers.reset();
    return true;
  }
  bool fetch_validated_prefix(const blocks_request& request, tools::read_only::prefix_selection_limits,
    prefix_reply& out, const std::atomic<bool>&) override
  {
    ++block_requests;
    last_blocks = request;
    if (!blocks) return false;
    out = *blocks;
    blocks.reset();
    return true;
  }
  bool call_COMMAND_RPC_GET_BLOCKS_DIRECT(const blocks_request&, blocks_response&) override
  { ++forbidden; return false; }
  bool call_COMMAND_RPC_SEND_RAW_TX(const currency::COMMAND_RPC_SEND_RAW_TX::request&,
    currency::COMMAND_RPC_SEND_RAW_TX::response&) override { ++forbidden; return false; }
  bool call_COMMAND_RPC_INVOKE(const std::string&, const std::string&, int&, std::string&) override
  { ++forbidden; return false; }
};

crypto::public_key public_point()
{
  crypto::public_key point{};
  // Public Edwards base point, not wallet material.
  require(epee::string_tools::hex_to_pod(
    std::string("5866666666666666666666666666666666666666666666666666666666666666"), point));
  return point;
}

block_entry genesis_entry()
{
  epee::log_space::scoped_log_silence quiet;
  currency::block genesis{};
  require(currency::generate_genesis_block(genesis));
  require(currency::get_block_hash(genesis) == build_profile().genesis);
  auto block = std::make_shared<currency::block_extended_info>();
  block->bl = genesis;
  auto coinbase = std::make_shared<currency::transaction_chain_entry>();
  coinbase->m_global_output_indexes.resize(genesis.miner_tx.vout.size());
  return {block, coinbase, std::list<std::shared_ptr<const currency::transaction_chain_entry>>{}};
}

// Synthetic empty-coinbase child with an explicit header timestamp.
block_entry child(const block_entry& parent, uint64_t timestamp)
{
  auto block = std::make_shared<currency::block_extended_info>();
  block->bl = currency::block{};
  block->bl.major_version = 1;
  block->bl.timestamp = timestamp;
  block->bl.prev_id = currency::get_block_hash(parent.block_ptr->bl);
  block->bl.miner_tx.version = TRANSACTION_VERSION_INITAL;
  block->bl.miner_tx.vin.emplace_back(currency::txin_gen{currency::get_block_height(parent.block_ptr->bl) + 1});
  block->bl.miner_tx.extra.emplace_back(public_point());
  return {block, std::make_shared<currency::transaction_chain_entry>(),
    std::list<std::shared_ptr<const currency::transaction_chain_entry>>{}};
}

crypto::hash id_of(const block_entry& entry) { return currency::get_block_hash(entry.block_ptr->bl); }

// g(0) -> b1(1, before the seed-date cutoff) -> b2(2, first at/after it) -> b3(3, view).
// The account's actual stored creation time selects the boundary H=2, anchor b1.
struct legacy_chain
{
  uint64_t rounded = 0;
  block_entry g, b1, b2, b3;
  explicit legacy_chain(uint64_t createtime)
  {
    require(createtime > week_offset + week_seconds);
    rounded = week_offset + (createtime - week_offset) / week_seconds * week_seconds;
    g = genesis_entry();
    b1 = child(g, 1);
    b2 = child(b1, rounded);
    b3 = child(b2, rounded + 120);
  }
  std::shared_ptr<header_prefix::page> page() const
  {
    auto out = std::make_shared<header_prefix::page>();
    out->view.target_height = 3;
    out->view.target_hash = wide(id_of(b3));
    out->view.token = out->view.target_hash;
    out->view.genesis_hash = wide(id_of(g));
    out->start_height = 0;
    const block_entry* entries[] = {&g, &b1, &b2, &b3};
    for (uint64_t h = 0; h != 4; ++h)
    {
      auto& row = out->headers[h];
      row.height = h;
      row.timestamp = entries[h]->block_ptr->bl.timestamp;
      row.hash = wide(id_of(*entries[h]));
      if (h) row.previous_hash = wide(id_of(*entries[h - 1]));
    }
    out->count = 4;
    out->decode_clock = std::make_shared<steady_source>();
    out->decode_deadline = std::chrono::steady_clock::now() + std::chrono::hours(1);
    return out;
  }
  static std::shared_ptr<prefix_reply> reply(uint64_t start, uint64_t current,
    std::initializer_list<const block_entry*> selected, size_t full)
  {
    auto out = std::make_shared<prefix_reply>();
    out->selected.status = API_RETURN_CODE_OK;
    out->selected.start_height = start;
    out->selected.current_height = current;
    for (const auto* entry : selected) out->selected.blocks.push_back(*entry);
    out->full_block_count = full;
    out->prefix_truncated = selected.size() != full;
    return out;
  }
};

tools::read_only::step_limits replay_limits() { return {8, 16}; }

crypto::secret_key inert_tx_key()
{
  crypto::secret_key key{};
  reinterpret_cast<unsigned char*>(&key)[0] = 0x42; // inert test bytes, never used to sign
  return key;
}

// The admitted original, observed through the public session and its wallet.
void expect_original_unchanged(file_session& session, tools::wallet2& original, const sync_step_result& r)
{
  EXPECT_TRUE(&access::wallet(session) == &original);
  EXPECT_EQ(1u, original.get_blockchain_current_size());
  EXPECT_FALSE(dirty_of(original));
  EXPECT_EQ(0u, original.m_minimum_height);
  EXPECT_TRUE(origin_of(original).phase == origin_phase::Absent);
  // Scratch never publishes tip, rows, daemon facts or a possible change.
  EXPECT_EQ(1u, r.scan.local_blocks_before);
  EXPECT_EQ(1u, r.scan.local_blocks_after);
  EXPECT_EQ(0u, r.scan.blocks_added);
  EXPECT_FALSE(r.scan.has_daemon_observation);
  EXPECT_EQ(0u, r.scan.reported_daemon_blocks);
  EXPECT_FALSE(r.scan.batch_truncated);
  EXPECT_FALSE(r.scan.state_may_have_changed);
  // The coverage-free page reads only the authoritative wallet: the original's
  // exact local position and the session's current revision, scratch or not.
  const auto page = session.capture_snapshot(snapshot_request{});
  ASSERT_EQ(snapshot_status::Available, page.status);
  ASSERT_TRUE(page.observation != nullptr);
  EXPECT_EQ(1u, page.observation->local_blocks);
  EXPECT_EQ(0u, page.observation->local_tip_height);
  EXPECT_EQ(access::revision(session), page.observation->revision);
  const auto v1 = session.capture_product_snapshot_v1(snapshot_request{});
  const auto v3 = capture_v3(session);
  if (access::replay(session))
  {
    // No admitted boundary exists for scratch: every coverage-bearing product
    // page (v1 through v4 share one selection) refuses with no payload.
    EXPECT_EQ(snapshot_status::InvalidData, v3.status);
    EXPECT_TRUE(v3.observation == nullptr);
    EXPECT_TRUE(v3.owned_outputs.empty());
    EXPECT_FALSE(v3.retained_birthday.available);
    EXPECT_EQ(v3.status, v1.status);
    EXPECT_TRUE(v1.observation == nullptr);
    EXPECT_EQ(scan_start_source_v1::Unobserved, v1.scan_coverage.source);
    EXPECT_EQ(0u, v1.scan_coverage.minimum_height);
    const auto v4 = capture(session);
    EXPECT_EQ(v3.status, v4.status);
    EXPECT_TRUE(v4.observation == nullptr);
    EXPECT_EQ(history_interval_state_v1::Unavailable, v4.history_interval.state);
    EXPECT_EQ(0u, v4.history_interval.first_height);
  }
  else
  {
    // Scratch and its scanner are gone: the original's saved boundary again.
    EXPECT_EQ(snapshot_status::Available, v3.status);
    EXPECT_EQ(scan_start_source_v1::SavedBoundary, v3.scan_coverage.source);
    ASSERT_EQ(snapshot_status::Available, v1.status);
    ASSERT_TRUE(v1.observation != nullptr);
    EXPECT_EQ(1u, v1.observation->local_blocks);
    EXPECT_EQ(v3.scan_coverage.source, v1.scan_coverage.source);
    EXPECT_EQ(0u, v1.scan_coverage.minimum_height);
  }
}

void expect_preparation_step(file_session& session, std::atomic<bool>& stop, tools::wallet2& original)
{
  const auto r = session.step_sync(stop);
  EXPECT_EQ(sync_step_status::Stepped, r.status);
  EXPECT_EQ(step_status::PreparationProgress, r.scan.status);
  expect_original_unchanged(session, original, r);
}

// Real prepare -> boundary page -> partial bootstrap -> equal-height step ->
// remainder (Complete) -> bounded reconciliation. Returns the ready candidate.
tools::wallet2* drive_to_ready(file_session& session, replay_loopback& net, const legacy_chain& c,
  tools::wallet2& original, bool reconcile = true)
{
  std::atomic<bool> stop{false};
  auto* scratch = access::replay(session);
  if (!scratch || scratch == &original) fail_closed("production prepare built no replay candidate");
  // 1. One birthday-prefix page selects H=2 with anchor b1. No wallet bytes change.
  net.headers = c.page();
  expect_preparation_step(session, stop, original);
  EXPECT_EQ(1u, net.header_requests);
  EXPECT_EQ(0u, net.last_headers.start_height);
  EXPECT_EQ(1u, scratch->get_blockchain_current_size());
  // 2. Truncated bootstrap from the anchor: scratch reaches b2, Scanning.
  net.blocks = legacy_chain::reply(1, 4, {&c.b1, &c.b2}, 3);
  expect_preparation_step(session, stop, original);
  EXPECT_EQ(1u, net.last_blocks.minimum_height);
  EXPECT_EQ(3u, scratch->get_blockchain_current_size());
  EXPECT_TRUE(origin_of(*scratch).phase == origin_phase::Scanning);
  EXPECT_EQ(2u, origin_of(*scratch).first_height);
  EXPECT_EQ(1u, scratch->m_minimum_height);
  // 3. Equal-height reply (known b2 only): nothing added, still Scanning.
  net.blocks = legacy_chain::reply(2, 3, {&c.b2}, 1);
  expect_preparation_step(session, stop, original);
  EXPECT_EQ(3u, scratch->get_blockchain_current_size());
  EXPECT_TRUE(origin_of(*scratch).phase == origin_phase::Scanning);
  // 4. Remainder to the pinned view: Complete in scratch, still not ready.
  net.blocks = legacy_chain::reply(2, 4, {&c.b2, &c.b3}, 2);
  expect_preparation_step(session, stop, original);
  EXPECT_EQ(4u, scratch->get_blockchain_current_size());
  EXPECT_TRUE(origin_of(*scratch).phase == origin_phase::Complete);
  EXPECT_FALSE(access::replay_ready(session));
  EXPECT_EQ(session_state::OpenUnchanged, session.state());
  EXPECT_EQ(0u, net.forbidden);
  if (!reconcile) return scratch;
  // 5. Zero-RPC bounded reconciliation is its own step; only now is it ready.
  const auto requests = net.header_requests + net.block_requests;
  expect_preparation_step(session, stop, original);
  EXPECT_EQ(requests, net.header_requests + net.block_requests);
  EXPECT_TRUE(access::replay(session) == scratch);
  EXPECT_TRUE(access::replay_ready(session));
  EXPECT_TRUE(dirty_of(*scratch));
  EXPECT_EQ(session_state::OpenDirty, session.state());
  return scratch;
}

NATIVE_INTERVAL_TEST(legacy_saved_minimum_image_replays_in_scratch_and_commits_only_after_reconciliation)
{
  fixture f(true);
  const auto c = f.capable(writer_mode::Commit);
  const auto before = read_bytes(f.file());
  auto session = f.open(c);
  auto& original = access::wallet(*session);
  use_test_clock(original, 1);
  require(original.m_minimum_height == 0 && origin_of(original).phase == origin_phase::Absent);
  // Local metadata of the admitted original, withheld from scratch until reconciliation.
  original.m_tx_keys[row_id(0x71)] = inert_tx_key();
  const legacy_chain chain(original.get_account().get_createtime());
  auto net = std::make_shared<replay_loopback>();
  const auto revision = access::revision(*session);
  ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(net, replay_limits()));
  auto* scratch = access::replay(*session);
  ASSERT_TRUE(scratch != nullptr && scratch != &original);
  EXPECT_FALSE(access::replay_ready(*session));
  EXPECT_EQ(revision + 1, access::revision(*session)); // replay boundary invalidates old reviews
  EXPECT_EQ(WALLET_MINIMUM_HEIGHT_UNSET_CONST, scratch->m_minimum_height);
  EXPECT_TRUE(scratch->get_account().get_public_address() == original.get_account().get_public_address());
  EXPECT_EQ(original.get_account().get_createtime(), scratch->get_account().get_createtime());
  EXPECT_TRUE(scratch->m_tx_keys.empty());
  EXPECT_EQ(payment_status::Busy, session->assess_local_payment(payment_request{}).status);
  drive_to_ready(*session, *net, chain, original);
  EXPECT_EQ(1u, scratch->m_tx_keys.count(row_id(0x71)));
  EXPECT_EQ(1u, original.m_tx_keys.count(row_id(0x71)));
  EXPECT_EQ(0u, c->checkpoints);
  EXPECT_TRUE(before == read_bytes(f.file()));
  // Ready is never replayed or stepped again; no transport call.
  {
    std::atomic<bool> stop{false};
    const auto requests = net->header_requests + net->block_requests;
    const auto r = session->step_sync(stop);
    EXPECT_EQ(step_status::PreparationProgress, r.scan.status);
    EXPECT_EQ(requests, net->header_requests + net->block_requests);
    EXPECT_EQ(sync_prepare_result::UnsupportedState, session->prepare_sync(net, replay_limits()));
  }
  EXPECT_EQ(payment_status::Busy, session->assess_local_payment(payment_request{}).status);
  // Positive Committed: only now does the candidate become authoritative.
  EXPECT_EQ(checkpoint_result::Saved, session->checkpoint());
  EXPECT_EQ(1u, c->checkpoints);
  EXPECT_TRUE(access::replay(*session) == nullptr);
  EXPECT_FALSE(access::replay_ready(*session));
  EXPECT_TRUE(&access::wallet(*session) == scratch);
  EXPECT_EQ(revision + 2, access::revision(*session));
  EXPECT_EQ(session_state::OpenUnchanged, session->state());
  EXPECT_FALSE(dirty_of(*scratch));
  EXPECT_TRUE(before != read_bytes(f.file()));
  {
    const auto v4 = capture_available(*session);
    EXPECT_EQ(4u, v4.observation->local_blocks);
    EXPECT_EQ(history_interval_state_v1::Complete, v4.history_interval.state);
    EXPECT_EQ(2u, v4.history_interval.first_height);
    EXPECT_EQ(3u, v4.history_interval.connected_height);
    EXPECT_EQ(chain.rounded, v4.history_interval.rounded_date);
    EXPECT_EQ(1u, v4.scan_coverage.minimum_height);
    EXPECT_EQ(scan_start_source_v1::SeedDate, v4.scan_coverage.source);
  }
  EXPECT_NE(payment_status::Busy, session->assess_local_payment(payment_request{}).status);
  EXPECT_EQ(close_result::ReleasedUnchanged, session->close());
  EXPECT_FALSE(f.storage->held);
  // The committed image is the reconciled candidate: origin, boundary and metadata.
  auto again = f.open(f.control());
  auto& reopened = access::wallet(*again);
  use_test_clock(reopened, 1);
  {
    const auto v4 = capture_available(*again);
    EXPECT_EQ(history_interval_state_v1::Complete, v4.history_interval.state);
    EXPECT_TRUE(v4.history_interval.checkpointed);
    EXPECT_EQ(scan_start_source_v1::SavedBoundary, v4.scan_coverage.source);
    EXPECT_EQ(1u, v4.scan_coverage.minimum_height);
  }
  EXPECT_EQ(1u, reopened.m_tx_keys.count(row_id(0x71)));
  EXPECT_EQ(release_result::ReleasedUnchanged, again->release_unchanged());
}

NATIVE_INTERVAL_TEST(rejected_checkpoint_keeps_the_original_authoritative_and_the_ready_image_retained)
{
  fixture f(true);
  const auto c = f.capable(writer_mode::Reject);
  const auto before = read_bytes(f.file());
  auto session = f.open(c);
  auto& original = access::wallet(*session);
  use_test_clock(original, 1);
  const legacy_chain chain(original.get_account().get_createtime());
  auto net = std::make_shared<replay_loopback>();
  ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(net, replay_limits()));
  auto* ready = drive_to_ready(*session, *net, chain, original);
  const auto revision = access::revision(*session);
  EXPECT_EQ(checkpoint_result::Rejected, session->checkpoint());
  EXPECT_TRUE(&access::wallet(*session) == &original);
  EXPECT_TRUE(access::replay(*session) == ready);
  EXPECT_TRUE(access::replay_ready(*session));
  EXPECT_EQ(revision, access::revision(*session));
  EXPECT_EQ(session_state::OpenDirty, session->state());
  EXPECT_EQ(1u, original.get_blockchain_current_size());
  EXPECT_TRUE(before == read_bytes(f.file()));
  // Close attempts the save; a definite refusal keeps everything (no retry/rollback).
  EXPECT_EQ(close_result::RetainedFailure, session->close());
  EXPECT_EQ(2u, c->checkpoints);
  EXPECT_TRUE(access::replay(*session) == ready);
  EXPECT_TRUE(f.storage->held);
  // A later explicit Committed close publishes exactly the retained ready image.
  c->writer = writer_mode::Commit;
  EXPECT_EQ(close_result::ReleasedSaved, session->close());
  EXPECT_FALSE(f.storage->held);
  EXPECT_TRUE(before != read_bytes(f.file()));
  auto again = f.open(f.control());
  use_test_clock(access::wallet(*again), 1);
  EXPECT_EQ(history_interval_state_v1::Complete, capture_available(*again).history_interval.state);
  EXPECT_EQ(release_result::ReleasedUnchanged, again->release_unchanged());
}

NATIVE_INTERVAL_TEST(unknown_checkpoint_outcome_is_sticky_and_never_swaps_or_rewrites_the_original)
{
  fixture f(true);
  const auto c = f.capable(writer_mode::Unknown);
  const auto before = read_bytes(f.file());
  {
    auto session = f.open(c);
    auto& original = access::wallet(*session);
    use_test_clock(original, 1);
    const legacy_chain chain(original.get_account().get_createtime());
    auto net = std::make_shared<replay_loopback>();
    ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(net, replay_limits()));
    drive_to_ready(*session, *net, chain, original);
    EXPECT_EQ(checkpoint_result::OutcomeUnknown, session->checkpoint());
    EXPECT_EQ(session_state::OutcomeUnknown, session->state());
    EXPECT_TRUE(&access::wallet(*session) == &original);
    EXPECT_TRUE(access::replay_ready(*session));
    EXPECT_EQ(close_result::OutcomeUnknown, session->close());
    EXPECT_EQ(checkpoint_result::OutcomeUnknown, session->checkpoint());
    EXPECT_EQ(1u, c->checkpoints);
    EXPECT_TRUE(f.storage->held);
  } // Destruction quiesces scanner and scratch before material; it never saves.
  EXPECT_FALSE(f.storage->held);
  EXPECT_EQ(1u, c->checkpoints);
  EXPECT_TRUE(before == read_bytes(f.file()));
  auto again = f.open(f.control());
  use_test_clock(access::wallet(*again), 1);
  expect_unavailable(capture_available(*again).history_interval, history_interval_reason_v1::Absent);
  EXPECT_EQ(release_result::ReleasedUnchanged, again->release_unchanged());
}

NATIVE_INTERVAL_TEST(cancelled_reconciliation_close_and_destruction_dispose_only_scratch)
{
  fixture f(true);
  const auto c = f.capable(writer_mode::Commit);
  const auto before = read_bytes(f.file());
  {
    auto session = f.open(c);
    auto& original = access::wallet(*session);
    use_test_clock(original, 1);
    const legacy_chain chain(original.get_account().get_createtime());
    auto net = std::make_shared<replay_loopback>();
    ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(net, replay_limits()));
    drive_to_ready(*session, *net, chain, original, false);
    const auto revision = access::revision(*session);
    // Owner stop during the reconciliation step: scratch only is disposed.
    std::atomic<bool> stop{true};
    const auto r = session->step_sync(stop);
    EXPECT_EQ(sync_step_status::Stepped, r.status);
    EXPECT_EQ(step_status::Cancelled, r.scan.status);
    EXPECT_TRUE(access::replay(*session) == nullptr);
    EXPECT_FALSE(access::replay_ready(*session));
    expect_original_unchanged(*session, original, r);
    EXPECT_EQ(revision, access::revision(*session));
    EXPECT_EQ(session_state::OpenUnchanged, session->state());
    EXPECT_NE(payment_status::Busy, session->assess_local_payment(payment_request{}).status);
    std::atomic<bool> go{false};
    EXPECT_EQ(sync_step_status::NotPrepared, session->step_sync(go).status); // no automatic retry
    // Explicit re-preparation builds a new fresh candidate from the original.
    ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(net, replay_limits()));
    ASSERT_TRUE(access::replay(*session) != nullptr);
    EXPECT_EQ(1u, access::replay(*session)->get_blockchain_current_size());
    // Close abandons nonready scratch and saves nothing.
    EXPECT_EQ(close_result::ReleasedUnchanged, session->close());
    EXPECT_EQ(0u, c->checkpoints);
    EXPECT_FALSE(f.storage->held);
  }
  EXPECT_TRUE(before == read_bytes(f.file()));
  {
    // Destruction with a READY image: scanner/scratch die before material; no save.
    auto session = f.open(c);
    auto& original = access::wallet(*session);
    use_test_clock(original, 1);
    const legacy_chain chain(original.get_account().get_createtime());
    auto net = std::make_shared<replay_loopback>();
    ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(net, replay_limits()));
    drive_to_ready(*session, *net, chain, original);
    EXPECT_EQ(release_result::Unavailable, session->release_unchanged()); // ready is unsaved progress
    EXPECT_TRUE(f.storage->held);
  }
  EXPECT_FALSE(f.storage->held);
  EXPECT_EQ(0u, c->checkpoints);
  EXPECT_TRUE(before == read_bytes(f.file()));
}

NATIVE_INTERVAL_TEST(undatable_legacy_scratch_is_disposed_and_a_clean_genesis_image_prepares_directly)
{
  {
    fixture f(true);
    const auto c = f.capable(writer_mode::Commit);
    auto session = f.open(c);
    auto& original = access::wallet(*session);
    use_test_clock(original, 1);
    // An account without a usable stored creation time: the candidate copies
    // it exactly and the real scanner refuses before any request.
    original.get_account().set_createtime(0);
    auto net = std::make_shared<replay_loopback>();
    ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(net, replay_limits()));
    ASSERT_TRUE(access::replay(*session) != nullptr);
    std::atomic<bool> stop{false};
    const auto r = session->step_sync(stop);
    EXPECT_EQ(step_status::PreparationRequired, r.scan.status);
    EXPECT_EQ(0u, net->header_requests + net->block_requests);
    EXPECT_TRUE(access::replay(*session) == nullptr); // fresh, never-mutated scratch
    expect_original_unchanged(*session, original, r);
    EXPECT_NE(payment_status::Busy, session->assess_local_payment(payment_request{}).status);
    EXPECT_EQ(sync_step_status::NotPrepared, session->step_sync(stop).status);
    EXPECT_EQ(close_result::ReleasedUnchanged, session->close());
    EXPECT_EQ(0u, c->checkpoints);
  }
  {
    // An untouched genesis image (unset minimum) is not legacy: no scratch.
    fixture f;
    const auto c = f.capable(writer_mode::Commit);
    auto session = f.open(c);
    auto& w = access::wallet(*session);
    use_test_clock(w, 1);
    w.m_minimum_height = WALLET_MINIMUM_HEIGHT_UNSET_CONST;
    require(origin_of(w).phase == origin_phase::Absent);
    auto net = std::make_shared<replay_loopback>();
    ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(net, replay_limits()));
    EXPECT_TRUE(access::replay(*session) == nullptr);
    EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  }
}

#endif
}
