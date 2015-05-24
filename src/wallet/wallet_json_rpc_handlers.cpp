/*!
 * \file wallet_json_rpc_handlers.cpp
 * \brief Implementations of JSON RPC handlers (Wallet)
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

#include "wallet_json_rpc_handlers.h"

#define MAX_RESPONSE_SIZE 2000

/*!
 * \namespace
 * \brief Anonymous namespace to keep things in the scope of this file.
 */
namespace
{
  tools::wallet2 *wallet;

  /*!
   * \brief Constructs a response string given a result JSON object.
   * 
   * It also adds boilerplate properties like id, method.
   * \param req           net_skeleton request object
   * \param result_json   rapidjson result object
   * \param response_json Root rapidjson document that will eventually have the whole response
   * \param response      Response as a string gets written here.
   */
  void construct_response_string(struct ns_rpc_request *req, rapidjson::Value &result_json,
    rapidjson::Document &response_json, std::string &response)
  {
    /*response_json.SetObject();
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
    response = buffer.GetString();*/
  }

  /*!
   * \brief Implementation of 'getbalance' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int getbalance(char *buf, int len, struct ns_rpc_request *req)
  {
    /*uint64_t balance, unlocked_balance;
    try
    {
      balance = wallet->balance();
      unlocked_balance = wallet->unlocked_balance();
    }
    catch (std::exception& e)
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::internal_error,
        "Internal error", "{}");
    }
    rapidjson::Document response_json;
    rapidjson::Value result_json;
    result_json.SetObject();
    result_json.AddMember("balance", balance, response_json.GetAllocator());
    result_json.AddMember("unlocked_balance", unlocked_balance, response_json.GetAllocator());
    result_json.AddMember("status", CORE_RPC_STATUS_OK, response_json.GetAllocator());
    std::string response;
    construct_response_string(req, result_json, response_json, response);
    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();*/
  }

  /*!
   * \brief Implementation of 'getaddress' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int getaddress(char *buf, int len, struct ns_rpc_request *req)
  {
    /*std::string address;
    try
    {
      address = wallet->get_account().get_public_address_str(wallet->testnet());
    }
    catch (std::exception& e)
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::internal_error,
        "Internal error", "{}");
    }
    rapidjson::Document response_json;
    rapidjson::Value result_json;
    result_json.SetObject();
    rapidjson::Value string_value(rapidjson::kStringType);
    string_value.SetString(address.c_str(), address.length());
    result_json.AddMember("address", string_value, response_json.GetAllocator());
    result_json.AddMember("status", CORE_RPC_STATUS_OK, response_json.GetAllocator());
    std::string response;
    construct_response_string(req, result_json, response_json, response);
    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();*/
  }

  // Contains a list of method names.
  const char *method_names[] = {
    "getbalance",
    "getaddress",
    NULL
  };

  // Contains a list of function pointers. These must map 1-1 by index with `method_names`.
  ns_rpc_handler_t handlers[] = {
    getbalance,
    getaddress,
    NULL
  };
}

/*!
 * \namespace RPC
 * \brief RPC related utilities
 */
namespace RPC
{
  /*!
   * \namespace Wallet
   * \brief RPC relevant to wallet
   */
  namespace Wallet
  {
    /*!
     * \brief initializes module (must call this before handling requests)
     * \param p_wallet    Pointer to wallet2 object
     */
    void init(tools::wallet2 *p_wallet)
    {
      wallet = p_wallet;
    }

    /*!
     * \Inits certain options used in Wallet CLI.
     * \param desc Instance of options description object
     */
    void init_options(boost::program_options::options_description& desc)
    {
      command_line::add_arg(desc, arg_rpc_bind_ip);
      command_line::add_arg(desc, arg_rpc_bind_port);
    }

    /*!
     * \brief Gets IP address and port number from variable map
     * \param vm         Variable map
     * \param ip_address IP address
     * \param port       Port number
     */
    void get_address_and_port(const boost::program_options::variables_map& vm,
      std::string &ip_address, std::string &port)
    {
      ip_address = command_line::get_arg(vm, arg_rpc_bind_ip);
      port = command_line::get_arg(vm, arg_rpc_bind_port);
    }

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
}
