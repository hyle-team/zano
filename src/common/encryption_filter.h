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
#include "crypto/chacha.h"
#include "crypto/chacha8_stream.h"


namespace tools
{

  inline size_t to_size(std::streamsize n) {
    if (n < 0) {
      throw std::out_of_range("negative value cannot be converted to size_t");
    }
    return static_cast<size_t>(n);
  }
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

  class encrypt_chacha8_processer_base
  {
  public:
    typedef char                          char_type;
    //typedef boost::iostreams::multichar_output_filter_tag    category;
    //typedef boost::iostreams::flushable_tag    category;
    static const uint32_t block_size = ECRYPT_BLOCKLENGTH;

    encrypt_chacha8_processer_base(std::string const &pass, const crypto::chacha_iv& iv)
      : m_iv(iv)
      , m_ctx{}
    {
      crypto::generate_chacha_key_legacy(pass, m_key);
      ECRYPT_keysetup(&m_ctx, &m_key.data[0], sizeof(m_key.data) * 8, sizeof(m_iv.data) * 8);
      ECRYPT_ivsetup(&m_ctx, &m_iv.data[0]);
    }

    encrypt_chacha8_processer_base(std::string const& pass, const crypto::chacha_iv& iv, const char(&/*hdss*/)[32])
      : m_iv(iv)
      , m_ctx{}
    {
      crypto::generate_chacha_key_legacy(pass, m_key);
      ECRYPT_keysetup(&m_ctx, &m_key.data[0], sizeof(m_key.data) * 8, sizeof(m_iv.data) * 8);
      ECRYPT_ivsetup(&m_ctx, &m_iv.data[0]);
    }


    ~encrypt_chacha8_processer_base()
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
    const crypto::chacha_iv& m_iv;
    mutable ECRYPT_ctx m_ctx;
    //std::ostream &m_underlying_stream;
    crypto::chacha_key m_key;
    mutable std::string m_buff;
  };


  /************************************************************************/
/*  ChaCha20 processor base with domain separator support               */
/************************************************************************/

  class encrypt_chacha20_processer_base
  {
  public:
    typedef char                          char_type;
    static const uint32_t block_size = 64; // ChaCha20 block size

    encrypt_chacha20_processer_base(std::string const& pass, const crypto::chacha_iv& iv, const char(&hdss)[32])
      : m_total_written(0)
      , m_base_iv(iv)
    {
      crypto::chacha_generate_key_and_iv(hdss, pass.data(), pass.size(), 0, m_key);
    }

    ~encrypt_chacha20_processer_base()
    {
      memset(&m_key, 0, sizeof(m_key));
      memset(&m_base_iv, 0, sizeof(m_base_iv));
    }

    template<typename cb_handler>
    std::streamsize process(char_type const* const buf, std::streamsize const n, cb_handler cb) const
    {
      if (n == 0)
        return 0;

      size_t current_block_number = m_total_written / block_size;
      size_t offset_in_current_block = m_total_written % block_size;

      size_t bytes_left = n;

      std::vector<char_type> local_buf(n);
      while (bytes_left)
      {

        uint8_t keystream[block_size];
        generate_keystream_block(current_block_number, keystream);
        size_t to_process = std::min(bytes_left, block_size - offset_in_current_block);
        size_t process_i_until = offset_in_current_block + to_process;

        for( size_t i = offset_in_current_block; i < process_i_until; ++i)
        {
          size_t local_offset = n - bytes_left;
          if (local_offset > to_size(n))
          {
            // This should never happen, but we check it just in case
            throw std::runtime_error("Local offset exceeds buffer size");
          }
          local_buf[local_offset] = buf[local_offset] ^ keystream[i];
          bytes_left -= sizeof(char_type);
        }
        
        offset_in_current_block = 0;
        current_block_number++;
      }

      cb(&local_buf[0], n);
      m_total_written += n;
      return n;
    }

    template<typename cb_handler>
    bool flush(cb_handler cb)
    {
      return true;
    }

  private:

    void generate_keystream_block(uint64_t block_num, uint8_t* keystream) const
    {
      crypto::chacha_iv block_iv = m_base_iv;
      block_iv.u64 += block_num;

      std::memset(keystream, 0, block_size);
      crypto::chacha20(keystream, block_size, m_key, block_iv, keystream);
    }

    mutable crypto::chacha_iv m_base_iv;
    crypto::chacha_key m_key{};
    //mutable std::string m_buff;
    mutable uint64_t m_total_written = 0;
  };


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  template<typename t_chacha_base>
  class encrypt_chacha_out_filter_t : public t_chacha_base
  {
  public:
    typedef char                          char_type;
    struct category :
      public boost::iostreams::multichar_output_filter_tag,
      public boost::iostreams::flushable_tag
    { };

    encrypt_chacha_out_filter_t(std::string const &pass, const crypto::chacha_iv& iv) :t_chacha_base(pass, iv)
    {
    }
    encrypt_chacha_out_filter_t(std::string const& pass, const crypto::chacha_iv& iv, const char(&hdss)[32]) :t_chacha_base(pass, iv, hdss)
    {
    }

    ~encrypt_chacha_out_filter_t()
    {
    }

    template<typename t_sink>
    std::streamsize write(t_sink& snk, char_type const * const buf, std::streamsize const n) const
    {
      return t_chacha_base::process(buf, n, [&](char_type const * const buf_lambda, std::streamsize const n_lambda) {
        boost::iostreams::write(snk, &buf_lambda[0], n_lambda);
      });
    }

    template<typename Sink>
    bool flush(Sink& snk)
    {

      t_chacha_base::flush([&](char_type const * const buf_lambda, std::streamsize const n_lambda) {
        boost::iostreams::write(snk, &buf_lambda[0], n_lambda);
      });

      return true;
    }

  private:
  };


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  template<typename t_chacha_base>
  class encrypt_chacha_in_filter_t : public t_chacha_base
  {
  public:
    typedef char                          char_type;
    struct category : //public boost::iostreams::seekable_device_tag,
      public boost::iostreams::multichar_input_filter_tag,
      public boost::iostreams::flushable_tag
      //public boost::iostreams::seekable_filter_tag
    { };
    encrypt_chacha_in_filter_t(std::string const &pass, const crypto::chacha_iv& iv) :t_chacha_base(pass, iv), m_was_eof(false)
    {
    }
    encrypt_chacha_in_filter_t(std::string const& pass, const crypto::chacha_iv& iv, const char(&hdss)[32]) :t_chacha_base(pass, iv, hdss), m_was_eof(false)
    {

    }

    ~encrypt_chacha_in_filter_t()
    {
    }

//     template<typename Source>
//     std::streamsize read(Source& src, char* s, std::streamsize n)
//     {
//       std::streamsize actually_read = boost::iostreams::read(src, s, n);
//       if (actually_read > 0)
//       {
//         t_chacha_base::process(s, actually_read, [&](char_type const* const buf_lambda, std::streamsize const n_lambda) {
//           CHECK_AND_ASSERT_THROW_MES(n_lambda == actually_read, "Error in decrypt: check n_lambda == actually_read failed");
//           std::memcpy(s, buf_lambda, n_lambda);
//         });
//         m_total_read += actually_read;
//       }
// 
//       return actually_read;
//     }
    template<typename Source>
    std::streamsize read(Source& src, char* s, std::streamsize n)
    {
      if (m_encrypted_cache_buff.size() >= static_cast<size_t>(n))
      {
        return withdraw_from_encrypted_cache(s, n);
      }
      if (m_was_eof)
      {
        if (m_encrypted_cache_buff.empty())
        {
          return -1;
        }
        else
        {
          return withdraw_from_encrypted_cache(s, n);
        }
      }

      std::streamsize size_to_read_for_decrypt = (n - m_encrypted_cache_buff.size());
      size_to_read_for_decrypt += size_to_read_for_decrypt % t_chacha_base::block_size;
      size_t offset_in_buff = m_encrypted_cache_buff.size();
      m_encrypted_cache_buff.resize(m_encrypted_cache_buff.size() + size_to_read_for_decrypt);

      std::streamsize result = boost::iostreams::read(src, (char*)&m_encrypted_cache_buff.data()[offset_in_buff], size_to_read_for_decrypt);
      m_total_read += result;
      if (result == size_to_read_for_decrypt)
      {
        //regular read process, read data enough to get decrypted
        t_chacha_base::process(&m_encrypted_cache_buff.data()[offset_in_buff], size_to_read_for_decrypt, [&](char_type const* const buf_lambda, std::streamsize const n_lambda)
        {
          CHECK_AND_ASSERT_THROW_MES(n_lambda == size_to_read_for_decrypt, "Error in decrypt: check n_lambda == size_to_read_for_decrypt failed");
          std::memcpy((char*)&m_encrypted_cache_buff.data()[offset_in_buff], buf_lambda, n_lambda);
        });
        return withdraw_from_encrypted_cache(s, n);
      }
      else
      {
        //been read some size_but it's basically might be eof
        if (!m_was_eof)
        {
          size_t offset_before_flush = offset_in_buff;
          if (result != -1)
          {
            m_encrypted_cache_buff.resize(offset_in_buff + result);
            //eof
            t_chacha_base::process(&m_encrypted_cache_buff.data()[offset_in_buff], result, [&](char_type const* const buf_lambda, std::streamsize const n_lambda) {
              std::memcpy((char*)&m_encrypted_cache_buff.data()[offset_in_buff], buf_lambda, n_lambda);
              offset_before_flush = offset_in_buff + n_lambda;
            });
          }else
          {
            m_encrypted_cache_buff.resize(offset_in_buff);
          }

          t_chacha_base::flush([&](char_type const* const buf_lambda, std::streamsize const n_lambda) {
            if (n_lambda + offset_before_flush > m_encrypted_cache_buff.size())
            {
              m_encrypted_cache_buff.resize(n_lambda + offset_before_flush);
            }
            std::memcpy((char*)&m_encrypted_cache_buff.data()[offset_before_flush], buf_lambda, n_lambda);
            m_encrypted_cache_buff.resize(offset_before_flush + n_lambda);
          });

          //just to make sure that it's over
          std::string buff_stub(10, ' ');
          std::streamsize r = boost::iostreams::read(src, (char*)&buff_stub.data()[0], 10);
          CHECK_AND_ASSERT_THROW_MES(r == -1, "expected EOF");
          m_was_eof = true;
          return withdraw_from_encrypted_cache(s, n);
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

    std::streamsize withdraw_from_encrypted_cache(char* s, std::streamsize n)
    {

      size_t copy_size = m_encrypted_cache_buff.size() > static_cast<size_t>(n) ? static_cast<size_t>(n) : m_encrypted_cache_buff.size();
      std::memcpy(s, m_encrypted_cache_buff.data(), copy_size);
      m_encrypted_cache_buff.erase(0, copy_size);
      return copy_size;
    }   

    std::string m_encrypted_cache_buff;
    bool m_was_eof;

    //debug
    size_t m_total_read = 0;
  };


  typedef encrypt_chacha_in_filter_t<encrypt_chacha8_processer_base> encrypt_chacha8_in_filter;
  typedef encrypt_chacha_in_filter_t<encrypt_chacha20_processer_base> encrypt_chacha20_in_filter;

  typedef encrypt_chacha_out_filter_t<encrypt_chacha8_processer_base> encrypt_chacha8_out_filter;
  typedef encrypt_chacha_out_filter_t<encrypt_chacha20_processer_base> encrypt_chacha20_out_filter;

}