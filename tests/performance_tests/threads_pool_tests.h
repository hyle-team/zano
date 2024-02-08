// Copyright (c) 2019 The Zano developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
 
#include <string>
#include "include_base_utils.h"
#include "common/threads_pool.h"


inline
void thread_pool_tests_simple()
{
  {
    utils::threads_pool pool;
    pool.init();
    std::atomic<uint64_t> count_jobs_finished(0);
    size_t i = 0;
    for (; i != 10; i++)
    {
      pool.add_job([&, i]() {LOG_PRINT_L0("Job " << i << " started"); epee::misc_utils::sleep_no_w(10000); ++count_jobs_finished; LOG_PRINT_L0("Job " << i << " finished"); });
    }
    while (count_jobs_finished != i)
    {
      epee::misc_utils::sleep_no_w(500);
    }
    LOG_PRINT_L0("All jobs finished");
  }
  LOG_PRINT_L0("Scope left");
}

inline
void thread_pool_tests()
{
  {
    utils::threads_pool pool;
    pool.init();
    std::atomic<uint64_t> count_jobs_finished(0);

    utils::threads_pool::jobs_container jobs;
    size_t i = 0;
    for (; i != 10; i++)
    {
      utils::threads_pool::add_job_to_container(jobs, [&, i]() {LOG_PRINT_L0("Job " << i << " started"); epee::misc_utils::sleep_no_w(10000); ++count_jobs_finished; LOG_PRINT_L0("Job " << i << " finished"); });
    }

    pool.add_batch_and_wait(jobs);
    if (count_jobs_finished != i)
    {
      LOG_ERROR("Test failed");
      return;
    }
    LOG_PRINT_L0("All jobs finished");
  }
  LOG_PRINT_L0("Scope left");
}