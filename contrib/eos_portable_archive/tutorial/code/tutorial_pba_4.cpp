/** tutorial_pba_4.cpp
 *
 * (C) Copyright 2011 François Mauger, Christian Pfligersdorffer
 *
 * Use, modification and distribution is subject to the Boost Software
 * License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 */

/**
 * The intent of this program is to serve as a tutorial for
 * users of the portable binary archive in the framework of 
 * the Boost/Serialization library. 
 *
 * This sample program shows how to use a portable binary archive 
 * to store/load integer numbers of various sizes using the Boost 
 * portable integer typedefs.
 *
 */

#include <string>
#include <fstream>

#include <boost/cstdint.hpp>
#include <boost/archive/portable_binary_oarchive.hpp>
#include <boost/archive/portable_binary_iarchive.hpp>

int main (void)
{
  using namespace std;

  // The name for the example data file :  
  string filename = "pba_4.data"; 

  {
    // Some integer numbers :
    bool t = true;
    char c = 'c';
    unsigned char u = 'u';
    int8_t   b = -3; // char
    uint8_t  B = +6; // unsigned char 
    int16_t  s = -16;
    uint16_t S = +32;
    int32_t  l = -128;
    uint32_t L = +127;
    int64_t  ll = -1024;
    uint64_t LL = +2048;

    // Open an output file stream in binary mode :
    ofstream fout (filename.c_str (), ios_base::binary);
    
    {
      // Create an output portable binary archive attached to the output file :
      boost::archive::portable_binary_oarchive opba (fout);
      
      // Store (serialize) variables :
      opba & t & c & u & b & B & s & S & l & L & ll & LL;
    }
  }

  { 
    // Single precision floating numbers to be loaded :
    // Some integer numbers :
    bool t;
    char c;
    unsigned char u;
    int8_t   b;
    uint8_t  B;
    int16_t  s;
    uint16_t S;
    int32_t  l;
    uint32_t L;
    int64_t  ll;
    uint64_t LL;

    // Open an input file stream in binary mode :
    ifstream fin (filename.c_str (), ios_base::binary);
  
    {
      // Create an input portable binary archive attached to the input file :
      boost::archive::portable_binary_iarchive ipba (fin);
      
      // Load (de-serialize) variables using the same 
      // order than for serialization :
      ipba & t & c & u & b & B & s & S & l & L & ll & LL;
    }

    clog << "t  = " << t << " (bool)" << endl;
    clog << "c  = '" << c << "' (char)" << endl;
    clog << "u  = '" << u << "' (unsigned char)" << endl;
    clog << "b  = " << (int) b << " (int8_t)" << endl;
    clog << "B  = " << (int) B << " (uint8_t)" << endl;
    clog << "s  = " << s << " (int16_t)" << endl;
    clog << "S  = " << S << " (uint16_t)" << endl;
    clog << "l  = " << l << " (int32_t)" << endl;
    clog << "L  = " << L << " (uint32_t)" << endl;
    clog << "ll = " << ll << " (int64_t)" << endl;
    clog << "LL = " << LL << " (uint64_t)" << endl;
  }

  return 0;   
}

// end of tutorial_pba_4.cpp
