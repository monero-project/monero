// Copyright (c) 2014-2018, The Monero Project
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
#include "blockchain_db/db_types.h"
#include "version.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "bcutil"

namespace po = boost::program_options;
using namespace epee;
using namespace cryptonote;

struct output_data
{
  uint64_t amount;
  uint64_t index;
  mutable bool coinbase;
  mutable uint64_t height;
  output_data(uint64_t a, uint64_t i, bool cb, uint64_t h): amount(a), index(i), coinbase(cb), height(h) {}
  bool operator==(const output_data &other) const { return other.amount == amount && other.index == index; }
  void info(bool c, uint64_t h) const { coinbase = c; height =h; }
};
namespace std
{
  template<> struct hash<output_data>
  {
    size_t operator()(const output_data &od) const
    {
      const uint64_t data[2] = {od.amount, od.index};
      crypto::hash h;
      crypto::cn_fast_hash(data, 2 * sizeof(uint64_t), h);
      return reinterpret_cast<const std::size_t &>(h);
    }
  };
}

struct reference
{
  uint64_t height;
  uint64_t ring_size;
  uint64_t position;
  reference(uint64_t h, uint64_t rs, uint64_t p): height(h), ring_size(rs), position(p) {}
};

int main(int argc, char* argv[])
{
  TRY_ENTRY();

  epee::string_tools::set_module_name_and_folder(argv[0]);

  std::string default_db_type = "lmdb";

  std::string available_dbs = cryptonote::blockchain_db_types(", ");
  available_dbs = "available: " + available_dbs;

  uint32_t log_level = 0;

  tools::on_startup();

  boost::filesystem::path output_file_path;

  po::options_description desc_cmd_only("Command line options");
  po::options_description desc_cmd_sett("Command line options and settings options");
  const command_line::arg_descriptor<std::string> arg_log_level  = {"log-level",  "0-4 or categories", ""};
  const command_line::arg_descriptor<std::string> arg_database = {
    "database", available_dbs.c_str(), default_db_type
  };
  const command_line::arg_descriptor<bool> arg_rct_only  = {"rct-only", "Only work on ringCT outputs", false};
  const command_line::arg_descriptor<std::string> arg_input = {"input", ""};

  command_line::add_arg(desc_cmd_sett, cryptonote::arg_testnet_on);
  command_line::add_arg(desc_cmd_sett, cryptonote::arg_stagenet_on);
  command_line::add_arg(desc_cmd_sett, arg_log_level);
  command_line::add_arg(desc_cmd_sett, arg_database);
  command_line::add_arg(desc_cmd_sett, arg_rct_only);
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

  mlog_configure(mlog_get_default_log_path("monero-blockchain-usage.log"), true);
  if (!command_line::is_arg_defaulted(vm, arg_log_level))
    mlog_set_log(command_line::get_arg(vm, arg_log_level).c_str());
  else
    mlog_set_log(std::string(std::to_string(log_level) + ",bcutil:INFO").c_str());

  LOG_PRINT_L0("Starting...");

  bool opt_testnet = command_line::get_arg(vm, cryptonote::arg_testnet_on);
  bool opt_stagenet = command_line::get_arg(vm, cryptonote::arg_stagenet_on);
  network_type net_type = opt_testnet ? TESTNET : opt_stagenet ? STAGENET : MAINNET;
  bool opt_rct_only = command_line::get_arg(vm, arg_rct_only);

  std::string db_type = command_line::get_arg(vm, arg_database);
  if (!cryptonote::blockchain_valid_db_type(db_type))
  {
    std::cerr << "Invalid database type: " << db_type << std::endl;
    return 1;
  }

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
  BlockchainDB* db = new_db(db_type);
  if (db == NULL)
  {
    LOG_ERROR("Attempted to use non-existent database type: " << db_type);
    throw std::runtime_error("Attempting to use non-existent database type");
  }
  LOG_PRINT_L0("database: " << db_type);

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

  LOG_PRINT_L0("Building usage patterns...");

  size_t done = 0;
  std::unordered_map<output_data, std::list<reference>> outputs;
  std::unordered_map<uint64_t,uint64_t> indices;

  LOG_PRINT_L0("Reading blockchain from " << input);
  core_storage->for_all_transactions([&](const crypto::hash &hash, const cryptonote::transaction &tx)->bool
  {
    const bool coinbase = tx.vin.size() == 1 && tx.vin[0].type() == typeid(txin_gen);
    const uint64_t height = core_storage->get_db().get_tx_block_height(hash);

    // create new outputs
    for (const auto &out: tx.vout)
    {
      if (opt_rct_only && out.amount)
        continue;
      uint64_t index = indices[out.amount]++;
      output_data od(out.amount, indices[out.amount], coinbase, height);
      auto itb = outputs.emplace(od, std::list<reference>());
      itb.first->first.info(coinbase, height);
    }

    for (const auto &in: tx.vin)
    {
      if (in.type() != typeid(txin_to_key))
        continue;
      const auto &txin = boost::get<txin_to_key>(in);
      if (opt_rct_only && txin.amount != 0)
        continue;

      const std::vector<uint64_t> absolute = cryptonote::relative_output_offsets_to_absolute(txin.key_offsets);
      for (size_t n = 0; n < txin.key_offsets.size(); ++n)
      {
        output_data od(txin.amount, absolute[n], coinbase, height);
        outputs[od].push_back(reference(height, txin.key_offsets.size(), n));
      }
    }
    return true;
  });

  std::unordered_map<uint64_t, uint64_t> counts;
  size_t total = 0;
  for (const auto &out: outputs)
  {
    counts[out.second.size()]++;
    total++;
  }
  for (const auto &c: counts)
  {
    float percent = 100.f * c.second / total;
    MINFO(std::to_string(c.second) << " outputs used " << c.first << " times (" << percent << "%)");
  }

  LOG_PRINT_L0("Blockchain usage exported OK");
  return 0;

  CATCH_ENTRY("Export error", 1);
}
