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

#include <boost/variant.hpp>

namespace epee
{
  namespace serialization
  {

    template<class t_storage>
    struct serialize_variant_visitor : public boost::static_visitor<bool>
    {
      serialize_variant_visitor(t_storage& rstg, typename t_storage::hsection hsection, const char* pn)
      :stg(rstg), hparent_section(hsection), pname(pn)
      {}
      t_storage& stg;
      typename t_storage::hsection hparent_section;
      const char* pname;

      template<class t_type>
      bool operator()(const t_type& v) const
      {
        return kv_serialization_overloads_impl_is_base_serializable_types<boost::mpl::contains<base_serializable_types<t_storage>, typename std::remove_const<t_type>::type>::value>::kv_serialize(v, stg, hparent_section, pname);
      }
    };

    template<class t_storage, typename t_type, typename... rest>
    bool kv_serialize(const boost::variant<t_type, rest...>& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      serialize_variant_visitor<t_storage> sv(stg, hparent_section, pname);
      return boost::apply_visitor(sv, d);
    }
    //-------------------------------------------------------------------------------------------------------------------
    template<class t_storage, typename t_type, typename... rest>
    bool kv_unserialize(boost::variant<t_type, rest... > &d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname)
    {
      LOG_ERROR("kv_unserialize on boost::variant call not allowed");
      return false;
    }
  }
}
