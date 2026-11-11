// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <stdint.h>
#include <atomic>
#include <time.h>

// core time helper
struct test_core_time
{
  static void init()
  {
    m_time_shift = 0;
  }
  static uint64_t get_time()
  {
    return time(NULL) + m_time_shift.load(std::memory_order_relaxed);
  }
  static void adjust(uint64_t adjustment, bool relative = false)
  {
    if (relative)
    {
      m_time_shift.store(adjustment, std::memory_order_relaxed);
      return;
    }
    m_time_shift.store(adjustment - time(NULL), std::memory_order_relaxed);
  }
  static std::atomic<int64_t> m_time_shift;
};

