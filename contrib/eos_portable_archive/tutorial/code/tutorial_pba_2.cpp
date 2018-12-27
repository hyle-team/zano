/** tutorial_pba_2.cpp
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
 * to store/load floating point numbers including non-finite and 
 * special (denormalized) values.
 *
 */

#include <string>
#include <fstream>
#include <limits>

#include <boost/archive/portable_binary_oarchive.hpp>
#include <boost/archive/portable_binary_iarchive.hpp>

int main (void)
{
  using namespace std;

  // The name for the example data file :  
  string filename = "pba_2.data"; 

  {
    // A normal single precision floating point number :
    float pi = 3.14159265; 

    // Single precision zeroed floating point number :
    float zero = 0.0;

    // A denormalized single precision floating point number :
    float tiny = 1.e-40;
    
    // A single precision floating point number with `+Infinity' value :
    float plus_infinity = numeric_limits<float>::infinity ();
    
    // A single precision floating point number with `-Infinity' value :
    float minus_infinity = -numeric_limits<float>::infinity ();
    
    // A single precision `Not-a-Number' (NaN):
    float nan = numeric_limits<float>::quiet_NaN ();
    
    // Open an output file stream in binary mode :
    ofstream fout (filename.c_str (), ios_base::binary);
    
    {
      // Create an output portable binary archive attached to the output file :
      boost::archive::portable_binary_oarchive opba (fout);
      
      // Store (serialize) variables :
      opba & pi & zero & tiny & plus_infinity & minus_infinity & nan;
    }
  }

  { 
    // Single precision floating point numbers to be loaded :
    float x[6];

    // Open an input file stream in binary mode :
    ifstream fin (filename.c_str (), ios_base::binary);
  
    {
      // Create an input portable binary archive attached to the input file :
      boost::archive::portable_binary_iarchive ipba (fin);
      
      // Load (de-serialize) variables using the same 
      // order than for serialization :
      for (int i = 0; i < 6; ++i)
	{
	  ipba & x[i];
	}
    }

    // Print :
    for (int i = 0; i < 6; ++i)
      {
	cout.precision (8);
	cout << "Loaded x[" << i << "] = " << x[i];
	switch (fp::fpclassify(x[i]))
	  {
	  case FP_NAN: cout << " (NaN)"; break;
	  case FP_INFINITE: cout << " (infinite)"; break;
	  case FP_SUBNORMAL: cout << " (denormalized)"; break;
	  case FP_NORMAL:  cout << " (normalized)"; break;
	  }
	cout << endl;
      }
  }

  return 0;   
}

// end of tutorial_pba_2.cpp
