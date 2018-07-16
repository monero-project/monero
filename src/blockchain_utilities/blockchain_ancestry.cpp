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

#include <unordered_map>
#include <unordered_set>
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

struct ancestor
{
  uint64_t amount;
  uint64_t offset;

  bool operator==(const ancestor &other) const { return amount == other.amount && offset == other.offset; }
};

namespace std
{
  template<> struct hash<ancestor>
  {
    size_t operator()(const ancestor &a) const
    {
      crypto::hash h;
      crypto::cn_fast_hash(&a, sizeof(a), h);
      return reinterpret_cast<const std::size_t &>(h);
    }
  };
}

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
  const command_line::arg_descriptor<std::string> arg_txid  = {"txid", "Get ancestry for this txid", ""};
  const command_line::arg_descriptor<uint64_t> arg_height  = {"height", "Get ancestry for all txes at this height", 0};
  const command_line::arg_descriptor<bool> arg_all  = {"all", "Include the whole chain", false};
  const command_line::arg_descriptor<bool> arg_cache_outputs  = {"cache-outputs", "Cache outputs (memory hungry)", false};
  const command_line::arg_descriptor<bool> arg_cache_txes  = {"cache-txes", "Cache txes (memory hungry)", false};
  const command_line::arg_descriptor<bool> arg_cache_blocks  = {"cache-blocks", "Cache blocks (memory hungry)", false};
  const command_line::arg_descriptor<bool> arg_include_coinbase  = {"include-coinbase", "Including coinbase tx", false};
  const command_line::arg_descriptor<bool> arg_show_cache_stats  = {"show-cache-stats", "Show cache statistics", false};

  command_line::add_arg(desc_cmd_sett, cryptonote::arg_data_dir);
  command_line::add_arg(desc_cmd_sett, cryptonote::arg_testnet_on);
  command_line::add_arg(desc_cmd_sett, cryptonote::arg_stagenet_on);
  command_line::add_arg(desc_cmd_sett, arg_log_level);
  command_line::add_arg(desc_cmd_sett, arg_database);
  command_line::add_arg(desc_cmd_sett, arg_txid);
  command_line::add_arg(desc_cmd_sett, arg_height);
  command_line::add_arg(desc_cmd_sett, arg_all);
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
  uint64_t opt_height = command_line::get_arg(vm, arg_height);
  bool opt_all = command_line::get_arg(vm, arg_all);
  bool opt_cache_outputs = command_line::get_arg(vm, arg_cache_outputs);
  bool opt_cache_txes = command_line::get_arg(vm, arg_cache_txes);
  bool opt_cache_blocks = command_line::get_arg(vm, arg_cache_blocks);
  bool opt_include_coinbase = command_line::get_arg(vm, arg_include_coinbase);
  bool opt_show_cache_stats = command_line::get_arg(vm, arg_show_cache_stats);

  if ((!opt_txid_string.empty()) + !!opt_height + !!opt_all > 1)
  {
    std::cerr << "Only one of --txid, --height and --all can be given" << std::endl;
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
  std::unique_ptr<Blockchain> core_storage;
  tx_memory_pool m_mempool(*core_storage);
  core_storage.reset(new Blockchain(m_mempool));
  BlockchainDB *db = new_db(db_type);
  if (db == NULL)
  {
    LOG_ERROR("Attempted to use non-existent database type: " << db_type);
    throw std::runtime_error("Attempting to use non-existent database type");
  }
  LOG_PRINT_L0("database: " << db_type);

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

  std::vector<crypto::hash> start_txids;

  // forward method
  if (opt_all)
  {
    uint64_t cached_txes = 0, cached_blocks = 0, cached_outputs = 0, total_txes = 0, total_blocks = 0, total_outputs = 0;
    std::unordered_map<crypto::hash, std::unordered_set<ancestor>> ancestry;
    std::unordered_map<ancestor, crypto::hash> output_cache;
    std::unordered_map<crypto::hash, cryptonote::transaction> tx_cache;
    std::map<uint64_t, cryptonote::block> block_cache;

    const uint64_t db_height = db->height();
    for (uint64_t h = 0; h < db_height; ++h)
    {
      size_t block_ancestry_size = 0;
      const crypto::hash block_hash = db->get_block_hash_from_height(h);
      const cryptonote::blobdata bd = db->get_block_blob(block_hash);
      ++total_blocks;
      cryptonote::block b;
      if (!cryptonote::parse_and_validate_block_from_blob(bd, b))
      {
        LOG_PRINT_L0("Bad block from db");
        return 1;
      }
      if (opt_cache_blocks)
        block_cache.insert(std::make_pair(h, b));
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
        cryptonote::transaction tx;
        std::unordered_map<crypto::hash, cryptonote::transaction>::const_iterator i = tx_cache.find(txid);
        ++total_txes;
        if (i != tx_cache.end())
        {
          ++cached_txes;
          tx = i->second;
        }
        else
        {
          cryptonote::blobdata bd;
          if (!db->get_pruned_tx_blob(txid, bd))
          {
            LOG_PRINT_L0("Failed to get txid " << txid << " from db");
            return 1;
          }
          if (!cryptonote::parse_and_validate_tx_base_from_blob(bd, tx))
          {
            LOG_PRINT_L0("Bad tx: " << txid);
            return 1;
          }
          if (opt_cache_txes)
            tx_cache.insert(std::make_pair(txid, tx));
        }
        const bool coinbase = tx.vin.size() == 1 && tx.vin[0].type() == typeid(cryptonote::txin_gen);
        if (coinbase)
        {
          add_ancestry(ancestry, txid, std::unordered_set<ancestor>());
        }
        else
        {
          for (size_t ring = 0; ring < tx.vin.size(); ++ring)
          {
            if (tx.vin[ring].type() == typeid(cryptonote::txin_to_key))
            {
              const cryptonote::txin_to_key &txin = boost::get<cryptonote::txin_to_key>(tx.vin[ring]);
              const uint64_t amount = txin.amount;
              auto absolute_offsets = cryptonote::relative_output_offsets_to_absolute(txin.key_offsets);
              for (uint64_t offset: absolute_offsets)
              {
                const output_data_t od = db->get_output_key(amount, offset);
                add_ancestry(ancestry, txid, ancestor{amount, offset});
                cryptonote::block b;
                auto iblock = block_cache.find(od.height);
                ++total_blocks;
                if (iblock != block_cache.end())
                {
                  ++cached_blocks;
                  b = iblock->second;
                }
                else
                {
                  const crypto::hash block_hash = db->get_block_hash_from_height(od.height);
                  cryptonote::blobdata bd = db->get_block_blob(block_hash);
                  if (!cryptonote::parse_and_validate_block_from_blob(bd, b))
                  {
                    LOG_PRINT_L0("Bad block from db");
                    return 1;
                  }
                  if (opt_cache_blocks)
                    block_cache.insert(std::make_pair(od.height, b));
                }
                // find the tx which created this output
                bool found = false;
                std::unordered_map<ancestor, crypto::hash>::const_iterator i = output_cache.find({amount, offset});
                ++total_outputs;
                if (i != output_cache.end())
                {
                  ++cached_outputs;
                  add_ancestry(ancestry, txid, get_ancestry(ancestry, i->second));
                  found = true;
                }
                else for (size_t out = 0; out < b.miner_tx.vout.size(); ++out)
                {
                  if (b.miner_tx.vout[out].target.type() == typeid(cryptonote::txout_to_key))
                  {
                    const auto &txout = boost::get<cryptonote::txout_to_key>(b.miner_tx.vout[out].target);
                    if (txout.key == od.pubkey)
                    {
                      found = true;
                      add_ancestry(ancestry, txid, get_ancestry(ancestry, cryptonote::get_transaction_hash(b.miner_tx)));
                      if (opt_cache_outputs)
                        output_cache.insert(std::make_pair(ancestor{amount, offset}, cryptonote::get_transaction_hash(b.miner_tx)));
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
                  cryptonote::transaction tx2;
                  std::unordered_map<crypto::hash, cryptonote::transaction>::const_iterator i = tx_cache.find(block_txid);
                  ++total_txes;
                  if (i != tx_cache.end())
                  {
                    ++cached_txes;
                    tx2 = i->second;
                  }
                  else
                  {
                    cryptonote::blobdata bd;
                    if (!db->get_pruned_tx_blob(block_txid, bd))
                    {
                      LOG_PRINT_L0("Failed to get txid " << block_txid << " from db");
                      return 1;
                    }
                    if (!cryptonote::parse_and_validate_tx_base_from_blob(bd, tx2))
                    {
                      LOG_PRINT_L0("Bad tx: " << block_txid);
                      return 1;
                    }
                    if (opt_cache_txes)
                      tx_cache.insert(std::make_pair(block_txid, tx2));
                  }
                  for (size_t out = 0; out < tx2.vout.size(); ++out)
                  {
                    if (tx2.vout[out].target.type() == typeid(cryptonote::txout_to_key))
                    {
                      const auto &txout = boost::get<cryptonote::txout_to_key>(tx2.vout[out].target);
                      if (txout.key == od.pubkey)
                      {
                        found = true;
                        add_ancestry(ancestry, txid, get_ancestry(ancestry, block_txid));
                        if (opt_cache_outputs)
                          output_cache.insert(std::make_pair(ancestor{amount, offset}, block_txid));
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
        const size_t ancestry_size = get_ancestry(ancestry, txid).size();
        block_ancestry_size += ancestry_size;
        MINFO(txid << ": " << ancestry_size);
      }
      if (!txids.empty())
      {
        std::string stats_msg;
        if (opt_show_cache_stats)
          stats_msg = std::string(", cache: txes ") + std::to_string(cached_txes*100./total_txes)
              + ", blocks " + std::to_string(cached_blocks*100./total_blocks) + ", outputs "
              + std::to_string(cached_outputs*100./total_outputs);
        MINFO("Height " << h << ": " << (block_ancestry_size / txids.size()) << " average over " << txids.size() << stats_msg);
      }
    }
    goto done;
  }

  if (!opt_txid_string.empty())
  {
    start_txids.push_back(opt_txid);
  }
  else
  {
    const crypto::hash block_hash = db->get_block_hash_from_height(opt_height);
    const cryptonote::blobdata bd = db->get_block_blob(block_hash);
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
      const bool coinbase = tx.vin.size() == 1 && tx.vin[0].type() == typeid(cryptonote::txin_gen);
      if (coinbase)
        continue;

      for (size_t ring = 0; ring < tx.vin.size(); ++ring)
      {
        if (tx.vin[ring].type() == typeid(cryptonote::txin_to_key))
        {
          const cryptonote::txin_to_key &txin = boost::get<cryptonote::txin_to_key>(tx.vin[ring]);
          const uint64_t amount = txin.amount;
          auto absolute_offsets = cryptonote::relative_output_offsets_to_absolute(txin.key_offsets);
          for (uint64_t offset: absolute_offsets)
          {
            add_ancestor(ancestry, amount, offset);
            const output_data_t od = db->get_output_key(amount, offset);
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
                  txids.push_back(cryptonote::get_transaction_hash(b.miner_tx));
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
                    txids.push_back(block_txid);
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

    MINFO("Ancestry for " << start_txid << ": " << get_deduplicated_ancestry(ancestry) << " / " << get_full_ancestry(ancestry));
    for (const auto &i: ancestry)
    {
      MINFO(cryptonote::print_money(i.first.amount) << "/" << i.first.offset << ": " << i.second);
    }
  }

done:
  core_storage->deinit();
  return 0;

  CATCH_ENTRY("Depth query error", 1);
}
