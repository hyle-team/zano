// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma  once
#include "currency_protocol/currency_protocol_handler.h"
#include "p2p/net_node.h"

// Helper listener.
// Usefull to process all txs and/or blocks are being pushed to the core (from events queue and from core proxy).
// (ex: to make sure all txs have correct blob size across all the tests)
class test_core_listener
{
public:
  virtual ~test_core_listener() = default;

  virtual void before_tx_pushed_to_core(const currency::transaction& tx, const currency::blobdata& blob, currency::core& c, bool invalid_tx = false) {}  // invalid_tx is true when processing a tx, marked as invalid in a test
  virtual void before_block_pushed_to_core(const currency::block& block, const currency::blobdata& blob, currency::core& c) {}
};

// Special protocol handler for test environment.
// Pretends being always connected, synctonized and so on.
// Necessary for running tools::wallet2 in test environment.
class test_protocol_handler : public currency::t_currency_protocol_handler<currency::core>
{
public:
  typedef currency::t_currency_protocol_handler<currency::core> super_class_t;

  test_protocol_handler(currency::core& core, test_core_listener* core_listener = nullptr)
    : super_class_t(core, nullptr)
    , m_core_listener(core_listener)
    , m_core(core)
  {}

  ~test_protocol_handler()
  {}

  bool init(const boost::program_options::variables_map& vm)
  {
    return super_class_t::init(vm);
  }

  bool deinit()
  {
    return super_class_t::deinit();
  }

  virtual bool is_synchronized() override { return true; } // pretend being always syncronized
  virtual size_t get_synchronized_connections_count() override { return 1; } // pretend being connected to a peer

  //----------------- i_currency_protocol ---------------------------------------
  virtual bool relay_block(currency::NOTIFY_NEW_BLOCK::request& /*arg*/, currency::currency_connection_context& /*exclude_context*/) override { return false; }
  
  virtual bool relay_transactions(currency::NOTIFY_OR_INVOKE_NEW_TRANSACTIONS::request& arg, currency::currency_connection_context& /*exclude_context*/) override
  {
    if (m_core_listener)
    {
      for (auto& tx_blob : arg.txs)
      {
        currency::transaction tx = AUTO_VAL_INIT(tx);
        crypto::hash stub;
        currency::parse_and_validate_tx_from_blob(tx_blob, tx, stub);
        m_core_listener->before_tx_pushed_to_core(tx, tx_blob, m_core);
      }
    }
    return false;
  }

private:
  currency::core& m_core;
  test_core_listener* m_core_listener;
};

class test_node_server : public nodetool::node_server<currency::t_currency_protocol_handler<currency::core> >
{
public:
  test_node_server(test_protocol_handler& ph)
    : nodetool::node_server<currency::t_currency_protocol_handler<currency::core> >(ph)
  {}

  virtual uint64_t get_connections_count() override { return 1; } // pretend to be always connected
};

