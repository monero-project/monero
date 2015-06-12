/*!
 * \file rpc_translator.cpp
 * \brief Implementations of JSON RPC handlers (Daemon)
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

#include "daemon_deprecated_rpc.h"
#include <stdexcept>

#define MAX_RESPONSE_SIZE 100000

/*!
 * \namespace
 * \brief Anonymous namespace to keep things in the scope of this file.
 */
namespace
{
  int daemon_connection_error = -326701;
  int parse_error = -32700;
  int invalid_request = -32600;
  int invalid_params = -32602;
  int internal_error = -32603;
  int not_mining_error = -32604;

  RPC::Json_rpc_http_server *server = NULL;
  wap_client_t *ipc_client = NULL;

  const char* STATUS_OK = "OK";

  bool check_connection_to_daemon()
  {
    return ipc_client && wap_client_connected(ipc_client);
  }

  void connect_to_daemon() {
    if (check_connection_to_daemon()) {
      return;
    }
    ipc_client = wap_client_new();
    wap_client_connect(ipc_client, "ipc://@/monero", 200, "wallet identity");
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
    response_json.AddMember("result", result_json, response_json.GetAllocator());
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
    connect_to_daemon();
    int rc = wap_client_get_height(ipc_client);
    if (rc < 0) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    uint64_t height = wap_client_height(ipc_client);
    rapidjson::Document response_json;
    rapidjson::Value result_json;
    result_json.SetObject();
    result_json.AddMember("height", height, response_json.GetAllocator());
    result_json.AddMember("status", "OK", response_json.GetAllocator());
    std::string response;
    construct_response_string(req, result_json, response_json, response);
    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
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
    connect_to_daemon();
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
    connect_to_daemon();
    int rc = wap_client_stop(ipc_client);
    if (rc < 0) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
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
    connect_to_daemon();
    int rc = wap_client_get_info(ipc_client);
    if (rc < 0) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
    }
    if (wap_client_status(ipc_client) != IPC::STATUS_OK)
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
    connect_to_daemon();
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
    connect_to_daemon();
    int rc = wap_client_get_mining_status(ipc_client);
    if (rc < 0) {
      return ns_rpc_create_error(buf, len, req, daemon_connection_error,
        "Couldn't connect to daemon.", "{}");
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
    connect_to_daemon();
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

    if (wap_client_status(ipc_client) == IPC::STATUS_NOT_MINING) {
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
    connect_to_daemon();
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

    if (wap_client_status(ipc_client) == IPC::STATUS_INVALID_LOG_LEVEL) {
      return ns_rpc_create_error(buf, len, req, invalid_params,
        "Invalid log level", "{}");
    }

    return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", STATUS_OK);
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
   * \namespace Daemon
   * \brief RPC relevant to daemon
   */
  namespace DaemonDeprecated
  {

    int start() {
      server = new RPC::Json_rpc_http_server("127.0.0.1", "9997", "daemon_json_rpc", &ev_handler);
      if (!server->start()) {
        return FAILURE_HTTP_SERVER;
      }
      std::cout << "Started Daemon server at 127.0.0.1/daemon_json_rpc:9997\n";
      ipc_client = wap_client_new();
      wap_client_connect(ipc_client, "ipc://@/monero", 200, "wallet identity");
      if (!check_connection_to_daemon()) {
        return FAILURE_DAEMON_NOT_RUNNING;
      }
      return SUCCESS;
    }

    void stop() {
      if (server) {
        server->stop();
        delete server;
      }
      if (ipc_client) {
        wap_client_destroy(&ipc_client);
      }
    }
  }
}
