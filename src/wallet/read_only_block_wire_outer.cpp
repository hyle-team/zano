// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#include "read_only_block_wire_outer.h"
#include "serialization/binary_archive.h"
#include <map>
#include "storages/portable_storage_base.h"
#include "currency_core/currency_config.h"
#include "common/error_codes.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <utility>

namespace tools { namespace read_only { namespace wire
{
  namespace
  {
    bool poll(binary_decode_budget& budget) noexcept
    {
      if (budget.cancelled || budget.exhausted) return false;
      if (budget.should_stop && budget.should_stop(budget.stop_context)) {
        budget.cancelled = true;
        return false;
      }
      return true;
    }
    transport_failure budget_failure(const binary_decode_budget& budget,
      transport_limit_context* context = nullptr) noexcept
    {
      if (budget.cancelled) return transport_failure::DecodeDeadline;
      if (budget.exhausted) {
        if (context && *context == transport_limit_context::Unobserved) {
          switch (budget.first_refusal) {
            case binary_decode_refusal::Arithmetic: *context = transport_limit_context::DecodedArithmetic; break;
            case binary_decode_refusal::Allocation: *context = transport_limit_context::DecodedAllocation; break;
            case binary_decode_refusal::Collection: *context = transport_limit_context::DecodedCollection; break;
            default: break;
          }
        }
        return transport_failure::LimitExceeded;
      }
      return transport_failure::MalformedResponse;
    }

    // The finite schema only admits strings, uint64 scalars, string arrays,
    // uint64 arrays and section arrays. Every other wire tag is refused.
    class cursor
    {
    public:
      cursor(bytes_view input, binary_decode_budget* budget = nullptr)
        : m_input(input), m_budget(budget) {}
      size_t offset() const noexcept { return m_offset; }
      size_t remaining() const noexcept { return m_input.size - m_offset; }
      bytes_view span(size_t first) const noexcept {
        return {m_input.data ? m_input.data + first : nullptr, m_offset - first};
      }
      bool take(size_t size, bytes_view& result) noexcept
      {
        // Poll more frequently than the strict reader's 256-step cadence.
        if (m_budget && ((++m_steps & 63) == 0) && !poll(*m_budget)) return false;
        if (size > remaining() || (!m_input.data && size)) return false;
        result = {m_input.data ? m_input.data + m_offset : nullptr, size};
        m_offset += size;
        return true;
      }
      bool number(size_t width, uint64_t& value) noexcept
      {
        bytes_view bytes;
        if (width > sizeof(value) || !take(width, bytes)) return false;
        value = 0;
        for (size_t i = 0; i != width; ++i)
          value |= uint64_t(static_cast<unsigned char>(bytes.data[i])) << (8 * i);
        return true;
      }
      bool byte(uint8_t& value) noexcept {
        uint64_t wide;
        if (!number(1, wide)) return false;
        value = static_cast<uint8_t>(wide);
        return true;
      }
      bool packed(uint64_t& value) noexcept
      {
        if (!remaining() || !m_input.data) return false;
        const unsigned mark = static_cast<unsigned char>(m_input.data[m_offset]) & PORTABLE_RAW_SIZE_MARK_MASK;
        // Intentionally accepts overwide encodings, as read_varint() does.
        if (!number(size_t(1) << mark, value)) return false;
        value >>= 2;
        return true;
      }
      bool name(bytes_view& value) noexcept {
        uint8_t size;
        return byte(size) && take(size, value);
      }
      bool string(bytes_view& value, size_t cap) noexcept
      {
        uint64_t size;
        return packed(size) && size <= std::numeric_limits<size_t>::max() &&
          size < MAX_STRING_LEN_POSSIBLE && size <= cap &&
          size <= remaining() && take(static_cast<size_t>(size), value);
      }
    private:
      bytes_view m_input;
      binary_decode_budget* m_budget;
      size_t m_offset = 0;
      size_t m_steps = 0;
    };

    bool named(bytes_view name, const char* expected) noexcept {
      const size_t size = std::strlen(expected);
      return name.size == size && (!size || std::memcmp(name.data, expected, size) == 0);
    }
    bool type(cursor& reader, uint8_t expected) noexcept {
      uint8_t actual;
      return reader.byte(actual) && actual == expected;
    }

    class outer_decoder
    {
    public:
      outer_decoder(const std::string& body, const block_transport_limits& limits,
        binary_decode_budget& budget, transport_limit_context& context)
        : m_reader({body.data(), body.size()}, &budget), m_limits(limits),
          m_budget(budget), m_context(context) {}
      transport_failure read(outer_response& out)
      {
        if (!poll(m_budget)) return refused();
        uint64_t a, b, version;
        if (!m_reader.number(4, a) || !m_reader.number(4, b) || !m_reader.number(1, version) ||
            a != PORTABLE_STORAGE_SIGNATUREA || b != PORTABLE_STORAGE_SIGNATUREB ||
            version != PORTABLE_STORAGE_FORMAT_VER) return refused();
        size_t count;
        if (!section(1, count)) return refused();
        unsigned fields = 0;
        for (size_t i = 0; i != count; ++i) {
          bytes_view name;
          if (!entry(name)) return refused();
          const unsigned field = named(name, "status") ? 1 : named(name, "start_height") ? 2 :
            named(name, "current_height") ? 4 : named(name, "current_hardfork") ? 8 :
            named(name, "blocks") ? 16 : named(name, "max_count") ? 32 : 0;
          if (!field || (fields & field)) return transport_failure::MalformedResponse;
          fields |= field;
          if (field == 1) {
            bytes_view status;
            if (!type(m_reader, SERIALIZE_TYPE_STRING) || !m_reader.string(status, m_limits.max_blob_bytes) ||
                status.size > 64) return refused();
            if (!m_budget.charge(status.size + 1, 1, 0)) return refused();
            out.status.assign(status.data, status.size);
          } else if (field == 16) {
            const transport_failure failure = blocks(out.blocks);
            if (failure != transport_failure::None) return failure;
          } else {
            uint64_t value;
            if (!type(m_reader, SERIALIZE_TYPE_UINT64) || !m_reader.number(8, value)) return refused();
            if (field == 2) out.start_height = value;
            else if (field == 4) out.current_height = value;
            else if (field == 8) out.current_hardfork = value;
            else out.max_count = value;
          }
        }
        if ((fields & 15) != 15 || m_reader.remaining()) return transport_failure::MalformedResponse;
        if (!poll(m_budget)) return refused();
        if (out.max_count && (*out.max_count < 1 || *out.max_count > COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT))
          return transport_failure::InvalidResponse;
        if (m_typed_refusal != transport_limit_context::Unobserved) return limit(m_typed_refusal);
        if (out.status != API_RETURN_CODE_OK)
          return out.blocks.empty() ? transport_failure::None : transport_failure::InvalidResponse;
        if (!out.current_height || out.start_height >= out.current_height ||
            out.blocks.size() > out.current_height - out.start_height)
          return transport_failure::InvalidResponse;
        return transport_failure::None;
      }
    private:
      transport_failure refused() noexcept { return budget_failure(m_budget, &m_context); }
      transport_failure limit(transport_limit_context context) noexcept {
        if (m_context == transport_limit_context::Unobserved) m_context = context;
        return transport_failure::LimitExceeded;
      }
      void defer_limit(transport_limit_context context) noexcept {
        if (m_typed_refusal == transport_limit_context::Unobserved) m_typed_refusal = context;
      }
      bool add(size_t& total, size_t count, size_t cap) noexcept {
        if (total > cap || count > cap - total) return false;
        total += count;
        return true;
      }
      bool section(size_t depth, size_t& count) noexcept
      {
        uint64_t wide;
        if (depth > m_limits.max_nesting_depth || !add(m_sections, 1, m_limits.max_storage_sections) ||
            !m_reader.packed(wide) || wide > m_reader.remaining() / 2) return false;
        count = static_cast<size_t>(wide);
        return true;
      }
      bool entry(bytes_view& name) noexcept {
        return m_reader.name(name) && add(m_entries, 1, m_limits.max_storage_entries);
      }
      bool array(uint8_t element_type, size_t depth, size_t& count) noexcept
      {
        uint64_t wide;
        if (depth > m_limits.max_nesting_depth || !type(m_reader, SERIALIZE_FLAG_ARRAY | element_type) ||
            !m_reader.packed(wide) || wide > m_reader.remaining() ||
            wide > std::numeric_limits<size_t>::max()) return false;
        count = static_cast<size_t>(wide);
        return add(m_elements, count, m_limits.max_storage_array_elements);
      }
      transport_failure indices(size_t depth, bytes_view& span, size_t& count)
      {
        // Save the packed-count offset, excluding the array tag.
        const size_t first = m_reader.offset() + 1;
        if (!array(SERIALIZE_TYPE_UINT64, depth, count)) return refused();
        if (!add(m_output_indices, count, m_limits.max_output_indices))
          defer_limit(transport_limit_context::OutputIndexCount);
        if (count > m_reader.remaining() / sizeof(uint64_t)) return refused();
        bytes_view values;
        if (!m_reader.take(count * sizeof(uint64_t), values)) return refused();
        span = m_reader.span(first);
        return transport_failure::None;
      }
      transport_failure tx_index_section()
      {
        size_t count;
        if (!section(5, count)) return refused();
        bool seen = false;
        for (size_t i = 0; i != count; ++i) {
          bytes_view name;
          if (!entry(name)) return refused();
          if (!named(name, "v") || seen) return transport_failure::MalformedResponse;
          seen = true;
          bytes_view values;
          size_t value_count;
          const transport_failure failure = indices(6, values, value_count);
          if (failure != transport_failure::None) return failure;
        }
        return transport_failure::None;
      }
      transport_failure block(block_view& out)
      {
        size_t count;
        if (!section(3, count)) return refused();
        unsigned fields = 0;
        for (size_t i = 0; i != count; ++i) {
          bytes_view name;
          if (!entry(name)) return refused();
          const unsigned field = named(name, "block") ? 1 : named(name, "txs") ? 2 :
            named(name, "coinbase_global_outs") ? 4 : named(name, "tx_global_outs") ? 8 : 0;
          if (!field || (fields & field)) return transport_failure::MalformedResponse;
          fields |= field;
          if (field == 1) {
            if (!type(m_reader, SERIALIZE_TYPE_STRING) || !m_reader.string(out.block, m_limits.max_blob_bytes) ||
                !out.block.size) return refused();
          } else if (field == 2) {
            const size_t first = m_reader.offset() + 1;
            if (!array(SERIALIZE_TYPE_STRING, 4, out.tx_count)) return refused();
            for (size_t tx = 0; tx != out.tx_count; ++tx) {
              bytes_view blob;
              if (!m_reader.string(blob, m_limits.max_blob_bytes) || !blob.size) return refused();
              out.max_tx_blob_bytes = std::max(out.max_tx_blob_bytes, blob.size);
            }
            out.txs = m_reader.span(first);
          } else if (field == 4) {
            const transport_failure failure = indices(4, out.coinbase_indices, out.coinbase_index_count);
            if (failure != transport_failure::None) return failure;
          } else {
            const size_t first = m_reader.offset() + 1;
            if (!array(SERIALIZE_TYPE_OBJECT, 4, out.tx_index_count)) return refused();
            for (size_t tx = 0; tx != out.tx_index_count; ++tx) {
              const transport_failure failure = tx_index_section();
              if (failure != transport_failure::None) return failure;
            }
            out.tx_indices = m_reader.span(first);
          }
        }
        if (!(fields & 1)) return transport_failure::MalformedResponse;
        if (!add(m_transactions, out.tx_count + 1, m_limits.max_transactions))
          defer_limit(transport_limit_context::TransactionCount);
        return transport_failure::None;
      }
      transport_failure blocks(std::vector<block_view>& out)
      {
        size_t count;
        if (!array(SERIALIZE_TYPE_OBJECT, 2, count)) return refused();
        const bool over_cap = count > std::min<size_t>(m_limits.max_blocks, COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT);
        if (over_cap) defer_limit(transport_limit_context::BlockCount);
        // Preserve the strict reader's outer syntax-before-typed-limit order.
        // An over-cap array is fully walked using one fixed descriptor; never
        // reserve an oversized table just to discover a malformed tail.
        if (!over_cap) {
          // One bounded table, no per-transaction or per-field heap storage.
          if (!m_budget.charge(count, sizeof(block_view), 0)) return refused();
          out.reserve(count);
        }
        for (size_t i = 0; i != count; ++i) {
          block_view value;
          const transport_failure failure = block(value);
          if (failure != transport_failure::None) return failure;
          if (!over_cap) out.push_back(value);
        }
        return transport_failure::None;
      }
      cursor m_reader;
      const block_transport_limits& m_limits;
      binary_decode_budget& m_budget;
      transport_limit_context& m_context;
      size_t m_entries = 0, m_sections = 0, m_elements = 0;
      size_t m_output_indices = 0, m_transactions = 0;
      transport_limit_context m_typed_refusal = transport_limit_context::Unobserved;
    };

    bool array_count(cursor& reader, size_t expected) noexcept {
      uint64_t count;
      return reader.packed(count) && count == expected;
    }
    bool next_tx_indices(cursor& reader, bytes_view& values, size_t& count) noexcept
    {
      uint64_t fields;
      values = {};
      count = 0;
      // The first pass admits only an absent v or one v uint64 array.
      if (!reader.packed(fields) || fields > 1) return false;
      if (!fields) return true;
      bytes_view name;
      uint64_t wide;
      if (!reader.name(name) || !named(name, "v") ||
          !type(reader, SERIALIZE_FLAG_ARRAY | SERIALIZE_TYPE_UINT64)) return false;
      const size_t first = reader.offset();
      if (!reader.packed(wide) || wide > reader.remaining() / sizeof(uint64_t)) return false;
      count = static_cast<size_t>(wide);
      bytes_view raw;
      if (!reader.take(count * sizeof(uint64_t), raw)) return false;
      values = reader.span(first);
      return true;
    }
  }

  transport_failure read_outer(const std::string& body, const block_transport_limits& limits,
    binary_decode_budget& budget, outer_response& out, transport_limit_context& context)
  {
    out = outer_response{};
    if (body.size() > limits.max_response_bytes) {
      if (context == transport_limit_context::Unobserved) context = transport_limit_context::BodyBytes;
      return transport_failure::ResponseTooLarge;
    }
    try {
      outer_response candidate;
      outer_decoder decoder(body, limits, budget, context);
      const transport_failure failure = decoder.read(candidate);
      if (failure != transport_failure::None) return failure;
      if (!poll(budget)) return budget_failure(budget, &context);
      out = std::move(candidate);
      return transport_failure::None;
    } catch (...) {
      return transport_failure::LocalFailure;
    }
  }

  transport_failure for_each_tx(const block_view& block, binary_decode_budget& budget,
    tx_visitor visit, void* context)
  {
    if (!poll(budget)) return budget_failure(budget);
    if (!visit) return transport_failure::LocalFailure;
    if (!block.txs.data) return block.txs.size || block.tx_count ?
      transport_failure::MalformedResponse : transport_failure::None;
    cursor reader(block.txs, &budget);
    if (!array_count(reader, block.tx_count)) return budget_failure(budget);
    for (size_t i = 0; i != block.tx_count; ++i) {
      bytes_view tx;
      if (!poll(budget)) return budget_failure(budget);
      if (!reader.string(tx, block.txs.size) || !tx.size) return budget_failure(budget);
      const transport_failure failure = visit(context, tx);
      if (failure != transport_failure::None) return failure;
    }
    if (reader.remaining()) return transport_failure::MalformedResponse;
    return poll(budget) ? transport_failure::None : budget_failure(budget);
  }

  transport_failure for_each_tx_indices(const block_view& block, binary_decode_budget& budget,
    tx_indices_visitor visit, void* context)
  {
    if (!poll(budget)) return budget_failure(budget);
    if (!visit) return transport_failure::LocalFailure;
    if (!block.tx_indices.data) return block.tx_indices.size || block.tx_index_count ?
      transport_failure::MalformedResponse : transport_failure::None;
    cursor reader(block.tx_indices, &budget);
    if (!array_count(reader, block.tx_index_count)) return budget_failure(budget);
    for (size_t i = 0; i != block.tx_index_count; ++i) {
      if (!poll(budget)) return budget_failure(budget);
      bytes_view values;
      size_t count;
      if (!next_tx_indices(reader, values, count)) return budget_failure(budget);
      const transport_failure failure = visit(context, values, count);
      if (failure != transport_failure::None) return failure;
    }
    if (reader.remaining()) return transport_failure::MalformedResponse;
    return poll(budget) ? transport_failure::None : budget_failure(budget);
  }

  transport_failure for_each_tx_pair(const block_view& block, binary_decode_budget& budget,
    tx_pair_visitor visit, void* context)
  {
    if (!poll(budget)) return budget_failure(budget);
    if (!visit) return transport_failure::LocalFailure;
    if (block.tx_count != block.tx_index_count) return transport_failure::InvalidResponse;
    if ((!block.txs.data && (block.txs.size || block.tx_count)) ||
        (!block.tx_indices.data && (block.tx_indices.size || block.tx_index_count)))
      return transport_failure::MalformedResponse;
    cursor transactions(block.txs, &budget);
    cursor indices(block.tx_indices, &budget);
    if ((block.txs.data && !array_count(transactions, block.tx_count)) ||
        (block.tx_indices.data && !array_count(indices, block.tx_index_count)))
      return budget_failure(budget);
    for (size_t i = 0; i != block.tx_count; ++i) {
      if (!poll(budget)) return budget_failure(budget);
      bytes_view tx, values;
      size_t count;
      if (!transactions.string(tx, block.txs.size) || !tx.size ||
          !next_tx_indices(indices, values, count)) return budget_failure(budget);
      const transport_failure failure = visit(context, tx, values, count);
      if (failure != transport_failure::None) return failure;
    }
    if (transactions.remaining() || indices.remaining()) return transport_failure::MalformedResponse;
    return poll(budget) ? transport_failure::None : budget_failure(budget);
  }

  bool read_index(bytes_view indices, size_t index, uint64_t& value) noexcept
  {
    cursor reader(indices);
    uint64_t count;
    if (!reader.packed(count) || count > reader.remaining() / sizeof(uint64_t) || index >= count)
      return false;
    // Count bounded by remaining bytes makes offset multiplication safe.
    bytes_view skip;
    return reader.take(index * sizeof(uint64_t), skip) && reader.number(sizeof(uint64_t), value);
  }
}}}
