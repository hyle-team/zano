/** tutorial_pba_3.cpp
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
 * and prevent the serialization of non-finite floating numbers.
 *
 */

#include <string>
#include <fstream>
#include <limits>

#include <boost/archive/portable_binary_oarchive.hpp>

int main (void)
{
  using namespace std;

  // The name for the example data file :  
  string filename = "pba_3.data"; 

  try 
    {
      // An array of single precision floating numbers:
      float x[5]; 
      x[0] = 3.14159;  // Pi
      x[1] = 6.022e22; // Avogadro constant
      x[2] = 1.6e-19;  // Electron charge magnitude
      x[3] = 1.e-40;   // A tiny (denormalized) value
      x[4] = numeric_limits<float>::infinity (); // This will fail while serializing...

      // Open an output file stream in binary mode :
      ofstream fout (filename.c_str (), ios_base::binary);
    
      {
	// Create an output portable binary archive attached to the output file,
	// using the special 'boost::archive::no_infnan' flag :
	boost::archive::portable_binary_oarchive opba (fout, boost::archive::no_infnan);
	
	// Store (serialize) variables :
	for (int i = 0; i < 5; ++i)
	  {
	    clog << "Serializing value : " << x[i] << " ... ";
	    opba & x[i];
	    clog << "Ok !" << endl;
	  }
      }
    }
  catch (exception & x)
    {
      cerr << "ERROR: " << x.what () << endl;
      return 1;
    }

  return 0;   
}

// end of tutorial_pba_3.cpp
