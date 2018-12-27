/** tutorial_pba_10b.cpp
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
#include <boost/archive/xml_oarchive.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/version.hpp>

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

//BOOST_CLASS_VERSION(data_type, 7)

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
  ar & BOOST_SERIALIZATION_NVP (values);
  return;
}

class data_type2
{
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize (Archive & ar, const unsigned int version);
public:
  double value;
  data_type2 ();
};

BOOST_CLASS_VERSION(data_type2, 99)

data_type2::data_type2 () : value (666.666)
{
  return;
}

template<class Archive>
void data_type2::serialize (Archive & ar, const unsigned int version)
{
  ar & BOOST_SERIALIZATION_NVP (value);
  return;
}

class data_type3
{
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize (Archive & ar, const unsigned int version);
public:
  vector<data_type2> values;
  data_type3 ();
};

BOOST_CLASS_VERSION(data_type3, 33)

data_type3::data_type3 ()
{
  {
    data_type2 d;
    d.value = 6.66;
    values.push_back (d);
  }
  {
    data_type2 d;
    d.value = 66.66;
    values.push_back (d);
  }
  {
    data_type2 d;
    d.value = 666.66;
    values.push_back (d);
  }
  return;
}

template<class Archive>
void data_type3::serialize (Archive & ar, const unsigned int version)
{
  ar & BOOST_SERIALIZATION_NVP (values);
  return;
}

/********************/

void do_xml_out (void)
{
  // The name for the output data file :  
  string filename = "pba_10.xml"; 

  // A data structure to be stored :
  data_type my_data;

  // Fill the vector with arbitrary (possibly non-finite) values :
  size_t dim = 6;
  my_data.values.reserve (dim);
  for (int i = 0; i < dim; ++i)
    {      
      double val = (i + 1) * (1.0 + 3 * numeric_limits<double>::epsilon ());
      if (i == 4) val = numeric_limits<double>::quiet_NaN ();
      if (i == 7) val = numeric_limits<double>::infinity ();
      if (i == 9) val = -numeric_limits<double>::infinity ();
      if (i == 13) val = 0.0;
      my_data.values.push_back (val);
    }

  // Print:
  my_data.print (clog, "Stored data");

  data_type2 my_data2;
  data_type3 my_data3;

  // Open an output file stream in binary mode :
  ofstream fout (filename.c_str ());

  // Save to PBA :
  {
    // Create an output XML archive attached to the output file :
    boost::archive::xml_oarchive oxa (fout);
    
    // Store (serializing) the data :
    oxa & BOOST_SERIALIZATION_NVP(my_data);
    oxa & BOOST_SERIALIZATION_NVP(my_data2);
    oxa & BOOST_SERIALIZATION_NVP(my_data3);
  }

  return;   
}

int main (void)
{
  do_xml_out ();
  return 0;
}

// end of tutorial_pba_10b.cpp
