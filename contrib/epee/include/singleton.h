
// Copyright (c) 2006-2018, Andrey N. Sabelnikov, www.sabelnikov.net
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

#include <memory>
#include "static_helpers.h"

template<class owned_object_t>
class abstract_singleton
{

  static std::shared_ptr<owned_object_t> get_set_instance_internal(bool is_need_set = false, owned_object_t* pnew_obj = nullptr)
  {
    static epee::static_helpers::wrapper<std::shared_ptr<owned_object_t>> val_pobj;

    if (is_need_set)
      val_pobj.reset(pnew_obj);

    return val_pobj;
  }

  static bool get_is_deinitialized(bool need_to_set = false, bool val_to_set = false)
  {
    static bool is_deintialized = false;
    if (need_to_set)
      is_deintialized = val_to_set;
    return is_deintialized;
  }


public:
  inline static bool init()
  {
    //better to call before multiple threads start to call it
    get_instance();
    return true;/*do nothing here*/
  }
  inline static bool un_init()
  {
    get_is_deinitialized(true, true);
    get_set_instance_internal(true, nullptr);
    return true;
  }

  static std::shared_ptr<owned_object_t> get_instance()
  {
    std::shared_ptr<owned_object_t> val_pobj = get_set_instance_internal();
    if (!val_pobj.get() && !get_is_deinitialized())
    {
      owned_object_t*pnewobj = new owned_object_t();
      get_set_instance_internal(true, pnewobj);
      val_pobj = get_set_instance_internal();
    }
    return val_pobj;
  }
};