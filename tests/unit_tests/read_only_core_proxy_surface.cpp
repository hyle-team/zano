// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
// Source-only candidate. Compilation is not execution evidence.
// Every delegate method is inert. No wallet, cryptographic or network call is made.
#include "gtest/gtest.h"
#include "wallet/read_only_core_proxy.h"
#include <memory>

namespace
{
  class counting_delegate final : public tools::i_core_proxy
  {
  public:
    size_t calls = 0;
    bool accept_blocks = true;
    bool set_connection_addr(const std::string&) override { ++calls; return true; }
    bool call_COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES(const currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request&, currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_GET_BLOCKS_FAST(const currency::COMMAND_RPC_GET_BLOCKS_FAST::request&, currency::COMMAND_RPC_GET_BLOCKS_FAST::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_GET_BLOCKS_DIRECT(const currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request&, currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response&) override { ++calls; return accept_blocks; }
    bool call_COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE(const currency::COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::request&, currency::COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_GET_INFO(const currency::COMMAND_RPC_GET_INFO::request&, currency::COMMAND_RPC_GET_INFO::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_GET_TX_POOL(const currency::COMMAND_RPC_GET_TX_POOL::request&, currency::COMMAND_RPC_GET_TX_POOL::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_GET_ALIASES_BY_ADDRESS(const currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::request&, currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request&, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::request&, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request&, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_SEND_RAW_TX(const currency::COMMAND_RPC_SEND_RAW_TX::request&, currency::COMMAND_RPC_SEND_RAW_TX::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_FORCE_RELAY_RAW_TXS(const currency::COMMAND_RPC_FORCE_RELAY_RAW_TXS::request&, currency::COMMAND_RPC_FORCE_RELAY_RAW_TXS::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_GET_ALL_ALIASES(currency::COMMAND_RPC_GET_ALL_ALIASES::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_GET_ALIAS_DETAILS(const currency::COMMAND_RPC_GET_ALIAS_DETAILS::request&, currency::COMMAND_RPC_GET_ALIAS_DETAILS::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_GET_ALIAS_REWARD(const currency::COMMAND_RPC_GET_ALIAS_REWARD::request&, currency::COMMAND_RPC_GET_ALIAS_REWARD::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_ALIAS_LOOKUP(const currency::COMMAND_RPC_ALIAS_LOOKUP::request&, currency::COMMAND_RPC_ALIAS_LOOKUP::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_GET_TRANSACTIONS(const currency::COMMAND_RPC_GET_TRANSACTIONS::request&, currency::COMMAND_RPC_GET_TRANSACTIONS::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_COMMAND_RPC_CHECK_KEYIMAGES(const currency::COMMAND_RPC_CHECK_KEYIMAGES::request&, currency::COMMAND_RPC_CHECK_KEYIMAGES::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_GETBLOCKTEMPLATE(const currency::COMMAND_RPC_GETBLOCKTEMPLATE::request&, currency::COMMAND_RPC_GETBLOCKTEMPLATE::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_SUBMITBLOCK(const currency::COMMAND_RPC_SUBMITBLOCK::request&, currency::COMMAND_RPC_SUBMITBLOCK::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_SUBMITBLOCK2(const currency::COMMAND_RPC_SUBMITBLOCK2::request&, currency::COMMAND_RPC_SUBMITBLOCK2::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_GET_POS_MINING_DETAILS(const currency::COMMAND_RPC_GET_POS_MINING_DETAILS::request&, currency::COMMAND_RPC_GET_POS_MINING_DETAILS::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_GET_BLOCKS_DETAILS(const currency::COMMAND_RPC_GET_BLOCKS_DETAILS::request&, currency::COMMAND_RPC_GET_BLOCKS_DETAILS::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_GET_OFFERS_EX(const currency::COMMAND_RPC_GET_OFFERS_EX::request&, currency::COMMAND_RPC_GET_OFFERS_EX::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN(const currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::request&, currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_GET_POOL_INFO(const currency::COMMAND_RPC_GET_POOL_INFO::request&, currency::COMMAND_RPC_GET_POOL_INFO::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_GET_ASSET_INFO(const currency::COMMAND_RPC_GET_ASSET_INFO::request&, currency::COMMAND_RPC_GET_ASSET_INFO::response&) override { ++calls; return true; }
    bool call_COMMAND_RPC_INVOKE(const std::string&, const std::string&, int&, std::string&) override { ++calls; return true; }
    bool check_connection() override { ++calls; return true; }
    time_t get_last_success_interract_time() override { ++calls; return 7; }
    uint64_t get_download_speed() override { ++calls; return 7; }
    bool get_transfer_address(const std::string&, currency::account_public_address&, std::string&) override { ++calls; return true; }
    bool get_transfer_address(const std::string&, currency::address_v&, std::string&) override { ++calls; return true; }
    void set_connectivity(unsigned int, size_t) override { ++calls;  }
    bool is_daemon_inbox() override { ++calls; return true; }
  };
}

TEST(read_only_core_proxy_surface, every_denied_signature_is_not_delegated)
{
  auto delegate = std::make_shared<counting_delegate>();
  tools::read_only::core_proxy proxy(delegate);
  {
    SCOPED_TRACE("set_connection_addr(const std::string&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const std::string&);
    const method_type method = &tools::i_core_proxy::set_connection_addr;
    std::string url{};
    EXPECT_FALSE((proxy.*method)(url));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES(const currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request&, currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request&, currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES;
    currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request rqt{};
    currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response rsp{};
    EXPECT_FALSE((proxy.*method)(rqt, rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_GET_BLOCKS_FAST(const currency::COMMAND_RPC_GET_BLOCKS_FAST::request&, currency::COMMAND_RPC_GET_BLOCKS_FAST::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_GET_BLOCKS_FAST::request&, currency::COMMAND_RPC_GET_BLOCKS_FAST::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GET_BLOCKS_FAST;
    currency::COMMAND_RPC_GET_BLOCKS_FAST::request rqt{};
    currency::COMMAND_RPC_GET_BLOCKS_FAST::response rsp{};
    EXPECT_FALSE((proxy.*method)(rqt, rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE(const currency::COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::request&, currency::COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::request&, currency::COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE;
    currency::COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::request rqt{};
    currency::COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::response rsp{};
    EXPECT_FALSE((proxy.*method)(rqt, rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_GET_INFO(const currency::COMMAND_RPC_GET_INFO::request&, currency::COMMAND_RPC_GET_INFO::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_GET_INFO::request&, currency::COMMAND_RPC_GET_INFO::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GET_INFO;
    currency::COMMAND_RPC_GET_INFO::request rqt{};
    currency::COMMAND_RPC_GET_INFO::response rsp{};
    EXPECT_FALSE((proxy.*method)(rqt, rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_GET_TX_POOL(const currency::COMMAND_RPC_GET_TX_POOL::request&, currency::COMMAND_RPC_GET_TX_POOL::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_GET_TX_POOL::request&, currency::COMMAND_RPC_GET_TX_POOL::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GET_TX_POOL;
    currency::COMMAND_RPC_GET_TX_POOL::request rqt{};
    currency::COMMAND_RPC_GET_TX_POOL::response rsp{};
    EXPECT_FALSE((proxy.*method)(rqt, rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_GET_ALIASES_BY_ADDRESS(const currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::request&, currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::request&, currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GET_ALIASES_BY_ADDRESS;
    currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::request rqt{};
    currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::response rsp{};
    EXPECT_FALSE((proxy.*method)(rqt, rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request&, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request&, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS;
    currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request rqt{};
    currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response rsp{};
    EXPECT_FALSE((proxy.*method)(rqt, rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::request&, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::request&, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3;
    currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::request rqt{};
    currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::response rsp{};
    EXPECT_FALSE((proxy.*method)(rqt, rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request&, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request&, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4;
    currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request rqt{};
    currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response rsp{};
    EXPECT_FALSE((proxy.*method)(rqt, rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_SEND_RAW_TX(const currency::COMMAND_RPC_SEND_RAW_TX::request&, currency::COMMAND_RPC_SEND_RAW_TX::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_SEND_RAW_TX::request&, currency::COMMAND_RPC_SEND_RAW_TX::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_SEND_RAW_TX;
    currency::COMMAND_RPC_SEND_RAW_TX::request rqt{};
    currency::COMMAND_RPC_SEND_RAW_TX::response rsp{};
    EXPECT_FALSE((proxy.*method)(rqt, rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_FORCE_RELAY_RAW_TXS(const currency::COMMAND_RPC_FORCE_RELAY_RAW_TXS::request&, currency::COMMAND_RPC_FORCE_RELAY_RAW_TXS::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_FORCE_RELAY_RAW_TXS::request&, currency::COMMAND_RPC_FORCE_RELAY_RAW_TXS::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_FORCE_RELAY_RAW_TXS;
    currency::COMMAND_RPC_FORCE_RELAY_RAW_TXS::request rqt{};
    currency::COMMAND_RPC_FORCE_RELAY_RAW_TXS::response rsp{};
    EXPECT_FALSE((proxy.*method)(rqt, rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_GET_ALL_ALIASES(currency::COMMAND_RPC_GET_ALL_ALIASES::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(currency::COMMAND_RPC_GET_ALL_ALIASES::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GET_ALL_ALIASES;
    currency::COMMAND_RPC_GET_ALL_ALIASES::response rsp{};
    EXPECT_FALSE((proxy.*method)(rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_GET_ALIAS_DETAILS(const currency::COMMAND_RPC_GET_ALIAS_DETAILS::request&, currency::COMMAND_RPC_GET_ALIAS_DETAILS::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_GET_ALIAS_DETAILS::request&, currency::COMMAND_RPC_GET_ALIAS_DETAILS::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GET_ALIAS_DETAILS;
    currency::COMMAND_RPC_GET_ALIAS_DETAILS::request req{};
    currency::COMMAND_RPC_GET_ALIAS_DETAILS::response rsp{};
    EXPECT_FALSE((proxy.*method)(req, rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_GET_ALIAS_REWARD(const currency::COMMAND_RPC_GET_ALIAS_REWARD::request&, currency::COMMAND_RPC_GET_ALIAS_REWARD::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_GET_ALIAS_REWARD::request&, currency::COMMAND_RPC_GET_ALIAS_REWARD::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GET_ALIAS_REWARD;
    currency::COMMAND_RPC_GET_ALIAS_REWARD::request req{};
    currency::COMMAND_RPC_GET_ALIAS_REWARD::response rsp{};
    EXPECT_FALSE((proxy.*method)(req, rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_ALIAS_LOOKUP(const currency::COMMAND_RPC_ALIAS_LOOKUP::request&, currency::COMMAND_RPC_ALIAS_LOOKUP::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_ALIAS_LOOKUP::request&, currency::COMMAND_RPC_ALIAS_LOOKUP::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_ALIAS_LOOKUP;
    currency::COMMAND_RPC_ALIAS_LOOKUP::request req{};
    currency::COMMAND_RPC_ALIAS_LOOKUP::response rsp{};
    EXPECT_FALSE((proxy.*method)(req, rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_GET_TRANSACTIONS(const currency::COMMAND_RPC_GET_TRANSACTIONS::request&, currency::COMMAND_RPC_GET_TRANSACTIONS::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_GET_TRANSACTIONS::request&, currency::COMMAND_RPC_GET_TRANSACTIONS::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GET_TRANSACTIONS;
    currency::COMMAND_RPC_GET_TRANSACTIONS::request req{};
    currency::COMMAND_RPC_GET_TRANSACTIONS::response rsp{};
    EXPECT_FALSE((proxy.*method)(req, rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_COMMAND_RPC_CHECK_KEYIMAGES(const currency::COMMAND_RPC_CHECK_KEYIMAGES::request&, currency::COMMAND_RPC_CHECK_KEYIMAGES::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_CHECK_KEYIMAGES::request&, currency::COMMAND_RPC_CHECK_KEYIMAGES::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_COMMAND_RPC_CHECK_KEYIMAGES;
    currency::COMMAND_RPC_CHECK_KEYIMAGES::request req{};
    currency::COMMAND_RPC_CHECK_KEYIMAGES::response rsp{};
    EXPECT_FALSE((proxy.*method)(req, rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_GETBLOCKTEMPLATE(const currency::COMMAND_RPC_GETBLOCKTEMPLATE::request&, currency::COMMAND_RPC_GETBLOCKTEMPLATE::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_GETBLOCKTEMPLATE::request&, currency::COMMAND_RPC_GETBLOCKTEMPLATE::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GETBLOCKTEMPLATE;
    currency::COMMAND_RPC_GETBLOCKTEMPLATE::request req{};
    currency::COMMAND_RPC_GETBLOCKTEMPLATE::response rsp{};
    EXPECT_FALSE((proxy.*method)(req, rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_SUBMITBLOCK(const currency::COMMAND_RPC_SUBMITBLOCK::request&, currency::COMMAND_RPC_SUBMITBLOCK::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_SUBMITBLOCK::request&, currency::COMMAND_RPC_SUBMITBLOCK::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_SUBMITBLOCK;
    currency::COMMAND_RPC_SUBMITBLOCK::request req{};
    currency::COMMAND_RPC_SUBMITBLOCK::response rsp{};
    EXPECT_FALSE((proxy.*method)(req, rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_SUBMITBLOCK2(const currency::COMMAND_RPC_SUBMITBLOCK2::request&, currency::COMMAND_RPC_SUBMITBLOCK2::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_SUBMITBLOCK2::request&, currency::COMMAND_RPC_SUBMITBLOCK2::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_SUBMITBLOCK2;
    currency::COMMAND_RPC_SUBMITBLOCK2::request req{};
    currency::COMMAND_RPC_SUBMITBLOCK2::response rsp{};
    EXPECT_FALSE((proxy.*method)(req, rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_GET_POS_MINING_DETAILS(const currency::COMMAND_RPC_GET_POS_MINING_DETAILS::request&, currency::COMMAND_RPC_GET_POS_MINING_DETAILS::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_GET_POS_MINING_DETAILS::request&, currency::COMMAND_RPC_GET_POS_MINING_DETAILS::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GET_POS_MINING_DETAILS;
    currency::COMMAND_RPC_GET_POS_MINING_DETAILS::request req{};
    currency::COMMAND_RPC_GET_POS_MINING_DETAILS::response rsp{};
    EXPECT_FALSE((proxy.*method)(req, rsp));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_GET_BLOCKS_DETAILS(const currency::COMMAND_RPC_GET_BLOCKS_DETAILS::request&, currency::COMMAND_RPC_GET_BLOCKS_DETAILS::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_GET_BLOCKS_DETAILS::request&, currency::COMMAND_RPC_GET_BLOCKS_DETAILS::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GET_BLOCKS_DETAILS;
    currency::COMMAND_RPC_GET_BLOCKS_DETAILS::request req{};
    currency::COMMAND_RPC_GET_BLOCKS_DETAILS::response res{};
    EXPECT_FALSE((proxy.*method)(req, res));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_GET_OFFERS_EX(const currency::COMMAND_RPC_GET_OFFERS_EX::request&, currency::COMMAND_RPC_GET_OFFERS_EX::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_GET_OFFERS_EX::request&, currency::COMMAND_RPC_GET_OFFERS_EX::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GET_OFFERS_EX;
    currency::COMMAND_RPC_GET_OFFERS_EX::request req{};
    currency::COMMAND_RPC_GET_OFFERS_EX::response res{};
    EXPECT_FALSE((proxy.*method)(req, res));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN(const currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::request&, currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::request&, currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN;
    currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::request req{};
    currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::response res{};
    EXPECT_FALSE((proxy.*method)(req, res));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_GET_POOL_INFO(const currency::COMMAND_RPC_GET_POOL_INFO::request&, currency::COMMAND_RPC_GET_POOL_INFO::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_GET_POOL_INFO::request&, currency::COMMAND_RPC_GET_POOL_INFO::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GET_POOL_INFO;
    currency::COMMAND_RPC_GET_POOL_INFO::request req{};
    currency::COMMAND_RPC_GET_POOL_INFO::response res{};
    EXPECT_FALSE((proxy.*method)(req, res));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_GET_ASSET_INFO(const currency::COMMAND_RPC_GET_ASSET_INFO::request&, currency::COMMAND_RPC_GET_ASSET_INFO::response&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_GET_ASSET_INFO::request&, currency::COMMAND_RPC_GET_ASSET_INFO::response&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GET_ASSET_INFO;
    currency::COMMAND_RPC_GET_ASSET_INFO::request req{};
    currency::COMMAND_RPC_GET_ASSET_INFO::response res{};
    EXPECT_FALSE((proxy.*method)(req, res));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("call_COMMAND_RPC_INVOKE(const std::string&, const std::string&, int&, std::string&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const std::string&, const std::string&, int&, std::string&);
    const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_INVOKE;
    std::string uri{};
    std::string body{};
    int response_code{};
    std::string response_body{};
    EXPECT_FALSE((proxy.*method)(uri, body, response_code, response_body));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("check_connection()");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)();
    const method_type method = &tools::i_core_proxy::check_connection;
    EXPECT_FALSE((proxy.*method)());
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("get_last_success_interract_time()");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = time_t (tools::i_core_proxy::*)();
    const method_type method = &tools::i_core_proxy::get_last_success_interract_time;
    EXPECT_EQ(static_cast<time_t>(0), (proxy.*method)());
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("get_download_speed()");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = uint64_t (tools::i_core_proxy::*)();
    const method_type method = &tools::i_core_proxy::get_download_speed;
    EXPECT_EQ(static_cast<uint64_t>(0), (proxy.*method)());
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("get_transfer_address(const std::string&, currency::account_public_address&, std::string&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const std::string&, currency::account_public_address&, std::string&);
    const method_type method = &tools::i_core_proxy::get_transfer_address;
    std::string adr_str{};
    currency::account_public_address addr{};
    std::string payment_id{};
    EXPECT_FALSE((proxy.*method)(adr_str, addr, payment_id));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("get_transfer_address(const std::string&, currency::address_v&, std::string&)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)(const std::string&, currency::address_v&, std::string&);
    const method_type method = &tools::i_core_proxy::get_transfer_address;
    std::string adr_str{};
    currency::address_v addr{};
    std::string payment_id{};
    EXPECT_FALSE((proxy.*method)(adr_str, addr, payment_id));
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("set_connectivity(unsigned int, size_t)");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = void (tools::i_core_proxy::*)(unsigned int, size_t);
    const method_type method = &tools::i_core_proxy::set_connectivity;
    unsigned int connection_timeout{};
    size_t repeats_count{};
    (proxy.*method)(connection_timeout, repeats_count);
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
  {
    SCOPED_TRACE("is_daemon_inbox()");
    proxy.begin_step();
    EXPECT_FALSE(proxy.denied());
    using method_type = bool (tools::i_core_proxy::*)();
    const method_type method = &tools::i_core_proxy::is_daemon_inbox;
    EXPECT_FALSE((proxy.*method)());
    EXPECT_TRUE(proxy.denied());
    EXPECT_EQ(0u, delegate->calls);
  }
}

TEST(read_only_core_proxy_surface, block_fetch_is_once_per_step_and_reset_is_explicit)
{
  auto delegate = std::make_shared<counting_delegate>();
  tools::read_only::core_proxy proxy(delegate);
  using method_type = bool (tools::i_core_proxy::*)(const currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request&, currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response&);
  const method_type method = &tools::i_core_proxy::call_COMMAND_RPC_GET_BLOCKS_DIRECT;
  currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request request{};
  currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response response{};
  proxy.begin_step();
  EXPECT_TRUE((proxy.*method)(request, response));
  EXPECT_FALSE(proxy.denied());
  EXPECT_EQ(1u, delegate->calls);
  EXPECT_FALSE((proxy.*method)(request, response));
  EXPECT_TRUE(proxy.denied());
  EXPECT_EQ(1u, delegate->calls);
  proxy.begin_step();
  EXPECT_FALSE(proxy.denied());
  EXPECT_TRUE((proxy.*method)(request, response));
  EXPECT_EQ(2u, delegate->calls);
  // A delegated false result still consumes the one allowed fetch.
  delegate->accept_blocks = false;
  proxy.begin_step();
  EXPECT_FALSE((proxy.*method)(request, response));
  EXPECT_FALSE(proxy.denied());
  EXPECT_EQ(3u, delegate->calls);
  EXPECT_FALSE((proxy.*method)(request, response));
  EXPECT_TRUE(proxy.denied());
  EXPECT_EQ(3u, delegate->calls);
}

TEST(read_only_core_proxy_surface, absent_delegate_cannot_fetch)
{
  tools::read_only::core_proxy proxy(nullptr);
  currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request request{};
  currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response response{};
  EXPECT_FALSE(proxy.available());
  proxy.begin_step();
  EXPECT_FALSE(proxy.call_COMMAND_RPC_GET_BLOCKS_DIRECT(request, response));
  EXPECT_TRUE(proxy.denied());
}
