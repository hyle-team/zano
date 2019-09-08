// Copyright (c) 2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include <epee/include/misc_os_dependent.h>

namespace tools
{
#if defined(WIN32)
  extern std::string get_callstack_win_x64();
#endif

  inline std::string get_callstack()
  {
#if defined(__GNUC__)
    return epee::misc_utils::print_trace_default();
#elif defined(WIN32)
    return get_callstack_win_x64();
#else
    return "";
#endif
  }

} // namespace tools
