// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"
#include "currency_protocol/currency_protocol_handler.h"
#include "currency_core/currency_core.h"
#include "p2p/net_node.h"
#include "currency_core/bc_offers_service.h"
#include "rpc/core_rpc_server.h"

struct wallet_test : virtual public test_chain_unit_enchanced
{
  enum { MINER_ACC_IDX = 0, ALICE_ACC_IDX = 1, BOB_ACC_IDX = 2, CAROL_ACC_IDX = 3, DAN_ACC_IDX = 4, TOTAL_ACCS_COUNT = 5 }; // to be used as index for m_accounts

  wallet_test();

  bool need_core_proxy() const { return true; }
  void set_core_proxy(std::shared_ptr<tools::i_core_proxy> p) { m_core_proxy = p; }
  bool check_balance_via_build_wallets(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool check_balance(currency::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

  void on_test_constructed() override { on_test_generator_created(this->generator); }

  static std::string get_test_account_name_by_id(size_t acc_id);

  template<typename wallet_t>
  std::shared_ptr<wallet_t> init_playtime_test_wallet_t(const std::vector<test_event_entry>& events, currency::core& c, const currency::account_base& acc, bool true_http_rpc = false) const
  {
    CHECK_AND_ASSERT_THROW_MES(events.size() > 0 && events[0].type() == typeid(currency::block), "Invalid events queue, can't find genesis block at the beginning");
    crypto::hash genesis_hash = get_block_hash(boost::get<currency::block>(events[0]));

    if (true_http_rpc)
    {
      m_core_proxy = std::make_shared<tools::default_http_core_proxy>();
      m_core_proxy->set_connectivity(100, 1);
      CHECK_AND_ASSERT_MES(m_core_proxy->set_connection_addr("127.0.0.1:33777"), false, "");
      if (!m_core_proxy->check_connection())
      {
        // if there's not http rpc core server yet, create one
        boost::program_options::options_description desc_options;
        currency::core_rpc_server::init_options(desc_options);
        boost::program_options::variables_map vm{};
        char* argv[] = {"--rpc-bind-ip=127.0.0.1", "--rpc-bind-port=33777"};
        boost::program_options::store(boost::program_options::parse_command_line(sizeof argv / sizeof argv[0], argv, desc_options), vm);
        m_http_rpc_server = std::make_shared<core_http_rpc_server_details>(c, vm);
      }
      m_core_proxy->set_connectivity(30000, 1);
      CHECK_AND_ASSERT_MES(m_core_proxy->check_connection(), false, "no connection");
    }

    std::shared_ptr<wallet_t> w(new wallet_t);
    w->set_core_runtime_config(c.get_blockchain_storage().get_core_runtime_config());
    w->assign_account(acc);
    w->set_genesis(genesis_hash);
    w->set_core_proxy(m_core_proxy);
    w->set_disable_tor_relay(true);
    return w;
  }


  template<typename wallet_t>
  std::shared_ptr<wallet_t> init_playtime_test_wallet_t(const std::vector<test_event_entry>& events, currency::core& c, size_t account_index) const
  {
    CHECK_AND_ASSERT_THROW_MES(account_index < m_accounts.size(), "Invalid account index");
    return init_playtime_test_wallet_t<wallet_t>(events, c, m_accounts[account_index]);
  }


protected:

  struct params_check_balance
  {
    params_check_balance(size_t account_index = 0,
      uint64_t total_balance = 0,
      uint64_t unlocked_balance = std::numeric_limits<uint64_t>::max(),
      uint64_t mined_balance = std::numeric_limits<uint64_t>::max(),
      uint64_t awaiting_in = std::numeric_limits<uint64_t>::max(),
      uint64_t awaiting_out = std::numeric_limits<uint64_t>::max())
      : account_index(account_index), total_balance(total_balance), unlocked_balance(unlocked_balance), mined_balance(mined_balance), awaiting_in(awaiting_in), awaiting_out(awaiting_out) {}
    uint64_t total_balance;
    uint64_t unlocked_balance;
    uint64_t mined_balance;
    uint64_t awaiting_in;
    uint64_t awaiting_out;
    size_t account_index;
  };

  struct core_http_rpc_server_details
  {
    currency::t_currency_protocol_handler<currency::core> cph;
    nodetool::node_server<currency::t_currency_protocol_handler<currency::core> > p2p;
    bc_services::bc_offers_service bos;
    currency::core_rpc_server core_rpc_server;

    core_http_rpc_server_details(currency::core& c, const boost::program_options::variables_map& vm)
      : cph(c, nullptr)
      , p2p(cph)
      , bos(nullptr)
      , core_rpc_server(c, p2p, bos)
    {
      bos.set_disabled(true);
      core_rpc_server.set_ignore_connectivity_status(true);
      
      bool r = core_rpc_server.init(vm);
      CRYPTO_CHECK_AND_THROW_MES(r, "core_http_rpc_server_details: rpc server init failed");
      r = core_rpc_server.run(2, false);
      CRYPTO_CHECK_AND_THROW_MES(r, "core_http_rpc_server_details: rpc server run failed");
    }

    ~core_http_rpc_server_details()
    {
      core_rpc_server.deinit();
    }
  };

  std::shared_ptr<tools::wallet2> init_playtime_test_wallet(const std::vector<test_event_entry>& events, currency::core& c, size_t account_index) const;
  std::shared_ptr<tools::wallet2> init_playtime_test_wallet(const std::vector<test_event_entry>& events, currency::core& c, const currency::account_base& acc) const;
  std::shared_ptr<tools::wallet2> wallet_test::init_playtime_test_wallet_with_true_http_rpc(const std::vector<test_event_entry>& events, currency::core& c, size_t account_index) const;

  mutable std::vector<currency::account_base> m_accounts;
  mutable test_generator generator;
  mutable std::shared_ptr<tools::i_core_proxy> m_core_proxy;
  mutable std::shared_ptr<core_http_rpc_server_details> m_http_rpc_server; // "True" RPC via http on localhost
};


inline const tools::wallet_public::asset_balance_entry get_native_balance_entry(const std::list<tools::wallet_public::asset_balance_entry>& balances)
{
  for (const auto& b : balances)
  {
    if (b.asset_info.asset_id == currency::native_coin_asset_id)
      return b;
  }
  return tools::wallet_public::asset_balance_entry();
}

// wallet callback helper to check balance in wallet callbacks
// see escrow_balance test for usage example 
struct wallet_callback_balance_checker : public tools::i_wallet2_callback
{
  wallet_callback_balance_checker(const std::string& label) : m_label(label), m_result(true), m_called(false), m_balance(UINT64_MAX), m_unlocked_balance(UINT64_MAX), m_total_mined(UINT64_MAX) {}

  void expect_balance(uint64_t balance = UINT64_MAX, uint64_t unlocked_balance = UINT64_MAX, uint64_t total_mined = UINT64_MAX)
  {
    m_balance = balance;
    m_unlocked_balance = unlocked_balance;
    m_total_mined = total_mined;
    m_called = false;
  }

  virtual void on_transfer2(const tools::wallet_public::wallet_transfer_info& wti, const std::list<tools::wallet_public::asset_balance_entry>& balances, uint64_t total_mined) override
  {
    tools::wallet_public::asset_balance_entry native_balance = get_native_balance_entry(balances);
    uint64_t balance = native_balance.total;
    uint64_t unlocked_balance = native_balance.unlocked;

    m_called = true;
    m_result = false;
    CHECK_AND_ASSERT_MES(m_balance == UINT64_MAX          || balance          == m_balance,          (void)(0), m_label << " balance is incorrect: "          << currency::print_money_brief(balance)          << ", expected: " << currency::print_money_brief(m_balance));
    CHECK_AND_ASSERT_MES(m_unlocked_balance == UINT64_MAX || unlocked_balance == m_unlocked_balance, (void)(0), m_label << " unlocked balance is incorrect: " << currency::print_money_brief(unlocked_balance) << ", expected: " << currency::print_money_brief(m_unlocked_balance));
    CHECK_AND_ASSERT_MES(m_total_mined == UINT64_MAX      || total_mined      == m_total_mined,      (void)(0), m_label << " total mined is incorrect: "      << currency::print_money_brief(total_mined)      << ", expected: " << currency::print_money_brief(m_total_mined));
    m_result = true;
  }

  bool check()
  {
    bool result = m_result;
    m_result = false; // clear result to avoid errorneous successive calls to check() without calling except_balance()
    return result;
  }

  bool called()
  {
    return m_called;
  }

  bool m_result;
  bool m_called;
  std::string m_label;
  uint64_t m_balance;
  uint64_t m_unlocked_balance;
  uint64_t m_total_mined;
};

struct wlt_lambda_on_transfer2_wrapper : public tools::i_wallet2_callback
{
  typedef std::function<bool(const tools::wallet_public::wallet_transfer_info&, const std::list<tools::wallet_public::asset_balance_entry>&, uint64_t)> Func;
  wlt_lambda_on_transfer2_wrapper(Func callback) : m_result(false), m_callback(callback) {}
  virtual void on_transfer2(const tools::wallet_public::wallet_transfer_info& wti, const std::list<tools::wallet_public::asset_balance_entry>& balances, uint64_t total_mined) override
  {
    m_result = m_callback(wti, balances, total_mined);
  }
  bool m_result;
  Func m_callback;
};

class  debug_wallet2: public tools::wallet2
{
public:
  epee::misc_utils::events_dispatcher& get_debug_events_dispatcher()
  {
    return this->m_debug_events_dispatcher;
  }
};