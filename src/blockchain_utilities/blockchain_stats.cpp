// Copyright (c) 2014-2019, The Monero Project
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

#include <boost/algorithm/string.hpp>
#include "common/command_line.h"
#include "common/varint.h"
#include "cryptonote_basic/cryptonote_boost_serialization.h"
#include "cryptonote_core/tx_pool.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_core/blockchain.h"
#include "blockchain_db/blockchain_db.h"
#include "version.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "bcutil"

namespace po = boost::program_options;
using namespace epee;
using namespace cryptonote;

static bool stop_requested = false;

int main(int argc, char* argv[])
{
  TRY_ENTRY();

  epee::string_tools::set_module_name_and_folder(argv[0]);

  uint32_t log_level = 0;
  uint64_t block_start = 0;
  uint64_t block_stop = 0;

  tools::on_startup();

  boost::filesystem::path output_file_path;

  po::options_description desc_cmd_only("Command line options");
  po::options_description desc_cmd_sett("Command line options and settings options");
  const command_line::arg_descriptor<std::string> arg_log_level  = {"log-level",  "0-4 or categories", ""};
  const command_line::arg_descriptor<uint64_t> arg_block_start  = {"block-start", "start at block number", block_start};
  const command_line::arg_descriptor<uint64_t> arg_block_stop = {"block-stop", "Stop at block number", block_stop};
  const command_line::arg_descriptor<bool> arg_inputs  = {"with-inputs", "with input stats", false};
  const command_line::arg_descriptor<bool> arg_outputs  = {"with-outputs", "with output stats", false};
  const command_line::arg_descriptor<bool> arg_ringsize  = {"with-ringsize", "with ringsize stats", false};
  const command_line::arg_descriptor<bool> arg_hours  = {"with-hours", "with txns per hour", false};

  command_line::add_arg(desc_cmd_sett, cryptonote::arg_data_dir);
  command_line::add_arg(desc_cmd_sett, cryptonote::arg_testnet_on);
  command_line::add_arg(desc_cmd_sett, cryptonote::arg_stagenet_on);
  command_line::add_arg(desc_cmd_sett, arg_log_level);
  command_line::add_arg(desc_cmd_sett, arg_block_start);
  command_line::add_arg(desc_cmd_sett, arg_block_stop);
  command_line::add_arg(desc_cmd_sett, arg_inputs);
  command_line::add_arg(desc_cmd_sett, arg_outputs);
  command_line::add_arg(desc_cmd_sett, arg_ringsize);
  command_line::add_arg(desc_cmd_sett, arg_hours);
  command_line::add_arg(desc_cmd_only, command_line::arg_help);

  po::options_description desc_options("Allowed options");
  desc_options.add(desc_cmd_only).add(desc_cmd_sett);

  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_options, [&]()
  {
    auto parser = po::command_line_parser(argc, argv).options(desc_options);
    po::store(parser.run(), vm);
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

  mlog_configure(mlog_get_default_log_path("monero-blockchain-stats.log"), true);
  if (!command_line::is_arg_defaulted(vm, arg_log_level))
    mlog_set_log(command_line::get_arg(vm, arg_log_level).c_str());
  else
    mlog_set_log(std::string(std::to_string(log_level) + ",bcutil:INFO").c_str());

  LOG_PRINT_L0("Starting...");

  std::string opt_data_dir = command_line::get_arg(vm, cryptonote::arg_data_dir);
  bool opt_testnet = command_line::get_arg(vm, cryptonote::arg_testnet_on);
  bool opt_stagenet = command_line::get_arg(vm, cryptonote::arg_stagenet_on);
  network_type net_type = opt_testnet ? TESTNET : opt_stagenet ? STAGENET : MAINNET;
  block_start = command_line::get_arg(vm, arg_block_start);
  block_stop = command_line::get_arg(vm, arg_block_stop);
  bool do_inputs = command_line::get_arg(vm, arg_inputs);
  bool do_outputs = command_line::get_arg(vm, arg_outputs);
  bool do_ringsize = command_line::get_arg(vm, arg_ringsize);
  bool do_hours = command_line::get_arg(vm, arg_hours);

  LOG_PRINT_L0("Initializing source blockchain (BlockchainDB)");
  std::unique_ptr<Blockchain> core_storage;
  tx_memory_pool m_mempool(*core_storage);
  core_storage.reset(new Blockchain(m_mempool));
  BlockchainDB *db = new_db();
  if (db == NULL)
  {
    LOG_ERROR("Failed to initialize a database");
    throw std::runtime_error("Failed to initialize a database");
  }

  const std::string filename = (boost::filesystem::path(opt_data_dir) / db->get_db_name()).string();
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
  r = core_storage->init(db, net_type);

  CHECK_AND_ASSERT_MES(r, 1, "Failed to initialize source blockchain storage");
  LOG_PRINT_L0("Source blockchain storage initialized OK");

  tools::signal_handler::install([](int type) {
    stop_requested = true;
  });

  const uint64_t db_height = db->height();
  if (!block_stop)
      block_stop = db_height;
  MINFO("Starting from height " << block_start << ", stopping at height " << block_stop);

/*
 * The default output can be plotted with GnuPlot using these commands:
set key autotitle columnhead
set title "Monero Blockchain Growth"
set timefmt "%Y-%m-%d"
set xdata time
set xrange ["2014-04-17":*]
set format x "%Y-%m-%d"
set yrange [0:*]
set y2range [0:*]
set ylabel "Txs/Day"
set y2label "Bytes"
set y2tics nomirror
plot 'stats.csv' index "DATA" using (timecolumn(1,"%Y-%m-%d")):4 with lines, '' using (timecolumn(1,"%Y-%m-%d")):7 axes x1y2 with lines
 */

  // spit out a comment that GnuPlot can use as an index
  std::cout << ENDL << "# DATA" << ENDL;
  std::cout << "Date\tBlocks/day\tBlocks\tTxs/Day\tTxs\tBytes/Day\tBytes";
  if (do_inputs)
    std::cout << "\tInMin\tInMax\tInAvg";
  if (do_outputs)
    std::cout << "\tOutMin\tOutMax\tOutAvg";
  if (do_ringsize)
    std::cout << "\tRingMin\tRingMax\tRingAvg";
  if (do_hours) {
    char buf[8];
    unsigned int i;
    for (i=0; i<24; i++) {
      sprintf(buf, "\t%02u:00", i);
      std::cout << buf;
    }
  }
  std::cout << ENDL;

  struct tm prevtm = {0}, currtm;
  uint64_t prevsz = 0, currsz = 0;
  uint64_t prevtxs = 0, currtxs = 0;
  uint64_t currblks = 0;
  uint64_t totins = 0, totouts = 0, totrings = 0;
  uint32_t minins = 10, maxins = 0;
  uint32_t minouts = 10, maxouts = 0;
  uint32_t minrings = 50, maxrings = 0;
  uint32_t io, tottxs = 0;
  uint32_t txhr[24] = {0};
  unsigned int i;

  for (uint64_t h = block_start; h < block_stop; ++h)
  {
    cryptonote::blobdata bd = db->get_block_blob_from_height(h);
    cryptonote::block blk;
    if (!cryptonote::parse_and_validate_block_from_blob(bd, blk))
    {
      LOG_PRINT_L0("Bad block from db");
      return 1;
    }
    time_t tt = blk.timestamp;
    char timebuf[64];
    epee::misc_utils::get_gmt_time(tt, currtm);
    if (!prevtm.tm_year)
      prevtm = currtm;
    // catch change of day
    if (currtm.tm_mday > prevtm.tm_mday || (currtm.tm_mday == 1 && prevtm.tm_mday > 27))
    {
      // check for timestamp fudging around month ends
      if (prevtm.tm_mday == 1 && currtm.tm_mday > 27)
        goto skip;
      strftime(timebuf, sizeof(timebuf), "%Y-%m-%d", &prevtm);
      prevtm = currtm;
      std::cout << timebuf << "\t" << currblks << "\t" << h << "\t" << currtxs << "\t" << prevtxs + currtxs << "\t" << currsz << "\t" << prevsz + currsz;
      prevsz += currsz;
      currsz = 0;
      currblks = 0;
      prevtxs += currtxs;
      currtxs = 0;
      if (!tottxs)
        tottxs = 1;
      if (do_inputs) {
        std::cout << "\t" << (maxins ? minins : 0) << "\t" << maxins << "\t" << totins / tottxs;
        minins = 10; maxins = 0; totins = 0;
      }
      if (do_outputs) {
        std::cout << "\t" << (maxouts ? minouts : 0) << "\t" << maxouts << "\t" << totouts / tottxs;
        minouts = 10; maxouts = 0; totouts = 0;
      }
      if (do_ringsize) {
        std::cout << "\t" << (maxrings ? minrings : 0) << "\t" << maxrings << "\t" << totrings / tottxs;
        minrings = 50; maxrings = 0; totrings = 0;
      }
      tottxs = 0;
      if (do_hours) {
        for (i=0; i<24; i++) {
          std::cout << "\t" << txhr[i];
          txhr[i] = 0;
        }
      }
      std::cout << ENDL;
    }
skip:
    currsz += bd.size();
    for (const auto& tx_id : blk.tx_hashes)
    {
      if (tx_id == crypto::null_hash)
      {
        throw std::runtime_error("Aborting: tx == null_hash");
      }
      if (!db->get_pruned_tx_blob(tx_id, bd))
      {
        throw std::runtime_error("Aborting: tx not found");
      }
      transaction tx;
      if (!parse_and_validate_tx_base_from_blob(bd, tx))
      {
        LOG_PRINT_L0("Bad txn from db");
        return 1;
      }
      currsz += bd.size();
      currtxs++;
      if (do_hours)
        txhr[currtm.tm_hour]++;
      if (do_inputs) {
        io = tx.vin.size();
        if (io < minins)
          minins = io;
        else if (io > maxins)
          maxins = io;
        totins += io;
      }
      if (do_ringsize) {
        const cryptonote::txin_to_key& tx_in_to_key
                       = boost::get<cryptonote::txin_to_key>(tx.vin[0]);
        io = tx_in_to_key.key_offsets.size();
        if (io < minrings)
          minrings = io;
        else if (io > maxrings)
          maxrings = io;
        totrings += io;
      }
      if (do_outputs) {
        io = tx.vout.size();
        if (io < minouts)
          minouts = io;
        else if (io > maxouts)
          maxouts = io;
        totouts += io;
      }
      tottxs++;
    }
    currblks++;

    if (stop_requested)
      break;
  }

  core_storage->deinit();
  return 0;

  CATCH_ENTRY("Stats reporting error", 1);
}
