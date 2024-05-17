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

#include <unordered_map>
#include <unordered_set>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/archive/portable_binary_iarchive.hpp>
#include <boost/archive/portable_binary_oarchive.hpp>
#include <boost/filesystem/path.hpp>
#include "common/unordered_containers_boost_serialization.h"
#include "common/command_line.h"
#include "common/varint.h"
#include "cryptonote_basic/cryptonote_boost_serialization.h"
#include "cryptonote_core/cryptonote_core.h"
#include "version.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "bcutil"

namespace po = boost::program_options;
using namespace epee;
using namespace cryptonote;

static bool stop_requested = false;
static uint64_t cached_txes = 0, cached_blocks = 0, cached_outputs = 0, total_txes = 0, total_blocks = 0, total_outputs = 0;
static bool opt_cache_outputs = false, opt_cache_txes = false, opt_cache_blocks = false;

struct ancestor
{
  uint64_t amount;
  uint64_t offset;

  bool operator==(const ancestor &other) const { return amount == other.amount && offset == other.offset; }

  template <typename t_archive> void serialize(t_archive &a, const unsigned int ver)
  {
    a & amount;
    a & offset;
  }
};
BOOST_CLASS_VERSION(ancestor, 0)

namespace std
{
  template<> struct hash<ancestor>
  {
    size_t operator()(const ancestor &a) const
    {
      return a.amount ^ a.offset; // not that bad, since amount almost always have a high bit set, and offset doesn't
    }
  };
}

struct tx_data_t
{
  std::vector<std::pair<uint64_t, std::vector<uint64_t>>> vin;
  std::vector<crypto::public_key> vout;
  bool coinbase;

  tx_data_t(): coinbase(false) {}
  tx_data_t(const cryptonote::transaction &tx)
  {
    coinbase = tx.vin.size() == 1 && tx.vin[0].type() == typeid(cryptonote::txin_gen);
    if (!coinbase)
    {
      vin.reserve(tx.vin.size());
      for (size_t ring = 0; ring < tx.vin.size(); ++ring)
      {
        if (tx.vin[ring].type() == typeid(cryptonote::txin_to_key))
        {
          const cryptonote::txin_to_key &txin = boost::get<cryptonote::txin_to_key>(tx.vin[ring]);
          vin.push_back(std::make_pair(txin.amount, cryptonote::relative_output_offsets_to_absolute(txin.key_offsets)));
        }
        else
        {
          LOG_PRINT_L0("Bad vin type in txid " << get_transaction_hash(tx));
          throw std::runtime_error("Bad vin type");
        }
      }
    }
    vout.reserve(tx.vout.size());
    for (size_t out = 0; out < tx.vout.size(); ++out)
    {
      if (tx.vout[out].target.type() == typeid(cryptonote::txout_to_key))
      {
        const auto &txout = boost::get<cryptonote::txout_to_key>(tx.vout[out].target);
        vout.push_back(txout.key);
      }
      else
      {
        LOG_PRINT_L0("Bad vout type in txid " << get_transaction_hash(tx));
        throw std::runtime_error("Bad vout type");
      }
    }
  }

  template <typename t_archive> void serialize(t_archive &a, const unsigned int ver)
  {
    a & coinbase;
    a & vin;
    a & vout;
  }
};

struct ancestry_state_t
{
  uint64_t height;
  std::unordered_map<crypto::hash, std::unordered_set<ancestor>> ancestry;
  std::unordered_map<ancestor, crypto::hash> output_cache;
  std::unordered_map<crypto::hash, ::tx_data_t> tx_cache;
  std::vector<cryptonote::block> block_cache;

  ancestry_state_t(): height(0) {}

  template <typename t_archive> void serialize(t_archive &a, const unsigned int ver)
  {
    a & height;
    a & ancestry;
    a & output_cache;
    if (ver < 1)
    {
      std::unordered_map<crypto::hash, cryptonote::transaction> old_tx_cache;
      a & old_tx_cache;
      for (const auto& i: old_tx_cache)
        tx_cache.insert(std::make_pair(i.first, ::tx_data_t(i.second)));
    }
    else
    {
      a & tx_cache;
    }
    if (ver < 2)
    {
      std::unordered_map<uint64_t, cryptonote::block> old_block_cache;
      a & old_block_cache;
      block_cache.resize(old_block_cache.size());
      for (const auto& i: old_block_cache)
        block_cache[i.first] = i.second;
    }
    else
    {
      a & block_cache;
    }
  }
};
BOOST_CLASS_VERSION(ancestry_state_t, 2)

static void add_ancestor(std::unordered_map<ancestor, unsigned int> &ancestry, uint64_t amount, uint64_t offset)
{
  std::pair<std::unordered_map<ancestor, unsigned int>::iterator, bool> p = ancestry.insert(std::make_pair(ancestor{amount, offset}, 1));
  if (!p.second)
  {
    ++p.first->second;
  }
}

static size_t get_full_ancestry(const std::unordered_map<ancestor, unsigned int> &ancestry)
{
  size_t count = 0;
  for (const auto &i: ancestry)
    count += i.second;
  return count;
}

static size_t get_deduplicated_ancestry(const std::unordered_map<ancestor, unsigned int> &ancestry)
{
  return ancestry.size();
}

static void add_ancestry(std::unordered_map<crypto::hash, std::unordered_set<ancestor>> &ancestry, const crypto::hash &txid, const std::unordered_set<ancestor> &ancestors)
{
  std::pair<std::unordered_map<crypto::hash, std::unordered_set<ancestor>>::iterator, bool> p = ancestry.insert(std::make_pair(txid, ancestors));
  if (!p.second)
  {
    for (const auto &e: ancestors)
      p.first->second.insert(e);
  }
}

static void add_ancestry(std::unordered_map<crypto::hash, std::unordered_set<ancestor>> &ancestry, const crypto::hash &txid, const ancestor &new_ancestor)
{
  std::pair<std::unordered_map<crypto::hash, std::unordered_set<ancestor>>::iterator, bool> p = ancestry.insert(std::make_pair(txid, std::unordered_set<ancestor>()));
  p.first->second.insert(new_ancestor);
}

static std::unordered_set<ancestor> get_ancestry(const std::unordered_map<crypto::hash, std::unordered_set<ancestor>> &ancestry, const crypto::hash &txid)
{
  std::unordered_map<crypto::hash, std::unordered_set<ancestor>>::const_iterator i = ancestry.find(txid);
  if (i == ancestry.end())
  {
    //MERROR("txid ancestry not found: " << txid);
    //throw std::runtime_error("txid ancestry not found");
    return std::unordered_set<ancestor>();
  }
  return i->second;
}

static bool get_block_from_height(ancestry_state_t &state, BlockchainDB *db, uint64_t height, cryptonote::block &b)
{
  ++total_blocks;
  if (state.block_cache.size() > height && !state.block_cache[height].miner_tx.vin.empty())
  {
    ++cached_blocks;
    b = state.block_cache[height];
    return true;
  }
  cryptonote::blobdata bd = db->get_block_blob_from_height(height);
  if (!cryptonote::parse_and_validate_block_from_blob(bd, b))
  {
    LOG_PRINT_L0("Bad block from db");
    return false;
  }
  if (opt_cache_blocks)
  {
    state.block_cache.resize(height + 1);
    state.block_cache[height] = b;
  }
  return true;
}

static bool get_transaction(ancestry_state_t &state, BlockchainDB *db, const crypto::hash &txid, ::tx_data_t &tx_data)
{
  std::unordered_map<crypto::hash, ::tx_data_t>::const_iterator i = state.tx_cache.find(txid);
  ++total_txes;
  if (i != state.tx_cache.end())
  {
    ++cached_txes;
    tx_data = i->second;
    return true;
  }

  cryptonote::blobdata bd;
  if (!db->get_pruned_tx_blob(txid, bd))
  {
    LOG_PRINT_L0("Failed to get txid " << txid << " from db");
    return false;
  }
  cryptonote::transaction tx;
  if (!cryptonote::parse_and_validate_tx_base_from_blob(bd, tx))
  {
    LOG_PRINT_L0("Bad tx: " << txid);
    return false;
  }
  tx_data = ::tx_data_t(tx);
  if (opt_cache_txes)
    state.tx_cache.insert(std::make_pair(txid, tx_data));
  return true;
}

static bool get_output_txid(ancestry_state_t &state, BlockchainDB *db, uint64_t amount, uint64_t offset, crypto::hash &txid)
{
  ++total_outputs;
  std::unordered_map<ancestor, crypto::hash>::const_iterator i = state.output_cache.find({amount, offset});
  if (i != state.output_cache.end())
  {
    ++cached_outputs;
    txid = i->second;
    return true;
  }

  const output_data_t od = db->get_output_key(amount, offset, false).data;
  cryptonote::block b;
  if (!get_block_from_height(state, db, od.height, b))
    return false;

  for (size_t out = 0; out < b.miner_tx.vout.size(); ++out)
  {
    if (b.miner_tx.vout[out].target.type() == typeid(cryptonote::txout_to_key))
    {
      const auto &txout = boost::get<cryptonote::txout_to_key>(b.miner_tx.vout[out].target);
      if (txout.key == od.pubkey)
      {
        txid = cryptonote::get_transaction_hash(b.miner_tx);
        if (opt_cache_outputs)
          state.output_cache.insert(std::make_pair(ancestor{amount, offset}, txid));
        return true;
      }
    }
    else
    {
      LOG_PRINT_L0("Bad vout type in txid " << cryptonote::get_transaction_hash(b.miner_tx));
      return false;
    }
  }
  for (const crypto::hash &block_txid: b.tx_hashes)
  {
    ::tx_data_t tx_data3;
    if (!get_transaction(state, db, block_txid, tx_data3))
      return false;

    for (size_t out = 0; out < tx_data3.vout.size(); ++out)
    {
      if (tx_data3.vout[out] == od.pubkey)
      {
        txid = block_txid;
        if (opt_cache_outputs)
          state.output_cache.insert(std::make_pair(ancestor{amount, offset}, txid));
        return true;
      }
    }
  }
  return false;
}

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
  const command_line::arg_descriptor<std::string> arg_txid  = {"txid", "Get ancestry for this txid", ""};
  const command_line::arg_descriptor<std::string> arg_output  = {"output", "Get ancestry for this output (amount/offset format)", ""};
  const command_line::arg_descriptor<uint64_t> arg_height  = {"height", "Get ancestry for all txes at this height", 0};
  const command_line::arg_descriptor<bool> arg_refresh  = {"refresh", "Refresh the whole chain first", false};
  const command_line::arg_descriptor<bool> arg_cache_outputs  = {"cache-outputs", "Cache outputs (memory hungry)", false};
  const command_line::arg_descriptor<bool> arg_cache_txes  = {"cache-txes", "Cache txes (memory hungry)", false};
  const command_line::arg_descriptor<bool> arg_cache_blocks  = {"cache-blocks", "Cache blocks (memory hungry)", false};
  const command_line::arg_descriptor<bool> arg_include_coinbase  = {"include-coinbase", "Including coinbase tx in per height average", false};
  const command_line::arg_descriptor<bool> arg_show_cache_stats  = {"show-cache-stats", "Show cache statistics", false};

  command_line::add_arg(desc_cmd_sett, cryptonote::arg_data_dir);
  command_line::add_arg(desc_cmd_sett, cryptonote::arg_testnet_on);
  command_line::add_arg(desc_cmd_sett, cryptonote::arg_stagenet_on);
  command_line::add_arg(desc_cmd_sett, arg_log_level);
  command_line::add_arg(desc_cmd_sett, arg_txid);
  command_line::add_arg(desc_cmd_sett, arg_output);
  command_line::add_arg(desc_cmd_sett, arg_height);
  command_line::add_arg(desc_cmd_sett, arg_refresh);
  command_line::add_arg(desc_cmd_sett, arg_cache_outputs);
  command_line::add_arg(desc_cmd_sett, arg_cache_txes);
  command_line::add_arg(desc_cmd_sett, arg_cache_blocks);
  command_line::add_arg(desc_cmd_sett, arg_include_coinbase);
  command_line::add_arg(desc_cmd_sett, arg_show_cache_stats);
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

  mlog_configure(mlog_get_default_log_path("monero-blockchain-ancestry.log"), true);
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
  std::string opt_output_string = command_line::get_arg(vm, arg_output);
  uint64_t opt_height = command_line::get_arg(vm, arg_height);
  bool opt_refresh = command_line::get_arg(vm, arg_refresh);
  opt_cache_outputs = command_line::get_arg(vm, arg_cache_outputs);
  opt_cache_txes = command_line::get_arg(vm, arg_cache_txes);
  opt_cache_blocks = command_line::get_arg(vm, arg_cache_blocks);
  bool opt_include_coinbase = command_line::get_arg(vm, arg_include_coinbase);
  bool opt_show_cache_stats = command_line::get_arg(vm, arg_show_cache_stats);

  if ((!opt_txid_string.empty()) + !!opt_height + !opt_output_string.empty() > 1)
  {
    std::cerr << "Only one of --txid, --height, --output can be given" << std::endl;
    return 1;
  }
  crypto::hash opt_txid = crypto::null_hash;
  uint64_t output_amount = 0, output_offset = 0;
  if (!opt_txid_string.empty())
  {
    if (!epee::string_tools::hex_to_pod(opt_txid_string, opt_txid))
    {
      std::cerr << "Invalid txid" << std::endl;
      return 1;
    }
  }
  else if (!opt_output_string.empty())
  {
    if (sscanf(opt_output_string.c_str(), "%" SCNu64 "/%" SCNu64, &output_amount, &output_offset) != 2)
    {
      std::cerr << "Invalid output" << std::endl;
      return 1;
    }
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

  ancestry_state_t state;

  const std::string state_file_path = (boost::filesystem::path(opt_data_dir) / "ancestry-state.bin").string();
  LOG_PRINT_L0("Loading state data from " << state_file_path);
  std::ifstream state_data_in;
  state_data_in.open(state_file_path, std::ios_base::binary | std::ios_base::in);
  if (!state_data_in.fail())
  {
    try
    {
      boost::archive::portable_binary_iarchive a(state_data_in);
      a >> state;
    }
    catch (const std::exception &e)
    {
      MERROR("Failed to load state data from " << state_file_path << ", restarting from scratch");
      state = ancestry_state_t();
    }
    state_data_in.close();
  }

  tools::signal_handler::install([](int type) {
    stop_requested = true;
  });

  // forward method
  const uint64_t db_height = db->height();
  if (opt_refresh)
  {
    MINFO("Starting from height " << state.height);
    state.block_cache.reserve(db_height);
    for (uint64_t h = state.height; h < db_height; ++h)
    {
      size_t block_ancestry_size = 0;
      const cryptonote::blobdata bd = db->get_block_blob_from_height(h);
      ++total_blocks;
      cryptonote::block b;
      if (!cryptonote::parse_and_validate_block_from_blob(bd, b))
      {
        LOG_PRINT_L0("Bad block from db");
        return 1;
      }
      if (opt_cache_blocks)
      {
        state.block_cache.resize(h + 1);
        state.block_cache[h] = b;
      }
      std::vector<crypto::hash> txids;
      txids.reserve(1 + b.tx_hashes.size());
      if (opt_include_coinbase)
        txids.push_back(cryptonote::get_transaction_hash(b.miner_tx));
      for (const auto &h: b.tx_hashes)
        txids.push_back(h);
      for (const crypto::hash &txid: txids)
      {
        printf("%lu/%lu               \r", (unsigned long)h, (unsigned long)db_height);
        fflush(stdout);
        ::tx_data_t tx_data;
        std::unordered_map<crypto::hash, ::tx_data_t>::const_iterator i = state.tx_cache.find(txid);
        ++total_txes;
        if (i != state.tx_cache.end())
        {
          ++cached_txes;
          tx_data = i->second;
        }
        else
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
          tx_data = ::tx_data_t(tx);
          if (opt_cache_txes)
            state.tx_cache.insert(std::make_pair(txid, tx_data));
        }
        if (tx_data.coinbase)
        {
          add_ancestry(state.ancestry, txid, std::unordered_set<ancestor>());
        }
        else
        {
          for (size_t ring = 0; ring < tx_data.vin.size(); ++ring)
          {
            const uint64_t amount = tx_data.vin[ring].first;
            const std::vector<uint64_t> &absolute_offsets = tx_data.vin[ring].second;
            for (uint64_t offset: absolute_offsets)
            {
              add_ancestry(state.ancestry, txid, ancestor{amount, offset});
              // find the tx which created this output
              crypto::hash output_txid;
              if (!get_output_txid(state, db, amount, offset, output_txid))
              {
                LOG_PRINT_L0("Output originating transaction not found");
                return 1;
              }
              add_ancestry(state.ancestry, txid, get_ancestry(state.ancestry, output_txid));
            }
          }
        }
        const size_t ancestry_size = get_ancestry(state.ancestry, txid).size();
        block_ancestry_size += ancestry_size;
        MINFO(txid << ": " << ancestry_size);
      }
      if (!txids.empty())
      {
        std::string stats_msg;
        MINFO("Height " << h << ": " << (block_ancestry_size / txids.size()) << " average over " << txids.size() << stats_msg);
      }
      state.height = h;
      if (stop_requested)
        break;
    }

    LOG_PRINT_L0("Saving state data to " << state_file_path);
    std::ofstream state_data_out;
    state_data_out.open(state_file_path, std::ios_base::binary | std::ios_base::out | std::ios::trunc);
    if (!state_data_out.fail())
    {
      try
      {
        boost::archive::portable_binary_oarchive a(state_data_out);
        a << state;
      }
      catch (const std::exception &e)
      {
        MERROR("Failed to save state data to " << state_file_path);
      }
      state_data_out.close();
    }
  }
  else
  {
    if (state.height < db_height)
    {
      MWARNING("The state file is only built up to height " << state.height << ", but the blockchain reached height " << db_height);
      MWARNING("You may want to run with --refresh if you want to get ancestry for newer data");
    }
  }

  if (!opt_txid_string.empty())
  {
    start_txids.push_back(opt_txid);
  }
  else if (!opt_output_string.empty())
  {
    crypto::hash txid;
    if (!get_output_txid(state, db, output_amount, output_offset, txid))
    {
      LOG_PRINT_L0("Output not found in db");
      return 1;
    }
    start_txids.push_back(txid);
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
  }

  if (start_txids.empty())
  {
    LOG_PRINT_L0("No transaction(s) to check");
    return 1;
  }

  for (const crypto::hash &start_txid: start_txids)
  {
    LOG_PRINT_L0("Checking ancestry for txid " << start_txid);

    std::unordered_map<ancestor, unsigned int> ancestry;

    std::list<crypto::hash> txids;
    txids.push_back(start_txid);
    while (!txids.empty())
    {
      const crypto::hash txid = txids.front();
      txids.pop_front();

      if (stop_requested)
        goto done;

      ::tx_data_t tx_data2;
      if (!get_transaction(state, db, txid, tx_data2))
        return 1;

      const bool coinbase = tx_data2.coinbase;
      if (coinbase)
        continue;

      for (size_t ring = 0; ring < tx_data2.vin.size(); ++ring)
      {
        {
          const uint64_t amount = tx_data2.vin[ring].first;
          auto absolute_offsets = tx_data2.vin[ring].second;
          for (uint64_t offset: absolute_offsets)
          {
            add_ancestor(ancestry, amount, offset);

            // find the tx which created this output
            crypto::hash output_txid;
            if (!get_output_txid(state, db, amount, offset, output_txid))
            {
              LOG_PRINT_L0("Output originating transaction not found");
              return 1;
            }

            add_ancestry(state.ancestry, txid, get_ancestry(state.ancestry, output_txid));
            txids.push_back(output_txid);
            MDEBUG("adding txid: " << output_txid);
          }
        }
      }
    }

    MINFO("Ancestry for " << start_txid << ": " << get_deduplicated_ancestry(ancestry) << " / " << get_full_ancestry(ancestry));
    for (const auto &i: ancestry)
    {
      MINFO(cryptonote::print_money(i.first.amount) << "/" << i.first.offset << ": " << i.second);
    }
  }

done:
  core_storage->blockchain.deinit();

  if (opt_show_cache_stats)
  MINFO("cache: txes " << std::to_string(cached_txes*100./total_txes)
        << "%, blocks " << std::to_string(cached_blocks*100./total_blocks)
        << "%, outputs " << std::to_string(cached_outputs*100./total_outputs)
        << "%");

  return 0;

  CATCH_ENTRY("Depth query error", 1);
}
