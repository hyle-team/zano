// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license; see COPYING.
#include "gtest/gtest.h"
#include "wallet/shared_block_source.h"
#include "currency_core/currency_format_utils.h"
#include <algorithm>
#include <condition_variable>
#include <future>
#include <thread>

namespace
{
  using source = tools::shared_block_source;
  using result = source::result;
  using raw_response = source::response;

  struct block_transport : tools::i_core_proxy
  {
    std::vector<currency::block_complete_entry> blocks;
    std::vector<crypto::hash> hashes;
    std::atomic<size_t> calls{0};
    size_t batch_size = 4;
    bool connected = true;
    std::string status = API_RETURN_CODE_OK;
    std::mutex mutex;
    std::condition_variable cv;
    bool hold = false, entered = false, released = false;

    block_transport(size_t count = 12, uint64_t salt = 1)
    {
      crypto::hash previous{};
      for (size_t height = 0; height < count; ++height)
      {
        currency::block block{};
        block.major_version = BLOCK_MAJOR_VERSION_INITIAL;
        block.minor_version = 0;
        block.timestamp = salt;
        block.prev_id = previous;
        block.miner_tx.version = TRANSACTION_VERSION_PRE_HF4;
        currency::txin_gen input{};
        input.height = height;
        block.miner_tx.vin.push_back(input);
        previous = currency::get_block_hash(block);
        hashes.push_back(previous);
        currency::block_complete_entry entry{};
        entry.block = t_serializable_object_to_blob(block);
        blocks.push_back(entry);
      }
    }
    bool call_COMMAND_RPC_GET_BLOCKS_FAST(
      const currency::COMMAND_RPC_GET_BLOCKS_FAST::request& req, raw_response& res) override
    {
      ++calls;
      if (hold)
      {
        std::unique_lock<std::mutex> lock(mutex);
        entered = true;
        cv.notify_all();
        cv.wait(lock, [&] {return released;});
      }
      get_editable_proxy_diagnostic_info()->last_daemon_is_disconnected = !connected;
      get_editable_proxy_diagnostic_info()->last_success_interract_time = time(nullptr);
      if (!connected) return false;
      res = {};
      res.status = status;
      res.current_height = blocks.size();
      if (status != API_RETURN_CODE_OK) return true;
      size_t start = blocks.size();
      for (const auto& id : req.block_ids)
      {
        const auto found = std::find(hashes.begin(), hashes.end(), id);
        if (found != hashes.end()) {start = std::distance(hashes.begin(), found);break;}
      }
      if (start == blocks.size())
      {
        res.status = API_RETURN_CODE_GENESIS_MISMATCH;
        return true;
      }
      start = std::max<uint64_t>(start, req.minimum_height);
      res.start_height = start;
      for (size_t i = start; i < blocks.size() && i < start + batch_size; ++i)
        res.blocks.push_back(blocks[i]);
      return true;
    }
    void fork_after(size_t anchor, size_t count, uint64_t salt)
    {
      blocks.resize(anchor + 1);hashes.resize(anchor + 1);
      crypto::hash previous=hashes.back();
      for(size_t height=anchor+1;height<count;++height)
      {
        currency::block block{};
        block.major_version=BLOCK_MAJOR_VERSION_INITIAL;
        block.timestamp=salt;block.prev_id=previous;
        block.miner_tx.version=TRANSACTION_VERSION_PRE_HF4;
        currency::txin_gen input{};input.height=height;
        block.miner_tx.vin.push_back(input);
        previous=currency::get_block_hash(block);hashes.push_back(previous);
        currency::block_complete_entry entry{};
        entry.block=t_serializable_object_to_blob(block);blocks.push_back(entry);
      }
    }
    void wait_entered()
    {
      std::unique_lock<std::mutex> lock(mutex);
      ASSERT_TRUE(cv.wait_for(lock, std::chrono::seconds(5), [&]{return entered;}));
    }
    void release()
    {
      std::lock_guard<std::mutex> lock(mutex);released=true;cv.notify_all();
    }
  };

  source::request request(const block_transport& transport, size_t head = 0, size_t minimum = 0)
  {
    source::request req{};
    req.minimum_height = minimum;
    req.block_ids = {transport.hashes[head]};
    if (head) req.block_ids.push_back(transport.hashes[0]);
    req.m_return_compact = true;
    req.m_client_version = "test";
    return req;
  }
  struct harness
  {
    std::shared_ptr<block_transport> transport = std::make_shared<block_transport>();
    source shared{transport};
    std::atomic<bool> stop{false};
    std::shared_ptr<tools::proxy_diagnostic_info> diagnostic =
      std::make_shared<tools::proxy_diagnostic_info>();
    uint64_t epoch = 0;
    result get(uint64_t id, const source::request& req, size_t size, raw_response& res)
    {return shared.get_blocks(id,req,size,stop,res,epoch,diagnostic);}
  };
}

TEST(shared_block_source, aligned_and_different_cursors_use_one_download)
{
  harness h;
  auto alice=h.shared.subscribe(),bob=h.shared.subscribe();
  raw_response a{},b{};
  ASSERT_EQ(result::ready,h.get(alice,request(*h.transport),1,a));
  ASSERT_EQ(result::ready,h.get(bob,request(*h.transport,2),3,b));
  ASSERT_EQ(1,h.transport->calls);
  ASSERT_EQ(2,b.start_height);
  ASSERT_EQ(2,b.blocks.size());
  a.blocks.pop_front();
  ASSERT_EQ(2,b.blocks.size());
  ASSERT_GT(h.shared.retained_bytes(),0);
}
TEST(shared_block_source, different_birthdays_share_the_same_ancestry)
{
  harness h;
  auto alice=h.shared.subscribe(),bob=h.shared.subscribe();
  raw_response a{},b{};
  ASSERT_EQ(result::ready,h.get(alice,request(*h.transport,0,1),1,a));
  ASSERT_EQ(result::ready,h.get(bob,request(*h.transport,0,2),1,b));
  ASSERT_EQ(1,h.transport->calls);
  ASSERT_EQ(2,b.start_height);
}
TEST(shared_block_source, anchor_only_cache_advances_and_old_range_can_be_refetched)
{
  harness h;
  auto alice=h.shared.subscribe(),bob=h.shared.subscribe();
  raw_response a{},b{};
  ASSERT_EQ(result::ready,h.get(alice,request(*h.transport),1,a));
  ASSERT_EQ(result::ready,h.get(alice,request(*h.transport,3),4,a));
  ASSERT_EQ(2,h.transport->calls);
  ASSERT_EQ(3,a.start_height);
  h.shared.finish(alice,false);
  ASSERT_EQ(result::ready,h.get(bob,request(*h.transport),1,b));
  ASSERT_EQ(3,h.transport->calls);
  ASSERT_EQ(0,b.start_height);
}
TEST(shared_block_source, cancelled_and_idle_subscribers_do_not_pin_demand)
{
  harness h;
  auto alice=h.shared.subscribe(),bob=h.shared.subscribe();
  raw_response a{},b{};
  ASSERT_EQ(result::ready,h.get(alice,request(*h.transport),1,a));
  ASSERT_EQ(result::deferred,h.get(bob,request(*h.transport,8),9,b));
  ASSERT_EQ(1,h.transport->calls);
  h.shared.finish(alice,false);
  ASSERT_EQ(result::ready,h.get(bob,request(*h.transport,8),9,b));
  h.shared.unsubscribe(alice);
  h.shared.unsubscribe(bob);
  ASSERT_EQ(0,h.shared.retained_bytes());
  auto calls=h.transport->calls.load();
  ASSERT_EQ(result::deferred,h.get(bob,request(*h.transport),1,b));
  ASSERT_EQ(calls,h.transport->calls);
}
TEST(shared_block_source, oversized_response_reaches_its_owner_without_retention)
{
  auto transport=std::make_shared<block_transport>();
  source shared(transport,1);
  auto alice=shared.subscribe(),bob=shared.subscribe();
  std::atomic<bool> stop{false};
  auto diagnostic=std::make_shared<tools::proxy_diagnostic_info>();
  raw_response a{},b{};uint64_t epoch=0;
  ASSERT_EQ(result::ready,shared.get_blocks(alice,request(*transport),1,stop,a,epoch,diagnostic));
  ASSERT_EQ(4,a.blocks.size());
  ASSERT_EQ(0,shared.retained_bytes());
  ASSERT_EQ(result::deferred,shared.get_blocks(bob,request(*transport,8),9,stop,b,epoch,diagnostic));
  shared.finish(alice,false);
  ASSERT_EQ(result::ready,shared.get_blocks(bob,request(*transport,8),9,stop,b,epoch,diagnostic));
  ASSERT_EQ(8,b.start_height);
  ASSERT_EQ(0,shared.retained_bytes());
}
TEST(shared_block_source, parallel_waiter_reuses_completed_download)
{
  harness h;
  h.transport->hold=true;
  auto alice=h.shared.subscribe(),bob=h.shared.subscribe();
  raw_response a{},b{};
  auto worker=std::async(std::launch::async,[&]{return h.get(alice,request(*h.transport),1,a);});
  h.transport->wait_entered();
  // Avoid sharing the harness's epoch output between concurrent calls.
  uint64_t epoch=0;
  auto r=h.shared.get_blocks(bob,request(*h.transport),1,h.stop,b,epoch,h.diagnostic);
  h.transport->release();
  ASSERT_EQ(result::deferred,r);
  ASSERT_EQ(result::ready,worker.get());
  ASSERT_EQ(result::ready,h.get(bob,request(*h.transport),1,b));
  ASSERT_EQ(1,h.transport->calls);
}
TEST(shared_block_source, endpoint_reset_discards_inflight_result_and_keeps_single_flight)
{
  harness h;h.transport->hold=true;
  auto alice=h.shared.subscribe(),bob=h.shared.subscribe();
  raw_response a{},b{};
  auto worker=std::async(std::launch::async,[&]{return h.get(alice,request(*h.transport),1,a);});
  h.transport->wait_entered();
  auto replacement=std::make_shared<block_transport>(8,2);
  h.shared.reset(replacement);
  uint64_t epoch=0;
  auto r=h.shared.get_blocks(bob,request(*replacement),1,h.stop,b,epoch,h.diagnostic);
  h.transport->release();
  ASSERT_EQ(result::deferred,r);
  ASSERT_EQ(0,replacement->calls);
  ASSERT_EQ(result::deferred,worker.get());
  ASSERT_EQ(0,h.shared.retained_bytes());
  ASSERT_EQ(result::ready,h.get(bob,request(*replacement),1,b));
  ASSERT_EQ(1,replacement->calls);
}
TEST(shared_block_source, formats_and_genesis_are_not_mixed)
{
  harness h;raw_response res{};
  auto alice=h.shared.subscribe(),bob=h.shared.subscribe();
  auto a=request(*h.transport);
  ASSERT_EQ(result::ready,h.get(alice,a,1,res));
  auto b=a;b.m_return_legacy_full=true;
  ASSERT_EQ(result::ready,h.get(bob,b,1,res));
  ASSERT_EQ(2,h.transport->calls);
  h.shared.finish(alice,false);h.shared.finish(bob,false);
  auto other=std::make_shared<block_transport>(8,2);
  ASSERT_EQ(result::ready,h.get(bob,request(*other),1,res));
  ASSERT_EQ(API_RETURN_CODE_GENESIS_MISMATCH,res.status);
}
TEST(shared_block_source, transport_failure_has_shared_backoff_and_no_retention)
{
  harness h;h.transport->connected=false;
  raw_response res{};auto alice=h.shared.subscribe(),bob=h.shared.subscribe();
  ASSERT_EQ(result::disconnected,h.get(alice,request(*h.transport),1,res));
  ASSERT_EQ(result::disconnected,h.get(bob,request(*h.transport),1,res));
  ASSERT_EQ(1,h.transport->calls);
  ASSERT_EQ(0,h.shared.retained_bytes());
}
TEST(shared_block_source, busy_result_is_not_cached_as_blocks)
{
  harness h;h.transport->status=API_RETURN_CODE_BUSY;
  raw_response res{};auto alice=h.shared.subscribe();
  ASSERT_EQ(result::ready,h.get(alice,request(*h.transport),1,res));
  ASSERT_EQ(API_RETURN_CODE_BUSY,res.status);
  ASSERT_EQ(0,h.shared.retained_bytes());
}
TEST(shared_block_source, stale_tip_does_not_hide_same_height_or_shorter_replacement)
{
  harness h;h.transport->batch_size=20;
  auto alice=h.shared.subscribe();
  raw_response res{};
  ASSERT_EQ(result::ready,h.get(alice,request(*h.transport),1,res));
  h.shared.finish(alice,false);
  auto req=request(*h.transport,11);
  req.block_ids.insert(++req.block_ids.begin(),h.transport->hashes[2]);
  h.transport->fork_after(2,12,2);
  std::this_thread::sleep_for(std::chrono::milliseconds(1100));
  ASSERT_EQ(result::ready,h.get(alice,req,12,res));
  ASSERT_EQ(12,res.current_height);
  ASSERT_EQ(2,res.start_height);
  ASSERT_EQ(2,h.transport->calls);
  h.shared.finish(alice,false);
  // A real wallet adopts the first replacement head before its next probe.
  req=request(*h.transport,11);
  req.block_ids.insert(++req.block_ids.begin(),h.transport->hashes[2]);
  h.transport->fork_after(2,7,3);
  std::this_thread::sleep_for(std::chrono::milliseconds(1100));
  ASSERT_EQ(result::ready,h.get(alice,req,12,res));
  ASSERT_EQ(7,res.current_height);
  ASSERT_EQ(2,res.start_height);
  ASSERT_EQ(3,h.transport->calls);
}
TEST(shared_block_source, malformed_or_noncontiguous_batch_is_not_retained)
{
  harness h;
  std::swap(h.transport->blocks[1],h.transport->blocks[2]);
  raw_response res{};auto alice=h.shared.subscribe();
  ASSERT_EQ(result::ready,h.get(alice,request(*h.transport),1,res));
  ASSERT_EQ(0,h.shared.retained_bytes());
}

TEST(shared_block_source, cancellation_during_fetch_does_not_cancel_other_wallet)
{
  harness h;h.transport->hold=true;
  auto alice=h.shared.subscribe(),bob=h.shared.subscribe();
  raw_response a{},b{};
  auto worker=std::async(std::launch::async,[&]{return h.get(alice,request(*h.transport),1,a);});
  h.transport->wait_entered();
  h.shared.unsubscribe(alice);
  h.transport->release();
  ASSERT_EQ(result::deferred,worker.get());
  ASSERT_EQ(result::ready,h.get(bob,request(*h.transport),1,b));
  ASSERT_EQ(2,h.transport->calls);
}

TEST(shared_block_source, production_4000_block_batch_is_shared)
{
  auto transport=std::make_shared<block_transport>(4001);
  transport->batch_size=4000;
  source shared(transport);
  auto alice=shared.subscribe(),bob=shared.subscribe();
  std::atomic<bool> stop{false};
  auto diagnostic=std::make_shared<tools::proxy_diagnostic_info>();
  raw_response a{},b{};uint64_t epoch=0;
  ASSERT_EQ(result::ready,shared.get_blocks(alice,request(*transport),1,stop,a,epoch,diagnostic));
  ASSERT_EQ(result::ready,shared.get_blocks(bob,request(*transport,2000),2001,stop,b,epoch,diagnostic));
  ASSERT_EQ(1,transport->calls);
  ASSERT_EQ(4000,a.blocks.size());
  ASSERT_EQ(2000,b.blocks.size());
  ASSERT_GT(shared.retained_bytes(),0);
  ASSERT_LE(shared.retained_bytes(),32*1024*1024);
}
TEST(shared_block_source, historical_overlap_survives_tip_freshness_interval)
{
  harness h;raw_response a{},b{};
  auto alice=h.shared.subscribe(),bob=h.shared.subscribe();
  ASSERT_EQ(result::ready,h.get(alice,request(*h.transport),1,a));
  std::this_thread::sleep_for(std::chrono::milliseconds(1100));
  ASSERT_EQ(result::ready,h.get(bob,request(*h.transport,2),3,b));
  ASSERT_EQ(1,h.transport->calls);
  ASSERT_EQ(2,b.blocks.size());
}
