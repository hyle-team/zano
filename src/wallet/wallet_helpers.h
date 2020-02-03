// Copyright (c) 2014-2020 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once


#include "wallet2.h"
#include "gui/qt-daemon/application/view_iface.h"


namespace tools
{
  inline bool get_wallet_info(wallet2& w, view::wallet_info& wi)
  {
    wi = AUTO_VAL_INIT_T(view::wallet_info);
    wi.address = w.get_account().get_public_address_str();
    wi.tracking_hey = epee::string_tools::pod_to_hex(w.get_account().get_keys().m_view_secret_key);
    uint64_t fake = 0;
    wi.balance = w.balance(wi.unlocked_balance, fake, fake, wi.mined_total);
    wi.path = epee::string_encoding::wstring_to_utf8(w.get_wallet_path());
    return API_RETURN_CODE_OK;
  }
}