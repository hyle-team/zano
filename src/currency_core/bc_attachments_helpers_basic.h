// Copyright (c) 2014-2024 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "currency_basic.h"

namespace bc_services
{
  template<typename T>
  struct is_boost_variant : std::false_type {};

  template<typename... Args>
  struct is_boost_variant<boost::variant<Args...>> : std::true_type {};

  template<bool is_variant>
  struct type_selector;
  
  template<>
  struct type_selector<true>
  {
    template<typename t_type>
    static const std::type_info& get_type(const t_type& t)
    {
      return t.type();
    }
    template<typename t_type, typename t_return_type>
    static const t_return_type& get(const t_type& t)
    {
      return boost::get<t_return_type>(t);
    }
  };

  template<>
  struct type_selector<false>
  {
    template<typename t_type>
    static const std::type_info& get_type(const t_type& t)
    {
      return typeid(t);
    }
    template<typename t_type, typename t_return_type>
    static const t_return_type& get(const t_type& t)
    {
      return t;
    }
  };

  template<class t_attachment_type_container_t>
  bool get_first_service_attachment_by_id(const t_attachment_type_container_t& tx_items, const std::string& id, const std::string& instruction, currency::tx_service_attachment& res)
  {
    for (const auto& item : tx_items)
    {
      typedef type_selector<is_boost_variant<typename t_attachment_type_container_t::value_type>::value> TS;
      if (TS::get_type(item) == typeid(currency::tx_service_attachment))
      {
        const currency::tx_service_attachment& tsa = TS::template get<decltype(item), currency::tx_service_attachment>(item);
        if (tsa.service_id == id && tsa.instruction == instruction)
        {
          res = tsa;
          return true;
        }
      }
    }
    return false;
  }

}
