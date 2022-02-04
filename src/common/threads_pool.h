// Copyright (c) 2014-2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 

#include <thread>
#include <string>
#include <boost/thread.hpp>

namespace utils
{

  struct call_executor_base
  {
    virtual void execute()=0;
  };

  template<class t_executor_func>
  struct call_executor_t : public call_executor_base
  {
    call_executor_t(t_executor_func f) :m_func(f)
    {}
    t_executor_func m_func;
    virtual void execute()
    {
      m_func();
    }
  };

  template<t_executor_func> 
  std::shared_ptr<call_executor_base> build_call_executor(t_executor_func func)
  {
    std::shared_ptr<call_executor_base> res = new call_executor_t<t_executor_func>(func);
    return res;
  }

  class threads_pool
  {
    void init()
    {
      m_is_stop = false;
      int num_threads = std::thread::hardware_concurrency();
      
      for (int i = 0; i < num_threads; i++)
      {
        m_threads.push_back(std::thread(worker_func));
      }
    }

    threads_pool(): m_is_stop(false), m_threads_counter(0)
    {}
    
    template<t_executor_func>
    bool add_job(t_executor_func func)
    {
      {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        m_jobs_que.push_back(build_call_executor(func));
      }
      m_condition.notify_one();
      return true;
    }

    private:
      void worker_func()
      {
        LOG_PRINT_L0("Worker thread is started");
        while (true)
        {
          std::shared_ptr<call_executor_base> job;
          {
            std::unique_lock<std::mutex> lock(m_queue_mutex);

            m_condition.wait(lock, [this]() 
            {
              return !m_jobs_que.empty() || m_is_stop;
            });
            if (m_is_stop)
            {
              LOG_PRINT_L0("Worker thread is finished");
              return;
            }
              
            job = m_jobs_que.front();
            m_jobs_que.pop_front();
          }

          job->execute();
        }
      }



      std::list<std::shared_ptr<call_executor_base>> m_jobs_que;
      std::condition_variable m_condition;
      std::mutex m_queue_mutex;
      std::vector<std::thread> m_threads;
      atomic<bool> m_is_stop;
      std::atomic<int64_t> m_threads_counter;
  };
}