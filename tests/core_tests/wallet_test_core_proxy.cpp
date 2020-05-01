// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "wallet_test_core_proxy.h"
#include "currency_core/alias_helper.h"

wallet_test_core_proxy::wallet_test_core_proxy()
  : m_first_call(true)
{}

void wallet_test_core_proxy::clear()
{
  m_blocks.clear();
  m_txs_outs.clear();
  m_oi.clear();
  m_unconfirmed_txs.clear();
  m_first_call = true;
}

bool wallet_test_core_proxy::update_blockchain(const std::vector<test_event_entry>& events, test_generator& generator, const crypto::hash& blockchain_head)
{
  clear();

  generator.get_block_chain(m_blocks, blockchain_head, std::numeric_limits<size_t>::max());
  generator.build_outputs_indext_for_chain(m_blocks, m_oi, m_txs_outs);

  // build unconfirmed tx list
  std::unordered_set<crypto::hash> confirmed_txs;
  for (auto b : m_blocks)
    std::for_each(b->b.tx_hashes.begin(), b->b.tx_hashes.end(), [&confirmed_txs](const crypto::hash& h) { confirmed_txs.insert(h); });

  for (auto e : events)
  {
    if (e.type() != typeid(currency::transaction))
      continue;

    const currency::transaction& tx = boost::get<currency::transaction>(e);
    if (confirmed_txs.count(currency::get_transaction_hash(tx)) == 0)
      m_unconfirmed_txs.push_back(currency::tx_to_blob(tx));
  }

  return true;
}

bool wallet_test_core_proxy::call_COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES(const currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& rqt, currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& rsp)
{
  auto it = m_txs_outs.find(rqt.txid);
  CHECK_AND_ASSERT_MES(it != m_txs_outs.end(), false, "tx " << rqt.txid << " was not found in tx global outout indexes");
  rsp.status = CORE_RPC_STATUS_OK;
  rsp.o_indexes = it->second;
  return true;
}

bool wallet_test_core_proxy::call_COMMAND_RPC_GET_BLOCKS_FAST(const currency::COMMAND_RPC_GET_BLOCKS_FAST::request& rqt, currency::COMMAND_RPC_GET_BLOCKS_FAST::response& rsp)
{
  rsp.current_height = 0;
  rsp.start_height = 0;
  rsp.status = CORE_RPC_STATUS_OK;
  if (!m_first_call)
  {
    m_first_call = true;
    return true; // respond with empty blocks on second call to gracefully stop wallet refreshing
  }
  m_first_call = false;
  for (auto b : m_blocks)
  {
    currency::block_complete_entry bce = AUTO_VAL_INIT(bce);
    for (auto tx : b->m_transactions)
      bce.txs.push_back(tx_to_blob(tx));
    bce.block = block_to_blob(b->b);
    rsp.blocks.push_back(bce);
  }
  rsp.current_height = m_blocks.size() - 1;
  return true;
}
bool wallet_test_core_proxy::call_COMMAND_RPC_GET_BLOCKS_DIRECT(const currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request& rqt, currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response& rsp)
{
  currency::COMMAND_RPC_GET_BLOCKS_FAST::request req;
  req.block_ids = rqt.block_ids;
  currency::COMMAND_RPC_GET_BLOCKS_FAST::response res = AUTO_VAL_INIT(res);
  bool r = this->call_COMMAND_RPC_GET_BLOCKS_FAST(req, res);
  rsp.status = res.status;
  if (rsp.status == CORE_RPC_STATUS_OK)
  {
    rsp.current_height = res.current_height;
    rsp.start_height = res.start_height;
    r = unserialize_block_complete_entry(res, rsp);
  }
  return r;

}

bool wallet_test_core_proxy::call_COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE(const currency::COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::request& rqt, currency::COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::response& rsp)
{
  rsp.h = 0;
  rsp.status = CORE_RPC_STATUS_OK;
  return true;
}

bool wallet_test_core_proxy::call_COMMAND_RPC_GET_INFO(const currency::COMMAND_RPC_GET_INFO::request& rqt, currency::COMMAND_RPC_GET_INFO::response& rsp)
{
  rsp.synchronized_connections_count = 1;
  return true;
}

bool wallet_test_core_proxy::call_COMMAND_RPC_SEND_RAW_TX(const currency::COMMAND_RPC_SEND_RAW_TX::request& rqt, currency::COMMAND_RPC_SEND_RAW_TX::response& rsp)
{
  rsp.status = CORE_RPC_STATUS_OK;
  return true;
}

bool wallet_test_core_proxy::call_COMMAND_RPC_GET_TX_POOL(const currency::COMMAND_RPC_GET_TX_POOL::request& rqt, currency::COMMAND_RPC_GET_TX_POOL::response& rsp)
{
  rsp.status = CORE_RPC_STATUS_OK;
  rsp.txs = m_unconfirmed_txs;
  return true;
}

bool wallet_test_core_proxy::call_COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN(const currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::request& req, currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::response& res)
{
  res.expiration_median = 0;
  uint64_t count = 0;
  std::vector<uint64_t> tiemstamps;
  for (auto it = m_blocks.rbegin(); it != m_blocks.rend() && count <= TX_EXPIRATION_TIMESTAMP_CHECK_WINDOW; it++, count++)
  {
    tiemstamps.push_back((*it)->b.timestamp);
  }

  res.expiration_median = epee::misc_utils::median(tiemstamps);  
  res.status = CORE_RPC_STATUS_OK;
  return true;
}

bool wallet_test_core_proxy::get_transfer_address(const std::string& adr_str, currency::account_public_address& addr, std::string& payment_id)
{
  return tools::get_transfer_address_cb(adr_str, addr, payment_id, [](currency::COMMAND_RPC_GET_ALIAS_DETAILS::request&, currency::COMMAND_RPC_GET_ALIAS_DETAILS::response){
   LOG_ERROR("alias resolving is not currently supported by wallet_test_core_proxy");
    return false;
  });

}
