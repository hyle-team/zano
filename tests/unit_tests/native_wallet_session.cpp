// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
// Source-only integration candidate. Execution generates disposable wallets and
// invokes the pinned KDF; it requires the separate reviewed runtime gate.
#include "gtest/gtest.h"
#include "wallet/native_wallet_session.h"
#include "wallet/i_core_rpc_proxy.h"
#include "wallet/wallet2.h"
#include "currency_core/account.h"
#include "common/base58.h"
#include "currency_core/currency_config.h"
#include "currency_core/currency_format_utils.h"
#include <algorithm>
#include <array>
#include <limits>
#include <set>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <boost/filesystem.hpp>
#if defined(__linux__)
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace tools { namespace native_session
{
  // Test-only observation/fault seam. Defined only here; no production caller.
  struct checkpoint_test_access
  {
    static uint64_t local_blocks(const file_session& session)
    {
      return session.m_wallet ? session.m_wallet->get_blockchain_current_size() : 0;
    }
    static void limit_image(file_session& session, std::size_t limit) { session.m_image_limit = limit; }
    // In-memory drift of the loaded wallet's genesis after verification.
    static void forge_genesis(file_session& session)
    {
      auto genesis = session.m_wallet->m_chain.get_genesis();
      reinterpret_cast<unsigned char*>(&genesis)[0] ^= 1;
      session.m_wallet->m_chain.set_genesis(genesis);
    }
    // In-memory local records/clock for snapshot regressions. Never persisted
    // by these tests; no production caller.
    static tools::wallet2& wallet(file_session& session) { return *session.m_wallet; }
    static uint64_t generation(const file_session& session) { return session.m_generation; }
    static uint64_t revision(const file_session& session) { return session.m_revision; }
    static void exhaust_revision(file_session& session) { session.m_revision_exhausted = true; }
    static void set_revision(file_session& session, uint64_t value) { session.m_revision = value; }
  };
}}

namespace native_session_test
{
namespace fs = boost::filesystem;
using namespace tools::native_session;
using owner = std::unique_ptr<file_session>;
using lease_owner = std::unique_ptr<material_lease>;

void require(bool condition)
{
  if (!condition) throw std::runtime_error("native disposable fixture failed");
}

std::string read(const fs::path& path)
{
  std::ifstream input(path.string(), std::ios::binary);
  require(input.is_open());
  std::string bytes((std::istreambuf_iterator<char>(input)), {});
  require(!input.bad());
  return bytes;
}

void write(const fs::path& path, const std::string& bytes)
{
  std::ofstream output(path.string(), std::ios::binary | std::ios::trunc);
  require(output.is_open());
  output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  output.flush();
  require(output.good());
  output.close();
  require(!output.fail());
}

using tree_image = std::map<std::string, std::string>;
tree_image snapshot(const fs::path& root)
{
  tree_image result;
  for (fs::recursive_directory_iterator it(root), end; it != end; ++it)
  {
    const auto path = it->path();
    const auto name = path.lexically_relative(root).generic_string();
    if (fs::is_symlink(path)) result.emplace(name, "link:" + fs::read_symlink(path).generic_string());
    else if (fs::is_directory(path)) result.emplace(name + "/", "");
    else
    {
      require(fs::is_regular_file(path));
      result.emplace(name, read(path));
    }
  }
  return result;
}

// This test provider models an already-held storage exclusion. It is not a
// platform lock or production path-confinement implementation.
struct exclusion
{
  bool held = false;
};
// The test provider's checkpoint replacement, following the provider
// obligations: exclusive no-follow temporary beside the target, checked
// complete write, fsync, checked close, ONE rename over the target, directory
// fsync. Any failure is reported as uncertain; nothing is ever deleted.
bool replace_target(const fs::path& target, const char* data, size_t size)
{
#if defined(__linux__)
  const std::string leaf = target.filename().string();
  const std::string temporary = leaf + ".checkpoint-tmp";
  const int directory = ::open(target.parent_path().c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
  if (directory < 0) return false;
  bool ok = false;
  const int fd = ::openat(directory, temporary.c_str(),
    O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, S_IRUSR | S_IWUSR);
  if (fd >= 0)
  {
    ok = true;
    size_t done = 0;
    while (ok && done < size)
    {
      const ssize_t wrote = ::write(fd, data + done, size - done);
      if (wrote < 0 && errno == EINTR) continue;
      if (wrote <= 0) ok = false;
      else done += static_cast<size_t>(wrote);
    }
    ok = ok && ::fsync(fd) == 0;
    ok = ::close(fd) == 0 && ok; // Closed exactly once, checked.
    ok = ok && ::renameat(directory, temporary.c_str(), directory, leaf.c_str()) == 0;
    ok = ok && ::fsync(directory) == 0;
  }
  ::close(directory);
  return ok;
#else
  (void)target;
  (void)data;
  (void)size;
  return false;
#endif
}

// Registration boundary. Only the Linux fixture above can really replace the
// target, so Saved/ReleasedSaved results and strict reopen of a replaced file
// are expected only there. Elsewhere replace_target always fails and the
// provider (correctly) reports Unknown; those cases are not registered or their
// success tails are skipped. Refusal, fault, reentrancy and state coverage that
// never needs a successful write stays registered on every platform. This
// fixture is not a production writer for any platform.
#if defined(__linux__)
#define NATIVE_SESSION_REPLACEMENT_FIXTURE 1
#else
#define NATIVE_SESSION_REPLACEMENT_FIXTURE 0
#endif
constexpr bool replacement_supported = NATIVE_SESSION_REPLACEMENT_FIXTURE != 0;

enum class writer_mode { Commit, Reject, Throw, UnknownAfterReplace };

struct provider_state
{
  bool authorized = true;
  bool revoke_in_material = false;
  bool throw_in_material = false;
  size_t material_reads = 0;
  size_t authorization_reads = 0;
  size_t revoke_at_authorization_read = 0;
  size_t releases = 0;
  bool material_calls_silenced = true;
  bool authorization_calls_silenced = true;
  bool release_silenced = false;
  std::shared_ptr<exclusion> storage;
  // Checkpoint capability (default: unsupported, like existing providers).
  bool checkpoint_capable = false;
  writer_mode writer = writer_mode::Commit;
  bool drift_material = false;       // Violates material stability.
  size_t checkpoints = 0;
  bool checkpoint_authorized = false; // Provider authorization at the last write.
  bool checkpoint_silenced = true;
  // Runs inside replace_checkpoint, before the writer's outcome, to observe
  // reentrant calls while the session's checkpoint is in progress.
  std::function<void()> on_checkpoint;
  // Request-mutation seams for the payment callback capture regressions.
  std::function<void()> on_authorization;
  std::function<void()> on_material;
};

bool same_identity(const public_identity& actual, const public_identity& expected);

class provider_lease final : public material_lease
{
public:
  provider_lease(open_material value, std::shared_ptr<provider_state> state)
    : m_value(std::move(value)), m_drifted(m_value), m_state(std::move(state))
  {
    require(m_state && m_state->storage && !m_state->storage->held);
    reinterpret_cast<unsigned char*>(&m_drifted.expected.spend_public_key)[0] ^= 1;
    m_state->storage->held = true;
  }
  bool checkpoint_supported() const noexcept override { return m_state->checkpoint_capable; }
  ~provider_lease() noexcept override
  {
    m_state->release_silenced = epee::log_space::is_thread_log_silenced();
    m_state->storage->held = false;
    ++m_state->releases;
  }
  bool authorization_current() const noexcept override
  {
    ++m_state->authorization_reads;
    m_state->authorization_calls_silenced = m_state->authorization_calls_silenced &&
      epee::log_space::is_thread_log_silenced();
    if (m_state->revoke_at_authorization_read &&
        m_state->authorization_reads >= m_state->revoke_at_authorization_read)
      m_state->authorized = false;
    if (m_state->on_authorization) m_state->on_authorization();
    return m_state->authorized;
  }
protected:
  const open_material& material() const override
  {
    ++m_state->material_reads;
    m_state->material_calls_silenced = m_state->material_calls_silenced &&
      epee::log_space::is_thread_log_silenced();
    if (m_state->on_material) m_state->on_material();
    if (m_state->throw_in_material)
      throw std::runtime_error("native provider material unavailable");
    // The first authorization sample succeeds. Revocation is visible after
    // material retrieval, so publication must sample authorization again.
    if (m_state->revoke_in_material) m_state->authorized = false;
    return m_state->drift_material ? m_drifted : m_value;
  }
  commit_status replace_checkpoint(const public_identity& bound, const char* image, size_t size) override
  {
    ++m_state->checkpoints;
    m_state->checkpoint_authorized = m_state->authorized;
    m_state->checkpoint_silenced = m_state->checkpoint_silenced &&
      epee::log_space::is_thread_log_silenced();
    if (m_state->on_checkpoint) m_state->on_checkpoint();
    // Only this lease's committed identity, under its still-held exclusion.
    if (!m_state->checkpoint_capable || !m_state->storage->held || !same_identity(bound, m_value.expected))
      return commit_status::Rejected;
    switch (m_state->writer)
    {
      case writer_mode::Reject: return commit_status::Rejected; // Refused before any byte.
      case writer_mode::Throw: throw std::runtime_error("synthetic checkpoint writer failure");
      default: break;
    }
    if (!replace_target(fs::path(m_value.path), image, size)) return commit_status::Unknown;
    // UnknownAfterReplace models a replaced target whose directory sync
    // could not be confirmed.
    return m_state->writer == writer_mode::Commit ? commit_status::Committed : commit_status::Unknown;
  }
private:
  open_material m_value;
  open_material m_drifted;
  std::shared_ptr<provider_state> m_state;
};

// Genesis the disposable FILE stores. The provider expectation is always the
// compiled descriptor, committed independently of the file being reopened.
enum class file_genesis { Compiled, Alternate, LegacyDefault };

class fixture final
{
public:
  explicit fixture(bool generate = true, file_genesis stored = file_genesis::Compiled)
  {
    try
    {
      parent = fs::canonical(fs::temp_directory_path());
      const auto requested = parent / fs::unique_path("zano-native-session-%%%%-%%%%-%%%%");
      require(fs::create_directory(requested));
      root = fs::canonical(requested);
      require(root == requested && root.parent_path() == parent);
      path = root / "disposable.wallet";
      input.path = path.wstring();
      input.expected.stored_genesis = build_profile().genesis;
      if (generate)
      {
        epee::log_space::scoped_log_silence quiet;
        require(build_profile().status == profile_status::Available);
        input.password = epee::string_tools::pod_to_hex(crypto::rand<std::array<uint8_t, 20>>());
        require(currency::validate_password(input.password));
        tools::wallet2 setup;
        setup.set_votes_config_path((root / "voting-sentinel.json").string());
        // generate() stores the shared gdefault_genesis (the MAINNET literal)
        // on every build. Initialize the intended compiled network explicitly.
        setup.generate(path.wstring(), input.password, false);
        setup.set_minimum_height(0); // Explicit legacy fixture; fresh birthday cases set UNSET themselves.
        const auto& address = setup.get_account().get_public_address();
        input.expected.spend_public_key = address.spend_public_key;
        input.expected.view_public_key = address.view_public_key;
        input.expected.flags = address.flags;
        expected_address = setup.get_account().get_public_address_str();
        require(!expected_address.empty());
        if (stored != file_genesis::LegacyDefault)
        {
          auto chosen = input.expected.stored_genesis;
          if (stored == file_genesis::Alternate) reinterpret_cast<unsigned char*>(&chosen)[0] ^= 1;
          setup.set_genesis(chosen);
          setup.store();
        }
        if (stored == file_genesis::LegacyDefault)
          require(setup.m_chain.get_genesis() == currency::gdefault_genesis);
        else
          require((setup.m_chain.get_genesis() == input.expected.stored_genesis) ==
            (stored == file_genesis::Compiled));
        baseline = read(path);
        require(baseline.size() > sizeof(header));
        std::memcpy(&header, baseline.data(), sizeof(header));
        require(header.m_cb_keys > 0 && baseline.size() > sizeof(header) + header.m_cb_keys);
      }
      write(root / "sibling-sentinel", "unrelated disposable sibling");
      write(root / "disposable.outkey2ki", "corrupt disposable sidecar sentinel");
      write(root / "voting-sentinel.json", "{invalid disposable vote sentinel");
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

  std::shared_ptr<provider_state> control(const open_material& material)
  {
    auto& slot = exclusions[material.path];
    if (!slot) slot = std::make_shared<exclusion>();
    auto state = std::make_shared<provider_state>();
    state->storage = slot;
    return state;
  }
  lease_owner lease(const open_material& material, const std::shared_ptr<provider_state>& state)
  {
    return lease_owner(new provider_lease(material, state));
  }
  void restore() { write(path, baseline); }
  void replace_header(tools::wallet_file_binary_header replacement)
  {
    std::string bytes = baseline;
    std::memcpy(&bytes[0], &replacement, sizeof(replacement));
    write(path, bytes);
  }

  fs::path root, parent, path;
  open_material input;
  std::string baseline, expected_address;
  tools::wallet_file_binary_header header{};

private:
  void cleanup() noexcept
  {
    if (root.empty()) return;
    try
    {
      for (const auto& item : exclusions)
        if (item.second->held)
        {
          ADD_FAILURE() << "native fixture still owns storage exclusion";
          return;
        }
      boost::system::error_code error;
      const auto status = fs::symlink_status(root, error);
      if (error || fs::is_symlink(status) || !fs::is_directory(status))
      {
        ADD_FAILURE() << "native fixture root changed";
        return;
      }
      const auto current = fs::canonical(root, error);
      if (error || current != root || current.parent_path() != parent)
      {
        ADD_FAILURE() << "native fixture root changed";
        return;
      }
      fs::remove_all(current, error);
      if (error) ADD_FAILURE() << "native fixture cleanup failed";
      root.clear();
    }
    catch (...)
    {
      // GTest may itself throw with throw_on_failure enabled. Cleanup is
      // noexcept even when reporting the original fixed diagnostic fails.
      try { ADD_FAILURE() << "native fixture cleanup failed"; }
      catch (...) {}
    }
  }
  std::map<std::wstring, std::shared_ptr<exclusion>> exclusions;
};

// These observations prove only scoped guard coverage of provider callbacks,
// not capture or suppression of every process output path.
void expect_guarded_calls(const std::shared_ptr<provider_state>& control)
{
  if (control->material_reads) { EXPECT_TRUE(control->material_calls_silenced); }
  if (control->authorization_reads) { EXPECT_TRUE(control->authorization_calls_silenced); }
}

void expect_failure(fixture& f, lease_owner& lease,
  const std::shared_ptr<provider_state>& control, open_result expected,
  acquisition_kind kind = acquisition_kind::OpenExisting)
{
  const auto tree = snapshot(f.root);
  const auto* original = lease.get();
  owner session;
  EXPECT_EQ(expected, file_session::acquire(kind, lease, session));
  EXPECT_FALSE(session);
  EXPECT_TRUE(original == lease.get());
  EXPECT_TRUE(control->storage->held);
  EXPECT_EQ(0u, control->releases);
  expect_guarded_calls(control);
  // Never give GTest bytes, passwords, identities, addresses or paths.
  EXPECT_TRUE(tree == snapshot(f.root));
}

bool same_identity(const public_identity& actual, const public_identity& expected)
{
  return actual.spend_public_key == expected.spend_public_key &&
    actual.view_public_key == expected.view_public_key && actual.flags == expected.flags &&
    actual.stored_genesis == expected.stored_genesis;
}

bool canonical_receive(const receive_result& result)
{
  epee::log_space::scoped_log_silence quiet;
  currency::account_public_address parsed{};
  currency::payment_id_t payment_id;
  return currency::get_account_address_and_payment_id_from_str(parsed, payment_id, result.address) &&
    payment_id.empty() && parsed.spend_public_key == result.identity.spend_public_key &&
    parsed.view_public_key == result.identity.view_public_key && parsed.flags == result.identity.flags &&
    currency::get_account_address_as_str(parsed) == result.address;
}

template<class Body> void fixed_failure_boundary(Body&& body)
{
  try { body(); }
  catch (...) { ADD_FAILURE() << "native session fixture helper failed"; }
}
#define NATIVE_SESSION_TEST(name) \
  static void native_session_case_##name(); \
  TEST(native_wallet_session, name) \
  { fixed_failure_boundary(&native_session_case_##name); } \
  static void native_session_case_##name()

static_assert(std::has_virtual_destructor<material_lease>::value, "lease cleanup must dispatch");
static_assert(std::is_nothrow_destructible<material_lease>::value, "lease cleanup cannot throw");
static_assert(!std::is_copy_constructible<file_session>::value, "session cannot copy");
static_assert(!std::is_move_constructible<file_session>::value, "session cannot move");
static_assert(noexcept(file_session::acquire(acquisition_kind::OpenExisting,
  std::declval<lease_owner&>(), std::declval<owner&>())), "acquire has a fixed result");
static_assert(noexcept(std::declval<const file_session&>().get_receive()), "Receive cannot throw");
static_assert(noexcept(std::declval<file_session&>().release_unchanged()), "release cannot throw");
static_assert(noexcept(std::declval<file_session&>().step_sync(std::declval<std::atomic<bool>&>())),
  "a step has a typed result");
static_assert(noexcept(std::declval<file_session&>().checkpoint()), "checkpoint has a typed result");
static_assert(noexcept(std::declval<file_session&>().close()), "close has a typed result");
static_assert(noexcept(std::declval<file_session&>().capture_snapshot(std::declval<const snapshot_request&>())),
  "a snapshot has a typed result");
static_assert(!std::is_copy_constructible<snapshot_result>::value, "a snapshot result is published by move");

// Controlled in-process transport: one canned GET_BLOCKS_DIRECT response, no
// endpoint. Any other request the deny proxy leaked would be counted.
using blocks_request = currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request;
using blocks_response = currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response;
using block_entry = currency::block_direct_data_entry;
class controlled_transport final : public tools::i_core_proxy
{
public:
  blocks_response response{};
  size_t requests = 0, forbidden = 0;
  bool consume = false;
  std::function<void()> on_request;
  blocks_request last{};
  bool call_COMMAND_RPC_GET_BLOCKS_DIRECT(const blocks_request& request, blocks_response& out) override
  {
    ++requests;
    last = request;
    if (on_request) on_request();
    if (consume) out = std::move(response);
    else out = response;
    return true;
  }
  bool call_COMMAND_RPC_SEND_RAW_TX(const currency::COMMAND_RPC_SEND_RAW_TX::request&,
    currency::COMMAND_RPC_SEND_RAW_TX::response&) override { ++forbidden; return true; }
  bool call_COMMAND_RPC_INVOKE(const std::string&, const std::string&, int&, std::string&) override
  { ++forbidden; return true; }
};

void serve(controlled_transport& transport, uint64_t start, uint64_t current, std::list<block_entry> blocks)
{
  transport.response = blocks_response{};
  transport.response.status = API_RETURN_CODE_OK;
  transport.response.start_height = start;
  transport.response.current_height = current;
  transport.response.blocks = std::move(blocks);
}

crypto::public_key public_point()
{
  crypto::public_key point{};
  // Public Edwards base point, not wallet material.
  require(epee::string_tools::hex_to_pod(
    std::string("5866666666666666666666666666666666666666666666666666666666666666"), point));
  return point;
}

// The compiled genesis block itself: its hash is the build profile genesis
// every native fixture wallet is pinned to.
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

// A synthetic empty-coinbase child. `nonce` selects a competing sibling.
block_entry child(const block_entry& parent, uint64_t nonce = 0, bool with_key = true)
{
  auto block = std::make_shared<currency::block_extended_info>();
  block->bl = currency::block{};
  block->bl.major_version = 1;
  block->bl.nonce = nonce;
  block->bl.prev_id = currency::get_block_hash(parent.block_ptr->bl);
  block->bl.miner_tx.version = TRANSACTION_VERSION_INITAL;
  block->bl.miner_tx.vin.emplace_back(currency::txin_gen{currency::get_block_height(parent.block_ptr->bl) + 1});
  if (with_key) block->bl.miner_tx.extra.emplace_back(public_point());
  return {block, std::make_shared<currency::transaction_chain_entry>(),
    std::list<std::shared_ptr<const currency::transaction_chain_entry>>{}};
}

struct test_chain
{
  block_entry g = genesis_entry();
  block_entry b1 = child(g);
  block_entry b2 = child(b1);
  block_entry b3 = child(b2);
};

tools::read_only::step_limits limits() { return {8, 16}; }

std::shared_ptr<provider_state> capable(fixture& f)
{
  auto state = f.control(f.input);
  state->checkpoint_capable = true;
  return state;
}

owner open_session(fixture& f, const std::shared_ptr<provider_state>& state)
{
  auto lease = f.lease(f.input, state);
  owner session;
  require(file_session::acquire(acquisition_kind::OpenExisting, lease, session) == open_result::Opened);
  require(static_cast<bool>(session) && !lease);
  return session;
}

// Everything except the wallet file itself: no temporary, sidecar, vote file
// or sibling may be created or changed by a checkpoint.
tree_image without_wallet(tree_image tree)
{
  tree.erase("disposable.wallet");
  return tree;
}

// One real step; requires the scanner to have run.
tools::read_only::step_result step(file_session& session, std::atomic<bool>& stop,
  sync_step_status expected = sync_step_status::Stepped)
{
  const auto result = session.step_sync(stop);
  EXPECT_EQ(expected, result.status);
  return result.scan;
}

#if !((!defined(TESTNET) && CURRENCY_FORMATION_VERSION == 84) || \
      (defined(TESTNET) && CURRENCY_FORMATION_VERSION == 103))
NATIVE_SESSION_TEST(unsupported_profile_preserves_provider_lease_without_wallet_setup)
{
  EXPECT_EQ(profile_status::Unsupported, build_profile().status);
  EXPECT_TRUE(build_profile().genesis == currency::null_hash);
  fixture f(false);
  const auto c = f.control(f.input);
  auto lease = f.lease(f.input, c);
  expect_failure(f, lease, c, open_result::UnsupportedCapability);
  EXPECT_EQ(0u, c->material_reads);
}
#else
// Restores the legacy process-wide getter even when an expectation fails.
class legacy_genesis_override final
{
public:
  explicit legacy_genesis_override(const crypto::hash& value)
    : m_saved(currency::get_genesis_hash())
  {
    currency::get_genesis_hash(true, value);
  }
  ~legacy_genesis_override() noexcept { currency::get_genesis_hash(true, m_saved); }
  legacy_genesis_override(const legacy_genesis_override&) = delete;
  legacy_genesis_override& operator=(const legacy_genesis_override&) = delete;
private:
  crypto::hash m_saved;
};

NATIVE_SESSION_TEST(compiled_descriptor_is_the_pinned_profile_of_this_build)
{
  epee::log_space::scoped_log_silence quiet;
  const auto& profile = build_profile();
  ASSERT_EQ(profile_status::Available, profile.status);
  EXPECT_TRUE(&profile == &build_profile());
  EXPECT_TRUE(profile.genesis != currency::null_hash);
#ifdef TESTNET
  EXPECT_EQ(compiled_network::Testnet, profile.network);
  EXPECT_EQ(103u, profile.formation);
  // The shared default literal is MAINNET's. A TESTNET descriptor equal to it
  // would mean an accidental mainnet fallback. No TESTNET literal is pinned:
  // an independent known-answer/node identity check remains a later gate.
  EXPECT_TRUE(profile.genesis != currency::gdefault_genesis);
#else
  EXPECT_EQ(compiled_network::Mainnet, profile.network);
  EXPECT_EQ(84u, profile.formation);
  // The independently committed MAINNET literal agrees with generation.
  EXPECT_TRUE(profile.genesis == currency::gdefault_genesis);
#endif
  const auto again = derive_build_profile();
  EXPECT_EQ(profile.status, again.status);
  EXPECT_EQ(profile.network, again.network);
  EXPECT_EQ(profile.formation, again.formation);
  EXPECT_TRUE(profile.genesis == again.genesis);
}

NATIVE_SESSION_TEST(legacy_getter_override_cannot_change_descriptor_or_admission)
{
  fixture f;
  const auto authority = build_profile().genesis;
  auto forged = authority;
  reinterpret_cast<unsigned char*>(&forged)[0] ^= 1;
  {
    epee::log_space::scoped_log_silence quiet;
    legacy_genesis_override override_getter(forged);
    ASSERT_TRUE(currency::get_genesis_hash() == forged);
    // Neither the cached descriptor nor a fresh derivation follows the cache.
    EXPECT_TRUE(build_profile().genesis == authority);
    EXPECT_TRUE(derive_build_profile().genesis == authority);
    // A registry expectation matching only the forged getter is rejected
    // before any file access; the genuine expectation still opens.
    auto input = f.input;
    input.expected.stored_genesis = forged;
    const auto c = f.control(input);
    auto lease = f.lease(input, c);
    expect_failure(f, lease, c, open_result::GenesisMismatch);
    EXPECT_EQ(1u, c->material_reads);
    lease.reset(); // Same-path provider exclusion is released before reopening.
    const auto good = f.control(f.input);
    auto good_lease = f.lease(f.input, good);
    owner session;
    ASSERT_EQ(open_result::Opened,
      file_session::acquire(acquisition_kind::OpenExisting, good_lease, session));
    ASSERT_TRUE(static_cast<bool>(session));
    EXPECT_TRUE(session->get_receive().identity.stored_genesis == authority);
    EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  }
  epee::log_space::scoped_log_silence quiet;
  EXPECT_TRUE(currency::get_genesis_hash() != forged);
}

NATIVE_SESSION_TEST(foreign_or_opposite_network_expectation_rejects_before_file_access)
{
  // No wallet file exists: reaching the loader would report NotFound.
  fixture f(false);
  std::vector<crypto::hash> foreign;
  auto flipped = build_profile().genesis;
  reinterpret_cast<unsigned char*>(&flipped)[0] ^= 1;
  foreign.push_back(flipped);
#ifdef TESTNET
  // A registry record committed against the MAINNET literal.
  foreign.push_back(currency::gdefault_genesis);
#endif
  for (const auto& genesis : foreign)
  {
    auto input = f.input;
    input.expected.stored_genesis = genesis;
    const auto c = f.control(input);
    auto lease = f.lease(input, c);
    expect_failure(f, lease, c, open_result::GenesisMismatch);
    EXPECT_EQ(1u, c->material_reads);
    EXPECT_EQ(1u, c->authorization_reads);
  }
  EXPECT_FALSE(fs::exists(f.path));
}

#ifdef TESTNET
NATIVE_SESSION_TEST(testnet_file_left_on_the_shared_mainnet_default_is_rejected)
{
  // Keys and password are genuine; only the file's stored genesis is the
  // uninitialized MAINNET default. The post-load check must reject it.
  fixture f(true, file_genesis::LegacyDefault);
  const auto c = f.control(f.input);
  auto lease = f.lease(f.input, c);
  expect_failure(f, lease, c, open_result::GenesisMismatch);
  EXPECT_EQ(1u, c->material_reads);
  EXPECT_EQ(2u, c->authorization_reads);
}
#endif

NATIVE_SESSION_TEST(empty_path_or_null_genesis_preserves_material_without_loading)
{
  fixture f(false);
  for (unsigned variant = 0; variant != 2; ++variant)
  {
    auto input = f.input;
    if (variant == 0) input.path.clear();
    else input.expected.stored_genesis = currency::null_hash;
    const auto c = f.control(input);
    auto lease = f.lease(input, c);
    expect_failure(f, lease, c, open_result::InvalidMaterial);
    EXPECT_EQ(1u, c->material_reads);
    EXPECT_EQ(1u, c->authorization_reads);
  }
}

NATIVE_SESSION_TEST(valid_open_receive_and_explicit_close_preserve_the_tree)
{
  fixture f;
  const auto tree = snapshot(f.root);
  const auto c = f.control(f.input);
  auto lease = f.lease(f.input, c);
  owner session;
  ASSERT_EQ(open_result::Opened,
    file_session::acquire(acquisition_kind::OpenExisting, lease, session));
  ASSERT_TRUE(static_cast<bool>(session));
  EXPECT_FALSE(lease);
  EXPECT_EQ(session_state::OpenUnchanged, session->state());
  EXPECT_TRUE(c->storage->held);
  EXPECT_EQ(0u, c->releases);
  const auto material_reads = c->material_reads;
  for (unsigned i = 0; i != 5; ++i)
  {
    const auto receive = session->get_receive();
    EXPECT_EQ(receive_status::Available, receive.status);
    EXPECT_TRUE(receive.address == f.expected_address);
    EXPECT_TRUE(same_identity(receive.identity, f.input.expected));
    EXPECT_TRUE(canonical_receive(receive));
  }
  EXPECT_EQ(material_reads, c->material_reads);
  EXPECT_TRUE(tree == snapshot(f.root));
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  EXPECT_EQ(session_state::Released, session->state());
  EXPECT_EQ(1u, c->releases);
  EXPECT_TRUE(c->release_silenced);
  expect_guarded_calls(c);
  EXPECT_FALSE(c->storage->held);
  const auto closed = session->get_receive();
  EXPECT_EQ(receive_status::Unavailable, closed.status);
  EXPECT_TRUE(closed.address.empty());
  EXPECT_TRUE(same_identity(closed.identity, public_identity{}));
  EXPECT_EQ(release_result::AlreadyReleased, session->release_unchanged());
  session.reset();
  EXPECT_EQ(1u, c->releases);
  EXPECT_TRUE(tree == snapshot(f.root));
}

NATIVE_SESSION_TEST(receive_is_cached_and_authorization_revocation_hides_it)
{
  fixture f;
  const auto c = f.control(f.input);
  auto lease = f.lease(f.input, c);
  owner session;
  ASSERT_EQ(open_result::Opened,
    file_session::acquire(acquisition_kind::OpenExisting, lease, session));
  ASSERT_TRUE(static_cast<bool>(session));
  // Deliberate test-only removal after acquisition proves Receive does not
  // reload the wallet file. Release is compared against this new baseline.
  require(fs::remove(f.path));
  const auto tree = snapshot(f.root);
  const auto reads = c->material_reads;
  for (unsigned i = 0; i != 3; ++i)
  {
    const auto receive = session->get_receive();
    EXPECT_EQ(receive_status::Available, receive.status);
    EXPECT_TRUE(receive.address == f.expected_address);
    EXPECT_TRUE(same_identity(receive.identity, f.input.expected));
    EXPECT_TRUE(canonical_receive(receive));
  }
  EXPECT_EQ(reads, c->material_reads);
  // Revoke at the second Receive authorization observation, after the
  // public projection has been prepared but before it may be returned.
  c->revoke_at_authorization_read = c->authorization_reads + 2;
  const auto revoked = session->get_receive();
  EXPECT_EQ(receive_status::Unavailable, revoked.status);
  EXPECT_TRUE(revoked.address.empty());
  EXPECT_TRUE(same_identity(revoked.identity, public_identity{}));
  EXPECT_FALSE(c->authorized);
  EXPECT_EQ(receive_status::Unavailable, session->get_receive().status);
  EXPECT_EQ(0u, c->releases);
  EXPECT_TRUE(c->storage->held);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  EXPECT_EQ(1u, c->releases);
  EXPECT_TRUE(c->release_silenced);
  expect_guarded_calls(c);
  EXPECT_FALSE(c->storage->held);
  EXPECT_TRUE(tree == snapshot(f.root));
}

NATIVE_SESSION_TEST(wrong_password_and_independent_identity_or_genesis_are_typed)
{
  fixture f;
  for (unsigned variant = 0; variant != 5; ++variant)
  {
    auto input = f.input;
    if (variant == 0) input.password[0] = input.password[0] == '0' ? '1' : '0';
    if (variant == 1) reinterpret_cast<unsigned char*>(&input.expected.spend_public_key)[0] ^= 1;
    if (variant == 2) reinterpret_cast<unsigned char*>(&input.expected.view_public_key)[0] ^= 1;
    if (variant == 3) input.expected.flags ^= 1;
    if (variant == 4) reinterpret_cast<unsigned char*>(&input.expected.stored_genesis)[0] ^= 1;
    const auto c = f.control(input);
    auto lease = f.lease(input, c);
    const auto result = variant == 0 ? open_result::WrongPassword :
      variant == 4 ? open_result::GenesisMismatch : open_result::IdentityMismatch;
    expect_failure(f, lease, c, result);
  }
}

NATIVE_SESSION_TEST(stored_genesis_mismatch_preserves_provider_lease)
{
  // The provider remains pinned to the compiled profile. Only the encrypted
  // file's chain differs, exercising the post-load check, not the preflight.
  fixture f(true, file_genesis::Alternate);
  const auto c = f.control(f.input);
  auto lease = f.lease(f.input, c);
  expect_failure(f, lease, c, open_result::GenesisMismatch);
  EXPECT_EQ(1u, c->material_reads);
  EXPECT_EQ(2u, c->authorization_reads);
}

NATIVE_SESSION_TEST(malformed_truncated_trailing_and_unsupported_files_never_publish)
{
  fixture f;
  for (unsigned variant = 0; variant != 7; ++variant)
  {
    f.restore();
    auto header = f.header;
    if (variant == 0) write(f.path, f.baseline.substr(0, sizeof(header) - 1));
    if (variant == 1) { header.m_cb_keys = 0; f.replace_header(header); }
    if (variant == 2) write(f.path, f.baseline.substr(0, sizeof(header) + header.m_cb_keys - 1));
    if (variant == 3) write(f.path, f.baseline.substr(0, f.baseline.size() - 1));
    if (variant == 4) write(f.path, f.baseline + std::string(8193, 'x'));
    if (variant == 5) { header.m_ver = 0xffffffffu; f.replace_header(header); }
    if (variant == 6) { header.m_reserved = 1; f.replace_header(header); }
    const auto c = f.control(f.input);
    auto lease = f.lease(f.input, c);
    expect_failure(f, lease, c, variant >= 5 ? open_result::RecoveryRequired : open_result::InvalidFile);
  }
}

NATIVE_SESSION_TEST(occupied_output_and_consumed_lease_do_not_read_new_material)
{
  fixture f;
  const auto c = f.control(f.input);
  auto lease = f.lease(f.input, c);
  owner session;
  ASSERT_EQ(open_result::Opened,
    file_session::acquire(acquisition_kind::OpenExisting, lease, session));
  ASSERT_TRUE(static_cast<bool>(session));
  owner empty;
  EXPECT_EQ(open_result::InvalidMaterial,
    file_session::acquire(acquisition_kind::OpenExisting, lease, empty));
  EXPECT_FALSE(empty);
  auto other_input = f.input;
  other_input.path = (f.root / "other-unopened.wallet").wstring();
  const auto other = f.control(other_input);
  auto other_lease = f.lease(other_input, other);
  auto* const original_session = session.get();
  auto* const original_lease = other_lease.get();
  const auto tree = snapshot(f.root);
  EXPECT_EQ(open_result::OccupiedOutput,
    file_session::acquire(acquisition_kind::OpenExisting, other_lease, session));
  EXPECT_TRUE(original_session == session.get());
  EXPECT_TRUE(original_lease == other_lease.get());
  EXPECT_EQ(0u, other->material_reads);
  EXPECT_EQ(0u, other->releases);
  EXPECT_TRUE(other->storage->held);
  EXPECT_TRUE(tree == snapshot(f.root));
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  EXPECT_TRUE(c->release_silenced);
  expect_guarded_calls(c);
}

NATIVE_SESSION_TEST(revocation_before_material_and_before_publication_preserves_ownership)
{
  fixture f;
  for (unsigned stage = 0; stage != 3; ++stage)
  {
    const auto c = f.control(f.input);
    c->authorized = stage != 0;
    c->revoke_in_material = stage == 1;
    // The first two predicate samples authorize material access and loading.
    // The third revokes after the real load and canonical-address preparation.
    c->revoke_at_authorization_read = stage == 2 ? 3 : 0;
    auto lease = f.lease(f.input, c);
    expect_failure(f, lease, c, open_result::AuthorizationUnavailable);
    EXPECT_EQ(stage == 0 ? 0u : 1u, c->material_reads);
    EXPECT_EQ(stage + 1u, c->authorization_reads);
  }
}

NATIVE_SESSION_TEST(provider_exception_is_fixed_failure_without_consuming_lease)
{
  fixture f(false);
  const auto c = f.control(f.input);
  c->throw_in_material = true;
  auto lease = f.lease(f.input, c);
  expect_failure(f, lease, c, open_result::Unavailable);
  EXPECT_EQ(1u, c->material_reads);
}

NATIVE_SESSION_TEST(unsupported_create_restore_and_missing_parent_never_create_files)
{
  fixture f(false);
  for (const auto kind : {acquisition_kind::CreateNativeZano, acquisition_kind::RestoreNativeZano})
  {
    const auto c = f.control(f.input);
    auto lease = f.lease(f.input, c);
    expect_failure(f, lease, c, open_result::UnsupportedCapability, kind);
    EXPECT_EQ(0u, c->material_reads);
  }
  auto missing = f.input;
  missing.path = (f.root / "missing-parent" / "missing.wallet").wstring();
  const auto c = f.control(missing);
  auto lease = f.lease(missing, c);
  expect_failure(f, lease, c, open_result::NotFound);
  EXPECT_FALSE(fs::exists(f.root / "missing-parent"));
}

NATIVE_SESSION_TEST(owner_destruction_releases_material_and_exclusion_once)
{
  fixture f;
  const auto tree = snapshot(f.root);
  const auto c = f.control(f.input);
  auto lease = f.lease(f.input, c);
  owner session;
  ASSERT_EQ(open_result::Opened,
    file_session::acquire(acquisition_kind::OpenExisting, lease, session));
  ASSERT_TRUE(static_cast<bool>(session));
  EXPECT_FALSE(lease);
  EXPECT_TRUE(c->storage->held);
  // Deterministic provider-model exclusion only. No platform-lock claim:
  // the same-path slot cannot issue a second lease while the session owns it.
  const auto competing = f.control(f.input);
  bool rejected = false;
  try { auto forbidden = f.lease(f.input, competing); }
  catch (const std::runtime_error&) { rejected = true; }
  EXPECT_TRUE(rejected);
  EXPECT_TRUE(c->storage->held);
  EXPECT_EQ(0u, competing->releases);
  EXPECT_TRUE(tree == snapshot(f.root));
  session.reset();
  EXPECT_EQ(1u, c->releases);
  EXPECT_TRUE(c->release_silenced);
  expect_guarded_calls(c);
  EXPECT_FALSE(c->storage->held);
  EXPECT_TRUE(tree == snapshot(f.root));
  session.reset();
  EXPECT_EQ(1u, c->releases);
  // The caller can acquire and discard a new provider lease after teardown.
  // It was never consumed by a session, so no silence guard is required here.
  auto next = f.lease(f.input, competing);
  EXPECT_TRUE(competing->storage->held);
  next.reset();
  EXPECT_EQ(1u, competing->releases);
  EXPECT_FALSE(competing->storage->held);
  EXPECT_TRUE(tree == snapshot(f.root));
}

// Separate reveal authority. The subject is the test's own independently held
// expectation, never a value read back from the reopened wallet.
class test_backup_admission final : public backup_admission
{
public:
  explicit test_backup_admission(const public_identity& subject) : m_subject(subject) {}
  bool admission_current() const noexcept override
  {
    ++reads;
    if (revoke_at_read && reads >= revoke_at_read) current = false;
    return current;
  }
  const public_identity& subject() const noexcept override { return m_subject; }
  mutable bool current = true;
  mutable size_t reads = 0;
  size_t revoke_at_read = 0;
private:
  public_identity m_subject;
};

struct revealed
{
  std::string phrase;
  size_t visits = 0;
};

void capture_reveal(void* context, const char* phrase, size_t size) noexcept
{
  auto& target = *static_cast<revealed*>(context);
  ++target.visits;
  try { target.phrase.assign(phrase, size); }
  catch (...) {}
}

size_t word_total(const std::string& phrase)
{
  size_t count = 0;
  bool in_word = false;
  for (const unsigned char c : phrase)
  {
    const bool space = std::isspace(c) != 0;
    if (!space && !in_word) ++count;
    in_word = !space;
  }
  return count;
}

NATIVE_SESSION_TEST(backup_reveal_round_trips_the_verified_identity_without_retention)
{
  fixture f;
  const auto tree = snapshot(f.root);
  const auto c = f.control(f.input);
  auto lease = f.lease(f.input, c);
  owner session;
  ASSERT_EQ(open_result::Opened,
    file_session::acquire(acquisition_kind::OpenExisting, lease, session));
  test_backup_admission admission(f.input.expected);
  revealed out;
  EXPECT_EQ(backup_result::Presented, session->present_backup(admission, &capture_reveal, &out));
  EXPECT_EQ(1u, out.visits);
  EXPECT_EQ(2u, admission.reads);  // Before rendering and immediately before exposure.
  EXPECT_EQ(static_cast<size_t>(SEED_PHRASE_V2_WORDS_COUNT), word_total(out.phrase));
  {
    epee::log_space::scoped_log_silence quiet;
    currency::account_base recovered;
    ASSERT_TRUE(recovered.restore_from_seed_phrase(out.phrase, std::string()));
    const auto& address = recovered.get_public_address();
    EXPECT_TRUE(address.spend_public_key == f.input.expected.spend_public_key);
    EXPECT_TRUE(address.view_public_key == f.input.expected.view_public_key);
    EXPECT_EQ(f.input.expected.flags, address.flags);
  }
  // A reveal is read-only: the file tree is unchanged and Receive still works.
  EXPECT_TRUE(tree == snapshot(f.root));
  EXPECT_EQ(receive_status::Available, session->get_receive().status);
  // A second reveal needs (and here receives) the same current admission; the
  // session never cached the first rendering.
  revealed second;
  EXPECT_EQ(backup_result::Presented, session->present_backup(admission, &capture_reveal, &second));
  EXPECT_EQ(out.phrase, second.phrase);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  expect_guarded_calls(c);
}

NATIVE_SESSION_TEST(backup_reveal_refuses_revoked_cross_account_and_released_sessions)
{
  fixture f;
  const auto c = f.control(f.input);
  auto lease = f.lease(f.input, c);
  owner session;
  ASSERT_EQ(open_result::Opened,
    file_session::acquire(acquisition_kind::OpenExisting, lease, session));
  const auto tree = snapshot(f.root);
  const auto refused = [&](const test_backup_admission& admission, backup_result expected,
                           backup_visitor visitor = &capture_reveal)
  {
    revealed out;
    EXPECT_EQ(expected, session->present_backup(admission, visitor, &out));
    EXPECT_EQ(0u, out.visits);
    EXPECT_TRUE(out.phrase.empty());
  };
  {
    test_backup_admission stale(f.input.expected);
    stale.current = false;
    refused(stale, backup_result::Revoked);
  }
  {
    // Revoked after rendering, immediately before exposure.
    test_backup_admission late(f.input.expected);
    late.revoke_at_read = 2;
    refused(late, backup_result::Revoked);
  }
  for (unsigned variant = 0; variant != 4; ++variant)
  {
    auto other = f.input.expected;
    if (variant == 0) reinterpret_cast<unsigned char*>(&other.spend_public_key)[0] ^= 1;
    if (variant == 1) reinterpret_cast<unsigned char*>(&other.view_public_key)[0] ^= 1;
    if (variant == 2) other.flags ^= 1;
    if (variant == 3) reinterpret_cast<unsigned char*>(&other.stored_genesis)[0] ^= 1;
    test_backup_admission cross(other);
    refused(cross, backup_result::IdentityMismatch);
  }
  {
    test_backup_admission valid(f.input.expected);
    refused(valid, backup_result::InvalidVisitor, nullptr);
    EXPECT_EQ(0u, valid.reads);
  }
  {
    // The open authority is independent of the reveal admission.
    c->authorized = false;
    test_backup_admission valid(f.input.expected);
    refused(valid, backup_result::Revoked);
    c->authorized = true;
  }
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  {
    // The session's lifetime has ended; a current admission cannot revive it.
    test_backup_admission valid(f.input.expected);
    refused(valid, backup_result::Unavailable);
  }
  EXPECT_TRUE(tree == snapshot(f.root));
}

// ---- Persistent sync: scanner ownership, checkpoint and close outcomes. ----

#if NATIVE_SESSION_REPLACEMENT_FIXTURE
// Real replacement and strict reopen of the replaced file: Linux fixture only.
NATIVE_SESSION_TEST(checkpoints_save_repeated_progress_and_a_fresh_strict_reopen_resumes_from_it)
{
  fixture f;
  const test_chain chain;
  const auto tree = snapshot(f.root);
  const auto c = capable(f);
  auto session = open_session(f, c);
  const auto transport = std::make_shared<controlled_transport>();
  ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
  std::atomic<bool> stop{false};

  serve(*transport, 0, 2, {chain.g, chain.b1});
  auto scan = step(*session, stop);
  EXPECT_EQ(tools::read_only::step_status::Progress, scan.status);
  EXPECT_EQ(1u, scan.local_blocks_before);
  EXPECT_EQ(2u, scan.local_blocks_after);
  EXPECT_EQ(0u, transport->last.minimum_height);  // Fixed at zero, from genesis.
  EXPECT_EQ(session_state::OpenDirty, session->state());
  // Scanning never writes, and the old close cannot discard the progress.
  EXPECT_TRUE(f.baseline == read(f.path));
  EXPECT_EQ(release_result::Unavailable, session->release_unchanged());
  EXPECT_TRUE(c->storage->held);
  EXPECT_EQ(receive_status::Available, session->get_receive().status);

  EXPECT_EQ(checkpoint_result::Saved, session->checkpoint());
  EXPECT_EQ(1u, c->checkpoints);
  EXPECT_TRUE(c->checkpoint_authorized);
  EXPECT_TRUE(c->checkpoint_silenced);
  EXPECT_EQ(session_state::OpenUnchanged, session->state());
  const auto first = read(f.path);
  EXPECT_TRUE(first != f.baseline);
  EXPECT_TRUE(without_wallet(tree) == without_wallet(snapshot(f.root)));
  // Dirty state was reset only by that commit: nothing further to save.
  EXPECT_EQ(checkpoint_result::Unchanged, session->checkpoint());
  EXPECT_EQ(1u, c->checkpoints);

  serve(*transport, 1, 3, {chain.b1, chain.b2});
  scan = step(*session, stop);
  EXPECT_EQ(tools::read_only::step_status::Progress, scan.status);
  EXPECT_EQ(3u, scan.local_blocks_after);
  EXPECT_EQ(session_state::OpenDirty, session->state());
  EXPECT_EQ(checkpoint_result::Saved, session->checkpoint());
  EXPECT_EQ(2u, c->checkpoints);
  EXPECT_TRUE(read(f.path) != first);
  EXPECT_EQ(close_result::ReleasedUnchanged, session->close());
  EXPECT_EQ(session_state::Released, session->state());
  EXPECT_EQ(close_result::AlreadyReleased, session->close());
  EXPECT_EQ(1u, c->releases);
  EXPECT_FALSE(c->storage->held);
  EXPECT_EQ(2u, transport->requests);
  EXPECT_EQ(0u, transport->forbidden);
  EXPECT_TRUE(without_wallet(tree) == without_wallet(snapshot(f.root)));

  // Independent strict reopen (same identity/genesis expectation) of the
  // replaced file: the saved chain is the starting point of the next step.
  const auto next = capable(f);
  auto again = open_session(f, next);
  EXPECT_EQ(3u, checkpoint_test_access::local_blocks(*again));
  EXPECT_EQ(session_state::OpenUnchanged, again->state());
  EXPECT_TRUE(same_identity(again->get_receive().identity, f.input.expected));
  ASSERT_EQ(sync_prepare_result::Prepared, again->prepare_sync(transport, limits()));
  serve(*transport, 2, 4, {chain.b2, chain.b3});
  scan = step(*again, stop);
  EXPECT_EQ(tools::read_only::step_status::Progress, scan.status);
  EXPECT_EQ(3u, scan.local_blocks_before);
  EXPECT_EQ(4u, scan.local_blocks_after);
  EXPECT_EQ(close_result::ReleasedSaved, again->close());
  EXPECT_EQ(1u, next->checkpoints);
  EXPECT_EQ(1u, next->releases);
  auto third = open_session(f, capable(f));
  EXPECT_EQ(4u, checkpoint_test_access::local_blocks(*third));
  EXPECT_EQ(close_result::ReleasedUnchanged, third->close());
}

NATIVE_SESSION_TEST(cancelled_mutation_without_height_increase_stays_dirty_until_close_saves_it)
{
  fixture f;
  const auto c = capable(f);
  auto session = open_session(f, c);
  const auto transport = std::make_shared<controlled_transport>();
  ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
  // The handler drops the genesis entry after processing it; the sole owner
  // of its indices then raises the caller's stop flag at that boundary.
  const auto stop = std::make_shared<std::atomic<bool>>(false);
  auto genesis = genesis_entry();
  const auto first = child(genesis);
  {
    auto* indices = new currency::transaction_chain_entry(*genesis.coinbase_ptr);
    genesis.coinbase_ptr = std::shared_ptr<const currency::transaction_chain_entry>(indices,
      [stop](const currency::transaction_chain_entry* entry) { delete entry; *stop = true; });
  }
  transport->consume = true;
  std::list<block_entry> blocks;
  blocks.push_back(std::move(genesis));
  blocks.push_back(first);
  serve(*transport, 0, 2, std::move(blocks));
  const auto scan = step(*session, *stop);
  EXPECT_EQ(tools::read_only::step_status::Cancelled, scan.status);
  EXPECT_TRUE(stop->load());
  EXPECT_TRUE(scan.state_may_have_changed);
  EXPECT_EQ(1u, scan.local_blocks_before);
  EXPECT_EQ(1u, scan.local_blocks_after);
  EXPECT_EQ(0u, scan.blocks_added);
  // Equal height, cancelled: still dirty, never unchanged.
  EXPECT_EQ(session_state::OpenDirty, session->state());
  EXPECT_EQ(release_result::Unavailable, session->release_unchanged());
  // A later step on the same stop flag is cancelled before any request and
  // cannot clear the obligation either; neither can scanner recreation.
  EXPECT_EQ(tools::read_only::step_status::Cancelled, step(*session, *stop).status);
  EXPECT_EQ(1u, transport->requests);
  ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
  EXPECT_EQ(session_state::OpenDirty, session->state());
  EXPECT_EQ(0u, c->releases);
  EXPECT_TRUE(f.baseline == read(f.path));
  EXPECT_EQ(close_result::ReleasedSaved, session->close());
  EXPECT_EQ(1u, c->checkpoints);
  EXPECT_EQ(1u, c->releases);
  EXPECT_TRUE(f.baseline != read(f.path));
  auto again = open_session(f, capable(f));
  EXPECT_EQ(1u, checkpoint_test_access::local_blocks(*again));
  EXPECT_EQ(close_result::ReleasedUnchanged, again->close());
}

NATIVE_SESSION_TEST(equal_height_reorg_is_dirty_and_the_saved_branch_survives_reopen)
{
  fixture f;
  const test_chain chain;
  const auto sibling = child(chain.g, 7);
  const auto after_sibling = child(sibling);
  const auto transport = std::make_shared<controlled_transport>();
  std::atomic<bool> stop{false};
  {
    auto session = open_session(f, capable(f));
    ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
    serve(*transport, 0, 2, {chain.g, chain.b1});
    EXPECT_EQ(tools::read_only::step_status::Progress, step(*session, stop).status);
    EXPECT_EQ(checkpoint_result::Saved, session->checkpoint());
    // Same height, different block: a mutation without height increase.
    serve(*transport, 0, 2, {chain.g, sibling});
    const auto scan = step(*session, stop);
    EXPECT_EQ(tools::read_only::step_status::Progress, scan.status);
    EXPECT_EQ(2u, scan.local_blocks_before);
    EXPECT_EQ(2u, scan.local_blocks_after);
    EXPECT_EQ(session_state::OpenDirty, session->state());
    EXPECT_EQ(release_result::Unavailable, session->release_unchanged());
    EXPECT_EQ(close_result::ReleasedSaved, session->close());
  }
  auto again = open_session(f, capable(f));
  ASSERT_EQ(sync_prepare_result::Prepared, again->prepare_sync(transport, limits()));
  // The replaced branch is no longer a known anchor (clean preflight refusal,
  // no fault latched); the saved sibling is.
  serve(*transport, 1, 3, {chain.b1, chain.b2});
  const auto stale = step(*again, stop);
  EXPECT_EQ(tools::read_only::step_status::RecoveryRequired, stale.status);
  EXPECT_FALSE(stale.state_may_have_changed);
  EXPECT_EQ(session_state::OpenUnchanged, again->state());
  serve(*transport, 1, 3, {sibling, after_sibling});
  const auto resumed = step(*again, stop);
  EXPECT_EQ(tools::read_only::step_status::Progress, resumed.status);
  EXPECT_EQ(2u, resumed.local_blocks_before);
  EXPECT_EQ(3u, resumed.local_blocks_after);
  EXPECT_EQ(close_result::ReleasedSaved, again->close());
}

NATIVE_SESSION_TEST(revoke_during_transport_blocks_scans_and_reads_but_clean_progress_drains)
{
  fixture f;
  const test_chain chain;
  const auto c = capable(f);
  auto session = open_session(f, c);
  const auto transport = std::make_shared<controlled_transport>();
  ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
  std::atomic<bool> stop{false};
  serve(*transport, 0, 2, {chain.g, chain.b1});
  EXPECT_EQ(tools::read_only::step_status::Progress, step(*session, stop).status);

  // Revocation and the stop signal arrive while the request is in flight.
  // The step cannot interrupt the transport; it must not be torn down under
  // it, and completing afterwards must not restore authority.
  close_result nested_close = close_result::Unavailable;
  release_result nested_release = release_result::ReleasedUnchanged;
  checkpoint_result nested_checkpoint = checkpoint_result::Saved;
  sync_step_status nested_step = sync_step_status::Stepped;
  transport->on_request = [&]
  {
    c->authorized = false;
    stop = true;
    nested_close = session->close();
    nested_release = session->release_unchanged();
    nested_checkpoint = session->checkpoint();
    nested_step = session->step_sync(stop).status;
  };
  serve(*transport, 1, 3, {chain.b1, chain.b2});
  const auto late = step(*session, stop, sync_step_status::Revoked);
  EXPECT_EQ(tools::read_only::step_status::Cancelled, late.status);
  EXPECT_FALSE(late.state_may_have_changed);
  EXPECT_EQ(close_result::Busy, nested_close);
  EXPECT_EQ(release_result::Unavailable, nested_release);
  EXPECT_EQ(checkpoint_result::Busy, nested_checkpoint);
  EXPECT_EQ(sync_step_status::Busy, nested_step);
  EXPECT_EQ(0u, c->releases);
  EXPECT_TRUE(c->storage->held);
  EXPECT_EQ(0u, c->checkpoints);
  transport->on_request = nullptr;

  // A provider reporting current again cannot restore scans or reads.
  c->authorized = true;
  std::atomic<bool> fresh{false};
  EXPECT_EQ(sync_step_status::Revoked, session->step_sync(fresh).status);
  EXPECT_EQ(sync_prepare_result::Revoked, session->prepare_sync(transport, limits()));
  EXPECT_EQ(receive_status::Unavailable, session->get_receive().status);
  EXPECT_EQ(2u, transport->requests);
  c->authorized = false;

  // Drain: the held writer saves the clean progress with no authorization.
  EXPECT_EQ(session_state::OpenDirty, session->state());
  EXPECT_EQ(close_result::ReleasedSaved, session->close());
  EXPECT_EQ(1u, c->checkpoints);
  EXPECT_FALSE(c->checkpoint_authorized);
  EXPECT_EQ(1u, c->releases);
  EXPECT_FALSE(c->storage->held);
  auto again = open_session(f, capable(f));
  EXPECT_EQ(2u, checkpoint_test_access::local_blocks(*again));
  EXPECT_EQ(close_result::ReleasedUnchanged, again->close());
}
#endif // NATIVE_SESSION_REPLACEMENT_FIXTURE

// Useful data returns after revocation with stop NOT set, so the scan applies
// it. The step reports Revoked with the dirty observation retained; nothing
// restores public reads or scanning. Drain uses the already-held writer.
NATIVE_SESSION_TEST(late_revocation_without_stop_keeps_the_useful_scan_dirty_and_drains)
{
  fixture f;
  const test_chain chain;
  const auto c = capable(f);
  auto session = open_session(f, c);
  const auto transport = std::make_shared<controlled_transport>();
  ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
  std::atomic<bool> stop{false};
  EXPECT_EQ(session_state::OpenUnchanged, session->state());

  transport->on_request = [&] { c->authorized = false; };
  serve(*transport, 0, 3, {chain.g, chain.b1, chain.b2});
  const auto late = step(*session, stop, sync_step_status::Revoked);
  transport->on_request = nullptr;
  EXPECT_FALSE(stop.load());
  EXPECT_EQ(tools::read_only::step_status::Progress, late.status);
  EXPECT_TRUE(late.state_may_have_changed);
  EXPECT_EQ(1u, late.local_blocks_before);
  EXPECT_EQ(3u, late.local_blocks_after);
  // The applied progress is a dirty obligation, not published state.
  EXPECT_EQ(session_state::OpenDirty, session->state());
  EXPECT_EQ(3u, checkpoint_test_access::local_blocks(*session));
  EXPECT_EQ(receive_status::Unavailable, session->get_receive().status);
  EXPECT_EQ(release_result::Unavailable, session->release_unchanged());
  EXPECT_TRUE(f.baseline == read(f.path));

  // The provider reporting current again restores neither reads nor scans.
  c->authorized = true;
  EXPECT_EQ(receive_status::Unavailable, session->get_receive().status);
  {
    test_backup_admission admission(f.input.expected);
    revealed out;
    EXPECT_EQ(backup_result::Revoked, session->present_backup(admission, &capture_reveal, &out));
    EXPECT_EQ(0u, out.visits);
  }
  EXPECT_EQ(sync_step_status::Revoked, session->step_sync(stop).status);
  EXPECT_EQ(sync_prepare_result::Revoked, session->prepare_sync(transport, limits()));
  EXPECT_EQ(sync_step_status::Revoked, session->step_sync(stop).status);
  EXPECT_EQ(1u, transport->requests);
  EXPECT_EQ(0u, transport->forbidden);
  EXPECT_EQ(session_state::OpenDirty, session->state());
  EXPECT_EQ(0u, c->checkpoints);
  EXPECT_EQ(0u, c->releases);
  EXPECT_TRUE(c->storage->held);

  if (replacement_supported)
  {
    // Drain through the held writer, without a new authorization sample.
    c->authorized = false;
    const auto samples = c->authorization_reads;
    EXPECT_EQ(close_result::ReleasedSaved, session->close());
    EXPECT_EQ(samples, c->authorization_reads);
    EXPECT_EQ(1u, c->checkpoints);
    EXPECT_FALSE(c->checkpoint_authorized);
    EXPECT_EQ(1u, c->releases);
    EXPECT_FALSE(c->storage->held);
    auto again = open_session(f, capable(f));
    EXPECT_EQ(3u, checkpoint_test_access::local_blocks(*again));
    EXPECT_TRUE(same_identity(again->get_receive().identity, f.input.expected));
    EXPECT_EQ(close_result::ReleasedUnchanged, again->close());
  }
}

// Calls made from inside the provider's checkpoint writer are refused without
// tearing anything down, and the outer checkpoint/close outcome is unaffected.
NATIVE_SESSION_TEST(checkpoint_writer_reentrancy_is_refused_and_keeps_the_outer_outcome)
{
  const test_chain chain;
  for (const auto mode : {writer_mode::Reject, writer_mode::Commit})
  {
    // A committed outer write needs the Linux replacement fixture.
    if (mode == writer_mode::Commit && !replacement_supported) continue;
    SCOPED_TRACE(mode == writer_mode::Commit ? "commit" : "reject");
    fixture f;
    const auto c = capable(f);
    c->writer = mode;
    auto session = open_session(f, c);
    const auto transport = std::make_shared<controlled_transport>();
    ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
    std::atomic<bool> stop{false};
    serve(*transport, 0, 2, {chain.g, chain.b1});
    EXPECT_EQ(tools::read_only::step_status::Progress, step(*session, stop).status);

    struct nested_calls
    {
      size_t calls = 0;
      sync_step_status step = sync_step_status::Stepped;
      sync_prepare_result prepare = sync_prepare_result::Prepared;
      checkpoint_result checkpoint = checkpoint_result::Saved;
      close_result close = close_result::ReleasedSaved;
      release_result release = release_result::ReleasedUnchanged;
      receive_status receive = receive_status::Available;
      session_state state = session_state::Released;
      bool held = false;
      size_t releases = 1;
    };
    nested_calls nested;
    std::atomic<bool> nested_stop{false};
    const auto reenter = [&]
    {
      ++nested.calls;
      nested.step = session->step_sync(nested_stop).status;
      nested.prepare = session->prepare_sync(transport, limits());
      nested.checkpoint = session->checkpoint();
      nested.close = session->close();
      nested.release = session->release_unchanged();
      nested.receive = session->get_receive().status;
      nested.state = session->state();
      nested.held = c->storage->held;
      nested.releases = c->releases;
    };
    const auto expect_refused = [&]
    {
      EXPECT_EQ(1u, nested.calls);
      EXPECT_EQ(sync_step_status::Busy, nested.step);
      EXPECT_EQ(sync_prepare_result::Busy, nested.prepare);
      EXPECT_EQ(checkpoint_result::Busy, nested.checkpoint);
      EXPECT_EQ(close_result::Busy, nested.close);
      EXPECT_EQ(release_result::Unavailable, nested.release);
      EXPECT_EQ(receive_status::Unavailable, nested.receive);
      EXPECT_EQ(session_state::OpenDirty, nested.state);
      EXPECT_TRUE(nested.held);
      EXPECT_EQ(0u, nested.releases);
    };

    c->on_checkpoint = reenter;
    const auto outer = session->checkpoint();
    c->on_checkpoint = nullptr;
    expect_refused();
    // The nested checkpoint/close never reached the writer; no nested scan ran.
    EXPECT_EQ(1u, c->checkpoints);
    EXPECT_EQ(1u, transport->requests);
    EXPECT_EQ(0u, c->releases);
    EXPECT_TRUE(c->storage->held);

    if (mode == writer_mode::Reject)
    {
      EXPECT_EQ(checkpoint_result::Rejected, outer);
      EXPECT_EQ(session_state::OpenDirty, session->state());
      EXPECT_TRUE(f.baseline == read(f.path));
      EXPECT_EQ(receive_status::Available, session->get_receive().status);
      // Still retained and retryable after the refused nested calls.
      nested = nested_calls{};
      c->on_checkpoint = reenter;
      EXPECT_EQ(close_result::RetainedFailure, session->close());
      c->on_checkpoint = nullptr;
      expect_refused();
      EXPECT_EQ(2u, c->checkpoints);
      EXPECT_EQ(0u, c->releases);
      EXPECT_TRUE(c->storage->held);
      EXPECT_TRUE(f.baseline == read(f.path));
      // Test teardown only; not persistence evidence.
      session.reset();
      EXPECT_EQ(1u, c->releases);
      continue;
    }

    // Commit: the outer write's own result clears dirty state exactly once.
    EXPECT_EQ(checkpoint_result::Saved, outer);
    EXPECT_EQ(session_state::OpenUnchanged, session->state());
    EXPECT_TRUE(f.baseline != read(f.path));
    EXPECT_EQ(checkpoint_result::Unchanged, session->checkpoint());
    EXPECT_EQ(1u, c->checkpoints);
    // The same refusal from inside a close-driven checkpoint; the outer close
    // then releases once, after its commit.
    serve(*transport, 1, 3, {chain.b1, chain.b2});
    EXPECT_EQ(tools::read_only::step_status::Progress, step(*session, stop).status);
    nested = nested_calls{};
    c->on_checkpoint = reenter;
    EXPECT_EQ(close_result::ReleasedSaved, session->close());
    c->on_checkpoint = nullptr;
    expect_refused();
    EXPECT_EQ(2u, c->checkpoints);
    EXPECT_EQ(2u, transport->requests);
    EXPECT_EQ(session_state::Released, session->state());
    EXPECT_EQ(1u, c->releases);
    EXPECT_FALSE(c->storage->held);
    auto again = open_session(f, capable(f));
    EXPECT_EQ(3u, checkpoint_test_access::local_blocks(*again));
    EXPECT_EQ(close_result::ReleasedUnchanged, again->close());
  }
}

NATIVE_SESSION_TEST(incapable_or_revoked_provider_cannot_start_mutation_and_old_close_works)
{
  fixture f;
  const auto tree = snapshot(f.root);
  const auto transport = std::make_shared<controlled_transport>();
  std::atomic<bool> stop{false};
  {
    // Existing provider shape: no checkpoint capability.
    const auto c = f.control(f.input);
    auto session = open_session(f, c);
    EXPECT_EQ(sync_prepare_result::CheckpointUnsupported, session->prepare_sync(transport, limits()));
    EXPECT_EQ(sync_step_status::NotPrepared, session->step_sync(stop).status);
    EXPECT_EQ(checkpoint_result::Unchanged, session->checkpoint());
    EXPECT_EQ(receive_status::Available, session->get_receive().status);
    EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
    EXPECT_EQ(0u, c->checkpoints);
  }
  {
    const auto c = capable(f);
    auto session = open_session(f, c);
    EXPECT_EQ(sync_prepare_result::InvalidTransport, session->prepare_sync(nullptr, limits()));
    c->authorized = false;
    EXPECT_EQ(sync_prepare_result::Revoked, session->prepare_sync(transport, limits()));
    c->authorized = true;
    EXPECT_EQ(sync_prepare_result::Revoked, session->prepare_sync(transport, limits()));
    EXPECT_EQ(sync_step_status::NotPrepared, session->step_sync(stop).status);
    EXPECT_EQ(close_result::ReleasedUnchanged, session->close());
    EXPECT_EQ(0u, c->checkpoints);
  }
  EXPECT_EQ(0u, transport->requests);
  EXPECT_TRUE(tree == snapshot(f.root));
}

NATIVE_SESSION_TEST(rejected_writer_keeps_the_previous_file_and_a_retryable_dirty_session)
{
  fixture f;
  const test_chain chain;
  const auto c = capable(f);
  c->writer = writer_mode::Reject;
  auto session = open_session(f, c);
  const auto transport = std::make_shared<controlled_transport>();
  ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
  std::atomic<bool> stop{false};
  serve(*transport, 0, 2, {chain.g, chain.b1});
  EXPECT_EQ(tools::read_only::step_status::Progress, step(*session, stop).status);
  EXPECT_EQ(checkpoint_result::Rejected, session->checkpoint());
  EXPECT_EQ(session_state::OpenDirty, session->state());
  EXPECT_EQ(close_result::RetainedFailure, session->close());
  EXPECT_EQ(release_result::Unavailable, session->release_unchanged());
  EXPECT_EQ(2u, c->checkpoints);
  EXPECT_EQ(0u, c->releases);
  EXPECT_TRUE(c->storage->held);
  EXPECT_TRUE(f.baseline == read(f.path));
  // Everything is retained: progress continues and a later commit saves it.
  EXPECT_EQ(receive_status::Available, session->get_receive().status);
  serve(*transport, 1, 3, {chain.b1, chain.b2});
  EXPECT_EQ(tools::read_only::step_status::Progress, step(*session, stop).status);
  if (!replacement_supported) return; // The committed retry needs the Linux fixture.
  c->writer = writer_mode::Commit;
  EXPECT_EQ(close_result::ReleasedSaved, session->close());
  EXPECT_EQ(3u, c->checkpoints);
  EXPECT_EQ(1u, c->releases);
  auto again = open_session(f, capable(f));
  EXPECT_EQ(3u, checkpoint_test_access::local_blocks(*again));
  EXPECT_EQ(close_result::ReleasedUnchanged, again->close());
}

NATIVE_SESSION_TEST(throwing_or_uncertain_writer_is_a_sticky_unknown_obligation)
{
  for (const auto mode : {writer_mode::Throw, writer_mode::UnknownAfterReplace})
  {
    // A real replacement followed by an uncertain result needs the Linux fixture.
    if (mode == writer_mode::UnknownAfterReplace && !replacement_supported) continue;
    SCOPED_TRACE(mode == writer_mode::Throw ? "throw" : "unknown-after-replace");
    fixture f;
    const auto tree = snapshot(f.root);
    const test_chain chain;
    const auto c = capable(f);
    c->writer = mode;
    auto session = open_session(f, c);
    const auto transport = std::make_shared<controlled_transport>();
    ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
    std::atomic<bool> stop{false};
    serve(*transport, 0, 2, {chain.g, chain.b1});
    EXPECT_EQ(tools::read_only::step_status::Progress, step(*session, stop).status);
    EXPECT_EQ(checkpoint_result::OutcomeUnknown, session->checkpoint());
    EXPECT_EQ(session_state::OutcomeUnknown, session->state());
    // The file may or may not hold the image; either way it stays unknown.
    EXPECT_EQ(mode == writer_mode::Throw, f.baseline == read(f.path));
    // No ordinary retry, scan, read or release; a now-healthy writer is not
    // consulted.
    c->writer = writer_mode::Commit;
    EXPECT_EQ(checkpoint_result::OutcomeUnknown, session->checkpoint());
    EXPECT_EQ(sync_step_status::OutcomeUnknown, session->step_sync(stop).status);
    EXPECT_EQ(sync_prepare_result::UnsupportedState, session->prepare_sync(transport, limits()));
    EXPECT_EQ(receive_status::Unavailable, session->get_receive().status);
    EXPECT_EQ(close_result::OutcomeUnknown, session->close());
    EXPECT_EQ(release_result::Unavailable, session->release_unchanged());
    EXPECT_EQ(1u, c->checkpoints);
    EXPECT_EQ(1u, transport->requests);
    EXPECT_EQ(0u, c->releases);
    EXPECT_TRUE(c->storage->held);
    // Owner-level reconciliation is outside the session. Destruction only
    // tears down; it reports nothing and is not persistence evidence.
    session.reset();
    EXPECT_EQ(1u, c->releases);
    EXPECT_FALSE(c->storage->held);
    // Explicit recovery input check after that teardown: the target strictly
    // reopens under the test's independent identity/genesis expectation, with
    // the prior file (Throw) or the replaced image (UnknownAfterReplace). This
    // reads the target; it does not turn the Unknown verdict into Saved, and
    // no session or provider API acknowledged the write.
    EXPECT_TRUE(without_wallet(tree) == without_wallet(snapshot(f.root)));
    const auto recovery = capable(f);
    auto again = open_session(f, recovery);
    EXPECT_EQ(mode == writer_mode::Throw ? 1u : 2u, checkpoint_test_access::local_blocks(*again));
    EXPECT_EQ(session_state::OpenUnchanged, again->state());
    const auto reopened = again->get_receive();
    EXPECT_EQ(receive_status::Available, reopened.status);
    EXPECT_TRUE(same_identity(reopened.identity, f.input.expected));
    EXPECT_TRUE(reopened.address == f.expected_address);
    EXPECT_EQ(close_result::ReleasedUnchanged, again->close());
    EXPECT_EQ(0u, recovery->checkpoints);
    EXPECT_EQ(1u, c->checkpoints);
  }
}

NATIVE_SESSION_TEST(sticky_processing_fault_is_retained_and_never_written)
{
  fixture f;
  const test_chain chain;
  const auto c = capable(f);
  auto session = open_session(f, c);
  const auto transport = std::make_shared<controlled_transport>();
  ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
  std::atomic<bool> stop{false};
  serve(*transport, 0, 2, {chain.g, chain.b1});
  EXPECT_EQ(tools::read_only::step_status::Progress, step(*session, stop).status);
  // The earlier commit needs the Linux fixture; the fault coverage does not.
  if (replacement_supported) EXPECT_EQ(checkpoint_result::Saved, session->checkpoint());
  const size_t committed = replacement_supported ? 1 : 0;
  const auto saved = read(f.path);
  // Malformed coinbase extra fails inside legacy processing, after mutation began.
  auto malformed = child(chain.b1, 0, false);
  {
    auto block = std::make_shared<currency::block_extended_info>(*malformed.block_ptr);
    block->bl.miner_tx.extra.emplace_back(crypto::public_key{});
    block->bl.miner_tx.extra.emplace_back(crypto::public_key{});
    malformed.block_ptr = block;
  }
  serve(*transport, 1, 3, {chain.b1, malformed});
  const auto scan = step(*session, stop);
  EXPECT_EQ(tools::read_only::step_status::RecoveryRequired, scan.status);
  EXPECT_TRUE(scan.state_may_have_changed);
  EXPECT_EQ(session_state::RecoveryRequired, session->state());
  EXPECT_EQ(checkpoint_result::RecoveryRequired, session->checkpoint());
  EXPECT_EQ(close_result::RecoveryRequired, session->close());
  EXPECT_EQ(release_result::Unavailable, session->release_unchanged());
  // Scanner recreation and further steps cannot clear the latch.
  EXPECT_EQ(sync_prepare_result::UnsupportedState, session->prepare_sync(transport, limits()));
  EXPECT_EQ(sync_step_status::RecoveryRequired, session->step_sync(stop).status);
  EXPECT_EQ(2u, transport->requests);
  EXPECT_EQ(committed, c->checkpoints);
  EXPECT_EQ(0u, c->releases);
  EXPECT_TRUE(c->storage->held);
  EXPECT_TRUE(saved == read(f.path));
  session.reset();
  // The last committed state, not the faulted one, is what reopens.
  auto again = open_session(f, capable(f));
  EXPECT_EQ(1u + committed, checkpoint_test_access::local_blocks(*again));
  EXPECT_EQ(close_result::ReleasedUnchanged, again->close());
}

NATIVE_SESSION_TEST(identity_or_genesis_drift_is_never_written)
{
  fixture f;
  const test_chain chain;
  for (unsigned variant = 0; variant != 2; ++variant)
  {
    const auto c = capable(f);
    auto session = open_session(f, c);
    const auto transport = std::make_shared<controlled_transport>();
    ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
    std::atomic<bool> stop{false};
    serve(*transport, 0, 2, {chain.g, chain.b1});
    EXPECT_EQ(tools::read_only::step_status::Progress, step(*session, stop).status);
    if (variant == 0) checkpoint_test_access::forge_genesis(*session); // Loaded wallet drift.
    else c->drift_material = true;                                     // Lease identity drift.
    EXPECT_EQ(checkpoint_result::IdentityMismatch, session->checkpoint());
    EXPECT_EQ(session_state::RecoveryRequired, session->state());
    EXPECT_EQ(close_result::RecoveryRequired, session->close());
    EXPECT_EQ(checkpoint_result::RecoveryRequired, session->checkpoint());
    EXPECT_EQ(0u, c->checkpoints);
    EXPECT_EQ(0u, c->releases);
    EXPECT_TRUE(f.baseline == read(f.path));
    session.reset();
  }
}

NATIVE_SESSION_TEST(image_limit_is_enforced_before_the_provider_and_retains_progress)
{
  fixture f;
  const test_chain chain;
  const auto c = capable(f);
  auto session = open_session(f, c);
  const auto transport = std::make_shared<controlled_transport>();
  ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
  std::atomic<bool> stop{false};
  serve(*transport, 0, 2, {chain.g, chain.b1});
  EXPECT_EQ(tools::read_only::step_status::Progress, step(*session, stop).status);
  // Header too large for the bound, then header and keys fitting with the
  // encrypted body overflowing the bounded sink.
  for (const size_t limit : {size_t{1}, sizeof(tools::wallet_file_binary_header) + f.header.m_cb_keys + 1})
  {
    checkpoint_test_access::limit_image(*session, limit);
    EXPECT_EQ(checkpoint_result::ImageLimit, session->checkpoint());
    EXPECT_EQ(session_state::OpenDirty, session->state());
    EXPECT_EQ(close_result::RetainedFailure, session->close());
  }
  EXPECT_EQ(0u, c->checkpoints);
  EXPECT_EQ(0u, c->releases);
  EXPECT_TRUE(f.baseline == read(f.path));
  if (!replacement_supported) return; // The in-bound commit needs the Linux fixture.
  checkpoint_test_access::limit_image(*session, max_checkpoint_image_bytes);
  EXPECT_EQ(close_result::ReleasedSaved, session->close());
  EXPECT_EQ(1u, c->checkpoints);
  auto again = open_session(f, capable(f));
  EXPECT_EQ(2u, checkpoint_test_access::local_blocks(*again));
  EXPECT_EQ(close_result::ReleasedUnchanged, again->close());
}

// ---- Detached public snapshot: owned amounts, retained activity, cursors. ----
//
// Local records below are synthetic in-memory wallet state injected through
// the test seam; they exercise only the snapshot reader. Every test removes
// them (clear_local or an explicit clear) before any checkpoint, save or close
// that could persist them. Unlock-time extras follow the documented scalar or
// per-output encoding, one encoding per helper, so each reader path is reached.

using wti_t = tools::wallet_public::wallet_transfer_info;
using leg_t = tools::wallet_public::wallet_sub_transfer_info;
// A reference: the core constant may be dynamically initialized elsewhere.
const crypto::public_key& native_id = currency::native_coin_asset_id;

crypto::hash tx_id(uint32_t n)
{
  crypto::hash id{};
  auto* bytes = reinterpret_cast<unsigned char*>(&id);
  bytes[0] = static_cast<unsigned char>(n >> 24);
  bytes[1] = static_cast<unsigned char>(n >> 16);
  bytes[2] = static_cast<unsigned char>(n >> 8);
  bytes[3] = static_cast<unsigned char>(n);
  bytes[31] = 0xa5;
  return id;
}

crypto::public_key asset_key(uint32_t n)
{
  crypto::public_key key{};
  auto* bytes = reinterpret_cast<unsigned char*>(&key);
  bytes[0] = static_cast<unsigned char>(n >> 24);
  bytes[1] = static_cast<unsigned char>(n >> 16);
  bytes[2] = static_cast<unsigned char>(n >> 8);
  bytes[3] = static_cast<unsigned char>(n);
  bytes[31] = 0x5a;
  require(key != currency::native_coin_asset_id && key != currency::null_pkey);
  return key;
}

// Post-HF4 shape: hidden input/outputs, explicit fee entry.
currency::transaction hidden_tx(uint64_t fee, size_t outs = 2, bool with_fee = true)
{
  currency::transaction tx{};
  tx.version = TRANSACTION_VERSION_POST_HF4;
  tx.vin.emplace_back(currency::txin_zc_input{});
  for (size_t i = 0; i != outs; ++i) tx.vout.emplace_back(currency::tx_out_zarcanum{});
  if (with_fee) tx.extra.emplace_back(currency::zarcanum_tx_data_v1{fee});
  return tx;
}

std::shared_ptr<tools::transaction_wallet_info> wallet_tx(uint64_t height, size_t outs, uint64_t unlock_time = 0)
{
  auto info = std::make_shared<tools::transaction_wallet_info>();
  info->m_block_height = height;
  info->m_tx = hidden_tx(0, outs);
  // Scalar encoding only: one value for every output.
  if (unlock_time != 0)
    info->m_tx.extra.emplace_back(currency::etc_tx_details_unlock_time{unlock_time});
  return info;
}

// Per-output encoding only: one value per output, no scalar entry.
std::shared_ptr<tools::transaction_wallet_info> per_output_tx(uint64_t height, std::vector<uint64_t> times)
{
  auto info = std::make_shared<tools::transaction_wallet_info>();
  info->m_block_height = height;
  info->m_tx = hidden_tx(0, times.size());
  currency::etc_tx_details_unlock_time2 per_output{};
  per_output.unlock_time_array = std::move(times);
  info->m_tx.extra.emplace_back(per_output);
  return info;
}

tools::transfer_details output(const std::shared_ptr<tools::transaction_wallet_info>& info, uint64_t index,
  uint64_t amount, const crypto::public_key& asset, uint32_t flags = 0)
{
  tools::transfer_details td{};
  td.m_ptx_wallet_info = info;
  td.m_internal_output_index = index;
  td.m_amount = amount;
  td.m_flags = flags;
  td.m_zc_info_ptr = std::make_shared<tools::transfer_details_base::ZC_out_info>();
  td.m_zc_info_ptr->asset_id = asset;
  return td;
}

leg_t leg(const crypto::public_key& asset, bool income, uint64_t amount)
{
  leg_t result{};
  result.asset_id = asset;
  result.is_income = income;
  result.amount = amount;
  return result;
}

tools::wallet_public::employed_tx_entry received(const crypto::public_key& asset, uint64_t amount)
{
  tools::wallet_public::employed_tx_entry entry{};
  entry.asset_id = asset;
  entry.amount = amount;
  return entry;
}

// One default payment-id group holding `legs`; fee is both the recorded wallet
// fee and the explicit fee entry of the (synthetic) transaction.
wti_t record(const crypto::hash& id, uint64_t timestamp, uint64_t height, std::vector<leg_t> legs,
  uint64_t fee = 0)
{
  wti_t wti{};
  wti.tx_hash = id;
  wti.timestamp = timestamp;
  wti.height = height;
  wti.fee = fee;
  wti.tx = hidden_tx(fee);
  if (!legs.empty())
  {
    tools::wallet_public::wallet_sub_transfers_by_pid_info group{};
    group.subtransfers = std::move(legs);
    wti.subtransfers_by_pid.push_back(std::move(group));
  }
  return wti;
}

void add_pending(tools::wallet2& w, wti_t wti)
{
  const auto id = wti.tx_hash;
  w.m_unconfirmed_txs[id] = std::move(wti);
}

void clear_local(tools::wallet2& w)
{
  w.m_transfers.clear();
  w.m_unconfirmed_txs.clear();
  w.m_transfer_history.clear();
  w.m_custom_assets.clear();
}

// Synthetic local chain index entries (never compared with a node here).
void extend_local_chain(tools::wallet2& w, uint64_t blocks)
{
  for (uint64_t h = w.get_blockchain_current_size(); h < blocks; ++h)
    w.m_chain.push_new_block_id(tx_id(0x100000u + static_cast<uint32_t>(h)), h);
}

uint64_t clock_value = 0;
size_t clock_reads = 0;
uint64_t test_clock()
{
  ++clock_reads;
  return clock_value;
}
void use_test_clock(tools::wallet2& w, uint64_t now)
{
  clock_value = now;
  w.get_core_runtime_config().get_core_time = &test_clock;
}

snapshot_result first_page(file_session& session, size_t page = default_activity_page)
{
  snapshot_request request;
  request.page_size = page;
  return session.capture_snapshot(request);
}

snapshot_result continue_page(file_session& session, const activity_cursor& cursor,
  size_t page = default_activity_page)
{
  snapshot_request request;
  request.page_size = page;
  request.continuation = true;
  request.cursor = cursor;
  return session.capture_snapshot(request);
}

void expect_no_payload(const snapshot_result& result, snapshot_status expected)
{
  EXPECT_EQ(expected, result.status);
  EXPECT_TRUE(result.observation == nullptr);
}

const public_snapshot& available(const snapshot_result& result)
{
  require(result.status == snapshot_status::Available && result.observation != nullptr);
  return *result.observation;
}

const asset_amounts& amounts_of(const public_snapshot& s, const crypto::public_key& id)
{
  for (const auto& entry : s.assets)
    if (entry.asset_id == id) return entry;
  throw std::runtime_error("native snapshot asset missing");
}

void expect_amounts(const asset_amounts& a, uint64_t total, uint64_t unlocked, uint64_t in, uint64_t out)
{
  EXPECT_EQ(total, a.raw_total);
  EXPECT_EQ(unlocked, a.raw_unlocked);
  EXPECT_EQ(in, a.raw_awaiting_in);
  EXPECT_EQ(out, a.raw_awaiting_out);
}

bool same_assets(const public_snapshot& a, const public_snapshot& b)
{
  if (a.assets.size() != b.assets.size()) return false;
  for (size_t i = 0; i != a.assets.size(); ++i)
  {
    const auto& x = a.assets[i];
    const auto& y = b.assets[i];
    if (x.asset_id != y.asset_id || x.raw_total != y.raw_total || x.raw_unlocked != y.raw_unlocked ||
        x.raw_awaiting_in != y.raw_awaiting_in || x.raw_awaiting_out != y.raw_awaiting_out ||
        x.decimals_known != y.decimals_known || x.decimals != y.decimals)
      return false;
  }
  return true;
}

bool bytes_ascending(const public_snapshot& s)
{
  for (size_t i = 1; i < s.assets.size(); ++i)
    if (std::memcmp(&s.assets[i - 1].asset_id, &s.assets[i].asset_id, sizeof(crypto::public_key)) >= 0)
      return false;
  return true;
}

std::vector<crypto::hash> row_ids(const public_snapshot& s)
{
  std::vector<crypto::hash> ids;
  for (const auto& row : s.activity) ids.push_back(row.tx_hash);
  return ids;
}

NATIVE_SESSION_TEST(snapshot_of_a_valid_zero_wallet_is_available_with_an_explicit_native_zero)
{
  fixture f;
  const auto tree = snapshot(f.root);
  const auto c = f.control(f.input);
  auto session = open_session(f, c);
  const auto result = first_page(*session);
  ASSERT_EQ(snapshot_status::Available, result.status);
  ASSERT_TRUE(result.observation != nullptr);
  const auto& s = *result.observation;
  EXPECT_TRUE(same_identity(s.identity, f.input.expected));
  EXPECT_EQ(build_profile().network, s.network);
  EXPECT_EQ(build_profile().formation, s.formation);
  EXPECT_TRUE(s.genesis == build_profile().genesis);
  EXPECT_NE(0u, s.session_generation);
  EXPECT_EQ(0u, s.revision);
  EXPECT_EQ(1u, s.local_blocks);
  EXPECT_EQ(0u, s.local_tip_height);
  // Genesis only: no recent tip hash is invented, not even the genesis.
  EXPECT_FALSE(s.local_tip_hash_known);
  EXPECT_TRUE(s.local_tip_hash == currency::null_hash);
  ASSERT_EQ(1u, s.assets.size());
  EXPECT_TRUE(s.assets.front().asset_id == native_id);
  expect_amounts(s.assets.front(), 0, 0, 0, 0);
  EXPECT_TRUE(s.assets.front().decimals_known);
  EXPECT_EQ(static_cast<unsigned>(CURRENCY_DISPLAY_DECIMAL_POINT), static_cast<unsigned>(s.assets.front().decimals));
  EXPECT_TRUE(s.activity.empty());
  EXPECT_EQ(0u, s.coverage.retained_rows);
  EXPECT_FALSE(s.coverage.more_rows);
  // An empty retained page is not complete history, and the pool is unseen.
  EXPECT_TRUE(s.coverage.retained_local_only);
  EXPECT_FALSE(s.coverage.history_completeness_known);
  EXPECT_FALSE(s.coverage.pool_observed);
  EXPECT_EQ(session_state::OpenUnchanged, session->state());
  EXPECT_EQ(receive_status::Available, session->get_receive().status);
  EXPECT_TRUE(tree == snapshot(f.root));
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  expect_no_payload(first_page(*session), snapshot_status::Unavailable);
  expect_guarded_calls(c);
}

NATIVE_SESSION_TEST(snapshot_final_authorization_sample_never_publishes_and_revocation_is_sticky)
{
  fixture f;
  const auto tree = snapshot(f.root);
  {
    const auto c = f.control(f.input);
    auto session = open_session(f, c);
    c->authorized = false;
    expect_no_payload(first_page(*session), snapshot_status::Revoked);
    c->authorized = true;
    expect_no_payload(first_page(*session), snapshot_status::Revoked);
    EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  }
  const auto c = f.control(f.input);
  auto session = open_session(f, c);
  // The first sample admits the build; the sample immediately before
  // publication revokes. The complete observation is discarded.
  c->revoke_at_authorization_read = c->authorization_reads + 2;
  expect_no_payload(first_page(*session), snapshot_status::Revoked);
  EXPECT_FALSE(c->authorized);
  c->authorized = true; // A provider reporting current again restores nothing.
  expect_no_payload(first_page(*session), snapshot_status::Revoked);
  EXPECT_EQ(receive_status::Unavailable, session->get_receive().status);
  EXPECT_EQ(session_state::OpenUnchanged, session->state());
  EXPECT_EQ(0u, c->releases);
  EXPECT_TRUE(c->storage->held);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  EXPECT_TRUE(tree == snapshot(f.root));
  expect_guarded_calls(c);
}

NATIVE_SESSION_TEST(snapshot_inside_a_step_or_checkpoint_is_busy_and_changes_nothing)
{
  fixture f;
  const test_chain chain;
  const auto c = capable(f);
  c->writer = writer_mode::Reject;
  auto session = open_session(f, c);
  const auto transport = std::make_shared<controlled_transport>();
  ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
  std::atomic<bool> stop{false};
  snapshot_status nested = snapshot_status::Available;
  bool nested_payload = true;
  const auto reenter = [&]
  {
    const auto inner = first_page(*session);
    nested = inner.status;
    nested_payload = inner.observation != nullptr;
  };
  transport->on_request = reenter;
  serve(*transport, 0, 2, {chain.g, chain.b1});
  EXPECT_EQ(tools::read_only::step_status::Progress, step(*session, stop).status);
  transport->on_request = nullptr;
  EXPECT_EQ(snapshot_status::Busy, nested);
  EXPECT_FALSE(nested_payload);

  nested = snapshot_status::Available;
  nested_payload = true;
  c->on_checkpoint = reenter;
  EXPECT_EQ(checkpoint_result::Rejected, session->checkpoint());
  c->on_checkpoint = nullptr;
  EXPECT_EQ(snapshot_status::Busy, nested);
  EXPECT_FALSE(nested_payload);

  // Outside the callbacks the dirty session is observable and stays dirty.
  const auto after = first_page(*session);
  const auto& s = available(after);
  EXPECT_EQ(2u, s.local_blocks);
  EXPECT_EQ(1u, s.local_tip_height);
  EXPECT_TRUE(s.local_tip_hash_known);
  EXPECT_TRUE(s.local_tip_hash == currency::get_block_hash(chain.b1.block_ptr->bl));
  EXPECT_EQ(1u, s.revision);
  EXPECT_EQ(session_state::OpenDirty, session->state());
  EXPECT_EQ(1u, c->checkpoints);
  EXPECT_EQ(1u, transport->requests);
  EXPECT_EQ(0u, transport->forbidden);
  EXPECT_TRUE(f.baseline == read(f.path));
  session.reset(); // Test teardown only; not persistence evidence.
}

NATIVE_SESSION_TEST(snapshot_refuses_recovery_required_and_outcome_unknown_sessions)
{
  const test_chain chain;
  {
    fixture f;
    const auto c = capable(f);
    auto session = open_session(f, c);
    const auto transport = std::make_shared<controlled_transport>();
    ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
    std::atomic<bool> stop{false};
    serve(*transport, 0, 2, {chain.g, chain.b1});
    EXPECT_EQ(tools::read_only::step_status::Progress, step(*session, stop).status);
    auto malformed = child(chain.b1, 0, false);
    {
      auto block = std::make_shared<currency::block_extended_info>(*malformed.block_ptr);
      block->bl.miner_tx.extra.emplace_back(crypto::public_key{});
      block->bl.miner_tx.extra.emplace_back(crypto::public_key{});
      malformed.block_ptr = block;
    }
    serve(*transport, 1, 3, {chain.b1, malformed});
    EXPECT_EQ(tools::read_only::step_status::RecoveryRequired, step(*session, stop).status);
    expect_no_payload(first_page(*session), snapshot_status::RecoveryRequired);
    EXPECT_EQ(close_result::RecoveryRequired, session->close());
    EXPECT_EQ(0u, c->checkpoints);
    session.reset();
  }
  {
    fixture f;
    const auto c = capable(f);
    c->writer = writer_mode::Throw;
    auto session = open_session(f, c);
    const auto transport = std::make_shared<controlled_transport>();
    ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
    std::atomic<bool> stop{false};
    serve(*transport, 0, 2, {chain.g, chain.b1});
    EXPECT_EQ(tools::read_only::step_status::Progress, step(*session, stop).status);
    EXPECT_EQ(checkpoint_result::OutcomeUnknown, session->checkpoint());
    expect_no_payload(first_page(*session), snapshot_status::OutcomeUnknown);
    EXPECT_EQ(close_result::OutcomeUnknown, session->close());
    EXPECT_EQ(1u, c->checkpoints);
    session.reset();
  }
}

NATIVE_SESSION_TEST(snapshot_freezes_one_core_time_for_unlocks_without_accumulation_or_side_effects)
{
  fixture f;
  const auto tree = snapshot(f.root);
  const auto c = capable(f);
  auto session = open_session(f, c);
  const auto transport = std::make_shared<controlled_transport>();
  ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
  auto& w = checkpoint_test_access::wallet(*session);
  extend_local_chain(w, 12);
  const uint64_t unlock_at = 1700000000;
  static_assert(1700000000 >= CURRENCY_MAX_BLOCK_NUMBER, "interpreted as a time");
  const auto timed = wallet_tx(1, 1, unlock_at);
  const auto plain = wallet_tx(1, 2);
  const auto recent = wallet_tx(11, 1);
  w.m_transfers[0] = output(timed, 0, 5 * COIN, native_id);
  w.m_transfers[1] = output(plain, 0, 7 * COIN, native_id);
  w.m_transfers[2] = output(recent, 0, COIN, native_id);  // Matures at 21 > 12 local blocks.
  w.m_transfers[3] = output(plain, 1, 100 * COIN, native_id, WALLET_TRANSFER_DETAIL_FLAG_SPENT);
  w.m_transfer_history.push_back(record(tx_id(1), 100, 1, {leg(native_id, true, 5 * COIN)}));
  w.m_transfer_history.push_back(record(tx_id(2), 200, 1, {leg(native_id, true, 7 * COIN)}));
  const auto bare_cache = w.m_has_bare_unspent_outputs;
  const auto whitelist = w.m_whitelisted_assets.size();
  const uint64_t boundary = unlock_at - CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS;

  use_test_clock(w, boundary - 1);
  const size_t reads = clock_reads;
  const auto locked = first_page(*session, 1);
  const auto& before = available(locked);
  EXPECT_EQ(reads + 1, clock_reads);
  EXPECT_EQ(boundary - 1, before.captured_at);
  expect_amounts(amounts_of(before, native_id), 13 * COIN, 7 * COIN, 0, 0);
  ASSERT_TRUE(before.coverage.more_rows);

  // The continuation reuses the first page's frozen time, not the moved clock.
  clock_value = boundary;
  const auto continued = continue_page(*session, before.coverage.next, 1);
  const auto& later = available(continued);
  EXPECT_EQ(reads + 1, clock_reads);
  EXPECT_EQ(boundary - 1, later.captured_at);
  expect_amounts(amounts_of(later, native_id), 13 * COIN, 7 * COIN, 0, 0);
  EXPECT_FALSE(later.coverage.more_rows);

  // A new first page samples the clock once, exactly at the boundary.
  const auto at = first_page(*session, 1);
  const auto& unlocked = available(at);
  EXPECT_EQ(reads + 2, clock_reads);
  EXPECT_EQ(boundary, unlocked.captured_at);
  expect_amounts(amounts_of(unlocked, native_id), 13 * COIN, 12 * COIN, 0, 0);
  // Repeated captures neither accumulate nor drift.
  const auto repeated = first_page(*session, 1);
  EXPECT_TRUE(same_assets(unlocked, available(repeated)));
  EXPECT_TRUE(same_assets(unlocked, available(first_page(*session, 1))));

  // No cache, proxy, whitelist, persistence or state side effect.
  EXPECT_TRUE(bare_cache == w.m_has_bare_unspent_outputs);
  EXPECT_EQ(whitelist, w.m_whitelisted_assets.size());
  EXPECT_EQ(0u, transport->requests);
  EXPECT_EQ(0u, transport->forbidden);
  EXPECT_EQ(0u, c->checkpoints);
  EXPECT_EQ(session_state::OpenUnchanged, session->state());
  EXPECT_TRUE(tree == snapshot(f.root));
  clear_local(w);
  EXPECT_EQ(close_result::ReleasedUnchanged, session->close());
}

NATIVE_SESSION_TEST(snapshot_amounts_classify_each_pending_record_independently)
{
  fixture f;
  const auto c = f.control(f.input);
  auto session = open_session(f, c);
  auto& w = checkpoint_test_access::wallet(*session);
  extend_local_chain(w, 12);
  use_test_clock(w, 1);
  const auto x = asset_key(0x10), unknown = asset_key(0x20), custom = asset_key(0x30);
  const uint64_t fee = COIN / 100;
  const auto confirmed = wallet_tx(1, 5);
  w.m_transfers[0] = output(confirmed, 0, 10 * COIN, native_id, WALLET_TRANSFER_DETAIL_FLAG_SPENT); // Spent input.
  w.m_transfers[1] = output(confirmed, 1, COIN, native_id);
  w.m_transfers[2] = output(confirmed, 2, 20, x);
  w.m_transfers[3] = output(confirmed, 3, 3, unknown);
  w.m_transfers[4] = output(confirmed, 4, 4, custom);
  w.m_custom_assets[custom].decimal_point = 6;

  // Ordinary incoming native.
  auto incoming = record(tx_id(0xa), 0, 0, {leg(native_id, true, 2 * COIN)});
  incoming.employed_entries.receive.push_back(received(native_id, 2 * COIN));
  add_pending(w, incoming);
  // Native send: legs are amount + fee; change 6.99.
  auto send = record(tx_id(0xb), 0, 0, {leg(native_id, false, 3 * COIN + fee)}, fee);
  send.employed_entries.receive.push_back(received(native_id, 6 * COIN + 99 * fee));
  add_pending(w, send);
  // Asset send paying the native fee: asset change 2, native change 0.99.
  auto asset_send = record(tx_id(0xc), 0, 0, {leg(x, false, 5), leg(native_id, false, fee)}, fee);
  asset_send.employed_entries.receive.push_back(received(x, 2));
  asset_send.employed_entries.receive.push_back(received(native_id, 99 * fee));
  add_pending(w, asset_send);
  // Incoming of the same asset in another record.
  auto asset_in = record(tx_id(0xd), 0, 0, {leg(x, true, 4)});
  asset_in.employed_entries.receive.push_back(received(x, 4));
  add_pending(w, asset_in);
  // Self-consolidation: no asset leg, only the fee; the asset receive entry is
  // change. A direction map shared with the incoming record would drop it.
  auto consolidation = record(tx_id(0xe), 0, 0, {leg(native_id, false, fee)}, fee);
  consolidation.employed_entries.receive.push_back(received(x, 7));
  add_pending(w, consolidation);

  const auto bare_cache = w.m_has_bare_unspent_outputs;
  const auto first = first_page(*session);
  const auto& s = available(first);
  ASSERT_EQ(4u, s.assets.size());
  EXPECT_TRUE(bytes_ascending(s));
  const auto& native = amounts_of(s, native_id);
  expect_amounts(native, COIN + 2 * COIN + 6 * COIN + 99 * fee + 99 * fee, COIN, 2 * COIN, 3 * COIN);
  EXPECT_TRUE(native.decimals_known);
  const auto& held = amounts_of(s, x);
  expect_amounts(held, 20 + 2 + 4 + 7, 20, 4, 5);
  EXPECT_FALSE(held.decimals_known);
  const auto& unknown_precision = amounts_of(s, unknown);
  expect_amounts(unknown_precision, 3, 3, 0, 0);
  EXPECT_FALSE(unknown_precision.decimals_known); // Never defaulted, never filtered.
  EXPECT_EQ(0u, static_cast<unsigned>(unknown_precision.decimals));
  const auto& known = amounts_of(s, custom);
  EXPECT_TRUE(known.decimals_known);
  EXPECT_EQ(6u, static_cast<unsigned>(known.decimals));
  EXPECT_EQ(5u, s.activity.size());
  for (const auto& row : s.activity)
  {
    EXPECT_EQ(activity_status::RetainedPending, row.status);
  }
  // Deterministic and non-accumulating across captures.
  EXPECT_TRUE(same_assets(s, available(first_page(*session))));
  EXPECT_TRUE(bare_cache == w.m_has_bare_unspent_outputs);
  clear_local(w);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_SESSION_TEST(snapshot_pending_native_fee_is_derived_once_per_record_and_independent_of_order)
{
  fixture f;
  const auto c = f.control(f.input);
  auto session = open_session(f, c);
  auto& w = checkpoint_test_access::wallet(*session);
  extend_local_chain(w, 12);
  use_test_clock(w, 1);
  const auto x = asset_key(0x10);
  const uint64_t fee = COIN / 100;

  // Two outgoing native legs in different payment-id groups: summed, then
  // the record's one derived fee is subtracted once.
  auto split = record(tx_id(0x71), 0, 0, {leg(native_id, false, 2 * COIN)}, fee);
  {
    tools::wallet_public::wallet_sub_transfers_by_pid_info other{};
    other.payment_id = "pid";
    other.subtransfers.push_back(leg(native_id, false, COIN + fee));
    split.subtransfers_by_pid.push_back(other);
  }
  add_pending(w, split);
  {
    const auto once = first_page(*session);
    expect_amounts(amounts_of(available(once), native_id), 0, 0, 0, 3 * COIN);
  }
  clear_local(w);

  // The recorded activity fee (7) is not the derived fee (the explicit entry):
  // amounts use only the derived fee, the row reports only the recorded one.
  auto differing = record(tx_id(0x72), 0, 0, {leg(native_id, false, 5 * COIN + fee)}, 7);
  differing.tx = hidden_tx(fee);
  add_pending(w, differing);
  {
    const auto shown = first_page(*session);
    const auto& s = available(shown);
    expect_amounts(amounts_of(s, native_id), 0, 0, 0, 5 * COIN);
    ASSERT_EQ(1u, s.activity.size());
    EXPECT_EQ(fee_record::WalletRecorded, s.activity[0].fee_source);
    EXPECT_EQ(7u, s.activity[0].recorded_fee);
    ASSERT_EQ(1u, s.activity[0].legs.size());
    EXPECT_EQ(5 * COIN + fee, s.activity[0].legs[0].amount); // Leg as recorded, not netted.
  }
  clear_local(w);

  // Equivalent records inserted in opposite orders give equal amounts/rows.
  auto incoming = record(tx_id(0x73), 0, 0, {leg(x, true, 9)});
  incoming.employed_entries.receive.push_back(received(x, 9));
  const std::vector<wti_t> records = {split, differing, incoming};
  for (const auto& r : records) add_pending(w, r);
  const auto forward = first_page(*session);
  clear_local(w);
  for (auto it = records.rbegin(); it != records.rend(); ++it) add_pending(w, *it);
  const auto reverse = first_page(*session);
  const auto& a = available(forward);
  const auto& b = available(reverse);
  EXPECT_TRUE(same_assets(a, b));
  EXPECT_TRUE(row_ids(a) == row_ids(b));
  expect_amounts(amounts_of(a, native_id), 0, 0, 0, 8 * COIN);
  expect_amounts(amounts_of(a, x), 9, 0, 9, 0);
  clear_local(w);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_SESSION_TEST(snapshot_unlock_boundaries_reach_maturity_scalar_and_per_output_paths)
{
  fixture f;
  const auto c = f.control(f.input);
  auto session = open_session(f, c);
  auto& w = checkpoint_test_access::wallet(*session);
  const uint64_t local_blocks = 12;
  extend_local_chain(w, local_blocks);
  const uint64_t now = 1700000000;
  static_assert(1700000000 >= CURRENCY_MAX_BLOCK_NUMBER, "interpreted as a time");
  use_test_clock(w, now);
  const uint64_t age = WALLET_DEFAULT_TX_SPENDABLE_AGE;
  require(age < local_blocks);
  // Highest unlocking values: block index and captured core time.
  const uint64_t block_limit = local_blocks - 1 + CURRENCY_LOCKED_TX_ALLOWED_DELTA_BLOCKS;
  require(block_limit + 1 < CURRENCY_MAX_BLOCK_NUMBER);
  const uint64_t time_limit = now + CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS;

  // One asset per case, so each decision is observed on its own.
  const auto mature = asset_key(0x61), immature = asset_key(0x62);
  const auto scalar_at = asset_key(0x63), scalar_above = asset_key(0x64);
  const auto array_block_at = asset_key(0x65), array_block_above = asset_key(0x66);
  const auto array_time_at = asset_key(0x67), array_time_above = asset_key(0x68);
  const auto zero_scalar = asset_key(0x69), scalar_first = asset_key(0x6a);
  // Maturity: height + age must not exceed the local block count.
  w.m_transfers[0] = output(wallet_tx(local_blocks - age, 1), 0, 1, mature);
  w.m_transfers[1] = output(wallet_tx(local_blocks - age + 1, 1), 0, 2, immature);
  // Scalar block-index unlock.
  w.m_transfers[2] = output(wallet_tx(1, 1, block_limit), 0, 3, scalar_at);
  w.m_transfers[3] = output(wallet_tx(1, 1, block_limit + 1), 0, 4, scalar_above);
  // Per-output arrays: each output decides alone, by block index or by time.
  const auto blocks = per_output_tx(1, {block_limit, block_limit + 1});
  w.m_transfers[4] = output(blocks, 0, 5, array_block_at);
  w.m_transfers[5] = output(blocks, 1, 6, array_block_above);
  const auto times = per_output_tx(1, {time_limit + 1, time_limit});
  w.m_transfers[6] = output(times, 1, 7, array_time_at);
  w.m_transfers[7] = output(times, 0, 8, array_time_above);
  {
    // A zero first scalar falls through to the FIRST array; a later scalar
    // and a later array are ignored.
    const auto tx = per_output_tx(1, {block_limit});
    auto& extra = tx->m_tx.extra;
    extra.insert(extra.begin(), currency::etc_tx_details_unlock_time{0});
    extra.emplace_back(currency::etc_tx_details_unlock_time{time_limit + 1});
    currency::etc_tx_details_unlock_time2 later{};
    later.unlock_time_array.assign(1, time_limit + 1);
    extra.emplace_back(later);
    w.m_transfers[8] = output(tx, 0, 9, zero_scalar);
  }
  {
    // A nonzero first scalar wins over any array.
    const auto tx = wallet_tx(1, 1, block_limit + 1);
    currency::etc_tx_details_unlock_time2 unlocking{};
    unlocking.unlock_time_array.assign(1, 0);
    tx->m_tx.extra.emplace_back(unlocking);
    w.m_transfers[9] = output(tx, 0, 10, scalar_first);
  }

  const auto result = first_page(*session);
  const auto& s = available(result);
  EXPECT_EQ(now, s.captured_at);
  expect_amounts(amounts_of(s, mature), 1, 1, 0, 0);
  expect_amounts(amounts_of(s, immature), 2, 0, 0, 0);
  expect_amounts(amounts_of(s, scalar_at), 3, 3, 0, 0);
  expect_amounts(amounts_of(s, scalar_above), 4, 0, 0, 0);
  expect_amounts(amounts_of(s, array_block_at), 5, 5, 0, 0);
  expect_amounts(amounts_of(s, array_block_above), 6, 0, 0, 0);
  expect_amounts(amounts_of(s, array_time_at), 7, 7, 0, 0);
  expect_amounts(amounts_of(s, array_time_above), 8, 0, 0, 0);
  expect_amounts(amounts_of(s, zero_scalar), 9, 9, 0, 0);
  expect_amounts(amounts_of(s, scalar_first), 10, 0, 0, 0);
  clear_local(w);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_SESSION_TEST(snapshot_unlock_reader_charges_extras_and_arrays_to_the_shared_nested_budget)
{
  fixture f;
  const auto c = f.control(f.input);
  auto session = open_session(f, c);
  auto& w = checkpoint_test_access::wallet(*session);
  extend_local_chain(w, 12);
  use_test_clock(w, 1);
  const size_t limit = max_snapshot_nested_elements;
  const auto refused = [&](const char* label)
  {
    SCOPED_TRACE(label);
    // Deterministic: the same state is refused the same way, never cut.
    expect_no_payload(first_page(*session), snapshot_status::LimitExceeded);
    expect_no_payload(first_page(*session), snapshot_status::LimitExceeded);
    EXPECT_EQ(session_state::OpenUnchanged, session->state());
  };
  {
    // Nonzero first scalar: only the traversed extras are charged.
    const auto tx = wallet_tx(1, 3, 5);
    auto& extra = tx->m_tx.extra;
    extra.resize(limit, currency::extra_v(crypto::public_key{}));
    w.m_transfers[0] = output(tx, 0, 1, native_id);
    const auto fits = first_page(*session);
    expect_amounts(amounts_of(available(fits), native_id), 1, 1, 0, 0);
    extra.emplace_back(crypto::public_key{});
    refused("oversized extras");

    // Outputs sharing one transaction each pay for its extras again.
    extra.resize(limit / 2);
    w.m_transfers[1] = output(tx, 1, 2, native_id);
    const auto shared = first_page(*session);
    expect_amounts(amounts_of(available(shared), native_id), 3, 3, 0, 0);
    w.m_transfers[2] = output(tx, 2, 4, native_id);
    refused("shared-output repeated extras");
    clear_local(w);
  }
  {
    // Zero first scalar: the first per-output array is referenced and
    // charged in full before it is indexed.
    const auto tx = per_output_tx(1, {0, 0});
    auto& extra = tx->m_tx.extra;
    extra.insert(extra.begin(), currency::etc_tx_details_unlock_time{0});
    auto& times = boost::get<currency::etc_tx_details_unlock_time2>(extra.back()).unlock_time_array;
    times.resize(limit - extra.size(), 0);
    w.m_transfers[0] = output(tx, 1, 1, native_id);
    const auto fits = first_page(*session);
    expect_amounts(amounts_of(available(fits), native_id), 1, 1, 0, 0);
    times.push_back(0);
    refused("oversized per-output unlock array");
    clear_local(w);
  }
  EXPECT_EQ(snapshot_status::Available, first_page(*session).status);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_SESSION_TEST(snapshot_after_a_deep_detach_reports_an_unknown_tip_not_invalid_data)
{
  fixture f;
  const auto c = f.control(f.input);
  auto session = open_session(f, c);
  auto& w = checkpoint_test_access::wallet(*session);
  use_test_clock(w, 1);
  const auto local_id = [](uint64_t h) { return tx_id(0x100000u + static_cast<uint32_t>(h)); };
  extend_local_chain(w, 110); // Recent window 90..109, sparse every 10th.
  {
    const auto before = first_page(*session);
    const auto& s = available(before);
    EXPECT_EQ(109u, s.local_tip_height);
    EXPECT_TRUE(s.local_tip_hash_known);
    EXPECT_TRUE(s.local_tip_hash == local_id(109));
  }
  // Detach 39 blocks below the recent window: every recent entry goes, and
  // the exact sparse entry at the new tip 70 survives.
  w.m_chain.detach(71);
  std::pair<uint64_t, crypto::hash> sparse{};
  ASSERT_TRUE(w.m_chain.lookup_item_around(70, sparse));
  EXPECT_EQ(70u, sparse.first);
  EXPECT_TRUE(w.m_chain.get_top_block_id() == currency::null_hash);
  {
    const auto aligned = first_page(*session);
    const auto& s = available(aligned);
    EXPECT_EQ(71u, s.local_blocks);
    EXPECT_EQ(70u, s.local_tip_height);
    EXPECT_FALSE(s.local_tip_hash_known);
    EXPECT_TRUE(s.local_tip_hash == currency::null_hash);
  }
  // A tip that is not a multiple of ten has no exact sparse entry either.
  w.m_chain.detach(66);
  {
    const auto unaligned = first_page(*session);
    const auto& s = available(unaligned);
    EXPECT_EQ(66u, s.local_blocks);
    EXPECT_EQ(65u, s.local_tip_height);
    EXPECT_FALSE(s.local_tip_hash_known);
    EXPECT_TRUE(s.local_tip_hash == currency::null_hash);
  }
  // A null exact sparse identity is still malformed, never a tip.
  w.m_chain.clear();
  for (uint64_t h = 1; h != 110; ++h)
    w.m_chain.push_new_block_id(h == 70 ? currency::null_hash : local_id(h), h);
  EXPECT_TRUE(available(first_page(*session)).local_tip_hash == local_id(109));
  w.m_chain.detach(71);
  expect_no_payload(first_page(*session), snapshot_status::InvalidData);
  EXPECT_EQ(session_state::OpenUnchanged, session->state());
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_SESSION_TEST(snapshot_refuses_overflow_underflow_malformed_and_ambiguous_records_without_payload)
{
  fixture f;
  const auto c = f.control(f.input);
  auto session = open_session(f, c);
  auto& w = checkpoint_test_access::wallet(*session);
  extend_local_chain(w, 12);
  use_test_clock(w, 1);
  const auto x = asset_key(0x10);
  const uint64_t max = std::numeric_limits<uint64_t>::max();
  const auto tx = wallet_tx(1, 2);
  const auto refused = [&](snapshot_status expected, const char* label)
  {
    SCOPED_TRACE(label);
    expect_no_payload(first_page(*session), expected);
    EXPECT_EQ(session_state::OpenUnchanged, session->state());
    clear_local(w);
    EXPECT_EQ(snapshot_status::Available, first_page(*session).status);
  };

  w.m_transfers[0] = output(tx, 0, max, native_id);
  w.m_transfers[1] = output(tx, 1, 1, native_id);
  refused(snapshot_status::InvalidData, "confirmed total overflow");

  w.m_transfers[0] = output(tx, 0, 1, native_id);
  add_pending(w, record(tx_id(1), 0, 0, {leg(native_id, true, max)}));
  refused(snapshot_status::InvalidData, "pending incoming overflow");

  add_pending(w, record(tx_id(1), 0, 0, {leg(x, false, max)}));
  add_pending(w, record(tx_id(2), 0, 0, {leg(x, false, 1)}));
  refused(snapshot_status::InvalidData, "awaiting-out overflow across records");

  add_pending(w, record(tx_id(1), 0, 0, {leg(native_id, false, 1)}, 2));
  refused(snapshot_status::InvalidData, "native outgoing below the derived fee");

  {
    auto no_fee = record(tx_id(1), 0, 0, {leg(native_id, false, 5)});
    no_fee.tx.extra.clear();
    add_pending(w, no_fee);
    refused(snapshot_status::InvalidData, "post-HF4 fee entry missing");
  }
  {
    auto split = record(tx_id(1), 0, 0, {leg(native_id, false, 5)}, max);
    split.tx.extra.emplace_back(currency::zarcanum_tx_data_v1{1});
    add_pending(w, split);
    refused(snapshot_status::InvalidData, "fee entry sum overflow");
  }
  {
    // Pre-HF4 open amounts: fee = 10 - 3 = 7, so awaiting-out = 10 - 7.
    auto legacy = record(tx_id(1), 0, 0, {leg(native_id, false, 10)});
    legacy.tx = currency::transaction{};
    legacy.tx.version = TRANSACTION_VERSION_PRE_HF4;
    currency::txin_to_key in{};
    in.amount = 10;
    legacy.tx.vin.emplace_back(in);
    currency::tx_out_bare out{};
    out.amount = 3;
    legacy.tx.vout.emplace_back(out);
    add_pending(w, legacy);
    const auto accepted = first_page(*session);
    expect_amounts(amounts_of(available(accepted), native_id), 0, 0, 0, 3);
    clear_local(w);

    legacy.tx.vin.emplace_back(currency::txin_zc_input{});
    add_pending(w, legacy);
    refused(snapshot_status::InvalidData, "hidden input in a pre-HF4 fee shape");

    legacy.tx.vin.resize(1);
    boost::get<currency::tx_out_bare>(legacy.tx.vout.front()).amount = 11;
    add_pending(w, legacy);
    refused(snapshot_status::InvalidData, "pre-HF4 outputs exceed inputs");
  }
  {
    // A generated input has no fee.
    auto generated = record(tx_id(1), 0, 0, {leg(native_id, false, 5)});
    generated.tx = currency::transaction{};
    generated.tx.vin.emplace_back(currency::txin_gen{});
    add_pending(w, generated);
    const auto accepted = first_page(*session);
    expect_amounts(amounts_of(available(accepted), native_id), 0, 0, 0, 5);
    clear_local(w);
  }

  w.m_transfers[0] = output(tx, 2, 1, native_id);
  refused(snapshot_status::InvalidData, "output index outside the transaction");

  w.m_transfers[0] = output(wallet_tx(max - 5, 1), 0, 1, native_id);
  refused(snapshot_status::InvalidData, "maturity height overflow");

  add_pending(w, record(tx_id(1), 0, 0, {leg(currency::null_pkey, true, 1)}));
  refused(snapshot_status::InvalidData, "null asset leg");

  add_pending(w, record(tx_id(1), 0, 0, {leg(x, true, 1), leg(x, false, 1)}));
  refused(snapshot_status::UnsupportedData, "mixed directions in one group");
  {
    auto across = record(tx_id(1), 0, 0, {leg(x, true, 1)});
    tools::wallet_public::wallet_sub_transfers_by_pid_info other{};
    other.payment_id = "pid";
    other.subtransfers.push_back(leg(x, false, 1));
    across.subtransfers_by_pid.push_back(other);
    add_pending(w, across);
    refused(snapshot_status::UnsupportedData, "mixed directions across payment-id groups");
  }
  {
    auto escrow = record(tx_id(1), 0, 0, {leg(native_id, false, 5)}, 1);
    escrow.contract.emplace_back();
    add_pending(w, escrow);
    refused(snapshot_status::UnsupportedData, "escrow record");
  }
  {
    auto keyed = record(tx_id(1), 0, 0, {leg(native_id, true, 1)});
    w.m_unconfirmed_txs[tx_id(2)] = keyed;
    refused(snapshot_status::InvalidData, "pending key differs from its record");
  }
  add_pending(w, record(tx_id(1), 0, 3, {leg(native_id, true, 1)}));
  refused(snapshot_status::InvalidData, "pending record carrying a height");

  w.m_transfer_history.push_back(record(tx_id(1), 5, 12, {leg(native_id, true, 1)}));
  refused(snapshot_status::InvalidData, "confirmed height not below the local block count");

  w.m_transfer_history.push_back(record(currency::null_hash, 5, 1, {leg(native_id, true, 1)}));
  refused(snapshot_status::InvalidData, "null retained hash");

  {
    // Descriptor precision is uint8_t metadata that core admits without a
    // ceiling: 20 and 255 are reported as is, raw atomic amounts unchanged.
    w.m_transfers[0] = output(tx, 0, max, x);
    for (const unsigned precision : {20u, 255u})
    {
      SCOPED_TRACE(precision);
      w.m_custom_assets[x].decimal_point = static_cast<uint8_t>(precision);
      const auto accepted = first_page(*session);
      const auto& held = amounts_of(available(accepted), x);
      EXPECT_TRUE(held.decimals_known);
      EXPECT_EQ(precision, static_cast<unsigned>(held.decimals));
      expect_amounts(held, max, max, 0, 0);
    }
    clear_local(w);
  }

  add_pending(w, record(tx_id(1), 0, 0, {leg(native_id, true, 1)}));
  w.m_transfer_history.push_back(record(tx_id(1), 5, 1, {leg(native_id, true, 1)}));
  refused(snapshot_status::InvalidData, "duplicate identity across pending and confirmed");

  w.m_transfer_history.push_back(record(tx_id(3), 5, 1, {leg(native_id, true, 1)}));
  w.m_transfer_history.push_back(record(tx_id(3), 6, 2, {leg(native_id, true, 1)}));
  refused(snapshot_status::InvalidData, "duplicate confirmed identity");

  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_SESSION_TEST(snapshot_collection_page_and_nested_limits_fail_closed)
{
  fixture f;
  const auto c = f.control(f.input);
  auto session = open_session(f, c);
  auto& w = checkpoint_test_access::wallet(*session);
  extend_local_chain(w, 12);
  use_test_clock(w, 1);

  expect_no_payload(first_page(*session, 0), snapshot_status::InvalidRequest);
  expect_no_payload(first_page(*session, max_activity_page + 1), snapshot_status::InvalidRequest);
  EXPECT_EQ(snapshot_status::Available, first_page(*session, max_activity_page).status);
  expect_no_payload(continue_page(*session, activity_cursor{}), snapshot_status::InvalidRequest);

  {
    // 255 held assets plus native fill the inventory; one more is refused.
    const auto many = wallet_tx(11, max_snapshot_assets);
    for (uint32_t i = 0; i + 1 < max_snapshot_assets; ++i)
      w.m_transfers[i] = output(many, i, 1, asset_key(0x1000 + i));
    const auto full = first_page(*session);
    EXPECT_EQ(max_snapshot_assets, available(full).assets.size());
    w.m_transfers[max_snapshot_assets] = output(many, max_snapshot_assets - 1, 1, asset_key(0x9999));
    expect_no_payload(first_page(*session), snapshot_status::LimitExceeded);
    clear_local(w);
  }
  {
    const auto spent = output(wallet_tx(1, 1), 0, 1, native_id, WALLET_TRANSFER_DETAIL_FLAG_SPENT);
    for (uint64_t i = 0; i != max_snapshot_records; ++i)
      w.m_transfers.emplace_hint(w.m_transfers.end(), i, spent);
    EXPECT_EQ(snapshot_status::Available, first_page(*session).status);
    w.m_transfers.emplace_hint(w.m_transfers.end(), max_snapshot_records, spent);
    expect_no_payload(first_page(*session), snapshot_status::LimitExceeded);
    clear_local(w);
  }
  {
    // 128 legs over two payment-id groups are returned in recorded order.
    std::vector<leg_t> legs;
    for (uint32_t i = 0; i != max_activity_legs / 2; ++i) legs.push_back(leg(native_id, true, i + 1));
    auto wide = record(tx_id(1), 5, 1, legs);
    tools::wallet_public::wallet_sub_transfers_by_pid_info second{};
    second.payment_id = "pid";
    second.subtransfers = legs;
    wide.subtransfers_by_pid.push_back(second);
    w.m_transfer_history.push_back(wide);
    const auto accepted = first_page(*session);
    const auto& row = available(accepted).activity.at(0);
    ASSERT_EQ(max_activity_legs, row.legs.size());
    EXPECT_EQ(1u, row.legs.front().amount);
    EXPECT_EQ(max_activity_legs / 2, row.legs.back().amount);
    w.m_transfer_history.back().subtransfers_by_pid.back().subtransfers.push_back(leg(native_id, true, 1));
    expect_no_payload(first_page(*session), snapshot_status::LimitExceeded);
    clear_local(w);
  }
  {
    auto parties = record(tx_id(1), 5, 1, {leg(native_id, true, 1)});
    parties.remote_addresses.assign(max_activity_counterparties, std::string(max_counterparty_bytes, 'a'));
    w.m_transfer_history.push_back(parties);
    const auto accepted = first_page(*session);
    EXPECT_EQ(max_activity_counterparties, available(accepted).activity.at(0).counterparties.size());
    w.m_transfer_history.back().remote_addresses.push_back("b");
    expect_no_payload(first_page(*session), snapshot_status::LimitExceeded);
    w.m_transfer_history.back().remote_addresses.assign(1, std::string(max_counterparty_bytes + 1, 'a'));
    expect_no_payload(first_page(*session), snapshot_status::LimitExceeded);
    clear_local(w);
  }
  {
    // Aggregate nested bound over records, not a per-record allowance.
    auto a = record(tx_id(1), 0, 0, {});
    a.employed_entries.receive.assign(max_snapshot_nested_elements / 2, received(native_id, 0));
    auto b = record(tx_id(2), 0, 0, {});
    b.employed_entries.receive.assign(max_snapshot_nested_elements / 2, received(native_id, 0));
    add_pending(w, a);
    add_pending(w, b);
    EXPECT_EQ(snapshot_status::Available, first_page(*session).status);
    w.m_unconfirmed_txs[tx_id(2)].employed_entries.receive.push_back(received(native_id, 0));
    expect_no_payload(first_page(*session), snapshot_status::LimitExceeded);
    clear_local(w);
  }
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_SESSION_TEST(snapshot_activity_order_rows_and_cursors_are_deterministic_and_bound)
{
  fixture f;
  const auto c = f.control(f.input);
  auto session = open_session(f, c);
  auto& w = checkpoint_test_access::wallet(*session);
  extend_local_chain(w, 12);
  use_test_clock(w, 1);
  const auto x = asset_key(0x10);
  add_pending(w, record(tx_id(0x31), 500, 0, {leg(native_id, true, 1)}));
  add_pending(w, record(tx_id(0x32), 0, 0, {leg(native_id, true, 2)}));   // Unknown time.
  add_pending(w, record(tx_id(0x33), 900, 0, {leg(native_id, true, 3)}));
  {
    // Two payment-id groups, a recorded fee and recorded counterparties.
    auto rich = record(tx_id(0x41), 800, 5, {leg(native_id, false, 5), leg(x, false, 2)}, 7);
    tools::wallet_public::wallet_sub_transfers_by_pid_info second{};
    second.payment_id = "not exposed";
    second.subtransfers.push_back(leg(native_id, true, 1));
    rich.subtransfers_by_pid.push_back(second);
    rich.remote_addresses = {"counterparty-a", "counterparty-b"};
    rich.remote_aliases = {"alias-not-exposed"};
    rich.comment = "comment-not-exposed";
    w.m_transfer_history.push_back(rich);
  }
  w.m_transfer_history.push_back(record(tx_id(0x23), 800, 7, {leg(native_id, true, 1)}));
  w.m_transfer_history.push_back(record(tx_id(0x21), 800, 7, {leg(native_id, true, 1)}));
  w.m_transfer_history.push_back(record(tx_id(0x22), 800, 7, {leg(native_id, true, 1)}));
  w.m_transfer_history.push_back(record(tx_id(0x45), 0, 9, {leg(native_id, true, 1)}));  // Unknown time.
  w.m_transfer_history.push_back(record(tx_id(0x46), 900, 0, {leg(native_id, true, 1)})); // Genesis height.
  const std::vector<crypto::hash> expected = {tx_id(0x33), tx_id(0x31), tx_id(0x32), tx_id(0x46),
    tx_id(0x21), tx_id(0x22), tx_id(0x23), tx_id(0x41), tx_id(0x45)};

  const auto one = first_page(*session, 4);
  const auto& p1 = available(one);
  ASSERT_TRUE(p1.coverage.more_rows);
  EXPECT_EQ(9u, p1.coverage.retained_rows);
  EXPECT_EQ(0u, p1.coverage.offset);
  EXPECT_EQ(4u, p1.coverage.next.offset);
  EXPECT_EQ(p1.session_generation, p1.coverage.next.generation);
  EXPECT_EQ(p1.revision, p1.coverage.next.revision);
  EXPECT_EQ(p1.captured_at, p1.coverage.next.captured_at);
  const auto two = continue_page(*session, p1.coverage.next, 4);
  const auto& p2 = available(two);
  ASSERT_TRUE(p2.coverage.more_rows);
  EXPECT_EQ(4u, p2.coverage.offset);
  const auto three = continue_page(*session, p2.coverage.next, 4);
  const auto& p3 = available(three);
  EXPECT_FALSE(p3.coverage.more_rows);
  EXPECT_EQ(8u, p3.coverage.offset);
  std::vector<crypto::hash> walked = row_ids(p1);
  for (const auto& id : row_ids(p2)) walked.push_back(id);
  for (const auto& id : row_ids(p3)) walked.push_back(id);
  EXPECT_TRUE(walked == expected);

  // Row facts: status from the container, fee provenance, unknown time,
  // checked confirmations, flattened legs and transaction counterparties.
  EXPECT_EQ(activity_status::RetainedPending, p1.activity[2].status);
  EXPECT_FALSE(p1.activity[2].timestamp_known);
  EXPECT_EQ(0u, p1.activity[2].confirmations);
  EXPECT_EQ(fee_record::NotRecorded, p1.activity[2].fee_source);
  const auto& genesis_row = p1.activity[3];
  EXPECT_EQ(activity_status::RetainedConfirmed, genesis_row.status);
  EXPECT_EQ(0u, genesis_row.confirmed_height);
  EXPECT_EQ(12u, genesis_row.confirmations);
  const auto& rich = p2.activity[3];
  EXPECT_TRUE(rich.tx_hash == tx_id(0x41));
  EXPECT_TRUE(rich.timestamp_known);
  EXPECT_EQ(800u, rich.timestamp);
  EXPECT_EQ(5u, rich.confirmed_height);
  EXPECT_EQ(7u, rich.confirmations);
  EXPECT_EQ(fee_record::WalletRecorded, rich.fee_source);
  EXPECT_EQ(7u, rich.recorded_fee);
  ASSERT_EQ(3u, rich.legs.size());
  EXPECT_TRUE(rich.legs[0].asset_id == native_id && !rich.legs[0].incoming && rich.legs[0].amount == 5);
  EXPECT_TRUE(rich.legs[1].asset_id == x && !rich.legs[1].incoming && rich.legs[1].amount == 2);
  EXPECT_TRUE(rich.legs[2].asset_id == native_id && rich.legs[2].incoming && rich.legs[2].amount == 1);
  EXPECT_TRUE(rich.counterparties == std::vector<std::string>({"counterparty-a", "counterparty-b"}));
  const auto& unknown_time = p3.activity[0];
  EXPECT_FALSE(unknown_time.timestamp_known);
  EXPECT_EQ(3u, unknown_time.confirmations);
  EXPECT_TRUE(unknown_time.counterparties.empty());

  // Every mismatching binding is stale; a zero offset is malformed.
  const auto cursor = p1.coverage.next;
  auto wrong = cursor;
  ++wrong.generation;
  expect_no_payload(continue_page(*session, wrong, 4), snapshot_status::StaleCursor);
  wrong = cursor;
  ++wrong.revision;
  expect_no_payload(continue_page(*session, wrong, 4), snapshot_status::StaleCursor);
  wrong = cursor;
  ++wrong.capture;
  expect_no_payload(continue_page(*session, wrong, 4), snapshot_status::StaleCursor);
  wrong = cursor;
  ++wrong.captured_at;
  expect_no_payload(continue_page(*session, wrong, 4), snapshot_status::StaleCursor);
  wrong = cursor;
  wrong.offset = 9;
  expect_no_payload(continue_page(*session, wrong, 4), snapshot_status::StaleCursor);
  wrong.offset = 0;
  expect_no_payload(continue_page(*session, wrong, 4), snapshot_status::InvalidRequest);
  EXPECT_EQ(snapshot_status::Available, continue_page(*session, cursor, 4).status);

  // A new first page supersedes every earlier cursor of this session.
  const auto fresh = first_page(*session, 4);
  const auto& restarted = available(fresh);
  EXPECT_TRUE(row_ids(restarted) == row_ids(p1));
  expect_no_payload(continue_page(*session, cursor, 4), snapshot_status::StaleCursor);
  EXPECT_EQ(snapshot_status::Available, continue_page(*session, restarted.coverage.next, 4).status);

  // A reopened session never accepts the old session's cursor.
  const auto old_generation = restarted.session_generation;
  const auto old_cursor = restarted.coverage.next;
  clear_local(w);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  auto reopened = open_session(f, f.control(f.input));
  auto& again = checkpoint_test_access::wallet(*reopened);
  for (uint32_t i = 0; i != 3; ++i)
    again.m_transfer_history.push_back(record(tx_id(0x50 + i), 10 + i, 0, {leg(native_id, true, 1)}));
  const auto next = first_page(*reopened, 1);
  EXPECT_NE(old_generation, available(next).session_generation);
  expect_no_payload(continue_page(*reopened, old_cursor, 1), snapshot_status::StaleCursor);
  EXPECT_EQ(snapshot_status::Available, continue_page(*reopened, available(next).coverage.next, 1).status);
  clear_local(again);
  EXPECT_EQ(release_result::ReleasedUnchanged, reopened->release_unchanged());
}

NATIVE_SESSION_TEST(snapshot_bulk_pages_return_every_retained_row_exactly_once)
{
  fixture f;
  const auto c = f.control(f.input);
  auto session = open_session(f, c);
  auto& w = checkpoint_test_access::wallet(*session);
  extend_local_chain(w, 12);
  use_test_clock(w, 1);
  struct key { bool pending; uint64_t timestamp; uint64_t height; crypto::hash id; };
  std::vector<key> keys;
  for (uint32_t i = 0; i != 120; ++i)
  {
    const uint64_t timestamp = i % 5 == 0 ? 0 : 1000 + i % 7;
    const uint64_t height = i % 11 + 1;
    w.m_transfer_history.push_back(record(tx_id(0x1000 + i), timestamp, height, {leg(native_id, true, 1)}));
    keys.push_back({false, timestamp, height, tx_id(0x1000 + i)});
  }
  for (uint32_t i = 0; i != 3; ++i)
  {
    add_pending(w, record(tx_id(0x2000 + i), i == 1 ? 0 : 50 + i, 0, {leg(native_id, true, 1)}));
    keys.push_back({true, i == 1 ? 0u : 50u + i, 0, tx_id(0x2000 + i)});
  }
  // The documented contract, stated independently of the session.
  std::sort(keys.begin(), keys.end(), [](const key& a, const key& b)
  {
    if (a.pending != b.pending) return a.pending;
    if (a.timestamp != b.timestamp) return a.timestamp > b.timestamp;
    if (a.height != b.height) return a.height > b.height;
    return std::memcmp(&a.id, &b.id, sizeof(a.id)) < 0;
  });
  std::vector<crypto::hash> walked;
  auto page = first_page(*session);
  size_t pages = 0;
  for (;;)
  {
    const auto& s = available(page);
    ++pages;
    EXPECT_EQ(keys.size(), s.coverage.retained_rows);
    EXPECT_EQ(walked.size(), s.coverage.offset);
    EXPECT_LE(s.activity.size(), default_activity_page);
    for (const auto& id : row_ids(s)) walked.push_back(id);
    if (!s.coverage.more_rows) break;
    require(pages < 10);
    const auto cursor = s.coverage.next;
    page = continue_page(*session, cursor);
  }
  EXPECT_EQ(3u, pages);
  ASSERT_EQ(keys.size(), walked.size());
  std::set<std::string> unique;
  for (size_t i = 0; i != keys.size(); ++i)
  {
    EXPECT_TRUE(keys[i].id == walked[i]);
    unique.insert(std::string(reinterpret_cast<const char*>(&walked[i]), sizeof(crypto::hash)));
  }
  EXPECT_EQ(keys.size(), unique.size());
  clear_local(w);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_SESSION_TEST(snapshot_revision_advances_on_possible_mutation_never_resets_and_never_wraps)
{
  const test_chain chain;
  {
    fixture f;
    const auto sibling = child(chain.g, 7);
    const auto after_sibling = child(sibling);
    const auto c = capable(f);
    auto session = open_session(f, c);
    const auto transport = std::make_shared<controlled_transport>();
    ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
    std::atomic<bool> stop{false};
    auto& w = checkpoint_test_access::wallet(*session);
    // Two synthetic rows so the first page leaves an in-range cursor; they
    // are removed below, before the checkpoint, and are never persisted.
    w.m_transfer_history.push_back(record(tx_id(1), 10, 0, {leg(native_id, true, 1)}));
    w.m_transfer_history.push_back(record(tx_id(2), 20, 0, {leg(native_id, true, 1)}));

    const auto initial = first_page(*session, 1);
    EXPECT_EQ(0u, available(initial).revision);
    const auto cursor = available(initial).coverage.next;
    ASSERT_TRUE(available(initial).coverage.more_rows);
    serve(*transport, 0, 2, {chain.g, chain.b1});
    EXPECT_EQ(tools::read_only::step_status::Progress, step(*session, stop).status);
    EXPECT_EQ(1u, checkpoint_test_access::revision(*session));
    // Rows are still retained, so staleness here is the revision alone.
    EXPECT_GE(w.m_transfer_history.size(), 2u);
    expect_no_payload(continue_page(*session, cursor, 1), snapshot_status::StaleCursor);
    // Synthetic rows end here, before any checkpoint; scanned state is kept.
    auto& history = w.m_transfer_history;
    history.erase(std::remove_if(history.begin(), history.end(), [](const wti_t& wti)
      { return wti.tx_hash == tx_id(1) || wti.tx_hash == tx_id(2); }), history.end());

    // Equal height, different block: still a new state.
    serve(*transport, 0, 2, {chain.g, sibling});
    const auto reorg = step(*session, stop);
    EXPECT_EQ(tools::read_only::step_status::Progress, reorg.status);
    EXPECT_EQ(reorg.local_blocks_before, reorg.local_blocks_after);
    const auto after_reorg = first_page(*session, 1);
    EXPECT_EQ(2u, available(after_reorg).revision);
    EXPECT_TRUE(available(after_reorg).local_tip_hash == currency::get_block_hash(sibling.block_ptr->bl));

    // A clean preflight refusal reports no possible change: no new revision.
    serve(*transport, 1, 3, {chain.b1, chain.b2});
    const auto refusal = step(*session, stop);
    EXPECT_EQ(tools::read_only::step_status::RecoveryRequired, refusal.status);
    EXPECT_FALSE(refusal.state_may_have_changed);
    EXPECT_EQ(2u, checkpoint_test_access::revision(*session));
    EXPECT_EQ(session_state::OpenDirty, session->state());

    if (replacement_supported)
    {
      // A committed checkpoint clears dirty state but never the revision.
      EXPECT_EQ(checkpoint_result::Saved, session->checkpoint());
      EXPECT_EQ(session_state::OpenUnchanged, session->state());
      const auto saved = first_page(*session, 1);
      EXPECT_EQ(2u, available(saved).revision);
    }

    // Exhaustion is sticky and fails closed; the counter never wraps.
    const uint64_t max = std::numeric_limits<uint64_t>::max();
    checkpoint_test_access::set_revision(*session, max);
    const auto top = first_page(*session, 1);
    EXPECT_EQ(max, available(top).revision);
    serve(*transport, 1, 3, {sibling, after_sibling});
    EXPECT_EQ(tools::read_only::step_status::Progress, step(*session, stop).status);
    EXPECT_EQ(max, checkpoint_test_access::revision(*session));
    expect_no_payload(first_page(*session, 1), snapshot_status::LimitExceeded);
    auto exhausted = available(top).coverage.next;
    exhausted.offset = 1; // Well-formed; refused for the exhausted counter.
    expect_no_payload(continue_page(*session, exhausted, 1), snapshot_status::LimitExceeded);
    expect_no_payload(first_page(*session, 1), snapshot_status::LimitExceeded);
    EXPECT_EQ(session_state::OpenDirty, session->state());
    session.reset(); // Test teardown only.
  }
  {
    // Cancelled after processing, without a height increase.
    fixture f;
    const auto c = capable(f);
    auto session = open_session(f, c);
    const auto transport = std::make_shared<controlled_transport>();
    ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
    const auto stop = std::make_shared<std::atomic<bool>>(false);
    auto genesis = genesis_entry();
    const auto first = child(genesis);
    {
      auto* indices = new currency::transaction_chain_entry(*genesis.coinbase_ptr);
      genesis.coinbase_ptr = std::shared_ptr<const currency::transaction_chain_entry>(indices,
        [stop](const currency::transaction_chain_entry* entry) { delete entry; *stop = true; });
    }
    transport->consume = true;
    std::list<block_entry> blocks;
    blocks.push_back(std::move(genesis));
    blocks.push_back(first);
    serve(*transport, 0, 2, std::move(blocks));
    const auto scan = step(*session, *stop);
    EXPECT_EQ(tools::read_only::step_status::Cancelled, scan.status);
    EXPECT_TRUE(scan.state_may_have_changed);
    const auto cancelled = first_page(*session);
    EXPECT_EQ(1u, available(cancelled).revision);
    EXPECT_EQ(1u, available(cancelled).local_blocks);
    session.reset();
  }
  {
    // A step completing after revocation still advances the revision; the
    // snapshot itself is refused.
    fixture f;
    const auto c = capable(f);
    auto session = open_session(f, c);
    const auto transport = std::make_shared<controlled_transport>();
    ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
    std::atomic<bool> stop{false};
    transport->on_request = [&] { c->authorized = false; };
    serve(*transport, 0, 3, {chain.g, chain.b1, chain.b2});
    const auto late = step(*session, stop, sync_step_status::Revoked);
    transport->on_request = nullptr;
    EXPECT_TRUE(late.state_may_have_changed);
    EXPECT_EQ(1u, checkpoint_test_access::revision(*session));
    c->authorized = true;
    expect_no_payload(first_page(*session), snapshot_status::Revoked);
    session.reset();
  }
}

NATIVE_SESSION_TEST(snapshot_never_changes_dirty_state_persistence_or_close)
{
  fixture f;
  const test_chain chain;
  const auto c = capable(f);
  auto session = open_session(f, c);
  const auto transport = std::make_shared<controlled_transport>();
  ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
  std::atomic<bool> stop{false};
  serve(*transport, 0, 2, {chain.g, chain.b1});
  EXPECT_EQ(tools::read_only::step_status::Progress, step(*session, stop).status);
  const auto tree = snapshot(f.root);
  uint64_t generation = 0;
  for (unsigned i = 0; i != 3; ++i)
  {
    const auto observed = first_page(*session);
    generation = available(observed).session_generation;
    EXPECT_EQ(session_state::OpenDirty, session->state());
  }
  EXPECT_EQ(0u, c->checkpoints);
  EXPECT_TRUE(f.baseline == read(f.path));
  EXPECT_TRUE(tree == snapshot(f.root));
  EXPECT_EQ(release_result::Unavailable, session->release_unchanged());
  EXPECT_EQ(1u, transport->requests);
  if (!replacement_supported)
  {
    session.reset(); // The committed close needs the Linux fixture.
    return;
  }
  EXPECT_EQ(close_result::ReleasedSaved, session->close());
  EXPECT_EQ(1u, c->checkpoints);
  auto again = open_session(f, capable(f));
  const auto reopened = first_page(*again);
  EXPECT_EQ(2u, available(reopened).local_blocks);
  EXPECT_EQ(0u, available(reopened).revision);
  EXPECT_NE(generation, available(reopened).session_generation);
  EXPECT_EQ(close_result::ReleasedUnchanged, again->close());
}

// Source-authored only; these cases require a separately admitted native run.
#ifdef TESTNET
payment_request payment_for(file_session& session, uint64_t amount = 1)
{
  payment_request request;
  request.recipient = session.get_receive().address;
  request.amount = amount;
  request.expected_generation = checkpoint_test_access::generation(session);
  request.expected_revision = checkpoint_test_access::revision(session);
  return request;
}
void payment_refused(const payment_result& result, payment_status status)
{
  EXPECT_EQ(status, result.status);
  EXPECT_FALSE(result.assessment);
}

NATIVE_SESSION_TEST(payment_capacity_is_advisory_and_excludes_reservations_spent_locked_and_bad_mix)
{
  fixture f;
  const auto tree = snapshot(f.root);
  const auto c = f.control(f.input);
  auto session = open_session(f, c);
  auto& w = checkpoint_test_access::wallet(*session);
  extend_local_chain(w, 12);
  use_test_clock(w, 1700000000);
  const auto tx = wallet_tx(1, 6);
  w.m_transfers[0] = output(tx, 0, TX_DEFAULT_FEE + 7, native_id);
  w.m_transfers[1] = output(tx, 1, 100, native_id, WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION);
  w.m_transfers[2] = output(tx, 2, 100, native_id, WALLET_TRANSFER_DETAIL_FLAG_COLD_SIG_RESERVATION);
  w.m_transfers[3] = output(tx, 3, 100, native_id, WALLET_TRANSFER_DETAIL_FLAG_SPENT);
  w.m_transfers[4] = output(tx, 4, 100, native_id, WALLET_TRANSFER_DETAIL_FLAG_BLOCKED);
  w.m_transfers[5] = output(tx, 5, 100, asset_key(700)); // Other asset never funds native amount/fee.
  const auto no_mix = wallet_tx(1, 1);
  boost::get<currency::tx_out_zarcanum>(no_mix->m_tx.vout[0]).mix_attr = CURRENCY_TO_KEY_OUT_FORCED_NO_MIX;
  w.get_core_runtime_config().hf4_minimum_mixins = 4;
  w.m_transfers[6] = output(no_mix, 0, 100, native_id);
  const auto bare = wallet_tx(1, 1);
  bare->m_tx.version = TRANSACTION_VERSION_PRE_HF4;
  currency::tx_out_bare ordinary{};
  ordinary.amount = 3;
  ordinary.target = currency::txout_to_key{};
  bare->m_tx.vout[0] = ordinary;
  auto bare_td = output(bare, 0, 3, native_id);
  bare_td.m_zc_info_ptr.reset();
  w.m_transfers[7] = bare_td;
  w.m_transfers[8] = output(wallet_tx(1, 1), 0, 100, native_id, WALLET_TRANSFER_DETAIL_FLAG_ASSET_OP_RESERVATION);
  auto request = payment_for(*session, 10);
  const auto before_reads = clock_reads;
  const auto result = session->assess_local_payment(request);
  ASSERT_EQ(payment_status::LocalAdvisory, result.status);
  ASSERT_TRUE(result.assessment != nullptr);
  const auto& a = *result.assessment;
  EXPECT_EQ(request.recipient, a.canonical_recipient);
  EXPECT_TRUE(a.asset_id == native_id);
  EXPECT_EQ(CURRENCY_DISPLAY_DECIMAL_POINT, a.decimals);
  EXPECT_EQ(TX_DEFAULT_FEE, a.compiled_policy_fee);
  EXPECT_EQ(TX_DEFAULT_FEE + 10, a.amount_plus_fee);
  EXPECT_EQ(TX_DEFAULT_FEE + 10, a.eligible_local_capacity);
  EXPECT_EQ(2u, a.eligible_output_count);
  EXPECT_EQ(CURRENCY_DEFAULT_DECOY_SET_SIZE, a.bare_decoy_policy);
  EXPECT_EQ(4u, a.confidential_decoy_policy);
  EXPECT_EQ(10u, a.advisory_amount_upper_bound);
  EXPECT_TRUE(a.local_capacity_covers_amount_and_fee);
  EXPECT_FALSE(a.pool_observed);
  EXPECT_FALSE(a.live_freshness_verified);
  EXPECT_EQ(before_reads + 1, clock_reads);
  EXPECT_EQ(0u, checkpoint_test_access::revision(*session));
  EXPECT_EQ(session_state::OpenUnchanged, session->state());
  EXPECT_EQ(0u, w.m_transfers[0].m_flags); // No selection/reservation mutation.
  EXPECT_EQ(WALLET_TRANSFER_DETAIL_FLAG_ESCROW_PROPOSAL_RESERVATION, w.m_transfers[1].m_flags);
  EXPECT_EQ(0u, c->checkpoints);
  EXPECT_TRUE(tree == snapshot(f.root));
  request.amount = 11;
  const auto over = session->assess_local_payment(request);
  ASSERT_TRUE(over.assessment != nullptr);
  EXPECT_FALSE(over.assessment->local_capacity_covers_amount_and_fee);
  clear_local(w);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  expect_guarded_calls(c);
}

NATIVE_SESSION_TEST(payment_rejects_nonordinary_recipients_zero_and_amount_fee_overflow)
{
  fixture f;
  const auto c = f.control(f.input);
  auto session = open_session(f, c);
  auto request = payment_for(*session);
  const auto canonical = request.recipient;
  currency::account_public_address address{};
  currency::payment_id_t pid;
  { epee::log_space::scoped_log_silence quiet;
    require(currency::get_account_address_and_payment_id_from_str(address, pid, canonical)); }
  address.flags = ACCOUNT_PUBLIC_ADDRESS_FLAG_AUDITABLE;
  const std::vector<std::string> invalid = {"", "@recipient", " " + canonical, canonical + "\n",
    std::string(max_payment_recipient_bytes + 1, '1'), std::string(1, '\0') + canonical,
    currency::get_account_address_as_str(address), canonical.substr(1)};
  for (const auto& recipient : invalid)
  {
    request.recipient = recipient;
    payment_refused(session->assess_local_payment(request), payment_status::InvalidRecipient);
  }
  address.flags = 0;
  request.recipient = currency::get_account_address_as_str(address, "test");
  payment_refused(session->assess_local_payment(request), payment_status::InvalidRecipient);
  request.recipient = currency::get_account_address_as_str(currency::gateway_address_id_type{});
  payment_refused(session->assess_local_payment(request), payment_status::InvalidRecipient);
  request.recipient = canonical;
  request.amount = 0;
  payment_refused(session->assess_local_payment(request), payment_status::InvalidAmount);
  request.amount = std::numeric_limits<uint64_t>::max();
  payment_refused(session->assess_local_payment(request), payment_status::AmountOverflow);
  request.amount = std::numeric_limits<uint64_t>::max() - TX_DEFAULT_FEE;
  const auto exact = session->assess_local_payment(request);
  ASSERT_TRUE(exact.assessment != nullptr);
  EXPECT_EQ(std::numeric_limits<uint64_t>::max(), exact.assessment->amount_plus_fee);
  EXPECT_FALSE(exact.assessment->local_capacity_covers_amount_and_fee);
  EXPECT_EQ(0u, exact.assessment->advisory_amount_upper_bound);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}


NATIVE_SESSION_TEST(payment_rejects_decodable_noncanonical_recipient_encodings)
{
  fixture f;
  auto session = open_session(f, f.control(f.input));
  auto request = payment_for(*session);
  const auto canonical = request.recipient;
  currency::account_public_address address{};
  currency::payment_id_t pid;
  { epee::log_space::scoped_log_silence quiet;
    require(currency::get_account_address_and_payment_id_from_str(address, pid, canonical)); }
  ASSERT_EQ(0u, address.flags);
  ASSERT_TRUE(pid.empty());
  const auto old_blob = ::t_serializable_object_to_blob(address.to_old());
  const auto new_blob = ::t_serializable_object_to_blob(address);
  ASSERT_EQ(sizeof(currency::account_public_address_old), old_blob.size());
  ASSERT_EQ(sizeof(currency::account_public_address), new_blob.size());
  // All four decode to the same ordinary flags-zero keys and an empty payment
  // id. Only the canonical re-encode comparison rejects these encodings.
  const std::vector<std::string> noncanonical = {
    tools::base58::encode_addr(CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX, new_blob),
    tools::base58::encode_addr(CURRENCY_PUBLIC_AUDITABLE_ADDRESS_BASE58_PREFIX, new_blob),
    tools::base58::encode_addr(CURRENCY_PUBLIC_INTEG_ADDRESS_BASE58_PREFIX, old_blob),
    tools::base58::encode_addr(CURRENCY_PUBLIC_INTEG_ADDRESS_V2_BASE58_PREFIX, new_blob)
  };
  for (const auto& text : noncanonical)
  {
    currency::address_v decoded;
    currency::payment_id_t decoded_pid;
    { epee::log_space::scoped_log_silence quiet;
      ASSERT_TRUE(currency::get_account_address_and_payment_id_from_str(decoded, decoded_pid, text)); }
    ASSERT_TRUE(decoded.type() == typeid(currency::account_public_address));
    const auto& ordinary = boost::get<currency::account_public_address>(decoded);
    ASSERT_EQ(0u, ordinary.flags);
    ASSERT_TRUE(decoded_pid.empty());
    ASSERT_TRUE(ordinary.spend_public_key == address.spend_public_key);
    ASSERT_TRUE(ordinary.view_public_key == address.view_public_key);
    ASSERT_EQ(canonical, currency::get_account_address_as_str(ordinary));
    ASSERT_NE(canonical, text);
    request.recipient = text;
    payment_refused(session->assess_local_payment(request), payment_status::InvalidRecipient);
  }
  currency::account_base other;
  other.generate();
  request.recipient = other.get_public_address_str();
  ASSERT_NE(canonical, request.recipient);
  const auto accepted = session->assess_local_payment(request);
  ASSERT_EQ(payment_status::LocalAdvisory, accepted.status);
  ASSERT_TRUE(accepted.assessment != nullptr);
  EXPECT_EQ(request.recipient, accepted.assessment->canonical_recipient);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_SESSION_TEST(payment_capacity_uses_captured_unlock_maturity_and_checked_sums)
{
  fixture f;
  auto session = open_session(f, f.control(f.input));
  auto& w = checkpoint_test_access::wallet(*session);
  extend_local_chain(w, 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE);
  const uint64_t at = 1700000000;
  const auto timed = wallet_tx(1, 1, at);
  w.m_transfers[0] = output(timed, 0, TX_DEFAULT_FEE, native_id);
  const auto request = payment_for(*session);
  use_test_clock(w, at - CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS - 1);
  const auto locked = session->assess_local_payment(request);
  ASSERT_TRUE(locked.assessment != nullptr);
  EXPECT_EQ(0u, locked.assessment->eligible_local_capacity);
  clock_value += 1;
  const auto fee_only = session->assess_local_payment(request);
  ASSERT_TRUE(fee_only.assessment != nullptr);
  EXPECT_EQ(TX_DEFAULT_FEE, fee_only.assessment->eligible_local_capacity);
  EXPECT_EQ(0u, fee_only.assessment->advisory_amount_upper_bound);
  EXPECT_FALSE(fee_only.assessment->local_capacity_covers_amount_and_fee);
  timed->m_block_height += 1; // One block short of maturity.
  const auto immature = session->assess_local_payment(request);
  ASSERT_TRUE(immature.assessment != nullptr);
  EXPECT_EQ(0u, immature.assessment->eligible_local_capacity);
  timed->m_block_height = 1;
  clock_value = std::numeric_limits<uint64_t>::max();
  payment_refused(session->assess_local_payment(request), payment_status::InvalidData);
  use_test_clock(w, at);
  w.m_transfers[0].m_amount = std::numeric_limits<uint64_t>::max();
  w.m_transfers[1] = output(wallet_tx(1, 1), 0, 1, native_id);
  payment_refused(session->assess_local_payment(request), payment_status::InvalidData);
  clear_local(w);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_SESSION_TEST(payment_refuses_retained_pending_stale_observations_and_identity_drift)
{
  fixture f;
  auto session = open_session(f, f.control(f.input));
  auto& w = checkpoint_test_access::wallet(*session);
  auto request = payment_for(*session);
  ++request.expected_generation;
  payment_refused(session->assess_local_payment(request), payment_status::StaleObservation);
  request = payment_for(*session);
  ++request.expected_revision;
  payment_refused(session->assess_local_payment(request), payment_status::StaleObservation);
  request = payment_for(*session);
  add_pending(w, record(tx_id(800), 1, 0, {leg(native_id, true, 1), leg(native_id, false, 2)}));
  payment_refused(session->assess_local_payment(request), payment_status::RetainedPendingUnsupported);
  clear_local(w);
  w.m_unconfirmed_in_transfers.emplace(tx_id(801), hidden_tx(0));
  payment_refused(session->assess_local_payment(request), payment_status::RetainedPendingUnsupported);
  w.m_unconfirmed_in_transfers.clear();
  w.m_unconfirmed_multisig_transfers.insert(tx_id(802));
  payment_refused(session->assess_local_payment(request), payment_status::RetainedPendingUnsupported);
  w.m_unconfirmed_multisig_transfers.clear();
  w.m_money_expirations.emplace_back();
  payment_refused(session->assess_local_payment(request), payment_status::RetainedPendingUnsupported);
  w.m_money_expirations.clear();
  checkpoint_test_access::forge_genesis(*session);
  payment_refused(session->assess_local_payment(request), payment_status::IdentityMismatch);
  session.reset(); // Fault-injected test teardown, no checkpoint.
}

NATIVE_SESSION_TEST(payment_refuses_unsupported_outputs_nested_limits_and_exhausted_revision)
{
  fixture f;
  auto session = open_session(f, f.control(f.input));
  auto& w = checkpoint_test_access::wallet(*session);
  extend_local_chain(w, 12);
  use_test_clock(w, 1700000000);
  const auto request = payment_for(*session);
  const auto tx = wallet_tx(1, 1);
  w.m_transfers[0] = output(tx, 0, 1, native_id);
  w.m_transfers[0].m_zc_info_ptr.reset(); // Bare metadata on a confidential output.
  payment_refused(session->assess_local_payment(request), payment_status::UnsupportedData);
  w.m_transfers[0] = output(tx, 0, 1, native_id);
  w.m_transfers[0].m_flags = uint32_t(1 << 5); // Unassigned flag.
  payment_refused(session->assess_local_payment(request), payment_status::UnsupportedData);
  w.m_transfers[0].m_flags = 0;
  tx->m_tx.extra.resize(max_snapshot_nested_elements + 1, currency::extra_v(crypto::public_key{}));
  payment_refused(session->assess_local_payment(request), payment_status::LimitExceeded);
  clear_local(w);
  w.m_minimum_height = w.m_chain.get_blockchain_current_size(); // Invalid: boundary cannot equal local block count.
  payment_refused(session->assess_local_payment(request), payment_status::UnsupportedWallet);
  w.m_minimum_height = WALLET_MINIMUM_HEIGHT_UNSET_CONST;
  checkpoint_test_access::exhaust_revision(*session);
  payment_refused(session->assess_local_payment(request), payment_status::LimitExceeded);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

std::function<void()> payment_clock_callback;
uint64_t reentrant_payment_clock()
{
  payment_clock_callback();
  return 1700000000;
}

NATIVE_SESSION_TEST(payment_captures_request_before_authorization_and_material_callbacks)
{
  for (unsigned at_authorization = 0; at_authorization != 2; ++at_authorization)
  {
    fixture f;
    const auto c = f.control(f.input);
    auto session = open_session(f, c);
    auto request = payment_for(*session, 7);
    const auto original = request;
    ASSERT_GT(original.expected_generation, 0u);
    ASSERT_EQ(checkpoint_test_access::revision(*session), original.expected_revision);
    size_t callback_reads = 0;
    const auto mutate = [&]
    {
      ++callback_reads;
      request.recipient = "@x";
      request.amount = std::numeric_limits<uint64_t>::max();
      request.expected_generation = 0;
      ++request.expected_revision;
    };
    if (at_authorization) c->on_authorization = mutate;
    else c->on_material = mutate;
    const auto result = session->assess_local_payment(request);
    c->on_authorization = nullptr;
    c->on_material = nullptr;
    ASSERT_GT(callback_reads, 0u);
    ASSERT_EQ(payment_status::LocalAdvisory, result.status);
    ASSERT_TRUE(result.assessment != nullptr);
    EXPECT_EQ(original.recipient, result.assessment->canonical_recipient);
    EXPECT_EQ(original.amount, result.assessment->amount);
    EXPECT_EQ(original.amount + TX_DEFAULT_FEE, result.assessment->amount_plus_fee);
    EXPECT_EQ(original.expected_generation, result.assessment->session_generation);
    EXPECT_EQ(original.expected_revision, result.assessment->revision);
    // An early authorization callback cannot repair an originally stale
    // generation or revision by changing the caller's request.
    for (unsigned stale_generation = 0; stale_generation != 2; ++stale_generation)
    {
      request = original;
      if (stale_generation) request.expected_generation = 0;
      else ++request.expected_revision;
      size_t repair_reads = 0;
      c->on_authorization = [&] { ++repair_reads; request = original; };
      payment_refused(session->assess_local_payment(request), payment_status::StaleObservation);
      c->on_authorization = nullptr;
      EXPECT_EQ(1u, repair_reads);
    }
    EXPECT_EQ(0u, c->checkpoints);
    EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
    expect_guarded_calls(c);
  }
}

NATIVE_SESSION_TEST(payment_captures_validated_request_before_clock_mutation)
{
  fixture f;
  const auto c = f.control(f.input);
  auto session = open_session(f, c);
  auto& w = checkpoint_test_access::wallet(*session);
  extend_local_chain(w, 1 + WALLET_DEFAULT_TX_SPENDABLE_AGE);
  w.m_transfers[0] = output(wallet_tx(1, 1), 0, TX_DEFAULT_FEE + 7, native_id);
  auto request = payment_for(*session, 7);
  const auto original = request;
  ASSERT_GT(original.expected_generation, 0u);
  ASSERT_EQ(checkpoint_test_access::revision(*session), original.expected_revision);
  size_t callback_reads = 0;
  payment_clock_callback = [&]
  {
    ++callback_reads;
    request.recipient = "@x";
    request.amount = std::numeric_limits<uint64_t>::max();
    request.expected_generation = 0;
    ++request.expected_revision;
  };
  w.get_core_runtime_config().get_core_time = &reentrant_payment_clock;
  const auto result = session->assess_local_payment(request);
  payment_clock_callback = nullptr;
  ASSERT_EQ(1u, callback_reads);
  ASSERT_EQ(payment_status::LocalAdvisory, result.status);
  ASSERT_TRUE(result.assessment != nullptr);
  EXPECT_EQ(original.recipient, result.assessment->canonical_recipient);
  EXPECT_EQ(original.amount, result.assessment->amount);
  EXPECT_EQ(TX_DEFAULT_FEE + 7, result.assessment->amount_plus_fee);
  EXPECT_EQ(original.expected_generation, result.assessment->session_generation);
  EXPECT_EQ(original.expected_revision, result.assessment->revision);
  EXPECT_TRUE(result.assessment->local_capacity_covers_amount_and_fee);
  EXPECT_EQ(0u, c->checkpoints);
  EXPECT_EQ(0u, checkpoint_test_access::revision(*session));
  clear_local(w);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  expect_guarded_calls(c);
}

NATIVE_SESSION_TEST(payment_reentry_is_busy_and_final_revocation_is_sticky_with_no_payload)
{
  fixture f;
  const auto c = f.control(f.input);
  auto session = open_session(f, c);
  auto& w = checkpoint_test_access::wallet(*session);
  const auto request = payment_for(*session);
  payment_clock_callback = [&]
  {
    payment_refused(session->assess_local_payment(request), payment_status::Busy);
    EXPECT_EQ(receive_status::Unavailable, session->get_receive().status);
    c->authorized = false;
  };
  w.get_core_runtime_config().get_core_time = &reentrant_payment_clock;
  payment_refused(session->assess_local_payment(request), payment_status::Revoked);
  payment_clock_callback = nullptr;
  c->authorized = true;
  payment_refused(session->assess_local_payment(request), payment_status::Revoked);
  EXPECT_EQ(0u, c->checkpoints);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}
#else
NATIVE_SESSION_TEST(payment_assessment_refuses_mainnet_profile)
{
  fixture f;
  auto session = open_session(f, f.control(f.input));
  payment_request request;
  request.expected_generation = checkpoint_test_access::generation(*session);
  request.expected_revision = checkpoint_test_access::revision(*session);
  const auto result = session->assess_local_payment(request);
  EXPECT_EQ(payment_status::UnsupportedProfile, result.status);
  EXPECT_FALSE(result.assessment);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}
#endif

#if NATIVE_SESSION_REPLACEMENT_FIXTURE
// Clean-tip terminal regressions use the existing real wallet/session/writer seams.
// Source only here. Execution belongs to the separately authorized native test gate.
// Initialize the synthetic zero-minimum policy before scanning; every assertion
// about the clean tip is AFTER a real adding step and Committed checkpoint.
owner clean_tip_session(fixture& f, const std::shared_ptr<provider_state>& control,
  const std::shared_ptr<controlled_transport>& transport, const test_chain& chain,
  std::atomic<bool>& stop, tools::read_only::step_limits bounds = limits())
{
  auto session = open_session(f, control);
  checkpoint_test_access::wallet(*session).set_minimum_height(0);
  require(session->prepare_sync(transport, bounds) == sync_prepare_result::Prepared);
  serve(*transport, 0, 2, {chain.g, chain.b1});
  require(step(*session, stop).status == tools::read_only::step_status::Progress);
  require(session->checkpoint() == checkpoint_result::Saved);
  serve(*transport, 1, 3, {chain.b1, chain.b2});
  require(step(*session, stop).status == tools::read_only::step_status::Progress);
  require(session->checkpoint() == checkpoint_result::Saved);
  return session;
}

NATIVE_SESSION_TEST(clean_tip_terminal_preserves_committed_final_batch_revision_image_and_snapshot)
{
  fixture f; const test_chain chain; const auto control = capable(f);
  const auto transport = std::make_shared<controlled_transport>(); std::atomic<bool> stop{false};
  auto session = clean_tip_session(f, control, transport, chain, stop);
  const auto revision = checkpoint_test_access::revision(*session);
  const auto image = read(f.path);
  const auto chain_state = checkpoint_test_access::wallet(*session).m_chain.get_internal_state_text();
  const auto captured = first_page(*session);
  ASSERT_EQ(snapshot_status::Available, captured.status);
  ASSERT_TRUE(captured.observation != nullptr);
  EXPECT_EQ(3u, captured.observation->local_blocks);
  EXPECT_EQ(revision, captured.observation->revision);
  ASSERT_EQ(session_state::OpenUnchanged, session->state());
  serve(*transport, 2, 3, {chain.b2});
  const auto terminal = session->step_sync(stop);
  EXPECT_EQ(sync_step_status::Stepped, terminal.status);
  EXPECT_EQ(tools::read_only::step_status::NoProgress, terminal.scan.status);
  EXPECT_FALSE(terminal.scan.state_may_have_changed);
  EXPECT_FALSE(terminal.scan.batch_truncated);
  EXPECT_TRUE(terminal.scan.has_daemon_observation);
  EXPECT_EQ(3u, terminal.scan.local_blocks_before);
  EXPECT_EQ(3u, terminal.scan.local_blocks_after);
  EXPECT_EQ(3u, terminal.scan.reported_daemon_blocks);
  EXPECT_EQ(0u, terminal.scan.blocks_added);
  EXPECT_EQ(session_state::OpenUnchanged, session->state());
  EXPECT_EQ(revision, checkpoint_test_access::revision(*session));
  EXPECT_TRUE(chain_state == checkpoint_test_access::wallet(*session).m_chain.get_internal_state_text());
  EXPECT_TRUE(image == read(f.path)); // Never print wallet/image material.
  const auto after = first_page(*session);
  ASSERT_EQ(snapshot_status::Available, after.status);
  ASSERT_TRUE(after.observation != nullptr);
  EXPECT_EQ(revision, after.observation->revision);
  EXPECT_EQ(captured.observation->local_blocks, after.observation->local_blocks);
  EXPECT_TRUE(captured.observation->local_tip_hash == after.observation->local_tip_hash);
  EXPECT_EQ(checkpoint_result::Unchanged, session->checkpoint());
  EXPECT_EQ(2u, control->checkpoints);
  EXPECT_EQ(0u, transport->forbidden);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_SESSION_TEST(clean_tip_terminal_does_not_clean_preexisting_dirty_progress)
{
  fixture f; const test_chain chain; const auto control = capable(f);
  const auto transport = std::make_shared<controlled_transport>(); std::atomic<bool> stop{false};
  auto session = open_session(f, control);
  checkpoint_test_access::wallet(*session).set_minimum_height(0);
  ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
  serve(*transport, 0, 2, {chain.g, chain.b1});
  ASSERT_EQ(tools::read_only::step_status::Progress, step(*session, stop).status);
  const auto revision = checkpoint_test_access::revision(*session);
  serve(*transport, 1, 2, {chain.b1});
  const auto terminal = step(*session, stop);
  EXPECT_EQ(tools::read_only::step_status::NoProgress, terminal.status);
  EXPECT_TRUE(terminal.state_may_have_changed);
  EXPECT_EQ(session_state::OpenDirty, session->state());
  EXPECT_GT(checkpoint_test_access::revision(*session), revision);
  EXPECT_EQ(checkpoint_result::Saved, session->checkpoint());
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_SESSION_TEST(clean_tip_terminal_keeps_older_anchor_and_truncated_replies_on_dirty_path)
{
  for (bool truncated : {false, true})
  {
    fixture f; const test_chain chain; const auto control = capable(f);
    const auto transport = std::make_shared<controlled_transport>(); std::atomic<bool> stop{false};
    auto session = clean_tip_session(f, control, transport, chain, stop, {2, 16});
    if (truncated) serve(*transport, 0, 3, {chain.g, chain.b1, chain.b2});
    else serve(*transport, 1, 3, {chain.b1, chain.b2});
    const auto terminal = step(*session, stop);
    EXPECT_EQ(tools::read_only::step_status::NoProgress, terminal.status);
    EXPECT_EQ(truncated, terminal.batch_truncated);
    EXPECT_TRUE(terminal.state_may_have_changed);
    EXPECT_EQ(session_state::OpenDirty, session->state());
    EXPECT_EQ(checkpoint_result::Saved, session->checkpoint());
    EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  }
}

NATIVE_SESSION_TEST(clean_tip_terminal_preserves_required_daemon_bookkeeping_update)
{
  fixture f; const test_chain chain; const auto control = capable(f);
  const auto transport = std::make_shared<controlled_transport>(); std::atomic<bool> stop{false};
  auto session = clean_tip_session(f, control, transport, chain, stop);
  // Establish different committed daemon bookkeeping through the real handler.
  serve(*transport, 2, 4, {chain.b2});
  ASSERT_TRUE(step(*session, stop).state_may_have_changed);
  ASSERT_EQ(checkpoint_result::Saved, session->checkpoint());
  const auto revision = checkpoint_test_access::revision(*session);
  serve(*transport, 2, 3, {chain.b2});
  const auto terminal = step(*session, stop);
  EXPECT_EQ(tools::read_only::step_status::NoProgress, terminal.status);
  EXPECT_TRUE(terminal.state_may_have_changed);
  EXPECT_EQ(session_state::OpenDirty, session->state());
  EXPECT_GT(checkpoint_test_access::revision(*session), revision);
  EXPECT_EQ(checkpoint_result::Saved, session->checkpoint());
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

NATIVE_SESSION_TEST(clean_tip_terminal_never_bypasses_anchor_transaction_or_cancellation_validation)
{
  for (unsigned variant = 0; variant != 5; ++variant)
  {
    fixture f; const test_chain chain; const auto control = capable(f);
    const auto transport = std::make_shared<controlled_transport>(); std::atomic<bool> stop{false};
    auto session = clean_tip_session(f, control, transport, chain, stop);
    const auto image = read(f.path); const auto revision = checkpoint_test_access::revision(*session);
    auto tip = chain.b2;
    if (variant == 0) tip = child(chain.b1, 7); // Same height, wrong anchored hash.
    if (variant == 1) tip.coinbase_ptr.reset();
    if (variant == 2) tip.txs_ptr.push_back(std::make_shared<currency::transaction_chain_entry>());
    serve(*transport, 2, 3, {tip});
    if (variant == 3) stop = true;
    if (variant == 4) transport->on_request = [&] { stop = true; };
    const auto terminal = step(*session, stop);
    const auto expected = variant == 0 ? tools::read_only::step_status::RecoveryRequired :
      variant < 3 ? tools::read_only::step_status::InvalidResponse : tools::read_only::step_status::Cancelled;
    EXPECT_EQ(expected, terminal.status);
    EXPECT_FALSE(terminal.state_may_have_changed);
    EXPECT_EQ(session_state::OpenUnchanged, session->state());
    EXPECT_EQ(revision, checkpoint_test_access::revision(*session));
    EXPECT_TRUE(image == read(f.path));
    EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  }
}

NATIVE_SESSION_TEST(clean_tip_terminal_never_bypasses_revoked_session_authority)
{
  fixture f; const test_chain chain; const auto control = capable(f);
  const auto transport = std::make_shared<controlled_transport>(); std::atomic<bool> stop{false};
  auto session = clean_tip_session(f, control, transport, chain, stop);
  serve(*transport, 2, 3, {chain.b2});
  const auto requests = transport->requests;
  const auto revision = checkpoint_test_access::revision(*session);
  const auto image = read(f.path);
  control->authorized = false;
  const auto terminal = session->step_sync(stop);
  EXPECT_EQ(sync_step_status::Revoked, terminal.status);
  EXPECT_FALSE(terminal.scan.state_may_have_changed);
  EXPECT_EQ(requests, transport->requests);
  EXPECT_EQ(revision, checkpoint_test_access::revision(*session));
  EXPECT_TRUE(image == read(f.path));
}

NATIVE_SESSION_TEST(clean_tip_terminal_excludes_genesis_initialization_and_fresh_birthday)
{
  {
    fixture f; const test_chain chain; const auto control = capable(f);
    const auto transport = std::make_shared<controlled_transport>(); std::atomic<bool> stop{false};
    auto session = open_session(f, control);
    checkpoint_test_access::wallet(*session).set_minimum_height(0);
    ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
    serve(*transport, 0, 1, {chain.g});
    ASSERT_TRUE(step(*session, stop).state_may_have_changed);
    ASSERT_EQ(checkpoint_result::Saved, session->checkpoint());
    serve(*transport, 0, 1, {chain.g}); // Clean/equal metadata still must visit the genesis handler.
    const auto terminal = step(*session, stop);
    EXPECT_EQ(tools::read_only::step_status::NoProgress, terminal.status);
    EXPECT_TRUE(terminal.state_may_have_changed);
    EXPECT_EQ(session_state::OpenDirty, session->state());
    EXPECT_EQ(checkpoint_result::Saved, session->checkpoint());
  }
  {
    fixture f; const auto control = capable(f);
    const auto transport = std::make_shared<controlled_transport>(); std::atomic<bool> stop{false};
    auto session = open_session(f, control);
    checkpoint_test_access::wallet(*session).set_minimum_height(WALLET_MINIMUM_HEIGHT_UNSET_CONST);
    ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
    const auto terminal = step(*session, stop);
    EXPECT_NE(tools::read_only::step_status::NoProgress, terminal.status);
    EXPECT_FALSE(terminal.state_may_have_changed);
    EXPECT_EQ(1u, terminal.local_blocks_after);
    EXPECT_EQ(0u, transport->requests); // Existing date preparation, never the matching-tip block no-op.
    EXPECT_EQ(session_state::OpenUnchanged, session->state());
  }
}
NATIVE_SESSION_TEST(clean_tip_genuine_reopen_first_matching_tip_requires_real_checkpoint_and_capture)
{
  fixture f; const test_chain chain; const auto first = capable(f);
  const auto transport = std::make_shared<controlled_transport>(); std::atomic<bool> stop{false};
  auto original = clean_tip_session(f, first, transport, chain, stop);
  ASSERT_EQ(close_result::ReleasedUnchanged, original->close());
  original.reset();
  ASSERT_FALSE(first->storage->held);
  // A genuine strict native reopen. No post-reopen block is added and no daemon metadata is forged.
  const auto control = capable(f);
  auto session = open_session(f, control);
  ASSERT_EQ(3u, checkpoint_test_access::local_blocks(*session));
  ASSERT_EQ(session_state::OpenUnchanged, session->state());
  const auto revision = checkpoint_test_access::revision(*session);
  const auto image = read(f.path);
  ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, limits()));
  serve(*transport, 2, 3, {chain.b2});
  const auto terminal = session->step_sync(stop);
  ASSERT_EQ(sync_step_status::Stepped, terminal.status);
  ASSERT_EQ(tools::read_only::step_status::NoProgress, terminal.scan.status);
  EXPECT_TRUE(terminal.scan.state_may_have_changed);
  EXPECT_EQ(0u, terminal.scan.blocks_added);
  EXPECT_EQ(3u, terminal.scan.local_blocks_before);
  EXPECT_EQ(3u, terminal.scan.local_blocks_after);
  EXPECT_EQ(3u, terminal.scan.reported_daemon_blocks);
  EXPECT_EQ(session_state::OpenDirty, session->state());
  const auto changed_revision = checkpoint_test_access::revision(*session);
  EXPECT_GT(changed_revision, revision);
  EXPECT_TRUE(image == read(f.path)); // No implicit write or dirty clearing during the scan.
  ASSERT_EQ(checkpoint_result::Saved, session->checkpoint());
  ASSERT_EQ(session_state::OpenUnchanged, session->state());
  EXPECT_EQ(1u, control->checkpoints);
  const auto captured = first_page(*session);
  ASSERT_EQ(snapshot_status::Available, captured.status);
  ASSERT_TRUE(captured.observation != nullptr);
  EXPECT_EQ(3u, captured.observation->local_blocks);
  EXPECT_EQ(changed_revision, captured.observation->revision);
  EXPECT_TRUE(captured.observation->local_tip_hash == currency::get_block_hash(chain.b2.block_ptr->bl));
  EXPECT_EQ(checkpoint_result::Unchanged, session->checkpoint());
  EXPECT_EQ(1u, control->checkpoints);
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
}

#endif // NATIVE_SESSION_REPLACEMENT_FIXTURE

#endif
#undef NATIVE_SESSION_TEST
#undef NATIVE_SESSION_REPLACEMENT_FIXTURE
} // namespace native_session_test

