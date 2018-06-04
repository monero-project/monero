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
#include "wallet/ringdb.h"
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
  output_data(uint64_t a, uint64_t i): amount(a), index(i) {}
  bool operator==(const output_data &other) const { return other.amount == amount && other.index == index; }
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

static std::string get_default_db_path()
{
  boost::filesystem::path dir = tools::get_default_data_dir();
  // remove .bitmonero, replace with .shared-ringdb
  dir = dir.remove_filename();
  dir /= ".shared-ringdb";
  return dir.string();
}

static bool for_all_transactions(const std::string &filename, const std::function<bool(const cryptonote::transaction_prefix&)> &f)
{
  MDB_env *env;
  MDB_dbi dbi;
  MDB_txn *txn;
  MDB_cursor *cur;
  int dbr;
  bool tx_active = false;

  dbr = mdb_env_create(&env);
  if (dbr) throw std::runtime_error("Failed to create LDMB environment: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_env_set_maxdbs(env, 2);
  if (dbr) throw std::runtime_error("Failed to set max env dbs: " + std::string(mdb_strerror(dbr)));
  const std::string actual_filename = filename;
  dbr = mdb_env_open(env, actual_filename.c_str(), 0, 0664);
  if (dbr) throw std::runtime_error("Failed to open rings database file '"
      + actual_filename + "': " + std::string(mdb_strerror(dbr)));

  dbr = mdb_txn_begin(env, NULL, 0, &txn);
  if (dbr) throw std::runtime_error("Failed to create LMDB transaction: " + std::string(mdb_strerror(dbr)));
  epee::misc_utils::auto_scope_leave_caller txn_dtor = epee::misc_utils::create_scope_leave_handler([&](){if (tx_active) mdb_txn_abort(txn);});
  tx_active = true;

  dbr = mdb_dbi_open(txn, "txs_pruned", MDB_INTEGERKEY, &dbi);
  if (dbr)
    dbr = mdb_dbi_open(txn, "txs", MDB_INTEGERKEY, &dbi);
  if (dbr) throw std::runtime_error("Failed to open LMDB dbi: " + std::string(mdb_strerror(dbr)));
  dbr = mdb_cursor_open(txn, dbi, &cur);
  if (dbr) throw std::runtime_error("Failed to create LMDB cursor: " + std::string(mdb_strerror(dbr)));

  MDB_val k;
  MDB_val v;
  bool fret = true;

  MDB_cursor_op op = MDB_FIRST;
  while (1)
  {
    int ret = mdb_cursor_get(cur, &k, &v, op);
    op = MDB_NEXT;
    if (ret == MDB_NOTFOUND)
      break;
    if (ret)
      throw std::runtime_error("Failed to enumerate transactions: " + std::string(mdb_strerror(ret)));

    cryptonote::transaction_prefix tx;
    blobdata bd;
    bd.assign(reinterpret_cast<char*>(v.mv_data), v.mv_size);
    std::stringstream ss;
    ss << bd;
    binary_archive<false> ba(ss);
    bool r = do_serialize(ba, tx);
    CHECK_AND_ASSERT_MES(r, false, "Failed to parse transaction from blob");

    if (!f(tx)) {
      fret = false;
      break;
    }
  }

  mdb_cursor_close(cur);
  mdb_txn_commit(txn);
  tx_active = false;
  mdb_dbi_close(env, dbi);
  mdb_env_close(env);
  return fret;
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
  const command_line::arg_descriptor<std::string, false, true, 2> arg_blackball_db_dir = {
      "blackball-db-dir", "Specify blackball database directory",
      get_default_db_path(),
      {{ &arg_testnet_on, &arg_stagenet_on }},
      [](std::array<bool, 2> testnet_stagenet, bool defaulted, std::string val)->std::string {
        if (testnet_stagenet[0])
          return (boost::filesystem::path(val) / "testnet").string();
        else if (testnet_stagenet[1])
          return (boost::filesystem::path(val) / "stagenet").string();
        return val;
      }
  };
  const command_line::arg_descriptor<std::string> arg_log_level  = {"log-level",  "0-4 or categories", ""};
  const command_line::arg_descriptor<std::string> arg_database = {
    "database", available_dbs.c_str(), default_db_type
  };
  const command_line::arg_descriptor<bool> arg_rct_only  = {"rct-only", "Only work on ringCT outputs", false};
  const command_line::arg_descriptor<std::vector<std::string> > arg_inputs = {"inputs", "Path to Monero DB, and path to any fork DBs"};

  command_line::add_arg(desc_cmd_sett, arg_blackball_db_dir);
  command_line::add_arg(desc_cmd_sett, cryptonote::arg_testnet_on);
  command_line::add_arg(desc_cmd_sett, cryptonote::arg_stagenet_on);
  command_line::add_arg(desc_cmd_sett, arg_log_level);
  command_line::add_arg(desc_cmd_sett, arg_database);
  command_line::add_arg(desc_cmd_sett, arg_rct_only);
  command_line::add_arg(desc_cmd_sett, arg_inputs);
  command_line::add_arg(desc_cmd_only, command_line::arg_help);

  po::options_description desc_options("Allowed options");
  desc_options.add(desc_cmd_only).add(desc_cmd_sett);

  po::positional_options_description positional_options;
  positional_options.add(arg_inputs.name, -1);

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

  mlog_configure(mlog_get_default_log_path("monero-blockchain-blackball.log"), true);
  if (!command_line::is_arg_defaulted(vm, arg_log_level))
    mlog_set_log(command_line::get_arg(vm, arg_log_level).c_str());
  else
    mlog_set_log(std::string(std::to_string(log_level) + ",bcutil:INFO").c_str());

  LOG_PRINT_L0("Starting...");

  bool opt_testnet = command_line::get_arg(vm, cryptonote::arg_testnet_on);
  bool opt_stagenet = command_line::get_arg(vm, cryptonote::arg_stagenet_on);
  network_type net_type = opt_testnet ? TESTNET : opt_stagenet ? STAGENET : MAINNET;
  output_file_path = command_line::get_arg(vm, arg_blackball_db_dir);
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
  const std::vector<std::string> inputs = command_line::get_arg(vm, arg_inputs);
  if (inputs.empty())
  {
    LOG_PRINT_L0("No inputs given");
    return 1;
  }
  std::vector<std::unique_ptr<Blockchain>> core_storage(inputs.size());
  Blockchain *blockchain = NULL;
  tx_memory_pool m_mempool(*blockchain);
  for (size_t n = 0; n < inputs.size(); ++n)
  {
    core_storage[n].reset(new Blockchain(m_mempool));

    BlockchainDB* db = new_db(db_type);
    if (db == NULL)
    {
      LOG_ERROR("Attempted to use non-existent database type: " << db_type);
      throw std::runtime_error("Attempting to use non-existent database type");
    }
    LOG_PRINT_L0("database: " << db_type);

    std::string filename = inputs[n];
    while (boost::ends_with(filename, "/") || boost::ends_with(filename, "\\"))
      filename.pop_back();
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
    r = core_storage[n]->init(db, net_type);

    CHECK_AND_ASSERT_MES(r, 1, "Failed to initialize source blockchain storage");
    LOG_PRINT_L0("Source blockchain storage initialized OK");
  }

  boost::filesystem::path direc(output_file_path.string());
  if (boost::filesystem::exists(direc))
  {
    if (!boost::filesystem::is_directory(direc))
    {
      MERROR("LMDB needs a directory path, but a file was passed: " << output_file_path.string());
      return 1;
    }
  }
  else
  {
    if (!boost::filesystem::create_directories(direc))
    {
      MERROR("Failed to create directory: " << output_file_path.string());
      return 1;
    }
  }

  LOG_PRINT_L0("Scanning for blackballable outputs...");

  size_t done = 0;
  std::unordered_map<crypto::key_image, std::vector<uint64_t>> relative_rings;
  std::unordered_map<output_data, std::unordered_set<crypto::key_image>> outputs;
  std::unordered_set<output_data> spent, newly_spent;

  cryptonote::block b = core_storage[0]->get_db().get_block_from_height(0);
  tools::ringdb ringdb(output_file_path.string(), epee::string_tools::pod_to_hex(get_block_hash(b)));

  for (size_t n = 0; n < inputs.size(); ++n)
  {
    LOG_PRINT_L0("Reading blockchain from " << inputs[n]);
    for_all_transactions(inputs[n], [&](const cryptonote::transaction_prefix &tx)->bool
    {
      for (const auto &in: tx.vin)
      {
        if (in.type() != typeid(txin_to_key))
          continue;
        const auto &txin = boost::get<txin_to_key>(in);
        if (opt_rct_only && txin.amount != 0)
          continue;

        const std::vector<uint64_t> absolute = cryptonote::relative_output_offsets_to_absolute(txin.key_offsets);
        if (n == 0)
          for (uint64_t out: absolute)
            outputs[output_data(txin.amount, out)].insert(txin.k_image);

        std::vector<uint64_t> new_ring = txin.key_offsets;
        const uint32_t ring_size = txin.key_offsets.size();
        if (ring_size == 1)
        {
          const crypto::public_key pkey = core_storage[n]->get_output_key(txin.amount, txin.key_offsets[0]);
          MINFO("Blackballing output " << pkey << ", due to being used in a 1-ring");
          ringdb.blackball(pkey);
          newly_spent.insert(output_data(txin.amount, txin.key_offsets[0]));
          spent.insert(output_data(txin.amount, txin.key_offsets[0]));
        }
        else if (relative_rings.find(txin.k_image) != relative_rings.end())
        {
          MINFO("Key image " << txin.k_image << " already seen: rings " <<
              boost::join(relative_rings[txin.k_image] | boost::adaptors::transformed([](uint64_t out){return std::to_string(out);}), " ") <<
              ", " << boost::join(txin.key_offsets | boost::adaptors::transformed([](uint64_t out){return std::to_string(out);}), " "));
          if (relative_rings[txin.k_image] != txin.key_offsets)
          {
            MINFO("Rings are different");
            const std::vector<uint64_t> r0 = cryptonote::relative_output_offsets_to_absolute(relative_rings[txin.k_image]);
            const std::vector<uint64_t> r1 = cryptonote::relative_output_offsets_to_absolute(txin.key_offsets);
            std::vector<uint64_t> common;
            for (uint64_t out: r0)
            {
              if (std::find(r1.begin(), r1.end(), out) != r1.end())
                common.push_back(out);
            }
            if (common.empty())
            {
              MERROR("Rings for the same key image are disjoint");
            }
            else if (common.size() == 1)
            {
              const crypto::public_key pkey = core_storage[n]->get_output_key(txin.amount, common[0]);
              MINFO("Blackballing output " << pkey << ", due to being used in rings with a single common element");
              ringdb.blackball(pkey);
              newly_spent.insert(output_data(txin.amount, common[0]));
              spent.insert(output_data(txin.amount, common[0]));
            }
            else
            {
              MINFO("The intersection has more than one element, it's still ok");
              for (const auto &out: r0)
                if (std::find(common.begin(), common.end(), out) != common.end())
                  new_ring.push_back(out);
              new_ring = cryptonote::absolute_output_offsets_to_relative(new_ring);
            }
          }
        }
        relative_rings[txin.k_image] = new_ring;
      }
      return true;
    });
  }

  while (!newly_spent.empty())
  {
    LOG_PRINT_L0("Secondary pass due to " << newly_spent.size() << " newly found spent outputs");
    std::unordered_set<output_data> work_spent = std::move(newly_spent);
    newly_spent.clear();

    for (const output_data &od: work_spent)
    {
      for (const crypto::key_image &ki: outputs[od])
      {
        std::vector<uint64_t> absolute = cryptonote::relative_output_offsets_to_absolute(relative_rings[ki]);
        size_t known = 0;
        uint64_t last_unknown = 0;
        for (uint64_t out: absolute)
        {
          output_data new_od(od.amount, out);
          if (spent.find(new_od) != spent.end())
            ++known;
          else
            last_unknown = out;
        }
        if (known == absolute.size() - 1)
        {
          const crypto::public_key pkey = core_storage[0]->get_output_key(od.amount, last_unknown);
          MINFO("Blackballing output " << pkey << ", due to being used in a " <<
              absolute.size() << "-ring where all other outputs are known to be spent");
          ringdb.blackball(pkey);
          newly_spent.insert(output_data(od.amount, last_unknown));
          spent.insert(output_data(od.amount, last_unknown));
        }
      }
    }
  }

  LOG_PRINT_L0("Blockchain blackball data exported OK");
  return 0;

  CATCH_ENTRY("Export error", 1);
}
