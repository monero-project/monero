// Copyright (c) 2014-2024, The Monero Project
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

#include <boost/range/adaptor/transformed.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string.hpp>
#include "common/command_line.h"
#include "common/varint.h"
#include "cryptonote_core/cryptonote_core.h"
#include "version.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "bcutil"

namespace po = boost::program_options;
using namespace epee;
using namespace cryptonote;

int main(int argc, char* argv[])
{
  TRY_ENTRY();

  epee::string_tools::set_module_name_and_folder(argv[0]);

  uint32_t log_level = 0;

  tools::on_startup();

  boost::filesystem::path output_file_path;

  po::options_description desc_cmd_only("Command line options");
  po::options_description desc_cmd_sett("Command line options and settings options");
  const command_line::arg_descriptor<std::string> arg_log_level  = {"log-level",  "0-4 or categories", ""};
  const command_line::arg_descriptor<std::string> arg_txid  = {"txid", "Get min depth for this txid", ""};
  const command_line::arg_descriptor<uint64_t> arg_height  = {"height", "Get min depth for all txes at this height", 0};
  const command_line::arg_descriptor<bool> arg_include_coinbase  = {"include-coinbase", "Include coinbase in the average", false};

  command_line::add_arg(desc_cmd_sett, cryptonote::arg_data_dir);
  command_line::add_arg(desc_cmd_sett, cryptonote::arg_testnet_on);
  command_line::add_arg(desc_cmd_sett, cryptonote::arg_stagenet_on);
  command_line::add_arg(desc_cmd_sett, arg_log_level);
  command_line::add_arg(desc_cmd_sett, arg_txid);
  command_line::add_arg(desc_cmd_sett, arg_height);
  command_line::add_arg(desc_cmd_sett, arg_include_coinbase);
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

  mlog_configure(mlog_get_default_log_path("monero-blockchain-depth.log"), true);
  if (!command_line::is_arg_defaulted(vm, arg_log_level))
    mlog_set_log(command_line::get_arg(vm, arg_log_level).c_str());
  else
    mlog_set_log(std::string(std::to_string(log_level) + ",bcutil:INFO").c_str());

  LOG_PRINT_L0("Starting...");

  std::string opt_data_dir = command_line::get_arg(vm, cryptonote::arg_data_dir);
  bool opt_testnet = command_line::get_arg(vm, cryptonote::arg_testnet_on);
  bool opt_stagenet = command_line::get_arg(vm, cryptonote::arg_stagenet_on);
  network_type net_type = opt_testnet ? TESTNET : opt_stagenet ? STAGENET : MAINNET;
  std::string opt_txid_string = command_line::get_arg(vm, arg_txid);
  uint64_t opt_height = command_line::get_arg(vm, arg_height);
  bool opt_include_coinbase = command_line::get_arg(vm, arg_include_coinbase);

  if (!opt_txid_string.empty() && opt_height)
  {
    std::cerr << "txid and height cannot be given at the same time" << std::endl;
    return 1;
  }
  crypto::hash opt_txid = crypto::null_hash;
  if (!opt_txid_string.empty())
  {
    if (!epee::string_tools::hex_to_pod(opt_txid_string, opt_txid))
    {
      std::cerr << "Invalid txid" << std::endl;
      return 1;
    }
  }

  // If we wanted to use the memory pool, we would set up a fake_core.

  // Use Blockchain instead of lower-level BlockchainDB for two reasons:
  // 1. Blockchain has the init() method for easy setup
  // 2. exporter needs to use get_current_blockchain_height(), get_block_id_by_height(), get_block_by_hash()
  LOG_PRINT_L0("Initializing source blockchain (BlockchainDB)");
  std::unique_ptr<BlockchainAndPool> core_storage = std::make_unique<BlockchainAndPool>();
  BlockchainDB *db = new_db();
  if (db == NULL)
  {
    LOG_ERROR("Failed to initialize a database");
    throw std::runtime_error("Failed to initialize a database");
  }
  LOG_PRINT_L0("database: LMDB");

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
  r = core_storage->blockchain.init(db, net_type);

  CHECK_AND_ASSERT_MES(r, 1, "Failed to initialize source blockchain storage");
  LOG_PRINT_L0("Source blockchain storage initialized OK");

  std::vector<crypto::hash> start_txids;
  if (!opt_txid_string.empty())
  {
    start_txids.push_back(opt_txid);
  }
  else
  {
    const cryptonote::blobdata bd = db->get_block_blob_from_height(opt_height);
    cryptonote::block b;
    if (!cryptonote::parse_and_validate_block_from_blob(bd, b))
    {
      LOG_PRINT_L0("Bad block from db");
      return 1;
    }
    for (const crypto::hash &txid: b.tx_hashes)
      start_txids.push_back(txid);
    if (opt_include_coinbase)
      start_txids.push_back(cryptonote::get_transaction_hash(b.miner_tx));
  }

  if (start_txids.empty())
  {
    LOG_PRINT_L0("No transaction(s) to check");
    return 1;
  }

  std::vector<uint64_t> depths;
  for (const crypto::hash &start_txid: start_txids)
  {
    uint64_t depth = 0;
    bool coinbase = false;

    LOG_PRINT_L0("Checking depth for txid " << start_txid);
    std::vector<crypto::hash> txids(1, start_txid);
    while (!coinbase)
    {
      LOG_PRINT_L0("Considering "<< txids.size() << " transaction(s) at depth " << depth);
      std::vector<crypto::hash> new_txids;
      for (const crypto::hash &txid: txids)
      {
        cryptonote::blobdata bd;
        if (!db->get_pruned_tx_blob(txid, bd))
        {
          LOG_PRINT_L0("Failed to get txid " << txid << " from db");
          return 1;
        }
        cryptonote::transaction tx;
        if (!cryptonote::parse_and_validate_tx_base_from_blob(bd, tx))
        {
          LOG_PRINT_L0("Bad tx: " << txid);
          return 1;
        }
        for (size_t ring = 0; ring < tx.vin.size(); ++ring)
        {
          if (tx.vin[ring].type() == typeid(cryptonote::txin_gen))
          {
            MDEBUG(txid << " is a coinbase transaction");
            coinbase = true;
            goto done;
          }
          if (tx.vin[ring].type() == typeid(cryptonote::txin_to_key))
          {
            const cryptonote::txin_to_key &txin = boost::get<cryptonote::txin_to_key>(tx.vin[ring]);
            const uint64_t amount = txin.amount;
            auto absolute_offsets = cryptonote::relative_output_offsets_to_absolute(txin.key_offsets);
            for (uint64_t offset: absolute_offsets)
            {
              const output_data_t od = db->get_output_key(amount, offset).data;
              const crypto::hash block_hash = db->get_block_hash_from_height(od.height);
              bd = db->get_block_blob(block_hash);
              cryptonote::block b;
              if (!cryptonote::parse_and_validate_block_from_blob(bd, b))
              {
                LOG_PRINT_L0("Bad block from db");
                return 1;
              }
              // find the tx which created this output
              bool found = false;
              for (size_t out = 0; out < b.miner_tx.vout.size(); ++out)
              {
                if (b.miner_tx.vout[out].target.type() == typeid(cryptonote::txout_to_key))
                {
                  const auto &txout = boost::get<cryptonote::txout_to_key>(b.miner_tx.vout[out].target);
                  if (txout.key == od.pubkey)
                  {
                    found = true;
                    new_txids.push_back(cryptonote::get_transaction_hash(b.miner_tx));
                    MDEBUG("adding txid: " << cryptonote::get_transaction_hash(b.miner_tx));
                    break;
                  }
                }
                else
                {
                  LOG_PRINT_L0("Bad vout type in txid " << cryptonote::get_transaction_hash(b.miner_tx));
                  return 1;
                }
              }
              for (const crypto::hash &block_txid: b.tx_hashes)
              {
                if (found)
                  break;
                if (!db->get_pruned_tx_blob(block_txid, bd))
                {
                  LOG_PRINT_L0("Failed to get txid " << block_txid << " from db");
                  return 1;
                }
                cryptonote::transaction tx2;
                if (!cryptonote::parse_and_validate_tx_base_from_blob(bd, tx2))
                {
                  LOG_PRINT_L0("Bad tx: " << block_txid);
                  return 1;
                }
                for (size_t out = 0; out < tx2.vout.size(); ++out)
                {
                  if (tx2.vout[out].target.type() == typeid(cryptonote::txout_to_key))
                  {
                    const auto &txout = boost::get<cryptonote::txout_to_key>(tx2.vout[out].target);
                    if (txout.key == od.pubkey)
                    {
                      found = true;
                      new_txids.push_back(block_txid);
                      MDEBUG("adding txid: " << block_txid);
                      break;
                    }
                  }
                  else
                  {
                    LOG_PRINT_L0("Bad vout type in txid " << block_txid);
                    return 1;
                  }
                }
              }
              if (!found)
              {
                LOG_PRINT_L0("Output originating transaction not found");
                return 1;
              }
            }
          }
          else
          {
            LOG_PRINT_L0("Bad vin type in txid " << txid);
            return 1;
          }
        }
      }
      if (!coinbase)
      {
        std::swap(txids, new_txids);
        ++depth;
      }
    }
done:
    LOG_PRINT_L0("Min depth for txid " << start_txid << ": " << depth);
    depths.push_back(depth);
  }

  uint64_t cumulative_depth = 0;
  for (uint64_t depth: depths)
    cumulative_depth += depth;
  LOG_PRINT_L0("Average min depth for " << start_txids.size() << " transaction(s): " << cumulative_depth/(float)depths.size());
  LOG_PRINT_L0("Median min depth for " << start_txids.size() << " transaction(s): " << epee::misc_utils::median(depths));

  core_storage->blockchain.deinit();
  return 0;

  CATCH_ENTRY("Depth query error", 1);
}
