// Copyright (c) 2024, The Monero Project
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

#include <filesystem>

#include "common/command_line.h"
#include "cryptonote_core/blockchain_and_pool.h"
#include "cryptonote_core/cryptonote_core.h"
#include "version.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "bcutil"

namespace po = boost::program_options;
using namespace epee;
using namespace cryptonote;

static std::atomic<bool> stop_requested = false;

enum class unlock_time_type_t
{
  Standard,         // coinbase or unlock_time == 0
  NonStandardBlock, // non-coinbase and 0 < unlock_time < CRYPTONOTE_MAX_BLOCK_NUMBER
  NonStandardUnix   // non-coinbase and unlock_time >= CRYPTONOTE_MAX_BLOCK_NUMBER
};

struct report_slot_entry_t
{
  uint32_t num_standard_locks_opened;
  uint32_t num_nonstandard_blocktime_locks_opened;
  uint32_t num_nonstandard_unixtime_locks_opened;
  uint32_t num_total_locks_opened;

  uint32_t num_standard_locks_closed;
  uint32_t num_nonstandard_blocktime_locks_closed;
  uint32_t num_nonstandard_unixtime_locks_closed;
  uint32_t num_total_locks_closed;

  uint32_t standard_locks_balance;
  uint32_t nonstandard_blocktime_locks_balance;
  uint32_t nonstandard_unixtime_locks_balance;
  uint32_t total_locks_balance;

  void tally_totals()
  {
    num_total_locks_opened = num_standard_locks_opened
      + num_nonstandard_blocktime_locks_opened
      + num_nonstandard_unixtime_locks_opened;
    
    num_total_locks_closed = num_standard_locks_closed
      + num_nonstandard_blocktime_locks_closed
      + num_nonstandard_unixtime_locks_closed;
    
    total_locks_balance = standard_locks_balance
      + nonstandard_blocktime_locks_balance
      + nonstandard_unixtime_locks_balance;
  }

  void update_balances(uint32_t &standard_locks_balance_inout,
    uint32_t &nonstandard_blocktime_locks_balance_inout,
    uint32_t &nonstandard_unixtime_locks_balance_inout,
    uint32_t &total_locks_balance_inout)
  {
    this->standard_locks_balance = standard_locks_balance_inout =
      (standard_locks_balance_inout + this->num_standard_locks_opened - this->num_standard_locks_closed);

    this->nonstandard_blocktime_locks_balance = nonstandard_blocktime_locks_balance_inout =
      (nonstandard_blocktime_locks_balance_inout + this->num_nonstandard_blocktime_locks_opened - 
        this->num_nonstandard_blocktime_locks_closed);

    this->nonstandard_unixtime_locks_balance = nonstandard_unixtime_locks_balance_inout =
      (nonstandard_unixtime_locks_balance_inout + this->num_nonstandard_unixtime_locks_opened - 
        this->num_nonstandard_unixtime_locks_closed);

    this->total_locks_balance = total_locks_balance_inout =
      (total_locks_balance_inout + this->num_total_locks_opened - this->num_total_locks_closed);
  }
};

const std::string report_slot_entry_header = "num standard locks opened,num nonstandard blocktime locks opened,"
  "num nonstandard unixtime locks opened,num total locks opened,num standard locks closed,"
  "num nonstandard blocktime locks closed,num nonstandard unixtime locks closed,num total locks closed,"
  "standard locks balance,nonstandard blocktime locks balance,nonstandard unixtime locks balance,total locks balance";

std::ostream& operator<<(std::ostream &os, const report_slot_entry_t &report_slot_entry)
{
  os << report_slot_entry.num_standard_locks_opened;
  os << ',';
  os << report_slot_entry.num_nonstandard_blocktime_locks_opened;
  os << ',';
  os << report_slot_entry.num_nonstandard_unixtime_locks_opened;
  os << ',';
  os << report_slot_entry.num_total_locks_opened;
  os << ',';

  os << report_slot_entry.num_standard_locks_closed;
  os << ',';
  os << report_slot_entry.num_nonstandard_blocktime_locks_closed;
  os << ',';
  os << report_slot_entry.num_nonstandard_unixtime_locks_closed;
  os << ',';
  os << report_slot_entry.num_total_locks_closed;
  os << ',';

  os << report_slot_entry.standard_locks_balance;
  os << ',';
  os << report_slot_entry.nonstandard_blocktime_locks_balance;
  os << ',';
  os << report_slot_entry.nonstandard_unixtime_locks_balance;
  os << ',';
  os << report_slot_entry.total_locks_balance;
  //os << ',';

  return os;
}

static uint32_t get_first_spendable_block_index(
  const uint32_t unlock_time,
  const uint32_t block_included_in_chain)
{
  uint32_t unlock_block_index = 0;

  static_assert(CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE > 0, "unexpected default spendable age");
  const uint32_t default_block_index = block_included_in_chain + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE;

  if (unlock_time < CRYPTONOTE_MAX_BLOCK_NUMBER)
  {
    unlock_block_index = unlock_time;
  }
  else // unix timelock
  {
    // Interpret the unlock_time as time
    // TODO: hardcode correct times for each network and take in nettype
    const uint32_t hf_v15_time = 1656629118;
    const uint32_t hf_v15_height = 2689608;

    // Use the last hard fork's time and block combo to convert the time-based timelock into an unlock block
    // TODO: consider taking into account 60s block times when that was consensus
    if (hf_v15_time > unlock_time)
    {
      const uint32_t seconds_since_unlock = hf_v15_time - unlock_time;
      const uint32_t blocks_since_unlock = seconds_since_unlock / DIFFICULTY_TARGET_V2;

      unlock_block_index = hf_v15_height > blocks_since_unlock
        ? (hf_v15_height - blocks_since_unlock)
        : 0;
    }
    else
    {
      const uint32_t seconds_until_unlock = unlock_time - hf_v15_time;
      const uint32_t blocks_until_unlock = seconds_until_unlock / DIFFICULTY_TARGET_V2;
      unlock_block_index = hf_v15_height + blocks_until_unlock;
    }
  }

  // Can't unlock earlier than the default unlock block
  return std::max(unlock_block_index, default_block_index);
}

static uint32_t get_fcmp_tree_insertion_block_index(
  const uint32_t unlock_time,
  const uint32_t block_included_in_chain,
  const bool is_coinbase,
  const uint32_t ignore_after_block_index,
  const uint32_t fcmp_activation_index)
{
  const uint32_t effective_unlock_time =
    (is_coinbase || block_included_in_chain <= ignore_after_block_index)
      ? unlock_time : 0;

  const uint32_t first_spendable_block_index = std::min<uint32_t>(
    get_first_spendable_block_index(effective_unlock_time, block_included_in_chain),
    CRYPTONOTE_MAX_BLOCK_NUMBER);

  if (first_spendable_block_index <= fcmp_activation_index)
    return block_included_in_chain;
  else
    return first_spendable_block_index;
}

static unlock_time_type_t classify_unlock_time(const uint32_t unlock_time, const bool is_coinbase)
{
  if (unlock_time == 0 || is_coinbase) return unlock_time_type_t::Standard;
  else if (unlock_time < CRYPTONOTE_MAX_BLOCK_NUMBER) return unlock_time_type_t::NonStandardBlock;
  else return unlock_time_type_t::NonStandardUnix;
}

static bool is_coinbase_tx(epee::span<const char> tx_prefix_blob)
{
  uint32_t v;
  int nread;

  // consume version
  nread = tools::read_varint(tx_prefix_blob.data(), tx_prefix_blob.cend(), v);
  if (nread < 0)
    return false;
  tx_prefix_blob.remove_prefix(nread);

  // consume unlock_time
  nread = tools::read_varint(tx_prefix_blob.data(), tx_prefix_blob.cend(), v);
  if (nread < 0)
    return false;
  tx_prefix_blob.remove_prefix(nread);

  // consume size of vin and expect 1
  nread = tools::read_varint(tx_prefix_blob.data(), tx_prefix_blob.cend(), v);
  tx_prefix_blob.remove_prefix(nread);
  if (v != 1 || nread < 0)
    return false;

  // consume first vin type and expect txin_gen
  if (tx_prefix_blob.empty())
    return false;
  v = static_cast<unsigned char>(tx_prefix_blob[0]);
  tx_prefix_blob.remove_prefix(1);
  if (v != 0xff)
    return false;

  return true;
}

int main(int argc, char* argv[])
{
  epee::string_tools::set_module_name_and_folder(argv[0]);

  uint32_t log_level = 0;

  tools::on_startup();

  tools::signal_handler::install([](int type) {
    stop_requested.store(true);
  });

  po::options_description desc_cmd_only("Command line options");
  po::options_description desc_cmd_sett("Command line options and settings options");
  const command_line::arg_descriptor<std::string> arg_log_level = {"log-level",  "0-4 or categories", ""};
  const command_line::arg_descriptor<uint32_t> arg_ignore_index = {"ignore-index", "ignore non-standard unlock times of outputs created after this block index", CRYPTONOTE_MAX_BLOCK_NUMBER};
  const command_line::arg_descriptor<uint32_t> arg_fcmp_index = {"fcmp-index",
    "if the calculated first spendable block index is less than or equal to this block index, simulate inserting immediately", 0};
  const command_line::arg_descriptor<std::string> arg_output_file = {"output-file", "path to CSV output file", "blockchain-find-locked-output.csv"};

  command_line::add_arg(desc_cmd_sett, cryptonote::arg_data_dir);
  command_line::add_arg(desc_cmd_sett, cryptonote::arg_testnet_on);
  command_line::add_arg(desc_cmd_sett, cryptonote::arg_stagenet_on);
  command_line::add_arg(desc_cmd_sett, arg_log_level);
  command_line::add_arg(desc_cmd_sett, arg_ignore_index);
  command_line::add_arg(desc_cmd_sett, arg_fcmp_index);
  command_line::add_arg(desc_cmd_sett, arg_output_file);
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
  const bool opt_testnet = command_line::get_arg(vm, cryptonote::arg_testnet_on);
  const bool opt_stagenet = command_line::get_arg(vm, cryptonote::arg_stagenet_on);
  network_type net_type = opt_testnet ? TESTNET : opt_stagenet ? STAGENET : MAINNET;
  const uint32_t opt_ignore_index = command_line::get_arg(vm, arg_ignore_index);
  const uint32_t opt_fcmp_index = command_line::get_arg(vm, arg_fcmp_index);
  const std::string opt_output_file = command_line::get_arg(vm, arg_output_file);

  if (std::filesystem::exists(opt_output_file))
  {
    LOG_ERROR("Output file already exists");
    throw std::runtime_error("Output file already exists");
  }

  // If we wanted to use the memory pool, we would set up a fake_core.

  // Use Blockchain instead of lower-level BlockchainDB for ona main reason:
  //     Blockchain has the init() method for easy setup
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

  const std::string filename = (std::filesystem::path(opt_data_dir) / db->get_db_name()).string();
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

  std::map<uint32_t, report_slot_entry_t> report_by_block;

  LOG_PRINT_L0("Opening output file: " << opt_output_file);
  std::ofstream report_ofs;
  report_ofs.open(opt_output_file);
  CHECK_AND_ASSERT_THROW_MES(!report_ofs.fail(),
    "Could not open file '" << opt_output_file << "' for writing");

  // get number of blocks and resize standard_cache_updates accordingly
  const uint32_t db_height = db->height();

  LOG_PRINT_L0("Blockchain height: " << db_height);

  LOG_PRINT_L0("Starting main output iteration loop...");
  uint32_t last_print_height = 0;
  std::cout << "0 / " << db_height << "\r"; std::cout.flush();

  // perform main data processing on all outputs
  cryptonote::blobdata pruned_txblob;
  db->for_all_outputs([&](const rct::xmr_amount amount,
    const crypto::hash &tx_hash,
    const uint32_t block_included_in_chain,
    const size_t tx_idx,
    const uint32_t unlock_time) -> bool
    {
      // determine if this output is a coinbase output
      const bool is_potential_coinbase = unlock_time == block_included_in_chain + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW;
      bool is_coinbase = false;
      if (is_potential_coinbase)
      {
        CHECK_AND_ASSERT_THROW_MES(db->get_pruned_tx_blob(tx_hash, pruned_txblob),
          "Pruned tx fetch fail");
        is_coinbase = is_coinbase_tx(epee::to_span(pruned_txblob));

        if (!is_coinbase)
          MDEBUG("fake coinbase at txid " << tx_hash);
      }

      // calculate tree insertion index and unlock time type, sanity checking
      const uint32_t tree_insertion_index = get_fcmp_tree_insertion_block_index(unlock_time,
        block_included_in_chain,
        is_coinbase,
        opt_ignore_index,
        opt_fcmp_index);

      CHECK_AND_ASSERT_THROW_MES(tree_insertion_index >= block_included_in_chain,
        "get_fcmp_tree_insertion_block_index() trying to insert before creation");

      // get references to relevant entries
      report_slot_entry_t &open_entry = report_by_block[block_included_in_chain];
      report_slot_entry_t &close_entry = report_by_block[tree_insertion_index];      

      // depending on unlock_time_type, increment columns
      const unlock_time_type_t unlock_time_type = classify_unlock_time(unlock_time, is_coinbase);
      switch (unlock_time_type)
      {
      case unlock_time_type_t::Standard:
        ++open_entry.num_standard_locks_opened;
        ++close_entry.num_standard_locks_closed;
        break;
      case unlock_time_type_t::NonStandardBlock:
        ++open_entry.num_nonstandard_blocktime_locks_opened;
        ++close_entry.num_nonstandard_blocktime_locks_closed;
        break;
      case unlock_time_type_t::NonStandardUnix:
        ++open_entry.num_nonstandard_unixtime_locks_opened;
        ++close_entry.num_nonstandard_unixtime_locks_closed;
        break;
      }

      // print progress
      if (amount == 0 && block_included_in_chain % 1000 == 0 && block_included_in_chain > last_print_height)
      {
        std::cout << block_included_in_chain << " / " << db_height << "\r"; std::cout.flush();
        last_print_height = block_included_in_chain;
      }

      // goto next output if interrupt signal not received
      return !stop_requested.load();
    }
  );

  LOG_PRINT_L0("Finished main output iteration loop. Cumulating results...");

  uint32_t standard_locks_balance = 0;
  uint32_t nonstandard_blocktime_locks_balance = 0;
  uint32_t nonstandard_unixtime_locks_balance = 0;
  uint32_t total_locks_balance = 0;
  for (auto &p : report_by_block)
  {
    p.second.tally_totals();
    p.second.update_balances(standard_locks_balance,
      nonstandard_blocktime_locks_balance,
      nonstandard_unixtime_locks_balance,
      total_locks_balance);
  }

  LOG_PRINT_L0("Writing report to CSV file...");

  report_ofs << "block_index," << report_slot_entry_header << "\n"; // header row
  for (const auto &p : report_by_block)
  {
    report_ofs << p.first << ','; // block index col
    report_ofs << p.second;       // data columns
    report_ofs << '\n';           // newline
  }
  report_ofs << '\n';

  CHECK_AND_ASSERT_THROW_MES(!report_ofs.fail(), "writing CSV to output file failed");

  LOG_PRINT_L0("Saved report.... Done!");

  return 0;
}
