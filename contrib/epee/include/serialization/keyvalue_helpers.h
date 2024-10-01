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
#include <type_traits>
#include <optional>
#include "misc_language.h"
namespace epee
{

  template<class t_obj>
  struct enableable
  {
    t_obj v;
    bool enabled;

    enableable()
      : v(t_obj()), enabled(true)
    {	// construct from defaults
    }

    enableable(const t_obj& _v)
      : v(_v), enabled(true)
    {	// construct from specified values
    }

    enableable(const enableable<t_obj>& _v)
      : v(_v.v), enabled(_v.enabled)
    {	// construct from specified values
    }
  };

  template<typename T>
  struct is_std_optional : std::false_type {};

  template<typename T>
  struct is_std_optional<std::optional<T>> : std::true_type {};


  template<typename T>
  struct is_std_optional<boost::optional<T>> : std::true_type {};


  //basic helpers for pod-to-hex serialization 
  template<class t_pod_type>
  std::string transform_t_pod_to_str_internal(const t_pod_type& a)
  {
    return epee::string_tools::pod_to_hex(a);
  }

  template<class t_pod_type>
  std::string transform_t_pod_to_str_internal(const std::optional<t_pod_type>& a)
  {
    if (a.has_value())
      return epee::string_tools::pod_to_hex(*a);
    else
      return "";
  }

  template<class t_pod_type>
  std::string transform_t_pod_to_str_internal(const boost::optional<t_pod_type>& a)
  {
    if (a.has_value())
      return epee::string_tools::pod_to_hex(*a);
    else
      return "";
  }

	//basic helpers for pod-to-hex serialization 
	template<class t_pod_type>
	std::string transform_t_pod_to_str(const t_pod_type & a)
	{
		return transform_t_pod_to_str_internal(a);
	}


  
  template<class t_pod_type>
	t_pod_type transform_str_to_t_pod(const std::string& a)
	{
    t_pod_type res = AUTO_VAL_INIT(res);
    if (a.empty())
      return res;
    if constexpr (is_std_optional<t_pod_type>::value)
    {
      typename t_pod_type::value_type v = AUTO_VAL_INIT(v);
      if (!epee::string_tools::hex_to_pod(a, v))
        throw std::runtime_error(std::string("Unable to transform \"") + a + "\" to pod type " + typeid(typename t_pod_type::value_type).name());
      return v;
    }


    if (!epee::string_tools::hex_to_pod(a, res))
      throw std::runtime_error(std::string("Unable to transform \"") + a + "\" to pod type " + typeid(t_pod_type).name());
		return res;
	}



  //basic helpers for blob-to-hex serialization 
  
  inline std::string transform_binbuf_to_hexstr(const std::string& a)
  {
    return epee::string_tools::buff_to_hex_nodelimer(a);
  }

  inline std::string transform_hexstr_to_binbuff(const std::string& a)
  {
    std::string res;
    if (!epee::string_tools::parse_hexstr_to_binbuff(a, res))
    {
      CHECK_AND_ASSERT_THROW_MES(false, "Failed to parse hex string:" << a);
    }
    return res;
  }

  // helper for blob-to-base64 serialization
  inline std::string transfrom_binbuf_to_base64(const std::string& a)
  {
    return epee::string_encoding::base64_encode(a);
  }

  inline std::string transform_base64_to_binbuf(const std::string& a)
  {
    return epee::string_encoding::base64_decode(a);
  }


  //basic helpers for pod-to-hex serialization 
  template<class t_pod_container_type>
  std::string transform_t_pod_array_to_hex_str_array(const t_pod_container_type& a)
  {
    std::string res;
    for (const auto& item : a)
    {
      res += epee::string_tools::pod_to_hex(item) + ", ";
    }
    if (a.size())
    {
      res.erase(res.size() - 2);
    }

    return res;
  }
  template<class t_pod_container_type>
  t_pod_container_type transform_hex_str_array_to_t_pod_array(const std::string& a)
  {
    std::vector<std::string> pod_items;
    boost::split(pod_items, a, boost::is_any_of(", ][\""));

    t_pod_container_type res;
    if (!a.size())
      return res;
    for (const auto& item : pod_items)
    {
      if(!item.size())
        continue;

      res.resize(res.size() + 1);
      typename t_pod_container_type::value_type& pod_val = res.back();

      if (!epee::string_tools::hex_to_pod(item, pod_val))
        throw std::runtime_error(std::string("Unable to transform \"") + item + "\" to pod type " + typeid(typename t_pod_container_type::value_type).name());
    }
    return res;
  }
	//-------------------------------------------------------------------------------------------------------------------
#pragma pack(push, 1)
  template<class first_t, class second_t>
  struct pod_pair
  {
    first_t first;
    second_t second;
  };
#pragma pack(pop)
}