// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
//
// Source-only candidate. These tests invoke wallet/block hashing when executed.
// Compilation is not execution evidence; runtime needs the separate core gate.
#include "gtest/gtest.h"
#include "wallet/read_only_wallet_sync.h"
#include "wallet/read_only_core_proxy.h"
#include "wallet/wallet2.h"
#include <functional>
#include <stdexcept>
#include <vector>

namespace
{
  using tools::read_only::block_sync;
  using tools::read_only::step_status;
  using response_type = currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response;

  class fake_proxy final : public tools::i_core_proxy
  {
  public:
    response_type response{};
    size_t requests = 0, forbidden = 0;
    bool received = true, throws = false, consume_response = false;
    std::function<void()> on_request;
    currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request last_request{};
    bool call_COMMAND_RPC_GET_BLOCKS_DIRECT(
      const currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request& request, response_type& out) override
    {
      ++requests; last_request = request;
      if (on_request) on_request();
      if (throws) throw std::runtime_error("private transport diagnostic");
      if (consume_response) out = std::move(response);
      else out = response;
      return received;
    }
    bool call_COMMAND_RPC_SEND_RAW_TX(const currency::COMMAND_RPC_SEND_RAW_TX::request&,
      currency::COMMAND_RPC_SEND_RAW_TX::response&) override { ++forbidden; return true; }
    bool call_COMMAND_RPC_FORCE_RELAY_RAW_TXS(const currency::COMMAND_RPC_FORCE_RELAY_RAW_TXS::request&,
      currency::COMMAND_RPC_FORCE_RELAY_RAW_TXS::response&) override { ++forbidden; return true; }
    bool call_COMMAND_RPC_SUBMITBLOCK(const currency::COMMAND_RPC_SUBMITBLOCK::request&,
      currency::COMMAND_RPC_SUBMITBLOCK::response&) override { ++forbidden; return true; }
    bool call_COMMAND_RPC_INVOKE(const std::string&, const std::string&, int&, std::string&) override
    { ++forbidden; return true; }
  };

  currency::block_direct_data_entry make_block(size_t height, crypto::hash previous)
  {
    auto block = std::make_shared<currency::block_extended_info>();
    block->bl = currency::block{};
    block->bl.major_version = 1;
    block->bl.prev_id = previous;
    block->bl.miner_tx.version = TRANSACTION_VERSION_INITAL;
    block->bl.miner_tx.vin.emplace_back(currency::txin_gen{height});
    // Like the HTTP adapter, coinbase_ptr carries indices, not a copy of miner_tx.
    return {block, std::make_shared<currency::transaction_chain_entry>(),
      std::list<std::shared_ptr<const currency::transaction_chain_entry>>{}};
  }
  std::shared_ptr<currency::block_extended_info> edit_block(currency::block_direct_data_entry& entry)
  {
    auto copy = std::make_shared<currency::block_extended_info>(*entry.block_ptr);
    entry.block_ptr = copy;
    return copy;
  }
  crypto::hash id(const currency::block_direct_data_entry& entry)
  { return currency::get_block_hash(entry.block_ptr->bl); }

  struct scenario
  {
    // Default wallet construction does not generate keys, load files or connect.
    tools::wallet2 wallet;
    std::shared_ptr<fake_proxy> proxy = std::make_shared<fake_proxy>();
    std::atomic<bool> stop{false};
    crypto::hash genesis = wallet.m_chain.get_genesis();
    scenario()
    {
      auto anchor = make_block(1, genesis);
      wallet.m_chain.push_new_block_id(id(anchor), 1);
      // A synthetic block below the explicit scan height exercises the real
      // chain-progress path without pretending to be a funded-wallet fixture.
      wallet.set_minimum_height(100);
      proxy->response.status = API_RETURN_CODE_OK;
      proxy->response.start_height = 1;
      proxy->response.current_height = 3;
      proxy->response.blocks = {anchor, make_block(2, id(anchor))};
    }
    void expect_unchanged(step_status expected)
    {
      const auto chain = wallet.m_chain.get_internal_state_text();
      const auto old_proxy = wallet.get_core_proxy();
      block_sync sync(wallet, genesis, proxy, {8, 16});
      const auto result = sync.step(stop);
      EXPECT_EQ(expected, result.status);
      EXPECT_FALSE(result.state_may_have_changed);
      EXPECT_FALSE(sync.persistence_required());
      EXPECT_FALSE(sync.recovery_required());
      EXPECT_FALSE(wallet.read_only_sync_recovery_required());
      EXPECT_EQ(chain, wallet.m_chain.get_internal_state_text());
      EXPECT_EQ(old_proxy, wallet.get_core_proxy());
      EXPECT_EQ(genesis, wallet.m_chain.get_genesis());
    }
  };
}

TEST(read_only_wallet_sync, applies_one_batch_and_keeps_dirty_state_on_wallet)
{
  scenario s;
  const auto old_proxy = s.wallet.get_core_proxy();
  block_sync sync(s.wallet, s.genesis, s.proxy, {8, 16});
  const auto result = sync.step(s.stop);
  EXPECT_EQ(step_status::Progress, result.status);
  EXPECT_EQ(2u, result.local_blocks_before);
  EXPECT_EQ(3u, result.local_blocks_after);
  EXPECT_EQ(1u, result.blocks_added);
  EXPECT_TRUE(result.state_may_have_changed);
  EXPECT_TRUE(sync.persistence_required());
  EXPECT_FALSE(sync.recovery_required());
  EXPECT_EQ(1u, s.proxy->requests);
  EXPECT_EQ(s.genesis, s.proxy->last_request.block_ids.back());
  EXPECT_EQ(old_proxy, s.wallet.get_core_proxy());
  block_sync later(s.wallet, s.genesis, s.proxy, {8, 16});
  EXPECT_TRUE(later.persistence_required());
  s.stop = true;
  const auto cancelled = later.step(s.stop);
  EXPECT_FALSE(cancelled.state_may_have_changed); // Per step, unlike dirty state.
  EXPECT_TRUE(later.persistence_required());
}

TEST(read_only_wallet_sync, stops_before_request)
{
  scenario s; s.stop = true;
  s.expect_unchanged(step_status::Cancelled);
  EXPECT_EQ(0u, s.proxy->requests);
}
TEST(read_only_wallet_sync, stops_after_request_before_processing)
{
  scenario s; s.proxy->on_request = [&] { s.stop = true; };
  s.expect_unchanged(step_status::Cancelled);
  EXPECT_EQ(1u, s.proxy->requests);
}
TEST(read_only_wallet_sync, transport_failure_and_exception_have_fixed_outcomes)
{
  scenario failed; failed.proxy->received = false;
  failed.expect_unchanged(step_status::ConnectionFailed);
  scenario throws; throws.proxy->throws = true;
  throws.expect_unchanged(step_status::Unavailable);
}
TEST(read_only_wallet_sync, busy_response_does_not_modify_callers_stop_flag)
{
  scenario s; s.proxy->response.status = API_RETURN_CODE_BUSY;
  s.expect_unchanged(step_status::Busy);
  EXPECT_FALSE(s.stop.load());
}
TEST(read_only_wallet_sync, rejects_daemon_genesis_mismatch_without_adoption)
{
  scenario s; s.proxy->response.status = API_RETURN_CODE_GENESIS_MISMATCH;
  s.expect_unchanged(step_status::GenesisMismatch);
}
TEST(read_only_wallet_sync, rejects_foreign_genesis_inside_ok_response)
{
  scenario s;
  s.proxy->response.start_height = 0;
  s.proxy->response.blocks = {make_block(0, currency::null_hash)};
  ASSERT_NE(s.genesis, id(s.proxy->response.blocks.front()));
  s.expect_unchanged(step_status::GenesisMismatch);
}
TEST(read_only_wallet_sync, checks_wallet_binding_again_on_each_step)
{
  scenario s;
  block_sync sync(s.wallet, s.genesis, s.proxy, {8, 16});
  s.wallet.m_chain.set_genesis(id(s.proxy->response.blocks.front()));
  EXPECT_EQ(step_status::GenesisMismatch, sync.step(s.stop).status);
  EXPECT_EQ(0u, s.proxy->requests);
}
TEST(read_only_wallet_sync, requires_explicit_scan_height_and_no_callbacks)
{
  scenario unset; unset.wallet.set_minimum_height(WALLET_MINIMUM_HEIGHT_UNSET_CONST);
  unset.expect_unchanged(step_status::PreparationRequired);
  EXPECT_EQ(0u, unset.proxy->requests);
  scenario callback;
  auto observer = std::make_shared<tools::i_wallet2_callback>();
  callback.wallet.callback(observer);
  callback.expect_unchanged(step_status::UnsupportedCapability);
  scenario enabled; enabled.wallet.set_do_rise_transfer(true);
  enabled.expect_unchanged(step_status::UnsupportedCapability);
}
TEST(read_only_wallet_sync, rejects_null_pointers_and_incomplete_output_indices)
{
  scenario missing; missing.proxy->response.blocks.back().block_ptr.reset();
  missing.expect_unchanged(step_status::InvalidResponse);
  scenario coinbase; coinbase.proxy->response.blocks.back().coinbase_ptr.reset();
  coinbase.expect_unchanged(step_status::InvalidResponse);
  scenario indices;
  edit_block(indices.proxy->response.blocks.back())->bl.miner_tx.vout.emplace_back(currency::tx_out_bare{});
  indices.expect_unchanged(step_status::InvalidResponse);
}
TEST(read_only_wallet_sync, rejects_noncontiguous_height_or_previous_block)
{
  scenario height;
  auto b = edit_block(height.proxy->response.blocks.back());
  boost::get<currency::txin_gen>(b->bl.miner_tx.vin.front()).height = 7;
  height.expect_unchanged(step_status::InvalidResponse);
  scenario previous;
  edit_block(previous.proxy->response.blocks.back())->bl.prev_id = currency::null_hash;
  previous.expect_unchanged(step_status::InvalidResponse);
}
TEST(read_only_wallet_sync, rejects_missing_and_mismatched_transactions_before_any_mutation)
{
  scenario count;
  edit_block(count.proxy->response.blocks.back())->bl.tx_hashes.emplace_back();
  count.expect_unchanged(step_status::InvalidResponse);
  scenario hash;
  auto b = edit_block(hash.proxy->response.blocks.back());
  b->bl.tx_hashes.emplace_back();
  auto tx = std::make_shared<currency::transaction_chain_entry>();
  tx->tx.version = TRANSACTION_VERSION_INITAL;
  hash.proxy->response.blocks.back().txs_ptr.push_back(tx);
  hash.expect_unchanged(step_status::InvalidResponse);
}
TEST(read_only_wallet_sync, checks_reported_bounds_and_hardfork)
{
  scenario zero; zero.proxy->response.current_height = 0;
  zero.expect_unchanged(step_status::InvalidResponse);
  scenario range; range.proxy->response.current_height = 2;
  range.expect_unchanged(step_status::InvalidResponse);
  scenario hardfork; hardfork.proxy->response.current_hardfork = UINT64_MAX;
  hardfork.expect_unchanged(step_status::InvalidResponse);
}
TEST(read_only_wallet_sync, enforces_caller_block_and_transaction_caps)
{
  scenario blocks;
  block_sync a(blocks.wallet, blocks.genesis, blocks.proxy, {1, 16});
  EXPECT_EQ(step_status::PreparationRequired, a.step(blocks.stop).status);
  EXPECT_EQ(0u, blocks.proxy->requests);
  EXPECT_FALSE(a.persistence_required());
  scenario transactions;
  block_sync b(transactions.wallet, transactions.genesis, transactions.proxy, {8, 1});
  EXPECT_EQ(step_status::StepLimitExceeded, b.step(transactions.stop).status);
  EXPECT_FALSE(b.persistence_required());
}
TEST(read_only_wallet_sync, empty_response_is_only_a_height_observation)
{
  scenario s;
  s.proxy->response.blocks.clear();
  s.proxy->response.current_height = 2;
  block_sync sync(s.wallet, s.genesis, s.proxy, {8, 16});
  const auto result = sync.step(s.stop);
  EXPECT_EQ(step_status::NoProgress, result.status);
  EXPECT_TRUE(result.has_daemon_observation);
  EXPECT_EQ(2u, result.reported_daemon_blocks);
  EXPECT_FALSE(result.state_may_have_changed);
  EXPECT_FALSE(sync.persistence_required());
}
TEST(read_only_wallet_sync, preflight_requires_known_anchor)
{
  scenario s; edit_block(s.proxy->response.blocks.front())->bl.nonce = 4;
  s.expect_unchanged(step_status::RecoveryRequired);
}
TEST(read_only_wallet_sync, processing_failure_retains_progress_and_fault_across_recreation)
{
  scenario s;
  s.wallet.set_minimum_height(2);
  auto third = make_block(3, id(s.proxy->response.blocks.back()));
  auto malformed = edit_block(third);
  malformed->bl.miner_tx.extra.emplace_back(crypto::public_key{});
  malformed->bl.miner_tx.extra.emplace_back(crypto::public_key{});
  s.proxy->response.blocks.push_back(third);
  s.proxy->response.current_height = 4;
  block_sync sync(s.wallet, s.genesis, s.proxy, {8, 16});
  const auto result = sync.step(s.stop);
  EXPECT_EQ(step_status::RecoveryRequired, result.status);
  EXPECT_EQ(1u, result.blocks_added);
  EXPECT_EQ(3u, result.local_blocks_after);
  EXPECT_TRUE(result.state_may_have_changed);
  EXPECT_TRUE(sync.recovery_required());
  block_sync recreated(s.wallet, s.genesis, s.proxy, {8, 16});
  EXPECT_TRUE(recreated.recovery_required());
  EXPECT_TRUE(recreated.persistence_required());
  EXPECT_EQ(step_status::RecoveryRequired, recreated.step(s.stop).status);
  EXPECT_EQ(1u, s.proxy->requests);
  EXPECT_TRUE(s.wallet.read_only_sync_recovery_required());
  EXPECT_TRUE(s.wallet.read_only_sync_persistence_required());
  // A scanner can be gone; the wallet still refuses all store overloads.
  auto expect_store_refused = [&](const std::function<void()>& attempt) {
    bool refused = false;
    try { attempt(); }
    catch (const tools::error::wallet_common_error& error)
    {
      // Never emit the exception text. Require the fixed pre-I/O refusal.
      refused = std::string(error.what()).find(
        "read-only synchronization requires recovery") != std::string::npos;
    }
    catch (...) {}
    EXPECT_TRUE(refused);
  };
  expect_store_refused([&] { s.wallet.store(); });
  expect_store_refused([&] { s.wallet.store(std::wstring{}); });
  expect_store_refused([&] { s.wallet.store(std::wstring{}, std::string{}); });
  expect_store_refused([&] { s.wallet.store_watch_only(std::wstring{}, std::string{}); });
}
TEST(read_only_wallet_sync, deep_recovery_cannot_detach_to_genesis_automatically)
{
  scenario s;
  auto genesis = make_block(0, currency::null_hash);
  s.genesis = id(genesis);
  s.wallet.m_chain.clear();
  s.wallet.m_chain.set_genesis(s.genesis);
  s.wallet.m_chain.push_new_block_id(id(make_block(100, s.genesis)), 100);
  s.proxy->response.start_height = 0;
  s.proxy->response.current_height = 102;
  s.proxy->response.blocks = {genesis, make_block(1, s.genesis)};
  const auto before = s.wallet.get_blockchain_current_size();
  block_sync sync(s.wallet, s.genesis, s.proxy, {8, 16});
  EXPECT_EQ(step_status::RecoveryRequired, sync.step(s.stop).status);
  EXPECT_EQ(before, s.wallet.get_blockchain_current_size());
  EXPECT_EQ(s.genesis, s.wallet.m_chain.get_genesis());
  EXPECT_TRUE(sync.recovery_required());
}
TEST(read_only_wallet_sync, denies_whitelist_and_reentrant_steps_during_fetch)
{
  scenario s; s.wallet.set_use_assets_whitelisting(false);
  block_sync sync(s.wallet, s.genesis, s.proxy, {8, 16});
  s.proxy->on_request = [&] {
    EXPECT_FALSE(s.wallet.load_whitelisted_tokens());
    EXPECT_EQ(step_status::Busy, sync.step(s.stop).status);
    block_sync another(s.wallet, s.genesis, s.proxy, {8, 16});
    EXPECT_EQ(step_status::Busy, another.step(s.stop).status);
  };
  EXPECT_EQ(step_status::Progress, sync.step(s.stop).status);
  EXPECT_TRUE(s.wallet.load_whitelisted_tokens()); // Disabled whitelist, no HTTP.
}
TEST(read_only_wallet_sync, proxy_never_delegates_spend_mining_or_generic_methods)
{
  auto delegate = std::make_shared<fake_proxy>();
  tools::read_only::core_proxy proxy(delegate);
  currency::COMMAND_RPC_SEND_RAW_TX::request send{};
  currency::COMMAND_RPC_SEND_RAW_TX::response sent{};
  currency::COMMAND_RPC_FORCE_RELAY_RAW_TXS::request relay{};
  currency::COMMAND_RPC_FORCE_RELAY_RAW_TXS::response relayed{};
  currency::COMMAND_RPC_SUBMITBLOCK::request block{};
  currency::COMMAND_RPC_SUBMITBLOCK::response submitted{};
  int status = 0; std::string body;
  EXPECT_FALSE(proxy.call_COMMAND_RPC_SEND_RAW_TX(send, sent));
  EXPECT_FALSE(proxy.call_COMMAND_RPC_FORCE_RELAY_RAW_TXS(relay, relayed));
  EXPECT_FALSE(proxy.call_COMMAND_RPC_SUBMITBLOCK(block, submitted));
  EXPECT_FALSE(proxy.call_COMMAND_RPC_INVOKE("/sendrawtransaction", "inert", status, body));
  EXPECT_TRUE(proxy.denied());
  EXPECT_EQ(0u, delegate->forbidden);
  proxy.begin_step();
  currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request request{};
  response_type response{};
  EXPECT_TRUE(proxy.call_COMMAND_RPC_GET_BLOCKS_DIRECT(request, response));
  EXPECT_FALSE(proxy.call_COMMAND_RPC_GET_BLOCKS_DIRECT(request, response));
  EXPECT_TRUE(proxy.denied());
  EXPECT_EQ(1u, delegate->requests);
}

namespace
{
  std::vector<currency::block_direct_data_entry> extend_wallet(scenario& s, size_t top)
  {
    std::vector<currency::block_direct_data_entry> history(1);
    history.push_back(s.proxy->response.blocks.front());
    for (size_t height = 2; height <= top; ++height)
    {
      history.push_back(make_block(height, id(history.back())));
      s.wallet.m_chain.push_new_block_id(id(history.back()), height);
    }
    return history;
  }
}

TEST(read_only_wallet_sync, progresses_through_a_protocol_batch_with_a_small_step)
{
  scenario s;
  while (s.proxy->response.blocks.size() < COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT)
  {
    const auto height = s.proxy->response.blocks.size() + 1;
    s.proxy->response.blocks.push_back(make_block(height, id(s.proxy->response.blocks.back())));
  }
  s.proxy->response.current_height = COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT + 1;
  block_sync sync(s.wallet, s.genesis, s.proxy, {64, 64});
  const auto result = sync.step(s.stop);
  EXPECT_EQ(step_status::Progress, result.status);
  EXPECT_EQ(63u, result.blocks_added); // The inclusive anchor consumes one slot.
  EXPECT_EQ(65u, result.local_blocks_after);
  EXPECT_TRUE(result.batch_truncated);
  EXPECT_EQ(COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT + 1u, result.reported_daemon_blocks);
  EXPECT_TRUE(result.has_daemon_observation);
  EXPECT_FALSE(sync.recovery_required());
}
TEST(read_only_wallet_sync, transaction_budget_selects_whole_blocks_and_ignores_unselected_tail)
{
  scenario s;
  auto tail = make_block(3, id(s.proxy->response.blocks.back()));
  tail.block_ptr.reset(); // Deliberately outside the selected prefix.
  s.proxy->response.blocks.push_back(tail);
  s.proxy->response.current_height = 4;
  block_sync sync(s.wallet, s.genesis, s.proxy, {8, 2});
  const auto result = sync.step(s.stop);
  EXPECT_EQ(step_status::Progress, result.status);
  EXPECT_EQ(1u, result.blocks_added);
  EXPECT_TRUE(result.batch_truncated);
  EXPECT_EQ(4u, result.reported_daemon_blocks);
  EXPECT_FALSE(sync.recovery_required());
}
TEST(read_only_wallet_sync, rejects_protocol_overflow_and_an_unfittable_first_candidate_cleanly)
{
  scenario overflow;
  overflow.proxy->response.blocks.resize(COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT + 1);
  overflow.proxy->response.current_height = COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT + 2;
  overflow.expect_unchanged(step_status::InvalidResponse);
  scenario large;
  large.proxy->response.blocks.back().txs_ptr.resize(16);
  block_sync sync(large.wallet, large.genesis, large.proxy, {8, 16});
  const auto before = large.wallet.m_chain.get_internal_state_text();
  const auto result = sync.step(large.stop);
  EXPECT_EQ(step_status::StepLimitExceeded, result.status);
  EXPECT_FALSE(result.state_may_have_changed);
  EXPECT_FALSE(sync.persistence_required());
  EXPECT_FALSE(sync.recovery_required());
  EXPECT_EQ(before, large.wallet.m_chain.get_internal_state_text());
}
TEST(read_only_wallet_sync, birthday_jump_requires_explicit_preparation)
{
  scenario s;
  s.proxy->response.start_height = 100;
  s.proxy->response.current_height = 101;
  s.proxy->response.blocks = {make_block(100, s.genesis)};
  s.expect_unchanged(step_status::PreparationRequired);
}
TEST(read_only_wallet_sync, behind_node_cannot_roll_back_sparse_wallet_history)
{
  scenario s;
  const auto history = extend_wallet(s, 100);
  s.proxy->response.start_height = 40;
  s.proxy->response.current_height = 50;
  s.proxy->response.blocks = {history[40], history[41]};
  s.wallet.set_concise_mode(false);
  const auto before = s.wallet.m_chain.get_internal_state_text();
  block_sync sync(s.wallet, s.genesis, s.proxy, {8, 16});
  const auto result = sync.step(s.stop);
  EXPECT_EQ(step_status::NodeBehind, result.status);
  EXPECT_EQ(50u, result.reported_daemon_blocks);
  EXPECT_TRUE(result.has_daemon_observation); // Unverified node report only.
  EXPECT_FALSE(result.state_may_have_changed);
  EXPECT_FALSE(sync.persistence_required());
  EXPECT_FALSE(sync.recovery_required());
  EXPECT_EQ(101u, s.wallet.get_blockchain_current_size());
  EXPECT_EQ(before, s.wallet.m_chain.get_internal_state_text());
}
TEST(read_only_wallet_sync, sparse_reorg_is_bounded_without_concise_mode)
{
  scenario s;
  const auto history = extend_wallet(s, 100);
  auto fork = history[11];
  edit_block(fork)->bl.nonce = 1;
  s.proxy->response.start_height = 10;
  s.proxy->response.current_height = 102;
  s.proxy->response.blocks = {history[10], fork};
  s.wallet.set_concise_mode(false);
  s.wallet.set_concise_mode_reorg_max_reorg_blocks(UINT64_MAX);
  const auto before = s.wallet.m_chain.get_internal_state_text();
  block_sync sync(s.wallet, s.genesis, s.proxy, {8, 16, 20});
  const auto result = sync.step(s.stop);
  EXPECT_EQ(step_status::RecoveryRequired, result.status);
  EXPECT_FALSE(result.state_may_have_changed);
  EXPECT_FALSE(sync.recovery_required()); // Clean preflight refusal.
  EXPECT_FALSE(sync.persistence_required());
  EXPECT_EQ(before, s.wallet.m_chain.get_internal_state_text());
}
TEST(read_only_wallet_sync, accepts_shallow_reorg_at_exact_bound)
{
  scenario s;
  const auto history = extend_wallet(s, 10);
  auto fork = history[9];
  edit_block(fork)->bl.nonce = 1;
  s.proxy->response.start_height = 8;
  s.proxy->response.current_height = 12;
  s.proxy->response.blocks = {history[8], fork};
  block_sync sync(s.wallet, s.genesis, s.proxy, {8, 16, 2});
  const auto result = sync.step(s.stop);
  EXPECT_EQ(step_status::Progress, result.status);
  EXPECT_EQ(1u, result.blocks_added);
  EXPECT_EQ(10u, result.local_blocks_after);
  EXPECT_FALSE(sync.recovery_required());
}
TEST(read_only_wallet_sync, cancellation_after_genesis_processing_keeps_dirty_observation)
{
  scenario s;
  auto genesis = make_block(0, currency::null_hash);
  crypto::public_key public_point{};
  ASSERT_TRUE(epee::string_tools::hex_to_pod(
    std::string("5866666666666666666666666666666666666666666666666666666666666666"),
    public_point)); // Public Edwards base point, not wallet material.
  edit_block(genesis)->bl.miner_tx.extra.emplace_back(public_point);
  s.genesis = id(genesis);
  s.wallet.m_chain.clear();
  s.wallet.m_chain.set_genesis(s.genesis);
  s.wallet.set_minimum_height(0);
  // The handler drops its genesis entry after processing it. Its sole indices
  // owner provides a deterministic stop at that boundary, without a new runtime
  // callback/test hook or concurrent wallet access.
  const auto stop_after_genesis = std::make_shared<std::atomic<bool>>(false);
  genesis.coinbase_ptr = std::shared_ptr<const currency::transaction_chain_entry>(
    new currency::transaction_chain_entry{},
    [stop_after_genesis](const currency::transaction_chain_entry* entry) {
      delete entry; *stop_after_genesis = true;
    });
  s.proxy->consume_response = true;
  s.proxy->response.blocks.clear();
  s.proxy->response.blocks.push_back(std::move(genesis));
  s.proxy->response.blocks.push_back(make_block(1, s.genesis));
  s.proxy->response.start_height = 0;
  s.proxy->response.current_height = 2;
  block_sync sync(s.wallet, s.genesis, s.proxy, {8, 16});
  const auto result = sync.step(*stop_after_genesis);
  EXPECT_EQ(step_status::Cancelled, result.status);
  EXPECT_TRUE(stop_after_genesis->load());
  EXPECT_TRUE(result.state_may_have_changed);
  EXPECT_TRUE(s.wallet.read_only_sync_persistence_required());
  EXPECT_FALSE(s.wallet.read_only_sync_recovery_required());
  EXPECT_EQ(1u, result.local_blocks_after);
  EXPECT_EQ(0u, result.blocks_added);
}
TEST(read_only_wallet_sync, clean_no_progress_does_not_erase_prior_dirty_state)
{
  scenario s;
  block_sync sync(s.wallet, s.genesis, s.proxy, {8, 16});
  ASSERT_EQ(step_status::Progress, sync.step(s.stop).status);
  s.proxy->response.blocks.clear();
  s.proxy->response.start_height = 2;
  const auto result = sync.step(s.stop);
  EXPECT_EQ(step_status::NoProgress, result.status);
  EXPECT_FALSE(result.state_may_have_changed);
  EXPECT_TRUE(sync.persistence_required());
  EXPECT_FALSE(sync.recovery_required());
}
TEST(read_only_wallet_sync, null_transport_zero_genesis_and_unbounded_reorg_require_preparation)
{
  scenario s;
  block_sync missing(s.wallet, s.genesis, {}, {8, 16});
  EXPECT_EQ(step_status::PreparationRequired, missing.step(s.stop).status);
  block_sync zero(s.wallet, currency::null_hash, s.proxy, {8, 16});
  EXPECT_EQ(step_status::PreparationRequired, zero.step(s.stop).status);
  block_sync unbounded(s.wallet, s.genesis, s.proxy, {8, 16, UINT64_MAX});
  EXPECT_EQ(step_status::PreparationRequired, unbounded.step(s.stop).status);
  EXPECT_EQ(0u, s.proxy->requests);
}

namespace
{
  // Implements only the stop-aware entry; the legacy entry must never be used.
  class stop_aware_proxy final : public tools::i_core_proxy
  {
  public:
    response_type response{};
    const std::atomic<bool>* seen = nullptr;
    size_t stop_aware = 0, legacy = 0;
    std::function<void()> on_request;
    bool call_COMMAND_RPC_GET_BLOCKS_DIRECT(
      const currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request&, response_type&) override
    { ++legacy; return false; }
    bool call_COMMAND_RPC_GET_BLOCKS_DIRECT(const currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request&,
      response_type& out, const std::atomic<bool>& stop) override
    {
      ++stop_aware; seen = &stop;
      if (on_request) on_request();
      out = response;
      return true;
    }
  };
}

TEST(read_only_wallet_sync, forwards_the_owner_stop_token_to_a_stop_aware_transport)
{
  scenario s;
  auto transport = std::make_shared<stop_aware_proxy>();
  transport->response = s.proxy->response;
  block_sync sync(s.wallet, s.genesis, transport, {8, 16});
  EXPECT_EQ(step_status::Progress, sync.step(s.stop).status);
  EXPECT_EQ(&s.stop, transport->seen); // The same borrowed token, not a copy.
  EXPECT_EQ(1u, transport->stop_aware);
  EXPECT_EQ(0u, transport->legacy);
  EXPECT_FALSE(s.stop.load());
}

TEST(read_only_wallet_sync, stop_during_stop_aware_fetch_leaves_wallet_unchanged)
{
  scenario s;
  auto transport = std::make_shared<stop_aware_proxy>();
  transport->response = s.proxy->response;
  transport->on_request = [&] { s.stop = true; };
  const auto chain = s.wallet.m_chain.get_internal_state_text();
  block_sync sync(s.wallet, s.genesis, transport, {8, 16});
  const auto result = sync.step(s.stop);
  EXPECT_EQ(step_status::Cancelled, result.status);
  EXPECT_FALSE(result.state_may_have_changed);
  EXPECT_EQ(chain, s.wallet.m_chain.get_internal_state_text());
  EXPECT_TRUE(s.stop.load()); // Never cleared by the scanner or proxy.
  EXPECT_EQ(1u, transport->stop_aware);
}

TEST(read_only_wallet_sync, legacy_transport_keeps_working_through_the_compatibility_default)
{
  // fake_proxy overrides only the original entry: the base default samples
  // the token and delegates, so no old transport needs changing.
  auto delegate = std::make_shared<fake_proxy>();
  tools::read_only::core_proxy proxy(delegate);
  currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request request{};
  response_type response{};
  std::atomic<bool> stop{true};
  proxy.begin_step();
  EXPECT_FALSE(proxy.call_COMMAND_RPC_GET_BLOCKS_DIRECT(request, response, stop));
  EXPECT_EQ(0u, delegate->requests); // Stop sampled before the legacy call.
  EXPECT_FALSE(proxy.call_COMMAND_RPC_GET_BLOCKS_DIRECT(request, response));
  EXPECT_TRUE(proxy.denied()); // One request per step across both entries.
  proxy.begin_step();
  stop = false;
  EXPECT_TRUE(proxy.call_COMMAND_RPC_GET_BLOCKS_DIRECT(request, response, stop));
  EXPECT_EQ(1u, delegate->requests);
  EXPECT_FALSE(proxy.call_COMMAND_RPC_GET_BLOCKS_DIRECT(request, response, stop));
  EXPECT_EQ(1u, delegate->requests);
  EXPECT_EQ(0u, delegate->forbidden);
}

TEST(read_only_wallet_sync, explicit_zero_minimum_can_scan_from_genesis)
{
  scenario s;
  auto genesis = make_block(0, currency::null_hash);
  crypto::public_key public_point{};
  ASSERT_TRUE(epee::string_tools::hex_to_pod(
    std::string("5866666666666666666666666666666666666666666666666666666666666666"),
    public_point));
  edit_block(genesis)->bl.miner_tx.extra.emplace_back(public_point);
  s.genesis = id(genesis);
  auto first = make_block(1, s.genesis);
  edit_block(first)->bl.miner_tx.extra.emplace_back(public_point);
  s.wallet.m_chain.clear();
  s.wallet.m_chain.set_genesis(s.genesis);
  s.wallet.set_minimum_height(0);
  s.proxy->response.start_height = 0;
  s.proxy->response.current_height = 2;
  s.proxy->response.blocks = {genesis, first};
  block_sync sync(s.wallet, s.genesis, s.proxy, {8, 16});
  const auto result = sync.step(s.stop);
  EXPECT_EQ(step_status::Progress, result.status);
  EXPECT_EQ(2u, result.local_blocks_after);
  EXPECT_EQ(1u, result.blocks_added);
  EXPECT_FALSE(sync.recovery_required());
  EXPECT_EQ(0u, s.proxy->last_request.minimum_height);
}
