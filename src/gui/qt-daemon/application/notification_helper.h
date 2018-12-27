// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#pragma once
#include <string>
#include "epee/include/misc_log_ex.h"

struct notification_helper
{
  static void show(const std::string& title, const std::string& message)
#if !defined(__APPLE__)
  {
    // just a stub, implemented for macOS only, see .mm
    LOG_PRINT_RED("system notifications are supported only for macOS!", LOG_LEVEL_0);
  }
#endif
  ;
};
