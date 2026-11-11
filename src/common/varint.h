// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <limits>
#include <type_traits>
#include <utility>
#include <sstream>
#include <string>

namespace tools {


  //---------------------------------------------------------------
  template<typename T>
  size_t get_varint_packed_size(T v)
  {
    if(v <= 127)
      return 1;
    else if(v <= 16383)
      return 2;
    else if(v <= 2097151)
      return 3;
    else if(v <= 268435455)
      return 4;
    else if(v <= 34359738367)
      return 5;
    else if(v <= 4398046511103)
      return 6;
    else if(v <= 562949953421311)
      return 7;
    else 
      return 8;
  }
    template<typename OutputIt, typename T>
    typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value, void>::type
    write_varint(OutputIt &&dest, T i) {
        while (i >= 0x80) {
            *dest++ = (static_cast<char>(i) & 0x7f) | 0x80;
            i >>= 7;
        }
        *dest++ = static_cast<char>(i);
    }

    template<typename t_type>
    std::string get_varint_data(const t_type& v)
    {
      std::stringstream ss;
      write_varint(std::ostreambuf_iterator<char>(ss), v);
      return ss.str();
    }

    template<int bits, typename InputIt, typename T>
    typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value && 0 <= bits && bits <= std::numeric_limits<T>::digits, int>::type
    read_varint(InputIt &&first, InputIt &&last, T &i) {
        int read = 0;
        i = 0;
        for (int shift = 0;; shift += 7) {
            if (first == last) {
                return read; // End of input.
            }
            unsigned char byte = *first++;
            ++read;
            if (shift + 7 >= bits && byte >= 1 << (bits - shift)) {
                return -1; // Overflow.
            }
            if (byte == 0 && shift != 0) {
                return -2; // Non-canonical representation.
            }
            i |= static_cast<T>(byte & 0x7f) << shift;
            if ((byte & 0x80) == 0) {
                break;
            }
        }
        return read;
    }

    template<typename InputIt, typename T>
    int read_varint(InputIt &&first, InputIt &&last, T &i) {
        return read_varint<std::numeric_limits<T>::digits, InputIt, T>(std::move(first), std::move(last), i);
    }
}
