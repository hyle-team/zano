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
#include "portable_storage_to_.h"

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
      using use_descriptions = std::false_type;

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
        strm << make_indent(indent) << "}";
      }

      template<class t_stream>
      static void handle_print_key(t_stream& strm, const std::string& key, size_t indent)
      {
        const std::string indent_str = make_indent(indent);
        strm << indent_str << "\"" << misc_utils::parse::transform_to_json_escape_sequence(key) << "\"" << ": ";
      }


      template<class t_stream>
      static void handle_section_entry_separator(t_stream& strm, size_t indent)
      {
        strm << ",";
      }

      template<class t_stream>
      static void handle_array_entry_separator(t_stream& strm, size_t indent)
      {
        strm << ",";
      }

      template<class t_stream>
      static void handle_line_break(t_stream& strm, size_t indent)
      {
        strm << eol;
      }
    };

  }
}
