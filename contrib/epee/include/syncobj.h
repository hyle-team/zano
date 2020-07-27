// Copyright (c) 2019, Zano Project
// Copyright (c) 2019, anonimal, <anonimal@zano.org>
// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
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
#include <thread>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "singleton.h"
#include "static_helpers.h"
#include "misc_helpers.h"

//#define DISABLE_DEADLOCK_GUARD


namespace epee
{

  struct simple_event
  {
    simple_event() : m_rised(false)
    {
    }

    void raise()
    {
      std::unique_lock<std::mutex> lock(m_mx);
      m_rised = true;
      m_cond_var.notify_one();
    }

    void wait()
    {
      std::unique_lock<std::mutex> lock(m_mx);
      while (!m_rised) 
        m_cond_var.wait(lock);
      m_rised = false;
    }

  private:
    std::mutex m_mx;
    std::condition_variable m_cond_var;
    bool m_rised;
  };


  class critical_region;

  class critical_section
  {
    boost::recursive_mutex m_section;

  public:
    //to make copy fake!
    critical_section(const critical_section& section)
    {
    }

    critical_section()
    {
    }

    ~critical_section()
    {
    }

    void lock()
    {
      m_section.lock();
      //EnterCriticalSection( &m_section );
    }

    void unlock()
    {
      m_section.unlock();
    }

    bool try_lock()
    {
      return m_section.try_lock();
    }

    // to make copy fake
    critical_section& operator=(const critical_section& section)
    {
      return *this;
    }
  };

  class dummy_critical_section
  {

  public:
    //to make copy fake!
    dummy_critical_section(const critical_section& section)
    {
    }
    dummy_critical_section()
    {
    }
    ~dummy_critical_section()
    {
    }
    void lock(){}
    void unlock(){}
    void lock_exclusive(){}
    void unlock_exclusive(){}

    bool tryLock(){ return true; }

    // to make copy fake
    dummy_critical_section& operator=(const dummy_critical_section& section)
    {
      return *this;
    }
  };

  


  template<class t_lock>
  class critical_region_t
  {
    t_lock&	m_locker;
    bool m_unlocked;

    critical_region_t(const critical_region_t&) {}

  public:
    critical_region_t(t_lock& cs): m_locker(cs), m_unlocked(false)
    {
      m_locker.lock();
    }

    ~critical_region_t()
    {
      unlock();
    }

    void unlock()
    {
      if (!m_unlocked)
      {
        m_locker.unlock();
        m_unlocked = true;
      }
    }
  };



#if defined(WINDWOS_PLATFORM)
  class shared_critical_section
  {
  public: 
    shared_critical_section()
    {
      ::InitializeSRWLock(&m_srw_lock);
    }
    ~shared_critical_section()
    {}

    bool lock_shared()
    {
      AcquireSRWLockShared(&m_srw_lock);
      return true;
    }
    bool unlock_shared()
    {
      ReleaseSRWLockShared(&m_srw_lock);
      return true;
    }
    bool lock_exclusive()
    {
      ::AcquireSRWLockExclusive(&m_srw_lock);
      return true;
    }
    bool unlock_exclusive()
    {
      ::ReleaseSRWLockExclusive(&m_srw_lock);
      return true;
    }
  private:
    SRWLOCK m_srw_lock;
  };


  class shared_guard
  {
  public:
    shared_guard(shared_critical_section& ref_sec):m_ref_sec(ref_sec)
    {
      m_ref_sec.lock_shared();
    }

    ~shared_guard()
    {
      m_ref_sec.unlock_shared();
    }

  private:
    shared_critical_section& m_ref_sec;
  };


  class exclusive_guard
  {
  public:
    exclusive_guard(shared_critical_section& ref_sec):m_ref_sec(ref_sec)
    {
      m_ref_sec.lock_exclusive();
    }

    ~exclusive_guard()
    {
      m_ref_sec.unlock_exclusive();
    }

  private:
    shared_critical_section& m_ref_sec;
  };

#endif





#if defined(WINDWOS_PLATFORM)
  inline const char* get_wait_for_result_as_text(DWORD res)
  {
    switch(res)
    {
    case WAIT_ABANDONED:  return "WAIT_ABANDONED";
    case WAIT_TIMEOUT:    return "WAIT_TIMEOUT";
    case WAIT_OBJECT_0:   return "WAIT_OBJECT_0";
    case WAIT_OBJECT_0+1: return "WAIT_OBJECT_1";
    case WAIT_OBJECT_0+2: return "WAIT_OBJECT_2";
    default:              return "UNKNOWN CODE";
    }
  }
#endif

#ifdef ENABLE_DLG_DEBUG_MESSAGES
#define DO_DEBUG_COUT(mes) std::cout << mes;
#else 
#define DO_DEBUG_COUT(mes) 
#endif

#define POTENTIAL_HANG_PREVENT_LIMIT 1000



  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class deadlock_guard
  {
  public:
    typedef void* lock_reference_type;
  private:
    struct thread_info
    {
      std::map<lock_reference_type, size_t> m_owned_objects;
      bool is_blocked;
      lock_reference_type blocker_lock;
      const char* block_location;
      const char* func_name;
      const char* lock_name;
      std::string thread_name;
    };

    std::recursive_mutex m_lock;

    //thread_id -> owned locks(lock_id, recursion_count)
    typedef std::map<std::thread::id, thread_info> thread_id_to_info_map;
    std::map<std::thread::id, thread_info> m_thread_owned_locks;
    //lock -> owner thread it
    std::map<lock_reference_type, thread_id_to_info_map::iterator> m_owned_locks_to_thread;
    // deadlock journal
    std::list<std::string> m_deadlock_journal;

  public:
    void on_before_lock(lock_reference_type lock, const char* func_name, const char* loction, const char* lock_name, const std::string& thread_name)
    {
      std::lock_guard<std::recursive_mutex> gd(m_lock);
      DO_DEBUG_COUT("[" << std::this_thread::get_id() << "][BEFORE_LOCK]:" << lock << std::endl);
      std::thread::id this_id = std::this_thread::get_id();
      is_lock_safe(lock, this_id, func_name, loction, lock_name, thread_name);//can return only true, if false throw exception

      thread_info& ti = m_thread_owned_locks[this_id];
      ti.is_blocked = true;
      ti.blocker_lock = lock;
      ti.block_location = loction;
      ti.func_name = func_name;
      ti.lock_name = lock_name;
      ti.thread_name = thread_name;
    }
    void on_after_lock(lock_reference_type lock)
    {
      std::lock_guard<std::recursive_mutex> gd(m_lock);
      DO_DEBUG_COUT("[" << std::this_thread::get_id() << "][AFTER_LOCK]:" << lock << std::endl);
      add_ownership(lock);
    }

    void on_unlock(lock_reference_type lock)
    {
      std::lock_guard<std::recursive_mutex> gd(m_lock);
      DO_DEBUG_COUT("[" << std::this_thread::get_id() << "][UNLOCK]:" << lock << std::endl);
      std::thread::id this_id = std::this_thread::get_id();
      auto it = m_thread_owned_locks.find(this_id);
      if (it == m_thread_owned_locks.end())
      {
        throw std::runtime_error("Internal error: unknown thread is unlocking mutex");
      }

      if (it->second.is_blocked)
      {
        throw std::runtime_error("Internal error: thread is trying to unlock while is_blocked = true");
      }
      auto ownership_it = it->second.m_owned_objects.find(lock);
      if (ownership_it == it->second.m_owned_objects.end())
      {
        throw std::runtime_error("Internal error: can't find ownership for locker on unlock");
      }

      ownership_it->second--;
      if (!ownership_it->second)
      {
        // ownership liquidation
        auto lock_to_thread_it = m_owned_locks_to_thread.find(lock);
        if (lock_to_thread_it == m_owned_locks_to_thread.end())
        {
          throw std::runtime_error("Internal error: can't find ownership for locker on unlock");
        }
        else
        {
          m_owned_locks_to_thread.erase(lock_to_thread_it);
        }
        it->second.m_owned_objects.erase(ownership_it);
        DO_DEBUG_COUT("[" << std::this_thread::get_id() << "][REMOVED_OWNERSHIP]: " << lock  << std::endl);
      }
    }

    std::string get_dlg_state()
    {
      std::lock_guard<std::recursive_mutex> gd(m_lock);
      std::stringstream ss;
      ss << "Threads lockers ownership:" << std::endl;
      for (auto& tol : m_thread_owned_locks)
      {
        ss << tol.second.thread_name << "(tid:" << tol.first << ") owned: ";
        for (auto& owned_obj: tol.second.m_owned_objects)
        {
          ss << "(" << owned_obj.first << ":" << owned_obj.second << ")";
        }
        if (tol.second.is_blocked)
        {
          ss << "BLOCKED BY: \"" << tol.second.lock_name << "\"(" << tol.second.blocker_lock << ") at " << tol.second.func_name << " @ " << tol.second.block_location;
        }
        ss << std::endl;
      }

      ss << "Lockers to threads links:" << std::endl;
      for(auto& lock_to_thread: m_owned_locks_to_thread)
      {
        ss << "locker: " << lock_to_thread.first << " owned by " << lock_to_thread.second->first << std::endl;
      }
      ss << "Deadlocks history:" << std::endl;
      for (auto err : m_deadlock_journal)
      {
        ss << "-----------------------------------------------------------------------" << std::endl << err << std::endl;
      }

      return ss.str();
    }

    std::list<std::string> get_deadlock_journal()
    {
      return m_deadlock_journal;
    }

  private:
    void add_ownership(lock_reference_type lock)
    {
      std::thread::id this_id = std::this_thread::get_id();
      auto it = m_thread_owned_locks.find(this_id);
      if (it == m_thread_owned_locks.end())
      {
        // it's ok, probably someone used try_lock and didn't call on_before_lock
        auto r = m_thread_owned_locks.insert(std::make_pair(this_id, thread_info()));
        it = r.first;
      }
      thread_info& ti = it->second;
      ti.is_blocked = false;
      auto lock_it = ti.m_owned_objects.find(lock);
      if (lock_it == ti.m_owned_objects.end())
      {
        //non-recursive ownership
        ti.m_owned_objects[lock] = 1;
        //need to add lock-to-thread reccord 
        m_owned_locks_to_thread[lock] = it;
        DO_DEBUG_COUT("[" << std::this_thread::get_id() << "][ADDED_OWNERSHIP]: " << lock << std::endl);
      }
      else
      {
        lock_it->second++;
        DO_DEBUG_COUT("[" << std::this_thread::get_id() << "][INC_OWNERSHIP]: " << lock << std::endl);
      }
    }

    bool is_lock_safe(lock_reference_type lock, const std::thread::id& this_id, const char* func_name, const char* location, const char* lock_name, const std::string& thread_name)
    {
      //lookup if this lock is already owned by someone
      auto it_th_owned_locks = m_thread_owned_locks.find(this_id);
      if (it_th_owned_locks == m_thread_owned_locks.end())
      {
        return true;
      }
      //we already own some locks, let's figure out if this lock is owned by someone 
      auto it_to_owner_thread = m_owned_locks_to_thread.find(lock);
      if (it_to_owner_thread == m_owned_locks_to_thread.end() || it_to_owner_thread->second->first == this_id)
      {
        //lock is free or owned by this thread
        return true;
      }
      std::list<decltype(it_to_owner_thread->second)> threads_chain;
      threads_chain.push_back(it_to_owner_thread->second);

      //lookup loop
      while (true)
      {
        if (!it_to_owner_thread->second->second.is_blocked)
        {
          return true;
        }
        if (it_to_owner_thread->second->second.m_owned_objects.count(it_to_owner_thread->second->second.blocker_lock))
        {
          //recursive lock is happening
          return true;
        }

        auto new_it_to_owner_thread = m_owned_locks_to_thread.find(it_to_owner_thread->second->second.blocker_lock); 
        if (new_it_to_owner_thread == m_owned_locks_to_thread.end())
        {
          //this could happen when on_before_lock already worked and on_after_lock not called yet, but mutex itself is free, 
          //still theoretically possible deadlock that can be missed(or not ?)
          return true;
        }
        if (it_to_owner_thread == new_it_to_owner_thread)
        {
          //newer should happen 
          throw std::runtime_error("Internal error: thread referred to itself, despite the the check in m_owned_locks_to_thread.find(it_to_owner_thread->second->second.blocker_lock);  ");
        }
        it_to_owner_thread = new_it_to_owner_thread;

        threads_chain.push_back(it_to_owner_thread->second);
        if (threads_chain.size() > POTENTIAL_HANG_PREVENT_LIMIT)
        {
          throw std::runtime_error("Internal error: threads_chain size is too big, endless loop detected");
        }
        if (it_to_owner_thread->second->first == this_id)
        {
          //got deadlock!
          std::stringstream ss;
          ss << "Deadlock detected at: " << std::endl << std::endl;
          auto prev_it = m_thread_owned_locks.end();
          auto current_it = *threads_chain.begin();
          for (auto it = threads_chain.begin(); it != threads_chain.end(); it++)
          {
            current_it = *it;
            if (prev_it == m_thread_owned_locks.end())
            {
              prev_it = current_it;
              continue;
            }
            ss << prev_it->second.thread_name << "(tid:" << prev_it->first << ") blocked by locker \"" << prev_it->second.lock_name
              << "\"(owned by " << current_it->second.thread_name << " tid:" << current_it->first << ")] at "
              << prev_it->second.func_name << " @ " << prev_it->second.block_location << std::endl << "    |" << std::endl << "    V" << std::endl;
            prev_it = current_it;
          }
          if (prev_it != m_thread_owned_locks.end())
          {
            ss << prev_it->second.thread_name << "(tid:" << prev_it->first << ")  blocked by locker \"" << lock_name << "(owned by " << (*threads_chain.begin())->second.thread_name << " tid:" << (*threads_chain.begin())->first << ")] at "
              << func_name << " @ " << location << std::endl;
          }
          m_deadlock_journal.push_back(ss.str());
          throw std::runtime_error(ss.str());
        }

      }
    }
  };

  //const static initializer<abstract_singleton<deadlock_guard> > singleton_initializer;

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class deadlock_guard_singleton
  {
  public:
    static void on_before_lock(deadlock_guard::lock_reference_type lock, const char* func_name, const char* loction, const char* lock_name, const std::string& thread_name)
    {
      auto dlg_ptr = abstract_singleton<deadlock_guard>::get_instance();
      if (dlg_ptr)
        dlg_ptr->on_before_lock(lock, func_name, loction, lock_name, thread_name);
    }
    static void on_after_lock(deadlock_guard::lock_reference_type lock)
    {
      auto dlg_ptr = abstract_singleton<deadlock_guard>::get_instance();
      if (dlg_ptr)
        dlg_ptr->on_after_lock(lock);
    }
    static void on_unlock(deadlock_guard::lock_reference_type lock)
    {
      auto dlg_ptr = abstract_singleton<deadlock_guard>::get_instance();
      if (dlg_ptr)
        dlg_ptr->on_unlock(lock);
    }

    static std::string get_dlg_state()
    {
      auto dlg_ptr = abstract_singleton<deadlock_guard>::get_instance();
      if (dlg_ptr)
        return dlg_ptr->get_dlg_state();
      return "";
    }

    static std::list<std::string> get_deadlock_journal()
    {
      auto dlg_ptr = abstract_singleton<deadlock_guard>::get_instance();
      if (dlg_ptr)
        return dlg_ptr->get_deadlock_journal();
      return std::list<std::string>();
    }
  };


  
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

  template<class t_lock>
  class guarded_critical_region_t
  {
    t_lock&	m_locker;
    std::atomic<bool> m_unlocked;

    guarded_critical_region_t(const guarded_critical_region_t&) {}

  public:
    guarded_critical_region_t(t_lock& cs, const char* func_name, const char* location, const char* lock_name, const std::string& thread_name) : m_locker(cs), m_unlocked(false)
    {
      deadlock_guard_singleton::on_before_lock(&m_locker, func_name, location, lock_name, thread_name);
      m_locker.lock();
      deadlock_guard_singleton::on_after_lock(&m_locker);
    }

    ~guarded_critical_region_t()
    {
      TRY_ENTRY();
      unlock();
      CATCH_ALL_DO_NOTHING();
    }

    void unlock()
    {
      if (!m_unlocked)
      {
        deadlock_guard_singleton::on_unlock(&m_locker);
        m_locker.unlock();        
        m_unlocked = true;
      }
    }
  };

  template<class locker_t>
  inline bool dlg_try_lock_locker(locker_t& lock)
  {
    bool res = lock.try_lock();
    if (res)
    {
      deadlock_guard_singleton::on_after_lock(&lock);
    }
    return res;
  }



#define _DEADLOCK_STRINGIFY(X) #X
#define DEADLOCK_STRINGIFY(X) _DEADLOCK_STRINGIFY(X)

#if defined(_MSC_VER)
#define DEADLOCK_FUNCTION_DEF __FUNCTION__
#else
#define DEADLOCK_FUNCTION_DEF __PRETTY_FUNCTION__ 
#endif 

#define DEADLOCK_LOCATION __FILE__ ":" DEADLOCK_STRINGIFY(__LINE__)

  /*

    We do DEADLOCK_LOCATION and DEADLOCK_FUNCTION_DEF as separate variables passed into deadlock_guard
    because in GCC __PRETTY_FUNCTION__ is not a literal (like __FILE__ macro) but const variable, and
    can't concatenate it with other macro like we did in DEADLOCK_LOCATION.

  */

#define VALIDATE_MUTEX_IS_FREE(mutex_mame)    \
  if (mutex_mame.try_lock()) \
  { \
    mutex_mame.unlock(); \
    return; \
  } \
  else \
  { \
    auto state_str = epee::deadlock_guard_singleton::get_dlg_state(); \
    LOG_ERROR("MUTEX IS NOT FREE ON DESTRUCTOR: " << #mutex_mame << ", address:" << (void*)&mutex_mame << ENDL << "DEAD LOCK GUARD state:" << ENDL << state_str); \
  }



#define DLG_CRITICAL_REGION_LOCAL_VAR(lock, varname)     epee::guarded_critical_region_t<decltype(lock)>   varname(lock, DEADLOCK_FUNCTION_DEF, DEADLOCK_LOCATION, #lock, epee::log_space::log_singletone::get_thread_log_prefix())
#define DLG_CRITICAL_REGION_BEGIN_VAR(lock, varname)   { epee::guarded_critical_region_t<decltype(lock)>   varname(lock, DEADLOCK_FUNCTION_DEF, DEADLOCK_LOCATION, #lock, epee::log_space::log_singletone::get_thread_log_prefix())

#define DLG_CRITICAL_SECTION_LOCK(lck)     epee::deadlock_guard_singleton::on_before_lock(&lck, DEADLOCK_FUNCTION_DEF, DEADLOCK_LOCATION, #lck, epee::log_space::log_singletone::get_thread_log_prefix());\
                                            lck.lock();\
                                            epee::deadlock_guard_singleton::on_after_lock(&lck)

#define DLG_CRITICAL_SECTION_TRY_LOCK(lock) dlg_try_lock_locker(lock)

#define DLG_CRITICAL_SECTION_UNLOCK(lock)   epee::deadlock_guard_singleton::on_unlock(&lock);\
                                            lock.unlock();
                                            


#define  FAST_CRITICAL_REGION_LOCAL_VAR(x, varname)  epee::critical_region_t<decltype(x)>   varname(x)
#define  FAST_CRITICAL_REGION_BEGIN_VAR(x, varname) {epee::critical_region_t<decltype(x)>   varname(x)
#define  FAST_CRITICAL_REGION_LOCAL(x)               FAST_CRITICAL_REGION_LOCAL_VAR(x, critical_region_var)
#define  FAST_CRITICAL_REGION_BEGIN(x)               FAST_CRITICAL_REGION_BEGIN_VAR(x, critical_region_var)
#define  FAST_CRITICAL_REGION_END()                }

#define  FAST_CRITICAL_SECTION_LOCK(x)               x.lock()
#define  FAST_CRITICAL_SECTION_TRY_LOCK(x)           x.try_lock()
#define  FAST_CRITICAL_SECTION_UNLOCK(x)             x.unlock()


//#define DISABLE_DEADLOCK_GUARD

#ifdef DISABLE_DEADLOCK_GUARD
#define  CRITICAL_REGION_LOCAL_VAR(x, varname)  FAST_CRITICAL_REGION_LOCAL_VAR(x, varname)
#define  CRITICAL_REGION_BEGIN_VAR(x, varname)  FAST_CRITICAL_REGION_BEGIN_VAR(x, varname)

#define  CRITICAL_SECTION_LOCK(x)               FAST_CRITICAL_SECTION_LOCK(x)
#define  CRITICAL_SECTION_TRY_LOCK(x)           FAST_CRITICAL_SECTION_TRY_LOCK(x)
#define  CRITICAL_SECTION_UNLOCK(x)             FAST_CRITICAL_SECTION_UNLOCK(x)
#else
#define  CRITICAL_REGION_LOCAL_VAR(x, varname)  DLG_CRITICAL_REGION_LOCAL_VAR(x, varname)
#define  CRITICAL_REGION_BEGIN_VAR(x, varname)  DLG_CRITICAL_REGION_BEGIN_VAR(x, varname)

#define  CRITICAL_SECTION_LOCK(x)               DLG_CRITICAL_SECTION_LOCK(x)
#define  CRITICAL_SECTION_TRY_LOCK(x)           DLG_CRITICAL_SECTION_TRY_LOCK(x)
#define  CRITICAL_SECTION_UNLOCK(x)             DLG_CRITICAL_SECTION_UNLOCK(x)
#endif


#define  CRITICAL_REGION_LOCAL(x)               CRITICAL_REGION_LOCAL_VAR(x, critical_region_var)
#define  CRITICAL_REGION_LOCAL1(x)              CRITICAL_REGION_LOCAL_VAR(x, critical_region_var1)
#define  CRITICAL_REGION_BEGIN(x)               CRITICAL_REGION_BEGIN_VAR(x, critical_region_var)
#define  CRITICAL_REGION_BEGIN1(x)              CRITICAL_REGION_BEGIN_VAR(x, critical_region_var1)
#define  CRITICAL_REGION_END()                  }


#define  CIRITCAL_OPERATION(obj,op) {obj##_lock.lock();obj . op;obj##_lock.unlock();}

#define  SHARED_CRITICAL_REGION_LOCAL(x) boost::shared_lock< boost::shared_mutex > critical_region_var(x)
#define  EXCLUSIVE_CRITICAL_REGION_LOCAL(x) boost::unique_lock< boost::shared_mutex > critical_region_var(x)

#define  SHARED_CRITICAL_REGION_BEGIN(x) { SHARED_CRITICAL_REGION_LOCAL(x)
#define  SHARED_CRITICAL_REGION_END() }
#define  EXCLUSIVE_CRITICAL_REGION_BEGIN(x) { EXCLUSIVE_CRITICAL_REGION_LOCAL(x)
#define  EXCLUSIVE_CRITICAL_REGION_END() }

}
