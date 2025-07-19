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
#include "net/net_utils_base.h"
#include "storages/portable_storage_extended_for_doc.h"



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
        KV_SERIALIZE(jsonrpc) DOC_DSCR("") DOC_EXMP("2.0") DOC_END
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
        KV_SERIALIZE(jsonrpc) DOC_DSCR("") DOC_EXMP("2.0") DOC_END
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


template<typename typename_t>
typename_t get_documentation_json_struct()
{
  return AUTO_VAL_INIT_T(typename_t);
}

struct documentation_entry
{
  std::string uri;
  bool is_binary = false; //if not - then it's JSON
  std::string json_method_name;
  std::string request_json_example;
  std::string request_json_descriptions;

  std::string response_json_example;
  std::string response_json_descriptions;

  std::string method_general_decription;
};

struct documentation
{
  bool do_generate_documentation = false;
  std::list<documentation_entry> entries;
};

// Primary template
template<typename T>
struct has_static_member_description {
private:
  // SFINAE test function
  template<typename U>
  static auto test(int) -> decltype(U::description, std::true_type{});

  // Fallback function
  template<typename>
  static auto test(...) -> std::false_type;

public:
  // Member constant indicating whether T has a static member
  static constexpr bool value = decltype(test<T>(0))::value;
};




template <typename T>
const char* get_command_description()
{
  if constexpr (has_static_member_description<T>::value)
  {
    return T::description;
  }
  else
  {
    return "NO DESCRIPTION";
  }
} 

template <typename T>
void f(T) {}  // Definition #2


// Base template
template<typename T>
struct is_std_simple_container : std::false_type {};

// Specializations for each container
template<typename T, typename Alloc>
struct is_std_simple_container<std::vector<T, Alloc>> : std::true_type {};

template<typename T, typename Alloc>
struct is_std_simple_container<std::deque<T, Alloc>> : std::true_type {};

template<typename T, typename Alloc>
struct is_std_simple_container<std::list<T, Alloc>> : std::true_type {};

template<typename T, std::size_t N>
struct is_std_simple_container<std::array<T, N>> : std::true_type {};


template<typename command_type_t, bool is_json_rpc_method>
bool auto_doc(const std::string& uri, const std::string& method, bool is_json, documentation& docs)
{
  if (!docs.do_generate_documentation) return true;

  docs.entries.resize(docs.entries.size()+1);
  docs.entries.back().is_binary = !is_json;
  docs.entries.back().uri = uri;
  docs.entries.back().json_method_name = method;
  docs.entries.back().method_general_decription = get_command_description<command_type_t>();
  
  if constexpr (is_json_rpc_method)
  {
    //json rpc-like call
    typedef typename epee::json_rpc::request<typename command_type_t::request> request_t;
    typedef typename epee::json_rpc::response<typename command_type_t::response, typename epee::json_rpc::dummy_error> response_t;
    request_t req = AUTO_VAL_INIT(req); //get_documentation_json_struct<request_t>();
    if constexpr (is_std_simple_container<typename command_type_t::request>::value)
    {
      req.params.resize(1);
    }
    
    response_t res = AUTO_VAL_INIT(res); 
    if constexpr (is_std_simple_container<typename command_type_t::response>::value)
    {
      req.result.resize(1);
    }

    req.method = method;
    epee::serialization::portable_storage_extended_doc ps;
    req.store(ps, nullptr);
    ps.dump_as_json(docs.entries.back().request_json_example);
    ps.dump_as_decriptions(docs.entries.back().request_json_descriptions);

    epee::serialization::portable_storage_extended_doc ps_res;
    res.store(ps_res, nullptr);
    ps_res.dump_as_json(docs.entries.back().response_json_example);
    ps_res.dump_as_decriptions(docs.entries.back().response_json_descriptions);

  }
  else
  {
    //json/bin uri/based
    typedef typename command_type_t::request request_t;
    typedef typename command_type_t::response response_t;

    request_t req = AUTO_VAL_INIT(req); //get_documentation_json_struct<request_t>();
    response_t res = AUTO_VAL_INIT(res); //get_documentation_json_struct<response_t>();


    epee::serialization::portable_storage_extended_doc ps;
    req.store(ps, nullptr);
    ps.dump_as_json(docs.entries.back().request_json_example);
    ps.dump_as_decriptions(docs.entries.back().request_json_descriptions);

    epee::serialization::portable_storage_extended_doc ps_res;
    res.store(ps_res, nullptr);
    ps_res.dump_as_json(docs.entries.back().response_json_example);
    ps_res.dump_as_decriptions(docs.entries.back().response_json_descriptions);
  }


//  std::stringstream ss;
//  ss << prefix_name << ENDL
//    << "REQUEST: " << ENDL << req_str << ENDL << req_str_descr << "--------------------------------" << ENDL
//    << "RESPONSE: " << ENDL << res_str << ENDL << res_str_descr << "################################" << ENDL;
//  generate_reference += ss.str();
  return true;

  //return auto_doc_t<typename command_type_t::request, typename command_type_t::response>(prefix_name, generate_reference);
}

namespace epee {
  namespace net_utils {
    namespace http {
      struct i_chain_handler
      {
        virtual bool handle_http_request(const epee::net_utils::http::http_request_info& query_info, epee::net_utils::http::http_response_info& response_info,
          epee::net_utils::connection_context_base& conn_context, bool& call_found, documentation& docs = epee::net_utils::http::i_chain_handler::m_empty_documentation)
        {
          return this->handle_http_request_map(query_info, response_info, conn_context, call_found, docs);
        }

        virtual bool handle_http_request_map(const epee::net_utils::http::http_request_info& query_info, epee::net_utils::http::http_response_info& response_info,
          epee::net_utils::connection_context_base& m_conn_context, bool& call_found, documentation& docs = epee::net_utils::http::i_chain_handler::m_empty_documentation) = 0;
        

        static inline documentation m_empty_documentation;
      };
    }
  }
} 




#define CHAIN_HTTP_TO_MAP2(context_type) bool handle_http_request(const epee::net_utils::http::http_request_info& query_info, \
              epee::net_utils::http::http_response_info& response, \
              context_type& m_conn_context) \
{\
  response.m_response_code = 200; \
  response.m_response_comment = "Ok"; \
  bool call_found = false; \
  if(!handle_http_request_map(query_info, response, m_conn_context, call_found) && response.m_response_code == 200) \
  { response.m_response_code = 500; response.m_response_comment = "Internal Server Error"; return true; } \
  if (!call_found) \
  { response.m_response_code = 404; response.m_response_comment = "Not Found"; return true; } \
  return true; \
}

#define BEGIN_URI_MAP2()   template<class t_context> bool handle_http_request_map(const epee::net_utils::http::http_request_info& query_info, \
  epee::net_utils::http::http_response_info& response_info, \
  t_context& m_conn_context, bool& call_found, documentation& docs = epee::net_utils::http::i_chain_handler::m_empty_documentation) { \
  call_found = false; \
  if(false) return true; //just a stub to have "else if"

#define BEGIN_URI_MAP2_VIRTUAL()   virtual bool handle_http_request_map(const epee::net_utils::http::http_request_info& query_info, \
  epee::net_utils::http::http_response_info& response_info, \
  epee::net_utils::connection_context_base& m_conn_context, bool& call_found, documentation& docs = epee::net_utils::http::i_chain_handler::m_empty_documentation) { \
  call_found = false; \
  if(false) return true; //just a stub to have "else if"


#define MAP_URI2(pattern, callback)  else if(std::string::npos != query_info.m_URI.find(pattern)) return callback(query_info, response_info, m_conn_context);

#define MAP_URI_AUTO_XML2(s_pattern, callback_f, command_type) //TODO: don't think i ever again will use xml - ambiguous and "overtagged" format

#define MAP_URI_AUTO_JON2(s_pattern, callback_f, command_type) \
    else if(auto_doc<command_type, false>(s_pattern, "", true, docs) && query_info.m_URI == s_pattern) \
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
    else if(auto_doc<command_type, false>(s_pattern, "", false, docs) && query_info.m_URI == s_pattern) \
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

#define CHAIN_TO_PHANDLER(pi_chain_handler) else if (pi_chain_handler && pi_chain_handler->handle_http_request(query_info, response_info, m_conn_context, call_found, docs) && call_found) { return true;}

#define CHAIN_URI_MAP2(callback) else {callback(query_info, response_info, m_conn_context);call_found = true;}

#define END_URI_MAP2() return true;}



//template<typename command_type_t>
//struct json_command_type_t
//{
//  typedef typename epee::json_rpc::request<typename command_type_t::request> request;
//  typedef typename epee::json_rpc::request<typename command_type_t::response> response;
//};

//#define JSON_RPC_REFERENCE_MARKER          "JSON_RPC"

//    if(query_info.m_URI == JSON_RPC_REFERENCE_MARKER) {generate_reference = "JSON RPC URL: " uri "\n";} \

#define BEGIN_JSON_RPC_MAP(uri)    else if(docs.do_generate_documentation || query_info.m_URI == uri) \
    { \
    const char* current_zone_json_uri = uri;\
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
    else if(auto_doc<command_type, true>(current_zone_json_uri, method_name, true, docs) && callback_name == method_name) \
{ \
  call_found = true; \
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
    else if(auto_doc<command_type, true>(current_zone_json_uri, method_name, true, docs) && callback_name == method_name) \
{ \
  call_found = true; \
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
    else if(auto_doc<command_type, true>(current_zone_json_uri, method_name, true, docs) && callback_name == method_name) \
{ \
  call_found = true; \
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

#define MAP_JON_RPC_CONDITIONAL(method_name, callback_f, command_type, predicate) \
    else if(predicate && auto_doc<command_type, true>(current_zone_json_uri, method_name, true, docs) && callback_name == method_name) \
{ \
  call_found = true; \
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

namespace epee
{
   template<typename t_rpc_server>
   bool generate_doc_as_md_files(const std::string& folder, t_rpc_server& server, const std::string& sufix = std::string())
   {
     LOG_PRINT_L0("Dumping RPC auto-generated documents!");
     epee::net_utils::http::http_request_info query_info;
     epee::net_utils::http::http_response_info response_info;
     epee::net_utils::connection_context_base conn_context;
     //std::string generate_reference = std::string("WALLET_RPC_COMMANDS_LIST:\n");
     bool call_found = false;

     documentation docs;
     docs.do_generate_documentation = true;
     //    query_info.m_URI = JSON_RPC_REFERENCE_MARKER;
     query_info.m_body = "{\"jsonrpc\": \"2.0\", \"method\": \"nonexisting_method\", \"params\": {}},";
     server.handle_http_request_map(query_info, response_info, conn_context, call_found, docs);

     for (const auto& de : docs.entries)
     {
       std::stringstream ss;
       ss << de.method_general_decription << ENDL << ENDL;;

       ss << "URL: ```http:://127.0.0.1:11211" << de.uri << "```" << ENDL;

       ss << "### Request: " << ENDL << "```json" << ENDL << de.request_json_example << ENDL << "```" << ENDL;
       ss << "### Request description: " << ENDL << "```" << ENDL << de.request_json_descriptions << ENDL << "```" << ENDL;
       ss << "### Response: " << ENDL << "```json" << ENDL << de.response_json_example << ENDL << "```" << ENDL;
       ss << "### Response description: " << ENDL << "```" << ENDL << de.response_json_descriptions << ENDL << "```" << ENDL;

       ss << sufix;

       std::string filename = de.json_method_name;
       if (!filename.size())
       {
         filename = de.uri;
         if (filename.front() == '/')
           filename.erase(filename.begin());
       }
       filename += ".md";
       bool r = epee::file_io_utils::save_string_to_file(folder + "/" + filename, ss.str());
       if (!r)
       {
         LOG_ERROR("Failed to save file " << filename);
         return false;
       }
     }
     return true;
   }
}