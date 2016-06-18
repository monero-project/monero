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


template <class Type>
rapidjson::Value toJsonValue(rapidjson::Document& doc, const Type& pod)
{
  return rapidjson::Value(epee::string_tools::pod_to_hex(pod).c_str(), doc.GetAllocator());
}

template <class Type>
Type fromJsonValue(const rapidjson::Value& val)
{
  Type retval;

  //TODO: handle failure to convert hex string to POD type
  bool success = epee::string_tools::hex_to_pod(val.GetString(), retval);

  return retval;
}

template <>
rapidjson::Value toJsonValue(rapidjson::Document& doc, const std::string& i)
{
  return rapidjson::Value(i.c_str(), doc.GetAllocator());
}

template <>
std::string fromJsonValue(const rapidjson::Value& i)
{
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
  return i.GetBool();
}

template <>
rapidjson::Value toJsonValue(rapidjson::Document& doc, const uint8_t& i)
{
  return rapidjson::Value(i);
}

template <>
uint8_t fromJsonValue(const rapidjson::Value& i)
{
  return (uint8_t) ( i.GetUint() & 0xFF);
}

template <>
rapidjson::Value toJsonValue(rapidjson::Document& doc, const uint16_t& i)
{
  return rapidjson::Value(i);
}

template <>
uint16_t fromJsonValue(const rapidjson::Value& i)
{
  return (uint16_t) ( i.GetUint() & 0xFFFF);
}

template <>
rapidjson::Value toJsonValue(rapidjson::Document& doc, const uint32_t& i)
{
  return rapidjson::Value(i);
}

template <>
uint32_t fromJsonValue(const rapidjson::Value& i)
{
  return i.GetUint();
}

template <>
rapidjson::Value toJsonValue(rapidjson::Document& doc, const uint64_t& i)
{
  return rapidjson::Value(i);
}

template <>
uint64_t fromJsonValue(const rapidjson::Value& i)
{
  return i.GetUint64();
}

template <>
rapidjson::Value toJsonValue<cryptonote::transaction>(rapidjson::Document& doc, const cryptonote::transaction& tx)
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
  cryptonote::transaction tx;

  tx.version = fromJsonValue<size_t>(val["version"]);
  tx.unlock_time = fromJsonValue<uint64_t>(val["unlock_time"]);
  tx.vin = fromJsonValue<cryptonote::txin_v, std::vector >(val["vin"]);
  tx.vout = fromJsonValue<cryptonote::tx_out, std::vector >(val["vout"]);
  tx.extra = fromJsonValue<uint8_t, std::vector>(val["extra"]);
  tx.signatures = fromJsonValue<crypto::signature, std::vector, std::vector>(val["signatures"]);

  return tx;
}

template <>
rapidjson::Value toJsonValue<cryptonote::block>(rapidjson::Document& doc, const cryptonote::block& b)
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
  cryptonote::block b;

  b.major_version = fromJsonValue<uint8_t>(val["major_version"]);
  b.minor_version = fromJsonValue<uint8_t>(val["minor_version"]);
  b.timestamp = fromJsonValue<uint64_t>(val["timestamp"]);
  b.prev_id = fromJsonValue<crypto::hash>(val["prev_id"]);
  b.nonce = fromJsonValue<uint32_t>(val["nonce"]);
  b.miner_tx = fromJsonValue<transaction>(val["miner_tx"]);
  b.tx_hashes = fromJsonValue<crypto::hash, std::vector>(val["tx_hashes"]);

  return b;
}

template <>
rapidjson::Value toJsonValue<cryptonote::txin_v>(rapidjson::Document& doc, const cryptonote::txin_v& txin)
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
  cryptonote::txin_v txin;

  if (val["type"]== "txin_gen")
  {
    txin = fromJsonValue<cryptonote::txin_gen>(val["value"]);
  }
  else if (val["type"]== "txin_to_script")
  {
    txin = fromJsonValue<cryptonote::txin_to_script>(val["value"]);
  }
  else if (val["type"] == "txin_to_scripthash")
  {
    txin = fromJsonValue<cryptonote::txin_to_scripthash>(val["value"]);
  }
  else if (val["type"] == "txin_to_key")
  {
    txin = fromJsonValue<cryptonote::txin_to_key>(val["value"]);
  }

  return txin;
}

template <>
rapidjson::Value toJsonValue<cryptonote::txin_gen>(rapidjson::Document& doc, const cryptonote::txin_gen& txin)
{
  rapidjson::Value val;

  val.SetObject();

  val.AddMember("height", toJsonValue(doc, txin.height), doc.GetAllocator());

  return val;
}

template <>
cryptonote::txin_gen fromJsonValue<cryptonote::txin_gen>(const rapidjson::Value& val)
{
  cryptonote::txin_gen txin;

  txin.height = fromJsonValue<size_t>(val["height"]);

  return txin;
}

template <>
rapidjson::Value toJsonValue<cryptonote::txin_to_script>(rapidjson::Document& doc, const cryptonote::txin_to_script& txin)
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
  cryptonote::txin_to_script txin;

  txin.prev = fromJsonValue<crypto::hash>(val["prev"]);
  txin.prevout = fromJsonValue<size_t>(val["prevout"]);
  txin.sigset = fromJsonValue<uint8_t, std::vector>(val["sigset"]);

  return txin;
}

template <>
rapidjson::Value toJsonValue<cryptonote::txin_to_scripthash>(rapidjson::Document& doc, const cryptonote::txin_to_scripthash& txin)
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
  cryptonote::txin_to_scripthash txin;

  txin.prev = fromJsonValue<crypto::hash>(val["prev"]);
  txin.prevout = fromJsonValue<size_t>(val["prevout"]);
  txin.script = fromJsonValue<txout_to_script>(val["script"]);
  txin.sigset = fromJsonValue<uint8_t, std::vector>(val["sigset"]);

  return txin;
}

template <>
rapidjson::Value toJsonValue<cryptonote::txin_to_key>(rapidjson::Document& doc, const cryptonote::txin_to_key& txin)
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
  cryptonote::txin_to_key txin;

  txin.amount = fromJsonValue<uint64_t>(val["amount"]);
  txin.key_offsets = fromJsonValue<uint64_t, std::vector>(val["key_offsets"]);
  txin.k_image = fromJsonValue<crypto::key_image>(val["k_image"]);

  return txin;
}

template <>
rapidjson::Value toJsonValue<cryptonote::txout_target_v>(rapidjson::Document& doc, const cryptonote::txout_target_v& txout)
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
  cryptonote::txout_target_v txout;

  if (val["type"]== "txout_to_script")
  {
    txout = fromJsonValue<cryptonote::txout_to_script>(val["value"]);
  }
  else if (val["type"] == "txout_to_scripthash")
  {
    txout = fromJsonValue<cryptonote::txout_to_scripthash>(val["value"]);
  }
  else if (val["type"] == "txout_to_key")
  {
    txout = fromJsonValue<cryptonote::txout_to_key>(val["value"]);
  }

  return txout;
}

template <>
rapidjson::Value toJsonValue<cryptonote::txout_to_script>(rapidjson::Document& doc, const cryptonote::txout_to_script& txout)
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
  cryptonote::txout_to_script txout;

  txout.keys = fromJsonValue<crypto::public_key, std::vector>(val["keys"]);
  txout.script = fromJsonValue<uint8_t, std::vector>(val["script"]);

  return txout;
}

template <>
rapidjson::Value toJsonValue<cryptonote::txout_to_scripthash>(rapidjson::Document& doc, const cryptonote::txout_to_scripthash& txout)
{
  rapidjson::Value val;

  val.SetObject();

  val.AddMember("hash", toJsonValue(doc, txout.hash), doc.GetAllocator());

  return val;
}

template <>
cryptonote::txout_to_scripthash fromJsonValue<cryptonote::txout_to_scripthash>(const rapidjson::Value& val)
{
  cryptonote::txout_to_scripthash txout;

  txout.hash = fromJsonValue<crypto::hash>(val["hash"]);

  return txout;
}

template <>
rapidjson::Value toJsonValue<cryptonote::txout_to_key>(rapidjson::Document& doc, const cryptonote::txout_to_key& txout)
{
  rapidjson::Value val;

  val.SetObject();

  val.AddMember("key", toJsonValue(doc, txout.key), doc.GetAllocator());

  return val;
}

template <>
cryptonote::txout_to_key fromJsonValue<cryptonote::txout_to_key>(const rapidjson::Value& val)
{
  cryptonote::txout_to_key txout;

  txout.key = fromJsonValue<crypto::public_key>(val["key"]);

  return txout;
}

template <>
rapidjson::Value toJsonValue<cryptonote::tx_out>(rapidjson::Document& doc, const cryptonote::tx_out& txout)
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
  cryptonote::tx_out txout;

  txout.amount = fromJsonValue<uint64_t>(val["amount"]);
  txout.target = fromJsonValue<txout_target_v>(val["target"]);

  return txout;
}

template <>
rapidjson::Value toJsonValue<cryptonote::connection_info>(rapidjson::Document& doc, const cryptonote::connection_info& info)
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
  cryptonote::connection_info info;

  info.incoming = fromJsonValue<bool>(val["incoming"]);
  info.localhost = fromJsonValue<bool>(val["localhost"]);
  info.local_ip = fromJsonValue<bool>(val["local_ip"]);

  info.ip = fromJsonValue<std::string>(val["ip"]);
  info.port = fromJsonValue<std::string>(val["port"]);

  info.peer_id = fromJsonValue<std::string>(val["peer_id"]);

  info.recv_count = fromJsonValue<uint64_t>(val["recv_count"]);
  info.recv_idle_time = fromJsonValue<uint64_t>(val["recv_idle_time"]);

  info.send_count = fromJsonValue<uint64_t>(val["send_count"]);
  info.send_idle_time = fromJsonValue<uint64_t>(val["send_idle_time"]);

  info.state = fromJsonValue<std::string>(val["state"]);

  info.live_time = fromJsonValue<uint64_t>(val["live_time"]);

  info.avg_download = fromJsonValue<uint64_t>(val["avg_download"]);
  info.current_download = fromJsonValue<uint64_t>(val["current_download"]);

  info.avg_upload = fromJsonValue<uint64_t>(val["avg_upload"]);
  info.current_upload = fromJsonValue<uint64_t>(val["current_upload"]);

  return info;
}

template <>
rapidjson::Value toJsonValue<cryptonote::block_complete_entry>(rapidjson::Document& doc, const cryptonote::block_complete_entry& blk)
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
  cryptonote::block_complete_entry blk;

  blk.block = fromJsonValue<blobdata>(val["block"]);
  blk.txs = fromJsonValue<blobdata, std::list>(val["txs"]);

  return blk;
}

template <>
rapidjson::Value toJsonValue<cryptonote::block_with_transactions>(rapidjson::Document& doc, const cryptonote::block_with_transactions& blk)
{
  rapidjson::Value val;

  val.SetObject();

  val.AddMember("block", toJsonValue(doc, blk.block), doc.GetAllocator());
  val.AddMember("transactions", toJsonValue<crypto::hash, cryptonote::transaction>
                                (doc, blk.transactions), doc.GetAllocator());

  return val;
}

template <>
cryptonote::block_with_transactions fromJsonValue<cryptonote::block_with_transactions>(const rapidjson::Value& val)
{
  cryptonote::block_with_transactions blk;

  blk.block = fromJsonValue<block>(val["block"]);
  blk.transactions = fromJsonValue<crypto::hash,
                                   cryptonote::transaction,
                                   std::unordered_map<crypto::hash, cryptonote::transaction>
                                  >(val["transactions"]);

  return blk;
}

template <>
rapidjson::Value toJsonValue<cryptonote::transaction_info>(rapidjson::Document& doc, const cryptonote::transaction_info& tx_info)
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
cryptonote::transaction_info fromJsonValue<cryptonote::transaction_info>(const rapidjson::Value& val)
{
  cryptonote::transaction_info tx_info;

  tx_info.height = fromJsonValue<uint64_t>(val["height"]);
  tx_info.in_pool = fromJsonValue<bool>(val["in_pool"]);
  tx_info.transaction = fromJsonValue<cryptonote::transaction>(val["transaction"]);

  return tx_info;
}

}  // namespace json

}  // namespace cryptonote
