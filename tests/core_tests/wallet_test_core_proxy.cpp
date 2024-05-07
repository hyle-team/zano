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

  size_t invalid_tx_index = UINT64_MAX;
  for (size_t i = 0; i < events.size(); ++i)
  {
    const test_event_entry& e = events[i];
    if (test_chain_unit_enchanced::is_event_mark_invalid_tx(e))
    {
      invalid_tx_index = i + 1;
      continue;
    }
    if (e.type() != typeid(currency::transaction) || i == invalid_tx_index)
      continue;

    const currency::transaction& tx = boost::get<currency::transaction>(e);
    if (confirmed_txs.count(currency::get_transaction_hash(tx)) == 0)
      m_unconfirmed_txs.push_back(currency::tx_to_blob(tx));
  }

  return true;
}

bool wallet_test_core_proxy::call_COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES(const currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& req, currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& res)
{
  res.tx_global_outs.resize(req.txids.size());
  size_t i = 0;
  for (auto& txid : req.txids)
  {
    auto it = m_txs_outs.find(txid);
    CHECK_AND_ASSERT_MES(it != m_txs_outs.end(), false, "tx " << txid << " was not found in tx global outout indexes");
    res.tx_global_outs[i].v = it->second;
    i++;
  }
  res.status = API_RETURN_CODE_OK;
  return true;
}

bool wallet_test_core_proxy::call_COMMAND_RPC_GET_BLOCKS_FAST(const currency::COMMAND_RPC_GET_BLOCKS_FAST::request& rqt, currency::COMMAND_RPC_GET_BLOCKS_FAST::response& rsp)
{
  //might be not best way to do it.
  std::unordered_map<crypto::hash, uint64_t> blocks_map;
  for (uint64_t i = 0; i != m_blocks.size(); i++)
  {
    blocks_map[currency::get_block_hash(m_blocks[i]->b)] = i;
  }

  rsp.start_height = 0;
  //find out where we supposed to start refresh
  for (auto id : rqt.block_ids)
  {
    auto it = blocks_map.find(id);
    if (it == blocks_map.end())
      continue;
    rsp.start_height = it->second;
    break;
  }

  rsp.current_height = m_blocks.size();
  rsp.status = API_RETURN_CODE_OK;
  if (!m_first_call)
  {
    m_first_call = true;
    return true; // respond with empty blocks on second call to gracefully stop wallet refreshing
  }
  m_first_call = false;
  for (size_t i = rsp.start_height; i != m_blocks.size(); i++)
  {
    auto b = m_blocks[i];
    currency::block_complete_entry bce = AUTO_VAL_INIT(bce);
    bce.tx_global_outs.resize(b->m_transactions.size());
    bce.coinbase_global_outs = get_tx_gindex(currency::get_transaction_hash(b->b.miner_tx));
    for (size_t j = 0; j != b->m_transactions.size(); j++)
    {
      const auto& tx = b->m_transactions[j];
      bce.txs.push_back(tx_to_blob(tx));
      bce.tx_global_outs[j].v = get_tx_gindex(currency::get_transaction_hash(tx));
    }
    bce.block = block_to_blob(b->b);
    rsp.blocks.push_back(bce);
  }
  rsp.current_height = m_blocks.size();
  return true;
}

const std::vector<uint64_t>& wallet_test_core_proxy::get_tx_gindex(const crypto::hash& tx_id)
{
  return get_tx_gindex_from_map(tx_id, m_txs_outs);
}
bool wallet_test_core_proxy::call_COMMAND_RPC_GET_BLOCKS_DIRECT(const currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request& rqt, currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response& rsp)
{
  currency::COMMAND_RPC_GET_BLOCKS_FAST::request req = AUTO_VAL_INIT(req);
  req.block_ids = rqt.block_ids;
  req.minimum_height = rqt.minimum_height;
  currency::COMMAND_RPC_GET_BLOCKS_FAST::response res = AUTO_VAL_INIT(res);
  bool r = this->call_COMMAND_RPC_GET_BLOCKS_FAST(req, res);
  rsp.status = res.status;
  if (rsp.status == API_RETURN_CODE_OK)
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
  rsp.status = API_RETURN_CODE_OK;
  return true;
}

bool wallet_test_core_proxy::call_COMMAND_RPC_GET_INFO(const currency::COMMAND_RPC_GET_INFO::request& rqt, currency::COMMAND_RPC_GET_INFO::response& rsp)
{
  rsp.synchronized_connections_count = 1;
  return true;
}

bool wallet_test_core_proxy::call_COMMAND_RPC_SEND_RAW_TX(const currency::COMMAND_RPC_SEND_RAW_TX::request& rqt, currency::COMMAND_RPC_SEND_RAW_TX::response& rsp)
{
  rsp.status = API_RETURN_CODE_OK;
  return true;
}

bool wallet_test_core_proxy::call_COMMAND_RPC_GET_TX_POOL(const currency::COMMAND_RPC_GET_TX_POOL::request& rqt, currency::COMMAND_RPC_GET_TX_POOL::response& rsp)
{
  rsp.status = API_RETURN_CODE_OK;
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
  res.status = API_RETURN_CODE_OK;
  return true;
}

bool wallet_test_core_proxy::get_transfer_address(const std::string& adr_str, currency::account_public_address& addr, std::string& payment_id)
{
  return tools::get_transfer_address_cb(adr_str, addr, payment_id, [](currency::COMMAND_RPC_GET_ALIAS_DETAILS::request&, currency::COMMAND_RPC_GET_ALIAS_DETAILS::response){
   LOG_ERROR("alias resolving is not currently supported by wallet_test_core_proxy");
    return false;
  });

}
