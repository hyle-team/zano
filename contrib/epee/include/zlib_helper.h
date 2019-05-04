// Copyright (c) 2019, anonimal <anonimal@zano.org>
// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef CONTRIB_EPEE_INCLUDE_ZLIB_HELPER_H_
#define CONTRIB_EPEE_INCLUDE_ZLIB_HELPER_H_

#include "misc_log_ex.h"

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>

#include <exception>
#include <sstream>
#include <string>

namespace epee
{
namespace zlib_helper
{
inline bool pack(const std::string& input, std::string& output)
{
  TRY_ENTRY();

  std::stringstream compressed;
  boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
  out.push(boost::iostreams::zlib_compressor());
  out.push(compressed);

  std::stringstream decompressed(input);
  boost::iostreams::copy(decompressed, out);
  output = compressed.str();

  CATCH_ENTRY(__func__, false);  // TODO(anonimal): exception dispatcher
  return true;
}

inline bool pack(std::string& data)
{
  std::string result;
  if (pack(data, result))
    {
      data.swap(result);
      return true;
    }
  return false;
}

inline bool unpack(const std::string& input, std::string& output)
{
  TRY_ENTRY();

  std::stringstream decompressed;
  boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
  out.push(boost::iostreams::zlib_decompressor());
  out.push(decompressed);

  std::stringstream compressed(input);
  boost::iostreams::copy(compressed, out);
  output = decompressed.str();

  CATCH_ENTRY(__func__, false);  // TODO(anonimal): exception dispatcher
  return true;
}

inline bool unpack(std::string& data)
{
  std::string result;
  if (unpack(data, result))
    {
      data.swap(result);
      return true;
    }
  return false;
}

}  // namespace zlib_helper
}  // namespace epee

#endif  // CONTRIB_EPEE_INCLUDE_ZLIB_HELPER_H_
