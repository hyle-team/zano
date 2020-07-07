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



/************************************************************************/
/*                                                                      */
/************************************************************************/

class encrypt_chacha_processer_base
{
public:
  typedef char                          char_type;
  //typedef boost::iostreams::multichar_output_filter_tag    category;
  //typedef boost::iostreams::flushable_tag    category;

  encrypt_chacha_processer_base(std::string const &pass, const crypto::chacha8_iv& iv) :m_iv(iv), m_ctx(AUTO_VAL_INIT(m_ctx))
  {
    crypto::generate_chacha8_key(pass, m_key);
    ECRYPT_keysetup(&m_ctx, &m_key.data[0], sizeof(m_key.data) * 8, sizeof(m_iv.data) * 8);
    ECRYPT_ivsetup(&m_ctx, &m_iv.data[0]);
  }
  ~encrypt_chacha_processer_base()
  {

  }

  template<typename cb_handler>
  std::streamsize process(char_type const * const buf, std::streamsize const n, cb_handler cb) const
  {
    if (n == 0)
      return n;
    if (n%ECRYPT_BLOCKLENGTH == 0 && m_buff.empty())
    {
      std::vector<char_type> buff(n);
      ECRYPT_encrypt_blocks(&m_ctx, (u8*)buf, (u8*)&buff[0], (u32)(n / ECRYPT_BLOCKLENGTH));
      cb(&buff[0], n);
      //m_underlying_stream.write(&buff[0], n);
      return n;
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
      cb(&buff[0], encr_count);
      m_buff.erase(0, encr_count);
      return encr_count;
    }

  }

  template<typename cb_handler>
  bool flush(cb_handler cb)
  {
    if (m_buff.empty())
      return true;

    std::vector<char_type> buff(m_buff.size());
    ECRYPT_encrypt_bytes(&m_ctx, (u8*)m_buff.data(), (u8*)&buff[0], (u32)m_buff.size());
    cb(&buff[0], m_buff.size());
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

/************************************************************************/
/*                                                                      */
/************************************************************************/


class encrypt_chacha_out_filter: public encrypt_chacha_processer_base
{
public:
  typedef char                          char_type;
  struct category:
    public boost::iostreams::multichar_output_filter_tag,
    public boost::iostreams::flushable_tag
  { };

  encrypt_chacha_out_filter(std::string const &pass, const crypto::chacha8_iv& iv) :encrypt_chacha_processer_base(pass, iv)
  {
  }
  ~encrypt_chacha_out_filter()
  {
  }

  template<typename t_sink>
  std::streamsize write(t_sink& snk, char_type const * const buf, std::streamsize const n) const
  {
    return encrypt_chacha_processer_base::process(buf, n, [&](char_type const * const buf_lambda, std::streamsize const n_lambda) {
      boost::iostreams::write(snk, &buf_lambda[0], n_lambda);
    });
  }

  template<typename Sink>
  bool flush(Sink& snk)
  {

    encrypt_chacha_processer_base::flush([&](char_type const * const buf_lambda, std::streamsize const n_lambda) {
      boost::iostreams::write(snk, &buf_lambda[0], n_lambda);
    });
    return true;
  }

private:
};


/************************************************************************/
/*                                                                      */
/************************************************************************/

class encrypt_chacha_in_filter : public encrypt_chacha_processer_base
{
public:
  typedef char                          char_type;
  struct category: //public boost::iostreams::seekable_device_tag,
    public boost::iostreams::multichar_input_filter_tag,
    public boost::iostreams::flushable_tag
    //public boost::iostreams::seekable_filter_tag
  { };
  encrypt_chacha_in_filter(std::string const &pass, const crypto::chacha8_iv& iv) :encrypt_chacha_processer_base(pass, iv), m_was_eof(false)
  {
  }
  ~encrypt_chacha_in_filter()
  {
  }

  template<typename Source>
  std::streamsize read(Source& src, char* s, std::streamsize n)
  {
    std::streamsize result = 0;
    if ((result = boost::iostreams::read(src, s, n)) == -1)
    {
      if (!m_was_eof)
      {
        m_was_eof = true;
        std::streamsize written_butes = 0;
        encrypt_chacha_processer_base::flush([&](char_type const * const buf_lambda, std::streamsize const n_lambda) {
          return written_butes = withdraw_to_read_buff(s, n, buf_lambda, n_lambda);
        });
        if (!written_butes)
          return -1;
        return written_butes;
      }
      else
      {
        if (!m_buff.size())
        {
          return -1;
        }
        else
        {
          return withdraw_to_read_buff(s, n, "", 0);
        }
      }
    }

    return encrypt_chacha_processer_base::process(s, result, [&](char_type const * const buf_lambda, std::streamsize const n_lambda) {
      return withdraw_to_read_buff(s, n, buf_lambda, n_lambda);
    });
  }


  template<typename Sink>
  bool flush(Sink& snk)
  {
    encrypt_chacha_processer_base::flush([&](char_type const * const buf_lambda, std::streamsize const n_lambda) {
      boost::iostreams::write(snk, &buf_lambda[0], n_lambda);
    });
    return true;
  }

private:
  std::streamsize withdraw_to_read_buff(char* s, std::streamsize n, char_type const * const buf_lambda, std::streamsize const n_lambda)
  {
    if (m_buff.size())
    {
      //need to use m_buff anyway
      m_buff.append(buf_lambda, n_lambda);
      size_t copy_size = n > m_buff.size() ? m_buff.size() : n;
      std::memcpy(s, buf_lambda, copy_size);
      m_buff.erase(0, copy_size);
      return copy_size;
    }
    else
    {
      size_t copy_size = n > n_lambda ? n_lambda : n;
      std::memcpy(s, buf_lambda, copy_size);
      if (n_lambda > copy_size)
      {
        //need to add tail to m_buff
        m_buff.append(buf_lambda + copy_size, n_lambda - copy_size);
      }
      return copy_size;
    }
  }

  std::string m_buff;
  bool m_was_eof;
};
