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

#include "misc_language.h"
#include "portable_storage_base.h"

namespace epee
{
  namespace serialization
  {
    inline const char* get_endline(end_of_line_t eol)
    {
      switch (eol)
      {
        case eol_lf:    return "\n";
        case eol_cr:    return  "\r";
        case eol_space: return  " ";
        default:        return  "\r\n";
      }
    }

    inline std::string make_indent(size_t indent)
    {
      return std::string(indent * 2, ' ');
    }

    class strategy_json
    {
    public:
      inline static const char* eol = get_endline(eol_crlf);
      //static const end_of_line_t eol = eol_crlf;
      template<class t_stream>
      static void handle_value(t_stream& strm, const std::string& v, size_t indent)
      {
        strm << "\"" << misc_utils::parse::transform_to_json_escape_sequence(v) << "\"";
      }
      template<class t_stream>
      static void handle_value(t_stream& strm, const int8_t& v, size_t indent)
      {
        strm << static_cast<int32_t>(v);
      }
      template<class t_stream>
      static void handle_value(t_stream& strm, const uint8_t& v, size_t indent)
      {
        strm << static_cast<int32_t>(v);
      }
      template<class t_stream>
      static void handle_value(t_stream& strm, const bool& v, size_t indent)
      {
        if (v)
          strm << "true";
        else
          strm << "false";
      }
      template<class t_stream>
      static void handle_value(t_stream& strm, const double& v, size_t indent)
      {
        boost::io::ios_flags_saver ifs(strm);
        strm.precision(8);
        strm << std::fixed << v;
      }
      template<class t_stream, class t_type>
      static void handle_value(t_stream& strm, const t_type& v, size_t indent)
      {
        strm << v;
      }

      template<class t_stream>
      static void handle_array_start(t_stream& strm, size_t indent)
      {
        strm << "[";
      }

      template<class t_stream>
      static void handle_array_end(t_stream& strm, size_t indent)
      {
        strm << "]";
      }

      template<class t_stream>
      static void handle_obj_begin(t_stream& strm, size_t indent)
      {
        strm << "{";
      }

      template<class t_stream>
      static void handle_obj_end(t_stream& strm, size_t indent)
      {
        strm << "}";
      }

      template<class t_stream>
      static void handle_print_key(t_stream& strm, const std::string& key, size_t indent)
      {
        const std::string indent_str = make_indent(indent);
        strm << indent_str << "\"" << misc_utils::parse::transform_to_json_escape_sequence(key) << "\"" << ": ";
      }



    };


    template <typename t_strategy_layout_strategy>
    class recursive_visitor
    {
    public:
      template<class t_stream>
      struct array_entry_store_to_json_visitor : public boost::static_visitor<void>
      {
        t_stream& m_strm;
        size_t m_indent;

        array_entry_store_to_json_visitor(t_stream& strm, size_t indent)
          : m_strm(strm)
          , m_indent(indent)
        {}

        template<class t_type>
        void operator()(const array_entry_t<t_type>& a)
        {

          t_strategy_layout_strategy::handle_array_start(m_strm, m_indent);
          if (a.m_array.size())
          {
            auto last_it = --a.m_array.end();
            for (auto it = a.m_array.begin(); it != a.m_array.end(); it++)
            {
              dump_as_(m_strm, *it, m_indent);
              if (it != last_it)
                m_strm << ",";
            }
          }
          t_strategy_layout_strategy::handle_array_end(m_strm, m_indent);
        }
      };

      template<class t_stream>
      struct storage_entry_store_to_json_visitor : public boost::static_visitor<void>
      {
        t_stream& m_strm;
        size_t m_indent;

        storage_entry_store_to_json_visitor(t_stream& strm, size_t indent)
          : m_strm(strm)
          , m_indent(indent)
        {}

        //section, array_entry
        template<class visited_type>
        void operator()(const visited_type& v)
        {
          dump_as_(m_strm, v, m_indent);
        }
      };

      template<class t_stream>
      void static dump_as_(t_stream& strm, const array_entry& ae, size_t indent)
      {
        array_entry_store_to_json_visitor<t_stream> aesv(strm, indent);
        boost::apply_visitor(aesv, ae);
      }

      template<class t_stream>
      void static dump_as_(t_stream& strm, const storage_entry& se, size_t indent)
      {
        storage_entry_store_to_json_visitor<t_stream> sv(strm, indent);
        boost::apply_visitor(sv, se);
      }

      template<class t_stream, class t_type>
      void static dump_as_(t_stream& strm, const t_type& v, size_t indent)
      {
        t_strategy_layout_strategy::handle_value(strm, v, indent);
      }

      template<class t_stream>
      void static dump_as_(t_stream& strm, const section& sec, size_t indent)
      {


        size_t local_indent = indent + 1;
        t_strategy_layout_strategy::handle_obj_begin(strm, indent);
        put_eol();
        
        if (sec.m_entries.size())
        {
          auto it_last = --sec.m_entries.end();
          for (auto it = sec.m_entries.begin(); it != sec.m_entries.end(); it++)
          {
            t_strategy_layout_strategy::handle_print_key(strm, it->first, local_indent);
            dump_as_(strm, it->second, local_indent/*, eol*/);
            if (it_last != it)
              strm << ",";
            put_eol();
          }
        }
        t_strategy_layout_strategy::handle_obj_end(strm, indent);
      }
    };

  }
}
