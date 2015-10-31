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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

/*!
 * \file daemon_deprecated_rpc.cpp
 * \brief Implementations of old JSON RPC handlers (Daemon)
 */

// NOTE: 
// While this uses net_skeleton (aka fossa) for JSON RPC handling, JSON parsing
// and string conversion are done with rapidjson because it is way easier and better
// suited.
// To add a new method, add the name and function pointer to `method_names` and `handlers`.
// The handler function should have the same signature as the rest of them here.
// It should use rapidjson to parse the request string and the internal objects kept in the
// anonymous namespace to generate the response. The response must eventually get
// stringified using rapidjson.
// Trivial and error responses may be returned with ns_create_rpc_reply and ns_create_rpc_error
// respectively.

// TODO: Add core busy checks to all methods here

#include "daemon_deprecated_rpc.h"
#include <stdexcept>

#define MAX_RESPONSE_SIZE 100000

/*!
 * \namespace
 * \brief Anonymous namespace to keep things in the scope of this file.
 */
namespace
{
  // TODO: put right error codes here
  const int daemon_connection_error = -326701;
  const int parse_error = -32700;
  const int invalid_request = -32600;
  const int invalid_params = -32602;
  const int internal_error = -32603;
  const int not_mining_error = -32604;

  RPC::Json_rpc_http_server *server = NULL;
  wap_client_t *ipc_client = NULL;

  const char* STATUS_OK = "OK";

  /*!
   * \brief Checks if daemon can be reached via IPC
   * \return true if daemon can be reached
   */
  bool check_connection_to_daemon()
  {
    return ipc_client && wap_client_connected(ipc_client);
  }

  /*!
   * \brief Checks if daemon can be reached and if not tries to connect to it.
   * \return true if daemon is reachable at the end of the function
   */
  bool connect_to_daemon() {
    if (check_connection_to_daemon()) {
      return true;
    }
    ipc_client = wap_client_new();
    wap_client_connect(ipc_client, "ipc://@/monero", 200, "wallet identity");
    if (!check_connection_to_daemon()) {
      wap_client_destroy(&ipc_client); // this sets ipc_client to NULL
      return false;
    }
    return true;
  }

  /*!
   * \brief Initializes a rapidjson response object
   * \param req           net_skeleton request object
   * \param response_json           net_skeleton request object to fill
   */
  void init_response_object(struct ns_rpc_request *req, rapidjson::Document &response_json) {
    response_json.SetObject();
    response_json.AddMember("jsonrpc", "2.0" , response_json.GetAllocator());
    rapidjson::Value string_value(rapidjson::kStringType);
    // If ID was present in request use it else use "null".
    if (req->id != NULL)
    {
      string_value.SetString(req->id[0].ptr, req->id[0].len);
    }
    else
    {
      string_value.SetString("null", 4);
    }
    response_json.AddMember("id", string_value, response_json.GetAllocator());
    string_value.SetString(req->method[0].ptr, req->method[0].len);
    response_json.AddMember("method", string_value, response_json.GetAllocator());
  }
  
  /*!
   * \brief Constructs a response string given a result JSON object.
   * 
   * It also adds boilerplate properties like id, method.
   * \param req           net_skeleton request object
   * \param result_json   rapidjson result object
   * \param response_json "Root" rapidjson document that will eventually have the whole response
   * \param response      Response as a string gets written here.
   */
  void construct_response_string(struct ns_rpc_request *req, rapidjson::Value &result_json,
    rapidjson::Document &response_json, std::string &response)
  {
    init_response_object(req, response_json);
    response_json.AddMember("result", result_json, response_json.GetAllocator());
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    response_json.Accept(writer);
    // Write string to `response`.
    response = buffer.GetString();
  }

  /*!
   * \brief Constructs a response string given a result string.
   * 
   * It also adds boilerplate properties like id, method.
   * \param req           net_skeleton request object
   * \param result   rapidjson result object
   * \param response_json "Root" rapidjson document that will eventually have the whole response
   * \param response      Response as a string gets written here.
   */
  void construct_response_string(struct ns_rpc_request *req, const std::string &result,
    rapidjson::Document &response_json, std::string &response)
  {
    init_response_object(req, response_json);
    rapidjson::Value string_value(rapidjson::kStringType);
    string_value.SetString(result.c_str(), result.length());
    response_json.AddMember("result", string_value, response_json.GetAllocator());
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    response_json.Accept(writer);
    // Write string to `response`.
    response = buffer.GetString();
  }

  /*!
   * \brief Implementation of 'getheight' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int getheight(char *buf, int len, struct ns_rpc_request *req)
  {
    if (!connect_to_daemon()) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    int rc = wap_client_get_height(ipc_client);
    if (rc < 0) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    uint64_t status = wap_client_status(ipc_client);
    if (status == IPC::STATUS_CORE_BUSY) {
      return ns_rpc_create_error(buf, len, req, internal_error,
        "Core busy.", "{}");
    }
    uint64_t height = wap_client_height(ipc_client);
    rapidjson::Document response_json;
    rapidjson::Value result_json;
    result_json.SetObject();
    result_json.AddMember("height", height, response_json.GetAllocator());
    result_json.AddMember("status", "OK", response_json.GetAllocator());
    std::string response;
    construct_response_string(req, result_json, response_json, response);
    strncpy(buf, response.c_str(), (size_t)len);
    return response.length();
  }

  /*!
   * \brief Implementation of 'startmining' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int startmining(char *buf, int len, struct ns_rpc_request *req)
  {
    if (!connect_to_daemon()) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    if (req->params == NULL)
    {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Parameters missing.", "{}");
    }
    rapidjson::Document request_json;
    char request_buf[1000];
    strncpy(request_buf, req->params[0].ptr, req->params[0].len);
    size_t zidx = sizeof(request_buf) - 1;
    if (req->params[0].len < zidx)
      zidx = req->params[0].len;
    request_buf[zidx] = '\0';
    if (request_json.Parse(request_buf).HasParseError())
    {
      return ns_rpc_create_error(buf, len, req, parse_error,
        "Invalid JSON passed", "{}");
    }

    if (!request_json.HasMember("miner_address") || !request_json["miner_address"].IsString())
    {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Incorrect miner_address", "{}");
    }
    if (!request_json.HasMember("threads_count") || !request_json["threads_count"].IsUint64())
    {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Incorrect threads_count", "{}");
    }

    std::string miner_address = request_json["miner_address"].GetString();
    uint64_t threads_count = request_json["threads_count"].GetUint();

    zchunk_t *address_chunk = zchunk_new((void*)miner_address.c_str(), miner_address.length());
    int rc = wap_client_start(ipc_client, &address_chunk, threads_count);
    zchunk_destroy(&address_chunk);
    if (rc < 0) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    uint64_t status = wap_client_status(ipc_client);
    if (status == IPC::STATUS_CORE_BUSY) {
      return ns_rpc_create_error(buf, len, req, internal_error,
        "Core busy.", "{}");
    }
    if (status == IPC::STATUS_WRONG_ADDRESS)
    {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Failed, wrong address", "{}");
    }
    if (status == IPC::STATUS_MINING_NOT_STARTED)
    {
      return ns_rpc_create_error(buf, len, req, invalid_request,
        "Failed, mining not started", "{}");
    }
    return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", STATUS_OK);
  }

  /*!
   * \brief Implementation of 'stopmining' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int stopmining(char *buf, int len, struct ns_rpc_request *req)
  {
    if (!connect_to_daemon()) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    int rc = wap_client_stop(ipc_client);
    if (rc < 0) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    uint64_t status = wap_client_status(ipc_client);
    if (status == IPC::STATUS_CORE_BUSY) {
      return ns_rpc_create_error(buf, len, req, internal_error,
        "Core busy.", "{}");
    }
    if (wap_client_status(ipc_client) != IPC::STATUS_OK)
    {
      return ns_rpc_create_error(buf, len, req, invalid_request,
        "Failed, mining not stopped", "{}");
    }
    return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", STATUS_OK);
  }

  /*!
   * \brief Implementation of 'getinfo' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int getinfo(char *buf, int len, struct ns_rpc_request *req)
  {
    if (!connect_to_daemon()) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    int rc = wap_client_get_info(ipc_client);
    if (rc < 0) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    uint64_t status = wap_client_status(ipc_client);
    if (status == IPC::STATUS_CORE_BUSY) {
      return ns_rpc_create_error(buf, len, req, internal_error,
        "Core busy.", "{}");
    }
    if (status != IPC::STATUS_OK)
    {
      return ns_rpc_create_error(buf, len, req, invalid_request,
        "Failed to get info", "{}");
    }
    rapidjson::Document response_json;
    rapidjson::Value result_json;
    result_json.SetObject();
    result_json.AddMember("height", wap_client_height(ipc_client), response_json.GetAllocator());
    result_json.AddMember("target_height", wap_client_target_height(ipc_client),
      response_json.GetAllocator());
    result_json.AddMember("difficulty", wap_client_difficulty(ipc_client),
      response_json.GetAllocator());
    result_json.AddMember("tx_count", wap_client_tx_count(ipc_client),
      response_json.GetAllocator());
    result_json.AddMember("tx_pool_size", wap_client_tx_pool_size(ipc_client),
      response_json.GetAllocator());
    result_json.AddMember("alt_blocks_count", wap_client_alt_blocks_count(ipc_client),
      response_json.GetAllocator());
    result_json.AddMember("outgoing_connections_count", wap_client_outgoing_connections_count(ipc_client),
      response_json.GetAllocator());
    result_json.AddMember("incoming_connections_count", wap_client_incoming_connections_count(ipc_client),
      response_json.GetAllocator());
    result_json.AddMember("white_peerlist_size", wap_client_white_peerlist_size(ipc_client),
      response_json.GetAllocator());
    result_json.AddMember("grey_peerlist_size", wap_client_grey_peerlist_size(ipc_client),
      response_json.GetAllocator());
    result_json.AddMember("status", "OK", response_json.GetAllocator());

    std::string response;
    construct_response_string(req, result_json, response_json, response);
    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
  }

  /*!
   * \brief Implementation of 'getpeerlist' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int getpeerlist(char *buf, int len, struct ns_rpc_request *req)
  {
    if (!connect_to_daemon()) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    int rc = wap_client_get_peer_list(ipc_client);
    if (rc < 0) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }

    rapidjson::Document response_json;
    rapidjson::Document::AllocatorType &allocator = response_json.GetAllocator();
    rapidjson::Value result_json;
    result_json.SetObject();

    zframe_t *white_list_frame = wap_client_white_list(ipc_client);
    rapidjson::Document white_list_json;
    char *data = reinterpret_cast<char*>(zframe_data(white_list_frame));
    size_t size = zframe_size(white_list_frame);

    if (white_list_json.Parse(data, size).HasParseError()) {
      return ns_rpc_create_error(buf, len, req, internal_error,
        "Couldn't parse JSON sent by daemon.", "{}");
    }

    result_json.AddMember("white_list", white_list_json["peers"], allocator);

    zframe_t *gray_list_frame = wap_client_gray_list(ipc_client);
    rapidjson::Document gray_list_json;
    data = reinterpret_cast<char*>(zframe_data(gray_list_frame));
    size = zframe_size(gray_list_frame);

    if (gray_list_json.Parse(data, size).HasParseError()) {
      return ns_rpc_create_error(buf, len, req, internal_error,
        "Couldn't parse JSON sent by daemon.", "{}");
    }
    result_json.AddMember("gray_list", gray_list_json["peers"], allocator);
    result_json.AddMember("status", "OK", allocator);

    std::string response;
    construct_response_string(req, result_json, response_json, response);
    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
  }

  /*!
   * \brief Implementation of 'getminingstatus' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int getminingstatus(char *buf, int len, struct ns_rpc_request *req)
  {
    if (!connect_to_daemon()) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    int rc = wap_client_get_mining_status(ipc_client);
    if (rc < 0) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    uint64_t status = wap_client_status(ipc_client);
    if (status == IPC::STATUS_CORE_BUSY) {
      return ns_rpc_create_error(buf, len, req, internal_error,
        "Core busy.", "{}");
    }
    rapidjson::Document response_json;
    rapidjson::Document::AllocatorType &allocator = response_json.GetAllocator();
    rapidjson::Value result_json;
    result_json.SetObject();

    result_json.AddMember("speed", wap_client_speed(ipc_client), allocator);
    result_json.AddMember("active", (wap_client_active(ipc_client) == 1), allocator);
    result_json.AddMember("threads_count", wap_client_thread_count(ipc_client), allocator);
    zchunk_t *address_chunk = wap_client_address(ipc_client);
    rapidjson::Value string_value(rapidjson::kStringType);
    string_value.SetString((char*)(zchunk_data(address_chunk)), zchunk_size(address_chunk));
    result_json.AddMember("address", string_value, allocator);
    result_json.AddMember("status", "OK", allocator);

    std::string response;
    construct_response_string(req, result_json, response_json, response);
    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
  }

  /*!
   * \brief Implementation of 'setloghashrate' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int setloghashrate(char *buf, int len, struct ns_rpc_request *req)
  {
    if (!connect_to_daemon()) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    if (req->params == NULL)
    {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Parameters missing.", "{}");
    }

    rapidjson::Document request_json;
    char request_buf[1000];
    strncpy(request_buf, req->params[0].ptr, req->params[0].len);
    request_buf[req->params[0].len] = '\0';
    if (request_json.Parse(request_buf).HasParseError())
    {
      return ns_rpc_create_error(buf, len, req, parse_error,
        "Invalid JSON passed", "{}");
    }

    if (!request_json.HasMember("visible") || !request_json["visible"].IsBool())
    {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Incorrect 'visible' field", "{}");
    }

    bool visible = request_json["visible"].GetBool();
    // 0MQ server expects an integer. 1 is true, 0 is false.
    int rc = wap_client_set_log_hash_rate(ipc_client, visible ? 1 : 0);
    if (rc < 0) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    uint64_t status = wap_client_status(ipc_client);
    if (status == IPC::STATUS_CORE_BUSY) {
      return ns_rpc_create_error(buf, len, req, internal_error,
        "Core busy.", "{}");
    }
    if (status == IPC::STATUS_NOT_MINING) {
      return ns_rpc_create_error(buf, len, req, not_mining_error,
        "Not mining", "{}");
    }

    return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", STATUS_OK);
  }

  /*!
   * \brief Implementation of 'setloglevel' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int setloglevel(char *buf, int len, struct ns_rpc_request *req)
  {
    if (!connect_to_daemon()) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    if (req->params == NULL)
    {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Parameters missing.", "{}");
    }

    rapidjson::Document request_json;
    char request_buf[1000];
    strncpy(request_buf, req->params[0].ptr, req->params[0].len);
    request_buf[req->params[0].len] = '\0';
    if (request_json.Parse(request_buf).HasParseError())
    {
      return ns_rpc_create_error(buf, len, req, parse_error,
        "Invalid JSON passed", "{}");
    }

    if (!request_json.HasMember("level") || !request_json["level"].IsNumber())
    {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Incorrect 'level' field", "{}");
    }

    int level = request_json["level"].GetInt();
    int rc = wap_client_set_log_level(ipc_client, level);
    if (rc < 0) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    uint64_t status = wap_client_status(ipc_client);
    if (status == IPC::STATUS_CORE_BUSY) {
      return ns_rpc_create_error(buf, len, req, internal_error,
        "Core busy.", "{}");
    }
    if (status == IPC::STATUS_INVALID_LOG_LEVEL) {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Invalid log level", "{}");
    }

    return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", STATUS_OK);
  }

  /*!
   * \brief Implementation of 'getblockcount' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int getblockcount(char *buf, int len, struct ns_rpc_request *req)
  {
    if (!connect_to_daemon()) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    int rc = wap_client_get_height(ipc_client);
    if (rc < 0) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    uint64_t status = wap_client_status(ipc_client);
    if (status == IPC::STATUS_CORE_BUSY) {
      return ns_rpc_create_error(buf, len, req, internal_error,
        "Core busy.", "{}");
    }
    uint64_t count = wap_client_height(ipc_client);
    rapidjson::Document response_json;
    rapidjson::Value result_json;
    result_json.SetObject();
    result_json.AddMember("count", count, response_json.GetAllocator());
    result_json.AddMember("status", "OK", response_json.GetAllocator());
    std::string response;
    construct_response_string(req, result_json, response_json, response);
    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
  }

  /*!
   * \brief Implementation of 'startsavegraph' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int startsavegraph(char *buf, int len, struct ns_rpc_request *req)
  {
    if (!connect_to_daemon()) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    int rc = wap_client_start_save_graph(ipc_client);
    if (rc < 0) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", STATUS_OK);
  }

  /*!
   * \brief Implementation of 'stopsavegraph' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int stopsavegraph(char *buf, int len, struct ns_rpc_request *req)
  {
    if (!connect_to_daemon()) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    int rc = wap_client_stop_save_graph(ipc_client);
    if (rc < 0) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", STATUS_OK);
  }

  /*!
   * \brief Implementation of 'getblockhash' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int getblockhash(char *buf, int len, struct ns_rpc_request *req)
  {
    if (!connect_to_daemon()) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    if (req->params == NULL)
    {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Parameters missing.", "{}");
    }

    rapidjson::Document request_json;
    char request_buf[1000];
    strncpy(request_buf, req->params[0].ptr, req->params[0].len);
    request_buf[req->params[0].len] = '\0';
    if (request_json.Parse(request_buf).HasParseError())
    {
      return ns_rpc_create_error(buf, len, req, parse_error,
        "Invalid JSON passed", "{}");
    }

    if (!request_json.IsArray() || request_json.Size() < 1 || !request_json[(unsigned int)0].IsNumber())
    {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Incorrect 'height' field", "{}");
    }


    int height = request_json[(unsigned int)0].GetUint();
    int rc = wap_client_get_block_hash(ipc_client, height);
    if (rc < 0) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    uint64_t status = wap_client_status(ipc_client);
    if (status == IPC::STATUS_CORE_BUSY) {
      return ns_rpc_create_error(buf, len, req, internal_error,
        "Core busy.", "{}");
    }
    if (status == IPC::STATUS_HEIGHT_TOO_BIG) {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Height too big.", "{}");
    }
    zchunk_t *hash_chunk = wap_client_hash(ipc_client);
    std::string hash((char*)zchunk_data(hash_chunk), zchunk_size(hash_chunk));
    std::string response;
    rapidjson::Document response_json;
    construct_response_string(req, hash, response_json, response);
    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
  }

  /*!
   * \brief Implementation of 'getblocktemplate' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int getblocktemplate(char *buf, int len, struct ns_rpc_request *req)
  {
    if (!connect_to_daemon()) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    if (req->params == NULL)
    {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Parameters missing.", "{}");
    }

    rapidjson::Document request_json;
    char request_buf[1000];
    strncpy(request_buf, req->params[0].ptr, req->params[0].len);
    request_buf[req->params[0].len] = '\0';
    if (request_json.Parse(request_buf).HasParseError())
    {
      return ns_rpc_create_error(buf, len, req, parse_error,
        "Invalid JSON passed", "{}");
    }

    if (!request_json.HasMember("reserve_size") || !request_json["reserve_size"].IsNumber())
    {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Incorrect 'reserve_size' field", "{}");
    }
    if (!request_json.HasMember("wallet_address") || !request_json["wallet_address"].IsString())
    {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Incorrect 'wallet_address' field", "{}");
    }

    uint64_t reserve_size = request_json["reserve_size"].GetUint();
    std::string wallet_address = request_json["wallet_address"].GetString();
    zchunk_t *address_chunk = zchunk_new((void*)wallet_address.c_str(), wallet_address.length());
    int rc = wap_client_get_block_template(ipc_client, reserve_size, &address_chunk);
    if (rc < 0) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    
    uint64_t status = wap_client_status(ipc_client);
    if (status == IPC::STATUS_CORE_BUSY) {
      return ns_rpc_create_error(buf, len, req, internal_error,
        "Core busy.", "{}");
    }
    if (status == IPC::STATUS_RESERVE_SIZE_TOO_BIG) {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Reserve size too big.", "{}");
    }
    if (status == IPC::STATUS_WRONG_ADDRESS) {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Wrong address.", "{}");
    }

    rapidjson::Document response_json;
    rapidjson::Document::AllocatorType &allocator = response_json.GetAllocator();
    rapidjson::Value result_json;
    result_json.SetObject();
    result_json.AddMember("difficulty", wap_client_difficulty(ipc_client), allocator);
    result_json.AddMember("height", wap_client_height(ipc_client), allocator);
    result_json.AddMember("reserved_offset", wap_client_reserved_offset(ipc_client), allocator);
    zchunk_t *prev_hash_chunk = wap_client_prev_hash(ipc_client);
    rapidjson::Value string_value(rapidjson::kStringType);
    string_value.SetString((char*)zchunk_data(prev_hash_chunk), zchunk_size(prev_hash_chunk));
    result_json.AddMember("prev_hash", string_value, allocator);
    zchunk_t *block_template_chunk = wap_client_prev_hash(ipc_client);
    string_value.SetString((char*)zchunk_data(block_template_chunk), zchunk_size(block_template_chunk));
    result_json.AddMember("blocktemplate_blob", string_value, allocator);
    std::string response;
    construct_response_string(req, result_json, response_json, response);
    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
  }

  /*!
   * \brief Implementation of 'getblocks' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int getblocks(char *buf, int len, struct ns_rpc_request *req)
  {
    if (!connect_to_daemon()) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    if (req->params == NULL)
    {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Parameters missing.", "{}");
    }

    rapidjson::Document request_json;
    char request_buf[1000];
    strncpy(request_buf, req->params[0].ptr, req->params[0].len);
    request_buf[req->params[0].len] = '\0';
    if (request_json.Parse(request_buf).HasParseError())
    {
      return ns_rpc_create_error(buf, len, req, parse_error,
        "Invalid JSON passed", "{}");
    }

    if (!request_json.HasMember("start_height") || !request_json["start_height"].IsNumber())
    {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Incorrect 'start_height' field", "{}");
    }
    if (!request_json.HasMember("block_ids") || !request_json["block_ids"].IsArray())
    {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Incorrect 'block_ids' field", "{}");
    }

    uint64_t start_height = request_json["start_height"].GetUint();
    uint64_t block_count = request_json["blocks_ids"].Size();
    zlist_t *list = zlist_new();
    for (uint64_t i = 0; i < block_count; i++) {
      if (!request_json["blocks_ids"][i].IsString()) {
        zlist_destroy(&list);
        return ns_rpc_create_error(buf, len, req, invalid_params,
          "Incorrect block_id", "{}");
      }
      std::string block_id = request_json["blocks_ids"][i].GetString();
      char *size_prepended_block_id = new char[block_id.length() + 1];
      size_prepended_block_id[0] = crypto::HASH_SIZE;
      memcpy(size_prepended_block_id + 1, block_id.c_str(), crypto::HASH_SIZE);
      zlist_append(list, size_prepended_block_id);
    }
    int rc = wap_client_blocks(ipc_client, &list, start_height);
    zlist_destroy(&list);

    if (rc < 0) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    
    uint64_t status = wap_client_status(ipc_client);
    if (status == IPC::STATUS_CORE_BUSY) {
      return ns_rpc_create_error(buf, len, req, internal_error,
        "Core busy.", "{}");
    }
    if (status == IPC::STATUS_INTERNAL_ERROR) {
      return ns_rpc_create_error(buf, len, req, internal_error,
        "Internal error.", "{}");
    }

    rapidjson::Document response_json;
    rapidjson::Document::AllocatorType &allocator = response_json.GetAllocator();
    rapidjson::Value result_json;
    result_json.SetObject();
    rapidjson::Value blocks(rapidjson::kArrayType);

    zframe_t *frame = zmsg_first(wap_client_block_data(ipc_client));
    if (!frame) {
      return ns_rpc_create_error(buf, len, req, internal_error,
        "Internal error.", "{}");
    }
    size_t size = zframe_size(frame);
    char *block_data = reinterpret_cast<char*>(zframe_data(frame));

    rapidjson::Document json;
    if (json.Parse(block_data, size).HasParseError()) {
      return ns_rpc_create_error(buf, len, req, internal_error,
        "Internal error.", "{}");
    }
    for (rapidjson::SizeType i = 0; i < json["blocks"].Size(); i++) {
      rapidjson::Value block_entry(rapidjson::kObjectType);
      std::string block_string(json["blocks"][i]["block"].GetString(), json["blocks"][i]["block"].GetStringLength());
      rapidjson::Value block_string_json(rapidjson::kStringType);
      block_string_json.SetString(block_string.c_str(), block_string.length());
      block_entry.AddMember("block", block_string_json, allocator);
      rapidjson::Value txs(rapidjson::kArrayType);
      for (rapidjson::SizeType j = 0; j < json["blocks"][i]["txs"].Size(); j++) {
        rapidjson::Value txs_json(rapidjson::kStringType);
        std::string txs_string(json["blocks"][i]["txs"][j].GetString(), json["blocks"][i]["txs"][j].GetStringLength());
        txs_json.SetString(txs_string.c_str(), txs_string.length());
        txs.PushBack(txs_json, allocator);
      }
      block_entry.AddMember("txs", txs, allocator);
      blocks.PushBack(block_entry, allocator);
    }

    result_json.AddMember("start_height", wap_client_start_height(ipc_client), allocator);
    result_json.AddMember("current_height", wap_client_curr_height(ipc_client), allocator);
    result_json.AddMember("blocks", blocks, allocator);
    std::string response;
    construct_response_string(req, result_json, response_json, response);
    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
  }

  // Contains a list of method names.
  const char *method_names[] = {
    "getheight",
    "startmining",
    "stopmining",
    "getinfo",
    "getpeerlist",
    "getminingstatus",
    "setloghashrate",
    "setloglevel",
    "getblockcount",
    "startsavegraph",
    "stopsavegraph",
    "getblockhash",
    "getblocktemplate",
    "getblocks",
    NULL
  };

  // Contains a list of function pointers. These must map 1-1 by index with `method_names`.
  ns_rpc_handler_t handlers[] = {
    getheight,
    startmining,
    stopmining,
    getinfo,
    getpeerlist,
    getminingstatus,
    setloghashrate,
    setloglevel,
    getblockcount,
    startsavegraph,
    stopsavegraph,
    getblockhash,
    getblocktemplate,
    getblocks,
    NULL
  };

  /*!
   * \brief Event handler that is invoked upon net_skeleton network events.
   * 
   * Any change in behavior of RPC should happen from this point.
   * \param nc      net_skeleton connection
   * \param ev      Type of event
   * \param ev_data Event data
   */
  void ev_handler(struct ns_connection *nc, int ev, void *ev_data)
  {
    struct http_message *hm = (struct http_message *) ev_data;
    char buf[MAX_RESPONSE_SIZE];
    switch (ev) {
      case NS_HTTP_REQUEST:
        ns_rpc_dispatch(hm->body.p, hm->body.len, buf, sizeof(buf),
          method_names, handlers);
        ns_printf(nc, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\n"
          "Content-Type: application/json\r\n\r\n%s",
          (int) strlen(buf), buf);
          nc->flags |= NSF_FINISHED_SENDING_DATA;
        break;
      default:
        break;
    }
  }
}

/*!
 * \namespace RPC
 * \brief RPC related utilities
 */
namespace RPC
{
  /*!
   * \namespace DaemonDeprecated
   * \brief DaemonDeprecated RPC stuff
   */
  namespace DaemonDeprecated
  {

    /*!
     * \brief Starts an HTTP server that listens to old style JSON RPC requests
     *        and creates an IPC client to be able to talk to the daemon
     * \return status code
     */
    int start() {
      server = new RPC::Json_rpc_http_server("127.0.0.1", "9997", "daemon_json_rpc", &ev_handler);
      if (!server->start()) {
        delete server;
        server = NULL;
        return FAILURE_HTTP_SERVER;
      }
      std::cout << "Started Daemon server at 127.0.0.1/daemon_json_rpc:9997\n";
      ipc_client = wap_client_new();
      wap_client_connect(ipc_client, "ipc://@/monero", 200, "wallet identity");
      if (!check_connection_to_daemon()) {
        wap_client_destroy(&ipc_client); // this sets ipc_client to NULL
        server->stop();
        delete server;
        server = NULL;
        return FAILURE_DAEMON_NOT_RUNNING;
      }
      return SUCCESS;
    }

    /*!
     * \brief Stops the HTTP server and destroys the IPC client
     */
    void stop() {
      if (server) {
        server->stop();
        delete server;
        server = NULL;
      }
      if (ipc_client) {
        wap_client_destroy(&ipc_client);
      }
    }
  }
}
