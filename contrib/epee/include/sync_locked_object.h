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



#pragma  once

#include "syncobj.h"
#include "misc_log_ex.h"

namespace epee
{
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct default_lock_time_watching_policy
  {
    inline static void watch_lock_time(uint64_t lock_time) {}
  };

  template<typename t_object, typename lock_time_watching_policy = default_lock_time_watching_policy>
  class locked_object_proxy
  {
    uint64_t start_lock_time;
    t_object& rt;
    std::unique_lock<std::recursive_mutex> lock;
  public:
    locked_object_proxy(t_object& t, std::recursive_mutex& m) :rt(t), lock(m), start_lock_time(epee::misc_utils::get_tick_count())
    {}
    locked_object_proxy(const locked_object_proxy& lop) :rt(lop.rt), lock(*lop.lock.mutex()), start_lock_time(epee::misc_utils::get_tick_count())
    {}
    ~locked_object_proxy()
    {
      TRY_ENTRY();
      uint64_t lock_time = epee::misc_utils::get_tick_count() - start_lock_time;
      lock_time_watching_policy::watch_lock_time(lock_time);
      CATCH_ALL_DO_NOTHING();
    }

    /*
    t_object& operator()()
    {
    return rt;
    }*/
    t_object* operator ->()
    {
      return &rt;
    }
    t_object& operator *()
    {
      return rt;
    }
  };

  template<typename t_object, typename lock_time_watching_policy = default_lock_time_watching_policy>
  class locked_object
  {

    t_object t;
    mutable std::recursive_mutex m;
    template<typename t_proxy_object, typename t_proxy_lock_time_watching_policy>
    friend class locked_object_proxy;
  public:
    std::shared_ptr<locked_object_proxy<t_object, lock_time_watching_policy>> lock()
    {
      std::shared_ptr<locked_object_proxy<t_object, lock_time_watching_policy>> res;
      res.reset(new locked_object_proxy<t_object, lock_time_watching_policy>(t, m));
      return res;
    }

    std::shared_ptr<locked_object_proxy<t_object, lock_time_watching_policy>> try_lock()
    {
      std::shared_ptr<locked_object_proxy<t_object, lock_time_watching_policy>> res;
      if (CRITICAL_SECTION_TRY_LOCK(m))
      {
        res.reset(new locked_object_proxy<t_object, lock_time_watching_policy>(t, m));
        CRITICAL_SECTION_UNLOCK(m);
      }
      return res;
    }
    t_object& unlocked_get()
    {
      return t;
    }

    locked_object_proxy<t_object, lock_time_watching_policy> operator->()
    {
      return locked_object_proxy<t_object, lock_time_watching_policy>(t, m);
    }
    locked_object_proxy<t_object, lock_time_watching_policy> operator*()
    {
      return locked_object_proxy<t_object, lock_time_watching_policy>(t, m);
    }

    locked_object_proxy<const t_object, lock_time_watching_policy> operator->() const
    {
      return locked_object_proxy<const t_object, lock_time_watching_policy>(t, m);
    }

    locked_object_proxy<const t_object, lock_time_watching_policy> operator*() const
    {
      return locked_object_proxy<const t_object, lock_time_watching_policy>(t, m);
    }

    /*locked_object_proxy<t_object> operator()()
    {
    return locked_object_proxy<t_object>(t, m);
    }*/
  };
}