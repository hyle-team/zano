/** tutorial_pba_10.cpp
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
 * This example shows how use PBAs combined with on-the-fly 
 * compressed I/O streams.
 *
 */

#include <string>
#include <fstream>
#include <limits>
#include <vector>

#include <boost/cstdint.hpp>
#include <boost/archive/portable_binary_oarchive.hpp>
#include <boost/archive/portable_binary_iarchive.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>

using namespace std;

class data_type
{
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize (Archive & ar, const unsigned int version);
public:
  void print (ostream & out, const string & title) const;
public:
  vector<double> values;
  data_type ();
};

data_type::data_type () : values ()
{
  return;
}

void data_type::print (ostream & out, const string & title) const
{
  out << endl;
  out << title << " :" << endl;
  for (int i = 0; i < this->values.size (); ++i)
    {
      out.precision (16);
      out.width (18);
      out << this->values [i] << ' ' ;
      if ((i%4) == 3) clog << endl;
    }
  out << endl;
  return;
}
  
template<class Archive>
void data_type::serialize (Archive & ar, const unsigned int version)
{
  ar & values;
  return;
}

void do_gzipped_out (void)
{
  // The name for the output data file :  
  string filename = "pba_10.data.gz"; 

  // A data structure to be stored :
  data_type my_data;

  // Fill the vector with arbitrary (possibly non-finite) values :
  size_t dim = 1000;
  my_data.values.reserve (dim);
  for (int i = 0; i < dim; ++i)
    {      
      double val = (i + 1) * (1.0 + 3 * numeric_limits<double>::epsilon ());
      if (i == 4) val = numeric_limits<double>::quiet_NaN ();
      if (i == 23) val = numeric_limits<double>::infinity ();
      if (i == 73) val = -numeric_limits<double>::infinity ();
      if (i == 90) val = 0.0;
      my_data.values.push_back (val);
    }

  // Print:
  my_data.print (clog, "Stored data");

  // Create an output filtering stream :
  boost::iostreams::filtering_ostream zout;
  zout.push (boost::iostreams::gzip_compressor ());
  
  // Open an output file stream in binary mode :
  ofstream fout (filename.c_str (), ios_base::binary);
  zout.push (fout);

  // Save to PBA :
  {
    // Create an output portable binary archive attached to the output file :
    boost::archive::portable_binary_oarchive opba (zout);
    
    // Store (serializing) the data :
    opba & my_data;
  }

  // Clean termination of the streams :
  zout.flush ();
  zout.reset ();

  return;   
}

void do_gzipped_in (void)
{
  // The name for the input data file :  
  string filename = "pba_10.data.gz"; 

  // A data structure to be loaded :
  data_type my_data;

  // Create an input filtering stream :
  boost::iostreams::filtering_istream zin;
  zin.push (boost::iostreams::gzip_decompressor ());
  
  // Open an input file stream in binary mode :
  ifstream fin (filename.c_str (), ios_base::binary);
  zin.push (fin);

  // Load from PBA :
  {
    // Create an input portable binary archive attached to the input file :
    boost::archive::portable_binary_iarchive ipba (zin);
    
    // Load (deserializing) the data :
    ipba & my_data;
  }

  // Print:
  my_data.print (clog, "Loaded data");

  return;   
}

int main (void)
{
  do_gzipped_out (); 
  do_gzipped_in ();
  return 0;
}

// end of tutorial_pba_10.cpp
