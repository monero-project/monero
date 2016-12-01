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

const char* const GetHeight::name = "get_height";
const char* const GetBlocksFast::name = "get_blocks_fast";
const char* const GetHashesFast::name = "get_hashes_fast";
const char* const GetTransactions::name = "get_transactions";
const char* const KeyImagesSpent::name = "key_images_spent";
const char* const GetTxGlobalOutputIndices::name = "get_tx_global_output_indices";
const char* const GetRandomOutputsForAmounts::name = "get_random_outputs_for_amounts";
const char* const SendRawTx::name = "send_raw_tx";
const char* const GetInfo::name = "get_info";
const char* const SaveBC::name = "save_bc";
const char* const GetBlockHash::name = "get_block_hash";
const char* const GetLastBlockHeader::name = "get_last_block_header";
const char* const GetBlockHeaderByHash::name = "get_block_header_by_hash";
const char* const GetBlockHeaderByHeight::name = "get_block_header_by_height";
const char* const GetPeerList::name = "get_peer_list";
const char* const SetLogLevel::name = "set_log_level";
const char* const GetTransactionPool::name = "get_transaction_pool";
const char* const HardForkInfo::name = "hard_fork_info";
const char* const GetOutputHistogram::name = "get_output_histogram";
const char* const GetOutputKeys::name = "get_output_keys";
const char* const GetRPCVersion::name = "get_rpc_version";
const char* const GetPerKBFeeEstimate::name = "get_dynamic_per_kb_fee_estimate";




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
  OBJECT_HAS_MEMBER_OR_THROW(val, "height")
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
  OBJECT_HAS_MEMBER_OR_THROW(val, "block_ids")
  block_ids = cryptonote::json::fromJsonValue<decltype(block_ids)>(val["block_ids"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "start_height")
  start_height = cryptonote::json::fromJsonValue<uint64_t>(val["start_height"]);

}

rapidjson::Value GetBlocksFast::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("blocks", cryptonote::json::toJsonValue<decltype(blocks)>(doc, blocks), al);
  val.AddMember("start_height", start_height, al);
  val.AddMember("current_height", current_height, al);
  val.AddMember("output_indices", cryptonote::json::toJsonValue<decltype(output_indices)>(doc, output_indices), al);

  return val;
}

void GetBlocksFast::Response::fromJson(rapidjson::Value& val)
{
  OBJECT_HAS_MEMBER_OR_THROW(val, "blocks")
  blocks = cryptonote::json::fromJsonValue<decltype(blocks)>(val["blocks"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "start_height")
  start_height = cryptonote::json::fromJsonValue<uint64_t>(val["start_height"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "current_height")
  current_height = cryptonote::json::fromJsonValue<uint64_t>(val["current_height"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "output_indices")
  output_indices = cryptonote::json::fromJsonValue<decltype(output_indices)>(val["output_indices"]);

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
  OBJECT_HAS_MEMBER_OR_THROW(val, "known_hashes")
  known_hashes = cryptonote::json::fromJsonValue<decltype(known_hashes)>(val["known_hashes"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "start_height")
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
  OBJECT_HAS_MEMBER_OR_THROW(val, "hashes")
  hashes = cryptonote::json::fromJsonValue<decltype(hashes)>(val["hashes"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "start_height")
  start_height = cryptonote::json::fromJsonValue<uint64_t>(val["start_height"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "current_height")
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
  OBJECT_HAS_MEMBER_OR_THROW(val, "tx_hashes")
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
  OBJECT_HAS_MEMBER_OR_THROW(val, "txs")
  txs = cryptonote::json::fromJsonValue<decltype(txs)>(val["txs"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "missed_hashes")
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
  OBJECT_HAS_MEMBER_OR_THROW(val, "key_images")
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
  OBJECT_HAS_MEMBER_OR_THROW(val, "spent_status")
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
  OBJECT_HAS_MEMBER_OR_THROW(val, "tx_hash")
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
  OBJECT_HAS_MEMBER_OR_THROW(val, "output_indices")
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
  OBJECT_HAS_MEMBER_OR_THROW(val, "amounts")
  amounts = cryptonote::json::fromJsonValue<decltype(amounts)>(val["amounts"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "count")
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
  OBJECT_HAS_MEMBER_OR_THROW(val, "amounts_with_outputs")
  amounts_with_outputs = cryptonote::json::fromJsonValue<decltype(amounts_with_outputs)>(val["amounts_with_outputs"]);

}


rapidjson::Value SendRawTx::Request::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("tx", cryptonote::json::toJsonValue<decltype(tx)>(doc, tx), al);
  val.AddMember("relay", cryptonote::json::toJsonValue<decltype(relay)>(doc, relay), al);

  return val;
}

void SendRawTx::Request::fromJson(rapidjson::Value& val)
{
  OBJECT_HAS_MEMBER_OR_THROW(val, "tx")
  tx = cryptonote::json::fromJsonValue<decltype(tx)>(val["tx"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "relay")
  relay = cryptonote::json::fromJsonValue<decltype(relay)>(val["relay"]);

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
  OBJECT_HAS_MEMBER_OR_THROW(val, "relayed")
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
  OBJECT_HAS_MEMBER_OR_THROW(val, "height")
  height = cryptonote::json::fromJsonValue<decltype(height)>(val["height"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "target_height")
  target_height = cryptonote::json::fromJsonValue<decltype(target_height)>(val["target_height"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "difficulty")
  difficulty = cryptonote::json::fromJsonValue<decltype(difficulty)>(val["difficulty"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "target")
  target = cryptonote::json::fromJsonValue<decltype(target)>(val["target"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "tx_count")
  tx_count = cryptonote::json::fromJsonValue<decltype(tx_count)>(val["tx_count"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "tx_pool_size")
  tx_pool_size = cryptonote::json::fromJsonValue<decltype(tx_pool_size)>(val["tx_pool_size"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "alt_blocks_count")
  alt_blocks_count = cryptonote::json::fromJsonValue<decltype(alt_blocks_count)>(val["alt_blocks_count"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "outgoing_connections_count")
  outgoing_connections_count = cryptonote::json::fromJsonValue<decltype(outgoing_connections_count)>(val["outgoing_connections_count"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "incoming_connections_count")
  incoming_connections_count = cryptonote::json::fromJsonValue<decltype(incoming_connections_count)>(val["incoming_connections_count"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "white_peerlist_size")
  white_peerlist_size = cryptonote::json::fromJsonValue<decltype(white_peerlist_size)>(val["white_peerlist_size"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "grey_peerlist_size")
  grey_peerlist_size = cryptonote::json::fromJsonValue<decltype(grey_peerlist_size)>(val["grey_peerlist_size"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "testnet")
  testnet = cryptonote::json::fromJsonValue<decltype(testnet)>(val["testnet"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "top_block_hash")
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
  OBJECT_HAS_MEMBER_OR_THROW(val, "height")
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
  OBJECT_HAS_MEMBER_OR_THROW(val, "hash")
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

  val.AddMember("header", cryptonote::json::toJsonValue<decltype(header)>(doc, header), al);

  return val;
}

void GetLastBlockHeader::Response::fromJson(rapidjson::Value& val)
{
  OBJECT_HAS_MEMBER_OR_THROW(val, "header")
  header = cryptonote::json::fromJsonValue<decltype(header)>(val["header"]);
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
  OBJECT_HAS_MEMBER_OR_THROW(val, "hash")
  hash = cryptonote::json::fromJsonValue<decltype(hash)>(val["hash"]);

}

rapidjson::Value GetBlockHeaderByHash::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("header", cryptonote::json::toJsonValue<decltype(header)>(doc, header), al);

  return val;
}

void GetBlockHeaderByHash::Response::fromJson(rapidjson::Value& val)
{
  OBJECT_HAS_MEMBER_OR_THROW(val, "header")
  header = cryptonote::json::fromJsonValue<decltype(header)>(val["header"]);
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
  OBJECT_HAS_MEMBER_OR_THROW(val, "height")
  height = cryptonote::json::fromJsonValue<decltype(height)>(val["height"]);

}

rapidjson::Value GetBlockHeaderByHeight::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("header", cryptonote::json::toJsonValue<decltype(header)>(doc, header), al);

  return val;
}

void GetBlockHeaderByHeight::Response::fromJson(rapidjson::Value& val)
{
  OBJECT_HAS_MEMBER_OR_THROW(val, "header")
  header = cryptonote::json::fromJsonValue<decltype(header)>(val["header"]);
}


rapidjson::Value GetPeerList::Request::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  return val;
}

void GetPeerList::Request::fromJson(rapidjson::Value& val)
{
}

rapidjson::Value GetPeerList::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("white_list", cryptonote::json::toJsonValue<decltype(white_list)>(doc, white_list), al);
  val.AddMember("gray_list", cryptonote::json::toJsonValue<decltype(gray_list)>(doc, gray_list), al);

  return val;
}

void GetPeerList::Response::fromJson(rapidjson::Value& val)
{
  OBJECT_HAS_MEMBER_OR_THROW(val, "white_list")
  white_list = cryptonote::json::fromJsonValue<decltype(white_list)>(val["white_list"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "gray_list")
  gray_list = cryptonote::json::fromJsonValue<decltype(gray_list)>(val["gray_list"]);

}


rapidjson::Value SetLogLevel::Request::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("level", level, al);

  return val;
}

void SetLogLevel::Request::fromJson(rapidjson::Value& val)
{
  OBJECT_HAS_MEMBER_OR_THROW(val, "level")
  level = cryptonote::json::fromJsonValue<decltype(level)>(val["level"]);

}

rapidjson::Value SetLogLevel::Response::toJson(rapidjson::Document& doc)
{
  return Message::toJson(doc);
}

void SetLogLevel::Response::fromJson(rapidjson::Value& val)
{
}


rapidjson::Value GetTransactionPool::Request::toJson(rapidjson::Document& doc)
{
  return Message::toJson(doc);
}

void GetTransactionPool::Request::fromJson(rapidjson::Value& val)
{
}

rapidjson::Value GetTransactionPool::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("transactions", cryptonote::json::toJsonValue<decltype(transactions)>(doc, transactions), al);
  val.AddMember("key_images", cryptonote::json::toJsonValue<decltype(key_images)>(doc, key_images), al);

  return val;
}

void GetTransactionPool::Response::fromJson(rapidjson::Value& val)
{
  OBJECT_HAS_MEMBER_OR_THROW(val, "transactions")
  transactions = cryptonote::json::fromJsonValue<decltype(transactions)>(val["transactions"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "key_images")
  key_images = cryptonote::json::fromJsonValue<decltype(key_images)>(val["key_images"]);

}


rapidjson::Value HardForkInfo::Request::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("version", cryptonote::json::toJsonValue<decltype(version)>(doc, version), al);

  return val;
}

void HardForkInfo::Request::fromJson(rapidjson::Value& val)
{
  OBJECT_HAS_MEMBER_OR_THROW(val, "version")
  version = cryptonote::json::fromJsonValue<decltype(version)>(val["version"]);

}

rapidjson::Value HardForkInfo::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("info", cryptonote::json::toJsonValue<decltype(info)>(doc, info), al);

  return val;
}

void HardForkInfo::Response::fromJson(rapidjson::Value& val)
{
  OBJECT_HAS_MEMBER_OR_THROW(val, "info")
  info = cryptonote::json::fromJsonValue<decltype(info)>(val["info"]);
}


rapidjson::Value GetOutputHistogram::Request::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("amounts", cryptonote::json::toJsonValue<decltype(amounts)>(doc, amounts), al);
  val.AddMember("min_count", cryptonote::json::toJsonValue<decltype(min_count)>(doc, min_count), al);
  val.AddMember("max_count", cryptonote::json::toJsonValue<decltype(max_count)>(doc, max_count), al);
  val.AddMember("unlocked", cryptonote::json::toJsonValue<decltype(unlocked)>(doc, unlocked), al);
  val.AddMember("recent_cutoff", cryptonote::json::toJsonValue<decltype(recent_cutoff)>(doc, recent_cutoff), al);

  return val;
}

void GetOutputHistogram::Request::fromJson(rapidjson::Value& val)
{
  OBJECT_HAS_MEMBER_OR_THROW(val, "amounts")
  amounts = cryptonote::json::fromJsonValue<decltype(amounts)>(val["amounts"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "min_count")
  min_count = cryptonote::json::fromJsonValue<decltype(min_count)>(val["min_count"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "max_count")
  max_count = cryptonote::json::fromJsonValue<decltype(max_count)>(val["max_count"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "unlocked")
  unlocked = cryptonote::json::fromJsonValue<decltype(unlocked)>(val["unlocked"]);

  OBJECT_HAS_MEMBER_OR_THROW(val, "recent_cutoff")
  recent_cutoff = cryptonote::json::fromJsonValue<decltype(recent_cutoff)>(val["recent_cutoff"]);
}

rapidjson::Value GetOutputHistogram::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("histogram", cryptonote::json::toJsonValue<decltype(histogram)>(doc, histogram), al);

  return val;
}

void GetOutputHistogram::Response::fromJson(rapidjson::Value& val)
{
  OBJECT_HAS_MEMBER_OR_THROW(val, "histogram")
  histogram = cryptonote::json::fromJsonValue<decltype(histogram)>(val["histogram"]);

}


rapidjson::Value GetOutputKeys::Request::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("outputs", cryptonote::json::toJsonValue<decltype(outputs)>(doc, outputs), al);

  return val;
}

void GetOutputKeys::Request::fromJson(rapidjson::Value& val)
{
  OBJECT_HAS_MEMBER_OR_THROW(val, "outputs")
  outputs = cryptonote::json::fromJsonValue<decltype(outputs)>(val["outputs"]);
}

rapidjson::Value GetOutputKeys::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("keys", cryptonote::json::toJsonValue<decltype(keys)>(doc, keys), al);

  return val;
}

void GetOutputKeys::Response::fromJson(rapidjson::Value& val)
{
  OBJECT_HAS_MEMBER_OR_THROW(val, "keys")
  keys = cryptonote::json::fromJsonValue<decltype(keys)>(val["keys"]);
}


rapidjson::Value GetRPCVersion::Request::toJson(rapidjson::Document& doc)
{
  return Message::toJson(doc);
}

void GetRPCVersion::Request::fromJson(rapidjson::Value& val)
{
}

rapidjson::Value GetRPCVersion::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("version", cryptonote::json::toJsonValue<decltype(version)>(doc, version), al);

  return val;
}

void GetRPCVersion::Response::fromJson(rapidjson::Value& val)
{
  OBJECT_HAS_MEMBER_OR_THROW(val, "version")
  version = cryptonote::json::fromJsonValue<decltype(version)>(val["version"]);

}

rapidjson::Value GetPerKBFeeEstimate::Request::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("num_grace_blocks", cryptonote::json::toJsonValue<decltype(num_grace_blocks)>(doc, num_grace_blocks), al);

  return val;
}

void GetPerKBFeeEstimate::Request::fromJson(rapidjson::Value& val)
{
  OBJECT_HAS_MEMBER_OR_THROW(val, "num_grace_blocks")
  num_grace_blocks = cryptonote::json::fromJsonValue<decltype(num_grace_blocks)>(val["num_grace_blocks"]);
}

rapidjson::Value GetPerKBFeeEstimate::Response::toJson(rapidjson::Document& doc)
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("estimated_fee_per_kb", cryptonote::json::toJsonValue<decltype(estimated_fee_per_kb)>(doc, estimated_fee_per_kb), al);

  return val;
}

void GetPerKBFeeEstimate::Response::fromJson(rapidjson::Value& val)
{
  OBJECT_HAS_MEMBER_OR_THROW(val, "estimated_fee_per_kb")
  estimated_fee_per_kb = cryptonote::json::fromJsonValue<decltype(estimated_fee_per_kb)>(val["estimated_fee_per_kb"]);

}


}  // namespace rpc

}  // namespace cryptonote
