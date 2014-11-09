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
#include "crypto/hash-ops.h"

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

  void construct_response_string(struct ns_rpc_request *req, rapidjson::Document &response_json,
    std::string &response)
  {
    rapidjson::Value string_value(rapidjson::kStringType);
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
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    response_json.Accept(writer);
    response = buffer.GetString();
  }

  int getheight(char *buf, int len, struct ns_rpc_request *req)
  {
    CHECK_CORE_BUSY();
    uint64_t height = core->get_current_blockchain_height();
    rapidjson::Document response_json;
    response_json.SetObject();
    response_json.AddMember("height", height, response_json.GetAllocator());
    response_json.AddMember("status", CORE_RPC_STATUS_OK, response_json.GetAllocator());
    std::string response;
    construct_response_string(req, response_json, response);
    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
  }

  int getblocks(char *buf, int len, struct ns_rpc_request *req)
  {
    CHECK_CORE_BUSY();
    rapidjson::Document request_json;
    char request_buf[1000];
    strncpy(request_buf, req->message[0].ptr, req->message[0].len);
    request_buf[req->message[0].len] = '\0';
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
    for (rapidjson::Value::ConstValueIterator it = request_json["block_ids"].Begin();
      it != request_json["block_ids"].End(); it++, ii++)
    {
      crypto::hash hash;
      if (!it->IsString())
      {
        return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Wrong type in block_ids");
      }
      if (strlen(it->GetString()) > crypto::HASH_SIZE)
      {
        return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Block ID exceeds length.");
      }
      strcpy(hash.data, it->GetString());
      block_ids.push_back(hash);
    }

    std::list<std::pair<cryptonote::block, std::list<cryptonote::transaction> > > bs;
    uint64_t result_current_height = 0;
    uint64_t result_start_height = 0;
    if (!core->find_blockchain_supplement(start_height, block_ids, bs, result_current_height,
      result_start_height, COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT))
    {
      return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Failed");
    }

    rapidjson::Document response_json;
    response_json.SetObject();
    rapidjson::Value block_json(rapidjson::kArrayType);
    rapidjson::Document::AllocatorType &allocator = response_json.GetAllocator();
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

    std::string response;
    construct_response_string(req, response_json, response);

    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
  }

  int gettransactions(char *buf, int len, struct ns_rpc_request *req)
  {
    CHECK_CORE_BUSY();
    rapidjson::Document request_json;
    char request_buf[1000];
    strncpy(request_buf, req->message[0].ptr, req->message[0].len);
    request_buf[req->message[0].len] = '\0';
    if (request_json.Parse(request_buf).HasParseError())
    {
      return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Invalid JSON passed");
    }

    if (!request_json.HasMember("txs_hashes") || !request_json["txs_hashes"].IsArray())
    {
      return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Incorrect txs_hashes");
    }

    std::list<std::string> txs_hashes;
    for (rapidjson::Value::ConstValueIterator it = request_json["txs_hashes"].Begin();
      it != request_json["txs_hashes"].End(); it++)
    {
      if (!it->IsString())
      {
        return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Wrong type in txs_hashes");
      }
      txs_hashes.push_back(std::string(it->GetString()));
    }
    std::vector<crypto::hash> vh;
    BOOST_FOREACH(const auto& tx_hex_str, txs_hashes)
    {
      cryptonote::blobdata b;
      if (!string_tools::parse_hexstr_to_binbuff(tx_hex_str, b))
      {
        return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Failed to parse hex representation of transaction hash");
      }
      if (b.size() != sizeof(crypto::hash))
      {
        return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Failed, size of data mismatch");
      }
      vh.push_back(*reinterpret_cast<const crypto::hash*>(b.data()));
    }
    std::list<crypto::hash> missed_txs;
    std::list<cryptonote::transaction> txs;
    bool r = core->get_transactions(vh, txs, missed_txs);
    if (!r)
    {
      return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Failed");
    }

    rapidjson::Document response_json;
    response_json.SetObject();
    rapidjson::Document::AllocatorType &allocator = response_json.GetAllocator();

    rapidjson::Value txs_as_hex(rapidjson::kArrayType);
    BOOST_FOREACH(auto &tx, txs)
    {
      cryptonote::blobdata blob = t_serializable_object_to_blob(tx);
      std::string string_blob = string_tools::buff_to_hex_nodelimer(blob);
      rapidjson::Value string_value(rapidjson::kStringType);
      string_value.SetString(string_blob.c_str(), string_blob.length());
      txs_as_hex.PushBack(string_value, allocator);
    }
    response_json.AddMember("txs_as_hex", txs_as_hex, response_json.GetAllocator());

    rapidjson::Value missed_tx(rapidjson::kArrayType);
    BOOST_FOREACH(const auto &miss_tx, missed_txs)
    {
      std::string string_blob = string_tools::pod_to_hex(miss_tx);
      rapidjson::Value string_value(rapidjson::kStringType);
      string_value.SetString(string_blob.c_str(), string_blob.length());
      missed_tx.PushBack(string_value, allocator);
    }
    response_json.AddMember("missed_tx", missed_tx, response_json.GetAllocator());

    std::string response;
    construct_response_string(req, response_json, response);

    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
  }

  int startmining(char *buf, int len, struct ns_rpc_request *req)
  {
    CHECK_CORE_BUSY();
    rapidjson::Document request_json;
    char request_buf[1000];
    strncpy(request_buf, req->message[0].ptr, req->message[0].len);
    request_buf[req->message[0].len] = '\0';
    if (request_json.Parse(request_buf).HasParseError())
    {
      return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Invalid JSON passed");
    }

    if (!request_json.HasMember("miner_address") || !request_json["miner_address"].IsString())
    {
      return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Incorrect miner_address");
    }
    if (!request_json.HasMember("threads_count") || !request_json["threads_count"].IsUint64())
    {
      return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Incorrect threads_count");
    }

    std::string miner_address = request_json["miner_address"].GetString();
    uint64_t threads_count = request_json["threads_count"].GetUint();

    cryptonote::account_public_address adr;
    if (!cryptonote::get_account_address_from_str(adr, testnet, miner_address))
    {
      return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Failed, wrong address");
    }

    boost::thread::attributes attrs;
    attrs.set_stack_size(THREAD_STACK_SIZE);
    if (!core->get_miner().start(adr, static_cast<size_t>(threads_count), attrs))
    {
      return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Failed, mining not started");
    }
    return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", CORE_RPC_STATUS_OK);
  }

  int stopmining(char *buf, int len, struct ns_rpc_request *req)
  {
    CHECK_CORE_BUSY();
    if (!core->get_miner().stop())
    {
      return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Failed, mining not stopped");
    }
    return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", CORE_RPC_STATUS_OK);
  }

  int miningstatus(char *buf, int len, struct ns_rpc_request *req)
  {
    CHECK_CORE_BUSY();
    rapidjson::Document response_json;
    response_json.SetObject();
    bool active = core->get_miner().is_mining();
    response_json.AddMember("active", active, response_json.GetAllocator());

    if (active)
    {
      uint64_t speed = core->get_miner().get_speed();
      uint64_t threads_count = core->get_miner().get_threads_count();
      const cryptonote::account_public_address &mining_address = core->get_miner().get_mining_address();
      std::string address = cryptonote::get_account_address_as_str(testnet, mining_address);
      response_json.AddMember("speed", speed, response_json.GetAllocator());
      response_json.AddMember("threads_count", threads_count, response_json.GetAllocator());
      rapidjson::Value string_address(rapidjson::kStringType);
      string_address.SetString(address.c_str(), address.length());
      response_json.AddMember("address", string_address, response_json.GetAllocator());
    }
    response_json.AddMember("status", CORE_RPC_STATUS_OK, response_json.GetAllocator());

    std::string response;
    construct_response_string(req, response_json, response);

    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
  }

  int getblockcount(char *buf, int len, struct ns_rpc_request *req)
  {
    CHECK_CORE_BUSY();
    rapidjson::Document response_json;
    response_json.SetObject();
    response_json.AddMember("count", core->get_current_blockchain_height(), response_json.GetAllocator());
    response_json.AddMember("status", CORE_RPC_STATUS_OK, response_json.GetAllocator());

    std::string response;
    construct_response_string(req, response_json, response);

    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
  }

  int getblockhash(char *buf, int len, struct ns_rpc_request *req)
  {
    CHECK_CORE_BUSY();
    rapidjson::Document request_json;
    char request_buf[1000];
    strncpy(request_buf, req->message[0].ptr, req->message[0].len);
    request_buf[req->message[0].len] = '\0';
    if (request_json.Parse(request_buf).HasParseError())
    {
      return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Invalid JSON passed");
    }

    if (!request_json.HasMember("height") || !request_json["height"].IsUint64())
    {
      return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Incorrect height");
    }
    uint64_t height = request_json["height"].GetUint();
    if (core->get_current_blockchain_height() <= height)
    {
      return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Height specified is too big.");
    }
    std::string hash = string_tools::pod_to_hex(core->get_block_id_by_height(height));
    rapidjson::Document response_json;
    response_json.SetObject();
    rapidjson::Value string_value(rapidjson::kStringType);
    string_value.SetString(hash.c_str(), hash.length());
    response_json.AddMember("hash", string_value, response_json.GetAllocator());
    response_json.AddMember("status", CORE_RPC_STATUS_OK, response_json.GetAllocator());

    std::string response;
    construct_response_string(req, response_json, response);
    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
  }

  const char *method_names[] = {
    "getheight",
    "getblocks",
    "gettransactions",
    "startmining",
    "stopmining",
    "miningstatus",
    "getblockcount",
    "getblockhash",
    NULL
  };

  ns_rpc_handler_t handlers[] = {
    getheight,
    getblocks,
    gettransactions,
    startmining,
    stopmining,
    miningstatus,
    getblockcount,
    getblockhash,
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
