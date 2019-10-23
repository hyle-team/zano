// Copyright (c) 2019, anonimal <anonimal@zano.org>
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

#pragma once
#include <atomic>
#include <unordered_map>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/smart_ptr/make_shared.hpp>

#include "levin_base.h"
#include "misc_language.h"
#include "profile_tools.h"

#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL "levin_protocol"
ENABLE_CHANNEL_BY_DEFAULT("levin_protocol");

namespace epee
{
namespace levin
{

/************************************************************************/
/*                                                                      */
/************************************************************************/
template<class t_connection_context>
class async_protocol_handler;

template<class t_connection_context>
class async_protocol_handler_config
{
  typedef std::unordered_map<boost::uuids::uuid, async_protocol_handler<t_connection_context>* , boost::hash<boost::uuids::uuid> > connections_map;
  critical_section m_connects_lock;
  std::atomic<bool> m_is_in_sendstop_loop;
  connections_map m_connects;

  void add_connection(async_protocol_handler<t_connection_context>* pc);
  void del_connection(async_protocol_handler<t_connection_context>* pc);

  async_protocol_handler<t_connection_context>* find_connection(boost::uuids::uuid connection_id) const;
  int find_and_lock_connection(boost::uuids::uuid connection_id, async_protocol_handler<t_connection_context>*& aph);

  friend class async_protocol_handler<t_connection_context>;

public:
  typedef t_connection_context connection_context;
  levin_commands_handler<t_connection_context>* m_pcommands_handler;
  uint64_t m_max_packet_size; 
  uint64_t m_invoke_timeout;

  void on_send_stop_signal();
  int invoke(int command, const std::string& in_buff, std::string& buff_out, boost::uuids::uuid connection_id);
  template<class callback_t>
  int invoke_async(int command, const std::string& in_buff, boost::uuids::uuid connection_id, const callback_t& cb, size_t timeout = LEVIN_DEFAULT_TIMEOUT_PRECONFIGURED);

  int notify(int command, const std::string& in_buff, boost::uuids::uuid connection_id);
  bool close(boost::uuids::uuid connection_id);
  bool update_connection_context(const t_connection_context& contxt);
  bool request_callback(boost::uuids::uuid connection_id);
  template<class callback_t>
  bool foreach_connection(callback_t cb);
  size_t get_connections_count();

  async_protocol_handler_config() :m_pcommands_handler(NULL), m_max_packet_size(LEVIN_DEFAULT_MAX_PACKET_SIZE), m_is_in_sendstop_loop(false), m_invoke_timeout{}
  {}
  ~async_protocol_handler_config()
  {
    NESTED_TRY_ENTRY();

    CRITICAL_REGION_LOCAL(m_connects_lock);
    m_connects.clear();

    NESTED_CATCH_ENTRY(__func__);
  }
};


/************************************************************************/
/*                                                                      */
/************************************************************************/
template<class t_connection_context = net_utils::connection_context_base>
class async_protocol_handler
{
public:
  typedef t_connection_context connection_context;
  typedef async_protocol_handler_config<t_connection_context> config_type;

  enum stream_state
  {
    stream_state_head,
    stream_state_body
  };

  std::atomic<bool> m_deletion_initiated;
  std::atomic<bool> m_protocol_released;
  volatile uint32_t m_invoke_buf_ready;

  volatile int m_invoke_result_code;

  critical_section m_local_inv_buff_lock;
  std::string m_local_inv_buff;

  critical_section m_send_lock;
  critical_section m_call_lock;

  volatile uint32_t m_wait_count;
  volatile uint32_t m_close_called;
  bucket_head2 m_current_head;
  net_utils::i_service_endpoint* m_pservice_endpoint; 
  config_type& m_config;
  t_connection_context& m_connection_context;

  std::string m_cache_in_buffer;
  stream_state m_state;

  int32_t m_oponent_protocol_ver;
  bool m_connection_initialized;

  struct invoke_response_handler_base
  {
    virtual bool handle(int res, const std::string& buff, connection_context& context)=0;
    virtual bool is_timer_started() const=0;
    virtual void cancel()=0;
    virtual bool cancel_timer()=0;
  };
  template <class callback_t>
  struct invoke_handler: invoke_response_handler_base
  {
    invoke_handler(const callback_t& cb, uint64_t timeout,  async_protocol_handler& con, int command)
      :m_cb(cb), m_con(con), m_timer(con.m_pservice_endpoint->get_io_service()), m_timer_started(false),
      m_cancel_timer_called(false), m_timer_cancelled(false), m_command(command)
    {
      if(m_con.start_outer_call())
      {
        m_timer.expires_from_now(boost::posix_time::milliseconds(timeout));
        m_timer.async_wait([&con, command, cb](const boost::system::error_code& ec)
        {
          if(ec == boost::asio::error::operation_aborted)
            return;
          LOG_PRINT_CC(con.get_context_ref(), "Timeout on invoke operation happened, command: " << command, LOG_LEVEL_2);
          std::string fake;
          cb(LEVIN_ERROR_CONNECTION_TIMEDOUT, fake, con.get_context_ref());
          con.close();
          con.finish_outer_call();
        });
        m_timer_started = true;
      }
    }
    virtual ~invoke_handler()
    {}
    callback_t m_cb;
    async_protocol_handler& m_con;
    boost::asio::deadline_timer m_timer;
    bool m_timer_started;
    std::atomic<bool> m_cancel_timer_called;
    bool m_timer_cancelled;
    int m_command;
    virtual bool handle(int res, const std::string& buff, typename async_protocol_handler::connection_context& context)
    {
      if(!cancel_timer())
        return false;
      m_cb(res, buff, context);
      m_con.finish_outer_call();
      return true;
    }
    virtual bool is_timer_started() const
    {
      return m_timer_started;
    }
    virtual void cancel()
    {
      if(cancel_timer())
      {
        std::string fake;
        m_cb(LEVIN_ERROR_CONNECTION_DESTROYED, fake, m_con.get_context_ref());
        m_con.finish_outer_call();
      }
    }
    virtual bool cancel_timer()
    {
      if(!m_cancel_timer_called)
      {
        m_cancel_timer_called = true;
        boost::system::error_code ignored_ec;
        m_timer_cancelled = 1 == m_timer.cancel(ignored_ec);
      }
      return m_timer_cancelled;
    }
  };
  critical_section m_invoke_response_handlers_lock;
  std::list<boost::shared_ptr<invoke_response_handler_base> > m_invoke_response_handlers;
  
  template<class callback_t>
  bool add_invoke_response_handler(const callback_t& cb, uint64_t timeout,  async_protocol_handler& con, int command)
  {
    CRITICAL_REGION_LOCAL(m_invoke_response_handlers_lock);
    if (m_protocol_released)
    {
      LOG_PRINT_L0("ERROR: Adding response handler to a released object");
      return false;
    }
    boost::shared_ptr<invoke_response_handler_base> handler(boost::make_shared<invoke_handler<callback_t>>(cb, timeout, con, command));
    m_invoke_response_handlers.push_back(handler);
    LOG_PRINT_L4("[LEVIN_PROTOCOL" << this << "] INVOKE_HANDLER_QUE: PUSH_BACK RESPONSE HANDLER");
    return handler->is_timer_started();
  }
  template<class callback_t> friend struct invoke_handler;
public:
  async_protocol_handler(net_utils::i_service_endpoint* psnd_hndlr, 
    config_type& config, 
    t_connection_context& conn_context):
            m_invoke_buf_ready{},
            m_invoke_result_code{},
            m_current_head(bucket_head2()),
            m_pservice_endpoint(psnd_hndlr), 
            m_config(config), 
            m_connection_context(conn_context), 
            m_state(stream_state_head)
  {
    m_close_called = 0;
    m_deletion_initiated = false;
    m_protocol_released = false;
    m_wait_count = 0;
    m_oponent_protocol_ver = 0;
    m_connection_initialized = false;
    LOG_PRINT_CC(m_connection_context, "[LEVIN_PROTOCOL" << this << "] CONSTRUCTED", LOG_LEVEL_4);
  }

  virtual ~async_protocol_handler()
  {
    NESTED_TRY_ENTRY();

    m_deletion_initiated = true;
    if(m_connection_initialized)
    {
      m_config.del_connection(this);
    }

    for (size_t i = 0; i < 60 * 1000 / 100 && 0 != boost::interprocess::ipcdetail::atomic_read32(&m_wait_count); ++i)
    {
      misc_utils::sleep_no_w(100);
    }
    CHECK_AND_ASSERT_MES_NO_RET(0 == boost::interprocess::ipcdetail::atomic_read32(&m_wait_count), "Failed to wait for operation completion. m_wait_count = " << m_wait_count);

    LOG_PRINT_CC(m_connection_context, "[LEVIN_PROTOCOL" << this << "] DESTRUCTING", LOG_LEVEL_4);

    VALIDATE_MUTEX_IS_FREE(m_local_inv_buff_lock);
    VALIDATE_MUTEX_IS_FREE(m_send_lock);
    VALIDATE_MUTEX_IS_FREE(m_call_lock);
    VALIDATE_MUTEX_IS_FREE(m_invoke_response_handlers_lock);

    NESTED_CATCH_ENTRY(__func__);
  }

  bool start_outer_call()
  {
    LOG_PRINT_CC_L4(m_connection_context, "[LEVIN_PROTOCOL" << this << "] -->> start_outer_call");
    if(!m_pservice_endpoint->add_ref())
    {
      LOG_PRINT_CC_RED(m_connection_context, "[LEVIN_PROTOCOL" << this << "] -->> start_outer_call failed", LOG_LEVEL_4);
      return false;
    }
    boost::interprocess::ipcdetail::atomic_inc32(&m_wait_count);
    return true;
  }
  bool finish_outer_call()
  {
    LOG_PRINT_CC_L4(m_connection_context, "[LEVIN_PROTOCOL" << this << "] <<-- finish_outer_call");
    boost::interprocess::ipcdetail::atomic_dec32(&m_wait_count);
    m_pservice_endpoint->release();
    return true;
  }

  bool release_protocol()
  {
    decltype(m_invoke_response_handlers) local_invoke_response_handlers;
    CRITICAL_REGION_BEGIN(m_invoke_response_handlers_lock);
    LOG_PRINT_L4("[LEVIN_PROTOCOL" << this << "] INVOKE_HANDLER_QUE: Invoke response_handlers(" << m_invoke_response_handlers.size()  <<") swap, protocol released");
    local_invoke_response_handlers.swap(m_invoke_response_handlers);
    m_protocol_released = true;
    CRITICAL_REGION_END();

    // Never call callback inside critical section, that can cause deadlock. Callback can be called when
    // invoke_response_handler_base is cancelled
    std::for_each(local_invoke_response_handlers.begin(), local_invoke_response_handlers.end(), [](const boost::shared_ptr<invoke_response_handler_base>& pinv_resp_hndlr) {
      pinv_resp_hndlr->cancel();
    });
    LOG_PRINT_L4("[LEVIN_PROTOCOL" << this << "] Invoke response_handlers(" << local_invoke_response_handlers.size() << ") canceled");
    return true;
  }
  
  bool close()
  {    
    boost::interprocess::ipcdetail::atomic_inc32(&m_close_called);

    m_pservice_endpoint->close();
    return true;
  }

  void update_connection_context(const connection_context& contxt)
  {
    m_connection_context = contxt;
  }

  void request_callback()
  {
    misc_utils::auto_scope_leave_caller scope_exit_handler = misc_utils::create_scope_leave_handler(
      boost::bind(&async_protocol_handler::finish_outer_call, this));

    m_pservice_endpoint->request_callback();
  }

  void handle_qued_callback()   
  {
    m_config.m_pcommands_handler->callback(m_connection_context);
  }

  virtual bool handle_recv(const void* ptr, size_t cb)
  {
    if(boost::interprocess::ipcdetail::atomic_read32(&m_close_called))
      return false; //closing connections
    
    //update threads name to see connection context where errors came from
    std::string original_prefix = epee::log_space::log_singletone::get_thread_log_prefix();
    epee::log_space::log_singletone::set_thread_log_prefix(original_prefix + "[" + epee::net_utils::print_connection_context_short(m_connection_context) + "]");
    ON_EXIT([&](){epee::log_space::log_singletone::set_thread_log_prefix(original_prefix); });

    //create_scope_leave_handler()

    if(!m_config.m_pcommands_handler)
    {
      LOG_ERROR_CC(m_connection_context, "[LEVIN_PROTOCOL" << this << "]Commands handler not set!");
      return false;
    }

    if(m_cache_in_buffer.size() +  cb > m_config.m_max_packet_size)
    {
      LOG_ERROR_CC(m_connection_context, "[LEVIN_PROTOCOL" << this << "]Maximum packet size exceed!, m_max_packet_size = " << m_config.m_max_packet_size 
                          << ", packet received " << m_cache_in_buffer.size() +  cb 
                          << ", connection will be closed.");
      return false;
    }

    m_cache_in_buffer.append((const char*)ptr, cb);

    bool is_continue = true;
    while(is_continue)
    {
      switch(m_state)
      {
      case stream_state_body:
        if(m_cache_in_buffer.size() < m_current_head.m_cb)
        {
          is_continue = false;
          break;
        }
        {
          std::string buff_to_invoke;
          if(m_cache_in_buffer.size()  == m_current_head.m_cb)
            buff_to_invoke.swap(m_cache_in_buffer);
          else
          {
            buff_to_invoke.assign(m_cache_in_buffer, 0, (std::string::size_type)m_current_head.m_cb);
            m_cache_in_buffer.erase(0, (std::string::size_type)m_current_head.m_cb);
          }

          bool is_response = (m_oponent_protocol_ver == LEVIN_PROTOCOL_VER_1 && m_current_head.m_flags&LEVIN_PACKET_RESPONSE);


          LOG_PRINT_CC_L4(m_connection_context, "[LEVIN_PROTOCOL" << this << "] RECIEVED PACKET [" << (is_response ? ("RESP"):(m_current_head.m_have_to_return_data? "-->INVOKE":"-->NOTIFY")) << "][len=" << m_current_head.m_cb
            << ", flags" << m_current_head.m_flags 
            << ", r?=" << m_current_head.m_have_to_return_data 
            <<", cmd = " << m_current_head.m_command 
            << ", v=" << m_current_head.m_protocol_version);

          if(is_response)
          {//response to some invoke 
            CRITICAL_REGION_LOCAL_VAR(m_invoke_response_handlers_lock, invoke_response_handlers_guard);
            if(!m_invoke_response_handlers.empty())
            {//async call scenario
              boost::shared_ptr<invoke_response_handler_base> response_handler = m_invoke_response_handlers.front();
              bool timer_cancelled = response_handler->cancel_timer();
              
              if (timer_cancelled)
              {
                LOG_PRINT_L4("[LEVIN_PROTOCOL" << this << "] INVOKE_HANDLER_QUE: POP_FRONT");
                m_invoke_response_handlers.pop_front();                
              }
              invoke_response_handlers_guard.unlock();//manual unlock(this type of guard let manual unlock)

              if(timer_cancelled)
                response_handler->handle(m_current_head.m_return_code, buff_to_invoke, m_connection_context);
            }
            else
            {
              invoke_response_handlers_guard.unlock();//manual unlock(this type of guard let manual unlock)
              //use sync call scenario
              if(!boost::interprocess::ipcdetail::atomic_read32(&m_wait_count) && !boost::interprocess::ipcdetail::atomic_read32(&m_close_called))
              {
                if(!m_protocol_released)
                {
                  LOG_ERROR_CC(m_connection_context, "[LEVIN_PROTOCOL" << this << "]no active invoke when response came, wtf?");
                }
                return false;
              }else
              {
                CRITICAL_REGION_BEGIN(m_local_inv_buff_lock);
                buff_to_invoke.swap(m_local_inv_buff);
                buff_to_invoke.clear();
                m_invoke_result_code = m_current_head.m_return_code;
                CRITICAL_REGION_END();
                boost::interprocess::ipcdetail::atomic_write32(&m_invoke_buf_ready, 1);
              }
            }
          }else
          {
            if(m_current_head.m_have_to_return_data)
            {
              std::string return_buff;
              TIME_MEASURE_START_MS(invoke_handle_time);
              m_current_head.m_return_code = m_config.m_pcommands_handler->invoke(
                                                                  m_current_head.m_command, 
                                                                  buff_to_invoke, 
                                                                  return_buff, 
                                                                  m_connection_context);
              TIME_MEASURE_FINISH_MS(invoke_handle_time);
              LOG_PRINT_CC_L3(m_connection_context, "[LEVIN_PROTOCOL" << this << "] INVOKE HANDLER: " << invoke_handle_time << "ms, command: " << m_current_head.m_command);
              if (invoke_handle_time > m_config.m_invoke_timeout / 2)
              {
                LOG_PRINT_CC_RED(m_connection_context, "[LEVIN_PROTOCOL" << this << "] LONG INVOKE HANDLER: " << invoke_handle_time << "ms, command: " << m_current_head.m_command, LOG_LEVEL_0);
              }

              m_current_head.m_cb = return_buff.size();
              m_current_head.m_have_to_return_data = false;
              m_current_head.m_protocol_version = LEVIN_PROTOCOL_VER_1;
              m_current_head.m_flags = LEVIN_PACKET_RESPONSE;
              std::string send_buff((const char*)&m_current_head, sizeof(m_current_head));
              send_buff += return_buff;
              CRITICAL_REGION_BEGIN(m_send_lock);
              if(!m_pservice_endpoint->do_send(send_buff.data(), send_buff.size()))
                return false;
              CRITICAL_REGION_END();
              LOG_PRINT_CC_L4(m_connection_context, "[LEVIN_PROTOCOL" << this << "] PACKET_SENT. [len=" << m_current_head.m_cb 
                << ", flags" << m_current_head.m_flags 
                << ", r?=" << m_current_head.m_have_to_return_data 
                <<", cmd = " << m_current_head.m_command 
                << ", ver=" << m_current_head.m_protocol_version);
            }
            else
            {
              
              TIME_MEASURE_START_MS(notify_handle_time);
              m_config.m_pcommands_handler->notify(m_current_head.m_command, buff_to_invoke, m_connection_context);
              TIME_MEASURE_FINISH_MS(notify_handle_time);
              LOG_PRINT_CC_L3(m_connection_context, "[LEVIN_PROTOCOL" << this << "] NOTIFY HANDLER: " << notify_handle_time << "ms, command: " << m_current_head.m_command);
              if (notify_handle_time > m_config.m_invoke_timeout / 2)
              {
                LOG_PRINT_CC_RED(m_connection_context, "[LEVIN_PROTOCOL" << this << "] LONG NOTIFY HANDLER: " << notify_handle_time << "ms, command: " << m_current_head.m_command, LOG_LEVEL_0);
              }

            }
              
          }
        }
        m_state = stream_state_head;
        break;
      case stream_state_head:
        {
          if(m_cache_in_buffer.size() < sizeof(bucket_head2))
          {
            if(m_cache_in_buffer.size() >= sizeof(uint64_t) && *((uint64_t*)m_cache_in_buffer.data()) != LEVIN_SIGNATURE)
            {
              LOG_PRINT_CC_L0(m_connection_context, "[LEVIN_PROTOCOL" << this << "] Signature mismatch, connection will be closed");
              return false;
            }
            is_continue = false;
            break;
          }

          bucket_head2* phead = (bucket_head2*)m_cache_in_buffer.data();
          if(LEVIN_SIGNATURE != phead->m_signature)
          {
            LOG_PRINT_CC_L0(m_connection_context, "[LEVIN_PROTOCOL" << this << "] Signature mismatch, connection will be closed");
            return false;
          }
          m_current_head = *phead;

          m_cache_in_buffer.erase(0, sizeof(bucket_head2));
          m_state = stream_state_body;
          LOG_PRINT_L4("[LEVIN_PROTOCOL" << this << "] RECEIVED_HEADER(command " << m_current_head.m_command << ")");
          m_oponent_protocol_ver = m_current_head.m_protocol_version;
          if(m_current_head.m_cb > m_config.m_max_packet_size)
          {
            LOG_ERROR_CC(m_connection_context, "[LEVIN_PROTOCOL" << this << "]Maximum packet size exceed!, m_max_packet_size = " << m_config.m_max_packet_size 
              << ", packet header received " << m_current_head.m_cb 
              << ", connection will be closed.");
            return false;
          }
        }
        break;
      default:
        LOG_ERROR_CC(m_connection_context, "[LEVIN_PROTOCOL" << this << "]Undefined state in levin_server_impl::connection_handler, m_state=" << m_state);
        return false;
      }
    }

    return true;
  }

  bool after_init_connection()
  {
    if (!m_connection_initialized)
    {
      m_connection_initialized = true;
      m_config.add_connection(this);
    }
    return true;
  }

  template<class callback_t>
  bool async_invoke(int command, const std::string& in_buff, const callback_t& cb, size_t timeout = LEVIN_DEFAULT_TIMEOUT_PRECONFIGURED)
  {
    misc_utils::auto_scope_leave_caller scope_exit_handler = misc_utils::create_scope_leave_handler(
      boost::bind(&async_protocol_handler::finish_outer_call, this));

    if(timeout == LEVIN_DEFAULT_TIMEOUT_PRECONFIGURED)
      timeout = static_cast<size_t>(m_config.m_invoke_timeout);

    int err_code = LEVIN_OK;
    do
    {
      if(m_deletion_initiated)
      {
        err_code = LEVIN_ERROR_CONNECTION_DESTROYED;
        break;
      }

      CRITICAL_REGION_LOCAL(m_call_lock);

      if(m_deletion_initiated)
      {
        err_code = LEVIN_ERROR_CONNECTION_DESTROYED;
        break;
      }

      bucket_head2 head = {0};
      head.m_signature = LEVIN_SIGNATURE;
      head.m_cb = in_buff.size();
      head.m_have_to_return_data = true;

      head.m_flags = LEVIN_PACKET_REQUEST;
      head.m_command = command;
      head.m_protocol_version = LEVIN_PROTOCOL_VER_1;

      boost::interprocess::ipcdetail::atomic_write32(&m_invoke_buf_ready, 0);
      CRITICAL_REGION_BEGIN(m_send_lock);
      CRITICAL_REGION_LOCAL1(m_invoke_response_handlers_lock);
      if(!m_pservice_endpoint->do_send(&head, sizeof(head)))
      {
        LOG_PRINT_CC_RED(m_connection_context, "[LEVIN_PROTOCOL" << this << "]Failed to do_send", LOG_LEVEL_2);
        err_code = LEVIN_ERROR_CONNECTION;
        break;
      }
      LOG_PRINT_L4("[LEVIN_PROTOCOL" << this << "] SENT_HEADER(command " << command << ")");
      
      
      //add response handler before do_send of the last 
      //part of packet, in case if it somehow lead to situation when response 
      //comes before it return control and response could be handled 
      //by protocol state machine without proper invoke_response_handler
      if (!add_invoke_response_handler(cb, timeout, *this, command))
      {
        err_code = LEVIN_ERROR_CONNECTION_DESTROYED;
        break;
      }
      LOG_PRINT_L4("[LEVIN_PROTOCOL" << this << "] ADD_INVOKE_HANDLER(command " << command << ")");


      if(!m_pservice_endpoint->do_send(in_buff.data(), (int)in_buff.size()))
      {
        LOG_PRINT_CC_RED(m_connection_context, "Failed to do_send", LOG_LEVEL_2);
        err_code = LEVIN_ERROR_CONNECTION;
        break;
      }
      LOG_PRINT_L4("[LEVIN_PROTOCOL" << this << "] SENT_BODY(command " << command << ")");


      CRITICAL_REGION_END();
    } while (false);

    if (LEVIN_OK != err_code)
    {
      std::string stub_buff;
      // Never call callback inside critical section, that can cause deadlock
      cb(err_code, stub_buff, m_connection_context);
      return false;
    }

    return true;
  }

  int invoke(int command, const std::string& in_buff, std::string& buff_out)
  {
    misc_utils::auto_scope_leave_caller scope_exit_handler = misc_utils::create_scope_leave_handler(
                                      boost::bind(&async_protocol_handler::finish_outer_call, this));

    if(m_deletion_initiated)
      return LEVIN_ERROR_CONNECTION_DESTROYED;

    CRITICAL_REGION_LOCAL(m_call_lock);

    if(m_deletion_initiated)
      return LEVIN_ERROR_CONNECTION_DESTROYED;

    bucket_head2 head = {0};
    head.m_signature = LEVIN_SIGNATURE;
    head.m_cb = in_buff.size();
    head.m_have_to_return_data = true;

    head.m_flags = LEVIN_PACKET_REQUEST;
    head.m_command = command;
    head.m_protocol_version = LEVIN_PROTOCOL_VER_1;

    boost::interprocess::ipcdetail::atomic_write32(&m_invoke_buf_ready, 0);
    CRITICAL_REGION_BEGIN(m_send_lock);
    if(!m_pservice_endpoint->do_send(&head, sizeof(head)))
    {
      LOG_ERROR_CC(m_connection_context, "[LEVIN_PROTOCOL" << this << "]Failed to do_send");
      return LEVIN_ERROR_CONNECTION;
    }

    if(!m_pservice_endpoint->do_send(in_buff.data(), (int)in_buff.size()))
    {
      LOG_ERROR_CC(m_connection_context, "[LEVIN_PROTOCOL" << this << "]Failed to do_send");
      return LEVIN_ERROR_CONNECTION;
    }
    CRITICAL_REGION_END();

    LOG_PRINT_CC_L4(m_connection_context, "[LEVIN_PROTOCOL" << this << "] PACKET_SENT. [len=" << head.m_cb 
                            << ", f=" << head.m_flags 
                            << ", r?=" << head.m_have_to_return_data 
                            << ", cmd = " << head.m_command 
                            << ", ver=" << head.m_protocol_version);

    uint64_t ticks_start = misc_utils::get_tick_count();

    while(!boost::interprocess::ipcdetail::atomic_read32(&m_invoke_buf_ready) && !m_deletion_initiated && !m_protocol_released)
    {
      if(misc_utils::get_tick_count() - ticks_start > m_config.m_invoke_timeout)
      {
        LOG_PRINT_CC_L2(m_connection_context, "[LEVIN_PROTOCOL" << this << "] invoke timeout (" << m_config.m_invoke_timeout << "), closing connection ");
        close();
        return LEVIN_ERROR_CONNECTION_TIMEDOUT;
      }
      if(!m_pservice_endpoint->call_run_once_service_io())
        return LEVIN_ERROR_CONNECTION_DESTROYED;
    }

    if(m_deletion_initiated || m_protocol_released)
      return LEVIN_ERROR_CONNECTION_DESTROYED;

    CRITICAL_REGION_BEGIN(m_local_inv_buff_lock);
    buff_out.swap(m_local_inv_buff);
    m_local_inv_buff.clear();
    CRITICAL_REGION_END();

    return m_invoke_result_code;
  }

  int notify(int command, const std::string& in_buff)
  {
    misc_utils::auto_scope_leave_caller scope_exit_handler = misc_utils::create_scope_leave_handler(
                          boost::bind(&async_protocol_handler::finish_outer_call, this));

    if(m_deletion_initiated)
      return LEVIN_ERROR_CONNECTION_DESTROYED;

    CRITICAL_REGION_LOCAL(m_call_lock);

    if(m_deletion_initiated)
      return LEVIN_ERROR_CONNECTION_DESTROYED;

    bucket_head2 head = {0};
    head.m_signature = LEVIN_SIGNATURE;
    head.m_have_to_return_data = false;
    head.m_cb = in_buff.size();

    head.m_command = command;
    head.m_protocol_version = LEVIN_PROTOCOL_VER_1;
    head.m_flags = LEVIN_PACKET_REQUEST;
    CRITICAL_REGION_BEGIN(m_send_lock);
    if(!m_pservice_endpoint->do_send(&head, sizeof(head)))
    {
      LOG_PRINT_CC_RED(m_connection_context, "[LEVIN_PROTOCOL" << this << "]Failed to do_send()", LOG_LEVEL_2);
      return -1;
    }

    if(!m_pservice_endpoint->do_send(in_buff.data(), (int)in_buff.size()))
    {
      LOG_PRINT_CC_RED(m_connection_context, "[LEVIN_PROTOCOL" << this << "]Failed to do_send()", LOG_LEVEL_2);
      return -1;
    }
    CRITICAL_REGION_END();
    LOG_PRINT_CC_L4(m_connection_context, "[LEVIN_PROTOCOL" << this << "] PACKET_SENT. [len=" << head.m_cb << 
      ", f=" << head.m_flags << 
      ", r?=" << head.m_have_to_return_data <<
      ", cmd = " << head.m_command << 
      ", ver=" << head.m_protocol_version);

    return 1;
  }
  //------------------------------------------------------------------------------------------
  boost::uuids::uuid get_connection_id() {return m_connection_context.m_connection_id;}
  //------------------------------------------------------------------------------------------
  t_connection_context& get_context_ref() {return m_connection_context;}
};
//------------------------------------------------------------------------------------------
template<class t_connection_context>
void async_protocol_handler_config<t_connection_context>::del_connection(async_protocol_handler<t_connection_context>* pconn)
{
  CRITICAL_REGION_BEGIN(m_connects_lock);
  //in case if ->close() leads to phisical erasing any connections in the same thread - just ignore it
  
  if (m_is_in_sendstop_loop == true)
  {
    //LOG_ERROR("Internal problem on close connections: attempt to erase connection object while on_send_stop_signal loop is working");
    // edit2: as soon as we added local copy of map of pointers in caller function, now we can erase it
    LOG_PRINT_L0(" [LEVIN_PROTOCOL_CONFIG]NOTICE: ERASING CONNECTION FROM SAME THREAD (assuming that local copy in caller function should handle it) In case of crush look at this code.");
    m_connects.erase(pconn->get_connection_id());
  }
  else
  {
    m_connects.erase(pconn->get_connection_id());
  }  
  CRITICAL_REGION_END();
  m_pcommands_handler->on_connection_close(pconn->m_connection_context);
}
//------------------------------------------------------------------------------------------
template<class t_connection_context>
void async_protocol_handler_config<t_connection_context>::add_connection(async_protocol_handler<t_connection_context>* pconn)
{
  CRITICAL_REGION_BEGIN(m_connects_lock);
  m_connects[pconn->get_connection_id()] = pconn;
  CRITICAL_REGION_END();
  m_pcommands_handler->on_connection_new(pconn->m_connection_context);
}
//------------------------------------------------------------------------------------------
template<class t_connection_context>
async_protocol_handler<t_connection_context>* async_protocol_handler_config<t_connection_context>::find_connection(boost::uuids::uuid connection_id) const
{
  auto it = m_connects.find(connection_id);
  return it == m_connects.end() ? 0 : it->second;
}
//------------------------------------------------------------------------------------------
template<class t_connection_context>
int async_protocol_handler_config<t_connection_context>::find_and_lock_connection(boost::uuids::uuid connection_id, async_protocol_handler<t_connection_context>*& aph)
{
  CRITICAL_REGION_LOCAL(m_connects_lock);
  aph = find_connection(connection_id);
  if(0 == aph)
    return LEVIN_ERROR_CONNECTION_NOT_FOUND;
  if(!aph->start_outer_call())
    return LEVIN_ERROR_CONNECTION_DESTROYED;
  return LEVIN_OK;
}
//------------------------------------------------------------------------------------------
template<class t_connection_context>
int async_protocol_handler_config<t_connection_context>::invoke(int command, const std::string& in_buff, std::string& buff_out, boost::uuids::uuid connection_id)
{
  async_protocol_handler<t_connection_context>* aph;
  int r = find_and_lock_connection(connection_id, aph);
  return LEVIN_OK == r ? aph->invoke(command, in_buff, buff_out) : r;
}
//------------------------------------------------------------------------------------------
template<class t_connection_context> template<class callback_t>
int async_protocol_handler_config<t_connection_context>::invoke_async(int command, const std::string& in_buff, boost::uuids::uuid connection_id, const callback_t& cb, size_t timeout)
{
  async_protocol_handler<t_connection_context>* aph;
  int r = find_and_lock_connection(connection_id, aph);
  return LEVIN_OK == r ? aph->async_invoke(command, in_buff, cb, timeout) : r;
}
//------------------------------------------------------------------------------------------
template<class t_connection_context> template<class callback_t>
bool async_protocol_handler_config<t_connection_context>::foreach_connection(callback_t cb)
{
  CRITICAL_REGION_LOCAL(m_connects_lock);
  for(auto& c: m_connects)
  {
    async_protocol_handler<t_connection_context>* aph = c.second;
    if(!cb(aph->get_context_ref()))
      return false;
  }
  return true;
}
//------------------------------------------------------------------------------------------
template<class t_connection_context>
void async_protocol_handler_config<t_connection_context>::on_send_stop_signal()
{
  //at the moment we put workaround here, since this sync_handler has collection of connections
  CRITICAL_REGION_LOCAL(m_connects_lock);
  m_is_in_sendstop_loop = true;
  /* Some times aph->close() can lead to call del_connection() wich erase 
  item from m_connects and this leads to undefined behavior in range-based for loop.
  Since connections_map is light container we just do local copy of this for enumeration
  */
  connections_map local_lonnects = m_connects;

  for (auto& c : local_lonnects)
  {
    async_protocol_handler<t_connection_context>* aph = c.second;
    aph->close();
  }
  m_is_in_sendstop_loop = false;
}
//------------------------------------------------------------------------------------------
template<class t_connection_context>
size_t async_protocol_handler_config<t_connection_context>::get_connections_count()
{
  CRITICAL_REGION_LOCAL(m_connects_lock);
  return m_connects.size();
}
//------------------------------------------------------------------------------------------
template<class t_connection_context>
int async_protocol_handler_config<t_connection_context>::notify(int command, const std::string& in_buff, boost::uuids::uuid connection_id)
{
  async_protocol_handler<t_connection_context>* aph;
  int r = find_and_lock_connection(connection_id, aph);
  return LEVIN_OK == r ? aph->notify(command, in_buff) : r;
}
//------------------------------------------------------------------------------------------
template<class t_connection_context>
bool async_protocol_handler_config<t_connection_context>::close(boost::uuids::uuid connection_id)
{
  CRITICAL_REGION_LOCAL(m_connects_lock);
  async_protocol_handler<t_connection_context>* aph = find_connection(connection_id);
  return 0 != aph ? aph->close() : false;
}
//------------------------------------------------------------------------------------------
template<class t_connection_context>
bool async_protocol_handler_config<t_connection_context>::update_connection_context(const t_connection_context& contxt)
{
  CRITICAL_REGION_LOCAL(m_connects_lock);
  async_protocol_handler<t_connection_context>* aph = find_connection(contxt.m_connection_id);
  if(0 == aph)
    return false;
  aph->update_connection_context(contxt);
  return true;
}
//------------------------------------------------------------------------------------------
template<class t_connection_context>
bool async_protocol_handler_config<t_connection_context>::request_callback(boost::uuids::uuid connection_id)
{
  async_protocol_handler<t_connection_context>* aph;
  int r = find_and_lock_connection(connection_id, aph);
  if(LEVIN_OK == r)
  {
    aph->request_callback();
    return true;
  }
  else
  {
    return false;
  }
}
}
}


#undef LOG_DEFAULT_CHANNEL 
#define LOG_DEFAULT_CHANNEL NULL




