#include "net_skeleton/net_skeleton.h"
#include "common/command_line.h"
#include "net/http_server_impl_base.h"
#include "cryptonote_core/cryptonote_core.h"
#include "p2p/net_node.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include <string>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <cstring>
#include "cryptonote_core/cryptonote_basic.h"

#include <iostream>

#define CHECK_CORE_BUSY() if (check_core_busy()) { return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", CORE_RPC_STATUS_BUSY); }

namespace RPC
{
  cryptonote::core *core;
  nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > *p2p;
  bool testnet;

  const command_line::arg_descriptor<std::string> arg_rpc_bind_ip   = {
    "rpc-bind-ip",
    "IP for RPC server",
    "127.0.0.1"
  };

  const command_line::arg_descriptor<std::string> arg_rpc_bind_port = {
    "rpc-bind-port",
    "Port for RPC server",
    std::to_string(config::RPC_DEFAULT_PORT)
  };

  const command_line::arg_descriptor<std::string> arg_testnet_rpc_bind_port = {
    "testnet-rpc-bind-port",
    "Port for testnet RPC server",
    std::to_string(config::testnet::RPC_DEFAULT_PORT)
  };

  void init(cryptonote::core *p_core,
    nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > *p_p2p,
    bool p_testnet)
  {
    core = p_core;
    p2p = p_p2p;
    testnet = p_testnet;
  }

  void init_options(boost::program_options::options_description& desc)
  {
    command_line::add_arg(desc, arg_rpc_bind_ip);
    command_line::add_arg(desc, arg_rpc_bind_port);
    command_line::add_arg(desc, arg_testnet_rpc_bind_port);
  }

  void get_address_and_port(const boost::program_options::variables_map& vm,
    std::string &ip_address, std::string &port)
  {
    auto p2p_bind_arg = testnet ? arg_testnet_rpc_bind_port : arg_rpc_bind_port;

    ip_address = command_line::get_arg(vm, arg_rpc_bind_ip);
    port = command_line::get_arg(vm, p2p_bind_arg);
  }

  bool check_core_busy()
  {
    if (p2p->get_payload_object().get_core().get_blockchain_storage().is_storing_blockchain())
    {
      return true;
    }
    return false;
  }

  int getheight(char *buf, int len, struct ns_rpc_request *req)
  {
    CHECK_CORE_BUSY();
    uint64_t height = core->get_current_blockchain_height();
    rapidjson::Document json;
    json.SetObject();
    json.AddMember("height", height, json.GetAllocator());
    json.AddMember("status", CORE_RPC_STATUS_OK, json.GetAllocator());
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    json.Accept(writer);
    std::string response = buffer.GetString();
    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
  }

  int getblocks(char *buf, int len, struct ns_rpc_request *req)
  {
    CHECK_CORE_BUSY();
    rapidjson::Document request_json;
    char request_buf[1000];
    strncpy(request_buf, req->message[0].ptr, len);
    if (request_json.Parse(request_buf).HasParseError())
    {
      return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Invalid JSON passed");
    }

    if (!request_json.HasMember("start_height") || !request_json["start_height"].IsUint64())
    {
      return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Incorrect start_height");
    }
    if (!request_json.HasMember("block_ids") || !request_json["block_ids"].IsArray())
    {
      return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Incorrect block_ids");
    }

    uint64_t start_height = request_json["start_height"].GetUint();
    std::list<crypto::hash> block_ids;
    int ii = 0;
    for (rapidjson::Value::ConstValueIterator it = request_json["block_ids"].Begin(); it != request_json["block_ids"].End(); it++, ii++)
    {
      crypto::hash hash;
      strcpy(hash.data, it->GetString());
      block_ids.push_back(hash);
    }

    std::list<std::pair<cryptonote::block, std::list<cryptonote::transaction> > > bs;
    uint64_t result_current_height = 0;
    uint64_t result_start_height = 0;
    if (!core->find_blockchain_supplement(start_height, block_ids, bs, result_current_height,
      result_current_height, COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT))
    {
      return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Failed");
    }

    rapidjson::Document response_json;
    response_json.SetObject();
    rapidjson::Value block_json(rapidjson::kArrayType);
    rapidjson::Document::AllocatorType& allocator = response_json.GetAllocator();
    BOOST_FOREACH(auto &b, bs)
    {
      rapidjson::Value this_block(rapidjson::kObjectType);
      std::string blob = block_to_blob(b.first);
      rapidjson::Value string_value(rapidjson::kStringType);
      string_value.SetString(blob.c_str(), blob.length());
      this_block.AddMember("block", string_value, allocator);
      rapidjson::Value txs_blocks(rapidjson::kArrayType);
      BOOST_FOREACH(auto &t, b.second)
      {
        blob = cryptonote::tx_to_blob(t);
        string_value.SetString(blob.c_str(), blob.length());
        txs_blocks.PushBack(string_value.Move(), allocator);
      }
      this_block.AddMember("txs", txs_blocks, allocator);
      block_json.PushBack(this_block, allocator);
    }

    response_json.AddMember("start_height", result_start_height, allocator);
    response_json.AddMember("current_height", result_current_height, allocator);
    response_json.AddMember("status", CORE_RPC_STATUS_OK, allocator);
    response_json.AddMember("blocks", block_json, allocator);
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    response_json.Accept(writer);
    std::string response = buffer.GetString();

    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
  }

  const char *method_names[] = {
    "getheight",
    "getblocks",
    NULL
  };

  ns_rpc_handler_t handlers[] = {
    getheight,
    getblocks,
    NULL
  };

  void ev_handler(struct ns_connection *nc, int ev, void *ev_data)
  {
    struct http_message *hm = (struct http_message *) ev_data;
    char buf[1000];
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
