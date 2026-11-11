// Copyright (c) 2026 Zano Project
// Distributed under the MIT/X11 software license, see COPYING.
#pragma once
#include "i_core_rpc_proxy.h"
#include "read_only_validated_prefix.h"
#include "read_only_wallet_birthday.h"
#include "read_only_birthday_prefix.h"

namespace tools { namespace read_only
{
  // Deny by default. One fixed birthday OR block request per step, never both.
  // The delegate is trusted native code; this is not a network sandbox.
  class core_proxy final : public i_core_proxy
  {
  public:
    explicit core_proxy(std::shared_ptr<i_core_proxy> transport)
      : m_transport(std::move(transport)),
        m_prefix_source(dynamic_cast<validated_prefix_source*>(m_transport.get())),
        m_birthday_source(dynamic_cast<birthday_height_source*>(m_transport.get())),
        m_birthday_prefix_source(dynamic_cast<birthday_prefix_source*>(m_transport.get())) {}
    void begin_step() noexcept { m_requested = false; m_denied = false; }
    bool denied() const noexcept { return m_denied; }
    bool available() const noexcept { return static_cast<bool>(m_transport); }
    bool has_validated_prefix_source() const noexcept { return m_prefix_source != nullptr; }
    bool fetch_validated_prefix(const currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request& rqt,
      prefix_selection_limits limits, validated_prefix_response& rsp, const std::atomic<bool>& stop)
    {
      rsp = {};
      if (!claim_fetch()) return false;
      if (!m_prefix_source) { m_denied = true; return false; }
      try
      {
        if (m_prefix_source->fetch_validated_prefix(rqt, limits, rsp, stop)) return true;
      }
      catch (...)
      {
        rsp = {};
        throw;
      }
      rsp = {};
      return false;
    }
    bool has_birthday_prefix_source() const noexcept { return m_birthday_prefix_source != nullptr; }
    bool fetch_birthday_prefix(const birthday_prefix::request& request,
      birthday_prefix::page& response, const std::atomic<bool>& stop)
    {
      response = {};
      if (!claim_fetch() || !m_birthday_prefix_source) { m_denied = true; return false; }
      try {
        if (m_birthday_prefix_source->fetch_birthday_prefix_page(request, response, stop)) return true;
      } catch (...) { response = {}; throw; }
      response = {};
      return false;
    }
    bool fetch_birthday(uint64_t timestamp, uint64_t& height, const std::atomic<bool>& stop)
    {
      height = 0;
      if (!claim_fetch() || !m_birthday_source) { m_denied = true; return false; }
      return m_birthday_source->resolve_birthday_height(timestamp, height, stop);
    }
    bool set_connection_addr(const std::string& url) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES(const currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& rqt, currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& rsp) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_GET_BLOCKS_FAST(const currency::COMMAND_RPC_GET_BLOCKS_FAST::request& rqt, currency::COMMAND_RPC_GET_BLOCKS_FAST::response& rsp) override { m_denied = true; return false; }
    // All DIRECT and native prefix entries share one fetch guard per step.
    bool call_COMMAND_RPC_GET_BLOCKS_DIRECT(const currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request& rqt, currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response& rsp) override
    {
      return fetch_once(rqt, rsp, nullptr);
    }
    bool call_COMMAND_RPC_GET_BLOCKS_DIRECT(const currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request& rqt, currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response& rsp, const std::atomic<bool>& stop) override
    {
      return fetch_once(rqt, rsp, &stop);
    }
    bool call_COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE(const currency::COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::request& rqt, currency::COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::response& rsp) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_GET_INFO(const currency::COMMAND_RPC_GET_INFO::request& rqt, currency::COMMAND_RPC_GET_INFO::response& rsp) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_GET_TX_POOL(const currency::COMMAND_RPC_GET_TX_POOL::request& rqt, currency::COMMAND_RPC_GET_TX_POOL::response& rsp) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_GET_ALIASES_BY_ADDRESS(const currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::request& rqt, currency::COMMAND_RPC_GET_ALIASES_BY_ADDRESS::response& rsp) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& rqt, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& rsp) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::request& rqt, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS3::response& rsp) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4(const currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::request& rqt, currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS4::response& rsp) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_SEND_RAW_TX(const currency::COMMAND_RPC_SEND_RAW_TX::request& rqt, currency::COMMAND_RPC_SEND_RAW_TX::response& rsp) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_FORCE_RELAY_RAW_TXS(const currency::COMMAND_RPC_FORCE_RELAY_RAW_TXS::request& rqt, currency::COMMAND_RPC_FORCE_RELAY_RAW_TXS::response& rsp) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_GET_ALL_ALIASES(currency::COMMAND_RPC_GET_ALL_ALIASES::response& rsp) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_GET_ALIAS_DETAILS(const currency::COMMAND_RPC_GET_ALIAS_DETAILS::request& req, currency::COMMAND_RPC_GET_ALIAS_DETAILS::response& rsp) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_GET_ALIAS_REWARD(const currency::COMMAND_RPC_GET_ALIAS_REWARD::request& req, currency::COMMAND_RPC_GET_ALIAS_REWARD::response& rsp) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_ALIAS_LOOKUP(const currency::COMMAND_RPC_ALIAS_LOOKUP::request& req, currency::COMMAND_RPC_ALIAS_LOOKUP::response& rsp) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_GET_TRANSACTIONS(const currency::COMMAND_RPC_GET_TRANSACTIONS::request& req, currency::COMMAND_RPC_GET_TRANSACTIONS::response& rsp) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_COMMAND_RPC_CHECK_KEYIMAGES(const currency::COMMAND_RPC_CHECK_KEYIMAGES::request& req, currency::COMMAND_RPC_CHECK_KEYIMAGES::response& rsp) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_GETBLOCKTEMPLATE(const currency::COMMAND_RPC_GETBLOCKTEMPLATE::request& req, currency::COMMAND_RPC_GETBLOCKTEMPLATE::response& rsp) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_SUBMITBLOCK(const currency::COMMAND_RPC_SUBMITBLOCK::request& req, currency::COMMAND_RPC_SUBMITBLOCK::response& rsp) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_SUBMITBLOCK2(const currency::COMMAND_RPC_SUBMITBLOCK2::request& req, currency::COMMAND_RPC_SUBMITBLOCK2::response& rsp) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_GET_POS_MINING_DETAILS(const currency::COMMAND_RPC_GET_POS_MINING_DETAILS::request& req, currency::COMMAND_RPC_GET_POS_MINING_DETAILS::response& rsp) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_GET_BLOCKS_DETAILS(const currency::COMMAND_RPC_GET_BLOCKS_DETAILS::request& req, currency::COMMAND_RPC_GET_BLOCKS_DETAILS::response& res) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_GET_OFFERS_EX(const currency::COMMAND_RPC_GET_OFFERS_EX::request& req, currency::COMMAND_RPC_GET_OFFERS_EX::response& res) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN(const currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::request& req, currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::response& res) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_GET_POOL_INFO(const currency::COMMAND_RPC_GET_POOL_INFO::request& req, currency::COMMAND_RPC_GET_POOL_INFO::response& res) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_GET_ASSET_INFO(const currency::COMMAND_RPC_GET_ASSET_INFO::request& req, currency::COMMAND_RPC_GET_ASSET_INFO::response& res) override { m_denied = true; return false; }
    bool call_COMMAND_RPC_INVOKE(const std::string& uri, const std::string& body, int& response_code, std::string& response_body) override { m_denied = true; return false; }
    bool check_connection() override { m_denied = true; return false; }
    time_t get_last_success_interract_time() override { m_denied = true; return 0; }
    uint64_t get_download_speed() override { m_denied = true; return 0; }
    bool get_transfer_address(const std::string& adr_str, currency::account_public_address& addr, std::string& payment_id) override { m_denied = true; return false; }
    bool get_transfer_address(const std::string& adr_str, currency::address_v& addr, std::string& payment_id) override { m_denied = true; return false; }
    void set_connectivity(unsigned int connection_timeout, size_t repeats_count) override { m_denied = true; }
    bool is_daemon_inbox() override { m_denied = true; return false; }
  private:
    bool claim_fetch() noexcept
    {
      if (m_requested || !m_transport) { m_denied = true; return false; }
      m_requested = true;
      return true;
    }
    bool fetch_once(const currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request& rqt, currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response& rsp, const std::atomic<bool>* stop)
    {
      if (!claim_fetch()) return false;
      // Forward the same borrowed token; a transport without the stop-aware
      // override gets the base default (sample, then legacy call).
      return stop ? m_transport->call_COMMAND_RPC_GET_BLOCKS_DIRECT(rqt, rsp, *stop)
                  : m_transport->call_COMMAND_RPC_GET_BLOCKS_DIRECT(rqt, rsp);
    }
    const std::shared_ptr<i_core_proxy> m_transport;
    // Discovered once; the owning shared_ptr keeps the native side alive.
    validated_prefix_source* const m_prefix_source;
    birthday_height_source* const m_birthday_source;
    birthday_prefix_source* const m_birthday_prefix_source;
    bool m_requested = false;
    bool m_denied = false;
  };
}}
