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

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "include_base_utils.h"


namespace epee
{

  /*
  Recursive reader-writer mutex.

  Ownership is tracked per thread, so a thread may acquire the mutex again in any combination and release it in any order:
    exclusive -> exclusive : recursion
    shared    -> shared    : recursion, never waits (even if a writer is waiting, otherwise it would be a deadlock)
    exclusive -> shared    : allowed; if the exclusive ownership is released first, the thread keeps the shared one
    shared    -> exclusive : upgrade, the thread waits until all other threads release their shared ownership;
                             when it releases the exclusive ownership it still has its shared one

  The upgrade is atomic: nobody can change the protected data between the moment the thread acquired shared ownership and
  the moment it got the exclusive one. That is the reason for the only restriction: if two threads having shared ownership
  upgrade at the same time, each of them needs the other one to release it first. Instead of a deadlock the second thread
  gets an exception from lock() (try_lock() returns false) and keeps its shared ownership.

  Writers go first: a thread that has no ownership yet can't acquire the shared one while a writer is waiting.
  */
  class shared_recursive_mutex
  {
    // threads that have shared ownership and how many times each of them has acquired it;
    // there are only a few of them at a time, so a vector is faster than a map and needs no allocation per lock
    typedef std::vector<std::pair<std::thread::id, size_t>> readers_container;

    std::mutex m_mutex;                             // protects all the fields below
    std::condition_variable m_cv_writers;           // for threads waiting in lock()
    std::condition_variable m_cv_readers;           // for threads waiting in lock_shared()
    std::thread::id m_writer;                       // thread that has exclusive ownership, default-constructed id if none
    size_t m_writer_recursion = 0;                  // how many times m_writer has acquired exclusive ownership
    readers_container m_readers;
    size_t m_writers_waiting = 0;                   // threads waiting in lock(), an upgrading one included
    bool m_upgrade_pending = false;                 // a thread having shared ownership is waiting in lock()

  public:
    void lock()
    {
      do_lock(true);
    }

    bool try_lock()
    {
      return do_lock(false);
    }

    void unlock()
    {
      std::unique_lock<std::mutex> lk(m_mutex);
      if (m_writer != std::this_thread::get_id())
        fail(lk, "shared_recursive_mutex: unlock() is called by a thread that has no exclusive ownership");

      if (--m_writer_recursion == 0)
      {
        m_writer = std::thread::id();
        // readers can't get in while a writer is waiting, so there is no need to wake them up in that case
        if (m_writers_waiting != 0)
          m_cv_writers.notify_all();
        else
          m_cv_readers.notify_all();
      }
    }

    void lock_shared()
    {
      do_lock_shared(true);
    }

    bool try_lock_shared()
    {
      return do_lock_shared(false);
    }

    void unlock_shared()
    {
      std::unique_lock<std::mutex> lk(m_mutex);
      auto it = find_reader(std::this_thread::get_id());
      if (it == m_readers.end())
        fail(lk, "shared_recursive_mutex: unlock_shared() is called by a thread that has no shared ownership");

      if (--it->second == 0)
      {
        *it = m_readers.back();
        m_readers.pop_back();
        if (m_writers_waiting != 0)
          m_cv_writers.notify_all();
      }
    }

    // true if the calling thread has shared ownership (exclusive ownership alone doesn't count);
    // used to check the lock order against other locks, see basic_db_accessor::begin_transaction()
    bool is_locked_shared_by_this_thread()
    {
      std::lock_guard<std::mutex> lk(m_mutex);
      return find_reader(std::this_thread::get_id()) != m_readers.end();
    }

  private:
    bool do_lock(bool wait)
    {
      std::unique_lock<std::mutex> lk(m_mutex);
      const std::thread::id this_thread = std::this_thread::get_id();
      if (m_writer == this_thread)
      {
        ++m_writer_recursion;
        return true;
      }

      // a thread that already has shared ownership upgrades it, so it waits for the other readers only
      const bool upgrade = find_reader(this_thread) != m_readers.end();
      auto is_free = [&]() { return m_writer == std::thread::id() && m_readers.size() == (upgrade ? 1u : 0u); };
      if (!is_free())
      {
        if (!wait)
          return false;

        if (upgrade)
        {
          if (m_upgrade_pending)
            fail(lk, "shared_recursive_mutex: two threads are upgrading shared ownership to exclusive at the same time, it would be a deadlock");
          m_upgrade_pending = true;
        }

        ++m_writers_waiting;
        m_cv_writers.wait(lk, is_free);
        --m_writers_waiting;

        if (upgrade)
          m_upgrade_pending = false;
      }

      m_writer = this_thread;
      m_writer_recursion = 1;
      return true;
    }

    bool do_lock_shared(bool wait)
    {
      std::unique_lock<std::mutex> lk(m_mutex);
      const std::thread::id this_thread = std::this_thread::get_id();
      auto it = find_reader(this_thread);
      if (it != m_readers.end())
      {
        ++it->second;
        return true;
      }

      if (m_writer != this_thread)
      {
        auto is_free = [&]() { return m_writer == std::thread::id() && m_writers_waiting == 0; };
        if (!is_free())
        {
          if (!wait)
            return false;

          m_cv_readers.wait(lk, is_free);
        }
      }

      m_readers.emplace_back(this_thread, 1);
      return true;
    }

    readers_container::iterator find_reader(std::thread::id id)
    {
      return std::find_if(m_readers.begin(), m_readers.end(), [&](const readers_container::value_type& r) { return r.first == id; });
    }

    // misuse is reported with an exception, the state of the mutex is left intact
    [[noreturn]] static void fail(std::unique_lock<std::mutex>& lk, const char* message)
    {
      lk.unlock();
      ASSERT_MES_AND_THROW(message);
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
