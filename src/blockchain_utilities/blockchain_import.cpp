// Copyright (c) 2014-2023, The Monero Project
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

#include <atomic>
#include <cstdio>
#include <algorithm>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <unistd.h>
#include "misc_log_ex.h"
#include "bootstrap_file.h"
#include "bootstrap_serialization.h"
#include "blocks/blocks.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "serialization/binary_utils.h" // dump_binary(), parse_binary()
#include "include_base_utils.h"
#include "cryptonote_core/cryptonote_core.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "bcutil"

namespace
{
// CONFIG
bool opt_batch   = true;
bool opt_verify  = true; // use add_new_block, which does verification before calling add_block
bool opt_resume  = true;
bool opt_testnet = true;
bool opt_stagenet = true;

// number of blocks per batch transaction
// adjustable through command-line argument according to available RAM
#if ARCH_WIDTH != 32
uint64_t db_batch_size = 20000;
#else
// set a lower default batch size, pending possible LMDB issue with large transaction size
uint64_t db_batch_size = 100;
#endif

// when verifying, use a smaller default batch size so progress is more
// frequently saved
uint64_t db_batch_size_verify = 5000;

std::string refresh_string = "\r                                    \r";
}



namespace po = boost::program_options;

using namespace cryptonote;
using namespace epee;

// db_mode: safe, fast, fastest
int get_db_flags_from_mode(const std::string& db_mode)
{
  int db_flags = 0;
  if (db_mode == "safe")
    db_flags = DBF_SAFE;
  else if (db_mode == "fast")
    db_flags = DBF_FAST;
  else if (db_mode == "fastest")
    db_flags = DBF_FASTEST;
  return db_flags;
}

int pop_blocks(cryptonote::core& core, int num_blocks)
{
  bool use_batch = opt_batch;

  if (use_batch)
    core.get_blockchain_storage().get_db().batch_start();

  int quit = 0;
  block popped_block;
  std::vector<transaction> popped_txs;
  for (int i=0; i < num_blocks; ++i)
  {
    // simple_core.m_storage.pop_block_from_blockchain() is private, so call directly through db
    core.get_blockchain_storage().get_db().pop_block(popped_block, popped_txs);
    quit = 1;
  }


  if (use_batch)
  {
    if (quit > 1)
    {
      // There was an error, so don't commit pending data.
      // Destructor will abort write txn.
    }
    else
    {
      core.get_blockchain_storage().get_db().batch_stop();
    }
    core.get_blockchain_storage().get_db().show_stats();
  }

  return num_blocks;
}

int check_flush(cryptonote::core &core, std::vector<block_complete_entry> &blocks, bool force)
{
  if (blocks.empty())
    return 0;
  if (!force && blocks.size() < db_batch_size)
    return 0;

  // wait till we can verify a full HOH without extra, for speed
  uint64_t new_height = core.get_blockchain_storage().get_db().height() + blocks.size();
  if (!force && new_height % HASH_OF_HASHES_STEP)
    return 0;

  std::vector<crypto::hash> hashes;
  for (const auto &b: blocks)
  {
    cryptonote::block block;
    if (!parse_and_validate_block_from_blob(b.block, block))
    {
      MERROR("Failed to parse block: "
          << epee::string_tools::buff_to_hex_nodelimer(b.block));
      core.cleanup_handle_incoming_blocks();
      return 1;
    }
    hashes.push_back(cryptonote::get_block_hash(block));
  }
  core.prevalidate_block_hashes(core.get_blockchain_storage().get_db().height(), hashes, {});

  std::vector<block> pblocks;
  if (!core.prepare_handle_incoming_blocks(blocks, pblocks))
  {
    MERROR("Failed to prepare to add blocks");
    return 1;
  }
  if (!pblocks.empty() && pblocks.size() != blocks.size())
  {
    MERROR("Unexpected parsed blocks size");
    core.cleanup_handle_incoming_blocks();
    return 1;
  }

  size_t blockidx = 0;
  for(const block_complete_entry& block_entry: blocks)
  {
    // process transactions
    for(auto& tx_blob: block_entry.txs)
    {
      tx_verification_context tvc = AUTO_VAL_INIT(tvc);
      CHECK_AND_ASSERT_THROW_MES(tx_blob.prunable_hash == crypto::null_hash,
        "block entry must not contain pruned txs");
      core.handle_incoming_tx(tx_blob.blob, tvc, relay_method::block, true);
      if(tvc.m_verifivation_failed)
      {
        cryptonote::transaction transaction;
        if (cryptonote::parse_and_validate_tx_from_blob(tx_blob.blob, transaction))
          MERROR("Transaction verification failed, tx_id = " << cryptonote::get_transaction_hash(transaction));
        else
          MERROR("Transaction verification failed, transaction is unparsable");
        core.cleanup_handle_incoming_blocks();
        return 1;
      }
    }

    // process block

    block_verification_context bvc = {};
    pool_supplement ps{};

    core.handle_incoming_block(block_entry.block, pblocks.empty() ? NULL : &pblocks[blockidx++], bvc, ps); // <--- process block

    if(bvc.m_verifivation_failed)
    {
      cryptonote::block block;
      if (cryptonote::parse_and_validate_block_from_blob(block_entry.block, block))
        MERROR("Block verification failed, id = " << cryptonote::get_block_hash(block));
      else
        MERROR("Block verification failed, block is unparsable");
      core.cleanup_handle_incoming_blocks();
      return 1;
    }
    if(bvc.m_marked_as_orphaned)
    {
      MERROR("Block received at sync phase was marked as orphaned");
      core.cleanup_handle_incoming_blocks();
      return 1;
    }

  } // each download block
  if (!core.cleanup_handle_incoming_blocks())
    return 1;

  blocks.clear();
  return 0;
}

int import_from_file(cryptonote::core& core, const std::string& import_file_path, uint64_t block_stop=0)
{
  // Reset stats, in case we're using newly created db, accumulating stats
  // from addition of genesis block.
  // This aligns internal db counts with importer counts.
  core.get_blockchain_storage().get_db().reset_stats();

  boost::filesystem::path fs_import_file_path(import_file_path);
  boost::system::error_code ec;
  if (!boost::filesystem::exists(fs_import_file_path, ec))
  {
    MFATAL("bootstrap file not found: " << fs_import_file_path);
    return false;
  }

  uint64_t block_first;
  uint64_t start_height = 1, seek_height;
  if (opt_resume)
    start_height = core.get_blockchain_storage().get_current_blockchain_height();

  seek_height = start_height;
  BootstrapFile bootstrap;
  std::streampos pos;
  // BootstrapFile bootstrap(import_file_path);
  uint64_t total_source_blocks = bootstrap.count_blocks(import_file_path, pos, seek_height, block_first);
  MINFO("bootstrap file last block number: " << total_source_blocks+block_first-1 << " (zero-based height)  total blocks: " << total_source_blocks);

  if (total_source_blocks+block_first-1 <= start_height)
  {
    return false;
  }

  std::cout << ENDL;
  std::cout << "Preparing to read blocks..." << ENDL;
  std::cout << ENDL;

  std::ifstream import_file;
  import_file.open(import_file_path, std::ios_base::binary | std::ifstream::in);

  uint64_t h = 0;
  uint64_t num_imported = 0;
  if (import_file.fail())
  {
    MFATAL("import_file.open() fail");
    return false;
  }

  // 4 byte magic + (currently) 1024 byte header structures
  uint8_t major_version, minor_version;
  uint64_t dummy;
  bootstrap.seek_to_first_chunk(import_file, major_version, minor_version, dummy, dummy);

  std::string str1;
  char buffer1[1024];
  char buffer_block[BUFFER_SIZE];
  block b;
  transaction tx;
  int quit = 0;
  uint64_t bytes_read;

  // Note that a new blockchain will start with block number 0 (total blocks: 1)
  // due to genesis block being added at initialization.

  if (! block_stop)
  {
    block_stop = total_source_blocks+block_first - 1;
  }

  // These are what we'll try to use, and they don't have to be a determination
  // from source and destination blockchains, but those are the defaults.
  MINFO("start block: " << start_height << "  stop block: " <<
      block_stop);

  bool use_batch = opt_batch && !opt_verify;

  MINFO("Reading blockchain from bootstrap file...");
  std::cout << ENDL;

  std::vector<block_complete_entry> blocks;

  // Skip to start_height before we start adding.
  {
    bool q2 = false;
    import_file.seekg(pos);
    bytes_read = bootstrap.count_bytes(import_file, start_height-seek_height, h, q2);
    if (q2)
    {
      quit = 2;
      goto quitting;
    }
    h = start_height;
  }

  if (use_batch)
  {
    uint64_t bytes, h2;
    bool q2;
    pos = import_file.tellg();
    bytes = bootstrap.count_bytes(import_file, db_batch_size, h2, q2);
    if (import_file.eof())
      import_file.clear();
    import_file.seekg(pos);
    core.get_blockchain_storage().get_db().batch_start(db_batch_size, bytes);
  }
  while (! quit)
  {
    uint32_t chunk_size;
    import_file.read(buffer1, sizeof(chunk_size));
    // TODO: bootstrap.read_chunk();
    if (! import_file) {
      std::cout << refresh_string;
      MINFO("End of file reached");
      quit = 1;
      break;
    }
    bytes_read += sizeof(chunk_size);

    str1.assign(buffer1, sizeof(chunk_size));
    if (! ::serialization::parse_binary(str1, chunk_size))
    {
      throw std::runtime_error("Error in deserialization of chunk size");
    }
    MDEBUG("chunk_size: " << chunk_size);

    if (chunk_size > BUFFER_SIZE)
    {
      MWARNING("WARNING: chunk_size " << chunk_size << " > BUFFER_SIZE " << BUFFER_SIZE);
      throw std::runtime_error("Aborting: chunk size exceeds buffer size");
    }
    if (chunk_size > CHUNK_SIZE_WARNING_THRESHOLD)
    {
      MINFO("NOTE: chunk_size " << chunk_size << " > " << CHUNK_SIZE_WARNING_THRESHOLD);
    }
    else if (chunk_size == 0) {
      MFATAL("ERROR: chunk_size == 0");
      return 2;
    }
    import_file.read(buffer_block, chunk_size);
    if (! import_file) {
      if (import_file.eof())
      {
        std::cout << refresh_string;
        MINFO("End of file reached - file was truncated");
        quit = 1;
        break;
      }
      else
      {
        MFATAL("ERROR: unexpected end of file: bytes read before error: "
            << import_file.gcount() << " of chunk_size " << chunk_size);
        return 2;
      }
    }
    bytes_read += chunk_size;
    MDEBUG("Total bytes read: " << bytes_read);

    if (h > block_stop)
    {
      std::cout << refresh_string << "block " << h-1
        << " / " << block_stop
        << "\r" << std::flush;
      std::cout << ENDL << ENDL;
      MINFO("Specified block number reached - stopping.  block: " << h-1 << "  total blocks: " << h);
      quit = 1;
      break;
    }

    try
    {
      str1.assign(buffer_block, chunk_size);
      bootstrap::block_package bp;
      bool res;
      if (major_version == 0)
      {
        bootstrap::block_package_1 bp1;
        res = ::serialization::parse_binary(str1, bp1);
        if (res)
        {
          bp.block = std::move(bp1.block);
          bp.txs = std::move(bp1.txs);
          bp.block_weight = bp1.block_weight;
          bp.cumulative_difficulty = bp1.cumulative_difficulty;
          bp.coins_generated = bp1.coins_generated;
        }
      }
      else
        res = ::serialization::parse_binary(str1, bp);
      if (!res)
        throw std::runtime_error("Error in deserialization of chunk");

      int display_interval = 1000;
      int progress_interval = 10;
      // NOTE: use of NUM_BLOCKS_PER_CHUNK is a placeholder in case multi-block chunks are later supported.
      for (int chunk_ind = 0; chunk_ind < NUM_BLOCKS_PER_CHUNK; ++chunk_ind)
      {
        ++h;
        if ((h-1) % display_interval == 0)
        {
          std::cout << refresh_string;
          MDEBUG("loading block number " << h-1);
        }
        else
        {
          MDEBUG("loading block number " << h-1);
        }
        b = bp.block;
        MDEBUG("block prev_id: " << b.prev_id << ENDL);

        if ((h-1) % progress_interval == 0)
        {
          std::cout << refresh_string << "block " << h-1
            << " / " << block_stop
            << "\r" << std::flush;
        }

        if (opt_verify)
        {
          cryptonote::blobdata block;
          cryptonote::block_to_blob(bp.block, block);
          std::vector<tx_blob_entry> txs;
          for (const auto &tx: bp.txs)
          {
            txs.push_back({cryptonote::blobdata(), crypto::null_hash});
            cryptonote::tx_to_blob(tx, txs.back().blob);
          }
          block_complete_entry bce;
          bce.pruned = false;
          bce.block = std::move(block);
          bce.txs = std::move(txs);
          blocks.push_back(bce);
          int ret = check_flush(core, blocks, false);
          if (ret)
          {
            quit = 2; // make sure we don't commit partial block data
            break;
          }
        }
        else
        {
          std::vector<std::pair<transaction, blobdata>> txs;
          std::vector<transaction> archived_txs;

          archived_txs = bp.txs;

          // tx number 1: coinbase tx
          // tx number 2 onwards: archived_txs
          for (const transaction &tx : archived_txs)
          {
            // add blocks with verification.
            // for Blockchain and blockchain_storage add_new_block().
            // for add_block() method, without (much) processing.
            // don't add coinbase transaction to txs.
            //
            // because add_block() calls
            // add_transaction(blk_hash, blk.miner_tx) first, and
            // then a for loop for the transactions in txs.
            txs.push_back(std::make_pair(tx, tx_to_blob(tx)));
          }

          size_t block_weight;
          difficulty_type cumulative_difficulty;
          uint64_t coins_generated;

          block_weight = bp.block_weight;
          cumulative_difficulty = bp.cumulative_difficulty;
          coins_generated = bp.coins_generated;

          try
          {
            uint64_t long_term_block_weight = core.get_blockchain_storage().get_next_long_term_block_weight(block_weight);
            core.get_blockchain_storage().get_db().add_block(std::make_pair(b, block_to_blob(b)), block_weight, long_term_block_weight, cumulative_difficulty, coins_generated, txs);
          }
          catch (const std::exception& e)
          {
            std::cout << refresh_string;
            MFATAL("Error adding block to blockchain: " << e.what());
            quit = 2; // make sure we don't commit partial block data
            break;
          }

          if (use_batch)
          {
            if ((h-1) % db_batch_size == 0)
            {
              uint64_t bytes, h2;
              bool q2;
              std::cout << refresh_string;
              // zero-based height
              std::cout << ENDL << "[- batch commit at height " << h-1 << " -]" << ENDL;
              core.get_blockchain_storage().get_db().batch_stop();
              pos = import_file.tellg();
              bytes = bootstrap.count_bytes(import_file, db_batch_size, h2, q2);
              import_file.seekg(pos);
              core.get_blockchain_storage().get_db().batch_start(db_batch_size, bytes);
              std::cout << ENDL;
              core.get_blockchain_storage().get_db().show_stats();
            }
          }
        }
        ++num_imported;
      }
    }
    catch (const std::exception& e)
    {
      std::cout << refresh_string;
      MFATAL("exception while reading from file, height=" << h << ": " << e.what());
      return 2;
    }
  } // while

quitting:
  import_file.close();

  if (opt_verify)
  {
    int ret = check_flush(core, blocks, true);
    if (ret)
      return ret;
  }

  if (use_batch)
  {
    if (quit > 1)
    {
      // There was an error, so don't commit pending data.
      // Destructor will abort write txn.
    }
    else
    {
      core.get_blockchain_storage().get_db().batch_stop();
    }
  }

  core.get_blockchain_storage().get_db().show_stats();
  MINFO("Number of blocks imported: " << num_imported);
  if (h > 0)
    // TODO: if there was an error, the last added block is probably at zero-based height h-2
    MINFO("Finished at block: " << h-1 << "  total blocks: " << h);

  std::cout << ENDL;
  return 0;
}

int main(int argc, char* argv[])
{
  TRY_ENTRY();

  epee::string_tools::set_module_name_and_folder(argv[0]);

  uint32_t log_level = 0;
  uint64_t num_blocks = 0;
  uint64_t block_stop = 0;
  std::string m_config_folder;
  std::string db_arg_str;

  tools::on_startup();

  std::string import_file_path;

  po::options_description desc_cmd_only("Command line options");
  po::options_description desc_cmd_sett("Command line options and settings options");
  const command_line::arg_descriptor<std::string> arg_input_file = {"input-file", "Specify input file", "", true};
  const command_line::arg_descriptor<std::string> arg_log_level   = {"log-level",  "0-4 or categories", ""};
  const command_line::arg_descriptor<uint64_t> arg_block_stop  = {"block-stop", "Stop at block number", block_stop};
  const command_line::arg_descriptor<uint64_t> arg_batch_size  = {"batch-size", "", db_batch_size};
  const command_line::arg_descriptor<uint64_t> arg_pop_blocks  = {"pop-blocks", "Remove blocks from end of blockchain", num_blocks};
  const command_line::arg_descriptor<bool>        arg_drop_hf  = {"drop-hard-fork", "Drop hard fork subdbs", false};
  const command_line::arg_descriptor<bool>     arg_count_blocks = {
    "count-blocks"
      , "Count blocks in bootstrap file and exit"
      , false
  };
  const command_line::arg_descriptor<bool> arg_noverify =  {"dangerous-unverified-import",
    "Blindly trust the import file and use potentially malicious blocks and transactions during import (only enable if you exported the file yourself)", false};
  const command_line::arg_descriptor<bool> arg_batch  =  {"batch",
    "Batch transactions for faster import", true};
  const command_line::arg_descriptor<bool> arg_resume =  {"resume",
    "Resume from current height if output database already exists", true};

  command_line::add_arg(desc_cmd_sett, arg_input_file);
  command_line::add_arg(desc_cmd_sett, arg_log_level);
  command_line::add_arg(desc_cmd_sett, arg_batch_size);
  command_line::add_arg(desc_cmd_sett, arg_block_stop);

  command_line::add_arg(desc_cmd_only, arg_count_blocks);
  command_line::add_arg(desc_cmd_only, arg_pop_blocks);
  command_line::add_arg(desc_cmd_only, arg_drop_hf);
  command_line::add_arg(desc_cmd_only, command_line::arg_help);

  // call add_options() directly for these arguments since
  // command_line helpers support only boolean switch, not boolean argument
  desc_cmd_sett.add_options()
    (arg_noverify.name, make_semantic(arg_noverify), arg_noverify.description)
    (arg_batch.name,  make_semantic(arg_batch),  arg_batch.description)
    (arg_resume.name, make_semantic(arg_resume), arg_resume.description)
    ;

  po::options_description desc_options("Allowed options");
  desc_options.add(desc_cmd_only).add(desc_cmd_sett);
  cryptonote::core::init_options(desc_options);

  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_options, [&]()
  {
    po::store(po::parse_command_line(argc, argv, desc_options), vm);
    po::notify(vm);
    return true;
  });
  if (! r)
    return 1;

  opt_verify    = !command_line::get_arg(vm, arg_noverify);
  opt_batch     = command_line::get_arg(vm, arg_batch);
  opt_resume    = command_line::get_arg(vm, arg_resume);
  block_stop    = command_line::get_arg(vm, arg_block_stop);
  db_batch_size = command_line::get_arg(vm, arg_batch_size);

  if (command_line::get_arg(vm, command_line::arg_help))
  {
    std::cout << "Monero '" << MONERO_RELEASE_NAME << "' (v" << MONERO_VERSION_FULL << ")" << ENDL << ENDL;
    std::cout << desc_options << std::endl;
    return 1;
  }

  if (! opt_batch && !command_line::is_arg_defaulted(vm, arg_batch_size))
  {
    std::cerr << "Error: batch-size set, but batch option not enabled" << ENDL;
    return 1;
  }
  if (! db_batch_size)
  {
    std::cerr << "Error: batch-size must be > 0" << ENDL;
    return 1;
  }
  if (opt_verify && command_line::is_arg_defaulted(vm, arg_batch_size))
  {
    // usually want batch size default lower if verify on, so progress can be
    // frequently saved.
    //
    // currently, with Windows, default batch size is low, so ignore
    // default db_batch_size_verify unless it's even lower
    if (db_batch_size > db_batch_size_verify)
    {
      db_batch_size = db_batch_size_verify;
    }
  }

  opt_testnet = command_line::get_arg(vm, cryptonote::arg_testnet_on);
  opt_stagenet = command_line::get_arg(vm, cryptonote::arg_stagenet_on);
  if (opt_testnet && opt_stagenet)
  {
    std::cerr << "Error: Can't specify more than one of --testnet and --stagenet" << ENDL;
    return 1;
  }
  m_config_folder = command_line::get_arg(vm, cryptonote::arg_data_dir);

  mlog_configure(mlog_get_default_log_path("monero-blockchain-import.log"), true);
  if (!command_line::is_arg_defaulted(vm, arg_log_level))
    mlog_set_log(command_line::get_arg(vm, arg_log_level).c_str());
  else
    mlog_set_log(std::string(std::to_string(log_level) + ",bcutil:INFO").c_str());

  MINFO("Starting...");

  boost::filesystem::path fs_import_file_path;

  if (command_line::has_arg(vm, arg_input_file))
    fs_import_file_path = boost::filesystem::path(command_line::get_arg(vm, arg_input_file));
  else
    fs_import_file_path = boost::filesystem::path(m_config_folder) / "export" / BLOCKCHAIN_RAW;

  import_file_path = fs_import_file_path.string();

  if (command_line::has_arg(vm, arg_count_blocks))
  {
    BootstrapFile bootstrap;
    bootstrap.count_blocks(import_file_path);
    return 0;
  }

  MINFO("database: LMDB");
  MINFO("verify:  " << std::boolalpha << opt_verify << std::noboolalpha);
  if (opt_batch)
  {
    MINFO("batch:   " << std::boolalpha << opt_batch << std::noboolalpha
        << "  batch size: " << db_batch_size);
  }
  else
  {
    MINFO("batch:   " << std::boolalpha << opt_batch << std::noboolalpha);
  }
  MINFO("resume:  " << std::boolalpha << opt_resume  << std::noboolalpha);
  MINFO("nettype: " << (opt_testnet ? "testnet" : opt_stagenet ? "stagenet" : "mainnet"));

  MINFO("bootstrap file path: " << import_file_path);
  MINFO("database path:       " << m_config_folder);

  if (!opt_verify)
  {
    MCLOG_RED(el::Level::Warning, "global", "\n"
      "Import is set to proceed WITHOUT VERIFICATION.\n"
      "This is a DANGEROUS operation: if the file was tampered with in transit, or obtained from a malicious source,\n"
      "you could end up with a compromised database. It is recommended to NOT use " << arg_noverify.name << ".\n"
      "*****************************************************************************************\n"
      "You have 90 seconds to press ^C or terminate this program before unverified import starts\n"
      "*****************************************************************************************");
    sleep(90);
  }

  cryptonote::cryptonote_protocol_stub pr; //TODO: stub only for this kind of test, make real validation of relayed objects
  cryptonote::core core(&pr);

  try
  {

  core.disable_dns_checkpoints(true);
#if defined(PER_BLOCK_CHECKPOINT)
  const GetCheckpointsCallback& get_checkpoints = blocks::GetCheckpointsData;
#else
  const GetCheckpointsCallback& get_checkpoints = nullptr;
#endif
  if (!core.init(vm, nullptr, get_checkpoints))
  {
    std::cerr << "Failed to initialize core" << ENDL;
    return 1;
  }
  core.get_blockchain_storage().get_db().set_batch_transactions(true);

  if (!command_line::is_arg_defaulted(vm, arg_pop_blocks))
  {
    num_blocks = command_line::get_arg(vm, arg_pop_blocks);
    MINFO("height: " << core.get_blockchain_storage().get_current_blockchain_height());
    pop_blocks(core, num_blocks);
    MINFO("height: " << core.get_blockchain_storage().get_current_blockchain_height());
    return 0;
  }

  if (!command_line::is_arg_defaulted(vm, arg_drop_hf))
  {
    MINFO("Dropping hard fork tables...");
    core.get_blockchain_storage().get_db().drop_hard_fork_info();
    core.deinit();
    return 0;
  }

  import_from_file(core, import_file_path, block_stop);

  // ensure db closed
  //   - transactions properly checked and handled
  //   - disk sync if needed
  //
  core.deinit();
  }
  catch (const DB_ERROR& e)
  {
    std::cout << std::string("Error loading blockchain db: ") + e.what() + " -- shutting down now" << ENDL;
    core.deinit();
    return 1;
  }

  return 0;

  CATCH_ENTRY("Import error", 1);
}
