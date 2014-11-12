/*!
 * \file json_rpc_handlers.cpp
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

#include "json_rpc_handlers.h"

#define CHECK_CORE_BUSY() if (check_core_busy()) { \
 return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::internal_error, \
  CORE_RPC_STATUS_BUSY, "{}"); }

/*!
 * \namespace
 * \brief Anonymous namespace to keep things in the scope of this file.
 */
namespace
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

  /*!
   * \brief Tells if core is busy
   * \return True if core is busy, false otherwise.
   */
  bool check_core_busy()
  {
    if (p2p->get_payload_object().get_core().get_blockchain_storage().is_storing_blockchain())
    {
      return true;
    }
    return false;
  }

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
    response_json.SetObject();
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
   * \brief equivalent of strstr, but with arbitrary bytes (ie, NULs)
   * 
   * This does not differentiate between "not found" and "found at offset 0"
   * \param  start_buff String to search
   * \param  buflen     length of string
   * \param  pat        Pattern to search
   * \param  patlen     Length of pattern
   * \return            Position of match
   */
  uint64_t slow_memmem(const void *start_buff, size_t buflen, const void *pat, size_t patlen)
  {
    const void *buf = start_buff;
    const void *end = (const char*)buf + buflen;
    if (patlen > buflen || patlen == 0)
    {
      return 0;
    }
    while (buflen > 0 && (buf = memchr(buf,((const char*)pat)[0], buflen - patlen + 1)))
    {
      if (memcmp(buf,pat,patlen) == 0)
        return (const char*)buf - (const char*)start_buff;
      buf = (const char*)buf + 1;
      buflen = (const char*)end - (const char*)buf;
    }
    return 0;
  }

  uint64_t get_block_reward(const cryptonote::block& blk)
  {
    uint64_t reward = 0;
    BOOST_FOREACH(const cryptonote::tx_out& out, blk.miner_tx.vout)
    {
      reward += out.amount;
    }
    return reward;
  }

  void fill_block_header_response(const cryptonote::block& blk, bool orphan_status, uint64_t height,
    const crypto::hash& hash, rapidjson::Value &header_response, rapidjson::Document &root_doc)
  {
    rapidjson::Value string_value(rapidjson::kStringType);
    header_response.AddMember("major_version", blk.major_version, root_doc.GetAllocator());
    header_response.AddMember("minor_version", blk.minor_version, root_doc.GetAllocator());
    header_response.AddMember("timestamp", blk.timestamp, root_doc.GetAllocator());
    std::string string_prev_hash = epee::string_tools::pod_to_hex(blk.prev_id);
    string_value.SetString(string_prev_hash.c_str(), string_prev_hash.length());
    header_response.AddMember("prev_hash", string_value, root_doc.GetAllocator());
    header_response.AddMember("nonce", blk.nonce, root_doc.GetAllocator());
    header_response.AddMember("orphan_status", orphan_status, root_doc.GetAllocator());
    header_response.AddMember("height", height, root_doc.GetAllocator());
    header_response.AddMember("depth", core->get_current_blockchain_height() - height - 1,
      root_doc.GetAllocator());
    std::string string_hash = epee::string_tools::pod_to_hex(hash);
    string_value.SetString(string_hash.c_str(), string_hash.length());
    header_response.AddMember("hash", string_value, root_doc.GetAllocator());
    header_response.AddMember("difficulty", core->get_blockchain_storage().block_difficulty(height),
      root_doc.GetAllocator());
    header_response.AddMember("reward", get_block_reward(blk), root_doc.GetAllocator());
  }

  // The actual RPC method implementations start here.

  /*!
   * \brief Implementation of 'getheight' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int getheight(char *buf, int len, struct ns_rpc_request *req)
  {
    CHECK_CORE_BUSY();
    uint64_t height = core->get_current_blockchain_height();
    rapidjson::Document response_json;
    rapidjson::Value result_json;
    result_json.SetObject();
    result_json.AddMember("height", height, response_json.GetAllocator());
    result_json.AddMember("status", CORE_RPC_STATUS_OK, response_json.GetAllocator());
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
    CHECK_CORE_BUSY();
    if (req->params == NULL)
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Parameters missing.", "{}");
    }
    rapidjson::Document request_json;
    char request_buf[1000];
    strncpy(request_buf, req->params[0].ptr, req->params[0].len);
    request_buf[req->params[0].len] = '\0';
    if (request_json.Parse(request_buf).HasParseError())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::parse_error,
        "Invalid JSON passed", "{}");
    }

    if (!request_json.HasMember("start_height") || !request_json["start_height"].IsUint64())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Incorrect start_height", "{}");
    }
    if (!request_json.HasMember("block_ids") || !request_json["block_ids"].IsArray())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Incorrect block_ids", "{}");
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
        return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
          "Wrong type in block_ids", "{}");
      }
      if (strlen(it->GetString()) > crypto::HASH_SIZE)
      {
        return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
          "Block ID exceeds length.", "{}");
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
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::internal_error,
        "Failed", "{}");
    }

    rapidjson::Document response_json;
    rapidjson::Value result_json;
    result_json.SetObject();
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

    result_json.AddMember("start_height", result_start_height, allocator);
    result_json.AddMember("current_height", result_current_height, allocator);
    result_json.AddMember("status", CORE_RPC_STATUS_OK, allocator);
    result_json.AddMember("blocks", block_json, allocator);

    std::string response;
    construct_response_string(req, result_json, response_json, response);

    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
  }

  /*!
   * \brief Implementation of 'gettransactions' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int gettransactions(char *buf, int len, struct ns_rpc_request *req)
  {
    CHECK_CORE_BUSY();
    if (req->params == NULL)
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Parameters missing.", "{}");
    }
    rapidjson::Document request_json;
    char request_buf[1000];
    strncpy(request_buf, req->params[0].ptr, req->params[0].len);
    request_buf[req->params[0].len] = '\0';
    if (request_json.Parse(request_buf).HasParseError())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::parse_error,
        "Invalid JSON passed", "{}");
    }

    if (!request_json.HasMember("txs_hashes") || !request_json["txs_hashes"].IsArray())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Incorrect txs_hashes", "{}");
    }

    std::list<std::string> txs_hashes;
    for (rapidjson::Value::ConstValueIterator it = request_json["txs_hashes"].Begin();
      it != request_json["txs_hashes"].End(); it++)
    {
      if (!it->IsString())
      {
        return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
          "Wrong type in txs_hashes", "{}");
      }
      txs_hashes.push_back(std::string(it->GetString()));
    }
    std::vector<crypto::hash> vh;
    BOOST_FOREACH(const auto& tx_hex_str, txs_hashes)
    {
      cryptonote::blobdata b;
      if (!epee::string_tools::parse_hexstr_to_binbuff(tx_hex_str, b))
      {
        return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
          "Failed to parse hex representation of transaction hash", "{}");
      }
      if (b.size() != sizeof(crypto::hash))
      {
        return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
          "Failed, size of data mismatch", "{}");
      }
      vh.push_back(*reinterpret_cast<const crypto::hash*>(b.data()));
    }
    std::list<crypto::hash> missed_txs;
    std::list<cryptonote::transaction> txs;
    bool r = core->get_transactions(vh, txs, missed_txs);
    if (!r)
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::internal_error,
        "Failed", "{}");
    }

    rapidjson::Document response_json;
    rapidjson::Value result_json;
    result_json.SetObject();
    rapidjson::Document::AllocatorType &allocator = response_json.GetAllocator();

    rapidjson::Value txs_as_hex(rapidjson::kArrayType);
    BOOST_FOREACH(auto &tx, txs)
    {
      cryptonote::blobdata blob = t_serializable_object_to_blob(tx);
      std::string string_blob = epee::string_tools::buff_to_hex_nodelimer(blob);
      rapidjson::Value string_value(rapidjson::kStringType);
      string_value.SetString(string_blob.c_str(), string_blob.length());
      txs_as_hex.PushBack(string_value, allocator);
    }
    result_json.AddMember("txs_as_hex", txs_as_hex, response_json.GetAllocator());

    rapidjson::Value missed_tx(rapidjson::kArrayType);
    BOOST_FOREACH(const auto &miss_tx, missed_txs)
    {
      std::string string_blob = epee::string_tools::pod_to_hex(miss_tx);
      rapidjson::Value string_value(rapidjson::kStringType);
      string_value.SetString(string_blob.c_str(), string_blob.length());
      missed_tx.PushBack(string_value, allocator);
    }
    result_json.AddMember("missed_tx", missed_tx, response_json.GetAllocator());

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
    CHECK_CORE_BUSY();
    if (req->params == NULL)
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Parameters missing.", "{}");
    }
    rapidjson::Document request_json;
    char request_buf[1000];
    strncpy(request_buf, req->params[0].ptr, req->params[0].len);
    request_buf[req->params[0].len] = '\0';
    if (request_json.Parse(request_buf).HasParseError())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::parse_error,
        "Invalid JSON passed", "{}");
    }

    if (!request_json.HasMember("miner_address") || !request_json["miner_address"].IsString())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Incorrect miner_address", "{}");
    }
    if (!request_json.HasMember("threads_count") || !request_json["threads_count"].IsUint64())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Incorrect threads_count", "{}");
    }

    std::string miner_address = request_json["miner_address"].GetString();
    uint64_t threads_count = request_json["threads_count"].GetUint();

    cryptonote::account_public_address adr;
    if (!cryptonote::get_account_address_from_str(adr, testnet, miner_address))
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Failed, wrong address", "{}");
    }

    boost::thread::attributes attrs;
    attrs.set_stack_size(THREAD_STACK_SIZE);
    if (!core->get_miner().start(adr, static_cast<size_t>(threads_count), attrs))
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_request,
        "Failed, mining not started", "{}");
    }
    return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", CORE_RPC_STATUS_OK);
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
    CHECK_CORE_BUSY();
    if (!core->get_miner().stop())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_request,
        "Failed, mining not stopped", "{}");
    }
    return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", CORE_RPC_STATUS_OK);
  }

  /*!
   * \brief Implementation of 'miningstatus' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int miningstatus(char *buf, int len, struct ns_rpc_request *req)
  {
    CHECK_CORE_BUSY();
    rapidjson::Document response_json;
    rapidjson::Value result_json;
    result_json.SetObject();
    bool active = core->get_miner().is_mining();
    result_json.AddMember("active", active, response_json.GetAllocator());

    if (active)
    {
      uint64_t speed = core->get_miner().get_speed();
      uint64_t threads_count = core->get_miner().get_threads_count();
      const cryptonote::account_public_address &mining_address = core->get_miner().get_mining_address();
      std::string address = cryptonote::get_account_address_as_str(testnet, mining_address);
      result_json.AddMember("speed", speed, response_json.GetAllocator());
      result_json.AddMember("threads_count", threads_count, response_json.GetAllocator());
      rapidjson::Value string_address(rapidjson::kStringType);
      string_address.SetString(address.c_str(), address.length());
      result_json.AddMember("address", string_address, response_json.GetAllocator());
    }
    result_json.AddMember("status", CORE_RPC_STATUS_OK, response_json.GetAllocator());

    std::string response;
    construct_response_string(req, result_json, response_json, response);

    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
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
    CHECK_CORE_BUSY();
    rapidjson::Document response_json;
    rapidjson::Value result_json;
    result_json.SetObject();
    result_json.AddMember("count", core->get_current_blockchain_height(), response_json.GetAllocator());
    result_json.AddMember("status", CORE_RPC_STATUS_OK, response_json.GetAllocator());

    std::string response;
    construct_response_string(req, result_json, response_json, response);

    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
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
    CHECK_CORE_BUSY();
    if (req->params == NULL)
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Parameters missing.", "{}");
    }
    rapidjson::Document request_json;
    char request_buf[1000];
    strncpy(request_buf, req->params[0].ptr, req->params[0].len);
    request_buf[req->params[0].len] = '\0';
    if (request_json.Parse(request_buf).HasParseError())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::parse_error,
        "Invalid JSON passed", "{}");
    }

    if (!request_json.HasMember("height") || !request_json["height"].IsUint64())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Incorrect height", "{}");
    }
    uint64_t height = request_json["height"].GetUint();
    if (core->get_current_blockchain_height() <= height)
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Height specified is too big.", "{}");
    }
    std::string hash = epee::string_tools::pod_to_hex(core->get_block_id_by_height(height));
    rapidjson::Document response_json;
    rapidjson::Value result_json;
    result_json.SetObject();
    rapidjson::Value string_value(rapidjson::kStringType);
    string_value.SetString(hash.c_str(), hash.length());
    result_json.AddMember("hash", string_value, response_json.GetAllocator());
    result_json.AddMember("status", CORE_RPC_STATUS_OK, response_json.GetAllocator());

    std::string response;
    construct_response_string(req, result_json, response_json, response);
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
    CHECK_CORE_BUSY();
    if (req->params == NULL)
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Parameters missing.", "{}");
    }
    rapidjson::Document request_json;
    char request_buf[1000];
    strncpy(request_buf, req->params[0].ptr, req->params[0].len);
    request_buf[req->params[0].len] = '\0';

    if (request_json.Parse(request_buf).HasParseError())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::parse_error,
        "Invalid JSON passed", "{}");
    }

    if (!request_json.HasMember("reserve_size") || !request_json["reserve_size"].IsUint64())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Incorrect reserve_size", "{}");
    }
    if (!request_json.HasMember("wallet_address") || !request_json["wallet_address"].IsString())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Incorrect wallet_address", "{}");
    }

    uint64_t reserve_size = request_json["reserve_size"].GetUint();
    std::string wallet_address = request_json["wallet_address"].GetString();
    
    if (reserve_size > 255)
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "To big reserved size, maximum 255", "{}");
    }

    cryptonote::account_public_address acc = AUTO_VAL_INIT(acc);
    if (!wallet_address.size() || !cryptonote::get_account_address_from_str(acc, testnet, wallet_address))
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Failed to parse wallet address", "{}");
    }

    cryptonote::block b = AUTO_VAL_INIT(b);
    cryptonote::blobdata blob_reserve;
    blob_reserve.resize(reserve_size, 0);

    uint64_t difficulty, height, reserved_offset;
    if (!core->get_block_template(b, acc, difficulty, height, blob_reserve))
    {
      LOG_ERROR("Failed to create block template");
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::internal_error,
        "Failed to create block template", "{}");
    }

    cryptonote::blobdata block_blob = t_serializable_object_to_blob(b);
    crypto::public_key tx_pub_key = cryptonote::get_tx_pub_key_from_extra(b.miner_tx);
    if (tx_pub_key == cryptonote::null_pkey)
    {
      LOG_ERROR("Failed to tx pub key in coinbase extra");
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::internal_error,
        "Failed to create block template", "{}");
    }

    reserved_offset = slow_memmem((void*)block_blob.data(), block_blob.size(), &tx_pub_key, sizeof(tx_pub_key));
    if (!reserved_offset)
    {
      LOG_ERROR("Failed to find tx pub key in blockblob");
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::internal_error,
        "Internal error: failed to create block template", "{}");
    }

    reserved_offset += sizeof(tx_pub_key) + 3; // 3 bytes: tag for TX_EXTRA_TAG_PUBKEY(1 byte), tag for TX_EXTRA_NONCE(1 byte), counter in TX_EXTRA_NONCE(1 byte)
    if (reserved_offset + reserve_size > block_blob.size())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::internal_error,
        "Internal error: failed to create block template", "{}");
    }
    cryptonote::blobdata blocktemplate_blob = epee::string_tools::buff_to_hex_nodelimer(block_blob);

    rapidjson::Document response_json;
    rapidjson::Value result_json;
    result_json.SetObject();
    result_json.AddMember("difficulty", difficulty, response_json.GetAllocator());
    result_json.AddMember("height", height, response_json.GetAllocator());
    result_json.AddMember("reserved_offset", reserved_offset, response_json.GetAllocator());
    rapidjson::Value string_value(rapidjson::kStringType);
    string_value.SetString(blocktemplate_blob.c_str(), blocktemplate_blob.length());
    result_json.AddMember("blocktemplate_blob", string_value, response_json.GetAllocator());
    result_json.AddMember("status", CORE_RPC_STATUS_OK, response_json.GetAllocator());

    std::string response;
    construct_response_string(req, result_json, response_json, response);
    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
  }

  /*!
   * \brief Implementation of 'submitblock' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int submitblock(char *buf, int len, struct ns_rpc_request *req)
  {
    CHECK_CORE_BUSY();
    if (req->params == NULL)
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Parameters missing.", "{}");
    }
    rapidjson::Document request_json;
    char request_buf[1000];
    strncpy(request_buf, req->params[0].ptr, req->params[0].len);
    request_buf[req->params[0].len] = '\0';

    if (request_json.Parse(request_buf).HasParseError())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::parse_error,
        "Invalid JSON passed", "{}");
    }
    if (!request_json.HasMember("block") || !request_json["block"].IsString())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Incorrect block", "{}");
    }

    std::string string_blockblob = request_json["block"].GetString();
    cryptonote::blobdata blockblob;
    if (!epee::string_tools::parse_hexstr_to_binbuff(string_blockblob, blockblob))
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_request,
        "Incorrect block", "{}");
    }
    // Fixing of high orphan issue for most pools
    cryptonote::block b = AUTO_VAL_INIT(b);
    if (!parse_and_validate_block_from_blob(blockblob, b))
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_request,
        "Wrong block blob", "{}");
    }
    // Fix from Boolberry neglects to check block
    // size, do that with the function below
    if (!core->check_incoming_block_size(blockblob))
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_request,
        "Block blob size is too big, rejecting block", "{}");
    }
    if (!core->handle_block_found(b))
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_request,
        "Block not accepted.", "{}");
    }

    return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", CORE_RPC_STATUS_OK);
  }

  /*!
   * \brief Implementation of 'getlastblockheader' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int getlastblockheader(char *buf, int len, struct ns_rpc_request *req)
  {
    CHECK_CORE_BUSY();
    uint64_t last_block_height;
    crypto::hash last_block_hash;
    bool have_last_block_hash = core->get_blockchain_top(last_block_height, last_block_hash);

    if (!have_last_block_hash)
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_request,
        "Internal error: can't get last block hash", "{}");
    }
    
    cryptonote::block last_block;
    bool have_last_block = core->get_block_by_hash(last_block_hash, last_block);
    if (!have_last_block)
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_request,
        "Internal error: can't get last block hash", "{}");
    }
    rapidjson::Document response_json;
    rapidjson::Value result_json;
    result_json.SetObject();
    fill_block_header_response(last_block, false, last_block_height, last_block_hash,
      result_json, response_json);
    result_json.AddMember("status", CORE_RPC_STATUS_OK, response_json.GetAllocator());

    std::string response;
    construct_response_string(req, result_json, response_json, response);
    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
  }

  /*!
   * \brief Implementation of 'getblockheaderbyhash' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int getblockheaderbyhash(char *buf, int len, struct ns_rpc_request *req)
  {
    CHECK_CORE_BUSY();
    if (req->params == NULL)
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Parameters missing.", "{}");
    }
    rapidjson::Document request_json;
    char request_buf[1000];
    strncpy(request_buf, req->params[0].ptr, req->params[0].len);
    request_buf[req->params[0].len] = '\0';

    if (request_json.Parse(request_buf).HasParseError())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::parse_error,
        "Invalid JSON passed", "{}");
    }
    if (!request_json.HasMember("hash") || !request_json["hash"].IsString())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Incorrect hash", "{}");
    }

    std::string hash = request_json["hash"].GetString();
    crypto::hash block_hash;
    bool hash_parsed = parse_hash256(hash, block_hash);
    if (!hash_parsed)
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        std::string("Failed to parse hex representation of block hash. Hex = " + hash + '.').c_str(), "{}");
    }

    cryptonote::block blk;
    bool have_block = core->get_block_by_hash(block_hash, blk);
    if (!have_block)
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::internal_error,
        std::string("Internal error: can't get block by hash. Hash = " + hash + '.').c_str(), "{}");
    }    

    if (blk.miner_tx.vin.front().type() != typeid(cryptonote::txin_gen))
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::internal_error,
        std::string("Internal error: coinbase transaction in the block has the wrong type").c_str(), "{}");
    }

    uint64_t block_height = boost::get<cryptonote::txin_gen>(blk.miner_tx.vin.front()).height;
    rapidjson::Document response_json;
    rapidjson::Value result_json;
    result_json.SetObject();
    result_json.AddMember("status", CORE_RPC_STATUS_OK, response_json.GetAllocator());
    fill_block_header_response(blk, false, block_height, block_hash, result_json, response_json);

    std::string response;
    construct_response_string(req, result_json, response_json, response);
    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
  }

  /*!
   * \brief Implementation of 'getblockheaderbyheight' method.
   * \param  buf Buffer to fill in response.
   * \param  len Max length of response.
   * \param  req net_skeleton RPC request
   * \return     Actual response length.
   */
  int getblockheaderbyheight(char *buf, int len, struct ns_rpc_request *req)
  {
    CHECK_CORE_BUSY();
    if (req->params == NULL)
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Parameters missing.", "{}");
    }
    rapidjson::Document request_json;
    char request_buf[1000];
    strncpy(request_buf, req->params[0].ptr, req->params[0].len);
    request_buf[req->params[0].len] = '\0';

    if (request_json.Parse(request_buf).HasParseError())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::parse_error,
        "Invalid JSON passed", "{}");
    }
    if (!request_json.HasMember("height") || !request_json["height"].IsUint64())
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        "Incorrect height", "{}");
    }

    uint64_t height = request_json["height"].GetUint();
    if (core->get_current_blockchain_height() <= height)
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        std::string(std::string("To big height: ") + std::to_string(height) +
        ", current blockchain height = " + std::to_string(core->get_current_blockchain_height())).c_str(), "{}");
    }
    crypto::hash block_hash = core->get_block_id_by_height(height);
    cryptonote::block blk;
    bool have_block = core->get_block_by_hash(block_hash, blk);
    if (!have_block)
    {
      return ns_rpc_create_error(buf, len, req, RPC::Json_rpc_http_server::invalid_params,
        std::string("Internal error: can't get block by height. Height = " + std::to_string(height) + '.').c_str(), "{}");
    }

    rapidjson::Document response_json;
    rapidjson::Value result_json;
    result_json.SetObject();
    result_json.AddMember("status", CORE_RPC_STATUS_OK, response_json.GetAllocator());
    fill_block_header_response(blk, false, height, block_hash, result_json, response_json);

    std::string response;
    construct_response_string(req, result_json, response_json, response);
    size_t copy_length = ((uint32_t)len > response.length()) ? response.length() + 1 : (uint32_t)len;
    strncpy(buf, response.c_str(), copy_length);
    return response.length();
  }

  // Contains a list of method names.
  const char *method_names[] = {
    "getheight",
    "getblocks",
    "gettransactions",
    "startmining",
    "stopmining",
    "miningstatus",
    "getblockcount",
    "getblockhash",
    "getblocktemplate",
    "submitblock",
    "getlastblockheader",
    "getblockheaderbyhash",
    "getblockheaderbyheight",
    NULL
  };

  // Contains a list of function pointers. These must map 1-1 by index with `method_names`.
  ns_rpc_handler_t handlers[] = {
    getheight,
    getblocks,
    gettransactions,
    startmining,
    stopmining,
    miningstatus,
    getblockcount,
    getblockhash,
    getblocktemplate,
    submitblock,
    getlastblockheader,
    getblockheaderbyhash,
    getblockheaderbyheight,
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
   * \brief initializes module (must call this before handling requests)
   * \param p_core    Pointer to cryptonote core object
   * \param p_p2p     Pointer to P2P object
   * \param p_testnet True if testnet false otherwise
   */
  void init(cryptonote::core *p_core,
    nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > *p_p2p,
    bool p_testnet)
  {
    core = p_core;
    p2p = p_p2p;
    testnet = p_testnet;
  }

  /*!
   * \Inits certain options used in Daemon CLI.
   * \param desc Instance of options description object
   */
  void init_options(boost::program_options::options_description& desc)
  {
    command_line::add_arg(desc, arg_rpc_bind_ip);
    command_line::add_arg(desc, arg_rpc_bind_port);
    command_line::add_arg(desc, arg_testnet_rpc_bind_port);
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
    auto p2p_bind_arg = testnet ? arg_testnet_rpc_bind_port : arg_rpc_bind_port;

    ip_address = command_line::get_arg(vm, arg_rpc_bind_ip);
    port = command_line::get_arg(vm, p2p_bind_arg);
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
