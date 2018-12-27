/** tutorial_pba_7.cpp
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
 * This example shows how the default behaviour of standard 
 * I/O streams does not support the read/write operations of 
 * non-finite floating point values in a portable way.
 *
 */

#include <string>
#include <iostream>
#include <sstream>
#include <limits>

using namespace std;

int main (void)
{
  {
    float x = numeric_limits<float>::infinity ();
    double y = numeric_limits<double>::quiet_NaN ();
    cout.precision (8);
    cout << "x = " << x << endl;
    cout.precision (16);
    cout << "y = " << y << endl;
  }

  {
    string input ("inf nan");
    istringstream iss (input);
    float x; 
    double y;
    iss >> x >> y;
    if (! iss)
      {
	cerr << "Cannot read 'x' or 'y' : non finite values are not supported !" << endl;
      }
  }
  return 0;
}

// end of tutorial_pba_7.cpp
