// Copyright (c) 2014, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef JSONRPC_SERVER_HANDLERS_MAP_H
#define	JSONRPC_SERVER_HANDLERS_MAP_H

#include <string>
#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage_template_helper.h"
#include "storages/portable_storage_base.h"
#include "jsonrpc_structs.h"
#include "jsonrpc_protocol_handler.h"

#define BEGIN_JSONRPC2_MAP(t_connection_context) \
bool handle_rpc_request(const std::string& req_data, \
                        std::string& resp_data, \
                        t_connection_context& m_conn_context) \
{ \
  bool handled = false; \
  uint64_t ticks = epee::misc_utils::get_tick_count(); \
  epee::serialization::portable_storage ps; \
  if (!ps.load_from_json(req_data)) \
  { \
    epee::net_utils::jsonrpc2::make_error_resp_json(-32700, "Parse error", resp_data); \
    return true; \
  } \
  epee::serialization::storage_entry id_; \
  id_ = epee::serialization::storage_entry(std::string()); \
  if (!ps.get_value("id", id_, nullptr)) \
  { \
    epee::net_utils::jsonrpc2::make_error_resp_json(-32600, "Invalid Request", resp_data); \
    return true; \
  } \
  std::string callback_name; \
  if (!ps.get_value("method", callback_name, nullptr)) \
  { \
    epee::net_utils::jsonrpc2::make_error_resp_json(-32600, "Invalid Request", resp_data, id_); \
    return true; \
  } \
  if (false) return true; //just a stub to have "else if"



#define PREPARE_JSONRPC2_OBJECTS_FROM_JSON(command_type) \
  handled = true; \
  boost::value_initialized<epee::json_rpc::request<command_type::request> > req_; \
  epee::json_rpc::request<command_type::request>& req = static_cast<epee::json_rpc::request<command_type::request>&>(req_);\
  if(!req.load(ps)) \
  { \
    epee::net_utils::jsonrpc2::make_error_resp_json(-32602, "Invalid params", resp_data, req.id); \
    return true; \
  } \
  uint64_t ticks1 = epee::misc_utils::get_tick_count(); \
  boost::value_initialized<epee::json_rpc::response<command_type::response, epee::json_rpc::dummy_error> > resp_; \
  epee::json_rpc::response<command_type::response, epee::json_rpc::dummy_error>& resp =  static_cast<epee::json_rpc::response<command_type::response, epee::json_rpc::dummy_error> &>(resp_); \
  resp.jsonrpc = "2.0"; \
  resp.id = req.id;

#define FINALIZE_JSONRPC2_OBJECTS_TO_JSON(method_name) \
  uint64_t ticks2 = epee::misc_utils::get_tick_count(); \
  epee::serialization::store_t_to_json(resp, resp_data, 0, false); \
  resp_data += "\n"; \
  uint64_t ticks3 = epee::misc_utils::get_tick_count(); \
  LOG_PRINT("[" << method_name << "] processed with " << ticks1-ticks << "/"<< ticks2-ticks1 << "/" << ticks3-ticks2 << "ms", LOG_LEVEL_2);


#define MAP_JSONRPC2_WE(method_name, callback_f, command_type) \
  else if (callback_name == method_name) \
  { \
    PREPARE_JSONRPC2_OBJECTS_FROM_JSON(command_type) \
    epee::json_rpc::error_response fail_resp = AUTO_VAL_INIT(fail_resp); \
    fail_resp.jsonrpc = "2.0"; \
    fail_resp.id = req.id; \
    if(!callback_f(req.params, resp.result, fail_resp.error, m_conn_context)) \
    { \
      epee::serialization::store_t_to_json(static_cast<epee::json_rpc::error_response&>(fail_resp), resp_data, 0, false); \
      resp_data += "\n"; \
      return true; \
    } \
    FINALIZE_JSONRPC2_OBJECTS_TO_JSON(method_name) \
    return true; \
  }

#define END_JSONRPC2_MAP() \
  epee::net_utils::jsonrpc2::make_error_resp_json(-32601, "Method not found", resp_data, id_); \
  return true; \
}

#endif	/* JSONRPC_SERVER_HANDLERS_MAP_H */
