/** tutorial_pba_8.cpp
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
 * This example shows how to store some variables
 * of basic types (bool, integer, floating point numbers, STL string) 
 * using the text or XML archive format associated to a 
 * standard output file stream supporting portable non-finite
 * floating point values.
 *
 */

#include <string>
#include <fstream>
#include <limits>
#include <locale>

#include <boost/cstdint.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/archive/codecvt_null.hpp>
#include <boost/math/special_functions/nonfinite_num_facets.hpp>

using namespace std;

void do_text_out (void)
{
  // The name for the example data text file :  
  string filename = "pba_8.txt"; 

  // Some variables of various primitive types :
  bool        b         = true;
  char        c         = 'B';
  uint32_t    answer    = 42;
  float       value     = numeric_limits<float>::infinity ();
  double      precision = numeric_limits<double>::quiet_NaN ();
  string      question  = "What makes you think she's a witch?";
  
  // Open an output file stream :
  ofstream fout (filename.c_str ());

  // Prepare the output file stream for inf/NaN support :
  locale default_locale (locale::classic (),
			 new boost::archive::codecvt_null<char>);
  locale infnan_locale (default_locale,
			new boost::math::nonfinite_num_put<char>);
  fout.imbue (infnan_locale);
  
  {
    // Create an output text archive attached to the output file :
    boost::archive::text_oarchive ota (fout, boost::archive::no_codecvt);
    
    // Store (serializing) variables :
    ota & b & c & answer & value & precision & question;
  }

  return;   
}

void do_xml_out (void)
{
  // The name for the example data XML file :  
  string filename = "pba_8.xml"; 

  // Some variables of various primitive types :
  bool        b         = true;
  char        c         = 'B';
  uint32_t    answer    = 42;
  float       value     = numeric_limits<float>::infinity ();
  double      precision = numeric_limits<double>::quiet_NaN ();
  string      question  = "What makes you think she's a witch?";
  
  // Open an output file stream :
  ofstream fout (filename.c_str ());

  // Prepare the output file stream for inf/NaN support :
  locale default_locale (locale::classic (),
			 new boost::archive::codecvt_null<char>);
  locale infnan_locale (default_locale,
			new boost::math::nonfinite_num_put<char>);
  fout.imbue (infnan_locale);
   
  {
    // Create an output text archive attached to the output file :
    boost::archive::xml_oarchive oxa (fout, boost::archive::no_codecvt);
    
    // Store (serializing) variables :
    oxa & BOOST_SERIALIZATION_NVP(b)
      & BOOST_SERIALIZATION_NVP(c)
      & BOOST_SERIALIZATION_NVP(answer)
      & BOOST_SERIALIZATION_NVP(value)
      & BOOST_SERIALIZATION_NVP(precision) 
      & BOOST_SERIALIZATION_NVP(question);
  }

  return;   
}

int main (void)
{
  do_text_out ();
  do_xml_out ();
  return 0;
}

// end of tutorial_pba_8.cpp
