// Copyright (c) 2014-2025, The Monero Project
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

#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_core/blockchain.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <atomic>

#include "common/command_line.h"
#include "version.h"

#include "blockchain_utilities.h"

class BootstrapFile
{
public:

  uint64_t count_bytes(std::ifstream& import_file, uint64_t blocks, uint64_t& h, bool& quit);
  uint64_t count_blocks(const std::string& dir_path, std::streampos& start_pos, uint64_t& seek_height, uint64_t& block_first);
  uint64_t count_blocks(const std::string& dir_path);
  uint64_t seek_to_first_chunk(std::ifstream& import_file, uint8_t &major_version, uint8_t &minor_version, uint64_t &block_first, uint64_t &block_last);

  bool store_blockchain_raw(cryptonote::Blockchain* cs, cryptonote::tx_memory_pool* txp,
      boost::filesystem::path& output_file, uint64_t start_block=0, uint64_t stop_block=0);

protected:

  cryptonote::Blockchain* m_blockchain_storage;

  cryptonote::tx_memory_pool* m_tx_pool;
  typedef std::vector<char> buffer_type;
  std::ofstream * m_raw_data_file;
  buffer_type m_buffer;
  boost::iostreams::stream<boost::iostreams::back_insert_device<buffer_type>>* m_output_stream;

  // open export file for write
  bool open_writer(const boost::filesystem::path& file_path, uint64_t start_block, uint64_t stop_block);
  bool initialize_file(uint64_t start_block, uint64_t stop_block);
  bool close();
  void write_block(cryptonote::block& block);
  void flush_chunk();

private:

  uint64_t m_height;
  uint64_t m_cur_height; // tracks current height during export
  uint32_t m_max_chunk;
};
