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
#include "http_base.h"
#include "jsonrpc_structs.h"
#include "storages/portable_storage_template_helper.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.http"


#define CHAIN_HTTP_TO_MAP2(context_type) bool handle_http_request(const epee::net_utils::http::http_request_info& query_info, \
              epee::net_utils::http::http_response_info& response, \
              context_type& m_conn_context) \
{\
  MINFO("HTTP [" << m_conn_context.m_remote_address.host_str() << "] " << query_info.m_http_method_str << " " << query_info.m_URI); \
  response.m_response_code = 200; \
  response.m_response_comment = "Ok"; \
  try \
  { \
    if(!handle_http_request_map(query_info, response, m_conn_context)) \
    {response.m_response_code = 404;response.m_response_comment = "Not found";} \
  } \
  catch (const std::exception &e) \
  { \
    MERROR(m_conn_context << "Exception in handle_http_request_map: " << e.what()); \
    response.m_response_code = 500; \
    response.m_response_comment = "Internal Server Error"; \
  } \
  return true; \
}


#define BEGIN_URI_MAP2()   template<class t_context> bool handle_http_request_map(const epee::net_utils::http::http_request_info& query_info, \
  epee::net_utils::http::http_response_info& response_info, \
  t_context& m_conn_context) { \
  bool handled = false; \
  if(false) return true; //just a stub to have "else if"

#define MAP_URI_AUTO_JON2_IF(s_pattern, callback_f, command_type, cond) \
    else if((query_info.m_URI == s_pattern) && (cond)) \
    { \
      handled = true; \
      uint64_t ticks = epee::misc_utils::get_tick_count(); \
      boost::value_initialized<command_type::request> req; \
      bool parse_res = epee::serialization::load_t_from_json(static_cast<command_type::request&>(req), query_info.m_body); \
      if (!parse_res) \
      { \
         MERROR("Failed to parse json: \r\n" << query_info.m_body); \
         response_info.m_response_code = 400; \
         response_info.m_response_comment = "Bad request"; \
         return true; \
      } \
      uint64_t ticks1 = epee::misc_utils::get_tick_count(); \
      boost::value_initialized<command_type::response> resp;\
      MINFO(m_conn_context << "calling " << s_pattern); \
      bool res = false; \
      try { res = callback_f(static_cast<command_type::request&>(req), static_cast<command_type::response&>(resp), &m_conn_context); } \
      catch (const std::exception &e) { MERROR(m_conn_context << "Failed to " << #callback_f << "(): " << e.what()); } \
      if (!res) \
      { \
        response_info.m_response_code = 500; \
        response_info.m_response_comment = "Internal Server Error"; \
        return true; \
      } \
      uint64_t ticks2 = epee::misc_utils::get_tick_count(); \
      epee::serialization::store_t_to_json(static_cast<command_type::response&>(resp), response_info.m_body); \
      uint64_t ticks3 = epee::misc_utils::get_tick_count(); \
      response_info.m_mime_tipe = "application/json"; \
      response_info.m_header_info.m_content_type = " application/json"; \
      MDEBUG( s_pattern << " processed with " << ticks1-ticks << "/"<< ticks2-ticks1 << "/" << ticks3-ticks2 << "ms"); \
    }

#define MAP_URI_AUTO_JON2(s_pattern, callback_f, command_type) MAP_URI_AUTO_JON2_IF(s_pattern, callback_f, command_type, true)

#define MAP_URI_AUTO_BIN2(s_pattern, callback_f, command_type) \
    else if(query_info.m_URI == s_pattern) \
    { \
      handled = true; \
      uint64_t ticks = epee::misc_utils::get_tick_count(); \
      boost::value_initialized<command_type::request> req; \
      bool parse_res = epee::serialization::load_t_from_binary(static_cast<command_type::request&>(req), epee::strspan<uint8_t>(query_info.m_body)); \
      if (!parse_res) \
      { \
         MERROR("Failed to parse bin body data, body size=" << query_info.m_body.size()); \
         response_info.m_response_code = 400; \
         response_info.m_response_comment = "Bad request"; \
         return true; \
      } \
      uint64_t ticks1 = epee::misc_utils::get_tick_count(); \
      boost::value_initialized<command_type::response> resp;\
      MINFO(m_conn_context << "calling " << s_pattern); \
      bool res = false; \
      try { res = callback_f(static_cast<command_type::request&>(req), static_cast<command_type::response&>(resp), &m_conn_context); } \
      catch (const std::exception &e) { MERROR(m_conn_context << "Failed to " << #callback_f << "()"); } \
      if (!res) \
      { \
        response_info.m_response_code = 500; \
        response_info.m_response_comment = "Internal Server Error"; \
        return true; \
      } \
      uint64_t ticks2 = epee::misc_utils::get_tick_count(); \
      epee::byte_slice buffer; \
      epee::serialization::store_t_to_binary(static_cast<command_type::response&>(resp), buffer, 64 * 1024); \
      uint64_t ticks3 = epee::misc_utils::get_tick_count(); \
      response_info.m_body.assign(reinterpret_cast<const char*>(buffer.data()), buffer.size()); \
      response_info.m_mime_tipe = " application/octet-stream"; \
      response_info.m_header_info.m_content_type = " application/octet-stream"; \
      MDEBUG( s_pattern << "() processed with " << ticks1-ticks << "/"<< ticks2-ticks1 << "/" << ticks3-ticks2 << "ms"); \
    }

#define END_URI_MAP2() return handled;}


#define BEGIN_JSON_RPC_MAP(uri)    else if(query_info.m_URI == uri) \
    { \
    uint64_t ticks = epee::misc_utils::get_tick_count(); \
    response_info.m_mime_tipe = "application/json"; \
    epee::json_rpc::request_generic req{}; \
    if(!epee::serialization::load_t_from_json(req, query_info.m_body)) \
    { \
       epee::json_rpc::response<std::string> rsp{};\
       rsp.jsonrpc = "2.0"; \
       rsp.error_.emplace(); \
       rsp.error_->code = -32700; \
       rsp.error_->message = "Parse error"; \
       epee::serialization::store_t_to_json(rsp, response_info.m_body); \
       return true; \
    } \
    const wire::basic_value id_ = req.id; \
    const std::string callback_name = req.method; \
    if(false) return true; //just a stub to have "else if"


#define PREPARE_OBJECTS_FROM_JSON(command_type) \
  handled = true; \
  response_info.m_mime_tipe = "application/json"; \
  epee::json_rpc::request_specific<command_type::request> req_{}; \
  std::error_code req_error{}; \
  epee::json_rpc::response<command_type::response> resp{}; \
  resp.jsonrpc = "2.0"; \
  resp.id = req.id; \
  if((req_error = wire::json::from_bytes(epee::to_span(query_info.m_body), req_))) \
  { \
    resp.error_.emplace(); \
    resp.error_->code = -32602; \
    resp.error_->message = "Invalid params: " + req_error.message(); \
    epee::serialization::store_t_to_json(resp, response_info.m_body); \
    return true; \
  } \
  uint64_t ticks1 = epee::misc_utils::get_tick_count(); \
  resp.result.emplace()

#define FINALIZE_OBJECTS_TO_JSON(method_name) \
  uint64_t ticks2 = epee::misc_utils::get_tick_count(); \
  epee::serialization::store_t_to_json(resp, response_info.m_body); \
  uint64_t ticks3 = epee::misc_utils::get_tick_count(); \
  response_info.m_mime_tipe = "application/json"; \
  response_info.m_header_info.m_content_type = " application/json"; \
  MDEBUG( query_info.m_URI << "[" << method_name << "] processed with " << ticks1-ticks << "/"<< ticks2-ticks1 << "/" << ticks3-ticks2 << "ms");

#define MAP_JON_RPC_WE_IF(method_name, callback_f, command_type, cond) \
    else if((callback_name == method_name) && (cond)) \
{ \
  PREPARE_OBJECTS_FROM_JSON(command_type); \
  resp.error_.emplace(); \
  MINFO(m_conn_context << "Calling RPC method " << method_name); \
  bool res = false; \
  try { res = callback_f(req_.params, *resp.result, *resp.error_, &m_conn_context); } \
  catch (const std::exception &e) { MERROR(m_conn_context << "Failed to " << #callback_f << "(): " << e.what()); } \
  if (!res) \
  { \
    resp.result.reset(); \
    epee::serialization::store_t_to_json(resp, response_info.m_body); \
    return true; \
  } \
  resp.error_.reset(); \
  FINALIZE_OBJECTS_TO_JSON(method_name) \
  return true;\
}

#define MAP_JON_RPC_WE(method_name, callback_f, command_type) MAP_JON_RPC_WE_IF(method_name, callback_f, command_type, true)

#define MAP_JON_RPC(method_name, callback_f, command_type) \
    else if(callback_name == method_name) \
{ \
  PREPARE_OBJECTS_FROM_JSON(command_type); \
  MINFO(m_conn_context << "calling RPC method " << method_name); \
  bool res = false; \
  try { res = callback_f(req_.params, *resp.result, &m_conn_context); } \
  catch (const std::exception &e) { MERROR(m_conn_context << "Failed to " << #callback_f << "(): " << e.what()); } \
  if (!res) \
  { \
    resp.result.reset(); \
    resp.error_.emplace(); \
    resp.error_->code = -32603; \
    resp.error_->message = "Internal error"; \
    epee::serialization::store_t_to_json(resp, response_info.m_body); \
    return true; \
  } \
  FINALIZE_OBJECTS_TO_JSON(method_name) \
  return true;\
}

#define END_JSON_RPC_MAP() \
  epee::json_rpc::response<std::string> rsp{}; \
  rsp.id = id_; \
  rsp.jsonrpc = "2.0"; \
  rsp.error_.emplace(); \
  rsp.error_->code = -32601; \
  rsp.error_->message = "Method not found"; \
  epee::serialization::store_t_to_json(rsp, response_info.m_body); \
  return true; \
}


