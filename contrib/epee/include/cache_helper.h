// Copyright (c) 2006-2016, Andrey N. Sabelnikov, www.sabelnikov.net
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
#include <map>
#include <unordered_map>
#include <list>
#include <thread>
#include "boost/optional.hpp"
#include "syncobj.h"
#include "include_base_utils.h"

namespace epee
{
  namespace misc_utils  
  {
    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    template<bool is_ordered_type, class t_key, class t_value>
    struct container_selector;

    template<class t_key, class t_value>
    struct container_selector<true, t_key, t_value>
    {
      typedef std::map<t_key, t_value > container;
    };

    template<class t_key, class t_value>
    struct container_selector<false, t_key, t_value>
    {
      typedef std::unordered_map<t_key, t_value > container;
    };

 

    template<bool is_ordered_container, typename t_key, typename t_value, uint64_t max_elements>
    class cache_base
    {
      uint64_t mac_allowed_elements;
      std::list<t_key> most_recet_acessed;
      typename container_selector<is_ordered_container, t_key, std::pair<t_value, typename std::list<t_key>::iterator> >::container data;
    protected:
      critical_section m_lock;
    public:

      cache_base() : mac_allowed_elements(max_elements)
      {}

      size_t size()
      {
        
        return data.size();
      }

      void set_max_elements(uint64_t e)
      {
        mac_allowed_elements = e;
      }

      bool get(const t_key& k, t_value& v)
      {
        CRITICAL_REGION_LOCAL(m_lock);
        auto it = data.find(k);
        if (it == data.end())
          return false;

        most_recet_acessed.splice(most_recet_acessed.begin(), most_recet_acessed, it->second.second);
        v = it->second.first;
        return true;
      }

      bool set(const t_key& k, const t_value& v)
      {
        CRITICAL_REGION_LOCAL(m_lock);
        most_recet_acessed.push_front(k);
        data[k] = std::pair<t_value, typename std::list<t_key>::iterator>(v, most_recet_acessed.begin());

        trim();
        return true;
      }

      void clear()
      {
        CRITICAL_REGION_LOCAL(m_lock);
        data.clear();
        most_recet_acessed.clear();
      }

      bool erase(const t_key& k)
      {
        CRITICAL_REGION_LOCAL(m_lock);
        auto data_it = data.find(k);
        if (data_it == data.end())
          return false;

        most_recet_acessed.erase(data_it->second.second);
        data.erase(data_it);
        return true;
      }
    protected:
      void trim()
      {
        CRITICAL_REGION_LOCAL(m_lock);
        while (most_recet_acessed.size() > mac_allowed_elements)
        {
          auto data_it = data.find(most_recet_acessed.back());
          if (data_it != data.end())
            data.erase(data_it);
          most_recet_acessed.erase(--most_recet_acessed.end());  
        }
      }
    };

    class isolation_lock
    {
    private: 
      critical_section m_my_lock;
      critical_section& m_lock;
      boost::optional<std::thread::id> m_current_writer_thread;
    public:
      isolation_lock() :m_lock(m_my_lock)
      {}

      isolation_lock(critical_section& lock) :m_lock(lock)
      {}

      template<typename res_type, typename callback_t>
      res_type isolated_access(callback_t cb) const 
      {
        CRITICAL_REGION_LOCAL(m_lock);
        if (m_current_writer_thread.is_initialized())
        {
          //has writer
          if (std::this_thread::get_id() != m_current_writer_thread.get())
          {
            // cache isolated (not allowed)
            return cb(false);
          }
        }
        //cache shared(allowed)
        return cb(true);
      }


      template<typename res_type, typename callback_t>
      res_type isolated_write_access(callback_t cb) const 
      {
        return isolated_access<res_type>([&](bool is_allowed_cache)
        {
          if (!is_allowed_cache)
          {
            ASSERT_MES_AND_THROW("internal error: writer not allowed while it's not in writer transaction");
          }
          return cb();
        });
      }

      void set_isolation_mode()
      {
        CRITICAL_REGION_LOCAL(m_lock);
        CHECK_AND_ASSERT_THROW_MES(!m_current_writer_thread.is_initialized(), "Isolation mode already enabled for cache");
        m_current_writer_thread = std::this_thread::get_id();
      }

      void reset_isolation_mode()
      {
        CRITICAL_REGION_LOCAL(m_lock);
        CHECK_AND_ASSERT_THROW_MES(m_current_writer_thread.is_initialized(), "Isolation mode already disable for cache");
        m_current_writer_thread = boost::optional<std::thread::id>();
      }
    };


    template<bool is_ordered_container, typename t_key, typename t_value, uint64_t max_elements>
    class cache_with_write_isolation : public cache_base<is_ordered_container, t_key, t_value, max_elements>
    {
      typedef cache_base<is_ordered_container, t_key, t_value, max_elements> base_class;
      isolation_lock& m_isolation;
    public:
      cache_with_write_isolation(isolation_lock& isolation) : m_isolation(isolation)
      {}

      bool get(const t_key& k, t_value& v)
      {
        return m_isolation.isolated_access<bool>([&](bool allowed_cache)
        {
          if (allowed_cache)
            return base_class::get(k, v);
          else 
            return false;
        }); 
      }

      bool set(const t_key& k, const t_value& v)
      {
        return m_isolation.isolated_access<bool>([&] (bool cache_allowed)
        {
          if (cache_allowed)
            return base_class::set(k, v); 
          return true;
        });
      }

      void clear()
      {
        m_isolation.isolated_access<bool>([&](bool cache_allowed)
        {
          if (cache_allowed)
            base_class::clear(); 
          return true;
        });

      }

      bool erase(const t_key& k)
      {
        return m_isolation.isolated_write_access<bool>([&](){return base_class::erase(k); });
      }
    };

    template<bool is_ordered_container, typename t_key, typename t_value, uint64_t max_elements>
    class cache_dummy : public cache_base<is_ordered_container, t_key, t_value, max_elements>
    {
      typedef cache_base<is_ordered_container, t_key, t_value, max_elements> base_class;
      isolation_lock& m_isolation;
    public:
      cache_dummy(isolation_lock& isolation) : m_isolation(isolation){}
      bool get(const t_key& k, t_value& v){return false;}
      bool set(const t_key& k, const t_value& v){return true;}
      void clear(){}
      bool erase(const t_key& k){return true;}
    };

  }

}
