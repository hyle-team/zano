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
#include <limits>
#include <set>
#include <iterator>
#include <algorithm>
#include <functional>
#include <boost/thread.hpp>
#include <boost/any.hpp>
#include "include_base_utils.h"
#include "auto_val_init.h"


#define MARK_AS_POD_C11(type)   \
namespace std  \
{  \
  template<> \
struct is_pod< type > \
  { \
  static const bool value = true; \
  }; \
}


namespace epee
{
#define STD_TRY_BEGIN() try {

#define STD_TRY_CATCH(where_, ret_val) \
	} \
	catch (const std::exception  &e) \
	{ \
		LOG_ERROR("EXCEPTION: " << where_  << ", mes: "<< e.what());  \
		return ret_val; \
	} \
	catch (...) \
	{ \
		LOG_ERROR("EXCEPTION: " << where_ ); \
		return ret_val; \
	}


#define STD_TRY_CATCH_LOCATION(return_val) STD_TRY_CATCH(LOCATION_SS, return_val)

  /* helper class, to make able get namespace via decltype()::*/
  template<class base_class>
  class namespace_accessor: public base_class{};


#define STRINGIFY_EXPAND(s) STRINGIFY(s)
#define STRINGIFY(s) #s



namespace misc_utils
{

  template<typename t_type_a, typename t_type_b>
  void cast_assign_a_to_b(const t_type_a& a, t_type_b& b)
  {
    *static_cast<t_type_a*>(&b) = a;
  }

  template<class _Ty1,
    class _Ty2,
    class _Ty3>
    struct triple
  {	// store a pair of values
    typedef _Ty1 first_type;
    typedef _Ty2 second_type;
    typedef _Ty3 third_type;

    triple()
      : first(), second(), third()
    {	// default construct
    }

    triple(const _Ty1& _Val1, const _Ty2& _Val2, const _Ty3& _Val3)
      : first(_Val1), second(_Val2), third(_Val3)
    {	// construct from specified values
    }
  
    _Ty1 first;		// the first stored value
    _Ty2 second;	// the second stored value
    _Ty3 third;	// the second stored value
  };



	template<typename t_type>
		t_type get_max_t_val(t_type t)
		{
			return (std::numeric_limits<t_type>::max)();
		}

  template<class _Ty>
  bool is_less_as_pod(const _Ty& _Left, const _Ty& _Right)
  {	// apply operator< to operands
      return memcmp(&_Left, &_Right, sizeof(_Left)) < 0;
  }
	

	inline
	bool sleep_no_w(long ms )
	{
		boost::this_thread::sleep( 
			boost::get_system_time() + 
			boost::posix_time::milliseconds( std::max<long>(ms,0) ) );
		
		return true;
	}
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  template<typename key, typename associated_data>
  class median_helper
  {
    typedef std::multiset<key> ordered_items_container;
    typedef std::pair<typename ordered_items_container::iterator, associated_data> queued_item_type;
    typedef std::list<queued_item_type> queued_items_container;
    typedef std::list<std::pair<key, associated_data> > recent_items_container;
    ordered_items_container ordered_items;
    queued_items_container  queued_items;
    recent_items_container  recent_items; // this needed to have chance to roll back at least for some depth, so we keep elements removed from median window for a while
    bool had_been_in_recent_items;
    mutable critical_section m_lock;

    typename ordered_items_container::iterator m_current_median_it;
    uint64_t current_index;

    virtual void handle_purge_back_item() {}
    virtual void handle_add_front_item(const key& k, const associated_data& ad) {}
    virtual void handle_remove_front_item(const key& k) {}
    void push_item_as_recent(const key& k, const associated_data& ad)
    {
      CRITICAL_REGION_LOCAL(m_lock);
      auto it = ordered_items.insert(k);
      queued_items.push_front(queued_item_type(it, ad));
    }
  public:
    median_helper() :had_been_in_recent_items(false)
    {
      m_current_median_it = ordered_items.end();
      current_index = 0;
    }
    void clear()
    {
      CRITICAL_REGION_LOCAL(m_lock);
      ordered_items.clear();
      queued_items.clear();
      recent_items.clear();
      had_been_in_recent_items = false;
      m_current_median_it = ordered_items.end();
      current_index = 0;
    }

    void push_item(const key& k, const associated_data& ad)
    {
      CRITICAL_REGION_LOCAL(m_lock);
      auto it = ordered_items.insert(k);
      queued_items.push_back(queued_item_type(it, ad));
      handle_add_front_item(k, ad);
    }

    void pop_item()
    {      
      CRITICAL_REGION_LOCAL(m_lock);
      if (!queued_items.empty())
      {
        auto it = queued_items.back().first;
        handle_remove_front_item(*it);
        ordered_items.erase(it);
        queued_items.pop_back();
      }
    }

    /*
    cb(key, associated_data) - enumerating while result is false
    when returned true - clean items behind and move it to recent_items

    cb_final_remove(key, associated_data) - for every element in recent_items if returned false - element removed forever
    */
    template<class cb_t, class cb_t2>
    bool scan_items(cb_t cb, cb_t2 cb_final_remove)
    {
      CRITICAL_REGION_LOCAL(m_lock);
      //process median window items
      for (auto it = queued_items.begin(); it != queued_items.end(); it++)
      {
        if (cb(*it->first, it->second))
        {
          //stop element, remove all items before this
          for (auto clean_it = queued_items.begin(); clean_it != it; clean_it++)
          {
            //std::pair<key, associated_data> p(*it->first, it->second);
            recent_items.push_back(std::make_pair(*clean_it->first, clean_it->second));
            had_been_in_recent_items = true;
            ordered_items.erase(clean_it->first);
          }
          queued_items.erase(queued_items.begin(), it);
          break;
        }
      }
      //process recent items front part (put back to median window set if needed)
      for (auto it = recent_items.rbegin(); it != recent_items.rend();)
      {
        if (!cb(it->first, it->second))
          break;//element should stay in recent

        //element should be back to median window
        this->push_item_as_recent(it->first, it->second);          
        it = typename recent_items_container::reverse_iterator(recent_items.erase((++it).base())); // remove *it and advance 'it' by 1
      }
      
      
      //process recent items back part (remove outdated elements)
      while (recent_items.size() && !cb_final_remove(recent_items.begin()->first, recent_items.begin()->second))
      {
        recent_items.erase(recent_items.begin());
        handle_purge_back_item();
      }

      //detect if recent_items exhausted
      if (!recent_items.size() && had_been_in_recent_items)
      {
        LOG_PRINT_RED_L0("[MEDIAN_HELPER]: recent_items exhausted, need to rebuild median from scratch");
        return false;
      }
      return true;
    }

    uint64_t get_median() const 
    {
      CRITICAL_REGION_LOCAL(m_lock);
      if (!ordered_items.size())
        return 0;

      size_t n = ordered_items.size() / 2;
      //std::sort(v.begin(), v.end());

      auto n_it = std::next(ordered_items.begin(), n);

      //nth_element(v.begin(), v.begin()+n-1, v.end());
      if (ordered_items.size() % 2)
      {//1, 3, 5...
        return *n_it;
      }
      else
      {//2, 4, 6...
        auto n_it_sub = n_it;
        --n_it_sub;
        return (*n_it_sub + *n_it) / 2;
      }
    }

    uint64_t get_avg() const
    {
      CRITICAL_REGION_LOCAL(m_lock);
      if (!queued_items.size())
        return 0;

      uint64_t summ = 0;
      for (const auto& item : queued_items)
      {
        summ += *item.first;
      }

      return summ / queued_items.size();      
    }

    template<typename key_t, typename associated_data_t>
    friend std::ostream & operator<< (std::ostream &out, median_helper<key_t, associated_data_t> const &mh);
  }; // class median_helper

  template<typename key_t, typename associated_data_t>
  std::ostream & operator<< (std::ostream &s, median_helper<key_t, associated_data_t> const &mh) 
  {
    s << "median_helper<" << typeid(key_t).name() << ", " << typeid(associated_data_t).name() << "> instance 0x" << &mh << ENDL
      << "  ordered_items:            " << mh.ordered_items.size() << ENDL
      << "  queued_items:             " << mh.queued_items.size() << ENDL
      << "  recent_items:             " << mh.recent_items.size() << ENDL
      << "  had_been_in_recent_items: " << mh.had_been_in_recent_items << ENDL;
    return s;
  }

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  template<typename container_t>
  typename container_t::value_type median(container_t &v)
  {
    typename container_t::value_type median{};
    if(v.empty())
      return median;
    if(v.size() == 1)
      return v[0];

    auto median_it = v.begin() + v.size() / 2;
    std::nth_element(v.begin(), median_it, v.end());
    median = *median_it;
    if (v.size() % 2 == 0)
    {
      auto max_it = std::max_element(v.begin(), median_it); // it's garanteed that after nth_element() the necessary element is in this interval
      median = (median + *max_it) / 2;                      // average of [size/2-1] and [size/2] elements
    }

    return median;
  }

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

  struct call_befor_die_base
  {
    virtual ~call_befor_die_base(){}
  };

  typedef boost::shared_ptr<call_befor_die_base> auto_scope_leave_caller;


  template<class t_scope_leave_handler>
  struct call_befor_die: public call_befor_die_base
  {
    t_scope_leave_handler m_func;
    call_befor_die(t_scope_leave_handler f):m_func(f)
    {}
    ~call_befor_die()
    {
      NESTED_TRY_ENTRY();

      m_func();

      NESTED_CATCH_ENTRY(__func__);
    }
  };

  template<class t_scope_leave_handler>
  auto_scope_leave_caller create_scope_leave_handler(t_scope_leave_handler f)
  {
    auto_scope_leave_caller slc(new call_befor_die<t_scope_leave_handler>(f));
    return slc;
  }


#define ON_EXIT misc_utils::auto_scope_leave_caller scope_exit_handler = misc_utils::create_scope_leave_handler


    template< typename t_contaner, typename t_redicate>
    void erase_if( t_contaner& items, const t_redicate& predicate ) 
    {
      for(auto it = items.begin(); it != items.end(); ) 
      {
        if( predicate(*it) ) 
          it = items.erase(it);
        else 
          ++it;
      }
    };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct call_basic
  {
    virtual void do_call(){};
  };
  
  template<typename param_t>
  struct call_basic_param
  {
    virtual void do_call(param_t& p) {};
  };
  
  template<typename t_callback>
  struct call_specific: public call_basic
  {
    call_specific(t_callback cb):m_cb(cb)
    {}
    virtual void do_call()
    {
      m_cb();
    }
  private:
    t_callback m_cb;
  };
  
  template<typename param_t, typename t_callback>
  struct call_specific_param : public call_basic_param<param_t>
  {
    call_specific_param(t_callback cb) :m_cb(cb)
    {}
    virtual void do_call(const param_t& p)
    {
      m_cb(p);
    }
  private:
    t_callback m_cb;
  };



  template<typename t_callback>
  auto build_abstract_callback(t_callback cb) -> std::shared_ptr<call_basic>
  {
    return std::shared_ptr<call_basic>(new call_specific<t_callback>(cb));
  }


  template<typename param_t, typename t_callback>
  auto build_abstract_callback_param(t_callback cb) -> std::shared_ptr<call_basic_param<param_t>>
  {
    return std::shared_ptr<call_basic_param<param_t>>(new call_specific_param<param_t, t_callback>(cb));
  }

  
  
  template<class callback_type>
  bool static_initializer(callback_type cb)
  {
    return cb();
  };
  
  
  template<class t_container_type>
  typename t_container_type::mapped_type& get_or_insert_value_initialized(t_container_type& container, const typename t_container_type::key_type& key)
  {
    auto it = container.find(key);
    if (it != container.end())
    {
      return it->second;
    }
    
    auto res = container.insert(typename t_container_type::value_type(key, AUTO_VAL_INIT(typename t_container_type::mapped_type())));
    return res.first->second;
  }

  template<class t_container_type>
  typename t_container_type::iterator it_get_or_insert_value_initialized(t_container_type& container, const typename t_container_type::key_type& key)
  {
    auto it = container.find(key);
    if (it != container.end())
    {
      return it;
    }

    auto res = container.insert(typename t_container_type::value_type(key, AUTO_VAL_INIT(typename t_container_type::mapped_type())));
    return res.first;
  }


  class events_dispatcher
  {

  public:

    template<typename param_t>
    struct callback_entry
    {
      std::shared_ptr<epee::misc_utils::call_basic_param<const param_t> > m_cb;
    };

    std::map<std::type_index, boost::any> m_callbacks;

    template<typename param_t, typename callback_t>
    void SUBSCIRBE_DEBUG_EVENT(callback_t cb)
    {
      std::type_index ti = typeid(param_t);
      auto it = m_callbacks.find(ti);
      if (it != m_callbacks.end())
      {
        throw std::runtime_error("Handler for this type already registered");
      }

      callback_entry<const param_t> cb_entry = { epee::misc_utils::build_abstract_callback_param<const param_t>(cb) };

      m_callbacks[ti] = cb_entry;
    }

    template<typename param_t>
    void UNSUBSCRIBE_DEBUG_EVENT()
    {
      std::type_index ti = typeid(param_t);
      auto it = m_callbacks.find(ti);
      if (it != m_callbacks.end())
      {
        m_callbacks.erase(it);
      }
    }

    void UNSUBSCRIBE_ALL()
    {
      m_callbacks.clear();
    }

    template<typename param_t>
    void RAISE_DEBUG_EVENT(const param_t& p)
    {
      std::type_index ti = typeid(param_t);
      auto it = m_callbacks.find(ti);
      if (it != m_callbacks.end())
      {
        callback_entry<const param_t >* pcallback_entry = boost::any_cast<callback_entry<const param_t >>(&it->second);
        if (!pcallback_entry)
        {
          throw std::runtime_error("Unexpected error: registered tipe holding something else in boost::eny");
        }
        pcallback_entry->m_cb->do_call(p);
      }
    }

  };

  template<typename key, typename expiration_type>
  struct expirating_set
  {
    typedef std::set<key> main_set;
    main_set m_set;
    std::multimap<expiration_type, typename main_set::iterator> m_expirations;

    const main_set& get_set()
    {
      return m_set;
    }
    void add(const key& k, const expiration_type& e)
    {
      auto res = m_set.insert(k);
      m_expirations.insert({ e, res.first });
    }

    void remove_if_expiration_less_than(const expiration_type& e)
    {
      while(m_expirations.size() && m_expirations.begin()->first < e)
      {
        m_set.erase(m_expirations.begin()->second);
        m_expirations.erase(m_expirations.begin());
      }
    }
  };

  template<typename key, typename expiration_type, typename value_type>
  struct expirating_map
  {
    typedef std::map<key, value_type> main_map;
    main_map m_map;
    std::multimap<expiration_type, typename main_map::iterator> m_expirations;

    const main_map& get_map()
    {
      return m_map;
    }
    void add(const key& k, const value_type& v, const expiration_type& e)
    {
      auto res = m_map.insert(k, v);
      m_expirations.insert({ e, res.first });
    }

    void remove_if_expiration_less_than(const expiration_type& e)
    {
      while (m_expirations.size() && m_expirations.begin()->first < e)
      {
        m_map.erase(m_expirations.begin()->second);
        m_expirations.erase(m_expirations.begin());
      }
    }

    template <class t_archive>
    inline void serialize(t_archive& a, const unsigned int ver)
    {
      std::vector<std::tuple<key, value_type, expiration_type> > items;
      if constexpr (t_archive::is_saving::value)
      {
        for (const auto& item: m_expirations)
        {
          items.resize(items.size + 1);
          std::get<2>(items.back()) = item.first;
          std::get<0>(items.back()) = item.second.first;
          std::get<1>(items.back()) = item.second.second;
        }
      }
      a & items;

      if constexpr (!t_archive::is_saving::value)
      {
        for (const auto& item : items)
        {
          this->add(std::get<0>(item), std::get<1>(item), std::get<2>(item));


          items.resize(items.size + 1);
          std::get<2>(items.back()) = item.first;
          std::get<0>(items.back()) = item.second.first;
          std::get<1>(items.back()) = item.second.second;
        }
      }

    }

  };


} // namespace misc_utils
} // namespace epee

template<typename T>
std::ostream& print_container_content(std::ostream& out, const T& v);

namespace std
{
  template<typename T>
  std::ostream& operator<< (std::ostream& out, const std::vector<T>& v)
  {
    return print_container_content(out, v);
  }

  template<typename T>
  std::ostream& operator<< (std::ostream& out, const std::list<T>& v)
  {
    return print_container_content(out, v);
  }

} // namespace std

template<typename T>
std::ostream& print_container_content(std::ostream& out, const T& v)
{

  out << "[";
  if (!v.size())
  {
    out << "]";
    return out;
  }

  typename T::const_iterator last_it = --v.end();

  for (typename T::const_iterator it = v.begin(); it != v.end(); it++)
  {
    out << *it;
    if (it != last_it)
      out << ", ";
  }
  out << "]";
  return out;
}


