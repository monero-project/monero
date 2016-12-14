// Copyright (c) 2016, The Monero Project
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

#include "json_object.h"

#include "string_tools.h"

namespace cryptonote
{

namespace json
{
template <class Type, typename>
Type fromJsonValue(const rapidjson::Value& val)
{
  Type retval;

  if (!val.IsString())
  {
    throw WRONG_TYPE("string");
  }

  //TODO: handle failure to convert hex string to POD type
  bool success = epee::string_tools::hex_to_pod(val.GetString(), retval);

  if (!success)
  {
    throw BAD_INPUT();
  }

  return retval;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const std::string& i)
{
  return rapidjson::Value(i.c_str(), doc.GetAllocator());
}

template <>
std::string fromJsonValue(const rapidjson::Value& i)
{
  if (!i.IsString())
  {
    throw WRONG_TYPE("string");
  }

  return i.GetString();
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, bool i)
{
  rapidjson::Value val;

  val.SetBool(i);

  return val;
}

bool fromJsonValue(const rapidjson::Value& i)
{
  if (!i.IsBool())
  {
    throw WRONG_TYPE("boolean");
  }
  return i.GetBool();
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const uint8_t& i)
{
  return rapidjson::Value(i);
}

template <>
uint8_t fromJsonValue(const rapidjson::Value& i)
{
  if (!i.IsUint())
  {
    throw WRONG_TYPE("unsigned integer");
  }

  return (uint8_t) ( i.GetUint() & 0xFF);
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const int8_t& i)
{
  return rapidjson::Value(i);
}

template <>
int8_t fromJsonValue(const rapidjson::Value& i)
{
  if (!i.IsInt())
  {
    throw WRONG_TYPE("integer");
  }

  return (int8_t) ( i.GetInt() & 0xFF);
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const uint16_t& i)
{
  return rapidjson::Value(i);
}

template <>
uint16_t fromJsonValue(const rapidjson::Value& i)
{
  if (!i.IsUint())
  {
    throw WRONG_TYPE("unsigned integer");
  }

  return (uint16_t) ( i.GetUint() & 0xFFFF);
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const uint32_t& i)
{
  return rapidjson::Value(i);
}

template <>
uint32_t fromJsonValue(const rapidjson::Value& i)
{
  if (!i.IsUint())
  {
    throw WRONG_TYPE("unsigned integer");
  }

  return i.GetUint();
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const uint64_t& i)
{
  return rapidjson::Value(i);
}

template <>
uint64_t fromJsonValue(const rapidjson::Value& i)
{
  if (!i.IsUint64())
  {
    throw WRONG_TYPE("unsigned integer");
  }

  return i.GetUint64();
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::transaction& tx)
{
  rapidjson::Value val;

  val.SetObject();

  val.AddMember("version", toJsonValue(doc, tx.version), doc.GetAllocator());
  val.AddMember("unlock_time", toJsonValue(doc, tx.unlock_time), doc.GetAllocator());
  val.AddMember("vin", toJsonValue(doc, tx.vin), doc.GetAllocator());
  val.AddMember("vout", toJsonValue(doc, tx.vout), doc.GetAllocator());
  val.AddMember("extra", toJsonValue(doc, tx.extra), doc.GetAllocator());
  val.AddMember("signatures", toJsonValue(doc, tx.signatures), doc.GetAllocator());

  return val;
}

template <>
cryptonote::transaction fromJsonValue<cryptonote::transaction>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::transaction tx;

  OBJECT_HAS_MEMBER_OR_THROW(val, "version")
  tx.version = fromJsonValue<size_t>(val["version"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "unlock_time")
  tx.unlock_time = fromJsonValue<uint64_t>(val["unlock_time"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "vin")
  tx.vin = fromJsonValue<std::vector<cryptonote::txin_v> >(val["vin"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "vout")
  tx.vout = fromJsonValue<std::vector<cryptonote::tx_out> >(val["vout"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "extra")
  tx.extra = fromJsonValue<std::vector<uint8_t> >(val["extra"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "signatures")
  tx.signatures = fromJsonValue<std::vector<std::vector<crypto::signature> > >(val["signatures"]);


  return tx;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::block& b)
{
  rapidjson::Value val;

  val.SetObject();

  val.AddMember("major_version", toJsonValue(doc, b.major_version), doc.GetAllocator());
  val.AddMember("minor_version", toJsonValue(doc, b.minor_version), doc.GetAllocator());
  val.AddMember("timestamp", toJsonValue(doc, b.timestamp), doc.GetAllocator());
  val.AddMember("prev_id", toJsonValue(doc, b.prev_id), doc.GetAllocator());
  val.AddMember("nonce", toJsonValue(doc, b.nonce), doc.GetAllocator());
  val.AddMember("miner_tx", toJsonValue(doc, b.miner_tx), doc.GetAllocator());
  val.AddMember("tx_hashes", toJsonValue(doc, b.tx_hashes), doc.GetAllocator());

  return val;
}

template <>
cryptonote::block fromJsonValue<cryptonote::block>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::block b;

  OBJECT_HAS_MEMBER_OR_THROW(val, "major_version")
  b.major_version = fromJsonValue<uint8_t>(val["major_version"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "minor_version")
  b.minor_version = fromJsonValue<uint8_t>(val["minor_version"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "timestamp")
  b.timestamp = fromJsonValue<uint64_t>(val["timestamp"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "prev_id")
  b.prev_id = fromJsonValue<crypto::hash>(val["prev_id"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "nonce")
  b.nonce = fromJsonValue<uint32_t>(val["nonce"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "miner_tx")
  b.miner_tx = fromJsonValue<transaction>(val["miner_tx"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "tx_hashes")
  b.tx_hashes = fromJsonValue<std::vector<crypto::hash> >(val["tx_hashes"]);


  return b;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::txin_v& txin)
{
  rapidjson::Value val;

  val.SetObject();

  if (txin.type() == typeid(cryptonote::txin_gen))
  {
    val.AddMember("type", "txin_gen", doc.GetAllocator());
    val.AddMember("value", toJsonValue(doc, boost::get<cryptonote::txin_gen>(txin)), doc.GetAllocator());
  }
  else if (txin.type() == typeid(cryptonote::txin_to_script))
  {
    val.AddMember("type", "txin_to_script", doc.GetAllocator());
    val.AddMember("value", toJsonValue(doc, boost::get<cryptonote::txin_to_script>(txin)), doc.GetAllocator());
  }
  else if (txin.type() == typeid(cryptonote::txin_to_scripthash))
  {
    val.AddMember("type", "txin_to_scripthash", doc.GetAllocator());
    val.AddMember("value", toJsonValue(doc, boost::get<cryptonote::txin_to_scripthash>(txin)), doc.GetAllocator());
  }
  else if (txin.type() == typeid(cryptonote::txin_to_key))
  {
    val.AddMember("type", "txin_to_key", doc.GetAllocator());
    val.AddMember("value", toJsonValue(doc, boost::get<cryptonote::txin_to_key>(txin)), doc.GetAllocator());
  }

  return val;
}

template <>
cryptonote::txin_v fromJsonValue<cryptonote::txin_v>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::txin_v txin;

  OBJECT_HAS_MEMBER_OR_THROW(val, "type")
  if (val["type"]== "txin_gen")

  {
    OBJECT_HAS_MEMBER_OR_THROW(val, "value")
    txin = fromJsonValue<cryptonote::txin_gen>(val["value"]);

  }
  OBJECT_HAS_MEMBER_OR_THROW(val, "type")
  else if (val["type"]== "txin_to_script")

  {
    OBJECT_HAS_MEMBER_OR_THROW(val, "value")
    txin = fromJsonValue<cryptonote::txin_to_script>(val["value"]);

  }
  OBJECT_HAS_MEMBER_OR_THROW(val, "type")
  else if (val["type"] == "txin_to_scripthash")

  {
    OBJECT_HAS_MEMBER_OR_THROW(val, "value")
    txin = fromJsonValue<cryptonote::txin_to_scripthash>(val["value"]);

  }
  OBJECT_HAS_MEMBER_OR_THROW(val, "type")
  else if (val["type"] == "txin_to_key")

  {
    OBJECT_HAS_MEMBER_OR_THROW(val, "value")
    txin = fromJsonValue<cryptonote::txin_to_key>(val["value"]);

  }

  return txin;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::txin_gen& txin)
{
  rapidjson::Value val;

  val.SetObject();

  val.AddMember("height", toJsonValue(doc, txin.height), doc.GetAllocator());

  return val;
}

template <>
cryptonote::txin_gen fromJsonValue<cryptonote::txin_gen>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::txin_gen txin;

  OBJECT_HAS_MEMBER_OR_THROW(val, "height")
  txin.height = fromJsonValue<size_t>(val["height"]);


  return txin;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::txin_to_script& txin)
{
  rapidjson::Value val;

  val.SetObject();

  val.AddMember("prev", toJsonValue(doc, txin.prev), doc.GetAllocator());
  val.AddMember("prevout", toJsonValue(doc, txin.prevout), doc.GetAllocator());
  val.AddMember("sigset", toJsonValue(doc, txin.sigset), doc.GetAllocator());

  return val;
}

template <>
cryptonote::txin_to_script fromJsonValue<cryptonote::txin_to_script>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::txin_to_script txin;

  OBJECT_HAS_MEMBER_OR_THROW(val, "prev")
  txin.prev = fromJsonValue<crypto::hash>(val["prev"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "prevout")
  txin.prevout = fromJsonValue<size_t>(val["prevout"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "sigset")
  txin.sigset = fromJsonValue<std::vector<uint8_t> >(val["sigset"]);


  return txin;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::txin_to_scripthash& txin)
{
  rapidjson::Value val;

  val.SetObject();

  val.AddMember("prev", toJsonValue(doc, txin.prev), doc.GetAllocator());
  val.AddMember("prevout", toJsonValue(doc, txin.prevout), doc.GetAllocator());
  val.AddMember("script", toJsonValue(doc, txin.script), doc.GetAllocator());
  val.AddMember("sigset", toJsonValue(doc, txin.sigset), doc.GetAllocator());

  return val;
}

template <>
cryptonote::txin_to_scripthash fromJsonValue<cryptonote::txin_to_scripthash>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::txin_to_scripthash txin;

  OBJECT_HAS_MEMBER_OR_THROW(val, "prev")
  txin.prev = fromJsonValue<crypto::hash>(val["prev"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "prevout")
  txin.prevout = fromJsonValue<size_t>(val["prevout"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "script")
  txin.script = fromJsonValue<txout_to_script>(val["script"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "sigset")
  txin.sigset = fromJsonValue<std::vector<uint8_t> >(val["sigset"]);


  return txin;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::txin_to_key& txin)
{
  rapidjson::Value val;

  val.SetObject();

  val.AddMember("amount", toJsonValue(doc, txin.amount), doc.GetAllocator());
  val.AddMember("key_offsets", toJsonValue(doc, txin.key_offsets), doc.GetAllocator());
  val.AddMember("k_image", toJsonValue(doc, txin.k_image), doc.GetAllocator());

  return val;
}

template <>
cryptonote::txin_to_key fromJsonValue<cryptonote::txin_to_key>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::txin_to_key txin;

  OBJECT_HAS_MEMBER_OR_THROW(val, "amount")
  txin.amount = fromJsonValue<uint64_t>(val["amount"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "key_offsets")
  txin.key_offsets = fromJsonValue<std::vector<uint64_t> >(val["key_offsets"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "k_image")
  txin.k_image = fromJsonValue<crypto::key_image>(val["k_image"]);


  return txin;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::txout_target_v& txout)
{
  rapidjson::Value val;

  val.SetObject();

  if (txout.type() == typeid(cryptonote::txout_to_script))
  {
    val.AddMember("type", "txout_to_script", doc.GetAllocator());
    val.AddMember("value", toJsonValue(doc, boost::get<cryptonote::txout_to_script>(txout)), doc.GetAllocator());
  }
  else if (txout.type() == typeid(cryptonote::txout_to_scripthash))
  {
    val.AddMember("type", "txout_to_scripthash", doc.GetAllocator());
    val.AddMember("value", toJsonValue(doc, boost::get<cryptonote::txout_to_scripthash>(txout)), doc.GetAllocator());
  }
  else if (txout.type() == typeid(cryptonote::txout_to_key))
  {
    val.AddMember("type", "txout_to_key", doc.GetAllocator());
    val.AddMember("value", toJsonValue(doc, boost::get<cryptonote::txout_to_key>(txout)), doc.GetAllocator());
  }

  return val;
}

template <>
cryptonote::txout_target_v fromJsonValue<cryptonote::txout_target_v>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::txout_target_v txout;

  OBJECT_HAS_MEMBER_OR_THROW(val, "type")
  if (val["type"]== "txout_to_script")

  {
    OBJECT_HAS_MEMBER_OR_THROW(val, "value")
    txout = fromJsonValue<cryptonote::txout_to_script>(val["value"]);

  }
  OBJECT_HAS_MEMBER_OR_THROW(val, "type")
  else if (val["type"] == "txout_to_scripthash")

  {
    OBJECT_HAS_MEMBER_OR_THROW(val, "value")
    txout = fromJsonValue<cryptonote::txout_to_scripthash>(val["value"]);

  }
  OBJECT_HAS_MEMBER_OR_THROW(val, "type")
  else if (val["type"] == "txout_to_key")

  {
    OBJECT_HAS_MEMBER_OR_THROW(val, "value")
    txout = fromJsonValue<cryptonote::txout_to_key>(val["value"]);

  }

  return txout;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::txout_to_script& txout)
{
  rapidjson::Value val;

  val.SetObject();

  val.AddMember("keys", toJsonValue(doc, txout.keys), doc.GetAllocator());
  val.AddMember("script", toJsonValue(doc, txout.script), doc.GetAllocator());

  return val;
}

template <>
cryptonote::txout_to_script fromJsonValue<cryptonote::txout_to_script>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::txout_to_script txout;

  OBJECT_HAS_MEMBER_OR_THROW(val, "keys")
  txout.keys = fromJsonValue<std::vector<crypto::public_key> >(val["keys"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "script")
  txout.script = fromJsonValue<std::vector<uint8_t> >(val["script"]);


  return txout;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::txout_to_scripthash& txout)
{
  rapidjson::Value val;

  val.SetObject();

  val.AddMember("hash", toJsonValue(doc, txout.hash), doc.GetAllocator());

  return val;
}

template <>
cryptonote::txout_to_scripthash fromJsonValue<cryptonote::txout_to_scripthash>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::txout_to_scripthash txout;

  OBJECT_HAS_MEMBER_OR_THROW(val, "hash")
  txout.hash = fromJsonValue<crypto::hash>(val["hash"]);


  return txout;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::txout_to_key& txout)
{
  rapidjson::Value val;

  val.SetObject();

  val.AddMember("key", toJsonValue(doc, txout.key), doc.GetAllocator());

  return val;
}

template <>
cryptonote::txout_to_key fromJsonValue<cryptonote::txout_to_key>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::txout_to_key txout;

  OBJECT_HAS_MEMBER_OR_THROW(val, "key")
  txout.key = fromJsonValue<crypto::public_key>(val["key"]);


  return txout;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::tx_out& txout)
{
  rapidjson::Value val;

  val.SetObject();

  val.AddMember("amount", toJsonValue(doc, txout.amount), doc.GetAllocator());
  val.AddMember("target", toJsonValue(doc, txout.target), doc.GetAllocator());

  return val;
}

template <>
cryptonote::tx_out fromJsonValue<cryptonote::tx_out>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::tx_out txout;

  OBJECT_HAS_MEMBER_OR_THROW(val, "amount")
  txout.amount = fromJsonValue<uint64_t>(val["amount"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "target")
  txout.target = fromJsonValue<txout_target_v>(val["target"]);


  return txout;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::connection_info& info)
{
  rapidjson::Value val;

  val.SetObject();

  auto& al = doc.GetAllocator();

  val.AddMember("incoming", toJsonValue(doc, info.incoming), al);
  val.AddMember("localhost", toJsonValue(doc, info.localhost), al);
  val.AddMember("local_ip", toJsonValue(doc, info.local_ip), al);

  val.AddMember("ip", toJsonValue(doc, info.ip), al);
  val.AddMember("port", toJsonValue(doc, info.port), al);

  val.AddMember("peer_id", toJsonValue(doc, info.peer_id), al);

  val.AddMember("recv_count", toJsonValue(doc, info.recv_count), al);
  val.AddMember("recv_idle_time", toJsonValue(doc, info.recv_idle_time), al);

  val.AddMember("send_count", toJsonValue(doc, info.send_count), al);
  val.AddMember("send_idle_time", toJsonValue(doc, info.send_idle_time), al);

  val.AddMember("state", toJsonValue(doc, info.state), al);

  val.AddMember("live_time", toJsonValue(doc, info.live_time), al);

  val.AddMember("avg_download", toJsonValue(doc, info.avg_download), al);
  val.AddMember("current_download", toJsonValue(doc, info.current_download), al);

  val.AddMember("avg_upload", toJsonValue(doc, info.avg_upload), al);
  val.AddMember("current_upload", toJsonValue(doc, info.current_upload), al);

  return val;
}

template <>
cryptonote::connection_info fromJsonValue<cryptonote::connection_info>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::connection_info info;

  OBJECT_HAS_MEMBER_OR_THROW(val, "incoming")
  info.incoming = fromJsonValue<bool>(val["incoming"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "localhost")
  info.localhost = fromJsonValue<bool>(val["localhost"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "local_ip")
  info.local_ip = fromJsonValue<bool>(val["local_ip"]);


  OBJECT_HAS_MEMBER_OR_THROW(val, "ip")
  info.ip = fromJsonValue<std::string>(val["ip"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "port")
  info.port = fromJsonValue<std::string>(val["port"]);


  OBJECT_HAS_MEMBER_OR_THROW(val, "peer_id")
  info.peer_id = fromJsonValue<std::string>(val["peer_id"]);


  OBJECT_HAS_MEMBER_OR_THROW(val, "recv_count")
  info.recv_count = fromJsonValue<uint64_t>(val["recv_count"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "recv_idle_time")
  info.recv_idle_time = fromJsonValue<uint64_t>(val["recv_idle_time"]);


  OBJECT_HAS_MEMBER_OR_THROW(val, "send_count")
  info.send_count = fromJsonValue<uint64_t>(val["send_count"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "send_idle_time")
  info.send_idle_time = fromJsonValue<uint64_t>(val["send_idle_time"]);


  OBJECT_HAS_MEMBER_OR_THROW(val, "state")
  info.state = fromJsonValue<std::string>(val["state"]);


  OBJECT_HAS_MEMBER_OR_THROW(val, "live_time")
  info.live_time = fromJsonValue<uint64_t>(val["live_time"]);


  OBJECT_HAS_MEMBER_OR_THROW(val, "avg_download")
  info.avg_download = fromJsonValue<uint64_t>(val["avg_download"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "current_download")
  info.current_download = fromJsonValue<uint64_t>(val["current_download"]);


  OBJECT_HAS_MEMBER_OR_THROW(val, "avg_upload")
  info.avg_upload = fromJsonValue<uint64_t>(val["avg_upload"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "current_upload")
  info.current_upload = fromJsonValue<uint64_t>(val["current_upload"]);


  return info;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::block_complete_entry& blk)
{
  rapidjson::Value val;

  val.SetObject();

  val.AddMember("block", toJsonValue(doc, blk.block), doc.GetAllocator());
  val.AddMember("txs", toJsonValue(doc, blk.txs), doc.GetAllocator());

  return val;
}

template <>
cryptonote::block_complete_entry fromJsonValue<cryptonote::block_complete_entry>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::block_complete_entry blk;

  OBJECT_HAS_MEMBER_OR_THROW(val, "block")
  blk.block = fromJsonValue<blobdata>(val["block"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "txs")
  blk.txs = fromJsonValue<std::list<blobdata> >(val["txs"]);


  return blk;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::block_with_transactions& blk)
{
  rapidjson::Value val;

  val.SetObject();

  val.AddMember("block", toJsonValue(doc, blk.block), doc.GetAllocator());
  val.AddMember("transactions", toJsonValue(doc, blk.transactions), doc.GetAllocator());

  return val;
}

template <>
cryptonote::rpc::block_with_transactions fromJsonValue<cryptonote::rpc::block_with_transactions>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::rpc::block_with_transactions blk;

  OBJECT_HAS_MEMBER_OR_THROW(val, "block")
  blk.block = fromJsonValue<block>(val["block"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "transactions")
  blk.transactions = fromJsonValue<std::unordered_map<crypto::hash, cryptonote::transaction> >(val["transactions"]);


  return blk;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::transaction_info& tx_info)
{
  rapidjson::Value val;

  val.SetObject();

  auto& al = doc.GetAllocator();

  val.AddMember("height", toJsonValue(doc, tx_info.height), al);
  val.AddMember("in_pool", toJsonValue(doc, tx_info.in_pool), al);
  val.AddMember("transaction", toJsonValue(doc, tx_info.transaction), al);

  return val;
}

template <>
cryptonote::rpc::transaction_info fromJsonValue<cryptonote::rpc::transaction_info>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::rpc::transaction_info tx_info;

  OBJECT_HAS_MEMBER_OR_THROW(val, "height")
  tx_info.height = fromJsonValue<uint64_t>(val["height"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "in_pool")
  tx_info.in_pool = fromJsonValue<bool>(val["in_pool"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "transaction")
  tx_info.transaction = fromJsonValue<cryptonote::transaction>(val["transaction"]);


  return tx_info;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::output_key_and_amount_index& out)
{
  rapidjson::Value val;

  val.SetObject();

  auto& al = doc.GetAllocator();

  val.AddMember("amount_index", toJsonValue(doc, out.amount_index), al);
  val.AddMember("key", toJsonValue(doc, out.key), al);

  return val;
}

template <>
cryptonote::rpc::output_key_and_amount_index fromJsonValue<cryptonote::rpc::output_key_and_amount_index>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::rpc::output_key_and_amount_index out;

  OBJECT_HAS_MEMBER_OR_THROW(val, "amount_index")
  out.amount_index = fromJsonValue<decltype(out.amount_index)>(val["amount_index"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "key")
  out.key = fromJsonValue<decltype(out.key)>(val["key"]);


  return out;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::amount_with_random_outputs& out)
{
  rapidjson::Value val;

  val.SetObject();

  auto& al = doc.GetAllocator();

  val.AddMember("amount", toJsonValue(doc, out.amount), al);
  val.AddMember("outputs", toJsonValue(doc, out.outputs), al);

  return val;
}

template <>
cryptonote::rpc::amount_with_random_outputs fromJsonValue<cryptonote::rpc::amount_with_random_outputs>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::rpc::amount_with_random_outputs out;

  OBJECT_HAS_MEMBER_OR_THROW(val, "amount")
  out.amount = fromJsonValue<decltype(out.amount)>(val["amount"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "outputs")
  out.outputs = fromJsonValue<decltype(out.outputs)>(val["outputs"]);


  return out;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::peer& peer)
{
  rapidjson::Value val;

  val.SetObject();

  auto& al = doc.GetAllocator();

  val.AddMember("id", toJsonValue(doc, peer.id), al);
  val.AddMember("ip", toJsonValue(doc, peer.ip), al);
  val.AddMember("port", toJsonValue(doc, peer.port), al);
  val.AddMember("last_seen", toJsonValue(doc, peer.last_seen), al);

  return val;
}

template <>
cryptonote::rpc::peer fromJsonValue<cryptonote::rpc::peer>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::rpc::peer peer;

  OBJECT_HAS_MEMBER_OR_THROW(val, "id")
  peer.id = fromJsonValue<decltype(peer.id)>(val["id"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "ip")
  peer.ip = fromJsonValue<decltype(peer.ip)>(val["ip"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "port")
  peer.port = fromJsonValue<decltype(peer.port)>(val["port"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "last_seen")
  peer.last_seen = fromJsonValue<decltype(peer.last_seen)>(val["last_seen"]);


  return peer;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::tx_in_pool& tx)
{
  rapidjson::Value val;

  val.SetObject();

  auto& al = doc.GetAllocator();

  val.AddMember("tx", toJsonValue(doc, tx.tx), al);
  val.AddMember("blob_size", toJsonValue(doc, tx.blob_size), al);
  val.AddMember("fee", toJsonValue(doc, tx.fee), al);
  val.AddMember("max_used_block_hash", toJsonValue(doc, tx.max_used_block_hash), al);
  val.AddMember("max_used_block_height", toJsonValue(doc, tx.max_used_block_height), al);
  val.AddMember("kept_by_block", toJsonValue(doc, tx.kept_by_block), al);
  val.AddMember("last_failed_block_hash", toJsonValue(doc, tx.last_failed_block_hash), al);
  val.AddMember("last_failed_block_height", toJsonValue(doc, tx.last_failed_block_height), al);
  val.AddMember("receive_time", toJsonValue(doc, tx.receive_time), al);
  val.AddMember("last_relayed_time", toJsonValue(doc, tx.last_relayed_time), al);
  val.AddMember("relayed", toJsonValue(doc, tx.relayed), al);

  return val;
}

template <>
cryptonote::rpc::tx_in_pool fromJsonValue<cryptonote::rpc::tx_in_pool>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::rpc::tx_in_pool tx;

  OBJECT_HAS_MEMBER_OR_THROW(val, "tx")
  tx.tx = fromJsonValue<decltype(tx.tx)>(val["tx"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "blob_size")
  tx.blob_size = fromJsonValue<decltype(tx.blob_size)>(val["blob_size"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "fee")
  tx.fee = fromJsonValue<decltype(tx.fee)>(val["fee"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "max_used_block_hash")
  tx.max_used_block_hash = fromJsonValue<decltype(tx.max_used_block_hash)>(val["max_used_block_hash"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "max_used_block_height")
  tx.max_used_block_height = fromJsonValue<decltype(tx.max_used_block_height)>(val["max_used_block_height"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "kept_by_block")
  tx.kept_by_block = fromJsonValue<decltype(tx.kept_by_block)>(val["kept_by_block"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "last_failed_block_hash")
  tx.last_failed_block_hash = fromJsonValue<decltype(tx.last_failed_block_hash)>(val["last_failed_block_hash"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "last_failed_block_height")
  tx.last_failed_block_height = fromJsonValue<decltype(tx.last_failed_block_height)>(val["last_failed_block_height"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "receive_time")
  tx.receive_time = fromJsonValue<decltype(tx.receive_time)>(val["receive_time"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "last_relayed_time")
  tx.last_relayed_time = fromJsonValue<decltype(tx.last_relayed_time)>(val["last_relayed_time"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "relayed")
  tx.relayed = fromJsonValue<decltype(tx.relayed)>(val["relayed"]);


  return tx;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::output_amount_count& oac)
{
  rapidjson::Value val;

  val.SetObject();

  auto& al = doc.GetAllocator();

  val.AddMember("amount", toJsonValue(doc, oac.amount), al);
  val.AddMember("total_count", toJsonValue(doc, oac.total_count), al);
  val.AddMember("unlocked_count", toJsonValue(doc, oac.unlocked_count), al);
  val.AddMember("recent_count", toJsonValue(doc, oac.recent_count), al);

  return val;
}

template <>
cryptonote::rpc::output_amount_count fromJsonValue<cryptonote::rpc::output_amount_count>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::rpc::output_amount_count oac;

  OBJECT_HAS_MEMBER_OR_THROW(val, "amount")
  oac.amount = fromJsonValue<decltype(oac.amount)>(val["amount"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "total_count")
  oac.total_count = fromJsonValue<decltype(oac.total_count)>(val["total_count"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "unlocked_count")
  oac.unlocked_count = fromJsonValue<decltype(oac.unlocked_count)>(val["unlocked_count"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "recent_count")
  oac.recent_count = fromJsonValue<decltype(oac.recent_count)>(val["recent_count"]);


  return oac;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::output_amount_and_index& out)
{
  rapidjson::Value val;

  val.SetObject();

  auto& al = doc.GetAllocator();

  val.AddMember("amount", toJsonValue(doc, out.amount), al);
  val.AddMember("index", toJsonValue(doc, out.index), al);

  return val;
}

template <>
cryptonote::rpc::output_amount_and_index fromJsonValue<cryptonote::rpc::output_amount_and_index>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::rpc::output_amount_and_index out;

  OBJECT_HAS_MEMBER_OR_THROW(val, "amount")
  out.amount = fromJsonValue<decltype(out.amount)>(val["amount"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "index")
  out.index = fromJsonValue<decltype(out.index)>(val["index"]);


  return out;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::output_key_mask_unlocked& out)
{
  rapidjson::Value val;

  val.SetObject();

  auto& al = doc.GetAllocator();

  val.AddMember("key", toJsonValue(doc, out.key), al);
  val.AddMember("mask", toJsonValue(doc, out.mask), al);
  val.AddMember("unlocked", toJsonValue(doc, out.unlocked), al);

  return val;
}

template <>
cryptonote::rpc::output_key_mask_unlocked fromJsonValue<cryptonote::rpc::output_key_mask_unlocked>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::rpc::output_key_mask_unlocked out;

  OBJECT_HAS_MEMBER_OR_THROW(val, "key")
  out.key = fromJsonValue<decltype(out.key)>(val["key"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "mask")
  out.mask = fromJsonValue<decltype(out.mask)>(val["mask"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "unlocked")
  out.unlocked = fromJsonValue<decltype(out.unlocked)>(val["unlocked"]);


  return out;
}

template <>
cryptonote::rpc::hard_fork_info fromJsonValue<cryptonote::rpc::hard_fork_info>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::rpc::hard_fork_info info;

  OBJECT_HAS_MEMBER_OR_THROW(val, "version")
  info.version = fromJsonValue<decltype(info.version)>(val["version"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "enabled")
  info.enabled = fromJsonValue<decltype(info.enabled)>(val["enabled"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "window")
  info.window = fromJsonValue<decltype(info.window)>(val["window"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "votes")
  info.votes = fromJsonValue<decltype(info.votes)>(val["votes"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "threshold")
  info.threshold = fromJsonValue<decltype(info.threshold)>(val["threshold"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "voting")
  info.voting = fromJsonValue<decltype(info.voting)>(val["voting"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "state")
  info.state = fromJsonValue<decltype(info.state)>(val["state"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "earliest_height")
  info.earliest_height = fromJsonValue<decltype(info.earliest_height)>(val["earliest_height"]);

  return info;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::hard_fork_info& info)
{
  rapidjson::Value val;

  val.SetObject();

  auto& al = doc.GetAllocator();

  val.AddMember("version", toJsonValue(doc, info.version), al);
  val.AddMember("enabled", toJsonValue(doc, info.enabled), al);
  val.AddMember("window", toJsonValue(doc, info.window), al);
  val.AddMember("votes", toJsonValue(doc, info.votes), al);
  val.AddMember("threshold", toJsonValue(doc, info.threshold), al);
  val.AddMember("voting", toJsonValue(doc, info.voting), al);
  val.AddMember("state", toJsonValue(doc, info.state), al);
  val.AddMember("earliest_height", toJsonValue(doc, info.earliest_height), al);

  return val;
}

template <>
cryptonote::rpc::error fromJsonValue<cryptonote::rpc::error>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::rpc::error err;

  OBJECT_HAS_MEMBER_OR_THROW(val, "code")
  err.code = fromJsonValue<decltype(err.code)>(val["code"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "error_str")
  err.error_str = fromJsonValue<decltype(err.error_str)>(val["error_str"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "message")
  err.message = fromJsonValue<decltype(err.message)>(val["message"]);

  return err;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::error& err)
{
  rapidjson::Value val;

  val.SetObject();

  auto& al = doc.GetAllocator();

  val.AddMember("code", toJsonValue(doc, err.code), al);
  val.AddMember("error_str", toJsonValue(doc, err.error_str), al);
  val.AddMember("message", toJsonValue(doc, err.message), al);

  return val;
}

template <>
cryptonote::rpc::BlockHeaderResponse fromJsonValue<cryptonote::rpc::BlockHeaderResponse>(const rapidjson::Value& val)
{
  if (!val.IsObject())
  {
    throw WRONG_TYPE("json object");
  }

  cryptonote::rpc::BlockHeaderResponse response;

  OBJECT_HAS_MEMBER_OR_THROW(val, "major_version")
  response.major_version = fromJsonValue<decltype(response.major_version)>(val["major_version"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "minor_version")
  response.minor_version = fromJsonValue<decltype(response.minor_version)>(val["minor_version"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "timestamp")
  response.timestamp = fromJsonValue<decltype(response.timestamp)>(val["timestamp"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "prev_id")
  response.prev_id = fromJsonValue<decltype(response.prev_id)>(val["prev_id"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "nonce")
  response.nonce = fromJsonValue<decltype(response.nonce)>(val["nonce"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "height")
  response.height = fromJsonValue<decltype(response.height)>(val["height"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "depth")
  response.depth = fromJsonValue<decltype(response.depth)>(val["depth"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "hash")
  response.hash = fromJsonValue<decltype(response.hash)>(val["hash"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "difficulty")
  response.difficulty = fromJsonValue<decltype(response.difficulty)>(val["difficulty"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "reward")
  response.reward = fromJsonValue<decltype(response.reward)>(val["reward"]);

  return response;
}

rapidjson::Value toJsonValue(rapidjson::Document& doc, const cryptonote::rpc::BlockHeaderResponse& response)
{
  rapidjson::Value val;

  val.SetObject();

  auto& al = doc.GetAllocator();

  val.AddMember("major_version", toJsonValue(doc, response.major_version), al);
  val.AddMember("minor_version", toJsonValue(doc, response.minor_version), al);
  val.AddMember("timestamp", toJsonValue(doc, response.timestamp), al);
  val.AddMember("prev_id", toJsonValue(doc, response.prev_id), al);
  val.AddMember("nonce", toJsonValue(doc, response.nonce), al);
  val.AddMember("height", toJsonValue(doc, response.height), al);
  val.AddMember("depth", toJsonValue(doc, response.depth), al);
  val.AddMember("hash", toJsonValue(doc, response.hash), al);
  val.AddMember("difficulty", toJsonValue(doc, response.difficulty), al);
  val.AddMember("reward", toJsonValue(doc, response.reward), al);

  return val;
}

}  // namespace json

}  // namespace cryptonote
