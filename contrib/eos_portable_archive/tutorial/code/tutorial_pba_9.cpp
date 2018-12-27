/** tutorial_pba_9.cpp
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
 * This example shows how to load some variables of basic 
 * types (bool, char, integer, floating point numbers, STL string) 
 * using the text or XML archive format associated to a 
 * standard file input stream supporting portable non-finite
 * floating point values.
 *
 */

#include <string>
#include <fstream>
#include <limits>
#include <locale>

#include <boost/cstdint.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/archive/codecvt_null.hpp>
#include <boost/math/special_functions/nonfinite_num_facets.hpp>

using namespace std;

void do_text_in (void)
{
  // The name for the example data text file :  
  string filename = "pba_8.txt"; 
  // Some variables of various primitive types :
  bool        b;
  char        c;
  uint32_t    answer;
  float       value;
  double      precision;
  string      question;
  
  // Open an input file stream :
  ifstream fin (filename.c_str ());

  // Prepare the input file stream for inf/NaN support :
  locale default_locale (locale::classic (),
			 new boost::archive::codecvt_null<char>);
  locale infnan_locale (default_locale,
			new boost::math::nonfinite_num_get<char>);
  fin.imbue (infnan_locale);
 
  {
    // Create an input text archive attached to the input file :
    boost::archive::text_iarchive ita (fin, boost::archive::no_codecvt);
    
    // Store (serializing) variables :
    ita & b & c & answer & value & precision & question;
  }

  clog << "Loaded values from text archive are: " << endl;
  clog << "  b         = " << b << endl;
  clog << "  c         = '" << c << "'" <<  endl;
  clog << "  answer    = " << answer << endl;
  clog << "  value     = " << value << endl;
  clog << "  precision = " << precision << endl;
  clog << "  question  = \"" << question << "\"" << endl;

  return;   
}

void do_xml_in (void)
{
  // The name for the example data text file :  
  string filename = "pba_8.xml"; 

  // Some variables of various primitive types :
  bool        b;
  char        c;
  uint32_t    answer;
  float       value;
  double      precision;
  string      question;
  
  // Open an input file stream :
  ifstream fin (filename.c_str ());

  // Prepare the input file stream for inf/NaN support :
  locale default_locale (locale::classic (),
			 new boost::archive::codecvt_null<char>);
  locale infnan_locale (default_locale,
			new boost::math::nonfinite_num_get<char>);
  fin.imbue (infnan_locale);

  {
    // Create an output text archive attached to the output file :
    boost::archive::xml_iarchive ixa (fin, boost::archive::no_codecvt);
    
    // Store (serializing) variables :
    ixa & BOOST_SERIALIZATION_NVP(b)
      & BOOST_SERIALIZATION_NVP(c)
      & BOOST_SERIALIZATION_NVP(answer)
      & BOOST_SERIALIZATION_NVP(value)
      & BOOST_SERIALIZATION_NVP(precision) 
      & BOOST_SERIALIZATION_NVP(question);
  }

  clog << "Loaded values from XML archive are: " << endl;
  clog << "  b         = " << b << endl;
  clog << "  c         = '" << c << "'" <<  endl;
  clog << "  answer    = " << answer << endl;
  clog << "  value     = " << value << endl;
  clog << "  precision = " << precision << endl;
  clog << "  question  = \"" << question << "\"" << endl;

  return;   
}

int main (void)
{
  do_text_in ();
  do_xml_in ();
  return 0;
}

// end of tutorial_pba_9.cpp
