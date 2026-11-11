// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace tools
{
  struct i_core_proxy;
  namespace read_only
  {
    class block_transport;

    // Every default below is also the hard policy maximum: callers may tighten
    // a value (fixtures, deployment) but never exceed it or set it to zero.
    // Count/byte limits are INCLUSIVE: a value equal to the limit is accepted,
    // one more is refused. Deadlines are EXCLUSIVE: now >= deadline is expired.
    // A limit can refuse a legal daemon batch; the daemon has no caller batch
    // size field, so such a response fails whole. There is no widening,
    // truncation, retry or partial publication through the legacy DIRECT API.
    // The scanner-native validated-prefix side API retains only its requested
    // whole-block prefix, with full original count/truncation metadata, after
    // complete reply validation. Refused tails never publish a prefix.
    struct block_transport_limits
    {
      size_t max_request_bytes = 65536;
      size_t max_response_bytes = 16777216;
      size_t max_blob_bytes = 1048576;          // one block or transaction blob
      size_t max_blocks = 4000;                 // also capped by the core protocol constant
      size_t max_transactions = 16000;          // includes one coinbase per block
      size_t max_output_indices = 262144;
      size_t max_storage_entries = 65536;       // outer portable storage
      size_t max_storage_sections = 65536;
      size_t max_storage_array_elements = 262144;
      size_t max_nesting_depth = 16;            // outer sections/arrays, root = 1
      // Legacy/no-work codec: cumulative complete-reply inner work.
      // Native prefix: per collection AND cumulative selected source elements;
      // full-reply traversal additionally has ONE body.size() work bound.
      size_t max_collection_elements = 262144;
      uint64_t max_decode_allocation_bytes = 67108864; // inner requested storage, cumulative
      std::chrono::milliseconds exchange_timeout{15000}; // from call entry
      std::chrono::milliseconds decode_timeout{2000};    // after a successful exchange
    };

    // True when every value is nonzero, within its maximum and consistent.
    bool block_transport_limits_valid(const block_transport_limits& limits) noexcept;

    // Monotonic time source. Tests may inject logical time; production uses
    // std::chrono::steady_clock. now() must not throw or block.
    class monotonic_clock
    {
    public:
      virtual ~monotonic_clock() = default;
      virtual std::chrono::steady_clock::time_point now() const noexcept = 0;
    };

    // Immutable, borrowed view valid only during one synchronous exchange.
    // Do not retain it. Core checks stop/deadline before the call and after it
    // returns and discards late data, but it cannot interrupt a callback that
    // never returns: a hard network wait is the platform adapter's obligation.
    class exchange_context
    {
    public:
      exchange_context(const exchange_context&) = delete;
      exchange_context& operator=(const exchange_context&) = delete;
      std::chrono::steady_clock::time_point deadline() const noexcept { return m_deadline; }
      bool stop_requested() const noexcept { return m_stop.load(std::memory_order_relaxed); }
      bool expired() const noexcept { return m_clock.now() >= m_deadline; }
      bool should_abort() const noexcept { return stop_requested() || expired(); }
    private:
      friend class block_transport;
      exchange_context(const std::atomic<bool>& stop, const monotonic_clock& clock,
        std::chrono::steady_clock::time_point deadline) noexcept
        : m_stop(stop), m_clock(clock), m_deadline(deadline) {}
      const std::atomic<bool>& m_stop;
      const monotonic_clock& m_clock;
      const std::chrono::steady_clock::time_point m_deadline;
    };

    // Capped body sink. append() refuses (and latches failure) instead of
    // growing past the cap or trimming; it never throws.
    class response_sink
    {
    public:
      response_sink(const response_sink&) = delete;
      response_sink& operator=(const response_sink&) = delete;
      bool append(const void* data, size_t size) noexcept;
      size_t size() const noexcept { return m_body.size(); }
      size_t limit() const noexcept { return m_limit; }
      bool failed() const noexcept { return m_overflow || m_failed; }
    private:
      friend class block_transport;
      response_sink(std::string& body, size_t limit) noexcept : m_body(body), m_limit(limit) {}
      std::string& m_body;
      const size_t m_limit;
      bool m_overflow = false;
      bool m_failed = false;
    };

    enum class exchange_status { Ok, Failed };

    // One fixed-purpose native binary exchange: POST the given bytes to the
    // configured daemon's /getblocks.bin and stream the HTTP body into `body`.
    // The request bytes and context are borrowed for this call only. The
    // implementation must not retain them, re-enter wallet code, retry, change
    // endpoint authority, raise limits, trim a body, or log bodies/raw errors.
    // Exceptions are caught by core and treated as Failed.
    class block_exchange
    {
    public:
      virtual ~block_exchange() = default;
      // Same configured origin, fixed /json_rpc get_est_height_from_date only.
      // Separate owner-tagged step, same cancellation/physical-slot contract.
      virtual exchange_status exchange_birthday_json(const unsigned char*, size_t,
        const exchange_context&, response_sink&) { return exchange_status::Failed; }
      // Fixed /json_rpc get_native_birthday_prefix_v1 only. Same configured
      // origin, owner token, physical slot and complete-response staging.
      virtual exchange_status exchange_birthday_prefix_json(const unsigned char*, size_t,
        const exchange_context&, response_sink&) { return exchange_status::Failed; }
      virtual exchange_status exchange_getblocks_bin(const unsigned char* request, size_t request_size,
        const exchange_context& context, response_sink& body) = 0;
    };

    // Fixed, content-free failure classes for diagnostics and tests.
    enum class transport_failure
    {
      None, Busy, Stopped, InvalidRequest, RequestTooLarge, ExchangeFailed, ExchangeDeadline,
      ResponseTooLarge, DecodeDeadline, MalformedResponse, LimitExceeded, InvalidResponse,
      LocalFailure, NotBlockTransport
    };

    // Finite source-fixed first refusing predicate, no counts or response content.
    enum class transport_limit_context {
      Unobserved, BodyBytes, BlockCount, OutputIndexCount, TransactionCount,
      DecodedArithmetic, DecodedAllocation, DecodedCollection
    };
    struct transport_observation {
      transport_failure failure = transport_failure::None;
      transport_limit_context limit = transport_limit_context::Unobserved;
    };

    // Returns a private i_core_proxy that answers only the DIRECT block fetch
    // (both overloads, each bounded identically) and refuses everything else.
    // Also implements the private stop-aware birthday_height_source capability.
    // Returns nullptr for a null exchange or invalid limits.
    std::shared_ptr<i_core_proxy> make_block_transport(std::shared_ptr<block_exchange> exchange,
      const block_transport_limits& limits = block_transport_limits{},
      std::shared_ptr<const monotonic_clock> clock = nullptr) noexcept;

    // Last failure class of a transport created above; NotBlockTransport otherwise.
    transport_failure block_transport_last_failure(const i_core_proxy& transport) noexcept;
    // One atomically paired last-call observation. Use only after this exact
    // owned call returns; this getter creates no new call or authority.
    // Unknown/inconsistent metadata is unobserved; non-transport is unobserved.
    transport_observation block_transport_last_observation(const i_core_proxy& transport) noexcept;
  }
}
