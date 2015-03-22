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

#include <atomic>
#include <cstdio>
#include <algorithm>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "cryptonote_core/cryptonote_basic.h"
#include "cryptonote_core/cryptonote_format_utils.h"
#include "cryptonote_core/cryptonote_boost_serialization.h"
#include "serialization/json_utils.h" // dump_json()
#include "include_base_utils.h"
#include "common/command_line.h"
#include "version.h"

#include <lmdb.h> // for db flag arguments

#include "import.h"
#include "fake_core.h"

// CONFIG
static bool opt_batch   = true;
static bool opt_verify  = true; // use add_new_block, which does verification before calling add_block
static bool opt_resume  = true;
static bool opt_testnet = true;

// number of blocks per batch transaction
// adjustable through command-line argument according to available RAM
static uint64_t db_batch_size = 20000;


namespace po = boost::program_options;

using namespace cryptonote;
using namespace epee;


int parse_db_arguments(const std::string& db_arg_str, std::string& db_engine, int& mdb_flags)
{
  std::vector<std::string> db_args;
  boost::split(db_args, db_arg_str, boost::is_any_of("#"));
  db_engine = db_args.front();
  boost::algorithm::trim(db_engine);

  if (db_args.size() == 1)
  {
    return 0;
  }
  else if (db_args.size() > 2)
  {
    std::cerr << "unrecognized database argument format: " << db_arg_str << ENDL;
    return 1;
  }

  std::string db_arg_str2 = db_args[1];
  boost::split(db_args, db_arg_str2, boost::is_any_of(","));
  for (auto& it : db_args)
  {
    boost::algorithm::trim(it);
    if (it.empty())
      continue;
    LOG_PRINT_L1("LMDB flag: " << it);
    if (it == "nosync")
    {
      mdb_flags |= MDB_NOSYNC;
    }
    else if (it == "nometasync")
    {
      mdb_flags |= MDB_NOMETASYNC;
    }
    else if (it == "writemap")
    {
      mdb_flags |= MDB_WRITEMAP;
    }
    else if (it == "mapasync")
    {
      mdb_flags |= MDB_MAPASYNC;
    }
    else
    {
      std::cerr << "unrecognized database flag: " << it << ENDL;
      return 1;
    }
  }
  return 0;
}


int count_blocks(std::string& import_file_path)
{
  boost::filesystem::path raw_file_path(import_file_path);
  boost::system::error_code ec;
  if (!boost::filesystem::exists(raw_file_path, ec))
  {
    LOG_PRINT_L0("import file not found: " << raw_file_path);
    throw std::runtime_error("Aborting");
  }
  std::ifstream import_file;
  import_file.open(import_file_path, std::ios_base::binary | std::ifstream::in);

  uint64_t h = 0;
  if (import_file.fail())
  {
    LOG_PRINT_L0("import_file.open() fail");
    throw std::runtime_error("Aborting");
  }
  LOG_PRINT_L0("Scanning blockchain from import file...");
  char buffer1[STR_LENGTH_OF_INT + 1];
  block b;
  transaction tx;
  bool quit = false;
  uint64_t bytes_read = 0;
  int progress_interval = 10;

  while (! quit)
  {
    int chunk_size;
    import_file.read(buffer1, STR_LENGTH_OF_INT);
    if (!import_file) {
      std::cout << "\r                   \r";
      LOG_PRINT_L1("End of import file reached");
      quit = true;
      break;
    }
    h += NUM_BLOCKS_PER_CHUNK;
    if (h % progress_interval == 0)
    {
      std::cout << "\r                   \r" << "block height: " << h <<
        std::flush;
    }
    bytes_read += STR_LENGTH_OF_INT;
    buffer1[STR_LENGTH_OF_INT] = '\0';
    chunk_size = atoi(buffer1);
    if (chunk_size > BUFFER_SIZE)
    {
      std::cout << "\r                   \r";
      LOG_PRINT_L0("WARNING: chunk_size " << chunk_size << " > BUFFER_SIZE " << BUFFER_SIZE
          << "  height: " << h);
      throw std::runtime_error("Aborting: chunk size exceeds buffer size");
    }
    if (chunk_size > 100000)
    {
      std::cout << "\r                   \r";
      LOG_PRINT_L0("WARNING: chunk_size " << chunk_size << " > 100000" << "  height: "
          << h);
    }
    else if (chunk_size <= 0) {
      std::cout << "\r                   \r";
      LOG_PRINT_L0("ERROR: chunk_size " << chunk_size << " <= 0" << "  height: " << h);
      throw std::runtime_error("Aborting");
    }
    // skip to next expected block size value
    import_file.seekg(chunk_size, std::ios_base::cur);
    if (! import_file) {
      std::cout << "\r                   \r";
      LOG_PRINT_L0("ERROR: unexpected end of import file: bytes read before error: "
          << import_file.gcount() << " of chunk_size " << chunk_size);
      throw std::runtime_error("Aborting");
    }
    bytes_read += chunk_size;
    std::cout << "\r                   \r";
    LOG_PRINT_L3("Total bytes scanned: " << bytes_read);
  }

  import_file.close();

  std::cout << ENDL;
  std::cout << "Done scanning import file" << ENDL;
  std::cout << "Total bytes scanned: " << bytes_read << ENDL;
  std::cout << "Height: " << h << ENDL;

  return h;
}

template <typename FakeCore>
int import_from_file(FakeCore& simple_core, std::string& import_file_path)
{
#if !defined(BLOCKCHAIN_DB)
  static_assert(std::is_same<fake_core_memory, FakeCore>::value || std::is_same<fake_core_lmdb, FakeCore>::value,
      "FakeCore constraint error");
#endif
#if !defined(BLOCKCHAIN_DB) || (BLOCKCHAIN_DB == DB_LMDB)
  if (std::is_same<fake_core_lmdb, FakeCore>::value)
  {
    // Reset stats, in case we're using newly created db, accumulating stats
    // from addition of genesis block.
    // This aligns internal db counts with importer counts.
    simple_core.m_storage.get_db().reset_stats();
  }
#endif
  boost::filesystem::path raw_file_path(import_file_path);
  boost::system::error_code ec;
  if (!boost::filesystem::exists(raw_file_path, ec))
  {
    LOG_PRINT_L0("import file not found: " << raw_file_path);
    return false;
  }

  uint64_t source_height = count_blocks(import_file_path);
  LOG_PRINT_L0("import file blockchain height: " << source_height);

  std::ifstream import_file;
  import_file.open(import_file_path, std::ios_base::binary | std::ifstream::in);

  uint64_t h = 0;
  if (import_file.fail())
  {
    LOG_PRINT_L0("import_file.open() fail");
    return false;
  }
  char buffer1[STR_LENGTH_OF_INT + 1];
  char buffer_block[BUFFER_SIZE];
  block b;
  transaction tx;
  int quit = 0;
  uint64_t bytes_read = 0;

  uint64_t start_height = 1;
  if (opt_resume)
    start_height = simple_core.m_storage.get_current_blockchain_height();

  // Note that a new blockchain will start with a height of 1 (block number 0)
  // due to genesis block being added at initialization.

  // CONFIG
  // TODO: can expand on this, e.g. with --block-number option
  uint64_t stop_height = source_height;

  // These are what we'll try to use, and they don't have to be a determination
  // from source and destination blockchains, but those are the current
  // defaults.
  LOG_PRINT_L0("start height: " << start_height << "  stop height: " <<
      stop_height);

  bool use_batch = false;
  if (opt_batch)
  {
    if (simple_core.support_batch)
      use_batch = true;
    else
      LOG_PRINT_L0("WARNING: batch transactions enabled but unsupported or unnecessary for this database engine - ignoring");
  }

  if (use_batch)
    simple_core.batch_start();

  LOG_PRINT_L0("Reading blockchain from import file...");
  std::cout << ENDL;

  // Within the loop, we skip to start_height before we start adding.
  // TODO: Not a bottleneck, but we can use what's done in count_blocks() and
  // only do the chunk size reads, skipping the chunk content reads until we're
  // at start_height.
  while (! quit)
  {
    int chunk_size;
    import_file.read(buffer1, STR_LENGTH_OF_INT);
    if (! import_file) {
      std::cout << "\r                   \r";
      LOG_PRINT_L0("End of import file reached");
      quit = 1;
      break;
    }
    bytes_read += STR_LENGTH_OF_INT;
    buffer1[STR_LENGTH_OF_INT] = '\0';
    chunk_size = atoi(buffer1);
    if (chunk_size > BUFFER_SIZE)
    {
      LOG_PRINT_L0("WARNING: chunk_size " << chunk_size << " > BUFFER_SIZE " << BUFFER_SIZE);
      throw std::runtime_error("Aborting: chunk size exceeds buffer size");
    }
    if (chunk_size > 100000)
    {
      LOG_PRINT_L0("WARNING: chunk_size " << chunk_size << " > 100000");
    }
    else if (chunk_size < 0) {
      LOG_PRINT_L0("ERROR: chunk_size " << chunk_size << " < 0");
      return 2;
    }
    import_file.read(buffer_block, chunk_size);
    if (! import_file) {
      LOG_PRINT_L0("ERROR: unexpected end of import file: bytes read before error: "
          << import_file.gcount() << " of chunk_size " << chunk_size);
      return 2;
    }
    bytes_read += chunk_size;
    LOG_PRINT_L3("Total bytes read: " << bytes_read);

    if (h + NUM_BLOCKS_PER_CHUNK < start_height + 1)
    {
      h += NUM_BLOCKS_PER_CHUNK;
      continue;
    }
    if (h > stop_height)
    {
      LOG_PRINT_L0("Specified height reached - stopping.  height: " << h << "  block: " << h-1);
      quit = 1;
      break;
    }

    try
    {
      boost::iostreams::basic_array_source<char> device(buffer_block, chunk_size);
      boost::iostreams::stream<boost::iostreams::basic_array_source<char>> s(device);
      boost::archive::binary_iarchive a(s);

      int display_interval = 1000;
      int progress_interval = 10;
      for (int chunk_ind = 0; chunk_ind < NUM_BLOCKS_PER_CHUNK; chunk_ind++)
      {
        h++;
        if (h % display_interval == 0)
        {
          std::cout << "\r                   \r";
          LOG_PRINT_L0("loading block height " << h);
        }
        else
        {
          LOG_PRINT_L3("loading block height " << h);
        }
        try {
          a >> b;
        }
        catch (const std::exception& e)
        {
          std::cout << "\r                   \r";
          LOG_PRINT_RED_L0("exception while de-archiving block, height=" << h);
          quit = 1;
          break;
        }
        LOG_PRINT_L2("block prev_id: " << b.prev_id << ENDL);

        if (h % progress_interval == 0)
        {
          std::cout << "\r                   \r" << "block " << h-1
            << std::flush;
        }

        std::vector<transaction> txs;

        int num_txs;
        try
        {
          a >> num_txs;
        }
        catch (const std::exception& e)
        {
          std::cout << "\r                   \r";
          LOG_PRINT_RED_L0("exception while de-archiving tx-num, height=" << h);
          quit = 1;
          break;
        }
        for(int tx_num = 1; tx_num <= num_txs; tx_num++)
        {
          try {
                a >> tx;
          }
          catch (const std::exception& e)
          {
            LOG_PRINT_RED_L0("exception while de-archiving tx, height=" << h <<", tx_num=" << tx_num);
            quit = 1;
            break;
          }
          // if (tx_num == 1) {
          //   std::cout << "coinbase transaction" << ENDL;
          // }
          // crypto::hash hsh = null_hash;
          // size_t blob_size = 0;
          // NOTE: all tx hashes except for coinbase tx are available in the block data
          // get_transaction_hash(tx, hsh, blob_size);
          // LOG_PRINT_L0("tx " << tx_num << "  " << hsh << " : " << ENDL);
          // LOG_PRINT_L0(obj_to_json_str(tx) << ENDL);

          // add blocks with verification.
          // for Blockchain and blockchain_storage add_new_block().
          if (opt_verify)
          {
            if (tx_num == 1) {
              continue; // coinbase transaction. no need to insert to tx_pool.
            }
            // crypto::hash hsh = null_hash;
            // size_t blob_size = 0;
            // get_transaction_hash(tx, hsh, blob_size);
            tx_verification_context tvc = AUTO_VAL_INIT(tvc);
            bool r = true;
            r = simple_core.m_pool.add_tx(tx, tvc, true);
            if (!r)
            {
              LOG_PRINT_RED_L0("failed to add transaction to transaction pool, height=" << h <<", tx_num=" << tx_num);
              quit = 1;
              break;
            }
          }
          else
          {
            // for add_block() method, without (much) processing.
            // don't add coinbase transaction to txs.
            //
            // because add_block() calls
            // add_transaction(blk_hash, blk.miner_tx) first, and
            // then a for loop for the transactions in txs.
            if (tx_num > 1)
            {
              txs.push_back(tx);
            }
          }
        }

        if (opt_verify)
        {
          block_verification_context bvc = boost::value_initialized<block_verification_context>();
          simple_core.m_storage.add_new_block(b, bvc);

          if (bvc.m_verifivation_failed)
          {
            LOG_PRINT_L0("Failed to add block to blockchain, verification failed, height = " << h);
            LOG_PRINT_L0("skipping rest of import file");
            // ok to commit previously batched data because it failed only in
            // verification of potential new block with nothing added to batch
            // yet
            quit = 1;
            break;
          }
          if (! bvc.m_added_to_main_chain)
          {
            LOG_PRINT_L0("Failed to add block to blockchain, height = " << h);
            LOG_PRINT_L0("skipping rest of import file");
            // make sure we don't commit partial block data
            quit = 2;
            break;
          }
        }
        else
        {
          size_t block_size;
          difficulty_type cumulative_difficulty;
          uint64_t coins_generated;

          a >> block_size;
          a >> cumulative_difficulty;
          a >> coins_generated;

          std::cout << "\r                   \r";
          LOG_PRINT_L2("block_size: " << block_size);
          LOG_PRINT_L2("cumulative_difficulty: " << cumulative_difficulty);
          LOG_PRINT_L2("coins_generated: " << coins_generated);

          try
          {
            simple_core.add_block(b, block_size, cumulative_difficulty, coins_generated, txs);
          }
          catch (const std::exception& e)
          {
            std::cout << "\r                   \r";
            LOG_PRINT_RED_L0("Error adding block to blockchain: " << e.what());
            quit = 2; // make sure we don't commit partial block data
            break;
          }
        }

        if (use_batch)
        {
          if (h % db_batch_size == 0)
          {
            std::cout << "\r                   \r";
            std::cout << ENDL << "[- batch commit at height " << h << " -]" << ENDL;
            simple_core.batch_stop();
            simple_core.batch_start();
            std::cout << ENDL;
#if !defined(BLOCKCHAIN_DB) || (BLOCKCHAIN_DB == DB_LMDB)
            simple_core.m_storage.get_db().show_stats();
#endif
          }
        }
      }
    }
    catch (const std::exception& e)
    {
      std::cout << "\r                   \r";
      LOG_PRINT_RED_L0("exception while reading from import file, height=" << h);
      return 2;
    }
  } // while

  import_file.close();

  if (use_batch)
  {
    if (quit > 1)
    {
      // There was an error, so don't commit pending data.
      // Destructor will abort write txn.
    }
    else
    {
      simple_core.batch_stop();
    }
#if !defined(BLOCKCHAIN_DB) || (BLOCKCHAIN_DB == DB_LMDB)
    simple_core.m_storage.get_db().show_stats();
#endif
    if (h > 0)
      LOG_PRINT_L0("Finished at height: " << h << "  block: " << h-1);
  }
  std::cout << ENDL;
  return 0;
}

int main(int argc, char* argv[])
{
  std::string import_filename = BLOCKCHAIN_RAW;
#if defined(BLOCKCHAIN_DB) && (BLOCKCHAIN_DB == DB_MEMORY)
  std::string default_db_engine = "memory";
#else
  std::string default_db_engine = "lmdb";
#endif

  uint32_t log_level = LOG_LEVEL_0;
  std::string dirname;
  std::string db_arg_str;

  boost::filesystem::path default_data_path {tools::get_default_data_dir()};
  boost::filesystem::path default_testnet_data_path {default_data_path / "testnet"};

  po::options_description desc_cmd_only("Command line options");
  po::options_description desc_cmd_sett("Command line options and settings options");
  const command_line::arg_descriptor<uint32_t> arg_log_level   =  {"log-level",  "", log_level};
  const command_line::arg_descriptor<uint64_t> arg_batch_size  =  {"batch-size", "", db_batch_size};
  const command_line::arg_descriptor<bool>     arg_testnet_on  = {
    "testnet"
      , "Run on testnet."
      , false
  };
  const command_line::arg_descriptor<bool>     arg_count_blocks = {
    "count-blocks"
      , "Count blocks in import file and exit"
      , false
  };
  const command_line::arg_descriptor<std::string> arg_database = {
    "database", "available: memory, lmdb"
      , default_db_engine
  };
  const command_line::arg_descriptor<bool> arg_verify =  {"verify",
    "Verify blocks and transactions during import", true};
  const command_line::arg_descriptor<bool> arg_batch  =  {"batch",
    "Batch transactions for faster import", true};
  const command_line::arg_descriptor<bool> arg_resume =  {"resume",
    "Resume from current height if output database already exists", true};

  command_line::add_arg(desc_cmd_sett, command_line::arg_data_dir, default_data_path.string());
  command_line::add_arg(desc_cmd_sett, command_line::arg_testnet_data_dir, default_testnet_data_path.string());
  command_line::add_arg(desc_cmd_sett, arg_log_level);
  command_line::add_arg(desc_cmd_sett, arg_batch_size);
  command_line::add_arg(desc_cmd_sett, arg_testnet_on);
  command_line::add_arg(desc_cmd_sett, arg_database);

  command_line::add_arg(desc_cmd_only, arg_count_blocks);
  command_line::add_arg(desc_cmd_only, command_line::arg_help);

  // call add_options() directly for these arguments since
  // command_line helpers support only boolean switch, not boolean argument
  desc_cmd_sett.add_options()
    (arg_verify.name, make_semantic(arg_verify), arg_verify.description)
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
  if (! r)
    return 1;

  log_level     = command_line::get_arg(vm, arg_log_level);
  opt_verify    = command_line::get_arg(vm, arg_verify);
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
    exit(1);
  }
  if (! db_batch_size)
  {
    std::cerr << "Error: batch-size must be > 0" << ENDL;
    exit(1);
  }

  std::vector<std::string> db_engines {"memory", "lmdb"};

  opt_testnet = command_line::get_arg(vm, arg_testnet_on);
  auto data_dir_arg = opt_testnet ? command_line::arg_testnet_data_dir : command_line::arg_data_dir;
  dirname = command_line::get_arg(vm, data_dir_arg);
  db_arg_str = command_line::get_arg(vm, arg_database);

  log_space::get_set_log_detalisation_level(true, log_level);
  log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
  LOG_PRINT_L0("Starting...");
  LOG_PRINT_L0("Setting log level = " << log_level);

  boost::filesystem::path file_path {dirname};
  std::string import_file_path;

  import_file_path = (file_path / "export" / import_filename).string();

  if (command_line::has_arg(vm, arg_count_blocks))
  {
    count_blocks(import_file_path);
    exit(0);
  }


  std::string db_engine;
  int mdb_flags = 0;
  int res = 0;
  res = parse_db_arguments(db_arg_str, db_engine, mdb_flags);
  if (res)
  {
    std::cerr << "Error parsing database argument(s)" << ENDL;
    exit(1);
  }

  if (std::find(db_engines.begin(), db_engines.end(), db_engine) == db_engines.end())
  {
    std::cerr << "Invalid database engine: " << db_engine << std::endl;
    exit(1);
  }

  LOG_PRINT_L0("database: " << db_engine);
  LOG_PRINT_L0("verify:  " << std::boolalpha << opt_verify << std::noboolalpha);
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

  std::cout << "import file path: " << import_file_path << ENDL;
  std::cout << "database path:    " << file_path.string() << ENDL;

  try
  {

  // fake_core needed for verification to work when enabled.
  //
  // NOTE: don't need fake_core method of doing things when we're going to call
  // BlockchainDB add_block() directly and have available the 3 block
  // properties to do so. Both ways work, but fake core isn't necessary in that
  // circumstance.

  // for multi_db_runtime:
#if !defined(BLOCKCHAIN_DB)
  if (db_engine == "lmdb")
  {
    fake_core_lmdb simple_core(dirname, opt_testnet, opt_batch, mdb_flags);
    import_from_file(simple_core, import_file_path);
  }
  else if (db_engine == "memory")
  {
    fake_core_memory simple_core(dirname, opt_testnet);
    import_from_file(simple_core, import_file_path);
  }
  else
  {
    std::cerr << "database engine unrecognized" << ENDL;
    exit(1);
  }

  // for multi_db_compile:
#else
  if (db_engine != default_db_engine)
  {
    std::cerr << "Invalid database engine for compiled version: " << db_engine << std::endl;
    exit(1);
  }
#if BLOCKCHAIN_DB == DB_LMDB
  fake_core_lmdb simple_core(dirname, opt_testnet, opt_batch, mdb_flags);
#else
  fake_core_memory simple_core(dirname, opt_testnet);
#endif

  import_from_file(simple_core, import_file_path);
#endif

  }
  catch (const DB_ERROR& e)
  {
    std::cout << std::string("Error loading blockchain db: ") + e.what() + " -- shutting down now" << ENDL;
    exit(1);
  }

  // destructors called at exit:
  //
  // ensure db closed
  //   - transactions properly checked and handled
  //   - disk sync if needed
  //
  // fake_core object's destructor is called when it goes out of scope. For an
  // LMDB fake_core, it calls Blockchain::deinit() on its object, which in turn
  // calls delete on its BlockchainDB derived class' object, which closes its
  // files.
}
