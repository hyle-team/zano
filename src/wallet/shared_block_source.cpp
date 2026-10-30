// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license; see COPYING.
#include "shared_block_source.h"
#include "currency_core/currency_format_utils.h"
#include <algorithm>
#include <limits>

namespace tools
{
  shared_block_source::shared_block_source(std::shared_ptr<i_core_proxy> transport,
    size_t byte_limit, size_t block_limit)
    : m_transport(std::move(transport)), m_byte_limit(byte_limit), m_block_limit(block_limit)
  {}
  uint64_t shared_block_source::subscribe()
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto id = m_next_id++;
    m_demands.emplace(id, demand{});
    return id;
  }
  void shared_block_source::unsubscribe(uint64_t id)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_demands.erase(id);
    if (m_demands.empty()) m_batch.reset();
  }
  void shared_block_source::finish(uint64_t id, bool deferred)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_demands.find(id);
    if (it != m_demands.end())
    {
      it->second.pending = deferred;
      it->second.scanning = false;
    }
  }
  void shared_block_source::reset(std::shared_ptr<i_core_proxy> transport)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_transport = std::move(transport);
    ++m_generation;
    m_batch.reset();
    m_retry_after = {};
    for (auto& entry : m_demands)
    {
      entry.second.known = false;
      entry.second.pending = false;
      entry.second.scanning = false;
    }
    // Keep the single-flight guard until any old transport finishes decoding.
  }
  size_t shared_block_source::retained_bytes() const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_batch ? m_batch->bytes : 0;
  }
  bool shared_block_source::same_scope(const request& a, const request& b)
  {
    return !a.block_ids.empty() && !b.block_ids.empty()
      && a.block_ids.back() == b.block_ids.back()
      && a.m_return_compact == b.m_return_compact
      && a.m_return_legacy_full == b.m_return_legacy_full
      && a.m_client_version == b.m_client_version;
  }
  bool shared_block_source::inspect(batch& data, size_t byte_limit, size_t block_limit)
  {
    if (data.res.status != API_RETURN_CODE_OK || data.res.blocks.empty()
      || data.res.blocks.size() > block_limit) return false;
    // Charge allocated raw capacities and conservative allocation/node overhead.
    // Parsed wallet transactions are not retained by this source.
    size_t bytes = sizeof(batch) + data.res.status.capacity() + 64;
    auto charge = [&](size_t count, size_t unit = 1) {
      if (count > byte_limit / unit || bytes > byte_limit - count * unit) return false;
      bytes += count * unit;
      return true;
    };
    data.hashes.reserve(data.res.blocks.size());
    if (!charge(data.hashes.capacity(), sizeof(crypto::hash))
      || !charge(data.req.block_ids.size(), sizeof(crypto::hash) + 64)
      || !charge(data.req.m_client_version.capacity() + 64)) return false;
    uint64_t height = data.res.start_height;
    crypto::hash previous{};
    for (const auto& entry : data.res.blocks)
    {
      if (!charge(sizeof(entry) + 64) || !charge(entry.block.capacity() + 64)
        || !charge(entry.coinbase_global_outs.capacity(), sizeof(uint64_t))
        || !charge(entry.tx_global_outs.capacity(), sizeof(entry.tx_global_outs.front())))
        return false;
      for (const auto& indexes : entry.tx_global_outs)
        if (!charge(indexes.v.capacity(), sizeof(uint64_t))) return false;
      for (const auto& tx : entry.txs)
        if (!charge(sizeof(tx) + 64) || !charge(tx.capacity() + 64)) return false;
      currency::block block{};
      if (!currency::parse_and_validate_block_from_blob(entry.block, block)
        || currency::get_block_height(block) != height
        || (height != data.res.start_height && block.prev_id != previous)) return false;
      previous = currency::get_block_hash(block);
      data.hashes.push_back(previous);
      if (height == std::numeric_limits<uint64_t>::max()) return false;
      ++height;
    }
    if (height > data.res.current_height) return false;
    data.bytes = bytes;
    return true;
  }
  bool shared_block_source::slice(const request& req, response& res) const
  {
    if (!m_batch || !same_scope(req, m_batch->req)) return false;
    uint64_t start = m_batch->res.start_height;
    // Only a proven head anchor or the complete original request is reusable.
    const auto found = std::find(m_batch->hashes.begin(), m_batch->hashes.end(), req.block_ids.front());
    if (found != m_batch->hashes.end())
      start += std::distance(m_batch->hashes.begin(), found);
    else if (!(req.block_ids == m_batch->req.block_ids
      && req.minimum_height >= m_batch->req.minimum_height)) return false;
    start = std::max(start, req.minimum_height);
    if (start < m_batch->res.start_height
      || start - m_batch->res.start_height >= m_batch->res.blocks.size()) return false;
    // Do not replay a lone retained anchor when the daemon has more blocks.
    if (start + 1 == m_batch->res.start_height + m_batch->res.blocks.size()
      && (start + 1 < m_batch->res.current_height
        || clock::now() - m_batch->fetched >= std::chrono::seconds(1))) return false;
    res = {};
    res.status = m_batch->res.status;
    res.current_height = m_batch->res.current_height;
    res.current_hardfork = m_batch->res.current_hardfork;
    res.start_height = start;
    auto first = m_batch->res.blocks.begin();
    std::advance(first, start - m_batch->res.start_height);
    res.blocks.assign(first, m_batch->res.blocks.end());
    return true;
  }
  void shared_block_source::delivered(demand& item, const response& res)
  {
    item.pending = false;
    item.scanning = true;
    if (!res.blocks.empty()) item.cursor = res.start_height + res.blocks.size() - 1;
  }
  shared_block_source::result shared_block_source::get_blocks(uint64_t id,
    const request& req, uint64_t local_size, const std::atomic<bool>& stop,
    response& res, uint64_t& generation,
    const std::shared_ptr<proxy_diagnostic_info>& diagnostics)
  {
    std::shared_ptr<i_core_proxy> transport;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      const auto it = m_demands.find(id);
      generation = m_generation.load();
      if (stop || it == m_demands.end()) return result::deferred;
      auto& item = it->second;
      item.req = req;
      item.cursor = std::max(req.minimum_height, local_size ? local_size - 1 : 0);
      item.known = item.pending = true;
      item.scanning = false;
      if (slice(req, res))
      {
        delivered(item, res);
        diagnostics->last_daemon_is_disconnected = false;
        diagnostics->last_success_interract_time = time(nullptr);
        return result::ready;
      }
      if (m_fetching) return result::deferred;
      if (clock::now() < m_retry_after) return result::disconnected;
      // The lowest subscriber fetches its own continuation. Oversized uncached
      // responses therefore always reach the wallet that requested them.
      for (const auto& entry : m_demands)
      {
        const auto& other = entry.second;
        if (entry.first != id && other.known && (other.pending || other.scanning)
          && same_scope(req, other.req) && other.cursor < item.cursor)
          return result::deferred;
      }
      m_fetching = true;
      transport = m_transport;
    }
    response fetched{};
    bool ok = false;
    std::unique_ptr<batch> candidate;
    bool retain = false;
    try
    {
      currency::COMMAND_RPC_GET_BLOCKS_FAST::request wire{};
      wire.minimum_height = req.minimum_height;
      wire.block_ids = req.block_ids;
      wire.m_return_compact = req.m_return_compact;
      wire.m_return_legacy_full = req.m_return_legacy_full;
      wire.m_client_version = req.m_client_version;
      ok = transport && transport->call_COMMAND_RPC_GET_BLOCKS_FAST(wire, fetched);
      candidate.reset(new batch);
      candidate->req = req;
      candidate->res = std::move(fetched);
      candidate->fetched = clock::now();
      retain = ok && inspect(*candidate, m_byte_limit, m_block_limit);
    }
    catch (...)
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_fetching = false;
      m_retry_after = clock::now() + std::chrono::seconds(1);
      throw;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_fetching = false;
    const auto it = m_demands.find(id);
    if (generation != m_generation.load() || stop || it == m_demands.end())
      return result::deferred;
    if (transport)
    {
      const auto d = transport->get_proxy_diagnostic_info();
      diagnostics->last_daemon_is_disconnected = d->last_daemon_is_disconnected.load();
      diagnostics->last_success_interract_time = d->last_success_interract_time.load();
    }
    if (!ok)
    {
      m_retry_after = clock::now() + std::chrono::seconds(1);
      return result::disconnected;
    }
    if (candidate->res.status != API_RETURN_CODE_OK)
      m_retry_after = clock::now() + std::chrono::seconds(1);
    if (retain)
    {
      m_batch = std::move(candidate);
      res = m_batch->res;
    }
    else
    {
      m_batch.reset();
      res = std::move(candidate->res);
    }
    delivered(it->second, res);
    return result::ready;
  }
}
