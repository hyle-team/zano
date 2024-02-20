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
	virtual void on_transfer2(size_t wallet_id, const tools::wallet_public::wallet_transfer_info& wti, const std::list<tools::wallet_public::asset_balance_entry>& balances, uint64_t total_mined) {}
  virtual void on_pos_block_found(size_t wallet_id, const currency::block& /*block*/) {}
  virtual void on_sync_progress(size_t wallet_id, const uint64_t& /*percents*/) {}
  virtual void on_transfer_canceled(size_t wallet_id, const tools::wallet_public::wallet_transfer_info& wti) {}
  virtual void on_tor_status_change(size_t wallet_id, const std::string& state) {}

  virtual void on_mw_get_wallets(std::vector<tools::wallet_public::wallet_entry_info>& wallets) {}
  virtual bool on_mw_select_wallet(uint64_t wallet_id) { return true; }

};

struct i_wallet_to_i_backend_adapter: public tools::i_wallet2_callback
{
  i_wallet_to_i_backend_adapter(i_backend_wallet_callback* pbackend, size_t wallet_id) :m_pbackend(pbackend),
                                                                                        m_wallet_id(wallet_id)
  {}

  virtual void on_new_block(uint64_t height, const currency::block& block) {
    m_pbackend->on_new_block(m_wallet_id, height, block);
  }
	virtual void on_transfer2(const tools::wallet_public::wallet_transfer_info& wti, const std::list<tools::wallet_public::asset_balance_entry>& balances, uint64_t total_mined) {
		m_pbackend->on_transfer2(m_wallet_id, wti, balances, total_mined);
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
  virtual void on_tor_status_change(const std::string& state)
  {
    m_pbackend->on_tor_status_change(m_wallet_id, state);
  }

  virtual void on_mw_get_wallets(std::vector<tools::wallet_public::wallet_entry_info>& wallets) 
  {
    m_pbackend->on_mw_get_wallets(wallets);
  }
  virtual bool on_mw_select_wallet(uint64_t wallet_id) 
  {
    return m_pbackend->on_mw_select_wallet(wallet_id);
  }


private:
  i_backend_wallet_callback* m_pbackend;
  size_t m_wallet_id;
};
