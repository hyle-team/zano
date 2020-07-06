// Copyright (c) 2014-2018 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <iostream>
#include <iosfwd> 
#include <type_traits>
#include <boost/iostreams/categories.hpp>  // sink_tag
#include "include_base_utils.h"
#include "crypto/chacha8.h"
#include "crypto/chacha8_stream.h"


class encrypt_chacha_sink
{
public:
  typedef char                          char_type;
  typedef boost::iostreams::sink_tag    category;
  encrypt_chacha_sink(std::ostream &underlyingStream, std::string const &pass, const crypto::chacha8_iv& iv);
  std::streamsize write(char_type const * const buf, std::streamsize const n) const;
  void flush() const;
  ~encrypt_chacha_sink();
private:
  const crypto::chacha8_iv& m_iv;
  mutable ECRYPT_ctx m_ctx;
  std::ostream &m_underlying_stream;
  crypto::chacha8_key m_key;
  mutable std::string m_buff;
};