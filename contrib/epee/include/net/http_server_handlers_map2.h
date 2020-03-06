// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//     * Neither the name of the Andrey N. Sabelnikov nor the
//     names of its contributors may be used to endorse or promote products
//     derived from this software without specific prior written permission.
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
#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage_template_helper.h"
#include "http_base.h"

template<typename request_t, typename response_t>
bool auto_doc_t(const std::string& prefix_name, std::string& generate_reference)
{
  if (!generate_reference.size()) return true;
  request_t req = AUTO_VAL_INIT(req);
  response_t res = AUTO_VAL_INIT(res);
  std::stringstream ss;
  ss << prefix_name << ENDL
    << "REQUEST: " << ENDL << epee::serialization::store_t_to_json(req) << ENDL <<  "--------------------------------" << ENDL
    << "RESPONSE: " << ENDL << epee::serialization::store_t_to_json(res) << ENDL << "################################" << ENDL;
  generate_reference += ss.str();
  return true;
}


template<typename command_type_t>
bool auto_doc(const std::string& prefix_name, std::string& generate_reference)
{
  return auto_doc_t<typename command_type_t::request, typename command_type_t::response>(prefix_name, generate_reference);
}


#define CHAIN_HTTP_TO_MAP2(context_type) bool handle_http_request(const epee::net_utils::http::http_request_info& query_info, \
              epee::net_utils::http::http_response_info& response, \
              context_type& m_conn_context) \
{\
  response.m_response_code = 200; \
  response.m_response_comment = "Ok"; \
  std::string reference_stub; \
  bool call_found = false; \
  if(!handle_http_request_map(query_info, response, m_conn_context, call_found, reference_stub) && response.m_response_code == 200) \
  { response.m_response_code = 500; response.m_response_comment = "Internal Server Error"; return true; } \
  if (!call_found) \
  { response.m_response_code = 404; response.m_response_comment = "Not Found"; return true; } \
  return true; \
}

#define BEGIN_URI_MAP2()   template<class t_context> bool handle_http_request_map(const epee::net_utils::http::http_request_info& query_info, \
  epee::net_utils::http::http_response_info& response_info, \
  t_context& m_conn_context, bool& call_found, std::string& generate_reference) { \
  call_found = false; \
  if(false) return true; //just a stub to have "else if"

#define MAP_URI2(pattern, callback)  else if(std::string::npos != query_info.m_URI.find(pattern)) return callback(query_info, response_info, m_conn_context);

#define MAP_URI_AUTO_XML2(s_pattern, callback_f, command_type) //TODO: don't think i ever again will use xml - ambiguous and "overtagged" format

#define MAP_URI_AUTO_JON2(s_pattern, callback_f, command_type) \
    else if(auto_doc<command_type>(s_pattern "[JSON]", generate_reference) && query_info.m_URI == s_pattern) \
    { \
      call_found = true; \
      uint64_t ticks = misc_utils::get_tick_count(); \
      boost::value_initialized<command_type::request> req; \
      bool res = epee::serialization::load_t_from_json(static_cast<command_type::request&>(req), query_info.m_body); \
      CHECK_AND_ASSERT_MES(res, false, "Failed to parse json: \r\n" << query_info.m_body); \
      uint64_t ticks1 = epee::misc_utils::get_tick_count(); \
      boost::value_initialized<command_type::response> resp;\
      res = callback_f(static_cast<command_type::request&>(req), static_cast<command_type::response&>(resp), m_conn_context); \
      CHECK_AND_ASSERT_MES(res, false, "Failed to call " << #callback_f << "() while handling " << s_pattern); \
      uint64_t ticks2 = epee::misc_utils::get_tick_count(); \
      epee::serialization::store_t_to_json(static_cast<command_type::response&>(resp), response_info.m_body); \
      uint64_t ticks3 = epee::misc_utils::get_tick_count(); \
      response_info.m_mime_tipe = "application/json"; \
      response_info.m_header_info.m_content_type = " application/json"; \
      LOG_PRINT("[HTTP/JSON][" << epee::string_tools::get_ip_string_from_int32(m_conn_context.m_remote_ip ) << "][" << query_info.m_URI << "] processed with " << ticks1-ticks << "/"<< ticks2-ticks1 << "/" << ticks3-ticks2 << "ms", LOG_LEVEL_2); \
    }

#define MAP_URI_AUTO_BIN2(s_pattern, callback_f, command_type) \
    else if(auto_doc<command_type>(s_pattern "[BIN]", generate_reference) && query_info.m_URI == s_pattern) \
    { \
      call_found = true; \
      uint64_t ticks = misc_utils::get_tick_count(); \
      boost::value_initialized<command_type::request> req; \
      bool res = epee::serialization::load_t_from_binary(static_cast<command_type::request&>(req), query_info.m_body); \
      CHECK_AND_ASSERT_MES(res, false, "Failed to parse bin body data, body size=" << query_info.m_body.size()); \
      uint64_t ticks1 = misc_utils::get_tick_count(); \
      boost::value_initialized<command_type::response> resp;\
      res = callback_f(static_cast<command_type::request&>(req), static_cast<command_type::response&>(resp), m_conn_context); \
      CHECK_AND_ASSERT_MES(res, false, "Failed to call " << #callback_f << "() while handling " << s_pattern); \
      uint64_t ticks2 = misc_utils::get_tick_count(); \
      epee::serialization::store_t_to_binary(static_cast<command_type::response&>(resp), response_info.m_body); \
      uint64_t ticks3 = epee::misc_utils::get_tick_count(); \
      response_info.m_mime_tipe = " application/octet-stream"; \
      response_info.m_header_info.m_content_type = " application/octet-stream"; \
      LOG_PRINT( "[HTTP/BIN][" << epee::string_tools::get_ip_string_from_int32(m_conn_context.m_remote_ip ) << "][" << query_info.m_URI << "] processed with " << ticks1-ticks << "/"<< ticks2-ticks1 << "/" << ticks3-ticks2 << "ms", LOG_LEVEL_2); \
    }

#define CHAIN_URI_MAP2(callback) else {callback(query_info, response_info, m_conn_context);call_found = true;}

#define END_URI_MAP2() return true;}




namespace epee 
{
  namespace json_rpc
  {
    template<typename t_param>
    struct request
    {
      std::string jsonrpc;
      std::string method;
      epee::serialization::storage_entry id;
      t_param     params;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(jsonrpc)
        KV_SERIALIZE(id)
        KV_SERIALIZE(method)
        KV_SERIALIZE(params)
      END_KV_SERIALIZE_MAP()
    };

    struct error
    {
      int64_t code;
      std::string message;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(code)
        KV_SERIALIZE(message)
      END_KV_SERIALIZE_MAP()
    };
    
    struct dummy_error
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct dummy_result
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    template<typename t_param, typename t_error>
    struct response
    {
      std::string jsonrpc;
      t_param     result;
      epee::serialization::storage_entry id;
      t_error     error;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(jsonrpc)
        KV_SERIALIZE(id)
        KV_SERIALIZE(result)
        KV_SERIALIZE(error)
      END_KV_SERIALIZE_MAP()
    };

    template<typename t_param>
    struct response<t_param, dummy_error>
    {
      std::string jsonrpc;
      t_param     result;
      epee::serialization::storage_entry id;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(jsonrpc)
        KV_SERIALIZE(id)
        KV_SERIALIZE(result)
      END_KV_SERIALIZE_MAP()
    };

    template<typename t_error>
    struct response<dummy_result, t_error>
    {
      std::string jsonrpc;
      std::string method;
      t_error     error;
      epee::serialization::storage_entry id;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(jsonrpc)
        KV_SERIALIZE(method)
        KV_SERIALIZE(id)
        KV_SERIALIZE(error)
      END_KV_SERIALIZE_MAP()
    };

    typedef response<dummy_result, error> error_response;
  }
}

template<typename command_type_t>
struct json_command_type_t
{
  typedef typename epee::json_rpc::request<typename command_type_t::request> request;
  typedef typename epee::json_rpc::request<typename command_type_t::response> response;
};

#define JSON_RPC_REFERENCE_MARKER          "JSON_RPC"



#define BEGIN_JSON_RPC_MAP(uri)    else if(query_info.m_URI == JSON_RPC_REFERENCE_MARKER || query_info.m_URI == uri) \
    { \
    if(query_info.m_URI == JSON_RPC_REFERENCE_MARKER) {generate_reference = "JSON RPC URL: " uri "\n";} \
    LOG_PRINT_L4("[JSON_REQUEST_BODY]: " << ENDL << query_info.m_body); \
    uint64_t ticks = epee::misc_utils::get_tick_count(); \
    epee::serialization::portable_storage ps; \
    if(!ps.load_from_json(query_info.m_body)) \
    { \
       boost::value_initialized<epee::json_rpc::error_response> rsp; \
       static_cast<epee::json_rpc::error_response&>(rsp).error.code = -32700; \
       static_cast<epee::json_rpc::error_response&>(rsp).error.message = "Parse error"; \
       epee::serialization::store_t_to_json(static_cast<epee::json_rpc::error_response&>(rsp), response_info.m_body); \
       return true; \
    } \
    epee::serialization::storage_entry id_; \
    id_ = epee::serialization::storage_entry(std::string()); \
    ps.get_value("id", id_, nullptr); \
    std::string callback_name; \
    if(!ps.get_value("method", callback_name, nullptr)) \
    { \
      epee::json_rpc::error_response rsp; \
      rsp.jsonrpc = "2.0"; \
      rsp.error.code = -32600; \
      rsp.error.message = "Invalid Request"; \
      epee::serialization::store_t_to_json(static_cast<epee::json_rpc::error_response&>(rsp), response_info.m_body); \
      return true; \
    } \
    if(false) return true; //just a stub to have "else if"




#define PREPARE_OBJECTS_FROM_JSON(command_type) \
  call_found = true; \
  boost::value_initialized<epee::json_rpc::request<command_type::request> > req_; \
  epee::json_rpc::request<command_type::request>& req = static_cast<epee::json_rpc::request<command_type::request>&>(req_);\
  if(!req.load(ps)) \
  { \
    epee::json_rpc::error_response fail_resp = AUTO_VAL_INIT(fail_resp); \
    fail_resp.jsonrpc = "2.0"; \
    fail_resp.id = req.id; \
    fail_resp.error.code = -32602; \
    fail_resp.error.message = "Invalid params"; \
    epee::serialization::store_t_to_json(static_cast<epee::json_rpc::error_response&>(fail_resp), response_info.m_body); \
    return true; \
  } \
  uint64_t ticks1 = epee::misc_utils::get_tick_count(); \
  boost::value_initialized<epee::json_rpc::response<command_type::response, epee::json_rpc::dummy_error> > resp_; \
  epee::json_rpc::response<command_type::response, epee::json_rpc::dummy_error>& resp =  static_cast<epee::json_rpc::response<command_type::response, epee::json_rpc::dummy_error> &>(resp_); \
  resp.jsonrpc = "2.0"; \
  resp.id = req.id;

#define FINALIZE_OBJECTS_TO_JSON(method_name) \
  uint64_t ticks2 = epee::misc_utils::get_tick_count(); \
  epee::serialization::store_t_to_json(resp, response_info.m_body); \
  uint64_t ticks3 = epee::misc_utils::get_tick_count(); \
  response_info.m_mime_tipe = "application/json"; \
  response_info.m_header_info.m_content_type = " application/json"; \
  LOG_PRINT( query_info.m_URI << "[" << method_name << "] processed with " << ticks1-ticks << "/"<< ticks2-ticks1 << "/" << ticks3-ticks2 << "ms", LOG_LEVEL_2);

#define MAP_JON_RPC_WE(method_name, callback_f, command_type) \
    else if(auto_doc<json_command_type_t<command_type>>("[" method_name "]", generate_reference) && callback_name == method_name) \
{ \
  PREPARE_OBJECTS_FROM_JSON(command_type) \
  epee::json_rpc::error_response fail_resp = AUTO_VAL_INIT(fail_resp); \
  fail_resp.jsonrpc = "2.0"; \
  fail_resp.method = req.method; \
  fail_resp.id = req.id; \
  if(!callback_f(req.params, resp.result, fail_resp.error, m_conn_context)) \
  { \
    epee::serialization::store_t_to_json(static_cast<epee::json_rpc::error_response&>(fail_resp), response_info.m_body); \
    return true; \
  } \
  FINALIZE_OBJECTS_TO_JSON(method_name) \
  return true;\
}

#define MAP_JON_RPC_WERI(method_name, callback_f, command_type) \
    else if(auto_doc<json_command_type_t<command_type>>("[" method_name "]", generate_reference) && callback_name == method_name) \
{ \
  PREPARE_OBJECTS_FROM_JSON(command_type) \
  epee::json_rpc::error_response fail_resp = AUTO_VAL_INIT(fail_resp); \
  fail_resp.jsonrpc = "2.0"; \
  fail_resp.method = req.method; \
  fail_resp.id = req.id; \
  if(!callback_f(req.params, resp.result, fail_resp.error, m_conn_context, response_info)) \
  { \
    epee::serialization::store_t_to_json(static_cast<epee::json_rpc::error_response&>(fail_resp), response_info.m_body); \
    return true; \
  } \
  FINALIZE_OBJECTS_TO_JSON(method_name) \
  return true;\
}

#define MAP_JON_RPC(method_name, callback_f, command_type) \
    else if(auto_doc<json_command_type_t<command_type>>(std::string("[") + method_name + "]", generate_reference) && callback_name == method_name) \
{ \
  PREPARE_OBJECTS_FROM_JSON(command_type) \
  if(!callback_f(req.params, resp.result, m_conn_context)) \
  { \
    epee::json_rpc::error_response fail_resp = AUTO_VAL_INIT(fail_resp); \
    fail_resp.jsonrpc = "2.0"; \
    fail_resp.method = req.method; \
    fail_resp.id = req.id; \
    fail_resp.error.code = -32603; \
    fail_resp.error.message = "Internal error"; \
    epee::serialization::store_t_to_json(static_cast<epee::json_rpc::error_response&>(fail_resp), response_info.m_body); \
    return true; \
  } \
  FINALIZE_OBJECTS_TO_JSON(method_name) \
  return true;\
}

#define MAP_JON_RPC_N(callback_f, command_type) MAP_JON_RPC(command_type::methodname(), callback_f, command_type)

#define END_JSON_RPC_MAP() \
  epee::json_rpc::error_response rsp; \
  rsp.id = id_; \
  rsp.jsonrpc = "2.0"; \
  rsp.method = callback_name; \
  rsp.error.code = -32601; \
  rsp.error.message = "Method not found"; \
  epee::serialization::store_t_to_json(static_cast<epee::json_rpc::error_response&>(rsp), response_info.m_body); \
  LOG_PRINT_L4("[JSON_RESPONSE_BODY]: " << ENDL << query_info.m_body); \
  return true; \
  }


