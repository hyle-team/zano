// Copyright (c) 2014-2018 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <iostream>
#include <iosfwd> 
#include <type_traits>
#include <boost/iostreams/categories.hpp>  // sink_tag
#include <boost/iostreams/operations.hpp>  // boost::iostreams::write, boost::iostreams::read
#include "include_base_utils.h"
#include "crypto/chacha8.h"
#include "crypto/chacha8_stream.h"


namespace tools
{

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

  class encrypt_chacha_processer_base
  {
  public:
    typedef char                          char_type;
    //typedef boost::iostreams::multichar_output_filter_tag    category;
    //typedef boost::iostreams::flushable_tag    category;
    static const uint32_t block_size = ECRYPT_BLOCKLENGTH;

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


  class encrypt_chacha_out_filter : public encrypt_chacha_processer_base
  {
  public:
    typedef char                          char_type;
    struct category :
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
    struct category : //public boost::iostreams::seekable_device_tag,
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
      if (m_buff.size() >= static_cast<size_t>(n))
      {
        return withdraw_to_read_buff(s, n);
      }
      if (m_was_eof && m_buff.empty())
      {
        return -1;
      }

      std::streamsize size_to_read_for_decrypt = (n - m_buff.size());
      size_to_read_for_decrypt += size_to_read_for_decrypt % encrypt_chacha_processer_base::block_size;
      size_t offset_in_buff = m_buff.size();
      m_buff.resize(m_buff.size() + size_to_read_for_decrypt);

      std::streamsize result = boost::iostreams::read(src, (char*)&m_buff.data()[offset_in_buff], size_to_read_for_decrypt);
      if (result == size_to_read_for_decrypt)
      {
        //regular read proocess, readed data enought to get decrypteds
        encrypt_chacha_processer_base::process(&m_buff.data()[offset_in_buff], size_to_read_for_decrypt, [&](char_type const* const buf_lambda, std::streamsize const n_lambda)
        {
          CHECK_AND_ASSERT_THROW_MES(n_lambda == size_to_read_for_decrypt, "Error in decrypt: check n_lambda == size_to_read_for_decrypt failed");
          std::memcpy((char*)&m_buff.data()[offset_in_buff], buf_lambda, n_lambda);
        });
        return withdraw_to_read_buff(s, n);
      }
      else
      {
        //been read some size_but it's basically might be eof
        if (!m_was_eof)
        {
          size_t offset_before_flush = offset_in_buff;
          if (result != -1)
          {
            //eof
            encrypt_chacha_processer_base::process(&m_buff.data()[offset_in_buff], result, [&](char_type const* const buf_lambda, std::streamsize const n_lambda) {
              std::memcpy((char*)&m_buff.data()[offset_in_buff], buf_lambda, n_lambda);
              offset_before_flush = offset_in_buff + n_lambda;
            });
          }

          encrypt_chacha_processer_base::flush([&](char_type const* const buf_lambda, std::streamsize const n_lambda) {
            if (n_lambda + offset_before_flush > m_buff.size())
            {
              m_buff.resize(n_lambda + offset_before_flush);
            }
            std::memcpy((char*)&m_buff.data()[offset_before_flush], buf_lambda, n_lambda);
            m_buff.resize(offset_before_flush + n_lambda);
          });

          //just to make sure that it's over
          std::string buff_stub(10, ' ');
          std::streamsize r = boost::iostreams::read(src, (char*)&buff_stub.data()[0], 10);
          CHECK_AND_ASSERT_THROW_MES(r == -1, "expected EOF");
          m_was_eof = true;
          return withdraw_to_read_buff(s, n);
        }
        else
        {
          return -1;
        }
      }
    }


    template<typename Sink>
    bool flush(Sink& snk)
    {

      return true;
    }

  private:

    std::streamsize withdraw_to_read_buff(char* s, std::streamsize n)
    {

      size_t copy_size = m_buff.size() > static_cast<size_t>(n) ? static_cast<size_t>(n) : m_buff.size();
      std::memcpy(s, m_buff.data(), copy_size);
      m_buff.erase(0, copy_size);
      return copy_size;
    }   

    std::string m_buff;
    bool m_was_eof;
  };
}