// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <iostream>

#ifdef WIN32
#define NOMINMAX // to suppress min and max macros in windows headers
#include <windows.h>
#else
#include <sched.h>
#include <pthread.h>
#endif

void set_process_affinity(int core)
{
#if defined(WIN32)
  DWORD_PTR mask = 1;
  for (int i = 0; i < core; ++i)
  {
    mask <<= 1;
  }
  SetProcessAffinityMask(GetCurrentProcess(), core);
#elif defined(__MACH__)
//#warning set_process_affinity is not currently supported on MacOS
#else
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core, &cpuset);
  if (0 != pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset))
  {
    std::cout << "pthread_setaffinity_np - ERROR" << std::endl;
  }
#endif
}

void set_thread_high_priority()
{
#if defined(WIN32)
  SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#elif defined(__MACH__)
//#warning set_thread_high_priority is not currently supported on MacOS
#else
  pthread_attr_t attr;
  int policy = 0;
  int max_prio_for_policy = 0;

  pthread_attr_init(&attr);
  pthread_attr_getschedpolicy(&attr, &policy);
  max_prio_for_policy = sched_get_priority_max(policy);

  if (0 != pthread_setschedprio(pthread_self(), max_prio_for_policy))
  {
    std::cout << "pthread_setschedprio - ERROR" << std::endl;
  }

  pthread_attr_destroy(&attr);
#endif
}
