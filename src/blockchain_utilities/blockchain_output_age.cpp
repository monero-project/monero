// Copyright (c) 2018, The Monero Project
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
#include <boost/algorithm/string.hpp>
#include "common/command_line.h"
#include "common/varint.h"
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

static std::unordered_map<uint64_t, std::deque<uint64_t>> outputs;

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
  const command_line::arg_descriptor<bool> arg_rct_only  = {"rct-only", "Only work on ringCT outputs", false};
  const command_line::arg_descriptor<std::string> arg_separator  = {"separator", "Field separator", "\t"};
  const command_line::arg_descriptor<std::string> arg_input = {"input", ""};

  command_line::add_arg(desc_cmd_sett, cryptonote::arg_testnet_on);
  command_line::add_arg(desc_cmd_sett, cryptonote::arg_stagenet_on);
  command_line::add_arg(desc_cmd_sett, arg_log_level);
  command_line::add_arg(desc_cmd_sett, arg_rct_only);
  command_line::add_arg(desc_cmd_sett, arg_separator);
  command_line::add_arg(desc_cmd_sett, arg_input);
  command_line::add_arg(desc_cmd_only, command_line::arg_help);

  po::options_description desc_options("Allowed options");
  desc_options.add(desc_cmd_only).add(desc_cmd_sett);

  po::positional_options_description positional_options;
  positional_options.add(arg_input.name, -1);

  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_options, [&]()
  {
    auto parser = po::command_line_parser(argc, argv).options(desc_options).positional(positional_options);
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

  mlog_configure(mlog_get_default_log_path("monero-blockchain-output-age.log"), true);
  if (!command_line::is_arg_defaulted(vm, arg_log_level))
    mlog_set_log(command_line::get_arg(vm, arg_log_level).c_str());
  else
    mlog_set_log(std::string(std::to_string(log_level) + ",bcutil:INFO").c_str());

  LOG_PRINT_L0("Starting...");

  bool opt_testnet = command_line::get_arg(vm, cryptonote::arg_testnet_on);
  bool opt_stagenet = command_line::get_arg(vm, cryptonote::arg_stagenet_on);
  network_type net_type = opt_testnet ? TESTNET : opt_stagenet ? STAGENET : MAINNET;
  bool opt_rct_only = command_line::get_arg(vm, arg_rct_only);
  std::string sep = command_line::get_arg(vm, arg_separator);

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
  const std::string input = command_line::get_arg(vm, arg_input);
  std::unique_ptr<Blockchain> core_storage;
  tx_memory_pool m_mempool(*core_storage);
  core_storage.reset(new Blockchain(m_mempool));
  BlockchainDB* db = new_db();
  if (db == NULL)
  {
    throw std::runtime_error("Failed to creage db");
  }

  const std::string filename = input;
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

  LOG_PRINT_L0("Reading blockchain from " << input);

  uint64_t n_txes = 0;
  uint8_t version = 1;
  const uint64_t blockchain_height = core_storage->get_current_blockchain_height();
  for (uint64_t height = 0; height < blockchain_height; ++height)
  {
    cryptonote::blobdata bd = core_storage->get_db().get_block_blob_from_height(height);
    cryptonote::block b;
    if (!parse_and_validate_block_from_blob(bd, b))
    {
      MERROR("Failed to parse block at height " << height);
      return 1;
    }

    if (version != b.major_version)
    {
      MINFO("txes till v" << (unsigned)b.major_version << " (" << height << "): " << n_txes << " (without coinbases)");
      version = b.major_version;
    }
    n_txes += b.tx_hashes.size();

    for (const auto &out: b.miner_tx.vout)
    {
      uint64_t amount = out.amount;
      if (b.miner_tx.version >= 2)
        amount = 0;
      if (opt_rct_only && amount)
        continue;
      if (out.target.type() == typeid(txout_to_key))
      {
        outputs[amount].push_back(height);
      }
    }

    for (const crypto::hash &txid: b.tx_hashes)
    {
      if (!core_storage->get_db().get_tx_blob(txid, bd))
      {
        MERROR("Failed to find transaction " << txid);
        return 1;
      }
      cryptonote::transaction tx;
      if (!parse_and_validate_tx_base_from_blob(bd, tx))
      {
        MERROR("Failed to parse transaction at height " << height);
        return 1;
      }

      for (const auto &out: tx.vout)
      {
        if (opt_rct_only && out.amount)
          continue;
        if (out.target.type() == typeid(txout_to_key))
        {
          outputs[out.amount].push_back(height);
          MINFO("out" << sep << out.amount << sep << outputs[out.amount].size()-1 << sep << height << sep << b.timestamp);
        }
      }

      for (const auto &in: tx.vin)
      {
        if (in.type() == typeid(txin_to_key))
        {
          const cryptonote::txin_to_key &ink = boost::get<cryptonote::txin_to_key>(in);
          if (opt_rct_only && ink.amount)
            continue;
          std::stringstream str;
          str << "tx" << sep << height << sep << epee::string_tools::pod_to_hex(txid) << sep << ink.key_offsets.size();
          uint64_t offset = 0;
          for (uint64_t o: ink.key_offsets)
          {
            offset += o;
            str << sep << height - outputs[ink.amount][offset];
          }
          MINFO(str.str());
        }
      }
    }
  }

  LOG_PRINT_L0("Blockchain output age data exported OK");
  return 0;

  CATCH_ENTRY("Export error", 1);
}
