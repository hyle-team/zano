// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
// Standalone deterministic semantic tests. No SDK, node, wallet or real hashes.
// QA owns compilation/execution; the author has not run this translation unit.
#include "../../src/wallet/native_wallet_birthday_prefix.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <cstdint>

namespace prefix = zano::native_wallet_birthday_prefix;

namespace
{

constexpr std::uint64_t date =
  prefix::kEncodedWeekOffset + 100 * prefix::kEncodedWeekSeconds;
constexpr std::uint64_t cutoff = date - prefix::kCutoffSafetySeconds;

void expect(bool condition, const char* description)
{
  if (!condition)
  {
    std::fprintf(stderr, "FAILED: %s\n", description);
    std::exit(EXIT_FAILURE);
  }
}

void expect_status(const prefix::Result& result,
                   prefix::Status expected,
                   const char* description)
{
  expect(result.status == expected, description);
  if (expected != prefix::Status::BoundaryFound)
    expect(!result.boundary.has_value(), "non-success has no boundary payload");
}

prefix::Hash identity(std::uint64_t number)
{
  prefix::Hash result{};
  for (std::size_t i = 0; i != 8; ++i)
  {
    result[i] = static_cast<std::uint8_t>(number & 0xff);
    number >>= 8;
  }
  result[31] = 0xa5;
  return result;
}

template<std::size_t N>
std::array<prefix::Header, N> chain(
  const std::array<std::uint64_t, N>& timestamps)
{
  std::array<prefix::Header, N> result{};
  for (std::size_t i = 0; i < N; ++i)
  {
    result[i].height = i;
    result[i].timestamp = timestamps[i];
    result[i].hash = identity(i + 1);
    if (i != 0)
      result[i].previous_hash = result[i - 1].hash;
  }
  return result;
}

template<std::size_t N>
prefix::Request request_for(const std::array<prefix::Header, N>& headers)
{
  prefix::Request request{};
  request.rounded_date = date;
  request.trusted_now = date + 1000;
  request.expected_genesis = headers[0].hash;
  request.view.token = identity(9001);
  request.view.target_height = N - 1;
  request.view.target_hash = headers[N - 1].hash;
  request.view.genesis_hash = headers[0].hash;
  return request;
}

prefix::HeaderPage page(const prefix::Request& request,
                        const prefix::Header* headers,
                        std::uint64_t first_height,
                        std::size_t count)
{
  return {request.view, first_height, headers, count};
}

struct Stop
{
  std::size_t calls = 0;
  std::size_t cancel_at = 0;
};

bool stop_requested(void* context) noexcept
{
  Stop& stop = *static_cast<Stop*>(context);
  ++stop.calls;
  return stop.cancel_at != 0 && stop.calls >= stop.cancel_at;
}

bool same_header(const prefix::Header& a, const prefix::Header& b)
{
  return a.height == b.height && a.timestamp == b.timestamp &&
         a.hash == b.hash && a.previous_hash == b.previous_hash;
}

void expect_boundary(const prefix::Result& result,
                     const prefix::Request& request,
                     const prefix::Header& candidate,
                     const prefix::Header& predecessor)
{
  expect_status(result, prefix::Status::BoundaryFound, "boundary found");
  expect(result.boundary.has_value(), "success includes boundary");
  const auto& boundary = *result.boundary;
  expect(boundary.rounded_date == request.rounded_date, "date preserved");
  expect(boundary.cutoff == request.rounded_date - 90000, "cutoff uses 90000");
  expect(same_header(boundary.candidate, candidate), "actual earliest candidate");
  expect(same_header(boundary.predecessor, predecessor), "actual predecessor");
  expect(prefix::same_view(boundary.view, request.view), "bound view preserved");
}

void unordered_timestamps()
{
  const auto headers = chain<5>({cutoff - 10, cutoff - 1, cutoff + 50,
                                 cutoff - 400, cutoff + 100});
  const auto request = request_for(headers);
  Stop stop{};
  prefix::Selector selector(request, stop_requested, &stop);
  expect_boundary(selector.consume(page(request, headers.data(), 0, 5)),
                  request, headers[2], headers[1]);
  expect_status(selector.consume(page(request, headers.data(), 0, 5)),
                prefix::Status::AlreadyTerminal, "success is terminal");
}

void boundary_across_pages_and_inclusive_cutoff()
{
  const auto headers = chain<5>({cutoff - 100, cutoff - 50, cutoff,
                                 cutoff + 20, cutoff - 5});
  const auto request = request_for(headers);
  Stop stop{};
  prefix::Selector selector(request, stop_requested, &stop);
  expect_status(selector.consume(page(request, headers.data(), 0, 2)),
                prefix::Status::NeedMore, "validated prefix needs next page");
  expect_boundary(selector.consume(page(request, headers.data() + 2, 2, 3)),
                  request, headers[2], headers[1]);
}

void natural_genesis_predecessor()
{
  const auto headers = chain<3>({cutoff - 1, cutoff, cutoff - 2});
  const auto request = request_for(headers);
  Stop stop{};
  prefix::Selector selector(request, stop_requested, &stop);
  expect_boundary(selector.consume(page(request, headers.data(), 0, 3)),
                  request, headers[1], headers[0]);
}

void bad_tail_after_candidate()
{
  const auto original = chain<4>({cutoff - 1, cutoff, cutoff - 2, cutoff + 1});
  const auto request = request_for(original);
  for (int defect = 0; defect < 3; ++defect)
  {
    auto headers = original;
    if (defect == 0)
      headers[2].previous_hash = identity(1234);
    else if (defect == 1)
      headers[2].height = 3;
    else
      headers[3].hash = identity(4321);
    Stop stop{};
    prefix::Selector selector(request, stop_requested, &stop);
    expect_status(selector.consume(page(request, headers.data(), 0, 4)),
                  prefix::Status::InvalidData, "candidate cannot hide bad tail");
    expect_status(selector.consume(page(request, original.data(), 0, 4)),
                  prefix::Status::AlreadyTerminal, "invalid attempt cannot retry");
  }
}

void duplicate_and_missing_heights()
{
  const auto original = chain<3>({cutoff - 3, cutoff - 2, cutoff});
  const auto request = request_for(original);
  for (std::uint64_t wrong_height : {std::uint64_t{0}, std::uint64_t{2}})
  {
    auto headers = original;
    headers[1].height = wrong_height;
    Stop stop{};
    prefix::Selector selector(request, stop_requested, &stop);
    expect_status(selector.consume(page(request, headers.data(), 0, 3)),
                  prefix::Status::InvalidData, "duplicate or skipped height refused");
  }
  Stop stop{};
  prefix::Selector selector(request, stop_requested, &stop);
  expect_status(selector.consume(page(request, original.data() + 1, 1, 2)),
                prefix::Status::InvalidData, "cannot start after genesis");
}

void wrong_genesis_and_previous_link()
{
  const auto original = chain<3>({cutoff - 3, cutoff - 2, cutoff});
  const auto request = request_for(original);
  for (int defect = 0; defect < 2; ++defect)
  {
    auto headers = original;
    if (defect == 0)
      headers[0].hash = identity(777);
    else
      headers[1].previous_hash = identity(888);
    Stop stop{};
    prefix::Selector selector(request, stop_requested, &stop);
    expect_status(selector.consume(page(request, headers.data(), 0, 3)),
                  prefix::Status::InvalidData, "genesis and hash links are checked");
  }
}

void changed_view_and_wrong_offset()
{
  const auto headers = chain<4>({cutoff - 4, cutoff - 3, cutoff - 2, cutoff});
  const auto request = request_for(headers);
  for (int defect = 0; defect < 5; ++defect)
  {
    Stop stop{};
    prefix::Selector selector(request, stop_requested, &stop);
    expect_status(selector.consume(page(request, headers.data(), 0, 2)),
                  prefix::Status::NeedMore, "first page admitted");
    auto next = page(request, headers.data() + 2, 2, 2);
    if (defect == 0)
      next.view.token = identity(9010);
    else if (defect == 1)
      ++next.view.target_height;
    else if (defect == 2)
      next.view.target_hash = identity(9011);
    else if (defect == 3)
      next.view.genesis_hash = identity(9012);
    else
      next.first_height = 1;
    expect_status(selector.consume(next), prefix::Status::InvalidData,
                  "view or cursor change refused across pages");
  }
}

void exhausted_view_and_target_identity()
{
  const auto headers = chain<3>({cutoff - 3, cutoff - 2, cutoff - 1});
  const auto request = request_for(headers);
  Stop stop{};
  prefix::Selector selector(request, stop_requested, &stop);
  expect_status(selector.consume(page(request, headers.data(), 0, 1)),
                prefix::Status::NeedMore, "prefix alone is not exhaustion");
  expect_status(selector.consume(page(request, headers.data() + 1, 1, 2)),
                prefix::Status::NoBoundary, "exhausted view has no boundary");
  expect_status(selector.consume(page(request, headers.data(), 0, 3)),
                prefix::Status::AlreadyTerminal, "exhaustion is terminal");

  auto wrong_target = request;
  wrong_target.view.target_hash = identity(123);
  Stop other_stop{};
  prefix::Selector other(wrong_target, stop_requested, &other_stop);
  expect_status(other.consume(page(wrong_target, headers.data(), 0, 3)),
                prefix::Status::InvalidData, "target record must match bound hash");
}

void genesis_candidate_is_unsupported()
{
  const auto headers = chain<3>({cutoff, cutoff - 2, cutoff + 1});
  const auto request = request_for(headers);
  Stop stop{};
  prefix::Selector selector(request, stop_requested, &stop);
  expect_status(selector.consume(page(request, headers.data(), 0, 3)),
                prefix::Status::Unsupported, "height zero cannot invent predecessor");
  expect_status(selector.consume(page(request, headers.data(), 0, 3)),
                prefix::Status::AlreadyTerminal, "unsupported is terminal");

  auto bad_tail = headers;
  bad_tail[2].previous_hash = identity(999);
  Stop tail_stop{};
  prefix::Selector tail_selector(request, stop_requested, &tail_stop);
  expect_status(tail_selector.consume(page(request, bad_tail.data(), 0, 3)),
                prefix::Status::InvalidData, "unsupported candidate still validates tail");
}

void invalid_requests()
{
  const auto headers = chain<2>({cutoff - 1, cutoff});
  const auto request = request_for(headers);
  const std::array<std::uint64_t, 5> invalid_dates{
    0,
    prefix::kEncodedWeekOffset,
    prefix::kEncodedWeekOffset + prefix::kEncodedWeekSeconds + 1,
    prefix::kEncodedWeekOffset + 800 * prefix::kEncodedWeekSeconds,
    date + prefix::kEncodedWeekSeconds
  };
  for (const auto invalid_date : invalid_dates)
  {
    auto invalid = request;
    invalid.rounded_date = invalid_date;
    // Isolate the week upper bound from the future-date rejection.
    if (invalid_date == prefix::kEncodedWeekOffset +
                        800 * prefix::kEncodedWeekSeconds)
      invalid.trusted_now = invalid_date;
    Stop stop{};
    prefix::Selector selector(invalid, stop_requested, &stop);
    expect_status(selector.consume(page(invalid, headers.data(), 0, 2)),
                  prefix::Status::InvalidRequest, "date must be canonical and not future");
    expect_status(selector.consume(page(request, headers.data(), 0, 2)),
                  prefix::Status::AlreadyTerminal, "invalid request cannot restart");
  }

  auto wrong_genesis = request;
  wrong_genesis.expected_genesis = identity(321);
  Stop stop{};
  prefix::Selector selector(wrong_genesis, stop_requested, &stop);
  expect_status(selector.consume(page(wrong_genesis, headers.data(), 0, 2)),
                prefix::Status::InvalidRequest, "producer genesis cannot override admission");

  prefix::Selector missing_stop(request, nullptr, nullptr);
  expect_status(missing_stop.consume(page(request, headers.data(), 0, 2)),
                prefix::Status::InvalidRequest, "stop/deadline predicate is mandatory");
}

void page_bounds_and_empty_data()
{
  std::array<std::uint64_t, 1001> timestamps{};
  timestamps.fill(cutoff - 1);
  const auto headers = chain<1001>(timestamps);
  const auto request = request_for(headers);
  Stop too_large_stop{};
  prefix::Selector too_large(request, stop_requested, &too_large_stop);
  expect_status(too_large.consume(page(request, headers.data(), 0, 1001)),
                prefix::Status::InvalidData, "1001 records exceeds page bound");

  Stop bounded_stop{};
  prefix::Selector bounded(request, stop_requested, &bounded_stop);
  expect_status(bounded.consume(page(request, headers.data(), 0, 1000)),
                prefix::Status::NeedMore, "1000 records is an admitted page");
  expect_status(bounded.consume(page(request, headers.data() + 1000, 1000, 1)),
                prefix::Status::NoBoundary, "bounded pages cover view");

  Stop empty_stop{};
  prefix::Selector empty(request, stop_requested, &empty_stop);
  expect_status(empty.consume(page(request, headers.data(), 0, 0)),
                prefix::Status::InvalidData, "empty page cannot claim progress");
  Stop null_stop{};
  prefix::Selector null_data(request, stop_requested, &null_stop);
  expect_status(null_data.consume(page(request, nullptr, 0, 1)),
                prefix::Status::InvalidData, "nonempty page needs borrowed data");

  const auto short_chain = chain<2>({cutoff - 2, cutoff - 1});
  const auto short_request = request_for(short_chain);
  Stop past_stop{};
  prefix::Selector past(short_request, stop_requested, &past_stop);
  expect_status(past.consume(page(short_request, headers.data(), 0, 3)),
                prefix::Status::InvalidData, "records past target refused");
}

void malformed_header_and_view_identities()
{
  const auto original = chain<4>({cutoff - 3, cutoff - 2, cutoff, cutoff + 1});
  const auto request = request_for(original);
  for (int defect = 0; defect < 3; ++defect)
  {
    auto headers = original;
    if (defect == 0)
    {
      headers[2].hash = {};
      headers[3].previous_hash = {};
    }
    else if (defect == 1)
      headers[0].previous_hash = identity(6001);
    else
    {
      headers[1].hash = headers[0].hash;
      headers[2].previous_hash = headers[1].hash;
    }
    Stop stop{};
    prefix::Selector selector(request, stop_requested, &stop);
    expect_status(selector.consume(page(request, headers.data(), 0, 4)),
                  prefix::Status::InvalidData,
                  "zero identity, genesis predecessor or adjacent repeated identity refused");
  }
  for (int defect = 0; defect < 3; ++defect)
  {
    auto invalid = request;
    if (defect == 0)
    {
      invalid.expected_genesis = {};
      invalid.view.genesis_hash = {};
    }
    else if (defect == 1)
      invalid.view.token = {};
    else
      invalid.view.target_hash = {};
    Stop stop{};
    prefix::Selector selector(invalid, stop_requested, &stop);
    expect_status(selector.consume(page(invalid, original.data(), 0, 4)),
                  prefix::Status::InvalidRequest,
                  "admitted genesis and chain-view identities must be nonzero");
  }
}

void cancellation_at_required_sampling_points()
{
  const auto headers = chain<3>({cutoff - 1, cutoff, cutoff - 2});
  const auto request = request_for(headers);
  // Before work, between records, and at the final publication sample.
  // Earlier additional samples are permitted; every case must still cancel.
  for (std::size_t cancel_at : {std::size_t{1}, std::size_t{3}, std::size_t{5}})
  {
    Stop stop{0, cancel_at};
    prefix::Selector selector(request, stop_requested, &stop);
    expect_status(selector.consume(page(request, headers.data(), 0, 3)),
                  prefix::Status::Cancelled, "cancellation publishes no boundary");
    stop.cancel_at = 0;
    expect_status(selector.consume(page(request, headers.data(), 0, 3)),
                  prefix::Status::AlreadyTerminal, "cancelled attempt never retries");
  }

  const auto later = chain<4>({cutoff - 4, cutoff - 3, cutoff - 2, cutoff});
  const auto later_request = request_for(later);
  Stop between{};
  prefix::Selector selector(later_request, stop_requested, &between);
  expect_status(selector.consume(page(later_request, later.data(), 0, 2)),
                prefix::Status::NeedMore, "first prefix completes before cancellation");
  between.cancel_at = between.calls + 1;
  expect_status(selector.consume(page(later_request, later.data() + 2, 2, 2)),
                prefix::Status::Cancelled, "cancellation between pages is terminal");
}

} // namespace

int main()
{
  unordered_timestamps();
  boundary_across_pages_and_inclusive_cutoff();
  natural_genesis_predecessor();
  bad_tail_after_candidate();
  duplicate_and_missing_heights();
  wrong_genesis_and_previous_link();
  changed_view_and_wrong_offset();
  exhausted_view_and_target_identity();
  genesis_candidate_is_unsupported();
  invalid_requests();
  page_bounds_and_empty_data();
  malformed_header_and_view_identities();
  cancellation_at_required_sampling_points();
  std::puts("native wallet birthday prefix semantic tests passed");
  return EXIT_SUCCESS;
}
