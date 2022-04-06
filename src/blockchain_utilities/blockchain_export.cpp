// Copyright (c) 2014-2022, The Monero Project
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

#include "bootstrap_file.h"
#include "blocksdat_file.h"
#include "common/command_line.h"
#include "cryptonote_core/tx_pool.h"
#include "cryptonote_core/cryptonote_core.h"
#include "blockchain_db/blockchain_db.h"
#include "version.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "bcutil"

namespace po = boost::program_options;
using namespace epee;

int main(int argc, char* argv[])
{
  TRY_ENTRY();

  epee::string_tools::set_module_name_and_folder(argv[0]);

  uint32_t log_level = 0;
  uint64_t block_start = 0;
  uint64_t block_stop = 0;
  bool blocks_dat = false;

  tools::on_startup();

  boost::filesystem::path output_file_path;

  po::options_description desc_cmd_only("Command line options");
  po::options_description desc_cmd_sett("Command line options and settings options");
  const command_line::arg_descriptor<std::string> arg_output_file = {"output-file", "Specify output file", "", true};
  const command_line::arg_descriptor<std::string> arg_log_level  = {"log-level",  "0-4 or categories", ""};
  const command_line::arg_descriptor<uint64_t> arg_block_start = {"block-start", "Start at block number", block_start};
  const command_line::arg_descriptor<uint64_t> arg_block_stop = {"block-stop", "Stop at block number", block_stop};
  const command_line::arg_descriptor<bool> arg_blocks_dat = {"blocksdat", "Output in blocks.dat format", blocks_dat};


  command_line::add_arg(desc_cmd_sett, cryptonote::arg_data_dir);
  command_line::add_arg(desc_cmd_sett, arg_output_file);
  command_line::add_arg(desc_cmd_sett, cryptonote::arg_testnet_on);
  command_line::add_arg(desc_cmd_sett, cryptonote::arg_stagenet_on);
  command_line::add_arg(desc_cmd_sett, arg_log_level);
  command_line::add_arg(desc_cmd_sett, arg_block_start);
  command_line::add_arg(desc_cmd_sett, arg_block_stop);
  command_line::add_arg(desc_cmd_sett, arg_blocks_dat);

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
    std::cout << "Monero '" << MONERO_RELEASE_NAME << "' (v" << MONERO_VERSION_FULL << ")" << ENDL << ENDL;
    std::cout << desc_options << std::endl;
    return 1;
  }

  mlog_configure(mlog_get_default_log_path("monero-blockchain-export.log"), true);
  if (!command_line::is_arg_defaulted(vm, arg_log_level))
    mlog_set_log(command_line::get_arg(vm, arg_log_level).c_str());
  else
    mlog_set_log(std::string(std::to_string(log_level) + ",bcutil:INFO").c_str());
  block_start = command_line::get_arg(vm, arg_block_start);
  block_stop = command_line::get_arg(vm, arg_block_stop);

  LOG_PRINT_L0("Starting...");

  bool opt_testnet = command_line::get_arg(vm, cryptonote::arg_testnet_on);
  bool opt_stagenet = command_line::get_arg(vm, cryptonote::arg_stagenet_on);
  if (opt_testnet && opt_stagenet)
  {
    std::cerr << "Can't specify more than one of --testnet and --stagenet" << std::endl;
    return 1;
  }
  bool opt_blocks_dat = command_line::get_arg(vm, arg_blocks_dat);

  std::string m_config_folder;

  m_config_folder = command_line::get_arg(vm, cryptonote::arg_data_dir);

  if (command_line::has_arg(vm, arg_output_file))
    output_file_path = boost::filesystem::path(command_line::get_arg(vm, arg_output_file));
  else
    output_file_path = boost::filesystem::path(m_config_folder) / "export" / BLOCKCHAIN_RAW;
  LOG_PRINT_L0("Export output file: " << output_file_path.string());

  // If we wanted to use the memory pool, we would set up a fake_core.

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

  BlockchainDB* db = new_db();
  if (db == NULL)
  {
    LOG_ERROR("Failed to initialize a database");
    throw std::runtime_error("Failed to initialize a database");
  }
  LOG_PRINT_L0("database: LMDB");

  boost::filesystem::path folder(m_config_folder);
  folder /= db->get_db_name();
  const std::string filename = folder.string();

  LOG_PRINT_L0("Loading blockchain from folder " << filename << " ...");
  try
  {
    db->open(filename, DBF_RDONLY);
  }
  catch (const std::exception& e)
  {
    LOG_PRINT_L0("Error opening database: " << e.what());
    return 1;
  }
  r = core_storage->init(db, opt_testnet ? cryptonote::TESTNET : opt_stagenet ? cryptonote::STAGENET : cryptonote::MAINNET);

  if (core_storage->get_blockchain_pruning_seed() && !opt_blocks_dat)
  {
    LOG_PRINT_L0("Blockchain is pruned, cannot export");
    return 1;
  }

  CHECK_AND_ASSERT_MES(r, 1, "Failed to initialize source blockchain storage");
  LOG_PRINT_L0("Source blockchain storage initialized OK");
  LOG_PRINT_L0("Exporting blockchain raw data...");

  if (opt_blocks_dat)
  {
    BlocksdatFile blocksdat;
    r = blocksdat.store_blockchain_raw(core_storage, NULL, output_file_path, block_stop);
  }
  else
  {
    BootstrapFile bootstrap;
    r = bootstrap.store_blockchain_raw(core_storage, NULL, output_file_path, block_start, block_stop);
  }
  CHECK_AND_ASSERT_MES(r, 1, "Failed to export blockchain raw data");
  LOG_PRINT_L0("Blockchain raw data exported OK");
  return 0;

  CATCH_ENTRY("Export error", 1);
}
