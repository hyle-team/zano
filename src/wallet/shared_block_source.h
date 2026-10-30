// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license; see COPYING.
#pragma once
#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <vector>
#include "i_core_rpc_proxy.h"

namespace tools
{
  // One source belongs to one remote-node manager. No keys or scan state.
  class shared_block_source
  {
  public:
    using request = currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request;
    using response = currency::COMMAND_RPC_GET_BLOCKS_FAST::response;
    enum class result { ready, deferred, disconnected };
    explicit shared_block_source(std::shared_ptr<i_core_proxy> transport,
      size_t byte_limit = 32 * 1024 * 1024, size_t block_limit = 4000);
    uint64_t subscribe();
    void unsubscribe(uint64_t id);
    void finish(uint64_t id, bool deferred);
    void reset(std::shared_ptr<i_core_proxy> transport);
    uint64_t generation() const { return m_generation.load(); }
    size_t retained_bytes() const;
    result get_blocks(uint64_t id, const request& req, uint64_t local_size,
      const std::atomic<bool>& stop, response& res, uint64_t& generation,
      const std::shared_ptr<proxy_diagnostic_info>& diagnostics);
  private:
    using clock = std::chrono::steady_clock;
    struct demand
    {
      request req{};
      uint64_t cursor = 0;
      bool known = false;
      bool pending = false;
      bool scanning = false;
    };
    struct batch
    {
      request req{};
      response res{};
      std::vector<crypto::hash> hashes;
      size_t bytes = 0;
      clock::time_point fetched;
    };
    static bool same_scope(const request& a, const request& b);
    static bool inspect(batch& data, size_t byte_limit, size_t block_limit);
    bool slice(const request& req, response& res) const;
    void delivered(demand& item, const response& res);
    mutable std::mutex m_mutex;
    std::shared_ptr<i_core_proxy> m_transport;
    std::map<uint64_t, demand> m_demands;
    std::unique_ptr<batch> m_batch;
    std::atomic<uint64_t> m_generation{1};
    uint64_t m_next_id = 1;
    bool m_fetching = false;
    clock::time_point m_retry_after{};
    const size_t m_byte_limit;
    const size_t m_block_limit;
  };
}
