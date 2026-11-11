// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
//
// Source-only candidate. Executing this file generates disposable local wallets
// and invokes the pinned KDF. It requires the separately reviewed core build gate.
#include "gtest/gtest.h"
#include "gtest/gtest-spi.h"
#include "wallet/offline_wallet_session.h"
#include "wallet/wallet2.h"
#include "common/boost_serialization_helper.h"
#include "common/encryption_filter.h"
#include "currency_core/crypto_config.h"
#include "serialization/serialization.h"
#include "serialization/binary_utils.h"
#include <array>
#include <cstring>
#include <cstdio>
#include <limits>
#include <fstream>
#include <exception>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <boost/filesystem.hpp>

namespace offline_fixture_detail
{
namespace fs = boost::filesystem;
using password_material = std::array<uint8_t, 20>;
static_assert(sizeof(password_material) == 20, "fixture password must encode to 40 hex characters");
template<class Archive> class formation_archive
{
public:
  using is_saving = typename Archive::is_saving;
  using is_loading = typename Archive::is_loading;
  formation_archive(Archive& archive, uint64_t formation)
    : archive(archive), formation(formation) {}
  formation_archive& operator&(uint64_t& value)
  {
    if (first) { first = false; archive & formation; }
    else archive & value;
    return *this;
  }
  template<class T> formation_archive& operator&(T& value)
  {
    if (first) throw std::runtime_error("fixture formation boundary changed");
    archive & value;
    return *this;
  }
private:
  Archive& archive;
  uint64_t formation;
  bool first = true;
};
template<unsigned Version> struct body_image
{
  tools::wallet2_base_state& wallet;
  uint64_t formation;
  template<class Archive> void serialize(Archive& archive, unsigned)
  {
    // Actual serializer and nested archive types are reused. Only formation is
    // intercepted, while the root object's class version is explicit below.
    formation_archive<Archive> patched(archive, formation);
    wallet.serialize(patched, WALLET_FILE_SERIALIZATION_VERSION);
  }
};
inline void require(bool condition)
{
  if (!condition) throw std::runtime_error("disposable fixture preparation failed");
}
inline std::string read(const fs::path& path)
{
  std::ifstream input(path.string(), std::ios::binary);
  require(input.is_open());
  std::string data((std::istreambuf_iterator<char>(input)), {});
  require(!input.bad());
  return data;
}
inline void write(const fs::path& path, const std::string& data)
{
  std::ofstream output(path.string(), std::ios::binary | std::ios::trunc);
  require(output.is_open());
  output.write(data.data(), static_cast<std::streamsize>(data.size()));
  output.flush();
  require(output.good());
  output.close();
  require(!output.fail());
}
using tree_image = std::map<std::string, std::string>;
inline tree_image snapshot(const fs::path& root)
{
  tree_image result;
  for (fs::recursive_directory_iterator it(root), end; it != end; ++it)
  {
    const auto path = it->path();
    const auto name = path.lexically_relative(root).generic_string();
    if (fs::is_symlink(path)) result.emplace(name, "link:" + fs::read_symlink(path).generic_string());
    else if (fs::is_directory(path)) result.emplace(name + "/", "");
    else { require(fs::is_regular_file(path)); result.emplace(name, read(path)); }
  }
  return result;
}
inline std::string transform_body(const std::string& bytes,
  const std::string& password, const crypto::chacha_iv& iv)
{
  tools::encrypt_chacha20_v2_processer_base cipher(password, iv, CRYPTO_HDS_CHACHA_WALLET_BODY);
  std::string result;
  if (!bytes.empty())
    cipher.process(bytes.data(), static_cast<std::streamsize>(bytes.size()),
      [&](const char* data, std::streamsize length) { result.append(data, static_cast<size_t>(length)); });
  return result;
}
inline bool same_public(const tools::offline::public_info& a, const tools::offline::public_info& b)
{
  return a.spend_public_key == b.spend_public_key && a.view_public_key == b.view_public_key &&
    a.flags == b.flags && a.stored_genesis == b.stored_genesis &&
    a.connection == b.connection && a.sync == b.sync;
}
}
BOOST_CLASS_VERSION(offline_fixture_detail::body_image<169>, 169)
BOOST_CLASS_VERSION(offline_fixture_detail::body_image<170>, 170)
BOOST_CLASS_VERSION(offline_fixture_detail::body_image<171>, 171)

namespace tools { namespace offline {
// Sole material factory: test translation unit, test-owned directory only.
class fixture_access
{
public:
  using checkpoint = fixture_control::checkpoint;
  using read_stage = fixture_control::read_stage;
  using read_fault = fixture_control::read_fault;
  using decoder_fault = fixture_control::decoder_fault;
  using decoder_stage = fixture_control::decoder_stage;
  using control = std::shared_ptr<fixture_control>;
  struct counts
  {
    size_t opens, acquired, closes, headers, keys, bodies, finals, eof, parses, kdf, decrypt_attempts, decodes, complete;
    size_t directories, sidecars, votes, whitelist, proxy, stores;
    size_t guard_init, guard_connection, guard_proxy, guard_whitelist;
    size_t destructions, lease_releases, releases, abandonments, destroyed_at, released_at;
    bool quiet_destruction, read_fault_fired;
    std::array<size_t, 29> values() const
    {
      return {{opens, acquired, closes, headers, keys, bodies, finals, eof, parses, kdf, decrypt_attempts, decodes, complete,
        directories, sidecars, votes, whitelist, proxy, stores,
        guard_init, guard_connection, guard_proxy, guard_whitelist, destructions,
        lease_releases, releases, abandonments, destroyed_at, released_at}};
    }
  };
  static control observe() { return control(new fixture_control()); }
  static counts inspect(const control& c)
  {
    return {c->file_opens, c->file_acquisitions, c->file_closes, c->header_reads, c->key_reads, c->body_reads,
      c->final_reads, c->clean_eof_reads, c->key_parses, c->kdf_calls, c->decrypt_attempts, c->body_decodes, c->body_completions,
      c->directory_creates, c->sidecar_opens, c->voting_reads, c->whitelist_fetches,
      c->proxy_calls, c->store_calls, c->guarded_init,
      c->guarded_connection, c->guarded_proxy, c->guarded_whitelist,
      c->wallet_destructions, c->lease_releases, c->successful_releases,
      c->abandonments, c->wallet_destroy_sequence, c->lease_release_sequence,
      c->destruction_was_silenced, c->read_fault_fired};
  }
  static void fail(const control& c, checkpoint p) { c->fail_at = p; }
  static void fail_read(const control& c, read_stage s, read_fault f)
  { c->fail_read_stage = s; c->read_failure = f; }
  static void fail_decoder(const control& c, decoder_fault f,
    decoder_stage stage = decoder_stage::BeforeArchive)
  { c->decoder_failure = f; c->fail_decoder_stage = stage; }
  static void fail_close(const control& c) { c->close_failure = true; }
  static bool holds(const prepared_wallet_material& m) { return m.m_payload && m.m_payload->lease; }
  static const void* lease_identity(const prepared_wallet_material& m)
  { return m.m_payload ? m.m_payload->lease.get() : nullptr; }
  static void fault(offline_wallet_session& s) { s.m_state = session_state::Faulted; }
  static std::unique_ptr<wallet2> offline_wallet(const control& c)
  { return std::unique_ptr<wallet2>(new wallet2(wallet2::offline_construction_tag{}, c)); }
  static public_info identity(wallet2& wallet)
  {
    public_info result;
    const auto& address = wallet.m_account.get_public_address();
    result.spend_public_key = address.spend_public_key;
    result.view_public_key = address.view_public_key;
    result.flags = address.flags;
    result.stored_genesis = wallet.m_chain.get_genesis();
    return result;
  }
  class disposable_fixture
  {
  public:
    explicit disposable_fixture(bool generate_fixture = true)
    {
      try
      {
      namespace fs = boost::filesystem;
      const auto parent = fs::canonical(fs::temp_directory_path());
      const auto requested = parent / fs::unique_path("zano-offline-fixture-%%%%-%%%%-%%%%");
      offline_fixture_detail::require(fs::create_directory(requested));
      root = fs::canonical(requested);
      offline_fixture_detail::require(root.parent_path() == parent && root == requested);
      path = root / "private-path-sentinel.wallet";
      if (!generate_fixture) return; // unsupported-profile test performs no wallet/crypto work
      {
        epee::log_space::scoped_log_silence silence;
        password = epee::string_tools::pod_to_hex(
          crypto::rand<offline_fixture_detail::password_material>());
        offline_fixture_detail::require(currency::validate_password(password));
        auto setup = offline_wallet(observe());
        setup->generate(path.wstring(), password, false);
        expected = identity(*setup);
        baseline = offline_fixture_detail::read(path);
        offline_fixture_detail::require(baseline.size() > sizeof(header));
        std::memcpy(&header, baseline.data(), sizeof(header));
        offline_fixture_detail::require(header.m_cb_keys > 0 &&
          baseline.size() > sizeof(header) + header.m_cb_keys);
        const auto key_bytes = baseline.substr(sizeof(header), header.m_cb_keys);
        setup->load_keys(key_bytes, password, header.m_signature, key_data, &body_password);
        decoded_body = offline_fixture_detail::transform_body(
          baseline.substr(sizeof(header) + header.m_cb_keys), body_password, key_data.iv);
        // Mandatory producer equivalence. No mutation can claim a format
        // boundary if the test wrapper does not reproduce the current bytes.
        offline_fixture_detail::require(serialize_body<170>(*setup, CURRENCY_FORMATION_VERSION) == decoded_body);
        old_archive = serialize_body<169>(*setup, CURRENCY_FORMATION_VERSION);
        future_archive = serialize_body<171>(*setup, CURRENCY_FORMATION_VERSION);
        wrong_formation = serialize_body<170>(*setup, CURRENCY_FORMATION_VERSION + 1);
        // The other pinned profile's formation, never this build's.
        opposite_formation = serialize_body<170>(*setup, CURRENCY_FORMATION_VERSION == 84 ? 103 : 84);
        setup.reset();
      }
#ifdef TESTNET
      // open_prepared remains a MAINNET-only facade. TESTNET cases use the
      // shared strict loader directly through strict_load().
      return;
#endif
      offline_fixture_detail::write(root / "sibling-sentinel", "unrelated disposable sibling");
      offline_fixture_detail::write(root / "private-path-sentinel.outkey2ki", "corrupt disposable sidecar");
      offline_fixture_detail::write(root / "voting-sentinel.json", "{not valid json");
      auto material = prepare();
      std::unique_ptr<offline_wallet_session> session;
      offline_fixture_detail::require(open_prepared(material, session) == open_result::Opened);
      offline_fixture_detail::require(session &&
        offline_fixture_detail::same_public(session->get_info().info, expected));
      offline_fixture_detail::require(session->release_unchanged() == release_result::ReleasedUnchanged);
      session.reset();
      }
      catch (...) { cleanup(); throw std::runtime_error("disposable fixture preparation failed"); }
    }
    ~disposable_fixture() noexcept { cleanup(); }
    void cleanup() noexcept
    {
      if (root.empty()) return;
      try
      {
      namespace fs = boost::filesystem;
      for (const auto& c : leases)
        if (inspect(c).lease_releases != 1)
        { ADD_FAILURE() << "disposable fixture still owns a lease"; return; }
      boost::system::error_code error;
      const auto current = fs::canonical(root, error);
      if (error || current != root || !fs::is_directory(current))
      { ADD_FAILURE() << "disposable fixture root changed"; return; }
      fs::remove_all(current, error);
      if (error) ADD_FAILURE() << "disposable fixture cleanup failed";
      root.clear();
      }
      catch (...) { ADD_FAILURE() << "disposable fixture cleanup failed"; }
    }
    prepared_wallet_material prepare(control c = observe())
    { return prepare_at(path, expected, password, std::move(c)); }
    prepared_wallet_material prepare_at(const boost::filesystem::path& candidate,
      const public_info& public_expectation, const std::string& secret, control c)
    {
      const auto relative = candidate.lexically_relative(root);
      offline_fixture_detail::require(!relative.empty() && !relative.is_absolute() &&
        *relative.begin() != "..");
      prepared_wallet_material result;
      result.m_payload.reset(new prepared_wallet_material::payload());
      result.m_payload->path = candidate.wstring();
      result.m_payload->password = secret;
      result.m_payload->expected = public_expectation;
      result.m_payload->control = c;
      result.m_payload->lease.reset(new storage_lease(c));
      leases.push_back(c);
      return result;
    }
    void restore() { offline_fixture_detail::write(path, baseline); }
    void envelope(wallet_file_binary_header h, const std::string& keys)
    {
      offline_fixture_detail::require(keys.size() <= UINT16_MAX);
      h.m_cb_keys = static_cast<uint16_t>(keys.size());
      offline_fixture_detail::write(path, std::string(reinterpret_cast<const char*>(&h), sizeof(h)) +
        keys + baseline.substr(sizeof(header) + header.m_cb_keys));
    }
    std::string keys() const { return baseline.substr(sizeof(header), header.m_cb_keys); }
    void body(const std::string& decoded)
    {
      offline_fixture_detail::write(path, baseline.substr(0, sizeof(header) + header.m_cb_keys) +
        offline_fixture_detail::transform_body(decoded, body_password, key_data.iv));
    }
    void key_profile(wallet2::keys_file_data changed)
    {
      std::string bytes;
      offline_fixture_detail::require(::serialization::dump_binary(changed, bytes));
      envelope(header, bytes);
    }
    void watch_only(bool auditable)
    {
      epee::log_space::scoped_log_silence silence;
      auto setup = offline_wallet(observe());
      wallet2::keys_file_data parsed{};
      setup->load_keys(keys(), password, header.m_signature, parsed);
      auto& address = const_cast<currency::account_public_address&>(setup->m_account.get_public_address());
      if (auditable) address.flags |= ACCOUNT_PUBLIC_ADDRESS_FLAG_AUDITABLE;
      setup->m_account.make_account_watch_only();
      std::string bytes, new_body_password;
      wallet2::keys_file_data replacement{};
      offline_fixture_detail::require(setup->store_keys(bytes, password, replacement, true, &new_body_password));
      auto h = header;
      h.m_cb_keys = static_cast<uint16_t>(bytes.size());
      offline_fixture_detail::write(path, std::string(reinterpret_cast<const char*>(&h), sizeof(h)) + bytes +
        offline_fixture_detail::transform_body(decoded_body, new_body_password, replacement.iv));
      setup.reset();
    }
    boost::filesystem::path root, path;
    public_info expected;
    wallet_file_binary_header header{};
    wallet2::keys_file_data key_data{};
    std::string baseline, decoded_body, old_archive, future_archive, wrong_formation, opposite_formation;
  private:
    template<unsigned Version> static std::string serialize_body(wallet2& wallet, uint64_t formation)
    {
      offline_fixture_detail::body_image<Version> image{wallet, formation};
      std::ostringstream output(std::ios::binary);
      eos::portable_oarchive archive(output);
      archive << image;
      offline_fixture_detail::require(output.good());
      return output.str();
    }
    std::string password, body_password;
    std::vector<control> leases;
    friend class fixture_access;
  };
  static prepared_wallet_material wrong_password(disposable_fixture& f, control c)
  { return f.prepare_at(f.path, f.expected, f.password + "-wrong", std::move(c)); }
  static prepared_wallet_material mismatched(disposable_fixture& f, public_info info, control c)
  { return f.prepare_at(f.path, info, f.password, std::move(c)); }
  static prepared_wallet_material alternate_path(disposable_fixture& f,
    const boost::filesystem::path& path, control c)
  { return f.prepare_at(path, f.expected, f.password, std::move(c)); }
  static bool guard_connections(wallet2& wallet)
  {
    const auto original_proxy = wallet.m_core_proxy;
    wallet.init("http://offline-fixture.invalid");
    wallet.reset_connection_addr("http://offline-fixture.invalid");
    const bool proxy = wallet.set_core_proxy(std::make_shared<i_core_proxy>());
    return !proxy && wallet.m_core_proxy == original_proxy;
  }
  static bool guard_whitelist_preserves(wallet2& wallet)
  {
    crypto::public_key key{};
    currency::asset_descriptor_base value{};
    wallet.m_whitelisted_assets.emplace(key, value);
    const bool result = wallet.load_whitelisted_tokens();
    return !result && wallet.m_whitelisted_assets.count(key) == 1 && !wallet.m_whitelist_updated;
  }
  static bool legacy_load(disposable_fixture& f, const control& c, bool expect_recovery)
  {
    epee::log_space::scoped_log_silence silence;
    std::unique_ptr<wallet2> wallet(new wallet2());
    wallet->m_offline_control = c;
    wallet->m_votes_config_path = (f.root / "nonexistent-local-voting.json").string();
    bool recovered = false;
    try { wallet->load(f.path.wstring(), f.password); }
    catch (const error::wallet_load_notice_wallet_restored&) { recovered = true; }
    const bool result = !wallet->m_offline_mode && recovered == expect_recovery &&
      (expect_recovery || offline_fixture_detail::same_public(identity(*wallet), f.expected));
    wallet.reset();
    return result;
  }
  // Direct strict-loader result for the current file, with no session facade.
  static open_result strict_load(disposable_fixture& f, const control& c)
  {
    epee::log_space::scoped_log_silence silence;
    auto wallet = offline_wallet(c);
    try { wallet->load_offline_prepared(f.path.wstring(), f.password); }
    catch (open_result result) { return result; }
    catch (...) { return open_result::Unavailable; }
    return open_result::Opened;
  }
  static bool attach_callback(offline_wallet_session& session,
    const std::shared_ptr<i_wallet2_callback>& callback)
  {
    session.m_wallet->callback(callback);
    return session.m_wallet->get_callback() == callback && session.m_wallet->m_do_rise_transfer;
  }
  static bool deny_resolvers(wallet2& wallet)
  {
    currency::account_public_address simple{};
    currency::address_v extended{};
    std::string payment_id;
    const bool first = wallet.m_core_proxy->get_transfer_address("offline-fixture.invalid", simple, payment_id);
    const bool second = wallet.m_core_proxy->get_transfer_address("offline-fixture.invalid", extended, payment_id);
    return !first && !second;
  }
  static bool voting_path_empty(const wallet2& wallet) { return wallet.m_votes_config_path.empty(); }
  static bool offline_mode(const wallet2& wallet) { return wallet.m_offline_mode; }
  static void reset_base(wallet2& wallet) { wallet.reset_all(); }
};
}}

namespace
{
using namespace tools::offline;
using access = fixture_access;
using fixture = access::disposable_fixture;
using owner = std::unique_ptr<offline_wallet_session>;
namespace fs = boost::filesystem;
static_assert(!std::is_copy_constructible<prepared_wallet_material>::value, "material cannot copy");
static_assert(std::is_nothrow_move_constructible<prepared_wallet_material>::value, "lease transfer cannot throw");
static_assert(std::is_nothrow_move_assignable<prepared_wallet_material>::value, "lease transfer cannot throw");
static_assert(!std::is_copy_constructible<offline_wallet_session>::value, "session cannot copy");
static_assert(!std::is_move_constructible<offline_wallet_session>::value, "session cannot move");
static_assert(!std::is_move_assignable<offline_wallet_session>::value, "session cannot move");
static_assert(std::is_nothrow_destructible<offline_wallet_session>::value, "teardown cannot throw");
static_assert(std::is_nothrow_destructible<storage_lease>::value, "lease teardown cannot throw");
static_assert(noexcept(open_prepared(std::declval<prepared_wallet_material&>(), std::declval<owner&>())),
  "open has a fixed result");
static_assert(noexcept(std::declval<const offline_wallet_session&>().get_info()), "info cannot throw");
static_assert(noexcept(std::declval<offline_wallet_session&>().release_unchanged()), "release cannot throw");
void expect_no_side_effects(const access::control& c)
{
  const auto n = access::inspect(c);
  EXPECT_EQ(0u, n.directories); EXPECT_EQ(0u, n.sidecars); EXPECT_EQ(0u, n.votes);
  EXPECT_EQ(0u, n.whitelist); EXPECT_EQ(0u, n.proxy);
  EXPECT_EQ(0u, n.stores);
}
void expect_failure(fixture& f, prepared_wallet_material& material,
  const access::control& c, open_result expected)
{
  const auto tree = offline_fixture_detail::snapshot(f.root);
  const auto lease = access::lease_identity(material);
  owner out;
  EXPECT_EQ(expected, open_prepared(material, out));
  EXPECT_FALSE(out);
  EXPECT_TRUE(access::holds(material));
  EXPECT_EQ(lease, access::lease_identity(material));
  const auto n = access::inspect(c);
  EXPECT_EQ(0u, n.lease_releases); EXPECT_EQ(n.acquired, n.closes); EXPECT_EQ(0u, n.releases);
  if (n.destructions)
  {
    EXPECT_TRUE(n.quiet_destruction);
  }
  expect_no_side_effects(c);
  // Do not print paths, file contents or material on failure.
  EXPECT_TRUE(tree == offline_fixture_detail::snapshot(f.root));
}

// Every test body runs through this boundary. Normal ASSERT/EXPECT failures
// retain their original lines and behavior. Unexpected helper/legacy/filesystem
// exceptions become a fixed diagnostic before the GTest what() handler sees them.
constexpr const char* helper_failure_message = "offline fixture helper failed";
template<class Body> void material_free_test_boundary(Body&& body)
{
  try { body(); }
  catch (...) { ADD_FAILURE() << helper_failure_message; }
}
#define OFFLINE_WALLET_TEST(name) \
  static void offline_wallet_case_##name(); \
  TEST(offline_wallet_session, name) \
  { material_free_test_boundary(&offline_wallet_case_##name); } \
  static void offline_wallet_case_##name()

constexpr int offline_capture_sink_type = 98653;
struct captured_output
{
  std::mutex mutex;
  std::vector<std::string> records;
};
class offline_memory_sink : public epee::log_space::ibase_log_stream
{
public:
  explicit offline_memory_sink(std::shared_ptr<captured_output> output) : output(std::move(output)) {}
  bool out_buffer(const char* bytes, int length, int, int, const char* = nullptr) override
  {
    std::lock_guard<std::mutex> lock(output->mutex);
    output->records.emplace_back(bytes, static_cast<size_t>(length));
    return true;
  }
  int get_type() override { return offline_capture_sink_type; }
private:
  std::shared_ptr<captured_output> output;
};
class offline_output_capture
{
public:
  offline_output_capture() : output(std::make_shared<captured_output>())
  {
    previous_level = epee::log_space::log_singletone::get_set_log_detalisation_level();
    try
    {
      epee::log_space::log_singletone::get_set_log_detalisation_level(true, LOG_LEVEL_4);
      offline_fixture_detail::require(epee::log_space::log_singletone::add_logger(new offline_memory_sink(output)));
      sink_installed = true;
      LOG_PRINT_L0("offline-fixture-visible-control");
      {
        std::lock_guard<std::mutex> lock(output->mutex);
        offline_fixture_detail::require(!output->records.empty());
      }

      // Exercise a known journalled route, then baseline after observing it.
      const std::string journal_control = "offline-fixture-journal-control";
      offline_fixture_detail::require(epee::log_space::log_singletone::do_log_message(
        journal_control, LOG_LEVEL_0, LOG_COLOR_BLUE, true));
      epee::log_space::log_singletone::get_journal_items(before_journal, LOG_JOURNAL_MAX_ELEMENTS);
      offline_fixture_detail::require(!before_journal.empty() && before_journal.back() == journal_control);
      {
        std::lock_guard<std::mutex> lock(output->mutex);
        output->records.clear();
      }
#if GTEST_HAS_STREAM_REDIRECTION
      // Prove both descriptor captures separately, using material-free text.
      // Stop them, inspect only boolean equality, then start fresh captures.
      begin_stream_capture();
      const std::string out_control = "offline-fixture-stdout-control\n";
      const std::string err_control = "offline-fixture-stderr-control\n";
      offline_fixture_detail::require(std::fputs(out_control.c_str(), stdout) >= 0);
      offline_fixture_detail::require(std::fputs(err_control.c_str(), stderr) >= 0);
      offline_fixture_detail::require(std::fflush(stdout) == 0 && std::fflush(stderr) == 0);
      const auto out = ::testing::internal::GetCapturedStdout();
      stdout_capturing = false;
      const auto err = ::testing::internal::GetCapturedStderr();
      stderr_capturing = false;
      offline_fixture_detail::require(std::string(out.c_str(), out.length()) == out_control);
      offline_fixture_detail::require(std::string(err.c_str(), err.length()) == err_control);
      begin_stream_capture();
#endif
    }
    catch (...)
    {
      stop_stream_capture();
      restore_logger();
      throw std::runtime_error("offline fixture output capture failed");
    }
  }
  ~offline_output_capture() noexcept
  {
    stop_stream_capture();
    restore_logger();
  }
  bool finish()
  {
#if GTEST_HAS_STREAM_REDIRECTION
    if (stdout_capturing)
    {
      const auto out = ::testing::internal::GetCapturedStdout();
      stdout_capturing = false;
      streams_empty = streams_empty && out.empty();
    }
    if (stderr_capturing)
    {
      const auto err = ::testing::internal::GetCapturedStderr();
      stderr_capturing = false;
      streams_empty = streams_empty && err.empty();
    }
#endif
    std::list<std::string> after_journal;
    epee::log_space::log_singletone::get_journal_items(after_journal, LOG_JOURNAL_MAX_ELEMENTS);
    std::lock_guard<std::mutex> lock(output->mutex);
    return streams_empty && output->records.empty() && before_journal == after_journal;
  }
private:
  static void report_cleanup_failure() noexcept
  {
    // GTest may be configured to throw after recording a failure. Teardown must
    // still restore the other global resources without rethrowing raw material.
    try { ADD_FAILURE() << "offline fixture capture cleanup failed"; }
    catch (...) {}
  }
  void begin_stream_capture()
  {
#if GTEST_HAS_STREAM_REDIRECTION
    ::testing::internal::CaptureStdout();
    stdout_capturing = true;
    ::testing::internal::CaptureStderr();
    stderr_capturing = true;
#endif
  }
  void stop_stream_capture() noexcept
  {
#if GTEST_HAS_STREAM_REDIRECTION
    if (stdout_capturing)
    {
      try { (void)::testing::internal::GetCapturedStdout(); }
      catch (...) { report_cleanup_failure(); }
      stdout_capturing = false;
    }
    if (stderr_capturing)
    {
      try { (void)::testing::internal::GetCapturedStderr(); }
      catch (...) { report_cleanup_failure(); }
      stderr_capturing = false;
    }
#endif
  }
  void restore_logger() noexcept
  {
    if (sink_installed)
    {
      try
      {
        if (!epee::log_space::log_singletone::remove_logger(offline_capture_sink_type))
          report_cleanup_failure();
      }
      catch (...) { report_cleanup_failure(); }
      sink_installed = false;
    }
    try { epee::log_space::log_singletone::get_set_log_detalisation_level(true, previous_level); }
    catch (...) { report_cleanup_failure(); }
  }
  std::shared_ptr<captured_output> output;
  int previous_level = 0;
  bool stdout_capturing = false, stderr_capturing = false;
  bool streams_empty = true, sink_installed = false;
  std::list<std::string> before_journal;
};
class counting_wallet_callback : public tools::i_wallet2_callback
{
public:
  void on_new_block(uint64_t, const currency::block&) override { ++calls; }
  void on_transfer2(const tools::wallet_public::wallet_transfer_info&,
    const std::list<tools::wallet_public::asset_balance_entry>&, uint64_t) override { ++calls; }
  void on_pos_block_found(const currency::block&) override { ++calls; }
  void on_sync_progress(const uint64_t&) override { ++calls; }
  void on_transfer_canceled(const tools::wallet_public::wallet_transfer_info&) override { ++calls; }
  void on_message(message_severity, const std::string&) override { ++calls; }
  void on_tor_status_change(const std::string&) override { ++calls; }
  void on_mw_get_wallets(std::vector<tools::wallet_public::wallet_entry_info>&) override { ++calls; }
  bool on_mw_select_wallet(uint64_t) override { ++calls; return false; }
  size_t calls = 0;
};

TEST(offline_wallet_session, fixture_password_material_matches_core_policy)
{
  // Constant inputs exercise the exact material type and formatter used above.
  // This test performs no entropy request, wallet generation or KDF work.
  offline_fixture_detail::password_material material{};
  const auto zero = epee::string_tools::pod_to_hex(material);
  EXPECT_EQ(zero.size(), 40u);
  EXPECT_TRUE(currency::validate_password(zero));
  material.fill(0xff);
  const auto full = epee::string_tools::pod_to_hex(material);
  EXPECT_EQ(full.size(), 40u);
  EXPECT_TRUE(currency::validate_password(full));
  EXPECT_FALSE(currency::validate_password(std::string(64, 'a')));
}

OFFLINE_WALLET_TEST(unexpected_helper_exceptions_have_fixed_gtest_diagnostics)
{
  struct private_exception : std::exception
  {
    explicit private_exception(size_t& calls) : calls(calls) {}
    const char* what() const noexcept override
    { ++calls; return "private-path-sentinel/private-password-sentinel"; }
    size_t& calls;
  };
  size_t what_calls = 0;
  for (unsigned variant = 0; variant != 2; ++variant)
  {
    ::testing::TestPartResultArray diagnostics;
    {
      ::testing::ScopedFakeTestPartResultReporter reporter(
        ::testing::ScopedFakeTestPartResultReporter::INTERCEPT_ONLY_CURRENT_THREAD, &diagnostics);
      material_free_test_boundary([&]()
      {
        if (variant == 0) throw private_exception(what_calls);
        throw fs::filesystem_error("private-password-sentinel",
          fs::path("/synthetic/private-path-sentinel"),
          boost::system::errc::make_error_code(boost::system::errc::permission_denied));
      });
    }
    ASSERT_EQ(1, diagnostics.size());
    const auto& diagnostic = diagnostics.GetTestPartResult(0);
    EXPECT_EQ(::testing::TestPartResult::kNonFatalFailure, diagnostic.type());
    const std::string message(diagnostic.message());
    EXPECT_TRUE(message.find(helper_failure_message) != std::string::npos);
    EXPECT_TRUE(message.find("private-path-sentinel") == std::string::npos);
    EXPECT_TRUE(message.find("private-password-sentinel") == std::string::npos);
  }
  EXPECT_EQ(0u, what_calls);
  size_t after_fatal = 0, after_nonfatal = 0;
  ::testing::TestPartResultArray normal_assertions;
  {
    ::testing::ScopedFakeTestPartResultReporter reporter(
      ::testing::ScopedFakeTestPartResultReporter::INTERCEPT_ONLY_CURRENT_THREAD, &normal_assertions);
    material_free_test_boundary([&]()
    {
      ASSERT_TRUE(false);
      ++after_fatal;
    });
    material_free_test_boundary([&]()
    {
      EXPECT_TRUE(false);
      ++after_nonfatal;
    });
  }
  ASSERT_EQ(2, normal_assertions.size());
  EXPECT_EQ(::testing::TestPartResult::kFatalFailure, normal_assertions.GetTestPartResult(0).type());
  EXPECT_EQ(::testing::TestPartResult::kNonFatalFailure, normal_assertions.GetTestPartResult(1).type());
  EXPECT_EQ(0u, after_fatal);
  EXPECT_EQ(1u, after_nonfatal);
}

#ifdef TESTNET
OFFLINE_WALLET_TEST(unsupported_profile_rejects_nonempty_material_before_io)
{
  fixture f(false); // directory and lease only: no wallet or fixture generation
  const auto c = access::observe();
  auto material = f.prepare(c);
  const auto lease = access::lease_identity(material);
  owner out;
  EXPECT_EQ(open_result::RecoveryRequired, open_prepared(material, out));
  EXPECT_FALSE(out);
  EXPECT_TRUE(access::holds(material));
  EXPECT_EQ(lease, access::lease_identity(material));
  EXPECT_EQ(0u, access::inspect(c).opens);
  EXPECT_EQ(0u, access::inspect(c).kdf);
  EXPECT_EQ(0u, access::inspect(c).destructions);
}

OFFLINE_WALLET_TEST(testnet_strict_loader_admits_only_its_own_formation)
{
  fixture f;
  // Current TESTNET/103 body loads; MAINNET/84 and unknown formations stop
  // in the decoded body before completion, without any store or sibling write.
  for (unsigned variant = 0; variant != 3; ++variant)
  {
    if (variant == 0) f.restore();
    else f.body(variant == 1 ? f.opposite_formation : f.wrong_formation);
    const auto tree = offline_fixture_detail::snapshot(f.root);
    const auto c = access::observe();
    EXPECT_EQ(variant == 0 ? open_result::Opened : open_result::RecoveryRequired,
      access::strict_load(f, c));
    EXPECT_EQ(1u, access::inspect(c).decodes);
    EXPECT_EQ(variant == 0 ? 1u : 0u, access::inspect(c).complete);
    EXPECT_EQ(0u, access::inspect(c).stores);
    EXPECT_EQ(0u, access::inspect(c).directories);
    EXPECT_TRUE(tree == offline_fixture_detail::snapshot(f.root));
  }
}
#else
OFFLINE_WALLET_TEST(exact_current_fixture_opens_and_info_is_cached)
{
  fixture f;
  const auto c = access::observe();
  auto material = f.prepare(c);
  const auto tree = offline_fixture_detail::snapshot(f.root);
  owner session;
  ASSERT_EQ(open_result::Opened, open_prepared(material, session));
  ASSERT_TRUE(static_cast<bool>(session));
  EXPECT_FALSE(access::holds(material));
  EXPECT_EQ(session_state::OpenUnchanged, session->state());
  const auto before = access::inspect(c);
  EXPECT_EQ(1u, before.opens); EXPECT_EQ(1u, before.closes);
  EXPECT_EQ(1u, before.parses); EXPECT_EQ(1u, before.kdf);
  EXPECT_EQ(1u, before.decrypt_attempts); EXPECT_EQ(1u, before.decodes);
  EXPECT_EQ(1u, before.complete); EXPECT_EQ(1u, before.finals);
  EXPECT_GT(before.eof, 0u);
  for (unsigned i = 0; i != 5; ++i)
  {
    const auto result = session->get_info();
    EXPECT_EQ(info_status::Available, result.status);
    EXPECT_TRUE(offline_fixture_detail::same_public(f.expected, result.info));
    EXPECT_EQ(connectivity::Offline, result.info.connection);
    EXPECT_EQ(synchronization::NoSynchronizationEvidence, result.info.sync);
  }
  EXPECT_TRUE(before.values() == access::inspect(c).values());
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  const auto released = access::inspect(c);
  EXPECT_EQ(1u, released.destructions); EXPECT_EQ(1u, released.lease_releases);
  EXPECT_EQ(1u, released.releases); EXPECT_LT(released.destroyed_at, released.released_at);
  EXPECT_TRUE(released.quiet_destruction);
  EXPECT_EQ(session_state::Released, session->state());
  EXPECT_EQ(info_status::Unavailable, session->get_info().status);
  EXPECT_EQ(release_result::AlreadyReleased, session->release_unchanged());
  EXPECT_TRUE(released.values() == access::inspect(c).values());
  session.reset();
  EXPECT_TRUE(released.values() == access::inspect(c).values());
  expect_no_side_effects(c);
  EXPECT_TRUE(tree == offline_fixture_detail::snapshot(f.root));
}

OFFLINE_WALLET_TEST(consumed_material_and_occupied_output_do_no_work)
{
  fixture f;
  const auto first_control = access::observe();
  auto first_material = f.prepare(first_control);
  owner first;
  ASSERT_EQ(open_result::Opened, open_prepared(first_material, first));
  owner empty;
  EXPECT_EQ(open_result::InvalidMaterial, open_prepared(first_material, empty));
  EXPECT_FALSE(empty);
  const auto c = access::observe();
  auto second_material = f.prepare(c);
  auto* const original_session = first.get();
  const auto original_lease = access::lease_identity(second_material);
  EXPECT_EQ(open_result::OccupiedOutput, open_prepared(second_material, first));
  EXPECT_EQ(original_session, first.get());
  EXPECT_EQ(original_lease, access::lease_identity(second_material));
  EXPECT_EQ(0u, access::inspect(c).opens); EXPECT_EQ(0u, access::inspect(c).destructions);
  EXPECT_EQ(0u, access::inspect(c).lease_releases);
  EXPECT_EQ(release_result::ReleasedUnchanged, first->release_unchanged());
}

OFFLINE_WALLET_TEST(moving_prepared_material_transfers_one_exact_lease)
{
  fixture f;
  const auto c = access::observe();
  auto original = f.prepare(c);
  const auto lease = access::lease_identity(original);
  prepared_wallet_material moved(std::move(original));
  EXPECT_FALSE(access::holds(original));
  EXPECT_EQ(lease, access::lease_identity(moved));
  EXPECT_EQ(0u, access::inspect(c).opens);
  EXPECT_EQ(0u, access::inspect(c).lease_releases);
  owner session;
  ASSERT_EQ(open_result::Opened, open_prepared(moved, session));
  EXPECT_FALSE(access::holds(moved));
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  EXPECT_EQ(1u, access::inspect(c).lease_releases);
}

OFFLINE_WALLET_TEST(acquisition_faults_keep_the_exact_caller_lease)
{
  fixture f;
  for (auto point : {access::checkpoint::BeforeAllocation, access::checkpoint::BeforeKdf,
    access::checkpoint::BeforeDecrypt,
    access::checkpoint::AfterKeys, access::checkpoint::DuringBody,
    access::checkpoint::AfterValidation, access::checkpoint::BeforeCommit})
  {
    SCOPED_TRACE(static_cast<int>(point));
    const auto c = access::observe(); access::fail(c, point);
    auto material = f.prepare(c);
    expect_failure(f, material, c, open_result::Unavailable);
    if (point == access::checkpoint::BeforeAllocation) EXPECT_EQ(0u, access::inspect(c).opens);
    else EXPECT_EQ(1u, access::inspect(c).destructions);
    if (point == access::checkpoint::BeforeKdf)
    {
      EXPECT_EQ(0u, access::inspect(c).kdf);
      EXPECT_EQ(0u, access::inspect(c).decrypt_attempts);
    }
    if (point == access::checkpoint::BeforeDecrypt)
    {
      EXPECT_EQ(1u, access::inspect(c).kdf);
      EXPECT_EQ(1u, access::inspect(c).decrypt_attempts);
    }
  }
}

OFFLINE_WALLET_TEST(unsupported_envelope_and_version_pairs_stop_before_kdf)
{
  fixture f;
  for (uint64_t signature : {uint64_t(WALLET_FILE_SIGNATURE_OLD), uint64_t(0)})
  {
    auto h = f.header; h.m_signature = signature; f.envelope(h, f.keys());
    const auto c = access::observe(); auto m = f.prepare(c);
    expect_failure(f, m, c, open_result::RecoveryRequired);
    EXPECT_EQ(0u, access::inspect(c).kdf);
  }
  for (uint32_t version : {1000u, 1001u, 1002u, 1003u, 1005u, 0xffffffffu})
  {
    auto h = f.header; h.m_ver = version; f.envelope(h, f.keys());
    const auto c = access::observe(); auto m = f.prepare(c);
    expect_failure(f, m, c, open_result::RecoveryRequired);
    EXPECT_EQ(0u, access::inspect(c).kdf); EXPECT_EQ(0u, access::inspect(c).decodes);
  }
  auto reserved = f.header; reserved.m_reserved = 1; f.envelope(reserved, f.keys());
  { const auto c = access::observe(); auto m = f.prepare(c);
    expect_failure(f, m, c, open_result::RecoveryRequired); EXPECT_EQ(0u, access::inspect(c).kdf); }
  const auto original_keys = f.keys();
  ASSERT_FALSE(original_keys.empty());
  ASSERT_EQ(4, static_cast<unsigned char>(original_keys.front()));
  for (unsigned char version : {0, 1, 2, 3, 5, 255})
  {
    auto keys = original_keys; keys.front() = static_cast<char>(version);
    f.envelope(f.header, keys);
    const auto c = access::observe(); auto m = f.prepare(c);
    expect_failure(f, m, c, open_result::RecoveryRequired);
    EXPECT_EQ(0u, access::inspect(c).kdf); EXPECT_EQ(0u, access::inspect(c).decrypt_attempts);
  }
  // A future byte alone must reject before version-dependent parsing.
  f.envelope(f.header, std::string(1, char(5)));
  { const auto c = access::observe(); auto m = f.prepare(c);
    expect_failure(f, m, c, open_result::RecoveryRequired); EXPECT_EQ(0u, access::inspect(c).kdf); }
}

OFFLINE_WALLET_TEST(noncurrent_kdf_profile_is_not_executed)
{
  fixture f;
  for (unsigned variant = 0; variant != 5; ++variant)
  {
    auto key_data = f.key_data;
    switch (variant)
    {
      case 0: key_data.kdf_algo = 0; break;
      case 1: key_data.kdf_algo = 255; break;
      case 2: --key_data.kdf_N_log2; break;
      case 3: --key_data.kdf_phase2_log2_reduction; break;
      case 4: key_data.kdf_salt.pop_back(); break;
    }
    f.key_profile(key_data);
    const auto c = access::observe(); auto material = f.prepare(c);
    expect_failure(f, material, c, open_result::RecoveryRequired);
    EXPECT_EQ(0u, access::inspect(c).kdf); EXPECT_EQ(0u, access::inspect(c).decrypt_attempts);
  }
}

OFFLINE_WALLET_TEST(malformed_header_keys_and_truncation_are_invalid_files)
{
  fixture f;
  std::vector<std::string> corrupt;
  corrupt.push_back("");
  corrupt.push_back(f.baseline.substr(0, sizeof(f.header) - 1));
  corrupt.push_back(f.baseline.substr(0, sizeof(f.header) + f.header.m_cb_keys - 1));
  for (uint16_t length : {uint16_t(0), uint16_t(WALLET_FILE_MAX_KEYS_SIZE + 1)})
  {
    auto h = f.header; h.m_cb_keys = length;
    corrupt.push_back(std::string(reinterpret_cast<const char*>(&h), sizeof(h)) + f.baseline.substr(sizeof(h)));
  }
  for (const auto& bytes : corrupt)
  {
    offline_fixture_detail::write(f.path, bytes);
    const auto c = access::observe(); auto material = f.prepare(c);
    expect_failure(f, material, c, open_result::InvalidFile); EXPECT_EQ(0u, access::inspect(c).kdf);
  }
  f.envelope(f.header, f.keys() + std::string(1, '\0'));
  const auto c = access::observe(); auto material = f.prepare(c);
  expect_failure(f, material, c, open_result::InvalidFile); EXPECT_EQ(0u, access::inspect(c).kdf);
}

OFFLINE_WALLET_TEST(malformed_internal_key_lengths_reject_before_kdf)
{
  fixture f;
  const auto original = f.keys();
  const auto encode_length = [](size_t value)
  {
    std::string bytes;
    do
    {
      unsigned char byte = static_cast<unsigned char>(value & 0x7f);
      value >>= 7;
      if (value) byte |= 0x80;
      bytes.push_back(static_cast<char>(byte));
    } while (value);
    return bytes;
  };
  struct length_span { size_t offset, width; };
  // Rebuild the complete exact-current key section independently of its parser.
  // The equality precondition makes both targeted offsets format-aware.
  std::string canonical(1, char(4));
  canonical.append(reinterpret_cast<const char*>(&f.key_data.iv), sizeof(f.key_data.iv));
  const auto account_length = encode_length(f.key_data.account_data.size());
  const length_span account{canonical.size(), account_length.size()};
  canonical += account_length;
  canonical += f.key_data.account_data;
  canonical.push_back(static_cast<char>(f.key_data.kdf_algo));
  canonical.push_back(static_cast<char>(f.key_data.kdf_N_log2));
  const auto salt_length = encode_length(f.key_data.kdf_salt.size());
  const length_span salt{canonical.size(), salt_length.size()};
  canonical += salt_length;
  canonical += f.key_data.kdf_salt;
  canonical.push_back(static_cast<char>(f.key_data.kdf_phase2_log2_reduction));
  ASSERT_TRUE(canonical == original); // never dump encrypted material on failure

  for (const auto span : {account, salt})
  {
    auto overlong = original.substr(span.offset, span.width);
    overlong.back() = static_cast<char>(static_cast<unsigned char>(overlong.back()) | 0x80);
    overlong.push_back('\0'); // same decoded value, explicitly noncanonical
    const std::string overflow((std::numeric_limits<size_t>::digits + 6) / 7 + 1, char(0xff));
    auto truncated = original.substr(span.offset, span.width);
    truncated.back() = static_cast<char>(static_cast<unsigned char>(truncated.back()) | 0x80);
    const std::vector<std::string> malformed{
      original.substr(0, span.offset) + overlong + original.substr(span.offset + span.width),
      original.substr(0, span.offset) + overflow + char(0x7f) + original.substr(span.offset + span.width),
      original.substr(0, span.offset) + truncated};
    for (const auto& bytes : malformed)
    {
      f.envelope(f.header, bytes); // adjusts outer key size, leaves body unchanged
      const auto c = access::observe();
      auto material = f.prepare(c);
      expect_failure(f, material, c, open_result::InvalidFile);
      EXPECT_EQ(1u, access::inspect(c).parses);
      EXPECT_EQ(0u, access::inspect(c).kdf);
      EXPECT_EQ(0u, access::inspect(c).decrypt_attempts);
      EXPECT_EQ(0u, access::inspect(c).decodes);
    }
  }
  f.restore();
  const auto c = access::observe();
  auto material = f.prepare(c);
  owner session;
  ASSERT_EQ(open_result::Opened, open_prepared(material, session));
  ASSERT_TRUE(static_cast<bool>(session));
  EXPECT_TRUE(offline_fixture_detail::same_public(f.expected, session->get_info().info));
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

OFFLINE_WALLET_TEST(decoded_archive_and_formation_mismatches_require_recovery)
{
  fixture f;
  for (const auto* body : {&f.old_archive, &f.future_archive, &f.wrong_formation, &f.opposite_formation})
  {
    f.body(*body);
    const auto c = access::observe(); auto material = f.prepare(c);
    expect_failure(f, material, c, open_result::RecoveryRequired);
    EXPECT_EQ(1u, access::inspect(c).decodes);
    EXPECT_EQ(0u, access::inspect(c).complete);
    EXPECT_EQ(0u, access::inspect(c).finals);
  }
}

OFFLINE_WALLET_TEST(decoded_library_header_distinguishes_unsupported_version_from_malformed_data)
{
  fixture f;
  // The pinned eos format fixes its archive version to 100 independently of
  // Boost's linked library version. init() writes raw magic, then the integer
  // byte width and its little-endian value: 7f 01 64.
  ASSERT_EQ(0x7f, static_cast<unsigned char>(eos::magic_byte));
  ASSERT_EQ(100u, static_cast<unsigned int>(eos::archive_version));
  const std::string current_header{char(0x7f), char(0x01), char(0x64)};
  std::ostringstream header_output(std::ios::binary);
  { eos::portable_oarchive header_archive(header_output); }
  ASSERT_TRUE(header_output.good());
  ASSERT_TRUE(header_output.str() == current_header);
  ASSERT_GT(f.decoded_body.size(), current_header.size());
  ASSERT_TRUE(f.decoded_body.compare(0, current_header.size(), current_header) == 0);

  for (unsigned variant = 0; variant != 3; ++variant)
  {
    SCOPED_TRACE(variant);
    auto body = f.decoded_body;
    if (variant == 0) body[2] = char(101); // valid integer encoding, unsupported eos version
    else if (variant == 1) body[0] = char(0x7e); // invalid magic, supported version unchanged
    else body.resize(current_header.size() - 1); // magic and width, but the version byte is missing
    f.body(body); // re-encrypt the actual decoded header, preserving the key section
    const auto c = access::observe();
    auto material = f.prepare(c);
    expect_failure(f, material, c,
      variant == 0 ? open_result::RecoveryRequired : open_result::InvalidFile);
    const auto n = access::inspect(c);
    EXPECT_EQ(1u, n.acquired);
    EXPECT_EQ(1u, n.kdf);
    EXPECT_EQ(1u, n.decrypt_attempts);
    EXPECT_EQ(1u, n.decodes);
    EXPECT_EQ(0u, n.complete);
    EXPECT_EQ(0u, n.finals);
  }

  // The unchanged header remains accepted after all three targeted mutations.
  f.restore();
  const auto c = access::observe();
  auto material = f.prepare(c);
  owner session;
  ASSERT_EQ(open_result::Opened, open_prepared(material, session));
  ASSERT_TRUE(static_cast<bool>(session));
  EXPECT_TRUE(offline_fixture_detail::same_public(f.expected, session->get_info().info));
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

OFFLINE_WALLET_TEST(decoded_trailing_data_second_object_and_terminal_truncation_reject)
{
  fixture f;
  ASSERT_GT(f.decoded_body.size(), 8u);
  const std::vector<std::string> mutations{
    f.decoded_body + std::string(1, '\0'),
    f.decoded_body + std::string(8193, 'x'),
    f.decoded_body + f.decoded_body,
    f.decoded_body.substr(0, f.decoded_body.size() - 1),
    f.decoded_body.substr(0, f.decoded_body.size() - 8)};
  size_t index = 0;
  for (const auto& body : mutations)
  {
    f.body(body);
    const auto c = access::observe(); auto material = f.prepare(c);
    expect_failure(f, material, c, open_result::InvalidFile);
    EXPECT_EQ(1u, access::inspect(c).decodes);
    if (index < 3)
    {
      EXPECT_EQ(1u, access::inspect(c).complete);
      EXPECT_EQ(0u, access::inspect(c).finals);
    }
    // A single appended byte in this small fixture has already reached raw
    // EOF before decoded exhaustion; the retained decoder byte still rejects.
    if (index == 0)
    {
      EXPECT_GT(access::inspect(c).eof, 0u);
    }
    ++index;
  }
}

OFFLINE_WALLET_TEST(actual_source_read_faults_and_error_instead_of_eof_reject)
{
  fixture f;
  for (auto stage : {access::read_stage::Header, access::read_stage::Keys, access::read_stage::Body})
    for (auto failure : {access::read_fault::ReturnedError, access::read_fault::ShortRead, access::read_fault::Throw})
    {
      SCOPED_TRACE(static_cast<int>(stage));
      SCOPED_TRACE(static_cast<int>(failure));
      const auto c = access::observe(); access::fail_read(c, stage, failure);
      auto material = f.prepare(c);
      expect_failure(f, material, c, open_result::InvalidFile);
      EXPECT_TRUE(access::inspect(c).read_fault_fired);
    }
  const auto c = access::observe();
  access::fail_read(c, access::read_stage::Body, access::read_fault::ErrorAtEof);
  auto material = f.prepare(c);
  expect_failure(f, material, c, open_result::InvalidFile);
  EXPECT_TRUE(access::inspect(c).read_fault_fired);
  EXPECT_EQ(1u, access::inspect(c).decodes);
}

OFFLINE_WALLET_TEST(complete_object_followed_by_terminal_read_failure_never_publishes)
{
  fixture f;
  for (auto failure : {access::read_fault::ReturnedError, access::read_fault::ShortRead,
    access::read_fault::Throw, access::read_fault::ErrorAtEof})
  {
    const auto c = access::observe();
    access::fail_read(c, access::read_stage::Final, failure);
    auto material = f.prepare(c);
    expect_failure(f, material, c, open_result::InvalidFile);
    EXPECT_EQ(1u, access::inspect(c).complete);
    EXPECT_EQ(1u, access::inspect(c).finals);
    EXPECT_TRUE(access::inspect(c).read_fault_fired);
  }
}

OFFLINE_WALLET_TEST(decoder_stream_states_and_close_failure_cannot_publish)
{
  fixture f;
  for (auto failure : {access::decoder_fault::Failbit, access::decoder_fault::Badbit, access::decoder_fault::Throw})
  {
    const auto c = access::observe(); access::fail_decoder(c, failure);
    auto material = f.prepare(c);
    expect_failure(f, material, c, open_result::InvalidFile);
  }
  // Synthetic close-return observation at the real close boundary. This is
  // classification/publication coverage, not an induced kernel fclose failure.
  const auto c = access::observe(); access::fail_close(c);
  auto material = f.prepare(c);
  expect_failure(f, material, c, open_result::InvalidFile);
  EXPECT_EQ(1u, access::inspect(c).decodes);
}

OFFLINE_WALLET_TEST(completed_object_with_decoder_failure_cannot_publish)
{
  fixture f;
  for (auto failure : {access::decoder_fault::Failbit, access::decoder_fault::Badbit, access::decoder_fault::Throw})
  {
    const auto c = access::observe();
    access::fail_decoder(c, failure, access::decoder_stage::AfterObject);
    auto material = f.prepare(c);
    expect_failure(f, material, c, open_result::InvalidFile);
    EXPECT_EQ(1u, access::inspect(c).decodes);
    EXPECT_EQ(1u, access::inspect(c).complete);
    EXPECT_EQ(0u, access::inspect(c).finals);
  }
}

OFFLINE_WALLET_TEST(wrong_password_and_independent_identity_mismatches_are_typed)
{
  fixture f;
  { const auto c = access::observe(); auto m = access::wrong_password(f, c);
    expect_failure(f, m, c, open_result::WrongPassword); }
  for (unsigned variant = 0; variant != 4; ++variant)
  {
    auto identity = f.expected;
    if (variant == 0) reinterpret_cast<unsigned char*>(&identity.spend_public_key)[0] ^= 1;
    if (variant == 1) reinterpret_cast<unsigned char*>(&identity.view_public_key)[0] ^= 1;
    if (variant == 2) identity.flags ^= 1;
    if (variant == 3) reinterpret_cast<unsigned char*>(&identity.stored_genesis)[0] ^= 1;
    const auto c = access::observe(); auto m = access::mismatched(f, identity, c);
    expect_failure(f, m, c, variant == 3 ? open_result::GenesisMismatch : open_result::IdentityMismatch);
  }
}

OFFLINE_WALLET_TEST(ordinary_and_auditable_watch_only_stop_before_sidecars)
{
  fixture f;
  for (bool auditable : {false, true})
  {
    f.watch_only(auditable);
    const auto c = access::observe(); auto material = f.prepare(c);
    expect_failure(f, material, c, open_result::UnsupportedCapability);
    EXPECT_EQ(0u, access::inspect(c).sidecars); EXPECT_EQ(0u, access::inspect(c).decodes);
  }
}

OFFLINE_WALLET_TEST(missing_parent_and_symlinks_never_create_or_follow_files)
{
  fixture f;
  const auto missing = f.root / "missing-parent" / "missing.wallet";
  const auto c = access::observe(); auto material = access::alternate_path(f, missing, c);
  expect_failure(f, material, c, open_result::NotFound);
  EXPECT_FALSE(fs::exists(missing.parent_path()));
  const auto directory = f.root / "not-a-wallet";
  fs::create_directory(directory);
  const auto directory_leaf_control = access::observe();
  auto directory_leaf = access::alternate_path(f, directory, directory_leaf_control);
  expect_failure(f, directory_leaf, directory_leaf_control, open_result::InvalidMaterial);
  EXPECT_EQ(0u, access::inspect(directory_leaf_control).opens);
#ifndef _WIN32
  const auto link = f.root / "linked.wallet";
  fs::create_symlink(f.path.filename(), link);
  const auto linked_control = access::observe();
  auto linked = access::alternate_path(f, link, linked_control);
  expect_failure(f, linked, linked_control, open_result::InvalidMaterial);
  EXPECT_EQ(0u, access::inspect(linked_control).opens);
  const auto directory_link = f.root / "linked-directory";
  fs::create_directory_symlink(f.root, directory_link);
  const auto directory_control = access::observe();
  auto through_directory = access::alternate_path(f, directory_link / f.path.filename(), directory_control);
  expect_failure(f, through_directory, directory_control, open_result::InvalidMaterial);
  EXPECT_EQ(0u, access::inspect(directory_control).opens);
  const auto hard_link = f.root / "hard-linked.wallet";
  fs::create_hard_link(f.path, hard_link);
  const auto alias_control = access::observe();
  auto alias = access::alternate_path(f, hard_link, alias_control);
  expect_failure(f, alias, alias_control, open_result::InvalidMaterial);
  EXPECT_EQ(0u, access::inspect(alias_control).opens);
#endif
}

OFFLINE_WALLET_TEST(offline_guards_survive_base_reconstruction)
{
  const auto c = access::observe();
  epee::log_space::scoped_log_silence silence;
  auto wallet = access::offline_wallet(c);
  ASSERT_TRUE(access::voting_path_empty(*wallet));
  for (unsigned i = 0; i != 2; ++i)
  {
    EXPECT_TRUE(access::offline_mode(*wallet));
    EXPECT_TRUE(access::guard_connections(*wallet));
    EXPECT_TRUE(access::guard_whitelist_preserves(*wallet));
    access::reset_base(*wallet);
  }
  const auto n = access::inspect(c);
  EXPECT_EQ(2u, n.guard_init); EXPECT_EQ(2u, n.guard_connection);
  EXPECT_EQ(2u, n.guard_proxy); EXPECT_EQ(2u, n.guard_whitelist);
  expect_no_side_effects(c);
  wallet.reset();
  EXPECT_TRUE(access::inspect(c).quiet_destruction);
}

OFFLINE_WALLET_TEST(deny_proxy_overrides_both_address_resolution_overloads)
{
  const auto c = access::observe();
  epee::log_space::scoped_log_silence silence;
  auto wallet = access::offline_wallet(c);
  EXPECT_TRUE(access::deny_resolvers(*wallet));
  EXPECT_EQ(2u, access::inspect(c).proxy);
  EXPECT_EQ(0u, access::inspect(c).opens);
  wallet.reset();
}

OFFLINE_WALLET_TEST(unique_owner_reset_and_overwrite_are_abandonment)
{
  fixture f;
  const auto a = access::observe(), b = access::observe();
  auto ma = f.prepare(a), mb = f.prepare(b);
  owner first, second;
  ASSERT_EQ(open_result::Opened, open_prepared(ma, first));
  ASSERT_EQ(open_result::Opened, open_prepared(mb, second));
  const auto tree = offline_fixture_detail::snapshot(f.root);
  first = std::move(second);
  auto n = access::inspect(a);
  EXPECT_EQ(1u, n.abandonments); EXPECT_EQ(0u, n.releases);
  EXPECT_EQ(1u, n.destructions); EXPECT_EQ(1u, n.lease_releases);
  EXPECT_LT(n.destroyed_at, n.released_at);
  first.reset();
  n = access::inspect(b);
  EXPECT_EQ(1u, n.abandonments); EXPECT_EQ(0u, n.releases);
  EXPECT_EQ(1u, n.destructions); EXPECT_EQ(1u, n.lease_releases);
  EXPECT_LT(n.destroyed_at, n.released_at);
  EXPECT_TRUE(tree == offline_fixture_detail::snapshot(f.root));
}

OFFLINE_WALLET_TEST(faulted_session_refuses_release_until_owner_abandons)
{
  fixture f;
  const auto c = access::observe();
  auto material = f.prepare(c);
  owner session;
  ASSERT_EQ(open_result::Opened, open_prepared(material, session));
  access::fault(*session);
  const auto before = access::inspect(c);
  EXPECT_EQ(info_status::Unavailable, session->get_info().status);
  EXPECT_EQ(release_result::Unavailable, session->release_unchanged());
  EXPECT_EQ(session_state::Faulted, session->state());
  EXPECT_TRUE(before.values() == access::inspect(c).values());
  session.reset();
  EXPECT_EQ(1u, access::inspect(c).abandonments);
  EXPECT_EQ(0u, access::inspect(c).releases);
  EXPECT_EQ(1u, access::inspect(c).lease_releases);
}

OFFLINE_WALLET_TEST(legacy_load_and_recovery_remain_separate_from_strict_loading)
{
  fixture f;
  const auto normal = access::observe();
  EXPECT_TRUE(access::legacy_load(f, normal, false));
  EXPECT_EQ(1u, access::inspect(normal).directories);
  EXPECT_EQ(1u, access::inspect(normal).votes);
  EXPECT_EQ(0u, access::inspect(normal).guard_init);
  auto header = f.header;
  header.m_ver = 0xffffffffu;
  f.envelope(header, f.keys());
  const auto recovery = access::observe();
  const auto before = offline_fixture_detail::snapshot(f.root);
  EXPECT_TRUE(access::legacy_load(f, recovery, true));
  EXPECT_TRUE(before == offline_fixture_detail::snapshot(f.root));
  EXPECT_EQ(1u, access::inspect(recovery).votes);
  const auto strict = access::observe(); auto material = f.prepare(strict);
  expect_failure(f, material, strict, open_result::RecoveryRequired);
  EXPECT_EQ(0u, access::inspect(strict).votes);
}

OFFLINE_WALLET_TEST(open_errors_info_release_and_abandonment_emit_no_material)
{
  offline_output_capture captured;
  bool outcomes = true;
  size_t callbacks = 0;
  {
    fixture f; // includes offline construction/generation under its own silence
    // No callback exists during construction/open/error by API construction.
    // This probe registers through the real callback() API only after open,
    // and covers cached info, unchanged release and abandonment.
    auto callback = std::make_shared<counting_wallet_callback>();
    const auto c = access::observe();
    auto material = f.prepare(c);
    owner session;
    outcomes = open_prepared(material, session) == open_result::Opened;
    if (session)
    {
      const bool registered = access::attach_callback(*session, callback);
      outcomes = registered && outcomes;
      const bool info_available = session->get_info().status == info_status::Available;
      outcomes = info_available && outcomes;
      const bool released = session->release_unchanged() == release_result::ReleasedUnchanged;
      outcomes = released && outcomes;
      session.reset();
    }
    const auto wrong = access::observe();
    auto bad = access::wrong_password(f, wrong);
    const bool wrong_rejected = open_prepared(bad, session) == open_result::WrongPassword && !session;
    outcomes = wrong_rejected && outcomes;
    session.reset();
    const auto failing = access::observe();
    access::fail(failing, access::checkpoint::BeforeCommit);
    auto failed = f.prepare(failing);
    const bool failed_rejected = open_prepared(failed, session) == open_result::Unavailable && !session;
    outcomes = failed_rejected && outcomes;
    session.reset();
    const auto abandoned = access::observe();
    auto last = f.prepare(abandoned);
    const bool last_opened = open_prepared(last, session) == open_result::Opened;
    outcomes = last_opened && outcomes;
    if (session)
    {
      const bool registered = access::attach_callback(*session, callback);
      outcomes = registered && outcomes;
    }
    session.reset();
    callbacks = callback->calls;
  }
  const bool quiet = captured.finish();
  EXPECT_TRUE(outcomes);
  EXPECT_TRUE(quiet); // never print captured material on a regression
  EXPECT_EQ(0u, callbacks);
#if !GTEST_HAS_STREAM_REDIRECTION
  ADD_FAILURE() << "stdout/stderr capture is required for the offline execution gate";
#endif
}
#endif
#undef OFFLINE_WALLET_TEST
} // anonymous namespace

TEST(offline_wallet_session, read_only_scanner_cannot_upgrade_offline_fixture)
{
  const auto control = tools::offline::fixture_access::observe();
  auto wallet = tools::offline::fixture_access::offline_wallet(control);
  wallet->set_minimum_height(0);
  auto transport = std::make_shared<tools::i_core_proxy>();
  tools::read_only::block_sync scanner(*wallet, wallet->m_chain.get_genesis(), transport, {1, 1});
  std::atomic<bool> stop{false};
  EXPECT_EQ(tools::read_only::step_status::UnsupportedCapability, scanner.step(stop).status);
  EXPECT_FALSE(scanner.persistence_required());
}
