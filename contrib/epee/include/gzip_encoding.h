// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
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




#ifndef _GZIP_ENCODING_H_
#define _GZIP_ENCODING_H_
#include "boost/core/ignore_unused.hpp"
#include "net/http_client_base.h"
#include "zlib/zlib.h"


namespace epee
{
namespace net_utils
{



  class content_encoding_gzip: public i_sub_handler
  {
  public:
    /*! \brief
    *  Function content_encoding_gzip : Constructor
    *
    */
    inline 
      content_encoding_gzip(i_target_handler* powner_filter, bool is_deflate_mode = false, int compression_level = Z_DEFAULT_COMPRESSION) :m_powner_filter(powner_filter),
      m_is_stream_ended(false), 
      m_is_deflate_mode(is_deflate_mode),
      m_is_first_update_in(true)
    {
      memset(&m_zstream_in, 0, sizeof(m_zstream_in));
      memset(&m_zstream_out, 0, sizeof(m_zstream_out));
      int ret = 0;
      boost::ignore_unused(ret);
      if(is_deflate_mode)
      {
        ret = inflateInit(&m_zstream_in);  
        ret = deflateInit(&m_zstream_out, compression_level);
      }else
      {
        ret = inflateInit2(&m_zstream_in, 0x1F);
        ret = deflateInit2(&m_zstream_out, compression_level, Z_DEFLATED, 0x1F, 8, Z_DEFAULT_STRATEGY);
      }  
    }
    /*! \brief
    *  Function content_encoding_gzip : Destructor
    *
    */
    inline 
    ~content_encoding_gzip()
    {
      inflateEnd(& m_zstream_in );
      deflateEnd(& m_zstream_out );
    }
    /*! \brief
    *  Function update_in : Entry point for income data
    *
    */
    inline 
    virtual bool update_in( std::string& piece_of_transfer)
    {  

      bool is_first_time_here = m_is_first_update_in;
      m_is_first_update_in = false;

      if(m_pre_decode.size())
        m_pre_decode += piece_of_transfer;
      else
        m_pre_decode.swap(piece_of_transfer);
      piece_of_transfer.clear();

      std::string decode_summary_buff;

      size_t  ungzip_size = m_pre_decode.size() * 0x30;
      std::string current_decode_buff(ungzip_size, 'X');
      auto slh = misc_utils::create_scope_leave_handler([&]() { m_zstream_in.next_out = nullptr; } ); // make sure local pointer to current_decode_buff.data() won't be used out of this scope

      //Here the cycle is introduced where we unpack the buffer, the cycle is required
      //because of the case where if after unpacking the data will exceed the awaited size, we will not halt with error
      bool continue_unpacking = true;
      bool first_step = true;
      while(m_pre_decode.size() && continue_unpacking)
      {

        //fill buffers
        m_zstream_in.next_in = (Bytef*)m_pre_decode.data();
        m_zstream_in.avail_in = (uInt)m_pre_decode.size();
        m_zstream_in.next_out = (Bytef*)current_decode_buff.data();
        m_zstream_in.avail_out = (uInt)ungzip_size;

        int flag = Z_NO_FLUSH;
        int ret = inflate(&m_zstream_in, flag);
        CHECK_AND_ASSERT_MES(ret>=0 || m_zstream_in.avail_out ||m_is_deflate_mode, false, "content_encoding_gzip::update_in() Failed to inflate. ret = " << ret << ", msg: " << (m_zstream_in.msg ? m_zstream_in.msg : ""));

        if(Z_STREAM_END == ret)
          m_is_stream_ended = true;
        else if(Z_DATA_ERROR == ret && is_first_time_here && m_is_deflate_mode&& first_step)
        {
          // some servers (notably Apache with mod_deflate) don't generate zlib headers
          // insert a dummy header and try again
          static char dummy_head[2] =
          {
            0x8 + 0x7 * 0x10,
            (((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
          };
          inflateReset(&m_zstream_in);
          m_zstream_in.next_in = (Bytef*) dummy_head;
          m_zstream_in.avail_in = sizeof(dummy_head);

          ret = inflate(&m_zstream_in, Z_NO_FLUSH);
          if (ret != Z_OK)
          {
            LOCAL_ASSERT(0);
            m_pre_decode.swap(piece_of_transfer);
            return false;
          }
          m_zstream_in.next_in = (Bytef*)m_pre_decode.data();
          m_zstream_in.avail_in = (uInt)m_pre_decode.size();

          ret = inflate(&m_zstream_in, Z_NO_FLUSH);
          if (ret != Z_OK)
          {
            LOCAL_ASSERT(0);
            m_pre_decode.swap(piece_of_transfer);
            return false;
          }
        }
        else
        {
          CHECK_AND_ASSERT_MES(Z_DATA_ERROR != ret, false, "content_encoding_gzip::update_in() Failed to inflate. err = Z_DATA_ERROR");
        }


        //leave only unpacked part in the output buffer to start with it the next time
        m_pre_decode.erase(0, m_pre_decode.size()-m_zstream_in.avail_in);
        //if decoder gave nothing to return, then everything is ahead, now simply break
        if(ungzip_size == m_zstream_in.avail_out)
          break;

        //decode_buff currently stores data parts that were unpacked, fix this size
        current_decode_buff.resize(ungzip_size - m_zstream_in.avail_out);
        if(decode_summary_buff.size())
          decode_summary_buff += current_decode_buff;
        else
          current_decode_buff.swap(decode_summary_buff);

        current_decode_buff.resize(ungzip_size);
        first_step = false;
      }

      //Process these data if required
      return m_powner_filter->handle_target_data(decode_summary_buff);
    }
    /*! \brief
    *  Function stop : Entry point for stop signal and flushing cached data buffer.
    *
    */
    inline 
    virtual void stop(std::string& OUT collect_remains)
    {
    }
  protected:
  private:
    /*! \brief
    *  Pointer to parent HTTP-parser
    */
    i_target_handler* m_powner_filter;
    /*! \brief
    *  ZLIB object for income stream
    */
    z_stream    m_zstream_in;
    /*! \brief
    *  ZLIB object for outcome stream
    */
    z_stream    m_zstream_out;
    /*! \brief
    *  Data that could not be unpacked immediately, left to wait for the next packet of data
    */
    std::string m_pre_decode;
    /*! \brief
    *  The data are accumulated for a package in the buffer to send the web client
    */
    std::string m_pre_encode;
    /*! \brief
    *  Signals that stream looks like ended
    */
    bool    m_is_stream_ended;
    /*! \brief
    *  If this flag is set, income data is in HTTP-deflate mode
    */
    bool    m_is_deflate_mode;
    /*! \brief
    *  Marks that it is a first data packet 
    */
    bool    m_is_first_update_in;
  }; // class content_encoding_gzip

  struct abstract_callback_base
  {
    virtual bool do_call(const std::string& piece_of_transfer) = 0;
    virtual ~abstract_callback_base() {}
  };

  template <typename callback_t>
  struct abstract_callback : public abstract_callback_base
  {
    callback_t m_cb;

    abstract_callback(callback_t cb) : m_cb(cb){}
    virtual bool do_call(const std::string& piece_of_transfer)
    {
      return m_cb(piece_of_transfer);
    }
  };

  class gzip_decoder_lambda : public content_encoding_gzip,
                              public i_target_handler
  {
    std::shared_ptr<abstract_callback_base> m_pcb;

    virtual bool handle_target_data(std::string& piece_of_transfer)
    {
      bool r = m_pcb->do_call(piece_of_transfer);
      piece_of_transfer.clear();
      return r;
    }
  public: 
    gzip_decoder_lambda() : content_encoding_gzip(this, true, Z_BEST_COMPRESSION)
    {}
    
    template<class callback_t>
    bool update_in(std::string& piece_of_transfer, callback_t cb)
    {
      m_pcb.reset(new abstract_callback<callback_t>(cb));
      return content_encoding_gzip::update_in(piece_of_transfer);
    }
    template<class callback_t>
    bool stop(callback_t cb)
    {return true;}
  }; // class gzip_decoder_lambda

  class gzip_encoder_lyambda
  {
    bool m_initialized;
    z_stream  m_zstream;
  public: 
    gzip_encoder_lyambda(int compression_level = Z_DEFAULT_COMPRESSION) :m_initialized(false), m_zstream(AUTO_VAL_INIT(m_zstream))
    {
      int ret = deflateInit(&m_zstream, compression_level);
      if (ret == Z_OK)
        m_initialized = true;
    }

    ~gzip_encoder_lyambda()
    {
      deflateEnd(&m_zstream);
    }

    template<typename callback_t>
    bool update_in(const std::string& target, callback_t cb)
    {
      if (!m_initialized)
      {        
        return false;
      }
      
      if (!target.size())
      {
        return true;
      }

      std::string result_packed_buff;
      auto slh = misc_utils::create_scope_leave_handler([&]() { m_zstream.next_out = nullptr; } ); // make sure local pointer to result_packed_buff.data() won't be used out of this scope

      //theoretically it supposed to be smaller
      result_packed_buff.resize(target.size(), 'X');
      while (true)
      {
        m_zstream.next_in = (Bytef*)target.data();
        m_zstream.avail_in = (uInt)target.size();
        m_zstream.next_out = (Bytef*)result_packed_buff.data();
        m_zstream.avail_out = (uInt)result_packed_buff.size();

        int ret = deflate(&m_zstream, Z_NO_FLUSH);
        CHECK_AND_ASSERT_MES(ret >= 0, false, "Failed to deflate. err = " << ret);
        if (m_zstream.avail_out == 0)
        {
          //twice bigger buffer
          result_packed_buff.resize(result_packed_buff.size()*2);
          continue;
        }        
        CHECK_AND_ASSERT_MES(result_packed_buff.size() >= m_zstream.avail_out, false, "result_packed_buff.size()=" << result_packed_buff.size() << " >= m_zstream.avail_out=" << m_zstream.avail_out);
        result_packed_buff.resize(result_packed_buff.size() - m_zstream.avail_out);
        break;
      }
      
      return cb(result_packed_buff);
    }

    template<typename callback_t>
    bool stop(callback_t cb)
    {
      if (!m_initialized)
      {
        return false;
      }

      std::string result_packed_buff;
      //theoretically it supposed to be smaller
      result_packed_buff.resize(1000000, 'X');
      while (true)
      {
        m_zstream.next_in =  nullptr;
        m_zstream.avail_in = 0;
        m_zstream.next_out = (Bytef*)result_packed_buff.data();
        m_zstream.avail_out = (uInt)result_packed_buff.size();

        int ret = deflate(&m_zstream, Z_FINISH);
        CHECK_AND_ASSERT_MES(ret >= 0, false, "Failed to deflate at finish. err = " << ret);
        if (ret != Z_STREAM_END)
        {
          //twice bigger buffer
          result_packed_buff.resize(result_packed_buff.size() * 2);
          continue;
        }

        CHECK_AND_ASSERT_MES(result_packed_buff.size() >= m_zstream.avail_out, false, "result_packed_buff.size()=" << result_packed_buff.size() << " >= m_zstream.avail_out=" << m_zstream.avail_out);
        result_packed_buff.resize(result_packed_buff.size() - m_zstream.avail_out);

        m_initialized = false;
        break;
      }

      return cb(result_packed_buff);      
    }

  }; // class gzip_encoder_lyambda

} // namespace net_utils
} // namespace epee



#endif //_GZIP_ENCODING_H_
