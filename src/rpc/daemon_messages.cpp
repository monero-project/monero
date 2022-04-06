// Copyright (c) 2016-2022, The Monero Project
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
void GetHeight::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{}

void GetHeight::Request::fromJson(const rapidjson::Value& val)
{
}

void GetHeight::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, height, height);
}

void GetHeight::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, height, height);
}


void GetBlocksFast::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, block_ids, block_ids);
  INSERT_INTO_JSON_OBJECT(dest, start_height, start_height);
  INSERT_INTO_JSON_OBJECT(dest, prune, prune);
}

void GetBlocksFast::Request::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, block_ids, block_ids);
  GET_FROM_JSON_OBJECT(val, start_height, start_height);
  GET_FROM_JSON_OBJECT(val, prune, prune);
}

void GetBlocksFast::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, blocks, blocks);
  INSERT_INTO_JSON_OBJECT(dest, start_height, start_height);
  INSERT_INTO_JSON_OBJECT(dest, current_height, current_height);
  INSERT_INTO_JSON_OBJECT(dest, output_indices, output_indices);
}

void GetBlocksFast::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, blocks, blocks);
  GET_FROM_JSON_OBJECT(val, start_height, start_height);
  GET_FROM_JSON_OBJECT(val, current_height, current_height);
  GET_FROM_JSON_OBJECT(val, output_indices, output_indices);
}


void GetHashesFast::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, known_hashes, known_hashes);
  INSERT_INTO_JSON_OBJECT(dest, start_height, start_height);
}

void GetHashesFast::Request::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, known_hashes, known_hashes);
  GET_FROM_JSON_OBJECT(val, start_height, start_height);
}

void GetHashesFast::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, hashes, hashes);
  INSERT_INTO_JSON_OBJECT(dest, start_height, start_height);
  INSERT_INTO_JSON_OBJECT(dest, current_height, current_height);
}

void GetHashesFast::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, hashes, hashes);
  GET_FROM_JSON_OBJECT(val, start_height, start_height);
  GET_FROM_JSON_OBJECT(val, current_height, current_height);
}


void GetTransactions::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, tx_hashes, tx_hashes);
}

void GetTransactions::Request::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, tx_hashes, tx_hashes);
}

void GetTransactions::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, txs, txs);
  INSERT_INTO_JSON_OBJECT(dest, missed_hashes, missed_hashes);
}

void GetTransactions::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, txs, txs);
  GET_FROM_JSON_OBJECT(val, missed_hashes, missed_hashes);
}


void KeyImagesSpent::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, key_images, key_images);
}

void KeyImagesSpent::Request::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, key_images, key_images);
}

void KeyImagesSpent::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, spent_status, spent_status);
}

void KeyImagesSpent::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, spent_status, spent_status);
}


void GetTxGlobalOutputIndices::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, tx_hash, tx_hash);
}

void GetTxGlobalOutputIndices::Request::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, tx_hash, tx_hash);
}

void GetTxGlobalOutputIndices::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, output_indices, output_indices);
}

void GetTxGlobalOutputIndices::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, output_indices, output_indices);
}

void SendRawTx::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, tx, tx);
  INSERT_INTO_JSON_OBJECT(dest, relay, relay);
}

void SendRawTx::Request::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, tx, tx);
  GET_FROM_JSON_OBJECT(val, relay, relay);
}

void SendRawTx::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, relayed, relayed);
}


void SendRawTx::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, relayed, relayed);
}

void SendRawTxHex::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, tx_as_hex, tx_as_hex);
  INSERT_INTO_JSON_OBJECT(dest, relay, relay);
}

void SendRawTxHex::Request::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, tx_as_hex, tx_as_hex);
  GET_FROM_JSON_OBJECT(val, relay, relay);
}

void StartMining::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, miner_address, miner_address);
  INSERT_INTO_JSON_OBJECT(dest, threads_count, threads_count);
  INSERT_INTO_JSON_OBJECT(dest, do_background_mining, do_background_mining);
  INSERT_INTO_JSON_OBJECT(dest, ignore_battery, ignore_battery);
}

void StartMining::Request::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, miner_address, miner_address);
  GET_FROM_JSON_OBJECT(val, threads_count, threads_count);
  GET_FROM_JSON_OBJECT(val, do_background_mining, do_background_mining);
  GET_FROM_JSON_OBJECT(val, ignore_battery, ignore_battery);
}

void StartMining::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{}

void StartMining::Response::fromJson(const rapidjson::Value& val)
{
}


void StopMining::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{}

void StopMining::Request::fromJson(const rapidjson::Value& val)
{
}

void StopMining::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{}

void StopMining::Response::fromJson(const rapidjson::Value& val)
{
}


void MiningStatus::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{}

void MiningStatus::Request::fromJson(const rapidjson::Value& val)
{
}

void MiningStatus::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, active, active);
  INSERT_INTO_JSON_OBJECT(dest, speed, speed);
  INSERT_INTO_JSON_OBJECT(dest, threads_count, threads_count);
  INSERT_INTO_JSON_OBJECT(dest, address, address);
  INSERT_INTO_JSON_OBJECT(dest, is_background_mining_enabled, is_background_mining_enabled);
}

void MiningStatus::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, active, active);
  GET_FROM_JSON_OBJECT(val, speed, speed);
  GET_FROM_JSON_OBJECT(val, threads_count, threads_count);
  GET_FROM_JSON_OBJECT(val, address, address);
  GET_FROM_JSON_OBJECT(val, is_background_mining_enabled, is_background_mining_enabled);
}


void GetInfo::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{}

void GetInfo::Request::fromJson(const rapidjson::Value& val)
{
}

void GetInfo::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, info, info);
}

void GetInfo::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, info, info);
}


void SaveBC::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{}

void SaveBC::Request::fromJson(const rapidjson::Value& val)
{
}

void SaveBC::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{}

void SaveBC::Response::fromJson(const rapidjson::Value& val)
{
}


void GetBlockHash::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, height, height);
}

void GetBlockHash::Request::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, height, height);
}

void GetBlockHash::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, hash, hash);
}

void GetBlockHash::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, hash, hash);
}


void GetLastBlockHeader::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{}

void GetLastBlockHeader::Request::fromJson(const rapidjson::Value& val)
{
}

void GetLastBlockHeader::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, header, header);
}

void GetLastBlockHeader::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, header, header);
}


void GetBlockHeaderByHash::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, hash, hash);
}

void GetBlockHeaderByHash::Request::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, hash, hash);
}

void GetBlockHeaderByHash::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, header, header);
}

void GetBlockHeaderByHash::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, header, header);
}


void GetBlockHeaderByHeight::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, height, height);
}

void GetBlockHeaderByHeight::Request::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, height, height);
}

void GetBlockHeaderByHeight::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, header, header);
}

void GetBlockHeaderByHeight::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, header, header);
}


void GetBlockHeadersByHeight::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, heights, heights);
}

void GetBlockHeadersByHeight::Request::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, heights, heights);
}

void GetBlockHeadersByHeight::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, headers, headers);
}

void GetBlockHeadersByHeight::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, headers, headers);
}


void GetPeerList::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{}

void GetPeerList::Request::fromJson(const rapidjson::Value& val)
{
}

void GetPeerList::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, white_list, white_list);
  INSERT_INTO_JSON_OBJECT(dest, gray_list, gray_list);
}

void GetPeerList::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, white_list, white_list);
  GET_FROM_JSON_OBJECT(val, gray_list, gray_list);
}


void SetLogLevel::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, level, level);
}

void SetLogLevel::Request::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, level, level);
}

void SetLogLevel::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{}

void SetLogLevel::Response::fromJson(const rapidjson::Value& val)
{
}


void GetTransactionPool::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{}

void GetTransactionPool::Request::fromJson(const rapidjson::Value& val)
{
}

void GetTransactionPool::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, transactions, transactions);
  INSERT_INTO_JSON_OBJECT(dest, key_images, key_images);
}

void GetTransactionPool::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, transactions, transactions);
  GET_FROM_JSON_OBJECT(val, key_images, key_images);
}


void HardForkInfo::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, version, version);
}

void HardForkInfo::Request::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, version, version);
}

void HardForkInfo::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, info, info);
}

void HardForkInfo::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, info, info);
}


void GetOutputHistogram::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, amounts, amounts);
  INSERT_INTO_JSON_OBJECT(dest, min_count, min_count);
  INSERT_INTO_JSON_OBJECT(dest, max_count, max_count);
  INSERT_INTO_JSON_OBJECT(dest, unlocked, unlocked);
  INSERT_INTO_JSON_OBJECT(dest, recent_cutoff, recent_cutoff);
}

void GetOutputHistogram::Request::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, amounts, amounts);
  GET_FROM_JSON_OBJECT(val, min_count, min_count);
  GET_FROM_JSON_OBJECT(val, max_count, max_count);
  GET_FROM_JSON_OBJECT(val, unlocked, unlocked);
  GET_FROM_JSON_OBJECT(val, recent_cutoff, recent_cutoff);
}

void GetOutputHistogram::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, histogram, histogram);
}

void GetOutputHistogram::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, histogram, histogram);
}


void GetOutputKeys::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, outputs, outputs);
}

void GetOutputKeys::Request::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, outputs, outputs);
}

void GetOutputKeys::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, keys, keys);
}

void GetOutputKeys::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, keys, keys);
}


void GetRPCVersion::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{}

void GetRPCVersion::Request::fromJson(const rapidjson::Value& val)
{
}

void GetRPCVersion::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, version, version);
}

void GetRPCVersion::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, version, version);
}

void GetFeeEstimate::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, num_grace_blocks, num_grace_blocks);
}

void GetFeeEstimate::Request::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, num_grace_blocks, num_grace_blocks);
}

void GetFeeEstimate::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, estimated_base_fee, estimated_base_fee);
  INSERT_INTO_JSON_OBJECT(dest, fee_mask, fee_mask);
  INSERT_INTO_JSON_OBJECT(dest, size_scale, size_scale);
  INSERT_INTO_JSON_OBJECT(dest, hard_fork_version, hard_fork_version);
}

void GetFeeEstimate::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, estimated_base_fee, estimated_base_fee);
  GET_FROM_JSON_OBJECT(val, fee_mask, fee_mask);
  GET_FROM_JSON_OBJECT(val, size_scale, size_scale);
  GET_FROM_JSON_OBJECT(val, hard_fork_version, hard_fork_version);
}

void GetOutputDistribution::Request::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, amounts, amounts);
  INSERT_INTO_JSON_OBJECT(dest, from_height, from_height);
  INSERT_INTO_JSON_OBJECT(dest, to_height, to_height);
  INSERT_INTO_JSON_OBJECT(dest, cumulative, cumulative);
}

void GetOutputDistribution::Request::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, amounts, amounts);
  GET_FROM_JSON_OBJECT(val, from_height, from_height);
  GET_FROM_JSON_OBJECT(val, to_height, to_height);
  GET_FROM_JSON_OBJECT(val, cumulative, cumulative);
}

void GetOutputDistribution::Response::doToJson(rapidjson::Writer<epee::byte_stream>& dest) const
{
  INSERT_INTO_JSON_OBJECT(dest, status, status);
  INSERT_INTO_JSON_OBJECT(dest, distributions, distributions);
}

void GetOutputDistribution::Response::fromJson(const rapidjson::Value& val)
{
  GET_FROM_JSON_OBJECT(val, status, status);
  GET_FROM_JSON_OBJECT(val, distributions, distributions);
}

}  // namespace rpc

}  // namespace cryptonote
