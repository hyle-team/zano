/** tutorial_pba_1.cpp
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
 * the Boost/Serialization package. 
 *
 * This quick start example shows how to load some variables
 * of basic types (bool, integer, floating point numbers, STL string) 
 * using the portable binary archive format associated to a 
 * standard input file stream. 
 *
 */

#include <iostream>
#include <string>
#include <fstream>

#include <boost/cstdint.hpp>
#include <boost/archive/portable_binary_iarchive.hpp>

int main (void)
{
  using namespace std;

  // The name for the example data file :  
  string filename = "pba_0.data"; 

  // Some variables of various types :
  bool     b;
  char     c;
  uint32_t answer;
  float    computing_time;
  double   e;
  string   slogan;
   
  // Open an input file stream in binary mode :
  ifstream fin (filename.c_str (), ios_base::binary);
  
  {
    // Create an input portable binary archive attached to the input file :
    boost::archive::portable_binary_iarchive ipba (fin);
    
    // Loading (de-serializing) variables using the same 
    // order than for serialization (see tutorial_pba_0.cpp) :
    ipba & b & c & answer & computing_time & e & slogan;
  }

  cout.precision (15);
  cout << "Variable 'b' is              : " << b << " " << "(bool)" << endl;
  cout << "Variable 'c' is              : '" << c << "' " << " " << "(char)" << endl;
  cout << "Variable 'answer' is         : " << answer  << " " << "(unsigned 32-bit integer)" << endl;
  cout << "Variable 'computing_time' is : " << computing_time  << " " << "(single precision 32-bit float)" << endl;
  cout << "Variable 'e' is              : " << e  << " " << "(double precision 64-bit float)" << endl;
  cout << "Variable 'slogan' is         : \"" << slogan  << "\" " << "(std::string)" << endl;

  return 0;   
}

// end of tutorial_pba_1.cpp
