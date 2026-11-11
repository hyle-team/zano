// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
// Source-only integration candidate. Execution generates disposable accounts,
// invokes the pinned KDF and writes inside one disposable directory; it
// requires the separate reviewed runtime gate. No network, daemon or sync.
#include "gtest/gtest.h"
#include "wallet/native_wallet_provisioning.h"
#include "wallet/i_core_rpc_proxy.h"
#include "wallet/wallet2.h"
#include "currency_core/account.h"
#include "currency_core/currency_config.h"
#include "currency_core/currency_format_utils.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <fstream>
#include <list>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
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
  // Test-only access to the narrow persistence seam. Defined only here; the
  // production provision() always uses the real POSIX calls.
  struct provisioning_test_access
  {
    static provision_result provision(acquisition_kind kind, provisioning_lease& lease,
      std::unique_ptr<file_session>& out, const provisioning_io& io) noexcept
    {
      return wallet_provisioning::provision_with_io(kind, lease, out, io);
    }
  };
}}

namespace native_provisioning_test
{
namespace fs = boost::filesystem;
using namespace tools::native_session;

void require(bool condition)
{
  if (!condition) throw std::runtime_error("native provisioning disposable fixture failed");
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

std::string random_password()
{
  return epee::string_tools::pod_to_hex(crypto::rand<std::array<uint8_t, 20>>());
}

std::vector<std::string> words(const std::string& phrase)
{
  std::istringstream input(phrase);
  std::vector<std::string> result;
  std::string word;
  while (input >> word) result.push_back(word);
  return result;
}

std::string join(const std::vector<std::string>& values, size_t count)
{
  std::string result;
  for (size_t i = 0; i != count && i != values.size(); ++i)
    result += (i ? " " : "") + values[i];
  return result;
}

// Disposable account used only to produce public test vectors: a native phrase
// and the public identity the provisioned wallet must reproduce.
std::string disposable_phrase(bool auditable, const std::string& seed_password,
  public_identity& identity, bool v1 = false)
{
  epee::log_space::scoped_log_silence quiet;
  currency::account_base account;
  account.generate(auditable);
  const auto& address = account.get_public_address();
  identity = {address.spend_public_key, address.view_public_key, address.flags,
    build_profile().genesis};
  const auto phrase = account.get_seed_phrase(seed_password);
  const auto list = words(phrase);
  require(list.size() == SEED_PHRASE_V2_WORDS_COUNT);
  return v1 ? join(list, SEED_PHRASE_V1_WORDS_COUNT) : phrase;
}

const char kSentinel[] = "unrelated disposable sibling";

// One test-owned disposable root with a NEW empty private attempt directory,
// as a provider would establish before lending. The disposable root stands in
// for the provider's known durable confined root; the provider creates both
// links below it (attempts/ and attempts/attempt/). Test cleanup is the test's
// own disposal of its directory; the core under test never deletes anything.
class location final
{
public:
  location()
  {
    parent = fs::canonical(fs::temp_directory_path());
    const auto requested = parent / fs::unique_path("zano-native-provisioning-%%%%-%%%%-%%%%");
    require(fs::create_directory(requested));
    root = fs::canonical(requested);
    attempts = root / "attempts";
    require(fs::create_directory(attempts));
    attempt = attempts / "attempt";
    require(fs::create_directory(attempt));
    fs::permissions(attempt, fs::owner_all);
    wallet = attempt / "provisioned.wallet";
    write(root / "sibling-sentinel", kSentinel);
  }
  ~location()
  {
    boost::system::error_code ignored;
    fs::remove_all(root, ignored);
  }
  location(const location&) = delete;
  location& operator=(const location&) = delete;

  std::map<std::string, std::string> entries() const
  {
    std::map<std::string, std::string> result;
    for (fs::directory_iterator it(attempt), end; it != end; ++it)
    {
      const auto name = it->path().filename().string();
      result.emplace(name, fs::is_regular_file(fs::symlink_status(it->path())) ? read(it->path()) : "<other>");
    }
    return result;
  }
  bool only_wallet() const
  {
    const auto list = entries();
    return list.size() == 1 && list.count(wallet.filename().string()) == 1;
  }
  bool sibling_unchanged() const { return read(root / "sibling-sentinel") == kSentinel; }
  // Every directory holding a link the provider created for this attempt,
  // deepest first, ending at the durable root. Nothing above the root.
  std::vector<fs::path> ancestry() const { return {attempts, root}; }

  fs::path parent, root, attempts, attempt, wallet;
};

bool same_keys(const public_identity& left, const public_identity& right)
{
  return left.spend_public_key == right.spend_public_key &&
    left.view_public_key == right.view_public_key && left.flags == right.flags;
}

// Ordered observation shared by provider steps and I/O hooks. Never throws.
void note(std::vector<std::string>& events, const char* name) noexcept
{
  try { events.push_back(name); }
  catch (...) {}
}

size_t position(const std::vector<std::string>& events, const std::string& name)
{
  return static_cast<size_t>(std::find(events.begin(), events.end(), name) - events.begin());
}

// Consecutive repeats (several write calls) collapse to one entry.
std::vector<std::string> collapsed(const std::vector<std::string>& events)
{
  std::vector<std::string> result;
  for (const auto& value : events)
    if (result.empty() || result.back() != value) result.push_back(value);
  return result;
}

// The real test provider's checked persistence of one directory's entries:
// open without following links, fsync, then a checked close. An injected
// failure skips fsync and reports failure; the descriptor is still closed once.
bool sync_directory_path(const fs::path& directory, bool inject_failure)
{
#if defined(__linux__)
  const int fd = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
  if (fd < 0) return false;
  const bool synced = !inject_failure && ::fsync(fd) == 0;
  const bool closed = ::close(fd) == 0;
  return synced && closed;
#else
  (void)directory;
  (void)inject_failure;
  return false;
#endif
}

struct provider_state
{
  bool authorized = true;
  size_t authorization_reads = 0;
  size_t revoke_at_authorization_read = 0;
  std::array<commit_status, 3> results{{commit_status::Committed, commit_status::Committed,
    commit_status::Committed}};
  size_t throw_at_step = 0;  // 1-based provider step.
  std::array<size_t, 3> calls{{0, 0, 0}};
  std::array<size_t, 3> order{{0, 0, 0}};
  std::array<public_identity, 3> identities{};
  size_t sequence = 0;
  size_t backups = 0;
  bool callbacks_silenced = true;
  bool file_at_material_commit = false;
  bool released = false;
  bool effects_at_release = false;
  fs::path wallet;
  // Attempt-directory ancestry the provider must persist in commit_material.
  std::vector<fs::path> ancestry;
  size_t fail_ancestry_at = 0;  // 1-based directory sync that fails.
  size_t ancestry_synced = 0;
  std::vector<std::string> events;
  // Shared registry of bound identities; a duplicate binding is Rejected.
  std::vector<public_identity>* registry = nullptr;
  // Storage mutation after binding is committed, before the strict reopen.
  std::function<void()> after_binding;
  // Optional checkpoint capability (default: unsupported, like existing providers).
  bool checkpoint_capable = false;
  size_t checkpoints = 0;
  public_identity checkpoint_identity{};
  // Checkpoint writer outcome: Committed writes for real; Rejected refuses
  // before any byte; Unknown reports uncertainty AFTER a real replacement.
  // checkpoint_throws throws before any byte. None fakes a successful write.
  commit_status checkpoint_outcome = commit_status::Committed;
  bool checkpoint_throws = false;
};

// The test provider's checkpoint replacement of its own final wallet path:
// exclusive no-follow temporary, checked complete write, fsync, checked close,
// one rename over the target, directory fsync. Failure is uncertain.
bool replace_wallet(const fs::path& target, const char* data, size_t size)
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
    ok = ::close(fd) == 0 && ok;
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

class test_lease final : public provisioning_lease
{
public:
  test_lease(acquisition_kind kind, provider_state& state) : m_kind(kind), m_state(state) {}
  ~test_lease() noexcept override
  {
    m_state.released = true;
    m_state.effects_at_release = effects_possible();
  }
  bool authorization_current() const noexcept override
  {
    ++m_state.authorization_reads;
    if (m_state.revoke_at_authorization_read &&
        m_state.authorization_reads >= m_state.revoke_at_authorization_read)
      m_state.authorized = false;
    return m_state.authorized;
  }
  bool checkpoint_supported() const noexcept override { return m_state.checkpoint_capable; }
  provisioning_material value;
  std::unique_ptr<generated_backup> backup;
protected:
  acquisition_kind provisioning_kind() const noexcept override { return m_kind; }
  const provisioning_material& material() const override { return value; }
  commit_status commit_material(const public_identity& expected) override
  {
    note(m_state.events, "material:invoked");
    // Provider obligation: make every directory link created for this attempt
    // durably reachable from the durable root BEFORE recording material. A
    // failed or uncertain sync is Unknown; nothing is recorded.
    for (const auto& directory : m_state.ancestry)
    {
      if (!sync_directory_path(directory, m_state.fail_ancestry_at == m_state.ancestry_synced + 1))
      {
        note(m_state.events, "ancestor:failed");
        return commit_status::Unknown;
      }
      ++m_state.ancestry_synced;
      note(m_state.events, ("ancestor:" + std::to_string(m_state.ancestry_synced)).c_str());
    }
    return step(0, expected);
  }
  commit_status commit_binding(const public_identity& expected) override { return step(1, expected); }
  commit_status commit_ready(const public_identity& verified) override { return step(2, verified); }
  void retain_backup(std::unique_ptr<generated_backup> value) noexcept override
  {
    ++m_state.backups;
    m_state.callbacks_silenced = m_state.callbacks_silenced && epee::log_space::is_thread_log_silenced();
    backup = std::move(value);
  }
  commit_status replace_checkpoint(const public_identity& bound, const char* image, size_t size) override
  {
    ++m_state.checkpoints;
    m_state.checkpoint_identity = bound;
    m_state.callbacks_silenced = m_state.callbacks_silenced && epee::log_space::is_thread_log_silenced();
    // Only the Ready identity this provider committed, only after Ready.
    const auto& ready = m_state.identities[2];
    if (!m_state.checkpoint_capable || m_state.calls[2] != 1 || !same_keys(bound, ready) ||
        bound.stored_genesis != ready.stored_genesis)
      return commit_status::Rejected;
    if (m_state.checkpoint_throws) throw std::runtime_error("synthetic checkpoint writer failure");
    if (m_state.checkpoint_outcome == commit_status::Rejected) return commit_status::Rejected;
    if (!replace_wallet(m_state.wallet, image, size)) return commit_status::Unknown;
    return m_state.checkpoint_outcome == commit_status::Committed ? commit_status::Committed : commit_status::Unknown;
  }
private:
  commit_status step(size_t index, const public_identity& identity)
  {
    static const char* const names[] = {"material", "binding", "ready"};
    ++m_state.calls[index];
    m_state.order[index] = ++m_state.sequence;
    m_state.identities[index] = identity;
    m_state.callbacks_silenced = m_state.callbacks_silenced && epee::log_space::is_thread_log_silenced();
    note(m_state.events, names[index]);
    if (index == 0) m_state.file_at_material_commit = fs::is_regular_file(m_state.wallet);
    if (m_state.throw_at_step == index + 1) throw std::runtime_error("synthetic provider failure");
    const auto result = m_state.results[index];
    if (index == 1 && result == commit_status::Committed)
    {
      if (m_state.registry)
      {
        for (const auto& bound : *m_state.registry)
          if (same_keys(bound, identity)) return commit_status::Rejected;  // Definitely not bound.
        m_state.registry->push_back(identity);
      }
      if (m_state.after_binding) m_state.after_binding();
    }
    return result;
  }
  acquisition_kind m_kind;
  provider_state& m_state;
};

void configure(test_lease& lease, provider_state& state, const location& where,
  const std::string& password, const std::string& phrase, const std::string& seed_password)
{
  state.wallet = where.wallet;
  state.ancestry = where.ancestry();
  lease.value.path = where.wallet.wstring();
  lease.value.wallet_password.assign(password.data(), password.size());
  lease.value.phrase.assign(phrase.data(), phrase.size());
  lease.value.seed_password.assign(seed_password.data(), seed_password.size());
}

std::unique_ptr<test_lease> make_lease(acquisition_kind kind, provider_state& state,
  const location& where, const std::string& password, const std::string& phrase = std::string(),
  const std::string& seed_password = std::string())
{
  std::unique_ptr<test_lease> lease(new test_lease(kind, state));
  configure(*lease, state, where, password, phrase, seed_password);
  return lease;
}

// Independent strict reopen with an expectation the test holds itself.
class open_lease final : public material_lease
{
public:
  open_lease(open_material value, bool checkpoint)
    : m_value(std::move(value)), m_checkpoint(checkpoint) {}
  bool authorization_current() const noexcept override { return true; }
  bool checkpoint_supported() const noexcept override { return m_checkpoint; }
protected:
  const open_material& material() const override { return m_value; }
  commit_status replace_checkpoint(const public_identity& bound, const char* image, size_t size) override
  {
    if (!m_checkpoint || !same_keys(bound, m_value.expected) ||
        bound.stored_genesis != m_value.expected.stored_genesis)
      return commit_status::Rejected;
    return replace_wallet(fs::path(m_value.path), image, size) ? commit_status::Committed : commit_status::Unknown;
  }
private:
  open_material m_value;
  const bool m_checkpoint;
};

open_result reopen(const fs::path& wallet, const std::string& password,
  const public_identity& expected, std::unique_ptr<file_session>& out, bool checkpoint = false)
{
  std::unique_ptr<material_lease> lease(new open_lease({wallet.wstring(), password, expected}, checkpoint));
  return file_session::acquire(acquisition_kind::OpenExisting, lease, out);
}

// Controlled in-process block transport: one canned response, no endpoint.
using blocks_request = currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request;
using blocks_response = currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response;
using block_entry = currency::block_direct_data_entry;
class sync_transport final : public tools::i_core_proxy
{
public:
  blocks_response response{};
  size_t requests = 0;
  bool call_COMMAND_RPC_GET_BLOCKS_DIRECT(const blocks_request&, blocks_response& out) override
  {
    ++requests;
    out = response;
    return true;
  }
  void serve(uint64_t start, uint64_t current, std::list<block_entry> blocks)
  {
    response = blocks_response{};
    response.status = API_RETURN_CODE_OK;
    response.start_height = start;
    response.current_height = current;
    response.blocks = std::move(blocks);
  }
};

// The compiled genesis block, whose hash every provisioned wallet stores.
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

// Synthetic empty-coinbase child with a public (base point) transaction key.
block_entry child(const block_entry& parent)
{
  crypto::public_key point{};
  require(epee::string_tools::hex_to_pod(
    std::string("5866666666666666666666666666666666666666666666666666666666666666"), point));
  auto block = std::make_shared<currency::block_extended_info>();
  block->bl = currency::block{};
  block->bl.major_version = 1;
  block->bl.prev_id = currency::get_block_hash(parent.block_ptr->bl);
  block->bl.miner_tx.version = TRANSACTION_VERSION_INITAL;
  block->bl.miner_tx.vin.emplace_back(currency::txin_gen{currency::get_block_height(parent.block_ptr->bl) + 1});
  block->bl.miner_tx.extra.emplace_back(point);
  return {block, std::make_shared<currency::transaction_chain_entry>(),
    std::list<std::shared_ptr<const currency::transaction_chain_entry>>{}};
}

void copy_phrase(void* context, const char* phrase, size_t size) noexcept
{
  try { static_cast<std::string*>(context)->assign(phrase, size); }
  catch (...) {}
}

void count_visit(void* context, const char*, size_t) noexcept
{
  ++*static_cast<size_t*>(context);
}

void expect_no_effect(const provision_result& result, const test_lease& lease,
  const provider_state& state, const location& where)
{
  EXPECT_EQ(provision_status::NotStarted, result.status);
  EXPECT_FALSE(lease.effects_possible());
  EXPECT_TRUE(where.entries().empty());
  EXPECT_EQ(0u, state.calls[0] + state.calls[1] + state.calls[2]);
  EXPECT_EQ(0u, state.ancestry_synced);
  EXPECT_EQ(0u, state.backups);
  EXPECT_TRUE(where.sibling_unchanged());
}

// Destroys a lease constructed in caller storage on every exit path.
struct placed_lease final
{
  test_lease* lease = nullptr;
  ~placed_lease() { reset(); }
  void reset() noexcept
  {
    if (lease) lease->~test_lease();
    lease = nullptr;
  }
};

#if defined(__linux__)
// Bounded faults around the production Phase B descriptor calls, reached only
// through provisioning_test_access. Each hook forwards to the real POSIX call
// with the production flags unless its fault is armed, and records its order.
struct io_faults
{
  std::vector<std::string>* events = nullptr;
  int create_errno = 0;        // create fails without touching the directory
  int write_errno = 0;         // once short_write bytes are persisted
  size_t short_write = 0;
  size_t chunk = 0;            // a successful write persists at most chunk bytes
  size_t eintr = 0;            // EINTR returns interleaved with progress
  int sync_file_errno = 0;
  int close_errno = 0;         // reported AFTER the descriptor is really closed
  int sync_directory_errno = 0;
  size_t write_calls = 0, eintr_returned = 0, closes = 0;
  int closed_fd = -1;
  std::string persisted;       // Exactly the bytes the kernel accepted, in order.
  bool capture_failed = false;
};

io_faults& faults_of(void* context) noexcept { return *static_cast<io_faults*>(context); }

int fault_create(void* context, int directory, const char* leaf, int flags, unsigned mode) noexcept
{
  auto& f = faults_of(context);
  if (f.events) note(*f.events, "create");
  if (f.create_errno)
  {
    errno = f.create_errno;
    return -1;
  }
  return ::openat(directory, leaf, flags, static_cast<mode_t>(mode));
}

long fault_write(void* context, int fd, const char* data, size_t size) noexcept
{
  auto& f = faults_of(context);
  ++f.write_calls;
  if (f.events) note(*f.events, "write");
  if (f.eintr_returned < f.eintr && f.write_calls % 2 == 1)
  {
    ++f.eintr_returned;
    errno = EINTR;
    return -1;
  }
  size_t allowed = size;
  // Keep progress tiny only while interleaving EINTR; use normal chunks after
  // the final injection, without depending on the serialized image size.
  if (f.eintr_returned < f.eintr) allowed = std::min(allowed, size_t{1});
  if (f.chunk) allowed = std::min(allowed, f.chunk);
  if (f.write_errno)
  {
    if (f.persisted.size() >= f.short_write)
    {
      errno = f.write_errno;
      return -1;
    }
    allowed = std::min(allowed, f.short_write - f.persisted.size());
  }
  const ssize_t written = ::write(fd, data, allowed);
  if (written > 0)
  {
    try { f.persisted.append(data, static_cast<size_t>(written)); }
    catch (...) { f.capture_failed = true; }
  }
  return static_cast<long>(written);
}

int fault_sync_file(void* context, int fd) noexcept
{
  auto& f = faults_of(context);
  if (f.events) note(*f.events, "sync_file");
  if (f.sync_file_errno)
  {
    errno = f.sync_file_errno;
    return -1;
  }
  return ::fsync(fd);
}

int fault_close_file(void* context, int fd) noexcept
{
  auto& f = faults_of(context);
  if (f.events) note(*f.events, "close_file");
  // Linux releases the descriptor even when close reports an error. The
  // simulated failure therefore closes it exactly once, then reports EIO, so
  // no later teardown can close a reused descriptor number.
  const int result = ::close(fd);
  ++f.closes;
  f.closed_fd = fd;
  if (f.close_errno)
  {
    errno = f.close_errno;
    return -1;
  }
  return result;
}

int fault_sync_directory(void* context, int fd) noexcept
{
  auto& f = faults_of(context);
  if (f.events) note(*f.events, "sync_directory");
  if (f.sync_directory_errno)
  {
    errno = f.sync_directory_errno;
    return -1;
  }
  return ::fsync(fd);
}

provision_result provision_through(io_faults& faults, acquisition_kind kind,
  provisioning_lease& lease, std::unique_ptr<file_session>& out)
{
  provisioning_io io;
  io.context = &faults;
  io.create = &fault_create;
  io.write = &fault_write;
  io.sync_file = &fault_sync_file;
  io.close_file = &fault_close_file;
  io.sync_directory = &fault_sync_directory;
  return provisioning_test_access::provision(kind, lease, out, io);
}
#endif

// Returns the provisioned identity, releasing the session before the lease.
public_identity provision_ready(acquisition_kind kind, const location& where,
  const std::string& password, const std::string& phrase = std::string(),
  const std::string& seed_password = std::string(), provider_state* observed = nullptr)
{
  provider_state local;
  provider_state& state = observed ? *observed : local;
  auto lease = make_lease(kind, state, where, password, phrase, seed_password);
  std::unique_ptr<file_session> session;
  const auto result = wallet_provisioning::provision(kind, *lease, session);
  EXPECT_EQ(provision_status::Ready, result.status);
  EXPECT_EQ(provision_detail::None, result.detail);
  public_identity identity{};
  if (session)
  {
    const auto read = session->get_receive();
    EXPECT_EQ(receive_status::Available, read.status);
    identity = read.identity;
    EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  }
  session.reset();
  lease.reset();
  return identity;
}

}  // namespace native_provisioning_test

using namespace native_provisioning_test;

#if !((!defined(TESTNET) && CURRENCY_FORMATION_VERSION == 84) || \
      (defined(TESTNET) && CURRENCY_FORMATION_VERSION == 103))
TEST(native_wallet_provisioning, unsupported_compiled_profile_has_no_effect)
{
  location where;
  provider_state state;
  auto lease = make_lease(acquisition_kind::CreateNativeZano, state, where, random_password());
  std::unique_ptr<file_session> session;
  const auto result = wallet_provisioning::provision(acquisition_kind::CreateNativeZano, *lease, session);
  EXPECT_EQ(provision_detail::UnsupportedCapability, result.detail);
  expect_no_effect(result, *lease, state, where);
}
#else

TEST(native_wallet_provisioning, create_persists_reopens_and_publishes_ready)
{
  const auto& profile = build_profile();
  ASSERT_EQ(profile_status::Available, profile.status);
  location where;
  provider_state state;
  const auto password = random_password();
  auto lease = make_lease(acquisition_kind::CreateNativeZano, state, where, password);
  std::unique_ptr<file_session> session;
  const auto result = wallet_provisioning::provision(acquisition_kind::CreateNativeZano, *lease, session);
  ASSERT_EQ(provision_status::Ready, result.status);
  EXPECT_EQ(provision_detail::None, result.detail);
  ASSERT_TRUE(session != nullptr);
  EXPECT_EQ(session_state::OpenUnchanged, session->state());
  EXPECT_TRUE(lease->effects_possible());

  // Material, binding, Ready: each exactly once and in order, after the file.
  EXPECT_EQ((std::array<size_t, 3>{{1, 1, 1}}), state.calls);
  EXPECT_EQ((std::array<size_t, 3>{{1, 2, 3}}), state.order);
  EXPECT_TRUE(state.file_at_material_commit);

  const auto read = session->get_receive();
  ASSERT_EQ(receive_status::Available, read.status);
  for (const auto& identity : state.identities)
  {
    EXPECT_TRUE(same_keys(identity, read.identity));
    EXPECT_EQ(profile.genesis, identity.stored_genesis);
  }
  EXPECT_EQ(0, read.identity.flags);
  EXPECT_EQ(profile.genesis, read.identity.stored_genesis);
  {
    currency::account_public_address parsed{};
    currency::payment_id_t payment_id;
    ASSERT_TRUE(currency::get_account_address_and_payment_id_from_str(parsed, payment_id, read.address));
    EXPECT_TRUE(payment_id.empty());
    EXPECT_EQ(read.identity.spend_public_key, parsed.spend_public_key);
    EXPECT_EQ(read.identity.view_public_key, parsed.view_public_key);
    EXPECT_EQ(read.address, currency::get_account_address_as_str(parsed));
  }
  // Only the wallet: no temp name, .outkey2ki sidecar or voting file.
  EXPECT_TRUE(where.only_wallet());
  EXPECT_TRUE(where.sibling_unchanged());
  // The created file is owner read/write only: no group or other access.
  {
    const auto mode = fs::status(where.wallet).permissions();
    EXPECT_EQ(static_cast<int>(fs::owner_read | fs::owner_write), static_cast<int>(mode & fs::owner_all));
    EXPECT_EQ(0, static_cast<int>(mode & (fs::group_all | fs::others_all)));
  }
  // The provider persisted both created directory links (deepest first) up
  // to its durable root before recording material, then binding and Ready.
  EXPECT_EQ(2u, state.ancestry_synced);
  EXPECT_EQ((std::vector<std::string>{"material:invoked", "ancestor:1", "ancestor:2",
    "material", "binding", "ready"}), state.events);

  // The generated backup is a native 26-word phrase for exactly this account,
  // lent only through this lease lifetime while it is authorized.
  ASSERT_TRUE(lease->backup != nullptr);
  EXPECT_EQ(1u, state.backups);
  std::string lent;
  ASSERT_TRUE(lease->backup->lend(*lease, copy_phrase, &lent));
  EXPECT_EQ(static_cast<size_t>(SEED_PHRASE_V2_WORDS_COUNT), words(lent).size());
  {
    epee::log_space::scoped_log_silence quiet;
    currency::account_base check;
    ASSERT_TRUE(check.restore_from_seed_phrase(lent, std::string()));
    EXPECT_EQ(read.identity.spend_public_key, check.get_public_address().spend_public_key);
    EXPECT_EQ(read.identity.view_public_key, check.get_public_address().view_public_key);
  }
  lent.assign(lent.size(), '\0');
  {
    provider_state other_state;
    test_lease other(acquisition_kind::CreateNativeZano, other_state);
    EXPECT_FALSE(lease->backup->lend(other, copy_phrase, &lent));
  }
  state.authorized = false;
  EXPECT_FALSE(lease->backup->lend(*lease, copy_phrase, &lent));
  state.authorized = true;
  EXPECT_EQ(3u, state.sequence); // Lending acknowledged nothing.
  EXPECT_TRUE(state.callbacks_silenced);

  // The session borrows the lease: release it first, then the lease.
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  session.reset();
  lease.reset();
  EXPECT_TRUE(state.released && state.effects_at_release);

  std::unique_ptr<file_session> again;
  EXPECT_EQ(open_result::Opened, reopen(where.wallet, password, read.identity, again));
  ASSERT_TRUE(again != nullptr);
  EXPECT_EQ(read.address, again->get_receive().address);
  again.reset();
  EXPECT_EQ(open_result::WrongPassword, reopen(where.wallet, random_password(), read.identity, again));
  EXPECT_FALSE(again != nullptr);
  EXPECT_TRUE(where.only_wallet());
}

TEST(native_wallet_provisioning, generated_backup_is_bound_to_one_lease_lifetime)
{
  location where;
  provider_state first_state;
  const auto password = random_password();
  // Lease A and, later, lease B are constructed in the SAME storage, so a
  // raw-address binding would accept B.
  alignas(test_lease) unsigned char storage[sizeof(test_lease)];
  placed_lease placed;
  placed.lease = new (storage) test_lease(acquisition_kind::CreateNativeZano, first_state);
  test_lease* const first = placed.lease;
  configure(*first, first_state, where, password, std::string(), std::string());
  std::unique_ptr<file_session> session;
  ASSERT_EQ(provision_status::Ready,
    wallet_provisioning::provision(acquisition_kind::CreateNativeZano, *first, session).status);
  ASSERT_TRUE(session != nullptr);
  ASSERT_TRUE(first->backup != nullptr);
  const auto identity = session->get_receive().identity;

  // Normal same-lifetime access while authorized.
  size_t visits = 0;
  std::string lent;
  EXPECT_TRUE(first->backup->lend(*first, count_visit, &visits));
  EXPECT_EQ(1u, visits);
  ASSERT_TRUE(first->backup->lend(*first, copy_phrase, &lent));
  // Revoked authority on the same lifetime: no visitor call.
  first_state.authorized = false;
  EXPECT_FALSE(first->backup->lend(*first, count_visit, &visits));
  first_state.authorized = true;
  // A different, live and authorized lease: no visitor call.
  {
    provider_state other_state;
    test_lease other(acquisition_kind::CreateNativeZano, other_state);
    EXPECT_TRUE(other.authorization_current());
    EXPECT_FALSE(first->backup->lend(other, count_visit, &visits));
  }
  EXPECT_EQ(1u, visits);

  // The provider retains the backup outside the lease, then the lease ends
  // (session first, as required).
  std::unique_ptr<generated_backup> retained = std::move(first->backup);
  const void* const first_address = first;
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  session.reset();
  placed.reset();
  EXPECT_TRUE(first_state.released);

  // A fresh authorized lease at the same address is a new lifetime.
  provider_state second_state;
  placed.lease = new (storage) test_lease(acquisition_kind::CreateNativeZano, second_state);
  test_lease* const second = placed.lease;
  EXPECT_EQ(first_address, static_cast<const void*>(second));
  EXPECT_TRUE(second->authorization_current());
  EXPECT_FALSE(retained->lend(*second, count_visit, &visits));
  EXPECT_EQ(1u, visits);
  EXPECT_EQ(0u, second_state.sequence);
  placed.reset();
  retained.reset();

  // Viewing and restoring never acknowledge anything: restoring the lent
  // phrase elsewhere reproduces the identity and makes no call on the
  // creating provider, whose only steps remain material, binding and Ready.
  location restore_where;
  provider_state restore_state;
  const auto restored = provision_ready(acquisition_kind::RestoreNativeZano, restore_where,
    random_password(), lent, std::string(), &restore_state);
  lent.assign(lent.size(), '\0');
  EXPECT_TRUE(same_keys(identity, restored));
  EXPECT_EQ(0u, restore_state.backups);
  EXPECT_EQ(3u, first_state.sequence);
  EXPECT_EQ(1u, first_state.backups);
  EXPECT_TRUE(where.only_wallet());
}

TEST(native_wallet_provisioning, restore_native_v2_and_v1_phrases_with_optional_seed_password)
{
  for (const auto& seed_password : {std::string(), std::string("SyntheticSeedPassword1")})
  {
    for (bool v1 : {false, true})
    {
      public_identity original{};
      const auto phrase = disposable_phrase(false, seed_password, original, v1);
      location where;
      provider_state state;
      const auto identity = provision_ready(acquisition_kind::RestoreNativeZano, where,
        random_password(), phrase, seed_password, &state);
      EXPECT_TRUE(same_keys(original, identity));
      EXPECT_EQ(build_profile().genesis, identity.stored_genesis);
      // Restoring never generates or hands back backup material.
      EXPECT_EQ(0u, state.backups);
      EXPECT_TRUE(where.only_wallet());
    }
  }
}

TEST(native_wallet_provisioning, compiled_genesis_is_installed_before_the_first_persisted_byte)
{
  const auto& profile = build_profile();
  location where;
  provider_state state;
  const auto password = random_password();
  const auto identity = provision_ready(acquisition_kind::CreateNativeZano, where, password,
    std::string(), std::string(), &state);
  // The provider saw the compiled genesis when the file was first committed.
  EXPECT_EQ(profile.genesis, state.identities[0].stored_genesis);
#if defined(TESTNET)
  // The legacy generate()/restore() path would have written the MAINNET literal.
  EXPECT_NE(currency::gdefault_genesis, profile.genesis);
#endif
  // The strict reader compares the FILE's stored genesis with this expectation.
  std::unique_ptr<file_session> again;
  EXPECT_EQ(open_result::Opened, reopen(where.wallet, password, identity, again));
  // One O_EXCL creation, never a later rewrite: exactly one entry.
  EXPECT_TRUE(where.only_wallet());
}

TEST(native_wallet_provisioning, seed_password_mismatch_never_restores_the_original_identity)
{
  const std::string seed_password = "SyntheticSeedPassword1";
  {
    public_identity original{};
    const auto phrase = disposable_phrase(false, seed_password, original);
    {
      // Password-protected phrase without its seed password.
      location where;
      provider_state state;
      auto lease = make_lease(acquisition_kind::RestoreNativeZano, state, where, random_password(), phrase);
      std::unique_ptr<file_session> session;
      const auto result = wallet_provisioning::provision(acquisition_kind::RestoreNativeZano, *lease, session);
      EXPECT_EQ(provision_detail::SeedPasswordMismatch, result.detail);
      expect_no_effect(result, *lease, state, where);
    }
    // A different password on a V2 phrase normally fails the core checksum. The
    // core scheme has a small false-accept rate; even then the original
    // identity must not be produced.
    location where;
    provider_state state;
    auto lease = make_lease(acquisition_kind::RestoreNativeZano, state, where, random_password(),
      phrase, "DifferentSeedPassword2");
    std::unique_ptr<file_session> session;
    const auto result = wallet_provisioning::provision(acquisition_kind::RestoreNativeZano, *lease, session);
    if (result.status == provision_status::Ready)
    {
      ASSERT_TRUE(session != nullptr);
      EXPECT_FALSE(same_keys(original, session->get_receive().identity));
      session.reset();
    }
    else
    {
      EXPECT_EQ(provision_detail::InvalidPhrase, result.detail);
      expect_no_effect(result, *lease, state, where);
    }
  }
  {
    public_identity original{};
    const auto phrase = disposable_phrase(false, std::string(), original);
    location where;
    provider_state state;
    auto lease = make_lease(acquisition_kind::RestoreNativeZano, state, where, random_password(), phrase, seed_password);
    std::unique_ptr<file_session> session;
    const auto result = wallet_provisioning::provision(acquisition_kind::RestoreNativeZano, *lease, session);
    EXPECT_EQ(provision_detail::SeedPasswordMismatch, result.detail);
    expect_no_effect(result, *lease, state, where);
  }
}

TEST(native_wallet_provisioning, unsupported_inputs_and_kinds_have_no_effect)
{
  public_identity identity{};
  const auto phrase = disposable_phrase(false, std::string(), identity);
  const auto list = words(phrase);
  std::string tracking;
  {
    epee::log_space::scoped_log_silence quiet;
    currency::account_base account;
    account.generate(false);
    tracking = account.get_tracking_seed();
  }
  public_identity auditable_identity{};
  const auto auditable = disposable_phrase(true, std::string(), auditable_identity);
  struct variant
  {
    acquisition_kind call;
    acquisition_kind lease;
    std::string phrase;
    provision_detail expected;
  };
  const std::vector<variant> variants{
    {acquisition_kind::RestoreNativeZano, acquisition_kind::RestoreNativeZano, tracking, provision_detail::UnsupportedInput},
    {acquisition_kind::RestoreNativeZano, acquisition_kind::RestoreNativeZano, join(list, 12), provision_detail::UnsupportedInput},
    {acquisition_kind::RestoreNativeZano, acquisition_kind::RestoreNativeZano, join(list, 24), provision_detail::UnsupportedInput},
    {acquisition_kind::RestoreNativeZano, acquisition_kind::RestoreNativeZano, join(list, 20), provision_detail::InvalidPhrase},
    {acquisition_kind::RestoreNativeZano, acquisition_kind::RestoreNativeZano, std::string(2000, 'a'), provision_detail::InvalidPhrase},
    {acquisition_kind::RestoreNativeZano, acquisition_kind::RestoreNativeZano, std::string(), provision_detail::InvalidPhrase},
    {acquisition_kind::RestoreNativeZano, acquisition_kind::RestoreNativeZano, auditable, provision_detail::UnsupportedCapability},
    {acquisition_kind::OpenExisting, acquisition_kind::OpenExisting, std::string(), provision_detail::UnsupportedCapability},
    {acquisition_kind::RestoreNativeZano, acquisition_kind::CreateNativeZano, phrase, provision_detail::InvalidMaterial},
    {acquisition_kind::CreateNativeZano, acquisition_kind::CreateNativeZano, phrase, provision_detail::InvalidMaterial},
  };
  for (const auto& value : variants)
  {
    location where;
    provider_state state;
    auto lease = make_lease(value.lease, state, where, random_password(), value.phrase);
    std::unique_ptr<file_session> session;
    const auto result = wallet_provisioning::provision(value.call, *lease, session);
    EXPECT_EQ(value.expected, result.detail);
    EXPECT_FALSE(session != nullptr);
    expect_no_effect(result, *lease, state, where);
  }
}

TEST(native_wallet_provisioning, occupied_locations_are_never_overwritten)
{
  for (const std::string name : {"provisioned.wallet", "provisioned.outkey2ki",
                                 "provisioned.wallet.newtmp_1", "provisioned.wallet.oldtmp_1",
                                 "unrelated"})
  {
    location where;
    write(where.attempt / name, "existing disposable entry");
    const auto before = where.entries();
    provider_state state;
    auto lease = make_lease(acquisition_kind::CreateNativeZano, state, where, random_password());
    std::unique_ptr<file_session> session;
    const auto result = wallet_provisioning::provision(acquisition_kind::CreateNativeZano, *lease, session);
    EXPECT_EQ(provision_status::NotStarted, result.status);
    EXPECT_EQ(provision_detail::OccupiedOutput, result.detail);
    EXPECT_FALSE(session != nullptr);
    EXPECT_FALSE(lease->effects_possible());
    EXPECT_EQ(before, where.entries());
  }
  // An occupied output slot is refused before the provider material is read.
  location first;
  provider_state first_state;
  auto first_lease = make_lease(acquisition_kind::CreateNativeZano, first_state, first, random_password());
  std::unique_ptr<file_session> occupied;
  ASSERT_EQ(provision_status::Ready,
    wallet_provisioning::provision(acquisition_kind::CreateNativeZano, *first_lease, occupied).status);
  location second;
  provider_state second_state;
  auto second_lease = make_lease(acquisition_kind::CreateNativeZano, second_state, second, random_password());
  const auto result = wallet_provisioning::provision(acquisition_kind::CreateNativeZano, *second_lease, occupied);
  EXPECT_EQ(provision_detail::OccupiedOutput, result.detail);
  expect_no_effect(result, *second_lease, second_state, second);
  occupied.reset();
}

TEST(native_wallet_provisioning, invalid_locations_and_credentials_have_no_effect)
{
  for (int variant = 0; variant != 6; ++variant)
  {
    location where;
    provider_state state;
    auto password = random_password();
    auto lease = make_lease(acquisition_kind::CreateNativeZano, state, where, password);
    if (variant == 0) lease->value.path = L"relative/provisioned.wallet";
    if (variant == 1) lease->value.path = (where.attempt / "missing" / "provisioned.wallet").wstring();
    if (variant == 2)
    {
      fs::create_symlink(where.attempt, where.root / "link");
      lease->value.path = (where.root / "link" / "provisioned.wallet").wstring();
    }
    if (variant == 3) fs::permissions(where.attempt, fs::add_perms | fs::group_write);
    if (variant == 4) lease->value.wallet_password.clear();
    if (variant == 5) lease->value.path = where.attempt.wstring() + L"/";
    std::unique_ptr<file_session> session;
    const auto result = wallet_provisioning::provision(acquisition_kind::CreateNativeZano, *lease, session);
    EXPECT_EQ(provision_detail::InvalidMaterial, result.detail);
    expect_no_effect(result, *lease, state, where);
  }
}

TEST(native_wallet_provisioning, revocation_at_each_authorization_sample)
{
  bool not_started = false, recovery = false, revoked_ready = false, ready = false;
  for (size_t at = 1; at != 64 && !ready; ++at)
  {
    location where;
    provider_state state;
    state.revoke_at_authorization_read = at;
    auto lease = make_lease(acquisition_kind::CreateNativeZano, state, where, random_password());
    std::unique_ptr<file_session> session;
    const auto result = wallet_provisioning::provision(acquisition_kind::CreateNativeZano, *lease, session);
    if (result.status == provision_status::Ready)
    {
      ready = true;
      EXPECT_TRUE(state.authorized && session != nullptr);
      session.reset();
      continue;
    }
    EXPECT_FALSE(session != nullptr);
    EXPECT_EQ(provision_detail::AuthorizationUnavailable, result.detail);
    switch (result.status)
    {
      case provision_status::NotStarted:
        not_started = true;
        expect_no_effect(result, *lease, state, where);
        break;
      case provision_status::RecoveryRequired:
        recovery = true;
        EXPECT_TRUE(lease->effects_possible());
        EXPECT_TRUE(where.only_wallet()); // Retained, never deleted.
        EXPECT_EQ(0u, state.calls[2]);
        EXPECT_EQ(0u, state.backups);
        break;
      case provision_status::ReadyAccessRevoked:
      {
        revoked_ready = true;
        EXPECT_EQ(1u, state.calls[2]);
        // Ready committed. The creating-session backup was handed to this
        // lease but is not lendable without authorization, and it expires
        // with the lease; the encrypted wallet still holds the native seed.
        EXPECT_EQ(1u, state.backups);
        size_t visits = 0;
        EXPECT_FALSE(lease->backup && lease->backup->lend(*lease, count_visit, &visits));
        EXPECT_EQ(0u, visits);
        EXPECT_TRUE(where.only_wallet());
        break;
      }
      default:
        ADD_FAILURE() << "unexpected provisioning status at authorization sample";
    }
  }
  EXPECT_TRUE(not_started && recovery && revoked_ready && ready);
}

TEST(native_wallet_provisioning, provider_step_failure_retains_material_and_is_never_retried)
{
  for (size_t step = 0; step != 3; ++step)
  {
    for (int fault = 0; fault != 3; ++fault)
    {
      location where;
      provider_state state;
      if (fault == 0) state.results[step] = commit_status::Rejected;
      if (fault == 1) state.results[step] = commit_status::Unknown;
      if (fault == 2) state.throw_at_step = step + 1;
      auto lease = make_lease(acquisition_kind::CreateNativeZano, state, where, random_password());
      std::unique_ptr<file_session> session;
      const auto result = wallet_provisioning::provision(acquisition_kind::CreateNativeZano, *lease, session);
      EXPECT_EQ(fault == 0 ? provision_status::RecoveryRequired : provision_status::OutcomeUnknown, result.status);
      EXPECT_EQ(fault == 0 ? provision_detail::ProviderRejected : provision_detail::ProviderUncertain, result.detail);
      EXPECT_FALSE(session != nullptr);
      EXPECT_TRUE(lease->effects_possible());
      EXPECT_EQ(1u, state.calls[step]);
      for (size_t later = step + 1; later < 3; ++later) EXPECT_EQ(0u, state.calls[later]);
      EXPECT_EQ(0u, state.backups);
      EXPECT_TRUE(where.only_wallet());
      const auto before = where.entries();
      // The same attempt is never provisioned again, even if asked.
      const auto again = wallet_provisioning::provision(acquisition_kind::CreateNativeZano, *lease, session);
      EXPECT_EQ(provision_status::RecoveryRequired, again.status);
      EXPECT_EQ(provision_detail::OccupiedOutput, again.detail);
      EXPECT_FALSE(session != nullptr);
      EXPECT_EQ(1u, state.calls[step]);
      EXPECT_EQ(before, where.entries());
    }
  }
}

// Shared post-conditions for an attempt that stopped after effects began:
// nothing published, the lease kept by its owner with effects recorded,
// unrelated files untouched, and the same lease refused without any new
// descriptor operation, provider call or change in the attempt directory.
void expect_retained_and_never_retried(test_lease& lease, provider_state& state,
  const location& where, const std::unique_ptr<file_session>& session)
{
  EXPECT_FALSE(session != nullptr);
  EXPECT_TRUE(lease.effects_possible());
  EXPECT_FALSE(state.released);
  EXPECT_EQ(0u, state.calls[2]);
  EXPECT_EQ(0u, state.backups);
  EXPECT_TRUE(where.sibling_unchanged());
  const auto entries = where.entries();
  const auto events = state.events.size();
  const auto calls = state.calls;
  std::unique_ptr<file_session> again;
  const auto repeated = wallet_provisioning::provision(acquisition_kind::CreateNativeZano, lease, again);
  EXPECT_EQ(provision_status::RecoveryRequired, repeated.status);
  EXPECT_EQ(provision_detail::OccupiedOutput, repeated.detail);
  EXPECT_FALSE(again != nullptr);
  EXPECT_EQ(entries, where.entries());
  EXPECT_EQ(events, state.events.size());
  EXPECT_EQ(calls, state.calls);
}

TEST(native_wallet_provisioning, provider_ancestry_persistence_fault_blocks_material_binding_and_ready)
{
  for (size_t at = 1; at <= 2; ++at)
  {
    SCOPED_TRACE(at);
    location where;
    provider_state state;
    state.fail_ancestry_at = at;
    auto lease = make_lease(acquisition_kind::CreateNativeZano, state, where, random_password());
    std::unique_ptr<file_session> session;
    const auto result = wallet_provisioning::provision(acquisition_kind::CreateNativeZano, *lease, session);
    EXPECT_EQ(provision_status::OutcomeUnknown, result.status);
    EXPECT_EQ(provision_detail::ProviderUncertain, result.detail);
    // Earlier links were synced; the failed one stopped everything, and
    // material was never recorded, so binding and Ready never ran.
    EXPECT_EQ(at - 1, state.ancestry_synced);
    EXPECT_EQ((std::array<size_t, 3>{{0, 0, 0}}), state.calls);
    ASSERT_FALSE(state.events.empty());
    EXPECT_EQ("ancestor:failed", state.events.back());
    EXPECT_EQ(state.events.size(), position(state.events, "material"));
    // The complete, fsynced wallet is kept for recovery.
    EXPECT_TRUE(where.only_wallet());
    expect_retained_and_never_retried(*lease, state, where, session);
  }
}

TEST(native_wallet_provisioning, strict_reopen_rejects_missing_corrupt_or_substituted_material)
{
  const auto password = random_password();
  // Donor: a different, genuine Ready wallet under the SAME wallet password
  // and compiled genesis, as a substitution would present it.
  location donor_where;
  const auto donor_identity = provision_ready(acquisition_kind::CreateNativeZano, donor_where, password);
  const auto donor = read(donor_where.wallet);
  for (int variant = 0; variant != 3; ++variant)
  {
    SCOPED_TRACE(variant);
    location where;
    provider_state state;
    const fs::path wallet = where.wallet;
    state.after_binding = [variant, wallet, &donor]
    {
      if (variant == 0) require(fs::remove(wallet));
      if (variant == 1)
      {
        const auto bytes = read(wallet);
        write(wallet, bytes.substr(0, bytes.size() / 2));
      }
      if (variant == 2) write(wallet, donor);
    };
    auto lease = make_lease(acquisition_kind::CreateNativeZano, state, where, password);
    std::unique_ptr<file_session> session;
    const auto result = wallet_provisioning::provision(acquisition_kind::CreateNativeZano, *lease, session);
    EXPECT_EQ(provision_status::RecoveryRequired, result.status);
    EXPECT_EQ(variant == 2 ? provision_detail::IdentityMismatch : provision_detail::ReopenFailed, result.detail);
    EXPECT_EQ((std::array<size_t, 3>{{1, 1, 0}}), state.calls);
    EXPECT_FALSE(same_keys(donor_identity, state.identities[0]));
    if (variant == 0) EXPECT_TRUE(where.entries().empty());
    else EXPECT_TRUE(where.only_wallet());
    if (variant == 2) EXPECT_EQ(donor, read(where.wallet));
    expect_retained_and_never_retried(*lease, state, where, session);
  }
  EXPECT_EQ(donor, read(donor_where.wallet));
}

TEST(native_wallet_provisioning, restore_phrase_whitespace_case_and_dictionary_follow_native_parsing)
{
  public_identity original{};
  const auto phrase = disposable_phrase(false, std::string(), original);
  const auto list = words(phrase);
  const auto joined = [&list](const std::string& separator)
  {
    std::string result;
    for (size_t i = 0; i != list.size(); ++i) result += (i ? separator : std::string()) + list[i];
    return result;
  };
  std::string upper = phrase;
  for (auto& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  auto capitalized = list;
  capitalized[0][0] = static_cast<char>(std::toupper(static_cast<unsigned char>(capitalized[0][0])));
  auto foreign = list;
  foreign[0] = "zzzznotanativeword";
  // expected: 1 accepted by native parsing, 0 rejected, -1 pinned only to
  // whatever the unchanged native parser decides (no new normalization).
  struct variant { const char* name; std::string phrase; int expected; };
  const std::vector<variant> variants{
    {"tabs", joined("\t"), 1},
    {"newlines", joined("\n"), 1},
    {"crlf", joined("\r\n"), 1},
    {"repeated-spaces", joined("   "), 1},
    {"leading-trailing", " \t" + phrase + " \n", -1},
    {"uppercase", upper, -1},
    {"capitalized-first-word", join(capitalized, capitalized.size()), -1},
    {"non-dictionary-word", join(foreign, foreign.size()), 0},
  };
  for (const auto& value : variants)
  {
    SCOPED_TRACE(value.name);
    // The same native checks provisioning delegates to, in the same order.
    bool native = false;
    public_identity native_identity{};
    {
      epee::log_space::scoped_log_silence quiet;
      currency::account_base check;
      bool protected_phrase = true;
      try
      {
        native = currency::account_base::is_seed_password_protected(value.phrase, protected_phrase) &&
          !protected_phrase && check.restore_from_seed_phrase(value.phrase, std::string());
      }
      catch (...) { native = false; }
      if (native)
      {
        const auto& address = check.get_public_address();
        native_identity = {address.spend_public_key, address.view_public_key, address.flags,
          build_profile().genesis};
      }
    }
    if (value.expected >= 0) EXPECT_EQ(value.expected == 1, native);
    location where;
    provider_state state;
    auto lease = make_lease(acquisition_kind::RestoreNativeZano, state, where, random_password(), value.phrase);
    std::unique_ptr<file_session> session;
    const auto result = wallet_provisioning::provision(acquisition_kind::RestoreNativeZano, *lease, session);
    if (native)
    {
      EXPECT_EQ(provision_status::Ready, result.status);
      ASSERT_TRUE(session != nullptr);
      const auto identity = session->get_receive().identity;
      EXPECT_TRUE(same_keys(native_identity, identity));
      if (value.expected == 1) EXPECT_TRUE(same_keys(original, identity));
      EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
      session.reset();
      EXPECT_TRUE(where.only_wallet());
    }
    else
    {
      EXPECT_EQ(provision_detail::InvalidPhrase, result.detail);
      EXPECT_FALSE(session != nullptr);
      expect_no_effect(result, *lease, state, where);
    }
  }
}

TEST(native_wallet_provisioning, duplicate_restore_identity_is_rejected_at_binding_and_retained)
{
  public_identity original{};
  const auto phrase = disposable_phrase(false, std::string(), original);
  std::vector<public_identity> registry;
  location first_where;
  provider_state first;
  first.registry = &registry;
  const auto password = random_password();
  const auto identity = provision_ready(acquisition_kind::RestoreNativeZano, first_where,
    password, phrase, std::string(), &first);
  EXPECT_TRUE(same_keys(original, identity));
  ASSERT_EQ(1u, registry.size());
  const auto first_bytes = read(first_where.wallet);

  // The same native phrase into a second attempt: the core cannot know it is
  // a duplicate; the provider's registry rejects the binding definitely.
  location second_where;
  provider_state second;
  second.registry = &registry;
  auto lease = make_lease(acquisition_kind::RestoreNativeZano, second, second_where,
    random_password(), phrase);
  std::unique_ptr<file_session> session;
  const auto result = wallet_provisioning::provision(acquisition_kind::RestoreNativeZano, *lease, session);
  EXPECT_EQ(provision_status::RecoveryRequired, result.status);
  EXPECT_EQ(provision_detail::ProviderRejected, result.detail);
  EXPECT_EQ((std::array<size_t, 3>{{1, 1, 0}}), second.calls);
  EXPECT_TRUE(same_keys(original, second.identities[1]));
  EXPECT_EQ(1u, registry.size());
  // The stranded encrypted file and material record stay for recovery.
  EXPECT_TRUE(second_where.only_wallet());
  expect_retained_and_never_retried(*lease, second, second_where, session);
  // The first wallet is untouched and still opens as the same identity.
  EXPECT_EQ(first_bytes, read(first_where.wallet));
  std::unique_ptr<file_session> again;
  EXPECT_EQ(open_result::Opened, reopen(first_where.wallet, password, identity, again));
}

// Reveal authority held by the test, bound to its own expected identity.
class reveal_admission final : public backup_admission
{
public:
  explicit reveal_admission(const public_identity& subject) : m_subject(subject) {}
  bool admission_current() const noexcept override { return true; }
  const public_identity& subject() const noexcept override { return m_subject; }
private:
  public_identity m_subject;
};

TEST(native_wallet_provisioning, reopened_backup_recovers_restored_v1_and_seed_password_identity)
{
  // Compare recovered spend/view keys and flags, not phrase text: V1 and
  // seed-password inputs are rendered as the unprotected canonical V2 text.
  struct variant { const char* name; bool v1; const char* seed_password; };
  for (const auto& value : {variant{"v2", false, ""}, variant{"v1", true, ""},
                            variant{"v2-password", false, "disposable-pass"},
                            variant{"v1-password", true, "disposable-pass"}})
  {
    SCOPED_TRACE(value.name);
    public_identity original{};
    const auto phrase = disposable_phrase(false, value.seed_password, original, value.v1);
    location where;
    const auto password = random_password();
    const auto identity = provision_ready(acquisition_kind::RestoreNativeZano, where, password,
      phrase, value.seed_password);
    ASSERT_TRUE(same_keys(original, identity));

    // Later, independent strict reopen with the separately held expectation.
    std::unique_ptr<file_session> session;
    ASSERT_EQ(open_result::Opened, reopen(where.wallet, password, identity, session));
    reveal_admission admission(identity);
    std::string rendered;
    ASSERT_EQ(backup_result::Presented, session->present_backup(admission, &copy_phrase, &rendered));
    EXPECT_EQ(static_cast<size_t>(SEED_PHRASE_V2_WORDS_COUNT), words(rendered).size());
    const bool same_text = rendered == phrase;
    EXPECT_EQ(!value.v1 && std::string(value.seed_password).empty(), same_text);
    {
      epee::log_space::scoped_log_silence quiet;
      currency::account_base recovered;
      ASSERT_TRUE(recovered.restore_from_seed_phrase(rendered, std::string()));
      const auto& address = recovered.get_public_address();
      EXPECT_TRUE(address.spend_public_key == original.spend_public_key);
      EXPECT_TRUE(address.view_public_key == original.view_public_key);
      EXPECT_EQ(original.flags, address.flags);
    }
    EXPECT_TRUE(where.only_wallet());  // No second phrase was persisted.
    EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  }
}

TEST(native_wallet_provisioning, creating_backup_lifetime_is_unchanged_by_reopened_reveal)
{
  location where;
  provider_state state;
  const auto password = random_password();
  auto lease = make_lease(acquisition_kind::CreateNativeZano, state, where, password);
  std::unique_ptr<file_session> session;
  ASSERT_EQ(provision_status::Ready,
    wallet_provisioning::provision(acquisition_kind::CreateNativeZano, *lease, session).status);
  ASSERT_TRUE(lease->backup != nullptr);
  const auto identity = session->get_receive().identity;
  std::string generated;
  ASSERT_TRUE(lease->backup->lend(*lease, &copy_phrase, &generated));
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  session.reset();
  auto backup = std::move(lease->backup);
  lease.reset();
  size_t visits = 0;
  // The creating-session backup still expires with its lease ...
  provider_state other_state;
  auto other = make_lease(acquisition_kind::CreateNativeZano, other_state, where, password);
  EXPECT_FALSE(backup->lend(*other, &count_visit, &visits));
  EXPECT_EQ(0u, visits);
  // ... and later display goes through a fresh reopen and separate admission.
  std::unique_ptr<file_session> reopened;
  ASSERT_EQ(open_result::Opened, reopen(where.wallet, password, identity, reopened));
  reveal_admission admission(identity);
  std::string rendered;
  ASSERT_EQ(backup_result::Presented, reopened->present_backup(admission, &copy_phrase, &rendered));
  EXPECT_EQ(generated, rendered);  // Create has no seed password: same V2 text.
  EXPECT_EQ(release_result::ReleasedUnchanged, reopened->release_unchanged());
}

#if defined(__linux__)
TEST(native_wallet_provisioning, persistence_order_and_eintr_progress_through_real_calls)
{
  location where;
  provider_state state;
  const auto password = random_password();
  io_faults faults;
  faults.events = &state.events;
  faults.eintr = 4;
  // Only three bytes can precede the fourth interrupt. Every valid image
  // exceeds the wallet header, so all four injections are guaranteed.
  faults.chunk = 509;  // Normal short writes resume after the interleaving.
  auto lease = make_lease(acquisition_kind::CreateNativeZano, state, where, password);
  std::unique_ptr<file_session> session;
  const auto result = provision_through(faults, acquisition_kind::CreateNativeZano, *lease, session);
  ASSERT_EQ(provision_status::Ready, result.status);
  ASSERT_TRUE(session != nullptr);
  // EINTR made no progress and was retried; short successful writes resumed
  // at the right offset until the whole image was persisted.
  EXPECT_EQ(4u, faults.eintr_returned);
  EXPECT_GT(faults.write_calls, 5u);
  EXPECT_FALSE(faults.capture_failed);
  EXPECT_EQ(faults.persisted, read(where.wallet));
  EXPECT_EQ(1u, faults.closes);
  // Core persistence, then the provider's ancestry persistence, then the
  // recorded steps, in exactly this order.
  EXPECT_EQ((std::vector<std::string>{"create", "write", "sync_file", "close_file", "sync_directory",
    "material:invoked", "ancestor:1", "ancestor:2", "material", "binding", "ready"}),
    collapsed(state.events));
  const auto identity = session->get_receive().identity;
  EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
  session.reset();
  lease.reset();
  std::unique_ptr<file_session> again;
  EXPECT_EQ(open_result::Opened, reopen(where.wallet, password, identity, again));
  EXPECT_TRUE(where.only_wallet());
  EXPECT_TRUE(where.sibling_unchanged());
}

TEST(native_wallet_provisioning, persistence_faults_are_classified_retained_and_never_retried)
{
  struct fault
  {
    const char* name;
    int create_errno, write_errno, sync_file_errno, close_errno, sync_directory_errno;
    provision_status status;
    provision_detail detail;
    bool file;
  };
  const size_t short_write = 97;
  const std::vector<fault> faults_list{
    {"create", EACCES, 0, 0, 0, 0, provision_status::RecoveryRequired, provision_detail::PersistenceFailed, false},
    {"create-exists", EEXIST, 0, 0, 0, 0, provision_status::RecoveryRequired, provision_detail::OccupiedOutput, false},
    {"short-write-enospc", 0, ENOSPC, 0, 0, 0, provision_status::RecoveryRequired, provision_detail::PersistenceFailed, true},
    {"short-write-eio", 0, EIO, 0, 0, 0, provision_status::RecoveryRequired, provision_detail::PersistenceFailed, true},
    {"file-fsync", 0, 0, EIO, 0, 0, provision_status::OutcomeUnknown, provision_detail::DurabilityUncertain, true},
    {"checked-close", 0, 0, 0, EIO, 0, provision_status::OutcomeUnknown, provision_detail::DurabilityUncertain, true},
    {"directory-fsync", 0, 0, 0, 0, EIO, provision_status::OutcomeUnknown, provision_detail::DurabilityUncertain, true},
  };
  for (const auto& value : faults_list)
  {
    SCOPED_TRACE(value.name);
    location where;
    provider_state state;
    io_faults faults;
    faults.events = &state.events;
    faults.create_errno = value.create_errno;
    faults.write_errno = value.write_errno;
    faults.short_write = short_write;
    faults.sync_file_errno = value.sync_file_errno;
    faults.close_errno = value.close_errno;
    faults.sync_directory_errno = value.sync_directory_errno;
    auto lease = make_lease(acquisition_kind::CreateNativeZano, state, where, random_password());
    std::unique_ptr<file_session> session;
    const auto result = provision_through(faults, acquisition_kind::CreateNativeZano, *lease, session);
    EXPECT_EQ(value.status, result.status);
    EXPECT_EQ(value.detail, result.detail);
    if (value.close_errno)
    {
      // Closed exactly once by the hook; the core never closes it again.
      EXPECT_EQ(1u, faults.closes);
      errno = 0;
      const int flags = ::fcntl(faults.closed_fd, F_GETFD);
      const int error = errno;
      EXPECT_EQ(-1, flags);
      EXPECT_EQ(EBADF, error);
    }
    // No provider step (not even the ancestry obligation) after a
    // persistence fault, and nothing Ready.
    EXPECT_EQ(state.events.size(), position(state.events, "material:invoked"));
    EXPECT_EQ(0u, state.ancestry_synced);
    EXPECT_EQ((std::array<size_t, 3>{{0, 0, 0}}), state.calls);
    if (!value.file)
    {
      // Recovery with an empty attempt directory.
      EXPECT_TRUE(where.entries().empty());
      EXPECT_EQ(0u, faults.write_calls);
    }
    else
    {
      EXPECT_TRUE(where.only_wallet());
      EXPECT_FALSE(faults.capture_failed);
      // Exactly the accepted bytes are retained: never truncated or removed.
      EXPECT_EQ(faults.persisted, read(where.wallet));
      if (value.write_errno) EXPECT_EQ(short_write, faults.persisted.size());
    }
    expect_retained_and_never_retried(*lease, state, where, session);
    lease.reset();
    EXPECT_TRUE(state.effects_at_release);
  }
}
#endif

secret_buffer secret(const std::string& value)
{
  secret_buffer result;
  result.assign(value.data(), value.size());
  return result;
}

// The provider's generated wallet-file credential (20 CSPRNG bytes as 40
// lowercase hex, 160 bits) against the production validator, plus the exact
// alphabet/length boundaries shared by seed passwords. No Unicode support is
// implied: non-ASCII and excluded ASCII are refused.
TEST(native_wallet_provisioning, wallet_password_policy_matches_core_validator)
{
  const auto generated = random_password();
  ASSERT_EQ(40u, generated.size());
  EXPECT_TRUE(wallet_provisioning::wallet_password_supported(secret(generated)));
  EXPECT_TRUE(currency::validate_password(generated));
  EXPECT_FALSE(wallet_provisioning::wallet_password_supported(secret(generated + "0")));  // 41
  EXPECT_FALSE(wallet_provisioning::wallet_password_supported(secret(std::string(64, 'a'))));  // Old 64-hex.
  EXPECT_FALSE(wallet_provisioning::wallet_password_supported(secret_buffer()));
  const std::string punctuation = "~!?@#$%^&*_+|{}[]()<>:;\"'-=/.,";
  EXPECT_TRUE(wallet_provisioning::wallet_password_supported(secret(punctuation)));
  EXPECT_TRUE(wallet_provisioning::wallet_password_supported(secret(std::string(40, 'Z'))));
  for (const std::string bad : {std::string("with space"), std::string("back\\slash"), std::string("back`tick"),
                                std::string("tab\there"), std::string("caf\xc3\xa9"), std::string("nul\0x", 5)})
  {
    SCOPED_TRACE(bad);
    EXPECT_FALSE(wallet_provisioning::wallet_password_supported(secret(bad)));
    EXPECT_FALSE(currency::validate_password(bad));
  }
  // The generated credential provisions and strictly reopens.
  location where;
  const auto identity = provision_ready(acquisition_kind::CreateNativeZano, where, generated,
    std::string(), std::string());
  std::unique_ptr<file_session> again;
  EXPECT_EQ(open_result::Opened, reopen(where.wallet, generated, identity, again));
}

// The preflight reports exactly what provision() would report with NotStarted,
// creates nothing, and accepts the core's own whitespace separators.
TEST(native_wallet_provisioning, restore_preflight_matches_provision_without_effects)
{
  public_identity plain_identity{}, protected_identity{};
  const auto plain = disposable_phrase(false, std::string(), plain_identity);
  const auto protected_phrase = disposable_phrase(false, "disposable-pass", protected_identity);
  const auto list = words(plain);
  std::string lf, crlf, tabs;
  for (size_t i = 0; i != list.size(); ++i)
  {
    lf += (i ? "\n" : "") + list[i];
    crlf += (i ? "\r\n" : "") + list[i];
    tabs += (i ? "\t \t" : "") + list[i];
  }
  for (const auto& phrase : {plain, lf, crlf, tabs, "  \n" + plain + " \r\n\t"})
  {
    EXPECT_EQ(provision_detail::None, wallet_provisioning::preflight_restore(secret(phrase), secret_buffer()));
    location where;
    EXPECT_TRUE(same_keys(plain_identity, provision_ready(acquisition_kind::RestoreNativeZano, where,
      random_password(), phrase, std::string())));
  }
  EXPECT_EQ(provision_detail::None,
    wallet_provisioning::preflight_restore(secret(protected_phrase), secret("disposable-pass")));

  struct refused { const char* name; std::string phrase, seed; provision_detail detail; };
  const std::vector<refused> cases{
    {"empty", "", "", provision_detail::InvalidPhrase},
    {"23-words", join(list, 23), "", provision_detail::InvalidPhrase},
    {"bip39-24", join(list, 24), "", provision_detail::UnsupportedInput},
    {"unknown-word", join(list, 25) + " notaword", "", provision_detail::InvalidPhrase},
    {"missing-seed-password", protected_phrase, "", provision_detail::SeedPasswordMismatch},
    {"unexpected-seed-password", plain, "disposable-pass", provision_detail::SeedPasswordMismatch},
    {"seed-password-space", protected_phrase, "disposable pass", provision_detail::InvalidMaterial},
    {"seed-password-41", protected_phrase, std::string(41, 'a'), provision_detail::InvalidMaterial},
    {"seed-password-non-ascii", protected_phrase, "caf\xc3\xa9", provision_detail::InvalidMaterial},
  };
  for (const auto& value : cases)
  {
    SCOPED_TRACE(value.name);
    EXPECT_EQ(value.detail, wallet_provisioning::preflight_restore(secret(value.phrase), secret(value.seed)));
    location where;
    provider_state state;
    auto lease = make_lease(acquisition_kind::RestoreNativeZano, state, where, random_password(),
      value.phrase, value.seed);
    std::unique_ptr<file_session> session;
    const auto result = wallet_provisioning::provision(acquisition_kind::RestoreNativeZano, *lease, session);
    EXPECT_EQ(provision_status::NotStarted, result.status);
    EXPECT_EQ(value.detail, result.detail);
    EXPECT_TRUE(where.entries().empty());
  }
  public_identity auditable_identity{};
  const auto auditable = disposable_phrase(true, std::string(), auditable_identity);
  EXPECT_EQ(provision_detail::UnsupportedCapability,
    wallet_provisioning::preflight_restore(secret(auditable), secret_buffer()));
}

#if defined(__linux__)
// Forwarded checkpoint cases below need provision() Ready (Linux descriptor
// persistence only) and the Linux replace_wallet fixture for real writes.
// They are not registered elsewhere.

// The Ready session's strict-reopen material forwards the provisioning
// lease's checkpoint capability for both kinds. Saved progress is proven by a
// fresh strict reopen whose next step is anchored at the saved block.
TEST(native_wallet_provisioning, create_and_restore_forward_checkpoint_capability_through_strict_reopen)
{
  const auto genesis = genesis_entry();
  const auto first = child(genesis);
  const auto second = child(first);
  for (const auto kind : {acquisition_kind::CreateNativeZano, acquisition_kind::RestoreNativeZano})
  {
    SCOPED_TRACE(kind == acquisition_kind::CreateNativeZano ? "create" : "restore");
    public_identity original{};
    const auto phrase = kind == acquisition_kind::RestoreNativeZano
      ? disposable_phrase(false, std::string(), original) : std::string();
    location where;
    provider_state state;
    state.checkpoint_capable = true;
    const auto password = random_password();
    auto lease = make_lease(kind, state, where, password, phrase);
    std::unique_ptr<file_session> session;
    ASSERT_EQ(provision_status::Ready, wallet_provisioning::provision(kind, *lease, session).status);
    ASSERT_TRUE(session != nullptr);
    const auto identity = session->get_receive().identity;
    if (kind == acquisition_kind::RestoreNativeZano) EXPECT_TRUE(same_keys(original, identity));
    const auto created = read(where.wallet);

    const auto transport = std::make_shared<sync_transport>();
    ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, {8, 16}));
    std::atomic<bool> stop{false};
    transport->serve(0, 2, {genesis, first});
    auto result = session->step_sync(stop);
    EXPECT_EQ(sync_step_status::Stepped, result.status);
    EXPECT_EQ(tools::read_only::step_status::Progress, result.scan.status);
    EXPECT_EQ(2u, result.scan.local_blocks_after);
    EXPECT_EQ(release_result::Unavailable, session->release_unchanged());
    EXPECT_EQ(close_result::ReleasedSaved, session->close());
    EXPECT_EQ(1u, state.checkpoints);
    EXPECT_TRUE(same_keys(identity, state.checkpoint_identity));
    EXPECT_EQ(build_profile().genesis, state.checkpoint_identity.stored_genesis);
    EXPECT_EQ(3u, state.sequence);  // No provider registry step was added.
    EXPECT_TRUE(state.callbacks_silenced);
    // One atomic replacement: no temporary, sidecar or vote file remains.
    EXPECT_TRUE(where.only_wallet());
    EXPECT_TRUE(where.sibling_unchanged());
    EXPECT_TRUE(read(where.wallet) != created);
    // Session (scanner, wallet, borrowed reopen material) before the lease.
    session.reset();
    lease.reset();
    EXPECT_TRUE(state.released);

    // Fresh strict reopen against the test's own independent expectation.
    std::unique_ptr<file_session> again;
    ASSERT_EQ(open_result::Opened, reopen(where.wallet, password, identity, again, true));
    ASSERT_EQ(sync_prepare_result::Prepared, again->prepare_sync(transport, {8, 16}));
    transport->serve(1, 3, {first, second});
    result = again->step_sync(stop);
    EXPECT_EQ(tools::read_only::step_status::Progress, result.scan.status);
    EXPECT_EQ(2u, result.scan.local_blocks_before);
    EXPECT_EQ(3u, result.scan.local_blocks_after);
    EXPECT_EQ(close_result::ReleasedSaved, again->close());
    again.reset();
    // Identity/genesis checks still apply to the checkpointed file.
    auto other = identity;
    reinterpret_cast<unsigned char*>(&other.spend_public_key)[0] ^= 1;
    EXPECT_EQ(open_result::IdentityMismatch, reopen(where.wallet, password, other, again));
    other = identity;
    reinterpret_cast<unsigned char*>(&other.stored_genesis)[0] ^= 1;
    EXPECT_EQ(open_result::GenesisMismatch, reopen(where.wallet, password, other, again));
    EXPECT_EQ(2u, transport->requests);
    EXPECT_TRUE(where.only_wallet());
  }
}

// A retained forwarded session admits no new material: no further provider
// registry step or backup, and the same lease refuses a new attempt without
// any change in the attempt directory.
void expect_no_new_material_admission(test_lease& lease, provider_state& state,
  const location& where, acquisition_kind kind, size_t backups)
{
  EXPECT_EQ((std::array<size_t, 3>{{1, 1, 1}}), state.calls);
  EXPECT_EQ(3u, state.sequence);
  EXPECT_EQ(backups, state.backups);
  const auto entries = where.entries();
  std::unique_ptr<file_session> other;
  const auto repeated = wallet_provisioning::provision(kind, lease, other);
  EXPECT_EQ(provision_status::RecoveryRequired, repeated.status);
  EXPECT_EQ(provision_detail::OccupiedOutput, repeated.detail);
  EXPECT_FALSE(other != nullptr);
  EXPECT_EQ(entries, where.entries());
  EXPECT_EQ((std::array<size_t, 3>{{1, 1, 1}}), state.calls);
  EXPECT_TRUE(where.sibling_unchanged());
}

// A definite writer rejection of the forwarded checkpoint after revocation:
// close retains the dirty session and its borrowed lease and stays
// retryable; a later commit drains through the already-held writer.
TEST(native_wallet_provisioning, forwarded_rejected_close_is_retained_and_retryable_after_revocation)
{
  const auto genesis = genesis_entry();
  const auto first = child(genesis);
  const auto second = child(first);
  for (const auto kind : {acquisition_kind::CreateNativeZano, acquisition_kind::RestoreNativeZano})
  {
    SCOPED_TRACE(kind == acquisition_kind::CreateNativeZano ? "create" : "restore");
    public_identity original{};
    const auto phrase = kind == acquisition_kind::RestoreNativeZano
      ? disposable_phrase(false, std::string(), original) : std::string();
    location where;
    provider_state state;
    state.checkpoint_capable = true;
    state.checkpoint_outcome = commit_status::Rejected;
    const auto password = random_password();
    auto lease = make_lease(kind, state, where, password, phrase);
    std::unique_ptr<file_session> session;
    ASSERT_EQ(provision_status::Ready, wallet_provisioning::provision(kind, *lease, session).status);
    ASSERT_TRUE(session != nullptr);
    const auto identity = session->get_receive().identity;
    if (kind == acquisition_kind::RestoreNativeZano) EXPECT_TRUE(same_keys(original, identity));
    const auto created = read(where.wallet);
    const auto backups = state.backups;

    const auto transport = std::make_shared<sync_transport>();
    ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, {8, 16}));
    std::atomic<bool> stop{false};
    transport->serve(0, 2, {genesis, first});
    EXPECT_EQ(sync_step_status::Stepped, session->step_sync(stop).status);
    EXPECT_EQ(session_state::OpenDirty, session->state());

    // Revocation reaches the session through the borrowed lease and is sticky
    // for reads and scans even when the provider reports current again.
    state.authorized = false;
    EXPECT_EQ(receive_status::Unavailable, session->get_receive().status);
    state.authorized = true;
    EXPECT_EQ(receive_status::Unavailable, session->get_receive().status);
    EXPECT_EQ(sync_step_status::Revoked, session->step_sync(stop).status);
    EXPECT_EQ(sync_prepare_result::Revoked, session->prepare_sync(transport, {8, 16}));
    state.authorized = false;

    // Definite rejection: dirty session, scanner, reopen material and the
    // borrowed lease are all retained; the created file is intact.
    EXPECT_EQ(close_result::RetainedFailure, session->close());
    EXPECT_EQ(1u, state.checkpoints);
    EXPECT_TRUE(same_keys(identity, state.checkpoint_identity));
    EXPECT_EQ(build_profile().genesis, state.checkpoint_identity.stored_genesis);
    EXPECT_EQ(session_state::OpenDirty, session->state());
    EXPECT_EQ(release_result::Unavailable, session->release_unchanged());
    EXPECT_FALSE(state.released);
    EXPECT_TRUE(read(where.wallet) == created);
    EXPECT_TRUE(where.only_wallet());
    // An explicit retry is allowed and reaches the writer again.
    EXPECT_EQ(checkpoint_result::Rejected, session->checkpoint());
    EXPECT_EQ(2u, state.checkpoints);
    EXPECT_TRUE(read(where.wallet) == created);
    expect_no_new_material_admission(*lease, state, where, kind, backups);

    // Drain: a now-committing writer saves through the held capability while
    // authorization stays revoked. Only the borrowed reopen material is
    // released; the caller's lease lives on.
    state.checkpoint_outcome = commit_status::Committed;
    EXPECT_EQ(close_result::ReleasedSaved, session->close());
    EXPECT_EQ(3u, state.checkpoints);
    EXPECT_EQ(session_state::Released, session->state());
    EXPECT_FALSE(state.released);
    EXPECT_TRUE(read(where.wallet) != created);
    EXPECT_TRUE(where.only_wallet());
    EXPECT_EQ(3u, state.sequence);
    EXPECT_EQ(1u, transport->requests);
    EXPECT_TRUE(state.callbacks_silenced);
    session.reset();
    lease.reset();
    EXPECT_TRUE(state.released);

    // Fresh strict reopen against the test's own expectation resumes from the
    // drained block.
    std::unique_ptr<file_session> again;
    ASSERT_EQ(open_result::Opened, reopen(where.wallet, password, identity, again, true));
    EXPECT_TRUE(same_keys(identity, again->get_receive().identity));
    ASSERT_EQ(sync_prepare_result::Prepared, again->prepare_sync(transport, {8, 16}));
    transport->serve(1, 3, {first, second});
    const auto result = again->step_sync(stop);
    EXPECT_EQ(tools::read_only::step_status::Progress, result.scan.status);
    EXPECT_EQ(2u, result.scan.local_blocks_before);
    EXPECT_EQ(close_result::ReleasedSaved, again->close());
  }
}

// A throwing writer (before any byte) or an Unknown result after a real
// replacement is a sticky, retained OutcomeUnknown with no ordinary retry,
// also after revocation. Teardown acknowledges nothing; the target is then
// only checked as a recovery input by an independent strict reopen.
TEST(native_wallet_provisioning, forwarded_unknown_or_throwing_checkpoint_is_sticky_and_retained)
{
  const auto genesis = genesis_entry();
  const auto first = child(genesis);
  const auto second = child(first);
  for (const auto kind : {acquisition_kind::CreateNativeZano, acquisition_kind::RestoreNativeZano})
  {
    for (const bool throws : {true, false})
    {
      SCOPED_TRACE(kind == acquisition_kind::CreateNativeZano ? "create" : "restore");
      SCOPED_TRACE(throws ? "throw" : "unknown-after-replace");
      public_identity original{};
      const auto phrase = kind == acquisition_kind::RestoreNativeZano
        ? disposable_phrase(false, std::string(), original) : std::string();
      location where;
      provider_state state;
      state.checkpoint_capable = true;
      state.checkpoint_throws = throws;
      if (!throws) state.checkpoint_outcome = commit_status::Unknown;
      const auto password = random_password();
      auto lease = make_lease(kind, state, where, password, phrase);
      std::unique_ptr<file_session> session;
      ASSERT_EQ(provision_status::Ready, wallet_provisioning::provision(kind, *lease, session).status);
      ASSERT_TRUE(session != nullptr);
      const auto identity = session->get_receive().identity;
      if (kind == acquisition_kind::RestoreNativeZano) EXPECT_TRUE(same_keys(original, identity));
      const auto created = read(where.wallet);
      const auto backups = state.backups;

      const auto transport = std::make_shared<sync_transport>();
      ASSERT_EQ(sync_prepare_result::Prepared, session->prepare_sync(transport, {8, 16}));
      std::atomic<bool> stop{false};
      transport->serve(0, 2, {genesis, first});
      EXPECT_EQ(sync_step_status::Stepped, session->step_sync(stop).status);
      state.authorized = false;
      EXPECT_EQ(receive_status::Unavailable, session->get_receive().status);
      EXPECT_EQ(sync_step_status::Revoked, session->step_sync(stop).status);

      // Revoked drain attempt with an uncertain writer.
      EXPECT_EQ(close_result::OutcomeUnknown, session->close());
      EXPECT_EQ(session_state::OutcomeUnknown, session->state());
      EXPECT_EQ(1u, state.checkpoints);
      EXPECT_TRUE(same_keys(identity, state.checkpoint_identity));
      EXPECT_EQ(throws, read(where.wallet) == created);
      EXPECT_TRUE(where.only_wallet());

      // Sticky: a now-healthy, authorized writer is never consulted again.
      state.checkpoint_throws = false;
      state.checkpoint_outcome = commit_status::Committed;
      state.authorized = true;
      EXPECT_EQ(checkpoint_result::OutcomeUnknown, session->checkpoint());
      EXPECT_EQ(close_result::OutcomeUnknown, session->close());
      EXPECT_EQ(sync_step_status::OutcomeUnknown, session->step_sync(stop).status);
      EXPECT_EQ(sync_prepare_result::UnsupportedState, session->prepare_sync(transport, {8, 16}));
      EXPECT_EQ(receive_status::Unavailable, session->get_receive().status);
      EXPECT_EQ(release_result::Unavailable, session->release_unchanged());
      EXPECT_EQ(1u, state.checkpoints);
      EXPECT_EQ(1u, transport->requests);
      EXPECT_FALSE(state.released);
      EXPECT_TRUE(state.callbacks_silenced);
      expect_no_new_material_admission(*lease, state, where, kind, backups);

      // Explicit owner teardown: the session and its borrowed reopen material
      // first, the lease afterwards. Neither acknowledges the write.
      session.reset();
      EXPECT_FALSE(state.released);
      lease.reset();
      EXPECT_TRUE(state.released);
      EXPECT_EQ(1u, state.checkpoints);
      EXPECT_EQ(3u, state.sequence);

      // Recovery input check only: the target strictly reopens under the
      // test's own expectation, holding the created file (throw) or the
      // replaced image (unknown-after-replace).
      std::unique_ptr<file_session> again;
      ASSERT_EQ(open_result::Opened, reopen(where.wallet, password, identity, again, true));
      const auto reopened = again->get_receive();
      EXPECT_EQ(receive_status::Available, reopened.status);
      EXPECT_TRUE(same_keys(identity, reopened.identity));
      EXPECT_EQ(build_profile().genesis, reopened.identity.stored_genesis);
      ASSERT_EQ(sync_prepare_result::Prepared, again->prepare_sync(transport, {8, 16}));
      if (throws) transport->serve(0, 2, {genesis, first});
      else transport->serve(1, 3, {first, second});
      const auto result = again->step_sync(stop);
      EXPECT_EQ(tools::read_only::step_status::Progress, result.scan.status);
      EXPECT_EQ(throws ? 1u : 2u, result.scan.local_blocks_before);
      EXPECT_EQ(close_result::ReleasedSaved, again->close());
    }
  }
}
// This Ready-session fixture also requires the Linux provisioning path;
// it tests refusal of scanning without requiring a successful checkpoint write.
// Revocation of the provisioning lease reaches the session through reopen.
TEST(native_wallet_provisioning, unsupported_or_revoked_provider_ready_session_cannot_scan)
{
  const auto transport = std::make_shared<sync_transport>();
  std::atomic<bool> stop{false};
  for (bool capable : {false, true})
  {
    location where;
    provider_state state;
    state.checkpoint_capable = capable;
    auto lease = make_lease(acquisition_kind::CreateNativeZano, state, where, random_password());
    std::unique_ptr<file_session> session;
    ASSERT_EQ(provision_status::Ready,
      wallet_provisioning::provision(acquisition_kind::CreateNativeZano, *lease, session).status);
    ASSERT_TRUE(session != nullptr);
    const auto created = read(where.wallet);
    if (!capable)
    {
      EXPECT_EQ(sync_prepare_result::CheckpointUnsupported, session->prepare_sync(transport, {8, 16}));
    }
    else
    {
      state.authorized = false;
      EXPECT_EQ(sync_prepare_result::Revoked, session->prepare_sync(transport, {8, 16}));
      state.authorized = true;
      EXPECT_EQ(receive_status::Unavailable, session->get_receive().status);
    }
    EXPECT_EQ(sync_step_status::NotPrepared, session->step_sync(stop).status);
    EXPECT_EQ(checkpoint_result::Unchanged, session->checkpoint());
    EXPECT_EQ(release_result::ReleasedUnchanged, session->release_unchanged());
    EXPECT_EQ(0u, state.checkpoints);
    EXPECT_TRUE(read(where.wallet) == created);
    session.reset();
    lease.reset();
  }
  EXPECT_EQ(0u, transport->requests);
}

#endif // defined(__linux__)

#endif
