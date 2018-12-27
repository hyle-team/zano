// Copyright (c) 2006-2017, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 



#pragma once

#include <condition_variable>
#include <atomic>
#include <mutex>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>


namespace epee
{

  template<typename t_data>
  class per_thread_data
  {
    typedef std::map<std::thread::id, t_data> container;
  public:
    std::recursive_mutex cs;
    container th_data;
  public:
    typename container::iterator get_it()
    {
      std::unique_lock<std::recursive_mutex> lk(cs);
      auto it = th_data.find(std::this_thread::get_id());
      if (it == th_data.end())
      {
        it = th_data.insert(std::make_pair(std::this_thread::get_id(), AUTO_VAL_INIT(t_data()))).first;
      }
      return it;
    }

    t_data& get()
    {
      std::unique_lock<std::recursive_mutex> lk(cs);
      return get_it()->second;
    }

    void erase(typename container::iterator it)
    {
      std::unique_lock<std::recursive_mutex> lk(cs);
      th_data.erase(it);
    }
    void erase()
    {
      std::unique_lock<std::recursive_mutex> lk(cs);
      auto it = th_data.find(std::this_thread::get_id());
      if (it != th_data.end())
        th_data.erase(it);
    }
  };


  /*
  Naive implementation of shared recursive mutex with restriction: if you already have acquired shared ownership then
  attempt to acquire exclusive ownership will cause deadlock.
  */
  class shared_recursive_mutex
  {
    struct thread_data
    {
      uint64_t recursion_count;
      bool is_writer_thread;
    };



    std::condition_variable cv;
    std::atomic<uint64_t> readers_count;
    std::recursive_mutex exclusive_lock;
    std::mutex cv_mutex;
    per_thread_data<thread_data> ptc;
  public:
    shared_recursive_mutex()
      : readers_count(0)
    {}

    ~shared_recursive_mutex()
    {}

    void lock()
    {
      exclusive_lock.lock();
      std::unique_lock<std::mutex> lk(cv_mutex);
      thread_data& thd = ptc.get();
      if (thd.recursion_count != 0 && !thd.is_writer_thread)
      {
        ASSERT_MES_AND_THROW("attempt to acquire exclusive ownership while shared ownership already owning");
      }
      thd.is_writer_thread = true;
      thd.recursion_count++;

      if (readers_count == 0)
        return;

      cv.wait(lk, [&](){return readers_count == 0; });
    }
    //  bool try_lock();
    void unlock()
    {
      auto thd_it = ptc.get_it();
      thd_it->second.recursion_count--;
      if (thd_it->second.recursion_count == 0)
        ptc.erase(thd_it);
      
      exclusive_lock.unlock();
    }
    void lock_shared()
    {
      thread_data& td = ptc.get();
      if (!td.recursion_count)
      {
        //lock exclusive lock only first entrance to reader lock
        exclusive_lock.lock();

        //always lock cv_mutex when modifying readers_count
        std::lock_guard<std::mutex> lk(cv_mutex);
        readers_count++;
      }

      td.recursion_count++;
      if (td.recursion_count == 1)
        exclusive_lock.unlock();
    }
    //  bool try_lock_shared();
    void unlock_shared()
    {
      bool do_notify = false;
      {
        //process per thread data
        auto thd_it = ptc.get_it();
        thd_it->second.recursion_count--;
        if (thd_it->second.recursion_count == 0)
        {
          {
            std::lock_guard<std::mutex> lk(cv_mutex);
            readers_count--;
          }
          if (readers_count == 0)
            do_notify = true;

          ptc.erase(thd_it);
        }
      }
      if (do_notify)
        cv.notify_all();
    }
  };



  /*
  Shared membership for mutex
  */
  template<class t_shared_mutex>
  class shared_membership
  {
    t_shared_mutex& m_mutex;
  public:
    //to make copy fake!
    shared_membership(t_shared_mutex& m) :m_mutex(m)
    {
    }
    ~shared_membership()
    {
    }
    void lock()
    {
      m_mutex.lock_shared();
    }
    void unlock()
    {
      m_mutex.unlock_shared();
    }
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  //for now ignore shared mutex stuff
  template<>
  class guarded_critical_region_t<epee::shared_recursive_mutex> : public critical_region_t<shared_recursive_mutex>
  {
  public:
    guarded_critical_region_t(epee::shared_recursive_mutex& cs, const char* /*func_name*/, const char* /*location*/, const char* /*lock_name*/, const std::string& /*thread_name*/) : critical_region_t<shared_recursive_mutex>(cs)
    {}
  };
  template<>
  class guarded_critical_region_t<shared_membership<shared_recursive_mutex> > : public critical_region_t<shared_membership<shared_recursive_mutex>>
  {
  public:
    guarded_critical_region_t(shared_membership<shared_recursive_mutex>& cs, const char* /*func_name*/, const char* /*location*/, const char* /*lock_name*/, const std::string& /*thread_name*/) : critical_region_t<epee::shared_membership<shared_recursive_mutex>>(cs)
    {}
  };


}
