// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "wallet/wallet2.h"

class i_backend_wallet_callback
{
public:
  virtual void on_new_block(size_t wallet_id, uint64_t /*height*/, const currency::block& /*block*/) {}
	virtual void on_transfer2(size_t wallet_id, const tools::wallet_public::wallet_transfer_info& wti, uint64_t balance, uint64_t unlocked_balance, uint64_t total_mined) {}
  virtual void on_pos_block_found(size_t wallet_id, const currency::block& /*block*/) {}
  virtual void on_sync_progress(size_t wallet_id, const uint64_t& /*percents*/) {}
  virtual void on_transfer_canceled(size_t wallet_id, const tools::wallet_public::wallet_transfer_info& wti) {}
};

struct i_wallet_to_i_backend_adapter: public tools::i_wallet2_callback
{
  i_wallet_to_i_backend_adapter(i_backend_wallet_callback* pbackend, size_t wallet_id) :m_pbackend(pbackend),
                                                                                        m_wallet_id(wallet_id)
  {}

  virtual void on_new_block(uint64_t height, const currency::block& block) {
    m_pbackend->on_new_block(m_wallet_id, height, block);
  }
	virtual void on_transfer2(const tools::wallet_public::wallet_transfer_info& wti, uint64_t balance, uint64_t unlocked_balance, uint64_t total_mined) {
		m_pbackend->on_transfer2(m_wallet_id, wti, balance, unlocked_balance, total_mined);
  }
  virtual void on_pos_block_found(const currency::block& wti) {
    m_pbackend->on_pos_block_found(m_wallet_id, wti);
  }
  virtual void on_sync_progress(const uint64_t& progress) {
    m_pbackend->on_sync_progress(m_wallet_id, progress);
  }
  virtual void on_transfer_canceled(const tools::wallet_public::wallet_transfer_info& wti) {
    m_pbackend->on_transfer_canceled(m_wallet_id, wti);
  }
private:
  i_backend_wallet_callback* m_pbackend;
  size_t m_wallet_id;
};
