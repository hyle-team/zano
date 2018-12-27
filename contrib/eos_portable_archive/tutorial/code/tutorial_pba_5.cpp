/** tutorial_pba_5.cpp
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
 * to store/load data in a memory buffer.
 *
 */

#include <string>
#include <vector>

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/device/array.hpp>

#include <boost/cstdint.hpp>
#include <boost/archive/portable_binary_oarchive.hpp>
#include <boost/archive/portable_binary_iarchive.hpp>

int main (void)
{
  using namespace std;

  // The memory buffer is implemented using a STL vector :
  typedef std::vector<char> buffer_type;
  buffer_type buffer; 

  {
    // Some data to be stored :
    bool    t = true;
    char    c = 'c';
    int16_t s = +16;
    int32_t l = -128;
    int64_t ll = +10000000000;
    float   pi = 3.14159;
    double  nan = numeric_limits<double>::quiet_NaN ();
    string  hello = "World !";

    buffer.reserve (1024); // pre-allocate some memory

    // The output stream interface to the buffer :
    boost::iostreams::stream<boost::iostreams::back_insert_device<buffer_type> > output_stream (buffer);

    {    
      // Create an output portable binary archive attached to the output file :
      boost::archive::portable_binary_oarchive opba (output_stream);
      
      // Store (serialize) variables :
      opba & t & c & s & l & ll & pi & nan & hello;
    }

  }

  clog << "Buffer content is " << buffer.size () << " bytes : " << endl << "  ";
  for (int i = 0; i < buffer.size (); ++i)
    {
      clog << (int) ((unsigned char) buffer[i]) << ' '; 
      if ((i + 1) % 20 == 0) clog << endl << "  ";
    }
  clog << endl;

  { 
    // Some data to be loaded :
    bool    t;
    char    c;
    int16_t s;
    int32_t l;
    int64_t ll;
    float   pi;
    double  nan;
    string  hello;

    // The input stream interface to the buffer :
    boost::iostreams::stream<boost::iostreams::array_source> input_stream (&buffer[0], 
									   buffer.size ());

    {
      // Create an input portable binary archive attached to the input file :
      boost::archive::portable_binary_iarchive ipba (input_stream);
      
      // Load (de-serialize) variables :
      ipba & t & c & s & l & ll & pi & nan & hello;
    }

    clog << "Loaded values from the buffer are: " << endl;
    clog << "  t  = " << t << " (bool)" << endl;
    clog << "  c  = '" << c << "' (char)" << endl;
    clog << "  s  = " << s << " (int16_t)" << endl;
    clog << "  l  = " << l << " (int32_t)" << endl;
    clog << "  ll = " << ll << " (int64_t)" << endl;
    clog << "  pi = " << pi << " (float)" << endl;
    clog << "  nan = " << nan << " (double)" << endl;
    clog << "  hello = \"" << hello << "\" (std::string)" << endl;
  }

  return 0;   
}

// end of tutorial_pba_5.cpp
