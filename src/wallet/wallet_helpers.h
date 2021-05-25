// Copyright (c) 2014-2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once


#include "wallet2.h"
#include "view_iface.h"


namespace tools
{
  inline bool get_wallet_info(wallet2& w, view::wallet_info& wi)
  {
    wi = AUTO_VAL_INIT_T(view::wallet_info);
    wi.address = w.get_account().get_public_address_str();
    wi.view_sec_key = epee::string_tools::pod_to_hex(w.get_account().get_keys().view_secret_key);
    uint64_t fake = 0;
    wi.balance = w.balance(wi.unlocked_balance, fake, fake, wi.mined_total);
    wi.path = epee::string_encoding::wstring_to_utf8(w.get_wallet_path());
    wi.is_auditable = w.is_auditable();
    wi.is_watch_only = w.is_watch_only();
    return true;
  }

  inline std::string get_seed_phrase_info(const std::string& seed_phrase, const std::string& seed_password, view::seed_phrase_info& result)
  {
    //cut the last timestamp word from restore_dats
    try{
      result.syntax_correct = currency::account_base::is_seed_password_protected(seed_phrase, result.require_password);
    }
    catch (...)
    {
      result.syntax_correct = false;
      return API_RETURN_CODE_OK;
    }

    if (result.syntax_correct && result.require_password)
    {
      if (seed_password.size())
      {
        currency::account_base acc;
        result.hash_sum_matched = acc.restore_from_seed_phrase(seed_phrase, seed_password);
      }
      else 
      {
        result.hash_sum_matched = false;
      }
    }
    if (!result.syntax_correct)
    {
      //possibly tracking wallet
      currency::account_base acc;
      result.syntax_correct = acc.restore_from_tracking_seed(seed_phrase);
      if (result.syntax_correct)
      {
        result.tracking = true;
      }
    }
    return API_RETURN_CODE_OK;
  }
}