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

namespace epee
{
  namespace serialization
  {
    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    class portable_storage_extended: public portable_storage
    {
    public:
      typedef epee::serialization::hsection hsection;
      typedef epee::serialization::harray  harray;
      typedef storage_entry meta_entry;

      portable_storage_extended(){}
      virtual ~portable_storage_extended(){}
      hsection   open_section(const std::string& section_name,  hsection hparent_section, bool create_if_notexist = false);
      template<class t_value>
      bool       get_value(const std::string& value_name, t_value& val, hsection hparent_section);
      bool       get_value(const std::string& value_name, storage_entry& val, hsection hparent_section);
      template<class t_value>
      bool       set_value(const std::string& value_name, const t_value& target, hsection hparent_section);

      //serial access for arrays of values --------------------------------------
      //values
      template<class t_value>
      harray        get_first_value(const std::string& value_name, t_value& target, hsection hparent_section);
      template<class t_value>
      bool          get_next_value(harray hval_array, t_value& target);
      template<class t_value>
      harray        insert_first_value(const std::string& value_name, const t_value& target, hsection hparent_section);
      template<class t_value>
      bool          insert_next_value(harray hval_array, const t_value& target);
      //sections
      harray        get_first_section(const std::string& pSectionName, hsection& h_child_section, hsection hparent_section);
      bool          get_next_section(harray hSecArray, hsection& h_child_section);
      harray        insert_first_section(const std::string& pSectionName, hsection& hinserted_childsection, hsection hparent_section);
      bool          insert_next_section(harray hSecArray, hsection& hinserted_childsection);
      //------------------------------------------------------------------------
      //delete entry (section, value or array)
      bool        delete_entry(const std::string& pentry_name, hsection hparent_section = nullptr);      
      //-------------------------------------------------------------------------------
      bool		store_to_binary(binarybuffer& target);
      bool		load_from_binary(const binarybuffer& target);
      template<class trace_policy>
      bool		  dump_as_xml(std::string& targetObj, const std::string& root_name = "");
      bool		  dump_as_json(std::string& targetObj, size_t indent = 0, end_of_line_t eol = eol_crlf);
      bool		  load_from_json(const std::string& source);

      template<typename cb_t>
      bool enum_entries(hsection hparent_section, cb_t cb);
    private:
      section m_root;
      hsection	get_root_section() {return &m_root;}
      storage_entry* find_storage_entry(const std::string& pentry_name, hsection psection);
      template<class entry_type>
      storage_entry* insert_new_entry_get_storage_entry(const std::string& pentry_name, hsection psection, const entry_type& entry);

      hsection    insert_new_section(const std::string& pentry_name, hsection psection);
    };

    //---------------------------------------------------------------------------------------------------------------
  }
}
