// Copyright (c) 2014-2016, The Monero Project
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

#include "blocksdat_file.h"


namespace po = boost::program_options;

using namespace cryptonote;
using namespace epee;

namespace
{
  std::string refresh_string = "\r                                    \r";
}



bool BlocksdatFile::open_writer(const boost::filesystem::path& file_path, uint64_t block_stop)
{
  const boost::filesystem::path dir_path = file_path.parent_path();
  if (!dir_path.empty())
  {
    if (boost::filesystem::exists(dir_path))
    {
      if (!boost::filesystem::is_directory(dir_path))
      {
        LOG_PRINT_RED_L0("export directory path is a file: " << dir_path);
        return false;
      }
    }
    else
    {
      if (!boost::filesystem::create_directory(dir_path))
      {
        LOG_PRINT_RED_L0("Failed to create directory " << dir_path);
        return false;
      }
    }
  }

  m_raw_data_file = new std::ofstream();

  LOG_PRINT_L0("creating file");

  m_raw_data_file->open(file_path.string(), std::ios_base::binary | std::ios_base::out | std::ios::trunc);
  if (m_raw_data_file->fail())
    return false;

  initialize_file(block_stop);

  return true;
}


bool BlocksdatFile::initialize_file(uint64_t block_stop)
{
  const uint32_t nblocks = block_stop + 1;
  unsigned char nblocksc[4];

  nblocksc[0] = nblocks & 0xff;
  nblocksc[1] = (nblocks >> 8) & 0xff;
  nblocksc[2] = (nblocks >> 16) & 0xff;
  nblocksc[3] = (nblocks >> 24) & 0xff;

  // 4 bytes little endian
  *m_raw_data_file << nblocksc[0];
  *m_raw_data_file << nblocksc[1];
  *m_raw_data_file << nblocksc[2];
  *m_raw_data_file << nblocksc[3];

  return true;
}

void BlocksdatFile::write_block(const crypto::hash& block_hash)
{
  const std::string data(block_hash.data, sizeof(block_hash));
  *m_raw_data_file << data;
}

bool BlocksdatFile::close()
{
  if (m_raw_data_file->fail())
    return false;

  m_raw_data_file->flush();
  delete m_raw_data_file;
  return true;
}


bool BlocksdatFile::store_blockchain_raw(Blockchain* _blockchain_storage, tx_memory_pool* _tx_pool, boost::filesystem::path& output_file, uint64_t requested_block_stop)
{
  uint64_t num_blocks_written = 0;
  m_blockchain_storage = _blockchain_storage;
  uint64_t progress_interval = 100;
  block b;

  uint64_t block_start = 0;
  uint64_t block_stop = 0;
  LOG_PRINT_L0("source blockchain height: " <<  m_blockchain_storage->get_current_blockchain_height()-1);
  if ((requested_block_stop > 0) && (requested_block_stop < m_blockchain_storage->get_current_blockchain_height()))
  {
    LOG_PRINT_L0("Using requested block height: " << requested_block_stop);
    block_stop = requested_block_stop;
  }
  else
  {
    block_stop = m_blockchain_storage->get_current_blockchain_height() - 1;
    LOG_PRINT_L0("Using block height of source blockchain: " << block_stop);
  }
  LOG_PRINT_L0("Storing blocks raw data...");
  if (!BlocksdatFile::open_writer(output_file, block_stop))
  {
    LOG_PRINT_RED_L0("failed to open raw file for write");
    return false;
  }
  for (m_cur_height = block_start; m_cur_height <= block_stop; ++m_cur_height)
  {
    // this method's height refers to 0-based height (genesis block = height 0)
    crypto::hash hash = m_blockchain_storage->get_block_id_by_height(m_cur_height);
    write_block(hash);
    if (m_cur_height % NUM_BLOCKS_PER_CHUNK == 0) {
      num_blocks_written += NUM_BLOCKS_PER_CHUNK;
    }
    if (m_cur_height % progress_interval == 0) {
      std::cout << refresh_string;
      std::cout << "block " << m_cur_height << "/" << block_stop << std::flush;
    }
  }
  // print message for last block, which may not have been printed yet due to progress_interval
  std::cout << refresh_string;
  std::cout << "block " << m_cur_height-1 << "/" << block_stop << ENDL;

  LOG_PRINT_L0("Number of blocks exported: " << num_blocks_written);

  return BlocksdatFile::close();
}

