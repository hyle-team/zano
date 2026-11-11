// Copyright (c) 2006-2020, Andrey N. Sabelnikov, www.sabelnikov.net
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
#include <boost/thread.hpp>
#include "include_base_utils.h"
#include "auto_val_init.h"

#define DEFINE_SECURE_STATIC_VAR(type, var) static epee::static_helpers::wrapper<type> var##inst; \
  static type& var = var##inst;

namespace epee
{
  namespace static_helpers
  {
    template<class to_initialize>
    class initializer
    {
    public:
      initializer()
      {
        to_initialize::init();
        //get_set_is_initialized(true, true);
      }
      ~initializer()
      {
        to_initialize::un_init();
        //get_set_is_uninitialized(true, true);
      }
    };


    typedef void(*static_destroy_handler_type)();

    inline
      bool set_or_call_on_destruct(bool set = false, static_destroy_handler_type destr_ptr = nullptr)
    {
      volatile static bool deinit_called = false;
      volatile static static_destroy_handler_type static_destroy_handler = nullptr;

      if (set)
      {
        static_destroy_handler = destr_ptr;
        return true;
      }
      if (!deinit_called)
      {

        if (static_destroy_handler)
          static_destroy_handler();

        deinit_called = true;
      }

      return true;
    }

    template<class t_base>
    struct wrapper : public t_base
    {
      ~wrapper()
      {
        set_or_call_on_destruct();
      }

    };

  }
  



}
