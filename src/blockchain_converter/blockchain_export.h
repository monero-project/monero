// Copyright (c) 2014-2015, The Monero Project
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

#pragma once

#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include "cryptonote_core/cryptonote_basic.h"
#include "cryptonote_core/blockchain_storage.h"
#include "cryptonote_core/blockchain.h"
#include "blockchain_db/blockchain_db.h"
#include "blockchain_db/lmdb/db_lmdb.h"

// CONFIG: choose one of the three #define's
//
// DB_MEMORY is a sensible default for users migrating to LMDB, as it allows
// the exporter to use the in-memory blockchain while the other binaries
// work with LMDB, without recompiling anything.
//
#define SOURCE_DB DB_MEMORY
// #define SOURCE_DB DB_LMDB
// to use global compile-time setting (DB_MEMORY or DB_LMDB):
// #define SOURCE_DB BLOCKCHAIN_DB

using namespace cryptonote;

class BlockchainExport
{
public:
#if SOURCE_DB == DB_MEMORY
  bool store_blockchain_raw(cryptonote::blockchain_storage* cs, cryptonote::tx_memory_pool* txp,
      boost::filesystem::path& output_dir, uint64_t use_block_height=0);
#else
  bool store_blockchain_raw(cryptonote::Blockchain* cs, cryptonote::tx_memory_pool* txp,
      boost::filesystem::path& output_dir, uint64_t use_block_height=0);
#endif

protected:
#if SOURCE_DB == DB_MEMORY
  blockchain_storage* m_blockchain_storage;
#else
  Blockchain* m_blockchain_storage;
#endif

  tx_memory_pool* m_tx_pool;
  typedef std::vector<char> buffer_type;
  std::ofstream * m_raw_data_file;
  boost::archive::binary_oarchive * m_raw_archive;
  buffer_type m_buffer;
  boost::iostreams::stream<boost::iostreams::back_insert_device<buffer_type>>* m_output_stream;

  // open export file for write
  bool open(const boost::filesystem::path& dir_path);
  bool close();
  void write_block(block& block);
  void serialize_block_to_text_buffer(const block& block);
  void buffer_serialize_tx(const transaction& tx);
  void buffer_write_num_txs(const std::list<transaction> txs);
  void flush_chunk();
};
