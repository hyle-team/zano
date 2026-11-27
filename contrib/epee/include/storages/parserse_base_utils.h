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
#include <algorithm>
#include <ios>
#include <iterator>
#include <boost/locale/utf.hpp>

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

    // JSON carries Unicode as \uXXXX and a raw byte as a literal \xHH marker
    // double original backslashes before JSON escaping to distinguish them from markers
    //
    // Writer input (hex) | Writer output (JSON) | Reader after JSON unescaping | Reader output (hex)
    // A6                 | "\\xA6"              | \xA6                         | A6
    // 5C 78 41 36        | "\\\\xA6"            | \\xA6                        | 5C 78 41 36
    //
    // strategy_json::handle_value adds the quotes around the writer output
    // match_string2 unescapes JSON, then decodes Zano byte markers
    inline std::string transform_to_json_escape_sequence(const std::string& str)
    {
      std::string result;
      static const char hex_map[] = "0123456789ABCDEF";
      const char* current = str.data();
      const char* end = current + str.size();
      while (current != end)
      {
        const unsigned char byte = *current;
        if (byte >= 0x20 && byte <= 0x7e)
        {
          if (*current == '\\')
            result += R"(\\\\)"; // preserve literal \xA6 across both decoding steps
          else if (*current == '"')
            result += "\\\"";
          else
            result.push_back(*current);
          ++current;
          continue;
        }

        const char* next = current;
        const auto value = boost::locale::utf::utf_traits<char>::decode(next, end); // try to read utf8
        if (value == boost::locale::utf::illegal || value == boost::locale::utf::incomplete)
        {
          //just save as this byte using \xHH notation
          //std::string byte = "\xA6" // just one byte A6
          result += R"(\\x)";                   // \\x
          result.push_back(hex_map[byte >> 4]); // \\xA
          result.push_back(hex_map[byte & 0xf]);// \\xA6
          ++current;
          continue;
        }

        char16_t units[2];// for \uXXXX, [2] cuz for example specific 😀 needs a surrogate pair like D83D and DE00
        const auto units_end = boost::locale::utf::utf_traits<char16_t>::encode(value, units); // points past the last UTF-16 unit written
        for (const char16_t* unit = units; unit != units_end; ++unit)
        {
          result += "\\u";
          for (int shift = 12; shift >= 0; shift -= 4) // take four hex digits from high to low
            result.push_back(hex_map[(*unit >> shift) & 0xf]); // \uD -> \uD8 -> \uD83 -> \uD83D
        }
        current = next;
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
      \uXXXX Unicode code unit

      */
      inline void match_string2(std::string::const_iterator& star_end_string, std::string::const_iterator buf_end, std::string& val)
      {
        const auto hex_digit = [](char c) -> int
        {
          if (c >= '0' && c <= '9') return c - '0';
          if (c >= 'a' && c <= 'f') return c - 'a' + 10;
          if (c >= 'A' && c <= 'F') return c - 'A' + 10;
          return -1;
        };
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
                const auto read_code_unit = [&it, buf_end, &hex_digit]()
                {
                  size_t chars_left = buf_end - it - 1;
                  if (chars_left < 4)
                    ASSERT_MES_AND_THROW("JSON incomplete \\u escape sequence");
                  unsigned long value = 0;
                  for (size_t i = 0; i < 4; ++i)
                  {
                    const int digit = hex_digit(*++it);
                    if (digit < 0)
                      ASSERT_MES_AND_THROW("JSON invalid hex digit in \\u escape sequence");
                    value = (value << 4) + digit;
                  }
                  return value;
                };
                unsigned long value = read_code_unit();
                if (value >= 0xd800 && value <= 0xdbff)
                {
                  size_t chars_left = buf_end - it - 1;
                  if (chars_left < 6 || *(it + 1) != '\\' || *(it + 2) != 'u')
                    ASSERT_MES_AND_THROW("JSON missing low surrogate in \\u escape sequence");
                  it += 2;
                  unsigned long low = read_code_unit();
                  if (low < 0xdc00 || low > 0xdfff)
                    ASSERT_MES_AND_THROW("JSON invalid low surrogate in \\u escape sequence");
                  value = 0x10000 + ((value - 0xd800) << 10) + (low - 0xdc00);
                }
                else if (value >= 0xdc00 && value <= 0xdfff)
                  ASSERT_MES_AND_THROW("JSON unexpected low surrogate in \\u escape sequence");
                boost::locale::utf::utf_traits<char>::encode(static_cast<boost::locale::utf::code_point>(value), std::back_inserter(val));
              }
              break;
            default:
              LOG_PRINT_L0("JSON unknown escape sequence :0x" << std::hex << (int)(uint8_t)*it);
              --it; // drop the backslash and read the character normally on the next iteration
            }
            escape_mode = false;
          }else if(*it == '"')
          {
            star_end_string = it;
            break;
          }else if(*it == '\\')
          {
            escape_mode = true;
          }          
          else
          {
            const unsigned char byte = *it;
            if (byte < 0x20)
              ASSERT_MES_AND_THROW("JSON unescaped control character");
            // for older versions writers emitted high bytes verbatim inside strings
            /*
              if (byte >= 0x80)
              {
                auto next = it;
                auto value = boost::locale::utf::utf_traits<char>::decode(next, buf_end);
                if (value == boost::locale::utf::illegal || value == boost::locale::utf::incomplete)
                  ASSERT_MES_AND_THROW("JSON invalid UTF-8 string");
                val.append(it, next);
                it = next - 1;
                continue;
              }
            */
            val.push_back(*it);
          }
        }
        if (it == buf_end)
          ASSERT_MES_AND_THROW("Failed to match string in json entry: " << std::string(star_end_string, buf_end));

        // decode markers in the json decoded string, only consume original input bytes
        // reader input "A\\\\\\xA6" becomes 41 5C 5C 5C 78 41 36 in val
        // read/write start at 1r/1w, move to 3r/2w after the backslash pair, then to 7r/3w after \xA6
        // resize keeps the decoded bytes 41 5C A6

        size_t read = val.find('\\');
        if (read == std::string::npos)
          return; // no escapes to decode
        size_t write = read; // keep prefix, write from first backslash
        while (read < val.size()) // consume original string before shrinking it
        {
          char c = val[read++];
          if (c == '\\' && read < val.size()) // trailing backslash has no escape suffix
          {
            if (val[read] == '\\')
            {
              ++read; // \\\xA6 -> keep one backslash, then read \xA6
            }
            else if (val[read] == 'x' && val.size() - read >= 3) // need x and two digits
            {
              const int high = hex_digit(val[read + 1]); // A -> 10
              const int low = hex_digit(val[read + 2]); // 6 -> 6
              if (high >= 0 && low >= 0)
              {
                c = static_cast<char>((high << 4) | low); // \xA6 -> byte A6
                read += 3; // consume x, A and 6
              }
            }
          }
          val[write++] = c;
        }
        val.resize(write); // A6 78 41 36 -> A6 when write == 1
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
          if(isdigit(static_cast<unsigned char>(*it)) || (it == star_end_string && *it == '-') || (val.size() && *it == '.' ) || (is_float_val && (*it == 'e' || *it == 'E' || *it == '-' || *it == '+' )) )
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
          if(!isalpha(static_cast<unsigned char>(*it)))
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