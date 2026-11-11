// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
// Pure structural selection over records supplied by the admitted node/SDK view.
// The caller must obtain hashes and timestamps from coherent canonical headers.
// This is not consensus validation, SPV, or proof of real-world creation time.
#ifndef ZANO_NATIVE_WALLET_BIRTHDAY_PREFIX_H
#define ZANO_NATIVE_WALLET_BIRTHDAY_PREFIX_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace zano
{
namespace native_wallet_birthday_prefix
{

using Hash = std::array<std::uint8_t, 32>;

inline constexpr std::size_t kMaxHeadersPerPage = 1000;
inline constexpr std::uint64_t kEncodedWeekOffset = 1543622400;
inline constexpr std::uint64_t kEncodedWeekSeconds = 604800;
inline constexpr std::uint64_t kCutoffSafetySeconds = 90000;

struct Header
{
  std::uint64_t height = 0;
  std::uint64_t timestamp = 0;
  Hash hash{};
  Hash previous_hash{};
};

// The opaque token is supplied by the producer, not invented by the selector.
// Target and genesis metadata must remain identical on every page.
struct ChainView
{
  Hash token{};
  std::uint64_t target_height = 0;
  Hash target_hash{};
  Hash genesis_hash{};
};

struct Request
{
  std::uint64_t rounded_date = 0;
  // One trusted snapshot for the entire operation, not a per-page clock.
  std::uint64_t trusted_now = 0;
  // The consumer supplies its compiled network genesis, independently of the
  // producer's view metadata. Producer-chosen genesis is not admission.
  Hash expected_genesis{};
  ChainView view{};
};

struct HeaderPage
{
  ChainView view{};
  std::uint64_t first_height = 0;
  // Borrowed and immutable for the duration of consume().
  const Header* headers = nullptr;
  std::size_t count = 0;
};

enum class Status
{
  NeedMore,
  BoundaryFound,
  NoBoundary,
  Cancelled,
  InvalidData,
  InvalidRequest,
  Unsupported,
  AlreadyTerminal
};

struct Boundary
{
  std::uint64_t rounded_date = 0;
  std::uint64_t cutoff = 0;
  Header candidate{};
  Header predecessor{};
  ChainView view{};
};

struct Result
{
  Status status;
  // Engaged only for BoundaryFound. A failed result carries no boundary.
  std::optional<Boundary> boundary;
};

// true means cancellation OR deadline expiry. The caller owns the clock,
// whole-operation slot, and enclosing transport/decode limits.
using StopPredicate = bool (*)(void*) noexcept;

inline bool nonzero_hash(const Hash& value) noexcept
{
  for (const auto byte : value)
    if (byte != 0)
      return true;
  return false;
}

inline bool same_view(const ChainView& a, const ChainView& b) noexcept
{
  return a.token == b.token &&
         a.target_height == b.target_height &&
         a.target_hash == b.target_hash &&
         a.genesis_hash == b.genesis_hash;
}

// Constant retained storage, no allocation and no I/O. One selector is one
// attempt: it has no reset/retry API, and cannot be copied into another attempt.
// Borrowed pages do not relax the existing 262144 per-collection/cumulative
// decoded-element or 64 MiB allocation budgets. The producer must still enforce
// those budgets, exchange 15 s / 16 MiB and decode 2 s before handing us records.
class Selector final
{
public:
  Selector(const Request& request, StopPredicate stop, void* stop_context) noexcept
    : request_(request), stop_(stop), stop_context_(stop_context)
  {
  }

  Selector(const Selector&) = delete;
  Selector& operator=(const Selector&) = delete;
  Selector(Selector&&) = delete;
  Selector& operator=(Selector&&) = delete;

  Result consume(const HeaderPage& page) noexcept
  {
    if (terminal_)
      return {Status::AlreadyTerminal, std::nullopt};

    if (stop_ == nullptr)
      return finish(Status::InvalidRequest);

    if (stop_(stop_context_))
      return finish(Status::Cancelled);

    if (!valid_request())
      return finish(Status::InvalidRequest);

    if (!same_view(page.view, request_.view) ||
        page.first_height != next_height_ ||
        page.count == 0 ||
        page.count > kMaxHeadersPerPage ||
        page.headers == nullptr)
      return finish(Status::InvalidData);

    // All cursor/proof changes are local until the complete page, including
    // its tail, and the final stop/deadline sample have passed.
    std::uint64_t local_next = next_height_;
    Header local_previous = previous_;
    bool local_has_previous = has_previous_;
    bool reached_target = false;
    bool found = false;
    Header candidate{};
    Header predecessor{};
    const std::uint64_t cutoff = request_.rounded_date - kCutoffSafetySeconds;

    for (std::size_t i = 0; i < page.count; ++i)
    {
      if (stop_(stop_context_))
        return finish(Status::Cancelled);

      const Header& header = page.headers[i];
      // reached_target also avoids incrementing UINT64_MAX on an exhausted
      // view and refuses every extra record after the bound target.
      if (reached_target ||
          !nonzero_hash(header.hash) ||
          header.height != local_next ||
          header.height > request_.view.target_height)
        return finish(Status::InvalidData);

      if (header.height == 0)
      {
        if (local_has_previous || header.hash != request_.expected_genesis ||
            nonzero_hash(header.previous_hash))
          return finish(Status::InvalidData);
      }
      else if (!local_has_previous ||
               header.previous_hash != local_previous.hash ||
               header.hash == local_previous.hash)
      {
        return finish(Status::InvalidData);
      }

      if (header.height == request_.view.target_height &&
          header.hash != request_.view.target_hash)
        return finish(Status::InvalidData);

      // Only sequentially validated heights can establish earliestness.
      // Timestamp monotonicity is deliberately neither assumed nor required.
      if (!found && header.timestamp >= cutoff)
      {
        found = true;
        candidate = header;
        if (local_has_previous)
          predecessor = local_previous;
      }

      local_previous = header;
      local_has_previous = true;
      if (header.height == request_.view.target_height)
        reached_target = true;
      else
        ++local_next;
    }

    if (stop_(stop_context_))
      return finish(Status::Cancelled);

    // A successful candidate at height zero has no actual predecessor.
    // Never translate it, or an exhausted nonqualifying view, into zero.
    if (found)
    {
      if (candidate.height == 0)
        return finish(Status::Unsupported);

      Boundary boundary{};
      boundary.rounded_date = request_.rounded_date;
      boundary.cutoff = cutoff;
      boundary.candidate = candidate;
      boundary.predecessor = predecessor;
      boundary.view = request_.view;
      terminal_ = true;
      return {Status::BoundaryFound, boundary};
    }

    if (reached_target)
      return finish(Status::NoBoundary);

    previous_ = local_previous;
    has_previous_ = local_has_previous;
    next_height_ = local_next;
    return {Status::NeedMore, std::nullopt};
  }

private:
  bool valid_request() const noexcept
  {
    if (request_.rounded_date <= kEncodedWeekOffset ||
        request_.rounded_date > request_.trusted_now ||
        !nonzero_hash(request_.expected_genesis) ||
        !nonzero_hash(request_.view.token) ||
        !nonzero_hash(request_.view.target_hash) ||
        request_.expected_genesis != request_.view.genesis_hash)
      return false;

    const std::uint64_t elapsed = request_.rounded_date - kEncodedWeekOffset;
    const std::uint64_t week = elapsed / kEncodedWeekSeconds;
    if (elapsed % kEncodedWeekSeconds != 0 || week == 0 || week >= 800)
      return false;

    return request_.view.target_height != 0 ||
           request_.view.target_hash == request_.expected_genesis;
  }

  Result finish(Status status) noexcept
  {
    terminal_ = true;
    return {status, std::nullopt};
  }

  const Request request_;
  const StopPredicate stop_;
  void* const stop_context_;
  bool terminal_ = false;
  std::uint64_t next_height_ = 0;
  Header previous_{};
  bool has_previous_ = false;
};

} // namespace native_wallet_birthday_prefix
} // namespace zano

#endif // ZANO_NATIVE_WALLET_BIRTHDAY_PREFIX_H
