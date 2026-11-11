// Copyright (c) 2006-2022, Andrey N. Sabelnikov, www.sabelnikov.net
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
#include "keyvalue_helpers.h"


//should be done in global namespace
#define KV_ENABLE_POD_SERIALIZATION_AS_HEX(type_name)   \
namespace epee \
{ \
  namespace serialization \
  { \
    template<class t_storage> \
    bool kv_serialize(const type_name& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname) \
    { \
      std::string s = epee::transform_t_pod_to_str(d); \
      return kv_serialize(s, stg, hparent_section, pname); \
    } \
    template<class t_storage> \
    bool kv_unserialize(type_name& d, t_storage& stg, typename t_storage::hsection hparent_section, const char* pname) \
    { \
      std::string s; \
      bool r = kv_unserialize(s, stg, hparent_section, pname); \
      if (r) \
      { \
        d = epee::transform_str_to_t_pod<type_name>(s); \
      } \
      return r; \
    } \
  } \
} 
