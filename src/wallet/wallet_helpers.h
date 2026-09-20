// Copyright (c) 2014-2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once


#include "wallet2.h"
#include "view_iface.h"


#ifdef MOBILE_WALLET_BUILD
  #define DAEMON_IDLE_UPDATE_TIME_MS        10000
  #define TX_POOL_SCAN_INTERVAL             5
#else
  #define DAEMON_IDLE_UPDATE_TIME_MS        2000
  #define TX_POOL_SCAN_INTERVAL             1
#endif

namespace tools
{

  inline uint64_t get_native_entry_balance(const std::list<tools::wallet_public::asset_balance_entry>& balances)
  {
    for (const auto& b : balances)
    {
      if (b.asset_info.asset_id == currency::native_coin_asset_id)
        return b.total;
    }
    return 0;
  }

  //this number estimated based on the staking performance of the dev fund wallet during last month,
  //and is used to give users some rough estimation of how many iterations they need to do to mint a block
  //with their current stake; of course, it can be very different for different wallets, but at least it gives some reference point
#ifndef TESTNET
#define ITERATIONS_NEEDED_PER_ONE_COIN_MAGIC_NUMBER 464400000
#else
#define ITERATIONS_NEEDED_PER_ONE_COIN_MAGIC_NUMBER 283000 // testnet = mainnet * (pos_diff_testnet / pos_diff_mainnet)
#endif

  inline uint64_t estimate_iterations_per_pos_block(const std::list<tools::wallet_public::asset_balance_entry>& balances)
  {
    uint64_t native_amount = get_native_entry_balance(balances);
    //reduce it to decimal part of the coin, to avoid overflow
    native_amount /= COIN;
    if (native_amount)
      return ITERATIONS_NEEDED_PER_ONE_COIN_MAGIC_NUMBER / native_amount;
    return 0;
  }

  inline bool get_wallet_info_unlocked(wallet2& w, view::wallet_info& wi)
  {
    wi = AUTO_VAL_INIT_T(view::wallet_info);
    wi.address = w.get_account().get_public_address_str();
    wi.view_sec_key = epee::string_tools::pod_to_hex(w.get_account().get_keys().view_secret_key);
    wi.path = epee::string_encoding::wstring_to_utf8(w.get_wallet_path());
    wi.is_auditable = w.is_auditable();
    wi.is_watch_only = w.is_watch_only();
    wi.current_pos_attempts = w.get_current_pos_attempts();
    return true;
  }

  inline bool get_wallet_info(wallet2& w, view::wallet_info& wi)
  {
    wi = AUTO_VAL_INIT_T(view::wallet_info);
    get_wallet_info_unlocked(w, wi);
    w.balance(wi.balances, wi.mined_total);
    wi.has_bare_unspent_outputs = w.has_bare_unspent_outputs();
    wi.est_iterations_per_pos_block = estimate_iterations_per_pos_block(wi.balances);
    return true;
  }

  inline bool get_is_remote_daemon_connected_from_diag_info(std::shared_ptr<const tools::proxy_diagnostic_info> diag_info)
  {
    if (diag_info->is_busy)
      return false;
    if (time(nullptr) - diag_info->last_success_interract_time > DAEMON_IDLE_UPDATE_TIME_MS * 2)
      return false;
    return true;
  }

  inline std::string get_seed_phrase_info(const std::string& seed_phrase, const std::string& seed_password, view::seed_phrase_info& result)
  {
    //cut the last timestamp word from restore_dats
    try
    {
      //restore_from_tracking_seed
      bool is_tracking = currency::account_base::is_seed_tracking(seed_phrase);
      if (is_tracking)
      {
        currency::account_base acc;
        result.require_password = false;
        result.hash_sum_matched = false;
        result.syntax_correct = acc.restore_from_tracking_seed(seed_phrase);
        if (result.syntax_correct)
        {
          result.tracking = true;
          result.address = acc.get_public_address_str();
        }
      }
      else
      {
        result.syntax_correct = currency::account_base::is_seed_password_protected(seed_phrase, result.require_password);
        if (result.syntax_correct )
        {
          if (result.require_password)
          {
            if (seed_password.size())
            {
              currency::account_base acc;
              result.hash_sum_matched = acc.restore_from_seed_phrase(seed_phrase, seed_password);
              if (result.hash_sum_matched)
              {
                result.address = acc.get_public_address_str();
              }
            }
            else
            {
              result.hash_sum_matched = false;
            }
          }
          else
          {
            currency::account_base acc;
            result.syntax_correct = acc.restore_from_seed_phrase(seed_phrase, "");
            if (result.syntax_correct)
            {
              result.address = acc.get_public_address_str();
            }            
          }
        }
      }
      return API_RETURN_CODE_OK;
    }
    catch (...)
    {
      result.syntax_correct = false;
      return API_RETURN_CODE_OK;
    }
  }
}