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

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <boost/iostreams/copy.hpp>
#include <atomic>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include "common/command_line.h"
#include "version.h"
#include "blockchain_export.h"
#include "cryptonote_core/cryptonote_boost_serialization.h"

#include "import.h"

static int max_chunk = 0;
static size_t height;

namespace po = boost::program_options;

using namespace cryptonote;
using namespace epee;

bool BlockchainExport::open(const boost::filesystem::path& dir_path)
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

  std::string file_path = (dir_path / BLOCKCHAIN_RAW).string();
  m_raw_data_file = new std::ofstream();
  m_raw_data_file->open(file_path , std::ios_base::binary | std::ios_base::out| std::ios::trunc);
  if (m_raw_data_file->fail())
    return false;

  m_output_stream = new boost::iostreams::stream<boost::iostreams::back_insert_device<buffer_type>>(m_buffer);
  m_raw_archive = new boost::archive::binary_oarchive(*m_output_stream);
  if (m_raw_archive == NULL)
    return false;

  return true;
}

void BlockchainExport::flush_chunk()
{
  m_output_stream->flush();
  char buffer[STR_LENGTH_OF_INT + 1];
  int chunk_size = (int) m_buffer.size();
  if (chunk_size > BUFFER_SIZE)
  {
    LOG_PRINT_L0("WARNING: chunk_size " << chunk_size << " > BUFFER_SIZE " << BUFFER_SIZE);
  }
  sprintf(buffer, STR_FORMAT_OF_INT, chunk_size);
  m_raw_data_file->write(buffer, STR_LENGTH_OF_INT);
  if (max_chunk < chunk_size)
  {
    max_chunk = chunk_size;
  }
  long pos_before = m_raw_data_file->tellp();
  std::copy(m_buffer.begin(), m_buffer.end(), std::ostreambuf_iterator<char>(*m_raw_data_file));
  m_raw_data_file->flush();
  long pos_after = m_raw_data_file->tellp();
  long num_chars_written = pos_after - pos_before;
  if ((int) num_chars_written != chunk_size)
  {
    LOG_PRINT_RED_L0("INTERNAL ERROR: num chars wrote NEQ buffer size. height = " << height);
  }

  m_buffer.clear();
  delete m_raw_archive;
  delete m_output_stream;
  m_output_stream = new boost::iostreams::stream<boost::iostreams::back_insert_device<buffer_type>>(m_buffer);
  m_raw_archive = new boost::archive::binary_oarchive(*m_output_stream);
}

void BlockchainExport::serialize_block_to_text_buffer(const block& block)
{
  *m_raw_archive << block;
}

void BlockchainExport::buffer_serialize_tx(const transaction& tx)
{
  *m_raw_archive << tx;
}

void BlockchainExport::buffer_write_num_txs(const std::list<transaction> txs)
{
  int n = txs.size();
  *m_raw_archive << n;
}

void BlockchainExport::write_block(block& block)
{
  serialize_block_to_text_buffer(block);
  std::list<transaction> txs;

  uint64_t block_height = boost::get<txin_gen>(block.miner_tx.vin.front()).height;

  // put coinbase transaction first
  transaction coinbase_tx = block.miner_tx;
  crypto::hash coinbase_tx_hash = get_transaction_hash(coinbase_tx);
#if SOURCE_DB == DB_MEMORY
  const transaction* cb_tx_full = m_blockchain_storage->get_tx(coinbase_tx_hash);
#else
  transaction cb_tx_full = m_blockchain_storage->get_db().get_tx(coinbase_tx_hash);
#endif

#if SOURCE_DB == DB_MEMORY
  if (cb_tx_full != NULL)
  {
    txs.push_back(*cb_tx_full);
  }
#else
  // TODO: should check and abort if cb_tx_full equals null_hash?
  txs.push_back(cb_tx_full);
#endif

  // now add all regular transactions
  BOOST_FOREACH(const auto& tx_id, block.tx_hashes)
  {
#if SOURCE_DB == DB_MEMORY
    const transaction* tx = m_blockchain_storage->get_tx(tx_id);
#else
    transaction tx = m_blockchain_storage->get_db().get_tx(tx_id);
#endif

#if SOURCE_DB == DB_MEMORY
    if(tx == NULL)
    {
      if (! m_tx_pool)
        throw std::runtime_error("Aborting: tx == NULL, so memory pool required to get tx, but memory pool isn't enabled");
      else
      {
        transaction tx;
        if(m_tx_pool->get_transaction(tx_id, tx))
          txs.push_back(tx);
        else
          throw std::runtime_error("Aborting: tx not found in pool");
      }
    }
    else
      txs.push_back(*tx);
#else
    txs.push_back(tx);
#endif
  }

  // serialize all txs to the persistant storage
  buffer_write_num_txs(txs);
  BOOST_FOREACH(const auto& tx, txs)
  {
    buffer_serialize_tx(tx);
  }

  // These three attributes are currently necessary for a fast import that adds blocks without verification.
  bool include_extra_block_data = true;
  if (include_extra_block_data)
  {
#if SOURCE_DB == DB_MEMORY
    size_t block_size = m_blockchain_storage->get_block_size(block_height);
#else
    size_t block_size = m_blockchain_storage->get_db().get_block_size(block_height);
#endif
#if SOURCE_DB == DB_MEMORY
    difficulty_type cumulative_difficulty = m_blockchain_storage->get_block_cumulative_difficulty(block_height);
#else
    difficulty_type cumulative_difficulty = m_blockchain_storage->get_db().get_block_cumulative_difficulty(block_height);
#endif
#if SOURCE_DB == DB_MEMORY
    uint64_t coins_generated = m_blockchain_storage->get_block_coins_generated(block_height);
#else
    // TODO TEST to verify that this is the equivalent. make sure no off-by-one error with block height vs block number
    uint64_t coins_generated = m_blockchain_storage->get_db().get_block_already_generated_coins(block_height);
#endif

    *m_raw_archive << block_size;
    *m_raw_archive << cumulative_difficulty;
    *m_raw_archive << coins_generated;
  }
}

bool BlockchainExport::BlockchainExport::close()
{
  if (m_raw_data_file->fail())
    return false;

  m_raw_data_file->flush();
  delete m_raw_archive;
  delete m_output_stream;
  delete m_raw_data_file;
  return true;
}


#if SOURCE_DB == DB_MEMORY
bool BlockchainExport::store_blockchain_raw(blockchain_storage* _blockchain_storage, tx_memory_pool* _tx_pool, boost::filesystem::path& output_dir, uint64_t requested_block_height)
#else
bool BlockchainExport::store_blockchain_raw(Blockchain* _blockchain_storage, tx_memory_pool* _tx_pool, boost::filesystem::path& output_dir, uint64_t requested_block_height)
#endif
{
  uint64_t block_height = 0;
  m_blockchain_storage = _blockchain_storage;
  m_tx_pool = _tx_pool;
  uint64_t progress_interval = 100;
  std::string refresh_string = "\r                                    \r";
  LOG_PRINT_L0("Storing blocks raw data...");
  if (!BlockchainExport::open(output_dir))
  {
    LOG_PRINT_RED_L0("failed to open raw file for write");
    return false;
  }
  block b;
  LOG_PRINT_L0("source blockchain height: " <<  m_blockchain_storage->get_current_blockchain_height());
  LOG_PRINT_L0("requested block height: " << requested_block_height);
  if ((requested_block_height > 0) && (requested_block_height < m_blockchain_storage->get_current_blockchain_height()))
    block_height = requested_block_height;
  else
  {
    block_height = m_blockchain_storage->get_current_blockchain_height();
    LOG_PRINT_L0("Using block height of source blockchain: " << block_height);
  }
  for (height=0; height < block_height; ++height)
  {
    crypto::hash hash = m_blockchain_storage->get_block_id_by_height(height);
    m_blockchain_storage->get_block_by_hash(hash, b);
    write_block(b);
    if (height % NUM_BLOCKS_PER_CHUNK == 0) {
      flush_chunk();
    }
    if (height % progress_interval == 0) {
      std::cout << refresh_string;
      std::cout << "height " << height << "/" << block_height << std::flush;
    }
  }
  if (height % NUM_BLOCKS_PER_CHUNK != 0)
  {
    flush_chunk();
  }
  std::cout << refresh_string;
  std::cout << "height " << height << "/" << block_height << ENDL;

  LOG_PRINT_L0("longest chunk was " << max_chunk << " bytes");
  return BlockchainExport::close();
}


int main(int argc, char* argv[])
{
  uint32_t log_level = 0;
  uint64_t block_height = 0;
  std::string import_filename = BLOCKCHAIN_RAW;

  boost::filesystem::path default_data_path {tools::get_default_data_dir()};
  boost::filesystem::path default_testnet_data_path {default_data_path / "testnet"};

  po::options_description desc_cmd_only("Command line options");
  po::options_description desc_cmd_sett("Command line options and settings options");
  const command_line::arg_descriptor<uint32_t> arg_log_level   =  {"log-level",  "", log_level};
  const command_line::arg_descriptor<uint64_t> arg_block_height =  {"block-number", "", block_height};
  const command_line::arg_descriptor<bool>     arg_testnet_on  = {
    "testnet"
      , "Run on testnet."
      , false
  };


  command_line::add_arg(desc_cmd_sett, command_line::arg_data_dir, default_data_path.string());
  command_line::add_arg(desc_cmd_sett, command_line::arg_testnet_data_dir, default_testnet_data_path.string());
  command_line::add_arg(desc_cmd_sett, arg_log_level);
  command_line::add_arg(desc_cmd_sett, arg_block_height);
  command_line::add_arg(desc_cmd_sett, arg_testnet_on);

  command_line::add_arg(desc_cmd_only, command_line::arg_help);

  po::options_description desc_options("Allowed options");
  desc_options.add(desc_cmd_only).add(desc_cmd_sett);

  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_options, [&]()
  {
    po::store(po::parse_command_line(argc, argv, desc_options), vm);
    po::notify(vm);
    return true;
  });
  if (! r)
    return 1;

  if (command_line::get_arg(vm, command_line::arg_help))
  {
    std::cout << CRYPTONOTE_NAME << " v" << MONERO_VERSION_FULL << ENDL << ENDL;
    std::cout << desc_options << std::endl;
    return 1;
  }

  log_level    = command_line::get_arg(vm, arg_log_level);
  block_height = command_line::get_arg(vm, arg_block_height);

  log_space::get_set_log_detalisation_level(true, log_level);
  log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
  LOG_PRINT_L0("Starting...");
  LOG_PRINT_L0("Setting log level = " << log_level);

  bool opt_testnet = command_line::get_arg(vm, arg_testnet_on);

  std::string m_config_folder;

  auto data_dir_arg = opt_testnet ? command_line::arg_testnet_data_dir : command_line::arg_data_dir;
  m_config_folder = command_line::get_arg(vm, data_dir_arg);
  boost::filesystem::path output_dir {m_config_folder};
  output_dir /= "export";
  LOG_PRINT_L0("Export directory: " << output_dir.string());

  // If we wanted to use the memory pool, we would set up a fake_core.

#if SOURCE_DB == DB_MEMORY
  // blockchain_storage* core_storage = NULL;
  // tx_memory_pool m_mempool(*core_storage); // is this fake anyway? just passing in NULL! so m_mempool can't be used anyway, right?
  // core_storage = new blockchain_storage(&m_mempool);

  blockchain_storage* core_storage = new blockchain_storage(NULL);
  LOG_PRINT_L0("Initializing source blockchain (in-memory database)");
  r = core_storage->init(m_config_folder, opt_testnet);
#else
  // Use Blockchain instead of lower-level BlockchainDB for two reasons:
  // 1. Blockchain has the init() method for easy setup
  // 2. exporter needs to use get_current_blockchain_height(), get_block_id_by_height(), get_block_by_hash()
  //
  // cannot match blockchain_storage setup above with just one line,
  // e.g.
  //   Blockchain* core_storage = new Blockchain(NULL);
  // because unlike blockchain_storage constructor, which takes a pointer to
  // tx_memory_pool, Blockchain's constructor takes tx_memory_pool object.
  LOG_PRINT_L0("Initializing source blockchain (BlockchainDB)");
  Blockchain* core_storage = NULL;
  tx_memory_pool m_mempool(*core_storage);
  core_storage = new Blockchain(m_mempool);
  r = core_storage->init(m_config_folder, opt_testnet);
#endif

  CHECK_AND_ASSERT_MES(r, false, "Failed to initialize source blockchain storage");
  LOG_PRINT_L0("Source blockchain storage initialized OK");
  LOG_PRINT_L0("Exporting blockchain raw data...");

  BlockchainExport be;
  r = be.store_blockchain_raw(core_storage, NULL, output_dir, block_height);
  CHECK_AND_ASSERT_MES(r, false, "Failed to export blockchain raw data");
  LOG_PRINT_L0("Blockchain raw data exported OK");
}
