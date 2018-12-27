/** tutorial_pba_0.cpp
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
 * This quick start example shows how to store some variables
 * of basic types (bool, integer, floating point numbers, STL string) 
 * using the portable binary archive format associated to a 
 * standard output file stream.
 *
 */

#include <string>
#include <fstream>

#include <boost/cstdint.hpp>
#include <boost/archive/portable_binary_oarchive.hpp>

int main (void)
{
  // The name for the example data file :  
  std::string filename = "pba_0.data"; 

  // Some variables of various primitive types :
  bool        b              = true;
  char        c              = 'B';
  uint32_t    answer         = 42;
  float       computing_time = 7.5e6;
  double      e              = 2.71828182845905;
  std::string slogan         = "DON'T PANIC";
  
  // Open an output file stream in binary mode :
  std::ofstream fout (filename.c_str (), std::ios_base::binary);
  
  {
    // Create an output portable binary archive attached to the output file :
    boost::archive::portable_binary_oarchive opba (fout);
    
    // Store (serializing) variables :
    opba & b & c & answer & computing_time & e & slogan;
  }

  return 0;   
}

// end of tutorial_pba_0.cpp
