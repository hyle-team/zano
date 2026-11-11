// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "misc_language.h"

namespace tools
{
#define LOC_INT_ERR(cond, msg) CHECK_AND_ASSERT_THROW_MES(cond, "internal error in median_db_cache: " << msg << ENDL << *this)
  template<typename key, typename associated_data>
  class median_db_cache : public epee::misc_utils::median_helper<key, associated_data>
  {
    typedef epee::misc_utils::median_helper<key, associated_data> super_t;
    struct db_entry_type
    {
      key k;
      associated_data d;
    };

    //m_tx_median_helper;
    typedef tools::db::cached_key_value_accessor<uint64_t, db_entry_type, false, true> median_db_container;
    median_db_container m_median_container;
    uint64_t m_counter_hi;
    uint64_t m_counter_lo;
    bool m_is_loading_from_db;

    bool load_items_from_db()
    {
      std::map<uint64_t, db_entry_type> loaded_entries;
      LOG_PRINT_L0("Loading median from db cache...");
      m_median_container.enumerate_items([&](uint64_t i, uint64_t index, const db_entry_type& entry)
      {
        //adjust counters
        if (index > m_counter_hi)
          m_counter_hi = index;
        if (index < m_counter_lo)
          m_counter_lo = index;
        loaded_entries[index] = entry;
        return true;
      });
      LOG_PRINT_L0("Loaded " << loaded_entries.size() << " items, building median cache...");

      m_is_loading_from_db = true;
      for (auto i : loaded_entries)
      {
        this->push_item(i.second.k, i.second.d);
      }
      m_is_loading_from_db = false;
      LOG_PRINT_L0("Median cache build OK");
      return true;
    }
    virtual void handle_add_front_item(const key& k, const associated_data& ad)
    {
      if (m_is_loading_from_db)
        return;
      db_entry_type det = AUTO_VAL_INIT(det);
      det.k = k;
      det.d = ad;
      m_median_container.set(++m_counter_hi, det);
      
      //in case if it was totally empty at init(which is very doubtful)
      if (m_counter_lo == std::numeric_limits<uint64_t>::max())
        m_counter_lo = m_counter_hi;

    }
    virtual void handle_remove_front_item(const key& k)
    {
      LOC_INT_ERR(!m_is_loading_from_db, "m_is_loading_from_db is true");
      LOC_INT_ERR(m_median_container.erase_validate(m_counter_hi), "erase_validate failed");
      --m_counter_hi;
      //TODO: check coretests and make a decision on what to do with the following check
      //LOC_INT_ERR(m_counter_lo <= m_counter_hi, "handle_remove_front_item: m_counter_lo(" << m_counter_lo << ") > m_counter_hi(" << m_counter_hi << ")");
    }
    virtual void handle_purge_back_item()
    {
      LOC_INT_ERR(m_counter_lo < m_counter_hi, "handle_purge_back_item: m_counter_lo(" << m_counter_lo << ") >= m_counter_hi(" << m_counter_hi << ")");
      m_median_container.erase(m_counter_lo++);
    }



  public:
    median_db_cache(db::basic_db_accessor& db) :
      m_median_container(db),
      m_counter_hi(0),
      m_counter_lo(std::numeric_limits<uint64_t>::max()),
      m_is_loading_from_db(false)
    {

    }
    bool init(const std::string& name)
    {
      bool r = m_median_container.init(name);
      CHECK_AND_ASSERT_MES(r, false, "Failed to init m_median_container");
      return load_items_from_db();
    }

    ~median_db_cache()
    {
    }

    void clear()
    {
      super_t::clear();
      m_median_container.clear();
      m_counter_hi = 0;
      m_counter_lo = std::numeric_limits<uint64_t>::max();
    }
    
    template<typename key_t, typename associated_data_t>
    friend std::ostream& operator<<(std::ostream& ss, const median_db_cache<key_t, associated_data_t> &mdbc);
  }; // class median_db_cache
#undef LOC_INT_ERR

  template<typename key_t, typename associated_data_t>
  inline std::ostream& operator<<(std::ostream& s, const median_db_cache<key_t, associated_data_t> &mdbc)
  {
    s << "median_db_cache<" << typeid(key_t).name() << ", " << typeid(associated_data_t).name() << "> instance 0x" << &mdbc << ENDL
      << "  m_counter_lo:                       " << mdbc.m_counter_lo << ENDL
      << "  m_counter_hi:                       " << mdbc.m_counter_hi << ENDL
      << "  m_is_loading_from_db:               " << mdbc.m_is_loading_from_db << ENDL
      << "  m_median_container.size:            " << mdbc.m_median_container.size() << ENDL
      << "  m_median_container.size_no_cache(): " << mdbc.m_median_container.size_no_cache() << ENDL
      << "parent " << static_cast<epee::misc_utils::median_helper<key_t, associated_data_t>>(mdbc);
    return s;
  }


}