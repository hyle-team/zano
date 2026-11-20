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
    const uint64_t frozen_time = m_frozen_time.load(std::memory_order_relaxed);
    if (frozen_time != unfrozen)
      return frozen_time;
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

  class scoped_time
  {
  public:
    explicit scoped_time(uint64_t timestamp)
      : m_previous_time(m_frozen_time.exchange(timestamp, std::memory_order_relaxed))
    {}

    ~scoped_time()
    {
      set_time(m_previous_time);
    }

    scoped_time(const scoped_time&) = delete;
    scoped_time& operator=(const scoped_time&) = delete;

    void set_time(uint64_t timestamp)
    {
      m_frozen_time.store(timestamp, std::memory_order_relaxed);
    }

  private:
    const uint64_t m_previous_time;
  };

private:
  static constexpr uint64_t unfrozen = UINT64_MAX;
  inline static std::atomic<uint64_t> m_frozen_time{unfrozen};
};

