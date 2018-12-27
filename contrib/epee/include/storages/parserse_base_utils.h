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

namespace epee 
{
namespace misc_utils
{
  namespace parse
  {
    inline std::string transform_to_escape_sequence(const std::string& src)
    {
      //std::stringstream res;
      std::string res;
      for(std::string::const_iterator it = src.begin(); it!=src.end(); ++it)
      {
        switch(*it)
        {
        case '\b':  //Backspace (ascii code 08)
          res+="\\b"; break;
        case '\f':  //Form feed (ascii code 0C)
          res+="\\f"; break;
        case '\n':  //New line
          res+="\\n"; break;
        case '\r':  //Carriage return
          res+="\\r"; break;
        case '\t':  //Tab
          res+="\\t"; break;
        case '\v':  //Vertical tab
          res+="\\v"; break;
        //case '\'':  //Apostrophe or single quote
        //  res+="\\'"; break;
        case '"':  //Double quote
          res+="\\\""; break;
        case '\\':  //Backslash caracter
          res+="\\\\"; break;
        case '/':  //Backslash caracter
          res+="\\/"; break;
        default:
          res.push_back(*it);
        }
      }
      return res;
    }

    /* Escapes JSON control characters and ASCII control characters (00h-1Fh => \u00XX) according to JSON reference.
       UTF-8-safe.
       from https://www.json.org/ :
       char --  any-Unicode-character-except-"-or-\-or-control-character: \"  \\  \/  \b  \f  \n  \r  \t  \uDDDD
    */
    inline std::string transform_to_json_escape_sequence(const std::string& str)
    {
      std::string result;

      static const char hex_map[17] = "0123456789abcdef";
      static const std::string su00 = "\\u00";

      const char* c_str = str.c_str();
      for(size_t i = 0, len = str.length(); i < len; ++i)
      {
        char c = c_str[i];
        switch(c)
        {
        case '\b':   // ASCII 08h
          result += "\\b";
          break;
        case '\t':   // ASCII 09h
          result += "\\t";
          break;
        case '\n':   // ASCII 0Ah
          result += "\\n";
          break;
        case '\f':   // ASCII 0Ch
          result += "\\f";
          break;
        case '\r':   // ASCII 0Dh
          result += "\\r";
          break;
        case '"':    // ASCII 22h
        case '\\':   // ASCII 5Ch
        case '/':    // ASCII 2Fh
          result.push_back('\\');
          result.push_back(c);
          break;

        default:
          if (static_cast<unsigned char>(c) < 0x20)
            result += su00 + hex_map[(c >> 4) & 0xf] + hex_map[c & 0xf]; // encode all other control characters as \uDDDD
          else
            result.push_back(c);
        }
      }
      return result;
    }

    /*
      
      \b     Backspace (ascii code 08)
      \f     Form feed (ascii code 0C)
      \n     New line
      \r     Carriage return
      \t     Tab
      \v     Vertical tab
      \'     Apostrophe or single quote
      \"     Double quote
      \\     Backslash character
      \u00XY ASCII character with code 0xXY

      */
      inline void match_string2(std::string::const_iterator& star_end_string, std::string::const_iterator buf_end, std::string& val)
      {
        val.clear();
        bool escape_mode = false;
        val.reserve(buf_end - star_end_string);
        std::string::const_iterator it = star_end_string;
        ++it;
        for(;it != buf_end;it++)
        {
          if(escape_mode/*prev_ch == '\\'*/)
          {
            switch(*it)
            {
            case 'b':  //Backspace (ascii code 08)
              val.push_back(0x08);break;
            case 'f':  //Form feed (ascii code 0C)
              val.push_back(0x0C);break;
            case 'n':  //New line
              val.push_back('\n');break;
            case 'r':  //Carriage return
              val.push_back('\r');break;
            case 't':  //Tab
              val.push_back('\t');break;
            case 'v':  //Vertical tab
              val.push_back('\v');break;
            case '\'':  //Apostrophe or single quote
              val.push_back('\'');break;
            case '"':  //Double quote
              val.push_back('"');break;
            case '\\':  //Backslash character
              val.push_back('\\');break;
            case '/':  //Slash character
              val.push_back('/');break;
            case 'u':  // \uDDDD sequence
              {
                bool ok = false;
                size_t chars_left = buf_end - it - 1;
                size_t chars_to_get = std::min(static_cast<size_t>(4), chars_left); // in [0, 4]
                char tmp[10] = {0};
                memcpy(tmp, &(*(it+1)), chars_to_get);
                it = it + chars_to_get; // move forward to skip 0..4 digits
                if (chars_to_get == 4)
                {
                  char *p_last_decoded_char = nullptr;
                  unsigned long value = strtoul(tmp, &p_last_decoded_char, 16); // expected value is from 0x0000 (\u0000) to 0x00ff (\u00ff)
                  if (value <= 0xff && p_last_decoded_char - tmp == 4)
                  {
                    val.push_back(static_cast<char>(value));
                    ok = true;
                  }
                }
                if (!ok)
                {
                  LOG_PRINT_L0("JSON invalid escape sequence: \\u" << tmp);
                }
              }
              break;
            default:
              val.push_back(*it);
              LOG_PRINT_L0("JSON unknown escape sequence :\"\\" << *it << "\"");
            }
            escape_mode = false;
          }else if(*it == '"')
          {
            star_end_string = it;
            return;
          }else if(*it == '\\')
          {
            escape_mode = true;
          }          
          else
          {
            val.push_back(*it); 
          }
        }
        ASSERT_MES_AND_THROW("Failed to match string in json entry: " << std::string(star_end_string, buf_end));
      }
      inline bool match_string(std::string::const_iterator& star_end_string, std::string::const_iterator buf_end, std::string& val)
      {
        try
        {

          match_string2(star_end_string, buf_end, val);
          return true;
        }
        catch(...)
        {
          return false;
        }
      }
      inline void match_number2(std::string::const_iterator& star_end_string, std::string::const_iterator buf_end, std::string& val, bool& is_float_val, bool& is_signed_val)
      {
        val.clear();
        is_float_val = false;
        for(std::string::const_iterator it = star_end_string;it != buf_end;it++)
        {
          if(isdigit(*it) || (it == star_end_string && *it == '-') || (val.size() && *it == '.' ) || (is_float_val && (*it == 'e' || *it == 'E' || *it == '-' || *it == '+' )) )
          {
            if(!val.size() &&  *it == '-')
              is_signed_val = true;
            if(*it == '.' ) 
              is_float_val = true;
            val.push_back(*it);
          }
          else
          {
            if(val.size())
            {
              star_end_string = --it;
              return;
            }
            else 
              ASSERT_MES_AND_THROW("wrong number in json entry: " << std::string(star_end_string, buf_end));
          }
        }
        ASSERT_MES_AND_THROW("wrong number in json entry: " << std::string(star_end_string, buf_end));
      }
      inline bool match_number(std::string::const_iterator& star_end_string, std::string::const_iterator buf_end, std::string& val)
      {
        try
        {
          bool is_v_float = false;bool is_signed_val = false;
          match_number2(star_end_string, buf_end, val, is_v_float, is_signed_val);
          return !is_v_float;
        }
        catch(...)
        {
          return false;
        }
      }
      inline void match_word2(std::string::const_iterator& star_end_string, std::string::const_iterator buf_end, std::string& val)
      {
        val.clear();

        for(std::string::const_iterator it = star_end_string;it != buf_end;it++)
        {
          if(!isalpha(*it))
          {
            val.assign(star_end_string, it);
            if(val.size())
            {
              star_end_string = --it;
              return;
            }else 
              ASSERT_MES_AND_THROW("failed to match word number in json entry: " << std::string(star_end_string, buf_end));
          }
        }
        ASSERT_MES_AND_THROW("failed to match word number in json entry: " << std::string(star_end_string, buf_end));
      }
      inline bool match_word(std::string::const_iterator& star_end_string, std::string::const_iterator buf_end, std::string& val)
      {
        try
        {
          match_word2(star_end_string, buf_end, val);
          return true;
        }
        catch(...)
        {
          return false;
        }
      }
      inline bool match_word_with_extrasymb(std::string::const_iterator& star_end_string, std::string::const_iterator buf_end, std::string& val)
      {
        val.clear();

        for(std::string::const_iterator it = star_end_string;it != buf_end;it++)
        {
          if(!isalnum(*it) && *it != '-' && *it != '_')
          {
            val.assign(star_end_string, it);
            if(val.size())
            {
              star_end_string = --it;
              return true;
            }else 
              return false;
          }
        }
        return false;
      }
      inline bool match_word_til_equal_mark(std::string::const_iterator& star_end_string, std::string::const_iterator buf_end, std::string::const_iterator& word_end)
      {
        word_end = star_end_string;

        for(std::string::const_iterator it = star_end_string;it != buf_end;it++)
        {
          if(isspace(*it))
          {

            continue;
          }else if( *it == '=' )
          {            
            star_end_string = it;
            word_end = it;
            return true;
          }
        }
        return false;
      }
  }
}
}