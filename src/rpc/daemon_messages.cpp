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

#include "daemon_messages.h"
#include "serialization/json_object.h"

namespace cryptonote
{

namespace rpc
{

const char* GetHeight::name = "get_height";
const char* GetBlocksFast::name = "get_blocks_fast";
const char* GetHashesFast::name = "get_hashes_fast";
const char* GetTransactions::name = "get_transactions";
const char* KeyImagesSpent::name = "key_images_spent";
const char* GetTxGlobalOutputIndices::name = "get_tx_global_output_indices";
const char* GetRandomOutputsForAmounts::name = "get_random_outputs_for_amounts";
const char* SendRawTx::name = "send_raw_tx";
const char* GetInfo::name = "get_info";
const char* SaveBC::name = "save_bc";
const char* GetBlockHash::name = "get_block_hash";
const char* GetLastBlockHeader::name = "get_last_block_header";
const char* GetBlockHeaderByHash::name = "get_block_header_by_hash";
const char* GetBlockHeaderByHeight::name = "get_block_header_by_height";




rapidjson::Value GetHeight::Request::toJson(rapidjson::Document& doc)
{
  return Message::toJson(doc);
}

void GetHeight::Request::fromJson(rapidjson::Value& val)
{
}

rapidjson::Value GetHeight::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("height", height, al);

  return val;
}

void GetHeight::Response::fromJson(rapidjson::Value& val)
{
  height = cryptonote::json::fromJsonValue<uint64_t>(val["height"]);
}


rapidjson::Value GetBlocksFast::Request::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("block_ids", cryptonote::json::toJsonValue<decltype(block_ids)>(doc, block_ids), al);
  val.AddMember("start_height", start_height, al);

  return val;
}

void GetBlocksFast::Request::fromJson(rapidjson::Value& val)
{
  block_ids = cryptonote::json::fromJsonValue<decltype(block_ids)>(val["block_ids"]);
  start_height = cryptonote::json::fromJsonValue<uint64_t>(val["start_height"]);
}

rapidjson::Value GetBlocksFast::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("blocks", cryptonote::json::toJsonValue<decltype(blocks)>(doc, blocks), al);
  val.AddMember("start_height", start_height, al);
  val.AddMember("current_height", current_height, al);

  return val;
}

void GetBlocksFast::Response::fromJson(rapidjson::Value& val)
{
  blocks = cryptonote::json::fromJsonValue<decltype(blocks)>(val["blocks"]);
  start_height = cryptonote::json::fromJsonValue<uint64_t>(val["start_height"]);
  current_height = cryptonote::json::fromJsonValue<uint64_t>(val["current_height"]);
}


rapidjson::Value GetHashesFast::Request::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("known_hashes", cryptonote::json::toJsonValue<decltype(known_hashes)>(doc, known_hashes), al);
  val.AddMember("start_height", start_height, al);

  return val;
}

void GetHashesFast::Request::fromJson(rapidjson::Value& val)
{
  known_hashes = cryptonote::json::fromJsonValue<decltype(known_hashes)>(val["known_hashes"]);
  start_height = cryptonote::json::fromJsonValue<uint64_t>(val["start_height"]);
}

rapidjson::Value GetHashesFast::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("hashes", cryptonote::json::toJsonValue<decltype(hashes)>(doc, hashes), al);
  val.AddMember("start_height", start_height, al);
  val.AddMember("current_height", current_height, al);

  return val;
}

void GetHashesFast::Response::fromJson(rapidjson::Value& val)
{
  hashes = cryptonote::json::fromJsonValue<decltype(hashes)>(val["hashes"]);
  start_height = cryptonote::json::fromJsonValue<uint64_t>(val["start_height"]);
  current_height = cryptonote::json::fromJsonValue<uint64_t>(val["current_height"]);
}


rapidjson::Value GetTransactions::Request::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("tx_hashes", cryptonote::json::toJsonValue<decltype(tx_hashes)>(doc, tx_hashes), al);

  return val;
}

void GetTransactions::Request::fromJson(rapidjson::Value& val)
{
  tx_hashes = cryptonote::json::fromJsonValue<decltype(tx_hashes)>(val["tx_hashes"]);
}

rapidjson::Value GetTransactions::Response::toJson(rapidjson::Document& doc)
{
  rapidjson::Value val(rapidjson::kObjectType);

  auto& al = doc.GetAllocator();

  val.AddMember("txs",
                cryptonote::json::toJsonValue<decltype(txs)>(doc, txs),
                al);
  val.AddMember("missed_hashes",
                cryptonote::json::toJsonValue<decltype(missed_hashes)>(doc, missed_hashes),
                al);

  return val;
}

void GetTransactions::Response::fromJson(rapidjson::Value& val)
{
  txs = cryptonote::json::fromJsonValue<decltype(txs)>(val["txs"]);
  missed_hashes = cryptonote::json::fromJsonValue<decltype(missed_hashes)>(val["missed_hashes"]);
}


rapidjson::Value KeyImagesSpent::Request::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("key_images", cryptonote::json::toJsonValue<decltype(key_images)>(doc, key_images), al);

  return val;
}

void KeyImagesSpent::Request::fromJson(rapidjson::Value& val)
{
  key_images = cryptonote::json::fromJsonValue<decltype(key_images)>(val["key_images"]);
}

rapidjson::Value KeyImagesSpent::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("spent_status", cryptonote::json::toJsonValue<decltype(spent_status)>(doc, spent_status), al);

  return val;
}

void KeyImagesSpent::Response::fromJson(rapidjson::Value& val)
{
  spent_status = cryptonote::json::fromJsonValue<decltype(spent_status)>(val["spent_status"]);
}


rapidjson::Value GetTxGlobalOutputIndices::Request::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("tx_hash", cryptonote::json::toJsonValue<decltype(tx_hash)>(doc, tx_hash), al);

  return val;
}

void GetTxGlobalOutputIndices::Request::fromJson(rapidjson::Value& val)
{
  tx_hash = cryptonote::json::fromJsonValue<decltype(tx_hash)>(val["tx_hash"]);
}

rapidjson::Value GetTxGlobalOutputIndices::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("output_indices", cryptonote::json::toJsonValue<decltype(output_indices)>(doc, output_indices), al);

  return val;
}

void GetTxGlobalOutputIndices::Response::fromJson(rapidjson::Value& val)
{
  output_indices = cryptonote::json::fromJsonValue<decltype(output_indices)>(val["output_indices"]);
}


rapidjson::Value GetRandomOutputsForAmounts::Request::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("amounts", cryptonote::json::toJsonValue<decltype(amounts)>(doc, amounts), al);
  val.AddMember("count", cryptonote::json::toJsonValue<decltype(count)>(doc, count), al);

  return val;
}

void GetRandomOutputsForAmounts::Request::fromJson(rapidjson::Value& val)
{
  amounts = cryptonote::json::fromJsonValue<decltype(amounts)>(val["amounts"]);
  count = cryptonote::json::fromJsonValue<decltype(count)>(val["count"]);
}

rapidjson::Value GetRandomOutputsForAmounts::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("amounts_with_outputs", cryptonote::json::toJsonValue<decltype(amounts_with_outputs)>(doc, amounts_with_outputs), al);

  return val;
}

void GetRandomOutputsForAmounts::Response::fromJson(rapidjson::Value& val)
{
  amounts_with_outputs = cryptonote::json::fromJsonValue<decltype(amounts_with_outputs)>(val["amounts_with_outputs"]);
}


rapidjson::Value SendRawTx::Request::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("tx", cryptonote::json::toJsonValue<decltype(tx)>(doc, tx), al);
  val.AddMember("do_not_relay", cryptonote::json::toJsonValue<decltype(do_not_relay)>(doc, do_not_relay), al);

  return val;
}

void SendRawTx::Request::fromJson(rapidjson::Value& val)
{
  tx = cryptonote::json::fromJsonValue<decltype(tx)>(val["tx"]);
  do_not_relay = cryptonote::json::fromJsonValue<decltype(do_not_relay)>(val["do_not_relay"]);
}

rapidjson::Value SendRawTx::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("relayed", cryptonote::json::toJsonValue<decltype(relayed)>(doc, relayed), al);

  return val;
}

void SendRawTx::Response::fromJson(rapidjson::Value& val)
{
  relayed = cryptonote::json::fromJsonValue<decltype(relayed)>(val["relayed"]);
}


rapidjson::Value GetInfo::Request::toJson(rapidjson::Document& doc)
{
  return Message::toJson(doc);
}

void GetInfo::Request::fromJson(rapidjson::Value& val)
{
}

rapidjson::Value GetInfo::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("height", cryptonote::json::toJsonValue<decltype(height)>(doc, height), al);
  val.AddMember("target_height", cryptonote::json::toJsonValue<decltype(target_height)>(doc, target_height), al);
  val.AddMember("difficulty", cryptonote::json::toJsonValue<decltype(difficulty)>(doc, difficulty), al);
  val.AddMember("target", cryptonote::json::toJsonValue<decltype(target)>(doc, target), al);
  val.AddMember("tx_count", cryptonote::json::toJsonValue<decltype(tx_count)>(doc, tx_count), al);
  val.AddMember("tx_pool_size", cryptonote::json::toJsonValue<decltype(tx_pool_size)>(doc, tx_pool_size), al);
  val.AddMember("alt_blocks_count", cryptonote::json::toJsonValue<decltype(alt_blocks_count)>(doc, alt_blocks_count), al);
  val.AddMember("outgoing_connections_count", cryptonote::json::toJsonValue<decltype(outgoing_connections_count)>(doc, outgoing_connections_count), al);
  val.AddMember("incoming_connections_count", cryptonote::json::toJsonValue<decltype(incoming_connections_count)>(doc, incoming_connections_count), al);
  val.AddMember("white_peerlist_size", cryptonote::json::toJsonValue<decltype(white_peerlist_size)>(doc, white_peerlist_size), al);
  val.AddMember("grey_peerlist_size", cryptonote::json::toJsonValue<decltype(grey_peerlist_size)>(doc, grey_peerlist_size), al);
  val.AddMember("testnet", cryptonote::json::toJsonValue<decltype(testnet)>(doc, testnet), al);
  val.AddMember("top_block_hash", cryptonote::json::toJsonValue<decltype(top_block_hash)>(doc, top_block_hash), al);

  return val;
}

void GetInfo::Response::fromJson(rapidjson::Value& val)
{
  height = cryptonote::json::fromJsonValue<decltype(height)>(val["height"]);
  target_height = cryptonote::json::fromJsonValue<decltype(target_height)>(val["target_height"]);
  difficulty = cryptonote::json::fromJsonValue<decltype(difficulty)>(val["difficulty"]);
  target = cryptonote::json::fromJsonValue<decltype(target)>(val["target"]);
  tx_count = cryptonote::json::fromJsonValue<decltype(tx_count)>(val["tx_count"]);
  tx_pool_size = cryptonote::json::fromJsonValue<decltype(tx_pool_size)>(val["tx_pool_size"]);
  alt_blocks_count = cryptonote::json::fromJsonValue<decltype(alt_blocks_count)>(val["alt_blocks_count"]);
  outgoing_connections_count = cryptonote::json::fromJsonValue<decltype(outgoing_connections_count)>(val["outgoing_connections_count"]);
  incoming_connections_count = cryptonote::json::fromJsonValue<decltype(incoming_connections_count)>(val["incoming_connections_count"]);
  white_peerlist_size = cryptonote::json::fromJsonValue<decltype(white_peerlist_size)>(val["white_peerlist_size"]);
  grey_peerlist_size = cryptonote::json::fromJsonValue<decltype(grey_peerlist_size)>(val["grey_peerlist_size"]);
  testnet = cryptonote::json::fromJsonValue<decltype(testnet)>(val["testnet"]);
  top_block_hash = cryptonote::json::fromJsonValue<decltype(top_block_hash)>(val["top_block_hash"]);
}


rapidjson::Value SaveBC::Request::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  return val;
}

void SaveBC::Request::fromJson(rapidjson::Value& val)
{
}

rapidjson::Value SaveBC::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  return val;
}

void SaveBC::Response::fromJson(rapidjson::Value& val)
{
}


rapidjson::Value GetBlockHash::Request::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("height", cryptonote::json::toJsonValue<decltype(height)>(doc, height), al);

  return val;
}

void GetBlockHash::Request::fromJson(rapidjson::Value& val)
{
  height = cryptonote::json::fromJsonValue<decltype(height)>(val["height"]);
}

rapidjson::Value GetBlockHash::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("hash", cryptonote::json::toJsonValue<decltype(hash)>(doc, hash), al);

  return val;
}

void GetBlockHash::Response::fromJson(rapidjson::Value& val)
{
  hash = cryptonote::json::fromJsonValue<decltype(hash)>(val["hash"]);
}


rapidjson::Value GetLastBlockHeader::Request::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  return val;
}

void GetLastBlockHeader::Request::fromJson(rapidjson::Value& val)
{
}

rapidjson::Value GetLastBlockHeader::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("major_version", cryptonote::json::toJsonValue<decltype(major_version)>(doc, major_version), al);
  val.AddMember("minor_version", cryptonote::json::toJsonValue<decltype(minor_version)>(doc, minor_version), al);
  val.AddMember("timestamp", cryptonote::json::toJsonValue<decltype(timestamp)>(doc, timestamp), al);
  val.AddMember("prev_id", cryptonote::json::toJsonValue<decltype(prev_id)>(doc, prev_id), al);
  val.AddMember("nonce", cryptonote::json::toJsonValue<decltype(nonce)>(doc, nonce), al);
  val.AddMember("height", cryptonote::json::toJsonValue<decltype(height)>(doc, height), al);
  val.AddMember("hash", cryptonote::json::toJsonValue<decltype(hash)>(doc, hash), al);
  val.AddMember("difficulty", cryptonote::json::toJsonValue<decltype(difficulty)>(doc, difficulty), al);
  val.AddMember("reward", cryptonote::json::toJsonValue<decltype(reward)>(doc, reward), al);

  return val;
}

void GetLastBlockHeader::Response::fromJson(rapidjson::Value& val)
{
  major_version = cryptonote::json::fromJsonValue<decltype(major_version)>(val["major_version"]);
  minor_version = cryptonote::json::fromJsonValue<decltype(minor_version)>(val["minor_version"]);
  timestamp = cryptonote::json::fromJsonValue<decltype(timestamp)>(val["timestamp"]);
  prev_id = cryptonote::json::fromJsonValue<decltype(prev_id)>(val["prev_id"]);
  nonce = cryptonote::json::fromJsonValue<decltype(nonce)>(val["nonce"]);
  height = cryptonote::json::fromJsonValue<decltype(height)>(val["height"]);
  hash = cryptonote::json::fromJsonValue<decltype(hash)>(val["hash"]);
  difficulty = cryptonote::json::fromJsonValue<decltype(difficulty)>(val["difficulty"]);
  reward = cryptonote::json::fromJsonValue<decltype(reward)>(val["reward"]);
}


rapidjson::Value GetBlockHeaderByHash::Request::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("hash", cryptonote::json::toJsonValue<decltype(hash)>(doc, hash), al);

  return val;
}

void GetBlockHeaderByHash::Request::fromJson(rapidjson::Value& val)
{
  hash = cryptonote::json::fromJsonValue<decltype(hash)>(val["hash"]);
}

rapidjson::Value GetBlockHeaderByHash::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("major_version", cryptonote::json::toJsonValue<decltype(major_version)>(doc, major_version), al);
  val.AddMember("minor_version", cryptonote::json::toJsonValue<decltype(minor_version)>(doc, minor_version), al);
  val.AddMember("timestamp", cryptonote::json::toJsonValue<decltype(timestamp)>(doc, timestamp), al);
  val.AddMember("prev_id", cryptonote::json::toJsonValue<decltype(prev_id)>(doc, prev_id), al);
  val.AddMember("nonce", cryptonote::json::toJsonValue<decltype(nonce)>(doc, nonce), al);
  val.AddMember("height", cryptonote::json::toJsonValue<decltype(height)>(doc, height), al);
  val.AddMember("depth", cryptonote::json::toJsonValue<decltype(depth)>(doc, depth), al);
  val.AddMember("hash", cryptonote::json::toJsonValue<decltype(hash)>(doc, hash), al);
  val.AddMember("difficulty", cryptonote::json::toJsonValue<decltype(difficulty)>(doc, difficulty), al);
  val.AddMember("reward", cryptonote::json::toJsonValue<decltype(reward)>(doc, reward), al);

  return val;
}

void GetBlockHeaderByHash::Response::fromJson(rapidjson::Value& val)
{
  major_version = cryptonote::json::fromJsonValue<decltype(major_version)>(val["major_version"]);
  minor_version = cryptonote::json::fromJsonValue<decltype(minor_version)>(val["minor_version"]);
  timestamp = cryptonote::json::fromJsonValue<decltype(timestamp)>(val["timestamp"]);
  prev_id = cryptonote::json::fromJsonValue<decltype(prev_id)>(val["prev_id"]);
  nonce = cryptonote::json::fromJsonValue<decltype(nonce)>(val["nonce"]);
  height = cryptonote::json::fromJsonValue<decltype(height)>(val["height"]);
  depth = cryptonote::json::fromJsonValue<decltype(depth)>(val["depth"]);
  hash = cryptonote::json::fromJsonValue<decltype(hash)>(val["hash"]);
  difficulty = cryptonote::json::fromJsonValue<decltype(difficulty)>(val["difficulty"]);
  reward = cryptonote::json::fromJsonValue<decltype(reward)>(val["reward"]);
}


rapidjson::Value GetBlockHeaderByHeight::Request::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("height", cryptonote::json::toJsonValue<decltype(height)>(doc, height), al);

  return val;
}

void GetBlockHeaderByHeight::Request::fromJson(rapidjson::Value& val)
{
  height = cryptonote::json::fromJsonValue<decltype(height)>(val["height"]);
}

rapidjson::Value GetBlockHeaderByHeight::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("major_version", cryptonote::json::toJsonValue<decltype(major_version)>(doc, major_version), al);
  val.AddMember("minor_version", cryptonote::json::toJsonValue<decltype(minor_version)>(doc, minor_version), al);
  val.AddMember("timestamp", cryptonote::json::toJsonValue<decltype(timestamp)>(doc, timestamp), al);
  val.AddMember("prev_id", cryptonote::json::toJsonValue<decltype(prev_id)>(doc, prev_id), al);
  val.AddMember("nonce", cryptonote::json::toJsonValue<decltype(nonce)>(doc, nonce), al);
  val.AddMember("height", cryptonote::json::toJsonValue<decltype(height)>(doc, height), al);
  val.AddMember("depth", cryptonote::json::toJsonValue<decltype(depth)>(doc, depth), al);
  val.AddMember("hash", cryptonote::json::toJsonValue<decltype(hash)>(doc, hash), al);
  val.AddMember("difficulty", cryptonote::json::toJsonValue<decltype(difficulty)>(doc, difficulty), al);
  val.AddMember("reward", cryptonote::json::toJsonValue<decltype(reward)>(doc, reward), al);

  return val;
}

void GetBlockHeaderByHeight::Response::fromJson(rapidjson::Value& val)
{
  major_version = cryptonote::json::fromJsonValue<decltype(major_version)>(val["major_version"]);
  minor_version = cryptonote::json::fromJsonValue<decltype(minor_version)>(val["minor_version"]);
  timestamp = cryptonote::json::fromJsonValue<decltype(timestamp)>(val["timestamp"]);
  prev_id = cryptonote::json::fromJsonValue<decltype(prev_id)>(val["prev_id"]);
  nonce = cryptonote::json::fromJsonValue<decltype(nonce)>(val["nonce"]);
  height = cryptonote::json::fromJsonValue<decltype(height)>(val["height"]);
  depth = cryptonote::json::fromJsonValue<decltype(depth)>(val["depth"]);
  hash = cryptonote::json::fromJsonValue<decltype(hash)>(val["hash"]);
  difficulty = cryptonote::json::fromJsonValue<decltype(difficulty)>(val["difficulty"]);
  reward = cryptonote::json::fromJsonValue<decltype(reward)>(val["reward"]);
}


}  // namespace rpc

}  // namespace cryptonote
