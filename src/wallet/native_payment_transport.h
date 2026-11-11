// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#pragma once
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace tools {
struct i_core_proxy;
namespace native_payment {
inline constexpr std::size_t max_request_bytes = 256 * 1024;
inline constexpr std::size_t max_response_bytes = 16 * 1024 * 1024;
inline constexpr std::size_t max_batches = 256;
inline constexpr std::size_t max_requested_heights = 8192;
inline constexpr std::size_t max_returned_outputs = 32768;
inline constexpr std::chrono::milliseconds exchange_budget{20000};

// Fixed detail of this one unsigned preparation; no values, exception text or authority.
enum class construction_detail : std::uint16_t {
  None = 0,
  LocalCoverage = 1,
  StoppedBeforeConstruction = 2,
  MissingExchange = 3,
  ProxyCreation = 4,
  PreparationException = 5,
  ConstructionSetupException = 6,
  UnsignedConstructionFalse = 7,
  UnsignedConstructionException = 8,
  PreparedShape = 9,
  SelectedInput = 10,
  SelectedInputIdentity = 11,
  DuplicateSelectedInput = 12,
  PreparedRecordException = 13,
  ProxyAlreadyCalled = 14,
  ProxyStopped = 15,
  ProxyRequestRefused = 16,
  ProxyRequestEncoding = 17,
  ProxySinkRefused = 18,
  ProxyEmptyResponse = 19,
  ProxyResponseDecode = 20,
  ProxyResponseStatus = 21,
  ProxyResponseShape = 22,
  ProxyResponseBounds = 23,
  ProxyException = 24,
  ExchangeInvalidInput = 25,
  ExchangeStopped = 26,
  ExchangeExpired = 27,
  ExchangeNoEnvironment = 28,
  ExchangeJniFailure = 29,
  ExchangeStartRefused = 30,
  ExchangePollFailure = 31,
  ExchangeInvalidChunk = 32,
  ExchangeTooLarge = 33,
  ExchangeAllocationFailure = 34,
  ExchangeAppendRefused = 35,
  ExchangePollLimit = 36,
  ExchangeUnclassified = 37,
  ExchangePollCancelled = 38,
  ExchangePollDeadlineExpired = 39,
  ExchangePollHttpStatusRefused = 40,
  ExchangePollEncodingRefused = 41,
  ExchangePollResponseTooLarge = 42,
  ExchangePollExchangeFailed = 43,
  ExchangePollLocalFailure = 44,
  ExchangePollInvalidArgument = 45,
  ExchangePollThrew = 46,
  ExchangePollJniException = 47,
};
enum class exchange_result {
  Complete, InvalidInput, Stopped, Expired, NoEnvironment, JniFailure, StartRefused,
  PollFailure, InvalidChunk, TooLarge, AllocationFailure, AppendRefused, PollLimit, Unclassified,
  PollCancelled, PollDeadlineExpired, PollHttpStatusRefused, PollEncodingRefused, PollResponseTooLarge, PollExchangeFailed, PollLocalFailure, PollInvalidArgument, PollThrew, PollJniException
};

class payment_transport;
class exchange_context final {
 public:
  bool stopped() const noexcept;
  std::chrono::milliseconds remaining() const noexcept;
 private:
  friend class payment_transport;
  exchange_context(const std::atomic<bool>&, std::chrono::steady_clock::time_point);
  const std::atomic<bool>& stop_;
  const std::chrono::steady_clock::time_point deadline_;
};
class response_sink final {
 public:
  bool append(const unsigned char*, std::size_t) noexcept;
 private:
  friend class payment_transport;
  explicit response_sink(const exchange_context&);
  const exchange_context& context_;
  std::string bytes_;
  bool failed_{false};
};
// Trusted native HTTP implementation only. Fixed TESTNET endpoint and
// POST /json_rpc method getrandom_outs4; no URI/address/profile/relay supplied by the caller.
// The legacy virtual method name is retained; its owned bytes are now JSON-RPC.
// It must honour context, bound reads before allocation and return once.
class decoy_exchange {
 public:
  virtual ~decoy_exchange() = default;
  virtual exchange_result exchange_getrandomouts4_bin(const unsigned char*, std::size_t,
      const exchange_context&, response_sink&) noexcept = 0;
};
// One preparation owns this proxy/stop for the entire call. Requests and
// responses are pinned below the verified session's local block count.
// Every other i_core_proxy entry retains its deny-by-default implementation.
std::shared_ptr<i_core_proxy> make_payment_transport(std::shared_ptr<decoy_exchange>,
    std::uint64_t local_blocks, const std::atomic<bool>& stop,
    construction_detail* detail = nullptr) noexcept;
} }
