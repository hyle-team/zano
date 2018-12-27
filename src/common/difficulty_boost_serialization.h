// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/multiprecision/cpp_int.hpp>

namespace boost
{
  namespace serialization
  {
    //---------------------------------------------------
    template <class archive_t>
    inline void serialize(archive_t &a, currency::wide_difficulty_type &x, const boost::serialization::version_type ver)
    {
      if(archive_t::is_loading::value)
      {
        //load high part
        uint64_t v = 0;        
#ifdef DEBUG_DIFFICULTY_SERIALIZATION
        std::cout << "loading" << ENDL;
#endif
        a & v; 
        x = v;
#ifdef DEBUG_DIFFICULTY_SERIALIZATION
        std::cout << "hight part: " << std::hex << v << ENDL;
#endif
        //load low part
        x = x << 64;
        a & v;
#ifdef DEBUG_DIFFICULTY_SERIALIZATION
        std::cout << "low part: " << std::hex << v << ENDL;
#endif
        x += v;
#ifdef DEBUG_DIFFICULTY_SERIALIZATION
        std::cout << "loaded value: " << std::hex << x << ENDL;
#endif
      }else
      {
#ifdef DEBUG_DIFFICULTY_SERIALIZATION
        std::cout << "storing" << ENDL;
#endif
        //store high part
        currency::wide_difficulty_type x_ = x;
#ifdef DEBUG_DIFFICULTY_SERIALIZATION
        std::cout << "original: " << std::hex << x_ << ENDL;
#endif
        x_ = x_ >> 64;
        uint64_t v = x_.convert_to<uint64_t>();
#ifdef DEBUG_DIFFICULTY_SERIALIZATION
        std::cout << "hight part: " << std::hex << v << ENDL;
#endif
        a & v;         
        //store low part
        x_ = x;
        x_ = x_ << 64;
        x_ = x_ >> 64;
        v = x_.convert_to<uint64_t>();
#ifdef DEBUG_DIFFICULTY_SERIALIZATION
        std::cout << "low part: " << std::hex << v << ENDL;
#endif
        a & v;
      }      
    }
  }
}
