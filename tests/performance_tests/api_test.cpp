// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "wallet/core_rpc_proxy.h"
#include "wallet/core_default_rpc_proxy.h"

int test_get_rand_outs()
{
  currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request req = AUTO_VAL_INIT(req);
  currency::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response rsp = AUTO_VAL_INIT(rsp);
  req.use_forced_mix_outs = false;
  req.decoys_count = 10 + 1;
  for (size_t i = 0; i != 50; i++)
    req.amounts.push_back(COIN);

  std::shared_ptr<tools::i_core_proxy> m_core_proxy(new tools::default_http_core_proxy());
  m_core_proxy->set_connection_addr("127.0.0.1:11211");
  m_core_proxy->check_connection();

  bool r = m_core_proxy->call_COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS(req, rsp);

  return 0;
}
