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

#include "portable_storage.h"
#include "portable_storage_to_description.h"


namespace epee
{
  namespace serialization
  {
    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    class portable_storage_extended_doc: public portable_storage
    {
    public:
      using use_descriptions = std::true_type;

      void set_entry_description(hsection hparent_section, const std::string& name, const std::string& description)
      {
        if (!hparent_section)
          hparent_section = &m_root;
        hparent_section->m_descriptions[name] = description;
      }

      bool dump_as_decriptions(std::string& buff, size_t indent  = 0 , end_of_line_t eol = eol_crlf)
      {
        TRY_ENTRY();
        std::stringstream ss;
        recursive_visitor<strategy_descriptin>::dump_as_(ss, m_root, indent);
        buff = ss.str();
        return true;
        CATCH_ENTRY("portable_storage_base<t_section>::dump_as_json", false)
      }

    };
    //---------------------------------------------------------------------------------------------------------------
  }
}
