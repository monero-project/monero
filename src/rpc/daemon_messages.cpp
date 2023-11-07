// Copyright (c) 2016-2023, The Monero Project
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
#include "cryptonote_config.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "serialization/wire.h"
#include "serialization/wire/adapted/list.h"
#include "serialization/wire/adapted/unordered_map.h"
#include "serialization/wire/adapted/vector.h"
#include "serialization/wire/wrapper/array.h"
#include "serialization/wire/wrappers_impl.h"

namespace cryptonote
{

namespace rpc
{
namespace
{
  using max_outputs = wire::max_element_count<2000>;

  template<typename F, typename T>
  void height_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version));
  }

  template<typename F, typename T>
  void height_response_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(height));
  }
}

void GetHeight::Request::write_bytes(wire::writer& dest) const
{
  height_request_map(dest, *this);
}

void GetHeight::Request::read_bytes(wire::reader& source)
{
  height_request_map(source, *this);
}

void GetHeight::Response::write_bytes(wire::writer& dest) const
{
  height_response_map(dest, *this);
}

void GetHeight::Response::read_bytes(wire::reader& source)
{
  height_response_map(source, *this);
}

// technically exceeds max bounds, but is "OK"
static void read_bytes(wire::reader& source, std::vector<block_with_transactions>& dest)
{ wire_read::array_unchecked(source, dest, 0, COMMAND_RPC_GET_BLOCKS_FAST_MAX_BLOCK_COUNT); }

static void write_bytes(wire::writer& dest, const std::vector<block_with_transactions>& source)
{ wire_write::array(dest, source); }

namespace
{
  template<typename F, typename T>
  void blocks_fast_request_map(F& format, T& self)
  {
    wire::object(format,
      WIRE_FIELD(rpc_version),
      WIRE_FIELD(block_ids),
      WIRE_FIELD(start_height),
      WIRE_FIELD(prune)
    );
  }

  template<typename F, typename T>
  void blocks_fast_response_map(F& format, T& self)
  {
    using max_blocks = wire::max_element_count<COMMAND_RPC_GET_BLOCKS_FAST_MAX_BLOCK_COUNT>;
    using max_txes = wire::max_element_count<COMMAND_RPC_GET_BLOCKS_FAST_MAX_TX_COUNT>;
    wire::object(format,
      WIRE_FIELD(rpc_version),
      WIRE_FIELD(blocks),
      WIRE_FIELD(start_height),
      WIRE_FIELD(current_height),
      wire::field("output_indices", wire::array<max_blocks>(wire::array<max_txes>(wire::array<max_outputs>(std::ref(self.output_indices)))))
    );
  }
}

void GetBlocksFast::Request::write_bytes(wire::writer& dest) const
{
  blocks_fast_request_map(dest, *this);
}

void GetBlocksFast::Request::read_bytes(wire::reader& source)
{
  blocks_fast_request_map(source, *this);
}

void GetBlocksFast::Response::write_bytes(wire::writer& dest) const
{
  blocks_fast_response_map(dest, *this);
}

void GetBlocksFast::Response::read_bytes(wire::reader& source)
{
  blocks_fast_response_map(source, *this);
}

namespace
{
  template<typename F, typename T>
  void hashes_fast_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(known_hashes), WIRE_FIELD(start_height));
  }

  template<typename F, typename T>
  void hashes_fast_response_map(F& format, T& self)
  {
    wire::object(format,
      WIRE_FIELD(rpc_version),
      WIRE_FIELD(hashes),
      WIRE_FIELD(start_height),
      WIRE_FIELD(current_height)
    );
  }
}

void GetHashesFast::Request::write_bytes(wire::writer& dest) const
{
  hashes_fast_request_map(dest, *this);
}

void GetHashesFast::Request::read_bytes(wire::reader& source)
{
  hashes_fast_request_map(source, *this);
}

void GetHashesFast::Response::write_bytes(wire::writer& dest) const
{
  hashes_fast_response_map(dest, *this);
}

void GetHashesFast::Response::read_bytes(wire::reader& source)
{
  hashes_fast_response_map(source, *this);
}

namespace
{
  template<typename F, typename T>
  void transactions_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(tx_hashes));
  }

  template<typename F, typename T>
  void transactions_response_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(txs), WIRE_FIELD(missed_hashes));
  }
}

void GetTransactions::Request::write_bytes(wire::writer& dest) const
{
  transactions_request_map(dest, *this);
}

void GetTransactions::Request::read_bytes(wire::reader& source)
{
  transactions_request_map(source, *this);
}

void GetTransactions::Response::write_bytes(wire::writer& dest) const
{
  transactions_response_map(dest, *this);
}

void GetTransactions::Response::read_bytes(wire::reader& source)
{
  throw std::logic_error{"Not yet imeplemented, reading to unordered_map"};
}

namespace
{
  template<typename F, typename T>
  void key_images_spent_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(key_images));
  }

  template<typename F, typename T>
  void key_images_spent_response_map(F& format, T& self)
  {
    using max_spent = wire::max_element_count<65536>;
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD_ARRAY(spent_status, max_spent));
  }
}

void KeyImagesSpent::Request::write_bytes(wire::writer& dest) const
{
  key_images_spent_request_map(dest, *this);
}

void KeyImagesSpent::Request::read_bytes(wire::reader& source)
{
  key_images_spent_request_map(source, *this);
}

void KeyImagesSpent::Response::write_bytes(wire::writer& dest) const
{
  key_images_spent_response_map(dest, *this);
}

void KeyImagesSpent::Response::read_bytes(wire::reader& source)
{
  key_images_spent_response_map(source, *this);
}

namespace
{
  template<typename F, typename T>
  void output_indices_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(tx_hash));
  }

  template<typename F, typename T>
  void output_indices_response_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD_ARRAY(output_indices, max_outputs));
  }
}

void GetTxGlobalOutputIndices::Request::write_bytes(wire::writer& dest) const
{
  output_indices_request_map(dest, *this);
}

void GetTxGlobalOutputIndices::Request::read_bytes(wire::reader& source)
{
  output_indices_request_map(source, *this);
}

void GetTxGlobalOutputIndices::Response::write_bytes(wire::writer& dest) const
{
  output_indices_response_map(dest, *this);
}

void GetTxGlobalOutputIndices::Response::read_bytes(wire::reader& source)
{
  output_indices_response_map(source, *this);
}

namespace
{
  template<typename F, typename T>
  void send_raw_tx_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(tx), WIRE_FIELD(relay));
  }

  template<typename F, typename T>
  void send_raw_tx_response_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(relayed));
  }
}

void SendRawTx::Request::write_bytes(wire::writer& dest) const
{
  send_raw_tx_request_map(dest, *this);
}

void SendRawTx::Request::read_bytes(wire::reader& source)
{
  send_raw_tx_request_map(source, *this);
}

void SendRawTx::Response::write_bytes(wire::writer& dest) const
{
  send_raw_tx_response_map(dest, *this);
}


void SendRawTx::Response::read_bytes(wire::reader& source)
{
  send_raw_tx_response_map(source, *this);
}

namespace
{
  template<typename F, typename T>
  void send_raw_tx_hex_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(tx_as_hex), WIRE_FIELD(relay));
  }
}

void SendRawTxHex::Request::write_bytes(wire::writer& dest) const
{
  send_raw_tx_hex_request_map(dest, *this);
}

void SendRawTxHex::Request::read_bytes(wire::reader& source)
{
  send_raw_tx_hex_request_map(source, *this);
}

namespace
{
  template<typename F, typename T>
  void start_mining_request_map(F& format, T& self)
  {
    wire::object(format,
      WIRE_FIELD(rpc_version),
      WIRE_FIELD(miner_address),
      WIRE_FIELD(threads_count),
      WIRE_FIELD(do_background_mining),
      WIRE_FIELD(ignore_battery)
    );
  }

  template<typename F, typename T>
  void start_mining_response_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version));
  }
}

void StartMining::Request::write_bytes(wire::writer& dest) const
{
  start_mining_request_map(dest, *this);
}

void StartMining::Request::read_bytes(wire::reader& source)
{
  start_mining_request_map(source, *this);
}

void StartMining::Response::write_bytes(wire::writer& dest) const
{
  start_mining_response_map(dest, *this);
}

void StartMining::Response::read_bytes(wire::reader& source)
{
  start_mining_response_map(source, *this);
}

namespace
{
  template<typename F, typename T>
  void stop_mining_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version));
  }

  template<typename F, typename T>
  void stop_mining_response_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version));
  }
}

void StopMining::Request::write_bytes(wire::writer& dest) const
{
  stop_mining_request_map(dest, *this);
}

void StopMining::Request::read_bytes(wire::reader& source)
{
  stop_mining_request_map(source, *this);
}

void StopMining::Response::write_bytes(wire::writer& dest) const
{
  stop_mining_response_map(dest, *this);
}

void StopMining::Response::read_bytes(wire::reader& source)
{
  stop_mining_response_map(source, *this);
}

namespace
{
  template<typename F, typename T>
  void mining_status_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version));
  }

  template<typename F, typename T>
  void mining_status_response_map(F& format, T& self)
  {
    wire::object(format,
      WIRE_FIELD(rpc_version),
      WIRE_FIELD(active),
      WIRE_FIELD(speed),
      WIRE_FIELD(threads_count),
      WIRE_FIELD(address),
      WIRE_FIELD(is_background_mining_enabled)
    );
  }
}

void MiningStatus::Request::write_bytes(wire::writer& dest) const
{
  mining_status_request_map(dest, *this);
}

void MiningStatus::Request::read_bytes(wire::reader& source)
{
  mining_status_request_map(source, *this);
}

void MiningStatus::Response::write_bytes(wire::writer& dest) const
{
  mining_status_response_map(dest, *this);
}

void MiningStatus::Response::read_bytes(wire::reader& source)
{
  mining_status_response_map(source, *this);
}

namespace
{
  template<typename F, typename T>
  void get_info_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version));
  }

  template<typename F, typename T>
  void get_info_response_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(info));
  }
}

void GetInfo::Request::write_bytes(wire::writer& dest) const
{
  get_info_request_map(dest, *this);
}

void GetInfo::Request::read_bytes(wire::reader& source)
{
  get_info_request_map(source, *this);
}

void GetInfo::Response::write_bytes(wire::writer& dest) const
{
  get_info_response_map(dest, *this);
}

void GetInfo::Response::read_bytes(wire::reader& source)
{
  get_info_response_map(source, *this);
}

namespace
{
  template<typename F, typename T>
  void save_bc_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version));
  }

  template<typename F, typename T>
  void save_bc_response_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version));
  }
}

void SaveBC::Request::write_bytes(wire::writer& dest) const
{
  save_bc_request_map(dest, *this);
}

void SaveBC::Request::read_bytes(wire::reader& source)
{
  save_bc_request_map(source, *this);
}

void SaveBC::Response::write_bytes(wire::writer& dest) const
{
  save_bc_response_map(dest, *this);
}

void SaveBC::Response::read_bytes(wire::reader& source)
{
  save_bc_response_map(source, *this);
}

namespace
{
  template<typename F, typename T>
  void get_block_hash_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(height));
  }

  template<typename F, typename T>
  void get_block_hash_response_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(hash));
  }
}

void GetBlockHash::Request::write_bytes(wire::writer& dest) const
{
  get_block_hash_request_map(dest, *this);
}

void GetBlockHash::Request::read_bytes(wire::reader& source)
{
  get_block_hash_request_map(source, *this);
}

void GetBlockHash::Response::write_bytes(wire::writer& dest) const
{
  get_block_hash_response_map(dest, *this);
}

void GetBlockHash::Response::read_bytes(wire::reader& source)
{
  get_block_hash_response_map(source, *this);
}

namespace
{
  template<typename F, typename T>
  void get_last_block_header_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version));
  }

  template<typename F, typename T>
  void get_last_block_header_response_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(header));
  }
}

void GetLastBlockHeader::Request::write_bytes(wire::writer& dest) const
{
  get_last_block_header_request_map(dest, *this);
}

void GetLastBlockHeader::Request::read_bytes(wire::reader& source)
{
  get_last_block_header_request_map(source, *this);
}

void GetLastBlockHeader::Response::write_bytes(wire::writer& dest) const
{
  get_last_block_header_response_map(dest, *this);
}

void GetLastBlockHeader::Response::read_bytes(wire::reader& source)
{
  get_last_block_header_response_map(source, *this);
}

namespace
{
  template<typename F, typename T>
  void block_header_hash_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(hash));
  }

  template<typename F, typename T>
  void block_header_hash_response_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(header));
  }
}

void GetBlockHeaderByHash::Request::write_bytes(wire::writer& dest) const
{
  block_header_hash_request_map(dest, *this);
}

void GetBlockHeaderByHash::Request::read_bytes(wire::reader& source)
{
  block_header_hash_request_map(source, *this);
}

void GetBlockHeaderByHash::Response::write_bytes(wire::writer& dest) const
{
  block_header_hash_response_map(dest, *this);
}

void GetBlockHeaderByHash::Response::read_bytes(wire::reader& source)
{
  block_header_hash_response_map(source, *this);
}

namespace
{
  template<typename F, typename T>
  void header_by_height_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(height));
  }

  template<typename F, typename T>
  void header_by_height_response_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(header));
  }
}

void GetBlockHeaderByHeight::Request::write_bytes(wire::writer& dest) const
{
  header_by_height_request_map(dest, *this);
}

void GetBlockHeaderByHeight::Request::read_bytes(wire::reader& source)
{
  header_by_height_request_map(source, *this);
}

void GetBlockHeaderByHeight::Response::write_bytes(wire::writer& dest) const
{
  header_by_height_response_map(dest, *this);
}

void GetBlockHeaderByHeight::Response::read_bytes(wire::reader& source)
{
  header_by_height_response_map(source, *this);
}

namespace
{
  using max_headers = wire::max_element_count<2048>;
  template<typename F, typename T>
  void headers_by_height_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD_ARRAY(heights, max_headers));
  }

  template<typename F, typename T>
  void headers_by_height_response_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD_ARRAY(headers, max_headers));
  }
}

void GetBlockHeadersByHeight::Request::write_bytes(wire::writer& dest) const
{
  headers_by_height_request_map(dest, *this);
}

void GetBlockHeadersByHeight::Request::read_bytes(wire::reader& source)
{
  headers_by_height_request_map(source, *this);
}

void GetBlockHeadersByHeight::Response::write_bytes(wire::writer& dest) const
{
  headers_by_height_response_map(dest, *this);
}

void GetBlockHeadersByHeight::Response::read_bytes(wire::reader& source)
{
  headers_by_height_response_map(source, *this);
}

namespace
{
  template<typename F, typename T>
  void peer_list_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version));
  }

  template<typename F, typename T>
  void peer_list_response_map(F& format, T& self)
  {
    using max_peers = wire::max_element_count<8192>;
    wire::object(format,
      WIRE_FIELD(rpc_version),
      WIRE_FIELD_ARRAY(white_list, max_peers),
      WIRE_FIELD_ARRAY(gray_list, max_peers)
    );
  }
}

void GetPeerList::Request::write_bytes(wire::writer& dest) const
{
  peer_list_request_map(dest, *this);
}

void GetPeerList::Request::read_bytes(wire::reader& source)
{
  peer_list_request_map(source, *this);
}

void GetPeerList::Response::write_bytes(wire::writer& dest) const
{
  peer_list_response_map(dest, *this);
}

void GetPeerList::Response::read_bytes(wire::reader& source)
{
  peer_list_response_map(source, *this);
}

namespace
{
  template<typename F, typename T>
  void set_log_level_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(level));
  }

  template<typename F, typename T>
  void set_log_level_response_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version));
  }
}

void SetLogLevel::Request::write_bytes(wire::writer& dest) const
{
  set_log_level_request_map(dest, *this);
}

void SetLogLevel::Request::read_bytes(wire::reader& source)
{
  set_log_level_request_map(source, *this);
}

void SetLogLevel::Response::write_bytes(wire::writer& dest) const
{
  set_log_level_response_map(dest, *this);
}

void SetLogLevel::Response::read_bytes(wire::reader& source)
{
  set_log_level_response_map(source, *this);
}

namespace
{
  template<typename F, typename T>
  void transaction_pool_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version));
  }

  template<typename F, typename T>
  void transaction_pool_response_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(transactions), WIRE_FIELD(key_images));
  }
}

void GetTransactionPool::Request::write_bytes(wire::writer& dest) const
{
  transaction_pool_request_map(dest, *this);
}

void GetTransactionPool::Request::read_bytes(wire::reader& source)
{
  transaction_pool_request_map(source, *this);
}

void GetTransactionPool::Response::write_bytes(wire::writer& dest) const
{
  transaction_pool_response_map(dest, *this);
}

void GetTransactionPool::Response::read_bytes(wire::reader& source)
{
  throw std::logic_error{"Not yet imeplemented, reading to unordered_map"};
}

namespace
{
  template<typename F, typename T>
  void fork_info_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(version));
  }

  template<typename F, typename T>
  void fork_info_response_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(info));
  }
}

void HardForkInfo::Request::write_bytes(wire::writer& dest) const
{
  fork_info_request_map(dest, *this);
}

void HardForkInfo::Request::read_bytes(wire::reader& source)
{
  fork_info_request_map(source, *this);
}

void HardForkInfo::Response::write_bytes(wire::writer& dest) const
{
  fork_info_response_map(dest, *this);
}

void HardForkInfo::Response::read_bytes(wire::reader& source)
{
  fork_info_response_map(source, *this);
}

namespace
{
  using max_histograms = wire::max_element_count<16384>;

  template<typename F, typename T>
  void output_histogram_request_map(F& format, T& self)
  {
    wire::object(format,
      WIRE_FIELD(rpc_version),
      WIRE_FIELD_ARRAY(amounts, max_histograms),
      WIRE_FIELD(min_count),
      WIRE_FIELD(max_count),
      WIRE_FIELD(unlocked),
      WIRE_FIELD(recent_cutoff)
    );
  }

  template<typename F, typename T>
  void output_histogram_response_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD_ARRAY(histogram, max_histograms));
  }
}

void GetOutputHistogram::Request::write_bytes(wire::writer& dest) const
{
  output_histogram_request_map(dest, *this);
}

void GetOutputHistogram::Request::read_bytes(wire::reader& source)
{
  output_histogram_request_map(source, *this);
}

void GetOutputHistogram::Response::write_bytes(wire::writer& dest) const
{
  output_histogram_response_map(dest, *this);
}

void GetOutputHistogram::Response::read_bytes(wire::reader& source)
{
  output_histogram_response_map(source, *this);
}

namespace
{
  template<typename F, typename T>
  void output_keys_request_map(F& format, T& self)
  {
    using max_outputs = wire::max_element_count<32768>;
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD_ARRAY(outputs, max_outputs));
  }

  template<typename F, typename T>
  void output_keys_response_map(F& format, T& self)
  {
    using min_size = wire::min_element_sizeof<crypto::public_key, rct::key>;
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD_ARRAY(keys, min_size));
  }
}

void GetOutputKeys::Request::write_bytes(wire::writer& dest) const
{
  output_keys_request_map(dest, *this);
}

void GetOutputKeys::Request::read_bytes(wire::reader& source)
{
  output_keys_request_map(source, *this);
}

void GetOutputKeys::Response::write_bytes(wire::writer& dest) const
{
  output_keys_response_map(dest, *this);
}

void GetOutputKeys::Response::read_bytes(wire::reader& source)
{
  output_keys_response_map(source, *this);
}

namespace
{
  template<typename F, typename T>
  void rpc_version_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version));
  }

  template<typename F, typename T>
  void rpc_version_response_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(version));
  }
}

void GetRPCVersion::Request::write_bytes(wire::writer& dest) const
{
  rpc_version_request_map(dest, *this);
}

void GetRPCVersion::Request::read_bytes(wire::reader& source)
{
  rpc_version_request_map(source, *this);
}

void GetRPCVersion::Response::write_bytes(wire::writer& dest) const
{
  rpc_version_response_map(dest, *this);
}

void GetRPCVersion::Response::read_bytes(wire::reader& source)
{
  rpc_version_response_map(source, *this);
}

namespace
{
  template<typename F, typename T>
  void fee_estimate_request_map(F& format, T& self)
  {
    wire::object(format, WIRE_FIELD(rpc_version), WIRE_FIELD(num_grace_blocks));
  }

  template<typename F, typename T>
  void fee_estimate_response_map(F& format, T& self)
  {
    wire::object(format,
      WIRE_FIELD(rpc_version),
      WIRE_FIELD(estimated_base_fee),
      WIRE_FIELD(fee_mask),
      WIRE_FIELD(size_scale),
      WIRE_FIELD(hard_fork_version)
    );
  }
}

void GetFeeEstimate::Request::write_bytes(wire::writer& dest) const
{
  fee_estimate_request_map(dest, *this);
}

void GetFeeEstimate::Request::read_bytes(wire::reader& source)
{
  fee_estimate_request_map(source, *this);
}

void GetFeeEstimate::Response::write_bytes(wire::writer& dest) const
{
  fee_estimate_response_map(dest, *this);
}

void GetFeeEstimate::Response::read_bytes(wire::reader& source)
{
  fee_estimate_response_map(source, *this);
}

namespace
{
  using max_distributions = wire::max_element_count<1024>;

  template<typename F, typename T>
  void output_distribution_request_map(F& format, T& self)
  {
    wire::object(format,
      WIRE_FIELD(rpc_version),
      WIRE_FIELD_ARRAY(amounts, max_distributions),
      WIRE_FIELD(from_height),
      WIRE_FIELD(to_height),
      WIRE_FIELD(cumulative)
    );
  }

  template<typename F, typename T>
  void output_distribution_response_map(F& format, T& self)
  {
    wire::object(format,
      WIRE_FIELD(rpc_version),
      WIRE_FIELD(status),
      WIRE_FIELD_ARRAY(distributions, max_distributions)
    );
  }
}

void GetOutputDistribution::Request::write_bytes(wire::writer& dest) const
{
  output_distribution_request_map(dest, *this);
}

void GetOutputDistribution::Request::read_bytes(wire::reader& source)
{
  output_distribution_request_map(source, *this);
}

void GetOutputDistribution::Response::write_bytes(wire::writer& dest) const
{
  output_distribution_response_map(dest, *this);
}

void GetOutputDistribution::Response::read_bytes(wire::reader& source)
{
  output_distribution_response_map(source, *this);
}

}  // namespace rpc

}  // namespace cryptonote
