// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "chaingen.h"

struct wallet_test_core_proxy : public tools::i_core_proxy
{
  wallet_test_core_proxy();

  void clear();
  bool update_blockchain(const std::vector<test_event_entry>& events, test_generator& generator, const crypto::hash& blockchain_head);

  ////////////////////////////////////////////
  // i_core_proxy
  ////////////////////////////////////////////
  virtual bool call_COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES(const currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::request& rqt, currency::COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES::response& rsp) override;
  virtual bool call_COMMAND_RPC_GET_BLOCKS_FAST(const currency::COMMAND_RPC_GET_BLOCKS_FAST::request& rqt, currency::COMMAND_RPC_GET_BLOCKS_FAST::response& rsp) override;
  virtual bool call_COMMAND_RPC_GET_BLOCKS_DIRECT(const currency::COMMAND_RPC_GET_BLOCKS_DIRECT::request& rqt, currency::COMMAND_RPC_GET_BLOCKS_DIRECT::response& rsp) override;
  virtual bool call_COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE(const currency::COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::request& rqt, currency::COMMAND_RPC_GET_EST_HEIGHT_FROM_DATE::response& rsp) override;
  virtual bool call_COMMAND_RPC_GET_INFO(const currency::COMMAND_RPC_GET_INFO::request& rqt, currency::COMMAND_RPC_GET_INFO::response& rsp) override;
  virtual bool call_COMMAND_RPC_SEND_RAW_TX(const currency::COMMAND_RPC_SEND_RAW_TX::request& rqt, currency::COMMAND_RPC_SEND_RAW_TX::response& rsp) override;
  virtual bool call_COMMAND_RPC_GET_TX_POOL(const currency::COMMAND_RPC_GET_TX_POOL::request& rqt, currency::COMMAND_RPC_GET_TX_POOL::response& rsp) override;
  virtual bool call_COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN(const currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::request& req, currency::COMMAND_RPC_GET_CURRENT_CORE_TX_EXPIRATION_MEDIAN::response& res) override;
  virtual bool get_transfer_address(const std::string& adr_str, currency::account_public_address& addr, std::string& payment_id) override;

  const std::vector<uint64_t>& get_tx_gindex(const crypto::hash& tx_id);


  test_generator::tx_global_indexes m_txs_outs;
  test_generator::blockchain_vector m_blocks;
  test_generator::outputs_index m_oi;
  std::list<currency::blobdata> m_unconfirmed_txs; // txs in the event queue that weren't included into a block

  bool m_first_call;
};
