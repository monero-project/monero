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

#include "include_base_utils.h"
#include "common/util.h"
#include "warnings.h"
#include "crypto/crypto.h"
#include "cryptonote_config.h"
#include "cryptonote_core/cryptonote_format_utils.h"
#include "misc_language.h"
#include "cryptonote_core/blockchain_storage.h"
#include "blockchain_db/blockchain_db.h"
#include "cryptonote_core/blockchain.h"
#include "blockchain_db/lmdb/db_lmdb.h"
#include "cryptonote_core/tx_pool.h"
#include "common/command_line.h"
#include "serialization/json_utils.h"
#include "include_base_utils.h"
#include "version.h"
#include <iostream>


unsigned int epee::g_test_dbg_lock_sleep = 0;

namespace
{

// CONFIG
bool opt_batch   = true;
bool opt_resume  = true;
bool opt_testnet = false;

// number of blocks per batch transaction
// adjustable through command-line argument according to available RAM
uint64_t db_batch_size = 20000;

}

namespace po = boost::program_options;

using namespace cryptonote;
using namespace epee;

struct fake_core
{
  Blockchain dummy;
  tx_memory_pool m_pool;

  blockchain_storage m_storage;

#if !defined(BLOCKCHAIN_DB)
  // for multi_db_runtime:
  fake_core(const boost::filesystem::path &path, const bool use_testnet) : dummy(m_pool), m_pool(&dummy), m_storage(m_pool)
#else
  // for multi_db_compile:
  fake_core(const boost::filesystem::path &path, const bool use_testnet) : dummy(m_pool), m_pool(dummy), m_storage(&m_pool)
#endif
  {
    m_pool.init(path.string());
    m_storage.init(path.string(), use_testnet);
  }
};

int main(int argc, char* argv[])
{
  uint64_t height = 0;
  uint64_t start_block = 0;
  uint64_t end_block = 0;
  uint64_t num_blocks = 0;

  boost::filesystem::path default_data_path {tools::get_default_data_dir()};
  boost::filesystem::path default_testnet_data_path {default_data_path / "testnet"};


  po::options_description desc_cmd_only("Command line options");
  po::options_description desc_cmd_sett("Command line options and settings options");
  const command_line::arg_descriptor<uint32_t> arg_log_level   =  {"log-level", "", LOG_LEVEL_0};
  const command_line::arg_descriptor<uint64_t> arg_batch_size  =  {"batch-size", "", db_batch_size};
  const command_line::arg_descriptor<bool>     arg_testnet_on  = {
    "testnet"
      , "Run on testnet."
      , opt_testnet
  };
  const command_line::arg_descriptor<uint64_t> arg_block_number =
  {"block-number", "Number of blocks (default: use entire source blockchain)",
    0};

  command_line::add_arg(desc_cmd_sett, command_line::arg_data_dir, default_data_path.string());
  command_line::add_arg(desc_cmd_sett, command_line::arg_testnet_data_dir, default_testnet_data_path.string());
  command_line::add_arg(desc_cmd_sett, arg_log_level);
  command_line::add_arg(desc_cmd_sett, arg_batch_size);
  command_line::add_arg(desc_cmd_sett, arg_testnet_on);
  command_line::add_arg(desc_cmd_sett, arg_block_number);

  command_line::add_arg(desc_cmd_only, command_line::arg_help);

  const command_line::arg_descriptor<bool> arg_batch  =  {"batch",
    "Batch transactions for faster import", true};
  const command_line::arg_descriptor<bool> arg_resume =  {"resume",
        "Resume from current height if output database already exists", true};

  // call add_options() directly for these arguments since command_line helpers
  // support only boolean switch, not boolean argument
  desc_cmd_sett.add_options()
    (arg_batch.name,  make_semantic(arg_batch),  arg_batch.description)
    (arg_resume.name, make_semantic(arg_resume), arg_resume.description)
    ;

  po::options_description desc_options("Allowed options");
  desc_options.add(desc_cmd_only).add(desc_cmd_sett);


  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_options, [&]()
  {
    po::store(po::parse_command_line(argc, argv, desc_options), vm);
    po::notify(vm);

    return true;
  });
  if (!r)
    return 1;

  int log_level = command_line::get_arg(vm, arg_log_level);
  opt_batch     = command_line::get_arg(vm, arg_batch);
  opt_resume    = command_line::get_arg(vm, arg_resume);
  db_batch_size = command_line::get_arg(vm, arg_batch_size);

  if (command_line::get_arg(vm, command_line::arg_help))
  {
    std::cout << CRYPTONOTE_NAME << " v" << MONERO_VERSION_FULL << ENDL << ENDL;
    std::cout << desc_options << std::endl;
    return 1;
  }

  if (! opt_batch && ! vm["batch-size"].defaulted())
  {
    std::cerr << "Error: batch-size set, but batch option not enabled" << ENDL;
    return 1;
  }
  if (! db_batch_size)
  {
    std::cerr << "Error: batch-size must be > 0" << ENDL;
    return 1;
  }

  log_space::get_set_log_detalisation_level(true, log_level);
  log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
  LOG_PRINT_L0("Starting...");

  std::string src_folder;
  opt_testnet = command_line::get_arg(vm, arg_testnet_on);
  auto data_dir_arg = opt_testnet ? command_line::arg_testnet_data_dir : command_line::arg_data_dir;
  src_folder = command_line::get_arg(vm, data_dir_arg);
  boost::filesystem::path dest_folder(src_folder);

  num_blocks = command_line::get_arg(vm, arg_block_number);

  if (opt_batch)
  {
    LOG_PRINT_L0("batch:   " << std::boolalpha << opt_batch << std::noboolalpha
        << "  batch size: " << db_batch_size);
  }
  else
  {
    LOG_PRINT_L0("batch:   " << std::boolalpha << opt_batch << std::noboolalpha);
  }
  LOG_PRINT_L0("resume:  " << std::boolalpha << opt_resume  << std::noboolalpha);
  LOG_PRINT_L0("testnet: " << std::boolalpha << opt_testnet << std::noboolalpha);

  fake_core c(src_folder, opt_testnet);

  height = c.m_storage.get_current_blockchain_height();

  BlockchainDB *blockchain;
  blockchain = new BlockchainLMDB(opt_batch);
  dest_folder /= blockchain->get_db_name();
  LOG_PRINT_L0("Source blockchain: " << src_folder);
  LOG_PRINT_L0("Dest blockchain:   " << dest_folder.string());
  LOG_PRINT_L0("Opening dest blockchain (BlockchainDB " << blockchain->get_db_name() << ")");
  blockchain->open(dest_folder.string());
  LOG_PRINT_L0("Source blockchain height: " << height);
  LOG_PRINT_L0("Dest blockchain height:   " << blockchain->height());

  if (opt_resume)
    // next block number to add is same as current height
    start_block = blockchain->height();

  if (! num_blocks || (start_block + num_blocks > height))
    end_block = height - 1;
  else
    end_block = start_block + num_blocks - 1;

  LOG_PRINT_L0("start height: " << start_block+1 << "  stop height: " <<
            end_block+1);

  if (start_block > end_block)
  {
    LOG_PRINT_L0("Finished: no blocks to add");
    delete blockchain;
    return 0;
  }

  if (opt_batch)
    blockchain->batch_start();
  uint64_t i = 0;
  for (i = start_block; i < end_block + 1; ++i)
  {
    // block: i  height: i+1  end height: end_block + 1
    if ((i+1) % 10 == 0)
    {
      std::cout << "\r                   \r" << "height " << i+1 << "/" <<
        end_block+1 << " (" << (i+1)*100/(end_block+1)<< "%)" << std::flush;
    }
    // for debugging:
    // std::cout << "height " << i+1 << "/" << end_block+1
    //   << " ((" << i+1 << ")*100/(end_block+1))" << "%)" << ENDL;

    block b = c.m_storage.get_block(i);
    size_t bsize = c.m_storage.get_block_size(i);
    difficulty_type bdiff = c.m_storage.get_block_cumulative_difficulty(i);
    uint64_t bcoins = c.m_storage.get_block_coins_generated(i);
    std::vector<transaction> txs;
    std::vector<crypto::hash> missed;

    c.m_storage.get_transactions(b.tx_hashes, txs, missed);
    if (missed.size())
    {
      std::cout << ENDL;
      std::cerr << "Missed transaction(s) for block at height " << i + 1 << ", exiting" << ENDL;
      delete blockchain;
      return 1;
    }

    try
    {
      blockchain->add_block(b, bsize, bdiff, bcoins, txs);

      if (opt_batch)
      {
        if ((i < end_block) && ((i + 1) % db_batch_size == 0))
        {
          std::cout << "\r                   \r";
          std::cout << "[- batch commit at height " << i + 1 << " -]" << ENDL;
          blockchain->batch_stop();
          blockchain->batch_start();
          std::cout << ENDL;
          blockchain->show_stats();
        }
      }
    }
    catch (const std::exception& e)
    {
      std::cout << ENDL;
      std::cerr << "Error adding block " << i << " to new blockchain: " << e.what() << ENDL;
      delete blockchain;
      return 2;
    }
  }
  if (opt_batch)
  {
    std::cout << "\r                   \r" << "height " << i << "/" <<
      end_block+1 << " (" << (i)*100/(end_block+1)<< "%)" << std::flush;
    std::cout << ENDL;
    std::cout << "[- batch commit at height " << i << " -]" << ENDL;
    blockchain->batch_stop();
  }
  std::cout << ENDL;
  blockchain->show_stats();
  std::cout << "Finished at height: " << i << "  block: " << i-1 << ENDL;

  delete blockchain;
  return 0;
}
