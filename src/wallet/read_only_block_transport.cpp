// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#include "read_only_block_transport.h"
#include "i_core_rpc_proxy.h"
#include "currency_core/currency_format_utils.h"
#include "currency_core/currency_format_utils_abstract.h"
#include "currency_core/currency_format_utils_blocks.h"
#include "serialization/binary_archive.h"
#include "storages/portable_storage_template_helper.h"
#include "read_only_validated_prefix.h"
#include "read_only_block_wire.h"
#include "read_only_wallet_birthday.h"
#include "read_only_birthday_prefix.h"
#include <algorithm>
#include <limits>
#include <list>
#include <sstream>
#include <typeinfo>
#include <utility>

namespace tools { namespace read_only
{
  namespace
  {
    namespace ps = epee::serialization;
    using time_point = std::chrono::steady_clock::time_point;
    using fast_request = currency::COMMAND_RPC_GET_BLOCKS_FAST::request;
    using fast_response = currency::COMMAND_RPC_GET_BLOCKS_FAST::response;
    using direct_request = currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request;
    using direct_response = currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response;

    const block_transport_limits policy_maxima{};
    // The fixed normal native scan batch: inclusive anchor counts as a block.
    constexpr size_t native_request_block_count = 128;
    // A request buffer below this cannot carry even a genesis-only history.
    constexpr size_t min_request_bytes = 128;
    // The response schema is six levels deep: root, blocks[], block,
    // tx_global_outs[], per-transaction section, v[].
    constexpr size_t min_nesting_depth = 6;
    constexpr size_t max_status_bytes = 64;
    // Approximate bookkeeping charged for intermediate FAST/DIRECT objects.
    constexpr uint64_t list_node_bytes = 2 * sizeof(void*);
    constexpr uint64_t shared_control_bytes = 4 * sizeof(void*);

    // Private packing only, never a wire/native/JS code table. Preserve all
    // original failure bits; app adapters map the typed names explicitly.
    uint64_t pack_observation(transport_failure failure, transport_limit_context limit) noexcept {
      return static_cast<uint32_t>(failure) |
        (static_cast<uint64_t>(static_cast<uint32_t>(limit)) << 32);
    }
    transport_failure unpack_failure(uint64_t value) noexcept {
      return static_cast<transport_failure>(static_cast<uint32_t>(value));
    }
    transport_observation unpack_observation(uint64_t value) noexcept {
      transport_observation result{unpack_failure(value),
        static_cast<transport_limit_context>(static_cast<uint32_t>(value >> 32))};
      switch (result.failure) {
        case transport_failure::None: case transport_failure::Busy: case transport_failure::Stopped:
        case transport_failure::InvalidRequest: case transport_failure::RequestTooLarge:
        case transport_failure::ExchangeFailed: case transport_failure::ExchangeDeadline:
        case transport_failure::ResponseTooLarge: case transport_failure::DecodeDeadline:
        case transport_failure::MalformedResponse: case transport_failure::LimitExceeded:
        case transport_failure::InvalidResponse: case transport_failure::LocalFailure:
        case transport_failure::NotBlockTransport: break;
        default: return {};
      }
      switch (result.limit) {
        case transport_limit_context::BodyBytes:
          if (result.failure == transport_failure::ResponseTooLarge) return result;
          break;
        case transport_limit_context::BlockCount: case transport_limit_context::OutputIndexCount:
        case transport_limit_context::TransactionCount: case transport_limit_context::DecodedArithmetic:
        case transport_limit_context::DecodedAllocation: case transport_limit_context::DecodedCollection:
          if (result.failure == transport_failure::LimitExceeded) return result;
          break;
        case transport_limit_context::Unobserved: default: break;
      }
      result.limit = transport_limit_context::Unobserved;
      return result;
    }

    class steady_clock_source final : public monotonic_clock
    {
    public:
      time_point now() const noexcept override { return std::chrono::steady_clock::now(); }
    };

    bool deadline_after(time_point start, std::chrono::milliseconds timeout, time_point& deadline) noexcept
    {
      const auto step = std::chrono::duration_cast<std::chrono::steady_clock::duration>(timeout);
      if (step.count() <= 0 || start > time_point::max() - step)
        return false;
      deadline = start + step;
      return true;
    }

    // Cooperative decode cancellation: the owner's stop or the decode deadline.
    struct decode_watch
    {
      const std::atomic<bool>& stop;
      const monotonic_clock& clock;
      const time_point deadline;
      bool fired = false;

      static bool poll(void* self) noexcept
      {
        decode_watch& watch = *static_cast<decode_watch*>(self);
        if (watch.stop.load(std::memory_order_relaxed) || watch.clock.now() >= watch.deadline)
          watch.fired = true;
        return watch.fired;
      }
      transport_failure reason() const noexcept
      {
        return stop.load(std::memory_order_relaxed) ? transport_failure::Stopped : transport_failure::DecodeDeadline;
      }
    };

    template<class T>
    std::list<T>* array_of(ps::storage_entry& entry)
    {
      ps::array_entry* array = boost::get<ps::array_entry>(&entry);
      if (!array)
        return nullptr;
      ps::array_entry_t<T>* typed = boost::get<ps::array_entry_t<T>>(array);
      return typed ? &typed->m_array : nullptr;
    }

    // Strict schema mapping for COMMAND_RPC_GET_BLOCKS_FAST::response as the
    // existing epee serializer writes it, then conversion to DIRECT. Blob
    // strings are moved/spliced out of the storage tree, never copied.
    class response_decoder
    {
    public:
      response_decoder(const block_transport_limits& limits, decode_watch& watch, uint64_t requested_count)
        : m_limits(limits), m_watch(watch), m_requested_count(requested_count)
      {
        m_budget.max_bytes = limits.max_decode_allocation_bytes;
        m_budget.max_elements = limits.max_collection_elements;
        m_budget.should_stop = &decode_watch::poll;
        m_budget.stop_context = &watch;
      }

      transport_failure decode(std::string& body, direct_response& out)
      {
        ps::portable_storage_limits outer;
        outer.max_storage_entries_count = m_limits.max_storage_entries;
        outer.max_sections_entries_count = m_limits.max_storage_sections;
        outer.max_array_elements_count = m_limits.max_storage_array_elements;
        outer.strict = true;
        outer.max_nesting_depth = m_limits.max_nesting_depth;
        outer.max_string_length = m_limits.max_blob_bytes;
        outer.should_stop = &decode_watch::poll;
        outer.stop_context = &m_watch;
        ps::section root;
        {
          ps::portable_storage storage;
          if (!storage.load_from_binary(body, outer))
            return m_watch.fired ? m_watch.reason() : transport_failure::MalformedResponse;
          storage.take_root(root);
        }
        std::string().swap(body); // The tree owns the only remaining copy.

        fast_response fast{};
        transport_failure failure = map_root(root, fast);
        if (failure == transport_failure::None)
          failure = check_envelope(fast);
        direct_response direct{};
        if (failure == transport_failure::None)
          failure = convert(fast, direct);
        if (failure == transport_failure::None && decode_watch::poll(&m_watch))
          failure = m_watch.reason();
        if (failure != transport_failure::None)
          return failure;
        out = std::move(direct);
        return transport_failure::None;
      }

      transport_limit_context limit_context() const noexcept { return m_limit_context; }

    private:
      void record_limit(transport_limit_context context) const noexcept {
        if (m_limit_context == transport_limit_context::Unobserved) m_limit_context = context;
      }
      transport_failure refused() const noexcept
      {
        if (m_watch.fired || m_budget.cancelled)
          return m_watch.reason();
        if (m_budget.exhausted) {
          switch (m_budget.first_refusal) {
            case binary_decode_refusal::Arithmetic: record_limit(transport_limit_context::DecodedArithmetic); break;
            case binary_decode_refusal::Allocation: record_limit(transport_limit_context::DecodedAllocation); break;
            case binary_decode_refusal::Collection: record_limit(transport_limit_context::DecodedCollection); break;
            case binary_decode_refusal::Unobserved: default: break;
          }
          return transport_failure::LimitExceeded;
        }
        return transport_failure::MalformedResponse;
      }
      bool charge(uint64_t count, uint64_t unit) noexcept { return m_budget.charge(count, unit, 0); }
      bool add_output_indices(size_t count) noexcept
      {
        if (m_output_indices > m_limits.max_output_indices || count > m_limits.max_output_indices - m_output_indices) {
          record_limit(transport_limit_context::OutputIndexCount);
          return false;
        }
        m_output_indices += count;
        return true;
      }

      transport_failure map_root(ps::section& root, fast_response& fast)
      {
        bool status = false, start = false, current = false, hardfork = false, acknowledged = false;
        for (auto& item : root.m_entries)
        {
          ps::storage_entry& value = item.second;
          if (item.first == "status")
          {
            std::string* text = boost::get<std::string>(&value);
            if (!text || text->size() > max_status_bytes)
              return transport_failure::MalformedResponse;
            fast.status = std::move(*text);
            status = true;
          }
          else if (item.first == "start_height" || item.first == "current_height" || item.first == "current_hardfork")
          {
            const uint64_t* number = boost::get<uint64_t>(&value);
            if (!number)
              return transport_failure::MalformedResponse;
            if (item.first == "start_height") { fast.start_height = *number; start = true; }
            else if (item.first == "current_height") { fast.current_height = *number; current = true; }
            else { fast.current_hardfork = *number; hardfork = true; }
          }
          else if (item.first == "max_count")
          {
            const uint64_t* count = boost::get<uint64_t>(&value);
            if (!count) return transport_failure::MalformedResponse;
            if (*count < 1 || *count > COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT || *count != m_requested_count)
              return transport_failure::InvalidResponse;
            fast.max_count = *count;
            acknowledged = true;
          }
          else if (item.first == "blocks")
          {
            std::list<ps::section>* blocks = array_of<ps::section>(value);
            if (!blocks)
              return transport_failure::MalformedResponse;
            const size_t block_cap = std::min<size_t>(m_limits.max_blocks,
              static_cast<size_t>(COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT));
            if (blocks->size() > block_cap) {
              record_limit(transport_limit_context::BlockCount);
              return transport_failure::LimitExceeded;
            }
            for (ps::section& block : *blocks)
            {
              if (!charge(1, sizeof(currency::block_complete_entry) + list_node_bytes))
                return refused();
              fast.blocks.emplace_back();
              const transport_failure failure = map_block(block, fast.blocks.back());
              if (failure != transport_failure::None)
                return failure;
            }
          }
          else
            return transport_failure::MalformedResponse; // Unknown field.
        }
        // current_hardfork is required and retained; the legacy adapter drops it.
        if (!status || !start || !current || !hardfork)
          return transport_failure::MalformedResponse;
        if (!acknowledged) return transport_failure::InvalidResponse;
        return transport_failure::None;
      }

      transport_failure map_block(ps::section& section, currency::block_complete_entry& entry)
      {
        bool has_block = false;
        for (auto& item : section.m_entries)
        {
          ps::storage_entry& value = item.second;
          if (item.first == "block")
          {
            std::string* blob = boost::get<std::string>(&value);
            if (!blob || blob->empty())
              return transport_failure::MalformedResponse;
            entry.block = std::move(*blob);
            has_block = true;
          }
          else if (item.first == "txs")
          {
            std::list<std::string>* txs = array_of<std::string>(value);
            if (!txs)
              return transport_failure::MalformedResponse;
            for (const std::string& tx : *txs)
              if (tx.empty())
                return transport_failure::MalformedResponse;
            entry.txs.splice(entry.txs.end(), *txs);
          }
          else if (item.first == "coinbase_global_outs")
          {
            std::list<uint64_t>* outs = array_of<uint64_t>(value);
            if (!outs)
              return transport_failure::MalformedResponse;
            if (!add_output_indices(outs->size()))
              return transport_failure::LimitExceeded;
            if (!charge(outs->size(), sizeof(uint64_t)))
              return refused();
            entry.coinbase_global_outs.assign(outs->begin(), outs->end());
          }
          else if (item.first == "tx_global_outs")
          {
            std::list<ps::section>* per_tx = array_of<ps::section>(value);
            if (!per_tx)
              return transport_failure::MalformedResponse;
            using outs_entry = decltype(entry.tx_global_outs)::value_type;
            if (!charge(per_tx->size(), sizeof(outs_entry)))
              return refused();
            entry.tx_global_outs.reserve(per_tx->size());
            for (ps::section& tx_section : *per_tx)
            {
              entry.tx_global_outs.emplace_back();
              for (auto& field : tx_section.m_entries)
              {
                std::list<uint64_t>* outs = field.first == "v" ? array_of<uint64_t>(field.second) : nullptr;
                if (!outs)
                  return transport_failure::MalformedResponse;
                if (!add_output_indices(outs->size()))
                  return transport_failure::LimitExceeded;
                if (!charge(outs->size(), sizeof(uint64_t)))
                  return refused();
                entry.tx_global_outs.back().v.assign(outs->begin(), outs->end());
              }
            }
          }
          else
            return transport_failure::MalformedResponse;
        }
        if (!has_block)
          return transport_failure::MalformedResponse;
        // Count transaction objects (with the coinbase) before any blob parse.
        const size_t objects = entry.txs.size() + 1;
        if (m_transactions > m_limits.max_transactions || objects > m_limits.max_transactions - m_transactions) {
          record_limit(transport_limit_context::TransactionCount);
          return transport_failure::LimitExceeded;
        }
        m_transactions += objects;
        return transport_failure::None;
      }

      transport_failure check_envelope(const fast_response& fast) const
      {
        // A non-OK status carries no blocks; the scanner maps the status.
        if (fast.status != API_RETURN_CODE_OK)
          return fast.blocks.empty() ? transport_failure::None : transport_failure::InvalidResponse;
        if (!fast.current_height || fast.start_height >= fast.current_height ||
            fast.blocks.size() > fast.current_height - fast.start_height)
          return transport_failure::InvalidResponse;
        return transport_failure::None;
      }

      // decoded_bytes receives everything charged while decoding this blob
      // (stream copy, containers, strings, conversions).
      template<class T>
      bool parse_blob(const std::string& blob, T& value, uint64_t& decoded_bytes)
      {
        const uint64_t before = m_budget.bytes;
        // The input stream holds one copy of the blob; charge it first. The
        // archive then charges every container, string and conversion.
        if (blob.empty() || blob.size() > m_limits.max_blob_bytes || !charge(blob.size(), 1))
          return false;
        std::istringstream stream(blob);
        binary_archive<false> archive(stream);
        archive.set_decode_budget(&m_budget);
        // serialize() also requires the blob to be consumed completely.
        const bool parsed = ::serialization::serialize(archive, value);
        decoded_bytes = m_budget.bytes - before;
        return parsed;
      }

      // Precharge for hashing one decoded transaction prefix through
      // get_object_hash_in_buffer(). With the fixed scratch there is no stream
      // growth and no str() copy; the remaining heap requests are the saving
      // transitions' temporaries (PRE_HF4 transaction_prefix_v1, or
      // std::vector<tx_out_v_v1> before HF6), deep copies of storage whose
      // decode was already charged in decoded_bytes under the same accounting,
      // plus the exactly reserved converted output vector.
      bool charge_prefix_hash(uint64_t decoded_bytes, size_t outputs) noexcept
      {
        return charge(decoded_bytes, 1) &&
          charge(outputs, std::max(sizeof(currency::tx_out_bare), sizeof(currency::tx_out_v_v1)));
      }

      // Precharge for get_block_hash_in_buffer(): the exactly reserved id
      // vector, the same again as a bound on the caller-owned checked tree
      // hash scratch (crypto::tree_hash_scratch_count(ids) is 0 for ids <= 2,
      // otherwise the greatest power of two <= ids - 1, so always < ids), and
      // the reserved hashing blob string.
      bool charge_block_hash(size_t tx_hashes) noexcept
      {
        const uint64_t ids = static_cast<uint64_t>(tx_hashes) + 1;
        return charge(ids, 2 * sizeof(crypto::hash)) && charge(currency::BLOCK_HASHING_BLOB_MAX_BYTES + 1, 1);
      }

      // Validates the WHOLE response, including any tail the scanner will not
      // select: every blob parses completely, heights are contiguous from
      // start_height, blocks link by prev_id, transaction hashes match the
      // block's list in order, and output-index dimensions are exact. This is
      // not consensus validation; chain/genesis/hardfork authority stays with
      // the scanner.
      transport_failure convert(fast_response& fast, direct_response& direct)
      {
        direct.status = std::move(fast.status);
        direct.start_height = fast.start_height;
        direct.current_height = fast.current_height;
        direct.current_hardfork = fast.current_hardfork;
        direct.max_count = fast.max_count;
        // One fixed hash scratch for the whole response, charged once: every
        // prefix re-encoding fits in its source blob (strict decoding accepts
        // only canonical, fully consumed encodings), the block hashing blob in
        // BLOCK_HASHING_BLOB_MAX_BYTES plus its length varint.
        size_t scratch_size = currency::BLOCK_HASHING_BLOB_MAX_BYTES + 2;
        for (const currency::block_complete_entry& source : fast.blocks)
        {
          scratch_size = std::max(scratch_size, source.block.size());
          for (const std::string& tx : source.txs)
            scratch_size = std::max(scratch_size, tx.size());
        }
        if (!charge(scratch_size, 1))
          return refused();
        std::vector<char> scratch(scratch_size);

        uint64_t height = fast.start_height; // Bounded by current_height above.
        crypto::hash previous = currency::null_hash;
        bool first = true;
        for (currency::block_complete_entry& source : fast.blocks)
        {
          if (!charge(1, sizeof(currency::block_direct_data_entry) + list_node_bytes +
                sizeof(currency::block_extended_info) + sizeof(currency::transaction_chain_entry) + 2 * shared_control_bytes))
            return refused();
          auto block = std::make_shared<currency::block_extended_info>();
          uint64_t decoded = 0;
          if (!parse_blob(source.block, block->bl, decoded))
            return refused();
          std::string().swap(source.block);
          const currency::block& bl = block->bl;
          if (bl.miner_tx.vin.empty() || bl.miner_tx.vin.front().type() != typeid(currency::txin_gen) ||
              currency::get_block_height(bl) != height || (!first && bl.prev_id != previous) ||
              source.coinbase_global_outs.size() != bl.miner_tx.vout.size() ||
              source.txs.size() != bl.tx_hashes.size() || source.tx_global_outs.size() != source.txs.size())
            return transport_failure::InvalidResponse;
          // Bounded hashing; the strict helpers equal get_block_hash() and
          // get_transaction_hash() (differential fixtures), without growth.
          crypto::hash miner_hash = currency::null_hash;
          if (!charge_prefix_hash(decoded, bl.miner_tx.vout.size()) || !charge_block_hash(bl.tx_hashes.size()))
            return refused();
          // Both allocations are covered by charge_block_hash() above. They
          // run inside fetch(), so an allocation failure throws out to the
          // catch in call_COMMAND_RPC_GET_BLOCKS_DIRECT(): LocalFailure with
          // an empty DIRECT response, never a zero-root hash.
          std::vector<crypto::hash> tx_ids;
          tx_ids.reserve(bl.tx_hashes.size() + 1);
          std::vector<crypto::hash> tree_scratch(crypto::tree_hash_scratch_count(bl.tx_hashes.size() + 1));
          if (!currency::get_object_hash_in_buffer(static_cast<const currency::transaction_prefix&>(bl.miner_tx), scratch, miner_hash) ||
              !currency::get_block_hash_in_buffer(bl, miner_hash, tx_ids, tree_scratch, scratch, previous))
            return transport_failure::InvalidResponse;
          auto coinbase = std::make_shared<currency::transaction_chain_entry>();
          // Like the legacy adapter, coinbase_ptr carries indices; the coinbase
          // itself stays in block_ptr->bl.miner_tx.
          coinbase->m_global_output_indexes = std::move(source.coinbase_global_outs);
          currency::block_direct_data_entry target;
          target.block_ptr = block;
          target.coinbase_ptr = coinbase;
          auto outs = source.tx_global_outs.begin();
          size_t index = 0;
          for (std::string& blob : source.txs)
          {
            if (!charge(1, sizeof(currency::transaction_chain_entry) + shared_control_bytes + list_node_bytes))
              return refused();
            auto tx = std::make_shared<currency::transaction_chain_entry>();
            uint64_t decoded = 0;
            if (!parse_blob(blob, tx->tx, decoded))
              return refused();
            std::string().swap(blob);
            if (!charge_prefix_hash(decoded, tx->tx.vout.size()))
              return refused();
            crypto::hash tx_hash = currency::null_hash;
            if (!currency::get_object_hash_in_buffer(static_cast<const currency::transaction_prefix&>(tx->tx), scratch, tx_hash) ||
                tx_hash != bl.tx_hashes[index] || outs->v.size() != tx->tx.vout.size())
              return transport_failure::InvalidResponse;
            tx->m_global_output_indexes = std::move(outs->v);
            target.txs_ptr.push_back(std::move(tx));
            ++outs;
            ++index;
          }
          direct.blocks.push_back(std::move(target));
          ++height;
          first = false;
        }
        return transport_failure::None;
      }

      const block_transport_limits& m_limits;
      decode_watch& m_watch;
      const uint64_t m_requested_count;
      binary_decode_budget m_budget;
      mutable transport_limit_context m_limit_context = transport_limit_context::Unobserved;
      size_t m_transactions = 0;
      size_t m_output_indices = 0;
    };

    // Scanner-native view path: complete reply validation, selected graph only.
    class prefix_decoder {
    public:
      prefix_decoder(const block_transport_limits& limits,decode_watch& watch,uint64_t requested_count)
        : limits(limits),watch(watch),requested_count(requested_count) {
        budget.max_bytes=limits.max_decode_allocation_bytes;
        budget.max_elements=limits.max_collection_elements;
        budget.should_stop=&decode_watch::poll;budget.stop_context=&watch;
      }
      transport_limit_context limit_context() const noexcept {return context;}
      transport_failure decode(const std::string& body,prefix_selection_limits selection,
        validated_prefix_response& out) {
        out={};
        wire::outer_response envelope;
        transport_failure failure=wire::read_outer(body,limits,budget,envelope,context);
        if(failure!=transport_failure::None)return remap(failure);
        // Count alone cannot prove support: an old daemon may return a short tip batch.
        if(!envelope.max_count || *envelope.max_count!=requested_count)
          return transport_failure::InvalidResponse;
        // Shared across EVERY blob in this call; never reset or published.
        wire::inner_collection_work work(body.size());
        validated_prefix_response candidate;
        candidate.full_block_count=envelope.blocks.size();
        candidate.selected.status=std::move(envelope.status);
        candidate.selected.start_height=envelope.start_height;
        candidate.selected.current_height=envelope.current_height;
        candidate.selected.current_hardfork=envelope.current_hardfork;
        candidate.selected.max_count=envelope.max_count;
        size_t selected=0,remaining=selection.max_transactions,max_ids=1;
        size_t scratch_size=currency::BLOCK_HASHING_BLOB_MAX_BYTES+2;
        bool ended=false;
        for(const auto& block:envelope.blocks) {
          max_ids=std::max(max_ids,block.tx_count+1);
          scratch_size=std::max(scratch_size,std::max(block.block.size,block.max_tx_blob_bytes));
          if(!ended && selected<selection.max_blocks && remaining &&
             block.tx_count<=remaining-1) {++selected;remaining-=block.tx_count+1;}
          else ended=true;
        }
        candidate.prefix_truncated=selected!=candidate.full_block_count;
        // One cumulative request per reusable scratch; no refunds or resets.
        const size_t tree_count=crypto::tree_hash_scratch_count(max_ids);
        if(!budget.charge(scratch_size,1,0) ||
           !budget.charge(max_ids,sizeof(crypto::hash),0) ||
           !budget.charge(tree_count,sizeof(crypto::hash),0))return refused();
        std::vector<char> scratch(scratch_size);
        std::vector<crypto::hash> ids;ids.reserve(max_ids);
        std::vector<crypto::hash> tree(tree_count);
        uint64_t height=envelope.start_height;
        crypto::hash previous=currency::null_hash;
        bool first=true;
        size_t block_number=0;
        for(const auto& source:envelope.blocks) {
          if(!budget.charge(0,0,0))return refused();
          const bool keep=block_number<selected;
          std::shared_ptr<currency::block_extended_info> block;
          std::shared_ptr<currency::transaction_chain_entry> coinbase;
          currency::block_direct_data_entry target;
          if(keep) {
            if(!budget.charge(1,sizeof(currency::block_direct_data_entry)+list_node_bytes+
              sizeof(currency::block_extended_info)+sizeof(currency::transaction_chain_entry)+
              2*shared_control_bytes,0))return refused();
            block=std::make_shared<currency::block_extended_info>();
            coinbase=std::make_shared<currency::transaction_chain_entry>();
          }
          wire::block_summary summary;
          failure=wire::decode_block(source.block,budget,scratch,keep?&block->bl:nullptr,summary,&work);
          if(failure!=transport_failure::None)return remap(failure,&work);
          if(!summary.miner.first_input_generation || summary.miner.generation_height!=height ||
             (!first && summary.header.prev_id!=previous) ||
             source.coinbase_index_count!=summary.miner.outputs ||
             source.tx_count!=summary.transaction_count || source.tx_index_count!=source.tx_count)
            return transport_failure::InvalidResponse;
          if(!block_hash(summary,ids,tree,scratch,previous))
            return budget.cancelled?watch.reason():transport_failure::InvalidResponse;
          if(keep) {
            if(!copy_indices(source.coinbase_indices,source.coinbase_index_count,
              coinbase->m_global_output_indexes))return refused();
            target.block_ptr=std::move(block);target.coinbase_ptr=std::move(coinbase);
          }
          tx_context txs{*this,summary,scratch,work,keep,keep?&target:nullptr,0};
          failure=wire::for_each_tx_pair(source,budget,&prefix_decoder::transaction_pair,&txs);
          if(failure!=transport_failure::None)return remap(failure);
          if(txs.index!=source.tx_count)return transport_failure::InvalidResponse;
          if(keep)candidate.selected.blocks.push_back(std::move(target));
          ++height;++block_number;first=false;
        }
        if(!budget.charge(0,0,0))return refused();
        // A stop arriving during the clock sample of the final poll also
        // prevents publication; scanner still rechecks its borrowed token.
        if(watch.stop.load(std::memory_order_relaxed))return transport_failure::Stopped;
        out=std::move(candidate); // Only owned graph, no borrowed views escape.
        return transport_failure::None;
      }
    private:
      transport_failure refused() {
        if(budget.cancelled || watch.fired)return watch.reason();
        if(budget.exhausted) {
          if(context==transport_limit_context::Unobserved) {
            switch(budget.first_refusal) {
              case binary_decode_refusal::Arithmetic:context=transport_limit_context::DecodedArithmetic;break;
              case binary_decode_refusal::Allocation:context=transport_limit_context::DecodedAllocation;break;
              case binary_decode_refusal::Collection:context=transport_limit_context::DecodedCollection;break;
              default:break;
            }
          }
          return transport_failure::LimitExceeded;
        }
        return transport_failure::MalformedResponse;
      }
      transport_failure remap(transport_failure failure,const wire::inner_collection_work* work=nullptr) {
        if(budget.cancelled || watch.fired)return watch.reason();
        if(budget.exhausted)return refused();
        if(work && work->refused) {
          if(context==transport_limit_context::Unobserved)
            context=transport_limit_context::DecodedCollection;
          return transport_failure::LimitExceeded;
        }
        return failure;
      }
      bool copy_indices(wire::bytes_view bytes,size_t count,std::vector<uint64_t>& out) {
        if(!budget.charge(count,sizeof(uint64_t),0))return false;
        out.reserve(count);
        for(size_t i=0;i<count;++i) {
          uint64_t value=0;
          if(!budget.charge(0,0,0) || !wire::read_index(bytes,i,value))return false;
          out.push_back(value);
        }
        return true;
      }
      bool block_hash(const wire::block_summary& block,std::vector<crypto::hash>& ids,
        std::vector<crypto::hash>& tree,std::vector<char>& scratch,crypto::hash& hash) {
        if(!budget.charge(0,0,0) || block.transaction_count>=ids.capacity() ||
          block.transaction_hashes.size!=block.transaction_count*sizeof(crypto::hash))return false;
        ids.clear();ids.push_back(block.miner.prefix_hash);
        for(size_t i=0;i<block.transaction_count;++i) {
          crypto::hash id{};
          std::memcpy(&id,block.transaction_hashes.data+i*sizeof(id),sizeof(id));
          ids.push_back(id);
        }
        crypto::hash root{};
        if(!crypto::tree_hash_checked(ids.data(),ids.size(),tree.data(),tree.size(),root))return false;
        size_t header_size=0;
        if(!currency::serialize_object_to_buffer(block.header,scratch,header_size))return false;
        char count[(std::numeric_limits<size_t>::digits+6)/7];char* end=count;
        tools::write_varint(end,ids.size());
        const size_t count_size=static_cast<size_t>(end-count);
        const size_t blob_size=header_size+sizeof(root)+count_size;
        if(blob_size>currency::BLOCK_HASHING_BLOB_MAX_BYTES)return false;
        std::memcpy(scratch.data()+header_size,&root,sizeof(root));
        std::memcpy(scratch.data()+header_size+sizeof(root),count,count_size);
        char length[(std::numeric_limits<size_t>::digits+6)/7];char* length_end=length;
        tools::write_varint(length_end,blob_size);
        const size_t length_size=static_cast<size_t>(length_end-length);
        if(length_size>scratch.size() || blob_size>scratch.size()-length_size)return false;
        std::memmove(scratch.data()+length_size,scratch.data(),blob_size);
        std::memcpy(scratch.data(),length,length_size);
        if(!budget.charge(0,0,0))return false;
        crypto::cn_fast_hash(scratch.data(),length_size+blob_size,hash);
        return budget.charge(0,0,0);
      }
      struct tx_context {
        prefix_decoder& owner;const wire::block_summary& block;std::vector<char>& scratch;
        wire::inner_collection_work& work;
        bool keep;currency::block_direct_data_entry* target;size_t index;
      };
      static transport_failure transaction_pair(void* state,wire::bytes_view blob,
        wire::bytes_view indices,size_t index_count) {
        tx_context& context=*static_cast<tx_context*>(state);
        prefix_decoder& owner=context.owner;
        if(context.index>=context.block.transaction_count)return transport_failure::InvalidResponse;
        std::shared_ptr<currency::transaction_chain_entry> tx;
        if(context.keep) {
          if(!owner.budget.charge(1,sizeof(currency::transaction_chain_entry)+
              shared_control_bytes+list_node_bytes,0))return owner.refused();
          tx=std::make_shared<currency::transaction_chain_entry>();
        }
        wire::transaction_summary summary;
        const transport_failure failure=wire::decode_transaction(blob,owner.budget,
          context.scratch,context.keep?&tx->tx:nullptr,summary,&context.work);
        if(failure!=transport_failure::None)return owner.remap(failure,&context.work);
        crypto::hash expected{};
        std::memcpy(&expected,context.block.transaction_hashes.data+
          context.index*sizeof(expected),sizeof(expected));
        if(summary.prefix_hash!=expected || index_count!=summary.outputs)
          return transport_failure::InvalidResponse;
        if(context.keep) {
          if(!owner.copy_indices(indices,index_count,tx->m_global_output_indexes))return owner.refused();
          context.target->txs_ptr.push_back(std::move(tx));
        }
        ++context.index;return transport_failure::None;
      }
      const block_transport_limits& limits;decode_watch& watch;
      const uint64_t requested_count;
      binary_decode_budget budget;
      transport_limit_context context=transport_limit_context::Unobserved;
    };
  }

  bool response_sink::append(const void* data, size_t size) noexcept
  {
    if (failed())
      return false;
    if (!size)
      return true;
    if (!data)
    {
      m_failed = true;
      return false;
    }
    if (size > m_limit || m_body.size() > m_limit - size)
    {
      m_overflow = true; // Refuse whole; never trim.
      return false;
    }
    try
    {
      const size_t needed = m_body.size() + size;
      if (needed > m_body.capacity())
        m_body.reserve(std::min(m_limit, std::max(needed, m_body.capacity() * 2)));
      m_body.append(static_cast<const char*>(data), size);
      return true;
    }
    catch (...)
    {
      m_failed = true;
      return false;
    }
  }

  class block_transport final : public i_core_proxy, public validated_prefix_source, public birthday_height_source, public birthday_prefix_source
  {
  public:
    block_transport(std::shared_ptr<block_exchange> exchange, const block_transport_limits& limits,
      std::shared_ptr<const monotonic_clock> clock)
      : m_exchange(std::move(exchange)), m_limits(limits), m_clock(std::move(clock)) {}

    bool call_COMMAND_RPC_GET_BLOCKS_DIRECT(const direct_request& rqt, direct_response& rsp) override
    {
      // Same bounded path; this legacy entry just has no owner token to observe.
      const std::atomic<bool> never_stopped{false};
      return call_COMMAND_RPC_GET_BLOCKS_DIRECT(rqt, rsp, never_stopped);
    }

    bool call_COMMAND_RPC_GET_BLOCKS_DIRECT(const direct_request& rqt, direct_response& rsp,
      const std::atomic<bool>& stop) override
    {
      transport_failure failure = transport_failure::Busy;
      transport_limit_context limit = transport_limit_context::Unobserved;
      bool idle = false;
      if (m_active.compare_exchange_strong(idle, true))
      {
        try { failure = fetch(rqt, rsp, stop, limit); }
        catch (...) { failure = transport_failure::LocalFailure; }
        m_active.store(false);
      }
      if (failure != transport_failure::None)
        rsp = direct_response{}; // No partial DIRECT data on any failure.
      // Same original publication position. One word pairs this call's broad
      // failure with its first refusal; every call begins with unobserved limit.
      if (failure != transport_failure::ResponseTooLarge && failure != transport_failure::LimitExceeded)
        limit = transport_limit_context::Unobserved;
      m_last_observation.store(pack_observation(failure, limit));
      return failure == transport_failure::None;
    }

    bool fetch_validated_prefix(const direct_request& rqt,prefix_selection_limits selection,
      validated_prefix_response& rsp,const std::atomic<bool>& stop) override
    {
      rsp={};
      transport_failure failure=transport_failure::Busy;
      transport_limit_context limit=transport_limit_context::Unobserved;
      bool idle=false;
      if(m_active.compare_exchange_strong(idle,true)) {
        try {
          if(selection.max_blocks<2 || selection.max_blocks>COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT ||
             !selection.max_transactions) failure=transport_failure::InvalidRequest;
          else failure=fetch(rqt,rsp.selected,stop,limit,&selection,&rsp);
        } catch(...) {failure=transport_failure::LocalFailure;}
        m_active.store(false);
      }
      if(failure!=transport_failure::None)rsp={};
      if(failure!=transport_failure::ResponseTooLarge && failure!=transport_failure::LimitExceeded)
        limit=transport_limit_context::Unobserved;
      m_last_observation.store(pack_observation(failure,limit));
      return failure==transport_failure::None;
    }

    bool resolve_birthday_height(uint64_t timestamp, uint64_t& height,
      const std::atomic<bool>& stop) override
    {
      height = 0;
      bool idle = false;
      transport_failure failure = transport_failure::Busy;
      uint64_t candidate = 0;
      if (m_active.compare_exchange_strong(idle, true)) {
        try { failure = fetch_birthday(timestamp, candidate, stop); }
        catch (...) { failure = transport_failure::LocalFailure; }
        m_active.store(false);
      }
      m_last_observation.store(pack_observation(failure, transport_limit_context::Unobserved));
      if (failure != transport_failure::None || stop.load(std::memory_order_relaxed)) return false;
      height = candidate;
      return true;
    }

    bool fetch_birthday_prefix_page(const birthday_prefix::request& request,
      birthday_prefix::page& response, const std::atomic<bool>& stop) override
    {
      response = {};
      bool idle = false;
      transport_failure failure = transport_failure::Busy;
      transport_limit_context limit = transport_limit_context::Unobserved;
      if (m_active.compare_exchange_strong(idle, true)) {
        try { failure = fetch_prefix_page(request, response, stop, limit); }
        catch (...) { failure = transport_failure::LocalFailure; }
        m_active.store(false);
      }
      if (failure != transport_failure::None) response = {};
      if (failure != transport_failure::LimitExceeded &&
          failure != transport_failure::ResponseTooLarge)
        limit = transport_limit_context::Unobserved;
      m_last_observation.store(pack_observation(failure, limit));
      return failure == transport_failure::None;
    }

    transport_failure last_failure() const noexcept { return unpack_failure(m_last_observation.load()); }
    transport_observation last_observation() const noexcept { return unpack_observation(m_last_observation.load()); }

  private:
    transport_failure fetch_prefix_page(const birthday_prefix::request& request,
      birthday_prefix::page& response, const std::atomic<bool>& stop,
      transport_limit_context& limit)
    {
      time_point deadline{};
      if (!deadline_after(m_clock->now(), m_limits.exchange_timeout, deadline))
        return transport_failure::LocalFailure;
      if (stop.load(std::memory_order_relaxed)) return transport_failure::Stopped;
      if (!birthday_prefix::valid(request) || request.start_height >= CURRENCY_MAX_BLOCK_NUMBER ||
          request.target_height >= CURRENCY_MAX_BLOCK_NUMBER)
        return transport_failure::InvalidRequest;
      // Fixed retained page, no decoded dynamic strings/tree/containers. Charge
      // its whole storage, even when this particular page has fewer rows.
      if (sizeof(birthday_prefix::page) > m_limits.max_decode_allocation_bytes) {
        limit = transport_limit_context::DecodedAllocation;
        return transport_failure::LimitExceeded;
      }
      const auto encoded = birthday_prefix::encode(request);
      if (encoded.empty() || encoded.size() > birthday_prefix::max_request_bytes ||
          encoded.size() > m_limits.max_request_bytes)
        return transport_failure::RequestTooLarge;
      std::string body;
      response_sink sink(body, std::min(m_limits.max_response_bytes, birthday_prefix::max_response_bytes));
      const exchange_context context(stop, *m_clock, deadline);
      if (context.should_abort())
        return stop.load() ? transport_failure::Stopped : transport_failure::ExchangeDeadline;
      const auto status = m_exchange->exchange_birthday_prefix_json(
        reinterpret_cast<const unsigned char*>(encoded.data()), encoded.size(), context, sink);
      if (stop.load(std::memory_order_relaxed)) return transport_failure::Stopped;
      if (m_clock->now() >= deadline) return transport_failure::ExchangeDeadline;
      if (sink.m_overflow) { limit = transport_limit_context::BodyBytes; return transport_failure::ResponseTooLarge; }
      if (sink.m_failed) return transport_failure::LocalFailure;
      if (status != exchange_status::Ok) return transport_failure::ExchangeFailed;
      time_point decode_deadline{};
      if (!deadline_after(m_clock->now(), m_limits.decode_timeout, decode_deadline))
        return transport_failure::LocalFailure;
      decode_watch watch{stop, *m_clock, decode_deadline};
      const auto aborted = [](void* value) noexcept { return decode_watch::poll(value); };
      birthday_prefix::response_reader reader(body, aborted, &watch,
        m_limits.max_collection_elements, m_limits.max_storage_entries,
        m_limits.max_storage_sections, m_limits.max_storage_array_elements,
        m_limits.max_nesting_depth);
      if (!reader.parse(response)) {
        if (watch.fired) return watch.reason();
        if (reader.element_limit_exceeded()) {
          limit = transport_limit_context::DecodedCollection;
          return transport_failure::LimitExceeded;
        }
        return transport_failure::MalformedResponse;
      }
      if (response.start_height != request.start_height ||
          response.view.genesis_hash != request.genesis_hash ||
          response.view.target_height >= CURRENCY_MAX_BLOCK_NUMBER ||
          response.start_height > response.view.target_height ||
          !birthday_prefix::policy::nonzero_hash(response.view.target_hash) ||
          (birthday_prefix::policy::nonzero_hash(request.target_hash) &&
           (request.target_hash != response.view.target_hash ||
            request.target_height != response.view.target_height)))
        return transport_failure::InvalidResponse;
      const auto available = response.view.target_height - response.start_height + 1;
      if (response.count != std::min<uint64_t>(request.count, available))
        return transport_failure::InvalidResponse;
      // The scanner's selector validates canonical genesis, contiguous heights,
      // all links and the full tail before retaining any prefix progress, under
      // this same remaining decode budget. No fresh selector deadline.
      if (stop.load(std::memory_order_relaxed)) return transport_failure::Stopped;
      if (m_clock->now() >= decode_deadline) return transport_failure::DecodeDeadline;
      response.decode_clock = m_clock;
      response.decode_deadline = decode_deadline;
      return transport_failure::None;
    }

    transport_failure fetch_birthday(uint64_t timestamp, uint64_t& height,
      const std::atomic<bool>& stop)
    {
      time_point deadline{};
      if (!deadline_after(m_clock->now(), m_limits.exchange_timeout, deadline))
        return transport_failure::LocalFailure;
      if (stop.load(std::memory_order_relaxed)) return transport_failure::Stopped;
      const std::string request = "{\"jsonrpc\":\"2.0\",\"id\":\"0\",\"method\":\"get_est_height_from_date\",\"params\":{\"timestamp\":" +
        std::to_string(timestamp) + "}}";
      if (request.size() > birthday::max_request_bytes) return transport_failure::RequestTooLarge;
      std::string body;
      response_sink sink(body, std::min(m_limits.max_response_bytes, birthday::max_response_bytes));
      const exchange_context context(stop, *m_clock, deadline);
      if (context.should_abort()) return stop.load() ? transport_failure::Stopped : transport_failure::ExchangeDeadline;
      const auto status = m_exchange->exchange_birthday_json(
        reinterpret_cast<const unsigned char*>(request.data()), request.size(), context, sink);
      if (stop.load(std::memory_order_relaxed)) return transport_failure::Stopped;
      if (m_clock->now() >= deadline) return transport_failure::ExchangeDeadline;
      if (sink.m_overflow) return transport_failure::ResponseTooLarge;
      if (sink.m_failed) return transport_failure::LocalFailure;
      if (status != exchange_status::Ok) return transport_failure::ExchangeFailed;
      time_point decode_deadline{};
      if (!deadline_after(m_clock->now(), m_limits.decode_timeout, decode_deadline))
        return transport_failure::LocalFailure;
      decode_watch watch{stop, *m_clock, decode_deadline};
      const auto aborted = [](void* value) { return decode_watch::poll(value); };
      birthday::response_reader reader(body, aborted, &watch);
      if (!reader.parse(height)) return watch.fired ? watch.reason() : transport_failure::MalformedResponse;
      if (stop.load(std::memory_order_relaxed)) return transport_failure::Stopped;
      if (m_clock->now() >= decode_deadline) return transport_failure::DecodeDeadline;
      return transport_failure::None;
    }

    transport_failure fetch(const direct_request& rqt, direct_response& rsp, const std::atomic<bool>& stop,
      transport_limit_context& limit,const prefix_selection_limits* selection=nullptr,
      validated_prefix_response* prefix_out=nullptr)
    {
      rsp = direct_response{};
      time_point exchange_deadline{};
      if (!deadline_after(m_clock->now(), m_limits.exchange_timeout, exchange_deadline))
        return transport_failure::LocalFailure;
      if (stop.load(std::memory_order_relaxed))
        return transport_failure::Stopped;

      // Bound the history before building the request with the existing codec.
      if (rqt.block_ids.empty() || !currency::get_blocks_max_count_valid(rqt.max_count))
        return transport_failure::InvalidRequest;
      if (rqt.block_ids.size() > m_limits.max_request_bytes / sizeof(crypto::hash))
        return transport_failure::RequestTooLarge;
      // Normal native requests ask for 128 complete blocks. Explicit smaller
      // caller/decoder bounds are honored; no caller may widen this fixed batch.
      const uint64_t requested_count = std::min<uint64_t>(
        std::min<uint64_t>(native_request_block_count,
          selection ? selection->max_blocks : native_request_block_count),
        std::min<uint64_t>(m_limits.max_blocks, rqt.max_count.value_or(native_request_block_count)));
      block_transport_limits response_limits = m_limits;
      response_limits.max_blocks = static_cast<size_t>(requested_count);
      std::string request_body;
      {
        fast_request request{};
        request.minimum_height = rqt.minimum_height;
        request.block_ids = rqt.block_ids;
        request.max_count = requested_count;
        // The generic helper discards store()'s bool. Check both stages here so
        // a failed optional-field write cannot silently become a legacy request.
        ps::portable_storage storage;
        if (!request.store(storage) || !storage.store_to_binary(request_body))
          return transport_failure::LocalFailure;
      }
      if (request_body.empty() || request_body.size() > m_limits.max_request_bytes)
        return transport_failure::RequestTooLarge;
      if (stop.load(std::memory_order_relaxed))
        return transport_failure::Stopped;
      if (m_clock->now() >= exchange_deadline)
        return transport_failure::ExchangeDeadline;

      // Exactly one exchange; no retry on any outcome.
      std::string body;
      exchange_status status = exchange_status::Failed;
      {
        response_sink sink(body, m_limits.max_response_bytes);
        const exchange_context context(stop, *m_clock, exchange_deadline);
        try
        {
          status = m_exchange->exchange_getblocks_bin(
            reinterpret_cast<const unsigned char*>(request_body.data()), request_body.size(), context, sink);
        }
        catch (...)
        {
          status = exchange_status::Failed;
        }
        // Checked after return, before any byte is interpreted: late data is discarded.
        if (stop.load(std::memory_order_relaxed))
          return transport_failure::Stopped;
        if (m_clock->now() >= exchange_deadline)
          return transport_failure::ExchangeDeadline;
        if (sink.m_overflow) {
          limit = transport_limit_context::BodyBytes;
          return transport_failure::ResponseTooLarge;
        }
        if (sink.m_failed)
          return transport_failure::LocalFailure;
      }
      if (status != exchange_status::Ok)
        return transport_failure::ExchangeFailed;
      if (body.empty())
        return transport_failure::MalformedResponse;

      time_point decode_deadline{};
      if (!deadline_after(m_clock->now(), m_limits.decode_timeout, decode_deadline))
        return transport_failure::LocalFailure;
      decode_watch watch{stop, *m_clock, decode_deadline};
      if(selection && prefix_out) {
        prefix_decoder decoder(response_limits,watch,requested_count);
        const transport_failure failure=decoder.decode(body,*selection,*prefix_out);
        if(failure==transport_failure::LimitExceeded)limit=decoder.limit_context();
        return failure;
      }
      response_decoder decoder(response_limits, watch, requested_count);
      direct_response decoded{};
      const transport_failure failure = decoder.decode(body, decoded);
      if (failure != transport_failure::None) {
        if (failure == transport_failure::LimitExceeded) limit = decoder.limit_context();
        return failure;
      }
      rsp = std::move(decoded); // Published whole, only after full validation.
      return transport_failure::None;
    }

    const std::shared_ptr<block_exchange> m_exchange;
    const block_transport_limits m_limits;
    const std::shared_ptr<const monotonic_clock> m_clock;
    std::atomic<bool> m_active{false};
    std::atomic<uint64_t> m_last_observation{pack_observation(transport_failure::None, transport_limit_context::Unobserved)};
  };

  bool block_transport_limits_valid(const block_transport_limits& l) noexcept
  {
    const block_transport_limits& m = policy_maxima;
    auto within = [](uint64_t value, uint64_t maximum) { return value != 0 && value <= maximum; };
    if (!within(l.max_request_bytes, m.max_request_bytes) ||
        !within(l.max_response_bytes, m.max_response_bytes) ||
        !within(l.max_blob_bytes, m.max_blob_bytes) ||
        !within(l.max_blocks, m.max_blocks) ||
        !within(l.max_transactions, m.max_transactions) ||
        !within(l.max_output_indices, m.max_output_indices) ||
        !within(l.max_storage_entries, m.max_storage_entries) ||
        !within(l.max_storage_sections, m.max_storage_sections) ||
        !within(l.max_storage_array_elements, m.max_storage_array_elements) ||
        !within(l.max_nesting_depth, m.max_nesting_depth) ||
        !within(l.max_collection_elements, m.max_collection_elements) ||
        !within(l.max_decode_allocation_bytes, m.max_decode_allocation_bytes))
      return false;
    if (l.exchange_timeout.count() <= 0 || l.exchange_timeout > m.exchange_timeout ||
        l.decode_timeout.count() <= 0 || l.decode_timeout > m.decode_timeout)
      return false;
    // Contradictions: settings that make another limit unreachable.
    return l.max_request_bytes >= min_request_bytes &&
      l.max_nesting_depth >= min_nesting_depth &&
      l.max_blob_bytes <= l.max_response_bytes &&
      l.max_transactions >= l.max_blocks &&
      l.max_blocks <= l.max_storage_sections &&
      l.max_output_indices <= l.max_storage_array_elements;
  }

  std::shared_ptr<i_core_proxy> make_block_transport(std::shared_ptr<block_exchange> exchange,
    const block_transport_limits& limits, std::shared_ptr<const monotonic_clock> clock) noexcept
  {
    if (!exchange || !block_transport_limits_valid(limits))
      return nullptr;
    try
    {
      if (!clock)
        clock = std::make_shared<steady_clock_source>();
      return std::make_shared<block_transport>(std::move(exchange), limits, std::move(clock));
    }
    catch (...)
    {
      return nullptr;
    }
  }

  transport_failure block_transport_last_failure(const i_core_proxy& transport) noexcept
  {
    const auto* typed = dynamic_cast<const block_transport*>(&transport);
    return typed ? typed->last_failure() : transport_failure::NotBlockTransport;
  }
  transport_observation block_transport_last_observation(const i_core_proxy& transport) noexcept {
    const auto* typed = dynamic_cast<const block_transport*>(&transport);
    return typed ? typed->last_observation() : transport_observation{};
  }
}}
