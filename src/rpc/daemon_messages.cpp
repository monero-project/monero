// Copyright (c) 2016-2018, The Monero Project
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
const char* const StartMining::name = "start_mining";
const char* const StopMining::name = "stop_mining";
const char* const MiningStatus::name = "mining_status";
const char* const GetInfo::name = "get_info";
const char* const SaveBC::name = "save_bc";
const char* const GetBlockHash::name = "get_block_hash";
const char* const GetLastBlockHeader::name = "get_last_block_header";
const char* const GetBlockHeaderByHash::name = "get_block_header_by_hash";
const char* const GetBlockHeaderByHeight::name = "get_block_header_by_height";
const char* const GetBlockHeadersByHeight::name = "get_block_headers_by_height";
const char* const GetPeerList::name = "get_peer_list";
const char* const SetLogLevel::name = "set_log_level";
const char* const GetTransactionPool::name = "get_transaction_pool";
const char* const HardForkInfo::name = "hard_fork_info";
const char* const GetOutputHistogram::name = "get_output_histogram";
const char* const GetOutputKeys::name = "get_output_keys";
const char* const GetRPCVersion::name = "get_rpc_version";
const char* const GetPerKBFeeEstimate::name = "get_dynamic_per_kb_fee_estimate";




rapidjson::Value GetHeight::Request::toJson(rapidjson::Document& doc) const
{
  return Message::toJson(doc);
}

void GetHeight::Request::fromJson(rapidjson::Value& val)
{
}

rapidjson::Value GetHeight::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("height", height, al);

  return val;
}

void GetHeight::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, height, height);
}


rapidjson::Value GetBlocksFast::Request::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, block_ids, block_ids);
  val.AddMember("start_height", start_height, al);
  val.AddMember("prune", prune, al);

  return val;
}

void GetBlocksFast::Request::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, block_ids, block_ids);
  GET_FROM_JSON_OBJECT(val, start_height, start_height);
  GET_FROM_JSON_OBJECT(val, prune, prune);
}

rapidjson::Value GetBlocksFast::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, blocks, blocks);
  val.AddMember("start_height", start_height, al);
  val.AddMember("current_height", current_height, al);
  INSERT_INTO_JSON_OBJECT(val, doc, output_indices, output_indices);

  return val;
}

void GetBlocksFast::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, blocks, blocks);
  GET_FROM_JSON_OBJECT(val, start_height, start_height);
  GET_FROM_JSON_OBJECT(val, current_height, current_height);
  GET_FROM_JSON_OBJECT(val, output_indices, output_indices);
}


rapidjson::Value GetHashesFast::Request::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, known_hashes, known_hashes);
  val.AddMember("start_height", start_height, al);

  return val;
}

void GetHashesFast::Request::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, known_hashes, known_hashes);
  GET_FROM_JSON_OBJECT(val, start_height, start_height);
}

rapidjson::Value GetHashesFast::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, hashes, hashes);
  val.AddMember("start_height", start_height, al);
  val.AddMember("current_height", current_height, al);

  return val;
}

void GetHashesFast::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, hashes, hashes);
  GET_FROM_JSON_OBJECT(val, start_height, start_height);
  GET_FROM_JSON_OBJECT(val, current_height, current_height);
}


rapidjson::Value GetTransactions::Request::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, tx_hashes, tx_hashes);

  return val;
}

void GetTransactions::Request::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, tx_hashes, tx_hashes);
}

rapidjson::Value GetTransactions::Response::toJson(rapidjson::Document& doc) const
{
  rapidjson::Value val(rapidjson::kObjectType);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, txs, txs);
  INSERT_INTO_JSON_OBJECT(val, doc, missed_hashes, missed_hashes);

  return val;
}

void GetTransactions::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, txs, txs);
  GET_FROM_JSON_OBJECT(val, missed_hashes, missed_hashes);
}


rapidjson::Value KeyImagesSpent::Request::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, key_images, key_images);

  return val;
}

void KeyImagesSpent::Request::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, key_images, key_images);
}

rapidjson::Value KeyImagesSpent::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, spent_status, spent_status);

  return val;
}

void KeyImagesSpent::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, spent_status, spent_status);
}


rapidjson::Value GetTxGlobalOutputIndices::Request::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, tx_hash, tx_hash);

  return val;
}

void GetTxGlobalOutputIndices::Request::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, tx_hash, tx_hash);
}

rapidjson::Value GetTxGlobalOutputIndices::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, output_indices, output_indices);

  return val;
}

void GetTxGlobalOutputIndices::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, output_indices, output_indices);
}


rapidjson::Value GetRandomOutputsForAmounts::Request::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, amounts, amounts);
  INSERT_INTO_JSON_OBJECT(val, doc, count, count);

  return val;
}

void GetRandomOutputsForAmounts::Request::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, amounts, amounts);
  GET_FROM_JSON_OBJECT(val, count, count);
}

rapidjson::Value GetRandomOutputsForAmounts::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, amounts_with_outputs, amounts_with_outputs);

  return val;
}

void GetRandomOutputsForAmounts::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, amounts_with_outputs, amounts_with_outputs);
}


rapidjson::Value SendRawTx::Request::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, tx, tx);
  INSERT_INTO_JSON_OBJECT(val, doc, relay, relay);

  return val;
}

void SendRawTx::Request::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, tx, tx);
  GET_FROM_JSON_OBJECT(val, relay, relay);
}

rapidjson::Value SendRawTx::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, relayed, relayed);

  return val;
}


void SendRawTx::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, relayed, relayed);
}

rapidjson::Value StartMining::Request::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, miner_address, miner_address);
  INSERT_INTO_JSON_OBJECT(val, doc, threads_count, threads_count);
  INSERT_INTO_JSON_OBJECT(val, doc, do_background_mining, do_background_mining);
  INSERT_INTO_JSON_OBJECT(val, doc, ignore_battery, ignore_battery);

  return val;
}

void StartMining::Request::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, miner_address, miner_address);
  GET_FROM_JSON_OBJECT(val, threads_count, threads_count);
  GET_FROM_JSON_OBJECT(val, do_background_mining, do_background_mining);
  GET_FROM_JSON_OBJECT(val, ignore_battery, ignore_battery);
}

rapidjson::Value StartMining::Response::toJson(rapidjson::Document& doc) const
{
  return Message::toJson(doc);
}

void StartMining::Response::fromJson(rapidjson::Value& val)
{
}


rapidjson::Value StopMining::Request::toJson(rapidjson::Document& doc) const
{
  return Message::toJson(doc);
}

void StopMining::Request::fromJson(rapidjson::Value& val)
{
}

rapidjson::Value StopMining::Response::toJson(rapidjson::Document& doc) const
{
  return Message::toJson(doc);
}

void StopMining::Response::fromJson(rapidjson::Value& val)
{
}


rapidjson::Value MiningStatus::Request::toJson(rapidjson::Document& doc) const
{
  return Message::toJson(doc);
}

void MiningStatus::Request::fromJson(rapidjson::Value& val)
{
}

rapidjson::Value MiningStatus::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, active, active);
  INSERT_INTO_JSON_OBJECT(val, doc, speed, speed);
  INSERT_INTO_JSON_OBJECT(val, doc, threads_count, threads_count);
  INSERT_INTO_JSON_OBJECT(val, doc, address, address);
  INSERT_INTO_JSON_OBJECT(val, doc, is_background_mining_enabled, is_background_mining_enabled);

  return val;
}

void MiningStatus::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, active, active);
  GET_FROM_JSON_OBJECT(val, speed, speed);
  GET_FROM_JSON_OBJECT(val, threads_count, threads_count);
  GET_FROM_JSON_OBJECT(val, address, address);
  GET_FROM_JSON_OBJECT(val, is_background_mining_enabled, is_background_mining_enabled);
}


rapidjson::Value GetInfo::Request::toJson(rapidjson::Document& doc) const
{
  return Message::toJson(doc);
}

void GetInfo::Request::fromJson(rapidjson::Value& val)
{
}

rapidjson::Value GetInfo::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, info, info);

  return val;
}

void GetInfo::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, info, info);
}


rapidjson::Value SaveBC::Request::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  return val;
}

void SaveBC::Request::fromJson(rapidjson::Value& val)
{
}

rapidjson::Value SaveBC::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  return val;
}

void SaveBC::Response::fromJson(rapidjson::Value& val)
{
}


rapidjson::Value GetBlockHash::Request::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, height, height);

  return val;
}

void GetBlockHash::Request::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, height, height);
}

rapidjson::Value GetBlockHash::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, hash, hash);

  return val;
}

void GetBlockHash::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, hash, hash);
}


rapidjson::Value GetLastBlockHeader::Request::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  return val;
}

void GetLastBlockHeader::Request::fromJson(rapidjson::Value& val)
{
}

rapidjson::Value GetLastBlockHeader::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, header, header);

  return val;
}

void GetLastBlockHeader::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, header, header);
}


rapidjson::Value GetBlockHeaderByHash::Request::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, hash, hash);

  return val;
}

void GetBlockHeaderByHash::Request::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, hash, hash);
}

rapidjson::Value GetBlockHeaderByHash::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, header, header);

  return val;
}

void GetBlockHeaderByHash::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, header, header);
}


rapidjson::Value GetBlockHeaderByHeight::Request::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, height, height);

  return val;
}

void GetBlockHeaderByHeight::Request::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, height, height);
}

rapidjson::Value GetBlockHeaderByHeight::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, header, header);

  return val;
}

void GetBlockHeaderByHeight::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, header, header);
}


rapidjson::Value GetBlockHeadersByHeight::Request::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, heights, heights);

  return val;
}

void GetBlockHeadersByHeight::Request::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, heights, heights);
}

rapidjson::Value GetBlockHeadersByHeight::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, headers, headers);

  return val;
}

void GetBlockHeadersByHeight::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, headers, headers);
}


rapidjson::Value GetPeerList::Request::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  return val;
}

void GetPeerList::Request::fromJson(rapidjson::Value& val)
{
}

rapidjson::Value GetPeerList::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, white_list, white_list);
  INSERT_INTO_JSON_OBJECT(val, doc, gray_list, gray_list);

  return val;
}

void GetPeerList::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, white_list, white_list);
  GET_FROM_JSON_OBJECT(val, gray_list, gray_list);
}


rapidjson::Value SetLogLevel::Request::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  val.AddMember("level", level, al);

  return val;
}

void SetLogLevel::Request::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, level, level);
}

rapidjson::Value SetLogLevel::Response::toJson(rapidjson::Document& doc) const
{
  return Message::toJson(doc);
}

void SetLogLevel::Response::fromJson(rapidjson::Value& val)
{
}


rapidjson::Value GetTransactionPool::Request::toJson(rapidjson::Document& doc) const
{
  return Message::toJson(doc);
}

void GetTransactionPool::Request::fromJson(rapidjson::Value& val)
{
}

rapidjson::Value GetTransactionPool::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, transactions, transactions);
  INSERT_INTO_JSON_OBJECT(val, doc, key_images, key_images);

  return val;
}

void GetTransactionPool::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, transactions, transactions);
  GET_FROM_JSON_OBJECT(val, key_images, key_images);
}


rapidjson::Value HardForkInfo::Request::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, version, version);

  return val;
}

void HardForkInfo::Request::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, version, version);
}

rapidjson::Value HardForkInfo::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, info, info);

  return val;
}

void HardForkInfo::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, info, info);
}


rapidjson::Value GetOutputHistogram::Request::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, amounts, amounts);
  INSERT_INTO_JSON_OBJECT(val, doc, min_count, min_count);
  INSERT_INTO_JSON_OBJECT(val, doc, max_count, max_count);
  INSERT_INTO_JSON_OBJECT(val, doc, unlocked, unlocked);
  INSERT_INTO_JSON_OBJECT(val, doc, recent_cutoff, recent_cutoff);

  return val;
}

void GetOutputHistogram::Request::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, amounts, amounts);
  GET_FROM_JSON_OBJECT(val, min_count, min_count);
  GET_FROM_JSON_OBJECT(val, max_count, max_count);
  GET_FROM_JSON_OBJECT(val, unlocked, unlocked);
  GET_FROM_JSON_OBJECT(val, recent_cutoff, recent_cutoff);
}

rapidjson::Value GetOutputHistogram::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, histogram, histogram);

  return val;
}

void GetOutputHistogram::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, histogram, histogram);
}


rapidjson::Value GetOutputKeys::Request::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, outputs, outputs);

  return val;
}

void GetOutputKeys::Request::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, outputs, outputs);
}

rapidjson::Value GetOutputKeys::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, keys, keys);

  return val;
}

void GetOutputKeys::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, keys, keys);
}


rapidjson::Value GetRPCVersion::Request::toJson(rapidjson::Document& doc) const
{
  return Message::toJson(doc);
}

void GetRPCVersion::Request::fromJson(rapidjson::Value& val)
{
}

rapidjson::Value GetRPCVersion::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, version, version);

  return val;
}

void GetRPCVersion::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, version, version);
}

rapidjson::Value GetPerKBFeeEstimate::Request::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, num_grace_blocks, num_grace_blocks);

  return val;
}

void GetPerKBFeeEstimate::Request::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, num_grace_blocks, num_grace_blocks);
}

rapidjson::Value GetPerKBFeeEstimate::Response::toJson(rapidjson::Document& doc) const
{
  auto val = Message::toJson(doc);

  auto& al = doc.GetAllocator();

  INSERT_INTO_JSON_OBJECT(val, doc, estimated_fee_per_kb, estimated_fee_per_kb);

  return val;
}

void GetPerKBFeeEstimate::Response::fromJson(rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, estimated_fee_per_kb, estimated_fee_per_kb);
}


}  // namespace rpc

}  // namespace cryptonote
