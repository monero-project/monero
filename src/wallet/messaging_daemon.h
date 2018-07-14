
#pragma once

#include "messaging_daemon_commands_defs.h"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include "net/http_server_impl_base.h"
#include "net/http_client.h"
#include "common/util.h"
#include "serialization/keyvalue_serialization.h"

#define MESSAGING_DAEMON_ERROR_CODE_UNKNOWN_ERROR -1

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.mms"

/*
{"inboxMessages":[{
    "encodingType": 2,
    "toAddress": "BM-2cUVEbbb3H6ojddYQziK3RafJ5GPcFQv7e",
    "read": 1,
    "msgid": "1494252ecf5531d0abac6fd66c5d2d6ddab1ad1a3a92954d215250a95e599217",
    "message": "TU1TIG1ha2VzIHByb2dyZXNz",
    "fromAddress": "BM-2cXwZ8TEHK3fGqm3DKAUAk3nfDjmTawZr6",
    "receivedTime": "1529832036",
    "subject": "SGVsbG8gS2FsaSBMaW51eA=="
},{
    "encodingType": 2,
    "toAddress": "BM-2cUVEbbb3H6ojddYQziK3RafJ5GPcFQv7e",
    "read": 1,
    "msgid": "b8a98f43730246f7bf08a0cf0be7e6e9cbada1c9d0bf0797de26330afb0317ed",
    "message": "SXMgdGhpcyBmYXN0ZXIgbm93Pw==",
    "fromAddress": "BM-2cXwZ8TEHK3fGqm3DKAUAk3nfDjmTawZr6",
    "receivedTime": "1529832273",
    "subject": "U2Vjb25kIG1lc3NhZ2U="
}]}
*/

namespace tools
{
  namespace bitmessage_rpc
  {
    
    struct message_info
    {
      uint32_t encodingType;
      std::string toAddress;
      uint32_t read;
      std::string msgid;
      std::string message;
      std::string fromAddress;
      std::string receivedTime;
      std::string subject;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(encodingType)
        KV_SERIALIZE(toAddress)
        KV_SERIALIZE(read)
        KV_SERIALIZE(msgid)
        KV_SERIALIZE(message);
        KV_SERIALIZE(fromAddress)
        KV_SERIALIZE(receivedTime)
        KV_SERIALIZE(subject)
      END_KV_SERIALIZE_MAP()
    };
    
    struct inbox_messages_response
    {
      std::vector<message_info> inboxMessages;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(inboxMessages)
      END_KV_SERIALIZE_MAP()
    };
    
  }
}

namespace tools
{
  class messaging_daemon: public epee::http_server_impl_base<messaging_daemon>
  {
  public:
    typedef epee::net_utils::connection_context_base connection_context;

    static const char* tr(const char* str);

    messaging_daemon();
    ~messaging_daemon();

    bool init(const boost::program_options::variables_map *vm);
    bool run();
    void stop();

  private:
    CHAIN_HTTP_TO_MAP2(connection_context);

    BEGIN_URI_MAP2()
      BEGIN_JSON_RPC_MAP("/json_rpc")
        MAP_JON_RPC_WE("add", on_add, messaging_daemon_rpc::COMMAND_RPC_ADD)
        MAP_JON_RPC_WE("send_message", on_send_message, messaging_daemon_rpc::COMMAND_RPC_SEND_MESSAGE)
        MAP_JON_RPC_WE("receive_messages", on_receive_messages, messaging_daemon_rpc::COMMAND_RPC_RECEIVE_MESSAGES)
        MAP_JON_RPC_WE("delete_message", on_delete_message, messaging_daemon_rpc::COMMAND_RPC_DELETE_MESSAGE)
      END_JSON_RPC_MAP()
    END_URI_MAP2()

    bool on_add(const messaging_daemon_rpc::COMMAND_RPC_ADD::request& req, messaging_daemon_rpc::COMMAND_RPC_ADD::response& res, epee::json_rpc::error& er);
    bool on_send_message(const messaging_daemon_rpc::COMMAND_RPC_SEND_MESSAGE::request& req, messaging_daemon_rpc::COMMAND_RPC_SEND_MESSAGE::response& res, epee::json_rpc::error& er);
    bool on_receive_messages(const messaging_daemon_rpc::COMMAND_RPC_RECEIVE_MESSAGES::request& req, messaging_daemon_rpc::COMMAND_RPC_RECEIVE_MESSAGES::response& res, epee::json_rpc::error& er);
    bool on_delete_message(const messaging_daemon_rpc::COMMAND_RPC_DELETE_MESSAGE::request& req, messaging_daemon_rpc::COMMAND_RPC_DELETE_MESSAGE::response& res, epee::json_rpc::error& er);

    void handle_rpc_exception(const std::exception_ptr& e, epee::json_rpc::error& er, int default_error_code);

    const boost::program_options::variables_map *m_vm;
    std::atomic<bool> m_stop;
    epee::net_utils::http::http_simple_client m_http_client;
    std::string m_bitmessage_url;
    std::string m_bitmessage_user;
    std::string m_bitmessage_password;
    
    bool post_request(const std::string &request, std::string &answer);
    std::string get_str_between_tags(const std::string &s, const std::string &start_delim, const std::string &stop_delim);
    
    void start_xml_rpc_cmd(std::string &xml, const std::string &method_name);
    void add_xml_rpc_string_param(std::string &xml, const std::string &param);
    void add_xml_rpc_base64_param(std::string &xml, const std::string &param);
    void add_xml_rpc_integer_param(std::string &xml, const int32_t &param);
    void end_xml_rpc_cmd(std::string &xml);
  };
  
}
