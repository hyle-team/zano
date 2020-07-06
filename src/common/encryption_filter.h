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


class encrypt_chacha_filter
{
public:
  typedef char                          char_type;
  //typedef boost::iostreams::multichar_output_filter_tag    category;
  //typedef boost::iostreams::flushable_tag    category;

  struct category: //public boost::iostreams::seekable_device_tag,
                   public boost::iostreams::multichar_output_filter_tag,
       	           public boost::iostreams::flushable_tag
  { };
  encrypt_chacha_filter(std::string const &pass, const crypto::chacha8_iv& iv);
  ~encrypt_chacha_filter();

  template<typename t_sink>
  std::streamsize write(t_sink& snk, char_type const * const buf, std::streamsize const n) const
  {
    if (n == 0)
      return n;
    if (n%ECRYPT_BLOCKLENGTH == 0 && m_buff.empty())
    {
      std::vector<char_type> buff(n);
      ECRYPT_encrypt_blocks(&m_ctx, (u8*)buf, (u8*)&buff[0], (u32)(n / ECRYPT_BLOCKLENGTH));
      boost::iostreams::write(snk, &buff[0], n);
      //m_underlying_stream.write(&buff[0], n);
    }
    else
    {
      m_buff.append(buf, n);
      size_t encr_count = m_buff.size() - m_buff.size() % ECRYPT_BLOCKLENGTH;
      if (!encr_count)
        return n;
      std::vector<char_type> buff(encr_count);
      ECRYPT_encrypt_blocks(&m_ctx, (u8*)m_buff.data(), (u8*)&buff[0], (u32)(m_buff.size() / ECRYPT_BLOCKLENGTH));
      //m_underlying_stream.write(&buff[0], encr_count);
      boost::iostreams::write(snk, &buff[0], encr_count);
      m_buff.erase(0, encr_count);
    }
    return n;
  }

  template<typename Sink>
  bool flush(Sink& snk)
  {
    if (m_buff.empty())
      return true;

    std::vector<char_type> buff(m_buff.size());
    ECRYPT_encrypt_bytes(&m_ctx, (u8*)m_buff.data(), (u8*)&buff[0], (u32)m_buff.size());
    boost::iostreams::write(snk, &buff[0], m_buff.size());
    //m_underlying_stream.write(&buff[0], m_buff.size());
    return true;
  }

private:
  const crypto::chacha8_iv& m_iv;
  mutable ECRYPT_ctx m_ctx;
  //std::ostream &m_underlying_stream;
  crypto::chacha8_key m_key;
  mutable std::string m_buff;
};